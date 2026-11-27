void viewpoint_study(const std::vector<braid::Genes> &parents, const Options &o) {
    Word size = parents.size(), cases = o.braid_perspectives, seed = o.braid_seed, reps = o.braid_examples,
         panels = o.braid_views, budget = o.braid_view_budget;
    Temporary stock(s.d, Count(size) * sizeof(parents[0])), device(s.d, Count(cases) * sizeof(braid::ViewCase));
    s.d.check(s.d.upload(stock.ptr, parents.data(), Count(size) * sizeof(parents[0])),
              "sample visual and acoustic genotypes");
    Function trial{};
    s.bind(trial, "braid_parallax", Image::runnels);
    const char *labels[]{"calibrated",    "literal", "same_panel", "unpaired",    "wrong_voice", "outgoing_only",
                         "incoming_only", "muted",   "cyclic",     "single_port", "turned",      "readapted"};
    std::unique_ptr<EvidenceFile> file;
    if (!o.braid_view_trace.empty()) {
        file = std::make_unique<EvidenceFile>(o.braid_view_trace);
        file->out << "arm\tcase\tworld\tvoice\ttwin\tnoun\tprior_"
                     "reply\trig\twrist\tmotor\tstrokes\tstart\tend\tsockets\tcolors\tvisits\tlaw\tarbor\tobservations"
                     "\tanchor\tsur"
                     "vivors\topening\trequest\traw_reply\tanswer\treciprocal\tchosen\tcover0\tcover1\tdirected_"
                     "open\tdirected_request\tdirected_sound\tdirected_answer\tdirected_"
                     "reciprocal\tthroat\tpending\teligible\n";
    }
    auto answer = [](Word board, Word ref) {
        return ref < 4 ? 4 + braid::pigment(board, ref) : Word(std::countr_zero(braid::lamps(board, ref - 4, 3)));
    };
    std::vector<braid::ViewCase> rows(cases);
    Event a{}, b{};
    s.d.check(s.d.event_new(&a, 0), "start perspective clock");
    s.d.check(s.d.event_new(&b, 0), "finish perspective clock");
    float elapsed = 0;
    for (Word arm = 0; arm < braid::view_tests; ++arm) {
        s.d.check(s.d.record(a, stream), "start situated perspective exchanges");
        launch(trial, (cases + 31) / 32, 32, stock.ptr, size, cases, seed, reps, panels, budget, arm, device.ptr);
        s.d.check(s.d.record(b, stream), "end situated perspective exchanges");
        synchronize();
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "time viewpoint inference");
        elapsed += ms;
        s.d.check(s.d.download(rows.data(), device.ptr, Count(cases) * sizeof(rows[0])),
                  "receive independent optical reports");
        Count correct = 0, strict = 0, memory = 0, spoken = 0, returned = 0, answered = 0, paired = 0, conflicts = 0,
              chosen = 0, exact = 0, available = 0, heard = 0, pre = 0, states = 0;
        Count directed = 0, left_fixed = 0, right_fixed = 0, unique_opportunity = 0, picked_only = 0, equal = 0,
              equal_win = 0, routed = 0, switched = 0;
        for (Word i = 0; i < cases; ++i) {
            auto &r = rows[i];
            bool right[2][2][8]{};
            for (Word j = 0; j < 2; ++j) {
                auto lens = r.glass[j];
                std::ostringstream tape;
                for (Word k = 0; k < r.visits; ++k) {
                    auto v = r.panels[j][k];
                    available += std::popcount(v.asked);
                    heard += std::popcount(v.heard);
                    if (file) {
                        if (k)
                            tape << ',';
                        tape << std::hex
                             << (Count(v.own) | (Count(v.other) << 8) | (Count(v.asked) << 16) |
                                 (Count(v.heard) << 20) | (Count(v.replies) << 24) | (Count(v.first) << 32) |
                                 (Count(v.order) << 40));
                    }
                }
                states += r.survivors[j];
                chosen += r.survivors[j] == 1;
                pre += r.before_turn[j] == 1;
                if (r.survivors[j] == 1) {
                    auto estimate = braid::candidate_view({1, 0, r.anchors[j]}, std::countr_zero(r.hypotheses[j]));
                    exact += estimate.places == lens.places && estimate.hues == lens.hues;
                }
                for (Word twin = 0; twin < 2; ++twin)
                    for (Word noun = 0; noun < 8; ++noun) {
                        auto &v = r.turns[j][twin][noun];
                        Word opening = answer(r.board, noun), ref = r.mode ? opening : noun;
                        Word goal = answer(r.ends[twin], ref), request = braid::change_reference(lens, ref, false);
                        Word reply = braid::change_reference(lens, goal, false);
                        right[j][twin][noun] =
                            v.opening == opening && v.request == request && v.sound == reply && v.answer == goal;
                        correct += v.answer == goal;
                        spoken += v.request != braid::none;
                        returned += v.sound != braid::none;
                        answered += v.answer != braid::none;
                        strict += right[j][twin][noun];
                        memory += right[j][twin][noun] && v.remembered == ref;
                        if (file) {
                            auto &f = file->out;
                            f << labels[arm];
                            for (Word x : std::initializer_list<Word>{
                                     i, r.key, j, twin, noun, r.mode, r.rig, r.wrist, r.motor, r.length, r.board,
                                     r.ends[twin], lens.places, lens.hues, r.visits, r.learning, r.arbor})
                                f << '\t' << x;
                            f << '\t' << tape.str();
                            for (Word x : std::initializer_list<Word>{r.anchors[j], r.hypotheses[j], v.opening,
                                                                      v.request, v.sound, v.answer, v.remembered})
                                f << '\t' << x;
                            auto single = r.public_turn[twin][noun];
                            for (Word x : std::initializer_list<Word>{
                                     r.selected[twin][noun], r.coverage[twin][noun][0], r.coverage[twin][noun][1],
                                     single.opening, single.request, single.sound, single.answer, single.remembered})
                                f << '\t' << x;
                            f << '\t' << r.throat << '\t' << Word(r.pending[twin][noun]) << '\t'
                              << Word(r.eligible[twin][noun]);
                            f << '\n';
                        }
                    }
            }
            Word useful[2]{}, taken[2]{};
            for (Word twin = 0; twin < 2; ++twin)
                for (Word noun = 0; noun < 8; ++noun) {
                    Word ref = r.mode ? answer(r.board, noun) : noun, goal = answer(r.ends[twin], ref);
                    bool different = braid::change_reference(r.glass[0], ref, false) !=
                                         braid::change_reference(r.glass[1], ref, false) ||
                                     braid::change_reference(r.glass[0], goal, false) !=
                                         braid::change_reference(r.glass[1], goal, false);
                    Word pick = r.selected[twin][noun];
                    const auto &chosen_reply = r.public_turn[twin][noun], &fixed = r.turns[pick][twin][noun];
                    if (std::memcmp(&chosen_reply, &fixed, sizeof(fixed)) != 0)
                        throw std::logic_error("an unconsulted witness affected the single public call");
                    bool solved = right[pick][twin][noun], single = right[0][twin][noun] != right[1][twin][noun];
                    bool matched = r.survivors[0] == r.survivors[1] && r.survivors[0] > 1;
                    left_fixed += right[0][twin][noun];
                    right_fixed += right[1][twin][noun];
                    directed += solved;
                    unique_opportunity += single;
                    picked_only += single && solved;
                    equal += single && matched;
                    equal_win += single && solved && matched;
                    if (single) {
                        Word winning = right[1][twin][noun];
                        useful[twin] |= 1u << winning;
                        if (solved)
                            taken[twin] |= 1u << winning;
                    }
                    conflicts += different;
                    paired += different && right[0][twin][noun] && right[1][twin][noun];
                }
            for (Word twin = 0; twin < 2; ++twin) {
                routed += useful[twin] == 3;
                switched += taken[twin] == 3;
            }
        }
        std::cout << "braid_audiences=" << labels[arm] << "  opportunities=" << Count(cases) * 16
                  << "  directed=" << directed << "  fixed0=" << left_fixed << "  fixed1=" << right_fixed
                  << "  sole=" << unique_opportunity << "  found=" << picked_only << "  equal=" << equal
                  << "  equal_found=" << equal_win << "  both_needed=" << routed << "  consulted_both=" << switched
                  << '\n';
        std::cout << "braid_parallax=" << labels[arm] << "  communities=" << cases << "  offered=" << Count(cases) * 32
                  << "  answered=" << answered << "  correct=" << correct << "  grounded=" << strict
                  << "  retained=" << memory << "  conflicts=" << conflicts << "  both=" << paired
                  << "  asked=" << spoken << "  voiced=" << returned << "  panels=" << panels << "  budget=" << budget
                  << "  calibration_calls=" << available << "  heard=" << heard << "  contacts=" << 2 * cases
                  << "  before_unique=" << pre << "  unique=" << chosen << "  true_frames=" << exact
                  << "  alternatives=" << states << '\n';
    }
    s.d.event_free(a);
    s.d.event_free(b);
    if (file)
        file->publish();
    std::cout << "braid_view_ms=" << elapsed << "  conversations=" << Count(cases) * 32 * braid::view_tests
              << "  partner_bytes=" << sizeof(braid::Correspondence) << '\n';
}
