void context_study(const Habitat &h, const storage::AgentImage &initial, capillary::Body body, capillary::Course c,
                   const Options &o) {
    using namespace capillary;
    auto series = o.context_series;
    Word count = series.streams * series.rounds;
    auto boards = capillary_works(h, o, c, count);
    DevicePtr natural = boards ? boards->ptr : 0;
    std::vector<Work> layouts(boards && !o.context_trace.empty() ? count : 0);
    Temporary brain(s.d, sizeof(Saved)), book(s.d, sizeof(Gloss)), records(s.d, Count(count) * sizeof(StreamRow)),
        grades(s.d, Count(count) * sizeof(StreamGrade));
    auto language_book = language(o.cap_dialect);
    s.d.check(s.d.upload(brain.ptr, &initial.channels, sizeof(Saved)),
              "connect the continuing resident's material and speech tissue");
    s.d.check(s.d.upload(book.ptr, &language_book, sizeof(language_book)), "connect the acoustic examiner");
    Function train{}, grade{};
    s.bind(train, "context_currents", Image::runnels);
    s.bind(grade, "context_grade", Image::runnels);
    FieldTimer tuition(s.d);
    tuition.start(stream);
    launch(train, (series.streams + 63) / 64, 64, brain.ptr, body, c, series, book.ptr, natural, records.ptr);
    tuition.stop(stream);
    FieldTimer physical(s.d);
    physical.start(stream);
    launch(grade, (count + 127) / 128, 128, c, series.varied, book.ptr, natural, records.ptr, count, grades.ptr);
    physical.stop(stream);
    synchronize();
    if (!layouts.empty())
        s.d.check(s.d.download(layouts.data(), boards->ptr, Count(count) * sizeof(Work)),
                  "retain actual field geometry for each continuing life");
    std::vector<StreamRow> visits(count);
    std::vector<StreamGrade> outcomes(count);
    s.d.check(s.d.download(visits.data(), records.ptr, Count(count) * sizeof(visits[0])),
              "read paid actions and changing forecasts");
    s.d.check(s.d.download(outcomes.data(), grades.ptr, Count(count) * sizeof(outcomes[0])),
              "weigh the independently released channels");
    std::unique_ptr<EvidenceFile> trace;
    if (!o.context_trace.empty()) {
        trace = std::make_unique<EvidenceFile>(o.context_trace);
        trace->out << "subject\tepisode\tkey\trock\tswitch\tsenses\tprior_on\tprior_off\tobserved_on\tobserved_"
                      "off\tmodel_on\tmodel_"
                      "off\ton\toff\tpassive\treceipts\tlaw\tviews\tprobes\theat\tedits\tspikes\trechecks\tchallenges\t"
                      "first_review\tfingerprint\tdesign\tready\tcaught\tsolvable\tbefore_correct\twatch_"
                      "correct\tcorrect\texpected\tspoken\twick\twater\tsill\tfilm\tfeed\tcatcher\tspill\ttools\tsupply"
                      "\tdepth\tmoves\tdendrites\tmotor\tmode";
        if (series.varied)
            trace->out << "\tquestion\trow\tchange\tpigment\tpad\tprior\tafter\tnamed\trequested\ttrue_cause\tpeg_"
                          "materials\tdialect";
        trace->out << '\n';
    }
    Count cases[2]{}, before[2]{}, observed[2]{}, after[2]{}, said[2]{}, explains[2]{}, collected[2]{}, accessible[2]{},
        heat = 0, attempts = 0, checks = 0, writes = 0;
    struct Phase {
        Count n = 0, exact = 0, questions = 0, words = 0, empty = 0, unsupported = 0, meals = 0, possible = 0;
    };
    Phase phases[2]{};
    Count asked[kinds]{}, answerable[kinds]{}, right[kinds]{}, unsolicited[kinds]{};
    Count signature = 1469598103934665603ull;
    for (Word i = 0; i < count; ++i) {
        auto &r = visits[i];
        auto &g = outcomes[i];
        Word which = r.changed;
        if (series.varied) {
            Word kind = (g.scene >> 18) & 7;
            ++asked[kind];
            answerable[kind] += g.available;
            right[kind] += g.said;
            unsolicited[kind] += !g.available && r.spoken;
        }
        auto &phase = phases[i % series.rounds >= series.at];
        ++phase.n;
        phase.exact += g.exact;
        phase.questions += g.available;
        phase.words += g.said;
        phase.empty += !g.available;
        phase.unsupported += !g.available && r.spoken;
        phase.meals += g.caught;
        phase.possible += g.possible;
        ++cases[which];
        before[which] += g.old_exact;
        observed[which] += g.watched_exact;
        after[which] += g.exact;
        said[which] += g.said;
        explains[which] += g.available;
        collected[which] += g.caught;
        accessible[which] += g.possible;
        heat += r.heat;
        attempts += r.probes;
        checks += r.rechecks;
        writes += r.anchors;
        signature = (signature ^ r.fingerprint) * 1099511628211ull;
        if (r.heat > c.budget || !valid(r.memory))
            throw std::logic_error("invalid context contact or expenditure");
        if (trace) {
            auto w = boards ? layouts[i] : channel(mix(r.key + 9791), c.depth, c.scarce);
            Count handles = 0;
            for (Word j = 0; j < w.n; ++j)
                handles |= Count(w.handle[j].site | (w.handle[j].material << 6)) << (8 * j);
            auto &f = trace->out;
            f << i / series.rounds;
            for (Count v : std::initializer_list<Count>{i % series.rounds,
                                                        r.key,
                                                        r.rock,
                                                        r.changed,
                                                        r.senses,
                                                        r.before.on,
                                                        r.before.off,
                                                        r.observed.on,
                                                        r.observed.off,
                                                        r.after.on,
                                                        r.after.off,
                                                        r.tissue.on,
                                                        r.tissue.off,
                                                        footprint(r.memory),
                                                        r.memory.recent,
                                                        g.law,
                                                        r.passive_looks,
                                                        r.probes,
                                                        r.heat,
                                                        r.edits,
                                                        r.spikes,
                                                        r.rechecks,
                                                        r.anchors,
                                                        r.earliest,
                                                        r.fingerprint,
                                                        r.design,
                                                        r.ready,
                                                        g.caught,
                                                        g.possible,
                                                        g.old_exact,
                                                        g.watched_exact,
                                                        g.exact,
                                                        g.goal,
                                                        r.spoken,
                                                        w.bed.wick,
                                                        w.bed.wet,
                                                        w.bed.sill,
                                                        w.bed.paint,
                                                        w.feed,
                                                        w.catcher,
                                                        w.waste,
                                                        handles,
                                                        w.stock,
                                                        w.time,
                                                        c.moves,
                                                        body.dendrites,
                                                        body.motor,
                                                        series.mode})
                f << '\t' << v;
            if (series.varied) {
                Word z = g.scene;
                for (Word v : {(z >> 18) & 7, z & 63, (z >> 6) & 63, (z >> 12) & 3, (z >> 14) & 3, (z >> 16) & 1,
                               (z >> 17) & 1, (z >> 21) & 3, (z >> 23) & 7, z >> 26, g.handles, o.cap_dialect})
                    f << '\t' << v;
            }
            f << '\n';
        }
    }
    if (trace)
        trace->publish();
    if (series.second < 32)
        for (Word j = 0; j < 2; ++j) {
            auto &p = phases[j];
            std::cout << "context_phase=" << j << "  rock=" << (j ? series.second : c.rock) << "  episodes=" << p.n
                      << "  exact=" << p.exact << "  answerable=" << p.questions << "  spoken=" << p.words
                      << "  indeterminate=" << p.empty << "  unsupported=" << p.unsupported << "  food=" << p.meals
                      << "  accessible=" << p.possible << '\n';
        }
    if (series.varied)
        for (Word k = 0; k < kinds; ++k)
            std::cout << "context_question=" << k << "  asked=" << asked[k] << "  answerable=" << answerable[k]
                      << "  exact=" << right[k] << "  unsupported=" << unsolicited[k] << '\n';
    for (Word arm = 0; arm < 2; ++arm)
        std::cout << "context_condition=" << arm << "  n=" << cases[arm] << "  before=" << before[arm]
                  << "  watched=" << observed[arm] << "  after=" << after[arm] << "  explained=" << said[arm]
                  << "  answerable=" << explains[arm] << "  repaired=" << collected[arm]
                  << "  solvable=" << accessible[arm] << '\n';
    std::cout << "context_series=" << initial.resident.slot << "  streams=" << series.streams
              << "  episodes=" << series.rounds << "  total=" << count << "  switches=" << cases[1]
              << "  before_switch=" << before[1] << "  observed_switch=" << observed[1]
              << "  resolved_switch=" << after[1] << "  stable=" << cases[0] << "  stable_correct=" << after[0]
              << "  worded=" << said[0] + said[1] << "  answerable=" << explains[0] + explains[1]
              << "  repaired=" << collected[0] + collected[1] << "  solvable=" << accessible[0] + accessible[1]
              << "  heat=" << heat << "  actions=" << attempts << "  signature=" << signature << "  rechecks=" << checks
              << "  receipts=" << writes
              << "  bytes=" << (series.mode == single_memory || series.mode == examiner_clears ? 0 : sizeof(History))
              << "  arm=" << series.mode << "  basal_rule=" << (body.growth.law & 15) << "  material_rule="
              << ((body.growth.law ^ body.pressure_xor ^ (series.mode == retention_flipped ? 4u : 0u)) & 15)
              << "  module_mask=" << body.module_mask << "  dendrites=" << body.dendrites << "  motor=" << body.motor
              << "  varied_questions=" << series.varied << "  transfer_rock=" << series.second
              << "  transfer_at=" << series.at << "  odds=" << series.change_percent << "  jitter=" << series.jitter
              << "  cuda_ms=" << tuition.milliseconds() << "  physical_ms=" << physical.milliseconds() << '\n';
    Count rocks = 0, changes = 0, early = 0, resolved = 0, found = 0, meals = 0, due = 0;
    for (Word j = 0; j < series.rounds; ++j) {
        auto &row = visits[j];
        auto &grade = outcomes[j];
        Count one = bit(j);
        if (row.rock != c.rock)
            rocks |= one;
        if (row.changed)
            changes |= one;
        if (grade.old_exact)
            early |= one;
        if (grade.exact)
            resolved |= one;
        if (row.anchors)
            found |= one;
        if (grade.caught)
            meals |= one;
        if (row.rechecks)
            due |= one;
    }
    std::cout << "context_life=0  episodes=" << series.rounds << "  rocks=" << rocks << "  changes=" << changes
              << "  before=" << early << "  resolved=" << resolved << "  signed=" << found << "  checks=" << due
              << "  meals=" << meals << "  base=" << c.rock << '\n';
    if (!o.context_save.empty()) {
        auto pupil = initial;
        auto &last = visits[series.rounds - 1];
        pupil.hydraulic = true;
        pupil.channels.mind.tissue = last.tissue;
        pupil.channels.history = last.memory;
        pupil.channels.adaptive = series.mode == single_memory || series.mode == examiner_clears ? 0
                                  : c.regional                                                   ? 3
                                                                                                 : 1;
        pupil.brain.tissue[utterance::band_of(pupil)].spike = last.terminal;
        storage::write_checked(o.context_save, pupil);
    }
}
