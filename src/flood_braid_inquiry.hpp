void braid_inquiries(const std::vector<braid::Genes> &parents, const Options &o) {
    Word count = o.braid_inquiries, seed = o.braid_seed, reps = o.braid_examples, stock = parents.size();
    Word holdout = o.braid_holdout;
    Temporary pool(s.d, Count(stock) * sizeof(parents[0])), device(s.d, Count(count) * sizeof(braid::Interrogation));
    s.d.check(s.d.upload(pool.ptr, parents.data(), Count(stock) * sizeof(parents[0])), "sample event learners");
    Function probe{};
    s.bind(probe, "braid_inquiries", Image::runnels);
    std::vector<braid::Interrogation> rows(count);
    Event a{}, b{};
    s.d.check(s.d.event_new(&a, 0), "start event clock");
    s.d.check(s.d.event_new(&b, 0), "finish event clock");
    s.d.check(s.d.record(a, stream), "start paired episodes");
    launch(probe, (count + 31) / 32, 32, pool.ptr, stock, count, seed, reps, holdout, device.ptr);
    s.d.check(s.d.record(b, stream), "finish paired episodes");
    synchronize();
    float ms = 0;
    s.d.check(s.d.elapsed(&ms, a, b), "measure situated conversation");
    s.d.event_free(a);
    s.d.event_free(b);
    s.d.check(s.d.download(rows.data(), device.ptr, Count(count) * sizeof(rows[0])), "collect intervention histories");
    std::unique_ptr<EvidenceFile> file;
    if (!o.braid_report_trace.empty()) {
        file = std::make_unique<EvidenceFile>(o.braid_report_trace);
        file->out << "arm\tcase\tworld\tcontext\ttwin\trig\twrist\tplan\tstrokes\tstart\tprefix\taim\tend\tmode\topenin"
                     "g\tasked\texplicit\twitness\tanswer\treverse\tretained\tagain\tseen_before\tseen_"
                     "after\tintention\tomission\n";
    }
    const char *labels[]{"witness",      "muted",      "echoed",  "misreported", "no_context",
                         "no_retention", "both_blind", "visible", "untaught"};
    auto optical = [](Word state, Word ref) {
        if (ref < 4)
            return 4 + braid::pigment(state, ref);
        return Word(std::countr_zero(braid::lamps(state, ref - 4, 3)));
    };
    Count names = 0, learned = 0, paid = 0, answers[4]{}, distinguished = 0, counter = 0, intention = 0;
    for (auto &r : rows) {
        names += r.nouns[0] + r.nouns[1];
        learned += r.forms[0] + r.forms[1];
        paid += r.training;
        for (Word ctx = 0; ctx < 2 * holdout; ++ctx) {
            Word ref = braid::held_subject(ctx);
            const Word scenes[]{r.board, r.ends[0], r.aim, r.prefix};
            for (Word j = 0; j < 4; ++j)
                answers[j] += ((r.frame_answers[0][j] >> (8 * ctx)) & 255) == optical(scenes[j], ref);
            if (optical(r.prefix, ref) != optical(r.aim, ref)) {
                ++distinguished;
                intention += ((r.frame_answers[1][1] >> (8 * ctx)) & 255) == optical(r.ends[1], ref) &&
                             ((r.frame_answers[1][2] >> (8 * ctx)) & 255) == optical(r.aim, ref);
                counter += ((r.frame_answers[0][3] >> (8 * ctx)) & 255) == optical(r.prefix, ref) &&
                           ((r.frame_answers[0][2] >> (8 * ctx)) & 255) == optical(r.aim, ref);
            }
        }
    }
    for (Word arm = 0; arm < braid::report_arms; ++arm) {
        Count attempted = 0, asked = 0, explicitly = 0, answered = 0, exact = 0, reverse = 0, carry = 0, pairs = 0,
              broken = 0, open = 0, voices = 0;
        Count context_switches = 0, context_correct = 0;
        for (Word i = 0; i < count; ++i) {
            auto &r = rows[i];
            Word mode = (r.key >> 16) & 1;
            for (Word ctx = 0; ctx < 2 * holdout; ++ctx) {
                Word original = braid::held_subject(ctx), expected = optical(r.board, original),
                     referent = mode ? expected : original;
                Word goals[]{optical(r.ends[0], referent), optical(r.ends[1], referent)};
                bool pass = true;
                for (Word twin = 0; twin < 2; ++twin) {
                    auto &v = r.trial[arm][ctx][twin];
                    ++attempted;
                    open += v.before == expected;
                    asked += v.asked;
                    explicitly += v.reframed;
                    voices += v.witness != braid::none;
                    answered += v.answer != braid::none;
                    bool right = v.answer == goals[twin];
                    exact += right;
                    reverse += right && v.reverse == referent;
                    carry += right && v.reverse == referent && v.reverse_from_report;
                    pass &= right;
                    if (ctx >= 2) {
                        Word earlier = braid::held_subject(ctx - 2);
                        if (mode)
                            earlier = optical(r.board, earlier);
                        Word answer = optical(r.ends[twin], earlier);
                        context_switches += answer != goals[twin];
                        context_correct +=
                            answer != goals[twin] && right && r.trial[arm][ctx - 2][twin].answer == answer;
                    }
                    if (file) {
                        file->out << labels[arm];
                        for (Word x : std::initializer_list<Word>{i,         r.key,    ctx,       twin,
                                                                  r.rig,     r.wrist,  r.stream,  r.strokes,
                                                                  r.board,   r.prefix, r.aim,     r.ends[twin],
                                                                  mode,      v.before, v.asked,   v.reframed,
                                                                  v.witness, v.answer, v.reverse, v.reverse_from_report,
                                                                  v.repeat})
                            file->out << '\t' << x;
                        for (Word j = 0; j < 4; ++j)
                            file->out << '\t' << ((r.frame_answers[twin][j] >> (8 * ctx)) & 255);
                        file->out << '\n';
                    }
                }
                broken += goals[0] != goals[1];
                pairs += pass && goals[0] != goals[1];
            }
        }
        std::cout << "braid_inquiry=" << labels[arm] << "  cases=" << count << "  offered=" << attempted
                  << "  opening=" << open << "  asked=" << asked << "  explicit=" << explicitly
                  << "  witness=" << voices << "  answered=" << answered << "  correct=" << exact
                  << "  reversed=" << reverse << "  retained=" << carry << "  changed=" << broken << "  both=" << pairs
                  << "  topic_changes=" << context_switches << "  both_topics=" << context_correct << '\n';
    }
    std::cout << "braid_episodes=" << count << "  training_pulls=" << paid << "  noun_contacts=" << names
              << "  resolved_forms=" << learned << "  exposures=" << reps << "  withheld_per_kind=" << holdout
              << "  held_out=" << 2 * holdout * count << "  before=" << answers[0] << "  after=" << answers[1]
              << "  aim=" << answers[2] << "  omitted=" << answers[3] << "  differentiated=" << distinguished
              << "  counterfactual=" << counter << "  distinguished_intent=" << intention << "  cuda_ms=" << ms << '\n';
    if (file)
        file->publish();
}
