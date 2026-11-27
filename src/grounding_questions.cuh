#pragma once
#include "grounding_neural.cuh"

namespace axion::life::grounding {
AX_FN bool content(const Mind &m, Word id) {
    if (!id || id > m.count)
        return false;
    if (m.word[id].viable)
        return true;
    for (Word r = 0; r < 3; ++r)
        if ((m.layout & (1u << r)) && m.edge[r][id].possible)
            return true;
    return false;
}
AX_FN Phrase outline(const Mind &m, Phrase q) {
    for (Word j = 0; j < q.n; ++j)
        if (content(m, q.id[j]))
            q.id[j] = 0;
    return q;
}
AX_FN bool same_frame(Phrase a, Phrase b) {
    if (a.n != b.n)
        return false;
    for (Word j = 0; j < a.n; ++j)
        if (a.id[j] != b.id[j])
            return false;
    return true;
}
AX_FN bool matches(const Mind &m, Phrase outline, Phrase phrase) {
    if (outline.n != phrase.n)
        return false;
    for (Word j = 0; j < phrase.n; ++j)
        if (outline.id[j] ? outline.id[j] != phrase.id[j] : !content(m, phrase.id[j]))
            return false;
    return true;
}
constexpr Byte previous = utterance + 1;
AX_FN Phrase instantiate(const Construction &rule, Phrase q, Phrase prior, Word answer) {
    Phrase p;
    p.n = 3;
    for (Word j = 0; j < 3; ++j) {
        Word k = rule.field[j];
        if (!k)
            p.id[j] = static_cast<Byte>(answer);
        else if (k < previous)
            p.id[j] = q.id[k - 1];
        else if (prior.n == 3)
            p.id[j] = prior.id[k - previous];
        else
            return {};
    }
    return p;
}
AX_FN Answer interpret(const Mind &m, const Scene &s, const Construction &rule, Phrase q, Phrase prior) {
    Answer a;
    if (rule.answer_slot == 3) {
        auto p = instantiate(rule, q, prior, 0);
        Word answer = p.n ? truth(m, s, p) : 0;
        if (answer) {
            Word id = answer == 2 ? rule.yes : rule.no;
            a.names = 1ull << id;
            a.wave = m.word[id].wave;
            a.statement = p;
            a.complete = 1;
        }
        return a;
    }
    Count eligible = 0, definite = 0, sites = 0;
    for (Word id = 1; id <= m.count; ++id) {
        if ((m.genes.high & (1u << 18)) && !(m.fills[rule.answer_slot] & (1ull << id)))
            continue;
        Phrase p = instantiate(rule, q, prior, id);
        if (!p.n)
            continue;
        for (Word r = 0; r < 3; ++r)
            if (m.layout & (1u << r)) {
                auto f = read_frame(m, s, p, r);
                if (!f.possible)
                    continue;
                eligible |= 1ull << id;
                if (f.certain) {
                    definite |= 1ull << id;
                    a.statement = p;
                }
                if (rule.answer_slot == r)
                    continue;
                for (Count pairs = f.possible; pairs; pairs &= pairs - 1) {
                    Word k = __builtin_ctzll(pairs), j = rule.answer_slot == left_slot(r) ? k / 8 : k % 8;
                    sites |= 1ull << s.patch[j].site;
                }
            }
    }
    a.sites = sites;
    a.names = eligible;
    a.definite = definite;
    if (eligible && eligible == definite && !(eligible & (eligible - 1))) {
        a.wave = m.word[__builtin_ctzll(eligible)].wave;
        a.complete = 1;
    }
    a.ambiguous = bool(eligible & (eligible - 1));
    return a;
}
AX_FN Answer respond(const Mind &m, const Scene &s, Phrase q, Phrase prior) {
    Answer answer;
    bool agreement = true;
    for (Word k = 0; k < m.constructions; ++k) {
        auto &rule = m.construction[k];
        if (!matches(m, rule.motif, q))
            continue;
        auto v = interpret(m, s, rule, q, prior);
        if (!answer.hypotheses) {
            answer = v;
            answer.hypotheses = 1;
        } else {
            agreement &= answer.wave == v.wave && v.complete;
            answer.names |= v.names;
            answer.definite &= v.definite;
            answer.sites |= v.sites;
            ++answer.hypotheses;
        }
    }
    if (!agreement) {
        answer.complete = 0;
        answer.wave = 0;
        answer.ambiguous = 1;
    }
    return answer;
}
// The possible referents come from learned word fields and question frames.
// A reply may choose among genuine alternatives that the pixels alone leave open.
AX_FN Answer clarify(const Mind &m, const Scene &scene, Phrase heard, Phrase prior, Clarification &pending) {
    if (pending.remaining && heard.n == 1 && (heard.id[0] == pending.yes || heard.id[0] == pending.no)) {
        Count proposal = 1ull << pending.current;
        pending.remaining = heard.id[0] == pending.yes ? proposal : pending.remaining & ~proposal;
        Answer a;
        a.hypotheses = 1;
        if (pending.remaining) {
            Word id = __builtin_ctzll(pending.remaining);
            if (pending.remaining & (pending.remaining - 1)) {
                pending.current = id;
                a.asking = 1;
                a.wave = m.word[id].wave;
                return a;
            }
            auto fact = instantiate(m.construction[pending.rule], pending.prompt, pending.prior, id);
            if (truth(m, scene, fact) == 2) {
                a.complete = 1;
                a.statement = fact;
                a.wave = m.word[id].wave;
            }
        }
        pending = {};
        return a;
    }
    pending = {};
    auto a = respond(m, scene, heard, prior);
    if (a.hypotheses != 1 || !a.ambiguous || a.names != a.definite || !a.names)
        return a;
    Word frame = 0, yes = 0, no = 0;
    for (Word k = 0; k < m.constructions; ++k) {
        auto &r = m.construction[k];
        if (r.answer_slot != 3 && matches(m, r.motif, heard))
            frame = k;
        if (r.answer_slot == 3 && r.yes && r.no) {
            if (yes && (r.yes != yes || r.no != no))
                return a;
            yes = r.yes;
            no = r.no;
        }
    }
    if (yes && no) {
        pending.prompt = heard;
        pending.prior = prior;
        pending.remaining = a.names;
        pending.rule = frame;
        pending.current = __builtin_ctzll(a.names);
        pending.yes = yes;
        pending.no = no;
        a.asking = 1;
        a.wave = m.word[pending.current].wave;
    }
    return a;
}
AX_FN bool draft(Phrase motif, Word code, Construction &v) {
    Byte inputs[7]{};
    Word choices = 1, current = 0;
    for (Word j = 0; j < motif.n; ++j)
        if (!motif.id[j]) {
            if (choices == 4)
                return false;
            inputs[choices++] = static_cast<Byte>(j + 1);
            ++current;
        }
    for (Word j = 0; j < 3; ++j)
        inputs[choices++] = static_cast<Byte>(previous + j);
    if (code >= choices * choices * choices)
        return false;
    v = {};
    v.motif = motif;
    v.answer_slot = 3;
    Word used = 0;
    for (Word j = 0; j < 3; ++j) {
        Word k = code % choices;
        code /= choices;
        if (used & (1u << k))
            return false;
        used |= 1u << k;
        v.field[j] = inputs[k];
        if (!k)
            v.answer_slot = static_cast<Byte>(j);
    }
    Word mask = ((1u << current) - 1) << 1;
    return current && (mask & used) == mask;
}
AX_FN Word assess(const Mind &m, Construction &c) {
    Word evidence = 0, vary = 0, targets = 0;
    Phrase first{};
    for (Word k = 0; k < m.examples_n; ++k) {
        auto &e = exchange(m, k);
        if (!matches(m, c.motif, e.heard))
            continue;
        if (!first.n)
            first = e.heard;
        else
            for (Word j = 0; j < e.heard.n; ++j)
                if (e.heard.id[j] != first.id[j])
                    vary |= 1u << j;
        if (c.answer_slot != 3 && !content(m, e.answer))
            return 0;
        if (c.answer_slot == 3 && content(m, e.answer))
            return 0;
        auto z = instantiate(c, e.heard, e.prior, e.answer);
        if (!z.n)
            return 0;
        auto v = truth(m, e.scene, z);
        if (!v)
            return 0;
        if (c.answer_slot != 3) {
            if (v != 2)
                return 0;
        } else {
            auto &sound = v == 2 ? c.yes : c.no;
            if (sound && sound != e.answer)
                return 0;
            sound = e.answer;
            targets |= 1u << v;
        }
        ++evidence;
    }
    Word slots = 0;
    for (Word j = 0; j < c.motif.n; ++j)
        if (!c.motif.id[j])
            slots |= 1u << j;
    if (c.answer_slot == 3 && (targets != 6 || c.yes == c.no))
        return 0;
    if ((m.genes.high & (1u << 16)) && (vary & slots) != slots)
        return 0;
    return evidence >= 2 + ((m.genes.expression >> 6) & 1) ? evidence : 0;
}
#if defined(__CUDA__)
AX_FN void fit_questions(Mind &m) {
    Word t = lane();
    if (!t) {
        m.constructions = 0;
        m.crowded_templates = 0;
    }
    sync();
    for (Word k = 0; k < m.examples_n; ++k) {
        auto shape = outline(m, exchange(m, k).heard);
        bool already = false;
        for (Word j = 0; j < k && !already; ++j)
            already = same_frame(outline(m, exchange(m, j).heard), shape);
        if (already)
            continue;
        Word base = m.constructions;
        bool over = false;
        for (Word block = 0; block < 343; block += 32) {
            Construction r;
            Word qualified = 0;
            if (draft(shape, block + t, r))
                qualified = assess(m, r);
            Word winners = ballot(qualified), offset = __builtin_popcount(winners & ((1u << t) - 1));
            if (base + Word(__builtin_popcount(winners)) > questions) {
                over = true;
                break;
            }
            if (qualified) {
                r.evidence = static_cast<Byte>(qualified);
                m.construction[base + offset] = r;
            }
            base += __builtin_popcount(winners);
        }
        if (!t) {
            if (over)
                ++m.crowded_templates;
            else
                m.constructions = base;
        }
        sync();
    }
}
#endif
} // namespace axion::life::grounding

