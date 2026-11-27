#pragma once
#include "utterance.hpp"

namespace axion::life::detritus {
using Half = unsigned short;
constexpr Word gear = 16, displaced = 32, modes = gear | displaced, exchanges = 64;

// A rotated two-by-two partition owns its four food stocks exclusively for one beat.
struct Bed {
    Half food[4]{}, water[4]{}, sill[4]{}, seam[4]{};
};
struct Motion {
    Word offered = 0, moved = 0;
};
AX_FN int drop(const Bed &b, Word source, Word target, Word edge) {
    return int(b.water[source]) - int(b.water[target]) + 2 + int(b.seam[edge] > 7 ? 7 : b.seam[edge]) -
           int(b.sill[edge] > 31 ? 31 : b.sill[edge]);
}
AX_FN Motion route(const Bed &b, Word gates, Word priority) {
    // Edges in order: NW to NE, NW to SW, NE to SE, and SW to SE.
    int east = drop(b, 0, 1, 0), south = drop(b, 0, 2, 1);
    Word offers = 0;
    if ((gates & 1) && b.food[0] && b.water[0] && (east > 0 || south > 0))
        offers |= (east > south || (east == south && !(priority & 1))) ? 1 : 2;
    if ((gates & 2) && b.food[1] && b.water[1] && drop(b, 1, 3, 2) > 0)
        offers |= 4;
    if ((gates & 4) && b.food[2] && b.water[2] && drop(b, 2, 3, 3) > 0)
        offers |= 8;
    Word moved = offers;
    if (b.food[1] == 65535)
        moved &= ~1u;
    if (b.food[2] == 65535)
        moved &= ~2u;
    if (b.food[3] == 65535)
        moved &= ~12u;
    else if (b.food[3] == 65534 && (offers & 12) == 12)
        moved &= ~((priority & 2) ? 4u : 8u);
    return {offers, moved};
}
AX_FN Word head(Word edge) {
    return edge == 0 ? 1u : edge == 1 ? 2u : 3u;
}
AX_FN Word tail(Word edge) {
    return edge < 2 ? 0u : edge - 1;
}
AX_FN int balance(Word transfers, Word site) {
    Word into = site == 1 ? 1 : site == 2 ? 2 : site == 3 ? 12 : 0;
    Word from = site == 0 ? 3 : site == 1 ? 4 : site == 2 ? 8 : 0;
    return __builtin_popcount(transfers & into) - __builtin_popcount(transfers & from);
}
// A donor sends its original surface film before accepting any upstream grain.
AX_FN Word stain(Word palette, Word moves, Word priority) {
    Word lower_first = (priority & 2) ? 3 : 2;
    for (Word k = 0; k < 2; ++k) {
        Word e = lower_first ^ k;
        if (moves & (1u << e))
            palette = utterance::swap_ink(palette, tail(e), head(e));
    }
    if (moves & 3) {
        Word e = (moves & 1) ? 0 : 1;
        palette = utterance::swap_ink(palette, tail(e), head(e));
    }
    return palette;
}
struct Ledger {
    Count tiles = 0, gates = 0, stocked = 0, offers = 0, departures = 0, arrivals = 0, blocked = 0, east = 0, south = 0,
          stimulus_changes = 0, food_enabled = 0, food_prevented = 0, mass_before = 0, mass_after = 0, recoloured = 0;
};
struct Note {
    Bed bed{};
    Half final[4]{};
    Word tick = 0, tile = 0, x = 0, y = 0, epoch = 0, mode = 0, local = 0, gates = 0, changes = 0, priority = 0,
         offered = 0, moved = 0, alternate[3]{}, actor[3]{}, born[3]{}, surface = 0, paint_before = 0, paint_after = 0;
};
struct Recording {
    Note *notes = nullptr;
    Word uniform = 0, watched = 0;
};
} // namespace axion::life::detritus

namespace axion::life::detritus {
constexpr Word proof_cases = 1u << 16;
struct Proof {
    Count final = 0;
    Word offered = 0, moved = 0;
};
AX_FN Bed boundary_case(Word id) {
    constexpr Half stock[]{0, 1, 65534, 65535};
    constexpr Half depths[][4] = {{2, 2, 2, 2}, {0, 4, 0, 9},     {65535, 0, 65535, 1}, {3, 3, 8, 1},
                                  {4, 3, 2, 1}, {80, 35, 32, 63}, {1, 4, 1, 6},         {65532, 65534, 65535, 65533}};
    Word shape = id >> 13;
    Bed bed{};
    for (Word j = 0; j < 4; ++j) {
        bed.food[j] = stock[(id >> (2 * j)) & 3];
        bed.water[j] = depths[shape][j];
        bed.sill[j] = !shape ? 0u : (shape + j) % 5 == 0 ? 65535u : (shape * 5 + j * 7) & 15;
        bed.seam[j] = !shape ? 0u : (shape + j) % 4 == 0 ? 65535u : (shape * 9 + j * 11) & 7;
    }
    return bed;
}
} // namespace axion::life::detritus
