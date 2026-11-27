void reset_links(const linkage::Totals &v) {
    s.d.check(s.d.zero(link_books.ptr, 0, Count((n / 4 + 127) / 128) * sizeof(v)), "clear machine ledgers");
    s.d.check(s.d.upload(link_books.ptr, &v, sizeof(v)), "restore machine ledgers");
}
void rehouse_works(const Habitat &soil, Habitat &h) {
    h.millwork.totals = {};
    reset_links(h.millwork.totals);
    if (soil.linking) {
        auto prior = soil.seals;
        for (auto &p : prior)
            if (p.maker) {
                p.maker = ~0u;
                p.born = 0;
                p.dated = ~0u;
            }
        s.d.check(s.d.upload(patents.ptr, prior.data(), prior.size() * sizeof(prior[0])),
                  "transplant public physical provenance");
    } else
        s.d.check(s.d.zero(patents.ptr, 0, Count(n / 4) * sizeof(linkage::Provenance)),
                  "clear provenance on replaced soil");
}
void disturb_works(const Habitat &) {
    Function f{};
    s.bind(f, "linkage_disturb", Image::floodplain);
    Word sites = n / 4;
    launch(f, (sites + 127) / 128, 128, world, c, sites);
    synchronize();
}
void link_rear(Habitat &h, const Options &o) {
    if (!o.link_epochs && o.link_origin.empty() && o.link_law.empty())
        return;
    sample(h);
    std::vector<linkage::Genes> donors;
    Word seed = o.link_seed, adult = o.link_adults, paid = o.link_paid, cap = h.millwork.arena;
    Word graft = 0;
    if (!o.link_law.empty()) {
        storage::AgentImage p;
        p.read(o.link_law);
        if (!p.values_effort)
            throw std::invalid_argument("material law donor needs an effort organ");
        graft = p.prospects.mind.genes.muscle;
    }
    if (!o.link_origin.empty()) {
        Habitat origin;
        origin.read(o.link_origin);
        if (origin.linking)
            for (Word i = 0; i < origin.sites(); ++i)
                if (origin.bodies[i].energy)
                    donors.push_back(origin.makers[i].genes);
        if (donors.empty())
            throw std::invalid_argument("no living linkage germlines in that watershed");
    }
    std::vector<linkage::Family> families(adult);
    for (Word i = 0; i < adult; ++i) {
        Word k = mix(seed + i * 9421);
        auto gene = donors.empty() ? linkage::founder(k) : donors[k % donors.size()];
        if (!o.link_law.empty())
            gene.touch = (gene.touch & ~0xfffffu) | (graft & 0xfffffu);
        families[i] = {gene, 0};
    }
    if (o.link_epochs) {
        Count bytes = Count(adult) * sizeof(families[0]);
        Temporary first(s.d, bytes), next(s.d, bytes);
        s.d.check(s.d.upload(first.ptr, families.data(), bytes), "place juvenile mechanism germlines");
        Function eat{}, born{};
        s.bind(eat, o.link_climate ? "linkage_changing_food" : "linkage_feed", Image::siphons);
        Word climate = o.link_climate;
        s.bind(born, "linkage_births", Image::siphons);
        for (Word t = 0; t < o.link_epochs; ++t) {
            if (climate)
                launch(eat, (adult + 127) / 128, 128, first.ptr, adult, seed, t, cap, climate);
            else
                launch(eat, (adult + 127) / 128, 128, first.ptr, adult, seed, t, cap);
            if (!t || (t + 1) % 64 == 0 || t + 1 == o.link_epochs) {
                synchronize();
                s.d.check(s.d.download(families.data(), first.ptr, bytes), "count actual egg budgets");
                Count eggs = 0;
                Word best = 0;
                for (auto &f : families) {
                    eggs += f.eggs;
                    best = std::max(best, f.eggs);
                }
                std::cout << "linkage_season=" << t + 1 << "  parents=" << adult << "  eggs=" << eggs
                          << "  largest=" << best << "  earned=" << paid << '\n';
            }
            launch(born, (adult / 2 + 127) / 128, 128, first.ptr, next.ptr, adult, seed, t, paid);
            std::swap(first.ptr, next.ptr);
        }
        synchronize();
        s.d.check(s.d.download(families.data(), first.ptr, bytes),
                  "read the new generation of component learning laws");
    }
    for (Word i = 0; i < n; ++i)
        h.makers[i] = {families[mix(seed + i * 4507) & (adult - 1)].g};
    s.d.check(s.d.upload(inventors.ptr, h.makers.data(), Count(n) * sizeof(linkage::Mind)),
              "place newborn causal organs");
    s.d.check(s.d.zero(links.ptr, 0, Count(n) * cap * sizeof(linkage::Tissue)), "clear private acquired contacts");
    std::fill(h.mechanisms.begin(), h.mechanisms.end(), linkage::Tissue{});
}
void link_study(const Habitat &h, const Options &o) {
    if (!h.linking)
        return;
    std::vector<linkage::Genes> genes;
    std::vector<Word> ids;
    Word observing = 0;
    Count contacts = 0, banks = 0, issued = 0;
    std::array<Count, 2> lineages{};
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy) {
            auto &m = h.makers[i];
            genes.push_back(m.genes);
            ids.push_back(i);
            observing += !!m.tags;
            contacts += m.contacts;
            for (Word j = 0; j < 4; ++j)
                banks += !!linkage::key_at(m, j);
            ++lineages[bool(h.wanderers[i].efferent & hollow::lineage_tag)];
        }
    for (auto &p : h.seals)
        issued += p.maker != 0;
    Word ng = genes.size(), tasks = o.link_test, seed = mix(o.link_seed + 111331), width = o.link_width,
         lessons = o.link_practice, cap = h.millwork.arena;
    if (!o.link_carrier.empty() && o.link_scholar.empty())
        throw std::invalid_argument("an organ recipient requires a saved scholar");
    if (!ng && !o.link_scholar.empty())
        throw std::invalid_argument("an empty watershed has no scholar");
    if (ng && (tasks || !o.link_scholar.empty() || o.link_witness || o.link_recall)) {
        Temporary parents(s.d, Count(ng) * sizeof(genes[0]));
        s.d.check(s.d.upload(parents.ptr, genes.data(), Count(ng) * sizeof(genes[0])),
                  "place the actual heritable causal circuits");
        if (o.link_recall) {
            Word cases = o.link_recall;
            Temporary visits(s.d, Count(cases) * 15 * 4);
            std::vector<std::array<Word, 15>> stages(cases);
            Function recall{};
            s.bind(recall, "linkage_changed", Image::siphons);
            const char *names[]{"", "steady", "changing", "no_new_evidence", "cleared_each_time"};
            for (Word mode = 1; mode <= 4; ++mode) {
                launch(recall, (cases + 63) / 64, 64, parents.ptr, ng, cases, seed, cap, mode, width, visits.ptr);
                synchronize();
                s.d.check(s.d.download(stages.data(), visits.ptr, stages.size() * sizeof(stages[0])),
                          "observe actual physical work after a change of material dynamics");
                std::array<Count, 15> sums{};
                for (auto &row : stages)
                    for (Word i = 0; i < 15; ++i)
                        sums[i] += row[i];
                for (Word season = 0; season < 3; ++season) {
                    const auto *z = sums.data() + 5 * season;
                    std::cout << "linkage_recall=" << names[mode] << "  season=" << season << "  cases=" << 2 * cases
                              << "  completed=" << z[0] << "  proposed=" << z[1] << "  heat=" << z[2]
                              << "  built=" << z[3] << "  mistaken=" << z[4] << '\n';
                }
            }
        }
        if (tasks) {
            Temporary background(s.d, Count(tasks) * sizeof(linkage::Peripheral));
            Function f{};
            s.bind(f, "linkage_periphery", Image::siphons);
            std::vector<linkage::Peripheral> peripheral(tasks);
            const char *scopes[]{"whole", "heritable", "focal", "all_requested"};
            for (Word arm = 0; arm < 4; ++arm) {
                launch(f, (tasks + 63) / 64, 64, parents.ptr, ng, tasks, seed, cap, arm, background.ptr);
                synchronize();
                s.d.check(s.d.download(peripheral.data(), background.ptr, peripheral.size() * sizeof(peripheral[0])),
                          "test work near unfamiliar passive parts");
                std::array<Count, 8> sum{};
                for (auto &p : peripheral) {
                    auto row = std::bit_cast<std::array<Word, 8>>(p);
                    for (Word j = 0; j < 8; ++j)
                        sum[j] += row[j];
                }
                std::cout << "linkage_attention=" << scopes[arm] << "  trials=" << sum[0] << "  inherited=" << sum[1]
                          << "  planned=" << sum[2] << "  achieved=" << sum[3] << "  heat=" << sum[4]
                          << "  false=" << sum[5] << "  minimal=" << sum[6] << "  changed=" << sum[7] << '\n';
            }
            Temporary results(s.d, Count(tasks) * sizeof(linkage::Trial)), detail(s.d, 3 * sizeof(linkage::Receipt));
            Function probe{};
            s.bind(probe, "linkage_exams", Image::siphons);
            std::vector<linkage::Trial> rows(tasks);
            const char *names[]{"own",
                                "watched",
                                "unexposed",
                                "local_erased",
                                "seams_erased",
                                "seams_exchanged",
                                "sight_covered",
                                "pairs_only",
                                "prior_gates",
                                "no_grip",
                                "one_context",
                                "fixed",
                                "displaced_witness",
                                "self_directed",
                                "assumed_loose"};
            for (Word mode = 0; mode < linkage::school_count; ++mode) {
                launch(probe, (tasks + 127) / 128, 128, parents.ptr, ng, tasks, width, seed, mode, lessons, cap,
                       results.ptr, detail.ptr);
                synchronize();
                s.d.check(s.d.download(rows.data(), results.ptr, Count(tasks) * sizeof(rows[0])),
                          "read the self-generated work sequences");
                std::array<Count, sizeof(linkage::Trial) / 4> sum{};
                for (auto &r : rows) {
                    auto v = std::bit_cast<std::array<Word, sum.size()>>(r);
                    for (size_t j = 0; j < v.size(); ++j)
                        sum[j] += v[j];
                }
                std::cout << "linkage_transfer=" << names[mode] << "  parts=" << width << "  cases=" << sum[0]
                          << "  built=" << sum[1] << "  possible=" << sum[2] << "  silent=" << sum[3]
                          << "  mistaken=" << sum[4] << "  paid=" << sum[5] << "  shortest=" << sum[6]
                          << "  food=" << sum[7] << "  minimum=" << sum[8] << "  plans=" << sum[9]
                          << "  indirect=" << sum[10] << "  indirect_built=" << sum[11] << "  long=" << sum[12]
                          << "  long_built=" << sum[13] << "  elementary=" << sum[15] << "  expanded=" << sum[16]
                          << "  banks=" << sum[18] << "  predictions=" << sum[20] << "  exact=" << sum[21]
                          << "  voiced=" << sum[22] << '\n';
                if (mode == linkage::own) {
                    std::array<linkage::Receipt, 3> receipts{};
                    s.d.check(s.d.download(receipts.data(), detail.ptr, std::min(3u, tasks) * sizeof(receipts[0])),
                              "recover three original mechanisms");
                    for (Word i = 0; i < std::min(3u, tasks); ++i) {
                        auto &q = receipts[i];
                        std::cout << "linkage_scene=" << q.seed << "  parts=" << q.parts << "  start=" << q.start
                                  << "  target=" << q.desired << "  care=" << q.care << "  joints=" << q.joints
                                  << "  materials=" << q.glyphs << "  handles=" << q.handles
                                  << "  finish=" << q.finish << "  low=" << q.plan.low << "  high=" << q.plan.high
                                  << "  length=" << q.plan.len << "  solved=" << q.reached
                                  << "  minimum=" << q.shortest << "  claimed=" << q.claims << "  depth=" << q.depth
                                  << "  motor=" << q.g.impulse << '\n';
                        for (Word g = 0; g < 4; ++g)
                            std::cout << "linkage_mechanism=" << q.seed << "  material=" << g
                                      << "  true_local=" << q.true_next[g] << "  true_mesh=" << q.true_links[0][g]
                                      << "  true_belt=" << q.true_links[1][g] << "  learned_local=" << q.map.next[g]
                                      << "  learned_mesh=" << q.map.link[0][g]
                                      << "  learned_belt=" << q.map.link[1][g] << '\n';
                    }
                }
            }
        }
        if (o.link_witness)
            link_culture(parents.ptr, ng, o.link_witness, width, seed, cap, lessons);
        if (tasks && width >= 2)
            link_assembly(parents.ptr, ng, tasks, width, seed, cap, lessons);
        if (!o.link_scholar.empty()) {
            Temporary out(s.d, Count(ng) * 4);
            Function f{};
            s.bind(f, o.link_climate ? "linkage_revisers" : "linkage_candidates", Image::siphons);
            launch(f, (ng + 127) / 128, 128, parents.ptr, ng, seed, cap, out.ptr);
            synchronize();
            std::vector<Word> score(ng);
            s.d.check(s.d.download(score.data(), out.ptr, Count(ng) * 4), "retrieve paid-work candidate screens");
            Word best = ~0u;
            for (Word i = 0; i < ng; ++i)
                if (!o.link_carrier.empty() || (h.bodies[ids[i]].body.eyes && h.bodies[ids[i]].body.limbs))
                    if (best == ~0u || score[i] > score[best])
                        best = i;
            if (best == ~0u)
                throw std::invalid_argument("no resident can see and handle the jig");
            auto pupil = extract_individual(h, ids[best], false);
            if (!o.link_carrier.empty()) {
                pupil.read(o.link_carrier);
                if (!pupil.ranges)
                    throw std::invalid_argument("recipient needs a portable watershed body and sensory tissue");
                pupil.invents = true;
                pupil.mechanism = {};
                pupil.mechanism.arena = cap;
                pupil.mechanism.mind.genes = genes[best];
            }
            storage::write_checked(o.link_scholar, pupil);
            std::cout << "linkage_scholar=" << ids[best] << "  screened=" << ng
                      << "  work_samples=" << (o.link_climate ? 48 : 36) << "  meals_less_work=" << score[best]
                      << "  carrier=" << !o.link_carrier.empty() << '\n';
        }
    }
    if (o.link_census && !h.seals.empty()) {
        // Preserve each bin's actual count of certified openings in the final map.
        const Word side = h.header.configuration.side / 2, bins = std::min(side, 32u), span = side / bins;
        const char digits[] = "0123456789abcdef";
        for (Word y = 0; y < bins; ++y) {
            std::string row;
            for (Word x = 0; x < bins; ++x) {
                Word seals = 0, actors = 0;
                for (Word yy = y * span; yy < (y + 1) * span; ++yy)
                    for (Word xx = x * span; xx < (x + 1) * span; ++xx) {
                        seals += !!h.seals[yy * side + xx].maker;
                        for (Word q = 0; q < 4; ++q) {
                            auto pos = (2 * yy + (q >> 1)) * h.header.configuration.side + 2 * xx + (q & 1);
                            auto id = h.slots[pos];
                            actors += h.bodies[id].energy && h.makers[id].contacts;
                        }
                    }
                for (Word v : {seals, actors}) {
                    row += digits[(v >> 8) & 15];
                    row += digits[(v >> 4) & 15];
                    row += digits[v & 15];
                }
            }
            std::cout << "linkage_ground=" << y << "  bins=" << bins << "  span=" << span << "  counts=" << row
                      << '\n';
        }
    }
    const auto &b = h.millwork.totals;
    std::cout << "linkage_field=" << h.header.tick << "  living=" << ng << "  remembering=" << observing
              << "  contexts=" << banks << "  contact_history=" << contacts << "  current_seals=" << issued
              << "  considered=" << b.active << "  searches=" << b.attempts << "  proposed=" << b.invented
              << "  planned_finishes=" << b.solved << "  taps=" << b.taps << "  propagated=" << b.transmitted
              << "  heat=" << b.heat << "  probes=" << b.explore << "  seen=" << b.seen
              << "  private_edits=" << b.self_changes << "  observed_edits=" << b.public_changes
              << "  carbon=" << b.flow << "  meals=" << b.eaten << "  loose=" << b.loose << "  seals=" << b.stamped
              << "  maker_meals=" << b.own_meal << "  other_meals=" << b.other_meal
              << "  older_meals=" << b.older_meal << "  other_carbon=" << b.other_carbon
              << "  loops_placed=" << b.loops_in << "  loops_lifted=" << b.loops_out
              << "  structural_plans=" << b.designs << "  fitting_edits=" << b.join_edits
              << "  joined_openings=" << b.certified_joins << "  older_carbon=" << b.older_carbon
              << "  water=" << b.water << "  arena=" << cap << '\n';
    if (h.millwork.mode & linkage::mark_families)
        for (Word i = 0; i < 2; ++i)
            std::cout << "linkage_lineage=" << i << "  planning=" << linkage::public_motor(h.millwork.mode, i)
                      << "  living=" << lineages[i] << "  tick=" << h.header.tick << '\n';
}

