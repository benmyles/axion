void pod_assay(Habitat &h, Word subjects, Word rounds, Word seed) {
    if (!rounds)
        return;
    // The probe is read-only. Copy the actual adults' inherited rules and
    // hatch new tissue on independently keyed physical contingencies.
    std::vector<pods::Animal> adults(n);
    std::vector<pods::Genes> pool;
    s.d.check(s.d.download(adults.data(), pod_tissue.ptr, Count(n) * sizeof(pods::Animal)),
              "sample delayed learning germlines");
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            pool.push_back(adults[i].genes);
    if (pool.empty()) {
        std::cout << "pod_transfers=0  living_donors=0\n";
        return;
    }
    Temporary dna(s.d, pool.size() * sizeof(pods::Genes)), outputs(s.d, subjects * sizeof(pods::Audit)),
        local(s.d, n * sizeof(pods::LocalAudit));
    s.d.check(s.d.upload(dna.ptr, pool.data(), pool.size() * sizeof(pool[0])),
              "send independently sampled crop parents");
    Function assay{}, field{};
    s.bind(assay, "flood_pod_assay", Image::floodplain);
    s.bind(field, "flood_pod_local", Image::floodplain);
    Word count = Word(pool.size());
    cuda::Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "create crop timer");
    s.d.check(s.d.event_new(&end, 0), "create crop timer");
    for (Word forced = 0; forced <= 1; ++forced) {
        s.d.check(s.d.record(start, stream), "start delayed training timer");
        launch(assay, (subjects + 31) / 32, 32, dna.ptr, count, outputs.ptr, subjects, rounds, seed, world.pods.physics,
               forced);
        s.d.check(s.d.record(end, stream), "stop crop timer");
        s.d.check(s.d.wait(end), "finish crop assay");
        float ms{};
        s.d.check(s.d.elapsed(&ms, start, end), "measure crop training");
        std::vector<pods::Audit> scores(subjects);
        s.d.check(s.d.download(scores.data(), outputs.ptr, scores.size() * sizeof(scores[0])),
                  "read generalization controls");
        Count before = 0, surprise = 0, opened = 0, bites = 0, delivered = 0, collisions = 0, held = 0, alone = 0,
              shadow[2]{}, passed[2]{}, yoke[2]{}, erased[2]{}, shifted[2]{};
        for (auto r : scores) {
            before += r.before;
            surprise += r.surprise;
            opened += r.opened;
            bites += r.grasps;
            delivered += r.delivered;
            collisions += r.coincident;
            held += r.held;
            alone += r.alone;
            for (Word j = 0; j < 2; ++j) {
                shadow[j] += r.shadow[j];
                passed[j] += r.live[j];
                yoke[j] += r.yoked[j];
                erased[j] += r.cleared[j];
                shifted[j] += r.displaced[j];
            }
        }
        std::cout << "pod_transfers=" << subjects << "  practice=" << (forced ? "imposed" : "self")
                  << "  living_donors=" << count << "  rounds=" << rounds << "  probes=" << Count(subjects) * 64
                  << "  naive=" << before << "  practiced=" << passed[0] << "  surprise=" << surprise
                  << "  relearned=" << passed[1] << "  yoked=" << yoke[0] << "  yoked_reversal=" << yoke[1]
                  << "  erased=" << erased[0] << "  erased_reversal=" << erased[1] << "  displaced=" << shifted[0]
                  << "  displaced_reversal=" << shifted[1] << "  opened=" << opened << "  attempts=" << bites
                  << "  deliveries=" << delivered << "  coincident=" << collisions << "  imitated=" << shadow[0]
                  << "  imitated_reversal=" << shadow[1] << "  held=" << held << "  isolated=" << alone
                  << "  pod_presentations_per_s=" << Count(2) * subjects * rounds * 1000.0 / ms
                  << "  assay_gpu_ms=" << ms << '\n';
    }
    s.d.event_free(start);
    s.d.event_free(end);
    launch(field, (n + 127) / 128, 128, world, c, clock.ptr, local.ptr);
    synchronize();
    std::vector<pods::LocalAudit> rows(n);
    s.d.check(s.d.download(rows.data(), local.ptr, n * sizeof(rows[0])),
              "observe a native bite with and without its acquired state");
    Count alive = 0, experienced = 0, warm = 0, cold = 0, changed = 0, gain = 0, loss = 0;
    for (auto r : rows) {
        alive += r.alive;
        experienced += r.arrived;
        warm += r.live;
        cold += r.cold;
        changed += r.changed;
        gain += r.gained;
        loss += r.lost;
    }
    std::cout << "pod_wild_replay=" << alive << "  experienced=" << experienced << "  intact=" << warm
              << "  erased=" << cold << "  changed=" << changed << "  gained=" << gain << "  lost=" << loss << '\n';
}

void pod_remap(Habitat &h, const Options &o) {
    if (!o.remap_training)
        return;
    std::vector<pods::Animal> living(n);
    std::vector<pods::Inheritance> germ;
    s.d.check(s.d.download(living.data(), pod_tissue.ptr, n * sizeof(living[0])),
              "sample delayed and motor frame germlines");
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            germ.push_back({living[i].genes, living[i].frame_genes});
    if (germ.empty()) {
        std::cout << "pod_remap=0  donors=0\n";
        return;
    }
    Word count = germ.size(), subjects = o.pod_subjects, calibrate = o.remap_training, observe = o.remap_observe,
         cues = o.remap_cues, seed = o.pod_test_seed;
    Temporary dna(s.d, germ.size() * sizeof(germ[0])), out(s.d, subjects * sizeof(pods::Transfer));
    s.d.check(s.d.upload(dna.ptr, germ.data(), germ.size() * sizeof(germ[0])), "introduce motor frames");
    Function examine{};
    s.bind(examine, "flood_pod_transfer", Image::floodplain);
    for (Word forced = 0; forced < 2; ++forced) {
        launch(examine, (subjects + 31) / 32, 32, dna.ptr, count, out.ptr, subjects, calibrate, observe, cues, seed,
               forced, world.pods.physics);
        synchronize();
        std::vector<pods::Transfer> rows(subjects);
        s.d.check(s.d.download(rows.data(), out.ptr, rows.size() * sizeof(rows[0])), "read untouched-fruit responses");
        std::array<Count, sizeof(pods::Transfer) / sizeof(Word)> total{};
        for (auto r : rows) {
            auto fields = std::bit_cast<std::array<Word, total.size()>>(r);
            for (size_t i = 0; i < fields.size(); ++i)
                total[i] += fields[i];
        }
        const char *columns[]{"unseen",
                              "calibrated",
                              "before",
                              "intact",
                              "yoked",
                              "silent",
                              "home_frame",
                              "frame_only",
                              "independent",
                              "different_arm_probes",
                              "different_arm_correct",
                              "available",
                              "switched",
                              "anchored"};
        static_assert(std::size(columns) == std::tuple_size_v<decltype(total)>);
        std::cout << "pod_remap=" << subjects << "  practice=" << (forced ? "imposed" : "self")
                  << "  presented_cues=" << cues << "  exposures=" << observe;
        for (size_t j = 0; j < total.size(); ++j)
            std::cout << "  " << columns[j] << '=' << total[j];
        std::cout << '\n';
    }
}
