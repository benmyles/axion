static std::string float_paint(Word hues) {
    std::string result;
    for (Word i = 0; i < 4; ++i)
        result += char('0' + braid::pigment(hues, i));
    return result;
}
static Word float_paint(std::string_view text) {
    if (text.size() != 4)
        throw std::invalid_argument("give the four visible float pigments as digits 0 through 3");
    Word out = 0;
    for (Word i = 0; i < 4; ++i) {
        if (text[i] < '0' || text[i] > '3')
            throw std::invalid_argument("unknown float pigment");
        out |= Word(text[i] - '0') << (2 * i);
    }
    return out;
}
static std::string float_words(const braid::Speech &p) {
    std::string text;
    for (Word i = 0; i < p.n; ++i) {
        if (i)
            text += ' ';
        text += acoustic::spell(p.wave[i]);
    }
    return text;
}
AgentImage &float_actor(Word who) { return who == 0 ? local : who == 1 ? partner() : *float_worker; }
bool float_command(const std::string &token, std::istringstream &input, const std::string &line) {
    if (token == "/braid-exit") {
        at_braid = false;
        return true;
    }
    if (token != "/braid" && !at_braid)
        return false;
    if (!float_head.ptr)
        throw std::invalid_argument("the speaker needs float transition tissue");
    Word cmd = braid::inspect, who = 0, a = 0, z = 0, option = 0;
    std::string arg;
    std::vector<Count> sounds;
    auto quantity = [&](Word max) {
        if (!(input >> arg))
            throw std::invalid_argument("missing float quantity");
        return parse(arg, max);
    };
    auto member = [&]() {
        if (!(input >> arg))
            throw std::invalid_argument("choose speaker, partner or worker");
        if (arg == "speaker")
            return 0u;
        if (arg == "partner" && partner().binds_floats)
            return 1u;
        if (arg == "worker" && float_worker)
            return 2u;
        throw std::invalid_argument("the requested float operator is unavailable");
    };
    auto hear_line = [&](std::istream &from) {
        std::string sound;
        while (from >> sound) {
            Count v = acoustic::spoken_word(sound);
            if (!v)
                throw std::invalid_argument("silence has no acoustic identity");
            sounds.push_back(v);
        }
    };
    if (token == "/braid") {
        cmd = braid::open_table;
        a = quantity(~0u);
        at_braid = true;
        at_bank = at_flume = at_studio = at_rack = at_ford = at_marsh = at_wheel = false;
    } else if (token == "/braid-worker") {
        if (!(input >> std::quoted(arg)))
            throw std::invalid_argument("supply an independent worker checkpoint");
        AgentImage incoming;
        incoming.read(arg);
        if (!incoming.binds_floats || !incoming.riverine)
            throw std::invalid_argument("the worker has no portable float tissue");
        float_worker = std::move(incoming);
        std::cout << "braid_worker=present\n";
        return true;
    } else if (token == "/braid-save-worker") {
        if (!float_worker || !(input >> std::quoted(arg)))
            throw std::invalid_argument("supply a worker and destination");
        storage::write_checked(arg, *float_worker);
        return true;
    } else if (token == "/braid-station") {
        cmd = braid::set_station;
        who = member();
        a = quantity(127);
        if (!(input >> arg))
            throw std::invalid_argument("supply four float pigments");
        z = float_paint(arg);
        option = (input >> std::ws).eof() ? 15u : quantity(15);
    } else if (token == "/braid-filter") {
        cmd = braid::filter;
        who = member();
        if (!(input >> arg))
            throw std::invalid_argument("supply four sieve pigments");
        a = float_paint(arg);
    } else if (token == "/braid-feed") {
        cmd = braid::feed;
        who = member();
        a = quantity(8192);
    } else if(token=="/braid-pull"){
        cmd=braid::guided_pull;who=member();
        if(!(input>>arg) || arg.empty() || arg.size()>braid::max_phrase)throw std::invalid_argument("give one to eight motor signals");
        for(char motor:arg){
            if(motor<'0'||motor>'3')throw std::invalid_argument("motor signals are numbered 0 through 3");
            z|=Word(motor-'0')<<(3*a++);
        }
    } else if (token == "/braid-practice") {
        cmd = braid::calibrate;
        who = member();
        a = quantity(256);
    } else if (token == "/braid-teach") {
        cmd = braid::teach;
        who = member();
        a = quantity(32);
        z = quantity(~0u);
        if (!(input >> arg) || (arg != "nested" && arg != "single" && arg != "forward" && arg != "backward"))
            throw std::invalid_argument("choose nested, single, forward or backward demonstration order");
        option = arg == "single" ? 1 : arg == "forward" ? 2 : arg == "backward" ? 3 : 0;
        hear_line(input);
        if (sounds.size() < 2 || sounds.size() > 8)
            throw std::invalid_argument("give two through eight distinct sound forms");
        if (std::set<Count>(sounds.begin(), sounds.end()).size() != sounds.size())
            throw std::invalid_argument("the demonstration forms must be distinct");
    } else if (token == "/braid-show") {
        cmd = braid::exhibit;
        who = member();
        if (!(input >> arg))
            throw std::invalid_argument("supply the starting tray");
        a = float_paint(arg);
        if (!(input >> arg) || arg.size() > 8)
            throw std::invalid_argument("give up to eight public pulls");
        if (arg != "_")
            for (char c : arg) {
                if (c < '0' || c > '3')
                    throw std::invalid_argument("pull an edge numbered 0 through 3");
                z |= Word(c - '0') << (2 * option++);
            }
        hear_line(input);
        if (sounds.empty() || sounds.size() > 8)
            throw std::invalid_argument("the observed episode can hold one through eight words");
    } else if (token == "/braid-pass") {
        if (!partner().binds_floats)
            throw std::invalid_argument("the partner needs float tissue");
        cmd = braid::revoice;
        a = quantity(4);
        option = float_worker ? 2 : 0;
        while (input >> arg) {
            if (arg == "borrow")
                option |= 1;
            else if (arg == "mute")
                z = braid::mute;
            else if (arg == "one")
                z = braid::no_repeat;
            else if (arg == "repeat")
                z = braid::repeat_probe;
            else if (arg == "foreign")
                z = braid::foreign;
            else if (arg == "blind")
                z = braid::unseeing;
            else if (arg == "garble")
                z = braid::misarticulate;
            else
                throw std::invalid_argument("unknown float dialogue channel");
        }
    } else if (token == "/braid-view") {
        cmd = braid::inspect;
    } else if (token == "/braid-say") {
        cmd = braid::utter;
        who = member();
        hear_line(input);
    } else if (!token.empty() && token[0] != '/') {
        cmd = braid::utter;
        std::istringstream whole(line);
        hear_line(whole);
    } else {
        if (token == "/backwater" || token == "/flume" || token == "/studio" || token == "/rack" || token == "/ford" ||
            token == "/marsh" || token == "/wheel" || token == "/reed")
            at_braid = false;
        return false;
    }
    if (input >> arg && cmd != braid::utter)
        throw std::invalid_argument("unexpected float argument");
    Word people = float_worker ? 3 : 2;
    std::array<braid::Saved, 3> heads{};
    Word purses[3]{};
    for (Word j = 0; j < people; ++j) {
        auto &z = float_actor(j);
        heads[j] = z.braided;
        purses[j] = z.riverine ? z.resident.animal.energy : 0;
    }
    auto usable = [&](Word j) {
        auto &z = float_actor(j);
        auto &body = z.resident.animal;
        return z.binds_floats && z.riverine && body.energy && body.body.eyes && body.body.limbs && body.body.pores;
    };
    if ((cmd == braid::utter || cmd == braid::calibrate || cmd == braid::teach || cmd == braid::exhibit) &&
        !usable(who))
        throw std::invalid_argument("the operator needs living limbs, eyes and feeding pores");
    if (cmd == braid::revoice && (!usable(0) || !usable(1) || (people == 3 && !usable(2))))
        throw std::invalid_argument("each float speaker needs a living sensory body");
    s.d.check(s.d.upload(float_head.ptr, heads.data(), sizeof(heads)), "present independent float operators");
    braid::Speech sound{};
    if (sounds.size() <= braid::max_phrase) {
        sound.n = sounds.size();
        std::copy(sounds.begin(), sounds.end(), sound.wave);
    } else {
        if (sounds.size() > std::numeric_limits<Word>::max())
            throw std::invalid_argument("the acoustic stream exceeds the device address range");
        Word n = sounds.size();
        Temporary waves(s.d, Count(n) * sizeof(Count)), result(s.d, 8);
        auto person = float_head.ptr + who * sizeof(braid::Saved);
        s.d.check(s.d.upload(waves.ptr, sounds.data(), Count(n) * sizeof(Count)),
                  "feed raw speech impulses to the GPU");
        void *args[]{&person, &waves.ptr, &n, &result.ptr};
        s.d.check(s.d.launch(float_stream, 1, 1, 1, 32, 1, 1, 0, nullptr, args, nullptr),
                  "compose a continuous acoustic stream");
        Word received[2]{};
        s.d.check(s.d.download(received, result.ptr, 8), "read the recurrent sensory goal");
        cmd = braid::continuous;
        a = received[0];
        z = n;
        option = received[1];
    }
    report_memory(true);
    void *args[]{&float_head.ptr,
                 &float_tray.ptr,
                 &cmd,
                 &who,
                 &a,
                 &z,
                 &option,
                 &sound,
                 &purses[0],
                 &purses[1],
                 &purses[2],
                 &float_reply.ptr,
                 &float_memories.ptr,
                 &people,
                 &report_shutters};
    if(cmd==braid::calibrate && lesson_modes[who]!=apprentice::unrestricted) {
        auto band=connect_cortex(who);Word mode=lesson_modes[who];
        void *hand[]{&float_head.ptr,&float_tray.ptr,&who,&a,&purses[who],&float_reply.ptr,&float_regulator.ptr,&mode};
        s.d.check(s.d.launch(regulated_practice,1,1,1,1,1,1,0,nullptr,hand,nullptr),"practice a motor prediction");
        return_spikes(who,band);
    } else {
        s.d.check(s.d.launch(float_act,1,1,1,1,1,1,0,nullptr,args,nullptr),"manipulate floats through grounded speech");
    }
    braid::Table b{};
    braid::Exchange output{};
    s.d.check(s.d.download(&b, float_tray.ptr, sizeof(b)), "read the three trays");
    s.d.check(s.d.download(&output, float_reply.ptr, sizeof(output)), "hear the returned motion program");
    s.d.check(s.d.download(heads.data(), float_head.ptr, sizeof(heads)), "retain each private lexicon");
    report_memory(false);
    if (b.tutor + b.tuition != 8192 || b.source + b.food != 8192)
        throw std::logic_error("float dialogue material imbalance");
    for (Word j = 0; j < people; ++j) {
        if (!float_actor(j).binds_floats)
            continue;
        auto &z = float_actor(j);
        z.braided = heads[j];
        auto &body = z.resident.animal;
        if (output.paid[j] > purses[j] || purses[j] - output.paid[j] + output.food[j] > 65535)
            throw std::logic_error("float operator purse imbalance");
        body.energy = purses[j] - output.paid[j] + output.food[j];
        body.worked += output.paid[j];
        body.meals += output.food[j];
        body.diet[1] += output.food[j];
    }
    if (cmd == braid::utter || cmd == braid::continuous || cmd == braid::revoice)
        std::cout << (cmd == braid::revoice ? "interpreter> " : "actor> ")
                  << (output.output.n            ? float_words(output.output)
                      : output.description.found ? "(rest)"
                                                 : "?")
                  << '\n';
    if(cmd==braid::guided_pull && output.accepted)std::cout<<"braid_hand_event="<<who<<"  before="<<output.before
            <<"  actual="<<output.after<<"  unobstructed="<<output.effects[0]<<"  shortened="<<output.effects[1]
            <<"  prediction="<<output.goal<<"  predicted_mask="<<output.known<<"  strokes="<<output.work.size<<'\n';
    std::cout << "braid_talk=" << cmd << "  person=" << who << "  ready=" << b.ready
              << "  understood=" << output.accepted << "  start=" << output.before << "  expected=" << output.goal
              << "  known=" << output.known << "  end=" << output.after << "  motor=" << output.work.stream
              << "  strokes=" << output.work.size << "  word_plan=" << output.description.stream
              << "  said=" << output.output.n << "  informed=" << output.articulated << "  delivered=" << output.heard
              << "  consultations=" << output.consulted << "  evidence=" << b.ground << "  relation=" << output.choice
              << "  incurred=" << output.tutor << "  spent=" << output.paid[0] + output.paid[1] + output.paid[2]
              << "  mouth=" << output.food[0] + output.food[1] + output.food[2] << "  borrowed=" << output.borrowed
              << "  writes=" << output.edits << "  viewed=" << output.samples << "  heard_words=" << b.heard_words
              << "  source=" << b.source << "  tutor=" << b.tutor << "  stage0=" << b.paint[0]
              << "  stage1=" << b.paint[1] << "  stage2=" << b.paint[2] << '\n';
    if (output.consulted)
        for (Word i = 0; i < output.consulted; ++i)
            std::cout << "braid_show=" << i + 1 << "  start=" << output.probes[i] << "  end=" << output.effects[i]
                      << '\n';
    return true;
}