void link_assembly(DevicePtr genes, Word ng, Word n, Word width, Word seed, Word cap, Word lessons) {
    Temporary results(s.d, Count(n) * sizeof(linkage::Fabrication)), proof(s.d, 3 * sizeof(linkage::Drawing));
    std::vector<linkage::Fabrication> rows(n);
    Function f{};
    s.bind(f, "linkage_spools", Image::siphons);
    const char *names[]{"built",       "loop_removed", "false_splice", "splice_erased", "relay_erased", "no_stock",
                        "wrong_parts", "turns_only",   "untaught",     "witnessed",     "self_directed"};
    for (Word mode = 0; mode < linkage::build_count; ++mode) {
        launch(f, (n + 127) / 128, 128, genes, ng, n, width, seed, mode, cap, lessons, results.ptr, proof.ptr);
        synchronize();
        s.d.check(s.d.download(rows.data(), results.ptr, Count(n) * sizeof(rows[0])),
                  "audit the fitted assemblies and achieved configurations");
        std::array<Count, sizeof(linkage::Fabrication) / 4> sum{};
        for (auto &r : rows) {
            auto a = std::bit_cast<std::array<Word, sum.size()>>(r);
            for (size_t j = 0; j < a.size(); ++j)
                sum[j] += a[j];
        }
        std::cout << "linkage_fabrication=" << names[mode] << "  parts=" << width << "  cases=" << sum[0]
                  << "  produced=" << sum[1] << "  possible=" << sum[2] << "  cheapest=" << sum[3]
                  << "  threaded=" << sum[4] << "  needs_loop=" << sum[5] << "  new_function=" << sum[6]
                  << "  replaceable=" << sum[7] << "  wrong_seam=" << sum[8] << "  heat=" << sum[9]
                  << "  carbon=" << sum[10] << "  strokes=" << sum[11] << "  mistaken=" << sum[12]
                  << "  no_plan=" << sum[13] << "  without_possible=" << sum[14] << "  seam_predictions=" << sum[15]
                  << "  seam_correct=" << sum[16] << '\n';
        if (mode == linkage::assembled) {
            std::array<linkage::Drawing, 3> examples{};
            s.d.check(s.d.download(examples.data(), proof.ptr, std::min(n, 3u) * sizeof(examples[0])),
                      "read the physical inventory");
            for (Word i = 0; i < std::min(3u, n); ++i) {
                auto &q = examples[i];
                std::cout << "linkage_product=" << q.seed << "  parts=" << width << "  materials=" << q.glyphs
                          << "  before=" << q.joints << "  after=" << q.new_joints << "  sockets=" << q.sockets
                          << "  grip=" << q.grip << "  start=" << q.start << "  target=" << q.goal
                          << "  care=" << q.care << "  finish=" << q.last << "  fitting=" << q.edit
                          << "  loose=" << q.remaining << "  cost=" << q.spent << "  food=" << q.food
                          << "  strokes=" << q.plan.len << "  low=" << q.plan.low << "  high=" << q.plan.high
                          << "  optimum=" << q.optimal << "  without=" << q.untouched << '\n';
            }
        }
    }
}
void link_census(const Habitat &h, const Options &o) {
    if (!o.link_census && !o.link_bench)
        return;
    if (o.link_bench) {
        std::vector<linkage::Genes> genes;
        for (Word i = 0; i < n && genes.size() < 512; ++i)
            if (h.bodies[i].energy)
                genes.push_back(h.makers[i].genes);
        if (!genes.empty()) {
            Word count = genes.size(), size = o.link_width, seed = o.link_seed, cap = h.millwork.arena;
            Temporary germ(s.d, count * sizeof(genes[0])), views(s.d, count * sizeof(linkage::Panel));
            s.d.check(s.d.upload(germ.ptr, genes.data(), count * sizeof(genes[0])),
                      "place actual residents in new controlled fixtures");
            Function f{};
            s.bind(f, "linkage_practiced", Image::siphons);
            launch(f, (count + 63) / 64, 64, germ.ptr, count, size, seed, cap, views.ptr);
            synchronize();
            std::vector<linkage::Panel> trained(count);
            s.d.check(s.d.download(trained.data(), views.ptr, trained.size() * sizeof(trained[0])),
                      "read individually learned fixture charts");
            link_speed(trained, o.link_bench, size, seed, "practiced");
        }
    }
    Word blocks = n / 4, seed = o.link_seed, tick = h.header.tick, size = o.link_width;
    Temporary tally(s.d, Count(blocks) * sizeof(linkage::Audit)),
        frames(s.d, o.link_bench ? Count(blocks) * sizeof(linkage::Panel) : 0);
    std::vector<linkage::Audit> rows(blocks);
    Function f{};
    s.bind(f, "linkage_views", Image::floodplain);
    const char *names[]{"lived", "no_joints", "other_joint", "no_rotation"};
    for (Word mode = 0; mode < (o.link_census ? 4u : 1u); ++mode) {
        DevicePtr save = !mode ? frames.ptr : 0;
        launch(f, (blocks + 127) / 128, 128, world, c, seed, tick, mode, size, save, tally.ptr);
        synchronize();
        s.d.check(s.d.download(rows.data(), tally.ptr, rows.size() * sizeof(rows[0])),
                  "read independently tested public mechanisms");
        std::array<Count, sizeof(linkage::Audit) / 4> t{};
        for (auto &r : rows) {
            auto a = std::bit_cast<std::array<Word, t.size()>>(r);
            for (size_t j = 0; j < t.size(); ++j)
                t[j] += a[j];
        }
        if (!mode && o.link_census && (h.millwork.mode & linkage::hydration)) {
            Word half = c.side / 2, bins = std::min(half, 32u), span = half / bins;
            const char digits[] = "0123456789abcdef";
            for (Word y = 0; y < bins; ++y) {
                std::string water;
                for (Word x = 0; x < bins; ++x) {
                    Word count = 0;
                    for (Word j = 0; j < span; ++j)
                        for (Word i = 0; i < span; ++i)
                            count += rows[(y * span + j) * half + x * span + i].hydrated;
                    water += digits[(count >> 8) & 15];
                    water += digits[(count >> 4) & 15];
                    water += digits[count & 15];
                }
                std::cout << "linkage_moisture=" << y << "  bins=" << bins << "  span=" << span
                          << "  counts=" << water << '\n';
            }
        }
        std::cout << "linkage_inspection=" << names[mode] << "  plots=" << blocks << "  residents=" << t[0]
                  << "  pieces=" << t[1] << "  supplied=" << t[2] << "  working=" << t[3] << "  tasks=" << t[4]
                  << "  proposals=" << t[6] << "  produced=" << t[7] << "  multistep=" << t[8] << "  heat=" << t[9]
                  << "  forecast=" << t[10] << "  confident=" << t[11] << "  correct=" << t[12] << "  seals=" << t[13]
                  << "  hydrated_plots=" << t[15] << "  settled=" << t[14] << '\n';
    }
    if (o.link_bench) {
        std::vector<linkage::Panel> all(blocks), chosen;
        s.d.check(s.d.download(all.data(), frames.ptr, all.size() * sizeof(all[0])), "read live material programs");
        for (auto &v : all)
            if (v.owner != linkage::absent)
                chosen.push_back(v);
        if (chosen.empty()) {
            std::cout << "linkage_compile=vacant\n";
            return;
        }
        link_speed(chosen, o.link_bench, size, seed, "wild");
    }
}
void link_speed(const std::vector<linkage::Panel> &panels, Word count, Word size, Word seed, const char *origin) {
    Word ng = panels.size(), states = 1u << (2 * size), batch = std::min(1u << 18, count);
    Temporary charts(s.d, ng * sizeof(panels[0])), images(s.d, Count(ng) * states / 2), a(s.d, Count(batch) * 8),
        b(s.d, Count(batch) * 8), errors(s.d, 4);
    s.d.check(s.d.upload(charts.ptr, panels.data(), ng * sizeof(panels[0])), "upload learned views");
    s.d.check(s.d.zero(errors.ptr, 0, 4), "clear program discrepancies");
    Function build{}, writable{}, fast{}, cmp{};
    s.bind(build, "linkage_compile", Image::siphons);
    s.bind(writable, "linkage_writable_plans", Image::siphons);
    s.bind(fast, "linkage_read_plans", Image::siphons);
    s.bind(cmp, "hollow_compare", Image::siphons);
    cuda::Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "start mechanism clock");
    s.d.check(s.d.event_new(&end, 0), "stop mechanism clock");
    std::array<float, 5> times[3]{};
    for (Word j = 0; j < 6; ++j) {
        auto measure = [&](Word ix, auto action) {
            s.d.check(s.d.record(start, stream), "start device interval");
            action();
            s.d.check(s.d.record(end, stream), "finish device interval");
            s.d.check(s.d.wait(end), "synchronize interval");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, start, end), "time linked actions");
            if (j)
                times[ix][j - 1] += ms;
        };
        measure(2, [&] { launch(build, (ng + 3) / 4, 128, charts.ptr, ng, states, images.ptr); });
        Word draw = mix(seed + 41 * j);
        for (Word off = 0; off < count; off += batch) {
            Word len = std::min(count - off, batch), grid = (len + 127) / 128;
            for (Word order = 0; order < 2; ++order) {
                Word quick = (j + order) & 1;
                auto &out = quick ? b : a;
                measure(quick, [&] {
                    launch(quick ? fast : writable, grid, 128, charts.ptr, images.ptr, ng, states, len, off, draw,
                           out.ptr);
                });
            }
            launch(cmp, grid, 128, a.ptr, b.ptr, len, errors.ptr);
        }
    }
    synchronize();
    s.d.event_free(start);
    s.d.event_free(end);
    Word wrong = 0;
    s.d.check(s.d.download(&wrong, errors.ptr, 4), "verify every emitted action, length and refusal");
    if (wrong)
        throw std::logic_error("compiled inverse action programs disagree with writable planning");
    std::vector<unsigned char> codes(Count(ng) * states / 2);
    s.d.check(s.d.download(codes.data(), images.ptr, codes.size()), "count accessible starting configurations");
    Count idle = 0, unreachable = 0, active = 0;
    for (auto value : codes)
        for (Word j = 0; j < 2; ++j) {
            Word move = (value >> (4 * j)) & 15;
            idle += move == linkage::stop;
            active += move < 8;
            unreachable += move == linkage::no_path;
        }
    if (idle + active + unreachable != Count(ng) * states)
        throw std::logic_error("invalid compiled motor population");
    for (auto &t : times)
        std::ranges::sort(t);
    std::cout << "linkage_compile=" << ng << "  parts=" << size << "  states=" << states
              << "  image_bytes=" << states / 2 << "  trials=" << count << "  checked=" << Count(count) * 6
              << "  differences=" << wrong << "  compile_ms=" << times[2][2] << "  writable_ms=" << times[0][2]
              << "  image_ms=" << times[1][2] << "  origin=" << origin << "  productive_states=" << active
              << "  already_aligned=" << idle << "  no_path=" << unreachable
              << "  Mplans_per_s=" << count / (1000. * times[1][2]) << '\n';
}
void link_culture(DevicePtr g, Word ng, Word n, Word width, Word seed, Word cap, Word practice) {
    Function pass{}, chain{};
    s.bind(pass, "linkage_handoffs", Image::siphons);
    s.bind(chain, "linkage_culture", Image::siphons);
    std::vector<linkage::Exchange> x(n);
    std::vector<linkage::Culture> y(n);
    Temporary a(s.d, n * sizeof(x[0])), b(s.d, n * sizeof(y[0]));
    const char *inherit[]{"as_made",   "unstrung", "foreign_loop", "different_goal", "unexposed_recipient",
                          "one_handle"};
    for (Word mode = 0; mode < linkage::legacy_count; ++mode) {
        launch(pass, (n + 127) / 128, 128, g, ng, n, width, seed, mode, cap, a.ptr);
        synchronize();
        s.d.check(s.d.download(x.data(), a.ptr, n * sizeof(x[0])),
                  "audit a second body working the inherited product");
        std::array<Count, sizeof(x[0]) / 4> z{};
        for (auto &r : x) {
            auto t = std::bit_cast<std::array<Word, z.size()>>(r);
            for (size_t i = 0; i < t.size(); ++i)
                z[i] += t[i];
        }
        std::cout << "linkage_bequest=" << inherit[mode] << "  cases=" << z[0] << "  possible=" << z[1]
                  << "  maker_plans=" << z[2] << "  loops=" << z[4] << "  recipient_fed=" << z[5]
                  << "  carbon=" << z[6] << "  loose=" << z[7] << "  needs_loop=" << z[8] << "  novel_uses=" << z[9]
                  << "  receiver_heat=" << z[10] << "  maker_heat=" << z[11] << "  recipient_plans=" << z[12]
                  << "  mistaken=" << z[13] << "  absent_makers=" << z[14] << '\n';
    }
    const char *names[]{"living_witness", "obscured",      "terminal_only", "wrong_handle",
                        "wrong_effect",   "no_primitives", "local_practice"};
    Word exposure = 16 * practice;
    for (Word gen : {1u, 2u, 3u})
        for (Word mode = 0; mode < linkage::culture_count; ++mode) {
            launch(chain, (n + 127) / 128, 128, g, ng, n, width, seed, mode, cap, gen, exposure, b.ptr);
            synchronize();
            s.d.check(s.d.download(y.data(), b.ptr, n * sizeof(y[0])), "question successors on unheard work orders");
            std::array<Count, sizeof(y[0]) / 4> z{};
            for (auto &r : y) {
                auto t = std::bit_cast<std::array<Word, z.size()>>(r);
                for (size_t i = 0; i < t.size(); ++i)
                    z[i] += t[i];
            }
            std::cout << "linkage_teaching=" << names[mode] << "  generation=" << gen
                      << "  two_piece_demonstrations=" << exposure << "  cases=" << z[0] << "  possible=" << z[1]
                      << "  mentor_harvests=" << z[2] << "  built=" << z[3] << "  long=" << z[4]
                      << "  long_built=" << z[5] << "  indirect=" << z[6] << "  indirect_built=" << z[7]
                      << "  demonstrated=" << z[8] << "  seen=" << z[9] << "  tutor_heat=" << z[10]
                      << "  edits=" << z[11] << "  plans=" << z[12] << "  pupil_heat=" << z[13]
                      << "  nutrition=" << z[14] << '\n';
        }
}
