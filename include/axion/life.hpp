#pragma once
#include "model.hpp"

namespace axion::life {

// Each instruction holds three 3-bit source addresses, an 8-bit truth table,
// a 2-bit destination and a conditional cell-type field. The tape can grow,
// shrink, duplicate instructions and specialize its expression in a lineage.
struct Genome {
    Word op[8], edges[2], development, receptors, motors, body, reproduction, expression, effectors[2];
};
static_assert(sizeof(Genome) == 72);

// Each bit position is one neuron. gate_a and gate_b are experience-dependent
// synapses selecting between inherited recurrent afferents.
struct alignas(16) Tissue { Word spike, memory, gate_a, gate_b; };
struct Body {
    Word slot, founder, generation, born;
    unsigned short energy;
    unsigned char heading, recent;
};
struct Site {
    Body body;
    unsigned char food, scent;
    unsigned short stock;
};
static_assert(sizeof(Site) == 24);

enum Contest : Word { none, ancestry, plasticity, neural_genes, body_genes, agency, dendritic_plasticity, metaplasticity, recurrent_paths, distal_paths, contextual_memory, contextual_evidence, prospective, prospective_plasticity, outcome_association, navigation_signal, navigation_plasticity, navigation_range, temporal_signal, temporal_plasticity, temporal_span };
enum class Habitat : Word { meadow, grove, compound, coupled, thicket };
enum class Flora : Word { evolving, clonal, fixed, autonomous };
enum class Fruit : Word { scattered, persistent, inherited };
enum class Graft : Word { whole, memory, alternative, body, soma, dendrites, pathways, outcomes, fidelity, navigation, temporal };
AX_FN bool paired_habitat(Habitat kind) { return kind >= Habitat::compound; }
struct Settings {
    Word side, tiles, seed, density, mutate, plastic, random_actions, contest, swap;
    Habitat habitat = Habitat::meadow;
    Word digestion = 0; // zero selects the original one-tick contact apparatus
    Word species = 4;
    Word synapse_bits = 1, season = 0;
    Word relations = 0;
    Flora flora = Flora::evolving;
    Fruit fruit = Fruit::scattered;
    Word climates=0, climate_cue=0;
    Word programs=1, polarity=0;
    Word balanced=0, stand=4, seep=0, leach=0, repair=0;
    Word ancestor_depth=0, husk=0, navigation=0, springs=0, basin_width=0;
};
// One 32-contact dendritic assembly per motor pool. The delayed peripheral
// signal records which receptor and motor population fired before the outcome.
struct Contacts { Word weights[8], previous; };
static_assert(sizeof(Contacts) == 36);
using Count = unsigned long long;
struct Ledger {
    Count births, deaths, moves, bitten, donated, marks, edits, growth, spent, grazed;
    Count updates, spikes, gate_changes, structural, stored, retrieved;
};
struct SynapticGenes { Word conditional, modulation; };
struct SynapticLayer { SynapticGenes genes; Word state[8]; };
static_assert(sizeof(SynapticLayer) == 40);
struct DelayLine {
    unsigned short discharge[8];
    unsigned char digestion[8]; // 1..22 units of food, 255 for a bitter outcome
};
static_assert(sizeof(DelayLine) == 24);
struct LinkGenes { Word switches, plastic, wiring; };
struct HeritableLinks { LinkGenes genes; Word convergence; };
AX_FN Word course_switch(Word phase, Word fabric) {
    phase ^= (fabric >> (phase >> 1)) & 1;
    phase ^= ((fabric >> (2 + (phase & 1))) & 1) << 1;
    return phase ^ ((fabric >> (4 + (phase >> 1))) & 1);
}
AX_FN Word route_fibre(Word wiring, Word channel, Word phase) {
    if (wiring & (1u << 24)) return course_switch(phase,wiring >> (6*channel));
    if (channel & 1) phase = course_switch(phase,wiring);
    if (channel & 2) phase = course_switch(phase,wiring >> 6);
    return phase;
}
// Each sensory row carries four 16-bit axon masks. Another 256 plastic
// synapses connect internally recalled sequences to the same motor system.
struct alignas(16) AssociationLayer { LinkGenes genes; Word rows[16][2], recalled[8], convergence; };
static_assert(sizeof(AssociationLayer) == 176);
// Four competing cell groups share inherited programs. Each has four physical
// motor rows, 16 internal motor contacts and 16 ambient receptor contacts.
struct ContextGenes { Word plastic, evidence, dynamics, coordination, receptors; };
struct ContextCell { Word inputs[4], bindings; };
struct alignas(16) ContextLayer {
    ContextGenes genes;
    ContextCell cells[4];
    Word activity, proposals, landmarks;
};
static_assert(sizeof(ContextLayer) == 112);
struct ExpressionGenes { ContextGenes alternative; Word sensors, gate; };
struct alignas(16) ExpressionLayer { ExpressionGenes genes; Word history; };
static_assert(sizeof(ExpressionLayer)==32);
// Sixteen banks of 32 contacts couple an efferent discharge and a receptor
// population to a sensory consequence. A second 4x4 junction receives taste.
struct OutcomeGenes { Word plastic, wiring, valuation, gates; };
struct OutcomeLayer {
    OutcomeGenes genes;
    Word afferents[16], associations, stores;
};
static_assert(sizeof(OutcomeLayer)==88);
struct MapGenes { Word plastic,wiring,spread,currents; };
struct MapLayer {
    MapGenes genes;
    Word contacts[32],appetites[4],previous;
};
static_assert(sizeof(MapLayer)==164);
AX_FN MapLayer *map_tail(OutcomeLayer *organs,Word sites) { return reinterpret_cast<MapLayer *>(organs+sites); }
AX_FN const MapLayer *map_tail(const OutcomeLayer *organs,Word sites) { return reinterpret_cast<const MapLayer *>(organs+sites); }
struct TraceGenes { Word plastic,charge,attention; };
AX_FN Word trace_offset(TraceGenes g,Word place) { return (place*((g.plastic>>27)|1))&31; }
struct TraceLayer {
    TraceGenes genes;
    Word predictions[32];
    unsigned char sources[32];
    Word recent,held,conflicts;
};
static_assert(sizeof(TraceLayer)==184);
AX_FN unsigned map_bytes(Word mode) {
    return mode ? sizeof(MapLayer)+(mode>=4 ? sizeof(TraceLayer) : 0) : 0;
}
AX_FN TraceLayer *trace_tail(OutcomeLayer *organs,Word sites) { return reinterpret_cast<TraceLayer *>(map_tail(organs,sites)+sites); }
AX_FN const TraceLayer *trace_tail(const OutcomeLayer *organs,Word sites) { return reinterpret_cast<const TraceLayer *>(map_tail(organs,sites)+sites); }
// Only material expressed in the germline crosses a habitat boundary.
struct Propagule {
    Genome genome;
    SynapticGenes dendrites;
    HeritableLinks paths;
    ContextGenes context;
    ExpressionGenes expression;
    OutcomeGenes outcomes;
    MapGenes map;
    TraceGenes trace;
    Word ancestor, generation;
};
static_assert(sizeof(Propagule)==196);
AX_FN Word tasted(Word associations) { return (associations>>16)&7; }
AX_FN Word absorbed(Word associations) { return (associations>>19)&7; }
AX_FN Word receipt(Word associations,Word flavour,Word nutrient) {
    return (associations&0xff00ffffu)|(flavour<<16)|(nutrient<<19);
}
AX_FN Word food_spike(Word code) { return code ? 1u<<(code-1) : 0; }
// Each material pouch holds at most 47 units. The top two bits record time
// still needed to handle the last mouthful, without another byte of state.
AX_FN Word contents(Word stores) { return stores&0x3fffffffu; }
AX_FN Word chewing(Word stores) { return stores>>30; }
AX_FN Word appetite(Word stores,Word body) {
    stores=contents(stores);
    Word threshold=1+((body>>24)&15), demand=0;
    for (Word k=0;k<4;++k) demand|=Word(((stores>>(8*k))&255)<threshold)<<k;
    return demand;
}
AX_FN Word food_field(Word ambient,Word needs,Word event) {
    return (ambient&0x000fffffu)|(needs<<20)|(food_spike(tasted(event))<<24)|(food_spike(absorbed(event))<<28);
}
struct SeepLedger { Count uptake[4]; };
struct DietLedger { Count intake[4], tasted, depleted, converted; };
AX_FN ContextLayer *context_tail(AssociationLayer *links, Word size) {
    return reinterpret_cast<ContextLayer *>(links+size);
}
AX_FN const ContextLayer *context_tail(const AssociationLayer *links, Word size) {
    return reinterpret_cast<const ContextLayer *>(links+size);
}
AX_FN ExpressionLayer *expression_tail(AssociationLayer *links,Word size) {
    return reinterpret_cast<ExpressionLayer *>(context_tail(links,size)+size);
}
AX_FN const ExpressionLayer *expression_tail(const AssociationLayer *links,Word size) {
    return reinterpret_cast<const ExpressionLayer *>(context_tail(links,size)+size);
}
struct alignas(16) Plant { Word shape, texture, reaction, seed; };
struct SeedLedger { Count births, edits; };
struct Chemistry { Word low, high; };
struct Harvest { Count attempts, opened; };
struct ClimateLedger { Harvest group[4]; };
AX_FN Word climate_shards(Word sites) { return (sites+127)/128; }
AX_FN Word *food_chemistry(Plant *plants,Word sites,Word climates) {
    return reinterpret_cast<Word *>(plants+sites)+(climates ? climates-1 : 0)*sites;
}
AX_FN DietLedger *diet_ledger(Harvest *harvest,Word sites,Word climates) {
    auto *weather=reinterpret_cast<ClimateLedger *>(harvest+sites/4);
    return reinterpret_cast<DietLedger *>(weather+(climates ? climate_shards(sites) : 0));
}
enum class JointLaw : Word { ring, parity, private_code, changing, kinds, independent, transposed, mosaic, latent_table, chains, islands, parity_chains };
struct Conditioning {
    Word quartets, varieties, exposures, seed;
    bool dimensions = false;
    Word delay = 1;
    bool composition = false;
    JointLaw law = JointLaw::ring;
    Word fitness_offset = 0;
};
AX_FN Word panel_variants(Conditioning c) {
    return c.dimensions && (!c.composition || c.law == JointLaw::private_code ||
           c.law == JointLaw::changing || c.law == JointLaw::mosaic) ? 8u : 4u;
}
AX_FN bool chain_law(JointLaw law) { return law == JointLaw::chains || law == JointLaw::parity_chains; }
AX_FN Word practice_width(Conditioning c) {
    return chain_law(c.law) || c.law == JointLaw::islands ? 2 : c.varieties/2;
}
AX_FN Word probe_width(Conditioning c) {
    if (!c.dimensions) return 1;
    return c.law == JointLaw::islands ? c.varieties/2 : c.varieties-practice_width(c);
}
struct Aptitude { Word score[12]; };
enum class NurseryCourse : Word { paired, varied, recall, lifelong, repertoire, integrated, outcomes, subsistence, navigation, migration, junctions };
struct Valuation {
    Word sets=0, items=16, rounds=16, pairings=4, seed=924677, inferred=1, fitness_offset=0;
};
enum class ValueArm : Word { intact, silent, fixed, unpaired, no_chain, wrong_need, erased, no_taste, wrong_object };
template<class S> struct ValueCounts {
    S naive, explored, learned, surprise, changed, old, sensitive, actions, trials, tasted, reachable, complete;
};
using ValueScore=ValueCounts<Count>;
struct PupilSite { Word site,slot,founder,generation; Count genotype; };
struct AppetiteScore { Count correct, possible, responsive, sampled, age, stocked,away,moving,closer,open,passable; };
struct Travel { Word sets=0,depth=4,rounds=8,seed=173801,offset=0,motivation=0,change_food=0,history=0,foot=0; };
struct TravelScore { Count naive,first,surprise,changed,old,steps,arrived,redirected,tried,observed,seen_answers,seen_items; };
struct Maze {
    Word original[32],changed[32],junctions;
    unsigned char distances[2][32][32];
};
struct MazeScore {
    Count questions,first,near_items,near_first,far_items,far_first,changed_items,ready,surprise,turned,obsolete;
    Count journeys,reached,shortest,steps,length,seen,distinct,coverage;
};
struct SpringChoice { Count items,correct,far_items,far_correct,familiar_items,familiar_correct; };
struct GrowthScore { Count alive, ticks, births, offspring, absorbed, seep, converted, terminal, visited; };
enum class AppetiteArm : Word { resident, silent, reset, innate, wrong_need, remote, map_silent, map_reset, one_pulse, no_appetite, raw, immediate, no_hold, trace_reset };
enum class RevisitTask : Word { arbitrary, frames, paired, exceptions, interleaved };
enum class RevisitArm : Word { intact, pooled, silent, fixed, ambiguous, erased, blind, no_binding, wrong_cue, no_reinstatement, fixed_confidence, labile, external_only, assigned, primary_only, alternate_only, swapped, paired_hint };
struct Revisit {
    Word sets=0, items=8, practice=16, regimes=2, seed=483971, marked=0, delay=1;
    RevisitTask task=RevisitTask::arbitrary;
    Word fitness_offset=6;
};
AX_FN Word recall_items(Revisit cfg) {
    if (cfg.task==RevisitTask::paired) return cfg.items*cfg.items/2;
    if (cfg.task==RevisitTask::interleaved) return cfg.items-2+cfg.items*cfg.items/2;
    return cfg.items-(cfg.task==RevisitTask::arbitrary ? 2 : 4);
}
constexpr Word recall_stages=6;
AX_FN Word recall_dose(Word stage) { return stage ? 1u<<(stage-1) : 0; }
template<class Size> struct RecallStage { Size correct, returning, identified, identified_correct, oracle_units, by_regime[4], exceptions, transfer; };
template<class Size> struct RecallCounts {
    Size acquired, trained, positive, used, reach, alternate_trials, acquired_in[4], acquired_subset[4], base_first, base_final;
    Size anchor_items, anchor_correct, base_anchor_first, base_anchor_final;
    RecallStage<Size> stage[recall_stages];
};
using RevisitScore=RecallCounts<Count>;
struct ConditioningScore {
    Count naive, learned, practiced, erased, surprise, one, recovered, retained, attempts, successes, delivered, changed_items, reachable, readout_cut, connected;
    Count discordant_items, discordant_correct, alternate_correct;
    Count distant_items, distant_correct;
    Count remote_items, remote_correct, local_readout, remote_local;
};
struct Assay { Word trials, gates, spikes, motors, effectors; };
struct BehavioralSample { Word response[2][4], site; };
struct Census {
    Ledger history;
    Count alive, reserve, plants, founders_energy, depth_sum, cohort, tape_sum, caches;
    Word deepest, fingerprint;
    Harvest harvest;
    Count digesting;
    SeedLedger propagation;
    Count plant_boundaries, chemistries;
    ClimateLedger climate;
    Count climate_differences;
    DietLedger diet;
    SeepLedger seepage;
    Count leached;
    Count stocks[4], carbon, age, seasons_lived, grip_count, full_grips;
    Count supply[4], cycle_supply[4], fidelity[4];
};
constexpr Word microticks = 4;
AX_FN Word rotate(Word x, Word shift) {
    shift &= 31;
    return (x << shift) | (x >> ((0u - shift) & 31));
}
AX_FN Tissue initial_tissue(Word identity) { return {identity,rotate(identity,11),mix(identity+1),mix(identity+2)}; }

AX_FN Word cell_at(Word x, Word y, Word side) { return (x & (side - 1)) + (y & (side - 1)) * side; }
AX_FN Word neighbor(Word i, Word direction, Word side) {
    Word x = i & (side - 1), y = i / side;
    return cell_at(x + (direction == 1) - (direction == 3), y + (direction == 2) - (direction == 0), side);
}
AX_FN Word living_tiles(Word cap, Word developmental) {
    Word extent = 2u << (developmental >> 30);
    if (developmental & (1u << 29)) extent *= 2;
    return extent < cap ? extent : cap;
}
AX_FN Word earlier_cue(Word delayed) { return (delayed&31)|((delayed>>4)&~31u); }
AX_FN Word pattern(Word cue, Word mask=31) {
    Word first=1u<<(cue&mask);
    return cue&1024 ? first|(1u<<((cue>>5)&mask)) : first;
}
// A label for reciprocal common-garden experiments. Creatures cannot sense it.
AX_FN bool treatment(Word founder, Settings c) {
    return ((mix(founder ^ c.seed ^ 0x4e91a57bu) & 1) ^ c.swap) != 0;
}
AX_FN Word initial_food(Word site, Word seed) { return 24 + (mix(site ^ seed ^ 0xec10785du) & 31); }
AX_FN Word founding_energy(Word site, Word seed, Word density) {
    Word draw = mix(site ^ seed ^ 0xd3ba721fu);
    return draw % 100 < density ? 40 + ((draw >> 8) & 15) : 0;
}
#if !defined(__CUDA__)
int command(int argc, char **argv);
#endif
} // namespace axion::life
