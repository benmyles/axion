#include "axion/floodplain.hpp"
#include "axion/mind.hpp"
#include "axion/genesis.hpp"
#include "axion/grounding.hpp"
#include "axion/evolution.hpp"
#include "axion/journal.hpp"
#include "axion/tutor.hpp"
#include "axion/replay.hpp"
#include "axion/actor.hpp"
#include "axion/dialogue.hpp"
#include "axion/recollection.hpp"
#include "axion/sequence.hpp"
#include "axion/composition.hpp"
#include "axion/procedure.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unistd.h>

namespace {
using namespace axion;
using namespace std::literals;

constexpr std::string_view names[] = {"scent", "delay", "context", "trail", "mixed", "naming", "sorting", "pattern", "routes", "landmarks", "errand", "workshop", "invention", "seminar", "detector"};

struct Options {
    std::string_view command, backend = "gpu";
    Layout layout = Layout::automatic;
    std::optional<Learning> learning;
    std::optional<Word> brains, neurons, seed, world_seed, reversed;
    std::optional<Task> task;
    std::optional<Recall> recall;
    std::optional<Course::Experiments> experiments;
    std::optional<Word> lesson_seed, scrambled, survey, random_walk;
    Word lessons = 4, trial = 0, animal = 0;
    std::optional<Word> chain, difficult, consolidation, vocabulary, archive, plan_nodes;
    Word episodes = 256, probes = 128, report = 0, threads = 0, repeats = 5, generations = 8;
    std::string load, save;
    bool mute = false, swap_words = false;
};

void usage() {
    std::cout << R"(Axion  |  online learning in recurrent spiking populations

  ./axion life --load examples/estuary.eco --steps 4096 --appetites 256 --save world.eco
  ./tools/outcome-study.py lesson --stage probe --changed | ./axion agent --load examples/prospector.mind
  ./axion agent --from examples/relations.eco --lesson examples/relations.lesson --save animal.mind
  ./tools/relational-study.py chain-lesson | ./axion agent --from examples/pathways.eco --save pathfinder.mind
  ./axion curriculum --save colony.axn
  ./axion train --task trail --episodes 512 --save trail.axn
  ./axion eval --load trail.axn --episodes 1024
  ./axion replay --load examples/student.axn --task errand --trial 7
  ./axion tutor --save student.axn
  ./axion detector --backend gpu --brains 4096
  ./axion transfer --backend gpu --episodes 16 --lessons 8
  ./axion evolve --task context --generations 12 --save founders.axn
  ./axion bench --backend gpu --brains 4096 --neurons 64

journal     Rebuild the WebP progress overview and Markdown journal. --journal-dir DIR sets the destination.
genesis     Blackwell-native embodied ALife & CA-DNA morphogenetic spiking brains (bench, evolve, inspect).
agent       Run a descendant on your own sensory events or resume its acquired state.
floodplain  Evolve bodies and spiking tissues in a shared, editable watershed.
ground      Acquire word meanings from crowded physical scenes.
imitate     Acquire reusable motor procedures from unsegmented pulse streams.
compose     Learn reusable relations and utterances from another animal.
develop     Discover hidden states through raw sensorimotor experience.
recollect   Evolve a local visual memory under changing observations.
attention   Evolve word-conditioned control of sight and recollection.
imagine     Learn the physical effects of actions and replay unseen action sequences.
relate      Evolve object binding and active experimentation.
conjoin     Evolve conjunctive contacts and lateral prediction.
narrate     Evolve grounded, multiword reports of private events.
cultivate   Evolve intervention learning and goal-directed action.
infer       Learn named objects, relational programs and another animal's view.
commons     Breed spiking foragers on food shared by neighboring animals.
talk        Ask the learned spiking pair about food and water in a scene.
dialogue    Two spiking animals learn a private signalling convention. See axion dialogue --help.
life        Evolve programmable spiking creatures together on the GPU. See axion life --help.
train       Learn online. Probe the same held-out worlds before and after.
eval        Freeze synapses and act greedily on fresh worlds.
curriculum  Carry one colony through scent, delay, context, trail and reversal.
evolve      Select inherited circuits for learning two opposing rules quickly.
replay      Inspect an actual held-out route or see a generated picture.
tutor       Teach one animal with your own sensory examples and scalar feedback on stdin.
detector    Run the four-pair observational study with one frozen test choice.
transfer    Measure adaptation to new vocabularies and rules at increasing example counts.
bench       Time inference and online learning after warm-up.

  --backend gpu|auto|cpu      CUDA by default. Reference playgrounds also accept CPU or automatic fallback.
  --layout auto|warp|block|grid  GPU mapping (auto chooses by width)
  --brains N                  independent animals (wide networks default to 1)
  --neurons N                 power of two, 64 to 1,048,576 cells per animal
  --episodes N                trials per animal or per curriculum stage
  --trial N  --animal N       episode and animal to inspect with replay (0)
  --lessons N                 lesson draws in transfer (4) or per selection round in evolve (1)
  --lesson-seed N             hidden rule and vocabulary seed, independent of stimulus noise
  --recall cortex|content|full  readout only, identity memory, or identity + relational memory
  --feedback real|shuffled    support reward control for lesson tasks
  --generations N             selection rounds in evolve (8)
  --eval-episodes N           held-out trials per animal per probe (128)
  --report-every N            report online results in windows of N trials
  --task scent|delay|context|trail|mixed|naming|sorting|pattern|routes|landmarks|errand|workshop|invention|seminar|detector
  --rule normal|flipped       initial food contingency
  --learn full|motor|off|no-scaling  full learning, readout only, frozen, or omit sensory scaling
  --seed N                    initial wiring seed (1)
  --world-seed N              experience seed (eval overrides the held-out seed)
  --experiments off|random|paired|curious  learn sensory dependencies from observations or controlled changes
  --plan-nodes N              nonlinear inverse search nodes per probe (4096, zero for local search only)
  --chain N                   tools composed on each workshop probe (1..8)
  --vocabulary N              naming vocabulary, power of two from 8 to 4096 (8)
  --archive N                 consolidated slots, zero or a power of two from 8 to 65536
  --consolidation N           divisor for shared synaptic updates alongside fast memory (64)
  --effects regular|novel|dense|nonlinear  tool family for workshop
  --survey N                  continuous exploration moves per route outing (0..128)
  --explore curious|random    survey untried doors or take uniformly random actions
  --swap-words                reverse the two input goal cues in errand probes
  --mute-odor                 remove the informative odor during eval
  --load FILE  --save FILE    resume or save a colony at an episode boundary
  --threads N                 CPU workers (default hardware concurrency)
  --repeats N                 measured runs per benchmark mode (5)
)";
}

