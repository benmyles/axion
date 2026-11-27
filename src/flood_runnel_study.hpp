void cut_reeds(const Options &o) {
    Word width = std::min(8u, c.side), regions = n / (width * width), salt = o.reed_seed, arm = o.reed_cut;
    Temporary dev(s.d, regions * sizeof(Word));
    Function cut{};
    s.bind(cut, "reed_cut_sites", Image::floodplain);
    launch(cut, (regions + 63) / 64, 64, world, c, salt, arm, width, dev.ptr);
    synchronize();
    std::vector<Word> affected(regions);
    s.d.check(s.d.download(affected.data(), dev.ptr, affected.size() * 4), "count physically matched reed cuts");
    Count pairs = 0, places = 0;
    for (auto a : affected) {
        pairs += a;
        places += a != 0;
    }
    std::cout << "reed_cut="
              << (arm == 1   ? "observed"
                  : arm == 0 ? "matched"
                             : "sham")
              << "  pairs=" << pairs << "  removed=" << (arm < 2 ? pairs : 0) << "  districts=" << places
              << "  district_side=" << width << "  strata=32  seed=" << salt << '\n';
}
void replace_reeds(const Habitat &donor) {
    if (donor.reeding) {
        // The foreign geometry travels. Attribution belongs to the resident population.
        auto shapes = donor.folds;
        for (auto &q : shapes)
            q &= 0x77;
        s.d.check(s.d.upload(folded.ptr, shapes.data(), n), "transfer the donor's physical reed shapes");
    } else
        s.d.check(s.d.zero(folded.ptr, 0, n), "unfold replaced creekbed");
}
void reed_rear(Habitat &h, const Options &o) {
    if (!h.reeding)
        return;
    if (o.reed_strip)
        s.d.check(s.d.zero(folded.ptr, 0, n), "flatten the folds without consuming strands");
    if (o.reed_stock.empty() && !o.reed_rear)
        return;
    Word pop = o.reed_adults, seed = o.reed_seed, paid = o.reed_paid, school = o.reed_school,
         chiral = !!(h.rush.mode & runnel::handed);
    std::vector<runnel::Genes> stock;
    if (!o.reed_stock.empty()) {
        Habitat source;
        source.read(o.reed_stock);
        if (source.reeding)
            for (Word i = 0; i < source.sites(); ++i)
                if (source.bodies[i].energy)
                    stock.push_back(source.cutters[i].genes);
        if (stock.empty())
            throw std::invalid_argument("reed germ donor has no living folding circuits");
    }
    Word nstock = stock.size();
    Temporary families(s.d, Count(pop) * sizeof(runnel::Family)), next(s.d, Count(pop) * sizeof(runnel::Family)),
        genes(s.d, Count(std::max(pop, nstock)) * sizeof(runnel::Genes));
    if (nstock)
        s.d.check(s.d.upload(genes.ptr, stock.data(), Count(nstock) * sizeof(stock[0])), "load reed founders");
    Function found{}, eat{}, breed{}, settle{};
    s.bind(found, "reed_families", Image::runnels);
    s.bind(eat, school == 4 ? "reed_keep_food" : "reed_eat", Image::runnels);
    s.bind(breed, "reed_breed", Image::runnels);
    s.bind(settle, "reed_settle", Image::runnels);
    launch(found, (pop + 127) / 128, 128, families.ptr, pop, seed, genes.ptr, nstock);
    if (o.reed_rear || stock.empty()) {
        std::vector<runnel::Family> readout(pop);
        for (Word g = 0; g < o.reed_rear; ++g) {
            if (school == 4)
                launch(eat, (pop + 63) / 64, 64, families.ptr, pop, seed, g, chiral);
            else
                launch(eat, (pop + 63) / 64, 64, families.ptr, pop, seed, g, school, chiral);
            if (g == 0 || g + 1 == o.reed_rear || !((g + 1) & 63)) {
                synchronize();
                s.d.check(s.d.download(readout.data(), families.ptr, pop * sizeof(readout[0])),
                          "observe reproductive carbon");
                Count yield = 0;
                for (auto &f : readout)
                    yield += f.reserve - 1;
                std::cout << "reed_brood=" << g + 1 << "  juveniles=" << pop << "  source_carbon_delivered=" << yield
                          << "  transfer_nursery=" << o.reed_school << "  selection=" << (paid ? "earned" : "uniform")
                          << '\n';
            }
            launch(breed, (pop + 127) / 128, 128, families.ptr, next.ptr, pop, seed, g, paid);
            std::swap(families.ptr, next.ptr);
        }
        synchronize();
        s.d.check(s.d.download(readout.data(), families.ptr, pop * sizeof(readout[0])),
                  "retain surviving fold germlines");
        stock.clear();
        for (auto &a : readout)
            stock.push_back(a.genes);
        nstock = pop;
        s.d.check(s.d.upload(genes.ptr, stock.data(), Count(nstock) * sizeof(stock[0])),
                  "plant the selected spiking germs");
    }
    launch(settle, (n + 127) / 128, 128, world.wicks.reeds, n, genes.ptr, nstock, seed);
    synchronize();
}
void reed_bench(const std::vector<runnel::Mind> &m, Word calls, Word relation, Word practice, Word seed, Word chiral) {
    if (!calls || m.empty())
        return;
    Word people = std::min(Word(m.size()), 4096u), phase = 2;
    Temporary bodies(s.d, people * sizeof(m[0])), images(s.d, people * sizeof(runnel::Image)), why(s.d, people * 4),
        out(s.d, Count(calls) * 4);
    s.d.check(s.d.upload(bodies.ptr, m.data(), people * sizeof(m[0])), "upload sampled folding circuits");
    if (practice) {
        Function acquire{};
        s.bind(acquire, "reed_prepare", Image::runnels);
        launch(acquire, (people + 127) / 128, 128, bodies.ptr, people, seed, chiral);
        synchronize();
    }
    Function compiler{}, reader{};
    s.bind(compiler, "reed_images", Image::runnels);
    s.bind(reader, "reed_dispatch", Image::runnels);
    Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "create reed clock");
    s.d.check(s.d.event_new(&end, 0), "create reed clock");
    auto time = [&](auto action) {
        s.d.check(s.d.record(start, stream), "start reed clock");
        action();
        s.d.check(s.d.record(end, stream), "stop reed clock");
        synchronize();
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, start, end), "measure reed kernels");
        return ms;
    };
    auto compile = [&] {
        launch(compiler, (people + 3) / 4, 128, bodies.ptr, people, phase, relation, images.ptr, why.ptr);
    };
    compile();
    synchronize();
    auto setup = time(compile);
    std::vector<Word> reference(calls), observed(calls);
    float times[2]{};
    for (Word arm = 0; arm < 2; ++arm) {
        auto read = [&] {
            launch(reader, (calls + 127) / 128, 128, bodies.ptr, images.ptr, people, calls, phase, arm, relation,
                   out.ptr);
        };
        read();
        synchronize();
        times[arm] = time(read);
        s.d.check(s.d.download(arm ? observed.data() : reference.data(), out.ptr, Count(calls) * 4),
                  "compare every chosen motor");
    }
    Count differences = 0;
    for (Word i = 0; i < calls; ++i)
        differences += reference[i] != observed[i];
    s.d.event_free(start);
    s.d.event_free(end);
    std::vector<Word> evidence(people);
    s.d.check(s.d.download(evidence.data(), why.ptr, people * 4), "count indirectly inferred decisions");
    Count projected = 0;
    for (auto v : evidence)
        projected += std::popcount(v);
    std::cout << "reed_image=" << people << "  exposure=" << (practice ? "practised" : "field")
              << "  relational_entries=" << projected << "  decisions=" << calls << "  mismatches=" << differences
              << "  bytes=" << people * sizeof(runnel::Image) << "  compile_ms=" << setup << "  live_ms=" << times[0]
              << "  packed_ms=" << times[1] << '\n';
    if (differences)
        throw std::logic_error("reed dispatch changed a motor");
}
void reed_study(const Habitat &h, const Options &o) {
    if (!h.reeding)
        return;
    std::vector<runnel::Mind> minds;
    std::vector<runnel::Genes> pool;
    Count contacts = 0, peers = 0, personal = 0, seen = 0, folds = 0, one_way = 0;
    Word chiral = !!(h.rush.mode & runnel::handed), stone = runnel::material_id(mix(h.rush.seed + 9421), chiral);
    for (Word i = 0; i < n; ++i) {
        if (h.bodies[i].energy) {
            auto &m = h.cutters[i];
            minds.push_back(m);
            pool.push_back(m.genes);
            contacts += runnel::population(m.cells.touched);
            peers += runnel::population(m.cells.witnessed);
            personal += m.practice & 65535;
            seen += m.practice >> 16;
        }
        for (Word axis = 0; axis < 2; ++axis) {
            Word fold = runnel::fold_at(h.folds[i], axis);
            if (!fold)
                continue;
            ++folds;
            Word side = c.side, j = axis ? (i + side) % (side * side) : i % side + 1 == side ? i + 1 - side : i + 1;
            Word cue = runnel::texture((h.mineral_bed[i].faces >> (2 * axis)) & 3,
                                       (h.mineral_bed[j].faces >> (2 * (axis + 2))) & 3);
            Word mask = runnel::contact(stone, cue, fold - 1);
            one_way += (mask == 1 || mask == 2) && h.mineral_bed[i].faces && h.mineral_bed[j].faces;
        }
    }
    if (pool.empty() && (o.reed_cases || o.reed_keep || o.reed_general || o.reed_calls || !o.reed_scholar.empty()))
        throw std::invalid_argument("the requested folding study needs living residents");
    auto &b = h.rush.book;
    std::cout << "reed_field=" << h.header.tick << "  living=" << pool.size() << "  learned=" << contacts
              << "  peer_contacts=" << peers << "  own_samples=" << personal << "  peer_samples=" << seen
              << "  standing=" << folds << "  directional=" << one_way << "  opportunities=" << b.opportunities
              << "  folds=" << b.folds << "  created=" << b.created << "  retuned=" << b.retuned
              << "  flexes=" << b.probes << "  witnessed=" << b.peered << "  own_writes=" << b.personal_writes
              << "  social_writes=" << b.social_writes << "  reused=" << b.recalled << "  social_uses=" << b.social_use
              << "  predicted=" << b.predicted << "  accurate=" << b.accurate << "  selected=" << b.selected
              << "  work=" << b.work << "  loads=" << b.wet_contacts << "  held=" << b.stopped
              << "  uphill_water=" << b.lifted_water << "  uphill_carbon=" << b.lifted_food
              << "  shaped_water=" << b.thumped_water << "  shaped_carbon=" << b.thumped_carbon
              << "  analogies=" << b.tasted << "  eaten=" << b.swallowed
              << "  device_bytes=" << sizeof(runnel::Mind) + 1 << '\n';
    auto &r = h.reed_audit;
    Count signed_count = 0;
    for (auto m : h.folds)
        signed_count += __builtin_popcount(m & 0x88);
    std::cout << "reed_legacy=" << h.header.tick << "  audited=" << r.observed_makers << "  changed=" << r.altered
              << "  improved=" << r.better << "  spoiled=" << r.worse << "  installed=" << r.marked
              << "  standing=" << signed_count << "  water=" << r.water << "  carbon=" << r.carbon
              << "  uphill_water=" << r.lifted_water << "  uphill_carbon=" << r.lifted_carbon << "  meals=" << r.meals
              << "  eaten=" << r.swallowed << "  relation_changed=" << r.inferred_changes
              << "  relation_improved=" << r.inferred_better << "  relation_spoiled=" << r.inferred_worse << '\n';
    if (h.rush.mode & runnel::families)
        for (Word group = 0; group < 2; ++group) {
            Count people = 0, energy = 0, experience = 0, writing = 0;
            for (Word i = 0; i < n; ++i) {
                auto &body = h.bodies[i];
                if (body.energy && bool(h.wanderers[i].efferent & hollow::lineage_tag) == bool(group)) {
                    ++people;
                    energy += body.energy;
                    experience += h.cutters[i].practice >> 16;
                    writing += runnel::population(h.cutters[i].cells.witnessed);
                }
            }
            std::cout << "reed_descent=" << h.header.tick << "  group=" << group
                      << "  enabled=" << runnel::entitled(h.rush.mode, group) << "  living=" << people
                      << "  reserve=" << energy << "  peer_samples=" << experience << "  peer_contacts=" << writing
                      << '\n';
        }
    if (o.fold_census) {
        Word bins = std::min(c.side, 32u), scale = c.side / bins;
        const char hex[] = "0123456789abcdef";
        for (Word y = 0; y < bins; ++y) {
            std::string row;
            for (Word x = 0; x < bins; ++x) {
                Word gates = 0, eyes = 0;
                for (Word j = 0; j < scale; ++j)
                    for (Word i = 0; i < scale; ++i) {
                        Word pos = (y * scale + j) * c.side + x * scale + i, id = h.slots[pos];
                        gates += !!runnel::fold_at(h.folds[pos], 0) + !!runnel::fold_at(h.folds[pos], 1);
                        eyes += bool(h.bodies[id].energy && runnel::population(h.cutters[id].cells.witnessed));
                    }
                for (Word v : {gates, eyes}) {
                    row += hex[(v >> 8) & 15];
                    row += hex[(v >> 4) & 15];
                    row += hex[v & 15];
                }
            }
            std::cout << "reed_ground=" << y << "  bins=" << bins << "  counts=" << row << '\n';
        }
    }
    if (o.reed_rig) {
        Function test{};
        s.bind(test, "reed_rig", Image::runnels);
        Temporary dev(s.d, sizeof(runnel::Gallery));
        Word length = o.reed_rig, ticks = 24 * length + 160;
        const char *names[]{"forward", "quiet", "cut", "unfolded", "reversed"};
        for (Word arm = 0; arm < 5; ++arm) {
            launch(test, 1, 1, length, ticks, arm, dev.ptr);
            synchronize();
            runnel::Gallery r{};
            s.d.check(s.d.download(&r, dev.ptr, sizeof(r)), "measure uphill gallery");
            if (r.water_total != 4096 + 5 * (length - 1) || r.carbon_total != 4096 || r.into - r.back != r.food)
                throw std::logic_error("unbalanced reed gallery");
            std::cout << "reed_rig=" << names[arm] << "  span=" << length << "  ticks=" << ticks
                      << "  head_from=5  head_to=17  water=" << r.water << "  carbon=" << r.food
                      << "  water_total=" << r.water_total << "  carbon_total=" << r.carbon_total
                      << "  reached=" << r.highest << '\n';
        }
    }
    if (o.reed_calls)
        for (Word training = 0; training < 2; ++training)
            reed_bench(minds, o.reed_calls, Word(!!(h.rush.mode & runnel::relating)), training, o.reed_seed, chiral);
    if (o.reed_keep && !pool.empty())
        reed_keepers(pool, o, chiral, h.rush.mode & runnel::relating);
    if (o.reed_general && !pool.empty())
        reed_analogies(pool, o, chiral);
    if ((!o.reed_cases && o.reed_scholar.empty()) || pool.empty())
        return;
    Word subjects = pool.size(), cases = o.reed_cases ? o.reed_cases : 1, gen = o.reed_turns, span = o.reed_span,
         seed = o.reed_seed, play = o.reed_play, lectures = o.reed_watch, invited = o.reed_invited;
    Temporary stock(s.d, Count(subjects) * sizeof(pool[0])), out(s.d, Count(cases) * sizeof(runnel::Transmission)),
        shape(s.d, span), heir(s.d, sizeof(runnel::Mind));
    s.d.check(s.d.upload(stock.ptr, pool.data(), Count(subjects) * sizeof(pool[0])),
              "offer actual living germlines to the transmission study");
    Function trial{};
    Word reasoning = !!(h.rush.mode & runnel::relating);
    s.bind(trial, "reed_houses", Image::runnels);
    std::vector<runnel::Transmission> results(cases);
    const char *names[]{"joined",     "alone",      "screen",   "scrambled",   "raw_muscle",
                        "new_adults", "school_a",   "school_b", "no_practice", "unpaired",
                        "middle_cut", "quiet_tide", "unfolded", "backwards",   "one_load"};
    double chain_seconds = 0;
    Word arms = o.reed_cases ? 15u : 1u;
    for (Word arm = 0; arm < arms; ++arm) {
        auto start = std::chrono::steady_clock::now();
        launch(trial, (cases + 63) / 64, 64, stock.ptr, subjects, cases, seed, gen, span, arm, play, lectures, invited,
               chiral, reasoning, out.ptr, shape.ptr, heir.ptr);
        synchronize();
        s.d.check(s.d.download(results.data(), out.ptr, results.size() * sizeof(results[0])),
                  "measure each pupil's independent construction");
        chain_seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        Count water = 0, carbon = 0, delivered = 0, reached = 0, mastered = 0, social = 0, person = 0, seen_n = 0,
              acts = 0, peer_strokes = 0, peer_use = 0, heard = 0, first = 0, better = 0, poor = 0, unique = 0, a = 0,
              best = 0, bb = 0, strict = 0;
        for (auto &r : results) {
            auto &v = r.gallery;
            if (v.water_total != 4096 + 5 * (span - 1) || v.carbon_total != 4096 || v.into - v.back != v.food)
                throw std::logic_error("unbalanced learned gallery");
            water += v.water;
            carbon += v.food;
            delivered += !!v.food;
            reached += v.highest;
            mastered += r.mastered;
            social += r.social_writes;
            person += r.own_writes;
            seen_n += r.social;
            acts += r.own;
            peer_strokes += r.demonstrated;
            peer_use += r.social_use;
            heard += r.changed_hand;
            first += r.first_food;
            better += r.better;
            poor += r.poorer;
            unique += r.both_win;
            a += r.alone_a;
            bb += r.alone_b;
            best += r.alone_best;
            strict += v.positive == span;
        }
        std::cout << "reed_chain=" << names[arm] << "  communities=" << cases << "  generations=" << gen
                  << "  span=" << span << "  play=" << play << "  reached=" << delivered
                  << "  observed_limit=" << (o.reed_watch == ~0u ? 48 : o.reed_watch) << "  invited=" << o.reed_invited
                  << "  carbon=" << carbon << "  water=" << water << "  distance_sum=" << reached
                  << "  correct_folds=" << mastered << "  all_one_way=" << strict << "  own_actions=" << acts
                  << "  peer_actions=" << peer_strokes << "  seen=" << seen_n << "  self_edits=" << person
                  << "  peer_edits=" << social << "  peer_used=" << peer_use << "  remapped=" << heard
                  << "  first_carbon=" << first << "  parent_a=" << a << "  parent_b=" << bb
                  << "  stronger_parent=" << best << "  beat_both=" << better << "  below_best=" << poor
                  << "  unique_delivery=" << unique << '\n';
        if (arm == 0) {
            std::vector<Byte> view(span);
            runnel::Mind mind{};
            s.d.check(s.d.download(view.data(), shape.ptr, span), "see the demonstrated layout");
            s.d.check(s.d.download(&mind, heir.ptr, sizeof(mind)), "retain the apprentice's own synapses");
            std::cout << "reed_example=0  seed=" << seed << "  handed=" << chiral
                      << "  stone=" << runnel::material_id(mix(mix(seed) + 167), chiral) << "  span=" << span
                      << "  carbon=" << results[0].gallery.food << "  water=" << results[0].gallery.water
                      << "  joints=";
            const char code[] = "0123456789abcdef";
            for (auto x : view)
                std::cout << code[x >> 4] << code[x & 15];
            std::cout << '\n';
            if (!o.reed_scholar.empty()) {
                storage::AgentImage child;
                if (o.reed_carrier.empty()) {
                    Word slot = 0;
                    while (slot < n && !h.bodies[slot].energy)
                        ++slot;
                    child = extract_individual(h, slot, false);
                } else
                    child.read(o.reed_carrier);
                if (!child.riverine)
                    throw std::invalid_argument("a reed scholar needs a watershed body");
                child.folds_reeds = true;
                child.reed.mind = mind;
                storage::write_checked(o.reed_scholar, child);
            }
        }
    }
    std::cout << "runnel_timing=communities  trials=" << Count(cases) * arms << "  seconds=" << chain_seconds << '\n';
}

