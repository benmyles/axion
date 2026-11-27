#include "axion/braid_culture.hpp"
namespace axion::life::braid {
struct Usage {
    unsigned char law[voices]{};
    Word seen = 0, clashes = 0;
};
AX_FN void observed(Usage &u, Word tone, Word effect) {
    Word bit = 1u << tone;
    if ((u.seen & bit) && u.law[tone] != effect)
        u.clashes |= bit;
    else
        u.law[tone] = effect;
    u.seen |= bit;
}
AX_FN Word examples(Mind &adult, Mind &pupil, Word adult_rig, Word key, Word visits, Word generation, Word arm,
                    Usage &speech, Word &coins, Word &work, Word &heard) {
    Word spoken = 0;
    for (Word k = 0; k < visits; ++k) {
        Word d = mix(key + k * 1733), screen = shade(d, k), motor = (d >> 5) & 3;
        if (arm == uniform_cups)
            screen = (d & 3) * 85;
        Word tone = none, truth = order, end = screen;
        bool deliberate = generation && (adult.genes.throat & (1u << 22)) && (d & 3) != 0;
        if (deliberate) {
            Word candidate = (d >> 8) & 7;
            if (adult.lived & (1u << (motors + candidate))) {
                auto target = predict(matrix(adult, motors + candidate), screen);
                auto plan = planning(adult, screen, target.colors, target.known, depth(adult.genes));
                if (target.known == 15 && plan.found && plan.size) {
                    truth = realize(order, plan, adult.genes, adult_rig, 15);
                    end = transfer(screen, truth);
                    tone = candidate;
                    work += plan.size;
                }
            }
        }
        if (tone == none) {
            truth = valve(order, motor, adult.genes, adult_rig, 15);
            end = transfer(screen, truth);
            ++work;
            motor_touch(adult, motor, screen, end, false);
            Word fresh = 0;
            tone = term(adult, screen, end, motor, d, true, fresh);
            coins += fresh;
        }
        if (tone == none || tone >= voices)
            continue;
        ++spoken;
        observed(speech, tone, truth);
        auto q = phrase(tone, 1);
        listen(adult, q, screen, end);
        if (arm != imperceptible && arm != solitary) {
            Word view = arm == distant ? shade(mix(d + 881), k) : end;
            listen(pupil, q, screen, view);
            ++heard;
        }
    }
    return spoken;
}
struct Community {
    Mind resident{}, parent{};
    Usage initial{}, last{};
    Word coins = 0, spoken = 0, heard = 0, work = 0, practised = 0, rig = 0, retained[8]{};
};
AX_FN Community upbringing(Genes a, Genes b, Genes c, Word key, Word twin, Word gen, Word visits, Word arm) {
    Community h{};
    Mind adult{a};
    Word rig = new_rig(key, 0);
    for (Word i = 0; i < gen; ++i) {
        Mind pupil{i + 1 == gen ? c : (i & 1 ? a : b)};
        Word pupil_rig = new_rig(mix(key + i * 2003 + 3907), 0);
        if (arm != no_practice) {
            drill(pupil, pupil_rig, 3, mix(key + i * 8161 + 6211), learned);
            h.practised += 12;
        }
        Usage seen{};
        // Only the founder's order of physical experiences differs in the twins.
        // Every germ, subsequent district, observer and available muscle is matched.
        Word variation = i == 0 ? twin * 20431 : 0;
        h.spoken += examples(adult, pupil, rig, mix(key + i * 7867 + variation), visits, i, arm, seen, h.coins, h.work,
                             h.heard);
        h.parent = adult;
        if (arm == solitary) {
            pupil = adult;
            pupil_rig = rig;
        }
        if (!i)
            h.initial = seen;
        h.last = seen;
        Word keep = 0;
        for (Word j = 0; j < voices; ++j) {
            Word bit = 1u << j;
            if (!((h.initial.seen & ~h.initial.clashes) & bit))
                continue;
            auto belief = predict(matrix(pupil, motors + j), order);
            keep += belief.known == 15 && belief.colors == h.initial.law[j];
        }
        if (i < 8)
            h.retained[i] = keep;
        adult = pupil;
        rig = pupil_rig;
    }
    h.resident = adult;
    h.rig = rig;
    return h;
}
AX_FN Word challenge(const Community &h, Word a, Word b, Word arm, Word &fund, Invention::Outcome &out) {
    Word mask = (1u << a) | (1u << b), consensus = h.initial.seen & ~h.initial.clashes;
    if ((consensus & mask) != mask)
        return 0;
    out.available = 1;
    auto p = phrase(a | (b << 3), 2);
    Word wanted = transfer(h.initial.law[a], h.initial.law[b]);
    out.wanted = wanted;
    if (arm == switch_words) {
        p.cell[0] = (p.cell[0] + 1) & 7;
        p.cell[1] = (p.cell[1] + 1) & 7;
    }
    auto r = predict(pathway(h.resident, p), order);
    out.received = r.known == 15 && r.colors == wanted;
    auto act = planning(h.resident, order, r.colors, r.known, depth(h.resident.genes));
    out.end = realize(order, act, h.resident.genes, h.rig, 15);
    out.strokes = act.size;
    out.motor = act.stream;
    fund += harvest(out.end, wanted, 8).swallowed;
    return out.received && out.end == wanted;
}
// The remote sieve is set before anyone speaks. The first body can see the
// aperture and issue pulses. The second body sees only its floats and those pulses.
AX_FN RequestWork signal_filter(const Community &h, Word sockets, Word arm) {
    RequestWork out{};
    auto words = release(h.parent, true);
    auto call = route(words, order, sockets, 15, depth(h.parent.genes));
    out.available = call.found;
    if (!call.found || !call.size) {
        out.after = order;
        out.food = harvest(order, sockets, 8).swallowed;
        return out;
    }
    auto p = phrase(0, call.size);
    for (Word j = 0; j < call.size; ++j)
        p.cell[j] = (call.stream >> (3 * j)) & 7;
    p = sequence(p, reversed(h.parent));
    out.spoken = p.n;
    out.phrase = phrase_code(p);
    if (arm == switch_words)
        for (Word j = 0; j < p.n; ++j)
            p.cell[j] = (p.cell[j] + 1) & 7;
    bool known = true;
    for (Word j = 0; j < p.n; ++j)
        known &= (h.resident.lived & (1u << (motors + p.cell[j]))) != 0;
    if (known && arm != imperceptible) {
        auto target = predict(pathway(h.resident, p), order);
        out.understood = target.known == 15;
        if (out.understood) {
            auto act = planning(h.resident, order, target.colors, 15, depth(h.resident.genes));
            out.pulls = act.size;
            out.sequence = act.stream;
            out.after = realize(order, act, h.resident.genes, h.rig, 15);
            out.exact = out.after == sockets;
            out.changed = out.after != order;
        }
    }
    out.food = harvest(out.after, sockets, 8).swallowed;
    return out;
}
KERNEL void braid_cultures(const Genes *stock, Word size, Word n, Word seed, Word gen, Word visits, Word arm,
                           FilterVillage *out) {
    Word i = tid();
    if (i >= n)
        return;
    Word key = mix(seed + i * 6173);
    FilterVillage record{};
    auto &log = record.workshop;
    for (Word j = 0; j < 3; ++j)
        log.genes[j] = stock[mix(key + 1709 + j * 571) % size];
    for (Word t = 0; t < 2; ++t) {
        auto h = upbringing(log.genes[0], log.genes[1], log.genes[2], key, t, gen, visits, arm);
        for (Word w = 0; w < voices; ++w) {
            log.first[t][w] = h.initial.law[w];
            log.final[t][w] = h.last.law[w];
        }
        log.consistent[t] = h.initial.seen & ~h.initial.clashes;
        log.familiar[t] = h.last.seen & ~h.last.clashes;
        log.coined[t] = h.coins;
        log.emitted[t] = h.spoken;
        log.shown[t] = h.heard;
        log.practised[t] = h.practised;
        log.paid[t] = h.work;
        log.muscle[t] = h.resident.genes.muscle;
        log.rig[t] = h.rig;
        for (Word j = 0; j < 8; ++j)
            log.stage[t][j] = h.retained[j];
        Word different = 0;
        for (Word w = 0; w < voices; ++w)
            if (log.consistent[t] & (1u << w))
                different |= 1u << address_of(h.initial.law[w]);
        record.distinct[t] = __builtin_popcount(different);
        for (Word goal = 0; goal < 24; ++goal)
            record.requests[t][goal] = signal_filter(h, assembly(goal), arm);
        Word idx = 0;
        for (Word a = 0; a < voices; ++a)
            for (Word b = 0; b < voices; ++b)
                if (a != b) {
                    log.true_voices[t] += challenge(h, a, b, arm, log.food[t], log.cases[t][idx++]);
                }
    }
    out[i] = record;
}
KERNEL void braid_community_food(Family *stock, Word n, Word epoch, Word seed, Word visits, Word filters) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + epoch * 6553), next = 1 + mix(k + 29) % (n - 1), share = 0;
    for (Word role = 0; role < 2; ++role) {
        Word elder = role ? (i + n - next) % n : i, young = (elder + next) % n, world = mix(k + elder * 1831);
        auto h = upbringing(stock[elder].genes, stock[young].genes, stock[young].genes, world, 0, 1, visits, lineage);
        Word delivered = 0;
        if (filters) {
            for (Word goal = 0; goal < 24; ++goal)
                delivered += signal_filter(h, assembly(goal), lineage).food;
        } else
            for (Word a = 0; a < voices; ++a)
                for (Word b = 0; b < voices; ++b)
                    if (a != b) {
                        Invention::Outcome target{};
                        challenge(h, a, b, lineage, delivered, target);
                    }
        // Solar input fills eight-unit filter cups. The heard program alone
        // selects the remote configuration. The two ends share its discharge.
        share += delivered / 2;
    }
    stock[i].food = share;
}
} // namespace axion::life::braid

namespace axion::life::braid {
KERNEL void braid_graduate(const Genes *stock, Word size, Word index, Word seed, Word gen, Word visits, Word twin,
                           Mind *trained, Word *facts) {
    Word key = mix(seed + index * 6173), adult = mix(key + 1709) % size, second = mix(key + 2280) % size,
         child = mix(key + 2851) % size;
    auto h = upbringing(stock[adult], stock[second], stock[child], key, twin, gen, visits, lineage);
    *trained = h.resident;
    facts[0] = child;
    facts[1] = h.rig;
    facts[2] = h.initial.seen & ~h.initial.clashes;
    for (Word w = 0; w < voices; ++w)
        facts[3 + w] = h.initial.law[w];
}
} // namespace axion::life::braid
