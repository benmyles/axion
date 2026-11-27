void press_rear(Habitat &h, const Options &o) {
    if (!o.press_seasons)
        return;
    sample(h);
    std::vector<Word> wild;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            wild.push_back(press::germ(h.potters[i]));
    if (wild.empty())
        throw std::invalid_argument("press rearing needs living founders");
    Word size = o.press_population, seed = o.press_seed, paid = o.press_paid, practice = o.press_mixed;
    std::vector<press::Family> families(size);
    for (Word i = 0; i < size; ++i) {
        Word k = mix(seed + i * 3929);
        families[i].gene = o.press_varied ? k & 0x3fffffu : wild[k % wild.size()];
    }
    Temporary youth(s.d, Count(size) * sizeof(families[0])), adults(s.d, Count(size) * sizeof(families[0]));
    s.d.check(s.d.upload(youth.ptr, families.data(), Count(size) * sizeof(families[0])),
              "introduce pressure organ germlines");
    Function life{}, breed{};
    s.bind(life, "press_hatchery", Image::siphons);
    s.bind(breed, "press_offspring", Image::siphons);
    for (Word epoch = 0; epoch < o.press_seasons; ++epoch) {
        launch(life, (size + 127) / 128, 128, youth.ptr, adults.ptr, size, seed, epoch, practice);
        if (epoch == 0 || (epoch + 1) % 64 == 0 || epoch + 1 == o.press_seasons) {
            synchronize();
            s.d.check(s.d.download(families.data(), adults.ptr, Count(size) * sizeof(families[0])),
                      "count provisioned eggs");
            Count eggs = 0;
            Word maximum = 0;
            std::set<Word> distinct;
            for (auto &f : families) {
                eggs += f.eggs;
                maximum = std::max(maximum, f.eggs);
                distinct.insert(f.gene);
            }
            std::cout << "press_season=" << epoch + 1 << "  adults=" << size << "  eggs=" << eggs
                      << "  largest=" << maximum << "  germlines=" << distinct.size() << "  paid=" << paid << '\n';
        }
        launch(breed, (size + 127) / 128, 128, adults.ptr, youth.ptr, size, seed, epoch, paid);
    }
    synchronize();
    s.d.check(s.d.download(families.data(), youth.ptr, Count(size) * sizeof(families[0])),
              "return offspring germlines");
    std::erase_if(families, [](const auto &f) { return f.gene == ~0u; });
    if (families.empty())
        throw std::runtime_error("no hatchlings were provisioned");
    for (Word i = 0; i < n; ++i)
        press::express(h.potters[i], families[mix(seed + i * 53) % families.size()].gene);
    if (paid == 2)
        std::cout << "press_hatchlings=" << families.size() << "  slots=" << size
                  << "  carbon_committed=" << 16 * families.size() << '\n';
    s.d.check(s.d.upload(clayers.ptr, h.potters.data(), Count(n) * sizeof(clay::Mind)),
              "install reared learning alleles");
}
void press_census(const Options &o, Word tick) {
    if (!o.press_survey)
        return;
    Word patches = ((c.side + 15) / 16) * ((c.side + 15) / 16);
    std::vector<Word> histogram(Count(patches) * 256);
    Temporary samples(s.d, Count(n) * sizeof(press::Survey)), frequencies(s.d, histogram.size() * sizeof(Word));
    s.d.check(s.d.zero(frequencies.ptr, 0, histogram.size() * sizeof(Word)), "clear blinded catchment records");
    Function inspect{};
    s.bind(inspect, "press_catchment", Image::floodplain);
    Word seed = o.press_seed;
    launch(inspect, (n + 127) / 128, 128, world, c, seed, tick, samples.ptr, frequencies.ptr);
    synchronize();
    std::vector<press::Survey> results(n);
    s.d.check(s.d.download(results.data(), samples.ptr, Count(n) * sizeof(results[0])), "read paired field choices");
    Count den = 0, choices = 0, alive = 0, seen = 0, tasted = 0, correct[6]{}, mass[6]{}, work[6]{};
    for (const auto &r : results) {
        den += r.useful;
        choices += r.choices;
        alive += r.live;
        seen += r.seen;
        tasted += r.valued;
        for (Word k = 0; k < 6; ++k) {
            work[k] += r.work[k];
            correct[k] += r.correct[k];
            mass[k] += r.mass[k];
        }
    }
    const char *names[]{"own", "random", "other_sight", "other_taste", "optical", "displaced"};
    for (Word k = 0; k < 6; ++k)
        std::cout << "press_census=" << names[k] << "  residents=" << alive << "  eligible_offers=" << choices
                  << "  discriminating=" << den << "  work=" << work[k] << "  opened=" << correct[k]
                  << "  potential_carbon=" << mass[k] << "  visual_tissue=" << seen << "  taste_tissue=" << tasted
                  << '\n';
    s.d.check(s.d.download(histogram.data(), frequencies.ptr, histogram.size() * sizeof(Word)),
              "return optical affordance frequencies");
    for (Word local : {0u, 1u}) {
        auto bins = histogram;
        if (!local)
            for (size_t i = 256; i < bins.size(); ++i)
                bins[i % 256] += bins[i];
        Count twice = 0, total = 0, supported = 0;
        for (Word p = 0; p < (local ? patches : 1u); ++p)
            for (Word fold = 0; fold < 2; ++fold)
                for (Word key = 0; key < 64; ++key) {
                    auto *fit = &bins[(p * 2 + fold) * 128 + key * 2],
                         *held = &bins[(p * 2 + (fold ^ 1)) * 128 + key * 2];
                    auto observations = held[0] + held[1];
                    total += observations;
                    if (fit[0] + fit[1])
                        supported += observations;
                    twice += fit[0] == fit[1] ? observations : 2 * held[fit[1] > fit[0]];
                }
        if (total != den)
            throw std::logic_error("inconsistent held-out field offers");
        std::cout << "press_affordance=" << (local ? "local" : "global") << "  offers=" << total
                  << "  twice_correct=" << twice << "  with_training=" << supported
                  << "  tile_side=" << (local ? 16 : c.side) << '\n';
    }
}
void press_study(const Habitat &h, const Options &o) {
    if (!o.press_test)
        return;
    std::vector<Word> genes;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            genes.push_back(press::germ(h.potters[i]));
    if (genes.empty()) {
        std::cout << "press_study=vacant\n";
        return;
    }
    Word count = genes.size(), tasks = o.press_test, seed = mix(o.press_seed + 0xba790bu);
    Temporary laws(s.d, Count(count) * 4), out(s.d, Count(tasks) * sizeof(press::Record)),
        sample(s.d, sizeof(press::Specimen));
    s.d.check(s.d.upload(laws.ptr, genes.data(), Count(count) * sizeof(Word)), "sample actual pressure germlines");
    Function probe{};
    s.bind(probe, "press_probe", Image::siphons);
    std::vector<press::Record> rows(tasks);
    const char *names[]{"revalued",      "prior",       "own_contact",   "no_sight",    "no_taste",
                        "shifted_pitch", "other_bath",  "erase_sight",   "erase_taste", "smooth_tools",
                        "random",        "stable_bath", "mixed_contacts"};
    for (Word mode = 0; mode < press::trials_count; ++mode) {
        launch(probe, (tasks + 127) / 128, 128, laws.ptr, count, tasks, seed, mode, out.ptr, sample.ptr);
        synchronize();
        s.d.check(s.d.download(rows.data(), out.ptr, Count(tasks) * sizeof(rows[0])), "read blind first choices");
        constexpr size_t fields = sizeof(press::Record) / sizeof(Word);
        std::array<Count, fields> t{};
        for (const auto &r : rows) {
            if (r.food + r.residual != r.stocked || r.heat != r.trials || r.edible > r.trials)
                throw std::logic_error("unbalanced press reservoir");
            auto a = std::bit_cast<std::array<Word, fields>>(r);
            for (size_t j = 0; j < fields; ++j)
                t[j] += a[j];
        }
        std::cout << "press_transfer=" << names[mode] << "  preparations=" << tasks << "  offered=" << t[0]
                  << "  productive=" << t[1] << "  prior_tool=" << t[2] << "  recovered=" << t[3] << "  heat=" << t[4]
                  << "  casts=" << t[5] << "  recast=" << t[6] << "  dry_contacts=" << t[7] << "  blind_tastes=" << t[8]
                  << "  sensory_edits=" << t[9] << "  taste_edits=" << t[10] << "  ties=" << t[11]
                  << "  stock=" << t[12] << "  residual=" << t[13] << '\n';
        if (!mode) {
            press::Specimen d{};
            s.d.check(s.d.download(&d, sample.ptr, sizeof(d)), "recover first physical press trial");
            std::cout << "press_trace=" << mix(seed) << "  germ=" << d.gene << "  forecast=" << d.surface
                      << "  value=" << d.appetite << "  before=" << d.prior_appetite << "  old=" << d.baths
                      << "  next=" << d.shifted << "  pitches=" << d.pitches << "  decisions=" << d.decisions
                      << "  outcomes=" << d.outcomes << "  bare=" << d.raw_tips << '\n';
            for (Word j = 0; j < 4; ++j)
                std::cout << "press_specimen=" << j << "  glaze=" << d.skins[j] << "  face=" << d.faces[j]
                          << "  impression=" << d.casts[j] << '\n';
        }
    }
}
void press_benchmark(const Habitat &h, const Options &o) {
    Word calls = o.press_bench;
    if (!calls)
        return;
    std::vector<clay::Mind> residents;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            residents.push_back(h.potters[i]);
    if (residents.empty()) {
        std::cout << "press_deploy=vacant\n";
        return;
    }
    Word number = residents.size(), blocks = (calls + 127) / 128;
    Temporary original(s.d, Count(number) * sizeof(clay::Mind)), compact(s.d, Count(number) * sizeof(press::Frozen)),
        active(s.d, Count(calls) * 4), frozen(s.d, Count(calls) * 4), errors(s.d, 4);
    s.d.check(s.d.upload(original.ptr, residents.data(), Count(number) * sizeof(clay::Mind)),
              "prepare live choice circuits");
    s.d.check(s.d.zero(errors.ptr, 0, 4), "clear press comparison");
    Function encode{}, normal{}, fast{}, compare{};
    s.bind(encode, "press_images", Image::siphons);
    s.bind(normal, "press_writable", Image::siphons);
    s.bind(fast, "press_ranked", Image::siphons);
    s.bind(compare, "press_differences", Image::siphons);
    launch(encode, (number + 127) / 128, 128, original.ptr, compact.ptr, number);
    cuda::Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "start press clock");
    s.d.check(s.d.event_new(&end, 0), "end press clock");
    std::array<float, 7> elapsed[2]{};
    for (Word sample = 0; sample < 8; ++sample) {
        Word seed = mix(o.press_seed + sample * 3729);
        for (Word order = 0; order < 2; ++order) {
            Word index = (sample + order) & 1;
            auto &in = index ? compact : original;
            auto &out = index ? frozen : active;
            s.d.check(s.d.record(start, stream), "start paired choices");
            launch(index ? fast : normal, blocks, 128, in.ptr, number, calls, seed, out.ptr);
            s.d.check(s.d.record(end, stream), "end paired choices");
            s.d.check(s.d.wait(end), "synchronize paired choices");
            if (sample)
                s.d.check(s.d.elapsed(&elapsed[index][sample - 1], start, end), "measure paired choices");
        }
        launch(compare, blocks, 128, active.ptr, frozen.ptr, errors.ptr, calls);
    }
    synchronize();
    s.d.event_free(start);
    s.d.event_free(end);
    Word differences = 0;
    s.d.check(s.d.download(&differences, errors.ptr, 4), "check exact binary choices");
    if (differences)
        throw std::logic_error("compiled press choices differ");
    std::ranges::sort(elapsed[0]);
    std::ranges::sort(elapsed[1]);
    std::cout << "press_deploy=" << number << "  per_batch=" << Count(calls) * 8 << "  verified=" << Count(calls) * 64
              << "  differences=" << differences << "  bytes=" << sizeof(press::Frozen)
              << "  writable_ms=" << elapsed[0][3] << "  packed_ms=" << elapsed[1][3]
              << "  Mdecisions_per_s=" << 8. * calls / (1000 * elapsed[1][3]) << '\n';
}
void press_scholar(const Habitat &h, const Options &o) {
    if (o.press_scholar.empty())
        return;
    std::vector<Word> ids;
    std::vector<clay::Mind> minds;
    for (Word i = 0; i < n; ++i) {
        auto &b = h.bodies[i];
        if (b.energy > 24 && b.body.eyes && b.body.limbs && b.body.pores) {
            ids.push_back(i);
            minds.push_back(h.potters[i]);
        }
    }
    if (ids.empty())
        throw std::invalid_argument("no living pupil can handle or see these tools");
    Word count = ids.size(), seed = mix(o.press_seed + 319297);
    Temporary pupils(s.d, Count(count) * sizeof(minds[0])), score(s.d, Count(count) * 4);
    s.d.check(s.d.upload(pupils.ptr, minds.data(), Count(count) * sizeof(minds[0])), "present eligible living pupils");
    Function rank{};
    s.bind(rank, "press_scholars", Image::siphons);
    launch(rank, (count + 127) / 128, 128, pupils.ptr, count, seed, score.ptr);
    synchronize();
    std::vector<Word> points(count);
    s.d.check(s.d.download(points.data(), score.ptr, Count(count) * 4), "read separate pupil screens");
    Word at = std::max_element(points.begin(), points.end()) - points.begin();
    auto pupil = extract_individual(h, ids[at], false);
    storage::write_checked(o.press_scholar, pupil);
    std::cout << "press_scholar=" << ids[at] << "  candidates=" << count << "  correct=" << points[at]
              << "  possible=128  seed=" << seed << '\n';
}
