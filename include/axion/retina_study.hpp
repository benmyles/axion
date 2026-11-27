#pragma once
#include "inlet.hpp"
#include "fovea.hpp"

namespace axion::life::retina {
enum Stimulus : Word { water, tone };
struct Surface {
    drift::View material{};
    Word side = 0, tile = 0, tick = 0, law = 0, seed = 0;
};
struct Course {
    Word rounds = 16, former = 0, laps = 1, landscape = inlet::channels, ports = 0;
    Word history = 0, channel = paired, stimulus = water, contrast = 0;
    Word beds = 1, fused = 0, rear_laps = 0;
    const Word *surfaces{};
    Word sites = 0, natural_order = 0, turns = 0;
    Word gaze = passive, purse = 0, probes = ambient, harvests = 1, inquiry = 0, query_hand = directed,
         query_budget = 0, reversed_question = 0;
};
struct Observation {
    Word light = 0, pre = 0, post = 0, admitted = 0, before = 0, after = 0;
};
struct Forecast {
    Word contacts = 0, spikes = 0, changed = 0, acquired = 0, witnessed = 0;
    Word resting = 0, opening = 0, onset = 0, options = 0, command = 0, bare = 0, single = 0;
    Word source = 0, target = 0, first = 0, draw = 0, map = 0, maps = 0, lamps = 0, gate_a = 0, gate_b = 0, laps = 0,
         surface = ~0u;
    Count schedule = 0;
    Word spent = 0, harvested = 0;
};
struct Score {
    Word right = 0, blank = 0, once = 0, different = 0, different_right = 0, certain = 0, certain_right = 0;
    Word paired = 0, distinct = 0, edits = 0, perceived = 0, spent = 0, harvested = 0;
};
AX_FN Word rest_light(Word key, Word hue, bool bright) { return bright ? (hue ^ 3u) * 85u : mix(key + 531) & 255u; }
// One peripheral nerve can receive either retinal positions or impulses from
// the four resonators. The downstream cells have no modality or reward input.
AX_FN Word tone_image(Word socket) { return 3u << (2 * socket); }
AX_FN Light input(const Observer &body, Word base, Word hue, Word source, Word dest, bool sound) {
    auto l = project(reflected(base, hue, source, dest), body.eyes);
    if (sound) {
        // Four acoustic nerve addresses are independent of the optics.
        // A temporal transition from sound to sight has separate resting levels.
        Word call = drift::face(body.senses, source);
        l.onset = l.rest ^ tone_image(call);
    }
    return l;
}
AX_FN Word route_room(Word key, Word i, Word beds) {
    if (beds == 1)
        return 0;
    if (i < beds)
        return (braid::port(braid::assembly(mix(key + 5987) % (beds == 2 ? 2u : 24u)), i) + (key >> 8)) & (beds - 1);
    return mix(key + 19759 * i) & (beds - 1);
}
AX_FN Word site_index(Word seed, Word life, Course c, Word pair) {
    return mix(seed ^ mix((life >> 2) * 9739u + pair * 15263u + 8387)) % c.sites;
}
AX_FN drift::Path as_built(const Surface &s) {
    bool bound = s.law & drift::fibres, seep = s.law & drift::seepage;
    Word lower = drift::connections(s.material, bound, seep);
    return s.law & drift::winding ? drift::woven_path(s.material, lower, bound, seep) : drift::Path{lower};
}
AX_FN Course exposure_order(Word seed, Word life, Course c) {
    if (c.natural_order) {
        auto p = drift::Path::recover(c.surfaces[site_index(seed, life, c, 0)]);
        c.laps = p.length;
        c.turns = p.turns;
    }
    return c;
}
AX_FN Word chosen_room(Word key, Word step, Course c, bool twin) {
    Word j = c.beds > 1 && twin ? c.laps - 1 - step : step;
    return c.natural_order ? (c.turns >> j) & 1u : route_room(key, j, c.beds);
}
AX_FN inlet::Layout bed(Word seed, Word life, Course c, Word i) {
    if (!c.sites)
        return inlet::habitat(seed ^ (0xabe17705u * i), life, true, c.landscape);
    auto path = drift::Path::recover(c.surfaces[site_index(seed, life, c, i / 2)]);
    Word key = mix(seed ^ mix(0xc781493du + (life >> 2) * 0x9e3779b9u) ^ (0xabe17705u * i));
    Word held = ((key >> 12) + life) & 3;
    return {key,  i & 1 ? path.upper : path.lower,     braid::order, braid::order,
            held, (held + 1 + mix(key + 891) % 3) & 3, inlet::field};
}
AX_FN Word propagate(const Observer &body, Banks contacts, Word firing, Count cues, Course c) {
    return sequence(firing, contacts, body.genes, body.wiring, c.ports, c.laps, c.beds > 1 && !c.fused,
                    [&](Word i) { return Word(cues >> (2 * i)) & 3; });
}
AX_FN Word lamp_color(Word lamps, Word context, braid::Optics eyes) {
    return braid::pigment(eyes.hues, braid::port(lamps, context));
}
AX_FN Word source_position(Word stamp, Word step, bool alternative) {
    return alternative ? (step + 1 + mix(stamp ^ 0x297b79u) % 3) & 3 : step & 3;
}
AX_FN Light incident(const Observer &body, const inlet::Layout &tray, Word step, bool other_world, Course c,
                     bool alternative = false) {
    Word stamp = mix(tray.key + (step >> 2) * 1919 + 263);
    Word order = braid::assembly(stamp % 24), slot = source_position(stamp, step, alternative);
    Word source = braid::port(order, slot);
    Word shown = source;
    if (c.channel == displaced)
        shown = braid::port(order, (slot + 1 + ((stamp >> 7) % 3)) & 3);
    // A fresh independent permutation keeps the four outgoing impulses in each
    // quartet. Its order carries no systematic exclusion of the real source.
    if (c.channel == independent)
        shown = braid::port(braid::assembly(mix(stamp ^ 0xbc459e21u) % 24), slot);
    Word to = inlet::destination(tray, shown, other_world);
    Word dye = (stamp >> 18) & 3, base = rest_light(stamp, dye, c.contrast);
    auto light = input(body, base, dye, source, to, c.stimulus == tone);
    if (c.channel == dark)
        light.end = light.rest;
    return light;
}
AX_FN Word matrix(const Observer &body, Course c, Word lamp) {
    return c.beds == 1 || c.fused ? 0 : bank(body.genes, lamp);
}
AX_FN Observation receive(const Observer &body, Light light, Word lamp, Word site, Word channel, Banks &contacts,
                          Word &spikes) {
    Word old = contacts.get(site);
    auto e = expose(body.genes, light, body.heading, old, spikes);
    if (channel == closed)
        e = {0, 0, 0, spikes, old};
    contacts.put(site, e.contacts);
    spikes = e.spikes;
    return {
        (lamp << 24) | (light.rest << 16) | (light.onset << 8) | light.end, e.pre, e.post, e.admitted, old, e.contacts};
}
AX_FN Observation glimpse(const Observer &body, const inlet::Layout &tray, Word step, Word lamp, bool other_world,
                          Course c, Banks &contacts, Word &spikes) {
    return receive(body, incident(body, tray, step, other_world, c), lamp, matrix(body, c, lamp), c.channel, contacts,
                   spikes);
}
struct Acquired {
    Banks banks{};
    Word spikes = 0, edits = 0, admitted = 0, witnessed = 0, spent = 0;
};
AX_FN Acquired born_with(const Observer &body, Course c) {
    return c.history ? Acquired{{body.contacts, body.gate_a, body.gate_b}, body.spikes} : Acquired{};
}
AX_FN Observation attend(const Observer &body, Light light, Word lamp, Word site, Course c, Acquired &state, bool buy) {
    auto e = receive(body, light, lamp, site, buy ? c.channel : closed, state.banks, state.spikes);
    state.spent += buy;
    state.edits += __builtin_popcount(e.before ^ e.after);
    state.admitted += e.admitted;
    state.witnessed += bool(e.pre && e.post);
    return e;
}
// Each reach moves one tracer from one of two available sources. The
// comparison uses the two onset images. The second image of the selected
// source first enters the learning circuit after the muscle fires.
AX_FN Observation sound(const Observer &body, const inlet::Layout &tray, Word step, bool twin, Course c, Word lamp,
                        Word site, Word tie, Acquired &actual, Acquired *intact) {
    Light light[2]{incident(body, tray, step, twin, c), incident(body, tray, step, twin, c, true)};
    auto &policy = intact ? *intact : actual;
    auto action = reach(body, light[0], light[1], lamp, policy.banks.get(site), policy.spikes, c.ports, tie);
    bool paid = action.started && policy.spent < c.purse;
    Word hand = action.hand;
    if (intact) {
        attend(body, light[hand], lamp, site, c, *intact, paid);
        if (c.probes == reversed_hands)
            hand ^= 1;
        if (c.probes == random_hand ||
            (c.probes == matched_hand && bool(action.eye[0].onset) == bool(action.eye[1].onset)))
            hand = (tie >> 7) & 1u;
    }
    auto e = attend(body, light[hand], lamp, site, c, actual, paid);
    e.light |= (Word(paid) << 26) | (action.eye[action.hand].volley << 27) | (hand << 31);
    return e;
}
AX_FN Acquired investigate(const Observer &body, Word seed, Word life, bool twin, Course c, Word *trace) {
    auto animal = born_with(body, c), intact = animal;
    auto room = bed(seed, life, c, 0);
    Word pigments = braid::assembly(mix(room.key + 91993) % 24);
    inlet::Layout tray = room;
    for (Word t = 0; t < c.rounds + c.former; ++t) {
        bool past = t < c.former;
        Word step = past ? t : t - c.former, context = (step >> 2) & (c.beds - 1);
        if (!(step & 3))
            tray = bed(past ? seed ^ 0x183911b3u : seed, past ? life & ~3u : life, c, context);
        Word lamp = lamp_color(pigments, context, body.eyes);
        Word tie = mix(tray.key ^ (step * 15791 + 17041));
        auto e = sound(body, tray, step, c.beds == 1 && !past && twin, c, lamp, matrix(body, c, lamp), tie, animal,
                       c.probes == directed ? nullptr : &intact);
        if (trace)
            trace[t] = e.light;
    }
    return animal;
}
AX_FN Acquired attentive(const Observer &body, Word seed, Word life, bool twin, Course c, Word *trace) {
    auto independent = born_with(body, c);
    auto animal = independent;
    const auto original = animal;
    auto room = bed(seed, life, c, 0);
    Word pigments = braid::assembly(mix(room.key + 91993) % 24);
    for (Word epoch = 0; epoch < 2; ++epoch) {
        bool past = epoch == 0;
        Word count = past ? c.former : c.rounds;
        for (Word start = 0; start < count; start += 4) {
            Word slot = (start >> 2) & (c.beds - 1), lamp = lamp_color(pigments, slot, body.eyes),
                 site = matrix(body, c, lamp);
            auto tray = bed(past ? seed ^ 0x183911b3u : seed, past ? life & ~3u : life, c, slot);
            Word size = drift::low(4, count - start), choices = 0, visible = 0;
            if (c.gaze == yoked || c.gaze == visible_yoke) {
                // Keep the pupil's own number of looks on this bed and in this
                // quartet. Only their assignment to physical onsets is changed.
                for (Word j = 0; j < size; ++j) {
                    auto light = incident(body, tray, start + j, c.beds == 1 && !past && twin, c);
                    auto action = orient(body, light.rest, light.onset, lamp, independent.banks.get(site),
                                         independent.spikes, c.ports);
                    bool buy = independent.spent < c.purse && action.opened;
                    attend(body, light, lamp, site, c, independent, buy);
                    choices |= Word(buy) << j;
                    visible |= Word(action.onset != 0) << j;
                }
            }
            Word rotation = mix(tray.key ^ mix(start + epoch * 193 + 0x48e3u));
            for (Word j = 0; j < size; ++j) {
                auto light = incident(body, tray, start + j, c.beds == 1 && !past && twin, c);
                const auto &read = c.gaze == natal_eye ? original : animal;
                auto action = orient(body, light.rest, light.onset, lamp, read.banks.get(site), read.spikes, c.ports);
                bool motor = c.gaze == open_eye || ((c.gaze == voluntary || c.gaze == natal_eye) && action.opened);
                if (c.gaze == yoked)
                    motor = (choices >> ((j + (rotation % size)) % size)) & 1u;
                if (c.gaze == visible_yoke)
                    motor = (choices >> matched_position(j, visible, size, rotation)) & 1u;
                bool buy = motor && animal.spent < c.purse;
                auto e = attend(body, light, lamp, site, c, animal, buy);
                if (trace)
                    trace[(past ? 0 : c.former) + start + j] = e.light | (Word(buy) << 26) | (action.volley << 27);
            }
        }
    }
    return animal;
}
template <bool active>
AX_FN Acquired acquire(const Observer &body, Word seed, Word life, bool twin, Course c, Word *trace = nullptr) {
    if constexpr (active) {
        if (c.probes)
            return investigate(body, seed, life, twin, c, trace);
        if (c.gaze != passive)
            return attentive(body, seed, life, twin, c, trace);
    }
    auto room = bed(seed, life, c, 0);
    Word pigments = braid::assembly(mix(room.key + 91993) % 24);
    Banks contacts = c.history ? Banks{body.contacts, body.gate_a, body.gate_b} : Banks{};
    Word spikes = c.history ? body.spikes : 0, edits = 0, admitted = 0, witnessed = 0;
    inlet::Layout tray = room;
    for (Word t = 0; t < c.rounds + c.former; ++t) {
        bool past = t < c.former;
        Word step = past ? t : t - c.former;
        Word context = (step >> 2) & (c.beds - 1);
        if (!(step & 3))
            tray = bed(past ? seed ^ 0x183911b3u : seed, past ? life & ~3u : life, c, context);
        Word lamp = lamp_color(pigments, context, body.eyes);
        auto e = glimpse(body, tray, step, lamp, c.beds == 1 && !past && twin, c, contacts, spikes);
        if (trace)
            trace[t] = e.light;
        edits += __builtin_popcount(e.before ^ e.after);
        admitted += e.admitted;
        witnessed += e.pre && e.post;
    }
    return {contacts, spikes, edits, admitted, witnessed};
}
AX_FN Count signals(Word key, Word pigments, braid::Optics eyes, Course c, bool twin) {
    Count cues = 0;
    for (Word j = 0; j < c.laps; ++j) {
        Word which = chosen_room(key, j, c, twin);
        cues |= Count(lamp_color(pigments, which, eyes)) << (2 * j);
    }
    return cues;
}
AX_FN Forecast forecast(const Observer &body, Word seed, Word life, bool twin, Course c, const Acquired &m) {
    auto room = bed(seed, life, c, 0);
    Word pigments = braid::assembly(mix(room.key + 91993) % 24);
    auto contacts = m.banks;
    Banks original = c.history ? Banks{body.contacts, body.gate_a, body.gate_b} : Banks{};
    Word src = room.hold, dye = (room.key >> 22) & 3;
    Word base = rest_light(mix(room.key + 411), dye, true);
    auto current = input(body, base, dye, src, src, c.stimulus == tone);
    Word onset = initial(body.genes, current.rest, current.onset, body.heading);
    Count cues = signals(room.key, pigments, body.eyes, c, twin);
    Word opts = propagate(body, contacts, onset, cues, c), draw = mix(room.key ^ 0x597a174bu);
    Word blank = drift::choose(propagate(body, original, onset, cues, c), draw);
    auto single = c;
    single.laps = 1;
    Word one = drift::choose(propagate(body, contacts, onset, cues, single), draw);
    Forecast reply{contacts.hand,
                   m.spikes,
                   m.edits,
                   m.admitted,
                   m.witnessed,
                   current.rest,
                   current.onset,
                   onset,
                   opts,
                   drift::choose(opts, draw),
                   blank,
                   one,
                   src,
                   4,
                   4,
                   draw,
                   0};
    reply.spent = m.spent;
    reply.lamps = pigments;
    reply.gate_a = contacts.a;
    reply.gate_b = contacts.b;
    reply.schedule = cues;
    reply.laps = c.laps;
    if (c.sites)
        reply.surface = site_index(seed, life, c, 0);
    return reply;
}
AX_FN void release(Forecast &reply, const Observer &body, Word seed, Word life, bool twin, Course c) {
    auto room = bed(seed, life, c, 0);
    Word maps = 0;
    for (Word k = 0; k < c.beds; ++k) {
        auto tray = k ? bed(seed, life, c, k) : room;
        for (Word j = 0; j < 4; ++j)
            maps |= inlet::destination(tray, j, c.beds == 1 && twin) << (8 * k + 2 * j);
    }
    Word at = reply.source;
    for (Word j = 0; j < c.laps; ++j) {
        Word k = chosen_room(room.key, j, c, twin);
        at = (maps >> (8 * k + 2 * at)) & 3;
        if (!j)
            reply.first = at;
    }
    reply.target = at;
    reply.maps = maps;
    reply.map = maps & 255u;
    reply.command = drift::face(body.muscles, reply.command);
    reply.bare = drift::face(body.muscles, reply.bare);
    reply.single = drift::face(body.muscles, reply.single);
}
// Subsequent portions travel through the same physical beds. Their inlets,
// marker and public stage order can change. They all read the same experience.
AX_FN Word harvest_again(const Observer &body, const Acquired &trained, Word seed, Word life, bool twin, Course c,
                         const Forecast &first) {
    auto room = bed(seed, life, c, 0);
    Word catches = 0;
    for (Word j = 1; j < c.harvests; ++j) {
        Word key = mix(room.key ^ (j * 29569u + 0x651701u)), src = (room.hold + j) & 3;
        Word hue = (key >> 18) & 3, base = rest_light(key, hue, true);
        auto image = input(body, base, hue, src, src, c.stimulus == tone);
        Word opening = initial(body.genes, image.rest, image.onset, body.heading);
        Count cues = signals(key, first.lamps, body.eyes, c, twin);
        Word predicted = propagate(body, trained.banks, opening, cues, c);
        Word motor = drift::face(body.muscles, drift::choose(predicted, mix(key + 9131)));
        Word at = src;
        for (Word k = 0; k < c.laps; ++k)
            at = (first.maps >> (8 * chosen_room(key, k, c, twin) + 2 * at)) & 3;
        catches += motor == at;
    }
    return catches;
}
AX_FN void accumulate(Score &s, const Forecast &r) {
    bool right = r.command == r.target, certain = r.options && !(r.options & (r.options - 1));
    s.right += right;
    s.blank += r.bare == r.target;
    s.once += r.single == r.target;
    s.different += r.target != r.first;
    s.different_right += right && r.target != r.first;
    s.certain += certain;
    s.certain_right += right && certain;
    s.spent += r.spent;
    s.harvested += r.harvested;
    s.edits += r.changed;
    s.perceived += r.witnessed;
}
} // namespace axion::life::retina
