struct FieldTimer {
    Driver &driver;
    Event begin{}, end{};
    explicit FieldTimer(Driver &api) : driver(api) {
        driver.check(driver.event_new(&begin, 0), "create field experiment clock");
        auto status = driver.event_new(&end, 0);
        if (status) {
            driver.event_free(begin);
            driver.check(status, "create end of field experiment clock");
        }
    }
    ~FieldTimer() {
        driver.event_free(end);
        driver.event_free(begin);
    }
    void start(Stream stream) { driver.check(driver.record(begin, stream), "begin field measurement"); }
    void stop(Stream stream) { driver.check(driver.record(end, stream), "end field measurement"); }
    float milliseconds() {
        float elapsed = 0;
        driver.check(driver.elapsed(&elapsed, begin, end), "measure field kernel");
        return elapsed;
    }
};
void field_portrait(const Habitat &h, Word slot) {
    const auto &animal = h.bodies[slot];
    auto shape = animal.body;
    std::cout << "field_portrait=" << slot << "  tick=" << h.header.tick << "  born=" << animal.born
              << "  generation=" << animal.generation << "  cells=" << 32 * h.conducting(slot)
              << "  modules=" << h.genomes[slot].module_count << "  body_plan=" << shape.shape
              << "  legs=" << shape.limbs << "  eyes=" << shape.eyes << "  pores=" << shape.pores << '\n';
}
void field_class(const Habitat &h, const Options &o) {
    if (!o.field_class && !o.class_retests)
        return;
    if (!h.recording_floats || !float_events.ptr)
        throw std::invalid_argument("a field class needs residents who can record real sightings");
    std::vector<Word> students;
    students.reserve(std::min(n, o.field_class));
    const Word stride = mix(o.class_seed + 37) | 1, first = mix(o.class_seed ^ 0x6483u);
    Word living = 0, observing = 0, organs = 0;
    for (Word j = 0; j < n; ++j) {
        Word slot = (first + j * stride) & (n - 1);
        if (!h.bodies[slot].energy)
            continue;
        ++living;
        if (!h.bodies[slot].body.eyes || !h.float_events[slot].stamp)
            continue;
        ++observing;
        auto &g = h.riggers[slot].genes;
        bool connected = (g.arbor & (3u << 14)) == (3u << 14);
        organs += connected;
        if (o.class_organs && !connected)
            continue;
        if (students.size() < o.field_class)
            students.push_back(slot);
    }
    if (o.class_retests) {
        if (o.slot >= n || !h.bodies[o.slot].energy || !h.bodies[o.slot].body.eyes || !h.float_events[o.slot].stamp)
            throw std::invalid_argument("this resident has no living field observation for a retest");
        students.assign(o.class_retests, o.slot);
        field_portrait(h, o.slot);
    }
    Word retest = bool(o.class_retests), prescribed = o.class_prescribed, history = o.class_history;
    Word size = students.size(), seed = o.class_seed, practice = o.class_practice;
    std::vector<braid::FieldStudent> scores(size);
    float kernel_ms = 0;
    if (size) {
        Temporary slots(s.d, Count(size) * sizeof(Word)), responses(s.d, Count(size) * sizeof(scores[0]));
        s.d.check(s.d.upload(slots.ptr, students.data(), Count(size) * sizeof(Word)), "enroll wild field observers");
        Function train{};
        s.bind(train, "braid_field_class", Image::runnels);
        FieldTimer timer(s.d);
        timer.start(stream);
        launch(train, (size + 31) / 32, 32, riggers.ptr, float_events.ptr, slots.ptr, size, seed, practice, retest,
               prescribed, history, responses.ptr);
        timer.stop(stream);
        synchronize();
        kernel_ms = timer.milliseconds();
        s.d.check(s.d.download(scores.data(), responses.ptr, Count(size) * sizeof(scores[0])),
                  "read individual field reports");
    }
    Word named = 0, solved = 0, qualified = 0, best = ~0u;
    Count offered = 0, correct = 0, changed = 0, both = 0, empty = 0, classroom = 0, answered = 0;
    std::unique_ptr<EvidenceFile> trace;
    if (!o.class_trace.empty()) {
        trace = std::make_unique<EvidenceFile>(o.class_trace);
        trace->out << "slot\tborn\tplastic\tarbor\timages\tknown\tstamp\tnames\tforms\toffered\tanswered\tcorrect\tchan"
                      "ged\tboth\tempty_answers\tclass_correct\tanswer_mask\tanswer_codes\tdiagnostic\tdialect_"
                      "key\tcourse_key\tdiagnostic_mask\tdiagnostic_codes\n";
    }
    for (Word i = 0; i < size; ++i) {
        auto &v = scores[i];
        Word slot = students[i];
        named += v.names == 8;
        qualified += v.names == 8 && v.diagnostic == 8;
        solved += v.correct && v.correct == v.offered;
        offered += v.offered;
        answered += v.answered;
        correct += v.correct;
        changed += v.changed;
        both += v.both;
        empty += v.empty_answers;
        classroom += v.class_correct;
        auto rank = [](const auto &r) { return std::array{r.diagnostic, r.names, r.forms}; };
        if (best == ~0u || rank(v) > rank(scores[best]))
            best = i;
        if (trace) {
            Word language = retest ? mix(seed + i * 7919) : mix(slot ^ seed);
            const auto &g = h.riggers[slot].genes;
            const auto &event = h.float_events[slot];
            trace->out << slot << '\t' << h.bodies[slot].born << '\t' << g.plastic << '\t' << g.arbor << '\t'
                       << event.images << '\t' << event.known << '\t' << event.stamp << '\t' << v.names << '\t'
                       << v.forms << '\t' << v.offered << '\t' << v.answered << '\t' << v.correct << '\t' << v.changed
                       << '\t' << v.both << '\t' << v.empty_answers << '\t' << v.class_correct << '\t' << v.answer_mask
                       << '\t' << v.replies << '\t' << v.diagnostic << '\t' << language << '\t'
                       << (prescribed ? history : language) << '\t' << v.diagnostic_mask << '\t' << v.diagnostic_codes
                       << '\n';
        }
    }
    if (trace)
        trace->publish();
    std::cout << "field_class=" << size << "  individual=" << retest << "  prescribed=" << prescribed
              << "  history=" << history << "  tick=" << h.header.tick << "  living=" << living
              << "  observing=" << observing << "  connected=" << organs << "  selected_organs=" << o.class_organs
              << "  seed=" << o.class_seed << "  practice=" << o.class_practice << "  named=" << named
              << "  qualified=" << qualified << "  complete=" << solved << "  offered=" << offered
              << "  answered=" << answered << "  correct=" << correct << "  changed=" << changed << "  both=" << both
              << "  without_event=" << empty << "  classroom_only=" << classroom << "  cuda_ms=" << kernel_ms
              << "  best_slot=" << (best == ~0u ? ~0u : students[best]) << '\n';
}

