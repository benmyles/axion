#include "grounding_world.cuh"
#include "grounding_questions.cuh"
using namespace axion;
using namespace axion::life;
using namespace axion::life::grounding;
#define KERNEL extern "C" __attribute__((global))
KERNEL void ground_lens(Lens *lens) {
    Word v = block(), t = lane();
    for (Word j = t; j < hypotheses; j += 32) {
        auto c = physical_bank.cube[j];
        if (!v)
            lens->cell[j] = c;
        Word group = ballot((v & c.mask) == c.value);
        if (!t)
            lens->active[v][j / 32] = group;
    }
}
KERNEL void ground_school(Mind *children, const Lens *lens, const Vocabulary *v, const RecollectionGenes *g,
                          Word siblings, Word seed, Word style, Word rounds, Word crowd, Word cut) {
    Word i = block(), t = lane();
    auto &m = children[i];
    if (!t) {
        m = {};
        m.genes = g[i / siblings];
    }
    sync();
    Word key = mix(seed + (i % siblings) * 0x9e3779b9u), rng = mix(key + 317);
    auto c = tabletop::culture(*v, key, style);
    __attribute__((shared)) alignas(Scene) static unsigned char eye_bytes[sizeof(Scene)];
    auto &seen = *reinterpret_cast<Scene *>(eye_bytes);
    __attribute__((shared)) alignas(Phrase) static unsigned char ear_bytes[sizeof(Phrase)];
    auto &heard = *reinterpret_cast<Phrase *>(ear_bytes);
    for (Word j = 0; j < rounds; ++j) {
        if (!t) {
            auto physical = tabletop::room(tabletop::draw(rng), crowd, 0);
            auto f = tabletop::describe(c, physical.truth, tabletop::draw(rng), false);
            auto surfaces = tabletop::utter(c, f);
            if (cut == 2 && (j & 1)) {
                auto swap = surfaces.waves[0];
                surfaces.waves[0] = surfaces.waves[2];
                surfaces.waves[2] = swap;
            }
            heard = f.ok ? listen(m, surfaces, true) : Phrase{};
            if (cut == 1)
                physical = tabletop::room(mix(key + j * 82547), crowd, 0);
            seen = see(physical.film, m.genes);
        }
        sync();
        expose(m, *lens, seen, heard);
        sync();
    }
}
KERNEL void ground_words(Mind *m, const Lens *lens, Word n) {
    Word j = block() % (forms + 1), i = block() / (forms + 1);
    if (i >= n || !j || j > m[i].count)
        return;
    __attribute__((shared)) static Word scratch[bundles];
    consolidate_word(m[i], *lens, j, scratch);
}
KERNEL void ground_bind(Mind *m) { bind(m[block()]); }
KERNEL void ground_probe(const Mind *m, const Vocabulary *v, GroundingAudit *out, Word siblings, Word seed, Word style,
                         Word probes, Word crowd) {
    Word i = block(), t = lane(), key = mix(seed + (i % siblings) * 0x9e3779b9u);
    auto c = tabletop::culture(*v, key, style);
    const auto &s = m[i];
    GroundingAudit result;
    for (Word name = t; name < 8; name += 32) {
        Word id = recognize(s, c.voice.noun[name]);
        bool exact = id != 0;
        for (Word p = 0; p < 256 && exact; ++p) {
            bool target = tabletop::identity(c, p) == name;
            auto &w = s.word[id];
            exact = admits(w.certain, p) == target && admits(w.possible, p) == target;
        }
        result.names += exact;
        if (id)
            for (Word v = 0; v < 256; ++v) {
                bool wanted = tabletop::identity(c, v) == name;
                auto &w = s.word[id];
                if (w.viable)
                    result.positions += wanted ? 7 * Word(admits(w.certain, v)) : Word(!admits(w.possible, v));
            }
    }
    if (t < 6) {
        Word r = 0;
        while (c.order[r] != 1)
            ++r;
        Word sense = c.verb[t];
        if (c.order[left_slot(r)] == 2 && (sense < 18 || sense == 20 || sense == 21))
            sense ^= 1;
        Word id = recognize(s, c.voice.verb[t]);
        result.verbs = s.layout == (1u << r) && id && s.edge[r][id].possible == (1u << sense);
    }
    for (Word j = t; j < probes; j += 32) {
        auto physical = tabletop::room(mix(key ^ mix(j + 53791)), crowd, 1);
        auto fact = tabletop::describe(c, physical.truth, mix(key + j + 481), true);
        if (!fact.ok)
            continue;
        auto scene = see(physical.film, s.genes);
        // Mask one acoustic position. Only a unique answer in the external
        // scene can earn credit. The learner receives the remaining sounds.
        Word role = j % 3, gap = 0;
        while (c.order[gap] != role)
            ++gap;
        Word type = role == 0 ? 0 : role == 2 ? 1 : 2;
        Word valid = tabletop::answers(c, physical.truth, fact, type);
        if (valid && !(valid & (valid - 1))) {
            auto prompt = hear(s, tabletop::utter(c, fact));
            Construction masked;
            masked.answer_slot = static_cast<Byte>(gap);
            for (Word j = 0; j < 3; ++j)
                masked.field[j] = static_cast<Byte>(j == gap ? 0 : j + 1);
            prompt.id[gap] = 0;
            auto answer = interpret(s, scene, masked, prompt, {});
            ++result.expected;
            result.spoken += answer.complete && answer.wave == tabletop::answer_wave(c, type, __builtin_ctz(valid));
        }
        for (Word mode = 0; mode < 2; ++mode) {
            auto event = fact;
            if (mode) {
                event.left = fact.right;
                event.right = fact.left;
            }
            // The reverse can itself be a true proposition. Inspect the world.
            Word expected = tabletop::oracle(c, physical.truth, event),
                 heard = truth(s, scene, hear(s, tabletop::utter(c, event)));
            ++result.cases;
            result.full += heard == expected + 1;
            result.wrong += heard && heard != expected + 1;
            result.silent += !heard;
            if (mode && !expected) {
                ++result.trials;
                result.bindings += heard == 1;
            }
        }
    }
    Word *counts = reinterpret_cast<Word *>(&result);
    for (Word j = 0; j < sizeof(result) / sizeof(Word); ++j) {
        Word x = counts[j];
        for (Word k = 16; k; k >>= 1)
            x += through(x, t ^ k);
        if (!t)
            reinterpret_cast<Word *>(out + i)[j] = x;
    }
}

