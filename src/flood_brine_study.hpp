void raise_brine(Habitat &h, const Options &o) {
    if (!o.brine_seasons)
        return;
    sample(h);
    std::vector<brine::Genes> pool;
    if (o.brine_origin)
        for (Word i = 0; i < n; ++i)
            if (h.bodies[i].energy)
                pool.push_back(h.saline_animals[i].genes);
    if (o.brine_origin && pool.empty())
        throw std::invalid_argument("no living material learner in the brood source");
    Word count = o.work_capacity, donors = pool.size(), seed = c.seed, species = h.solution.species;
    Temporary germ(s.d, pool.size() * sizeof(brine::Genes)), parents(s.d, Count(count) * sizeof(brine::Larva)),
        young(s.d, Count(count) * sizeof(brine::Larva)), feeding(s.d, Count(count) * sizeof(brine::Harvest));
    if (donors)
        s.d.check(s.d.upload(germ.ptr, pool.data(), pool.size() * sizeof(pool[0])), "introduce actual brine germlines");
    Function born{}, feed{}, eggs{};
    s.bind(born, "brine_birth", Image::floodplain);
    s.bind(feed, "brine_feed", Image::floodplain);
    s.bind(eggs, "brine_hatch", Image::floodplain);
    Word branches = o.brine_arbors;
    launch(born, (count + 127) / 128, 128, parents.ptr, count, seed, germ.ptr, donors, branches);
    auto before = std::chrono::steady_clock::now();
    std::vector<brine::Harvest> rows(count);
    Word tides = o.brine_tides, offered = o.brine_examples, drift = o.brine_drift, shore = o.brine_shore;
    Count visits = 0;
    for (Word season = 0; season < o.brine_seasons; ++season) {
        launch(feed, (count + 127) / 128, 128, parents.ptr, feeding.ptr, count, seed, season, tides, species, offered,
               shore);
        launch(eggs, (count + 127) / 128, 128, parents.ptr, young.ptr, feeding.ptr, count, seed, season, drift,
               branches);
        std::swap(parents.ptr, young.ptr);
        if (season && ((season + 1) % 64) && season + 1 != o.brine_seasons)
            continue;
        synchronize();
        s.d.check(s.d.download(rows.data(), feeding.ptr, Count(count) * sizeof(rows[0])),
                  "audit food, work and embryonic provisions");
        Count live = 0, contacts = 0, scars = 0, meals = 0, eggs_total = 0, body = 0, work = 0;
        for (auto &r : rows) {
            if (r.initial + r.supplied != r.body + r.heat + r.residual || r.eggs * 6 > r.body)
                throw std::logic_error("brine foraging energy imbalance");
            live += r.initial != 0;
            contacts += r.contacts;
            scars += r.requests;
            meals += r.meals;
            eggs_total += r.eggs;
            body += r.body;
            work += r.heat;
        }
        visits = contacts;
        std::cout << "brine_season=" << season + 1 << "  adults=" << live << "  tested=" << contacts
                  << "  sampling=" << scars << "  meals=" << meals << "  closing_reserve=" << body << "  heat=" << work
                  << "  affordable_eggs=" << eggs_total << "  foster=" << o.brine_drift << '\n';
    }
    synchronize();
    std::vector<brine::Larva> brood(count);
    s.d.check(s.d.download(brood.data(), parents.ptr, Count(count) * sizeof(brood[0])), "collect viable descendants");
    pool.clear();
    std::set<Word> families;
    for (const auto &child : brood)
        if (child.live) {
            pool.push_back(child.genes);
            families.insert(child.family);
        }
    if (pool.empty())
        throw std::runtime_error("the brine brood went extinct");
    for (Word i = 0; i < n; ++i)
        h.saline_animals[i] = {pool[mix(seed + i * 3307) % pool.size()], {}};
    s.d.check(s.d.upload(brine_tissue.ptr, h.saline_animals.data(), Count(n) * sizeof(brine::Animal)),
              "settle germlines with empty synapses");
    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - before).count();
    std::cout << "brine_brood=" << pool.size() << "  families=" << families.size() << "  seasons=" << o.brine_seasons
              << "  seconds=" << elapsed << "  cohort_slots_per_s=" << Count(count) * o.brine_seasons / elapsed
              << "  closing_contacts=" << visits << '\n';
}
void brine_assay(const Habitat &h, const Options &o) {
    if (!o.brine_trials)
        return;
    std::vector<brine::Animal> population;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            population.push_back(h.saline_animals[i]);
    if (population.empty()) {
        std::cout << "brine_transfer=vacant  residents=0\n";
        return;
    }
    Word count = o.work_subjects, donors = population.size(), seed = o.work_test_seed;
    Temporary animals(s.d, Count(donors) * sizeof(brine::Animal)), result(s.d, Count(count) * sizeof(brine::Reading));
    s.d.check(s.d.upload(animals.ptr, population.data(), Count(donors) * sizeof(population[0])),
              "retain the residents' saline tissue");
    Function examine{};
    s.bind(examine, "brine_assess", Image::floodplain);
    std::vector<brine::Reading> records(count);
    const char *names[]{"responsive",  "frozen",         "yoked",       "opposite",
                        "rearranged",  "reversal",       "self_sample", "random_sample",
                        "prospective", "without_replay", "decoy_goal",  "replay_fixed_budget"};
    Word species = o.brine_trial_species ? o.brine_trial_species : h.solution.species;
    Word trials = o.brine_trials, offered = o.brine_examples;
    for (Word mode = 0; mode < std::size(names); ++mode) {
        auto start = std::chrono::steady_clock::now();
        launch(examine, (count + 127) / 128, 128, animals.ptr, donors, result.ptr, count, seed, trials, species,
               offered, mode);
        synchronize();
        s.d.check(s.d.download(records.data(), result.ptr, Count(count) * sizeof(records[0])),
                  "observe paid choices in changed solvents");
        std::array<Count, sizeof(brine::Reading) / sizeof(Word)> sum{};
        for (auto &r : records) {
            auto fields = std::bit_cast<std::array<Word, sum.size()>>(r);
            for (size_t j = 0; j < fields.size(); ++j)
                sum[j] += fields[j];
        }
        if (sum[3] != sum[4] + sum[5] || sum[1] != sum[0] + sum[3] ||
            ((mode == 1 || mode == 2) && 2 * sum[3] != sum[0]))
            throw std::logic_error("incorrectly paired material choices or physical work");
        double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "brine_transfer=" << names[mode] << "  residents=" << donors << "  pupils=" << count
                  << "  choices=" << sum[0] << "  paid=" << sum[1] << "  food=" << sum[2] << "  correct=" << sum[3]
                  << "  first=" << sum[4] << "  second=" << sum[5] << "  changed=" << sum[6] << "  observed=" << sum[7]
                  << "  ties=" << sum[8] << "  distinguishable=" << sum[9] << "  deducible=" << sum[10]
                  << "  solved_deducible=" << sum[11] << "  examples=" << o.brine_examples << "  solutes=" << species
                  << "  seconds=" << elapsed << '\n';
    }
}

