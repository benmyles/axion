#pragma once
#include "axion/life.hpp"
namespace axion::life::pods {
struct TrayReading {
    Word light = 0, site = 64, muscle = 0, bites = 0, opened = 0, edible[3]{}, husks = 0, ticks = 0, ambiguous = 0,
         aligned = 0, surfaces = 0;
};
} // namespace axion::life::pods
