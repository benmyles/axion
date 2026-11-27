#pragma once
#include "linkage.hpp"
namespace axion::life::practice {
struct Genes {
    Word attention = 0, inference = 0, learning = 0, muscles = 0;
};
struct Mind {
    Genes genes{};
    Word banks = 0, focus = 0, trace = 0, samples = 0;
};
// Six possible oriented four-face cycles per glyph. Each two-bit seam observation
// denotes stationary or engaged; polarity and fitting have separate contacts.
struct Context {
    Word rings = 0, gates[2]{}, signs = 0, joins = 0;
};
struct Slots {
    Context *data;
    Word stride, cap;
    AX_FN Context &at(Word i) const { return data[Count(i) * stride]; }
};
struct Saved {
    Mind mind{};
    Context bank[4]{};
    Word arena = 2, tag = 0x54434154, extent = sizeof(Saved);
};
enum Mode : Word {
    engaging = 1,
    holding = 2,
    probing = 4,
    contextual = 8,
    studying = 16,
    social = 32,
    crafting = 64,
    collecting = 128,
    random_probes = 256,
    wrong_effect = 512,
    constant_stay = 1024,
    withheld = 2048,
    families = 4096,
    reciprocal = 8192,
    sham = 16384,
    all = 255
};
struct Totals {
    Count invitations = 0, entered = 0, continued = 0, left = 0, stayed = 0, strokes = 0, chosen = 0, random = 0,
          hypotheses = 0, switches = 0, writes = 0, observed = 0, collections = 0, work = 0, searches = 0,
          proposals = 0, alignments = 0, fittings = 0, placed = 0, removed = 0, loops_learned = 0, direct_known = 0,
          direct_exact = 0, bouts_eight = 0, changed_handles = 0, protected_goal = 0;
};
struct State {
    Word mode = all, arena = 2, seed = 0, version = 1;
    Totals book{};
};
struct Mark {
    Word who = 0, born = 0, date = 0, edge = 0;
};
struct Credits {
    Count installed = 0, channel_open = 0, necessary = 0, water = 0, transport = 0, eaten = 0, other_events = 0,
          other_food = 0, older_events = 0, older_food = 0, immediate = 0, stripped = 0;
};
struct Field {
    Mind *minds = nullptr;
    Context *banks = nullptr;
    Totals *book = nullptr;
    Word sites = 0, cap = 2, mode = all;
    Mark *marks = nullptr;
    Credits *credits = nullptr;
};
static_assert(sizeof(Context) == 20 && sizeof(Mind) == 32 && sizeof(Saved) == 124);
AX_FN Word size(const Mind &m) { return m.banks & 7; }
AX_FN Word active(const Mind &m) { return (m.banks >> 3) & 3; }
AX_FN Word capacity(Genes g, Word cap) {
    Word n = 1 + ((g.inference >> 2) & 3);
    return n < cap ? n : cap;
}
AX_FN Slots cells(Field f, Word id) { return {f.banks + id, f.sites, f.cap}; }
AX_FN bool valid(const Mind &m, Word cap) {
    return cap >= 1 && cap <= 4 && size(m) <= capacity(m.genes, cap) && !(m.banks & ~31u) &&
           (size(m) ? active(m) < size(m) : !m.banks) && (!m.focus || ((m.focus & 63) != 0 && (m.focus >> 12) != 0));
}
AX_FN bool valid(const Context &c, bool used) {
    if (!used)
        return !(c.rings | c.signs | c.joins | c.gates[0] | c.gates[1]);
    if ((c.rings >> 24) || (c.signs >> 16) || (c.joins >> 24))
        return false;
    Word words[]{c.gates[0], c.gates[1], c.signs, c.joins};
    for (Word x : words)
        if (x & (x >> 1) & 0x55555555u)
            return false;
    return true;
}
AX_FN Genes founder(Word k) { return {mix(k + 1709), mix(k + 5209), mix(k + 7297), mix(k + 3181)}; }
AX_FN Genes child(Genes g, Word k, Word rate) {
    auto mut = [&](Word v, Word salt) {
        Word s = mix(k + salt);
        return (s & 255) < rate ? v ^ (1u << ((s >> 8) & 31)) : v;
    };
    return {mut(g.attention, 5801), mut(g.inference, 4523), mut(g.learning, 7789), mut(g.muscles, 3691)};
}
AX_FN bool stays(const Mind &m, Word tile) { return (m.focus >> 12) == tile + 1 && (m.focus & 63); }
AX_FN bool entitled(Word mode, Word tag) {
    return !(mode & families) || (mode & sham) || (bool(tag) != bool(mode & reciprocal));
}
AX_FN Word tile_at(Word pos, Word side) { return (pos % side) / 2 + (pos / side / 2) * (side / 2); }
AX_FN void disturb(Field f, Word side, Word from, Word to) {
    if (!f.marks)
        return;
    Word tile = tile_at(from, side);
    if (tile != tile_at(to, side))
        return;
    Word a = (from & 1) | (((from / side) & 1) << 1), b = (to & 1) | (((to / side) & 1) << 1);
    if (a > b) {
        Word swap = a;
        a = b;
        b = swap;
    }
    // The edge code is written by one tile thread. Only the unique thread
    // that changes this physical edge can invalidate the maker during a pair pass.
    auto &r = f.marks[tile];
    if (r.edge == (a | (b << 2)))
        r.who = 0;
}
AX_FN Word span(Genes g) { return 1 + ((g.attention >> 12) & 31); }
AX_FN Word reach(Genes g) { return 1 + ((g.muscles >> 8) & 7); }
AX_FN bool willing(const Mind &m, Word motor, Word reserve, Word event, Word roll) {
    Word sensory = (1u << (motor & 7)) | (1u << (8 + (event & 3)));
    Word excitation = __builtin_popcount(m.genes.attention & sensory) + ((m.genes.attention >> 24) & 3);
    Word threshold = (m.genes.attention >> 26) & 3;
    return reserve > 2 + ((m.genes.muscles >> 20) & 15) && excitation > threshold &&
           ((roll >> 16) & 7) <= ((m.genes.attention >> 17) & 7);
}
struct Family {
    Genes genes{};
    Word eggs = 0;
};
} // namespace axion::life::practice
namespace axion::life::practice {
constexpr Word responsive_attention = 32768, misassigned_attention = 65536;
AX_FN Word cue(bool available, bool unknown) { return 2 * Word(available) + Word(unknown); }
AX_FN int preference(const Mind &m, Word place, Word mode) {
    if (!(mode & responsive_attention) || !(m.genes.learning & (1u << 20)))
        return 0;
    Word row = (m.trace >> (8 + 4 * place)) & 15;
    return int((row >> 2) & 3) - int(row & 3);
}
AX_FN bool persist(const Mind &m, Word site, Word random, Word mode, Word reserve) {
    if (!reserve)
        return false;
    if (mode & constant_stay)
        return true;
    int prior = 3 + int((m.genes.attention >> (2 * site)) & 3) + int((m.genes.attention >> 20) & 1);
    return int((random >> 19) & 7) < prior + preference(m, site, mode);
}
AX_FN void intend(Mind &m, Word site, bool stayed) {
    m.trace = (m.trace & ~240u) | (site << 4) | (Word(stayed) << 6) | 128;
}
AX_FN void reinforce(Mind &m, bool changed, bool correct, bool fed, Word mode) {
    if (!(m.trace & 128))
        return;
    if (changed)
        m.trace |= 1u << 24;
    else
        m.trace &= ~(1u << 24);
    if (!(mode & responsive_attention) || !(mode & studying) || !(m.genes.learning & (1u << 20)))
        return;
    Word receptor = (m.trace >> 4) & 3, action = (m.trace >> 6) & 1;
    if (mode & misassigned_attention)
        receptor ^= 3;
    Word shift = 8 + 4 * receptor + 2 * action, weight = (m.trace >> shift) & 3;
    Word event = Word(changed) + 2 * Word(correct);
    bool reward = fed || ((m.genes.inference >> (20 + event)) & 1);
    Word transition = (m.genes.learning >> (reward ? 8 : 24)) & 255, next = (transition >> (2 * weight)) & 3;
    m.trace = (m.trace & ~(3u << shift)) | (next << shift);
}
} // namespace axion::life::practice
