void capillary_speech_study(storage::AgentImage &pupil, capillary::Body body, capillary::Course c, const Options &o) {
    using namespace capillary;
    auto words = language(o.cap_dialect);
    Temporary brain(s.d, sizeof(Saved)), book(s.d, sizeof(Gloss)), learning(s.d, sizeof(SpeechRecord));
    s.d.check(s.d.upload(brain.ptr, &pupil.channels, sizeof(Saved)), "place the private causal and acoustic cortex");
    s.d.check(s.d.upload(book.ptr, &words, sizeof(words)), "connect the external dialect");
    if (o.cap_vocab) {
        Word amount = o.cap_vocab, purse = o.cap_purse;
        Function teach{};
        s.bind(teach, "capillary_speech", Image::runnels);
        launch(teach, 1, 1, brain.ptr, body, c, book.ptr, amount, purse, learning.ptr);
        synchronize();
        SpeechRecord bill{};
        s.d.check(s.d.download(&bill, learning.ptr, sizeof(bill)), "measure paid linguistic demonstrations");
        s.d.check(s.d.download(&pupil.channels, brain.ptr, sizeof(Saved)), "retain grounded causal expressions");
        pupil.brain.tissue[utterance::band_of(pupil)].spike = bill.terminal;
        body.cells.spike = bill.terminal;
        if (!capillary::valid(pupil.channels.speech) || bill.heat > purse)
            throw std::logic_error("inconsistent acquisition of causal speech");
        std::cout << "capillary_vocabulary=" << pupil.resident.slot << "  labels=" << bill.pointed
                  << "  examples=" << bill.examples << "  edits=" << bill.edits << "  spikes=" << bill.spikes
                  << "  heat=" << bill.heat << "  reserve=" << purse - bill.heat << "  dialect=" << o.cap_dialect
                  << '\n';
    }
    if (o.cap_recipes || o.cap_orders) {
        auto demonstration = rack_language(o.cap_dialect);
        Temporary rack(s.d, sizeof(demonstration));
        s.d.check(s.d.upload(rack.ptr, &demonstration, sizeof(demonstration)), "set out the external motor tutor");
        if (o.cap_recipes) {
            Word n = o.cap_recipes, purse = o.cap_purse;
            Function teach{};
            s.bind(teach, "capillary_apprentice", Image::runnels);
            launch(teach, 1, 1, brain.ptr, body, c, rack.ptr, n, purse, learning.ptr);
            synchronize();
            SpeechRecord bill{};
            s.d.check(s.d.download(&bill, learning.ptr, sizeof(bill)), "count paid motor demonstrations");
            s.d.check(s.d.download(&pupil.channels, brain.ptr, sizeof(Saved)), "retain procedural speech");
            pupil.brain.tissue[utterance::band_of(pupil)].spike = bill.terminal;
            body.cells.spike = bill.terminal;
            std::cout << "capillary_manual=" << pupil.resident.slot << "  examples=" << bill.examples
                      << "  heat=" << bill.heat << "  spikes=" << bill.spikes << "  edits=" << bill.edits
                      << "  single=" << pupil.channels.speech.manual.script.views
                      << "  pairs=" << pupil.channels.speech.manual.script.pairs << '\n';
        }
        if (o.cap_orders) {
            storage::AgentImage child;
            child.read(o.cap_peer);
            auto receiver = phenotype(child);
            Word n = o.cap_orders * 2, arm = o.cap_talk_arm;
            Temporary other(s.d, sizeof(Saved)), reports(s.d, Count(n) * sizeof(Instruction));
            s.d.check(s.d.upload(other.ptr, &child.channels, sizeof(Saved)),
                      "connect the listener's own learned commands");
            Function voice{};
            s.bind(voice, "capillary_instructions", Image::runnels);
            FieldTimer time(s.d);
            time.start(stream);
            launch(voice, (n + 63) / 64, 64, brain.ptr, other.ptr, body, receiver, c, rack.ptr, n, arm, reports.ptr);
            time.stop(stream);
            synchronize();
            std::vector<Instruction> report(n);
            s.d.check(s.d.download(report.data(), reports.ptr, Count(n) * sizeof(Instruction)),
                      "grade the listener's constructed channel");
            std::unique_ptr<EvidenceFile> f;
            if (!o.cap_manual_trace.empty()) {
                f = std::make_unique<EvidenceFile>(o.cap_manual_trace);
                f->out << "pair\ttwin\tintended\tdecoded\tacoustic_"
                          "hash\taction\tplanned\tsteps\twords\tfinal\tdenied\tearned\tbefore\tsolvable\tneeds_order\n";
            }
            Count before = 0, after = 0, possible = 0, novel = 0, long_ok = 0, order_cases = 0, order_ok = 0,
                  sounds = 0;
            for (Word i = 0; i < n; ++i) {
                auto &v = report[i];
                before += v.before;
                after += v.earned;
                possible += v.solvable;
                sounds += v.words;
                novel += v.steps > 2;
                long_ok += v.steps > 2 && v.earned && v.decoded == v.intended;
                order_cases += v.needs_order;
                order_ok += v.needs_order && v.earned;
                if (f) {
                    f->out << i / 2 << '\t' << (i & 1);
                    for (Count x : std::initializer_list<Count>{v.intended, v.decoded, v.acoustic, v.action, v.planned,
                                                                v.steps, v.words, v.final, v.denied, v.earned, v.before,
                                                                v.solvable, v.needs_order})
                        f->out << '\t' << x;
                    f->out << '\n';
                }
            }
            if (f)
                f->publish();
            std::cout << "capillary_instruction=" << pupil.resident.slot << "  listener=" << child.resident.slot
                      << "  cases=" << n << "  possible=" << possible << "  before=" << before << "  after=" << after
                      << "  novel=" << novel << "  long_exact=" << long_ok << "  order_matters=" << order_cases
                      << "  ordered_food=" << order_ok << "  words=" << sounds << "  hearing=" << arm
                      << "  cuda_ms=" << time.milliseconds() << '\n';
        }
    }
    if (o.cap_questions) {
        Word amount = 2 * kinds * o.cap_questions, mask = o.cap_talk_arm;
        Function ask{};
        s.bind(ask, "capillary_converse", Image::runnels);
        Temporary result(s.d, Count(amount) * sizeof(SpokenCase));
        FieldTimer time(s.d);
        time.start(stream);
        launch(ask, (amount + 63) / 64, 64, brain.ptr, body, c, book.ptr, amount, mask, result.ptr);
        time.stop(stream);
        synchronize();
        std::vector<SpokenCase> replies(amount);
        s.d.check(s.d.download(replies.data(), result.ptr, replies.size() * sizeof(replies[0])),
                  "read words produced for unpractised contrasts");
        std::unique_ptr<EvidenceFile> record;
        if (!o.cap_speech_trace.empty()) {
            record = std::make_unique<EvidenceFile>(o.cap_speech_trace);
            record->out << "layout\ttwin\tquestion\trock\trow\tchange\tpigment\tpad\tprior\tafter\tnamed\trequested\ttr"
                           "ue_cause\tchosen_cause\twanted\tsaid\tknown\ton\toff\tpeg_materials\n";
        }
        Count examples[kinds]{}, right[kinds]{}, spoken[kinds]{}, absent[kinds]{}, invented[kinds]{}, pairs[kinds]{},
            both[kinds]{};
        for (Word i = 0; i < amount; ++i) {
            auto &v = replies[i];
            Word q = v.question;
            bool good = v.want && v.want == v.said;
            examples[q] += bool(v.want);
            right[q] += good;
            spoken[q] += bool(v.said);
            absent[q] += !v.want;
            invented[q] += !v.want && v.said;
            if (v.material && v.want && replies[i - kinds].want && v.want != replies[i - kinds].want) {
                ++pairs[q];
                both[q] += good && replies[i - kinds].said == replies[i - kinds].want;
            }
            if (record) {
                record->out << i / (2 * kinds);
                for (Count x : std::initializer_list<Count>{v.material, q, v.rock, v.row, v.changed, v.source, v.pad,
                                                            v.earlier, v.actual, v.named, v.goal, v.minimal, v.pieces,
                                                            v.want, v.said, v.known, v.tissue_on, v.tissue_off, v.pegs})
                    record->out << '\t' << x;
                record->out << '\n';
            }
        }
        if (record)
            record->publish();
        for (Word j = 0; j < kinds; ++j)
            std::cout << "capillary_question=" << j << "  answerable=" << examples[j] << "  exact=" << right[j]
                      << "  spoken=" << spoken[j] << "  ambiguous=" << absent[j] << "  invented=" << invented[j]
                      << "  different=" << pairs[j] << "  both=" << both[j] << '\n';
        std::cout << "capillary_interview=" << pupil.resident.slot << "  cases=" << amount
                  << "  answerable=" << std::accumulate(examples, examples + kinds, Count{})
                  << "  exact=" << std::accumulate(right, right + kinds, Count{}) << "  explanations=" << right[0]
                  << "  incompatible_causes=" << pairs[0] << "  both_causes=" << both[0]
                  << "  unsupported_causes=" << invented[0] << "  hearing=" << mask
                  << "  cuda_ms=" << time.milliseconds() << '\n';
    }
    if (o.cap_meetings) {
        storage::AgentImage child;
        child.read(o.cap_peer);
        auto receiver = phenotype(child);
        if (!child.hydraulic)
            throw std::invalid_argument("a listener needs its own capillary speech tissue");
        Word amount = o.cap_meetings * 2, arm = o.cap_talk_arm;
        Temporary hear(s.d, sizeof(Saved)), score(s.d, Count(amount) * sizeof(AdviceCase));
        s.d.check(s.d.upload(hear.ptr, &child.channels, sizeof(Saved)), "connect independent receiving tissue");
        Function meet{};
        s.bind(meet, "capillary_meeting", Image::runnels);
        FieldTimer time(s.d);
        time.start(stream);
        launch(meet, (amount + 63) / 64, 64, brain.ptr, hear.ptr, body, receiver, c, book.ptr, amount, arm, score.ptr);
        time.stop(stream);
        synchronize();
        std::vector<AdviceCase> receipts(amount);
        s.d.check(s.d.download(receipts.data(), score.ptr, receipts.size() * sizeof(receipts[0])),
                  "weigh remotely instructed repairs");
        std::unique_ptr<EvidenceFile> record;
        if (!o.cap_meeting_trace.empty()) {
            record = std::make_unique<EvidenceFile>(o.cap_meeting_trace);
            record->out << "layout\ttwin\tmodel_on\tmodel_off\tspeech\tbefore\tafter\texchanged_"
                           "material\tsurvivors\tcandidates\tprior_action\taction\twrong\twords\n";
        }
        Count before = 0, after = 0, other = 0, heard = 0, words = 0, wrong = 0, revised = 0;
        for (Word i = 0; i < amount; ++i) {
            auto &v = receipts[i];
            before += v.before;
            after += v.after;
            other += v.other;
            heard += v.kept != 0;
            revised += v.kept && v.kept < v.possible;
            words += v.teacher_view;
            wrong += v.wrong;
            if (record) {
                record->out << (i >> 1) << '\t' << (i & 1);
                for (Count x :
                     std::initializer_list<Count>{v.model_on, v.model_off, v.words, v.before, v.after, v.other, v.kept,
                                                  v.possible, v.baseline, v.designed, v.wrong, v.teacher_view})
                    record->out << '\t' << x;
                record->out << '\n';
            }
        }
        if (record)
            record->publish();
        std::cout << "capillary_exchange=" << pupil.resident.slot << "  listener=" << child.resident.slot
                  << "  cases=" << amount << "  words=" << words << "  interpreted=" << heard
                  << "  narrowed=" << revised << "  before=" << before << "  after=" << after
                  << "  other_material=" << other << "  failed_commitments=" << wrong << "  hearing=" << arm
                  << "  cuda_ms=" << time.milliseconds() << '\n';
    }
}
