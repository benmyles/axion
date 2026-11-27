#pragma once
#include "axion/pods.hpp"

namespace axion::life::pods {
AX_FN Word sensory_channels(const Physics &p, Word cohort) {
    Word excluded = (p.background >> 5) & 3;
    return excluded && cohort == excluded ? p.background & ~4u : p.background;
}
AX_FN Word lower(Word a, Word b) { return a < b ? a : b; }
AX_FN Genes founder(Word key) { return {mix(key + 5731), mix(key + 7219), mix(key + 1711), mix(key + 2381)}; }
AX_FN Genes descend(Genes g, Word key, Word rate) {
    Word *ports[] = {&g.plastic, &g.timing, &g.readout, &g.sense};
    for (Word k = 0; k < 4; ++k) {
        Word r = mix(key + 911 * k);
        if ((r & 255) < rate)
            *ports[k] ^= 1u << ((r >> 8) & 31);
    }
    return g;
}
// This control germline supplies a transparent increment/decrement law. Wild
// founders start with random bit strings and do not inherit these constants.
AX_FN Genes reference() {
    Genes g;
    for (Word state = 0; state < 4; ++state) {
        g.plastic |= (state ? state - 1 : 0) << (2 * state);
        g.plastic |= lower(3, state + 1) << (8 + 2 * state);
        g.readout |= ((1u << state) - 1) << (4 * state);
    }
    g.timing = 0x800000ffu | (0x7bd1u << 8);
    g.sense = 0xff000000u | (1u << 3) | (2u << 6);
    return g;
}
AX_FN Word receptor(Word light, const Genes &g) {
    Word x = 0;
    for (Word k = 0; k < 3; ++k)
        x |= ((light >> (g.sense >> (3 * k) & 7)) & 1) << k;
    return x ^ ((g.sense >> 9) & 7);
}
AX_FN Word group(Word receptor, const Genes &g) { return (receptor >> ((g.sense >> 12) % 3)) & 1; }
AX_FN Word discharge(Word state, const Genes &g) { return __builtin_popcount((g.readout >> (4 * state)) & 15); }
AX_FN Word contact(Word cue, Word muscle) { return 4 * cue + muscle; }
// One bit from each packed synapse selects the eligible lag. The four
// population counts occupy separate bytes, including states with zero firing.
AX_FN Word volleys(const Circuit &brain, const Genes &genes, Word cue) {
    Word enabled = genes.timing & 255, mask = 0;
    const auto *support = brain.support[group(cue, genes)];
    int peak = -128;
    for (Word t = 0; t < 8; ++t)
        if ((enabled & (1u << t)) && support[t] > peak)
            peak = support[t];
    for (Word t = 0; t < 8; ++t)
        if ((enabled & (1u << t)) &&
            ((genes.timing & (1u << 27)) || support[t] + int((genes.timing >> 24) & 7) >= peak))
            mask |= 1u << (2 * t);
    Word spike[4]{};
    for (Word z = 0; z < 4; ++z)
        spike[z] = discharge(z, genes);
    Word firing = 0;
    for (Word a = 0; a < 4; ++a) {
        Word lo = brain.contact[contact(cue, a)], hi = lo >> 1;
        Word sum = spike[0] * __builtin_popcount(mask & ~lo & ~hi) + spike[1] * __builtin_popcount(mask & lo & ~hi) +
                   spike[2] * __builtin_popcount(mask & ~lo & hi) + spike[3] * __builtin_popcount(mask & lo & hi);
        firing |= sum << (8 * a);
    }
    return firing;
}
AX_FN Word winners(Word firing, Word boost, Word basal) {
    Word peak = 0, mask = 0;
    for (Word a = 0; a < 4; ++a) {
        Word spikes = (firing >> (8 * a)) & 255;
        if (spikes > peak) {
            peak = spikes;
            mask = 0;
        }
        if (spikes == peak)
            mask |= 1u << a;
    }
    Word preferred = 1u << (basal & 3);
    return boost && (mask & preferred) ? preferred : mask;
}
AX_FN Word from_tie(Word mask, Word start) { return (start + Word(__builtin_ctz((mask | (mask << 4)) >> start))) & 3; }
AX_FN bool explores(const Genes &g, Word key) { return ((key >> 8) & 255) < ((g.readout >> 18) & 31); }
AX_FN Word choose(const Circuit &b, const Genes &g, Word light, Word somatic, Word key) {
    Word start = mix(key) & 3;
    if (explores(g, key))
        return start;
    Word firing = volleys(b, g, receptor(light, g));
    return from_tie(winners(firing, (g.readout >> 16) & 3, somatic), start);
}
AX_FN void remember(Circuit &b, Word cue, Word muscle, bool grasped) {
    Word event = contact(cue, muscle) | 32 * Word(grasped) | 64;
    b.history = (b.history << 8) | event;
}
AX_FN Word associate(Circuit &b, const Genes &g, bool nourished, Word treatment, Word salt = 0) {
    if (!treatment)
        return 0;
    Word changes = 0;
    for (Word t = 0; t < 8; ++t) {
        if (!(g.timing & (1u << t)))
            continue;
        Word step = treatment == 2 ? (t + 1 + (mix(salt) % 7)) & 7 : t;
        Word event = Word(b.history >> (8 * step)) & 255;
        if (!(event & 64) || (!(event & 32) && !(g.sense & (1u << 16))))
            continue;
        Word address = event & 31, shift = 2 * t, old = (b.contact[address] >> shift) & 3;
        Word spike = discharge(old, g) >= 1 + ((g.readout >> 26) & 3);
        Word feature = Word(nourished) * 2 + spike;
        int delta = int((g.timing >> (8 + 4 * feature)) & 15);
        delta = delta < 8 ? delta : delta - 16;
        auto &integrator = b.support[group(address >> 2, g)][t];
        int next = int(integrator) + delta;
        integrator = static_cast<signed char>(next > 63 ? 63 : next < -63 ? -63 : next);
        Word code = (g.plastic >> (8 * Word(nourished) + 2 * old)) & 3;
        b.contact[address] = static_cast<Half>((b.contact[address] & ~(3u << shift)) | (code << shift));
        changes += code != old;
        if (g.sense & (1u << 18))
            for (Word k = 1; k < 4; ++k) {
                Word other = address ^ k, v = (b.contact[other] >> shift) & 3;
                Word release = (g.plastic >> (16 + 8 * Word(nourished) + 2 * v)) & 3;
                b.contact[other] = static_cast<Half>((b.contact[other] & ~(3u << shift)) | (release << shift));
                changes += release != v;
            }
    }
    return changes;
}
AX_FN void omission(Circuit &b, const Genes &g) {
    // A gut contraction is a sensory event in its own right. The gene can
    // change each delayed bouton when an earlier grip has no following pulse.
    int delta = int(g.timing >> 28);
    if (delta >= 8)
        delta -= 16;
    if (!delta)
        return;
    for (Word t = 0; t < 8; ++t) {
        Word event = Word(b.history >> (8 * t)) & 255;
        if ((event & 96) != 96 || !(g.timing & (1u << t)))
            continue;
        auto &v = b.support[group((event & 31) >> 2, g)][t];
        int next = int(v) + delta;
        v = static_cast<signed char>(next < -63 ? -63 : next > 63 ? 63 : next);
    }
}
AX_FN Word light(Word spore, Word winding, Word background) {
    // Two surface pigments and a visible crease. The other light channels
    // come from the actual water, vegetation and neighbouring material.
    return (spore & 3) | ((winding & 1) << 2) | (background & 248);
}
AX_FN Word key_for(Word seed, Word district, Word tick, const Physics &p) {
    Word law = mix(seed ^ mix(district + 8701) ^ mix(tick / p.period + 971));
    if (!(p.background & 8))
        return law;
    Word base = mix(seed ^ mix(district + 8701)), turn = (law >> 19) & 3, table = 0;
    for (Word i = 0; i < 8; ++i)
        table |= ((((base >> (2 * i)) & 3) + turn) & 3) << (2 * i);
    return table | (law & 0xffff0000u);
}
AX_FN Word aperture(Word colour, Word key) { return (key >> (2 * (colour & 7))) & 3; }
AX_FN Word latency(Word colour, Word district_key, const Physics &p) {
    if (!p.variable)
        return p.lag;
    return 1 + mix(district_key ^ ((colour & 1) * 0xbc52f391u) ^ 0xd591f21u) % p.lag;
}
AX_FN bool occupied(const Gut &gut) {
    for (Word k = 0; k < 8; ++k)
        if (gut.food[0][k] || gut.food[1][k] || gut.food[2][k] || gut.chaff[k])
            return true;
    return false;
}
AX_FN bool strike(const Circuit &b, const Genes &g, Word cue, Word muscle, bool pending) {
    Word address = contact(cue, muscle);
    // Interoception, a potentiated afferent and recent motor discharge are
    // independent inputs to one mutable motor gate.
    Word key = Word(pending) + 2 * Word(b.contact[address] != 0) + 4 * Word(bool(b.history & 32));
    return ((g.sense >> (24 + key)) & 1) != 0;
}
AX_FN void swallow(Gut &gut, Word tick, Word duration, Word food, Word mass, Word useful) {
    Word k = (tick + duration) & 7;
    gut.food[food][k] += static_cast<Byte>(useful);
    gut.chaff[k] += static_cast<Byte>(mass - useful);
}
struct Delivery {
    Word edible = 0, waste = 0, food[3]{};
};
AX_FN Delivery mature(Gut &gut, Word tick) {
    Delivery out;
    Word k = tick & 7;
    out.waste = gut.chaff[k];
    gut.chaff[k] = 0;
    for (Word j = 0; j < 3; ++j) {
        out.food[j] = gut.food[j][k];
        out.edible += out.food[j];
        gut.food[j][k] = 0;
    }
    return out;
}
AX_FN Word content(const Gut &gut) {
    Word n = 0;
    for (Word k = 0; k < 8; ++k)
        n += gut.chaff[k] + gut.food[0][k] + gut.food[1][k] + gut.food[2][k];
    return n;
}
} // namespace axion::life::pods

#include "flood_pod_frames.cuh"
