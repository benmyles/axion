#include "axion/engine.hpp"
#include "axion/journal.hpp"
#include <bit>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

namespace axion {

bool valid_shape(Word animals, Word neurons) {
    return animals > 0 && animals <= 65536 && neurons >= 64 && neurons <= (1u << 20)
        && (neurons & (neurons - 1)) == 0 && std::uint64_t(animals) * neurons <= (1ull << 32);
}

namespace {
bool valid_archive(Word slots) {
    return !slots || (slots >= 8 && slots <= 65536 && (slots & (slots - 1)) == 0);
}
bool valid_vocabulary(Word words) {
    return words >= 8 && words <= 4096 && (words & (words - 1)) == 0;
}
}

void resize_archive(Model &m, Word slots) {
    if (!valid_archive(slots)) throw std::invalid_argument("archive capacity must be zero or a power of two from 8 to 65536");
    Word old = m.course.archive_slots;
    if (slots == old) return;
    std::vector<Association> rebuilt;
    if (slots && !m.archive.empty()) {
        rebuilt.resize(size_t(slots) * m.animals);
        for (Word a = 0; a < m.animals; ++a) {
            Archive table{rebuilt.data() + size_t(a) * slots, slots};
            for (Word i = 0; i < old; ++i) {
                const auto &r = m.archive[size_t(a) * old + i];
                if (r.support) table.store(r.cue, r.outcome, r.support);
            }
        }
    }
    m.archive.swap(rebuilt);
    m.course.archive_slots = slots;
}

void validate_course(const Course &c) {
    if (Word(c.experiments) > 3 || Word(c.task) > Word(Task::canvas) || Word(c.recall) > Word(Recall::associative)
        || c.chain < 1 || c.chain > 8 || (c.task == Task::canvas && c.chain > 3)
        || c.plan_nodes > 65536 || c.novel > 1 || c.difficult > 3 || c.consolidation < 1 || c.consolidation > 1024 || c.reversed > 1
        || c.survey_steps > 128 || !valid_vocabulary(c.vocabulary))
        throw std::invalid_argument("invalid simulation course");
    if (c.task == Task::signals && (c.signals.outcome > 1 || c.signals.action < -1 || c.signals.action > 1))
        throw std::invalid_argument("invalid external feedback");
}

void check_storage(const Model &m, Word animals, Word neurons) {
    validate_course(m.course);
    if (!valid_shape(animals, neurons) || !valid_archive(m.course.archive_slots)
        || (!m.archive.empty() && m.archive.size() != size_t(animals) * m.course.archive_slots)
        || m.animals != animals || m.neurons != neurons
        || (!m.interventions.empty() && (m.interventions.size() != animals || m.drawings.empty()))
        || (!m.drawings.empty() && m.drawings.size() != animals)
        || (!m.maps.empty() && m.maps.size() != animals)
        || (!m.memory.empty() && m.memory.size() != animals)
        || m.cells.size() != size_t(animals) * neurons)
        throw std::invalid_argument("population dimensions and weight storage must match");
}

Model hatch(Word animals, Word neurons, Word seed, Course course) {
    if (!valid_shape(animals, neurons)) throw std::invalid_argument("invalid population shape");
    Model m{animals, neurons, seed, 0, 0, course, std::vector<Weights>(size_t(animals) * neurons)};
    for (Word a = 0; a < animals; ++a)
        for (Word i = 0; i < neurons; ++i)
            m.cells[size_t(a) * neurons + i] = initial_weights(seed, a, int(i));
    return m;
}

namespace {
static_assert(std::endian::native == std::endian::little);
struct Header {
    char magic[8];
    Word version, animals, neurons, seed, cursor, generation, task, world_seed, reversed;
};
static_assert(sizeof(Header) == 44);

std::uint64_t digest(const void *data, size_t size, std::uint64_t hash = 14695981039346656037ull) {
    const auto *p = static_cast<const unsigned char *>(data);
    for (size_t i = 0; i < size; ++i) hash = (hash ^ p[i]) * 1099511628211ull;
    return hash;
}

void write_all(int fd, const void *data, size_t size) {
    auto *p = static_cast<const char *>(data);
    while (size) {
        auto n = ::write(fd, p, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) throw std::system_error(errno, std::generic_category(), "write checkpoint");
        p += n;
        size -= size_t(n);
    }
}
} // namespace

void save_model(const Model &m, const std::filesystem::path &path) {
    check_storage(m, m.animals, m.neurons);
    bool extended = is_lesson(m.course.task) || m.course.lesson || !m.memory.empty() || !m.maps.empty() || !m.drawings.empty()
                 || m.course.recall != Recall::associative || m.course.scrambled || m.course.survey_steps || m.course.random_walk || m.course.chain != 3 || m.course.difficult || m.course.consolidation != 64
                 || m.course.vocabulary != 8 || m.course.archive_slots;
    bool causal = m.course.experiments != Course::Experiments::off || !m.interventions.empty();
    extended |= causal;
    Word version = causal ? 9 : (extended ? 8 : 2);
    Header h{"AXION\r\n", version, m.animals, m.neurons, m.seed, m.cursor, m.generation, Word(m.course.task),
             m.course.seed, m.course.reversed};
    size_t bytes = m.cells.size() * sizeof(Weights);
    Word mode = Word(m.learning);
    Word stores = Word(!m.memory.empty()) | (Word(!m.maps.empty()) << 1) | (Word(!m.drawings.empty()) << 2)
                | (Word(!m.archive.empty()) << 3) | (Word(!m.interventions.empty()) << 4);
    Word options = m.course.scrambled | (m.course.survey_steps << 1) | (m.course.random_walk << 9)
                 | ((m.course.chain - 1) << 10) | (m.course.difficult << 13) | ((m.course.consolidation - 1) << 16)
                 | (Word(m.course.experiments) << 26);
    Word config[] = {m.course.lesson, Word(m.course.recall), stores, options, m.course.vocabulary, m.course.archive_slots};
    size_t bank_bytes = m.memory.size() * sizeof(MemoryBank);
    size_t map_bytes = m.maps.size() * sizeof(PlaceMemory);
    size_t drawing_bytes = m.drawings.size() * sizeof(DrawingMemory);
    size_t archive_bytes = m.archive.size() * sizeof(Association);
    size_t causal_bytes = m.interventions.size() * sizeof(InterventionMemory);
    auto hash = digest(&mode, sizeof(mode), digest(&h, sizeof(h)));
    if (extended) hash = digest(config, sizeof(config), hash);
    hash = digest(m.cells.data(), bytes, hash);
    hash = digest(m.memory.data(), bank_bytes, hash);
    hash = digest(m.maps.data(), map_bytes, hash);
    hash = digest(m.drawings.data(), drawing_bytes, hash);
    hash = digest(m.archive.data(), archive_bytes, hash);
    hash = digest(m.interventions.data(), causal_bytes, hash);
    std::string tmp = path.string() + ".XXXXXX";
    int fd = ::mkstemp(tmp.data());
    if (fd < 0) throw std::system_error(errno, std::generic_category(), "create checkpoint");
    try {
        write_all(fd, &h, sizeof(h));
        write_all(fd, &mode, sizeof(mode));
        if (extended) write_all(fd, config, sizeof(config));
        write_all(fd, m.cells.data(), bytes);
        write_all(fd, m.memory.data(), bank_bytes);
        write_all(fd, m.maps.data(), map_bytes);
        write_all(fd, m.drawings.data(), drawing_bytes);
        write_all(fd, m.archive.data(), archive_bytes);
        write_all(fd, m.interventions.data(), causal_bytes);
        write_all(fd, &hash, sizeof(hash));
        if (::fsync(fd) != 0) throw std::system_error(errno, std::generic_category(), "flush checkpoint");
        int rc = ::close(fd);
        fd = -1;
        if (rc != 0) throw std::system_error(errno, std::generic_category(), "close checkpoint");
        std::filesystem::rename(tmp, path);
        journal::checkpoint(path.string(),hash,true);
    } catch (...) {
        if (fd >= 0) ::close(fd);
        ::unlink(tmp.c_str());
        throw;
    }
}

Model load_model(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("cannot open checkpoint " + path.string());
    auto size = file.tellg();
    file.seekg(0);
    Header h{};
    if (!file.read(reinterpret_cast<char *>(&h), sizeof(h)) || std::memcmp(h.magic, "AXION\r\n", 8)
        || (h.version < 1 || h.version > 9) || !valid_shape(h.animals, h.neurons) || h.task > Word(Task::detector)
        || h.reversed > 1) throw std::runtime_error("invalid checkpoint header");
    Word mode = Word(Learning::full);
    auto header_hash = digest(&h, sizeof(h));
    if (h.version >= 2) {
        if (!file.read(reinterpret_cast<char *>(&mode), sizeof(mode)) || mode > Word(Learning::no_scaling))
            throw std::runtime_error("invalid checkpoint learning mode");
        header_hash = digest(&mode, sizeof(mode), header_hash);
    }
    size_t header_size = sizeof(h) + (h.version >= 2 ? sizeof(mode) : 0);
    Word config[] = {0, Word(Recall::associative), 0, 0, 8, 0};
    if (h.version >= 3) {
        size_t config_bytes = sizeof(Word) * (h.version < 8 ? 4 : 6);
        if (!file.read(reinterpret_cast<char *>(config), config_bytes))
            throw std::runtime_error("incomplete episodic memory header");
        Word stores = config[2], opts = config[3];
        Word allowed_opts = h.version < 5 ? 0x3ffu : (h.version == 5 ? 0x3fffu : 0x7fffu);
        if (h.version >= 7) allowed_opts |= 0x3ff0000u;
        if (h.version >= 9) allowed_opts |= 3u << 26;
        Word allowed_stores = h.version < 5 ? 3 : (h.version < 8 ? 7 : (h.version == 8 ? 15 : 31));
        if (config[1] > Word(Recall::associative) || stores > allowed_stores
            || !valid_vocabulary(config[4]) || !valid_archive(config[5]) || ((stores & 8) && !config[5])
            || ((stores & 16) && !(stores & 4))
            || (opts & ~allowed_opts) || ((opts >> 1) & 255) > 128)
            throw std::runtime_error("invalid episodic memory header");
        header_hash = digest(config, config_bytes, header_hash);
        header_size += config_bytes;
    }
    size_t bank_count = (config[2] & 1) ? h.animals : 0;
    size_t map_count = (config[2] & 2) ? h.animals : 0;
    size_t map_bytes = map_count * sizeof(PlaceMemory);
    size_t drawing_count = (config[2] & 4) ? h.animals : 0;
    size_t drawing_stride = h.version < 6 ? offsetof(DrawingMemory, spans) : sizeof(DrawingMemory);
    size_t drawing_bytes = drawing_count * drawing_stride;
    size_t bank_bytes = bank_count * sizeof(MemoryBank);
    size_t archive_count = (config[2] & 8) ? size_t(h.animals) * config[5] : 0;
    size_t archive_bytes = archive_count * sizeof(Association);
    size_t causal_count = (config[2] & 16) ? h.animals : 0;
    size_t causal_bytes = causal_count * sizeof(InterventionMemory);
    size_t count = size_t(h.animals) * h.neurons;
    size_t bytes = count * sizeof(Weights);
    if (size != std::streamoff(header_size + bytes + bank_bytes + map_bytes + drawing_bytes + archive_bytes + causal_bytes + sizeof(std::uint64_t)))
        throw std::runtime_error("invalid checkpoint length");
    Model model{h.animals, h.neurons, h.seed, h.cursor, h.generation, {Task(h.task), h.world_seed, h.reversed, 0},
                std::vector<Weights>(count)};
    model.learning = Learning(mode);
    model.course.lesson = config[0];
    model.course.vocabulary = config[4];
    model.course.archive_slots = config[5];
    model.archive.resize(archive_count);
    model.course.recall = Recall(config[1]);
    model.course.scrambled = config[3] & 1;
    model.course.survey_steps = (config[3] >> 1) & 255;
    model.course.random_walk = (config[3] >> 9) & 1;
    model.course.chain = h.version >= 5 ? 1 + ((config[3] >> 10) & 7) : 3;
    if (h.version >= 3) model.course.consolidation = h.version >= 7 ? 1 + ((config[3] >> 16) & 1023) : 1;
    model.course.experiments = Course::Experiments(config[3] >> 26);
    model.course.difficult = (config[3] >> 13) & 3;
    model.drawings.resize(drawing_count);
    model.interventions.resize(causal_count);
    model.memory.resize(bank_count);
    model.maps.resize(map_count);
    std::uint64_t expected;
    if (!file.read(reinterpret_cast<char *>(model.cells.data()), bytes)
        || (bank_bytes && !file.read(reinterpret_cast<char *>(model.memory.data()), bank_bytes))
        || (map_bytes && !file.read(reinterpret_cast<char *>(model.maps.data()), map_bytes)))
        throw std::runtime_error("incomplete checkpoint");
    for (auto &drawing : model.drawings)
        if (!file.read(reinterpret_cast<char *>(&drawing), drawing_stride)) throw std::runtime_error("incomplete drawing");
    if ((archive_bytes && !file.read(reinterpret_cast<char *>(model.archive.data()), archive_bytes))
        || (causal_bytes && !file.read(reinterpret_cast<char *>(model.interventions.data()), causal_bytes))
        || !file.read(reinterpret_cast<char *>(&expected), sizeof(expected)))
        throw std::runtime_error("checkpoint checksum mismatch");
    auto hash = digest(model.cells.data(), bytes, header_hash);
    hash = digest(model.memory.data(), bank_bytes, hash);
    hash = digest(model.maps.data(), map_bytes, hash);
    for (const auto &drawing : model.drawings) hash = digest(&drawing, drawing_stride, hash);
    hash = digest(model.archive.data(), archive_bytes, hash);
    hash = digest(model.interventions.data(), causal_bytes, hash);
    if (expected != hash) throw std::runtime_error("checkpoint checksum mismatch");
    for (const auto &r : model.archive)
        if (r.support > 255) throw std::runtime_error("invalid association strength");
    for (const auto &drawing : model.drawings)
        if (drawing.palette.used > tools_per_brain || drawing.palette.next >= tools_per_brain
            || (drawing.palette.used < tools_per_brain && drawing.palette.next != drawing.palette.used))
            throw std::runtime_error("invalid tool memory");
    for (const auto &map : model.maps) {
        if (map.used > 32) throw std::runtime_error("invalid place count");
        Word occupied = map.used == 32 ? ~0u : (1u << map.used) - 1;
        for (Word i = 0; i < map.used; ++i)
            for (Word exit : map.places[i].exits.arm)
                if ((exit & ~occupied) || (exit & (exit - 1))) throw std::runtime_error("invalid learned edge");
    }
    // Preserve arithmetic bounds even for externally produced checkpoints.
    for (auto &w : model.cells)
        for (int x : w.motor)
            if (x < -2048 || x > 2048) throw std::runtime_error("motor weight outside model range");
    for (auto &bank : model.memory) {
        if (bank.used > memory_rows || bank.next >= memory_rows
            || (bank.used < memory_rows && bank.next != bank.used))
            throw std::runtime_error("invalid episodic memory cursor");
        for (int e : bank.evidence)
            if (e < -384 || e > 384) throw std::runtime_error("invalid attention evidence");
        for (Word i = 0; i < bank.used; ++i) {
            auto row = bank.rows[i];
            row.bind();
            if (h.version == 3) bank.rows[i].geometry = row.geometry;
            else if (row.geometry != bank.rows[i].geometry) throw std::runtime_error("invalid relation code");
        }
    }
    journal::checkpoint(path.string(),hash,false);
    return model;
}
} // namespace axion
