#pragma once
#include "capillary_course.hpp"
#include "material_context.hpp"

namespace axion::life::capillary {
struct ContactRun {
    Lessons record{};
    History memory{};
    Word deliberate = 0, anchors = 0, switched = 0, first_review = ~0u;
};
template <class Observer>
AX_FN ContactRun context_stream(Body body, Course settings, Word salt, Word rock, Tissue tissue, History memory,
                                bool check, Observer observe) {
    ContactRun e{};
    auto &r = e.record;
    r.tissue = tissue;
    e.memory = memory;
    Word present = 0, seen = channels(body.dendrites);
    for (Word t = 0; t < settings.views + settings.probes; ++t) {
        bool voluntary = t >= settings.views;
        Word cost = 1, row = 0;
        Revision prior{};
        if (voluntary) {
            if (!body.limbs)
                break;
            prior = revise(r.tissue, e.memory, body);
            auto coin = mix(salt ^ mix(t * 11971 + 4711));
            auto hand = recheck(r.tissue, prior, present, coin, body, settings.budget - r.heat, check);
            if (!hand.spent)
                break;
            row = hand.choice;
            cost = hand.spent;
        } else
            row = passive_row(salt, t);
        if (cost > settings.budget - r.heat)
            break;
        Word result = surge(row, rock), input = row & seen, old = e.memory.recent & 255, admitted = 0;
        r.edits += material_receive(r.tissue, body, row, result, r.spikes, settings.gate, &admitted);
        remember(e.memory, input, result, voluntary, prior.diagnostic, admitted);
        if (voluntary) {
            ++r.acts;
            bool relevant = bool(prior.diagnostic & bit(input));
            r.informative += relevant;
            e.deliberate += relevant && check && prior.review;
            e.anchors += relevant && bool(admitted & (result ? 1 : 2));
            if (e.first_review == ~0u && relevant && (admitted & (result ? 1 : 2)))
                e.first_review = t - settings.views;
            if (old && relevant && (old & 63) == input && ((old >> 6) & 1) != result)
                ++e.switched;
            r.actions = (r.actions ^ row) * 1099511628211ull;
        }
        r.heat += cost;
        r.passed += result;
        r.settled += !result;
        ++r.looks;
        present = row;
        observe(t, row, result, voluntary, r.tissue, e.memory);
    }
    r.terminal = body.cells.spike;
    return e;
}
AX_FN ContactRun explore_context(Body body, Course settings, Word salt, Word rock, Tissue tissue, History memory,
                                 bool check = true) {
    return context_stream(body, settings, salt, rock, tissue, memory, check,
                          [](Word, Word, Word, bool, Tissue, History) {});
}
} // namespace axion::life::capillary
