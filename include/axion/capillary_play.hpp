#pragma once
#include "capillary_manual.hpp"
#include "context_exposure.hpp"
#include "catalysis.hpp"
#include "clarification.hpp"

namespace axion::life::capillary {
struct Visit {
    Word previous = 0, current = 0, first = absent, last = absent, optical_frame = 16;
};
struct Workshop {
    Work layout{};
    Word ready = 0, material = 0, current = 0, source = 0, pad = 0, shutters = 3, net = 0, seal = 0, feeder_stock = 64,
         shared_food = 0,second_seal=0,second_row=0,second_rock=0;
    Word movable[6]{6, 6, 6, 6, 6, 6}, initial[6]{};
    Word drops = 32, settled = 0, eaten = 0, outlet[4]{}, coating[4]{}, rested[2]{};
    utterance::Trail pending[2]{}, handled[2]{};
    Visit seen[2]{};
    Model received[2]{};
    Word advised[2]{}, voluntary[2]{};
    Word ears=3,stocked=0,goal=0,witness=0,auditory_seed=0,foot[2]{},channel_noise=0,questions=0,reply_mode=0,alternate_light=4;
};
AX_FN Evidence experience(const Workshop &bench, Word who, const Mind &pupil) {
    if (!bench.ready)
        return witnessed(pupil);
    auto note = bench.seen[who];
    auto w = bench.layout;
    w.bed = assemble(bench.layout, note.previous);
    w.feed = source(bench.layout, note.previous);
    Word mobile = note.optical_frame < 16 ? note.optical_frame & 3 : bench.source;
    Word bound = note.optical_frame < 16 ? note.optical_frame >> 2 : bench.pad;
    return {w, note.previous ^ note.current, mobile, bound, note.first, note.last};
}
AX_FN Word occupied(const Work &w, Word material) {
    if (material >= 4)
        return count(w.feed);
    return count(material == 0 ? w.bed.wick : material == 1 ? w.bed.wet : material == 2 ? w.bed.sill : w.bed.paint);
}
AX_FN void setup(Workshop &t, Work w, Word mineral, Word from, Word pad) {
    t.layout = w;
    t.material = mineral;
    t.ready = 1;
    t.current = 0;
    t.source = from;
    t.pad = pad;
    t.shutters = t.ears = 3;t.stocked=0;
    t.net = t.seal = t.second_seal = 0;
    t.feeder_stock = 64;
    t.shared_food = 0;
    t.drops = 32;
    t.eaten = t.settled = t.rested[0] = t.rested[1] = 0;
    for (Word j = 0; j < 4; ++j)
        t.outlet[j] = t.coating[j] = 0;
    for (Word j = 0; j < 2; ++j) {
        t.seen[j] = {};
        t.voluntary[j] = 0;
        t.pending[j] = {};
        t.handled[j] = {};
    }
    for (Word j = 0; j < 6; ++j) {
        t.movable[j] = portions(w.stock, substance(j));
        t.initial[j] = t.movable[j] + occupied(w, j);
    }
}
AX_FN Work current_work(const Workshop &t) {
    auto s = t.layout;
    s.bed = assemble(s, t.current);
    s.feed = source(t.layout, t.current);
    s.stock = 0;
    for (Word j = 0; j < 5; ++j)
        s.stock |= t.movable[j] << (4 * j);
    return s;
}
AX_FN bool place(Workshop &t, Word new_position) {
    if (new_position >= (1u << t.layout.n))
        return false;
    auto s = current_work(t);
    auto r = perform(s, order(s, new_position ^ t.current));
    if (r.denied)
        return false;
    for (Word j = 0; j < 5; ++j)
        t.movable[j] = portions(r.supply, j);
    t.current ^= r.position;
    return true;
}
AX_FN Junction sealed_port(const Workshop &t,Word port,Word row){
    Word state=port?t.second_seal:t.seal;
    return {row,port?t.second_rock:t.material,state==2?t.source:t.pad,state==2?t.pad:t.source};
}
AX_FN Word press_port(Workshop &t,Word port,Word row){
    auto j=sealed_port(t,port,row);
    auto reaction=cycle(j.base,j.coating,j.carrier,j.row);
    Word &state=port?t.second_seal:t.seal;
    if(reaction.flow)state=3-state;
    return reaction.flow;
}
AX_FN bool conserved(const Workshop &t) {
    if (!t.ready)
        return true;
    if (t.feeder_stock + t.shared_food != 64 || t.seal > 2 || t.second_seal>2 || (t.second_seal&&!t.seal) || t.drops + t.settled + t.eaten + Word(t.seal != 0)+Word(t.second_seal!=0) != 32 ||
        t.rested[0] + t.rested[1] != t.settled)
        return false;
    Word emitted = 0, deposits = t.eaten + t.settled;
    for (Word k = 0; k < 4; ++k) {
        emitted += t.outlet[k];
        if (t.outlet[k] + t.coating[k] != deposits * (Word(k == t.source) + Word(k == t.pad)))
            return false;
    }
    if (emitted != deposits)
        return false;
    auto w = t.layout;
    w.bed = assemble(t.layout, t.current);
    w.feed = source(t.layout, t.current);
    for (Word k = 0; k < 5; ++k)
        if (t.movable[k] + occupied(w, k) != t.initial[k])
            return false;
    return true;
}
enum Control : Word {
    inspect_cell,
    set_cell,
    set_rig,
    move_handles,
    pulse,
    experiment,
    design_work,
    answer_question,
    send_answer,
    acquire_name,
    acquire_form,
    open_eye,
    set_net,
    recall_field,
    new_sample,
    follow_words,
    caption_grips,
    set_context,
    change_mineral,
    probe_cell,
    recover_drops,
    seal_cell,
    signal_peer, attach_second, prime_pair, signal_quadrant, private_feeder, acoustic_shutter, teach_resonator, teach_heading, set_acoustic_link
};
struct Request {
    Word action = inspect_cell, who = 0, a = 0, b = 0, c = 0, d = 0, e = 0, seed = 0, amount[2]{};
    SignalVoice voices[2]{};Count cochlea[2]{};
    Word compound[2]{};
    Tissue appetitive[2]{};
    utterance::Waves question{}, statement{};
};
struct Reply {
    utterance::Waves words{};
    Word status = 0, heat[2]{}, gained = 0, spikes = 0, edits = 0, terminal = 0, cause = 0, possible = 0, intended = 0,
         ready = 0, current = 0, prior = absent, result = absent, change = 0, incoming = 0, pad = 0, outgoing = 0,
         stock = 0, settled = 0, eaten = 0, observed = 0, considered = 0, accepted = 0, physical = 0, narrowed = 0,
         saved = 0;
    Word adaptive = 0, rechecks = 0, written = 0, threshold = 0, contact = 0, new_spike[2]{}, stimulus = absent;
    Word sealed = 0, geology = 0, source_before = 0, bound_before = 0, pump_row = 0;
    SignalVoice voices[2]{};Clarification exchange{};Word delivered=0,voice_changes=0,voice_action=0,sound_state=4,listener_state=4;
    Word compound[2]{};
    Tissue appetitive[2]{};
    Word share = 0, feeder_stock = 0, marked = 0, lamp = 0, feeder = 0, erased_choice = 0, opposite_choice = 0;
    Word missing_first=0,missing_second=0,second_seal=0,crop=0;
    History history{};
    Model belief{};
    Tissue tissue{};
};
} // namespace axion::life::capillary
