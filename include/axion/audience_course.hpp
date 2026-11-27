#pragma once
#include "audience.hpp"
#include "retina_study.hpp"
#include "apprentice.hpp"

namespace axion::life::audience {
struct Animal {
    retina::Observer eye{};
    regulation::Program genes{};
    Word mouth = 0, ears = 0, energy = 0, limbs = 0;
    Count names[4]{};
    Word lexical_law = 0, receptive = 0;
};
struct Pair {
    Animal guide{}, pupil{};
};
// Each orientation has two four-by-four banks. Each active acoustic cell
// projects to four local position cells. The sensory event arrives later.
struct Alignment {
    Word bank[2]{};
};
enum Access : Word { split, common, neither, mixed, unequal };
struct Stock {
    Word seed = 0, rounds = 2, arm = intact, ports = 0, bolus = 16;
    const Word *paths{};
    Word sites = 0, tracer = 0;
    Word calibrated = 0;
    Alignment dialect{};
    Word access = split;
};
AX_FN Word exposure(Word access, Word life, Word seed, Word seat) {
    if (access == common)
        return 3;
    if (access == neither)
        return 0;
    if (access == unequal)
        return (seat ^ (life & 1)) ? 0 : 3;
    if (access == mixed) {
        // All sixteen joint histories occur once in each block of layouts.
        Word set = (life + mix(seed ^ 0x9a84297u)) & 15;
        return (set >> (2 * seat)) & 3;
    }
    return 1u << seat;
}
AX_FN Word allowance(Stock c) {
    return (c.access == split ? 5u : 6u) * c.rounds + 3;
}
struct Layout {
    Word key, map, source[2], coat[2], site, tie, access[2];
};
AX_FN Layout water(Stock c, Word life) {
    Word key = mix(c.seed ^ mix(life * 49523u + 19471u));
    Word order = braid::assembly(mix(key + 1847) % 24);
    Word coats = braid::assembly(mix(key + 7681) % 24);
    Word site = c.sites ? mix(key + 7559) % c.sites : ~0u;
    Word map = braid::assembly(mix(key + 14753) % 24);
    if (c.sites) {
        auto path = drift::Path::recover(c.paths[site]);
        Word links = key & 1024 ? path.upper : path.lower;
        map = 0;
        for (Word j = 0; j < 4; ++j)
            map |= drift::outlet(j, links) << (2 * j);
    }
    return {key,
            map,
            {braid::port(order, 0), braid::port(order, 1)},
            {braid::port(coats, 0), braid::port(coats, 1)},
            site,
            mix(key + 2053),
            {exposure(c.access, life, c.seed, 0), exposure(c.access, life, c.seed, 1)}};
}
struct TissueState {
    Word contacts = 0, spikes = 0;
};
AX_FN retina::Light glint(const Animal &a, Word from, Word to) {
    return retina::project(retina::reflected(255, 0, from, to), a.eye.eyes);
}
AX_FN retina::Reception view(const Animal &a, TissueState &m, retina::Light light) {
    auto v = retina::expose(a.eye.genes, light, a.eye.heading, m.contacts, m.spikes);
    m = {v.contacts, v.spikes};
    return v;
}
struct Pose {
    Word pre = 0, options = 0, aim = 4, voice = 4;
};
AX_FN Pose expect(const Animal &a, TissueState m, Word source, Word ports, Word tie) {
    auto l = glint(a, source, source);
    Word pre = retina::initial(a.eye.genes, l.rest, l.onset, a.eye.heading);
    Word options = retina::advance(pre, m.contacts, a.eye.wiring, ports);
    Word aim = drift::face(a.eye.muscles, drift::choose(options, tie));
    Word voice = options && !(options & (options - 1)) ? braid::pigment(a.eye.eyes.places, aim) : 4;
    return {pre, options, aim, voice};
}
AX_FN Count utter(const Animal &a, Word ref) {
    return a.mouth && ref < 4 ? a.names[ref] : 0;
}
AX_FN Word hear(const Animal &a, Count word) {
    if (a.ears && word)
        for (Word j = 0; j < 4; ++j)
            if (a.names[j] == word)
                return j;
    return 4;
}
AX_FN Word restrict_row(Word bank, Word name, Word light, Word law) {
    if (name >= 4 || !light)
        return bank;
    Word shift = 4 * name, mask = 15u << shift;
    Word next = braid::rule_bits(law, bank, ((15u ^ light) << shift));
    return (bank & ~mask) | (next & mask);
}
AX_FN void couple(Alignment &code, const Animal &a, Word first, Word second, retina::Light light) {
    if (!a.receptive || first >= 4 || second >= 4)
        return;
    Word on = retina::contrast(light.rest, light.onset, a.eye.genes.receptors);
    Word off = retina::contrast(light.rest, light.end, a.eye.genes.receptors >> 2);
    for (Word k = 0; k < 2; ++k) {
        Word lo = restrict_row(code.bank[k] & 65535u, first, k ? off : on, a.lexical_law);
        Word hi = restrict_row(code.bank[k] >> 16, second, k ? on : off, a.lexical_law);
        code.bank[k] = lo | (hi << 16);
    }
}
AX_FN bool viable(Word bank) {
    return !(bank & (bank >> 1) & (bank >> 2) & (bank >> 3) & 0x11111111u);
}
AX_FN Word resolved(Word set) {
    return set && !(set & (set - 1)) ? Word(__builtin_ctz(set)) : 4;
}
AX_FN void translate(Alignment code, Word f, Word s, Word &from, Word &to) {
    Word source = 0, end = 0;
    if (f < 4 && s < 4)
        for (Word k = 0; k < 2; ++k) {
            Word bank = code.bank[k];
            if (!viable(bank))
                continue;
            Word a = (~bank >> (4 * f)) & 15, b = (~bank >> (16 + 4 * s)) & 15;
            source |= k ? b : a;
            end |= k ? a : b;
        }
    from = resolved(source);
    to = resolved(end);
}
struct Contact {
    Word key = 0, site = ~0u, map = 0, twin = 0, first = 0, second = 0, coat_a = 0, coat_b = 0;
    Word viewed = 0, seen = 0, tape_before = 0, tape = 0, addressed = 0, write_gene = 0, motor_gene = 0;
    Word pre_a = 0, pre_b = 0, forecast_a = 0, forecast_b = 0, tie = 0, left = 0, right = 0, picked = 0, opened = 0;
    Word said_from = 4, said_to = 4, heard_from = 4, heard_to = 4, taught = 0, before = 0, after = 0;
    Word guide_contacts = 0, guide_spikes = 0, pupil_spikes = 0, starting_spikes = 0;
    Word prior_aim = 0, final_aim = 0, prior_voice = 0, final_voice = 0, work = 0, guide_looks = 0, pupil_looks = 0;
    Word spoken_order = 0, received_order = 0, release_from = 4, release_to = 4;
    Count first_wave = 0, second_wave = 0;
};
struct Catch {
    Word prior = 0, final = 0, voiced_before = 0, voiced_after = 0, truthful = 0, fresh = 0;
};
// Seat determines which public shell reappears. Both seats share the complete
// history, the expert, the two candidates, the tie and the later releases.
AX_FN Contact encounter(const Pair &pair, regulation::Program genes, Stock c, Layout bed, Word seat) {
    const auto &mentor = pair.guide, &child = pair.pupil;
    Contact r{};
    r.key = bed.key;
    r.site = bed.site;
    r.map = bed.map;
    r.twin = seat;
    r.first = bed.source[0];
    r.second = bed.source[1];
    r.coat_a = bed.coat[0];
    r.coat_b = bed.coat[1];
    r.write_gene = genes.first;
    r.motor_gene = genes.second;
    r.tie = bed.tie;
    r.tape = r.tape_before = mentor.eye.gate_b >> 16;
    TissueState expert{mentor.eye.contacts, mentor.eye.spikes}, pupil{child.eye.contacts, child.eye.spikes};
    Word order = braid::assembly(mix(bed.key + 30467) % 24);
    for (Word t = 0; t < c.rounds * 4; ++t) {
        Word source = braid::port(order, (t + (t >> 2)) & 3), dest = braid::port(bed.map, source);
        bool can_see = r.guide_looks + 1 < mentor.energy;
        retina::Reception v{};
        if (can_see) {
            v = view(mentor, expert, glint(mentor, source, dest));
            ++r.guide_looks;
        }
        Word station = source == r.first ? 1u : source == r.second ? 2u : 0u;
        for (Word i = 0; i < 2; ++i) {
            if (!(bed.access[i] & station))
                continue;
            Word badge = bed.coat[c.arm == disguised ? 0 : i];
            Word face = braid::pigment(mentor.eye.eyes.hues, badge);
            if (can_see && c.arm != screened && c.arm != hidden) {
                r.tape = remember(genes, r.tape, face, v.pre, v.post);
                r.seen |= 1u << source;
            }
        }
        if ((bed.access[seat ^ Word(c.arm == private_view)] & station) && r.pupil_looks + 1 < child.energy) {
            view(child, pupil, glint(child, source, dest));
            ++r.pupil_looks;
            r.viewed |= 1u << source;
        }
    }
    Word badge = bed.coat[c.arm == disguised ? 0 : (seat ^ Word(c.arm == exchanged))];
    r.addressed = braid::pigment(mentor.eye.eyes.hues, badge);
    auto a = expect(mentor, expert, r.first, c.ports, r.tie);
    auto b = expect(mentor, expert, r.second, c.ports, r.tie);
    r.guide_contacts = expert.contacts;
    r.guide_spikes = expert.spikes;
    auto action = offer(genes, r.tape, r.addressed, a.pre, b.pre, a.options, b.options, r.tie);
    r.pre_a = a.pre;
    r.pre_b = b.pre;
    r.forecast_a = a.options;
    r.forecast_b = b.options;
    r.left = action.first;
    r.right = action.second;
    r.picked = bed.source[action.hand];
    r.opened = action.opened && (c.tracer ? mentor.limbs : mentor.mouth) &&
               r.guide_looks + (c.tracer ? 2 : 3) <= mentor.energy;
    r.spoken_order = (genes.first >> 18) & 1;
    r.received_order = (child.genes.first >> 18) & 1;
    if (r.opened && c.tracer) {
        r.release_from = r.picked;
        r.release_to = braid::port(bed.map, r.release_from);
        r.work = 1;
    } else if (r.opened) {
        r.said_from = braid::pigment(mentor.eye.eyes.places, r.picked);
        r.said_to = action.hand ? b.voice : a.voice;
        r.first_wave = utter(mentor, r.spoken_order ? r.said_to : r.said_from);
        r.second_wave = utter(mentor, r.spoken_order ? r.said_from : r.said_to);
        r.work = Word(r.first_wave != 0) + Word(r.second_wave != 0);
    }
    r.before = pupil.contacts;
    r.starting_spikes = pupil.spikes;
    bool live = c.arm != muted && c.arm != hidden && child.energy > r.pupil_looks + 1;
    bool exchange = r.received_order ^ Word(c.arm == reversed);
    r.heard_from = hear(child, live ? (exchange ? r.second_wave : r.first_wave) : 0);
    r.heard_to = hear(child, live ? (exchange ? r.first_wave : r.second_wave) : 0);
    if (c.calibrated) {
        Word first = hear(child, live ? (c.arm == reversed ? r.second_wave : r.first_wave) : 0);
        Word second = hear(child, live ? (c.arm == reversed ? r.first_wave : r.second_wave) : 0);
        translate(c.dialect, first, second, r.heard_from, r.heard_to);
    }
    auto previous = pupil;
    if (c.tracer && r.opened && live) {
        view(child, pupil, glint(child, r.release_from, c.arm == reversed ? (r.release_to ^ 1u) : r.release_to));
        r.taught = 1;
    } else if (r.heard_from < 4 && r.heard_to < 4) {
        // The two decoded sounds excite the pupil's own local position
        // populations in sequence. No projection from the expert's weights.
        view(child, pupil, retina::reflected(255, 0, r.heard_from, r.heard_to));
        r.taught = 1;
    }
    r.after = pupil.contacts;
    r.pupil_spikes = pupil.spikes;
    for (Word j = 0; j < 4; ++j) {
        Word tie = mix(bed.key + j * 8731 + 3719);
        auto prior = expect(child, previous, j, c.ports, tie), final = expect(child, pupil, j, c.ports, tie);
        r.prior_aim |= prior.aim << (2 * j);
        r.final_aim |= final.aim << (2 * j);
        r.prior_voice |= (utter(child, prior.voice) ? prior.voice : 4) << (3 * j);
        r.final_voice |= (utter(child, final.voice) ? final.voice : 4) << (3 * j);
    }
    return r;
}
AX_FN Catch release(const Pair &pair, const Layout &bed, const Contact &r) {
    Catch out{};
    out.fresh = (r.release_from < 4 || (r.first_wave && r.second_wave)) && !(r.viewed & (1u << r.picked));
    auto goal = braid::pigment(pair.guide.eye.eyes.places, braid::port(bed.map, r.picked));
    out.truthful = r.first_wave && r.second_wave && r.said_to == goal;
    for (Word j : bed.source) {
        Word to = braid::port(bed.map, j), name = braid::pigment(pair.pupil.eye.eyes.places, to);
        out.prior += braid::port(r.prior_aim, j) == to;
        out.final += braid::port(r.final_aim, j) == to;
        out.voiced_before += ((r.prior_voice >> (3 * j)) & 7) == name;
        out.voiced_after += ((r.final_voice >> (3 * j)) & 7) == name;
    }
    return out;
}
struct Practice {
    Alignment learned{};
    Word offers = 0, paired = 0, moved = 0, writes = 0, guide_heat = 0, pupil_heat = 0, pictures = 0, heard = 0,
         grains = 0;
    TissueState guide{}, pupil{};
    Word tape = 0;
};
struct Demonstration {
    Word first, second, source, dest, opening, ending, before_a, before_b, after_a, after_b;
};
AX_FN Practice calibrate(Pair p, Stock stock, Word seed, Word rounds, Word pairing, Alignment initial,
                         Word guide_budget, Word pupil_budget, Demonstration *events) {
    Practice r{};
    r.learned = initial;
    stock.rounds = 1;
    stock.arm = muted;
    stock.calibrated = 0;
    stock.tracer = 0;
    stock.seed = seed;
    stock.access = split;
    r.guide = {p.guide.eye.contacts, p.guide.eye.spikes};
    r.pupil = {p.pupil.eye.contacts, p.pupil.eye.spikes};
    r.tape = p.guide.eye.gate_b >> 16;
    for (Word j = 0; j + 1 < rounds; j += 2) {
        auto old_guide = r.guide, old_pupil = r.pupil;
        Word old_tape = r.tape;
        Word first[2], second[2], from[2], to[2], guide_cost = 0, pupil_cost = 0;
        for (Word t = 0; t < 2; ++t) {
            p.guide.eye.contacts = r.guide.contacts;
            p.guide.eye.spikes = r.guide.spikes;
            p.guide.eye.gate_b = (p.guide.eye.gate_b & 65535u) | (r.tape << 16);
            p.pupil.eye.contacts = r.pupil.contacts;
            p.pupil.eye.spikes = r.pupil.spikes;
            auto bed = water(stock, j + t);
            auto c = encounter(p, p.guide.genes, stock, bed, (bed.key >> 17) & 1);
            first[t] = hear(p.pupil, c.first_wave);
            second[t] = hear(p.pupil, c.second_wave);
            from[t] = c.opened && p.guide.limbs ? c.picked : 4;
            to[t] = from[t] < 4 ? braid::port(bed.map, from[t]) : 4;
            guide_cost += c.guide_looks + c.work + Word(from[t] < 4);
            pupil_cost += c.pupil_looks + Word(first[t] < 4 && second[t] < 4) + Word(from[t] < 4);
            r.guide = {c.guide_contacts, c.guide_spikes};
            r.tape = c.tape;
            r.pupil = {c.before, c.starting_spikes};
            if (from[t] < 4)
                view(p.pupil, r.pupil, glint(p.pupil, from[t], to[t]));
        }
        if (r.guide_heat + guide_cost > guide_budget || r.pupil_heat + pupil_cost > pupil_budget) {
            r.guide = old_guide;
            r.pupil = old_pupil;
            r.tape = old_tape;
            break;
        }
        r.guide_heat += guide_cost;
        r.pupil_heat += pupil_cost;
        r.offers += 2;
        for (Word t = 0; t < 2; ++t) {
            Word k = t ^ Word(pairing == 1);
            auto before = r.learned;
            Word opening = 0, ending = 0;
            if (from[t] < 4) {
                auto old = r.learned;
                auto light = glint(p.pupil, from[t], to[t]);
                opening = retina::contrast(light.rest, light.onset, p.pupil.eye.genes.receptors);
                ending = retina::contrast(light.rest, light.end, p.pupil.eye.genes.receptors >> 2);
                if (pairing != 2)
                    couple(r.learned, p.pupil, first[k], second[k], light);
                r.writes += __builtin_popcount(old.bank[0] ^ r.learned.bank[0]) +
                            __builtin_popcount(old.bank[1] ^ r.learned.bank[1]);
                r.paired += Word(first[k] < 4 && second[k] < 4);
                ++r.grains;
            }
            if (events)
                events[j + t] = {first[k],          second[k],        from[t],        to[t],
                                 opening,           ending,           before.bank[0], before.bank[1],
                                 r.learned.bank[0], r.learned.bank[1]};
            r.moved += from[t] != to[t];
            r.pictures = mix(r.pictures ^ (from[t] + 8 * to[t] + j * 31 + t));
            r.heard = mix(r.heard ^ (first[t] + 8 * second[t] + j * 31 + t));
        }
    }
    return r;
}
enum Act : Word { meet, load, ask, pour };
struct Table {
    Layout bed{};
    Contact contact{};
    Word used = 0, held = 0, source = 0, dye = 0, ports = 0, bites = 0, settled = 0;
};
struct Request {
    Stock stock{};
    Word act = meet, life = 0, seat = 0, inlet = 0, who = 1;
    Count noun = 0;
    braid::Speech words{};
    const retina::Surface *specimen{};
};
struct Reply {
    Word status = 0, first = 0, second = 0, viewed = 0, picked = 0, face = 0, changed = 0;
    Word paid_guide = 0, paid_pupil = 0, heard = 0, held = 0, stock = 0, bites = 0, settled = 0, eaten = 0, outlet = 4;
    braid::Speech words{};
};
} // namespace axion::life::audience
