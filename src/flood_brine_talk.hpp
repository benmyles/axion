void consult_water(Word held, Word limit, Word closure = 0) {
    Function ask{}, watch{};
    s.bind(ask, "brine_question", Image::floodplain);
    s.bind(watch, "brine_witness", Image::floodplain);
    auto &brain = subject.brine_mind.cortex;
    Word attempted = 0, heard = 0, halves = 0;
    limit = std::min(limit, (brain.genes.readout >> 17) & 15u);
    for (Word i = 0; i < limit; ++i) {
        Word reserve = subject.resident.animal.energy,
             peer_reserve = other.riverine ? other.resident.animal.energy : 64;
        if (reserve < 2 || peer_reserve < 3)
            break;
        void *question[]{
            &samples.ptr, &saline_cortex.ptr, &eye,     &held,        &consult_round,       &hearing,
            &reserve,     &rehearsal,         &closure, &channel.ptr, &recipient_board.ptr, &request_note.ptr};
        s.d.check(s.d.launch(ask, 1, 1, 1, 1, 1, 1, 0, nullptr, question, nullptr),
                  "voice a requested material experiment");
        void *respond[]{&listener_eye, &channel.ptr,  &recipient_board.ptr,
                        &hearing,      &peer_reserve, &recipient_reply.ptr};
        s.d.check(s.d.launch(receive_call, 1, 1, 1, 1, 1, 1, 0, nullptr, respond, nullptr),
                  "the partner chooses and pays for the spoken experiment");
        void *observe[]{&saline_cortex.ptr, &recipient_board.ptr, &recipient_reply.ptr,
                        &water_vision,      &request_note.ptr,    &samples.ptr};
        s.d.check(s.d.launch(watch, 1, 1, 1, 1, 1, 1, 0, nullptr, observe, nullptr),
                  "witness the physical result on the partner's bench");
        material::SampleRequest req{};
        material::ListenerResult peer{};
        s.d.check(s.d.download(&req, request_note.ptr, sizeof(req)), "hear the requested reference cup");
        s.d.check(s.d.download(&peer, recipient_reply.ptr, sizeof(peer)), "observe the public demonstration");
        if (req.stop)
            break;
        if (req.wave)
            --subject.resident.animal.energy;
        if (other.riverine)
            other.resident.animal.energy =
                static_cast<unsigned short>(std::min(65535u, peer_reserve - peer.paid + peer.calories));
        ++attempted;
        heard += peer.paid != 0;
        halves += req.seen < 16;
        std::cout << "brine_request=" << consult_round << "  speech=" << acoustic::spell(req.wave)
                  << "  reference_coating=" << req.chip << "  proposed_cup=" << req.suggested
                  << "  soil0=" << req.raw[0] << "  soil1=" << req.raw[1] << "  actual_site=" << peer.chosen
                  << "  paid=" << peer.paid << "  food=" << peer.calories << "  witnessed_pair=" << req.seen
                  << "  visible_pulse=" << req.outcome << std::endl;
        ++consult_round;
    }
    s.d.check(s.d.download(&brain, saline_cortex.ptr, sizeof(brain)),
              "retain the investigator's acquired contact strengths");
    std::cout << "brine_consult=" << attempted << "  heard=" << heard << "  observations=" << halves
              << "  target=" << held << "  vision=" << water_vision
              << "  speaker_reserve=" << subject.resident.animal.energy << std::endl;
}

void material_dialogues(Word tables, Word species, Word limit, Word seed, bool intention) {
    material::ConversationTest test{};
    const char *nouns[]{"hare", "seal", "lark", "toad"};
    for (Word i = 0; i < 4; ++i)
        test.held[i] = acoustic::pulse(nouns[i]);
    for (Word k = 0; k < 2; ++k) {
        auto &f = test.form[k];
        f.length = 4;
        f.wave[0] = acoustic::pulse("which");
        f.wave[1] = acoustic::pulse("soil");
        f.wave[2] = acoustic::pulse(k ? "resists" : "feeds");
    }
    if (intention)
        for (Word k = 0; k < 2; ++k) {
            auto &f = test.form[k];
            f.length = 5;
            f.wave[0] = acoustic::pulse("where");
            f.wave[1] = acoustic::pulse("would");
            f.wave[2] = acoustic::pulse("you");
            f.wave[3] = acoustic::pulse(k ? "avoid" : "try");
        }
    test.intention = intention;
    test.trials = tables;
    test.seed = seed;
    test.species = species;
    test.limit = limit;
    test.speaker_energy = subject.resident.animal.energy;
    test.peer_energy = other.riverine ? other.resident.animal.energy : 64;
    Temporary results(s.d, Count(tables) * sizeof(material::ConversationCounts));
    std::vector<material::ConversationCounts> copies(tables);
    Function protocol{};
    s.bind(protocol, "brine_conversations", Image::floodplain);
    const char *conditions[]{"autonomous",   "consulted",   "reference_quiet", "false_feedback",
                             "answer_quiet", "answer_foil", "no_prospection",  "prior_confidence"};
    for (Word mode = 0; mode < std::size(conditions); ++mode) {
        auto start = std::chrono::steady_clock::now();
        void *args[]{&saline_cortex.ptr, &lived.ptr, &law.ptr, &eye,        &listener_eye,
                     &vocabulary.ptr,    &test,      &mode,    &results.ptr};
        s.d.check(s.d.launch(protocol, (tables + 63) / 64, 1, 1, 64, 1, 1, 0, nullptr, args, nullptr),
                  "conduct counterfactual conversations on unfamiliar material");
        s.d.check(s.d.download(copies.data(), results.ptr, Count(tables) * sizeof(copies[0])),
                  "read independent listener actions");
        std::array<Count, sizeof(material::ConversationCounts) / sizeof(Word)> total{};
        for (auto &r : copies) {
            auto q = std::bit_cast<std::array<Word, total.size()>>(r);
            for (size_t j = 0; j < q.size(); ++j)
                total[j] += q[j];
        }
        if (total[1] != total[3] + total[4] + total[5] || total[14] || (mode == 2 && total[13]) ||
            (mode == 4 && total[6]))
            throw std::logic_error("invalid solvent consultation or exposed test coating");
        double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "brine_dialogue=" << conditions[mode] << "  tables=" << tables << "  offered=" << total[0]
                  << "  questions=" << total[1] << "  before=" << total[2] << "  correct=" << total[3]
                  << "  wrong=" << total[4] << "  silent=" << total[5] << "  acted=" << total[6]
                  << "  intended_site=" << total[7] << "  food_requests=" << total[8] << "  harvested=" << total[9]
                  << "  inactive_harvests=" << total[10] << "  spoken_requests=" << total[11]
                  << "  tests_shown=" << total[12] << "  witnessed=" << total[13] << "  target_sampled=" << total[14]
                  << "  word_changes=" << total[15] << "  paired_correct=" << total[16] << "  peer_work=" << total[17]
                  << "  reference_food=" << total[18] << "  solutes=" << species << "  policy_agreement=" << total[19]
                  << "  report=" << (intention ? "choice" : "fact") << "  limit=" << limit << "  seed=" << seed
                  << "  seconds=" << secs << std::endl;
    }
}
