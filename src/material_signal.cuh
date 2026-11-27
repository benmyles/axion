#include "axion/material_signal.hpp"

namespace axion::life::capillary {
AX_FN Word junction_code(Word seed, Word i, Word exclude) {
    Word start = mix(seed ^ mix(i * 2539 + 2351)), stride = mix(start + 57527) | 1;
    for (Word k = 0; k < 32768; ++k) {
        Word v = (start + k * stride) & 32767;
        if (v == exclude)
            continue;
        auto p = cycle(v >> 10, (v >> 8) & 3, (v >> 6) & 3, v & 63);
        if (p.flow && !surge(v & 63, p.after))
            return v;
    }
    return ~0u;
}
KERNEL void signal_junctions(Word seed, Word holdout, Word n, Junction *fixtures) {
    Word i = tid();
    if (i >= n)
        return;
    Word training = junction_code(seed, 0, ~0u), v = i ? junction_code(seed ^ holdout, i, training) : training;
    fixtures[i] = {v & 63, v >> 10, (v >> 8) & 3, (v >> 6) & 3};
}
AX_FN void forage_day(SignalMemory &sender, SignalMemory &receiver, Signalling course, Junction fixture, Word seed,
                      Word day, Word *total) {
    Word salt = mix(seed ^ mix(day * 8387 + 29009)), cue = mix(salt + 17401) & 1;
    auto r = encounter_signal(sender, receiver, fixture, cue, salt, course.inert, false);
    total[training_food] += r.food;
    total[training_cost] += r.cost;
    total[deposits] += r.write;
    if (day < 32)
        total[initial_meals] += r.food;
    if (day + 32 >= course.lessons)
        total[final_meals] += r.food;
    if (course.feedback != 1) {
        Word feedback = course.feedback == 2 ? (mix(salt + 56893) & 1) : r.food;
        if (course.learners & 1)
            total[sender_changes] += digest_signal(sender, cue, r.write, feedback);
        if (course.learners & 2)
            total[receiver_changes] += digest_signal(receiver, r.light, r.choice, feedback);
    }
}
AX_FN void rear_signals(SignalMemory &sender, SignalMemory &receiver, Signalling course, Junction fixture, Word seed,
                        Word *total) {
    for (Word day = 0; day < course.lessons; ++day)
        forage_day(sender, receiver, course, fixture, seed, day, total);
}
AX_FN Word relay_seed(Signalling c, Word life) { return mix(c.seed ^ mix(life * 4283 + 33791)); }
AX_FN Word relay_noise(Word key, Word trial, Signalling course) {
    return mix((key ^ course.holdout) ^ mix(trial * 2203 + 8719));
}
KERNEL void signal_rear_pair(SignalMemory *actors, Signalling course, const Junction *fixtures,
                             SignalMemory *neighbours, Word count, SignalGrade *bill) {
    SignalGrade totals{};
    auto a = actors[0], b = actors[1];
    Word seed = relay_seed(course, course.first);
    if (count) {
        Word index = mix(seed + 65123) % (count + 1);
        for (Word day = 0; day < course.lessons; ++day) {
            if (!index)
                forage_day(a, b, course, fixtures[0], seed, day, totals.total);
            else if (course.learners == 1)
                forage_day(a, neighbours[index - 1], course, fixtures[0], seed, day, totals.total);
            else
                forage_day(neighbours[index - 1], b, course, fixtures[0], seed, day, totals.total);
            if (++index > count)
                index = 0;
        }
    } else
        rear_signals(a, b, course, fixtures[0], seed, totals.total);

    actors[0] = a;
    actors[1] = b;
    *bill = totals;
}
KERNEL void signal_grade_pair(const SignalMemory *actors, Signalling course, const Junction *fixtures,
                              SignalTransfer *out) {
    Word i = tid();
    if (i >= 2 * course.tests)
        return;
    auto a = actors[0], b = actors[1];
    Word cue = i & 1, trial = i / 2;
    auto r = encounter_signal(a, b, fixtures[trial + 1], cue,
                              relay_noise(relay_seed(course, course.first), trial, course), course.inert, true);
    out[i] = {cue, r.write, r.light, r.choice, r.erased, r.inverse, r.food, r.cost};
}
KERNEL void signal_pairs(const Body *bodies, const unsigned char *prior, Word n, Signalling course,
                         SignalMemory teacher, const Junction *fixtures, SignalGrade *all) {
    Word pair = tid() / 32, lane = tid() & 31;
    if (pair >= n)
        return;
    Word peer = (pair + 1 + (mix(course.seed + 9137) % (n - 1))) % n;
    SignalGrade summed{};
    auto &total = summed.total;
    for (Word life = lane; life < course.replicas; life += 32) {
        SignalMemory sender{Byte(prior ? prior[pair] : 0), bodies[pair]},
            receiver{Byte(prior ? prior[peer] : 0), bodies[peer]};
        if (course.tutor == 1) {
            sender = teacher;
            receiver = {Byte(prior ? prior[pair] : 0), bodies[pair]};
        }
        if (course.tutor == 2)
            receiver = teacher;
        Word seed = relay_seed(course, life + course.first);
        auto assess = [&](bool trained) {
            for (Word k = 0; k < course.tests; ++k) {
                Word noise = relay_noise(seed, k, course);
                Relay r[2]{};
                for (Word cue = 0; cue < 2; ++cue) {
                    auto w = sender, v = receiver;
                    r[cue] = encounter_signal(w, v, fixtures[k + 1], cue, noise, course.inert, true);
                    total[trained ? after_correct : before_correct] += r[cue].food;
                    if (trained) {
                        total[erased_correct] += r[cue].erased == cue;
                        total[inverse_correct] += r[cue].inverse == cue;
                        total[cue ? right_marks : left_marks] += r[cue].write;
                    }
                }
                bool both = r[0].food && r[1].food;
                total[trained ? finally_both : initially_both] += both;
                if (trained) {
                    total[left_code] += both && r[0].write && !r[1].write;
                    total[right_code] += both && r[1].write && !r[0].write;
                }
            }
        };
        Word initial = total[initially_both];
        assess(false);
        bool born = total[initially_both] - initial == course.tests;
        rear_signals(sender, receiver, course, fixtures[0], seed, total);
        Word left = total[left_code], right = total[right_code];
        Word later = total[finally_both];
        assess(true);
        // Each lane owns one or more independent lives. Count mastery per lifetime.
        total[reliable_left] += (total[left_code] - left) * 4 >= 3 * course.tests;
        total[reliable_right] += (total[right_code] - right) * 4 >= 3 * course.tests;
        bool mastery = total[finally_both] - later == course.tests;
        total[initial_mastery] += born;
        total[final_mastery] += mastery;
        total[acquired_mastery] += !born && mastery;
        total[lost_mastery] += born && !mastery;
    }
    for (Word k = 0; k < signal_metrics; ++k) {
        Word r;
        asm("redux.sync.add.u32 %0,%1,0xffffffff;" : "=r"(r) : "r"(total[k]));
        total[k] = r;
    }
    if (!lane)
        all[pair] = summed;
}
} // namespace axion::life::capillary
