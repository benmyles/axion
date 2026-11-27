#pragma once
#include "axion/effort.hpp"
#include "grounding_eye.cuh"
namespace axion::life::effort {
AX_FN Word channel(Word stimulus, Word law) { return (stimulus ^ ((law >> 24) & 3)) & 3; }
AX_FN Word pulse(Word quantity, Word law) {
    Word level = quantity >> ((law >> 20) & 1);
    if (level > 8)
        level = 8;
    return (1u << level) - 1;
}
AX_FN void contact(grounding::Synapse &bank, Word law, Word identity, Word amount) {
    Word at = 8 * channel(identity, law), gates = 255u << at, signal = pulse(amount, law) << at;
    auto result = grounding::plastic(bank, signal, law & 65535);
    bank.low = (bank.low & ~gates) | (result.low & gates);
    bank.high = (bank.high & ~gates) | (result.high & gates);
}
AX_FN void taste(Mind &m, Word cue, Word consumed, Word mode) {
    if ((mode & (learning | blinded_food)) != learning)
        return;
    contact(m.food, m.genes.appetite, cue, consumed);
    m.experience |= 1u << cue;
}
AX_FN void strain(Mind &m, Word surface, Word spent, Word mode) {
    if ((mode & (learning | blinded_work)) != learning)
        return;
    contact(m.work, m.genes.muscle, surface, spent);
    m.experience |= 16u << surface;
}
AX_FN Word intensity(grounding::Synapse bank, Word law, Word cue) {
    Word fired = grounding::release(bank, (law >> 16) & 15);
    return __builtin_popcount((fired >> (8 * channel(cue, law))) & 255u) << ((law >> 20) & 1);
}
AX_FN int weight(Word gene, Word shift) { return int((gene >> shift) & 15) - 8; }
AX_FN Word context(Prospect p, Word law) {
    Word channels[4]{p.fruit, p.surface, Word(p.steps > 0) + Word(p.steps > 2) + Word(p.steps > 5),
                     p.fruit ^ p.surface};
    return (channels[(law >> 26) & 3] & 3) + 4 * (channels[(law >> 28) & 3] & 3) + 16 * Word(p.steps != 0);
}
AX_FN Word history(const Mind &m) { return grounding::release(m.outcomes, (m.genes.episode >> 16) & 15); }
AX_FN int integrate(Word g, Word food, Word work, Word habit, Word range) {
    return weight(g, 0) * int(food) + int(range) * (weight(g, 4) * int(work) + weight(g, 8)) +
           weight(g, 12) * int(habit) + weight(g, 16) * int(range != 0);
}
AX_FN Estimate expect(const Mind &m, Prospect p) {
    Word food = intensity(m.food, m.genes.appetite, p.fruit), work = intensity(m.work, m.genes.muscle, p.surface);
    Word habit = (history(m) >> context(p, m.genes.episode)) & 1;
    return {integrate(m.genes.projection, food, work, habit, p.steps), food, work, habit};
}
AX_FN bool elect(int a, int b, Word key) { return b > a || (a == b && (key & 1)); }
AX_FN void imprint(Mind &m, Word addr, bool consequence) {
    Word mask = 1u << addr;
    auto n = grounding::plastic(m.outcomes, consequence ? mask : 0u, m.genes.episode & 65535);
    m.outcomes.low = (m.outcomes.low & ~mask) | (n.low & mask);
    m.outcomes.high = (m.outcomes.high & ~mask) | (n.high & mask);
}
AX_FN void begin(Mind &m, Prospect p, Word reserve, bool chosen, bool predicted, bool random, Word duration) {
    m.experience = (m.experience & ~0xf00u) | ((duration >> 4) << 8);
    m.trace = context(p, m.genes.episode) | ((duration & 15) << 5) | (Word(chosen) << 9) | (Word(predicted) << 10) |
              (Word(random) << 11);
    m.account = reserve;
}
AX_FN void recognise_traffic(Mind &m, Word skin, Word mode) {
    if (!(mode & learning))
        return;
    Word rule = m.genes.episode >> 16, activity = 0;
    for (Word j = 0; j < 4; ++j) {
        Word old = (m.experience >> (12 + 2 * j)) & 3;
        Word next = (rule >> (2 * (old + 4 * Word(j == skin)))) & 3;
        activity |= next << (12 + 2 * j);
    }
    m.experience = (m.experience & ~0xff000u) | activity;
}
AX_FN void brood(Mind &m, Word shared) {
    if (!remaining(m))
        return;
    Word sum = (m.account >> 16) + shared;
    m.account = (m.account & 65535) | ((sum > 65535 ? 65535 : sum) << 16);
}
AX_FN int balance(const Mind &m, Word reserve) { return int(reserve) + int(m.account >> 16) - int(m.account & 65535); }
AX_FN bool advance(Mind &m, Word reserve, Word mode) {
    Word left = remaining(m);
    if (!left)
        return false;
    if (reserve && left > 1) {
        --left;
        m.trace = (m.trace & ~(15u << 5)) | ((left & 15) << 5);
        m.experience = (m.experience & ~(7u << 8)) | ((left >> 4) << 8);
        return false;
    }
    if (mode & learning) {
        int threshold = int((m.genes.episode >> 24) & 7) - 3;
        imprint(m, m.trace & 31, balance(m, reserve) > threshold);
    }
    return true;
}
} // namespace axion::life::effort
