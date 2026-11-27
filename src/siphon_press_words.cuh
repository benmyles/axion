// Retrieval receives only the pupil, the word forms and the public offer.
// The pressure chamber, its hidden slits and the evaluation answers are absent.
namespace axion::life::press {
AX_FN Word strongest(const Mind &m, Word varying, Word given, Word offered, Word tone = 4) {
    Word mask = 0, best = 0;
    for (Word i = 0; i < 4; ++i) {
        Word cue = varying ? i : given, bath = varying ? given : i;
        if (!(offered & (1u << cue)))
            continue;
        if (tone < 4 && !(predictions(m, cue) & values(m, bath) & (1u << tone)))
            continue;
        Word current = drive(m, cue, bath);
        if (!mask || current > best) {
            best = current;
            mask = 1u << i;
        } else if (current == best)
            mask |= 1u << i;
    }
    return mask;
}
AX_FN siphon::Report describe(const Mind &m, const siphon::Animal &body, const siphon::Vocabulary &words,
                              RecollectionGenes g, siphon::Sentence question, Word offered, Topic &topic) {
    auto r = siphon::interpret(words, g, body, question);
    if (!r.recognized)
        return r;
    Word cue = r.appearance, bath = r.property, tone = r.source, options = 0;
    if (r.role == 1 && bath < 4)
        options = strongest(m, 1, bath, offered, tone);
    else if (r.role == 0 && cue < 4)
        options = strongest(m, 0, cue, offered, tone);
    else if (r.role == 2) {
        if (cue == 4 && bath == 4)
            cue = topic.tool;
        if (cue < 4 && !(offered & (1u << cue)))
            return r;
        if (cue < 4 || bath < 4)
            options = (cue < 4 ? predictions(m, cue) : 15u) & (bath < 4 ? values(m, bath) : 15u);
    }
    r.candidates = options;
    if (__builtin_popcount(options) != 1)
        return r;
    Word answer = __builtin_ctz(options);
    siphon::complete(r, words, g, body, answer);
    r.spoken = answer;
    if (r.accepted) {
        topic.tool = r.role == 1 ? answer : cue;
        topic.bath = r.role == 0 ? answer : bath;
    }
    return r;
}
} // namespace axion::life::press
KERNEL void press_speak(const clay::Mind *m, const siphon::Pair *animals, const siphon::Vocabulary *words,
                        RecollectionGenes law, siphon::Sentence question, Word offered, press::Topic *topic,
                        siphon::Report *out) {
    if (!tid())
        *out = press::describe(*m, animals->b, *words, law, question, offered, *topic);
}
AX_FN press::Receipt wheel_event(press::Wheel *w, clay::Mind *m, siphon::Room *scene, press::Topic *topic, Word command,
                                 Word param, Word second, Word mask, Word a, Word b, Word body, Word plastic) {
    press::Receipt r{};
    *topic = {};
    scene->exposed = 0;
    if (command == press::set_wheel) {
        *w = {};
        *scene = {};
        w->seed = param;
        w->gates = press::deck(mix(param + 9137));
        for (Word i = 0; i < 4; ++i)
            w->stock[i] = 512;
        press::Record fabrication{};
        press::forge(w->tools, w->siblings, param, fabrication);
    } else if (command == press::turn_wheel) {
        Word next = 0;
        for (Word i = 0; i < 4; ++i)
            next |= ((press::item(w->gates, i) + param) & 3) << (2 * i);
        w->gates = next;
    } else if (command == press::watch && (body & 16) && (body & 32)) {
        for (Word j = 0; j < 12 && r.others + 1 < a; ++j) {
            auto &p = w->siblings[j & 3];
            Word hue = press::appearance(p.body, p.face), tone = press::resonance(p.plate, p.body, p.face);
            if (plastic)
                press::see(*m, hue, tone, true);
            ++r.others;
            ++r.witnesses;
        }
    } else if (command == press::sample_water && (body & 4)) {
        for (Word cycle = 0; cycle < 3; ++cycle) {
            Word order = mix(w->seed + 991 * cycle + w->clock * 173);
            for (Word j = 0; j < 16; ++j) {
                Word turn = (j * ((order >> 4) | 1u) + order) & 15, bath = turn >> 2, tone = turn & 3;
                if (b + r.meal <= r.own + 1 || !w->stock[bath])
                    continue;
                Word can_eat = (body & 2) ? 65535u - (b + r.meal - r.own - 1) : 0;
                Word food = press::flow(tone, press::item(w->gates, bath), 32, w->stock[bath]);
                Word paid = food < can_eat ? food : can_eat;
                if (plastic)
                    press::taste(*m, bath, tone, paid != 0);
                w->stock[bath] -= food;
                w->loose += food - paid;
                w->consumed += paid;
                r.meal += paid;
                ++r.own;
                ++r.tastes;
            }
        }
    } else if (command == press::demonstrate || command == press::work) {
        bool self = command == press::work;
        Word who = second, bath = param;
        if (self) {
            Word candidates = press::strongest(*m, 1, bath, mask);
            who = siphon::elect(candidates ? candidates : mask, mix(w->seed + w->clock * 917));
        }
        if (who < 4 && (mask & (1u << who)) && (self ? ((body & 4) && b > 1) : ((body & 32) && a > 1))) {
            auto &p = w->tools[who];
            Word tone = press::resonance(p.plate, p.body, p.face), cue = press::appearance(p.body, p.face);
            bool sight = body & 16, loaded = w->stock[bath] != 0;
            Word food = press::flow(tone, press::item(w->gates, bath), 64, w->stock[bath]);
            Word capacity = (self ? body & 2 : body & 1) ? 65535u - (self ? b : a) + 1 : 0;
            Word paid = food < capacity ? food : capacity;
            w->stock[bath] -= food;
            w->loose += food - paid;
            w->consumed += paid;
            if (self) {
                r.own = 1;
                r.meal = paid;
            } else {
                r.others = 1;
                r.share = paid;
            }
            if (plastic && sight)
                press::see(*m, cue, tone, !self);
            if (plastic && loaded && (self || sight))
                press::taste(*m, bath, tone, paid != 0);
            if (sight || self) {
                r.tone = tone;
                r.bath = bath;
            }
            if (sight) {
                r.cue = cue;
                scene->exposed = 1;
                scene->site.membrane = bath;
                scene->last.percept = cue;
                scene->context = tone;
            }
        }
    }
    ++w->clock;
    for (Word i = 0; i < 4; ++i)
        r.reserve += w->stock[i];
    r.spilled = w->loose;
    r.consumed = w->consumed;
    return r;
}
KERNEL void press_wheel(press::Wheel *w, clay::Mind *m, siphon::Room *scene, press::Topic *topic, Word command,
                        Word param, Word second, Word mask, Word a, Word b, Word body, Word plastic,
                        press::Receipt *out) {
    if (!tid())
        *out = wheel_event(w, m, scene, topic, command, param, second, mask, a, b, body, plastic);
}
KERNEL void press_questions(const clay::Mind *resident, const siphon::Pair *pair, const siphon::Vocabulary *words,
                            RecollectionGenes law, const press::OralTest *tests, press::OralResponse *answers, Word n,
                            Word donor, Word pupil, Word body) {
    Word i = tid();
    if (i >= n)
        return;
    const auto &q = tests[i];
    auto m = *resident;
    press::Wheel wheel{};
    press::Topic topic{};
    siphon::Room scene{};
    press::OralResponse r{};
    Word a = donor, b = pupil;
    auto phase = [&](Word command, Word param) {
        auto v = wheel_event(&wheel, &m, &scene, &topic, command, param, 0, q.offered, a, b, body,
                             q.exposure != press::Listening::fixed);
        a = a - v.others + v.share;
        b = b - v.own + v.meal;
        r.demonstrations += v.witnesses;
        r.tastings += v.tastes;
        r.paid += v.own + v.others;
        r.food += v.meal + v.share;
    };
    phase(press::set_wheel, q.seed);
    if (q.exposure != press::Listening::no_sight)
        phase(press::watch, 0);
    if (q.exposure != press::Listening::no_taste)
        phase(press::sample_water, 0);
    phase(press::turn_wheel, q.rotate);
    if (q.exposure != press::Listening::no_taste && q.exposure != press::Listening::prior)
        phase(press::sample_water, 0);
    r.first = press::describe(m, pair->b, *words, law, q.first, q.offered, topic);
    if (q.exposure == press::Listening::no_topic)
        topic = {};
    if (q.follow.n)
        r.follow = press::describe(m, pair->b, *words, law, q.follow, q.offered, topic);
    answers[i] = r;
}
