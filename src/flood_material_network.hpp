void material_census(const Habitat &h, Word exposure, Word samples) {
    if (!h.articulated)
        return;
    Word open = material::conducts(h.minerals.physics), side = c.side;
    Temporary parents(s.d, n * sizeof(Word)), tables(s.d, n * sizeof(material::Component));
    Function label{}, join{}, survey{};
    s.bind(label, "flood_material_labels", Image::floodplain);
    s.bind(join, "flood_material_join", Image::floodplain);
    s.bind(survey, "flood_material_components", Image::floodplain);
    s.d.check(s.d.zero(tables.ptr, 0, n * sizeof(material::Component)), "clear assembly census");
    launch(label, (n + 127) / 128, 128, parents.ptr, n);
    launch(join, (n + 127) / 128, 128, mineral_bed.ptr, parents.ptr, side, open);
    launch(survey, (n + 127) / 128, 128, mineral_bed.ptr, parents.ptr, tables.ptr, side, open);
    synchronize();
    std::vector<material::Component> rows(n);
    s.d.check(s.d.download(rows.data(), tables.ptr, n * sizeof(rows[0])), "observe connected assemblies");
    Count parts = 0, bonds = 0, cycles = 0, branch = 0, touched = 0, molecules = 0, highest = 0, handled_molecules = 0;
    Word best = ~0u;
    for (Word i = 0; i < n; ++i) {
        auto &r = rows[i];
        if (!r.size)
            continue;
        parts += r.size;
        bonds += r.edges;
        cycles += r.edges + 1 - r.size;
        branch += r.branches;
        touched += r.handled;
        ++molecules;
        handled_molecules += r.size >= 2 && r.handled;
        if (r.size > highest) {
            highest = r.size;
            best = i;
        }
    }
    std::cout << "assemblies=" << molecules << "  sites=" << parts << "  bonds=" << bonds << "  largest=" << highest
              << "  independent_loops=" << cycles << "  branches=" << branch << "  handled_parts=" << touched
              << "  handled_assemblies=" << handled_molecules << "  largest_root=" << best << '\n';
    Word shown = characterize(h, rows, samples);
    material_photograph(h, shown == ~0u ? best : shown);
    if (!exposure)
        return;
    if (exposure > ~0u - tick)
        throw std::invalid_argument("assembly exposure exceeds habitat clock");
    Function rewind{}, arrange{}, simulate{};
    s.bind(rewind, "flood_material_rewind", Image::floodplain);
    s.bind(arrange, "flood_material_stock", Image::floodplain);
    s.bind(simulate, "flood_material_flux", Image::floodplain);
    Temporary model(s.d, n * sizeof(material::Grain)), food(s.d, n * sizeof(Half)), product(s.d, n * sizeof(Half)),
        report(s.d, n * sizeof(material::Flux));
    std::vector<material::Flux> intact(n), current(n);
    Word disorder = mix(c.seed ^ tick);
    for (Word mode = 0; mode < 4; ++mode) {
        s.d.check(s.d.copy_device(model.ptr, mineral_bed.ptr, n * sizeof(material::Grain)),
                  "retain built geometry and stored excitation");
        s.d.check(s.d.zero(report.ptr, 0, n * sizeof(material::Flux)), "clear product observations");
        launch(arrange, (n + 127) / 128, 128, land.ptr, food.ptr, product.ptr, n);
        if (mode == 3)
            launch(rewind, (n + 127) / 128, 128, model.ptr, mineral_bed.ptr, side, disorder);
        Word treatment = open ? (mode == 3 ? 0 : mode) : 1;
        for (Word j = 0; j < exposure; ++j)
            launch(simulate, (n / 2 + 127) / 128, 128, model.ptr, land.ptr, food.ptr, product.ptr, report.ptr, c, tick,
                   j, treatment, world.tide.active, salts.ptr);
        synchronize();
        s.d.check(s.d.download(current.data(), report.ptr, n * sizeof(current[0])),
                  "observe product after builders depart");
        if (!mode)
            intact = current;
        Count made = 0, passed = 0, gain = 0, loss = 0, sites = 0;
        for (Word i = 0; i < n; ++i) {
            auto q = current[i];
            made += q.made;
            passed += q.passed;
            gain += std::max(q.made, intact[i].made) - q.made;
            loss += std::max(q.made, intact[i].made) - intact[i].made;
            sites += q.made != intact[i].made;
        }
        std::cout << "material_legacy="
                  << (mode == 0   ? "intact"
                      : mode == 1 ? "insulated"
                      : mode == 2 ? "handled_cut"
                                  : "rearranged")
                  << "  ticks=" << exposure << "  reactions=" << made << "  transfers=" << passed
                  << "  improved=" << gain << "  reduced=" << loss << "  changed_sites=" << sites << '\n';
    }
}
void hydraulic_trial(Word turns) {
    if (!turns)
        return;
    if (turns > ~0u - tick)
        throw std::invalid_argument("hydraulic exposure exceeds the clock");
    Word shards = (n / 2 + 127) / 128;
    Temporary soil(s.d, Count(n) * sizeof(Silt)), grains(s.d, Count(n) * sizeof(material::Grain)),
        physical(s.d, Count(shards) * sizeof(Counters)), work(s.d, Count(shards) * sizeof(material::Ledger)),
        totals(s.d, sizeof(Counters));
    std::vector<Silt> before(n), now(n);
    Temporary carbon(s.d, world.seams.stock ? Count(n) * sizeof(Half) : 0);
    Temporary salt_copy(s.d, world.brine.solutes ? Count(n) * sizeof(brine::Salts) : 0);
    for (Word mode = 0; mode < 2; ++mode) {
        s.d.check(s.d.copy_device(soil.ptr, land.ptr, Count(n) * sizeof(Silt)),
                  "retain identical water and growing producers");
        s.d.check(s.d.copy_device(grains.ptr, mineral_bed.ptr, Count(n) * sizeof(material::Grain)),
                  "retain charged mineral geometry");
        s.d.check(s.d.zero(physical.ptr, 0, shards * sizeof(Counters)), "clear detached water flux");
        s.d.check(s.d.zero(work.ptr, 0, shards * sizeof(material::Ledger)), "clear detached mineral flux");
        World q{};
        q.siphons.ports = world.siphons.ports;
        q.siphons.bands = world.siphons.bands;
        if (salt_copy.ptr) {
            s.d.check(s.d.copy_device(salt_copy.ptr, salts.ptr, Count(n) * sizeof(brine::Salts)),
                      "retain electrolytes in both currents");
            q.brine = world.brine;
            q.brine.animals = nullptr;
            q.brine.solutes = reinterpret_cast<brine::Salts *>(salt_copy.ptr);
        }
        if (carbon.ptr) {
            s.d.check(s.d.copy_device(carbon.ptr, polymers.ptr, Count(n) * sizeof(Half)),
                      "retain bound nutrients in the paired currents");
            q.seams = world.seams;
            q.seams.stock = reinterpret_cast<Half *>(carbon.ptr);
            q.seams.account = nullptr;
        }
        q.tide.active = world.tide.active;
        q.land = reinterpret_cast<Silt *>(soil.ptr);
        q.account = reinterpret_cast<Counters *>(physical.ptr);
        q.matter.bed = reinterpret_cast<material::Grain *>(grains.ptr);
        q.matter.ledger = reinterpret_cast<material::Ledger *>(work.ptr);
        q.matter.conditions = world.matter.conditions;
        q.matter.conditions.bonded = (q.matter.conditions.bonded & 1) | (mode ? 0 : 2);
        for (Word j = 0; j < turns; ++j)
            launch(water, (n / 2 + 127) / 128, 128, q, c, clock.ptr, j);
        sum_account(physical.ptr, totals.ptr);
        synchronize();
        Counters all;
        s.d.check(s.d.download(&all, totals.ptr, sizeof(all)), "measure water through the mineral field");
        s.d.check(s.d.download(now.data(), soil.ptr, n * sizeof(Silt)), "compare flood levels under the same light");
        if (!mode)
            before = now;
        Count wet = 0, dist = 0, food = 0, material = 0;
        for (Word i = 0; i < n; ++i) {
            int d = int(now[i].water) - int(before[i].water);
            wet += d != 0;
            dist += std::abs(d);
            int x = int(now[i].leaf) + now[i].fruit + now[i].litter,
                y = int(before[i].leaf) + before[i].fruit + before[i].litter;
            food += std::abs(x - y);
            material += now[i].water;
        }
        std::cout << "mineral_hydraulics=" << (mode ? "permeable" : "coupled") << "  ticks=" << turns
                  << "  water=" << material << "  flow=" << all.flow << "  water_sites=" << wet
                  << "  water_distance=" << dist << "  food_distance=" << food << '\n';
    }
}
