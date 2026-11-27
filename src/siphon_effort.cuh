#include "effort_bundle.cuh"
namespace axion::life::effort {
struct Grove {
    Word fruits = 0, next_fruit = 0, strata = 0, next_strata = 0, seed = 0;
};
struct Trial {
    Prospect near{}, far{};
    Word stock[2]{}, floor[2]{}, units[2]{}, body = 0, opening = 0, eaten = 0, spent = 0;
    int x = 0;
};
AX_FN Word rotate_values(Word packed, Word offset) {
    Word v = 0;
    for (Word j = 0; j < 4; ++j)
        v |= ((press::item(packed, j) + offset) & 3) << (2 * j);
    return v;
}
AX_FN Grove grove(Word seed) {
    Grove v{};
    v.seed = seed;
    v.fruits = press::deck(mix(seed + 41));
    v.strata = press::deck(mix(seed + 971));
    v.next_fruit = rotate_values(v.fruits, 1 + mix(seed + 371) % 3);
    v.next_strata = rotate_values(v.strata, 1 + mix(seed + 5113) % 3);
    return v;
}
AX_FN Word flesh(Word map, Word cue) { return 2 + 4 * press::item(map, cue); }
AX_FN Word resistance(Word map, Word surface) { return 1 + press::item(map, surface); }
AX_FN Word drink(Word &reserve, Word &stock, Word appetite) {
    Word q = stock < appetite ? stock : appetite;
    q = q < 65535 - reserve ? q : 65535 - reserve;
    stock -= q;
    reserve += q;
    return q;
}
AX_FN void sample_fruit(Mind &m, Word map, Word k, Word mode, Word &reserve, Word &eaten, bool exchanged = false) {
    for (Word i = 0; i < 4; ++i) {
        Word skin = (k + i * (1 + 2 * ((k >> 5) & 1))) & 3, stock = flesh(map, skin);
        Word mouth = drink(reserve, stock, 16);
        eaten += mouth;
        taste(m, (skin + Word(exchanged)) & 3, mouth, mode);
    }
}
AX_FN void sample_soil(Mind &m, Word map, Word k, Word mode, Word &reserve, Word &work, bool exchanged = false) {
    int position = 0;
    int sign = (k & 32) ? 1 : -1;
    for (Word i = 0; i < 4; ++i) {
        Word sample = (k + Word(position) * (1 + 2 * ((k >> 7) & 1))) & 3, fee = resistance(map, sample);
        if (reserve <= fee)
            continue;
        reserve -= fee;
        position += sign;
        work += fee;
        strain(m, (sample + Word(exchanged)) & 3, fee, mode);
    }
}
AX_FN void calibrate(Mind &m, Grove g, Word intervention, Word &reserve, Word &food, Word &paid) {
    Word k = g.seed;
    for (Word phase = 0; phase < 2; ++phase)
        for (Word repeat = 0; repeat < 3; ++repeat) {
            Word observation = mix(k + phase * 1217 + repeat * 73);
            Word read = intervention == fixed_organs ? 0u : Word(learning);
            Word fruit = phase ? g.next_fruit : g.fruits, soil = phase ? g.next_strata : g.strata;
            if (intervention == empty || intervention == no_food || (phase && intervention == old_fruit))
                read |= blinded_food;
            if (intervention == empty || intervention == no_work || (phase && intervention == old_drag))
                read |= blinded_work;
            // Every counterfactual body receives the same physical samples and steps.
            // The intervention changes what reaches its private synapses.
            sample_fruit(m, fruit, observation, read, reserve, food, intervention == other_food);
            sample_soil(m, soil, mix(observation + 617), read, reserve, paid, intervention == other_work);
        }
}
AX_FN Trial doorstep(Grove g, Word k, Word intervention) {
    Trial t{};
    Word a = mix(k + 193), b = mix(k + 373);
    t.near = {a & 3, (a >> 3) & 3, intervention == both_remote ? 1 + ((a >> 6) & 3) : 0u};
    t.far = {(t.near.fruit + 1 + ((b >> 3) % 3)) & 3, (b >> 8) & 3, 1 + ((b >> 13) % 5)};
    t.units[0] = 1 + ((a >> 19) & 1);
    t.units[1] = 1 + ((b >> 19) % 4);
    t.stock[0] = t.units[0] * flesh(g.next_fruit, t.near.fruit);
    t.stock[1] = t.units[1] * flesh(g.next_fruit, t.far.fruit);
    t.floor[0] = resistance(g.next_strata, t.near.surface);
    t.floor[1] = resistance(g.next_strata, t.far.surface);
    if (intervention == flat_cost)
        t.floor[0] = t.floor[1] = 1;
    if (intervention == opposite_surface)
        t.far.surface = (t.far.surface + 2) & 3;
    t.body = t.opening = 32;
    return t;
}
AX_FN void walk(Trial &t, Mind &m, bool other, Word mode) {
    Prospect p = other ? t.far : t.near;
    Word fee = t.floor[other];
    for (Word step = 0; step < p.steps; ++step) {
        if (t.body <= fee)
            return;
        t.body -= fee;
        t.spent += fee;
        t.x += other ? 1 : -1;
        strain(m, p.surface, fee, mode);
    }
    if (t.x == (other ? int(p.steps) : -int(p.steps))) {
        t.eaten = drink(t.body, t.stock[other], 16 * t.units[other]);
        taste(m, p.fruit, t.eaten / t.units[other], mode);
    }
}
AX_FN Crossing decision(Mind &m, Grove g, Trial &t, Word key, Word intervention, Card *record) {
    auto forecast = [&](Prospect p, Word units) {
        Bundle consumption{};
        nourishment(consumption, p.fruit, units);
        exertion(consumption, p.surface, p.steps);
        return potential(m, consumption);
    };
    int left = forecast(t.near, t.units[0]), right = forecast(t.far, t.units[1]);
    bool away = elect(left, right, key);
    int a = int(t.stock[0]) - int(t.floor[0] * t.near.steps), b = int(t.stock[1]) - int(t.floor[1] * t.far.steps);
    int better = a > b ? a : b, base = int(t.opening);
    int old_a =
            int(t.units[0] * flesh(g.fruits, t.near.fruit)) - int(resistance(g.strata, t.near.surface) * t.near.steps),
        old_b = int(t.units[1] * flesh(g.fruits, t.far.fruit)) - int(resistance(g.strata, t.far.surface) * t.far.steps);
    bool switched = (a > b && old_a < old_b) || (a < b && old_a > old_b);
    if (record) {
        *record = {};
        record->id = g.seed;
        record->fruit_map = g.fruits;
        record->other_map = g.next_fruit;
        record->tariff = g.strata;
        record->new_tariff = g.next_strata;
        record->home = t.near.fruit;
        record->away = t.far.fruit;
        record->surface = t.far.surface;
        record->far = t.far.steps;
        record->near = t.near.steps;
        record->initial = t.opening;
        record->units = t.units[0] | (t.units[1] << 8);
        record->genes = m.genes;
        record->brain = m;
        record->left = left;
        record->right = right;
        record->chosen = away;
    }
    Prospect p = away ? t.far : t.near;
    begin(m, p, t.body, away, away, false, 1);
    walk(t, m, away, intervention == fixed_organs ? 0u : Word(learning));
    advance(m, t.body, intervention == fixed_organs ? 0u : Word(learning));
    m.trace = m.account = 0;
    int earned = int(t.body) - base;
    Crossing r{};
    r.legs = 1;
    r.right = earned == better;
    r.stay = !away;
    r.travel = away;
    r.fruit = t.eaten;
    r.work = t.spent;
    r.optimum = better + base;
    r.goal = t.body;
    r.regret = better - earned;
    r.forced_stay = a + base;
    r.forced_travel = b + base;
    r.ties = a == b;
    r.stay_better = a > b;
    r.travel_better = b > a;
    r.stay_right = a > b && !away;
    r.travel_right = b > a && away;
    r.would_switch = switched;
    r.switched_right = switched && r.right;
    if (record) {
        record->ate = t.eaten;
        record->worked = t.spent;
        record->ideal = r.optimum;
    }
    return r;
}
AX_FN Crossing visit(Genes genes, Word seed, Word intervention, Card *record) {
    auto g = grove(seed);
    Mind m{genes};
    Word budget = 192, food = 0, paid = 0;
    calibrate(m, g, intervention, budget, food, paid);
    auto t = doorstep(g, mix(seed + 15313), intervention);
    return decision(m, g, t, mix(seed + 3779), intervention, record);
}
} // namespace axion::life::effort
KERNEL void effort_probe(const effort::Genes *laws, Word ng, Word n, Word seed, Word mode, effort::Crossing *out,
                         effort::Card *cards) {
    Word i = tid();
    if (i >= n)
        return;
    Word key = mix(seed + i * 15773);
    out[i] = effort::visit(laws[mix(key + 1591) % ng], key, mode, i < 4 ? cards + i : nullptr);
}
#include "siphon_slats.cuh"
KERNEL void effort_parents(effort::Family *adults, Word n, Word seed, Word epoch, Word curriculum) {
    Word i = tid();
    if (i >= n)
        return;
    auto &f = adults[i];
    Word reserve = 8;
    for (Word j = 0; j < 20; ++j) {
        // Neighbours use separate reservoirs in the same local weather.
        Word k = mix(seed + (i / 2) * 6781 + epoch * 4391 + j * 1171);
        Word food = 0, spent = 0;
        if (curriculum == 2 || (curriculum && (j & 3) >= 2)) {
            auto r = effort::carry(f.genes, k, effort::new_route, nullptr);
            food = r.food;
            spent = r.cost;
        } else {
            auto r = effort::visit(f.genes, k, (k & 1) ? effort::lived : effort::both_remote, nullptr);
            food = r.fruit;
            spent = r.work;
        }
        // A common travelling advance is repaid from the actual meal.
        spent += 5;
        if (food > spent)
            reserve += food - spent;
        else
            reserve -= reserve > spent - food ? spent - food : reserve;
    }
    f.eggs = 1 + reserve / 4;
}
KERNEL void effort_offspring(const effort::Family *a, effort::Family *b, Word n, Word seed, Word epoch, Word paid) {
    Word i = tid();
    if (i >= n / 2)
        return;
    Word k = mix(seed + epoch * 57467), hop = mix(k + 9337) | 1u, first = 2 * i, second = first + 1;
    Word funds[2]{a[first].eggs, a[second].eggs};
    for (Word j = 0; j < 2; ++j) {
        Word key = mix(k + i * 8753 + j * 179), v = paid ? Word((key % (funds[0] + funds[1])) >= funds[0]) : key & 1;
        if (paid)
            --funds[v];
        auto g = a[v ? second : first].genes, donor = a[v ? first : second].genes;
        if ((g.projection & (1u << 21)) && ((key >> 12) & 7) == 0) {
            Word mask = 0u - Word(1u << ((key >> 23) & 31));
            switch ((key >> 18) & 3) {
            case 0:
                g.appetite = (g.appetite & ~mask) | (donor.appetite & mask);
                break;
            case 1:
                g.muscle = (g.muscle & ~mask) | (donor.muscle & mask);
                break;
            case 2:
                g.projection = (g.projection & ~mask) | (donor.projection & mask);
                break;
            default:
                g.episode = (g.episode & ~mask) | (donor.episode & mask);
            }
        }
        Word out = (k + (2 * i + j) * hop) & (n - 1);
        b[out] = {effort::child(g, mix(key + 2731), 3 + ((g.appetite >> 26) & 31)), 0};
    }
}
KERNEL void effort_candidates(const effort::Genes *g, Word n, Word seed, Word *points) {
    Word i = tid();
    if (i >= n)
        return;
    Word score = 0;
    for (Word j = 0; j < 128; ++j) {
        Word k = mix(seed + 3791 + j * 1327);
        score += effort::visit(g[i], k, (j & 1) ? effort::lived : effort::both_remote, nullptr).goal;
        auto load = effort::carry(g[i], mix(k + 73), effort::new_route, nullptr);
        score += 64 + load.food - load.cost;
    }
    points[i] = score;
}
KERNEL void effort_images(const effort::Mind *m, effort::Frozen *out, Word n) {
    Word i = tid();
    if (i < n)
        out[i] = effort::freeze(m[i]);
}
template <bool fast>
AX_FN void effort_answers(const effort::Mind *m, const effort::Frozen *f, Word n, Word calls, Word offset, Word seed,
                          Count *out) {
    Word i = tid();
    if (i >= calls)
        return;
    Word ix = i + offset, person = (ix / 8) % n, k = mix(seed + ix * 3517);
    auto bundle = [&](Word q) {
        return effort::Bundle{mix(q + 331) & 0x0f0f0f0fu, mix(q + 863) & 0x1f1f1f1fu, q & 15};
    };
    auto a = bundle(k), b = bundle(mix(k + 2729));
    int left, right;
    if constexpr (fast) {
        auto cortex = f[person];
        left = effort::potential(cortex, a);
        right = effort::potential(cortex, b);
    } else {
        auto cortex = m[person];
        left = effort::potential(cortex, a);
        right = effort::potential(cortex, b);
    }
    out[i] = Word(left) | (Count(Word(right)) << 32);
}
#define COST_CALL(fn, fast)                                                                                            \
    KERNEL void fn(const effort::Mind *m, const effort::Frozen *f, Word n, Word calls, Word off, Word seed,            \
                   Count *out) {                                                                                       \
        effort_answers<fast>(m, f, n, calls, off, seed, out);                                                          \
    }
COST_CALL(effort_live_queries, false)
COST_CALL(effort_frozen_queries, true)
#undef COST_CALL
