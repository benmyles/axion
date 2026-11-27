#pragma once
#include "signal_voice.hpp"

namespace axion::life::capillary {
struct Clarification {
    Count question[2]{}, reply[2]{};
    Word offered = 0, answered = 0, initial = 15, remaining = 15, possible_actions = 0, routes = 0, committed = 4,
         effort = 0;
};
AX_FN Word compatible_lights(const SignalVoice &voice, Count ear, Count sound) {
    Word socket = acoustic_slot(voice, ear, 0, sound);
    if (socket >= 4)
        return 15;
    Word event = positive_event(voice, 0, socket);
    if (event >= 4)
        return 15;
    Word high = event >= 2 ? 12u : 10u;
    return event & 1 ? high : 15 ^ high;
}
AX_FN Count answer_probe(const SignalVoice &owner, Count ear, Count proposed, Word remembered) {
    if (remembered >= 4)
        return 0;
    Word s = acoustic_slot(owner, ear, 0, proposed);
    if (s >= 4)
        return 0;
    Word event = positive_event(owner, 0, s);
    if (event >= 4)
        return 0;
    Word axis = event >> 1;
    return voice_event(owner, 0, 2 * axis + ((remembered >> axis) & 1));
}
AX_FN Clarification resolve_audio(const SignalVoice &witness, const SignalVoice &listener, Count source_ear,
                                  Count listener_ear, CompoundMind reader, const utterance::Waves &heard, Word noise,
                                  Word budget, Word reply_mode, Word foreign) {
    Clarification q{};
    for (Word j = 0; j < heard.n; ++j)
        q.initial &= compatible_lights(listener, listener_ear, heard.wave[j]);
    q.remaining = q.initial;
    Word routes = 0;
    for (Word state = 0; state < 4; ++state) {
        auto b = reader;
        routes |= compose_move(b, state, noise) << (2 * state);
    }
    q.routes = routes;
    Word tried = 0;
    while (q.remaining) {
        Word actions = 0;
        for (Word state = 0; state < 4; ++state)
            if (q.remaining & (1u << state))
                actions |= 1u << ((routes >> (2 * state)) & 3);
        if (!tried)
            q.possible_actions = actions;
        if (!(actions & (actions - 1))) {
            q.committed = __builtin_ctz(q.remaining);
            break;
        }
        if (q.offered >= budget)
            break;
        Word best = 0, axis = 2;
        Count sound = 0;
        for (Word j = 0; j < 2; ++j) {
            if (tried & (1u << j))
                continue;
            Word score = 0;
            for (Word a = 0; a < 4; ++a)
                if ((q.remaining >> a) & 1)
                    for (Word b = a + 1; b < 4; ++b)
                        if (((q.remaining >> b) & 1) && (((a ^ b) >> j) & 1) &&
                            (((routes >> (2 * a)) ^ (routes >> (2 * b))) & 3))
                            ++score;
            score *= 1 + ((reader.body.motor >> (8 + 4 * j)) & 3);
            Word guess = (reader.body.motor >> (21 + j)) & 1;
            Count word = voice_event(listener, 0, 2 * j + guess);
            if (!word)
                word = voice_event(listener, 0, 2 * j + 1 - guess);
            if (word && score > best) {
                axis = j;
                best = score;
                sound = word;
            }
        }
        if (axis == 2 || best < (1u + ((reader.body.motor >> 28) & 1)))
            break;
        tried |= 1u << axis;
        Word at = q.offered++;
        q.question[at] = sound;
        ++q.effort;
        Word memory = reply_mode == 2 ? foreign : last_sensation(witness, 0);
        Count response = reply_mode == 1 ? 0 : answer_probe(witness, source_ear, sound, memory);
        if (response) {
            q.reply[at] = response;
            ++q.answered;
            ++q.effort;
            q.remaining &= compatible_lights(listener, listener_ear, response);
        }
    }
    return q;
}
AX_FN utterance::Waves damaged_call(utterance::Waves line, Word arm, Word noise) {
    if (arm == 1)
        return {};
    if (arm == 2 && line.n)
        line.n = 1;
    if (arm == 3 && line.n == 2) {
        Count swap = line.wave[0];
        line.wave[0] = line.wave[1];
        line.wave[1] = swap;
    }
    if (arm == 4 && line.n == 2)
        line.wave[1] = line.wave[0];
    if (arm == 7) {
        utterance::Waves received{};
        Word mask = mix(noise + 8731) & 3;
        for (Word j = 0; j < line.n; ++j)
            if (mask & (1u << j))
                received.wave[received.n++] = line.wave[j];
        return received;
    }
    return line;
}
} // namespace axion::life::capillary
