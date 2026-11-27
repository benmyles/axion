void vocal_signal_study(storage::AgentImage &writer, storage::AgentImage &reader, capillary::CompoundMind (&bodies)[2],
                        const Options &o, capillary::Signalling c, DevicePtr brains, DevicePtr beds) {
    using namespace capillary;
    SignalVoice voices[2]{writer.signal_voice, reader.signal_voice};
    Count ears[2]{signal_ear(writer), signal_ear(reader)};
    VoiceSchool languages[2]{voice_environment(o.signal_dialects[0]), voice_environment(o.signal_dialects[1])};
    Temporary tissue(s.d, sizeof(voices)), hearing(s.d, sizeof(ears)), schools(s.d, sizeof(languages)),
        bill(s.d, 2 * sizeof(VoiceTuition));
    s.d.check(s.d.upload(tissue.ptr, voices, sizeof(voices)), "connect learned acoustic signalling contacts");
    s.d.check(s.d.upload(hearing.ptr, ears, sizeof(ears)), "connect the real ears of both residents");
    s.d.check(s.d.upload(schools.ptr, languages, sizeof(languages)),
              "set the two public resonators and movement words");
    if (o.signal_voice_exposures) {
        Function learn{};
        s.bind(learn, "learn_signal_voices", Image::runnels);
        auto count = o.signal_voice_exposures, purse = o.cap_purse;
        launch(learn, 1, 2, brains, tissue.ptr, hearing.ptr, schools.ptr, beds, count, purse, c.seed, bill.ptr);
        synchronize();
        VoiceTuition paid[2]{};
        s.d.check(s.d.download(paid, bill.ptr, sizeof(paid)), "count physical sound exposures and expenditure");
        s.d.check(s.d.download(voices, tissue.ptr, sizeof(voices)), "retain separately learned auditory associations");
        s.d.check(s.d.download(bodies, brains, sizeof(bodies)), "retain both vocal pupils' somatic state");
        for (Word j = 0; j < 2; ++j) {
            auto &animal = j ? reader : writer;
            animal.voices_signs = animal.compound_signals = animal.material_signals = animal.hydraulic = true;
            animal.signal_voice = voices[j];
            if (!capillary::valid(voices[j]) || paid[j].heat > purse)
                throw std::logic_error("invalid acquisition of elementary waves");
            std::cout << "signal_vocabulary=" << animal.resident.slot << "  role=" << j
                      << "  dialect=" << o.signal_dialects[j] << "  single_pressure=" << paid[j].singles[0]
                      << "  single_movements=" << paid[j].singles[1] << "  heard=" << paid[j].heard
                      << "  pressure_strokes=" << paid[j].pressure_strokes << "  limb_strokes=" << paid[j].limb_strokes
                      << "  heat=" << paid[j].heat << "  synaptic_edits=" << paid[j].edits
                      << "  spikes=" << paid[j].spikes << "  combined_examples=0  sensory_links=" << voices[j].links[0]
                      << "  motor_links=" << voices[j].links[1] << "  voice_bytes=" << sizeof(SignalVoice) << '\n';
        }
    }
    if (!o.signal_dialogues)
        return;
    Word n = o.signal_dialogues, amount = n * 4, arm = o.signal_audio, questions = o.signal_questions,
         reply_mode = o.signal_reply;
    Temporary ledger(s.d, Count(amount) * sizeof(AudibleVisit));
    Function speak{};
    s.bind(speak, "private_signal_dialogue", Image::runnels);
    FieldTimer time(s.d);
    time.start(stream);
    launch(speak, (amount + 63) / 64, 64, brains, tissue.ptr, hearing.ptr, schools.ptr, beds, c, n, arm, questions,
           reply_mode, ledger.ptr);
    time.stop(stream);
    synchronize();
    std::vector<AudibleVisit> results(amount);
    s.d.check(s.d.download(results.data(), ledger.ptr, Count(amount) * sizeof(AudibleVisit)),
              "record what the animals actually produced and heard");
    std::unique_ptr<EvidenceFile> file;
    if (!o.signal_words.empty()) {
        file = std::make_unique<EvidenceFile>(o.signal_words);
        file->out << "scene\tcue\tmark\tvisible\tcall_a\tcall_b\theard\tchoice\tanswer_a\tanswer_"
                     "b\tstatement\tfood\tformed\tinitial\tremaining\talternatives\trequests\tanswers\task_a\treply_"
                     "a\task_b\treply_b\troutes\n";
    }
    Word spoken = 0, responded = 0, heard = 0, food = 0, reported = 0, goal = 0, false_words = 0, asked = 0,
         answered = 0, recovered = 0, unasked = 0;
    for (Word i = 0; i < amount; ++i) {
        auto &v = results[i];
        asked += v.requests;
        answered += v.answers;
        recovered += v.requests && v.food;
        unasked += !v.requests && v.food;
        spoken += v.call[0] != 0;
        responded += v.choice < 4;
        heard += v.heard == v.visible;
        food += v.food;
        reported += v.statement < 4 && v.statement == v.choice;
        goal += v.statement == v.cue;
        false_words += v.statement < 4 && v.statement != v.choice;
        if (file) {
            file->out << i / 4;
            for (Count k : std::initializer_list<Count>{
                     v.cue,          v.mark,      v.visible,      v.call[0],   v.call[1], v.heard,
                     v.choice,       v.answer[0], v.answer[1],    v.statement, v.food,    v.formed,
                     v.initial,      v.remaining, v.alternatives, v.requests,  v.answers, v.query[0],
                     v.clarified[0], v.query[1],  v.clarified[1], v.routes})
                file->out << '\t' << k;
            file->out << '\n';
        }
    }
    if (file)
        file->publish();
    std::cout << "auditory_feeders=" << writer.resident.slot << "  reader=" << reader.resident.slot
              << "  cases=" << amount << "  called=" << spoken << "  interpreted=" << responded
              << "  both_senses=" << heard << "  found=" << food << "  reported_choice=" << reported
              << "  named_food=" << goal << "  misdescribed_choice=" << false_words << "  requests=" << asked
              << "  answers=" << answered << "  food_after_question=" << recovered
              << "  food_without_question=" << unasked << "  question_budget=" << questions
              << "  reply_mode=" << reply_mode << "  control=" << arm << "  writer_dialect=" << o.signal_dialects[0]
              << "  reader_dialect=" << o.signal_dialects[1] << "  cuda_ms=" << time.milliseconds() << '\n';
}
