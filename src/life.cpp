#include "axion/actor.hpp"
#include "axion/habitat_view.hpp"
#include "axion/journal.hpp"
#include "journal_population.hpp"
#include "axion/passage.hpp"
#include "life_io.hpp"
#include "life_runtime.hpp"
#include "launch_graph.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <system_error>
#include <tuple>
#include <vector>
#include <unistd.h>

namespace axion::life {
namespace {
using namespace cuda;
using namespace runtime;
using namespace storage;
constexpr std::array<std::string_view, 21> contest_names{"off", "ancestry", "plasticity", "brain", "body", "agency", "dendrites", "second-state", "paths","waves","contexts","evidence","outcomes","outcome-plasticity","outcome-chain","navigation","navigation-plasticity","navigation-range","temporal","temporal-plasticity","temporal-span"};
constexpr std::array<std::string_view, 5> habitat_names{"meadow", "grove", "compound", "coupled","thicket"};
constexpr std::array<std::string_view,4> inheritance_names{"standard","modular","uniform","cost-only"};
constexpr std::array<std::string_view,4> flora_names{"evolving","clonal","fixed","autonomous"};
constexpr std::array<std::string_view,3> fruit_names{"scattered","persistent","inherited"};
constexpr std::array<std::string_view,5> memory_tasks{"arbitrary","frames","paired","exceptions","interleaved"};
constexpr std::array<std::string_view,12> law_names{"composition","parity","mixed-laws","law-switch","kinds","unstructured","transposed","mosaic","latent-table","chains","islands","parity-chains"};
constexpr std::string_view help = R"(axion life  |  shared CUDA ecosystem
  ./axion life --side 128 --neurons 256 --steps 2048 --report-every 512 --save world.eco
  ./axion life --load world.eco --steps 2048 --save continued.eco
  ./axion life --founders world.eco --seed 107 --steps 512 --map

