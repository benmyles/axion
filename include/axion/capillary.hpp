#pragma once
#include "narrative_world.hpp"

namespace axion::life::capillary {
using utterance::Byte;
constexpr Word inputs = 6, states = 1u << inputs, absent = 2;
constexpr Count full = ~Count(0);
AX_FN Word count(Count bits) { return __builtin_popcountll(bits); }
AX_FN Word first(Count bits) { return __builtin_ctzll(bits); }
AX_FN Count bit(Word slot) { return Count(1) << slot; }
AX_FN Count lower(Word axis) { return full / ((Count(1) << (1u << axis)) + 1); }
AX_FN Count across(Count sheet, Word axis) {
    Word distance = 1u << axis;
    Count lo = lower(axis);
    return ((sheet & lo) << distance) | ((sheet & ~lo) >> distance);
}
AX_FN Count translate(Count sheet, Word axes) {
    for (Word k = 0; k < inputs; ++k)
        if ((axes >> k) & 1)
            sheet = across(sheet, k);
    return sheet;
}
AX_FN Count forget(Count sheet, Word axes) {
    for (Word k = 0; k < inputs; ++k)
        if ((axes >> k) & 1)
            sheet |= across(sheet, k);
    return sheet;
}
struct Tissue {
    Count on = 0, off = 0;
};
struct Body {
    utterance::Biology growth{};
    regulation::Cells cells{};
    Word dendrites = 0, motor = 0, eyes = 0, limbs = 0, mouth = 0, module_mask = 0, pressure_xor = 0, palate=0;
};
// A mutable routing word belongs to each actual neural module. Its upper nibble
// can independently regulate the four entries of a material synapse's update table.
AX_FN Word pressure_regulator(const flood::Germ &g, Word bands) {
    if (!bands)
        return 0;
    Word band = regulation::innervated_band(g, bands);
    return g.module[regulation::module_index(g, band)].routing >> 28;
}
struct Mind {
    Tissue tissue{};
    Word before = 0, after = 0;
};
static_assert(sizeof(Tissue) == 16);
AX_FN Word channels(Word gene) {
    // Each inherited inhibitory branch can disconnect a sensory afferent.
    return 63u & ~(gene & (gene >> 6) & (gene >> 12) & (gene >> 18));
}
AX_FN Word width(Word gene) { return 3 + ((gene >> 26) & 3); }
AX_FN Word admitted_sign(Body &body, Word old, Word evidence, Word &spikes, Word gate_mode, Word *permission,
                         Word law_mask) {
    if (permission)
        *permission = 0;
    if (!body.eyes || !(body.growth.arbor & (1u << 14)))
        return old;
    auto b = body.growth;
    b.gated = gate_mode;
    Word proposal = braid::rule_bits(b.law ^ law_mask, old, evidence) & 3;
    utterance::Growth change{};
    change.cells = body.cells;
    Word release = utterance::conducting(b, change, old, evidence, proposal, old != 0, old == evidence, 3);
    body.cells = change.cells;
    spikes += change.firings;
    if (permission)
        *permission = release;
    return regulation::apply(old, proposal, release);
}
AX_FN Word receive(Tissue &m, Body &body, Word x, bool pulse, Word &spikes, Word gate_mode = 1,
                   Word *permission = nullptr, Word law_mask = 0) {
    Count site = bit(x & channels(body.dendrites));
    Word old = Word(bool(m.on & site)) | (Word(bool(m.off & site)) << 1);
    Word next = admitted_sign(body, old, pulse ? 1u : 2u, spikes, gate_mode, permission, law_mask);
    if ((next ^ old) & 1)
        m.on ^= site;
    if ((next ^ old) & 2)
        m.off ^= site;
    return __builtin_popcount(next ^ old);
}
AX_FN Word material_receive(Tissue &t, Body &body, Word x, bool pulse, Word &spikes, Word gating = 1,
                            Word *permission = nullptr) {
    return receive(t, body, x, pulse, spikes, gating, permission, body.pressure_xor);
}
// Spatial inference only reads these two masks. Their bits record well-defined
// positive and negative responses to each possible six-afferent spike volley.
struct Model {
    Count on = 0, off = 0;
};
struct Fit {
    Model model{};
    Count rivalry = 0, supports = 0;
    Word degree = 0;
};
constexpr Count cardinalities(Word n) {
    Count mask = 0;
    for (Word j = 0; j < states; ++j)
        if (Word(__builtin_popcount(j)) == n)
            mask |= Count(1) << j;
    return mask;
}
AX_FN Count layer(Word n) {
    switch (n) {
    case 0:
        return cardinalities(0);
    case 1:
        return cardinalities(1);
    case 2:
        return cardinalities(2);
    case 3:
        return cardinalities(3);
    case 4:
        return cardinalities(4);
    case 5:
        return cardinalities(5);
    default:
        return cardinalities(6);
    }
}
AX_FN Fit fit(Tissue observed, Word gene, bool generalize = true) {
    Fit r{};
    Count conflict = observed.on & observed.off;
    Count on = observed.on & ~conflict, off = observed.off & ~conflict;
    r.model = {on, off};
    r.degree = inputs;
    if (!generalize || (!on && !off))
        return r;
    // A candidate set of afferents must distinguish every incompatible pair
    // of witnessed volleys. One 64-bit upward closure tests all 64 supports.
    Count discrepancies = conflict ? 1 : 0, small = on, other = off;
    if (count(off) < count(on)) {
        small = off;
        other = on;
    }
    while (small) {
        Word x = first(small);
        small &= small - 1;
        discrepancies |= translate(other, x);
    }
    for (Word k = 0; k < inputs; ++k)
        discrepancies |= (discrepancies & lower(k)) << (1u << k);
    Count permitted = translate(~discrepancies, 63);
    Word sensors = channels(gene);
    for (Word k = 0; k < inputs; ++k)
        if (!(sensors & (1u << k)))
            permitted &= lower(k);
    Word cap = width(gene);
    for (Word degree = 0; degree <= cap; ++degree) {
        auto candidates = permitted & layer(degree);
        if (!candidates)
            continue;
        Count yes = full, no = full, maybe_yes = 0, maybe_no = 0;
        r.supports = candidates;
        r.degree = degree;
        while (candidates) {
            Word ignored = (~first(candidates)) & 63;
            candidates &= candidates - 1;
            Count a = forget(on, ignored), b = forget(off, ignored);
            yes &= a;
            no &= b;
            maybe_yes |= a;
            maybe_no |= b;
        }
        r.model = {yes & ~conflict, no & ~conflict};
        r.rivalry = maybe_yes & maybe_no;
        break;
    }
    return r;
}
AX_FN Word predict(Model m, Word x) {
    Count q = bit(x);
    return m.on & q ? 1u : m.off & q ? 0u : absent;
}
// Regional mineral resistance, compressibility and port cross section affect
// head. All sites obey this same local threshold, including animal-built ones.
AX_FN bool surge(Word x, Word mineral) {
    Word a = x & 1, b = (x >> 1) & 1, wick = (x >> 2) & 1, wet = (x >> 3) & 1, sill = (x >> 4) & 1;
    if (mineral & 16)
        wick = (x >> 5) & 1;
    Word head = a * (1 + ((mineral >> 2) & 1)) + b * (1 + ((mineral >> 3) & 1));
    return wet && head && head + wick * (1 + (mineral & 1)) >= 1 + sill * (1 + ((mineral >> 1) & 1));
}
struct Search {
    Word choice = 0, spent = 0;
};
AX_FN Search investigate(Tissue tissue, Fit forecast, Word present, Word options, Word salt, Word motor, Word allowance,
                         Word senses = 63, Count eligible = full) {
    unsigned best = 0;
    Search selected{present, 0};
    for (Word row = 0; row < states; ++row) {
        if (!(eligible & bit(row & senses)))
            continue;
        Word change = row ^ present;
        if (change & ~options)
            continue;
        Word work = 2 + __builtin_popcount(change);
        if (work > allowance)
            continue;
        Count stimulus = bit(row & senses);
        Word new_contact = !((tissue.on | tissue.off) & stimulus);
        Word open = !((forecast.model.on | forecast.model.off) & stimulus);
        Word clash = bool(forecast.rivalry & stimulus);
        Word value = new_contact * (1 + (motor & 15)) + open * ((motor >> 4) & 15) + clash * ((motor >> 8) & 31) +
                     Word(bool(forecast.model.on & stimulus)) * ((motor >> 13) & 7) +
                     (6 - __builtin_popcount(change)) * ((motor >> 16) & 7);
        unsigned priority = (value << 10) | ((mix(salt + row * 919) & 1023u));
        if (!selected.spent || priority > best) {
            best = priority;
            selected = {row, work};
        }
    }
    return selected;
}
} // namespace axion::life::capillary
