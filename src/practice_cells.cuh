#pragma once
#include "axion/practice.hpp"
#include "linkage_design.cuh"
namespace axion::life::practice {
using linkage::absent;
using linkage::bits;
using linkage::joint;
using linkage::Map;
using linkage::Pattern;
using linkage::replace;
using linkage::Rig;
AX_FN Word cycle(Word i, Word force) {
    constexpr Word f[]{0x39, 0x8d, 0x1e, 0x72, 0x4b, 0x93}, r[]{0x93, 0x72, 0x4b, 0x8d, 0x1e, 0x39};
    return force ? r[i] : f[i];
}
AX_FN Word support(Word from, Word to, Word force) {
    Word votes = 0;
    for (Word i = 0; i < 6; ++i)
        votes |= Word(bits(cycle(i, force), from) == to) << i;
    return votes;
}
AX_FN Word rings(Context c, Word g) {
    Word r = (c.rings >> (6 * g)) & 63;
    return r ? r : 63;
}
AX_FN Word possibilities(Word states, Word from, Word action) {
    Word colors = 0;
    for (; states; states &= states - 1)
        colors |= 1u << bits(cycle(__builtin_ctz(states), action), from);
    return colors;
}
AX_FN bool narrow(Context &c, Word g, Word seen, Genes genes) {
    Word r = rings(c, g), table = (genes.learning >> 16) & 15;
    Word next = ((table & 1) ? ~r & ~seen : 0) | ((table & 2) ? r & ~seen : 0) | ((table & 4) ? ~r & seen : 0) |
                ((table & 8) ? r & seen : 0);
    next &= 63;
    if (next == 63)
        next = 0;
    Word out = (c.rings & ~(63u << (6 * g))) | (next << (6 * g));
    bool changed = out != c.rings;
    c.rings = out;
    return changed;
}
AX_FN Word one(Word mask) { return mask && !(mask & (mask - 1)) ? Word(__builtin_ctz(mask)) : 4u; }
AX_FN Word gate(const Context &c, Word glyph, Word type, Word face) {
    return bits(c.gates[type - 1], 4 * glyph + face);
}
AX_FN Word sign(const Context &c, Word glyph, Word type) { return bits(c.signs, 4 * (type - 1) + glyph); }
AX_FN bool agrees(const Context &c, Rig r, Word old, Word move, Word now, Word ignore = 0) {
    Word i = move >> 1, a = move & 1, g = bits(r.glyphs, i), face = bits(old, i);
    if (!(rings(c, g) & support(face, bits(now, i), a)))
        return false;
    for (Word j = 0; j < 4; ++j)
        if (Word type = joint(r.joints, i, j)) {
            Word previous = bits(old, j), seen = bits(now, j), engaged = 1 + Word(previous != seen),
                 polarity = sign(c, g, type);
            if (gate(c, g, type, face) && gate(c, g, type, face) != engaged && !ignore)
                return false;
            if (engaged == 2 && polarity &&
                !(rings(c, bits(r.glyphs, j)) & support(previous, seen, a ^ (polarity - 1))))
                return false;
        }
    return true;
}
AX_FN Word consistency(const Mind &m, Slots s, Rig r, Word old, Word move, Word now) {
    Word lit = 0;
    for (Word b = 0; b < size(m); ++b)
        lit |= Word(agrees(s.at(b), r, old, move, now)) << b;
    return lit;
}
AX_FN Word select(Mind &m, Slots s, Word votes, Word mode, bool writing) {
    Word n = size(m), present = active(m),
         cap = (mode & contextual) && (m.genes.inference & 1) ? capacity(m.genes, s.cap) : 1;
    if (cap == 1 && n) {
        if (votes & (1u << present))
            return present;
        if (!writing)
            return 4;
        if (m.genes.learning & (1u << 21))
            s.at(present) = {};
        return present;
    }
    if (present < n && votes & (1u << present))
        return present;
    Word target = 0;
    if (votes) {
        target = __builtin_ctz(votes);
    } else if (!writing)
        return 4;
    else if (n < cap) {
        target = n++;
        s.at(target) = {};
    } else {
        target = (present + 1) % cap;
        if (m.genes.learning & (1u << 21))
            s.at(target) = {};
    }
    m.banks = n | (target << 3);
    return target;
}
struct Evidence {
    Word edits = 0, novel = 0, switched = 0, consistent = 0;
};
AX_FN Evidence observe(Mind &m, Slots slots, Rig r, Word old, Word move, Word now, bool witness, Word mode) {
    Evidence e{};
    if (witness && !(mode & social))
        return e;
    Word learn = (m.genes.learning >> (witness ? 4 : 0)) & 15;
    if (!learn)
        return e;
    if (mode & wrong_effect)
        now ^= 3u << (2 * (move >> 1));
    Word before = size(m), prior = active(m);
    Word votes = consistency(m, slots, r, old, move, now);
    Word b = select(m, slots, votes, mode, mode & studying);
    e.consistent = Word(votes != 0);
    if (b == 4)
        return e;
    e.novel = before < size(m);
    e.switched = before && prior != b;
    if (!(mode & studying))
        return e;
    if (m.samples != ~0u)
        ++m.samples;
    auto &c = slots.at(b);
    Word i = move >> 1, a = move & 1, g = bits(r.glyphs, i), f = bits(old, i);
    if (learn & 1)
        e.edits += narrow(c, g, support(f, bits(now, i), a), m.genes);
    if (learn & 2)
        for (Word j = 0; j < 4; ++j)
            if (Word type = joint(r.joints, i, j)) {
                Word from = bits(old, j), to = bits(now, j), engaged = 1 + Word(from != to),
                     other = bits(r.glyphs, j);
                Word ix = 4 * g + f, updated = replace(c.gates[type - 1], ix, engaged);
                e.edits += updated != c.gates[type - 1];
                c.gates[type - 1] = updated;
                if (from == to)
                    continue;
                Word polarity = sign(c, g, type), rivals = rings(c, other);
                Word same = rivals & support(from, to, a), reverse = rivals & support(from, to, 1 ^ a);
                if (bool(same) != bool(reverse)) {
                    polarity = 1 + Word(!!reverse);
                    Word s = replace(c.signs, 4 * (type - 1) + g, polarity);
                    e.edits += s != c.signs;
                    c.signs = s;
                }
                if ((learn & 4) && polarity)
                    e.edits += narrow(c, other, support(from, to, a ^ (polarity - 1)), m.genes);
            }
    return e;
}
AX_FN Word fitting(Mind &m, Slots s, Word glyph, Word before, Word now, bool witness, Word mode) {
    if (!(mode & studying) || (witness && !(mode & social)) || !(m.genes.learning & (1u << (witness ? 7 : 3))) ||
        before > 2 || !now || now > 2)
        return 0;
    Word ix = glyph * 3 + before, votes = 0;
    for (Word b = 0; b < size(m); ++b) {
        Word after = bits(s.at(b).joins, ix);
        if (!after || after == now)
            votes |= 1u << b;
    }
    Word b = select(m, s, votes, mode, true);
    auto &c = s.at(b);
    Word result = replace(c.joins, ix, now);
    bool changed = result != c.joins;
    c.joins = result;
    return changed;
}
AX_FN void insert_map(Word &registers, Word row, Word value) {
    if (value < 4)
        registers |= (value << (2 * row)) | (1u << (16 + row));
}
// A shared, expressed clutch can generalise after two concordant contacts.
AX_FN Word response(const Context &c, const Genes &g, Word type, Word t, Word f, Word a) {
    Word grip = gate(c, type, t, f), polarity = sign(c, type, t);
    if (!grip && (g.inference & (1u << (7 + t)))) {
        Word field = (c.gates[t - 1] >> (8 * type)) & 255, still = field & 0x55, moving = (field >> 1) & 0x55;
        if (!still && __builtin_popcount(moving) >= 2)
            grip = 2;
        else if (!moving && __builtin_popcount(still) >= 2)
            grip = 1;
    }
    if (grip == 1)
        return 0;
    return grip == 2 && polarity ? 1 + (a ^ (polarity - 1)) : 4;
}
AX_FN Word forecast(const Context &c, const Genes &g, Rig r, Word before, Word motor) {
    Word p = motor >> 1, a = motor & 1, type = bits(r.glyphs, p), from = bits(before, p),
         to = one(possibilities(rings(c, type), from, a));
    if (to == 4)
        return absent;
    Word result = replace(before, p, to);
    for (Word j = 0; j < 4; ++j)
        if (Word seam = joint(r.joints, p, j)) {
            Word power = response(c, g, type, seam, from, a);
            if (power == 4)
                return absent;
            if (power) {
                Word next = one(possibilities(rings(c, bits(r.glyphs, j)), bits(before, j), power - 1));
                if (next == 4)
                    return absent;
                result = replace(result, j, next);
            }
        }
    return result;
}
AX_FN Map read(const Context &c, const Genes &g) {
    Map map{};
    for (Word type = 0; type < 4; ++type) {
        Word candidates = rings(c, type);
        for (Word f = 0; f < 4; ++f)
            for (Word a = 0; a < 2; ++a) {
                Word row = f + 4 * a;
                insert_map(map.next[type], row, one(possibilities(candidates, f, a)));
                for (Word t = 1; t <= 2; ++t) {
                    insert_map(map.link[t - 1][type], row, response(c, g, type, t, f, a));
                }
            }
        for (Word prior = 0; prior < 3; ++prior)
            if (Word after = bits(c.joins, type * 3 + prior))
                insert_map(map.splice[type], 4 + prior, after);
    }
    return map;
}
AX_FN linkage::Genes motor_genes(const Genes &g) {
    linkage::Genes k{};
    k.impulse = g.muscles;
    k.arbor = g.inference;
    return k;
}
AX_FN Word inquiry(const Mind &m, Slots s, Rig r, Word color, Word key, Word mode) {
    Word mask = r.handles;
    if (!mask)
        return linkage::no_path;
    if (mode & random_probes)
        return linkage::bump(mask, key);
    Word best = 0, elected = linkage::no_path;
    for (; mask; mask &= mask - 1) {
        Word a = __builtin_ctz(mask), p = a >> 1, f = bits(color, p), g = bits(r.glyphs, p);
        Word alone = 0, union_faces = 0, distinct = 0, unknown = 0;
        for (Word ix = 0; ix < size(m); ++ix) {
            if (!(mode & contextual) && ix != active(m))
                continue;
            auto &c = s.at(ix);
            Word outcomes = possibilities(rings(c, g), f, a & 1);
            union_faces |= outcomes;
            if ((outcomes & (outcomes - 1)) == 0)
                alone |= outcomes;
            if (ix == active(m))
                unknown += __builtin_popcount(outcomes) - 1;
            for (Word j = 0; j < 4; ++j)
                if (Word t = joint(r.joints, p, j)) {
                    unknown += gate(c, g, t, f) == 0;
                    unknown += sign(c, g, t) == 0;
                }
        }
        distinct = __builtin_popcount(alone);
        Word v = (distinct > 1 ? (distinct - 1) * (1 + ((m.genes.inference >> 17) & 7)) : 0) +
                 (unknown * ((m.genes.attention >> 21) & 3)) +
                 (__builtin_popcount(union_faces) * ((m.genes.inference >> 13) & 3));
        Word rank = (v << 6) | ((mix(key + a * 2749) >> 17) & 63);
        if (elected == linkage::no_path || rank > best) {
            best = rank;
            elected = a;
        }
    }
    return elected;
}
} // namespace axion::life::practice
