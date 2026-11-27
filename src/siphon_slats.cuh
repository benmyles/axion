#include "effort_bundle.cuh"
namespace axion::life::effort {
struct Slat {
    Word pigments = 0, pose = 0, handed = 0;
};
AX_FN Word outer(const Slat &s) { return press::item(s.pigments, s.pose); }
AX_FN void roll(Slat &s, Word actuator) { s.pose = (s.pose + ((actuator ^ s.handed) ? 1 : 3)) & 3; }
struct Load {
    Slat slat{};
    Word handles = 0, actions = 0, length = 1, trips = 1, fruit = 0, price_map = 0, portion = 0;
};
AX_FN Word handle(const Load &s, Word j) { return press::item(s.handles, (s.actions >> j) & 1); }
AX_FN Load job(Grove g, Word condition) {
    Load t{};
    Word v = mix(g.seed + 2347);
    t.slat = {press::deck(mix(g.seed + 1699)), (v >> 4) & 3, (v >> 13) & 1};
    t.handles = press::deck(mix(g.seed + 7013));
    t.actions = (v >> 8) & 7;
    t.length = (condition == double_action) ? 2 : 1 + ((v >> 10) & 1);
    t.trips = condition == short_route ? 1 : condition == long_route ? 8 : 1 + ((v >> 16) & 7);
    t.fruit = (v >> 19) & 3;
    t.price_map = g.next_strata;
    t.portion = flesh(g.next_fruit, t.fruit);
    return t;
}
AX_FN void handling(Mind &m, Load s, Word condition) {
    for (Word j = 0; j < 24; ++j) {
        Slat sister = s.slat;
        sister.pose = (j + (s.actions >> 1)) & 3;
        Word action = (j >> 2) & 1, before = outer(sister);
        roll(sister, action);
        Word after = outer(sister);
        Word mode = (condition == erased_model || (condition == passive_only && action)) ? 0u : Word(learning);
        change(m, before, action, condition == wrong_model ? (after + 1) & 3 : after, mode);
    }
}
struct Rehearsal {
    Bundle bare{}, shaped{};
    Word future = 0, unknown = 0;
};
AX_FN Rehearsal rehearse(const Frozen &m, Word first, Word handles, Word bits, Word count, Word trips, Word fruit) {
    Rehearsal r{};
    r.future = first;
    nourishment(r.bare, fruit, 2);
    nourishment(r.shaped, fruit, 2);
    exertion(r.bare, first, trips);
    for (Word i = 0; i < count; ++i) {
        Word a = (bits >> i) & 1;
        exertion(r.shaped, press::item(handles, a));
        Word p = successor(m.next, r.future, a);
        if (p < 4)
            r.future = p;
        else
            ++r.unknown;
    }
    exertion(r.shaped, r.future, trips);
    return r;
}
struct Work {
    Word spent = 0, received = 0, reached = 0, face = 0;
};
AX_FN Work carry_load(Load s, bool prepare, Word &purse, Mind *learner = nullptr) {
    Work r{};
    for (Word i = 0; prepare && i < s.length; ++i) {
        Word a = (s.actions >> i) & 1, before = outer(s.slat), h = handle(s, i), fee = resistance(s.price_map, h);
        if (purse <= fee)
            return r;
        purse -= fee;
        r.spent += fee;
        roll(s.slat, a);
        if (learner) {
            strain(*learner, h, fee, learning);
            change(*learner, before, a, outer(s.slat), learning);
        }
    }
    r.face = outer(s.slat);
    int bank = 0;
    for (Word n = 0; n < s.trips; ++n) {
        Word fee = resistance(s.price_map, outer(s.slat));
        if (purse <= fee)
            return r;
        purse -= fee;
        r.spent += fee;
        bank = 1 - bank;
        ++r.reached;
        if (learner)
            strain(*learner, r.face, fee, learning);
    }
    Word trays[2]{0, 0};
    trays[s.trips & 1] = 2 * s.portion;
    r.received = drink(purse, trays[bank], 32);
    if (learner)
        taste(*learner, s.fruit, r.received / 2, learning);
    return r;
}
AX_FN Cause carry(Genes genes, Word seed, Word condition, Sample *card) {
    Mind m{genes};
    auto g = grove(seed);
    auto load = job(g, condition);
    Word reserve = 192, food = 0, work = 0;
    calibrate(m, g, condition == old_prices ? old_drag : condition == no_prices ? no_work : lived, reserve, food, work);
    handling(m, load, condition);
    auto image = freeze(m);
    auto display = rehearse(image, outer(load.slat), load.handles, load.actions, load.length, load.trips, load.fruit);
    int a = potential(image, display.bare), b = potential(image, display.shaped);
    bool prepare = condition != never_reworked && elect(a, b, mix(seed + 5243));
    Word capital = 64, alternative = 64;
    auto r = carry_load(load, prepare, capital, &m);
    auto other = carry_load(load, !prepare, alternative);
    auto &ordinary = prepare ? other : r;
    auto &changed = prepare ? r : other;
    Cause out{};
    out.trials = 1;
    out.correct = capital >= alternative;
    out.prepared = prepare;
    out.profitable = changed.spent < ordinary.spent;
    out.ordinary_better = changed.spent > ordinary.spent;
    out.good_preparations = prepare && out.profitable;
    out.good_ordinary = !prepare && out.ordinary_better;
    out.ties = capital == alternative;
    out.food = r.received;
    out.cost = r.spent;
    out.regret = alternative > capital ? alternative - capital : 0;
    out.one_touch = load.length == 1;
    out.two_touches = load.length == 2;
    out.issued = prepare * load.length;
    out.unknown = display.unknown != 0;
    out.initially_changed = ordinary.face != changed.face;
    out.durable = r.face != ordinary.face;
    out.same = display.future == changed.face;
    out.wrongly_changed = display.future != changed.face && display.future != ordinary.face;
    out.oracle_work = r.spent < other.spent ? r.spent : other.spent;
    out.ordinary_work = ordinary.spent;
    out.prepared_work = changed.spent;
    if (card) {
        *card = {seed,
                 outer(load.slat),
                 load.slat.pigments,
                 load.price_map,
                 load.handles,
                 load.trips,
                 load.actions,
                 load.length,
                 changed.face,
                 ordinary.face,
                 display.future,
                 Word(prepare),
                 r.spent,
                 load.fruit,
                 r.received,
                 a,
                 b,
                 image.next,
                 image.costs,
                 image.wiring};
    }
    return out;
}
} // namespace axion::life::effort
KERNEL void effort_loads(const effort::Genes *laws, Word ng, Word n, Word seed, Word condition, effort::Cause *out,
                         effort::Sample *details) {
    Word i = tid();
    if (i >= n)
        return;
    Word key = mix(seed + i * 5591);
    out[i] = effort::carry(laws[mix(key + 57) % ng], key, condition, i < 4 ? details + i : nullptr);
}
