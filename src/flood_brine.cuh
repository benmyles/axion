namespace axion::life::brine {
AX_FN Word raw_for(Word chip, Word solution) {
    for (Word s = 0; s < 4; ++s)
        if (surface(s, solution) == chip)
            return s;
    return 4;
}
AX_FN Word alternate(Word avoid, Word key) { return (avoid + 1 + (key % 3)) & 3; }
AX_FN Word perception(Word x, Word map, Word key) {
    if (!map)
        return x;
    // An arbitrary one-to-one rearrangement of the 16 sensory contacts.
    Word left = x >> 2, right = x & 3;
    for (Word i = 0; i < 3; ++i) {
        Word next = left ^ ((key >> (8 * i + 2 * right)) & 3);
        left = right;
        right = next;
    }
    return 4 * left + right;
}
AX_FN Word order(Word i, Word seed) {
    constexpr Word step[]{1, 5, 7, 11};
    return ((seed >> 5) + i * step[seed & 3]) % 12;
}
AX_FN Word lesson(Word i, Word held, Word schedule) {
    Word index = order(i, schedule), chip = (1 + held + index / 4) & 3;
    return cue(chip, index & 3);
}
AX_FN Word choose(const Genes &g, Trace t, Word a, Word b, Word coin) {
    auto w = efficacy(g, t);
    Word x = estimate(g, w, a), y = estimate(g, w, b);
    return x == y ? (coin & 1) : Word(x < y);
}
AX_FN Word probe(const Genes &g, Trace t, Word a, Word b, Word coin) {
    auto w = efficacy(g, t);
    Word total = mass(w);
    auto interest = [&](Word q) {
        Word x = estimate(g, w, q), against = total - x, unsettled = x < against ? x : against;
        return x * ((g.readout >> 12) & 3) + unsettled * ((g.readout >> 14) & 7);
    };
    Word x = interest(a), y = interest(b);
    return x == y ? (coin & 1) : Word(x < y);
}
// Replaying an observed pulse and its absence in detached, eight-byte tissue
// tests whether a prospective contact could alter a pending material choice.
AX_FN int margin(const Genes &g, Trace t, Word a, Word b) {
    Trace w = efficacy(g, t);
    return int(estimate(g, w, a)) - int(estimate(g, w, b));
}
AX_FN bool settled(const Genes &g, Trace t, Word a, Word b, Word observations = ~0u) {
    if (observations < arrival(g))
        return false;
    auto w = efficacy(g, t);
    Word total = mass(w);
    int d = int(estimate(g, w, a)) - int(estimate(g, w, b));
    return total && 8u * Word(__builtin_abs(d)) > total * ((g.readout >> 23) & 7);
}
AX_FN Word request(const Genes &g, Trace t, Word a, Word b, Word target_a, Word target_b, Word coin, bool rehearse) {
    Word gain = (g.readout >> 21) & 3, turn = (g.readout >> 30) & 3;
    if (!rehearse || !(gain | turn))
        return probe(g, t, a, b, coin);
    auto w = efficacy(g, t);
    Word total = mass(w);
    auto worth = [&](Word contact) {
        Word x = estimate(g, w, contact), y = total - x, spread = x < y ? x : y;
        Trace yes = t, no = t;
        observe(g, yes, contact, true);
        observe(g, no, contact, false);
        int up = margin(g, yes, target_a, target_b), down = margin(g, no, target_a, target_b);
        Word switchable = (up > 0) != (down > 0) || (up < 0) != (down < 0);
        return gain * Word(__builtin_abs(up - down)) + turn * total * switchable + x * ((g.readout >> 12) & 3) +
               spread * ((g.readout >> 14) & 7);
    };
    Word left = worth(a), right = worth(b);
    return left == right ? (coin & 1) : Word(right > left);
}
AX_FN Word solution(Word seed, Word species) { return phase(dissolve(mix(seed + 517), mix(seed + 299), species), 32); }
AX_FN void lifecycle(const Genes &g, Word key, Word tides, Word species, Word offers, Word shore, Harvest &h) {
    Trace traces{};
    Word energy = 24;
    h.initial = energy;
    for (Word day = 0; day < tides; ++day) {
        Word k = mix(key + day * 7933), law = solution(k, species), held = mix(k + 923) & 3, schedule = mix(k + 1455);
        Word patience = (g.readout >> 17) & 15;
        Word watches = (shore == 1 || shore == 4) ? offers : (patience < offers ? patience : offers);
        Word coming = mix(k + 3511), active = raw_for(held, law), inert = alternate(active, mix(coming + 77));
        Word goal_a = cue(held, (coming & 1) ? inert : active), goal_b = cue(held, (coming & 1) ? active : inert);
        // Warm, input-poor scrapings can be sampled before the next rich layer
        // is uncovered. The animal chooses which of two fresh surfaces to test.
        for (Word i = 0; i < watches && energy >= 3; ++i) {
            Word m = mix(k + i * 193 + 5051);
            Word a = lesson(i, held, schedule), b = lesson(i + 5, held, schedule);
            if ((shore == 3 || shore == 5) && settled(g, traces, goal_a, goal_b, i))
                break;
            Word q = (shore == 1 || shore == 4)                                              ? a
                     : request(g, traces, a, b, goal_a, goal_b, m, shore == 3 || shore == 5) ? b
                                                                                             : a;
            if ((shore == 3 || shore == 5) && (i & 3) == 3) {
                --energy;
                ++h.heat;
                if (energy < 3)
                    break;
            }
            Word stock = 1;
            h.supplied += stock;
            Word lamp = 3, food = 0;
            if (shore) {
                h.supplied += lamp;
                food = press(grain(q >> 2, m), q & 3, law, stock, lamp);
                h.residual += lamp;
            } else
                food = press(grain(q >> 2, m), q & 3, law, stock, energy);
            h.residual += stock;
            ++h.heat;
            ++h.contacts;
            ++h.requests;
            observe(g, traces, q, food != 0);
        }
        // A solitary stone needs a long reach. The other feeding opportunity
        // is a small, guaranteed detrital meal. Decide before seeing either yield.
        Word single = cue(held, ((coming >> 17) & 1) ? active : inert);
        bool commit = false;
        if (shore >= 4) {
            auto weights = efficacy(g, traces);
            commit = impulse(g, estimate(g, weights, single), mass(weights));
        }
        // Two material-bearing faces of the new vein remain available during
        // each visit. One contains product-forming substrate for this coating.
        for (Word i = 0; i < (shore ? 1u : 4u) && energy >= 3; ++i) {
            Word m = mix(k + i * 1973 + 3511), yes = raw_for(held, law), no = alternate(yes, mix(m + 77));
            Word s0 = (m & 1) ? no : yes, s1 = (m & 1) ? yes : no;
            Word decision = choose(g, traces, cue(held, s0), cue(held, s1), m >> 8), side = decision ? s1 : s0;
            Word stock = 12;
            h.supplied += stock;
            Word product = press(grain(held, m), side, law, stock, energy);
            h.residual += stock;
            ++h.heat;
            ++h.contacts;
            h.meals += product != 0;
            observe(g, traces, cue(held, side), product != 0);
        }
        if (shore >= 4 && energy >= 7) {
            Word carbon = 12, litter = 2;
            h.supplied += carbon + litter;
            if (commit) {
                energy -= 3;
                h.heat += 3;
                Word food = press(grain(held, mix(k + 393)), single & 3, law, carbon, energy);
                ++h.heat;
                ++h.contacts;
                h.meals += food != 0;
                observe(g, traces, single, food != 0);
            } else {
                energy += litter;
                litter = 0;
            }
            h.residual += carbon + litter;
        }
        Word upkeep = (shore ? 3u : 8u) + __builtin_popcount(g.active) / 16 + ((k >> 5) & 1);
        Word paid = energy < upkeep ? energy : upkeep;
        energy -= paid;
        h.heat += paid;
        if (energy < 3)
            break;
    }
    h.body = energy;
    h.eggs = energy / 6;
}
// This detached auditor receives the transcript after the animal acts. Its
// mask includes every physical bath in this family, not just the test twins.
struct PhysicalRecords {
    Word positive[16]{};
    AX_FN constexpr PhysicalRecords() {
        for (Word q = 0; q < 16; ++q)
            for (Word f = 0; f < 32; ++f)
                if (surface(q & 3, f) == (q >> 2))
                    positive[q] |= 1u << f;
    }
};
AX_FN bool forced(Word sampled, Word reactions, Word a, Word b, Word species) {
    constexpr PhysicalRecords laws{};
    Word possible = species == 5 ? ~0u : ((1u << (1u << species)) - 1u);
    while (sampled) {
        Word q = __builtin_ctz(sampled);
        sampled &= sampled - 1;
        possible &= (reactions & (1u << q)) ? laws.positive[q] : ~laws.positive[q];
    }
    Word yes = possible & laws.positive[a], other = possible & laws.positive[b];
    return !!yes != !!other;
}
AX_FN void audit(const Genes &genes, Trace initial, Word seed, Word trials, Word species, Word offered, Word mode,
                 Reading &r) {
    for (Word t = 0; t < trials; ++t) {
        Word k = mix(seed + t * 6217), held = mix(k + 39) & 3, schedule = mix(k + 257),
             law[2]{solution(mix(k + 613), species), 0};
        Word chemistry = solution(mix(k + 1103), species);
        law[1] = chemistry;
        if (raw_for(held, law[0]) == raw_for(held, law[1]))
            law[1] ^= 1;
        Word sites[]{raw_for(held, law[0]), raw_for(held, law[1])};
        Word swap = mix(k + 627) & 1, s0 = sites[swap], s1 = sites[swap ^ 1];
        Word projection = mix(k + 2153);
        Trace memory[2]{initial, initial};
        Word first_choice = 2;
        for (Word bath = 0; bath < 2; ++bath) {
            if (mode == 5 && bath)
                memory[1] = memory[0];
            auto &cells = memory[bath];
            Word budget = 128, available = 0, seen = 0;
            for (Word i = 0; i < offered; ++i) {
                Word raw = lesson(i, held, schedule), m = mix(k + i * 317);
                if (mode >= 6) {
                    Word a = cue(held, s0), b = cue(held, s1);
                    if (mode >= 8 && mode <= 10 && settled(genes, cells, a, b, i))
                        break;
                    if (i >= ((genes.readout >> 17) & 15))
                        break;
                    Word other = lesson(i + 5, held, schedule);
                    if (mode == 10) {
                        a ^= 4;
                        b ^= 4;
                    }
                    Word picked =
                        mode == 7 ? ((m >> 12) & 1)
                                  : request(genes, cells, raw, other, a, b, m, mode == 8 || mode == 10 || mode == 11);
                    if (picked)
                        raw = other;
                }
                Word stock = 1;
                Word where = mode == 2 ? 0 : mode == 3 ? (bath ^ 1) : bath;
                Word product = press(grain(raw >> 2, m), raw & 3, law[where], stock, budget);
                if (mode != 1)
                    observe(genes, cells, perception(raw, mode == 4, projection), product != 0);
                ++r.witnesses;
                available |= 1u << raw;
                if (product)
                    seen |= 1u << raw;
            }
            // Count whether the physical evidence distinguishes these two
            // baths. This statistic is never passed to the decision circuit.
            if (available) {
                Word other_pattern = 0;
                for (Word j = 0; j < 4; ++j)
                    other_pattern |= 1u << cue(j, raw_for(j, law[(mode == 3 ? bath ^ 1 : bath) ^ 1]));
                r.informative += ((other_pattern & available) != seen);
            }
            Word a = perception(cue(held, s0), mode == 4, projection),
                 b = perception(cue(held, s1), mode == 4, projection);
            Word choice = choose(genes, cells, a, b, k >> 12);
            auto weights = efficacy(genes, cells);
            r.ambiguous += estimate(genes, weights, a) == estimate(genes, weights, b);
            Word carbon = 9, opening = budget,
                 food = press(grain(held, k), choice ? s1 : s0, law[bath], carbon, budget);
            bool decidable = forced(available, seen, cue(held, s0), cue(held, s1), species);
            r.deducible += decidable;
            r.solved += decidable && food != 0;
            ++r.choices;
            r.correct += food != 0;
            r.absorbed += food;
            r.paid += opening + food - budget;
            if (!bath) {
                first_choice = choice;
                r.first += food != 0;
            } else {
                r.second += food != 0;
                r.reversed += choice != first_choice;
            }
        }
    }
}
} // namespace axion::life::brine

