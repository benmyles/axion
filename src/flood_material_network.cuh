namespace {
AX_FN Word mineral_atomic(Word *address, Word x) {
    Word old;
    asm volatile("atom.global.min.u32 %0,[%1],%2;" : "=r"(old) : "l"(address), "r"(x) : "memory");
    return old;
}
AX_FN Word mineral_root(Word *parent, Word node) {
    while (true) {
        Word next;
        asm volatile("ld.volatile.global.u32 %0,[%1];" : "=r"(next) : "l"(parent + node) : "memory");
        if (next == node)
            return node;
        node = next;
    }
}
AX_FN void join_grains(Word *parent, Word a, Word b) {
    a = mineral_root(parent, a);
    b = mineral_root(parent, b);
    while (a != b) {
        Word hi = a > b ? a : b, lo = a > b ? b : a;
        Word was = mineral_atomic(parent + hi, lo);
        if (was == hi || was == lo)
            return;
        a = mineral_root(parent, was);
        b = mineral_root(parent, lo);
    }
}
} // namespace
KERNEL void flood_material_labels(Word *parent, Word n) {
    Word i = tid();
    if (i < n)
        parent[i] = i;
}
KERNEL void flood_material_join(const material::Grain *bed, Word *parent, Word side, Word open) {
    Word i = tid();
    if (i >= side * side || !open || !bed[i].faces)
        return;
    for (Word d = 1; d < 3; ++d) {
        Word j = neighbor(i, d, side);
        if (material::bonded(bed[i], bed[j], d))
            join_grains(parent, i, j);
    }
}
KERNEL void flood_material_components(const material::Grain *bed, Word *parent, material::Component *tables, Word side,
                                      Word open) {
    Word i = tid();
    if (i >= side * side || !bed[i].faces)
        return;
    auto id = mineral_root(parent, i);
    auto *record = tables + id;
    Word deg = open ? attachments(bed, i, side) : 0, e = 0;
    if (open)
        for (Word d = 1; d < 3; ++d)
            e += material::bonded(bed[i], bed[neighbor(i, d, side)], d);
    auto add = [](Word &target, Word n) {
        if (n)
            asm volatile("red.global.add.u32 [%0],%1;" ::"l"(&target), "r"(n) : "memory");
    };
    add(record->size, 1);
    add(record->edges, e);
    add(record->branches, deg >= 3);
    add(record->handled, (bed[i].marks & 2) != 0);
    add(record->charge, bed[i].charge);
}
KERNEL void flood_material_flux(material::Grain *bed, const Silt *soil, Half *reagent, Half *product,
                                material::Flux *flux, Configuration c, Word start, Word offset, Word mode, Word climate,
                                const brine::Salts *solutes) {
    Word i = tid();
    if (i >= c.side * c.side / 2)
        return;
    Word tick = start + offset;
    auto p = pair(i, c.side, tick & 3);
    auto &a = bed[p.a], &b = bed[p.b];
    bool touched = (a.marks | b.marks) & 2, open = !mode || (mode == 2 && !touched);
    Word passage = material::conduct(a, b, p.axis ? 2 : 1, open);
    flux[p.a].passed += passage;
    if (open && ((tick + 3 * p.a) & 31) == 0 && material::bonded(a, b, p.axis ? 2 : 1) &&
        a.charge + 1 >= material::capacity(a) && b.charge && material::exchange(a, b, p.axis ? 2 : 1))
        --b.charge;
    for (Word end = 0; end < 2; ++end) {
        Word site = end ? p.b : p.a;
        auto effect = material::excite(bed[site], mineral_sky(site, tick, c, climate),
                                       brine::surface(wet_surface(soil[site], site, tick, c, climate),
                                                      solutes ? brine::phase(solutes[site], soil[site].water) : 0),
                                       reagent[site], product[site]);
        flux[site].made += effect.reaction;
    }
}
KERNEL void flood_material_stock(const Silt *land, Half *reagent, Half *product, Word n) {
    Word i = tid();
    if (i < n) {
        reagent[i] = land[i].litter;
        product[i] = land[i].fruit;
    }
}
KERNEL void flood_material_rewind(material::Grain *twin, const material::Grain *present, Word side, Word seed) {
    Word dest = tid(), n = side * side;
    if (dest >= n)
        return;
    // Permute within disjoint 4x4 neighborhoods, keeping each entire grain
    // (including its charge and chemical groups) on the same local substrate.
    Word x = dest % side, y = dest / side, bx = x & ~3u, by = y & ~3u, key = mix(seed ^ mix(bx + by * side)),
         index = (x & 3) + 4 * (y & 3);
    Word k = (index * ((key & 7) * 2 + 1) + ((key >> 11) & 15)) & 15, src = bx + (k & 3) + (by + (k >> 2)) * side;
    twin[dest] = present[src];
}
