#pragma once
#include "lesson.hpp"

namespace axion {

constexpr int branches = 3, tools_per_brain = 4;

// Thirty-two parallel two-bit synaptic mismatch counters in two machine words.
struct Costs {
    Word low, high;

    AX_FN Word winners() const { return ~(low | high); }
    AX_FN void observe(Word mismatch) {
        Word inc = mismatch & ~(low & high);
        high |= low & inc;
        low ^= inc;
    }
    AX_FN void lower_all() {
        // Called only after every candidate of this output has positive cost.
        high &= low;
        low = ~low;
    }
};

AX_FN Word shift_pixels(Word x, unsigned shift) {
    return (x << shift) | (x >> (32 - shift));
}

struct Inputs {
    Word branch[branches];
    AX_FN explicit Inputs(Word image)
        : branch{image, image & shift_pixels(image, 1), image ^ shift_pixels(image, 8)} {}
};

struct PixelCircuit {
    Costs syn[branches][2][32];

    AX_FN bool fire(const Inputs &x, int cell, int count) const {
        int drive = 0;
        for (int b = 0; b < count; ++b) {
            Word on = syn[b][0][cell].winners(), off = syn[b][1][cell].winners();
            drive += 2 * (population(on & x.branch[b]) + population(off & ~x.branch[b]))
                   - population(on) - population(off);
        }
        return drive == 0 ? (x.branch[0] >> cell) & 1 : drive > 0;
    }

    AX_FN Word hypothesis(int cell) const {
        for (int b = 0; b < branches; ++b) {
            for (int sign = 0; sign < 2; ++sign) {
                Word candidates = syn[b][sign][cell].winners();
                if (candidates) return Word((2 * b + sign) * 32 + __builtin_ctz(candidates));
            }
        }
        return Word(cell);
    }

    AX_FN void observe(const Inputs &x, int cell, bool target, int count, bool fresh) {
        Word any = 0;
        for (int b = 0; b < count; ++b) {
            Word mismatch = target ? ~x.branch[b] : x.branch[b];
            for (int sign = 0; sign < 2; ++sign) {
                auto &c = syn[b][sign][cell];
                if (fresh) c = {};
                c.observe(sign ? ~mismatch : mismatch);
                any |= c.winners();
            }
        }
        if (!any) {
            for (int b = 0; b < count; ++b)
                for (int sign = 0; sign < 2; ++sign) syn[b][sign][cell].lower_all();
        }
        if (fresh && count < branches)
            for (int b = count; b < branches; ++b)
                for (int sign = 0; sign < 2; ++sign) syn[b][sign][cell] = {};
    }
};

struct Palette {
    Word cue[tools_per_brain];
    Word used, next;

    AX_FN int find(Word key) const {
        for (Word i = 0; i < used; ++i)
            if (cue[i] == key) return int(i);
        return -1;
    }

    AX_FN int recruit(Word key) {
        Word slot = next;
        next = (next + 1) & (tools_per_brain - 1);
        if (used < tools_per_brain) ++used;
        cue[slot] = key;
        return int(slot);
    }
};

// A second population binds changes of entire input and output assemblies.
// Each occupied pivot is zero in every other row. This reduced basis lets
// 32 lanes project into the experienced span in one simultaneous XOR sum.
struct Transition {
    Word input, output;
};
struct Span {
    Transition row[32];
    Word anchor, response, confirmed;
};
AX_FN void update_pivot(Transition &row, int lane, int pivot, Transition delta) {
    if (lane == pivot) row = delta;
    else if ((row.input >> pivot) & 1) {
        row.input ^= delta.input;
        row.output ^= delta.output;
    }
}

struct DrawingMemory {
    Palette palette;
    PixelCircuit circuit[tools_per_brain];
    Span spans[tools_per_brain];
};
static_assert(sizeof(DrawingMemory) == 7240);

// A changed output following a single changed input establishes a causal
// afferent for that output. Each cell learns a local truth table over at most
// eight discovered parents. Adding a parent invalidates its old table.
struct CausalCircuit {
    Word parents[32];
    unsigned char trust[32];
    Word known[8][32], values[8][32];
    Word previous_input, previous_output, observed;

