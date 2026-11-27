#pragma once
#include "compound_sign.hpp"

namespace axion::life::capillary {
// The same learned link can be read in both directions, from sensation to
// vocal reproduction or from a heard waveform to the associated sensation.
struct SignalVoice {
    Count form[2][4]{}, prompt[2]{};
    Word links[2]{};
    utterance::Trail episodes{};
};
static_assert(sizeof(SignalVoice) == 96);
struct VoiceSchool {
    Count sound[2][4]{};
    utterance::Waves prompt[2]{};
};
struct VoiceTuition {
    Word singles[2]{}, heard = 0, edits = 0, spikes = 0, heat = 0, pressure_strokes = 0, limb_strokes = 0;
};
struct AudibleVisit {
    Count call[2]{}, answer[2]{}, query[2]{}, clarified[2]{};
    Word cue = 0, mark = 0, visible = 0, heard = 4, choice = 4, motion = 0, statement = 4, food = 0, initial = 15,
         remaining = 15, alternatives = 0, requests = 0, answers = 0, formed = 0, routes = 0;
};
constexpr Word unknown_signal = 4;
AX_FN Count prompt_image(const utterance::Waves &line, Count cochlea) {
    if (!line.n || !cochlea)
        return 0;
    Count h = 1469598103934665603ull;
    for (Word i = 0; i < line.n; ++i) {
        Count v = line.wave[i] & cochlea;
        if (!v)
            return 0;
        h = (h ^ v) * 1099511628211ull;
    }
    return h;
}
AX_FN Word acoustic_slot(const SignalVoice &m, Count mask, Word organ, Count wave) {
    if (!(wave & mask))
        return 4;
    for (Word i = 0; i < 4; ++i)
        if (m.form[organ][i] && !((m.form[organ][i] ^ wave) & mask))
            return i;
    return 4;
}
AX_FN Word positive_event(const SignalVoice &m, Word organ, Word socket) {
    Word result = 4;
    for (Word event = 0; event < 4; ++event)
        if (((m.links[organ] >> (2 * (4 * socket + event))) & 3) == 1) {
            if (result < 4)
                return 4;
            result = event;
        }
    return result;
}
AX_FN Word acquire_sound(SignalVoice &m, Body &b, Count mask, Word organ, Word event, Count wave,
                         const utterance::Waves &prompt, VoiceTuition &bill) {
    if (!b.eyes || !b.limbs || !b.mouth || !(wave & mask))
        return 0;
    Count q = prompt_image(prompt, mask);
    if (!q || (m.prompt[organ] && q != m.prompt[organ]))
        return 0;
    Word socket = acoustic_slot(m, mask, organ, wave);
    if (socket == 4) {
        for (socket = 0; socket < 4 && m.form[organ][socket]; ++socket) {
        }
        if (socket == 4)
            return 0;
    }
    Word bit = 2 * (4 * socket + event), before = (m.links[organ] >> bit) & 3;
    Word next = admitted_sign(b, before, 1, bill.spikes, 1, nullptr, 0);
    // A new acoustic receptor is recruited only when the observed event can
    // actually form an excitatory synapse under this animal's release rule.
    if (!m.form[organ][socket] && !(next & 1))
        return 0;
    m.form[organ][socket] = wave;
    m.prompt[organ] = q;
    m.links[organ] ^= (before ^ next) << bit;
    bill.edits += __builtin_popcount(before ^ next);
    bit = 2 * (4 * socket + (event ^ 1u));
    before = (m.links[organ] >> bit) & 3;
    next = admitted_sign(b, before, 2, bill.spikes, 1, nullptr, 0);
    m.links[organ] ^= (before ^ next) << bit;
    bill.edits += __builtin_popcount(before ^ next);
    return 1;
}
AX_FN Count voice_event(const SignalVoice &m, Word organ, Word event) {
    Count word = 0;
    for (Word j = 0; j < 4; ++j)
        if (m.form[organ][j] && positive_event(m, organ, j) == event) {
            if (word)
                return 0;
            word = m.form[organ][j];
        }
    return word;
}
AX_FN bool addressed(const SignalVoice &m, Count cochlea, Word domain, const utterance::Waves &line) {
    return m.prompt[domain] && prompt_image(line, cochlea) == m.prompt[domain];
}
AX_FN utterance::Waves relate(const SignalVoice &m, const Body &b, Count cochlea, Word organ, Word state,
                              const utterance::Waves &question) {
    utterance::Waves out{};
    if (!b.mouth || !addressed(m, cochlea, organ, question))
        return out;
    Word axis = (b.motor >> (11 + organ)) & 1;
    for (Word k = 0; k < 2; ++k, axis ^= 1) {
        Count event = voice_event(m, organ, 2 * axis + ((state >> axis) & 1));
        if (!event)
            return {};
        out.wave[out.n++] = event;
    }
    return out;
}
AX_FN Word recognise(const SignalVoice &m, Count cochlea, Word organ, const utterance::Waves &line) {
    if (line.n != 2)
        return unknown_signal;
    Word view = 0, axes = 0;
    for (Word j = 0; j < 2; ++j) {
        Word socket = acoustic_slot(m, cochlea, organ, line.wave[j]);
        if (socket >= 4)
            return unknown_signal;
        Word event = positive_event(m, organ, socket);
        if (event >= 4 || ((axes >> (event >> 1)) & 1))
            return unknown_signal;
        axes |= 1u << (event >> 1);
        view |= (event & 1) << (event >> 1);
    }
    return view;
}
AX_FN bool remember_sensation(SignalVoice &m, Body &b, Word kind, Word state, Word &spikes) {
    if (!(admitted_sign(b, 0, 1, spikes, 1, nullptr, 0) & 1))
        return false;
    m.episodes.append((kind << 2) | state);
    return true;
}
AX_FN Word last_sensation(const SignalVoice &m, Word kind) {
    for (Word i = m.episodes.size(); i; --i) {
        Word v = m.episodes.at(i - 1);
        if ((v >> 2) == kind)
            return v & 3;
    }
    return unknown_signal;
}
AX_FN bool valid(const SignalVoice &m) {
    if (!m.episodes.valid())
        return false;
    for (Word i = 0; i < m.episodes.size(); ++i)
        if (m.episodes.at(i) >= 8)
            return false;
    for (Word organ = 0; organ < 2; ++organ)
        for (Word j = 0; j < 4; ++j) {
            if (!m.form[organ][j] && ((m.links[organ] >> (8 * j)) & 255))
                return false;
            for (Word k = 0; k < j; ++k)
                if (m.form[organ][j] && m.form[organ][j] == m.form[organ][k])
                    return false;
        }
    return true;
}
AX_FN Word named_direction(const VoiceSchool &s, const utterance::Waves &line) {
    if (line.n != 2)
        return 4;
    Word axes = 0, where = 0;
    for (Word j = 0; j < 2; ++j) {
        Word f = 0;
        while (f < 4 && s.sound[1][f] != line.wave[j])
            ++f;
        if (f == 4 || ((axes >> (f / 2)) & 1))
            return 4;
        axes |= 1u << (f / 2);
        where |= (f & 1) << (f / 2);
    }
    return where;
}
} // namespace axion::life::capillary
