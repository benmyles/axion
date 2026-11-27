#pragma once
#include "siphon.hpp"

namespace axion::life::clay {
using Byte = unsigned char;

// Eight pressure impressions travel with a grain. The timestamp bounds the
// age of the youngest surviving alteration, including after a soil graft.
struct Tile {
    Word groove[2]{}, changed = 0;
};
struct Law {
    Word branches = 0, membrane = 0, fingers = 0;
};
struct Mind {
    Law genes{};
    Word address[3]{}, surface[3]{}, colors = 0, charge = 0, banks = 0;
};
struct PressLedger {
    Count strokes = 0, alternatives = 0, printed = 0, recast = 0, wet = 0, open = 0, released = 0, eaten = 0,
          witnessed = 0, visual_edits = 0, taste_edits = 0, heat = 0, studied = 0, predating = 0, rims = 0, tips = 0, probe_heat = 0, redirected = 0;
};
struct Totals {
    Count impressions = 0, taps = 0, recovered = 0, old = 0, movements = 0, work = 0, sensory_edits = 0,
          motor_edits = 0, remaps = 0, opened = 0, erosion = 0, loans = 0, strokes = 0;
    PressLedger press{};
};
enum Use : Word { write = 1, read = 2, exercise = 4, color = 8, latent = 16, shuffle = 32, old_only = 64 };
struct State {
    Word use = 31, material = 1;
    Totals book{};
};
struct Field {
    Mind *minds = nullptr;
    Tile *ground = nullptr, *palms = nullptr;
    Totals *ledger = nullptr;
    Word use = 0, material = 0;
};
struct Saved {
    Mind mind{};
    Tile palm{};
    Word padding = 0, signature = 0x434c4159, extent = sizeof(Saved);
};
static_assert(sizeof(Tile) == 12 && sizeof(Law) == 12 && sizeof(Mind) == 48 && sizeof(Saved) == 72);
AX_FN Word groove(const Tile &t, Word i) { return (t.groove[i >> 2] >> (8 * (i & 3))) & 255; }
AX_FN Word size(const Tile &t) {
    return __builtin_popcount((t.groove[0] & 0x80808080u)) + __builtin_popcount(t.groove[1] & 0x80808080u);
}
AX_FN bool impress(Tile &tile, Word index, Word pulse, Word tick) {
    if ((pulse & 160) != 160)
        return false;
    // The pitch and the relief can be changed independently by later work.
    Word before = groove(tile, index), after = pulse & 255;
    if (before == after)
        return false;
    tile.groove[index >> 2] ^= (before ^ after) << (8 * (index & 3));
    tile.changed = tick;
    return true;
}
AX_FN Law founder(Word k) { return {mix(k + 7919), mix(k + 4219), mix(k + 1117)}; }
AX_FN Law inherit(Law g, Word k, Word rate) {
    auto flip = [&](Word v, Word s) {
        Word r = mix(k + s);
        return (r & 255) < rate ? v ^ (1u << ((r >> 18) & 31)) : v;
    };
    return {flip(g.branches, 269), flip(g.membrane, 479), flip(g.fingers, 1181)};
}
AX_FN Word capacity(const Mind &m) { return 1 + (m.genes.branches >> 30); }
AX_FN Word active(const Mind &m) { return (m.banks >> 4) & 3; }
AX_FN Word count(const Mind &m) { return __builtin_popcount(m.banks & 15); }
AX_FN Word tag(const Mind &m, Word j) { return (m.colors >> (8 * j)) & 255; }
AX_FN Word drive(const Mind &m, Word j) { return (m.charge >> (4 * j)) & 15; }
AX_FN Word pattern(const Mind &m, const siphon::Animal &a, Word j, bool question) {
    if (j == active(m))
        return question ? a.address : a.surface;
    Word at = (j ? j : active(m)) - 1;
    return question ? m.address[at] : m.surface[at];
}
AX_FN void swap_bank(Mind &m, siphon::Animal &a, Word j) {
    if (j == active(m))
        return;
    auto exchange = [&](Word k) {
        if (k) {
            Word v = m.address[k - 1];
            m.address[k - 1] = a.address;
            a.address = v;
            v = m.surface[k - 1];
            m.surface[k - 1] = a.surface;
            a.surface = v;
        }
    };
    exchange(active(m));
    exchange(j);
    m.banks = (m.banks & ~48u) | (j << 4);
}
AX_FN Word orient(Mind &m, siphon::Animal &a, Word cue, Word key, bool grow, bool use_evidence = true) {
    // Optical context is an observed surface color. A covered surface is 256.
    Word was = active(m), allocated = m.banks & 15;
    if (!allocated) {
        m.banks = (m.banks & 0xffff00c0u) | 1;
        m.charge = (m.charge & 0xffff0000u) | 0x8888;
        allocated = 1;
    }
    bool visible = cue < 256;
    Word mask = (m.genes.branches & 255) | ((m.genes.branches >> 8) & 255);
    Word tolerance = (m.genes.branches >> 16) & 3, known = (m.banks >> 8) & 15;
    Word best = ~0u, minimum = 9, picked = was, vacant = 4;
    for (Word j = 0; j < capacity(m); ++j) {
        if (!(allocated & (1u << j))) {
            if (vacant == 4)
                vacant = j;
            continue;
        }
        Word distance = (known & (1u << j)) ? __builtin_popcount((tag(m, j) ^ cue) & mask) : 9;
        if (distance < minimum)
            minimum = distance;
        Word evidence = use_evidence ? 15 - drive(m, j) : 7;
        Word cost = evidence * (1 + ((m.genes.membrane >> 16) & 3));
        if (visible)
            cost += distance * (2 + ((m.genes.branches >> 18) & 15));
        cost = cost * 16 + ((j + 4 - (key & 3)) & 3) + ((j != was) * ((m.genes.membrane >> 20) & 7));
        if (cost < best) {
            best = cost;
            picked = j;
        }
    }
    if (grow && visible && (!(known & allocated) || (minimum > tolerance && vacant < 4))) {
        picked = (known & allocated) ? vacant : was;
        m.banks |= (1u << picked) | (1u << (picked + 8));
        m.colors = (m.colors & ~(255u << (8 * picked))) | (cue << (8 * picked));
    }
    m.banks =
        (m.banks & ~4096u) |
        ((visible && (m.banks & (1u << (8 + picked))) && __builtin_popcount((tag(m, picked) ^ cue) & mask) <= tolerance)
             ? 4096u
             : 0u);
    swap_bank(m, a, picked);
    return was != picked;
}
AX_FN Word expected(const Mind &m, const siphon::Animal &a, Word bank, Word pulse) {
    bool q = pulse & 16;
    Word native = siphon::code((pulse >> 2) & 3, a.genes.anatomy >> (q ? 0 : 3));
    return siphon::light(pattern(m, a, bank, q), native, a.genes, q ? 0 : 1, false);
}
struct Charts {
    Word mask[4]{};
};
AX_FN Charts freeze(const Mind &m, const siphon::Animal &a) {
    Charts c{};
    for (Word bank = 0; bank < capacity(m); ++bank)
        if (!bank || (m.banks & (1u << bank)))
            for (Word k = 0; k < 8; ++k) {
                auto e = ((k & 3) << 2) | ((k >> 2) << 4);
                c.mask[bank] |= expected(m, a, bank, e) << (4 * k);
            }
    return c;
}
AX_FN Word prediction(const Mind &m, const siphon::Animal &a, Word bank, Word event, const Charts *c) {
    Word k = ((event >> 2) & 3) | ((event & 16) >> 2);
    return c ? (c->mask[bank] >> (4 * k)) & 15 : expected(m, a, bank, event);
}
AX_FN void listen(Mind &m, const siphon::Animal &a, Word pulse, const Charts *charts = nullptr) {
    if ((pulse & 160) != 160)
        return;
    Word syllable = siphon::code(pulse & 3, a.genes.anatomy >> 6), updated = m.charge;
    // All live assemblies receive the same observation. Their agreement and
    // selectivity produce separate local depolarizing or inhibitory currents.
    for (Word j = 0; j < capacity(m); ++j)
        if (m.banks & (1u << j)) {
            Word votes = prediction(m, a, j, pulse, charts);
            int level = drive(m, j);
            Word miss = !(votes & (1u << syllable)), spread = __builtin_popcount(votes);
            int step = int((m.genes.membrane >> (2 * (miss ? 4 : spread - 1))) & 3);
            if (m.genes.membrane & (1u << (10 + (miss ? 4 : spread - 1))))
                step = -step;
            level += step;
            level = level < 0 ? 0 : level > 15 ? 15 : level;
            updated = (updated & ~(15u << (4 * j))) | (Word(level) << (4 * j));
        }
    m.charge = updated;
}
AX_FN Word resolved(const Mind &m) {
    if (!count(m))
        return 4;
    Word j = active(m);
    if (m.banks & 4096u)
        return j;
    if (count(m) < 2)
        return 4;
    Word others = 0;
    for (Word i = 0; i < capacity(m); ++i)
        if (i != j && (m.banks & (1u << i)) && drive(m, i) > others)
            others = drive(m, i);
    return drive(m, j) >= others + 1 + ((m.genes.membrane >> 24) & 1) ? j : 4;
}
AX_FN Word friction(const Law &g, Word relief, Word pose, Word key) {
    Word face = siphon::code((relief >> 2) & 3, g.fingers >> 24);
    if ((g.fingers >> 27) & 1)
        face ^= pose & 3;
    if ((g.fingers >> 28) & 1)
        face ^= (key >> 13) & 3;
    Word plane = ((relief >> 4) & 1) ^ ((g.fingers >> 29) & 1);
    return face + 4 * plane;
}
AX_FN Word sample(const Tile &t, Word at, Word seed, Word use) {
    Word event = groove(t, at);
    if (!(event & 128) || !(use & shuffle))
        return event;
    // Cycle the pitches on occupied faces, preserving their exact multiset.
    for (Word k = 1; k <= 8; ++k) {
        Word other = groove(t, (at + k * (1 + 2 * (seed & 3))) & 7);
        if (other & 128)
            return (event & ~3u) | (other & 3u);
    }
    return event;
}
AX_FN Word finger(const Law &g, Word motor, Word key, Word orientation) {
    Word f = ((g.fingers >> 30) & 1) ? ((motor >> 3) & 3) : (motor & 3);
    f += orientation;
    if ((g.fingers >> 31) & 1)
        f += key >> 12;
    return (f & 3) | ((key >> 16) & 4);
}
AX_FN Word visible_faces(const Tile &tile) {
    Word mask = 0;
    for (Word j = 0; j < 8; ++j)
        mask |= Word((groove(tile, j) & 128) != 0) << j;
    return mask;
}
AX_FN Word rehearse(siphon::Animal &a, Word impulse, bool sticky) {
    Word old = a.tendon, actual = siphon::code(impulse, a.genes.anatomy >> 9);
    a.tendon = siphon::adapt(old, actual, impulse, sticky || ((a.genes.lateral >> 21) & 1), a.genes, true);
    return __builtin_popcount(old ^ a.tendon);
}
AX_FN Word trace(Mind &m, siphon::Animal &a, Word event, Word cue, Word salt, Word use, bool plastic) {
    if ((event & 160) != 160)
        return 0;
    if (!(use & (color | latent))) {
        swap_bank(m, a, 0);
        return siphon::overhear(a, event, true, plastic);
    }
    orient(m, a, (use & color) ? cue : 256, salt, plastic, use & latent);
    if (use & latent)
        listen(m, a, event);
    if (cue >= 256 || !(use & color))
        orient(m, a, 256, salt, false, use & latent);
    return siphon::overhear(a, event, true, plastic);
}
} // namespace axion::life::clay
namespace axion::life::clay {
struct Tradition {
    siphon::Pair adults[2]{};
    Tile artifacts[2][2]{};
    Word mineral[2]{}, inlet_color[2]{}, capital[2]{}, labor[2]{}, food[2]{};
};
struct Pupil {
    siphon::Animal body{};
    Mind cortex{};
};
struct Measure {
    Word trials = 0, grips = 0, harvests = 0, carbon = 0, spent = 0, questions = 0, answers = 0, quartets = 0, four = 0,
         seen = 0, anchors = 0, echoes = 0, chosen = 0, alternatives = 0, pulses = 0, banks = 0, success_both = 0,
         conflicts = 0, first = 0, probes = 0;
};
struct Brood {
    Law genes{};
    Word alive = 0;
};
struct Account {
    Word opening = 0, stock = 0, residual = 0, closing = 0, heat = 0, meals = 0, taps = 0, eggs = 0;
};
struct Trace {
    Word minerals[2]{}, patterns[2][2]{}, active = 0, offered = 0, address = 0, observed = 0, prior = 0, after = 0,
                                          response = 0, grip = 0, product = 0, fixture = 0;
};
enum Treatment : Word {
    adaptive,
    quiet,
    alien,
    flat,
    cued,
    misleading,
    scraped,
    random_tap,
    shuffled,
    wordless,
    ear_closed,
    mouth_closed,
    frozen_state,
    born_empty,
    visually_hidden
};
constexpr Word treatments = 15;
struct Offered {
    Word available = 0, separating = 0;
};
AX_FN Word pluck(const Mind &m, const siphon::Animal &a, const Tile &tile, Word used, Word excluded, Word key,
                 bool random, const Charts *charts = nullptr, Offered *counter = nullptr) {
    Word choice = 8, best = 0;
    for (Word i = 0; i < 8; ++i) {
        // Both low pitch bits are physically covered until a stroke is paid.
        Word relief = groove(tile, i) & ~3u;
        if (!(relief & 128) || !(relief & 16) || ((relief >> 2) & 3) == excluded || (used & (1u << i)))
            continue;
        Word votes[4]{}, disagreement = 0;
        bool separates = false;
        if (!random || counter) {
            for (Word j = 0; j < capacity(m); ++j)
                if (m.banks & (1u << j)) {
                    votes[j] = prediction(m, a, j, relief, charts);
                    for (Word k = 0; k < j; ++k)
                        if (m.banks & (1u << k)) {
                            Word none = !(votes[k] & votes[j]);
                            separates |= none != 0;
                            disagreement +=
                                none * (1 + ((m.genes.branches >> 23) & 3)) + __builtin_popcount(votes[k] ^ votes[j]);
                        }
                }
            if (m.genes.branches & (1u << 26))
                disagreement = 64 - disagreement;
            if (!(m.genes.branches & (1u << 27)))
                disagreement = 0;
        }
        if (counter) {
            counter->available |= 1u << i;
            if (separates)
                counter->separating |= 1u << i;
        }
        if (random)
            disagreement = 0;
        Word score = (disagreement << 8) | (mix(key + i * 587) & 255);
        if (choice == 8 || score > best) {
            choice = i;
            best = score;
        }
    }
    return choice;
}
} // namespace axion::life::clay
namespace axion::life::clay {
AX_FN bool predates(const Tile &t, Word birth) { return t.changed == ~0u || t.changed < birth; }
AX_FN bool valid(const Mind &m) {
    Word used = m.banks & 15, known = (m.banks >> 8) & 15;
    return !(m.banks & 0xe000u) && !(known & ~used) && !(used >> capacity(m)) &&
           (used ? bool(used & (1u << active(m))) : !active(m));
}
AX_FN bool valid(const Tile &t, Word tick) {
    if (t.changed > tick && t.changed != ~0u)
        return false;
    if (!size(t) && t.changed)
        return false;
    for (Word j = 0; j < 8; ++j) {
        Word b = groove(t, j);
        if (b && (b & 160) != 160)
            return false;
    }
    return true;
}
} // namespace axion::life::clay

