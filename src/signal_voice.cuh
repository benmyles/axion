#include "axion/clarification.hpp"
namespace axion::life::capillary {
KERNEL void learn_signal_voices(CompoundMind *pair, SignalVoice *voices, const Count *ears, const VoiceSchool *schools,
                                const CompoundBed *beds, Word n, Word budget, Word seed, VoiceTuition *bills) {
    Word who = tid();
    if (who >= 2)
        return;
    auto animal = pair[who];
    auto memory = voices[who];
    auto &school = schools[who];
    VoiceTuition bill{};
    Junction valves[2]{beds[0].cell[0], beds[0].cell[1]};
    Word carriage = animal.body.motor & 3;
    auto stroke = [&](Word axis, Word input) {
        auto &v = valves[axis];
        auto r = cycle(v.base, v.coating, v.carrier, input);
        v.coating = r.coating;
        v.carrier = r.carrier;
        ++bill.pressure_strokes;
        return r.flow;
    };
    for (Word t = 0; t < n; ++t) {
        Word id = (t + (mix(seed + who * 2171) & 3)) & 3, axis = id >> 1;
        for (Word organ = 0; organ < 2; ++organ) {
            Word step = id & 1;
            Word physical = organ ? Word(((carriage >> axis) & 1) != step) : 2 + step;
            Word cost = school.prompt[organ].n + 2 + physical;
            if (cost > budget - bill.heat || !animal.body.limbs)
                continue;
            Word state;
            if (!organ) {
                if (step)
                    stroke(axis, valves[axis].row);
                state = stroke(axis, valves[axis].row);
                stroke(axis, 9); // The same clear return path restores the two pigments.
            } else {
                carriage = (carriage & ~(1u << axis)) | (step << axis);
                bill.limb_strokes += physical;
                state = (carriage >> axis) & 1;
            }
            Word event = 2 * axis + state;
            bill.heat += cost;
            ++bill.heard;
            bill.singles[organ] += acquire_sound(memory, animal.body, ears[who], organ, event,
                                                 school.sound[organ][event], school.prompt[organ], bill);
        }
    }
    pair[who] = animal;
    voices[who] = memory;
    bills[who] = bill;
}
KERNEL void private_signal_dialogue(const CompoundMind *pair, const SignalVoice *voices, const Count *ears,
                                    const VoiceSchool *schools, const CompoundBed *beds, Signalling c, Word n, Word arm,
                                    Word budget, Word reply_mode, AudibleVisit *out) {
    Word i = tid();
    if (i >= 4 * n)
        return;
    Word cue = i & 3, key = relay_noise(relay_seed(c, c.first), i / 4, c);
    auto a = pair[0], b = pair[1];
    auto scene = beds[1 + (i / 4) % c.tests];
    AudibleVisit r{};
    r.cue = cue;
    r.mark = compose_move(a, cue, mix(key + 49277));
    r.visible = compound_light(scene.optics, r.mark);
    auto question = schools[0].prompt[0];
    if (arm == 5)
        question = {};
    auto source = voices[0];
    Word spikes = 0;
    bool saw = arm != 6 && remember_sensation(source, a.body, 0, r.visible, spikes);
    auto line = saw ? relate(source, a.body, ears[0], 0, last_sensation(source, 0), question) : utterance::Waves{};
    r.formed = line.n;
    line = damaged_call(line, arm, key);
    if (line.n)
        r.call[0] = line.wave[0];
    if (line.n == 2)
        r.call[1] = line.wave[1];
    Word input = recognise(voices[1], ears[1], 0, line);
    if (r.formed && arm != 1) {
        auto repair = resolve_audio(source, voices[1], ears[0], ears[1], b, line, mix(key + 24407), budget, reply_mode,
                                    compound_light(scene.optics, r.mark ^ 3));
        r.routes = repair.routes;
        r.initial = repair.initial;
        r.remaining = repair.remaining;
        r.alternatives = repair.possible_actions;
        r.requests = repair.offered;
        r.answers = repair.answered;
        for (Word j = 0; j < 2; ++j) {
            r.query[j] = repair.question[j];
            r.clarified[j] = repair.reply[j];
        }
        input = repair.committed;
        r.heard = __builtin_popcount(repair.remaining) == 1 ? __builtin_ctz(repair.remaining) : 4;
    } else {
        r.heard = input;
        r.initial = 15;
        for (Word i = 0; i < line.n; ++i)
            r.initial &= compatible_lights(voices[1], ears[1], line.wave[i]);
        r.remaining = r.initial;
    }
    if (input < 4) {
        r.choice = compose_move(b, input, mix(key + 24407));
        r.food = gather_meal(cue, r.choice);
        r.motion = 1;
        auto listener = voices[1];
        bool moved = remember_sensation(listener, b.body, 1, r.choice, spikes);
        auto reply = moved ? relate(listener, b.body, ears[1], 1, last_sensation(listener, 1), schools[1].prompt[1])
                           : utterance::Waves{};
        if (reply.n)
            r.answer[0] = reply.wave[0];
        if (reply.n == 2)
            r.answer[1] = reply.wave[1];
        r.statement = named_direction(schools[1], reply);
    }
    out[i] = r;
}
} // namespace axion::life::capillary