Word number(std::string_view s, Word min, Word max, std::string_view flag) {
    Word n{};
    auto [pos, error] = std::from_chars(s.data(), s.data() + s.size(), n);
    if (error != std::errc{} || pos != s.data() + s.size() || n < min || n > max)
        throw std::invalid_argument("invalid value for " + std::string(flag));
    return n;
}

Options parse(int argc, char **argv) {
    Options o;
    o.command = argv[1];
    constexpr std::string_view commands[] = {
        "train", "eval", "bench", "evolve", "transfer", "tutor", "replay", "detector", "curriculum"
    };
    if (std::find(std::begin(commands), std::end(commands), o.command) == std::end(commands))
        throw std::invalid_argument("unknown command " + std::string(o.command));
    if (o.command == "bench" || o.command == "evolve") o.episodes = 32;
    if (o.command == "transfer") o.episodes = 16;
    if (o.command == "evolve") o.lessons = 1;
    if (o.command == "detector") { o.episodes = 8; o.probes = 1; o.task = Task::detector; }
    constexpr Word umax = std::numeric_limits<Word>::max();
    for (int i = 2; i < argc; ++i) {
        std::string_view flag = argv[i];
        if (flag == "--swap-words") { o.swap_words = true; continue; }
        if (flag == "--mute-odor") { o.mute = true; continue; }
        if (i + 1 == argc) throw std::invalid_argument("missing value for " + std::string(flag));
        std::string_view value = argv[++i];
        if (flag == "--backend" && (value == "gpu" || value == "cpu" || value == "auto")) o.backend = value;
        else if (flag == "--layout" && value == "warp") o.layout = Layout::warp;
        else if (flag == "--layout" && value == "grid") o.layout = Layout::grid;
        else if (flag == "--layout" && value == "block") o.layout = Layout::block;
        else if (flag == "--layout" && value == "auto") o.layout = Layout::automatic;
        else if (flag == "--learn" && value == "no-scaling") o.learning = Learning::no_scaling;
        else if (flag == "--learn" && value == "full") o.learning = Learning::full;
        else if (flag == "--learn" && value == "motor") o.learning = Learning::motor;
        else if (flag == "--learn" && value == "off") o.learning = Learning::frozen;
        else if (flag == "--rule" && (value == "normal" || value == "flipped")) o.reversed = (value == "flipped");
        else if (flag == "--recall" && value == "cortex") o.recall = Recall::cortical;
        else if (flag == "--recall" && value == "content") o.recall = Recall::content;
        else if (flag == "--recall" && value == "full") o.recall = Recall::associative;
        else if (flag == "--vocabulary") o.vocabulary = number(value, 8, 4096, flag);
        else if (flag == "--archive") o.archive = number(value, 0, 65536, flag);
        else if (flag == "--consolidation") o.consolidation = number(value, 1, 1024, flag);
        else if (flag == "--plan-nodes") o.plan_nodes = number(value, 0, 65536, flag);
        else if (flag == "--chain") o.chain = number(value, 1, 8, flag);
        else if (flag == "--experiments" && value == "off") o.experiments = Course::Experiments::off;
        else if (flag == "--experiments" && value == "random") o.experiments = Course::Experiments::random;
        else if (flag == "--experiments" && value == "curious") o.experiments = Course::Experiments::curious;
        else if (flag == "--experiments" && value == "paired") o.experiments = Course::Experiments::paired;
        else if (flag == "--effects" && value == "regular") o.difficult = 0;
        else if (flag == "--effects" && value == "novel") o.difficult = 1;
        else if (flag == "--effects" && value == "dense") o.difficult = 2;
        else if (flag == "--effects" && value == "nonlinear") o.difficult = 3;
        else if (flag == "--survey") o.survey = number(value, 0, 128, flag);
        else if (flag == "--explore" && (value == "curious" || value == "random")) o.random_walk = value == "random";
        else if (flag == "--lesson-seed") o.lesson_seed = number(value, 0, umax, flag);
        else if (flag == "--trial") o.trial = number(value, 0, umax, flag);
        else if (flag == "--animal") o.animal = number(value, 0, 65535, flag);
        else if (flag == "--lessons") o.lessons = number(value, 1, 10000, flag);
        else if (flag == "--feedback" && (value == "real" || value == "shuffled")) o.scrambled = value == "shuffled";
        else if (flag == "--task") {
            auto it = std::find(std::begin(names), std::end(names), value);
            if (it == std::end(names)) throw std::invalid_argument("unknown task " + std::string(value));
            o.task = Task(it - std::begin(names));
        }
        else if (flag == "--brains") o.brains = number(value, 1, 65536, flag);
        else if (flag == "--neurons") o.neurons = number(value, 64, 1u << 20, flag);
        else if (flag == "--episodes") o.episodes = number(value, 1, 10000000, flag);
        else if (flag == "--eval-episodes") o.probes = number(value, 1, 10000000, flag);
        else if (flag == "--report-every") o.report = number(value, 1, 10000000, flag);
        else if (flag == "--seed") o.seed = number(value, 0, umax, flag);
        else if (flag == "--world-seed") o.world_seed = number(value, 0, umax, flag);
        else if (flag == "--threads") o.threads = number(value, 1, 1024, flag);
        else if (flag == "--generations") o.generations = number(value, 1, 100000, flag);
        else if (flag == "--repeats") o.repeats = number(value, 1, 51, flag);
        else if (flag == "--load") o.load = value;
        else if (flag == "--save") o.save = value;
        else throw std::invalid_argument("invalid option " + std::string(flag) + " " + std::string(value));
    }
    if (o.command == "detector" && (o.task != Task::detector || o.episodes != 8 || o.probes != 1))
        throw std::invalid_argument("the detector study uses eight observations and one test choice");
    if (o.command == "transfer" && o.task && !is_lesson(*o.task))
        throw std::invalid_argument("transfer requires a lesson task");
    if (o.swap_words && o.command != "eval" && o.command != "replay" && o.command != "transfer")
        throw std::invalid_argument("--swap-words applies to errand probes");
    if (o.mute && o.command != "eval" && o.command != "replay") throw std::invalid_argument("--mute-odor applies to eval and replay");
    if (!o.save.empty() && o.command != "train" && o.command != "curriculum" && o.command != "evolve" && o.command != "tutor")
        throw std::invalid_argument("--save applies to train, evolve, curriculum and tutor");
    if (o.task && o.command == "curriculum") throw std::invalid_argument("curriculum sets its own tasks");
    if (o.backend == "cpu" && o.layout != Layout::automatic)
        throw std::invalid_argument("--layout selects a GPU mapping");
    return o;
}