  --nursery N        optional generations of preselection for learning, before a new habitat starts (1..4096)
  --nursery-varieties N   paired values per property in founding selection, 4, 8 or 16 (4)
  --nursery-problem NAME   paired challenge, varied, revisit, lifelong, repertoire, integrated, outcomes, subsistence, navigation, migration, or junctions
  --practice-schedule fixed|short-long   during outcome breeding, alternate single-trial and extended experience
  --breeding select|drift|cross|extend|splice|organ|map|history|history-local   full selection, drift, crossing, context-only, or organ-only founding treatments
  --relations off|on|waves|contexts   plastic relays, wave propagation and competing context memories
  --programs 1|2     heritable context programs sharing the same acquired synapses (1)
  --polarity shared|evolvable   permit each expressed program to inherit different resting contact polarities
  --synapse-bits N   one or two mutable bits per receptor-motor connection (1)
  --season N         base ticks between territorial chemistry changes (8..65536, default scales with varieties)
  --species N        values per fruit property, 4..32 in groves or 4..16 in compound groves (powers of two)
  --digestion N      delayed grove with an eight-tick heritable conduction path and 1..8 ticks to absorb food
  --probe-delay N    feedback delay in the independent conditioning assay, 1..8 (1)
  --habitat meadow|grove|compound|coupled|thicket   vegetation, seasonal fruit or locally reproducing plants
  --flora evolving|clonal|fixed|autonomous   plant reproduction and mutation in a thicket (evolving)
  --fruit scattered|persistent|inherited   resample after a grasp, keep unopened fruit, or also inherit its appearance
  --metabolism energy|balanced|food-fuel   convert four complementary foods into energy in a thicket
  --leach N          lose up to N units of the locally dilute mineral per tick in food-fuel mode (0..8)
  --husk N           fruit handling time, 0 unrestricted or 1..3 for 16, 8, or 4 units per tick
  --mouth-origin inherited|whole|random   initial bite subdivision alleles in a handling-time experiment
  --seep N           absorb up to N units of each locally abundant material from the ground (0..8)
  --stand-size N    side length of a clonal founding stand (power of two, default 4)
  --basin-width N     repeated habitats in independent local orientations, with a regional visual cue (0 or power of two >=8)
  --spring-flow N     four aquifers per 32 sites supply local minerals in a food-fuel thicket (0..32)
  --history-origin inherited|inert|random   initialize temporal rule variation before breeding
  --junction-types N   independently permuted junctions per wing (1..3)
  --junction-pupils N  rank sampled genotypes across independent junction environments
  --junctions N      learn private door effects at identical junctions in four wings
  --retention N      intervening sensory events before a test choice (0..256)
  --practice-retention N   intervening events while learning (0..64)
  --motor-clock lifetime|mixed   use the individual actor clock or independent tie-break draws
  --passage-test paired|continuous|mixed|frozen   paired active approaches, a continuing lifetime, both schedules, or a frozen diagnostic
  --practice-timing fixed|variable   hold the practice delay constant or vary it across visits
  --passage-noise repeat|varied|novel|mixed   repeated texture, familiar distractors, new distractors, or both distractor sets
  --practice-noise repeat|varied  texture during learning
  --junction-cues shared|distinct  identical intersections or a visible spatial control
  --navigation on|exploratory|competitive|episodic|attentive|associative|predictive|off   express mutable sensory transition circuits in a thicket
  --crossings N        test N private directed worlds with a changed distant exit
  --distance N         directed corridor length, 2..7
  --spring-vision distinct|paired|shared   appearances of different rooms in private foraging catchments
  --migration N        forage and reproduce using actual intake from four springs on a private cyclic map
  --mazes N            test on N private cyclic maps with moved shortcuts and several routes
  --rooms N            8, 16 or 32 distinguishable places per maze
  --exploration local|roam   visit each room or choose all movements in one continuous outing
  --locomotion hand|foot   cross physical doorways with a grasp or with the habitat movement port
  --history fresh|before|after|mixed   let prior motor experience compete with navigation
  --destination landmark|food|mixed   cue a visible goal, a physiological need, or alternate
  --diversion doors|food|both   reroute distant exits or change what food a visited place supplies
  --forage-profile   report resident ages and physical grip accessibility
  --colonize SEED    hatch a new habitat from a sample of living residents, preserving their ancestry
  --passages N       colonize and simulate N successive habitats for --steps ticks each (1)
  --settlers N       maximum distinct residents eligible to found each new habitat (256)
  --resident-index auto|warp|grid   CUDA scheduling for resident sampling and dispersal
  --vegetation novel|repeat   draw new producer genomes per passage or repeat the original planting
  --climates N       recurrent local climates in a thicket, 1, 2 or 4, each with independently mutable plant reactions
  --climate-cue on|off   expose the current atmospheric light bands to the animals (off)
  --side N           toroidal habitat width, power of two from 8 to 1024 (64)
  --neurons N        maximum spiking cells per creature, powers of two from 64 to 1024 (256)
  --density N        initial percentage of inhabited sites, 0..100 (35)
  --seed N           founder and habitat seed (1)
  --steps N          additional ecological ticks (1024)
  --report-every N   census interval in ticks (end of run by default)
  --plasticity on|off  express or freeze the two neural synaptic gate planes
  --inheritance standard|modular|uniform|cost-only   express inherited mutation rates and replication costs
  --modifier-origin inherited|zero|random   initialization of the four copying modifiers at founding
  --mutation on|off    permit or suppress heritable edits at birth
  --behavior neural|random  use the neural motor output or a matched random action
  --contest NAME     ancestry, brain, body, plasticity, agency, dendrites, second-state, paths, waves,
                     contexts, evidence, outcomes, outcome-plasticity or outcome-chain
  --swap-cohorts     exchange the two treatments at the same founding positions
  --load FILE        resume an entire world exactly, including its history
  --reset-contacts   return living residents' acquired sensory and associative contacts to their developmental values once
  --founders FILE    seed a fresh world with genomes from living descendants
  --founder-site N   hatch independent descendants of one occupied donor site
  --context-founders FILE  import only the context germline from living donors, repeat to intermix populations
  --plant-founders FILE   transplant producer genomes onto fresh ground, retaining their spatial arrangement
  --food-genes FILE       transplant only the inherited flavour-to-food chemistry of a producer population
  --graft MODULE FILE   exchange body, soma, dendrites, pathways, memory, outcomes, fidelity, navigation or history genes
  --alternate-founders FILE   seed the second program from a donor population, retaining the recipient's switching circuit
  --save FILE        atomically save the complete world at the last tick
  --conditioning N   test N independent rule sets with counterbalanced lifetimes (2..8192)
  --growth N         measure births and nutrient use in N independent common gardens
  --forage-cues single|paired   dimensionality of food cues during the growth assay
  --pupils N         screen N resident genotypes for revaluation and retest leading distinct genotypes on fresh hidden worlds
  --appetites N      test resident choices for four needs on the actual plants where they live
  --valuation N      test choice under four bodily needs and a passive change in food composition
  --outcome-task direct|inferred   supply nutrients during object practice, or only during later passive exposure
  --pairings N       passive exposures per taste before each frozen choice test (1..32)
  --value-controls on|off   matched sensory, contact and goal interventions
  --revisit N        test N independent sets of recurring environments in uninterrupted lifetimes
  --regimes N        number of incompatible contexts per lifetime (2..4)
  --memory-task arbitrary|frames|paired|exceptions|interleaved   recurring worlds with private sensorimotor mappings
  --landmarks hidden|visible   ambient observations during context changes
  --memory-controls on|off   include matched reminder and circuit interventions in the revisit assay (on)
  --dissect          count living update rules and restore one ancestral table entry at a time in the assay
  --problem NAME     grips, dimensions, composition, parity, mixed-laws, law-switch, kinds, transposed, mosaic, latent-table, unstructured, chains, islands, parity-chains
  --varieties N      values per attribute, 2..32 for grips or 4..16 for dimensions (powers of two)
  --exposures N      encounters per variety before and after a reversal (1..64, default 16)
  --probe-seed N     fresh hidden assignments and practice order in the conditioning assay
  --assay N          probe each living brain with N paired sensory histories (1..256)
  --survey N         sample up to N living donors, report exact motor repertoires and representative sites (1..65536)
  --survey-seed N    fixed experiences and common tie breaks in the behavioral survey
  --schedule auto|dense|occupied   assign groups per site or pack living bodies into warps (auto)
  --dispatch graph|direct   batch 32 ecological ticks per CUDA graph submission (default graph)
  --map              show the final spatial distribution of energy and plants
  --backend gpu      run all neural updates, selection, ecology and mutation on CUDA
)";
Word parse_word(std::string_view s, Word lo, Word hi, std::string_view flag) {
    Word n;
    auto [p, err] = std::from_chars(s.data(), s.data() + s.size(), n);
    if (err != std::errc{} || p != s.data() + s.size() || n < lo || n > hi)
        throw std::invalid_argument("invalid value for " + std::string(flag));
    return n;
}
bool toggle(std::string_view value) {
    if (value == "on") return true;
    if (value == "off") return false;
    throw std::invalid_argument("expected on or off");
}
struct Options {
    std::optional<Word> side, neurons, seed, density, mutate, plastic, random;
    std::optional<Word> founder_site;
    Word steps = 1024, interval = 0, probes = 0, nursery = 0;
    Word survey = 0, survey_seed = 473327;
    std::optional<Word> colonize, passages, settlers, mouths;
    std::optional<bool> novel_plants;
    std::optional<bool> grid_index;
    std::string load, save, ancestors;
    std::vector<std::string> context_ancestors;
    bool map = false, swap = false, graphs = true, dissect = false, joint_nursery = false;
    Word breeding=0;
    std::optional<bool> packed;
    JointLaw nursery_law = JointLaw::ring;
    NurseryCourse course=NurseryCourse::paired;
    Revisit revisit{};
    Valuation value{};
    Word appetites=0,growth=0,paired_food=1,pupils=0;
    bool value_controls=true;
    bool short_practice=false;
    bool memory_controls=true, reset_contacts=false, forage_profile=false;
    std::string alternate_founders,plant_founders,food_genes;
    std::vector<std::pair<Graft,std::string>> grafts;
    Word nursery_varieties = 4;
    Conditioning panel{0, 4, 16, 173167};
    std::optional<Word> contest;
    std::optional<Habitat> habitat;
    std::optional<Flora> flora;
    std::optional<Fruit> fruit;
    std::optional<Word> digestion, species, synapse_bits, season, relations;
    std::optional<Word> climates, climate_cue, programs, polarity, balanced, stand, seep, leach, repair, modifier_origin, husk, navigation, springs, basin_width;
    Travel travel{};JunctionTrial passage{};Word passage_pupils=0,trace_origin=0,vision=0;
    Word mazes=0,migration=0,rooms=16,roam=0;
};
Options options(int argc, char **argv) {
    Options o;
    for (int i = 2; i < argc; ++i) {
        std::string_view f = argv[i];
        if (f == "--swap-cohorts") { o.swap = true; continue; }
        if (f=="--forage-profile") { o.forage_profile=true; continue; }
        if (f=="--reset-contacts") { o.reset_contacts=true; continue; }
        if (f == "--dissect") { o.dissect = true; continue; }
        if (f == "--map") { o.map = true; continue; }
        if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
        std::string_view v = argv[i];
        if (f=="--navigation") o.navigation=v=="predictive" ? 7u : v=="associative" ? 6u : v=="attentive" ? 5u : v=="episodic" ? 4u : v=="competitive" ? 3u : v=="exploratory" ? 2u : Word(toggle(v));
        else if (f=="--motor-clock" && (v=="lifetime" || v=="mixed")) o.passage.lifetime_clock=v=="lifetime";
        else if (f=="--passage-test" && (v=="paired" || v=="continuous" || v=="frozen" || v=="mixed")) o.passage.traversal=v=="mixed" ? 3u : v=="frozen" ? 2u : Word(v=="continuous");
        else if (f=="--practice-timing" && (v=="fixed" || v=="variable")) o.passage.variable=v=="variable";
        else if (f=="--history-origin" && (v=="inherited" || v=="inert" || v=="random")) o.trace_origin=v=="random" ? 2u : Word(v=="inert");
        else if (f=="--junction-types") o.passage.places=parse_word(v,1,3,f);
        else if (f=="--junction-pupils") o.passage_pupils=parse_word(v,1,65536,f);
        else if (f=="--junctions") o.passage.sets=parse_word(v,1,8192,f);
        else if (f=="--retention") o.passage.delay=parse_word(v,0,256,f);
        else if (f=="--practice-retention") o.passage.practice_delay=parse_word(v,0,64,f);
        else if (f=="--passage-noise" && (v=="repeat" || v=="varied" || v=="novel" || v=="mixed")) o.passage.noise=v=="mixed" ? 3u : v=="novel" ? 2u : Word(v=="varied");
        else if (f=="--practice-noise" && (v=="repeat" || v=="varied")) o.passage.practice_noise=v=="varied";
        else if (f=="--junction-cues" && (v=="shared" || v=="distinct")) o.passage.visible=v=="distinct";
        else if (f=="--basin-width") o.basin_width=parse_word(v,0,1024,f);
        else if (f=="--spring-flow") o.springs=parse_word(v,0,32,f);
        else if (f=="--crossings") o.travel.sets=parse_word(v,1,8192,f);
        else if (f=="--distance") o.travel.depth=parse_word(v,2,7,f);
        else if (f=="--spring-vision" && (v=="distinct" || v=="paired" || v=="shared")) o.vision=v=="shared" ? 2u : Word(v=="paired");
        else if (f=="--migration") o.migration=parse_word(v,1,8192,f);
        else if (f=="--mazes") o.mazes=parse_word(v,1,8192,f);
        else if (f=="--rooms") { o.rooms=parse_word(v,8,32,f); if (!std::has_single_bit(o.rooms)) throw std::invalid_argument("maze size must be 8, 16 or 32"); }
        else if (f=="--exploration" && (v=="local" || v=="roam")) o.roam=v=="roam";
        else if (f=="--locomotion" && (v=="hand" || v=="foot")) o.travel.foot=v=="foot";
        else if (f=="--history" && (v=="fresh" || v=="before" || v=="after" || v=="mixed"))
            o.travel.history=v=="mixed" ? 3u : v=="after" ? 2u : Word(v=="before");
        else if (f=="--destination" && (v=="landmark" || v=="food" || v=="mixed")) o.passage.motivation=o.travel.motivation=v=="mixed" ? 2u : Word(v=="food");
        else if (f=="--diversion" && (v=="doors" || v=="food" || v=="both")) o.travel.change_food=v=="both" ? 2u : Word(v=="food");
        else if (f == "--steps") o.steps = parse_word(v, 0, 0xffffffffu, f);
        else if (f=="--colonize") o.colonize=parse_word(v,0,~0u,f);
        else if (f=="--husk") o.husk=parse_word(v,0,3,f);
        else if (f=="--mouth-origin" && (v=="inherited" || v=="whole" || v=="random")) o.mouths=v=="random" ? 2u : Word(v=="whole");
        else if (f=="--passages") o.passages=parse_word(v,1,1000000,f);
        else if (f=="--settlers") o.settlers=parse_word(v,1,1048576,f);
        else if (f=="--vegetation" && (v=="novel" || v=="repeat")) o.novel_plants=v=="novel";
        else if (f=="--resident-index" && (v=="auto" || v=="grid" || v=="warp")) {
            o.grid_index=v=="auto" ? std::optional<bool>{} : std::optional<bool>{v=="grid"};
        }
        else if (f == "--report-every") o.interval = parse_word(v, 1, 0xffffffffu, f);
        else if (f == "--assay") o.probes = parse_word(v, 1, 256, f);
        else if (f == "--survey") o.survey = parse_word(v,1,65536,f);
        else if (f == "--survey-seed") o.survey_seed = parse_word(v,0,~0u,f);
        else if (f == "--conditioning") o.panel.quartets = parse_word(v, 2, 8192, f);
        else if (f=="--pupils") o.pupils=parse_word(v,1,65536,f);
        else if (f=="--growth") o.growth=parse_word(v,1,8192,f);
        else if (f=="--forage-cues" && (v=="single" || v=="paired")) o.paired_food=v=="paired";
        else if (f=="--appetites") o.appetites=parse_word(v,1,65536,f);
        else if (f=="--valuation") o.value.sets=parse_word(v,1,8192,f);
        else if (f=="--outcome-task" && (v=="direct" || v=="inferred")) o.value.inferred=v=="inferred";
        else if (f=="--value-controls") o.value_controls=toggle(v);
        else if (f=="--pairings") o.value.pairings=parse_word(v,1,32,f);
        else if (f=="--revisit") o.revisit.sets=parse_word(v,1,8192,f);
        else if (f=="--regimes") o.revisit.regimes=parse_word(v,2,4,f);
        else if (f=="--memory-task") {
            auto it=std::find(memory_tasks.begin(),memory_tasks.end(),v);
            if (it==memory_tasks.end()) throw std::invalid_argument("unknown memory challenge");
            o.revisit.task=RevisitTask(it-memory_tasks.begin());
        }
        else if (f=="--landmarks" && (v=="visible" || v=="hidden")) o.revisit.marked=v=="visible";
        else if (f=="--memory-controls") o.memory_controls=toggle(v);
        else if (f == "--problem") {
            auto it = std::find(law_names.begin(),law_names.end(),v);
            if (it == law_names.end() && v != "grips" && v != "dimensions") throw std::invalid_argument("unknown conditioning problem");
            o.panel.dimensions = v != "grips"; o.panel.composition = it != law_names.end();
            o.panel.law = o.panel.composition ? JointLaw(it-law_names.begin()) : JointLaw::ring;
        }
        else if (f == "--varieties") o.value.items=o.revisit.items=o.panel.varieties = parse_word(v, 2, 32, f);
        else if (f == "--exposures") o.passage.rounds=o.travel.rounds=o.value.rounds=o.revisit.practice=o.panel.exposures = parse_word(v, 1, 64, f);
        else if (f=="--practice-schedule" && (v=="fixed" || v=="short-long")) o.short_practice=v=="short-long";
        else if (f == "--nursery") o.nursery = parse_word(v, 1, 4096, f);
        else if (f == "--nursery-varieties") o.nursery_varieties = parse_word(v,4,16,f);
        else if (f == "--nursery-problem") {
            auto it = std::find(law_names.begin(),law_names.end(),v);
            o.course=v=="varied" ? NurseryCourse::varied : v=="revisit" ? NurseryCourse::recall : v=="lifelong" ? NurseryCourse::lifelong :
                     v=="junctions" ? NurseryCourse::junctions : v=="migration" ? NurseryCourse::migration : v=="navigation" ? NurseryCourse::navigation : v=="subsistence" ? NurseryCourse::subsistence : v=="outcomes" ? NurseryCourse::outcomes : v=="repertoire" ? NurseryCourse::repertoire : v=="integrated" ? NurseryCourse::integrated : NurseryCourse::paired;
            if (it == law_names.end() && v != "dimensions" && o.course==NurseryCourse::paired) throw std::invalid_argument("unknown nursery problem");
            o.joint_nursery = o.course!=NurseryCourse::paired || it != law_names.end();
            o.nursery_law = o.course!=NurseryCourse::paired ? JointLaw::mosaic : it != law_names.end() ? JointLaw(it-law_names.begin()) : JointLaw::ring;
        }
        else if (f == "--breeding" && (v == "select" || v == "drift" || v=="cross" || v=="extend" || v=="splice" || v=="organ" || v=="map" || v=="history" || v=="history-local"))
            o.breeding=v=="history-local" ? 8 : v=="history" ? 7 : v=="map" ? 6 : v=="organ" ? 5 : v=="splice" ? 4 : v=="extend" ? 3 : v=="cross" ? 2 : Word(v=="drift");
        else if (f == "--relations") o.relations = v == "contexts" ? 3u : v == "waves" ? 2u : Word(toggle(v));
        else if (f=="--metabolism" && (v=="energy" || v=="balanced" || v=="food-fuel")) o.balanced=v=="food-fuel" ? 2 : Word(v=="balanced");
        else if (f=="--inheritance") {
            auto it=std::find(inheritance_names.begin(),inheritance_names.end(),v);
            if (it==inheritance_names.end()) throw std::invalid_argument("unknown inheritance mode");
            o.repair=Word(it-inheritance_names.begin());
        }
        else if (f=="--modifier-origin" && (v=="inherited" || v=="zero" || v=="random")) o.modifier_origin=v=="random" ? 2u : Word(v=="zero");
        else if (f=="--leach") o.leach=parse_word(v,0,8,f);
        else if (f=="--seep") o.seep=parse_word(v,0,8,f);
        else if (f=="--stand-size") o.stand=parse_word(v,1,1024,f);
        else if (f=="--polarity" && (v=="shared" || v=="evolvable")) o.polarity=v=="evolvable";
        else if (f=="--programs") o.programs=parse_word(v,1,2,f);
        else if (f=="--climates") o.climates=parse_word(v,1,4,f);
        else if (f=="--climate-cue") o.climate_cue=toggle(v);
        else if (f == "--flora") {
            auto it = std::find(flora_names.begin(),flora_names.end(),v);
            if (it == flora_names.end()) throw std::invalid_argument("unknown plant treatment");
            o.flora = Flora(it-flora_names.begin());
        }
        else if (f == "--fruit") {
            auto it = std::find(fruit_names.begin(),fruit_names.end(),v);
            if (it == fruit_names.end()) throw std::invalid_argument("unknown fruit development");
            o.fruit = Fruit(it-fruit_names.begin());
        }
        else if (f == "--synapse-bits") o.synapse_bits = parse_word(v, 1, 2, f);
        else if (f == "--season") o.season = parse_word(v, 8, 65536, f);
        else if (f == "--species") o.species = parse_word(v, 4, 32, f);
        else if (f == "--digestion") o.digestion = parse_word(v, 1, 8, f);
        else if (f == "--probe-delay") o.revisit.delay=o.panel.delay = parse_word(v, 1, 8, f);
        else if (f == "--probe-seed") o.passage.seed=o.travel.seed=o.value.seed=o.revisit.seed=o.panel.seed = parse_word(v, 0, 0xffffffffu, f);
        else if (f == "--side") o.side = parse_word(v, 8, 1024, f);
        else if (f == "--neurons") o.neurons = parse_word(v, 64, 1024, f);
        else if (f == "--seed") o.seed = parse_word(v, 0, 0xffffffffu, f);
        else if (f == "--density") o.density = parse_word(v, 0, 100, f);
        else if (f == "--plasticity") o.plastic = toggle(v);
        else if (f == "--mutation") o.mutate = toggle(v);
        else if (f == "--behavior" && (v == "random" || v == "neural")) o.random = v == "random";
        else if (f == "--contest") {
            auto it = std::find(contest_names.begin() + 1, contest_names.end(), v);
            if (it == contest_names.end()) throw std::invalid_argument("unknown ecological contest");
            o.contest = Word(it - contest_names.begin());
        }
        else if (f == "--schedule" && (v == "auto" || v == "dense" || v == "occupied")) {
            if (v == "auto") o.packed.reset(); else o.packed = v == "occupied";
        }
        else if (f == "--dispatch" && (v == "graph" || v == "direct")) o.graphs = v == "graph";
        else if (f == "--habitat") {
            auto it = std::find(habitat_names.begin(), habitat_names.end(), v);
            if (it == habitat_names.end()) throw std::invalid_argument("unknown habitat");
            o.habitat = Habitat(it - habitat_names.begin());
        }
        else if (f == "--load") o.load = v;
        else if (f == "--save") o.save = v;
        else if (f == "--founders") o.ancestors = v;
        else if (f=="--context-founders") o.context_ancestors.emplace_back(v);
        else if (f=="--plant-founders") o.plant_founders=v;
        else if (f=="--food-genes") o.food_genes=v;
        else if (f=="--graft") {
            constexpr std::array names{"whole","memory","alternative","body","soma","dendrites","pathways","outcomes","fidelity","navigation","history"};
            auto it=std::find(names.begin(),names.end(),v);
            if (it==names.end() || it==names.begin() || v=="alternative" || ++i>=argc)
                throw std::invalid_argument("--graft needs body, soma, dendrites, pathways, memory or outcomes and a donor file");
            o.grafts.emplace_back(Graft(it-names.begin()),argv[i]);
        }
        else if (f=="--alternate-founders") o.alternate_founders=v;
        else if (f == "--founder-site") o.founder_site = parse_word(v,0,1024*1024-1,f);
        else if (f != "--backend" || v != "gpu") throw std::invalid_argument("unknown life option or value for " + std::string(f));
    }
    if (!o.load.empty() && (!o.ancestors.empty() || o.side || o.neurons || o.seed || o.density || o.contest || o.swap || o.habitat || o.digestion || o.species || o.synapse_bits || o.season || o.relations || o.climates || o.programs || o.polarity || o.climate_cue || o.balanced || o.stand || o.seep || o.leach || o.repair || o.modifier_origin || o.husk || o.navigation || o.springs || o.basin_width || o.trace_origin))
        throw std::invalid_argument("a resumed world gets its shape, initial population and seed from its checkpoint");
    if ((o.contest == Contest::ancestry || o.contest == Contest::neural_genes || o.contest == Contest::body_genes) && o.ancestors.empty())
        throw std::invalid_argument("genome competition needs a --founders checkpoint");
    if (!std::has_single_bit(o.nursery_varieties)) throw std::invalid_argument("nursery varieties must be a power of two");
    if ((o.value.sets || o.growth || o.pupils) && (o.value.items<4 || o.value.items>16)) throw std::invalid_argument("valuation uses 4, 8 or 16 objects");
    if (o.revisit.sets && o.revisit.items!=8 && o.revisit.items!=16) throw std::invalid_argument("revisit uses 8 or 16 property values");
    if (!std::has_single_bit(o.panel.varieties)) throw std::invalid_argument("the conditioning vocabulary must be a power of two");
    if (o.panel.dimensions && (o.panel.varieties < 4 || o.panel.varieties > 16))
        throw std::invalid_argument("paired-object problems use 4, 8 or 16 sensory values per attribute");
    if (o.nursery && (!o.load.empty() || o.contest)) throw std::invalid_argument("nursery breeding precedes founding a new habitat; use --founders to import parents");
    if ((o.breeding || o.joint_nursery || o.nursery_varieties != 4) && !o.nursery) throw std::invalid_argument("a nursery treatment requires founding selection");
    if (o.swap && !o.contest) throw std::invalid_argument("--swap-cohorts needs --contest");
    if (!o.colonize && (o.passages || o.settlers || o.novel_plants)) throw std::invalid_argument("passage settings require --colonize SEED");
    if (o.travel.change_food && !o.travel.motivation) throw std::invalid_argument("a food diversion needs a food destination");
    if ((o.breeding>=7 || o.trace_origin) && o.navigation.value_or(0)<4) throw std::invalid_argument("history breeding requires episodic navigation");
    if ((o.course==NurseryCourse::navigation || o.course==NurseryCourse::migration || o.breeding==6) && !o.navigation.value_or(0)) throw std::invalid_argument("navigation breeding needs --navigation on");
    if (o.short_practice && (!o.nursery || o.course!=NurseryCourse::outcomes)) throw std::invalid_argument("short-long practice applies to an outcome nursery");
    if (o.mouths && !o.load.empty()) throw std::invalid_argument("mouth alleles are initialized at founding");
    if (o.founder_site && o.ancestors.empty()) throw std::invalid_argument("--founder-site requires --founders");
    if ((!o.context_ancestors.empty() || !o.alternate_founders.empty() || !o.grafts.empty() || !o.plant_founders.empty() || !o.food_genes.empty()) && !o.load.empty()) throw std::invalid_argument("germline transplantation starts a new habitat");
    if (!o.alternate_founders.empty() && o.programs!=2) throw std::invalid_argument("alternate transplantation needs --programs 2");
    return o;
}
void validate(Settings c) {
    if (c.husk>3 || (c.husk && c.balanced!=2)) throw std::invalid_argument("fruit handling time requires food-fuel metabolism");
    if (c.basin_width && (c.basin_width<8 || c.basin_width>c.side || !std::has_single_bit(c.basin_width) || !c.navigation)) throw std::invalid_argument("basins require navigation and a power-of-two width in the habitat");
    if (c.contest>=Contest::temporal_signal && c.navigation<4) throw std::invalid_argument("temporal contests require episodic navigation");
    if (c.springs>32 || (c.springs && c.balanced!=2)) throw std::invalid_argument("aquifers require food-fuel metabolism");
    if (c.navigation>7 || (c.navigation && !c.balanced) || (c.contest>=Contest::navigation_signal && !c.navigation)) throw std::invalid_argument("navigation requires a thicket with food metabolism and a navigation circuit for a navigation contest");
    if (c.repair>=inheritance_names.size()) throw std::invalid_argument("invalid inheritance mode");
    if (c.leach>8 || (c.leach && c.balanced!=2)) throw std::invalid_argument("mineral loss requires food-fuel metabolism");
    if (c.seep>8 || (c.seep && !c.balanced)) throw std::invalid_argument("material seepage requires balanced metabolism");
    if (c.balanced>2 || !std::has_single_bit(c.stand) || c.stand>c.side || ((c.balanced || c.stand!=4) && c.habitat!=Habitat::thicket))
        throw std::invalid_argument("food materials and clonal stand sizes require a thicket, with a stand no wider than the world");
    if (c.polarity>1 || (c.polarity && c.programs!=2)) throw std::invalid_argument("evolvable synaptic polarities require two expressed programs");
    if ((c.programs!=1 && c.programs!=2) || (c.programs==2 && c.relations!=3))
        throw std::invalid_argument("two programs require context assemblies");
    if ((c.climates && (!std::has_single_bit(c.climates) || c.climates>4 || c.habitat!=Habitat::thicket)) ||
        c.climate_cue>1 || (c.climate_cue && !c.climates))
        throw std::invalid_argument("recurrent climates use one, two or four inherited thicket chemistries");
    if (c.flora > Flora::autonomous || c.fruit > Fruit::inherited ||
        (c.habitat != Habitat::thicket && (c.flora != Flora::evolving || c.fruit != Fruit::scattered)) ||
        (c.habitat == Habitat::thicket && c.season && !c.climates))
        throw std::invalid_argument("plant settings require a thicket with locally inherited chemistry");
    if ((c.synapse_bits != 1 && c.synapse_bits != 2) || (c.season && (c.season < 8 || c.season > 65536))
        || (c.habitat == Habitat::meadow && (c.synapse_bits != 1 || c.season)))
        throw std::invalid_argument("invalid grove synaptic capacity or seasonal period");
    if (!std::has_single_bit(c.species) || c.species < 4 || c.species > 32 ||
        (paired_habitat(c.habitat) && c.species > 16) || (c.habitat == Habitat::meadow && c.species != 4))
        throw std::invalid_argument("invalid number of fruit varieties");
    if (!std::has_single_bit(c.side) || !std::has_single_bit(c.tiles) || c.side < 8 || c.side > 1024
        || c.tiles < 2 || c.tiles > 32 || c.density > 100 || c.mutate > 1 || c.plastic > 1 || c.random_actions > 1 || c.contest >= contest_names.size() || c.swap > 1 || c.habitat > Habitat::thicket || c.digestion > 8 || c.relations > 3 || ((c.digestion || c.relations) && c.habitat == Habitat::meadow))
        throw std::invalid_argument("invalid ecosystem geometry or settings");
}
struct Header { char magic[8]; Word version, tick; Settings config; };
constexpr size_t meadow_header = offsetof(Header, config) + offsetof(Settings, habitat);
constexpr size_t grove_header = offsetof(Header, config) + offsetof(Settings, digestion);
static_assert(meadow_header == 52 && grove_header == 56);
constexpr size_t digestion_header = offsetof(Header, config) + offsetof(Settings, species);
size_t header_bytes(Word version) {
    if (version == 1) return meadow_header;
    if (version == 2) return grove_header;
    if (version == 3) return digestion_header;
    if (version == 4) return offsetof(Header, config) + offsetof(Settings, synapse_bits);
    if (version == 5) return offsetof(Header,config) + offsetof(Settings,relations);
    if (version == 6) return offsetof(Header,config) + offsetof(Settings,flora);
    if (version==7) return offsetof(Header,config)+offsetof(Settings,fruit);
    if (version<10) return offsetof(Header,config)+offsetof(Settings,climates);
    if (version==10) return offsetof(Header,config)+offsetof(Settings,programs);
    if (version==11) return offsetof(Header,config)+offsetof(Settings,polarity);
    if (version==12) return offsetof(Header,config)+offsetof(Settings,balanced);
    if (version==13) return offsetof(Header,config)+offsetof(Settings,seep);
    if (version<16) return offsetof(Header,config)+offsetof(Settings,leach);
    if (version==16) return offsetof(Header,config)+offsetof(Settings,repair);
    if (version==17) return offsetof(Header,config)+offsetof(Settings,ancestor_depth);
    if (version==18) return offsetof(Header,config)+offsetof(Settings,husk);
    if (version==19) return offsetof(Header,config)+offsetof(Settings,navigation);
    if (version==20) return offsetof(Header,config)+offsetof(Settings,springs);
    if (version<=22) return offsetof(Header,config)+offsetof(Settings,basin_width);
    return sizeof(Header);
}
constexpr size_t association_bytes(Word version) {
    return version < 7 ? offsetof(AssociationLayer,convergence) : sizeof(AssociationLayer);
}
struct Donors {
    std::vector<Site> sites;
    std::vector<Genome> genes;
    std::vector<SynapticLayer> layers;
    std::vector<AssociationLayer> links;
    std::vector<ContextLayer> contexts;
    std::vector<ExpressionLayer> expression;
    std::vector<OutcomeLayer> outcomes;
    std::vector<MapLayer> maps;
    std::vector<TraceLayer> traces;
};
struct Snapshot : Donors {
    Header h{};
    std::vector<Tissue> tissue;
    std::vector<Ledger> records;
    std::vector<Contacts> contacts;
    std::vector<unsigned char> pods;
    std::vector<Harvest> harvest;
    std::vector<DelayLine> delay;
    std::vector<Plant> flora;
    std::vector<SeedLedger> propagation;
    std::vector<Word> reactions;
    std::vector<ClimateLedger> seasons;
    std::vector<Word> food_types;
    std::vector<DietLedger> diet;
    std::vector<SeepLedger> seepage;
    std::vector<Count> leached;
    void allocate() {
        size_t n = h.config.side * h.config.side;
        sites.resize(n); genes.resize(n); tissue.resize(n * h.config.tiles); records.resize(n / 4);
        if (h.config.digestion) delay.resize(n);
        if (h.config.synapse_bits == 2) layers.resize(n);
        if (h.config.relations) links.resize(n);
        if (h.config.relations==3) contexts.resize(n);
        if (h.config.programs==2) expression.resize(n);
        if (h.config.leach) leached.resize(climate_shards(Word(n)));
        if (h.config.seep || h.config.springs) seepage.resize(climate_shards(Word(n)));
        if (h.config.navigation) maps.resize(n);
        if (h.config.navigation>=4) traces.resize(n);
        if (h.config.balanced) { outcomes.resize(n); food_types.resize(n); diet.resize(climate_shards(Word(n))); }
        if (h.config.habitat == Habitat::thicket) { flora.resize(n); propagation.resize(n/4); }
        if (h.config.climates) {
            reactions.resize((h.config.climates-1)*n); seasons.resize(climate_shards(Word(n)));
        }
        if (h.config.habitat != Habitat::meadow) { contacts.resize(n); pods.resize(n); harvest.resize(n/4); }
    }
    template<class Visitor> void chunks(Visitor &&visit) {
        visit(&h, header_bytes(h.version));
        visit(sites.data(), sites.size() * sizeof(Site));
        visit(genes.data(), genes.size() * sizeof(Genome));
        visit(tissue.data(), tissue.size() * sizeof(Tissue));
        visit(records.data(), records.size() * sizeof(Ledger));
        if (h.config.habitat != Habitat::meadow) {
            visit(contacts.data(), contacts.size() * sizeof(Contacts));
            visit(pods.data(), pods.size());
            visit(harvest.data(), harvest.size() * sizeof(Harvest));
            if (h.config.digestion) visit(delay.data(), delay.size() * sizeof(DelayLine));
            if (!flora.empty()) { visit(flora.data(),flora.size()*sizeof(Plant)); visit(propagation.data(),propagation.size()*sizeof(SeedLedger)); }
            if (!seasons.empty()) {
                if (!reactions.empty()) visit(reactions.data(),reactions.size()*sizeof(Word));
                visit(seasons.data(),seasons.size()*sizeof(ClimateLedger));
            }
            if (h.config.synapse_bits == 2) visit(layers.data(), layers.size() * sizeof(SynapticLayer));
            if (h.config.relations) {
                if (h.version == 6) for (auto &link : links) visit(&link,association_bytes(6));
                else visit(links.data(),links.size()*sizeof(AssociationLayer));
                if (!contexts.empty()) visit(contexts.data(),contexts.size()*sizeof(ContextLayer));
                if (!expression.empty()) visit(expression.data(),expression.size()*sizeof(ExpressionLayer));
            }
            if (!outcomes.empty()) {
                visit(food_types.data(),food_types.size()*sizeof(Word)); visit(diet.data(),diet.size()*sizeof(DietLedger));
                if (!seepage.empty()) visit(seepage.data(),seepage.size()*sizeof(SeepLedger));
                if (!leached.empty()) visit(leached.data(),leached.size()*sizeof(Count));
                visit(outcomes.data(),outcomes.size()*sizeof(OutcomeLayer));
                if (!maps.empty()) visit(maps.data(),maps.size()*sizeof(MapLayer));
                if (!traces.empty()) visit(traces.data(),traces.size()*sizeof(TraceLayer));
            }
        }
    }
};
size_t read_header(std::ifstream &in, Header &h) {
    auto length = in.tellg(); in.seekg(0);
    if (!in.read(reinterpret_cast<char *>(&h), meadow_header) || std::memcmp(h.magic, "AXLIFE1", 8) || (h.version < 1 || h.version > 23))
        throw std::runtime_error("invalid ecosystem checkpoint header");
    if (h.version >= 2 && (!in.read(reinterpret_cast<char *>(&h.config.habitat), sizeof(Habitat)) || (h.config.habitat==Habitat::meadow && h.version<17)))
        throw std::runtime_error("invalid grove checkpoint header");
    if (h.version >= 3 && (!in.read(reinterpret_cast<char *>(&h.config.digestion), sizeof(Word)) || (h.version == 3 && !h.config.digestion)))
        throw std::runtime_error("invalid digestion checkpoint header");
    if (h.version >= 4 && !in.read(reinterpret_cast<char *>(&h.config.species), sizeof(Word)))
        throw std::runtime_error("invalid species checkpoint header");
    if (h.version >= 5 && (!in.read(reinterpret_cast<char *>(&h.config.synapse_bits), sizeof(Word))
                          || !in.read(reinterpret_cast<char *>(&h.config.season), sizeof(Word))))
        throw std::runtime_error("invalid synaptic checkpoint header");
    if (h.version >= 6 && !in.read(reinterpret_cast<char *>(&h.config.relations),sizeof(Word)))
        throw std::runtime_error("incomplete associative memory header");
    if ((h.version >= 7 && !in.read(reinterpret_cast<char *>(&h.config.flora),sizeof(Flora))) ||
        (h.config.habitat == Habitat::thicket && h.version < 7))
        throw std::runtime_error("invalid plant population header");
    if (h.version >= 8 && !in.read(reinterpret_cast<char *>(&h.config.fruit),sizeof(Fruit)))
        throw std::runtime_error("incomplete fruit development header");
    if (h.version>=10 && (!in.read(reinterpret_cast<char *>(&h.config.climates),sizeof(Word)) ||
        !in.read(reinterpret_cast<char *>(&h.config.climate_cue),sizeof(Word)))) throw std::runtime_error("incomplete climate header");
    if (h.config.relations==3 && h.version<9) throw std::runtime_error("invalid contextual memory version");
    if (h.version>=11 && !in.read(reinterpret_cast<char *>(&h.config.programs),sizeof(Word))) throw std::runtime_error("missing expression capacity");
    if (h.version>=12 && !in.read(reinterpret_cast<char *>(&h.config.polarity),sizeof(Word))) throw std::runtime_error("missing synaptic polarity");
    if (h.version>=13 && (!in.read(reinterpret_cast<char *>(&h.config.balanced),sizeof(Word)) ||
        !in.read(reinterpret_cast<char *>(&h.config.stand),sizeof(Word)))) throw std::runtime_error("missing thicket metabolism");
    if (h.version>=14 && !in.read(reinterpret_cast<char *>(&h.config.seep),sizeof(Word))) throw std::runtime_error("missing material flow header");
    if (h.version>=16 && !in.read(reinterpret_cast<char *>(&h.config.leach),sizeof(Word))) throw std::runtime_error("missing mineral loss header");
    if (h.version>=17 && !in.read(reinterpret_cast<char *>(&h.config.repair),sizeof(Word))) throw std::runtime_error("missing copying modifiers");
    if (h.version>=18 && !in.read(reinterpret_cast<char *>(&h.config.ancestor_depth),sizeof(Word))) throw std::runtime_error("missing founding lineage depth");
    if (h.version>=19 && !in.read(reinterpret_cast<char *>(&h.config.husk),sizeof(Word))) throw std::runtime_error("missing fruit handling time");
    if (h.version>=20 && !in.read(reinterpret_cast<char *>(&h.config.navigation),sizeof(Word))) throw std::runtime_error("missing navigation mode");
    if (h.version>=21 && !in.read(reinterpret_cast<char *>(&h.config.springs),sizeof(Word))) throw std::runtime_error("missing aquifer flow");
    if (h.version>=23 && !in.read(reinterpret_cast<char *>(&h.config.basin_width),sizeof(Word))) throw std::runtime_error("missing basin width");
    if ((h.version==22 && h.config.navigation<4) || (h.version<22 && h.config.navigation>=4) || (h.version==23 && !h.config.basin_width)) throw std::runtime_error("invalid temporal circuit version");
    validate(h.config);
    size_t n = size_t(h.config.side) * h.config.side;
    size_t expected = header_bytes(h.version) + sizeof(Count) + n * (sizeof(Site) + sizeof(Genome) + h.config.tiles * sizeof(Tissue)) + n / 4 * sizeof(Ledger);
    if (h.config.habitat != Habitat::meadow) expected += n * (sizeof(Contacts) + 1) + n/4 * sizeof(Harvest);
    if (h.config.digestion) expected += n * sizeof(DelayLine);
    if (h.config.synapse_bits == 2) expected += n * sizeof(SynapticLayer);
    if (h.config.relations) expected += n*association_bytes(h.version);
    if (h.config.relations==3) expected += n*sizeof(ContextLayer);
    if (h.config.programs==2) expected+=n*sizeof(ExpressionLayer);
    if (h.config.habitat == Habitat::thicket) expected += n*sizeof(Plant) + n/4*sizeof(SeedLedger);
    if (h.config.climates) expected+=n*(h.config.climates-1)*sizeof(Word)+climate_shards(Word(n))*sizeof(ClimateLedger);
    expected+=n*map_bytes(h.config.navigation);
    if (h.config.balanced) expected+=n*(sizeof(Word)+sizeof(OutcomeLayer))+climate_shards(Word(n))*sizeof(DietLedger);
    if (h.config.leach) expected+=climate_shards(Word(n))*sizeof(Count);
    if (h.config.seep || h.config.springs) expected+=climate_shards(Word(n))*sizeof(SeepLedger);
    if (length != std::streamoff(expected)) throw std::runtime_error("ecosystem checkpoint length mismatch");
    return expected;
}
void verify_world_file(std::ifstream &in,size_t bytes,const std::string &path) {
    std::array<char,65536> buffer;
    Count hash=hash_start,stored{};
    in.seekg(0);
    for (size_t left=bytes-sizeof(Count);left;) {
        size_t n=std::min(left,buffer.size());
        if (!in.read(buffer.data(),n)) throw std::runtime_error("incomplete habitat");
        hash=digest(hash,buffer.data(),n); left-=n;
    }
    if (!in.read(reinterpret_cast<char *>(&stored),sizeof(stored)) || stored!=hash)
        throw std::runtime_error("ecosystem checkpoint checksum mismatch");
    journal::checkpoint(path,hash,false);
}
struct ProducerStock { Settings config; std::vector<Plant> plants; std::vector<Word> reactions,food; };
ProducerStock read_producers(const std::string &path,bool chemistry_only) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if (!in) throw std::runtime_error("cannot open producer checkpoint "+path);
    Header h{}; size_t bytes=read_header(in,h),n=size_t(h.config.side)*h.config.side;
    if (h.config.habitat!=Habitat::thicket || (chemistry_only && !h.config.balanced)) throw std::invalid_argument("producer germline absent from donor");
    verify_world_file(in,bytes,path);
    ProducerStock stock{h.config,{},{},{}};
    auto read=[&](auto &array,size_t count,size_t offset) {
        array.resize(count); in.seekg(std::streamoff(offset));
        if (!in.read(reinterpret_cast<char *>(array.data()),count*sizeof(array[0]))) throw std::runtime_error("missing producer germline");
    };
    if (!chemistry_only) {
        size_t offset=header_bytes(h.version)+n*(sizeof(Site)+sizeof(Genome)+h.config.tiles*sizeof(Tissue)+sizeof(Contacts)+1)
                     +(n/4)*(sizeof(Ledger)+sizeof(Harvest))+(h.config.digestion ? n*sizeof(DelayLine) : 0);
        read(stock.plants,n,offset);
        if (h.config.climates>1) read(stock.reactions,(h.config.climates-1)*n,offset+n*sizeof(Plant)+(n/4)*sizeof(SeedLedger));
    }
    if (h.config.balanced) {
        size_t tail=n*(sizeof(OutcomeLayer)+map_bytes(h.config.navigation))+climate_shards(Word(n))*(sizeof(DietLedger)+((h.config.seep || h.config.springs) ? sizeof(SeepLedger):0)+(h.config.leach ? sizeof(Count):0));
        read(stock.food,n,bytes-sizeof(Count)-tail-n*sizeof(Word));
    }
    return stock;
}
bool valid_site(const Site &q, const Header &h, size_t sites) {
    const Body &b = q.body;
    return b.slot < sites && b.founder < sites && b.born <= h.tick && b.generation <= Count(h.tick)+h.config.ancestor_depth &&
           b.energy <= 1023 && b.heading <= 3 && b.recent <= 2 && q.food <= 80 && q.stock <= 1023;
}
Snapshot read_snapshot(const std::string &path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) throw std::runtime_error("cannot open ecosystem checkpoint " + path);
    Snapshot s;
    read_header(in,s.h);
    size_t n = size_t(s.h.config.side)*s.h.config.side;
    s.allocate();
    in.seekg(0);
    Count hash = hash_start, stored;
    s.chunks([&](void *data, size_t bytes) {
        if (!in.read(static_cast<char *>(data), bytes)) throw std::runtime_error("incomplete ecosystem checkpoint");
        hash = digest(hash, data, bytes);
    });
    if (!in.read(reinterpret_cast<char *>(&stored), sizeof(stored)) || stored != hash)
        throw std::runtime_error("ecosystem checkpoint checksum mismatch");
    journal::checkpoint(path,hash,false);
    std::vector<bool> seen(n);
    for (const auto &q : s.sites) {
        const auto &b = q.body;
        if (!valid_site(q,s.h,n) || seen[b.slot])
            throw std::runtime_error("invalid organism or habitat in checkpoint");
        seen[b.slot] = true;
    }
    for (size_t i = 0; i < s.pods.size(); ++i)
        if (s.pods[i] >= (paired_habitat(s.h.config.habitat) ? s.h.config.species*s.h.config.species : s.h.config.species) || s.contacts[i].previous > (paired_habitat(s.h.config.habitat) ? 32767u : 511u)) throw std::runtime_error("invalid grove sensory state");
    for (const auto &line : s.delay) {
        for (auto packet : line.digestion)
            if (packet > 22 && packet != 255) throw std::runtime_error("invalid digestive reservoir");
    }
    return s;
}
struct World : Session {
    GraphApi graphs{};
    GraphExec program[4]{};
    Stream capture{};
    static constexpr Word graph_ticks = 32;
    bool use_graph, packed, adaptive, forage_profile=false, grid_index=false;
    Event ha{}, hb{};
    Function init{}, think{}, neural_kernels[2]{}, occupancy{}, settle{}, census{}, transplant{}, advance_clock{}, paired_assay{}, forecast{};
    DevicePtr grid[2]{}, genes{}, brains{}, intents{}, records{}, totals{}, families{}, epoch{}, contacts{}, pods{}, harvest{}, delay{}, layers{}, weather{}, links{}, flora{}, propagation{}, outcomes{};
    DevicePtr resident_offsets{};
    std::vector<DevicePtr> allocations;
    size_t allocated{};
    Settings c;
    Word tick{}, sites;
    int regs{}, local{}, settle_regs{}, settle_local{};
    World(Settings config,bool graph,bool compact,bool adapt) : use_graph(graph),packed(compact),adaptive(adapt),c(config),sites(c.side*c.side) {}
    ~World() {
        if (current) {
            for (auto executable : program) if (executable) graphs.destroy_exec(executable);
            if (capture) graphs.destroy_stream(capture);
            for (auto ptr : allocations) d.free_mem(ptr);
            if (ha) d.event_free(ha);
            if (hb) d.event_free(hb);
        }
    }
    DevicePtr allocate(size_t n) {
        DevicePtr p{}; d.check(d.alloc(&p, n), "allocate habitat");
        try { allocations.push_back(p); } catch (...) { d.free_mem(p); throw; }
        allocated += n; return p;
    }
    void schedule(bool compact) {
        auto &kernel=neural_kernels[compact];
        if (!kernel) {
            std::string family=c.relations ? neural_prefix(c.relations) : c.habitat!=Habitat::meadow ? "grove_" : "life_";
            if (c.balanced) bind(kernel,"food"+std::to_string(c.relations)+(compact ? "_packed" : "_think")+std::to_string(c.tiles),c.navigation>=4 ? Image::sequence_field : c.navigation ? Image::trails : Image::outcomes);
            else bind(kernel,family+(compact ? "packed" : "think")+std::to_string(c.tiles));
        }
        packed=compact; think=kernel;
        d.check(d.func_attr(&regs,4,think),"brain register usage"); d.check(d.func_attr(&local,3,think),"brain local memory");
    }
    void consider_density(Count alive,Word size) {
        bool compact=5*alive<2ull*size;
        if (adaptive && compact!=packed) schedule(compact);
    }
    void setup() {
        allocations.reserve(16);
        open();
        bind(init,"life_init"); schedule(packed);
        if (adaptive) bind(occupancy,"life_occupancy");
        bind(settle, c.balanced ? "food_settle" : c.habitat == Habitat::thicket ? "thicket_settle" : c.habitat != Habitat::meadow ? "grove_settle" : "life_settle");
        bind(census, "life_census"); bind(transplant, "life_transplant"); bind(advance_clock, "life_clock");
        if (use_graph) {
            graphs.bind(d); d.check(graphs.create_stream(&capture, 1), "open graph capture stream");
            epoch = allocate(sizeof(Word));
            if (c.habitat != Habitat::meadow && (c.habitat != Habitat::thicket || c.climates)) {
                Word across = std::max(1u, c.side/16);
                weather = allocate(graph_ticks * across * across * sizeof(Chemistry));
                bind(forecast,"life_weather");
            }
        }
        d.check(d.func_attr(&settle_regs, 4, settle), "habitat register usage"); d.check(d.func_attr(&settle_local, 3, settle), "habitat local memory");
        for (auto &g : grid) g = allocate(size_t(sites) * sizeof(Site));
        genes = allocate(size_t(sites) * sizeof(Genome));
        brains = allocate(size_t(sites) * c.tiles * sizeof(Tissue));
        intents = allocate(size_t(sites) * 4 * sizeof(Word));
        records = allocate(size_t(sites / 4) * sizeof(Ledger));
        totals = allocate(sizeof(Census)); families = allocate(sites / 8);
        if (c.habitat != Habitat::meadow) {
            contacts = allocate(size_t(sites) * sizeof(Contacts));
            pods = allocate(sites); harvest = allocate(size_t(sites/4)*sizeof(Harvest)+(c.climates ? climate_shards(sites)*sizeof(ClimateLedger) : 0)+(c.balanced ? climate_shards(sites)*(sizeof(DietLedger)+((c.seep || c.springs) ? sizeof(SeepLedger) : 0)+(c.leach ? sizeof(Count) : 0)) : 0));
            if (c.balanced) outcomes=allocate(size_t(sites)*(sizeof(OutcomeLayer)+map_bytes(c.navigation)));
            if (c.relations) links = allocate(size_t(sites)*association_stride(c.relations,c.programs));
            if (c.synapse_bits == 2) layers = allocate(size_t(sites)*sizeof(SynapticLayer));
            if (c.digestion) delay = allocate(size_t(sites)*sizeof(DelayLine));
            if (c.habitat == Habitat::thicket) {
                flora = allocate(size_t(sites)*(sizeof(Plant)+((c.climates ? c.climates-1 : 0)+Word(c.balanced!=0))*sizeof(Word))); propagation = allocate(size_t(sites/4)*sizeof(SeedLedger));
            }
        }
        d.check(d.event_new(&ha, 0), "allocate ecosystem timer"); d.check(d.event_new(&hb, 0), "allocate ecosystem timer");
    }
    void launch(Function f, Word threads, void **args, Stream stream = nullptr) {
        d.check(d.launch(f, (threads + 127) / 128, 1, 1, 128, 1, 1, 0, stream, args, nullptr), "launch ecosystem kernel");
    }
    void start(Word plant_seed) {
        void *args[]{&grid[0], &genes, &c, &pods, &layers, &links, &flora, &outcomes,&plant_seed}; launch(init, sites, args);
        if (propagation) d.check(d.zero(propagation,0,size_t(sites/4)*sizeof(SeedLedger)), "initialize seed ledger");
        if (delay) d.check(d.zero(delay, 0, size_t(sites)*sizeof(DelayLine)), "initialize digestive reservoir");
        if (contacts) {
            d.check(d.zero(contacts, 0, size_t(sites)*sizeof(Contacts)), "initialize dendrites");
            d.check(d.zero(harvest,0,size_t(sites/4)*sizeof(Harvest)+(c.climates ? climate_shards(sites)*sizeof(ClimateLedger) : 0)+(c.balanced ? climate_shards(sites)*(sizeof(DietLedger)+((c.seep || c.springs) ? sizeof(SeepLedger) : 0)+(c.leach ? sizeof(Count) : 0)) : 0)), "initialize harvest ledger");
        }
        d.check(d.zero(brains, 0, size_t(sites) * c.tiles * sizeof(Tissue)), "initialize neural tissue");
        d.check(d.zero(records, 0, size_t(sites / 4) * sizeof(Ledger)), "initialize ecological ledger");
    }
    void vary_histories(Word origin) {
        Function initialize{};bind(initialize,"temporal_founders",Image::assays);
        void *args[]{&outcomes,&sites,&c.seed,&origin};launch(initialize,sites,args);
    }
    void modifier_founders(Word origin,Word mouths) {
        Function adjust{}; bind(adjust,"modifier_founders");
        void *args[]{&genes,&sites,&c.seed,&origin,&mouths}; launch(adjust,sites,args);
    }
    double colonize(Word survivors,Word count,Word key,Word plant_seed) {
        Temporary positions(d,size_t(survivors)*sizeof(Word)), pool(d,size_t(count)*sizeof(Propagule));
        d.check(d.record(ha,nullptr),"start colonization timer");
        index_living(positions.ptr,true);
        Function collect{},hatch{}; bind(collect,"dispersal_collect"); bind(hatch,"dispersal_hatch");
        void *source[]{&grid[tick&1],&positions.ptr,&genes,&layers,&links,&outcomes,&c,&survivors,&count,&key,&pool.ptr};
        launch(collect,count,source);
        tick=0; start(plant_seed);
        void *young[]{&grid[0],&genes,&layers,&links,&outcomes,&c,&pool.ptr,&count,&key};
        launch(hatch,sites,young);
        d.check(d.record(hb,nullptr),"record colonization"); d.check(d.wait(hb),"complete colonization");
        float ms{}; d.check(d.elapsed(&ms,ha,hb),"measure colonization"); return ms;
    }
    void reset_contacts() {
        Function renew{};
        bind(renew,"life_renew"+std::to_string(c.relations));
        void *args[]{&grid[tick&1],&genes,&contacts,&layers,&links,&c,&tick,&outcomes};
        launch(renew,sites*8,args);
    }
    template<class V> void copy(V &v, DevicePtr ptr, bool upload) {
        size_t bytes = v.size() * sizeof(v[0]);
        if (upload) d.check(d.upload(ptr, v.data(), bytes), "upload ecosystem state");
        else d.check(d.download(v.data(), ptr, bytes), "read ecosystem state");
    }
    void state(Snapshot &s, bool upload) {
        if (upload) tick=s.h.tick;
        else {
            Word version = 1;
            if (c.habitat != Habitat::meadow) version = 2;
            if (c.digestion) version = 3;
            if (c.species != 4) version = 4;
            if (c.synapse_bits == 2 || c.season) version = 5;
            if (c.habitat == Habitat::coupled) version = 6;
            if (links || flora) version = 7;
            if (c.fruit != Fruit::scattered || c.relations == 2) version = 8;
            if (c.relations==3) version=9;
            if (c.climates) version=10;
            if (c.programs==2) version=c.polarity ? 12 : 11;
            if (c.balanced || c.stand!=4 || c.flora==Flora::autonomous) version=13;
            if (c.seep) version=14;
            if (c.balanced==2) version=15;
            if (c.leach) version=16;
            if (c.repair) version=17;
            if (c.ancestor_depth) version=18;
            if (c.husk) version=19;
            if (c.navigation) version=20;
            if (c.springs) version=21;
            if (c.navigation>=4) version=22;
            if (c.basin_width) version=23;
            s.h = {"AXLIFE1", version, tick, c};
            s.allocate();
        }
        copy(s.sites, grid[tick & 1], upload); copy(s.genes, genes, upload);
        copy(s.tissue, brains, upload); copy(s.records, records, upload);
        if (delay) copy(s.delay, delay, upload);
        if (layers) copy(s.layers, layers, upload);
        if (links) copy(s.links, links, upload);
        if (c.relations==3) copy(s.contexts,links+size_t(sites)*sizeof(AssociationLayer),upload);
        if (c.programs==2) copy(s.expression,links+size_t(sites)*(sizeof(AssociationLayer)+sizeof(ContextLayer)),upload);
        if (flora) { copy(s.flora,flora,upload); copy(s.propagation,propagation,upload); }
        if (c.climates) {
            if (!s.reactions.empty()) copy(s.reactions,flora+size_t(sites)*sizeof(Plant),upload);
            copy(s.seasons,harvest+size_t(sites/4)*sizeof(Harvest),upload);
        }
        if (c.balanced) {
            copy(s.outcomes,outcomes,upload);
            if (c.navigation) copy(s.maps,outcomes+size_t(sites)*sizeof(OutcomeLayer),upload);
            if (c.navigation>=4) copy(s.traces,outcomes+size_t(sites)*(sizeof(OutcomeLayer)+sizeof(MapLayer)),upload);
            copy(s.food_types,flora+size_t(sites)*(sizeof(Plant)+(c.climates ? c.climates-1 : 0)*sizeof(Word)),upload);
            size_t offset=size_t(sites/4)*sizeof(Harvest)+(c.climates ? climate_shards(sites)*sizeof(ClimateLedger) : 0);
            copy(s.diet,harvest+offset,upload);
            offset+=climate_shards(sites)*sizeof(DietLedger);
            if (c.seep || c.springs) { copy(s.seepage,harvest+offset,upload); offset+=climate_shards(sites)*sizeof(SeepLedger); }
            if (c.leach) copy(s.leached,harvest+offset,upload);
        }
        if (contacts) { copy(s.contacts, contacts, upload); copy(s.pods, pods, upload); copy(s.harvest, harvest, upload); }
    }
    void seed_from(Donors &stock,Graft part=Graft::whole,Word pool=0,Word pools=1) {
        if (part==Graft::temporal && (c.navigation<4 || stock.traces.empty())) throw std::invalid_argument("a history graft requires two temporal circuits");
        if (part==Graft::navigation && (!c.navigation || stock.maps.empty())) throw std::invalid_argument("navigation graft requires two route circuits");
        bool memory=part==Graft::memory || part==Graft::alternative;
        if (memory && (c.relations!=3 || stock.contexts.empty()))
            throw std::invalid_argument("context transplantation needs contextual donor and recipient populations");
        if (part==Graft::outcomes && (!outcomes || stock.outcomes.empty())) throw std::invalid_argument("outcome graft requires two populations with outcome contacts");
        if (part==Graft::pathways && (!links || stock.links.empty())) throw std::invalid_argument("pathway graft requires two associative habitats");
        Word count=Word(stock.sites.size()), mask=count-1;
        Word survivors=Word(std::count_if(stock.sites.begin(),stock.sites.end(),[](const Site &s){ return s.body.energy!=0; }));
        if (!survivors) throw std::invalid_argument("founder checkpoint contains no living organisms");
        Word sparse=survivors*32<count;
        std::vector<Word> addresses;
        std::vector<Genome> heritable;
        std::vector<SynapticGenes> synaptic;
        std::vector<HeritableLinks> recurrent;
        std::vector<ContextGenes> contextual;
        std::vector<ExpressionGenes> expression;
        std::vector<OutcomeGenes> digestive; std::vector<MapGenes> navigation;std::vector<TraceGenes> traces;
        bool send_map=c.navigation && !stock.maps.empty() && (part==Graft::whole || part==Graft::navigation);
        bool send_outcomes=outcomes && !stock.outcomes.empty() && (part==Graft::whole || part==Graft::outcomes);
        if (send_outcomes) digestive.reserve(survivors);
        bool core=part!=Graft::temporal && part!=Graft::navigation && part!=Graft::pathways && part!=Graft::outcomes && !memory;
        bool send_enzymes=(part==Graft::whole || part==Graft::dendrites) && layers && !stock.layers.empty();
        bool send_paths=(part==Graft::whole || part==Graft::pathways) && links && !stock.links.empty();
        bool send_context=(part==Graft::whole || memory) && c.relations==3 && !stock.contexts.empty();
        if (!sparse) addresses.reserve(count);
        if (core) heritable.reserve(survivors);
        if (send_enzymes) synaptic.reserve(survivors);
        if (send_paths) recurrent.reserve(survivors);
        if (send_context) contextual.reserve(survivors);
        bool send_expression=send_context && c.programs==2 && !stock.expression.empty();
        if (send_expression) expression.reserve(survivors);
        Word order=0;
        for (const auto &site:stock.sites) {
            if (!sparse) addresses.push_back(site.body.energy ? order : ~0u);
            if (!site.body.energy) continue;
            Word slot=site.body.slot; ++order;
            if (core) heritable.push_back(stock.genes[slot]);
            if (send_enzymes) synaptic.push_back(stock.layers[slot].genes);
            if (send_paths) recurrent.push_back({stock.links[slot].genes,stock.links[slot].convergence});
            if (send_context) contextual.push_back(stock.contexts[slot].genes);
            if (send_expression) expression.push_back(stock.expression[slot].genes);
            if (send_map) navigation.push_back(stock.maps[slot].genes);
            if ((send_map || part==Graft::temporal) && c.navigation>=4 && !stock.traces.empty()) traces.push_back(stock.traces[slot].genes);
            if (send_outcomes) digestive.push_back(stock.outcomes[slot].genes);
        }
        Temporary index(d,addresses.size()*sizeof(Word)), ancestry(d,heritable.size()*sizeof(Genome));
        Temporary enzymes(d,synaptic.size()*sizeof(SynapticGenes)), pathways(d,recurrent.size()*sizeof(HeritableLinks)),
                  contexts(d,contextual.size()*sizeof(ContextGenes)), switches(d,expression.size()*sizeof(ExpressionGenes));
        Temporary nutrients(d,digestive.size()*sizeof(OutcomeGenes)),maps(d,navigation.size()*sizeof(MapGenes));
        Temporary histories(d,traces.size()*sizeof(TraceGenes));
        if (histories.ptr) copy(traces,histories.ptr,true);
        if (maps.ptr) copy(navigation,maps.ptr,true);
        if (nutrients.ptr) copy(digestive,nutrients.ptr,true);
        if (index.ptr) copy(addresses,index.ptr,true);
        if (ancestry.ptr) copy(heritable,ancestry.ptr,true);
        if (enzymes.ptr) copy(synaptic,enzymes.ptr,true);
        if (pathways.ptr) copy(recurrent,pathways.ptr,true);
        if (contexts.ptr) copy(contextual,contexts.ptr,true);
        if (switches.ptr) copy(expression,switches.ptr,true);
        void *args[]{&index.ptr,&ancestry.ptr,&mask,&sparse,&survivors,&genes,&c,&enzymes.ptr,&layers,&pathways.ptr,&links,&contexts.ptr,&switches.ptr,&part,&pool,&pools,&nutrients.ptr,&outcomes,&maps.ptr,&histories.ptr};
        launch(transplant,sites,args);
        d.check(d.record(hb,nullptr),"record transplant"); d.check(d.wait(hb),"finish transplant");
    }
    void sow(ProducerStock &stock) {
        if (!flora || (stock.plants.empty() && !c.balanced)) throw std::invalid_argument("producer transplants need the matching thicket phenotype");
        Temporary stems(d,stock.plants.size()*sizeof(Plant)),branches(d,stock.reactions.size()*sizeof(Word)),
                  materials(d,c.balanced ? stock.food.size()*sizeof(Word) : 0);
        if (stems.ptr) copy(stock.plants,stems.ptr,true);
        if (branches.ptr) copy(stock.reactions,branches.ptr,true);
        if (materials.ptr) copy(stock.food,materials.ptr,true);
        Function sow{}; bind(sow,"plant_transplant");
        void *args[]{&flora,&pods,&c,&stems.ptr,&branches.ptr,&materials.ptr,&stock.config.side,&stock.config.climates};
        launch(sow,sites,args);
        d.check(d.record(hb,nullptr),"record producer transplant"); d.check(d.wait(hb),"complete producer transplant");
    }
    void nursery(Word generations, Word drift, bool joint, JointLaw law, Word values, NurseryCourse course, Revisit memory,Valuation specimen,bool short_practice,Travel voyage,Word chambers,JunctionTrial passage,Word vision) {
        Function lesson{}, brood{}, rehearse{}, valuation{},routes{};
        bool migrate=course==NurseryCourse::migration,overlap=course==NurseryCourse::junctions;
        if (overlap) bind(routes,"waymark"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),Image::junctions);
        if (course==NurseryCourse::navigation || migrate) bind(routes,std::string(migrate ? "migrants" : "navigator")+std::to_string(c.relations)+"_"+std::to_string(c.tiles),migrate ? (c.navigation>=4 ? Image::sequence_springs : Image::springbeds) : (c.navigation>=4 ? Image::sequence_routes : Image::travel));
        Word ordinary=course==NurseryCourse::paired || course==NurseryCourse::integrated ? 2 : (course==NurseryCourse::recall || course==NurseryCourse::outcomes || course==NurseryCourse::subsistence || course==NurseryCourse::navigation || migrate || overlap) ? 0 : 3;
        Word revisits=course==NurseryCourse::integrated ? 4 : course==NurseryCourse::repertoire ? 3 : Word(course==NurseryCourse::recall || course==NurseryCourse::lifelong);
        if (course==NurseryCourse::outcomes || course==NurseryCourse::subsistence) bind(valuation,std::string(course==NurseryCourse::outcomes ? "appetition" : "broodfood")+std::to_string(c.relations)+"_"+std::to_string(c.tiles),Image::appraisals);
        if (ordinary) bind(lesson,std::string(neural_prefix(c.relations))+"audition"+std::to_string(c.tiles),Image::assays);
        if (revisits) bind(rehearse,"rehearse"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),Image::memories);
        bind(brood,"life_brood",Image::assays);
        Temporary candidates(d, size_t(sites)*sizeof(Genome)), extra(d, layers ? size_t(sites)*sizeof(SynapticLayer) : 0);
        Temporary fitness(d, size_t(sites)*sizeof(Aptitude)), connections(d,size_t(sites)*association_stride(c.relations,c.programs));
        Temporary newborn_food(d,c.balanced ? size_t(sites)*(sizeof(OutcomeLayer)+map_bytes(c.navigation)) : 0);
        DevicePtr adult_food=outcomes,young_food=newborn_food.ptr;
        DevicePtr adult_links=links, young_links=connections.ptr;
        DevicePtr parents = genes, young = candidates.ptr, adults = layers, next = extra.ptr;
        auto *notebook=journal::current();
        constexpr Word observations=8;
        Word sampled=0;
        Function survey_brood{};
        std::vector<Word> sampled_generations;
        Temporary survey(d,notebook && drift!=1 ? observations*sizeof(journal::BroodScore) : 0);
        if (survey.ptr) {
            bind(survey_brood,"journal_brood",Image::scenes);
            d.check(d.zero(survey.ptr,0,observations*sizeof(journal::BroodScore)),"clear breeding survey");
            for (Word i=0;i<std::min(observations,generations);++i)
                sampled_generations.push_back(Word(Count(i)*(generations-1)/std::max(1u,std::min(observations,generations)-1)));
        }
        d.check(d.record(ha, nullptr), "start nursery timer");
        for (Word generation = 0; generation < generations; ++generation) {
            if (drift!=1) {
                d.check(d.zero(fitness.ptr, 0, size_t(sites)*sizeof(Aptitude)), "reset nursery trials");
                for (Word challenge = 0; challenge < ordinary; ++challenge) {
                    Word key = trial_key(c.seed ^ 0x67774e2du, challenge, generation);
                    Conditioning test{2, challenge ? values : 8u, (4 + (mix(key) & 7)) * (challenge ? values/4 : 1), key, bool(challenge), std::max(1u,c.digestion), bool(challenge) && joint,
                                      challenge == 2 ? JointLaw::chains : law, 2*challenge};
                    Word trials = 4 * test.quartets * sites;
                    for (Word first = 0; first < trials;) {
                        Word count = std::min(8192u, trials-first);
                        void *args[]{&parents, &fitness.ptr, &c, &test, &first, &count, &adults, &adult_links};
                        launch(lesson, count*c.tiles, args);
                        first += count;
                    }
                }
                if (routes) for (Word challenge=0;challenge<2;++challenge) {
                    Travel test=voyage;test.sets=1;test.seed=trial_key(c.seed^0x15acaebdu,challenge,generation);
                    test.depth=challenge ? 2+mix(test.seed)%(voyage.depth-1) : 1;
                    if (test.motivation==2) { test.motivation=(generation+challenge)&1;test.change_food=test.motivation && (generation&2); }
                    test.offset=2*challenge;
                    Word trials=4*sites,zero=0;DevicePtr none=0;
                    for (Word first=0;first<trials;) {
                        Word count=std::min(8192u,trials-first);
                        void *args[]{&parents,&adults,&adult_links,&adult_food,&none,&zero,&c,&test,&zero,&none,&fitness.ptr,&first,&count};
                        if (overlap) {
                            JunctionTrial setting=passage;setting.sets=1;setting.seed=test.seed;setting.offset=test.offset;
                            setting.delay=challenge ? passage.delay : mix(test.seed)%(passage.delay+1);
                            if (setting.traversal==3) setting.traversal=((generation>>1)+challenge)&1;
                            if (setting.noise==3) setting.noise=1+((generation+challenge)&1);
                            if (setting.motivation==2) setting.motivation=(generation+challenge)&1;
                            void *trial[]{&parents,&adults,&adult_links,&adult_food,&none,&zero,&c,&setting,&zero,&none,&fitness.ptr,&first,&count};
                            launch(routes,count*c.tiles,trial);
                        } else if (migrate) {
                            Word area=challenge ? chambers : std::max(8u,chambers/2);
                            void *forage[]{&parents,&adults,&adult_links,&adult_food,&none,&zero,&c,&test,&area,&vision,&zero,&none,&fitness.ptr,&first,&count,&none,&none,&none};
                            launch(routes,count*c.tiles,forage);
                        } else launch(routes,count*c.tiles,args);
                        first+=count;
                    }
                }
                if (valuation) for (Word challenge=0;challenge<2;++challenge) {
                    Valuation test=specimen;
                    test.items=values; test.sets=2; test.inferred=challenge; test.fitness_offset=2*challenge;
                    test.seed=trial_key(c.seed^0xeabf6c19u,challenge,generation);
                    test.pairings=1+mix(test.seed)%specimen.pairings;
                    if (short_practice && challenge==(generation&1)) test.rounds=1;
                    Word trials=4*test.sets*sites,survivors=0; DevicePtr none=0; ValueArm arm=ValueArm::intact;
                    for (Word first=0;first<trials;) {
                        Word count=std::min(4096u,trials-first);
                        void *args[]{&parents,&adults,&adult_links,&adult_food,&none,&survivors,&c,&test,&arm,&none,&fitness.ptr,&first,&count};
                        launch(valuation,count*c.tiles,args); first+=count;
                    }
                }
                for (Word challenge=0;challenge<revisits;++challenge) {
                    Revisit test=memory;
                    test.seed=trial_key(c.seed^0x22db1b67u,3+challenge,generation);
                    test.sets=2; test.items=std::max(8u,values);
                    test.regimes=2+mix(test.seed+3)%(memory.regimes-1);
                    test.delay=c.digestion ? c.digestion : memory.delay;
                    if (course==NurseryCourse::lifelong || course==NurseryCourse::repertoire || course==NurseryCourse::integrated) {
                        test.task=course==NurseryCourse::lifelong ? RevisitTask(mix(test.seed+1)%3) :
                                  challenge==3 ? RevisitTask::interleaved : RevisitTask(challenge);
                        test.marked=(mix(test.seed+2)&3)==0;
                        if (test.task==RevisitTask::paired) test.practice=std::max(32u,test.practice);
                    }
                    test.fitness_offset=(course==NurseryCourse::integrated ? 4u : 6u)+2*challenge;
                    Word trials=4*test.sets*sites;
                    for (Word first=0;first<trials;) {
                        Word count=std::min(4096u,trials-first);
                        void *args[]{&parents,&adults,&adult_links,&fitness.ptr,&c,&test,&first,&count};
                        launch(rehearse,count*c.tiles,args);
                        first+=count;
                    }
                }
            }
            if (sampled<sampled_generations.size() && generation==sampled_generations[sampled]) {
                DevicePtr dest=survey.ptr+sampled++*sizeof(journal::BroodScore);
                void *sample[]{&fitness.ptr,&sites,&dest};launch(survey_brood,sites,sample);
            }
            void *args[]{&parents, &young, &adults, &next, &fitness.ptr, &sites, &c.seed, &generation, &drift, &c.mutate, &adult_links, &young_links, &course, &c.relations, &c.programs,&adult_food,&young_food,&c.repair,&c.navigation};
            launch(brood, sites, args);
            std::swap(parents, young); std::swap(adults, next); std::swap(adult_links,young_links); std::swap(adult_food,young_food);
        }
        if (parents != genes) {
            if (outcomes) d.check(d.copy_device(outcomes,adult_food,size_t(sites)*(sizeof(OutcomeLayer)+map_bytes(c.navigation))),"release outcome genes");
            d.check(d.copy_device(genes, parents, size_t(sites)*sizeof(Genome)), "release nursery genomes");
            if (links) d.check(d.copy_device(links,adult_links,size_t(sites)*association_stride(c.relations,c.programs)), "release associative genes");
            if (layers) d.check(d.copy_device(layers, adults, size_t(sites)*sizeof(SynapticLayer)), "release synaptic genes");
        }
        d.check(d.record(hb, nullptr), "stop nursery timer"); d.check(d.wait(hb), "finish nursery");
        float ms; d.check(d.elapsed(&ms, ha, hb), "measure nursery");
        if (sampled) {
            std::vector<journal::BroodScore> records(sampled);copy(records,survey.ptr,false);
            for (Word i=0;i<sampled;++i) notebook->breeding.push_back({sampled_generations[i]+1,sites,records[i]});
        }
        std::cout << "nursery=" << (drift==1 ? "drift" : drift==2 ? "cross" : drift==3 ? "extend" : drift==4 ? "splice" : drift==5 ? "organ" : drift==8 ? "history-local" : drift==7 ? "history" : drift==6 ? "map" : "selection") << "  generations=" << generations
                  << "  candidates=" << sites << "  tested_lifetimes=" << (drift==1 ? 0ull : Count(sites)*generations*8*(ordinary+revisits+(valuation ? 2 : 0)+Word(bool(routes))))
                  << "  GPU_ms=" << std::fixed << std::setprecision(3) << ms << '\n';
    }
    void step(Word parity, DevicePtr clock, Word offset, Stream stream) {
        DevicePtr &old = grid[parity], &next = grid[parity ^ 1];
        void *a[]{&old, &genes, &brains, &intents, &clock, &offset, &c, &contacts, &pods, &delay, &layers, &links, &outcomes};
        DevicePtr climate = clock ? weather : 0;
        void *b[]{&old, &next, &genes, &intents, &records, &clock, &offset, &c, &pods, &harvest, &delay, &layers, &climate, &links, &flora, &propagation, &outcomes};
        launch(think, packed ? sites : sites*c.tiles, a, stream); launch(settle, sites / 4, b, stream);
    }
    GraphExec batch(Word parity) {
        auto &cached=program[2*Word(packed)+parity];
        if (cached) return cached;
        Graph graph{};
        d.check(graphs.begin(capture, 1), "begin ecological graph capture");
        bool capturing = true;
        try {
            Word length = graph_ticks;
            if (weather) {
                Word across = std::max(1u, c.side/16);
                void *weather_args[]{&weather, &epoch, &c};
                launch(forecast, graph_ticks*across*across, weather_args, capture);
            }
            for (Word i = 0; i < length; ++i) step(parity ^ (i & 1), epoch, i, capture);
            void *clock_args[]{&epoch, &length};
            d.check(d.launch(advance_clock, 1, 1, 1, 1, 1, 1, 0, capture, clock_args, nullptr), "capture ecological clock");
            auto rc = graphs.end(capture, &graph); capturing = false;
            d.check(rc, "finish ecological graph capture");
            d.check(graphs.instantiate(&cached, graph, 0), "instantiate ecological graph");
        } catch (...) {
            if (capturing) graphs.end(capture, &graph);
            if (graph) graphs.destroy(graph);
            throw;
        }
        d.check(graphs.destroy(graph), "release graph definition");
        return cached;
    }
    double advance_batch(Word steps) {
        GraphExec executable{};
        if (use_graph && steps >= graph_ticks) {
            executable = batch(tick & 1);
            d.check(d.upload(epoch, &tick, sizeof(tick)), "initialize ecological graph clock");
        }
        d.check(d.record(ha, nullptr), "start ecological timer");
        while (executable && steps >= graph_ticks) {
            d.check(graphs.launch(executable, nullptr), "replay ecological graph");
            tick += graph_ticks; steps -= graph_ticks;
        }
        for (Word i = 0; i < steps; ++i, ++tick) step(tick & 1, 0, tick, nullptr);
        d.check(d.record(hb, nullptr), "stop ecological timer"); d.check(d.wait(hb), "finish ecological ticks");
        float ms; d.check(d.elapsed(&ms, ha, hb), "measure ecological ticks"); return ms * .001;
    }
    double advance(Word steps) {
        double seconds=0;
        while (steps) {
            Word n=adaptive ? std::min(steps,256u-(tick&255u)) : steps;
            seconds+=advance_batch(n);
            steps-=n;
            if (steps) {
                Word key=c.seed^tick, sample=0;
                void *args[]{&grid[tick&1],&sites,&key,&totals};
                d.check(d.record(ha,nullptr),"start population sample");
                d.check(d.launch(occupancy,1,1,1,256,1,1,0,nullptr,args,nullptr),"sample population density");
                d.check(d.download(&sample,totals,sizeof(sample)),"read population sample");
                d.check(d.record(hb,nullptr),"stop population sample");
                d.check(d.wait(hb),"finish population sample");
                float ms; d.check(d.elapsed(&ms,ha,hb),"measure population sample");
                seconds+=ms*1e-3;
                consider_density(sample,4096);
            }
        }
        return seconds;
    }
    std::pair<Census, Word> inspect() {
        d.check(d.zero(totals, 0, sizeof(Census)), "clear census"); d.check(d.zero(families, 0, sites / 8), "clear family census");
        Word profile=Word(forage_profile || c.balanced || c.repair);
        void *args[]{&grid[tick & 1], &genes, &brains, &records, &totals, &families, &c, &contacts, &pods, &harvest, &delay, &layers, &links, &flora, &propagation, &outcomes,&tick,&profile};
        launch(census, sites, args);
        Census value; d.check(d.download(&value, totals, sizeof(value)), "read ecological census");
        std::vector<Word> families_seen(sites / 32); copy(families_seen, families, false);
        Word count = 0; for (auto word : families_seen) count += std::popcount(word);
        if (value.founders_energy + value.history.growth != value.reserve + value.plants + value.caches + value.digesting + value.history.spent+(c.balanced==2 ? value.carbon : value.stocks[0]+value.stocks[1]+value.stocks[2]+value.stocks[3]))
            throw std::runtime_error("ecosystem energy conservation failed at tick " + std::to_string(tick));
        if (c.balanced && value.history.grazed!=value.diet.intake[0]+value.diet.intake[1]+value.diet.intake[2]+value.diet.intake[3]) throw std::runtime_error("dietary intake accounting failed");
        if (c.climates) {
            Harvest counted{};
            for (auto group:value.climate.group) { counted.attempts+=group.attempts; counted.opened+=group.opened; }
            if (counted.attempts!=value.harvest.attempts || counted.opened!=value.harvest.opened)
                throw std::runtime_error("climate harvest accounting failed at tick "+std::to_string(tick));
        }
        return {value, count};
    }
    void probe_synapses(Word repeats) {
        bind(paired_assay,std::string(links ? neural_prefix(c.relations) : contacts ? "grove_" : "life_")+"assay"+std::to_string(c.tiles),Image::assays);
        Temporary storage(d, 3 * sizeof(Assay));
        DevicePtr buffer = storage.ptr;
        d.check(d.zero(buffer, 0, 3 * sizeof(Assay)), "initialize synaptic assay");
        for (Word control = 0; control < 3; ++control) {
            void *args[]{&grid[tick & 1], &genes, &brains, &buffer, &tick, &repeats, &control, &c, &contacts, &pods, &delay, &layers, &links, &outcomes};
            launch(paired_assay, sites*c.tiles, args);
        }
        Assay result[3];
        d.check(d.download(result, buffer, sizeof(result)), "read synaptic assay");
        const char *labels[]{"different_history", "identical_history", "frozen_induction"};
        for (int k = 0; k < 3; ++k) {
            auto r = result[k];
            if (k && (r.gates || r.motors || r.effectors || r.spikes)) throw std::runtime_error("paired synaptic control diverged");
            std::cout << "assay=" << labels[k] << "  trials=" << r.trials << "  different_gates=" << r.gates
                      << "  different_spikes=" << r.spikes << "  different_choices=" << r.motors
                      << "  different_effectors=" << r.effectors << '\n';
        }
    }
    void rules() {
        Function histogram{};
        bind(histogram,"life_rules");
        Temporary storage(d, 65536 * sizeof(Word));
        DevicePtr counts = storage.ptr;
        d.check(d.zero(counts, 0, 65536*sizeof(Word)), "clear allele histogram");
        void *args[]{&grid[tick & 1], &genes, &sites, &counts}; launch(histogram, sites, args);
        std::vector<Word> bins(65536); copy(bins, counts, false);
        std::vector<std::pair<Word, Word>> entries;
        for (Word rule = 0; rule < bins.size(); ++rule) if (bins[rule]) entries.emplace_back(bins[rule], rule);
        std::sort(entries.rbegin(), entries.rend());
        std::cout << "distinct_rules=" << entries.size();
        for (size_t i = 0; i < std::min<size_t>(8,entries.size()); ++i) {
            std::cout << "  rule_" << std::hex << std::setw(4) << std::setfill('0') << entries[i].second
                      << std::dec << std::setfill(' ') << '=' << entries[i].first;
        }
        std::cout << '\n';
    }
    void index_living(DevicePtr destination, bool positions) {
        if (grid_index && sites>=512) {
            Word stripes=sites/512;
            if (!resident_offsets) resident_offsets=allocate(size_t(stripes)*sizeof(Word));
            Function counts{},prefix{},scatter{};
            bind(counts,"life_stripes"); bind(prefix,"life_stripe_offsets");
            bind(scatter,positions ? "life_index_sites" : "life_index_slots");
            void *scan[]{&grid[tick&1],&stripes,&resident_offsets,&destination},*sum[]{&resident_offsets,&stripes};
            launch(counts,stripes*32,scan);
            d.check(d.launch(prefix,1,1,1,32,1,1,0,nullptr,sum,nullptr),"sum resident counts");
            launch(scatter,stripes*32,scan);
            return;
        }
        Function compact{};
        bind(compact,positions ? "life_positions" : "life_compact");
        void *args[]{&grid[tick & 1], &sites, &destination};
        d.check(d.launch(compact,1,1,1,32,1,1,0,nullptr,args,nullptr),"index living donors");
    }
    void survey(Word requested, Word seed, Word survivors) {
        Word samples = std::min(requested,survivors);
        std::map<std::array<Word,8>,std::pair<Word,Word>> inventory;
        Word sensitive = 0;
        if (samples) {
            Temporary positions(d,size_t(survivors)*sizeof(Word)), results(d,size_t(samples)*sizeof(BehavioralSample));
            index_living(positions.ptr,true);
            Function test{};
            bind(test,"survey"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),Image::assays);
            void *args[]{&grid[tick&1],&genes,&positions.ptr,&survivors,&samples,&seed,&c,&layers,&links,&results.ptr};
            launch(test,2*samples*c.tiles,args);
            std::vector<BehavioralSample> expressed(samples); copy(expressed,results.ptr,false);
            for (const auto &individual : expressed) {
                std::array<Word,8> key;
                std::memcpy(key.data(),individual.response,sizeof(key));
                sensitive += !std::equal(key.begin(),key.begin()+4,key.begin()+4);
                auto &group = inventory[key];
                if (!group.first) group.second = individual.site;
                ++group.first;
            }
        }
        std::cout << "survey=" << samples << "  survey_seed=" << seed << "  distinct=" << inventory.size()
                  << "  history_sensitive=" << sensitive << '\n';
        for (const auto &[key,group] : inventory) {
            std::cout << "phenotype=" << std::hex << std::setfill('0');
            for (Word word : key) for (Word byte=0; byte<4; ++byte) std::cout << std::setw(2) << ((word>>(8*byte))&255);
            std::cout << std::dec << std::setfill(' ') << "  count=" << group.first << "  site=" << group.second << '\n';
        }
    }
    void screen(Word requested,Word survivors,Valuation cfg,JunctionTrial *passage=nullptr) {
        const char *label=passage ? "junction_pupils" : "pupils";
        if (!survivors) { std::cout << label << "=empty  residents=0\n"; return; }
        Word n=std::min(requested,survivors);
        Temporary living(d,size_t(survivors)*sizeof(Word)),slots(d,size_t(n)*sizeof(Word)),
                  metadata(d,size_t(n)*sizeof(PupilSite)),fitness(d,size_t(n)*sizeof(Aptitude));
        index_living(living.ptr,true);
        Function sample{},assess{};
        bind(sample,"pupil_sites",Image::appraisals);
        if (passage) bind(assess,"waymark"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),Image::junctions);
        else bind(assess,"appetition"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),Image::appraisals);
        void *args[]{&grid[tick&1],&genes,&layers,&links,&outcomes,&living.ptr,&survivors,&n,&cfg.seed,&c,&slots.ptr,&metadata.ptr};
        launch(sample,n,args);
        std::vector<PupilSite> individuals(n); copy(individuals,metadata.ptr,false);
        auto evaluate=[&](Word count,Word sets,Word key) {
            cfg.sets=sets; cfg.seed=key; DevicePtr none=0;
            d.check(d.zero(fitness.ptr,0,size_t(count)*sizeof(Aptitude)),"clear individual learning scores");
            Word total=count*4*sets,arm_index=0;
            for (auto arm:{ValueArm::intact,ValueArm::silent,ValueArm::fixed}) {
                Word intervention=arm_index++;cfg.fitness_offset=2*intervention;
                if (passage) {passage->sets=sets;passage->seed=key;passage->offset=2*intervention;}
                for (Word first=0;first<total;) {
                    Word batch=std::min(4096u,total-first);
                    void *args[]{&genes,&layers,&links,&outcomes,&slots.ptr,&survivors,&c,&cfg,&arm,&none,&fitness.ptr,&first,&batch};
                    if (passage) {args[7]=passage;args[8]=&intervention;}
                    launch(assess,batch*c.tiles,args); first+=batch;
                }
            }
            std::vector<Aptitude> scores(count); copy(scores,fitness.ptr,false); return scores;
        };
        Word key=cfg.seed,sets=4,points_per_life=128*(passage ? passage->places : 1),max_points=4*sets*points_per_life;
        auto screen=evaluate(n,sets,key);
        std::vector<Word> order(n); Word over_half=0,over_three_quarters=0,dependent=0;
        Count total=0;
        std::set<Count> genotypes;
        for (Word i=0;i<n;++i) {
            order[i]=i; genotypes.insert(individuals[i].genotype); auto *s=screen[i].score;
            over_half+=2*s[1]>max_points; over_three_quarters+=4*s[1]>3*max_points; total+=s[1];
            dependent+=2*s[1]>max_points && s[1]>std::max(s[3],s[5])+max_points/8;
        }
        auto rank=[&](Word i) {
            auto *s=screen[i].score;
            int gain=int(s[1])-int(std::max(s[3],s[5]));
            return std::make_tuple(std::min(int(s[1])-int(max_points/4),gain),s[1],s[0]);
        };
        std::stable_sort(order.begin(),order.end(),[&](Word a,Word b){return rank(a)>rank(b);});
        std::vector<Word> selected,donors; std::set<Count> seen;
        for (Word i:order) {
            if (!seen.insert(individuals[i].genotype).second) continue;
            selected.push_back(i); donors.push_back(individuals[i].slot);
            if (selected.size()==16) break;
        }
        std::cout << label << "=screen  residents=" << n << "  genotypes=" << genotypes.size() << "  worlds=" << sets
                  << "  total=" << total << "  possible=" << Count(max_points)*n << "  over_half=" << over_half
                  << "  over_three_quarters=" << over_three_quarters << "  contact_dependent=" << dependent << '\n';
        copy(donors,slots.ptr,true);
        auto audit=evaluate(Word(selected.size()),64,mix(key^0x54cdf983u));
        for (Word r=0;r<selected.size();++r) {
            auto &person=individuals[selected[r]]; auto *s=audit[r].score;
            std::cout << label << "=audit  rank=" << r+1 << "  site=" << person.site << "  founder=" << person.founder
                      << "  generation=" << person.generation << "  genotype=" << std::hex << person.genotype << std::dec
                      << "  screen_changed=" << screen[selected[r]].score[1] << "  screen_points=" << max_points
                      << "  worlds=64  points=" << 4*64*points_per_life << "  learned=" << s[0] << "  changed=" << s[1]
                      << "  silent=" << s[3] << "  fixed=" << s[5] << '\n';
        }
    }
    void junctions(JunctionTrial cfg,Word survivors,bool controls) {
        constexpr std::array names{"intact","raw","trace-fixed","immediate","no-hold","hidden","swapped","labile","no-novelty","fixed-support"};
        if (!survivors) { std::cout << "junctions=extinct  items=0\n";return; }
        Temporary living(d,size_t(survivors)*sizeof(Word)),totals(d,names.size()*sizeof(PassageScore));
        index_living(living.ptr,false);d.check(d.zero(totals.ptr,0,names.size()*sizeof(PassageScore)),"clear ambiguous junction scores");
        Function run{};bind(run,"junction"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),Image::junctions);
        Word lives=4*cfg.sets,arms=controls ? Word(names.size())-Word(c.navigation<7) : 1;DevicePtr none=0;
        for (Word arm=0;arm<arms;++arm) for (Word first=0;first<lives;) {
            Word count=std::min(4096u,lives-first);
            void *args[]{&genes,&layers,&links,&outcomes,&living.ptr,&survivors,&c,&cfg,&arm,&totals.ptr,&none,&first,&count};
            launch(run,count*c.tiles,args);first+=count;
        }
        std::vector<PassageScore> rows(names.size());copy(rows,totals.ptr,false);
        for (Word i=0;i<arms;++i) {
            const auto &r=rows[i];
            std::cout << "junctions=" << names[i] << "  seed=" << cfg.seed << "  lives=" << lives << "  items=" << Count(lives)*16*cfg.places
                      << "  junction_types=" << cfg.places << "  rounds=" << cfg.rounds << "  retention=" << cfg.delay << "  practice_retention=" << cfg.practice_delay
                      << "  motor_clock=" << (cfg.lifetime_clock ? "lifetime" : "mixed") << "  test=" << (cfg.traversal==3 ? "mixed" : cfg.traversal==2 ? "frozen" : cfg.traversal ? "continuous" : "paired") << "  variable=" << cfg.variable
                      << "  varied=" << cfg.noise << "  practice_varied=" << cfg.practice_noise << "  distinct=" << cfg.visible
                      << "  destination=" << (cfg.motivation==2 ? "mixed" : cfg.motivation ? "food" : "landmark")
                      << "  naive=" << r.naive << "  learned=" << r.acquired << "  surprise=" << r.surprise << "  revised=" << r.revised
                      << "  attempts=" << r.observed << "  moves=" << r.moved << "  covered=" << r.covered << '\n';
        }
    }
    void crossings(Travel setting,Word survivors,bool controls) {
        constexpr std::array names{"intact","silent","fixed","one-pulse","erased","misbound","wrong-goal","old-exits","escorted","tonic","no-appetite","unpaired-meals"};
        if (!survivors) { std::cout << "crossings=extinct  items=0\n";return; }
        Temporary slots(d,size_t(survivors)*sizeof(Word)),scores(d,names.size()*sizeof(TravelScore));
        index_living(slots.ptr,false);d.check(d.zero(scores.ptr,0,names.size()*sizeof(TravelScore)),"clear route counters");
        Function run{};bind(run,"crossing"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),c.navigation>=4 ? Image::sequence_routes : Image::travel);
        Word lives=setting.sets*4,arms=controls ? Word(names.size()) : 1; DevicePtr none=0;
        for (Word arm=0;arm<arms;++arm) for (Word first=0;first<lives;) {
            Word count=std::min(8192u,lives-first);
            void *args[]{&genes,&layers,&links,&outcomes,&slots.ptr,&survivors,&c,&setting,&arm,&scores.ptr,&none,&first,&count};
            launch(run,count*c.tiles,args);first+=count;
        }
        std::vector<TravelScore> result(names.size());copy(result,scores.ptr,false);
        for (Word arm=0;arm<arms;++arm) {
            const auto &r=result[arm];
            std::cout << "crossings=" << names[arm] << "  worlds=" << setting.sets << "  depth=" << setting.depth
                      << "  destination=" << (setting.motivation==2 ? "mixed" : setting.motivation ? "food" : "landmark") << "  diversion=" << (setting.change_food==2 ? "both" : setting.change_food ? "food" : "doors")
                      << "  exposures=" << setting.rounds << "  items=" << Count(lives)*16 << "  naive=" << r.naive
                      << "  learned=" << r.first << "  surprise=" << r.surprise << "  changed=" << r.changed << "  former=" << r.old
                      << "  routes=" << Count(lives)*4 << "  arrived=" << r.arrived << "  redirected=" << r.redirected
                      << "  seen_answers=" << r.seen_answers << "  seen_items=" << r.seen_items << "  moves=" << r.steps << "  encounters=" << r.tried << "  displacements=" << r.observed << '\n';
        }
    }
    void migration(Travel setting,Word survivors,Word size,bool controls,Word vision) {
        if (!survivors) { std::cout << "migration=extinct  lives=0\n";return; }
        constexpr std::array names{"intact","silent","fixed","one-pulse","erased","misbound","wrong-goal","old-exits","escorted","tonic","no-appetite","unpaired-meals","raw","trace-fixed","immediate","no-hold","no-novelty"};
        Temporary slots(d,size_t(survivors)*sizeof(Word)),scores(d,names.size()*sizeof(GrowthScore));
        Word probe_arms=c.navigation>=4 ? 10 : 6;
        Temporary residence(d,names.size()*sizeof(ForageLedger));
        d.check(d.zero(residence.ptr,0,names.size()*sizeof(ForageLedger)),"clear foraging behavior counters");
        std::vector<Word> treatments{0,1,2,3,6,9,10};
        if (c.navigation>=4) treatments.insert(treatments.end(),{12,13,14,15,16});
        Temporary oracle(d,controls ? size_t(setting.sets)*sizeof(Maze) : 0),probes(d,controls ? probe_arms*sizeof(SpringChoice) : 0);
        if (controls) {
            Function lay{};bind(lay,"lay_maze",c.navigation>=4 ? Image::sequence_springs : Image::springbeds);Word catchment=1;
            void *args[]{&oracle.ptr,&setting.sets,&size,&setting.seed,&catchment};
            launch(lay,setting.sets*size*32,args);d.check(d.zero(probes.ptr,0,probe_arms*sizeof(SpringChoice)),"clear counterfactual needs");
        }
        index_living(slots.ptr,false);d.check(d.zero(scores.ptr,0,names.size()*sizeof(GrowthScore)),"clear spring foraging scores");
        Function run{};bind(run,"springs"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),c.navigation>=4 ? Image::sequence_springs : Image::springbeds);
        Word lives=4*setting.sets;DevicePtr none=0;
        for (Word arm:treatments) {
            if (!controls && arm) continue;
            for (Word first=0;first<lives;) {
                Word count=std::min(2048u,lives-first);
                void *args[]{&genes,&layers,&links,&outcomes,&slots.ptr,&survivors,&c,&setting,&size,&vision,&arm,&scores.ptr,&none,&first,&count,&oracle.ptr,&probes.ptr,&residence.ptr};
                launch(run,count*c.tiles,args);first+=count;
            }
        }
        std::vector<ForageLedger> movements(names.size());copy(movements,residence.ptr,false);
        std::vector<GrowthScore> result(names.size());copy(result,scores.ptr,false);
        for (Word arm:treatments) {
            if (!controls && arm) continue;
            const auto &r=result[arm];const auto &h=movements[arm];
            std::cout << "migration=" << names[arm] << "  lives=" << lives << "  rooms=" << size << "  vision=" << (vision==2 ? "shared" : vision ? "paired" : "distinct") << "  span=" << size*setting.rounds
                      << "  survived=" << r.alive << "  ticks=" << r.ticks << "  births=" << r.births << "  offspring_mass=" << r.offspring
                      << "  harvested=" << r.absorbed << "  oxidized=" << r.converted << "  energy=" << r.terminal << "  visited=" << r.visited
                      << "  at_spring=" << h.at_spring << "  away=" << h.away << "  depart_spring=" << h.depart_spring << "  depart_away=" << h.depart_away
                      << "  familiar_spring=" << h.returning << "  depart_familiar=" << h.depart_returning << "  meals=" << h.meals
                      << "  found_spring=" << h.found << "  first_arrival_sum=" << h.first_arrival << '\n';
        }
        if (controls) {
            constexpr std::array interventions{"resident","reset","silent","one-pulse","no-appetite","wrong-need","raw","immediate","no-hold","no-novelty"};
            std::vector<SpringChoice> rows(probe_arms);copy(rows,probes.ptr,false);
            for (Word j=0;j<rows.size();++j) {
                const auto &r=rows[j];std::cout << "spring_probe=" << interventions[j] << "  rooms=" << size << "  items=" << r.items
                    << "  correct=" << r.correct << "  far_items=" << r.far_items << "  far_correct=" << r.far_correct
                    << "  familiar_items=" << r.familiar_items << "  familiar_correct=" << r.familiar_correct << '\n';
            }
        }
    }
    void mazes(Travel setting,Word survivors,Word size,Word roam,bool controls) {
        constexpr std::array names{"intact","silent","fixed","one-pulse","erased","misbound","wrong-goal","old-exits","escorted","tonic","no-appetite","unpaired-meals"};
        if (!survivors) { std::cout << "maze=extinct  questions=0\n";return; }
        Temporary slots(d,size_t(survivors)*sizeof(Word)),maps(d,size_t(setting.sets)*sizeof(Maze)),scores(d,names.size()*sizeof(MazeScore));
        index_living(slots.ptr,false);d.check(d.zero(scores.ptr,0,names.size()*sizeof(MazeScore)),"clear maze results");
        Function prepare{},run{};bind(prepare,"lay_maze",c.navigation>=4 ? Image::sequence_mazes : Image::mazes);
        bind(run,"maze"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),c.navigation>=4 ? Image::sequence_mazes : Image::mazes);
        Word catchment=0;void *prepare_args[]{&maps.ptr,&setting.sets,&size,&setting.seed,&catchment};launch(prepare,setting.sets*size*32,prepare_args);
        Word lives=4*setting.sets,arms=controls ? Word(names.size()) : 1;
        for (Word arm=0;arm<arms;++arm) for (Word first=0;first<lives;) {
            Word count=std::min(2048u,lives-first);
            void *args[]{&genes,&layers,&links,&outcomes,&slots.ptr,&survivors,&c,&maps.ptr,&setting,&size,&roam,&arm,&scores.ptr,&first,&count};
            launch(run,count*c.tiles,args);first+=count;
        }
        std::vector<MazeScore> result(names.size());copy(result,scores.ptr,false);
        for (Word arm=0;arm<arms;++arm) {
            const auto &r=result[arm];
            std::cout << "maze=" << names[arm] << "  worlds=" << setting.sets << "  rooms=" << size
                      << "  destination=" << (setting.motivation==2 ? "mixed" : setting.motivation ? "food" : "landmark") << "  diversion=" << (setting.change_food==2 ? "both" : setting.change_food ? "food" : "doors")
                      << "  exploration=" << (roam ? "roam" : "local") << "  exposures=" << setting.rounds
                      << "  questions=" << r.questions << "  first=" << r.first << "  near_items=" << r.near_items << "  near=" << r.near_first
                      << "  far_items=" << r.far_items << "  far=" << r.far_first << "  changed_items=" << r.changed_items
                      << "  ready=" << r.ready << "  surprise=" << r.surprise << "  changed=" << r.turned << "  obsolete=" << r.obsolete
                      << "  journeys=" << r.journeys << "  arrived=" << r.reached << "  shortest=" << r.shortest << "  steps=" << r.steps
                      << "  minimum_steps=" << r.length << "  encounters=" << r.seen << "  distinct=" << r.distinct << "  visited=" << r.coverage << '\n';
        }
    }
    void appetites(Word requested,Word survivors,Word seed,Word repeats) {
        if (!survivors) { std::cout << "appetite=empty  residents=0\n"; return; }
        constexpr std::array names{"resident","silent","reset","innate","wrong_need","remote","map_silent","map_reset","one_pulse","no_appetite","raw","immediate","no_hold","trace_reset"};
        Word arms=c.navigation>=4 ? Word(names.size()) : c.navigation ? 10u : 6u;
        Word samples=std::min(survivors,requested);
        Temporary positions(d,size_t(survivors)*sizeof(Word)),scores(d,names.size()*sizeof(AppetiteScore));
        index_living(positions.ptr,true); d.check(d.zero(scores.ptr,0,names.size()*sizeof(AppetiteScore)),"clear resident food probe");
        Function assay{}; bind(assay,"appetite"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),c.navigation>=4 ? Image::sequence_appraisals : Image::appraisals);
        for (Word i=0;i<arms;++i) {
            AppetiteArm arm=AppetiteArm(i);
            void *args[]{&grid[tick&1],&genes,&brains,&contacts,&pods,&layers,&links,&outcomes,&flora,&delay,&positions.ptr,
                         &survivors,&samples,&tick,&seed,&repeats,&arm,&c,&scores.ptr};
            launch(assay,samples*c.tiles,args);
        }
        std::vector<AppetiteScore> out(names.size()); copy(out,scores.ptr,false);
        for (Word i=0;i<arms;++i) {
            const auto &r=out[i];
            std::cout << "appetite=" << names[i] << "  residents=" << r.sampled << "  age_sum=" << r.age << "  with_fruit=" << r.stocked
                      << "  objects=" << r.sampled*repeats << "  goals=" << r.sampled*repeats*4 << "  available=" << r.possible
                      << "  correct=" << r.correct << "  responsive=" << r.responsive;
            if (c.springs) std::cout << "  away=" << r.away << "  moving=" << r.moving << "  closer=" << r.closer
                                    << "  open=" << r.open << "  passable=" << r.passable;
            std::cout << '\n';
        }
    }
    void growth(Valuation cfg,Word survivors,bool controls) {
        if (!survivors) { std::cout << "growth=empty  lifetimes=0\n"; return; }
        constexpr std::array arms{ValueArm::intact,ValueArm::silent,ValueArm::fixed,ValueArm::no_chain,ValueArm::wrong_need};
        constexpr std::array names{"intact","silent","fixed","no_chain","wrong_need"};
        Temporary positions(d,size_t(survivors)*sizeof(Word)),scores(d,9*sizeof(GrowthScore));
        index_living(positions.ptr,false);
        d.check(d.zero(scores.ptr,0,9*sizeof(GrowthScore)),"clear foraging measurements");
        Function garden{}; bind(garden,"growfood"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),Image::appraisals);
        DevicePtr none=0; Word lives=4*cfg.sets,conditions=controls ? arms.size() : 1;
        for (Word i=0;i<conditions;++i) for (Word first=0;first<lives;) {
            Word count=std::min(4096u,lives-first); auto arm=arms[i];
            void *args[]{&genes,&layers,&links,&outcomes,&positions.ptr,&survivors,&c,&cfg,&arm,&scores.ptr,&none,&first,&count};
            launch(garden,count*c.tiles,args); first+=count;
        }
        std::vector<GrowthScore> out(9); copy(out,scores.ptr,false);
        for (Word i=0;i<conditions;++i) {
            const auto &r=out[Word(arms[i])];
            std::cout << "growth=" << names[i] << "  cue=" << (cfg.inferred ? "paired" : "single") << "  seed=" << cfg.seed
                << "  lifetimes=" << lives << "  span=" << cfg.rounds*cfg.items << "  survived=" << r.alive << "  lived_ticks=" << r.ticks
                << "  births=" << r.births << "  offspring_mass=" << r.offspring << "  harvested=" << r.absorbed
                << "  mineral_uptake=" << r.seep << "  converted=" << r.converted << "  reserves=" << r.terminal << '\n';
        }
    }
    void valuation(Valuation cfg,Word survivors,bool controls) {
        if (!survivors) { std::cout << "valuation=empty  lifetimes=0\n"; return; }
        constexpr std::array names{"intact","silent","fixed","unpaired","no_chain","wrong_need","erased","no_taste","wrong_object"};
        Temporary living(d,size_t(survivors)*sizeof(Word)),scores(d,names.size()*sizeof(ValueScore)); index_living(living.ptr,false);
        d.check(d.zero(scores.ptr,0,names.size()*sizeof(ValueScore)),"clear food choice scores");
        Function query{}; bind(query,"evaluate"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),Image::appraisals);
        Word lives=4*cfg.sets, arms=controls ? Word(names.size()) : 1; DevicePtr none=0;
        for (Word i=0;i<arms;++i) for (Word first=0;first<lives;) {
            ValueArm arm=ValueArm(i); Word count=std::min(4096u,lives-first);
            void *args[]{&genes,&layers,&links,&outcomes,&living.ptr,&survivors,&c,&cfg,&arm,&scores.ptr,&none,&first,&count};
            launch(query,count*c.tiles,args); first+=count;
        }
        std::vector<ValueScore> result(names.size()); copy(result,scores.ptr,false);
        for (Word i=0;i<arms;++i) {
            const auto &r=result[i];
            std::cout << "valuation=" << names[i] << "  task=" << (cfg.inferred ? "inferred" : "direct") << "  seed=" << cfg.seed
                << "  lifetimes=" << lives << "  objects=" << cfg.items << "  exposures=" << cfg.rounds << "  pairings=" << cfg.pairings
                << "  items=" << Count(lives)*cfg.items*4 << "  naive=" << r.naive << "  explored=" << r.explored << "  learned=" << r.learned
                << "  surprise=" << r.surprise << "  changed=" << r.changed << "  former=" << r.old << "  responsive_objects=" << r.sensitive
                << "  distinct_actions=" << r.actions << "  complete_objects=" << r.complete << "  trials=" << r.trials
                << "  observed=" << r.tasted << "  accessible_grips=" << r.reachable << '\n';
        }
    }
    void revisit(Revisit cfg,Word survivors,bool controls) {
        if (!survivors) { std::cout << "revisit=empty  lifetimes=0\n"; return; }
        Temporary slots(d,size_t(survivors)*sizeof(Word));
        index_living(slots.ptr,false);
        constexpr std::array<std::string_view,18> names{"intact","pooled","silent","fixed","ambiguous","erased","blind","no_binding","wrong_cue","no_reinstatement","fixed_confidence","labile","external_only","assigned","primary_only","alternate_only","swapped","paired_hint"};
        Temporary store(d,names.size()*sizeof(RevisitScore));
        d.check(d.zero(store.ptr,0,names.size()*sizeof(RevisitScore)),"clear memory-return scores");
        Function test{};
        bind(test,"revisit"+std::to_string(c.relations)+"_"+std::to_string(c.tiles),Image::memories);
        std::vector<RevisitArm> arms{RevisitArm::intact,RevisitArm::fixed,RevisitArm::ambiguous,RevisitArm::erased};
        if (c.relations==3) arms.insert(arms.end(),{RevisitArm::pooled,RevisitArm::silent,RevisitArm::blind,RevisitArm::no_binding,RevisitArm::no_reinstatement,RevisitArm::fixed_confidence,RevisitArm::labile,RevisitArm::external_only,RevisitArm::assigned});
        if (cfg.marked) arms.push_back(RevisitArm::wrong_cue);
        if (c.programs==2) arms.insert(arms.end(),{RevisitArm::primary_only,RevisitArm::alternate_only,RevisitArm::swapped});
        if (cfg.task==RevisitTask::interleaved) arms.push_back(RevisitArm::paired_hint);
        if (!controls) arms.resize(1);
        Word lifetimes=cfg.sets*4;
        for (auto arm:arms) for (Word first=0;first<lifetimes;) {
            Word count=std::min(4096u,lifetimes-first);
            void *args[]{&genes,&layers,&links,&slots.ptr,&survivors,&store.ptr,&c,&cfg,&arm,&first,&count};
            launch(test,count*c.tiles,args); first+=count;
        }
        std::vector<RevisitScore> scores(names.size()); copy(scores,store.ptr,false);
        Word per=recall_items(cfg);
        Count items=Count(lifetimes)*cfg.regimes*per;
        for (auto arm:arms) for (Word stage=0;stage<recall_stages;++stage) {
            const auto &run=scores[Word(arm)]; const auto &s=run.stage[stage];
            std::cout << "revisit=" << names[Word(arm)] << "  task=" << memory_tasks[Word(cfg.task)] << "  landmarks=" << cfg.marked
                      << "  regimes=" << cfg.regimes << "  varieties=" << cfg.items << "  exposures=" << cfg.practice
                      << "  seed=" << cfg.seed << "  lifetimes=" << lifetimes << "  reminders=" << recall_dose(stage)
                      << "  items=" << items << "  acquired=" << run.acquired << "  correct=" << s.correct
                      << "  returning_items=" << Count(lifetimes)*(cfg.regimes-1)*per << "  returning=" << s.returning
                      << "  identified_items=" << s.identified << "  identified_correct=" << s.identified_correct
                      << "  feedback_ceiling_twelfths=" << s.oracle_units << "  training_trials=" << run.trained
                      << "  training_rewards=" << run.positive << "  alternate_trials=" << run.alternate_trials
                      << "  assemblies_used=" << run.used << "  accessible_grips=" << run.reach
                      << "  per_regime_items=" << Count(lifetimes)*per << "  base_first=" << run.base_first << "  base_final=" << run.base_final;
            std::cout << "  anchor_items=" << run.anchor_items << "  anchor_correct=" << run.anchor_correct
                      << "  base_anchor_first=" << run.base_anchor_first << "  base_anchor_final=" << run.base_anchor_final;
            for (Word r=0;r<cfg.regimes;++r) std::cout << "  acquired_" << r << '=' << run.acquired_in[r] << "  recall_" << r << '=' << s.by_regime[r];
            if (cfg.task==RevisitTask::exceptions) {
                Count block=Count(lifetimes)*(cfg.regimes-1), except=cfg.items/4;
                std::cout << "  exception_items=" << block*except << "  exception_correct=" << s.exceptions
                          << "  transfer_items=" << block*(per-except) << "  transfer_correct=" << s.transfer;
            }
            if (cfg.task==RevisitTask::interleaved) {
                Count unary=Count(lifetimes)*cfg.regimes*(cfg.items-2);
                std::cout << "  unary_items=" << unary << "  unary_correct=" << s.exceptions
                          << "  paired_items=" << items-unary << "  paired_correct=" << s.transfer;
                for (Word r=0;r<cfg.regimes;++r) std::cout << "  acquired_unary_" << r << '=' << run.acquired_subset[r];
            }
            std::cout << '\n';
        }
    }
    void condition(Conditioning cfg, Word survivors, bool dissect) {
        if (!survivors) { std::cout << "conditioning=empty  panels=0\n"; return; }
        Temporary donors(d, size_t(survivors) * sizeof(Word));
        DevicePtr live = donors.ptr;
        Function probe{};
        bind(probe,std::string(neural_prefix(c.relations))+"condition"+std::to_string(c.tiles),Image::assays);
        index_living(live,false);
        Word trials = panel_variants(cfg) * cfg.quartets, exposures = 2 * cfg.varieties * cfg.exposures;
        Temporary replay(d, size_t(trials) * ((exposures+31)/32) * sizeof(Word));
        DevicePtr trace = replay.ptr;
        std::vector<Word> arms{0,1,2,3};
        if (dissect) for (Word k = 4; k < (c.digestion ? 28u : 20u); ++k) arms.push_back(k);
        if (layers) {
            arms.push_back(28);
            if (dissect) for (Word k = 29; k < 77; ++k) arms.push_back(k);
        }
        if (links) for (Word k=77; k<85; ++k) arms.push_back(k);
        if (c.relations >= 2) arms.push_back(85);
        auto capacity = arms.back() + 1;
        Temporary measurements(d, capacity * sizeof(ConditioningScore));
        DevicePtr scores = measurements.ptr;
        d.check(d.zero(scores, 0, capacity * sizeof(ConditioningScore)), "initialize conditioning measurements");
        for (Word arm : arms) {
            for (Word first = 0; first < trials;) {
                Word count = std::min(4096u, trials - first);
                void *args[]{&genes, &live, &survivors, &trace, &scores, &c, &cfg, &arm, &first, &count, &layers, &links};
                launch(probe, count * c.tiles, args);
                first += count;
            }
        }
        std::vector<ConditioningScore> results(capacity); copy(results, scores, false);
        const char *names[]{"intact","yoked","fixed_contacts","scrambled_motor"};
        if (results[0].delivered != results[1].delivered) throw std::runtime_error("yoked outcome counts diverged");
        Count items = Count(trials) * cfg.varieties * probe_width(cfg);
        for (Word arm : arms) {
            auto r = results[arm];
            std::string label;
            if (arm < 4) label = names[arm];
            else if (arm < 20) label = "ancestral_row_" + std::to_string(arm-4);
            else if (arm < 28) label = "conduction_" + std::to_string(arm-19);
            else if (arm == 28) label = "fixed_second_state";
            else if (arm < 45) label = "ancestral_row_" + std::to_string(arm-29+16);
            else if (arm < 77) label = "ancestral_state_" + std::to_string(arm-45);
            else {
                const char *control[]{"direct_only","silent_switches","silent_recall","fixed_pairs","fixed_recall","open_gates","no_convergence","fixed_range","short_range"};
                label = control[arm-77];
            }
            std::cout << "conditioning=" << label << "  problem=" << (cfg.composition ? law_names[Word(cfg.law)] : cfg.dimensions ? "dimensions" : "grips") << "  seed=" << cfg.seed << "  lifetimes=" << trials
                      << "  varieties=" << cfg.varieties << "  probe_delay=" << cfg.delay << "  exposures=" << cfg.exposures << "  items=" << items
                      << "  reachable=" << r.reachable << "  naive=" << r.naive << "  learned=" << r.learned << "  practiced_items=" << Count(trials)*cfg.varieties*(cfg.dimensions ? std::min(cfg.exposures,practice_width(cfg)) : 1)
                      << "  practiced=" << r.practiced << "  erased=" << r.erased
                      << "  changed_items=" << r.changed_items << "  stable_items=" << items-r.changed_items << "  surprise=" << r.surprise << "  after_one=" << r.one
                      << "  recovered=" << r.recovered << "  retained=" << r.retained << "  attempts=" << r.attempts
                      << "  readout_cut=" << r.readout_cut << "  connected=" << r.connected
                      << "  discordant_items=" << r.discordant_items << "  discordant_correct=" << r.discordant_correct
                      << "  alternate_correct=" << r.alternate_correct
                      << "  distant_items=" << r.distant_items << "  distant_correct=" << r.distant_correct
                      << "  remote_items=" << r.remote_items << "  remote_correct=" << r.remote_correct
                      << "  local_readout=" << r.local_readout << "  remote_local=" << r.remote_local
                      << "  delivered=" << r.delivered << "  successes=" << r.successes << '\n';
        }
    }
    void rank_population(journal::Report &r) {
        Function readout{};bind(readout,"journal_population",Image::scenes);
        Temporary packed(d,size_t(sites)*sizeof(journal::CensusMember));
        void *args[]{&grid[tick&1],&genes,&c,&packed.ptr};launch(readout,sites,args);
        std::vector<journal::CensusMember> members(sites);copy(members,packed.ptr,false);
        journal::ranked_forms(r,std::move(members));r.metrics["census_tick"]=tick;
    }
    void photograph(journal::Report &r,journal::Moment moment) {
        Word edge=std::min(128u,c.side),limit=std::min(1024u,sites);
        Temporary pixels(d,size_t(edge)*edge*sizeof(journal::FieldTile)),animals(d,size_t(limit)*sizeof(journal::Animal));
        Function scene{};bind(scene,"journal_field",Image::scenes);
        void *args[]{&grid[tick&1],&genes,&contacts,&c,&edge,&limit,&pixels.ptr,&animals.ptr,&outcomes};
        launch(scene,edge*edge,args);
        r.side=c.side;r.resolution=edge;r.tiles.resize(size_t(edge)*edge);copy(r.tiles,pixels.ptr,false);
        journal::Frame f{moment,std::vector<journal::Animal>(limit)};copy(f.residents,animals.ptr,false);
        std::erase_if(f.residents,[](const auto &a){return !a.energy;});
        r.frames.push_back(std::move(f));
    }
    void map() {
        std::vector<Site> habitat(sites); copy(habitat, grid[tick & 1], false);
        Word stride = std::max(1u, c.side / 64);
        std::cout << "Map  @ dense life  o life  + vegetation  . sparse vegetation\n";
        for (Word y = 0; y < c.side; y += stride) {
            for (Word x = 0; x < c.side; x += stride) {
                Word occupants = 0, food = 0, area = stride*stride;
                for (Word dy = 0; dy < stride; ++dy) for (Word dx = 0; dx < stride; ++dx) {
                    auto &s = habitat[(y+dy)*c.side + x+dx]; occupants += s.body.energy > 0; food += s.food;
                }
                std::cout << (occupants*2 > area ? '@' : (occupants ? 'o' : (food > 36*area ? '+' : '.')));
            }
            std::cout << '\n';
        }
    }
};
} // namespace

