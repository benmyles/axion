#pragma once
#include "linkage_design.hpp"
namespace axion::life::linkage {
struct Exchange {
    Word samples = 0, available = 0, drafted = 0, built = 0, threaded = 0, finished = 0, food = 0, spool = 0,
         indispensable = 0, indispensable_delivered = 0, paid = 0, maker_cost = 0, pupil_plans = 0, mistaken = 0,
         unvisited = 0;
};
struct Culture {
    Word n = 0, possible = 0, adults_fed = 0, done = 0, long_tasks = 0, long_done = 0, indirect = 0,
         indirect_done = 0, demonstrations = 0, seen = 0, cost = 0, edits = 0, requested = 0, productive = 0,
         food = 0, allowed = 0, missing = 0;
};
enum Rehearsal : Word {
    shared_work,
    obscured,
    final_only,
    wrong_actor,
    other_seam,
    no_priming,
    owner_learn,
    culture_count
};
enum Legacy : Word { as_made, unstrung, extra_loop, wrong_goal, no_private_memory, one_hand, legacy_count };
} // namespace axion::life::linkage
