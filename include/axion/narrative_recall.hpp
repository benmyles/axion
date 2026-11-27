#pragma once
#include "narrative.hpp"

namespace axion::life::utterance {
AX_FN Word difference(Count a, Count b) {
    Word mismatch = 0;
    for (Word j = 0; j < phases; ++j, a >>= 6, b >>= 6) {
        if (bool(a & 63) != bool(b & 63))
            return 255;
        if ((a & 63) != (b & 63))
            mismatch |= 1u << j;
    }
    return mismatch;
}
AX_FN Word cue_index(Memory &m, Count key, bool learn) {
    if (!key)
        return cues;
    Word free = cues, possible = cues;
    for (Word j = 0; j < cues; ++j) {
        auto &c = m.questions[j];
        if (!c.pattern) {
            if (free == cues)
                free = j;
            continue;
        }
        Word d = difference(c.pattern, key);
        if (!d || (c.variable && d == (1u << (c.variable - 1))))
            return j;
        if (!c.variable && learn && d && !(d & (d - 1)))
            possible = j;
    }
    if (learn && possible != cues) {
        m.questions[possible].variable = 1 + __builtin_ctz(difference(m.questions[possible].pattern, key));
        return possible;
    }
    if (learn && free != cues)
        m.questions[free].pattern = key;
    return learn ? free : cues;
}
struct Replay {
    Packet line[prompts][depth]{};
    Word n = 0;
};
AX_FN Replay activate(const Memory &m, Trail retained) {
    Replay r{};
    r.n = retained.size();
    for (Word j = 0; j < prompts; ++j)
        if (m.script[j].prompt)
            for (Word t = 0; t < r.n; ++t)
                r.line[j][t] = unit(m.script[j], retained.at(t));
    return r;
}
AX_FN Word marked(Packet sound, Word phase) {
    Word n = sound.size(), offset = phase & (phases - 1);
    return offset < n ? sound.at(phase & phases ? n - offset - 1 : offset) : 0;
}
AX_FN Word encounter_at(const Replay &r, Word reader, Word phase, Word path, Word noun) {
    for (Word j = 0; j < r.n; ++j) {
        Word k = path >= 3 ? r.n - j - 1 : j;
        if (marked(r.line[reader][k], phase) == noun) {
            Word destination = k + (path % 3) - 1u;
            return destination < r.n ? destination : depth;
        }
    }
    return depth;
}
AX_FN bool repeats(Packet p, const Phrase &heard) {
    if (!p.size() || p.size() != heard.n)
        return false;
    for (Word j = 0; j < p.size(); ++j)
        if (p.at(j) != heard.word[j])
            return false;
    return true;
}
AX_FN bool attend_query(Memory &m, const Phrase &q, const Phrase &answer, Trail retained, Biology body,
                        Growth &changes) {
    if (!retained.size() || !answer.n || !(body.arbor & (1u << 14)))
        return false;
    Word i = cue_index(m, address(q), true);
    if (i == cues)
        return false;
    auto &c = m.questions[i];
    if (!c.variable)
        return true;
    Word noun = q.word[c.variable - 1];
    auto r = activate(m, retained);
    ++c.examples;
    for (Word reader = 0; reader < prompts; ++reader)
        for (Word p = 0; p < paths; ++p) {
            Word at[2 * phases];
            for (Word j = 0; j < 2 * phases; ++j)
                at[j] = encounter_at(r, reader, j, p, noun);
            for (Word output = 0; output < prompts; ++output) {
                auto &row = c.excluded[output][reader][p];
                Word bad = 0;
                for (Word j = 0; j < 2 * phases; ++j)
                    if (at[j] == depth || !repeats(r.line[output][at[j]], answer))
                        bad |= 1u << j;
                Word proposal = braid::rule_bits(body.law, row, bad) & 65535;
                Word gate = conducting(body, changes, row, bad, proposal, row != 0, bad != 65535, 65535);
                Word next = regulation::apply(row, proposal, gate);
                changes.contacts += __builtin_popcount(row ^ next);
                row = next;
            }
        }
    return true;
}
struct Retrieval {
    Packet reply{};
    Word event = depth, routes = 0, recognized = 0;
};
AX_FN Retrieval recall(Memory &m, const Phrase &prompt, Trail history) {
    Word id = cue_index(m, address(prompt), false);
    if (id == cues || !m.questions[id].examples || !m.questions[id].variable)
        return {};
    const auto &c = m.questions[id];
    Word noun = prompt.word[c.variable - 1];
    auto r = activate(m, history);
    Retrieval a{};
    a.recognized = 1;
    bool first = true;
    for (Word reader = 0; reader < prompts; ++reader)
        for (Word path = 0; path < paths; ++path) {
            Word possible = 0;
            for (Word output = 0; output < prompts; ++output)
                possible |= ~Word(c.excluded[output][reader][path]) & 65535;
            while (possible) {
                Word j = __builtin_ctz(possible);
                possible &= possible - 1;
                Word t = encounter_at(r, reader, j, path, noun);
                for (Word output = 0; output < prompts; ++output)
                    if (!(c.excluded[output][reader][path] & (1u << j))) {
                        Packet answer = t == depth ? Packet{} : r.line[output][t];
                        if (!first && a.reply.bits != answer.bits) {
                            a.reply = {};
                            a.event = depth;
                            return a;
                        }
                        if (first) {
                            a.reply = answer;
                            a.event = t;
                        }
                        if (a.event != t)
                            a.event = depth;
                        ++a.routes;
                        first = false;
                    }
            }
        }
    return a;
}
AX_FN Phrase phrase(Packet p) {
    Phrase answer{};
    for (Word i = 0; i < p.size(); ++i)
        answer.push(p.at(i));
    return answer;
}
} // namespace axion::life::utterance
