// Grammar, physical forecasts, vocal choice and the receiver's action live on
// the GPU. The host only transcribes words and controls the experimental room.
bool field_control(const std::string &line) {
    if (field_open && (line == "/field-memory lived" || line == "/field-memory blank")) {
        auto copy = subject.inquiry;
        if (line.ends_with("blank"))
            copy.chemistry = 0;
        s.d.check(s.d.upload(lived.ptr, &copy, sizeof(copy)), "select lived or blank material evidence");
        std::cout << "field_memory=" << (line.ends_with("blank") ? "blank" : "lived") << std::endl;
        return true;
    }
    if (field_open && (line == "/listen-field open" || line == "/listen-field quiet" || line == "/listen-field foil")) {
        hearing = line.ends_with("quiet") ? 0u : line.ends_with("foil") ? 2u : 1u;
        std::cout << "field_hearer=" << hearing << std::endl;
        return true;
    }
    std::istringstream input(line);
    std::string token;
    input >> token;
    Word command = 0, value = 0, point = 0;
    if (field_open && saline_table && token == "/brine-dialogues") {
        std::string t, ions, requests, k, kind = "fact", extra;
        if (!(input >> t >> ions >> requests >> k) ||
            ((input >> kind) && (input >> extra || (kind != "fact" && kind != "choice"))))
            throw std::invalid_argument("use /brine-dialogues TABLES SOLUTES LIMIT SEED [fact|choice]");
        material_dialogues(quantity(t, 1, 65536), quantity(ions, 2, 5), quantity(requests, 0, 12), quantity(k, 0, ~0u),
                           kind == "choice");
        return true;
    }
    if (field_open && saline_table && token == "/brine-memory") {
        std::string mode, extra;
        if (!(input >> mode) || input >> extra || (mode != "blank" && mode != "lived"))
            throw std::invalid_argument("use /brine-memory blank|lived");
        subject.brine_mind.cortex.memory = mode == "blank" ? brine::Trace{} : entry_trace;
        s.d.check(s.d.upload(saline_cortex.ptr, &subject.brine_mind.cortex, sizeof(brine::Animal)),
                  "prepare the requested comparison");
        std::cout << "brine_memory=" << mode << std::endl;
        return true;
    }
    if (field_open && saline_table && token == "/water-replay") {
        std::string mode, extra;
        if (!(input >> mode) || input >> extra || (mode != "on" && mode != "off"))
            throw std::invalid_argument("use /water-replay on|off");
        rehearsal = mode == "on";
        std::cout << "water_replay=" << mode << std::endl;
        return true;
    }
    if (field_open && saline_table && token == "/water-evidence") {
        std::string mode, extra;
        if (!(input >> mode) || input >> extra || (mode != "visible" && mode != "dark" && mode != "inverted"))
            throw std::invalid_argument("use /water-evidence visible|dark|inverted");
        water_vision = mode == "visible" ? 1 : mode == "inverted" ? 2 : 0;
        std::cout << "water_evidence=" << mode << std::endl;
        return true;
    }
    if (field_open && saline_table && token == "/consult-water") {
        std::string which, budget, extra;
        if (!(input >> which >> budget) || input >> extra)
            throw std::invalid_argument("use /consult-water CHIP|auto LIMIT");
        Word count = quantity(budget, 0, 12);
        if (which == "auto") {
            automatic = count;
            std::cout << "brine_auto=" << count << std::endl;
        } else
            consult_water(quantity(which, 0, 1), count);
        return true;
    }
    if (token == "/assay-table" || token == "/brine-table") {
        std::string seed, extra;
        if (!(input >> seed))
            throw std::invalid_argument("supply a physical table seed");
        saline_table = token == "/brine-table";
        if (saline_table) {
            std::string water, ions;
            if (!subject.saline || !(input >> water >> ions) || input >> extra)
                throw std::invalid_argument("use /brine-table SCENE WATER SOLUTES with a brine resident");
            Word k = quantity(water, 0, ~0u), species = quantity(ions, 2, 5);
            liquid = brine::phase(brine::dissolve(mix(k + 517), mix(k + 299), species), 32);
            consult_round = 0;
        } else {
            liquid = 0;
            if (input >> extra)
                throw std::invalid_argument("use /assay-table SEED");
        }
        value = quantity(seed, 0, ~0u);
        if (!field_open) {
            s.bind(inspect_samples, "flood_field_utterance", Image::floodplain);
            s.bind(receive_call, "flood_field_recipient", Image::floodplain);
            s.d.check(s.d.upload(lived.ptr, &subject.inquiry, sizeof(subject.inquiry)),
                      "seat the field-reared investigator");
            s.d.check(s.d.upload(law.ptr, &subject.worker.genes, sizeof(subject.worker.genes)),
                      "restore the field sensory code");
            s.d.check(s.d.upload(vocabulary.ptr, &subject.field_speech, sizeof(subject.field_speech)),
                      "restore acquired questions");
        }
        field_open = true;
        open = false;
        shore_open = false;
        shore_requests = false;
        spoken_work = false;
    } else {
        if (!field_open)
            return false;
        if (token == "/touch-coupons" || (saline_table && token == "/try-grain")) {
            std::string sample, extra;
            if (!(input >> sample) || input >> extra)
                throw std::invalid_argument("use /touch-coupons SAMPLE");
            value = quantity(sample, 0, 1);
            command = token == "/try-grain" ? 4 : 1;
        } else if (token == "/show-question" || (saline_table && token == "/show-choice")) {
            std::string sample, deposit;
            if (!(input >> sample >> deposit))
                throw std::invalid_argument("use /show-question SAMPLE DEPOSIT QUESTION");
            value = quantity(sample, 0, 1);
            command = token == "/show-choice" ? 5 : 2;
            point = command == 5 && deposit == "taken"   ? 2
                    : command == 5 && deposit == "other" ? 3
                                                         : quantity(deposit, 0, 1);
        } else if (line.front() == '/')
            return false;
        else {
            command = 3;
            input.clear();
            input.str(line);
        }
    }
    material::FieldUtterance phrase{};
    if (command == 2 || command == 3 || command == 5) {
        while (input >> token) {
            while (!token.empty() &&
                   (token.back() == '?' || token.back() == '.' || token.back() == '!' || token.back() == ','))
                token.pop_back();
            if (token.empty())
                continue;
            for (auto &c : token)
                if (c >= 'A' && c <= 'Z')
                    c = char(c - 'A' + 'a');
            if (phrase.length == 8 || token.size() > 12 ||
                !std::all_of(token.begin(), token.end(), [](char c) { return c >= 'a' && c <= 'z'; })) {
                if (command == 3)
                    return false;
                throw std::invalid_argument(
                    "a field question has up to eight spoken words of at most twelve letters each");
            }
            phrase.wave[phrase.length++] = acoustic::pulse(token);
        }
        if (!phrase.length)
            return false;
    }
    Word reserve = subject.resident.animal.energy;
    DevicePtr brain = saline_table ? saline_cortex.ptr : 0;
    void *args[]{&lived.ptr,
                 &law.ptr,
                 &eye,
                 &vocabulary.ptr,
                 &samples.ptr,
                 &phrase,
                 &command,
                 &value,
                 &point,
                 &reserve,
                 &field_reply.ptr,
                 &channel.ptr,
                 &recipient_board.ptr,
                 &brain,
                 &liquid,
                 &water_vision};
    s.d.check(s.d.launch(inspect_samples, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "interpret a question in a material scene");
    material::FieldAnswer r{};
    material::Samples room{};
    s.d.check(s.d.download(&r, field_reply.ptr, sizeof(r)), "listen to the field investigator");
    if (command == 3 && saline_table && automatic && r.recognized && r.uncertain && r.referent < 2) {
        consult_water(r.referent, automatic, 1);
        reserve = subject.resident.animal.energy;
        s.d.check(s.d.launch(inspect_samples, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                  "answer after the requested demonstrations");
        s.d.check(s.d.download(&r, field_reply.ptr, sizeof(r)), "hear the revised answer");
    }
    if ((command == 1 || command == 4) && saline_table)
        s.d.check(s.d.download(&subject.brine_mind.cortex, brain, sizeof(brine::Animal)),
                  "retain paired physical samples");
    if (command == 3 && !r.recognized)
        return false;
    s.d.check(s.d.download(&room, samples.ptr, sizeof(room)), "observe visible samples and deposits");
    if (command == 2 || command == 5)
        s.d.check(s.d.download(&subject.field_speech, vocabulary.ptr, sizeof(subject.field_speech)),
                  "retain a question learned from pointing");
    if (command == 1 || command == 4)
        subject.resident.animal.energy = static_cast<unsigned short>(std::min(65535u, reserve - r.paid + r.calories));
    if (command == 0) {
        std::cout << "field_table=" << value;
        for (Word j = 0; j < 4; ++j)
            std::cout << "  " << (j < 2 ? "chip" : "bed") << (j & 1) << "=" << acoustic::spell(r.names[j])
                      << "  pigment" << j << "=" << room.pigment[j] << "  site" << j << "=" << room.site[j];
        if (saline_table)
            std::cout << "  brine=1  predictive_cells=" << std::popcount(subject.brine_mind.cortex.genes.active);
        std::cout << std::endl;
    } else if (command == 1) {
        std::cout << "field_coupon=" << value << "  observed=" << bool(room.measured & (1u << value))
                  << "  result=" << r.evidence << "  spent=" << r.paid << "  food=" << r.calories << std::endl;
    } else if (command == 4) {
        std::cout << "brine_motion=" << value << "  chosen=" << r.destination << "  paid=" << r.paid
                  << "  food=" << r.calories << std::endl;
    } else if (command == 2 || command == 5) {
        std::cout << "field_question=lesson  accepted=" << r.recognized << "  frames=" << r.questions
                  << "  exposures=" << subject.field_speech.examples << std::endl;
    } else {
        if (auto *j = journal::current())
            j->line("field_query=heard  text=" + line);
        std::cout << "animal> " << (r.wave ? acoustic::spell(r.wave) : "?") << std::endl;
        if (hearing == 2)
            s.d.check(s.d.upload(channel.ptr, &r.foil, sizeof(r.foil)),
                      "substitute the audible name of the other deposit");
        Word peer_reserve = other.riverine ? other.resident.animal.energy : 64;
        void *reply_args[]{&listener_eye, &channel.ptr,  &recipient_board.ptr,
                           &hearing,      &peer_reserve, &recipient_reply.ptr};
        s.d.check(s.d.launch(receive_call, 1, 1, 1, 1, 1, 1, 0, nullptr, reply_args, nullptr),
                  "the recipient interprets only the acoustic signal");
        material::ListenerResult peer{};
        s.d.check(s.d.download(&peer, recipient_reply.ptr, sizeof(peer)), "read the recipient's physical intervention");
        if (other.riverine)
            other.resident.animal.energy =
                static_cast<unsigned short>(std::min(65535u, peer_reserve - peer.paid + peer.calories));
        std::cout << "field_answer=" << acoustic::spell(r.wave) << "  chip=" << r.referent
                  << "  predicted=" << r.expected << "  referred_bed=" << r.destination << "  hypotheses=" << r.eligible
                  << "  receiver_site=" << peer.chosen << "  receiver_work=" << peer.paid
                  << "  receiver_food=" << peer.calories << std::endl;
    }
    if (auto *j = journal::current()) {
        ++j->metrics["field_conversation_events"];
        j->metrics["field_question_examples"] = subject.field_speech.examples;
        if (command == 3) {
            ++j->metrics["field_questions_heard"];
            j->metrics["field_material_answers"] += r.wave != 0;
        }
    }
    return true;
}
