void signal_study(const Habitat &h, const Options &o, const std::vector<Word> &slots, DevicePtr bodies,
                  DevicePtr appetites, const storage::AgentImage *tutor) {
    using namespace capillary;
    Word n = slots.size();
    auto course = o.signal;
    course.seed = o.class_seed;
    Temporary fixtures(s.d, Count(course.tests + 1) * sizeof(Junction)), results(s.d, Count(n) * sizeof(SignalGrade));
    Function prepare{}, learn{};
    s.bind(prepare, "signal_junctions", Image::runnels);
    s.bind(learn, "signal_pairs", Image::runnels);
    Word beds_count = course.tests + 1;
    launch(prepare, (beds_count + 127) / 128, 128, course.seed, course.holdout, beds_count, fixtures.ptr);
    FieldTimer time(s.d);
    time.start(stream);
    SignalMemory teacher = tutor ? connect_signal(tutor->appetitive, phenotype(*tutor)) : SignalMemory{};
    launch(learn, n, 32, bodies, appetites, n, course, teacher, fixtures.ptr, results.ptr);
    time.stop(stream);
    synchronize();
    std::vector<SignalGrade> grades(n);
    std::vector<Junction> beds(course.tests + 1);
    s.d.check(s.d.download(grades.data(), results.ptr, Count(n) * sizeof(SignalGrade)),
              "count independent chemical communications");
    s.d.check(s.d.download(beds.data(), fixtures.ptr, Count(beds.size()) * sizeof(Junction)),
              "read the pressure junctions used for the signalling test");
    constexpr const char *names[] = {"before",          "after",          "initial_both",    "both",
                                     "initial_mastery", "final_mastery",  "acquired",        "lost",
                                     "reliable_left",   "reliable_right", "erased",          "inverse",
                                     "written_left",    "written_right",  "left_convention", "right_convention",
                                     "marks",           "early_food",     "late_food",       "training_food",
                                     "training_heat",   "sender_edits",   "receiver_edits"};
    static_assert(std::size(names) == signal_metrics);
    std::unique_ptr<EvidenceFile> out;
    if (!o.cap_screen.empty()) {
        out = std::make_unique<EvidenceFile>(o.cap_screen);
        out->out << "sender\treceiver\tsender_born\treceiver_born\tsender_cells\treceiver_cells\tsender_rule\treceiver_"
                    "rule\thistories\ttests\tlessons";
        if (tutor)
            out->out << "\ttutor_role";
        for (auto k : names)
            out->out << '\t' << k;
        out->out << '\n';
    }
    std::array<Count, signal_metrics> tally{};
    Word both_all = 0, best = 0, shift = 1 + mix(course.seed + 9137) % (n - 1);
    for (Word j = 0; j < n; ++j) {
        Word a = slots[j], b = slots[(j + shift) % n];
        auto &g = grades[j];
        if (std::tuple{g.total[final_mastery], g.total[after_correct], g.total[acquired_mastery]} >
            std::tuple{grades[best].total[final_mastery], grades[best].total[after_correct],
                       grades[best].total[acquired_mastery]})
            best = j;
        both_all += g.total[final_mastery] == course.replicas;
        if (out) {
            bool adult = course.tutor == 1, visitor = course.tutor == 2;
            if (adult)
                b = a;
            out->out << (adult ? tutor->resident.slot : a);
            for (Word v :
                 {visitor ? tutor->resident.slot : b, adult ? tutor->resident.animal.born : h.bodies[a].born,
                  visitor ? tutor->resident.animal.born : h.bodies[b].born,
                  adult ? 32 * (tutor->grows_cells ? tutor->cortex.active
                                                   : living_tiles(tutor->brain.tiles, tutor->brain.genes.development))
                        : 32 * h.conducting(a),
                  visitor ? 32 * (tutor->grows_cells ? tutor->cortex.active
                                                     : living_tiles(tutor->brain.tiles, tutor->brain.genes.development))
                          : 32 * h.conducting(b),
                  adult ? (teacher.body.growth.law & 15) : ((h.riggers[a].genes.plastic >> 8) & 15),
                  visitor ? (teacher.body.growth.law & 15) : ((h.riggers[b].genes.plastic >> 8) & 15), course.replicas,
                  course.tests, course.lessons})
                out->out << '\t' << v;
            if (tutor)
                out->out << '\t' << course.tutor;
            for (Word v : g.total)
                out->out << '\t' << v;
            out->out << '\n';
        }
        for (Word k = 0; k < signal_metrics; ++k)
            tally[k] += g.total[k];
    }
    if (out)
        out->publish();
    auto shape = beds[0];
    Count cases = Count(n) * course.replicas * course.tests * 2;
    if (tally[erased_correct] * 2 != cases || tally[finally_both] != tally[left_code] + tally[right_code])
        throw std::logic_error("the isolated receivers disagreed with their matched no-mark condition");
    std::cout << "material_relay=" << tick << "  pairs=" << n << "  lessons=" << course.lessons
              << "  histories=" << course.replicas << "  tests=" << course.tests << "  cases=" << cases
              << "  retained=" << course.retained << "  medium=" << course.inert << "  feedback=" << course.feedback
              << "  full_pairs=" << both_all << "  tutor=" << course.tutor << "  learning=" << course.learners
              << "  sender=" << (course.tutor == 1 ? tutor->resident.slot : slots[best]) << "  receiver="
              << (course.tutor == 2   ? tutor->resident.slot
                  : course.tutor == 1 ? slots[best]
                                      : slots[(best + shift) % n])
              << "  best_correct=" << grades[best].total[after_correct]
              << "  best_early=" << grades[best].total[before_correct]
              << "  best_erased=" << grades[best].total[erased_correct] << "  training_row=" << shape.row
              << "  geology=" << shape.base << "  bound=" << shape.coating << "  carrier=" << shape.carrier
              << "  contact_bytes=" << sizeof(SignalMemory::contacts)
              << "  delivered_portions=" << 2 * tally[training_food]
              << "  remaining_portions=" << 2 * (Count(n) * course.replicas * course.lessons - tally[training_food])
              << "  cuda_ms=" << time.milliseconds();
    for (Word k = 0; k < signal_metrics; ++k)
        std::cout << "  " << names[k] << '=' << tally[k];
    std::cout << '\n';
}

