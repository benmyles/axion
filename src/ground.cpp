#include "grounding_world.cuh"
#include "ground_host.hpp"
#include <fstream>
#include "individual_io.hpp"
#include "life_runtime.hpp"
#include "pulse_words.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <iostream>
#include <sstream>

namespace axion::life::grounding {
namespace {
using namespace runtime;
template <class... Args> void run(Session &s, const char *name, Word blocks, Word threads, Args &...args) {
    Function f{};
    s.bind(f, name, Image::grounding);
    void *p[]{static_cast<void *>(&args)...};
    s.d.check(s.d.launch(f, blocks, 1, 1, threads, 1, 1, 0, nullptr, p, nullptr), name);
}
template <class T> void send(Session &s, DevicePtr d, const T &v) {
    s.d.check(s.d.upload(d, &v, sizeof(v)), "deliver retinal input");
}
template <class T> void take(Session &s, T &v, DevicePtr d) {
    s.d.check(s.d.download(&v, d, sizeof(v)), "read grounded response");
}
Word number(std::string_view s, Word lo, Word hi) {
    Word n;
    auto [end, err] = std::from_chars(s.data(), s.data() + s.size(), n);
    if (err != std::errc{} || end != s.data() + s.size() || n < lo || n > hi)
        throw std::invalid_argument("invalid scene quantity " + std::string(s));
    return n;
}
Vocabulary dialect() {
    Vocabulary v;
    const char *names[]{"moth", "vole", "newt", "wren", "hare", "seal", "lark", "toad"};
    const char *verbs[]{"west", "north", "trails", "rises", "nears", "crosses"};
    const char *gaps[]{"who", "whom", "how", "is", "and"};
    const char *parts[]{"yes", "no", "against", "of"};
    for (Word k = 0; k < 8; ++k)
        v.noun[k] = acoustic::pulse(names[k]);
    for (Word k = 0; k < 6; ++k)
        v.verb[k] = acoustic::pulse(verbs[k]);
    for (Word k = 0; k < 5; ++k)
        v.gap[k] = acoustic::pulse(gaps[k]);
    for (Word k = 0; k < 4; ++k)
        v.particle[k] = acoustic::pulse(parts[k]);
    return v;
}
struct Options {
    Word repair_probes = 0, generations = 0, population = 64, origin = 3, drift = 0, dialogues = 60;
    bool fresh = false, explicit_genes = false;
    std::string save, lesson;
    Word n = 32, seed = 91571, rounds = 256, probes = 128, crowd = 4, cut = 0, style = 1;
    bool inspect = false;
    std::string source = "examples/lampwright.mind";
};
Options options(int argc, char **argv) {
    Options o;
    for (int i = 2; i < argc; ++i) {
        std::string f = argv[i];
        if (f == "--fresh") {
            o.fresh = true;
            continue;
        }
        if (f == "--inspect-code") {
            o.inspect = true;
            continue;
        }
        if (++i == argc)
            throw std::invalid_argument("missing value for " + f);
        std::string v = argv[i];
        if (f == "--worlds")
            o.n = number(v, 1, 8192);
        else if (f == "--seed")
            o.seed = number(v, 0, ~0u);
        else if (f == "--episodes")
            o.rounds = number(v, 1, 65536);
        else if (f == "--probes")
            o.probes = number(v, 0, 8192);
        else if (f == "--repair-probes")
            o.repair_probes = number(v, 0, 8192);
        else if (f == "--generations")
            o.generations = number(v, 0, 512);
        else if (f == "--population")
            o.population = number(v, 1, 2048);
        else if (f == "--save")
            o.save = v;
        else if (f == "--lesson")
            o.lesson = v;
        else if (f == "--dialogues")
            o.dialogues = number(v, 0, demonstrations);
        else if (f == "--selection" && (v == "select" || v == "drift"))
            o.drift = v == "drift";
        else if (f == "--origin" && (v == "random" || v == "inherited" || v == "inert" || v == "permissive")) {
            o.explicit_genes = true;
            o.origin = v == "inherited" ? 0 : v == "inert" ? 1 : v == "random" ? 2 : 3;
        } else if (f == "--crowd")
            o.crowd = number(v, 2, objects);
        else if (f == "--load")
            o.source = v;
        else if (f == "--dialect" && (v == "english" || v == "shuffled"))
            o.style = v == "shuffled";
        else if (f == "--pairing" && (v == "observed" || v == "shuffled" || v == "scrambled"))
            o.cut = v == "shuffled" ? 1 : v == "scrambled" ? 2 : 0;
        else
            throw std::invalid_argument("unknown scene option " + f);
    }
    return o;
}
void fit(Session &s, DevicePtr bodies, DevicePtr lens, Word n) {
    run(s, "ground_words", n * (forms + 1), 32, bodies, lens, n);
    run(s, "ground_bind", n, 32, bodies);
}
template <class... Args> void breed(Session &s, const char *name, Word n, Args &...args) {
    Function f{};
    s.bind(f, name, Image::recollection);
    void *p[]{static_cast<void *>(&args)...};
    s.d.check(s.d.launch(f, (n + 63) / 64, 1, 1, 64, 1, 1, 0, nullptr, p, nullptr), name);
}
RecollectionGenes select_rule(Session &s, const Options &o, DevicePtr lens, DevicePtr vocab,
                              RecollectionGenes inherited) {
    RecollectionGenes prior{0xf755f755u, 0x0005e488u, 9};
    Word origin = o.origin, n = o.population, seed = o.seed, keep = 1, drift = o.drift;
    if (origin == 3) {
        origin = 0;
        inherited = prior;
    }
    if (!origin && !o.generations)
        return inherited;
    Temporary parents(s.d, n * sizeof(prior)), next(s.d, n * sizeof(prior)), brains(s.d, Count(n) * sizeof(Mind)),
        fitness(s.d, n * sizeof(Word)), audit(s.d, n * sizeof(GroundingAudit));
    breed(s, "recollect_seed", n, parents.ptr, n, inherited, seed, origin);
    Word siblings = 1, style = 1, rounds = 128, test = 40, cut = 0;
    for (Word epoch = 0; epoch < o.generations; ++epoch) {
        seed = mix(o.seed ^ mix(0xe7a83bu + epoch));
        Word crowd = 3 + epoch % 2;
        run(s, "ground_school", n, 32, brains.ptr, lens, vocab, parents.ptr, siblings, seed, style, rounds, crowd,
            cut);
        fit(s, brains.ptr, lens, n);
        run(s, "ground_probe", n, 32, brains.ptr, vocab, audit.ptr, siblings, seed, style, test, crowd);
        run(s, "ground_merit", (n + 31) / 32, 32, audit.ptr, fitness.ptr, n, siblings);
        if (auto *j = journal::current()) {
            std::vector<Word> scores(n);
            s.d.check(s.d.download(scores.data(), fitness.ptr, n * sizeof(Word)), "observe scene selection");
            Count sum = 0;
            for (Word q : scores)
                sum += q;
            j->breeding.push_back({epoch, n, {sum, *std::max_element(scores.begin(), scores.end())}});
        }
        breed(s, "recollect_brood", n, parents.ptr, next.ptr, fitness.ptr, n, seed, drift, keep);
        std::swap(parents.ptr, next.ptr);
    }
    take(s, inherited, parents.ptr);
    return inherited;
}
Voice words(std::string line) {
    Voice v;
    for (auto &c : line)
        if (c == '?' || c == '!' || c == '.' || c == ',')
            c = ' ';
    std::istringstream input(line);
    std::string token;
    while (input >> token) {
        if (v.n == utterance)
            throw std::invalid_argument("this acoustic turn exceeds eight waveforms");
        v.waves[v.n++] = acoustic::pulse(token);
    }
    return v;
}
Word hex(std::string_view s, Word hi) {
    Word n;
    auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), n, 16);
    if (ec != std::errc{} || end != s.data() + s.size() || n > hi)
        throw std::invalid_argument("invalid eight-channel light value");
    return n;
}
View stage(const std::string &line) {
    View view;
    std::istringstream in(line);
    std::string body;
    while (in >> body) {
        auto at = body.find('@'), to = body.find('>');
        if (at == body.npos || to == body.npos || to <= at + 1)
            throw std::invalid_argument("use LIGHT@BEFORE>AFTER for each patch");
        Word light = hex(std::string_view(body).substr(0, at), 255);
        auto spot = [&](Retina &retina, const std::string &port) {
            if (port == "-")
                return;
            Word i = number(port, 0, 63);
            if (retina.sites & (1ull << i))
                throw std::invalid_argument("two light patches occupy the same retinal site");
            illuminate(retina, i, light);
        };
        spot(view.before, body.substr(at + 1, to - at - 1));
        spot(view.after, body.substr(to + 1));
    }
    return view;
}
} // namespace
GroundedSpeech::GroundedSpeech(Session &s, const Mind &initial, bool present, DevicePtr shared)
    : lab(s), private_body(s.d, present && !shared ? sizeof(Mind) : 0), lens(s.d, present ? sizeof(Lens) : 0),
      working_turn(s.d, present ? sizeof(Clarification) : 0), body(shared ? shared : private_body.ptr) {
    if (present) {
        s.d.check(s.d.zero(working_turn.ptr, 0, sizeof(Clarification)), "begin conversation");
        if (!shared)
            send(lab, body, initial);
        run(lab, "ground_lens", 256, 32, lens.ptr);
    }
}
void GroundedSpeech::present(const View &scene) {
    if(!body)return;
    lab.d.check(lab.d.zero(working_turn.ptr,0,sizeof(Clarification)),"look at the new physical scene");
    Temporary image(lab.d,sizeof(scene));send(lab,image.ptr,scene);run(lab,"ground_stage",1,1,body,image.ptr);
}
bool GroundedSpeech::hear(const std::string &line, std::string &answer) {
    if (!body || line.empty())
        return false;
    if (line == "/tabletop" || line.starts_with("/tabletop ")) {
        present(stage(line.substr(9)));
        answer = "_";
        return true;
    }
    if (line.starts_with("/point ")) {
        auto input = words(line.substr(7));
        if (input.n != 1)
            throw std::invalid_argument("pointing requires one acoustic form");
        Temporary target(lab.d, 2 * sizeof(Count));
        run(lab, "ground_point", 1, 1, body, input.waves[0], target.ptr);
        std::array<Count, 2> sites{};
        take(lab, sites, target.ptr);
        std::ostringstream text;
        for (Word i = 0; i < 64; ++i)
            if (sites[1] & (1ull << i)) {
                if (text.tellp() > 0)
                    text << ' ';
                text << i;
                if (!(sites[0] & (1ull << i)))
                    text << '?';
            }
        answer = text.tellp() > 0 ? text.str() : "?";
        if (auto *j = journal::current())
            j->line("scene_point=" + line.substr(7) + "  certain=" + std::to_string(sites[0]) +
                    "  possible=" + std::to_string(sites[1]));
        return true;
    }
    if (line == "/ground-replay") {
        lab.d.check(lab.d.zero(working_turn.ptr, 0, sizeof(Clarification)), "finish prior question");
        fit(lab, body, lens.ptr, 1);
        run(lab, "ground_question_fit", 1, 32, body);
        answer = "_";
        return true;
    }
    bool teach = line.starts_with("/overhear "), example = line.starts_with("/ground-example ");
    if (line[0] == '/' && !teach && !example)
        return false;
    auto heard = teach ? line.substr(10) : example ? line.substr(16) : line;
    Count y = 0;
    if (example) {
        auto bar = heard.find('|');
        if (bar == heard.npos)
            throw std::invalid_argument("a question example needs the heard question | response");
        auto response = words(heard.substr(bar + 1));
        if (response.n != 1)
            throw std::invalid_argument("a grounded response has one acoustic form");
        y = response.waves[0];
        heard.resize(bar);
    }
    Voice v;
    try {
        v = words(heard);
    } catch (const std::invalid_argument &) {
        if (teach || example)
            throw;
        return false;
    }
    if (!v.n)
        return false;
    Temporary audio(lab.d, sizeof(v));
    send(lab, audio.ptr, v);
    if (teach) {
        run(lab, "ground_capture", 1, 32, body, lens.ptr, audio.ptr);
        answer = "_";
        return true;
    }
    if (example) {
        run(lab, "ground_exchange", 1, 1, body, audio.ptr, y);
        answer = "_";
        return true;
    }
    Temporary result(lab.d, sizeof(Answer));
    run(lab, "ground_consult", 1, 1, body, audio.ptr, result.ptr, working_turn.ptr);
    Answer a;
    take(lab, a, result.ptr);
    if (!a.hypotheses)
        return false;
    answer = a.asking ? acoustic::spell(a.wave) + "?" : a.complete ? acoustic::spell(a.wave) : "?";
    if (auto *j = journal::current()) {
        ++j->metrics["scene_turns"];
        j->line("scene_reply=" + answer + "  resolved=" + std::to_string(a.complete) +
                "  asking=" + std::to_string(a.asking) + "  sites=" + std::to_string(a.sites) + "  heard=" + line);
    }
    return true;
}
void GroundedSpeech::store(Mind &out) {
    if (body)
        take(lab, out, body);
}

