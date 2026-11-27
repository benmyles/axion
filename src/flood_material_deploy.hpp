void hand_bench(const Habitat &h, Word calls) {
    if (!calls)
        return;
    std::vector<Word> live;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            live.push_back(i);
    if (live.empty()) {
        std::cout << "construction_deployment=0  trials=0\n";
        return;
    }
    Word donors = live.size(), tested = std::min(256u, donors);
    Temporary slots(s.d, Count(donors) * sizeof(Word)), images(s.d, Count(donors) * sizeof(material::HandImage)),
        out(s.d, Count(calls) * sizeof(Word)), error(s.d, sizeof(Word));
    s.d.check(s.d.upload(slots.ptr, live.data(), live.size() * sizeof(Word)), "select actual manipulating residents");
    s.d.check(s.d.zero(error.ptr, 0, sizeof(Word)), "clear construction comparator");
    Function encode{}, compare{}, kernel[2]{};
    s.bind(encode, "flood_hand_encode", Image::floodplain);
    s.bind(compare, "flood_hand_check", Image::floodplain);
    s.bind(kernel[0], "flood_hand_plastic", Image::floodplain);
    s.bind(kernel[1], "flood_hand_compact", Image::floodplain);
    launch(encode, (donors + 127) / 128, 128, workers.ptr, slots.ptr, images.ptr, donors);
    launch(compare, 2 * tested, 128, workers.ptr, slots.ptr, images.ptr, tested, error.ptr);
    synchronize();
    Word wrong = 0;
    s.d.check(s.d.download(&wrong, error.ptr, sizeof(wrong)), "compare intact and compiled hand choices");
    if (wrong)
        throw std::logic_error("compiled construction choices differ from plastic tissue");
    cuda::Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "create hand choice timer");
    s.d.check(s.d.event_new(&end, 0), "create hand completion timer");
    std::array<float, 9> elapsed[2]{};
    for (Word trial = 0; trial < 10; ++trial)
        for (Word j = 0; j < 2; ++j) {
            Word mode = (j + trial) & 1, seed = mix(19871 + trial);
            s.d.check(s.d.record(start, stream), "start hand choice batch");
            launch(kernel[mode], (calls + 127) / 128, 128, workers.ptr, slots.ptr, images.ptr, donors, calls, seed,
                   out.ptr);
            s.d.check(s.d.record(end, stream), "stop hand choice batch");
            s.d.check(s.d.wait(end), "finish hand choice batch");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, start, end), "measure visual to muscle mapping");
            if (trial)
                elapsed[mode][trial - 1] = ms;
        }
    s.d.event_free(start);
    s.d.event_free(end);
    for (auto &set : elapsed)
        std::sort(set.begin(), set.end());
    std::cout << "construction_deployment=" << donors << "  calls=" << calls
              << "  comparisons=" << Count(tested) * 32768 << "  differences=" << wrong
              << "  hand_image_bytes=" << sizeof(material::HandImage)
              << "  mutable_worker_bytes=" << sizeof(material::Worker) << "  mutable_ms=" << elapsed[0][4]
              << "  median_ms=" << elapsed[1][4] << "  Mconstruction_choices_per_s=" << calls / (1000.0 * elapsed[1][4])
              << '\n';
}
