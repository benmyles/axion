#pragma once
#include "axion/retina_study.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace axion::life::retina {
inline std::vector<Surface> read_surfaces(const std::string &path) {
    std::ifstream f(path);
    std::string format;
    if (!(f >> format) || format != "AXION-WATERCOURSES-1")
        throw std::invalid_argument("invalid watercourse collection");
    std::vector<retina::Surface> sites;
    while (f >> std::ws && !f.eof()) {
        retina::Surface site{};
        Word tide;
        auto &v = site.material;
        if (!(f >> site.seed >> site.side >> site.tile >> site.tick >> site.law >> tide >> v.phase >> v.rock) ||
            tide > 16 || !std::has_single_bit(site.side) || site.side < 4 || site.side > 1024 ||
            site.tile >= site.side * site.side / 4 || v.phase > 3 || sites.size() == 65536)
            throw std::invalid_argument("invalid tile in the recorded watercourse collection");
        auto read = [&](auto &edges) {
            for (auto &e : edges) {
                Word x;
                if (!(f >> x) || x > 65535)
                    throw std::invalid_argument("unreadable physical surface");
                e = x;
            }
        };
        read(v.bed.water);
        read(v.bed.clay);
        read(v.bed.fibre);
        Word x = (2 * (site.tile % (site.side / 2)) + ((site.tick >> 3) & 1)) & (site.side - 1);
        Word y = (2 * (site.tile / (site.side / 2)) + ((site.tick >> 4) & 1)) & (site.side - 1);
        Word rock = mix(site.seed ^ mix((x >> 3) + 19391 * (y >> 3)));
        Word phase = ((rock >> 13) + ((site.law & drift::tide) ? site.tick >> tide : 0)) & 3;
        if (rock != v.rock || phase != v.phase)
            throw std::invalid_argument("the retained bed disagrees with its field coordinates");
        sites.push_back(site);
    }
    if (sites.empty())
        throw std::invalid_argument("the collection contains no observed watercourse");
    return sites;
}
} // namespace axion::life::retina