int command(int argc, char **argv) {
    if (argc == 3 && std::string_view(argv[2]) == "--help") {
        std::cout << "axion ground --worlds N --episodes N --crowd N --dialect english|shuffled --pairing "
                     "observed|shuffled|scrambled --probes N --seed N --inspect-code\n  --load MIND --save MIND "
                     "--lesson FILE|- --fresh --dialogues N --repair-probes N --generations N --population N --origin "
                     "inherited|random|inert|permissive --selection select|drift\n";
        return 0;
    }
    auto o = options(argc, argv);
    storage::AgentImage animal;
    animal.read(o.source);
    Session lab;
    lab.open();
    auto v = dialect();
    RecollectionGenes genes{};
    Word n = o.lesson.empty() ? o.n : 1;
    Temporary brain(lab.d, Count(n) * sizeof(Mind)), lens(lab.d, sizeof(Lens)), acoustic(lab.d, sizeof(v)),
        dna(lab.d, sizeof(genes)), scores(lab.d, n * sizeof(GroundingAudit));
    send(lab, acoustic.ptr, v);
    run(lab, "ground_lens", 256, 32, lens.ptr);
    if (animal.grounded && !o.explicit_genes)
        o.origin = 0;
    genes = select_rule(lab, o, lens.ptr, acoustic.ptr,
                        animal.grounded ? animal.referent.genes : animal.recollection.genes);
    send(lab, dna.ptr, genes);
    if (auto *j = journal::current()) {
        j->source = o.source;
        j->device = lab.name;
    }
    if (!o.lesson.empty()) {
        Mind initial = animal.grounded && !o.fresh ? animal.referent : Mind{};
        initial.genes = genes;
        GroundedSpeech pupil(lab, initial, true);
        std::ifstream file;
        std::istream *in = &std::cin;
        if (o.lesson != "-") {
            file.open(o.lesson);
            if (!file)
                throw std::runtime_error("cannot open grounded lesson");
            in = &file;
        }
        bool pending = false;
        for (std::string line; std::getline(*in, line);)
            if (!line.empty()) {
                if (line.starts_with("/overhear ") || line.starts_with("/ground-example "))
                    pending = true;
                if (line == "/ground-replay")
                    pending = false;
                std::string answer;
                bool known = pupil.hear(line, answer);
                if (!known && line[0] == '/')
                    throw std::invalid_argument("unknown grounded stimulus " + line);
                if (line[0] != '/' || line.starts_with("/point "))
                    std::cout << "animal> " << (known ? answer : "?") << '\n';
            }
        std::string done;
        if (pending)
            pupil.hear("/ground-replay", done);
        pupil.store(animal.referent);
        animal.grounded = true;
        animal.referent.extent = sizeof(Mind);
        validate(animal.referent);
        animal.specimen(o.save.empty() ? o.source : o.save, "focal");
        if (!o.save.empty())
            storage::write_checked(o.save, animal);
        return 0;
    }
    cuda::Event school_start{}, school_end{};
    lab.d.check(lab.d.event_new(&school_start, 0), "create school timer");
    lab.d.check(lab.d.event_new(&school_end, 0), "create replay timer");
    lab.d.check(lab.d.record(school_start, nullptr), "start timed scene exposure");
    run(lab, "ground_school", n, 32, brain.ptr, lens.ptr, acoustic.ptr, dna.ptr, n, o.seed, o.style, o.rounds, o.crowd,
        o.cut);
    fit(lab, brain.ptr, lens.ptr, n);
    lab.d.check(lab.d.record(school_end, nullptr), "finish learning and replay");
    lab.d.check(lab.d.wait(school_end), "finish timed exposure");
    float school_ms = 0;
    lab.d.check(lab.d.elapsed(&school_ms, school_start, school_end), "measure the training batch");
    lab.d.event_free(school_start);
    lab.d.event_free(school_end);
    std::cout << "school_gpu_ms=" << school_ms << "  scenes_offered=" << Count(n) * o.rounds
              << "  school_scenes_per_s=" << (school_ms > 0 ? Count(n) * o.rounds * 1000.0 / school_ms : 0) << '\n';
    Mind child;
    take(lab, child, brain.ptr);
    std::cout << lab.name << "  grounding_gene=" << std::hex << genes.low << '.' << genes.high << '.'
              << genes.expression << std::dec << "  grounding_neurons=" << hypotheses << "  bytes=" << sizeof(child)
              << "  utterances=" << child.experienced << "  acoustic_forms=" << child.count
              << "  layouts=" << child.layout << '\n';
    if (o.inspect)
        for (Word k = 1; k <= child.count; ++k) {
            auto &w = child.word[k];
            std::cout << "grounded_form=" << acoustic::spell(w.wave) << "  visible_hypotheses=" << w.viable
                      << "  observations=" << w.exposures;
            for (Word r = 0; r < 3; ++r)
                if (child.layout & (1u << r))
                    std::cout << "  comparison_mask=" << std::hex << child.edge[r][k].possible << std::dec;
            std::cout << '\n';
        }
    run(lab, "ground_probe", n, 32, brain.ptr, acoustic.ptr, scores.ptr, n, o.seed, o.style, o.probes, o.crowd);
    std::vector<GroundingAudit> all(n);
    lab.d.check(lab.d.download(all.data(), scores.ptr, n * sizeof(GroundingAudit)), "test unfamiliar meanings");
    Count nouns = 0, verbs = 0, cloze = 0, blanks = 0, passed = 0, tried = 0, wrong = 0, silent = 0, reversed = 0,
          excluded = 0;
    for (auto &a : all) {
        nouns += a.names;
        verbs += a.verbs;
        cloze += a.spoken;
        blanks += a.expected;
        passed += a.full;
        tried += a.cases;
        wrong += a.wrong;
        silent += a.silent;
        reversed += a.bindings;
        excluded += a.trials;
    }
    std::cout << "scene_transfer="
              << (o.cut == 1   ? "shuffled"
                  : o.cut == 2 ? "scrambled"
                               : "observed")
              << "  worlds=" << n << "  nouns=" << nouns << "  available_nouns=" << n * 8 << "  verbs=" << verbs
              << "  available_verbs=" << n * 6 << "  exact=" << passed << "  opportunities=" << tried
              << "  errors=" << wrong << "  undecided=" << silent << "  reversed=" << reversed
              << "  reversals=" << excluded << "  cloze=" << cloze << "  blanks=" << blanks << '\n';
    Word examples = o.dialogues;
    run(lab, "ground_colloquy", (n + 31) / 32, 32, brain.ptr, acoustic.ptr, n, o.seed, o.style, examples, o.crowd);
    run(lab, "ground_question_fit", n, 32, brain.ptr);
    Temporary interview(lab.d, n * sizeof(InterviewAudit));
    run(lab, "ground_interview", n, 32, brain.ptr, acoustic.ptr, interview.ptr, o.seed, o.style, o.probes, o.crowd);
    std::vector<InterviewAudit> observed(n);
    lab.d.check(lab.d.download(observed.data(), interview.ptr, n * sizeof(InterviewAudit)),
                "read grounded interviews");
    for (Word k = 0; k < 5; ++k) {
        Count correct = 0, total = 0, mistakes = 0;
        for (const auto &x : observed) {
            correct += x.exact[k];
            total += x.valid[k];
            mistakes += x.wrong[k];
        }
        std::cout << "visual_question=" << k << "  correct=" << correct << "  trials=" << total
                  << "  wrong=" << mistakes << '\n';
    }
    if (o.repair_probes) {
        Temporary trials(lab.d, n * sizeof(RepairAudit));
        run(lab, "ground_repair", n, 32, brain.ptr, acoustic.ptr, trials.ptr, o.seed, o.style, o.repair_probes,
            o.crowd);
        std::vector<RepairAudit> rows(n);
        lab.d.check(lab.d.download(rows.data(), trials.ptr, n * sizeof(RepairAudit)),
                    "test clarification with a hidden intended referent");
        Count needed = 0, offered = 0, answered = 0, correct = 0, wrong = 0, broken = 0, chance = 0;
        for (auto r : rows) {
            needed += r.trials;
            offered += r.requests;
            answered += r.answered;
            correct += r.correct;
            wrong += r.wrong;
            broken += r.misinformed;
            chance += r.chance1024;
        }
        std::cout << "referent_repair=" << needed << "  questions=" << offered << "  resolved=" << answered
                  << "  correct=" << correct << "  wrong=" << wrong << "  random_feedback=" << broken
                  << "  expected_guesses=" << double(chance) / 1024 << '\n';
    }
    take(lab, child, brain.ptr);
    std::cout << "question_frames=" << child.constructions << "  conversations=" << child.exchanges
              << "  clipped=" << child.crowded_templates << '\n';
    if (o.inspect)
        for (Word j = 0; j < child.constructions; ++j) {
            const auto &r = child.construction[j];
            std::cout << "question_rule=" << j << "  form=";
            for (Word k = 0; k < r.motif.n; ++k)
                std::cout << (k ? "/" : "") << (r.motif.id[k] ? acoustic::spell(child.word[r.motif.id[k]].wave) : "*");
            std::cout << "  fields=" << Word(r.field[0]) << ',' << Word(r.field[1]) << ',' << Word(r.field[2])
                      << "  gap=" << Word(r.answer_slot) << "  witnesses=" << Word(r.evidence) << '\n';
        }
    if (auto *j = journal::current(); j && child.kept) {
        auto &e = child.episodes[(child.oldest + child.kept - 1) % turns];
        std::ostringstream line;
        line << "retinal_sample=" << child.experienced << "  patches=";
        for (Word k = 0; k < e.scene.n; ++k) {
            auto p = e.scene.patch[k];
            line << (k ? "," : "") << Word(p.light) << '/' << Word(p.prior) << '/' << Word(p.site);
        }
        line << "  sound=";
        for (Word k = 0; k < e.heard.n; ++k)
            line << (k ? "/" : "") << acoustic::spell(child.word[e.heard.id[k]].wave);
        j->line(line.str());
    }
    if (auto *j = journal::current()) {
        j->source = o.source;
        j->device = lab.name;
        j->metrics["animals"] = n;
        j->scores.push_back({"Held-out physical statements", passed, tried});
    }
    animal.grounded = true;
    animal.referent = child;
    animal.referent.extent = sizeof(Mind);
    validate(animal.referent);
    animal.specimen(o.save.empty() ? o.source : o.save, "focal");
    if (!o.save.empty())
        storage::write_checked(o.save, animal);
    return 0;
}
} // namespace axion::life::grounding
