// The mark contains the two actual materials and the observed outcome.
// Neither the animal's hidden motor identity nor its learned weights enter soil.
namespace worksite {
AX_FN material::Inquiry founder(Word key) { return {mix(key + 407), mix(key + 6151)}; }
AX_FN material::Inquiry inherit(const material::Inquiry &p, Word key, Word rate) {
    auto vary = [&](Word word, Word salt) {
        Word v = mix(key + salt);
        return (v & 255) < rate ? word ^ (1u << ((v >> 11) & 31)) : word;
    };
    return {vary(p.receptors, 211), vary(p.impulses, 1627)};
}
AX_FN Word evidence(Word category, Word left, Word right, bool changed) {
    return 128 | (category << 6) | (Word(changed) << 5) | (left << 2) | right;
}
AX_FN Word surface_color(const material::Genes &g, Word band) { return band ^ ((g.muscle >> 26) & 3); }
AX_FN bool consume_observation(material::Inquiry &p, material::Foresight &f, const material::Genes &g, Word mark) {
    if (!(mark & 128))
        return false;
    Word left = (mark >> 2) & 3, right = mark & 3, pulse = (mark >> 5) & 1;
    if (mark & 64) {
        Word odor = right ^ material::scree::wild_odor(g), pair = 4 * surface_color(g, left) + odor;
        bool fresh = !(p.sampled & (1u << pair));
        p.sampled |= 1u << pair;
        p.chemistry = material::scree::observe(p.chemistry, pair, pulse, g.practice);
        f.appetite = material::scree::observe(f.appetite, odor, pulse, g.practice);
        return fresh;
    }
    Word glass = material::scree::wild_glass(g), a = left ^ glass, b = right ^ glass;
    bool fresh = !(f.appetite & (1u << (16 + 4 * a + b)));
    material::scree::plume(f, g, a, b, pulse);
    return fresh;
}
AX_FN void inscribe(World w, Word pos, Word at, Word mark) {
    if (!mark)
        return;
    auto &film = w.worksites.ground[pos];
    film.events = (film.events << 8) | mark;
    film.tick = at;
    count(w.worksites.ledger->inscriptions, 1);
}
AX_FN Word exchange_results(Word events, bool made_only) {
    Word answer = events;
    for (Word i = 0; i < 4; ++i) {
        Word a = (events >> (8 * i)) & 255;
        if (!(a & 128) || (made_only && !(a & 16)))
            continue;
        for (Word j = 1; j <= 4; ++j) {
            Word b = (events >> (8 * ((i + j) & 3))) & 255;
            if (!(b & 128) || ((a ^ b) & 64) || (made_only && !(b & 16)))
                continue;
            answer = (answer & ~(32u << (8 * i))) | ((b & 32u) << (8 * i));
            break;
        }
    }
    return answer;
}
AX_FN void read(World w, Configuration c, Word pos, Word id, Word tick) {
    auto &body = w.animals[id];
    auto &sites = w.worksites;
    auto &p = sites.residents[id];
    if (!sites.marks || !body.body.eyes || !(p.impulses & (1u << (24 + (w.motor[id] & 7)))))
        return;
    if (c.side >= 128)
        sites.ledger += id / 256;
    Word span = 1 + cut(4u, nbits(body.body.eyes)), best = 0, chosen = pos;
    Word salt = mix(c.seed ^ mix(tick + id * 331));
    for (Word j = 0; j < span; ++j) {
        Word at = j ? neighbor(pos, (body.heading + j - 1) & 3, c.side) : pos;
        auto here = sites.ground[at];
        if (!here.events)
            continue;
        if (sites.marks >= 3 && ((here.tick != ~0u && here.tick >= body.born) || !(here.events & 0x10101010u)))
            continue;
        Word tag = mix(here.events ^ mix(here.tick + at * 19));
        auto &g = w.matter.residents[id].genes;
        Word pending = here.events & 255, kind = (pending >> 6) & 1, left = (pending >> 2) & 3, right = pending & 3;
        Word pair = kind ? 4 * surface_color(g, left) + (right ^ material::scree::wild_odor(g))
                         : 4 * (left ^ material::scree::wild_glass(g)) + (right ^ material::scree::wild_glass(g));
        Word exposed = kind ? (p.sampled & 65535) : (w.tide.cells[id].appetite >> 16);
        Word interest = __builtin_popcount(exposed ^ 65535) + 4 * Word(!(exposed & (1u << pair)));
        Word score = 1 + ((mix(salt + at) >> 26) & 31) + ((p.receptors >> 26) & 3) * interest;
        if (tag != p.last_seen && score > best) {
            chosen = at;
            best = score;
        }
    }
    if (!best)
        return;
    auto film = sites.ground[chosen];
    p.last_seen = mix(film.events ^ mix(film.tick + chosen * 19));
    Word events = film.events;
    if (sites.marks == 2 || sites.marks == 4)
        events = exchange_results(events, sites.marks == 4);
    Word heard = 0;
    for (Word j = 0; j < 4; ++j) {
        Word mark = (events >> (8 * j)) & 255;
        if ((mark & 128) && (sites.marks < 3 || (mark & 16))) {
            consume_observation(p, w.tide.cells[id], w.matter.residents[id].genes, mark);
            ++heard;
        }
    }
    if (w.brine.animals) {
        auto &cells = w.brine.animals[id];
        for (int j = 3; j >= 0; --j) {
            Word mark = (events >> (8 * j)) & 255;
            if ((mark & 192) == 192 && (sites.marks < 3 || (mark & 16)))
                brine::observe(cells.genes, cells.memory, mark & 15, (mark >> 5) & 1);
        }
    }
    count(sites.ledger->readings, heard);
    if (film.tick == ~0u || film.tick < body.born)
        count(sites.ledger->before_birth, heard);
}
// The ordinary recurrent somatic circuit receives predictions through
// heritable afferents. All light patterns are in this individual’s own frame.
AX_FN Word afferent(World w, Configuration c, Word pos, Word id) {
    auto &p = w.worksites.residents[id];
    auto &f = w.tide.cells[id];
    auto &worker = w.matter.residents[id];
    Word dir = w.animals[id].heading, adj = neighbor(pos, dir, c.side);
    auto a = worker.hand.faces ? worker.hand : w.matter.bed[pos];
    auto b = w.matter.bed[adj];
    auto &g = worker.genes;
    Word glass = material::scree::wild_glass(g);
    Word shape = 4 * (material::face(a, dir) ^ glass) + (material::face(b, dir ^ 2) ^ glass);
    Word smell =
        4 * surface_color(g, material::catalytic_band(a)) + ((w.land[pos].spore & 3) ^ material::scree::wild_odor(g));
    Word pulses = Word(a.faces && b.faces && material::scree::fires(f.contacts, shape, g.release)) |
                  (Word(a.faces && w.land[pos].litter && material::scree::fires(p.chemistry, smell, g.release)) << 1) |
                  (((p.sampled >> smell) & 1) << 2) | (((f.appetite >> (16 + shape)) & 1) << 3);
    if (w.brine.animals && !(w.brine.flowing & 2) && a.faces) {
        auto &cells = w.brine.animals[id];
        auto weights = brine::efficacy(cells.genes, cells.memory);
        auto votes =
            brine::estimate(cells.genes, weights, brine::cue(material::catalytic_band(a), w.land[pos].spore & 3));
        pulses = (pulses & ~2u) | (Word(brine::impulse(cells.genes, votes, brine::mass(weights))) << 1);
    }
    return rotate((pulses & ((p.receptors >> 8) & 15)) * 0x11111111u, (p.impulses >> 15) & 31);
}
AX_FN void shed(World w, Word home, Word other, Word id) {
    auto &cargo = w.matter.residents[id].hand;
    if (w.animals[id].energy || !cargo.faces)
        return;
    auto &floor = w.matter.bed[w.matter.bed[home].faces ? other : home];
    if (floor.faces)
        return;
    clay_carry(w, w.matter.bed[home].faces ? other : home, other, id, 4);
    floor = cargo;
    cargo = {};
    count(w.worksites.ledger->released, 1);
}
AX_FN Word choose(const material::Inquiry &p, Word sensory, Word soma, Word key) {
    Word cue = material::index(sensory, p.receptors), bias = (p.impulses >> (3 * cue)) & 7;
    if ((key & 255) < ((p.receptors >> 21) & 63))
        return (key >> 12) & 7;
    Word start = (key >> 16) & 7, pick = start;
    int best = -1;
    for (Word j = 0; j < 8; ++j) {
        Word act = (start + j) & 7;
        Word curiosity = (p.anticipation >> (2 * act)) & 3, feeding = (p.anticipation >> (16 + 2 * act)) & 3;
        int value = Word(act == bias) * ((p.impulses >> 24) & 7) + Word(act == soma) * ((p.receptors >> 10) & 3) +
                    ((p.impulses >> (27 + curiosity)) & 1) * ((p.receptors >> 13) & 3) +
                    ((p.receptors >> (17 + feeding)) & 1) * ((p.impulses >> 29) & 7);
        if (value > best) {
            best = value;
            pick = act;
        }
    }
    return pick;
}
AX_FN Word lift_interest(Word remaining, Word key) {
    if (!remaining)
        return key & 7;
    Word shift = key & 7, rolled = (remaining >> shift) | (remaining << (8 - shift));
    return (shift + __builtin_ctz(rolled)) & 7;
}
AX_FN void practice(World w, Configuration c, Word pos, Word other, Word dir, Word slot, Word motor, Word tick,
                    Word key) {
    auto &sites = w.worksites;
    auto &body = w.animals[slot];
    auto &p = sites.residents[slot];
    auto &worker = w.matter.residents[slot];
    auto &f = w.tide.cells[slot];
    auto &g = worker.genes;
    if (!sites.active || !w.matter.conditions.agency || !body.body.limbs || body.energy < 4 ||
        ((key >> 24) & 3) > ((p.impulses >> 27) & 3))
        return;
    auto &ground = w.matter.bed[pos], &away = w.matter.bed[other];
    Word cue =
        material::sense(ground, away, worker.hand, dir, Word(body.energy > 24) + 2 * Word(tick - body.born < 32));
    Word action = choose(p, cue, motor, key), heat = 0, pumped = 0, novelty = 0, eaten = 0;
    count(sites.ledger->visits, 1);
    if (!action)
        return;
    if ((action & 1) && body.energy > 2) {
        // The palm can reach either face of this contact. An unexperienced
        // tendon is selected in proprioceptive coordinates before it is fired.
        bool full = worker.hand.faces, reverse = (key >> 6) & 1;
        if (full && bool(ground.faces) != bool(away.faces))
            reverse = ground.faces != 0;
        if (!full && bool(ground.faces) != bool(away.faces))
            reverse = ground.faces == 0;
        Word here = reverse ? other : pos, there = reverse ? pos : other, d = reverse ? (dir ^ 2) : dir;
        auto &a = w.matter.bed[here], &b = w.matter.bed[there];
        auto before = a;
        Word shift = full ? 24 : 16, remaining = (~(p.sampled >> shift)) & 255;
        Word m = lift_interest(remaining, mix(key + 911)), op = material::effector(g, m);
        bool eligible = full ? !a.faces : bool(a.faces);
        auto e = material::manipulate(a, b, worker.hand, op, d, w.matter.attachment[here], w.matter.attachment[there],
                                      body.energy, 0);
        if (e.performed)
            clay_carry(w, here, there, slot, op);
        if (c.plastic) {
            material::scree::motor_event(f, g, m, full, material::quantity(worker.hand), before, a);
            material::practice(worker, m, e);
            if (eligible) {
                novelty |= (remaining >> m) & 1;
                p.sampled |= 1u << (shift + m);
            }
        }
        heat += e.spent;
        body.energy -= static_cast<Half>(e.spent);
        count(sites.ledger->actions, e.performed);
        if (e.performed) {
            count(w.matter.ledger->lifts, op == 3);
            count(w.matter.ledger->placements, op == 4);
            count(w.matter.ledger->turns, op == 1 || op == 2);
            count(w.matter.ledger->translations, op == 5 || op == 6);
            count(w.matter.ledger->exchanges, op == 7);
        }
    }
    material::Grain *first = worker.hand.faces ? &worker.hand : (ground.faces ? &ground : &away);
    material::Grain *second = worker.hand.faces ? (away.faces ? &away : &ground) : (&ground == first ? &away : &ground);
    // Compression turns paid body work into stored excitation. The thermodynamic
    // account distinguishes it from incident light and dissipated handling work.
    if ((action & 6) && first->faces && body.energy > 3) {
        Word charge = 1 + ((p.receptors >> 28) & 3);
        pumped = cut(charge, cut(material::capacity(*first) - first->charge, body.energy - 2u));
        first->charge += static_cast<Byte>(pumped);
        body.energy -= static_cast<Half>(pumped);
    }
    if ((action & 2) && first->faces && second->faces && body.energy > 1) {
        Word d = (dir + ((key >> 9) & 3)) & 3;
        bool forward = first->charge >= second->charge + 1;
        auto &hi = forward ? *first : *second, &lo = forward ? *second : *first;
        bool eligible = hi.charge >= lo.charge + 2 && lo.charge < material::capacity(lo);
        Word x = material::face(hi, forward ? d : d ^ 2), y = material::face(lo, forward ? d ^ 2 : d);
        Word current = material::conduct(*first, *second, d, material::conducts(w.matter.conditions));
        if (eligible) {
            Word mark = evidence(0, x, y, current);
            if (c.plastic)
                novelty |= consume_observation(p, f, g, mark);
            inscribe(w, pos, tick, mark | 16);
            count(sites.ledger->contacts, 1);
        }
        count(w.matter.ledger->transfers, current);
        ++heat;
        --body.energy;
    }
    if ((action & 4) && first->faces && body.energy > 1) {
        Word at = (key & (1u << 19)) ? pos : other;
        Word band = material::catalytic_band(*first);
        if (w.seams.stock) {
            // A receptive map can choose the surface before an actual press.
            // Exploration and reversal of the approach signal remain mutable.
            Word soil0 = w.land[pos].spore & 3, soil1 = w.land[other].spore & 3;
            Word x = 4 * surface_color(g, band);
            auto response = [&](Word odor) {
                return material::scree::fires(p.chemistry, x + (odor ^ material::scree::wild_odor(g)), g.release);
            };
            bool go0 = response(soil0), go1 = response(soil1);
            if (w.brine.animals && !(w.brine.flowing & 2)) {
                auto &cells = w.brine.animals[slot];
                auto weights = brine::efficacy(cells.genes, cells.memory);
                Word left = brine::estimate(cells.genes, weights, brine::cue(band, soil0));
                Word right = brine::estimate(cells.genes, weights, brine::cue(band, soil1));
                go0 = left > right;
                go1 = right > left;
            }
            count(w.seams.account->alternatives, soil0 != soil1);
            if (go0 != go1 && w.seams.guidance && (key & 15) >= ((p.impulses >> 20) & 7)) {
                bool reverse = ((p.receptors >> 23) & 7) == 7;
                at = (go0 ^ reverse) ? pos : other;
                count(w.seams.account->differential, 1);
            }
        }
        auto &soil = w.land[at];
        Word surface = wet_surface(soil, at, tick, c, w.tide.active);
        Word liquid = w.brine.solutes ? brine::phase(w.brine.solutes[at], soil.water) : 0;
        Word effective = brine::surface(surface, liquid);
        Word trapped = w.seams.stock ? w.seams.stock[at] : 0;
        if ((soil.litter || trapped) && surface < 4 && first->charge >= 1 + ((first->substance >> 5) & 3) &&
            soil.fruit <= 65533) {
            material::Rates reaction{};
            Word food = 0;
            if (trapped) {
                count(w.seams.account->presses, 1);
                if (band == effective) {
                    Word area = 2 + __builtin_popcount(first->faces) + ((first->substance >> 1) & 3);
                    Word unlocked = cut(trapped, cut(area, 65534u - soil.fruit));
                    if (unlocked) {
                        --first->charge;
                        w.seams.stock[at] -= static_cast<Half>(unlocked);
                        food = unlocked + 1;
                        soil.fruit += static_cast<Half>(food);
                        reaction.reaction = 1;
                        reaction.folds = material::fold(*first);
                        count(w.seams.account->released, unlocked);
                    }
                }
            } else {
                reaction = material::excite(*first, 4, effective, soil.litter, soil.fruit);
                food = 2 * reaction.reaction;
            }
            if (c.plastic && w.brine.animals) {
                auto &cells = w.brine.animals[slot];
                brine::observe(cells.genes, cells.memory, brine::cue(band, surface), reaction.reaction);
            }
            Word mark = evidence(1, band, surface, reaction.reaction);
            if (c.plastic)
                novelty |= consume_observation(p, f, g, mark);
            inscribe(w, at, tick, mark | 16);
            count(sites.ledger->scrapes, 1);
            count(sites.ledger->reactions, reaction.reaction);
            count(w.matter.ledger->reactions, reaction.reaction);
            count(w.matter.ledger->folds, reaction.folds);
            if (food && body.body.pores && (p.impulses & (1u << (24 + (motor & 7))))) {
                eaten = pour(soil.fruit, body.energy, food);
                body.meals += eaten;
                body.diet[1] += eaten;
                count(w.account->harvest, eaten);
                count(w.account->digested, eaten);
                if (trapped)
                    count(w.seams.account->meals, eaten);
            }
        }
        ++heat;
        --body.energy;
    }
    if (c.plastic) {
        p.anticipation = material::scree::observe(p.anticipation, action, novelty, g.observation);
        p.anticipation = material::scree::observe(p.anticipation, action + 8, eaten != 0, g.observation);
    }
    count(sites.ledger->pumped, pumped);
    count(sites.ledger->eaten, eaten);
    count(sites.ledger->heat, heat);
    count(w.account->heat, heat);
    count(w.matter.ledger->heat, heat);
}
} // namespace worksite
