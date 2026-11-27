#pragma once
#include "life.hpp"
#include "runnel.hpp"
namespace axion::life::wicks {
constexpr Word flowing = 1, across = 2, present = 1u << 27, trail = (1u << 28) - 1;
struct Totals {
    Count tracers = 0, contacts = 0, water = 0, carbon = 0, transverse = 0, transverse_food = 0, worked = 0,
          worked_food = 0, tracer_moves = 0, later_hops = 0, bridged_hops = 0, strung = 0, frayed = 0, admitted = 0,
          released = 0;
};
struct State {
    Word mode = flowing | across, seed = 0;
    Totals book{};
};
struct Field {
    Word *parcels = nullptr;
    Totals *ledger = nullptr;
    Word mode = 0, sites = 0;
    runnel::Field reeds{};
};
AX_FN Word hops(Word p) { return p & 127; }
AX_FN Word origin(Word p) { return (p >> 7) & 0xfffffu; }
AX_FN Word label(Word home, Word hops) { return present | (home << 7) | hops; }
AX_FN Word edge_bit(Word axis) { return 1u << (30 + axis); }
AX_FN bool valid(Word v, Word n) { return !(v & 0x30000000u) && ((v & present) ? origin(v) < n : !(v & trail)); }
struct Passage {
    Word water = 0, carbon = 0, passable = 0, from_right = 0;
};
} // namespace axion::life::wicks
namespace axion::life::wicks {
struct Result {
    Word n = 0, length = 0, ticks = 0, water_before = 0, water_left = 0, water_caught = 0, food_before = 0,
         food_left = 0, food_caught = 0, strand_before = 0, strand_after = 0, spare = 0, spent = 0, distance = 0,
         hops = 0;
};
} // namespace axion::life::wicks
