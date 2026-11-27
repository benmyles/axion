void mill_rearing(Habitat &h, const Options &o) {
    if (!o.work_seasons && !o.quarry_seasons && !o.shoal_seasons)
        return;
    Word size = o.work_capacity, seed = c.seed, weeks = std::max({o.shoal_seasons, o.quarry_seasons, o.work_seasons}),
         bouts = o.work_days, span = o.work_span, origin = o.work_origin;
    sample(h);
    std::vector<material::Genes> pool;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            pool.push_back(h.workers[i].genes);
    if (pool.empty()) {
        std::cout << "work_descendants=0  recruitment=vacant\n";
        return;
    }
    if (!o.colonists.empty() || !o.founder.empty())
        origin = 2;
    Temporary donors(s.d, pool.size() * sizeof(material::Genes)), adults(s.d, Count(size) * sizeof(material::Brood)),
        young(s.d, Count(size) * sizeof(material::Brood)), budget(s.d, size / 32 * sizeof(material::Season));
    s.d.check(s.d.upload(donors.ptr, pool.data(), pool.size() * sizeof(pool[0])),
              "send mechanical germlines to the nursery");
    Word parent_count = pool.size();
    Function hatch{}, raise{}, release{}, settle{};
    auto opt = o.shoal;
    Temporary field(s.d, o.shoal_seasons ? Count(size) * sizeof(material::ShoalScore) : 0);
    if (o.shoal_seasons)
        s.bind(settle, "flood_shoal_disperse", Image::floodplain);
    s.bind(hatch, "flood_mill_found", Image::floodplain);
    s.bind(raise,
           o.shoal_seasons    ? "flood_shoal_season"
           : o.quarry_seasons ? "flood_quarry_season"
                              : "flood_mill_season",
           Image::floodplain);
    s.bind(release, "flood_mill_release", Image::floodplain);
    launch(hatch, (size + 127) / 128, 128, adults.ptr, donors.ptr, parent_count, size, seed, origin);
    auto start = std::chrono::steady_clock::now();
    std::vector<material::Season> ledgers(size / 32);
    Word foster = o.work_foster, sight = h.minerals.physics.vision;
    Word duration = o.quarry_ticks;
    for (Word g = 0; g < weeks; ++g) {
        if (o.shoal_seasons) {
            launch(raise, (size + 3) / 4, 128, adults.ptr, field.ptr, size, seed, g, opt);
            launch(settle, size / 32, 32, adults.ptr, young.ptr, field.ptr, budget.ptr, size, seed, g, foster);
        } else if (o.quarry_seasons)
            launch(raise, size / 32, 32, adults.ptr, young.ptr, budget.ptr, size, seed, g, bouts, span, foster,
                   duration);
        else
            launch(raise, size / 32, 32, adults.ptr, young.ptr, budget.ptr, size, seed, g, bouts, span, foster, sight);
        std::swap(adults.ptr, young.ptr);
        if ((g + 1) % 32 && g && g + 1 < weeks)
            continue;
        synchronize();
        s.d.check(s.d.download(ledgers.data(), budget.ptr, ledgers.size() * sizeof(ledgers[0])),
                  "check conserved workshop stock");
        std::array<Count, sizeof(material::Season) / sizeof(Word)> totals{};
        for (auto &book : ledgers) {
            if (book.initial + book.brightness != book.residual + book.heat || book.provision != book.eggs * 24)
                throw std::logic_error("workshop light, matter or provisions are inconsistent");
            auto fields = std::bit_cast<std::array<Word, totals.size()>>(book);
            for (size_t j = 0; j < totals.size(); ++j)
                totals[j] += fields[j];
        }
        std::cout << "work_season=" << g + 1 << "  sun=" << totals[1] << "  stock=" << totals[2]
                  << "  heat=" << totals[3] << "  provision=" << totals[4] << "  offspring=" << totals[5]
                  << "  reactions=" << totals[6] << "  mechanical_work=" << totals[7] << "  interventions=" << totals[8]
                  << "  witnessed=" << totals[9] << '\n';
    }
    synchronize();
    std::vector<material::Brood> brood(size);
    s.d.check(s.d.download(brood.data(), adults.ptr, brood.size() * sizeof(brood[0])),
              "collect newborn construction germlines");
    pool.clear();
    std::set<Word> families;
    for (auto &j : brood)
        if (j.live) {
            pool.push_back(j.genes);
            families.insert(j.family);
        }
    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "work_descendants=" << pool.size() << "  families=" << families.size() << "  habitat="
              << (o.shoal_seasons    ? "shoal"
                  : o.quarry_seasons ? "quarry"
                                     : "rack")
              << "  recruitment=" << (foster ? "fostered" : "provisioned") << "  workshop_seconds=" << elapsed
              << "  station_visits_per_s=" << Count(size) * weeks * (o.shoal_seasons ? opt.moves : bouts) / elapsed
              << '\n';
    if (pool.empty()) {
        Function empty{};
        s.bind(empty, "flood_raft_empty", Image::floodplain);
        launch(empty, (n + 127) / 128, 128, world, c);
        synchronize();
        return;
    }
    Temporary dna(s.d, pool.size() * sizeof(pool[0]));
    s.d.check(s.d.upload(dna.ptr, pool.data(), pool.size() * sizeof(pool[0])), "release provisioned builders");
    parent_count = pool.size();
    launch(release, (n + 127) / 128, 128, dna.ptr, parent_count, world, c);
    synchronize();
}
void mill_exam(const Habitat &h, const Options &o) {
    if (!o.work_probes)
        return;
    std::vector<material::Genes> pool;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            pool.push_back(h.workers[i].genes);
    if (pool.empty()) {
        std::cout << "construction_probe=empty  subjects=0\n";
        return;
    }
    Word count = pool.size(), subjects = o.work_subjects, shown = o.work_shown, probes = o.work_probes,
         span = o.work_span, seed = o.work_test_seed;
    Temporary dna(s.d, Count(count) * sizeof(pool[0])), out(s.d, Count(subjects) * sizeof(material::Transfer));
    s.d.check(s.d.upload(dna.ptr, pool.data(), count * sizeof(pool[0])), "sample the builders’ actual genomes");
    Function run{};
    s.bind(run, "flood_mill_exam", Image::floodplain);
    const char *modes[]{"observed",        "shuttered",          "scrambled",    "same_muscle",           "unpracticed",
                        "other_workpiece", "independent_joints", "live_teacher", "live_teacher_shuttered"};
    std::vector<material::Transfer> rows(subjects);
    for (Word j = 0; j < std::size(modes); ++j) {
        launch(run, (subjects + 31) / 32, 32, dna.ptr, count, out.ptr, subjects, seed, shown, probes, span, j);
        synchronize();
        s.d.check(s.d.download(rows.data(), out.ptr, rows.size() * sizeof(rows[0])),
                  "read independent construction transfers");
        std::array<Count, sizeof(material::Transfer) / sizeof(Word)> totals{};
        for (auto &r : rows) {
            auto f = std::bit_cast<std::array<Word, totals.size()>>(r);
            for (size_t k = 0; k < totals.size(); ++k)
                totals[k] += f[k];
        }
        std::cout << "construction_probe=" << modes[j] << "  subjects=" << subjects
                  << "  trials=" << Count(subjects) * probes << "  shown=" << shown << "  span=" << span
                  << "  matching_effect=" << totals[0] << "  working=" << totals[1] << "  reactions=" << totals[2]
                  << "  insulated_reactions=" << totals[3] << "  mentor_working=" << totals[4]
                  << "  performed=" << totals[5] << '\n';
    }
}

