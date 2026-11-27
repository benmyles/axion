void report_dispatch(const Habitat &h) {
    if (!h.developing || h.cortices.size() != n || h.slots.size() != n)
        return;
    CellWidths widths{};
    auto shape = neural_shape(h, &widths);
    std::cout << "neural_dispatch=" << (packed_brains ? "packed" : "fixed") << "  sites=" << n
              << "  capacity_bands=" << tiles << "  living=" << shape.living << "  expressed_lanes=" << shape.expressed
              << "  scheduled_bands=" << shape.scheduled << "  irregular=" << shape.irregular
              << "  fixed_live_lanes=" << shape.living * tiles << "  packed_round_lanes=" << shape.rounds * 32
              << "  fixed_work_warps=" << shape.fixed_warps << "  global_schedule_bytes=0" << '\n';
    Word seen = 0, median = 0, maximum = 0;
    for (Word w = 1; w <= tiles; ++w) {
        seen += widths[w];
        if (widths[w])
            maximum = 32 * w;
        if (!median && seen && 2ull * seen >= shape.living)
            median = 32 * w;
    }
    std::cout << "neural_widths=" << h.header.tick << "  living=" << shape.living
              << "  tissue_cells=" << shape.scheduled * 32 << "  conducting_cells=" << shape.expressed * 32
              << "  median_cells=" << median << "  max_cells=" << maximum << "  capacity_cells=" << tiles * 32
              << "  at_capacity=" << widths[tiles] << "  by_32_cells=";
    for (Word w = 1; w <= tiles; ++w)
        std::cout << (w == 1 ? "" : ",") << widths[w];
    std::cout << '\n';
}
void bench_dispatch(Word steps) {
    if (!world.cortex.resident)
        throw std::invalid_argument("the neural dispatch comparison needs a developing cortex");
    Function kernel[2]{};
    s.bind(kernel[0], "flood_think_" + std::to_string(tiles), Image::floodplain);
    s.bind(kernel[1], "flood_think_packed", Image::floodplain);
    Trial fixed(s, world, n, tiles), packed(s, world, n, tiles);
    std::array<Trial *, 2> copies{&fixed, &packed};
    struct Buffer {
        DevicePtr initial, first, second;
        size_t bytes;
    };
    Buffer buffers[]{{animals.ptr, fixed.bodies.ptr, packed.bodies.ptr, Count(n) * sizeof(Colony)},
                     {nerves.ptr, fixed.nerve.ptr, packed.nerve.ptr, Count(n) * tiles * sizeof(Tissue)},
                     {ears.ptr, fixed.ear.ptr, packed.ear.ptr, Count(n) * sizeof(Ear)},
                     {motors.ptr, fixed.motor.ptr, packed.motor.ptr, n}};
    Event start{}, finish{};
    s.d.check(s.d.event_new(&start, 0), "create neural benchmark clock");
    s.d.check(s.d.event_new(&finish, 0), "create end of neural benchmark clock");
    std::array<float, 5> times[2]{};
    for (Word trial = 0; trial < 6; ++trial) {
        synchronize();
        for (auto &block : buffers) {
            if (block.initial == nerves.ptr) {
                clone_neural(block.first);
                clone_neural(block.second);
                continue;
            }
            s.d.check(s.d.copy_device(block.first, block.initial, block.bytes), "restore fixed subgroups");
            s.d.check(s.d.copy_device(block.second, block.initial, block.bytes), "restore packed subgroups");
        }
        for (Word i = 0; i < 2; ++i) {
            Word mode = (i + trial) & 1;
            s.d.check(s.d.record(start, stream), "begin fixed-sensory neural ticks");
            for (Word offset = 0; offset < steps; ++offset) {
                Word blocks = (n * (mode ? 1u : tiles) + 127) / 128;
                launch(kernel[mode], blocks, 128, copies[mode]->view, c, clock.ptr, offset);
            }
            s.d.check(s.d.record(finish, stream), "end fixed-sensory neural ticks");
            synchronize();
            if (trial)
                s.d.check(s.d.elapsed(&times[mode][trial - 1], start, finish), "measure the neural kernel");
        }
    }
    s.d.event_free(start);
    s.d.event_free(finish);
    std::vector<unsigned char> a(128 * 1024), b(a.size());
    size_t checked = 0;
    for (auto &block : buffers) {
        for (size_t begin = 0; begin < block.bytes; begin += a.size()) {
            size_t bytes = std::min(a.size(), block.bytes - begin);
            s.d.check(s.d.download(a.data(), block.first + begin, bytes), "read fixed neural result");
            s.d.check(s.d.download(b.data(), block.second + begin, bytes), "read packed neural result");
            if (!std::equal(a.begin(), a.begin() + bytes, b.begin()))
                throw std::runtime_error("packed cortical step changed neural state, sensory history or motor output");
        }
        checked += block.bytes;
    }
    for (auto &sample : times)
        std::sort(sample.begin(), sample.end());
    int registers[2]{}, local[2]{};
    for (int i = 0; i < 2; ++i) {
        s.d.check(s.d.func_attr(&registers[i], 4, kernel[i]), "count cortical registers");
        s.d.check(s.d.func_attr(&local[i], 3, kernel[i]), "count local cortical memory");
    }
    std::cout << "neural_benchmark=" << steps << "  sites=" << n << "  fixed_ms=" << times[0][2]
              << "  packed_ms=" << times[1][2] << "  shadow_memory=dense  ratio=" << times[0][2] / times[1][2]
              << "  compared_bytes=" << checked << "  fixed_registers=" << registers[0]
              << "  packed_registers=" << registers[1] << "  fixed_local=" << local[0] << "  packed_local=" << local[1]
              << '\n';
}
