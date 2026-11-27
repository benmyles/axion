// The intake carries four independent material properties. The outlet is a
// rotating, persistent slit looking at one property of the incident stream.
AX_FN siphon::Scene siphon_scene(World w, Word inlet, Word outlet, Word dir, Word a, Word b, Word tick, Word key) {
    siphon::Scene r{};
    auto &source = w.land[inlet], &bed = w.land[outlet];
    auto &system = w.siphons;
    r.inlet = siphon::spectrum(siphon_inlet(w, inlet, dir), system.bands);
    r.membrane = ((bed.spore >> 4) ^ (bed.root >> 12) ^ bed.grains) & (system.bands - 1);
    Word face = dir ^ 2;
    r.opening = (system.ports[outlet] >> (2 * face)) & 3;
    r.carbon = w.seams.stock ? w.seams.stock[outlet] : bed.litter;
    r.area = 4 + cut(16, source.water / 4) + 2 * __builtin_popcount(w.matter.bed[inlet].faces);
    r.charged = source.water && bed.water;
    r.source_pose = w.motor[a] & 7;
    r.sink_pose = w.motor[b] & 7;
    Word visibility = Word((source.water >> 4) == (bed.water >> 4) || ((key >> 16) & 3) == 0);
    if ((system.windows & 3) == 2 || ((system.windows & 3) && visibility))
        r.seen = Word(w.animals[a].body.eyes != 0) | 2u * Word(w.animals[b].body.eyes != 0);
    r.shared = siphon::open_view(system.windows, key) & r.seen;
    r.resonant = (system.windows & 16) != 0;
    return r;
}
AX_FN void siphon_encounter(World w, Configuration c, Word left, Word right, Word left_id, Word right_id,
                            Word direction, Word tick, Word key) {
    auto &system = w.siphons;
    auto &ledger = *system.ledger;
    if ((system.windows & 128) || (key & (system.pulse - 1)) || w.animals[left_id].energy < 4 ||
        w.animals[right_id].energy < 4)
        return;
    bool reverse = (key >> 11) & 1;
    Word inlet = reverse ? right : left, outlet = reverse ? left : right, dir = reverse ? (direction ^ 2) : direction;
    Word first = reverse ? right_id : left_id, second = reverse ? left_id : right_id;
    auto &caller = w.animals[first], &listener = w.animals[second];
    auto scene = siphon_scene(w, inlet, outlet, dir, first, second, tick, key);
    if (!scene.charged || !scene.carbon)
        return;
    auto &a = system.residents[first], &b = system.residents[second];
    Word channel = c.wire & 3;
    Word wire = channel && system.wire ? (channel == 1 ? system.wire : channel) : 0;
    if (w.clay.minds) {
        clay_orient(w, outlet, first, second, key, c.plastic);
        clay_orient(w, inlet, second, first, key + 221, c.plastic);
    }
    auto action = siphon::meet(a, b, scene, key, wire);
    // A grip needs a limb. A receptive surface can still hear and make sound.
    if (!listener.body.limbs)
        action.stroke = 4;
    auto change = siphon::squeeze(scene, action, b.genes.anatomy);
    Word shift = 2 * (dir ^ 2);
    system.ports[outlet] = static_cast<Byte>((system.ports[outlet] & ~(3u << shift)) | (change.angle << shift));
    auto &bed = w.land[outlet];
    Word food = cut(change.food, 65535u - bed.fruit);
    change.food = food;
    change.speaker_share = food / 2;
    change.receiver_share = food - change.speaker_share;
    if (w.seams.stock) {
        w.seams.stock[outlet] -= static_cast<Half>(food);
        count(w.seams.account->released, food);
    } else
        bed.litter -= static_cast<Half>(food);
    bed.fruit += static_cast<Half>(food);
    caller.energy -= static_cast<Half>(change.cost_a);
    listener.energy -= static_cast<Half>(change.cost_b);
    Word fed_a = caller.body.pores ? pour(bed.fruit, caller.energy, change.speaker_share) : 0;
    Word fed_b = listener.body.pores ? pour(bed.fruit, listener.energy, change.receiver_share) : 0;
    caller.meals += fed_a;
    caller.diet[1] += fed_a;
    listener.meals += fed_b;
    listener.diet[1] += fed_b;
    Word spent = change.cost_a + change.cost_b, speech = Word(action.query < 4) + Word(action.reply < 4);
    count(w.account->heat, spent);
    count(w.account->harvest, fed_a + fed_b);
    count(w.account->digested, fed_a + fed_b);
    count(ledger.pairs, 1);
    count(ledger.inquiries, action.query < 4);
    count(ledger.replies, action.reply < 4);
    count(ledger.strokes, action.stroke < 4);
    count(ledger.transmitted, wire ? speech : 0);
    count(ledger.released, food);
    count(ledger.consumed, fed_a + fed_b);
    count(ledger.native_work, spent);
    count(ledger.sights, action.stroke < 4 ? __builtin_popcount(scene.seen) : 0);
    Word edits = siphon::learn(a, b, scene, action, change, wire, key, c.plastic);
    count(ledger.field_edits, edits);
    if (w.clay.minds) {
        Word q = siphon::carried(action.query, wire, key + 121), v = siphon::carried(action.reply, wire, key + 367);
        bool worked = action.stroke < 4,
             wet = worked && ((system.windows & 3) == 2 ||
                              ((system.windows & 3) &&
                               ((w.land[inlet].water >> 4) == (w.land[outlet].water >> 4) || ((key >> 16) & 3) == 0)));
        clay_write(w, outlet, second, q, scene.membrane, true, wet, food != 0, tick, key);
        clay_write(w, inlet, first, v, action.percept, false, wet, food != 0, tick, key + 619);
        if (w.clay.use & clay::latent) {
            if (worked && (scene.seen & 1))
                clay::listen(w.clay.minds[first], a, siphon::radiate(0, q, scene.membrane, true, true, food != 0));
            if (worked && (scene.seen & 2))
                clay::listen(w.clay.minds[second], b, siphon::radiate(0, v, action.percept, false, true, food != 0));
        }
    }
    if (system.air) {
        Word ticket = 1 + (mix(key + tick * 127) % 1048575);
        a.volley = (a.volley & 4095) | (ticket << 12);
        b.volley = (b.volley & 4095) | (ticket << 12);
        bool visible = (system.windows & 3) == 2 ||
                       ((system.windows & 3) &&
                        ((w.land[inlet].water >> 4) == (w.land[outlet].water >> 4) || ((key >> 16) & 3) == 0));
        visible = visible && action.stroke < 4;
        auto ignite = [&](Word site, Word glyph, Word glint, bool outlet_rattle) {
            Word depth = 0;
            if (system.access & 2) {
                auto &s = w.land[site];
                Word organics = s.fibre_east + s.fibre_south;
                Word cavity = s.east + s.south + __builtin_popcount(w.matter.bed[site].faces);
                depth = cut(9, 1 + cut(3, organics) + cut(3, cavity / 3) + 2 * Word(change.food != 0));
            }
            siphon::ring(system.air[site],
                         siphon::radiate(ticket, glyph, glint, outlet_rattle, visible, change.food != 0), tick, depth);
        };
        ignite(outlet, siphon::carried(action.query, wire, key + 121), scene.membrane, true);
        ignite(inlet, siphon::carried(action.reply, wire, key + 367), action.percept, false);
    }
}

