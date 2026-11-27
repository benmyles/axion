#include "axion/hollow_words.hpp"
namespace axion::life::hollow {
AX_FN void carry_forms(siphon::Vocabulary &v, const siphon::Vocabulary &old, const RecollectionGenes &wiring) {
    if (v.count)
        return;
    for (Word i = 0; i < old.count; ++i) {
        Word fired = siphon::readings(old.forms[i], wiring);
        if (fired && fired < 256 && !(fired & (fired - 1)))
            v.forms[v.count++] = old.forms[i];
    }
}
AX_FN siphon::Report converse(const Saved &memory, const clay::Mind &eye, const siphon::Animal &voice,
                              const siphon::Vocabulary &places, const siphon::Vocabulary &older,
                              RecollectionGenes wiring, siphon::Sentence prompt, Topic &topic, Word clock, Word key) {
    auto r = siphon::interpret(places, wiring, voice, prompt);
    if (!r.recognized) {
        press::Topic object{topic.hue, topic.bath};
        auto material = press::describe(eye, voice, older, wiring, prompt, 15, object);
        if (material.accepted)
            topic = {object.tool, object.bath};
        return material;
    }
    if (r.role != 2)
        return r;
    Word hue = r.appearance, bath = r.property;
    if (bath < 4) {
        Word mask = best_tools(eye, bath);
        if (!mask || (mask & (mask - 1)))
            return r;
        Word strongest = __builtin_ctz(mask);
        if (hue < 4 && hue != strongest)
            return r;
        hue = strongest;
        if (!(press::predictions(eye, hue) & press::values(eye, bath)))
            return r;
    }
    if (hue == 4)
        hue = topic.hue;
    if (hue >= 4)
        return r;
    auto fired = recall(memory.mind, {memory.marks, 1, memory.slot_limit}, 1u << (4 * implement + hue), clock, key);
    if (fired.compass >= 4 || !fired.range)
        return r;
    r.candidates = fired.bearings;
    r.spoken = fired.compass;
    siphon::complete(r, places, wiring, voice, r.spoken);
    if (r.accepted)
        topic = {hue, bath};
    return r;
}
AX_FN Word bearing(int dx, int dy, Word heading) {
    if (!dx && !dy)
        return 4;
    Word ray = dx ? (dx > 0 ? 1u : 3u) : (dy > 0 ? 2u : 0u);
    return (ray - heading) & 3;
}
AX_FN void display(siphon::Room &stage, const Path &p, Word bath, Word hue, Word side, Word body) {
    stage.exposed = bool(body & 16);
    if (!stage.exposed)
        return;
    // A presenter places a glazed sample one stride away. The retinal
    // direction is computed from both physical positions and the body's head.
    Word dir = (p.head + side) & 3;
    Site shown{short(p.x + int(dir == 1) - int(dir == 3)), short(p.y + int(dir == 2) - int(dir == 0)), Byte(hue),
               Byte(bath)};
    stage.site.membrane = shown.relief;
    stage.last.percept = shown.tint;
    stage.context = bearing(shown.x - p.x, shown.y - p.y, p.head);
}
AX_FN void passive(Marsh &bay, Saved &memory, Word physics) {
    auto &r = bay.ground.sites[5];
    Word k = bay.ground.seed;
    lead(bay.body, memory.mind, r.x + ((k & 64) ? 4 : -4), r.y + ((k & 128) ? 3 : -3), k + 759, physics);
    bay.body.x = r.x;
    bay.body.y = r.y;
}
AX_FN Contact operate(Marsh &bay, Saved &memory, clay::Mind &eye, siphon::Room &scene, Topic &speaker, Word cmd,
                      Word param, Word other, Word side, Word body, Word energy) {
    constexpr Word physics = changing | anchoring | dead_reckoning;
    auto &m = memory.mind;
    auto &g = bay.ground;
    auto &p = bay.body;
    Slots cells{memory.marks, 1, memory.slot_limit};
    Contact r{};
    Word before = p.steps;
    scene.exposed = 0;
    if (cmd == arrive) {
        bay = {};
        g = clearing(param);
        bay.pieces = place_pieces(g);
        bay.gate = g.old_gate;
        p = {int((param >> 20) & 3) - 2, int((param >> 24) & 3) - 1, (param >> 16) & 3, 0};
        auto genes = m.genes;
        m = {genes};
        for (Word &v : memory.marks)
            v = 0;
        speaker = {};
        before = 0;
    } else if (cmd == survey && (body & 20) == 20) {
        // Each reached plot can be retained even if later travel is unaffordable.
        while (bay.seen < 6) {
            auto next = p;
            auto next_memory = memory;
            survey_sites(next_memory.mind, {next_memory.marks, 1, memory.slot_limit}, g, next, physics, bay.seen,
                         bay.seen + 1, &bay.pieces);
            if (next.steps - before >= energy)
                break;
            p = next;
            memory = next_memory;
            ++bay.seen;
        }
        r.work = p.steps - before;
    } else if (cmd == hear && (body & 20) == 20) {
        auto next = p;
        auto next_m = m;
        lead(next, next_m, 10 + int((g.seed >> 5) & 3), -10 - int((g.seed >> 14) & 3), g.seed + 313, physics);
        if (next.steps - before + 12 < energy) {
            p = next;
            m = next_m;
            hear_sisters(eye, g.seed);
            r.work = p.steps - before + 12;
        }
    } else if (cmd == sip && (body & 4)) {
        auto next = p;
        auto next_m = m;
        lead(next, next_m, g.sites[4].x, g.sites[4].y, g.seed + 511, physics);
        if (next.steps - before + 1 >= energy) {
            r.stock = bay.stock;
            r.held = bay.held;
            return r;
        }
        p = next;
        m = next_m;
        r.work = p.steps - before;
        Word key = mix(g.seed + bay.phase * 173);
        for (Word n = 0; n < 12 && energy + r.food > r.work + 1 && bay.stock; ++n) {
            Word tone = (n * (1 + 2 * ((key >> 8) & 1)) + key) & 3;
            Word offered = press::flow(tone, bay.gate, 32, bay.stock);
            Word available = (body & 2) ? 65535 - energy - r.food + r.work + 1 : 0;
            Word eaten = offered < available ? offered : available;
            press::taste(eye, g.bath, tone, eaten != 0);
            r.food += eaten;
            bay.stock -= offered;
            bay.loose += offered - eaten;
            ++r.work;
        }
    } else if (cmd == wheel)
        bay.gate = (bay.gate + param) & 3;
    else if (cmd == raft && (body & 4)) {
        auto next = bay;
        auto brain = memory;
        passive(next, brain, physics);
        if (next.body.steps - before < energy) {
            bay = next;
            memory = brain;
            r.work = p.steps - before;
        }
    } else if (cmd == recognize && (body & 16)) {
        Word texture = 63;
        for (Word j = 0; j < 6; ++j)
            if (p.x == g.sites[j].x && p.y == g.sites[j].y) {
                texture = g.sites[j].relief;
                r.corrected = orient(m, cells, texture, physics);
                break;
            }
        for (Word seen = within_reach(bay.pieces, p); seen; seen &= seen - 1)
            observe(m, cells, 4 * implement + __builtin_ctz(seen), texture, false, p.steps, bay.phase, physics);
    } else if (cmd == departure && (body & 4)) {
        auto next = p;
        auto tissue = m;
        lead(next, tissue, int((g.seed >> 1) % 5) - 2, int((g.seed >> 11) % 5) - 2, mix(g.seed + 913), physics);
        if (next.steps - before < energy) {
            p = next;
            m = tissue;
            r.work = p.steps - before;
        }
    } else if (cmd == face && (body & 4) && energy > 1) {
        motion(p, m, (p.head + param) & 3, 0, physics);
        r.work = 1;
    } else if (cmd == lead_step && (body & 4) && energy > other) {
        motion(p, m, (p.head + param) & 3, other, physics);
        r.work = other;
    } else if (cmd == exhibit) {
        display(scene, p, param, other, side, body);
        if (scene.exposed) {
            r.hue = scene.last.percept;
            r.view = scene.context;
        }
    } else if (cmd == grasp && (body & 4) && energy > 1 && bay.held == 4 &&
               (within_reach(bay.pieces, p) & (1u << param))) {
        bay.held = param;
        bay.pieces.loose &= ~(1u << param);
        r.work = 1;
    } else if (cmd == put_down && (body & 4) && energy > 1 && bay.held < 4) {
        lay_down(bay.pieces, bay.held, p);
        r.work = 1;
    } else if (cmd == take_home && (body & 6) == 6 && energy > 1) {
        Trip t{};
        Word allowance = energy - 1 < 48 ? energy - 1 : 48;
        search(t, m, cells, eye, g, bay.pieces, p, bay.gate, bay.stock, bay.held, allowance, open_route, nullptr);
        r.returned = t.delivered;
        r.work = t.work;
        Word capacity = 65535 - energy + r.work;
        r.food = t.food < capacity ? t.food : capacity;
        bay.loose += t.food - r.food;
    }
    ++bay.phase;
    bay.consumed += r.food;
    r.walked = p.steps - before;
    r.held = bay.held;
    r.stock = bay.stock;
    return r;
}
} // namespace axion::life::hollow
KERNEL void hollow_move(hollow::Marsh *bay, hollow::Saved *m, clay::Mind *p, siphon::Room *scene, hollow::Topic *topic,
                        Word cmd, Word arg, Word other, Word side, Word body, Word budget, hollow::Contact *result) {
    if (!tid())
        *result = hollow::operate(*bay, *m, *p, *scene, *topic, cmd, arg, other, side, body, budget);
}
KERNEL void hollow_words(siphon::Vocabulary *map, const siphon::Vocabulary *old, const siphon::Pair *actors,
                         RecollectionGenes law, const siphon::Room *room, siphon::Sentence speech, Word exchange,
                         siphon::Report *result) {
    if (tid())
        return;
    hollow::carry_forms(*map, *old, law);
    *result = {};
    result->accepted = siphon::caption(*map, actors->b, law, *room, speech, exchange);
    result->forms = map->count;
}
KERNEL void hollow_say(const hollow::Saved *m, const clay::Mind *p, const siphon::Pair *actors,
                       const siphon::Vocabulary *map, const siphon::Vocabulary *old, RecollectionGenes law,
                       siphon::Sentence speech, hollow::Topic *topic, const hollow::Marsh *bay, siphon::Report *out) {
    if (!tid())
        *out = hollow::converse(*m, *p, actors->b, *map, *old, law, speech, *topic, bay->body.steps,
                                mix(bay->ground.seed + bay->phase));
}
KERNEL void hollow_questions(const hollow::Saved *cortex, const clay::Mind *optics, const siphon::Pair *actors,
                             const siphon::Vocabulary *words, const siphon::Vocabulary *older, RecollectionGenes wiring,
                             const hollow::Question *questions, hollow::Answer *answers, Word n, Word organs) {
    Word i = tid();
    if (i >= n)
        return;
    using namespace hollow;
    const auto &q = questions[i];
    Saved memory{};
    memory.mind.genes = cortex->mind.genes;
    memory.slot_limit = cortex->slot_limit;
    auto g = clearing(q.seed);
    Path p{};
    hollow::Topic topic{};
    Answer out{};
    clay::Mind eye{};
    press::express(eye, press::germ(*optics));
    Word mode = q.conditions & 15;
    if ((organs & 22) == 22) {
        if (mode == one_cell)
            memory.slot_limit = 1;
        course(memory.mind, {memory.marks, 1, memory.slot_limit}, eye, g, p, mode, out.cost, out.corrected);
        auto site = g.sites[q.hue];
        int x = site.x + int(q.distance) * (int(q.ray == 1) - int(q.ray == 3)),
            y = site.y + int(q.distance) * (int(q.ray == 2) - int(q.ray == 0));
        Word input = mode == no_motion ? 0u : mode == skewed_motion ? dead_reckoning | distorted : dead_reckoning;
        lead(p, memory.mind, x, y, q.seed + 1437, input);
        motion(p, memory.mind, q.facing, 0, input);
        auto say = [&](siphon::Sentence words_in) {
            return converse(memory, eye, actors->b, *words, *older, wiring, words_in, topic, p.steps,
                            mix(q.seed + q.facing));
        };
        out.spoken[0] = say(q.first);
        if (q.conditions & 16)
            topic = {};
        motion(p, memory.mind, (p.head + q.rotation) & 3, 0, input);
        out.spoken[1] = say(q.follow);
        out.spoken[2] = say(q.effect);
        out.memories = memory.mind.used;
        out.cost = p.steps;
    }
    answers[i] = out;
}
