#pragma once

// This header is compiled as ordinary C++ and as freestanding CUDA C++.
#if defined(__CUDA__)
#define AX_FN __attribute__((device)) inline __attribute__((always_inline))
#else
#define AX_FN inline
#endif

namespace axion {

using Word = unsigned int;
static_assert(sizeof(Word) == 4 && sizeof(short) == 2);

enum class Learning : unsigned { frozen, motor, full, no_scaling };

template<Learning L>
inline constexpr bool plastic_core = L == Learning::full || L == Learning::no_scaling;

AX_FN int bound(int value, int low, int high) {
    return value < low ? low : (value > high ? high : value);
}

AX_FN Word mix(Word value) {
    value ^= value >> 16;
    value *= 0x21f0aaadu;
    value ^= value >> 15;
    value *= 0x735a2d97u;
    return value ^ (value >> 15);
}

AX_FN Word trial_key(Word seed, Word animal, Word episode) {
    return mix(mix(animal + 0xb5297a4du) ^ seed ^ (episode * 0x9e3779b9u));
}

AX_FN int population(Word bits) {
    return __builtin_popcount(bits);
}

// Each cell receives a local tile and a second, disjoint tile. Different lanes
// extend their dendrites to different remote tiles. No stored source indices.
AX_FN int remote_tile(int tile, int cell, int tiles) {
    return tile ^ (1 + 2 * (cell & (tiles / 2 - 1)));
}

struct alignas(16) Weights {
    Word sensory;
    Word local;
    Word remote;
    short motor[2];
};
static_assert(sizeof(Weights) == 16);

AX_FN Weights initial_weights(Word seed, Word animal, int cell) {
    Word key = trial_key(seed, animal, cell);
    return {mix(key), mix(key + 1), mix(key + 2),
            {static_cast<short>(int(key & 15) - 7),
             static_cast<short>(int((key >> 4) & 15) - 7)}};
}

struct Counts { Word wins, ticks, spikes, matches, image; };

struct Sums {
    int left, right, mass;
};

struct WideSums {
    long long left, right;
    int mass;
};
static_assert(sizeof(WideSums) == 24);

struct Values {
    int left, right;
    AX_FN int selected(int action) const { return action ? right : left; }
};

template<class Sum>
AX_FN Values normalize(Sum sums) {
    int mass = sums.mass > 0 ? sums.mass : 1;
    return {int(sums.left / mass), int(sums.right / mass)};
}

struct Activity {
    int voltage, adaptation, trace;

    AX_FN Word pack(int tag_age) const {
        return Word(voltage + 16) | (Word(adaptation) << 8) | (Word(trace) << 16) | (Word(tag_age) << 24);
    }
    AX_FN static Activity unpack(Word bits) {
        return {int(bits & 255) - 16, int((bits >> 8) & 255), int((bits >> 16) & 255)};
    }
};

// A cell probes one candidate input or remote synapse per episode. Age is the
// remaining coincidence window. Credit is latched at a decision, so sensory
// events after the action cannot rewrite its assignment.
struct Eligibility {
    int motor[2];
    Word sample;
    int age, credit;

    AX_FN Word pack_decision() const {
        return Word(motor[0]) | (Word(motor[1]) << 10) | (Word(credit + 1) << 20);
    }
    AX_FN void unpack_decision(Word bits) {
        motor[0] = int(bits & 1023);
        motor[1] = int((bits >> 10) & 1023);
        credit = int(bits >> 20) - 1;
    }
};

AX_FN Word probe_sample(Word key, int id) {
    return mix(key ^ (unsigned(id) * 0x85ebca6bu));
}

template<Learning L>
struct Cell {
    Weights weights;
    Activity state;
    Eligibility tag;
    unsigned spikes;

    AX_FN void reset(Word key, int id) {
        state = {};
        if constexpr (L != Learning::frozen) {
            tag.motor[0] = tag.motor[1] = tag.credit = 0;
            if constexpr (plastic_core<L>) {
                tag.sample = probe_sample(key, id);
                tag.age = 0;
            }
        }
    }

