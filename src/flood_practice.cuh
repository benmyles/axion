namespace axion::life::practice {
struct Work {
    Word owner = 4, stroke = linkage::no_path, edge = linkage::no_edit, exploratory = 0, designed = 0, adding = 1,
         gather = 0, extend = 0, renewed = 0, prediction = linkage::absent, braced = 0;
};
AX_FN Work approach(flood::World w, const linkage::Site &s, Word tile, Word k, bool due) {
    auto &f = w.practice;
    Work v{};
    if (!f.minds || !(f.mode & engaging))
        return v;
    for (Word pass = 0; pass < 2; ++pass)
        for (Word j = 0; j < 4; ++j) {
            Word candidate = (j + (k >> 8)) & 3, id = s.slots[candidate];
            auto &a = w.animals[id];
            auto &m = f.minds[id];
            if (a.energy < 4 || !a.body.eyes || !a.body.limbs ||
                !entitled(f.mode, w.hollows.minds[id].efferent & hollow::lineage_tag)) {
                if (stays(m, tile))
                    m.focus = 0;
                continue;
            }
            bool latched = stays(m, tile) && (f.mode & holding);
            if (pass == Word(latched) || (!latched && !due))
                continue;
            Word access = linkage::graspable(s, candidate, a.body);
            if (!access) {
                m.focus = 0;
                continue;
            }
            Word event = (!!s.rig.joints) + 2 * Word(w.hollows.beds[s.pos[s.from]].carbon != 0);
            if (!latched && !willing(m, w.motor[id], a.energy, event, mix(k + j * 1117)))
                continue;
            v.owner = candidate;
            v.renewed = latched;
            return v;
        }
    return v;
}
AX_FN void act(flood::World w, flood::Configuration config, linkage::Site s, Work &v, Word exposed, Word tile, Word k,
               bool open) {
    auto f = w.practice;
    Word id = s.slots[v.owner];
    auto &a = w.animals[id];
    auto &m = f.minds[id];
    auto bank = cells(f, id);
    auto &g = m.genes;
    Word source = s.pos[v.owner];
    bool loose = w.land[source].litter && (f.mode & collecting) && config.weaving &&
                 (a.load + a.fibre < 2 + 2 * __builtin_popcount(a.body.limbs));
    Word edges = config.weaving && (w.linkage.mode & linkage::coupled) && (w.linkage.mode & linkage::fitting)
                     ? linkage::reachable_edges(s, v.owner, a.body)
                     : 0;
    Word joinable = edges & ~s.bands;
    bool stocked = w.hollows.beds[s.pos[s.from]].carbon != 0;
    bool planner = linkage::public_motor(w.linkage.mode, w.hollows.minds[id].efferent & hollow::lineage_tag) &&
                   (f.mode & crafting) && (w.linkage.mode & linkage::planning) && (g.muscles & (1u << 30));
    if (!open && stocked && size(m) && planner) {
        auto model = read(bank.at(active(m)), g);
        auto fit = linkage::Mind{motor_genes(g)};
        auto design = linkage::water_order(fit, model, s, exposed, (a.fibre || loose) ? joinable : 0, linkage::focal);
        count(f.book->searches, 1);
        if (design.found && (design.edit != linkage::no_edit || design.turns.len)) {
            v.designed = 1;
            if (design.edit != linkage::no_edit) {
                if (a.fibre)
                    v.edge = design.edit;
                else
                    v.gather = 1;
            } else
                v.stroke = linkage::step(design.turns, 0);
            count(f.book->proposals, 1);
        }
    }
    if (!v.designed) {
        if (open && stocked && ((g.attention >> 28) & 1)) {
            m.focus = 0;
            count(f.book->protected_goal, 1);
            return;
        }
        v.exploratory = 1;
        Word drive = mix(k + id * 4583);
        bool join = (drive & 15) <= ((g.muscles >> 12) & 7);
        if (join && edges) {
            if (a.fibre && joinable)
                v.edge = linkage::bump(joinable, drive >> 6);
            else if (!a.fibre && loose)
                v.gather = 1;
            else if (s.bands & edges) {
                v.edge = linkage::bump(s.bands & edges, drive >> 6);
                v.adding = 0;
            }
        }
        if (w.wicks.parcels && config.weaving && v.edge == linkage::no_edit && !v.gather &&
            (g.muscles & (1u << 26)) && ((drive >> 17) & 7) == 0) {
            Word ports = ((v.owner & 1) ? 1u : 0u) | ((v.owner & 2) ? 2u : 0u);
            if (ports) {
                Word axis = linkage::bump(ports, drive >> 8);
                auto &soil = w.land[source];
                auto &strand = axis ? soil.fibre_south : soil.fibre_east;
                if (!strand && a.fibre) {
                    --a.fibre;
                    ++strand;
                    --a.energy;
                    ++a.worked;
                    v.extend = 1;
                    w.wicks.parcels[source] |= wicks::edge_bit(axis);
                    count(w.wicks.ledger->strung, 1);
                    count(w.account->fibre_laid, 1);
                    count(w.account->heat, 1);
                    count(f.book->work, 1);
                } else if (!a.fibre && loose)
                    v.gather = 1;
            }
        }
        if (!v.gather && !v.extend && v.edge == linkage::no_edit) {
            bool deliberate = (f.mode & probing) && !(f.mode & random_probes) && (g.inference & (1u << 5));
            v.stroke =
                deliberate ? inquiry(m, bank, s.rig, exposed, drive, f.mode) : linkage::bump(s.rig.handles, drive);
            count(deliberate ? f.book->chosen : f.book->random, 1);
            // A second limb can hold a neighbour steady during one paid stroke.
            if (s.rig.joints && __builtin_popcount(a.body.limbs) >= 2 && (g.muscles & (1u << 27)) &&
                ((drive >> 9) & 7) < ((g.attention >> 5) & 3) && !v.designed) {
                v.braced = 1;
                s.rig.joints = 0;
            }
        }
    }
    if (v.stroke < 8 && size(m))
        v.prediction = forecast(bank.at(active(m)), g, s.rig, exposed, v.stroke);
    Word length = v.renewed ? ((m.focus >> 6) & 63) + 1 : 1;
    if (length > 63)
        length = 63;
    Word left = v.renewed ? (m.focus & 63) - 1 : span(g);
    Word prior = m.trace & 15;
    bool different = v.stroke < 8 && prior && v.stroke + 1 != prior;
    if (different)
        count(f.book->changed_handles, 1);
    m.trace = (m.trace & ~15u) | (v.stroke < 8 ? v.stroke + 1 : 0u);
    if (v.renewed)
        count(f.book->continued, 1);
    else
        count(f.book->entered, 1);
    count(f.book->bouts_eight, length == 8);
    Word signal = cue(stocked, !size(m) || (m.trace & (1u << 24)));
    bool stick = (f.mode & holding) && left && a.energy > 3 + ((g.muscles >> 18) & 7) &&
                 ((f.mode & responsive_attention)
                      ? persist(m, signal, k, f.mode, a.energy)
                      : ((f.mode & constant_stay) || !v.renewed || ((k >> 19) & 31) >= ((g.attention >> 29) & 7)));
    intend(m, signal, stick);
    m.focus = stick ? ((tile + 1) << 12) | (length << 6) | left : 0;
    if (!stick)
        count(f.book->left, 1);
    if (v.gather) {
        --w.land[source].litter;
        ++a.fibre;
        --a.energy;
        ++a.worked;
        count(w.account->heat, 1);
        count(w.account->fibre_lifted, 1);
        count(f.book->work, 1);
        count(f.book->collections, 1);
    }
}
AX_FN void record(flood::World w, linkage::Site s, Word owner, linkage::Rig rig, Word before, Word motor, Word after,
                  Word mode) {
    auto f = w.practice;
    for (Word j = 0; j < 4; ++j) {
        Word actor = s.slots[j];
        auto &body = w.animals[actor];
        if (!body.energy || !body.body.eyes)
            continue;
        bool witness = j != owner;
        if (witness && !(mode & social))
            continue;
        auto e = observe(f.minds[actor], cells(f, actor), rig, before, motor, after, witness, mode);
        count(f.book->hypotheses, e.novel);
        count(f.book->switches, e.switched);
        count(f.book->writes, e.edits);
        if (!witness)
            reinforce(f.minds[actor], e.edits, (!e.edits && e.consistent), false, mode);
        count(f.book->observed, witness && (f.minds[actor].genes.learning & 240));
    }
}
} // namespace axion::life::practice
KERNEL void practice_found(practice::Field f, Word seed) {
    Word i = tid();
    if (i >= f.sites)
        return;
    f.minds[i] = {practice::founder(mix(seed + i * 5281))};
    for (Word j = 0; j < f.cap; ++j)
        f.banks[Count(j) * f.sites + i] = {};
}
