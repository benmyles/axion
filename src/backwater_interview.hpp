static bool can_sound_bank(const AgentImage &image) {
    const auto &a = image.resident.animal;
    return image.riverine && a.energy && a.body.pores && a.body.limbs;
}
void bank_trials(std::istringstream &input) {
    std::string a, b, c, extra;
    if (!(input >> a >> b >> c))
        throw std::invalid_argument("use /backwater-trials CASES EXPOSURE SEED [crossed]");
    Word cases = parse(a, 8192), exposure = parse(b, 512), seed = parse(c, ~0u), across = 0;
    if (input >> extra) {
        if (extra != "crossed" || input >> extra)
            throw std::invalid_argument("unexpected bank trial argument");
        across = 1;
    }
    if (!bank_brain.ptr || !can_sound_bank(local) || !cases)
        throw std::invalid_argument("use a bank listener and at least one paired trial");
    std::array<linkage::Tongue, 2> voices{};
    for (Word i = 0; i < 2; ++i) {
        auto &z = i ? partner() : local;
        if (!can_sound_bank(z))
            continue;
        voices[i] = {z.native_words, z.path_speech.words, z.calls.animal, z.referent.genes};
    }
    s.d.check(s.d.upload(bank_brain.ptr, &local.rattle_organ.mind, sizeof(rattle::Mind)), "sample an intact speaker");
    s.d.check(s.d.upload(bank_voices.ptr, voices.data(), sizeof(voices)), "sample both listeners' learned names");
    Temporary results(s.d, Count(cases) * sizeof(rattle::Spoken));
    Function trials{};
    s.bind(trials, "bank_conversations", Image::siphons);
    std::vector<rattle::Spoken> samples(cases);
    const char *names[]{"heard", "mute", "reversed", "no_first_school", "frozen_after", "inverted_probe"};
    for (Word arm = 0; arm < 6; ++arm) {
        void *args[]{&bank_brain.ptr, &bank_voices.ptr, &cases, &exposure, &seed, &arm, &across, &results.ptr};
        s.d.check(s.d.launch(trials, (cases + 31) / 32, 1, 1, 32, 1, 1, 0, nullptr, args, nullptr),
                  "audit public questions at two banks");
        s.d.check(s.d.download(samples.data(), results.ptr, cases * sizeof(samples[0])), "read both public choices");
        Count both = 0, citations = 0, faithful = 0, supplied = 0, correct = 0, voice = 0, decoded = 0, paid = 0,
              food = 0, changed = 0, experienced = 0;
        for (auto &r : samples) {
            Word conflicts = r.targets[0] ^ r.targets[1];
            if (std::popcount(conflicts) != 4 || r.verified != 16)
                throw std::logic_error("spoken twin worlds diverged from the physical probe");
            both += std::popcount(conflicts & ~(r.choices[0] ^ r.targets[0]) & ~(r.choices[1] ^ r.targets[1]));
            citations += r.cited;
            faithful += r.causal;
            supplied += r.reports;
            correct += r.correct;
            decoded += r.decoded;
            voice += r.voiced;
            paid += r.paid;
            food += r.earned;
            changed += r.other_pan;
            experienced += r.experienced;
        }
        std::cout << "bank_dialogue=" << names[arm] << "  cases=" << cases << "  exposure=" << exposure
                  << "  crossed=" << across << "  probes=" << 16 * cases << "  correct=" << correct
                  << "  opposed=" << 4 * cases << "  both_motors=" << both << "  voiced=" << voice
                  << "  supplied=" << supplied << "  decoded=" << decoded << "  work=" << paid << "  food=" << food
                  << "  cited=" << citations << "  faithful=" << faithful << "  changed_without=" << changed
                  << "  experience=" << experienced << '\n';
    }
}
bool bank_command(const std::string &token, std::istringstream &input, const std::string &line) {
    if (token == "/backwater-trials") {
        bank_trials(input);
        return true;
    }
    if (token == "/backwater-exit") {
        at_bank = false;
        return true;
    }
    if (token != "/backwater" && !at_bank)
        return false;
    Word cmd = rattle::view, arg = 0, channel = 0;
    effort::Speech words{};
    std::string argument, extra;
    if (token == "/backwater") {
        if (!bank_brain.ptr || !can_sound_bank(local))
            throw std::invalid_argument(
                "the bank needs a living speaker with a listening organ, a limb and a feeding pore");
        if (!(input >> argument))
            throw std::invalid_argument("give the backwater a world number");
        arg = parse(argument, ~0u);
        if (input >> argument) {
            if (argument != "crossed")
                throw std::invalid_argument("use crossed for opposed gustatory contacts");
            channel = 1;
        }
        cmd = rattle::assemble;
        at_bank = true;
        at_flume = at_studio = at_rack = at_ford = at_marsh = at_wheel = false;
    } else if (token == "/backwater-divert")
        cmd = rattle::divert;
    else if (token == "/backwater-work" || token == "/backwater-pass") {
        cmd = token == "/backwater-work" ? rattle::practise : rattle::relay;
        if (cmd == rattle::practise) {
            if (!(input >> argument) || !(arg = parse(argument, 4096)))
                throw std::invalid_argument("choose 1 through 4096 encounters");
        }
        if (input >> argument) {
            if (argument != "mute" && argument != "reversed")
                throw std::invalid_argument("use mute or reversed for a channel control");
            channel = argument == "mute" ? 1 : 2;
        }
    } else if (token == "/backwater-pull")
        cmd = rattle::pull;
    else if (token == "/backwater-view")
        cmd = rattle::view;
    else if (!token.empty() && token[0] != '/') {
        cmd = rattle::listen_word;
        std::istringstream phrase(line);
        words = plan_words(phrase);
    } else {
        if (token == "/flume" || token == "/studio" || token == "/rack" || token == "/ford" || token == "/marsh" ||
            token == "/wheel" || token == "/reed")
            at_bank = false;
        return false;
    }
    if (cmd != rattle::listen_word && input >> extra)
        throw std::invalid_argument("unexpected backwater argument");
    std::array<linkage::Tongue, 2> voices{};
    for (Word i = 0; i < 2; ++i) {
        auto &z = i ? partner() : local;
        if (!can_sound_bank(z))
            continue;
        voices[i] = {z.native_words, z.path_speech.words, z.calls.animal, z.referent.genes};
    }
    auto m = local.rattle_organ.mind;
    if (cmd != rattle::assemble)
        m = rattle::rebase(m, 0, bank_clock);
    s.d.check(s.d.upload(bank_brain.ptr, &m, sizeof(m)), "present the speaker's retained bank contacts");
    s.d.check(s.d.upload(bank_voices.ptr, voices.data(), sizeof(voices)), "present both acquired acoustic maps");
    auto &a = local.resident.animal;
    Word purse = a.energy, can_learn = plastic;
    void *args[]{&bank_table.ptr, &bank_brain.ptr, &bank_voices.ptr, &cmd,       &arg,
                 &channel,        &words,          &purse,           &can_learn, &bank_reply.ptr};
    s.d.check(s.d.launch(bank_act, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "communicate across independent bank resonators");
    rattle::Exchange r{};
    rattle::Table b{};
    s.d.check(s.d.download(&r, bank_reply.ptr, sizeof(r)), "hear the public bank exchange");
    s.d.check(s.d.download(&b, bank_table.ptr, sizeof(b)), "inspect the physical pans");
    s.d.check(s.d.download(&m, bank_brain.ptr, sizeof(m)), "retain private listener contacts");
    bank_clock = b.tick;
    local.rattle_organ.mind = rattle::rebase(m, b.tick, 0);
    if (purse + r.food < r.paid || purse + r.food - r.paid > 65535 ||
        b.source + b.stock[0] + b.stock[1] + b.eaten != 8192)
        throw std::logic_error("bank purse or sediment imbalance");
    a.energy = purse + r.food - r.paid;
    a.worked += r.paid;
    a.meals += r.food;
    a.diet[1] += r.food;
    if (cmd == rattle::relay && r.report.n)
        std::cout << "worker> " << audible(r.report) << '\n';
    if (cmd == rattle::listen_word || cmd == rattle::relay)
        std::cout << "animal> " << (r.reply.n ? audible(r.reply) : "?") << '\n';
    std::cout << "bank_talk=" << cmd << "  world=" << b.seed << "  round=" << b.round << "  stage=" << b.stage
              << "  paid=" << r.paid << "  inquiries=" << r.inquiries << "  upstream=" << r.upstream
              << "  reports=" << r.reports << "  pulls=" << r.pulls << "  changed=" << r.changed << "  food=" << r.food
              << "  consumed=" << b.eaten << "  source=" << b.source << "  left=" << b.stock[0]
              << "  right=" << b.stock[1] << "  writes=" << r.writes << "  turns=" << r.switches
              << "  request=" << b.asked << "  waveform=" << b.answer << "  choice=" << b.decision
              << "  cited=" << r.grounded << "  understood=" << r.understood << "  reserve=" << a.energy
              << "  diverted=" << b.diverted << '\n';
    return true;
}
