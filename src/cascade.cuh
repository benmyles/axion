#include "axion/cascade.hpp"

namespace axion::life::retina {
template <bool active>
AX_FN void examine_speech(const braid::Narration *vocabulary, Observer body, const inlet::Questions *script, Word seed,
                          Word cases, Course course, Word hearing, Terms terms, Spoken *record, Inquiry *evidence) {
    Word i = tid();
    if (i >= cases)
        return;
    Word life = i / 2;
    bool twin = i & 1;
    course = exposure_order(seed, life, course);
    auto learned = acquire<active>(body, seed, life, twin, course);
    auto room = bed(seed, life, course, 0);
    Word pigments = braid::assembly(mix(room.key + 91993) % 24), src = room.hold, physical_hue = (room.key >> 22) & 3;
    Word base = rest_light(mix(room.key + 411), physical_hue, true);
    auto stimulus = input(body, base, physical_hue, src, src, course.stimulus == tone);
    Word onset = initial(body.genes, stimulus.rest, stimulus.onset, body.heading);
    auto m = *vocabulary;
    Count public_cues = signals(room.key, pigments, body.eyes, course, twin);
    Itinerary heard{};
    auto public_words = say(public_cues, course.laps, terms);
    for (Word first = 0; first < public_words.n; first += braid::max_phrase) {
        braid::Speech line{};
        line.n = drift::low(braid::max_phrase, public_words.n - first);
        for (Word j = 0; j < line.n; ++j) {
            Word word = public_words.sounds[hearing == 1 ? public_words.n - 1 - first - j : first + j];
            line.wave[j] = hearing == 2 ? 0 : word < 4 ? script->names.wave[4 + word] : terms.wave[word - 4];
        }
        auto partial = listen(m, line);
        if (!partial.size) {
            heard = {};
            break;
        }
        heard = append(heard, partial);
        if (!heard.size)
            break;
    }
    Course understood = course;
    understood.laps = heard.size;
    auto previous = learned;
    Inquiry observation{};
    if constexpr (active)
        if (course.inquiry)
            observation = reconsider(body, seed, life, understood, heard, onset, course.rounds, course.inquiry,
                                     course.query_budget - drift::low(course.query_budget, learned.spent), learned);
    Word opts = active && course.inquiry
                    ? observation.final
                    : (heard.size ? propagate(body, learned.banks, onset, heard.cues, understood) : 0);
    Word draw = mix(room.key ^ 0x597a174bu), motor = drift::face(body.muscles, drift::choose(opts, draw));
    auto frame = drift::forecast_view(base, physical_hue, src, opts, body.muscles, i + 1, false);
    auto question = braid::fill_question(script->predict, script->variable,
                                         script->names.wave[4 + braid::pigment(body.eyes.hues, physical_hue)]);
    m.topic = m.reply = 0;
    braid::experience(m, {});
    Word silent = inlet::hear(m, question, script->names);
    m.topic = m.reply = 0;
    braid::experience(m, braid::through_glass(frame, body.eyes));
    Word before = inlet::hear(m, question, script->names), early = inlet::hear(m, script->actual, script->names);
    if constexpr (active)
        if (course.inquiry) {
            auto prior = *vocabulary;
            auto sight = drift::forecast_view(base, physical_hue, src, observation.prior, body.muscles, i + 1, false);
            prior.topic = prior.reply = 0;
            braid::experience(prior, braid::through_glass(sight, body.eyes));
            observation.prior_word = inlet::hear(prior, question, script->names);
        }
    record[i] = {frame,
                 physical_hue,
                 braid::pigment(body.eyes.places, src),
                 heard.size,
                 braid::pigment(body.eyes.places, motor),
                 opts,
                 before,
                 early,
                 silent,
                 m.topic,
                 m.reply,
                 previous.banks.hand,
                 previous.banks.a,
                 previous.banks.b,
                 heard.cues,
                 course.laps,
                 course.sites ? site_index(seed, life, course, 0) : ~0u,
                 public_words.n,
                 public_words.learned,
                 previous.spent};
    if constexpr (active)
        if (evidence)
            evidence[i] = observation;
}
KERNEL void cascade_before(const braid::Narration *vocabulary, Observer body, const inlet::Questions *script, Word seed,
                           Word cases, Course course, Word hearing, Terms terms, Spoken *record, Inquiry *evidence) {
    examine_speech<false>(vocabulary, body, script, seed, cases, course, hearing, terms, record, evidence);
}
KERNEL void cascade_active_before(const braid::Narration *vocabulary, Observer body, const inlet::Questions *script,
                                  Word seed, Word cases, Course course, Word hearing, Terms terms, Spoken *record,
                                  Inquiry *evidence) {
    examine_speech<true>(vocabulary, body, script, seed, cases, course, hearing, terms, record, evidence);
}
KERNEL void cascade_after(const braid::Narration *vocabulary, Observer body, const inlet::Questions *script, Word seed,
                          Word cases, Course course, const Spoken *record, Delivered *heard) {
    Word i = tid();
    if (i >= 2 * cases)
        return;
    Word case_id = i / 2, life = case_id / 2;
    bool twin = case_id & 1;
    course = exposure_order(seed, life, course);
    auto room = bed(seed, life, course, 0);
    Forecast f{};
    f.source = room.hold;
    release(f, body, seed, life, twin, course);
    Word exit = (f.target + (i & 1)) & 3;
    const auto &r = record[case_id];
    auto frame = r.event;
    Word base = rest_light(mix(room.key + 411), r.dye, true);
    braid::expose(frame, braid::result, drift::overlay(base, exit, r.dye), 15);
    auto m = *vocabulary;
    braid::experience(m, braid::through_glass(frame, body.eyes));
    m.topic = 0;
    m.reply = r.reply;
    Word cut = inlet::hear(m, script->actual, script->names);
    m.topic = r.topic;
    m.reply = r.reply;
    Word outcome = inlet::hear(m, script->actual, script->names);
    auto q = braid::fill_question(script->source, script->source_variable,
                                  script->names.wave[4 + braid::pigment(body.eyes.hues, r.dye)]);
    heard[i] = {braid::pigment(body.eyes.places, exit), outcome, inlet::hear(m, q, script->names), cut};
}
KERNEL void cascade_contact(Bench *bench, braid::Saved *hand, braid::Narration *words, Tissue *tissue, Observer body,
                            Request q, regulation::Source *auditory, Answer *receipt) {
    auto &b = *bench;
    auto &m = *words;
    auto &h = hand->mind;
    Answer r{};
    Banks weights{h.volley >> 16, tissue->gate_a, tissue->gate_b};
    Word spikes = tissue->spike;
    if (q.op == open) {
        b = {};
        b.seed = q.number;
        b.life = q.life;
        b.beds = q.beds;
        b.course.beds = q.beds;
        b.course.contrast = q.bright;
        b.course.ports = drift::live;
        b.course.landscape = inlet::channels;
        if (q.surface) {
            b.packed_bed = as_built(*q.surface).shape();
            b.course.surfaces = &b.packed_bed;
            b.course.sites = 1;
        }
        auto room = bed(b.seed, b.life, b.course, 0);
        b.lamps = braid::assembly(mix(room.key + 91993) % 24);
        b.source = room.hold;
        braid::experience(m, {});
        m.topic = m.reply = 0;
    } else if (q.op == supply_rack && !b.staged && !b.finished) {
        b.rack = q.number;
        b.rack_initial = b.rack_remaining = q.number * 2;
        b.rack_deposits = 0;
    } else if (q.op == watch && !b.staged) {
        for (Word j = 0; j < q.number && r.paid + 1 < q.energy; ++j) {
            Word step = b.glimpses++, context = (step >> 2) & (b.beds - 1);
            auto tray = bed(b.seed, b.life, b.course, context);
            auto c = b.course;
            c.channel = q.channel;
            Word lamp = lamp_color(b.lamps, context, body.eyes);
            auto light = incident(body, tray, step, false, c);
            Word site = matrix(body, c, lamp);
            ++r.offers;
            if (q.probes) {
                Word stamp = mix(tray.key + (step >> 2) * 1919 + 263), order = braid::assembly(stamp % 24);
                Word a = braid::port(order, step & 3),
                     other = braid::port(order, (step + 1 + mix(stamp ^ 0x297b79u) % 3) & 3);
                if (!b.grains[a] || !b.grains[other]) {
                    ++r.denied;
                    continue;
                }
                auto alternative = incident(body, tray, step, false, c, true);
                auto action = reach(body, light, alternative, lamp, weights.get(site), spikes, c.ports,
                                    mix(tray.key ^ (step * 15791 + 17041)));
                if (!action.started)
                    continue;
                Word source = action.hand ? other : a, ended = inlet::destination(tray, source, false);
                --b.grains[source];
                ++b.grains[ended];
                ++r.disturbed;
                r.relocated += ended != source;
                r.first_source = braid::pigment(body.eyes.places, a);
                r.second_source = braid::pigment(body.eyes.places, other);
                r.hand_source = braid::pigment(body.eyes.places, source);
                r.at = braid::pigment(body.eyes.places, ended);
                if (action.hand)
                    light = alternative;
                Word pigment = (stamp >> 18) & 3, base = rest_light(stamp, pigment, c.contrast);
                r.marker = braid::pigment(body.eyes.hues, pigment);
                auto sight = seen_probe(base, pigment, source, action.eye[action.hand].prediction, body.muscles,
                                        body.eyes, b.glimpses, c.channel, light);
                braid::experience(m, sight);
                m.topic = m.reply = 0;
            } else if (q.voluntary_view &&
                       !orient(body, light.rest, light.onset, lamp, weights.get(site), spikes, c.ports).opened)
                continue;
            auto e = receive(body, light, lamp, site, c.channel, weights, spikes);
            ++r.paid;
            ++r.attended;
            r.edits += __builtin_popcount(e.before ^ e.after);
        }
        b.heat += r.paid;
    } else if (q.op == load && !b.staged && !b.finished) {
        Word name = braid::ident(m, braid::intern(m, q.noun, false));
        if (name < 4 || name > 7)
            r.status = 1;
        else {
            b.dye = braid::pigment(braid::inverse_view(body.eyes.hues), name - 4);
            auto room = bed(b.seed, b.life, b.course, 0);
            b.base = rest_light(mix(room.key + 411), b.dye, true);
            auto l = input(body, b.base, b.dye, b.source, b.source, false);
            b.onset = initial(body.genes, l.rest, l.onset, body.heading);
            b.staged = 1;
            b.event = drift::forecast_view(b.base, b.dye, b.source, 0, body.muscles, b.glimpses + 1, false);
            braid::experience(m, braid::through_glass(b.event, body.eyes));
            m.topic = m.reply = 0;
        }
    } else if (q.op == word_sequence && b.staged && !b.finished) {
        auto line = listen(m, q.words);
        if (q.number)
            line = append(b.words, line);
        if (!line.size)
            r.status = 1;
        else if (q.energy <= 1)
            r.status = 2;
        else {
            b.words = line;
            auto c = b.course;
            c.laps = line.size;
            if (b.rack) {
                Acquired learned{weights, spikes};
                auto checked =
                    reconsider(body, b.seed, b.life, c, line, b.onset, b.glimpses, b.rack, q.energy - 2, learned);
                r.initial_prediction = checked.prior;
                r.stopped = checked.stopped;
                r.query_offers = checked.offers;
                r.query_paid = checked.paid;
                b.options = checked.final;
                b.glimpses += checked.offers;
                b.rack -= checked.offers;
                b.rack_remaining -= checked.paid;
                b.rack_deposits += checked.deposited;
                b.heat += checked.paid;
                r.paid += checked.paid;
                r.edits += learned.edits;
                weights = learned.banks;
                spikes = learned.spikes;
            } else
                b.options = propagate(body, weights, b.onset, line.cues, c);
            auto room = bed(b.seed, b.life, b.course, 0);
            b.command = drift::choose(b.options, mix(room.key ^ 0x597a174bu));
            b.chosen = 1;
            ++b.heat;
            ++r.paid;
            b.event = drift::forecast_view(b.base, b.dye, b.source, b.options, body.muscles, b.glimpses + 1, false);
            braid::experience(m, braid::through_glass(b.event, body.eyes));
            m.topic = m.reply = 0;
        }
    } else if (q.op == deliver && b.staged && !b.finished) {
        Word at = b.source;
        if (!q.number && b.course.sites) {
            auto path = drift::Path::recover(b.packed_bed);
            q.number = path.length;
            q.route = 0;
            for (Word j = 0; j < q.number; ++j)
                q.route |= Count((path.turns >> j) & 1) << (2 * j);
        }
        for (Word j = 0; j < q.number; ++j) {
            Word room = (q.route >> (2 * j)) & 3;
            at = inlet::destination(bed(b.seed, b.life, b.course, room), at, false);
        }
        b.ended = at;
        b.finished = 1;
        b.food = b.chosen && drift::face(body.muscles, b.command) == at
                     ? drift::low(b.bolus, drift::low(q.mouth, 65535u - q.energy))
                     : 0;
        b.settled = b.bolus - b.food;
        r.bite = b.food;
        braid::expose(b.event, braid::result, drift::overlay(b.base, at, b.dye), 15);
        braid::experience(m, braid::through_glass(b.event, body.eyes));
    } else if (q.op == demonstrate && !b.staged && !b.finished) {
        if (!(h.genes.arbor & (1u << 14)) || !q.examples || q.energy < 2) {
            r.status = 2;
        } else {
            Word slot = recruited(m, q.noun, q.number);
            if (slot == braid::none) {
                r.status = 3;
            } else {
                auto &f = m.forms[slot];
                f.tokens = braid::sequence_tag | q.number;
                f.pruned &= (1u << (4 * q.number)) - 1;
                r.earlier = f.pruned;
                r.spikes = auditory->cells.spike;
                r.background = auditory->cells.background;
                r.first = auditory->program.first;
                r.second = auditory->program.second;
                for (Word j = 0; j < q.examples && r.paid + 1 < q.energy; ++j) {
                    Word stamp = mix(b.seed + (b.count++) * 14951 + 0x1317dbu), visible = 0, place = stamp & 3;
                    for (Word k = 0; k < q.number; ++k) {
                        Word gate = (q.route >> (2 * k)) & 3;
                        place = inlet::destination(bed(b.seed, b.life, b.course, gate), place, false);
                        // Two independently glowing posts have the same brightness.
                        // Only the sets of retinal colors and the public sound
                        // cross into the resident's lexical tissue.
                        Word real = braid::port(b.lamps, gate), draw = mix(stamp + 191 * k);
                        if (q.demonstration == 3)
                            real = (draw >> 8) & 3;
                        Word rival = (real + 1 + (q.demonstration == 1 ? 0u : (draw % 3))) & 3;
                        Word colors = 1u << braid::pigment(body.eyes.hues, real);
                        if (q.demonstration != 2)
                            colors |= 1u << braid::pigment(body.eyes.hues, rival);
                        visible |= colors << (4 * k);
                    }
                    ++r.examples;
                    ++r.paid;
                    r.last_glints = visible;
                    r.passage = place;
                    r.edits += hear_glints(f, h.genes, *auditory, visible, q.number, q.mode);
                    q.exposures[j] = {visible | (Word(f.pruned) << 16), auditory->cells.spike};
                }
                b.heat += r.paid;
                r.contacts = f.pruned;
                braid::Speech word{};
                word.wave[0] = q.noun;
                word.n = 1;
                r.decoded = listen(m, word).size;
            }
        }
    } else if (q.op == disconnect && !b.staged) {
        b.course.fused = q.number;
    } else
        r.status = 2;
    h.volley = (h.volley & 65535u) | (weights.hand << 16);
    tissue->spike = spikes;
    tissue->gate_a = weights.a;
    tissue->gate_b = weights.b;
    r.available_rack = b.rack;
    r.rack_stock = b.rack_remaining;
    r.rack_initial = b.rack_initial;
    r.rack_deposits = b.rack_deposits;
    r.grains = b.grains[0] + b.grains[1] + b.grains[2] + b.grains[3];
    r.heat = b.heat;
    r.food = b.food;
    r.settled = b.settled;
    r.supply = b.finished ? 0 : b.bolus;
    r.source = braid::pigment(body.eyes.places, b.source);
    for (Word j = 0; j < b.beds; ++j)
        r.lamps |= lamp_color(b.lamps, j, body.eyes) << (2 * j);
    if (b.course.sites) {
        auto path = drift::Path::recover(b.packed_bed);
        r.built_length = path.length;
        r.built_turns = path.turns;
    }
    r.heard = b.words.size;
    r.cues = b.words.cues;
    r.options = b.options;
    r.chosen = b.chosen;
    if (b.staged)
        r.dye = braid::pigment(body.eyes.hues, b.dye);
    if (b.chosen)
        r.aim = braid::pigment(body.eyes.places, drift::face(body.muscles, b.command));
    if (b.finished)
        r.outlet = braid::pigment(body.eyes.places, b.ended);
    *receipt = r;
}
} // namespace axion::life::retina
