// Included inside SiphonInterview to share the resident, acoustic word organ
// and CUDA context with ordinary conversations.
bool wheel_command(const std::string &token, std::istream &input, const std::string &line) {
    if (token != "/wheel" && !at_wheel)
        return false;
    if (!assemblies.ptr) {
        if (token == "/wheel")
            throw std::invalid_argument("a pressure table needs a resident with clay tissue");
        return false;
    }
    if (token == "/wheel-tests") {
        wheel_trials(input);
        return true;
    }
    Word command = 0, param = 0, other = 0;
    std::string first, second, extra;
    if (token == "/wheel") {
        if (!(input >> first) || input >> extra)
            throw std::invalid_argument("use /wheel SEED");
        param = parse(first, ~0u);
        at_wheel = true;
        open = true;
        holding = false;
        offers = 15;
        command = press::set_wheel;
    } else if (token == "/wheel-watch" || token == "/wheel-taste") {
        if (input >> extra)
            throw std::invalid_argument("watching and tasting take no arguments");
        command = token == "/wheel-watch" ? press::watch : press::sample_water;
    } else if (token == "/wheel-turn" || token == "/wheel-offer" || token == "/wheel-press") {
        if (!(input >> first) || input >> extra)
            throw std::invalid_argument("give one wheel setting");
        param = parse(first, token == "/wheel-offer" ? 15 : 3);
        if (token == "/wheel-offer") {
            offers = param;
            std::cout << "press_offered=" << offers << std::endl;
            return true;
        }
        command = token == "/wheel-turn" ? press::turn_wheel : press::work;
    } else if (token == "/wheel-show") {
        if (!(input >> first >> second) || input >> extra)
            throw std::invalid_argument("use /wheel-show BATH TOOL");
        param = parse(first, 3);
        other = parse(second, 3);
        command = press::demonstrate;
    } else if (token == "/leave-wheel" || token == "/reed" || token == "/leave-reed") {
        at_wheel = false;
        open = false;
        return token == "/leave-wheel" || token == "/leave-reed";
    } else if (token.empty() || token.front() == '/')
        return false;
    else {
        std::istringstream sounds(line);
        auto phrase = utterance(sounds);
        auto law = local.referent.genes;
        void *args[]{&assemblies.ptr, &minds.ptr, &words.ptr, &law, &phrase, &offers, &subject.ptr, &response.ptr};
        s.d.check(s.d.launch(press_say, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                  "retrieve a tool or its expected effect");
        siphon::Report r{};
        s.d.check(s.d.download(&r, response.ptr, sizeof(r)), "hear the tool prediction");
        std::string answer;
        if (r.accepted)
            for (Word i = 0; i < r.said.n; ++i) {
                if (i)
                    answer += ' ';
                answer += acoustic::spell(r.said.wave[i]);
            }
        if (auto *j = journal::current())
            j->line("press_prompt=heard  text=" + line);
        std::cout << "animal> " << (answer.empty() ? "?" : answer) << "\npress_reply=" << r.accepted
                  << "  recognized=" << r.recognized << "  role=" << r.role << "  candidates=" << r.candidates
                  << "  answer=" << r.spoken << "  contacts=0" << std::endl;
        return true;
    }
    auto &a = partner().resident.animal;
    auto &b = local.resident.animal;
    Word ea = a.energy, eb = b.energy, body = organs() | (Word(a.body.limbs != 0) << 5), learn = plastic;
    void *args[]{&wheel.ptr, &assemblies.ptr, &table.ptr, &subject.ptr, &command, &param, &other, &offers, &ea,
                 &eb,        &body,           &learn,     &pressure.ptr};
    s.d.check(s.d.launch(press_move, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr), "operate the pressure wheel");
    press::Receipt r{};
    s.d.check(s.d.download(&r, pressure.ptr, sizeof(r)), "measure the tools and finite reservoir");
    if (r.reserve + r.spilled + r.consumed != 2048)
        throw std::logic_error("pressure wheel carbon imbalance");
    if (r.others > ea || r.own > eb + r.meal)
        throw std::logic_error("pressure work exceeds reserves");
    a.energy = static_cast<decltype(a.energy)>(ea - r.others + r.share);
    b.energy = static_cast<decltype(b.energy)>(eb + r.meal - r.own);
    a.meals += r.share;
    b.meals += r.meal;
    a.diet[1] += r.share;
    b.diet[1] += r.meal;
    s.d.check(s.d.download(&local.ceramic.mind, assemblies.ptr, sizeof(clay::Mind)), "retain pressure contacts");
    std::cout << "press_contact=" << command << "  shell=" << r.cue << "  bath=" << r.bath << "  tone=" << r.tone
              << "  witnessed=" << r.witnesses << "  blind=" << r.tastes << "  work=" << r.own + r.others
              << "  meals=" << r.meal + r.share << "  stock=" << r.reserve << "  loose=" << r.spilled
              << "  consumed=" << r.consumed << std::endl;
    return true;
}