KERNEL void ground_colloquy(Mind *m, const Vocabulary *v, Word siblings, Word seed, Word style, Word quantity,
                            Word crowd) {
    Word i = block() * 32 + lane();
    if (i >= siblings)
        return;
    Word key = mix(seed + i * 0x9e3779b9u), rng = mix(key + 138091);
    auto c = tabletop::culture(*v, key, style);
    auto &s = m[i];
    Word completed = 0;
    for (Word j = 0; j < quantity * 96 && completed < quantity; ++j) {
        Word type = completed % 5;
        auto room = tabletop::room(tabletop::draw(rng), crowd, 0);
        auto f = tabletop::describe(c, room.truth, tabletop::draw(rng), false);
        if (!f.ok)
            continue;
        if (type == 3 && ((completed / 5) & 1)) {
            f.left = static_cast<Byte>(tabletop::draw(rng) & 7);
            if (tabletop::oracle(c, room.truth, f) || tabletop::novel(c, f))
                continue;
        }
        auto set = tabletop::answers(c, room.truth, f, type);
        if (!set || (set & (set - 1)))
            continue;
        auto last = f;
        last.left = (f.left + 3 + (j % 3)) % 8;
        last.right = (f.right + 1 + j % 4) % 8;
        // Earlier turns are independent of the new query except when teaching
        // ellipsis. The referring expression has to learn which term persists.
        if (type != 4)
            last.verb = (f.verb + 1 + (j % 4)) % 6;
        auto prior = hear(s, tabletop::utter(c, last));
        auto q = listen(s, tabletop::ask(c, f, type), true);
        Word answer = learn_wave(s, tabletop::answer_wave(c, type, __builtin_ctz(set)));
        notice(s, see(room.film, s.genes), q, answer, prior);
        ++completed;
    }
}
KERNEL void ground_question_fit(Mind *m) { fit_questions(m[block()]); }
KERNEL void ground_interview(const Mind *m, const Vocabulary *v, InterviewAudit *out, Word seed, Word style,
                             Word probes, Word crowd) {
    Word i = block(), t = lane(), key = mix(seed + i * 0x9e3779b9u);
    auto c = tabletop::culture(*v, key, style);
    const auto &s = m[i];
    InterviewAudit result;
    for (Word j = t; j < probes; j += 32) {
        auto room = tabletop::room(mix(key ^ mix(71401 + j)), crowd, 1);
        auto scene = see(room.film, s.genes);
        auto f = tabletop::describe(c, room.truth, mix(key + j + 4787), true);
        if (!f.ok)
            continue;
        for (Word type = 0; type < 5; ++type) {
            auto ask = f;
            if (type == 3 && (j & 1)) {
                ask.left = f.right;
                ask.right = f.left;
            }
            auto before = ask;
            before.left = (ask.left + 1) % 8;
            before.right = (ask.right + 3) % 8;
            if (type != 4)
                before.verb = (ask.verb + 1) % 6;
            auto want = tabletop::answers(c, room.truth, ask, type);
            auto q = hear(s, tabletop::ask(c, ask, type));
            auto prior = hear(s, tabletop::utter(c, before));
            auto reply = respond(s, scene, q, prior);
            if (!want || (want & (want - 1))) {
                ++result.ambiguous;
                result.quiet += !reply.wave;
                continue;
            }
            ++result.valid[type];
            bool correct = reply.wave == tabletop::answer_wave(c, type, __builtin_ctz(want));
            result.exact[type] += correct;
            result.wrong[type] += reply.wave && !correct;
        }
    }
    auto *voted = reinterpret_cast<Word *>(&result);
    for (Word j = 0; j < sizeof(result) / sizeof(Word); ++j) {
        Word x = voted[j];
        for (Word k = 16; k; k >>= 1)
            x += through(x, t ^ k);
        if (!t)
            reinterpret_cast<Word *>(out + i)[j] = x;
    }
}

