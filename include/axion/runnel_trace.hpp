#pragma once
#include "runnel.hpp"
namespace axion::life::runnel {
struct ReachFrame {
    Word generation = 0, cursor = 0, work = 0, food = 0, water = 0;
    Byte joints[max_span]{};
};
} // namespace axion::life::runnel
