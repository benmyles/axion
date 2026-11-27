void unthread() {
    Function fn{};
    s.bind(fn, "practice_unthread", Image::floodplain);
    Word tiles = n / 4;
    launch(fn, (tiles + 127) / 128, 128, world, c, tiles);
    synchronize();
}
void replace_strands(const Habitat &donor, Habitat &h) {
    h.legacy_work = {};
    s.d.check(s.d.zero(practice_credits.ptr, 0, Count((n / 4 + 127) / 128) * sizeof(practice::Credits)),
              "start new construction return accounts");
    if (donor.practising) {
        auto works = donor.strands;
        for (auto &v : works)
            if (v.who) {
                v.who = ~0u;
                v.born = 0;
                v.date = ~0u;
            }
        s.d.check(s.d.upload(marked_strands.ptr, works.data(), works.size() * sizeof(works[0])),
                  "transfer the original public splice positions");
    } else
        s.d.check(s.d.zero(marked_strands.ptr, 0, Count(n / 4) * sizeof(practice::Mark)),
                  "clear marks on replacement soil");
    Function fn{};
    s.bind(fn, "practice_release_attention", Image::floodplain);
    launch(fn, (n + 127) / 128, 128, world.practice);
    synchronize();
}
void practice_rear(Habitat &h, const Options &o) {
    if (!h.practising || (!o.practice_epochs && o.practice_stock.empty()))
        return;
    sample(h);
    std::vector<practice::Genes> germ;
    Word nparents = o.practice_adults, seed = o.practice_seed, paid = o.practice_paid, cap = h.learning.arena;
    if (!o.practice_stock.empty()) {
        Habitat donor;
        donor.read(o.practice_stock);
        if (donor.practising)
            for (Word i = 0; i < donor.sites(); ++i)
                if (donor.bodies[i].energy)
                    germ.push_back(donor.pupils[i].genes);
        if (germ.empty())
            throw std::invalid_argument("no living practice germlines in donor");
    }
    std::vector<practice::Family> pool(nparents);
    for (Word i = 0; i < nparents; ++i) {
        Word k = mix(seed + i * 3169);
        pool[i].genes = germ.empty() ? practice::founder(k) : germ[k % germ.size()];
    }
    if (o.practice_epochs) {
        Function meal{}, breed{};
        s.bind(meal, o.practice_teaching ? "practice_patch_meals" : "practice_meals", Image::siphons);
        Word mixed = o.practice_teaching == 2, island = o.practice_island;
        s.bind(breed, "practice_breed", Image::siphons);
        Temporary current(s.d, nparents * sizeof(pool[0])), next(s.d, nparents * sizeof(pool[0]));
        s.d.check(s.d.upload(current.ptr, pool.data(), nparents * sizeof(pool[0])), "place attention germlines");
        for (Word t = 0; t < o.practice_epochs; ++t) {
            if (o.practice_teaching)
                launch(meal, (nparents + 63) / 64, 64, current.ptr, nparents, seed, t, cap, mixed, island);
            else
                launch(meal, (nparents + 63) / 64, 64, current.ptr, nparents, seed, t, cap);
            if (!t || (t + 1) % 64 == 0 || t + 1 == o.practice_epochs) {
                synchronize();
                s.d.check(s.d.download(pool.data(), current.ptr, nparents * sizeof(pool[0])),
                          "read real food and reproductive resources");
                Count eggs = 0;
                for (auto &v : pool)
                    eggs += v.eggs;
                std::cout << "practice_season=" << t + 1 << "  adults=" << nparents << "  eggs=" << eggs
                          << "  earned=" << paid << '\n';
            }
            launch(breed, (nparents / 2 + 127) / 128, 128, current.ptr, next.ptr, nparents, seed, t, paid);
            std::swap(current.ptr, next.ptr);
        }
        synchronize();
        s.d.check(s.d.download(pool.data(), current.ptr, nparents * sizeof(pool[0])), "read juvenile genotypes");
    }
    for (Word i = 0; i < n; ++i)
        h.pupils[i] = {pool[mix(seed + i * 8171) & (nparents - 1)].genes};
    std::fill(h.memories.begin(), h.memories.end(), practice::Context{});
    s.d.check(s.d.upload(apprentices.ptr, h.pupils.data(), Count(n) * sizeof(practice::Mind)),
              "replace only the private practice organs");
    s.d.check(s.d.zero(contexts.ptr, 0, Count(n) * h.learning.arena * sizeof(practice::Context)),
              "hatch with no acquired causal models");
}
void practice_study(const Habitat &h, const Options &o) {
    if (!h.practising)
        return;
    Count living = 0, experienced = 0, contacts = 0, banks = 0, latched = 0, eight = 0, thirty = 0;
    std::array<Count, 2> families{};
    std::vector<practice::Genes> genes;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy) {
            auto &m = h.pupils[i];
            ++living;
            experienced += !!m.samples;
            contacts += m.samples;
            banks += practice::size(m);
            latched += !!m.focus;
            eight += m.samples >= 8;
            thirty += m.samples >= 32;
            ++families[bool(h.wanderers[i].efferent & hollow::lineage_tag)];
            genes.push_back(m.genes);
        }
    auto &b = h.learning.book;
    std::cout << "practice_field=" << h.header.tick << "  living=" << living << "  observing=" << experienced
              << "  contacts=" << contacts << "  learned_contexts=" << banks << "  latched=" << latched
              << "  eight_contacts=" << eight << "  thirty_two=" << thirty << "  offers=" << b.invitations
              << "  entered=" << b.entered << "  continued=" << b.continued << "  left=" << b.left
              << "  stayed=" << b.stayed << "  strokes=" << b.strokes << "  informative=" << b.chosen
              << "  random=" << b.random << "  writes=" << b.writes << "  witnesses=" << b.observed
              << "  recruited=" << b.hypotheses << "  switches=" << b.switches << "  heat=" << b.work
              << "  gathered=" << b.collections << "  searches=" << b.searches << "  proposed=" << b.proposals
              << "  repaired=" << b.alignments << "  fitting_plans=" << b.fittings << "  fibre_in=" << b.placed
              << "  fibre_out=" << b.removed << "  join_edits=" << b.loops_learned << "  predicted=" << b.direct_known
              << "  exact=" << b.direct_exact << "  long_bouts=" << b.bouts_eight
              << "  changed_handles=" << b.changed_handles << "  guarded=" << b.protected_goal
              << "  arena=" << h.learning.arena << '\n';
    auto &ret = h.legacy_work;
    Count standing = 0;
    for (auto &z : h.strands)
        standing += !!z.who;
    std::cout << "practice_returns=" << h.header.tick << "  installed=" << ret.installed << "  standing=" << standing
              << "  aligned_checks=" << ret.channel_open << "  essential_checks=" << ret.necessary
              << "  immediate=" << ret.immediate << "  water=" << ret.water << "  carbon=" << ret.transport
              << "  eaten=" << ret.eaten << "  foreign_meals=" << ret.other_events
              << "  foreign_carbon=" << ret.other_food << "  prebirth_meals=" << ret.older_events
              << "  prebirth_carbon=" << ret.older_food << "  unthreaded=" << ret.stripped << '\n';
    if (h.learning.mode & practice::families)
        for (Word i = 0; i < 2; ++i)
            std::cout << "practice_lineage=" << i << "  engaged=" << practice::entitled(h.learning.mode, i)
                      << "  living=" << families[i] << '\n';
    if (o.practice_test || o.practice_speed || !o.practice_scholar.empty())
        practice_exams(h, o, genes);
    if (o.practice_survey) {
        Word side = h.header.configuration.side, dim = side / 2, bins = std::min(dim, 32u), span = dim / bins;
        const char digits[] = "0123456789abcdef";
        for (Word y = 0; y < bins; ++y) {
            std::string line;
            for (Word x = 0; x < bins; ++x) {
                Word attended = 0, marked = 0, experienced = 0;
                for (Word j = 0; j < span; ++j)
                    for (Word i = 0; i < span; ++i) {
                        Word tx = x * span + i, ty = y * span + j, tile = ty * dim + tx;
                        marked += !!h.strands[tile].who;
                        for (Word k = 0; k < 4; ++k) {
                            Word id = h.slots[(ty * 2 + (k >> 1)) * side + tx * 2 + (k & 1)];
                            if (h.bodies[id].energy) {
                                auto &p = h.pupils[id];
                                attended += practice::stays(p, tile);
                                experienced += p.samples >= 8;
                            }
                        }
                    }
                for (Word v : {attended, marked, experienced}) {
                    line += digits[(v >> 8) & 15];
                    line += digits[(v >> 4) & 15];
                    line += digits[v & 15];
                }
            }
            std::cout << "practice_ground=" << y << "  bins=" << bins << "  counts=" << line << '\n';
        }
        for (Word bin = 0; bin < 5; ++bin) {
            Word count = 0, active = 0;
            Count samples = 0, stays = 0;
            for (Word i = 0; i < n; ++i)
                if (h.bodies[i].energy) {
                    Word age = h.header.tick - h.bodies[i].born;
                    Word band = age < 4 ? 0 : age < 16 ? 1 : age < 64 ? 2 : age < 256 ? 3 : 4;
                    if (band != bin)
                        continue;
                    auto &m = h.pupils[i];
                    ++count;
                    active += !!m.samples;
                    samples += m.samples;
                    stays += !!m.focus;
                }
            std::cout << "practice_age=" << bin << "  bodies=" << count << "  experienced=" << active
                      << "  contacts=" << samples << "  attending=" << stays << '\n';
        }
    }
}
void practice_exams(const Habitat &h, const Options &o, const std::vector<practice::Genes> &genes) {
    Word ng = genes.size(), ntrials = o.practice_test, seed = mix(o.practice_seed + 33191), cap = h.learning.arena,
         size = o.link_width;
    if (!ng)
        return;
    Temporary source(s.d, Count(ng) * sizeof(genes[0]));
    s.d.check(s.d.upload(source.ptr, genes.data(), Count(ng) * sizeof(genes[0])),
              "place sampled juvenile genes in controlled fixtures");
    if (o.practice_speed)
        practice_benchmark(source.ptr, ng, o.practice_speed, seed, size, cap);
    if (ntrials) {
        Function f{};
        s.bind(f, "practice_trials", Image::siphons);
        Temporary results(s.d, Count(ntrials) * sizeof(practice::Score)), proof(s.d, 4 * sizeof(practice::Specimen));
        std::vector<practice::Score> rows(ntrials);
        Temporary daily(s.d, Count(ntrials) * sizeof(practice::Day));
        std::vector<practice::Day> days(ntrials);
        Word island = o.practice_island;
        Function day{};
        s.bind(day, "practice_patch_trials", Image::siphons);
        const char *treatments[]{"free", "periodic", "yoked", "unconditioned", "displaced", "one_contact"};
        for (Word arm = 0; arm < 6; ++arm) {
            launch(day, (ntrials + 63) / 64, 64, source.ptr, ng, ntrials, seed, arm, cap, island, daily.ptr);
            synchronize();
            s.d.check(s.d.download(days.data(), daily.ptr, days.size() * sizeof(days[0])),
                      "read finite foraging resources");
            std::array<Count, sizeof(practice::Day) / 4> sum{};
            for (auto &d : days) {
                auto v = std::bit_cast<std::array<Word, sum.size()>>(d);
                for (size_t j = 0; j < v.size(); ++j)
                    sum[j] += v[j];
            }
            if (sum[1] + sum[5] != sum[6] + sum[7] || sum[20] != sum[21] + sum[5])
                throw std::logic_error("unbalanced foraging day");
            std::cout << "practice_foraging=" << treatments[arm] << "  lives=" << sum[0] << "  initial=" << sum[1]
                      << "  departed=" << sum[2] << "  strokes=" << sum[3] << "  fittings=" << sum[4]
                      << "  eaten=" << sum[5] << "  heat=" << sum[6] << "  endowment=" << sum[7]
                      << "  spring_meals=" << sum[8] << "  grazing=" << sum[9] << "  opened=" << sum[10]
                      << "  intended=" << sum[11] << "  accurate=" << sum[12] << "  longest_sum=" << sum[13]
                      << "  survived=" << sum[14] << "  stayed=" << sum[15] << "  fed_stays=" << sum[16]
                      << "  dry_stays=" << sum[17] << "  edits=" << sum[18] << "  ticks=" << sum[19]
                      << "  food_stock=" << sum[20] << "  remaining=" << sum[21]
                      << "  landscape=" << (island ? "island" : "frontier") << '\n';
        }
        Function exposure{};
        s.bind(exposure, "practice_windows", Image::siphons);
        std::vector<practice::Windows> twins(ntrials);
        Temporary windows(s.d, Count(ntrials) * sizeof(twins[0]));
        launch(exposure, (ntrials + 63) / 64, 64, source.ptr, ng, ntrials, seed, cap, island, windows.ptr);
        synchronize();
        s.d.check(s.d.download(twins.data(), windows.ptr, twins.size() * sizeof(twins[0])),
                  "compare swapped attention timing at identical exposure");
        std::array<Count, sizeof(practice::Windows) / 4> totals{};
        for (auto &r : twins) {
            if (r.own_stays != r.swapped_stays || 96 + r.own_food != r.own_reserve + r.own_work ||
                96 + r.swapped_food != r.swapped_reserve + r.swapped_work)
                throw std::logic_error("mismatched foraging exposure");
            auto a = std::bit_cast<std::array<Word, totals.size()>>(r);
            for (size_t j = 0; j < a.size(); ++j)
                totals[j] += a[j];
        }
        std::cout << "practice_window=" << ntrials << "  ticks_each=" << totals[0] << "  own_stays=" << totals[1]
                  << "  swapped_stays=" << totals[2] << "  own_food=" << totals[3] << "  swapped_food=" << totals[4]
                  << "  own_heat=" << totals[5] << "  swapped_heat=" << totals[6] << "  own_reserve=" << totals[7]
                  << "  swapped_reserve=" << totals[8] << "  own_meals=" << totals[9]
                  << "  swapped_meals=" << totals[10] << "  wins=" << totals[11] << "  losses=" << totals[12]
                  << "  landscape=" << (island ? "island" : "frontier") << '\n';
        const char *names[]{"intact",        "no_report",   "no_probe", "unexposed",   "pooled",     "wrong_report",
                            "random_handle", "one_climate", "self",     "exploratory", "one_stroke", "last_only"};
        for (Word arm = 0; arm < practice::exam_count; ++arm) {
            launch(f, (ntrials + 63) / 64, 64, source.ptr, ng, ntrials, seed, size, arm, cap, results.ptr, proof.ptr);
            synchronize();
            s.d.check(s.d.download(rows.data(), results.ptr, rows.size() * sizeof(rows[0])),
                      "audit independent conditional realization");
            std::array<Count, sizeof(practice::Score) / 4> sum{};
            for (auto &r : rows) {
                auto w = std::bit_cast<std::array<Word, sum.size()>>(r);
                for (size_t i = 0; i < w.size(); ++i)
                    sum[i] += w[i];
            }
            std::cout << "practice_trial=" << names[arm] << "  cases=" << sum[0] << "  possible=" << sum[1]
                      << "  delivered=" << sum[2] << "  offered=" << sum[3] << "  abstained=" << sum[4]
                      << "  mistaken=" << sum[5] << "  paid=" << sum[6] << "  harvest=" << sum[7]
                      << "  inquiries=" << sum[8] << "  reports=" << sum[9] << "  indirect=" << sum[10]
                      << "  branching=" << sum[11] << "  followed=" << sum[12] << "  shared=" << sum[13]
                      << "  memories=" << sum[15] << "  changed=" << sum[16] << "  fitted=" << sum[17]
                      << "  exposure=" << sum[18] << "  switches=" << sum[14] << "  first_stroke_enough=" << sum[19]
                      << '\n';
            if (!arm) {
                std::array<practice::Specimen, 4> samples{};
                s.d.check(s.d.download(samples.data(), proof.ptr, std::min(4u, ntrials) * sizeof(samples[0])),
                          "read physical test examples");
                for (Word j = 0; j < std::min(4u, ntrials); ++j) {
                    auto &x = samples[j];
                    auto &r = x.request;
                    auto &q = x.script;
                    std::cout << "practice_case=" << x.seed << "  fixture=" << x.world << "  law_a=" << x.law[0]
                              << "  law_b=" << x.law[1] << "  actual=" << x.chosen << "  parts=" << r.rig.parts
                              << "  glyphs=" << r.rig.glyphs << "  joints=" << r.rig.joints
                              << "  grip=" << r.rig.handles << "  start=" << r.view << "  target=" << r.goal.faces
                              << "  care=" << r.goal.care << "  sockets=" << r.sockets << "  budget=" << r.work
                              << "  probe=" << q.ask << "  watch=" << q.look << "  responses=" << q.outcomes
                              << "  heard=" << x.report << "  final=" << x.final << "  found=" << q.found << '\n';
                    for (Word b = 0; b < 4; ++b)
                        if (q.outcomes & (1u << b)) {
                            auto &a = q.reply[b];
                            std::cout << "practice_branch=" << x.seed << "  face=" << b << "  fitting=" << a.edit
                                      << "  strokes=" << a.turns.len << "  low=" << a.turns.low
                                      << "  high=" << a.turns.high << '\n';
                        }
                }
            }
        }
    }
    if (!o.practice_scholar.empty()) {
        Function f{};
        s.bind(f, "practice_candidates", Image::siphons);
        Temporary scores(s.d, ng * 4);
        std::vector<Word> points(ng);
        launch(f, (ng + 63) / 64, 64, source.ptr, ng, seed, cap, scores.ptr);
        synchronize();
        s.d.check(s.d.download(points.data(), scores.ptr, ng * 4), "select on food earned in novel apparatuses");
        Word best = std::max_element(points.begin(), points.end()) - points.begin(), index = 0, id = 0;
        for (; id < n; ++id)
            if (h.bodies[id].energy && index++ == best)
                break;
        auto animal = extract_individual(h, id, false);
        if (!o.practice_carrier.empty()) {
            animal.read(o.practice_carrier);
            if (!animal.invents)
                throw std::invalid_argument("recipient needs a portable material cortex");
        }
        animal.practises = true;
        animal.training = {};
        animal.training.arena = cap;
        animal.training.mind.genes = genes[best];
        storage::write_checked(o.practice_scholar, animal);
        std::cout << "practice_scholar=" << id << "  screened=" << ng << "  paid_trials=32  reserve=" << points[best]
                  << "  carrier=" << !o.practice_carrier.empty() << '\n';
    }
}
void practice_benchmark(DevicePtr genes, Word ng, Word ncall, Word seed, Word width, Word cap) {
    Word number = std::min(1024u, ng), groups = (number + 127) / 128, chunk = std::min(1u << 16, ncall);
    Temporary panels(s.d, Count(number) * sizeof(practice::Panel)),
        records(s.d, Count(number) * sizeof(practice::Card)), at(s.d, number * sizeof(Half)),
        lengths(s.d, groups * 4), size(s.d, 4), data(s.d, number * 32), wrong(s.d, 4), a(s.d, Count(chunk) * 8),
        b(s.d, Count(chunk) * 8);
    Function prep{}, scan{}, offsets{}, pack{}, fast{}, online{}, cmp{};
    s.bind(prep, "practice_panels", Image::siphons);
    s.bind(scan, "practice_sizes", Image::siphons);
    s.bind(offsets, "practice_offsets", Image::siphons);
    s.bind(pack, "practice_pack", Image::siphons);
    s.bind(fast, "practice_frozen", Image::siphons);
    s.bind(online, "practice_online", Image::siphons);
    s.bind(cmp, "hollow_compare", Image::siphons);
    launch(prep, (number + 63) / 64, 64, genes, ng, number, seed, width, cap, panels.ptr);
    synchronize();
    s.d.check(s.d.zero(wrong.ptr, 0, 4), "clear mismatched branch count");
    cuda::Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "allocate decision clock");
    s.d.check(s.d.event_new(&end, 0), "allocate completion clock");
    std::array<float, 3> sample[5]{};
    Function expanded{}, compact_effect{};
    s.bind(expanded, "practice_full_effect", Image::siphons);
    s.bind(compact_effect, "practice_local_effect", Image::siphons);
    for (Word turn = 0; turn < 4; ++turn) {
        auto time = [&](Word index, auto action) {
            s.d.check(s.d.record(start, stream), "begin device timing");
            action();
            s.d.check(s.d.record(end, stream), "end device timing");
            s.d.check(s.d.wait(end), "synchronize decision");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, start, end), "measure conditional control");
            if (turn)
                sample[index][turn - 1] += ms;
        };
        time(2, [&] {
            launch(scan, groups, 128, panels.ptr, number, records.ptr, at.ptr, lengths.ptr);
            launch(offsets, 1, 1, lengths.ptr, groups, size.ptr);
            launch(pack, groups, 128, records.ptr, number, at.ptr, lengths.ptr, data.ptr);
        });
        Word key = (seed + turn) % 5;
        for (Word first = 0; first < ncall; first += chunk) {
            Word count = std::min(ncall - first, chunk);
            for (Word j = 0; j < 2; ++j) {
                Word kind = (j + turn) & 1;
                time(kind, [&] {
                    auto &dst = kind ? b : a;
                    launch(kind ? fast : online, (count + 63) / 64, 64, panels.ptr, number, at.ptr, lengths.ptr,
                           data.ptr, count, first, key, dst.ptr);
                });
            }
            launch(cmp, (count + 127) / 128, 128, a.ptr, b.ptr, count, wrong.ptr);
            for (Word j = 0; j < 2; ++j) {
                Word kind = (j + turn) & 1;
                auto &dst = kind ? b : a;
                time(3 + kind, [&] {
                    launch(kind ? compact_effect : expanded, (count + 127) / 128, 128, panels.ptr, number, count,
                           first, key, dst.ptr);
                });
            }
            launch(cmp, (count + 127) / 128, 128, a.ptr, b.ptr, count, wrong.ptr);
        }
    }
    synchronize();
    s.d.event_free(start);
    s.d.event_free(end);
    Word errors = 0, bytes = 0;
    s.d.check(s.d.download(&errors, wrong.ptr, 4), "check every action word");
    s.d.check(s.d.download(&bytes, size.ptr, 4), "measure the packed image");
    if (errors || bytes > 32 * number)
        throw std::logic_error("invalid conditional action image");
    std::vector<practice::Card> emitted(number);
    s.d.check(s.d.download(emitted.data(), records.ptr, number * sizeof(emitted[0])),
              "count meaningful conditional programs");
    Count fixed = 0, variable = 0, silent = 0, arms = 0;
    for (auto &e : emitted) {
        Word root = e.bytes[0] & 15;
        fixed += root == 8;
        silent += root == 15;
        variable += root < 8;
        arms += root < 8 ? std::popcount(unsigned(e.bytes[1])) : 0;
    }
    for (auto &t : sample)
        std::ranges::sort(t);
    std::cout << "practice_forecast=" << number << "  cases=" << ncall << "  matched=" << Count(ncall) * 4
              << "  differences=" << errors << "  chart_ms=" << sample[3][1] << "  contact_ms=" << sample[4][1]
              << "  Mforecasts_per_s=" << ncall / (1000. * sample[4][1]) << '\n';
    std::cout << "practice_image=" << number << "  parts=" << width << "  conditional=" << variable
              << "  continuations=" << arms << "  fixed=" << fixed << "  refused=" << silent
              << "  image_bytes=" << bytes << "  directory_bytes=" << (number * sizeof(Half) + groups * 4)
              << "  queries=" << ncall << "  checked=" << Count(ncall) * 4 << "  mismatches=" << errors
              << "  compile_ms=" << sample[2][1] << "  generating_ms=" << sample[0][1]
              << "  reading_ms=" << sample[1][1] << "  Mbranches_per_s=" << ncall / (1000. * sample[1][1]) << '\n';
}
