#pragma once
#include "material.hpp"

namespace axion::life::material {
// Two-bit synapses. One row predicts a visible pulse crossing each pair
// of terminals. Four rows predict changes to the hand, and one row
// associates a sensory surface with edible product. Spare bits in the appetite
// word record which muscles and terminal pairs this individual has sampled.
struct Foresight {
    Word contacts = 0, muscles[2]{}, appetite = 0;
};
struct SpatialImage {
    Half contacts = 0;
    Byte motor[4]{}, taste = 0, reserved = 0;
};
static_assert(sizeof(SpatialImage) == 8);
struct TideTotals {
    Count looks = 0, alternatives = 0, lifts = 0, placements = 0, turns = 0, work = 0;
};
struct TideState {
    Word period = 16, active = 3;
    TideTotals totals{};
};
struct TidalField {
    Foresight *cells = nullptr;
    TideTotals *account = nullptr;
    Half *sight = nullptr;
    Word period = 16, active = 1;
};
struct Coast {
    Word sun = 0, first = 0, second = 0, occupied = 0;
    Word glass = 0, tendons = 0, odor = 0, responsive = 0;
    Word catalyst = 0, contrast = 1;
};
struct Construction {
    Foresight memory{};
    Genes genes{};
};
struct ShoalScore {
    Word reactions = 0, sealed = 0, original = 0;
    Word work = 0, displacements = 0, rotations = 0, parts = 0;
    Word essential = 0, joints = 0, stored = 0, photons = 0;
    Word alternative = 0, forecast = 0, candidates = 0, reserve = 0;
    Word trace_size = 0, stranded_food = 0, seed = 0, invalid = 0;
};
struct ShoalTrace {
    Word before[32]{}, after[32]{}, origin[32]{}, actions[32]{};
    Coast coast{};
    Foresight memory{};
    Genes genes{};
};
struct ShoalOptions {
    Word practice = 96, moves = 8, ideas = 64, span = 3, exposure = 64;
};
static_assert(sizeof(Foresight) == 16 && sizeof(ShoalScore) == 76);
} // namespace axion::life::material
namespace axion::life::material {
struct ShoreTray {
    Coast coast{};
    Foresight memory{};
    Genes genes{};
    Word sites[32]{}, origin[32]{}, raw[2]{}, food[2]{};
    Grain hand{};
    Word held = 32, seed = 0, visits = 0, selected = 0;
};
struct ShoreReply {
    Word found = 0, certain = 0, acted = 0, cost = 0, made = 0, absorbed = 0, collected = 0, choice = 0;
    Word predicts = 0, targets = 0, first = 0, second = 0, sun = 0, pigment[2]{}, raw[2]{}, food[2]{}, cargo = 0,
         response[2]{};
};
} // namespace axion::life::material
