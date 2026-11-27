#pragma once
#include "material.hpp"
namespace axion::life::material {
struct Rack {
    Grain cell[6]{};
    Byte reach = 2, socket = 1, substrate = 0, spectrum = 3, bevel = 0;
};
struct Brood {
    Genes genes{};
    Word family = 0, live = 0;
};
struct MillBook {
    Word sun = 0, product = 0, flow = 0, folds = 0;
};
struct Season {
    Word initial = 0, brightness = 0, residual = 0, heat = 0, provision = 0, eggs = 0, product = 0, work = 0, moves = 0,
         observations = 0;
};
struct Transfer {
    Word matching = 0, working = 0, product = 0, isolated = 0, mentor = 0, moves = 0;
};
} // namespace axion::life::material
namespace axion::life::material {
struct SessionState {
    Rack work{}, model{};
    Grain drawer[32]{};
    Byte inventory = 0, next = 0;
    Half raw = 96, food = 0, model_raw = 96, model_food = 0;
    Word cursor = 0, key = 0;
};
struct Reading {
    Word operation = 0, changed = 0, spent = 0, product = 0, sun = 0, reached = 0, cargo = 0, site = 0, light = 0,
         ambiguous = 0, topology = 0, contact = 0, stock = 0, ports[6]{}, stored[6]{}, spectra[6]{}, candidates = 0,
         certain = 0, form = 0;
};
} // namespace axion::life::material

namespace axion::life::material {
struct QuarryReading {
    Word reactors = 0, pioneer = 0, improved = 0, harm = 0, food = 0, initial_food = 0, insulated = 0, turns = 0,
         shifts = 0, exchanges = 0, handled = 0, spanning = 0, length = 0;
};
} // namespace axion::life::material

namespace axion::life::material {
struct Tradition {
    Word acquired = 0, ancestral = 0, joint = 0, inducted = 0, signals = 0, opportunities = 0;
};
} // namespace axion::life::material
