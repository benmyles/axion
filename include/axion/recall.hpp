#pragma once
#include "model.hpp"

namespace axion {

enum class Recall : Word { cortical, content, associative };
constexpr int memory_rows = 32;
constexpr int channels = 4; // three temporal positions and their joint relation geometry

struct SpikeCode {
    Word lo, hi;
};

// The afferent compartment emits a binary onset response to an assembly of
// receptors that fired on a majority of the sensory volleys in a presentation.
AX_FN bool onset_spike(Word weights, Word receptors, int active) {
    return 2 * population(weights & receptors) > active;
}

// Fixed-width assemblies can be compared with two XORs and population
// counts. Silence supplies no positive evidence of a shared identity.
AX_FN int overlap(SpikeCode a, SpikeCode b) {
    if (!(a.lo | a.hi) || !(b.lo | b.hi)) return 0;
    return 64 - population(a.lo ^ b.lo) - population(a.hi ^ b.hi);
}

// A coincidence gate rejects background overlap between unrelated assemblies.
AX_FN int coincidence(SpikeCode a, SpikeCode b) {
    return bound(4 * (overlap(a, b) - 42), 0, 64);
}

struct Engram {
    SpikeCode item[5];
    Word geometry;

    AX_FN void bind() {
        geometry = Word(coincidence(item[0], item[1]))
                 | (Word(coincidence(item[0], item[2])) << 8)
                 | (Word(coincidence(item[1], item[2])) << 16);
    }
    AX_FN void prefer(int action) {
        if (action) {
            SpikeCode swap = item[4];
            item[4] = item[3];
            item[3] = swap;
        }
    }
};
static_assert(sizeof(Engram) == 44);

// The row stores the two action assemblies in reward-preference order. The
// value update is proportional to R * (features_chosen - features_alternative).
struct MemoryBank {
    Engram rows[memory_rows];
    int evidence[channels];
    Word used, next;
};

AX_FN int strength(int similarity) {
    int above = bound(similarity - 32, 0, 32);
    return above * above;
}

struct Votes {
    int preference[channels]{}, mass[channels]{}, base{};
    AX_FN void add(const Votes &other) {
        base += other.base;
        for (int h = 0; h < channels; ++h) {
            preference[h] += other.preference[h];
            mass[h] += other.mass[h];
        }
    }
};

// Open-ended cued recall uses the strongest match across presentation
// positions. The remembered, rewarded assembly supplies a new sensory goal.
AX_FN int binding_score(SpikeCode cue, const Engram &row, Word slot) {
    int best = 0;
    for (int j = 0; j < 3; ++j) {
        int affinity = overlap(cue, row.item[j]);
        best = affinity > best ? affinity : best;
    }
    return 32 * best + 31 - int(slot);
}

AX_FN Votes recall_row(const Engram &q, const Engram &row, Recall mode) {
    Votes v;
    int agree = coincidence(q.item[4], row.item[3]) + coincidence(q.item[3], row.item[4])
              - coincidence(q.item[3], row.item[3]) - coincidence(q.item[4], row.item[4]);
    v.base = agree;
    for (int h = 0; h < 3; ++h) v.mass[h] = strength(coincidence(q.item[h], row.item[h]));
    if (mode == Recall::associative) {
        int worst = 0;
        for (int i = 0; i < 3; ++i) {
            int d = int((q.geometry >> (i * 8)) & 255) - int((row.geometry >> (i * 8)) & 255);
            d = d < 0 ? -d : d;
            worst = d > worst ? d : worst;
        }
        v.mass[3] = (64 - worst) * (64 - worst) / 4;
    }
    for (int h = 0; h < channels; ++h) v.preference[h] = v.mass[h] * agree;
    return v;
}

struct RecallState {
    int evidence[channels];
    Word used, next;

    AX_FN void load(const MemoryBank &bank) {
        used = bank.used;
        next = bank.next;
        for (int h = 0; h < channels; ++h) evidence[h] = bank.evidence[h];
    }
    AX_FN void save(MemoryBank &bank) const {
        bank.used = used;
        bank.next = next;
        for (int h = 0; h < channels; ++h) bank.evidence[h] = evidence[h];
    }
    AX_FN int predict(const Votes &votes, int (&margin)[channels], Recall mode) const {
        bool relational = mode == Recall::associative;
        int best = evidence[0];
        for (int h = 1; h < channels; ++h)
            if (h != 3 || relational) best = evidence[h] > best ? evidence[h] : best;
        int sum = 0, total = 0;
        int prior = votes.base / int(used + 2);
        for (int h = 0; h < channels; ++h) {
            if (h == 3 && !relational) continue;
            margin[h] = (votes.preference[h] - prior * votes.mass[h]) / (votes.mass[h] + 96);
            int attention = 64 >> bound((best - evidence[h]) / 8, 0, 6);
            sum += attention * margin[h];
            total += attention;
        }
        return 64 * sum / total;
    }
    AX_FN void update(const int (&margin)[channels], int preferred) {
        for (int h = 0; h < channels; ++h) {
            int alignment = preferred ? margin[h] : -margin[h];
            evidence[h] = evidence[h] * 15 / 16 + bound(alignment / 2, -24, 24);
        }
        next = (next + 1) & (memory_rows - 1);
        if (used < memory_rows) ++used;
    }
};

} // namespace axion
