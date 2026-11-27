void replace_floats(Habitat &h, const Habitat &donor) {
    auto &b = h.loom.book;
    h.float_exports = b.trapped + h.float_imports - b.meals;
    h.loom.version = std::max(2u, h.loom.version);
    if (donor.braiding) {
        for (auto f : donor.floats)
            h.float_imports += braid::mass(f);
        s.d.check(s.d.upload(floaters.ptr, donor.floats.data(), Count(n) * sizeof(braid::Half)),
                  "replace a physical flotilla");
    } else {
        Function reset{};
        s.bind(reset, "braid_remoor", Image::floodplain);
        launch(reset, (n + 127) / 128, 128, floaters.ptr, n, c.side, h.loom.seed);
        synchronize();
    }
}
void braid_rear(Habitat &h, const Options &o) {
    if (!o.braid_epochs && o.braid_source.empty())
        return;
    std::vector<braid::Genes> germs;
    if (!o.braid_source.empty()) {
        Habitat donor;
        donor.read(o.braid_source);
        if (donor.braiding)
            for (Word i = 0; i < donor.sites(); ++i)
                if (donor.bodies[i].energy)
                    germs.push_back(donor.riggers[i].genes);
        if (germs.empty())
            throw std::invalid_argument("float germ donor has no living operators");
    }
    Word size = o.braid_parents, ng = germs.size(), salt = o.braid_seed, names = o.braid_terms, paid = o.braid_paid,
         mixed = o.braid_acquire;
    Temporary src(s.d, Count(std::max(1u, ng)) * sizeof(braid::Genes)), pop(s.d, Count(size) * sizeof(braid::Family)),
        next(s.d, Count(size) * sizeof(braid::Family));
    if (ng)
        s.d.check(s.d.upload(src.ptr, germs.data(), Count(ng) * sizeof(germs[0])), "restore float founders");
    Function found{}, test{}, breed{}, settle{};
    s.bind(found, "braid_stock", Image::runnels);
    s.bind(test, o.braid_school ? "braid_community_food" : "braid_select", Image::runnels);
    s.bind(breed, "braid_spawn", Image::runnels);
    s.bind(settle, "braid_rehouse", Image::runnels);
    launch(found, (size + 127) / 128, 128, pop.ptr, size, salt, src.ptr, ng);
    for (Word i = 0; i < o.braid_epochs; ++i) {
        if (o.braid_school) {
            Word encounters = o.braid_visits, filters = o.braid_school == 2;
            launch(test, (size + 63) / 64, 64, pop.ptr, size, i, salt, encounters, filters);
        } else
            launch(test, (size + 63) / 64, 64, pop.ptr, size, i, salt, names, mixed);
        if (!i || i + 1 == o.braid_epochs || !((i + 1) & 63)) {
            synchronize();
            std::vector<braid::Family> folk(size);
            s.d.check(s.d.download(folk.data(), pop.ptr, Count(size) * sizeof(folk[0])), "count filtered food");
            Count food = 0;
            for (auto &w : folk)
                food += w.food;
            std::cout << "braid_brood=" << i + 1 << "  parents=" << size << "  recovered=" << food
                      << "  offered=" << Count(size) * (o.braid_school == 1 ? 448 : 192)
                      << "  nursery=" << o.braid_school << "  selection=" << (paid ? "food" : "uniform")
                      << "  source=" << ng << '\n';
        }
        launch(breed, (size + 127) / 128, 128, pop.ptr, next.ptr, size, i, salt, paid);
        std::swap(pop.ptr, next.ptr);
    }
    launch(settle, (n + 127) / 128, 128, riggers.ptr, n, pop.ptr, size, salt);
    synchronize();
    h.flute_words.clear();
    h.narrators.clear();
    h.stories.clear();
    if (trails.ptr) s.d.check(s.d.zero(trails.ptr,0,Count(n)*sizeof(utterance::Trail)),"clear replaced cortical histories");
    if (float_events.ptr)
        s.d.check(s.d.zero(float_events.ptr, 0, Count(n) * sizeof(braid::Episode)),
                  "clear memories on experimental replacement");
    if (sluice_contacts.ptr)
        s.d.check(s.d.zero(sluice_contacts.ptr, 0, Count(n) * sizeof(braid::GateMemory)),
                  "reset born pressure contacts");
}
void braid_speed(const std::vector<braid::Mind> &pool, const Options &o) {
    Word size = std::min(Word(pool.size()), 4096u), calls = o.braid_bench, terms = o.braid_terms, salt = o.braid_seed;
    Temporary minds(s.d, Count(size) * sizeof(pool[0])), atlas(s.d, Count(size) * sizeof(braid::Atlas)),
        out(s.d, Count(calls) * 4);
    Function expose{}, build{}, read{};
    s.bind(expose, "braid_prepare", Image::runnels);
    s.bind(build, "braid_images", Image::runnels);
    s.bind(read, "braid_read", Image::runnels);
    Event a{}, b{};
    s.d.check(s.d.event_new(&a, 0), "create float clock");
    s.d.check(s.d.event_new(&b, 0), "create read clock");
    auto time = [&](auto f) {
        f();
        synchronize();
        std::array<float, 5> ms{};
        for (auto &t : ms) {
            s.d.check(s.d.record(a, stream), "start float clock");
            f();
            s.d.check(s.d.record(b, stream), "stop float clock");
            synchronize();
            s.d.check(s.d.elapsed(&t, a, b), "time dispatch");
        }
        std::sort(ms.begin(), ms.end());
        return ms[2];
    };
    s.d.check(s.d.upload(minds.ptr, pool.data(), Count(size) * sizeof(pool[0])), "sample float tissue");
    for (Word trained = 0; trained < 2; ++trained) {
        if (trained)
            launch(expose, (size + 63) / 64, 64, minds.ptr, size, terms, salt);
        auto ms = time([&] { launch(build, (size + 3) / 4, 128, minds.ptr, size, atlas.ptr); });
        float dt[2]{};
        std::vector<Word> direct(calls), packed(calls);
        for (Word compiled = 0; compiled < 2; ++compiled) {
            dt[compiled] = time([&] {
                launch(read, (calls + 127) / 128, 128, minds.ptr, atlas.ptr, size, calls, terms, compiled, out.ptr);
            });
            s.d.check(s.d.download(compiled ? packed.data() : direct.data(), out.ptr, Count(calls) * 4),
                      "check every compiled motor sequence");
        }
        Count errors = 0, hits = 0;
        for (Word i = 0; i < calls; ++i) {
            errors += direct[i] != (packed[i] & 0x7fffffff);
            hits += packed[i] >> 31;
        }
        std::cout << "braid_image=" << size << "  source=" << (trained ? "work" : "field") << "  calls=" << calls
                  << "  hits=" << hits << "  mismatches=" << errors << "  bytes=" << sizeof(braid::Atlas)
                  << "  build_ms=" << ms << "  live_ms=" << dt[0] << "  packed_ms=" << dt[1] << '\n';
        if (errors)
            throw std::logic_error("compiled float route diverged");
    }
    s.d.event_free(a);
    s.d.event_free(b);
}
void braid_study(Habitat &h, const Options &o) {
    if (!h.braiding)
        return;
    std::vector<braid::Mind> folk;
    std::vector<braid::Genes> parents;
    Count knowledge = 0, vowels = 0, distant = 0, stock = 0, transported = 0;
    Word reach = 0, side = c.side;
    for (Word i = 0; i < n; ++i) {
        auto f = h.floats[i];
        auto dist = braid::distance(f, i % side, i / side);
        stock += braid::mass(f);
        reach = std::max(reach, dist);
        distant += dist >= 3;
        transported += !!(f & 128);
        if (!h.bodies[i].energy)
            continue;
        const auto &m = h.riggers[i];
        parents.push_back(m.genes);
        folk.push_back(m);
        knowledge += std::popcount(m.lived & 15u);
        vowels += std::popcount(Word((m.lived >> 4) & 255));
    }
    const auto &b = h.loom.book;
    std::cout << "braid_field=" << h.header.tick << "  living=" << folk.size() << "  floats=" << n
              << "  carried_floats=" << transported << "  outside=" << distant << "  furthest=" << reach
              << "  carbon=" << stock << "  trapped=" << b.trapped << "  imported=" << h.float_imports
              << "  exported=" << h.float_exports << "  swallowed=" << b.meals << "  distant_food=" << b.traced_food
              << "  distant_meals=" << b.traced_pulls << "  work=" << b.spent << "  strokes=" << b.strokes
              << "  transfers=" << b.transferred << "  carried=" << b.carried << "  shifted_carbon=" << b.across
              << "  motor_cells=" << knowledge << "  acoustic_cells=" << vowels << "  bytes="
              << sizeof(braid::Mind) + 2 + (h.loom.version >= 3 ? sizeof(braid::GateMemory) : 0) +
                     (h.recording_floats ? sizeof(braid::Episode) : 0)
              << '\n';
    std::cout << "braid_signals=" << h.header.tick << "  spoken=" << b.spoken << "  heard=" << b.heard
              << "  coined=" << b.babbled << "  named=" << b.named << "  edits=" << b.changes
              << "  sought=" << b.requested << "  known_pulls=" << b.known_pulls << "  better=" << b.goal_gain
              << "  worse=" << b.goal_loss << "  material_events=" << b.soma_events
              << "  material_spikes=" << b.soma_changed << "  material_moves=" << b.soma_moves
              << "  bank_events=" << b.ear_events << "  bank_spikes=" << b.ear_changed << "  bank_moves=" << b.ear_moves
              << '\n';
    if (h.loom.version >= 3) {
        auto &g = h.sluice_book;
        Count marks = 0, open = 0;
        for (Word i = 0; i < n; ++i)
            if (h.bodies[i].energy) {
                marks += std::popcount(h.sluice_contacts[i].visited);
                open += std::popcount(h.sluice_contacts[i].open);
            }
        std::cout << "braid_gates=" << h.header.tick << "  phase=" << h.gate_phase << "  wet=" << g.wet
                  << "  contacts=" << g.crossings << "  carbon=" << g.flux << "  intercepted=" << g.picked
                  << "  observed=" << g.observed << "  edits=" << g.writes << "  remembered=" << marks
                  << "  conductive=" << open << "  calls=" << g.requests << "  inferred=" << g.predicted_requests
                  << "  syllables=" << g.syllables << "  composed=" << g.patterned << "  responses=" << g.responses
                  << "  understood=" << g.understood << "  pulls=" << g.pulls << "  changed=" << g.heard_work
                  << "  improved=" << g.better << "  worsened=" << g.worse << "  extra=" << g.hearing_surplus
                  << "  lost=" << g.hearing_deficit << "  learned_changes=" << g.learned_work
                  << "  learned_extra=" << g.learned_surplus << "  learned_loss=" << g.learned_deficit << '\n';
    }
    if (h.loom.version >= 5) {
        std::array<bool, 576> morph{};
        Count seen = 0, colored = 0, turns = 0;
        Word types = 0;
        for (Word i = 0; i < n; ++i)
            if (h.bodies[i].energy) {
                auto lens = braid::eye_wiring(h.loom.mode, h.riggers[i].genes, h.bodies[i].heading);
                Word k = 24 * braid::address_of(lens.places) + braid::address_of(lens.hues);
                if (!morph[k]) {
                    ++types;
                    morph[k] = true;
                }
                colored += lens.hues != braid::order;
                turns += lens.places != braid::order;
                seen += h.recording_floats && h.float_events[i].stamp;
            }
        std::cout << "braid_eyes=" << h.header.tick << "  local=" << bool(h.loom.mode & braid::individual_eyes)
                  << "  perspectives=" << types << "  recollections=" << seen << "  turned=" << turns
                  << "  filtered=" << colored << "  residents=" << folk.size() << '\n';
    }
    if (h.loom.version >= 4) {
        auto &z = h.sediment_book;
        std::cout << "braid_sediment=" << h.header.tick << "  mode="
                  << ((h.loom.mode & braid::mobile_bed)    ? "mobile"
                      : (h.loom.mode & braid::mineral_bed) ? "pinned"
                                                           : "bypass")
                  << "  wet=" << z.wet << "  shaped=" << z.relieved << "  opened=" << z.opened
                  << "  closed=" << z.closed << "  gained=" << z.gained << "  lost=" << z.lost
                  << "  lodged=" << z.lodged << "  scoured=" << z.scoured << "  under_motion=" << z.turned << '\n';
    }
    Word bins = std::min(side, 16u), step = side / bins;
    for (Word y = 0; y < bins; ++y) {
        std::string encoded, cargo, retinas;
        constexpr char hex[] = "0123456789abcdef";
        for (Word x = 0; x < bins; ++x) {
            Word moved = 0, distance = 0, units = 0, eyes[16]{}, living = 0;
            for (Word a = 0; a < step; ++a)
                for (Word j = 0; j < step; ++j) {
                    Word xx = x * step + j, yy = y * step + a;
                    Word site = yy * side + xx;
                    auto f = h.floats[site];
                    if (h.loom.version >= 5) {
                        Word id = h.slots[site];
                        if (h.bodies[id].energy) {
                            auto lens = braid::eye_wiring(h.loom.mode, h.riggers[id].genes, h.bodies[id].heading);
                            ++eyes[(lens.places & 3) + 4 * (lens.hues & 3)];
                            ++living;
                        }
                    }
                    units += braid::mass(f);
                    moved += !!(f & 128);
                    distance += braid::distance(f, xx, yy) >= 3;
                }
            auto k = std::min(3u, 4 * moved / (step * step)) + 4 * std::min(3u, 16 * distance / (step * step));
            if (h.loom.version >= 5) {
                Word common = std::max_element(std::begin(eyes), std::end(eyes)) - eyes;
                retinas += hex[living ? std::max(1u, 15 * eyes[common] / living) : 0];
                retinas += hex[common];
            }
            encoded += hex[k];
            Word quarter = (4 * units + step * step / 2) / (step * step);
            cargo += hex[quarter >> 4];
            cargo += hex[quarter & 15];
        }
        std::cout << "braid_map=" << y << "  cells=" << encoded << '\n';
        if (h.loom.version >= 5)
            std::cout << "braid_retina=" << y << "  eyes=" << retinas << '\n';
        std::cout << "braid_cargo=" << y << "  quarters=" << cargo << '\n';
    }
    if (!o.braid_voice.empty()) {
        Word slot = o.slot;
        if (slot == ~0u) {
            slot = 0;
            while (slot < n && !h.bodies[slot].energy)
                ++slot;
        }
        if (slot >= n || !h.bodies[slot].energy)
            throw std::invalid_argument("a float speaker requires a living slot");
        transfer(h, false);
        auto person = extract_individual(h, slot, false);
        storage::write_checked(o.braid_voice, person);
        std::cout << "braid_speaker=" << slot << "  cells=" << std::popcount(Word(h.riggers[slot].lived)&~braid::lineage_bits) << '\n';
    }
    if (!o.braid_perspectives && !o.braid_inquiries && !o.braid_cases && !o.braid_bench && !o.braid_dialogues &&
        !o.braid_villages && o.braid_chorister.empty())
        return;
    if (folk.empty())
        throw std::invalid_argument("the requested float study requires residents");
    if (o.braid_villages || !o.braid_chorister.empty())
        braid_culture(h, o, parents);
    if (o.braid_perspectives)
        viewpoint_study(parents, o);
    if (o.braid_inquiries)
        braid_inquiries(parents, o);
    if (o.braid_dialogues)
        braid_dialogues(parents, o);
    if (o.braid_bench)
        braid_speed(folk, o);
    if (!o.braid_cases)
        return;
    Word np = parents.size(), count = o.braid_cases, salt = o.braid_seed, rep = o.braid_rep, len = o.braid_length,
         terms = o.braid_terms, coupled = o.braid_chiral, grammar = o.braid_acquire;
    Temporary stock_dev(s.d, Count(np) * sizeof(parents[0])), rec(s.d, Count(count) * sizeof(braid::Trial));
    s.d.check(s.d.upload(stock_dev.ptr, parents.data(), Count(np) * sizeof(parents[0])),
              "examine inherited transition organs");
    Function test{};
    s.bind(test, "braid_probe", Image::runnels);
    std::unique_ptr<EvidenceFile> file;
    if (!o.braid_trace.empty()) {
        file = std::make_unique<EvidenceFile>(o.braid_trace);
        file->out << "arm\tcase\ttwin\trig\tstart\tquery\tlength\tterms\tfocus\tworld\tplastic\tarbor\tmuscle\tthroat\t"
                     "goal\timagined\tknown\tplan\tsteps\tfound\tend\tfood\tstrands\tisolated";
        for (Word i = 0; i < terms; ++i)
            file->out << "\tverb" << i;
        for (Word i = 0; i < terms; ++i)
            file->out << "\tmatrix" << i;
        for (Word i = 0; i < 4; ++i)
            file->out << "\thand" << i;
        file->out << "\tconvention\treading\tgrammar\n";
    }
    const char *names[]{"grounded",  "single_examples", "screen",     "unhandled",  "frozen", "wrist_turned",
                        "no_anchor", "misnamed",        "cut_strand", "new_single", "meaning"};
    std::vector<braid::Trial> results(count);
    Event a{}, bclock{};
    s.d.check(s.d.event_new(&a, 0), "create float study clock");
    s.d.check(s.d.event_new(&bclock, 0), "create finish clock");
    float time = 0;
    for (Word arm = 0; arm < braid::arms; ++arm) {
        s.d.check(s.d.record(a, stream), "begin float twins");
        launch(test, (count + 63) / 64, 64, stock_dev.ptr, np, count, salt, rep, terms, len, coupled, arm, grammar,
               rec.ptr);
        s.d.check(s.d.record(bclock, stream), "finish float twins");
        synchronize();
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, bclock), "time generic transition tissue");
        time += ms;
        s.d.check(s.d.download(results.data(), rec.ptr, Count(count) * sizeof(results[0])), "read float decisions");
        Count exact = 0, decoded = 0, diverged = 0, both = 0, alternative = 0, acted = 0, kept = 0, cost = 0, food = 0,
              lessons = 0, shown = 0, edits = 0, wrote = 0, tuition = 0, marked = 0, identified = 0, revised = 0;
        for (Word i = 0; i < count; ++i) {
            auto &p = results[i];
            auto &u = p.twin[0], &v = p.twin[1];
            bool distinct = u.wanted != v.wanted;
            diverged += distinct;
            both += distinct && u.correct && v.correct;
            alternative += distinct && u.after != v.after;
            for (Word j = 0; j < 2; ++j) {
                auto &t = p.twin[j];
                exact += t.after == t.wanted;
                decoded += t.known == 15 && t.imagined == t.wanted;
                acted += t.n != 0;
                kept += t.found;
                cost += t.cost;
                food += t.food;
                lessons += t.exposures;
                shown += t.seen;
                edits += t.writes;
                wrote += t.muscle_writes;
                tuition += t.tuition;
                marked += p.convention;
                identified += p.convention == t.orientation;
                revised += t.order_edits;
                if (!file)
                    continue;
                auto &f = file->out;
                f << names[arm] << '\t' << i << '\t' << j << '\t' << p.rig << '\t' << p.start << '\t' << p.utterance
                  << '\t' << p.length << '\t' << terms << '\t' << p.missing << '\t' << p.world;
                for (Word x : {p.germ.plastic, p.germ.arbor, p.germ.muscle, p.germ.throat, t.wanted, t.imagined,
                               t.known, t.plan, t.n, t.found, t.after, t.food, t.station_mask, t.seen})
                    f << '\t' << x;
                for (Word k = 0; k < terms; ++k)
                    f << '\t' << p.teacher[k];
                for (Word k = 0; k < terms; ++k)
                    f << '\t' << t.lexicon[k];
                for (auto x : t.muscles)
                    f << '\t' << x;
                f << '\t' << p.convention << '\t' << t.orientation << '\t' << grammar << '\n';
            }
        }
        std::cout << "braid_trial=" << names[arm] << "  pairs=" << count << "  words=" << terms << "  repeats=" << rep
                  << "  length=" << (arm == braid::one_word ? 1 : len) << "  coupled=" << coupled
                  << "  grammar=" << grammar << "  reversed=" << marked << "  identified=" << identified
                  << "  order_edits=" << revised << "  probes=" << 2 * count << "  exact=" << exact
                  << "  decoded=" << decoded << "  different=" << diverged << "  both=" << both
                  << "  divergent_actions=" << alternative << "  acted=" << acted << "  routed=" << kept
                  << "  cost=" << cost << "  food=" << food << "  offered=" << 16 * count << "  lessons=" << lessons
                  << "  isolated=" << shown << "  tutor_work=" << tuition << "  language_edits=" << edits
                  << "  hand_edits=" << wrote << '\n';
    }
    s.d.event_free(a);
    s.d.event_free(bclock);
    std::cout << "braid_timing=" << time << "  decisions=" << Count(2) * count * braid::arms << '\n';
    if (file)
        file->publish();
}
