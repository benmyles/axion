// Both counterfactual landscapes undergo exactly the same physical ticks.
// The cells carry their actual water, detritus, seeds and enzyme state. The
// control remobilizes wall sediment into the cell that owns the face.
void legacy(Word years) {
    if (!years)
        return;
    if (years > ~0u - tick)
        throw std::invalid_argument("legacy trial exceeds the physical clock");
    Word shards = (n / 2 + 127) / 128;
    Temporary sums(s.d, 2 * sizeof(Counters)), native(s.d, n * sizeof(Silt)), flat(s.d, n * sizeof(Silt)),
        ledgers(s.d, 2 * shards * sizeof(Counters)), differences(s.d, n * sizeof(LegacyDifference)), refused(s.d, 4);
    Temporary mineral_fields(s.d, world.matter.bed ? 2 * Count(n) * sizeof(material::Grain) : 0),
        mineral_accounts(s.d, world.matter.bed ? 2 * Count(shards) * sizeof(material::Ledger) : 0);
    if (world.matter.bed) {
        s.d.check(s.d.zero(mineral_accounts.ptr, 0, 2 * Count(shards) * sizeof(material::Ledger)),
                  "clear comparative mineral flux");
        for (Word arm = 0; arm < 2; ++arm)
            s.d.check(s.d.copy_device(mineral_fields.ptr + Count(arm) * n * sizeof(material::Grain), mineral_bed.ptr,
                                      Count(n) * sizeof(material::Grain)),
                      "keep the existing reactive material in both landscapes");
    }
    s.d.check(s.d.copy_device(native.ptr, land.ptr, n * sizeof(Silt)), "copy the engineered landscape");
    s.d.check(s.d.copy_device(flat.ptr, land.ptr, n * sizeof(Silt)), "copy the control landscape");
    s.d.check(s.d.zero(ledgers.ptr, 0, 2 * shards * sizeof(Counters)), "clear counterfactual budgets");
    s.d.check(s.d.zero(refused.ptr, 0, 4), "clear remobilization errors");
    Function flatten{}, compare{};
    s.bind(flatten, "flood_unbank", Image::floodplain);
    s.bind(compare, "flood_legacy_difference", Image::floodplain);
    launch(flatten, (n + 127) / 128, 128, flat.ptr, n, refused.ptr);
    synchronize();
    Word failed;
    s.d.check(s.d.download(&failed, refused.ptr, 4), "check conserved bank removal");
    if (failed)
        throw std::runtime_error("bank material exceeds the control cell's sediment capacity");
    Temporary carbon(s.d, world.seams.stock ? 2 * Count(n) * sizeof(Half) : 0);
    Temporary salts_copy(s.d, world.brine.solutes ? Count(n) * sizeof(brine::Salts) : 0);
    std::vector<brine::Salts> deposits[2];
    for (Word k = 0; k < 2; ++k) {
        World terrain{};
        terrain.siphons.ports = world.siphons.ports;
        terrain.siphons.bands = world.siphons.bands;
        if (salts_copy.ptr) {
            s.d.check(s.d.copy_device(salts_copy.ptr, salts.ptr, Count(n) * sizeof(brine::Salts)),
                      "retain the dissolved minerals");
            terrain.brine = world.brine;
            terrain.brine.animals = nullptr;
            terrain.brine.solutes = reinterpret_cast<brine::Salts *>(salts_copy.ptr);
        }
        if (carbon.ptr) {
            terrain.seams = world.seams;
            terrain.seams.account = nullptr;
            terrain.seams.stock = reinterpret_cast<Half *>(carbon.ptr) + Count(k) * n;
            s.d.check(s.d.copy_device(carbon.ptr + Count(k) * n * sizeof(Half), polymers.ptr, Count(n) * sizeof(Half)),
                      "retain equal bound carbon in the bank trial");
        }
        terrain.tide.active = world.tide.active;
        if (world.matter.bed) {
            terrain.matter.bed = reinterpret_cast<material::Grain *>(mineral_fields.ptr) + Count(k) * n;
            terrain.matter.ledger = reinterpret_cast<material::Ledger *>(mineral_accounts.ptr) + Count(k) * shards;
            terrain.matter.conditions = world.matter.conditions;
        }
        terrain.land = reinterpret_cast<Silt *>(k ? flat.ptr : native.ptr);
        terrain.account = reinterpret_cast<Counters *>(ledgers.ptr) + k * shards;
        for (Word j = 0; j < years; ++j)
            launch(water, (n / 2 + 127) / 128, 128, terrain, c, clock.ptr, j);
        if (salts_copy.ptr) {
            synchronize();
            deposits[k].resize(n);
            s.d.check(s.d.download(deposits[k].data(), salts_copy.ptr, Count(n) * sizeof(brine::Salts)),
                      "recover ionic transport after the builders leave");
        }
        sum_account(ledgers.ptr + k * shards * sizeof(Counters), sums.ptr + k * sizeof(Counters));
    }
    if (salts_copy.ptr) {
        std::vector<Silt> terrain[2];
        terrain[0].resize(n);
        terrain[1].resize(n);
        s.d.check(s.d.download(terrain[0].data(), native.ptr, Count(n) * sizeof(Silt)),
                  "measure the intact solvent volume");
        s.d.check(s.d.download(terrain[1].data(), flat.ptr, Count(n) * sizeof(Silt)),
                  "measure solvent volume after removing banks");
        Count shifted = 0, sites = 0, chemistry = 0, contacts = 0;
        std::array<Count, 5> before{}, after{};
        for (Word i = 0; i < n; ++i) {
            auto a = deposits[0][i], b = deposits[1][i];
            sites += a.ions != b.ions;
            for (Word k = 0; k < 5; ++k) {
                Word av = brine::amount(a, k), bv = brine::amount(b, k);
                before[k] += av;
                after[k] += bv;
                shifted += av > bv ? av - bv : 0;
            }
            Word left = brine::phase(a, terrain[0][i].water), right = brine::phase(b, terrain[1][i].water),
                 different = 0;
            for (Word j = 0; j < 4; ++j)
                different += brine::surface(j, left) != brine::surface(j, right);
            contacts += 2 * different;
            chemistry += different != 0;
        }
        if (before != after)
            throw std::logic_error("detached salt stocks diverged");
        std::cout << "brine_legacy=" << years << "  ion_sites=" << sites << "  displaced_units=" << shifted
                  << "  changed_laws=" << chemistry << "  different_pairs=" << contacts << '\n';
    }
    launch(compare, (n + 127) / 128, 128, native.ptr, flat.ptr, n, differences.ptr);
    synchronize();
    std::vector<LegacyDifference> out(n);
    s.d.check(s.d.download(out.data(), differences.ptr, n * sizeof(out[0])),
              "compare habitats after the builders leave");
    Count wet = 0, food = 0, water_difference = 0, stock_difference = 0, wetter = 0, greener = 0;
    for (auto d : out) {
        wet += d.water != 0;
        food += d.food != 0;
        water_difference += d.water;
        stock_difference += d.food;
        wetter += d.wetter;
        greener += d.vegetation;
    }
    Counters budgets[2];
    s.d.check(s.d.download(budgets, sums.ptr, sizeof(budgets)), "measure inherited hydrology");
    std::cout << "ecological_legacy=" << years << "  sites=" << n << "  water_sites=" << wet << "  food_sites=" << food
              << "  water_distance=" << water_difference << "  stock_distance=" << stock_difference
              << "  wetter=" << wetter << "  greener=" << greener << "  intact_flow=" << budgets[0].flow
              << "  flat_flow=" << budgets[1].flow << "  intact_light=" << budgets[0].light
              << "  flat_light=" << budgets[1].light << '\n';
}
