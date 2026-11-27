#pragma once
#include "axion/foresight.hpp"
namespace axion::life::material::scree {
AX_FN Word palette(Word k) {
    Word p = 0xe4u;
    for (Word j = 3; j; --j) {
        Word at = draw(k) % (j + 1), swap = ((p >> (2 * j)) ^ (p >> (2 * at))) & 3;
        p ^= (swap << (2 * j)) | (swap << (2 * at));
    }
    return p;
}
AX_FN Word color(Word p, Word x) { return (p >> (2 * x)) & 3; }
AX_FN Word view(Grain g, Word glass) {
    if (!g.faces)
        return 0;
    Word edges = 256;
    for (Word d = 0; d < 4; ++d)
        edges |= color(glass, face(g, d)) << (2 * d);
    return edges;
}
AX_FN Word observe(Word row, Word index, bool pulse, Word law) {
    Word shift = 2 * index, prior = (row >> shift) & 3;
    Word next = (law >> (8 * Word(pulse) + 2 * prior)) & 3;
    return row ^ ((prior ^ next) << shift);
}
AX_FN bool fires(Word row, Word index, Word release) { return (release >> ((row >> (2 * index)) & 3)) & 1; }
AX_FN void plume(Foresight &f, const Genes &g, Word a, Word b, bool arrived) {
    Word at = 4 * a + b;
    f.contacts = observe(f.contacts, at, arrived, g.practice);
    f.appetite |= 1u << (16 + at);
}
AX_FN void motor_event(Foresight &f, const Genes &g, Word motor, Word before, Word after, Grain old, Grain moved) {
    Word spin = 0;
    if (old.faces && moved.faces && old.faces != moved.faces) {
        if (rotate_faces(old.faces, 1) == moved.faces)
            spin |= 4;
        if (rotate_faces(old.faces, 3) == moved.faces)
            spin |= 8;
    }
    Word flash = Word(!before && after) | (Word(before && !after) << 1) | spin;
    Word receptive = (!before && old.faces) | (Word(before && !old.faces) << 1);
    if (old.faces && rotate_faces(old.faces, 1) != old.faces)
        receptive |= 12u;
    if (receptive)
        f.appetite |= 1u << (8 + motor);
    for (Word kind = 0; kind < 4; ++kind) {
        if (!(receptive & (1u << kind)))
            continue;
        Word &row = f.muscles[kind / 2];
        row = observe(row, (kind & 1) * 8 + motor, (flash >> kind) & 1, g.practice);
    }
}
AX_FN Word wild_glass(const Genes &g) { return (g.receptors >> 16) & 3; }
AX_FN Word wild_odor(const Genes &g) { return g.innate >> 30; }
// An inherited color shift is a permutation of the four visible labels.
AX_FN Word shifted_palette(Word k) { return 0xe4u ^ (k * 0x55u); }
} // namespace axion::life::material::scree