Model prepare(const Options &o) {
    Word seed = o.seed.value_or(1);
    Word width = o.neurons.value_or(128);
    Word default_animals = width > 4096 ? (o.command == "evolve" ? 16 : 1)
                                        : (o.command == "bench" ? 2048 : 128);
    if (o.command == "tutor" || o.command == "replay") default_animals = 1;
    auto m = o.load.empty()
        ? hatch(o.brains.value_or(default_animals), width,
                seed, {o.command == "evolve" ? Task::context : (o.command == "replay" ? Task::errand : Task::scent), mix(seed ^ 0xa35b73d9u), 0, 0})
        : load_model(o.load);
    if (o.brains.value_or(m.animals) != m.animals || o.neurons.value_or(m.neurons) != m.neurons
        || o.seed.value_or(m.seed) != m.seed)
        throw std::invalid_argument("checkpoint wiring seed and shape are fixed");
    if (o.command == "evolve" && (m.animals < 4 || m.generation > std::numeric_limits<Word>::max() - o.generations))
        throw std::invalid_argument("evolve needs at least four animals and an available generation number");
    if (o.layout == Layout::warp && m.neurons > 128)
        throw std::invalid_argument("warp layout supports 64 or 128 neurons");
    if (o.swap_words && o.task.value_or(m.course.task) != Task::errand)
        throw std::invalid_argument("--swap-words requires the errand task");
    m.course.swap_words = o.swap_words;
    if (o.recall) m.course.recall = *o.recall;
    if (o.lesson_seed) m.course.lesson = *o.lesson_seed;
    if (o.vocabulary) {
        if (*o.vocabulary & (*o.vocabulary - 1)) throw std::invalid_argument("--vocabulary requires a power of two");
        m.course.vocabulary = *o.vocabulary;
    }
    if (o.archive) resize_archive(m, *o.archive);
    else if (o.vocabulary && *o.vocabulary > memory_rows && !m.course.archive_slots) resize_archive(m, 4 * *o.vocabulary);
    if (o.consolidation) m.course.consolidation = *o.consolidation;
    if (o.experiments) m.course.experiments = *o.experiments;
    if (o.plan_nodes) m.course.plan_nodes = *o.plan_nodes;
    if (o.chain) m.course.chain = *o.chain;
    if (o.difficult) m.course.difficult = *o.difficult;
    if (o.survey) m.course.survey_steps = *o.survey;
    if (o.random_walk) m.course.random_walk = *o.random_walk;
    if (o.scrambled) m.course.scrambled = *o.scrambled;
    if (o.command == "transfer" || o.command == "evolve" || o.command == "detector") {
        m.memory.clear();
        m.maps.clear();
        m.drawings.clear();
        m.archive.clear();
        m.interventions.clear();
    }
    if (o.learning) m.learning = *o.learning;
    if (o.task) m.course.task = *o.task;
    if (o.reversed) m.course.reversed = *o.reversed;
    if (o.world_seed && o.command != "eval" && o.command != "replay") m.course.seed = *o.world_seed;
    auto stages = o.command == "curriculum" ? 5u : 1u;
    if ((o.command == "train" || o.command == "curriculum")
        && std::uint64_t(m.cursor) + std::uint64_t(stages) * o.episodes > std::numeric_limits<Word>::max())
        throw std::invalid_argument("episode cursor would overflow");
    return m;
}

