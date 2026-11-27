#pragma once
#include "workshop.hpp"

namespace axion {

// Each bit is an intermediate sensory variable. Off and on evidence can
// travel in either direction. Their intersection signals an impossible state.
struct Clamps {
    Word off{}, on{};
    AX_FN Word known() const { return off | on; }
    AX_FN void add(Word bits, bool value) {
        if (value) on |= bits;
        else off |= bits;
    }
};

struct Implications {
    Clamps input, output;
    Word choice = ~0u;
};

// Low five address axes of a 32-row table page.
AX_FN Word axis_pattern(int axis) {
    switch (axis) {
        case 0: return 0xaaaaaaaau;
        case 1: return 0xccccccccu;
        case 2: return 0xf0f0f0f0u;
        case 3: return 0xff00ff00u;
        default: return 0xffff0000u;
    }
}

struct LearnedGate {
    Word parents, rows[8];
    int arity;
    bool affine, bias;

    // A gate is admitted only when its response to every possible local
    // assignment is specified. Parent discovery and learning precede planning.
    AX_FN bool read(const DrawingMemory &drawing, const InterventionMemory *experience, int slot, int cell) {
        const auto &span = drawing.spans[slot];
        if (span.confirmed >= 2) {
            parents = 0;
            for (int i = 0; i < 32; ++i) {
                if (span.row[i].input != (1u << i)) return false;
                parents |= ((span.row[i].output >> cell) & 1) << i;
            }
            bias = ((span.response >> cell) ^ Word(population(parents & span.anchor))) & 1;
            affine = true;
            arity = population(parents);
            return true;
        }

        if (experience) {
            const auto &table = experience->circuit[slot];
            parents = table.parents[cell];
            arity = population(parents);
            if (table.trust[cell] >= 8 && arity && arity <= 8) {
                Word valid = arity < 5 ? (1u << (1u << arity)) - 1 : ~0u;
                for (int p = 0; p < ((1 << arity) + 31) / 32; ++p) {
                    if ((table.known[p][cell] & valid) != valid) return false;
                    rows[p] = table.values[p][cell];
                }
                affine = false;
                return true;
            }
        }

        // A consensus of one surviving literal also defines an exact gate.
        const auto &pixel = drawing.circuit[slot];
        int count = 0;
        for (int b = 0; b < branches; ++b)
            for (int sign = 0; sign < 2; ++sign) count += population(pixel.syn[b][sign][cell].winners());
        if (count != 1) return false;
        Word f = pixel.hypothesis(cell);
        Word primary = 1u << (f & 31);
        parents = primary;
        if (f >> 6) parents |= 1u << neighbor_of(f);
        bias = f & 32;
        affine = (f >> 6) != 1;
        arity = population(parents);
        if (!affine) rows[0] = bias ? 7 : 8; // 00, 01, 10, 11 for a signed conjunction.
        return true;
    }

    AX_FN static Word split_rank(int count, int stage, int output, Word variable, bool preferred) {
        // A smaller surviving support wins. Ties visit the later stage first.
        return (((Word(count * 8 + 7 - stage) * 32 + output) * 32
                 + Word(__builtin_ctz(variable))) << 1) | Word(preferred);
    }