namespace axion::life::clay {
struct Specimen {
    Pupil child{};
    Word ancestor = 0, school = 0, materials = 0, paid = 0;
};
struct Generation {
    Word tests = 0, native = 0, foreign = 0, silent = 0, paired = 0, formed = 0, food = 0, cost = 0, sightings = 0;
};
} // namespace axion::life::clay
namespace axion::life::clay {
struct Portable {
    Pupil pupil{};
    Tile object{};
};
struct Turn {
    Word trials = 0, productive = 0, complete = 0, turns = 0, evidence = 0, changes = 0;
};
} // namespace axion::life::clay
namespace axion::life::clay {
// Frozen state stores the forward winner sets in physical receptor order and
// the inverse sets in native ear order. The three joint/voice gates are shared.
struct Frozen {
    Law law{};
    Word colors = 0;
    unsigned short charge = 0, banks = 0;
    Charts charts{};
    Word inverse[4]{};
    unsigned short frame = 0, grip = 0;
    Byte activity[3]{}, rest[2]{}, switches = 0;
};
struct Deployment {
    Frozen brain{};
    Tile tile{};
};
static_assert(sizeof(Frozen) == 64 && sizeof(Deployment) == 76);
AX_FN Frozen compile(const Pupil &p) {
    Frozen f{};
    auto &m = p.cortex;
    auto &a = p.body;
    f.law = m.genes;
    f.colors = m.colors;
    f.charge = m.charge;
    f.banks = m.banks;
    f.charts = freeze(m, a);
    auto common = siphon::compile(a);
    f.frame = common.anatomy;
    f.grip = common.projection.grip;
    for (Word j = 0; j < 3; ++j)
        f.activity[j] = common.activity[j];
    f.rest[0] = common.rest[0];
    f.rest[1] = common.rest[1];
    f.switches = common.switches;
    for (Word j = 0; j < capacity(m); ++j)
        if (!j || (m.banks & (1u << j)))
            for (Word k = 0; k < 4; ++k) {
                f.inverse[j] |= siphon::light(pattern(m, a, j, true), k, a.genes, 0, true) << (4 * k);
                f.inverse[j] |= siphon::light(pattern(m, a, j, false), k, a.genes, 1, true) << (16 + 4 * k);
            }
    return f;
}
AX_FN siphon::Pocket native_image(const Frozen &f, Word selected) {
    siphon::Pocket p{};
    p.anatomy = f.frame;
    p.switches = f.switches;
    p.projection.grip = f.grip;
    for (Word j = 0; j < 3; ++j)
        p.activity[j] = f.activity[j];
    p.rest[0] = f.rest[0];
    p.rest[1] = f.rest[1];
    p.projection.attend = f.inverse[selected] & 65535;
    p.projection.hear = f.inverse[selected] >> 16;
    Word forward = f.charts.mask[selected];
    for (Word raw = 0; raw < 4; ++raw) {
        p.projection.ask |= ((forward >> (16 + 4 * raw)) & 15) << (4 * siphon::code(raw, f.frame));
        p.projection.speak |= ((forward >> (4 * raw)) & 15) << (4 * siphon::code(raw, f.frame >> 3));
    }
    return p;
}
} // namespace axion::life::clay
