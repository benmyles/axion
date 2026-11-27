#pragma once
#include "material.hpp"

namespace axion::life::siphon {
using Byte = unsigned char;
using Half = unsigned short;
// The same projection can express a sound from a perception or recall a
// perception from a sound. Each word carries a four by four two-bit field.
struct Genes {
    Word contact = 0, reinforcement = 0, release = 0, lateral = 0, anatomy = 0, volition = 0;
};
struct Animal {
    Genes genes{};
    Word address = 0, surface = 0, tendon = 0;
    Word occasions = 0, listened = 0, paid = 0, harvest = 0;
    Word volley = 0;
};
struct Totals {
    Count pairs = 0, inquiries = 0, replies = 0, strokes = 0, transmitted = 0, released = 0;
    Count consumed = 0, native_work = 0, sights = 0, field_edits = 0, seep = 0;
};
struct State {
    Word bands = 4, wire = 1, windows = 53, pulse = 1;
    Totals total{};
};
struct Echo {
    Word pulse = 0, tick = 0;
};
struct Audience {
    Count heard = 0, paired = 0, edits = 0, young = 0;
};
struct Chorus {
    Word coupling = 1, access = 1;
    Audience total{};
};
struct Estuary {
    Animal *residents = nullptr;
    Byte *ports = nullptr;
    Totals *ledger = nullptr;
    Word bands = 4, wire = 1, windows = 1, pulse = 1;
    Echo *air = nullptr;
    Audience *audience = nullptr;
    Word coupling = 1, access = 1;
};
struct Saved {
    Animal animal{};
    Word signature = 0x53595048u, extent = sizeof(Saved);
};
static_assert(sizeof(Genes) == 24 && sizeof(Animal) == 56 && sizeof(Saved) == 64);

AX_FN Genes founder(Word k) {
    return {mix(k + 559), mix(k + 1733), mix(k + 41), mix(k + 3929), mix(k + 6361), mix(k + 9173)};
}
AX_FN Word graded(Word key) {
    Word table = 0;
    for (Word event = 0; event < 4; ++event) {
        Word codon = (key >> (4 * event)) & 15;
        for (Word old = 0; old < 4; ++old) {
            int change = int(codon % 5) - 2;
            Word next = codon < 10   ? Word(((int(old) + change) < 0   ? 0
                                             : (int(old) + change) > 3 ? 3
                                                                       : int(old) + change))
                        : codon < 14 ? codon - 10
                                     : old ^ (codon == 14 ? 3 : 1);
            table |= next << (8 * event + 2 * old);
        }
    }
    return table;
}
AX_FN Genes develop(Word key, bool graded_laws) {
    auto g = founder(key);
    if (!graded_laws)
        return g;
    g.contact = graded(g.contact);
    g.reinforcement = graded(g.reinforcement);
    Word photic = (g.release & 1) ? 0xe4u : 0x1bu;
    for (Word j = 0; j < 3; ++j) {
        Word v = (g.release >> (8 * j)) & 255;
        if (v & 2)
            v = photic;
        g.release = (g.release & ~(255u << (8 * j))) | (v << (8 * j));
    }
    return g;
}
AX_FN Genes inherit(Genes g, const Genes &other, Word key, Word rate, bool cross) {
    auto edit = [&](Word a, Word b, Word salt) {
        Word h = mix(key + salt);
        Word v = cross && (h & 256) ? b : a;
        return (h & 255) < rate ? v ^ (1u << ((h >> 15) & 31)) : v;
    };
    return {edit(g.contact, other.contact, 311),  edit(g.reinforcement, other.reinforcement, 619),
            edit(g.release, other.release, 2857), edit(g.lateral, other.lateral, 2339),
            edit(g.anatomy, other.anatomy, 6113), edit(g.volition, other.volition, 7639)};
}
// Local ears and optics can permute their input channels. A twist changes
// their origin and a reflection exchanges two axes.
AX_FN Word code(Word value, Word wiring) {
    Word rotated = (wiring & 4) ? ((value >> 1) | ((value & 1) << 1)) : value;
    return rotated ^ (wiring & 3);
}
AX_FN Word undo(Word v, Word w) { return code(v ^ (w & 3), w & 4); }
AX_FN Word column(Word index, Word motor, bool backward) { return backward ? 4 * motor + index : 4 * index + motor; }
AX_FN Word strength(Word matrix, Word i, Word release) { return (release >> (2 * ((matrix >> (2 * i)) & 3))) & 3; }
AX_FN Word winners(Word matrix, Word which, Word release, Word inhibition, bool backward) {
    int best = -40;
    Word mask = 0;
    for (Word j = 0; j < 4; ++j) {
        int score = 4 * int(strength(matrix, column(which, j, backward), release));
        if (inhibition) {
            Word other = 0;
            for (Word k = 0; k < 4; ++k)
                if (k != which) {
                    Word v = strength(matrix, column(k, j, backward), release);
                    if (v > other)
                        other = v;
                }
            score -= int(inhibition * other);
        }
        if (score > best) {
            best = score;
            mask = 0;
        }
        if (score == best)
            mask |= 1u << j;
    }
    return mask;
}
AX_FN Word elect(Word mask, Word dye) {
    // An affine traversal samples the tied cells without a fixed preferred cell.
    Word step = ((dye >> 2) & 1) * 2 + 1;
    for (Word i = 0; i < 4; ++i) {
        Word j = (dye + i * step) & 3;
        if (mask & (1u << j))
            return j;
    }
    return 4;
}
AX_FN Word adapt(Word matrix, Word receptor, Word act, bool reward, const Genes &genes, bool proprio = false) {
    Word law = proprio ? genes.reinforcement : genes.contact;
    // The inherited expression gate can extend eligibility down the acoustic
    // column. Unselected cells use their own branch of the transition table.
    Word update = matrix;
    bool neighbors = (genes.lateral >> (proprio ? 19 : 17)) & 1;
    for (Word j = 0; j < 4; ++j) {
        Word where = 2 * (4 * receptor + j), prev = (matrix >> where) & 3, match = j == act;
        Word next = (law >> (2 * prev + 8 * match + 16 * Word(reward))) & 3;
        update ^= (prev ^ next) << where;
        if (neighbors && j != receptor) {
            where = 2 * (4 * j + act);
            prev = (matrix >> where) & 3;
            next = (law >> (2 * prev + 16 * Word(reward))) & 3;
            update ^= (prev ^ next) << where;
        }
    }
    return update;
}
struct Image {
    Half ask = 0, attend = 0, speak = 0, hear = 0, grip = 0;
};
static_assert(sizeof(Image) == 10);
AX_FN Word row(Half r, Word j) { return (r >> (4 * j)) & 15; }
AX_FN Word light(Word field, Word event, const Genes &g, Word type, bool reverse) {
    return winners(field, event, g.release >> (8 * type), (g.lateral >> (2 * (2 * type + Word(reverse)))) & 3, reverse);
}
AX_FN Image settle(const Animal &a) {
    Image image{};
    for (Word j = 0; j < 4; ++j) {
        image.ask |= light(a.address, j, a.genes, 0, false) << (4 * j);
        image.attend |= light(a.address, j, a.genes, 0, true) << (4 * j);
        image.speak |= light(a.surface, j, a.genes, 1, false) << (4 * j);
        image.hear |= light(a.surface, j, a.genes, 1, true) << (4 * j);
        image.grip |= light(a.tendon, j, a.genes, 2, false) << (4 * j);
    }
    return image;
}
struct Scene {
    Word inlet = 0, membrane = 0, opening = 0, carbon = 0, area = 0;
    Word source_pose = 0, sink_pose = 0, seen = 0, charged = 0, shared = 0, resonant = 0;
};
struct Intent {
    Word query = 4, reply = 4, attention = 0, percept = 0, recall = 0, stroke = 4;
};
struct Outcome {
    Word angle = 0, actual = 4, opened = 0, food = 0, speaker_share = 0, receiver_share = 0, cost_a = 0, cost_b = 0;
};
AX_FN Word sensor(Word inlet, Word branch) { return (inlet >> (2 * branch)) & 3; }
AX_FN Word spectrum(Word light, Word bands) {
    return bands == 1 ? (light & 3) * 85u : bands == 2 ? (light & 15) * 17u : light & 255;
}
AX_FN Word open_view(Word flags, Word key) {
    Word stage = (flags >> 2) & 3;
    return stage == 3 || (stage && ((key >> 21) & 3) < stage) ? 3u : 0u;
}
AX_FN Word relative(Word desired, Word start, Word posture) {
    switch (posture & 3) {
    case 0:
        return (desired - start) & 3;
    case 1:
        return desired;
    case 2:
        return desired ^ start;
    default:
        return (desired + start) & 3;
    }
}
// The buffers on the two sides contain disjoint observations. The acoustic
// link is the only route from the inlet to the gripper before a stroke.
// Once learning is suspended, all ties, drives and inherited sensor wirings
// needed for native inference fit in 18 bytes. No germline lookup is needed.
struct Pocket {
    Image projection{};
    Half anatomy = 0;
    Byte activity[3]{}, rest[2]{}, switches = 0;
};
static_assert(sizeof(Pocket) == 18);
AX_FN Pocket compile(const Animal &a) {
    auto &g = a.genes;
    Pocket p{};
    p.projection = settle(a);
    p.anatomy = g.anatomy & 16383;
    for (Word j = 0; j < 3; ++j)
        p.activity[j] = g.volition >> (8 * j);
    p.rest[0] = mix((g.anatomy >> 16) ^ 953u);
    p.rest[1] = mix((g.anatomy >> 16) ^ 371u);
    p.switches = ((g.lateral >> 24) & 15) | (((g.release >> 27) & 3) << 4);
    return p;
}
AX_FN Word frame(const Pocket &p) { return p.anatomy; }
AX_FN Word frame(const Animal &a) { return a.genes.anatomy; }
AX_FN Word active(const Pocket &p, Word tissue, Word position) { return (p.activity[tissue] >> (position & 7)) & 1; }
AX_FN Word active(const Animal &a, Word tissue, Word position) {
    return (a.genes.volition >> (8 * tissue + (position & 7))) & 1;
}
AX_FN Word gates(const Pocket &p) { return p.switches; }
AX_FN Word gates(const Animal &a) { return ((a.genes.lateral >> 24) & 15) | (((a.genes.release >> 27) & 3) << 4); }
AX_FN Word bias(const Pocket &p, bool name) { return p.rest[name]; }
AX_FN Word bias(const Animal &a, bool name) { return mix((a.genes.anatomy >> 16) ^ (name ? 371u : 953u)); }
enum Projection { ask, attend, speak, hear, grip };
template <Projection k> AX_FN Word projection(const Pocket &p, Word at) {
    if constexpr (k == ask)
        return row(p.projection.ask, at);
    if constexpr (k == attend)
        return row(p.projection.attend, at);
    if constexpr (k == speak)
        return row(p.projection.speak, at);
    if constexpr (k == hear)
        return row(p.projection.hear, at);
    return row(p.projection.grip, at);
}
template <Projection k> AX_FN Word projection(const Animal &a, Word at) {
    return light(k <= attend ? a.address
                 : k <= hear ? a.surface
                             : a.tendon,
                 at, a.genes, k / 2, k == attend || k == hear);
}
template <class Tissue> AX_FN Word voice_choice(Word mask, Word receptor, Word key, const Tissue &p, bool name) {
    Word gate = gates(p);
    if ((gate & (name ? 8 : 4)) && ((key >> 12) & 7) >= (gate >> 4)) {
        Word preferred = (bias(p, name) >> (2 * receptor)) & 3;
        if (mask & (1u << preferred))
            return preferred;
    }
    return elect(mask, key);
}
template <class Tissue> AX_FN Word query(const Tissue &b, Word membrane, Word key, Word motor) {
    if (!active(b, 0, motor))
        return 4;
    Word anatomy = frame(b), at = code(membrane, anatomy);
    return undo(voice_choice(projection<ask>(b, at), at, key, b, false), anatomy >> 6);
}
template <class Tissue>
AX_FN Word reply(const Tissue &a, Word inlet, Word incoming, Word key, Word motor, Intent &act, Word visible = 4) {
    Word anatomy = frame(a), heard = code(incoming & 3, anatomy >> 6);
    act.attention = undo(elect(incoming < 4 ? projection<attend>(a, heard) : 15u, key), anatomy);
    if (visible < 4 && (gates(a) & 1))
        act.attention = visible;
    act.percept = sensor(inlet, act.attention);
    if (!active(a, 1, motor))
        return 4;
    Word at = code(act.percept, anatomy >> 3);
    return undo(voice_choice(projection<speak>(a, at), at, mix(key + 199), a, true), anatomy >> 6);
}
template <class Tissue>
AX_FN Word press(const Tissue &b, Word received, Word opening, Word key, Word motor, Intent &act, Word visible = 4) {
    Word anatomy = frame(b), heard = code(received & 3, anatomy >> 6);
    act.recall = undo(elect(received < 4 ? projection<hear>(b, heard) : 15u, key), anatomy >> 3);
    if (visible < 4 && (gates(b) & 2))
        act.recall = visible;
    if (!active(b, 2, motor))
        return 4;
    Word at = relative(act.recall, opening, anatomy >> 12);
    return elect(projection<grip>(b, at), mix(key + 739));
}
AX_FN Word carried(Word tone, Word wire, Word key) {
    if (tone == 4 || !wire)
        return 4;
    if (wire == 2)
        return mix(key) & 3;
    if (wire == 3)
        return (tone + 1) & 3;
    return tone;
}
template <class Upstream,class Outlet>
AX_FN Intent meet(const Upstream &a, const Outlet &b, const Scene &room, Word key, Word wire, Word lesion = 0) {
    Intent act;
    act.query = query(b, room.membrane, mix(key + 547), room.sink_pose);
    Word q = carried(act.query, lesion == 1 ? 0 : lesion == 3 ? 3 : wire, key + 121);
    act.reply = reply(a, room.inlet, q, mix(key + 733), room.source_pose, act, (room.shared & 1) ? room.membrane : 4);
    Word r = carried(act.reply, lesion == 2 ? 0 : lesion == 4 ? 3 : wire, key + 367);
    act.stroke = press(b, r, room.opening, mix(key + 971), room.sink_pose, act,
                       (room.shared & 2) ? sensor(room.inlet, room.membrane) : 4);
    return act;
}
AX_FN Outcome squeeze(const Scene &room, const Intent &act, Word joints) {
    Outcome r{};
    r.angle = room.opening;
    r.cost_a = room.resonant ? 1 : Word(act.reply < 4);
    r.cost_b = !room.resonant && act.query < 4;
    if (act.stroke >= 4)
        return r;
    r.actual = code(act.stroke, joints >> 9);
    r.angle = (room.opening + r.actual) & 3;
    ++r.cost_b;
    r.opened = room.charged && r.angle == sensor(room.inlet, room.membrane);
    if (r.opened) {
        r.food = room.carbon < room.area ? room.carbon : room.area;
        r.speaker_share = r.food / 2;
        r.receiver_share = r.food - r.speaker_share;
    }
    return r;
}
AX_FN Word learn(Animal &a, Animal &b, const Scene &room, const Intent &act, const Outcome &out, Word wire, Word key,
                 bool plastic, Word request = 4, Word lesion = 0) {
    Word edits = 0;
    auto write = [&](Animal &who, Word &table, Word index, Word glyph, bool positive, bool muscle = false) {
        Word next = adapt(table, index, glyph, positive, who.genes, muscle);
        edits += __builtin_popcount(next ^ table);
        table = next;
    };
    Word heard_q = carried(act.query, lesion == 1 ? 0 : lesion == 3 ? 3 : wire, key + 121);
    Word heard_r = carried(act.reply, lesion == 2 ? 0 : lesion == 4 ? 3 : wire, key + 367);
    if (plastic) {
        if (act.query < 4)
            write(b, b.address, code(request < 4 ? request : room.membrane, b.genes.anatomy),
                  code(act.query, b.genes.anatomy >> 6), out.food || ((b.genes.lateral >> 22) & 1));
        if (act.reply < 4)
            write(a, a.surface, code(act.percept, a.genes.anatomy >> 3), code(act.reply, a.genes.anatomy >> 6),
                  out.food || ((a.genes.lateral >> 23) & 1));
        if (heard_q < 4 && out.actual < 4 && (room.seen & 1))
            write(a, a.address, code(room.membrane, a.genes.anatomy), code(heard_q, a.genes.anatomy >> 6),
                  out.food || ((a.genes.lateral >> 20) & 1));
        if (heard_r < 4 && out.actual < 4 && (room.seen & 2))
            write(b, b.surface, code(act.percept, b.genes.anatomy >> 3), code(heard_r, b.genes.anatomy >> 6),
                  out.food || ((b.genes.lateral >> 20) & 1));
        if (out.actual < 4)
            write(b, b.tendon, out.actual, act.stroke, out.food || ((b.genes.lateral >> 21) & 1), true);
    }
    ++a.occasions;
    ++b.occasions;
    a.listened += heard_q < 4;
    b.listened += heard_r < 4;
    a.harvest += out.speaker_share;
    b.harvest += out.receiver_share;
    a.paid += out.cost_a;
    b.paid += out.cost_b;
    a.volley = Word(act.attention) | ((act.reply + 1) << 3) | ((heard_q + 1) << 8) | (Word(out.food != 0) << 11);
    b.volley = Word(act.recall) | ((act.stroke + 1) << 3) | ((heard_r + 1) << 8) | (Word(out.food != 0) << 11);
    return edits;
}
// A wave may be followed by a visible event at its physical source. The
// source tag is only used to exclude the two participants from the audience.
AX_FN Word radiate(Word ticket, Word glyph, Word glint, bool aperture, bool visible, bool product) {
    return (ticket << 12) | (glyph & 3) | ((glint & 3) << 2) | (Word(aperture) << 4) | (Word(visible) << 5) |
           (Word(product) << 6) | (Word(glyph < 4) << 7);
}
AX_FN Word tail(Word pulse) { return 1u << ((pulse >> 8) & 15); }
AX_FN Word daylight(const Echo &wave, Word now) { return now - wave.tick; }
AX_FN void ring(Echo &bed, Word event, Word tick, Word depth) {
    if (!(event & 128))
        return;
    Word duration = 1u << depth, age = daylight(bed, tick), old = tail(bed.pulse);
    if ((bed.pulse & 128) && age < old && old - age > duration)
        return;
    bed = {(event & ~3840u) | (depth << 8), tick};
}
AX_FN Word overhear(Animal &viewer, Word event, bool eyes, bool plastic) {
    ++viewer.listened;
    if (!plastic || !eyes || !(event & 32))
        return 0;
    bool question = event & 16;
    Word native = code((event >> 2) & 3, viewer.genes.anatomy >> (question ? 0 : 3));
    Word &field = question ? viewer.address : viewer.surface, old = field;
    bool gate = (event & 64) || ((viewer.genes.lateral >> (question ? 20 : 21)) & 1);
    field = adapt(field, native, code(event & 3, viewer.genes.anatomy >> 6), gate, viewer.genes);
    return __builtin_popcount(old ^ field);
}
struct Pair {
    Animal a{}, b{};
};
struct Brood {
    Genes a{}, b{};
    Word clan = 0, alive = 0;
};
struct Account {
    Word opening = 0, stock = 0, residual = 0, closing = 0, heat = 0, calls = 0, strokes = 0, food = 0, eggs = 0;
};
struct Reading {
    Word trials = 0, queries = 0, replies = 0, strokes = 0, harvests = 0, energy = 0, spent = 0, attended = 0;
    Word quartets = 0, all_four = 0, spoken_changes = 0, motor_changes = 0, individuals = 0, visits = 0, learning = 0;
};
struct PairScore {
    Word attempts = 0, worked = 0, closed = 0, no_question = 0, all_four = 0, quartets = 0;
};
struct Witness {
    Word address[4]{}, surface[4]{}, muscle[4]{}, attended[4]{}, seen[4]{}, worked[4]{};
};
} // namespace axion::life::siphon
