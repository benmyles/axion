#pragma once
#include "retina.hpp"

namespace axion::life::retina {
// Bit 21 is used transiently by the field's auditory projection.
constexpr Word iris_port = 1u << 22, sounding_port = 1u << 23, crossed_hands = 1u << 24, lineage_hands = 1u << 25;
enum Probe : Word { ambient, directed, reversed_hands, random_hand, matched_hand };

enum Aperture : Word { passive, voluntary, yoked, open_eye, shut_eye, visible_yoke, natal_eye };

// Rotate decisions within a set of stage positions without changing its count.
AX_FN Word matched_position(Word j, Word layout, Word span, Word shift) {
    Word eligible = (layout & (1u << j)) ? layout : ((1u << span) - 1u) ^ layout;
    Word steps = shift % __builtin_popcount(eligible);
    while (steps) {
        j = (j + 1) % span;
        if (eligible & (1u << j))
            --steps;
    }
    return j;
}
struct GazeLedger {
    Count offered = 0, unique = 0, bought_unique = 0, silent = 0, bought_silent = 0;
};
struct StirCounts {
    Count racks = 0, offered = 0, opened = 0, moved = 0, blocked = 0, divergent = 0, witnesses = 0, changed = 0;
};
struct StirSample {
    Word tick = 0, tile = 0, slot = 0, born = 0, first = 0, other = 0, rest = 0, dye = 0, lamp = 0, places = 0,
         hues = 0;
    Word heading = 0, receptors = 0, plasticity = 0, dendrite = 0, iris_motor = 0, wiring = 0, ports = 0, old = 0,
         spike = 0, tie = 0;
    Word left = 0, right = 0, hand = 0, physical = 0, started = 0, links = 0, destination = 0, blocked = 0;
    Word stock = 0, downstream = 0, stock_after = 0, downstream_after = 0, crossed = 0, lineage = 0;
};
struct StirPatch {
    StirCounts counts{};
    StirSample sample{};
};
struct Fixation {
    Word volley = 0, opened = 0, prediction = 0, onset = 0;
};

AX_FN Word fold_quarters(Word v) {
    v |= v >> 8;
    return (v | (v >> 4)) & 15u;
}

// Four retinotopic motor cells receive three genetically selected inputs apiece.
// Only the onset and the state *before* the second exposure reach these cells.
AX_FN Fixation iris_discharge(regulation::Program program, Word initial_volley, Word predicted, Word lamp, Word spike) {
    // Eight four-cell input populations fit in one register. Gene-selected
    // taps use variable shifts. Blackwell's PRMT evaluates the four cells in parallel.
    Word taps = initial_volley | (predicted << 4) | (frame(predicted, 1) << 8) | (frame(predicted, 2) << 12) |
                (frame(predicted, 3) << 16) | (fold_quarters(spike & 65535u) << 20) | ((1u << lamp) << 24) |
                (fold_quarters(spike >> 16) << 28);
    auto cells = [](Word routes, Word wiring) {
        Word a = (routes >> (4 * (wiring & 7))) & 15, b = (routes >> (4 * ((wiring >> 3) & 7))) & 15,
             c = (routes >> (4 * ((wiring >> 6) & 7))) & 15;
        return drift::lateral_columns(wiring >> 9, (a << 4) | (b << 8) | (c << 12));
    };
    Word first = cells(taps, program.first);
    taps = (taps & 0x0fff0fffu) | (first << 28) | (frame(first, (program.second >> 21) & 3) << 12);
    Word motor = cells(taps, program.second);
    Word threshold = 1 + ((program.second >> 17) & 3);
    return {motor, Word(__builtin_popcount(motor) >= threshold), predicted, initial_volley};
}
AX_FN Fixation orient(const Observer &body, Word rest, Word onset, Word lamp, Word contacts, Word spike, Word ports) {
    Word pre = initial(body.genes, rest, onset, body.heading);
    return iris_discharge(body.iris, pre, advance(pre, contacts, body.wiring, ports), lamp, spike);
}
struct Reach {
    Fixation eye[2]{};
    Word hand = 0, started = 0;
};
AX_FN Reach reach(const Observer &body, const Light &a, const Light &b, Word lamp, Word contacts, Word spikes,
                  Word ports, Word tie) {
    Reach action{{orient(body, a.rest, a.onset, lamp, contacts, spikes, ports),
                  orient(body, b.rest, b.onset, lamp, contacts, spikes, ports)}};
    const auto &left = action.eye[0], &right = action.eye[1];
    Word first = __builtin_popcount(left.volley), second = __builtin_popcount(right.volley);
    action.hand = first == second ? tie & 1u : Word(second > first);
    action.started = left.opened || right.opened;
    return action;
}
AX_FN braid::Episode seen_probe(Word base, Word dye, Word source, Word possible, Word axes, braid::Optics eyes,
                                Word stamp, Word channel, const Light &local) {
    if (channel == closed)
        return {};
    auto scene = braid::through_glass(drift::forecast_view(base, dye, source, possible, axes, stamp, false), eyes);
    if (channel != dark)
        braid::expose(scene, braid::result, local.end, 15);
    return scene;
}
} // namespace axion::life::retina
