#include "effort_bundle.cuh"
AX_FN Word fruit_skin(World w, Word pos) {
    auto b = w.hollows.beds[pos];
    return b.relief ? hollow::material(b) : (w.land[pos].spore & 3);
}
AX_FN Word ground_skin(World w, Word pos) {
    auto p = w.land[pos];
    Word faces = w.matter.bed ? w.matter.bed[pos].faces : 0;
    return (p.enzyme ^ (p.root >> 9) ^ faces) & 3;
}
AX_FN Word stamped_skin(World w, Word pos, Word axis, Word tread) {
    return (ground_skin(w, pos) + ((tread >> (4 * axis)) & 15) / 4) & 3;
}
AX_FN Word crossing_fee(World w, Configuration c, Word pos, Word edge, Word axis, Word tick, Word mass, Word wall,
                        Word weave, Word tread) {
    auto soil = w.land[pos];
    Word district = (pos % c.side) / 16 + 128 * ((pos / c.side) / 16);
    Word season = w.outlay.period ? tick / w.outlay.period : 0;
    Word weather = (mix(c.seed + district * 1789) + (season & 3)) & 3;
    Word material = ground_skin(w, pos);
    Word drag = 1 + ((material + weather) & 3) + cut(3, soil.water / 32) + cut(3, wall / 2) + mass / 12;
    Word support = cut(5, weave / 4) + ((tread >> (4 * axis)) & 15) / 3;
    (void)edge;
    return drag > support ? drag - support : 1;
}
AX_FN void wear_edge(World w, Word edge, Word axis, Word tick, Word key) {
    if (!(w.outlay.mode & effort::trails))
        return;
    auto &packed = w.outlay.beds[edge];
    Word off = 4 * axis, tone = (packed >> off) & 15;
    auto &soil = w.land[edge];
    Word fibre = axis ? soil.fibre_south : soil.fibre_east;
    Word scour = soil.water >= 48 ? 7u : soil.water >= 16 ? 15u : 63u;
    if (fibre >= 4)
        scour = 2 * scour + 1;
    if (tone && ((key ^ (tick * 173)) & scour) == 0) {
        Word loss = cut(tone, 1 + (soil.water >> 5));
        packed = Byte(packed - (loss << off));
        count(w.outlay.book->washed, loss);
    }
}
AX_FN void footfall(World w, Word edge, Word axis, Word mass) {
    if (!(w.outlay.mode & effort::trails))
        return;
    auto &packed = w.outlay.beds[edge];
    Word off = axis * 4, tone = (packed >> off) & 15;
    Word dent = cut(15 - tone, 1 + mass / 12);
    packed = Byte(packed + (dent << off));
    count(w.outlay.book->pressed, dent);
}
AX_FN void press_earth(World w, Configuration c, Word id, Word pos, Word edge, Word axis, Word action, Word tick,
                       Word key) {
    auto &f = w.outlay;
    auto &m = f.minds[id];
    auto &a = w.animals[id];
    if (!(f.mode & effort::tamping) || !(f.mode & effort::trails) || !a.body.limbs || a.energy < 4 ||
        !(m.genes.projection & (1u << (24 + (action & 7)))) || (key & 15) || w.land[edge].grains < 2)
        return;
    Word old = f.beds[edge], at = 4 * axis, compression = (old >> at) & 15;
    if (compression == 15)
        return;
    Word dent = cut(15 - compression, 1 + nbits(a.body.limbs) / 3);
    Word hue = stamped_skin(w, pos, axis, old), attempts = 1;
    if (f.mode & effort::appraisal) {
        Word previous = (m.experience >> (12 + 2 * hue)) & 3;
        Word visits = 1 + ((m.genes.episode >> 18) & 3) * previous +
                      Word(bool(m.genes.projection & (1u << 20))) * (compression >> 2);
        auto plan = effort::prepare(effort::freeze(m), hue, visits, 1 + ((m.genes.episode >> 28) & 1),
                                    f.mode & effort::foreign_surface);
        attempts = plan.strokes;
        bool known = plan.known;
        // A heritable exploratory tap can establish an unknown transition.
        if (!known && ((key >> 20) & 7) <= ((m.genes.episode >> 24) & 3))
            attempts = 1;
        m.experience = (m.experience & 0xfffffu) | ((1u + Word(known) + 2 * attempts) << 20);
    }
    for (Word j = 0; j < attempts && compression < 15 && a.energy > 2; ++j) {
        Word size = cut(15 - compression, dent);
        old = f.beds[edge];
        hue = stamped_skin(w, pos, axis, old);
        compression += size;
        f.beds[edge] = Byte(old + (size << at));
        --a.energy;
        count(w.account->heat, 1);
        count(f.book->contacts, 1);
        count(f.book->pressed, size);
        if (c.plastic && a.body.eyes)
            effort::change(m, hue, 1, stamped_skin(w, pos, axis, f.beds[edge]), f.mode);
    }
    (void)tick;
}
AX_FN void effort_meal(World w, Configuration c, Word id, Word cue, Word amount) {
    if (!c.plastic || !(w.outlay.mode & effort::learning))
        return;
    effort::taste(w.outlay.minds[id], cue, amount, w.outlay.mode);
    count(w.outlay.book->meals, 1);
}
AX_FN void finish_outing(World w, Configuration c, Word id, Word reserve) {
    auto &m = w.outlay.minds[id];
    if (!effort::remaining(m))
        return;
    bool sampled = m.trace & (1u << 11);
    Word group = (m.trace >> 9) & 3;
    int net = effort::balance(m, reserve);
    bool abandoned =
        !(m.trace & (1u << 11)) && (m.genes.projection & (1u << 23)) && net < -int(2 + ((m.genes.muscle >> 26) & 15));
    if (abandoned) {
        m.trace = (m.trace & ~(15u << 5)) | (1u << 5);
        m.experience &= ~(7u << 8);
    }
    if (!effort::advance(m, reserve, c.plastic ? w.outlay.mode : 0u))
        return;
    count(w.outlay.book->settled, 1);
    count(w.outlay.book->deaths, !reserve);
    if (c.plastic && (w.outlay.mode & effort::learning))
        count(w.outlay.book->credited, 1);
    if (sampled) {
        auto &g = w.outlay.book->experiment[group];
        count(g.n, 1);
        count(g.lost, !reserve);
        if (net > 0)
            count(g.gained, net);
        else
            count(g.depleted, Word(-net));
    }
    m.trace = m.account = 0;
    m.experience &= ~0xf00u;
}
AX_FN bool effort_access(const effort::Field &f, Word label) {
    return !(f.mode & effort::comp) || (f.mode & effort::same_comp) ||
           (bool(label & hollow::lineage_tag) != bool(f.mode & effort::reverse_comp));
}
AX_FN effort::Prospect destination(hollow::Recall r, Word surface) { return {r.cue & 3, surface, r.range}; }
AX_FN bool effort_route(World w, Configuration c, Word id, Word pos, hollow::Recall &goal, Word tick, Word k,
                        Word obstruction, Word skin) {
    auto &f = w.outlay;
    auto &price = f.minds[id];
    auto &memory = w.hollows.minds[id];
    if (effort::remaining(price) && (price.experience & (1u << 11))) {
        if (!(price.trace & (1u << 9)))
            return false;
        Word xy = price.trace >> 16;
        hollow::Recall target{hollow::limit, 0, (price.trace >> 12) & 15, 4, hollow::distance(xy, memory), xy};
        goal = hollow::emit(memory, target, k, obstruction);
        return true;
    }
    Word read_skin = (f.mode & effort::scrambled) ? (skin + 1) & 3 : skin;
    auto local = effort::Prospect{fruit_skin(w, pos), read_skin, 0};
    if (effort_access(f, memory.efferent) && (f.mode & effort::more_places) && (price.genes.projection & (1u << 22))) {
        int top = goal.slot != hollow::missing && goal.range
                      ? effort::expect(price, destination(goal, read_skin)).charge
                      : -1000000;
        auto slots = hollow::cells(w.hollows, id);
        for (Word j = 0; j < memory.used; ++j) {
            Word e = slots.at(j), cue = hollow::identity(e);
            if (!hollow::strength(e) || (cue >> 2) != hollow::harvest)
                continue;
            Word range = hollow::distance(e, memory);
            if (!range)
                continue;
            int value = effort::expect(price, {cue & 3, read_skin, range}).charge;
            if (value > top) {
                top = value;
                goal = {j, 0, cue, 4, range, e & 65535};
            }
        }
        if (goal.slot != hollow::missing && goal.range && goal.compass == 4)
            goal = hollow::emit(memory, goal, k, obstruction);
    }
    if (goal.slot == hollow::missing || !goal.range)
        return true;
    if (effort::remaining(price))
        return price.trace & (1u << 9);
    auto other = destination(goal, read_skin);
    bool predicted =
        effort::elect(effort::expect(price, local).charge, effort::expect(price, other).charge, mix(k + 461));
    bool use = effort_access(f, memory.efferent);
    bool choice = !(f.mode & effort::deciding) || !use || predicted;
    if (f.mode & effort::reversing)
        choice = !choice;
    Word draw = (f.mode & effort::fixed_survey) ? mix(c.seed + id * 1459 + tick * 4639) : k;
    bool sampled = (f.mode & effort::surveying) && !(mix(draw + 647) & 7);
    if (sampled)
        choice = mix(draw + 1427) & 1;
    bool latch = price.genes.episode & (1u << 31);
    Word span = 1 + ((price.genes.episode >> 20) & 7);
    if (latch)
        span += cut(7, other.steps) * ((price.genes.projection >> 20) & 3) * 2;
    Word standard = (f.mode >> 16) & 127;
    effort::begin(price, choice ? other : local, w.animals[id].energy, choice, predicted, sampled,
                  sampled ? (standard ? standard : 8) : span);
    if (latch) {
        price.experience |= 1u << 11;
        price.trace |= (goal.cue << 12) | (goal.goal << 16);
    }
    count(f.book->offers, 1);
    if (choice)
        count(f.book->trips, 1);
    else
        count(f.book->stays, 1);
    (void)c;
    (void)tick;
    return choice;
}
