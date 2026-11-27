static std::string audible(const effort::Speech &s) {
    std::string line;
    for (Word i = 0; i < s.n; ++i) {
        if (i)
            line += ' ';
        line += s.sounds[i] ? acoustic::spell(s.sounds[i]) : "_";
    }
    return line;
}
void studio_tongues(Word &senses) {
    std::array<linkage::Tongue, 2> organs{};
    for (Word i = 0; i < 2; ++i) {
        auto &p = i ? partner() : local;
        organs[i] = {p.native_words, p.path_speech.words, p.calls.animal, p.referent.genes};
        auto &b = p.resident.animal.body;
        senses |= (Word(!!b.eyes) | (Word(!!b.limbs) << 1) | (Word(!!b.pores) << 2)) << (3 * i);
    }
    s.d.check(s.d.upload(rack_voices.ptr, organs.data(), sizeof(organs)), "restore each animal's acoustic mappings");
}
void studio_trials(std::istream &input) {
    std::string file, path, extra;
    if (!(input >> std::quoted(file) >> std::quoted(path)) || input >> extra)
        throw std::invalid_argument("use /studio-trials QUESTIONS NEW_TRACE");
    std::ifstream in(file);
    if (!in)
        throw std::invalid_argument("cannot open contingent questions");
    if (std::filesystem::exists(path))
        throw std::invalid_argument("the trace path already exists");
    struct Grade {
        std::string arm, group;
        Word attainable = 0;
    };
    std::vector<practice::Invitation> tasks;
    std::vector<Grade> grades;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream row(line);
        std::string a, b, c;
        if (!std::getline(row, a, '|') || !std::getline(row, b, '|') || !std::getline(row, c, '|'))
            throw std::invalid_argument("invalid contingent row");
        std::istringstream setup(a), words(b), reference(c);
        practice::Invitation r{};
        Grade key{};
        if (!(setup >> r.world >> r.fixture >> r.width >> r.weather >> r.practice >> r.wire >> r.wallet >> r.spool >>
              r.lock >> r.care) ||
            setup >> extra || r.width < 1 || r.width > 4 || r.weather > 1 || r.practice > 3 || r.wire > 7 ||
            r.wallet > 16 || r.spool > 1 || r.lock > 255 || r.care > 255 ||
            !(reference >> key.arm >> key.group >> key.attainable) || key.attainable > 1 || reference >> extra ||
            tasks.size() >= 1048576)
            throw std::invalid_argument("invalid public apparatus or external grade");
        r.request = plan_words(words);
        std::string revised;
        if (std::getline(row, revised, '|')) {
            std::istringstream amended(revised);
            r.revision = plan_words(amended);
            if (!r.revision.n || row >> extra)
                throw std::invalid_argument("malformed revised order");
        }
        tasks.push_back(r);
        grades.push_back(key);
    }
    if (tasks.empty())
        throw std::invalid_argument("empty studio questions");
    std::ofstream trace(path);
    if (!trace)
        throw std::invalid_argument("cannot create studio trace");
    trace << "id\tarm\tgroup\tworld\tfixture\thidden\twidth\tbudget\tstock\tstart\tgoal\tcare\tglyphs\tjoints\thandle"
             "s\tlabels\tbuttons\treader\twords\tquery\treport\tprocedure\tstroke\twatch\tseen\tlocal\tpassed\tsteps"
             "\tfitted\theat\tfood\tfinal\tmodels\tbytes\trevised\tquery2\treport2\tsecond_move\tsecond_"
             "watch\tsecond_face\tsecond_local\n";
    Word n = tasks.size(), batch = std::min(n, 512u), balance = partner().resident.animal.energy, senses = 0;
    studio_tongues(senses);
    Temporary inputs(s.d, batch * sizeof(tasks[0])), outputs(s.d, batch * sizeof(practice::Record));
    Function fn{};
    s.bind(fn, "practice_conversations", Image::siphons);
    std::vector<practice::Record> heard(batch);
    std::map<std::pair<std::string, std::string>, std::array<Count, 13>> marks;
    struct Pair {
        std::string opening[2], report[2], moves[2];
        Word seen = 0, done = 0;
        bool repeated = false;
    };
    std::map<std::tuple<std::string, std::string, Word, Word, Word, std::string, std::string>, Pair> twins;
    auto start = std::chrono::steady_clock::now();
    for (Word off = 0; off < n; off += batch) {
        Word count = std::min(n - off, batch);
        s.d.check(s.d.upload(inputs.ptr, tasks.data() + off, count * sizeof(tasks[0])),
                  "place the two public apparatuses and independent hidden calibrations");
        void *args[]{&studio_brain.ptr, &rack_voices.ptr, &inputs.ptr, &count, &balance, &senses, &outputs.ptr};
        s.d.check(s.d.launch(fn, (count + 63) / 64, 1, 1, 64, 1, 1, 0, nullptr, args, nullptr),
                  "exchange a question, a real intervention and an observed consequence");
        s.d.check(s.d.download(heard.data(), outputs.ptr, count * sizeof(heard[0])),
                  "transcribe the separate sensory and motor messages");
        for (Word i = 0; i < count; ++i) {
            auto &r = heard[i];
            auto &k = grades[off + i];
            auto &t = tasks[off + i];
            auto &v = marks[{k.arm, k.group}];
            auto &a = r.source, &b = r.inspection, &c = r.worker;
            bool actual = c.recognised && !((r.after ^ t.lock) & t.care);
            bool understood = (r.care == t.care) && !((r.goal ^ t.lock) & t.care);
            ++v[0];
            v[1] += k.attainable;
            v[2] += actual;
            v[3] += a.asked + r.amendment.asked;
            v[4] += b.recognised + r.second.recognised;
            v[5] += b.decoded + r.second.decoded;
            v[6] += actual && a.asked;
            v[7] += b.spent + r.second.spent + c.spent;
            v[8] += c.food;
            v[9] += actual && c.fitted;
            v[10] += understood;
            v[11] += a.asked && (a.stroke >> 1) != a.watch;
            v[12] += r.amendment.recognised;
            auto &pair =
                twins[{k.arm, k.group, t.world, t.fixture, t.wallet, audible(t.request), audible(t.revision)}];
            Word side = t.weather;
            pair.repeated |= bool(pair.seen & (1u << side));
            pair.seen |= 1u << side;
            pair.done |= Word(actual) << side;
            pair.opening[side] = audible(a.said);
            pair.report[side] = audible(b.heard) + "|" + audible(r.second.heard);
            pair.moves[side] = audible(r.second.earlier) + "|" + audible(c.heard);
            trace << off + i << '\t' << k.arm << '\t' << k.group << '\t' << t.world << '\t' << t.fixture << '\t'
                  << t.weather << '\t' << t.width << '\t' << t.wallet << '\t' << t.spool << '\t' << r.before << '\t'
                  << r.goal << '\t' << r.care << '\t' << r.glyphs << '\t' << r.edges << '\t' << r.handles << '\t'
                  << r.labels << '\t' << r.buttons << '\t' << r.reader << '\t' << audible(a.said) << '\t'
                  << audible(b.earlier) << '\t' << audible(b.heard) << '\t' << audible(c.heard) << '\t' << b.stroke
                  << '\t' << b.watch << '\t' << b.seen << '\t' << b.local << '\t' << c.recognised << '\t' << c.steps
                  << '\t' << c.fitted << '\t' << b.spent + r.second.spent + c.spent << '\t' << c.food << '\t'
                  << r.after << '\t' << r.models << '\t' << a.nbytes << '\t' << audible(r.amendment.said) << '\t'
                  << audible(r.second.earlier) << '\t' << audible(r.second.heard) << '\t' << r.second.stroke << '\t'
                  << r.second.watch << '\t' << r.second.seen << '\t' << r.second.local << '\n';
            if (off + i < 2)
                std::cout << "studio_scene=" << off + i << "  scene=" << t.fixture << "  weather=" << t.weather
                          << "  parts=" << t.width << "  materials=" << r.glyphs << "  joints=" << r.edges
                          << "  grip=" << r.handles << "  start=" << r.before << "  final=" << r.after
                          << "  target=" << r.goal << "  first=" << a.stroke << "  watch=" << a.watch
                          << "  seen=" << b.seen << "  tail_strokes=" << c.steps << "  fitted=" << c.fitted
                          << "  paid=" << b.spent + c.spent + r.second.spent << "  answered=" << actual << '\n';
            if (off + i < 4)
                std::cout << "practice_message=" << off + i << "  probe=" << a.stroke << "  observed=" << b.seen
                          << "  finished=" << actual << "  program=" << audible(c.heard) << '\n';
        }
    }
    trace.close();
    if (!trace)
        throw std::runtime_error("incomplete studio transcript");
    for (auto &[k, v] : marks)
        std::cout << "practice_dialogue=" << k.first << "  also=" << k.second << "  cases=" << v[0]
                  << "  possible=" << v[1] << "  reached=" << v[2] << "  asked=" << v[3] << "  tested=" << v[4]
                  << "  replies=" << v[5] << "  with_report=" << v[6] << "  spent=" << v[7] << "  earned=" << v[8]
                  << "  fitted=" << v[9] << "  understood=" << v[10] << "  indirect=" << v[11]
                  << "  revised=" << v[12] << '\n';
    std::map<std::pair<std::string, std::string>, std::array<Count, 4>> paired;
    for (auto &[key, t] : twins)
        if (t.seen == 3 && !t.repeated) {
            auto &v = paired[{std::get<0>(key), std::get<1>(key)}];
            ++v[0];
            v[1] += t.done == 3;
            v[2] += t.opening[0] == t.opening[1];
            v[3] += t.done == 3 && t.report[0] != t.report[1] && t.moves[0] != t.moves[1];
        }
    for (auto &[k, v] : paired)
        std::cout << "practice_pair=" << k.first << "  group=" << k.second << "  pairs=" << v[0] << "  both=" << v[1]
                  << "  same_opening=" << v[2] << "  differing_answers=" << v[3] << '\n';
    double time = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "practice_conversations=" << n << "  seconds=" << time << "  conversations_per_s=" << n / time
              << '\n';
}
bool studio_command(const std::string &token, std::istream &input, const std::string &line) {
    if (token != "/studio" && !at_studio)
        return false;
    if (token == "/rack" || token == "/wheel" || token == "/marsh" || token == "/ford" || token == "/reed" ||
        token == "/leave-studio") {
        at_studio = false;
        return token == "/leave-studio";
    }
    if (!studio_brain.ptr)
        throw std::invalid_argument("the studio requires private causal contexts and a learned acoustic organ");
    if (token == "/studio-trials") {
        studio_trials(input);
        return true;
    }
    Word command = 0, p = 0, q = 4, hidden = 0, senses = 0;
    std::string a, b, c, extra;
    effort::Speech phrase{};
    if (token == "/studio" || token == "/studio-layout") {
        if (!(input >> a))
            throw std::invalid_argument("use /studio WORLD [PIECES] or /studio-layout FIXTURE WEATHER [PIECES]");
        p = parse(a, ~0u);
        command = token == "/studio" ? practice::enter : practice::layout;
        if (command == practice::layout) {
            if (!(input >> b))
                throw std::invalid_argument("supply a physical weather variant");
            hidden = parse(b, 1);
        }
        if (input >> c)
            q = parse(c, 4);
        if (!q)
            throw std::invalid_argument("the studio needs at least one part");
        at_studio = open = true;
        at_rack = at_ford = at_marsh = at_wheel = false;
    } else if (token == "/studio-lock") {
        std::string stock;
        if (!(input >> a >> b >> c >> stock))
            throw std::invalid_argument("use /studio-lock MARKS MASK WORK LOOSE_LOOP");
        p = parse(a, 255);
        q = parse(b, 255);
        hidden = parse(c, 16) | (parse(stock, 1) << 8);
        command = practice::lock_escrow;
    } else if (token == "/studio-rain")
        command = practice::water;
    else if (token == "/studio-study") {
        if (!(input >> a))
            throw std::invalid_argument("specify a number of practice passes");
        p = parse(a, 7);
        command = practice::tuition;
    } else if (token == "/studio-order")
        command = practice::current_order;
    else if (token == "/studio-heard")
        command = practice::last_report;
    else if (token == "/studio-pass") {
        command = practice::pass;
        if (input >> a) {
            if (a == "mute")
                command = practice::cut;
            else if (a == "wrong")
                command = practice::bad_report;
            else if (a == "muffle")
                command = practice::mute_query;
            else if (a == "garble")
                command = practice::alter_request;
            else
                throw std::invalid_argument("unknown acoustic intervention");
        }
    } else if (!token.empty() && token[0] != '/') {
        command = practice::utter;
        std::istringstream speech(line);
        phrase = plan_words(speech);
        if (auto *record = journal::current())
            record->line("studio_utterance=heard  text=" + line);
    } else
        return false;
    if (command != practice::utter && input >> extra)
        throw std::invalid_argument("unexpected studio argument");
    studio_tongues(senses);
    bool worker = command >= practice::pass && command <= practice::mute_query;
    auto &body = (worker ? partner() : local).resident.animal;
    Word purse = body.energy, organs = worker ? senses >> 3 : senses & 7;
    void *args[]{&studio.ptr, &studio_brain.ptr, &rack_voices.ptr,  &command, &p, &q, &hidden, &phrase,
                 &purse,      &organs,           &studio_output.ptr};
    s.d.check(s.d.launch(studio_act, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "perform a public, observation-contingent exchange");
    practice::Bench room{};
    practice::Exchange out{};
    s.d.check(s.d.download(&room, studio.ptr, sizeof(room)), "audit paid physical choices");
    s.d.check(s.d.download(&out, studio_output.ptr, sizeof(out)), "read acoustic replies");
    s.d.check(s.d.download(&local.training, studio_brain.ptr, sizeof(local.training)), "retain material hypotheses");
    if (room.physical.tutor + room.physical.demonstrated != 1024 ||
        room.physical.stock + room.physical.swallowed != 256 || room.used > room.allowance)
        throw std::logic_error("unbalanced conditional work");
    if (command == practice::lock_escrow && !out.recognised)
        throw std::invalid_argument("set the food marks on an unworked fixture using paired colour bits");
    if (worker) {
        if (out.spent > purse || purse - out.spent + out.food > 65535)
            throw std::logic_error("unbalanced worker reserve");
        body.energy = purse - out.spent + out.food;
        body.meals += out.food;
        body.diet[1] += out.food;
    }
    if (worker && out.heard.n && out.stroke < 8)
        std::cout << "worker> " << audible(out.heard) << '\n';
    if (command == practice::utter || command == practice::current_order || command == practice::last_report ||
        (worker && out.stroke < 8))
        std::cout << (command == practice::current_order ? "work order> " : "animal> ")
                  << (out.said.n ? audible(out.said) : "?") << '\n';
    std::cout << "studio_contact=" << command << "  scene=" << room.physical.fixture << "  variant=" << room.climate
              << "  initial=" << room.public_view.view << "  target=" << room.public_view.goal.faces
              << "  physical=" << out.after << "  asked=" << out.asked << "  probe=" << out.stroke
              << "  observe=" << out.watch << "  saw=" << out.seen << "  understood=" << out.recognised
              << "  reached=" << out.arrived << "  cost=" << (worker ? out.spent : 0) << "  credit=" << out.food
              << "  fitted=" << out.fitted << "  strokes=" << out.steps
              << "  remaining=" << room.allowance - room.used << "  contexts=" << practice::size(local.training.mind)
              << "  private_samples=" << local.training.mind.samples << "  tutor_work=" << room.physical.demonstrated
              << "  stage=" << room.stage << "  policy_bytes=" << room.explanation.size << '\n';
    return true;
}
