#pragma once
#include "context_exposure.hpp"
#include "capillary_tutor.hpp"

namespace axion::life::capillary {
enum StreamMode : Word {
    contextual_learning,
    single_memory,
    examiner_clears,
    unchecked,
    intervention_clamped,
    feedback_exchanged,
    retention_flipped
};
struct StreamCourse {
    Word streams = 0, rounds = 12, change_percent = 45, jitter = 0, mode = 0, second = 32, at = 0, varied = 0;
};
struct MaterialEpoch {
    Word key = 0, rock = 0, change = 0;
    Course lesson{};
};
AX_FN MaterialEpoch epoch(Course c, StreamCourse series, Word life, Word j, Word &variant, Word previous) {
    Word key = mix(c.seed ^ mix(life * 5347 + j * 17471 + 29311));
    if (j && mix(key + 9103) % 100 < series.change_percent)
        variant ^= 16;
    Word base = series.second < 32 && j >= series.at ? series.second : c.rock, rock = base ^ variant;
    if (series.jitter)
        c.views -= mix(key + 3803) % (series.jitter + 1);
    return {key, rock, Word(j && rock != previous), c};
}
enum LifeMetric : Word {
    warm_laws,
    later_laws,
    complete_warm,
    complete_later,
    complete_life,
    first_later,
    changed_laws,
    earned_food,
    life_metrics
};
struct LifeGrade {
    Word native[life_metrics]{}, paired[life_metrics]{}, heat = 0, probes = 0, changes = 0;
};
struct StreamRow {
    Model before{}, observed{}, after{};
    Tissue tissue{};
    History memory{};
    Count fingerprint = 0, spoken = 0;
    Word rock = 0, changed = 0, key = 0, views = 0, probes = 0, heat = 0, edits = 0, spikes = 0, rechecks = 0,
         anchors = 0, earliest = ~0u, design = 0, ready = 0, opening = 0, senses = 0, flux = 0, period = 0,
         passive_looks = 0, terminal = 0;
};
struct Encounter {
    Evidence visible{};
    Word kind = 0, seed = 0;
};
AX_FN Encounter encounter(Word key, Word rock, bool varied) {
    if (varied) {
        Word furniture = mix(key + 14293);
        return {display(furniture, rock, true, furniture & 16), mix(key + 29537) % kinds, mix(key + 30307)};
    }
    auto w = cell(25);
    return {{w, 9, 2, 3, actual(w, 0, rock), actual(w, 9, rock)}, 0, key};
}
struct StreamGrade {
    Count law = 0, goal = 0;
    Word old_exact = 0, watched_exact = 0, exact = 0, caught = 0, possible = 0, said = 0, available = 0, scene = 0,
         handles = 0;
};
AX_FN Word local_agreement(Model m, Count law, Word senses) {
    for (Word i = 0; i < 64; ++i)
        if (predict(m, i & senses) != Word(bool(law & bit(i))))
            return 0;
    return 1;
}
} // namespace axion::life::capillary