void brine_benchmark(const Habitat &h, Word calls) {
    if (!calls)
        return;
    std::vector<brine::Animal> living;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            living.push_back(h.saline_animals[i]);
    if (living.empty()) {
        std::cout << "brine_deployment=0  calls=0\n";
        return;
    }
    Word count = living.size();
    Temporary adults(s.d, Count(count) * sizeof(brine::Animal)), ranking(s.d, 4 * Count(count)),
        firing(s.d, 2 * Count(count)), a(s.d, Count(calls) * 4), b(s.d, Count(calls) * 4);
    s.d.check(s.d.upload(adults.ptr, living.data(), Count(count) * sizeof(living[0])),
              "deploy the living salt circuits");
    Function fold{}, dynamic{}, settled{};
    s.bind(fold, "brine_compile", Image::floodplain);
    s.bind(dynamic, "brine_mutable", Image::floodplain);
    s.bind(settled, "brine_settled", Image::floodplain);
    launch(fold, (count + 127) / 128, 128, adults.ptr, count, ranking.ptr, firing.ptr);
    synchronize();
    std::vector<Word> rank(count);
    std::vector<Half> fires(count);
    s.d.check(s.d.download(rank.data(), ranking.ptr, 4 * Count(count)), "verify the competition order");
    s.d.check(s.d.download(fires.data(), firing.ptr, 2 * Count(count)), "verify the emission threshold");
    for (Word i = 0; i < count; ++i) {
        auto &g = living[i].genes;
        auto m = living[i].memory;
        Word votes[16]{}, total = 0;
        for (Word bit = 0; bit < 32; ++bit)
            if (g.active & (1u << bit)) {
                Word state = ((m.low >> bit) & 1) + 2 * ((m.high >> bit) & 1), w = (g.readout >> (2 * state)) & 3;
                total += w;
                for (Word q = 0; q < 16; ++q)
                    if (g.receptive[q] & (1u << bit))
                        votes[q] += w;
            }
        for (Word q = 0; q < 16; ++q) {
            Word lower = 0;
            for (Word j = q & 12; j < (q & 12) + 4; ++j)
                lower += votes[j] < votes[q];
            for (Word j = q & 12; j < (q & 12) + 4; ++j) {
                Word that = (rank[i] >> (2 * j)) & 3;
                if ((lower > that) != (votes[q] > votes[j]) || (lower == that) != (votes[q] == votes[j]))
                    throw std::logic_error("unequal substrate preference after settling the circuit");
            }
            if (((rank[i] >> (2 * q)) & 3) != lower ||
                bool((fires[i] >> q) & 1) != (total && 8 * votes[q] > (2 + ((g.readout >> 8) & 3)) * total))
                throw std::logic_error("compiling a salt circuit changed a material decision");
        }
    }
    cuda::Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "open material timer");
    s.d.check(s.d.event_new(&end, 0), "open completion timer");
    std::array<float, 7> timings[2]{};
    for (Word t = 0; t < 8; ++t) {
        Word seed = mix(577 + t);
        for (Word phase = 0; phase < 2; ++phase) {
            Word mode = (t + phase) & 1;
            auto &out = mode ? b : a;
            s.d.check(s.d.record(start, stream), "time receptive readouts");
            launch(mode ? settled : dynamic, (calls + 127) / 128, 128, adults.ptr, count, ranking.ptr, firing.ptr,
                   calls, seed, out.ptr);
            s.d.check(s.d.record(end, stream), "close material timer");
            s.d.check(s.d.wait(end), "finish compiled choices");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, start, end), "measure material decisions");
            if (t)
                timings[mode][t - 1] = ms;
        }
    }
    s.d.event_free(start);
    s.d.event_free(end);
    std::vector<Word> first(calls), second(calls);
    s.d.check(s.d.download(first.data(), a.ptr, 4 * Count(calls)), "read native traces");
    s.d.check(s.d.download(second.data(), b.ptr, 4 * Count(calls)), "read frozen traces");
    if (first != second)
        throw std::logic_error("native and compiled material choices differ");
    for (auto &t : timings)
        std::sort(t.begin(), t.end());
    std::cout << "brine_deployment=" << count << "  calls=" << calls << "  comparisons=" << Count(count) * 80
              << "  differences=0  settled_bytes=6  writable_bytes=" << sizeof(brine::Animal)
              << "  trace_bytes=" << sizeof(brine::Trace) << "  mutable_ms=" << timings[0][3]
              << "  median_ms=" << timings[1][3] << "  Mbrine_choices_per_s=" << calls / (1000 * timings[1][3]) << '\n';
}
