void restore_costs(const effort::Ledger &ledger) {
    s.d.check(s.d.zero(cost_books.ptr, 0, Count((n / 2 + 127) / 128) * sizeof(ledger)), "replace effort ledgers");
    s.d.check(s.d.upload(cost_books.ptr, &ledger, sizeof(ledger)), "restore physical and developmental accounts");
}
void wash_treads(Habitat &h) {
    auto &book = h.economics.ledger;
    book.washed = book.pressed;
    restore_costs(book);
    s.d.check(s.d.zero(treads.ptr, 0, n), "let loose sediment fill the footprints");
}
void effort_rear(Habitat &h, const Options &o) {
    if (!o.effort_seasons && o.effort_germs.empty())
        return;
    sample(h);
    std::vector<effort::Genes> donors;
    if (!o.effort_germs.empty()) {
        Habitat nursery;
        nursery.read(o.effort_germs);
        if (nursery.appraising)
            for (Word i = 0; i < nursery.sites(); ++i)
                if (nursery.bodies[i].energy)
                    donors.push_back(nursery.economies[i].genes);
        if (donors.empty())
            throw std::invalid_argument("no living effort germlines in source");
    }
    Word adults = o.effort_population, seed = o.effort_seed, paid = o.effort_paid, practice = o.effort_practice;
    std::vector<effort::Family> families(adults);
    for (Word i = 0; i < adults; ++i) {
        Word key = mix(seed + i * 4079);
        families[i].genes = donors.empty() ? effort::founder(key) : donors[key % donors.size()];
    }
    if (o.effort_seasons) {
        Count bytes = Count(adults) * sizeof(families[0]);
        Temporary before(s.d, bytes), next(s.d, bytes);
        s.d.check(s.d.upload(before.ptr, families.data(), bytes), "place the physical foraging cohorts");
        Function eat{}, breed{};
        s.bind(eat, "effort_parents", Image::siphons);
        s.bind(breed, "effort_offspring", Image::siphons);
        for (Word j = 0; j < o.effort_seasons; ++j) {
            launch(eat, (adults + 127) / 128, 128, before.ptr, adults, seed, j, practice);
            if (!j || (j + 1) % 64 == 0 || j + 1 == o.effort_seasons) {
                synchronize();
                s.d.check(s.d.download(families.data(), before.ptr, bytes),
                          "read reproductive energy remaining after travel");
                Count sum = 0;
                Word max = 0;
                for (auto &f : families) {
                    sum += f.eggs;
                    max = std::max(max, f.eggs);
                }
                std::cout << "effort_season=" << j + 1 << "  parents=" << adults << "  eggs=" << sum
                          << "  maximum=" << max << "  earned=" << paid << '\n';
            }
            launch(breed, (adults / 2 + 127) / 128, 128, before.ptr, next.ptr, adults, seed, j, paid);
            std::swap(before.ptr, next.ptr);
        }
        synchronize();
        s.d.check(s.d.download(families.data(), before.ptr, bytes), "read newborn weighting and plasticity genes");
    }
    for (Word i = 0; i < n; ++i)
        h.economies[i] = {families[mix(seed + i * 9161) & (adults - 1)].genes};
    s.d.check(s.d.upload(cost_minds.ptr, h.economies.data(), Count(n) * sizeof(effort::Mind)),
              "introduce effort organs before experience");
}
void effort_study(const Habitat &h, const Options &o) {
    if (!h.appraising)
        return;
    std::vector<effort::Genes> genes;
    std::vector<Word> ids;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy) {
            genes.push_back(h.economies[i].genes);
            ids.push_back(i);
        }
    Word ng = genes.size(), count = o.effort_test, seed = mix(o.effort_seed + 917529);
    if (!ng && !o.effort_scholar.empty())
        throw std::invalid_argument("an empty watershed has no prospective organ to export");
    if (ng && (count || !o.effort_scholar.empty())) {
        Temporary laws(s.d, Count(ng) * sizeof(genes[0]));
        s.d.check(s.d.upload(laws.ptr, genes.data(), Count(ng) * sizeof(genes[0])),
                  "place living benefit and cost germlines");
        if (count) {
            Function probe{};
            s.bind(probe, "effort_probe", Image::siphons);
            Temporary ans(s.d, Count(count) * sizeof(effort::Crossing)), detail(s.d, 4 * sizeof(effort::Card));
            const char *names[]{"lived",      "old_fruit",  "old_drag", "empty",         "no_food", "no_work",
                                "other_food", "other_work", "fixed",    "false_surface", "smooth",  "two_routes"};
            std::vector<effort::Crossing> rows(count);
            for (Word mode = 0; mode < effort::internal_count; ++mode) {
                launch(probe, (count + 127) / 128, 128, laws.ptr, ng, count, seed, mode, ans.ptr, detail.ptr);
                synchronize();
                s.d.check(s.d.download(rows.data(), ans.ptr, Count(count) * sizeof(rows[0])),
                          "count reached patches and paid choices");
                std::array<Count, sizeof(effort::Crossing) / 4> t{};
                for (const auto &v : rows) {
                    auto a = std::bit_cast<std::array<Word, t.size()>>(v);
                    for (size_t j = 0; j < t.size(); ++j)
                        t[j] += a[j];
                }
                std::cout << "effort_choice=" << names[mode] << "  trials=" << t[0] << "  correct=" << t[1]
                          << "  stayed=" << t[2] << "  went=" << t[3] << "  food=" << t[4] << "  work=" << t[5]
                          << "  optimum=" << static_cast<long long>(t[6]) - 32ll * count
                          << "  net=" << static_cast<long long>(t[7]) - 32ll * count << "  regret=" << t[8]
                          << "  always_near=" << static_cast<long long>(t[9]) - 32ll * count
                          << "  always_far=" << static_cast<long long>(t[10]) - 32ll * count << "  ties=" << t[12]
                          << "  near_correct=" << t[14] << "  far_correct=" << t[15] << "  near_better=" << t[16]
                          << "  far_better=" << t[17] << "  changed=" << t[20] << "  changed_correct=" << t[21] << '\n';
                if (mode == effort::lived) {
                    std::array<effort::Card, 4> cards;
                    s.d.check(s.d.download(cards.data(), detail.ptr, std::min(4u, count) * sizeof(cards[0])),
                              "recover matched physical episodes");
                    for (Word i = 0; i < std::min(4u, count); ++i) {
                        auto &q = cards[i];
                        std::cout << "effort_receipt=" << q.id << "  old_food=" << q.fruit_map
                                  << "  fruit=" << q.other_map << "  old_toll=" << q.tariff << "  toll=" << q.new_tariff
                                  << "  home=" << q.home << "  away=" << q.away << "  ground=" << q.surface
                                  << "  near_units=" << (q.units & 255) << "  far_units=" << (q.units >> 8)
                                  << "  near=" << q.near << "  far=" << q.far << "  trip=" << q.chosen
                                  << "  paid=" << q.worked << "  ate=" << q.ate << "  left_current=" << q.left
                                  << "  right_current=" << q.right << "  taste_law=" << q.genes.appetite
                                  << "  load_law=" << q.genes.muscle << "  valuation=" << q.genes.projection
                                  << "  credit=" << q.genes.episode << "  yield_low=" << q.brain.food.low
                                  << "  yield_high=" << q.brain.food.high << "  effort_low=" << q.brain.work.low
                                  << "  effort_high=" << q.brain.work.high << '\n';
                    }
                }
            }
        }
        if (count)
            effort_load_study(laws.ptr, ng, count, seed);
        if (!o.effort_scholar.empty())
            effort_scholar(h, o, laws.ptr, ids, seed);
    }
    auto &b = h.economics.ledger;
    Count worn = 0, units = 0, light = 0, taste = 0, strain = 0, pending = 0, tagged[2]{};
    for (Word i = 0; i < n; ++i) {
        auto a = h.treads[i];
        worn += bool(a & 15) + bool(a >> 4);
        units += (a & 15) + (a >> 4);
        if (!h.bodies[i].energy)
            continue;
        const auto &m = h.economies[i];
        light += bool(m.experience & 255);
        taste += std::popcount(m.experience & 15);
        strain += std::popcount((m.experience >> 4) & 15);
        pending += bool(effort::remaining(m));
        ++tagged[bool(h.wanderers[i].efferent & hollow::lineage_tag)];
    }
    std::cout << "effort_field=" << h.header.tick << "  living=" << ng << "  experienced=" << light
              << "  food_cues=" << taste << "  surfaces=" << strain << "  tread_edges=" << worn
              << "  tread_units=" << units << "  worn=" << b.pressed << "  eroded=" << b.washed
              << "  strides=" << b.walks << "  preparation_work=" << b.contacts << "  paid=" << b.price
              << "  fresh_price=" << b.fresh_price << "  saved=" << b.saved << "  offers=" << b.offers
              << "  trips=" << b.trips << "  ordinary=" << b.stays << "  closed=" << b.settled
              << "  credited=" << b.credited << "  died=" << b.deaths << "  offspring_energy=" << b.inherited
              << "  pending_living=" << pending << '\n';
    if (h.economics.mode & effort::appraisal) {
        Count recent[8]{}, rich = 0;
        for (Word i = 0; i < n; ++i)
            if (h.bodies[i].energy) {
                Word flags = h.economies[i].experience;
                recent[(flags >> 20) & 7]++;
                rich += bool(flags & 0xff000u);
            }
        std::cout << "effort_builders=" << h.header.tick << "  traffic_memory=" << rich
                  << "  unknown_rest=" << recent[1] << "  learned_rest=" << recent[2] << "  exploratory=" << recent[3]
                  << "  one_stroke=" << recent[4] << "  two_strokes=" << recent[6]
                  << "  unfamiliar=" << bool(h.economics.mode & effort::foreign_surface) << '\n';
    }
    if (auto *journal = journal::current()) {
        Word side = h.header.configuration.side, dim = std::min(side, 32u), scale = side / dim, area = scale * scale;
        constexpr char hex[] = "0123456789abcdef";
        for (Word y = 0; y < dim; ++y) {
            std::string pixels;
            pixels.reserve(3 * dim);
            for (Word x = 0; x < dim; ++x) {
                Word east = 0, south = 0, pop = 0;
                for (Word dy = 0; dy < scale; ++dy)
                    for (Word dx = 0; dx < scale; ++dx) {
                        Word at = (y * scale + dy) * side + x * scale + dx;
                        east += h.treads[at] & 15;
                        south += h.treads[at] >> 4;
                        pop += h.bodies[h.slots[at]].energy != 0;
                    }
                pixels += hex[(east + area / 2) / area];
                pixels += hex[(south + area / 2) / area];
                pixels += hex[(15 * pop + area / 2) / area];
            }
            journal->line("effort_tread=" + std::to_string(y) + "  pixels=" + pixels);
        }
    }
    for (Word k = 0; k < 4; ++k) {
        auto &g = b.experiment[k];
        if (g.n)
            std::cout << "effort_random=" << k << "  predicted=" << (k >> 1) << "  assigned=" << (k & 1)
                      << "  trials=" << g.n << "  dead=" << g.lost << "  gains=" << g.gained
                      << "  losses=" << g.depleted << '\n';
    }
    if (h.economics.mode & effort::comp)
        for (Word j = 0; j < 2; ++j)
            std::cout << "effort_lineage=" << j << "  regulated="
                      << bool((h.economics.mode & effort::same_comp) ||
                              (bool(j) != bool(h.economics.mode & effort::reverse_comp)))
                      << "  living=" << tagged[j] << "  tick=" << h.header.tick << '\n';
}
void effort_scholar(const Habitat &h, const Options &o, DevicePtr laws, const std::vector<Word> &ids, Word seed) {
    Word count = ids.size();
    Temporary scores(s.d, Count(count) * 4);
    Function f{};
    s.bind(f, "effort_candidates", Image::siphons);
    launch(f, (count + 127) / 128, 128, laws, count, seed, scores.ptr);
    synchronize();
    std::vector<Word> points(count);
    s.d.check(s.d.download(points.data(), scores.ptr, Count(count) * 4),
              "screen prospective organs on independent physical offers");
    Word at = ~0u, eligible = 0;
    for (Word i = 0; i < count; ++i) {
        const auto &anatomy = h.bodies[ids[i]].body;
        if (o.effort_carrier.empty() && (!anatomy.limbs || !anatomy.pores || !anatomy.eyes))
            continue;
        ++eligible;
        if (at == ~0u || points[i] > points[at])
            at = i;
    }
    if (at == ~0u)
        throw std::invalid_argument("there are no living pupils with eyes, limbs and pores");

    auto pupil = extract_individual(h, ids[at], false);
    if (!o.effort_carrier.empty()) {
        pupil.read(o.effort_carrier);
        if (!pupil.ranges || !pupil.calling)
            throw std::invalid_argument("cost carrier needs acoustic and spatial tissue");
        pupil.values_effort = true;
        pupil.prospects = {};
        pupil.prospects.mind.genes = h.economies[ids[at]].genes;
    }
    storage::write_checked(o.effort_scholar, pupil);
    std::cout << "effort_scholar=" << ids[at] << "  eligible=" << eligible << "  offers=128  capital=" << points[at]
              << "  seed=" << seed << "  graft=" << !o.effort_carrier.empty() << '\n';
}

