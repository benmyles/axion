#pragma once
#include "practice.hpp"
#include "linkage_design.hpp"
namespace axion::life::practice {
constexpr Word no_probe = 8, limit = 4;
struct Request {
    linkage::Rig rig{};
    linkage::Pattern goal{};
    Word sockets = 0, view = 0, work = 8;
};
struct Script {
    linkage::Design reply[4]{};
    Word ask = no_probe, look = 4, outcomes = 0, found = 0, worst = 0, sum = 0, prior = 0;
};
struct Charts {
    linkage::Map model[4]{};
    Word count = 0;
};
struct Score {
    Word trials = 0, possible = 0, delivered = 0, issued = 0, abstained = 0, mistakes = 0, paid = 0, harvest = 0,
         questions = 0, answers = 0, indirect = 0, branches = 0, good_branches = 0, unconditional = 0, shifts = 0,
         active_contexts = 0, instances = 0, splices = 0, prep = 0, unneeded = 0;
};
struct Specimen {
    Request request{};
    Script script{};
    Word seed = 0, world = 0, law[2]{}, chosen = 0, final = 0, report = 4, alternate = 4, acting = 0;
};
enum Exam : Word {
    informed,
    silence,
    no_test,
    erased,
    merged,
    shifted,
    placebo,
    one_exposure,
    own_contacts,
    unguided,
    short_memory,
    discarded,
    exam_count
};
} // namespace axion::life::practice
namespace axion::life::practice {
struct Day {
    Word trials = 0, born = 0, departures = 0, turns = 0, repairs = 0, stomach = 0, heat = 0, reserve = 0, meals = 0,
         grazed = 0, opens = 0, solved = 0, inspected = 0, longest = 0, survived = 0, stable_choices = 0,
         rewarded_stays = 0, stale_stays = 0, learned = 0, decisions = 0, available_food = 0, unconsumed = 0;
};
} // namespace axion::life::practice
namespace axion::life::practice {
struct Windows {
    Word ticks = 0, own_stays = 0, swapped_stays = 0, own_food = 0, swapped_food = 0, own_work = 0, swapped_work = 0,
         own_reserve = 0, swapped_reserve = 0, own_springs = 0, swapped_springs = 0, longer = 0, shorter = 0;
};
} // namespace axion::life::practice
