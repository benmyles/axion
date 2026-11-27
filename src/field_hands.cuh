// One site owns its two sills and both fibre stocks. Transport and ordinary
// foraging run after these transfers on the same CUDA stream.
AX_FN bool hand_beat(Word tick, Word i, Word side, capillary::Settings law) {
    return i < side * side && !(tick & ((1u << law.shift) - 1));
}
AX_FN Word hand_sample(Word site, Word beat, Word seed, Word sites) {
    return (site * 17477u + mix(beat + seed * 31u)) & (sites - 1);
}
AX_FN capillary::CompoundMind hand_cortex(World w, Configuration c, Word slot, Word band) {
    auto organ = watershed_forager(w, c, slot, band);
    return capillary::compound_body(organ.body, 0, w.grip_contacts[slot]);
}
AX_FN void keep_hand_spikes(World w, Configuration c, Word slot, Word band, const capillary::CompoundMind &organ) {
    auto tissue = read_band(w, c.tiles, slot, band);
    if (tissue.spike != organ.body.cells.spike) {
        tissue.spike = organ.body.cells.spike;
        assign_band(w, c.tiles, slot, band, tissue);
    }
}
KERNEL void flood_grasp_material(World w, Configuration c, const Word *clock, Word offset, capillary::Device deck,
                                 capillary::Hands h) {
    using namespace capillary;
    Word i = tid(), tick = *clock + offset, side = c.side, n = side * side;
    if (!hand_beat(tick, i, side, deck.law))
        return;
    h.pending[i] = 0;
    Word slot = w.slots[i];
    auto &body = w.animals[slot];
    if (body.energy < 8 || !body.body.eyes || !body.body.limbs || !c.weaving || (deck.law.mode & 7) == 2)
        return;
    Word beat = tick >> deck.law.shift, coin = mix(c.seed ^ mix(i + beat * 9239u));
    auto &g = w.dna[slot];
    Word act = body.action & 7;
    if (!((g.grip >> act) & 1) || !((g.pigments[0] >> (16 + act)) & 1) || ((coin >> 12) & 3))
        return;
    Word x = i & (side - 1), y = i / side, words = (n + 31) / 32;
    auto &site = w.land[i];
    auto head = upstream_head(c, deck, beat, i, x, y, words, site);
    Word sight = head.a | (head.b << 1), before = sight | (capillary_grain(site, head.threshold) << 2);
    Word band = regulation::auditory_band(g, expressed_cells(w, slot, g.brain, c.tiles));
    auto mind = hand_cortex(w, c, slot, band), bare = mind;
    bare.tissue = 0;
    Word action = compose_move(mind, sight, coin), cold = compose_move(bare, sight, coin);
    keep_hand_spikes(w, c, slot, band, mind);
    bool recycling = deck.law.mode & 8, ghost = deck.law.mode & disconnected_hands;
    Word prediction = before, cold_row = before;
    Word feasible = candidate_edge(prediction, site, body, action, head.threshold, recycling);
    Word alternate = candidate_edge(cold_row, site, body, cold, head.threshold, recycling);
    if (ghost)
        cold_row = before;
    MaterialExchange inventory{};
    if (feasible && !ghost)
        inventory = exchange_capillary_piece(w, c, i, x, y, action, site, body, recycling);
    Word actual = sight | (capillary_grain(site, head.threshold) << 2);
    Word rock0 = capillary::rock(deck.law, c.seed, x, y);
    auto pad = deck.waves + 2 * words;
    Word coat = ((pad[i >> 5] >> (i & 31)) & 1) | (((pad[words + (i >> 5)] >> (i & 31)) & 1) << 1);
    Word carrier = w.braids.floats[i] & 3;
    bool reactive = deck.law.mode & catalytic_surface;
    Word stone = reactive ? conditioned_mineral(rock0, coat, carrier) : rock0;
    Word flow = surge(actual, stone), cold_flow = surge(cold_row, stone), untouched = surge(before, stone);
    --body.energy;
    ++body.worked;
    count(w.account[i >> 8].heat,1);
    Word meal = body.diet[2] & 7;
    h.pending[i] = 128 | (meal << 4) | (action << 2) | sight;
    auto &sum = h.totals[(i >> 7) & 127];
    count(sum.attempts, 1);
    count(sum.paid, 1);
    count(sum.unavailable, !feasible);
    count(sum.placed, feasible && !ghost && !inventory.edge0);
    count(sum.lifted, feasible && !ghost && inventory.edge0);
    count(sum.retained_moves, mind.tissue != 0);
    count(sum.different_part, action != cold);
    count(sum.different_flow, flow != cold_flow);
    count(sum.created_flow, flow && !untouched);
    count(sum.removed_flow, !flow && untouched);
    count(sum.young, tick - body.born < 64u);
    count(sum.direction[action], 1);
    Word rank = hand_sample(i, beat, c.seed, n);
    if (rank < h.samples)
        h.camera[rank] = {tick + 1,
                          i,
                          slot,
                          body.born,
                          mind.fields,
                          sight,
                          rock0,
                          coat,
                          carrier,
                          reactive,
                          stone,
                          before,
                          actual,
                          cold_row,
                          untouched,
                          flow,
                          cold_flow,
                          action,
                          cold,
                          feasible,
                          alternate,
                          Word(feasible && !ghost),
                          inventory.donation,
                          inventory.edge0,
                          inventory.edge1,
                          inventory.cargo0,
                          inventory.cargo1,
                          meal,
                          meal,
                          0,
                          0,
                          mind.tissue,
                          mind.tissue};
}
KERNEL void flood_taste_after_grasp(World w, Configuration c, const Word *clock, Word offset, capillary::Device deck,
                                    capillary::Hands h) {
    Word i = tid(), tick = *clock + offset, n = c.side * c.side;
    if (!hand_beat(tick, i, c.side, deck.law))
        return;
    Word receipt = h.pending[i];
    if (!receipt)
        return;
    Word slot = w.slots[i];
    auto &body = w.animals[slot];
    Word after = body.diet[2] & 7;
    // Only the intervening pressure kernel can feed here. Its mouthful is at most four.
    Word eaten = (after - ((receipt >> 4) & 7)) & 7, edits = 0;
    if (c.plastic && !(deck.law.mode & capillary::fixed_hands)) {
        auto &g = w.dna[slot];
        Word band = regulation::auditory_band(g, expressed_cells(w, slot, g.brain, c.tiles));
        auto mind = hand_cortex(w, c, slot, band);
        edits = nourish(mind, receipt & 3, (receipt >> 2) & 3, eaten);
        w.grip_contacts[slot] = mind.tissue;
        keep_hand_spikes(w, c, slot, band, mind);
    }
    auto &sum = h.totals[(i >> 7) & 127];
    count(sum.food, eaten);
    count(sum.new_food, eaten != 0);
    count(sum.edits, edits);
    Word rank = hand_sample(i, tick >> deck.law.shift, c.seed, n);
    if (rank < h.samples) {
        auto &note = h.camera[rank];
        note.later_diet = after;
        note.food = eaten;
        note.edits = edits;
        note.tissue1 = w.grip_contacts[slot];
    }
}
KERNEL void flood_hand_owners(World w, Word n, capillary::HandOwner *out) {
    Word i = tid();
    if (i < n)
        out[i] = {w.animals[i].born, w.animals[i].energy ? w.grip_contacts[i] : 0u};
}
