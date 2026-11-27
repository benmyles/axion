#pragma once
#include "braid_route.hpp"
namespace axion::life::braid {
enum Arm : Word {
    learned,
    direct,
    obscured,
    untrained,
    static_cells,
    mirror_hand,
    no_anchor,
    unpaired,
    untied,
    one_word,
    lex_only,
    arms
};
struct Family {
    Genes genes{};
    Word food = 0;
};
struct Trial {
    Word world, rig, start, utterance, length, missing, convention, teacher[voices];
    struct Twin {
        Word wanted, imagined, known, before, after, plan, n, found, correct, food, cost, seen, exposures, tuition,
            writes, muscle_writes, station_mask, orientation, order_edits, lexicon[voices], muscles[4];
    } twin[2];
    Genes germ;
};
struct Packed {
    Word motor_maps, known_motors;
    unsigned char words[voices];
    Word known_words;
};
static_assert(sizeof(Packed) == 20);
AX_FN Packed pack(const Mind &m) {
    auto mtr = release(m, false), word = release(m, true);
    Packed p{};
    p.known_motors = mtr.present;
    p.known_words = word.present | (Word(reversed(m)) << 16);
    for (Word i = 0; i < 4; ++i)
        p.motor_maps |= Word(mtr.table[i]) << (8 * i);
    for (Word i = 0; i < voices; ++i)
        p.words[i] = word.table[i];
    return p;
}
struct Food {
    Word source = 0, swallowed = 0;
};
AX_FN Food harvest(Word state, Word sockets, Word offered) {
    // Four independent catchments. A matching float aligns a hole above its
    // own cup. Each opening can discharge at most a quarter of the source.
    Food q{offered, 0};
    Word grain = offered / 4;
    for (Word i = 0; i < 4; ++i)
        if (pigment(state, i) == pigment(sockets, i)) {
            q.source -= grain;
            q.swallowed += grain;
        }
    return q;
}
} // namespace axion::life::braid

namespace axion::life::braid {
struct Atlas {
    Packed input{};
    Word movements[24]{};
};
AX_FN Word packed_plan(Plan p) {
    Word bits = 0;
    for (Word k = 0; k < p.size; ++k)
        bits |= ((p.stream >> (3 * k)) & 3) << (2 * k);
    return bits | (p.size << 16) | (p.found << 20);
}
} // namespace axion::life::braid
