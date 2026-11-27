struct PressOffer {
    const material::Grain *body;
    const clay::Tile *surface;
    Word face, cue;
};
struct PressTable {
    PressOffer tools[2];
    Word bath, slit, stock;
};
AX_FN PressTable press_table(World w, Word here, Word across, Word id, Word other, Word dir, Word hand, Word layer) {
    auto &own = w.matter.residents[id].hand, &offered = w.matter.residents[other].hand;
    auto &ground = w.matter.bed[here], &away = w.matter.bed[across];
    bool lend = offered.faces && w.animals[other].energy &&
                (w.clay.minds[other].genes.branches & (1u << (8 + (w.motor[other] & 7))));
    auto &first = own.faces ? own : away;
    auto &second = lend ? offered : ground;
    Word other_face = hand ^ (1u << ((w.clay.minds[id].genes.fingers >> 30) & 1));
    auto &bed = w.land[here];
    Word bath = (bed.spore >> 4) & 3;
    Word offset = ((bed.east + bed.south) >> 2) + (ground.faces ? material::face(ground, dir) : (bed.root >> 12));
    if (w.clay.use & press::bedded)
        offset = layer + ((bed.east + bed.south) >> 4) + (ground.marks & 1);
    return {{{&first, own.faces ? &w.clay.palms[id] : &w.clay.ground[across], hand, press::appearance(first, hand)},
             {&second, lend ? &w.clay.palms[other] : &w.clay.ground[here], other_face,
              press::appearance(second, other_face)}},
            bath,
            (bath + offset) & 3,
            Word(w.seams.stock ? w.seams.stock[here] : bed.litter)};
}
AX_FN Word press_layer(Configuration c, Word pos, Word tick, Word physics) {
    Word patch = (pos % c.side) / 16 + 4096 * ((pos / c.side) / 16);
    Word phase = mix(c.seed ^ mix(patch + 671));
    return (phase + ((physics & press::tidal) ? (tick + (phase >> 24)) / 128 : 0)) & 3;
}
AX_FN void temper_grain(World w, Configuration c, Word pos, Word tick, Word key) {
    auto &grain = w.matter.bed[pos];
    // Moist ground under cyclic loading slowly anneals bare crystal. A held
    // grain retains its orientation as it travels. Bound charge and mass stay
    // in the same carrier, and existing impressions keep their recorded pitches.
    if (grain.faces && w.land[pos].water && !(key & 15)) {
        Word lattice = press_layer(c, pos, tick, press::bedded);
        if ((grain.marks & 1) && !(lattice & 2)) {
            // A rigid lattice locks in the currently exposed ports.
            grain.faces = material::rotate_faces(grain.faces, 1);
            grain.marks &= ~1u;
        }
        grain.substance = (grain.substance & 63) | (lattice << 6);
    }
}
struct Palpation {
    Word context = 0, chosen = 0, schedule = 0, cost = 0, heard[2]{4, 4}, redirected = 0;
};
AX_FN Palpation feel_table(const PressTable &table, const clay::Mind &m, Word use, Word key, Word reserve, bool eyes,
                           Word rim_note, bool wet, Word prescribed = 8) {
    Palpation r{};
    r.context = table.bath;
    const auto &a = table.tools[0], &b = table.tools[1];
    bool active[2]{a.body->faces != 0, b.body->faces != 0};
    bool guide = (use & press::choosing) && eyes;
    Word baseline = (!active[0] || (active[1] && press::choose(m, a.cue, b.cue, table.bath, key, guide)));
    r.chosen = baseline;
    if (!(use & press::feeling) || !(use & press::choosing))
        return r;
    Word fee = (use & press::stiff) ? 3 : 1, high = Word(fee > 1 || reserve < 8);
    auto willing = [&](Word port, Word rules, bool ambiguous) {
        bool fire =
            prescribed < 8 ? bool(prescribed & (1u << port)) : bool((rules >> (2 * high + Word(ambiguous))) & 1);
        return fire && reserve > r.cost + fee + 1;
    };
    Word genes = m.genes.membrane;
    if (wet && willing(0, genes >> 8, __builtin_popcount(press::values(m, table.bath)) != 1)) {
        r.schedule |= 1;
        r.cost += fee;
        r.context = rim_note;
    }
    Word currents[2]{guide ? press::drive(m, a.cue, r.context) : 12u, guide ? press::drive(m, b.cue, r.context) : 12u};
    Word start = !active[0] || (active[1] && press::elect(currents[0], currents[1], key));
    for (Word i = 0; i < 2; ++i) {
        Word hand = start ^ i;
        auto &v = table.tools[hand];
        if (!active[hand])
            continue;
        bool ambiguous = currents[0] == currents[1] || __builtin_popcount(press::predictions(m, v.cue)) != 1;
        if (!willing(hand + 1, genes >> 12, ambiguous))
            continue;
        r.cost += fee;
        r.schedule |= 2u << hand;
        Word signal = press::ring(*v.surface, *v.body, v.face, use & press::bedded);
        if (use & press::dislocated)
            signal = (signal + 1 + (mix(key + hand * 727 + 979) % 3)) & 3;
        r.heard[hand] = signal;
        Word pulse = (1u << signal) ^ ((0u - ((genes >> 16) & 1)) & 15);
        currents[hand] = press::current(m, pulse, r.context);
    }
    r.chosen = !active[0] || (active[1] && press::elect(currents[0], currents[1], key));
    r.redirected = r.chosen != baseline;
    return r;
}
AX_FN Word rim_vibration(const PressTable &table, const Silt &bed, Word geology) {
    // The unloaded resonator includes the inertia of its fibre lining.
    return (table.slit + ((geology >> 18) & 3) + ((bed.fibre_east + bed.fibre_south) >> 5)) & 3;
}
// One edge owns both animals, their hands and the two pieces of ground during
// this phase. Every withdrawal has a matching deposit or a heat entry.
AX_FN void press_contact(World w, Configuration c, Word here, Word across, Word id, Word other, Word dir, Word tick,
                         Word key) {
    auto &animal = w.animals[id], &peer = w.animals[other];
    auto &m = w.clay.minds[id];
    Word pose = w.motor[id] & 7, use = w.clay.use;
    if (!animal.body.limbs || animal.energy < 3 || !(m.genes.fingers & (1u << pose)))
        return;
    Word hand = (pose + dir + ((w.motor[id] >> 3) & 3)) & 7;
    auto table = press_table(w, here, across, id, other, dir, hand, press_layer(c, here, tick, use));
    auto &first = table.tools[0], &second = table.tools[1];
    if (!first.body->faces && !second.body->faces)
        return;
    bool alternative = first.body->faces && second.body->faces && first.cue != second.cue && animal.body.eyes;
    auto contact = feel_table(table, m, use, key, animal.energy, animal.body.eyes,
                              rim_vibration(table, w.land[here], c.seed), w.land[here].water);
    auto &chosen = table.tools[contact.chosen];
    auto &tool = *chosen.body;
    auto &source = *chosen.surface;
    Word face = chosen.face, cue = chosen.cue, tone = press::ring(source, tool, face, use & press::bedded);
    auto &target = w.clay.ground[here];
    auto &ground = w.matter.bed[here];
    auto &bed = w.land[here];
    Word slit = table.slit, available = table.stock;
    bool loaded = bed.water && available;
    Word food = press::flow(tone, slit, bed.water, available);
    food = cut(food, 65535u - bed.fruit);
    auto &book = w.clay.ledger->press;
    animal.energy -= static_cast<Half>(1 + contact.cost);
    count(w.account->heat, 1 + contact.cost);
    count(book.heat, 1 + contact.cost);
    if (contact.schedule) {
        count(book.rims, contact.schedule & 1);
        count(book.tips, __builtin_popcount(contact.schedule >> 1));
        count(book.probe_heat, contact.cost);
        count(book.redirected, contact.redirected);
    }
    if (w.seams.stock) {
        w.seams.stock[here] -= static_cast<Half>(food);
        count(w.seams.account->released, food);
    } else
        bed.litter -= static_cast<Half>(food);
    bed.fruit += static_cast<Half>(food);
    Word swallowed = animal.body.pores ? pour(bed.fruit, animal.energy, food) : 0;
    animal.meals += swallowed;
    animal.diet[1] += swallowed;
    count(w.account->harvest, swallowed);
    count(w.account->digested, swallowed);
    count(book.strokes, 1);
    count(book.alternatives, alternative);
    count(book.wet, loaded);
    count(book.open, food != 0);
    count(book.released, food);
    count(book.eaten, swallowed);
    bool previous = clay::groove(source, face) & 128;
    if ((use & press::casting) && bed.water && ground.faces && &target != &source) {
        Word into = press::mirror(face, dir + ground.marks);
        bool written = press::cast(target, into, source, tool, face, food != 0, tick, use & press::bedded);
        count(book.printed, written);
        count(book.recast, written && previous);
    }
    if(w.hollows.minds && animal.body.eyes) {
        remember_here(w,c,id,here,4*hollow::implement+cue,swallowed!=0,tick,key);
        if(loaded) remember_here(w,c,id,here,4*hollow::inlet+table.bath,swallowed!=0,tick,mix(key+1927));
    }
    if (!c.plastic)
        return;
    if (animal.body.eyes && contact.schedule)
        for (Word hand = 0; hand < 2; ++hand)
            if (contact.heard[hand] < 4)
                count(book.visual_edits, press::see(m, table.tools[hand].cue, contact.heard[hand], false));
    if (animal.body.eyes)
        count(book.visual_edits, press::see(m, cue, tone, false));
    if ((use & press::tasting) && loaded)
        count(book.taste_edits, press::taste(m, contact.context, tone, swallowed != 0));
    if ((use & press::attending) && peer.energy && peer.body.eyes && peer.heading == (dir ^ 2u)) {
        auto &visitor = w.clay.minds[other];
        if (visitor.genes.fingers & (1u << (8 + (w.motor[other] & 7)))) {
            count(book.witnessed, 1);
            count(book.visual_edits, press::see(visitor, cue, tone, true));
            if ((use & press::tasting) && loaded)
                count(book.taste_edits, press::taste(visitor, contact.context, tone, food != 0));
        }
    }
}
