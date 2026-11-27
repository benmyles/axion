#pragma once
#include "braid_trials.hpp"

namespace axion::life::braid {
struct Invention {
    Genes genes[3]{};
    Word count = 0, first[2][voices]{}, final[2][voices]{}, consistent[2]{}, familiar[2]{}, coined[2]{}, emitted[2]{},
         shown[2]{}, practised[2]{}, true_voices[2]{}, paid[2]{}, food[2]{}, muscle[2]{}, rig[2]{};
    struct Outcome {
        Word available = 0, wanted = 0, received = 0, end = 0, strokes = 0, motor = 0;
    };
    Outcome cases[2][56]{};
    Word stage[2][8]{};
};
enum SocialArm : Word {
    lineage,
    imperceptible,
    distant,
    uniform_cups,
    switch_words,
    no_practice,
    solitary,
    cohort_arms
};
} // namespace axion::life::braid
namespace axion::life::braid {
struct RequestWork {
    Word available = 0, spoken = 0, phrase = 0, understood = 0, changed = 0, after = order, pulls = 0, sequence = 0,
         food = 0, exact = 0;
};
struct FilterVillage {
    Invention workshop{};
    RequestWork requests[2][24]{};
    Word born[2]{}, distinct[2]{}, horizon[2]{};
};
} // namespace axion::life::braid
