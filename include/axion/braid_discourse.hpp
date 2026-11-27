#pragma once
#include "braid_episode.hpp"
#include "braid_talk.hpp"

namespace axion::life::braid {
constexpr Word sounds_max = 32, forms_max = 16, variable = 33;
struct Form {
    Count pattern = 0;
    Half pruned = 0;
    unsigned char tokens = 0, examples = 0;
    Word alignment = 0; // One word of the resident’s four auditory partner records.
};
// The high token bit distinguishes a learned temporal association from a
// grammatical form. The acoustic key and sixteen local synapses occupy the
// same resident tissue. The four alignment bytes keep their usual function.
constexpr Word sequence_tag = 128u;
AX_FN bool sequence_form(const Form &f) { return f.tokens & sequence_tag; }
AX_FN Word sequence_length(const Form &f) { return f.tokens & 7u; }
struct Narration {
    Count sounds[sounds_max]{};
    Form forms[forms_max]{};
    Word labels = 0;
    unsigned char excluded[sounds_max]{};
    Episode event{};
    unsigned char topic = 0, reply = 0, pending = 0, auditory_route = 0;
    Word reported = 0;
};
struct ReportSaved {
    Narration memory{};
    Word tag = 0x4c4c4554u, extent = sizeof(ReportSaved);
};
struct Inquiry {
    Count pattern = 0;
    Word n = 0, reference[2]{none, none}, slots = 0;
};
struct Answer {
    Speech words{};
    Word matched = 0, queried = 0, answered = 0, options = 0, source = none, frame = 4, said = none, understood = 0,
         learned = 0, by_report = 0;
};
struct SpokenEvidence {
    Answer original{}, witness{}, final{};
    Word changed = 0;
};
enum Classroom : Word {
    point_lesson,
    name_lesson,
    form_lesson,
    manual_caption,
    normal_query,
    manual_answer,
    witness_reply,
    clear_topic,
    read_patterns,
    clear_reports,
    set_lens,
    share_view,
    compare_views,
    inspect_view
};
struct Course {
    Word cmd = 0, who = 0, place = 0, board = 0, frame = 0, role = 0, kind = 0, reps = 0, seed = 0, slot = none,
         channel = 0, peer = 1, budget = 4, sampling = 0, vision = 4095, alternative = none;
    Speech words{}, names{};
    Count echo = 0, address = 0, second_voice = 0;
};
struct Receipt {
    SpokenEvidence turns{};
    Word approached = none, coverage[2]{};
    Word hypotheses = 0, established = 0, asked = 0, replies = 0, examples = 0, pairs = 0;
    Word learned = 0, offered = 0, cost = 0, status = 0, forms = 0, nouns = 0;
};
static_assert(sizeof(Episode) == 12 && sizeof(Form) == 16 && sizeof(Narration) == 568 && sizeof(ReportSaved) == 576);
// Four words per acquaintance are interleaved in the four-byte tail of each
// construction. The existing 568-byte resident image stays byte compatible.
AX_FN Correspondence contact(const Narration &m, Word i) {
    auto *f = m.forms + 4 * i;
    return {Count(f[0].alignment) | (Count(f[1].alignment) << 32), f[2].alignment, f[3].alignment};
}
AX_FN void remember_contact(Narration &m, Word i, Correspondence c) {
    auto *f = m.forms + 4 * i;
    f[0].alignment = Word(c.voice);
    f[1].alignment = Word(c.voice >> 32);
    f[2].alignment = c.excluded;
    f[3].alignment = c.panel;
}
AX_FN Word acquaintance(const Narration &m, Count voice, bool recruit) {
    if (!voice)
        return none;
    Word first = mix(Word(voice) ^ Word(voice >> 32)) & 3, free = none;
    for (Word j = 0; j < 4; ++j) {
        Word i = (j + first) & 3;
        auto c = contact(m, i);
        if (c.voice == voice)
            return i;
        if (!c.voice && free == none)
            free = i;
    }
    return recruit ? (free != none ? free : first) : none;
}
AX_FN Count timbre(const Mind &g) { return (Count(g.genes.throat) << 32) | mix(g.genes.throat ^ 0x8d2cbac7u) | 1u; }
AX_FN void experience(Narration &m, Episode e) {
    m.event = e;
    m.pending = 0;
    m.reported = 0;
}
AX_FN Word intern(Narration &m, Count wave, bool write) {
    if (!wave)
        return none;
    Word start = mix(Word(wave) ^ Word(wave >> 32)) & (sounds_max - 1);
    for (Word j = 0; j < sounds_max; ++j) {
        Word i = (j + start) & (sounds_max - 1);
        if (m.sounds[i] == wave)
            return i;
        if (!m.sounds[i]) {
            if (write) {
                m.sounds[i] = wave;
                return i;
            }
            return none;
        }
    }
    return none;
}
AX_FN Word ident(const Narration &m, Word i) {
    if (i >= sounds_max || !(m.labels & (1u << i)))
        return none;
    Word allowed = 255 ^ m.excluded[i];
    return allowed && !(allowed & (allowed - 1)) ? Word(__builtin_ctz(allowed)) : none;
}
AX_FN Count name_of(const Narration &m, Word value) {
    for (Word j = 0; j < sounds_max; ++j)
        if (ident(m, j) == value)
            return m.sounds[j];
    return 0;
}
AX_FN Word notice(Narration &m, Genes genes, Count wave, Word socket, Word colors) {
    if (!(genes.arbor & (1u << 14)))
        return 0;
    Word c = intern(m, wave, true);
    if (c == none)
        return 0;
    Word allowed = (1u << socket) | (1u << (4 + pigment(colors, socket)));
    Word old = m.excluded[c], next = rule(genes.plastic >> 8, old, 255 ^ allowed) & 255;
    m.labels |= 1u << c;
    m.excluded[c] = next;
    return __builtin_popcount(next ^ old);
}
AX_FN Inquiry recognize_form(Narration &m, const Speech &line, bool acquire) {
    Inquiry q{};
    if (!line.n || line.n > max_phrase)
        return q;
    for (Word i = 0; i < line.n; ++i) {
        Word token = intern(m, line.wave[i], acquire);
        if (token == none)
            return {};
        Word value = ident(m, token), address = token + 1;
        if (m.labels & (1u << token)) {
            if (value == none || q.slots == 2)
                return {};
            q.reference[q.slots++] = value;
            address = variable + (value >> 2);
        }
        q.pattern |= Count(address) << (6 * i);
    }
    q.n = line.n;
    return q;
}
AX_FN Word lookup_form(Narration &m, const Inquiry &q, bool acquire) {
    if (!q.n)
        return none;
    for (Word i = 0; i < forms_max; ++i) {
        auto &f = m.forms[i];
        if (f.pattern == q.pattern && f.tokens == q.n)
            return i;
        if (!f.tokens) {
            if (acquire) {
                f.pattern = q.pattern;
                f.tokens = q.n;
                return i;
            }
            return none;
        }
    }
    return none;
}
AX_FN Word subject(const Narration &m, const Inquiry &q, Word channel) {
    if (channel < 2)
        return q.reference[channel];
    Word stored = channel == 2 ? m.topic : m.reply;
    return stored ? stored - 1 : none;
}
// Unobserved sockets still carry the dyes present before the reach. One
// reported socket can therefore constrain another without inventing a sight.
AX_FN Word possible(const Episode &e, Word frame, Word ref) {
    if (ref >= 8 || !e.stamp)
        return 0;
    Word scene = picture(e, frame), known = visible(e, frame), start = picture(e, beginning);
    if (ref < 4 && (known & (1u << ref)))
        return 1u << (4 + pigment(scene, ref));
    if (ref >= 4 && known == 15)
        return lamps(scene, ref - 4, 3);
    Word left = 0;
    for (Word hue = 0; hue < 4; ++hue) {
        Word total = visible(e, beginning) == 15 ? Word(__builtin_popcount(lamps(start, hue, 3))) : 4;
        Word seen = __builtin_popcount(lamps(scene, hue, 3) & known);
        if (seen > total)
            return 0;
        if (seen < total)
            left |= 1u << hue;
    }
    if (ref < 4)
        return left << 4;
    Word dye = ref - 4, seen = lamps(scene, dye, 3) & known;
    return seen | ((left & (1u << dye)) ? (15 ^ known) : 0);
}
template<class PlasticGate> AX_FN Word receive_form(Narration &m, Genes genes, const Speech &line, Count answer, PlasticGate gate) {
    if (!(genes.arbor & (1u << 15)))
        return 0;
    Word value = ident(m, intern(m, answer, false));
    if (value == none)
        return 0;
    auto q = recognize_form(m, line, true);
    Word f = lookup_form(m, q, true);
    if (f == none)
        return 0;
    Word support = 0;
    for (Word k = 0; k < 16; ++k) {
        Word ref = subject(m, q, k >> 2);
        if (possible(m.event, k & 3, ref) & (1u << value))
            support |= 1u << k;
    }
    auto &form = m.forms[f];
    Word old = form.pruned;
    Word conflict=all ^ support;
    form.pruned=Half(gate(old,conflict,rule(genes.plastic>>12,old,conflict)));
    if (form.examples < 255)
        ++form.examples;
    if (q.slots)
        m.topic = q.reference[0] + 1;
    m.reply = value + 1;
    m.pending = 0;
    return __builtin_popcount(old ^ form.pruned);
}
AX_FN Word teach_form(Narration &m, Genes genes, const Speech &line, Count answer) {
    return receive_form(m,genes,line,answer,[](Word,Word,Word proposed){return proposed;});
}
AX_FN bool equivalent(const Form &f, Word frame, Word arg) {
    Word active = all ^ f.pruned;
    return active && (active & ~(1u << (frame + 4 * arg))) == 0;
}
AX_FN Speech full_question(const Narration &m, Word frame, Word ref) {
    Count noun = name_of(m, ref);
    if (!noun)
        return {};
    for (Word i = 0; i < forms_max; ++i) {
        auto &f = m.forms[i];
        if (sequence_form(f) || !f.examples || !equivalent(f, frame, 0))
            continue;
        Speech q{};
        Word slots = 0;
        bool fits = true;
        for (Word j = 0; j < f.tokens; ++j) {
            Word v = (f.pattern >> (6 * j)) & 63;
            if (v == variable + (ref >> 2)) {
                q.wave[q.n++] = noun;
                ++slots;
            } else if (v >= 1 && v <= sounds_max)
                q.wave[q.n++] = m.sounds[v - 1];
            else {
                fits = false;
                break;
            }
        }
        if (fits && slots == 1)
            return q;
    }
    return {};
}
AX_FN Answer question(Narration &m, const Speech &line, bool may_ask) {
    Answer a{};
    auto q = recognize_form(m, line, false);
    Word f = lookup_form(m, q, false);
    if (f == none || !m.forms[f].examples)
        return a;
    a.matched = 1;
    m.pending = 0;
    Word active = all ^ m.forms[f].pruned, argument = none, frame = 4;
    bool unanimous = true, missing = false;
    for (Word k = 0; k < 16; ++k)
        if (active & (1u << k)) {
            Word ref = subject(m, q, k >> 2);
            if (ref >= 8) {
                unanimous = false;
                missing = true;
                continue;
            }
            if (argument == none) {
                argument = ref;
                frame = k & 3;
            } else if (argument != ref || frame != (k & 3))
                unanimous = false;
            auto available = possible(m.event, k & 3, ref);
            a.options |= available;
            missing |= !available;
        }
    if (argument == none)
        return a;
    a.source = argument;
    a.frame = unanimous ? frame : 4;
    if (!missing && a.options && !(a.options & (a.options - 1))) {
        Word value = __builtin_ctz(a.options);
        Count word = name_of(m, value);
        if (!word)
            return a;
        a.words.n = 1;
        a.words.wave[0] = word;
        a.answered = 1;
        a.said = value;
        if (unanimous && ((m.reported >> (4 * frame)) & 15)) {
            auto sight = m.event;
            sight.known &= ~m.reported;
            a.by_report = possible(sight, frame, argument) != (1u << value);
        }
        m.topic = (unanimous ? argument : q.slots ? q.reference[0] : m.topic ? m.topic - 1 : argument) + 1;
        m.reply = value + 1;
        return a;
    }
    if (may_ask && unanimous && a.options && visible(m.event, frame) != 15) {
        a.words = full_question(m, frame, argument);
        if (a.words.n) {
            m.pending = 1 + 8 * frame + argument;
            a.queried = 1;
        }
    }
    return a;
}
AX_FN Answer accept(Narration &m, Count raw, bool retain) {
    Answer a{};
    if (!m.pending)
        return a;
    Word value = ident(m, intern(m, raw, false)), source = (m.pending - 1) & 7, frame = (m.pending - 1) >> 3;
    a.source = source;
    a.frame = frame;
    if (value == none || !((source ^ value) & 4) || !(possible(m.event, frame, source) & (1u << value)))
        return a;
    Word socket = source < 4 ? source : value, hue = source >= 4 ? source - 4 : value - 4;
    if (retain) {
        Word shift = 2 * socket;
        auto colors = picture(m.event, frame);
        expose(m.event, frame, (colors & ~(3u << shift)) | (hue << shift), visible(m.event, frame) | (1u << socket));
        m.reported |= 1u << (4 * frame + socket);
    }
    m.pending = 0;
    m.topic = source + 1;
    m.reply = value + 1;
    a.understood = 1;
    a.answered = 1;
    a.said = value;
    a.words.wave[0] = raw;
    a.words.n = 1;
    return a;
}
AX_FN bool valid_narration(const Narration &m) {
    if (m.topic > 8 || m.reply > 8 || m.pending > 32 || m.auditory_route > 1 || !valid_episode(m.event) ||
        (m.reported & ~m.event.known))
        return false;
    for (Word i = 0; i < sounds_max; ++i) {
        if (!m.sounds[i] && ((m.labels & (1u << i)) || m.excluded[i]))
            return false;
        for (Word j = 0; j < i; ++j)
            if (m.sounds[i] && m.sounds[i] == m.sounds[j])
                return false;
    }
    for (Word i = 0; i < 4; ++i) {
        auto c = contact(m, i);
        if (!sound_contact(c))
            return false;
        for (Word j = 0; j < i; ++j)
            if (c.voice && c.voice == contact(m, j).voice)
                return false;
    }
    bool empty = false;
    for (auto &f : m.forms) {
        if (sequence_form(f)) {
            Word n = sequence_length(f);
            if (empty || !n || n > 4 || f.tokens != (sequence_tag | n) || !f.pattern || !f.examples ||
                (Word(f.pruned) >> (4 * n)))
                return false;
            for (const auto &earlier : m.forms) {
                if (&earlier == &f)
                    break;
                if (sequence_form(earlier) && earlier.pattern == f.pattern)
                    return false;
            }
            continue;
        }
        if (f.tokens > max_phrase)
            return false;
        if (!f.tokens) {
            if (f.pattern || f.pruned || f.examples)
                return false;
            empty = true;
            continue;
        }
        if (empty || !f.examples || (f.pattern >> (6 * f.tokens)))
            return false;
        for (Word j = 0; j < f.tokens; ++j) {
            Word x = (f.pattern >> (6 * j)) & 63;
            if (!x || x > 34 || (x <= sounds_max && !m.sounds[x - 1]))
                return false;
        }
    }
    return true;
}
} // namespace axion::life::braid
