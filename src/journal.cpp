#include "axion/journal.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" const char axion_journal_script[];

namespace axion::journal {
namespace {
thread_local Report *active = nullptr;
namespace fs = std::filesystem;

void quoted(std::ostream &out, std::string_view s) {
    out << '"';
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c < 32) out << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << int(c) << std::dec;
        else out << c;
    }
    out << '"';
}

template <class T, class F> void array(std::ostream &s, const T &items, F put) {
    bool first = true;
    s << '[';
    for (const auto &value : items) {
        if (!first) s << ',';
        put(value);
        first = false;
    }
    s << ']';
}

std::string encode(const Report &r) {
    std::ostringstream s;
    s << "{\"version\":1,\"command\":";
    quoted(s, r.command);
    for (auto pair : {std::pair{"source", &r.source}, {"device", &r.device}, {"archive", &r.archive}}) {
        s << ',';
        quoted(s, pair.first);
        s << ':';
        quoted(s, *pair.second);
    }
    s << ",\"args\":";
    array(s, r.args, [&](const auto &v) { quoted(s, v); });
    s << ",\"checkpoints\":";
    array(s, r.checkpoints, [&](const Checkpoint &file) {
        s << "{\"path\":";
        quoted(s, file.path);
        s << ",\"saved\":" << (file.saved ? "true" : "false") << ",\"checksum\":\""
          << std::hex << std::setw(16) << std::setfill('0') << file.checksum << std::dec << "\"}";
    });
    s << ",\"lines\":";
    array(s, r.lines, [&](const auto &v) { quoted(s, v); });
    s << ",\"metrics\":{";
    bool first = true;
    for (const auto &[name, v] : r.metrics) {
        if (!first) s << ',';
        first = false;
        quoted(s, name);
        s << ':' << v;
    }
    s << "}}";
    return s.str();
}

class Transcript : public std::streambuf {
    std::streambuf *original;
    Report &r;
    std::string pending;
    void take(const char *data, std::streamsize n) {
        while (n) {
            const auto *end = static_cast<const char *>(std::memchr(data, '\n', size_t(n)));
            auto length = end ? end - data : n;
            pending.append(data, size_t(std::min<std::streamsize>(length, 4096 - pending.size())));
            if (!end) return;
            r.line(std::move(pending));
            pending.clear();
            n -= length + 1;
            data = end + 1;
        }
    }

  protected:
    std::streamsize xsputn(const char *p, std::streamsize n) override {
        auto written = original->sputn(p, n);
        take(p, written);
        return written;
    }
    int overflow(int c) override {
        if (c == traits_type::eof()) return traits_type::not_eof(c);
        char b = char(c);
        return xsputn(&b, 1) == 1 ? c : traits_type::eof();
    }
    int sync() override { return original->pubsync(); }

  public:
    explicit Transcript(Report &report) : original(std::cout.rdbuf()), r(report) { std::cout.rdbuf(this); }
    ~Transcript() {
        std::cout.rdbuf(original);
        if (!pending.empty()) r.line(std::move(pending));
    }
};

fs::path root_dir() {
    if (const char *p = std::getenv("AXION_JOURNAL_DIR"); p && *p) return fs::absolute(p);
    std::error_code ec;
    auto binary = fs::read_symlink("/proc/self/exe", ec);
    if (!ec && fs::exists(binary.parent_path() / ".git") && fs::exists(binary.parent_path() / "include/axion"))
        return binary.parent_path() / "journal";
    return fs::absolute("journal");
}

