#pragma once
#include "braid.hpp"
namespace axion::life::braid {
constexpr Word order = 0xe4;
struct Plan {
    Word stream = 0, size = 0, found = 0;
};
struct Motors {
    unsigned char table[voices]{};
    Word present = 0;
};
AX_FN Word transfer(Word picture, Word layout) {
    return pigment(picture, layout & 3) | (pigment(picture, (layout >> 2) & 3) << 2) |
           (pigment(picture, (layout >> 4) & 3) << 4) | (pigment(picture, layout >> 6) << 6);
}
AX_FN bool bijective(Word layout) {
    Word shown = 0;
    for (Word i = 0; i < 4; ++i)
        shown |= 1u << pigment(layout, i);
    return shown == 15;
}
AX_FN Motors release(const Mind &m, bool speech) {
    Motors out{};
    for (Word j = 0; j < (speech ? voices : motors); ++j) {
        Word cell = (speech ? motors : 0) + j;
        if (!(m.lived & (1u << cell)))
            continue;
        auto q = predict(matrix(m, cell), order);
        if (q.known != 15 || !bijective(q.colors))
            continue;
        out.table[j] = q.colors;
        out.present |= 1u << j;
    }
    return out;
}
AX_FN Plan route(const Motors &moves, Word start, Word target, Word receptive, Word limit) {
    if (!receptive)
        return {};
    if (matches(start, target, receptive))
        return {0, 0, 1};
    if (!moves.present || !limit)
        return {};
    // A conserved four-float picture has at most 24 distinct arrangements.
    // Each queued address packs its sensory spikes, predecessor and motor.
    Half reached[24];
    Word visited[8]{};
    reached[0] = Half(start);
    visited[start >> 5] |= 1u << (start & 31);
    Word tail = 1, head = 0;
    for (Word n = 1; n <= limit && head < tail; ++n) {
        Word end = tail;
        for (; head < end; ++head) {
            Word here = reached[head] & 255, ports = moves.present;
            while (ports) {
                Word j = __builtin_ctz(ports);
                ports &= ports - 1;
                Word x = transfer(here, moves.table[j]), bit = 1u << (x & 31);
                if (visited[x >> 5] & bit)
                    continue;
                visited[x >> 5] |= bit;
                Word note = x | (head << 8) | (j << 13);
                reached[tail++] = Half(note);
                if (matches(x, target, receptive)) {
                    Plan p{0, n, 1};
                    for (Word i = 0; i < n; ++i) {
                        p.stream = (p.stream << 3) | (note >> 13);
                        note = reached[(note >> 8) & 31];
                    }
                    return p;
                }
            }
        }
    }
    return {};
}
AX_FN Word pouch(const Genes &g, Word motor, Word rig) { return action((g.muscle ^ rig) & 7, motor); }
AX_FN Word valve(Word colors, Word motor, const Genes &g, Word rig, Word strands = 15) {
    Word e = pouch(g, motor, rig);
    if (!(strands & (1u << e)))
        return colors;
    Word turned = exchanged(colors, e, (e + 1) & 3);
    // Joined eyes relay tension to the following pair of cups.
    if (((rig >> (3 + e)) & 1) && (strands & (1u << ((e + 1) & 3))))
        turned = exchanged(turned, (e + 1) & 3, (e + 2) & 3);
    return turned;
}
AX_FN Word assembly(Word idx) {
    Word seen = 0, out = 0;
    for (Word j = 0; j < 4; ++j) {
        Word a = idx % (4 - j);
        idx /= 4 - j;
        Word v = (~seen) & 15;
        while (a--) {
            v &= v - 1;
        }
        Word color = __builtin_ctz(v);
        seen |= 1u << color;
        out |= color << (2 * j);
    }
    return out;
}
AX_FN Word address_of(Word layout) {
    Word seen = 0, out = 0, scale = 1;
    for (Word i = 0; i < 4; ++i) {
        Word c = pigment(layout, i);
        out += scale * __builtin_popcount((~seen) & ((1u << c) - 1));
        seen |= 1u << c;
        scale *= 4 - i;
    }
    return out;
}
AX_FN Word phrase_code(const Phrase &p) {
    Word s = 0;
    for (Word k = 0; k < p.n; ++k)
        s |= Word(p.cell[k]) << (3 * k);
    return s;
}
AX_FN Phrase phrase(Word code, Word n) {
    Phrase p{};
    p.n = n;
    for (Word k = 0; k < n; ++k)
        p.cell[k] = (code >> (3 * k)) & 7;
    return p;
}
AX_FN Word named(const Mind &m, Word old, Word current, Word tie) {
    Word options = 0;
    for (Word i = 0; i < capacity(m.genes); ++i)
        if (m.lived & (1u << (motors + i))) {
            auto p = predict(matrix(m, motors + i), old);
            if (p.known == 15 && p.colors == current)
                options |= 1u << i;
        }
    if (!options || old == current)
        return none;
    Word shift = tie & 7;
    return (__builtin_ctz(((options | (options << 8)) >> shift) & 255) + shift) & 7;
}
AX_FN Word term(const Mind &m, Word before, Word after, Word motor, Word key, bool create, Word &fresh) {
    if (!(m.genes.throat & (1u << (16 + motor))) || before == after)
        return none;
    auto old = named(m, before, after, key >> 7);
    if (old != none && (m.genes.throat & (1u << 20)))
        return old;
    if (!create)
        return none;
    Word n = capacity(m.genes), available = ((1u << n) - 1) & ~(m.lived >> motors);
    if (available && (m.genes.throat & (1u << 21))) {
        Word shift = (key >> 11) & 7;
        fresh = 1;
        return (__builtin_ctz(((available | (available << 8)) >> shift) & 255) + shift) & 7;
    }
    return (m.genes.throat >> (3 * motor)) & 7;
}
} // namespace axion::life::braid
