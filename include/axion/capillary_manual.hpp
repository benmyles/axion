#pragma once
#include "capillary_tutor.hpp"

namespace axion::life::capillary {
struct LanguageRack {
    Count verb[2]{}, substance[4]{}, row[2]{}, column[3]{}, then = 0;
    utterance::Waves prompt{};
};
AX_FN bool tactile(Word v) {
    return ((v >> 2) & 3) < 2 && ((v >> 4) & 3) < 3 && (v >> 6) < 2;
}
AX_FN Word contact(const Work &w, Word pos, Word j) {
    return w.handle[j].material | (Word(loaded(w, j, pos)) << 2) | ((j % 3) << 4) | ((j / 3) << 6);
}
AX_FN utterance::Trail feel(const Work &w, Protocol moves, bool backwards = false) {
    utterance::Trail trail{};
    Word pos = 0;
    for (Word k = 0; k < moves.n; ++k) {
        Word j = moves.at(backwards ? moves.n - k - 1 : k);
        if (j >= w.n || w.handle[j].material >= 4)
            return {};
        trail.append(contact(w, pos, j));
        pos ^= 1u << j;
    }
    return trail;
}
AX_FN Receipt follow(const Work &w, utterance::Trail decoded) {
    Receipt r{0, w.stock};
    for (Word k = 0; k < decoded.size(); ++k) {
        Word v = decoded.at(k), peg = 3 * (v >> 6) + ((v >> 4) & 3);
        if (!tactile(v) || peg >= w.n || contact(w, r.position, peg) != v)
            ++r.denied;
        else
            stroke(w, r, peg);
    }
    return r;
}
AX_FN utterance::Waves label(const LanguageRack &rack, utterance::Trail touched) {
    utterance::Waves line{};
    for (Word k = 0; k < touched.size(); ++k) {
        Word v = touched.at(k);
        if (!tactile(v))
            return {};
        if (k)
            line.wave[line.n++] = rack.then;
        line.wave[line.n++] = rack.verb[(v >> 2) & 3];
        line.wave[line.n++] = rack.substance[v & 3];
        line.wave[line.n++] = rack.row[v >> 6];
        line.wave[line.n++] = rack.column[(v >> 4) & 3];
    }
    return line;
}
AX_FN Word cue(Manual &m, const utterance::Waves &q) {
    return utterance::address(utterance::register_waves(m.ear, q, false)) == m.script.prompt && m.script.prompt;
}
AX_FN bool practise(Manual &m, Body &body, utterance::Trail touch, const utterance::Waves &q,
                    const utterance::Waves &heard, Word gate, utterance::Growth &delta) {
    if (!body.limbs || !body.eyes || !touch.size() || !heard.n || !(body.growth.arbor & (1u << 14)))
        return false;
    auto key = utterance::address(utterance::register_waves(m.ear, q, true));
    if (!key || (m.script.prompt && m.script.prompt != key))
        return false;
    auto line = utterance::register_waves(m.ear, heard, true);
    if (!line.n)
        return false;
    m.script.prompt = key;
    auto biology = body.growth;
    biology.gated = gate;
    return utterance::rehearse(m.script, touch, line, biology, delta);
}
AX_FN utterance::Waves instruct(Manual &m, const Body &body, const utterance::Waves &prompt, utterance::Trail moves) {
    if (!body.mouth || !body.limbs || !cue(m, prompt))
        return {};
    return utterance::vocalize(m.ear, utterance::speak(m.script, moves));
}
AX_FN utterance::Trail understand(Manual &m, const utterance::Waves &prompt, const utterance::Waves &line) {
    if (!cue(m, prompt))
        return {};
    auto words = utterance::register_waves(m.ear, line, false);
    return utterance::listen(m.script, words,
                             [&](Word v) { return tactile(v) ? utterance::unit(m.script, v) : utterance::Packet{}; });
}
struct Instruction {
    Count intended = 0, decoded = 0, acoustic = 0;
    Word action = 0, planned = 0, steps = 0, words = 0, final = 0, denied = 0, earned = 0, before = 0, solvable = 0,
         needs_order = 0;
};
AX_FN Count fingerprint(const utterance::Waves &w) {
    Count value = 1469598103934665603ull;
    for (Word j = 0; j < w.n; ++j)
        value = (value ^ w.wave[j]) * 1099511628211ull;
    return value;
}
} // namespace axion::life::capillary