std::unique_ptr<Engine> backend(const Options &o, const Model &model) {
    if (o.backend == "cpu") return make_cpu(model, o.threads);
    try {
        return make_gpu(model, o.layout);
    } catch (const std::exception &err) {
        if (o.backend == "gpu" || o.layout != Layout::automatic) throw;
        std::cerr << "CUDA unavailable (" << err.what() << "). Using CPU.\n";
        return make_cpu(model, o.threads);
    }
}

Totals sum(const Run &r) {
    Totals result;
    for (auto a : r.animals) {
        result.wins += a.wins;
        result.ticks += a.ticks;
        result.spikes += a.spikes;
        result.matches += a.matches;
    }
    return result;
}

void report(std::string_view phase, Course c, Word episodes, const Run &r, Word neurons) {
    auto t = sum(r);
    if (auto *j=journal::current()) j->scores.push_back({std::string(phase)+" "+std::string(names[unsigned(c.task)]),t.wins,std::uint64_t(episodes)*r.animals.size()});
    double trials = double(r.animals.size()) * episodes;
    double mean = double(t.wins) / trials;
    double sq = 0;
    for (auto a : r.animals) sq += std::pow(double(a.wins) / episodes - mean, 2);
    auto n = r.animals.size();
    double se = n > 1 ? std::sqrt(sq / (double(n) * (n - 1))) : 0;
    std::cout << std::left << std::setw(12) << phase << std::setw(11) << names[unsigned(c.task)]
              << (c.reversed ? "flipped" : "normal ") << std::right << std::fixed
              << std::setw(11) << static_cast<std::uint64_t>(trials)
              << std::setw(10) << std::setprecision(2) << 100 * mean << '%';
    if (n > 1) std::cout << std::setw(9) << 100 * se;
    else std::cout << std::setw(9) << "n/a";
    std::cout << std::setw(10) << 100.0 * double(t.spikes) / (double(t.ticks) * (is_drawing(c.task) ? 32 : neurons)) << '%';
    if (is_drawing(c.task)) std::cout << "  pixels=" << 100 * t.matches / (trials * 32) << '%';
    std::cout << '\n';
}

