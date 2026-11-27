void introduce_calls(Habitat &h, const Options &o) {
    if (o.phonetic_germ.empty())
        return;
    Habitat stock;
    stock.read(o.phonetic_germ);
    if (!stock.calling)
        throw std::invalid_argument("the donor lacks a native acoustic organ");
    std::vector<siphon::Genes> genes;
    for (Word i = 0; i < stock.sites(); ++i)
        if (stock.bodies[i].energy)
            genes.push_back(stock.callers[i].genes);
    if (genes.empty())
        throw std::invalid_argument("vacant donor habitat");
    sample(h);
    for (Word i = 0; i < n; ++i)
        h.callers[i] = {genes[mix(c.seed + i * 9137) % genes.size()]};
    s.d.check(s.d.upload(reed_animals.ptr, h.callers.data(), Count(n) * sizeof(h.callers[0])),
              "introduce only the inherited acoustic circuit");
    std::cout << "siphon_germlines=" << genes.size() << "  sites=" << n << '\n';
}
void rear_siphons(Habitat &h, const Options &o) {
    if (!o.reed_seasons)
        return;
    sample(h);
    std::vector<siphon::Genes> founders;
    if (o.reed_origin)
        for (Word i = 0; i < n; ++i)
            if (h.bodies[i].energy)
                founders.push_back(h.callers[i].genes);
    if (o.reed_origin && founders.empty())
        throw std::invalid_argument("no living acoustic germline");
    Word size = o.work_capacity, seed = c.seed, donors = founders.size(), windows = h.reeds.windows,
         wire = h.reeds.wire, bands = h.reeds.bands, visits = o.reed_visits, foster = o.reed_foster;
    Temporary germ(s.d, Count(donors) * sizeof(siphon::Genes)), old(s.d, Count(size) * sizeof(siphon::Brood)),
        young(s.d, Count(size) * sizeof(siphon::Brood)), accounts(s.d, Count(size) * sizeof(siphon::Account));
    if (donors)
        s.d.check(s.d.upload(germ.ptr, founders.data(), Count(donors) * sizeof(founders[0])),
                  "retain acoustic germlines");
    Function birth{}, eat{}, hatch{};
    s.bind(birth, "siphon_born", Image::siphons);
    s.bind(eat, "siphon_forage", Image::siphons);
    s.bind(hatch, "siphon_replace", Image::siphons);
    launch(birth, (size + 127) / 128, 128, old.ptr, size, seed, germ.ptr, donors, windows);
    auto start = std::chrono::steady_clock::now();
    std::vector<siphon::Account> rows(size);
    for (Word season = 0; season < o.reed_seasons; ++season) {
        launch(eat, (size + 127) / 128, 128, old.ptr, accounts.ptr, size, seed, season, visits, bands, windows, wire);
        launch(hatch, (size + 127) / 128, 128, old.ptr, young.ptr, accounts.ptr, size, seed, season, foster, windows);
        std::swap(old.ptr, young.ptr);
        if (season && (season + 1) % 64 && season + 1 != o.reed_seasons)
            continue;
        synchronize();
        s.d.check(s.d.download(rows.data(), accounts.ptr, Count(size) * sizeof(rows[0])),
                  "measure paid social foraging");
        Count pairs = 0, calls = 0, grips = 0, food = 0, eggs = 0;
        for (auto &b : rows) {
            if (b.opening + b.stock != b.residual + b.heat + b.closing || b.eggs * 16 > b.closing)
                throw std::logic_error("social foraging energy discrepancy");
            pairs += b.opening != 0;
            calls += b.calls;
            grips += b.strokes;
            food += b.food;
            eggs += b.eggs;
        }
        std::cout << "siphon_season=" << season + 1 << "  pairs=" << pairs << "  calls=" << calls
                  << "  strokes=" << grips << "  consumed=" << food << "  affordable_eggs=" << eggs
                  << "  foster=" << foster << '\n';
    }
    synchronize();
    std::vector<siphon::Brood> brood(size);
    s.d.check(s.d.download(brood.data(), old.ptr, Count(size) * sizeof(brood[0])), "return untrained newborns");
    founders.clear();
    std::set<Word> families;
    for (auto &p : brood)
        if (p.alive) {
            founders.push_back(p.a);
            founders.push_back(p.b);
            families.insert(p.clan);
        }
    if (founders.empty())
        throw std::runtime_error("the acoustic brood went extinct");
    for (Word i = 0; i < n; ++i)
        h.callers[i] = {founders[mix(seed + i * 731) % founders.size()]};
    s.d.check(s.d.upload(reed_animals.ptr, h.callers.data(), Count(n) * sizeof(siphon::Animal)),
              "settle empty acoustic circuits");
    double time = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "siphon_brood=" << founders.size() << "  families=" << families.size() << "  seconds=" << time
              << "  pair_lives_per_s=" << Count(size) * o.reed_seasons / time << '\n';
}
std::vector<siphon::Pair> siphon_neighbors(const Habitat &h) {
    std::vector<siphon::Pair> contacts;
    for (Word i = 0; i < n; ++i) {
        auto u = h.slots[i];
        if (!h.bodies[u].energy)
            continue;
        for (Word d : {1u, 2u}) {
            auto v = h.slots[neighbor(i, d, c.side)];
            if (h.bodies[v].energy)
                contacts.push_back({h.callers[u], h.callers[v]});
        }
    }
    return contacts;
}
void examine_siphons(const Habitat &h, const Options &o) {
    if (!o.reed_trials)
        return;
    auto contacts = siphon_neighbors(h);
    Word donors = contacts.size();
    if (!donors) {
        std::cout << "siphon_transfer=vacant  neighbors=0\n";
        return;
    }
    Word count = o.work_subjects, seed = o.work_test_seed, visits = o.reed_practice, bands = h.reeds.bands,
         trials = o.reed_trials;
    Temporary donors_dev(s.d, Count(donors) * sizeof(siphon::Pair)),
        results(s.d, Count(count) * sizeof(siphon::Reading)), witness(s.d, sizeof(siphon::Witness));
    s.d.check(s.d.upload(donors_dev.ptr, contacts.data(), Count(donors) * sizeof(contacts[0])),
              "hold neighboring residents for paired transfers");
    Function assay{};
    s.bind(assay, "siphon_probe", Image::siphons);
    std::vector<siphon::Reading> rows(count);
    const char *names[]{"lived",       "query_shut",      "answer_shut",    "query_foil",
                        "answer_foil", "words_erased",    "tendons_erased", "parallel_history",
                        "stranger",    "frozen_learning", "newborn"};
    Word windows = h.reeds.windows;
    for (Word mode = 0; mode < std::size(names); ++mode) {
        auto start = std::chrono::steady_clock::now();
        launch(assay, (count + 127) / 128, 128, donors_dev.ptr, donors, results.ptr, witness.ptr, count, seed, visits,
               bands, trials, mode, windows);
        synchronize();
        s.d.check(s.d.download(rows.data(), results.ptr, Count(count) * sizeof(rows[0])),
                  "measure choices under four private source substitutions");
        std::array<Count, sizeof(siphon::Reading) / sizeof(Word)> sum{};
        for (auto &r : rows) {
            auto f = std::bit_cast<std::array<Word, sum.size()>>(r);
            for (size_t j = 0; j < sum.size(); ++j)
                sum[j] += f[j];
        }
        if (mode == 2 && (4 * sum[4] != sum[3] || sum[11]))
            throw std::logic_error("a shuttered listener used private source information");
        double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "siphon_transfer=" << names[mode] << "  neighbors=" << donors << "  pairs=" << count
                  << "  bands=" << bands << "  practice=" << visits << "  trials=" << sum[0] << "  questions=" << sum[1]
                  << "  replies=" << sum[2] << "  strokes=" << sum[3] << "  harvests=" << sum[4]
                  << "  carbon=" << sum[5] << "  work=" << sum[6] << "  attended=" << sum[7] << "  quartets=" << sum[8]
                  << "  four_correct=" << sum[9] << "  changed_voice=" << sum[10] << "  changed_grip=" << sum[11]
                  << "  lifetime_contacts=" << sum[13] << "  seconds=" << t << '\n';
        if (!mode) {
            siphon::Witness w{};
            s.d.check(s.d.download(&w, witness.ptr, sizeof(w)), "read one paired transcript");
            for (Word j = 0; j < bands; ++j)
                std::cout << "siphon_transcript=" << j << "  query=" << w.address[j] << "  attended=" << w.attended[j]
                          << "  voices=" << w.surface[j] << "  grips=" << w.muscle[j] << "  apertures=" << w.seen[j]
                          << "  worked=" << w.worked[j] << '\n';
        }
    }
}
void siphon_foster(const Habitat &h, const Options &o) {
    if (!o.reed_foster_rounds)
        return;
    auto pairs = siphon_neighbors(h);
    if (pairs.empty()) {
        std::cout << "siphon_foster=vacant\n";
        return;
    }
    Word donors = pairs.size(), subjects = o.work_subjects, seed = o.work_test_seed, prep = o.reed_practice,
         visits = o.reed_visit_school, generations = o.reed_foster_rounds, fixed = o.reed_tutors, bands = h.reeds.bands,
         trials = std::max(1u, o.reed_trials), windows = h.reeds.windows;
    Temporary pool(s.d, Count(donors) * sizeof(pairs[0])),
        results(s.d, Count(subjects) * generations * sizeof(siphon::Reading));
    s.d.check(s.d.upload(pool.ptr, pairs.data(), Count(donors) * sizeof(pairs[0])),
              "retain adults for cross-fostering");
    Function spawn{};
    s.bind(spawn, "siphon_adoption", Image::siphons);
    std::vector<siphon::Reading> rows(Count(subjects) * generations);
    const char *arms[]{"addressed", "shuttered", "reordered", "different_community", "grandparents"};
    for (Word mode = 0; mode < std::size(arms); ++mode) {
        launch(spawn, (subjects + 127) / 128, 128, pool.ptr, donors, results.ptr, subjects, seed, prep, visits,
               generations, bands, trials, mode, windows, fixed);
        synchronize();
        s.d.check(s.d.download(rows.data(), results.ptr, Count(rows.size()) * sizeof(rows[0])),
                  "observe the fostered generations");
        for (Word g = 0; g < generations; ++g) {
            Count total = 0, worked = 0, history = 0, modified = 0, quartets = 0, complete = 0;
            for (Word i = 0; i < subjects; ++i) {
                auto &q = rows[Count(g) * subjects + i];
                total += q.trials;
                worked += q.harvests;
                history += q.visits;
                modified += q.learning;
                quartets += q.quartets;
                complete += q.all_four;
            }
            std::cout << "siphon_foster=" << arms[mode] << "  cohort=" << g + 1 << "  trials=" << total
                      << "  harvests=" << worked << "  quartets=" << quartets << "  four_correct=" << complete
                      << "  visible_contacts=" << history << "  synaptic_edits=" << modified
                      << "  opportunities=" << visits << "  parents_practiced=" << prep << '\n';
        }
    }
}
void siphon_census(const Habitat &h, const Options &o) {
    if (!o.reed_census)
        return;
    auto pool = siphon_neighbors(h);
    if (pool.empty()) {
        std::cout << "siphon_census=vacant\n";
        return;
    }
    std::vector<std::array<Word, 2>> slot;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[h.slots[i]].energy)
            for (Word dir : {1u, 2u}) {
                Word other = h.slots[neighbor(i, dir, c.side)];
                if (h.bodies[other].energy)
                    slot.push_back({h.slots[i], other});
            }
    Word count = pool.size(), bands = h.reeds.bands, rounds = o.reed_census, seed = o.work_test_seed, selected = 0;
    Temporary raw(s.d, Count(count) * sizeof(pool[0])), read(s.d, Count(count) * sizeof(siphon::PairScore));
    s.d.check(s.d.upload(raw.ptr, pool.data(), Count(count) * sizeof(pool[0])),
              "retain the full adjacent acoustic census");
    Function eval{};
    s.bind(eval, "siphon_survey", Image::siphons);
    std::vector<siphon::PairScore> rows(count);
    for (Word j = 0; j < 2; ++j) {
        Word shift = j && count > 1 ? 1 + mix(seed + 311) % (count - 1) : 0;
        launch(eval, (count + 127) / 128, 128, raw.ptr, read.ptr, count, bands, rounds, seed, shift);
        synchronize();
        s.d.check(s.d.download(rows.data(), read.ptr, Count(count) * sizeof(rows[0])),
                  "audit native versus redistributed social pairings");
        Count test = 0, opened = 0, closed = 0, questions = 0, expert = 0, quartets = 0, four = 0, both = 0;
        for (Word i = 0; i < count; ++i) {
            const auto &p = rows[i];
            test += p.attempts;
            opened += p.worked;
            closed += p.closed;
            questions += p.no_question;
            expert += p.worked * 4 >= 3 * p.attempts;
            both += p.worked > p.no_question && p.no_question > p.closed;
            quartets += p.quartets;
            four += p.all_four;
            if (!j && p.worked > rows[selected].worked)
                selected = i;
        }
        std::cout << "siphon_census=" << (j ? "redistributed" : "neighbors") << "  pairs=" << count
                  << "  trials=" << test << "  harvests=" << opened << "  answer_shut=" << closed
                  << "  query_shut=" << questions << "  quartets=" << quartets << "  four_correct=" << four
                  << "  above_three_quarters=" << expert << "  both_turns=" << both << '\n';
        if (!j)
            std::cout << "siphon_pair=" << slot[selected][0] << "  partner=" << slot[selected][1]
                      << "  validation_harvests=" << rows[selected].worked << "  trials=" << rows[selected].attempts
                      << "  seed=" << seed << '\n';
    }
}
void siphon_benchmark(const Habitat &h, Word calls) {
    if (!calls)
        return;
    std::vector<siphon::Animal> adults;
    for (Word j = 0; j < n; ++j)
        if (h.bodies[j].energy)
            adults.push_back(h.callers[j]);
    if (adults.empty()) {
        std::cout << "siphon_deployment=vacant\n";
        return;
    }
    Word size = adults.size();
    Temporary living(s.d, Count(size) * sizeof(adults[0])), frozen(s.d, Count(size) * sizeof(siphon::Pocket)),
        first(s.d, Count(calls) * sizeof(Count)), second(s.d, Count(calls) * sizeof(Count)),
        mismatch(s.d, sizeof(Word));
    s.d.check(s.d.upload(living.ptr, adults.data(), Count(size) * sizeof(adults[0])),
              "retain acoustically active residents");
    Function pack{}, dynamic{}, compiled{}, compare{};
    s.bind(pack, "siphon_pack", Image::siphons);
    s.bind(dynamic, "siphon_mutable", Image::siphons);
    s.bind(compiled, "siphon_frozen", Image::siphons);
    s.bind(compare, "siphon_mismatches", Image::siphons);
    launch(pack, (size + 127) / 128, 128, living.ptr, frozen.ptr, size);
    synchronize();
    s.d.check(s.d.zero(mismatch.ptr, 0, sizeof(Word)), "clear native decision differences");
    cuda::Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "start native clock");
    s.d.check(s.d.event_new(&end, 0), "stop native clock");
    std::array<float, 7> timings[2]{};
    Word blocks = (calls + 127) / 128;
    for (Word j = 0; j < 8; ++j) {
        Word seed = mix(c.seed + j * 4573);
        for (Word phase = 0; phase < 2; ++phase) {
            Word mode = (j + phase) & 1;
            auto in = mode ? frozen.ptr : living.ptr;
            auto out = mode ? second.ptr : first.ptr;
            s.d.check(s.d.record(start, stream), "start contact timer");
            launch(mode ? compiled : dynamic, blocks, 128, in, size, calls, seed, out);
            s.d.check(s.d.record(end, stream), "close contact timer");
            s.d.check(s.d.wait(end), "wait for native decisions");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, start, end), "time two-resident decisions");
            if (j)
                timings[mode][j - 1] = ms;
        }
        launch(compare, blocks, 128, first.ptr, second.ptr, mismatch.ptr, calls);
    }
    synchronize();
    s.d.event_free(start);
    s.d.event_free(end);
    Word errors = 0;
    s.d.check(s.d.download(&errors, mismatch.ptr, 4), "verify every native outcome");
    if (errors)
        throw std::logic_error("frozen native decisions changed");
    for (auto &r : timings)
        std::sort(r.begin(), r.end());
    std::cout << "siphon_deployment=" << size << "  calls=" << calls << "  compared=" << Count(calls) * 8
              << "  differences=" << errors << "  settled_bytes=" << sizeof(siphon::Pocket)
              << "  writable_bytes=" << sizeof(siphon::Animal) << "  mutable_ms=" << timings[0][3]
              << "  median_ms=" << timings[1][3] << "  Mcontacts_per_s=" << calls / (1000 * timings[1][3]) << '\n';
}
// Replay exactly the same water, soil, dissolved components and deposited grains
// after the animals leave. Only the four orientations in each outlet are changed.
void siphon_legacy(const Habitat &h, Word ticks, const std::string &reference) {
    if (!ticks)
        return;
    if (ticks > ~0u - tick)
        throw std::invalid_argument("pore legacy exceeds the habitat clock");
    Word blocks = (n / 2 + 127) / 128;
    Temporary bed(s.d, Count(n) * sizeof(Silt)), grain(s.d, Count(n) * sizeof(material::Grain)),
        solvent(s.d, world.brine.solutes ? Count(n) * sizeof(brine::Salts) : 0),
        stock(s.d, world.seams.stock ? Count(n) * sizeof(Half) : 0), ports(s.d, n),
        accounts(s.d, Count(blocks) * sizeof(Counters)), rocks(s.d, Count(blocks) * sizeof(material::Ledger)),
        system(s.d, Count(blocks) * sizeof(siphon::Totals));
    auto copy = [&](Temporary &to, DevicePtr from, Count bytes) {
        if (to.ptr)
            s.d.check(s.d.copy_device(to.ptr, from, bytes), "preserve the occupied watershed for the legacy trial");
    };
    auto clear = [&](Temporary &to, Count bytes) {
        s.d.check(s.d.zero(to.ptr, 0, bytes), "clear counterfactual flux");
    };
    std::vector<Silt> original(n), deposit(n);
    std::vector<Byte> orientations(n);
    std::vector<siphon::Totals> ledger(blocks);
    std::vector<Byte> previous;
    if (!reference.empty()) {
        Habitat earlier;
        earlier.read(reference);
        if (!earlier.calling || earlier.sites() != n || earlier.header.configuration.seed != c.seed ||
            earlier.header.tick > tick)
            throw std::invalid_argument(
                "the filter reference needs earlier apertures on the same size and seed of watershed");
        previous = std::move(earlier.apertures);
    }
    auto modes = std::array{"inhabited", "rotated", "redistributed", "zeroed", "sealed", "reference"};
    for (Word arm = 0; arm < modes.size() - previous.empty(); ++arm) {
        copy(bed, land.ptr, Count(n) * sizeof(Silt));
        copy(grain, mineral_bed.ptr, Count(n) * sizeof(material::Grain));
        copy(solvent, salts.ptr, Count(n) * sizeof(brine::Salts));
        copy(stock, polymers.ptr, Count(n) * sizeof(Half));
        clear(accounts, Count(blocks) * sizeof(Counters));
        clear(rocks, Count(blocks) * sizeof(material::Ledger));
        clear(system, Count(blocks) * sizeof(siphon::Totals));
        Word shift = 1 + mix(c.seed ^ tick) % (n - 1);
        Count altered = 0;
        for (Word i = 0; i < n; ++i) {
            Byte old = h.apertures[i], value = old;
            if (arm == 1)
                value = ((old ^ 85u) ^ ((old & 85u) << 1)) & 255;
            if (arm == 2)
                value = h.apertures[(i + shift) % n];
            if (arm == 3)
                value = 0;
            if (arm == 5)
                value = previous[i];
            orientations[i] = value;
            for (Word d = 0; d < 4; ++d)
                altered += arm == 4 || ((old ^ value) >> (d * 2) & 3) != 0;
        }
        s.d.check(s.d.upload(ports.ptr, orientations.data(), n), "set the four outlet apertures");
        World terrain{};
        terrain.land = reinterpret_cast<Silt *>(bed.ptr);
        terrain.account = reinterpret_cast<Counters *>(accounts.ptr);
        terrain.matter.bed = reinterpret_cast<material::Grain *>(grain.ptr);
        terrain.matter.ledger = reinterpret_cast<material::Ledger *>(rocks.ptr);
        terrain.matter.conditions = world.matter.conditions;
        terrain.tide.active = world.tide.active;
        terrain.siphons.bands = h.reeds.bands;
        terrain.siphons.ports = arm == 4 ? nullptr : reinterpret_cast<Byte *>(ports.ptr);
        terrain.siphons.ledger = reinterpret_cast<siphon::Totals *>(system.ptr);
        terrain.brine = world.brine;
        terrain.brine.animals = nullptr;
        terrain.brine.solutes = reinterpret_cast<brine::Salts *>(solvent.ptr);
        terrain.seams = world.seams;
        terrain.seams.account = nullptr;
        terrain.seams.stock = reinterpret_cast<Half *>(stock.ptr);
        for (Word j = 0; j < ticks; ++j)
            launch(water, blocks, 128, terrain, c, clock.ptr, j);
        synchronize();
        s.d.check(s.d.download(deposit.data(), bed.ptr, Count(n) * sizeof(Silt)), "survey the abandoned filter soil");
        s.d.check(s.d.download(ledger.data(), system.ptr, Count(blocks) * sizeof(ledger[0])),
                  "read passive carbon transfer");
        if (!arm)
            original = deposit;
        Count seep = 0, moved = 0, sites = 0, fruit = 0;
        for (auto &r : ledger)
            seep += r.seep;
        for (Word i = 0; i < n; ++i) {
            sites += deposit[i].fruit != original[i].fruit;
            fruit += deposit[i].fruit;
            moved += std::abs(int(deposit[i].fruit) - int(original[i].fruit));
        }
        if (arm == 4 && seep)
            throw std::logic_error("carbon passed a sealed pore");
        std::cout << "siphon_legacy=" << modes[arm] << "  ticks=" << ticks << "  altered_faces=" << altered
                  << "  passive_carbon=" << seep << "  fruit=" << fruit << "  changed_food_sites=" << sites
                  << "  fruit_distance=" << moved << '\n';
    }
}
