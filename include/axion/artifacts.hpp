#pragma once
#include "material.hpp"

namespace axion::life::material {
constexpr Word assay_size = 64;
struct Fragment {
    Grain grain{};
    Byte neighbor[4]{64, 64, 64, 64};
    Word site = 0;
};
struct Assembly {
    Fragment fragments[assay_size]{};
    Word root = 0, size = 0, side = 0, first = 0, second = 0, sink = 0;
};
struct Response {
    Word product = 0, late = 0, first = 0, absorbed = 0, redirections = 0, stored = 0, thermal = 0;
};
} // namespace axion::life::material
