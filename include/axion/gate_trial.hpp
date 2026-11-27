#pragma once
#include "life.hpp"

namespace axion::life::flood {
struct GateSubject {
    Word born = 0;
    unsigned short opening_energy = 0, acquired = 0, changed = 0;
    unsigned char arm = 0, reserved = 0;
};
static_assert(sizeof(GateSubject) == 12);
} // namespace axion::life::flood
