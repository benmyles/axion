#pragma once
#include "drift.hpp"

namespace axion::life::retina {
struct Genes {
    Word receptors = 0, plasticity = 0;
};
struct Observer {
    Genes genes{};
    Word heading = 0, muscles = 0, senses = 0, wiring = 0;
    braid::Optics eyes{};
    Word contacts = 0, spikes = 0, slot = 0, gate_a = 0, gate_b = 0;
    regulation::Program iris{};
};
struct Banks {
    Word hand = 0, a = 0, b = 0;
    AX_FN Word get(Word i) const { return i == 0 ? hand : i == 1 ? a & 65535u : i == 2 ? a >> 16 : b & 65535u; }
    AX_FN void put(Word i, Word value) {
        if (i == 0)
            hand = value;
        else if (i == 1)
            a = (a & 0xffff0000u) | value;
        else if (i == 2)
            a = (a & 65535u) | (value << 16);
        else
            b = (b & 0xffff0000u) | value;
    }
};
AX_FN Word bank(Genes genes, Word surface) { return (genes.plasticity >> (20 + 2 * surface)) & 3; }
AX_FN Word tide_lamp(Word weather, Word rock) { return (weather + (rock >> 17)) & 3; }
struct Light {
    Word rest = 0, onset = 0, end = 0;
};
enum Channel : Word { closed, paired, displaced, dark, independent };
constexpr Word shift = 21, channel_mask = drift::optical_channel, cadence_shift = 23,
               cadence_mask = drift::optical_cadence;
struct Ledger {
    Count wet = 0, eyes = 0, attended = 0, opening = 0, ending = 0, both = 0, eligible = 0, edits = 0, spikes = 0,
          unobstructed = 0, unpaired = 0, same_source = 0, same_outlet = 0;
};
struct Reception {
    Word pre = 0, post = 0, admitted = 0, spikes = 0, contacts = 0;
};
AX_FN Word frame(Word addresses, Word mapping) {
    if (mapping & 4)
        addresses = (addresses & 5u) | ((addresses & 2u) << 2) | ((addresses & 8u) >> 2);
    Word shift = mapping & 3u;
    return ((addresses << shift) | (addresses >> (4 - shift))) & 15u;
}
AX_FN Word addressed(Word addresses, Word allele, Word heading) {
    // A receptor can compensate for the current proprioceptive heading.
    if (allele & 8u)
        addresses = frame(addresses, (4u - heading) & 3);
    return frame(addresses, allele & 7);
}
AX_FN Word contrast(Word rest, Word exposure, Word bands) {
    Word changed = rest ^ exposure;
    Word even = (changed & (0u - (bands & 1))) | ((changed >> 1) & (0u - ((bands >> 1) & 1)));
    even &= 0x55u;
    return ((even | (even >> 1)) & 3u) | (((even >> 2) | (even >> 3)) & 12u);
}
AX_FN Word rows(Word firing) {
    firing = (firing | (firing << 6)) & 0x303u;
    return ((firing | (firing << 3)) & 0x1111u) * 15u;
}
AX_FN Word initial(Genes g, Word rest, Word exposure, Word heading) {
    return addressed(contrast(rest, exposure, g.receptors), g.receptors >> 4, heading);
}
AX_FN Reception expose(Genes g, Light light, Word heading, Word old, Word soma) {
    Word pre = initial(g, light.rest, light.onset, heading);
    Word post = addressed(contrast(light.rest, light.end, g.receptors >> 2), g.receptors >> 8, heading);
    Word enabled = (g.plasticity >> (16 + 2 * Word(pre != 0) + Word(post != 0))) & 1;
    if (!enabled || !(pre | post))
        return {pre, post, 0, soma, old};
    Word earlier = rows(pre), later = post * 0x1111u;
    // The upper 16 cells carry the onset volley. At the next exposure each
    // lower cell combines its delayed afferent, new afferent and old spike.
    Word firing = regulation::truth(g.plasticity, earlier, later, soma);
    Word contacts = regulation::truth(g.plasticity >> 8, old, earlier, firing);
    return {pre, post, 1, (earlier << 16) | firing, contacts};
}
AX_FN Light project(Light photons, braid::Optics glass) {
    return {braid::project_view(photons.rest, glass), braid::project_view(photons.onset, glass),
            braid::project_view(photons.end, glass)};
}
AX_FN Light reflected(Word resting, Word hue, Word from, Word to) {
    return {resting, drift::overlay(resting, from, hue), drift::overlay(resting, to, hue)};
}
AX_FN Word advance(Word firing, Word contacts, Word wiring, Word ports) {
    if (!(ports & drift::lateral)) {
        Word excited = rows(firing) & ~contacts;
        excited |= excited >> 8;
        return (excited | (excited >> 4)) & 15;
    }
    Word next = 0;
    for (Word j = 0; j < 4; ++j)
        if (firing & (1u << j))
            next |= drift::openings(contacts, j, wiring, ports);
    return next;
}
template <class Signal>
AX_FN Word sequence(Word firing, Banks banks, Genes genes, Word wiring, Word ports, Word laps, bool separate,
                    Signal signal, Word *single = nullptr) {
    if (single)
        *single = 0;
    for (Word j = 0; j < laps && firing; ++j) {
        if (j)
            firing = frame(firing, (genes.receptors >> 24) & 7);
        Word index = separate ? bank(genes, signal(j)) : 0;
        firing = advance(firing, banks.get(index), wiring, ports);
        if (single && !j)
            *single = firing;
    }
    return firing;
}
} // namespace axion::life::retina
