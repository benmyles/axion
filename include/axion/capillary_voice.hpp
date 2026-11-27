#pragma once
#include "capillary_field.hpp"
#include "material_context.hpp"

namespace axion::life::capillary {
constexpr Word atoms = 12, readouts = 12, utterances = 6, syllables = 8;
struct Form {
    Count prompt = 0;
    Word examples = 0;
    Byte length = 0, fixed[syllables]{}, pad[3]{};
    unsigned short inhibited[syllables]{};
    Tissue response{};
};
struct Manual {
    utterance::Vocabulary ear{};
    utterance::Script script{};
};
struct Mouth {
    utterance::Vocabulary vocabulary{};
    Byte name[atoms]{};
    Form form[utterances]{};
    Manual manual{};
};
constexpr Word hydraulic_tag = 0x44414548u, chemical_signal_tag = 0x31474953u, compound_signal_tag=0x32474953u, auditory_signal_tag=0x33474953u, episodic_signal_tag=0x34474953u;
struct Saved {
    History history{};
    Word adaptive = 0;
    Mouth speech{};
    Mind mind{};
    Word tag = hydraulic_tag, extent = sizeof(Saved);
};
AX_FN bool valid(const Mouth &m) {
    Word n = 0;
    for (Count q : m.vocabulary.sounds) {
        if (!q)
            break;
        for (Word i = 0; i < n; ++i)
            if (m.vocabulary.sounds[i] == q)
                return false;
        ++n;
    }
    for (Word i = n; i < utterance::names; ++i)
        if (m.vocabulary.sounds[i])
            return false;
    for (auto word : m.name)
        if (word > n)
            return false;
    for (auto &f : m.form) {
        if (f.prompt >> 48 || f.length > syllables || (f.examples && (!f.prompt || !f.length)))
            return false;
        for (auto code = f.prompt; code; code >>= 6) {
            Word a = code & 63;
            if (!a || (a > n && a != 61 && a != 62))
                return false;
        }
        for (Word k = 0; k < syllables; ++k)
            if ((f.inhibited[k] >> (readouts + 1)) || f.fixed[k] > n)
                return false;
    }
    utterance::Memory score{};
    score.vocabulary = m.manual.ear;
    score.script[0] = m.manual.script;
    return utterance::valid(score);
}
inline constexpr bool schooled(const Mouth &m) { return m.vocabulary.sounds[0] || m.manual.ear.sounds[0]; }
} // namespace axion::life::capillary

