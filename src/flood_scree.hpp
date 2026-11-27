void shoal_exam(const Habitat &h, const Options &o) {
    if (!o.shoal_trials)
        return;
    std::vector<material::Genes> pool;
    std::vector<material::Foresight> memories;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy) {
            pool.push_back(h.workers[i].genes);
            if (o.resident_test)
                memories.push_back(h.forecasts[i]);
        }
    if (pool.empty()) {
        std::cout << "shoal_test=vacant  trials=0\n";
        return;
    }
    Word donors = pool.size(), subjects = o.work_subjects, key = o.work_test_seed;
    auto opt = o.shoal;
    Word resident = o.resident_test;
    Temporary learned(s.d, memories.size() * sizeof(material::Foresight));
    if (resident)
        s.d.check(s.d.upload(learned.ptr, memories.data(), memories.size() * sizeof(memories[0])),
                  "restore resident experience in its original sensory frame");
    Temporary dna(s.d, pool.size() * sizeof(pool[0])), animals(s.d, Count(subjects) * sizeof(material::Construction)),
        scores(s.d, Count(subjects) * sizeof(material::ShoalScore)), photo(s.d, sizeof(material::ShoalTrace));
    s.d.check(s.d.upload(dna.ptr, pool.data(), pool.size() * sizeof(pool[0])), "sample shore germlines");
    Function learn{}, act{};
    s.bind(learn, "flood_shoal_practice", Image::floodplain);
    s.bind(act, "flood_shoal_build", Image::floodplain);
    std::vector<material::ShoalScore> data(subjects);
    const char *conditions[]{"experienced", "no_contact_learning", "no_motor_learning", "scrambled_pulses",
                             "no_spread",   "random_ideas",        "prior_diet",        "retasted",
                             "blank_tissue"};
    bool published = false;
    std::vector<unsigned char> first(Count(subjects) * o.shoal_trials);
    for (Word mode = 0; mode < std::size(conditions); ++mode) {
        Word treatment = mode >= 7 ? mode + 1 : mode;
        auto start = std::chrono::steady_clock::now();
        launch(learn, (subjects + 127) / 128, 128, dna.ptr, learned.ptr, donors, animals.ptr, subjects, key,
               opt.practice, treatment);
        Count built = 0, necessary = 0, reactions = 0, blank = 0, cut = 0, parts = 0, predicted = 0, spent = 0,
              light = 0, stored = 0;
        Count target_only = 0, other_only = 0, both = 0, redirect = 0, anticipates = 0, hits = 0;
        for (Word day = 0; day < o.shoal_trials; ++day) {
            Word requested = ~0u;
            launch(act, (subjects + 3) / 4, 128, animals.ptr, scores.ptr, subjects, key, opt, treatment, day, requested,
                   photo.ptr, resident);
            synchronize();
            s.d.check(s.d.download(data.data(), scores.ptr, data.size() * sizeof(data[0])),
                      "read the exposed shore beds");
            for (Word i = 0; i < subjects; ++i) {
                auto &r = data[i];
                if (r.invalid || r.original || r.sealed || r.reactions + r.stored != r.photons)
                    throw std::logic_error("loose stock or exposure budget failed its physical control");
                auto &opening = first[Count(day) * subjects + i];
                bool wanted = r.reactions != 0, rival = r.alternative != 0;
                if (!mode)
                    opening = wanted && !rival;
                target_only += wanted && !rival;
                other_only += rival && !wanted;
                both += rival && wanted;
                redirect += opening && rival && !wanted;
                anticipates += r.forecast;
                hits += r.forecast && wanted;
                built += r.reactions > 0;
                necessary += r.reactions && r.essential >= 2;
                reactions += r.reactions;
                blank += r.original;
                cut += r.sealed;
                parts += r.parts;
                predicted += r.candidates;
                spent += r.work;
                light += r.photons;
                stored += r.stored;
                if (!mode && !published &&
                    ((o.shoal_sample == ~0u && r.reactions && r.essential >= 2) ||
                     (o.shoal_sample != ~0u && o.shoal_sample == Count(day) * subjects + i)))
                    requested = std::min(requested, i);
            }
            if (requested < subjects) {
                launch(act, (subjects + 3) / 4, 128, animals.ptr, scores.ptr, subjects, key, opt, treatment, day,
                       requested, photo.ptr, resident);
                synchronize();
                material::ShoalTrace frame{};
                s.d.check(s.d.download(&frame, photo.ptr, sizeof(frame)),
                          "photograph original grains and executed motor plans");
                auto &r = data[requested];
                auto &c = frame.coast;
                std::cout << "shoal_specimen=" << requested << "  episode=" << day << "  seed=" << r.seed
                          << "  light=" << c.sun << "  patch_a=" << c.first << "  patch_b=" << c.second
                          << "  diet=" << c.responsive << "  glass=" << c.glass << "  tendons=" << c.tendons
                          << "  odor=" << c.odor << "  occupied=" << c.occupied << "  food=" << r.reactions
                          << "  photons=" << r.photons << "  stored=" << r.stored << "  essential=" << r.essential
                          << "  joints=" << r.joints << '\n';
                auto print = [&](const char *label, const Word *data, Word length) {
                    std::cout << label << "=";
                    for (Word i = 0; i < length; ++i)
                        std::cout << (i ? "," : "") << data[i];
                    std::cout << '\n';
                };
                print("shoal_before", frame.before, 32);
                print("shoal_after", frame.after, 32);
                print("shoal_origins", frame.origin, 32);
                print("shoal_choices", frame.actions, r.trace_size);
                auto synapses = std::bit_cast<std::array<Word, 4>>(frame.memory);
                print("shoal_memory", synapses.data(), 4);
                auto genome = std::bit_cast<std::array<Word, 6>>(frame.genes);
                print("shoal_genes", genome.data(), 6);
                published = true;
            }
        }
        double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "shoal_test=" << conditions[mode] << "  trials=" << Count(subjects) * o.shoal_trials
                  << "  builders=" << built << "  two_essential=" << necessary << "  reactions=" << reactions
                  << "  untouched=" << blank << "  insulated=" << cut << "  moved_parts=" << parts << "  work=" << spent
                  << "  absorbed=" << light << "  retained=" << stored << "  candidates=" << predicted
                  << "  seconds=" << seconds << "  span=" << opt.span << "  moves=" << opt.moves
                  << "  practice=" << opt.practice << "  ideas=" << opt.ideas << "  target_only=" << target_only
                  << "  other_only=" << other_only << "  both=" << both << "  redirected=" << redirect
                  << "  forecasts=" << anticipates << "  confirmed=" << hits << "  native_experience=" << resident
                  << '\n';
    }
}

