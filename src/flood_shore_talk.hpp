// A live table shares the animal's acquired contacts and body reserves with the floodplain.
bool shore_control(const std::string &line) {
    std::istringstream words(line);
    std::string cmd;
    words >> cmd;
    const std::array<std::string_view, 13> commands{
        "/shore",         "/explore-shore", "/taste-shore", "/change-deposit", "/imagine-shore",
        "/build-shore",   "/light-shore",   "/eat-shore",   "/build-for",      "/forage-shore",
        "/inspect-shore", "/expose-shore",  "/pursue-shore"};
    if (shore_open && line.starts_with("/build-answer ")) {
        std::string answer;
        if (!eye || !visual.hear(line.substr(14), answer) || answer.empty() || answer.front() == '?' ||
            answer.ends_with('?')) {
            std::cout << "shore_request=unresolved" << std::endl;
            return true;
        }
        std::cout << "animal> " << answer << std::endl;
        if (answer.find_first_of(" \t\n")!=std::string::npos) {
            std::cout<<"shore_request=unresolved"<<std::endl;return true;
        }
        return shore_control("/build-for " + answer);
    }
    auto found = std::find(commands.begin(), commands.end(), cmd);
    if (found == commands.end())
        return false;
    Word action = found - commands.begin(), value = 0, amount = 1;
    std::vector<std::string> args;
    for (std::string v; words >> v;)
        args.push_back(v);
    Count heard = 0;
    if (action == 0) {
        if (args.empty() || args.size() > 2)
            throw std::invalid_argument("use /shore SEED [SEPARATION]");
        value = quantity(args[0], 0, ~0u);
        amount = args.size() == 2 ? quantity(args[1], 3, 5) : 3;
        if (!shore_open) {
            material::ShoreTray initial{};
            initial.genes = subject.worker.genes;
            initial.memory = subject.forecast;
            initial.hand = subject.worker.hand;
            s.d.check(s.d.upload(shore.ptr, &initial, sizeof(initial)), "seat the owner of the predictive circuit");
            s.bind(shore_visit, "flood_shore_visit", Image::floodplain);
            shore_open = true;
        }
        field_open = false;
        open = false;
        spoken_work = false;
        subject.projects = true;
    } else {
        if (!shore_open) {
            std::cout << "Open a loose bed with /shore SEED." << std::endl;
            return true;
        }
        if (action == 1 || action == 2 || action == 5 || action == 6 || action == 11) {
            if (args.size() != 1)
                throw std::invalid_argument("give the number of visits or moves");
            amount = quantity(args[0], 1, action == 5 ? 32 : 2048);
        } else if (action == 8) {
            if (args.size() != 1 || !eye)
                throw std::invalid_argument("use /build-for with one learned visual name");
            heard = acoustic::pulse(args[0]);
        } else if (!args.empty())
            throw std::invalid_argument("this shore gesture takes no arguments");
    }
    Word reserve = subject.resident.animal.energy;
    void *params[]{&shore.ptr, &shore_report.ptr, &eye, &heard, &action, &value, &amount, &reserve};
    s.d.check(s.d.launch(shore_visit, 1, 1, 1, 32, 1, 1, 0, nullptr, params, nullptr),
              "visit the inhabited shore table");
    material::ShoreReply r{};
    material::ShoreTray state{};
    s.d.check(s.d.download(&r, shore_report.ptr, sizeof(r)), "read the shore exposure");
    s.d.check(s.d.download(&state, shore.ptr, sizeof(state)), "retain acquired spatial and hand contacts");
    subject.forecast = state.memory;
    subject.worker.hand = state.hand;
    subject.resident.animal.energy = static_cast<unsigned short>(reserve - r.cost + r.collected);
    if (eye && action == 0) {
        grounding::View scene{};
        scene.before = shore_prior;
        grounding::illuminate(scene.after, r.first, r.pigment[0]);
        grounding::illuminate(scene.after, r.second, r.pigment[1]);
        if (action == 0)
            scene.before = scene.after;
        visual.present(scene);
        shore_prior = scene.after;
    }
    std::cout << "shore_event=" << cmd.substr(1) << "  region_a=" << r.first << "  pigment_a=" << r.pigment[0]
              << "  region_b=" << r.second << "  pigment_b=" << r.pigment[1] << "  lamp=" << r.sun
              << "  projected_sites=" << r.predicts << "  wanted_sites=" << r.targets << "  acted=" << r.acted
              << "  work=" << r.cost << "  food_a=" << r.food[0] << "  food_b=" << r.food[1] << "  reactions=" << r.made
              << "  light=" << r.absorbed << "  eaten=" << r.collected
              << "  reserve=" << subject.resident.animal.energy;
    if (action == 8)
        named_shore = r.found == r.certain && std::has_single_bit(r.found);
    if (action == 11)
        std::cout << "  detached_a=" << r.response[0] << "  detached_b=" << r.response[1];
    if (action == 4)
        std::cout << "  from=" << (r.choice & 31) << "  to=" << ((r.choice >> 5) & 31)
                  << "  motion=" << (r.choice >> 10);
    if (action == 8)
        std::cout << "  possible=" << r.found << "  certain=" << r.certain
                  << "  resolved=" << (r.found == r.certain && std::has_single_bit(r.found));
    std::cout << std::endl;
    if (action == 10) {
        std::cout << "shore_tiles=";
        for (Word i = 0; i < 32; ++i)
            std::cout << (i ? "," : "") << state.sites[i];
        std::cout << "  held=" << r.cargo << "  raw_a=" << r.raw[0] << "  raw_b=" << r.raw[1] << std::endl;
    }
    if (auto *j = journal::current()) {
        auto emit = [&](const char *name) {
            std::ostringstream row;
            row << name << "=";
            for (Word i = 0; i < 32; ++i)
                row << (i ? "," : "") << state.sites[i];
            j->line(row.str());
        };
        if (action == 0)
            emit("shore_before");
        emit("shore_after");
        j->line("shore_layout=" + std::to_string(state.seed) + "  light=" + std::to_string(r.sun) +
                "  patch_a=" + std::to_string(r.first) + "  patch_b=" + std::to_string(r.second) +
                "  diet=" + std::to_string(state.coast.responsive));
        ++j->metrics["shore_visits"];
        j->metrics["shore_actions"] += r.acted;
        j->metrics["shore_reactions"] += r.made;
        j->metrics["shore_predicted_sites"] = r.predicts;
        if (action == 8 && r.found == r.certain && std::has_single_bit(r.found))
            ++j->metrics["shore_referents"];
    }
    return true;
}