    AX_FN static Word address(Word picture, Word parents) {
        Word index = 0, place = 1;
        while (parents) {
            Word parent = parents & (0u - parents);
            if (picture & parent) index |= place;
            place <<= 1;
            parents &= parents - 1;
        }
        return index;
    }
    AX_FN int predict(Word picture, int cell) const {
        Word mask = parents[cell];
        if (trust[cell] < 8 || !mask || population(mask) > 8) return -1;
        Word index = address(picture, mask), bit = 1u << (index & 31);
        if (!(known[index >> 5][cell] & bit)) return -1;
        return int(bool(values[index >> 5][cell] & bit));
    }
    AX_FN int unknown(Word picture, int cell) const {
        Word mask = parents[cell];
        if (!mask || population(mask) > 8) return 0;
        Word index = address(picture, mask);
        return !(known[index >> 5][cell] & (1u << (index & 31)));
    }
    AX_FN void witness(int cell, Word picture, bool target, Word cause, bool fresh) {
        Word prior = fresh ? 0 : parents[cell];
        Word mask = prior | cause;
        if (fresh || mask != prior) {
            parents[cell] = mask;
            trust[cell] = 0;
            for (int i = 0; i < 8; ++i) { known[i][cell] = 0; values[i][cell] = 0; }
        }
        if (!mask || population(mask) > 8) return;
        Word index = address(picture, mask), bit = 1u << (index & 31);
        auto &seen = known[index >> 5][cell], &output = values[index >> 5][cell];
        if (seen & bit) {
            bool matched = bool(output & bit) == target;
            trust[cell] = static_cast<unsigned char>(bound(int(trust[cell]) + (matched ? 1 : -16), 0, 32));
        }
        seen |= bit;
        if (target) output |= bit;
        else output &= ~bit;
    }
    AX_FN Word intervention(Word input, bool fresh) const {
        Word change = input ^ previous_input;
        return !fresh && observed && population(change) == 1 ? change : 0;
    }
};
struct InterventionMemory { CausalCircuit circuit[tools_per_brain]; };
static_assert(sizeof(InterventionMemory) == 8880);

// Bit j in a plane is the activity under counterfactual intervention j.
// The same learned literal simultaneously predicts 32 intervention outcomes.
AX_FN Word apply_literal(Word literal, Word source, Word neighbor) {
    Word branch = literal >> 6;
    Word value = branch == 0 ? source : (branch == 1 ? source & neighbor : source ^ neighbor);
    return (literal & 32) ? ~value : value;
}
AX_FN int neighbor_of(Word literal) {
    return int((literal - ((literal >> 6) == 1 ? 1u : 8u)) & 31);
}

template<class Search>
AX_FN Word improve_input(Search &search, Word target) {
    Word best = target;
    int best_error = 33;
    for (int restart = 0; restart < 3; ++restart) {
        Word input = restart ? mix(target + 0x952bb917u * restart) : target;
        int error = search.score(input, target, false, 0) >> 5;
        if (error < best_error) { best_error = error; best = input; }
        for (int step = 0; step < 24 && best_error; ++step) {
            int salt = int(mix(input ^ target ^ Word(step)) & 31);
            int proposal = search.score(input, target, true, salt);
            int next_error = proposal >> 5;
            if (next_error >= error) break;
            input ^= 1u << ((proposal & 31) ^ salt);
            error = next_error;
            if (error < best_error) { best_error = error; best = input; }
        }
        if (!best_error) break;
    }
    return best;
}

// Hidden transformations are private to the workshop. Each output pixel
// reads one literal from a randomly permuted bank. The actual wiring and
// polarity are never available to a learning cell.
struct Workshop {
    Word key;
    AX_FN static Workshop make(const Course &c, Word who) {
        return {trial_key(c.lesson ^ (c.reversed * 0x6d9b23cdu), who, 0x4a72fc13u)};
    }
    AX_FN static Word picture(const Course &c, Word who, Word index, Word alternative = 0) {
        Word key = trial_key(c.seed, who, index);
        if (c.experiments == Course::Experiments::off) {
            Word x = mix(key ^ 0x903de75bu);
            return (x & ~1u) | ((mix(x >> 1) ^ c.novel) & 1);
        }
        bool pair = !c.novel && c.experiments >= Course::Experiments::paired;
        Word draw = (pair ? trial_key(c.seed, who, index / 2) : key) + alternative * 0xe35a71b1u;
        Word bit = 1u << ((index / 4 + mix(c.seed)) & 31);
        // Keep both halves of every controlled experiment in the support
        // partition. Trial generation uses no hidden property of the tool.
        for (;;) {
            Word x = mix(draw++);
            if ((mix(x ^ 0x341c0ffdu) & 1) != c.novel) continue;
            if (pair && (mix((x ^ bit) ^ 0x341c0ffdu) & 1)) continue;
            return pair && (index & 1) ? x ^ bit : x;
        }
    }
    AX_FN static Word sequence(const Course &c, Word key, Word index, Word length) {
        if (!c.novel && c.experiments >= Course::Experiments::paired) index /= 2;
        Word ids = c.novel ? key >> 5 : (index ^ (mix(index / 2 + c.seed) & 1)) & 1;
        if (c.novel && length > 1) ids = (ids & ~3u) | ((key & 1) ? 1u : 2u);
        return ids;
    }
    AX_FN Word cue(int tool) const { return symbol(key, 901 + tool); }
    AX_FN int effect_bit(Word image, Word w, int tool, Word effects, unsigned j) const {
        unsigned bit = (j * ((w >> 9) | 1u) + (w >> 22)) & 31;
        Word source = image;
        if (tool) {
            if (effects == 1) source = image ^ shift_pixels(image, 5) ^ shift_pixels(image, 13);
            else if (effects == 2) return (population(image & mix(w + bit)) ^ (mix(w ^ 0x37a8142bu) >> j)) & 1;
            else if (effects == 3) {
                source = ((image ^ shift_pixels(image, 5)) & (shift_pixels(image, 13) ^ shift_pixels(image, 7)))
                       ^ (shift_pixels(image, 3) & shift_pixels(image, 10) & shift_pixels(image, 27));
            } else source = (w & 1) ? image & shift_pixels(image, 1) : image ^ shift_pixels(image, 8);
        }
        return ((source >> bit) ^ (mix(w ^ 0x37a8142bu) >> j)) & 1;
    }
    AX_FN Word apply(Word image, int tool, Word effects) const {
        Word w = mix(key ^ (0x62f35de9u * (tool + 1)));
#if defined(__CUDA__)
        Word lane, result;
        asm("mov.u32 %0, %%laneid;" : "=r"(lane));
        int pixel = effect_bit(image, w, tool, effects, lane);
        asm volatile("{ .reg .pred p; setp.ne.u32 p, %1, 0; vote.sync.ballot.b32 %0, p, -1; }"
                     : "=r"(result) : "r"(pixel));
        return result;
#else
        Word result = 0;
        for (int j = 0; j < 32; ++j) result |= Word(effect_bit(image, w, tool, effects, j)) << j;
        return result;
#endif
    }
};

template<Learning L, bool ForwardOnly = false, class Predictor>
AX_FN Counts draw_trials(Predictor &prediction, const Course &c, Word animal, Word first, Word episodes) {
    Word who = c.common_trials ? 0 : animal;
    Workshop world = Workshop::make(c, who);
    Counts out{};
    for (Word e = 0; e < episodes; ++e) {
        Word i = first + e, key = trial_key(c.seed, who, i);
        bool external = c.task == Task::canvas;
        bool invert = !ForwardOnly && ((c.task == Task::invention && c.novel) || (external && c.inverse));
        // A nonlinear partition keeps support and query inputs disjoint while
        // allowing independent demonstrations to span the affine input space.
        Word length = c.novel ? c.chain : 1;
        Word ids = Workshop::sequence(c, key, i, length);
        Word original = external ? c.signals.objects[0] : Workshop::picture(c, who, i);
        if (!external && !c.novel && c.experiments == Course::Experiments::curious) {
            Word tool = world.cue(ids & 1);
            Word bit = 1u << ((i / 4 + mix(c.seed)) & 31);
            if (i & 1) original = prediction.previous(tool, original ^ bit) ^ bit;
            else {
                int best = -1;
                for (Word option = 0; option < 8; ++option) {
                    Word try_input = Workshop::picture(c, who, i, option);
                    int info = prediction.uncertainty(tool, try_input, try_input ^ bit);
                    if (info > best) { best = info; original = try_input; }
                }
            }
        }
        if (c.mute_odor) original = 0;
        Word expected = original, cue = 0;
        // Only the simulator sees the physical transformation. A request to
        // construct an input exposes the desired outcome and the tool cues.
        if (!external) {
            for (Word j = 0; j < length; ++j)
                expected = world.apply(expected, (ids >> j) & 1, c.difficult);
        } else expected = invert ? original : c.signals.objects[4];
        Word image = invert ? expected : original;
        typename Predictor::Program program{};
        for (Word j = 0; j < length; ++j) {
            Word token = external ? (j == 0 ? c.signals.objects[1] : (j == 1 ? c.signals.objects[2] : c.signals.objects[3])) : world.cue((ids >> j) & 1);
            cue = token;
            if (invert) program.put(j, cue);
            else {
                image = prediction.apply(cue, image, c.recall);
                out.spikes += Word(population(image));
                ++out.ticks;
            }
        }
        Word proposal = image;
        if (invert) {
            proposal = prediction.construct(expected, program, length, c.recall, c.plan_nodes);
            out.spikes += Word(population(proposal));
            ++out.ticks;
            image = proposal;
            if (!external)
                for (Word j = 0; j < length; ++j)
                    image = world.apply(image, (ids >> j) & 1, c.difficult);
        }
        if constexpr (L != Learning::frozen) {
            if (!c.novel && c.recall != Recall::cortical) {
                Word observed = c.scrambled ? mix(key ^ 0x7187cadbu) : expected;
                prediction.observe(cue, original, observed, c.recall);
            }
        }
        out.wins += Word(image == expected);
        out.matches += Word(32 - population(image ^ expected));
        out.image = proposal;
    }
    return out;
}
} // namespace axion
