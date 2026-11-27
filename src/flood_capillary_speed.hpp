void capillary_speed(capillary::Model m, Word senses, capillary::Work &w, Word seed, Word n) {
    auto projection = capillary::project(m, senses, w.bed);
    auto die = capillary::stamp(projection);
    auto surface = w.bed;
    Word depth = w.time;
    std::array<Function, 3> fn{};
    const char *kernel[]{"capillary_scalar", "capillary_bounds", "capillary_image"};
    const char *name[]{"local", "bounded", "compiled"};
    for (Word j = 0; j < 3; ++j)
        s.bind(fn[j], kernel[j], Image::runnels);
    Temporary output(s.d, Count(n) * sizeof(capillary::Bounds));
    std::vector<capillary::Bounds> original(n), checked(n);
    std::array<std::array<float, 7>, 3> timings{};
    Word lit = 0;
    for (Word lap = 0; lap < 8; ++lap)
        for (Word phase = 0; phase < 3; ++phase) {
            Word mode = (phase + lap) % 3;
            FieldTimer clock(s.d);
            clock.start(stream);
            launch(fn[mode], (n + 127) / 128, 128, m, senses, surface, projection, die, seed, n, depth, output.ptr);
            clock.stop(stream);
            synchronize();
            if (lap)
                timings[mode][lap - 1] = clock.milliseconds();
            else {
                auto &rows = mode ? checked : original;
                s.d.check(s.d.download(rows.data(), output.ptr, Count(n) * sizeof(rows[0])),
                          "compare every projected pressure field");
                if (mode && std::memcmp(original.data(), rows.data(), Count(n) * sizeof(rows[0])))
                    throw std::logic_error("compiled capillary forecasts differ from the local cortex");
                if (!mode)
                    for (auto v : rows)
                        lit += capillary::count(v.on);
            }
        }
    for (Word j = 0; j < 3; ++j) {
        auto times = timings[j];
        std::ranges::sort(times);
        std::cout << "capillary_speed=" << name[j] << "  sheets=" << n << "  depth=" << depth
                  << "  cell_steps=" << Count(64) * depth * n << "  median_ms=" << times[3]
                  << "  fast_ms=" << times.front() << "  slow_ms=" << times.back() << "  same=" << n << "  lit=" << lit
                  << "  bytes="
                  << (j == 0                                        ? sizeof(m) + sizeof(surface)
                      : j == 2 && (m.on | m.off) == capillary::full ? sizeof(die)
                                                                    : sizeof(projection))
                  << '\n';
    }
}
