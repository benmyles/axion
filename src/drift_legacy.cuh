// Each monitored geographic edge has a unique slot. Any later change in its
// thickness invalidates the deposited mark, including removal and retying.
struct DriftSurface {
    drift::Mark maker{};
    Word edge = 0, rank = 0;
};
AX_FN DriftSurface find_drift_surface(World w, Word side, const DriftSquare &sites, Word order, Word stamp) {
    for (Word j = 0; j < 4; ++j) {
        Word e = (j + order) & 3, rank = drift::surface(side, sites.edge_root(e), e & 1, w.drift_marks.salt);
        if (rank < w.drift_marks.watched) {
            auto m = w.drift_marks.items[rank];
            if (m.stamp && m.stamp < stamp)
                return {m, e, rank};
        }
    }
    return {};
}
AX_FN void inspect_drift_surface(World w, Word side, Word tiles, const DriftSquare &sites, Word tile, Word stamp,
                                 drift::Law law, const drift::View &view, DriftSurface s, drift::LegacyWatch watch,
                                 Word outlet, Word amount, Word priority, Word aims, Word capacities, Word meals,
                                 Word eaten) {
    auto &t = watch.counts[tile & 127];
    count(t.revisited, 1);
    auto bare = view;
    if (!bare.bed.fibre[s.edge])
        return;
    --bare.bed.fibre[s.edge];
    Word under = drift::connections(bare, law.ports & drift::fibres, law.ports & drift::seepage);
    Word alt =
        law.ports & drift::winding
            ? drift::woven_path(bare, under, law.ports & drift::fibres, law.ports & drift::seepage).run(view.inlet)
            : drift::outlet(view.inlet, under);
    if (alt == outlet)
        return;
    Word other = 0, self = 0, foreign = 0, young = 0, beneficiary = 0, priority_seen = 0;
    const auto &maker = w.animals[s.maker.owner];
    bool dead = !maker.energy || maker.born != s.maker.born;
    for (Word k = 0; k < 4; ++k) {
        Word j = (k + priority) & 3, cap = (capacities >> (5 * j)) & 31, got = (meals >> (5 * j)) & 31;
        if (((aims >> (2 * j)) & 3) == alt)
            other += cut(cap, amount - other);
        Word id = w.slots[sites[j]];
        Word priority_now = got ? 1 + Word(id != s.maker.owner || w.animals[id].born != s.maker.born) +
                                      Word(w.animals[id].born > s.maker.stamp)
                                : 0;
        if (priority_now > priority_seen) {
            beneficiary = j;
            priority_seen = priority_now;
        }
        if (id == s.maker.owner && w.animals[id].born == s.maker.born)
            self += got;
        else {
            foreign += got;
            if (w.animals[id].born > s.maker.stamp)
                young += got;
        }
    }
    count(t.diverted, 1);
    count(t.actual, eaten);
    count(t.without, other);
    count(t.gained, eaten > other ? eaten - other : 0);
    count(t.lost, other > eaten ? other - eaten : 0);
    count(t.self, self);
    count(t.foreign, foreign);
    count(t.after_death, dead ? foreign : 0);
    count(t.younger, young);
    count(t.orphans, dead ? young : 0);
    if (foreign)
        asm volatile("red.global.max.u64 [%0],%1;" ::"l"(&t.oldest), "l"(Count(stamp - s.maker.stamp)) : "memory");
    Word quality = 1 + Word(foreign != 0) + Word(young != 0) + Word(dead && foreign) + Word(dead && young);
    if (watch.samples && s.rank < watch.limit && quality > watch.samples[s.rank].rank) {
        auto &r = watch.samples[s.rank];
        r.view = view;
        r.maker = s.maker;
        r.side = side;
        r.origin = sites[0];
        r.tile = tile;
        r.tick = stamp;
        r.edge = s.edge;
        r.outlet = outlet;
        r.alternative = alt;
        r.load = amount;
        r.priority = priority;
        r.aims = aims;
        r.capacities = capacities;
        r.meals = meals;
        r.maker_now = maker.born;
        r.maker_energy = maker.energy;
        r.rank = quality;
        r.beneficiary = beneficiary;
        Word recipient = w.slots[sites[beneficiary]];
        const auto &body = w.animals[recipient];
        r.shape = body.body.shape;
        r.limbs = body.body.limbs;
        r.eyes = body.body.eyes;
        r.pores = body.body.pores;
        r.cells = body.energy ? 32 * expressed_cells(w, recipient, w.dna[recipient].brain, tiles) : 0;
        for (Word j = 0; j < 4; ++j) {
            r.who[j] = w.slots[sites[j]];
            r.born[j] = w.animals[r.who[j]].born;
        }
    }
}
