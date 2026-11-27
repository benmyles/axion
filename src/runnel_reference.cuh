#pragma once
#include "runnel_analogy.cuh"
namespace axion::life::runnel {
// Choose an audible question using only established overlaps in the caller's
// own motor receptive fields. A consultant's identity and knowledge are unavailable.
AX_FN Word ask_about(const Mind &m, Word target, Word dir, Word omitted, bool indirect) {
    if (indirect && (m.genes.eye & 64) && (m.genes.eye & 3) == 3) {
        Word x = target & 3, y = target >> 2, best = 0, candidate = 16;
        for (Word u = 0; u < 4; ++u)
            if (u != x) {
                Word cue = texture(u, y);
                if (omitted & (1u << cue))
                    continue;
                auto known = choose(m, cue, dir, 0, false);
                if (known.known && known.expected == (1u << dir))
                    continue;
                for (Word v = 0; v < 4; ++v)
                    if (v != y) {
                        auto a = profile(m, texture(x, v)), b = profile(m, texture(u, v));
                        if (!a.touched || !b.touched)
                            continue;
                        Word phases = 0, support = 0, n = (m.genes.eye & (1u << 15)) ? 8 : 4;
                        for (Word i = 0; i < n; ++i) {
                            auto p = align(b, i);
                            Word shared = p.touched & a.touched;
                            if (((a.forward ^ p.forward) | (a.backward ^ p.backward)) & shared)
                                continue;
                            support = __builtin_popcount(shared);
                            if (++phases > 1)
                                break;
                        }
                        if (phases == 1 && support >= 1 + ((m.genes.eye >> 14) & 1) && support > best) {
                            best = support;
                            candidate = cue;
                        }
                    }
            }
        if (candidate < 16)
            return candidate;
    }
    return omitted & (1u << target) ? 16 : target;
}
} // namespace axion::life::runnel
