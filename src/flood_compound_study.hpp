#include "flood_signal_voice.hpp"
void arrange_compound_fixtures(capillary::Signalling c, Temporary &rigs) {
    using namespace capillary;
    Word n = c.tests + 1;
    Temporary book(s.d, Count(n) * 2 * sizeof(Junction));
    DevicePtr left = book.ptr, right = book.ptr + Count(n) * sizeof(Junction);
    Function choose{}, couple{};
    s.bind(choose, "signal_junctions", Image::runnels);
    s.bind(couple, "compound_beds", Image::runnels);
    Word second = mix(c.seed ^ 0xe5a93971u);
    launch(choose, (n + 127) / 128, 128, c.seed, c.holdout, n, left);
    launch(choose, (n + 127) / 128, 128, second, c.holdout, n, right);
    launch(couple, (n + 127) / 128, 128, left, right, n, c.inert, rigs.ptr);
    synchronize();
}
void compound_cohort_study(const Habitat &h, const Options &o, const std::vector<Word> &slots, DevicePtr bodies,
                           const storage::AgentImage *tutor) {
    using namespace capillary;
    Word n = slots.size();
    auto c = o.signal;
    c.seed = o.class_seed;
    auto cc = o.compound;
    Temporary fixtures(s.d, Count(c.tests + 1) * sizeof(CompoundBed)), grades(s.d, Count(n) * sizeof(CompoundGrade));
    arrange_compound_fixtures(c, fixtures);
    CompoundMind teacher =
        tutor ? compound_body(phenotype(*tutor), cc.arbor, tutor->compound_contacts) : CompoundMind{};
    Temporary notes(s.d, c.retained && !h.compound_residents.empty() ? Count(n) * 4 : 0);
    if (notes.ptr) {
        std::unordered_map<Word, Word> memory;
        for (auto &a : h.compound_residents)
            memory.emplace(a.slot, a.signs);
        std::vector<Word> prior(n);
        for (Word i = 0; i < n; ++i)
            if (auto it = memory.find(slots[i]); it != memory.end())
                prior[i] = it->second;
        s.d.check(s.d.upload(notes.ptr, prior.data(), Count(n) * 4), "enroll previous two-port observations");
    }
    Function run{};
    s.bind(run, "compound_pairs", Image::runnels);
    FieldTimer time(s.d);
    time.start(stream);
    launch(run, n, 32, bodies, notes.ptr, n, c, cc, teacher, fixtures.ptr, grades.ptr);
    time.stop(stream);
    synchronize();
    std::vector<CompoundGrade> data(n);
    s.d.check(s.d.download(data.data(), grades.ptr, Count(n) * sizeof(data[0])),
              "read four-way food and communication outcomes");
    std::unique_ptr<EvidenceFile> out;
    constexpr const char *keys[] = {"before",         "after",
                                    "erase_first",    "erase_second",
                                    "erase_both",     "initial_all",
                                    "all_four",       "cue0",
                                    "cue1",           "cue2",
                                    "cue3",           "fluent",
                                    "joint_codes",    "separable_codes",
                                    "initial_fluent", "training_food",
                                    "training_heat",  "writer_edits",
                                    "reader_edits",   "trained0",
                                    "trained1",       "trained2",
                                    "trained3",       "first_crop",
                                    "second_crop"};
    static_assert(std::size(keys) == compound_metrics);
    if (!o.cap_screen.empty()) {
        out = std::make_unique<EvidenceFile>(o.cap_screen);
        out->out
            << "writer\treader\twriter_born\treader_born\twriter_fields\treader_fields\thistories\ttests\tomit\ttutor";
        for (auto name : keys)
            out->out << '\t' << name;
        out->out << '\n';
    }
    std::array<Count, compound_metrics> totals{};
    Word best = 0, has = 0;
    Word shift = 1 + mix(c.seed + 9137) % (n - 1);
    auto partner = [&](Word j) { return c.tutor ? slots[j] : slots[(j + shift) % n]; };
    for (Word j = 0; j < n; ++j) {
        auto &v = data[j];
        has += v.value[fluent] != 0;
        auto rank = [](const CompoundGrade &r) {
            return std::tuple{r.value[fluent], r.value[hot_all], r.value[hot_four]};
        };
        if (rank(v) > rank(data[best]))
            best = j;
        if (out) {
            Word a = slots[j], b = partner(j);
            out->out << (c.tutor == 1 ? tutor->resident.slot : a);
            for (Word t : {c.tutor == 2 ? tutor->resident.slot : b,
                           c.tutor == 1 ? tutor->resident.animal.born : h.bodies[a].born,
                           c.tutor == 2 ? tutor->resident.animal.born : h.bodies[b].born, v.writer, v.reader,
                           c.replicas, c.tests, cc.observed, c.tutor})
                out->out << '\t' << t;
            for (Word t : v.value)
                out->out << '\t' << t;
            out->out << '\n';
        }
        for (Word k = 0; k < compound_metrics; ++k)
            totals[k] += v.value[k];
    }
    if (out)
        out->publish();
    Count cases = Count(n) * c.replicas * c.tests * 4;
    if (totals[both_removed] * 4 != cases || totals[one_removed] * 2 > cases || totals[two_removed] * 2 > cases ||
        totals[axiswise] + totals[mixed] != totals[fluent])
        throw std::logic_error("inconsistent four-feeder control");
    std::cout << "compound_population=" << tick << "  pairs=" << n << "  histories=" << c.replicas
              << "  tests=" << c.tests << "  lessons=" << c.lessons << "  cases=" << cases << "  omit=" << cc.observed
              << "  arbor=" << cc.arbor << "  diet=" << cc.diet << "  wean=" << cc.wean << "  feedback=" << c.feedback
              << "  medium=" << c.inert << "  tutor=" << c.tutor << "  contact_bytes=4  capable_pairs=" << has
              << "  writer=" << (c.tutor == 1 ? tutor->resident.slot : slots[best])
              << "  reader=" << (c.tutor == 2 ? tutor->resident.slot : partner(best))
              << "  cuda_ms=" << time.milliseconds();
    for (Word k = 0; k < compound_metrics; ++k)
        std::cout << "  " << keys[k] << '=' << totals[k];
    std::cout << '\n';
}

