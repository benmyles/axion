#pragma once
#include "life.hpp"

namespace axion::life::runnel {
using Byte = unsigned char;
constexpr Word max_span = 96;
struct Genes {
    Word hand = 0, eye = 0, plastic = 0;
};
// Each texture recruits four motor assemblies. Two banks of 32 bits cover
// the 64 coincidences. A flex can excite either, both or neither sensory axon.
struct Circuit {
    Word touched[2]{}, forward[2]{}, backward[2]{}, witnessed[2]{};
};
struct Mind {
    Genes genes{};
    Circuit cells{};
    Word practice = 0;
};
struct Saved {
    Mind mind{};
    Word tag = 0x44454552u, extent = sizeof(Saved);
};
enum Mode : Word {
    forming = 1,
    watching = 2,
    sensing = 4,
    tide = 8,
    responding = 16,
    scrambled = 32,
    reversed = 64,
    decoupled = 128,
    relating = 256,
    families = 512,
    swap_families = 1024,
    equal_families = 2048,
    compete_reason = 4096,
    handed = 8192,
    live = forming | watching | sensing | tide | responding
};
struct Totals {
    Count opportunities = 0, folds = 0, created = 0, retuned = 0, probes = 0, peered = 0, personal_writes = 0,
          social_writes = 0, recalled = 0, social_use = 0, predicted = 0, accurate = 0, work = 0, wet_contacts = 0,
          stopped = 0, lifted_water = 0, lifted_food = 0, thumped_water = 0, thumped_carbon = 0, tasted = 0,
          swallowed = 0, selected = 0;
};
struct State {
    Word mode = live, seed = 71633, period = 8, version = 2;
    Totals book{};
};
struct Social {
    Count altered = 0, better = 0, worse = 0, marked = 0, water = 0, carbon = 0, lifted_water = 0, lifted_carbon = 0,
          meals = 0, swallowed = 0, inferred_changes = 0, inferred_better = 0, inferred_worse = 0, observed_makers = 0;
};
struct Field {
    Mind *minds = nullptr;
    Byte *seams = nullptr;
    Totals *book = nullptr;
    Word mode = 0, stone = 0, period = 8;
    Social *social = nullptr;
};
static_assert(sizeof(Genes) == 12 && sizeof(Circuit) == 32 && sizeof(Mind) == 48 && sizeof(Saved) == 56);
AX_FN Genes founder(Word k) { return {mix(k + 5297), mix(k + 9791), mix(k + 1307)}; }
AX_FN Word mutate(Word x, Word k, Word rate) { return (k & 255) < rate ? x ^ (1u << ((k >> 8) & 31)) : x; }
AX_FN Genes inherit(Genes g, Word k, Word rate) {
    return {mutate(g.hand, mix(k + 7213), rate), mutate(g.eye, mix(k + 1033), rate),
            mutate(g.plastic, mix(k + 7877), rate)};
}
AX_FN Word pose(Word local, Word frame) { return ((frame & 3) + ((frame & 4) ? 0u - local : local)) & 3; }
AX_FN Word native(Word public_pose, Word frame) {
    return ((frame & 4) ? frame - public_pose : public_pose - frame) & 3;
}
AX_FN Word texture(Word left, Word right) { return (left & 3) | ((right & 3) << 2); }
AX_FN Word material_id(Word grains, bool chiral) { return (grains & 0xfffffu) | (chiral ? 0x80000000u : 0u); }
AX_FN Word contact(Word stone, Word cue, Word fold) {
    // Each mineral exposes a spring normal under its surface groove. Seating a
    // strip between two normals sets its rest angle. Opposed seats can close both sides.
    Word left = (stone >> (2 * (cue & 3))) & 3, right = (stone >> (8 + 2 * (cue >> 2))) & 3;
    if ((stone >> 31) && ((stone >> (16 + (cue & 3))) & 1))
        fold = 0u - fold;
    Word angle = (fold + left - right) & 3;
    return (0xe1u >> (2 * angle)) & 3; // forward, closed, backward or free under opposite loads
}
AX_FN Word fold_at(Byte record, Word axis) { return (record >> (4 * axis)) & 7; }
AX_FN bool traced(Byte record, Word axis) { return (record >> (4 * axis)) & 8; }
AX_FN bool entitled(Word mode, bool label) {
    return !(mode & families) || (mode & equal_families) || (label != bool(mode & swap_families));
}
AX_FN void set_fold(Byte &record, Word axis, Word value) {
    record = Byte((record & ~(15u << (4 * axis))) | (value << (4 * axis)));
}
AX_FN Word row(const Genes &g, Word cue) {
    Word sensed = cue;
    if (!(g.eye & 1))
        sensed &= 3;
    if (!(g.eye & 2))
        sensed &= 12;
    return (sensed ^ ((g.eye >> 8) & 15)) & 15;
}
AX_FN Word nibble(const Word (&plane)[2], Word row_id) { return (plane[row_id >> 3] >> (4 * (row_id & 7))) & 15; }
AX_FN Word population(const Word (&plane)[2]) { return __builtin_popcount(plane[0]) + __builtin_popcount(plane[1]); }
AX_FN bool valid(const Mind &m) {
    for (Word i = 0; i < 2; ++i)
        if ((m.cells.forward[i] | m.cells.backward[i] | m.cells.witnessed[i]) & ~m.cells.touched[i])
            return false;
    return true;
}
AX_FN Word choose_bit(Word options, Word first) {
    Word shifted = ((options | (options << 4)) >> (first & 3)) & 15;
    return (first + __builtin_ctz(shifted)) & 3;
}
struct Choice {
    Word motor, known, from_peer, expected;
};
AX_FN Choice choose(const Mind &m, Word cue, Word dir, Word drive, bool explore) {
    auto &s = m.cells;
    Word r = row(m.genes, cue);
    Word a = nibble(s.forward, r), b = nibble(s.backward, r), t = nibble(s.touched, r);
    Word yes = dir ? b : a, no = dir ? a : b, options = yes & ~no;
    if (!options)
        options = yes;
    if (!(m.genes.hand & 8))
        options = 0;
    Word known = options != 0;
    if (explore && (!known || ((drive >> 7) & 31) < ((m.genes.hand >> 12) & 7)) && t != 15) {
        options = 15 ^ t;
        known = 0;
    }
    if (!options)
        options = 15;
    Word n = choose_bit(options, drive), bit = 1u << n;
    return {n, known, Word(known && (nibble(s.witnessed, r) & bit)), Word(!!(a & bit)) + 2 * Word(!!(b & bit))};
}
AX_FN Word gate(Word table, bool before, bool after) { return (table >> (2 * Word(before) + Word(after))) & 1; }
AX_FN void write_bit(Word &plane, Word bit, Word value) { plane = (plane & ~bit) | (value ? bit : 0); }
AX_FN bool observe(Mind &m, Word cue, Word gesture, Word frame, Word result, bool peer, Word mode, Word visible = 3) {
    if (!(mode & sensing) || !(m.genes.eye & (peer ? 8u : 4u)) || (peer && !(mode & watching)))
        return false;
    if (peer && (mode & scrambled))
        result = ((result & 1) << 1) | (result >> 1);
    Word n = peer && !(m.genes.eye & 16) ? gesture : native(gesture, frame);
    Word r = row(m.genes, cue), h = r >> 3, bit = 1u << (4 * (r & 7) + n), rule = m.genes.plastic >> (peer ? 4 : 0);
    auto &s = m.cells;
    Word f = gate(rule, s.forward[h] & bit, result & 1), b = gate(rule, s.backward[h] & bit, result & 2);
    if (!(visible & 1))
        f = !!(s.forward[h] & bit);
    if (!(visible & 2))
        b = !!(s.backward[h] & bit);
    bool changed = !(s.touched[h] & bit) || bool(s.forward[h] & bit) != bool(f) || bool(s.backward[h] & bit) != bool(b);
    write_bit(s.forward[h], bit, f);
    write_bit(s.backward[h], bit, b);
    s.touched[h] |= bit;
    // Credit a peer-derived decision only until a private flex has confirmed it.
    if (!peer || changed)
        write_bit(s.witnessed[h], bit, peer);
    Word pos = peer ? 16 : 0;
    if (((m.practice >> pos) & 65535) != 65535)
        m.practice += 1u << pos;
    return changed;
}
// One warp describes 16 textures at each flow direction. Admission and the folded
// motor stream together require three bits per decision at a fixed tie order.
struct Image {
    Word admitted = 0, motors[2]{};
};
static_assert(sizeof(Image) == 12);
AX_FN Word packed(const Image &im, Word cue, Word dir) {
    return ((im.motors[dir] >> (2 * cue)) & 3) | (((im.admitted >> (cue + 16 * dir)) & 1) << 2);
}
struct Unseen {
    Word tested = 0, both = 0, exact = 0, food = 0, water = 0, inferred = 0, seen_target = 0, attempted = 0,
         observations = 0, native_a = 0, native_b = 0, public_a = 0, public_b = 0, effect_a = 0, effect_b = 0,
         wanted = 0, target = 0, pivot = 0, stone_a = 0, stone_b = 0, proof_a = 16, proof_b = 16, rotation_a = 8,
         rotation_b = 8, signature = 0, goal_met = 0, calibrated = 0;
};
struct Family {
    Genes genes{};
    Word reserve = 0;
};
struct Gallery {
    Word from = 0, into = 0, back = 0, water = 0, food = 0, water_total = 0, carbon_total = 0, positive = 0,
         negative = 0, free = 0, closed = 0, highest = 0;
};
struct Transmission {
    Gallery gallery{};
    Word folds = 0, demonstrated = 0, own = 0, social = 0, own_writes = 0, social_writes = 0, recalled = 0,
         social_use = 0, mastered = 0, first_water = 0, first_food = 0, better = 0, poorer = 0, both_win = 0,
         alone_best = 0, alone_a = 0, alone_b = 0, changed_hand = 0;
};
} // namespace axion::life::runnel

namespace axion::life::runnel {
constexpr Word patrolling = 16384;
AX_FN Word gait(Genes g, bool near_food, bool far_food, bool folded, Word drive) {
    Word ports = Word(near_food) + 2 * Word(far_food) + 4 * Word(folded);
    Word reflex = (g.plastic >> (8 + 2 * ports)) & 3;
    return ((drive >> 13) & 31) < ((g.eye >> 24) & 7) ? (drive >> 5) & 3 : reflex;
}
struct Keeping {
    Word steps = 0, walked = 0, folded = 0, probed = 0, seen = 0, spent = 0, food = 0, eaten = 0, stock = 0, water = 0,
         store_water = 0, store_food = 0, reach = 0, built = 0, correct = 0, opening = 0, older = 0, offered = 0;
};
} // namespace axion::life::runnel
