#pragma once
#include "life.hpp"

namespace axion::life::braid {
using Half = unsigned short;
constexpr Word motors = 4, voices = 8, cells = motors + voices, none = 255, all = 65535, identity = 0x8421,
               max_phrase = 8;
enum Mode : Word {
    turning = 1,
    watching = 2,
    hearing = 4,
    changing = 8,
    projecting = 16,
    coupling = 32,
    feeding = 64,
    inventing = 128,
    live = 255,
    open_mesh = 256,
    slipping = 512
};
struct Genes {
    Word plastic = 0, arbor = 0, muscle = 0, throat = 0;
};
constexpr Word lineage_bits=0xc000u;
AX_FN Word drift_tag(Word lived){return lived>>14;}
struct Mind {
    Genes genes{};
    Half pruned[cells]{};
    Half lived = 0;
    unsigned char taste = 0, last = 0;
    Word volley = 0;
};
struct Words {
    Count wave[voices]{};
};
struct Phrase {
    unsigned char cell[max_phrase]{}, n = 0;
};
struct Saved {
    Mind mind{};
    Words words{};
    Word tag = 0x44494142u, extent = sizeof(Saved);
};
struct Totals {
    Count trapped = 0, spent = 0, strokes = 0, engaged = 0, transferred = 0, carried = 0, across = 0, heard = 0,
          spoken = 0, changes = 0, babbled = 0, named = 0, known_pulls = 0, requested = 0, earned = 0, meals = 0,
          others = 0, soma_events = 0, soma_changed = 0, soma_moves = 0, goal_gain = 0, goal_loss = 0, rerouted = 0,
          traced_pulls = 0, traced_food = 0, ear_events = 0, ear_changed = 0, ear_moves = 0;
};
struct State {
    Word version = 1, mode = live, seed = 36479, reserved = 0;
    Totals book{};
};
struct GateMemory;
struct GateBook;
struct SedimentBook;
struct Episode;
struct Field {
    Mind *minds{};
    Half *floats{};
    Totals *book{};
    Word mode = 0;
    GateMemory *contacts{};
    GateBook *gates{};
    Word phase = 0;
    Episode *episodes{};
    SedimentBook *sediment{};
    Count *trails{};
};
static_assert(sizeof(Genes) == 16 && sizeof(Mind) == 48 && sizeof(Words) == 64 && sizeof(Saved) == 120);
AX_FN Genes founder(Word seed) { return {mix(seed + 1901), mix(seed + 2297), mix(seed + 3881), mix(seed + 4507)}; }
AX_FN Genes inherit(Genes g, Word k, Word rate) {
    auto flip = [&](Word a, Word salt) {
        Word v = mix(k + salt);
        return (v & 255) < rate ? a ^ (1u << ((v >> 8) & 31)) : a;
    };
    return {flip(g.plastic, 541), flip(g.arbor, 2029), flip(g.muscle, 5521), flip(g.throat, 9187)};
}
AX_FN Word pigment(Word colors, Word hole) { return (colors >> (2 * hole)) & 3; }
AX_FN Word port(Word packed, Word position) { return pigment(packed, position); }
AX_FN Word lamps(Word colors, Word value, Word mask) {
    Word diff = (colors ^ (value * 85)) & (mask * 85);
    Word hits = ~(diff | (diff >> 1)) & 85;
    return (hits & 1) | ((hits >> 1) & 2) | ((hits >> 2) & 4) | ((hits >> 3) & 8);
}
AX_FN Word compatible(Word x, Word y, Word mask) {
    Word result = 0;
    for (Word j = 0; j < 4; ++j)
        result |= lamps(x, pigment(y, j), mask) << (4 * j);
    return result;
}
AX_FN Word back(Word a) {
    Word d = (a ^ (a >> 3)) & 0x0a0au;
    a ^= d | (d << 3);
    d = (a ^ (a >> 6)) & 0x00ccu;
    return a ^ d ^ (d << 6);
}
AX_FN Word forward(Word paths, Word lit) {
    Word q = paths & (lit * 0x1111u);
    q = (q | (q >> 1) | (q >> 2) | (q >> 3)) & 0x1111;
    return (q | (q >> 3) | (q >> 6) | (q >> 9)) & 15;
}
AX_FN Word compose(Word after, Word before) {
    Word t = back(before), r = 0;
    for (Word i = 0; i < 4; ++i)
        r |= forward(t, (after >> (4 * i)) & 15) << (4 * i);
    return r;
}
AX_FN Word enabled(Genes g, Word cell) { return cell < motors || cell - motors <= ((g.arbor >> 8) & 7); }
AX_FN Word raw(const Mind &m, Word cell) { return all ^ m.pruned[cell]; }
AX_FN Word rule_bits(Word law, Word old, Word conflict) {
    Word a = 0u - (law & 1), b = 0u - ((law >> 1) & 1), c = 0u - ((law >> 2) & 1), d = 0u - ((law >> 3) & 1);
    return ((~old & ~conflict & a) | (~old & conflict & b) | (old & ~conflict & c) | (old & conflict & d));
}
AX_FN Word rule(Word law,Word old,Word conflict){return rule_bits(law,old,conflict)&all;}
// A receptor's four afferents compete for the same float. A resolved incoming
// contact can release lateral inhibition onto other receptors sharing that source.
AX_FN Word compete(Word a) {
    for (Word k = 0; k < 3; ++k) {
        Word singles = 0, keep = 0;
        for (Word j = 0; j < 4; ++j) {
            Word r = (a >> (4 * j)) & 15;
            if (r && !(r & (r - 1))) {
                singles |= r;
                keep |= 15u << (4 * j);
            }
        }
        Word next = a & (~(singles * 0x1111u) | keep);
        if (next == a)
            break;
        a = next;
    }
    return a;
}
AX_FN Word matrix(const Mind &m, Word cell) {
    if (cell >= cells || !(m.lived & (1u << cell)))
        return all;
    auto r = raw(m, cell);
    return (m.genes.arbor & 4) ? compete(r) : r;
}
AX_FN bool viable(Word x) {
    auto cover = x | (x >> 1) | (x >> 2) | (x >> 3);
    return (cover & 0x1111) == 0x1111 && ((x | (x >> 4) | (x >> 8) | (x >> 12)) & 15) == 15;
}
template <class Gate> AX_FN Word receive_contact(Mind &m, Word cell, Word support, Word channel, Gate gate) {
    if (!enabled(m.genes, cell) || !(m.genes.arbor & (1u << (12 + channel))))
        return 0;
    Word old = m.pruned[cell], law = m.genes.plastic >> (4 * channel);
    Word next = rule(law, old, all ^ support);
    if ((m.genes.arbor & 8) && !viable(all ^ next))
        next = rule(law, 0, all ^ support);
    next = gate(old, all ^ support, next);
    m.pruned[cell] = Half(next);
    m.lived |= 1u << cell;
    return __builtin_popcount(old ^ next);
}
AX_FN Word strengthen(Mind &m, Word cell, Word support, Word channel) {
    return receive_contact(m, cell, support, channel, [](Word, Word, Word proposed) { return proposed; });
}
AX_FN Word motor_touch(Mind &m, Word motor, Word before, Word after, bool observed) {
    return strengthen(m, motor, compatible(before, after, m.genes.arbor & 3), observed ? 1 : 0);
}
AX_FN bool reversed(const Mind &m) { return (m.lived >> 13) & 1; }
AX_FN Phrase sequence(Phrase p, bool mirror) {
    if (mirror)
        for (Word i = 0; i < p.n / 2; ++i) {
            auto x = p.cell[i];
            p.cell[i] = p.cell[p.n - 1 - i];
            p.cell[p.n - 1 - i] = x;
        }
    return p;
}
AX_FN Word listen(Mind &m, Phrase p, Word before, Word after) {
    p = sequence(p, reversed(m));
    if (!p.n || p.n > max_phrase || (p.n > 1 && !(m.genes.arbor & 16)))
        return 0;
    Word scene = compatible(before, after, m.genes.arbor & 3), edits = 0, repeats = 1 + ((m.genes.arbor >> 5) & 3);
    // Excitation arrives from both ends of the episode. Neither a hand trace
    // nor an intermediate demonstration is required inside a spoken sequence.
    for (Word sweep = 0; sweep < repeats; ++sweep) {
        Half prefix[max_phrase];
        Word acc = identity;
        for (Word j = 0; j < p.n; ++j) {
            prefix[j] = Half(acc);
            acc = compose(matrix(m, motors + p.cell[j]), acc);
        }
        Word tail = identity, changed = 0;
        for (Word j = p.n; j-- > 0;) {
            Word cell = motors + p.cell[j];
            auto allowed = compose(back(tail), compose(scene, back(prefix[j])));
            changed += strengthen(m, cell, allowed, p.n == 1 ? 2 : 3);
            tail = compose(tail, matrix(m, cell));
        }
        edits += changed;
        if (!changed)
            break;
    }
    return edits;
}
struct Prediction {
    Word colors = 0, known = 0;
};
AX_FN Prediction predict(Word circuit, Word start) {
    Word out = 0, known = 0;
    for (Word i = 0; i < 4; ++i) {
        Word mask = (circuit >> (4 * i)) & 15;
        if (!mask)
            continue;
        Word hue = pigment(start, __builtin_ctz(mask));
        if (!(mask & ~lamps(start, hue, 3))) {
            known |= 1u << i;
            out |= hue << (2 * i);
        }
    }
    return {out, known};
}
AX_FN Word chain(const Mind &m, const Phrase &p, bool mirror) {
    auto order = sequence(p, mirror);
    Word path = identity;
    for (Word j = 0; j < order.n; ++j)
        path = compose(matrix(m, motors + order.cell[j]), path);
    return path;
}
AX_FN Word pathway(const Mind &m, const Phrase &p) { return chain(m, p, reversed(m)); }
AX_FN bool order_contact(Mind &m, const Phrase &p, Word start, Word end) {
    if (p.n < 2 || !(m.genes.arbor & (1u << 17)))
        return false;
    auto a = predict(chain(m, p, false), start), b = predict(chain(m, p, true), start);
    if (a.known != 15 || b.known != 15 || a.colors == b.colors || (a.colors != end && b.colors != end))
        return false;
    Word old = (m.lived >> 12) & 3, branch = (old >> 1) + 2 * Word(b.colors == end);
    Word value = (m.genes.throat >> (24 + 2 * branch)) & 3;
    m.lived = Half((m.lived & (lineage_bits | 0x0fffu)) | (value << 12));
    return old != value;
}
AX_FN bool matches(Word a, Word b, Word view) {
    for (Word j = 0; j < 4; ++j)
        if ((view & (1u << j)) && pigment(a, j) != pigment(b, j))
            return false;
    return true;
}
AX_FN Word exchanged(Word state, Word a, Word b) {
    Word d = pigment(state, a) ^ pigment(state, b);
    return state ^ (d << (2 * a)) ^ (d << (2 * b));
}
AX_FN Word face(Word frame, Word i) { return ((frame & 3) + ((frame & 4) ? 0u - i : i)) & 3; }
AX_FN Word action(Word frame, Word private_motor) {
    return ((frame & 3) + ((frame & 4) ? 0u - private_motor : private_motor)) & 3;
}
AX_FN Word capacity(const Genes &g) { return 1 + ((g.arbor >> 8) & 7); }
AX_FN Word depth(const Genes &g) { return 1 + ((g.arbor >> 20) & 7); }
AX_FN Word mass(Half f) { return (f >> 2) & 31; }
AX_FN Word birthmark(Word x, Word y) { return (x & 15) | ((y & 15) << 4); }
AX_FN Half floater(Word hue, Word x, Word y) { return Half((birthmark(x, y) << 8) | (hue & 3)); }
AX_FN Word distance(Half f, Word x, Word y) {
    auto delta = [](Word a, Word b) {
        Word z = (a - b) & 15;
        return z > 8 ? 16 - z : z;
    };
    return delta(x, f >> 8) + delta(y, f >> 12);
}
AX_FN Word afferent(const Mind &m) { return rotate(m.volley, (m.genes.arbor >> 24) & 31) & m.genes.throat; }
AX_FN bool valid(const Mind &m) {
    if (drift_tag(m.lived)==3 || ((m.lived & 0x3000) && !(m.genes.arbor & (1u << 17))))
        return false;
    for (Word k = 0; k < cells; ++k)
        if (!(m.lived & (1u << k)) && m.pruned[k])
            return false;
    return m.last <= voices;
}
AX_FN bool valid_words(const Words &w) {
    for (Word i = 0; i < voices; ++i) {
        for (Word j = 0; j < i; ++j)
            if (w.wave[i] && w.wave[i] == w.wave[j])
                return false;
    }
    return true;
}
} // namespace axion::life::braid
