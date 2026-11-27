#pragma once
#include "linkage.hpp"
namespace axion::life::linkage {
constexpr Word no_edit = 6;
struct Design {
    Program turns{};
    Word edit = no_edit, found = 0, effort = 0, faces = 0;
};
struct Fabrication {
    Word n = 0, achieved = 0, possible = 0, optimal = 0, threaded = 0, crucial = 0, crucial_solved = 0,
         unnecessary = 0, false_joins = 0, paid = 0, carbon = 0, turned = 0, wrong = 0, unanswered = 0, baseline = 0,
         predicted = 0, actual = 0;
};
struct Drawing {
    Word seed = 0, glyphs = 0, joints = 0, sockets = 0, grip = 0, start = 0, goal = 0, care = 0, edit = no_edit,
         new_joints = 0, using_joints = 0, last = 0, after_builder = 0, remaining = 0, spent = 0, food = 0,
         optimal = 0, untouched = 0;
    Program plan{};
};
enum BuildTest : Word {
    assembled,
    retained,
    shuffled_splice,
    no_splice_model,
    no_relation_model,
    empty_spool,
    borrowed_parts,
    only_spin,
    no_observation,
    public_shop,
    manual_shop,
    build_count
};
struct Handoff {
    Word n = 0, ready = 0, worked = 0, mentor_work = 0, watched = 0, copied = 0, executed = 0, food = 0, indirect = 0,
         changed = 0, unknown = 0;
};
} // namespace axion::life::linkage
namespace axion::life::linkage {
struct Peripheral {
    Word n = 0, expressed = 0, attempted = 0, finished = 0, spent = 0, mistaken = 0, minimum = 0, improved = 0;
};
} // namespace axion::life::linkage
