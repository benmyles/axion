void story_study(const Habitat &h, const Options &o) {
    if (o.story_residents) {
        story_screen(h, o);
        return;
    }
    storage::AgentImage pupil, mentor;
    pupil.read(o.story_pupil);
    auto body = utterance::senses(pupil, o.story_gate);
    auto peer = body;
    if (!o.story_mentor.empty()) {
        mentor.read(o.story_mentor);
        peer = utterance::senses(mentor);
        if (!mentor.describes)
            throw std::invalid_argument("the selected mentor has no acquired narrative circuit");
    }
    auto teacher = utterance::culture(o.story_language, 23771, body.voice);
    teacher.origin = body;
    if (!o.voice_donor.empty()) {
        storage::AgentImage origin;
        origin.read(o.voice_donor);
        teacher.origin = utterance::senses(origin);
        teacher.throat = teacher.origin.voice;
    }
    if (!o.voice_circuit.empty())
        teacher.throat = utterance::read_voice(o.voice_circuit);
    utterance::Course c{};
    c.seed = o.class_seed;
    c.split = o.story_partition;
    c.singles = o.story_singles;
    c.couples = o.story_pairs;
    c.length = o.story_span;
    c.arm = o.story_arm;
    c.natural_light = o.story_light;
    c.fast = o.story_fast;
    c.decoding = o.story_decoding;
    c.hearing = o.story_hearing;
    c.hand = o.story_hand;
    c.purse = o.story_ration;
    if (o.story_broods) {
        auto child = rear_calls(h, o, body, teacher.origin, c);
        if (!child)
            return;
        teacher.throat = *child;
    }
    if (!o.voice_output.empty()) {
        EvidenceFile f(o.voice_output);
        f.out << "AXION-RESONATORS-1 " << teacher.throat.dendrites << ' ' << teacher.throat.motor << '\n';
        f.publish();
    }
    Temporary tissue(s.d, sizeof(utterance::Memory)),
        source(s.d, o.story_mentor.empty() ? 0 : sizeof(utterance::Memory)), lex(s.d, sizeof(teacher)),
        change(s.d, sizeof(utterance::Teaching)), motor(s.d, utterance::prompts * sizeof(utterance::Chip)),
        dictionary(s.d, (c.decoding || c.fast == 2 || o.story_benchmark)
                            ? utterance::prompts * 256 * sizeof(utterance::Packet)
                            : 0);
    s.d.check(s.d.upload(tissue.ptr, &pupil.accounts.memory, sizeof(utterance::Memory)),
              "restore the pupil's grounded speech projections");
    s.d.check(s.d.upload(lex.ptr, &teacher, sizeof(teacher)), "place the external acoustic culture");
    if (source.ptr)
        s.d.check(s.d.upload(source.ptr, &mentor.accounts.memory, sizeof(utterance::Memory)),
                  "enroll a separately acquired mentor");
    Function teach{}, compile{}, index{}, perform{}, feed{};
    s.bind(teach, "narrative_school", Image::runnels);
    s.bind(compile, "narrative_compile", Image::runnels);
    s.bind(index, "narrative_dictionary", Image::runnels);
    s.bind(perform, "narrative_express", Image::runnels);
    s.bind(feed, "narrative_feed", Image::runnels);
    FieldTimer tuition(s.d);
    tuition.start(stream);
    launch(teach, 1, 1, tissue.ptr, body, lex.ptr, c, source.ptr, peer, change.ptr);
    auto reference_course = o.references;
    Temporary reference_budget(s.d, sizeof(utterance::ReferenceResult));
    if (o.references.examples) {
        Function rehearse{};
        s.bind(rehearse, "narrative_references", Image::runnels);
        launch(rehearse, 1, 1, tissue.ptr, body, lex.ptr, c, reference_course, change.ptr, reference_budget.ptr,
               source.ptr, peer);
    }
    tuition.stop(stream);
    launch(compile, utterance::prompts, 32, tissue.ptr, motor.ptr);

    synchronize();
    utterance::Teaching learned{};
    s.d.check(s.d.download(&learned, change.ptr, sizeof(learned)), "read the pupil's paid hearings");
    s.d.check(s.d.download(&pupil.accounts.memory, tissue.ptr, sizeof(utterance::Memory)),
              "return acquired motor programs");
    if (!utterance::valid(pupil.accounts.memory) || learned.heat > c.purse)
        throw std::logic_error("invalid learned vocal circuit or teaching budget");
    pupil.brain.tissue[utterance::band_of(pupil)].spike = learned.terminal;
    pupil.describes = true;
    if (!o.story_save.empty())
        storage::write_checked(o.story_save, pupil);
    std::array<utterance::Chip, utterance::prompts> chips{};
    s.d.check(s.d.download(chips.data(), motor.ptr, sizeof(chips)), "read compiled phase selectors");
    Word residual_cells = 0;
    for (auto &v : chips) {
        residual_cells += v.duration == utterance::fallback;
        for (auto &shape : v.motor)
            residual_cells += std::ranges::count(shape, utterance::fallback);
    }
    Count image_bytes = sizeof(chips) + residual_cells * sizeof(utterance::Socket);
    Temporary residual(s.d, image_bytes - sizeof(chips));
    if (residual_cells) {
        Function pack{};
        s.bind(pack, "narrative_residuals", Image::runnels);
        launch(pack, 1, 1, tissue.ptr, motor.ptr, residual.ptr);
    }
    if (dictionary.ptr)
        launch(index, utterance::prompts * 4, 64, residual.ptr, motor.ptr, dictionary.ptr);
    Word used = 0, packed = 0, units = 0, prompts = 0;
    for (Word k = 0; k < utterance::prompts; ++k) {
        auto &row = pupil.accounts.memory.script[k];
        if (!row.prompt)
            continue;
        ++prompts;
        ++used;
        packed += chips[k].duration != utterance::fallback;
        for (Word j = 0; j < utterance::shapes; ++j) {
            units += row.units[j].length != 0;
            for (Word i = 0; i < row.units[j].length; ++i) {
                ++used;
                packed += chips[k].motor[j][i] != utterance::fallback;
            }
        }
    }
    std::cout << "narrative_school=" << pupil.resident.slot << "  born=" << pupil.resident.animal.born
              << "  generation=" << pupil.resident.animal.generation << "  singles=" << learned.singles
              << "  pairs=" << learned.couples << "  incomplete=" << learned.unseen << "  admitted=" << learned.paired
              << "  sounds=" << learned.spoken << "  edits=" << learned.edits << "  regrown=" << learned.revisions
              << "  spike_changes=" << learned.spikes << "  seed=" << c.seed << "  gate=" << o.story_gate
              << "  inherited_law=" << body.learning.law << "  arbor=" << body.learning.arbor
              << "  routed=" << body.learning.program.first << "  threshold=" << body.learning.program.second
              << "  heat=" << learned.heat << "  reserve=" << c.purse - learned.heat << "  mentor=" << bool(source.ptr)
              << "  prompts=" << prompts << "  constructions=" << units << "  packed_cells=" << packed
              << "  cells=" << used << "  tissue_bytes=" << sizeof(utterance::Memory) << "  image_bytes=" << image_bytes
              << "  residual_cells=" << residual_cells << "  cuda_ms=" << tuition.milliseconds() << '\n';
    if (o.story_benchmark) {
        const char *symbols[]{"narrative_stream_plastic", "narrative_stream_packed", "narrative_stream_table"};
        const char *engines[]{"plastic", "packed", "table"};
        std::array<Function, 3> emit{};
        std::array<std::vector<float>, 3> durations{};
        Word amount = o.story_benchmark;
        Temporary digest(s.d, Count(amount) * sizeof(Count));
        for (Word j = 0; j < 3; ++j)
            s.bind(emit[j], symbols[j], Image::runnels);
        std::vector<Count> original(amount), check(amount);
        for (Word lap = 0; lap < 10; ++lap)
            for (Word order = 0; order < 3; ++order) {
                Word mode = (order + lap) % 3;
                FieldTimer event(s.d);
                event.start(stream);
                launch(emit[mode], (amount + 127) / 128, 128, tissue.ptr, motor.ptr, residual.ptr, dictionary.ptr,
                       c.seed, amount, c.length, digest.ptr);
                event.stop(stream);
                synchronize();
                if (lap)
                    durations[mode].push_back(event.milliseconds());
                else {
                    auto &output = mode ? check : original;
                    s.d.check(s.d.download(output.data(), digest.ptr, Count(amount) * sizeof(Count)),
                              "verify all benchmark reports");
                    if (mode && original != check)
                        throw std::logic_error("compiled vocal output differs from its plastic synapses");
                }
            }
        for (Word mode = 0; mode < 3; ++mode) {
            auto &v = durations[mode];
            std::ranges::sort(v);
            std::cout << "narrative_speed=" << engines[mode] << "  histories=" << amount << "  length=" << c.length
                      << "  trials=" << v.size() << "  median_ms=" << v[v.size() / 2] << "  fastest_ms=" << v.front()
                      << "  slowest_ms=" << v.back() << "  image_bytes="
                      << (!mode ? sizeof(utterance::Memory)
                                : (mode == 2 ? sizeof(chips) + sizeof(utterance::Packet) * 256 * utterance::prompts
                                             : image_bytes))
                      << "  matched=" << amount << '\n';
        }
    }
    retina::Course water{};
    sensory_surfaces(water, o);
    c.beds = water.surfaces;
    c.sites = water.sites;
    c.seed = mix(o.class_seed ^ 0x53e48cd1u);
    if (o.references.examples || o.references.trials) {
        utterance::ReferenceResult exposure{};
        if (o.references.examples)
            s.d.check(s.d.download(&exposure, reference_budget.ptr, sizeof(exposure)), "account for spoken questions");
        Word n = o.references.trials * 4;
        std::vector<utterance::ReferenceExam> shots(n);
        float latency = 0;
        if (n) {
            Function ask{};
            s.bind(ask, "narrative_reference_exam", Image::runnels);
            Temporary responses(s.d, Count(n) * sizeof(shots[0]));
            FieldTimer timer(s.d);
            timer.start(stream);
            launch(ask, (n + 63) / 64, 64, tissue.ptr, body, lex.ptr, c, reference_course, n, responses.ptr);
            timer.stop(stream);
            synchronize();
            latency = timer.milliseconds();
            s.d.check(s.d.download(shots.data(), responses.ptr, Count(n) * sizeof(shots[0])),
                      "read answers to unheard questions");
        }
        Count possible = 0, exact = 0, located = 0, avoidable = 0, wrong = 0, missing = 0, false_replies = 0,
              distinct = 0, both = 0;
        std::unique_ptr<EvidenceFile> evidence;
        if (!o.reference_trace.empty()) {
            evidence = std::make_unique<EvidenceFile>(o.reference_trace);
            evidence->out << "layout\tfamily\tnoun\theard_"
                             "noun\tphysical\tretinal\ttarget\tchosen\twanted\tsaid\troutes\trecognized\n";
        }
        for (Word i = 0; i < n; ++i) {
            auto &a = shots[i];
            bool answer = a.wanted.size(), right = a.said.bits == a.wanted.bits;
            possible += answer;
            exact += answer && right;
            located += answer && right && a.target == a.picked;
            avoidable += answer && !a.said.size();
            wrong += answer && a.said.size() && !right;
            missing += !answer;
            false_replies += !answer && a.said.size();
            if ((i & 1) && answer && shots[i - 1].wanted.size() && shots[i - 1].wanted.bits != a.wanted.bits) {
                ++distinct;
                both += right && shots[i - 1].said.bits == shots[i - 1].wanted.bits;
            }
            if (evidence)
                evidence->out << (i >> 2) << '\t' << ((i >> 1) & 1) << '\t' << (2 + (i & 1)) << '\t'
                              << (2 + ((i & 1) ^ o.references.other)) << '\t' << a.physical << '\t' << a.retinal << '\t'
                              << a.target << '\t' << a.picked << '\t' << a.wanted.bits << '\t' << a.said.bits << '\t'
                              << a.routes << '\t' << a.heard << '\n';
        }
        for (Word i = 0; i < n; ++i)
            if (shots[i].wanted.size() && shots[i].said.bits == shots[i].wanted.bits) {
                auto &z = shots[i];
                std::cout << "narrative_reference_scene=" << (i >> 2) << "  family=" << ((i >> 1) & 1)
                          << "  noun=" << (2 + (i & 1)) << "  target=" << z.target << "  heard=" << z.picked
                          << "  physical=" << z.physical << "  answer=" << z.said.bits << '\n';
                break;
            }
        if (evidence)
            evidence->publish();
        std::cout << "narrative_references=" << pupil.resident.slot << "  examples=" << exposure.taught
                  << "  edits=" << exposure.edits << "  spike_changes=" << exposure.spikes << "  heat=" << exposure.paid
                  << "  layouts=" << o.references.trials << "  answerable=" << possible << "  exact=" << exact
                  << "  located=" << located << "  wrong=" << wrong << "  silent=" << avoidable
                  << "  absent=" << missing << "  invented=" << false_replies << "  contrasts=" << distinct
                  << "  both=" << both << "  convention=" << o.references.swapped
                  << "  crossed=" << o.references.mispaired << "  other_noun=" << o.references.other
                  << "  cuda_ms=" << latency << '\n';
    }
    if (o.accent_generations)
        test_accents(o, teacher.throat, body, teacher.origin, c);
    if (o.story_language == utterance::native)
        call_test(teacher.throat, body, teacher.origin, c, o.story_trials, o.call_practice, Word(c.hearing == 1));
    Word tasks = o.story_language == utterance::native ? 1 : 4, count = 2 * tasks * o.story_trials;
    Temporary ears(s.d, Count(count) * sizeof(utterance::Exam)), food(s.d, Count(count) * sizeof(utterance::Meal)),
        log(s.d, o.story_trace.empty() ? 0 : Count(count) * sizeof(utterance::WordSample));
    FieldTimer clock(s.d);
    clock.start(stream);
    launch(perform, (count + 63) / 64, 64, tissue.ptr, body, lex.ptr, c, motor.ptr, residual.ptr, dictionary.ptr, count,
           ears.ptr, log.ptr);
    launch(feed, (count + 63) / 64, 64, ears.ptr, count, c, body, food.ptr);
    clock.stop(stream);
    synchronize();
    std::vector<utterance::Exam> report(count);
    std::vector<utterance::Meal> meals(count);
    s.d.check(s.d.download(report.data(), ears.ptr, report.size() * sizeof(report[0])),
              "read independently spoken descriptions");
    s.d.check(s.d.download(meals.data(), food.ptr, meals.size() * sizeof(meals[0])),
              "weigh the catch after the choice");
    std::unique_ptr<EvidenceFile> evidence;
    std::vector<utterance::WordSample> phrases(log.ptr ? count : 0);
    if (log.ptr) {
        s.d.check(s.d.download(phrases.data(), log.ptr, phrases.size() * sizeof(phrases[0])),
                  "hear exact words from both mouths");
        evidence = std::make_unique<EvidenceFile>(o.story_trace);
        evidence->out << "seed\tpartition\tlayout\ttwin\ttask\tlanguage\tlength\tsites\tlight\thearing\thand\tplaces\th"
                         "ues\treceptors\t"
                         "physical\tretinal\tdecoded\tvisible\tstained\tsaid\trequired\tcorrect\trecovered\tupstream\td"
                         "ownstream\tsettled\n";
    }
    std::array<Count, 4> right{}, voiced{}, both{}, different{};
    Count exact = 0, decoded = 0, total = 0, moved = 0, witnessed = 0, spoken = 0, upper = 0, lower = 0, settled = 0,
          word_count = 0;
    for (Word i = 0; i < count; ++i) {
        auto &r = report[i];
        auto &q = meals[i];
        if (utterance::Trail{r.physical}.size() != c.length)
            throw std::logic_error("insufficient held-out physical passages");
        right[r.task] += r.correct;
        exact += r.correct;
        voiced[r.task] += r.words != 0;
        spoken += r.words != 0;
        word_count += r.words;
        if (!r.task) {
            decoded += r.recovered;
            ++total;
            witnessed += r.visible == c.length;
            moved += r.stained;
        }
        if ((i & 1) && report[i - 1].expected != r.expected) {
            ++different[r.task];
            both[r.task] += r.correct && report[i - 1].correct;
        }
        upper += q.upper;
        lower += q.lower;
        settled += q.settled;
        if (evidence) {
            auto &out = evidence->out;
            out << c.seed << '\t' << c.split << '\t' << (i / 2 / tasks) << '\t' << (i & 1) << '\t' << r.task << '\t'
                << o.story_language << '\t' << c.length << '\t' << c.sites << '\t' << c.natural_light << '\t'
                << c.hearing << '\t' << c.hand << '\t' << Word(body.glass.places) << '\t' << Word(body.glass.hues)
                << '\t' << body.retina.receptors << '\t' << r.physical << '\t' << r.retinal << '\t' << r.heard << '\t'
                << r.visible << '\t' << r.stained;
            for (auto line : {&phrases[i].spoken, &phrases[i].expected}) {
                out << '\t';
                for (Word j = 0; j < line->n; ++j) {
                    if (j)
                        out << ',';
                    out << Word(line->word[j]);
                }
            }
            out << '\t' << r.correct << '\t' << r.recovered << '\t' << q.upper << '\t' << q.lower << '\t' << q.settled
                << '\n';
        }
    }
    if (count)
        std::cout << "narrative_scene=0  physical=" << report[0].physical << "  retinal=" << report[0].retinal
                  << "  words=" << report[0].words << "  needed=" << report[0].required
                  << "  exact=" << report[0].correct << '\n';
    if (evidence)
        evidence->publish();
    if (upper + lower + settled != 2 * total * c.length)
        throw std::logic_error("a captured portion disappeared");
    for (Word j = 0; j < tasks; ++j)
        std::cout << "narrative_question=" << j << "  cases=" << 2 * o.story_trials << "  voiced=" << voiced[j]
                  << "  exact=" << right[j] << "  ordered_pairs=" << different[j] << "  both_orders=" << both[j]
                  << '\n';
    std::cout << "narrative_exam=" << pupil.resident.slot << "  seed=" << c.seed << "  language=" << o.story_language
              << "  exposure=" << c.arm << "  length=" << c.length << "  cases=" << count << "  histories=" << total
              << "  observable=" << witnessed << "  changes_of_pigment=" << moved << "  utterances=" << spoken
              << "  words=" << word_count << "  exact=" << exact << "  complete_descriptions=" << right[0]
              << "  recovered_histories=" << decoded << "  upstream=" << upper << "  downstream=" << lower
              << "  settled=" << settled << "  portions=" << 2 * total * c.length << "  heldout=1  surfaces=" << c.sites
              << "  hearing=" << c.hearing << "  packed=" << c.fast << "  cuda_ms=" << clock.milliseconds() << '\n';
}