void probe(Engine &engine, const Model &m, Course c, Word episodes, std::string_view label) {
    report(label, c, episodes, engine.run(c, 0, episodes, Learning::frozen), m.neurons);
}

Course held_out(Course course) {
    course.seed ^= 0xe62f3a51u;
    course.novel = 1;
    return course;
}

void stage(Engine &engine, Model &m, const Options &o) {
    auto test = held_out(m.course);
    probe(engine, m, test, o.probes, "before");
    for (Word done = 0; done < o.episodes;) {
        Word n = std::min(o.report ? o.report : o.episodes, o.episodes - done);
        auto r = engine.run(m.course, m.cursor, n, m.learning);
        report("online", m.course, n, r, m.neurons);
        done += n;
        m.cursor += n;
    }
    probe(engine, m, test, o.probes, "after");
}

void benchmark(Engine &engine, const Model &m, const Options &o) {
    for (auto mode : {Learning::frozen, Learning::full}) {
        engine.restore(m);
        Course course = mode == Learning::frozen && is_lesson(m.course.task) ? held_out(m.course) : m.course;
        engine.run(course, 0, 4, mode);
        std::vector<double> times, walls;
        Totals t;
        for (Word r = 0; r < o.repeats; ++r) {
            engine.restore(m);
            auto result = engine.run(course, 4, o.episodes, mode);
            times.push_back(result.seconds);
            walls.push_back(result.wall_seconds);
            t = sum(result);
        }
        auto median = [](std::vector<double> &samples) {
            std::sort(samples.begin(), samples.end());
            return (samples[samples.size() / 2] + samples[(samples.size() - 1) / 2]) * 0.5;
        };
        double seconds = median(times);
        double steps = double(t.ticks) * (is_drawing(m.course.task) ? 32 : m.neurons);
        if (auto *j=journal::current()) {
            std::string label=mode==Learning::full ? "Online learning" : "Frozen inference";
            j->scores.push_back({label,t.wins,std::uint64_t(m.animals)*o.episodes});
            j->metrics[label+" trials per second"]=std::uint64_t(double(m.animals)*o.episodes/seconds);
        }
        std::cout << (mode == Learning::full ? "online learning" : "frozen inference")
                  << std::fixed << std::setprecision(3) << "  median_ms=" << seconds * 1e3
                  << "  wall_ms=" << median(walls) * 1e3
                  << (is_drawing(m.course.task) ? "  Goutput_bits/s=" : "  Gneuron_ticks/s=") << steps / seconds * 1e-9
                  << "  trials/s=" << std::setprecision(0) << double(m.animals) * o.episodes / seconds
                  << "  firing=" << std::setprecision(2) << 100 * t.spikes / steps << '%'
                  << engine.resources(mode) << '\n';
    }
}