KERNEL void brine_birth(brine::Larva *parents, Word n, Word seed, const brine::Genes *pool, Word donors,
                        Word branches) {
    Word i = tid();
    if (i >= n)
        return;
    Word key = mix(seed ^ mix(i + 371));
    parents[i] = {donors ? pool[key % donors] : brine::founder(key, branches), i, 1};
}
KERNEL void brine_feed(const brine::Larva *parents, brine::Harvest *records, Word n, Word seed, Word epoch, Word tides,
                       Word species, Word offered, Word shore) {
    Word i = tid();
    if (i >= n)
        return;
    brine::Harvest h{};
    if (parents[i].live)
        brine::lifecycle(parents[i].genes, mix(seed ^ mix(epoch * 733 + i * 1277)), tides, species, offered, shore, h);
    records[i] = h;
}
KERNEL void brine_hatch(const brine::Larva *parents, brine::Larva *next, const brine::Harvest *records, Word n,
                        Word seed, Word epoch, Word drift, Word branches) {
    Word id = tid();
    if (id >= n)
        return;
    Word lane = id & 31, k = mix(seed + epoch * 1399), slot = (id * (k | 1) + (k >> 15)) & (n - 1);
    Word eggs = records[slot].eggs, all = eggs, count = eggs > 0;
    for (Word step = 1; step < 32; step *= 2) {
        all += raft_shuffle(all, lane ^ step);
        count += raft_shuffle(count, lane ^ step);
    }
    Word material = drift ? Word(eggs != 0) : eggs, capacity = drift ? count : all;
    Word prefix = material;
    for (Word step = 1; step < 32; step *= 2) {
        Word v = raft_shuffle(prefix, (lane - step) & 31);
        if (lane >= step)
            prefix += v;
    }
    if (!capacity) {
        next[id] = {};
        return;
    }
    Word key = mix(k + id * 331), occupied = all < 32 ? all : 32;
    Word start = lane * capacity / occupied, end = (lane + 1) * capacity / occupied;
    Word pick = start + (end > start ? key % (end - start) : 0), other = mix(key + 381) % capacity;
    auto find_parent = [&](Word egg) {
        Word offset = 0;
        for (Word width = 16; width; width >>= 1) {
            Word edge = raft_shuffle(prefix, offset + width - 1);
            if (edge <= egg)
                offset += width;
        }
        return (((id & ~31u) + offset) * (k | 1) + (k >> 15)) & (n - 1);
    };
    Word first = find_parent(pick), second = find_parent(other);
    if (lane < occupied)
        next[id] = {brine::descend(parents[first].genes, parents[second].genes, key, true, true, branches),
                    parents[first].family, 1};
    else
        next[id] = {};
}
KERNEL void brine_assess(const brine::Animal *pool, Word donors, brine::Reading *out, Word n, Word seed, Word trials,
                         Word species, Word offered, Word mode) {
    Word id = tid();
    if (id >= n)
        return;
    auto &p = pool[mix(seed + id * 6011) % donors];
    brine::Reading r{};
    brine::audit(p.genes, p.memory, mix(seed + id * 11069), trials, species, offered, mode, r);
    out[id] = r;
}

