#pragma once
#include "capillary_sheet.hpp"
#include "material_context.hpp"

namespace axion::life::capillary {
enum Arm : Word {
    active,
    passive,
    random_grips,
    crossed_results,
    frozen,
    lookup,
    opposite_hand,
    erased,
    yoked,
    severed,
    reverse_order
};
struct Course {
    Word seed = 58099, views = 32, probes = 8, budget = 96, depth = 8, moves = 3, arm = active, gate = 1, rock = 0,
         scarce = 0, empty_contacts = 0, adaptive = 0, regional = 0;
};
struct CohortGrade {
    Word first = 0, caught = 0, opens = 0, strong = 0, disagreed = 0;
};
struct Lessons {
    Tissue tissue{};
    History history{}, initial_history{};
    Word edits = 0, spikes = 0, heat = 0, acts = 0, looks = 0, isolated = 0, informative = 0, terminal = 0, passed = 0,
         settled = 0;
    Count initial_on = 0, initial_off = 0, actions = 1469598103934665603ull;
};
struct Step {
    Count old_on = 0, old_off = 0, rivals = 0, positive = 0, negative = 0;
    Word previous = 0, intended = 0, physical = 0, seen = 0, result = 0, cost = 0, initial = 0, terminal = 0,
         admitted = 0;
};
AX_FN Word passive_row(Word seed, Word j) {
    Word row = (mix(seed + 2053) + j * (mix(seed + 2243) | 1u)) & 31;
    return row | (((row >> 2) & 1) << 5);
}
AX_FN Lessons develop(Body body, Course course, Word salt, Word mineral, Tissue natal = {}, Step *steps = nullptr,
                      History history = {}) {
    Lessons taught{};
    taught.tissue = natal;
    taught.history = history;
    taught.initial_history = history;
    History driver_history = history;
    taught.initial_on = natal.on;
    taught.initial_off = natal.off;
    Tissue driver = natal;
    Body operator_body = body;
    bool controlled = course.arm == yoked || course.arm == severed;
    Word present = 0;
    Word last = 0;
    for (Word t = 0; t < course.views + course.probes; ++t) {
        bool audition = t < course.views;
        Word cost = 1, row = 0, intent = 0;
        Fit belief{};
        Revision prior{};
        bool adaptive = course.adaptive && course.arm != lookup;
        if (audition || course.arm == passive) {
            if (!audition)
                cost = 2;
            row = passive_row(salt, t);
            intent = row;
        } else {
            Tissue source = controlled ? driver : taught.tissue;
            Word coin = mix(salt ^ mix(t * 11971 + 4711));
            Search grip{};
            if (adaptive) {
                prior = revise(source, controlled ? driver_history : taught.history, body);
                belief = prior.fit;
                grip = recheck(source, prior, present, coin, body, course.budget - taught.heat);
            } else {
                belief = fit(source, body.dendrites, course.arm != lookup);
                grip = investigate(source, belief, present, 63, coin, body.motor, course.budget - taught.heat,
                                   channels(body.dendrites));
            }
            if (course.arm == random_grips) {
                grip.choice = mix(coin + 3613) & 63;
                grip.spent = 2 + __builtin_popcount(grip.choice ^ present);
            }
            if (!grip.spent || !body.limbs)
                break;
            intent = grip.choice;
            row = intent;
            cost = grip.spent;
        }
        if (cost > course.budget - taught.heat)
            break;
        bool actual = surge(row, mineral);
        Word perceived = row & channels(body.dendrites), before = body.cells.spike;
        Word feedback = course.arm == crossed_results ? last
                        : course.arm == yoked         ? Word(surge(row, mineral ^ 16))
                                                      : Word(actual);
        if (steps)
            steps[t] = {taught.tissue.on,
                        taught.tissue.off,
                        belief.rivalry,
                        belief.model.on,
                        belief.model.off,
                        present,
                        intent,
                        row,
                        perceived,
                        feedback,
                        cost,
                        before,
                        0,
                        0};
        Word edits = 0;
        if (controlled) {
            Word flares = 0;
            Word permission = 0;
            material_receive(driver, operator_body, row, actual, flares, course.gate, &permission);
            if (adaptive)
                remember(driver_history, perceived, actual, !audition, prior.diagnostic, permission);
        }
        if (course.arm != frozen && (audition || course.arm != severed)) {
            Word permission = 0;
            Count rival = adaptive && controlled && !audition ? revise(taught.tissue, taught.history, body).diagnostic
                                                              : prior.diagnostic;
            edits = material_receive(taught.tissue, body, row, feedback, taught.spikes, course.gate, &permission);
            if (adaptive)
                remember(taught.history, perceived, feedback, !audition && course.arm != passive, rival, permission);
        }
        if (steps) {
            steps[t].terminal = body.cells.spike;
            steps[t].admitted = edits;
        }
        taught.passed += actual;
        taught.settled += !actual;
        taught.edits += edits;
        taught.heat += cost;
        ++taught.looks;
        if (!audition) {
            ++taught.acts;
            taught.isolated += ((row >> 2) ^ (row >> 5)) & 1;
            taught.informative += bool(belief.rivalry & bit(perceived));
            taught.actions = (taught.actions ^ row) * 1099511628211ull;
        }
        if (audition) {
            taught.initial_on = taught.tissue.on;
            taught.initial_off = taught.tissue.off;
            taught.initial_history = taught.history;
        }
        present = row;
        last = actual;
    }
    taught.terminal = body.cells.spike;
    return taught;
}
AX_FN Count scatter(Word seed) { return (Count(mix(seed + 7837)) << 32) | mix(seed ^ 0x673ac095u); }
AX_FN Work channel(Word key, Word length, Word scarce = 0) {
    Work w{};
    w.time = length;
    w.n = grips;
    Word least = length > 7 ? length - 7 : 0, most = length < 7 ? length : 7;
    Word east = least + mix(key + 1523) % (most - least + 1), south = length - east;
    Word x = mix(key + 4229) % (8 - east), y = mix(key + 8737) % (8 - south);
    Word at = y * 8 + x, trail[15]{};
    trail[0] = at;
    w.feed = bit(at);
    Count route = bit(at);
    for (Word t = 0; t < length; ++t) {
        bool horizontal = mix(key + t * 1877 + 3233) % (length - t) < east;
        if (horizontal) {
            ++at;
            --east;
        } else {
            at += 8;
            --south;
        }
        trail[t + 1] = at;
        route |= bit(at);
    }
    w.catcher = bit(at);
    w.bed.wet = route | (scatter(key + 17183) & scatter(key + 289369) & scatter(key + 4099));
    w.bed.sill = scatter(key + 4583) & ~route;
    w.bed.wick = scatter(key + 8219);
    w.bed.paint = scatter(key + 3067);
    // A movable pair of coatings at the same seat distinguishes materials
    // with identical observational histories. The handles carry no indication
    // of whether either coating conducts in this particular bed.
    Word hinge = trail[1 + mix(key + 1531) % length];
    w.bed.sill |= bit(hinge);
    w.bed.wick &= ~bit(hinge);
    w.bed.paint &= ~bit(hinge);
    w.handle[0] = {Byte(hinge), 0};
    w.handle[1] = {Byte(hinge), 3};
    Word repairs = (key >> 5) % 3;
    Word candidates[14]{};
    Word n = 0;
    for (Word j = 1; j <= length; ++j)
        if (trail[j] != hinge)
            candidates[n++] = trail[j];
    for (Word k = 2; k < grips; ++k) {
        Word pos = mix(key + 3709 * k);
        Word site;
        if (n) {
            Word rank = pos % n;
            site = candidates[rank];
            candidates[rank] = candidates[--n];
        } else
            site = pos & 63;
        Word material = (pos >> 8) & 3;
        if (k < 2 + repairs) {
            material = 1;
            w.bed.wet &= ~bit(site);
        }
        w.handle[k] = {Byte(site), Byte(material)};
    }
    if (scarce) {
        w.stock = standard_stock & ~0xf00fu;
        for (Word j = 4; j < 6; ++j) {
            Word material = j == 4 ? 0 : 3, site = mix(key + 911 * j) & 63;
            while (site == hinge || site == w.handle[2].site || site == w.handle[3].site ||
                   (j == 5 && site == w.handle[4].site))
                site = (site + 1) & 63;
            surface(w.bed, material) |= bit(site);
            w.handle[j] = {Byte(site), Byte(material)};
        }
    }
    // A discharge into another outlet loses the portion if that outlet is
    // connected. Spare wet seats and converging paths remain real shortcuts.
    if ((key & 3) == 0 && (at & 7) && at / 8 < 7)
        w.waste = bit(at + 7);
    for (Word i = grips - 1; i; --i) {
        Word j = mix(key + 7321 * i) % (i + 1);
        auto q = w.handle[j];
        w.handle[j] = w.handle[i];
        w.handle[i] = q;
    }
    return w;
}
struct Output {
    Tissue tissue{};
    Model belief{};
    Count supports = 0, rivalry = 0, fingerprint = 0, initial_on = 0, initial_off = 0;
    Word heat = 0, acts = 0, looks = 0, isolated = 0, informative = 0, edits = 0, spikes = 0, terminal = 0;
    Word baseline = 0, prior_actions = 0, prior_ready = 0, answer = absent, action = 0, ready = 0, alternatives = 0,
         tried = 0;
    Word predicted_on = 0, predicted_off = 0, single_known = 0;
    Count predictions = 0, known = 0;
    Word passed = 0, settled = 0;
    History history{};
};
struct Plate {
    Count before = 0, after = 0, goal = 0, spill = 0;
};
struct Result {
    Count solutions = 0, feasible = 0;
    Protocol commands{};
    Receipt actual{};
    Word shortest = grips + 1, caught = 0, leaked = 0, released = 0;
};
AX_FN Output assess(Body body, Course c, Lessons lesson, const Work &w) {
    Tissue tissue = c.arm == erased ? Tissue{} : lesson.tissue;
    auto theory =
        read_material(tissue, c.arm == erased ? History{} : lesson.history, body, c.adaptive, c.arm != lookup);
    Word senses = channels(body.dendrites);
    auto initial = read_material({lesson.initial_on, lesson.initial_off}, lesson.initial_history, body, c.adaptive,
                                 c.arm != lookup);
    auto base_image = project(theory.model, senses, w.bed);
    auto seen = forecast(w, base_image);
    auto earlier = design(w, initial.model, senses, 63, c.moves);
    auto p = design(w, theory.model, senses, 63, c.moves);
    Count known = 0, positive = 0;
    for (Word m = 0; m < (1u << w.n); ++m) {
        auto v = target(w, project_plan(w, altered(w, base_image, theory.model, senses, m),
                                        (theory.model.on | theory.model.off) == full, m));
        if (v != absent) {
            known |= bit(m);
            if (v)
                positive |= bit(m);
        }
    }
    return {tissue,
            theory.model,
            theory.supports,
            theory.rivalry,
            lesson.actions,
            lesson.initial_on,
            lesson.initial_off,
            lesson.heat,
            lesson.acts,
            lesson.looks,
            lesson.isolated,
            lesson.informative,
            lesson.edits,
            lesson.spikes,
            lesson.terminal,
            target(w, forecast(w, project(initial.model, senses, w.bed))),
            earlier.motions,
            earlier.ready,
            target(w, seen),
            p.motions,
            p.ready,
            p.alternatives,
            p.tried,
            count(theory.model.on),
            count(theory.model.off),
            count(theory.model.on | theory.model.off),
            positive,
            known,
            lesson.passed,
            lesson.settled,
            lesson.history};
}
AX_FN Result open_nets(const Work &w, Word mineral, const Output &o, Word arm) {
    Result r{};
    for (Word m = 0; m < (1u << w.n); ++m) {
        bool can = stocked(w, m);
        if (can)
            r.feasible |= bit(m);
        Count s = release(w, m, mineral);
        if ((s & w.catcher) == w.catcher && !(s & w.waste)) {
            r.solutions |= bit(m);
            Word n = __builtin_popcount(m);
            if (can && n < r.shortest)
                r.shortest = n;
        }
    }
    r.commands = order(w, o.action);
    r.actual = perform(w, r.commands, arm == reverse_order, arm == opposite_hand);
    auto actual = release(w, r.actual.position, mineral);
    r.released = o.ready;
    r.leaked = o.ready && bool(actual & w.waste);
    r.caught = o.ready && (actual & w.catcher) == w.catcher && !r.leaked;
    return r;
}
} // namespace axion::life::capillary