Individual hatch_from_habitat(const char *file, Word site, Word &origin, AssociationLayer &links, ContextLayer &contexts, ExpressionLayer &expression,bool resident,OutcomeLayer *outcome,Word *metabolism,MapLayer *map,Word *navigation,TraceLayer *trace) {
    std::ifstream in(file,std::ios::binary | std::ios::ate);
    if (!in) throw std::runtime_error(std::string("cannot open habitat ") + file);
    Header header{};
    size_t bytes = read_header(in,header), size = size_t(header.config.side)*header.config.side;
    // Verify the complete habitat, then seek just the chosen animal's planes.
    verify_world_file(in,bytes,file);
    auto header_size = header_bytes(header.version);
    origin = site == ~0u ? 0 : site;
    if (origin >= size) throw std::invalid_argument("donor site exceeds habitat");
    in.seekg(std::streamoff(header_size + origin*sizeof(Site)));
    Site occupant{};
    for (;;) {
        if (!in.read(reinterpret_cast<char *>(&occupant),sizeof(occupant)) || !valid_site(occupant,header,size))
            throw std::runtime_error("invalid donor site");
        if (occupant.body.energy) break;
        if (site != ~0u) throw std::invalid_argument("the donor site must be occupied");
        if (++origin == size) throw std::invalid_argument("the habitat contains no living donors");
    }
    Word slot = occupant.body.slot;
    Individual individual{};
    links={}; contexts={}; expression={};
    Word age=resident ? header.tick-occupant.body.born : 0;
    auto read_at=[&](size_t offset,void *dest,size_t count) {
        in.seekg(std::streamoff(offset));
        if (!in.read(static_cast<char *>(dest),std::streamsize(count))) throw std::runtime_error("missing donor tissue");
    };
    size_t genomes=header_size+size*sizeof(Site);
    read_at(genomes+slot*sizeof(Genome),&individual.genes,sizeof(Genome));
    size_t end=bytes-sizeof(Count);
    if (navigation) *navigation=header.config.navigation;
    if (trace) *trace={};
    if (header.config.navigation>=4) {
        end-=size*sizeof(TraceLayer);
        if (trace) read_at(end+slot*sizeof(TraceLayer),trace,age ? sizeof(TraceLayer) : sizeof(TraceGenes));
    }
    if (map) *map={};
    if (header.config.navigation) {
        end-=size*sizeof(MapLayer);
        if (map) read_at(end+slot*sizeof(MapLayer),map,age ? sizeof(MapLayer) : sizeof(MapGenes));
    }
    if (outcome) *outcome={};
    if (metabolism) *metabolism=header.config.balanced;
    if (header.config.balanced) {
        end-=size*sizeof(OutcomeLayer);
        if (outcome) read_at(end+slot*sizeof(OutcomeLayer),outcome,age ? sizeof(OutcomeLayer) : sizeof(OutcomeGenes));
        end-=size*sizeof(Word)+climate_shards(Word(size))*(sizeof(DietLedger)+((header.config.seep || header.config.springs) ? sizeof(SeepLedger) : 0)+(header.config.leach ? sizeof(Count) : 0));
    }
    if (header.config.programs==2) {
        end-=size*sizeof(ExpressionLayer);
        read_at(end+slot*sizeof(ExpressionLayer),&expression,age ? sizeof(ExpressionLayer) : sizeof(ExpressionGenes));
        individual.alternate_program=1+header.config.polarity;
    }
    if (header.config.relations==3) {
        end -= size*sizeof(ContextLayer);
        read_at(end+slot*sizeof(ContextLayer),&contexts,age ? sizeof(ContextLayer) : sizeof(ContextGenes));
    }
    if (header.config.relations) {
        size_t stride = association_bytes(header.version);
        end -= size*stride;
        size_t address = end + slot*stride;
        read_at(address,&links,age ? stride : sizeof(LinkGenes));
        if (!age && header.version>=7) read_at(address+offsetof(AssociationLayer,convergence),&links.convergence,sizeof(Word));
        individual.has_relations = header.config.relations;
    }
    if (header.config.synapse_bits == 2) {
        read_at(end-(size-slot)*sizeof(SynapticLayer),&individual.layer,age ? sizeof(SynapticLayer) : sizeof(SynapticGenes));
    }
    individual.tiles = header.config.tiles; individual.bits = header.config.synapse_bits;
    individual.delayed_path = header.config.digestion != 0;
    individual.outcome_lag = std::max(header.config.digestion, 1u);
    individual.seed = mix(header.config.seed ^ slot);
    individual.has_dendrites = header.config.habitat != Habitat::meadow;
    if (age) {
        individual.ticks=age;
        individual.due=Word(occupant.body.recent)<<(2*(age&7));
        size_t tissue=genomes+size*sizeof(Genome), slice=individual.tiles*sizeof(Tissue);
        read_at(tissue+slot*slice,individual.tissue,slice);
        if (individual.has_dendrites) {
            size_t contacts=tissue+size*slice+(size/4)*sizeof(Ledger);
            read_at(contacts+slot*sizeof(Contacts),&individual.contacts,sizeof(Contacts));
            DelayLine line{};
            if (individual.delayed_path) {
                size_t history=contacts+size*sizeof(Contacts)+size+(size/4)*sizeof(Harvest);
                read_at(history+slot*sizeof(DelayLine),&line,sizeof(line));
            }
            Word remembered=std::min(age,individual.delayed_path ? 8u : 1u);
            Word wiring=individual.has_relations || header.config.balanced ? 31 : (individual.genes.edges[1]>>10)&31;
            for (Word lag=1;lag<=remembered;++lag) {
                Word issued=individual.delayed_path ? line.discharge[(header.tick-lag)&7] : individual.contacts.previous;
                individual.afferent_history[(age-lag)&7]=issued ? pattern(earlier_cue(issued),wiring) : 0;
                individual.motor_history|=((issued>>5)&7)<<(3*(lag-1));
            }
        }
    }
    return individual;
}

