Word hollow_optics(const Habitat &h, const Options &o) {
    if (!o.hollow_optics.empty()) {
        storage::AgentImage animal;
        animal.read(o.hollow_optics);
        if (!animal.potter)
            throw std::invalid_argument("optics source lacks pressure tissue");
        return press::germ(animal.ceramic.mind);
    }
    if (h.pottery)
        for (Word i = 0; i < n; ++i)
            if (h.bodies[i].energy)
                return press::germ(h.potters[i]);
    clay::Mind germ{clay::founder(c.seed)};
    return press::germ(germ);
}
void hollow_rear(Habitat &h, const Options &o) {
    if (!o.hollow_seasons && o.hollow_origin.empty())
        return;
    sample(h);
    Word count = o.hollow_population, arena = h.recess.slots, seed = o.hollow_seed, paid = o.hollow_paid;
    std::vector<hollow::Family> families(count);
    std::vector<hollow::Genes> living;
    if (!o.hollow_origin.empty()) {
        Habitat source;
        source.read(o.hollow_origin);
        if (source.ranging)
            for (Word i = 0; i < source.sites(); ++i)
                if (source.bodies[i].energy)
                    living.push_back(source.wanderers[i].genes);
        if (living.empty())
            throw std::invalid_argument("no living spatial germlines in source");
    } else if (!o.hollow_varied) {
        for (Word i = 0; i < n; ++i)
            if (h.bodies[i].energy)
                living.push_back(h.wanderers[i].genes);
    }
    for (Word i = 0; i < count; ++i) {
        Word key = mix(seed + i * 5297);
        families[i].genes = living.empty() ? hollow::founder(key) : living[key % living.size()];
    }
    if (o.hollow_seasons) {
        Word eye = hollow_optics(h, o);
        Temporary first(s.d, Count(count) * sizeof(families[0])), next(s.d, Count(count) * sizeof(families[0]));
        s.d.check(s.d.upload(first.ptr, families.data(), Count(count) * sizeof(families[0])),
                  "fund new spatial cohorts");
        Function live{}, born{};
        s.bind(live, "hollow_lives", Image::siphons);
        s.bind(born, "hollow_births", Image::siphons);
        for (Word epoch = 0; epoch < o.hollow_seasons; ++epoch) {
            launch(live, (count + 127) / 128, 128, first.ptr, count, eye, arena, epoch, seed);
            if (!epoch || (epoch + 1) % 64 == 0 || epoch + 1 == o.hollow_seasons) {
                synchronize();
                s.d.check(s.d.download(families.data(), first.ptr, Count(count) * sizeof(families[0])),
                          "read physical reproductive budgets");
                Count eggs = 0;
                Word deepest = 0;
                for (auto &f : families) {
                    eggs += f.allowance;
                    deepest = std::max(deepest, f.allowance);
                }
                std::cout << "hollow_season=" << epoch + 1 << "  adults=" << count << "  eggs=" << eggs
                          << "  largest=" << deepest << "  earned=" << paid << '\n';
            }
            launch(born, (count / 2 + 127) / 128, 128, first.ptr, next.ptr, count, seed, epoch, paid);
            std::swap(first.ptr, next.ptr);
        }
        synchronize();
        s.d.check(s.d.download(families.data(), first.ptr, Count(count) * sizeof(families[0])),
                  "recover newborn spatial germlines");
    }
    for (Word i = 0; i < n; ++i)
        h.wanderers[i] = {families[mix(seed + i * 1087) & (count - 1)].genes};
    std::fill(h.bearings.begin(), h.bearings.end(), 0);
    std::fill(h.sightings.begin(), h.sightings.end(), 0);
    s.d.check(s.d.upload(path_brains.ptr, h.wanderers.data(), Count(n) * sizeof(hollow::Mind)),
              "introduce empty spatial organs");
    s.d.check(s.d.zero(path_marks.ptr, 0, Count(n) * arena * 4), "clear previous event traces");
    s.d.check(s.d.zero(path_sites.ptr, 0, Count((n + 15) / 16) * arena * 4), "reset independent birth records");
}
void hollow_exam(const Habitat &h, const Options &o) {
    if (!o.hollow_test)
        return;
    std::vector<hollow::Genes> germs;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy)
            germs.push_back(h.wanderers[i].genes);
    if (germs.empty()) {
        std::cout << "hollow_probe=vacant\n";
        return;
    }
    Word ng = germs.size(), tasks = o.hollow_test, seed = mix(o.hollow_seed + 127883), arena = h.recess.slots,
         eye = hollow_optics(h, o);
    Temporary laws(s.d, Count(ng) * sizeof(germs[0])), results(s.d, Count(tasks) * sizeof(hollow::Trip)),
        detail(s.d, sizeof(hollow::Proof));
    s.d.check(s.d.upload(laws.ptr, germs.data(), Count(ng) * sizeof(germs[0])), "place actual spatial germlines");
    Function ask{};
    s.bind(ask, "hollow_probe", Image::siphons);
    std::vector<hollow::Trip> sample(tasks);
    const char *names[]{"walked",          "reanchored",     "prior_water",     "places_erased",   "dry_unseen",
                        "water_unsampled", "still_odometer", "skewed_odometer", "landmark_hidden", "landmark_exchanged",
                        "one_cell",        "moved_stores",   "unchanged_water"};
    for (Word mode = 0; mode < hollow::trial_count; ++mode) {
        launch(ask, (tasks + 127) / 128, 128, laws.ptr, ng, eye, arena, tasks, seed, mode, results.ptr, detail.ptr);
        synchronize();
        s.d.check(s.d.download(sample.data(), results.ptr, Count(tasks) * sizeof(sample[0])),
                  "measure completed fetches");
        std::array<Count, sizeof(hollow::Trip) / sizeof(Word)> sum{};
        for (auto &r : sample) {
            auto q = std::bit_cast<std::array<Word, sum.size()>>(r);
            for (size_t j = 0; j < sum.size(); ++j)
                sum[j] += q[j];
        }
        std::cout << "hollow_trip=" << names[mode] << "  trials=" << sum[0] << "  grasped=" << sum[1]
                  << "  working_tool=" << sum[2] << "  fed=" << sum[3] << "  carbon=" << sum[4] << "  hulls=" << sum[5]
                  << "  work=" << sum[6] << "  reinstated=" << sum[8] << "  cells=" << sum[13] << "  firing=" << sum[14]
                  << "  returned=" << sum[16] << "  guided_steps=" << sum[10] << '\n';
        if (mode == hollow::refound) {
            hollow::Proof q{};
            s.d.check(s.d.download(&q, detail.ptr, sizeof(q)), "read a spatial trace");
            std::cout << "hollow_path=" << q.seed << "  bath=" << q.bath << "  earlier=" << q.old_gate
                      << "  later=" << q.gate << "  material=" << q.tones << "  forecast=" << q.forecast
                      << "  water=" << q.water << "  chosen=" << q.chosen << "  delivered=" << q.visits
                      << "  start_x=" << q.start.x << "  start_y=" << q.start.y << "  head=" << q.start.head
                      << "  finish_x=" << q.finish.x << "  finish_y=" << q.finish.y << "  x=" << Word(q.cortex.x)
                      << "  y=" << Word(q.cortex.y) << "  yaw=" << Word(q.cortex.yaw)
                      << "  used=" << Word(q.cortex.used) << "  synapse=" << q.genome[0] << "  compass=" << q.genome[1]
                      << "  readout=" << q.genome[2] << "  gates=" << q.genome[3] << '\n';
            for (Word i = 0; i < 6; ++i)
                std::cout << "hollow_site=" << i << "  x=" << q.sites[i].x << "  y=" << q.sites[i].y
                          << "  facade=" << Word(q.sites[i].relief) << "  hue=" << Word(q.sites[i].tint) << '\n';
            for (Word i = 0; i < q.cortex.used; ++i)
                std::cout << "hollow_trace=" << i << "  cell=" << q.memory[i] << '\n';
            for (Word i = 0; i < 32; ++i)
                if (q.track[i])
                    std::cout << "hollow_step=" << i << "  body=" << q.track[i] << '\n';
        }
    }
}
void hollow_census(const Options &o, Word tick) {
    if (!o.hollow_survey)
        return;
    Temporary records(s.d, Count(n) * sizeof(hollow::FieldTest));
    std::vector<hollow::FieldTest> rows(n);
    Function probe{};
    s.bind(probe, "hollow_catchment", Image::floodplain);
    Word seed = o.hollow_seed;
    for (Word minimum : {1u, 3u, 5u}) {
        launch(probe, (n + 127) / 128, 128, world, c, seed, tick, minimum, records.ptr);
        synchronize();
        s.d.check(s.d.download(rows.data(), records.ptr, Count(n) * sizeof(rows[0])),
                  "read independently located opportunities");
        std::array<Count, sizeof(hollow::FieldTest) / 4> t{};
        for (auto &r : rows) {
            auto q = std::bit_cast<std::array<Word, t.size()>>(r);
            for (size_t k = 0; k < t.size(); ++k)
                t[k] += q[k];
        }
        std::cout << "hollow_range=" << minimum << "  residents=" << t[0] << "  goals=" << t[1] << "  exact=" << t[2]
                  << "  rotated_exact=" << t[3] << "  stored_carbon=" << t[4] << "  rotated_carbon=" << t[5]
                  << "  work_distance=" << t[6] << "  certified=" << t[7] << "  cavities=" << t[8]
                  << "  rotated_cavities=" << t[9] << "  matching=" << t[10] << "  rotated_match=" << t[11]
                  << "  actual_source_carbon=" << t[12] << "  fruit=" << t[13] << "  rotated_fruit=" << t[14] << '\n';
    }
}

