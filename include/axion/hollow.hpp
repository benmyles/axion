#pragma once
#include "press.hpp"

namespace axion::life::hollow {
using Byte = unsigned char;
using Half = unsigned short;
constexpr Word limit = 32, missing = ~0u;
enum Kind : Word { implement, inlet, harvest, stone };
enum Mode : Word {
    digging = 1,
    changing = 2,
    directing = 4,
    anchoring = 8,
    distorted = 16,
    dead_reckoning = 32,
    eligible = 64,
    rich_beds = 128,
    drift = 256,
    all = digging | changing | directing | anchoring | dead_reckoning | eligible
};
struct Genes {
    Word synapse = 0, compass = 0, retrieval = 0, hands = 0;
};
struct Mind {
    Genes genes{};
    Byte x = 0, y = 0, yaw = 0, used = 0;
    Word efferent = 0;
};
struct Bed {
    Half carbon = 0;
    Byte relief = 0, cover = 0;
};
struct Ledger {
    Count settled = 0, placed = 0, released = 0, eaten = 0, heat = 0, formed = 0, opened = 0, pulses = 0, rewrites = 0,
          corrections = 0, recalls = 0, steered = 0, strides = 0, arrivals = 0, repeat_food = 0, same_place = 0,
          certified_food = 0;
};
struct State {
    Word slots = 8, mode = all;
    Count opening = 0;
    Ledger book{};
};
struct Field {
    Mind *minds = nullptr;
    Word *marks = nullptr, *sites = nullptr;
    Bed *beds = nullptr;
    Ledger *ledger = nullptr;
    Word stride = 0, slots = 0, mode = 0;
};
struct View {
    const Word *p;
    Word stride, capacity;
    AX_FN Word at(Word i) const { return p[i * stride]; }
};
struct Slots {
    Word *p;
    Word stride, capacity;
    AX_FN Word &at(Word i) const { return p[i * stride]; }
    AX_FN operator View() const { return {p, stride, capacity}; }
};
struct Saved {
    Mind mind{};
    Word slot_limit = 0, marks[limit]{}, tag = 0x4a415953, extent = sizeof(Saved);
};
static_assert(sizeof(Genes) == 16 && sizeof(Mind) == 24 && sizeof(Bed) == 4 && sizeof(Saved) == 164);
AX_FN Genes founder(Word k) { return {mix(k + 127), mix(k + 503), mix(k + 1079), mix(k + 1931)}; }
AX_FN Genes child(Genes g, Word k, Word rate) {
    auto change = [&](Word v, Word salt) {
        Word e = mix(k + salt);
        return (e & 255) < rate ? v ^ (1u << ((e >> 8) & 31)) : v;
    };
    return {change(g.synapse, 133), change(g.compass, 429), change(g.retrieval, 857), change(g.hands, 1259)};
}
AX_FN Word capacity(const Mind &m, Word arena) {
    Word c = 1 + ((m.genes.compass >> 24) & 31);
    return c < arena ? c : arena;
}
AX_FN Word strength(Word e) { return (e >> 26) & 3; }
AX_FN Word identity(Word e) { return (e >> 22) & 15; }
AX_FN Word landmark(Word e) { return (e >> 16) & 63; }
AX_FN bool valid_mark(Word e) { return !e || strength(e); }
AX_FN int delta(Word loc, Word here) { return int((loc - here + 128) & 255) - 128; }
AX_FN Word distance(Word e, const Mind &m) {
    return __builtin_abs(delta(e & 255, m.x)) + __builtin_abs(delta((e >> 8) & 255, m.y));
}
AX_FN Word phase(Word now) { return (now >> 3) & 15; }
AX_FN void turn(Mind &m, Word impulse) {
    if (!((m.genes.compass >> 3) & 1))
        return;
    m.yaw = (m.yaw + (((m.genes.compass >> 2) & 1) ? 4 - impulse : impulse)) & 3;
}
AX_FN void stride(Mind &m, Word relative, Word mode) {
    if (!(mode & dead_reckoning))
        return;
    Word receptor = ((m.genes.compass >> 2) & 1) ? 4 - relative : relative;
    Word direction = (m.yaw + receptor + (m.genes.compass & 3) + Word(bool(mode & distorted))) & 3;
    m.x = Byte(m.x + Word(direction == 1) - Word(direction == 3));
    m.y = Byte(m.y + Word(direction == 2) - Word(direction == 0));
}
// Recognition pools the locally witnessed texture. Two plausible places
// suppress correction. Neither the terrain address nor another brain is an input.
AX_FN bool orient(Mind &m, View s, Word texture, Word mode) {
    if (!(mode & anchoring) || !(m.genes.compass & (1u << 8)) || texture >= 64)
        return false;
    Word found = missing, best = 0;
    bool ambiguous = false;
    for (Word i = 0; i < m.used; ++i) {
        Word e = s.at(i), gain = strength(e);
        if (!gain || landmark(e) != texture)
            continue;
        if (gain > best) {
            best = gain;
            found = e & 65535;
            ambiguous = false;
        } else if (gain == best && found != (e & 65535))
            ambiguous = true;
    }
    if (!best || ambiguous || found == (Word(m.x) | (Word(m.y) << 8)))
        return false;
    m.x = found & 255;
    m.y = found >> 8;
    return true;
}
struct Write {
    Word slot = missing, prior = 0;
};
AX_FN Write observe(Mind &m, Slots s, Word cue, Word texture, bool consequence, Word now, Word key, Word mode) {
    if (!(mode & changing) || cue >= 16 || texture >= 64 ||
        !(m.genes.hands & (1u << ((cue >> 2) + 4 * Word(consequence)))))
        return {};
    Word available = capacity(m, s.capacity), pos = m.x | (Word(m.y) << 8), vacant = missing, hit = missing, victim = 0,
         lowest = ~0u;
    for (Word i = 0; i < m.used; ++i) {
        Word e = s.at(i), level = strength(e);
        if (!level) {
            if (vacant == missing)
                vacant = i;
            continue;
        }
        bool same =
            identity(e) == cue && ((e & 65535) == pos || (((m.genes.retrieval >> 18) & 1) && landmark(e) == texture));
        if (same) {
            hit = i;
            break;
        }
        Word age = (phase(now) - (e >> 28)) & 15;
        Word keep = 16 * (level * (1 + ((m.genes.retrieval >> 4) & 3)) + (15 - age) * ((m.genes.retrieval >> 6) & 1)) +
                    ((key + i * 7) & 15);
        if (keep < lowest) {
            lowest = keep;
            victim = i;
        }
    }
    Word i = hit != missing ? hit : vacant != missing ? vacant : m.used < available ? m.used : victim;
    Word old = i == m.used ? 0 : s.at(i),
         input = (hit != missing ? strength(old) : 0) + 4 * Word(hit != missing) + 8 * Word(consequence);
    Word level = (m.genes.synapse >> (2 * input)) & 3;
    if (!level && hit == missing)
        return {};
    if (i == m.used)
        ++m.used;
    s.at(i) = level ? pos | (texture << 16) | (cue << 22) | (level << 26) | (phase(now) << 28) : 0;
    return {i, old};
}
struct Recall {
    Word slot = missing, bearings = 0, cue = 16, compass = 4, range = 0, goal = 0;
};
AX_FN Word effector(const Mind &m, Word dir) {
    Word r = (dir - m.yaw) & 3;
    if ((m.genes.compass >> 6) & 1)
        r = (4 - r) & 3;
    return (r + ((m.genes.compass >> 4) & 3)) & 3;
}
AX_FN Recall emit(const Mind &m, Recall chosen, Word key, Word mask) {
    if (!chosen.range)
        return chosen;
    int dx = delta(chosen.goal & 255, m.x), dy = delta(chosen.goal >> 8, m.y);
    Word horizontal = dx < 0 ? 3u : 1u, vertical = dy < 0 ? 0u : 2u;
    horizontal = effector(m, horizontal);
    vertical = effector(m, vertical);
    if (dx)
        chosen.bearings |= 1u << horizontal;
    if (dy)
        chosen.bearings |= 1u << vertical;
    Word ax = __builtin_abs(dx), ay = __builtin_abs(dy), prefer = (m.genes.retrieval >> 11) & 3;
    bool first = dx && (!dy || (prefer == 0 ? ax >= ay : prefer == 1 ? ax <= ay : prefer == 2 ? true : bool(key & 1)));
    Word aim = first ? horizontal : vertical, other = first ? vertical : horizontal;
    if (mask & (1u << aim)) {
        if ((chosen.bearings & (1u << other)) && !(mask & (1u << other)))
            aim = other;
        else if (m.genes.retrieval & (1u << 13))
            aim = (aim + (((m.genes.retrieval >> 14) & 1) ? 1 : 3)) & 3;
    }
    chosen.compass = aim;
    return chosen;
}
AX_FN int priority(const Mind &m, Word e, Word i, Word range, Word now, Word key) {
    int score = int(strength(e)) * (1 + int(m.genes.retrieval & 15)) - int(range) * int((m.genes.retrieval >> 8) & 3) -
                int((phase(now) - (e >> 28)) & 15) * int((m.genes.retrieval >> 10) & 1);
    return score * 32 + int((key + i * 11) & 31);
}
AX_FN Recall recall(const Mind &m, View s, Word cues, Word now, Word key, Word mask = 0) {
    Recall chosen{};
    int best = -100000;
    for (Word i = 0; i < m.used; ++i) {
        Word e = s.at(i), gain = strength(e), cue = identity(e);
        if (!gain || !(cues & (1u << cue)))
            continue;
        Word range = distance(e, m);
        int score = priority(m, e, i, range, now, key);
        if (chosen.slot == missing || score > best) {
            best = score;
            chosen.slot = i;
            chosen.cue = cue;
            chosen.range = range;
            chosen.goal = e & 65535;
        }
    }
    if (chosen.slot == missing || !chosen.range)
        return chosen;
    return emit(m, chosen, key, mask);
}
AX_FN Slots cells(Field f, Word id) { return {f.marks + id, f.stride, f.slots}; }
AX_FN Word material(Bed b) { return (b.relief - 1) & 3; }
AX_FN Word texture(Bed b) { return (b.relief - 1) & 63; }
AX_FN bool viable(const Mind &m, Word arena) {
    return arena && arena <= limit && m.yaw < 4 && m.used <= capacity(m, arena);
}
AX_FN bool viable(Bed b) {
    return b.relief <= 64 && b.cover <= 15 && (!b.relief ? (!b.carbon && !b.cover) : b.carbon <= 255);
}
} // namespace axion::life::hollow

