#pragma once
#include "effort_core.cuh"
#include "axion/effort_bundle.hpp"
namespace axion::life::effort {
AX_FN void change(Mind &m, Word before, Word action, Word after, Word mode) {
    if (!(mode & learning))
        return;
    Word index = ((before & 3) + 4 * (action & 1)) * 4, mask = 15u << index, spike = 1u << (index + after);
    auto v = grounding::plastic(m.changes, spike, m.genes.muscle & 65535);
    m.changes.low = (m.changes.low & ~mask) | (v.low & mask);
    m.changes.high = (m.changes.high & ~mask) | (v.high & mask);
}
AX_FN Word futures(const Mind &m) { return grounding::release(m.changes, (m.genes.muscle >> 16) & 15); }
AX_FN Word successor(Word map, Word before, Word action) {
    Word fired = (map >> (4 * (before + 4 * action))) & 15;
    return fired && !(fired & (fired - 1)) ? Word(__builtin_ctz(fired)) : 4u;
}
AX_FN Frozen freeze(const Mind &m) {
    Frozen v{};
    for (Word j = 0; j < 4; ++j) {
        v.yields |= intensity(m.food, m.genes.appetite, j) << (8 * j);
        v.costs |= intensity(m.work, m.genes.muscle, j) << (8 * j);
    }
    v.wiring = (m.genes.projection & 0xfffffu) | (((m.genes.episode >> 26) & 15) << 20);
    v.outcomes = history(m);
    v.next = futures(m);
    return v;
}
AX_FN Earthwork prepare(const Frozen &v, Word surface, Word visits, Word length, bool mismatched) {
    int cost = weight(v.wiring, 4);
    Earthwork r{0, 0, surface, cost * int((v.costs >> (8 * surface)) & 255) * int(visits)};
    Word next = surface;
    for (Word j = 1; j <= length; ++j) {
        next = successor(v.next, next, 1);
        if (next >= 4)
            break;
        if (mismatched)
            next = (next + 1) & 3;
        r.known = 1;
        int current = cost * (int((v.costs >> (8 * next)) & 255) * int(visits) + int(j)) + weight(v.wiring, 8) * int(j);
        if (current > r.charge) {
            r.charge = current;
            r.strokes = j;
            r.expected = next;
        }
    }
    return r;
}
AX_FN Word dot_bytes(Word a, Word b) {
    Word score;
    asm("dp4a.u32.u32 %0,%1,%2,0;" : "=r"(score) : "r"(a), "r"(b));
    return score;
}
AX_FN void append_count(Word &hist, Word cue, Word n) {
    Word shift = 8 * cue, prior = (hist >> shift) & 255, nxt = prior + n;
    hist = (hist & ~(255u << shift)) | ((nxt < 255 ? nxt : 255u) << shift);
}
AX_FN void nourishment(Bundle &b, Word hue, Word n = 1) {
    append_count(b.food, hue, n);
    b.terminal = (b.terminal & 12) | hue;
}
AX_FN void exertion(Bundle &b, Word skin, Word n = 1) {
    append_count(b.work, skin, n);
    b.terminal = (b.terminal & 3) | (skin << 2);
}
AX_FN int composite(Word g, Word payoff, Word drag, Word duration, Word trace) {
    return int(payoff) * weight(g, 0) + int(drag) * weight(g, 4) + int(duration) * weight(g, 8) +
           weight(g, 12) * int(trace) + weight(g, 16) * int(duration != 0);
}
AX_FN int potential(const Frozen &v, Bundle p) {
    Word d = dot_bytes(p.work, 0x01010101u),
         addr = context({p.terminal & 3, p.terminal >> 2, d}, ((v.wiring >> 20) & 15) << 26);
    return composite(v.wiring, dot_bytes(v.yields, p.food), dot_bytes(v.costs, p.work), d, (v.outcomes >> addr) & 1);
}
AX_FN int potential(const Mind &m, Bundle p) {
    Word f = 0, w = 0, steps = 0;
    for (Word i = 0; i < 4; ++i) {
        Word fe = (p.food >> (8 * i)) & 255, we = (p.work >> (8 * i)) & 255;
        f += fe * intensity(m.food, m.genes.appetite, i);
        w += we * intensity(m.work, m.genes.muscle, i);
        steps += we;
    }
    Word addr = context({p.terminal & 3, p.terminal >> 2, steps}, m.genes.episode);
    return composite(m.genes.projection, f, w, steps, (history(m) >> addr) & 1);
}
} // namespace axion::life::effort
