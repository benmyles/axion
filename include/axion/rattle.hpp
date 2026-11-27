#pragma once
#include "life.hpp"

namespace axion::life::rattle {
constexpr Word silence = 8, version = 1;
enum Mode : Word { moving = 1, hearing = 2, learning = 4, voices = 8, jitter = 16, live = 15 };
struct Genes {
    Word drum = 0, ear = 0, synapse = 0, latch = 0;
};
struct Mind {
    Genes genes{};
    Word contacts = 0, trace = 0;
};
struct Saved {
    Mind mind{};
    Word tag = 0x4b4e4142u, extent = sizeof(Saved);
};
struct Totals {
    Count calls = 0, resonances = 0, pulls = 0, voiced = 0, mixed = 0, conducted = 0, food = 0, return_food = 0,
          cost = 0, settled = 0, edits = 0, switches = 0, sound_changes = 0, sound_gain = 0, sound_loss = 0,
          learned_changes = 0, learned_gain = 0, learned_loss = 0, resident_changes = 0, resident_gain = 0,
          resident_loss = 0, built_changes = 0, built_gain = 0, built_loss = 0;
};
struct State {
    Word format = version, mode = live, seed = 78283, phase = 0;
    Totals total{};
};
struct Attention;
struct Load;
struct Field {
    Mind *minds{};
    Totals *books{};
    Word mode = live, phase = 0;
    Attention *attention{};
    Load *load{};
};
static_assert(sizeof(Mind) == 24 && sizeof(Saved) == 32 && sizeof(Totals) == 24 * sizeof(Count));
AX_FN Genes founder(Word k) { return {mix(k + 1013), mix(k + 2341), mix(k + 4801), mix(k + 7369)}; }
AX_FN Genes inherit(Genes g, Word k, Word rate) {
    auto flip = [&](Word a, Word salt) {
        Word v = mix(k + salt);
        return (v & 255) < rate ? a ^ (1u << ((v >> 8) & 31)) : a;
    };
    return {flip(g.drum, 1201), flip(g.ear, 3631), flip(g.synapse, 5521), flip(g.latch, 7121)};
}
// The exciter has three binary afferents. Only the emitted impulse crosses a bank.
AX_FN Word knock(Genes g, bool laden, bool wet, Word motor) {
    Word row = Word(laden) | (Word(wet) << 1) | ((motor & 1) << 2);
    Word pulse = (g.drum >> (2 * row)) & 3;
    return pulse == 0 || pulse == 3 ? silence : (((g.drum >> 16) & 3) << 1) | (pulse - 1);
}
AX_FN Word conduct(Word wave, Word inverted) { return wave == silence ? silence : wave ^ (inverted & 1); }
AX_FN Word address(Genes g, Word wave) {
    Word mask = ((g.ear >> 16) & 1) | ((g.ear & (1u << 17)) ? 6u : 0u);
    return wave & mask;
}
AX_FN Word bank(const Mind &m) { return (m.genes.ear & (1u << 19)) ? m.trace & 1 : 0; }
AX_FN Word signal(const Mind &m, Word wave, bool intact) {
    if (wave == silence)
        return 2 * ((m.genes.latch >> 16) & 3);
    Word a = address(m.genes, wave), state = 0;
    if (intact && (m.genes.ear & (1u << 18)))
        state = (m.contacts >> (2 * (a + 8 * bank(m)))) & 3;
    return 2 * ((m.genes.ear >> (2 * state)) & 3) + ((m.genes.ear >> (8 + a)) & 1);
}
AX_FN Word pick(const Mind &m, Word left, Word right, Word drive, bool intact = true) {
    Word a = signal(m, left, intact), b = signal(m, right, intact);
    if (a == b || ((drive >> 8) & 31) < ((m.genes.ear >> 20) & 7))
        return drive & 1;
    return b > a;
}
// A bolus has already been swallowed. The trace carries its subsequent gut spike,
// the original synapse address, and the motor prediction. It carries no carbon.
AX_FN bool waiting(const Mind &m) { return m.trace & 2; }
AX_FN Word latency(Genes g) { return 2 + ((g.latch >> 20) & 7); }
AX_FN void swallow(Mind &m, Word wave, Word amount, Word tick) {
    if (wave == silence)
        return;
    Word a = address(m.genes, wave) + 8 * bank(m);
    Word tasted = amount > ((m.genes.latch >> 18) & 3);
    Word prediction = signal(m, wave, true) >= 4;
    m.trace =
        (m.trace & 1) | 2 | (a << 2) | (tasted << 6) | (prediction << 7) | ((wave & 1) << 8) | ((tick & 65535) << 9);
}
AX_FN Word digest(Mind &m, Word tick, bool plastic) {
    if (!waiting(m) || ((tick - (m.trace >> 9)) & 65535) < latency(m.genes))
        return 0;
    Word trace = m.trace, context = trace & 1;
    m.trace = context;
    if (!plastic)
        return 1;
    Word a = (trace >> 2) & 15, shift = 2 * a, old = (m.contacts >> shift) & 3;
    Word sweet = (trace >> 6) & 1, predicted = (trace >> 7) & 1, crest = (trace >> 8) & 1;
    if (trace & (1u << 25))
        sweet = (m.genes.drum >> (20 + sweet)) & 1;
    Word next = (m.genes.synapse >> (2 * (old + 4 * sweet + 8 * (a >> 3)))) & 3;
    m.contacts = (m.contacts & ~(3u << shift)) | (next << shift);
    bool edited = next != old;
    // Ears with one context bank can use four spare cells to learn from a
    // delayed coincidence between their own prediction and the taste pulse.
    if ((trace & (1u << 26)) && (m.genes.latch & (1u << 23)) && !(m.genes.ear & (1u << 19))) {
        Word index = 16 + 2 * ((a & 7) >> 1), previous = (m.contacts >> index) & 3;
        Word update = (m.genes.synapse >> (16 + 2 * (previous + 4 * Word(predicted == sweet)))) & 3;
        m.contacts = (m.contacts & ~(3u << index)) | (update << index);
        edited |= update != previous;
    }
    Word gate = context + 2 * predicted + 4 * sweet + 8 * crest;
    m.trace = (m.genes.ear & (1u << 19)) ? (m.genes.latch >> gate) & 1 : 0;
    return 1 | (Word(edited) << 1) | (Word(m.trace != context) << 2);
}
AX_FN bool valid(const Mind &m) {
    return !(m.trace >> 27) && (waiting(m) || m.trace <= 1) && (!(m.trace & 1) || (m.genes.ear & (1u << 19))) &&
           (!waiting(m) || ((m.trace >> 5) & 1) == bank(m));
}
AX_FN Word contacts(const Mind &m) { return __builtin_popcount((m.contacts | (m.contacts >> 1)) & 0x55555555u); }
AX_FN Word capacity(Word strand, Word limbs) {
    Word leverage = strand < limbs ? strand : limbs;
    return 2 + 2 * leverage;
}
AX_FN bool sound_motor(Word program, Word motor) { return (program >> (24 + (motor & 7))) & 1; }
} // namespace axion::life::rattle