    AX_FN bool tick(int id, Word input, Word local, Word remote, int active) {
        int matches = population(input & weights.sensory) * 3
                    + population(local & weights.local) + population(remote & weights.remote);
        // active already contains the negative half of the signed binary dots
        // and a population-dependent inhibitory current.
        int potential = 3 * state.voltage / 4 + 2 * matches - active;
        int threshold = 7 + (id & 3) + (state.adaptation >> 2);
        bool spike = potential >= threshold;
        state.voltage = bound(potential - (spike ? threshold : 0), -16, 24);
        state.adaptation -= (state.adaptation + 7) >> 3;
        state.adaptation += spike ? 6 : 0;
        // Interleave fast and slow readout traces within every tile.
        int shift = (id & 4) ? 4 : 2;
        state.trace -= (state.trace + (1 << shift) - 1) >> shift;
        state.trace = bound(state.trace + (spike ? 24 : 0), 0, 255);
        spikes += unsigned(spike);
        if constexpr (plastic_core<L>) {
            Word pre = (tag.sample & 32) ? remote : input;
            bool eligible = ((pre >> (tag.sample & 31)) & 1)
                         && potential >= threshold - 4 && potential <= threshold + 8;
            tag.age = eligible ? 12 : (tag.age > 0 ? tag.age - 1 : 0);
        }
        return spike;
    }

    AX_FN Sums read() const {
        return {weights.motor[0] * state.trace, weights.motor[1] * state.trace, state.trace};
    }

    AX_FN void commit(int action, int prediction) {
        for (int a = 0; a < 2; ++a)
            tag.motor[a] = 3 * tag.motor[a] / 4 + (a == action ? state.trace : 0);
        if constexpr (plastic_core<L>) {
            int influence = (action ? weights.motor[1] : weights.motor[0]) - prediction;
            tag.credit = tag.age ? (influence > 0) - (influence < 0) : 0;
        }
    }

    AX_FN void reinforce(int error) {
        int delta = bound(error, -512, 512);
        for (int a = 0; a < 2; ++a) {
            int change = delta * tag.motor[a] / 128;
            weights.motor[a] = static_cast<short>(bound(weights.motor[a] + change, -2048, 2048));
        }
        if constexpr (plastic_core<L>) {
            int direction = delta * tag.credit;
            // Error magnitude controls the probability of changing a binary weight.
            int gate = int((tag.sample >> 8) & 2047);
            Word bit = 1u << (tag.sample & 31);
            bool remote = tag.sample & 32;
            Word bank = remote ? weights.remote : weights.sensory;
            if (direction > gate) bank |= bit;
            if (-direction > gate) bank &= ~bit;
            // A rare, reward-independent adjustment to sampled sensory gain
            // recruits silent cells and restrains cells with sustained activity.
            if constexpr (L == Learning::full) {
                if (!remote && (tag.sample >> 24) == 0) {
                    int use = tag.motor[0] + tag.motor[1];
                    if (use == 0) bank |= bit;
                    if (use >= 32) bank &= ~bit;
                }
            }
            if (remote) weights.remote = bank;
            else weights.sensory = bank;
        }
    }
};

// A binary action policy driven only by spike traces, a random draw and an
// eventual scalar outcome. A new arena can use choose and outcome directly.
template<Learning L>
struct Policy {
    int last_q{};
    bool committed{};

    template<class Network>
    AX_FN int choose(Network &net, Word draw, int forced = -1) {
        Values values = normalize(net.read());
        int action = values.right == values.left ? int(draw & 1) : values.right > values.left;
        if constexpr (L != Learning::frozen) {
            if ((draw & 14) == 0) action = int((draw >> 8) & 1);
        }
        if (forced >= 0) action = forced;
        commit(net, values, action);
        return action;
    }

    template<class Network>
    AX_FN void commit(Network &net, Values values, int action, int consolidation = 1) {
        if constexpr (L != Learning::frozen) {
            int q = values.selected(action);
            if (committed) net.reinforce(bound(7 * q / 8 - last_q, -512, 512) / consolidation);
            last_q = q;
            net.commit(action, q);
        }
        committed = true;
    }

    template<class Network>
    AX_FN void outcome(Network &net, int reward, int consolidation = 1) const {
        if constexpr (L != Learning::frozen)
            if (committed) net.reinforce(bound(reward - last_q, -512, 512) / consolidation);
    }
};

AX_FN int active_cost(int sensory_count, Word local, Word remote) {
    int recurrent_count = population(local) + population(remote);
    return 3 * sensory_count + recurrent_count + (recurrent_count >> 2);
}

} // namespace axion
