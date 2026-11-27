void replace_wicks(const Habitat &donor, Habitat &h) {
    if (donor.wicking) {
        h.wicking_state.book = {};
        for (Word v : donor.liquid)
            h.wicking_state.book.tracers += !!(v & wicks::present);
        s.d.check(s.d.upload(parcels.ptr, donor.liquid.data(), Count(n) * 4),
                  "replace public capillary positions and tracers");
        s.d.check(s.d.zero(wick_ledgers.ptr, 0, Count((n / 2 + 127) / 128) * sizeof(wicks::Totals)),
                  "clear prior catchment accounts");
        s.d.check(s.d.upload(wick_ledgers.ptr, &h.wicking_state.book, sizeof(wicks::Totals)),
                  "restore donated tracer inventory");
    } else {
        Function clean{};
        s.bind(clean, "wick_rebed", Image::floodplain);
        launch(clean, (n + 127) / 128, 128, world.wicks, n);
        synchronize();
    }
}
void wick_study(const Habitat &h, const Options &o) {
    if (!h.wicking)
        return;
    Word side = h.header.configuration.side, period = h.wicking_state.mode >> 8;
    if (!period)
        period = side;
    Count dyes = 0, multi = 0, distant = 0, reach = 0, used = 0;
    Word furthest = 0;
    for (Word i = 0; i < n; ++i) {
        auto z = h.liquid[i];
        used += std::popcount(z >> 30);
        if (z & wicks::present) {
            ++dyes;
            auto from = wicks::origin(z);
            Word dx = std::abs(int(i % side) - int(from % side)), dy = std::abs(int(i / side) - int(from / side));
            dx %= period;
            dy %= period;
            Word distance = std::min(dx, period - dx) + std::min(dy, period - dy);
            furthest = std::max(furthest, distance);
            reach += distance;
            multi += wicks::hops(z) >= 2;
            distant += distance >= 4;
        }
    }
    auto &b = h.wicking_state.book;
    if (dyes != b.tracers)
        throw std::logic_error("capillary tracer mass changed");
    std::cout << "wick_field=" << h.header.tick << "  contacts=" << b.contacts << "  water=" << b.water
              << "  carbon=" << b.carbon << "  cross_tile=" << b.transverse << "  cross_carbon=" << b.transverse_food
              << "  made_links=" << b.strung << "  standing=" << used << "  worked_events=" << b.worked
              << "  worked_carbon=" << b.worked_food << "  tracers=" << dyes << "  hops=" << b.tracer_moves
              << "  later_hops=" << b.later_hops << "  outer_hops=" << b.bridged_hops << "  multihop=" << multi
              << "  far=" << distant << "  furthest=" << furthest << "  reference_side=" << period
              << "  distance=" << reach << "  cavity_in=" << b.admitted << "  cavity_out=" << b.released
              << "  removed=" << b.frayed << '\n';
    if (o.wick_census) {
        Word bins = std::min(32u, side), span = side / bins;
        const char digits[] = "0123456789abcdef";
        for (Word y = 0; y < bins; ++y) {
            std::string row;
            for (Word x = 0; x < bins; ++x) {
                Word fibre = 0, transit = 0, marked = 0;
                for (Word j = 0; j < span; ++j)
                    for (Word i = 0; i < span; ++i) {
                        Word pos = (y * span + j) * side + x * span + i, z = h.liquid[pos];
                        auto &soil = h.land[pos];
                        fibre += Word(!!soil.fibre_east) + Word(!!soil.fibre_south);
                        marked += std::popcount(z >> 30);
                        transit += (z & wicks::present) && wicks::hops(z) >= 2;
                    }
                for (Word a : {fibre, marked, transit}) {
                    row += digits[(a >> 8) & 15];
                    row += digits[(a >> 4) & 15];
                    row += digits[a & 15];
                }
            }
            std::cout << "wick_ground=" << y << "  bins=" << bins << "  counts=" << row << '\n';
        }
    }
    if (o.wick_length) {
        Word cases = 64, length = o.wick_length, rounds = 16 * length * length, seed = o.wick_seed;
        Temporary output(s.d, cases * sizeof(wicks::Result));
        Function f{};
        s.bind(f, "wick_courses", Image::floodplain);
        std::vector<wicks::Result> rows(cases);
        const char *arms[]{"continuous", "cut", "retied", "within_tiles"};
        for (Word mode = 0; mode < 4; ++mode) {
            launch(f, cases, 128, cases, length, rounds, mode, seed, output.ptr);
            synchronize();
            s.d.check(s.d.download(rows.data(), output.ptr, rows.size() * sizeof(rows[0])),
                      "weigh water, dissolved carbon, fibre and dye");
            std::array<Count, sizeof(wicks::Result) / 4> sum{};
            for (auto &v : rows) {
                if (v.water_before != v.water_left + v.water_caught || v.food_before != v.food_left + v.food_caught ||
                    v.strand_before != v.strand_after + v.spare)
                    throw std::logic_error("unbalanced capillary experiment");
                auto a = std::bit_cast<std::array<Word, sum.size()>>(v);
                for (size_t j = 0; j < a.size(); ++j)
                    sum[j] += a[j];
            }
            std::cout << "wick_transfer=" << arms[mode] << "  lanes=" << cases << "  length=" << length
                      << "  ticks=" << rounds << "  water_in=" << sum[3] << "  water_left=" << sum[4]
                      << "  water_caught=" << sum[5] << "  carbon_in=" << sum[6] << "  carbon_left=" << sum[7]
                      << "  carbon_caught=" << sum[8] << "  fibre=" << sum[9] << "  installed=" << sum[10]
                      << "  slack=" << sum[11] << "  repair_work=" << sum[12]
                      << "  mean_distance=" << double(sum[13]) / cases << '\n';
        }
    }
}
