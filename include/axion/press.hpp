#pragma once
#include "clay.hpp"
#include "material.hpp"

namespace axion::life::press {
AX_FN Word item(Word deck, Word i) { return (deck >> (2 * i)) & 3; }
using clay::Mind;
using clay::Tile;
using material::Grain;

// The two binary 4 x 4 projections occupy previously unused halves of the
// acoustic membrane and assembly words. A contact updates only the local row.
enum Coupling : Word {
    installed = 128,
    moving = 256,
    casting = 512,
    attending = 1024,
    tasting = 2048,
    choosing = 4096,
    bedded = 8192,
    tidal = 16384,
    feel_set = 32768,
    feeling = 65536,
    dislocated = 131072,
    stiff = 262144,
    full = installed | moving | casting | attending | tasting | choosing
};
AX_FN Word transition(Word previous, Word post, bool pulse, Word lut) {
    // Four binary contacts in parallel. The genome specifies a next value
    // for every combination of prior state, postsynaptic spike and modulator.
    Word low = (lut >> (4 * Word(pulse))) & 15;
    Word rest = (0u - (low & 1)) & ~previous;
    rest |= (0u - ((low >> 1) & 1)) & previous;
    Word fire = (0u - ((low >> 2) & 1)) & ~previous;
    fire |= (0u - ((low >> 3) & 1)) & previous;
    return ((rest & ~post) | (fire & post)) & 15;
}
AX_FN Word update(Word &tissue, Word receptor, Word tone, bool pulse, Word rule) {
    Word shift = 16 + 4 * receptor, old = (tissue >> shift) & 15;
    Word diff = old ^ transition(old, 1u << tone, pulse, rule);
    tissue ^= diff << shift;
    return __builtin_popcount(diff);
}
AX_FN Word see(Mind &m, Word cue, Word pitch, bool witnessed) {
    return update(m.charge, cue, pitch, witnessed, m.genes.membrane);
}
AX_FN Word taste(Mind &m, Word bath, Word pitch, bool nutrient) {
    return update(m.banks, bath, pitch, nutrient, m.genes.branches >> 8);
}
AX_FN Word row(Word matrix, Word address) { return (matrix >> (16 + 4 * address)) & 15; }
AX_FN Word predictions(const Mind &m, Word cue) {
    return row(m.charge, cue) ^ ((0u - ((m.genes.fingers >> 25) & 1)) & 15);
}
AX_FN Word values(const Mind &m, Word bath) { return row(m.banks, bath) ^ ((0u - ((m.genes.fingers >> 26) & 1)) & 15); }
AX_FN Word current(const Mind &m, Word forecast, Word bath) {
    Word value = values(m, bath), law = m.genes.fingers;
    Word good = __builtin_popcount(forecast & value), bad = __builtin_popcount(forecast & ~value);
    // An unsigned offset holds every possible excitatory/inhibitory current.
    return 12 + (1 + ((law >> 23) & 1)) * good - ((law >> 27) & 3) * bad;
}
AX_FN Word drive(const Mind &m, Word cue, Word bath) { return current(m, predictions(m, cue), bath); }
AX_FN Word elect(Word left, Word right, Word key) { return right == left ? key & 1 : Word(right > left); }
AX_FN Word choose(const Mind &m, Word a, Word b, Word bath, Word key, bool guided) {
    return guided ? elect(drive(m, a, bath), drive(m, b, bath), key) : key & 1;
}
struct Frozen {
    Word ranking = 0;
};
static_assert(sizeof(Frozen) == 4);
AX_FN Frozen compile(const Mind &m) {
    Frozen f{};
    for (Word bath = 0; bath < 4; ++bath) {
        Word activity[4];
        for (Word a = 0; a < 4; ++a)
            activity[a] = drive(m, a, bath);
        for (Word a = 0; a < 4; ++a) {
            Word rank = 0;
            for (Word b = 0; b < 4; ++b)
                rank += activity[b] < activity[a];
            f.ranking |= rank << (8 * bath + 2 * a);
        }
    }
    return f;
}
AX_FN Word choose(const Frozen &f, Word a, Word b, Word bath, Word key) {
    Word r = f.ranking >> (8 * bath);
    return elect((r >> (2 * a)) & 3, (r >> (2 * b)) & 3, key);
}
AX_FN Word appearance(Grain g, Word face) { return ((g.substance >> 4) ^ (face & 3)) & 3; }
AX_FN Word resonance(const Tile &surface, Grain g, Word face) {
    Word e = clay::groove(surface, face);
    return e & 128 ? e & 3 : material::face(g, face & 3);
}
// An unworked crystal in a laminated bed rings according to its optical band,
// the face loading it, and its own lattice orientation. A worked face retains
// the pitch left by its last physical impression.
AX_FN Word ring(const Tile &t, Grain g, Word face, bool laminated) {
    if (!laminated || (clay::groove(t, face) & 128))
        return resonance(t, g, face);
    return (appearance(g, face) + (g.substance >> 6)) & 3;
}
AX_FN Word mirror(Word face, Word turn) { return (face & 4) | ((turn - (face & 3)) & 3); }
AX_FN bool cast(Tile &receiver, Word dst, const Tile &source, Grain tool, Word face, bool food, Word tick,
                bool laminated = false) {
    Word e = clay::groove(source, face);
    Word relief = e & 128 ? (e >> 2) & 3 : material::face(tool, face & 3);
    Word pulse = siphon::radiate(0, ring(source, tool, face, laminated), relief, (face & 4) != 0, true, food);
    return clay::impress(receiver, dst, pulse, tick);
}
AX_FN Word flow(Word pitch, Word slit, Word water, Word available) {
    if (pitch != slit || !water)
        return 0;
    Word volume = 2 + (water < 124 ? water : 124) / 4;
    return volume < available ? volume : available;
}
AX_FN Word germ(const Mind &m) {
    return (m.genes.membrane & 255) | (m.genes.branches & 65280) | (((m.genes.fingers >> 23) & 63) << 16);
}
AX_FN void express(Mind &m, Word gene) {
    m.genes.membrane = (m.genes.membrane & ~255u) | (gene & 255);
    m.genes.branches = (m.genes.branches & ~65280u) | (gene & 65280);
    m.genes.fingers = (m.genes.fingers & ~(63u << 23)) | (((gene >> 16) & 63) << 23);
}
enum Intervention : Word {
    revised,
    before_change,
    self_taught,
    sight_closed,
    outcome_closed,
    stepped_sounds,
    other_bath,
    erased_forecast,
    erased_taste,
    smoothed,
    unguided,
    unmoved,
    mixed_contacts,
    trials_count
};
struct Record {
    Word trials = 0, edible = 0, old_choice = 0, food = 0, heat = 0, stamps = 0, copies = 0, demonstrations = 0,
         tastings = 0, sensory_edits = 0, taste_edits = 0, ties = 0, stocked = 0, residual = 0;
};
struct Specimen {
    Word gene = 0, surface = 0, appetite = 0, prior_appetite = 0, baths = 0, shifted = 0, pitches = 0, casts[4]{},
         skins[4]{}, faces[4]{}, decisions = 0, outcomes = 0, raw_tips = 0;
};
struct Family {
    Word gene = 0, eggs = 0;
};
struct Survey {
    Word useful = 0, correct[6]{}, mass[6]{}, work[6]{}, choices = 0, live = 0, seen = 0, valued = 0;
};
struct Piece {
    Tile plate{};
    Grain body{};
    Word face = 0;
};
struct Wheel {
    Piece siblings[4]{}, tools[4]{};
    Word seed = 0, gates = 0, stock[4]{}, clock = 0, loose = 0, consumed = 0;
};
struct Topic {
    Word tool = 4, bath = 4;
};
struct Receipt {
    Word own = 0, others = 0, meal = 0, share = 0, reserve = 0, tone = 4, cue = 4, bath = 4, witnesses = 0, tastes = 0,
         spilled = 0, consumed = 0;
};
enum WheelAction : Word { set_wheel, watch, sample_water, turn_wheel, demonstrate, work };
} // namespace axion::life::press
