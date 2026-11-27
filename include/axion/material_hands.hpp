#pragma once
#include "compound_sign.hpp"

namespace axion::life::capillary {
constexpr Word material_hands = 4096, fixed_hands = 8192, disconnected_hands = 16384;
struct HandCounts {
    Count attempts = 0, paid = 0, unavailable = 0, placed = 0, lifted = 0, food = 0, edits = 0, retained_moves = 0,
          different_part = 0, different_flow = 0, created_flow = 0, removed_flow = 0, new_food = 0, young = 0,
          direction[4]{};
};
struct HandNote {
    Word tick = 0, site = 0, slot = 0, born = 0, fields = 0, sight = 0, geology = 0, coating = 0, carrier = 0,
         reactive = 0, stone = 0, before = 0, actual = 0, cold_row = 0, untouched_flow = 0, flow = 0, cold_flow = 0,
         choice = 0, cold_choice = 0, possible = 0, cold_possible = 0, executed = 0, donation = 0, edge0 = 0, edge1 = 0,
         stock0 = 0, stock1 = 0, earlier_diet = 0, later_diet = 0, food = 0, edits = 0, tissue0 = 0, tissue1 = 0;
};
struct Hands {
    unsigned char *pending = nullptr;
    HandCounts *totals = nullptr;
    HandNote *camera = nullptr;
    Word samples = 0;
};
struct HandOwner {
    Word born = 0, signs = 0;
};
} // namespace axion::life::capillary