KERNEL void brine_land(World w, Configuration c, Word old, Word branches) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    Word district = (i % c.side) / 8 + 193 * ((i / c.side) / 8), seed = mix(c.seed + i * 2371);
    w.brine.solutes[i] = old ? brine::Salts{} : brine::dissolve(mix(c.seed + district * 13711), seed, w.brine.species);
    w.brine.animals[i] = {brine::founder(seed, branches), {}};
}
KERNEL void brine_extend(brine::Salts *land, const brine::Salts *saved, Word side, Word old) {
    Word i = tid();
    if (i < old * old)
        land[(i % old) + (i / old) * side] = saved[i];
}
KERNEL void brine_springs(World w, Configuration c, Word prior) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    Word district = (i % c.side) / 8 + 193 * ((i / c.side) / 8), seed = mix(c.seed + i * 2371);
    auto supplied = brine::dissolve(mix(c.seed + district * 13711), seed, w.brine.species);
    Word keep = (1u << (6 * prior)) - 1u;
    w.brine.solutes[i].ions = (w.brine.solutes[i].ions & keep) | (supplied.ions & ~keep);
}
KERNEL void brine_quiet_memory(World original, brine::Animal *copy, Word n) {
    Word i = tid();
    if (i < n)
        copy[i] = {original.brine.animals[i].genes, {}};
}