void hollow_scholar(const Habitat &h, const Options &o) {
    if (o.hollow_scholar.empty()) {
        if (!o.hollow_carrier.empty())
            throw std::invalid_argument("a spatial carrier needs an exported scholar");
        return;
    }
    std::vector<Word> ids;
    std::vector<hollow::Genes> candidates;
    for (Word i = 0; i < n; ++i) {
        const auto &b = h.bodies[i];
        if (b.energy > 24 && b.body.limbs && b.body.pores && b.body.eyes) {
            ids.push_back(i);
            candidates.push_back(h.wanderers[i].genes);
        }
    }
    if (ids.empty())
        throw std::invalid_argument("no living resident can walk, taste and see");
    Word count = ids.size(), seed = mix(o.hollow_seed + 35423), arena = h.recess.slots, eye = hollow_optics(h, o);
    Temporary laws(s.d, Count(count) * sizeof(hollow::Genes)), score(s.d, Count(count) * 4);
    s.d.check(s.d.upload(laws.ptr, candidates.data(), Count(count) * sizeof(hollow::Genes)),
              "prepare independent spatial phenotypes");
    Function rate{};
    s.bind(rate, "hollow_candidates", Image::siphons);
    launch(rate, (count + 127) / 128, 128, laws.ptr, count, eye, arena, seed, score.ptr);
    synchronize();
    std::vector<Word> points(count);
    s.d.check(s.d.download(points.data(), score.ptr, Count(count) * 4), "read actual retrievals");
    Word at = std::max_element(points.begin(), points.end()) - points.begin();
    auto pupil = extract_individual(h, ids[at], false);
    if (!o.hollow_carrier.empty()) {
        pupil.read(o.hollow_carrier);
        if (!pupil.calling || !pupil.potter || !pupil.resident.animal.body.limbs || !pupil.resident.animal.body.eyes)
            throw std::invalid_argument("incompatible spatial carrier");
        pupil.ranges = true;
        pupil.path_memory = {};
        pupil.path_memory.slot_limit = arena;
        pupil.path_memory.mind.genes = candidates[at];
        pupil.path_speech = {};
    }
    storage::write_checked(o.hollow_scholar, pupil);
    std::cout << "hollow_scholar=" << ids[at] << "  candidates=" << count << "  merit=" << points[at]
              << "  outings=12  seed=" << seed << "  carrier=" << (!o.hollow_carrier.empty()) << '\n';
}
void hollow_bench(const Habitat &h, const Options &o) {
    Word calls = o.hollow_calls;
    if (!calls)
        return;
    std::vector<Word> owners;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy && h.wanderers[i].used)
            owners.push_back(i);
    if (owners.empty()) {
        std::cout << "hollow_deploy=vacant\n";
        return;
    }
    Word count = owners.size(), tick = h.header.tick, tie = o.hollow_seed, batch = std::min(calls, 1u << 20);
    Temporary ids(s.d, Count(count) * 4), image(s.d, Count(count) * sizeof(hollow::Index)), a(s.d, Count(batch) * 8),
        b(s.d, Count(batch) * 8), errors(s.d, 4);
    s.d.check(s.d.upload(ids.ptr, owners.data(), Count(count) * 4), "select residents with occupied maps");
    s.d.check(s.d.zero(errors.ptr, 0, 4), "clear disagreement counter");
    Function build{}, online{}, indexed{}, compare{};
    s.bind(build, "hollow_indices", Image::siphons);
    s.bind(online, "hollow_online", Image::siphons);
    s.bind(indexed, "hollow_indexed", Image::siphons);
    s.bind(compare, "hollow_compare", Image::siphons);
    launch(build, (count + 127) / 128, 128, world.hollows, image.ptr, ids.ptr, count, tick, tie);
    cuda::Event begin{}, end{};
    s.d.check(s.d.event_new(&begin, 0), "start spatial clock");
    s.d.check(s.d.event_new(&end, 0), "stop spatial clock");
    std::array<float, 5> times[2]{};
    for (Word rep = 0; rep < 6; ++rep)
        for (Word done = 0; done < calls; done += batch) {
            Word length = std::min(batch, calls - done), groups = (length + 127) / 128,
                 seed = mix(tie + rep * 373 + done);
            for (Word run = 0; run < 2; ++run) {
                Word k = (rep + run) & 1;
                auto &out = k ? b : a;
                s.d.check(s.d.record(begin, stream), "time recall start");
                launch(k ? indexed : online, groups, 128, world.hollows, image.ptr, ids.ptr, count, tick, tie, length,
                       seed, out.ptr);
                s.d.check(s.d.record(end, stream), "time recall end");
                s.d.check(s.d.wait(end), "finish spatial choices");
                float ms = 0;
                s.d.check(s.d.elapsed(&ms, begin, end), "elapsed recall time");
                if (rep)
                    times[k][rep - 1] += ms;
            }
            launch(compare, groups, 128, a.ptr, b.ptr, length, errors.ptr);
        }
    synchronize();
    s.d.event_free(begin);
    s.d.event_free(end);
    Word wrong = 0;
    s.d.check(s.d.download(&wrong, errors.ptr, 4), "check directions, bearings, location, distance and cell");
    if (wrong)
        throw std::logic_error("spatial rank index disagrees with writable circuit");
    for (auto &t : times)
        std::ranges::sort(t);
    std::cout << "hollow_deploy=" << count << "  queries=" << calls << "  verified=" << Count(calls) * 6
              << "  differences=" << wrong << "  extra_bytes=" << sizeof(hollow::Index)
              << "  writable_ms=" << times[0][2] << "  indexed_ms=" << times[1][2]
              << "  Mdecisions_per_s=" << calls / (1000. * times[1][2]) << '\n';
}

void hollow_color(Word seed) {
    Function color{};
    s.bind(color, "hollow_color", Image::floodplain);
    launch(color, (n + 127) / 128, 128, world.hollows, seed, n);
}
