#pragma once
#include "axion/runnel.hpp"
namespace axion::life::runnel {
struct Profile {
    Word touched = 0, forward = 0, backward = 0;
};
AX_FN Profile profile(const Mind &m, Word cue) {
    Word r = row(m.genes, cue);
    return {nibble(m.cells.touched, r), nibble(m.cells.forward, r), nibble(m.cells.backward, r)};
}
// Align native motor spikes. Reflected wrists preserve cyclic order up to a
// reversal. No mineral normal, target observation or physical operator enters this circuit.
AX_FN Word roll4(Word bits, Word shift) { return ((bits | (bits << 4)) >> (shift & 3)) & 15; }
AX_FN Word align(Word bits, Word transform) {
    Word rotated = roll4(bits, transform);
    if (transform & 4)
        rotated = (rotated & 5) | ((rotated & 2) << 2) | ((rotated & 8) >> 2);
    return rotated;
}
AX_FN Profile align(Profile p, Word transform) {
    return {align(p.touched, transform), align(p.forward, transform), align(p.backward, transform)};
}
struct Inference {
    Choice action{};
    Word via = 16, transform = 8, derived = 0;
};
AX_FN Inference infer(const Mind &m, Word cue, Word d, Word drive, bool explore, bool permitted) {
    Inference out{};
    out.action = choose(m, cue, d, drive, explore);
    if (!permitted || !(m.genes.eye & 64) || !(m.genes.hand & 8) || (m.genes.eye & 3) != 3 ||
        (out.action.known && out.action.expected == (1u << d)))
        return out;
    Profile local = profile(m, cue);
    Word possible[4]{}, basis[4]{}, target_left = cue & 3, target_right = cue >> 2;
    Word threshold = 1 + ((m.genes.eye >> 14) & 1), transforms = (m.genes.eye & (1u << 15)) ? 8 : 4;
    for (Word l = 0; l < 4; ++l)
        if (l != target_left) {
            auto last = profile(m, texture(l, target_right));
            if (!last.touched)
                continue;
            for (Word r = 0; r < 4; ++r)
                if (r != target_right) {
                    auto a = profile(m, texture(target_left, r)), b = profile(m, texture(l, r));
                    if (!a.touched || !b.touched)
                        continue;
                    Word phases = 0, winner = 0;
                    for (Word t = 0; t < transforms; ++t) {
                        auto mapped = align(b, t);
                        Word common = a.touched & mapped.touched;
                        if (common & ((a.forward ^ mapped.forward) | (a.backward ^ mapped.backward)))
                            continue;
                        ++phases;
                        winner = __builtin_popcount(common) >= int(threshold) ? t : 8;
                        if (phases == 2)
                            break;
                    }
                    if (phases != 1 || winner == 8)
                        continue;
                    auto next = align(last, winner);
                    if (local.touched & next.touched &
                        ((local.forward ^ next.forward) | (local.backward ^ next.backward)))
                        continue;
                    Word unknown = next.touched & ~local.touched;
                    for (Word code = 0; code < 4; ++code) {
                        Word spikes = unknown & (code & 1 ? next.forward : ~next.forward) &
                                      (code & 2 ? next.backward : ~next.backward);
                        possible[code] |= spikes;
                        if (code == (1u << d))
                            for (Word b = spikes; b; b &= b - 1)
                                basis[__builtin_ctz(b)] = texture(l, r) | (winner << 4);
                    }
                }
        }
    Word wanted = 1u << d, options = possible[wanted];
    for (Word f = 0; f < 4; ++f)
        if (f != wanted)
            options &= ~possible[f];
    if (!options)
        return out;
    Word a = choose_bit(options, drive);
    out.action = {a, 1, 0, wanted};
    out.via = basis[a] & 15;
    out.transform = basis[a] >> 4;
    out.derived = 1;
    return out;
}
AX_FN Image synthesize(const Mind &m, Word phase, bool permitted) {
    Image im{};
    for (Word d = 0; d < 2; ++d)
        for (Word c = 0; c < 16; ++c) {
            auto a = infer(m, c, d, phase, false, permitted).action;
            im.admitted |= Word(a.known && a.expected == (1u << d)) << (c + 16 * d);
            im.motors[d] |= a.motor << (2 * c);
        }
    return im;
}
} // namespace axion::life::runnel
