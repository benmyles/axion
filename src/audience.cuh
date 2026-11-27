#include "axion/audience_batch.hpp"

namespace axion::life::audience {
KERNEL void audience_names(Pair *couple, const braid::Narration *memories) {
    Word i = tid();
    if (i < 8)
        (i < 4 ? couple->guide : couple->pupil).names[i & 3] = braid::name_of(memories[i >> 2], i & 3);
}
KERNEL void audience_practice(Pair *couple, Stock c, Word seed, Word rounds, Word pairing, Word guide_budget,
                              Word pupil_budget, Practice *out, Demonstration *events) {
    auto r = calibrate(*couple, c, seed, rounds, pairing, c.dialect, guide_budget, pupil_budget, events);
    couple->guide.eye.contacts = r.guide.contacts;
    couple->guide.eye.spikes = r.guide.spikes;
    couple->guide.eye.gate_b = (couple->guide.eye.gate_b & 65535u) | (r.tape << 16);
    couple->pupil.eye.contacts = r.pupil.contacts;
    couple->pupil.eye.spikes = r.pupil.spikes;
    *out = r;
}
KERNEL void audience_meet(const Pair *couple, Stock stock, Word first, Word count, Contact *result) {
    Word j = tid();
    if (j < count) {
        Word i = first + j;
        result[j] = encounter(*couple, couple->guide.genes, stock, water(stock, i >> 1), i & 1);
    }
}
KERNEL void audience_pour(const Pair *couple, Stock stock, Word first, Word count, const Contact *r, Catch *out) {
    Word j = tid();
    if (j < count)
        out[j] = release(*couple, water(stock, (first + j) >> 1), r[j]);
}
KERNEL void audience_commit(const Pair *couple, Stock c, Word count, Commitment *out) {
    Word i = tid();
    if (i < count)
        out[i] = seal(encounter(*couple, couple->guide.genes, c, water(c, i >> 1), i & 1));
}
KERNEL void audience_settle(const Pair *p, Stock c, Word cases, const Commitment *records, Sweep *groups) {
    Word i = tid();
    Sweep s{};
    if (i < cases / 2) {
        auto bed = water(c, i);
        auto a = unseal(records[2 * i]), b = unseal(records[2 * i + 1]);
        auto x = release(*p, bed, a), y = release(*p, bed, b);
        weigh(s, a, x, (records[2 * i].movement >> 24) & 31);
        weigh(s, b, y, (records[2 * i + 1].movement >> 24) & 31);
        s.values[changed] = a.opened && b.opened && a.picked != b.picked;
        s.values[both] = x.fresh && y.fresh;
        if (illustrate(bed, a, x, b, y))
            s.example = i;
    }
    for (Word &v : s.values) {
        for (Word delta = 16; delta; delta >>= 1) {
            Word other;
            asm("shfl.sync.bfly.b32 %0,%1,%2,31,-1;" : "=r"(other) : "r"(v), "r"(delta));
            v += other;
        }
    }
    for (Word delta = 16; delta; delta >>= 1) {
        Word other;
        asm("shfl.sync.bfly.b32 %0,%1,%2,31,-1;" : "=r"(other) : "r"(s.example), "r"(delta));
        s.example = drift::low(s.example, other);
    }
    if (!(i & 31))
        groups[i >> 5] = s;
}
KERNEL void audience_table(Table *table, Pair *couple, braid::Narration *words, braid::Saved *hands, Tissue *guide,
                           Tissue *pupil, Request q, Reply *answer) {
    auto &t = *table;
    const auto &p = *couple;
    Reply r{};
    if (q.act == meet) {
        t = {};
        for (Word i = 0; i < 2; ++i) {
            braid::experience(words[i], {});
            words[i].topic = words[i].reply = 0;
        }
        Word path = 0;
        if (q.specimen) {
            path = retina::as_built(*q.specimen).shape();
            q.stock.paths = &path;
            q.stock.sites = 1;
        }
        t.bed = water(q.stock, q.life);
        t.ports = q.stock.ports;
        t.contact = encounter(p, p.guide.genes, q.stock, t.bed, q.seat);
        const auto &c = t.contact;
        guide->spike = c.guide_spikes;
        guide->gate_b = (guide->gate_b & 65535u) | (c.tape << 16);
        pupil->spike = c.pupil_spikes;
        hands[0].mind.volley = (hands[0].mind.volley & 65535u) | (c.guide_contacts << 16);
        hands[1].mind.volley = (hands[1].mind.volley & 65535u) | (c.after << 16);
        r.paid_guide = c.guide_looks + c.work;
        r.paid_pupil = c.pupil_looks + c.taught;
        r.viewed = c.viewed;
        r.picked = c.picked;
        r.face = c.addressed;
        r.changed = __builtin_popcount(c.before ^ c.after);
        r.heard = c.taught;
        if (c.first_wave)
            r.words.wave[r.words.n++] = c.first_wave;
        if (c.second_wave)
            r.words.wave[r.words.n++] = c.second_wave;
    } else if (q.act == load) {
        Word ref = braid::ident(words[1], braid::intern(words[1], q.noun, false));
        if (ref < 4 || ref >= 8 || t.held || (t.used & (1u << q.inlet)))
            r.status = 1;
        else {
            t.held = 1;
            t.used |= 1u << q.inlet;
            t.source = t.bed.source[q.inlet];
            t.dye = braid::pigment(braid::inverse_view(p.pupil.eye.eyes.hues), ref - 4);
            for (Word i = 0; i < 2; ++i) {
                const auto &a = i ? p.pupil : p.guide;
                auto m = i ? TissueState{t.contact.after, t.contact.pupil_spikes}
                           : TissueState{t.contact.guide_contacts, t.contact.guide_spikes};
                auto action = expect(a, m, t.source, t.ports, mix(t.bed.key + t.source * 8731 + 3719));
                Word base = (t.dye ^ 3u) * 85u;
                auto event =
                    drift::forecast_view(base, t.dye, t.source, action.options, a.eye.muscles, t.bed.key | 1u, false);
                braid::experience(words[i], braid::through_glass(event, a.eye.eyes));
                words[i].topic = words[i].reply = 0;
            }
        }
    } else if (q.act == ask) {
        auto reply = braid::question(words[q.who], q.words, true);
        r.status = !reply.matched;
        if ((q.who ? p.pupil : p.guide).mouth)
            r.words = reply.words;
    } else if (q.act == pour) {
        if (!t.held)
            r.status = 1;
        else {
            t.held = 0;
            r.outlet = braid::port(t.bed.map, t.source);
            r.eaten =
                braid::port(t.contact.final_aim, t.source) == r.outlet ? drift::low(8u, 65535u - p.pupil.energy) : 0u;
            t.bites += r.eaten;
            t.settled += 8 - r.eaten;
            for (Word i = 0; i < 2; ++i) {
                auto glass = (i ? p.pupil : p.guide).eye.eyes;
                Word base = (t.dye ^ 3u) * 85u;
                auto image = braid::project_view(drift::overlay(base, r.outlet, t.dye), glass);
                braid::expose(words[i].event, braid::result, image, 15);
                words[i].reported = 0;
            }
        }
    }
    r.first = t.bed.source[0];
    r.second = t.bed.source[1];
    r.held = t.held * 8;
    r.stock = (2 - __builtin_popcount(t.used)) * 8;
    r.bites = t.bites;
    r.settled = t.settled;
    *answer = r;
}
KERNEL void audience_feeding(const Pair *couple, const apprentice::Animal *parents, Word n, Stock c,
                             apprentice::Harvest *food, Count *heat) {
    Word j = tid();
    if (j >= n)
        return;
    apprentice::Harvest meal{};
    const auto &adult = parents[j];
    if (adult.ancestor != ~0u) {
        auto bed = water(c, j >> 1);
        auto r = encounter(*couple, adult.program, c, bed, j & 1);
        auto caught = release(*couple, bed, r);
        Word sensory = r.guide_looks + r.pupil_looks + r.taught;
        meal.swallowed = c.bolus * caught.final + allowance(c) - r.work - sensory;
        meal.escaped = c.bolus * (2 - caught.final);
        asm("red.global.add.u64 [%0],%1;" ::"l"(heat + ((j >> 5) & 127)), "l"(Count(r.work)) : "memory");
        asm("red.global.add.u64 [%0],%1;" ::"l"(heat + 128 + ((j >> 5) & 127)), "l"(Count(sensory)) : "memory");
    }
    food[j] = meal;
}
KERNEL void audience_screen(const Pair *couple, const apprentice::Animal *parents, Word n, Stock c, Word layouts,
                            Word *score) {
    Word i = tid();
    if (i >= n || parents[i].ancestor == ~0u)
        return;
    Word total = 0;
    for (Word j = 0; j < 2 * layouts; ++j) {
        auto bed = water(c, j >> 1);
        auto r = encounter(*couple, parents[i].program, c, bed, j & 1);
        total +=
            c.bolus * release(*couple, bed, r).final + allowance(c) - r.work - r.guide_looks - r.pupil_looks - r.taught;
    }
    score[i] = total;
}
KERNEL void audience_juveniles(const Pair *couple, const Animal *residents, Word n, Stock c, Word *scores) {
    Word i = tid(), id = i >> 5;
    if (id >= n)
        return;
    Pair pair{residents[id], couple->pupil};
    Word sum = 0;
    for (Word turn = 0; turn < 2; ++turn) {
        auto bed = water(c, i & 31);
        auto r = encounter(pair, pair.guide.genes, c, bed, turn);
        sum += c.bolus * release(pair, bed, r).final + allowance(c) - r.guide_looks - r.pupil_looks - r.work - r.taught;
    }
    for (Word delta = 16; delta; delta >>= 1) {
        Word value;
        asm("shfl.sync.bfly.b32 %0,%1,%2,31,-1;" : "=r"(value) : "r"(sum), "r"(delta));
        sum += value;
    }
    if (!(i & 31))
        scores[id] = sum;
}
} // namespace axion::life::audience
