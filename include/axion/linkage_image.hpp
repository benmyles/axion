#pragma once
#include "linkage.hpp"
namespace axion::life::linkage {
struct Panel {
    Map model{};
    Rig rig{};
    Pattern target{};
    Genes body{};
    Word view = 0, owner = absent;
};
struct Audit {
    Word residents = 0, grains = 0, wet = 0, ready = 0, purpose = 0, possible = 0, requested = 0, productive = 0,
         indirect = 0, motion = 0, forecasts = 0, known = 0, correct = 0, marked = 0, old = 0, hydrated = 0;
};
} // namespace axion::life::linkage
