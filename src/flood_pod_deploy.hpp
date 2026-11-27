void motor_benchmark(const Habitat &h, Word calls) {
    if (!calls)
        return;
    std::vector<Word> living;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            living.push_back(i | (Word(!(pods::sensory_channels(world.pods.physics, h.bodies[i].cohort) & 4)) << 31));
    if (living.empty()) {
        std::cout << "pod_deployment=0  calls=0\n";
        return;
    }
    Word residents = living.size(), framed = bool(world.pods.physics.background & 4);
    Temporary slots(s.d, Count(residents) * sizeof(Word)), images(s.d, Count(residents) * sizeof(pods::MotorImage)),
        output(s.d, Count(calls) * sizeof(Word)), errors(s.d, sizeof(Word));
    s.d.check(s.d.upload(slots.ptr, living.data(), living.size() * sizeof(Word)), "index living motor circuits");
    s.d.check(s.d.zero(errors.ptr, 0, sizeof(Word)), "clear motor comparison");
    Function compile{}, audit{}, time_code[2]{};
    s.bind(compile, "flood_pod_compile", Image::floodplain);
    s.bind(audit, "flood_pod_validate", Image::floodplain);
    s.bind(time_code[0], "flood_pod_mutable_calls", Image::floodplain);
    s.bind(time_code[1], "flood_pod_image_calls", Image::floodplain);
    launch(compile, (residents + 127) / 128, 128, pod_tissue.ptr, slots.ptr, images.ptr, residents, framed);
    Word checked = std::min(256u, residents);
    launch(audit, 2 * checked, 128, pod_tissue.ptr, slots.ptr, images.ptr, checked, framed, errors.ptr);
    synchronize();
    Word wrong = 0;
    s.d.check(s.d.download(&wrong, errors.ptr, sizeof(wrong)), "verify every sensory and motor input");
    if (wrong)
        throw std::logic_error("settled motor inference disagrees with the independent cellular interpreter");
    cuda::Event begin{}, end{};
    s.d.check(s.d.event_new(&begin, 0), "create motor timer");
    s.d.check(s.d.event_new(&end, 0), "create motor fence");
    std::array<float, 9> times[2]{};
    // Interleave representations to balance changes in mobile GPU frequency.
    for (Word trial = 0; trial < 10; ++trial)
        for (Word j = 0; j < 2; ++j) {
            Word mode = (trial + j) & 1, seed = mix(98113 + trial);
            s.d.check(s.d.record(begin, stream), "start motor batch");
            launch(time_code[mode], (calls + 127) / 128, 128, pod_tissue.ptr, slots.ptr, images.ptr, residents, calls,
                   seed, framed, output.ptr);
            s.d.check(s.d.record(end, stream), "stop motor batch");
            s.d.check(s.d.wait(end), "finish motor batch");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, begin, end), "measure muscle decisions");
            if (trial)
                times[mode][trial - 1] = ms;
        }
    s.d.event_free(begin);
    s.d.event_free(end);
    for (auto &series : times)
        std::sort(series.begin(), series.end());
    std::cout << "pod_deployment=" << residents << "  calls=" << calls << "  audited_animals=" << checked
              << "  scalar_comparisons=" << Count(checked) * 8192 << "  differences=" << wrong
              << "  motor_image_bytes=" << sizeof(pods::MotorImage) << "  mutable_motor_bytes="
              << sizeof(pods::Genes) + sizeof(pods::Circuit) + sizeof(pods::FrameGenes) + sizeof(pods::Frame)
              << "  mutable_ms=" << times[0][4] << "  median_ms=" << times[1][4]
              << "  Mmuscle_choices_per_s=" << calls / (times[1][4] * 1000.0)
              << "  speedup=" << times[0][4] / times[1][4] << '\n';
}
