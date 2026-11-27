namespace axion::life::practice {
struct Creek {
    Request exhibit{};
    Word state = 0, initial = 0, calibration = 0, carbon = 0, fruit = 0, spool = 1;
};
AX_FN Creek creek(Word world, Word n, Word visit, const linkage::Material (&soil)[2][4], Word width) {
    Word k = mix(world + visit * 5807);
    Creek p{};
    p.calibration = (k >> 24) & 1;
    p.exhibit = apparatus(k, 1 + ((k >> 13) % width), soil[p.calibration]);
    p.state = position(p.exhibit.rig, p.exhibit.view, soil[p.calibration]);
    p.initial = p.state;
    p.carbon = (k & 3) ? 8 + ((k >> 8) & 31) : 0;
    p.fruit = (k >> 4) & 7;
    return p;
}
struct Timing {
    Count words[2]{};
    Word used = 0;
};
AX_FN bool day_move(const Timing &v, Word t) { return (v.words[t / 64] >> (t & 63)) & 1; }
AX_FN Day forage(Genes g, Word world, Word n, Word horizon, Word treatment, Word width, Word cap, Word island,
                 Timing *record, const Timing *foreign) {
    Mind native{g};
    Context banks[4]{};
    Slots s{banks, 1, cap};
    Word weather[2];
    linkage::Material laws[2][4];
    climate(world, weather, laws);
    Word visited = 0, together = 0, energy = 48, flags = all | responsive_attention;
    Day d{};
    d.trials = 1;
    d.born = energy;
    if (treatment == 3)
        flags &= ~responsive_attention;
    if (treatment == 4)
        flags |= misassigned_attention;
    Creek coast[4]{};
    Word position = 0;
    for (Word j = 0; j < (island ? 4u : 1u); ++j) {
        coast[j] = creek(world, n, j, laws, width);
        d.available_food += coast[j].carbon + coast[j].fruit;
    }
    if (treatment == 2 && foreign && foreign->used < horizon)
        horizon = foreign->used;
    for (Word t = 0; t < horizon && energy > 3; ++t) {
        auto &patch = coast[position];
        auto &r = patch.exhibit;
        auto &rig = r.rig;
        auto *law = laws[patch.calibration];
        Word before = linkage::expose(rig, patch.state, law);
        bool open = linkage::matches(before, r.goal);
        Word k = mix(world + n * 347 + visited * 31 + t * 4327);
        Word sensory = cue(patch.carbon != 0, !size(native) || (native.trace & (1u << 24)));
        bool stay = together < span(g) && persist(native, sensory, k, flags, energy);
        if (treatment == 1)
            stay = (t % 5) != 4;
        if (treatment == 2)
            stay = foreign && !day_move(*foreign, t);
        if (treatment == 5)
            stay = together < 1;
        if (record) {
            record->used = t + 1;
            if (!stay)
                record->words[t / 64] |= Count(1) << (t & 63);
        }
        intend(native, sensory, stay);
        ++d.decisions;
        d.stable_choices += stay;
        d.rewarded_stays += stay && open && patch.carbon;
        d.stale_stays += stay && !patch.carbon;
        if (!stay) {
            ++d.departures;
            ++visited;
            together = 0;
            if (island)
                position = (position + ((g.muscles & (1u << 21)) && (k & 1) ? 3 : 1)) & 3;
            else {
                d.unconsumed += patch.carbon + patch.fruit;
                patch = creek(world, n, visited, laws, width);
                d.available_food += patch.carbon + patch.fruit;
            }
            auto &destination = coast[position];
            --energy;
            ++d.heat;
            Word leaves = destination.fruit > 3 ? 3 : destination.fruit;
            destination.fruit -= leaves;
            energy += leaves;
            d.grazed += leaves;
            d.stomach += leaves;
            reinforce(native, false, false, leaves, flags);
            continue;
        }
        ++together;
        if (together > d.longest)
            d.longest = together;
        if (island && patch.fruit && energy <= 8 + ((g.muscles >> 16) & 31)) {
            Word bite = patch.fruit < 3 ? patch.fruit : 3;
            patch.fruit -= bite;
            energy += bite - 1;
            d.heat++;
            d.grazed += bite;
            d.stomach += bite;
            reinforce(native, false, false, true, flags);
            continue;
        }
        Word work = 0, changed = 0, predicted = absent;
        bool proposed = false;
        if (!open || !patch.carbon) {
            linkage::Design plan{};
            if ((g.muscles & (1u << 30)) && size(native) && patch.carbon) {
                auto model = read(s.at(active(native)), g);
                plan = linkage::design(model, rig, before, r.goal, patch.spool ? r.sockets : 0, motor_genes(g));
            }
            Word choice = linkage::no_path, edit = linkage::no_edit;
            if (plan.found && (plan.edit < 6 || plan.turns.len)) {
                proposed = true;
                edit = plan.edit;
                if (edit == 6)
                    choice = linkage::step(plan.turns, 0);
            } else if (patch.spool && r.sockets && (k & 15) < ((g.muscles >> 12) & 7))
                edit = linkage::bump(r.sockets, k >> 9);
            else
                choice = (g.inference & (1u << 5)) ? inquiry(native, s, rig, before, k, flags)
                                                   : linkage::bump(rig.handles, k);
            if (edit < 6) {
                Word a = linkage::edge_a(edit), b = linkage::edge_b(edit), old = joint(rig.joints, a, b);
                if (linkage::insert(rig, r.sockets, edit, patch.spool, law, energy)) {
                    work = 2;
                    ++d.repairs;
                    changed = fitting(native, s, bits(rig.glyphs, a), old, joint(rig.joints, a, b), false, flags);
                }
            } else if (choice < 8) {
                if (size(native)) {
                    predicted = forecast(s.at(active(native)), g, rig, before, choice);
                }
                patch.state = linkage::press(rig, patch.state, choice, law);
                auto e =
                    observe(native, s, rig, before, choice, linkage::expose(rig, patch.state, law), false, flags);
                changed = e.edits;
                work = 1;
                ++d.turns;
            }
        }
        Word after = linkage::expose(rig, patch.state, law);
        bool aligned = linkage::matches(after, r.goal);
        d.opens += !open && aligned;
        d.solved += !open && aligned && proposed;
        Word food = aligned ? (patch.carbon < 5 ? patch.carbon : 5) : 0;
        patch.carbon -= food;
        energy += food;
        d.stomach += food;
        d.meals += !!food;
        work += 1;
        energy -= work;
        d.heat += work;
        d.learned += changed;
        d.inspected += predicted == after;
        reinforce(native, changed, predicted == after, food, flags);
    }
    for (Word j = 0; j < (island ? 4u : 1u); ++j)
        d.unconsumed += coast[j].carbon + coast[j].fruit;
    d.survived = energy > 3;
    d.reserve = energy;
    return d;
}
} // namespace axion::life::practice
KERNEL void practice_patch_meals(practice::Family *p, Word n, Word seed, Word epoch, Word cap, Word mixed,
                                 Word island) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + (i / 2) * 5257 + epoch * 9511), wallet = 0;
    for (Word j = 0; j < 2; ++j) {
        auto r = practice::forage(p[i].genes, mix(k + 173 * j), j, 64, 0, 3, cap, island, nullptr, nullptr);
        wallet += r.reserve;
    }
    if (mixed) {
        for (Word j = 0; j < 3; ++j) {
            auto q = practice::school(p[i].genes, mix(k + j * 643), 1 + j, practice::informed, cap, false, nullptr);
            wallet += q.delivered ? 40 - q.paid : 0;
        }
    }
    p[i].eggs = 1 + wallet / 8;
}
KERNEL void practice_patch_trials(const practice::Genes *g, Word ng, Word n, Word seed, Word treatment, Word cap,
                                  Word island, practice::Day *out) {
    Word i = tid();
    if (i >= n)
        return;
    // Exchange timing between two different foraging grounds within each pair.
    Word pair = i >> 1, side = i & 1, k = mix(seed + pair * 13901), world = mix(k + side * 4733);
    auto genes = g[mix(k + 2267) % ng];
    practice::Timing other{};
    if (treatment == 2)
        practice::forage(genes, mix(k + (side ^ 1) * 4733), 0, 96, 0, 4, cap, island, &other, nullptr);
    out[i] = practice::forage(genes, world, 0, 96, treatment, 4, cap, island, nullptr, &other);
}
KERNEL void practice_windows(const practice::Genes *g, Word ng, Word n, Word seed, Word cap, Word island,
                             practice::Windows *out) {
    using namespace practice;
    Word i = tid();
    if (i >= n)
        return;
    Word ticket = mix(seed + i * 6719), worlds[]{mix(ticket + 4289), mix(ticket + 9751)};
    auto genes = g[mix(ticket + 3803) % ng];
    Timing reference[2]{};
    Word window = 96;
    for (Word j = 0; j < 2; ++j) {
        auto origin = forage(genes, worlds[j], 0, 96, 0, 4, cap, island, reference + j, nullptr);
        if (origin.decisions < window)
            window = origin.decisions;
    }
    for (Word j = 0; j < 2; ++j) {
        auto swap = forage(genes, worlds[j], 0, window, 2, 4, cap, island, nullptr, reference + (j ^ 1));
        if (swap.decisions < window)
            window = swap.decisions;
    }
    Windows v{};
    for (Word j = 0; j < 2; ++j) {
        auto own = forage(genes, worlds[j], 0, window, 2, 4, cap, island, nullptr, reference + j);
        auto swap = forage(genes, worlds[j], 0, window, 2, 4, cap, island, nullptr, reference + (j ^ 1));
        v.ticks += own.decisions;
        v.own_stays += own.stable_choices;
        v.swapped_stays += swap.stable_choices;
        v.own_food += own.stomach;
        v.swapped_food += swap.stomach;
        v.own_work += own.heat;
        v.swapped_work += swap.heat;
        v.own_reserve += own.reserve;
        v.swapped_reserve += swap.reserve;
        v.own_springs += own.meals;
        v.swapped_springs += swap.meals;
    }
    v.longer = v.own_reserve > v.swapped_reserve;
    v.shorter = v.own_reserve < v.swapped_reserve;
    out[i] = v;
}