void evolution(Engine &engine, Model &m, const Options &o) {
    auto score = [&](Word seed, Word pairs, std::string_view label) {
        std::vector<std::uint64_t> fitness(m.animals);
        for (Word pair = 0; pair < pairs; ++pair) {
            Course c = m.course;
            c.seed = pair ? mix(seed + pair) : seed;
            auto outcome = lifetime_scores(engine, m, c, o.episodes, o.probes, m.learning);
            for (Word i = 0; i < m.animals; ++i) fitness[i] += outcome[i];
        }
        double trials = 2.0 * pairs * o.probes;
        double sum = std::accumulate(fitness.begin(), fitness.end(), 0.0);
        if (auto *j=journal::current(); j && label=="audit")
            j->scores.push_back({"Held-out generation "+std::to_string(m.generation),std::accumulate(fitness.begin(),fitness.end(),std::uint64_t{}),std::uint64_t(2)*pairs*o.probes*m.animals});
        std::cout << label << "  generation=" << m.generation << "  lifetimes/genome=" << 2 * pairs
                  << "  mean_after_learning=" << std::fixed << std::setprecision(2)
                  << 100 * sum / (trials * m.animals) << "%  best="
                  << 100 * double(*std::max_element(fitness.begin(), fitness.end())) / trials << "%\n";
        return fitness;
    };
    Word audit = mix(m.course.seed ^ 0x3e749b5du);
    score(audit, 8, "audit");
    for (Word g = 0; g < o.generations; ++g) {
        Word seed = trial_key(m.course.seed, 0x9b18ae45u, m.generation);
        auto fitness = score(seed, o.lessons, "selection");
        breed(m, fitness);
    }
    score(audit, 8, "audit");
    // Scoring always leaves a trained lifetime on the device. The heritable
    // population, including the final generation, lives in the host Model.
    engine.restore(m);
}

