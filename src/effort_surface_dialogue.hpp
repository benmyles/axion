void surface_questions(std::istream &in) {
    std::string path;
    std::getline(in >> std::ws, path);
    std::ifstream input(path);
    if (!input)
        throw std::invalid_argument("cannot open physical surface questions");
    struct Ref {
        std::string arm, kind, word[2];
    };
    std::vector<Ref> refs;
    std::vector<effort::SurfaceQuestion> questions;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line.front() == '#')
            continue;
        std::istringstream row(line);
        std::string part, extra;
        auto field = [&]() {
            if (!std::getline(row, part, '|'))
                throw std::invalid_argument("incomplete surface question");
            return std::istringstream(part);
        };
        auto stage = field(), p = field(), q = field(), key = field();
        effort::SurfaceQuestion ask{};
        Ref ref{};
        if (!(stage >> ask.seed >> ask.mode >> ask.turns) || stage >> extra || ask.mode >= effort::cause_count ||
            ask.turns > 2 || !(key >> ref.arm >> ref.kind >> ref.word[0] >> ref.word[1]) || key >> extra ||
            row >> extra || questions.size() >= 1048576)
            throw std::invalid_argument("invalid surface scene");
        ask.first = plan_words(p);
        ask.second = plan_words(q);
        questions.push_back(ask);
        refs.push_back(ref);
    }
    if (questions.empty())
        throw std::invalid_argument("empty list of surfaces");
    Word n = questions.size(), size = std::min(4096u, n), orgs = organs();
    auto rule = local.referent.genes;
    Temporary stimuli(s.d, Count(size) * sizeof(questions[0])),
        result(s.d, Count(size) * sizeof(effort::SurfaceAnswer));
    std::vector<effort::SurfaceAnswer> reports(size);
    Function f{};
    s.bind(f, "effort_surface_questions", Image::siphons);
    std::map<std::pair<std::string, std::string>, std::array<Count, 4>> tally;
    auto start = std::chrono::steady_clock::now();
    for (Word off = 0; off < n; off += size) {
        Word count = std::min(size, n - off);
        s.d.check(s.d.upload(stimuli.ptr, questions.data() + off, Count(count) * sizeof(questions[0])),
                  "present independent slats and spoken hypothetical actions");
        void *args[]{&effort_brain.ptr, &minds.ptr, &words.ptr, &place_words.ptr, &rule, &stimuli.ptr,
                     &result.ptr,       &count,     &orgs};
        s.d.check(s.d.launch(f, (count + 127) / 128, 1, 1, 128, 1, 1, 0, nullptr, args, nullptr),
                  "predict an alteration from observed rotations");
        s.d.check(s.d.download(reports.data(), result.ptr, Count(count) * sizeof(reports[0])),
                  "hear the current and hypothetical faces");
        for (Word i = 0; i < count; ++i) {
            auto &r = reports[i];
            auto &ref = refs[i + off];
            for (Word j = 0; j < 2; ++j) {
                auto &a = j ? r.second : r.first;
                auto word = a.word ? acoustic::spell(a.word) : "_";
                auto &t = tally[{ref.arm, j ? "after_work" : ref.kind}];
                ++t[0];
                t[1] += ref.word[j] == word;
                t[2] += a.accepted;
                t[3] += r.paid;
                if (i + off < 6)
                    std::cout << "effort_shape_word=" << i + off << "  turn=" << j << "  said=" << word
                              << "  before=" << r.before << "  after=" << r.after
                              << "  demonstrations=" << r.demonstrations << "  cost=" << r.paid << '\n';
            }
        }
    }
    for (auto &[key, t] : tally)
        std::cout << "effort_shape_language=" << key.first << "  task=" << key.second << "  trials=" << t[0]
                  << "  correct=" << t[1] << "  spoken=" << t[2] << "  paid=" << t[3] << '\n';
    double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "effort_surface_dialogues=" << n << "  questions=" << 2 * Count(n) << "  seconds=" << dt
              << "  questions_per_s=" << 2. * n / dt << std::endl;
}
