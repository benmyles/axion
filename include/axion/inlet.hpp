#pragma once
#include "drift.hpp"

namespace axion::life::inlet {
using Animal = drift::Germline;
enum Landscape : Word { field, channels, basin, mixed };
struct Initial {
    regulation::Cells cells{};
    Word contacts = 0;
};
struct Course {
    Word steps = 36, prior_steps = 0, landscape = field, ports = 0;
    const Initial *ambient{};
    Word backgrounds = 0, carry = 0, former_all = 0;
};
AX_FN Initial starting(Course course, Word seed, Word life) {
    if (!course.backgrounds)
        return {};
    auto experience = course.ambient[mix(seed ^ mix(11353u + (life >> 2) * 9733u)) % course.backgrounds];
    if (!(course.carry & 1))
        experience.cells = {};
    if (!(course.carry & 2))
        experience.contacts = 0;
    return experience;
}
struct Totals {
    Word predicted = 0, proximal = 0, displaced = 0, committed = 0, assured = 0, crossed = 0, paired = 0, changed = 0,
         changed_right = 0, trained_food = 0, full_evidence = 0, absent_right = 0, alternate_right = 0,
         channel_cases = 0, channel_right = 0, basin_cases = 0, basin_right = 0, contradicted = 0, recovered = 0;
};
struct Report {
    Word inlet = 0, pipes = 0, outputs = 0, hold = 0, swap = 0, target = 0, sampled = 0, hits = 0, contacts = 0,
         spikes = 0, choices = 0, draw = 0, answer = 0, proximal = 0, displaced = 0, landscape = field,
         initial_spikes = 0, background = 0, physical_map = 0, initial_contacts = 0, incoming = 0, earlier_rows = 0;
};
struct Layout {
    Word key, channels, pipes, outputs, hold, other, landscape;
};
AX_FN Layout habitat(Word seed, Word life, bool seep, Word kind = field) {
    Word k = mix(seed ^ mix(0xc781493du + (life >> 2) * 0x9e3779b9u));
    drift::View view{};
    view.rock = mix(k + 751);
    view.phase = (k >> 7) & 3;
    if (kind == mixed)
        kind = ((life >> 2) ^ seed) & 1 ? basin : channels;
    Word bottom = (k >> 20) & 3;
    for (Word j = 0; j < 4; ++j) {
        Word soil = mix(k + j * 503 + 31);
        view.bed.water[j] = 9 + (soil & 31);
        view.bed.clay[j] = (soil >> 7) & 3;
        view.bed.fibre[j] = (soil & 32768) ? 0u : (soil >> 17) & 3;
        if (kind == channels)
            view.bed.water[j] = 8 + (soil & 1);
        if (kind == basin) {
            Word distance = j == bottom ? 0 : j == (bottom ^ 2) ? 2 : 1;
            view.bed.water[j] = 4 + 14 * distance + (soil & 3);
            view.bed.clay[j] = 1 + (soil & 1);
            view.bed.fibre[j] = 3 + (soil & 1);
        }
    }
    Word pipes = braid::assembly(mix(k + 2139) % 24);
    Word dies = braid::assembly(mix(k + 4919) % 24), outputs = 0;
    for (Word j = 0; j < 4; ++j)
        outputs |= ((braid::port(dies, j) + life) & 3) << (2 * j);
    Word hold = (k >> 12) & 3;
    return {k,     drift::connections(view, true, seep || kind != field),
            pipes, outputs,
            hold,  (hold + 1 + mix(k + 891) % 3) & 3,
            kind};
}
AX_FN Word destination(const Layout &l, Word inlet, bool twin) {
    bool swap = twin && l.landscape == field;
    Word port = swap && inlet == l.hold ? l.other : swap && inlet == l.other ? l.hold : inlet;
    Word outcome = braid::port(l.outputs, drift::outlet(braid::port(l.pipes, port), l.channels));
    return (outcome + Word(twin && l.landscape != field)) & 3;
}
AX_FN Layout prior_bed(Word seed, Word life, Course course, const Layout &world) {
    if (!course.prior_steps)
        return world;
    Word kind = world.landscape == field ? field : world.landscape == basin ? channels : basin;
    auto before =
        habitat(seed ^ 0x9f9a13cbu, course.former_all ? life & ~3u : life, course.ports & drift::seepage, kind);
    before.hold = world.hold;
    before.other = world.other;
    return before;
}
AX_FN Word practice_row(Word key, Word hold, Word t) {
    Word rotation = mix(key + (t / 3) * 193 + 3319) % 3;
    return (hold + 1 + ((rotation + (t % 3)) % 3)) & 3;
}
AX_FN Word practice_draw(Word key, Word t) { return mix(key + t * 0x895cd6cdu + 7741); }
// Only a taste from the chosen mouth can affect a synapse. The unvisited
// inlet is presented once after the last practice reach with plasticity off.
AX_FN Report prepare(const Animal &a, const Layout &layout, const Layout &before, Course course, bool twin,
                     Word initial = 0, regulation::Cells initial_cells = {}) {
    regulation::Source organ{a.program, initial_cells, 0};
    Word contacts = initial, sampled = 0, hits = 0, ports = course.ports, incoming = initial, earlier = 0;
    for (Word trial = 0; trial < course.steps + course.prior_steps; ++trial) {
        bool prior = trial < course.prior_steps;
        const auto &terrain = prior ? before : layout;
        Word t = prior ? trial : trial - course.prior_steps;
        Word row = prior && course.former_all ? (t + mix(terrain.key + 119 * (t / 4))) & 3
                                              : practice_row(terrain.key, layout.hold, t);
        Word options = drift::openings(contacts, row, a.wiring, ports);
        Word draw = practice_draw(terrain.key, t);
        Word reach = drift::choose(options, draw);
        bool tasted = reach == destination(terrain, row, prior && course.former_all ? false : twin);
        if (prior)
            earlier |= 1u << row;
        else
            sampled |= 1u << row;
        hits += tasted;
        if (a.active)
            contacts =
                drift::strength(contacts, row, reach, tasted, a.law, organ, true, ports & drift::regrowth, options);
        if (prior)
            incoming = contacts;
    }
    Word row = layout.hold, opts = drift::openings(contacts, row, a.wiring, ports);
    Word draw = mix(layout.key ^ 0xdb713f81u);
    Word local = drift::choose(drift::openings(contacts, row, a.wiring, 0), draw);
    Word offset = drift::choose(drift::openings(contacts, row, a.wiring, ports ^ drift::misaligned), draw);
    return {layout.channels,
            layout.pipes,
            layout.outputs,
            row,
            layout.other,
            4u,
            sampled,
            hits,
            contacts,
            organ.cells.spike,
            opts,
            draw,
            drift::choose(opts, draw),
            local,
            offset,
            layout.landscape,
            initial_cells.spike,
            initial_cells.background,
            0,
            initial,
            incoming,
            earlier};
}
AX_FN Report encounter(const Animal &a, const Layout &layout, const Layout &before, Course course, bool twin,
                       Initial start = {}) {
    auto response = prepare(a, layout, before, course, twin, start.contacts, start.cells);
    response.target = destination(layout, layout.hold, twin);
    return response;
}
AX_FN void score(Totals &t, const Report &r) {
    bool certain = r.choices && !(r.choices & (r.choices - 1)), right = r.answer == r.target;
    t.predicted += right;
    t.proximal += r.proximal == r.target;
    t.displaced += r.displaced == r.target;
    t.committed += certain;
    t.assured += certain && right;
    t.changed += r.answer != r.proximal;
    t.changed_right += r.answer != r.proximal && right;
    t.trained_food += r.hits;
    t.channel_cases += r.landscape == channels;
    t.channel_right += r.landscape == channels && right;
    t.basin_cases += r.landscape == basin;
    t.basin_right += r.landscape == basin && right;
    t.full_evidence += __builtin_popcount(r.sampled) == 3 && !(r.sampled & (1u << r.hold)) &&
                       (((r.contacts ^ r.incoming) >> (4 * r.hold)) & 15) == 0;
    Word prior = (r.incoming >> (4 * r.hold)) & 15;
    bool contradicted = prior != 15 && (prior & (1u << r.target));
    t.contradicted += contradicted;
    t.recovered += contradicted && right;
    // The physically detached outlet takes all four positions with the same
    // witnessed history. Moving it cannot alter the pre-contact response.
    for (Word outlet = 0; outlet < 4; ++outlet)
        t.absent_right += r.answer == outlet;
}
enum Operation : Word { open_table, handle, stage, pour, turn, quiet_hand };
struct Instrument {
    Layout room{};
    braid::Episode opening{};
    Word seed = 0, ports = 0, twin = 0, practiced = 0, tried = 0, supply = 0, captured = 0, settled = 0, heat = 0,
         pending = 0, finished = 0, dye = 0, ground = 0, source = 0, options = 0, choice = 0, local = 0, moved = 0;
};
struct Gesture {
    Word cmd = 0, value = 0, landscape = 0, ports = 0, twin = 0, life = 0, energy = 0, mouth = 0, heading = 0,
         stamp = 0;
    Count noun = 0;
};
struct Receipt {
    Word practiced = 0, tried = 0, supply = 0, captured = 0, settled = 0, heat = 0, paid = 0, eaten = 0, pending = 0,
         finished = 0, options = 0, reach = 0, local = 0, outlet = 4, marker = 4, inlet = 0, before = 0, after = 0,
         status = 0, moved = 0;
};
struct Tuition {
    Word scenes = 0, presented = 0, attempted = 0, matched = 0, edits = 0, paid = 0, food = 0, released = 0,
         changed_forecasts = 0, distinct_midpoints = 0;
};
AX_FN Word inlet_port(Word senses, Word row) {
    return (senses & 4) ? drift::face(senses, row) : (row + 4 - (senses & 3)) & 3;
}
struct Questions {
    braid::Speech names{}, predict{}, actual{}, source{};
    Word variable = 0, source_variable = 0;
};
struct Class {
    braid::Mind hand{};
    regulation::Cells soma{};
    Word lives = 8, practice = 12, reversal = 0, shared = 0, frames = 2, mode = 0;
    regulation::Source voice{{}, {}, ~0u};
};
struct Pupil {
    Animal genes{};
    braid::Optics eyes{};
    Word senses = 0, muscles = 0, contacts = 0;
    regulation::Cells cells{};
};
struct Before {
    braid::Episode observed{};
    Word dye = 0, start = 0, apertures = 0, preferred = 0, proximal = 0, projected = 8, early = 8, silent = 8,
         topic = 0, reply = 0, sampled = 0, contacts = 0;
    Initial initial{};
    Word spikes = 0, incoming = 0, earlier_rows = 0;
};
struct After {
    Word position = 0, actual = 8, source = 8, cleared = 8;
};
} // namespace axion::life::inlet