void transfer(Engine &engine, const Model &initial, const Options &o) {
    std::vector<Word> budgets{0, 1};
    while (budgets.back() < o.episodes)
        budgets.push_back(std::min(o.episodes, budgets.back() * 2));
    std::cout << "task          examples    lesson-worlds         probes    success    SE(pp)\n";
    for (auto task : {Task::naming, Task::sorting, Task::pattern, Task::routes, Task::landmarks, Task::errand, Task::workshop, Task::invention, Task::seminar, Task::detector}) {
        if (o.task && task != *o.task) continue;
        std::vector<std::vector<std::uint64_t>> wins(budgets.size(), std::vector<std::uint64_t>(initial.animals));
        std::vector<std::uint64_t> pixels(budgets.size());
        for (Word lesson = 0; lesson < o.lessons; ++lesson) {
            engine.restore(initial);
            Course c = initial.course;
            c.task = task;
            c.lesson = trial_key(c.lesson, 0x729a60f3u, lesson);
            c.seed = trial_key(c.seed, 0x6c35d84bu, lesson);
            Word trained = 0;
            for (size_t j = 0; j < budgets.size(); ++j) {
                Word budget = budgets[j];
                if (budget > trained) engine.run(c, trained, budget - trained, initial.learning);
                trained = budget;
                auto out = engine.run(held_out(c), 0, o.probes, Learning::frozen);
                for (Word i = 0; i < initial.animals; ++i) {
                    wins[j][i] += out.animals[i].wins;
                    pixels[j] += out.animals[i].matches;
                }
            }
        }
        for (size_t j = 0; j < budgets.size(); ++j) {
            double per_brain = double(o.probes) * o.lessons;
            double mean = std::accumulate(wins[j].begin(), wins[j].end(), 0.0) / (per_brain * initial.animals);
            if (auto *log=journal::current())
                log->scores.push_back({std::string(names[unsigned(task)])+" after "+std::to_string(budgets[j])+" examples",std::accumulate(wins[j].begin(),wins[j].end(),std::uint64_t{}),std::uint64_t(o.probes)*o.lessons*initial.animals});
            double squares = 0;
            for (auto count : wins[j]) squares += std::pow(count / per_brain - mean, 2);
            std::cout << std::left << std::setw(12) << names[unsigned(task)] << std::right
                      << std::setw(10) << budgets[j] << std::setw(17) << std::uint64_t(o.lessons) * initial.animals
                      << std::setw(15) << std::uint64_t(o.probes) * o.lessons * initial.animals
                      << std::setw(10) << std::fixed << std::setprecision(2) << mean * 100 << '%';
            if (initial.animals > 1)
                std::cout << std::setw(9) << 100 * std::sqrt(squares / (double(initial.animals) * (initial.animals - 1)));
            else std::cout << std::setw(9) << "n/a";
            if (is_drawing(task))
                std::cout << "  pixels=" << 100 * pixels[j] / (32 * per_brain * initial.animals) << '%';
            std::cout << '\n';
        }
    }
}

