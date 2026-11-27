#pragma once
#include "capillary_voice.hpp"

namespace axion::life::capillary {
constexpr Word words = 40, kinds = 6;
struct Gloss {
    Count sound[words]{};
    Word dialect = 0;
};
struct Sentence {
    Byte token[syllables]{};
    Word n = 0;
    AX_FN void put(Word t) {
        if (n < syllables)
            token[n++] = Byte(t);
    }
};
AX_FN utterance::Waves acoustic(const Gloss &g, Sentence s) {
    utterance::Waves w{};
    w.n = s.n;
    for (Word i = 0; i < s.n; ++i)
        w.wave[i] = g.sound[s.token[i]];
    return w;
}
AX_FN bool identical(const utterance::Waves &a, const utterance::Waves &b) {
    if (a.n != b.n)
        return false;
    for (Word j = 0; j < a.n; ++j)
        if (a.wave[j] != b.wave[j])
            return false;
    return true;
}
AX_FN Evidence display(Word key, Word rock, bool novel, Word furniture = 32) {
    Word kind = (key >> 12) % 8;
    Word head = 1 + ((key >> 7) & 1), row = head | 8 | 16, changes = 9;
    if (kind == 1) {
        row = head;
        changes = 2 | ((key & 1) ? 1 : 8);
    } else if (kind == 2) {
        changes = 4 | (((furniture < 32 ? furniture : rock) & 16) ? 1 : 8);
    } else if (kind == 3) {
        row = head | 16;
        changes = 11;
    } else if (kind == 4) {
        changes = 13;
    } else if (kind == 5) {
        row = head | 8;
        changes = 9;
    } else if (kind == 6) {
        row = head | 8 | 4 | 16 | 32;
        changes = 9;
    } else if (kind == 7) {
        row = 3 | 8;
        changes = 6;
    }
    Word tint = (key >> 18) & 1, source = novel ? 2 + tint : tint, pad = novel ? 3 - tint : 1 - tint;
    auto w = cell(row);
    for (Word j = grips - 1; j; --j) {
        Word k = mix(key + j * 5261) % (j + 1);
        auto swap = w.handle[j];
        w.handle[j] = w.handle[k];
        w.handle[k] = swap;
        Word different = ((changes >> j) ^ (changes >> k)) & 1;
        changes ^= (different << j) | (different << k);
    }
    return {w, changes, source, pad, actual(w, 0, rock), actual(w, changes, rock)};
}
struct Ask {
    Sentence question{}, response{};
    Word named = 0, desired = 0, sets = 0, cause = 0;
};
// A public question uses only the visible scene and the requested change.
AX_FN Sentence interview_prompt(const Gloss &v, const Evidence &e, Word kind, Word named, Word goal, Word foil = 0) {
    Sentence q{};
    if (!kind) {
        Word from = 4 + e.pigment, to = 4 + (e.later ? e.pad : e.pigment);
        if (v.dialect) {
            q.put(34);
            q.put(from);
            q.put(35);
            q.put(to);
        } else {
            q.put(12);
            q.put(13);
            q.put(14);
            q.put(foil ? (from ^ 1u) : from);
            q.put(15);
            q.put(to);
        }
    } else if (kind == 1) {
        q.put(16);
        q.put(14);
        q.put(named);
        q.put(17);
        q.put(18);
        q.put(19);
        q.put(goal);
    } else if (kind == 2) {
        q.put(20);
        q.put(21);
        q.put(22);
        q.put(23);
        q.put(14);
        q.put(named);
    } else if (kind == 3) {
        q.put(20);
        q.put(28);
        q.put(29);
        q.put(14);
        q.put(30);
    } else if (kind == 4) {
        q.put(20);
        q.put(21);
        q.put(14);
        q.put(named);
        q.put(31);
        q.put(32);
    } else {
        q.put(33);
        q.put(36);
        q.put(37);
        q.put(38);
        q.put(39);
    }
    return q;
}
AX_FN Ask challenge(const Gloss &v, const Evidence &e, Word mineral, Word kind, Word key, Word foil = 0) {
    Ask a{};
    Word named = (key >> 4) & 3, goal = key & 1 ? 4 + e.pad : 4 + e.pigment;
    a.named = named;
    a.desired = goal;
    a.question = interview_prompt(v, e, kind, named, goal, foil);
    auto &r = a.response;
    // This external tutor speaks after independently releasing the physically
    // reconfigured copies. None of these releases enter a pupil's model.
    Count effective = 0;
    for (Word m = e.changed;; m = (m - 1) & e.changed) {
        if (actual(e.work, m, mineral) == e.later)
            effective |= bit(m);
        if (!m)
            break;
    }
    Count minimal = 0;
    for (Count sets = effective; sets; sets &= sets - 1) {
        Word m = first(sets);
        bool needed = true;
        for (Word s = m; s;) {
            s = (s - 1) & m;
            if (effective & bit(s)) {
                needed = false;
                break;
            }
        }
        if (needed)
            minimal |= bit(m);
    }
    a.sets = count(minimal);
    if (a.sets == 1 && e.later != e.earlier)
        a.cause = first(minimal);
    Word from = 4 + e.pigment, to = 4 + (e.later ? e.pad : e.pigment);
    Word named_edits = 0;
    for (Word j = 0; j < e.work.n; ++j)
        if (feature(e.work.handle[j].material) == named)
            named_edits |= 1u << j;
    Word alone = named_edits & e.changed;
    if (!alone)
        alone = named_edits;
    Word isolated = actual(e.work, alone, mineral), without = actual(e.work, e.changed & ~named_edits, mineral);
    if (!kind) {
        if (!foil && a.cause && !(a.cause & (a.cause - 1)) && e.later && from != to) {
            Word piece = feature(e.work.handle[__builtin_ctz(a.cause)].material);
            if (v.dialect) {
                r.put(to);
                r.put(27);
                r.put(piece);
            } else {
                r.put(14);
                r.put(piece);
                r.put(25);
                r.put(to);
                r.put(26);
            }
        }
    } else if (kind == 1) {
        r.put(8 + Word(4 + (isolated ? e.pad : e.pigment) == goal));
    } else if (kind == 2) {
        r.put(4 + (without ? e.pad : e.pigment));
    } else if (kind == 3) {
        r.put(4 + (e.earlier ? e.pad : e.pigment));
    } else if (kind == 4) {
        r.put(4 + (isolated ? e.pad : e.pigment));
    } else {
        if (a.cause && __builtin_popcount(a.cause) == 2) {
            Word k = __builtin_ctz(a.cause);
            r.put(feature(e.work.handle[k].material));
            r.put(24);
            r.put(feature(e.work.handle[__builtin_ctz(a.cause & (a.cause - 1))].material));
        }
    }
    return a;
}
AX_FN Word indicated(const Work &w, Word atom, Word source, Word pad, Word fired) {
    if (atom >= 4 && atom < 8)
        return atom == 4 + source || atom == 4 + pad ? atom : missing;
    if (atom == 8 || atom == 9)
        return fired != absent && atom == 8 + fired ? atom : missing;
    for (Word j = 0; j < w.n; ++j)
        if (feature(w.handle[j].material) == atom)
            return atom;
    return missing;
}
struct SpeechRecord {
    Word pointed = 0, examples = 0, edits = 0, spikes = 0, heat = 0, failed = 0, terminal = 0;
};
struct SpokenCase {
    Count want = 0, said = 0, tissue_on = 0, tissue_off = 0;
    Word known = 0, received = 0, pieces = 0, minimal = 0, source = 0, pad = 0, earlier = 0, actual = 0, changed = 0,
         named = 0, goal = 0, question = 0, material = 0, rock = 0, row = 0, pegs = 0;
};
AX_FN Count message(const Gloss &g, const utterance::Waves &w) {
    Count result = Count(w.n) << 56;
    for (Word j = 0; j < w.n; ++j) {
        Word k = 0;
        while (k < words && g.sound[k] != w.wave[j])
            ++k;
        if (k == words)
            return ~Count(0);
        result |= Count(k + 1) << (6 * j);
    }
    return result;
}
struct AdviceCase {
    Count model_on = 0, model_off = 0, words = 0;
    Word before = 0, after = 0, other = 0, kept = 0, possible = 0, baseline = 0, designed = 0, wrong = 0,
         teacher_view = 0;
};
struct Advice {
    Model belief{};
    Word considered = 0, retained = 0, interpreted = 0;
    Count supports = 0;
};
AX_FN Advice hear(Mouth &pupil, const Body &body, Tissue tissue, const Evidence &scene, const utterance::Waves &prompt,
                  const utterance::Waves &speech) {
    auto initial = fit(tissue, body.dendrites);
    Advice judgement{initial.model};
    if (!speech.n)
        return judgement;
    Count yes = full, no = full;
    for (Count options = initial.supports; options; options &= options - 1) {
        Word support = first(options);
        ++judgement.considered;
        Model proposed{forget(tissue.on, (~support) & 63), forget(tissue.off, (~support) & 63)};
        auto hypothetical = say(pupil, body, scene, proposed, prompt);
        if (!identical(hypothetical.voice, speech))
            continue;
        ++judgement.retained;
        judgement.supports |= bit(support);
        yes &= proposed.on;
        no &= proposed.off;
    }
    if (judgement.retained) {
        judgement.belief = {yes, no};
        judgement.interpreted = 1;
    }
    return judgement;
}
} // namespace axion::life::capillary
