#include "runnel_physics.cuh"
#include "runnel_analogy.cuh"
using namespace axion;
using namespace axion::life::runnel;
#define KERNEL extern "C" __attribute__((global))
namespace {
AX_FN Word tid() {
    Word b, t, n;
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(b));
    asm("mov.u32 %0, %%tid.x;" : "=r"(t));
    asm("mov.u32 %0, %%ntid.x;" : "=r"(n));
    return b * n + t;
}
AX_FN Word reflect(Word mask, Word mirrored) { return mirrored ? ((mask >> 1) | ((mask & 1) << 1)) : mask; }
AX_FN Word surface(Word i, Word permutation, Word domain) {
    Word stride = (permutation >> 4) | 1, tag = i * stride + permutation;
    return domain < 2 ? ((tag & 7) << 1) | domain : tag & 15;
}
AX_FN Word hand(const Mind &m, Word scramble) { return (m.genes.hand ^ scramble) & 7; }
enum Arm : Word {
    full,
    alone,
    obscured,
    displaced,
    echo,
    newcomers,
    one_bank,
    other_bank,
    no_play,
    only_play,
    blocked,
    no_bell,
    bare,
    flipped,
    last_only,
    arms
};
AX_FN void lesson(Mind &student, Word frame, const Mind *adult, Word model_frame, Word stone, Word list_seed,
                  Word visit, Word domain, Word lectures, Word solo, Word arm, Transmission &out, Word invited = 0,
                  Word reasoning = 0) {
    Word mode = live | (arm == displaced ? Word(scrambled) : 0u);
    for (Word i = 0; i < lectures + solo; ++i) {
        bool peer = i < lectures;
        Word n = peer ? i : i - lectures;
        Word permutation = mix(list_seed + (n / 8) * 18587 + (peer ? 451 : 4127));
        Word cue = surface(n, permutation, domain);
        Word k = mix(visit + i * 6421), dir = (k >> 13) & 1;
        if (peer && invited) {
            // Four accessible work surfaces are visible. The pupil can invite a
            // demonstration where its own motor prediction is still ambiguous.
            for (Word alt = 0; alt < 4; ++alt) {
                auto ready = infer(student, cue, dir, k, false, reasoning).action;
                if (!ready.known || ready.expected != (1u << dir))
                    break;
                Word z = mix(k + alt * 859);
                cue = surface(n + 1 + alt, z, domain);
                dir = (z >> 30) & 1;
            }
        }
        bool masked = peer && arm == obscured;
        if (peer && arm == alone)
            peer = false;
        if (!peer && arm == no_play)
            continue;
        auto &model = peer ? *adult : student;
        Word active_frame = peer ? model_frame : frame;
        auto pick = infer(model, cue, dir, k, !peer, reasoning).action;
        Word gesture = pose(pick.motor, active_frame), result = flex(stone, cue, gesture);
        if (peer) {
            ++out.demonstrated;
            if (masked)
                continue;
            ++out.social;
        } else
            ++out.own;
        // With the screen closed, an assistant still spends the same paid strikes.
        Genes saved = student.genes;
        if (peer && arm == echo)
            student.genes.eye &= ~16u;
        Word watched = peer && arm == only_play ? cue ^ 5u : cue;
        bool edited =
            observe(student, watched, gesture, frame, result, peer, mode, peer && arm == last_only ? 1u << dir : 3u);
        student.genes = saved;
        if (peer) {
            out.social_writes += edited;
            out.changed_hand += native(gesture, frame) != pick.motor;
        } else
            out.own_writes += edited;
    }
}
AX_FN Gallery canal(const Mind &builder, Word frame, Word stone, Word plan, Word length, Word ticks, Word arm,
                    Transmission *record, Word reason, Byte *drawing = nullptr) {
    Byte gates[max_span];
    for (Word j = 0; j < length; ++j) {
        Word k = mix(plan + j * 28657), cue = (k >> 19) & 15, d = (k >> 8) & 1;
        auto decision = infer(builder, cue, d, k, false, reason).action;
        Word gesture = pose(decision.motor, frame);
        Word mask = reflect(contact(stone, cue, gesture), d);
        if (arm == blocked && j == length / 2)
            mask = 0;
        if (arm == bare)
            mask = 3;
        if (arm == flipped)
            mask = reflect(mask, 1);
        gates[j] = mask;
        if (drawing)
            drawing[j] = Byte(cue | (d << 4) | (gesture << 5));
        if (record) {
            ++record->folds;
            record->recalled += decision.known;
            record->social_use += decision.from_peer;
            record->mastered += mask == 1;
        }
    }
    return pump(gates, length, ticks, arm != no_bell);
}
AX_FN Transmission community(const Genes *stock, Word total, Word seed, Word stone, Word generations, Word span,
                             Word arm, Word budget, Word contact_time, Word invited, Word reason, Byte *shape,
                             Mind *heir) {
    Transmission record{};
    Mind original[2]{}, elders[2]{};
    Word frames[2]{};
    for (Word j = 0; j < 2; ++j) {
        elders[j].genes = stock[mix(seed + j * 5813) % total];
        frames[j] = hand(elders[j], mix(seed + 1251 + j * 3));
        original[j] = elders[j];
    }
    for (Word g = 0; g < generations; ++g) {
        for (Word j = 0; j < 2; ++j) {
            Word k = mix(seed + j * 4127 + g * 9041);
            Mind child{stock[mix(k + 1399) % total]};
            Word wrist = hand(child, mix(k + 2099));
            auto model = elders[j];
            if (arm == newcomers)
                model.cells = {};
            lesson(child, wrist, &model, frames[j], stone, mix(k + 113), k, j, contact_time == ~0u ? 32 : contact_time,
                   budget, arm, record, invited, reason);
            elders[j] = child;
            frames[j] = wrist;
        }
    }
    Word key = mix(seed + 16747), plan = mix(seed + 8893), ticks = 24 * span + 160;
    Mind recruit{stock[mix(seed + 4493) % total]};
    Word frame = hand(recruit, mix(seed + 1319));
    for (Word j = 0; j < 2; ++j) {
        Word side = arm == one_bank ? 0 : arm == other_bank ? 1 : j;
        auto model = elders[side];
        if (arm == newcomers)
            model.cells = {};
        lesson(recruit, frame, &model, frames[side], stone, mix(key + j * 3229), mix(key + j * 827), side,
               contact_time == ~0u ? 48 : contact_time, 0, arm, record, invited, reason);
    }
    // The last recruit receives the same independent opportunities in every social arm.
    lesson(recruit, frame, nullptr, 0, stone, mix(key + 217), mix(key + 601), 2, 0, budget, arm, record, 0, reason);
    record.gallery = canal(recruit, frame, stone, plan, span, ticks, arm, &record, reason, shape);
    if (heir)
        *heir = recruit;
    if (arm == full || arm == alone || arm == newcomers) {
        auto a = canal(elders[0], frames[0], stone, plan, span, ticks, full, nullptr, reason);
        auto b = canal(elders[1], frames[1], stone, plan, span, ticks, full, nullptr, reason);
        auto naive =
            canal(original[0], hand(original[0], mix(seed + 1251)), stone, plan, span, ticks, full, nullptr, reason);
        record.alone_a = a.food;
        record.alone_b = b.food;
        record.alone_best = a.food > b.food ? a.food : b.food;
        record.first_water = naive.water;
        record.first_food = naive.food;
        record.better = record.gallery.food > record.alone_best;
        record.poorer = record.gallery.food < record.alone_best;
        record.both_win = record.gallery.food > 0 && !a.food && !b.food;
    }
    return record;
}
AX_FN Word brood_value(Genes genes, Word key, Word stone) {
    Mind pupil{genes};
    Word frame = hand(pupil, key >> 8), score = 0;
    // Each pupil handles or observes elementary joints. A different hand operates the models.
    for (Word attempt = 0; attempt < 24; ++attempt) {
        Word k = mix(key + attempt * 2633), cue = (attempt & 3) * 3 + ((key >> 13) & 3);
        bool witness = (attempt & 1) ^ ((key >> 20) & 1);
        Word wrist = witness ? (k >> 25) & 7 : frame;
        Word movement = (k >> 10) & 3;
        Word gesture = pose(movement, wrist);
        observe(pupil, cue, gesture, frame, flex(stone, cue, gesture), witness, live);
    }
    for (Word i = 0; i < 8; ++i) {
        Word k = mix(key + 4589 + i * 4481), cue = (i & 3) * 3 + ((key >> 13) & 3), d = k >> 16 & 1;
        auto act = choose(pupil, cue, d, k, false);
        Word gate = reflect(contact(stone, cue, pose(act.motor, frame)), d);
        Byte valves[]{1, Byte(gate)};
        score += pump(valves, 2, 32, true).food;
    }
    return score;
}
enum Transfer : Word {
    relational,
    lookup,
    no_left,
    no_corner,
    no_right,
    opaque,
    mislabeled,
    given,
    independent,
    testimony,
    personal,
    elsewhere,
    rotations,
    varied,
    transfers
};
AX_FN Unseen transfer(Genes g, Word key, Word treatment, Word use_case, Word examples, Word chiral) {
    Unseen out{};
    Word cue = (key >> 6) & 15, d = (key >> 15) & 1, frame = hand(Mind{g}, key >> 21);
    Word left = cue & 3, right = cue >> 2, other_l = (left + 1 + mix(key + 593) % 3) & 3,
         other_r = (right + 1 + mix(key + 1709) % 3) & 3;
    Word corner = texture(other_l, other_r), stone = material_id(mix(key + 4421), chiral),
         rival = stone ^ (2u << (2 * left));
    out.target = cue;
    out.pivot = corner;
    out.stone_a = stone;
    out.stone_b = rival;
    out.wanted = d;
    out.signature = frame;
    for (Word twin = 0; twin < 2; ++twin) {
        Word geology = twin ? rival : stone;
        Mind mind{g};
        if (treatment == rotations)
            mind.genes.eye &= ~(1u << 15);
        Word surfaces[]{texture(left, other_r), corner, texture(other_l, right)};
        bool different_sources = use_case == 2 || treatment == varied;
        Word contacts = examples;
        if (different_sources)
            contacts += 4 * (mix(key + 7753) % 3);
        for (Word bench = 0; bench < 3; ++bench)
            for (Word j = 0; j < contacts; ++j) {
                Word other = surfaces[bench], noise = mix(key + bench * 47);
                Word gesture = (j + (noise >> 18)) & 3;
                Word reaction = flex(geology, other, gesture);
                if (treatment == opaque || (treatment == no_left && bench == 0) ||
                    (treatment == no_corner && bench == 1) || (treatment == no_right && bench == 2))
                    continue;
                bool peer = different_sources ? ((mix(key + 6337 + bench * 181 + (j / 4) * 1889) >> 19) & 1)
                                              : treatment == testimony || (treatment != personal && bench == 1);
                observe(mind, treatment == elsewhere && bench == 1 ? (other ^ 5) : other, gesture, frame, reaction,
                        peer, live | (treatment == mislabeled ? Word(scrambled) : 0u));
                ++out.observations;
            }
        if (treatment == given)
            for (Word a = 0; a < 4; ++a) {
                observe(mind, cue, a, frame, flex(geology, cue, a), false, live);
                ++out.seen_target;
            }
        auto chosen = infer(mind, cue, d, key >> 24, false, treatment != lookup);
        Word gesture = pose(chosen.action.motor, frame),
             physical = contact(geology, cue, (gesture + (treatment == independent ? mix(key + 6743) % 4 : 0)) & 3);
        Word exact = physical == (1u << d);
        out.exact += exact;
        out.inferred += chosen.derived;
        if (chosen.derived && chosen.via == corner) {
            bool equivalent = true;
            for (Word m = 0; m < 4; ++m) {
                Word transformed = ((chosen.transform & 4) ? 0u - m : m) + (chosen.transform & 3);
                Word hand_a = pose(m, frame), hand_b = pose(transformed & 3, frame);
                equivalent &= contact(geology, surfaces[0], hand_a) == contact(geology, surfaces[1], hand_b) &&
                              contact(geology, cue, hand_a) == contact(geology, surfaces[2], hand_b);
            }
            out.calibrated += equivalent;
        }
        ++out.attempted;
        if (twin) {
            out.native_b = chosen.action.motor;
            out.public_b = gesture;
            out.effect_b = physical;
            out.proof_b = chosen.via;
            out.rotation_b = chosen.transform;
        } else {
            out.native_a = chosen.action.motor;
            out.public_a = gesture;
            out.effect_a = physical;
            out.proof_a = chosen.via;
            out.rotation_a = chosen.transform;
        }
        Byte valves[]{1, Byte(reflect(physical, d))};
        auto v = pump(valves, 2, use_case ? 24 : 48, true);
        out.food += v.food;
        out.water += v.water;
        out.goal_met += v.food != 0;
    }
    out.tested = 1;
    out.both = out.exact == 2;
    return out;
}

} // namespace
KERNEL void reed_found(Field field, Word n, Word seed) {
    Word i = tid();
    if (i < n) {
        field.minds[i] = {founder(mix(seed + i * 10663))};
        field.seams[i] = 0;
    }
}
KERNEL void reed_families(Family *p, Word n, Word seed, const Genes *pool, Word source) {
    Word i = tid();
    if (i < n)
        p[i] = {source ? pool[mix(seed + i * 5077) % source] : founder(mix(seed + i * 8123)), 0};
}
KERNEL void reed_eat(Family *p, Word n, Word seed, Word epoch, Word heritage, Word chiral) {
    Word i = tid();
    if (i < n) {
        Word k = mix(seed + (i / 2) * 4177 + epoch * 2731), food = 0;
        if (heritage)
            for (Word j = 0; j < 4; ++j)
                food += transfer(p[i].genes, mix(k + j * 8123), relational, heritage == 3 ? 2u : 1u, 4, chiral).food;
        if (heritage == 0 || heritage == 2)
            food += brood_value(p[i].genes, k, material_id(mix(k + 581), chiral));
        p[i].reserve = 1 + food;
    }
}
KERNEL void reed_breed(const Family *p, Family *next, Word n, Word seed, Word epoch, Word paid) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 7487 + epoch * 17863);
    auto select = [&](Word s) {
        Word a = s & (n - 1), b = mix(s + 4831) & (n - 1);
        return p[paid && p[b].reserve > p[a].reserve ? b : a].genes;
    };
    auto a = select(k), b = select(mix(k + 9697));
    Word mask = mix(k + 349);
    Genes g{(mask & 1) ? a.hand : b.hand, (mask & 2) ? a.eye : b.eye, (mask & 4) ? a.plastic : b.plastic};
    next[i] = {inherit(g, mix(k + 7907), 48), 0};
}
KERNEL void reed_settle(Field field, Word n, const Genes *pool, Word count, Word seed) {
    Word i = tid();
    if (i < n)
        field.minds[i] = {pool[mix(seed + i * 8231) % count]};
}
KERNEL void reed_houses(const Genes *stock, Word n, Word cases, Word seed, Word generations, Word length, Word arm,
                        Word budget, Word contact_time, Word invited, Word chiral, Word reason, Transmission *rows,
                        Byte *shape, Mind *heir) {
    Word i = tid();
    if (i >= cases)
        return;
    Word k = mix(seed + i * 9377), stone = material_id(mix(k + 167), chiral);
    rows[i] = community(stock, n, k, stone, generations, length, arm, budget, contact_time, invited, reason,
                        i == 0 ? shape : nullptr, i == 0 ? heir : nullptr);
}
KERNEL void reed_rig(Word length, Word ticks, Word mode, Gallery *result) {
    if (tid())
        return;
    Byte mask[max_span];
    for (Word i = 0; i < length; ++i)
        mask[i] = mode == 3 ? 3 : mode == 4 ? 2 : mode == 2 && i == length / 2 ? 0 : 1;
    *result = pump(mask, length, ticks, mode != 1);
}
KERNEL void reed_prepare(Mind *m, Word n, Word seed, Word chiral) {
    Word i = tid();
    if (i >= n)
        return;
    auto &student = m[i];
    auto stone = material_id(mix(seed + i * 3779), chiral);
    student.cells = {};
    student.practice = 0;
    for (Word c = 0; c < 16; ++c)
        if ((c & 3) != ((c >> 2) + (i & 3)) % 4)
            for (Word g = 0; g < 4; ++g)
                observe(student, c, g, student.genes.hand & 7, flex(stone, c, g), c & 1, live);
}
KERNEL void reed_images(const Mind *m, Word n, Word phase, Word relating, Image *out, Word *evidence) {
    Word index = tid(), id = index >> 5;
    if (id >= n)
        return;
    Word lane = index & 31, cue = lane & 15, d = lane >> 4;
    auto v = infer(m[id], cue, d, phase, false, relating);
    Word admitted = Word(v.action.known && v.action.expected == (1u << d)), decisions = v.action.motor << (2 * cue),
         votes, reasons;
    asm("{.reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,0xffffffff;}" : "=r"(votes) : "r"(admitted));
    asm("{.reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,0xffffffff;}" : "=r"(reasons) : "r"(v.derived));
    for (Word step = 8; step; step >>= 1) {
        Word fellow;
        asm("shfl.sync.bfly.b32 %0,%1,%2,31,0xffffffff;" : "=r"(fellow) : "r"(decisions), "r"(step));
        decisions |= fellow;
    }
    if (!cue)
        out[id].motors[d] = decisions;
    if (!lane) {
        out[id].admitted = votes;
        evidence[id] = reasons;
    }
}
KERNEL void reed_dispatch(const Mind *subjects, const Image *images, Word n, Word calls, Word phase, Word using_images,
                          Word relating, Word *out) {
    Word i = tid();
    if (i >= calls)
        return;
    Word key = mix(i + 127), id = key % n, cue = (key >> 14) & 15, d = (key >> 28) & 1;
    if (using_images)
        out[i] = packed(images[id], cue, d);
    else {
        auto v = infer(subjects[id], cue, d, phase, false, relating).action;
        out[i] = v.motor | (Word(v.known && v.expected == (1u << d)) << 2);
    }
}

KERNEL void reed_unseen(const Genes *p, Word np, Word n, Word seed, Word arm, Word examples, Word chiral, Unseen *out) {
    Word i = tid();
    if (i < n) {
        Word k = mix(seed + i * 5503);
        out[i] = transfer(p[mix(k + 131) % np], k, arm, 0, examples, chiral);
    }
}

#include "runnel_keepers.cuh"

#include "rattle_trials.cuh"

#include "braid_studies.cuh"

#include "braid_recollection.cuh"
#include "braid_dialogue.cuh"
#include "braid_culture.cuh"
#include "braid_inquiry.cuh"
#include "braid_parallax.cuh"
#include "field_class.cuh"
#include "apprentice.cuh"
#include "drift_dialogue.cuh"
#include "inlet_interview.cuh"
#include "cascade.cuh"
#include "audience.cuh"
#include "narrative.cuh"

#include "capillary.cuh"

#include "material_context.cuh"

#include "material_signal.cuh"

#include "compound_sign.cuh"

#include "signal_voice.cuh"
