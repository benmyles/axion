#include "axion/detritus.hpp"

namespace axion::life::flood {
template <bool record>
AX_FN void drift_grains(World w, Configuration c, const Word *clock, Word offset, capillary::Device deck,
                        detritus::Ledger *accounts, detritus::Recording film) {
    Word tick = *clock + offset, step = tick >> deck.law.shift;
    if (tick & ((1u << deck.law.shift) - 1))
        return;
    Word tile = tid(), span = c.side / 2, total = span * span;
    if (tile >= total)
        return;
    Word mask = c.side - 1, x = (2 * (tile & (span - 1)) + (step & 1)) & mask,
         y = (2 * (tile / span) + ((step >> 1) & 1)) & mask;
    Word east = (x + 1) & mask, south = (y + 1) & mask;
    Word cell[]{y * c.side + x, y * c.side + east, south * c.side + x, south * c.side + east};
    Word words = (c.side * c.side + 31) / 32;
    auto wave = deck.waves + ((step + 1) & 1) * words;
    Word mirror = (deck.law.mode & detritus::displaced) ? (c.side / 2) * (c.side + 1) : 0;
    auto sense = [](const Word *image, Word where) { return (image[where >> 5] >> (where & 31)) & 1; };
    Word gates = 0, local = 0, changes = 0, stocked = 0;
    detritus::Bed b{};
    for (Word j = 0; j < 4; ++j) {
        auto &soil = w.land[cell[j]];
        b.food[j] = soil.litter;
        b.water[j] = soil.water;
        if (j < 3) {
            gates |= sense(wave, cell[j] ^ mirror) << j;
            stocked += bool(b.food[j]);
            if constexpr (record) {
                local |= sense(wave, cell[j]) << j;
                changes |= sense(deck.impulses, cell[j] ^ mirror) << j;
            }
        }
    }
    const auto &nw = w.land[cell[0]], &ne = w.land[cell[1]], &sw = w.land[cell[2]];
    b.sill[0] = nw.east;
    b.sill[1] = nw.south;
    b.sill[2] = ne.south;
    b.sill[3] = sw.east;
    b.seam[0] = nw.fibre_east;
    b.seam[1] = nw.fibre_south;
    b.seam[2] = ne.fibre_south;
    b.seam[3] = sw.fibre_east;
    Word priority = mix(c.seed ^ mix(cell[0] + step * 12373)) & 3;
    auto motion = detritus::route(b, gates, priority);
    Word moved = motion.moved, offered = motion.offered;
    detritus::Half next[4]{};
    Word before = 0, after = 0, arrivals = 0;
    for (Word j = 0; j < 4; ++j) {
        next[j] = detritus::Half(int(b.food[j]) + detritus::balance(moved, j));
        if (next[j] != b.food[j])
            w.land[cell[j]].litter = next[j];
        before += b.food[j];
        after += next[j];
    }
    // Count incoming edges independently of the source count, including through-flow.
    for (Word j = 1; j <= 3; ++j)
        arrivals += __builtin_popcount(moved & (j == 3 ? 12u : j));
    Word paint = 0, colour = 0, recoloured = 0;
    bool visible = deck.law.mode & detritus::exchanges;
    if (visible && (moved || record)) {
        for (Word j = 0; j < 4; ++j)
            paint |= Word(w.braids.floats[cell[j]] & 3) << (2 * j);
        colour = detritus::stain(paint, moved, priority);
        for (Word j = 0; j < 4; ++j)
            if (((colour ^ paint) >> (2 * j)) & 3) {
                auto &surface = w.braids.floats[cell[j]];
                surface = (surface & ~3u) | ((colour >> (2 * j)) & 3);
                ++recoloured;
            }
    }
    Word enabled = 0, prevented = 0;
    if constexpr (record) {
        Word altered[3]{};
        for (Word j = 0; j < 3; ++j) {
            altered[j] = moved;
            if (changes & (1u << j)) {
                altered[j] = detritus::route(b, gates ^ (1u << j), priority).moved;
                enabled += __builtin_popcount(moved & ~altered[j]);
                prevented += __builtin_popcount(altered[j] & ~moved);
            }
        }
        Word uniform = (tile * 14821 + mix(step ^ c.seed)) & (total - 1),
             fixed = (tile * 17239 + mix(c.seed + 1753)) & (total - 1);
        if (uniform < film.uniform || ((enabled || prevented) && fixed < film.watched)) {
            detritus::Note snap{};
            snap.bed = b;
            snap.surface = visible;
            snap.paint_before = paint;
            snap.paint_after = colour;
            snap.tick = tick + 1;
            snap.tile = tile;
            snap.x = x;
            snap.y = y;
            snap.epoch = step;
            snap.mode = mirror ? 2 : 1;
            snap.local = local;
            snap.gates = gates;
            snap.changes = changes;
            snap.priority = priority;
            snap.offered = offered;
            snap.moved = moved;
            for (Word j = 0; j < 4; ++j)
                snap.final[j] = next[j];
            for (Word j = 0; j < 3; ++j) {
                snap.alternate[j] = altered[j];
                auto id = w.slots[cell[j] ^ mirror];
                snap.actor[j] = id;
                snap.born[j] = w.animals[id].born;
            }
            if (uniform < film.uniform)
                film.notes[uniform] = snap;
            if ((enabled || prevented) && fixed < film.watched)
                film.notes[film.uniform + fixed] = snap;
        }
    }
    auto &r = accounts[tile / 128 & 127];
    auto group = drift_warp(true, 0, false);
#define AX_CARGO(name, value) drift_count(r.name, Word(value), group)
    AX_CARGO(tiles, 1);
    AX_CARGO(gates, __builtin_popcount(gates));
    AX_CARGO(stocked, stocked);
    AX_CARGO(offers, __builtin_popcount(offered));
    AX_CARGO(departures, __builtin_popcount(moved));
    AX_CARGO(arrivals, arrivals);
    AX_CARGO(blocked, __builtin_popcount(offered ^ moved));
    AX_CARGO(east, __builtin_popcount(moved & 9));
    AX_CARGO(south, __builtin_popcount(moved & 6));
    AX_CARGO(mass_before, before);
    AX_CARGO(mass_after, after);
    if (visible) {
        AX_CARGO(recoloured, recoloured);
    }
    if constexpr (record) {
        AX_CARGO(stimulus_changes, __builtin_popcount(changes));
        AX_CARGO(food_enabled, enabled);
        AX_CARGO(food_prevented, prevented);
    }
#undef AX_CARGO
}
KERNEL void flood_detritus(World w, Configuration c, const Word *clock, Word offset, capillary::Device deck,
                           detritus::Ledger *accounts, detritus::Recording film) {
    drift_grains<false>(w, c, clock, offset, deck, accounts, film);
}
KERNEL void flood_detritus_record(World w, Configuration c, const Word *clock, Word offset, capillary::Device deck,
                                  detritus::Ledger *accounts, detritus::Recording film) {
    drift_grains<true>(w, c, clock, offset, deck, accounts, film);
}
} // namespace axion::life::flood

KERNEL void flood_detritus_proof(axion::life::detritus::Proof *results) {
    using namespace axion::life::detritus;
    Word i = tid();
    if (i >= proof_cases)
        return;
    auto b = boundary_case(i);
    auto m = route(b, (i >> 8) & 7, (i >> 11) & 3);
    Proof p{0, m.offered, m.moved};
    for (Word j = 0; j < 4; ++j)
        p.final |= Count(Half(int(b.food[j]) + balance(m.moved, j))) << (16 * j);
    results[i] = p;
}
