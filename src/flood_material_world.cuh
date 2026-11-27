AX_FN Word mineral_sky(Word site, Word tick, Configuration c, Word climate = 0) {
    if (veined_site(site, c, climate)) {
        if (shore_band(site, tick, c) >= 2)
            return 4;
        return (tick / 4 + ((site % c.side) >> 3) + ((site / c.side) >> 3)) & 3;
    }
    Word x = site % c.side, y = site / c.side;
    Word sky = mix(c.seed ^ mix(x / 5 + 129 * (y / 5)) ^ mix(tick / c.season));
    return ((tick + 3 * x + y) & 3) == 0 && ((sky >> 9) & 3) != 0 ? (sky & 3) : 4;
}
AX_FN Word wet_surface(const Silt &soil, Word pos, Word tick, Configuration c, Word climate) {
    if (veined_site(pos, c, climate) && (!soil.water || shore_band(pos, tick, c) < 4))
        return 4;
    return soil.spore & 3;
}
#include "flood_worksite.cuh"
AX_FN void material_physics(World w, Configuration c, Pair p, Word tick) {
    auto &f = w.matter;
    auto &a = f.bed[p.a], &b = f.bed[p.b];
    auto &l = *f.ledger;
    auto old_a = a, old_b = b;
    Word direction = p.axis ? 2 : 1;
    Word passed = material::conduct(a, b, direction, material::conducts(f.conditions));
    count(l.transfers, passed);
    Word observation = 0;
    if (w.tide.cells && c.plastic && a.faces && b.faces && old_a.charge != old_b.charge) {
        bool forward = old_a.charge > old_b.charge;
        const auto &donor = forward ? old_a : old_b, &receiver = forward ? old_b : old_a;
        Word d = forward ? direction : (direction ^ 2);
        if (receiver.charge < material::capacity(receiver) && donor.charge >= receiver.charge + 2)
            observation = 32 | (passed << 4) | (material::face(donor, d) << 2) | material::face(receiver, d ^ 2);
    }
    if (material::conducts(f.conditions) && ((tick + 3 * p.a) & 31) == 0 && material::bonded(a, b, p.axis ? 2 : 1) &&
        a.charge + 1 >= material::capacity(a) && b.charge && material::exchange(a, b, p.axis ? 2 : 1)) {
        --b.charge;
        count(w.account->heat, 1);
        count(l.thermal, 1);
    }
    for (Word at = 0; at < 2; ++at) {
        Word pos = at ? p.b : p.a;
        auto &s = w.land[pos];
        Word surface = wet_surface(s, pos, tick, c, w.tide.active);
        bool exposed = f.bed[pos].charge >= 4 && s.litter && surface < 4;
        Word solvent = w.brine.solutes ? brine::phase(w.brine.solutes[pos], s.water) : 0;
        auto r = material::excite(f.bed[pos], mineral_sky(pos, tick, c, w.tide.active),
                                  brine::surface(surface, solvent), s.litter, s.fruit);
        if (w.worksites.ground) {
            if (observation & 32)
                worksite::inscribe(
                    w, pos, tick,
                    worksite::evidence(0, (observation >> 2) & 3, observation & 3, (observation >> 4) & 1));
            if (exposed)
                worksite::inscribe(w, pos, tick,
                                   worksite::evidence(1, material::catalytic_band(f.bed[pos]), surface, r.reaction));
        }
        if (w.tide.sight)
            w.tide.sight[pos] =
                static_cast<Half>(observation | (exposed ? (512u | ((s.spore & 3) << 6) | (r.reaction << 8)) : 0u));
        count(l.light, r.light);
        count(w.account->light, r.light);
        count(l.reactions, r.reaction);
        count(l.folds, r.folds);
    }
}
AX_FN void see_material(World w, Configuration c, Word pos, Word slot, Word tick) {
    auto &body = w.animals[slot];
    auto &g = w.matter.residents[slot].genes;
    auto &f = w.tide.cells[slot];
    Word glass = material::scree::wild_glass(g), odor = material::scree::wild_odor(g);
    Word glimpses = (w.tide.active & 2) && body.body.eyes ? 1 + cut(4u, nbits(body.body.eyes)) : 1;
    for (Word j = 0; j < glimpses; ++j) {
        Word at = j ? neighbor(pos, (body.heading + tick + j - 1) & 3, c.side) : pos;
        Word event = w.tide.sight[at];
        if (body.body.eyes && (event & 32))
            material::scree::plume(f, g, glass ^ ((event >> 2) & 3), glass ^ (event & 3), (event >> 4) & 1);
        if ((j ? bool(body.body.eyes) : bool(body.body.pores)) && (event & 512))
            f.appetite = material::scree::observe(f.appetite, ((event >> 6) & 3) ^ odor, (event >> 8) & 1, g.practice);
    }
}
AX_FN Word attachments(const material::Grain *bed, Word pos, Word side) {
    auto p = bed[pos];
    Word count = 0;
    if (p.faces)
        for (Word d = 0; d < 4; ++d)
            count += material::bonded(p, bed[neighbor(pos, d, side)], d);
    return count;
}
AX_FN void work_material(World w, Configuration c, Word pos, Word across, Word dir, Word slot, Word motor, Word tick,
                         Word key, Word gained) {
    auto &field = w.matter;
    auto &tool = field.residents[slot];
    auto &ledger = *field.ledger;
    auto previous = tool.recent;
    tool.recent = 0;
    if (c.plastic && gained && (previous & (1u << 16)) && (tool.genes.observation & (1u << 17))) {
        auto old = tool.edits;
        material::witness(tool, previous & 255, (previous >> 8) & 255);
        count(ledger.edits, tool.edits - old);
    }
    if (!field.conditions.agency || !(tool.genes.receptors & (1u << (24 + (motor & 7)))))
        return;
    auto &home = field.bed[pos], &away = field.bed[across];
    Word sight = w.land[across].spore & 3;
    Word cue = material::sense(home, away, tool.hand, dir, sight), move = material::choose(tool, cue, motor, key);
    if (w.tide.cells && c.plastic && ((key >> 25) & 63) < ((tool.genes.release >> 20) & 31))
        move = mix(key + 3257) & 7;
    if (w.tide.cells && c.plastic && ((key >> 5) & 7) < ((tool.genes.innate >> 27) & 7)) {
        Word unseen = (~(w.tide.cells[slot].appetite >> 8)) & 255;
        if (unseen) {
            Word start = (key >> 12) & 7, order = (unseen >> start) | (unseen << (8 - start));
            move = (start + __builtin_ctz(order)) & 7;
        }
    }
    Word operation = material::effector(tool.genes, move);
    auto before_grain = home;
    Word held = material::quantity(tool.hand);
    auto r = material::manipulate(home, away, tool.hand, operation, dir, field.attachment[pos],
                                  field.attachment[across], w.animals[slot].energy, sight);
    if (c.plastic) {
        Word before = tool.edits;
        material::practice(tool, move, r);
        if (w.tide.cells)
            material::scree::motor_event(w.tide.cells[slot], tool.genes, move, held, material::quantity(tool.hand),
                                         before_grain, home);
        count(ledger.edits, tool.edits - before);
    } else
        tool.recent = r.cue | (r.change << 8) | (Word(bool(r.performed)) << 16);
    w.animals[slot].energy -= static_cast<Half>(r.spent);
    count(w.account->heat, r.spent);
    count(ledger.heat, r.spent);
    if (r.performed) {
        clay_carry(w, pos, across, slot, operation);
        if (operation == 1 || operation == 2)
            count(ledger.turns, 1);
        if (operation == 3)
            count(ledger.lifts, 1);
        if (operation == 4)
            count(ledger.placements, 1);
        if (operation == 5 || operation == 6)
            count(ledger.translations, 1);
        if (operation == 7)
            count(ledger.exchanges, 1);
    }
}
AX_FN void see_work(World w, Word observer, Word observed, Word key) {
    auto &f = w.matter;
    auto &p = f.residents[observer];
    Word seen = f.residents[observed].recent;
    if (!f.conditions.vision || !(seen & (1u << 16)) || !w.animals[observer].body.eyes)
        return;
    Word view = seen & 255, flash = (seen >> 8) & 255;
    if (f.conditions.vision == 2)
        flash = mix(key ^ seen) & 255;
    Word old = p.edits, visits = p.sightings;
    material::witness(p, view, flash);
    count(f.ledger->observed, p.sightings - visits);
    count(f.ledger->edits, p.edits - old);
}