void quarry_exam(const Habitat &h, const Options &o) {
    if (!o.quarry_moves)
        return;
    std::vector<material::Genes> pool;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            pool.push_back(h.workers[i].genes);
    if (pool.empty()) {
        std::cout << "quarry_builders=0  trials=0\n";
        return;
    }
    Word samples = o.work_subjects, donors = pool.size(), moves = o.quarry_moves, seed = o.work_test_seed,
         days = o.quarry_ticks, span = o.work_span;
    Temporary dna(s.d, pool.size() * sizeof(pool[0])), readout(s.d, samples * sizeof(material::QuarryReading));
    s.d.check(s.d.upload(dna.ptr, pool.data(), pool.size() * sizeof(pool[0])), "supply independent quarry germlines");
    Function tool{};
    s.bind(tool, "flood_quarry_exam", Image::floodplain);
    launch(tool, (samples + 63) / 64, 64, dna.ptr, donors, readout.ptr, samples, seed, moves, days, span);
    synchronize();
    std::vector<material::QuarryReading> records(samples);
    s.d.check(s.d.download(records.data(), readout.ptr, records.size() * sizeof(records[0])),
              "observe mechanisms after the builders leave");
    std::array<Count, sizeof(material::QuarryReading) / sizeof(Word)> sum{};
    for (const auto &r : records) {
        auto row = std::bit_cast<std::array<Word, sum.size()>>(r);
        for (size_t j = 0; j < row.size(); ++j)
            sum[j] += row[j];
    }
    const char *labels[]{"operating", "new_devices", "extra",     "lost",  "harvest",
                         "untouched", "insulated",   "rotations", "moves", "surface_exchanges",
                         "handled",   "spanning",    "length_sum"};
    static_assert(std::size(labels) == std::tuple_size_v<decltype(sum)>);
    std::cout << "quarry_builders=" << samples << "  attempts=" << moves << "  duration=" << days
              << "  separation=" << span + 1;
    for (size_t j = 0; j < sum.size(); ++j)
        std::cout << "  " << labels[j] << '=' << sum[j];
    std::cout << '\n';
}

