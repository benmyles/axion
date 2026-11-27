#include "linkage_design.cuh"
#include "linkage_physics.cuh"
#include "linkage_cells.cuh"
namespace axion::life::linkage {
AX_FN Word physical_code(Material m, bool gate, Word form) {
    Word map = 0;
    for (Word pose = 0; pose < 4; ++pose)
        for (Word a = 0; a < 2; ++a) {
            Word f = pigment(m, pose), row = f + 4 * a,
                 newface = gate ? transmission(m, form, f, a) : pigment(m, (pose + ((a ^ m.reverse) ? 3 : 1)) & 3);
            map |= (newface << (2 * row)) | (1u << (16 + row));
        }
    return map;
}
AX_FN Map true_map(const Material *law) {
    Map m{};
    for (Word i = 0; i < 4; ++i) {
        m.next[i] = physical_code(law[i], false, 0);
        for (Word j = 0; j < 2; ++j)
            m.link[j][i] = physical_code(law[i], true, j + 1);
    }
    for (Word g = 0; g < 4; ++g)
        for (Word k = 0; k < 3; ++k)
            m.splice[g] |= (thread(law[g], k) << (2 * (4 + k))) | (1u << (20 + k));
    return m;
}
AX_FN Word encounter(Mind &m, Slots s, Rig r, Word &angle, Word action, const Material *materials, bool witness,
                     Word mode) {
    Word before = expose(r, angle, materials);
    angle = press(r, angle, action, materials);
    return learn(m, s, {r, before}, action, expose(r, angle, materials), witness, mode);
}
AX_FN Word tuition(Mind &m, Slots slots, const Material *law, Word kinds, Word seed, Word treatment, Word repeats,
                   bool pairing = true) {
    bool social = treatment == watched || treatment == wrong_witness || treatment == private_hands;
    Word mode = writable | witnesses, count = 0;
    if (treatment == unschooled || treatment == frozen)
        return 0;
    if (treatment == private_hands)
        mode &= ~Word(witnesses);
    if (treatment == wrong_witness)
        mode |= other_eye;
    Material prior[4];
    for (Word g = 0; g < 4; ++g) {
        prior[g] = law[g];
        prior[g].notches ^= 15;
        prior[g].mesh ^= 3;
    }
    if (treatment == autonomous) {
        for (Word epoch = 0; epoch < repeats * 4; ++epoch)
            for (Word driver = 0; driver < 4; ++driver)
                if (kinds & (1u << driver)) {
                    Word random = mix(seed + epoch * 271 + driver * 1777), other = bump(kinds, random >> 8);
                    Rig r{3, driver | (other << 2), 0, 15};
                    if (epoch >= 2)
                        join(r, 0, 1, 1 + (epoch & 1));
                    Word pose = random & 15;
                    for (Word n = 0; n < 8; ++n) {
                        Word a = probe_motor(m, slots, r, expose(r, pose, law), mix(random + 37 * n));
                        encounter(m, slots, r, pose, a, law, false, mode);
                        ++count;
                        if (n == 3) {
                            Word old = joint(r.joints, 0, 1), fresh = thread(law[driver], old);
                            join(r, 0, 1, fresh);
                            learn_join(m, slots, driver, old, true, fresh, false, mode);
                        }
                    }
                }
        return count;
    }
    for (Word epoch = 0; epoch < repeats; ++epoch)
        for (Word k = 0; k < 32; ++k) {
            Word q = (k * ((seed & 7) * 2 + 1) + (seed >> 18) + epoch) & 31, g = q >> 3;
            if (!(kinds & (1u << g)))
                continue;
            Rig r{1, g, 0, 3};
            Word pose = q & 3, a = (q >> 2) & 1;
            encounter(m, slots, r, pose, a, law, social, treatment == joined_only ? 0u : mode);
            ++count;
        }
    if (!pairing)
        return count;
    for (Word epoch = 0; epoch < repeats; ++epoch)
        for (Word k = 0; k < 64; ++k) {
            Word q = (k * (1 + 2 * ((seed >> 8) & 31)) + (seed >> 4) + epoch) & 63, g = q >> 4;
            if (!(kinds & (1u << g)))
                continue;
            Word salt = mix(seed + q * 1009 + epoch * 43), other = bump(kinds, salt >> 4), a = (q >> 2) & 1;
            Rig r{3, g | (other << 2), 0, 15};
            join(r, 0, 1, 1 + ((q >> 3) & 1));
            Word pose = (q & 3) | (salt & 12);
            const Material *actual = treatment == prior_gates ? prior : law;
            Word initial = ((q + epoch) >> 1) % 3;
            learn_join(m, slots, g, initial, true, thread(actual[g], initial), social, mode);
            learn_join(m, slots, g, thread(actual[g], initial), false, initial, social, mode);
            encounter(m, slots, r, pose, a, actual, social, mode);
            ++count;
        }
    return count;
}
struct Job {
    Rig rig{};
    Word angle = 0;
    Pattern goal{};
};
AX_FN Job order(Word seed, Word width, const Material *law, Word form = 0) {
    Job q{};
    q.rig = layout(seed, width, width > 1 && form != 1);
    Word k = mix(seed + 13217);
    q.angle = (k >> 16) & mask_parts(q.rig.parts);
    Word result = q.angle;
    Word mask = mask_parts(q.rig.parts);
    Word length = 1 + (mix(seed + 6007) % (2 + 2 * width));
    for (Word j = 0; j < length; ++j)
        result = press(q.rig, result, bump(q.rig.handles, mix(k + 229 * j)), law);
    if (result == q.angle)
        result = press(q.rig, result, bump(q.rig.handles, k), law);
    q.goal = {expose(q.rig, result, law), mask};
    if ((k & 7) == 0 || form == 2)
        q.goal.faces = (k >> 7) & mask;
    if ((k & 31) == 1 && width > 1)
        q.goal.care &= ~(3u << (2 * ((k >> 12) % width)));
    return q;
}
AX_FN Word types(Rig r) {
    Word kinds = 0;
    for (Word i = 0; i < 4; ++i)
        if (r.parts & (1u << i))
            kinds |= 1u << bits(r.glyphs, i);
    return kinds;
}
AX_FN void modify(Map &v, Word treatment) {
    for (Word g = 0; g < 4; ++g) {
        if (treatment == local_erased)
            v.next[g] = 0;
        if (treatment == seams_erased)
            v.link[0][g] = v.link[1][g] = 0;
        if (treatment == displaced) {
            Word temp = v.link[0][g];
            v.link[0][g] = v.link[1][g];
            v.link[1][g] = temp;
        }
    }
}
AX_FN Trial solve(Mind &m, Slots s, Job q, const Material *law, Word seed, Word treatment, bool grade,
                  Receipt *receipt) {
    auto chart = compile(m, s);
    modify(chart, treatment);
    if (treatment == blind_tools)
        q.rig.handles = 0;
    Rig seen = q.rig;
    Word before = expose(q.rig, q.angle, law);
    if (treatment == destructive)
        seen.joints = 0;
    auto plan = seek(chart, seen, before, q.goal, m.genes);
    Word angle = q.angle;
    for (Word i = 0; i < plan.course.len; ++i)
        angle = press(q.rig, angle, step(plan.course, i), law);
    Word after = expose(q.rig, angle, law);
    Trial v{};
    v.n = 1;
    v.solved = matches(after, q.goal);
    v.food = v.solved ? 32 : 0;
    v.taps = plan.course.len;
    v.refused = !plan.found;
    v.false_plan = plan.found && !v.solved;
    v.offered = plan.found;
    v.proposals = plan.expanded;
    for (Word i = 0; i < 4; ++i)
        if ((q.rig.parts & (1u << i)) && !(q.rig.handles & (3u << (2 * i))) && ((q.goal.care >> (2 * i)) & 3) &&
            bits(before, i) != bits(q.goal.faces, i)) {
            v.indirect = 1;
            break;
        }
    v.indirect_solved = v.indirect * v.solved;
    v.active = __builtin_popcount((m.tags | (m.tags >> 1) | (m.tags >> 2)) & 0x1111);
    if (grade) {
        auto reality = true_map(law);
        Genes full = m.genes;
        full.impulse |= 15u << 8;
        auto optimum = seek(reality, q.rig, before, q.goal, full);
        v.possible = optimum.found;
        v.best = optimum.course.len;
        v.optimal = v.solved && v.taps == v.best;
        v.multi = optimum.found && v.best >= 3;
        v.multi_solved = v.solved * v.multi;
        for (Word mask = q.rig.handles; mask; mask &= mask - 1) {
            Word a = __builtin_ctz(mask), pred = predict(chart, seen, before, a);
            ++v.forecast;
            v.exact += pred == expose(q.rig, press(q.rig, q.angle, a, law), law);
            v.against += pred != absent;
        }
        if (receipt) {
            *receipt = {};
            receipt->seed = seed;
            receipt->width = __builtin_popcount(q.rig.parts);
            receipt->parts = q.rig.parts;
            receipt->start = before;
            receipt->desired = q.goal.faces;
            receipt->care = q.goal.care;
            receipt->handles = q.rig.handles;
            receipt->joints = q.rig.joints;
            receipt->glyphs = q.rig.glyphs;
            receipt->finish = after;
            receipt->g = m.genes;
            receipt->map = chart;
            receipt->plan = plan.course;
            receipt->reached = v.solved;
            receipt->spent = v.taps;
            receipt->shortest = optimum.found ? v.best : ~0u;
            receipt->depth = horizon(m.genes);
            receipt->claims = plan.found;
            for (Word g = 0; g < 4; ++g) {
                receipt->true_next[g] = reality.next[g];
                receipt->true_links[0][g] = reality.link[0][g];
                receipt->true_links[1][g] = reality.link[1][g];
            }
        }
    }
    return v;
}
AX_FN Trial newborn_trial(Genes g, Word width, Word seed, Word treatment, Word repeats, Word cap, bool grade,
                          Receipt *receipt) {
    Mind m{g};
    Tissue store[4]{};
    Slots s{store, 1, treatment == one_context ? 1u : cap};
    Material laws[4];
    for (Word j = 0; j < 4; ++j)
        laws[j] = matter(seed, j);
    auto q = order(mix(seed + 527), width, laws);
    Word touched = tuition(m, s, laws, types(q.rig), seed, treatment, repeats);
    auto v = solve(m, s, q, laws, seed, treatment, grade, receipt);
    v.observed = touched;
    return v;
}
} // namespace axion::life::linkage
KERNEL void linkage_exams(const linkage::Genes *g, Word ng, Word n, Word width, Word seed, Word mode, Word lessons,
                          Word cap, linkage::Trial *rows, linkage::Receipt *example) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 11779);
    rows[i] = linkage::newborn_trial(g[mix(k + 521) % ng], width, k, mode, lessons, cap, true,
                                     i < 3 ? example + i : nullptr);
}
#include "siphon_linkage_design.cuh"
KERNEL void linkage_feed(linkage::Family *pop, Word n, Word seed, Word epoch, Word cap) {
    Word i = tid();
    if (i >= n)
        return;
    auto &p = pop[i];
    Word energy = 24;
    for (Word j = 0; j < 6; ++j) {
        Word k = mix(seed + (i / 2) * 22621 + epoch * 21557 + j * 1783), width = 1 + (j >= 1) + (j >= 4);
        Word food = 0, cost = 0;
        if (j >= 4) {
            auto t = linkage::build(p.g, k, width, j & 1 ? linkage::public_shop : linkage::assembled, cap, 1, false,
                                    nullptr);
            food = t.carbon;
            cost = t.paid;
        } else {
            auto t = linkage::newborn_trial(p.g, width, k, j & 1 ? linkage::watched : linkage::own, 1, cap, false,
                                            nullptr);
            food = t.food;
            cost = t.taps;
        }
        energy += food;
        energy -= energy > 5 + cost ? 5 + cost : energy;
    }
    p.eggs = 1 + energy / 8;
}
KERNEL void linkage_births(const linkage::Family *old, linkage::Family *brood, Word n, Word seed, Word epoch,
                           Word paid) {
    Word i = tid();
    if (i >= n / 2)
        return;
    Word k = mix(seed + 9131 * epoch), hop = mix(k + 359) | 1u, x = i * 2, y = x + 1;
    Word wallet[2]{old[x].eggs, old[y].eggs};
    for (Word j = 0; j < 2; ++j) {
        Word v = mix(k + i * 12719 + j * 19), b = paid ? Word((v % (wallet[0] + wallet[1])) >= wallet[0]) : v & 1;
        if (paid)
            --wallet[b];
        auto g = old[b ? y : x].g, other = old[b ? x : y].g;
        if ((g.arbor & 128) && (v & 7) == 0) {
            Word mask = ~0u << ((v >> 9) & 31);
            if (v & 256)
                g.binding = (g.binding & mask) | (other.binding & ~mask);
            else
                g.arbor = (g.arbor & mask) | (other.arbor & ~mask);
        }
        brood[(k + (2 * i + j) * hop) & (n - 1)] = {linkage::child(g, mix(v + 617), 3 + ((g.impulse >> 24) & 31)), 0};
    }
}
KERNEL void linkage_candidates(const linkage::Genes *g, Word n, Word seed, Word cap, Word *scores) {
    Word i = tid();
    if (i >= n)
        return;
    Word score = 0;
    for (Word j = 0; j < 24; ++j) {
        auto r = linkage::newborn_trial(g[i], 2 + j % 3, mix(seed + j * 2453),
                                        j & 1 ? linkage::watched : linkage::own, 2, cap, false, nullptr);
        score += r.solved * (32 - r.taps);
    }
    for (Word j = 0; j < 12; ++j) {
        auto r = linkage::build(g[i], mix(seed + 29173 + j * 4567), 3 + j % 2,
                                j & 1 ? linkage::public_shop : linkage::assembled, cap, 2, false, nullptr);
        score += r.achieved * (40 - r.paid);
    }
    scores[i] = score;
}
KERNEL void linkage_periphery(const linkage::Genes *genes, Word ng, Word n, Word seed, Word cap, Word mode,
                              linkage::Peripheral *out) {
    using namespace linkage;
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 8543);
    auto g = genes[mix(k + 379) % ng];
    linkage::Mind m{g};
    linkage::Tissue bank[4]{};
    Slots s{bank, 1, cap};
    Material material[4];
    for (Word j = 0; j < 4; ++j)
        material[j] = matter(k, j);
    Rig rack{15, permute(k), 0, 3};
    join(rack, 0, 1, 1);
    join(rack, 0, 2, 1 + ((k >> 9) & 1));
    join(rack, 1, 3, 2);
    Word angle = (k >> 12) & 255, first = expose(rack, angle, material);
    tuition(m, s, material, (1u << bits(rack.glyphs, 0)) | (1u << bits(rack.glyphs, 1)), k, own, 2);
    Word future = angle;
    for (Word j = 0; j < 1 + (k % 3); ++j)
        future = linkage::press(rack, future, k & 1, material);
    Pattern goal{expose(rack, future, material), 15};
    auto model = compile(m, s);
    Word express = g.arbor & (1u << 27);
    if (mode == 2 || mode == 3)
        g.arbor |= 1u << 27;
    if (mode == 3)
        goal.care = 255;
    auto plan = attend(model, rack, first, goal, g, mode != 0);
    auto ideal = seek(true_map(material), rack, first, goal, g);
    Word actual = angle;
    for (Word j = 0; j < plan.course.len; ++j)
        actual = linkage::press(rack, actual, step(plan.course, j), material);
    bool solved = matches(expose(rack, actual, material), goal);
    out[i] = {1,
              !!express,
              plan.found,
              solved,
              plan.course.len,
              plan.found && !solved,
              solved && plan.course.len == ideal.course.len,
              first != expose(rack, actual, material)};
}
