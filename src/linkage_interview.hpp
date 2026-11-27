#include "linkage_dialogues.hpp"
bool rack_command(const std::string &token, std::istream &input, const std::string &line) {
    if (token != "/rack" && !at_rack)
        return false;
    if (token == "/ford" || token == "/marsh" || token == "/reed" || token == "/wheel" || token == "/leave-rack") {
        at_rack = false;
        return token == "/leave-rack";
    }
    if (!rack_brain.ptr)
        throw std::invalid_argument("the rack requires an animal with material memory and a public word organ");
    if (token == "/rack-trials") {
        rack_trials(input);
        return true;
    }
    linkage::Task job;
    Word a = 0, b = 4;
    effort::Speech speech{};
    std::string first, second, extra;
    if (token == "/rack" || token == "/rack-layout") {
        if (!(input >> first))
            throw std::invalid_argument("use /rack SEED [PIECES] or /rack-layout FIXTURE [PIECES]");
        a = parse(first, ~0u);
        if (input >> second)
            b = parse(second, 4);
        if (!b)
            throw std::invalid_argument("choose one to four pieces");
        job = token == "/rack" ? linkage::enter_rack : linkage::exchange_rig;
        at_rack = open = true;
        at_ford = at_marsh = at_wheel = false;
    } else if (token == "/rack-study" || token == "/rack-explore" || token == "/rack-meal") {
        if (!(input >> first))
            throw std::invalid_argument("give the number of demonstrations, practice strokes or carbon units");
        job = token == "/rack-study"     ? linkage::show_practice
              : token == "/rack-explore" ? linkage::own_practice
                                         : linkage::drink;
        a = parse(first, job == linkage::show_practice ? 8 : 1024);
    } else if (token == "/rack-order")
        job = linkage::read_order;
    else if (token == "/rack-pass") {
        job = linkage::pass_words;
        if (input >> first) {
            if (first != "muted" && first != "shifted")
                throw std::invalid_argument("use /rack-pass [muted|shifted]");
            a = first == "muted" ? 1 : 2;
        }
    } else if (!token.empty() && token.front() != '/') {
        job = linkage::request_words;
        std::istringstream sounds(line);
        speech = plan_words(sounds);
        if (auto *j = journal::current())
            j->line("linkage_utterance=heard  text=" + line);
    } else
        return false;
    if (input >> extra && job != linkage::request_words)
        throw std::invalid_argument("unexpected rack argument");
    auto &agent = job == linkage::pass_words ? partner() : local;
    auto &animal = agent.resident.animal;
    Word senses = Word(animal.body.eyes != 0) | (Word(animal.body.limbs != 0) << 1) | (Word(animal.body.pores != 0) << 2), purse = animal.energy;
    if (!animal.body.pores && job == linkage::drink)
        a = 0;
    std::array<linkage::Tongue, 2> tongues{};
    for (Word i = 0; i < 2; ++i) {
        auto &p = i ? partner() : local;
        tongues[i] = {p.native_words, p.path_speech.words, p.calls.animal, p.referent.genes};
    }
    s.d.check(s.d.upload(rack_voices.ptr, tongues.data(), sizeof(tongues)),
              "provide independent acoustic and motor organs");
    void *args[]{&rack.ptr, &rack_brain.ptr, &rack_voices.ptr, &job,    &a,
                 &b,        &speech,         &purse,           &senses, &rack_reply.ptr};
    s.d.check(s.d.launch(rack_act, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "work the apparatus and listen through public sound");
    linkage::Rack r{};
    linkage::Uttered o{};
    s.d.check(s.d.download(&o, rack_reply.ptr, sizeof(o)), "hear the work sequence");
    s.d.check(s.d.download(&r, rack.ptr, sizeof(r)), "inspect the actual apparatus");
    if (r.stock + r.swallowed != 256 || r.tutor + r.demonstrated != 1024 || o.spent > purse ||
        purse - o.spent + o.food > 65535)
        throw std::logic_error("unbalanced rack accounts");
    animal.energy = purse - o.spent + o.food;
    animal.meals += o.food;
    animal.diet[1] += o.food;
    s.d.check(s.d.download(&local.mechanism, rack_brain.ptr, sizeof(local.mechanism)),
              "retain personally acquired material contacts");
    if (job == linkage::request_words || job == linkage::read_order) {
        std::string answer;
        for (Word i = 0; i < o.words.n; ++i) {
            if (i)
                answer += ' ';
            answer += acoustic::spell(o.words.sounds[i]);
        }
        std::cout << (job == linkage::read_order ? "work order> " : "animal> ") << (answer.empty() ? "?" : answer)
                  << '\n';
    }
    std::cout << "linkage_dialogue=" << Word(job) << "  world=" << r.world << "  fixture=" << r.fixture
              << "  parts=" << r.bench.rig.parts << "  materials=" << r.bench.rig.glyphs
              << "  joints=" << r.bench.rig.joints << "  grip=" << r.bench.rig.handles
              << "  sockets=" << r.bench.sockets << "  labels=" << r.labels << "  buttons=" << r.buttons
              << "  reader_order=" << r.reader << "  start=" << o.start << "  after=" << o.after
              << "  goal=" << o.goal << "  care=" << o.care << "  physical_lock=" << r.bench.lock.faces
              << "  blocked=" << r.block << "  understood=" << o.understood << "  planned=" << o.planned
              << "  executed=" << o.executed << "  alignment=" << o.aligned << "  words=" << o.words.n
              << "  strokes=" << o.steps << "  fitted=" << o.inserted << "  spool=" << r.bench.spool
              << "  heat=" << o.spent << "  carbon=" << o.food << "  stock=" << r.stock
              << "  reserve=" << animal.energy << "  local_low=" << o.local_motors << "  world_low=" << o.world_motors
              << "  local_high=" << o.local_tail << "  world_high=" << o.world_tail << "  observed=" << o.shown
              << "  edits=" << o.associations << "  tutor_paid=" << r.demonstrated << "  refused=" << o.refused
              << std::endl;
    return true;
}