void guild(const Habitat &h, const Options &o) {
    if (!o.guild_generations)
        return;
    std::vector<material::Genes> pool;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            pool.push_back(h.workers[i].genes);
    if (pool.empty()) {
        std::cout << "guild_chain=empty  subjects=0\n";
        return;
    }
    Word count = pool.size(), subjects = o.work_subjects, generations = o.guild_generations, examples = o.work_shown,
         visits = o.guild_practice, seed = o.work_test_seed;
    Temporary dna(s.d, pool.size() * sizeof(pool[0])),
        answers(s.d, Count(subjects) * generations * sizeof(material::Tradition));
    s.d.check(s.d.upload(dna.ptr, pool.data(), pool.size() * sizeof(pool[0])),
              "introduce actual builder germs into transmission chains");
    Function chain{};
    s.bind(chain, "flood_guild", Image::floodplain);
    for (Word mode = 0; mode < 3; ++mode) {
        launch(chain, (subjects + 31) / 32, 32, dna.ptr, count, answers.ptr, subjects, seed, generations, examples,
               visits, mode);
        synchronize();
        std::vector<material::Tradition> cohorts(Count(subjects) * generations);
        s.d.check(s.d.download(cohorts.data(), answers.ptr, cohorts.size() * sizeof(cohorts[0])),
                  "observe working devices from successive fresh generations");
        for (Word g = 0; g < generations; ++g) {
            Count own = 0, other = 0, both = 0, skilled = 0, signals = 0;
            for (Word i = 0; i < subjects; ++i) {
                auto r = cohorts[Count(g) * subjects + i];
                own += r.acquired;
                other += r.ancestral;
                both += r.joint;
                skilled += r.inducted;
                signals += r.signals;
            }
            std::cout << "guild_chain="
                      << (mode == 0   ? "observed"
                          : mode == 1 ? "shuttered"
                                      : "scrambled")
                      << "  generation=" << g + 1 << "  subjects=" << subjects
                      << "  paired_tests=" << Count(subjects) * 16 << "  personal=" << own
                      << "  inherited_skill=" << other << "  both=" << both << "  skilled=" << skilled
                      << "  witnessed=" << signals << '\n';
        }
    }
}