void calibrate_reader_watch(FieldWatch &watch) {
    Function calibrate{};
    s.bind(calibrate, "braid_class_laws", Image::runnels);
    s.bind(watch.scan, "braid_watch_class", Image::runnels);
    s.bind(watch.test, "braid_field_class", Image::runnels);
    launch(calibrate, 1, 32, watch.seed, watch.laws.ptr);
    synchronize();
    std::array<Word, 16> laws{};
    s.d.check(s.d.download(laws.data(), watch.laws.ptr, sizeof(laws)), "read isolated contact-law calibration");
    for (Word i = 0; i < laws.size(); ++i)
        if (laws[i])
            std::cout << "field_law=" << i << "  eight_names=" << laws[i] << "  tested=4\n";
}

bool reader_present(FieldWatch &watch) {
    s.d.check(s.d.zero(watch.seen.ptr, 0, 4), "clear the reader observation");
    launch(watch.scan, (n + 127) / 128, 128, riggers.ptr, animals.ptr, float_events.ptr, n, watch.laws.ptr,
           watch.bitmap.ptr, watch.seen.ptr);
    synchronize();
    Word observed = 0;
    ++watch.samples;
    s.d.check(s.d.download(&observed, watch.seen.ptr, 4), "check for a newly expressed contact table");
    if (!observed)
        return false;
    std::vector<Word> mask((n + 31) / 32), candidates;
    s.d.check(s.d.download(mask.data(), watch.bitmap.ptr, mask.size() * sizeof(Word)),
              "read possible pupil identities");
    for (Word i = 0; i < mask.size(); ++i)
        for (Word b = mask[i]; b; b &= b - 1)
            candidates.push_back(i * 32 + std::countr_zero(b));
    Word count = candidates.size(), key = watch.seed ^ 0x9ab7358fu;
    Temporary ids(s.d, candidates.size() * sizeof(Word)), answers(s.d, candidates.size() * sizeof(braid::FieldStudent));
    s.d.check(s.d.upload(ids.ptr, candidates.data(), candidates.size() * sizeof(Word)), "enroll prospective pupils");
    Word regular = 0;
    launch(watch.test, (count + 31) / 32, 32, riggers.ptr, float_events.ptr, ids.ptr, count, key, watch.practice,
           regular, regular, regular, answers.ptr);
    synchronize();
    std::vector<braid::FieldStudent> scores(count);
    s.d.check(s.d.download(scores.data(), answers.ptr, scores.size() * sizeof(scores[0])),
              "inspect independent reader qualification");
    watch.examined += count;
    for (Word i = 0; i < count; ++i) {
        if (scores[i].diagnostic == 8 && scores[i].names == 8) {
            watch.slot = candidates[i];
            return true;
        }
    }
    return false;
}

