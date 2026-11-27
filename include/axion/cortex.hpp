#pragma once
#include "life.hpp"

namespace axion::life::flood {
struct GrowthGenes {
    Word response = 0, regulation = 0;
};
struct Cortex {
    GrowthGenes genes{};
    unsigned char active = 0, natal = 0, peak = 0, least = 0;
};
struct GrowthTotals {
    Count recruited = 0, retired = 0, births = 0, deaths = 0;
    Count larger_births = 0, smaller_births = 0, investment = 0;
};
struct Neurogenesis {
    Cortex *resident = nullptr;
    GrowthTotals *ledger = nullptr;
    Word open = 1, mute_new = 0, fixed_cohort = 0;
};
AX_FN Word conducting_bands(const Cortex &state, bool silence) {
    return silence && state.active > state.natal ? state.natal : state.active;
}
AX_FN GrowthGenes growth_founder(Word key) { return {mix(key ^ 0xda9b31d7u), mix(key ^ 0x133eb89du)}; }
AX_FN Cortex hatch_cortex(GrowthGenes genes, Word count) {
    auto n = static_cast<unsigned char>(count);
    return {genes, n, n, n, n};
}
AX_FN bool viable_cortex(const Cortex &p, Word capacity) {
    return p.least >= 2 && p.least <= p.active && p.least <= p.natal && p.peak >= p.active && p.peak >= p.natal &&
           p.peak <= capacity;
}
static_assert(sizeof(Cortex) == 12 && sizeof(GrowthTotals) == 7 * sizeof(Count));
} // namespace axion::life::flood
