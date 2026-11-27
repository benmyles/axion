void rafts(Habitat &h, const Options &o) {
    if (!o.raft_seasons)
        return;
    Word count = o.rafts, seed = c.seed, days = o.raft_days, drift = o.raft_drift, flush=o.raft_flush, variation = o.raft_frames;
    Word origin = (!o.colonists.empty() || !o.pod_germ.empty() || !o.founder.empty()) ? 3 : h.pod_origin;
    if (h.pod_origin == 3)
        origin = 3;
    Temporary adults(s.d, Count(count) * sizeof(pods::Embryo)), eggs(s.d, Count(count) * sizeof(pods::Embryo)),
        ledger(s.d, Count(count / 32) * sizeof(pods::RaftBudget));
    std::vector<pods::Animal> organ(n);
    std::vector<pods::Inheritance> source;
    sample(h);
    s.d.check(s.d.download(organ.data(), pod_tissue.ptr, n * sizeof(organ[0])), "read marsh germlines");
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            source.push_back({organ[i].genes, organ[i].frame_genes});
    if (source.empty()) {
        std::cout << "raft_descendants=0  founding_lines=0  inherited_rules=0  recruitment=empty\n";
        return;
    }
    Temporary newborns(s.d, source.size() * sizeof(source[0]));
    Word donors = source.size();
    s.d.check(s.d.upload(newborns.ptr, source.data(), source.size() * sizeof(source[0])),
              "send the founder pool to the rafts");
    Function found{}, season{}, release{};
    s.bind(found, "flood_raft_seed", Image::floodplain);
    s.bind(season, "flood_raft_season", Image::floodplain);
    s.bind(release, "flood_raft_release", Image::floodplain);
    launch(found, (count + 127) / 128, 128, adults.ptr, newborns.ptr, donors, count, seed, origin, variation);
    auto start = std::chrono::steady_clock::now();
    std::vector<pods::RaftBudget> books(count / 32);
    for (Word gen = 0; gen < o.raft_seasons; ++gen) {
        launch(season, count / 32, 32, adults.ptr, eggs.ptr, ledger.ptr, count, seed, days, gen, drift, flush,
               world.pods.physics);
        std::swap(adults.ptr, eggs.ptr);
        if (gen == 0 || (gen + 1) % 16 == 0 || gen + 1 == o.raft_seasons) {
            synchronize();
            s.d.check(s.d.download(books.data(), ledger.ptr, books.size() * sizeof(books[0])),
                      "read tidal provisioning");
            pods::RaftBudget all;
            for (auto b : books) {
                if (b.initial + b.light != b.stock + b.waste + b.heat + b.reserve || b.provision > b.reserve ||
                    b.provision != 64 * b.offspring)
                    throw std::logic_error("tidal raft energy or embryonic provisioning is inconsistent");
                auto row = std::bit_cast<std::array<Count, sizeof(b) / sizeof(Count)>>(b);
                auto sum = std::bit_cast<decltype(row)>(all);
                for (size_t k = 0; k < row.size(); ++k)
                    sum[k] += row[k];
                all = std::bit_cast<pods::RaftBudget>(sum);
            }
            std::cout << "raft_season=" << gen + 1 << "  survivors=" << all.adults << "  embryos=" << all.offspring
                      << "  provision=" << all.provision << "  reserve=" << all.reserve << "  light=" << all.light
                      << "  heat=" << all.heat << "  bites=" << all.bites << "  opened=" << all.opened
                      << "  held=" << all.held << "  isolated=" << all.isolated << "  changes=" << all.edits << '\n';
        }
    }
    synchronize();
    std::vector<pods::Embryo> last(count);
    s.d.check(s.d.download(last.data(), adults.ptr, count * sizeof(last[0])), "retrieve the provisioned offspring");
    source.clear();
    std::set<Word> lines;
    std::set<std::array<Word, 7>> variants;
    for (auto e : last)
        if (e.living) {
            source.push_back({e.law, e.frame});
            lines.insert(e.lineage);
            variants.insert({e.law.plastic, e.law.timing, e.law.readout, e.law.sense, e.frame.routes, e.frame.learning,
                             e.frame.consolidation});
        }
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    Count slot_ticks = Count(count) * days * o.raft_seasons;
    std::cout << "raft_descendants=" << source.size() << "  founding_lines=" << lines.size()
              << "  inherited_rules=" << variants.size() << "  recruitment=" << (drift ? "fostered" : "provisioned")
              << "  raft_slot_ticks=" << slot_ticks << "  Mraft_slot_ticks_per_s=" << slot_ticks / (1e6 * seconds)
              << "  seconds=" << seconds << '\n';
    if (source.empty()) {
        // A failed brood cannot be substituted with fresh founders.
        Function vacant{};
        s.bind(vacant, "flood_raft_empty", Image::floodplain);
        launch(vacant, (n + 127) / 128, 128, world, c);
        synchronize();
        return;
    }
    Temporary mothers(s.d, source.size() * sizeof(pods::Inheritance));
    donors = source.size();
    s.d.check(s.d.upload(mothers.ptr, source.data(), donors * sizeof(pods::Inheritance)),
              "introduce provisioned crop germlines");
    launch(release, (n + 127) / 128, 128, mothers.ptr, donors, world, c, seed);
    synchronize();
}
