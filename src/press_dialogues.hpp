// The keys and expected words stay in this grading process. Each CUDA lane is
// a fresh copy of the pupil and runs the same wheel actions as an interactive session.
void wheel_trials(std::istream &input) {
    std::string file;
    std::getline(input >> std::ws, file);
    std::ifstream data(file);
    if (!data)
        throw std::invalid_argument("open the file of pressure questions");
    struct Key {
        std::string arm, task, expected[2];
    };
    std::vector<Key> keys;
    std::vector<press::OralTest> tests;
    std::string line;
    const std::array<std::string_view, 6> arms{"both", "prior", "no_sight", "no_taste", "no_topic", "fixed"};
    auto strip = [](std::string value) {
        auto first = value.find_first_not_of(" \t\r"), last = value.find_last_not_of(" \t\r");
        return first == std::string::npos ? std::string{} : value.substr(first, last - first + 1);
    };
    while (std::getline(data, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::array<std::string, 5> columns;
        std::istringstream row(line);
        for (auto &column : columns) {
            if (!std::getline(row, column, '|'))
                throw std::invalid_argument("each pressure trial needs five pipe-delimited fields");
            column = strip(column);
        }
        std::string extra;
        if (std::getline(row, extra, '|'))
            throw std::invalid_argument("extra pressure trial field");
        std::istringstream spec(columns[0]), one(columns[1]), two(columns[2]);
        std::string seed, turn, mask;
        Key grade{};
        press::OralTest q{};
        if (!(spec >> seed >> turn >> mask >> grade.arm >> grade.task) || spec >> extra || tests.size() >= 262144)
            throw std::invalid_argument("use SEED TURN MASK ARM TASK before each pair of pressure questions");
        auto at = std::ranges::find(arms, grade.arm);
        if (at == arms.end())
            throw std::invalid_argument("unknown pressure control");
        q.seed = parse(seed, ~0u);
        q.rotate = parse(turn, 3);
        q.offered = parse(mask, 15);
        q.exposure = static_cast<press::Listening>(at - arms.begin());
        q.first = utterance(one);
        q.follow = utterance(two);
        if (!q.first.n || columns[3].empty() || columns[4].empty())
            throw std::invalid_argument("empty pressure question or grade");
        grade.expected[0] = columns[3];
        grade.expected[1] = columns[4];
        keys.push_back(std::move(grade));
        tests.push_back(q);
    }
    if (tests.empty())
        throw std::invalid_argument("no pressure questions supplied");
    Word n = tests.size(), a = partner().resident.animal.energy, b = local.resident.animal.energy,
         body = organs() | (Word(partner().resident.animal.body.limbs != 0) << 5);
    auto law = local.referent.genes;
    Temporary prompts(s.d, Count(n) * sizeof(tests[0])), replies(s.d, Count(n) * sizeof(press::OralResponse));
    s.d.check(s.d.upload(prompts.ptr, tests.data(), Count(n) * sizeof(tests[0])), "present unheard questions");
    auto start = std::chrono::steady_clock::now();
    Function probe{};
    s.bind(probe, "press_questions", Image::siphons);
    void *args[]{&assemblies.ptr, &minds.ptr, &words.ptr, &law, &prompts.ptr, &replies.ptr, &n, &a, &b, &body};
    s.d.check(s.d.launch(probe, (n + 127) / 128, 1, 1, 128, 1, 1, 0, nullptr, args, nullptr),
              "rear and question independent pressure readers");
    std::vector<press::OralResponse> results(n);
    s.d.check(s.d.download(results.data(), replies.ptr, Count(n) * sizeof(results[0])),
              "hear the pupil before scoring");
    double time = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::map<std::pair<std::string, std::string>, std::array<Count, 3>> counts;
    Count questions = 0, hits = 0;
    for (Word i = 0; i < n; ++i) {
        const auto &r = results[i];
        auto &key = keys[i];
        for (Word j = 0; j < (tests[i].follow.n ? 2u : 1u); ++j) {
            const auto &spoken = j ? r.follow : r.first;
            std::string heard;
            if (spoken.accepted)
                for (Word k = 0; k < spoken.said.n; ++k) {
                    if (k)
                        heard += '/';
                    heard += acoustic::spell(spoken.said.wave[k]);
                }
            if (heard.empty())
                heard = "_";
            bool correct = heard == key.expected[j];
            auto &t = counts[{key.arm, j ? "followup" : key.task}];
            ++questions;
            hits += correct;
            ++t[0];
            t[1] += correct;
            t[2] += spoken.accepted;
            std::cout << "press_sentence=" << i << "  turn=" << j << "  said=" << heard
                      << "  recognized=" << spoken.recognized << "  mask=" << spoken.candidates
                      << "  demonstrations=" << r.demonstrations << "  tastes=" << r.tastings << "  work=" << r.paid
                      << "  meals=" << r.food << '\n';
        }
    }
    for (const auto &[setting, t] : counts)
        std::cout << "press_language=" << setting.first << "  task=" << setting.second << "  questions=" << t[0]
                  << "  correct=" << t[1] << "  voiced=" << t[2] << '\n';
    std::cout << "press_dialogues=" << questions << "  correct=" << hits << "  stations=" << n << "  seconds=" << time
              << "  questions_per_s=" << questions / time << std::endl;
}
