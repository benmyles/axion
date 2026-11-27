bool capillary_command(const std::string &token, std::istringstream &input, const std::string &line) {
    namespace k = capillary;
    bool ordinary = !token.empty() && token.front() != '/';
    if (ordinary ? !local.hydraulic : !token.starts_with("/cap-"))
        return false;
    if (!local.binds_floats || !partner().binds_floats || !local.riverine || !partner().riverine) {
        if (ordinary)
            return false;
        throw std::invalid_argument("enroll two embodied watershed residents");
    }
    k::Request q{};
    std::string arg;
    auto take = [&]() {
        if (!(input >> arg))
            throw std::invalid_argument("missing capillary argument");
        return arg;
    };
    auto numeric = [&](Word max) { return parse(take(), max); };
    auto person = [&]() {
        auto x = take();
        if (x != "speaker" && x != "pupil")
            throw std::invalid_argument("address speaker or pupil");
        return Word(x == "pupil");
    };
    auto colour = [&]() {
        auto x = take();
        const char *names[]{"amber", "jade", "plum", "teal"};
        for (Word j = 0; j < 4; ++j)
            if (x == names[j])
                return j;
        return parse(x, 3);
    };
    auto quadrant = [&]() {
        auto where = take();
        if (where == "nw")
            return 0u;
        if (where == "ne")
            return 1u;
        if (where == "sw")
            return 2u;
        if (where == "se")
            return 3u;
        return parse(where, 3);
    };
    if (token == "/cap-cell" || token == "/cap-channel") {
        q.action = token == "/cap-cell" ? k::set_cell : k::set_rig;
        q.a = numeric(q.action == k::set_cell ? 63u : ~0u);
        if (q.action == k::set_rig) {
            q.b = numeric(14);
            if (q.b < 2)
                throw std::invalid_argument("use two to fourteen pulse steps");
        }
        q.c = colour();
        q.d = colour();
        q.e = numeric(31);
        if (q.action == k::set_rig && input >> arg) {
            if (arg != "scarce" && arg != "plentiful")
                throw std::invalid_argument("choose scarce or plentiful stock");
            q.seed = arg == "scarce";
        }
    } else if (token == "/cap-radio") {
        q.action = k::set_acoustic_link;
        auto channel = take();
        if (channel == "clear")
            q.a = 0;
        else if (channel == "silent")
            q.a = 1;
        else if (channel == "partial")
            q.a = 2;
        else if (channel == "reordered")
            q.a = 3;
        else if (channel == "duplicate")
            q.a = 4;
        else if (channel == "patchy")
            q.a = 7;
        else
            throw std::invalid_argument("choose clear, silent, partial, reordered, duplicate or patchy transmission");
        q.b = numeric(2);
        if (input >> arg) {
            if (arg != "actual" && arg != "withheld" && arg != "exchanged")
                throw std::invalid_argument("choose actual, withheld or exchanged reply observations");
            q.c = arg == "withheld" ? 1 : arg == "exchanged" ? 2 : 0;
        }
    } else if (token == "/cap-private") {
        q.action = k::private_feeder;
        q.who = person();
        q.a = quadrant();
        q.seed = numeric(~0u);
    } else if (token == "/cap-ear") {
        q.action = k::acoustic_shutter;
        q.who = person();
        auto state = take();
        if (state != "open" && state != "closed")
            throw std::invalid_argument("open or close the acoustic shutter");
        q.a = state == "open";
    } else if (token == "/cap-name-port" || token == "/cap-name-step") {
        bool move = token == "/cap-name-step";
        q.action = move ? k::teach_heading : k::teach_resonator;
        q.who = person();
        q.a = numeric(1);
        if (move)
            q.b = numeric(1);
        q.statement = utterance::read_line(take());
        std::getline(input, arg);
        q.question = utterance::read_line(arg.find_first_not_of(" \t") == std::string::npos
                                              ? (move ? "Where is the food" : "What did you see")
                                              : arg);
    } else if (token == "/cap-twin") {
        q.action = k::attach_second;
        q.who = person();
        q.a = numeric(63);
        q.b = numeric(31);
    } else if (token == "/cap-prime") {
        q.action = k::prime_pair;
        q.who = person();
    } else if (token == "/cap-message4") {
        q.action = k::signal_quadrant;
        q.who = person();
        q.a = quadrant();
        q.seed = numeric(~0u);
        if (input >> arg) {
            if (arg != "practice" && arg != "frozen")
                throw std::invalid_argument("choose practice or frozen contacts");
            q.b = arg == "practice";
            if (input >> arg) {
                if (arg != "plumes" && arg != "pooled" && arg != "single")
                    throw std::invalid_argument("choose single, plumes or pooled food");
                q.c = arg == "plumes" ? 1 : arg == "pooled" ? 2 : 0;
            }
        }
    } else if (token == "/cap-message") {
        q.action = k::signal_peer;
        q.who = person();
        auto side = take();
        if (side != "left" && side != "right")
            throw std::invalid_argument("stock the left or right feeder");
        q.a = side == "right";
        q.seed = numeric(~0u);
        if (input >> arg) {
            if (arg != "practice" && arg != "frozen")
                throw std::invalid_argument("choose practice or frozen contacts");
            q.b = arg == "practice";
        }
    } else if (token == "/cap-seal") {
        q.action = k::seal_cell;
        q.who = person();
    } else if (token == "/cap-recover") {
        q.action = k::recover_drops;
        q.who = person();
        q.a = numeric(32);
    } else if (token == "/cap-mineral") {
        q.action = k::change_mineral;
        q.a = numeric(31);
    } else if (token == "/cap-probe") {
        q.action = k::probe_cell;
        q.who = person();
        q.seed = numeric(~0u);
    } else if (token == "/cap-context") {
        q.action = k::set_context;
        q.who = person();
        auto mode = take();
        if (mode != "adaptive" && mode != "single" && mode != "regional")
            throw std::invalid_argument("choose adaptive, regional or single material memory");
        q.a = mode == "regional" ? 3 : mode == "adaptive";
    } else if (token == "/cap-explore") {
        q.action = k::experiment;
        q.who = person();
        q.a = numeric(512);
        q.b = numeric(256);
        q.seed = numeric(~0u);
    } else if (token == "/cap-grip" || token == "/cap-build") {
        q.action = token == "/cap-grip" ? k::move_handles : k::design_work;
        q.who = person();
        q.a = numeric(q.action == k::move_handles ? 63 : k::grips);
    } else if (token == "/cap-look") {
        q.action = k::open_eye;
        q.who = person();
        auto state = take();
        if (state != "open" && state != "closed")
            throw std::invalid_argument("open or close the eye");
        q.a = state == "open";
    } else if (token == "/cap-memory" || token == "/cap-sample" || token == "/cap-follow" || token == "/cap-pour" ||
               token == "/cap-net") {
        q.action = token == "/cap-follow"   ? k::follow_words
                   : token == "/cap-memory" ? k::recall_field
                   : token == "/cap-sample" ? k::new_sample
                   : token == "/cap-pour"   ? k::pulse
                                            : k::set_net;
        q.who = person();
    } else if (token == "/cap-point") {
        q.action = k::acquire_name;
        q.who = person();
        q.a = numeric(k::atoms - 1);
        q.statement = utterance::read_line(take());
    } else if (token == "/cap-caption" || token == "/cap-caption-grips") {
        q.action = token == "/cap-caption" ? k::acquire_form : k::caption_grips;
        q.who = person();
        if (!std::getline(input, arg, '|') || input.eof())
            throw std::invalid_argument("separate the question and the public answer with |");
        q.question = utterance::read_line(arg);
        std::getline(input, arg);
        q.statement =
            arg.find_first_not_of(" \t") == std::string::npos ? utterance::Waves{} : utterance::read_line(arg);
    } else if (ordinary || token == "/cap-ask" || token == "/cap-pass") {
        q.action = token == "/cap-pass" ? k::send_answer : k::answer_question;
        try {
            if (ordinary)
                q.question = utterance::read_line(line);
            else {
                q.who = person();
                std::getline(input, arg);
                q.question = utterance::read_line(arg);
            }
        } catch (const std::invalid_argument &) {
            if (ordinary)
                return false;
            throw;
        }
    } else
        return false;
    if (q.action == k::answer_question || q.action == k::send_answer || q.action == k::acquire_form) {
        if (!q.question.n || q.question.n > k::syllables || q.statement.n > k::syllables) {
            if (ordinary)
                return false;
            throw std::invalid_argument("the learned expression has one to eight sounds");
        }
    }
    if (!ordinary && input >> arg)
        throw std::invalid_argument("unexpected capillary argument");
    if (!cap_table) {
        cap_table = std::make_unique<Temporary>(s.d, sizeof(k::Workshop));
        cap_brains = std::make_unique<Temporary>(s.d, 2 * sizeof(k::Saved));
        cap_bodies = std::make_unique<Temporary>(s.d, 2 * sizeof(k::Body));
        cap_reply = std::make_unique<Temporary>(s.d, sizeof(k::Reply));
        k::Workshop t{};
        s.d.check(s.d.upload(cap_table->ptr, &t, sizeof(t)), "set out the capillary workshop");
        s.bind(cap_handle, "capillary_table", Image::runnels);
    }
    bool plastic = q.action == k::experiment || q.action == k::acquire_form || q.action == k::acquire_name ||
                   q.action == k::caption_grips;
    std::array<k::Body, 2> bodies{};
    for (Word j = 0; j < 2; ++j) {
        auto &a = float_actor(j);
        q.amount[j] = a.resident.animal.energy;
        q.appetitive[j] = a.appetitive;
        q.compound[j] = a.compound_contacts;
        q.voices[j] = a.signal_voice;
        q.cochlea[j] = k::signal_ear(a);
        bool sensation = (q.action == k::pulse || q.action == k::probe_cell) &&
                         (a.channels.adaptive || (q.action == k::probe_cell && j == q.who));
        if (((plastic && j == q.who) || sensation || q.action == k::signal_peer || q.action == k::signal_quadrant ||
             q.action == k::private_feeder || q.action == k::teach_resonator || q.action == k::teach_heading ||
             ((q.action == k::send_answer || q.action == k::answer_question) && a.voices_signs)) &&
            (!j || !visitor)) {
            Word band = utterance::band_of(a);
            auto &t = a.brain.tissue[band];
            s.d.check(s.d.download(&t, somatic_bands[j] + Count(band) * sizeof(Tissue), sizeof(t)),
                      "read the expressed spiking pupil");
        }
        bodies[j] = k::phenotype(a);
        if (!cap_ready || std::memcmp(&cap_cache[j], &a.channels, sizeof(k::Saved))) {
            s.d.check(s.d.upload(cap_brains->ptr + j * sizeof(k::Saved), &a.channels, sizeof(k::Saved)),
                      "connect lived capillary contacts");
            cap_cache[j] = a.channels;
        }
    }
    cap_ready = true;
    s.d.check(s.d.upload(cap_bodies->ptr, bodies.data(), sizeof(bodies)), "connect both physical operators");
    void *args[]{&cap_table->ptr, &cap_brains->ptr, &cap_bodies->ptr, &q, &cap_reply->ptr};
    s.d.check(s.d.launch(cap_handle, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "intervene, predict or explain a pulse");
    k::Reply r{};
    s.d.check(s.d.download(&r, cap_reply->ptr, sizeof(r)), "read the actions and the actual drop");
    if (ordinary && r.status == 2)
        return false;
    if (r.status && r.status != 2) {
        cap_ready = false;
        throw std::invalid_argument(r.status == 3 ? "the workshop lost matter"
                                                  : "this stimulus, material stock or energy reserve is unavailable");
    }
    for (Word j = 0; j < 2; ++j) {
        if (r.heat[j] > q.amount[j])
            throw std::logic_error("capillary action exceeds the individual's energy");
        auto &a = float_actor(j).resident.animal;
        a.energy -= r.heat[j];
        a.worked += r.heat[j];
        Word meal = Word(r.gained == j + 1) + r.share;
        if (meal) {
            if (meal > 65535u - a.energy)
                throw std::logic_error("the receiving body cannot hold the offered portions");
            a.energy += meal;
            a.meals += meal;
            a.diet[2] += meal;
        }
    }
    auto retain = [&](Word j, Word spike, bool somatic) {
        auto &a = float_actor(j);
        s.d.check(s.d.download(&a.channels, cap_brains->ptr + j * sizeof(k::Saved), sizeof(k::Saved)),
                  "retain experienced pressure evidence");
        cap_cache[j] = a.channels;
        a.hydraulic = true;
        if (somatic) {
            Word band = utterance::band_of(a);
            a.brain.tissue[band].spike = spike;
            if (!j || !visitor)
                s.d.check(
                    s.d.upload(somatic_bands[j] + Count(band) * sizeof(Tissue) + offsetof(Tissue, spike), &spike, 4),
                    "retain the observer's spiking state");
        }
    };
    if (r.voice_action) {
        for (Word j = 0; j < 2; ++j)
            if (r.voice_changes & (1u << j)) {
                auto &p = float_actor(j);
                p.voices_signs = p.compound_signals = p.material_signals = p.hydraulic = true;
                p.signal_voice = r.voices[j];
            }
        std::cout << "voiced_encounter=" << r.voice_action << "  person=" << q.who
                  << "  cue=" << (q.action == k::private_feeder ? q.a : 4u) << "  mark=" << r.marked
                  << "  sensation=" << r.sound_state << "  interpreted=" << r.listener_state
                  << "  reached=" << (r.ready ? r.feeder : 4u) << "  words=" << r.words.n
                  << "  portions=" << 2 * r.share << "  transmitted_words=" << r.delivered
                  << "  questions=" << r.exchange.offered << "  answers=" << r.exchange.answered
                  << "  candidates=" << r.exchange.remaining << "  remaining=" << r.feeder_stock
                  << "  memory_changed=" << r.voice_changes << '\n';
    }
    if (q.action == k::signal_quadrant) {
        for (Word j = 0; j < 2; ++j) {
            auto &p = float_actor(j);
            p.compound_signals = p.material_signals = true;
            p.compound_contacts = r.compound[j];
        }
        std::cout << "compound_message=" << q.who << "  cue=" << q.a << "  mark=" << r.marked << "  sensed=" << r.lamp
                  << "  feeder=" << r.feeder << "  erase_first=" << r.missing_first
                  << "  erase_second=" << r.missing_second << "  erase_both=" << r.erased_choice
                  << "  portions=" << 2 * r.share << "  remaining=" << r.feeder_stock << "  diet=" << q.c
                  << "  crop=" << r.crop << "  external_drops=" << r.stock << "  first_orientation=" << r.sealed
                  << "  second_orientation=" << r.second_seal << "  practice=" << q.b << '\n';
    }
    if (q.action == k::signal_peer) {
        for (Word j = 0; j < 2; ++j) {
            auto &p = float_actor(j);
            p.material_signals = true;
            p.appetitive = r.appetitive[j];
        }
        std::cout << "material_message=" << q.who << "  cue=" << q.a << "  mark=" << r.marked << "  sensed=" << r.lamp
                  << "  receiver_choice=" << r.feeder << "  erased_choice=" << r.erased_choice
                  << "  inverse_choice=" << r.opposite_choice << "  portions=" << 2 * r.share
                  << "  remaining=" << r.feeder_stock << "  practice=" << q.b << '\n';
    }
    if (r.saved)
        retain(q.who, r.terminal, plastic);
    for (Word j = 0; j < 2; ++j)
        if (r.contact & (1u << j))
            retain(j, r.new_spike[j], true);
    if (q.action == k::answer_question || q.action == k::send_answer)
        std::cout << (q.who ? "pupil> " : "speaker> ") << (r.words.n ? utterance::render(r.words) : "?") << '\n';
    for (Word j = 0; j < r.exchange.offered; ++j) {
        std::cout << (q.who ? "speaker> " : "pupil> ") << acoustic::spell(r.exchange.question[j]) << "?\n";
        if (r.exchange.reply[j])
            std::cout << (q.who ? "pupil> " : "speaker> ") << acoustic::spell(r.exchange.reply[j]) << '\n';
        std::cout << "audible_clarification=" << j << "  requester=" << 1 - q.who
                  << "  question=" << r.exchange.question[j] << "  reply=" << r.exchange.reply[j]
                  << "  possibilities_before=" << r.exchange.initial << "  possibilities_after=" << r.exchange.remaining
                  << "  initial_actions=" << r.exchange.possible_actions << '\n';
    }
    std::cout << "capillary_turn=" << q.action << "  person=" << q.who << "  handles=" << r.current
              << "  earlier=" << r.prior << "  later=" << r.result << "  changed=" << r.change
              << "  causal_set=" << r.cause << "  alternatives=" << r.possible << "  words=" << r.words.n
              << "  heat=" << r.heat[0] << "  pupil_heat=" << r.heat[1] << "  on=" << r.tissue.on
              << "  off=" << r.tissue.off << "  predicted_on=" << r.belief.on << "  predicted_off=" << r.belief.off
              << "  plan=" << r.intended << "  built=" << r.ready << "  considered=" << r.considered
              << "  accepted=" << r.accepted << "  narrowed=" << r.narrowed << "  target=" << r.physical
              << "  source=" << r.incoming << "  pad=" << r.pad << "  out=" << r.outgoing << "  drops=" << r.stock
              << "  settled=" << r.settled << "  eaten=" << r.eaten << "  got=" << r.gained << "  eyes=" << r.observed
              << "  edits=" << r.edits << "  spikes=" << r.spikes << '\n';
    if (r.sealed && (q.action == k::pulse || q.action == k::probe_cell) && !r.status)
        std::cout << "sealed_cycle=" << q.who << "  geology=" << r.geology << "  input=" << r.pump_row
                  << "  carrier_before=" << r.source_before << "  bound_before=" << r.bound_before
                  << "  flow=" << r.physical << "  carrier_after=" << r.incoming << "  bound_after=" << r.pad
                  << "  remaining=" << r.stock << "  paid=" << r.heat[q.who] << "  contacts=" << r.contact << '\n';
    if (r.adaptive || q.action == k::set_context)
        std::cout << "material_memory=" << q.who << "  mode=" << r.adaptive
                  << "  passive_rows=" << k::footprint(r.history) << "  receipts=" << (r.history.recent & 0xffffff)
                  << "  since=" << (r.history.recent >> 24) << "  interval=" << r.threshold
                  << "  rechecks=" << r.rechecks << "  contacts=" << r.contact << "  stimulus=" << r.stimulus
                  << "  written=" << r.written << '\n';
    return true;
}
