#pragma once
#include "capillary.hpp"

namespace axion::life::capillary {
// Self-initiated contacts and repeated sights can have different predictive value.
// The third word is three signed sensory receipts and their age in attended volleys.
struct History {
    Word passive[2]{}, recent = 0;
};
static_assert(sizeof(History) == 12);
AX_FN Count footprint(History h) { return Count(h.passive[0]) | (Count(h.passive[1]) << 32); }
AX_FN bool valid(History h) {
    bool empty = false;
    for (Word k = 0; k < 3; ++k) {
        Word row = (h.recent >> (8 * k)) & 255;
        if (!row) {
            empty = true;
            continue;
        }
        if (empty || !(row & 128))
            return false;
    }
    return true;
}
AX_FN void remember(History &h, Word row, Word outcome, bool voluntary, Count diagnostic, Word permission) {
    if (!(permission & (outcome ? 1u : 2u)))
        return;
    bool keep = voluntary && bool(diagnostic & bit(row));
    if (!voluntary)
        h.passive[row >> 5] |= 1u << (row & 31);
    Word age = h.recent >> 24;
    if (!keep) {
        if (age < 255)
            h.recent += 1u << 24;
        return;
    }
    Word value = 128u | (outcome << 6) | row, prior = h.recent & 0xffffff;
    if ((prior & 255) == value)
        h.recent = prior;
    else
        h.recent = ((prior << 8) & 0xffffff) | value;
}
struct Revision {
    Fit fit{};
    Count diagnostic = 0, considered = 0;
    Word review = 0, selected = 0;
};
AX_FN Model cover(Tissue base, Word support) {
    Word erased = (~support) & 63;
    return {forget(base.on, erased), forget(base.off, erased)};
}
AX_FN Revision revise(Tissue tissue, History memory, const Body &body) {
    Revision r{};
    Count passive = footprint(memory);
    if (!passive) {
        r.fit = fit(tissue, body.dendrites);
        return r;
    }
    Tissue public_view{tissue.on & ~tissue.off & passive, tissue.off & ~tissue.on & passive};
    auto common = fit(public_view, body.dendrites);
    r.considered = common.supports;
    r.diagnostic = common.rivalry;
    if (!r.diagnostic) {
        r.fit = fit(tissue, body.dendrites);
        return r;
    }
    Word period = 4 + ((body.motor >> 23) & 31), age = memory.recent >> 24;
    r.review = bool((body.motor >> 8) & 31) && (!(memory.recent & 128) || age >= period);
    if (!(memory.recent & 128)) {
        r.fit = fit(tissue, body.dendrites);
        return r;
    }
    Count remaining = common.supports;
    Word depth = 1 + ((body.dendrites >> 20) % 3);
    for (Word k = 0; k < depth; ++k) {
        Word cue = (memory.recent >> (8 * k)) & 255;
        if (!(cue & 128))
            break;
        Count agreeing = 0, undecided = 0;
        for (Count v = remaining; v; v &= v - 1) {
            Word s = first(v);
            auto model = cover(public_view, s);
            Word predicted = predict(model, cue & 63);
            if (predicted == ((cue >> 6) & 1))
                agreeing |= bit(s);
            if (predicted == absent)
                undecided |= bit(s);
        }
        if (agreeing)
            remaining = agreeing | undecided;
    }
    if (remaining == common.supports) {
        r.fit = fit(tissue, body.dendrites);
        return r;
    }
    Model agreed{full, full};
    Count any_on = 0, any_off = 0;
    for (Count v = remaining; v; v &= v - 1) {
        auto m = cover(public_view, first(v));
        agreed.on &= m.on;
        agreed.off &= m.off;
        any_on |= m.on;
        any_off |= m.off;
    }
    r.fit = {agreed, any_on & any_off, remaining, common.degree};
    r.selected = 1;
    return r;
}
AX_FN Search recheck(Tissue t, Revision f, Word present, Word salt, const Body &body, Word purse,
                     bool voluntary = true) {
    if (voluntary && f.review) {
        auto action =
            investigate(t, f.fit, present, 63, salt, body.motor, purse, channels(body.dendrites), f.diagnostic);
        if (action.spent)
            return action;
    }
    return investigate(t, f.fit, present, 63, salt, body.motor, purse, channels(body.dendrites));
}
AX_FN Fit read_material(Tissue t, History h, const Body &b, bool adaptive, bool generalize = true) {
    return adaptive && generalize ? revise(t, h, b).fit : fit(t, b.dendrites, generalize);
}
} // namespace axion::life::capillary
