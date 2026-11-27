#pragma once
#include "axion/life.hpp"
#include "axion/journal.hpp"
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <unistd.h>

namespace axion::life::storage {
inline Count digest(Count hash, const void *p, size_t length) {
    auto bytes = static_cast<const unsigned char *>(p);
    for (size_t i = 0; i < length; ++i) hash = (hash ^ bytes[i]) * 1099511628211ull;
    return hash;
}
constexpr Count hash_start = 14695981039346656037ull;
template<class Image>
void write_checked(const std::string &path, Image &s) {
    std::string tmp = path + ".XXXXXX";
    int fd = ::mkstemp(tmp.data());
    if (fd < 0) throw std::system_error(errno, std::generic_category(), "create checkpoint");
    try {
        auto write_all = [&](const void *data, size_t bytes) {
            auto p = static_cast<const char *>(data);
            while (bytes) {
                ssize_t n = ::write(fd, p, bytes);
                if (n < 0 && errno == EINTR) continue;
                if (n <= 0) throw std::system_error(errno, std::generic_category(), "write checkpoint");
                p += n; bytes -= size_t(n);
            }
        };
        Count hash = hash_start;
        s.chunks([&](void *p, size_t bytes) { write_all(p, bytes); hash = digest(hash, p, bytes); });
        write_all(&hash, sizeof(hash));
        if (::fsync(fd)) throw std::system_error(errno, std::generic_category(), "flush checkpoint");
        int rc = ::close(fd); fd = -1;
        if (rc) throw std::system_error(errno, std::generic_category(), "close checkpoint");
        std::filesystem::rename(tmp, path);
        journal::checkpoint(path,hash,true);
        if constexpr (requires {s.specimen(path,"saved",hash);}) s.specimen(path,"saved",hash);
    } catch (...) {
        if (fd >= 0) ::close(fd);
        ::unlink(tmp.c_str()); throw;
    }
}

} // namespace axion::life::storage
