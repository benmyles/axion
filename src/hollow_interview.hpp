// Included in SiphonInterview. The grade strings never cross the device boundary.
std::string heard(const siphon::Report &r, const char *space = " ") const {
    std::string text;
    if (r.accepted)
        for (Word j = 0; j < r.said.n; ++j) {
            if (j)
                text += space;
            text += acoustic::spell(r.said.wave[j]);
        }
    return text.empty() ? "_" : text;
}
void marsh_tests(std::istream &input) {
    std::string path;
    std::getline(input >> std::ws, path);
    std::ifstream data(path);
    if (!data)
        throw std::invalid_argument("cannot open marsh questions");
    struct References {
        std::string arm, task, words[3];
    };
    std::vector<References> refs;
    std::vector<hollow::Question> tests;
    std::string line;
    while (std::getline(data, line)) {
        if (line.empty() || line.front() == '#')
            continue;
        std::istringstream stream(line);
        std::string section, extra;
        auto field = [&]() {
            if (!std::getline(stream, section, '|'))
                throw std::invalid_argument("incomplete marsh question");
            return std::istringstream(section);
        };
        auto header = field();
        hollow::Question q{};
        if (!(header >> q.seed >> q.conditions >> q.rotation >> q.hue >> q.ray >> q.distance >> q.facing) ||
            header >> extra || q.conditions > 31 || (q.conditions & 15) >= hollow::trial_count || q.rotation > 3 ||
            q.hue > 3 || q.ray > 3 || q.distance > 24 || !q.distance || q.facing > 3)
            throw std::invalid_argument("invalid physical marsh placement");
        auto first = field(), next = field(), effect = field(), key = field();
        q.first = utterance(first);
        q.follow = utterance(next);
        q.effect = utterance(effect);
        References r{};
        if (!(key >> r.arm >> r.task >> r.words[0] >> r.words[1] >> r.words[2]) || key >> extra || stream >> extra ||
            !q.first.n || !q.follow.n || !q.effect.n || tests.size() >= 1048576)
            throw std::invalid_argument("invalid marsh transcription");
        tests.push_back(q);
        refs.push_back(r);
    }
    if (tests.empty())
        throw std::invalid_argument("empty marsh");
    Word n = tests.size(), body = organs();
    auto law = local.referent.genes;
    Function f{};
    s.bind(f, "hollow_questions", Image::siphons);
    std::map<std::pair<std::string, std::string>, std::array<Count, 3>> grades;
    Word batch = std::min(8192u, n);
    Temporary in(s.d, Count(batch) * sizeof(tests[0])), out(s.d, Count(batch) * sizeof(hollow::Answer));
    std::vector<hollow::Answer> results(batch);
    auto start = std::chrono::steady_clock::now();
    for (Word off = 0; off < n; off += batch) {
        Word len = std::min(batch, n - off);
        s.d.check(s.d.upload(in.ptr, tests.data() + off, Count(len) * sizeof(tests[0])),
                  "place independent environments and unheard questions");
        void *args[]{
            &spatial.ptr, &assemblies.ptr, &minds.ptr, &place_words.ptr, &words.ptr, &law, &in.ptr, &out.ptr, &len,
            &body};
        s.d.check(s.d.launch(f, (len + 127) / 128, 1, 1, 128, 1, 1, 0, nullptr, args, nullptr),
                  "ask where to go before and after turning");
        s.d.check(s.d.download(results.data(), out.ptr, Count(len) * sizeof(results[0])), "transcribe spatial answers");
        for (Word i = 0; i < len; ++i)
            for (Word j = 0; j < 3; ++j) {
                auto &k = refs[off + i];
                auto &a = results[i].spoken[j];
                auto phrase = heard(a, "/");
                auto &g = grades[{k.arm, j == 0 ? k.task : j == 1 ? "after_turn" : "old_sound"}];
                ++g[0];
                g[1] += ("," + k.words[j] + ",").find("," + phrase + ",") != std::string::npos;
                g[2] += a.accepted;
                if (i + off < 12)
                    std::cout << "hollow_sentence=" << off + i << "  turn=" << j << "  said=" << phrase
                              << "  recognized=" << a.recognized << "  candidates=" << a.candidates
                              << "  walked=" << results[i].cost << '\n';
            }
    }
    for (const auto &[k, g] : grades)
        std::cout << "hollow_language=" << k.first << "  task=" << k.second << "  questions=" << g[0]
                  << "  correct=" << g[1] << "  voiced=" << g[2] << '\n';
    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "hollow_dialogues=" << n << "  turns=" << Count(n) * 3 << "  seconds=" << elapsed
              << "  questions_per_s=" << 3. * n / elapsed << std::endl;
}
bool marsh_command(const std::string &token, std::istream &input, const std::string &line) {
    if (token != "/marsh" && !at_marsh)
        return false;
    if (!spatial.ptr)
        throw std::invalid_argument("the marsh needs a resident with spatial and material tissue");
    if (token == "/marsh-tests") {
        marsh_tests(input);
        return true;
    }
    if (token == "/leave-marsh" || token == "/reed" || token == "/wheel") {
        at_marsh = false;
        return token == "/leave-marsh";
    }
    auto law = local.referent.genes;
    if (token == "/marsh-caption" || token == "/marsh-example" || (!token.empty() && token[0] != '/')) {
        bool lesson = token.starts_with("/marsh-");
        Word example = token == "/marsh-example";
        siphon::Sentence sentence{};
        if (lesson) {
            std::string text;
            std::getline(input >> std::ws, text);
            auto bar = text.find('|');
            if (example && (bar == std::string::npos || text.find('|', bar + 1) != std::string::npos))
                throw std::invalid_argument("use /marsh-example QUESTION | ANSWER");
            std::istringstream prompt(text.substr(0, bar));
            sentence = utterance(prompt);
            if (example) {
                std::istringstream answer(text.substr(bar + 1));
                auto word = utterance(answer);
                if (word.n != 1 || sentence.n >= siphon::phrase_length || !word.wave[0])
                    throw std::invalid_argument("one witnessed answer is required");
                sentence.wave[sentence.n++] = word.wave[0];
            }
            void *args[]{&place_words.ptr, &words.ptr, &minds.ptr, &law,
                         &table.ptr,       &sentence,  &example,   &response.ptr};
            s.d.check(s.d.launch(teach_places, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                      "learn spoken forms from a visible placement");
            s.d.check(s.d.download(&local.path_speech.words, place_words.ptr, sizeof(local.path_speech.words)),
                      "retain spatial speech");
        } else {
            std::istringstream prompt(line);
            sentence = utterance(prompt);
            void *args[]{&spatial.ptr, &assemblies.ptr, &minds.ptr,       &place_words.ptr, &words.ptr,
                         &law,         &sentence,       &marsh_topic.ptr, &marsh.ptr,       &response.ptr};
            s.d.check(s.d.launch(say_place, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                      "speak from a remembered place or effect");
        }
        siphon::Report r{};
        s.d.check(s.d.download(&r, response.ptr, sizeof(r)), "hear a situated word");
        if (lesson)
            std::cout << "hollow_caption=" << r.accepted << "  example=" << example << "  forms=" << r.forms
                      << std::endl;
        else {
            if (auto *j = journal::current())
                j->line("hollow_prompt=heard  text=" + line);
            auto speech = heard(r);
            std::cout << "animal> " << (speech == "_" ? "?" : speech) << "\nhollow_reply=" << r.accepted
                      << "  said=" << speech << "  recognized=" << r.recognized << "  candidates=" << r.candidates
                      << std::endl;
        }
        return true;
    }
    struct Action {
        std::string_view name;
        hollow::Activity command;
        Word argc;
    };
    constexpr Action actions[]{{"/marsh", hollow::arrive, 1},         {"/marsh-survey", hollow::survey, 0},
                               {"/marsh-listen", hollow::hear, 0},    {"/marsh-taste", hollow::sip, 0},
                               {"/marsh-tide", hollow::wheel, 1},     {"/marsh-raft", hollow::raft, 0},
                               {"/marsh-look", hollow::recognize, 0}, {"/marsh-start", hollow::departure, 0},
                               {"/marsh-face", hollow::face, 1},      {"/marsh-step", hollow::lead_step, 2},
                               {"/marsh-show", hollow::exhibit, 3},   {"/marsh-fetch", hollow::take_home, 0},
                               {"/marsh-grasp", hollow::grasp, 1},    {"/marsh-put", hollow::put_down, 0}};
    for (auto a : actions)
        if (token == a.name) {
            Word argsv[3]{};
            std::string txt;
            for (Word i = 0; i < a.argc; ++i) {
                if (!(input >> txt))
                    throw std::invalid_argument("missing marsh setting");
                argsv[i] = parse(txt, a.command == hollow::arrive                ? ~0u
                                      : a.command == hollow::lead_step && i == 1 ? 24u
                                                                                 : 3u);
            }
            if (input >> txt)
                throw std::invalid_argument("too many marsh settings");
            if (a.command == hollow::arrive) {
                at_marsh = true;
                at_wheel = false;
                open = true;
            }
            auto &resident = local.resident.animal;
            Word budget = resident.energy, body = organs(), cmd = a.command;
            void *args[]{&marsh.ptr, &spatial.ptr, &assemblies.ptr, &table.ptr, &marsh_topic.ptr, &cmd,
                         &argsv[0],  &argsv[1],    &argsv[2],       &body,      &budget,          &marsh_contact.ptr};
            s.d.check(s.d.launch(marsh_act, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                      "move a body in the clearing");
            hollow::Contact r{};
            hollow::Marsh site{};
            s.d.check(s.d.download(&r, marsh_contact.ptr, sizeof(r)), "audit the physical contact");
            s.d.check(s.d.download(&site, marsh.ptr, sizeof(site)), "audit floor and stores");
            if (site.pieces.loose != (15u ^ (r.held < 4 ? 1u << r.held : 0u)) ||
                site.stock + site.consumed + site.loose != 512 || budget + r.food < r.work ||
                budget + r.food - r.work > 65535)
                throw std::logic_error("marsh carbon imbalance");
            resident.energy = budget + r.food - r.work;
            resident.diet[1] += r.food;
            resident.meals += r.food;
            s.d.check(s.d.download(&local.path_memory, spatial.ptr, sizeof(local.path_memory)),
                      "retain personal place cells");
            s.d.check(s.d.download(&local.ceramic.mind, assemblies.ptr, sizeof(local.ceramic.mind)),
                      "retain the material lesson");
            std::cout << "hollow_contact=" << cmd << "  x=" << site.body.x << "  y=" << site.body.y
                      << "  head=" << site.body.head << "  held=" << r.held << "  walked=" << r.walked
                      << "  hue=" << r.hue << "  seen=" << r.view << "  food=" << r.food << "  work=" << r.work
                      << "  floor_mask=" << site.pieces.loose << "  stock=" << site.stock
                      << "  corrected=" << r.corrected << "  delivered=" << r.returned
                      << "  cells=" << Word(local.path_memory.mind.used) << "  reserve=" << resident.energy
                      << std::endl;
            return true;
        }
    return false;
}
