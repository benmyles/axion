void detritus_report(const Habitat &h, const std::string &path) {
    if (!cargo_totals.ptr)
        return;
    using Block = std::array<Count, sizeof(detritus::Ledger) / sizeof(Count)>;
    std::array<Block, 128> blocks{};
    Block sum{};
    s.d.check(s.d.download(blocks.data(), cargo_totals.ptr, sizeof(blocks)), "weigh hydraulic sediment transfer");
    for (const auto &b : blocks)
        for (Word j = 0; j < sum.size(); ++j)
            sum[j] += b[j];
    auto r = std::bit_cast<detritus::Ledger>(sum);
    if (r.arrivals != r.departures || r.departures + r.blocked != r.offers || r.east + r.south != r.departures ||
        r.mass_before != r.mass_after)
        throw std::logic_error("the transported food or its physical route lost mass");
    Word mode = (h.cap_law.mode & detritus::displaced) ? 2 : 1;
    std::cout << "detritus_field=" << tick << "  mode=" << mode << "  partitions=" << r.tiles
              << "  gate_exposures=" << r.gates << "  stocked_sites=" << r.stocked << "  offers=" << r.offers
              << "  departed=" << r.departures << "  arrived=" << r.arrivals << "  congested=" << r.blocked
              << "  east=" << r.east << "  south=" << r.south << "  stock_summed_before=" << r.mass_before
              << "  stock_summed_after=" << r.mass_after
              << "  routing_site_bits=0  transient_site_bits=" << bool(cargo_notes.ptr) << "  permanent_site_bits=0"
              << "  observed=" << bool(cargo_notes.ptr) << "  pressure_interventions=" << r.stimulus_changes
              << "  enabled_edges=" << r.food_enabled << "  prevented_edges=" << r.food_prevented
              << "  dye_exchange=" << bool(h.cap_law.mode & detritus::exchanges) << "  changed_colours=" << r.recoloured
              << "  observation_bytes="
              << (cargo_notes.ptr ? Count(cargo_film.uniform + cargo_film.watched) * sizeof(detritus::Note) +
                                        Count((n + 31) / 32) * 4
                                  : 0)
              << '\n';
    if (path.empty())
        return;
    std::vector<detritus::Note> notes(cargo_film.uniform + cargo_film.watched);
    s.d.check(s.d.download(notes.data(), cargo_notes.ptr, notes.size() * sizeof(notes[0])),
              "recover actual bounded food movements and matched material effects");
    auto pack = [](const auto &v) {
        Count a = 0;
        for (Word i = 0; i < 4; ++i)
            a |= Count(v[i]) << (16 * i);
        return a;
    };
    EvidenceFile evidence(path);
    evidence.out << "targeted\ttick\tepoch\tside\ttile\tx\ty\tmode\tlocal\tgates\tchanged\tpriority\tfood\twater\tsill"
                    "\tseam\tfinal\tproposed\tmoved\talt0\talt1\talt2\tactor0\tactor1\tactor2\tborn0\tborn1\tborn2\tsur"
                    "face\tpaint_before\tpaint_after\n";
    const detritus::Note *sample = nullptr;
    Word cause = 3, best = 0;
    for (Word j = 0; j < notes.size(); ++j) {
        auto &a = notes[j];
        if (!a.tick)
            continue;
        Word targeted = j >= cargo_film.uniform;
        evidence.out << targeted;
        for (Count v : std::initializer_list<Count>{a.tick,
                                                    a.epoch,
                                                    c.side,
                                                    a.tile,
                                                    a.x,
                                                    a.y,
                                                    a.mode,
                                                    a.local,
                                                    a.gates,
                                                    a.changes,
                                                    a.priority,
                                                    pack(a.bed.food),
                                                    pack(a.bed.water),
                                                    pack(a.bed.sill),
                                                    pack(a.bed.seam),
                                                    pack(a.final),
                                                    a.offered,
                                                    a.moved,
                                                    a.alternate[0],
                                                    a.alternate[1],
                                                    a.alternate[2],
                                                    a.actor[0],
                                                    a.actor[1],
                                                    a.actor[2],
                                                    a.born[0],
                                                    a.born[1],
                                                    a.born[2],
                                                    a.surface,
                                                    a.paint_before,
                                                    a.paint_after})
            evidence.out << '\t' << v;
        evidence.out << '\n';
        if (!sample && a.moved)
            sample = &a;
        for (Word k = 0; k < 3; ++k) {
            if (!(a.changes & (1u << k)) || a.alternate[k] == a.moved)
                continue;
            Word gain = a.moved & ~a.alternate[k];
            Word rank = 1 + Word(gain != 0) + Word(gain && a.surface && a.paint_before != a.paint_after);
            if (rank > best || (rank == best && a.tick < sample->tick)) {
                best = rank;
                sample = &a;
                cause = k;
            }
        }
    }
    evidence.publish();
    if (sample) {
        auto &a = *sample;
        std::cout << "detritus_scene=" << a.tick << "  epoch=" << a.epoch << "  x=" << a.x << "  y=" << a.y
                  << "  side=" << c.side << "  current=" << a.gates << "  food=" << pack(a.bed.food)
                  << "  after=" << pack(a.final) << "  wet=" << pack(a.bed.water) << "  sill=" << pack(a.bed.sill)
                  << "  strokes=" << a.moved << "  change=" << cause
                  << "  alternate=" << (cause < 3 ? a.alternate[cause] : a.moved)
                  << "  handler=" << (cause < 3 ? a.actor[cause] : 0)
                  << "  handler_born=" << (cause < 3 ? a.born[cause] : 0) << "  mode=" << mode
                  << "  surface=" << a.surface << "  paint_before=" << a.paint_before
                  << "  paint_after=" << a.paint_after
                  << "  priority=" << a.priority << "  selection=" << best << '\n';
    }
}
void detritus_proof(const std::string &path) {
    Function fn{};
    s.bind(fn, "flood_detritus_proof", Image::floodplain);
    Temporary receipts(s.d, Count(detritus::proof_cases) * sizeof(detritus::Proof));
    FieldTimer clock(s.d);
    clock.start(stream);
    launch(fn, detritus::proof_cases / 128, 128, receipts.ptr);
    clock.stop(stream);
    synchronize();
    std::vector<detritus::Proof> results(detritus::proof_cases);
    s.d.check(s.d.download(results.data(), receipts.ptr, Count(results.size()) * sizeof(results[0])),
              "verify pressure routing on bank and overflow boundaries");
    EvidenceFile file(path);
    file.out << "case\tproposed\tmoved\tfinal\n";
    Count moved = 0, denied = 0;
    for (Word j = 0; j < results.size(); ++j) {
        const auto &r = results[j];
        moved += std::popcount(r.moved);
        denied += std::popcount(r.offered & ~r.moved);
        file.out << j << '\t' << r.offered << '\t' << r.moved << '\t' << r.final << '\n';
    }
    file.publish();
    std::cout << "detritus_boundaries=" << results.size() << "  moved=" << moved << "  denied=" << denied
              << "  cuda_ms=" << clock.milliseconds() << '\n';
}
