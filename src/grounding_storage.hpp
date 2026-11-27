#pragma once
#include "axion/grounding.hpp"
#include <stdexcept>
namespace axion::life::grounding {
inline void validate(const Mind &m) {
    auto fail = []() { throw std::runtime_error("invalid grounded language circuit"); };
    if (m.version != 1 || m.extent != sizeof(Mind) || m.count > forms || m.kept > turns || m.oldest >= turns ||
        m.kept > m.experienced || m.consolidated > m.experienced || m.layout > 7 || m.examples_n > demonstrations ||
        m.examples_first >= demonstrations || m.constructions > questions || m.examples_n > m.exchanges)
        fail();
    auto scene = [&](const Scene &s) {
        if (s.n > objects || s.truncated > 1)
            fail();
        Count occupied = 0;
        for (Word j = 0; j < s.n; ++j) {
            auto p = s.patch[j];
            if (p.site >= 64 || (p.prior != absent && p.prior >= 64) || (occupied & (1ull << p.site)))
                fail();
            occupied |= 1ull << p.site;
        }
    };
    auto phrase = [&](Phrase p, bool mask = false) {
        if (p.n > utterance)
            fail();
        for (Word j = 0; j < p.n; ++j)
            if ((!mask && !p.id[j]) || p.id[j] > m.count)
                fail();
    };
    for (Word k = 1; k <= m.count; ++k) {
        auto &c = m.word[k];
        if (!c.wave || c.viable > hypotheses || c.order > 3)
            fail();
        for (Word j = 0; j < 8; ++j)
            if (c.certain[j] & ~c.possible[j])
                fail();
    }
    for (Word r = 0; r < 3; ++r) {
        if ((m.fills[r] >> (m.count + 1)) || (m.fills[r] & 1))
            fail();
        for (Word i = 0; i <= m.count; ++i)
            if (m.edge[r][i].possible >> relations)
                fail();
    }
    scene(m.here);
    phrase(m.discourse);
    for (Word k = 0; k < m.kept; ++k) {
        const auto &e = m.episodes[(m.oldest + k) % turns];
        scene(e.scene);
        phrase(e.heard);
    }
    for (Word k = 0; k < m.examples_n; ++k) {
        const auto &e = m.examples[(m.examples_first + k) % demonstrations];
        scene(e.scene);
        phrase(e.heard);
        phrase(e.prior);
        if (!e.answer || e.answer > m.count)
            fail();
    }
    for (Word k = 0; k < m.constructions; ++k) {
        auto &r = m.construction[k];
        phrase(r.motif, true);
        if (r.answer_slot > 3 || r.yes > m.count || r.no > m.count)
            fail();
        for (Word j = 0; j < 3; ++j) {
            Word f = r.field[j];
            if (f > utterance + 3 || (f && f <= utterance && (f > r.motif.n || r.motif.id[f - 1])) ||
                (!f) != (j == r.answer_slot))
                fail();
        }
    }
}
} // namespace axion::life::grounding
