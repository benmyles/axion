#pragma once
#include "utterance.hpp"

namespace axion::life::utterance {
constexpr Word names = 48, phases = 8, shapes = 3, prompts = 4, max_glue = 3, styles = 6;
constexpr Word voice_span = depth * phases + (depth - 1) * max_glue;
struct Phrase {
    Byte word[voice_span]{};
    Word n = 0;
    AX_FN bool push(Word token) {
        if (!token || token > names || n == voice_span)
            return false;
        word[n++] = Byte(token);
        return true;
    }
};
AX_FN bool same(const Phrase &a, const Phrase &b) {
    if (a.n != b.n)
        return false;
    for (Word i = 0; i < a.n; ++i)
        if (a.word[i] != b.word[i])
            return false;
    return true;
}
struct Vocabulary {
    Count sounds[names]{};
    AX_FN Word find(Count sound, bool recruit) {
        if (!sound)
            return 0;
        for (Word i = 0; i < names; ++i) {
            if (sounds[i] == sound)
                return i + 1;
            if (!sounds[i]) {
                if (recruit)
                    sounds[i] = sound;
                return recruit ? i + 1 : 0;
            }
        }
        return 0;
    }
};
AX_FN Count address(const Phrase &q) {
    if (!q.n || q.n > phases)
        return 0;
    Count code = 0;
    for (Word i = 0; i < q.n; ++i)
        code |= Count(q.word[i]) << (6 * i);
    return code;
}
struct Biology {
    Word law = 0, arbor = 0;
    regulation::Program program{};
    Word gated = 1, release = 0;
};
struct Growth {
    Word contacts = 0, revisions = 0, firings = 0;
    regulation::Cells cells{};
};
AX_FN Word conducting(Biology body, Growth &change, Word previous, Word activity, Word proposed, bool familiar,
                      bool successful, Word mask) {
    Word old = change.cells.spike;
    Word gate = regulation::discharge(body.program, change.cells, previous, activity, proposed, familiar, successful);
    change.firings += __builtin_popcount(old ^ change.cells.spike);
    if (!body.gated)
        return mask;
    if (body.gated == 2)
        return 0;
    if (body.gated == 4 || (body.gated == 1 && (body.release & (1u << 28))))
        return __builtin_popcount(gate) >= 1 + int((body.release >> 29) & 7) ? mask : 0;
    return regulation::rolled(gate, (body.release >> 20) & 15) & mask;
}
// Each row is one potential four-cell retinal projection to this vocal phase.
// Its output addresses are recruited during listening. A conflict is local:
// the very same active receptor previously excited a different sound.
struct Socket {
    Word row[fields]{};
    unsigned short closed = 0;
    Byte fixed = 0, inhibition = 0;
};
constexpr Word sockets = (1u << (fields + 1)) - 1;
AX_FN void pair(Socket &s, Word light, Word sound, Biology body, Growth &change) {
    if (!sound || !(body.arbor & (1u << 14)))
        return;
    Word conflict = 0, vacant = 0;
    if (!s.fixed)
        vacant |= 1u << fields;
    else if (s.fixed != sound)
        conflict = 1u << fields;
    for (Word j = 0; j < fields; ++j) {
        Word shift = 8 * channel(light, j), old = (s.row[j] >> shift) & 255;
        if (!old)
            vacant |= 1u << j;
        else if (old != sound)
            conflict |= 1u << j;
    }
    Word next = braid::rule_bits(body.law, s.closed, conflict) & sockets;
    Word gate = conducting(body, change, s.closed, conflict, next, vacant != sockets, !conflict, sockets);
    s.inhibition = (body.release >> 25) & 3;
    if (gate & vacant & (1u << fields))
        s.fixed = sound;
    for (Word write = gate & vacant & ((1u << fields) - 1); write; write &= write - 1) {
        Word j = __builtin_ctz(write);
        s.row[j] |= sound << (8 * channel(light, j));
    }
    change.contacts += __builtin_popcount(gate & vacant);
    next = regulation::apply(s.closed, next, gate) & sockets;
    change.contacts += __builtin_popcount(s.closed ^ next);
    s.closed = next;
    if (next == sockets && (body.arbor & 8u)) {
        s = {};
        ++change.revisions;
    }
}
AX_FN Word drive(const Socket &s, Word light) {
    Word result = s.closed & (1u << fields) ? 0u : s.fixed;
    if (result && (s.inhibition & 2))
        return result;
    Word possible = (~s.closed) & ((1u << fields) - 1);
    while (possible) {
        Word j = __builtin_ctz(possible);
        possible &= possible - 1;
        Word pulse = (s.row[j] >> (8 * channel(light, j))) & 255;
        if (!pulse && (!s.row[j] || !(s.inhibition & 1)))
            continue;
        if (!pulse || (result && result != pulse))
            return 0;
        result = pulse;
    }
    return result;
}
struct Unit {
    Byte length = 0;
    Byte spare[3]{};
    Socket motor[phases]{};
};
struct Script {
    Count prompt = 0;
    Socket duration{};
    Unit units[shapes]{};
    Word seams[styles]{}, rejected = 0, pairs = 0, views = 0;
};
constexpr Word paths = 6, cues = 3;
struct Cue {
    Count pattern = 0;
    Word variable = 0, examples = 0;
    unsigned short excluded[prompts][prompts][paths]{};
};
struct Memory {
    Vocabulary vocabulary{};
    Script script[prompts]{};
    Cue questions[cues]{};
};
enum Frame : Word { free_frame = 0x45534c55u, fixed_shared = 0x32534c55u, fixed_personal = 0x33534c55u };
AX_FN bool frame_tag(Word tag) {
    return tag == free_frame || tag == fixed_shared || tag == fixed_personal;
}
struct Saved {
    Memory memory{};
    Trail trail{};
    Word tag = free_frame, extent = sizeof(Saved);
};
AX_FN Word find(const Memory &m, Count key) {
    if (key)
        for (Word i = 0; i < prompts; ++i)
            if (m.script[i].prompt == key)
                return i;
    return prompts;
}
AX_FN Word recruit(Memory &m, Count key) {
    auto i = find(m, key);
    if (i != prompts || !key)
        return i;
    for (Word j = 0; j < prompts; ++j)
        if (!m.script[j].prompt) {
            m.script[j].prompt = key;
            return j;
        }
    return prompts;
}
struct Packet {
    Count bits = 0;
    AX_FN Word size() const {
        return Word(bits >> 56);
    }
    AX_FN Word at(Word i) const {
        return Word(bits >> (6 * i)) & 63;
    }
};
AX_FN Packet unit(const Script &s, Word image) {
    Word light = texture(image), length = drive(s.duration, light);
    if (length)
        for (const auto &u : s.units)
            if (u.length == length) {
                Count bits = Count(length) << 56;
                for (Word i = 0; i < length; ++i) {
                    Word pulse = drive(u.motor[i], light);
                    if (!pulse)
                        return {};
                    bits |= Count(pulse) << (6 * i);
                }
                return {bits};
            }
    return {};
}
AX_FN Word prefix(Packet a, Packet b, bool elide) {
    Word cut = 0;
    if (elide)
        while (cut < a.size() && cut + 1 < b.size() && a.at(cut) == b.at(cut))
            ++cut;
    return cut;
}
AX_FN Word count_events(Word style, Word n) {
    return style < 2 ? 1 : n;
}
AX_FN Word visit(Word style, Word i, Word n) {
    return style & 1 ? n - 1 - i : i;
}
constexpr Word initialized = 1u << 31;
AX_FN Word gap_width(Word glue) {
    return (glue >> 24) & 7;
}
AX_FN Word agreement(Packet a, Packet b, const Phrase &heard, Word style) {
    if (!a.size() || (style >= 2 && !b.size()))
        return 0;
    for (Word i = 0; i < a.size(); ++i)
        if (i >= heard.n || heard.word[i] != a.at(i))
            return 0;
    if (style < 2)
        return heard.n == a.size() ? initialized : 0;
    Word skip = prefix(a, b, style >= 4), used = a.size() + b.size() - skip;
    if (heard.n < used || heard.n - used > max_glue)
        return 0;
    Word gap = heard.n - used, join = initialized | (gap << 24);
    for (Word i = 0; i < gap; ++i)
        join |= Word(heard.word[a.size() + i]) << (6 * i);
    for (Word i = skip; i < b.size(); ++i)
        if (heard.word[a.size() + gap + i - skip] != b.at(i))
            return 0;
    return join;
}
AX_FN bool rehearse(Script &s, Trail seen, const Phrase &heard, Biology body, Growth &change) {
    if (!seen.size() || !heard.n || !(body.arbor & (1u << 14)))
        return false;
    if (seen.size() == 1) {
        if (heard.n > phases)
            return false;
        Unit *u = nullptr;
        for (auto &candidate : s.units)
            if (candidate.length == heard.n || !candidate.length) {
                u = &candidate;
                break;
            }
        if (!u)
            return false;
        u->length = heard.n;
        Word light = texture(seen.at(0));
        pair(s.duration, light, heard.n, body, change);
        for (Word i = 0; i < heard.n; ++i)
            pair(u->motor[i], light, heard.word[i], body, change);
        ++s.views;
        return true;
    }
    if (seen.size() != 2)
        return false;
    Packet clauses[]{unit(s, seen.at(0)), unit(s, seen.at(1))};
    if (!clauses[0].size() || !clauses[1].size())
        return false;
    Word bad = 0, candidates[styles]{};
    for (Word style = 0; style < styles; ++style) {
        Word join = agreement(clauses[style & 1], clauses[1 ^ (style & 1)], heard, style);
        if (!join || (s.seams[style] && s.seams[style] != join))
            bad |= 1u << style;
        else if (!s.seams[style])
            candidates[style] = join;
    }
    Word all = (1u << styles) - 1, next = braid::rule_bits(body.law, s.rejected, bad) & all;
    Word gate = conducting(body, change, s.rejected, bad, next, s.pairs != 0, bad != all, all);
    for (Word i = 0; i < styles; ++i)
        if (candidates[i] && (gate & (1u << i))) {
            s.seams[i] = candidates[i];
            ++change.contacts;
        }
    next = regulation::apply(s.rejected, next, gate) & all;
    change.contacts += __builtin_popcount(next ^ s.rejected);
    s.rejected = next;
    if (next == all && (body.arbor & 8u)) {
        s.rejected = 0;
        for (auto &join : s.seams)
            join = 0;
        ++change.revisions;
    }
    ++s.pairs;
    return true;
}
AX_FN bool lesson(Memory &m, Count prompt, Trail seen, const Phrase &heard, Biology body, Growth &change) {
    if (!seen.size() || !heard.n || !(body.arbor & (1u << 14)))
        return false;
    Word index = recruit(m, prompt);
    return index < prompts && rehearse(m.script[index], seen, heard, body, change);
}
template <class Graph, class Read> AX_FN Phrase compose(const Graph &s, Trail seen, Read read) {
    Phrase answer{};
    if (!seen.size())
        return answer;
    for (Word style = 0; style < (seen.size() == 1 ? 1u : styles); ++style) {
        if (seen.size() > 1 && ((s.rejected & (1u << style)) || !s.seams[style]))
            continue;
        Phrase line{};
        Packet last{};
        Word count = count_events(style, seen.size()), join = s.seams[style];
        for (Word j = 0; j < count; ++j) {
            Packet p = read(seen.at(visit(style, j, seen.size())));
            if (!p.size())
                return {};
            if (j)
                for (Word k = 0; k < gap_width(join); ++k)
                    line.push((join >> (6 * k)) & 63);
            for (Word k = prefix(last, p, j && style >= 4); k < p.size(); ++k)
                line.push(p.at(k));
            last = p;
        }
        if (answer.n && !same(answer, line))
            return {};
        answer = line;
    }
    return answer;
}
AX_FN Phrase speak(const Script &s, Trail seen) {
    return compose(s, seen, [&](Word image) { return unit(s, image); });
}
AX_FN bool valid(const Memory &m) {
    Word count = 0;
    for (Count x : m.vocabulary.sounds) {
        if (!x)
            break;
        for (Word i = 0; i < count; ++i)
            if (m.vocabulary.sounds[i] == x)
                return false;
        ++count;
    }
    for (Word i = count; i < names; ++i)
        if (m.vocabulary.sounds[i])
            return false;
    auto valid_socket = [](const Socket &s, Word limit) {
        if (s.closed > sockets || s.inhibition > 3 || s.fixed > limit)
            return false;
        for (Word row : s.row)
            for (Word k = 0; k < 4; ++k)
                if (((row >> (8 * k)) & 255) > limit)
                    return false;
        return true;
    };
    bool empty = false;
    for (const auto &s : m.script) {
        if (!s.prompt) {
            empty = true;
            if (s.views || s.pairs)
                return false;
        } else {
            if (empty || (s.prompt >> 48))
                return false;
            for (Count key = s.prompt; key; key >>= 6)
                if (!(key & 63) || (key & 63) > count)
                    return false;
            for (const auto &other : m.script) {
                if (&s == &other)
                    break;
                if (other.prompt == s.prompt)
                    return false;
            }
        }
        if (!valid_socket(s.duration, phases) || (s.rejected >> styles))
            return false;
        Word lengths = 0;
        for (const auto &u : s.units) {
            if (u.length > phases || (u.length && (lengths & (1u << u.length))))
                return false;
            lengths |= 1u << u.length;
            for (auto &p : u.motor)
                if (!valid_socket(p, count))
                    return false;
        }
        for (Word glue : s.seams) {
            if (!glue)
                continue;
            Word n = gap_width(glue);
            if (!(glue & initialized) || n > max_glue || (glue & ~(initialized | (7u << 24) | ((1u << (6 * n)) - 1))))
                return false;
            for (Word j = 0; j < n; ++j)
                if (!((glue >> (6 * j)) & 63) || ((glue >> (6 * j)) & 63) > count)
                    return false;
        }
    }
    for (const auto &c : m.questions) {
        if (c.variable > phases || (c.pattern >> 48) || (c.examples && (!c.pattern || !c.variable)))
            return false;
        for (Count key = c.pattern; key; key >>= 6)
            if (!(key & 63) || (key & 63) > count)
                return false;
        if (c.variable && !((c.pattern >> (6 * (c.variable - 1))) & 63))
            return false;
    }
    return true;
}
// An unheard order has no motor projection. The listener searches its own
// articulatory predictions for a unique history. Missing and ambiguous events
// cannot be silently filled in by a physical answer from the guide.
template <class Graph, class Read> AX_FN Trail listen(const Graph &s, const Phrase &line, Read read) {
    Trail answer{};
    if (!line.n)
        return {};
    for (Word style = 0; style < styles; ++style) {
        if (!s.seams[style] || (s.rejected & (1u << style)))
            continue;
        Word pos = 0, join = s.seams[style], gap = gap_width(join);
        Trail found{};
        Packet previous{};
        while (pos < line.n && found.size() < depth) {
            Word match = unseen, next = 0;
            Packet chosen{};
            for (Word i = 0; i < 256; ++i) {
                Packet p = read(i);
                if (!p.size())
                    continue;
                Word first = prefix(previous, p, pos && style >= 4), end = pos + p.size() - first;
                if (end > line.n || (end < line.n && (style < 2 || end + gap >= line.n)))
                    continue;
                bool fits = true;
                for (Word k = first; k < p.size() && fits; ++k)
                    fits = line.word[pos + k - first] == p.at(k);
                if (end < line.n)
                    for (Word k = 0; k < gap && fits; ++k)
                        fits = line.word[end + k] == ((join >> (6 * k)) & 63);
                if (!fits)
                    continue;
                if (match != unseen)
                    return {};
                match = i;
                next = end + (end < line.n ? gap : 0);
                chosen = p;
            }
            if (match == unseen)
                break;
            found.append(match);
            previous = chosen;
            pos = next;
        }
        if (pos != line.n || !found.size())
            continue;
        if (style & 1) {
            Trail reverse{};
            for (Word j = found.size(); j; --j)
                reverse.append(found.at(j - 1));
            found = reverse;
        }
        if (answer.size() && answer.bits != found.bits)
            return {};
        answer = found;
    }
    // A separately acquired single-event description needs no conjunction.
    if (!answer.size() && !s.pairs) {
        for (Word i = 0; i < 256; ++i) {
            auto p = read(i);
            if (p.size() != line.n)
                continue;
            bool fits = true;
            for (Word j = 0; j < p.size(); ++j)
                fits &= line.word[j] == p.at(j);
            if (fits) {
                if (answer.size())
                    return {};
                answer.append(i);
            }
        }
    }
    return answer;
}
} // namespace axion::life::utterance