KERNEL void ground_merit(const GroundingAudit *score, Word *fitness, Word n, Word siblings) {
    Word i = block() * 32 + lane();
    if (i >= n)
        return;
    Word value = 0;
    for (Word j = 0; j < siblings; ++j) {
        auto &a = score[i * siblings + j];
        Word errors = a.wrong;
        value += (a.expected ? 1900 * a.spoken / a.expected : 0);
        value += a.positions / 4 + 24 * a.names + 32 * a.verbs + (a.trials ? (228 * a.bindings / a.trials) : 0);
        value += (a.cases ? (288 * a.full / a.cases) : 0) + (a.cases ? 72 * (a.cases - errors) / a.cases : 0);
    }
    fitness[i] = value;
}
KERNEL void ground_stage(Mind *m, const View *v) {
    m->here = see(*v, m->genes);
    ++m->sights;
}
KERNEL void ground_capture(Mind *m, const Lens *lens, const Voice *utterance) {
    __attribute__((shared)) static Byte bytes[sizeof(Phrase)];
    auto &text = *reinterpret_cast<Phrase *>(bytes);
    if (!lane())
        text = listen(*m, *utterance, true);
    sync();
    expose(*m, *lens, m->here, text);
}
KERNEL void ground_exchange(Mind *m, const Voice *q, Count answer) {
    auto p = listen(*m, *q, true);
    Word heard = learn_wave(*m, answer);
    notice(*m, m->here, p, heard, m->discourse);
}
KERNEL void ground_consult(Mind *m, const Voice *q, Answer *out, Clarification *turn) {
    auto p = hear(*m, *q);
    *out = {};
    if (!p.n)
        return;
    *out = clarify(*m, m->here, p, m->discourse, *turn);
    if (out->complete) {
        m->discourse = out->statement;
        return;
    }
    if (!out->hypotheses && p.n == 3) {
        auto v = read(*m, m->here, p);
        if (!v.seen)
            return;
        m->discourse = p;
        out->hypotheses = 1;
        out->statement = p;
        Word result = v.certain ? 2 : v.possible ? 0 : 1;
        for (Word k = 0; k < m->constructions; ++k) {
            auto &frame = m->construction[k];
            if (frame.answer_slot != 3)
                continue;
            Word id = result == 2 ? frame.yes : result == 1 ? frame.no : 0;
            if (id) {
                out->wave = m->word[id].wave;
                out->complete = 1;
            }
            break;
        }
    }
}

