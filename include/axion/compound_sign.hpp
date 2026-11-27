#pragma once
#include "material_signal.hpp"

namespace axion::life::capillary {
struct CompoundCourse {
    Word observed = 4, arbor = 0, diet = 0, wean = 0;
};
struct CompoundMind {
    Body body{};
    Word tissue = 0, fields = 0;
};
AX_FN Word compound_fields(const Body &b, Word expression) {
    Word physical = channels(b.dendrites) & 3;
    Word left = (b.growth.program.first >> 22) & 3, right = (b.growth.program.second >> 22) & 3;
    if (expression == 1)
        left = right = 3;
    return (physical & left) | ((physical & right) << 2);
}
AX_FN CompoundMind compound_body(Body b, Word expression, Word tissue = 0) {
    return {b, tissue, compound_fields(b, expression)};
}
AX_FN Word compound_shift(Word port, Word sensed, Word movement) {
    return 2 * ((port << 3) | (sensed << 1) | movement);
}
AX_FN Word compose_move(CompoundMind &m, Word sensation, Word coin) {
    Word action = 0;
    for (Word port = 0; port < 2; ++port) {
        Word mask = (m.fields >> (2 * port)) & 3;
        Word signal = m.body.eyes ? (sensation & mask) : 0;
        Word motor = port ? regulation::rolled(m.body.motor, 13) : m.body.motor;
        Word pair = m.tissue >> compound_shift(port, signal, 0);
        int values[2]{appetitive_value(pair, motor), appetitive_value(pair >> 2, motor)};
        Word afference = (1u << signal) | (1u << (port + 4));
        action |= effector_choice(m.body, afference, values, bool(m.tissue & (0xffffu << (port * 16))),
                                  mix(coin + port * 12011), motor)
                  << port;
    }
    return action;
}
AX_FN Word nourish_ports(CompoundMind &m, Word sensed, Word action, Word metabolic) {
    Word edits = 0;
    for (Word port = 0; port < 2; ++port) {
        Word input = m.body.eyes ? sensed & ((m.fields >> (2 * port)) & 3) : 0;
        Word at = compound_shift(port, input, (action >> port) & 1), prior = (m.tissue >> at) & 3, spikes = 0;
        Word next = admitted_sign(m.body, prior, (metabolic & (1u << port)) ? 1u : 2u, spikes, 1, nullptr, 0);
        m.tissue ^= (next ^ prior) << at;
        edits += __builtin_popcount(next ^ prior);
    }
    return edits;
}
AX_FN Word nourish(CompoundMind &m, Word sensed, Word action, Word reward) {
    return nourish_ports(m, sensed, action, reward ? 3u : 0u);
}
// Four physical feeders each hold zero, one or two divisible nutrient pairs.
// Two intersecting streams carry independent nutrients along the two spatial axes.
AX_FN Word nutrient_bed(Word spring, bool exchange) {
    Word stock = 0;
    for (Word site = 0; site < 4; ++site) {
        Word x = (spring & 1) == (site & 1), y = (spring >> 1) == (site >> 1);
        stock |= (exchange ? (y | (x << 1)) : (x | (y << 1))) << (2 * site);
    }
    return stock;
}
AX_FN Word taste_ports(const Body &animal, Word crop, Word diet) {
    if (diet != 1)
        return crop ? 3u : 0u;
    return Word(bool(crop & (animal.palate & 3))) | (Word(bool(crop & ((animal.palate >> 2) & 3))) << 1);
}
AX_FN Word take_crop(Word &stock, Word site) {
    Word result = (stock >> (2 * site)) & 3;
    stock &= ~(3u << (2 * site));
    return result;
}
struct CompoundBed {
    Junction cell[2]{};
    Word optics = 0;
};
// Two optical responses per junction, computed by operating the constructed material.
AX_FN Word projector(Junction j, bool inert) { return outlet(j, false, inert) | (outlet(j, true, inert) << 1); }
AX_FN Word compound_light(Word optics, Word action) {
    return ((optics >> (action & 1)) & 1) | (((optics >> (2 + ((action >> 1) & 1))) & 1) << 1);
}
struct CompoundTurn {
    Word cue, mark, sensed, choice, without_first, without_second, blank, food, cost;
};
using PackedReport = unsigned short;
static_assert(sizeof(PackedReport) == 2);
AX_FN PackedReport pack_report(CompoundTurn v) {
    return PackedReport(v.mark | (v.sensed << 2) | (v.choice << 4) | (v.without_first << 6) | (v.without_second << 8) |
                        (v.blank << 10) | (v.food << 12) | (v.cost << 13));
}
AX_FN CompoundTurn unpack_report(Word cue, PackedReport result) {
    Word v = result;
    return {cue, v & 3, (v >> 2) & 3, (v >> 4) & 3, (v >> 6) & 3, (v >> 8) & 3, (v >> 10) & 3, (v >> 12) & 1, v >> 13};
}
AX_FN Word gather_meal(Word cue, Word choice) {
    Word stock = 2u << (2 * cue);
    return take_crop(stock, choice) >> 1;
}
AX_FN CompoundTurn compound_turn(CompoundMind &writer, CompoundMind &reader, Word optics, Word cue, Word noise,
                                 bool controls) {
    Word mark = compose_move(writer, cue, mix(noise + 49277)), muscle = mix(noise + 24407),
         light = compound_light(optics, mark);
    CompoundTurn r{cue, mark, light, 0, 0, 0, 0, 0, 3u + Word(__builtin_popcount(mark))};
    if (controls) {
        auto a = reader, b = reader, c = reader;
        r.without_first = compose_move(a, compound_light(optics, mark & 2), muscle);
        r.without_second = compose_move(b, compound_light(optics, mark & 1), muscle);
        r.blank = compose_move(c, compound_light(optics, 0), muscle);
    }
    r.choice = compose_move(reader, light, muscle);
    r.food = gather_meal(cue, r.choice);
    return r;
}
enum CompoundMetric : Word {
    cold_four,
    hot_four,
    one_removed,
    two_removed,
    both_removed,
    cold_all,
    hot_all,
    hot_zero,
    hot_one,
    hot_two,
    hot_three,
    fluent,
    mixed,
    axiswise,
    prefluent,
    rearing_food,
    rearing_cost,
    source_edits,
    listener_edits,
    trained_zero,
    trained_one,
    trained_two,
    trained_three,
    first_crop,
    second_crop,
    compound_metrics
};
struct CompoundGrade {
    Word value[compound_metrics]{}, writer = 0, reader = 0;
};
struct CompoundLife {
    CompoundGrade grade{};
    Word identity = 0, initial_code = 0, code = 0, writer = 0, reader = 0;
};
AX_FN Word codeword(Count counts) {
    Word best = 0;
    for (Word i = 1; i < 4; ++i)
        if (((counts >> (16 * i)) & 65535) > ((counts >> (16 * best)) & 65535))
            best = i;
    return best;
}
} // namespace axion::life::capillary
