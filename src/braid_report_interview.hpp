struct ConnectedBand {
    Word index = 0;
    DevicePtr address = 0;
    regulation::Source organ{};
};
ConnectedBand connect_cortex(Word who, bool sound = false) {
    auto &pupil = float_actor(who);
    Word width =
        pupil.grows_cells ? pupil.cortex.active : living_tiles(pupil.brain.tiles, pupil.brain.genes.development);
    Word band = sound ? regulation::auditory_band(pupil.resident.germ, width)
                      : regulation::innervated_band(pupil.resident.germ, width);
    auto tissue = pupil.brain.tissue[band];
    DevicePtr live_band = 0;
    if (who == 0 || (who == 1 && !visitor)) {
        live_band = somatic_bands[who] + Count(band) * sizeof(Tissue);
        s.d.check(s.d.download(&tissue, live_band, sizeof(tissue)), "read the speaker's current somatic band");
    }
    auto source = regulation::source(pupil.resident.germ, band, tissue);
    s.d.check(s.d.upload(float_regulator.ptr, &source, sizeof(source)), "connect the speaker's cortical program");
    return {band, live_band, source};
}
void retain_spikes(Word who, ConnectedBand band, const regulation::Source &source) {
    float_actor(who).brain.tissue[band.index].spike = source.cells.spike;
    if (band.address)
        s.d.check(s.d.upload(band.address + offsetof(Tissue, spike), &source.cells.spike, sizeof(Word)),
                  "return the speaker's cortical spikes");
}
void return_spikes(Word who, ConnectedBand band) {
    regulation::Source source{};
    s.d.check(s.d.download(&source, float_regulator.ptr, sizeof(source)), "read the learning response");
    retain_spikes(who, band, source);
}
Word report_people() const { return float_worker ? 3 : 2; }
void report_memory(bool upload) {
    std::array<braid::Narration, 3> memories{};
    if (upload) {
        for (Word j = 0; j < report_people(); ++j)
            memories[j] = float_actor(j).braid_report.memory;
        s.d.check(s.d.upload(float_memories.ptr, memories.data(), sizeof(memories)),
                  "present individual event memories");
    } else {
        s.d.check(s.d.download(memories.data(), float_memories.ptr, sizeof(memories)), "retain private event memories");
        for (Word j = 0; j < report_people(); ++j) {
            auto &person = float_actor(j);
            if (!person.binds_floats)
                continue;
            person.braid_report.memory = memories[j];
            person.narrates_floats = memories[j].event.stamp || memories[j].labels || memories[j].forms[0].examples ||
                                     memories[j].auditory_route;
        }
    }
}
bool report_command(const std::string &token, std::istringstream &input, const std::string &line) {
    bool ordinary = !token.empty() && token[0] != '/';
    if (!float_memories.ptr || (ordinary && !local.narrates_floats))
        return false;
    braid::Course c{};
    std::string item;
    auto take = [&]() {
        if (!(input >> item))
            throw std::invalid_argument("missing event inquiry argument");
        return item;
    };
    auto who = [&]() {
        auto word = take();
        if (word == "speaker")
            return 0u;
        if (word == "partner" && partner().binds_floats)
            return 1u;
        if (word == "worker" && float_worker)
            return 2u;
        throw std::invalid_argument("choose an available event speaker");
    };
    auto phrase = [&](std::istream &in, bool blank = false) {
        braid::Speech q{};
        std::string word;
        while (in >> word) {
            if (q.n == braid::max_phrase)
                return braid::Speech{};
            if (word == "*" && blank) {
                if (c.slot != braid::none)
                    throw std::invalid_argument("use a single pointing slot");
                c.slot = q.n;
            } else if (!(q.wave[q.n] = acoustic::spoken_word(word)))
                return braid::Speech{};
            ++q.n;
        }
        return q;
    };
    c.peer = report_witness;
    c.vision = report_shutters;
    if(token=="/braid-feedback"){
        Word person=who();auto choice=take();
        auto &route = float_actor(person).braid_report.memory.auditory_route;
        const bool routed = choice == "shared" || choice == "auditory";
        const bool auditory = choice == "auditory";
        if (routed)
            choice = "on";
        if((choice!="on" && choice!="off" && choice!="quiet" && choice!="remodel" && choice!="quiet-remodel") || input>>item)
            throw std::invalid_argument(
                "choose on, auditory, shared, quiet, remodel, quiet-remodel or off for this cortical contact");
        if (routed)
            route = auditory;
        bool growth=choice=="remodel" || choice=="quiet-remodel";
        Word base=choice=="off"?apprentice::unrestricted:(choice=="quiet" || choice=="quiet-remodel")?apprentice::quiet:apprentice::inherited;
        lesson_modes[person]=base|(growth?apprentice::remodel:0u);
        std::cout << "speech_feedback=" << choice << "  person=" << person << "  auditory_route=" << Word(route)
                  << '\n';
        return true;
    }
    if (token == "/braid-witness") {
        auto name = take();
        if (name == "auto" && float_worker) {
            report_witness = 1;
            choose_reporter = true;
            return true;
        }
        if (name == "partner" && partner().binds_floats)
            report_witness = 1;
        else if (name == "worker" && float_worker)
            report_witness = 2;
        else
            throw std::invalid_argument("choose an available partner, worker, or auto with two witnesses");
        choose_reporter = false;
        return true;
    }
    if (token == "/braid-signature") {
        Word agent = who();
        report_names[agent] = acoustic::spoken_word(take());
        return true;
    }
    if (token == "/braid-glass") {
        if (!at_braid)
            throw std::invalid_argument("open the float table before fitting its viewing instrument");
        c.who = who();
        c.cmd = braid::set_lens;
        c.place = float_paint(take());
        c.board = float_paint(take());
        if (!braid::bijective(c.place) || !braid::bijective(c.board))
            throw std::invalid_argument("each lens must send four positions and pigments to four distinct receptors");
    } else if (token == "/braid-align" || token == "/braid-compare" || token == "/braid-perspective") {
        c.who = who();
        c.peer = who();
        if (c.who == c.peer)
            throw std::invalid_argument("choose two independent eyes");
        c.cmd = token == "/braid-align"     ? braid::share_view
                : token == "/braid-compare" ? braid::compare_views
                                            : braid::inspect_view;
        if (c.cmd != braid::inspect_view) {
            if (!at_braid)
                throw std::invalid_argument("open the float table before comparing views");
            c.budget = parse(take(), 4);
            if (c.cmd == braid::compare_views) {
                c.reps = parse(take(), 256);
                c.seed = parse(take(), ~0u);
            }
        }
    } else if (ordinary) {
        c.cmd = braid::normal_query;
        c.channel = report_channel;
        std::istringstream words(line);
        try {
            c.words = phrase(words);
        } catch (const std::invalid_argument &) {
            return false;
        }
        if (!c.words.n)
            return false;
    } else if (token == "/braid-shutters") {
        Word member = who(), bits = parse(take(), 15);
        report_shutters = (report_shutters & ~(15u << (4 * member))) | (bits << (4 * member));
        return true;
    } else if (token == "/braid-replies") {
        auto channel = take();
        const std::array<std::string, 5> channels{"manual", "auto", "mute", "echo", "wrong"};
        auto i = std::find(channels.begin(), channels.end(), channel);
        if (i == channels.end())
            throw std::invalid_argument("choose manual, auto, mute, echo or wrong");
        report_channel = i - channels.begin();
        return true;
    } else if (token == "/braid-gloss" || token == "/braid-pattern" || token == "/inlet-pattern" ||
               token == "/braid-point" || token == "/braid-caption") {
        if (!at_braid)
            throw std::invalid_argument("open the demonstration tray with /braid SEED");
        c.who = who();
        auto &person = float_actor(c.who);
        if (!person.resident.animal.energy || !person.resident.animal.body.eyes)
            throw std::invalid_argument("demonstrations need a living observer with eyes");
        if (token == "/braid-gloss") {
            c.cmd = braid::name_lesson;
            c.reps = parse(take(), 256);
            c.seed = parse(take(), ~0u);
            c.names = phrase(input);
            if (c.names.n != 8 || std::set<Count>(c.names.wave, c.names.wave + 8).size() != 8)
                throw std::invalid_argument("give four distinct socket names followed by four distinct pigment names");
        } else if (token == "/braid-point") {
            c.cmd = braid::point_lesson;
            c.place = parse(take(), 3);
            c.words.wave[0] = acoustic::spoken_word(take());
        } else if (token == "/braid-caption") {
            c.cmd = braid::manual_caption;
            std::string request;
            if (!std::getline(input, request, '|') || input.eof())
                throw std::invalid_argument("give a question | observed reply");
            std::istringstream words(request);
            c.words = phrase(words);
            c.echo = acoustic::spoken_word(take());
        } else {
            c.cmd = braid::form_lesson;
            c.names = demo_names[c.who];
            if (!c.names.n)
                throw std::invalid_argument("supply the demonstrator's acoustic names with /braid-gloss first");
            auto index = [&](std::initializer_list<std::string_view> choices) {
                auto word = take();
                auto it = std::find(choices.begin(), choices.end(), word);
                if (it == choices.end())
                    throw std::invalid_argument("unknown event lesson argument " + word);
                return Word(it - choices.begin());
            };
            c.frame = index({"before", "actual", "plan", "short"});
            auto ref = index({"explicit", "topic", "reply"});
            c.role = ref ? ref + 1 : 0;
            c.kind = index({"port", "dye", "either"});
            c.reps = parse(take(), 256);
            c.seed = parse(take(), ~0u);
            c.words = phrase(input, true);
            if (!c.words.n || ((c.slot != braid::none) != (c.role == 0)))
                throw std::invalid_argument("use * for the indicated noun in an explicit question");
        }
    } else if (token == "/braid-forget-topic") {
        c.who = who();
        c.cmd = braid::clear_topic;
    } else if (token == "/braid-consult") {
        c.cmd = braid::witness_reply;
        c.channel = report_channel;
    } else if (token == "/braid-memory") {
        c.who = who();
        c.cmd = braid::read_patterns;
    } else
        return false;
    if (input >> item && !ordinary)
        throw std::invalid_argument("unexpected event inquiry argument");
    if (c.peer == c.who)
        c.peer = c.who ? 0 : 1;
    c.address = report_names[c.peer] ? report_names[c.peer] : braid::timbre(float_actor(c.peer).braided.mind);
    if (choose_reporter && float_worker && (c.cmd == braid::normal_query || c.cmd == braid::witness_reply)) {
        c.alternative = 2;
        c.second_voice = report_names[2] ? report_names[2] : braid::timbre(float_actor(2).braided.mind);
    }
    if (!float_actor(c.who).riverine || !float_actor(c.who).binds_floats)
        return false;
    for (Word j = 0; j < report_people(); ++j) {
        auto &person = float_actor(j);
        s.d.check(s.d.upload(float_head.ptr + Count(j) * sizeof(braid::Saved), &person.braided, sizeof(braid::Saved)),
                  "restore the listener's plasticity");
    }
    report_memory(true);
    Word mode = lesson_modes[c.who];
    if (token == "/inlet-pattern") {
        inlet_tuition(c, mode);
        return true;
    }
    bool regulated =
        mode != apprentice::unrestricted && (c.cmd == braid::form_lesson || c.cmd == braid::manual_caption);
    ConnectedBand band{};
    if (regulated)
        band = connect_cortex(c.who, float_actor(c.who).braid_report.memory.auditory_route);
    void *args[]{
        &float_memories.ptr, &float_head.ptr, &float_tray.ptr, &c, &float_answer.ptr, &float_regulator.ptr, &mode};
    s.d.check(s.d.launch(regulated ? regulated_recall : float_recall, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "hear a question about an experienced event");
    if (regulated)
        return_spikes(c.who, band);
    braid::Receipt r{};
    s.d.check(s.d.download(&r, float_answer.ptr, sizeof(r)), "hear an event reply");
    if (ordinary && !r.status)
        return false;
    report_memory(false);
    if (c.cmd == braid::name_lesson)
        demo_names[c.who] = c.names;
    if (c.cmd == braid::share_view || c.cmd == braid::compare_views || c.cmd == braid::inspect_view)
        std::cout << "braid_view=" << c.cmd << "  person=" << c.who << "  witness=" << c.peer
                  << "  anchored=" << r.established << "  candidates=" << r.hypotheses << "  panels=" << r.examples
                  << "  questions=" << r.asked << "  replies=" << r.replies << "  cost=" << r.cost << '\n';
    if (c.alternative != braid::none && r.approached != braid::none)
        std::cout << "braid_audience=" << r.approached << "  partner_coverage=" << r.coverage[0]
                  << "  worker_coverage=" << r.coverage[1] << '\n';
    auto &a = r.turns;
    if (a.original.queried)
        std::cout << "speaker asks> " << float_words(a.original.words) << "?\n";
    if (a.witness.answered && c.channel != 2)
        std::cout << "witness> " << float_words(a.witness.words) << '\n';
    if (a.final.answered || a.original.answered)
        std::cout << "speaker> " << float_words(a.final.answered ? a.final.words : a.original.words) << '\n';
    else if (ordinary && !a.original.queried)
        std::cout << "speaker> ?\n";
    std::cout << "braid_recall=" << c.cmd << "  person=" << c.who << "  nouns=" << r.nouns << "  forms=" << r.forms
              << "  offered=" << r.offered << "  cost=" << r.cost << "  writes=" << r.learned
              << "  asked=" << a.original.queried << "  witnessed=" << a.witness.answered
              << "  received=" << a.final.understood << "  answered=" << (a.final.answered || a.original.answered)
              << "  from_testimony=" << a.original.by_report << '\n';
    return true;
}