void reed_analogies(const std::vector<runnel::Genes> &pool, const Options &o, Word chiral) {
    Word n = o.reed_general, ng = pool.size(), seed = o.reed_seed, j = o.reed_examples;
    Temporary stock(s.d, Count(ng) * sizeof(pool[0])), answers(s.d, Count(n) * sizeof(runnel::Unseen));
    s.d.check(s.d.upload(stock.ptr, pool.data(), Count(ng) * sizeof(pool[0])),
              "offer native genomes for held-out comparisons");
    Function trial{};
    s.bind(trial, "reed_unseen", Image::runnels);
    std::vector<runnel::Unseen> cases(n);
    std::unique_ptr<EvidenceFile> trace;
    if (!o.reed_trace.empty()) {
        trace = std::make_unique<EvidenceFile>(o.reed_trace);
        trace->out << "arm\tcase\tseed\ttarget\tcorner\tdirection\tframe\tstone_a\tstone_b\tmotor_a\tmotor_b\tpose_"
                      "a\tpose_b\teffect_a\teffect_b\tvia_a\tvia_b\ttransform_a\ttransform_"
                      "b\texact\tboth\twater\tcarbon\ttarget_contacts\n";
    }
    const char *names[]{"related", "local",    "no_left",       "no_corner",        "no_right",
                        "opaque",  "misheard", "target_shown",  "independent_grit", "observed",
                        "private", "misbound", "no_reflection", "varied_sources"};
    double seconds = 0;
    for (Word a = 0; a < 14; ++a) {
        auto start = std::chrono::steady_clock::now();
        launch(trial, (n + 127) / 128, 128, stock.ptr, ng, n, seed, a, j, chiral, answers.ptr);
        synchronize();
        s.d.check(s.d.download(cases.data(), answers.ptr, Count(n) * sizeof(cases[0])),
                  "read physically tested twin decisions");
        seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        Count both = 0, exact = 0, food = 0, water = 0, derived = 0, seen = 0, exposed = 0, opposite = 0, certified = 0,
              calibrated = 0, moved = 0;
        for (Word i = 0; i < n; ++i) {
            auto &v = cases[i];
            both += v.both;
            exact += v.exact;
            food += v.food;
            water += v.water;
            derived += v.inferred;
            seen += v.seen_target;
            exposed += v.observations;
            opposite += v.public_a != v.public_b;
            certified += v.both && v.inferred == 2 && v.proof_a == v.pivot && v.proof_b == v.pivot;
            moved += v.goal_met;
            calibrated += v.both && v.calibrated == 2;
            if (trace)
                trace->out << names[a] << '\t' << i << '\t' << seed << '\t' << v.target << '\t' << v.pivot << '\t'
                           << v.wanted << '\t' << v.signature << '\t' << v.stone_a << '\t' << v.stone_b << '\t'
                           << v.native_a << '\t' << v.native_b << '\t' << v.public_a << '\t' << v.public_b << '\t'
                           << v.effect_a << '\t' << v.effect_b << '\t' << v.proof_a << '\t' << v.proof_b << '\t'
                           << v.rotation_a << '\t' << v.rotation_b << '\t' << v.exact << '\t' << v.both << '\t'
                           << v.water << '\t' << v.food << '\t' << v.seen_target << '\n';
            if (a == 0 && i < 4)
                std::cout << "reed_inference=" << i << "  target=" << v.target << "  corner=" << v.pivot
                          << "  direction=" << v.wanted << "  stone_a=" << v.stone_a << "  stone_b=" << v.stone_b
                          << "  pose_a=" << v.public_a << "  pose_b=" << v.public_b << "  flap_a=" << v.effect_a
                          << "  flap_b=" << v.effect_b << "  derived=" << v.inferred << "  via_a=" << v.proof_a
                          << "  via_b=" << v.proof_b << "  transform_a=" << v.rotation_a
                          << "  transform_b=" << v.rotation_b << "  frame=" << v.signature << '\n';
        }
        std::cout << "reed_analogy=" << names[a] << "  pairs=" << n << "  handed=" << chiral
                  << "  observations=" << exposed << "  target_contacts=" << seen << "  exact=" << exact
                  << "  both=" << both << "  differing_gestures=" << opposite << "  relational=" << derived
                  << "  paired_corner_uses=" << certified << "  verified_corners=" << calibrated
                  << "  pumping=" << moved << "  carbon=" << food << "  water=" << water << '\n';
    }
    if (trace)
        trace->publish();
    std::cout << "runnel_timing=calibrations  trials=" << Count(n) * 28 << "  seconds=" << seconds << '\n';
}