namespace {
class FieldObserver final : public HabitatObserver {
    World field;
    Function sample{},locate_resident{};
    DevicePtr signals{},nearest_key{};
public:
    explicit FieldObserver(Snapshot &snapshot):field(snapshot.h.config,true,false,false) {
        field.setup();field.state(snapshot,true);
        field.bind(sample,"voice_field_view",Image::vocal_brood);signals=field.allocate(sizeof(HabitatSignals));
        field.bind(locate_resident,"voice_find_resident",Image::vocal_brood);nearest_key=field.allocate(sizeof(Word));
    }
    Word side() const override {return field.c.side;}
    Word tick() const override {return field.tick;}
    HabitatSignals observe(Word x,Word y) override {
        if (x>=side() || y>=side()) throw std::invalid_argument("viewpoint outside the habitat");
        Word radius=std::min(8u,(side()-1)/2);auto grid=field.grid[tick()&1];
        void *args[]{&grid,&field.c,&field.tick,&x,&y,&radius,&signals};
        field.d.check(field.d.launch(sample,1,1,1,128,1,1,0,nullptr,args,nullptr),"sample the local food and spring fields");
        HabitatSignals v{};field.d.check(field.d.download(&v,signals,sizeof(v)),"read directional currents");return v;
    }
    bool locate(ResidentView &animal,Word x,Word y,bool nearest) override {
        Word side=field.c.side,area=side*side,acquire=nearest,key=~0u;auto grid=field.grid[tick()&1];
        field.d.check(field.d.upload(nearest_key,&key,sizeof(key)),"open resident search");
        void *args[]{&grid,&side,&x,&y,&animal,&acquire,&nearest_key};
        field.d.check(field.d.launch(locate_resident,(area+127)/128,1,1,128,1,1,0,nullptr,args,nullptr),"locate an animal in the living world");
        field.d.check(field.d.download(&key,nearest_key,sizeof(key)),"read observed resident position");
        if (key==~0u) return false;
        Word position=key%area;Body b{};
        field.d.check(field.d.download(&b,grid+Count(position)*sizeof(Site)+offsetof(Site,body),sizeof(b)),"identify the living resident");
        animal={b.slot,b.founder,b.born,b.generation,position};return true;
    }
    void advance(Word steps) override {
        if (Count(field.c.ancestor_depth)+field.tick+steps>~0u) throw std::invalid_argument("habitat clock exhausted");
        field.advance(steps);
    }
    void save(const std::string &path) override {
        Snapshot state;field.state(state,false);write_checked(path,state);
    }
};
}
std::unique_ptr<HabitatObserver> open_habitat(const std::string &path) {
    auto restored=read_snapshot(path);
    return std::make_unique<FieldObserver>(restored);
}

