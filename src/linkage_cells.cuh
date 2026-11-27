#pragma once
#include "axion/linkage.hpp"
#include "grounding_eye.cuh"
namespace axion::life::linkage {
AX_FN Word mapping(grounding::Synapse source, Word law) {
    Word out = 0, active = grounding::release(source, (law >> 16) & 15);
    for (Word j = 0; j < 8; ++j) {
        Word row = (active >> (4 * j)) & 15;
        if (row && !(row & (row - 1)))
            out |= (Word(__builtin_ctz(row)) << (2 * j)) | (1u << (16 + j));
    }
    return out;
}
AX_FN Word apply(Word mapping, Word face, Word impulse) {
    Word row = face + 4 * impulse;
    return mapping & (1u << (row + 16)) ? bits(mapping, row) : 4u;
}
AX_FN bool write(grounding::Synapse &source, Word law, Word row, Word effect) {
    Word shift = 4 * row, mask = 15u << shift;
    auto update = grounding::plastic(source, 1u << (shift + effect), law & 65535);
    Word low = (source.low & ~mask) | (update.low & mask), high = (source.high & ~mask) | (update.high & mask);
    bool changed = source.low != low || source.high != high;
    source = {low, high};
    return changed;
}
AX_FN Tissue &recruit(Mind &m, Slots s, Word glyph) {
    Word i = address(m, glyph, s.capacity);
    if (key_at(m, i) != 1 + glyph) {
        m.tags = (m.tags & ~(15u << (4 * i))) | ((1 + glyph) << (4 * i));
        s.at(i) = {};
    }
    return s.at(i);
}
AX_FN Word direct(const Mind &m, Slots s, Word glyph, Word old, Word action) {
    Word i = address(m, glyph, s.capacity);
    if (key_at(m, i) != glyph + 1)
        return 4;
    auto v = s.at(i).local;
    Word row = (grounding::release(v, (m.genes.touch >> 16) & 15) >> (4 * (old + 4 * action))) & 15;
    return row && !(row & (row - 1)) ? Word(__builtin_ctz(row)) : 4u;
}
AX_FN Word learn(Mind &m, Slots s, Obs before, Word motor, Word after, bool witnessed, Word mode) {
    if (!(mode & writable) || (witnessed && !(mode & witnesses)))
        return 0;
    Word i = motor >> 1, act = motor & 1, glyph = bits(before.rig.glyphs, i), f = bits(before.faces, i);
    Word sensory = (m.genes.arbor >> (8 + 2 * Word(witnessed))) & 3, changes = 0;
    if (sensory & 1)
        changes += write(recruit(m, s, glyph).local, m.genes.touch, f + 4 * act, bits(after, i));
    if (sensory & 2)
        for (Word j = 0; j < 4; ++j) {
            Word contact = joint(before.rig.joints, i, j);
            if (!contact)
                continue;
            Word old = bits(before.faces, j), seen = bits(after, j), signal = 0;
            if (old != seen) {
                Word other = bits(before.rig.glyphs, j);
                bool a = direct(m, s, other, old, 0) == seen, b = direct(m, s, other, old, 1) == seen;
                if (a == b)
                    continue;
                signal = 1 + Word(b);
            }
            if (mode & other_eye)
                signal = (signal + 1) % 3;
            Word law = (m.genes.binding & (1u << 30)) ? m.genes.touch : m.genes.binding;
            changes += write(recruit(m, s, glyph).seam[contact - 1], law, f + 4 * act, signal);
        }
    if (m.contacts != ~0u)
        ++m.contacts;
    return changes;
}
AX_FN Word learn_join(Mind &m, Slots s, Word glyph, Word before, bool attaching, Word after, bool witness,
                      Word mode) {
    if (!(mode & writable) || (witness && !(mode & witnesses)) || !((m.genes.arbor >> (8 + 2 * Word(witness))) & 2))
        return 0;
    Word law = m.genes.binding & (1u << 30) ? m.genes.touch : m.genes.binding;
    if (mode & other_eye)
        after = (after + 1) % 3;
    return write(recruit(m, s, glyph).splice, law, before + 4 * Word(attaching), after);
}
AX_FN Map compile(const Mind &m, Slots s) {
    Map result{};
    Word law = m.genes.binding & (1u << 30) ? m.genes.touch : m.genes.binding;
    for (Word j = 0; j < 4; ++j) {
        Word i = address(m, j, s.capacity);
        if (key_at(m, i) != j + 1)
            continue;
        auto t = s.at(i);
        result.next[j] = mapping(t.local, m.genes.touch);
        result.splice[j] = mapping(t.splice, law);
        result.link[0][j] = mapping(t.seam[0], law);
        result.link[1][j] = mapping(t.seam[1], law);
    }
    return result;
}
AX_FN Word predict(const Map &m, Rig r, Word before, Word motor) {
    Word i = motor >> 1, a = motor & 1, g = bits(r.glyphs, i), color = bits(before, i),
         to = apply(m.next[g], color, a);
    if (to == 4)
        return absent;
    Word after = replace(before, i, to);
    for (Word j = 0; j < 4; ++j) {
        Word edge = joint(r.joints, i, j);
        if (!edge)
            continue;
        Word power = apply(m.link[edge - 1][g], color, a);
        if (power > 2)
            return absent;
        if (power) {
            Word next = apply(m.next[bits(r.glyphs, j)], bits(before, j), power - 1);
            if (next == 4)
                return absent;
            after = replace(after, j, next);
        }
    }
    return after;
}
AX_FN Word motor_order(Genes g, Word index) {
    return ((g.impulse & 7) + index * (1 + 2 * ((g.impulse >> 3) & 3))) & 7;
}
AX_FN Word interest(const Mind &m, Slots slots, Rig r, Word colors, Word act) {
    Word i = act >> 1, g = bits(r.glyphs, i), prior = bits(colors, i), a = act & 1;
    Word score = 0, j = address(m, g, slots.capacity);
    if (key_at(m, j) != g + 1)
        return 8;
    auto &t = slots.at(j);
    if (direct(m, slots, g, prior, a) > 3)
        score += 2 + ((m.genes.impulse >> 16) & 7);
    Word law = (m.genes.binding & (1u << 30)) ? m.genes.touch : m.genes.binding;
    for (Word b = 0; b < 4; ++b)
        if (Word form = joint(r.joints, i, b)) {
            Word fiber = (grounding::release(t.seam[form - 1], (law >> 16) & 15) >> (4 * (prior + 4 * a))) & 15;
            if (!fiber || (fiber & (fiber - 1)))
                score += 1 + ((m.genes.impulse >> 19) & 7);
        }
    return score;
}
AX_FN Word probe_motor(const Mind &m, Slots s, Rig r, Word v, Word key) {
    int best = -1;
    Word choice = no_path;
    for (Word mask = r.handles; mask; mask &= mask - 1) {
        Word a = __builtin_ctz(mask);
        Word uncertainty = (m.genes.arbor & (1u << 15)) ? interest(m, s, r, v, a) : 0;
        int score = int(uncertainty * 32 + ((mix(key + 379 * a) >> 9) & 31));
        if (score > best) {
            best = score;
            choice = a;
        }
    }
    return choice;
}

struct Plan {
    Program course{};
    Word found = 0, expanded = 0;
};
AX_FN Plan seek(const Map &m, Rig r, Word start, Pattern goal, Genes genes) {
    Plan answer{};
    if (matches(start, goal)) {
        answer.found = 1;
        return answer;
    }
    if (!r.handles)
        return answer;
    // Each discovered configuration fires once. The preceding configuration
    // and responsible motor are retained only for the explored portion.
    Byte queue[256], parent[256], stroke[256];
    Word seen[8]{};
    queue[0] = start;
    seen[start >> 5] |= 1u << (start & 31);
    Word head = 0, count = 1, end = 1, depth = 1, budget = horizon(genes);
    while (head < end && depth <= budget) {
        Word from = head++;
        Word v = queue[from];
        ++answer.expanded;
        for (Word slot = 0; slot < 8; ++slot) {
            Word motor = motor_order(genes, slot);
            if (!(r.handles & (1u << motor)))
                continue;
            Word to = predict(m, r, v, motor);
            if (to == absent)
                continue;
            Word bit = 1u << (to & 31);
            if (seen[to >> 5] & bit)
                continue;
            seen[to >> 5] |= bit;
            if (matches(to, goal)) {
                Word ordered[12];
                ordered[depth - 1] = motor;
                for (Word j = depth - 1; j > 0; --j) {
                    ordered[j - 1] = stroke[from];
                    from = parent[from];
                }
                for (Word j = 0; j < depth; ++j)
                    append(answer.course, ordered[j]);
                answer.found = 1;
                return answer;
            }
            parent[count] = Byte(from);
            stroke[count] = Byte(motor);
            queue[count++] = Byte(to);
        }
        if (head == end) {
            end = count;
            ++depth;
        }
    }
    return answer;
}
} // namespace axion::life::linkage
namespace axion::life::linkage {
// A passive follower does not drive a second follower in one physical stroke.
// Suppress its handles as well as its afferents before dropping it from a
// counterfactual. Every emitted stroke still acts on the entire real rack.
AX_FN Plan attend(const Map &m, Rig r, Word start, Pattern aim, Genes g, Word enabled) {
    auto best = seek(m, r, start, aim, g);
    if (best.found || !enabled || !(g.arbor & (1u << 27)))
        return best;
    Word masks[2]{aim.care, aim.alternate};
    for (Word care : masks) {
        if (!care)
            continue;
        Word relevant = 0;
        for (Word i = 0; i < 4; ++i)
            if (care & (3u << (2 * i)))
                relevant |= 1u << i;
        Word optional = r.parts & ~relevant;
        for (Word subset = optional; subset;) {
            subset = (subset - 1) & optional;
            Word keep = relevant | subset, registers = mask_parts(keep);
            auto projected = r;
            projected.parts = keep;
            projected.handles &= registers;
            if (!projected.handles)
                continue;
            Word receivers = registers * 0x01010101u;
            projected.joints &= receivers;
            auto q = seek(m, projected, start & registers, {aim.faces & care, care}, g);
            if (q.found && (!best.found || q.course.len < best.course.len))
                best = q;
        }
    }
    return best;
}
} // namespace axion::life::linkage
