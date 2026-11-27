void drift_founders(Habitat &h, const Options &o, bool mark) {
    Word stock = 0, marking = mark, seed = h.drift_water.tag_seed;
    braid::Genes genes{};
    if (!o.drift_stock.empty()) {
        if (!h.braiding)
            throw std::invalid_argument("drifting stock needs a float-hand habitat");
        storage::AgentImage source;
        source.read(o.drift_stock);
        if (!source.riverine || !source.binds_floats)
            throw std::invalid_argument("drift stock needs an embodied float operator");
        genes = source.braided.mind.genes;
        stock = 1;
    }
    Function init{};
    s.bind(init, "drift_founders", Image::floodplain);
    launch(init, (n + 127) / 128, 128, world, n, seed, marking, stock, genes);
    synchronize();
}
void drift_lineages(Habitat &h) {
    if (drift_demography.ptr) {
        std::vector<drift::Pedigree> regions((n / 2 + 127) / 128);
        s.d.check(s.d.download(regions.data(), drift_demography.ptr, regions.size() * sizeof(regions[0])),
                  "read actual drift births and deaths");
        drift::Pedigree life{};
        for (const auto &r : regions)
            for (Word g = 0; g < 3; ++g) {
                life.births[g] += r.births[g];
                life.deaths[g] += r.deaths[g];
            }
        h.drift_water.pedigree = life;
        std::vector<drift::Headcount> samples(3 * ((n + 255) / 256));
        Temporary counts(s.d, samples.size() * sizeof(samples[0]));
        s.d.check(s.d.zero(counts.ptr, 0, samples.size() * sizeof(samples[0])), "clear living drift cohorts");
        Function observe{};
        s.bind(observe, "drift_population", Image::floodplain);
        launch(observe, (n + 127) / 128, 128, world, c, h.drift_water.marked_at, counts.ptr);
        synchronize();
        s.d.check(s.d.download(samples.data(), counts.ptr, samples.size() * sizeof(samples[0])),
                  "read living drift cohorts");
        drift::Headcount heads[3]{};
        for (size_t j = 0; j < samples.size(); ++j) {
            auto &a = heads[j % 3];
            auto &b = samples[j];
            a.alive += b.alive;
            a.young += b.young;
            a.energy += b.energy;
            a.bits += b.bits;
            a.cells += b.cells;
        }
        for (Word g = 0; g < 3; ++g) {
            Count born = life.births[g] - drift_births.births[g], dead = life.deaths[g] - drift_births.deaths[g];
            if (!drift_counted)
                drift_adults[g] = heads[g].alive + dead - born;
            if (drift_adults[g] + born != heads[g].alive + dead)
                throw std::logic_error("drift lineage census disagrees with births and deaths");
            std::cout << "drift_lineage=" << g << "  tick=" << tick << "  start=" << drift_start
                      << "  label_seed=" << h.drift_water.tag_seed << "  recall=" << drift::reads(drift_law.ports, g)
                      << "  distal=" << !!(drift::branch(drift_law.ports, g) & drift::lateral)
                      << "  initial=" << drift_adults[g] << "  living=" << heads[g].alive << "  born=" << born
                      << "  died=" << dead << "  born_after_release=" << heads[g].young
                      << "  energy=" << heads[g].energy << "  cells=" << heads[g].cells
                      << "  excluded=" << heads[g].bits << "  lifetime_births=" << life.births[g]
                      << "  lifetime_deaths=" << life.deaths[g] << '\n';
        }
        drift_counted = true;
    }
    if (!drift_yields.ptr)
        return;
    constexpr Word bins = drift::populations * drift::phases;
    std::vector<drift::Intake> shards(((n / 4 + 1023) / 1024) * bins);
    s.d.check(s.d.download(shards.data(), drift_yields.ptr, shards.size() * sizeof(shards[0])),
              "read simultaneous food choices");
    std::array<drift::Intake, bins> sums{};
    for (size_t i = 0; i < shards.size(); ++i) {
        auto &a = sums[i % bins];
        auto &b = shards[i];
        a.reached += b.reached;
        a.eaten += b.eaten;
        a.without += b.without;
        a.changed += b.changed;
        a.hits += b.hits;
        a.committed += b.committed;
        a.judged += b.judged;
        a.edits += b.edits;
    }
    for (Word i = 0; i < bins; ++i) {
        const auto &a = sums[i];
        Word g = i / drift::phases;
        if ((g == 3 && !drift_sampling.has_donor) || (!a.reached && !a.edits))
            continue;
        std::cout << "drift_harvest=" << g << "  quarter=" << i % drift::phases << "  tick=" << tick
                  << "  start=" << drift_start << "  reached=" << a.reached << "  eaten=" << a.eaten
                  << "  without=" << a.without << "  changed=" << a.changed << "  caught=" << a.hits
                  << "  committed=" << a.committed << "  confirmed=" << a.judged << "  edits=" << a.edits << '\n';
    }
}
void report_drift(Habitat &h) {
    drift_lineages(h);
    if (drift_rafts.ptr) {
        std::array<drift::Cargo, 128> areas{};
        s.d.check(s.d.download(areas.data(), drift_rafts.ptr, sizeof(areas)), "read the motion of loaded carriers");
        drift::Cargo cargo{};
        for (auto &a : areas) {
            cargo.opportunities += a.opportunities;
            cargo.swaps += a.swaps;
            cargo.blocked += a.blocked;
            cargo.carbon += a.carbon;
            cargo.travelled += a.travelled;
        }
        if (cargo.opportunities != cargo.swaps + cargo.blocked || cargo.carbon > 62 * cargo.swaps ||
            cargo.travelled > 2 * cargo.swaps)
            throw std::logic_error("a passively transported carrier exceeded its physical load");
        std::cout << "drift_cargo=" << tick << "  start=" << drift_start << "  moving_paths=" << cargo.opportunities
                  << "  exchanged=" << cargo.swaps << "  too_heavy=" << cargo.blocked
                  << "  raft_carbon=" << cargo.carbon << "  beyond_origin=" << cargo.travelled << '\n';
    }
    if (!drift_books.ptr)
        return;
    std::vector<drift::Totals> groups((n / 4 + 127) / 128);
    s.d.check(s.d.download(groups.data(), drift_books.ptr, groups.size() * sizeof(groups[0])),
              "weigh suspended food and interception");
    using Accounts = std::array<Count, sizeof(drift::Totals) / sizeof(Count)>;
    Accounts total{};
    for (auto &g : groups) {
        auto v = std::bit_cast<Accounts>(g);
        for (size_t j = 0; j < v.size(); ++j)
            total[j] += v[j];
    }
    h.drift_water.total = std::bit_cast<drift::Totals>(total);
    if (!drift::balanced(h.drift_water.total))
        throw std::logic_error("riverbed diverged from its carbon or motor accounts");
    auto opening = std::bit_cast<Accounts>(drift_opening);
    for (size_t j = 0; j < total.size(); ++j)
        total[j] -= opening[j];
    auto v = std::bit_cast<drift::Totals>(total);
    if (!drift::balanced(v))
        throw std::logic_error("drifting food escaped the observation interval");
    if (drift_spills.ptr) {
        std::array<drift::Spill, 128> shards{};
        s.d.check(s.d.download(shards.data(), drift_spills.ptr, sizeof(shards)),
                  "weigh retained and overtopping pools");
        using Values = std::array<Count, sizeof(drift::Spill) / sizeof(Count)>;
        Values sum{};
        for (auto &shard : shards) {
            auto part = std::bit_cast<Values>(shard);
            for (Word j = 0; j < sum.size(); ++j)
                sum[j] += part[j];
        }
        auto h = std::bit_cast<drift::Spill>(sum);
        if (h.floods + h.trickles != v.pulses || h.flood_stock + h.trickle_stock != v.released ||
            h.flood_meals + h.trickle_meals != v.eaten || h.retained > h.inspected || h.held_by_fibres > h.retained)
            throw std::logic_error("suspended material escaped the sill accounts");
        std::cout << "inlet_storage=" << tick << "  start=" << drift_start << "  wet_pools=" << h.inspected
                  << "  held=" << h.retained << "  spills=" << h.floods << "  released=" << h.flood_stock
                  << "  swallowed=" << h.flood_meals << "  trickles=" << h.trickles << "  trickled=" << h.trickle_stock
                  << "  shallow_meals=" << h.trickle_meals << "  held_by_fibres=" << h.held_by_fibres
                  << "  changed_rims=" << h.moved_rim << '\n';
    }
    std::cout << "drift_tick=" << tick << "  start=" << drift_start << "  mode=" << drift_law.ports
              << "  tide=" << (1u << drift_law.tide_shift) << "  stride=" << (1u << drift_law.stride_shift)
              << "  maximum_load=" << drift_law.load << "  pulses=" << v.pulses << "  released=" << v.released
              << "  eaten=" << v.eaten << "  settled=" << v.settled << "  moves=" << v.crossed
              << "  downstream=" << v.mobile_deposit << "  reached=" << v.reached << "  caught=" << v.hit
              << "  heat=" << v.heat << "  committed=" << v.committed << "  confirmed=" << v.confirmed
              << "  choices_changed=" << v.learned_choices << "  contact_edits=" << v.contact_edits
              << "  soma_changes=" << v.somatic_edits << "  blank_eaten=" << v.blank_eaten << "  gained=" << v.gained
              << "  lost=" << v.lost << "  bare_eaten=" << v.unbound_eaten << "  diverted=" << v.fibre_diversions
              << "  fibre_gain=" << v.fibre_gain << "  fibre_loss=" << v.fibre_loss << "  tied=" << v.tied
              << "  untied=" << v.untied << "  altered=" << v.changed_routes << "  young_reaches=" << v.young_reaches
              << "  young_meals=" << v.young_meals << "  older_reaches=" << v.old_reaches
              << "  older_meals=" << v.old_meals << "  lifetime_carbon=" << h.drift_water.total.eaten
              << "  counter_bytes=" << groups.size() * sizeof(groups[0]) << '\n';
}
void drift_inherit(const Habitat &h, const Options &o) {
    auto bank = [&](const std::string &path) {
        Habitat source;
        source.read(path);
        if (!source.braiding)
            throw std::invalid_argument("a drift germline pool needs living float operators");
        std::vector<drift::Germline> genes;
        Word sites = source.sites(), seed = h.drift_water.tag_seed;
        Word start = mix(seed + 349), stride = mix(seed + 3019) | 1;
        for (Word j = 0; j < sites && genes.size() < 32768; ++j) {
            Word id = (start + j * stride) & (sites - 1);
            if (!source.bodies[id].energy)
                continue;
            auto &g = source.genomes[id];
            auto p = regulation::source(g, regulation::innervated_band(g, source.conducting(id)), {});
            const auto &hand = source.riggers[id].genes;
            genes.push_back(
                {p.program, hand.plastic & 15, drift::wiring(hand, drift::recruitment), (hand.arbor >> 12) & 1});
        }
        if (genes.empty())
            throw std::invalid_argument("a drift germline pool contains no living source");
        return genes;
    };
    auto a = bank(o.drift_pool);
    auto b = o.drift_rival.empty() || o.drift_rival == o.drift_pool ? a : bank(o.drift_rival);
    Temporary first(s.d, a.size() * sizeof(a[0])), second(s.d, b.size() * sizeof(b[0]));
    s.d.check(s.d.upload(first.ptr, a.data(), a.size() * sizeof(a[0])), "introduce the first sample of germlines");
    s.d.check(s.d.upload(second.ptr, b.data(), b.size() * sizeof(b[0])), "introduce the second sample of germlines");
    Word na = a.size(), nb = b.size(), swap = o.drift_pool_order, loci = o.drift_loci, seed = h.drift_water.tag_seed;
    Function found{};
    s.bind(found, "drift_germlines", Image::floodplain);
    launch(found, (n + 127) / 128, 128, world, c, first.ptr, na, second.ptr, nb, swap, loci, seed);
    synchronize();
    std::cout << "drift_pools=" << tick << "  first=" << na << "  second=" << nb << "  swapped=" << swap
              << "  transferred_loci=" << loci << "  label_seed=" << seed << '\n';
}
// This is the post-run census of the genome and age arrays already read for
// the field guide. No food, classroom result, or speech transcript enters rank.
void drift_repertoire(const Habitat &h, const Options &o) {
    if (o.drift_germ.empty() && o.drift_repertoire.empty() && !o.drift_probe)
        return;
    if (!h.braiding)
        throw std::invalid_argument("a drift germline census needs float operators");
    struct Tally {
        Word count = 0, first = ~0u;
        Count excluded = 0;
    };
    using Allele = std::array<Word, 5>;
    bool coupled = h.drift_water.law.ports & drift::lateral;
    std::map<Allele, Tally> counts;
    Word living = 0;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy) {
            ++living;
            auto &g = h.genomes[i];
            auto band = regulation::innervated_band(g, h.conducting(i));
            auto program = regulation::source(g, band, {}).program;
            const auto &hand = h.riggers[i].genes;
            auto &t = counts[{program.first, program.second, hand.plastic & 15,
                              coupled ? drift::wiring(hand, h.drift_water.law.ports) : 0u,
                              coupled ? ((hand.arbor >> 12) & 1) : 1u}];
            ++t.count;
            t.first = std::min(t.first, i);
            t.excluded += std::popcount(h.riggers[i].volley >> 16);
        }
    std::vector<std::pair<Allele, Tally>> ranked(counts.begin(), counts.end());
    std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) {
        return a.second.count != b.second.count ? a.second.count > b.second.count : a.first < b.first;
    });
    if (!o.drift_repertoire.empty()) {
        EvidenceFile f(o.drift_repertoire);
        f.out << "first\tsecond\tlaw\tcopies\tslot\tborn\tgeneration\tcells\texcluded_bits\twiring\tactive\n";
        for (const auto &[a, t] : ranked) {
            const auto &b = h.bodies[t.first];
            f.out << a[0] << '\t' << a[1] << '\t' << a[2] << '\t' << t.count << '\t' << t.first << '\t' << b.born
                  << '\t' << b.generation << '\t' << 32 * h.conducting(t.first) << '\t' << t.excluded << '\t' << a[3]
                  << '\t' << a[4] << '\n';
        }
        f.publish();
    }
    if (ranked.empty()) {
        if (!o.drift_germ.empty() || !o.drift_food_germ.empty())
            throw std::invalid_argument("no living drift germline to export");
        return;
    }
    auto save = [&](const std::string &path, const Allele &a, const Tally &t) {
        EvidenceFile file(path);
        file.out << (h.drift_water.law.ports & drift::recruitment ? "AXION-INLET-GERM-2 "
                     : coupled                                    ? "AXION-INLET-GERM-1 "
                                                                  : "AXION-DRIFT-GERM-1 ")
                 << a[0] << ' ' << a[1] << ' ' << a[2] << ' ' << h.header.configuration.seed << ' '
                 << h.bodies[t.first].generation << ' ' << t.first << ' ' << tick << ' ' << t.count << ' ' << living;
        if (coupled)
            file.out << ' ' << a[3] << ' ' << a[4];
        file.out << '\n';
        file.publish();
    };
    if (o.drift_probe) {
        std::vector<drift::Germline> germs;
        for (const auto &[gene, tally] : ranked)
            germs.push_back({{gene[0], gene[1]}, gene[2], gene[3], gene[4]});
        Word count = germs.size(), lives = o.drift_probe, seed = o.class_seed,
             growth = bool(h.drift_water.law.ports & drift::regrowth);
        if (o.contact_remodel != ~0u)
            growth = o.contact_remodel;
        Word ports = (h.drift_water.law.ports & ~Word(drift::regrowth)) | (growth ? Word(drift::regrowth) : 0u);
        Temporary population(s.d, Count(count) * sizeof(germs[0])), records(s.d, Count(count) * sizeof(drift::Diet));
        s.d.check(s.d.upload(population.ptr, germs.data(), Count(count) * sizeof(germs[0])),
                  "introduce untrained copies of expressed learning rules");
        Function probe{};
        s.bind(probe, "drift_food_test", Image::floodplain);
        std::unique_ptr<EvidenceFile> evidence;
        if (!o.drift_probe_trace.empty()) {
            evidence = std::make_unique<EvidenceFile>(o.drift_probe_trace);
            evidence->out << "panel\tseed\tindex\tfirst\tsecond\tlaw\tcopies\tslot\tfood\tblank\tfirst_tide\tafter_"
                             "shift\tafter_return\tchanged\tchanged_food\twiring\tactive\n";
        }
        Word winner = 0;
        std::vector<drift::Diet> scores(count);
        for (Word panel = 0; panel < 2; ++panel) {
            Word key = panel ? mix(seed ^ 0x699d2c73u) : seed;
            FieldTimer timer(s.d);
            timer.start(stream);
            launch(probe, (count + 31) / 32, 32, population.ptr, count, key, lives, ports, records.ptr);
            timer.stop(stream);
            synchronize();
            float elapsed = timer.milliseconds();
            s.d.check(s.d.download(scores.data(), records.ptr, Count(count) * sizeof(scores[0])),
                      "weigh the common new currents");
            auto rank = [](const drift::Diet &r) {
                return std::tuple{std::min({r.phase[0], r.phase[1], r.phase[2]}), r.food};
            };
            Count food = 0, null = 0, offered = Count(living) * lives * 96;
            for (Word j = 0; j < count; ++j) {
                auto &v = scores[j];
                const auto &[a, t] = ranked[j];
                if (!panel && rank(v) > rank(scores[winner]))
                    winner = j;
                food += Count(t.count) * v.food;
                null += Count(t.count) * v.blank;
                if (evidence)
                    evidence->out << panel << '\t' << key << '\t' << j << '\t' << a[0] << '\t' << a[1] << '\t' << a[2]
                                  << '\t' << t.count << '\t' << t.first << '\t' << v.food << '\t' << v.blank << '\t'
                                  << v.phase[0] << '\t' << v.phase[1] << '\t' << v.phase[2] << '\t' << v.changed << '\t'
                                  << v.changed_food << '\t' << a[3] << '\t' << a[4] << '\n';
            }
            const auto &[a, t] = ranked[winner];
            auto &r = scores[winner];
            std::cout << "drift_food_test=" << (panel ? "confirmation" : "selection") << "  tick=" << tick
                      << "  alleles=" << count << "  residents=" << living << "  lives=" << lives << "  seed=" << key
                      << "  growth=" << growth << "  offered=" << offered << "  eaten=" << food << "  blank=" << null
                      << "  champion=" << winner << "  copies=" << t.count << "  slot=" << t.first << "  first=" << a[0]
                      << "  second=" << a[1] << "  law=" << a[2] << "  individual_food=" << r.food
                      << "  individual_blank=" << r.blank << "  individual_offered=" << 96 * lives
                      << "  changed=" << r.changed << "  changed_food=" << r.changed_food << "  cuda_ms=" << elapsed
                      << '\n';
        }
        if (evidence)
            evidence->publish();
        if (!o.drift_food_germ.empty())
            save(o.drift_food_germ, ranked[winner].first, ranked[winner].second);
    }
    const auto &[a, t] = ranked.front();
    if (!o.drift_germ.empty())
        save(o.drift_germ, a, t);
    std::cout << "drift_repertoire=" << tick << "  living=" << living << "  alleles=" << counts.size()
              << "  copies=" << t.count << "  slot=" << t.first << "  first=" << a[0] << "  second=" << a[1]
              << "  law=" << a[2] << "  generation=" << h.bodies[t.first].generation << '\n';
}
