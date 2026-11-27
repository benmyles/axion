#pragma once
#include "axion/floodplain.hpp"
#include "grounding_eye.cuh"
namespace axion::life::flood {
AX_FN Word cut(Word a, Word b) { return a < b ? a : b; }
AX_FN Word max2(Word a, Word b) { return a > b ? a : b; }
AX_FN Word nbits(Word x) { return Word(__builtin_popcount(x)); }
AX_FN Anatomy grow(const Germ &g) {
    Anatomy a;
    a.shape = 1u << center;
    a.pores = a.shape;
    Word n = 1 + (g.growth & 15), pos = center, saved = center, dir = (g.growth >> 4) & 3;
    for (Word k = 0; k < n; ++k) {
        Word codon = (g.buds[k / 4] >> (8 * (k & 3))) & 255;
        Word start = (codon & 3) == 1 ? center : (codon & 3) == 2 ? saved : pos;
        if ((codon & 3) == 3)
            saved = pos;
        dir = (dir + ((codon >> 2) & 3)) & 3;
        int x = int(start % 5) + (dir == 1) - (dir == 3), y = int(start / 5) + (dir == 2) - (dir == 0);
        if (x < 0 || x > 4 || y < 0 || y > 4)
            continue;
        pos = Word(x + 5 * y);
        Word bit = 1u << pos;
        a.shape |= bit;
        if (codon & 16)
            a.limbs |= bit;
        if (codon & 32)
            a.pores |= bit;
        if (codon & 64)
            a.eyes |= bit;
        if (codon & 128)
            saved = pos;
    }
    return a;
}
AX_FN Word segment(Word shape, Word dir) {
    constexpr Word outer[]{0x1fu, 0x1084210u, 0x1f00000u, 0x108421u};
    constexpr Word near[]{0x3e0u, 0x0842108u, 0x0f8000u, 0x0210842u};
    return (shape & outer[dir]) ? 2 : Word(bool(shape & near[dir]));
}
AX_FN Word terrain_height(Word site, Configuration c) {
    Word x = site & (c.side - 1), y = site / c.side;
    return ((mix(c.seed + (x / 4) * 137 + (y / 4) * 4271) >> 11) & 15) + ((x + 2 * y + (c.seed & 31)) & 15);
}
AX_FN int tide(Word site, Word tick, Configuration c) {
    Word season = tick / c.season, axis = (season / 4) & 1, along = axis ? site / c.side : site & (c.side - 1);
    Word phase = (along + season * 3) & 31;
    return int(phase < 16 ? phase : 31 - phase) * 2;
}
AX_FN Word optical(Silt s, Word animal) {
    return Word(s.water >= 24) | 2 * Word(s.grains >= 12) | 4 * Word(s.leaf >= 12) | 8 * Word(s.fruit >= 6) |
           16 * Word(s.litter >= 5) | 32 * Word(s.east + s.south + s.fibre_east + s.fibre_south >= 3) |
           64 * Word(s.enzyme != 0) | 128 * Word(animal != 0);
}
AX_FN Word pigments(const Germ &g, Word light) {
    // Eight heritable coincidence detectors and sixteen single-channel
    // detectors supply candidate meanings for a locally heard pulse.
    Word single = light | ((~light & 255) << 8), mixed = 0;
    for (Word i = 0; i < 8; ++i) {
        Word gene = (g.pigments[i / 2] >> (16 * (i & 1))) & 65535, mask = gene & 255, value = (gene >> 8) & mask;
        mixed |= Word((light & mask) == value) << (16 + i);
    }
    return single | mixed | ((single & 255) << 24);
}
AX_FN Word bed_edge(Silt s, Word dir, Silt other) {
    return dir == 1   ? s.east + s.fibre_east
           : dir == 2 ? s.south + s.fibre_south
           : dir == 3 ? other.east + other.fibre_east
                      : other.south + other.fibre_south;
}
AX_FN Word view(Word pos, const World &w, Configuration c, Word tick, Word &present, Word &front) {
    Word slot = w.slots[pos];
    auto a = w.animals[slot];
    const auto &g = w.dna[slot];
    auto home = w.land[pos];
    Word dir = a.heading & 3;
    Word fore = neighbor(pos, dir, c.side), left = neighbor(pos, (dir + 3) & 3, c.side),
         right = neighbor(pos, (dir + 1) & 3, c.side);
    Word reach = segment(a.body.eyes, dir);
    Word seen = reach == 2 ? neighbor(fore, dir, c.side) : reach ? fore : pos;
    Word one = optical(home, a.energy), two = optical(w.land[seen], w.animals[w.slots[seen]].energy);
    front = pigments(g, two);
    present = front | pigments(g, one);
    if (a.body.eyes && (g.growth & (1u << 8)))
        present |= pigments(g, optical(w.land[left], w.animals[w.slots[left]].energy));
    Word projected = 0, recall = 0;
    if (a.heard && (c.wire & 3)) {
        auto &e = w.ears[slot];
        Word call = a.heard - 1, active = grounding::release({e.low[call], e.high[call]}, g.hearing.high & 15);
        projected = nbits(active & front) & 15;
        if (c.wire & 4) {
            // Recalled contact identities can project onto individual neural
            // inputs. Rotation and the projection mask are germline traits.
            recall = rotate(active & present, (g.hearing.high >> 16) & 31) & g.hearing.expression;
        }
    }
    return recall ^
           (one | (two << 8) | (projected << 16) | (Word(a.heard & 15) << 20) | (Word(a.load || a.fibre) << 24) |
            (Word(a.energy >= 64) << 25) | (Word(w.land[left].leaf > w.land[right].leaf) << 26) |
            (Word(w.land[left].water > w.land[right].water) << 27) |
            (Word(bed_edge(home, dir, w.land[fore]) != 0 ||
                  (w.matter.bed && material::hydraulic(w.matter.conditions) &&
                   material::obstruction(w.matter.bed[pos], w.matter.bed[fore], dir) > 0))
             << 28) |
            (((tick / c.season) & 1) << 29) | (dir << 30));
}
} // namespace axion::life::flood