void reed_keepers(const std::vector<runnel::Genes> &pool, const Options &o, Word chiral, Word reason) {
    Word cases = o.reed_keep, ng = pool.size(), seed = o.reed_seed, length = o.reed_span, generations = o.reed_turns,
         turns = o.reed_life;
    Temporary members(s.d, Count(ng) * sizeof(pool[0])), results(s.d, Count(cases) * sizeof(runnel::Keeping));
    s.d.check(s.d.upload(members.ptr, pool.data(), Count(ng) * sizeof(pool[0])),
              "release living germs in unbuilt channels");
    Function run{};
    s.bind(run, "reed_keep", Image::runnels);
    std::vector<runnel::Keeping> logs(cases);
    const char *names[]{"inherited",         "no_witness", "flatten_at_birth", "isolated",  "wrong_flex",
                        "swapped_food_cues", "still",      "unbuilt",          "same_adult"};
    double seconds = 0;
    for (Word arm = 0; arm < 9; ++arm) {
        auto start = std::chrono::steady_clock::now();
        launch(run, (cases + 63) / 64, 64, members.ptr, ng, cases, seed, length, generations, turns, arm, chiral,
               reason, results.ptr);
        synchronize();
        s.d.check(s.d.download(logs.data(), results.ptr, Count(cases) * sizeof(logs[0])),
                  "audit each developing channel");
        seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        Count fed = 0, received = 0, eaten = 0, older = 0, initial = 0, standing = 0, correct = 0, steps = 0,
              walked = 0, folds = 0, probes = 0, seen = 0, heat = 0, water = 0, furthest = 0;
        for (auto &r : logs) {
            if (r.offered + r.eaten != r.stock + r.spent || r.store_food + r.eaten != 4096 ||
                r.store_water != 4096 + 5 * (length - 1))
                throw std::logic_error("unbalanced multi-generation waterworks");
            fed += !!r.food;
            received += r.food;
            eaten += r.eaten;
            older += r.older;
            initial += r.opening;
            standing += r.built;
            correct += r.correct;
            steps += r.steps;
            walked += r.walked;
            folds += r.folded;
            probes += r.probed;
            seen += r.seen;
            heat += r.spent;
            water += r.water;
            furthest += r.reach;
        }
        if (arm == 0 && generations) {
            Word sample = 0;
            for (Word i = 0; i < cases; ++i) {
                if (logs[i].food) {
                    sample = i;
                    break;
                }
                if (logs[i].reach > logs[sample].reach)
                    sample = i;
            }
            Temporary story(s.d, 6 * sizeof(runnel::ReachFrame)), copy(s.d, sizeof(runnel::Keeping));
            Function draw{};
            s.bind(draw, "reed_story", Image::runnels);
            launch(draw, 1, 1, members.ptr, ng, seed, sample, length, generations, turns, chiral, reason, story.ptr,
                   copy.ptr);
            synchronize();
            runnel::Keeping replay{};
            s.d.check(s.d.download(&replay, copy.ptr, sizeof(replay)),
                      "match the illustrated operator's complete trajectory");
            if (std::memcmp(&replay, &logs[sample], sizeof(replay)))
                throw std::logic_error("different illustrated community");
            std::array<runnel::ReachFrame, 6> frames{};
            Word snapshots = std::min(generations, 6u);
            s.d.check(s.d.download(frames.data(), story.ptr, snapshots * sizeof(frames[0])),
                      "read public construction history");
            const char hex[] = "0123456789abcdef";
            for (Word j = 0; j < snapshots; ++j) {
                auto &v = frames[j];
                std::cout << "reed_estate=" << v.generation << "  community=" << sample << "  length=" << length
                          << "  hand=" << v.cursor << "  work=" << v.work << "  carbon=" << v.food
                          << "  water=" << v.water << "  joints=";
                for (Word e = 0; e < length; ++e)
                    std::cout << hex[v.joints[e] >> 4] << hex[v.joints[e] & 15];
                std::cout << '\n';
            }
        }
        std::cout << "reed_stewards=" << names[arm] << "  communities=" << cases << "  generations=" << generations
                  << "  lifetime=" << turns << "  length=" << length << "  productive=" << fed
                  << "  source_delivered=" << received << "  eaten=" << eaten << "  older_outlet_carbon=" << older
                  << "  first_delivery=" << initial << "  standing=" << standing << "  forward=" << correct
                  << "  ticks=" << steps << "  walks=" << walked << "  folds=" << folds << "  tests=" << probes
                  << "  watched=" << seen << "  work=" << heat << "  water=" << water << "  reach_sum=" << furthest
                  << '\n';
    }
    std::cout << "runnel_timing=waterworks  trials=" << Count(cases) * 9 << "  seconds=" << seconds << '\n';
}
