#pragma once
#include "braid_route.hpp"
namespace axion::life::braid {
constexpr Word sluices = 1024, hush = 2048, frozen_gates = 4096, shifted_gates = 8192, passive_gates = 16384;
constexpr Word mineral_bed = 32768, mobile_bed = 65536;
struct SedimentBook {
    Count wet = 0, relieved = 0, opened = 0, closed = 0, gained = 0, lost = 0, lodged = 0, scoured = 0, turned = 0;
};
struct GateMemory {
    Half visited = 0, open = 0;
};
struct GateSaved {
    GateMemory gates{};
    Word tag = 0x54414c46u, extent = sizeof(GateSaved);
};
AX_FN bool valid_gate(const GateMemory &m) { return !(m.open & ~m.visited); }
struct GateBook {
    Count wet = 0, crossings = 0, flux = 0, picked = 0, observed = 0, writes = 0, requests = 0, predicted_requests = 0,
          syllables = 0, patterned = 0, responses = 0, understood = 0, pulls = 0, heard_work = 0, better = 0, worse = 0,
          hearing_surplus = 0, hearing_deficit = 0, learned_work = 0, learned_surplus = 0, learned_deficit = 0;
};
AX_FN bool pressure_event(GateMemory &m, Genes g, Word source, Word sink, bool passed) {
    Word bit = 1u << (4 * source + sink);
    Word old = !!(m.open & bit), updated = (g.throat >> (4 + old + 2 * Word(passed))) & 1;
    bool changed = !(m.visited & bit) || old != updated;
    m.visited |= Half(bit);
    m.open = Half((m.open & ~bit) | (updated ? bit : 0));
    return changed;
}
AX_FN Word indicated(const GateMemory &m, Word hue) {
    Word row = (Word(m.visited & m.open) >> (4 * hue)) & 15;
    return row && !(row & (row - 1)) ? Word(__builtin_ctz(row)) : none;
}
struct Transport {
    Word allowed = 0, volume = 0;
};
AX_FN Transport crossing(Half source, Half receiver, Word pressure, Word conductance, Word registration) {
    Word a = mass(source), b = mass(receiver);
    if (!conductance || !pressure || !a || b == 31)
        return {};
    if (((source ^ receiver) & 3) != registration)
        return {1, 0};
    Word flow = pressure < conductance ? pressure : conductance;
    if (flow > a)
        flow = a;
    if (flow > 31 - b)
        flow = 31 - b;
    return {1, flow};
}
} // namespace axion::life::braid