int renderer(const fs::path &root, std::vector<std::string> args, const std::string *payload) {
    fs::create_directories(root);
    fs::path receipt;
    struct QueueInput {
        int fd = -1;
        ~QueueInput() {
            if (fd >= 0) close(fd);
        }
    } source;
    if (payload) {
        std::string path = (root / ".write-XXXXXX").string();
        int fd = mkstemp(path.data());
        if (fd < 0) throw std::runtime_error("cannot allocate a journal receipt: " + std::string(std::strerror(errno)));
        std::FILE *file = fdopen(fd, "wb");
        if (!file) {
            close(fd);
            unlink(path.c_str());
            throw std::runtime_error("cannot open journal receipt");
        }
        bool written = std::fwrite(payload->data(), 1, payload->size(), file) == payload->size() &&
                       std::fflush(file) == 0 && fsync(fd) == 0;
        if (std::fclose(file) != 0 || !written) {
            unlink(path.c_str());
            throw std::runtime_error("cannot write journal evidence");
        }
        source.fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (source.fd < 0) {
            unlink(path.c_str());
            throw std::runtime_error("cannot read completed journal receipt");
        }
        receipt = root / (".receipt-" + std::to_string(getpid()) + "-" + fs::path(path).filename().string().substr(7));
        for (unsigned i = 0; link(path.c_str(), receipt.c_str()) < 0; ++i) {
            if (errno != EEXIST) {
                unlink(path.c_str());
                throw std::runtime_error("cannot queue journal evidence");
            }
            receipt = root / (".receipt-" + std::to_string(getpid()) + "-" +
                              fs::path(path).filename().string().substr(7) + "-" + std::to_string(i));
        }
        unlink(path.c_str());
        args.push_back(receipt.filename().string());
        args.push_back(std::to_string(source.fd));
    }
    std::string folder = root.string(), binary = fs::read_symlink("/proc/self/exe").string();
    QueueInput script;
    script.fd = memfd_create("axion-journal", MFD_CLOEXEC);
    if (script.fd < 0) throw std::runtime_error("cannot open embedded journal renderer");
    const char *part = axion_journal_script;
    size_t remaining = std::strlen(part);
    while (remaining) {
        ssize_t sent = write(script.fd, part, remaining);
        if (sent < 0 && errno == EINTR) continue;
        if (sent <= 0) throw std::runtime_error("cannot prepare embedded journal renderer");
        part += sent;
        remaining -= size_t(sent);
    }
    std::string program = "/proc/self/fd/" + std::to_string(script.fd);
    std::vector<char *> words{const_cast<char *>("python3"), program.data(), folder.data(), binary.data()};
    for (auto &arg : args) words.push_back(arg.data());
    words.push_back(nullptr);
    std::cout.flush();
    pid_t child = fork();
    if (child == 0) {
        if (fcntl(script.fd, F_SETFD, 0) < 0) _exit(127);
        if (source.fd >= 0 && fcntl(source.fd, F_SETFD, 0) < 0) _exit(127);
        execvp(words[0], words.data());
        _exit(127);
    }
    if (child < 0) throw std::runtime_error("cannot start the journal renderer");
    int status = 0;
    while (waitpid(child, &status, 0) < 0)
        if (errno != EINTR) throw std::runtime_error("cannot wait for journal renderer");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        throw std::runtime_error("journal rendering failed in " + folder +
                                 (payload ? ". Run axion journal rebuild to publish queued observations." : ""));
    return 0;
}
} // namespace

Report *current() { return active; }

void checkpoint(const std::string &path, Count checksum, bool saved) {
    if (!active) return;
    for (const auto &file : active->checkpoints)
        if (file.path == path && file.checksum == checksum && file.saved == saved) return;
    active->checkpoints.push_back({path, checksum, saved});
}

void Report::line(std::string entry) {
    if (entry.empty()) return;
    if (lines.size() >= 2048u) lines.erase(lines.begin() + 32);
    lines.push_back(std::move(entry));
}

int execute(int argc, char **argv, const std::function<int(int, char **)> &command) {
    if (argc < 2) return command(argc, argv);
    fs::path folder = root_dir();
    std::vector<char *> words{argv[0]};
    Report r;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--journal-dir") {
            if (++i >= argc) throw std::invalid_argument("--journal-dir needs a directory");
            folder = fs::absolute(argv[i]);
        } else {
            words.push_back(argv[i]);
        }
    }
    if (words.size() < 2) return command(int(words.size()), words.data());
    int n = int(words.size());
    words.push_back(nullptr);
    r.command = words[1];
    for (int i = 1; i < n; ++i) r.args.emplace_back(words[i]);
    if (r.command == "journal")
        return renderer(folder, std::vector<std::string>(r.args.begin() + 1, r.args.end()), nullptr);
    if (std::find(r.args.begin(), r.args.end(), "--help") != r.args.end() || r.command == "--help" || r.command == "help")
        return command(n, words.data());
    if (const char *archive = std::getenv("AXION_JOURNAL_ARCHIVE_KEY"); archive && *archive) r.archive = archive;
    for (int i = 2; i + 1 < n; ++i)
        if (std::string_view(words[i]) == "--load") r.source = words[i + 1];
    struct Scope {
        Scope(Report &v) { active = &v; }
        ~Scope() { active = nullptr; }
    };
    int result;
    auto start = std::chrono::steady_clock::now();
    {
        Scope scope(r);
        Transcript tee(r);
        result = command(n, words.data());
    }
    r.metrics["command_wall_ns"] =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
    if (!result) {
        auto data = encode(r);
        renderer(folder, {"record"}, &data);
    }
    return result;
}
} // namespace axion::journal