namespace axion::life::tabletop {
AX_FN Voice ask(const Convention &c, Fact f, Word type) {
    Voice q;
    auto term = utter(c, f);
    auto &v = c.voice;
    if (!type) {
        q.n = 5;
        q.waves[0] = v.gap[0];
        q.waves[1] = v.gap[3];
        q.waves[2] = v.verb[f.verb];
        q.waves[3] = v.particle[3];
        q.waves[4] = v.noun[f.right];
    } else if (type == 1) {
        q.n = 5;
        q.waves[0] = v.gap[0];
        q.waves[1] = v.gap[3];
        q.waves[2] = v.noun[f.left];
        q.waves[3] = v.verb[f.verb];
        q.waves[4] = v.particle[3];
    } else if (type == 2) {
        q.n = 4;
        q.waves[0] = v.gap[2];
        q.waves[1] = v.noun[f.left];
        q.waves[2] = v.particle[2];
        q.waves[3] = v.noun[f.right];
    } else if (type == 3) {
        q.n = 4;
        q.waves[0] = v.gap[3];
        for (Word j = 0; j < 3; ++j)
            q.waves[j + 1] = term.waves[j];
    } else {
        q.n = 2;
        q.waves[0] = v.gap[4];
        q.waves[1] = v.noun[f.right];
    }
    return q;
}
AX_FN Word answers(const Convention &c, const Scene &s, Fact f, Word type) {
    if (type == 3)
        return 1u << oracle(c, s, f);
    Word mask = 0, n = type == 2 ? 6 : 8;
    for (Word k = 0; k < n; ++k) {
        if (!type || type == 4)
            f.left = static_cast<Byte>(k);
        else if (type == 1)
            f.right = static_cast<Byte>(k);
        else
            f.verb = static_cast<Byte>(k);
        if (oracle(c, s, f))
            mask |= 1u << k;
    }
    return mask;
}
AX_FN Count answer_wave(const Convention &c, Word type, Word id) {
    return type == 3 ? c.voice.particle[id ? 0 : 1] : type == 2 ? c.voice.verb[id] : c.voice.noun[id];
}
} // namespace axion::life::tabletop
