// The nursery uses the same native optics, musculature and paid physical contacts
// as the field. The public object is the only training material supplied to a pupil.
namespace axion::life::clay {
AX_FN Tradition make_tradition(siphon::Pair first, siphon::Pair second, Word key, Word practice, Word visits,
                               Word windows, bool unrelated) {
    Tradition t{};
    for (Word j = 0; j < 2; ++j) {
        Word start = mix(key + j * 3299);
        auto pair = (j && unrelated) ? second : first;
        if (practice) {
            pair.a = {pair.a.genes};
            pair.b = {pair.b.genes};
            siphon::tidal_life(pair, mix(start + 193), practice, 4, windows, 1, true);
        }
        t.adults[j] = pair;
        // Equal mass and geometry, independently colored clay. The color
        // identifies no word, material category or productive aperture.
        t.mineral[j] = mix(key + 199) & 255;
        if (j)
            t.mineral[j] ^= 255;
        t.inlet_color[j] = t.mineral[j];
        auto hand_a = founder(mix(pair.a.genes.anatomy + 6119)), hand_b = founder(mix(pair.b.genes.anatomy + 1487));
        Word reserve[2]{96, 96};
        for (Word i = 0; i < visits; ++i) {
            if (reserve[0] < 5 || reserve[1] < 5)
                break;
            Word k = mix(start + i * 8719);
            Word side = (k >> 14) & 1;
            auto &a = side ? pair.b : pair.a, &b = side ? pair.a : pair.b;
            auto room = siphon::pool(k, 4, windows);
            auto act = siphon::meet(a, b, room, k, 1);
            auto out = siphon::squeeze(room, act, b.genes.anatomy);
            reserve[side] += out.speaker_share;
            reserve[side ^ 1] += out.receiver_share;
            reserve[side] -= out.cost_a;
            reserve[side ^ 1] -= out.cost_b;
            t.food[j] += out.food;
            t.labor[j] += out.cost_a + out.cost_b;
            for (Word port = 0; port < 2; ++port) {
                Word who = port ? (side ^ 1) : side, pose = port ? room.sink_pose : room.source_pose;
                auto &hand = who ? hand_b : hand_a;
                Word tone = port ? act.query : act.reply;
                if (tone >= 4 || act.stroke >= 4 || !(room.seen & (1u << port)) || !(hand.fingers & (1u << (pose & 7))))
                    continue;
                Word event = siphon::radiate(0, tone, port ? room.membrane : act.percept, port, true, out.food > 0);
                Word face = friction(hand, event, pose, k + port * 263);
                impress(t.artifacts[j][port], face, event, i + 1);
                --reserve[who];
                ++t.labor[j];
            }
        }
        t.capital[j] = reserve[0] + reserve[1];
    }
    return t;
}
AX_FN Pupil acquire(const Tradition &t, const siphon::Genes &native, Law law, Word opportunities, Word key, Word mode,
                    Word &sights, Word *paid = nullptr) {
    Pupil p{};
    p.body.genes = native;
    p.cortex.genes = law;
    if (mode == flat)
        p.cortex.genes.branches &= 0x3fffffffu;
    Word order = key & 1;
    // A pupil can feel the motion of its own intrinsic-to-physical tendons.
    // This practice has identical duration and cost in every treatment.
    if (paid)
        *paid += 24;
    for (Word i = 0; i < 24; ++i)
        rehearse(p.body, mix(key + i * 467) & 3, false);
    for (Word phase = 0; phase < 2; ++phase) {
        Word society = phase ^ order;
        for (Word i = 0; i < opportunities; ++i) {
            Word k = mix(key + phase * 3097 + i * 1213), motor = k & 255;
            if (!(law.fingers & (1u << (8 + (motor & 7)))))
                continue;
            if (paid)
                ++*paid;
            Word port = (k >> 21) & 1;
            auto &tile = t.artifacts[society][port];
            Word face = finger(law, motor, k, (k >> 9) & 3);
            Word use = read | color | latent;
            if (mode == shuffled)
                use |= shuffle;
            if (mode == scraped || mode == born_empty)
                continue;
            Word e = sample(tile, face, society + port, use);
            if (!(e & 128))
                continue;
            Word cue = mode == visually_hidden ? 256 : (port ? t.mineral[society] : t.inlet_color[society]);
            trace(p.cortex, p.body, e, cue, k, use, true);
            ++sights;
        }
    }
    return p;
}
AX_FN void inspect(Pupil &p, const Tile &tile, Word desired, Word k, Word taps, Word mode, Measure &r, Trace *w,
                   const Charts *charts = nullptr) {
    Word visited = 0;
    for (Word j = 0; j < taps; ++j) {
        Offered alternatives{};
        Word at = pluck(p.cortex, p.body, tile, visited, desired, mix(k + j * 799), mode == random_tap, charts,
                        &alternatives);
        bool contrast = alternatives.separating && alternatives.separating != alternatives.available;
        r.alternatives += contrast;
        r.pulses += contrast && at < 8 && bool(alternatives.separating & (1u << at));
        r.conflicts += alternatives.separating != 0;
        if (w)
            w->offered = alternatives.available;
        if (at == 8)
            break;
        visited |= 1u << at;
        Word event = groove(tile, at);
        ++r.anchors;
        ++r.spent;
        if (((event >> 2) & 3) == desired || !(event & 16))
            ++r.echoes;
        if (w) {
            w->address = at;
            w->observed = event & 255u;
        }
        if (mode != frozen_state)
            listen(p.cortex, p.body, event, charts);
    }
}
AX_FN void test_case(const Tradition &t, const Pupil &trained, Word source, Word filter, Word key, Word taps, Word mode,
                     Measure &r, Trace *w, const Charts *charts = nullptr, const Frozen *fixed = nullptr) {
    auto p = trained;
    swap_bank(p.cortex, p.body, 0);
    p.cortex.charge = (p.cortex.charge & 0xffff0000u) | 0x8888;
    if (mode == wordless) {
        p.body.address = p.body.surface = 0;
        for (Word j = 0; j < 3; ++j)
            p.cortex.address[j] = p.cortex.surface[j] = 0;
    }
    Word prior = active(p.cortex),
         cue = (mode == cued || mode == misleading) ? t.mineral[source ^ (mode == misleading)] : 256;
    orient(p.cortex, p.body, cue, key, false);
    if (mode != quiet && mode != cued && mode != misleading && mode != scraped && mode != born_empty) {
        inspect(p, t.artifacts[source ^ (mode == alien)][1], filter, key, taps, mode, r, w,
                mode == wordless ? nullptr : charts);
        orient(p.cortex, p.body, cue, key, false);
    }
    Word chosen = active(p.cortex);
    r.chosen += chosen != prior;
    ++r.probes;
    auto room = siphon::pool(key, 4, 0);
    room.membrane = filter;
    auto &up = t.adults[source].a;
    bool fast = fixed && mode != wordless;
    siphon::Pocket compiled{};
    if (fast)
        compiled = native_image(*fixed, chosen);
    Word open = 0;
    for (Word target = 0; target < 4; ++target) {
        room.inlet = (room.inlet & ~(3u << (2 * filter))) | (target << (2 * filter));
        Word wire = mode == ear_closed ? 2 : mode == mouth_closed ? 1 : 0;
        auto act = fast ? siphon::meet(up, compiled, room, key, 1, wire) : siphon::meet(up, p.body, room, key, 1, wire);
        auto out = siphon::squeeze(room, act, p.body.genes.anatomy);
        ++r.trials;
        r.harvests += out.food != 0;
        r.grips += act.stroke < 4;
        r.carbon += out.food;
        r.spent += out.cost_b;
        r.questions += act.query < 4;
        r.answers += act.reply < 4;
        open += out.food != 0;
        r.first += !target && out.food != 0;
        if (w && !target) {
            w->prior = prior;
            w->after = chosen;
            w->response = act.reply;
            w->grip = act.stroke;
            w->product = out.food;
            w->fixture = filter;
        }
    }
    ++r.quartets;
    r.four += open == 4;
}
AX_FN Measure assay(const Tradition &community, const siphon::Genes &native, Law gene, Word opportunities, Word taps,
                    Word repeats, Word key, Word mode, Trace *w = nullptr) {
    Measure r{};
    auto p = acquire(community, native, gene, opportunities, key, mode, r.seen);
    r.banks = count(p.cortex);
    auto frozen = compile(p);
    if (w) {
        w->active = p.cortex.banks;
        for (Word j = 0; j < 2; ++j) {
            w->minerals[j] = community.mineral[j];
            for (Word k = 0; k < 2; ++k)
                w->patterns[j][k] = community.artifacts[j][1].groove[k];
        }
    }
    for (Word rep = 0; rep < repeats; ++rep) {
        Word k = mix(key + rep * 983), desired = (k >> 12) & 3;
        Word first = r.harvests;
        test_case(community, p, 0, desired, k, taps, mode, r, w && rep == 0 ? w : nullptr, &frozen.charts, &frozen);
        Word other = r.harvests;
        test_case(community, p, 1, desired, k, taps, mode, r, nullptr, &frozen.charts, &frozen);
        r.success_both += other - first == 4 && r.harvests - other == 4;
    }
    return r;
}
} // namespace axion::life::clay
KERNEL void clay_traditions(const siphon::Pair *donors, Word donors_n, clay::Tradition *schools, Word n, Word seed,
                            Word practice, Word history, Word windows, Word unrelated) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 3917);
    schools[i] = clay::make_tradition(donors[mix(k + 731) % donors_n], donors[mix(k + 379) % donors_n], k, practice,
                                      history, windows, unrelated);
}
KERNEL void clay_study(const clay::Tradition *schools, Word nc, const clay::Law *genomes, Word ng, clay::Measure *out,
                       clay::Trace *w, Word n, Word seed, Word exposures, Word taps, Word repeats, Word mode) {
    Word i = tid();
    if (i >= n)
        return;
    Word key = mix(seed + i * 599);
    auto &c = schools[mix(key + 881) % nc];
    auto law = ng ? genomes[mix(key + 701) % ng] : clay::founder(mix(key + 411));
    out[i] =
        clay::assay(c, c.adults[(key >> 7) & 1].b.genes, law, exposures, taps, repeats, key, mode, i ? nullptr : w);
}
namespace axion::life::clay {
AX_FN Account living(const Tradition &t, Law law, Word key, Word lessons, Word taps) {
    Account r{};
    Word sights = 0, paid = 0;
    auto p = acquire(t, t.adults[(key >> 10) & 1].b.genes, law, lessons, key, adaptive, sights, &paid);
    Word reserve = 64 + 24 + 2 * lessons, adults[2]{96, 96};
    r.opening = reserve + 192;
    reserve -= paid;
    r.heat += paid;
    Word district = (key >> 17) & 1;
    auto creator0 = t.adults[0].a, creator1 = t.adults[1].a;
    for (Word j = 0; j < 128; ++j) {
        if (reserve < 8)
            break;
        Word k = mix(key + j * 2777);
        bool arrival = !j || ((k >> 13) & 7) == 0;
        if (arrival && j)
            district ^= 1;
        if (adults[district] < 3)
            continue;
        auto room = siphon::pool(k, 4, 0);
        if (arrival && (law.fingers & (1u << (8 + (room.sink_pose & 7))))) {
            Measure visit{};
            inspect(p, t.artifacts[district][1], room.membrane, k, taps < reserve - 4 ? taps : reserve - 4, adaptive,
                    visit, nullptr);
            reserve -= visit.spent;
            r.taps += visit.anchors;
            r.heat += visit.spent;
        }
        orient(p.cortex, p.body, 256, k, false);
        auto &up = district ? creator1 : creator0;
        auto act = siphon::meet(up, p.body, room, k, 1);
        auto out = siphon::squeeze(room, act, p.body.genes.anatomy);
        r.stock += room.carbon;
        r.residual += room.carbon - out.food;
        r.meals += out.food;
        reserve = reserve - out.cost_b + out.receiver_share;
        adults[district] += out.speaker_share;
        adults[district] -= out.cost_a;
        Word upkeep = 1 + Word((k & 15) < count(p.cortex));
        reserve -= upkeep;
        r.heat += out.cost_a + out.cost_b + upkeep;
    }
    r.closing = reserve + adults[0] + adults[1];
    r.eggs = reserve / 16;
    return r;
}
} // namespace axion::life::clay
KERNEL void clay_breed(const clay::Tradition *schools, Word nc, clay::Brood *mature, clay::Brood *young, Word n,
                       Word seed, Word epoch, Word exposures, Word taps, Word provisioned, clay::Account *accounts) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + epoch * 2971 + i * 3797);
    auto germ = epoch ? mature[i] : clay::Brood{clay::founder(mix(seed + i * 127)), 1};
    clay::Account account{};
    if (germ.alive)
        account = clay::living(schools[mix(k + 311) % nc], germ.genes, k, exposures, taps);
    accounts[i] = account;
    Word lane = i & 31, all = account.eggs, prefix = provisioned ? all : Word(all != 0);
    for (Word d = 1; d < 32; d *= 2) {
        all += raft_shuffle(all, lane ^ d);
        Word prev = raft_shuffle(prefix, (lane - d) & 31);
        if (lane >= d)
            prefix += prev;
    }
    Word total = raft_shuffle(prefix, 31), seats = all < 32 ? all : 32;
    Word perm = mix(seed + epoch * 67), dest = (i * (perm | 1) + (perm >> 9)) & (n - 1);
    if (!total) {
        young[dest] = {};
        return;
    }
    Word begin = lane * total / seats, end = (lane + 1) * total / seats,
         draw = begin + (end > begin ? mix(k + 439) % (end - begin) : 0), lo = 0;
    for (Word width = 16; width; width >>= 1) {
        Word before = raft_shuffle(prefix, lo + width - 1);
        if (before <= draw)
            lo += width;
    }
    auto take = [&](Word v) { return raft_shuffle(v, lo); };
    clay::Law g{take(germ.genes.branches), take(germ.genes.membrane), take(germ.genes.fingers)};
    young[dest] = lane < seats ? clay::Brood{clay::inherit(g, k, 64), 1} : clay::Brood{};
}
KERNEL void clay_found(clay::Field field, Word seed, Word n) {
    Word i = tid();
    if (i < n) {
        field.minds[i] = {clay::founder(mix(seed + i * 10039))};
        field.palms[i] = {};
        field.ground[i] = {};
    }
}
KERNEL void clay_widen(clay::Tile *widest, const clay::Tile *former, Word side, Word old) {
    Word i = tid();
    if (i < old * old)
        widest[(i / old) * side + i % old] = former[i];
}
KERNEL void clay_screen(const clay::Tradition *schools, Word nc, const clay::Law *laws, Word ng, Word subjects,
                        Word seed, Word exposures, Word taps, Word *scores) {
    Word i = tid();
    if (i >= subjects)
        return;
    Word k = mix(seed + i * 599);
    auto &c = schools[mix(k + 881) % nc];
    auto law = laws[mix(k + 701) % ng];
    Word hits = 0;
    auto p = clay::acquire(c, c.adults[(k >> 7) & 1].b.genes, law, exposures, k, 0, hits);
    clay::Measure normal{}, reversed{};
    for (Word r = 0; r < 24; ++r) {
        Word salt = mix(k + r * 1153);
        for (Word t = 0; t < 2; ++t) {
            clay::test_case(c, p, t, r & 3, salt, taps, clay::adaptive, normal, nullptr);
            clay::test_case(c, p, t, r & 3, salt, taps, clay::alien, reversed, nullptr);
        }
    }
    // Screen for accurate native exchange and sensitivity to whose clay is
    // actually presented. Later tests use newly generated physical tables.
    scores[i] = normal.harvests * 256 + (normal.trials - reversed.harvests);
}
KERNEL void clay_recover(const clay::Tradition *schools, Word nc, const clay::Law *laws, Word ng, Word index, Word seed,
                         Word lessons, clay::Specimen *result) {
    if (tid())
        return;
    Word k = mix(seed + index * 599), choice = mix(k + 881) % nc;
    auto &c = schools[choice];
    auto law = laws[mix(k + 701) % ng];
    clay::Specimen s{};
    s.ancestor = (k >> 7) & 1;
    s.school = choice;
    s.child = clay::acquire(c, c.adults[s.ancestor].b.genes, law, lessons, k, 0, s.materials, &s.paid);
    *result = s;
}
namespace axion::life::clay {
AX_FN void descendants(Tradition &t, Pupil &a, Pupil &b, Word key, Word visits, Word windows, Generation &r) {
    for (Word community = 0; community < 2; ++community) {
        orient(a.cortex, a.body, t.mineral[community], key, false);
        orient(b.cortex, b.body, t.mineral[community], key, false);
        t.artifacts[community][0] = {};
        t.artifacts[community][1] = {};
        Word reserve[2]{96, 96};
        t.adults[community] = {a.body, b.body};
        for (Word j = 0; j < visits; ++j) {
            if (reserve[0] < 6 || reserve[1] < 6)
                break;
            Word k = mix(key + j * 2677 + community * 1009), flip = (k >> 17) & 1;
            auto &one = flip ? b : a, &two = flip ? a : b;
            auto site = siphon::pool(k, 4, windows);
            auto act = siphon::meet(one.body, two.body, site, k, 1);
            auto out = siphon::squeeze(site, act, two.body.genes.anatomy);
            reserve[flip] = reserve[flip] - out.cost_a + out.speaker_share;
            reserve[flip ^ 1] = reserve[flip ^ 1] - out.cost_b + out.receiver_share;
            r.food += out.food;
            r.cost += out.cost_a + out.cost_b;
            for (Word port = 0; port < 2; ++port) {
                auto &hand = port ? two.cortex.genes : one.cortex.genes;
                Word pose = port ? site.sink_pose : site.source_pose;
                Word voice = port ? act.query : act.reply;
                if (voice >= 4 || act.stroke >= 4 || !(site.seen & (1u << port)) || !(hand.fingers & (1u << pose)))
                    continue;
                Word pulse = siphon::radiate(0, voice, port ? site.membrane : act.percept, port, true, out.food != 0);
                r.formed +=
                    impress(t.artifacts[community][port], friction(hand, pulse, pose, k + port * 263), pulse, j + 1);
                --reserve[port ? (flip ^ 1) : flip];
                ++r.cost;
            }
        }
    }
}
} // namespace axion::life::clay
KERNEL void clay_generations(const clay::Tradition *schools, Word nc, const clay::Law *laws, Word ng,
                             clay::Generation *rows, Word n, Word seed, Word lessons, Word taps, Word repeats,
                             Word generations, Word history, Word windows, Word mode) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 2333);
    auto original = schools[mix(k + 1117) % nc], t = original;
    for (Word gen = 0; gen < generations; ++gen) {
        Word s = mix(k + gen * 647);
        clay::Generation r{};
        auto &relatives = schools[mix(s + 411) % nc].adults[0];
        auto a = clay::acquire(t, relatives.a.genes, laws[mix(s + 557) % ng], lessons, s, mode, r.sightings);
        auto b = clay::acquire(t, relatives.b.genes, laws[mix(s + 761) % ng], lessons, mix(s + 131), mode, r.sightings);
        // The original adults are brought back only as assay partners. The
        // pupil has no contact with them while producing the next two objects.
        auto trial = t;
        trial.adults[0] = original.adults[0];
        trial.adults[1] = original.adults[1];
        clay::Measure native{}, crossed{}, closed{};
        for (Word j = 0; j < repeats; ++j) {
            Word key = mix(s + j * 543), filter = (key >> 15) & 3;
            for (Word town = 0; town < 2; ++town) {
                clay::test_case(trial, b, town, filter, key, taps, clay::adaptive, native, nullptr);
                clay::test_case(trial, b, town, filter, key, taps, clay::alien, crossed, nullptr);
                clay::test_case(trial, b, town, filter, key, taps, clay::ear_closed, closed, nullptr);
            }
        }
        r.tests = native.trials;
        r.native = native.harvests;
        r.foreign = crossed.harvests;
        r.silent = closed.harvests;
        r.paired = native.four;
        clay::descendants(t, a, b, mix(s + 919), history, windows, r);
        rows[Count(gen) * n + i] = r;
    }
}
KERNEL void clay_migrants(const siphon::Animal *population, Word n, const siphon::Animal *witnesses, Word cycles,
                          Word seed, Word arm, clay::Generation *out) {
    Word i = tid();
    if (i >= n)
        return;
    auto animal = population[i];
    if (arm == 2)
        animal.address = animal.surface = 0;
    if (arm == 3)
        animal.tendon = 0;
    clay::Generation r{};
    for (Word j = 0; j < cycles; ++j) {
        Word key = mix(seed + i * 1549 + j * 751), filter = (key >> 8) & 3;
        auto room = siphon::pool(key, 4, 0);
        room.membrane = filter;
        for (Word community = 0; community < 2; ++community) {
            Word successes = 0;
            for (Word target = 0; target < 4; ++target) {
                room.inlet = (room.inlet & ~(3u << (2 * filter))) | (target << (2 * filter));
                auto act = siphon::meet(witnesses[community], animal, room, key, 1, arm == 1 ? 2 : 0);
                auto outcome = siphon::squeeze(room, act, animal.genes.anatomy);
                successes += outcome.food != 0;
                if (!community) {
                    ++r.tests;
                    r.food += act.stroke < 4;
                }
            }
            if (community) {
                r.foreign += successes;
                r.formed += successes == 4;
            } else {
                r.native += successes;
                r.paired += successes == 4;
            }
        }
    }
    out[i] = r;
}
KERNEL void clay_tides(const clay::Tradition *schools, Word nc, const clay::Law *genes, Word ng, clay::Turn *out,
                       Word n, Word seed, Word lessons, Word loops, Word mode) {
    Word i = tid();
    if (i >= n)
        return;
    Word key = mix(seed + i * 7753);
    auto &t = schools[mix(key + 937) % nc];
    Word seen = 0;
    auto p = clay::acquire(t, t.adults[(key >> 7) & 1].b.genes, genes[mix(key + 71) % ng], lessons, key,
                           mode == 2 ? clay::flat : clay::adaptive, seen);
    auto charts = clay::freeze(p.cortex, p.body);
    clay::Turn by_age[8]{};
    // The experimenter changes the speaker between encounters. The learning
    // circuit receives only the conventional sounds and the visible relief.
    Word community = key & 1, age = 0;
    clay::orient(p.cortex, p.body, t.mineral[community], key, false);
    for (Word visit = 0; visit < loops; ++visit) {
        Word noise = mix(key + visit * 5503);
        if (visit && ((noise >> 17) & 7) == 0) {
            community ^= 1;
            age = 0;
        }
        auto &measure = by_age[age];
        Word filter = noise & 3, prior = clay::active(p.cortex);
        auto scene = siphon::pool(noise, 4, 0);
        scene.membrane = filter;
        Word hits = 0;
        for (Word x = 0; x < 4; ++x) {
            scene.inlet = (scene.inlet & ~(3u << (2 * filter))) | (x << (2 * filter));
            auto a = siphon::meet(t.adults[community].a, p.body, scene, noise, 1, mode == 4 ? 2 : 0);
            auto r = siphon::squeeze(scene, a, p.body.genes.anatomy);
            ++measure.trials;
            hits += r.food != 0;
        }
        measure.productive += hits;
        measure.complete += hits == 4;
        ++measure.turns;
        // A public address on another filter arrives *after* all four scored grips.
        Word local = (filter + 1 + (noise >> 9) % 3) & 3;
        auto &speaker = t.adults[community ^ (mode == 3)].b;
        Word glyph = siphon::query(speaker, local, mix(noise + 733), (noise >> 19) & 7);
        if (mode != 1 && glyph < 4) {
            ++measure.evidence;
            clay::listen(p.cortex, p.body, siphon::radiate(0, glyph, local, true, true, false), &charts);
            clay::orient(p.cortex, p.body, 256, noise, false);
        }
        measure.changes += clay::active(p.cortex) != prior;
        age = age < 7 ? age + 1 : 7;
    }
    for (Word j = 0; j < 8; ++j)
        out[Count(j) * n + i] = by_age[j];
}
KERNEL void clay_compile(const clay::Portable *input, clay::Deployment *out, Word n) {
    Word i = tid();
    if (i < n)
        out[i] = {clay::compile(input[i].pupil), input[i].object};
}
KERNEL void clay_compile_one(const siphon::Pair *p, const clay::Mind *m, clay::Charts *out) {
    if (!tid())
        *out = clay::freeze(*m, p->b);
}
namespace axion::life::clay {
template <class T> AX_FN Count finish_decision(const T &brain, const Mind &m, Word key, Word at, Word event) {
    Word filter = (key >> 12) & 3, tone = siphon::query(brain, filter, key, (key >> 7) & 7);
    siphon::Intent movement{};
    Word grip = siphon::press(brain, key & 3, (key >> 4) & 3, mix(key + 971), (key >> 7) & 7, movement);
    Word recall = movement.recall;
    auto reply = siphon::reply(brain, (key >> 16) & 255, tone, mix(key + 733), (key >> 11) & 7, movement);
    Word outcome =
        at | (event << 4) | (tone << 12) | (grip << 16) | (recall << 20) | (active(m) << 22) | (resolved(m) << 25);
    Word evidence = m.charge | (reply << 16) | (movement.attention << 19) | (movement.percept << 21);
    return outcome | (Count(evidence) << 32);
}
AX_FN Count decision(const Portable &animal, Word key) {
    auto p = animal.pupil;
    Word filter = (key >> 12) & 3;
    auto &obj = animal.object;
    Word at = pluck(p.cortex, p.body, obj, 0, filter, key, false), event = at < 8 ? groove(obj, at) : 0;
    listen(p.cortex, p.body, event);
    orient(p.cortex, p.body, 256, key, false);
    return finish_decision(p.body, p.cortex, key, at, event);
}
AX_FN Count decision(const Deployment &data, Word key) {
    auto &f = data.brain;
    Mind m{};
    m.genes = f.law;
    m.colors = f.colors;
    m.charge = f.charge;
    m.banks = f.banks;
    siphon::Animal sensory{};
    sensory.genes.anatomy = f.frame;
    Word at = pluck(m, sensory, data.tile, 0, (key >> 12) & 3, key, false, &f.charts),
         event = at < 8 ? groove(data.tile, at) : 0;
    listen(m, sensory, event, &f.charts);
    orient(m, sensory, 256, key, false);
    auto motor = native_image(f, active(m));
    return finish_decision(motor, m, key, at, event);
}
} // namespace axion::life::clay
KERNEL void clay_dynamic(const clay::Portable *p, Word n, Count *out, Word calls, Word seed) {
    Word i = tid();
    if (i < calls) {
        Word k = mix(seed + i * 331);
        out[i] = clay::decision(p[mix(k + 381) % n], k);
    }
}
KERNEL void clay_projected(const clay::Deployment *p, Word n, Count *out, Word calls, Word seed) {
    Word i = tid();
    if (i < calls) {
        Word k = mix(seed + i * 331);
        out[i] = clay::decision(p[mix(k + 381) % n], k);
    }
}
