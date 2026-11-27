AX_FN void read_pressure_mark(clay::Field &f, clay::Mind &m, Word glaze, Word face, Word event, bool old) {
    if (!(f.use & press::attending) || !(f.use & press::moving) || glaze >= 256 || !(event & 128))
        return;
    material::Grain grain{};
    grain.substance = glaze;
    auto &book = f.ledger->press;
    count(book.studied, 1);
    count(book.predating, old);
    count(book.visual_edits, press::see(m, press::appearance(grain, face), event & 3, true));
}
AX_FN void clay_carry(World w, Word home, Word away, Word id, Word op) {
    if (!w.clay.ground || op < 3 || op > 6)
        return;
    auto &here = w.clay.ground[home], &there = w.clay.ground[away], &hand = w.clay.palms[id];
    auto &from = op == 3 || op == 5 ? here : op == 4 ? hand : there;
    auto &to = op == 3 ? hand : op == 4 || op == 6 ? here : there;
    count(w.clay.ledger->movements, clay::size(from) != 0);
    to = from;
    from = {};
}
AX_FN Word clay_color(World w, Word pos, Word slot) {
    auto held = w.matter.residents[slot].hand;
    return held.faces ? held.substance : w.matter.bed[pos].faces ? w.matter.bed[pos].substance : 256u;
}
AX_FN void clay_orient(World w, Word other, Word self, Word fellow, Word key, bool plastic) {
    auto &m = w.clay.minds[self];
    Word cue = w.animals[self].body.eyes && (w.clay.use & clay::color) ? clay_color(w, other, fellow) : 256;
    if (!(w.clay.use & (clay::color | clay::latent))) {
        clay::swap_bank(m, w.siphons.residents[self], 0);
        return;
    }
    Word before = clay::count(m);
    count(w.clay.ledger->remaps,
          clay::orient(m, w.siphons.residents[self], cue, key, plastic, w.clay.use & clay::latent));
    count(w.clay.ledger->opened, clay::count(m) - before);
}
AX_FN void clay_write(World w, Word pos, Word slot, Word tone, Word relief, bool addressed, bool visible, bool fed,
                      Word tick, Word key) {
    auto &matter = w.clay;
    auto &animal = w.animals[slot];
    auto &m = matter.minds[slot];
    Word pose = w.motor[slot] & 7;
    if (!(matter.use & clay::write) || !visible || tone >= 4 || !animal.body.limbs || animal.energy <= 2 ||
        !(m.genes.fingers & (1u << pose)) || !w.land[pos].water)
        return;
    bool held = w.matter.residents[slot].hand.faces;
    auto grain = held ? w.matter.residents[slot].hand : w.matter.bed[pos];
    if (!grain.faces)
        return;
    auto &tile = held ? matter.palms[slot] : matter.ground[pos];
    auto event = siphon::radiate(0, tone, relief, addressed, true, fed);
    Word face = clay::friction(m.genes, event, pose, key);
    if (clay::impress(tile, face, event, tick))
        count(matter.ledger->impressions, 1);
    --animal.energy;
    count(w.account->heat, 1);
    count(matter.ledger->work, 1);
}
AX_FN void clay_lend(World w, Word first, Word second, Word key) {
    auto &one = w.matter.residents[first].hand, &two = w.matter.residents[second].hand;
    if (bool(one.faces) == bool(two.faces) || (key & 3))
        return;
    Word donor = one.faces ? first : second, receiver = one.faces ? second : first;
    auto &a = w.animals[donor], &b = w.animals[receiver];
    auto &g = w.clay.minds[donor].genes, &h = w.clay.minds[receiver].genes;
    if (a.energy <= 2 || b.energy <= 2 || !a.body.limbs || !b.body.limbs ||
        !(g.branches & (1u << (8 + (w.motor[donor] & 7)))) || !(h.fingers & (1u << (8 + (w.motor[receiver] & 7)))))
        return;
    auto &held = w.matter.residents[donor].hand;
    w.matter.residents[receiver].hand = held;
    held = {};
    auto &coat = w.clay.palms[donor];
    w.clay.palms[receiver] = coat;
    count(w.clay.ledger->movements, clay::size(coat) != 0);
    coat = {};
    --a.energy;
    --b.energy;
    count(w.account->heat, 2);
    count(w.clay.ledger->work, 2);
    count(w.clay.ledger->loans, 1);
}
AX_FN void clay_rake(World w, Configuration c, Word pos, Word across, Word id, Word other, Word tick, Word key) {
    auto &sys = w.clay;
    auto &m = sys.minds[id];
    auto &a = w.animals[id];
    auto &native = w.siphons.residents[id];
    if (a.energy <= 3 || !a.body.limbs)
        return;
    Word motor = w.motor[id], gates = m.genes.fingers, grip = (motor >> 3) & 3;
    bool fingers = sys.use & clay::exercise && (gates & (1u << (16 + (motor & 7))));
    bool listen = sys.use & clay::read && (gates & (1u << (8 + (motor & 7))));
    if (!fingers && !listen)
        return;
    Word reach = 1 + cut(7, __builtin_popcount(a.body.limbs)), maximum = 1 + 2 * ((m.genes.branches >> 28) & 3);
    Word span = cut(reach, maximum);
    if (span > a.energy - 2)
        span = a.energy - 2;
    // A sweep can cross multiple ridges on the same physical carrier. The
    // eye can see their relief before paying, but cannot see the fired pitch.
    const clay::Tile *piece = nullptr;
    Word glaze = 256, faces = 0, best = 0;
    if (listen && a.body.eyes)
        for (Word j = 0; j < 4; ++j) {
            if (j == 3 && ((sys.material >> 2) < 2 || !w.animals[other].energy ||
                           !(sys.minds[other].genes.branches & (1u << (8 + (w.motor[other] & 7))))))
                continue;
            if (j == 2 && reach < 3)
                continue;
            auto grain = j == 0   ? w.matter.residents[id].hand
                         : j == 3 ? w.matter.residents[other].hand
                                  : w.matter.bed[j == 1 ? pos : across];
            if (!grain.faces)
                continue;
            auto &surface = j == 0 ? sys.palms[id] : j == 3 ? sys.palms[other] : sys.ground[j == 1 ? pos : across];
            if ((sys.use & clay::old_only) && !clay::predates(surface, a.born))
                continue;
            Word available = clay::visible_faces(surface);
            if (!available)
                continue;
            Word rank = __builtin_popcount(available) * (1 + ((m.genes.membrane >> 27) & 7)) + (mix(key + j * 17) & 15);
            if (rank > best) {
                best = rank;
                piece = &surface;
                glaze = grain.substance;
                faces = available;
            }
        }
    if (!piece && !fingers)
        return;
    Word touches = piece ? cut(span, __builtin_popcount(faces)) : span;
    Word cost = 1 + (touches - 1) / 3; // Start the movement and load any additional fingers.
    a.energy -= cost;
    count(w.account->heat, cost);
    count(sys.ledger->work, cost);
    count(sys.ledger->strokes, 1);
    Word start = clay::finger(m.genes, motor, key, (key >> 8) & 3);
    for (Word motion = 0; motion < touches; ++motion) {
        if (fingers && c.plastic) {
            Word impulse = (grip + motion * (1 + 2 * ((key >> 24) & 1))) & 3;
            count(sys.ledger->motor_edits, clay::rehearse(native, impulse, false));
        }
        if (!piece)
            continue;
        Word rotated = ((faces | (faces << 8)) >> start) & 255;
        Word offset = __builtin_ctz(rotated), at = (start + offset) & 7;
        faces &= ~(1u << at);
        start = (at + 1) & 7;
        Word e = clay::sample(*piece, at, glaze, sys.use);
        count(sys.ledger->taps, 1);
        count(sys.ledger->recovered, 1);
        bool old = clay::predates(*piece, a.born);
        count(sys.ledger->old,old);
        if (c.plastic) read_pressure_mark(sys,m,glaze,at,e,old);
        Word previous = clay::active(m), known = clay::count(m);
        count(sys.ledger->sensory_edits, clay::trace(m, native, e, glaze, key, sys.use, c.plastic));
        count(sys.ledger->remaps, clay::active(m) != previous);
        count(sys.ledger->opened, clay::count(m) - known);
    }
}
AX_FN void clay_touch(World w, Configuration c, Word pos, Word id, Word tick, Word key) {
    auto &cs = w.clay;
    auto &m = cs.minds[id];
    auto &a = w.animals[id];
    Word motor = w.motor[id];
    if (a.energy <= 3 || !a.body.limbs)
        return;
    bool held = w.matter.residents[id].hand.faces;
    auto &grain = held ? w.matter.residents[id].hand : w.matter.bed[pos];
    auto &tile = held ? cs.palms[id] : cs.ground[pos];
    bool tap = (cs.use & clay::read) && grain.faces && (m.genes.fingers & (1u << (8 + (motor & 7))));
    bool feel = (cs.use & clay::exercise) && ((m.genes.fingers >> (16 + (motor & 7))) & 1);
    if (!tap && !feel)
        return;
    Word impulse = (m.genes.fingers & (1u << 31)) ? (key >> 19) & 3 : (motor >> 3) & 3;
    auto &body = w.siphons.residents[id];
    if (feel && c.plastic)
        count(cs.ledger->motor_edits, clay::rehearse(body, impulse, false));
    --a.energy;
    count(w.account->heat, 1);
    count(cs.ledger->work, 1);
    if (!tap)
        return;
    Word face = clay::finger(m.genes, motor, key, grain.marks >> 2);
    count(cs.ledger->taps, 1);
    bool old = clay::predates(tile, a.born);
    if ((cs.use & clay::old_only) && !old)
        return;
    Word e = clay::sample(tile, face, grain.substance, cs.use);
    if (!(e & 128))
        return;
    // Tapping transduces the physical resonance. Its relief requires contact
    // and a visible face to participate in the sound-to-perception projection.
    Word before = clay::active(m), known = clay::count(m);
    if (c.plastic && a.body.eyes) read_pressure_mark(cs,m,grain.substance,face,e,old);
    Word edits = a.body.eyes ? clay::trace(m, body, e, grain.substance, key, cs.use, c.plastic)
                             : siphon::overhear(body, e, false, false);
    count(cs.ledger->recovered, 1);
    count(cs.ledger->old, old);
    count(cs.ledger->sensory_edits, edits);
    count(cs.ledger->remaps, clay::active(m) != before);
    count(cs.ledger->opened, clay::count(m) - known);
}