KERNEL void ground_point(const Mind *m, Count wave, Count *sites) {
    const Word id = recognize(*m, wave);
    Count certain = 0, possible = 0;
    if (id)
        for (Word i = 0; i < m->here.n; ++i) {
            auto p = m->here.patch[i];
            Count bit = 1ull << p.site;
            if (admits(m->word[id].certain, p.light))
                certain |= bit;
            if (admits(m->word[id].possible, p.light))
                possible |= bit;
        }
    sites[0] = certain;
    sites[1] = possible;
}

KERNEL void ground_repair(const Mind *children, const Vocabulary *v, RepairAudit *out, Word seed, Word style,
                          Word probes, Word crowd) {
    Word i = block(), t = lane(), key = mix(seed + i * 0x9e3779b9u);
    auto c = tabletop::culture(*v, key, style);
    const auto &m = children[i];
    RepairAudit audit;
    for (Word j = t; j < probes; j += 32) {
        auto room = tabletop::room(mix(key ^ mix(j + 429013)), crowd, 1);
        auto f = tabletop::describe(c, room.truth, mix(key + j + 11771), true);
        if (!f.ok)
            continue;
        Word kind = j & 1, possible = tabletop::answers(c, room.truth, f, kind), count = __builtin_popcount(possible);
        if (count < 2)
            continue;
        ++audit.trials;
        audit.chance1024 += 1024 / count;
        Word kth = mix(key ^ mix(j + 17)) % count;
        while (kth--)
            possible &= possible - 1;
        Count target = tabletop::answer_wave(c, kind, __builtin_ctz(possible));
        auto scene = see(room.film, m.genes);
        auto question = hear(m, tabletop::ask(c, f, kind));
        for (Word intervention = 0; intervention < 2; ++intervention) {
            Clarification turn;
            auto a = clarify(m, scene, question, {}, turn);
            for (Word round = 0; a.asking && round < objects; ++round) {
                if (!intervention)
                    ++audit.requests;
                bool confirm = intervention ? bool(mix(key + j * 53 + round) & 1) : a.wave == target;
                Voice sound;
                sound.n = 1;
                sound.waves[0] = c.voice.particle[confirm ? 0 : 1];
                a = clarify(m, scene, hear(m, sound), {}, turn);
            }
            bool correct = a.complete && a.wave == target;
            if (!intervention) {
                audit.answered += a.complete;
                audit.correct += correct;
                audit.wrong += a.complete && !correct;
            } else
                audit.misinformed += correct;
        }
    }
    auto *fields = reinterpret_cast<Word *>(&audit);
    for (Word k = 0; k < sizeof(audit) / sizeof(Word); ++k) {
        Word x = fields[k];
        for (Word d = 16; d; d >>= 1)
            x += through(x, t ^ d);
        if (!t)
            reinterpret_cast<Word *>(out + i)[k] = x;
    }
}