int command(int argc, char **argv) {
    if (argc == 3 && std::string_view(argv[2]) == "--help") { std::cout << help; return 0; }
    auto o = options(argc, argv);
    std::optional<Snapshot> resume;
    if (!o.load.empty()) resume = read_snapshot(o.load);
    if (o.neurons && !std::has_single_bit(*o.neurons)) throw std::invalid_argument("neural capacity must be a power of two");
    Settings c = resume ? resume->h.config : Settings{o.side.value_or(64), o.neurons.value_or(256)/32, o.seed.value_or(1), o.density.value_or(35), 1, 1, 0, o.contest.value_or(0), Word(o.swap)};
    if (o.relations) c.relations = *o.relations;
    if (o.synapse_bits) c.synapse_bits = *o.synapse_bits;
    if (o.season) c.season = *o.season;
    if (o.climates) c.climates=*o.climates;
    if (o.programs) c.programs=*o.programs;
    if (o.polarity) c.polarity=*o.polarity;
    if (o.stand) c.stand=*o.stand;
    if (o.seep) c.seep=*o.seep;
    if (o.leach) c.leach=*o.leach;
    if (o.repair) c.repair=*o.repair;
    if (o.husk) c.husk=*o.husk;
    if (o.navigation) c.navigation=*o.navigation;
    if (o.springs) c.springs=*o.springs;
    if (o.basin_width) c.basin_width=*o.basin_width;
    if (o.balanced) c.balanced=*o.balanced;
    if (o.climate_cue) c.climate_cue=*o.climate_cue;
    if (o.species) c.species = *o.species;
    if (o.digestion) c.digestion = *o.digestion;
    if (o.habitat) c.habitat = *o.habitat;
    if (o.flora) {
        if (c.habitat != Habitat::thicket) throw std::invalid_argument("--flora requires a thicket habitat");
        c.flora = *o.flora;
    }
    if (o.fruit) {
        if (c.habitat != Habitat::thicket) throw std::invalid_argument("--fruit requires a thicket habitat");
        c.fruit = *o.fruit;
    }
    if (o.mutate) c.mutate = *o.mutate;
    if (o.plastic) c.plastic = *o.plastic;
    if (o.random) c.random_actions = *o.random;
    if (c.contest == Contest::plasticity && !c.plastic) throw std::invalid_argument("plasticity competition requires --plasticity on");
    if (c.contest == Contest::agency && c.random_actions) throw std::invalid_argument("agency competition requires --behavior neural");
    if (c.contest == Contest::dendritic_plasticity && (c.habitat == Habitat::meadow || !c.plastic))
        throw std::invalid_argument("dendritic competition requires a grove and enabled plasticity");
    if (c.contest == Contest::recurrent_paths && !c.relations) throw std::invalid_argument("the paths contest requires associative connections");
    if (c.contest == Contest::distal_paths && c.relations < 2) throw std::invalid_argument("the waves contest requires --relations waves");
    if ((c.contest==Contest::contextual_memory || c.contest==Contest::contextual_evidence) && c.relations!=3)
        throw std::invalid_argument("context competitions require --relations contexts");
    if (c.contest>=Contest::prospective && !c.balanced) throw std::invalid_argument("outcome competitions require a balanced metabolism");
    if ((o.breeding==5 || o.pupils || o.passage_pupils || o.appetites || o.growth || o.value.sets || o.course==NurseryCourse::outcomes || o.course==NurseryCourse::subsistence) && !c.balanced) throw std::invalid_argument("outcome experiments require a balanced metabolism");
    if ((o.breeding==3 || o.breeding==4) && c.relations!=3) throw std::invalid_argument("this breeding treatment needs context genes");
    if ((o.migration || o.course==NurseryCourse::migration) && c.balanced!=2) throw std::invalid_argument("spring foraging requires food-fuel metabolism");
    if ((o.travel.sets || o.mazes || o.passage.sets || o.course==NurseryCourse::junctions) && c.habitat==Habitat::meadow) throw std::invalid_argument("navigation experiments need a grove");
    if (o.reset_contacts && (o.load.empty() || c.habitat==Habitat::meadow))
        throw std::invalid_argument("contact reset operates on a resumed dendritic habitat");
    if (c.contest == Contest::metaplasticity && (c.synapse_bits != 2 || !c.plastic))
        throw std::invalid_argument("second-state competition requires four-state synapses with plasticity enabled");
    if ((o.nursery || o.dissect) && c.habitat == Habitat::meadow) throw std::invalid_argument("this experiment requires a grove habitat");
    validate(c);
    // The packed mapping has fewer independent warps. Dense dispatch is
    // faster for the small habitats and the widest tissue on the laptop GPU.
    bool packed = o.packed.value_or(c.habitat != Habitat::meadow &&
                  ((c.side >= 256 && c.tiles <= 16) || (c.side >= 128 && c.tiles == 2)));
    bool adaptive=!o.packed.has_value() && c.relations==3 && c.tiles==8 && c.side>=256;
    World world(c,o.graphs,packed,adaptive);
    world.forage_profile=o.forage_profile; world.grid_index=o.grid_index.value_or(world.sites>=16384); world.setup();
    if (resume) { world.state(*resume, true); resume.reset(); } else world.start(c.seed);
    if (o.reset_contacts) world.reset_contacts();
    if (o.founder_site) {
        Word origin{};
        AssociationLayer associations{}; ContextLayer contextual{}; ExpressionLayer expression{};
        OutcomeLayer outcome{}; MapLayer map{};TraceLayer trace{}; Word typed=0,navigation=0;
        auto organism=hatch_from_habitat(o.ancestors.c_str(),*o.founder_site,origin,associations,contextual,expression,false,&outcome,&typed,&map,&navigation,&trace);
        Donors from;
        Site only{}; only.body.energy = 1;
        from.sites.push_back(only); from.genes.push_back(organism.genes);
        if (organism.bits == 2) from.layers.push_back(organism.layer);
        if (organism.has_relations) from.links.push_back(associations);
        if (organism.has_relations==3) from.contexts.push_back(contextual);
        if (organism.alternate_program) from.expression.push_back(expression);
        if (typed) from.outcomes.push_back(outcome);
        if (navigation) from.maps.push_back(map);
        if (navigation>=4) from.traces.push_back(trace);
        world.seed_from(from);
        std::cout << "donor_site=" << origin << '\n';
    } else if (!o.ancestors.empty()) { auto from = read_snapshot(o.ancestors); world.seed_from(from); }
    for (Word i=0;i<o.context_ancestors.size();++i) {
        auto from=read_snapshot(o.context_ancestors[i]);
        world.seed_from(from,Graft::memory,i,Word(o.context_ancestors.size()));
    }
    if (!o.alternate_founders.empty()) { auto from=read_snapshot(o.alternate_founders); world.seed_from(from,Graft::alternative); }
    for (const auto &[part,path]:o.grafts) { auto from=read_snapshot(path); world.seed_from(from,part); }
    if (!o.plant_founders.empty()) { auto stock=read_producers(o.plant_founders,false); world.sow(stock); }
    if (!o.food_genes.empty()) { auto stock=read_producers(o.food_genes,true); world.sow(stock); }
    if (o.modifier_origin.value_or(0) || o.mouths.value_or(0))
        world.modifier_founders(o.modifier_origin.value_or(0),o.mouths.value_or(0));
    if (o.trace_origin) world.vary_histories(o.trace_origin);
    if (o.nursery) world.nursery(o.nursery,o.breeding,o.joint_nursery,o.nursery_law,o.nursery_varieties,o.course,o.revisit,o.value,o.short_practice,o.travel,o.rooms,o.passage,o.vision);
    if (!o.colonize && o.steps > std::numeric_limits<Word>::max()-world.tick) throw std::overflow_error("ecological clock would overflow");
    auto [last, family_count] = world.inspect();
    world.consider_density(last.alive,world.sites);
    std::cout << world.name << "  side=" << c.side << "  neural_capacity=" << c.tiles*32 << "  seed=" << c.seed
              << "  GPU_bytes=" << world.allocated << "  neural_regs/thread=" << world.regs << "  neural_local_B/thread=" << world.local
              << "  habitat_regs/thread=" << world.settle_regs << "  habitat_local_B/thread=" << world.settle_local
              << "  habitat=" << habitat_names[Word(c.habitat)] << "  digestion=" << c.digestion << "  species=" << c.species << "  relations=" << c.relations << "  programs=" << c.programs << "  polarity=" << c.polarity << "  synapse_bits=" << c.synapse_bits << "  season=" << c.season
              << "  spring_flow=" << c.springs << "  basin_width=" << c.basin_width << "  navigation=" << c.navigation << "  inheritance=" << inheritance_names[c.repair] << "  metabolism=" << (c.balanced==2 ? "food-fuel" : c.balanced ? "balanced" : "energy") << "  stand=" << c.stand << "  seep=" << c.seep << "  leach=" << c.leach << "  husk=" << c.husk
              << "  schedule=" << (world.packed ? "occupied" : "dense") << "  adaptive_schedule=" << adaptive << "  dispatch=" << (o.graphs ? "graph" : "direct") << "  contest=" << contest_names[c.contest] << "  swapped=" << c.swap
              << "  plasticity=" << c.plastic << "  mutation=" << c.mutate << "  behavior=" << (c.random_actions ? "random" : "neural") << '\n';
    auto print = [&](const Census &r, Word families, double seconds) {
        std::cout << "tick=" << world.tick << "  alive=" << r.alive << "  founders=" << families << "  max_generation=" << r.deepest
                  << "  births=" << r.history.births << "  deaths=" << r.history.deaths << "  edits=" << r.history.edits
                  << "  tape_edits=" << r.history.structural << "  mean_tape=" << std::fixed << std::setprecision(2)
                  << (r.alive ? double(r.tape_sum) / r.alive : 0)
                  << "  moves=" << r.history.moves << "  grazed=" << r.history.grazed << "  bitten=" << r.history.bitten
                  << "  stored=" << r.history.stored << "  retrieved=" << r.history.retrieved << "  cache=" << r.caches
                  << "  donated=" << r.history.donated << "  marks=" << r.history.marks << "  gate_flips=" << r.history.gate_changes
                  << "  updates=" << r.history.updates << "  spikes=" << r.history.spikes << "  energy=" << r.reserve << "  plants=" << r.plants
                  << "  fingerprint=" << std::hex << std::setw(8) << std::setfill('0') << r.fingerprint << std::dec << std::setfill(' ');
        if (c.habitat != Habitat::meadow) std::cout << "  attempts=" << r.harvest.attempts << "  opened=" << r.harvest.opened;
        if (c.habitat == Habitat::thicket) std::cout << "  flora=" << flora_names[Word(c.flora)]
            << "  fruit=" << fruit_names[Word(c.fruit)]
            << "  seedings=" << r.propagation.births << "  plant_edits=" << r.propagation.edits
            << "  plant_boundaries=" << r.plant_boundaries << "  response_disagreements=" << r.chemistries;
        if (c.digestion) std::cout << "  digesting=" << r.digesting;
        if (c.climates) {
            std::cout << "  climates=" << c.climates << "  climate_cue=" << c.climate_cue << "  climate_differences=" << r.climate_differences;
            const char *labels[]{"control_late","control_early","treatment_late","treatment_early"};
            for (Word k=0;k<4;++k) std::cout << "  " << labels[k] << "_tries=" << r.climate.group[k].attempts
                                             << "  " << labels[k] << "_opened=" << r.climate.group[k].opened;
        }
        if (c.leach) std::cout << "  minerals_lost=" << r.leached;
        if (c.seep || c.springs) for (Word k=0;k<4;++k) std::cout << "  seep_" << k << '=' << r.seepage.uptake[k];
        if (c.repair || o.forage_profile) for (Word k=0;k<4;++k) std::cout << "  copy_exponent_" << k << '=' << r.fidelity[k];
        if (c.balanced || o.forage_profile || c.repair) {
            std::cout << "  age_sum=" << r.age << "  lived_season=" << r.seasons_lived << "  accessible_grips=" << r.grip_count << "  all_grips=" << r.full_grips;
        }
        if (c.balanced) {
            std::cout << "  carbon=" << r.carbon << "  meals=" << r.diet.tasted << "  depleted=" << r.diet.depleted << "  converted=" << r.diet.converted
                      << "  tastes_without_food=" << r.harvest.attempts-r.harvest.opened;
            for (Word k=0;k<4;++k) std::cout << "  food_" << k << '=' << r.diet.intake[k] << "  pouch_" << k << '=' << r.stocks[k]
                << "  supply_" << k << '=' << r.supply[k] << "  cycle_supply_" << k << '=' << r.cycle_supply[k];
        }
        if (c.contest) std::cout << "  treatment_alive=" << r.cohort << "  control_alive=" << r.alive - r.cohort;
        if (adaptive) std::cout << "  next_schedule=" << (world.packed ? "occupied" : "dense");
        if (seconds > 0) std::cout << std::fixed << std::setprecision(3) << "  GPU_ms=" << seconds*1e3 << "  Gcell_updates/s="
                                     << double(r.history.updates - last.history.updates)*1e-9/seconds;
        std::cout << '\n' << std::flush;
    };
    auto *notebook=journal::current();Count elapsed=0; Word segment=0;
    Count duration=Count(o.steps)*o.passages.value_or(1);
    std::set<Count> frames{0,duration};
    if (notebook) {
        notebook->habitat=habitat_names[Word(c.habitat)];notebook->metabolism=c.balanced==2 ? "food-fuel" : c.balanced ? "balanced" : "energy";
        notebook->device=world.name;notebook->metrics["seed"]=c.seed;notebook->metrics["neurons"]=c.tiles*32;
        notebook->metrics["nursery"]=o.nursery;notebook->metrics["navigation"]=c.navigation;notebook->metrics["basin_width"]=c.basin_width;
        for (Word k=1;k<7;++k) { Count t=duration*k/7;frames.insert(duration>256 ? (t/32)*32 : t); }
        for (Word k=1;k<=7 && k<duration;++k) frames.insert(duration-k);
    }
    auto record=[&](const Census &now,Word families,bool picture) {
        if (!notebook) return;
        journal::Moment m{elapsed,world.tick,segment,now.alive,families,now.deepest,now.history.births,now.history.deaths,
                          now.history.moves,now.history.edits,now.history.grazed,now.history.bitten,now.history.marks};
        notebook->observe(m);
        if (picture) world.photograph(*notebook,m);
    };
    record(last,family_count,true);
    print(last, family_count, 0);
    for (Word passage=0;passage<o.passages.value_or(1);++passage) {
        if (o.colonize) {
            Word key=*o.colonize+passage, plant_seed=o.novel_plants.value_or(true) ? mix(key^0xc76ac189u) : c.seed;
            Word parents=std::min(Word(last.alive),o.settlers.value_or(256));
            std::cout << "passage=" << passage+1 << "  source_tick=" << world.tick << "  source_alive=" << last.alive
                      << "  parent_pool=" << parents << "  dispersal_seed=" << key << "  plant_seed=" << plant_seed;
            if (!parents) { std::cout << "  dispersal_ms=0\n"; break; }
            ++segment;
            world.c.ancestor_depth=last.deepest;
            auto ms=world.colonize(Word(last.alive),parents,key,plant_seed);
            std::cout << "  dispersal_ms=" << std::fixed << std::setprecision(3) << ms << '\n';
            auto fresh=world.inspect(); last=fresh.first; family_count=fresh.second;
            world.consider_density(last.alive,world.sites); print(last,family_count,0);record(last,family_count,false);
        }
        if (Count(world.c.ancestor_depth)+world.tick+o.steps>std::numeric_limits<Word>::max())
            throw std::overflow_error("ecological lineage depth would overflow");
        for (Word todo=o.steps;todo;) {
            Word interval=std::min(todo,o.interval ? o.interval : todo),remaining=interval;double seconds=0;
            Census now{};Word count=0;
            while (remaining) {
                Word n=remaining;
                if (notebook) {
                    auto next=frames.upper_bound(elapsed);
                    if (next!=frames.end()) n=Word(std::min<Count>(n,*next-elapsed));
                }
                seconds+=world.advance(n);elapsed+=n;remaining-=n;
                auto result=world.inspect();now=result.first;count=result.second;
                record(now,count,notebook && frames.contains(elapsed));
            }
            if (last.alive+(now.history.births-last.history.births)!=now.alive+(now.history.deaths-last.history.deaths))
                throw std::runtime_error("ecological population accounting failed");
            world.consider_density(now.alive,world.sites);
            print(now,count,seconds);last=now;family_count=count;todo-=interval;
        }
    }
    if (notebook && (notebook->frames.back().time.passage!=segment || notebook->frames.back().time.tick!=world.tick)) record(last,family_count,true);
    if (notebook) world.rank_population(*notebook);
    if (o.dissect) world.rules();
    if (o.panel.quartets) world.condition(o.panel, Word(last.alive), o.dissect);
    if (o.probes) world.probe_synapses(o.probes);
    if (o.survey) world.survey(o.survey,o.survey_seed,Word(last.alive));
    if (o.growth) { auto cfg=o.value; cfg.sets=o.growth; cfg.inferred=o.paired_food; world.growth(cfg,Word(last.alive),o.value_controls); }
    if (o.pupils) world.screen(o.pupils,Word(last.alive),o.value);
    if (o.passage_pupils) world.screen(o.passage_pupils,Word(last.alive),o.value,&o.passage);
    if (o.passage.sets) world.junctions(o.passage,Word(last.alive),o.value_controls);
    if (o.travel.sets) world.crossings(o.travel,Word(last.alive),o.value_controls);
    if (o.migration) { auto setting=o.travel;setting.sets=o.migration;world.migration(setting,Word(last.alive),o.rooms,o.value_controls,o.vision); }
    if (o.mazes) { auto setting=o.travel;setting.sets=o.mazes;world.mazes(setting,Word(last.alive),o.rooms,o.roam,o.value_controls); }
    if (o.appetites) world.appetites(o.appetites,Word(last.alive),o.value.seed,o.value.items);
    if (o.value.sets) world.valuation(o.value,Word(last.alive),o.value_controls);
    if (o.revisit.sets) world.revisit(o.revisit,Word(last.alive),o.memory_controls);
    if (o.map) world.map();
    if (!o.save.empty()) { Snapshot s; world.state(s, false); write_checked(o.save, s); }
    return 0;
}
} // namespace axion::life
