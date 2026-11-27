#include "axion/context_study.hpp"

namespace axion::life::capillary {
KERNEL void context_currents(Saved *resident, Body original, Course c, StreamCourse schedule, const Gloss *lexicon,
                             const Work *natural, StreamRow *rows) {
    c.adaptive = 0;
    Word i = tid();
    if (i >= schedule.streams)
        return;
    Body body = original, driver = original;
    if (schedule.mode == retention_flipped)
        body.growth.law ^= 4;
    Tissue t = c.empty_contacts ? Tissue{} : resident->mind.tissue, guided = t;
    History h = c.empty_contacts ? History{} : resident->history, driver_history = h;
    Word senses = channels(body.dendrites), variant = 0, last_rock = c.rock;
    bool pooled = schedule.mode == single_memory || schedule.mode == examiner_clears;
    bool matched = schedule.mode == intervention_clamped || schedule.mode == feedback_exchanged ||
                   schedule.mode == retention_flipped;
    auto model = [&]() { return pooled ? fit(t, body.dendrites).model : revise(t, h, body).fit.model; };
    for (Word j = 0; j < schedule.rounds; ++j) {
        auto state = epoch(c, schedule, i, j, variant, last_rock);
        Word key = state.key, mineral = state.rock;
        bool changed = state.change;
        auto lesson = state.lesson;
        last_rock = mineral;
        if (schedule.mode == examiner_clears)
            t = {};
        StreamRow r{};
        r.before = model();
        r.observed = r.before;
        r.rock = mineral;
        r.changed = changed;
        r.key = key;
        r.senses = senses;
        r.views = lesson.views;
        r.period = 4 + ((body.motor >> 23) & 31);
        Word salt = mix(key + 14143);
        Lessons paid{};
        if (pooled) {
            paid = develop(body, lesson, salt, mineral, t);
            body.cells.spike = paid.terminal;
            t = paid.tissue;
            r.observed = fit({paid.initial_on, paid.initial_off}, body.dendrites).model;
        } else {
            Word received = 0, firings = 0, writes = 0;
            auto observe = [&](Word at, Word row, Word actual, bool act, Tissue sender, History recent) {
                if (matched) {
                    if (schedule.mode == feedback_exchanged)
                        actual = surge(row, mineral ^ 16);
                    Count diagnostic = act && schedule.mode != intervention_clamped ? revise(t, h, body).diagnostic : 0;
                    Word permission = 0;
                    received += material_receive(t, body, row, actual, firings, c.gate, &permission);
                    remember(h, row & senses, actual, act, diagnostic, permission);
                    writes += act && bool(diagnostic & bit(row & senses)) && bool(permission & (actual ? 1 : 2));
                }
                if (at + 1 == lesson.views)
                    r.observed = matched ? model() : revise(sender, recent, body).fit.model;
            };
            auto result = context_stream(matched ? driver : body, lesson, salt, mineral, matched ? guided : t,
                                         matched ? driver_history : h, schedule.mode != unchecked, observe);
            paid = result.record;
            r.rechecks = result.deliberate;
            r.earliest = result.first_review;
            r.anchors = matched ? writes : result.anchors;
            if (matched) {
                guided = paid.tissue;
                driver_history = result.memory;
                driver.cells.spike = paid.terminal;
                paid.edits = received;
                paid.spikes = firings;
            } else {
                t = paid.tissue;
                h = result.memory;
                body.cells.spike = paid.terminal;
            }
        }
        r.after = model();
        r.tissue = t;
        r.memory = h;
        r.heat = paid.heat;
        r.probes = paid.acts;
        r.edits = paid.edits;
        r.spikes = paid.spikes;
        r.passive_looks = paid.looks - paid.acts;
        r.fingerprint = paid.actions;
        r.flux = paid.passed;
        auto work = natural ? natural[Count(i) * schedule.rounds + j] : channel(mix(key + 9791), c.depth, c.scarce);
        auto construction = design(work, r.after, senses, 63, c.moves);
        r.design = construction.motions;
        r.ready = construction.ready;
        auto show = encounter(key, mineral, schedule.varied);
        auto &e = show.visible;
        auto question =
            interview_prompt(*lexicon, e, show.kind, (show.seed >> 4) & 3, 4 + ((show.seed & 1) ? e.pad : e.pigment));
        auto said = say(resident->speech, body, e, r.after, acoustic(*lexicon, question));
        r.spoken = message(*lexicon, said.voice);
        r.terminal = body.cells.spike;
        rows[Count(i) * schedule.rounds + j] = r;
    }
}
KERNEL void context_grade(Course c, Word varied, const Gloss *lexicon, const Work *natural, const StreamRow *rows,
                          Word n, StreamGrade *out) {
    Word i = tid();
    if (i >= n)
        return;
    auto &r = rows[i];
    StreamGrade g{};
    for (Word z = 0; z < 64; ++z)
        if (surge(z, r.rock))
            g.law |= bit(z);
    g.old_exact = local_agreement(r.before, g.law, r.senses);
    g.watched_exact = local_agreement(r.observed, g.law, r.senses);
    g.exact = local_agreement(r.after, g.law, r.senses);
    auto w = natural ? natural[i] : channel(mix(r.key + 9791), c.depth, c.scarce);
    for (Word m = 0; m < (1u << w.n) && !g.possible; ++m)
        if (Word(__builtin_popcount(m)) <= c.moves && stocked(w, m))
            g.possible = actual(w, m, r.rock);
    auto worked = perform(w, order(w, r.design));
    g.caught = r.ready && !worked.denied && actual(w, worked.position, r.rock);
    auto show = encounter(r.key, r.rock, varied);
    auto &e = show.visible;
    auto task = challenge(*lexicon, e, r.rock, show.kind, show.seed);
    if (varied) {
        g.scene = address(e.work.feed, e.work.bed, 18) | (e.changed << 6) | (e.pigment << 12) | (e.pad << 14) |
                  (e.earlier << 16) | (e.later << 17) | (show.kind << 18) | (task.named << 21) | (task.desired << 23) |
                  (task.cause << 26);
        for (Word j = 0; j < e.work.n; ++j)
            g.handles |= Word(e.work.handle[j].material) << (3 * j);
    }
    g.goal = message(*lexicon, acoustic(*lexicon, task.response));
    g.said = r.spoken && r.spoken == g.goal;
    g.available = g.goal != 0;
    out[i] = g;
}
} // namespace axion::life::capillary