void signal_individual(const Options &o) {
    using namespace capillary;
    storage::AgentImage a, b;
    a.read(o.signal_adult);
    b.read(o.signal_youth);
    if (a.resident.slot == b.resident.slot && a.resident.animal.born == b.resident.animal.born &&
        a.resident.germ.grip == b.resident.germ.grip)
        throw std::invalid_argument("choose two independent residents for the material relay");
    Signalling course = o.signal;
    course.seed = o.class_seed;
    Word beds_count = course.tests + 1, rows = course.tests * 2;
    SignalMemory couple[]{connect_signal(a.appetitive, phenotype(a)), connect_signal(b.appetitive, phenotype(b))};
    if (!couple[0].body.eyes || !couple[1].body.eyes || !couple[0].body.limbs || !couple[1].body.limbs ||
        !couple[0].body.mouth || !couple[1].body.mouth)
        throw std::invalid_argument("the two foragers need working eyes, limbs and feeding pores");
    std::vector<SignalMemory> village;
    auto identity = [](const storage::AgentImage &p) {
        return std::tuple{p.resident.slot, p.resident.animal.born, p.resident.germ.grip};
    };
    std::set<decltype(identity(a))> identities{identity(a), identity(b)};
    for (auto &path : o.signal_neighbours) {
        storage::AgentImage visitor;
        visitor.read(path);
        if (!identities.insert(identity(visitor)).second)
            throw std::invalid_argument("choose distinct residents for the neighbourhood");
        auto participant = connect_signal(visitor.appetitive, phenotype(visitor));
        if (!participant.body.eyes || !participant.body.limbs || !participant.body.mouth)
            throw std::invalid_argument("an adult neighbour needs its sensory and feeding organs");
        village.push_back(participant);
    }
    Word neighbours = village.size();
    Temporary group(s.d, Count(neighbours) * sizeof(SignalMemory));
    if (neighbours)
        s.d.check(s.d.upload(group.ptr, village.data(), Count(neighbours) * sizeof(SignalMemory)),
                  "connect the independent adult foragers");
    Temporary rigs(s.d, Count(beds_count) * sizeof(Junction)), brains(s.d, sizeof(couple)),
        bill(s.d, sizeof(SignalGrade)), receipts(s.d, Count(rows) * sizeof(SignalTransfer));
    s.d.check(s.d.upload(brains.ptr, couple, sizeof(couple)), "connect the two residents' appetitive synapses");
    Function prepare{}, train{}, test{};
    s.bind(prepare, "signal_junctions", Image::runnels);
    s.bind(train, "signal_rear_pair", Image::runnels);
    s.bind(test, "signal_grade_pair", Image::runnels);
    launch(prepare, (beds_count + 127) / 128, 128, course.seed, course.holdout, beds_count, rigs.ptr);
    FieldTimer time(s.d);
    time.start(stream);
    launch(train, 1, 1, brains.ptr, course, rigs.ptr, group.ptr, neighbours, bill.ptr);
    launch(test, (rows + 63) / 64, 64, brains.ptr, course, rigs.ptr, receipts.ptr);
    time.stop(stream);
    synchronize();
    std::vector<SignalTransfer> trials(rows);
    std::vector<Junction> fixtures(beds_count);
    s.d.check(s.d.download(trials.data(), receipts.ptr, Count(rows) * sizeof(trials[0])),
              "read independent material messages");
    s.d.check(s.d.download(fixtures.data(), rigs.ptr, Count(beds_count) * sizeof(Junction)),
              "record visible junction geometry");
    s.d.check(s.d.download(couple, brains.ptr, sizeof(couple)), "retain the two resident spiking states");
    SignalGrade paid{};
    s.d.check(s.d.download(&paid, bill.ptr, sizeof(paid)), "count delivered food and spiking edits");
    std::unique_ptr<EvidenceFile> out;
    if (!o.signal_trace.empty()) {
        out = std::make_unique<EvidenceFile>(o.signal_trace);
        out->out << "trial\tcue\tgeology\tinput\tbound\tcarrier\twrite\tsensed\tchoice\tno_mark\tinverse\tfood\tcost\n";
    }
    Word correct = 0, bare = 0, other = 0, left = 0, right = 0, both = 0;
    for (Word i = 0; i < rows; ++i) {
        auto r = trials[i];
        auto j = fixtures[i / 2 + 1];
        correct += r.food;
        bare += r.erased == r.cue;
        other += r.inverse == r.cue;
        if ((i & 1) && r.food && trials[i - 1].food) {
            ++both;
            left += trials[i - 1].write;
            right += r.write;
        }
        if (out) {
            out->out << i / 2;
            for (Word v : {r.cue, j.base, j.row, j.coating, j.carrier, r.write, r.light, r.choice, r.erased, r.inverse,
                           r.food, r.cost})
                out->out << '\t' << v;
            out->out << '\n';
        }
    }
    if (out)
        out->publish();
    if (bare * 2 != rows || left + right != both)
        throw std::logic_error("a signal leaked across the isolated receiver");
    if (!o.signal_save_adult.empty()) {
        auto keep = [](storage::AgentImage &p, const SignalMemory &m, const std::string &path) {
            p.material_signals = p.hydraulic = true;
            p.appetitive = signal_tissue(m);
            p.brain.tissue[utterance::band_of(p)].spike = m.body.cells.spike;
            storage::write_checked(path, p);
        };
        keep(a, couple[0], o.signal_save_adult);
        keep(b, couple[1], o.signal_save_youth);
    }
    std::cout << "material_correspondence=" << a.resident.slot << "  receiver=" << b.resident.slot
              << "  lessons=" << course.lessons << "  neighbours=" << neighbours + 1 << "  learning=" << course.learners
              << "  history=" << course.first << "  test_seed=" << course.holdout << "  tests=" << course.tests
              << "  cases=" << rows << "  correct=" << correct << "  erased=" << bare << "  inverse=" << other
              << "  both=" << both << "  left_convention=" << left << "  right_convention=" << right
              << "  contact_bytes=" << sizeof(SignalMemory::contacts) << "  sender_on=" << signal_tissue(couple[0]).on
              << "  sender_off=" << signal_tissue(couple[0]).off << "  receiver_on=" << signal_tissue(couple[1]).on
              << "  receiver_off=" << signal_tissue(couple[1]).off << "  food=" << paid.total[training_food]
              << "  heat=" << paid.total[training_cost] << "  sender_edits=" << paid.total[sender_changes]
              << "  receiver_edits=" << paid.total[receiver_changes] << "  cuda_ms=" << time.milliseconds() << '\n';
}
