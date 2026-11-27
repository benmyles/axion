AX_FN Word expressed_cells(World w, Word slot, const Genome &g, Word tiles) {
    return w.cortex.resident ? w.cortex.resident[slot].active : living_tiles(tiles, g.development);
}
AX_FN GrowthGenes inherit_cortex(GrowthGenes genes, Word key, Word rate) {
    Word a = mix(key ^ 0x272c71d3u), b = mix(key ^ 0x66e23c21u);
    if ((a & 255) < rate)
        genes.response ^= 1u << ((a >> 10) & 31);
    if ((b & 255) < rate)
        genes.regulation ^= 1u << ((b >> 10) & 31);
    return genes;
}
AX_FN void grow_cortex(World w, Configuration c, Word slot, Colony &body, const Silt &ground, Word tick) {
    auto &organ = w.cortex.resident[slot];
    auto &book = *w.cortex.ledger;
    Word program = organ.genes.regulation, age = tick - body.born;
    Word period = 1u << (2 + (program & 3));
    if (!w.cortex.open || (body.cohort && body.cohort == w.cortex.fixed_cohort) || !age || (age & (period - 1)))
        return;
    Word source = ((program >> 10) & 31) % organ.active;
    auto precursor = read_band(w, c.tiles, slot, source);
    Word signals = Word(body.energy >= 16 + ((program >> 16) & 63)) | (Word(body.heard != 0) << 1) |
                   (Word(ground.water >= 8 + ((program >> 22) & 31)) << 2) | (Word(body.load || body.fibre) << 3) |
                   (Word(nbits(precursor.spike) >= ((program >> 27) & 31)) << 4) | (Word(body.action & 1) << 5) |
                   (Word(age & (1u << (4 + ((program >> 2) & 3)))) ? 64u : 0u) |
                   (Word(ground.fruit > ground.leaf) << 7);
    Word address = 0;
    for (Word i = 0; i < 4; ++i)
        address |= ((signals >> ((program >> (4 + 3 * i)) & 7)) & 1) << i;
    Word pulse = (organ.genes.response >> (2 * address)) & 3;
    if (pulse == 2 && organ.active > 2) {
        assign_band(w, c.tiles, slot, --organ.active, {});
        if (organ.active < organ.least)
            organ.least = organ.active;
        count(book.retired, 32);
        return;
    }
    if ((pulse == 1 || pulse == 3) && organ.active < c.tiles && body.energy > 2) {
        Word tile = organ.active;
        auto &g = w.dna[slot].brain;
        assign_band(w, c.tiles, slot, tile,
                    pulse == 3 ? precursor : develop(mix(g.development ^ mix(tick + slot) ^ mix(tile))));
        ++organ.active;
        if (organ.active > organ.peak)
            organ.peak = organ.active;
        body.energy -= 2;
        count(w.account->heat, 2);
        count(book.investment, 2);
        count(book.recruited, 32);
    }
}
