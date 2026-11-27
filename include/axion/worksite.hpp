#pragma once
#include "foresight.hpp"

namespace axion::life::material {
// The lower sixteen bits of sampled describe observed surface/soil pairs.
// The upper sixteen distinguish muscle experience with an empty or full palm.
struct Inquiry {
    Word receptors = 0, impulses = 0;
    Word chemistry = 0, sampled = 0, anticipation = 0, last_seen = 0;
};
struct Imprint {
    Word events = 0, tick = 0;
};
struct WorksiteTotals {
    Count released = 0, visits = 0, actions = 0, contacts = 0, scrapes = 0, reactions = 0;
    Count pumped = 0, heat = 0, eaten = 0, readings = 0, before_birth = 0, inscriptions = 0;
};
struct WorksiteState {
    Word active = 1, marks = 1;
    WorksiteTotals totals{};
};
struct Worksites {
    Inquiry *residents = nullptr;
    Imprint *ground = nullptr;
    WorksiteTotals *ledger = nullptr;
    Word active = 0, marks = 0;
};
static_assert(sizeof(Inquiry) == 24 && sizeof(Imprint) == 8 && sizeof(WorksiteTotals) == 96);
} // namespace axion::life::material
namespace axion::life::material {
struct FieldPupil {
    Construction mind{};
    Inquiry exploratory{};
    Word age = 0, location = 0;
};
struct FieldTransfer {
    Word trials = 0, terminals = 0, substrate = 0, selective = 0, reactors = 0, needed = 0, food = 0, other = 0;
    Word exclusively = 0, known_contacts = 0, known_soils = 0, lift = 0, place = 0, marks = 0, original = 0, sealed = 0;
};
} // namespace axion::life::material
namespace axion::life::material {
struct SedimentTotals {
    Count adsorbed = 0, released = 0, presses = 0, meals = 0, alternatives = 0, differential = 0;
};
struct SedimentState {
    Word pattern = 1, guidance = 1;
    Count opening = 0;
    SedimentTotals totals{};
};
struct Sediment {
    Half *stock = nullptr;
    SedimentTotals *account = nullptr;
    Word pattern = 0, guidance = 0;
};
} // namespace axion::life::material
