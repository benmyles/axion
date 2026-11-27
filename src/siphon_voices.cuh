#include "axion/ceramic_words.hpp"
namespace axion::life::clay {
AX_FN Hearing handle(siphon::Animal &a, Mind &m, const Instrument &kit, Word command, Word limit, Word seed,
                     Word excluded, Word budget, Word visible, bool body, bool eyes, Word already = 0,
                     const Charts *charts = nullptr) {
    Hearing response{};
    response.previous = active(m);
    if (command == 0) {
        swap_bank(m, a, 0);
        m.charge = (m.charge & 0xffff0000u) | 0x8888;
        m.banks &= ~4096u;
        if (visible && eyes)
            orient(m, a, kit.glaze[1], seed, false);
    } else if ((command == 1 || command == 3) && body && eyes) {
        Word used = already;
        for (Word j = 0; j < limit && response.strokes + 1 < budget; ++j) {
            if (command == 3 && (resolved(m) < 4 || count(m) < 2))
                break;
            Word at = pluck(m, a, kit.objects[1], used, excluded, mix(seed + j * 799), false, charts);
            if (at == 8)
                break;
            used |= 1u << at;
            Word e = groove(kit.objects[1], at);
            ++response.strokes;
            response.mask |= 1u << at;
            response.acoustic = (response.acoustic << 3) | ((e & 3) + 1);
            response.non_target += ((e >> 2) & 3) != excluded;
            listen(m, a, e, charts);
            if (command == 3)
                orient(m, a, visible ? kit.glaze[1] : 256, seed, false, true);
        }
        orient(m, a, visible ? kit.glaze[1] : 256, seed, false, true);
        a.listened += response.strokes;
        a.paid += response.strokes;
    } else if (command == 2 && body) {
        for (Word i = 0; i < limit && response.strokes + 1 < budget; ++i) {
            Word key = mix(seed + i * 7919), motor = key & 255, port = (key >> 21) & 1;
            if (!(m.genes.fingers & (1u << (8 + (motor & 7)))))
                continue;
            Word at = finger(m.genes, motor, key, (key >> 9) & 3), pulse = groove(kit.objects[port], at);
            ++response.strokes;
            rehearse(a, (key >> 11) & 3, false);
            if ((pulse & 128) && eyes) {
                trace(m, a, pulse, visible ? kit.glaze[port] : 256, key, read | color | latent, true);
                ++response.glints;
            }
        }
        a.paid += response.strokes;
    }
    response.visible = visible;
    response.context = resolved(m);
    response.alternatives = count(m);
    return response;
}
} // namespace axion::life::clay
KERNEL void clay_handle(siphon::Pair *native, clay::Mind *cortex, siphon::Room *room, const clay::Instrument *surface,
                        Word command, Word limit, Word seed, Word reserve, Word visibility, Word limbs, Word eyes,
                        clay::Hearing *out) {
    if (tid())
        return;
    *out = clay::handle(native->b, *cortex, *surface, command, limit, seed, room->site.membrane, reserve, visibility,
                        limbs, eyes, room->struck);
    if (command == 0) {
        room->struck = 0;
        room->exposed = 0;
    } else
        room->struck |= out->mask;
    room->context = out->context;
}
KERNEL void clay_turn(siphon::Pair *actors, clay::Mind *cortex, siphon::Room *room, const siphon::Vocabulary *words,
                      RecollectionGenes lex, const clay::Instrument *instrument, Word taps, Word visible, Word wire,
                      Word a, Word b, Word plastic, Word organs, siphon::Sentence phrase, siphon::Report *out,
                      clay::Hearing *e) {
    if (tid())
        return;
    auto intent = siphon::interpret(*words, lex, actors->b, phrase);
    clay::Hearing event{};
    if (intent.recognized) {
        Word target = intent.property < 4 ? intent.property : room->site.membrane;
        event = clay::handle(actors->b, *cortex, *instrument, 3, taps, room->seed, target, b, visible, organs & 4,
                             organs & 16, room->struck);
        room->struck |= event.mask;
        room->context = event.context;
    }
    room->wire = wire;
    auto r = siphon::reply_to(*actors, lex, *room, *words, intent, a, b - event.strokes, plastic);
    r.cost_b += event.strokes;
    *e = event;
    *out = r;
}
KERNEL void clay_utterances(const siphon::Pair *native, const clay::Mind *cortex, const siphon::Vocabulary *words,
                            RecollectionGenes lex, const clay::Kit *set, const clay::Charts *charts,
                            const clay::Prompt *problems, clay::Answer *answers, Word n, Word reserve, Word body) {
    Word i = tid();
    if (i >= n)
        return;
    auto &test = problems[i];
    auto q = test.problem;
    auto m = *cortex;
    auto actors = *native;
    actors.a = set->actors[test.actor];
    auto &tool = set->instruments[test.object];
    siphon::Room room{};
    room.site = siphon::pool(q.seed, 4, 0);
    room.seed = q.seed;
    room.site.inlet = q.inlet;
    room.site.membrane = q.filter;
    room.site.opening = q.angle;
    room.wire = q.wire;
    room.body = (body & 22) | (set->organs[test.actor] & 9);
    auto intent = siphon::interpret(*words, lex, actors.b, q.words);
    Word target = intent.property < 4 ? intent.property : q.filter;
    auto evidence =
        clay::handle(actors.b, m, tool, 0, 0, q.seed, target, reserve, test.visibility & 1, body & 4, body & 16);
    if (intent.recognized && test.taps)
        evidence = clay::handle(actors.b, m, tool, (test.visibility & 2) ? 3 : 1, test.taps, q.seed, target, reserve,
                                test.visibility & 1, body & 4, body & 16, 0, charts);
    room.context = evidence.context;
    auto r =
        siphon::reply_to(actors, lex, room, *words, intent, set->energy[test.actor], reserve - evidence.strokes, false);
    answers[i] = {r, evidence};
}
