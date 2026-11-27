#include "axion/siphon_words.hpp"
#include "grounding_eye.cuh"
namespace axion::life::siphon {
AX_FN Word identify(const Vocabulary &v, Count wave) {
    for (Word i = 0; i < v.count; ++i)
        if (v.forms[i].wave == wave)
            return i + 1;
    return 0;
}
AX_FN Word readings(const Form &word, const RecollectionGenes &g) {
    return grounding::release({word.faint, word.strong}, g.high & 15) & ((word.faint & 0x80000000u) ? 4095u : 255u);
}
AX_FN bool caption(Vocabulary &v, const Animal &local, const RecollectionGenes &g, const Room &room,
                   const Sentence &said, bool exchange = false) {
    if (!room.exposed || !said.n || said.n > phrase_length)
        return false;
    Caption e{};
    e.n = said.n;
    e.context = room.context < 4 ? room.context + 1 : 0;
    e.address = code(room.site.membrane, local.genes.anatomy);
    e.appearance = code(room.last.percept, local.genes.anatomy >> 3);
    if (exchange) {
        if (said.n < 2)
            return false;
        Word answer = identify(v, said.wave[said.n - 1]);
        if (!answer)
            return false;
        Word fires = readings(v.forms[answer - 1], g),
             allowed = (1u << e.address) | (16u << e.appearance) | (e.context ? (256u << (e.context - 1)) : 0u);
        if (!fires || (fires & (fires - 1)) || !(fires & allowed))
            return false;
    }
    Word missing = 0;
    for (Word i = 0; i < said.n; ++i) {
        if (!said.wave[i])
            return false;
        e.word[i] = identify(v, said.wave[i]);
        if (!e.word[i]) {
            bool new_word = true;
            for (Word j = 0; j < i; ++j)
                new_word &= said.wave[i] != said.wave[j];
            missing += new_word;
        }
    }
    if (v.count + missing > external_forms)
        return false;
    Word stimulus = (1u << e.address) | (16u << e.appearance);
    if (e.context)
        stimulus |= 256u << (e.context - 1);
    for (Word i = 0; i < said.n; ++i) {
        Word id = identify(v, said.wave[i]);
        if (!id) {
            id = ++v.count;
            v.forms[id - 1].wave = said.wave[i];
        }
        e.word[i] = id;
        auto &w = v.forms[id - 1];
        auto next = grounding::plastic({w.faint, w.strong}, stimulus, g.low & 65535);
        Word mask = e.context ? 4095 : 255;
        w.faint = (w.faint & ~mask) | (next.low & mask) | (e.context ? 0x80000000u : 0u);
        w.strong = (w.strong & ~mask) | (next.high & mask);
    }
    if (exchange)
        e.context |= 128;
    v.examples[v.written % sample_count] = e;
    ++v.written;
    if (v.kept < sample_count)
        ++v.kept;
    return true;
}
struct Pattern {
    Byte id[phrase_length]{};
    Word n = 0, request = 8, answer = 8, source = 8;
};
AX_FN Pattern construction(const Vocabulary &v, const RecollectionGenes &g, const Caption &seen) {
    Pattern p{};
    p.n = seen.n;
    for (Word i = 0; i < seen.n; ++i) {
        Word word = seen.word[i], firing = readings(v.forms[word - 1], g);
        if (firing == (1u << seen.address)) {
            if (p.request != 8)
                return {};
            p.request = i;
            p.id[i] = 128;
        } else if (firing == (16u << seen.appearance)) {
            if (p.answer != 8)
                return {};
            p.answer = i;
            p.id[i] = 129;
        } else if ((seen.context & 127) && firing == (256u << ((seen.context & 127) - 1))) {
            if (p.source != 8)
                return {};
            p.source = i;
            p.id[i] = 130;
        } else
            p.id[i] = word;
    }
    if (p.source == 8 && (p.request == 8 || p.answer == 8))
        return {};
    return p;
}
AX_FN Count term(const Vocabulary &v, const RecollectionGenes &law, Word feature) {
    Count word = 0;
    for (Word j = 0; j < v.count; ++j)
        if (readings(v.forms[j], law) == (1u << feature)) {
            if (word)
                return 0;
            word = v.forms[j].wave;
        }
    return word;
}
AX_FN Report interpret(const Vocabulary &v, const RecollectionGenes &law, const Animal &speaker,
                       const Sentence &speech) {
    Report r{};
    Word token[8]{};
    for (Word i = 0; i < speech.n; ++i) {
        token[i] = identify(v, speech.wave[i]);
        if (speech.wave[i] && !token[i])
            return r;
    }
    for (Word j = 0; j < v.kept; ++j) {
        auto &e = v.examples[j];
        bool paired = e.context & 128;
        if (e.n != speech.n + Word(paired))
            continue;
        if (paired) {
            bool complete = true;
            for (Word k = 0; k < speech.n; ++k)
                complete &= speech.wave[k] != 0;
            if (!complete)
                continue;
            token[speech.n] = 0;
        }
        auto p = construction(v, law, e);
        if (!p.n)
            continue;
        Word role = 3, bound[3]{4, 4, 4};
        bool match = true;
        for (Word i = 0; i < p.n; ++i) {
            if (p.id[i] < 128) {
                match &= p.id[i] == token[i];
                continue;
            }
            Word kind = p.id[i] - 128;
            if (!token[i]) {
                if (role != 3) {
                    match = false;
                    break;
                }
                role = kind;
                continue;
            }
            Word seen = readings(v.forms[token[i] - 1], law);
            if (!seen || (seen & (seen - 1))) {
                match = false;
                break;
            }
            Word cell = __builtin_ctz(seen);
            match &= (cell >> 2) == kind;
            bound[kind] = kind == 2 ? (cell & 3) : undo(cell & 3, speaker.genes.anatomy >> (kind == 0 ? 0 : 3));
        }
        if (!match || role >= 3 || (role == 0 && bound[1] >= 4) || (role == 1 && bound[0] >= 4))
            continue;
        if (r.recognized &&
            (r.role != role || r.property != bound[0] || r.appearance != bound[1] || r.source != bound[2]))
            return {};
        r.role = role;
        r.property = bound[0];
        r.appearance = bound[1];
        r.source = bound[2];
        r.referent = role == 0 ? bound[1] : bound[0];
        r.recognized = 1;
        ++r.frames;
        if (paired) {
            r.said = {};
            r.said.n = 1;
        } else
            r.said = speech;
    }
    return r;
}
AX_FN void complete(Report &r, const Vocabulary &v, const RecollectionGenes &g, const Animal &b, Word native) {
    Word bit = 4 * r.role + (r.role == 2 ? native : code(native, b.genes.anatomy >> (r.role == 1 ? 3 : 0)));
    Count wave = term(v, g, bit);
    if (!wave)
        return;
    for (Word j = 0; j < r.said.n; ++j)
        if (!r.said.wave[j])
            r.said.wave[j] = wave;
    r.accepted = 1;
}
AX_FN Intent exchange(const Pair &native, const Scene &s, Word desired, Word salt, Word lesion) {
    Intent a{};
    a.query = query(native.b, desired, mix(salt + 547), s.sink_pose);
    Word q = carried(a.query, lesion == 1 ? 0 : lesion == 3 ? 3 : 1, salt + 121);
    a.reply = reply(native.a, s.inlet, q, mix(salt + 733), s.source_pose, a);
    Word signal = carried(a.reply, lesion == 2 ? 0 : lesion == 4 ? 3 : 1, salt + 367);
    a.stroke = press(native.b, signal, s.opening, mix(salt + 971), s.sink_pose, a);
    return a;
}
AX_FN void feed(Room &room, Outcome &out, Word a, Word b) {
    Word allowed_a = (room.body & 1) ? 65535 - a + out.cost_a : 0,
         allowed_b = (room.body & 2) ? 65535 - b + out.cost_b : 0;
    if (out.speaker_share > allowed_a)
        out.speaker_share = allowed_a;
    if (out.receiver_share > allowed_b)
        out.receiver_share = allowed_b;
    room.site.carbon -= out.food;
    room.site.opening = out.angle;
    room.fruit += out.food - out.speaker_share - out.receiver_share;
}
AX_FN Report reply_to(Pair &native, const RecollectionGenes &g, Room &room, const Vocabulary &voc, Report r,
                      Word first_budget, Word second_budget, bool alter) {
    if (!r.recognized)
        return r;
    if ((r.source < 4 && r.source != room.context) || (r.role == 2 && room.context >= 4)) {
        r.said = {};
        return r;
    }
    if (r.role == 2 && (r.property >= 4 || r.appearance >= 4)) {
        if (r.property < 4 || r.appearance < 4) {
            r.said = {};
            return r;
        }
        r.candidates = 1u << room.context;
        r.spoken = room.context;
        complete(r, voc, g, native.b, room.context);
        return r;
    }
    Word options = 0, key = mix(room.seed + room.step++ * 191);
    // A request for an attribute is one address. Recovering an attribute
    // from a value can require asking about all four private readings.
    for (Word j = 0; j < (r.role != 0 ? 1u : 4u); ++j) {
        if (first_budget < 2 || second_budget < 3) {
            r.said = {};
            return r;
        }
        Word band = r.role != 0 ? r.referent : j;
        auto act = exchange(native, room.site, band, key, room.wire);
        Word heard_r = carried(act.reply, room.wire == 2 ? 0 : room.wire == 4 ? 3 : 1, key + 367);
        r.queries += act.query < 4;
        r.answers += act.reply < 4;
        if (r.role != 0) {
            if (!(room.body & 4))
                act.stroke = 4;
            auto out = squeeze(room.site, act, native.b.genes.anatomy);
            feed(room, out, first_budget, second_budget);
            r.cost_a += out.cost_a;
            r.cost_b += out.cost_b;
            r.food_a += out.speaker_share;
            r.food_b += out.receiver_share;

            room.last = act;
            room.effect = out;
            room.exposed = (room.site.seen & 2) && out.actual < 4;
            if (heard_r < 4)
                options |= 1u << act.recall;
            learn(native.a, native.b, room.site, act, out, 1, key, alter && !room.wire, band, room.wire);
        } else {
            Word ca = act.reply < 4, cb = act.query < 4;
            act.stroke = 4;
            Outcome evidence{};
            evidence.cost_a = ca;
            evidence.cost_b = cb;
            learn(native.a, native.b, room.site, act, evidence, 1, key, alter && !room.wire, band, room.wire);
            first_budget -= ca;
            second_budget -= cb;
            r.cost_a += ca;
            r.cost_b += cb;
            if (heard_r >= 4) {
                r.said = {};
                return r;
            }
            if (act.recall == r.referent)
                options |= 1u << j;
        }
    }
    r.candidates = options;
    if (options && !(options & (options - 1))) {
        Word chosen = __builtin_ctz(options);
        if (r.role == 2) {
            if (chosen == r.appearance) {
                r.spoken = room.context;
                complete(r, voc, g, native.b, room.context);
            }
        } else {
            r.spoken = chosen;
            complete(r, voc, g, native.b, chosen);
        }
    }
    if (!r.accepted)
        r.said = {};
    return r;
}
AX_FN Report converse(Pair &native, const RecollectionGenes &g, Room &room, const Vocabulary &voc,
                      const Sentence &speech, Word a, Word b, bool alter) {
    return reply_to(native, g, room, voc, interpret(voc, g, native.b, speech), a, b, alter);
}
} // namespace axion::life::siphon
KERNEL void siphon_converse(siphon::Pair *native, siphon::Room *room, siphon::Vocabulary *v, RecollectionGenes lex,
                            Word command, Word seed, Word inlet, Word band, Word angle, Word lesion, Word a_budget,
                            Word b_budget, Word plastic, Word body, const clay::Mind *cortex, siphon::Sentence speech,
                            siphon::Report *out) {
    if (tid())
        return;
    using namespace siphon;
    Report r{};
    if (command && cortex)
        room->context = clay::resolved(*cortex);
    if (command == 0) {
        *room = {};
        room->site = pool(seed, 4, 2);
        room->body = body;
        room->site.seen = (body >> 3) & 3;
        room->site.shared = 0;
        room->seed = seed;
        room->context = cortex ? clay::resolved(*cortex) : 4;
        if (inlet < 256) {
            room->site.inlet = inlet;
            room->site.membrane = band;
            room->site.opening = angle;
        }
        room->wire = lesion;
    } else if (command == 1 && a_budget >= 4 && b_budget >= 4) {
        Word k = mix(room->seed + room->step * 191);
        auto act = meet(native->a, native->b, room->site, k, 1, lesion);
        if (!(room->body & 4))
            act.stroke = 4;
        auto e = squeeze(room->site, act, native->b.genes.anatomy);
        feed(*room, e, a_budget, b_budget);
        learn(native->a, native->b, room->site, act, e, 1, k, plastic && !lesion, room->site.membrane, lesion);
        room->exposed = (room->site.seen & 2) && act.stroke < 4;

        room->last = act;
        room->effect = e;
        r.accepted = 1;
        r.queries = act.query < 4;
        r.answers = act.reply < 4;
        r.cost_a = e.cost_a;
        r.cost_b = e.cost_b;
        r.food_a = e.speaker_share;
        r.food_b = e.receiver_share;
        ++room->step;
    } else if (command == 1) {
        room->exposed = 0;
        room->last = {};
    } else if (command == 2 || command == 4) {
        r.accepted = caption(*v, native->b, lex, *room, speech, command == 4);
        r.forms = v->count;
    } else if (command == 3) {
        room->wire = lesion;
        r = converse(*native, lex, *room, *v, speech, a_budget, b_budget, plastic);
    }
    *out = r;
}
KERNEL void siphon_sentences(const siphon::Pair *partners, const siphon::Vocabulary *v, RecollectionGenes lex,
                             const siphon::Test *cases, siphon::Report *reply, Word n, Word upstream, Word outlet,
                             Word body) {
    Word i = tid();
    if (i >= n)
        return;
    auto &item = cases[i];
    auto actors = *partners;
    siphon::Room room{};
    room.site = siphon::pool(item.seed, 4, 0);
    room.seed = item.seed;
    room.body = body;
    room.wire = item.wire;
    room.site.inlet = item.inlet;
    room.site.membrane = item.filter;
    room.site.opening = item.angle;
    reply[i] = siphon::converse(actors, lex, room, *v, item.words, upstream, outlet, false);
}
KERNEL void siphon_coherence(const siphon::Pair *pair, siphon::Reading *records, Word n, Word seed, Word mode) {
    Word i = tid();
    if (i >= n)
        return;
    siphon::Pair actors = *pair;
    if (mode == 5) {
        actors.a.address = actors.a.surface = actors.b.address = actors.b.surface = 0;
    }
    if (mode == 6)
        actors.a.tendon = actors.b.tendon = 0;
    siphon::Reading r{};
    siphon::assess(actors, mix(seed + i * 3251), 4, 1, mode < 5 ? mode : 0, r);
    records[i] = r;
}
