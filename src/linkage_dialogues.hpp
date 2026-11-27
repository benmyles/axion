void rack_trials(std::istream &is) {
    std::string filename, artifact, extra;
    if (!(is >> std::quoted(filename) >> std::quoted(artifact)) || is >> extra)
        throw std::invalid_argument("use /rack-trials INPUT TRACE");
    std::ifstream input(filename);
    if (!input)
        throw std::invalid_argument("cannot open rack questions");
    if (std::filesystem::exists(artifact))
        throw std::invalid_argument("the experiment needs a new trace path");
    struct Key {
        std::string arm;
        Word mask, colors, limit;
    };
    std::vector<Key> keys;
    std::vector<linkage::Request> questions;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line.front() == '#')
            continue;
        std::istringstream row(line);
        std::string f;
        auto field = [&] {
            if (!std::getline(row, f, '|'))
                throw std::invalid_argument("missing work-order field");
            return std::istringstream(f);
        };
        auto env = field(), first = field(), second = field(), why = field(), grade = field();
        Key k{};
        linkage::Request q{};
        if (!(env >> q.world >> q.fixture >> q.width >> q.tuition >> q.wire) || q.width < 1 || q.width > 4 ||
            q.tuition > 8 || q.wire > 3 || env >> extra || !(grade >> k.arm >> k.mask >> k.colors >> k.limit) ||
            k.mask > 255 || k.colors > 255 || (k.limit > 14 && k.limit != 255) || grade >> extra || row >> extra ||
            questions.size() >= 262144)
            throw std::invalid_argument("invalid work-order trial");
        q.first = plan_words(first);
        q.revision = plan_words(second);
        q.why = plan_words(why);
        questions.push_back(q);
        keys.push_back(k);
    }
    if (questions.empty())
        throw std::invalid_argument("empty work-order study");
    std::ofstream trace(artifact);
    if (!trace)
        throw std::invalid_argument("cannot create the work-order trace");
    trace << "trial\tarm\tworld\tfixture\twidth\tstart\tfinal\tgoal\tcare\tjoints\tglyphs\tgrip\tblocked\tlabels\tbut"
             "tons\treader\texecuted\tstrokes\tinserted\tpaid\tfood\tlocal_low\tlocal_high\tworld_low\tworld_"
             "high\tfirst\trevised\treason\theard\n";
    Word n = questions.size(), batch = std::min(512u, n);
    std::array<linkage::Tongue, 2> tongues{};
    Word senses = 0;
    for (Word i = 0; i < 2; ++i) {
        auto &a = i ? partner() : local;
        tongues[i] = {a.native_words, a.path_speech.words, a.calls.animal, a.referent.genes};
        auto &body = a.resident.animal.body;
        senses |= (Word(!!body.eyes) | (Word(!!body.limbs) << 1) | (Word(!!body.pores) << 2)) << (3 * i);
    }
    s.d.check(s.d.upload(rack_voices.ptr, tongues.data(), sizeof(tongues)), "place the two independent vocabularies");
    Word purse = partner().resident.animal.energy;
    Temporary in(s.d, batch * sizeof(questions[0])), out(s.d, batch * sizeof(linkage::Conversation));
    std::vector<linkage::Conversation> result(batch);
    Function fn{};
    s.bind(fn, "linkage_meetings", Image::siphons);
    std::map<std::string, std::array<Count, 11>> score;
    auto write_words = [](const effort::Speech &phrase) {
        std::string z;
        for (Word i = 0; i < phrase.n; ++i) {
            if (i)
                z += ' ';
            z += phrase.sounds[i] ? acoustic::spell(phrase.sounds[i]) : "_";
        }
        return z;
    };
    auto start = std::chrono::steady_clock::now();
    for (Word at = 0; at < n; at += batch) {
        Word count = std::min(n - at, batch);
        s.d.check(s.d.upload(in.ptr, questions.data() + at, count * sizeof(questions[0])),
                  "send public questions and fixture settings");
        void *args[]{&rack_brain.ptr, &rack_voices.ptr, &in.ptr, &count, &purse, &senses, &out.ptr};
        s.d.check(s.d.launch(fn, (count + 63) / 64, 1, 1, 64, 1, 1, 0, nullptr, args, nullptr),
                  "learn from separate handling and exchange acoustic instructions");
        s.d.check(s.d.download(result.data(), out.ptr, count * sizeof(result[0])),
                  "collect real movements of the addressee");
        for (Word i = 0; i < count; ++i) {
            auto &r = result[i];
            auto &p = r.peer;
            auto &k = keys[at + i];
            auto &q = questions[at + i];
            auto &v = score[k.arm];
            bool done = p.executed && !((p.after ^ k.colors) & k.mask),
                 asked = (p.care == k.mask && !((p.goal ^ k.colors) & k.mask));
            bool changed = p.start != p.after;
            v[0]++;
            v[1] += k.limit < 255;
            v[2] += done;
            v[3] += done && p.spent == k.limit;
            v[4] += p.executed;
            v[5] += p.spent;
            v[6] += done && p.inserted;
            v[7] += done && p.steps >= 3;
            v[8] += asked;
            v[9] += r.reason.words.n != 0;
            v[10] += done && changed;
            auto first = write_words(r.first.words), revised = write_words(r.amended.words),
                 reason = write_words(r.reason.words), heard = write_words(p.words);
            trace << at + i << '\t' << k.arm << '\t' << q.world << '\t' << q.fixture << '\t' << q.width << '\t'
                  << p.start << '\t' << p.after << '\t' << p.goal << '\t' << p.care << '\t' << r.joints << '\t'
                  << r.glyphs << '\t' << r.grip << '\t' << r.barred << '\t' << r.labels << '\t' << r.buttons << '\t'
                  << r.order << '\t' << p.executed << '\t' << p.steps << '\t' << p.inserted << '\t' << p.spent << '\t'
                  << p.food << '\t' << p.local_motors << '\t' << p.local_tail << '\t' << p.world_motors << '\t'
                  << p.world_tail << '\t' << first << '\t' << revised << '\t' << reason << '\t' << heard << '\n';
            if (i < 2 && at == 0)
                std::cout << "linkage_instruction=" << at + i << "  strokes=" << p.steps << "  work=" << p.spent
                          << "  alignment=" << done << "  seed=" << q.world << "  sounds=" << heard << '\n';
        }
    }
    trace.close();
    if (!trace)
        throw std::runtime_error("could not finish the public instruction trace");
    for (auto &[arm, v] : score)
        std::cout << "linkage_language=" << arm << "  trials=" << v[0] << "  possible=" << v[1]
                  << "  aligned=" << v[2] << "  cheapest=" << v[3] << "  executed=" << v[4] << "  heat=" << v[5]
                  << "  fitted=" << v[6] << "  multistep=" << v[7] << "  request_correct=" << v[8]
                  << "  reasons=" << v[9] << "  changed=" << v[10] << '\n';
    auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "linkage_meetings=" << n << "  seconds=" << seconds << "  meetings_per_s=" << n / seconds
              << std::endl;
}
