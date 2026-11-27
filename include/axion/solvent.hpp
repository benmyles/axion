#pragma once
#include "material.hpp"

namespace axion::life::brine {
// One input volley for each four-bit combination of the two external senses.
// Bits in a word are parallel neurons. There are no chemical labels in a genome.
struct Genes {
    Word receptive[16]{}, active = 0, transition = 0, readout = 0;
};
struct Trace {
    Word low = 0, high = 0;
};
struct Animal {
    Genes genes{};
    Trace memory{};
};
struct Saved {
    Animal cortex{};
    Word signature = 0x42524e45u, extent = sizeof(Saved);
};
struct Salts {
    Word ions = 0;
};
struct State {
    Word species = 2, flowing = 1;
    Count stock[5]{};
};
struct Water {
    Salts *solutes = nullptr;
    Animal *animals = nullptr;
    Word species = 0, flowing = 0;
};
struct Larva {
    Genes genes{};
    Word family = 0, live = 0;
};
struct Harvest {
    Word initial = 0, supplied = 0, residual = 0, body = 0, heat = 0, eggs = 0, contacts = 0, meals = 0, requests = 0;
};
struct Reading {
    Word choices = 0, paid = 0, absorbed = 0, correct = 0, first = 0, second = 0, reversed = 0, witnesses = 0,
         ambiguous = 0, informative = 0, deducible = 0, solved = 0;
};
static_assert(sizeof(Genes) == 76 && sizeof(Trace) == 8 && sizeof(Salts) == 4);

AX_FN Word amount(Salts salt, Word species) { return (salt.ions >> (6 * species)) & 63; }
AX_FN Word phase(Salts salt, Word water) {
    Word threshold = 6 + ((water < 120 ? water : 120) >> 2), result = 0;
    for (Word j = 0; j < 5; ++j)
        result |= Word(amount(salt, j) > threshold) << j;
    return result;
}
// Each active solute reorients one surface group. All three shear operations
// are reversible. Changing concentrations can change which base matches a grain.
AX_FN constexpr Word surface(Word base, Word solution) {
    if (base >= 4)
        return base;
    Word x = base & 1, y = base >> 1;
    x ^= y & ((solution >> 2) & 1);
    y ^= x & ((solution >> 3) & 1);
    x ^= y & ((solution >> 4) & 1);
    return (x | (y << 1)) ^ (solution & 3);
}
AX_FN Salts dissolve(Word active, Word seed, Word species) {
    Salts salt;
    for (Word j = 0; j < species; ++j) {
        Word v = ((active >> j) & 1 ? 39u : 2u) + ((mix(seed + j * 343) >> 3) & 3);
        salt.ions |= v << (6 * j);
    }
    return salt;
}
// A developing branch can contact any two incident receptor channels. Two
// such branches can converge on an arbitrary binary dendritic gate.
AX_FN Word branch(Word inputs, Word wire) {
    Word a = (inputs >> (wire & 3)) & 1, b = (inputs >> ((wire >> 2) & 3)) & 1;
    return (wire >> (4 + 2 * a + b)) & 1;
}
AX_FN Word arbor(Word inputs, Word wire) {
    Word a = branch(inputs, wire), b = branch(inputs, wire >> 8);
    return (wire >> (16 + 2 * a + b)) & 1;
}
AX_FN Genes founder(Word seed, bool branches = true) {
    Genes dna{};
    // Developmental variation in tonic firing is independent of the habitat.
    Word sparse = mix(seed + 79), dense = mix(seed + 461) & ~sparse;
    for (Word q = 0; q < 16; ++q) {
        Word x = mix(seed + 37 * q), y = mix(seed + 37 * q + 9511);
        dna.receptive[q] = (x & ~sparse) | (x & y & sparse) | (y & dense);
    }
    if (branches) {
        for (Word neuron = 0; neuron < 32; ++neuron) {
            Word wire = mix(seed + neuron * 5171 + 1439);
            if ((wire >> 29) == 7)
                continue;
            for (Word q = 0; q < 16; ++q)
                dna.receptive[q] = (dna.receptive[q] & ~(1u << neuron)) | (arbor(q, wire) << neuron);
        }
    }
    dna.active = mix(seed + 1181);
    dna.transition = mix(seed + 8729);
    dna.readout = mix(seed + 6173);
    return dna;
}
AX_FN Genes descend(const Genes &a, const Genes &b, Word key, bool edit, bool cross, bool branches = true) {
    Genes c{};
    Word mask = cross ? mix(key + 121) : 0, changed = mix(key + 811);
    Word place = (changed >> 16) & 15, neuron = changed & 31, source = (changed >> 5) & 31,
         mutation = (changed >> 10) & 15;
    for (Word j = 0; j < 16; ++j) {
        Word v = (a.receptive[j] & ~mask) | (b.receptive[j] & mask);
        if (edit && mutation == 2)
            v = (v & ~(1u << neuron)) | (((v >> source) & 1u) << neuron);
        if (edit && (mutation >= 4 || mutation == 0) && j == place)
            v ^= 1u << neuron;
        if (edit && branches && mutation == 3 && (key & 7) <= ((a.readout >> 27) & 7))
            v = (v & ~(1u << neuron)) | (arbor(j, mix(key + 13011)) << neuron);
        c.receptive[j] = v;
    }
    c.active = (a.active & ~mask) | (b.active & mask);
    c.transition = (cross && (key & 1)) ? b.transition : a.transition;
    c.readout = (cross && (key & 2)) ? b.readout : a.readout;
    if (edit && (mutation == 1 || mutation == 2))
        c.active ^= 1u << neuron;
    if (edit && ((key >> 15) & 7) == 0)
        c.transition ^= 1u << ((key >> 19) & 31);
    if (edit && ((key >> 23) & 7) == 0)
        c.readout ^= 1u << ((key >> 3) & 31);
    return c;
}
AX_FN Word either(Word controls, Word no, Word yes) { return no ^ ((no ^ yes) & controls); }
AX_FN Word bitplane(Trace t, Word four) {
    Word z = 0u - (four & 1), o = 0u - ((four >> 1) & 1), two = 0u - ((four >> 2) & 1), three = 0u - ((four >> 3) & 1);
    return either(t.high, either(t.low, z, o), either(t.low, two, three));
}
AX_FN Word compress(Word pairs) { return (pairs & 1) | ((pairs >> 1) & 2) | ((pairs >> 2) & 4) | ((pairs >> 3) & 8); }
AX_FN Trace efficacy(const Genes &g, Trace t) {
    return {g.active & bitplane(t, compress(g.readout)), g.active & bitplane(t, compress(g.readout >> 1))};
}
AX_FN Word mass(Trace weights, Word fired = ~0u) {
    return __builtin_popcount(weights.low & fired) + 2u * __builtin_popcount(weights.high & fired);
}
AX_FN Word estimate(const Genes &g, Trace weights, Word cue) { return mass(weights, g.receptive[cue]); }
AX_FN Word arrival(const Genes &g) { return ((g.readout >> 10) & 3) | ((g.readout >> 24) & 4); }
AX_FN bool impulse(const Genes &g, Word votes, Word total) {
    return total && 8 * votes > (2 + ((g.readout >> 8) & 3)) * total;
}
AX_FN void observe(const Genes &g, Trace &t, Word cue, bool food) {
    Word signal = g.receptive[cue], law = g.transition >> (16 * Word(food));
    auto next = [&](Word bits) {
        return either(signal, bitplane(t, compress(bits)), bitplane(t, compress(bits >> 8)));
    };
    t = {next(law) & g.active, next(law >> 1) & g.active};
}
AX_FN Word cue(Word chip, Word soil) { return 4 * chip + soil; }

// A held duplicate spends one unit of manipulation and recovers unused
// compression. Excitation consumed by a successful reaction enters the fruit.
AX_FN Word press(material::Grain grain, Word raw, Word solution, Word &carbon, Word &energy) {
    if (energy < 3)
        return 0;
    grain.charge = static_cast<material::Byte>(1 + ((grain.substance >> 5) & 3));
    material::Half trace = carbon ? 1 : 0, product = 0;
    material::excite(grain, 4, surface(raw, solution), trace, product);
    --energy;
    if (!product)
        return 0;
    Word contact = 2 + __builtin_popcount(grain.faces) + ((grain.substance >> 1) & 3);
    Word liberated = carbon < contact ? carbon : contact;
    carbon -= liberated;
    energy += liberated;
    return liberated + 1;
}
AX_FN material::Grain grain(Word coating, Word key) {
    return {static_cast<material::Byte>(1 + ((key >> 8) % 255)),
            static_cast<material::Byte>(0x30 | (coating << 2) | (key & 3)), 0, 0};
}
} // namespace axion::life::brine
