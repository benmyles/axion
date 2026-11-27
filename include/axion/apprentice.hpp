#pragma once
#include "regulation.hpp"
#include "field_class.hpp"
namespace axion::life::apprentice {
using regulation::Program;
using regulation::Source;
enum GateMode : Word { unrestricted, inherited, quiet, displaced, yoked };
struct Teachers {
    Source motor{}, voice{{}, {}, ~0u};
};
constexpr Word remodel = 256;
struct LessonScore {
    Word mechanical[4]{}, predicted[4]{}, divergent[4]{}, divergent_right[4]{}, mechanical_mask[4]{}, physical[4]{},
        responses[4]{};
    Word names = 0, forms = 0, qualified = 0, correct = 0, answered = 0, offered = 0, changed = 0, both = 0,
         unseen_answers = 0;
    Word reply_mask = 0, replies = 0, probe_mask = 0, probe_replies = 0;
    Word updates = 0, released = 0, attempts = 0, matches = 0, intermediate = 0;
};
struct Donor {
    Program program{};
    Word law = 0, seed = 0, generations = 0, ancestor = 0, harvest = 0, offered = 0;
    Word field = 0, tick = 0, copies = 0, population = 0;
    Word coupled = 0, wiring = 0, hand_active = 0;
};
struct Carriers {
    Count alive = 0, exact = 0, newborn = 0, both_ports = 0, neighbours = 0;
};
struct Animal {
    Program program{};
    Word traits = 0, ancestor = ~0u;
};
static_assert(sizeof(Program) == 8 && sizeof(Animal) == 16);
struct Rations {
    Count carbon = 0, washed = 0, offspring = 0, left = 0, adults = 0;
};
struct Harvest {
    Word swallowed = 0, escaped = 0, after_shift = 0, after_return = 0, confident = 0, matches = 0;
};
} // namespace axion::life::apprentice
