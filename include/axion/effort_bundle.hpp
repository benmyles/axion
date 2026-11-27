#pragma once
#include "effort.hpp"
namespace axion::life::effort {
struct Bundle {
    Word food = 0, work = 0, terminal = 0;
};
struct Frozen {
    Word yields = 0, costs = 0, outcomes = 0, wiring = 0, next = 0;
};
static_assert(sizeof(Frozen) == 20);
struct Cause {
    Word trials = 0, correct = 0, prepared = 0, profitable = 0, good_preparations = 0, good_ordinary = 0,
         ordinary_better = 0, ties = 0, food = 0, cost = 0, regret = 0, one_touch = 0, two_touches = 0, issued = 0,
         unknown = 0, initially_changed = 0, durable = 0, same = 0, wrongly_changed = 0, oracle_work = 0,
         ordinary_work = 0, prepared_work = 0;
};
struct Sample {
    Word seed = 0, surface = 0, dye = 0, floor = 0, price = 0, repetitions = 0, sequence = 0, length = 0, physical = 0,
         perceived = 0, predicted = 0, choice = 0, actual_cost = 0, grain = 0, food = 0;
    int a = 0, b = 0;
    Word transitions = 0, rate = 0, rule = 0;
};
enum CauseTest : Word {
    new_route,
    erased_model,
    wrong_model,
    old_prices,
    no_prices,
    passive_only,
    short_route,
    long_route,
    double_action,
    never_reworked,
    cause_count
};
} // namespace axion::life::effort
namespace axion::life::effort {
struct Earthwork {
    Word strokes = 0, known = 0, expected = 0;
    int charge = 0;
};
struct Paving {
    Word plots = 0, indicated = 0, tested = 0, best = 0, worth = 0, benefit = 0, harm = 0, work = 0, plain = 0,
         perfect = 0, known = 0, two = 0;
};
struct Survey {
    Word possible = 0, picked = 0, correct = 0, better_here = 0, better_there = 0, correct_here = 0, correct_there = 0,
         ties = 0, fruit = 0, work = 0, regret = 0, ordinary = 0, bare = 0, compacted = 0, known = 0;
};
} // namespace axion::life::effort