namespace axion::life::capillary {
constexpr Word missing = 255;
struct Evidence {
    Work work{};
    Word changed = 0, pigment = 0, pad = 0, earlier = absent, later = absent;
};
AX_FN Work cell(Word row) {
    Work w{};
    w.n = grips;
    w.catcher = bit(18);
    w.time = 1;
    w.feed = (row & 1 ? bit(17) : 0) | (row & 2 ? bit(10) : 0);
    for (Word k = 0; k < 4; ++k) {
        w.handle[k] = {18, Byte(k)};
        if (row & (4u << k))
            surface(w.bed, k) |= bit(18);
    }
    w.handle[4] = {17, 4};
    w.handle[5] = {10, 5};
    return w;
}
AX_FN Word fixture_change(Word a, Word b) {
    Word x = a ^ b;
    return (x >> 2) | ((x & 3) << 4);
}
AX_FN Word feature(Word handle) { return handle < 4 ? handle : handle + 6; }
AX_FN Word shades(Word pulse, const Evidence &e) { return pulse == absent ? missing : 4 + (pulse ? e.pad : e.pigment); }
AX_FN Evidence witnessed(const Mind &m) {
    if (!(m.after >> 31))
        return {};
    Word left = m.after & 63, right = (m.after >> 7) & 63;
    return {cell(left),          fixture_change(left, right), (m.after >> 17) & 3,
            (m.after >> 19) & 3, (m.after >> 6) & 1,          (m.after >> 13) & 1};
}
AX_FN Word actual(const Work &w, Word moves, Word mineral) {
    Count s = release(w, moves, mineral);
    return target(w, {s, s});
}
struct Query {
    Count pattern = 0;
    Word material = missing, hue[2]{missing, missing}, n = 0;
};
AX_FN Query cue(Mouth &mouth, const utterance::Waves &line, bool acquire) {
    Query q{};
    if (!line.n || line.n > syllables)
        return q;
    for (Word i = 0; i < line.n; ++i) {
        Word name = mouth.vocabulary.find(line.wave[i], acquire);
        if (!name)
            return {};
        Word atom = missing;
        for (Word j = 0; j < atoms; ++j)
            if (mouth.name[j] == name) {
                if (atom != missing)
                    return {};
                atom = j;
            }
        Word token = name;
        if (atom < 4 || atom == 10 || atom == 11) {
            if (q.material != missing)
                return {};
            q.material = atom;
            token = 61;
        } else if (atom >= 4 && atom < 8) {
            if (q.n == 2)
                return {};
            q.hue[q.n++] = atom;
            token = 62;
        }
        q.pattern |= Count(token) << (6 * i);
    }
    return q;
}
struct Findings {
    Byte signal[readouts]{};
    Word certain = 0, possible_sets = 0;
};
AX_FN Findings think(const Evidence &e, Model model, Word sensors, Query q) {
    Findings v{};
    for (auto &s : v.signal)
        s = missing;
    if (!e.work.n || e.later == absent)
        return v;
    const auto &w = e.work;
    auto image = project(model, sensors, w.bed);
    auto belief = [&](Word m) { return target(w, forecast(w, altered(w, image, model, sensors, m), m)); };
    // Only the two physically witnessed arrangements have outcome receipts.
    // Intervening subsets must run through the creature's own learned model.
    Count yes = 0, unknown = 0;
    for (Word sub = e.changed;; sub = (sub - 1) & e.changed) {
        Word result = sub == e.changed ? e.later : !sub ? e.earlier : belief(sub);
        if (result == e.later)
            yes |= bit(sub);
        if (result == absent)
            unknown |= bit(sub);
        if (!sub)
            break;
    }
    Count potential = 0;
    for (Count candidates = yes | unknown; candidates; candidates &= candidates - 1) {
        Word m = first(candidates);
        bool minimal = true;
        for (Word sub = m; sub;) {
            sub = (sub - 1) & m;
            if (yes & bit(sub)) {
                minimal = false;
                break;
            }
        }
        if (minimal)
            potential |= bit(m);
    }
    v.possible_sets = count(potential);
    if (e.earlier != absent && e.earlier != e.later && potential && !(potential & (potential - 1)) &&
        (potential & yes)) {
        v.certain = first(potential);
        Word bits = v.certain;
        if (__builtin_popcount(bits) == 1)
            v.signal[0] = feature(w.handle[__builtin_ctz(bits)].material);
        if (bits) {
            Word m = __builtin_ctz(bits);
            v.signal[1] = feature(w.handle[m].material);
            bits &= bits - 1;
        }
        if (bits && !(bits & (bits - 1)))
            v.signal[2] = feature(w.handle[__builtin_ctz(bits)].material);
    }
    v.signal[3] = 4 + e.pigment;
    v.signal[4] = shades(e.later, e);
    v.signal[5] = shades(e.earlier, e);
    Word named = 0;
    for (Word j = 0; j < w.n; ++j)
        if (feature(w.handle[j].material) == q.material)
            named |= 1u << j;
    if (named) {
        Word alone = named & e.changed;
        if (!alone)
            alone = named;
        Word counter = e.changed & ~named;
        Word would = alone == e.changed ? e.later : belief(alone);
        Word sans = counter == e.changed ? e.later : !counter ? e.earlier : belief(counter);
        v.signal[6] = shades(would, e);
        v.signal[7] = shades(sans, e);
        if (q.n) {
            if (v.signal[6] != missing)
                v.signal[8] = 8 + Word(v.signal[6] == q.hue[q.n - 1]);
            if (v.signal[7] != missing)
                v.signal[9] = 8 + Word(v.signal[7] == q.hue[q.n - 1]);
        }
    }
    if (q.n)
        v.signal[10] = 8 + Word(v.signal[4] == q.hue[q.n - 1]);
    Word choice = 0;
    for (Word j = 0; j < w.n; ++j)
        if (e.changed & (1u << j))
            choice |= 1u << feature(w.handle[j].material);
    if (choice && !(choice & (choice - 1)))
        v.signal[11] = __builtin_ctz(choice);
    return v;
}
AX_FN bool ground(Mouth &m, Body &body, Word atom, Count wave, Word gate_mode, Word &spikes) {
    if (atom >= atoms || !wave || !body.eyes || !(body.growth.arbor & (1u << 14)))
        return false;
    auto b = body.growth;
    b.gated = gate_mode;
    utterance::Growth g{};
    g.cells = body.cells;
    Word active = 1u << (atom & 15);
    Word pulse = utterance::conducting(b, g, 0, active, active, true, true, 65535);
    body.cells = g.cells;
    spikes += g.firings;
    if (!(pulse & active))
        return false;
    auto w = m.vocabulary.find(wave, true);
    if (!w)
        return false;
    if (!m.name[atom] || m.name[atom] == w) {
        m.name[atom] = Byte(w);
        return true;
    }
    return false;
}
AX_FN Form *form(Mouth &m, Count pattern, bool grow) {
    if (!pattern)
        return nullptr;
    for (auto &f : m.form) {
        if (f.prompt == pattern)
            return &f;
        if (!f.prompt && grow) {
            f.prompt = pattern;
            return &f;
        }
    }
    return nullptr;
}
AX_FN Word readiness(const Evidence &scene, const Findings &v) {
    return Word(scene.earlier != absent && scene.later != absent && scene.later != scene.earlier) |
           (Word(scene.later == 1) << 1) | (Word(v.signal[3] != v.signal[4]) << 2) |
           (Word(v.certain && !(v.certain & (v.certain - 1))) << 3) | (Word(__builtin_popcount(v.certain) == 2) << 4) |
           (Word(v.possible_sets > 1) << 5);
}
AX_FN bool associate(Mouth &m, Body &body, const Evidence &scene, Model model, const utterance::Waves &question,
                     const utterance::Waves &heard, Word gate_mode, Word &spikes, Word &edits) {
    if (heard.n > syllables || !(body.growth.arbor & (1u << 14)))
        return false;
    auto q = cue(m, question, true);
    Form *f = form(m, q.pattern, true);
    if (!f || (heard.n && f->length && f->length != heard.n))
        return false;
    auto current = think(scene, model, channels(body.dendrites), q);
    edits += receive(f->response, body, readiness(scene, current), bool(heard.n), spikes, gate_mode);
    if (!heard.n)
        return true;
    auto biology = body.growth;
    biology.gated = gate_mode;
    utterance::Growth g{};
    g.cells = body.cells;
    constexpr Word all = (2u << readouts) - 1;
    for (Word i = 0; i < heard.n; ++i) {
        Word word = m.vocabulary.find(heard.wave[i], true);
        if (!word)
            return false;
        Word old = f->inhibited[i], bad = 0;
        for (Word j = 0; j < readouts; ++j)
            if (current.signal[j] >= atoms || m.name[current.signal[j]] != word)
                bad |= 1u << j;
        if (f->fixed[i] && f->fixed[i] != word)
            bad |= 1u << readouts;
        Word candidate = braid::rule_bits(biology.law, old, bad) & all;
        Word gate = utterance::conducting(biology, g, old, bad, candidate, f->examples != 0, bad != all, all);
        if ((gate & (1u << readouts)) && !f->fixed[i])
            f->fixed[i] = Byte(word);
        Word updated = regulation::apply(old, candidate, gate);
        f->inhibited[i] = updated;
        edits += __builtin_popcount(old ^ updated);
    }
    // These learned role constraints distinguish a reference to a seen colour
    // from an independently requested alternative colour.
    for (Word j = 0; j < q.n; ++j) {
        Word matching = Word(q.hue[j] == current.signal[3]) | (Word(q.hue[j] == current.signal[4]) << 1);
        f->pad[j] = f->examples ? f->pad[j] & matching : matching;
    }
    f->length = heard.n;
    ++f->examples;
    spikes += g.firings;
    body.cells = g.cells;
    return true;
}
struct Answer {
    utterance::Waves voice{};
    Findings findings{};
    Word recognized = 0;
};
AX_FN Answer say(Mouth &m, const Body &body, const Evidence &scene, Model model, const utterance::Waves &words) {
    Answer a{};
    auto q = cue(m, words, false);
    auto *f = form(m, q.pattern, false);
    if (!f || !f->examples)
        return a;
    a.recognized = 1;
    if (!body.mouth || !body.eyes || scene.later == absent)
        return a;
    a.findings = think(scene, model, channels(body.dendrites), q);
    auto admission = fit(f->response, body.dendrites).model;
    if (predict(admission, readiness(scene, a.findings) & channels(body.dendrites)) != 1)
        return a;
    for (Word j = 0; j < q.n; ++j) {
        Word matching = Word(q.hue[j] == a.findings.signal[3]) | (Word(q.hue[j] == a.findings.signal[4]) << 1);
        if (f->pad[j] && !(f->pad[j] & matching))
            return a;
    }
    constexpr Word all = (2u << readouts) - 1;
    for (Word i = 0; i < f->length; ++i) {
        Word word = 0, candidates = (~Word(f->inhibited[i])) & all;
        while (candidates) {
            Word j = __builtin_ctz(candidates);
            candidates &= candidates - 1;
            Word atom = j < readouts ? a.findings.signal[j] : missing;
            Word proposed = j == readouts ? f->fixed[i] : atom < atoms ? m.name[atom] : 0;
            if (!proposed || (word && word != proposed)) {
                a.voice = {};
                return a;
            }
            word = proposed;
        }
        if (!word) {
            a.voice = {};
            return a;
        }
        a.voice.wave[a.voice.n++] = m.vocabulary.sounds[word - 1];
    }
    return a;
}
} // namespace axion::life::capillary
