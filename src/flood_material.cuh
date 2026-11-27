#pragma once
#include "axion/material.hpp"

namespace axion::life::material {
AX_FN Word draw(Word &key) {
    key = mix(key + 0x9e3779b9u);
    return key;
}
AX_FN Word permutation(Word key) {
    Word order = 0xfac688u;
    for (Word j = 7; j; --j) {
        Word at = draw(key) % (j + 1), d = ((order >> (3 * at)) ^ (order >> (3 * j))) & 7;
        order ^= (d << (3 * at)) | (d << (3 * j));
    }
    return order;
}
AX_FN Genes founder(Word key) {
    return {permutation(key), mix(key + 113), mix(key + 313), mix(key + 1331), mix(key + 577), mix(key + 919)};
}
AX_FN Genes inherit(Genes g, Word key, Word rate) {
    Word *loci[]{&g.muscle, &g.innate, &g.receptors, &g.practice, &g.observation, &g.release};
    for (Word j = 0; j < 6; ++j) {
        Word r = draw(key);
        if ((r & 255) < rate)
            *loci[j] ^= 1u << ((r >> 9) & 31);
    }
    return g;
}
AX_FN Word index(Word raw, Word selectors) {
    Word r = 0;
    for (Word k = 0; k < 3; ++k)
        r |= ((raw >> (7 & (selectors >> (3 * k)))) & 1) << k;
    return r ^ ((selectors >> 9) & 7);
}
AX_FN Word emit(Half contacts, Word release) {
    Word mask = 0;
    for (Word i = 0; i < 8; ++i)
        mask |= ((release >> ((contacts >> (2 * i)) & 3)) & 1) << i;
    return mask;
}
AX_FN Word adapt(Half &contacts, Word sensory, Word rule) {
    Word old = contacts, next = 0;
    for (Word i = 0; i < 8; ++i) {
        Word state = (old >> (2 * i)) & 3, spike = (sensory >> i) & 1;
        next |= ((rule >> (8 * spike + 2 * state)) & 3) << (2 * i);
    }
    contacts = static_cast<Half>(next);
    return __builtin_popcount(old ^ next);
}
AX_FN Word choose(const Worker &w, Word cue, Word basal, Word key) {
    const auto &g = w.genes;
    Word start = mix(key) & 7, kind = index(cue, g.receptors);
    if (((key >> 8) & 255) < ((g.release >> 20) & 31))
        return start;
    Word want = emit(w.seeks[kind], g.release >> 4), inherited = (g.innate >> (3 * kind)) & 7;
    Word pick = start;
    int best = -256;
    for (Word k = 0; k < 8; ++k) {
        Word j = (start + k) & 7, effect = emit(w.predicts[j], g.release);
        int drive = int(__builtin_popcount(effect & want)) * int(1 + ((g.release >> 9) & 7)) -
                    int(__builtin_popcount(effect & ~want)) * int((g.release >> 12) & 3) +
                    int(j == inherited) * int((g.innate >> 24) & 7) +
                    int(j == (basal & 7)) * int((g.release >> 16) & 7);
        if (drive > best) {
            best = drive;
            pick = j;
        }
    }
    return pick;
}
AX_FN Word effector(const Genes &g, Word motor) { return (g.muscle >> (3 * (motor & 7))) & 7; }
AX_FN void practice(Worker &w, Word motor, Temper event) {
    if (event.performed || (w.genes.practice & (1u << 16))) {
        w.edits += adapt(w.predicts[motor & 7], event.change, w.genes.practice);
    }
    w.recent = event.cue | (event.change << 8) | (Word(event.performed != 0) << 16);
}
AX_FN void witness(Worker &w, Word cue, Word flash) {
    if (!flash && !(w.genes.observation & (1u << 16)))
        return;
    w.edits += adapt(w.seeks[index(cue, w.genes.receptors)], flash, w.genes.observation);
    ++w.sightings;
}
AX_FN Grain deposit(Word site, Word seed, Word density) {
    Word k = mix(seed ^ mix(site + 7187));
    if (k % 100 >= density)
        return {};
    Word r = mix(k + 311);
    return {static_cast<Byte>(1 + (r % 255)), static_cast<Byte>(r >> 16), 0, static_cast<Byte>((r >> 28) & 12)};
}
} // namespace axion::life::material