void effort_load_study(DevicePtr laws, Word ng, Word n, Word seed) {
    Temporary out(s.d, Count(n) * sizeof(effort::Cause)), proof(s.d, 4 * sizeof(effort::Sample));
    Function f{};
    s.bind(f, "effort_loads", Image::siphons);
    const char *names[]{"inferred",   "erased",   "foreign",     "prior",       "untested",
                        "one_action", "one_load", "eight_loads", "two_strokes", "unchanged"};
    std::vector<effort::Cause> rows(n);
    for (Word mode = 0; mode < effort::cause_count; ++mode) {
        launch(f, (n + 127) / 128, 128, laws, ng, n, seed, mode, out.ptr, proof.ptr);
        synchronize();
        s.d.check(s.d.download(rows.data(), out.ptr, Count(n) * sizeof(rows[0])),
                  "audit actual preparation and repeated crossings");
        std::array<Count, sizeof(effort::Cause) / 4> sum{};
        for (const auto &r : rows) {
            auto v = std::bit_cast<std::array<Word, sum.size()>>(r);
            for (size_t j = 0; j < v.size(); ++j)
                sum[j] += v[j];
        }
        std::cout << "effort_slat=" << names[mode] << "  offers=" << sum[0] << "  correct=" << sum[1]
                  << "  prepared=" << sum[2] << "  better_prepared=" << sum[3] << "  good_preparation=" << sum[4]
                  << "  good_ordinary=" << sum[5] << "  better_ordinary=" << sum[6] << "  ties=" << sum[7]
                  << "  food=" << sum[8] << "  price=" << sum[9] << "  regret=" << sum[10] << "  strokes=" << sum[13]
                  << "  undetermined=" << sum[14] << "  predicted_faces=" << sum[17] << "  best_price=" << sum[19]
                  << "  no_preparation=" << sum[20] << "  all_prepared=" << sum[21] << '\n';
        if (mode == effort::new_route) {
            std::array<effort::Sample, 4> cards;
            s.d.check(s.d.download(cards.data(), proof.ptr, std::min(4u, n) * sizeof(cards[0])),
                      "read four physical work orders");
            for (Word j = 0; j < std::min(4u, n); ++j) {
                auto &q = cards[j];
                std::cout << "effort_slab=" << q.seed << "  visible=" << q.surface << "  faces=" << q.dye
                          << "  rates=" << q.floor << "  handles=" << q.price << "  crossings=" << q.repetitions
                          << "  strokes=" << q.sequence << "  length=" << q.length << "  after=" << q.physical
                          << "  expected=" << q.predicted << "  prepared=" << q.choice << "  work=" << q.actual_cost
                          << "  harvest=" << q.food << "  normal_current=" << q.a << "  prepared_current=" << q.b
                          << "  learned_edges=" << q.transitions << "  learned_costs=" << q.rate
                          << "  wiring=" << q.rule << '\n';
            }
        }
    }
}
void effort_census(const Habitat &h, const Options &o) {
    if (!o.effort_survey)
        return;
    Temporary results(s.d, Count(n) * sizeof(effort::Survey));
    std::vector<effort::Survey> rows(n);
    Function rank{};
    s.bind(rank, "effort_estuary", Image::floodplain);
    Word tick = h.header.tick, seed = o.effort_seed;
    const char *names[]{"lived", "blank", "wrong_surface", "other_body"};
    for (Word mode = 0; mode < 4; ++mode) {
        launch(rank, (n + 127) / 128, 128, world, c, tick, seed, mode, results.ptr);
        synchronize();
        s.d.check(s.d.download(rows.data(), results.ptr, Count(n) * sizeof(rows[0])),
                  "audit a one-step bite on actual adjacent terrain");
        std::array<Count, sizeof(effort::Survey) / 4> sum{};
        for (auto &r : rows) {
            auto p = std::bit_cast<std::array<Word, sum.size()>>(r);
            for (size_t j = 0; j < p.size(); ++j)
                sum[j] += p[j];
        }
        std::cout << "effort_appraisal=" << names[mode] << "  offers=" << sum[0] << "  crossed=" << sum[1]
                  << "  correct=" << sum[2] << "  better_here=" << sum[3] << "  better_there=" << sum[4]
                  << "  here_correct=" << sum[5] << "  there_correct=" << sum[6] << "  ties=" << sum[7]
                  << "  available_food=" << sum[8] << "  price=" << sum[9] << "  missed_carbon=" << sum[10]
                  << "  crossing_price=" << sum[11] << "  virgin_price=" << sum[12] << "  worn=" << sum[13]
                  << "  encountered=" << sum[14] << '\n';
    }
}
void effort_quays(const Habitat &h, const Options &o) {
    if (!(o.effort_survey && (h.economics.mode & effort::appraisal)))
        return;
    Word tick = h.header.tick;
    Temporary out(s.d, Count(n) * sizeof(effort::Paving));
    std::vector<effort::Paving> rows(n);
    Function f{};
    s.bind(f, "effort_quay", Image::floodplain);
    for (Word duration : {1u, 4u, 8u})
        for (Word mode = 0; mode < 3; ++mode) {
            launch(f, (n + 127) / 128, 128, world, c, tick, duration, mode, out.ptr);
            synchronize();
            s.d.check(s.d.download(rows.data(), out.ptr, Count(n) * sizeof(rows[0])),
                      "measure finite work on matched cloned edges");
            std::array<Count, sizeof(effort::Paving) / 4> sum{};
            for (auto &r : rows) {
                auto a = std::bit_cast<std::array<Word, sum.size()>>(r);
                for (size_t j = 0; j < a.size(); ++j)
                    sum[j] += a[j];
            }
            std::cout << "effort_paving="
                      << (mode == 0   ? "lived"
                          : mode == 1 ? "erased"
                                      : "foreign")
                      << "  uses=" << duration << "  sites=" << sum[0] << "  selected=" << sum[1]
                      << "  strokes=" << sum[2] << "  best=" << sum[3] << "  beneficial_sites=" << sum[4]
                      << "  beneficial=" << sum[5] << "  costly=" << sum[6] << "  price=" << sum[7]
                      << "  plain=" << sum[8] << "  oracle=" << sum[9] << "  confident=" << sum[10]
                      << "  double=" << sum[11] << '\n';
        }
}
void effort_benchmark(const Habitat &h, const Options &o) {
    Word calls = o.effort_calls;
    if (!calls)
        return;
    std::vector<effort::Mind> residents;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            residents.push_back(h.economies[i]);
    if (residents.empty())
        return;
    Word people = residents.size(), batch = std::min(1u << 20, calls);
    Temporary m(s.d, Count(people) * sizeof(effort::Mind)), f(s.d, Count(people) * sizeof(effort::Frozen)),
        a(s.d, Count(batch) * 8), b(s.d, Count(batch) * 8), errors(s.d, 4);
    s.d.check(s.d.upload(m.ptr, residents.data(), Count(people) * sizeof(effort::Mind)),
              "place measured living circuits");
    s.d.check(s.d.zero(errors.ptr, 0, 4), "clear route comparison");
    Function make{}, normal{}, pack{}, compare{};
    s.bind(make, "effort_images", Image::siphons);
    s.bind(normal, "effort_live_queries", Image::siphons);
    s.bind(pack, "effort_frozen_queries", Image::siphons);
    s.bind(compare, "hollow_compare", Image::siphons);
    launch(make, (people + 127) / 128, 128, m.ptr, f.ptr, people);
    cuda::Event begin{}, end{};
    s.d.check(s.d.event_new(&begin, 0), "start route timer");
    s.d.check(s.d.event_new(&end, 0), "stop route timer");
    std::array<float, 5> times[2]{};
    for (Word j = 0; j < 6; ++j)
        for (Word offset = 0; offset < calls; offset += batch) {
            Word len = std::min(batch, calls - offset), blocks = (len + 127) / 128,
                 seed = mix(o.effort_seed + j * 1987);
            for (Word trial = 0; trial < 2; ++trial) {
                Word fast = (j + trial) & 1;
                auto &dest = fast ? b : a;
                s.d.check(s.d.record(begin, stream), "time two complete prospective currents");
                launch(fast ? pack : normal, blocks, 128, m.ptr, f.ptr, people, len, offset, seed, dest.ptr);
                s.d.check(s.d.record(end, stream), "end current timing");
                s.d.check(s.d.wait(end), "finish price readout");
                float ms = 0;
                s.d.check(s.d.elapsed(&ms, begin, end), "read device interval");
                if (j)
                    times[fast][j - 1] += ms;
            }
            launch(compare, blocks, 128, a.ptr, b.ptr, len, errors.ptr);
        }
    synchronize();
    s.d.event_free(begin);
    s.d.event_free(end);
    Word wrong = 0;
    s.d.check(s.d.download(&wrong, errors.ptr, 4), "check both signed route currents");
    if (wrong)
        throw std::logic_error("packed route values differ from writable currents");
    for (auto &v : times)
        std::ranges::sort(v);
    std::cout << "effort_deploy=" << people << "  comparisons=" << calls << "  currents_checked=" << Count(calls) * 12
              << "  errors=" << wrong << "  writable_bytes=" << sizeof(effort::Mind)
              << "  image_bytes=" << sizeof(effort::Frozen) << "  writable_ms=" << times[0][2]
              << "  packed_ms=" << times[1][2] << "  Mcomparisons_per_s=" << calls / (1000. * times[1][2]) << '\n';
}
