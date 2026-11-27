// Public words are transcribed on the host. The two residents perceive,
// choose, open the physical slit, learn and complete novel phrases on CUDA.
class SiphonInterview {
    Session &s;
    AgentImage &local, &upstream;
    Temporary minds, table, words, response, assemblies, slabs, hearing;
    Function touch{}, clay_reply{},press_move{},press_say{};
    Temporary wheel,subject,pressure;
    bool at_wheel=false,at_marsh=false;
    Temporary spatial,place_words,marsh,marsh_topic,marsh_contact;
    bool at_ford=false;
    Temporary effort_brain,ford,ford_topic,ford_reply,ford_bite,ford_flag;
    Function ford_move{},ford_say{},ford_teach{};
    bool at_rack=false;
    Temporary rack_brain,rack,rack_voices,rack_reply;
    Function rack_act{};
    bool at_studio=false;
    Temporary studio_brain,studio,studio_output;
    Function studio_act{};
    bool at_flume=false;Temporary reed_pair,flume,flume_voices,flume_reply;Function flume_act{};
    bool at_bank=false;Word bank_clock=0;Temporary bank_brain,bank_table,bank_voices,bank_reply;Function bank_act{};
    bool at_braid=false;std::optional<AgentImage> float_worker;Temporary float_head,float_tray,float_reply;Function float_act{},float_stream{},float_recall{};
    Temporary float_memories,float_answer,float_regulator;
    std::unique_ptr<Temporary> inlet_bench, inlet_receipt;
    Function inlet_action{}, cascade_act{};
    std::unique_ptr<Temporary> cascade_bench, cascade_receipt;
    Word cascade_beds = 0;
    bool cascade_listening = false, cascade_uses_surface = false;
    std::unique_ptr<Temporary> audience_table, audience_couple, audience_receipt;
    Function social_action{}, social_words{};
    bool audience_open = false;
    std::unique_ptr<Temporary> story_table,story_brains,story_bodies,story_reply;
    std::array<utterance::Saved,2> story_cache{};
    bool story_ready=false;
    Function story_action{},cap_handle{};
    std::unique_ptr<Temporary> cap_table,cap_brains,cap_bodies,cap_reply;
    std::array<capillary::Saved,2> cap_cache{};bool cap_ready=false;
    Function regulated_recall{},regulated_practice{};DevicePtr somatic_bands[2]{};
    Word lesson_modes[3]{apprentice::unrestricted,apprentice::unrestricted,apprentice::unrestricted};
    Word report_shutters=4095,report_channel=1,report_witness=1;
    std::array<Count,3> report_names{};bool choose_reporter=false;
    std::array<braid::Speech,3> demo_names{};
    Function marsh_act{},say_place{},teach_places{};
    Word offers=15;
    std::optional<AgentImage> visitor;
    std::optional<clay::Kit> display;
    bool holding = false, transparent = false;
    Word clay_round = 0, clay_budget = 4, scene_seed = 0;
    AgentImage &partner() { return visitor ? *visitor : upstream; }
    const AgentImage &partner() const { return visitor ? *visitor : upstream; }
    Function exchange{};
    bool open = false, plastic = true;
    Word wire = 0;
    static Word parse(std::string_view token, Word upper) {
        Word v = 0;
        auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), v);
        if (error != std::errc{} || end != token.data() + token.size() || v > upper)
            throw std::invalid_argument("invalid reed-table quantity");
        return v;
    }
    static std::string tone(Word id) {
        constexpr const char *names[]{"ka", "li", "mu", "so", "_"};
        return names[id < 4 ? id : 4];
    }
    static siphon::Sentence utterance(std::istream &stream) {
        siphon::Sentence phrase{};
        std::string token;
        while (stream >> token) {
            if (phrase.n == siphon::phrase_length)
                throw std::invalid_argument("a reed sentence has at most eight sounds");
            if (token == "?") {
                phrase.wave[phrase.n++] = 0;
                continue;
            }
            if (!token.empty() && (token.back() == '.' || token.back() == ',' || token.back() == '?'))
                token.pop_back();
            for (auto &k : token)
                if (k >= 'A' && k <= 'Z')
                    k += 32;
            phrase.wave[phrase.n++] = acoustic::pulse(token);
        }
        return phrase;
    }
    Word organs() const {
        const auto &one = partner().resident.animal.body, &two = local.resident.animal.body;
        return Word(one.pores != 0) | (Word(two.pores != 0) << 1) | (Word(two.limbs != 0) << 2) |
               (Word(one.eyes != 0) << 3) | (Word(two.eyes != 0) << 4);
    }
    static clay::Instrument cast(const std::string &file) {
        auto collection = clay::read_collection(file);
        clay::Instrument sample{};
        for (Word p = 0; p < 2; ++p) {
            sample.objects[p] = collection.objects[p];
            sample.glaze[p] = collection.glaze[p];
        }
        return sample;
    }
    void handle_clay(Word command, Word taps) {
        if (!holding)
            throw std::invalid_argument("place worked clay with /kiln FILE covered|visible");
        auto &body = local.resident.animal;
        Word seed = scene_seed + clay_round * 937, budget = body.energy, see = transparent, limbs = body.body.limbs,
             eyes = body.body.eyes;
        void *args[]{&minds.ptr, &assemblies.ptr, &table.ptr, &slabs.ptr, &command, &taps,
                     &seed,      &budget,         &see,       &limbs,     &eyes,    &hearing.ptr};
        s.d.check(s.d.launch(touch, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr), "feel an actual worked grain");
        clay::Hearing r{};
        s.d.check(s.d.download(&r, hearing.ptr, sizeof(r)), "read the paid acoustic evidence");
        body.energy -= r.strokes;
        s.d.check(s.d.download(&local.ceramic.mind, assemblies.ptr, sizeof(clay::Mind)),
                  "retain the reinstated memory");
        siphon::Pair actual{};
        s.d.check(s.d.download(&actual, minds.ptr, sizeof(actual)), "retain the active native projections");
        local.calls.animal = actual.b;
        if (command)
            ++clay_round;
        std::cout << "clay_contact=" << command << "  strokes=" << r.strokes << "  sounded_faces=" << r.mask
                  << "  pulses=" << r.acoustic << "  visible=" << see << "  glints=" << r.glints
                  << "  source_cell=" << r.context << "  available=" << r.alternatives
                  << "  distinct_from_query=" << r.non_target << "  reserve=" << body.energy << std::endl;
    }
