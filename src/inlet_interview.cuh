#include "axion/inlet.hpp"

namespace axion::life::inlet {
KERNEL void inlet_contact(Instrument *device, braid::Saved *body, braid::Narration *speech, regulation::Source *cortex,
                          Gesture act, Receipt *answer) {
    auto &b = *device;
    auto &m = body->mind;
    auto &words = *speech;
    auto &organ = *cortex;
    Receipt r{};
    Word purse = act.energy, wiring = drift::wiring(m.genes, act.cmd == open_table ? act.ports : b.ports);
    Word axes = (m.genes.muscle >> 24) & 7, sensory = (m.genes.arbor >> 25) & 7;
    auto eyes = braid::eye_wiring(braid::individual_eyes, m.genes, act.heading);
    auto source = [&](Word row) { return inlet_port(sensory, row); };
    auto swallow = [&](bool hit) {
        Word volume = drift::low(b.supply, 4u),
             portion = hit ? drift::low(volume, drift::low(act.mouth, 65535u - purse)) : 0;
        b.supply -= volume;
        b.settled += volume - portion;
        b.captured += portion;
        purse += portion;
        r.eaten += portion;
        return portion;
    };
    auto learn = [&](Word row, Word cmd, Word bite, Word options) {
        if (m.genes.arbor & (1u << 12)) {
            Word next = drift::strength(m.volley >> 16, row, cmd, bite != 0, m.genes.plastic, organ, true,
                                        b.ports & drift::regrowth, options);
            m.volley = (m.volley & 65535u) | (next << 16);
        }
    };
    r.before = m.volley >> 16;
    if (act.cmd == open_table) {
        b = {};
        b.seed = act.value;
        b.ports = act.ports;
        b.twin = act.twin;
        b.supply = 4096;
        b.room = habitat(b.seed, act.life, b.ports & drift::seepage, act.landscape);
        braid::experience(words, {});
        words.topic = words.reply = 0;
    } else if (act.cmd == quiet_hand && !b.pending) {
        m.volley &= 65535u;
        organ.cells.spike = 0;
    } else if (act.cmd == handle && !b.pending && !b.finished) {
        for (Word j = 0; j < act.value && b.supply && purse > 1; ++j) {
            Word row = practice_row(b.room.key, b.room.hold, b.practiced);
            Word opt = drift::openings(m.volley >> 16, row, wiring, b.ports);
            Word cmd = drift::choose(opt, practice_draw(b.room.key, b.practiced));
            Word dest = destination(b.room, row, b.twin);
            --purse;
            ++b.heat;
            ++r.paid;
            Word bite = swallow(cmd == dest);
            learn(row, cmd, bite, opt);
            b.tried |= 1u << row;
            ++b.practiced;
            Word hue = mix(b.seed + b.practiced * 67u) & 3;
            auto seen = drift::sighting(((hue + 1) & 3) * 85u, hue, source(row), drift::face(axes, dest), opt, axes,
                                        act.stamp + b.practiced, eyes);
            braid::experience(words, seen);
        }
    } else if (act.cmd == stage && !b.pending && !b.finished && purse > 1 && b.supply) {
        Word ref = act.noun ? braid::ident(words, braid::intern(words, act.noun, false))
                            : 4u + braid::pigment(eyes.hues, b.seed & 3);
        if (ref < 4 || ref >= 8) {
            r.status = 1;
        } else {
            b.dye = braid::pigment(braid::inverse_view(eyes.hues), ref - 4);
            b.ground = ((b.dye + 1) & 3) * 85u;
            b.source = source(b.room.hold);
            b.options = drift::openings(m.volley >> 16, b.room.hold, wiring, b.ports);
            Word draw = mix(b.room.key ^ 0xdb713f81u);
            b.choice = drift::choose(b.options, draw);
            b.local = drift::choose(drift::openings(m.volley >> 16, b.room.hold, wiring, 0), draw);
            --purse;
            ++b.heat;
            ++r.paid;
            b.pending = 1;
            b.opening =
                drift::forecast_view(b.ground, b.dye, b.source, b.options, axes, act.stamp + b.practiced + 1, false);
            braid::experience(words, braid::through_glass(b.opening, eyes));
        }
    } else if (act.cmd == turn && b.pending) {
        // Move the physical outlet couplings one position after commitment.
        Word outputs = 0;
        for (Word j = 0; j < 4; ++j)
            outputs |= ((braid::port(b.room.outputs, j) + 1) & 3) << (2 * j);
        b.room.outputs = outputs;
        ++b.moved;
    } else if (act.cmd == pour && b.pending) {
        Word dest = destination(b.room, b.room.hold, b.twin);
        Word bite = swallow(dest == b.choice);
        learn(b.room.hold, b.choice, bite, b.options);
        Word actual = drift::face(axes, dest);
        braid::expose(b.opening, braid::result, drift::overlay(b.ground, actual, b.dye), 15);
        braid::experience(words, braid::through_glass(b.opening, eyes));
        r.outlet = braid::pigment(eyes.places, actual);
        b.pending = 0;
        b.finished = 1;
    } else
        r.status = 2;
    r.practiced = b.practiced;
    r.tried = b.tried;
    r.supply = b.supply;
    r.captured = b.captured;
    r.settled = b.settled;
    r.heat = b.heat;
    r.pending = b.pending;
    r.finished = b.finished;
    r.options = b.options;
    r.after = m.volley >> 16;
    r.moved = b.moved;
    r.reach = braid::pigment(eyes.places, drift::face(axes, b.choice));
    r.local = braid::pigment(eyes.places, drift::face(axes, b.local));
    r.inlet = braid::pigment(eyes.places, source(b.room.hold));
    if (b.pending || b.finished)
        r.marker = braid::pigment(eyes.hues, b.dye);
    *answer = r;
}
AX_FN Word hear(braid::Narration &m, const braid::Speech &question, const braid::Speech &names) {
    auto answer = braid::question(m, question, false);
    if (answer.answered)
        for (Word i = 0; i < 8; ++i)
            if (answer.words.wave[0] == names.wave[i])
                return i;
    return 8;
}
KERNEL void inlet_tuition(braid::Saved *operator_state, braid::Narration *voice, apprentice::Teachers *organs,
                          braid::Course lesson, Word mode, Gesture body, Tuition *out) {
    auto &hand = operator_state->mind;
    auto &m = *voice;
    auto motor = organs->motor;
    bool separate = organs->voice.band != ~0u;
    braid::ContactPractice tutor{separate ? organs->voice : motor, mode, 0, 1, nullptr};
    Tuition r{};
    auto eyes = braid::eye_wiring(braid::individual_eyes, hand.genes, body.heading);
    Word wiring = drift::wiring(hand.genes, body.ports), senses = (hand.genes.arbor >> 25) & 7,
         axes = (hand.genes.muscle >> 24) & 7;
    Word purse = body.energy;
    for (Word i = 0; i < lesson.reps && purse > 1; ++i) {
        Word k = mix(lesson.seed + i * 0x9137d5u);
        auto room = habitat(lesson.seed, i, true, mixed);
        Word old = hand.volley >> 16;
        Word opt = 0, command = 0, source = 0, dest = 0, middle = 0;
        for (Word t = 0; t <= body.value && purse > 1; ++t) {
            bool last = t == body.value;
            Word row = last ? room.hold : practice_row(room.key, room.hold, t);
            opt = drift::openings(old, row, wiring, body.ports);
            command = drift::choose(opt, last ? mix(room.key ^ 0xdb713f81u) : practice_draw(room.key, t));
            source = inlet_port(senses, row);
            Word exit = destination(room, row, false);
            middle = braid::port(room.outputs, drift::horizontal(braid::port(room.pipes, row), room.channels));
            // The teacher can move the discharge sleeve after the commitment.
            // The intermediate crossing was visible before that change.
            if (last)
                exit = (exit + ((k >> 21) & 3)) & 3;
            dest = drift::face(axes, exit);
            --purse;
            ++r.paid;
            r.released += 4;
            Word bite = command == exit ? drift::low(4u, drift::low(body.mouth, 65535u - purse)) : 0;
            purse += bite;
            r.food += bite;
            if (hand.genes.arbor & (1u << 12))
                old = drift::strength(old, row, command, bite != 0, hand.genes.plastic, motor, true,
                                      body.ports & drift::regrowth, opt);
            if (!last)
                continue;
            ++r.scenes;
            Word board = braid::assembly(mix(k + 117) % 24);
            auto viewed = drift::forecast_view(board, 0, source, opt, axes, 1 + i, true);
            braid::expose(viewed, braid::result, braid::exchanged(board, source, dest), 15);
            braid::expose(viewed, braid::shortened, braid::exchanged(board, source, drift::face(axes, middle)), 15);
            auto scene = braid::through_glass(viewed, eyes);
            r.changed_forecasts += braid::visible(scene, braid::intended) == 15 &&
                                   braid::picture(scene, braid::intended) != braid::picture(scene, braid::result);
            r.distinct_midpoints += braid::picture(scene, braid::shortened) != braid::picture(scene, braid::result);
            braid::experience(m, scene);
            Word noun = ((k >> 12) & 3) + 4 * (lesson.kind == 2 ? ((k >> 3) & 1) : lesson.kind);
            Word previous = (k >> 18) & 7,
                 answer = braid::actual_answer(braid::picture(scene, braid::beginning), previous);
            braid::public_turn(m, lesson.names.wave[previous], lesson.names.wave[answer]);
            Word subject = lesson.role == 0 ? noun : lesson.role == 2 ? previous : answer;
            Word result = braid::possible(scene, lesson.frame, subject);
            if (!result || (result & (result - 1)))
                continue;
            Word ref = __builtin_ctz(result);
            auto sounds = braid::fill_question(lesson.words, lesson.slot, lesson.names.wave[noun]);
            if (!separate)
                tutor.source = motor;
            tutor.hear(m, hand.genes, sounds, lesson.names.wave[ref]);
            if (!separate)
                motor = tutor.source;
            ++r.presented;
        }
        hand.volley = (hand.volley & 65535u) | (old << 16);
    }
    r.attempted = tutor.attempts;
    r.matched = tutor.matches;
    r.edits = tutor.updates;
    organs->motor = motor;
    if (separate)
        organs->voice = tutor.source;
    *out = r;
}
KERNEL void inlet_before(const braid::Narration *vocabulary, Pupil subject, const Questions *script, Word seed,
                         Word cases, Course course, Before *records) {
    Word index = tid();
    if (index >= cases)
        return;
    Word life = index / 2, twin = index & 1;
    auto world = habitat(seed, life, course.ports & drift::seepage, course.landscape);
    auto old_room = prior_bed(seed, life, course, world);
    auto initial = course.backgrounds ? starting(course, seed, life) : Initial{subject.cells, subject.contacts};
    auto memory = prepare(subject.genes, world, old_room, course, twin, initial.contacts, initial.cells);
    // This launch has no target outlet or release command for the unseen
    // inlet. A later kernel may independently move the physical couplings.
    Word hue = (life + (world.key >> 5)) & 3, physical = braid::pigment(braid::inverse_view(subject.eyes.hues), hue);
    Word start = inlet_port(subject.senses, world.hold);
    auto frame = drift::forecast_view(((physical + 1) & 3) * 85u, physical, start, memory.choices, subject.muscles,
                                      index + 1, false);
    auto question = braid::fill_question(script->predict, script->variable, script->names.wave[4 + hue]);
    auto m = *vocabulary;
    m.topic = m.reply = 0;
    braid::experience(m, {});
    Word silent = hear(m, question, script->names);
    m.topic = m.reply = 0;
    braid::experience(m, braid::through_glass(frame, subject.eyes));
    Word predicted = hear(m, question, script->names);
    Word early = hear(m, script->actual, script->names);
    Word voted = braid::pigment(subject.eyes.places, drift::face(subject.muscles, memory.answer));
    Word local = braid::pigment(subject.eyes.places, drift::face(subject.muscles, memory.proximal));
    records[index] = {frame,
                      physical,
                      braid::pigment(subject.eyes.places, start),
                      memory.choices,
                      voted,
                      local,
                      predicted,
                      early,
                      silent,
                      m.topic,
                      m.reply,
                      memory.sampled,
                      memory.contacts,
                      initial,
                      memory.spikes,
                      memory.incoming,
                      memory.earlier_rows};
}
KERNEL void inlet_after(const braid::Narration *vocabulary, Pupil subject, const Questions *script, Word seed,
                        Word cases, Course course, const Before *record, After *answers) {
    Word index = tid();
    if (index >= cases * 2)
        return;
    Word specimen = index / 2, turn = index & 1, life = specimen / 2, twin = specimen & 1;
    auto world = habitat(seed, life, course.ports & drift::seepage, course.landscape);
    const auto &r = record[specimen];
    Word dest = (destination(world, world.hold, twin) + turn) & 3, physical = drift::face(subject.muscles, dest);
    auto visible = r.observed;
    braid::expose(visible, braid::result, drift::overlay(((r.dye + 1) & 3) * 85u, physical, r.dye), 15);
    auto m = *vocabulary;
    braid::experience(m, braid::through_glass(visible, subject.eyes));
    m.topic = 0;
    m.reply = r.reply;
    Word cleared = hear(m, script->actual, script->names);
    m.topic = r.topic;
    m.reply = r.reply;
    Word actual = hear(m, script->actual, script->names);
    Word hue = braid::pigment(subject.eyes.hues, r.dye);
    auto line = braid::fill_question(script->source, script->source_variable, script->names.wave[4 + hue]);
    Word start = hear(m, line, script->names);
    answers[index] = {braid::pigment(subject.eyes.places, physical), actual, start, cleared};
}
KERNEL void inlet_class(const Animal *genes, Word count, Word seed, Class school, Word *results) {
    Word i = tid();
    if (i >= count * school.lives)
        return;
    auto a = genes[i / school.lives];
    auto hand = school.hand;
    drift::wire(hand.genes, a.wiring, true);
    hand.genes.plastic = (hand.genes.plastic & ~15u) | a.law;
    hand.genes.arbor = (hand.genes.arbor & ~(1u << 12)) | (a.active << 12);
    if (school.shared)
        hand.genes.plastic = (hand.genes.plastic & ~0xf000u) | (a.law << 12);
    regulation::Source organ{a.program, school.soma, 0};
    Word language = mix(seed + (i % school.lives) * 7919);
    auto q = braid::regulated_lesson(hand, {}, organ, language, language, school.practice, apprentice::inherited,
                                     school.reversal, 0, 1, nullptr, school.frames, school.mode, 1,
                                     school.voice.band == ~0u ? nullptr : &school.voice);
    results[i] = q.intermediate | (q.qualified << 4) | (q.names << 8);
}
} // namespace axion::life::inlet
