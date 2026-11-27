namespace axion::life::siphon {
AX_FN Scene pool(Word k, Word bands, Word windows) {
    Word shutter = mix(k + 193);
    return {spectrum(mix(k + 441), bands),
            mix(k + 3011) & (bands - 1),
            (k >> 7) & 3,
            32,
            12 + ((k >> 18) & 15),
            (k >> 2) & 7,
            (k >> 11) & 7,
            (windows & 3) == 2 ? 3u
            : (windows & 3)    ? ((shutter >> 8) & 3)
                               : 0u,
            1,
            open_view(windows, k),
            Word((windows & 16) != 0)};
}
AX_FN Account tidal_life(Pair &pair, Word seed, Word visits, Word bands, Word windows, Word wire, bool plastic) {
    Word reserve[2]{64, 64};
    Account book{};
    book.opening = 128;
    for (Word j = 0; j < visits; ++j) {
        if (reserve[0] < 4 || reserve[1] < 4)
            break;
        Word k = mix(seed + j * 6151);
        bool flip = (k >> 10) & 1;
        auto &a = flip ? pair.b : pair.a, &b = flip ? pair.a : pair.b;
        auto room = pool(k, bands, windows);
        auto act = meet(a, b, room, k, wire);
        auto harvest = squeeze(room, act, b.genes.anatomy);
        book.stock += room.carbon;
        book.residual += room.carbon - harvest.food;
        reserve[flip] += harvest.speaker_share;
        reserve[!flip] += harvest.receiver_share;
        reserve[flip] -= harvest.cost_a;
        reserve[!flip] -= harvest.cost_b;
        Word upkeep = Word((k & 3) == 0);
        reserve[0] -= upkeep;
        reserve[1] -= upkeep;
        book.heat += harvest.cost_a + harvest.cost_b + 2 * upkeep;
        book.food += harvest.food;
        book.calls += Word(act.query < 4) + Word(act.reply < 4);
        book.strokes += act.stroke < 4;
        learn(a, b, room, act, harvest, wire, k, plastic);
    }
    book.closing = reserve[0] + reserve[1];
    book.eggs = book.closing / 16;
    return book;
}
AX_FN void assess(Pair pair, Word seed, Word bands, Word quartets, Word mode, Reading &r, Witness *witness = nullptr) {
    r.individuals = 2;
    r.visits = pair.a.occasions + pair.b.occasions;
    auto first = compile(pair.a), second = compile(pair.b);
    for (Word trial = 0; trial < quartets; ++trial) {
        Word key = mix(seed + trial * 5471);
        bool flip = (key >> 8) & 1;
        auto &a = flip ? second : first, &b = flip ? first : second;
        auto scene = pool(key, bands, 0);
        Word original = scene.inlet;
        for (Word branch = 0; branch < bands; ++branch) {
            scene.membrane = branch;
            Word succeeded = 0, sounds = 0, motors = 0;
            for (Word sample = 0; sample < 4; ++sample) {
                scene.inlet = spectrum((original & ~(3u << (2 * branch))) | (sample << (2 * branch)), bands);
                auto act = meet(a, b, scene, key, 1, mode);
                auto out = squeeze(scene, act, b.anatomy);
                ++r.trials;
                r.queries += act.query < 4;
                r.replies += act.reply < 4;
                r.strokes += act.stroke < 4;
                r.harvests += out.food != 0;
                succeeded += out.food != 0;
                r.energy += out.food;
                r.spent += out.cost_a + out.cost_b;
                r.attended += (act.attention & (bands - 1)) == branch;
                sounds |= 1u << act.reply;
                motors |= 1u << act.stroke;
                if (witness && !trial) {
                    if (!sample) {
                        witness->address[branch] = act.query;
                        witness->attended[branch] = act.attention;
                    }
                    witness->surface[branch] |= act.reply << (4 * sample);
                    witness->muscle[branch] |= act.stroke << (4 * sample);
                    witness->seen[branch] |= out.angle << (4 * sample);
                    witness->worked[branch] |= Word(out.food != 0) << sample;
                }
            }
            ++r.quartets;
            r.all_four += succeeded == 4;
            r.spoken_changes += __builtin_popcount(sounds) > 1;
            r.motor_changes += __builtin_popcount(motors) > 1;
        }
    }
}
} // namespace axion::life::siphon
KERNEL void siphon_seed(World w, Configuration c, Word old) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    Word key = mix(c.seed + i * 2539);
    w.siphons.residents[i] = {siphon::develop(key, w.siphons.windows & 32)};
    w.siphons.ports[i] = old ? 0 : mix(key + 157) & 255;
}
KERNEL void siphon_extend(Byte *ports, const Byte *old, Word side, Word prior) {
    Word i = tid();
    if (i < prior * prior)
        ports[i % prior + (i / prior) * side] = old[i];
}
KERNEL void siphon_born(siphon::Brood *out, Word n, Word seed, const siphon::Genes *field, Word donors, Word windows) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 881);
    out[i] = {donors ? field[k % donors] : siphon::develop(k, windows & 32),
              donors ? field[mix(k + 361) % donors] : siphon::develop((windows & 64) ? k : mix(k + 319), windows & 32),
              i, 1};
}
KERNEL void siphon_forage(const siphon::Brood *brood, siphon::Account *out, Word n, Word seed, Word epoch, Word visits,
                          Word bands, Word windows, Word wire) {
    Word i = tid();
    if (i >= n)
        return;
    siphon::Account a{};
    if (brood[i].alive) {
        siphon::Pair pair{{brood[i].a}, {brood[i].b}};
        a = siphon::tidal_life(pair, mix(seed ^ mix(epoch * 2671 + i * 7793)), visits, bands, windows, wire, true);
    }
    out[i] = a;
}
KERNEL void siphon_replace(const siphon::Brood *adults, siphon::Brood *young, const siphon::Account *stock, Word n,
                           Word seed, Word epoch, Word foster, Word windows) {
    Word i = tid();
    if (i >= n)
        return;
    Word lane = i & 31, k = mix(seed + epoch * 6203), slot = (i * (k | 1) + (k >> 9)) & (n - 1);
    Word eggs = stock[slot].eggs, all = eggs, capacity = foster ? Word(eggs != 0) : eggs, prefix = capacity;
    for (Word step = 1; step < 32; step *= 2) {
        all += raft_shuffle(all, lane ^ step);
        Word other = raft_shuffle(prefix, (lane - step) & 31);
        if (lane >= step)
            prefix += other;
    }
    capacity = raft_shuffle(prefix, 31);
    if (!capacity) {
        young[i] = {};
        return;
    }
    Word seats = all < 32 ? all : 32, key = mix(k + i * 2617);
    auto parent = [&](Word draw) {
        Word low = 0;
        for (Word width = 16; width; width >>= 1) {
            Word check = raft_shuffle(prefix, low + width - 1);
            if (check <= draw)
                low += width;
        }
        return (((i & ~31u) + low) * (k | 1) + (k >> 9)) & (n - 1);
    };
    Word begin = lane * capacity / seats, end = (lane + 1) * capacity / seats;
    Word u = parent(begin + (end > begin ? key % (end - begin) : 0)), v = parent(mix(key + 517) % capacity);
    if (lane >= seats) {
        young[i] = {};
        return;
    }
    auto &p = adults[u], &q = adults[v];
    auto one = (key & 1) ? p.a : p.b, two = (windows & 64) ? one : ((key & 2) ? q.a : q.b);
    young[i] = {siphon::inherit(one, two, key, 20, (key >> 7) & 1),
                siphon::inherit(two, one, mix(key + 101), 20, (key >> 8) & 1), p.clan, 1};
}
KERNEL void siphon_probe(const siphon::Pair *donors, Word na, siphon::Reading *out, siphon::Witness *first, Word n,
                         Word seed, Word visits, Word bands, Word trials, Word mode, Word windows) {
    Word i = tid();
    if (i >= n)
        return;
    Word key = mix(seed + i * 443), selected = mix(key + 1151) % na;
    auto p = donors[selected];
    if (mode == 10) {
        p.a = {p.a.genes};
        p.b = {p.b.genes};
    }
    if (visits)
        siphon::tidal_life(p, mix(key + 1009), visits, bands, windows, 1, mode != 9);
    Word learned = p.a.listened + p.b.listened;
    if (mode == 5) {
        p.a.address = p.a.surface = p.b.address = p.b.surface = 0;
    }
    if (mode == 6)
        p.a.tendon = p.b.tendon = 0;
    if (mode == 7) {
        auto other = donors[selected];
        if (visits)
            siphon::tidal_life(other, mix(key + 9001), visits, bands, windows, 1, true);
        p.b = other.b;
    }
    if (mode == 8) {
        auto other = donors[mix(key + 731) % na];
        if (visits)
            siphon::tidal_life(other, mix(key + 9001), visits, bands, windows, 1, true);
        p.b = other.b;
    }
    siphon::Reading r{};
    siphon::Witness w{};
    siphon::assess(p, mix(key + 3353), bands, trials, mode < 5 ? mode : 0, r, (!i && !mode) ? &w : nullptr);
    r.learning = learned;
    out[i] = r;
    if (!i && !mode)
        *first = w;
}
KERNEL void siphon_echo_extend(siphon::Echo *air, const siphon::Echo *saved, Word side, Word old) {
    Word i = tid();
    if (i < old * old)
        air[i % old + (i / old) * side] = saved[i];
}
namespace axion::life::siphon {
AX_FN Word hear_block(Animal &student, Word *observations, Word n, Word mode, Word &heard) {
    // In the control, rotate just the audible forms among the same visible
    // trials. The learner gets the same sights and number of contact events.
    Word edits = 0;
    for (Word i = 0; i < n; ++i) {
        Word mark = observations[i];
        if (!(mark & 128) || !(mark & 32))
            continue;
        ++heard;
        if (mode == 1)
            continue;
        if (mode == 2) {
            for (Word j = 1; j <= n; ++j) {
                Word next = observations[(i + j) % n];
                if ((next & 160) == 160) {
                    mark = (mark & ~3u) | (next & 3);
                    break;
                }
            }
        }
        edits += overhear(student, mark, true, true);
    }
    return edits;
}
AX_FN Word apprentice(Pair &adults, Animal &pupil, Word seed, Word visits, Word bands, Word windows, Word mode,
                      Word &heard, Word fixed) {
    Word edits = 0;
    // On an isolated slit each pupil feels the rotation that its own muscle
    // produces. All foster conditions have the same proprioceptive experience.
    for (Word j = 0; j < 32; ++j) {
        Word key = mix(seed + j * 719);
        Word stroke = key & 3, angle = code(stroke, pupil.genes.anatomy >> 9);
        pupil.tendon = adapt(pupil.tendon, angle, stroke, (pupil.genes.lateral >> 21) & 1, pupil.genes, true);
    }
    Word reserves[2]{64, 64};
    for (Word from = 0; from < visits; from += 8) {
        Word address[8]{}, names[8]{}, n = visits - from < 8 ? visits - from : 8;
        for (Word j = 0; j < n; ++j) {
            if (reserves[0] < 4 || reserves[1] < 4)
                break;
            Word k = mix(seed + (from + j) * 8317), side = (k >> 11) & 1;
            auto &a = side ? adults.b : adults.a, &b = side ? adults.a : adults.b;
            auto room = pool(k, bands, windows);
            auto act = meet(a, b, room, k, 1);
            auto result = squeeze(room, act, b.genes.anatomy);
            reserves[side] = reserves[side] - result.cost_a + result.speaker_share;
            reserves[side ^ 1] = reserves[side ^ 1] - result.cost_b + result.receiver_share;
            Word visible = act.stroke < 4 && ((windows & 3) == 2 || ((windows & 3) && ((k >> 7) & 3)));
            address[j] = radiate(0, act.query, room.membrane, true, visible, result.food != 0);
            names[j] = radiate(0, act.reply, act.percept, false, visible, result.food != 0);
            learn(a, b, room, act, result, 1, k, !fixed);
        }
        edits += hear_block(pupil, address, n, mode, heard);
        edits += hear_block(pupil, names, n, mode, heard);
    }
    return edits;
}
} // namespace axion::life::siphon
KERNEL void siphon_adoption(const siphon::Pair *pool, Word donors, siphon::Reading *out, Word subjects, Word seed,
                            Word prep, Word lessons, Word generations, Word bands, Word trials, Word mode, Word windows,
                            Word fixed) {
    Word i = tid();
    if (i >= subjects)
        return;
    Word base = mix(seed + i * 6373);
    auto adults = pool[mix(base + 827) % donors];
    if (prep)
        siphon::tidal_life(adults, mix(base + 193), prep, bands, windows, 1, true);
    auto original = adults,
         parallel = pool[(mix(base + 827) % donors + (donors > 1 ? 1 + mix(base + 4921) % (donors - 1) : 0)) % donors];
    if (prep)
        siphon::tidal_life(parallel, mix(base + 761), prep, bands, windows, 1, true);
    for (Word g = 0; g < generations; ++g) {
        Word key = mix(base + g * 2011);
        auto other = pool[mix(key + 3337) % donors];
        siphon::Animal pupil{(g & 1) ? other.b.genes : other.a.genes};
        Word heard = 0;
        auto teachers = adults;
        Word edits = siphon::apprentice(teachers, pupil, key, lessons, bands, windows, mode, heard, fixed);
        adults = {teachers.b, pupil};
        auto examined = adults;
        if (mode == 3)
            examined.a = parallel.a;
        if (mode == 4)
            examined.a = original.a;
        siphon::Reading result{};
        siphon::assess(examined, mix(key + 1151), bands, trials, 0, result);
        result.visits = heard;
        result.learning = edits;
        out[Count(g) * subjects + i] = result;
    }
}
KERNEL void siphon_survey(const siphon::Pair *population, siphon::PairScore *out, Word n, Word bands, Word rounds,
                          Word seed, Word permute) {
    Word i = tid();
    if (i >= n)
        return;
    auto pair = population[i];
    if (permute)
        pair.b = population[(i + permute) % n].b;
    siphon::Reading live{}, mute{}, quiet{};
    Word key = mix(seed + i * 1901);
    siphon::assess(pair, key, bands, rounds, 0, live);
    siphon::assess(pair, key, bands, rounds, 2, mute);
    siphon::assess(pair, key, bands, rounds, 1, quiet);
    out[i] = {live.trials, live.harvests, mute.harvests, quiet.harvests, live.all_four, live.quartets};
}
KERNEL void siphon_pack(const siphon::Animal *animals, siphon::Pocket *frozen, Word n) {
    Word i = tid();
    if (i < n)
        frozen[i] = siphon::compile(animals[i]);
}
namespace axion::life::siphon {
template <class Tissue> AX_FN Count contact_trace(const Tissue *tissue, Word animals, Word i, Word seed) {
    Word k = mix(seed + i * 499);
    auto &one = tissue[mix(k + 311) % animals], &two = tissue[mix(k + 613) % animals];
    auto room = pool(k, 4, (k >> 16) & 63);
    room.carbon = k & 31;
    room.charged = (k >> 3) & 1;
    auto act = meet(one, two, room, k, (k >> 24) & 3);
    auto out = squeeze(room, act, frame(two));
    Word neural = act.query | (act.reply << 3) | (act.attention << 6) | (act.percept << 8) | (act.recall << 10) |
                  (act.stroke << 12);
    Word material = out.angle | (out.actual << 2) | (out.opened << 5) | (out.food << 6) | (out.speaker_share << 11) |
                    (out.receiver_share << 16) | (out.cost_a << 21) | (out.cost_b << 23);
    return neural | (Count(material) << 32);
}
} // namespace axion::life::siphon
KERNEL void siphon_mutable(const siphon::Animal *animals, Word adults, Word calls, Word seed, Count *out) {
    Word i = tid();
    if (i < calls)
        out[i] = siphon::contact_trace(animals, adults, i, seed);
}
KERNEL void siphon_frozen(const siphon::Pocket *minds, Word adults, Word calls, Word seed, Count *out) {
    Word i = tid();
    if (i < calls)
        out[i] = siphon::contact_trace(minds, adults, i, seed);
}
KERNEL void siphon_mismatches(const Count *first, const Count *second, Word *error, Word calls) {
    Word i = tid();
    if (i < calls && first[i] != second[i])
        asm volatile("atom.global.add.u32 %0, [%1], 1;" : "=r"(i) : "l"(error) : "memory");
}