#include "effort_interview.hpp"
#include "linkage_interview.hpp"
#include "practice_interview.hpp"
#include "flume_trials.hpp"
#include "flume_interview.hpp"
#include "backwater_interview.hpp"
#include "braid_interview.hpp"
#include "braid_report_interview.hpp"
#include "inlet_interview.hpp"
#include "cascade_interview.hpp"
#include "audience_interview.hpp"
#include "narrative_interview.hpp"
#include "capillary_interview.hpp"
#include "hollow_interview.hpp"
#include "press_dialogues.hpp"
#include "press_interview.hpp"
    bool workshop(const std::string &token, std::istream &input) {
        bool related = token == "/reed-visitor" || token == "/kiln" || token == "/knock" || token == "/study-clay" ||
                       token == "/clay-inquiry" || token == "/clay-visitors" || token == "/clay-tests";
        if (!related)
            return false;
        if (!assemblies.ptr)
            throw std::invalid_argument("a resident with context assemblies is required");
        std::string arg, extra;
        if (!(input >> std::quoted(arg)))
            throw std::invalid_argument("missing clay workshop argument");
        if (token == "/reed-visitor") {
            AgentImage next;
            next.read(arg);
            if (!next.calling)
                throw std::invalid_argument("the visitor has no native acoustic tissue");
            s.d.check(s.d.upload(minds.ptr, &next.calls.animal, sizeof(siphon::Animal)),
                      "place a new upstream partner");
            visitor = std::move(next);
            std::cout << "clay_visitor=arrived" << std::endl;
        } else if (token == "/kiln") {
            std::string mode;
            if (!(input >> mode) || (mode != "covered" && mode != "visible") || input >> extra)
                throw std::invalid_argument("use /kiln FILE covered|visible");
            auto surface = cast(arg);
            s.d.check(s.d.upload(slabs.ptr, &surface, sizeof(surface)), "place the local physical collection");
            holding = true;
            transparent = mode == "visible";
            clay_round = 0;
            handle_clay(0, 0);
        } else if (token == "/clay-inquiry") {
            clay_budget = parse(arg, 8);
            std::cout << "clay_inquiry=" << clay_budget << std::endl;
        } else if (token == "/clay-visitors") {
            clay::Kit shelf{};
            for (Word k = 0; k < 2; ++k) {
                auto stem = arg + (k ? "-b" : "-a");
                AgentImage visitor;
                visitor.read(stem + ".mind");
                if (!visitor.calling)
                    throw std::invalid_argument("both visitors need native acoustic tissue");
                shelf.instruments[k] = cast(stem + ".clay");
                shelf.actors[k] = visitor.calls.animal;
                shelf.energy[k] = visitor.resident.animal.energy;
                auto &b = visitor.resident.animal.body;
                shelf.organs[k] = Word(b.pores != 0) | (Word(b.eyes != 0) << 3);
            }
            display = shelf;
            std::cout << "clay_visitors=2" << std::endl;
        } else if (token == "/clay-tests") {
            batch_clay(arg);
        } else
            handle_clay(token == "/knock" ? 1 : 2, parse(arg, token == "/knock" ? 8 : 8192));
        return true;
    }
    void batch_clay(const std::string &file) {
        if (!display)
            throw std::invalid_argument("place the two external witnesses with /clay-visitors PREFIX");
        std::ifstream input(file);
        if (!input)
            throw std::invalid_argument("cannot open the cultural questions");
        std::vector<clay::Prompt> items;
        std::vector<std::pair<std::string, std::string>> references;
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty() || line[0] == '#')
                continue;
            auto sep = line.find('|');
            std::istringstream row(line.substr(0, sep));
            std::string k, v, m, g, wire, speaker, tile, taps, visible;
            std::string category, wanted;
            if (sep != std::string::npos) {
                std::istringstream ref(line.substr(sep + 1));
                ref >> category;
                std::getline(ref >> std::ws, wanted);
                if (wanted.empty())
                    throw std::invalid_argument("missing external reference");
                std::replace(wanted.begin(), wanted.end(), ' ', '/');
            }
            references.emplace_back(category, wanted);
            if (!(row >> k >> v >> m >> g >> wire >> speaker >> tile >> taps >> visible))
                throw std::invalid_argument(
                    "expected material scene, wire, visitor, object, taps, visibility and sentence");
            clay::Prompt item;
            item.problem = {utterance(row), parse(k, ~0u), parse(v, 255), parse(m, 3), parse(g, 3), parse(wire, 4)};
            if (!item.problem.words.n || items.size() >= 1048576)
                throw std::invalid_argument("invalid cultural sentence batch");
            item.actor = parse(speaker, 1);
            item.object = parse(tile, 1);
            item.taps = parse(taps, 8);
            item.visibility = parse(visible, 3);
            items.push_back(item);
        }
        if (items.empty())
            throw std::invalid_argument("empty cultural sentence batch");
        Word n = items.size(), budget = local.resident.animal.energy, body = organs();
        Temporary chart(s.d, sizeof(clay::Charts)), shelf(s.d, sizeof(*display)),
            questions(s.d, Count(n) * sizeof(items[0])), answers(s.d, Count(n) * sizeof(clay::Answer));
        s.d.check(s.d.upload(shelf.ptr, &*display, sizeof(*display)),
                  "place the two sets of real artifacts and witnesses");
        s.d.check(s.d.upload(questions.ptr, items.data(), Count(n) * sizeof(items[0])),
                  "place independent opaque problems");
        Function f{};
        s.bind(f, "clay_utterances", Image::siphons);
        auto lex = local.referent.genes;
        Function compile{};
        s.bind(compile, "clay_compile_one", Image::siphons);
        void *projection[]{&minds.ptr, &assemblies.ptr, &chart.ptr};
        s.d.check(s.d.launch(compile, 1, 1, 1, 1, 1, 1, 0, nullptr, projection, nullptr),
                  "compile the pupil's fixed public predictions");
        void *args[]{&minds.ptr,     &assemblies.ptr, &words.ptr, &lex,    &shelf.ptr, &chart.ptr,
                     &questions.ptr, &answers.ptr,    &n,         &budget, &body};
        auto start = std::chrono::steady_clock::now();
        s.d.check(s.d.launch(f, (n + 127) / 128, 1, 1, 128, 1, 1, 0, nullptr, args, nullptr),
                  "recognize and consult a physical witness");
        std::vector<clay::Answer> result(n);
        s.d.check(s.d.download(result.data(), answers.ptr, Count(n) * sizeof(result[0])),
                  "transcribe the full replies");
        double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        Count completed = 0, asked = 0, strokes = 0, resolved = 0, distinct = 0;
        struct Grade {
            Count trials = 0, right = 0, spoken = 0, taps = 0;
        };
        std::map<std::pair<std::string, std::string>, Grade> grading;
        for (Word i = 0; i < n; ++i) {
            auto &r = result[i].response;
            auto &e = result[i].ear;
            std::string said;
            if (r.accepted)
                for (Word j = 0; j < r.said.n; ++j) {
                    if (j)
                        said += '/';
                    said += acoustic::spell(r.said.wave[j]);
                }
            if (!references[i].second.empty()) {
                auto &x = items[i];
                bool other = x.actor != x.object;
                const char *arm = x.problem.wire == 2  ? ((x.visibility&2)?"silent_witness":"answer_shut")
                                  : (x.visibility & 1) ? (other ? "false_color" : "visible")
                                  : !x.taps            ? "untapped"
                                  : (x.visibility & 2) ? (other ? "foreign_self_directed" : "self_directed")
                                  : other              ? "other_object"
                                                       : "matching";
                auto &g = grading[{references[i].first, arm}];
                ++g.trials;
                g.spoken += r.accepted;
                g.taps += e.strokes;
                g.right += references[i].second == (said.empty() ? "_" : said);
            }
            completed += r.accepted;
            asked += r.queries;
            strokes += e.strokes;
            distinct += e.non_target;
            resolved += e.context < 4;
            std::cout << "clay_sentence=" << i << "  said=" << (said.empty() ? "_" : said)
                      << "  recognized=" << r.recognized << "  role=" << r.role << "  source_cell=" << e.context
                      << "  paid_taps=" << e.strokes << "  sounded=" << e.mask << "  questions=" << r.queries
                      << "  answers=" << r.answers << "  work=" << r.cost_a + r.cost_b + e.strokes
                      << "  carbon=" << r.food_a + r.food_b << '\n';
        }
        for (auto &[key, g] : grading)
            std::cout << "clay_conversation=" << key.second << "  target=" << key.first << "  trials=" << g.trials
                      << "  correct=" << g.right << "  voiced=" << g.spoken << "  paid_taps=" << g.taps << '\n';
        if (distinct != strokes)
            throw std::logic_error("a cultural query physically sounded its target");
        std::cout << "clay_dialogues=" << n << "  completed=" << completed << "  identified=" << resolved
                  << "  questions=" << asked << "  paid_taps=" << strokes << "  target_taps=0  seconds=" << elapsed
                  << "  utterances_per_s=" << n / elapsed << std::endl;
    }
    void batch(const std::string &path) {
        std::ifstream file(path);
        if (!file)
            throw std::invalid_argument("cannot open the reed sentences");
        std::vector<siphon::Test> cases;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line.front() == '#')
                continue;
            std::istringstream row(line);
            std::string k, light, filter, angle, wire;
            if (!(row >> k >> light >> filter >> angle >> wire))
                throw std::invalid_argument("expected seed, spectrum, filter, opening, wire and a sentence");
            auto phrase = utterance(row);
            if (!phrase.n || cases.size() >= 1048576)
                throw std::invalid_argument("invalid reed sentence batch");
            cases.push_back(
                {phrase, parse(k, ~0u), parse(light, 255), parse(filter, 3), parse(angle, 3), parse(wire, 4)});
        }
        if (cases.empty())
            throw std::invalid_argument("no material questions to present");
        Word n = cases.size(), a = partner().resident.animal.energy, b = local.resident.animal.energy;
        Temporary stimuli(s.d, n * sizeof(cases[0])), read(s.d, Count(n) * sizeof(siphon::Report));
        s.d.check(s.d.upload(stimuli.ptr, cases.data(), n * sizeof(cases[0])),
                  "present independent new spoken problems");
        Function f{};
        s.bind(f, "siphon_sentences", Image::siphons);
        auto law = local.referent.genes;
        Word body = organs();
        void *args[]{&minds.ptr, &words.ptr, &law, &stimuli.ptr, &read.ptr, &n, &a, &b, &body};
        auto start = std::chrono::steady_clock::now();
        s.d.check(s.d.launch(f, (n + 127) / 128, 1, 1, 128, 1, 1, 0, nullptr, args, nullptr),
                  "complete material utterances from private acoustic information");
        std::vector<siphon::Report> result(n);
        s.d.check(s.d.download(result.data(), read.ptr, n * sizeof(result[0])), "hear each unprompted completion");
        double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        Count completed = 0, spoken = 0, food = 0;
        struct Evidence {
            Count trials = 0, unique = 0, accurate = 0, answered = 0, ambiguous = 0, withheld = 0;
        };
        std::array<Evidence, 5> evidence{};
        for (Word i = 0; i < n; ++i) {
            auto &r = result[i];
            Word heard_questions = cases[i].wire == 1 ? 0 : r.queries,
                 heard_answers = cases[i].wire == 2 ? 0 : r.answers;
            std::string speech;
            if (r.accepted)
                for (Word j = 0; j < r.said.n; ++j) {
                    if (j)
                        speech += '/';
                    speech += acoustic::spell(r.said.wave[j]);
                }
            std::cout << "reed_test=" << i << "  answer=" << (speech.empty() ? "_" : speech)
                      << "  recognized=" << r.recognized << "  role=" << r.role << "  value=" << r.spoken
                      << "  candidates=" << std::popcount(r.candidates) << "  candidate_mask=" << r.candidates
                      << "  queries=" << r.queries << "  replies=" << r.answers
                      << "  heard_questions=" << heard_questions << "  heard_replies=" << heard_answers
                      << "  work=" << r.cost_a + r.cost_b << "  carbon=" << r.food_a + r.food_b << '\n';
            auto &audit = evidence[cases[i].wire];
            ++audit.trials;
            if (r.recognized && r.role<2) {
                Word possible = 0;
                if (r.role == 1)
                    possible = 1u << siphon::sensor(cases[i].inlet, r.referent);
                else
                    for (Word j = 0; j < 4; ++j)
                        if (siphon::sensor(cases[i].inlet, j) == r.referent)
                            possible |= 1u << j;
                if (std::has_single_bit(possible)) {
                    ++audit.unique;
                    audit.accurate += r.accepted && possible == (1u << r.spoken);
                    audit.answered += r.accepted;
                } else {
                    ++audit.ambiguous;
                    audit.withheld += !r.accepted;
                }
            }
            completed += r.accepted;
            spoken += heard_questions + heard_answers;
            food += r.food_a + r.food_b;
        }
        constexpr const char *names[]{"open", "query_shut", "answer_shut", "query_shift", "answer_shift"};
        for (Word j = 0; j < evidence.size(); ++j)
            if (evidence[j].trials) {
                auto &e = evidence[j];
                std::cout << "reed_evidence=" << names[j] << "  offered=" << e.trials << "  unique=" << e.unique
                          << "  answered=" << e.answered << "  accurate=" << e.accurate << "  ambiguous=" << e.ambiguous
                          << "  withheld=" << e.withheld << '\n';
            }
        std::cout << "reed_sentences=" << n << "  completed=" << completed << "  audible_turns=" << spoken
                  << "  carbon=" << food << "  seconds=" << seconds << "  utterances_per_s=" << n / seconds
                  << std::endl;
    }
    void private_facts(Word rounds, Word seed) {
        if (!rounds)
            throw std::invalid_argument("at least one quartet is required");
        Temporary out(s.d, Count(rounds) * sizeof(siphon::Reading));
        std::vector<siphon::Reading> books(rounds);
        Function fn{};
        s.bind(fn, "siphon_coherence", Image::siphons);
        const char *labels[]{"lived",        "query_shut",   "answer_shut", "query_shift",
                             "answer_shift", "words_erased", "hands_erased"};
        for (Word mode = 0; mode < std::size(labels); ++mode) {
            void *args[]{&minds.ptr, &out.ptr, &rounds, &seed, &mode};
            s.d.check(s.d.launch(fn, (rounds + 127) / 128, 1, 1, 128, 1, 1, 0, nullptr, args, nullptr),
                      "test both roles across changed private sources");
            s.d.check(s.d.download(books.data(), out.ptr, Count(rounds) * sizeof(books[0])),
                      "read independent cold harvests");
            Count total = 0, worked = 0, four = 0, quartets = 0, strokes = 0, queries = 0, replies = 0;
            for (auto &r : books) {
                total += r.trials;
                worked += r.harvests;
                four += r.all_four;
                quartets += r.quartets;
                strokes += r.strokes;
                queries += r.queries;
                replies += r.replies;
            }
            std::cout << "reed_pair=" << labels[mode] << "  trials=" << total << "  productive=" << worked
                      << "  quartets=" << quartets << "  four_correct=" << four << "  strokes=" << strokes
                      << "  questions=" << queries << "  replies=" << replies << "  seed=" << seed << std::endl;
        }
    }

  public:
    SiphonInterview(Session &lab,AgentImage &a,AgentImage &b,DevicePtr soma,DevicePtr peer)
        : s(lab), local(a), upstream(b), minds(s.d, a.calling && b.calling ? sizeof(siphon::Pair) : 0),
          table(s.d, minds.ptr ? sizeof(siphon::Room) : 0), words(s.d, minds.ptr ? sizeof(siphon::Vocabulary) : 0),
          response(s.d, minds.ptr ? sizeof(siphon::Report) : 0),
          assemblies(s.d, a.potter && minds.ptr ? sizeof(clay::Mind) : 0),
          slabs(s.d, assemblies.ptr ? sizeof(clay::Instrument) : 0),
          hearing(s.d, assemblies.ptr ? sizeof(clay::Hearing) : 0),
          wheel(s.d,assemblies.ptr ? sizeof(press::Wheel) : 0),subject(s.d,assemblies.ptr ? sizeof(press::Topic) : 0),
          pressure(s.d,assemblies.ptr ? sizeof(press::Receipt) : 0),
          spatial(s.d,a.ranges && assemblies.ptr?sizeof(hollow::Saved):0),
          place_words(s.d,spatial.ptr?sizeof(siphon::Vocabulary):0),marsh(s.d,spatial.ptr?sizeof(hollow::Marsh):0),
          marsh_topic(s.d,spatial.ptr?sizeof(hollow::Topic):0),marsh_contact(s.d,spatial.ptr?sizeof(hollow::Contact):0),
          effort_brain(s.d,a.values_effort&&spatial.ptr?sizeof(effort::Saved):0),ford(s.d,effort_brain.ptr?sizeof(effort::Board):0),
          ford_topic(s.d,effort_brain.ptr?sizeof(effort::Topic):0),ford_reply(s.d,effort_brain.ptr?sizeof(effort::Reply):0),
          ford_bite(s.d,effort_brain.ptr?sizeof(effort::Bite):0),ford_flag(s.d,effort_brain.ptr?4:0),
          rack_brain(s.d,a.invents&&spatial.ptr?sizeof(linkage::Saved):0),rack(s.d,rack_brain.ptr?sizeof(linkage::Rack):0),
          rack_voices(s.d,rack_brain.ptr?2*sizeof(linkage::Tongue):0),rack_reply(s.d,rack_brain.ptr?sizeof(linkage::Uttered):0),
          studio_brain(s.d,a.practises&&rack_brain.ptr?sizeof(practice::Saved):0),studio(s.d,studio_brain.ptr?sizeof(practice::Bench):0),
          studio_output(s.d,studio_brain.ptr?sizeof(practice::Exchange):0),
          reed_pair(s.d,a.folds_reeds&&a.riverine?2*sizeof(runnel::Mind):0),flume(s.d,reed_pair.ptr?sizeof(runnel::Flume):0),
          flume_voices(s.d,reed_pair.ptr?2*sizeof(linkage::Tongue):0),flume_reply(s.d,reed_pair.ptr?sizeof(runnel::Dispatch):0),
          bank_brain(s.d,a.hears_banks?sizeof(rattle::Mind):0),bank_table(s.d,bank_brain.ptr?sizeof(rattle::Table):0),
          bank_voices(s.d,bank_brain.ptr?2*sizeof(linkage::Tongue):0),bank_reply(s.d,bank_brain.ptr?sizeof(rattle::Exchange):0),
          float_head(s.d,a.binds_floats?3*sizeof(braid::Saved):0),float_tray(s.d,float_head.ptr?sizeof(braid::Table):0),float_reply(s.d,float_head.ptr?sizeof(braid::Exchange):0),
          float_memories(s.d,float_head.ptr?3*sizeof(braid::Narration):0),float_answer(s.d,float_head.ptr?sizeof(braid::Receipt):0), float_regulator(s.d,float_head.ptr?sizeof(regulation::Source):0),somatic_bands{soma,peer}{
        if(float_head.ptr){s.bind(float_recall,"braid_recall",Image::runnels);s.bind(regulated_recall,"braid_recall_regulated",Image::runnels);s.bind(regulated_practice,"apprentice_hand",Image::runnels);s.bind(float_act,"braid_interview",Image::runnels);s.bind(float_stream,"braid_hear_stream",Image::runnels);}
        if(bank_brain.ptr)s.bind(bank_act,"bank_interview",Image::siphons);
        if(reed_pair.ptr)s.bind(flume_act,"flume_interview",Image::siphons);
        if(studio_brain.ptr){
            s.d.check(s.d.upload(studio_brain.ptr,&a.training,sizeof(a.training)),"restore curiosity and material memories");
            s.bind(studio_act,"practice_interview",Image::siphons);
        }
        if(rack_brain.ptr){
            s.d.check(s.d.upload(rack_brain.ptr,&a.mechanism,sizeof(a.mechanism)),"restore the animal's mechanism tissue");
            s.bind(rack_act,"linkage_conversation",Image::siphons);
        }
        if(effort_brain.ptr){
            s.d.check(s.d.upload(effort_brain.ptr,&a.prospects,sizeof(a.prospects)),"restore prospective plasticity");
            s.bind(ford_move,"effort_room",Image::siphons);s.bind(ford_say,"effort_speak",Image::siphons);s.bind(ford_teach,"effort_teach",Image::siphons);
        }
        if(spatial.ptr){
            s.d.check(s.d.upload(spatial.ptr,&a.path_memory,sizeof(a.path_memory)),"restore personal spatial tissue");
            s.d.check(s.d.upload(place_words.ptr,&a.path_speech.words,sizeof(a.path_speech.words)),"restore the spatial words");
            s.bind(marsh_act,"hollow_move",Image::siphons);s.bind(say_place,"hollow_say",Image::siphons);s.bind(teach_places,"hollow_words",Image::siphons);
        }
        if (!minds.ptr)
            return;
        siphon::Pair pair{b.calls.animal, a.calls.animal};
        s.d.check(s.d.upload(minds.ptr, &pair, sizeof(pair)), "place the two living acoustic circuits");
        s.d.check(s.d.upload(words.ptr, &a.native_words, sizeof(a.native_words)),
                  "retain the resident's acquired forms");
        s.bind(exchange, "siphon_converse", Image::siphons);
        if (assemblies.ptr) {
            s.d.check(s.d.upload(assemblies.ptr, &a.ceramic.mind, sizeof(clay::Mind)),
                      "restore independently acquired assemblies");
            s.bind(press_move,"press_wheel",Image::siphons);
            s.bind(press_say,"press_speak",Image::siphons);
            s.bind(touch, "clay_handle", Image::siphons);
            s.bind(clay_reply, "clay_turn", Image::siphons);
        }
    }
    bool control(const std::string &line) {
        std::istringstream input(line);
        std::string token, more;
        input >> token;
        if(capillary_command(token,input,line))return true;
        if (story_command(token,input,line))
            return true;
        if (audience_command(token, input, line))
            return true;
        if (cascade_command(token, input, line))
            return true;
        if (inlet_command(token, input))
            return true;
        if(report_command(token,input,line))return true;
        if(float_command(token,input,line))return true;
        if(bank_command(token,input,line))return true;
        if(flume_command(token,input,line))return true;
        if(studio_command(token,input,line))return true;
        if(rack_command(token,input,line))return true;
        if (ford_command(token,input,line))return true;
        if (marsh_command(token,input,line))return true;
        if (wheel_command(token,input,line)) return true;
        if (token != "/reed" && !open)
            return false;
        if (!minds.ptr) {
            if (token == "/reed")
                throw std::invalid_argument("the reed table requires two residents with native acoustic organs");
            return false;
        }
        if (open && workshop(token, input))
            return true;
        Word command = 0, seed = 0, inlet = 256, band = 0, angle = 0;
        siphon::Sentence message{};
        if (open && token == "/reed-tests") {
            std::string path;
            std::getline(input >> std::ws, path);
            batch(path);
            return true;
        } else if (open && token == "/private-facts") {
            std::string count, k;
            if (!(input >> count >> k) || input >> more)
                throw std::invalid_argument("use /private-facts QUARTETS SEED");
            private_facts(parse(count, 65536), parse(k, ~0u));
            return true;
        } else if (token == "/reed") {
            std::string k, q, b, g;
            if (!(input >> k))
                throw std::invalid_argument("use /reed SEED [SPECTRUM FILTER OPENING]");
            seed = parse(k, ~0u);
            scene_seed = seed;
            if (input >> q) {
                if (!(input >> b >> g) || input >> more)
                    throw std::invalid_argument("a material override needs three quantities");
                inlet = parse(q, 255);
                band = parse(b, 3);
                angle = parse(g, 3);
            }
            open = true;
        } else if (token == "/leave-reed") {
            open = false;
            return true;
        } else if (token == "/reed-wire") {
            if (!(input >> more) || (more != "open" && more != "query-shut" && more != "answer-shut" &&
                                     more != "query-shift" && more != "answer-shift"))
                throw std::invalid_argument("use /reed-wire open|query-shut|answer-shut|query-shift|answer-shift");
            wire = more == "query-shut"     ? 1
                   : more == "answer-shut"  ? 2
                   : more == "query-shift"  ? 3
                   : more == "answer-shift" ? 4
                                            : 0;
            std::cout << "reed_wire=" << more << std::endl;
            return true;
        } else if (token == "/reed-learning") {
            if (!(input >> more) || (more != "live" && more != "fixed"))
                throw std::invalid_argument("use /reed-learning live|fixed");
            plastic = more == "live";
            std::cout << "reed_learning=" << more << std::endl;
            return true;
        } else if (token == "/show-query") {
            std::string content;
            std::getline(input >> std::ws, content);
            auto sep = content.find('|');
            if (sep == std::string::npos || content.find('|', sep + 1) != std::string::npos)
                throw std::invalid_argument("use /show-query QUESTION | OBSERVED WORD");
            std::istringstream prompt(content.substr(0, sep)), answer(content.substr(sep + 1));
            message = utterance(prompt);
            auto word = utterance(answer);
            if (!message.n || message.n >= siphon::phrase_length || word.n != 1 || !word.wave[0])
                throw std::invalid_argument(
                    "a demonstrated question needs up to seven sounds and one witnessed answer");
            for (Word j = 0; j < message.n; ++j)
                if (!message.wave[j])
                    throw std::invalid_argument("demonstrate a complete question");
            message.wave[message.n++] = word.wave[0];
            command = 4;
        } else if (token == "/duet") {
            command = 1;
        } else {
            if (token == "/caption")
                command = 2;
            else {
                if (token.empty() || token[0] == '/')
                    return false;
                command = 3;
                input.clear();
                input.str(line);
            }
            message = utterance(input);
            if (!message.n)
                return false;
        }

        Word a = partner().resident.animal.energy, b = local.resident.animal.energy, mutable_tissue = plastic;
        auto law = local.referent.genes;
        Word body = organs();
        // A wheel contact places its vibration in the witnessed context. A
        // reed encounter obtains its context from the resident's assemblies.
        auto context_ptr = at_wheel ? decltype(assemblies.ptr){} : assemblies.ptr;
        void *args[]{&minds.ptr,      &table.ptr, &words.ptr,      &law,     &command,     &seed,
                     &inlet,          &band,      &angle,          &wire,    &a,           &b,
                     &mutable_tissue, &body,      &context_ptr,    &message, &response.ptr};
        if (command == 3 && holding && clay_budget) {
            Word see = transparent;
            void *request[]{&minds.ptr,  &assemblies.ptr, &table.ptr, &words.ptr, &law,
                            &slabs.ptr,  &clay_budget,    &see,       &wire,      &a,
                            &b,          &mutable_tissue, &body,      &message,   &response.ptr,
                            &hearing.ptr};
            s.d.check(s.d.launch(clay_reply, 1, 1, 1, 1, 1, 1, 0, nullptr, request, nullptr),
                      "choose evidence and complete the learned spoken request");
            clay::Hearing result{};
            s.d.check(s.d.download(&result, hearing.ptr, sizeof(result)), "observe self-directed source recognition");
            s.d.check(s.d.download(&local.ceramic.mind, assemblies.ptr, sizeof(clay::Mind)),
                      "retain the inferred source");
            std::cout << "clay_inferred=" << result.context << "  taps=" << result.strokes
                      << "  sounded=" << result.mask << "  alternatives=" << result.alternatives << std::endl;
        } else
            s.d.check(s.d.launch(exchange, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
                      "let the pair exchange physical evidence");
        siphon::Report r{};
        siphon::Room stage{};
        s.d.check(s.d.download(&r, response.ptr, sizeof(r)), "listen to the completed utterance");
        s.d.check(s.d.download(&stage, table.ptr, sizeof(stage)), "observe the aperture and local material");
        if (command == 1 || command == 3) {
            partner().resident.animal.energy = static_cast<unsigned short>(std::min(65535u, a - r.cost_a + r.food_a));
            local.resident.animal.energy = static_cast<unsigned short>(std::min(65535u, b - r.cost_b + r.food_b));
            siphon::Pair p{};
            s.d.check(s.d.download(&p, minds.ptr, sizeof(p)), "retain the partners' own acquired projections");
            local.calls.animal = p.b;
            partner().calls.animal = p.a;
        }
        if (auto *journal = journal::current()) {
            if (command == 1 && r.accepted) {
                ++journal->metrics["reed_native_contacts"];
                journal->metrics["reed_visible_contacts"] += stage.exposed != 0;
            }
            if (command == 2 || command == 4)
                journal->metrics["reed_public_captions"] += r.accepted;
            if (command == 3) {
                ++journal->metrics["reed_human_questions"];
                journal->metrics["reed_completed_questions"] += r.accepted;
            }
        }
        if (command == 0)
            std::cout << "reed_table=" << seed << "  filter=" << stage.site.membrane
                      << "  opening=" << stage.site.opening << "  stock=" << stage.site.carbon << std::endl;
        else if (command == 1) {
            std::cout << "outlet> " << tone(r.accepted ? stage.last.query : 4) << "\ninlet> "
                      << tone(r.accepted ? stage.last.reply : 4) << '\n';
            std::cout << "reed_duet=" << r.accepted << "  query=" << stage.last.query << "  reply=" << stage.last.reply
                      << "  requested=" << stage.site.membrane
                      << "  viewed=" << (stage.exposed ? stage.last.percept : 4u)
                      << "  selected_band=" << (stage.exposed ? stage.last.attention : 4u)
                      << "  recall=" << stage.last.recall << "  stroke=" << stage.last.stroke
                      << "  aperture=" << stage.site.opening << "  food=" << r.food_a + r.food_b
                      << "  work=" << r.cost_a + r.cost_b << "  precontact_sight=" << stage.site.shared << std::endl;
        } else if (command == 2 || command == 4) {
            s.d.check(s.d.download(&local.native_words, words.ptr, sizeof(local.native_words)),
                      "retain captions learned from public contacts");
            std::cout << (command == 4 ? "clay_question_example=" : "reed_caption=") << r.accepted
                      << "  forms=" << r.forms << "  exposures=" << local.native_words.written << std::endl;
        } else {
            if (auto *j = journal::current())
                j->line("reed_prompt=heard  text=" + line);
            std::string answer;
            if (r.accepted)
                for (Word i = 0; i < r.said.n; ++i) {
                    if (i)
                        answer += ' ';
                    answer += acoustic::spell(r.said.wave[i]);
                }
            std::cout << "animal> " << (answer.empty() ? "?" : answer) << '\n';
            std::cout << "reed_completion=" << r.accepted << "  identified=" << r.recognized << "  missing=" << r.role
                      << "  remaining=" << std::popcount(r.candidates) << "  candidate_mask=" << r.candidates
                      << "  answer=" << r.spoken << "  questions=" << r.queries << "  replies=" << r.answers
                      << "  heard_questions=" << (wire == 1 ? 0 : r.queries)
                      << "  heard_replies=" << (wire == 2 ? 0 : r.answers) << "  work=" << r.cost_a + r.cost_b
                      << "  harvest=" << r.food_a + r.food_b << "  frames=" << r.frames << std::endl;
        }
        return true;
    }
};