AX_FN Word audience_pulse(World w, Configuration c, Word pos, Word id, Word tick, bool plastic) {
    auto &sys = w.siphons;
    auto &animal = sys.residents[id];
    auto &body = w.animals[id];
    if (!sys.air || !tick || !sys.coupling || !(c.wire & 3) ||
        !(animal.genes.volition & (1u << (24 + (body.action & 7)))))
        return 0;
    Word above = (sys.access & 1) ? 5u : 1u, seed = mix(c.seed ^ mix(id + tick * 329)), best = 0, chosen = 0;
    for (Word j = 0; j < above; ++j) {
        Word loc = j ? neighbor(pos, (body.heading + j - 1) & 3, c.side) : pos;
        auto f = sys.air[loc];
        Word age = siphon::daylight(f, tick);
        if (!age || age > siphon::tail(f.pulse) || !(f.pulse & 128) || (f.pulse >> 12) == (animal.volley >> 12))
            continue;
        if (sys.coupling >= 4 && age <= tick - body.born)
            continue;
        Word drive =
            1 + (mix(seed + loc * 131) >> 24) + ((siphon::tail(f.pulse) - age) * ((animal.genes.lateral >> 28) & 3));
        if (drive > best) {
            best = drive;
            chosen = f.pulse;
        }
    }
    if (!best)
        return 0;
    if (sys.coupling == 2)
        chosen = (chosen & ~3u) | ((chosen + 1) & 3);
    if (sys.coupling == 3 || sys.coupling == 5)
        chosen = (chosen & ~3u) | (mix(seed + 781) & 3);
    bool seen = body.body.eyes && (chosen & 32);
    Word edits = siphon::overhear(animal, chosen, seen, plastic);
    auto &book = sys.audience[c.side >= 128 ? id / 256 : 0];
    count(book.heard, 1);
    count(book.paired, seen);
    count(book.edits, edits);
    count(book.young, tick - body.born < 16);
    return (1u << ((chosen & 3) + 4 * Word((chosen & 16) != 0)));
}
