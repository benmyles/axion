void clay_germs(Habitat &h, const Options &o) {
    if (o.clay_origin.empty())
        return;
    Habitat donor;
    donor.read(o.clay_origin);
    std::vector<clay::Law> germs;
    if (donor.pottery)
        for (Word id = 0; id < donor.sites(); ++id)
            if (donor.bodies[id].energy)
                germs.push_back(donor.potters[id].genes);
    if (germs.empty())
        throw std::invalid_argument("no living potters in the germline source");
    sample(h);
    for (Word id = 0; id < n; ++id) {
        clay::swap_bank(h.potters[id], h.callers[id], 0);
        h.potters[id] = {germs[mix(c.seed + id * 3331) % germs.size()]};
    }
    s.d.check(s.d.upload(clayers.ptr, h.potters.data(), Count(n) * sizeof(clay::Mind)),
              "introduce inherited clay circuitry");
    s.d.check(s.d.upload(reed_animals.ptr, h.callers.data(), Count(n) * sizeof(siphon::Animal)),
              "return the home projection");
}
struct WorkPair {
    Word position, other, first, second;
};
std::vector<WorkPair> clay_neighbors(const Habitat &h) {
    std::vector<WorkPair> paired;
    for (Word i = 0; i < n; ++i) {
        Word a = h.slots[i];
        if (!h.bodies[a].energy)
            continue;
        for (Word d : {1u, 2u}) {
            Word j = neighbor(i, d, c.side), b = h.slots[j];
            if (h.bodies[b].energy)
                paired.push_back({i, j, a, b});
        }
    }
    return paired;
}
clay::Collection local_artifacts(const Habitat &h, Word site, bool surrounding) {
    clay::Collection kit;
    kit.clock = h.header.tick;
    Word best[2]{};
    Word width = surrounding ? 5 : 1;
    for (Word dy = 0; dy < width; ++dy)
        for (Word dx = 0; dx < width; ++dx) {
            Word at = ((site % c.side + dx - width / 2) & (c.side - 1)) +
                      ((site / c.side + dy - width / 2) & (c.side - 1)) * c.side,
                 id = h.slots[at];
            for (Word held = 0; held < 2; ++held) {
                auto &g = held ? h.workers[id].hand : h.mineral_bed[at];
                if (!g.faces)
                    continue;
                auto &tile = held ? h.cargo[id] : h.ware[at];
                for (Word port = 0; port < 2; ++port) {
                    Word relief = 0, occupied = 0;
                    for (Word cell = 0; cell < 8; ++cell) {
                        auto e = clay::groove(tile, cell);
                        if ((e & 128) && ((e >> 4) & 1) == port) {
                            relief |= 1u << ((e >> 2) & 3);
                            ++occupied;
                        }
                    }
                    Word rank = 8 * std::popcount(relief) + occupied; // The pitches have not been sounded.
                    if (rank > best[port]) {
                        best[port] = rank;
                        kit.objects[port] = tile;
                        kit.glaze[port] = g.substance;
                    }
                }
            }
        }
    return kit;
}
void clay_object(const Habitat &h, const Options &o) {
    if (o.clay_export.empty())
        return;
    Word position = o.clay_site;
    if (position == ~0u) {
        Word best = 0;
        position = 0;
        for (Word i = 0; i < n; ++i) {
            Word s = clay::size(h.ware[i]) + clay::size(h.cargo[h.slots[i]]);
            if (s > best) {
                best = s;
                position = i;
            }
        }
    }
    if (position >= n)
        throw std::invalid_argument("clay site outside the watershed");
    auto kit = local_artifacts(h, position, false);
    clay::save_collection(o.clay_export, kit);
    std::cout << "clay_extraction=" << position << "  inlet_grooves=" << clay::size(kit.objects[0])
              << "  outlet_grooves=" << clay::size(kit.objects[1]) << '\n';
}
void clay_settle(Habitat &h, const Options &o) {
    if (o.clay_settlers.empty())
        return;
    storage::AgentImage adult;
    adult.read(o.clay_settlers);
    if (!adult.calling || !adult.potter)
        throw std::invalid_argument("the acoustic donor needs its own context organ");
    sample(h);
    Word inhabited = 0;
    for (Word i = 0; i < n; ++i) {
        if (!h.bodies[i].energy)
            continue;
        ++inhabited;
        auto &a = h.callers[i];
        a = {adult.calls.animal.genes, adult.calls.animal.address, adult.calls.animal.surface,
             adult.calls.animal.tendon};
        h.potters[i] = adult.ceramic.mind;
    }
    s.d.check(s.d.upload(clayers.ptr, h.potters.data(), Count(n) * sizeof(clay::Mind)),
              "introduce adult context histories");
    s.d.check(s.d.upload(reed_animals.ptr, h.callers.data(), Count(n) * sizeof(siphon::Animal)),
              "introduce adult native histories");
    std::cout << "clay_introduction=" << inhabited << "  tick=" << h.header.tick << "  imported_objects=0\n";
}
void clay_peers(const Habitat &h, const Options &o) {
    if (o.clay_peers.empty())
        return;
    std::vector<siphon::Animal> residents;
    std::vector<Word> ages;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy) {
            residents.push_back(h.callers[i]);
            ages.push_back(h.header.tick - h.bodies[i].born);
        }
    if (residents.empty()) {
        std::cout << "clay_peer=vacant\n";
        return;
    }
    siphon::Animal models[2]{};
    for (Word j = 0; j < 2; ++j) {
        storage::AgentImage a;
        a.read(o.clay_peers + (j ? "-b.mind" : "-a.mind"));
        if (!a.calling)
            throw std::invalid_argument("a reference partner needs native acoustic gates");
        models[j] = a.calls.animal;
    }
    Word count = residents.size(), seed = o.work_test_seed, trials = std::max(1u, o.reed_trials);
    Temporary native(s.d, Count(count) * sizeof(residents[0])), pair(s.d, sizeof(models)),
        book(s.d, Count(count) * sizeof(clay::Generation));
    s.d.check(s.d.upload(native.ptr, residents.data(), Count(count) * sizeof(residents[0])),
              "retain every living downstream animal");
    s.d.check(s.d.upload(pair.ptr, models, sizeof(models)), "hold the two original reference speakers");
    Function audit{};
    s.bind(audit, "clay_migrants", Image::siphons);
    std::vector<clay::Generation> rows(count);
    constexpr const char *arms[]{"lived", "shut", "words_erased", "tendons_erased"};
    for (Word arm = 0; arm < 4; ++arm) {
        launch(audit, (count + 127) / 128, 128, native.ptr, count, pair.ptr, trials, seed, arm, book.ptr);
        synchronize();
        s.d.check(s.d.download(rows.data(), book.ptr, Count(count) * sizeof(rows[0])), "compare reference conventions");
        for (Word layer = 0; layer < 3; ++layer) {
            Count seen = 0, tests = 0, a = 0, b = 0, all_a = 0, all_b = 0, pressed = 0;
            for (Word i = 0; i < count; ++i) {
                if (layer == 1 && ages[i] >= 32)
                    continue;
                if (layer == 2 && ages[i] < 32)
                    continue;
                auto &x = rows[i];
                ++seen;
                tests += x.tests;
                a += x.native;
                b += x.foreign;
                all_a += x.paired;
                all_b += x.formed;
                pressed += x.food;
            }
            if (arm == 1 && (a * 4 != pressed || a != b || all_a || all_b))
                throw std::logic_error("an opaque shutter transmitted a peer identity");
            std::cout << "clay_peer=" << arms[arm] << "  age="
                      << (layer == 0   ? "all"
                          : layer == 1 ? "juvenile"
                                       : "older")
                      << "  residents=" << seen << "  trials=" << tests << "  with_a=" << a << "  with_b=" << b
                      << "  complete_a=" << all_a << "  complete_b=" << all_b << "  strokes=" << pressed << '\n';
        }
    }
}
void clay_classroom(Habitat &h, const Options &o, bool rearing, bool field = false) {
    Word seasons = rearing ? o.clay_seasons : 0, trials = field ? o.clay_dig : rearing ? 0 : o.clay_test;
    if (!rearing && !field && (((o.clay_foster || o.clay_switches) && !o.clay_dig) || !o.clay_specimen.empty()))
        trials = std::max(1u, trials);
    if (!seasons && !trials)
        return;
    if (rearing)
        sample(h);
    auto neighbors = siphon_neighbors(h);
    if (neighbors.empty()) {
        std::cout << "clay_classes=vacant\n";
        return;
    }
    Word donors = neighbors.size(), classes = o.clay_classes, subjects = o.work_subjects, capacity = o.work_capacity;
    Word seed = rearing ? (o.clay_breed_seed ? o.clay_breed_seed : c.seed) : o.work_test_seed, prep = o.clay_prep,
         visits = o.clay_history, taps = o.clay_taps, lessons = o.clay_lessons;
    Word windows = h.reeds.windows, unrelated = o.clay_strangers, selected = o.clay_select;
    Temporary stock(s.d, Count(donors) * sizeof(neighbors[0])), teachers(s.d, Count(classes) * sizeof(clay::Tradition));
    s.d.check(s.d.upload(stock.ptr, neighbors.data(), Count(donors) * sizeof(neighbors[0])),
              "bring native pairs into the workshops");
    std::vector<clay::Tradition> village(classes);
    auto sites = clay_neighbors(h);
    if (field) {
        for (Word i = 0; i < classes; ++i) {
            auto &t = village[i];
            Word k = mix(seed + i * 3917);
            for (Word community = 0; community < 2; ++community) {
                auto &v = sites[mix(k + (community ? 379 : 731)) % donors];
                t.adults[community] = {h.callers[v.first], h.callers[v.second]};
                auto object = local_artifacts(h, v.position, true);
                t.artifacts[community][0] = object.objects[0];
                t.artifacts[community][1] = object.objects[1];
                t.mineral[community] = object.glaze[1];
                t.inlet_color[community] = object.glaze[0];
            }
        }
        s.d.check(s.d.upload(teachers.ptr, village.data(), Count(classes) * sizeof(village[0])),
                  "offer the two local sets of worked grains");
    } else {
        Function prepare{};
        s.bind(prepare, "clay_traditions", Image::siphons);
        launch(prepare, (classes + 127) / 128, 128, stock.ptr, donors, teachers.ptr, classes, seed, prep, visits,
               windows, unrelated);
        synchronize();
        s.d.check(s.d.download(village.data(), teachers.ptr, Count(classes) * sizeof(village[0])),
                  "inventory the worked physical objects");
    }
    Count impressed = 0, stocked = 0, work = 0;
    for (auto &workshop : village)
        for (Word j = 0; j < 2; ++j) {
            if (!field && 192 + workshop.food[j] != workshop.capital[j] + workshop.labor[j])
                throw std::logic_error("adult clay budget did not balance");
            impressed += clay::size(workshop.artifacts[j][0]) + clay::size(workshop.artifacts[j][1]);
            stocked += workshop.food[j];
            work += workshop.labor[j];
        }
    std::cout << "clay_classes=" << classes << "  origin=" << (field ? "watershed" : "workshop")
              << "  acquired_grooves=" << impressed << "  clay_pieces=" << 4 * classes << "  teacher_carbon=" << stocked
              << "  teacher_work=" << work << "  shared_practice=" << prep << "  contacts=" << visits
              << "  unrelated=" << unrelated << "  seed=" << seed << '\n';
    if (rearing) {
        Temporary old(s.d, Count(capacity) * sizeof(clay::Brood)), young(s.d, Count(capacity) * sizeof(clay::Brood)),
            books(s.d, Count(capacity) * sizeof(clay::Account));
        Function hatch{};
        s.bind(hatch, "clay_breed", Image::siphons);
        auto start = std::chrono::steady_clock::now();
        std::vector<clay::Account> accounts(capacity);
        for (Word epoch = 0; epoch < seasons; ++epoch) {
            launch(hatch, (capacity + 127) / 128, 128, teachers.ptr, classes, old.ptr, young.ptr, capacity, seed, epoch,
                   lessons, taps, selected, books.ptr);
            std::swap(old.ptr, young.ptr);
            if (epoch && (epoch + 1) % 64 && epoch + 1 != seasons)
                continue;
            synchronize();
            s.d.check(s.d.download(accounts.data(), books.ptr, Count(capacity) * sizeof(accounts[0])),
                      "read funded hatchlings and spent work");
            Count food = 0, heat = 0, eggs = 0, lives = 0, queries = 0;
            for (auto &life : accounts) {
                if (life.opening + life.stock != life.residual + life.heat + life.closing ||
                    16 * life.eggs > life.closing)
                    throw std::logic_error("unbalanced clay rearing");
                food += life.meals;
                heat += life.heat;
                eggs += life.eggs;
                lives += life.opening != 0;
                queries += life.taps;
            }
            std::cout << "clay_season=" << epoch + 1 << "  living=" << lives << "  carbon=" << food << "  heat=" << heat
                      << "  taps=" << queries << "  eggs=" << eggs << '\n';
        }
        synchronize();
        std::vector<clay::Brood> brood(capacity);
        s.d.check(s.d.download(brood.data(), old.ptr, Count(capacity) * sizeof(brood[0])),
                  "return untrained context organs");
        std::vector<clay::Law> genes;
        for (auto &p : brood)
            if (p.alive)
                genes.push_back(p.genes);
        if (genes.empty())
            throw std::runtime_error("the potter brood went extinct");
        for (Word id = 0; id < n; ++id) {
            clay::swap_bank(h.potters[id], h.callers[id], 0);
            h.potters[id] = {genes[mix(seed + id * 3221) % genes.size()]};
        }
        s.d.check(s.d.upload(clayers.ptr, h.potters.data(), Count(n) * sizeof(clay::Mind)),
                  "settle vacant associative assemblies");
        s.d.check(s.d.upload(reed_animals.ptr, h.callers.data(), Count(n) * sizeof(siphon::Animal)),
                  "retain resident native projections");
        double time = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "clay_brood=" << genes.size() << "  seasons=" << seasons << "  seconds=" << time
                  << "  slots_per_s=" << Count(seasons) * capacity / time << '\n';
        return;
    }
    std::vector<clay::Law> germ;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            germ.push_back(h.potters[i].genes);
    Word ng = germ.size();
    Temporary laws(s.d, Count(ng) * sizeof(clay::Law)), results(s.d, Count(subjects) * sizeof(clay::Measure)),
        witness(s.d, sizeof(clay::Trace));
    s.d.check(s.d.upload(laws.ptr, germ.data(), Count(ng) * sizeof(clay::Law)),
              "retain the inhabitants' actual germlines");
    Function probe{};
    s.bind(probe, "clay_study", Image::siphons);
    constexpr const char *arms[]{"inferred",    "untapped",      "other_clay",     "one_assembly", "visible_color",
                                 "false_color", "clean",         "random_tap",     "permuted",     "erased",
                                 "answer_shut", "question_shut", "fixed_activity", "newborn",      "veiled_school"};
    std::vector<clay::Measure> output(subjects);
    for (Word mode = 0; mode < clay::treatments; ++mode) {
        auto start = std::chrono::steady_clock::now();
        launch(probe, (subjects + 127) / 128, 128, teachers.ptr, classes, laws.ptr, ng, results.ptr, witness.ptr,
               subjects, seed, lessons, taps, trials, mode);
        synchronize();
        s.d.check(s.d.download(output.data(), results.ptr, Count(subjects) * sizeof(output[0])),
                  "measure the paid cross-context grips");
        std::array<Count, sizeof(clay::Measure) / sizeof(Word)> sum{};
        for (auto &r : output) {
            auto values = std::bit_cast<std::array<Word, sum.size()>>(r);
            for (size_t j = 0; j < sum.size(); ++j)
                sum[j] += values[j];
        }
        if (sum[11] || (mode == clay::ear_closed && (sum[1] != 4 * sum[2] || sum[8])))
            throw std::logic_error("a hidden material cue reached the probe");
        double time = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "clay_transfer=" << arms[mode] << "  origin=" << (field ? "watershed" : "workshop")
                  << "  pupils=" << subjects << "  opportunities=" << sum[0] << "  harvested=" << sum[2]
                  << "  grains=" << sum[3] << "  work=" << sum[4] << "  quartets=" << sum[7] << "  complete=" << sum[8]
                  << "  school_sightings=" << sum[9] << "  probes=" << sum[19] << "  strikes=" << sum[10]
                  << "  target_strikes=" << sum[11] << "  diagnostic_choices=" << sum[13]
                  << "  distinguishing_strikes=" << sum[14] << "  possible_disagreements=" << sum[17]
                  << "  first_harvests=" << sum[18] << "  recruited=" << sum[15] << "  recovered=" << sum[12]
                  << "  both_complete=" << sum[16] << "  max_taps=" << taps << "  seconds=" << time << '\n';
        if (!mode) {
            clay::Trace t{};
            s.d.check(s.d.download(&t, witness.ptr, sizeof(t)), "transcribe one hidden clay exchange");
            for (Word j = 0; j < 2; ++j)
                std::cout << "clay_tablet=" << j << "  mineral=" << t.minerals[j] << "  first=" << t.patterns[j][0]
                          << "  second=" << t.patterns[j][1] << '\n';
            std::cout << "clay_episode=" << t.fixture << "  slot=" << t.address << "  tapped=" << t.observed
                      << "  before=" << t.prior << "  after=" << t.after << "  heard=" << t.response
                      << "  grip=" << t.grip << "  food=" << t.product << '\n';
        }
    }
    if (o.clay_switches) {
        Word length = o.clay_switches;
        Temporary records(s.d, Count(subjects) * 8 * sizeof(clay::Turn));
        std::vector<clay::Turn> by_age(Count(subjects) * 8);
        Function turn{};
        s.bind(turn, "clay_tides", Image::siphons);
        constexpr const char *conditions[]{"observed", "unheard", "pooled", "other_source", "answer_shut"};
        for (Word mode = 0; mode < 5; ++mode) {
            launch(turn, (subjects + 127) / 128, 128, teachers.ptr, classes, laws.ptr, ng, records.ptr, subjects, seed,
                   lessons, length, mode);
            synchronize();
            s.d.check(s.d.download(by_age.data(), records.ptr, Count(by_age.size()) * sizeof(by_age[0])),
                      "read choices after unsignaled changes");
            for (Word age = 0; age < 8; ++age) {
                std::array<Count, 6> total{};
                for (Word id = 0; id < subjects; ++id) {
                    auto v = std::bit_cast<std::array<Word, 6>>(by_age[Count(age) * subjects + id]);
                    for (Word k = 0; k < 6; ++k)
                        total[k] += v[k];
                }
                std::cout << "clay_reversal=" << conditions[mode] << "  lag=" << age << "  trials=" << total[0]
                          << "  harvested=" << total[1] << "  complete=" << total[2] << "  encounters=" << total[3]
                          << "  public_sounds=" << total[4] << "  assembly_changes=" << total[5]
                          << "  origin=" << (field ? "watershed" : "workshop") << '\n';
            }
        }
    }
    if (o.clay_foster) {
        Word gens = o.clay_foster;
        Temporary accounts(s.d, Count(subjects) * gens * sizeof(clay::Generation));
        Function birth{};
        s.bind(birth, "clay_generations", Image::siphons);
        std::vector<clay::Generation> rows(Count(subjects) * gens);
        for (Word mode : {Word(clay::adaptive), Word(clay::flat), Word(clay::shuffled), Word(clay::scraped)}) {
            launch(birth, (subjects + 127) / 128, 128, teachers.ptr, classes, laws.ptr, ng, accounts.ptr, subjects,
                   seed, lessons, taps, trials, gens, visits, windows, mode);
            synchronize();
            s.d.check(s.d.download(rows.data(), accounts.ptr, Count(rows.size()) * sizeof(rows[0])),
                      "trace the grandchildren of the object makers");
            for (Word generation = 0; generation < gens; ++generation) {
                std::array<Count, 9> total{};
                for (Word i = 0; i < subjects; ++i) {
                    auto q = std::bit_cast<std::array<Word, 9>>(rows[Count(generation) * subjects + i]);
                    for (Word j = 0; j < 9; ++j)
                        total[j] += q[j];
                }
                std::cout << "clay_generation=" << generation + 1 << "  origin=" << (field ? "watershed" : "workshop")
                          << "  exposure=" << arms[mode] << "  trials=" << total[0] << "  ancestral=" << total[1]
                          << "  other_object=" << total[2] << "  blocked=" << total[3] << "  complete=" << total[4]
                          << "  new_impressions=" << total[5] << "  new_carbon=" << total[6] << "  work=" << total[7]
                          << "  inherited_sightings=" << total[8] << '\n';
            }
        }
    }
    if (!field && !o.clay_specimen.empty()) {
        // Input strength and language are screened on a separate set of tables.
        Word screening = seed ^ 0x8f134beb;
        Temporary ranks(s.d, subjects * sizeof(Word)), specimen(s.d, sizeof(clay::Specimen));
        Function show{}, bring{};
        s.bind(show, "clay_screen", Image::siphons);
        s.bind(bring, "clay_recover", Image::siphons);
        launch(show, (subjects + 127) / 128, 128, teachers.ptr, classes, laws.ptr, ng, subjects, screening, lessons,
               taps, ranks.ptr);
        std::vector<Word> scores(subjects);
        synchronize();
        s.d.check(s.d.download(scores.data(), ranks.ptr, subjects * 4), "read independent school screening");
        Word winner = std::max_element(scores.begin(), scores.end()) - scores.begin();
        launch(bring, 1, 1, teachers.ptr, classes, laws.ptr, ng, winner, screening, lessons, specimen.ptr);
        clay::Specimen q;
        synchronize();
        s.d.check(s.d.download(&q, specimen.ptr, sizeof(q)), "retain the selected pupil's actual contacts");
        auto &t = village[q.school];
        Word k = mix(seed + q.school * 3917);
        auto origins = [&](Word campus) { return sites[mix(k + ((campus && unrelated) ? 379 : 731)) % donors]; };
        auto pupil = extract_individual(h, origins(q.ancestor).second, false);
        pupil.calls.animal = q.child.body;
        pupil.ceramic.mind = q.child.cortex;
        pupil.ceramic.palm = {};
        pupil.resident.animal.energy = 64 + 24 + 2 * lessons - q.paid;
        storage::write_checked(o.clay_specimen + "-pupil.mind", pupil);
        for (Word j = 0; j < 2; ++j) {
            auto who = extract_individual(h, origins(j).first, false);
            who.calls.animal = t.adults[j].a;
            who.ceramic.mind = {clay::founder(mix(who.calls.animal.genes.anatomy + 6119))};
            who.ceramic.palm = {};
            auto name = o.clay_specimen + (j ? "-b" : "-a");
            storage::write_checked(name + ".mind", who);
            clay::Collection kit;
            kit.clock = visits;
            kit.glaze[0] = t.inlet_color[j];
            kit.glaze[1] = t.mineral[j];
            kit.objects[0] = t.artifacts[j][0];
            kit.objects[1] = t.artifacts[j][1];
            clay::save_collection(name + ".clay", kit);
        }
        std::cout << "clay_specimen=" << winner << "  screened=" << subjects << "  school=" << q.school
                  << "  score=" << scores[winner] << "  seed=" << screening << "  learned_contacts=" << q.materials
                  << "  paid=" << q.paid << "  assemblies=" << clay::count(q.child.cortex) << '\n';
    }
}
void clay_census(const Habitat &h) {
    if (!h.pottery)
        return;
    Count occupied = 0, grips = 0, full = 0, learned = 0, capacity = 0, peak = 0;
    for (Word i = 0; i < n; ++i) {
        Word a = clay::size(h.ware[i]), b = clay::size(h.cargo[i]);
        occupied += a != 0;
        grips += b != 0;
        full += a + b;
        if (h.bodies[i].energy) {
            auto &m = h.potters[i];
            learned += clay::count(m);
            capacity += clay::capacity(m);
            peak = std::max(peak, Count(clay::count(m)));
        }
    }
    auto &r = h.workshop.book;
    std::cout << "clay_tick=" << h.header.tick << "  ground=" << occupied << "  held=" << grips << "  grooves=" << full
              << "  written=" << r.impressions << "  tapped=" << r.taps << "  read=" << r.recovered
              << "  before_birth=" << r.old << "  carried=" << r.movements << "  abrasion=" << r.erosion
              << "  work=" << r.work << "  sensory_edits=" << r.sensory_edits << "  loans=" << r.loans
              << "  sweeps=" << r.strokes << "  motor_edits=" << r.motor_edits << "  switches=" << r.remaps
              << "  assemblies=" << learned << "  possible=" << capacity << "  highest=" << peak << '\n';
    Word shown = 0, rank = 0;
    bool hand = false;
    for (Word pos = 0; pos < n; ++pos)
        for (Word k = 0; k < 2; ++k) {
            Word id = h.slots[pos];
            auto &tile = k ? h.cargo[id] : h.ware[pos];
            Word size = clay::size(tile);
            if (size > rank) {
                shown = pos;
                rank = size;
                hand = k;
            }
        }
    if (rank) {
        Word id = h.slots[shown];
        auto tile = hand ? h.cargo[id] : h.ware[shown];
        auto grain = hand ? h.workers[id].hand : h.mineral_bed[shown];
        std::cout << "clay_object=" << shown << "  held=" << hand << "  owner=" << id
                  << "  mineral=" << Word(grain.substance) << "  faces=" << Word(grain.faces)
                  << "  first=" << tile.groove[0] << "  second=" << tile.groove[1] << "  last_change=" << tile.changed
                  << "  grooves=" << rank << '\n';
    }
}
void clay_benchmark(const Habitat &h, Word calls) {
    if (!calls)
        return;
    std::vector<clay::Portable> people;
    Count multiple = 0;
    for (Word pos = 0; pos < n; ++pos) {
        Word id = h.slots[pos];
        if (!h.bodies[id].energy || !clay::count(h.potters[id]))
            continue;
        auto kit = local_artifacts(h, pos, true);
        if (!clay::size(kit.objects[1]))
            continue;
        people.push_back({{h.callers[id], h.potters[id]}, kit.objects[1]});
        multiple += clay::count(h.potters[id]) > 1;
    }
    if (people.empty()) {
        std::cout << "clay_deployment=vacant\n";
        return;
    }
    Word count = people.size(), blocks = (calls + 127) / 128;
    Temporary population(s.d, Count(count) * sizeof(people[0])), charts(s.d, Count(count) * sizeof(clay::Deployment)),
        raw(s.d, Count(calls) * sizeof(Count)), compiled(s.d, Count(calls) * sizeof(Count)), diff(s.d, 4);
    s.d.check(s.d.upload(population.ptr, people.data(), Count(count) * sizeof(people[0])),
              "retain residents with actual worked grains");
    Function fold{}, one{}, two{}, compare{};
    s.bind(fold, "clay_compile", Image::siphons);
    s.bind(one, "clay_dynamic", Image::siphons);
    s.bind(two, "clay_projected", Image::siphons);
    s.bind(compare, "siphon_mismatches", Image::siphons);
    launch(fold, (count + 127) / 128, 128, population.ptr, charts.ptr, count);
    synchronize();
    s.d.check(s.d.zero(diff.ptr, 0, 4), "clear clay readout differences");
    cuda::Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "start clay clock");
    s.d.check(s.d.event_new(&end, 0), "stop clay clock");
    std::array<float, 7> timings[2]{};
    for (Word t = 0; t < 8; ++t) {
        Word key = mix(c.seed + t * 3779);
        for (Word phase = 0; phase < 2; ++phase) {
            Word fast = (t + phase) & 1;
            auto output = fast ? compiled.ptr : raw.ptr;
            s.d.check(s.d.record(start, stream), "start object choice");
            launch(fast ? two : one, blocks, 128, fast ? charts.ptr : population.ptr, count, output, calls, key);
            s.d.check(s.d.record(end, stream), "close object timer");
            s.d.check(s.d.wait(end), "finish object choices");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, start, end), "measure evidence collection");
            if (t)
                timings[fast][t - 1] = ms;
        }
        launch(compare, blocks, 128, raw.ptr, compiled.ptr, diff.ptr, calls);
    }
    synchronize();
    s.d.event_free(start);
    s.d.event_free(end);
    Word failure = 0;
    s.d.check(s.d.download(&failure, diff.ptr, 4), "compare complete inferred sources and motor states");
    if (failure)
        throw std::logic_error("clay compilation changed a source or action");
    for (auto &t : timings)
        std::sort(t.begin(), t.end());
    std::cout << "clay_deployment=" << count << "  multiple=" << multiple << "  calls=" << calls
              << "  compared=" << Count(calls) * 8 << "  differences=0  frozen_bytes=" << sizeof(clay::Frozen)
              << "  chart_bytes=" << sizeof(clay::Charts) << "  context_bytes=" << sizeof(clay::Mind)
              << "  native_bytes=" << sizeof(siphon::Animal) << "  mutable_ms=" << timings[0][3]
              << "  median_ms=" << timings[1][3] << "  Mclay_choices_per_s=" << calls / (timings[1][3] * 1000) << '\n';
}
