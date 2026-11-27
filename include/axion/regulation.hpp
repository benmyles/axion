#pragma once
#include "floodplain.hpp"

namespace axion::life::regulation {
constexpr Word motor_port = 1u << 18, silent_feedback = 1u << 19, structural_port = 1u << 20;
constexpr Word auditory_port = 1u << 21;
struct Ledger {
    Count contacts = 0, attempted = 0, matched = 0, proposed = 0, admitted = 0, releases = 0, soma_changes = 0;
};
// The physical ports of a pair of 16-cell populations. The two words are the
// routing and gate genes of the module innervating a real somatic band.
struct Program {
    Word first = 0, second = 0;
};
struct Cells {
    Word spike = 0, background = 0;
};
struct Source {
    Program program{};
    Cells cells{};
    Word band = 0;
};
AX_FN Word innervated_band(const flood::Germ &g, Word width) { return (g.brain.receptors >> 27) % width; }
AX_FN Word auditory_band(const flood::Germ &g, Word width) { return (g.hearing.high >> 27) % width; }
AX_FN Word module_index(const flood::Germ &g, Word band) {
    return ((g.allocation >> (2 * (band & 15))) & 3) % g.module_count;
}
AX_FN Source source(const flood::Germ &g, Word band, Tissue tissue) {
    auto module = g.module[module_index(g, band)];
    return {
        {module.routing, module.gate}, {tissue.spike, tissue.memory ^ tissue.gate_a ^ rotate(tissue.gate_b, 11)}, band};
}
AX_FN Word rolled(Word x, Word shift) {
    x &= 65535;
    shift &= 15;
    return ((x << shift) | (x >> ((0u - shift) & 15))) & 65535;
}
struct Ports {
    Word old, conflict, proposal, recurrent, attempted, agreed, body, incoming;
};
AX_FN Word wire(Word which, const Ports &p) {
    switch (which & 7) {
    case 0:
        return p.old;
    case 1:
        return p.conflict;
    case 2:
        return p.proposal;
    case 3:
        return p.recurrent;
    case 4:
        return p.attempted;
    case 5:
        return p.agreed;
    case 6:
        return p.body;
    default:
        return p.incoming;
    }
}
AX_FN Word mux(Word choose, Word a, Word b) { return (choose & a) | (~choose & b); }
AX_FN Word truth(Word table, Word a, Word b, Word c) {
    Word x0 = mux(a, 0u - ((table >> 1) & 1), 0u - (table & 1));
    Word x1 = mux(a, 0u - ((table >> 3) & 1), 0u - ((table >> 2) & 1));
    Word x2 = mux(a, 0u - ((table >> 5) & 1), 0u - ((table >> 4) & 1));
    Word x3 = mux(a, 0u - ((table >> 7) & 1), 0u - ((table >> 6) & 1));
    return mux(c, mux(b, x3, x2), mux(b, x1, x0)) & 65535;
}
AX_FN Word circuit(Word op, const Ports &p) { return truth(op >> 9, wire(op, p), wire(op >> 3, p), wire(op >> 6, p)); }
// Six unused natal bits from the first bank and two from the second can
// reverse any entry of the local write table. A zero allele retains the
// original proposal. Upper-bank spikes still admit individual contacts.
AX_FN Word candidate(Program program, Word spike, Word old, Word proposed, bool structural) {
    if (!structural)
        return proposed;
    Word switches = (program.first >> 26) | ((program.second >> 30) << 6);
    return proposed ^ truth(switches, old, proposed, spike & 65535u);
}
AX_FN Word discharge(Program program, Cells &cells, Word old, Word conflict, Word proposal, bool attempted,
                     bool agreed) {
    Ports p{old,
            conflict,
            proposal,
            rolled(cells.spike, program.first >> 17),
            0u - Word(attempted),
            0u - Word(agreed),
            rotate(cells.background, program.first >> 21),
            rolled(cells.spike >> 16, program.second >> 17)};
    Word local = circuit(program.first, p);
    p.incoming = local;
    p.recurrent = rolled(cells.spike >> 16, program.second >> 21);
    p.body = rotate(cells.background, program.second >> 25);
    Word output = circuit(program.second, p);
    cells.spike = local | (output << 16);
    return output;
}
AX_FN Word apply(Word old, Word proposed, Word release) { return old ^ ((old ^ proposed) & release); }
} // namespace axion::life::regulation
