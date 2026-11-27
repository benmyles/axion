bool cascade_command(const std::string &token, std::istringstream &input, const std::string &line) {
    bool ordinary = !token.empty() && token[0] != '/';
    if (ordinary ? !cascade_listening : token != "/cascade" && !token.starts_with("/cascade-"))
        return false;
    if (!local.riverine || !local.binds_floats || !float_memories.ptr || !local.resident.animal.body.eyes)
        throw std::invalid_argument("a sequence of watercourses needs a sighted embodied resident");
    retina::Request q{};
    std::unique_ptr<Temporary> specimen, exposures;
    std::string arg;
    auto take = [&]() {
        if (!(input >> arg))
            throw std::invalid_argument("missing cascade argument");
        return arg;
    };
    if (token == "/cascade-works") {
        std::string path;
        if (!(input >> std::quoted(path)))
            throw std::invalid_argument("select a captured watercourse file");
        auto samples = retina::read_surfaces(path);
        Word i = parse(take(), samples.size() - 1);
        q.number = parse(take(), ~0u);
        q.beds = 2;
        q.op = retina::open;
        if (input >> arg) {
            bool lighting = arg == "bright" || arg == "natural";
            if (!lighting)
                q.life = parse(arg, 65535);
            if (lighting || input >> arg) {
                if (arg != "bright" && arg != "natural")
                    throw std::invalid_argument("choose natural or bright illumination");
                q.bright = arg == "bright";
            }
        }
        specimen = std::make_unique<Temporary>(s.d, sizeof(retina::Surface));
        s.d.check(s.d.upload(specimen->ptr, &samples[i], sizeof(samples[i])),
                  "place the recorded physical bed on the table");
        q.surface = reinterpret_cast<const retina::Surface *>(specimen->ptr);
    } else if (token == "/cascade") {
        q.op = retina::open;
        q.number = parse(take(), ~0u);
        q.beds = parse(take(), 4);
        if (!q.beds || !std::has_single_bit(q.beds))
            throw std::invalid_argument("choose one, two or four beds");
        q.life = parse(take(), 65535);
        if (input >> arg) {
            if (arg != "bright" && arg != "natural")
                throw std::invalid_argument("use natural or bright illumination");
            q.bright = arg == "bright";
        }
    } else if (token == "/cascade-rack") {
        q.op = retina::supply_rack;
        q.number = parse(take(), 192);
    } else if (token == "/cascade-watch" || token == "/cascade-attend" || token == "/cascade-stir") {
        q.probes = token == "/cascade-stir";
        q.voluntary_view = token != "/cascade-watch";
        q.op = retina::watch;
        q.number = parse(take(), 512);
        if (input >> arg) {
            if (arg != "live" && arg != "displaced" && arg != "shutter" && arg != "independent" && arg != "closed")
                throw std::invalid_argument("choose live, displaced, independent, shutter or closed");
            q.channel = arg == "independent" ? retina::independent
                        : arg == "live"      ? retina::paired
                        : arg == "displaced" ? retina::displaced
                        : arg == "shutter"   ? retina::dark
                                             : retina::closed;
        }
    } else if (token == "/cascade-name") {
        q.op = retina::demonstrate;
        q.noun = acoustic::spoken_word(take());
        auto path = take();
        if (!q.noun || path.empty() || path.size() > 4)
            throw std::invalid_argument("a new word needs one to four demonstrated flashes");
        for (char digit : path) {
            if (digit < '0' || digit >= '0' + int(cascade_beds))
                throw std::invalid_argument("a demonstrated gate is absent");
            q.route |= Count(digit - '0') << (2 * q.number++);
        }
        q.examples = parse(take(), 128);
        q.mode = lesson_modes[0] & 255;
        if (q.examples) {
            exposures = std::make_unique<Temporary>(s.d, sizeof(retina::WordExposure) * q.examples);
            q.exposures = reinterpret_cast<retina::WordExposure *>(exposures->ptr);
        }
        if (input >> arg) {
            if (arg != "crowded" && arg != "fixed" && arg != "clear" && arg != "unpaired")
                throw std::invalid_argument("choose crowded, fixed, clear or unpaired lamps");
            q.demonstration = arg == "fixed" ? 1u : arg == "clear" ? 2u : arg == "unpaired" ? 3u : 0u;
        }
    } else if (token == "/cascade-load") {
        q.op = retina::load;
        q.noun = acoustic::spoken_word(take());
        if (!q.noun)
            throw std::invalid_argument("use an audible marker name");
    } else if (token == "/cascade-pour") {
        q.op = retina::deliver;
        auto route = take();
        if (route.empty() || route.size() > 32)
            throw std::invalid_argument("select up to 32 physical gates");
        if (route == "built") {
            if (!cascade_uses_surface)
                throw std::invalid_argument("a built route needs a captured physical bed");
            route.clear();
        }
        for (char digit : route) {
            if (digit < '0' || digit >= '0' + int(cascade_beds))
                throw std::invalid_argument("a gate number is outside this cascade");
            q.route |= Count(digit - '0') << (2 * q.number++);
        }
    } else if (token == "/cascade-banks") {
        q.op = retina::disconnect;
        auto kind = take();
        if (kind != "shared" && kind != "inherited")
            throw std::invalid_argument("use shared or inherited sensory banks");
        q.number = kind == "shared";
    } else if (ordinary || token == "/cascade-extend") {
        q.op = retina::word_sequence;
        q.number = !ordinary;
        std::istringstream sounds(ordinary ? line : std::string(std::istreambuf_iterator<char>(input), {}));
        while (sounds >> arg) {
            if (q.words.n >= braid::max_phrase || !(q.words.wave[q.words.n] = acoustic::spoken_word(arg))) {
                if (ordinary)
                    return false;
                throw std::invalid_argument("each utterance can contain up to eight audible words");
            }
            ++q.words.n;
        }
        if (!q.words.n)
            return false;
    } else
        return false;
    if (!ordinary && input >> arg)
        throw std::invalid_argument("unexpected cascade argument");
    if (!cascade_bench) {
        if (q.op != retina::open)
            throw std::invalid_argument("first open a physical cascade");
        cascade_bench = std::make_unique<Temporary>(s.d, sizeof(retina::Bench));
        cascade_receipt = std::make_unique<Temporary>(s.d, sizeof(retina::Answer));
        s.bind(cascade_act, "cascade_contact", Image::runnels);
    }
    auto &body = local.resident.animal;
    auto &g = local.resident.germ;
    auto &h = local.braided.mind;
    auto motor = connect_cortex(0);
    auto auditory = q.op == retina::demonstrate ? connect_cortex(0, local.braid_report.memory.auditory_route) : motor;
    retina::Observer eye{{g.pigments[0], g.pigments[1]},
                         body.heading,
                         (h.genes.muscle >> 24) & 7,
                         (h.genes.arbor >> 25) & 7,
                         drift::wiring(h.genes, drift::recruitment),
                         braid::eye_wiring(braid::individual_eyes, h.genes, body.heading)};
    eye.iris = {g.pigments[2], g.pigments[3]};
    q.energy = body.energy;
    q.mouth = body.body.pores ? 1 + std::popcount(body.body.pores) : 0;
    s.d.check(s.d.upload(float_head.ptr, &local.braided, sizeof(local.braided)), "present the learner's current hand");
    report_memory(true);
    void *args[]{&cascade_bench->ptr,  &float_head.ptr,      &float_memories.ptr, &motor.address, &eye, &q,
                 &float_regulator.ptr, &cascade_receipt->ptr};
    s.d.check(s.d.launch(cascade_act, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "observe, hear, or open a physical cascade");
    retina::Answer r{};
    s.d.check(s.d.download(&r, cascade_receipt->ptr, sizeof(r)), "weigh the material and hear the response");
    if (ordinary && r.status)
        return false;
    if (r.rack_stock + retina::grain_count(r.rack_deposits) != r.rack_initial || (q.probes && r.grains != 64) ||
        r.food + r.settled + r.supply != 8 || r.paid >= q.energy || Count(q.energy) + r.bite - r.paid > 65535)
        throw std::logic_error("cascade optical and feeding work exceeded the material budget");
    s.d.check(s.d.download(&local.brain.tissue[motor.index], motor.address, sizeof(Tissue)),
              "retain the living population's modified gates");
    s.d.check(s.d.download(&local.braided, float_head.ptr, sizeof(local.braided)), "retain the hand after watching");
    if (q.op == retina::demonstrate)
        return_spikes(0, auditory);
    report_memory(false);
    body.energy = body.energy + r.bite - r.paid;
    body.worked += r.paid;
    body.meals += r.bite;
    body.diet[1] += r.bite;
    if (!r.status && q.op == retina::open) {
        audience_open = false;
        cascade_uses_surface = r.built_length != 0;
        cascade_beds = q.beds;
        cascade_listening = false;
    }
    if (!r.status && q.op == retina::load)
        cascade_listening = true;
    if (!r.status && q.op == retina::deliver)
        cascade_listening = false;
    std::cout << "cascade=" << (ordinary ? "heard" : token.substr(token == "/cascade" ? 1 : 9))
              << "  status=" << r.status << "  observed=" << r.attended << "  synaptic_edits=" << r.edits
              << "  heat=" << r.heat << "  supply=" << r.supply << "  eaten=" << r.food << "  settled=" << r.settled
              << "  lamps=" << r.lamps << "  beds=" << cascade_beds << "  inlet=" << r.source
              << "  recognized=" << r.heard << "  sequence=" << r.cues << "  openings=" << r.options;
    if (q.op == retina::supply_rack || r.available_rack || r.query_offers || r.stopped)
        std::cout << "  available_probes=" << r.available_rack << "  query_offers=" << r.query_offers
                  << "  rack_stock=" << r.rack_stock << "  deposited=" << retina::grain_count(r.rack_deposits)
                  << "  query_paid=" << r.query_paid << "  initial_openings=" << r.initial_prediction
                  << "  committed=" << r.stopped;
    if (q.probes)
        std::cout << "  dislodged=" << r.disturbed << "  relocated=" << r.relocated << "  dry_offers=" << r.denied
                  << "  first_source=" << r.first_source << "  second_source=" << r.second_source
                  << "  selected_source=" << r.hand_source << "  tracer_exit=" << r.at << "  tracer_stock=" << r.grains;
    if (q.probes && r.marker < 4) {
        Count word = braid::name_of(local.braid_report.memory, 4 + r.marker);
        std::cout << "  marker_name=" << (word ? acoustic::spell(word) : std::string("?"));
    }
    if (q.voluntary_view)
        std::cout << "  opportunities=" << r.offers << "  looked=" << r.attended
                  << "  declined=" << r.offers - r.attended;
    if (q.op == retina::demonstrate)
        std::cout << "  word=" << acoustic::spell(q.noun) << "  demonstrations=" << r.examples
                  << "  visible=" << r.last_glints << "  contacts=" << r.contacts << "  resolved_stages=" << r.decoded
                  << "  last_water_exit=" << r.passage;
    auto spoken_port = [&](Word port) {
        auto wave = braid::name_of(local.braid_report.memory, port);
        return wave ? acoustic::spell(wave) : std::string("?");
    };
    if (r.chosen)
        std::cout << "  reached=" << r.aim << "  reached_name=" << spoken_port(r.aim);
    if (r.outlet < 4)
        std::cout << "  delivered=" << r.outlet << "  delivered_name=" << spoken_port(r.outlet);
    if (q.op == retina::open && r.built_length) {
        std::cout << "  built=";
        for (Word k = 0; k < r.built_length; ++k)
            std::cout << ((r.built_turns >> k) & 1);
    }
    if (q.op == retina::open) {
        std::cout << "  gate_names=";
        for (Word j = 0; j < cascade_beds; ++j) {
            if (j)
                std::cout << ',';
            auto known = braid::name_of(local.braid_report.memory, 4 + ((r.lamps >> (2 * j)) & 3));
            std::cout << (known ? acoustic::spell(known) : "?");
        }
    }
    std::cout << '\n';
    if (r.examples) {
        std::vector<retina::WordExposure> samples(r.examples);
        s.d.check(s.d.download(samples.data(), exposures->ptr, samples.size() * sizeof(samples[0])),
                  "read the word's retinal evidence");
        std::cout << "temporal_word=" << acoustic::spell(q.noun) << "  length=" << q.number
                  << "  rule=" << ((h.genes.plastic >> 8) & 15) << "  mode=" << q.mode << "  old=" << r.earlier
                  << "  initial_spikes=" << r.spikes << "  background=" << r.background << "  first=" << r.first
                  << "  second=" << r.second << "  decoded=" << r.decoded << "  evidence=";
        for (Word i = 0; i < samples.size(); ++i) {
            if (i)
                std::cout << ',';
            std::cout << samples[i].light_and_contacts << '/' << samples[i].spikes;
        }
        std::cout << '\n';
    }
    return true;
}
