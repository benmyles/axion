// A two-beat observer follows measured pump-induced pressure into an adjacent
// living receptor. Every alternative conditions on the recipient's sampled bed.
AX_FN void pass_chemical_pulse(capillary::PortGallery gallery, Word world, Word index, Word x, Word y, Word role,
                               Word pump, Word baseline, bool spring, bool connected, bool actual,
                               capillary::Reception r) {
    Word dx = (x - gallery.x) & (world - 1), dy = (y - gallery.y) & (world - 1), width = gallery.side;
    if (dx >= width || dy >= width)
        return;
    Word site = dy * width + dx, area = width * width;
    auto &next = gallery.origins[Count((index + 1) & 1) * area + site];
    Word no_pump = (r.input & ~1u) | (baseline & 1);
    if (role == 1 && pump && actual && !capillary::surge(no_pump, r.rock)) {
        Word meta = 1 | (no_pump << 1) | (r.input << 7) | (r.rock << 13) | (Word(r.bare != r.move) << 18);
        next = {r.reader, r.reader_born, meta};
    } else
        next.signal = 0;
    if (role != 2 || !connected)
        return;
    for (Word axis = 0; axis < 2; ++axis) {
        if (!((r.input >> axis) & 1) || (!axis && spring) || (width < world && (axis ? dy == 0 : dx == 0)))
            continue;
        Word local = axis ? ((dy - 1) & (width - 1)) * width + dx : dy * width + ((dx - 1) & (width - 1));
        auto prior = gallery.origins[Count(index & 1) * area + local];
        if (!prior.signal || (prior.writer == r.reader && prior.born == r.reader_born))
            continue;
        auto v = r;
        v.writer = prior.writer;
        v.writer_born = prior.born;
        v.writer_data = prior.signal;
        v.line = axis;
        v.source = axis ? ((y - 1) & (world - 1)) * world + x : y * world + ((x - 1) & (world - 1));
        v.counterfactual = v.input & ~(1u << axis);
        v.other_light = bool(v.counterfactual & 3);
        bool different = v.other_light != v.light;
        if (!different) {
            v.other_move = v.move;
            v.other_bare = v.bare;
        }
        v.other_cast = v.available && (!v.sorting || v.other_light) && v.other_move;
        Word alternative_heat = v.heat - v.cast + v.other_cast;
        v.other_food = v.other_cast && Word(capillary::surge(v.counterfactual, v.rock)) == v.wants
                           ? drift::low(v.stock, drift::low(v.mouthful, 65535u - v.energy + alternative_heat))
                           : 0;
        bool learned_source = bool(v.writer_data & (1u << 18)), changed = v.move != v.other_move;
        bool learned_response = changed && v.bare == v.other_bare;
        auto &t = *gallery.counts;
        Count rank;
        asm volatile("atom.global.add.u64 %0,[%1],1;" : "=l"(rank) : "l"(&t.pairs) : "memory");
        count(t.sole_source, different);
        count(t.changed_move, changed);
        count(t.acquired_source, learned_source);
        count(t.acquired_response, learned_response);
        count(t.learned_both, learned_source && learned_response);
        count(t.gained, v.food > v.other_food ? v.food - v.other_food : 0);
        count(t.missed, v.other_food > v.food ? v.other_food - v.food : 0);
        if (rank < gallery.capacity)
            gallery.records[rank] = v;
    }
}
