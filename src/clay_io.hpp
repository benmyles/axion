#pragma once
#include "axion/clay.hpp"
#include "life_io.hpp"
#include <cstring>
#include <fstream>
namespace axion::life::clay {
struct Collection {
    char magic[8] = {'A', 'X', 'C', 'L', 'A', 'Y', 0, 0};
    Word version = 1, clock = 0;
    Tile objects[2]{};
    Word glaze[2]{};
    template <class F> void chunks(F f) { f(this, sizeof(*this)); }
};
inline void save_collection(const std::string &name, Collection c) { storage::write_checked(name, c); }
inline Collection read_collection(const std::string &name) {
    std::ifstream file(name, std::ios::binary | std::ios::ate);
    if (!file || file.tellg() != std::streamoff(sizeof(Collection) + sizeof(Count)))
        throw std::invalid_argument("invalid clay collection extent");
    file.seekg(0);
    Collection c;
    Count hash = 0;
    if (!file.read(reinterpret_cast<char *>(&c), sizeof(c)) ||
        !file.read(reinterpret_cast<char *>(&hash), sizeof(hash)) ||
        hash != storage::digest(14695981039346656037ull, &c, sizeof(c)) || std::memcmp(c.magic, "AXCLAY\0", 8) ||
        c.version != 1)
        throw std::invalid_argument("invalid clay collection");
    for (Word j = 0; j < 2; ++j)
        if (c.glaze[j] > 255 || !valid(c.objects[j], c.clock))
            throw std::invalid_argument("invalid clay surface");
    return c;
}
} // namespace axion::life::clay
