bool audience_command(const std::string &token, std::istringstream &input, const std::string &line) {
    bool ordinary = !token.empty() && token.front() != '/';
    if (ordinary ? !audience_open : token != "/audience" && !token.starts_with("/audience-"))
        return false;
    if (token == "/audience-exit") {
        audience_open = false;
        return true;
    }
    auto &listener = partner();
    if (!local.binds_floats || !listener.binds_floats || !float_memories.ptr)
        throw std::invalid_argument("two embodied waterside residents are required");
    audience::Request q{};
    Word lessons = 0, instruction_seed = 0, pairing = 0;
    std::string word;
    auto take = [&]() {
        if (!(input >> word))
            throw std::invalid_argument("missing audience argument");
        return word;
    };
    auto position = [&]() {
        auto name = take();
        if (name != "first" && name != "second")
            throw std::invalid_argument("choose the first or second station");
        return Word(name == "second");
    };
    std::unique_ptr<Temporary> specimen;
    if (token == "/audience-practice") {
        instruction_seed = parse(take(), ~0u);
        lessons = parse(take(), 256);
        if (!lessons || lessons % 2)
            throw std::invalid_argument("give a positive even number of public demonstrations");
        if (input >> word) {
            if (word != "live" && word != "shifted" && word != "fixed")
                throw std::invalid_argument("choose live, shifted or fixed acoustic pairing");
            pairing = word == "shifted" ? 1 : word == "fixed" ? 2 : 0;
        }
    } else if (token == "/audience" || token == "/audience-works") {
        if (token == "/audience-works") {
            std::string path;
            if (!(input >> std::quoted(path)))
                throw std::invalid_argument("select a captured watercourse");
            auto samples = retina::read_surfaces(path);
            Word i = parse(take(), samples.size() - 1);
            specimen = std::make_unique<Temporary>(s.d, sizeof(retina::Surface));
            s.d.check(s.d.upload(specimen->ptr, &samples[i], sizeof(retina::Surface)), "place the physical bed");
            q.specimen = reinterpret_cast<const retina::Surface *>(specimen->ptr);
        }
        q.stock.seed = mix(parse(take(), ~0u) ^ 0x6394d38bu);
        q.life = parse(take(), ~0u / 2);
        q.seat = position();
        if (input >> word)
            q.stock.arm = audience::treatment(word);
        if (input >> word)
            q.stock.access = audience::access_mode(word);
    } else if (!audience_open) {
        throw std::invalid_argument("open a shared audience table first");
    } else if (token == "/audience-load") {
        q.act = audience::load;
        q.inlet = position();
        q.noun = acoustic::spoken_word(take());
    } else if (token == "/audience-pour") {
        q.act = audience::pour;
    } else if (ordinary || token == "/audience-ask") {
        q.act = audience::ask;
        if (!ordinary) {
            auto who = take();
            if (who != "guide" && who != "pupil")
                throw std::invalid_argument("address the guide or pupil");
            q.who = who == "pupil";
        }
        std::istringstream question(ordinary ? line : std::string(std::istreambuf_iterator<char>(input), {}));
        while (question >> word) {
            if (q.words.n == braid::max_phrase || !(q.words.wave[q.words.n] = acoustic::spoken_word(word))) {
                if (ordinary)
                    return false;
                throw std::invalid_argument("a question needs one to eight audible words");
            }
            ++q.words.n;
        }
    } else
        return false;
    if (!ordinary && input >> word)
        throw std::invalid_argument("unexpected audience argument");
    if (!audience_table) {
        audience_table = std::make_unique<Temporary>(s.d, sizeof(audience::Table));
        audience_couple = std::make_unique<Temporary>(s.d, sizeof(audience::Pair));
        audience_receipt = std::make_unique<Temporary>(s.d, sizeof(audience::Reply));
        s.bind(social_action, "audience_table", Image::runnels);
        s.bind(social_words, "audience_names", Image::runnels);
    }
    auto g = connect_cortex(0), p = connect_cortex(1);
    std::unique_ptr<Temporary> visiting;
    if (!p.address) {
        visiting = std::make_unique<Temporary>(s.d, sizeof(Tissue));
        p.address = visiting->ptr;
        s.d.check(s.d.upload(p.address, &listener.brain.tissue[p.index], sizeof(Tissue)), "seat the visiting pupil");
    }
    auto &a = local.brain.tissue[g.index], &b = listener.brain.tissue[p.index];
    s.d.check(s.d.download(&a, g.address, sizeof(a)), "read the guide's active cortex");
    s.d.check(s.d.download(&b, p.address, sizeof(b)), "read the pupil's active cortex");
    audience::Pair pair{audience::embodied(local, a), audience::embodied(listener, b)};
    Count voice = audience::public_voice(local);
    q.stock.calibrated = audience::recall(listener, voice, q.stock.dialect);
    s.d.check(s.d.upload(audience_couple->ptr, &pair, sizeof(pair)), "place two independently grounded bodies");
    std::array<braid::Saved, 2> hands{local.braided, listener.braided};
    s.d.check(s.d.upload(float_head.ptr, hands.data(), sizeof(hands)), "seat the two hands");
    report_memory(true);
    void *lexical[]{&audience_couple->ptr, &float_memories.ptr};
    s.d.check(s.d.launch(social_words, 1, 1, 1, 8, 1, 1, 0, nullptr, lexical, nullptr),
              "connect grounded public sounds");
    if (lessons) {
        Temporary received(s.d, sizeof(audience::Practice));
        Function learn{};
        s.bind(learn, "audience_practice", Image::runnels);
        Word guide_budget = pair.guide.energy ? pair.guide.energy - 1 : 0,
             pupil_budget = pair.pupil.energy ? pair.pupil.energy - 1 : 0;
        DevicePtr no_trace = 0;
        void *args[]{&audience_couple->ptr, &q.stock,      &instruction_seed, &lessons, &pairing,
                     &guide_budget,         &pupil_budget, &received.ptr,     &no_trace};
        s.d.check(s.d.launch(learn, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                  "hear words alongside publicly moving grains");
        audience::Practice r{};
        s.d.check(s.d.download(&r, received.ptr, sizeof(r)), "retain the witnessed lesson");
        if (r.guide_heat > guide_budget || r.pupil_heat > pupil_budget || r.grains > r.offers || r.moved > r.grains)
            throw std::logic_error("the demonstration exceeded its stock");
        if (r.paired && pairing != 2)
            audience::retain(listener, voice, r.learned);
        a.spike = r.guide.spikes;
        a.gate_b = (a.gate_b & 65535u) | (r.tape << 16);
        b.spike = r.pupil.spikes;
        local.braided.mind.volley = (local.braided.mind.volley & 65535u) | (r.guide.contacts << 16);
        listener.braided.mind.volley = (listener.braided.mind.volley & 65535u) | (r.pupil.contacts << 16);
        s.d.check(s.d.upload(g.address, &a, sizeof(a)), "retain the guide's visual activity");
        s.d.check(s.d.upload(p.address, &b, sizeof(b)), "retain the pupil's visual activity");
        local.resident.animal.energy -= r.guide_heat;
        listener.resident.animal.energy -= r.pupil_heat;
        local.resident.animal.worked += r.guide_heat;
        listener.resident.animal.worked += r.pupil_heat;
        std::cout << "public_practice=" << r.offers << "  paired=" << r.paired << "  grains=" << r.grains
                  << "  moved=" << r.moved << "  edits=" << r.writes << "  guide_heat=" << r.guide_heat
                  << "  pupil_heat=" << r.pupil_heat << "  first=" << r.learned.bank[0]
                  << "  second=" << r.learned.bank[1] << '\n';
        return true;
    }
    void *args[]{
        &audience_table->ptr,  &audience_couple->ptr, &float_memories.ptr, &float_head.ptr, &g.address, &p.address, &q,
        &audience_receipt->ptr};
    s.d.check(s.d.launch(social_action, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr), "hear the other resident");
    audience::Reply r{};
    s.d.check(s.d.download(&r, audience_receipt->ptr, sizeof(r)), "hear the public offer and weigh the food");
    if (r.status) {
        if (ordinary)
            return false;
        throw std::invalid_argument("the selected hold, marker or question is unavailable");
    }
    if (r.stock + r.held + r.settled + r.bites != 16 || r.paid_guide > pair.guide.energy ||
        r.paid_pupil > pair.pupil.energy)
        throw std::logic_error("the audience table exceeded its reserves");
    report_memory(false);
    if (q.act == audience::meet) {
        audience_open = true;
        cascade_listening = false;
        s.d.check(s.d.download(hands.data(), float_head.ptr, sizeof(hands)), "retain two sets of learned contacts");
        local.braided = hands[0];
        listener.braided = hands[1];
        s.d.check(s.d.download(&a, g.address, sizeof(a)), "retain the guide's social experience");
        s.d.check(s.d.download(&b, p.address, sizeof(b)), "retain the pupil's evidence");
        local.resident.animal.energy -= r.paid_guide;
        local.resident.animal.worked += r.paid_guide;
        listener.resident.animal.energy -= r.paid_pupil;
        listener.resident.animal.worked += r.paid_pupil;
        std::cout << "public_offer=" << r.face << "  first=" << r.first << "  second=" << r.second
                  << "  pupil_saw=" << r.viewed << "  selected=" << r.picked << "  words=" << r.words.n
                  << "  heard=" << r.heard << "  contact_edits=" << r.changed << "  guide_heat=" << r.paid_guide
                  << "  pupil_heat=" << r.paid_pupil << "  calibrated=" << q.stock.calibrated << '\n';
    } else if (q.act == audience::pour) {
        auto &body = listener.resident.animal;
        body.energy += r.eaten;
        body.meals += r.eaten;
        body.diet[1] += r.eaten;
    }
    if (q.act == audience::ask || q.act == audience::meet)
        std::cout << (q.act == audience::meet || !q.who ? "guide> " : "pupil> ")
                  << (r.words.n ? float_words(r.words) : "?") << '\n';
    if (q.act == audience::load || q.act == audience::pour)
        std::cout << "audience_delivery=" << q.act << "  held=" << r.held << "  available=" << r.stock
                  << "  pupil_ate=" << r.bites << "  settled=" << r.settled << "  outlet=" << r.outlet << '\n';
    return true;
}