namespace axion::life::hollow {
struct Site {
    short x = 0, y = 0;
    Byte tint = 0, relief = 0;
};
struct Path {
    int x = 0, y = 0;
    Word head = 0, steps = 0;
};
struct Ground {
    Site sites[6]{};
    Word seed = 0, bath = 0, old_gate = 0, gate = 0;
};
struct Trip {
    Word attempts = 0, chosen = 0, retrieved = 0, delivered = 0, food = 0, crumbs = 0, work = 0, lifted = 0,
         corrections = 0, lost = 0, body_steps = 0, straight = 0, stamped = 0, cells = 0, remembered = 0, pressured = 0,
         deposits = 0;
};
struct Proof {
    Word seed = 0, genome[4]{}, eye = 0, bath = 0, old_gate = 0, gate = 0, tones = 0, forecast = 0, water = 0;
    Site sites[6]{};
    Path start{}, finish{};
    Mind cortex{};
    Word memory[limit]{}, chosen = 4, visits = 0, course = 0, track[32]{};
};
enum Trial : Word {
    open_route,
    refound,
    old_water,
    no_positions,
    no_sisters,
    no_taste,
    no_motion,
    skewed_motion,
    no_landmark,
    false_landmark,
    one_cell,
    rearranged,
    same_water,
    trial_count
};
struct Family {
    Genes genes{};
    Word allowance = 0;
};
struct FieldTest {
    Word subjects = 0, goals = 0, exact = 0, control_exact = 0, old_food = 0, other_food = 0, travel = 0, dated = 0,
         original_pits = 0, other_pits = 0, matching = 0, control_match = 0, source_food = 0, grazed = 0,
         other_grazed = 0;
};
AX_FN Word best_tools(const clay::Mind &p, Word bath) {
    Word best = 0, mask = 0;
    for (Word i = 0; i < 4; ++i) {
        Word current = press::drive(p, i, bath);
        if (!mask || current > best) {
            best = current;
            mask = 1u << i;
        } else if (current == best)
            mask |= 1u << i;
    }
    return mask;
}
} // namespace axion::life::hollow
