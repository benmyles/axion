#pragma once
#include "worksite.hpp"
#include "grounding.hpp"
#include "solvent.hpp"
namespace axion::life::material {
struct FieldQuestion {
    Count wave[8]{};
    Word length = 0, slot = 0, contradictions = 0, examples = 0;
};
struct FieldLanguage {
    FieldQuestion question[4]{};
    Word count = 0, examples = 0, signature = 0x51464c44u, extent = sizeof(FieldLanguage);
};
struct Samples {
    Grain chip[2]{};
    Word soil[2]{}, pigment[4]{}, site[4]{}, observed[2]{}, measured = 0, seed = 0, solution = 0, tried = 0, taken[2]{},
                                                            witnesses = 0;
};
struct FieldUtterance {
    Count wave[8]{};
    Word length = 0;
};
struct FieldAnswer {
    Count wave = 0, foil = 0, names[4]{};
    Word recognized = 0, referent = 2, destination = 2, eligible = 0, expected = 0, evidence = 0, paid = 0,
         calories = 0, questions = 0, uncertain = 0, prior_uncertain = 0;
};
struct ListenerInput {
    Grain piece{};
    Word pigments[2]{}, soil[2]{}, sites[2]{}, solution = 0;
};
struct SampleRequest {
    Count wave = 0;
    Word chip = 0, suggested = 2, raw[2]{}, seen = 16, outcome = 0, stop = 0;
};
struct ConversationTest {
    Count held[4]{};
    FieldUtterance form[2]{};
    Word trials = 0, seed = 0, species = 0, limit = 0, speaker_energy = 0, peer_energy = 0, intention = 0;
};
struct ConversationCounts {
    Word offered = 0, asked = 0, before = 0, correct = 0, wrong = 0, silent = 0, acted = 0, arrived = 0,
         feed_requests = 0, harvested = 0, inactive_harvests = 0;
    Word requested = 0, shown = 0, observed = 0, held_contacts = 0, switched = 0, genuine_switch = 0, paid = 0,
         reference_food = 0, policy_agreement = 0;
};
struct ListenerResult {
    Word chosen = 32, paid = 0, calories = 0;
};
} // namespace axion::life::material
