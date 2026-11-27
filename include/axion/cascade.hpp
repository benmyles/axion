#pragma once
#include "retina_study.hpp"
#include "apprentice.hpp"

namespace axion::life::retina {
struct Itinerary {
    Count cues = 0;
    Word size = 0;
};
// Only the resident's acquired word contacts connect public sounds to the
// four spectral nerve addresses. A missing referent cannot issue a stroke.
AX_FN Itinerary listen(braid::Narration &lexicon, const braid::Speech &phrase) {
    Itinerary reply{};
    for (Word j = 0; j < phrase.n; ++j) {
        Count wave = phrase.wave[j];
        if (!wave)
            return {};
        Word ref = braid::ident(lexicon, braid::intern(lexicon, wave, false));
        if (ref >= 4 && ref < 8) {
            if (reply.size == 32)
                return {};
            reply.cues |= Count(ref - 4) << (2 * reply.size++);
            continue;
        }
        const braid::Form *chunk = nullptr;
        for (const auto &f : lexicon.forms)
            if (braid::sequence_form(f) && f.pattern == wave) {
                chunk = &f;
                break;
            }
        if (!chunk || !chunk->examples)
            return {};
        Word duration = braid::sequence_length(*chunk);
        if (reply.size + duration > 32)
            return {};
        for (Word t = 0; t < duration; ++t) {
            Word available = (~Word(chunk->pruned) >> (4 * t)) & 15;
            if (!available || (available & (available - 1)))
                return {};
            reply.cues |= Count(__builtin_ctz(available)) << (2 * reply.size++);
        }
    }
    return reply;
}
AX_FN Itinerary append(Itinerary a, Itinerary b) {
    if (!b.size || a.size + b.size > 32)
        return {};
    a.cues |= b.cues << (2 * a.size);
    a.size += b.size;
    return a;
}
struct Terms {
    Count wave[4]{};
    Word pulses[4]{}, width[4]{}, size = 0;
};
struct PublicOrder {
    unsigned char sounds[32]{};
    Word n = 0, learned = 0;
};
// This is the external speaker. The learner receives only the emitted sounds.
AX_FN PublicOrder say(Count cues, Word stages, const Terms &terms) {
    PublicOrder phrase{};
    for (Word at = 0; at < stages;) {
        Word best = terms.size, longest = 0;
        for (Word i = 0; i < terms.size; ++i) {
            Word width = terms.width[i], mask = (1u << (2 * width)) - 1;
            if (width > longest && at + width <= stages && (Word(cues >> (2 * at)) & mask) == terms.pulses[i]) {
                best = i;
                longest = width;
            }
        }
        if (best != terms.size) {
            phrase.sounds[phrase.n++] = 4 + best;
            ++phrase.learned;
            at += longest;
        } else {
            phrase.sounds[phrase.n++] = Word(cues >> (2 * at)) & 3;
            ++at;
        }
    }
    return phrase;
}
AX_FN Word recruited(braid::Narration &m, Count wave, Word length) {
    if (braid::ident(m, braid::intern(m, wave, false)) < 8)
        return braid::none;
    for (Word j = 0; j < braid::forms_max; ++j) {
        auto &f = m.forms[j];
        if (braid::sequence_form(f) && f.pattern == wave)
            return braid::sequence_length(f) == length ? j : braid::none;
        if (!f.tokens) {
            f.tokens = braid::sequence_tag | length;
            f.pattern = wave;
            return j;
        }
    }
    return braid::none;
}
AX_FN Word hear_glints(braid::Form &form, braid::Genes genes, regulation::Source &s, Word visible, Word size,
                       Word mode) {
    Word admitted = (1u << (4 * size)) - 1u, old = form.pruned, conflict = admitted ^ visible;
    Word proposal = braid::rule(genes.plastic >> 8, old, conflict) & admitted;
    Word old_choices = (~old) & admitted;
    bool predicted = true, supported = true;
    for (Word j = 0; j < size; ++j) {
        Word mask = (old_choices >> (4 * j)) & 15;
        predicted &= mask && !(mask & (mask - 1));
        supported &= (old_choices & visible & (15u << (4 * j))) != 0;
    }
    bool feedback = (mode & 255u) != apprentice::quiet;
    Word gate = regulation::discharge(s.program, s.cells, old, conflict, proposal, feedback && predicted,
                                      feedback && predicted && supported);
    form.pruned =
        regulation::apply(old, proposal, (mode & 255u) == apprentice::unrestricted ? 65535u : gate) & admitted;
    if (form.examples < 255)
        ++form.examples;
    return __builtin_popcount(old ^ form.pruned);
}
AX_FN Word grain_count(Word bins) { return (bins & 255) + ((bins >> 8) & 255) + ((bins >> 16) & 255) + (bins >> 24); }
struct Inquiry {
    Word prior = 0, final = 0, offers = 0, paid = 0, stopped = 0, digest = 0, prior_word = 8, actions = 0;
    Word initial_spike = 0, final_spike = 0;
    Banks after{};
    Word removed = 0, deposited = 0;
};
// Only the understood public sounds and the pupil's own prospective discharge
// enter its decision to lift a probe. Food is released separately afterwards.
AX_FN Inquiry reconsider(const Observer &body, Word seed, Word life, Course c, Itinerary heard, Word onset, Word start,
                         Word opportunities, Word budget, Acquired &actual) {
    Inquiry note{};
    note.initial_spike = actual.spikes;
    c.laps = heard.size;
    c.probes = c.query_hand;
    c.purse = actual.spent + budget;
    auto room = bed(seed, life, c, 0);
    Word lamps = braid::assembly(mix(room.key + 91993) % 24);
    auto intact = actual;
    auto &governor = c.query_hand == directed ? actual : intact;
    note.prior = heard.size ? propagate(body, actual.banks, onset, heard.cues, c) : 0;
    Count interest = heard.cues;
    if (c.reversed_question) {
        interest = 0;
        for (Word j = 0; j < heard.size; ++j)
            interest = (interest << 2) | ((heard.cues >> (2 * j)) & 3u);
    }
    if (heard.size) {
        while (note.offers < opportunities && note.paid < budget) {
            Word prediction = propagate(body, governor.banks, onset, interest, c);
            auto decision = iris_discharge(body.iris, onset, prediction, Word(interest) & 3, governor.spikes);
            if (!decision.opened) {
                note.stopped = 1;
                break;
            }
            Word step = start + note.offers, context = (step >> 2) & (c.beds - 1);
            auto tray = bed(seed, life, c, context);
            Word lamp = lamp_color(lamps, context, body.eyes), before = actual.spent;
            auto e =
                sound(body, tray, step, false, c, lamp, matrix(body, c, lamp), mix(tray.key ^ (step * 15791 + 17041)),
                      actual, c.query_hand == directed ? nullptr : &intact);
            Word paid = actual.spent - before;
            note.paid += paid;
            ++note.offers;
            Word seen = paid ? e.light : e.light & ~255u;
            note.digest = mix(note.digest ^ seen ^ decision.volley ^ step);
            if (paid) {
                Word hand = e.light >> 31;
                note.actions = mix(note.actions ^ ((step * 2 + hand) + 0x34d1u));
                Word mark = mix(tray.key + (step >> 2) * 1919 + 263);
                Word source = braid::port(braid::assembly(mark % 24), source_position(mark, step, hand));
                Word target = inlet::destination(tray, source, false);
                note.removed += 1u << (8 * source);
                note.deposited += 1u << (8 * target);
            }
        }
    }
    note.final = heard.size ? propagate(body, actual.banks, onset, heard.cues, c) : 0;
    note.final_spike = actual.spikes;
    note.after = actual.banks;
    return note;
}
struct Spoken {
    braid::Episode event{};
    Word dye = 0, origin = 0, heard = 0, motor = 0, options = 0, forecast = 8, early = 8, silent = 8, topic = 0,
         reply = 0, hand = 0, a = 0, b = 0;
    Count cues = 0;
    Word laps = 0, surface = ~0u, units = 0, learned = 0, heat = 0;
};
struct Delivered {
    Word outlet = 0, word = 8, source = 8, cut = 8;
};
enum Manipulation : Word { open, watch, load, word_sequence, deliver, disconnect, demonstrate, supply_rack };
struct Bench {
    Word seed = 0, life = 0, lamps = 0, count = 0, beds = 2, glimpses = 0, staged = 0, finished = 0, source = 0,
         dye = 0, base = 0;
    Word onset = 0, command = 0, options = 0, chosen = 0, ended = 0, heat = 0, food = 0, settled = 0, bolus = 8;
    Word packed_bed = 0;
    Word rack = 0, rack_initial = 0, rack_remaining = 0, rack_deposits = 0;
    Word grains[4]{16, 16, 16, 16};
    Course course{};
    Itinerary words{};
    braid::Episode event{};
};
struct WordExposure {
    Word light_and_contacts = 0, spikes = 0;
};
struct Request {
    Word op = 0, number = 0, life = 0, beds = 2, bright = 0, channel = paired, energy = 0, mouth = 0;
    Word examples = 0, demonstration = 0, mode = 1, voluntary_view = 0, probes = 0;
    const Surface *surface{};
    WordExposure *exposures{};
    Count route = 0, noun = 0;
    braid::Speech words{};
};
struct Answer {
    Word status = 0, attended = 0, edits = 0, heat = 0, food = 0, settled = 0, supply = 0, paid = 0, bite = 0;
    Word source = 4, dye = 4, lamps = 0, heard = 0, options = 0, aim = 4, outlet = 4, chosen = 0;
    Count cues = 0;
    Word built_length = 0, built_turns = 0, examples = 0, contacts = 0, decoded = 0;
    Word offers = 0, disturbed = 0, relocated = 0, denied = 0, first_source = 4, second_source = 4, hand_source = 4,
         at = 4, grains = 0, marker = 4;
    Word rack_stock = 0, rack_deposits = 0, rack_initial = 0;
    Word available_rack = 0, query_offers = 0, query_paid = 0, initial_prediction = 0, stopped = 0;
    Word last_glints = 0, passage = 0, earlier = 0, spikes = 0, background = 0, first = 0, second = 0;
};
} // namespace axion::life::retina