int run(const Options &o) {
    auto m = prepare(o);
    if (auto *j=journal::current()) { j->metrics["animals"]=m.animals;j->metrics["neurons"]=m.neurons;j->metrics["seed"]=m.seed;j->metrics["generation"]=m.generation; }
    auto engine = backend(o, m);
    if (auto *j=journal::current()) j->device=engine->name();
    std::cout << engine->name() << "  animals=" << m.animals << "  neurons/animal=" << m.neurons
              << "  weight_bytes/animal=" << m.neurons * sizeof(Weights) << "  seed=" << m.seed
              << "  train_world_seed=" << m.course.seed << "  cursor=" << m.cursor << "  generation=" << m.generation << '\n';
    if (m.course.archive_slots || m.course.vocabulary > 8)
        std::cout << "vocabulary=" << m.course.vocabulary << "  archive_slots/animal=" << m.course.archive_slots
                  << "  archive_bytes/animal=" << m.course.archive_slots * sizeof(Association) << '\n';
    if (o.command == "detector") {
        std::cout << "Eight observations per learner. One new tray choice with learning frozen.\n";
        for (Word rule = 0; rule < 2; ++rule) {
            if (o.reversed && rule != *o.reversed) continue;
            engine->restore(m);
            Course c = m.course;
            c.reversed = rule;
            engine->run(c, 0, 8, m.learning);
            auto probe = engine->run(held_out(c), 0, 1, Learning::frozen);
            auto totals = sum(probe);
            if (auto *j=journal::current()) j->scores.push_back({rule ? "Different activates" : "Same activates",totals.wins,m.animals});
            std::cout << (rule ? "Different activates" : "Same activates") << "  digital first choices="
                      << totals.wins << '/' << m.animals << " (" << std::fixed << std::setprecision(2)
                      << 100.0 * totals.wins / m.animals << "%)  published toddler choices=15/19\n";
        }
        std::cout << "Physical study and symbolic protocol are described in docs/transfer.md.\n";
        return 0;
    }
    if (o.command == "replay") {
        if (o.animal >= m.animals) throw std::invalid_argument("selected animal is outside this population");
        Course c = held_out(m.course);
        if (o.world_seed) c.seed = *o.world_seed;
        c.mute_odor = o.mute;
        if (c.task != Task::errand && c.task != Task::routes && !is_drawing(c.task))
            throw std::invalid_argument("replay supports routes, errand, workshop and invention");
        auto observed = engine->run(c, o.trial, 1, Learning::frozen);
        if (auto *j=journal::current()) j->scores.push_back({std::string(names[unsigned(c.task)])+" replay",observed.animals[o.animal].wins,1});
        replay(std::cout, c, o.animal, o.trial, observed.animals[o.animal]);
        return 0;
    }
    if (o.command == "transfer") {
        transfer(*engine, m, o);
        return 0;
    }
    if (o.command == "bench") {
        std::cout << "task=" << names[unsigned(m.course.task)] << "  trials/animal/run=" << o.episodes
                  << "  measured_runs=" << o.repeats << '\n';
        benchmark(*engine, m, o);
        return 0;
    }
    if (o.command != "evolve" && o.command != "tutor") std::cout << "phase       task       rule        trials   success    SE(pp)     firing\n";
    if (o.command == "eval") {
        Course c = held_out(m.course);
        if (o.world_seed) c.seed = *o.world_seed;
        c.mute_odor = o.mute;
        probe(*engine, m, c, o.episodes, o.mute ? "odorless" : "held-out");
    } else if (o.command == "curriculum") {
        for (auto t : {Task::scent, Task::delay, Task::context, Task::trail}) {
            m.course.task = t;
            stage(*engine, m, o);
        }
        m.course.task = Task::scent;
        m.course.reversed ^= 1;
        stage(*engine, m, o);
        auto old = held_out(m.course);
        old.reversed ^= 1;
        for (auto t : {Task::scent, Task::delay, Task::context, Task::trail}) {
            old.task = t;
            probe(*engine, m, old, o.probes, "retention");
        }
    } else if (o.command == "tutor") tutor(*engine, m, std::cin, std::cout, ::isatty(STDIN_FILENO));
    else if (o.command == "evolve") evolution(*engine, m, o);
    else stage(*engine, m, o);
    if (!o.save.empty()) {
        engine->download(m);
        save_model(m, o.save);
        std::cout << "Saved " << o.save << " at episode " << m.cursor << ".\n";
    }
    return 0;
}
} // namespace

int main(int argc, char **argv) {
    try {
        return axion::journal::execute(argc,argv,[](int argc,char **argv) {
        if (argc > 1 && (std::string_view(argv[1]) == "dialogue" || std::string_view(argv[1]) == "talk" || std::string_view(argv[1])=="commons")) return axion::life::dialogue_command(argc, argv);
        if (argc > 1 && std::string_view(argv[1]) == "floodplain") return axion::life::flood::command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "genesis") return axion::genesis::command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "mind") return axion::mind::command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "ground") return axion::life::grounding::command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "imitate") return axion::life::procedure::command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "compose") return axion::life::composition::command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "develop") return axion::life::develop_command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "recollect") return axion::life::recollection_command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "attention") return axion::life::attention_command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "imagine") return axion::life::imagery_command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "relate") return axion::life::binding_command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "conjoin") return axion::life::conjunction_command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "narrate") return axion::life::report_command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "cultivate") return axion::life::cultivate_command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "infer") return axion::life::concept_command(argc,argv);
        if (argc > 1 && std::string_view(argv[1]) == "agent") return axion::life::actor_command(argc, argv);
        if (argc > 1 && std::string_view(argv[1]) == "life") return axion::life::command(argc, argv);
        if (argc == 1 || std::find(argv + 1, argv + argc, "--help"sv) != argv + argc) {
            usage();
            return 0;
        }
        return run(parse(argc, argv));
        });
    } catch (const std::exception &error) {
        std::cerr << "axion: " << error.what() << '\n';
        return 1;
    }
}