    AX_FN Implications filter(Clamps in, Clamps out, int cell, int stage) const {
        Implications r{};
        Word bit = 1u << cell;
        bool high = out.on & bit, low = out.off & bit;
        Word open = parents & ~in.known();
        if (affine) {
            bool fixed_value = (population(in.on & parents) & 1) != bias;
            if (!open) r.output.add(bit, fixed_value);
            else if (high || low) {
                if (!(open & (open - 1))) r.input.add(open, high != fixed_value);
                else r.choice = split_rank(1 << bound(population(open) - 1, 0, 8), stage, cell, open, false);
            }
            return r;
        }
        if (!open) {
            Word address = CausalCircuit::address(in.on, parents);
            r.output.add(bit, (rows[address >> 5] >> (address & 31)) & 1);
            return r;
        }

        if (!(high || low || (parents & in.known()))) return r;
        Word fixed = CausalCircuit::address(in.known(), parents);
        Word selected = CausalCircuit::address(in.on, parents);
        Word mask = arity < 5 ? (1u << (1u << arity)) - 1 : ~0u;
        for (int k = 0; k < 5; ++k) {
            if ((fixed >> k) & 1) mask &= ((selected >> k) & 1) ? axis_pattern(k) : ~axis_pattern(k);
        }

        Word positions = 0, page_ones = 0, page_zeros = 0, possible_one = 0, possible_zero = 0;
        int count = 0;
        for (int page = 0; page < ((1 << arity) + 31) / 32; ++page) {
            if ((Word(page) & (fixed >> 5)) != (selected >> 5)) continue;
            Word values = rows[page];
            Word support = mask & (high ? values : (low ? ~values : ~0u));
            if (!support) continue;
            positions |= support;
            page_ones |= Word(page);
            page_zeros |= ~Word(page);
            possible_one |= support & values;
            possible_zero |= support & ~values;
            count += population(support);
        }
        if (!possible_one) r.output.off |= bit;
        if (!possible_zero) r.output.on |= bit;
        if (!positions || !(high || low)) return r;

        Word first_free = open & (0u - open);
        bool prefer_high = false;
        Word remaining = parents;
        for (int k = 0; remaining; ++k, remaining &= remaining - 1) {
            Word parent = remaining & (0u - remaining);
            if (!(open & parent)) continue;
            Word axis = axis_pattern(k);
            Word ones = k < 5 ? positions & axis : page_ones & (1u << (k - 5));
            Word zeros = k < 5 ? positions & ~axis : page_zeros & (1u << (k - 5));
            if (!ones) r.input.off |= parent;
            if (!zeros) r.input.on |= parent;
            if (parent == first_free && k < 5) prefer_high = population(ones) > population(zeros);
        }
        if (count < (1 << population(open)) && !r.input.known())
            r.choice = split_rank(count, stage, cell, first_free, prefer_high);
        return r;
    }
};

// Assignment stamps let a backtracking step retract all implications of a
// decision without copying the whole program at each search depth.
template<class Solver>
AX_FN bool solve_program(Solver &s, Word target, Word length, Word budget, Word &answer) {
    Clamps frame[9]{};
    frame[length] = {~target, target};
    int depth = 0;
    for (Word node = 0; node < budget; ++node) {
        bool conflict = false, changed;
        Word best;
        do {
            changed = false;
            best = ~0u;
            for (int k = int(length) - 1; k >= 0; --k) {
                auto r = s.filter(k, frame[k], frame[k + 1]);
                Word newly_in = r.input.known() & ~frame[k].known();
                Word newly_out = r.output.known() & ~frame[k + 1].known();
                if (((frame[k].off | r.input.off) & (frame[k].on | r.input.on))
                    || ((frame[k + 1].off | r.output.off) & (frame[k + 1].on | r.output.on))) {
                    conflict = true;
                    break;
                }
                frame[k].on |= r.input.on;
                frame[k].off |= r.input.off;
                frame[k + 1].on |= r.output.on;
                frame[k + 1].off |= r.output.off;
                if (newly_in) s.stamp(k, newly_in, depth + 1);
                if (newly_out) s.stamp(k + 1, newly_out, depth + 1);
                changed |= (newly_in | newly_out) != 0;
                best = r.choice < best ? r.choice : best;
            }
        } while (changed && !conflict);

        Word decision;
        if (conflict) {
            while (depth && (s.saved(depth - 1) & 512)) --depth;
            if (!depth) return false;
            for (unsigned k = 0; k < length; ++k) {
                Word erased = s.retract(k, depth + 1);
                frame[k].on &= ~erased;
                frame[k].off &= ~erased;
            }
            decision = (s.saved(depth - 1) ^ 1) | 512;
        } else {
            if (frame[0].known() == ~0u) { answer = frame[0].on; return true; }
            if (best == ~0u) decision = Word(__builtin_ctz(~frame[0].known())) * 2;
            else decision = ((7u - ((best >> 11) & 7)) << 6) | (best & 63);
            ++depth;
        }
        s.save(depth - 1, decision);
        int stage = (decision >> 6) & 7;
        Word bit = 1u << ((decision >> 1) & 31);
        frame[stage].add(bit, decision & 1);
        s.stamp(stage, bit, depth + 1);
    }
    return false;
}
} // namespace axion
