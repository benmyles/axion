static effort::Speech plan_words(std::istream &is) {
    effort::Speech s{};
    std::string word;
    while (is >> word) {
        if (s.n == std::size(s.sounds))
            throw std::invalid_argument("a verbal offer has at most 32 sounds");
        if (word.back() == '?' || word.back() == '.' || word.back() == ',')
            word.pop_back();
        for (auto &c : word)
            if (c >= 'A' && c <= 'Z')
                c += 32;
        if (word.empty())
            throw std::invalid_argument("empty sound in a verbal offer");
        s.sounds[s.n++] = acoustic::pulse(word);
    }
    return s;
}
#include "effort_surface_dialogue.hpp"
void ford_tests(std::istream &input) {
    std::string file;
    std::getline(input >> std::ws, file);
    std::ifstream stream(file);
    if (!stream)
        throw std::invalid_argument("cannot open the route conversation trials");
    struct Grade {
        std::string arm, task, reference[4];
    };
    std::vector<Grade> keys;
    std::vector<effort::Dialogue> trials;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line.front() == '#')
            continue;
        std::istringstream row(line);
        std::string section, extra;
        auto field = [&]() {
            if (!std::getline(row, section, '|'))
                throw std::invalid_argument("incomplete verbal offer");
            return std::istringstream(section);
        };
        auto head = field();
        effort::Dialogue t{};
        if (!(head >> t.seed >> t.mode >> t.turn) || (t.mode & 15) >= effort::internal_count || t.mode > 31 ||
            t.turn > 3)
            throw std::invalid_argument("invalid crossing setup");
        for (auto &b : t.real)
            if (!(head >> b.food >> b.work >> b.terminal) || b.terminal > 15)
                throw std::invalid_argument("invalid physical offer");
        if (head >> extra)
            throw std::invalid_argument("trailing crossing settings");
        auto first = field(), why = field(), edit = field(), repeat = field(), grade = field();
        t.first = plan_words(first);
        t.why = plan_words(why);
        t.edit = plan_words(edit);
        t.repeat = plan_words(repeat);
        Grade g{};
        if (!(grade >> g.arm >> g.task >> g.reference[0] >> g.reference[1] >> g.reference[2] >> g.reference[3]) ||
            grade >> extra || row >> extra || trials.size() >= 1048576 || !t.first.n)
            throw std::invalid_argument("invalid route question references");
        trials.push_back(t);
        keys.push_back(g);
    }
    if (trials.empty())
        throw std::invalid_argument("empty route trial file");
    Word n = trials.size(), body = organs(), batch = std::min(2048u, n);
    auto law = local.referent.genes;
    Temporary in(s.d, Count(batch) * sizeof(trials[0])), out(s.d, Count(batch) * sizeof(effort::Transcript));
    Function f{};
    s.bind(f, "effort_dialogues", Image::siphons);
    std::vector<effort::Transcript> result(batch);
    std::map<std::pair<std::string, std::string>, std::array<Count, 4>> grades;
    Count paid = 0, swallowed = 0, walked = 0;
    auto start = std::chrono::steady_clock::now();
    for (Word first = 0; first < n; first += batch) {
        Word count = std::min(batch, n - first);
        s.d.check(s.d.upload(in.ptr, trials.data() + first, Count(count) * sizeof(trials[0])),
                  "present unheard choices and matched real routes");
        void *args[]{&effort_brain.ptr, &minds.ptr, &words.ptr, &place_words.ptr, &law, &in.ptr,
                     &out.ptr,          &count,     &body};
        s.d.check(s.d.launch(f, (count + 127) / 128, 1, 1, 128, 1, 1, 0, nullptr, args, nullptr),
                  "understand and compare two verbal prospects");
        s.d.check(s.d.download(result.data(), out.ptr, Count(count) * sizeof(result[0])),
                  "transcribe choices and physical work");
        for (Word i = 0; i < count; ++i) {
            auto &trans = result[i];
            auto &key = keys[first + i];
            paid += trans.action.paid;
            swallowed += trans.action.food;
            walked += trans.action.reached;
            for (Word j = 0; j < 4; ++j) {
                auto a = trans.replies[j];
                auto word = a.word ? acoustic::spell(a.word) : "_";
                auto &t = grades[{key.arm, j == 0 ? key.task : j == 1 ? "reason" : j == 2 ? "edited" : "repeat"}];
                ++t[0];
                t[1] += ("," + key.reference[j] + ",").find("," + word + ",") != std::string::npos;
                t[2] += a.accepted;
                t[3] += a.decisive && a.accepted;
                if (first + i < 12)
                    std::cout << "effort_sentence=" << first + i << "  turn=" << j << "  said=" << word
                              << "  choice=" << a.choice << "  feature=" << a.feature << "  left=" << a.value[0]
                              << "  right=" << a.value[1] << "  decisive=" << a.decisive
                              << "  paid=" << trans.action.paid << "  eaten=" << trans.action.food << '\n';
            }
        }
    }
    for (const auto &[k, g] : grades)
        std::cout << "effort_language=" << k.first << "  task=" << k.second << "  trials=" << g[0]
                  << "  correct=" << g[1] << "  spoken=" << g[2] << "  decisive=" << g[3] << '\n';
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "effort_dialogues=" << n << "  turns=" << Count(n) * 4 << "  lived=" << walked << "  paid=" << paid
              << "  food=" << swallowed << "  seconds=" << seconds << "  turns_per_s=" << 4. * n / seconds << std::endl;
}
bool ford_command(const std::string &token, std::istream &input, const std::string &line) {
    if (token != "/ford" && !at_ford)
        return false;
    if (!effort_brain.ptr)
        throw std::invalid_argument("the ford needs a resident with spatial speech and effort tissue");
    if (token == "/marsh" || token == "/reed" || token == "/wheel" || token == "/leave-ford") {
        at_ford = false;
        return token == "/leave-ford";
    }
    if (token == "/ford-surfaces") {
        surface_questions(input);
        return true;
    }
    if (token == "/ford-tests") {
        ford_tests(input);
        return true;
    }
    if (token == "/ford-caption") {
        auto sound = plan_words(input);
        if (sound.n != 1)
            throw std::invalid_argument("demonstrate one public operator at a time");
        Word yes = 0;
        auto law = local.referent.genes;
        void *args[]{&place_words.ptr, &law, &ford.ptr, &sound.sounds[0], &ford_flag.ptr};
        s.d.check(s.d.launch(ford_teach, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                  "ground a word in the last visible tray change");
        s.d.check(s.d.download(&yes, ford_flag.ptr, 4), "read the observed form");
        s.d.check(s.d.download(&local.path_speech.words, place_words.ptr, sizeof(local.path_speech.words)),
                  "retain operator associations in the existing spatial word bank");
        std::cout << "effort_caption=" << yes << "  forms=" << local.path_speech.words.count << std::endl;
        return true;
    }
    Word action = ~0u, p = 0, q = 0;
    effort::Bundle lane{};
    std::string a, b, d, extra;
    if (token == "/ford") {
        if (!(input >> a))
            throw std::invalid_argument("use /ford SEED");
        p = parse(a, ~0u);
        action = 0;
        at_ford = true;
        at_wheel = at_marsh = false;
        open = true;
    } else if (token == "/ford-fruit" || token == "/ford-ground")
        action = token == "/ford-fruit" ? 1u : 2u;
    else if (token == "/ford-weather") {
        if (!(input >> a >> b))
            throw std::invalid_argument("use /ford-weather FRUIT_SHIFT GROUND_SHIFT");
        action = 3;
        p = parse(a, 3);
        q = parse(b, 3);
    } else if (token == "/ford-turn") {
        if (!(input >> a))
            throw std::invalid_argument("use /ford-turn QUARTERS");
        action = 4;
        p = parse(a, 3);
    } else if (token == "/ford-lane") {
        if (!(input >> a >> b >> d >> extra))
            throw std::invalid_argument("use /ford-lane SIDE FOOD_COUNTS GROUND_COUNTS TERMINAL");
        p = parse(a, 1);
        lane = {parse(b, ~0u), parse(d, ~0u), parse(extra, 15)};
        action = 5;
    } else if (token == "/ford-show") {
        if (!(input >> a >> b))
            throw std::invalid_argument("use /ford-show EVENT COMPONENT");
        const std::array<std::string, 7> gestures{"switch", "offer", "contrast", "lift", "release", "surface", "roll"};
        auto it = std::find(gestures.begin(), gestures.end(), a);
        if (it == gestures.end())
            throw std::invalid_argument("unknown physical change");
        action = 6;
        p = std::distance(gestures.begin(), it);
        q = parse(b, 7);
    } else if (token == "/ford-run")
        action = 7;
    else if (token == "/ford-study")
        action = 8;
    else if (token == "/ford-roll") {
        if (!(input >> a))
            throw std::invalid_argument("use /ford-roll ACTUATOR");
        action = 9;
        p = parse(a, 1);
    } else if (!token.empty() && token.front() != '/') {
        std::istringstream phrase(line);
        auto speech = plan_words(phrase);
        Word senses = organs();
        auto law = local.referent.genes;
        void *args[]{&effort_brain.ptr, &minds.ptr, &words.ptr, &place_words.ptr, &law,
                     &ford_topic.ptr,   &speech,    &ford.ptr,  &senses,          &ford_reply.ptr};
        s.d.check(s.d.launch(ford_say, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                  "compare the described journeys and retain the reference");
        effort::Reply r{};
        s.d.check(s.d.download(&r, ford_reply.ptr, sizeof(r)), "listen to the prospective reply");
        auto word = r.word ? acoustic::spell(r.word) : "?";
        if (auto *j = journal::current())
            j->line("effort_prompt=heard  text=" + line);
        std::cout << "animal> " << word << "\neffort_reply=" << r.accepted << "  said=" << word << "  role=" << r.op
                  << "  choice=" << r.choice << "  feature=" << r.feature << "  decisive=" << r.decisive
                  << "  left_current=" << r.value[0] << "  right_current=" << r.value[1] << std::endl;
        if (r.op == effort::enact && r.accepted)
            action = 7;
        else
            return true;
    } else
        return false;
    if (action != 7 && input >> extra)
        throw std::invalid_argument("too many ford settings");
    Word body = organs(), budget = local.resident.animal.energy;
    void *args[]{&ford.ptr, &effort_brain.ptr, &ford_topic.ptr, &action, &p, &q, &lane, &budget, &body, &ford_bite.ptr};
    s.d.check(s.d.launch(ford_move, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "perform the work on real substrates and food sources");
    effort::Board table{};
    effort::Bite expense{};
    s.d.check(s.d.download(&table, ford.ptr, sizeof(table)), "audit the physical ford");
    s.d.check(s.d.download(&expense, ford_bite.ptr, sizeof(expense)), "read realised work and harvest");
    Word total = 0;
    for (Word stock : table.capital)
        total += stock;
    if (total + table.consumed + table.loose != 2048 || budget + expense.food < expense.paid ||
        budget + expense.food - expense.paid > 65535)
        throw std::logic_error("ford energy imbalance");
    auto &animal = local.resident.animal;
    animal.energy = budget + expense.food - expense.paid;
    animal.meals += expense.food;
    animal.diet[1] += expense.food;

    s.d.check(s.d.download(&local.prospects, effort_brain.ptr, sizeof(local.prospects)),
              "retain learned yield and work circuits");
    std::cout << "effort_contact=" << action << "  frame=" << table.head << "  ray=" << table.ray
              << "  gesture=" << table.shown << "  paid=" << expense.paid
              << "  surface=" << press::item(table.pigments, table.pose)
              << "  first_handle=" << press::item(table.handle, 0) << "  second_handle=" << press::item(table.handle, 1)
              << "  eaten=" << expense.food << "  walked=" << expense.steps << "  arrived=" << expense.reached
              << "  stock=" << total << "  reserve=" << animal.energy << std::endl;
    return true;
}
