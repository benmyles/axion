bool story_command(const std::string &token, std::istringstream &input, const std::string &line) {
    bool ordinary = !token.empty() && token.front() != '/';
    if (ordinary ? !local.describes : !token.starts_with("/story-"))
        return false;
    if (!local.riverine || !partner().riverine || !local.binds_floats || !partner().binds_floats) {
        if (ordinary)
            return false;
        throw std::invalid_argument("the brook needs two embodied waterside residents");
    }
    utterance::Request q{};
    std::string arg;
    auto take = [&]() {
        if (!(input >> arg))
            throw std::invalid_argument("missing story argument");
        return arg;
    };
    auto who = [&]() {
        auto person = take();
        if (person != "speaker" && person != "pupil")
            throw std::invalid_argument("address the speaker or pupil");
        return Word(person == "pupil");
    };
    auto board = [&]() { return float_paint(take()); };
    if (token == "/story-bed") {
        q.action = utterance::open;
        q.bed.palette = board();
        q.bed.ducts = board();
        q.bed.fibres = parse(take(), 15);
        if (input >> arg) {
            q.bed.ground = float_paint(arg);
            q.bed.bright = 0;
        }
    } else if (token == "/story-pour") {
        q.action = utterance::pour;
        q.source = parse(take(), 3);
    } else if (token == "/story-memory") {
        q.action = utterance::inspect;
        q.who = who();
    } else if (token == "/story-clear") {
        q.action = utterance::clear;
        q.who = who();
    } else if (token == "/story-net" || token == "/story-serve") {
        q.action = token == "/story-net" ? utterance::cast_net : utterance::serve;
        q.who = who();
    } else if (token == "/story-curtain") {
        q.action = utterance::curtain;
        q.who = who();
        auto state = take();
        if (state != "open" && state != "closed")
            throw std::invalid_argument("open or close the observer's curtain");
        q.source = state == "open";
    } else if (token == "/story-caption" || token == "/story-hear") {
        q.action = token == "/story-caption" ? utterance::pair_speech : utterance::receive;
        q.who = who();
        if (!std::getline(input, arg, '|') || input.eof())
            throw std::invalid_argument("supply a question, a vertical bar and the public words");
        q.prompt = utterance::read_line(arg);
        std::getline(input, arg);
        q.statement = utterance::read_line(arg);
        if (!q.statement.n)
            throw std::invalid_argument("the statement is empty");
    } else if (ordinary || token == "/story-ask" || token == "/story-pass") {
        q.action = token == "/story-pass" ? utterance::recount : utterance::respond;
        if (!ordinary)
            q.who = who();
        try {
            if (ordinary)
                q.prompt = utterance::read_line(line);
            else {
                std::getline(input, arg);
                q.prompt = utterance::read_line(arg);
            }
        } catch (const std::invalid_argument &) {
            if (ordinary)
                return false;
            throw;
        }
    } else
        return false;
    if ((q.action == utterance::respond || q.action == utterance::recount || q.action == utterance::receive ||
         q.action == utterance::pair_speech) &&
        (!q.prompt.n || q.prompt.n > utterance::phases)) {
        if (ordinary)
            return false;
        throw std::invalid_argument("a learned cue uses one to eight sounds");
    }
    if (!ordinary && input >> arg)
        throw std::invalid_argument("unexpected story argument");
    if (!story_table) {
        story_table = std::make_unique<Temporary>(s.d, sizeof(utterance::Workbench));
        story_brains = std::make_unique<Temporary>(s.d, 2 * sizeof(utterance::Saved));
        story_bodies = std::make_unique<Temporary>(s.d, 2 * sizeof(utterance::Senses));
        story_reply = std::make_unique<Temporary>(s.d, sizeof(utterance::Reply));
        utterance::Workbench initial{};
        s.d.check(s.d.upload(story_table->ptr, &initial, sizeof(initial)), "open the visual record");
        s.bind(story_action, "narrative_table", Image::runnels);
    }
    std::array<utterance::Senses, 2> bodies{};

    for (Word i = 0; i < 2; ++i) {
        auto &a = float_actor(i);
        auto &t = a.brain.tissue[utterance::band_of(a)];
        if (q.action == utterance::pair_speech && i == q.who && (!i || !visitor)) {
            auto ptr = somatic_bands[i] + Count(utterance::band_of(a)) * sizeof(Tissue);
            s.d.check(s.d.download(&t, ptr, sizeof(t)), "read the conducting auditory tissue");
        }
        bodies[i] = utterance::senses(a, t);
        if (!story_ready || std::memcmp(&story_cache[i], &a.accounts, sizeof(a.accounts))) {
            s.d.check(s.d.upload(story_brains->ptr + i * sizeof(a.accounts), &a.accounts, sizeof(a.accounts)),
                      "connect the resident's acoustic tissue");
            story_cache[i] = a.accounts;
        }
    }
    auto &speaker = float_actor(q.who).resident.animal, &listener = float_actor(1 - q.who).resident.animal;
    q.energy = speaker.energy;
    q.other_energy = listener.energy;
    story_ready = true;
    s.d.check(s.d.upload(story_bodies->ptr, bodies.data(), sizeof(bodies)), "connect both eyes, ears and voices");
    void *args[]{&story_table->ptr, &story_brains->ptr, &story_bodies->ptr, &q, &story_reply->ptr};
    s.d.check(s.d.launch(story_action, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "hear, describe or release the held grains");
    utterance::Reply r{};
    s.d.check(s.d.download(&r, story_reply->ptr, sizeof(r)), "read the spoken words and resource balance");
    if (r.status == 2 && ordinary)
        return false;
    if (r.status && r.status != 2) {
        story_ready = false;
        throw std::invalid_argument("this passage, energy reserve or lesson is unavailable");
    }
    if (r.stock + r.held + r.settled + r.eaten != 64 || r.teacher_heat > q.energy || r.pupil_heat > q.other_energy)
        throw std::logic_error("the brook lost food or exceeded a resident's energy");
    if (q.action == utterance::pair_speech) {
        auto &c = story_cache[q.who];
        s.d.check(s.d.download(&c.memory, story_brains->ptr + q.who * sizeof(c), sizeof(c.memory)),
                  "retain newly acquired speech projections");
        float_actor(q.who).accounts.memory = c.memory;
    }
    for (Word i = 0; i < 2; ++i) {
        auto &a = float_actor(i);
        a.accounts.trail = i == q.who ? r.seen : r.other;
        story_cache[i].trail = a.accounts.trail;
        a.describes |= a.accounts.memory.script[0].prompt || a.accounts.trail.size();
    }
    if (q.action == utterance::pair_speech) {
        auto &a = float_actor(q.who);
        Word band = utterance::band_of(a);
        a.brain.tissue[band].spike = r.initial;
        if (!q.who || !visitor)
            s.d.check(s.d.upload(somatic_bands[q.who] + Count(band) * sizeof(Tissue) + offsetof(Tissue, spike),
                                 &r.initial, sizeof(Word)),
                      "retain the learner's spikes");
    }
    speaker.energy = speaker.energy - r.teacher_heat + r.bite;
    speaker.worked += r.teacher_heat;
    speaker.meals += r.bite;
    speaker.diet[1] += r.bite;
    listener.energy -= r.pupil_heat;
    listener.worked += r.pupil_heat;
    if (q.action == utterance::respond || q.action == utterance::recount)
        std::cout << (q.who ? "pupil> " : "speaker> ") << (r.reply.n ? utterance::render(r.reply) : "?") << '\n';
    std::cout << "narrative_turn=" << q.action << "  person=" << q.who << "  events=" << r.seen.size()
              << "  trail=" << r.seen.bits << "  other_trail=" << r.other.bits << "  decoded=" << r.decoded.bits
              << "  words=" << r.reply.n << "  prompts=" << r.prompts_known << "  heat=" << r.teacher_heat
              << "  other_heat=" << r.pupil_heat << "  edits=" << r.edits << "  sprouts=" << r.sprouts
              << "  palette=" << r.palette << "  grain_stock=" << r.grains << "  food_stock=" << r.stock
              << "  held=" << r.held << "  eaten=" << r.eaten << "  settled=" << r.settled << "  bite=" << r.bite
              << "  shutters=" << r.shutters << '\n';
    if (q.action == utterance::pour)
        std::cout << "narrative_glint=" << r.event.physical << "  from=" << r.event.from << "  to=" << r.event.to
                  << "  visible=" << r.event.view << "  before=" << r.event.before << "  after=" << r.event.after
                  << "  light=" << r.event.light << '\n';
    return true;
}
