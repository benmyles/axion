void record_surfaces(const Habitat &h, const Options &o) {
    Word count = std::min(o.surface_count, n / 4), seed = o.class_seed;
    Temporary output(s.d, Count(count) * sizeof(retina::Surface));
    Function gather{};
    s.bind(gather, "retina_surfaces", Image::floodplain);
    launch(gather, (count + 127) / 128, 128, world, c, tick, drift_law, seed, count, output.ptr);
    synchronize();
    std::vector<retina::Surface> observed(count);
    s.d.check(s.d.download(observed.data(), output.ptr, Count(count) * sizeof(observed[0])),
              "record the structure of actual estuary beds");
    EvidenceFile f(o.surface_output);
    f.out << "AXION-WATERCOURSES-1\n";
    Word used = 0, serial = 0, upper = 0;
    for (const auto &site : observed) {
        const auto &v = site.material;
        if (!std::ranges::any_of(v.bed.water, [](auto x) { return x != 0; }))
            continue;
        auto p = retina::as_built(site);
        if (o.only_wound && p.length <= 1)
            continue;
        ++used;
        serial += p.length > 1;
        upper += p.turns != 0;
        f.out << site.seed << ' ' << site.side << ' ' << site.tile << ' ' << site.tick << ' ' << site.law << ' '
              << drift_law.tide_shift << ' ' << v.phase << ' ' << v.rock;
        for (auto value : v.bed.water)
            f.out << ' ' << value;
        for (auto value : v.bed.clay)
            f.out << ' ' << value;
        for (auto value : v.bed.fibre)
            f.out << ' ' << value;
        f.out << '\n';
    }
    f.publish();
    std::cout << "watercourse_collection=" << h.header.tick << "  sampled=" << count << "  recorded=" << used
              << "  multiple=" << serial << "  upper=" << upper << "  seed=" << seed << '\n';
}
void sensory_surfaces(retina::Course &course, const Options &o) {
    if (o.surface_input.empty())
        return;
    if (!retinal_library) {
        auto sites = retina::read_surfaces(o.surface_input);
        retinal_sites = sites.size();
        retinal_library = std::make_unique<Temporary>(s.d, sites.size() * sizeof(Word));
        Temporary raw(s.d, sites.size() * sizeof(sites[0]));
        s.d.check(s.d.upload(raw.ptr, sites.data(), sites.size() * sizeof(sites[0])),
                  "restore the captured natural watercourses");
        Function compact{};
        s.bind(compact, "retina_compile_surfaces", Image::floodplain);
        launch(compact, (retinal_sites + 127) / 128, 128, raw.ptr, retinal_sites, retinal_library->ptr);
        synchronize();
        std::cout << "retinal_library=" << retinal_sites << "  source_bytes=" << sites.size() * sizeof(sites[0])
                  << "  device_bytes=" << sites.size() * sizeof(Word) << "  bytes_per_bed=4\n";
    }
    course.surfaces = reinterpret_cast<const Word *>(retinal_library->ptr);
    course.sites = retinal_sites;
    course.natural_order = o.natural_order;
}
