#pragma once
#include "recall.hpp"

namespace axion {

// Two four-way sets per cue. The two addresses omit different parts of the
// assembly, so a corrupted receptor can still reach a previously occupied set.
struct Association { Word cue, outcome, support; };
static_assert(sizeof(Association) == 12);

struct Archive {
    Association *entries{};
    Word slots{};

    AX_FN Word bucket(Word key, int bank) const {
        Word part = bank ? (key & 0xffffff00u) : (key & 0x00ffffffu);
        Word salt = bank ? 0x7c33e0c5u : 0x1b4a790fu;
        return (mix(part ^ salt) & (slots / 4 - 1)) * 4;
    }

    AX_FN int locate(Word key, int &victim) const {
        if (!entries || !key) return -1;
        int match = -1, best = 3;
        Word lowest = ~0u;
        for (int bank = 0; bank < 2; ++bank) {
            Word start = bucket(key, bank);
            for (int way = 0; way < 4; ++way) {
                int index = int(start + way);
                const auto &record = entries[index];
                int dist = population(record.cue ^ key);
                if (record.support && dist < best) { best = dist; match = index; }
                if (record.support < lowest) { lowest = record.support; victim = index; }
            }
        }
        return match;
    }

    AX_FN Word recall(Word key, int recent_score) const {
        int unused = -1;
        int i = locate(key, unused);
        if (i < 0) return 0;
        int similarity = 64 - 2 * population(entries[i].cue ^ key);
        return similarity > recent_score ? entries[i].outcome : 0;
    }

    AX_FN void store(Word key, Word value, Word count = 1) const {
        if (!entries || !key || !value) return;
        int victim = -1;
        int i = locate(key, victim);
        // Similar cues with different outcomes can be distinct words.
        // Allocate both when the observations distinguish their referents.
        if (i >= 0 && entries[i].cue != key && population(entries[i].outcome ^ value) > 2) i = -1;
        if (i < 0) entries[victim] = {key, value, count};
        else {
            auto &record = entries[i];
            if (population(record.outcome ^ value) <= 2)
                record.support = Word(bound(int(record.support + count), 1, 255));
            else if (count >= record.support) record = {key, value, 1};
            else record.support -= count;
        }
    }

    AX_FN void consolidate(const Engram &row) const {
        for (int h = 0; h < 3; ++h) store(row.item[h].lo, row.item[3].lo);
    }
    AX_FN void vote(Votes &votes, const Engram &query) const {
        for (int h = 0; h < 3; ++h) {
            int unused = -1;
            int i = locate(query.item[h].lo, unused);
            if (i < 0) continue;
            auto r = entries[i];
            int favor = population(r.outcome ^ query.item[3].lo) - population(r.outcome ^ query.item[4].lo);
            int gain = r.cue == query.item[h].lo ? 4 : 2;
            votes.mass[h] += 4096;
            votes.preference[h] += 4096 * gain * favor;
        }
    }
};
} // namespace axion
