#pragma once
#include "axion/grounding.hpp"

namespace axion::life::grounding {
struct Bank {
    Shape cube[hypotheses]{};
    constexpr Bank() {
        Word i = 0;
        for (Word depth = 1; depth <= 3; ++depth)
            for (Word mask = 1; mask < 256; ++mask)
                if (Word(__builtin_popcount(mask)) == depth) {
                    for (Word value = 0; value < 256; ++value)
                        if (!(value & ~mask)) {
                            cube[i] = {static_cast<Byte>(mask), static_cast<Byte>(value)};
                            ++i;
                        }
                }
    }
};
constexpr Bank physical_bank{};
AX_FN Word lamp(const Retina &r, Count site) {
    Word code = 0;
    for (Word k = 0; k < channels; ++k)
        if (r.band[k] & site)
            code |= 1u << k;
    return code;
}
AX_FN void illuminate(Retina &r, Word site, Word light) {
    Count bit = 1ull << site;
    r.sites |= bit;
    for (Word k = 0; k < channels; ++k)
        if ((light >> k) & 1)
            r.band[k] |= bit;
}
AX_FN Word dist(Word a, Word b) {
    int x = int(a & 7) - int(b & 7), y = int(a >> 3) - int(b >> 3);
    return Word(__builtin_abs(x) + __builtin_abs(y));
}
AX_FN Scene see(const View &v, RecollectionGenes g) {
    Scene s;
    Count found = v.after.sites;
    Word tolerance = (g.expression >> 12) & 3;
    while (found && s.n < objects) {
        Word site = __builtin_ctzll(found);
        found &= found - 1;
        Word light = lamp(v.after, 1ull << site);
        Word best = ~0u, prior = absent;
        for (Count old = v.before.sites; old; old &= old - 1) {
            Word k = __builtin_ctzll(old), d = __builtin_popcount(lamp(v.before, 1ull << k) ^ light);
            if (d > tolerance)
                continue;
            Word rank = d * 32 + dist(site, k);
            if (rank < best) {
                best = rank;
                prior = k;
            } else if (rank == best)
                prior = absent;
        }
        s.patch[s.n++] = {static_cast<Byte>(light), static_cast<Byte>(prior), static_cast<Byte>(site)};
    }
    s.truncated = found != 0;
    return s;
}
struct Response {
    Word fired = 0, known = 0;
};
AX_FN void contrast(Response &r, Word at, int one, int other) {
    r.known |= 3u << at;
    if (one < other)
        r.fired |= 1u << at;
    if (one > other)
        r.fired |= 2u << at;
}
AX_FN Response relative(Patch a, Patch b) {
    Response r;
    int x = a.site & 7, y = a.site >> 3, u = b.site & 7, v = b.site >> 3;
    contrast(r, 0, x, u);
    contrast(r, 2, y, v);
    contrast(r, 4, __builtin_popcount(a.light), __builtin_popcount(b.light));
    contrast(r, 6, x + y, u + v);
    contrast(r, 8, x - y, u - v);
    if (a.prior == absent || b.prior == absent)
        return r;
    int old_x = a.prior & 7, old_y = a.prior >> 3, old_u = b.prior & 7, old_v = b.prior >> 3;
    contrast(r, 10, old_x, old_u);
    contrast(r, 12, old_y, old_v);
    contrast(r, 14, x - old_x, u - old_u);
    contrast(r, 16, y - old_y, v - old_v);
    contrast(r, 18, dist(a.site, b.site), dist(a.prior, b.prior));
    contrast(r, 20, dist(a.prior, a.site), dist(b.prior, b.site));
    r.known |= 3u << 22;
    if ((x - u) * (old_x - old_u) < 0)
        r.fired |= 1u << 22;
    if ((y - v) * (old_y - old_v) < 0)
        r.fired |= 1u << 23;
    return r;
}
AX_FN Synapse plastic(Synapse old, Word spikes, Word table) {
    Synapse n;
    for (Word code = 0; code < 4; ++code) {
        Word cells = (code & 1 ? old.low : ~old.low) & (code & 2 ? old.high : ~old.high);
        Word on = (table >> (8 + 2 * code)) & 3, off = (table >> (2 * code)) & 3;
        if (on & 1)
            n.low |= cells & spikes;
        if (off & 1)
            n.low |= cells & ~spikes;
        if (on & 2)
            n.high |= cells & spikes;
        if (off & 2)
            n.high |= cells & ~spikes;
    }
    return n;
}
AX_FN Word release(Synapse w, Word table) {
    return ((table & 1) ? ~(w.low | w.high) : 0) | ((table & 2) ? w.low & ~w.high : 0) |
           ((table & 4) ? ~w.low & w.high : 0) | ((table & 8) ? w.low & w.high : 0);
}
AX_FN Word recognize(const Mind &m, Count pulse) {
    if (!pulse)
        return 0;
    Word chosen = 0, best = 65;
    for (Word i = 1; i <= m.count; ++i) {
        Word d = __builtin_popcountll(m.word[i].wave ^ pulse);
        if (!d)
            return i;
        if (d < best) {
            chosen = i;
            best = d;
        } else if (d == best)
            chosen = 0;
    }
    return best <= (m.genes.expression & 15) ? chosen : 0;
}
AX_FN Word learn_wave(Mind &m, Count pulse) {
    if (!pulse)
        return 0;
    Word cell = recognize(m, pulse);
    if (!cell) {
        if (m.count == forms)
            return 0;
        cell = ++m.count;
        m.word[cell].wave = pulse;
    }
    auto &w = m.word[cell];
    Count difference = w.wave ^ pulse;
    Count changed = ((m.genes.expression & 16) ? difference & ~w.stability : 0ull) |
                    ((m.genes.expression & 32) ? difference & w.stability : 0ull);
    w.wave ^= changed;
    w.stability = ~difference;
    return cell;
}
AX_FN Phrase listen(Mind &m, const Voice &p, bool learn) {
    Phrase s;
    if (p.n > utterance)
        return s;
    for (Word i = 0; i < p.n; ++i) {
        Word x = learn ? learn_wave(m, p.waves[i]) : recognize(m, p.waves[i]);
        if (!x)
            return {};
        s.id[s.n++] = static_cast<Byte>(x);
    }
    return s;
}
AX_FN Phrase hear(const Mind &m, const Voice &p) {
    Phrase s;
    for (Word k = 0; k < p.n && k < utterance; ++k) {
        Word x = recognize(m, p.waves[k]);
        if (!x)
            return {};
        s.id[s.n++] = static_cast<Byte>(x);
    }
    return s;
}
AX_FN bool admits(const Word *planes, Word color) { return (planes[color >> 5] >> (color & 31)) & 1; }
AX_FN const Episode &episode(const Mind &m, Word k) { return m.episodes[(k + m.oldest) % turns]; }
AX_FN const Example &exchange(const Mind &m, Word k) { return m.examples[(k + m.examples_first) % demonstrations]; }
AX_FN void keep(Mind &m, const Scene &s, Phrase p) {
    Word index = (m.oldest + m.kept) % turns;
    if (m.kept == turns)
        m.oldest = (m.oldest + 1) % turns;
    else
        ++m.kept;
    m.episodes[index] = {s, p};
    ++m.experienced;
}
AX_FN void notice(Mind &m, const Scene &s, Phrase p, Word answer, Phrase prior) {
    if (!p.n || !answer)
        return;
    Word index = (m.examples_first + m.examples_n) % demonstrations;
    if (m.examples_n == demonstrations)
        m.examples_first = (m.examples_first + 1) % demonstrations;
    else
        ++m.examples_n;
    m.examples[index] = {s, p, static_cast<Byte>(answer), prior};
    ++m.exchanges;
}
} // namespace axion::life::grounding
