#pragma once
#include "grounding.hpp"

namespace axion::life::linkage {
using Byte = unsigned char;
constexpr Word absent = ~0u, stop = 8, no_path = 15, contexts = 4, max_steps = 12;
struct Genes {
    Word touch = 0, binding = 0, arbor = 0, impulse = 0;
};
struct Mind {
    Genes genes{};
    Word tags = 0, contacts = 0;
};
struct Tissue {
    grounding::Synapse local{}, seam[2]{}, splice{};
};
struct Slots {
    Tissue *start = nullptr;
    Word stride = 1, capacity = 4;
    AX_FN Tissue &at(Word i) const { return start[Count(i) * stride]; }
};
struct Saved {
    Mind mind{};
    Tissue tissue[contexts]{};
    Word arena = 4, tag = 0x4b4e494c, extent = sizeof(Saved);
};
struct Material {
    Word dye = 0, notches = 0, reverse = 0, mesh = 0;
};
struct Rig {
    Word parts = 0, glyphs = 0, joints = 0, handles = 0;
};
struct Pattern {
    Word faces = 0, care = 0, alternate = 0;
};
struct Obs {
    Rig rig{};
    Word faces = 0;
};
struct Map {
    Word next[4]{}, link[2][4]{}, splice[4]{};
};
struct Program {
    Word low = 0, high = 0, len = 0;
};
struct Provenance {
    Word maker = 0, born = 0, dated = 0, stamp = 0, pattern = 0;
};
struct Totals {
    Count active = 0, attempts = 0, taps = 0, transmitted = 0, heat = 0, explore = 0, invented = 0, solved = 0,
          seen = 0, self_changes = 0, public_changes = 0, flow = 0, eaten = 0, loose = 0, stamped = 0, own_meal = 0,
          other_meal = 0, older_meal = 0, other_carbon = 0, older_carbon = 0, abandoned = 0, water = 0, loops_in = 0,
          loops_out = 0, designs = 0, join_edits = 0, certified_joins = 0;
};
enum Mode : Word {
    writable = 1,
    planning = 2,
    witnesses = 4,
    coupled = 8,
    working = 16,
    current = 32,
    other_eye = 64,
    scrambled = 128,
    private_only = 256,
    mark_families = 512,
    swap_families = 1024,
    equal_families = 2048,
    fitting = 4096,
    designing = 8192,
    focal = 16384,
    hydration = 32768,
    all = writable | planning | witnesses | coupled | working | current | fitting | designing
};
struct State {
    Word mode = all, arena = 4, period = 4, seed = 0;
    Totals totals{};
};
struct Field {
    Mind *minds = nullptr;
    Tissue *cells = nullptr;
    Provenance *works = nullptr;
    Totals *book = nullptr;
    Word count = 0, arena = 0, mode = 0, period = 4, seed = 0;
    Material *layers = nullptr;
};
static_assert(sizeof(Tissue) == 32 && sizeof(Mind) == 24 && sizeof(Provenance) == 20 && sizeof(Saved) == 164);
AX_FN Word bits(Word packed, Word i) { return (packed >> (2 * i)) & 3; }
AX_FN Word replace(Word value, Word i, Word code) { return (value & ~(3u << (2 * i))) | (code << (2 * i)); }
AX_FN bool matches(Word color, Pattern goal) {
    Word diff = color ^ goal.faces;
    return !(diff & goal.care) || (goal.alternate && !(diff & goal.alternate));
}
AX_FN Word mask_parts(Word parts) {
    Word r = 0;
    for (Word i = 0; i < 4; ++i)
        if (parts & (1u << i))
            r |= 3u << (2 * i);
    return r;
}
AX_FN Word joint(Word joints, Word from, Word to) { return (joints >> (8 * from + 2 * to)) & 3; }
AX_FN void join(Rig &r, Word a, Word b, Word x) {
    r.joints = replace(r.joints, 4 * a + b, x);
    r.joints = replace(r.joints, 4 * b + a, x);
}
AX_FN Word step(Program p, Word i) { return i < 8 ? (p.low >> (3 * i)) & 7 : (p.high >> (3 * (i - 8))) & 7; }
AX_FN void append(Program &p, Word move) {
    if (p.len < 8)
        p.low |= move << (3 * p.len);
    else
        p.high |= move << (3 * (p.len - 8));
    ++p.len;
}
AX_FN Word horizon(Genes g) {
    Word n = 1 + ((g.impulse >> 8) & 15);
    return n < max_steps ? n : max_steps;
}
AX_FN Word expressed(const Mind &m, Word arena) {
    Word n = 1u << ((m.genes.arbor >> 2) & 3);
    return n < arena ? n : arena;
}
AX_FN Word address(const Mind &m, Word glyph, Word arena) {
    return (glyph ^ (m.genes.arbor & 3)) & (expressed(m, arena) - 1);
}
AX_FN Word key_at(const Mind &m, Word slot) { return (m.tags >> (4 * slot)) & 15; }
AX_FN bool valid(const Mind &m, Word arena) {
    if (!arena || arena > 4 || (arena & (arena - 1)) || (m.tags & 0xffff0000u))
        return false;
    for (Word i = 0; i < 4; ++i) {
        Word key = key_at(m, i);
        if (key && (key > 4 || i >= expressed(m, arena) || address(m, key - 1, arena) != i))
            return false;
    }
    return true;
}
AX_FN Genes founder(Word k) { return {mix(k + 8161), mix(k + 4567), mix(k + 7717), mix(k + 9199)}; }
AX_FN Genes child(Genes g, Word k, Word rate) {
    auto mut = [&](Word a, Word salt) {
        Word r = mix(k + salt);
        return (r & 255) < rate ? a ^ (1u << ((r >> 8) & 31)) : a;
    };
    return {mut(g.touch, 631), mut(g.binding, 1451), mut(g.arbor, 8779), mut(g.impulse, 11743)};
}
AX_FN Slots cells(Field f, Word id) { return {f.cells + id, f.count, f.arena}; }
AX_FN bool public_motor(Word mode, Word tag) {
    return !(mode & mark_families) || (mode & equal_families) || (bool(tag) != bool(mode & swap_families));
}
struct Family {
    Genes g{};
    Word eggs = 0;
};
struct Trial {
    Word n = 0, solved = 0, possible = 0, refused = 0, false_plan = 0, taps = 0, optimal = 0, food = 0, best = 0,
         offered = 0, indirect = 0, indirect_solved = 0, multi = 0, multi_solved = 0, fresh = 0, observed = 0,
         proposals = 0, hits = 0, active = 0, compounds = 0, forecast = 0, exact = 0, against = 0;
};
struct Receipt {
    Word seed = 0, width = 0, parts = 0, start = 0, desired = 0, care = 0, handles = 0, joints = 0, glyphs = 0,
         finish = 0;
    Word true_next[4]{}, true_links[2][4]{};
    Genes g{};
    Map map{};
    Program plan{};
    Word reached = 0, spent = 0, shortest = 0, depth = 0, claims = 0;
};
enum School : Word {
    own,
    watched,
    unschooled,
    local_erased,
    seams_erased,
    displaced,
    private_hands,
    joined_only,
    prior_gates,
    blind_tools,
    one_context,
    frozen,
    wrong_witness,
    autonomous,
    destructive,
    school_count
};
} // namespace axion::life::linkage
