#pragma once
#include "practice_cells.cuh"
#include "axion/practice_tree.hpp"
namespace axion::life::practice {
AX_FN Charts recall(const Mind &m, Slots s, Word cap) {
    Charts c{};
    c.count = size(m) < cap ? size(m) : cap;
    for (Word j = 0; j < c.count; ++j)
        c.model[j] = read(s.at(j), m.genes);
    return c;
}
AX_FN Word simulate(const Map &model, Rig r, Word start, const linkage::Design &p) {
    if (!p.found)
        return absent;
    if (p.edit < linkage::no_edit && !linkage::model_splice(model, r, p.edit))
        return absent;
    Word v = start;
    for (Word i = 0; i < p.turns.len; ++i) {
        Word motor = linkage::step(p.turns, i);
        if (!(r.handles & (1u << motor)))
            return absent;
        v = linkage::predict(model, r, v, motor);
        if (v == absent)
            return absent;
    }
    return v;
}
AX_FN linkage::Design shared(const Charts &c, const Request &r, const Word *states, Word subset, Genes genes,
                             Word wallet) {
    linkage::Design best{};
    auto motors = motor_genes(genes);
    for (Word i = 0; i < c.count; ++i)
        if (subset & (1u << i)) {
            auto p = linkage::design(c.model[i], r.rig, states[i], r.goal, r.sockets, motors);
            if (!p.found || p.effort > wallet || (best.found && p.effort >= best.effort))
                continue;
            bool works = true;
            for (Word j = 0; j < c.count; ++j)
                if ((subset & (1u << j)) && j != i) {
                    Word result = simulate(c.model[j], r.rig, states[j], p);
                    if (result == absent || !linkage::matches(result, r.goal)) {
                        works = false;
                        break;
                    }
                }
            if (works)
                best = p;
        }
    return best;
}
AX_FN Script reason(const Mind &m, const Charts &charts, const Request &r, Word allowed, Word key, Word mode,
                    const Word *initial = nullptr) {
    Script best{};
    Word n = charts.count;
    if (!n)
        return best;
    Word starts[]{r.view, r.view, r.view, r.view}, all = (1u << n) - 1;
    if (initial)
        for (Word i = 0; i < n; ++i)
            starts[i] = initial[i];
    Word slots = r.work;
    auto fixed = shared(charts, r, starts, all, m.genes, slots);
    if (fixed.found) {
        best.reply[0] = fixed;
        best.found = best.outcomes = 1;
        best.worst = fixed.effort;
        best.sum = n * fixed.effort;
    }
    if (n < 2 || !slots || (fixed.found && fixed.effort <= 1) || !(mode & probing) ||
        !(m.genes.inference & (1u << 6)))
        return best;
    Word permitted = r.rig.handles & allowed;
    if ((mode & random_probes) && permitted)
        permitted = 1u << linkage::bump(permitted, key);
    for (Word a = 0; a < 8; ++a) {
        Word move = linkage::motor_order(motor_genes(m.genes), a);
        if (!(permitted & (1u << move)))
            continue;
        Word after[4]{};
        bool known = true;
        for (Word j = 0; j < n; ++j) {
            after[j] = linkage::predict(charts.model[j], r.rig, starts[j], move);
            if (after[j] == absent) {
                known = false;
                break;
            }
        }
        if (!known)
            continue;
        // Share a downstream motor plan wherever the same observation leaves
        // the same set of surviving models. No physical calibration is available here.
        linkage::Design tails[16]{};
        Word evaluated = 0;
        for (Word place = 0; place < 4; ++place) {
            Word look = (place + ((m.genes.muscles >> 5) & 3)) & 3;
            if (!(r.rig.parts & (1u << look)))
                continue;
            Word groups[4]{}, mask = 0;
            for (Word j = 0; j < n; ++j) {
                Word face = bits(after[j], look);
                groups[face] |= 1u << j;
                mask |= 1u << face;
            }
            if (!(mask & (mask - 1)))
                continue;
            Script q{};
            q.ask = move;
            q.look = look;
            q.worst = 1;
            q.sum = n;
            q.found = 1;
            q.outcomes = mask;
            for (Word b = 0; b < 4; ++b)
                if (groups[b]) {
                    Word team = groups[b];
                    if (!(evaluated & (1u << team))) {
                        tails[team] = shared(charts, r, after, team, m.genes, slots - 1);
                        evaluated |= 1u << team;
                    }
                    auto &p = tails[team];
                    if (!p.found) {
                        q.found = 0;
                        break;
                    }
                    q.reply[b] = p;
                    Word w = 1 + p.effort;
                    if (w > q.worst)
                        q.worst = w;
                    q.sum += p.effort * __builtin_popcount(team);
                }
            if (!q.found)
                continue;
            bool minimax = m.genes.inference & (1u << 10);
            Word score = minimax ? q.worst * 64 + q.sum : q.sum * 32 + q.worst;
            Word prior = minimax ? best.worst * 64 + best.sum : best.sum * 32 + best.worst;
            if (!best.found || score < prior)
                best = q;
        }
    }
    return best;
}
} // namespace axion::life::practice