void repeat_compound(const Options &o, capillary::Signalling c, capillary::CompoundCourse cc, DevicePtr brains,
                     DevicePtr beds, bool empty) {
    using namespace capillary;
    Word n = o.compound_repeats;
    Temporary grades(s.d, Count(n) * sizeof(CompoundLife));
    Function fn{};
    s.bind(fn, "compound_lifetimes", Image::runnels);
    FieldTimer time(s.d);
    time.start(stream);
    launch(fn, (n + 63) / 64, 64, brains, c, cc, beds, n, grades.ptr);
    time.stop(stream);
    synchronize();
    std::vector<CompoundLife> rows(n);
    s.d.check(s.d.download(rows.data(), grades.ptr, Count(n) * sizeof(rows[0])),
              "read independent lifetimes of the same compound foragers");
    std::unique_ptr<EvidenceFile> f;
    if (!o.compound_profiles.empty()) {
        f = std::make_unique<EvidenceFile>(o.compound_profiles);
        f->out << "history\tbefore_code\tcode\twriter_contacts\treader_contacts\tbefore\tafter\tall_"
                  "four\tfluent\tseparable\tcue0\tcue1\tcue2\tcue3\terase_first\terase_second\terase_"
                  "both\ttrained0\ttrained1\ttrained2\ttrained3\tfirst_crop\tsecond_crop\n";
    }
    Count sums[compound_metrics]{};
    for (auto &r : rows) {
        auto &g = r.grade.value;
        if (g[both_removed] != c.tests ||
            g[trained_zero] + g[trained_one] + g[trained_two] + g[trained_three] != c.lessons ||
            (cc.observed < 4 && g[trained_zero + cc.observed]))
            throw std::logic_error("the withheld destination was exposed or the isolated receiver was cued");
        for (Word j = 0; j < compound_metrics; ++j)
            sums[j] += g[j];
        if (f) {
            f->out << r.identity;
            for (Word v :
                 {r.initial_code, r.code,           r.writer,       r.reader,        g[cold_four],    g[hot_four],
                  g[hot_all],     g[fluent],        g[axiswise],    g[hot_zero],     g[hot_one],      g[hot_two],
                  g[hot_three],   g[one_removed],   g[two_removed], g[both_removed], g[trained_zero], g[trained_one],
                  g[trained_two], g[trained_three], g[first_crop],  g[second_crop]})
                f->out << '\t' << v;
            f->out << '\n';
        }
    }
    if (f)
        f->publish();
    std::cout << "compound_lives=" << n << "  tests=" << c.tests << "  lessons=" << c.lessons
              << "  cases=" << Count(n) * 4 * c.tests << "  before=" << sums[cold_four]
              << "  correct=" << sums[hot_four] << "  complete=" << sums[fluent] << "  starts_empty=" << empty
              << "  initially_complete=" << sums[prefluent] << "  separable=" << sums[axiswise]
              << "  joint=" << sums[mixed] << "  omit=" << cc.observed << "  arbor=" << cc.arbor << "  wean=" << cc.wean
              << "  diet=" << cc.diet << "  heldout_correct=" << (cc.observed < 4 ? sums[hot_zero + cc.observed] : 0)
              << "  erased=" << sums[both_removed] << "  elapsed_ms=" << time.milliseconds() << '\n';
}
void compound_speed(capillary::Signalling c, DevicePtr brains, DevicePtr beds, Word panels) {
    using namespace capillary;
    Word n = panels * 4;
    Temporary reference(s.d, Count(n) * sizeof(PackedReport)), parallel(s.d, Count(n) * sizeof(PackedReport)),
        counter(s.d, 4);
    Function direct{}, warp{}, check{};
    s.bind(direct, "compound_frozen_scalar", Image::runnels);
    s.bind(warp, "compound_frozen", Image::runnels);
    s.bind(check, "compare_compound_reports", Image::runnels);
    auto test = [&](Function which, DevicePtr receipt) {
        FieldTimer time(s.d);
        time.start(stream);
        launch(which, (n + 127) / 128, 128, brains, c, beds, panels, receipt);
        time.stop(stream);
        synchronize();
        return time.milliseconds();
    };
    test(direct, reference.ptr);
    test(warp, parallel.ptr);
    std::array<float, 7> scalar{}, batched{};
    for (Word r = 0; r < scalar.size(); ++r) {
        if (r & 1) {
            batched[r] = test(warp, parallel.ptr);
            scalar[r] = test(direct, reference.ptr);
        } else {
            scalar[r] = test(direct, reference.ptr);
            batched[r] = test(warp, parallel.ptr);
        }
    }
    s.d.check(s.d.zero(counter.ptr, 0, 4), "clear inference comparison");
    launch(check, (n + 127) / 128, 128, reference.ptr, parallel.ptr, n, counter.ptr);
    synchronize();
    Word bad = 0;
    s.d.check(s.d.download(&bad, counter.ptr, 4), "count unequal register-shuffled decisions");
    if (bad)
        throw std::logic_error("the cooperative frozen readout disagrees with independent physical trials");
    int regs = 0, local = 0;
    s.d.check(s.d.func_attr(&regs, 4, warp), "measure cooperative readout registers");
    s.d.check(s.d.func_attr(&local, 3, warp), "measure cooperative local memory");
    std::sort(scalar.begin(), scalar.end());
    std::sort(batched.begin(), batched.end());
    std::cout << "compound_benchmark=" << n << "  panels=" << panels << "  stored_fixtures=" << c.tests
              << "  repeats=" << scalar.size() << "  scalar_ms=" << scalar[3] << "  cooperative_ms=" << batched[3]
              << "  speedup=" << scalar[3] / batched[3] << "  differing=" << bad
              << "  receipt_bytes=" << sizeof(PackedReport) << "  registers=" << regs << "  local_bytes=" << local
              << "  choices_per_s=" << 1e3 * n / batched[3] << '\n';
}
void compound_individual_study(const Options &o) {
    using namespace capillary;
    auto c = o.signal;
    c.seed = o.class_seed;
    auto cc = o.compound;
    storage::AgentImage a, b;
    a.read(o.signal_adult);
    b.read(o.signal_youth);
    auto identity = [](const storage::AgentImage &p) {
        return std::tuple{p.resident.slot, p.resident.animal.born, p.resident.germ.grip};
    };
    std::set<decltype(identity(a))> identities;
    auto connect = [&](const storage::AgentImage &p) {
        if (!identities.insert(identity(p)).second)
            throw std::invalid_argument("choose distinct residents for the compound exchange");
        auto body = phenotype(p);
        if (!body.eyes || !body.limbs || !body.mouth)
            throw std::invalid_argument("a compound forager needs eyes, limbs and feeding pores");
        return compound_body(body, cc.arbor, p.compound_contacts);
    };
    CompoundMind actors[]{connect(a), connect(b)};
    std::vector<CompoundMind> adults;
    for (auto &path : o.signal_neighbours) {
        storage::AgentImage p;
        p.read(path);
        adults.push_back(connect(p));
    }
    Word n = 4 * c.tests, others = adults.size();
    Temporary physics(s.d, Count(c.tests + 1) * sizeof(CompoundBed)), brains(s.d, sizeof(actors)),
        neighbours(s.d, Count(others) * sizeof(CompoundMind)), grades(s.d, sizeof(CompoundGrade)),
        receipts(s.d, Count(n) * sizeof(PackedReport));
    arrange_compound_fixtures(c, physics);
    s.d.check(s.d.upload(brains.ptr, actors, sizeof(actors)), "connect both multiport animals");
    if (others)
        s.d.check(s.d.upload(neighbours.ptr, adults.data(), Count(others) * sizeof(CompoundMind)),
                  "connect multiple adult multiport tutors");
    if (o.compound_repeats)
        repeat_compound(o, c, cc, brains.ptr, physics.ptr, !actors[0].tissue && !actors[1].tissue);
    Function train{}, test{};
    s.bind(train, "compound_individual", Image::runnels);
    s.bind(test, "compound_frozen", Image::runnels);
    FieldTimer time(s.d);
    time.start(stream);
    launch(train, 1, 1, brains.ptr, c, cc, physics.ptr, neighbours.ptr, others, grades.ptr);
    launch(test, (n + 63) / 64, 64, brains.ptr, c, physics.ptr, c.tests, receipts.ptr);
    time.stop(stream);
    synchronize();
    std::vector<PackedReport> results(n);
    std::vector<CompoundBed> beds(c.tests + 1);
    CompoundGrade paid{};
    s.d.check(s.d.download(results.data(), receipts.ptr, Count(n) * sizeof(results[0])),
              "read the actual two-port signals");
    s.d.check(s.d.download(beds.data(), physics.ptr, Count(beds.size()) * sizeof(beds[0])),
              "read both physical junctions");
    s.d.check(s.d.download(actors, brains.ptr, sizeof(actors)), "retain compound contacts and somatic state");
    s.d.check(s.d.download(&paid, grades.ptr, sizeof(paid)), "read compound acquisition costs");
    std::unique_ptr<EvidenceFile> out;
    if (!o.signal_trace.empty()) {
        out = std::make_unique<EvidenceFile>(o.signal_trace);
        out->out << "trial\tcue\tleft_row\tleft_rock\tleft_bound\tleft_carrier\tright_row\tright_rock\tright_"
                    "bound\tright_carrier\tmark\tsensed\tchoice\twithout_first\twithout_second\terased\tfood\theat\n";
    }
    Word exact[4]{}, codes[4][4]{}, no_first = 0, no_second = 0, blank = 0, all = 0, partial = 0;
    for (Word i = 0; i < n; ++i) {
        auto v = unpack_report(i & 3, results[i]);
        auto &w = beds[i / 4 + 1];
        ++codes[v.cue][v.mark];
        exact[v.cue] += v.food;
        partial += v.food;
        if ((i & 3) == 3) {
            all += partial == 4;
            partial = 0;
        }
        no_first += v.without_first == v.cue;
        no_second += v.without_second == v.cue;
        blank += v.blank == v.cue;
        if (out) {
            out->out << i / 4 << '\t' << v.cue;
            for (auto j : w.cell)
                for (Word k : {j.row, j.base, j.coating, j.carrier})
                    out->out << '\t' << k;
            for (Word k : {v.mark, v.sensed, v.choice, v.without_first, v.without_second, v.blank, v.food, v.cost})
                out->out << '\t' << k;
            out->out << '\n';
        }
    }
    if (out)
        out->publish();
    if (blank * 4 != n || no_first * 2 > n || no_second * 2 > n)
        throw std::logic_error("the compound signal has an unintended channel");
    if (o.compound_bench)
        compound_speed(c, brains.ptr, physics.ptr, o.compound_bench);
    if (o.signal_voice_exposures || o.signal_dialogues)
        vocal_signal_study(a, b, actors, o, c, brains.ptr, physics.ptr);
    if (!o.signal_save_adult.empty()) {
        auto store = [](storage::AgentImage &p, const CompoundMind &m, const std::string &where) {
            p.compound_signals = p.material_signals = p.hydraulic = true;
            p.compound_contacts = m.tissue;
            p.brain.tissue[utterance::band_of(p)].spike = m.body.cells.spike;
            storage::write_checked(where, p);
        };
        store(a, actors[0], o.signal_save_adult);
        store(b, actors[1], o.signal_save_youth);
    }
    Word sum = 0;
    for (Word cue = 0; cue < 4; ++cue) {
        sum += exact[cue];
        std::cout << "compound_cue=" << cue << "  offered=" << (cc.observed != cue)
                  << "  training_exposures=" << paid.value[trained_zero + cue] << "  exact=" << exact[cue]
                  << "  tests=" << c.tests;
        for (Word k = 0; k < 4; ++k)
            std::cout << "  mark" << k << '=' << codes[cue][k];
        std::cout << '\n';
    }
    std::cout << "compound_exchange=" << a.resident.slot << "  receiver=" << b.resident.slot << "  cases=" << n
              << "  correct=" << sum << "  all_four=" << all << "  erase_first=" << no_first
              << "  erase_second=" << no_second << "  erase_both=" << blank << "  omit=" << cc.observed
              << "  arbor=" << cc.arbor << "  writer_fields=" << actors[0].fields
              << "  reader_fields=" << actors[1].fields << "  writer_taste=" << (actors[0].body.palate & 15)
              << "  reader_taste=" << (actors[1].body.palate & 15) << "  diet=" << cc.diet << "  wean=" << cc.wean
              << "  writer_contacts=" << actors[0].tissue << "  reader_contacts=" << actors[1].tissue
              << "  lessons=" << c.lessons << "  history=" << c.first << "  test_seed=" << c.holdout
              << "  training_food=" << paid.value[rearing_food] << "  training_heat=" << paid.value[rearing_cost]
              << "  writer_edits=" << paid.value[source_edits] << "  reader_edits=" << paid.value[listener_edits]
              << "  contact_bytes=4  cuda_ms=" << time.milliseconds() << '\n';
}
