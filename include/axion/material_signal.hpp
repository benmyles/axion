#pragma once
#include "catalysis.hpp"

namespace axion::life::capillary {
struct Signalling {
    Word lessons = 0, replicas = 4, tests = 16, feedback = 0, inert = 0, seed = 0, first = 0, holdout = 0, retained = 0,
         learners = 3, tutor = 0;
};
enum SignalMetric : Word {
    before_correct,
    after_correct,
    initially_both,
    finally_both,
    initial_mastery,
    final_mastery,
    acquired_mastery,
    lost_mastery,
    reliable_left,
    reliable_right,
    erased_correct,
    inverse_correct,
    left_marks,
    right_marks,
    left_code,
    right_code,
    deposits,
    initial_meals,
    final_meals,
    training_food,
    training_cost,
    sender_changes,
    receiver_changes,
    signal_metrics
};
struct SignalTransfer {
    Word cue, write, light, choice, erased, inverse, food, cost;
};
struct SignalGrade {
    Word total[signal_metrics]{};
};
struct Junction {
    Word row, base, coating, carrier;
};
struct SignalMemory {
    Byte contacts = 0;
    Body body{};
};
AX_FN Word signal_contact(Word light, Word move) {
    // The two lamps and two proprioceptive motor volleys have distinct axons.
    return (1u << light) | (4u << move);
}
// Four optic/motor pairs yield at most four distinct addresses. A missing
// receptor can merge two pairs. Preserve those aliases when packing a synapse.
AX_FN Word sign_shift(Word senses, Word light, Word move) {
    return 2 * ((light * bool(senses & 3)) | (2 * move * bool(senses & 12)));
}
AX_FN Byte pack_signs(Tissue t, Word gene) {
    Word senses = channels(gene), word = 0;
    for (Word i = 0; i < 4; ++i) {
        Word light = i & 1, move = i >> 1;
        Count site = bit(signal_contact(light, move) & senses);
        Word sign = Word(bool(t.on & site)) | (Word(bool(t.off & site)) << 1);
        word |= sign << sign_shift(senses, light, move);
    }
    return Byte(word);
}
AX_FN Tissue expand_signs(Byte word, Word gene) {
    Tissue t{};
    Word senses = channels(gene);
    for (Word i = 0; i < 4; ++i) {
        Word light = i & 1, move = i >> 1, sign = word >> sign_shift(senses, light, move);
        Count site = bit(signal_contact(light, move) & senses);
        if (sign & 1)
            t.on |= site;
        if (sign & 2)
            t.off |= site;
    }
    return t;
}
AX_FN SignalMemory connect_signal(Tissue t, Body b) { return {pack_signs(t, b.dendrites), b}; }
AX_FN Tissue signal_tissue(const SignalMemory &s) { return expand_signs(s.contacts, s.body.dendrites); }
AX_FN int appetitive_value(Word sign,Word motor){
    return (1+int((motor>>5)&3))*bool(sign&1)-(1+int((motor>>11)&3))*bool(sign&2);
}
AX_FN Word effector_choice(Body &b,Word afference,const int (&value)[2],bool known,Word coin,Word motor){
    Word prior = b.cells.spike, positive = Word(value[0] > 0) | (Word(value[1] > 0) << 1);
    Word gate = regulation::discharge(b.growth.program, b.cells, prior, afference, positive, known,
                                      positive != 0);
    Word spontaneous = __builtin_parity(gate & motor) ^ (coin & 1);
    // Genomes differ in exploratory drive, willingness to act and sensory gating.
    int margin = int((motor >> 19) & 3);
    if (!b.limbs)
        return 0;
    if (((coin >> 8) & 255) < ((motor >> 24) & 31))
        return spontaneous;
    return value[1] > value[0] + margin ? 1u : value[0] > value[1] + margin ? 0u : spontaneous;
}
AX_FN Word reach(SignalMemory &a,Word light,Word coin){
    auto &b=a.body;Word senses=channels(b.dendrites),optical=b.eyes?light:0u;
    int value[2]{};
    for(Word move=0;move<2;++move)value[move]=appetitive_value(a.contacts>>sign_shift(senses,optical,move),b.motor);
    return effector_choice(b,1u<<optical,value,bool(a.contacts),coin,b.motor);
}
AX_FN Word digest_signal(SignalMemory &a, Word input, Word action, Word meal) {
    Word shift = sign_shift(channels(a.body.dendrites), a.body.eyes ? input : 0u, action);
    Word spikes = 0, old = (a.contacts >> shift) & 3;
    Word next = admitted_sign(a.body, old, meal ? 1u : 2u, spikes, 1, nullptr, 0);
    a.contacts ^= Byte((old ^ next) << shift);
    return __builtin_popcount(old ^ next);
}
AX_FN Word outlet(Junction j, bool written, bool inert) {
    if (inert)
        return surge(j.row, j.base);
    if (written) {
        auto mark = cycle(j.base, j.coating, j.carrier, j.row);
        j.coating = mark.coating;
        j.carrier = mark.carrier;
    }
    return cycle(j.base, j.coating, j.carrier, j.row).flow;
}
struct Relay {
    Word cue = 0, write = 0, light = 0, choice = 0, erased = 0, inverse = 0, food = 0, cost = 0;
};
AX_FN Relay encounter_signal(SignalMemory &sender, SignalMemory &receiver, Junction j, Word cue, Word noise, bool inert,
                             bool controls) {
    Relay r{};
    r.cue = cue;
    r.write = reach(sender, cue, mix(noise + 17041));
    r.light = outlet(j, r.write, inert);
    Word muscle = mix(noise + 11677);
    if (controls) {
        // Identical receiver soma and exploration in the physical alternatives.
        auto bare = receiver, reversed = receiver;
        r.erased = reach(bare, outlet(j, false, inert), muscle);
        r.inverse = reach(reversed, outlet(j, !r.write, inert), muscle);
    }
    r.choice = reach(receiver, r.light, muscle);
    Word feeders[2]{};
    feeders[cue] = 2;
    Word harvest = feeders[r.choice];
    feeders[r.choice] -= harvest;
    r.food = harvest / 2; // one portion to each body when the receiving dock is stocked
    r.cost = 2 + r.write;
    return r;
}
} // namespace axion::life::capillary
