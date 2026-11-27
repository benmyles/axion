#pragma once
namespace axion::life::flood {
AX_FN void count(Count &total, Word amount) {
    if (amount)
        asm volatile("red.global.add.u64 [%0],%1;" ::"l"(&total), "l"(Count(amount)) : "memory");
}
struct Pair {
    Word a, b, axis;
};
AX_FN Pair pair(Word k, Word side, Word phase) {
    Word axis = (phase >> 1) & 1, parity = phase & 1;
    Word x = axis ? k % side : 2 * (k % (side / 2)) + parity, y = axis ? 2 * (k / side) + parity : k / (side / 2);
    Word a = cell_at(x, y, side);
    return {a, neighbor(a, axis ? 2 : 1, side), axis};
}
AX_FN Word pour(Half &from, Half &to, Word want) {
    Word moved = cut(want, cut(from, 65535u - to));
    from -= static_cast<Half>(moved);
    to += static_cast<Half>(moved);
    return moved;
}
// Each rooted producer expresses three eight-entry truth tables. Two route
// existing energy between stores and one gates investment in dispersal.
AX_FN Word circumstance(const Silt &p, Word tick, Word season) {
    return Word(p.water > 8 + ((p.root >> 24) & 63)) | (Word(p.grains > 4 + ((p.spore >> 2) & 31)) << 1) |
           (Word(bool(p.enzyme) ^ bool((tick / season) & 1)) << 2);
}
AX_FN void recruit(Silt &a, Silt &b, Word tick, Word key, Configuration c, Counters &ledger) {
    if ((key & 15) != 3)
        return;
    Silt &parent = (key & (1u << 16)) ? a : b, &bed = (&parent == &a) ? b : a;
    auto gene = parent.root;
    Word z = circumstance(parent, tick, c.season), stake = 2 + ((gene >> 28) & 7);
    if (!((gene >> (16 + z)) & 1) || parent.fruit < stake || bed.leaf > 2 + ((bed.root >> 24) & 15))
        return;
    // Propagules draw on the parent's fruit. The resident's accumulated leaf
    // stock remains in the carbon ledger when its occupation is displaced.
    if (Word(bed.litter) + bed.leaf > 65535)
        return;
    pour(bed.leaf, bed.litter, bed.leaf);
    pour(parent.fruit, bed.leaf, stake);
    Word child = gene;
    Byte strain = parent.spore;
    Word mutation = mix(key ^ 0xf45121u);
    if ((mutation & 63) < 1 + ((gene >> 30) & 3)) {
        Word bit = (mutation >> 8) % 40;
        if (bit < 32)
            child ^= 1u << bit;
        else
            strain ^= static_cast<Byte>(1u << (bit - 32));
        count(ledger.plant_edits, 1);
    }
    bed.root = child;
    bed.spore = strain;
    count(ledger.recruits, 1);
    count(ledger.plant_investment, stake);
}
// Seasonal exposure and damp substrate occupy opposite banks of a local vein.
AX_FN bool veined_site(Word site, Configuration c, Word climate) {
    if (climate & 4)
        return true;
    if (!(climate & 8))
        return false;
    return (mix(c.seed ^ mix(((site % c.side) >> 3) + 191 * ((site / c.side) >> 3))) & 1) != 0;
}
AX_FN Word shore_band(Word site, Word tick, Configuration c) {
    Word x = site % c.side, y = site / c.side;
    Word strata = mix(c.seed ^ mix((x >> 3) + 137 * (y >> 3)));
    Word side = (strata & 1) ? x : y, along = (strata & 1) ? y : x;
    return (side + (along >> 2) + (strata >> 12) + (tick / c.season) / 16) & 7;
}
AX_FN void produce(Silt &p, Word site, Word tick, Configuration c, Counters &ledger, bool veined) {
    Word key = mix(c.seed ^ mix(tick + site * 449u)), season = tick / c.season;
    Word wet = p.water >> 3, need = p.spore & 7, grain = p.grains >> 2, period = 24 + ((p.spore >> 4) & 15) * 3;
    bool season_open = (((site % c.side) + (site / c.side) * 3 + season * 7) % period) < (period * 3 / 4);
    Word light = 0;
    if (p.water && grain && season_open && p.leaf < 64 + 3 * (p.spore >> 3)) {
        Word fit = wet > need ? wet - need : need - wet;
        if ((key & 15) < (fit < 8 ? 13 - fit : 1u))
            light = 1 + ((key >> 8) & 3);
    }
    if (veined && shore_band(site, tick, c) >= 2)
        light = 0;
    p.leaf += static_cast<Half>(light);
    count(ledger.light, light);
    if (p.enzyme && p.litter && (((p.enzyme ^ p.spore) & 7) == 0 || (key & 7) == 0))
        count(ledger.converted, pour(p.litter, p.fruit, 1 + ((p.enzyme >> 5) & 3)));
    if (c.flora) {
        Word z = circumstance(p, tick, c.season);
        if ((key & 3) == 1 && ((p.root >> z) & 1))
            pour(p.leaf, p.fruit, 1 + ((p.spore >> 7) & 1));
        if (p.enzyme && ((p.root >> (8 + z)) & 1) && (key & 15) == 7)
            pour(p.litter, p.leaf, 1);
    } else if ((key & 7) == 1 && wet >= need)
        pour(p.leaf, p.fruit, 1 + ((p.spore >> 7) & 1));
    if ((key & 15) == 3 || (!p.water && (key & 3) == 2))
        pour(p.leaf, p.litter, 2);
    if ((key & 63) == 9)
        pour(p.fruit, p.litter, 1);
    if ((key & 127) == 16)
        p.enzyme = 0;
    if ((key & 31) == 13 && p.litter) {
        --p.litter;
        count(ledger.heat, 1);
    }
}
AX_FN Word siphon_inlet(const World &w, Word pos, Word dir) {
    auto &p = w.land[pos];
    auto g = w.matter.bed[pos];
    Word a = g.faces ? material::light_band(g) : (p.spore & 3);
    Word b = g.faces ? material::catalytic_band(g) : ((p.spore >> 2) & 3);
    Word c = g.faces ? material::face(g, dir) : ((p.root >> 8) & 3);
    Word d = g.faces ? material::face(g, dir ^ 2) : ((p.root >> 19) & 3);
    if (w.brine.solutes) {
        Word solvent = brine::phase(w.brine.solutes[pos], p.water);
        b = brine::surface(b, solvent);
    }
    return a | (b << 2) | (c << 4) | (d << 6);
}
AX_FN void currents(World w, Configuration c, Word tick, Word index) {
    auto sites = pair(index, c.side, tick);
    auto a = w.land[sites.a], b = w.land[sites.b];
    auto &all = *w.account;
    Half &wall = sites.axis ? a.south : a.east;
    Half &weave = sites.axis ? a.fibre_south : a.fibre_east;
    auto head = [&](Word site, const Silt &p) {
        return int(p.water) + int(terrain_height(site, c) * 2) + int(p.grains / 4) + tide(site, tick, c);
    };
    int difference = head(sites.a, a) - head(sites.b, b);
    bool direct = difference >= 0;
    auto &from = direct ? a : b, &to = direct ? b : a;
    Word pressure = Word(__builtin_abs(difference)), resist = 2 * Word(wall) + cut(wall, weave) + cut(4, weave / 3);
    if (w.matter.bed && material::hydraulic(w.matter.conditions))
        resist += material::obstruction(w.matter.bed[sites.a], w.matter.bed[sites.b], sites.axis ? 2 : 1);
    if(w.outlay.beds) {
        // The worn depression lowers the first lip. In loose saturated sand its sides
        // instead slump into the stream. The conserved water still moves through pour().
        Word packing=(w.outlay.beds[sites.a]>>(4*sites.axis))&15;
        if(a.water+b.water>100 && ((a.enzyme^b.spore)&1))resist+=packing/3;
        else resist-=cut(resist,packing/3);
    }
    Word key = mix(c.seed ^ mix(tick * 929u + sites.a));
    Word moving = 0;
    if (pressure > resist)
        moving = pour(from.water, to.water, cut(12, (pressure - resist + 1) / 3));
    count(all.flow, moving);
    if (w.brine.solutes && (w.brine.flowing & 1) && moving) {
        auto &donor = w.brine.solutes[direct ? sites.a : sites.b],
             &recipient = w.brine.solutes[direct ? sites.b : sites.a];
        Word volume = from.water + moving, transported = 0;
        for (Word ion = 0; ion < w.brine.species; ++ion) {
            Word a = brine::amount(donor, ion), b = brine::amount(recipient, ion);
            Word dissolved = (moving * a + (mix(key + ion * 531) % volume)) / volume;
            transported |= cut(63 - b, dissolved) << (6 * ion);
        }
        donor.ions -= transported;
        recipient.ions += transported;
    }
    if (w.worksites.ground && moving && ((key >> 16) & 255) < moving) {
        auto &film = w.worksites.ground[direct ? sites.b : sites.a];
        if (film.events) {
            Word oldest = (31 - __builtin_clz(film.events)) & ~7u;
            film.events &= ~(255u << oldest);
            if (!film.events)
                film.tick = 0;
        }
    }
    if (w.clay.ground && (w.clay.material & 3) && moving) {
        Word at = direct ? sites.b : sites.a;
        auto &tile = w.clay.ground[at];
        auto grain = w.matter.bed[at];
        Word shield = weave + ((grain.substance >> 4) & 3);
        Word chance = (moving * ((w.clay.material & 3) == 2 ? 8 : 1)) / (1 + shield);
        if (grain.faces && ((key >> 9) & 255) < chance) {
            Word face = (key >> 23) & 7, old = clay::groove(tile, face);
            if (old) {
                tile.groove[face >> 2] &= ~(255u << (8 * (face & 3)));
                tile.changed = clay::size(tile) ? tick : 0;
                count(w.clay.ledger[c.side >= 128 ? index / 128 : 0].erosion, 1);
            }
        }
    }
    if (w.siphons.ports && moving && ((key >> 12) & 7) < moving) {
        Word enter = direct ? sites.a : sites.b, leave = direct ? sites.b : sites.a;
        Word direction = (sites.axis ? 2u : 1u) ^ (direct ? 0u : 2u), face = direction ^ 2;
        Word gate = (w.siphons.ports[leave] >> (2 * face)) & 3;
        Word filter = ((to.spore >> 4) ^ (to.root >> 12) ^ to.grains) & (w.siphons.bands - 1);
        if (gate == siphon::sensor(siphon_inlet(w, enter, direction), filter)) {
            auto &stock = w.seams.stock ? w.seams.stock[leave] : to.litter;
            Word unlocked = pour(stock, to.fruit, 1 + moving / 6);
            if (w.seams.account)
                count(w.seams.account->released, unlocked);
            if (w.siphons.ledger)
                count(w.siphons.ledger->seep, unlocked);
        }
    }
    if (moving >= 3) {
        count(all.transported, pour(from.grains, to.grains, (key & 7) < moving ? 1 + moving / 7 : 0));
        // Loose fibre travels with the current. Exposed strands trap detritus.
        if (!weave || (key & 15) >= cut(13, weave))
            pour(from.litter, to.litter, (key & 7) < moving ? 1 : 0);
        if ((key & 7) == 0 && from.enzyme)
            to.enzyme = from.enzyme;
        if (!c.flora && from.leaf > to.leaf + 5 && (key & 31) == 1) {
            auto old = to.spore;
            to.spore = from.spore;
            if (((key >> 9) & 31) == 0)
                to.spore ^= static_cast<Byte>(1u << ((key >> 18) & 7));
            count(all.seeded, old != to.spore);
        }
    }
    if (wall && pressure > resist + 12 && (key & 7) == 1)
        count(all.erosion, pour(wall, to.grains, 1));
    if (weave && (key & 31) == 9 && pressure > 6 + cut(24, wall * 2))
    {
        Word frayed=pour(weave,to.litter,1);count(all.frayed,frayed);if(frayed){practice::disturb(w.practice,c.side,sites.a,sites.b);tag_strand(w,c.side,sites.a,sites.b,weave+frayed,weave);}
    }
    if (c.flora == 2)
        recruit(a, b, tick, mix(key + 11987), c, all);
    produce(a, sites.a, tick, c, all, veined_site(sites.a, c, w.tide.active));
    produce(b, sites.b, tick, c, all, veined_site(sites.b, c, w.tide.active));
    if (w.seams.stock) {
        for (Word j = 0; j < 2; ++j) {
            Word pos = j ? sites.b : sites.a;
            auto &soil = j ? b : a;
            Word island = mix(c.seed ^ mix((pos % c.side) / 4 + 193 * ((pos / c.side) / 4)));
            bool binds = w.seams.pattern == 1 || (w.seams.pattern == 2 && (island & 3)) ||
                         (w.seams.pattern == 3 && !(island & 3));
            auto &stock = w.seams.stock[pos];
            Word capacity = binds
                                ? cut(65535, 4 + 3u * soil.grains + 2u * (soil.east + soil.south) +
                                                 (w.matter.bed ? 2u * __builtin_popcount(w.matter.bed[pos].faces) : 0))
                                : 0;
            if (stock > capacity) {
                Word freed = pour(stock, soil.litter, stock - capacity);
                if (w.seams.account)
                    count(w.seams.account->released, freed);
            } else {
                Word rate = 1 + ((soil.grains >> 2) & 3), bound = pour(soil.leaf, stock, cut(rate, capacity - stock));
                bound += pour(soil.litter, stock, cut(rate, capacity - stock));
                if ((key & 3) == 2)
                    bound += pour(soil.fruit, stock, cut(rate, capacity - stock));
                if (w.seams.account)
                    count(w.seams.account->adsorbed, bound);
            }
        }
    }
    capillary(w,c,index,sites.a,sites.b,sites.axis,a,b,weave,tick);
    w.land[sites.a] = a;
    w.land[sites.b] = b;
}
} // namespace axion::life::flood
