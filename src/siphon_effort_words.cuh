#include "axion/effort_words.hpp"
namespace axion::life::effort {
AX_FN Word feature(const siphon::Vocabulary &old, const siphon::Animal &v, RecollectionGenes law, Count sound) {
    Word i = siphon::identify(old, sound);
    if (!i)
        return 8;
    Word row = siphon::readings(old.forms[i - 1], law);
    if (!row || (row & (row - 1)) || row >= 256)
        return 8;
    Word f = __builtin_ctz(row);
    Word tint = siphon::undo(f & 3, v.genes.anatomy >> (f < 4 ? 0 : 3));
    return (f < 4 ? 4 : 0) + tint;
}
AX_FN Count name_feature(const siphon::Vocabulary &old, const siphon::Animal &v, RecollectionGenes law, Word which) {
    if (which >= 8)
        return 0;
    Word feature = (which < 4 ? 4u : 0u) + siphon::code(which & 3, v.genes.anatomy >> (which < 4 ? 3 : 0));
    return siphon::term(old, law, feature);
}
AX_FN bool learn_gesture(siphon::Vocabulary &v, RecollectionGenes law, Count wave, Word shown) {
    if (!wave || !shown || (shown & (shown - 1)))
        return false;
    Word id = siphon::identify(v, wave);
    if (!id) {
        if (v.count == siphon::external_forms)
            return false;
        id = ++v.count;
        v.forms[id - 1].wave = wave;
    }
    auto &f = v.forms[id - 1];
    Word mask = 127u << 12;
    auto q = grounding::plastic({f.faint, f.strong}, shown << 12, law.low & 65535);
    f.faint = (f.faint & ~mask) | (q.low & mask) | 0xc0000000u;
    f.strong = (f.strong & ~mask) | (q.high & mask);
    return true;
}
AX_FN Word gesture(const siphon::Vocabulary &v, RecollectionGenes law, Count wave) {
    Word id = siphon::identify(v, wave);
    if (!id || !(v.forms[id - 1].faint & 0x40000000u))
        return unrecognized;
    auto f = v.forms[id - 1];
    Word r = (grounding::release({f.faint, f.strong}, law.high & 15) >> 12) & 127;
    return r && !(r & (r - 1)) ? Word(__builtin_ctz(r)) : unrecognized;
}
AX_FN void omit(Bundle &b, Word f) {
    Word &uses = f < 4 ? b.food : b.work;
    uses &= ~(255u << (8 * (f & 3)));
    Word index = f < 4 ? b.terminal & 3 : b.terminal >> 2;
    if (index == (f & 3)) {
        Word next = uses ? Word(__builtin_ctz(uses)) / 8 : 0;
        b.terminal = f < 4 ? (b.terminal & 12) | next : (b.terminal & 3) | (next << 2);
    }
}
AX_FN Reply discuss(const Frozen &values, const siphon::Vocabulary &old, const siphon::Vocabulary &space,
                    const siphon::Animal &body, RecollectionGenes law, Topic &topic, const Speech &s, Word ray,
                    Word head) {
    Reply r{};
    if (!s.n)
        return r;
    r.op = gesture(space, law, s.sounds[0]);
    if (r.op == unrecognized || r.op == alternation)
        return r;
    if (r.op == fold || r.op == material)
        return r;
    if (r.op == compare && s.n > 1) {
        Topic heard{};
        Word which = 0, count[2]{};
        for (Word i = 1; i < s.n; ++i) {
            if (gesture(space, law, s.sounds[i]) == alternation) {
                if (which || !count[0])
                    return r;
                which = 1;
                continue;
            }
            Word f = feature(old, body, law, s.sounds[i]);
            if (f >= 8)
                return r;
            if (f < 4)
                nourishment(heard.offer[which], f);
            else
                exertion(heard.offer[which], f - 4);
            ++count[which];
        }
        if (!count[1])
            return r;
        heard.ready = 1;
        topic = heard;
    } else if (r.op == remove) {
        if (s.n != 2 || !topic.ready)
            return r;
        Word f = feature(old, body, law, s.sounds[1]);
        if (f >= 8)
            return r;
        omit(topic.offer[0], f);
        omit(topic.offer[1], f);
    } else if (s.n != 1)
        return r;
    if (!topic.ready)
        return r;
    r.ready = 1;
    r.value[0] = potential(values, topic.offer[0]);
    r.value[1] = potential(values, topic.offer[1]);
    if (r.value[0] == r.value[1]) {
        topic.chosen = 2;
        return r;
    }
    Word elected = Word(r.value[1] > r.value[0]);
    topic.chosen = elected;
    r.choice = elected;
    if (r.op == explain) {
        int full = r.value[elected] - r.value[elected ^ 1], top = 0;
        Word contact = 8;
        bool tied = false;
        for (Word f = 0; f < 8; ++f) {
            auto a = topic.offer[elected], b = topic.offer[elected ^ 1];
            omit(a, f);
            omit(b, f);
            int difference = potential(values, a) - potential(values, b), impact = full - difference;
            if (impact > top) {
                top = impact;
                contact = f;
                tied = false;
                r.decisive = difference < 0;
            } else if (impact == top && top)
                tied = true;
        }
        if (tied || contact == 8)
            return r;
        r.feature = contact;
        r.word = name_feature(old, body, law, contact);
    } else {
        Word dir = (ray + 2 * elected - head) & 3;
        r.word = siphon::term(space, law, 8 + dir);
    }
    r.accepted = !!r.word;
    return r;
}
AX_FN Board ford(Word seed) {
    Board b{};
    auto g = grove(seed);
    b.seed = seed;
    b.fruit = g.fruits;
    b.drag = g.strata;
    b.ray = (mix(seed + 769) >> 8) & 3;
    b.head = (seed >> 5) & 3;
    auto sample = job(g, new_route);
    b.pigments = sample.slat.pigments;
    b.pose = sample.slat.pose;
    b.handed = sample.slat.handed;
    b.handle = sample.handles;
    for (Word &q : b.capital)
        q = 512;
    for (Word i = 0; i < 2; ++i) {
        Word k = mix(seed + 1213 + i * 2003);
        nourishment(b.lanes[i], k & 3, 1 + ((k >> 6) & 1));
        exertion(b.lanes[i], (k >> 10) & 3, 1 + ((k >> 14) & 3));
    }
    return b;
}
AX_FN Word material_count(Bundle p, Word f) { return ((f < 4 ? p.food : p.work) >> (8 * (f & 3))) & 255; }
AX_FN Word demonstration(Board &b, Word action, Word who, Word sensory) {
    Word previous = b.focus, opened = b.opened, exam = b.examined, trigger = b.trigger;
    auto a = b.lanes[0], c = b.lanes[1];
    Word pose = b.pose, handle = b.handle;
    b.shown = 0;
    if (action == alternation)
        b.focus ^= 1;
    else if (action == compare)
        b.opened ^= 3;
    else if (action == explain) {
        b.contrast[0] = b.lanes[0];
        b.contrast[1] = b.lanes[1];
        omit(b.contrast[0], who);
        omit(b.contrast[1], who);
        b.examined ^= 1;
    } else if (action == remove) {
        omit(b.lanes[0], who);
        omit(b.lanes[1], who);
    } else if (action == enact)
        b.trigger ^= 1;
    else if (action == material)
        b.handle = rotate_values(b.handle, 1);
    else if (action == fold) {
        Slat s{b.pigments, b.pose, b.handed};
        roll(s, who & 1);
        b.pose = s.pose;
    }
    if (sensory & 16) {
        b.shown = Word(previous != b.focus) + 2 * Word(opened != b.opened) + 4 * Word(exam != b.examined) +
                  8 * Word(a.food != b.lanes[0].food || c.food != b.lanes[1].food || a.work != b.lanes[0].work ||
                           c.work != b.lanes[1].work) +
                  16 * Word(trigger != b.trigger) + 32 * Word(handle != b.handle) + 64 * Word(pose != b.pose);
    }
    return b.shown;
}
AX_FN Bite perform(Board &b, Mind &m, Word selected, Word budget, Word organs, Word learn) {
    Bite action{};
    if (selected >= 2 || !(organs & 4))
        return action;
    auto actual = b.lanes[selected];
    Word reserve = budget;
    for (Word i = 0; i < 4; ++i) {
        Word fee = resistance(b.drag, i), number = material_count(actual, 4 + i);
        for (Word j = 0; j < number; ++j) {
            if (reserve <= fee) {
                b.work += action.paid;
                return action;
            }
            reserve -= fee;
            action.paid += fee;
            ++action.steps;
            strain(m, i, fee, learn);
        }
    }
    action.reached = 1;
    for (Word i = 0; i < 4; ++i) {
        Word count = material_count(actual, i);
        for (Word j = 0; j < count; ++j) {
            Word amount = (organs & 2) ? drink(reserve, b.capital[i], flesh(b.fruit, i)) : 0;
            action.food += amount;
            taste(m, i, amount, learn);
        }
    }
    b.work += action.paid;
    b.consumed += action.food;
    return action;
}
AX_FN Bite lessons(Board &b, Mind &m, Word kind, Word reserve, Word organs, Word mode) {
    Bite a{};
    for (Word j = 0; j < 4; ++j) {
        Word cue = (j + b.round) & 3;
        if (!kind && (organs & 2)) {
            Word dose = drink(reserve, b.capital[cue], flesh(b.fruit, cue));
            a.food += dose;
            taste(m, cue, dose, mode);
        } else if (kind && (organs & 4)) {
            Word fee = resistance(b.drag, cue);
            if (reserve <= fee)
                continue;
            reserve -= fee;
            a.paid += fee;
            ++a.steps;
            strain(m, cue, fee, mode);
        }
    }
    ++b.round;
    b.work += a.paid;
    b.consumed += a.food;
    return a;
}
struct SeenSurface {
    Word face, handles;
};
AX_FN SeenSurface sight(const Board &b, Word organs) {
    return {(organs & 16) ? press::item(b.pigments, b.pose) : 4u, b.handle & 15};
}
AX_FN Reply shape_reply(const Mind &m, SeenSurface b, const siphon::Vocabulary &old, const siphon::Vocabulary &space,
                        const siphon::Animal &speaker, RecollectionGenes law, const Speech &sentence) {
    Reply r{};
    if (!sentence.n || gesture(space, law, sentence.sounds[0]) != material)
        return r;
    r.op = material;
    Word face = b.face;
    if (face >= 4)
        return r;
    r.ready = 1;
    if (!(sentence.n & 1))
        return r;
    for (Word j = 1; j < sentence.n; j += 2) {
        if (gesture(space, law, sentence.sounds[j]) != fold)
            return r;
        Word handle = feature(old, speaker, law, sentence.sounds[j + 1]);
        if (handle < 4 || handle >= 8)
            return r;
        Word a = press::item(b.handles, 0) == handle - 4 ? 0u : press::item(b.handles, 1) == handle - 4 ? 1u : 2u;
        if (a >= 2)
            return r;
        face = successor(futures(m), face, a);
        if (face >= 4)
            return r;
    }
    r.feature = 4 + face;
    r.word = name_feature(old, speaker, law, r.feature);
    r.accepted = !!r.word;
    return r;
}
AX_FN Bite dry_lessons(Mind &m, Board &b, Word available, Word organs, Word intervention) {
    Bite r{};
    if ((organs & 20) != 20)
        return r;
    for (Word j = 0; j < 16; ++j) {
        Slat witness{b.pigments, (j + b.round) & 3, b.handed};
        Word action = (j >> 2) & 1, fee = resistance(b.drag, press::item(b.handle, action));
        if (available <= r.paid + fee)
            break;
        Word initial = outer(witness);
        roll(witness, action);
        Word after = outer(witness);
        change(m, initial, action, intervention == wrong_model ? (after + 1) & 3 : after,
               (intervention == erased_model || (intervention == passive_only && action)) ? 0u : Word(learning));
        ++r.steps;
        r.paid += fee;
    }
    b.work += r.paid;
    ++b.round;
    return r;
}
AX_FN Bite handle_slat(Mind &m, Board &b, Word a, Word budget, Word body) {
    Bite r{};
    Word h = press::item(b.handle, a), fee = resistance(b.drag, h);
    if (!(body & 4) || budget <= fee)
        return r;
    Slat s{b.pigments, b.pose, b.handed};
    Word first = outer(s);
    roll(s, a);
    b.pose = s.pose;
    strain(m, h, fee, learning);
    if (body & 16)
        change(m, first, a, outer(s), learning);
    r.paid = fee;
    r.steps = 1;
    b.work += fee;
    return r;
}

} // namespace axion::life::effort
KERNEL void effort_room(effort::Board *b, effort::Saved *m, effort::Topic *topic, Word action, Word p, Word q,
                        effort::Bundle lane, Word budget, Word body, effort::Bite *out) {
    if (tid())
        return;
    *out = {};
    if (action == 0) {
        *b = effort::ford(p);
        *topic = {};
        // Crossing to another ford starts a new physical episode. Learned
        // prices and transformations remain with this resident.
        m->mind.trace = 0;
        m->mind.account = 0;
        m->mind.experience &= ~0xf00u;
    } else if (action == 1 || action == 2)
        *out = effort::lessons(*b, m->mind, action - 1, budget, body, effort::learning);
    else if (action == 3) {
        b->fruit = effort::rotate_values(b->fruit, p);
        b->drag = effort::rotate_values(b->drag, q);
        b->shown = 0;
    } else if (action == 4) {
        b->head = (b->head + p) & 3;
        b->shown = 0;
    } else if (action == 5) {
        b->lanes[p] = lane;
        b->shown = 0;
    } else if (action == 6)
        effort::demonstration(*b, p, q, body);
    else if (action == 7)
        *out = effort::perform(*b, m->mind, topic->chosen, budget, body, effort::learning);
    else if (action == 8)
        *out = effort::dry_lessons(m->mind, *b, budget, body, effort::new_route);
    else if (action == 9)
        *out = effort::handle_slat(m->mind, *b, p, budget, body);
}
KERNEL void effort_teach(siphon::Vocabulary *v, RecollectionGenes law, const effort::Board *b, Count word, Word *yes) {
    if (!tid())
        *yes = effort::learn_gesture(*v, law, word, b->shown);
}
KERNEL void effort_speak(const effort::Saved *m, const siphon::Pair *b, const siphon::Vocabulary *old,
                         const siphon::Vocabulary *space, RecollectionGenes law, effort::Topic *topic,
                         effort::Speech spoken, const effort::Board *room, Word organs, effort::Reply *out) {
    if (tid())
        return;
    Word op = spoken.n ? effort::gesture(*space, law, spoken.sounds[0]) : effort::unrecognized;
    *out =
        op == effort::material
            ? effort::shape_reply(m->mind, effort::sight(*room, organs), *old, *space, b->b, law, spoken)
            : effort::discuss(effort::freeze(m->mind), *old, *space, b->b, law, *topic, spoken, room->ray, room->head);
}
KERNEL void effort_dialogues(const effort::Saved *genome, const siphon::Pair *residents, const siphon::Vocabulary *old,
                             const siphon::Vocabulary *space, RecollectionGenes law, const effort::Dialogue *prompts,
                             effort::Transcript *replies, Word n, Word body) {
    Word i = tid();
    if (i >= n)
        return;
    const auto &q = prompts[i];
    auto b = effort::ford(q.seed);
    auto g = effort::grove(q.seed);
    effort::Topic topic{};
    effort::Mind m{genome->mind.genes};
    effort::Transcript r{};
    Word start = 192, food = 0, spent = 0;
    effort::calibrate(m, g, q.mode & 15, start, food, spent);
    b.fruit = g.next_fruit;
    b.drag = g.next_strata;
    b.lanes[0] = q.real[0];
    b.lanes[1] = q.real[1];
    auto image = effort::freeze(m);
    auto say = [&](const effort::Speech &s) {
        return effort::discuss(image, *old, *space, residents->b, law, topic, s, b.ray, b.head);
    };
    r.replies[0] = say(q.first);
    if (q.mode & 16)
        topic = {};
    r.replies[1] = say(q.why);
    b.head = (b.head + q.turn) & 3;
    r.replies[2] = say(q.edit);
    r.replies[3] = say(q.repeat);
    r.action = effort::perform(b, m, topic.chosen, 192, body, effort::learning);
    r.sampled = food;
    r.practice = spent;
    r.face = b.head;
    replies[i] = r;
}

KERNEL void effort_surface_questions(const effort::Saved *m, const siphon::Pair *body, const siphon::Vocabulary *old,
                                     const siphon::Vocabulary *space, RecollectionGenes law,
                                     const effort::SurfaceQuestion *questions, effort::SurfaceAnswer *answers, Word n,
                                     Word organs) {
    Word i = tid();
    if (i >= n)
        return;
    const auto &q = questions[i];
    effort::Mind memory{m->mind.genes};
    auto board = effort::ford(q.seed);
    effort::SurfaceAnswer result{};
    auto demo = effort::dry_lessons(memory, board, 192, organs, q.mode);
    result.demonstrations = demo.steps;
    result.paid = demo.paid;
    result.before = press::item(board.pigments, board.pose);
    result.first = effort::shape_reply(memory, effort::sight(board, organs), *old, *space, body->b, law, q.first);
    if (q.turns < 2) {
        auto touch = effort::handle_slat(memory, board, q.turns, 192 - demo.paid, organs);
        result.paid += touch.paid;
    }
    result.after = press::item(board.pigments, board.pose);
    result.second = effort::shape_reply(memory, effort::sight(board, organs), *old, *space, body->b, law, q.second);
    answers[i] = result;
}
