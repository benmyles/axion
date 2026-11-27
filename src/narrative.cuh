#include "axion/narrative_image.hpp"
#include "axion/narrative_school.hpp"
#include "axion/narrative_recall.hpp"

namespace axion::life::utterance {
AX_FN Phrase exchange_words(const Vocabulary &speaker, const Phrase &line, Vocabulary &pupil, bool recruit) {
    Phrase heard{};
    for (Word j = 0; j < line.n; ++j)
        if (!heard.push(pupil.find(speaker.sounds[line.word[j] - 1], recruit)))
            return {};
    return heard;
}
AX_FN Phrase public_answer(const Culture &culture, Memory *mentor, Word task, const Scene &scene, Vocabulary &listener,
                           bool learn) {
    if (!mentor)
        return hear_tutor(listener, culture, tell(culture, task, culture.language == native ? scene.peer : scene.truth),
                          learn);
    auto q = hear_tutor(mentor->vocabulary, culture, question(task), false);
    Word id = find(*mentor, address(q));
    if (id == prompts)
        return {};
    return exchange_words(mentor->vocabulary, speak(mentor->script[id], scene.peer), listener, learn);
}
AX_FN void teach_one(Memory *target, Senses body, const Culture *teacher, Course c, Memory *mentor, Senses peer,
                     Teaching *result) {
    auto &m = *target;
    Teaching r{};
    Growth g{};
    g.cells = body.cells;
    for (Word t = 0; t < c.singles + c.couples; ++t) {
        bool two = t >= c.singles;
        if (c.purse - r.heat < 1 + Word(two))
            break;
        r.heat += 1 + Word(two);
        Word key = mix(c.seed ^ mix(t * 5099 + 16081));
        auto scene = stage(body, c, key, 1 + Word(two), false,
                           mentor                        ? &peer
                           : teacher->language == native ? &teacher->origin
                                                         : nullptr);
        auto picture = scene.look;
        if (c.arm == exchanged_scenes) {
            Word start = two ? c.singles : 0, span = two ? c.couples : c.singles;
            Word other = ((t - start) ^ 1u) < span ? start + ((t - start) ^ 1u) : t;
            picture = stage(body, c, mix(c.seed ^ mix(other * 5099 + 16081)), 1 + Word(two), false).look;
        }
        if (picture.size() != 1 + Word(two)) {
            ++r.unseen;
            continue;
        }
        Word tasks = teacher->language == native ? 1u : 4u;
        for (Word task = 0; task < tasks; ++task) {
            auto vocabulary = m.vocabulary;
            auto q = hear_tutor(vocabulary, *teacher, question(task), true);
            auto sounds = public_answer(*teacher, mentor, task, scene, vocabulary, true);
            Word cost = 2 * (q.n + sounds.n);
            if (!q.n || !sounds.n || cost > c.purse - r.heat)
                continue;
            r.heat += cost;
            m.vocabulary = vocabulary;
            if (c.arm == shuffled_words && sounds.n > 1) {
                Word k = mix(key + task * 1439);
                for (Word j = sounds.n - 1; j; --j) {
                    Word i = k % (j + 1);
                    auto save = sounds.word[j];
                    sounds.word[j] = sounds.word[i];
                    sounds.word[i] = save;
                    k = mix(k + 291);
                }
            }
            r.spoken += sounds.n;
            if (c.arm != fixed && !(two && c.arm == separated))
                r.paired += lesson(m, address(q), picture, sounds, body.learning, g);
        }
        if (two)
            ++r.couples;
        else
            ++r.singles;
    }
    r.edits = g.contacts;
    r.revisions = g.revisions;
    r.spikes = g.firings;
    r.terminal = g.cells.spike;
    *result = r;
}
KERNEL void narrative_school(Memory *target, Senses body, const Culture *teacher, Course c, Memory *mentor, Senses peer,
                             Teaching *result) {
    teach_one(target, body, teacher, c, mentor, peer, result);
}
KERNEL void narrative_cohort(Memory *brains, const Senses *animals, Word n, const Culture *teacher, Course c,
                             Word tests, Word *scores) {
    Word i = tid();
    if (i >= n)
        return;
    auto &m = brains[i];
    auto body = animals[i];
    Teaching learned{};
    Course practice = c;
    practice.beds = nullptr;
    practice.sites = 0;
    teach_one(&m, body, teacher, practice, nullptr, body, &learned);
    Word exact = 0;
    for (Word t = 0; t < tests; ++t) {
        Word task = t & 3;
        auto scene = stage(body, c, mix(c.seed ^ mix(0x8b3a2941u + t * 26693u)), c.length, true);
        auto q = hear_tutor(m.vocabulary, *teacher, question(task), false);
        Word id = find(m, address(q));
        if (id != prompts && body.mouth) {
            auto actual = speak(m.script[id], scene.look);
            auto right = hear_tutor(m.vocabulary, *teacher, tell(*teacher, task, scene.truth), false);
            exact += right.n && same(actual, right);
        }
    }
    scores[i] = exact;
}
KERNEL void narrative_compile(const Memory *target, Chip *chips) {
    Word i = tid(), q = i / 32, cell = i & 31;
    if (q >= prompts)
        return;
    auto &s = target->script[q];
    auto &c = chips[q];
    if (!cell) {
        c.duration = compile(s.duration);
        c.lengths = 0;
        for (Word j = 0; j < shapes; ++j)
            c.lengths |= Word(s.units[j].length) << (4 * j);
        for (Word j = 0; j < styles; ++j)
            c.seams[j] = s.seams[j];
        c.rejected = s.rejected;
        c.pairs = s.pairs;
    }
    if (cell < shapes * phases)
        c.motor[cell / phases][cell % phases] =
            compile(s.units[cell / phases].motor[cell % phases], &s.duration, s.units[cell / phases].length);
}
KERNEL void narrative_residuals(const Memory *m, Chip *chips, Socket *residual) {
    Word next = 0;
    for (Word q = 0; q < prompts; ++q) {
        auto &c = chips[q];
        const auto &s = m->script[q];
        if (c.duration == fallback) {
            residual[next] = s.duration;
            c.duration |= next++;
        }
        for (Word k = 0; k < shapes; ++k)
            for (Word i = 0; i < phases; ++i)
                if (c.motor[k][i] == fallback) {
                    residual[next] = s.units[k].motor[i];
                    c.motor[k][i] |= next++;
                }
    }
}
KERNEL void narrative_dictionary(const Socket *residual, const Chip *c, Packet *out) {
    Word i = tid(), q = i >> 8;
    if (q < prompts)
        out[i] = compact(c[q], residual, i & 255);
}
AX_FN Phrase transcribe(const Culture &c, const Vocabulary &v, const Phrase &line) {
    Phrase out{};
    for (Word j = 0; j < line.n; ++j) {
        Word k = 0;
        Count wave = v.sounds[line.word[j] - 1];
        while (k < names && c.waves[k] != wave)
            ++k;
        if (k == names || !out.push(k + 1))
            return {};
    }
    return out;
}
KERNEL void narrative_references(Memory *m, Senses body, const Culture *teacher, Course c, ReferenceCourse lesson,
                                 Teaching *prior, ReferenceResult *log, Memory *mentor, Senses peer) {
    Growth g{};
    g.cells = body.cells;
    g.cells.spike = prior->terminal;
    ReferenceResult budget{};
    for (Word family = 0; family < 2; ++family)
        for (Word j = 0; j < lesson.examples; ++j) {
            auto scene = reference_scene(body, c, family, j, lesson.swapped, mentor ? &peer : nullptr);
            Word k = reference_target(scene.truth, family, j & 1, lesson.swapped);
            if (k == depth)
                continue;
            auto words = reference_question(*teacher, family, j & 1);
            auto vocabulary = m->vocabulary;
            auto prompt = hear_tutor(vocabulary, *teacher, words, true);
            Phrase reply{};
            if (mentor) {
                auto cue = hear_tutor(mentor->vocabulary, *teacher, words, false);
                auto recited = recall(*mentor, cue, scene.peer);
                if (peer.mouth)
                    reply = exchange_words(mentor->vocabulary, phrase(recited.reply), vocabulary, true);
            } else
                reply = hear_tutor(vocabulary, *teacher,
                                   phrase(sentence(*teacher, family ? 3u : 0u, scene.truth.at(k))), true);
            Word cost = scene.truth.size() + 2 * (words.n + reply.n);
            if (cost > c.purse - prior->heat - budget.paid || !reply.n || !prompt.n)
                continue;
            m->vocabulary = vocabulary;
            auto seen =
                lesson.mispaired
                    ? reference_scene(body, c, family, (j ^ 2u) < lesson.examples ? j ^ 2u : j, lesson.swapped).look
                    : scene.look;
            budget.paid += cost;
            budget.spoken += reply.n;
            if (c.arm != fixed)
                budget.taught += attend_query(*m, prompt, reply, seen, body.learning, g);
        }
    budget.edits = g.contacts;
    budget.spikes = g.firings;
    prior->heat += budget.paid;
    prior->terminal = g.cells.spike;
    *log = budget;
}
KERNEL void narrative_reference_exam(Memory *m, Senses body, const Culture *teacher, Course c, ReferenceCourse course,
                                     Word n, ReferenceExam *out) {
    Word i = tid();
    if (i >= n)
        return;
    Word family = (i >> 1) & 1, colour = 2 + (i & 1);
    auto scene = stage(body, c, mix(c.seed ^ mix((i >> 2) * 24683 + 0x19b7effu)), c.length, true);
    Word t = reference_target(scene.truth, family, colour, course.swapped);
    auto q = reference_question(*teacher, family, colour ^ course.other);
    auto heard = hear_tutor(m->vocabulary, *teacher, q, false);
    auto choice = recall(*m, heard, scene.look);
    auto line = transcribe(*teacher, m->vocabulary, phrase(choice.reply));
    Count raw = Count(line.n) << 56;
    for (Word j = 0; j < line.n; ++j)
        raw |= Count(line.word[j]) << (6 * j);
    Packet said{body.mouth ? raw : 0};
    auto wanted = t == depth ? Packet{} : sentence(*teacher, family ? 3u : 0u, scene.truth.at(t));
    out[i] = {scene.truth.bits, scene.look.bits, said, wanted, t, choice.event, choice.routes, choice.recognized};
}
KERNEL void narrative_express(Memory *m, Senses body, const Culture *tutor, Course c, const Chip *chips,
                              const Socket *residual, const Packet *dictionary, Word n, Exam *reports,
                              WordSample *samples) {
    Word i = tid();
    if (i >= n)
        return;
    bool twin = i & 1;
    Word tasks = tutor->language == native ? 1u : 4u, task = (i / 2) % tasks, life = (i / 2) / tasks;
    auto scene = stage(body, c, mix(c.seed ^ mix(life * 1619 + 0xa9213u)), c.length, true,
                       tutor->language == native ? &tutor->origin : nullptr);
    if (twin) {
        scene.truth = reversed(scene.truth);
        scene.look = reversed(scene.look);
        scene.peer = reversed(scene.peer);
    }
    auto expected = tell(*tutor, task, tutor->language == native ? scene.peer : scene.truth);
    auto q = hear_tutor(m->vocabulary, *tutor, question(task), false);
    Word id = find(*m, address(q));
    Phrase voice{};
    Trail decoded{};
    if (id != prompts) {
        auto &s = m->script[id];
        const auto &chip = chips[id];
        if (body.mouth) {
            if (c.fast == 2)
                voice = compose(chip, scene.look, [&](Word x) { return dictionary[256 * id + x]; });
            else if (c.fast)
                voice = compose(chip, scene.look, [&](Word x) { return compact(chip, residual, x); });
            else
                voice = speak(s, scene.look);
        }
        if (c.decoding && !task) {
            auto words = hear_tutor(m->vocabulary, *tutor, expected, false);
            if (c.hearing == 1)
                for (Word j = 0; j < words.n / 2; ++j) {
                    auto tmp = words.word[j];
                    words.word[j] = words.word[words.n - 1 - j];
                    words.word[words.n - 1 - j] = tmp;
                }
            if (c.hearing == 2)
                words = {};
            if (c.hearing == 3)
                words = voice;
            decoded = listen(chip, words, [&](Word x) { return dictionary[256 * id + x]; });
        }
    }
    auto public_speech = transcribe(*tutor, m->vocabulary, voice);
    reports[i] = {scene.truth.bits,
                  scene.look.bits,
                  decoded.bits,
                  acoustic_digest(public_speech),
                  acoustic_digest(expected),
                  task,
                  voice.n,
                  expected.n,
                  Word(expected.n && same(public_speech, expected)),
                  Word(!task && decoded.bits == scene.look.bits && decoded.size() == c.length),
                  scene.truth.size(),
                  scene.look.size(),
                  scene.stains,
                  Word(id != prompts)};
    if (samples)
        samples[i] = {expected, public_speech};
}
// One lane per independently presented history, including silent and ambiguous programs.
template <Word engine>
AX_FN void stream_descriptions(const Memory *m, const Chip *chips, const Socket *residual, const Packet *dictionary,
                               Word seed, Word n, Word length, Count *digest) {
    Word i = tid();
    if (i >= n)
        return;
    Word q = i & 3, low = mix(seed ^ i), high = mix(low + 19463);
    Count images = (Count(high) << 32) | low, mask = 0x00ffffffffffffffull >> ((depth - length) * 8);
    Trail scene{(Count(length) << 56) | (images & mask)};
    auto &s = m->script[q];
    auto &chip = chips[q];
    Phrase line{};
    if constexpr (engine == 0)
        line = speak(s, scene);
    else if constexpr (engine == 1)
        line = compose(chip, scene, [&](Word e) { return compact(chip, residual, e); });
    else
        line = compose(chip, scene, [&](Word e) { return dictionary[256 * q + e]; });
    digest[i] = acoustic_digest(line) ^ line.n;
}
#define AX_STREAM(name, method)                                                                                        \
    KERNEL void name(const Memory *m, const Chip *chips, const Socket *residual, const Packet *dictionary, Word seed,  \
                     Word n, Word length, Count *digest) {                                                             \
        stream_descriptions<method>(m, chips, residual, dictionary, seed, n, length, digest);                          \
    }
AX_STREAM(narrative_stream_plastic, 0)
AX_STREAM(narrative_stream_packed, 1)
AX_STREAM(narrative_stream_table, 2)
#undef AX_STREAM
KERNEL void narrative_feed(const Exam *reported, Word n, Course c, Senses receiver, Meal *meals) {
    Word i = tid();
    if (i >= n)
        return;
    auto &r = reported[i];
    Meal m{};
    Trail true_path{r.physical}, committed{r.heard};
    if (!r.task) {
        for (Word j = 0; j < true_path.size(); ++j) {
            Word e = true_path.at(j);
            if (committed.size() == true_path.size()) {
                Word claim = committed.at(j);
                Word from = braid::pigment(receiver.glass.places, e & 3) ^ c.hand;
                Word to = braid::pigment(receiver.glass.places, (e >> 2) & 3) ^ c.hand;
                Word inlet = braid::pigment(receiver.glass.hues, (e >> 4) & 3);
                Word end = braid::pigment(receiver.glass.hues, e >> 6);
                m.upper += Word((claim & 3) == from && ((claim >> 4) & 3) == inlet);
                m.lower += Word(((claim >> 2) & 3) == to && (claim >> 6) == end);
            }
        }
        m.settled = 2 * true_path.size() - m.upper - m.lower;
    }
    meals[i] = m;
}
KERNEL void narrative_table(Workbench *table, Saved *minds, const Senses *bodies, Request q, Reply *output) {
    Reply r{};
    auto &t = *table;
    auto &person = minds[q.who];
    auto &other = minds[1 - q.who];
    auto &m = person.memory;
    const auto &body = bodies[q.who];
    if (q.action == open) {
        t = {};
        t.bed = q.bed;
        t.ready = 1;
        minds[0].trail = {};
        minds[1].trail = {};
    } else if (q.action == clear) {
        person.trail = {};
    } else if (q.action == pour) {
        if (!t.ready || t.stock < 2 || t.cast)
            r.status = 1;
        else {
            auto second = t.bed;
            auto e = release(t.bed, q.source, bodies[0]), f = release(second, q.source, bodies[1]);
            r.event = e;
            r.witnessed = e.available;
            if (e.available) {
                if (q.energy > 1 && (t.shutters & 1)) {
                    minds[0].trail.append(e.view);
                    r.teacher_heat = 1;
                }
                if (q.other_energy > 1 && (t.shutters & 2)) {
                    minds[1].trail.append(f.view);
                    r.pupil_heat = 1;
                }
                t.stock -= 2;
                t.held += 2;
                if (t.physical.size() == depth) {
                    t.settled += 2;
                    t.held -= 2;
                }
                t.physical.append(e.physical);
            }
        }
    } else if (q.action == curtain) {
        t.shutters = (t.shutters & ~(1u << q.who)) | (q.source << q.who);
    } else if (q.action == cast_net) {
        if (!t.held || t.cast || !body.limbs || q.energy <= 1)
            r.status = 1;
        else {
            t.net = person.trail;
            t.cast = 1;
            t.catcher = q.who;
            r.teacher_heat = 1;
        }
    } else if (q.action == serve) {
        if (!t.cast || !t.held || t.catcher != q.who)
            r.status = 1;
        else {
            if (t.net.size() == t.physical.size())
                for (Word j = 0; j < t.physical.size(); ++j) {
                    Word e = t.physical.at(j), a = t.net.at(j);
                    r.bite += Word((a & 3) == braid::pigment(body.glass.places, e & 3) &&
                                   ((a >> 4) & 3) == braid::pigment(body.glass.hues, (e >> 4) & 3));
                    r.bite += Word(((a >> 2) & 3) == braid::pigment(body.glass.places, (e >> 2) & 3) &&
                                   (a >> 6) == braid::pigment(body.glass.hues, e >> 6));
                }
            r.bite = drift::low(r.bite, 65535 - q.energy);
            t.settled += t.held - r.bite;
            t.eaten += r.bite;
            t.held = 0;
            t.cast = 0;
            t.physical = {};
        }
    } else if (q.action == pair_speech) {
        if (!person.trail.size() || q.energy <= q.statement.n)
            r.status = 1;
        else {
            auto prompt = register_waves(m.vocabulary, q.prompt, true);
            auto answer = register_waves(m.vocabulary, q.statement, true);
            Growth g{};
            g.cells = body.cells;
            bool acquired = person.trail.size() > 2
                                ? attend_query(m, prompt, answer, person.trail, body.learning, g)
                                : lesson(m, address(prompt), person.trail, answer, body.learning, g);
            if (!acquired)
                r.status = 1;
            else {
                r.edits = g.contacts;
                r.sprouts = g.revisions;
                r.teacher_heat = q.statement.n;
                r.initial = g.cells.spike;
            }
        }
    } else if (q.action == respond || q.action == receive || q.action == recount) {
        auto prompt = register_waves(m.vocabulary, q.prompt, false);
        Word id = find(m, address(prompt));
        if (id == prompts) {
            auto action = recall(m, prompt, person.trail);
            if (!action.recognized || q.action == receive)
                r.status = 2;
            else if (body.mouth && q.energy > action.reply.size()) {
                r.reply = vocalize(m.vocabulary, phrase(action.reply));
                r.teacher_heat = r.reply.n;
            }
        } else {
            auto &s = m.script[id];
            if (q.action == receive) {
                auto words = register_waves(m.vocabulary, q.statement, false);
                if (q.energy > words.n && words.n) {
                    r.decoded = listen(s, words, [&](Word image) { return unit(s, image); });
                    r.teacher_heat = words.n;
                }
                person.trail = r.decoded;
            } else {
                auto voice = speak(s, person.trail);
                if (body.mouth && q.energy > voice.n) {
                    r.reply = vocalize(m.vocabulary, voice);
                    r.teacher_heat = voice.n;
                }
                if (q.action == recount) {
                    auto heard_q = register_waves(other.memory.vocabulary, q.prompt, false);
                    Word peer = find(other.memory, address(heard_q));
                    if (peer != prompts && r.reply.n && q.other_energy > r.reply.n) {
                        auto line = register_waves(other.memory.vocabulary, r.reply, false);
                        auto &model = other.memory.script[peer];
                        r.decoded = listen(model, line, [&](Word image) { return unit(model, image); });
                        r.pupil_heat = r.reply.n;
                    }
                    other.trail = r.decoded;
                }
            }
        }
    }
    r.seen = person.trail;
    r.other = other.trail;
    r.palette = t.bed.palette;
    r.stock = t.stock;
    r.held = t.held;
    r.settled = t.settled;
    r.eaten = t.eaten;
    r.shutters = t.shutters;
    for (Word i = 0; i < 4; ++i)
        r.grains += t.bed.grains[i];
    for (auto &s : m.script)
        r.prompts_known += bool(s.prompt);
    *output = r;
}
KERNEL void narrative_families(const apprentice::Animal *adults, Word count, Senses pupil, Senses caller, Course c,
                               Word bolus, apprentice::Harvest *food, Count *energy) {
    Word i = tid();
    if (i >= count)
        return;
    auto a = adults[i];
    apprentice::Harvest meal{};
    if (a.ancestor != ~0u) {
        auto r = joint_life({a.program.first, a.program.second}, pupil, caller, c, mix(c.seed ^ mix(i * 4027 + 9127)),
                            c.singles, false);
        meal.swallowed = bolus * (r.upper + r.lower) + 16 * feeds - r.adult_heat;
        meal.escaped = bolus * (2 * feeds - r.upper - r.lower);
        Word patch = (i >> 5) & 127;
        asm("red.global.add.u64 [%0],%1;" ::"l"(energy + patch), "l"(Count(r.adult_heat)) : "memory");
        asm("red.global.add.u64 [%0],%1;" ::"l"(energy + 128 + patch), "l"(Count(r.school_heat)) : "memory");
    }
    food[i] = meal;
}
KERNEL void narrative_choose(const apprentice::Animal *adults, Word n, Senses pupil, Senses caller, Course c,
                             Word bolus, Word *score) {
    Word i = tid();
    if (i >= n)
        return;
    Word net = 0;
    auto a = adults[i];
    if (a.ancestor != ~0u)
        for (Word j = 0; j < 12; ++j) {
            auto r = joint_life({a.program.first, a.program.second}, pupil, caller, c,
                                mix(c.seed ^ mix(j * 13709 + 12341)), c.singles, false);
            net += bolus * (r.upper + r.lower) + 16 * feeds - r.adult_heat;
        }
    score[i] = net;
}
KERNEL void narrative_listeners(Genes genes, Senses pupil, Senses caller, Course c, Word n, Word crossed,
                                Returns *score) {
    Word i = tid();
    if (i < n)
        score[i] = joint_life(genes, pupil, caller, c, mix(c.seed ^ mix(i * 1801 + 8419)), c.singles, crossed);
}
} // namespace axion::life::utterance

#include "chorus.cuh"