namespace axion::life::rattle {
AX_FN Mind rebase(Mind m, Word from, Word to) {
    if (waiting(m)) {
        Word age = (from - (m.trace >> 9)) & 65535;
        if (age > latency(m.genes))
            age = latency(m.genes);
        m.trace = (m.trace & ~(65535u << 9)) | (((to - age) & 65535) << 9);
    }
    return m;
}
} // namespace axion::life::rattle

namespace axion::life::rattle {
AX_FN Word image(const Mind &m) {
    Word packed = ((m.genes.ear >> 20) & 7) << 27;
    for (Word wave = 0; wave <= silence; ++wave)
        packed |= signal(m, wave, true) << (3 * wave);
    return packed;
}
AX_FN Word dispatch(Word packed, Word l, Word r, Word drive) {
    Word a = (packed >> (3 * l)) & 7, b = (packed >> (3 * r)) & 7;
    return a == b || ((drive >> 8) & 31) < (packed >> 27) ? drive & 1 : Word(b > a);
}
} // namespace axion::life::rattle

namespace axion::life::rattle {
constexpr Word single = 32, crossed = 64, no_tag = 4;
AX_FN void taste(Mind &m, Word near_wave, Word far_wave, Word amount, Word tick, bool across) {
    bool distant = across && (m.genes.ear & (1u << 23)) && far_wave != silence &&
                   (near_wave == silence || (m.genes.drum & (1u << 23)));
    Word attention = distant ? far_wave : near_wave;
    if (attention == silence)
        return;
    swallow(m, attention, amount, tick);
    m.trace |= (Word(distant) << 25) | (Word(across) << 26);
}

struct Attention {
    Count solicited = 0, replies = 0, changed = 0, gain = 0, loss = 0;
};
AX_FN int contrast(const Mind &m, Word timbre) {
    if (timbre >= no_tag)
        return -1;
    int a = int(signal(m, 2 * timbre, true)), b = int(signal(m, 2 * timbre + 1, true));
    return a < b ? b - a : a - b;
}
AX_FN Word invite(const Mind &m, Word left, Word right, Word drive, bool memory = true, bool across = false) {
    auto preference = [&](Word tag) {
        if (tag == no_tag)
            return -8;
        Word regime = (m.genes.latch >> 24) & 3;
        if (regime < 2 && memory) {
            int gain = contrast(m, tag);
            if (across && (m.genes.latch & (1u << 23)) && !(m.genes.ear & (1u << 19))) {
                Word state = (m.contacts >> (16 + 2 * (address(m.genes, 2 * tag) >> 1))) & 3;
                gain += 3 * int((m.genes.ear >> (2 * state)) & 3);
            }
            return regime ? -gain : gain;
        }
        return regime == 3 ? int((m.genes.latch >> (28 + tag)) & 1) : 0;
    };
    int a = preference(left), b = preference(right);
    Word toss = mix(drive + 3457);
    if (a == b || ((toss >> 4) & 15) < ((m.genes.latch >> 26) & 3))
        return toss & 1;
    return b > a;
}
} // namespace axion::life::rattle

namespace axion::life::rattle {
constexpr Word loaded = 128;
struct Load {
    Count trapped = 0, opposed = 0, pinched = 0, added = 0, to_worker = 0, to_gripper = 0;
};
struct Catch {
    Word near, far, solo;
};
AX_FN Catch dislodge(Word stock, Word cover, Word force, Word brace, Word room, Word far_room) {
    Word solo = force > cover ? force - cover : 0, both = force + brace > cover ? force + brace - cover : 0;
    both = both < stock ? both : stock;
    Word far = brace ? both * brace / (force + brace) : 0;
    far = far < far_room ? far : far_room;
    Word near = both - far;
    near = near < room ? near : room;
    solo = solo < stock ? solo : stock;
    return {near, far, solo < room ? solo : room};
}
} // namespace axion::life::rattle
