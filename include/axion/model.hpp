#pragma once

// Compiled both as standard host C++20 and freestanding CUDA C++20.
#if defined(__CUDA__)
#define AX_FN __attribute__((device)) inline __attribute__((always_inline))
#else
#define AX_FN inline
#endif

namespace axion {

using Word = unsigned int;
static_assert(sizeof(Word) == 4 && sizeof(short) == 2);

AX_FN int bound(int value, int low, int high) {
    return value < low ? low : (value > high ? high : value);
}

AX_FN Word mix(Word value) {
    value ^= value >> 16;
    value *= 0x21f0aaadu;
    value ^= value >> 15;
    value *= 0x735a2d97u;
    return value ^ (value >> 15);
}

} // namespace axion
