#include "axion/runnel_trace.hpp"
namespace axion::life::runnel {
enum Steward : Word {
    inherited_land,
    unseen_work,
    young_ground,
    both_absent,
    misplaced,
    shuffled_gait,
    idle_tide,
    no_works,
    one_adult
};
struct Reach {
    Chamber cells[max_span + 1];
    Byte bends[max_span]{}, apertures[max_span]{}, provenance[max_span]{};
    Word clock = 0;
};
// Adult and observer share the visible seam. Neither receives the hidden mineral
// law or a global fault location. The next generation can retain the public bank.
AX_FN Keeping keep(const Genes *pool, Word np, Word seed, Word length, Word seasons, Word turns, Word arm, Word chiral,
                   Word reason, ReachFrame *frames = nullptr) {
    Reach land{};
    fill(land.cells, length);
    for (Word j = 0; j < length; ++j)
        land.apertures[j] = 3;
    Word world = material_id(mix(seed + 10753), chiral), plan = mix(seed + 7789), position = 0;
    Mind elder{pool[mix(seed + 661) % np]};
    Keeping sum{};
    for (Word generation = 0; generation < seasons; ++generation) {
        if (generation && (arm == young_ground || arm == both_absent))
            for (Word j = 0; j < length; ++j) {
                land.apertures[j] = 3;
                land.bends[j] = 0;
                land.provenance[j] = 0;
            }
        Mind young{pool[mix(seed + generation * 4201 + 5981) % np]};
        if (generation && arm != one_adult)
            position = 0;
        Word purse = turns + 8, spent = 0;
        for (Word step = 0; step < turns; ++step) {
            Word j = position, pin = landmark(plan, j), cue = local_cue(pin), direction = upstream(pin),
                 k = mix(seed + land.clock * 6917);
            bool near = land.cells[j].carbon != 0, far = land.cells[j + 1].carbon != 0;
            Word op = gait(elder.genes, near, far, land.bends[j] != 0, k);
            if (arm == shuffled_gait)
                op = gait(elder.genes, !far, !near, land.bends[j] != 0, k);
            ++sum.steps;
            if (purse && (op == 1 || op == 2)) {
                bool can = op == 1 ? position + 1 < length : position != 0;
                if (can) {
                    position = op == 1 ? position + 1 : position - 1;
                    ++sum.walked;
                }
                --purse;
                ++spent;
            } else if (purse >= 2 && (op == 0 || op == 3)) {
                auto decision = infer(elder, cue, direction, k, true, reason);
                Word gesture = pose(decision.action.motor, elder.genes.hand & 7), old = land.bends[j];
                if (op == 3 && old)
                    gesture = old - 1;
                Word response = flex(world, cue, gesture);
                --purse;
                ++spent;
                ++sum.probed;
                observe(elder, cue, gesture, elder.genes.hand & 7, response, false, live);
                if (arm != unseen_work && arm != both_absent && (young.genes.eye & 8)) {
                    ++sum.seen;
                    observe(young, cue, gesture, young.genes.hand & 7, response, true,
                            live | (arm == misplaced ? Word(scrambled) : 0u));
                }
                if (op == 0 && arm != no_works) {
                    --purse;
                    ++spent;
                    ++sum.folded;
                    land.bends[j] = Byte(gesture + 1);
                    land.apertures[j] = Byte(reflect(response, direction));
                    if (old != land.bends[j])
                        land.provenance[j] = Byte(generation + 1);
                }
            }
            Gallery flow{};
            advance(land.cells, land.apertures, length, land.clock, 2, arm != idle_tide, flow);
            land.clock += 2;
            if (position + 1 == length && land.cells[length].carbon && purse < 255) {
                Word bite = land.cells[length].carbon < 3 ? land.cells[length].carbon : 3;
                if (bite > 255 - purse)
                    bite = 255 - purse;
                land.cells[length].carbon -= bite;
                purse += bite;
                sum.eaten += bite;
            }
            if (flow.into && land.provenance[length - 1] && land.provenance[length - 1] < generation + 1)
                sum.older += flow.into;
        }
        sum.offered += turns + 8;
        sum.spent += spent;
        sum.stock += purse;
        if (!generation)
            sum.opening = land.cells[length].carbon + sum.eaten;
        if (frames) {
            Word nframes = seasons < 6 ? seasons : 6;
            for (Word j = 0; j < nframes; ++j)
                if (generation == (nframes < 2 ? 0 : j * (seasons - 1) / (nframes - 1))) {
                    auto &v = frames[j];
                    v.generation = generation + 1;
                    v.cursor = position;
                    v.work = sum.spent;
                    v.food = land.cells[length].carbon + sum.eaten;
                    v.water = land.cells[length].water;
                    for (Word e = 0; e < length; ++e)
                        v.joints[e] = Byte(land.apertures[e] | (Word(land.bends[e] != 0) << 2) |
                                           (Word(land.cells[e + 1].carbon != 0) << 3) |
                                           (Word(land.provenance[e] && land.provenance[e] < generation + 1) << 4));
                }
        }
        if (arm != one_adult)
            elder = young;
    }
    sum.food = land.cells[length].carbon + sum.eaten;
    sum.water = land.cells[length].water;
    for (Word j = 0; j <= length; ++j) {
        sum.store_water += land.cells[j].water;
        sum.store_food += land.cells[j].carbon;
        if (land.cells[j].carbon)
            sum.reach = j;
    }
    for (Word j = 0; j < length; ++j) {
        sum.built += land.bends[j] != 0;
        sum.correct += land.bends[j] && land.apertures[j] == 1;
    }
    return sum;
}
} // namespace axion::life::runnel
KERNEL void reed_keep(const Genes *p, Word ng, Word cases, Word seed, Word length, Word seasons, Word life, Word arm,
                      Word chiral, Word reason, Keeping *out) {
    Word i = tid();
    if (i < cases)
        out[i] = keep(p, ng, mix(seed + i * 1427), length, seasons, life, arm, chiral, reason);
}
KERNEL void reed_keep_food(Family *p, Word n, Word seed, Word epoch, Word chiral) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + (i / 2) * 2053 + epoch * 2789), length = 2 + ((k >> 13) & 3);
    // Reproduction draws on source carbon actually withdrawn by a resident
    // at the outlet. Each parent faces the same two catchments as its competitor.
    Word food = 0;
    for (Word j = 0; j < 2; ++j) {
        auto v = keep(&p[i].genes, 1, mix(k + 1733 * j), length, 3, 24, inherited_land, chiral, false);
        food += v.eaten;
    }
    p[i].reserve = 1 + food;
}

KERNEL void reed_story(const Genes *p, Word ng, Word seed, Word example, Word length, Word seasons, Word turns,
                       Word chiral, Word reason, ReachFrame *frames, Keeping *record) {
    if (tid())
        return;
    *record = keep(p, ng, mix(seed + example * 1427), length, seasons, turns, inherited_land, chiral, reason, frames);
}