namespace axion::life::capillary {
AX_FN void inspect_life(Word *grade, Model m, const Work &bed, Word rock, Count truth, Word senses, Word limit,
                        bool after, bool first, bool changed) {
    Word exact = local_agreement(m, truth, senses);
    grade[after ? later_laws : warm_laws] += exact;
    grade[first_later] += first && exact;
    grade[changed_laws] += changed && exact;
    auto p = design(bed, m, senses, 63, limit);
    grade[earned_food] += p.ready && actual(bed, p.motions, rock);
}
KERNEL void context_cohort(const Body *residents, const Mind *minds, const History *memories, Word enrolled, Course c,
                           StreamCourse series, const Work *natural, LifeGrade *tab) {
    Word id = tid() / 32, lane = tid() & 31;
    if (id >= enrolled)
        return;
    LifeGrade totals{};
    auto senses = channels(residents[id].dendrites);
    Count laws[4]{};
    for (Word k = 0; k < 4; ++k) {
        Word base = k >= 2 && series.second < 32 ? series.second : c.rock, rock = base ^ ((k & 1) * 16);
        for (Word row = 0; row < 64; ++row)
            if (surge(row, rock))
                laws[k] |= bit(row);
    }
    for (Word life = lane; life < series.streams; life += 32) {
        auto original = residents[id];
        original.pressure_xor = c.regional ? original.module_mask : 0;
        auto twin = original;
        twin.growth.law ^= 4;
        Tissue material = c.empty_contacts ? Tissue{} : minds[id].tissue, other = material;
        History history = c.empty_contacts || !memories ? History{} : memories[id], alternate = history;
        Word variant = 0, previous = c.rock, native[life_metrics]{}, paired[life_metrics]{};
        for (Word j = 0; j < series.rounds; ++j) {
            auto e = epoch(c, series, life, j, variant, previous);
            previous = e.rock;
            totals.changes += e.change;
            auto copy = [&](Word, Word row, Word observed, bool hand, Tissue, History) {
                Count wager = hand ? revise(other, alternate, twin).diagnostic : 0;
                Word admission = 0, flares = 0;
                material_receive(other, twin, row, observed, flares, c.gate, &admission);
                remember(alternate, row & senses, observed, hand, wager, admission);
            };
            auto studied =
                context_stream(original, e.lesson, mix(e.key + 14143), e.rock, material, history, true, copy);
            material = studied.record.tissue;
            history = studied.memory;
            original.cells.spike = studied.record.terminal;
            auto w = natural ? natural[Count(life) * series.rounds + j] : channel(mix(e.key + 9791), c.depth, c.scarce);
            bool late = j >= series.at;
            Count law = laws[(late ? 2 : 0) + (variant >> 4)];
            inspect_life(native, revise(material, history, original).fit.model, w, e.rock, law, senses, c.moves, late,
                         j == series.at, e.change);
            inspect_life(paired, revise(other, alternate, twin).fit.model, w, e.rock, law, senses, c.moves, late,
                         j == series.at, e.change);
            totals.heat += studied.record.heat;
            totals.probes += studied.record.acts;
        }
        auto summarize = [&](Word *r, Word *sum) {
            r[complete_warm] = r[warm_laws] == series.at;
            r[complete_later] = r[later_laws] == series.rounds - series.at;
            r[complete_life] = r[complete_warm] && r[complete_later];
            for (Word m = 0; m < life_metrics; ++m)
                sum[m] += r[m];
        };
        summarize(native, totals.native);
        summarize(paired, totals.paired);
    }
    auto sum = [](Word v) {
        Word r;
        asm("redux.sync.add.u32 %0,%1,0xffffffff;" : "=r"(r) : "r"(v));
        return r;
    };
    for (Word k = 0; k < life_metrics; ++k) {
        totals.native[k] = sum(totals.native[k]);
        totals.paired[k] = sum(totals.paired[k]);
    }
    totals.heat = sum(totals.heat);
    totals.probes = sum(totals.probes);
    totals.changes = sum(totals.changes);
    if (!lane)
        tab[id] = totals;
}
} // namespace axion::life::capillary
