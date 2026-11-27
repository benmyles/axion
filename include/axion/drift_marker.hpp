#pragma once
#include "model.hpp"
namespace axion::life::drift {
struct BottomAccess {
    unsigned long long attempted = 0, obstructed = 0, supported = 0, immediate_food = 0;
};
struct Mark {
    Word owner = 0, born = 0, stamp = 0, shape = 0, limbs = 0, eyes = 0, pores = 0, cells = 0;
};
struct Surfaces {
    Mark *items{};
    Word watched = 0, salt = 0;
};
AX_FN Word surface(Word side, Word origin, Word axis, Word salt) {
    return ((2 * origin + axis) * 0x9e3779b1u + salt) & (2 * side * side - 1);
}
AX_FN void disturb(Surfaces surfaces, Word side, Word origin, Word axis) {
    if (surfaces.items) {
        Word k = surface(side, origin, axis, surfaces.salt);
        if (k < surfaces.watched)
            surfaces.items[k] = {};
    }
}
} // namespace axion::life::drift
