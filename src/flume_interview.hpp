bool flume_command(const std::string &token, std::istringstream &input, const std::string &line) {
    if (token == "/flume-trials") {
        flume_trials(input);
        return true;
    }
    if (token == "/flume-exit") {
        at_flume = false;
        return true;
    }
    Word cmd = runnel::view_flume, a = 0, b = 0, n = 0, senses = 0;
    std::string name, extra;
    effort::Speech phrase{};
    auto uint_arg = [&](Word lo, Word hi) {
        std::string s;
        if (!(input >> s))
            throw std::invalid_argument("missing flume quantity");
        size_t used = 0;
        auto x = std::stoull(s, &used);
        if (used != s.size() || x < lo || x > hi)
            throw std::invalid_argument("flume quantity outside range");
        return Word(x);
    };
    auto person = [&]() {
        std::string p;
        if (!(input >> p) || (p != "speaker" && p != "partner"))
            throw std::invalid_argument("use speaker or partner");
        return Word(p == "partner");
    };
    auto treatment = [&]() {
        std::string t;
        if (!(input >> t))
            return 0u;
        if (t == "mute")
            return 1u;
        if (t == "wrong")
            return 2u;
        if (t == "garble")
            return 3u;
        throw std::invalid_argument("unknown flume channel");
    };
    if (token == "/flume") {
        if (!reed_pair.ptr)
            throw std::invalid_argument(
                "open the flume with a watershed resident carrying fold contacts and acoustic organs");
        a = uint_arg(0, ~0u);
        n = (input >> std::ws).eof() ? 12 : uint_arg(2, runnel::max_span);
        if (input >> name) {
            if (name != "handed")
                throw std::invalid_argument("use handed for chiral grain");
            b = 1;
        }
        cmd = runnel::open_flume;
        at_flume = true;
        at_studio = false;
        at_rack = false;
    } else if (!at_flume)
        return false;
    else if (token == "/flume-layout") {
        a = uint_arg(0, ~0u);
        n = uint_arg(2, runnel::max_span);
        cmd = runnel::layout_flume;
    } else if (token == "/flume-drill") {
        a = person();
        if (!(input >> name) || (name != "even" && name != "odd" && name != "all"))
            throw std::invalid_argument("choose an even, odd or complete mineral bank");
        b = name == "even" ? 0 : name == "odd" ? 1 : 2;
        n = uint_arg(1, 8);
        cmd = runnel::school_flume;
    } else if (token == "/flume-sample") {
        a = person();
        Word l = uint_arg(0, 3), r = uint_arg(0, 3);
        b = runnel::texture(l, r);
        cmd = runnel::sample_flume;
    } else if (token == "/flume-clear") {
        a = person();
        cmd = runnel::clear_flume;
    } else if (token == "/flume-order")
        cmd = runnel::order_flume;
    else if (token == "/flume-pass") {
        cmd = runnel::pass_flume;
        a = treatment();
    } else if (token == "/flume-build") {
        cmd = runnel::build_flume;
        a = treatment();
    } else if (token == "/flume-pump") {
        n = uint_arg(1, 65536);
        a = 1;
        if (input >> name) {
            if (name != "quiet")
                throw std::invalid_argument("use quiet to remove the tide");
            a = 0;
        }
        cmd = runnel::start_pump;
    } else if (token == "/flume-view")
        cmd = runnel::view_flume;
    else if (!token.empty() && token[0] != '/') {
        std::istringstream stream(line);
        phrase = plan_words(stream);
        cmd = runnel::speak_flume;
    } else {
        if (token == "/studio" || token == "/rack")
            at_flume = false;
        return false;
    }
    if (cmd != runnel::speak_flume && input >> extra)
        throw std::invalid_argument("unexpected flume argument");
    std::array<runnel::Mind, 2> m{local.reed.mind, partner().reed.mind};
    std::array<linkage::Tongue, 2> v{};
    for (Word i = 0; i < 2; ++i) {
        auto &z = i ? partner() : local;
        v[i] = {z.native_words, z.path_speech.words, z.calls.animal, z.referent.genes};
        auto body = z.resident.animal.body;
        senses |= (Word(!!body.eyes) | (Word(!!body.limbs) << 1) | (Word(!!body.pores) << 2)) << (3 * i);
    }
    s.d.check(s.d.upload(reed_pair.ptr, m.data(), sizeof(m)), "present the two residents' private fold circuits");
    s.d.check(s.d.upload(flume_voices.ptr, v.data(), sizeof(v)), "present the independent acoustic maps");
    Word purse = partner().resident.animal.energy;
    void *args[]{&flume.ptr, &reed_pair.ptr, &flume_voices.ptr, &cmd, &a, &b, &n, &phrase,
                 &purse,     &senses,        &flume_reply.ptr};
    s.d.check(s.d.launch(flume_act, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "assemble a shared pump by public consultation");
    runnel::Flume room{};
    runnel::Dispatch out{};
    s.d.check(s.d.download(&room, flume.ptr, sizeof(room)), "inspect the flume");
    s.d.check(s.d.download(&out, flume_reply.ptr, sizeof(out)), "read public work");
    s.d.check(s.d.download(m.data(), reed_pair.ptr, sizeof(m)), "retain each resident's own learning");
    local.reed.mind = m[0];
    if (partner().folds_reeds)
        partner().reed.mind = m[1];
    if (out.paid > purse || purse - out.paid + out.meal > 65535 || out.stock_water != 4096 + 5 * (room.span - 1) ||
        out.stock_carbon + room.swallowed != 4096 || room.tutor + room.tuition != 1024 ||
        room.outlet_in - room.outlet_back != out.food + room.swallowed)
        throw std::logic_error("flume mass or bodily work did not balance");
    auto &worker = partner().resident.animal;
    worker.energy = purse - out.paid + out.meal;
    worker.worked += out.paid;
    worker.meals += out.meal;
    worker.diet[1] += out.meal;
    if (cmd == runnel::pass_flume && out.installed)
        std::cout << "worker fits> " << audible(out.input) << '\n';
    else if (cmd == runnel::speak_flume || cmd == runnel::pass_flume || cmd == runnel::order_flume)
        std::cout << (cmd == runnel::order_flume ? "work order> "
                      : out.reported             ? "worker> "
                                                 : "animal> ")
                  << (out.reply.n ? audible(out.reply) : "?") << '\n';
    std::cout << "flume_contact=" << cmd << "  layout=" << room.layout << "  stone=" << room.stone
              << "  world=" << room.world << "  span=" << room.span << "  cursor=" << room.cursor
              << "  stage=" << room.stage << "  understood=" << out.understood << "  referrals=" << out.asked
              << "  answered=" << out.reported << "  accepted=" << out.learned << "  paid=" << out.paid
              << "  food=" << out.meal << "  pupil_contacts=" << runnel::population(m[0].cells.touched)
              << "  social_contacts=" << runnel::population(m[0].cells.witnessed) << "  question_cue=" << room.question
              << "  requested_cue=" << room.cue << "  rephrased=" << room.retry
              << "  teacher_contacts=" << runnel::population(m[1].cells.touched) << "  installed=" << room.installed
              << "  skipped=" << room.abstained << "  water=" << out.water
              << "  delivered=" << out.food + room.swallowed << "  tuition=" << room.tuition
              << "  source_carbon=" << out.stock_carbon + room.swallowed << "  related_pair=" << out.example
              << "  rotation=" << out.transform << "  pose=" << out.report_pose << "  prediction=" << out.report_effect
              << "  cached=" << out.cached << "  clock=" << room.clock << '\n';
    if (cmd == runnel::pass_flume || cmd == runnel::speak_flume || cmd == runnel::order_flume)
        std::cout << "flume_words=" << cmd << "  heard=" << std::quoted(audible(out.input))
                  << "  said=" << std::quoted(audible(out.reply)) << '\n';
    if (cmd == runnel::build_flume || cmd == runnel::view_flume || cmd == runnel::start_pump) {
        std::cout << "flume_works=" << room.span << "  pin=" << room.layout << "  shapes=";
        for (Word i = 0; i < room.span; ++i)
            std::cout << char('0' + room.fold[i]);
        std::cout << '\n';
    }
    return true;
}
