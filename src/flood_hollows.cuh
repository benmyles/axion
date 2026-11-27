AX_FN Word hollow_texture(World w, Word pos) {
    auto &p = w.land[pos];
    auto b = w.hollows.beds[pos];
    if (b.relief)
        return hollow::texture(b);
    Word faces = w.matter.bed ? w.matter.bed[pos].faces : 0;
    return (p.spore ^ (p.root >> 12) ^ faces ^ (p.east << 2) ^ p.south) & 63;
}
AX_FN void remember_here(World w, Configuration c, Word id, Word pos, Word cue, bool food, Word tick, Word key) {
    auto &f = w.hollows;
    auto &m = f.minds[id];
    auto slots = hollow::cells(f, id);
    if (!c.plastic || !(f.mode & hollow::changing))
        return;
    Word texture = hollow_texture(w, pos);
    auto change = hollow::observe(m, slots, cue, texture, food, tick, key, f.mode);
    if (change.slot == hollow::missing)
        return;
    count(f.ledger->pulses, 1);
    count(f.ledger->rewrites, change.prior != slots.at(change.slot));
    if (!(id & 15)) {
        auto &audit = f.sites[Count(change.slot) * ((f.stride + 15) / 16) + id / 16];
        bool certified =
            (audit & 0x80000000u) && (audit & 0x7fffffffu) == pos + 1 && hollow::identity(change.prior) == cue;
        count(f.ledger->same_place, certified);
        count(f.ledger->certified_food, certified && food);
        audit = slots.at(change.slot) ? pos + 1 : 0;
    }
}
AX_FN void hollow_weather(World w, Configuration c, Word pos, Word tick, Word key) {
    auto &f = w.hollows;
    auto &b = f.beds[pos];
    auto &site = w.land[pos];
    Word geology = mix(c.seed ^ mix(pos + 1439));
    if (!b.relief) {
        Word mask = (f.mode & hollow::rich_beds) ? 15u : 63u;
        if (!(geology & mask) && site.grains >= 3 && site.water >= 3) {
            b.relief = 1 + ((site.spore ^ (site.root >> 10) ^ (geology >> 15)) & 63);
            b.cover = 1 + ((geology >> 9) & 3);
            count(f.ledger->formed, 1);
        }
        return;
    }
    Word cap = 12 + ((geology >> 9) & 63) + 4 * b.cover;
    Word uptake = 1 + ((f.mode & hollow::rich_beds) ? 5u : 1u) + (site.water >> 6);
    if (site.water && b.carbon < cap && !(key & 3)) {
        auto &loose = site.fruit ? site.fruit : site.litter;
        auto captured = pour(loose, b.carbon, cut(uptake, cap - b.carbon));
        count(f.ledger->settled, captured);
    }
    if (site.water > 48 && !(key & 31) && !b.cover) {
        Word spilled = pour(b.carbon, site.litter, 1 + site.water / 16);
        count(f.ledger->released, spilled);
        if (!b.carbon && (key & 511) == 0 && (geology & 63))
            b = {};
    }
    if (b.cover && site.water > 32 && !(key & 255))
        --b.cover;
}
AX_FN void hollow_work(World w, Configuration c, Word id, Word pos, Word tick, Word key) {
    auto &f = w.hollows;
    auto &a = w.animals[id];
    auto &m = f.minds[id];
    auto &bed = f.beds[pos];
    auto &soil = w.land[pos];
    if (!a.energy)
        return;
    if (bed.relief && a.body.eyes)
        count(f.ledger->corrections, hollow::orient(m, hollow::cells(f, id), hollow::texture(bed), f.mode));
    if (!(f.mode & hollow::digging) || a.energy < 3 || !a.body.limbs)
        return;
    Word pose = w.motor[id] & 7;
    bool press = (m.genes.hands >> (8 + pose)) & 1;
    bool store = (m.genes.hands >> (24 + pose)) & 1;
    if (!bed.relief && store && a.energy >= 40 && soil.grains >= 6 && soil.east < 65535 && !(key & 127)) {
        // A displaced clay grain becomes part of the bank. Work alters the
        // local cover without synthesizing a resource.
        --soil.grains;
        ++soil.east;
        bed.relief = 1 + ((soil.spore ^ (soil.root >> 10) ^ (w.matter.bed ? w.matter.bed[pos].faces : 0)) & 63);
        bed.cover = 2;
        --a.energy;
        count(w.account->heat, 1);
        count(f.ledger->heat, 1);
        count(f.ledger->formed, 1);
    }
    if (!bed.relief)
        return;
    Word cue = 4 * hollow::harvest + hollow::material(bed);
    if (store && a.energy > 42 + ((m.genes.retrieval >> 19) & 31) && bed.carbon < 64 && ((key >> 10) & 3) == 0) {
        Word paid = 1 + ((m.genes.compass >> 19) & 7);
        a.energy -= static_cast<Half>(paid + 1);
        bed.carbon += paid;
        bed.cover = cut(15, bed.cover + 1);
        count(f.ledger->placed, paid);
        count(f.ledger->heat, 1);
        count(w.account->heat, 1);
        remember_here(w, c, id, pos, cue, false, tick, key);
    } else if (press && a.energy > 2) {
        Word work = 1 + (bed.cover >> 2);
        if (a.energy <= work)
            return;
        a.energy -= static_cast<Half>(work);
        count(f.ledger->heat, work);
        count(w.account->heat, work);
        if (bed.cover)
            --bed.cover;
        Word demand = 3 + nbits(a.body.limbs) + ((m.genes.compass >> 20) & 3), sampled = 0;
        if (a.body.pores)
            sampled = pour(bed.carbon, a.energy, demand);
        if(w.outlay.minds)effort_meal(w,c,id,cue&3,sampled);
        a.meals += sampled;
        a.diet[1] += sampled;
        count(w.account->harvest, sampled);
        count(w.account->digested, sampled);
        count(f.ledger->released, sampled);
        count(f.ledger->eaten, sampled);
        count(f.ledger->opened, 1);
        count(f.ledger->repeat_food, sampled && (m.efferent & 0x40));
        remember_here(w, c, id, pos, cue, sampled, tick, key);
    }
}
AX_FN Word hollow_goal(World w, Word id, Word pos) {
    auto &m = w.hollows.minds[id];
    Word laden = w.matter.residents && w.matter.residents[id].hand.faces;
    Word state = Word(w.animals[id].energy < 48) + 2 * laden;
    Word kind = (m.genes.retrieval >> (24 + 2 * state)) & 3;
    Word mask = 15;
    if (kind == hollow::implement && w.clay.minds) {
        Word b = (w.land[pos].spore >> 4) & 3;
        mask = hollow::best_tools(w.clay.minds[id], b);
        if (!mask && (m.genes.compass & (1u << 11)))
            mask = 15;
    }
    return mask << (kind * 4);
}
AX_FN Word hollow_steer(World w, Configuration c,Word id, Word pos, Word normal, Word tick, Word key, Word surface) {
    auto &f = w.hollows;
    auto &m = f.minds[id];
    auto &a = w.animals[id];
    Word apply = (m.genes.hands >> (16 + (w.motor[id] & 7))) & 1;
    Word before = m.efferent, turn = normal;
    m.efferent = before & hollow::lineage_tag;
    bool matched = hollow::may_steer(f.mode, before);
    bool pursuing=w.outlay.minds && effort::remaining(w.outlay.minds[id]) && (w.outlay.minds[id].experience&(1u<<11)) && (w.outlay.minds[id].trace&(1u<<9));
    if ((apply || pursuing) && (matched || (f.mode & hollow::crossed_motor)) && (f.mode & hollow::directing) && m.used &&
        a.body.limbs) {
        Word cues = hollow_goal(w, id, pos);
        auto fired = hollow::recall(m, hollow::cells(f, id), cues, tick, key, (before >> 8) & 15);
        bool permit=!w.outlay.minds || effort_route(w,c,id,pos,fired,tick,key,(before>>8)&15,surface);
        if (permit && fired.slot != hollow::missing) {
            count(f.ledger->recalls, 1);
            if (fired.range && fired.compass < 4) {
                turn = (fired.compass + Word(!matched)) & 3;
                m.efferent |= 16 | turn | ((fired.slot + 1) << 16);
                count(f.ledger->steered, turn != normal);
            } else if (fired.range == 0 && (before & 16) && ((before >> 16) & 63) == fired.slot + 1) {
                m.efferent |= 64;
                count(f.ledger->arrivals, 1);
            }
        }
    }
    if(w.outlay.minds && (w.outlay.mode&effort::selective) && effort_access(w.outlay,m.efferent)){
        const auto &trip=w.outlay.minds[id];
        if(effort::remaining(trip) && !(trip.trace&(1u<<9)) && (trip.genes.episode&(1u<<30)))m.efferent|=1u<<13;
    }
    hollow::turn(m, turn);
    return turn;
}
AX_FN void hollow_step(World w, Word id, Word from, Word relative, bool walked = true) {
    auto &f = w.hollows;
    auto &m = f.minds[id];
    if ((f.mode & hollow::eligible) && !(id & 15))
        for (Word j = 0; j < m.used; ++j) {
            auto &site = f.sites[Count(j) * ((f.stride + 15) / 16) + id / 16];
            if (site == from + 1)
                site |= 0x80000000u;
        }
    if (walked) {
        hollow::stride(m, relative, f.mode);
        count(f.ledger->strides, 1);
    }
}
