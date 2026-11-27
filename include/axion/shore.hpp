#pragma once
#include "feedback_contact.hpp"

namespace axion::life::shore {
enum Channel : Word { teaching, quiet, examination };
struct Syllabus {
    braid::Speech names{}, form[2]{};
    Word slot[2]{}, seed = 0, reversal = ~0u, channel = teaching;
};
struct Ledger {
    Count scenes = 0, pointings = 0, presented = 0, tested = 0, answered = 0, correct = 0, changed = 0, both = 0,
          attempts = 0, agreements = 0, changed_names = 0, changed_forms = 0, soma = 0;
};
struct Pupil {
    Word slot = 0, born = 0, founder = 0, generation = 0;
    Count brain_origin = 0, body_origin = 0;
    braid::Narration voice{};
    Ledger book{};
    Ledger after_switch{};
};
AX_FN bool present(const Pupil &p, const flood::Colony &a) {
    return a.energy && a.born == p.born && a.founder == p.founder && a.generation == p.generation &&
           a.brain_origin == p.brain_origin && a.body_origin == p.body_origin;
}
} // namespace axion::life::shore