void field_interview(Habitat &h, const storage::AgentImage &selected, const std::string &path) {
    Word slot = selected.resident.slot;
    const auto &living = h.bodies[slot], &original = selected.resident.animal;
    if (!same_field_individual(h, selected)) {
        std::cout << "field_interview=lost  slot=" << slot << "  born=" << original.born << "  tick=" << h.header.tick
                  << '\n';
        return;
    }
    if (!float_events.ptr)
        throw std::invalid_argument("field questions need a habitat that records visual episodes");
    field_portrait(h, slot);
    std::ifstream file;
    if (path != "-") {
        file.open(path);
        if (!file)
            throw std::invalid_argument("cannot open field questions " + path);
    }
    std::istream &input = path == "-" ? std::cin : static_cast<std::istream &>(file);
    auto found = std::ranges::find(h.narrators, slot, &Habitat::ResidentNarration::slot);
    braid::Narration local = found == h.narrators.end() ? braid::Narration{} : found->mind;
    Temporary memories(s.d, sizeof(local)), output(s.d, sizeof(braid::Answer));
    s.d.check(s.d.upload(memories.ptr, &local, sizeof(local)), "present this resident's acquired speech contacts");
    Function listen{};
    s.bind(listen, "braid_listen_field", Image::runnels);
    Word turns = 0, answered = 0;
    FieldTimer timer(s.d);
    double kernel_ms = 0;
    std::string line;
    const auto &episode = h.float_events[slot];
    while (std::getline(input, line)) {
        if (line == "/quit" || line == "/quit\r")
            break;
        std::istringstream words(line);
        std::string word;
        braid::Speech heard{};
        bool audible = true, any = false;
        while (words >> word) {
            any = true;
            if (heard.n == braid::max_phrase) {
                audible = false;
                break;
            }
            try {
                heard.wave[heard.n] = acoustic::spoken_word(word);
            } catch (const std::invalid_argument &) {
                audible = false;
                break;
            }
            if (!heard.wave[heard.n]) {
                audible = false;
                break;
            }
            ++heard.n;
        }
        if (!any)
            continue;
        braid::Answer reply{};
        if (audible) {
            timer.start(stream);
            launch(listen, 1, 1, memories.ptr, float_events.ptr, slot, heard, output.ptr);
            timer.stop(stream);
            synchronize();
            kernel_ms += timer.milliseconds();
            s.d.check(s.d.download(&reply, output.ptr, sizeof(reply)), "hear the field resident's reply");
        }
        ++turns;
        answered += reply.answered != 0;
        std::cout << "witness> " << (reply.answered ? acoustic::spell(reply.words.wave[0]) : "?") << '\n'
                  << "field_utterance=" << turns << "  matched=" << reply.matched << "  answered=" << reply.answered
                  << "  said=" << reply.said << "  wave=" << (reply.answered ? reply.words.wave[0] : 0)
                  << "  stamp=" << episode.stamp << '\n'
                  << std::flush;
    }
    if (input.bad())
        throw std::runtime_error("could not finish reading field questions");
    if (found != h.narrators.end())
        s.d.check(s.d.download(&found->mind, memories.ptr, sizeof(local)),
                  "retain this resident's conversational context");
    std::cout << "field_interview=present  slot=" << slot << "  born=" << original.born << "  tick=" << h.header.tick
              << "  cells=" << 32 * h.conducting(slot) << "  generation=" << living.generation
              << "  images=" << episode.images << "  known=" << episode.known << "  stamp=" << episode.stamp
              << "  questions=" << turns << "  cuda_ms=" << kernel_ms << "  answered=" << answered << '\n';
}
