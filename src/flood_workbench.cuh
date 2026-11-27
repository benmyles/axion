KERNEL void flood_workbench(material::Worker *resident, material::Worker *peer, material::SessionState *bench,
                            material::Reading *result, const grounding::Mind *sight, const Count *point, Word command,
                            Word value, Word amount, Word reserve, Word peer_reserve) {
    if (tid())
        return;
    auto &s = *bench;
    auto &p = *resident;
    auto &w = *peer;
    material::Reading r{};
    if (command == 0) {
        Word span = amount;
        s = {};
        s.key = value;

        s.work = material::rack(mix(value + 11), span);
        material::roughen(s.work, 1 + (s.work.bevel ^ (value & 1)));
        s.model = s.work;
        s.raw = s.model_raw = 96;
    }
    if (command == 1 || command == 2) {
        // Free handling is done on temporary material supplied by the tray.
        // The animal keeps its actual carried grain after the lesson.
        auto &pupil = command == 1 ? p : w;
        auto held = pupil.hand;
        material::play(pupil, pupil.genes.muscle, mix(s.key + value), amount);
        pupil.hand = held;
    }
    if (command == 3) {
        Word beak = value & 7;
        auto before = s.work;
        auto act = material::adjust(s.work, p, beak, p.genes.muscle, reserve, true);
        r.operation = material::effector(p.genes, beak);
        r.changed = act.change;
        r.spent = act.spent;
        if (act.performed)
            material::witness(w, material::cue(before, w), act.change);
    }
    if (command == 4) {

        Word motor =
            value < 8 ? value : material::choose(w, material::cue(s.model, w), s.key & 7, mix(s.key + s.cursor++));
        auto act = material::adjust(s.model, w, motor, w.genes.muscle, peer_reserve, true);
        r.operation = material::effector(w.genes, motor);
        r.changed = act.change;
        r.spent = act.spent;
    }
    if (command == 5) {
        Word motor = material::choose(p, material::cue(s.work, p), s.key & 7, mix(s.key + s.cursor++));
        auto act = material::adjust(s.work, p, motor, p.genes.muscle, reserve, true);
        r.operation = material::effector(p.genes, motor);
        r.changed = act.change;
        r.spent = act.spent;
    }
    if (command == 6) {
        auto flux = material::run_rack(s.work, amount, s.raw);
        s.raw -= flux.product;
        s.food += static_cast<material::Half>(2 * flux.product);
        if (flux.product && (p.recent & (1u << 16)) && (p.genes.observation & (1u << 17)))
            material::witness(p, p.recent & 255, (p.recent >> 8) & 255);
        r.product = flux.product;
        r.sun = flux.sun;
    }
    if (command == 10) {
        auto demo = material::run_rack(s.model, amount, s.model_raw);
        s.model_raw -= demo.product;
        s.model_food += static_cast<material::Half>(2 * demo.product);
        r.product = demo.product;
        r.sun = demo.sun;
        if (demo.product && (w.recent & (1u << 16)))
            material::witness(p, w.recent & 255, (w.recent >> 8) & 255);
        if (demo.product && (w.genes.observation & (1u << 17)) && (w.recent & (1u << 16)))
            material::witness(w, w.recent & 255, (w.recent >> 8) & 255);
    }
    if (command == 11) {
        for (Word i = 0; i < 32; ++i)
            s.drawer[i] = material::deposit(i, value, 100);
        s.inventory = 32;
        s.next = 0;
    }
    if (command == 12 && s.inventory && !p.hand.faces) {
        p.hand = s.drawer[s.next];
        s.drawer[s.next] = {};
        s.next = (s.next + 1) & 31;
        --s.inventory;
    }
    if (command == 13 && p.hand.faces && s.inventory < 32) {
        Word slot = (s.next + s.inventory) & 31;
        s.drawer[slot] = p.hand;
        p.hand = {};
        ++s.inventory;
    }
    if (command == 7)
        s.work.socket = static_cast<material::Byte>(value);
    if (command == 8) {
        // Project the *current physical rack* into the word circuit. Reading
        // an earlier tabletop or an externally supplied label would give a
        // different question. Only a uniquely grounded reachable part moves.
        Count possible = 0, certain = 0;
        const auto heard = *point;
        Word form = grounding::recognize(*sight, heard);
        r.form = form;
        if (form)
            for (Word j = 0; j <= 1u + s.work.reach; ++j) {
                auto part = s.work.cell[j];
                if (!part.faces)
                    continue;
                Word light = material::appearance(part);
                if (grounding::admits(sight->word[form].certain, light))
                    certain |= 1ull << j;
                if (grounding::admits(sight->word[form].possible, light))
                    possible |= 1ull << j;
            }
        r.candidates = Word(possible);
        r.certain = Word(certain);
        if (!certain || possible != certain || (certain & (certain - 1)))
            r.ambiguous = 1;
        else {
            Word loc = __builtin_ctzll(certain);
            r.site = loc;
            if (loc < 1 || loc > s.work.reach)
                r.ambiguous = 1;
            else {
                s.work.socket = static_cast<material::Byte>(loc);
                Word motor = material::choose(p, material::cue(s.work, p), s.key & 7, mix(s.key + s.cursor++));
                auto act = material::adjust(s.work, p, motor, p.genes.muscle, reserve, true);
                r.operation = material::effector(p.genes, motor);
                r.changed = act.change;
                r.spent = act.spent;
            }
        }
    }
    r.reached = material::tracks(s.work);
    r.cargo = p.hand.faces;
    r.topology = 0;
    for (Word j = 0; j <= s.work.reach; ++j)
        r.topology |= Word(material::bonded(s.work.cell[j], s.work.cell[j + 1], 1)) << j;
    r.stock = s.inventory;
    for (Word i = 0; i <= s.work.reach + 1u; ++i) {
        auto part = s.work.cell[i];
        r.ports[i] = material::rotate_faces(part.faces, part.marks & 1);
        r.stored[i] = part.charge;
        r.spectra[i] = material::appearance(part);
    }
    r.contact = s.work.socket;
    *result = r;
}
