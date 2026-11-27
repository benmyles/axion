#pragma once
#include "grounding_eye.cuh"

namespace axion::life::grounding {
struct Lens {
    Shape cell[hypotheses]{};
    Word active[256][bundles]{};
};
constexpr Word edge_mask = (1u << relations) - 1;
AX_FN Word left_slot(Word r) { return !r ? 1 : 0; }
AX_FN Word right_slot(Word r) { return r == 2 ? 1 : 2; }
struct Reading {
    Count possible = 0, certain = 0;
    Word seen = 0;
};
AX_FN Reading read_frame(const Mind &m, const Scene &s, Phrase p, Word r) {
    Reading out;
    if (p.n != 3 || s.truncated)
        return out;
    auto &u = m.word[p.id[left_slot(r)]], &v = m.word[p.id[right_slot(r)]];
    Word syn = m.edge[r][p.id[r]].possible;
    if (!syn || !u.viable || !v.viable)
        return out;
    out.seen = 1;
    for (Word i = 0; i < s.n; ++i) {
        auto a = s.patch[i];
        if (!admits(u.possible, a.light))
            continue;
        for (Word j = 0; j < s.n; ++j)
            if (i != j) {
                auto b = s.patch[j];
                if (!admits(v.possible, b.light))
                    continue;
                auto view = relative(a, b);
                Count pair = 1ull << (8 * i + j);
                if (syn & (view.fired | ~view.known))
                    out.possible |= pair;
                if (admits(u.certain, a.light) && admits(v.certain, b.light) && !(syn & ~view.fired))
                    out.certain |= pair;
            }
    }
    return out;
}
AX_FN Reading read(const Mind &m, const Scene &s, Phrase p) {
    Reading result;
    for (Word r = 0; r < 3; ++r)
        if (m.layout & (1u << r)) {
            auto f = read_frame(m, s, p, r);
            if (!f.seen)
                continue;
            result.possible |= f.possible;
            result.certain = result.seen ? result.certain & f.certain : f.certain;
            result.seen = 1;
        }
    return result;
}
AX_FN Word truth(const Mind &m, const Scene &s, Phrase p) {
    auto r = read(m, s, p);
    return !r.seen ? 0 : r.certain ? 2 : r.possible ? 0 : 1;
}
#if defined(__CUDA__)
AX_FN Word lane() {
    Word t;
    asm("mov.u32 %0, %%tid.x;" : "=r"(t));
    return t;
}
AX_FN Word block() {
    Word t;
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(t));
    return t;
}
AX_FN void sync() { asm volatile("bar.warp.sync 0xffffffff;" ::: "memory"); }
AX_FN Word ballot(Word yes) {
    Word v;
    asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,0xffffffff; }" : "=r"(v) : "r"(yes));
    return v;
}
AX_FN Word through(Word x, Word l = 0) {
    Word v;
    asm volatile("shfl.sync.idx.b32 %0,%1,%2,31,0xffffffff;" : "=r"(v) : "r"(x), "r"(l));
    return v;
}
AX_FN Word all_or(Word x) {
    for (Word k = 16; k; k >>= 1)
        x |= through(x, lane() ^ k);
    return x;
}
AX_FN void expose(Mind &m, const Lens &lens, const Scene &s, Phrase p) {
    Word t = lane();
    if (!s.n || s.truncated || !p.n)
        return;
    if (t < bundles) {
        Word available = 0;
        for (Word j = 0; j < s.n; ++j)
            available |= lens.active[s.patch[j].light][t];
        for (Word j = 0; j < p.n; ++j) {
            auto &w = m.word[p.id[j]].receptive[t];
            w = plastic(w, available, m.genes.low & 65535);
        }
    }
    if (!t) {
        keep(m, s, p);
        for (Word k = 0; k < p.n; ++k)
            ++m.word[p.id[k]].exposures;
    }
    sync();
}
AX_FN void consolidate_word(Mind &m, const Lens &lens, Word id, Word *scratch) {
    auto &w = m.word[id];
    Word t = lane(), active = t < bundles && w.exposures ? release(w.receptive[t], m.genes.high & 15) : 0;
    // The first 16 cells detect one literal, the next 112 two, and
    // the final 448 three. The readout priority for each order is inherited.
    Word ranks[4]{}, orders[4]{};
    for (Word order = 1; order <= 3; ++order) {
        Word mask = order == 1   ? (!t ? 65535u : 0u)
                    : order == 2 ? (t < 4 ? (t ? ~0u : 0xffff0000u) : 0u)
                                 : (t >= 4 ? ~0u : 0u);
        orders[order] = active & mask;
        if (ballot(orders[order]))
            ranks[order] = 1 + ((m.genes.high >> (8 + 2 * order)) & 3);
    }
    Word rank = 0;
    for (Word j = 1; j <= 3; ++j)
        if (ranks[j] > rank)
            rank = ranks[j];
    Word live = 0, depth = 0;
    for (Word j = 1; j <= 3; ++j)
        if (rank && ranks[j] == rank) {
            live |= orders[j];
            depth = j;
        }
    if (t < bundles)
        scratch[t] = live;
    sync();
    Word count = __builtin_popcount(live);
    for (Word j = 16; j; j >>= 1)
        count += through(count, t ^ j);
    if (!t) {
        w.viable = static_cast<Half>(count);
        w.order = static_cast<Byte>(depth);
    }
    for (Word population = 0; population < 8; ++population) {
        Word can = 0, dissent = 0;
        if (count)
            for (Word j = 0; j < bundles; ++j) {
                Word pattern = lens.active[32 * population + t][j];
                can |= pattern & scratch[j];
                dissent |= ~pattern & scratch[j];
            }
        Word some = ballot(can), all = ballot(count && !dissent);
        if (!t) {
            w.possible[population] = some;
            w.certain[population] = all;
        }
    }
    sync();
}
AX_FN Word witnesses(const Mind &m, const Scene &s, Phrase p, Word r) {
    Word t = lane(), events = 0;
    auto &a = m.word[p.id[left_slot(r)]], &b = m.word[p.id[right_slot(r)]];
    if (!a.viable || !b.viable)
        return ~0u;
    Word present = 0;
    for (Word pair = t; pair < 64; pair += 32) {
        Word i = pair >> 3, j = pair & 7;
        if (i >= s.n || j >= s.n || i == j)
            continue;
        auto x = s.patch[i], y = s.patch[j];
        if (!admits(a.possible, x.light) || !admits(b.possible, y.light))
            continue;
        auto hit = relative(x, y);
        events |= (hit.fired | ~hit.known) & edge_mask;
        present = 1;
    }
    return ballot(present) ? all_or(events) : ~0u;
}
AX_FN void bind(Mind &m) {
    Word t = lane();
    for (Word r = 0; r < 3; ++r)
        for (Word j = t; j <= m.count; j += 32)
            m.edge[r][j] = {};
    if (t < 3)
        m.fills[t] = 0;
    sync();
    Word records = 0;
    for (Word k = 0; k < m.kept; ++k) {
        const auto &e = episode(m, k);
        auto p = e.heard;
        if (p.n != 3)
            continue;
        ++records;
        if (t < 3)
            m.fills[t] |= 1ull << p.id[t];
        for (Word r = 0; r < 3; ++r) {
            auto traces = witnesses(m, e.scene, p, r);
            if (!t && traces != ~0u) {
                auto &v = m.edge[r][p.id[r]];
                v.synapse = plastic(v.synapse, traces, m.genes.low >> 16);
                ++v.observed;
            }
        }
    }
    sync();
    Word best = 0, mask = 0;
    for (Word r = 0; r < 3; ++r) {
        Word covered = 0;
        for (Word j = t; j <= m.count; j += 32) {
            auto &c = m.edge[r][j];
            c.possible = c.observed ? release(c.synapse, (m.genes.high >> 4) & 15) & edge_mask : 0;
            if (c.possible)
                covered += c.observed;
        }
        for (Word k = 16; k; k >>= 1)
            covered += through(covered, t ^ k);
        if (covered > best) {
            best = covered;
            mask = 0;
        }
        if (covered == best && covered)
            mask |= 1u << r;
    }
    if (!t) {
        m.layout = best && best * 4 >= records * (1 + ((m.genes.expression >> 8) & 3)) ? mask : 0;
        m.consolidated = m.experienced;
    }
    sync();
}
#endif
} // namespace axion::life::grounding
