#pragma once
#include "life.hpp"
namespace axion::life::flood {
constexpr Word basal_bands = 2, nerve_page_slots = 64, spill_page = ~0u;
AX_FN Tissue developmental_band(Word development, Word band) {
    return initial_tissue(mix(development + 0x9e3779b9u * band));
}
AX_FN bool same_band(Tissue a, Tissue b) {
    return a.spike == b.spike && a.memory == b.memory && a.gate_a == b.gate_a && a.gate_b == b.gate_b;
}
#if !defined(__CUDA__)
struct NeuralRowShape {
    Word extent = 0, mask = 0;
    bool germline = false;
};
inline NeuralRowShape describe_neural_row(const Tissue *row, Word bands, Word seed) {
    NeuralRowShape r{};
    for (Word j = 0; j < bands; ++j)
        if (!same_band(row[j], {})) {
            r.extent = j + 1;
            r.mask |= 1u << j;
        }
    r.germline = r.extent != 0;
    for (Word j = 0; r.germline && j < r.extent; ++j)
        r.germline = same_band(row[j], developmental_band(seed, j));
    return r;
}
#endif
struct NervePages {
    Word *index{}, *claims{};
    Tissue *spill{};
    Word *overrides{};
    unsigned char *origin{};
    Word sites = 0, groups = 0, hot = 0, reserve = 0;
};
} // namespace axion::life::flood
