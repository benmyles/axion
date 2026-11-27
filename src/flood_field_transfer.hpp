void field_transfer(const Habitat &h, const Options &o) {
    if (!o.field_trials)
        return;
    std::vector<material::FieldPupil> donors;
    for (Word i = 0; i < n; ++i) {
        Word id = h.slots[i], age = h.header.tick - h.bodies[id].born;
        if (h.bodies[id].energy && age >= o.field_min_age && age <= o.field_max_age)
            donors.push_back({{h.forecasts[id], h.workers[id].genes}, h.enquiries[id], age, i});
    }
    if (donors.empty()) {
        std::cout << "field_transfer=vacant  donors=0\n";
        return;
    }
    Word count = donors.size(), subjects = o.work_subjects, seed = o.work_test_seed, trials = o.field_trials;
    auto opt = o.shoal;
    Word side = c.side;
    Temporary cells(s.d, Count(count) * sizeof(donors[0])),
        answers(s.d, Count(subjects) * sizeof(material::FieldTransfer));
    s.d.check(s.d.upload(cells.ptr, donors.data(), Count(count) * sizeof(donors[0])),
              "retain lived tissue for a common garden");
    std::vector<material::FieldTransfer> rows(subjects);
    Function probe{};
    s.bind(probe, "flood_field_transfer", Image::floodplain);
    const char *modes[]{"lived",       "unexperienced", "contacts_erased", "hands_erased",
                        "soil_erased", "odor_only",     "ground_only",     "ground_scrambled"};
    for (Word mode = 0; mode < std::size(modes); ++mode) {
        auto started = std::chrono::steady_clock::now();
        launch(probe, (subjects + 3) / 4, 128, cells.ptr, count, impressions.ptr, side, answers.ptr, subjects, seed,
               trials, opt, mode);
        synchronize();
        s.d.check(s.d.download(rows.data(), answers.ptr, rows.size() * sizeof(rows[0])),
                  "read transferred causal judgments and constructions");
        double duration = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        std::array<Count, sizeof(material::FieldTransfer) / sizeof(Word)> totals{};
        for (auto &r : rows) {
            auto q = std::bit_cast<std::array<Word, totals.size()>>(r);
            for (size_t j = 0; j < totals.size(); ++j)
                totals[j] += q[j];
        }
        if (totals[14] || totals[15])
            throw std::logic_error("common garden acquired energy from a disconnected patch");
        std::cout << "field_transfer=" << modes[mode] << "  donors=" << count << "  pupils=" << subjects
                  << "  trials=" << totals[0] << "  contact_choices=" << totals[1] << "  soil_choices=" << totals[2]
                  << "  unique_soils=" << totals[3] << "  working=" << totals[4] << "  two_essential=" << totals[5]
                  << "  reactions=" << totals[6] << "  other_reactions=" << totals[7] << "  exclusive=" << totals[8]
                  << "  seen_faces=" << totals[9] << "  seen_soils=" << totals[10] << "  lifting=" << totals[11]
                  << "  placing=" << totals[12] << "  traces=" << totals[13] << "  untouched=" << totals[14]
                  << "  insulated=" << totals[15] << "  seconds=" << duration << "  min_age=" << o.field_min_age
                  << "  max_age=" << o.field_max_age << '\n';
    }
}

void inquiry_effect(bool saline = false) {
    Trial actual(s, world, n, tiles), naive(s, world, n, tiles);
    Temporary cells(s.d, Count(n) * (saline ? sizeof(brine::Animal) : sizeof(material::Inquiry))),
        forecasts(s.d, saline ? 0 : Count(n) * sizeof(material::Foresight)), out(s.d, Count(n) * sizeof(HearingEffect));
    Function zero{}, copy{}, measure{};
    s.bind(copy, "flood_listen_copy", Image::floodplain);
    s.bind(measure, "flood_inquiry_effect", Image::floodplain);
    Word groups = (n + 127) / 128, unchanged = 0;
    if (saline) {
        naive.view.brine.animals = reinterpret_cast<brine::Animal *>(cells.ptr);
        s.bind(zero, "brine_quiet_memory", Image::floodplain);
        launch(zero, groups, 128, world, cells.ptr, n);
    } else {
        naive.view.worksites.residents = reinterpret_cast<material::Inquiry *>(cells.ptr);
        naive.view.tide.cells = reinterpret_cast<material::Foresight *>(forecasts.ptr);
        s.bind(zero, "flood_blank_inquiry", Image::floodplain);
        launch(zero, groups, 128, world, cells.ptr, forecasts.ptr, n);
    }
    for (Trial *trial : {&actual, &naive}) {
        launch(copy, groups, 128, world, trial->view, c, unchanged);
        launch(think, (n * tiles + 127) / 128, 128, trial->view, c, clock.ptr, unchanged);
    }
    launch(measure, groups, 128, world, actual.view, naive.view, c, out.ptr);
    synchronize();
    std::vector<HearingEffect> rows(n);
    s.d.check(s.d.download(rows.data(), out.ptr, rows.size() * sizeof(rows[0])), "read matched somatic replay");
    Count alive = 0, changed = 0, spikes = 0, motor = 0, storage = 0;
    for (auto &r : rows) {
        alive += r.experienced;
        changed += r.inputs;
        spikes += r.spikes;
        motor += r.motor;
        storage += r.contacts;
    }
    std::cout << (saline ? "brine_replay=" : "inquiry_replay=") << alive << "  changed_inputs=" << changed
              << "  different_spikes=" << spikes << "  changed_motors=" << motor << "  recurrent_bits=" << storage
              << '\n';
}

