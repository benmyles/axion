#include "axion/compound_sign.hpp"

namespace axion::life::capillary {
KERNEL void compound_beds(const Junction *left, const Junction *right, Word n, Word inert, CompoundBed *out) {
    Word j = tid();
    if (j < n)
        out[j] = {{left[j], right[j]}, projector(left[j], inert) | (projector(right[j], inert) << 2)};
}
AX_FN void compound_day(CompoundMind &a, CompoundMind &b, Signalling c, CompoundCourse cc, Word optics, Word seed,
                        Word t, Word *values) {
    Word key = mix(seed ^ mix(t * 2833 + 7723)), cue = mix(key + 17627);
    if (cc.observed < 4) {
        cue %= 3;
        cue += cue >= cc.observed;
    } else
        cue &= 3;
    auto result = compound_turn(a, b, optics, cue, key, false);
    ++values[trained_zero + cue];
    values[rearing_food] += result.food;
    values[rearing_cost] += result.cost;
    Word diet = cc.wean && t >= cc.wean ? 0 : cc.diet;
    Word stock = diet ? nutrient_bed(cue, bool(seed & 1)) : (1u << (cue * 2));
    Word crop = take_crop(stock, result.choice);
    values[first_crop] += bool(crop & 1);
    values[second_crop] += bool(crop & 2);
    if (c.feedback != 1) {
        Word feedback = crop;
        if (c.feedback == 2) {
            Word alternate = mix(key + 4099) & 3;
            if (diet) {
                Word shadow = nutrient_bed(alternate, bool(seed & 1));
                feedback = take_crop(shadow, result.choice);
            } else
                feedback = alternate == 0;
        }
        if (c.learners & 1)
            values[source_edits] += nourish_ports(a, cue, result.mark, taste_ports(a.body, feedback, diet));
        if (c.learners & 2)
            values[listener_edits] +=
                nourish_ports(b, result.sensed, result.choice, taste_ports(b.body, feedback, diet));
    }
}
AX_FN void compound_rearing(CompoundMind &a, CompoundMind &b, Signalling c, CompoundCourse cc, Word optics, Word seed,
                            Word *values) {
    for (Word t = 0; t < c.lessons; ++t)
        compound_day(a, b, c, cc, optics, seed, t, values);
}
AX_FN Word compound_test(const CompoundMind &writer, const CompoundMind &reader, Signalling c, const CompoundBed *beds,
                         Word seed, bool trained, Word *totals) {
    Count routes[4]{}, correct = 0;
    for (Word j = 0; j < c.tests; ++j) {
        Word key = relay_noise(seed, j, c), joint = 0;
        Word decisions = 0, optics = beds[j + 1].optics;
        for (Word light = 0; light < 4; ++light) {
            auto b = reader;
            decisions |= compose_move(b, light, mix(key + 24407)) << (2 * light);
        }
        auto decide = [&](Word mark) { return (decisions >> (2 * compound_light(optics, mark))) & 3; };
        for (Word cue = 0; cue < 4; ++cue) {
            auto a = writer;
            Word mark = compose_move(a, cue, mix(key + 49277)), choice = decide(mark);
            CompoundTurn v{cue,
                           mark,
                           compound_light(optics, mark),
                           choice,
                           decide(mark & 2),
                           decide(mark & 1),
                           decide(0),
                           gather_meal(cue, choice),
                           3u + Word(__builtin_popcount(mark))};
            joint += v.food;
            routes[cue] += Count(1) << (v.mark * 16);
            correct += Count(v.food) << (cue * 16);
            if (trained) {
                totals[one_removed] += v.without_first == cue;
                totals[two_removed] += v.without_second == cue;
                totals[both_removed] += v.blank == cue;
                totals[hot_zero + cue] += v.food;
            }
        }
        totals[trained ? hot_four : cold_four] += joint;
        totals[trained ? hot_all : cold_all] += joint == 4;
    }
    Word modal[4]{}, codes = 0;
    bool reliable = true;
    for (Word cue = 0; cue < 4; ++cue) {
        Word mark = modal[cue] = codeword(routes[cue]);
        codes |= 1u << mark;
        reliable &= 4 * ((routes[cue] >> (mark * 16)) & 65535) >= 3 * c.tests &&
                    4 * ((correct >> (cue * 16)) & 65535) >= 3 * c.tests;
    }
    reliable &= codes == 15;
    if (trained) {
        bool separate = __builtin_popcount(modal[0] ^ modal[1]) == 1 && __builtin_popcount(modal[0] ^ modal[2]) == 1;
        totals[fluent] += reliable;
        totals[axiswise] += reliable && separate;
        totals[mixed] += reliable && !separate;
    } else
        totals[prefluent] += reliable;
    return modal[0] | (modal[1] << 2) | (modal[2] << 4) | (modal[3] << 6) | (Word(reliable) << 8);
}
KERNEL void compound_pairs(const Body *bodies, const Word *prior, Word n, Signalling course, CompoundCourse cc,
                           CompoundMind teacher, const CompoundBed *beds, CompoundGrade *all) {
    Word pair = tid() / 32, lane = tid() & 31;
    if (pair >= n)
        return;
    Word peer = (pair + 1 + mix(course.seed + 9137) % (n - 1)) % n;
    CompoundGrade sum{};
    auto writer = compound_body(bodies[pair], cc.arbor, prior ? prior[pair] : 0),
         reader = compound_body(bodies[peer], cc.arbor, prior ? prior[peer] : 0);
    if (course.tutor == 1) {
        writer = teacher;
        reader = compound_body(bodies[pair], cc.arbor, prior ? prior[pair] : 0);
    }
    if (course.tutor == 2)
        reader = teacher;
    sum.writer = writer.fields;
    sum.reader = reader.fields;
    for (Word life = lane; life < course.replicas; life += 32) {
        auto a = writer, b = reader;
        Word seed = relay_seed(course, life + course.first);
        compound_test(a, b, course, beds, seed, false, sum.value);
        compound_rearing(a, b, course, cc, beds[0].optics, seed, sum.value);
        compound_test(a, b, course, beds, seed, true, sum.value);
    }
    for (Word k = 0; k < compound_metrics; ++k) {
        Word v;
        asm("redux.sync.add.u32 %0,%1,0xffffffff;" : "=r"(v) : "r"(sum.value[k]));
        sum.value[k] = v;
    }
    if (!lane)
        all[pair] = sum;
}
KERNEL void compound_individual(CompoundMind *pair, Signalling course, CompoundCourse cc, const CompoundBed *beds,
                                CompoundMind *neighbours, Word count, CompoundGrade *result) {
    auto a = pair[0], b = pair[1];
    CompoundGrade bill{};
    Word seed = relay_seed(course, course.first);
    if (count) {
        Word who = mix(seed + 65123) % (count + 1);
        for (Word t = 0; t < course.lessons; ++t) {
            if (!who)
                compound_day(a, b, course, cc, beds[0].optics, seed, t, bill.value);
            else if (course.learners == 1)
                compound_day(a, neighbours[who - 1], course, cc, beds[0].optics, seed, t, bill.value);
            else
                compound_day(neighbours[who - 1], b, course, cc, beds[0].optics, seed, t, bill.value);
            if (++who > count)
                who = 0;
        }
    } else
        compound_rearing(a, b, course, cc, beds[0].optics, seed, bill.value);
    pair[0] = a;
    pair[1] = b;
    *result = bill;
}
KERNEL void compound_frozen_scalar(const CompoundMind *pair, Signalling c, const CompoundBed *beds, Word panels,
                                   PackedReport *receipts) {
    Word i = tid();
    if (i >= 4 * panels)
        return;
    auto a = pair[0], b = pair[1];
    Word panel = i / 4;
    receipts[i] = pack_report(compound_turn(a, b, beds[1 + panel % c.tests].optics, i & 3,
                                            relay_noise(relay_seed(c, c.first), panel, c), true));
}
KERNEL void compound_frozen(const CompoundMind *pair, Signalling c, const CompoundBed *beds, Word panels,
                            PackedReport *receipts) {
    Word i = tid();
    if (i >= 4 * panels)
        return;
    Word cue = i & 3, panel = i / 4, noise = relay_noise(relay_seed(c, c.first), panel, c),
         optics = beds[1 + panel % c.tests].optics;
    auto a = pair[0], b = pair[1];
    Word mark = compose_move(a, cue, mix(noise + 49277)), response = compose_move(b, cue, mix(noise + 24407));
    // The four balanced worlds share the same receiver and noise. Each lane
    // evaluates a different possible sensation. All four counterfactual reads are register shuffles.
    Word base = i & 28, mask = 15u << base;
    auto read = [&](Word optical) {
        Word value, index = base + optical;
        asm volatile("shfl.sync.idx.b32 %0,%1,%2,31,%3;" : "=r"(value) : "r"(response), "r"(index), "r"(mask));
        return value;
    };
    Word light = compound_light(optics, mark), choice = read(light);
    receipts[i] = pack_report({cue, mark, light, choice, read(compound_light(optics, mark & 2)),
                               read(compound_light(optics, mark & 1)), read(compound_light(optics, 0)),
                               gather_meal(cue, choice), 3u + Word(__builtin_popcount(mark))});
}
KERNEL void compare_compound_reports(const PackedReport *a, const PackedReport *b, Word n, Word *mismatch) {
    Word i = tid();
    if (i < n && a[i] != b[i])
        asm volatile("red.global.add.u32 [%0],1;" ::"l"(mismatch) : "memory");
}
} // namespace axion::life::capillary

namespace axion::life::capillary {
KERNEL void compound_lifetimes(const CompoundMind *parents, Signalling c, CompoundCourse cc, const CompoundBed *beds,
                               Word n, CompoundLife *out) {
    Word j = tid();
    if (j >= n)
        return;
    auto a = parents[0], b = parents[1];
    CompoundLife r{};
    r.identity = c.first + j;
    Word seed = relay_seed(c, r.identity);
    r.initial_code = compound_test(a, b, c, beds, seed, false, r.grade.value);
    compound_rearing(a, b, c, cc, beds[0].optics, seed, r.grade.value);
    r.code = compound_test(a, b, c, beds, seed, true, r.grade.value);
    r.writer = a.tissue;
    r.reader = b.tissue;
    out[j] = r;
}

} // namespace axion::life::capillary