void shoal_bench(const Habitat &h, Word calls) {
    if (!calls)
        return;
    std::vector<material::Construction> subjects;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            subjects.push_back({h.forecasts[i], h.workers[i].genes});
    if (subjects.empty()) {
        std::cout << "shore_deployment=0  diagrams=0\n";
        return;
    }
    Word population = subjects.size();
    Temporary peers(s.d, subjects.size() * sizeof(subjects[0])),
        frozen(s.d, Count(population) * sizeof(material::SpatialImage)), original(s.d, Count(calls) * sizeof(Word)),
        compiled(s.d, Count(calls) * sizeof(Word));
    s.d.check(s.d.upload(peers.ptr, subjects.data(), subjects.size() * sizeof(subjects[0])),
              "load actual resident prediction cells");
    Function settle{}, judge[2]{};
    s.bind(settle, "flood_scree_settle", Image::floodplain);
    s.bind(judge[0], "flood_scree_mutable", Image::floodplain);
    s.bind(judge[1], "flood_scree_frozen", Image::floodplain);
    launch(settle, (population + 127) / 128, 128, peers.ptr, frozen.ptr, population);
    synchronize();
    std::vector<material::SpatialImage> images(population);
    s.d.check(s.d.download(images.data(), frozen.ptr, images.size() * sizeof(images[0])), "verify settled contacts");
    for (Word i = 0; i < population; ++i) {
        auto &p = subjects[i];
        auto &img = images[i];
        auto emits = [&](Word row, Word at) { return (p.genes.release >> ((row >> (2 * at)) & 3)) & 1u; };
        for (Word j = 0; j < 16; ++j)
            if (emits(p.memory.contacts, j) != ((img.contacts >> j) & 1u))
                throw std::logic_error("compiled terminal cell diverged");
        for (Word j = 0; j < 32; ++j)
            if (emits(p.memory.muscles[j / 16], j & 15) != ((img.motor[j / 8] >> (j & 7)) & 1u))
                throw std::logic_error("compiled muscle cell diverged");
        for (Word j = 0; j < 4; ++j)
            if (emits(p.memory.appetite, j) != ((img.taste >> j) & 1u))
                throw std::logic_error("compiled odor cell diverged");
    }
    cuda::Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "create replay timer");
    s.d.check(s.d.event_new(&end, 0), "create replay completion timer");
    std::array<float, 7> times[2]{};
    for (Word repeat = 0; repeat <= 7; ++repeat) {
        Word seed = mix(317 + repeat);
        for (Word j = 0; j < 2; ++j) {
            Word mode = (repeat + j) & 1;
            auto &output = mode ? compiled : original;
            s.d.check(s.d.record(start, stream), "begin replay batch");
            launch(judge[mode], (calls + 3) / 4, 128, peers.ptr, frozen.ptr, population, calls, seed, output.ptr);
            s.d.check(s.d.record(end, stream), "end replay batch");
            s.d.check(s.d.wait(end), "finish replay batch");
            float elapsed = 0;
            s.d.check(s.d.elapsed(&elapsed, start, end), "time replay");
            if (repeat)
                times[mode][repeat - 1] = elapsed;
        }
    }
    s.d.event_free(start);
    s.d.event_free(end);
    std::vector<Word> a(calls), b(calls);
    s.d.check(s.d.download(a.data(), original.ptr, Count(calls) * sizeof(Word)), "read writable replay");
    s.d.check(s.d.download(b.data(), compiled.ptr, Count(calls) * sizeof(Word)), "read compact replay");
    if (a != b)
        throw std::logic_error("compiled spatial replay changed a prediction");
    for (auto &timing : times)
        std::sort(timing.begin(), timing.end());
    std::cout << "shore_deployment=" << population << "  diagrams=" << calls
              << "  receptor_comparisons=" << Count(population) * 52
              << "  mismatches=0  spatial_image_bytes=" << sizeof(material::SpatialImage)
              << "  spatial_bytes=" << sizeof(material::Foresight) << "  mutable_ms=" << times[0][3]
              << "  median_ms=" << times[1][3] << "  Mdiagrams_per_s=" << calls / (times[1][3] * 1000) << '\n';
}