void worksite_bench(const Habitat &h, Word calls) {
    if (!calls)
        return;
    std::vector<material::FieldPupil> occupants;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            occupants.push_back({{h.forecasts[i], h.workers[i].genes}, h.enquiries[i]});
    if (occupants.empty()) {
        std::cout << "worksite_deployment=0  calls=0\n";
        return;
    }
    Word donors = occupants.size();
    Temporary cells(s.d, occupants.size() * sizeof(occupants[0])), pairs(s.d, 4 * Count(donors)),
        moves(s.d, 4 * Count(donors)), odor(s.d, donors), plastic(s.d, 4 * Count(calls)), frozen(s.d, 4 * Count(calls));
    s.d.check(s.d.upload(cells.ptr, occupants.data(), occupants.size() * sizeof(occupants[0])),
              "deploy field-experienced circuits");
    Function encode{}, predict[2]{};
    s.bind(encode, "flood_worksite_image", Image::floodplain);
    s.bind(predict[0], "flood_worksite_writable", Image::floodplain);
    s.bind(predict[1], "flood_worksite_packed", Image::floodplain);
    launch(encode, (donors + 127) / 128, 128, cells.ptr, donors, pairs.ptr, moves.ptr, odor.ptr);
    synchronize();
    std::vector<Word> pair(donors), motor(donors);
    std::vector<unsigned char> odors(donors);
    s.d.check(s.d.download(pair.data(), pairs.ptr, pair.size() * 4), "verify contact planes");
    s.d.check(s.d.download(motor.data(), moves.ptr, motor.size() * 4), "verify effector planes");
    s.d.check(s.d.download(odors.data(), odor.ptr, odors.size()), "verify basal odors");
    for (Word i = 0; i < donors; ++i) {
        auto &p = occupants[i];
        auto fires = [&](Word row, Word bit) { return (p.mind.genes.release >> ((row >> (2 * bit)) & 3)) & 1; };
        for (Word j = 0; j < 32; ++j) {
            if (((pair[i] >> j) & 1) != fires(j < 16 ? p.mind.memory.contacts : p.exploratory.chemistry, j & 15) ||
                ((motor[i] >> j) & 1) != fires(p.mind.memory.muscles[j >> 4], j & 15))
                throw std::logic_error("field deployment changed a receptive cell");
        }
        for (Word j = 0; j < 4; ++j)
            if (((odors[i] >> j) & 1) != fires(p.mind.memory.appetite, j))
                throw std::logic_error("field deployment changed an odor");
    }
    cuda::Event start{}, end{};
    s.d.check(s.d.event_new(&start, 0), "create field timer");
    s.d.check(s.d.event_new(&end, 0), "create completion timer");
    std::array<float, 7> readings[2]{};
    for (Word t = 0; t < 8; ++t)
        for (Word j = 0; j < 2; ++j) {
            Word mode = (t + j) & 1, key = mix(t + 319);
            auto &out = mode ? frozen : plastic;
            s.d.check(s.d.record(start, stream), "begin cortical replay");
            launch(predict[mode], (calls + 3) / 4, 128, cells.ptr, donors, pairs.ptr, moves.ptr, odor.ptr, calls, key,
                   out.ptr);
            s.d.check(s.d.record(end, stream), "close cortical replay");
            s.d.check(s.d.wait(end), "finish cortical replay");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, start, end), "time paired field forecasts");
            if (t)
                readings[mode][t - 1] = ms;
        }
    s.d.event_free(start);
    s.d.event_free(end);
    std::vector<Word> a(calls), b(calls);
    s.d.check(s.d.download(a.data(), plastic.ptr, a.size() * 4), "read mutable choices");
    s.d.check(s.d.download(b.data(), frozen.ptr, b.size() * 4), "read frozen choices");
    if (a != b)
        throw std::logic_error("worksite image changed a spatial prediction");
    for (auto &v : readings)
        std::sort(v.begin(), v.end());
    std::cout << "worksite_deployment=" << donors << "  calls=" << calls << "  comparisons=" << 68 * Count(donors)
              << "  differences=0  receptive_bytes=9  mutable_bytes="
              << sizeof(material::Foresight) + sizeof(material::Inquiry) << "  mutable_ms=" << readings[0][3]
              << "  median_ms=" << readings[1][3] << "  Mmaterial_diagrams_per_s=" << calls / (1000 * readings[1][3])
              << '\n';
}
