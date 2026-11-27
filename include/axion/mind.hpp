#pragma once
#include "model.hpp"

namespace axion::mind {

using Byte = unsigned char;
using Half = unsigned short;
using Count = unsigned long long;

// ---------------------------------------------------------------------------
// Universal Cortical Currency: 1024-neuron Sparse Distributed Spike Vector
// 32 lanes x 32 bits = 1024 spiking pyramidal cells per cortical area.
// One warp holds an entire cortical area's spike pattern in 1 register/lane.
// ---------------------------------------------------------------------------
constexpr Word NEURONS = 1024;
constexpr Word WORDS = 32;
constexpr Word TARGET_K = 32;

struct alignas(16) SpikeVec {
    Word bits[WORDS]{};
};
static_assert(sizeof(SpikeVec) == 128);

// Thematic role compartments (Theta-Gamma nested oscillation slots)
enum Role : Word {
    R_SUBJ = 0,  // Subject / Agent / Entity
    R_REL = 1,   // Relation / Predicate / Action / Verb
    R_OBJ = 2,   // Object / Patient / Property / Value
    R_LOC = 3,   // Location / Container / Destination / Context
    R_CAUSE = 4, // Cause / Condition / Instrument / Source
    R_PERSP = 5, // Observer / Belief Perspective (SELF, PUBLIC, MIRA, FOX...)
    NUM_ROLES = 6
};

// Engram biological state & consolidation flags
constexpr Word ENG_ACTIVE = 1u << 0;
constexpr Word ENG_SUPERSEDED = 1u << 1;   // Archived autobiographical history ("was / earlier")
constexpr Word ENG_NEGATED = 1u << 2;      // Inhibitory dendritic veto ("NOT / CANNOT")
constexpr Word ENG_UNIVERSAL = 1u << 3;    // Consolidated general rule ("ALL X are/can Y")
constexpr Word ENG_HYPOTHETICAL = 1u << 4; // Counterfactual sandbox branch ("suppose...")
constexpr Word ENG_CONSOLIDATED = 1u << 5; // Replayed to neocortex during sleep
constexpr Word ENG_META_PROP = 1u << 6;    // Learned relational meta-property
constexpr Word ENG_EPISODIC = 1u << 7;     // Time-stamped autobiographical event
constexpr Word ENG_INFERRED = 1u << 8;     // Derived via multi-hop cortical attractor
constexpr Word ENG_IMPLIES = 1u << 9;      // Conditional implication rule: (SUBJ,REL,OBJ) -> (LOC,CAUSE,PERSP_or_extra)
constexpr Word ENG_DISJUNCTION = 1u << 10; // Disjunctive hypothesis: (SUBJ,REL,OBJ) OR (SUBJ,REL,LOC)

// Source-monitoring provenance (anterior PFC metacognition)
enum Provenance : Word {
    PROV_NONE = 0,
    PROV_OBSERVED = 1,
    PROV_TOLD_USER = 2,
    PROV_TOLD_PEER = 3,
    PROV_EXPERIMENT = 4,
    PROV_INFERRED = 5,
    PROV_CONSOLIDATED = 6,
    PROV_IMAGINED = 7
};

constexpr int QTY_NONE = -2147483647;

// ---------------------------------------------------------------------------
// Evolvable Cortical Genome: ZERO hardcoded learning rules.
// ---------------------------------------------------------------------------
struct alignas(16) CorticalGenome {
    Word basal_rule = 0;
    Word apical_rule = 0;
    Word hipp_rule = 0;
    Word causal_rule = 0;
    Word biophysics = 0;
    Word neuromod = 0;
    Word executive = 0;
    Word wiring[5]{};
};
static_assert(sizeof(CorticalGenome) == 48);

AX_FN Word leak_shift(const CorticalGenome &g) { return 1u + (g.biophysics & 3u); }
AX_FN Word adapt_step(const CorticalGenome &g) { return 2u + ((g.biophysics >> 4) & 15u); }
AX_FN Word bac_gain(const CorticalGenome &g) { return 1u + ((g.biophysics >> 8) & 15u); }
AX_FN Word kwta_target(const CorticalGenome &g) { return 16u + ((g.biophysics >> 12) & 31u); }
AX_FN Word gamma_steps(const CorticalGenome &g) { return 2u + ((g.biophysics >> 18) & 7u); }
AX_FN Word basal_threshold(const CorticalGenome &g) { return 4u + ((g.biophysics >> 21) & 15u); }
AX_FN Word apical_threshold(const CorticalGenome &g) { return 4u + ((g.biophysics >> 26) & 15u); }

AX_FN Word surprise_gain(const CorticalGenome &g) { return 1u + (g.neuromod & 31u); }
AX_FN Word novelty_gain(const CorticalGenome &g) { return 1u + ((g.neuromod >> 6) & 31u); }
AX_FN Word reward_gain(const CorticalGenome &g) { return 1u + ((g.neuromod >> 12) & 31u); }
AX_FN Word curiosity_weight(const CorticalGenome &g) { return (g.neuromod >> 18) & 63u; }
AX_FN Word confidence_margin(const CorticalGenome &g) { return 1u + ((g.neuromod >> 24) & 31u); }

AX_FN Word max_reasoning_hops(const CorticalGenome &g) { return 1u + (g.executive & 7u); }
AX_FN Word max_plan_depth(const CorticalGenome &g) { return 1u + ((g.executive >> 4) & 7u); }
AX_FN Word sleep_replay_cycles(const CorticalGenome &g) { return (g.executive >> 8) & 15u; }
AX_FN Word revision_policy(const CorticalGenome &g) { return (g.executive >> 13) & 15u; }
AX_FN Word mutation_prob(const CorticalGenome &g) { return 4u + ((g.executive >> 17) & 63u); }
AX_FN Word hipp_threshold(const CorticalGenome &g) { return 6u + ((g.executive >> 23) & 31u); }

AX_FN int plasticity_delta(Word lut, bool pre, bool post, bool pred, bool mod_pos) {
    Word idx = Word(pre) | (Word(post) << 1) | (Word(pred) << 2) | (Word(mod_pos) << 3);
    Word code = (lut >> (2u * idx)) & 3u;
    return code == 0 ? 0 : (code == 1 ? 1 : (code == 2 ? -1 : 2));
}

// ---------------------------------------------------------------------------
// Multi-Compartment Cortical Area State (1024 neurons)
// ---------------------------------------------------------------------------
struct alignas(16) CorticalArea {
    SpikeVec spikes{};
    SpikeVec predicted{};
    short voltage[NEURONS]{};
    Byte adapt[NEURONS]{};
    Byte excitability[NEURONS]{};
    Word total_spikes = 0;
    Word last_surprise = 0;
    Word last_omission = 0;
    Word tick = 0;
};

// ---------------------------------------------------------------------------
// Theta-Gamma Hippocampal Engram (CA3/CA1 Multi-Compartment Spiking Trace)
// ---------------------------------------------------------------------------
struct alignas(16) Engram {
    SpikeVec role[NUM_ROLES]{};
    Word timestamp = 0;
    Word flags = 0;
    int quantity = QTY_NONE;
    Word provenance = PROV_NONE;
    Word source_concept = 0;
    Word parent_a = 0;
    Word parent_b = 0;
    Half strength = 0;
    Half prev_ep = 0;
    Half next_ep = 0;
    Half recency_boost = 0;
    Word pad[2]{};
};
static_assert(sizeof(Engram) == 816);

struct alignas(16) HippQuery {
    SpikeVec cue[NUM_ROLES]{};
    Word active_roles = 0;
    Word require_flags = 0;
    Word forbid_flags = 0;
    Word prefer_latest = 1;
    Word target_role = R_OBJ;
    Word min_overlap = 14;
    Word exclude_engram = ~0u;
    Word min_time = 0;
    Word max_time = ~0u;
};

struct alignas(16) HippResult {
    Word best_index = ~0u;
    Word second_index = ~0u;
    int best_score = 0;
    int second_score = 0;
    Word match_count = 0;
    Word conflict_count = 0;
    Word negated_index = ~0u;
    int negated_score = 0;
    Word spikes_fired = 0;
    SpikeVec readout{};
};

// Multi-block reduction scratchpad for 26-SM Blackwell parallel Hippocampus!
constexpr Word HIPP_BLOCKS = 32;
struct alignas(16) HippBlockPartial {
    int best_score;
    Word best_idx;
    int second_score;
    Word second_idx;
    int neg_score;
    Word neg_idx;
    Word matches;
    Word conflicts;
    Word spikes;
    Word pad[3];
};

// ---------------------------------------------------------------------------
// Native Blackwell 1024x1024 Neocortical Associative Matrix (28 x 1 MiB in L2!)
// Combined with 3.2-MiB Hippocampus = 31,936 / 32,768 KiB (97.5% of L2 Cache!)
// ---------------------------------------------------------------------------
constexpr Word NEO_SYNAPSES = NEURONS * NEURONS; // 1,048,576 plastic synapses/pathway
constexpr Word NEO_PATHWAYS = 28; // 0..19 + 20:Abil->Exemp, 21:NegAbil->Exemp, 22:TopicGraph, 23:Goal->Tool, 24..27:ContinualPlasticity
constexpr Word MAX_THINK_HOPS = 4;
constexpr Word MAX_WM_SLOTS = 64;
constexpr Word MAX_ANALOGY_PWS = 4;
constexpr Word MAX_MULTI_CUES = 4;
constexpr Word MAX_HYP_CLUES = 4;
constexpr Word MAX_HYP_CANDS = 8;
constexpr Word MAX_BRIDGE_PWS = 4;
constexpr Word MAX_CLUSTER_ITEMS = 6;

constexpr Word MAX_SCAN_MEMBERS = 8;
constexpr Word MAX_DEDUCE_STEPS = 4;

struct alignas(16) NeoCounterfactQuery {
    SpikeVec subj_sdr{};
    SpikeVec rem_sdr{};
    SpikeVec add_sdr{};
    Word pw_tax = 8;
    Word pw_feat = 1;
    Word pw_neg = 2;
    Word pw_proto = 0;
    Word has_rem = 0;
    Word has_add = 0;
    Word top_k = 32;
    int min_curr = 4;
};

struct alignas(16) NeoPlanChainQuery {
    SpikeVec goal_sdr{};
    SpikeVec have_sdr{};
    SpikeVec blocked_sdr{};
    Word pw_inv = 23;
    Word has_have = 0;
    Word has_blocked = 0;
    Word top_k = 32;
    int min_curr = 4;
    Word pad[3]{};
};

struct alignas(16) NeoPlanChainResult {
    SpikeVec step1_spikes{};
    SpikeVec step2_spikes{};
    Word step1_count = 0;
    Word step2_count = 0;
    Word steps_needed = 0;
    Word already_ready = 0;
    int step1_max = 0;
    int step2_max = 0;
    Word spikes_fired = 0;
    Word pad = 0;
};

struct alignas(16) NeoSimulateQuery {
    SpikeVec init_sdr{};
    SpikeVec action1_sdr{};
    SpikeVec action2_sdr{};
    SpikeVec blocked_sdr{};
    Word pw_act = 7;
    Word n_actions = 1;
    Word has_blocked = 0;
    Word top_k = 32;
    int min_curr = 4;
    Word pad[3]{};
};

struct alignas(16) NeoSimulateResult {
    SpikeVec step1_spikes{};
    SpikeVec step2_spikes{};
    Word step1_count = 0;
    Word step2_count = 0;
    Word steps_completed = 0;
    Word blocked = 0;
    Word blocked_step = 0;
    int step1_max = 0;
    int step2_max = 0;
    Word spikes_fired = 0;
};

constexpr Word MAX_CONSTRAIN_CANDS = 8;

struct alignas(16) NeoConstrainQuery {
    SpikeVec cands[MAX_CONSTRAIN_CANDS]{};
    SpikeVec req_cat{};
    SpikeVec req_pos{};
    SpikeVec excl_sdr{};
    Word n_cands = 0;
    Word has_cat = 0;
    Word has_pos = 0;
    Word has_excl = 0;
    Word pw_tax = 17;
    Word pw_pos = 1;
    Word pw_neg = 2;
    Word pad = 0;
};

struct alignas(16) NeoConstrainResult {
    Word valid_mask = 0;
    Word n_valid = 0;
    Word best_cand = 0;
    int best_score = 0;
    Word eliminated = 0;
    Word spikes_fired = 0;
    int scores[MAX_CONSTRAIN_CANDS]{};
    Word pad[2]{};
};

constexpr Word MAX_INDUCE_PWS = 6;

struct alignas(16) NeoInduceQuery {
    SpikeVec ex1_in{};
    SpikeVec ex1_out{};
    SpikeVec ex2_in{};
    SpikeVec ex2_out{};
    SpikeVec query_in{};
    Word cand_pws[MAX_INDUCE_PWS]{4, 8, 16, 18, 23, 14};
    Word n_pws = 6;
    Word n_hops = 1;
    Word top_k = 32;
    int min_curr = 4;
    Word pad[2]{};
};

struct alignas(16) NeoInduceResult {
    SpikeVec step1_spikes{};
    SpikeVec step2_spikes{};
    Word found = 0;
    Word best_pw = 0;
    Word hops_done = 0;
    Word step1_count = 0;
    Word step2_count = 0;
    int ex1_score = 0;
    int ex2_score = 0;
    Word spikes_fired = 0;
};

constexpr Word MAX_CHAINRULE_PWS = 6;

struct alignas(16) NeoChainRuleQuery {
    SpikeVec start_sdr{};
    SpikeVec target_sdr{};
    Word cand_pws[MAX_CHAINRULE_PWS]{16, 10, 9, 18, 23, 8};
    Word n_pws = 6;
    Word mode = 0; // 0 = execute composition (pw1 -> pw2), 1 = discover (best_pw1, best_pw2) bridging start -> target
    Word pw1 = 16;
    Word pw2 = 10;
    Word top_k = 32;
    int min_curr = 4;
    Word pad[2]{};
};

struct alignas(16) NeoChainRuleResult {
    SpikeVec mid_spikes{};
    SpikeVec end_spikes{};
    Word found = 0;
    Word best_pw1 = 0;
    Word best_pw2 = 0;
    Word mid_count = 0;
    Word end_count = 0;
    int match_score = 0;
    Word spikes_fired = 0;
    Word pad = 0;
};

constexpr Word MAX_ONLINE_HOPS = 3;

struct alignas(16) NeoOnlineSchemaQuery {
    SpikeVec pre_sdr{};
    SpikeVec post_sdr{};
    SpikeVec post2_sdr{};
    SpikeVec block_sdr{};
    Word pathway = 19;
    Word mode = 0;        // 0 = 1-shot Hebbian write, 1 = 1/2/3-hop rollout, 2 = atomic write (A->B->C) + 2-hop rollout!
    Word max_hops = 2;
    Word strength = 4;
    Word has_blocker = 0;
    Word top_k = 32;
    int min_curr = 4;
    Word pad = 0;
};

struct alignas(16) NeoOnlineSchemaResult {
    SpikeVec hop_spikes[MAX_ONLINE_HOPS]{};
    Word hop_counts[MAX_ONLINE_HOPS]{};
    Word hops_done = 0;
    Word blocked_hit = 0;
    Word edits_made = 0;
    Word spikes_fired = 0;
};

struct alignas(16) NeoReviseSchemaQuery {
    SpikeVec pre_sdr{};
    SpikeVec old_sdr{};
    SpikeVec new_sdr{};
    SpikeVec new2_sdr{};
    Word pathway = 19;
    Word mode = 0;        // 0 = LTD unlearn old + LTP write pre->new + 2-hop verify; 1 = also LTP write new->new2 + 2-hop verify!
    Word has_old = 0;     // 0 = spontaneously recall superseded old target from W via NMDA before LTD; 1 = use explicit old_sdr
    Word strength = 6;
    Word top_k = 32;
    int min_curr = 4;
    Word pad[2]{};
};

struct alignas(16) NeoReviseSchemaResult {
    SpikeVec old_read_spikes{};
    SpikeVec hop1_spikes{};
    SpikeVec hop2_spikes{};
    Word old_count = 0;
    Word hop1_count = 0;
    Word hop2_count = 0;
    Word ltd_edits = 0;
    Word ltp_edits = 0;
    Word spikes_fired = 0;
    Word pad[2]{};
};

struct alignas(16) NeoRoleBindQuery {
    SpikeVec roles[4]{};   // Mode 0 (BIND): [0]=AGENT, [1]=ACTION, [2]=OBJ, [3]=LOC
    SpikeVec cue_a{};      // Mode 1 (UNBIND): primary cue SDR
    SpikeVec cue_b{};      // Mode 1 (UNBIND): optional secondary cue SDR
    Word pathway = 25;     // L2 pathway 25 (zero extra L2 memory!)
    Word mode = 0;         // 0 = 1-shot phase-shifted 4-role bind via evolved `g.basal_rule`, 1 = phase-shifted role unbind readout!
    Word query_role = 0;   // Target role to unbind in Mode 1 (0=AGENT, 1=ACTION, 2=OBJ, 3=LOC)
    Word strength = 6;
    Word top_k = 32;
    int min_curr = 4;
    Word pad[2]{};
};

struct alignas(16) NeoRoleBindResult {
    SpikeVec unbound_sdr{};
    Word active_count = 0;
    int max_current = 0;
    Word edits_made = 0;
    Word spikes_fired = 0;
};

constexpr Word MAX_SEQ_LEN = 6;

// Kernel 27 (Stage 5 Crossing!): Pure-GPU Spiking Predictive-Coding Sequence Role-Tagger & Grammar Circuit (`PW12`, zero extra L2!)
struct alignas(16) NeoSeqPredictQuery {
    SpikeVec pos_start_sdrs[MAX_SEQ_LEN]{}; // Orthogonal position-from-start SDRs (0..5)
    SpikeVec pos_end_sdrs[MAX_SEQ_LEN]{};   // Orthogonal position-from-end SDRs (0..5)
    SpikeVec role_sdrs[4]{};                // Orthogonal thematic role SDRs: 0=AGENT, 1=ACTION, 2=PATIENT, 3=LOCATION
    SpikeVec prev_bos_sdr{};                // Beginning-of-sequence context SDR
    SpikeVec adj_rel_sdr{};                 // Spatial-relator adjacency context SDR
    Word target_roles[MAX_SEQ_LEN]{};       // Mode 0 (TRAIN): observed dialect thematic role at each position (0..3)
    Word adj_rel_mask = 0;                  // Bitmask of positions adjacent to a learned spatial relator
    Word seq_len = 4;                       // Number of content tokens (1..6)
    Word pathway = 12;                      // L2 PW12 (Narrative Story-Role & Predictive Syntax Cortex!)
    Word mode = 0;                          // 0 = STDP grammar train (`g.basal_rule`), 1 = recurrent spiking role-tagger & order rollout
    Word strength = 6;
    Word pad = 0;
};

struct alignas(16) NeoSeqPredictResult {
    Word pred_roles[MAX_SEQ_LEN]{};         // Predicted thematic role at each position (0..3)
    int  pred_scores[MAX_SEQ_LEN]{};        // Winning role synaptic current at each position
    Word unique_count = 0;                  // Number of distinct roles confidently tagged
    Word edits_made = 0;                    // Mode 0: live STDP edits written into L2 PW12
    Word spikes_fired = 0;
    Word pad = 0;
};

// Kernel 28 (Stage 5b — Neural-Native GPU Math!): Pure-GPU Spiking Parietal Number-Line Bump Attractor on L2 PW15 (ZERO extra L2!)
struct alignas(16) NeoNeuralMathQuery {
    Word pathway = 15;   // L2 PW15: 64 states x 16 neurons = 1,024 parietal number-line cells!
    Word mode = 0;       // 0 = STDP consolidate successor ring `v -> (v+1)&63` via evolved `g.basal_rule`, 1 = spiking rollout!
    Word op = 0;         // Mode 1: 0 = a + b, 1 = a - b, 2 = (a + b) mod c, 3 = solve a*x + b = c, 4 = a * b
    Word a = 0;
    Word b = 0;
    Word c = 0;
    Word strength = 6;
    Word pad = 0;
};

struct alignas(16) NeoNeuralMathResult {
    SpikeVec answer_spikes{}; // Winning 16-neuron parietal bump SDR on Blackwell GPU!
    Word answer_val = 0;      // Decoded bump index (0..63) directly from GPU parietal spike population!
    Word active_count = 0;    // Neurons firing in winning bump (16)
    Word wave_steps = 0;      // Total recurrent spiking bump steps executed on GPU!
    Word edits_made = 0;      // Mode 0: live evolved-LUT STDP edits written into L2 PW15
    Word spikes_fired = 0;
    Word valid = 0;
    Word pad[2]{};
};

// Kernel 29 (Stage 5c — Pure-GPU Spiking Graph Wavefront!): Axonal-Delay Shortest-Path & Reachability on L2 PW11 (ZERO extra L2!)
constexpr Word MAX_GW_NODES = 32;
constexpr Word MAX_GW_EDGES = 16;

struct alignas(16) NeoGraphWaveQuery {
    Word pathway = 11;              // L2 PW11: 32 nodes x 32 neurons = 1,024 entorhinal graph cells!
    Word mode = 0;                  // 0 = evolved-LUT STDP weighted edge write, 1 = axonal-delay spiking wavefront Dijkstra!
    Word clear_first = 0;           // Mode 0: 1 = LTD-clear PW11 before writing edges
    Word n_edges = 0;
    Word edge_u[MAX_GW_EDGES]{};
    Word edge_v[MAX_GW_EDGES]{};
    Word edge_w[MAX_GW_EDGES]{};    // Positive edge latency cost (1..15) encoded inversely in synaptic efficacy!
    Word src_node = 0;
    Word dst_node = 0;
    Word blocked_node = ~0u;        // Optional GABA_A inhibitory node veto (0..31, or ~0u for none)
    Word pad = 0;
};

struct alignas(16) NeoGraphWaveResult {
    SpikeVec dest_spikes{};         // 32-neuron firing SDR of reached destination node!
    Word found = 0;
    Word total_cost = 0;            // Exact axonal-delay arrival tick = globally optimal weighted shortest-path distance!
    Word hop_count = 0;
    Word next_hop = ~0u;            // First hop after `src_node` along the optimal spiking trajectory!
    Word wave_ticks = 0;
    Word edits_made = 0;            // Mode 0: live evolved-LUT STDP synaptic edits written into L2 PW11
    Word spikes_fired = 0;
    Word pad = 0;
};

// ---------------------------------------------------------------------------
// Kernel 30 (Stage 5d!): Pure-GPU Spiking Boltzmann/Hopfield 3-SAT & Constraint Attractor (`mind_neo_sat_solve`)!
// Reuses unused L2 Pathway 13 (`PW13`) — ZERO extra L2 memory, ZERO atomics, ZERO host SAT backtracking!
// 16 variables x 2 polarities x 16 neurons = 512 literal neurons (0..511); 32 clauses x 16 neurons = 512 clause-alarm neurons (512..1023).
// Mode 0: Imprints CNF clauses (1/2/3 literals) into L2 PW13 purely via evolved `g.basal_rule` STDP (excitatory alarm -> lit, GABA_A lit -> alarm).
// Mode 1: Runs 8 parallel 32-lane spiking Boltzmann attractor warps directly on L2 PW13 synapses to find a zero-alarm satisfying state!
// Mode 2: Simultaneously pins `query_var = 1` (Warps 0..3) & `query_var = 0` (Warps 4..7) to prove forced backbone truth (`1=TRUE, 2=FALSE, 3=CONTINGENT, 0=UNSAT`)!
// ---------------------------------------------------------------------------
constexpr Word MAX_SAT_VARS = 16;
constexpr Word MAX_SAT_CLAUSES = 32;
constexpr Word MAX_SAT_WRITE = 16;

struct alignas(16) NeoSatSolveQuery {
    Word pathway = 13;              // Unused L2 Pathway 13 — ZERO extra L2 memory!
    Word mode = 1;                  // 0 = STDP clause imprint, 1 = SAT solve, 2 = forced backbone proof for `query_var`
    Word clear_first = 0;           // Mode 0: wipe L2 PW13 before writing clauses
    Word n_vars = 4;                // Active Boolean variables (1..16)
    Word clause_start = 0;          // Mode 0: starting clause slot index (0..31)
    Word n_clauses = 0;             // Mode 0: number of clauses to imprint (0..16)
    Word query_var = 0;             // Mode 2: variable index (0..15) to prove forced TRUE/FALSE/CONTINGENT
    Word seed = 1337;               // Deterministic thermal noise seed for spiking Boltzmann replicas
    Word lit0[MAX_SAT_WRITE]{};     // Encoded literal: `var * 2 + polarity` (1=pos, 0=neg), `~0u` = unused
    Word lit1[MAX_SAT_WRITE]{};
    Word lit2[MAX_SAT_WRITE]{};
};

struct alignas(16) NeoSatSolveResult {
    SpikeVec winner_spikes{};       // Firing literal assemblies of the satisfying spiking attractor state!
    Word satisfiable = 0;           // 1 if zero-alarm equilibrium reached, 0 if UNSAT
    Word assignment_mask = 0;       // Bit `v` = satisfying truth assignment for variable `v`
    Word forced_state = 0;          // Mode 2: 0=UNSAT, 1=MUST_TRUE, 2=MUST_FALSE, 3=CONTINGENT
    Word unsat_clauses = 0;         // Minimum remaining alarm clauses across all 8 spiking replicas
    Word steps_taken = 0;           // Recurrent spiking ticks to zero-alarm convergence
    Word active_clauses = 0;        // Clauses decoded straight from L2 PW13 plastic synapses
    Word edits_made = 0;            // Mode 0: live evolved-LUT STDP synaptic edits written into L2 PW13
    Word spikes_fired = 0;
};

// ---------------------------------------------------------------------------
// Kernel 31 (STAGE 6 CROSSING!): Pure-GPU 16x16 Spiking V1/V4 Retinal Vision
// & Spatial Scene Cortex (`mind_neo_visual_cortex`) on L2 Pathway 24 (`PW24`)!
// ZERO extra L2 memory, ZERO atomics, ZERO host flood-fill or shape rules!
//   - Input: raw 16x16 binary retinal photoreceptor grid (`Word pixels[8]` = 256 photoreceptors).
//   - Presynaptic V1 neurons `0..255`: 16 translation-invariant V1/V2/V4 oriented-edge, hole,
//     fill, corner & symmetry channels x 16 Gaussian-tuned receptive-field cells.
//   - Postsynaptic V4 neurons `512..767`: 16 learned shape prototype assemblies x 16 cells/shape.
//   - Mode 0: Evolved-LUT STDP (`g.basal_rule`) imprints V1->V4 receptive field of `proto_id` into L2 PW24!
//   - Mode 1: Runs 256-thread parallel spiking wavefront connected-component object counting +
//     2D spatial relation (`above/below/leftof/rightof`) + V1->V4 synaptic shape classification!
// SURVIVES `--ablate hipp` (Duotrigintuple 32-Track Ribot's Law!) & `--ablate freeze`, collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word VIS_W = 16;
constexpr Word VIS_H = 16;
constexpr Word MAX_VIS_PROTOS = 16;

struct alignas(16) NeoVisualCortexQuery {
    Word pixels[8]{};               // 256 retinal photoreceptors (bit `(y*16 + x) & 31` of `pixels[(y*16+x)>>5]`)
    Word pathway = 24;              // Dedicated L2 Pathway 24 (`PW24`) — ZERO extra L2 memory!
    Word mode = 1;                  // 0 = evolved-LUT STDP V1->V4 prototype learn, 1 = full retinal scene & V4 recognition
    Word clear_first = 0;           // Mode 0: wipe L2 PW24 before imprinting prototypes
    Word proto_id = 0;              // Mode 0: V4 prototype assembly slot (0..15)
};

struct alignas(16) NeoVisualCortexResult {
    SpikeVec v4_spikes{};           // Winning 16-neuron V4 shape prototype assembly firing on Blackwell GPU!
    Word best_proto = ~0u;          // Winning V4 shape ID (0..15, or ~0u if none learned/active)
    int  best_score = 0;            // Winning V4 synaptic drive straight from L2 PW24
    Word obj_count = 0;             // Distinct connected visual objects counted by 256-thread GPU spiking wavefront
    Word total_area = 0;            // Total firing retinal photoreceptors (0..256)
    Word sym_x = 0;                 // 1 if primary visual object has left-right mirror symmetry
    Word sym_y = 0;                 // 1 if primary visual object has top-bottom mirror symmetry
    Word rel_code = 0;              // 2-object 2D spatial relation of largest vs 2nd object: 0=single, 1=above, 2=below, 3=left, 4=right
    Word wave_steps = 0;            // Spiking wavefront iterations to connected-component convergence
    Word active_protos = 0;         // Number of learned V4 prototypes decoded straight from L2 PW24
    Word edits_made = 0;            // Mode 0: live evolved-LUT STDP synaptic edits written into L2 PW24
    Word spikes_fired = 0;
    Word pad = 0;
};

// ---------------------------------------------------------------------------
// Kernel 32 (STAGE 6b!): Pure-GPU 16x16 Spiking Cochlear Audio, Harmonic Chord
// & Emotional Prosody Cortex (`mind_neo_audio_cortex`) on L2 Pathway 27 (`PW27`)!
// ZERO extra L2 memory, ZERO atomics, ZERO host audio classification or pitch heuristics!
//   - Input: raw 16-frame x 16-frequency-band spiking cochleagram (`Word spectro[8]` = 256 hair cells).
//   - Step 1: 256-thread parallel 8-neighbor lateral inhibition suppresses isolated background noise spikes!
//   - Step 2: Computes rhythmic syllable/beat burst count (`beat_count`), pitch contour slope (`pitch_dir`:
//     0=flat, 1=rising question, 2=falling statement), harmonic consonance/dissonance & 16 time-shift +
//     pitch-transposition invariant A1 feature channels encoded as 256 Gaussian-tuned A1 spikes (`0..255`).
//   - Postsynaptic Auditory Belt Assemblies (`512..767`): 16 auditory prototypes x 16 cells/prototype in L2 PW27
//     (disjoint from upper zone `768..1023` used for online personal/cultural micro-ripples!).
//   - Mode 0: Evolved-LUT STDP (`g.basal_rule`) imprints A1->Belt receptive field of `proto_id` into L2 PW27!
//   - Mode 1: All 256 threads evaluate all 16 auditory assemblies simultaneously straight from L2 PW27!
// SURVIVES `--ablate hipp` (Tretrigintuple 33-Track Ribot's Law!) & `--ablate freeze`, collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word AUD_T = 16;
constexpr Word AUD_F = 16;
constexpr Word MAX_AUD_PROTOS = 16;

struct alignas(16) NeoAudioCortexQuery {
    Word spectro[8]{};              // 256 cochlear hair cells (bit `(t*16 + f) & 31` of `spectro[(t*16+f)>>5]`)
    Word pathway = 27;              // Dedicated L2 Pathway 27 (`PW27`, rows 512..767) — ZERO extra L2 memory!
    Word mode = 1;                  // 0 = evolved-LUT STDP A1->Belt prototype learn, 1 = cochlear pitch/beat/chord recognition
    Word clear_first = 0;           // Mode 0: clear auditory rows `512..767` in L2 PW27 before imprinting
    Word proto_id = 0;              // Mode 0: auditory prototype assembly slot (0..15)
};

struct alignas(16) NeoAudioCortexResult {
    SpikeVec a1_spikes{};           // Winning 16-neuron auditory belt assembly firing on Blackwell GPU!
    Word best_proto = ~0u;          // Winning auditory prototype ID (0..15, or ~0u if none active)
    int  best_score = 0;            // Winning auditory synaptic drive straight from L2 PW27
    Word beat_count = 0;            // Distinct rhythmic syllable/beat bursts counted on GPU
    Word pitch_dir = 0;             // 0 = pitchflat (steady/chord), 1 = pitchup (rising question), 2 = pitchdown (falling statement)
    Word consonance = 0;            // Harmonic consonance score (0..15)
    Word dissonance = 0;            // Harmonic dissonance score (0..15)
    Word active_frames = 0;         // Active denoised time frames (0..16)
    Word active_protos = 0;         // Number of learned auditory prototypes decoded straight from L2 PW27
    Word edits_made = 0;            // Mode 0: live evolved-LUT STDP synaptic edits written into L2 PW27
    Word spikes_fired = 0;
    Word pad = 0;
};

// ---------------------------------------------------------------------------
// Kernel 33 (STAGE 6c!): Pure-GPU Spiking Cross-Modal Vision+Audio Binding &
// Hierarchical Prefrontal Working-Memory Goal Stack (`mind_neo_crossmodal_stack`)
// on L2 Pathway 24 orthogonal quadrant (`pre 256..511`, `post rows 768..1023`)!
// ZERO extra L2 memory (orthogonal to K31 on PW24 rows 512..767), ZERO atomics, ZERO host stack!
//   - Sub-Region A (rows 768..895): 8 cross-modal (Vision K31 + Audio K32) assemblies x 16 cells.
//     * Mode 0: Evolved-LUT STDP (`g.basal_rule`) binds `(vis_id, aud_id) -> xproto_id` in L2 PW24!
//     * Mode 1: Joint cross-modal fusion `(vis_id, aud_id) -> best_xproto` straight from L2 PW24!
//     * Mode 2: Backward synesthetic transpose recall `W^T` (Vision->Audio or Audio->Vision)!
//   - Sub-Region B (rows 896..1023): 8-level Prefrontal Spiking Goal Stack (`depth 0..7`, 16 goals)!
//     * Mode 3: Push `goal_id` onto L2 spiking stack via evolved `g.basal_rule` LTP!
//     * Mode 4: Pop top goal via evolved `g.basal_rule` LTD & resurface parent goal straight from L2!
//     * Mode 5: Peek top goal & stack depth straight from L2 PW24!
//     * Mode 6: Clear prefrontal goal stack rows `896..1023`.
// SURVIVES `--ablate hipp` (Quattuortrigintuple 34-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_XMODAL_PROTOS = 8;
constexpr Word MAX_GOAL_STACK = 8;
constexpr Word MAX_GOAL_SLOTS = 16;

struct alignas(16) NeoCrossModalStackQuery {
    Word pathway = 24;              // Orthogonal quadrant of L2 PW24 (`pre 256..511`, `post 768..1023`) — ZERO extra L2 memory!
    Word mode = 1;                  // 0=bind STDP, 1=fuse, 2=synesthetic W^T recall, 3=stack push, 4=stack pop, 5=stack peek, 6=stack clear
    Word clear_first = 0;           // Mode 0: clear rows 768..895 before imprinting canonical cross-modal bindings
    Word xproto_id = 0;             // Mode 0: multimodal concept slot (0..7)
    Word vis_id = ~0u;              // Visual shape ID (0..7, or ~0u if absent)
    Word aud_id = ~0u;              // Auditory prototype ID (0..7, or ~0u if absent)
    Word goal_id = 0;               // Mode 3: prefrontal goal ID to push (0..15)
    Word pad = 0;
};

struct alignas(16) NeoCrossModalStackResult {
    SpikeVec mm_spikes{};           // Winning 16-neuron cross-modal / prefrontal stack assembly firing on Blackwell GPU!
    Word best_xproto = ~0u;         // Winning cross-modal concept ID (0..7)
    int  best_score = 0;            // Winning cross-modal synaptic drive straight from L2 PW24
    Word recalled_vis = ~0u;        // Mode 2: synesthetically recalled visual shape ID via W^T
    Word recalled_aud = ~0u;        // Mode 2: synesthetically recalled auditory pattern ID via W^T
    Word stack_depth = 0;           // Prefrontal spiking goal stack depth (0..8) straight from L2 PW24
    Word top_goal = ~0u;            // Active top goal ID (0..15, or ~0u if empty) decoded straight from L2 PW24
    Word active_xprotos = 0;        // Number of learned cross-modal assemblies in L2 PW24[768..895]
    Word edits_made = 0;            // Mode 0/3/4/6: live evolved-LUT STDP LTP/LTD synaptic edits in L2 PW24
    Word spikes_fired = 0;
    Word pad0 = 0;
    Word pad1 = 0;
};

// ---------------------------------------------------------------------------
// Kernel 34 (STAGE 7 CROSSING!): Pure-GPU Spiking Multi-Turn Dialogue Coherence,
// Topic-Shift & User-Claim Contradiction Cortex (`mind_neo_dialogue_cortex`) on
// Orthogonal Quadrant of L2 PW27 (`pre 256..511`, `post rows 768..1023`, ZERO extra L2 memory!)
//   - Sub-Region A (rows 768..895): 8 Pragmatic Discourse Modes (`dlgexplain`/`dlgempathize`/
//     `dlgplan`/`dlgcompare` + live 1-shot `dlgdebate`) imprinted via evolved `g.basal_rule` STDP!
//   - Sub-Region B (rows 896..911): Persistent Active Topic Register updated live on every Mode-1 turn
//     for pure-GPU multi-turn topic-shift detection (`dlgshift` vs `dlgstay`)!
//   - Sub-Region C (rows 928..1023): 6 Live User-Claim Stance Slots (`Mode 2` STDP imprint, `Mode 3`
//     consistency vs direct contradiction check -> `dlgagree` / `dlgcontra`)!
// SURVIVES `--ablate hipp` (Quintrigintuple 35-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_DLG_MODES = 8;
constexpr Word MAX_DLG_CUES = 16;
constexpr Word MAX_DLG_CLAIMS = 6;

struct alignas(16) NeoDialogueQuery {
    Word pathway = 27;              // Orthogonal quadrant of L2 PW27 (`pre 256..511`, `post 768..1023`) — ZERO extra L2 memory!
    Word mode = 1;                  // 0=imprint mode STDP, 1=classify+shift+update, 2=imprint stance, 3=check stance, 4=clear trace+stance
    Word clear_first = 0;           // Mode 0: clear rows 768..1023 (pre 256..511 + diagonals) before training
    Word read_only = 0;             // Mode 1: when 1 (e.g. --ablate freeze), classify without writing topic trace
    Word dlg_mode = 0;              // Mode 0: pragmatic mode slot (0..7)
    Word cue_a = ~0u;               // Cue channel A (0..15, or ~0u)
    Word cue_b = ~0u;               // Cue channel B (0..15, or ~0u)
    Word claim_slot = 0;            // Mode 2/3: claim slot (0..5)
    Word polarity = 1;              // Mode 2/3: 1=positive stance, 0=negative stance
    Word pad[3]{};
};

struct alignas(16) NeoDialogueResult {
    SpikeVec dlg_spikes{};          // Winning 16-neuron discourse-mode or stance assembly firing on Blackwell GPU!
    Word best_mode = ~0u;           // Mode 1: winning pragmatic discourse mode (0..7)
    Word prev_mode = ~0u;           // Mode 1: prior active discourse mode read straight from L2 PW27[896..911]
    Word topic_shifted = 0;         // Mode 1: 1 when prev_mode != ~0u and best_mode != prev_mode
    Word coh_status = 0;            // Mode 3: 0=unknown, 1=consistent (dlgagree), 2=contradiction (dlgcontra)
    int  best_score = 0;            // Winning synaptic drive straight from L2 PW27
    Word active_modes = 0;          // Number of learned pragmatic modes in L2 PW27[768..895]
    Word edits_made = 0;            // Live evolved-LUT STDP synaptic edits in L2 PW27
    Word spikes_fired = 0;
};

// ---------------------------------------------------------------------------
// Kernel 35 (STAGE 7b!): Pure-GPU Spiking Episodic Narrative Arc, Conceptual
// Metaphor Blending & Socratic Self-Critique Cortex (`mind_neo_narrative_metaphor`)
// on Orthogonal Quadrant of L2 PW27 (`pre 512..767`, `post rows 256..511`, ZERO extra L2 memory!)
//   - Sub-Region A (rows 256..383): 8 Conceptual Metaphor Blends (`metriver`/`metweb`/`metspark`/
//     `metmirror` + live 1-shot `metcompass`) imprinted via evolved `g.basal_rule` STDP (`Mode 0`),
//     forward blended & backward `W^T` explained (`Mode 1`)!
//   - Sub-Region B1 (rows 384..447): 4 Narrative Plot-Arc Beats (`plotquest -> plottrial -> plotturn -> plotwin`)
//     imprinted via evolved STDP (`Mode 2`) and unrolled over 2 recurrent spiking hops on GPU (`Mode 3`)!
//   - Sub-Region B2 (rows 448..511): 4 Socratic Self-Critique Flaw Detectors (`flawhasty`/`flawcircle`/
//     `flawfalse`/`flawstraw`) imprinted via evolved STDP (`Mode 2`) & checked on GPU (`Mode 4`)!
// SURVIVES `--ablate hipp` (Sextrigintuple 36-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_MET_BLENDS = 8;
constexpr Word MAX_MET_FEATS = 16;
constexpr Word MAX_PLOT_BEATS = 4;
constexpr Word MAX_FLAW_SLOTS = 4;

struct alignas(16) NeoNarrativeMetaphorQuery {
    Word pathway = 27;              // Orthogonal quadrant of L2 PW27 (`pre 512..767`, `post 256..511`) — ZERO extra L2 memory!
    Word mode = 1;                  // 0=imprint blend STDP, 1=forward blend or W^T explain, 2=imprint plot/flaw, 3=2-hop plot arc, 4=critique flaw
    Word clear_first = 0;           // Mode 0/2: clear Sub-Region A (`256..383`) or B (`384..511`) in `pre 512..767` + diagonal
    Word slot_id = 0;               // Mode 0/1(explain): blend 0..7; Mode 2: plot target 0..3 or flaw 4..7
    Word cue_a = ~0u;               // Feature/beat/flaw cue A (0..15, or ~0u for backward W^T explain in Mode 1)
    Word cue_b = ~0u;               // Feature/beat/flaw cue B (0..15, or ~0u)
    Word hops = 2;                  // Mode 3: recurrent narrative hops (1 or 2)
    Word pad = 0;
};

struct alignas(16) NeoNarrativeMetaphorResult {
    SpikeVec nm_spikes{};           // Winning 16-neuron metaphor/plot/critique assembly firing on Blackwell GPU!
    Word best_blend = ~0u;          // Mode 1 forward: winning metaphor blend ID (0..7)
    Word recalled_a = ~0u;          // Mode 1 backward W^T: recalled source feature (0..15)
    Word recalled_b = ~0u;          // Mode 1 backward W^T: recalled target feature (0..15)
    Word hop1_beat = ~0u;           // Mode 3: Hop-1 narrative beat (0..3)
    Word hop2_beat = ~0u;           // Mode 3: Hop-2 narrative beat (0..3)
    Word best_flaw = ~0u;           // Mode 4: winning Socratic self-critique flaw ID (0..3 -> slots 4..7)
    int  best_score = 0;            // Winning synaptic drive straight from L2 PW27
    Word active_blends = 0;         // Number of learned metaphor blends in L2 PW27[256..383]
    Word edits_made = 0;            // Mode 0/2: live evolved-LUT STDP synaptic edits in L2 PW27
    Word spikes_fired = 0;
    Word pad[2]{};
};

// ---------------------------------------------------------------------------
// Kernel 36 (STAGE 7c — 400 DOMAINS!): Pure-GPU Spiking Epistemic Belief-Graph,
// Evidence-Weighting & Multi-Agent Scientific Debate Cortex (`mind_neo_belief_debate`)
// on FREE Orthogonal Quadrant of L2 PW24 (`pre 512..767`, `post rows 256..511`, ZERO extra L2 memory!)
//   - Sub-Region A (rows 256..383): 8 Scientific Theories (`theorywave`/`theorydust`/`theoryfield` +
//     live 1-shot `theoryflux`) imprinted with supporting LTP / refuting LTD evidence via `g.basal_rule`!
//   - Sub-Region B (rows 384..511): 8 Peer Source-Credibility Registers (`peerada`/`peerloki`...)
//     calibrated via evolved STDP (`Mode 2`) and adjudicated in parallel on GPU (`Mode 3`)!
// SURVIVES `--ablate hipp` (Septentrigintuple 37-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_BEL_CLAIMS = 8;
constexpr Word MAX_BEL_EVID = 16;
constexpr Word MAX_BEL_PEERS = 8;

struct alignas(16) NeoBeliefDebateQuery {
    Word pathway = 24;              // Free orthogonal quadrant of L2 PW24 (`pre 512..767`, `post 256..511`) — ZERO extra L2 memory!
    Word mode = 1;                  // 0=imprint claim evidence (LTP/LTD), 1=evaluate belief-graph, 2=imprint peer credibility, 3=peer debate, 4=clear
    Word clear_first = 0;           // Mode 0/2: clear Sub-Region A (`256..383`) or B (`384..511`) in `pre 512..767` + diagonal
    Word claim_id = 0;              // Mode 0: scientific theory slot (0..7)
    Word ev_a = ~0u;                // Evidence channel A (0..15, or ~0u)
    Word ev_b = ~0u;                // Evidence channel B (0..15, or ~0u)
    Word polarity = 1;              // Mode 0/2: 1=support/verified truth (LTP), 0=refute/bluff (LTD)
    Word peer_a = 0;                // Mode 2: peer to calibrate (0..7); Mode 3: competing peer A (0..7)
    Word peer_b = 1;                // Mode 3: competing peer B (0..7)
    Word pad[3]{};
};

struct alignas(16) NeoBeliefDebateResult {
    SpikeVec bd_spikes{};           // Winning 16-neuron theory or credible-peer assembly firing on Blackwell GPU!
    Word best_claim = ~0u;          // Mode 1: winning scientific theory ID (0..7)
    Word verdict = 0;               // Mode 1: 0=unknown, 1=belconfirm, 2=belrefute, 3=belsplit
    Word best_peer = ~0u;           // Mode 3: winning credible peer ID (peer_a or peer_b)
    int  best_score = 0;            // Winning net spiking synaptic drive straight from L2 PW24
    int  cred_margin = 0;           // Mode 3: synaptic credibility margin (score_a - score_b)
    Word active_claims = 0;         // Number of learned theories in L2 PW24[256..383]
    Word edits_made = 0;            // Mode 0/2/4: live evolved-LUT STDP synaptic edits in L2 PW24
    Word spikes_fired = 0;
};

// ---------------------------------------------------------------------------
// Kernel 37 (STAGE 8 CROSSING — COLLECTIVE SCIENTIFIC MINDS!): Pure-GPU Spiking
// Multi-Mind Collective Discovery, Hypothesis Consensus & Compositional Theorem Prover
// (`mind_neo_collective_discovery`) on FREE Orthogonal Quadrant of L2 PW24
// (`pre 768..1023`, `post rows 0..255`, ZERO extra L2 memory & 100% DISJOINT from
// K31 Retina `512..767`, K33 XModal `768..1023` AND K36 Belief-Debate `256..511`!):
//   - Sub-Region A (rows 0..127): 8 Collective Scientific Conjectures (`lawthermo`/`lawgene`/
//     `lawether` + live 1-shot `lawquantum`) voted on by independent specialist minds
//     (`specphys`/`specbio`/`specmath`/`speclogic`) via evolved `g.basal_rule` STDP (`Mode 0`),
//     adjudicated in parallel on GPU (`Mode 1` -> `collaccept`/`collreject`/`collsplit`)!
//   - Sub-Region B (rows 128..255): 8 Compositional Theorems (`thmcarnot`/`thmforce`/`thmmaxwell`
//     + live 1-shot `thmdirac`) imprinted from pairs of founding axioms (`axenergy`..`axspin`,
//     `Mode 2`), synthesized forward & deconstructed via backward `W^T` proof recall (`Mode 3`)!
// SURVIVES `--ablate hipp` (Octotrigintuple 38-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_COLL_LAWS = 8;
constexpr Word MAX_COLL_THMS = 8;
constexpr Word MAX_COLL_CHANS = 16;

struct alignas(16) NeoCollectiveDiscoveryQuery {
    Word pathway = 24;              // Free orthogonal quadrant of L2 PW24 (`pre 768..1023`, `post 0..255`) — ZERO extra L2 memory!
    Word mode = 1;                  // 0=imprint specialist vote, 1=evaluate consensus pool/law, 2=imprint theorem, 3=forward derive or W^T prove, 4=clear
    Word clear_first = 0;           // Mode 0/2: clear Sub-Region A (`0..127`) or B (`128..255`) in `pre 768..1023` + diagonal
    Word law_id = ~0u;              // Mode 0: conjecture slot (0..7); Mode 1: specific conjecture (0..7) or ~0u for open pool
    Word thm_id = ~0u;              // Mode 2: theorem slot (0..7); Mode 3 backward W^T: theorem to deconstruct (0..7)
    Word cue_a = ~0u;               // Specialist (0..3) or Axiom (4..11) channel A, or ~0u
    Word cue_b = ~0u;               // Specialist (0..3) or Axiom (4..11) channel B, or ~0u
    Word polarity = 1;              // Mode 0: 1=support vote (LTP), 0=refutation vote (LTD)
};

struct alignas(16) NeoCollectiveDiscoveryResult {
    SpikeVec cd_spikes{};           // Winning 16-neuron conjecture or theorem assembly firing on Blackwell GPU!
    Word best_law = ~0u;            // Mode 1: winning/audited collective conjecture ID (0..7)
    Word verdict = 0;               // Mode 1: 0=unknown, 1=collaccept, 2=collreject, 3=collsplit
    Word best_thm = ~0u;            // Mode 3 forward: winning compositional theorem ID (0..7)
    Word recalled_a = ~0u;          // Mode 3 backward W^T: recalled founding axiom A (4..11)
    Word recalled_b = ~0u;          // Mode 3 backward W^T: recalled founding axiom B (4..11)
    int  best_score = 0;            // Winning net spiking synaptic drive straight from L2 PW24
    Word active_laws = 0;           // Active conjectures in L2 PW24[0..127]
    Word active_thms = 0;           // Active theorems in L2 PW24[128..255]
    Word edits_made = 0;            // Mode 0/2/4: live evolved-LUT STDP synaptic edits in L2 PW24
    Word spikes_fired = 0;
    Word pad[2]{};
};

// ---------------------------------------------------------------------------
// Kernel 38 (STAGE 8b — CONTINUAL SELF-RESEARCH & COUNTERFACTUAL CODE PATCHING!):
// Pure-GPU Spiking Discriminative Experiment-Design & Counterfactual Code-Patching Cortex
// (`mind_neo_self_research`) on FREE Orthogonal Quadrant of L2 PW27
// (`pre 768..1023`, `post rows 0..255`, ZERO extra L2 memory & 100% DISJOINT from
// K32 Cochlea `512..767`, K34 Dialogue `768..1023`, K35 Metaphor `256..511` & Cultural `768..1023`!):
//   - Sub-Region A (rows 0..127): 8 Diagnostic Experiments (`expvacuum`/`expcoil`/`expprism`/
//     `explitmus` + live 1-shot `expspectrum`) predicting opposite signals across competing
//     hypotheses (`hypgrav`..`hypbase`, channels 0..7) via evolved `g.basal_rule` STDP (`Mode 0`);
//     `Mode 1` computes `|score(exp, H_a) - score(exp, H_b)|` across all 8 experiments in parallel!
//   - Sub-Region B (rows 128..255): 8 Counterfactual Repair Patches (`patbound`/`patguard`/
//     `patmutex`/`patclamp` + live 1-shot `patfree`) bound to bug symptoms (`symoffone`..`symdrift`,
//     channels 8..15, `Mode 2`); `Mode 3` selects the optimal repair patch forward OR runs
//     backward `W^T` root-cause recall (`patbound -> symoffone syminfloop`)!
// SURVIVES `--ablate hipp` (Novemtrigintuple 39-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_SR_EXPS = 8;
constexpr Word MAX_SR_PATCHES = 8;
constexpr Word MAX_SR_CHANS = 16;

struct alignas(16) NeoSelfResearchQuery {
    Word pathway = 27;              // Free orthogonal quadrant of L2 PW27 (`pre 768..1023`, `post 0..255`) — ZERO extra L2 memory!
    Word mode = 1;                  // 0=imprint experiment predictions, 1=optimal discriminative experiment, 2=imprint patch, 3=patch or W^T why, 4=clear
    Word clear_first = 0;           // Mode 0/2: clear Sub-Region A (`0..127`) or B (`128..255`) in `pre 768..1023` + diagonal
    Word exp_id = ~0u;              // Mode 0: experiment slot (0..7)
    Word patch_id = ~0u;            // Mode 2: patch slot (0..7); Mode 3 backward W^T: patch to explain (0..7)
    Word cue_a = ~0u;               // Hypothesis A (0..7) or Symptom A (8..15), or ~0u
    Word cue_b = ~0u;               // Hypothesis B (0..7) or Symptom B (8..15), or ~0u
    Word polarity = 2;              // Mode 0: 2=(cue_a +LTP, cue_b -LTD), 1=+LTP, 0=-LTD
};

struct alignas(16) NeoSelfResearchResult {
    SpikeVec sr_spikes{};           // Winning 16-neuron experiment or repair-patch assembly firing on Blackwell GPU!
    Word best_exp = ~0u;            // Mode 1: optimal discriminative experiment ID (0..7)
    int  disc_power = 0;            // Mode 1: |score(exp, hyp_a) - score(exp, hyp_b)| straight from L2 PW27
    Word best_patch = ~0u;          // Mode 3 forward: winning counterfactual repair patch ID (0..7)
    Word recalled_a = ~0u;          // Mode 3 backward W^T: recalled root-cause symptom A (8..15)
    Word recalled_b = ~0u;          // Mode 3 backward W^T: recalled root-cause symptom B (8..15)
    int  best_score = 0;            // Winning net spiking synaptic drive straight from L2 PW27
    Word active_exps = 0;           // Active experiments in L2 PW27[0..127]
    Word active_patches = 0;        // Active repair patches in L2 PW27[128..255]
    Word edits_made = 0;            // Mode 0/2/4: live evolved-LUT STDP synaptic edits in L2 PW27
    Word spikes_fired = 0;
    Word pad[2]{};
};

// ---------------------------------------------------------------------------
// Kernel 39 (STAGE 8c — PURE-GPU NEURAL SYMBOLIC ALGEBRA & CALCULUS!):
// Pure-GPU Spiking Symbolic Polynomial Differentiation, Definite Integration &
// Horner Evaluation Cortex (`mind_neo_symbolic_calculus`) on the FIRST of FOUR
// FREE `256 x 256` Orthogonal Quadrants of L2 PW13 (`post rows 0..255`, `pre cols 0..255`,
// ZERO extra L2 memory & 100% DISJOINT from K30 3-SAT `rows/cols 512..1023`!):
//   - Stores up to 8 degree-3 polynomials `P(x) = c0 + c1*x + c2*x^2 + c3*x^3`
//     directly inside plastic L2 synapses via evolved `g.basal_rule` STDP (`Mode 0`)!
//   - Mode 1: Pure-GPU symbolic power-rule differentiation `d/dx P(x)` & `P'(x0)`!
//   - Mode 2: Pure-GPU definite antiderivative integration `\int_0^{x0} P(x) dx`!
//   - Mode 3: Pure-GPU Horner polynomial evaluation `P(x0)`!
//   - Mode 4: Clean clear of `PW13[0..255, 0..255]` without touching K30 3-SAT!
// SURVIVES `--ablate hipp` (Quadragintuple 40-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_SYMC_POLYS = 8;
constexpr Word MAX_SYMC_DEGS = 4;

struct alignas(16) NeoSymCalcQuery {
    Word pathway = 13;              // Free orthogonal quadrant of L2 PW13 (`rows 0..255`, `cols 0..255`) — ZERO extra L2 memory!
    Word mode = 1;                  // 0=STDP imprint poly, 1=symbolic diff P'(x0), 2=definite integral \int_0^x0 P dx, 3=Horner eval P(x0), 4=clear
    Word clear_first = 0;           // Mode 0: clear `PW13[0..255, 0..255]` before imprinting
    Word poly_id = 0;               // Polynomial register slot (0..7)
    Word coeffs[MAX_SYMC_DEGS]{};   // Mode 0: integer coefficients c0..c3 (0..15)
    Word eval_x = 0;                // Modes 1/2/3: evaluation/integration upper bound x0 (0..15)
    Word pad[3]{};
};

struct alignas(16) NeoSymCalcResult {
    SpikeVec sc_spikes{};           // Winning 16-neuron polynomial assembly firing on Blackwell GPU!
    Word best_poly = ~0u;           // Active polynomial ID (0..7)
    Word out_c[MAX_SYMC_DEGS]{};    // Decoded or differentiated symbolic coefficients
    Word eval_val = 0;              // Pure-GPU computed P'(x0), \int_0^x0 P(x) dx, or P(x0)
    Word active_polys = 0;          // Active polynomials stored in L2 PW13[0..127]
    Word edits_made = 0;            // Mode 0/4: live evolved-LUT STDP synaptic edits in L2 PW13
    Word spikes_fired = 0;
    Word pad[3]{};
};

// ---------------------------------------------------------------------------
// Kernel 40 (STAGE 8d — PURE-GPU AST PROGRAM SYNTHESIS, TYPE VERIFICATION &
// SYMBOLIC EXECUTION CORTEX!):
// Lives on the SECOND of FOUR FREE `256 x 256` Orthogonal Quadrants of L2 PW13
// (`post rows 0..255`, `pre cols 256..511`, ZERO extra L2 memory & 100% DISJOINT
// from K30 3-SAT `rows/cols 512..1023` AND K39 Symbolic Calculus `cols 0..255`!):
//   - Stores up to 8 typed 2-step spiking AST programs (`pid = 0..7`) directly
//     as 8 `16x16` plastic L2 synaptic tiles (`op0, arg0, op1, arg1, in_t, out_t, bound, tag`)
//     via evolved `g.basal_rule` STDP (`Mode 0`)!
//   - Mode 1: Pure-GPU static type & bounds safety verification (`astverify`)!
//   - Mode 2: Pure-GPU spiking symbolic execution `P(eval_x)` (`astexec`)!
//   - Mode 3: Pure-GPU 256-thread parallel AST program synthesis from `(x1->y1, x2->y2)`
//     input-output specs + immediate evolved-LUT STDP imprinting & generalization `P(eval_x)`!
//   - Mode 4: Clean clear of `PW13[0..255, 256..511]` preserving K30 & K39!
// SURVIVES `--ablate hipp` (Unquadragintuple 41-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_AST_PROGS = 8;
constexpr Word MAX_AST_FIELDS = 8;

struct alignas(16) NeoProgAstQuery {
    Word pathway = 13;               // Free orthogonal quadrant Q1 of L2 PW13 (`rows 0..255`, `cols 256..511`)
    Word mode = 2;                   // 0=STDP imprint AST, 1=type verify, 2=symbolic exec, 3=256-thread parallel synthesis+write+eval, 4=clear
    Word clear_first = 0;            // Mode 0: clear `PW13[0..255, 256..511]` before imprinting
    Word prog_id = 0;                // Program register slot (0..7)
    Word fields[MAX_AST_FIELDS]{};   // Mode 0: {op0, arg0, op1, arg1, in_t, out_t, bound, tag} (0..15 each)
    Word eval_x = 0;                 // Modes 2/3: execution / generalization input x
    Word in_x1 = 0;                  // Mode 3: spec pair 1 input
    Word out_y1 = 0;                 // Mode 3: spec pair 1 expected output
    Word in_x2 = 0;                  // Mode 3: spec pair 2 input
    Word out_y2 = 0;                 // Mode 3: spec pair 2 expected output
    Word no_write = 0;               // Mode 3: if 1 (e.g. --ablate freeze), forbid STDP synthesis imprinting
    Word pad[2]{};
};

struct alignas(16) NeoProgAstResult {
    SpikeVec ast_spikes{};           // Winning 16-neuron AST program assembly firing on Blackwell GPU!
    Word best_prog = ~0u;            // Active program ID (0..7)
    Word out_f[MAX_AST_FIELDS]{};    // Decoded or synthesized AST fields {op0, arg0, op1, arg1, in_t, out_t, bound, tag}
    Word eval_val = 0;               // Mode 2/3: pure-GPU symbolic execution output P(eval_x)
    Word type_valid = 0;             // Mode 1/2/3: 1 if static type & bounds check passes
    Word synth_found = 0;            // Mode 3: 1 if 256-thread parallel synthesis matched both specs
    Word active_progs = 0;           // Active AST programs stored in L2 PW13[0..127, 256..511]
    Word edits_made = 0;             // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW13 Q1
    Word spikes_fired = 0;
};

// ---------------------------------------------------------------------------
// Kernel 41 (STAGE 8e — PURE-GPU NEURAL LINEAR ALGEBRA, MATRIX-VECTOR &
// CRAMER/GAUSSIAN LINEAR SYSTEM SOLVER CORTEX!):
// Lives on the THIRD of FOUR FREE `256 x 256` Orthogonal Quadrants of L2 PW13
// (`post rows 256..511`, `pre cols 0..255`, ZERO extra L2 memory & 100% DISJOINT
// from K30 3-SAT `512..1023`, K39 SymCalc `0..255 x 0..255`, and K40 AST `0..255 x 256..511`!):
//   - Stores up to 8 2x2 integer matrices + RHS vectors `(a, b, c, d, rhs0, rhs1)` (`0..60` each)
//     as 6 `16x16` plastic L2 synaptic tiles (`W = val * 2`) via evolved `g.basal_rule` STDP (`Mode 0`)!
//   - Mode 1: Pure-GPU determinant `det(A)`, trace `tr(A)` & invertibility check (`lindet`/`lintr`)!
//   - Mode 2: Pure-GPU matrix-vector product `A * v` (`linvec`)!
//   - Mode 3: Pure-GPU Cramer/Gaussian linear system solver `A * x = rhs` (with optional live 1-shot
//     evolved-LUT STDP imprint + immediate solve in ONE kernel launch!) (`linsolve`)!
//   - Mode 4: Clean clear of `PW13[256..511, 0..255]` preserving K30, K39 & K40!
// SURVIVES `--ablate hipp` (Duoquaragintuple 42-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_LINALG_MATS = 8;
constexpr Word MAX_LINALG_FIELDS = 6;

struct alignas(16) NeoLinAlgQuery {
    Word pathway = 13;                 // Free orthogonal quadrant Q2 of L2 PW13 (`rows 256..511`, `cols 0..255`)
    Word mode = 1;                     // 0=STDP imprint, 1=det/trace, 2=matvec, 3=Cramer solve (+optional live imprint), 4=clear
    Word clear_first = 0;              // Mode 0: clear `PW13[256..511, 0..255]` before imprinting
    Word mat_id = 0;                   // Matrix register slot (0..7)
    Word fields[MAX_LINALG_FIELDS]{};  // Mode 0/3: {a, b, c, d, rhs0, rhs1} (0..60 each)
    Word vx = 0;                       // Mode 2: input vector x
    Word vy = 0;                       // Mode 2: input vector y
    Word sub_op = 0;                   // Mode 1: 0=det, 1=trace, 2=invertible; Mode 3: 0=x0, 1=x1
    Word also_imprint = 0;             // Mode 3: if 1, imprint `fields[0..5]` via STDP first, then solve
    Word no_write = 0;                 // Mode 0/3: if 1 (e.g. --ablate freeze), forbid STDP imprinting
    Word pad = 0;
};

struct alignas(16) NeoLinAlgResult {
    SpikeVec la_spikes{};              // Winning 16-neuron matrix assembly firing on Blackwell GPU!
    Word best_mat = ~0u;               // Active matrix slot (0..7)
    Word out_f[MAX_LINALG_FIELDS]{};   // Decoded {a, b, c, d, rhs0, rhs1} straight from L2 PW13 Q2
    Word det_val = 0;                  // |a*d - b*c|
    Word trace_val = 0;                // a + d
    Word invertible = 0;               // 1 if det != 0
    Word out_x0 = 0;                   // Mode 2: y0; Mode 3: solved x0
    Word out_x1 = 0;                   // Mode 2: y1; Mode 3: solved x1
    Word solved = 0;                   // Mode 3: 1 if exact integer solution verified on GPU
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_mats = 0;              // Active matrices stored in L2 PW13[256..383, 0..255]
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW13 Q2
    Word spikes_fired = 0;
    Word pad[2]{};
};

// ---------------------------------------------------------------------------
// Kernel 42 (v48 STAGE 8f — FINAL FREE QUADRANT OF L2 PW13!):
// Pure Blackwell GPU 256-Thread Spiking Probabilistic Bayes-Net & Expected-Utility
// Decision Inference Cortex (`mind_neo_bayes_net`) on Orthogonal Quadrant Q3
// (`PW13[rows 256..511, cols 256..511]` — 65,536 plastic synapses, ZERO extra L2 memory,
// 100% DISJOINT from K30 3-SAT `512..1023`, K39 SymCalc `0..255x0..255`,
// K40 ProgAst `0..255x256..511` & K41 LinAlg `256..511x0..255`!):
//   - Mode 0: Evolved-LUT STDP CPT Imprint `{prior, lik_pos, lik_neg, cost_fp, cost_fn, tag}` (0..100)
//   - Mode 1: Pure-GPU Positive-Evidence Posterior `P(H|E)` & Marginal `P(E)`
//   - Mode 2: Pure-GPU Negative-Evidence Posterior `P(H|~E)`
//   - Mode 3: Pure-GPU Expected-Utility Optimal Bayes Action (`bnetact`/`bnetwait`) + live 1-shot STDP imprint
//   - Mode 4: Clean clear of `PW13[256..511, 256..511]` preserving K30, K39, K40 & K41!
// SURVIVES `--ablate hipp` (Trequaragintuple 43-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_BNET_NODES = 8;
constexpr Word MAX_BNET_FIELDS = 6;

struct alignas(16) NeoBayesNetQuery {
    Word pathway = 13;                 // Final orthogonal quadrant Q3 of L2 PW13 (`rows 256..511`, `cols 256..511`)
    Word mode = 1;                     // 0=STDP imprint, 1=P(H|E)&P(E), 2=P(H|~E), 3=Bayes decision (+live imprint), 4=clear
    Word clear_first = 0;              // Mode 0: clear `PW13[256..511, 256..511]` before imprinting
    Word node_id = 0;                  // Bayes-net node slot (0..7)
    Word fields[MAX_BNET_FIELDS]{};    // {prior_pct, lik_pos_pct, lik_neg_pct, cost_fp, cost_fn, tag} (0..100 each)
    Word sub_op = 0;                   // Mode 1: 0=post_pos, 1=marginal; Mode 3: 0=post_pos, 1=opt_act
    Word also_imprint = 0;             // Mode 3: if 1, imprint `fields[0..5]` via STDP first, then decide
    Word no_write = 0;                 // Mode 0/3: if 1 (e.g. --ablate freeze), forbid STDP imprinting
    Word pad[3]{};
};

struct alignas(16) NeoBayesNetResult {
    SpikeVec bn_spikes{};              // Winning 16-neuron Bayes-net assembly firing on Blackwell GPU!
    Word best_node = ~0u;              // Active Bayes node slot (0..7)
    Word out_f[MAX_BNET_FIELDS]{};     // Decoded {prior, lik_pos, lik_neg, cost_fp, cost_fn, tag} straight from L2 PW13 Q3
    Word marginal_pct = 0;             // P(E) rounded %
    Word post_pos_pct = 0;             // P(H|E) rounded %
    Word post_neg_pct = 0;             // P(H|~E) rounded %
    Word opt_act = 0;                  // 1 = bnetact (loss_act <= loss_wait), 0 = bnetwait
    Word loss_act = 0;                 // Expected loss of acting: (100 - P(H|E)) * cost_fp
    Word loss_wait = 0;                // Expected loss of waiting: P(H|E) * cost_fn
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_nodes = 0;             // Active Bayes nodes stored in L2 PW13 Q3
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW13 Q3
    Word spikes_fired = 0;
    Word pad[2]{};
};

// ---------------------------------------------------------------------------
// Kernel 43 (v49 STAGE 8g — PURE-GPU SPIKING MDP VALUE-ITERATION & STOCHASTIC
// POLICY PLANNER): `mind_neo_mdp_planner` on FREE orthogonal quadrant of L2 PW24
// (`rows 256..511, cols 768..1023`, 65,536 synapses, ZERO extra L2 memory!):
//   - 100% disjoint from K31 Vision (`512..767`), K33 CrossModal (`768..1023`),
//     K36 BeliefDebate (`256..511 x 512..767` + diag `256..511`) & K37 CollDisc (`0..255`).
//   - 8 MDP registers (`eid = 0..7`, `row_base = 256 + 16*eid`), 6 16x16 plastic tiles
//     (`cols 768..863`) storing `{r0, r1, r2, p_a0, p_a1, gamma}` (0..100 each) +
//     `+16` tonic presence tags at `cols 896..1023`!
//   - Mode 0: Evolved `g.basal_rule` STDP MDP imprinting (respects `no_write`).
//   - Mode 1: Pure-GPU 4-step Bellman Value Iteration -> `V*(s0)` (`sub_op=0`) or `V*(s1)` (`sub_op=1`).
//   - Mode 2: Pure-GPU optimal stochastic policy `pi*(s0)` (`0=mdpsteady`, `1=mdpleap`).
//   - Mode 3: Optional live 1-shot STDP MDP imprinting + immediate 4-step GPU Bellman solve!
//   - Mode 4: Clean clear of `PW24[rows 256..511, cols 768..1023]` ONLY!
// SURVIVES `--ablate hipp` (Quattuorquaragintuple 44-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_MDP_ENVS = 8;
constexpr Word MAX_MDP_FIELDS = 6;

struct alignas(16) NeoMdpPlanQuery {
    Word pathway = 24;                 // Free orthogonal quadrant of L2 PW24 (`rows 256..511`, `cols 768..1023`)
    Word mode = 1;                     // 0=STDP imprint, 1=Bellman V*(s0)/V*(s1), 2=pi*(s0), 3=live imprint+solve, 4=clear
    Word clear_first = 0;              // Mode 0: clear `PW24[256..511, 768..1023]` before imprinting
    Word env_id = 0;                   // MDP slot (0..7)
    Word fields[MAX_MDP_FIELDS]{};     // {r0, r1, r2, p_a0, p_a1, gamma} (0..100 each)
    Word sub_op = 0;                   // Mode 1: 0=v_s0, 1=v_s1; Mode 3: 0=v_s0, 1=opt_act
    Word also_imprint = 0;             // Mode 3: if 1, imprint `fields[0..5]` via STDP first, then solve
    Word no_write = 0;                 // Mode 0/3: if 1 (e.g. --ablate freeze), forbid STDP imprinting
    Word pad[3]{};
};

struct alignas(16) NeoMdpPlanResult {
    SpikeVec mdp_spikes{};             // Winning 16-neuron MDP assembly firing on Blackwell GPU!
    Word best_env = ~0u;               // Active MDP slot (0..7)
    Word out_f[MAX_MDP_FIELDS]{};      // Decoded {r0, r1, r2, p_a0, p_a1, gamma} straight from L2 PW24
    Word v_s0 = 0;                     // Optimal 4-step Bellman value V*(s0)
    Word v_s1 = 0;                     // Optimal 4-step Bellman value V*(s1)
    Word q_act0 = 0;                   // Q*(s0, steady)
    Word q_act1 = 0;                   // Q*(s0, leap)
    Word opt_act = 0;                  // 0 = mdpsteady (q0 >= q1), 1 = mdpleap (q1 > q0)
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_envs = 0;              // Active MDP environments stored in L2 PW24
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW24
    Word spikes_fired = 0;
    Word pad[3]{};
};

// ---------------------------------------------------------------------------
// Kernel 44 (v50 STAGE 8h — 100-PILLAR MILESTONE! PURE-GPU SPIKING KALMAN
// FILTER & DYNAMICAL STATE-SPACE ESTIMATION CORTEX): `mind_neo_kalman_filter`
// on FREE orthogonal quadrant of L2 PW24 (`rows 512..767, cols 768..1023`,
// 65,536 plastic synapses, ZERO extra L2 memory!):
//   - 100% disjoint from K31 Vision (`rows 512..767 x cols 0..255 + diag 512..767`),
//     K33 CrossModal (`768..1023`), K36 BeliefDebate (`256..511 x 512..767`),
//     K37 CollDisc (`0..255`) & K43 MDP (`256..511 x 768..1023`).
//   - 8 Kalman filter channels (`kid = 0..7`, `row_base = 512 + 16*kid`), 6 16x16 tiles
//     (`cols 768..863`) storing `{x_prior, p_prior, q_proc, r_meas, z_meas, vel_u}` (0..100)
//     + `+16` tonic presence tags at `cols 896..1023`!
//   - Mode 0: Evolved `g.basal_rule` STDP filter imprinting (respects `no_write`).
//   - Mode 1: Pure-GPU Kalman Gain % (`sub_op=0`) or Posterior Variance (`sub_op=1`).
//   - Mode 2: Pure-GPU Posterior State `x_post` (`sub_op=0`) or Trust Regime (`sub_op=1`: 1=kftrust, 0=kfprior).
//   - Mode 3: Optional live 1-shot STDP filter imprinting + immediate Predict-Update Fusion!
//   - Mode 4: Clean clear of `PW24[rows 512..767, cols 768..1023]` ONLY!
// SURVIVES `--ablate hipp` (Quinquadragintuple 45-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_KFILT_CHANS = 8;
constexpr Word MAX_KFILT_FIELDS = 6;

struct alignas(16) NeoKalmanQuery {
    Word pathway = 24;                 // Free orthogonal quadrant of L2 PW24 (`rows 512..767`, `cols 768..1023`)
    Word mode = 1;                     // 0=STDP imprint, 1=gain/p_post, 2=x_post/trust, 3=live imprint+fuse, 4=clear
    Word clear_first = 0;              // Mode 0: clear `PW24[512..767, 768..1023]` before imprinting
    Word chan_id = 0;                  // Kalman channel slot (0..7)
    Word fields[MAX_KFILT_FIELDS]{};   // {x_prior, p_prior, q_proc, r_meas, z_meas, vel_u} (0..100 each)
    Word sub_op = 0;                   // Mode 1: 0=k_pct, 1=p_post; Mode 2/3: 0=x_post, 1=trust_meas
    Word also_imprint = 0;             // Mode 3: if 1, imprint `fields[0..5]` via STDP first, then predict-update
    Word no_write = 0;                 // Mode 0/3: if 1 (e.g. --ablate freeze), forbid STDP imprinting
    Word pad[3]{};
};

struct alignas(16) NeoKalmanResult {
    SpikeVec kf_spikes{};              // Winning 16-neuron Kalman assembly firing on Blackwell GPU!
    Word best_chan = ~0u;              // Active Kalman channel slot (0..7)
    Word out_f[MAX_KFILT_FIELDS]{};    // Decoded {x_prior, p_prior, q_proc, r_meas, z_meas, vel_u} straight from L2 PW24
    Word x_pred = 0;                   // Predicted state x_prior + vel_u
    Word p_pred = 0;                   // Predicted variance p_prior + q_proc
    Word k_gain_pct = 0;               // Kalman gain % = 100 * p_pred / (p_pred + r_meas)
    Word x_post = 0;                   // Posterior fused state estimate
    Word p_post = 0;                   // Posterior error variance
    Word trust_meas = 0;               // 1 = kftrust (k_gain_pct >= 50), 0 = kfprior
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_chans = 0;             // Active Kalman channels stored in L2 PW24
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW24
    Word spikes_fired = 0;
    Word pad[2]{};
};

// ---------------------------------------------------------------------------
// Kernel 45 (STAGE 8i — PURE-GPU SPIKING LQR / RICCATI FEEDBACK CONTROL &
// CLOSED-LOOP STABILITY CORTEX!):
// Lives on the THIRD FREE `256 x 256` Orthogonal Quadrant of L2 PW24
// (`post rows 0..255`, `pre cols 512..767`, ZERO extra L2 memory & 100% DISJOINT
// from K31 Vision `512..767 x 0..255`+diag, K33 CrossModal `768..1023`, K36 Belief
// `256..511 x 512..767`, K37 Collective `0..255 x 768..1023`+diag, K43 MDP
// `256..511 x 768..1023` & K44 Kalman `512..767 x 768..1023`!):
//   - Stores up to 8 discrete scalar plants `{a_pct, b_pct, q_wt, r_wt, x0, ref_u}`
//     (`0..120` each, representing `a = a_pct/100`, `b = b_pct/100`, cost `Q, R`,
//     state `x0`) as 6 `16x16` plastic L2 tiles (`cols 512..607`) + self-tonic
//     presence tags `+16` on `cols 640..767` via evolved `g.basal_rule` STDP (`Mode 0`)!
//   - Mode 1: Pure-GPU 6-step Discrete Algebraic Riccati Equation (DARE) solver
//     returning optimal feedback gain `K%` (`sub_op=0`) or closed-loop pole `|a - bK|%` (`sub_op=1`)!
//   - Mode 2: Pure-GPU optimal control input `u0 = K*x0` (`sub_op=0`) or closed-loop
//     stability verdict `lqstab` (`acl < 100`) vs `lqunst` (`sub_op=1`)!
//   - Mode 3: Live 1-shot STDP plant imprinting + immediate 6-step Riccati solve!
//   - Mode 4: Clean clear of `PW24[rows 0..255, cols 512..767]` ONLY!
// SURVIVES `--ablate hipp` (Sexquadragintuple 46-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_LQR_PLANTS = 8;
constexpr Word MAX_LQR_FIELDS = 6;

struct alignas(16) NeoLqrQuery {
    Word pathway = 24;                 // Free orthogonal quadrant of L2 PW24 (`rows 0..255`, `cols 512..767`)
    Word mode = 1;                     // 0=STDP imprint, 1=K%/acl%, 2=u0/stable, 3=live imprint+solve, 4=clear
    Word clear_first = 0;              // Mode 0: clear `PW24[0..255, 512..767]` before imprinting
    Word plant_id = 0;                 // Plant slot (0..7)
    Word fields[MAX_LQR_FIELDS]{};     // {a_pct, b_pct, q_wt, r_wt, x0, ref_u} (0..120 each)
    Word sub_op = 0;                   // Mode 1: 0=k_pct, 1=acl_pct; Mode 2/3: 0=u_ctrl, 1=is_stable
    Word also_imprint = 0;             // Mode 3: if 1, imprint `fields[0..5]` via STDP first, then solve DARE
    Word no_write = 0;                 // Mode 0/3: if 1 (e.g. --ablate freeze), forbid STDP imprinting
    Word pad[3]{};
};

struct alignas(16) NeoLqrResult {
    SpikeVec lq_spikes{};              // Winning 16-neuron LQR assembly firing on Blackwell GPU!
    Word best_plant = ~0u;             // Active plant slot (0..7)
    Word out_f[MAX_LQR_FIELDS]{};      // Decoded {a_pct, b_pct, q_wt, r_wt, x0, ref_u} straight from L2 PW24
    Word p_riccati = 0;                // Converged Riccati cost-to-go P*100
    Word k_gain_pct = 0;               // Optimal LQR gain K%
    Word acl_pct = 0;                  // Closed-loop pole |a - b*K|%
    Word u_ctrl = 0;                   // Optimal control action u0 = K*x0
    Word is_stable = 0;                // 1 = lqstab (acl_pct < 100), 0 = lqunst
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_plants = 0;            // Active LQR plants stored in L2 PW24
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW24
    Word spikes_fired = 0;
    Word pad[3]{};
};

// ---------------------------------------------------------------------------
// Kernel 46 (STAGE 8j — 500-DOMAIN MILESTONE! PURE-GPU 256-THREAD SPIKING
// SPECTRAL FFT / HARMONIC DECOMPOSITION CORTEX!):
// Lives on the FOURTH FREE `256 x 256` Orthogonal Quadrant of L2 PW24
// (`post rows 0..255`, `pre cols 256..511`, ZERO extra L2 memory & 100% DISJOINT
// from K31 Vision, K33 CrossModal, K36 Belief, K37 Collective `cols 768..1023`+diag,
// K43 MDP, K44 Kalman & K45 LQR `cols 512..767`!):
//   - Stores up to 8 8-sample signals `s[0..7]` (`0..120` each) as 8 `16x16`
//     plastic L2 tiles (`cols 256..383`) + self-tonic presence tags `+16` on
//     `cols 384..511` via evolved `g.basal_rule` STDP (`Mode 0`)!
//   - Mode 1: Pure-GPU 8-point harmonic DFT DC mean (`sub_op=0`) or fundamental
//     harmonic amplitude `H1` (`sub_op=1`)!
//   - Mode 2: Pure-GPU Nyquist alternating amplitude `H4` (`sub_op=0`) or
//     dominant spectral band `ffhigh`/`fflow` (`sub_op=1`)!
//   - Mode 3: Live 1-shot STDP signal imprinting + immediate 8-point FFT solve!
//   - Mode 4: Clean clear of `PW24[rows 0..255, cols 256..511]` ONLY!
// SURVIVES `--ablate hipp` (Septemquadragintuple 47-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_FFT_SIGNALS = 8;
constexpr Word MAX_FFT_SAMPLES = 8;

struct alignas(16) NeoFftSpecQuery {
    Word pathway = 24;                 // Free orthogonal quadrant of L2 PW24 (`rows 0..255`, `cols 256..511`)
    Word mode = 1;                     // 0=STDP imprint, 1=DC/H1, 2=Nyquist/band, 3=live imprint+FFT, 4=clear
    Word clear_first = 0;              // Mode 0: clear `PW24[0..255, 256..511]` before imprinting
    Word sig_id = 0;                   // Signal slot (0..7)
    Word samples[MAX_FFT_SAMPLES]{};   // 8 time-domain samples s[0..7] (0..120 each)
    Word sub_op = 0;                   // Mode 1: 0=dc_val, 1=h1_amp; Mode 2: 0=nyq_amp, 1=dom_high
    Word also_imprint = 0;             // Mode 3: if 1, imprint `samples[0..7]` via STDP first, then decompose
    Word no_write = 0;                 // Mode 0/3: if 1 (e.g. --ablate freeze), forbid STDP imprinting
    Word pad = 0;
};

struct alignas(16) NeoFftSpecResult {
    SpikeVec ff_spikes{};              // Winning 16-neuron spectral assembly firing on Blackwell GPU!
    Word best_sig = ~0u;               // Active signal slot (0..7)
    Word out_s[MAX_FFT_SAMPLES]{};     // Decoded 8 samples straight from L2 PW24
    Word dc_val = 0;                   // DC mean amplitude
    Word h1_amp = 0;                   // Fundamental k=1 harmonic amplitude
    Word nyq_amp = 0;                  // Nyquist k=4 alternating amplitude
    Word dom_high = 0;                 // 1 = ffhigh (nyq_amp > h1_amp), 0 = fflow
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_sigs = 0;              // Active spectral signals stored in L2 PW24
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW24
    Word spikes_fired = 0;
    Word pad[2]{};
};

// ---------------------------------------------------------------------------
// Kernel 47 (STAGE 8k — 510 DOMAINS! PURE-GPU 256-THREAD SPIKING 4TH-ORDER
// RUNGE-KUTTA `RK4` DYNAMICAL ODE INTEGRATOR & PHASE STABILITY CORTEX!):
// `mind_neo_ode_rk4` on FREE Orthogonal Quadrant of L2 `PW24`
// (`post rows 256..511`, `pre cols 0..255`, ZERO extra L2 memory & 100% DISJOINT
// from K31 Vision, K33 CrossModal, K36 Belief `cols 512..767`+diag `256..511`,
// K37 Collective, K43 MDP, K44 Kalman, K45 LQR & K46 FFT!):
//   - Stores up to 8 ODE systems `dx/dt = (sign_r ? -r : +r)/100 * x + u` with
//     6 parameters `{sign_r, r_mag, u_in, x0, dt_pct, steps}` in `16x16` plastic
//     L2 tiles (`cols 0..95`) + self-tonic presence tags `+16` on `cols 128..255`
//     via evolved `g.basal_rule` STDP (`Mode 0`)!
//   - Mode 1: Pure-GPU 4th-order Runge-Kutta final state `x_final` (`sub_op=0`)
//     or asymptotic equilibrium `x_eq` (`sub_op=1`)!
//   - Mode 2: Pure-GPU phase stability regime `grows` (`0=odestab`, `1=odegrow`)!
//   - Mode 3: Live 1-shot STDP ODE imprinting + immediate RK4 integration!
//   - Mode 4: Clean clear of `PW24[rows 256..511, cols 0..255]` ONLY!
// SURVIVES `--ablate hipp` (Octoquadragintuple 48-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
constexpr Word MAX_ODE_SYSTEMS = 8;
constexpr Word MAX_ODE_PARAMS  = 6;

struct alignas(16) NeoOdeRk4Query {
    Word pathway = 24;                 // Free orthogonal quadrant of L2 PW24 (`rows 256..511`, `cols 0..255`)
    Word mode = 1;                     // 0=STDP imprint, 1=x_final/x_eq, 2=stability, 3=live imprint+RK4, 4=clear
    Word clear_first = 0;              // Mode 0: clear `PW24[256..511, 0..255]` before imprinting
    Word sys_id = 0;                   // ODE system slot (0..7)
    Word params[MAX_ODE_PARAMS]{};     // {sign_r(0=+,1=-), r_mag, u_in, x0, dt_pct, steps}
    Word sub_op = 0;                   // Mode 1: 0=x_final, 1=x_eq
    Word also_imprint = 0;             // Mode 3: if 1, imprint `params[0..5]` via STDP first, then integrate
    Word no_write = 0;                 // Mode 0/3: if 1 (e.g. --ablate freeze), forbid STDP imprinting
    Word pad[3]{};
};

struct alignas(16) NeoOdeRk4Result {
    SpikeVec ode_spikes{};             // Winning 16-neuron ODE attractor assembly firing on Blackwell GPU!
    Word best_sys = ~0u;               // Active ODE slot (0..7)
    Word out_p[MAX_ODE_PARAMS]{};      // Decoded 6 ODE parameters straight from L2 PW24
    Word x_final = 0;                  // Pure-GPU 4th-order Runge-Kutta final state x(T)
    Word x_eq = 0;                     // Asymptotic equilibrium u / |r| when decaying
    Word grows = 0;                    // 0 = odestab (stable decay), 1 = odegrow (unstable growth)
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_sys = 0;               // Active ODE systems stored in L2 PW24
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW24
    Word spikes_fired = 0;
    Word pad[3]{};
};

// Stage 8l (v54 — 520 DOMAINS!): Kernel 48 Pure-GPU 256-Thread Spiking Simplex Linear Programming Optimizer & Dual Shadow-Price Cortex (`mind_neo_simplex_lp`)!
// Lives on FREE orthogonal quadrant of L2 PW24 (`post rows 768..1023`, `pre cols 512..767`, ZERO extra L2 memory — 100% disjoint from K33 `cols 256..511`+diag `768..1023` and all K31–K47!).
constexpr Word MAX_SIMLP_PROBS = 8;    // Up to 8 2-variable LP optimization problems in PW24 rows 768..895 (16 neurons each)
constexpr Word MAX_SIMLP_PARAMS = 8;   // {c1, c2, a1, b1, rhs1, a2, b2, rhs2} for max c1*x+c2*y s.t. a1*x+b1*y<=rhs1, a2*x+b2*y<=rhs2

struct alignas(16) NeoSimplexLpQuery {
    Word mode = 0;                     // 0=STDP imprint LP, 1=opt Z*/x*, 2=dual/corner bind class, 3=live 1-shot STDP imprint+solve, 4=clear
    Word prob_id = 0;                  // Target LP problem slot (0..7)
    Word params[MAX_SIMLP_PARAMS]{};   // {c1, c2, a1, b1, rhs1, a2, b2, rhs2}
    Word sub_op = 0;                   // Mode 1: 0=opt_val Z*, 1=opt_x x*
    Word pathway = 24;                 // Orthogonal FREE quadrant PW24[rows 768..1023, cols 512..767]
    Word clear_first = 0;
    Word also_imprint = 0;
    Word no_write = 0;
    Word pad[3]{};
};

struct alignas(16) NeoSimplexLpResult {
    SpikeVec lp_spikes{};              // Winning 16-neuron LP attractor assembly firing on Blackwell GPU!
    Word best_prob = ~0u;              // Active LP slot (0..7)
    Word out_p[MAX_SIMLP_PARAMS]{};    // Decoded 8 LP parameters straight from L2 PW24
    Word opt_val = 0;                  // Optimal objective Z* = c1*x* + c2*y*
    Word opt_x = 0;                    // Optimal primal decision variable x*
    Word opt_y = 0;                    // Optimal primal decision variable y*
    Word is_corner = 0;                // 0 = sxdual (interior dual-binding vertex), 1 = sxcorn (single-axis corner)
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_probs = 0;             // Active LP problems stored in L2 PW24
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW24
    Word spikes_fired = 0;
    Word pad[3]{};
};

// Stage 8m (v55 — 530 DOMAINS!): Kernel 49 Pure-GPU 256-Thread Spiking PCA / Power-Iteration Eigenvalue & Principal-Axis Cortex (`mind_neo_pca_eigen`)!
// Lives on FREE orthogonal quadrant of L2 PW27 (`post rows 0..255`, `pre cols 256..511`, ZERO extra L2 memory — 100% disjoint from K38 `cols 768..1023`+diag `0..255`, K32, K34, K35!).
constexpr Word MAX_PCA_MATS = 8;       // Up to 8 symmetric 2x2 covariance matrices in PW27 rows 0..127 (16 neurons each)
constexpr Word MAX_PCA_PARAMS = 4;     // {a, b, d, unused} for [[a, b], [b, d]]

struct alignas(16) NeoPcaEigenQuery {
    Word mode = 0;                     // 0=STDP imprint cov, 1=lam1/var_pct, 2=axis/iso class, 3=live 1-shot STDP imprint+solve, 4=clear
    Word mat_id = 0;                   // Target covariance slot (0..7)
    Word params[MAX_PCA_PARAMS]{};     // {a, b, d, 0}
    Word sub_op = 0;                   // Mode 1: 0=dominant lam1, 1=var_pct %
    Word pathway = 27;                 // Orthogonal FREE quadrant PW27[rows 0..255, cols 256..511]
    Word clear_first = 0;
    Word also_imprint = 0;
    Word no_write = 0;
    Word pad[3]{};
};

struct alignas(16) NeoPcaEigenResult {
    SpikeVec pca_spikes{};             // Winning 16-neuron PCA attractor assembly firing on Blackwell GPU!
    Word best_mat = ~0u;               // Active covariance slot (0..7)
    Word out_p[MAX_PCA_PARAMS]{};      // Decoded 4 parameters straight from L2 PW27
    Word lam1 = 0;                     // Dominant eigenvalue via 12-step Rayleigh-quotient power iteration!
    Word lam2 = 0;                     // Minor eigenvalue (tr - lam1)
    Word var_pct = 0;                  // % variance explained by PC1 (lam1 / (lam1+lam2) * 100)
    Word is_iso = 0;                   // 0 = pcaxis (dominant axis, lam1>=2*lam2), 1 = pciso (isotropic)
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_mats = 0;              // Active covariance matrices stored in L2 PW27
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW27
    Word spikes_fired = 0;
    Word pad[3]{};
};

// Stage 8n (v56 — 50-BLACKWELL-KERNEL MILESTONE & 540 DOMAINS!): Kernel 50 Pure-GPU 256-Thread Spiking 1D Heat/Diffusion PDE Finite-Difference Solver Cortex (`mind_neo_pde_solver`)!
// Lives on FREE orthogonal quadrant of L2 PW27 (`post rows 0..255`, `pre cols 512..767`, ZERO extra L2 memory — 100% disjoint from K38 `cols 768..1023`+diag `0..255`, K49 `cols 256..511`, K32, K34, K35!).
constexpr Word MAX_PDE_GRIDS = 8;      // Up to 8 1D spatial diffusion fields in PW27 rows 0..127 (16 neurons each)
constexpr Word MAX_PDE_PARAMS = 8;     // {u0, u1, u2, u3, u4, alpha_pct, steps, 0}

struct alignas(16) NeoPdeSolverQuery {
    Word mode = 0;                     // 0=STDP imprint field, 1=u_mid/u_side, 2=CFL stability class, 3=live 1-shot STDP imprint+solve, 4=clear
    Word grid_id = 0;                  // Target 1D PDE field slot (0..7)
    Word params[MAX_PDE_PARAMS]{};     // {u0, u1, u2, u3, u4, alpha_pct, steps, 0}
    Word sub_op = 0;                   // Mode 1: 0=center u_mid, 1=neighbor u_side
    Word pathway = 27;                 // Orthogonal FREE quadrant PW27[rows 0..255, cols 512..767]
    Word clear_first = 0;
    Word also_imprint = 0;
    Word no_write = 0;
    Word pad[3]{};
};

struct alignas(16) NeoPdeSolverResult {
    SpikeVec pde_spikes{};             // Winning 16-neuron PDE attractor assembly firing on Blackwell GPU!
    Word best_grid = ~0u;              // Active PDE field slot (0..7)
    Word out_p[MAX_PDE_PARAMS]{};      // Decoded 8 parameters straight from L2 PW27
    Word u_mid = 0;                    // Final evolved center temperature u[2]
    Word u_side = 0;                   // Final evolved side temperature u[1]
    Word is_unstab = 0;                // 0 = pdestab (CFL alpha<=50%), 1 = pdeunst (CFL blow-up alpha>50%)
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_grids = 0;             // Active PDE fields stored in L2 PW27
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW27
    Word spikes_fired = 0;
    Word pad[3]{};
};

// Stage 8o (v57 — 51 BLACKWELL KERNELS & 550 DOMAINS!): Kernel 51 Pure-GPU 256-Chain Spiking Metropolis-Hastings MCMC Posterior Sampler Cortex (`mind_neo_mcmc_sampler`)!
// Lives on FREE orthogonal quadrant of L2 PW27 (`post rows 256..511`, `pre cols 0..255`, ZERO extra L2 memory — 100% disjoint from K35 `cols 512..767`+diag `256..511`, K32, K34, K38, K49 & K50!).
constexpr Word MAX_MCMC_DISTS = 8;     // Up to 8 discrete target distributions in PW27 rows 256..383 (16 neurons each)
constexpr Word MAX_MCMC_PARAMS = 8;    // {w0, w1, w2, w3, n_steps, seed_tag, 0, 0}

struct alignas(16) NeoMcmcSamplerQuery {
    Word mode = 0;                     // 0=STDP imprint dist, 1=mode_prob/accept_pct, 2=peaked/flat regime, 3=live 1-shot STDP imprint+sample, 4=clear
    Word dist_id = 0;                  // Target posterior distribution slot (0..7)
    Word params[MAX_MCMC_PARAMS]{};    // {w0, w1, w2, w3, n_steps, seed_tag, 0, 0}
    Word sub_op = 0;                   // Mode 1: 0=mode_prob_pct, 1=accept_pct
    Word pathway = 27;                 // Orthogonal FREE quadrant PW27[rows 256..511, cols 0..255]
    Word clear_first = 0;
    Word also_imprint = 0;
    Word no_write = 0;
    Word pad[3]{};
};

struct alignas(16) NeoMcmcSamplerResult {
    SpikeVec mcmc_spikes{};            // Winning 16-neuron posterior attractor assembly firing on Blackwell GPU!
    Word best_dist = ~0u;              // Active posterior distribution slot (0..7)
    Word out_p[MAX_MCMC_PARAMS]{};     // Decoded 8 parameters straight from L2 PW27
    Word best_state = 0;               // Sampled posterior mode state index (0..3)
    Word mode_prob_pct = 0;            // Empirical MCMC posterior mode probability % across 256 parallel chains!
    Word accept_pct = 0;               // Empirical Metropolis-Hastings acceptance rate %!
    Word is_flat = 0;                  // 0 = mcpeak (peaked unimodal >=50%), 1 = mcflat (flat/multimodal <50%)
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_dists = 0;             // Active target distributions stored in L2 PW27
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW27
    Word spikes_fired = 0;
    Word pad[2]{};
};

// Stage 8p (v58 — 52 BLACKWELL KERNELS, 560 DOMAINS & 100/100 CENTURY VALLEY!): Kernel 52 Pure-GPU 256-Thread Spiking Monte-Carlo Tree Search (MCTS) Lookahead Planner Cortex (`mind_neo_mcts_planner`)!
// Lives on FREE orthogonal quadrant of L2 PW27 (`post rows 256..511`, `pre cols 768..1023`, ZERO extra L2 memory — 100% disjoint from K35 `cols 512..767`+diag `256..511`, K51 `cols 0..255`, K32, K34, K38, K49 & K50!).
constexpr Word MAX_MCTS_TREES = 8;     // Up to 8 stochastic 4-branch decision trees in PW27 rows 256..383 (16 neurons each)
constexpr Word MAX_MCTS_PARAMS = 8;    // {r0, r1, r2, r3, p0_pct, p1_pct, risk_pen, seed_tag}

struct alignas(16) NeoMctsPlannerQuery {
    Word mode = 0;                     // 0=STDP imprint tree, 1=best_val/visit_pct, 2=bold/safe policy, 3=live 1-shot STDP imprint+search, 4=clear
    Word tree_id = 0;                  // Target stochastic decision tree slot (0..7)
    Word params[MAX_MCTS_PARAMS]{};    // {r0, r1, r2, r3, p0_pct, p1_pct, risk_pen, seed_tag}
    Word sub_op = 0;                   // Mode 1: 0=best_val, 1=visit_pct
    Word pathway = 27;                 // Orthogonal FREE quadrant PW27[rows 256..511, cols 768..1023]
    Word clear_first = 0;
    Word also_imprint = 0;
    Word no_write = 0;
    Word pad[3]{};
};

struct alignas(16) NeoMctsPlannerResult {
    SpikeVec mcts_spikes{};            // Winning 16-neuron MCTS planner attractor assembly firing on Blackwell GPU!
    Word best_tree = ~0u;              // Active tree slot (0..7)
    Word out_p[MAX_MCTS_PARAMS]{};     // Decoded 8 parameters straight from L2 PW27
    Word best_act = 0;                 // Most-visited MCTS action branch (0..3) across 4,096 GPU rollouts!
    Word best_val = 0;                 // Empirical MCTS expected value of optimal branch!
    Word visit_pct = 0;                // MCTS visit-share confidence % of optimal branch!
    Word is_bold = 0;                  // 1 = mctbold (risky high-EV branch 0/1), 0 = mctsafe (steady branch 2/3)
    Word eval_val = 0;                 // Primary requested scalar output
    Word active_trees = 0;             // Active decision trees stored in L2 PW27
    Word edits_made = 0;               // Mode 0/3/4: live evolved-LUT STDP synaptic edits in L2 PW27
    Word spikes_fired = 0;
    Word pad[2]{};
};

struct alignas(16) NeoCounterfactResult {
    SpikeVec base_cat_spikes{};
    SpikeVec cf_cat_spikes{};
    Word base_count = 0;
    Word cf_count = 0;
    Word shifted = 0;
    Word overlap_base_cf = 0;
    int cf_max = 0;
    Word spikes_fired = 0;
};

struct alignas(16) NeoAbduceBackQuery {
    SpikeVec effect_sdr{};
    Word pathway = 16;
    Word top_k = 32;
    int min_curr = 6;
    Word pad = 0;
};

struct alignas(16) NeoAbduceBackResult {
    SpikeVec hop1_spikes{};
    SpikeVec hop2_spikes{};
    Word hop1_count = 0;
    Word hop2_count = 0;
    int hop1_max = 0;
    int hop2_max = 0;
    Word spikes_fired = 0;
    Word pad = 0;
};

struct alignas(16) NeoDeduceQuery {
    SpikeVec start_sdr{};
    SpikeVec goal_sdr{};
    SpikeVec block_sdr{};
    Word pathway = 8;
    Word max_hops = 4;
    Word top_k = 40;
    int min_curr = 6;
    Word has_blocker = 0;
    Word pad[3]{};
};

struct alignas(16) NeoDeduceResult {
    SpikeVec hop_spikes[MAX_DEDUCE_STEPS]{};
    Word hop_counts[MAX_DEDUCE_STEPS]{};
    Word proved = 0;
    Word blocked_hit = 0;
    Word proof_hops = 0;
    Word hops_done = 0;
    Word goal_overlap = 0;
    Word spikes_fired = 0;
    Word pad[2]{};
};

struct alignas(16) NeoScanQuery {
    SpikeVec members[MAX_SCAN_MEMBERS]{};
    SpikeVec pred_sdr{};
    Word n_members = 0;
    Word pw_pos = 1;
    Word pw_neg = 2;
    Word pad = 0;
};

struct alignas(16) NeoScanResult {
    Word member_status[MAX_SCAN_MEMBERS]{}; // 2 = VETO/EXCEPTION, 1 = SUPPORTED, 0 = ABSENT
    Word all_hold = 0;
    Word some_hold = 0;
    Word support_count = 0;
    Word veto_count = 0;
    Word witness_idx = ~0u;
    Word counterex_idx = ~0u;
    Word spikes_fired = 0;
    Word pad = 0;
};

struct alignas(16) NeoDreamQuery {
    SpikeVec seed_sdr{};
    Word pw_hop1 = 8;
    Word pw_hop2 = 8;
    Word consolidate_pw = ~0u;
    Word top_k = 40;
    int min_curr = 6;
    Word pad[3]{};
};

struct alignas(16) NeoDreamResult {
    SpikeVec mid_spikes{};
    SpikeVec end_spikes{};
    Word mid_count = 0;
    Word end_count = 0;
    int hop1_max = 0;
    int hop2_max = 0;
    Word dream_edits = 0;
    Word spikes_fired = 0;
};

struct alignas(16) NeoGateQuery {
    SpikeVec subj_sdr{};
    SpikeVec pred_sdr{};
    Word pw_pos = 1;
    Word pw_neg = 2;
    Word pw_tax = 8;
    int support_thresh = 400;
    int veto_thresh = 400;
    Word pad[3]{};
};

struct alignas(16) NeoGateResult {
    int direct_support = 0;
    int inherited_support = 0;
    int veto_conflict = 0;
    Word status = 0; // 0 = NOVEL/SURPRISE, 1 = CONSISTENT, 2 = EXCEPTION/VETOED
    Word spikes_fired = 0;
    Word pad[3]{};
};

struct alignas(16) NeoClusterQuery {
    SpikeVec items[MAX_CLUSTER_ITEMS]{};
    Word n_items = 0;
    Word pw_sem = 1;
    Word pw_tax = 8;
    int min_curr = 4;
};

struct alignas(16) NeoClusterResult {
    int sim_mat[MAX_CLUSTER_ITEMS * MAX_CLUSTER_ITEMS]{};
    int mean_sim[MAX_CLUSTER_ITEMS]{};
    Word outlier_idx = ~0u;
    Word closest_a = ~0u;
    Word closest_b = ~0u;
    int best_pair_sim = 0;
    int outlier_score = 0;
    Word spikes_fired = 0;
};

struct alignas(16) NeoBridgeQuery {
    SpikeVec cue_a{};
    SpikeVec cue_b{};
    Word pathways[MAX_BRIDGE_PWS]{};
    Word n_pws = 1;
    Word top_k = 32;
    int min_curr = 4;
    Word pad = 0;
};

struct alignas(16) NeoBridgeResult {
    SpikeVec bridge_spikes{};
    Word best_pathway = ~0u;
    Word active_count = 0;
    int max_score = 0;
    Word spikes_fired = 0;
};

struct alignas(16) NeoHypothesizeQuery {
    SpikeVec clues[MAX_HYP_CLUES]{};
    SpikeVec cands[MAX_HYP_CANDS]{};
    Word pathway = 0;
    Word n_clues = 0;
    Word n_cands = 0;
    Word pad = 0;
};

struct alignas(16) NeoHypothesizeResult {
    SpikeVec winner_spikes{};
    int scores[MAX_HYP_CANDS]{};
    Word best_cand = ~0u;
    Word second_cand = ~0u;
    int best_score = 0;
    int second_score = 0;
    Word spikes_fired = 0;
    Word pad = 0;
};

struct alignas(16) NeoMultiCueQuery {
    SpikeVec cues[MAX_MULTI_CUES]{};
    Word pathways[MAX_MULTI_CUES]{};
    Word weights[MAX_MULTI_CUES]{};
    SpikeVec inhib_cue{};
    Word n_cues = 0;
    Word inhib_pw = ~0u;
    Word top_k = 32;
    int min_curr = 6;
};

struct alignas(16) NeoMultiCueResult {
    SpikeVec spikes{};
    Word active_count = 0;
    int max_current = 0;
    Word spikes_fired = 0;
};

struct alignas(16) NeoComposeQuery {
    SpikeVec cue_cat{};     // Category cue (e.g., `bird`, `mammal`, `fish`) -> Pathway 3
    SpikeVec cue_pos{};     // Positive ability/feature cue (e.g., `swim`, `fly`, `gills`) -> Pathway 20 (or 3)
    SpikeVec cue_neg{};     // Optional negated ability cue (e.g., `fly` for `penguin`) -> Pathway 21
    Word pw_cat = 3;
    Word pw_pos = 20;
    Word pw_neg = 21;
    Word has_neg = 0;
    Word top_k = 32;
    int min_curr = 4;
    Word pad[2]{};
};

struct alignas(16) NeoComposeResult {
    SpikeVec answer_spikes{};
    Word active_count = 0;
    int max_current = 0;
    Word spikes_fired = 0;
};

struct alignas(16) NeoAnalogyQuery {
    SpikeVec cue_a{};
    SpikeVec cue_b{};
    SpikeVec cue_c{};
    Word candidate_pws[MAX_ANALOGY_PWS]{};
    Word n_pws = 4;
    Word top_k = 32;
    int min_curr = 6;
    Word pad = 0;
};

struct alignas(16) NeoAnalogyResult {
    SpikeVec answer_spikes{};
    Word best_pathway = ~0u;
    Word active_count = 0;
    int match_score = 0;
    Word spikes_fired = 0;
};

struct alignas(16) NeoContrastQuery {
    SpikeVec cue_a{};
    SpikeVec cue_b{};
    Word pos_pathway = 1;
    Word inhib_pathway = 2;
    Word top_k = 32;
    int min_curr = 6;
};

struct alignas(16) NeoContrastResult {
    SpikeVec shared_spikes{};
    SpikeVec only_a_spikes{};
    SpikeVec only_b_spikes{};
    Word shared_count = 0;
    Word only_a_count = 0;
    Word only_b_count = 0;
    Word spikes_fired = 0;
};

struct alignas(16) WmAttendQuery {
    SpikeVec slots[MAX_WM_SLOTS]{};
    Word slot_ids[MAX_WM_SLOTS]{};
    Word slot_recency[MAX_WM_SLOTS]{};
    SpikeVec query{};
    Word n_slots = 0;
    Word min_overlap = 8;
    Word ambig_margin = 4;
    Word pad = 0;
};

struct alignas(16) WmAttendResult {
    Word best_slot = ~0u;
    Word second_slot = ~0u;
    Word best_id = 0;
    Word second_id = 0;
    int best_score = 0;
    int second_score = 0;
    Word ambiguous = 0;
    Word spikes_fired = 0;
};

struct alignas(16) NeoChainQuery {
    SpikeVec init_cue{};
    SpikeVec aux_cues[MAX_THINK_HOPS]{};
    Word pathways[MAX_THINK_HOPS]{};
    Word inhib_pw[MAX_THINK_HOPS]{};
    Word n_hops = 1;
    Word top_k = 40;
    int min_curr = 6;
    Word pad = 0;
};

struct alignas(16) NeoChainResult {
    SpikeVec hop_spikes[MAX_THINK_HOPS]{};
    Word hop_counts[MAX_THINK_HOPS]{};
    Word hops_done = 0;
    Word total_spikes = 0;
    int final_max_curr = 0;
    Word pad = 0;
};

// ---------------------------------------------------------------------------
// Native Blackwell 16x16 Spiking Entorhinal-Hippocampal 2D Place-Cell Map
// ---------------------------------------------------------------------------
constexpr Word MAP_W = 16;
constexpr Word MAP_H = 16;
constexpr Word MAP_CELLS = MAP_W * MAP_H;

struct alignas(16) SpatialMapQuery {
    Word blocked[MAP_CELLS / 32]{}; // 256-bit inhibitory terrain mask (rivers, cliffs, broken bridges)
    Word start_x = 0;
    Word start_y = 0;
    Word goal_x = 0;
    Word goal_y = 0;
};

struct alignas(16) SpatialPathResult {
    Word found = 0;
    Word length = 0;
    Word spikes_fired = 0;
    Word wave_ticks = 0;
    Word path_cells[64]{};
};

struct alignas(16) NeoPair {
    SpikeVec pre{};
    SpikeVec post{};
    Word pathway = 0;
    Word negated = 0;
    Word strength = 1;
    Word pad = 0;
};

struct alignas(16) NeoQueryResult {
    SpikeVec spikes{};
    Word active_count = 0;
    int max_current = 0;
    int mean_current = 0;
    Word spikes_fired = 0;
};

// ---------------------------------------------------------------------------
// Causal World-Model Column
// ---------------------------------------------------------------------------
struct alignas(16) CausalSchema {
    SpikeVec action{};
    SpikeVec target_cat{};
    SpikeVec req_pos{};
    SpikeVec req_neg{};
    SpikeVec tool_req{};
    SpikeVec add_eff{};
    SpikeVec del_eff{};
    SpikeVec produces{};
    Word observations = 0;
    Word confirmations = 0;
    Word contradictions = 0;
    Word energy_cost = 1;
    Word flags = 0;
    Word pad[3]{};
};
static_assert(sizeof(CausalSchema) == 1056);

constexpr Word MAX_PLAN_STEPS = 8;
struct alignas(16) PlanQuery {
    SpikeVec init_state{};
    SpikeVec target_cat{};
    SpikeVec inventory{};
    SpikeVec goal_pos{};
    SpikeVec goal_neg{};
    Word max_depth = 6;
    Word skip_schema = ~0u;
    Word n_schemas = 0;
};

struct alignas(16) PlanResult {
    Word found = 0;
    Word length = 0;
    Word total_cost = 0;
    Word nodes_explored = 0;
    Word spikes_fired = 0;
    Word schema_ids[MAX_PLAN_STEPS]{};
    SpikeVec final_state{};
    SpikeVec missing_precond{};
    Word blocking_step = ~0u;
};

// ---------------------------------------------------------------------------
// Evolutionary Nursery: 10-Pillar Higher-Cognition GPU Evolution!
// ---------------------------------------------------------------------------
constexpr Word EVO_NEURONS = 256;
constexpr Word EVO_WORDS = 8;
constexpr Word EVO_ENGRAMS = 32;
constexpr Word EVO_SCHEMAS = 12;

struct alignas(16) EvoSpike {
    Word w[EVO_WORDS]{};
};

struct alignas(16) EvoEngram {
    EvoSpike role[4]{};
    Half strength = 0;
    Half timestamp = 0;
    Word flags = 0;
};

struct alignas(16) EvoFitness {
    Word total = 0;
    Word foraging = 0;
    Word grounding = 0;      // Pillar 1: Cross-situational novel word grounding
    Word episodic = 0;       // Pillar 2: Multi-hop transitive episodic memory
    Word theory_of_mind = 0; // Pillar 3: Perspective-gated false belief tracking
    Word causal_plan = 0;    // Pillar 4: Curiosity-driven causal discovery + planning
    Word counterfactual = 0; // Pillar 5: Counterfactual intervention prediction
    Word revision = 0;       // Pillar 6: Historical & hidden-switch belief revision
    Word uncertainty = 0;    // Pillar 7: Honest metacognitive uncertainty
    Word logic = 0;          // Pillar 8: Chained conditional (MP + MT) & Disjunctive elimination
    Word social_coop = 0;    // Pillar 9: Split-knowledge two-agent cooperative signaling
    Word consolidate = 0;    // Pillar 10: Ribot systems consolidation (survives hippocampal wipe!)
    Word abduct = 0;         // Pillar 11: Zero-shot feature-based prototype abduction!
    Word vigilance = 0;      // Pillar 12: Epistemic vigilance (trust reliable vs deceptive peer!)
    Word rule_transfer = 0;  // Pillar 13: Universal negative syllogism & variable-bound transfer!
    Word prospective = 0;    // Pillar 14: Prospective memory delayed trigger without false alarms!
    Word sequence = 0;       // Pillar 15: Temporal sequence pattern induction!
    Word consistency = 0;    // Pillar 16: Transitive cycle consistency & contradiction veto!
    Word deontic_norm = 0;   // Pillar 17: Context-gated deontic norm & permission compliance!
    Word intuitive_phys = 0; // Pillar 18: System-1 fast intuitive physical consequence prediction!
    Word sci_hyp = 0;        // Pillar 19: Active scientific experiment design & hypothesis falsification!
    Word game_strat = 0;     // Pillar 20: Strategic game-theory counter-move & reciprocal induction!
    Word narrative_wm = 0;   // Pillar 21: Multi-event narrative working-memory & causal-why binding!
    Word alloc_opt = 0;      // Pillar 22: Resource-constrained non-greedy optimization & scheduling!
    Word bayes_update = 0;   // Pillar 23: Sequential Bayesian posterior belief updating under base rates!
    Word abstr_plan = 0;     // Pillar 24: Multi-stage hierarchical subgoal decomposition!
    Word analogy_xfer = 0;   // Pillar 25: Relational proportional analogy transfer (A:B :: C:?)!
    Word meta_syntax = 0;    // Pillar 26: Live 1-shot syntactic convention binding & reversal!
    Word comp_intersect = 0; // Pillar 27: Compositional conjunction & negation-veto retrieval (A & B & ~C)!
    Word self_model = 0;     // Pillar 28: Metacognitive tool-gap detection & self-initiated acquisition!
    Word sit_model = 0;      // Pillar 29: Multi-event situation-model reading & causal motive binding!
    Word info_gain = 0;      // Pillar 30: Active information-gain hypothesis splitting & optimal inquiry!
    Word open_graph = 0;     // Pillar 31: Open-vocabulary arbitrary-relation graph binding & multi-hop traversal!
    Word hyp_rank = 0;       // Pillar 32: Multi-clue explanatory hypothesis ranking & contradictory revision!
    Word coref_disc = 0;     // Pillar 33: Multi-sentence discourse pronoun coreference & entity tracking!
    Word stoch_plan = 0;     // Pillar 34: Expected-utility probabilistic action planning under uncertainty!
    Word cluster_odd = 0;    // Pillar 35: Unsupervised cortical prototype clustering & odd-one-out discovery!
    Word contract_coal = 0;  // Pillar 36: Multi-agent coalition synergy & Shapley contract enforcement!
    Word ep_sem_gate = 0;    // Pillar 37: Episodic-vs-semantic cortical conflict & exception gating!
    Word policy_mdp = 0;     // Pillar 38: Multi-step Bellman horizon MDP sequential value-iteration planning!
    Word dream_closure = 0;  // Pillar 39: Offline SWR dream replay & 2-hop transitive shortcut consolidation!
    Word market_physics = 0; // Pillar 40: Multi-agent supply-demand market clearing & physical conservation!
    Word quant_counterex = 0;// Pillar 41: Universal quantifier verification & counterexample discovery!
    Word theory_finance = 0; // Pillar 42: Autonomous scientific law induction & quantitative NPV/kinematics!
    Word deduce_chain = 0;   // Pillar 43: Multi-step deductive proof chaining & counterfactual blocking!
    Word graph_opt_bayes = 0;// Pillar 44: Shortest-path graph optimization & sequential Bayesian updating!
    Word abduce_back = 0;    // Pillar 45: Top-down apical backward abduction & 2-hop root-cause tracing!
    Word dynamics_social = 0;// Pillar 46: Ecological resource dynamics, Pearl d-separation & Condorcet choice!
    Word counterfact_img = 0;// Pillar 47: Surgical counterfactual feature-grafting & prototype imagination!
    Word formal_alg_econ = 0;// Pillar 48: Markov steady-state, 2D LP optimization, Ohm circuits & Mendel genetics!
    Word plan_chain = 0;     // Pillar 49: Pure-GPU 2-hop hierarchical goal-to-subgoal plan chaining!
    Word soc_phys_lang = 0;  // Pillar 50: Natural pronoun dialogue, epidemiology, buoyancy, DAGs, Doppler & PD!
    Word causal_sim = 0;     // Pillar 51: Pure-GPU NMDA action-conditioned multi-step causal simulation!
    Word sci_lang_v26 = 0;   // Pillar 52: Multi-paragraph prose, stoichiometry, optics, entropy, LCS, queueing, DH & cake division!
    Word constrain_sat = 0;  // Pillar 53: Pure-GPU multi-attribute constraint satisfaction & inhibitory elimination!
    Word phys_econ_v27 = 0;  // Pillar 54: Causal-because prose, 2D intercept, ideal gas, CRT, bipartite, PV, Nyquist & commons!
    Word induce_rel = 0;     // Pillar 55: Pure-GPU few-shot relational schema induction & 1/2-hop cross-domain transfer!
    Word auto_orb_v28 = 0;   // Pillar 56: Concessive prose, Kepler orbits, 2x2 eigenvalues, Bayes odds, shoelace, DFA, MPC & Hamming!
    Word chain_rule = 0;     // Pillar 57: Pure-GPU compositional multi-pathway rule algebra & cross-domain operator discovery!
    Word rel_ctrl_v29 = 0;   // Pillar 58: Temporal prose, Lorentz relativity, capacitors/RC, Euler, F1, control, logistic & Huffman!
    Word online_schema = 0;  // Pillar 59: Pure-GPU live 1-shot Hebbian online schema write & instant 2/3-hop recall (`mind_neo_online_schema`)!
    Word quant_crypto_v30 = 0; // Pillar 60: Multi-agent/counterfactual prose, photoelectric, pH, RSA, A*, Nash mixed, PID & Bernoulli!
    Word revise_schema = 0;  // Pillar 61: Pure-GPU live LTD inhibitory unlearning & LTP schema overwrite without forgetting (`mind_neo_revise_schema`)!
    Word calc_sys_v31 = 0;   // Pillar 62: Belief-revision prose, calculus, half-life, Hardy-Weinberg, RPN stack, OLS regression, coin-change DP & Kirchhoff!
    Word role_bind = 0;      // Pillar 63: Pure-GPU spiking SDR circular-phase temporal role-binder & unbinder (`mind_neo_role_bind`)!
    Word thermo_arch_v32 = 0;// Pillar 64: Live unseen-word learning, Carnot thermo, Shannon-Hartley, 3D vectors, totient, dB, elasticity & CPU cache AMAT!
    Word seq_predict = 0;    // Pillar 65 (Stage 5!): Pure-GPU spiking predictive-coding sequence role-tagger & template-free syntax (`mind_neo_seq_predict`)!
    Word stage5_sci_v33 = 0; // Pillar 66 (Stage 5!): Compositional story chaining, hydrostatics/Pascal, Stefan-Boltzmann/Wien, 2x2 matrices, combinatorics, TCP BDP, compound interest & Kruskal MST!
    Word neural_math = 0;    // Pillar 67 (Stage 5b!): Pure-GPU parietal spiking bump-attractor math/mod/linear-equation solver (`mind_neo_neural_math`)!
    Word stage5b_sci_v34 = 0;// Pillar 68 (Stage 5b!): Multi-clause unseen prose, Coulomb/capacitor, organ-pipe acoustics, modpow/inv, Amdahl/Gustafson, virtual-mem/TLB, escape/Schwarzschild & 2D metrics!
    Word graph_wave = 0;     // Pillar 69 (Stage 5c!): Pure-GPU axonal-delay spiking shortest-path graph wavefront & GABA_A detouring (`mind_neo_graph_wave`)!
    Word stage5c_sci_v35 = 0;// Pillar 70 (Stage 5c!): Free unseen pronoun/wh-Q discourse, isobaric/entropy, Faraday/xform, Catalan/pigeonhole, 2's comp/gates, Bode damping, CPU SJF & quantum de Broglie/Bohr!
    Word sat_solve = 0;      // Pillar 71 (Stage 5d!): Pure-GPU spiking Boltzmann/Hopfield 3-SAT & forced-backbone constraint attractor (`mind_neo_sat_solve`)!
    Word stage5d_sci_v36 = 0;// Pillar 72 (Stage 5d!): Tier-2 16k HBM lifelong memory, free why/how causal unseen prose, Reynolds, Hamming, 3x3 mat, kinetics, CIDR & beam mechanics!
    Word visual_v1v4 = 0;    // Pillar 73 (Stage 6!): Pure-GPU 16x16 spiking V1/V4 retinal vision, wavefront object counting & spatial relations (`mind_neo_visual_cortex`)!
    Word stage6_sci_v37 = 0; // Pillar 74 (Stage 6!): Autonomous self-teaching reader/curiosity, algorithmic code execution/trace, acoustics, rigid-body, compilers, DNA, crypto & robotics!
    Word audio_a1 = 0;       // Pillar 75 (Stage 6b!): Pure-GPU 16x16 spiking cochlear audio, chord harmony & emotional prosody cortex (`mind_neo_audio_cortex`)!
    Word stage6b_sci_v38 = 0;// Pillar 76 (Stage 6b!): Multi-paragraph textbook causal induction/self-quiz, branching/looping counterfactual bug localization, prosody empathy, 3D solids, graph combinatorics, DSP, organic chem & DB query cost!
    Word crossmodal_stack = 0;// Pillar 77 (Stage 6c!): Pure-GPU cross-modal Vision+Audio binding & hierarchical prefrontal spiking goal stack (`mind_neo_crossmodal_stack`)!
    Word stage6c_sci_v39 = 0;// Pillar 78 (Stage 6c!): Socratic user-goal coaching & epistemic humility, 3D compound contraption physics, tradeoff deliberation, astro, stochastics, power grids, BFT consensus & supply-chain!
    Word dialogue_ctx = 0;   // Pillar 79 (Stage 7a!): Pure-GPU spiking multi-turn dialogue coherence, topic-shift & contradiction cortex (`mind_neo_dialogue_cortex`)!
    Word stage7a_sci_v40 = 0;// Pillar 80 (Stage 7a!): Open free-form multi-turn chat synthesis, pun wit, persona revision, hypothesis tests, Carnot thermo, Master Theorem, immunology & ethical dilemmas!
    Word narrative_met = 0;  // Pillar 81 (Stage 7b!): Pure-GPU spiking conceptual metaphor blending, 2-hop narrative arc & Socratic self-critique cortex (`mind_neo_narrative_metaphor`)!
    Word stage7b_sci_v41 = 0;// Pillar 82 (Stage 7b!): Unscripted universal-fallback chat, autonomous scientific research loop, compositional stories, LP shadow prices, spectral harmonics, Hardy-Weinberg, Diffie-Hellman & trophic cascades!
    Word belief_deb = 0;     // Pillar 83 (Stage 7c!): Pure-GPU spiking belief-graph, evidence-weighting & multi-agent scientific debate cortex (`mind_neo_belief_debate`)!
    Word stage7c_sci_v42 = 0;// Pillar 84 (Stage 7c — 400 Domains!): Multi-facet unscripted chat, scientific peer review, inductive proofs, Boltzmann statmech, VCG mechanism design, LIF neurobio, LQR control & Hohmann astrodynamics!
    Word coll_disc = 0;      // Pillar 85 (Stage 8 Crossing!): Pure-GPU multi-mind collective scientific discovery, consensus voting & compositional theorem prover (`mind_neo_collective_discovery`)!
    Word stage8_sci_v43 = 0; // Pillar 86 (Stage 8 — 410 Domains!): Socratic curiosity follow-up, spec-to-code synthesis, quantum gates, channel coding, 1D heat PDE, Shapley coalitions, epidemic optimal control & multi-lab consensus!
    Word self_res = 0;       // Pillar 87 (Stage 8b!): Pure-GPU continual self-research discriminative experiment design & counterfactual code patching (`mind_neo_self_research`)!
    Word stage8b_sci_v44 = 0;// Pillar 88 (Stage 8b — 420 Domains!): Multi-sentence unscripted discourse, Kalman filtering, Bayes nets, Poiseuille fluid dynamics, Schnorr ZK proofs, robotics trajectory, compiler LICM & Taylor-rule macroeconomics!
    Word sym_calc = 0;       // Pillar 89 (Stage 8c!): Pure-GPU neural symbolic algebra & polynomial calculus cortex (`mind_neo_symbolic_calculus`)!
    Word stage8c_sci_v45 = 0;// Pillar 90 (Stage 8c — 430 Domains!): Live spontaneous concept teaching/story/why, KL divergence, Bode phase margin, quantum tunneling, special relativity, Raft consensus, Chaitin register coloring & Hawk-Dove ESS!
    Word prog_ast = 0;       // Pillar 91 (Stage 8d!): Pure-GPU AST program synthesis, type verification & symbolic execution cortex (`mind_neo_program_ast`)!
    Word stage8d_sci_v46 = 0;// Pillar 92 (Stage 8d — 440 Domains!): Free-form counterfactual imagination/debate/zero-keyword chat, Merkle trees, Tsiolkovsky rockets, Little's Law queueing, Michaelis-Menten enzymes, quorum R+W>N, decibel SNR & Chomsky CFG!
    Word lin_alg = 0;        // Pillar 93 (Stage 8e!): Pure-GPU neural linear algebra, matvec & Cramer/Gaussian linear solver cortex (`mind_neo_linear_algebra`)!
    Word stage8e_sci_v47 = 0;// Pillar 94 (Stage 8e — 450 Domains!): Multi-turn story co-writing/self-questioning/zero-keyword contrast, RSA keygen, aerodynamic lift, cache geometry, Kepler orbits, ADC SQNR, Nernst EMF & Cournot duopoly!
    Word bayes_net = 0;      // Pillar 95 (Stage 8f!): Pure-GPU spiking probabilistic Bayes-net & expected-utility decision inference cortex (`mind_neo_bayes_net` on final PW13 Q3)!
    Word stage8f_sci_v48 = 0;// Pillar 96 (Stage 8f — 460 Domains!): Socratic self-critique/spontaneous analogy/zero-keyword why, blackbody radiation, Hamming ECC, thin-lens optics, amortized growth, harmonic pendulum, DNA melting & Solow growth!
    Word mdp_plan = 0;       // Pillar 97 (Stage 8g!): Pure-GPU spiking Bellman value-iteration & stochastic MDP policy planner (`mind_neo_mdp_planner` on free PW24 Q)!
    Word stage8g_sci_v49 = 0;// Pillar 98 (Stage 8g — 470 Domains!): Zero-template open user goal planning/synthesis/free narrative, B-tree DB, bandgap physics, 3D frustum, seismology, loop unrolling, fiber optics & buck converter!
    Word kalman_flt = 0;     // Pillar 99 (Stage 8h!): Pure-GPU spiking Kalman filter & dynamical state-space estimation cortex (`mind_neo_kalman_filter` on free PW24 Q)!
    Word stage8h_sci_v50 = 0;// Pillar 100 (Stage 8h — 100-PILLAR / 480-DOMAIN MILESTONE!): Zero-template hypothesis testing/assumption critique, branch pred, albedo, Bloom filter, cardiac output, password entropy, Sabine acoustics & convex polygons!
    Word lqr_ctrl = 0;       // Pillar 101 (Stage 8i!): Pure-GPU spiking LQR / Riccati feedback control & stability cortex (`mind_neo_lqr_control` on free PW24 Q)!
    Word stage8i_sci_v51 = 0;// Pillar 102 (Stage 8i — 490 DOMAINS!): Multi-turn topic bridging/epistemic justification/pronoun follow-up, image compression, barometric lapse, consistent hashing, Euler buckling, F1 score, Manning hydrology & page tables!
    Word fft_spec = 0;       // Pillar 103 (Stage 8j!): Pure-GPU spiking 8-point spectral FFT / harmonic cortex (`mind_neo_fft_spectrum` on free PW24 Q)!
    Word stage8j_sci_v52 = 0;// Pillar 104 (Stage 8j — 500-DOMAIN MILESTONE!): Zero-template challenge self-correction/curiosity/dual-topic pronoun chat, Shannon capacity, Froude wave, Bezier curves, RC filter, softmax margin, Hohmann transfer & ECC security!
    Word ode_rk4 = 0;        // Pillar 105 (Stage 8k!): Pure-GPU spiking 4th-order Runge-Kutta (`RK4`) dynamical ODE integrator & phase stability cortex (`mind_neo_ode_rk4` on free PW24 Q)!
    Word stage8k_sci_v53 = 0;// Pillar 106 (Stage 8k — 510 DOMAINS!): Zero-template hypothetical counterfactual reasoning/spontaneous elaboration/anaphoric switch, PV solar, M/M/1 queueing, glider L/D, pharmacokinetics, ConvNet geometry, seismology & battery packs!
    Word simplex_lp = 0;     // Pillar 107 (Stage 8l!): Pure-GPU spiking Simplex Linear Programming optimizer & dual shadow-price cortex (`mind_neo_simplex_lp` on free PW24 Q)!
    Word stage8l_sci_v54 = 0;// Pillar 108 (Stage 8l — 520 DOMAINS!): Zero-template multi-hop syllogistic proof chaining/autonomous recap/zero-keyword compare-both, capacitor banks, Darcy-Weisbach pipes, Gustafson speedup, Rankine turbines, birthday hash collision, Beer-Lambert absorbance & put-call parity!
    Word pca_eigen = 0;      // Pillar 109 (Stage 8m!): Pure-GPU spiking PCA / Rayleigh power-iteration eigenvalue & principal-axis cortex (`mind_neo_pca_eigen` on free PW27 Q)!
    Word stage8m_sci_v55 = 0;// Pillar 110 (Stage 8m — 530 DOMAINS!): Zero-template contrastive divergence/autonomous generalization/zero-keyword exception anaphora, Schwarzschild black holes, Hooke springs, TCP cwnd throughput, Henderson-Hasselbalch buffers, Phong shading, buoyant isostasy & Sharpe ratio!
    Word pde_solver = 0;     // Pillar 111 (Stage 8n — 50-KERNEL MILESTONE!): Pure-GPU spiking 1D Heat/Diffusion PDE finite-difference solver cortex (`mind_neo_pde_solver` on free PW27 Q)!
    Word stage8n_sci_v56 = 0;// Pillar 112 (Stage 8n — 540 DOMAINS!): Zero-template cross-category deduction/shared-root abstraction/zero-keyword dual contrast, de Broglie waves, tail latency SLA, ion thrusters, Hill cooperative binding, beam bending stress, arithmetic coding & Taylor rule!
    Word mcmc_sampler = 0;   // Pillar 113 (Stage 8o — 51 KERNELS!): Pure-GPU 256-chain spiking Metropolis-Hastings MCMC posterior sampler cortex (`mind_neo_mcmc_sampler` on free PW27 Q)!
    Word stage8o_sci_v57 = 0;// Pillar 114 (Stage 8o — 550 DOMAINS!): Zero-template 3-entity GPU clustering/spontaneous sibling expansion/zero-keyword capability anaphora, Fermi-Dirac bands, Bernoulli dynamic head, Shamir secret sharing, Nernst membrane drive, IK workspace, Lotka-Volterra & Roofline!
    Word mcts_planner = 0;   // Pillar 115 (Stage 8p — 52 KERNELS!): Pure-GPU 4,096-rollout spiking Monte-Carlo Tree Search (MCTS) lookahead planner cortex (`mind_neo_mcts_planner` on free PW27 Q)!
    Word stage8p_sci_v58 = 0;// Pillar 116 (Stage 8p — 560 DOMAINS & 100/100 CENTURY VALLEY!): 100% zero-command-keyword natural multi-turn human dialogue, Grover search, PCM bitrate, Stefan-Boltzmann, PBFT quorum, Young's modulus, Vis-Viva & 2nd-order damping!
    Word spikes = 0;
    Word edits = 0;
};

constexpr Word EVO_MAX_PER_WORLD = 1860; // 1828 + 16 + 16 = 1860 pts/world across 116 cognitive pillars!

struct EvoConfig {
    Word population = 512;
    Word generations = 64;
    Word worlds_per_gen = 4;
    Word seed = 20261124;
    Word drift = 0;
    Word freeze_plasticity = 0;
    Word lesion_apical = 0;
    Word lesion_hipp = 0;
    Word lesion_causal = 0;
    Word shuffle_feedback = 0;
};

struct EvoGenSummary {
    Word generation = 0;
    Word population = 0;
    float mean_total = 0, best_total = 0;
    float mean_ground = 0, mean_episodic = 0, mean_tom = 0, mean_causal = 0, mean_counter = 0, mean_rev = 0,
          mean_uncert = 0, mean_logic = 0, mean_social = 0, mean_cons = 0, mean_abduct = 0, mean_vigil = 0,
          mean_rule = 0, mean_prosp = 0, mean_seq = 0, mean_consist = 0, mean_norm = 0, mean_phys = 0,
          mean_sci = 0, mean_game = 0, mean_narr = 0, mean_opt = 0, mean_bayes = 0, mean_hier = 0,
          mean_analogy = 0, mean_msyn = 0, mean_comp = 0, mean_selfm = 0, mean_sit = 0, mean_info = 0,
          mean_ograph = 0, mean_hrank = 0, mean_coref = 0, mean_stoch = 0, mean_clust = 0, mean_coal = 0,
          mean_gate = 0, mean_mdp = 0, mean_dream = 0, mean_mkt = 0, mean_quant = 0, mean_thfin = 0,
          mean_deduce = 0, mean_gbayes = 0, mean_abdbk = 0, mean_dynsoc = 0, mean_cfimg = 0, mean_algec = 0,
          mean_plan = 0, mean_socpl = 0, mean_csim = 0, mean_sciv26 = 0, mean_csat = 0, mean_pev27 = 0,
          mean_ind = 0, mean_aor28 = 0, mean_chrl = 0, mean_rc29 = 0, mean_osch = 0, mean_qc30 = 0,
          mean_rvsc = 0, mean_cs31 = 0, mean_rbnd = 0, mean_ta32 = 0, mean_sqpr = 0, mean_s533 = 0, mean_nmat = 0, mean_s534 = 0, mean_gwav = 0, mean_s535 = 0, mean_sats = 0, mean_s536 = 0, mean_visv = 0, mean_s637 = 0, mean_audv = 0, mean_s638 = 0, mean_xmod = 0, mean_s639 = 0, mean_dlgc = 0, mean_s740 = 0, mean_nmet = 0, mean_s741 = 0, mean_bdeb = 0, mean_s742 = 0, mean_cdis = 0, mean_s843 = 0, mean_sres = 0, mean_s844 = 0, mean_symc = 0, mean_s845 = 0, mean_past = 0, mean_s846 = 0, mean_lalg = 0, mean_s847 = 0, mean_bnet = 0, mean_s848 = 0, mean_mdpp = 0, mean_s849 = 0, mean_kflt = 0, mean_s850 = 0, mean_lqrc = 0, mean_s851 = 0, mean_ffts = 0, mean_s852 = 0, mean_odrk = 0, mean_s853 = 0, mean_sxlp = 0, mean_s854 = 0, mean_pcae = 0, mean_s855 = 0, mean_pdes = 0, mean_s856 = 0, mean_mcmcs = 0, mean_s857 = 0, mean_mctsp = 0, mean_s858 = 0;
    float best_ground = 0, best_episodic = 0, best_tom = 0, best_causal = 0, best_counter = 0, best_rev = 0,
          best_uncert = 0, best_logic = 0, best_social = 0, best_cons = 0, best_abduct = 0, best_vigil = 0,
          best_rule = 0, best_prosp = 0, best_seq = 0, best_consist = 0, best_norm = 0, best_phys = 0,
          best_sci = 0, best_game = 0, best_narr = 0, best_opt = 0, best_bayes = 0, best_hier = 0,
          best_analogy = 0, best_msyn = 0, best_comp = 0, best_selfm = 0, best_sit = 0, best_info = 0,
          best_ograph = 0, best_hrank = 0, best_coref = 0, best_stoch = 0, best_clust = 0, best_coal = 0,
          best_gate = 0, best_mdp = 0, best_dream = 0, best_mkt = 0, best_quant = 0, best_thfin = 0,
          best_deduce = 0, best_gbayes = 0, best_abdbk = 0, best_dynsoc = 0, best_cfimg = 0, best_algec = 0,
          best_plan = 0, best_socpl = 0, best_csim = 0, best_sciv26 = 0, best_csat = 0, best_pev27 = 0,
          best_ind = 0, best_aor28 = 0, best_chrl = 0, best_rc29 = 0, best_osch = 0, best_qc30 = 0,
          best_rvsc = 0, best_cs31 = 0, best_rbnd = 0, best_ta32 = 0, best_sqpr = 0, best_s533 = 0, best_nmat = 0, best_s534 = 0, best_gwav = 0, best_s535 = 0, best_sats = 0, best_s536 = 0, best_visv = 0, best_s637 = 0, best_audv = 0, best_s638 = 0, best_xmod = 0, best_s639 = 0, best_dlgc = 0, best_s740 = 0, best_nmet = 0, best_s741 = 0, best_bdeb = 0, best_s742 = 0, best_cdis = 0, best_s843 = 0, best_sres = 0, best_s844 = 0, best_symc = 0, best_s845 = 0, best_past = 0, best_s846 = 0, best_lalg = 0, best_s847 = 0, best_bnet = 0, best_s848 = 0, best_mdpp = 0, best_s849 = 0, best_kflt = 0, best_s850 = 0, best_lqrc = 0, best_s851 = 0, best_ffts = 0, best_s852 = 0, best_odrk = 0, best_s853 = 0, best_sxlp = 0, best_s854 = 0, best_pcae = 0, best_s855 = 0, best_pdes = 0, best_s856 = 0, best_mcmcs = 0, best_s857 = 0, best_mctsp = 0, best_s858 = 0;
    Count total_spikes = 0, total_edits = 0;
    double gpu_ms = 0;
};

// ---------------------------------------------------------------------------
// Full Raised Spiking Mind Capacity Constants (Stage 5d: 8,192 Lexicon + 16,384 Tier-2 HBM Lifelong Engrams!)
// ---------------------------------------------------------------------------
constexpr Word MAX_CONCEPTS = 8192;
constexpr Word HIPP_L2_HOT = 4096;  // 3,264 KiB hot working-memory tier pinned inside Blackwell's 32-MiB L2 cache!
constexpr Word MAX_ENGRAMS = 16384; // 13,056 KiB total Tier-2 HBM lifelong episodic archive (4x beyond 4,096 L2 ceiling!)
constexpr Word MAX_CAUSAL = 256;
constexpr Word MAX_CONSTRUCTIONS = 8192;
constexpr Word MAX_PAT_TOKENS = 16;
constexpr Word MAX_WORD_CHARS = 32;

constexpr Word CAT_NONE = 0;
constexpr Word CAT_ENTITY = 1u << 0;
constexpr Word CAT_AGENT = 1u << 1;
constexpr Word CAT_PLACE = 1u << 2;
constexpr Word CAT_CONTAINER = 1u << 3;
constexpr Word CAT_RELATION = 1u << 4;
constexpr Word CAT_PROPERTY = 1u << 5;
constexpr Word CAT_ACTION = 1u << 6;
constexpr Word CAT_CATEGORY = 1u << 7;
constexpr Word CAT_TOOL = 1u << 8;
constexpr Word CAT_NUMBER = 1u << 9;
constexpr Word CAT_FUNC = 1u << 10;
constexpr Word CAT_PRONOUN = 1u << 11;
constexpr Word CAT_QUANTIFIER = 1u << 12;
constexpr Word CAT_COMPARE = 1u << 13;

enum IntentKind : Word {
    INT_NONE = 0,
    INT_ASSERT_REL = 1,
    INT_ASSERT_PROP = 2,
    INT_ASSERT_ISA = 3,
    INT_ASSERT_CAN = 4,
    INT_ASSERT_MOVE = 5,
    INT_ASSERT_GIVE = 6,
    INT_ASSERT_HAVE = 7,
    INT_ASSERT_OBSERVER = 8,
    INT_ASSERT_BELIEF = 9,
    INT_ASSERT_CAUSE = 10,
    INT_ASSERT_COMPARE = 11,
    INT_ASSERT_META_REL = 12,
    INT_ASSERT_GOAL = 13,
    INT_ASK_WHERE = 14,
    INT_ASK_WHAT_IN = 15,
    INT_ASK_WHO_DID = 16,
    INT_ASK_WHAT_IS = 17,
    INT_ASK_YESNO_ISA = 18,
    INT_ASK_YESNO_PROP = 19,
    INT_ASK_YESNO_CAN = 20,
    INT_ASK_YESNO_REL = 21,
    INT_ASK_BELIEF = 22,
    INT_ASK_BELIEF2 = 23,
    INT_ASK_PLAN = 24,
    INT_ASK_WHY = 25,
    INT_ASK_WHATIF = 26,
    INT_ASK_ANALOGY = 27,
    INT_ASK_COMPARE = 28,
    INT_ASK_HOW_MANY = 29,
    INT_ASK_HISTORY = 30,
    INT_ASK_HOW_KNOW = 31,
    INT_ASK_SELF = 32,
    INT_ASK_COMPARE_BOTH = 33,
    INT_ASK_REL_BETWEEN = 34,
    INT_CMD_ACT = 35,
    INT_SOCIAL_GREET = 36,
    INT_CORRECT_NO = 37,
    INT_ASK_WHY_NOT = 38,
    INT_DEFINE_WORD = 39,
    // --- NEW HIGHER-COGNITION INTENTS ---
    INT_ASSERT_IF_THEN = 40,   // If [SUBJ] is [PROP] then [OBJ] is [LOC] (chained MP & MT!)
    INT_ASSERT_EITHER_OR = 41, // [SUBJ] is either in [OBJ] or in [LOC] (disjunctive syllogism!)
    INT_ASSERT_GAIN = 42,      // [PERSP] found/gained [QTY] [OBJ] (+N conservation!)
    INT_ASSERT_LOSE = 43,      // [PERSP] ate/lost/used [QTY] [OBJ] (-N conservation!)
    INT_ASK_SUM = 44,          // How many [OBJ] altogether / in total?
    INT_ASK_SUPERLATIVE = 45,  // Which/who is the most/least [PROP]?
    INT_ASK_SUPPOSE = 46,      // Suppose [SUBJ] is [PROP] -> counterfactual world branch!
    INT_ASK_CURIOSITY = 47,    // What are you curious about? / Ask me a question!
    INT_ASK_LEARNED = 48,      // What did you learn?
    INT_ASK_ALL = 49,          // Universal quantification ("can all birds fly?", "are all birds animals?")
    INT_ASK_SOME = 50,         // Existential quantification ("can some birds swim?")
    INT_ASK_ELLIPSIS = 51,     // Follow-up ellipsis ("and penguin?", "what about fox?")
    INT_ASK_CAPABILITIES = 52, // Self-introspection ("what can you do?")
    INT_ASSERT_OBSERVER2 = 53, // Compound observer presence ("mira and fox are here")
    // --- v4 HIGHER-COGNITION INTENTS ---
    INT_ASK_ARITH = 54,        // Parietal mental arithmetic ("what is 7 plus 5?")
    INT_ASK_PATH = 55,         // Spatial graph navigation ("how to get from meadow to cave?")
    INT_ASK_EXEMPLAR = 56,     // Category exemplar enumeration ("name a bird", "list all fish")
    INT_ASSERT_NAME = 57,      // Personal identity ("my name is ben")
    INT_ASK_NAME = 58,         // Identity recall ("what is my name?")
    INT_ASSERT_DESIRE = 59,    // Social goals/desires ("mira wants cooked stew")
    INT_ASK_DESIRE = 60,       // Social goal recall & goal-to-plan ("what does mira want?", "how can mira get cooked stew?")
    INT_ASK_LOSE_TOOL = 61,    // Counterfactual tool-loss causal relevance ("suppose we lose flint how can stew become cooked?")
    // --- v5 DEEP NEOCORTEX, ABDUCTION, TRUST, CSP & INVERSE MATH INTENTS ---
    INT_ASK_WHAT_KIND = 62,    // Zero-shot feature-based neocortical prototype abduction ("what kind of animal is kestrel?")
    INT_ASK_DIAGNOSE = 63,     // Multi-evidence Bayesian abductive diagnosis ("why is ground wet?")
    INT_ASSERT_TESTIMONY = 64, // Attributed peer testimony for epistemic vigilance ("mira said gem is in cellar")
    INT_ASK_TRUST = 65,        // Epistemic trust comparison ("who do you trust more mira or fox?")
    INT_ASSERT_CSP = 66,       // Bipartite exclusivity constraint ("each of coin key gem is in red_box blue_box green_box")
    INT_ASK_STARTED = 67,      // Inverse algebraic word problem ("how many berry did leo start with?")
    INT_ASSERT_RATE = 68,      // Proportional unit rate ("1 basket holds 4 berry")
    INT_ASK_PROPORTION = 69,   // Proportional scaling ("how many berry in 3 basket?")
    INT_ASK_SUMMARY = 70,      // Rich multi-clause summary ("tell me everything about bird")
    // --- v6 HUMAN-LEVEL HIGHER-COGNITION INTENTS ---
    INT_ASSERT_NO_CAT = 71,    // Universal negative syllogism ("no fish can walk", "no reptile has fur")
    INT_ASSERT_UNIV_COND = 72, // Variable-bound universal conditional ("if any animal is hungry then it is seeking")
    INT_ASSERT_REMINDER = 73,  // Prospective memory trigger ("when fox returns remind me about gem")
    INT_ASK_REMINDERS = 74,    // Introspect active prospective reminders ("what are your reminders?")
    INT_ASSERT_BEFORE = 75,    // Explicit chronological timeline ("dawn happened before noon")
    INT_ASK_BEFORE = 76,       // Chronological query ("did dawn happen before dusk?", "what happened first/last?")
    INT_ASSERT_PRICE = 77,     // Unit price ("1 apple costs 3 coin")
    INT_ASK_COST = 78,         // Multi-item total cost ("how much for 2 apple and 1 loaf?")
    INT_ASSERT_SUM_DIFF = 79,  // 2-variable linear system ("alice and bob have 14 coin total" / "alice has 4 more coin than bob")
    INT_ASK_FAIR_SHARE = 80,   // Social fairness sharing ("how many berry should mira give fox to be fair?")
    INT_ASK_WOULD_LIKE = 81,   // Preference empathy ("would mira like berry?")
    INT_ASSERT_CAUSAL_STEP = 82, // 1-shot verbal causal law with precondition ("smelt on ore requires dust and causes ingot")
    INT_ASK_SIMILAR = 83,      // 4M-synapse GPU Neocortical semantic manifold similarity ("what is similar to kestrel?")
    // --- v7 OPEN-ENDED REASONING, CONJUNCTIONS, META-EPISTEMICS & PARAPHRASES ---
    INT_ASK_NUM_CMP = 84,      // Quantitative numeric comparison ("is 24 greater than 19?", "which number is bigger 31 or 19?")
    INT_ASK_FRACTION = 85,     // Parietal fractional/multiplicative scaling ("what is half of 18?", "what is twice 13?")
    INT_ASK_NEED_TOOL = 86,    // Causal necessity introspection ("do we need flint to cook stew?")
    INT_ASK_KNOWS_WHERE = 87,  // Epistemic knowledge vs false-belief meta-reasoning ("does fox know where gem is?")
    INT_ASSERT_CONJ_RULE = 88, // True conjunctive AND-gate rule ("if pump is on and valve is open then tank is full")
    // --- v8 6M NEOCORTEX, 2D PLACE CELLS, RIDDLES, SEQUENCES, CYCLES & PHYSICS ---
    INT_ASK_RIDDLE = 89,       // Multi-constraint entity riddle ("what bird cannot fly?", "what mammal can fly?")
    INT_ASK_SEQ_NEXT = 90,     // Sequence pattern induction ("what comes next after 2 4 6 8?")
    INT_ASK_MIDDLE = 91,       // Transitive median & full sorting ("who is in the middle of alice bob carol?", "rank ...")
    INT_ASK_DISTANCE = 92,     // Physics word problem ("speed 60 for 3 hours")
    INT_ASK_REMAINDER = 93,    // Modulo / remainder ("what is remainder of 17 divided_by 5")
    INT_ASK_BUDGET = 94,       // Multi-step budgeting ("ben has 20 coin and buys 2 apple")
    INT_ASK_OPPOSITE = 95,     // Semantic opposition / inverse ("what is the opposite of hot?")
    INT_ASK_NAV_2D = 96,       // GPU 16x16 spiking place-cell 2D map navigation ("navigate from camp to mine")
    INT_ASSERT_BRIDGE = 97,    // Dynamic 2D terrain obstacle ("bridge is broken" / "bridge is repaired")
    INT_ASK_OPINION = 98,      // Social trust opinion ("what do you think about fox?")
    INT_ASK_STORY = 99,        // Autobiographical multi-event narrative ("tell me a story")
    INT_ASK_CYCLE_NEXT = 100,  // Cyclical time / season ordering ("what comes after winter?")
    INT_ASSERT_CYCLE = 101,    // Cyclic ordering assertion ("summer comes after spring")
    // --- v9 8M NEOCORTEX, FORMAL LOGIC, MORAL ToM, DEONTICS, PHYSICS & SET ALGEBRA ---
    INT_ASSERT_IFF = 102,        // Biconditional rule ("lamp is bright iff switch is up")
    INT_ASK_PEMDAS = 103,        // Precedence order-of-operations arithmetic ("what is 3 plus 4 times 5?")
    INT_ASK_PERCENT = 104,       // Percentage computation ("what is 25 percent of 80?")
    INT_ASSERT_MORAL = 105,      // Intentional vs accidental harm ("fox broke cup on purpose" vs "by accident")
    INT_ASK_MORAL = 106,         // Moral culpability judgment ("who was wrong fox or mira?")
    INT_ASSERT_PROMISE = 107,    // Social commitment ("mira promised to bring key" / "brought" / "broke promise")
    INT_ASK_PROMISE = 108,       // Promise fulfillment verification ("did mira keep promise?")
    INT_ASSERT_NORM = 109,       // Deontic norm ("nobody may touch fire without glove" / "everyone must wear helmet")
    INT_ASK_NORM = 110,          // Deontic permission/obligation query ("may sam touch fire?", "must sam wear helmet?")
    INT_ASK_PREDICT_EFFECT = 111,// System-1 Pathway-7 & multi-action forward causal effect prediction!
    INT_ASK_BALANCE = 112,       // Balance-scale common-weight cancellation & comparison!
    INT_ASK_OVERFLOW = 113,      // Physical container capacity overflow prediction!
    INT_ASK_INCL_EXCL = 114,     // Set algebra inclusion-exclusion ("10 fly, 4 swim, 2 both -> 12")
    INT_ASK_PIGEONHOLE = 115,    // Pigeonhole principle deduction ("5 gem in 4 box -> yes")
    INT_ASK_AGE_DIFF = 116,      // Invariant age-difference algebra ("alice 12, bob 8 -> when bob 15, alice 19")
    INT_ASK_PARITY = 117,        // Integer parity ("is 14 even or odd?")
    // --- v10 12M RECURRENT NEOCORTEX, SCIENTIFIC METHOD, KINSHIP, GAME THEORY, AFFECT, PRAGMATICS & METAPHOR ---
    INT_ASSERT_HYP_TEST = 118,   // Scientific test law ("acidic turns litmus red")
    INT_ASK_DESIGN_EXP = 119,    // Active discriminating experiment design ("how can we test if crystal is acidic?")
    INT_ASSERT_EXP_OBS = 120,    // Experimental outcome observation ("litmus on crystal did not turn red")
    INT_ASK_FAIR_TEST = 121,     // Controlled vs confounded experiment critique
    INT_ASK_KINSHIP = 122,       // Recursive family-tree graph reasoning (grandparent, sibling, ancestor)
    INT_ASSERT_BEATS = 123,      // Strategic dominance relation ("paper beats rock")
    INT_ASK_COUNTER_MOVE = 124,  // Dominant counter-move selection ("if mira plays rock what should i play?")
    INT_ASK_MINIMAX = 125,       // Zero-sum worst-case maximin choice
    INT_ASK_TIT_FOR_TAT = 126,   // Iterated cooperation/defection reciprocity
    INT_ASSERT_AFFECT_EV = 127,  // Emotional event ("mira lost gem", "wren found gold", "wolf chased mira")
    INT_ASK_FEEL = 128,          // Pathway-9 GPU Neocortical affective appraisal ("how does mira feel?")
    INT_ASK_EMPATHY_HELP = 129,  // Prosocial empathetic action recommendation ("how can we help mira?")
    INT_ASK_PRAGMATIC = 130,     // Gricean scalar implicature, indirect speech acts & irony detection
    INT_ASSERT_METAPHOR = 131,   // Structural cross-domain metaphor ("brain is to body as elder is to village")
    INT_ASK_METAPHOR = 132,      // Structural metaphor mapping query ("what is the brain of village?")
    INT_ASK_EXPECTED_VAL = 133,  // Probabilistic expected value comparison & compound probability
    INT_ASK_PROVE = 134,         // Full multi-step explicit deductive proof chain ("prove why kestrel is living")
    INT_ASK_WHY_CHANGED = 135,   // Belief-revision transparency ("why did you change mind about crystal?")
    INT_ASK_THINK_CHAIN = 136,   // Pure GPU multi-hop recurrent cortical spreading (`mind_neo_think_chain`)!
    // --- v11 16M NEOCORTEX, WM ATTENTION GATE, NARRATIVE, ALGEBRA, OPTIMIZATION, SCHEMA XFER & COMMON KNOWLEDGE ---
    INT_ASSERT_STORY_EV = 137,   // Multi-event narrative ("elena took lantern to cavern", "elena sang lullaby so dragon slept")
    INT_ASK_STORY_WHY = 138,     // Narrative causal-why & counterfactual ("why did dragon sleep?")
    INT_ASK_SOLVE_EQ = 139,      // Linear algebraic equation solving ("solve 2 times x plus 3 equals 11")
    INT_ASSERT_PACK_LIMIT = 140, // Resource capacity constraint ("pack limit is 10")
    INT_ASSERT_ITEM_WV = 141,    // Weighted item value ("idol weight 6 value 15")
    INT_ASK_KNAPSACK = 142,      // Non-greedy 0/1 knapsack optimization ("which items maximize value?")
    INT_ASSERT_DEPEND = 143,     // Build dependency ("wall requires mortar")
    INT_ASK_SCHEDULE = 144,      // Dependency topological build schedule ("what order to build paint?")
    INT_ASK_CRIT_PATH = 145,     // Parallel critical-path time ("bake 4 and chop 2 parallel then serve 3 total time?")
    INT_ASSERT_CORRESP = 146,    // Cross-domain structural mapping ("wire corresponds to pipe")
    INT_ASK_SCHEMA_XFER = 147,   // Zero-shot analogical causal schema transfer queries
    INT_ASSERT_COORD = 148,      // Explicit 2D coordinate ("beacon is at 2 3")
    INT_ASK_GRID_DIST = 149,     // 2D Manhattan distance ("grid distance from beacon to tower")
    INT_ASK_DIRECTION = 150,     // 2D cardinal direction ("which direction is tower from beacon")
    INT_ASK_LINE_SIGHT = 151,    // 2D line-of-sight occlusion ("can beacon see tower")
    INT_ASSERT_EPIST_PRIV = 152, // Private vs public epistemic announcement ("mira privately saw ruby in vault")
    INT_ASK_COMMON_KNOW = 153,   // Common knowledge & muddy-children 2nd-order epistemic deduction
    INT_ASSERT_HIST_CAUSE = 154, // Causal history & overdetermination ("spark caused fire", "backup also caused alarm")
    INT_ASK_HIST_CF = 155,       // Pearl structural historical counterfactual ("if spark had not happened would alarm happen?")
    INT_ASK_META_WHY = 156,      // Socratic meta-dialogue ("why do you say that?", "tell me more")
    // --- v12 20M NEOCORTEX, KERNEL 6 CONTRAST, BAYES, LOGIC, COMBINATORICS, GCD/LCM, RATIOS, CLOCK, KNIGHTS, GRAPH & DIALOGUE ---
    INT_ASK_BAYES = 157,         // Quantitative Bayesian posterior updating & base-rate reasoning
    INT_ASK_PROP_LOGIC = 158,    // Contrapositive, De Morgan's Law & XOR truth tables
    INT_ASSERT_XOR_GATE = 159,   // XOR gate assertion ("gate is xor of alpha and beta")
    INT_ASK_COMBINAT = 160,      // Permutations (n!), Handshakes C(n,2), Subsets (2^n)
    INT_ASK_NUM_THEORY = 161,    // Gauss sum 1..n, Euclidean GCD & LCM
    INT_ASK_RATIO_RATE = 162,    // Ratio split, weighted mean & harmonic work-rate
    INT_ASK_CLOCK_INTV = 163,    // Clock wrap-around (mod 12) & interval overlap detection
    INT_ASSERT_KNIGHT = 164,     // Knight/Knave statement ("alice says bob is a knave")
    INT_ASK_KNIGHT = 165,        // Knight/Knave deduction ("what is alice?", "is sam knave possible?")
    INT_ASSERT_GRAPH = 166,      // Dynamic graph edge ("isle_a connects to isle_b")
    INT_ASK_GRAPH = 167,         // Graph 2/3-coloring feasibility & shortest hop distance
    INT_ASK_DIALOG_ACT = 168,    // Warm dialogue acts ("how are you feeling", "give me a riddle", "is it penguin", "thank you")
    INT_ASK_NEO_CONTRAST = 169,  // Pure GPU spiking differential contrast (`mind_neo_contrast`) & Pathway-17 syllogism Ribot!
    // --- v13 24M NEOCORTEX, KERNEL 7 ANALOGY, LIVE 1-SHOT SYNTAX, OPEN CONVERSATION & AUTONOMOUS ECOLOGY ---
    INT_TEACH_PHRASE = 170,      // Live 1-shot user-taught idiom or parameterized syntax construction!
    INT_ASK_PROFILE = 171,       // Open-ended conversational knowledge synthesis ("what do you know about X?", "teach me something")
    INT_ASK_NEO_ANALOGY = 172,   // Kernel 7 pure-GPU spiking proportional analogy (`mind_neo_analogy`, Octuple Ribot!)
    INT_ASSERT_LIN2 = 173,       // 2x2 multi-variable linear system equations ("x plus y equals 10", "2 times x plus y equals 14")
    INT_ASK_LIN2 = 174,          // Query 2x2 solution ("what is x", "what is y") & weighted average
    INT_ASK_DECISION = 175,      // Probabilistic decision trees & Monty-Hall conditional switching
    INT_ASSERT_MODAL = 176,      // Modal/temporal claim & counterexample event ("always/never/sometimes")
    INT_ASK_MODAL = 177,         // Modal temporal truth & counterexample falsification query
    INT_ASSERT_ALT_ROUTE = 178,  // Dynamic obstacle/tool failure & backup method binding
    INT_ASK_REPLAN = 179,        // Recursive goal decomposition with obstacle backtracking
    INT_ASSERT_INFORM = 180,     // Epistemic informing event ("mira told wren ruby is in vault")
    INT_ASK_BELIEF_UPD = 181,    // 3rd-order epistemic belief revision vs stale belief tracking
    INT_ASK_ECO_REASON = 182,    // Autonomous ecology decision & drive reasoning!
    // --- v14 28M COMPLETE-L2 NEOCORTEX, KERNEL 8 COMPOSE, PERSONAL MEMORY, SETS, RIVER-CROSS, BOUNDS, FEW-SHOT SYNTHESIS ---
    INT_ASK_NEO_COMPOSE = 183,   // Kernel 8 pure-GPU compositional intersection & veto (`mind_neo_compose`, Nonuple Ribot!)
    INT_ASSERT_USER_FACT = 184,  // Personal conversational memory ("my favorite animal is eagle", "i live in valley")
    INT_ASK_USER_FACT = 185,     // Recall & cross-link personal facts ("what is my favorite animal?", "can my favorite animal fly?")
    INT_ASK_BRAIN_SELF = 186,    // Self-reflective brain telemetry, active topic recall, bidirectional curiosity & warm chat
    INT_ASSERT_SET = 187,        // Formal set definition ("set alpha has ruby pearl opal")
    INT_ASK_SET = 188,           // Set intersection, set difference & unique union cardinality
    INT_ASSERT_CONFLICT = 189,   // River-crossing pairwise conflict ("fox threatens hen", "hen threatens grain")
    INT_ASK_RIVER = 190,         // River-crossing safety & first-crossing deductive constraint solver
    INT_ASK_RECUR_MATH = 191,    // Compound percentage growth, exponentiation & factorial ratios
    INT_ASSERT_BOUND = 192,      // Interval inequality bounds ("score of mira is at least 8", "at most 5")
    INT_ASK_BOUND = 193,         // Transitive strict inequality & interval overlap uncertainty + minimum gap
    INT_ASSERT_JOINT = 194,      // Multi-agent joint responsibility event ("mira unlocked gate", "fox stole gem")
    INT_ASK_BLAME = 195,         // Counterfactual joint responsibility & intentional blame attribution
    INT_ASSERT_FUNC_EX = 196,    // Few-shot inductive function example ("example 2 maps to 5")
    INT_ASK_FUNC_SYN = 197,      // Few-shot inductive program synthesis query ("what does 7 map to?")
    INT_ASK_NEO_TOOL = 198,      // Pathway-23 GPU inverse causal precondition recall (`cortex tool needed for cooked`, Ribot!)
    // --- v15 KERNEL 9 MULTI-CUE, OPEN READING, ACTIVE INFO-GAIN, SOCRATIC DIALECTIC, SYLLOGISMS, PARETO, DO-CALC & CULTURE ---
    INT_ASK_MULTI_CUE = 199,     // Kernel 9 pure-GPU multi-cue context retrieval (`mind_neo_multi_cue`, Decuple Ribot!)
    INT_ASSERT_SITUATION = 200,  // Open-ended multi-sentence reading situation-model events
    INT_ASK_SITUATION = 201,     // Open-ended story who/where/why/summary queries
    INT_ASSERT_CAND_FEAT = 202,  // Candidate hypothesis feature matrix for active information gain
    INT_ASK_INFO_GAIN = 203,     // Optimal Shannon-entropy splitting & sequential hypothesis elimination
    INT_ASK_SOCRATIC = 204,      // Socratic evidence defense, challenge resistance & counterfactual separation
    INT_ASSERT_SYLLOG = 205,     // Aristotelian categorical premise ("every X belongs to Y" / "no X belongs to Y")
    INT_ASK_SYLLOG = 206,        // Barbara / Celarent deduction & undistributed-middle fallacy rejection
    INT_ASSERT_VALUE = 207,      // Agent subjective utility valuation ("mira values ore at 8")
    INT_ASK_TRADE = 208,         // Multi-agent Pareto trade evaluation & joint surplus calculation
    INT_ASSERT_PAIR_SUM = 209,   // 3-variable pairwise sum constraint ("pair u plus v equals 10")
    INT_ASK_VAR3 = 210,          // 3-variable triangular algebraic solver ("what is value of variable u?")
    INT_ASSERT_DAG = 211,        // Directed causal DAG edge ("node rain directly causes wet")
    INT_ASK_DO_CALC = 212,       // Pearl Rung-1 passive observation vs Rung-2 surgical do() intervention
    INT_ASSERT_COIN_WORD = 213,  // Cultural composite word invention + Pathway-27 live micro-ripple!
    INT_ASK_DEFINE = 214,        // Composite word definition recall via Pathway 27!
    // --- v16 KERNEL 10 HYPOTHESIS RANKING, OPEN-VOCAB VERB GRAPH, PROACTIVE CURIOSITY, CNF SAT, ANALOGICAL TRANSFER & RISK ---
    INT_ASK_HYPOTHESIZE = 215,   // Kernel 10 pure-GPU multi-clue hypothesis ranking (`mind_neo_hypothesize`, Undecuple Ribot!)
    INT_ASSERT_OPEN_TRIPLE = 216,// General dialect-aware open-vocabulary SVO/SOV prose triple (`S V O` / `S O V`)!
    INT_ASK_OPEN_TRIPLE = 217,   // Open-verb queries (who V O, what did S V, what did S do, chain V, conflict check)!
    INT_ASK_WONDER = 218,        // Proactive gap-driven curiosity ("what do you wonder about X?", "X lives in Y", "what did you learn about X?")
    INT_ASSERT_TASK_DL = 219,    // Task duration + deadline constraint ("task harvest takes 4 hours deadline 6")
    INT_ASK_TASK_DL = 220,       // Resource/deadline schedule sum & feasibility check!
    INT_ASSERT_CLAUSE = 221,     // Propositional CNF SAT clause ("clause alpha or beta", "clause not alpha or gamma", "clause not beta")
    INT_ASK_SAT = 222,           // CNF satisfiability & forced variable deduction!
    INT_ASSERT_WORLD_RULE = 223, // Isomorphic world recipe step & cross-world role correspondence
    INT_ASK_WORLD_ANALOG = 224,  // Multi-step analogical problem transfer across worlds!
    INT_ASSERT_RISK = 225,       // Independent risk probability assertion ("risk storm is 50 percent")
    INT_ASK_RISK = 226,          // Probabilistic noisy-OR combined risk, joint AND risk & safety comparison!
    INT_ASSERT_INIT_BELIEF = 227,// Initial belief & explicit revision ("initially X is in Y" / "actually X moved to Z")
    INT_ASK_META_BELIEF = 228,   // Metacognitive prior-belief recall & belief-change explanation!
    // --- v17 KERNEL 11 BRIDGE, DISCOURSE COREFERENCE, 4-ARG NOVEL VERBS, STOCHASTIC PLANNING, BUDGET, EXCEPTIONS, INTERVALS, FAULT & DEBATE ---
    INT_ASK_BRIDGE = 229,        // Kernel 11 pure-GPU bidirectional spreading-activation bridge (`mind_neo_bridge`, Duodecuple Ribot!)
    INT_ASSERT_COREF = 230,      // Multi-sentence narrative event with cross-sentence pronoun coreference (`she carried it to X`, `she gave it to Y`)
    INT_ASK_COREF = 231,         // Coreferential discourse query (`who has X now?`, `where did she carry it?`, `who gave X to Y?`)
    INT_ASSERT_FRAME4 = 232,     // General 4-argument ditransitive / instrumental novel verb (`S V O with T` / `S V O to R`)!
    INT_ASK_FRAME4 = 233,        // 4-argument open-frame query (`what tool did S use to V O?`, `who received O from S?`, `with what did S V O?`)
    INT_ASSERT_STOCH = 234,      // Stochastic route action (`route cliff succeeds 50 percent reward 100`)
    INT_ASK_STOCH = 235,         // Expected-utility probabilistic choice & dynamic reward sensitivity!
    INT_ASSERT_ALLOC = 236,      // Budget cap & plan item (`budget cap is 20` / `allocate lantern cost 8 utility 14`)
    INT_ASK_ALLOC = 237,         // Optimal affordable pair & opportunity-cost tradeoff under budget shifts!
    INT_ASSERT_EXCEPT = 238,     // Higher-order quantified exception norm (`all scout except fox may enter vault`)
    INT_ASK_EXCEPT = 239,        // Set-difference exception permission check (`may mira/fox enter vault?`)
    INT_ASSERT_FREE = 240,       // Agent availability window (`mira is free from 2 to 6`)
    INT_ASK_MEET = 241,          // Multi-agent temporal interval overlap meeting scheduler!
    INT_ASSERT_PIPE = 242,       // Causal pipeline topology & sensor telemetry (`pipeline pump feeds filter` / `sensor boiler is failing`)
    INT_ASK_FAULT = 243,         // Causal chain root-fault localization & upstream reachability!
    INT_ASSERT_PROCON = 244,     // Dialectical pro/con weighted argument (`debate bridge pro 6 con 2`)
    INT_ASK_DEBATE = 245,        // Multi-criteria net score & priority-conditioned recommendation!
    // --- v18 KERNEL 12 UNSUPERVISED CLUSTERING, EMBEDDED CLAUSES, COALITION SHAPLEY, THERMODYNAMICS, ORDER REPLAY, BOM, BYZANTINE, EVENT CALC & META-RULES ---
    INT_ASK_CLUSTER = 246,       // Kernel 12 pure-GPU odd-one-out & closest-pair prototype clustering (`mind_neo_cluster`, Tredecuple Ribot!)
    INT_ASSERT_EMBED = 247,      // Recursive double-embedded relative clause (`tara who discovered chalice that is luminous visited watchtower`)
    INT_ASK_EMBED = 248,         // Query embedded-clause roles (agent, object, property, destination)
    INT_ASSERT_COAL = 249,       // Coalition game standalone & joint payoff (`coalition mira alone earns 4`, `together earn 16`)
    INT_ASK_COAL = 250,          // Coalition synergy, superadditivity & exact Shapley fair split!
    INT_ASK_THERMO = 251,        // Continuous weighted mixing conservation & thermal phase equilibrium!
    INT_ASSERT_SEQOP = 252,      // Stateful operator chain (`register starts at 3`, `step double`, `step add 4`)
    INT_ASK_SEQOP = 253,         // Forward evaluation, counterfactual swapped-order replay & commutativity check!
    INT_ASSERT_BOM = 254,        // Recursive supply-chain Bill-of-Materials recipe (`recipe 1 lantern needs 2 glass and 1 wick`)
    INT_ASK_BOM = 255,           // Multi-level recursive raw-material requirements & shortage feasibility!
    INT_ASSERT_WITNESS = 256,    // Witness report for epistemic consensus (`witness mira reports beacon is safe`)
    INT_ASK_CONSENSUS = 257,     // Majority consensus, Byzantine dissenter detection & supermajority quorum!
    INT_ASSERT_FLUENT = 258,     // Event-calculus timed state transition (`at tick 2 latch becomes open`)
    INT_ASK_FLUENT = 259,        // Event-calculus temporal inertia query (`at tick 4 is latch open?`)
    INT_ASSERT_METARULE = 260,   // Second-order hypothesis rule, counterexample & minimal exception patch!
    INT_ASK_METARULE = 261,      // Patched-rule evaluation & explicit rule explanation!
    // --- v19 KERNEL 13 CORTICAL CONFLICT GATE, MULTI-TURN CHAT RECAP, BELLMAN MDP, MEDIATION, MAX-FLOW, VICKREY AUCTION, 2D VECTORS, PERT/CPM & DUTY PRIORITY ---
    INT_ASK_GATE = 262,          // Kernel 13 pure-GPU episodic-vs-semantic cortical consistency/exception/novelty gate (`mind_neo_gate`, Quattuordecuple Ribot!)
    INT_ASSERT_CHAT = 263,       // Free-form multi-turn chat observation & follow-up (`i spotted hawk near cliff`, `hawk hunted mouse`)
    INT_ASK_CHAT = 264,          // Context-carried follow-up & conversational recap (`what did i spot?`, `what did it hunt?`, `recap our chat`)
    INT_ASSERT_MDP = 265,        // Sequential MDP transition (`state camp action forage reward 3 next river`)
    INT_ASK_MDP = 266,           // Non-myopic Bellman horizon optimal action & multi-step cumulative value!
    INT_ASSERT_MEDIATE = 267,    // Causal mediation chain & direct path (`mediator drought causes drysoil`, `direct heat causes cropfail`)
    INT_ASK_MEDIATE = 268,       // Mediated path identification & blocking intervention analysis!
    INT_ASSERT_FLOW = 269,       // Directed flow network capacity (`conduit fount to basin cap 10`, `expands to 8`)
    INT_ASK_FLOW = 270,          // Path bottleneck capacity & total multi-path max-flow!
    INT_ASSERT_BID = 271,        // Vickrey auction bid (`auction relic bidder mira bids 12`)
    INT_ASK_AUCTION = 272,       // Auction winner, second-price Vickrey payment & dominant truthful bidding!
    INT_ASSERT_POINT = 273,      // Continuous 2D point coordinates (`point alpha is at 2 4`)
    INT_ASK_VEC2D = 274,         // Midpoint, point reflection & squared Euclidean distance!
    INT_ASSERT_PERT = 275,       // PERT/CPM project module duration & precedence dependencies!
    INT_ASK_PERT = 276,          // Earliest-finish DAG propagation & critical-path bottleneck predecessor!
    INT_ASSERT_DUTY = 277,       // Deontic obligation priority (`duty rescue priority 10`)
    INT_ASK_DUTY = 278,          // Lexicographic obligation override, next duty & moral justification!
    // --- v20 KERNEL 14 SWR DREAM CLOSURE, MULTI-PARAGRAPH NARRATIVE, WALRASIAN MARKET, NEWTONIAN PHYSICS, SOCRATIC ASSISTANT, 2D AFFINE, INFO THEORY, NASH 2x2 & NECESSARY/REDUNDANT CAUSATION ---
    INT_ASK_DREAM = 279,         // Kernel 14 pure-GPU offline SWR 2-hop dream replay & consolidation (`mind_neo_dream`, Quindecuple Ribot!)
    INT_ASSERT_CHAPTER = 280,    // Multi-paragraph narrative story arc events (hero, goal, obstacle, tool, resolution!)
    INT_ASK_CHAPTER = 281,       // Multi-paragraph motivation, overcoming & full arc synthesis!
    INT_ASSERT_MARKET = 282,     // Walrasian double-auction seller asks & buyer bids (`seller mira offers grain at 3`)
    INT_ASK_MARKET = 283,        // Supply-demand clearing volume, equilibrium price & total gains from trade!
    INT_ASSERT_PHYSICS = 284,    // 2D Newtonian moving body (`mover boulder mass 4 velocity 6`)
    INT_ASK_PHYSICS = 285,       // Momentum, inelastic merged collision velocity & fulcrum lever equilibrium!
    INT_ASSERT_ASSIST = 286,     // Collaborative Socratic assistant goal, requirements & live progress updates!
    INT_ASK_ASSIST = 287,        // Missing precondition detection & dynamic next-step coaching!
    INT_ASSERT_AFFINE = 288,     // 2D affine coordinate transform (`affine scale 2 shift 3 1`)
    INT_ASK_AFFINE = 289,        // Forward map, iterated composition & inverse preimage!
    INT_ASK_INFOTHEORY = 290,    // Optimal binary decision tree depth, expected coding bits & compression savings!
    INT_ASSERT_PAYOFF = 291,     // 2x2 normal-form game payoff entry (`payoff coop defect 0 5`)
    INT_ASK_PAYOFF = 292,        // Dominant strategy, pure Nash equilibrium & Pareto social optimum comparison!
    INT_ASSERT_CIRCUIT = 293,    // Conjunctive (AND) vs disjunctive (OR) causal circuit structure & states!
    INT_ASK_CIRCUIT = 294,       // Necessary but-for root cause vs overdetermined redundant cause attribution!
    // --- v21 KERNEL 15 UNIVERSAL QUANTIFIER SCAN, REFLECTIVE OPINION DIALOGUE, SCIENTIFIC LAW INDUCTION, NPV FINANCE, KINEMATICS, MCDA UTILITY, KRUSKAL MST, LTL TEMPORAL LOGIC & PROPORTIONAL APPORTIONMENT ---
    INT_ASK_SCAN = 295,          // Kernel 15 pure-GPU universal/existential category scanner (`mind_neo_scan`, Sedecuple 16-Track Ribot!)
    INT_ASSERT_OPINION = 296,    // Reflective user preferences & reasons (`opinion user prefers forest over desert because shade`)
    INT_ASK_OPINION_REC = 297,   // Personalized reflection, recommendation & reason explanation!
    INT_ASSERT_TRIAL = 298,      // Experimental data point (`trial temp 10 yield 25`)
    INT_ASK_LAW = 299,           // Autonomous linear law induction, out-of-sample prediction & inverse extrapolation!
    INT_ASSERT_PROJECT = 300,    // Capital investment cash-flow (`project orchard costs 100 returns 60 then 80`)
    INT_ASK_FINANCE = 301,       // Net profit, ROI percentage & optimal project selection!
    INT_ASSERT_KINEM = 302,      // Kinematics initial parameters (`kinematics start 10 accel 4 time 5`)
    INT_ASK_KINEM = 303,         // Final velocity, accelerated distance traveled & braking stop time!
    INT_ASSERT_MCDA = 304,       // Multi-criteria criterion weight & option scores (`criterion agility weight 3`, `option glider agility 8 durability 4`)
    INT_ASK_MCDA = 305,          // Weighted utility score & sensitivity-aware winner selection!
    INT_ASSERT_CABLE = 306,      // Weighted undirected cable edge (`cable camp forest cost 4`)
    INT_ASK_MST = 307,           // Kruskal Minimum Spanning Tree total wiring cost & skipped expensive edge!
    INT_ASSERT_TRACE = 308,      // Execution trace step (`trace step 1 green`)
    INT_ASK_LTL = 309,           // Formal LTL Always (G), Eventually (F) & Until (U) trace verification!
    INT_ASSERT_APPORT = 310,     // Total seats & district votes (`seats total 10`, `district northward votes 50`)
    INT_ASK_APPORT = 311,        // Exact proportional quota seat apportionment!
    // --- v22 KERNEL 16 DEDUCTIVE PROOF CHAIN, OPEN-TOPIC SOCRATIC Q&A, DIJKSTRA, SEQUENTIAL BAYES, EXPR TREE, ELASTIC COLLISION, 0/1 DP KNAPSACK, COALITION CORE & SELF-INVENTED EXPERIMENTS ---
    INT_ASK_DEDUCE = 312,        // Kernel 16 pure-GPU multi-step deductive proof & counterfactual blocking (`mind_neo_deduce`, Septendecuple 17-Track Ribot!)
    INT_ASSERT_TOPIC = 313,      // Arbitrary open-topic knowledge note (`topic astronomy says comet has tail`)
    INT_ASK_TOPIC = 314,         // Open-topic synthesis, bridge discovery & curious Socratic follow-up!
    INT_ASSERT_ROAD = 315,       // Weighted road graph edge (`road camp to forest cost 4`)
    INT_ASK_DIJKSTRA = 316,      // Dijkstra shortest weighted distance & exact optimal route!
    INT_ASSERT_SEQBAYES = 317,   // Hypothesis prior & sequential independent sensor likelihood ratios!
    INT_ASK_SEQBAYES = 318,      // Sequential Bayesian posterior probability update!
    INT_ASSERT_EXPR = 319,       // Recursive variable/expression binding (`define var beta equals alpha plus 4`)
    INT_ASK_EXPR = 320,          // Dynamic recursive expression tree evaluation!
    INT_ASSERT_ELASTIC = 321,    // 2D elastic moving body (`body orb mass 4 speed 6`)
    INT_ASK_ELASTIC = 322,       // Kinetic energy, elastic rebound velocities & conservation audit!
    INT_ASSERT_CARGO = 323,      // 0/1 DP cargo knapsack capacity & item weight/value!
    INT_ASK_CARGO = 324,         // Optimal 0/1 DP max value & exact selected item reconstruction!
    INT_ASSERT_CORE = 325,       // Cooperative coalition characteristic values `v(S)` & payoff split!
    INT_ASK_CORE = 326,          // Core stability check, blocking coalition detection & minimum pair share!
    INT_ASSERT_EXP = 327,        // Competing scientific theories with predicted observables & experimental observation!
    INT_ASK_EXP = 328,           // Discriminative experiment design, surviving theory & falsification reason!
    // --- v23 KERNEL 17 BACKWARD APICAL ABDUCTION, ZERO-PREFIX OPEN DIALOGUE, RESOURCE DYNAMICS, BALLISTICS, MATRIX 2x2, STATS, INTERVAL DP, CONDORCET VOTING & PEARL D-SEPARATION ---
    INT_ASK_ABDUCE_BACK = 329,   // Kernel 17 pure-GPU 2-hop top-down apical backward abduction (`mind_neo_abduce_back`, Octodecuple 18-Track Ribot!)
    INT_ASK_FREE_SYNTH = 330,    // Zero-prefix open-triple multi-hop connection, locality synthesis & grounded curiosity!
    INT_ASSERT_DYN = 331,        // Discrete-time ecological stock init/regrow/draw rate!
    INT_ASK_DYN = 332,           // Ecological stock projection, depletion horizon & sustainable draw rate!
    INT_ASSERT_PROJ = 333,       // 2D continuous ballistic projectile velocity & gravity!
    INT_ASK_PROJ = 334,          // Apex time, max altitude & horizontal ballistic range!
    INT_ASSERT_MAT2 = 335,       // 2x2 matrix row entries!
    INT_ASK_MAT2 = 336,          // 2x2 determinant, area magnification & singularity check!
    INT_ASSERT_STATS = 337,      // Statistical dataset numeric sample!
    INT_ASK_STATS = 338,         // Dataset mean, population variance & anomaly detection!
    INT_ASSERT_JOB = 339,        // Weighted interval gig span & wage!
    INT_ASK_JOB = 340,           // Exact Weighted Interval Scheduling DP max wage & optimal subset!
    INT_ASSERT_VOTE = 341,       // Ranked-choice voter ballot (`ballot v1 ranks park plaza pier`)!
    INT_ASK_VOTE = 342,          // Condorcet pairwise majority winner / paradox cycle & Borda score!
    INT_ASSERT_PEARL = 343,      // Pearl directed causal DAG edge (`dag season arrows rainfall`)!
    INT_ASK_PEARL = 344,         // Collider conditioning bias, common-ancestor confounder & d-separation!
    // --- v24 KERNEL 18 COUNTERFACTUAL IMAGINATION, ZERO-PREFIX CAUSAL PROSE, MARKOV, LP, TREE LCA, WAVES, OHM, MENDEL & COMPLEXITY ---
    INT_ASK_COUNTERFACT = 345,   // Kernel 18 pure-GPU counterfactual feature-grafting & prototype shift (`mind_neo_counterfact`, Novemdecuple 19-Track Ribot!)
    INT_ASK_PROSE_CAUSAL = 346,  // Zero-prefix multi-sentence prose forward cascade, root-cause explanation & full story synthesis!
    INT_ASSERT_MARKOV = 347,     // Discrete 2-state Markov chain stay/shift transition probability!
    INT_ASK_MARKOV = 348,        // N-step Markov state probability & analytical steady-state distribution!
    INT_ASSERT_LP = 349,         // 2-variable linear programming objective & linear inequality constraints!
    INT_ASK_LP = 350,            // Optimal LP objective profit & optimal `(x, y)` corner allocation!
    INT_ASSERT_TREE = 351,       // Recursive directed tree parent-child edge (`treenode trunk0 spawns leftbranch`)!
    INT_ASK_TREE = 352,          // Recursive tree node depth, leaf tip count & lowest common ancestor (LCA)!
    INT_ASSERT_WAVE = 353,       // Harmonic wave frequency & wavelength assertion!
    INT_ASK_WAVE = 354,          // Wave velocity, inverse-frequency wavelength & standing harmonic frequencies!
    INT_ASSERT_OHM = 355,        // Electrical resistor ohms & battery voltage!
    INT_ASK_OHM = 356,           // Series/shunt equivalent resistance & Ohm's law current!
    INT_ASSERT_GENE = 357,       // Mendelian dominance & parent genotype (`het`/`rec`/`dom`)!
    INT_ASK_GENE = 358,          // Punnett-square dominant/recessive phenotype probability!
    INT_ASSERT_ALGO = 359,       // Algorithm asymptotic complexity class (`constant`/`linear`/`nlogn`/`quadratic`)!
    INT_ASK_ALGO = 360,          // Asymptotic scaling comparison, slowdown factor & fastest algorithm!
    // --- v25 KERNEL 19 HIERARCHICAL PLAN CHAINING, PRONOUN DIALOGUE, EPIDEMIC R0, BUOYANCY, PORTFOLIO, TOPO DAG, BINARY SEARCH, DOPPLER & ITERATED PD ---
    INT_ASK_PLAN_CHAIN = 361,    // Kernel 19 pure-GPU 2-hop hierarchical goal-to-subgoal plan chaining (`mind_neo_plan_chain`, Vigintuple 20-Track Ribot!)
    INT_ASSERT_PROSE_LOC = 362,  // Natural unprefixed spatial-action prose (`maya built windmill at hilltop`) with live pronoun topic binding!
    INT_ASK_PROSE_DIAL = 363,    // Natural pronoun-resolved dialogue queries (`what does windmill power`, `what happened to windmill`, `where did maya build windmill`)!
    INT_ASSERT_EPIDEMIC = 364,   // Epidemic contagion R0 & initial cases (`epidemic blight r0 3 cases 2`)!
    INT_ASK_EPIDEMIC = 365,      // Exponential R0 growth, herd immunity threshold & vaccine shrink check!
    INT_ASSERT_BUOY = 366,       // Fluid density & solid slab mass/volume (`fluid brine density 12`, `slab cedar mass 18 volume 3`)!
    INT_ASK_BUOY = 367,          // Archimedes density, float/sink check & submerged percentage!
    INT_ASSERT_PORTF = 368,      // Portfolio asset return & risk (`asset timber return 12 risk 4`)!
    INT_ASK_PORTF = 369,         // Risk-return efficiency, best asset & equal-split diversification return!
    INT_ASSERT_TOPO = 370,       // Directed dependency edge (`dep hull before mast`)!
    INT_ASK_TOPO = 371,          // Kahn topological sort order & circular deadlock detection!
    INT_ASSERT_BSEARCH = 372,    // Sorted array size (`sorted array size 256`)!
    INT_ASK_BSEARCH = 373,       // Binary search log2 probes, linear probes & exact speedup!
    INT_ASSERT_DOPPLER = 374,    // Acoustic siren emitted frequency & airspeed (`siren emit 120 airspeed 300`)!
    INT_ASK_DOPPLER = 375,       // Approaching/receding Doppler frequency shift & pitch direction!
    INT_ASSERT_PD = 376,         // Iterated Prisoner's Dilemma R/T/S/P payoffs & continuation probability!
    INT_ASK_PD = 377,            // Grim-trigger cooperation stability & exact discount threshold!
    // --- v26 KERNEL 20 COMPOSITIONAL CAUSAL SIMULATION, MULTI-PARAGRAPH PROSE, STOICHIOMETRY, OPTICS, SHANNON ENTROPY, LCS/EDIT DP, QUEUEING, DIFFIE-HELLMAN & ENVY-FREE CAKE DIVISION ---
    INT_ASK_SIMULATE = 378,      // Kernel 20 pure-GPU NMDA action-conditioned causal simulation (`mind_neo_simulate`, Unvigintuple 21-Track Ribot!)
    INT_ASK_PARAGRAPH = 379,     // Cross-paragraph natural prose pronoun & multi-event synthesis (`what did nora do`, `who warned crew`, `where did nora save ship`)!
    INT_ASSERT_CHEM = 380,       // Chemical reaction stoichiometry & available stock (`chem reaction 2 hydrogen and 1 oxygen makes 2 water`, `chem stock 6 hydrogen and 5 oxygen`)!
    INT_ASK_CHEM = 381,          // Limiting reagent, exact product yield & leftover excess reactant!
    INT_ASSERT_OPTICS = 382,     // Optical thin-lens focal length (`optics lens focal 6`)!
    INT_ASK_OPTICS = 383,        // Thin-lens image distance (`1/f = 1/do + 1/di`) & real/virtual classification!
    INT_ASSERT_ENTROPY = 384,    // Dyadic probability distribution (`dist skewed has 1 halves 2 quarters`)!
    INT_ASK_ENTROPY = 385,       // Exact Shannon entropy in centibits & minimum-entropy comparison!
    INT_ASSERT_LCS = 386,        // Symbolic token sequence (`seq seqa is oak pine maple cedar`)!
    INT_ASK_LCS = 387,           // Dynamic programming LCS length & Levenshtein edit distance!
    INT_ASSERT_QUEUE = 388,      // Queueing server rates (`queue dock arrivals 4 service 5 wait 3`)!
    INT_ASK_QUEUE = 389,         // Little's Law occupancy (`L = lambda*W`), utilization % & stability/saturation!
    INT_ASSERT_DH = 390,         // Diffie-Hellman prime/generator & agent secret keys!
    INT_ASK_DH = 391,            // Modular exponentiation pubkeys (`g^a mod p`) & verified shared key!
    INT_ASSERT_CAKE = 392,       // Multi-agent cake slice valuations & slice assignment!
    INT_ASK_CAKE = 393,          // Envy-free allocation check, envy detection & exact fair transfer!
    // --- v27 KERNEL 21 CONSTRAINT SATISFACTION, LAYER 3D CAUSAL-BECAUSE PROSE, 2D INTERCEPT, IDEAL GAS, CRT, BIPARTITE, COMPOUND/PV, NYQUIST & COMMONS ---
    INT_ASK_CONSTRAIN = 394,     // Kernel 21 pure-GPU multi-attribute constraint satisfaction & elimination (`mind_neo_constrain`, Duovigintuple 22-Track Ribot!)
    INT_ASSERT_PROSE_WHY = 395,  // Layer 3d unprefixed causal-because prose (`lena kindled lantern because fog veiled trail`)!
    INT_ASK_PROSE_WHY = 396,     // Natural causal explanation query over Layer 3d prose (`why did lena kindle lantern`)!
    INT_ASSERT_MOVER2D = 397,    // Continuous 2D body position & velocity (`mover2d runner at 0 0 vel 3 4`)!
    INT_ASK_MOVER2D = 398,       // 2D speed, collinear pursuit catch time & future 2D position!
    INT_ASSERT_GAS = 399,        // Ideal gas chamber state (`gas chamber pressure 4 vol 6 kelvin 300`)!
    INT_ASK_GAS = 400,           // Boyle, Gay-Lussac & Combined Ideal Gas Law pressure solver!
    INT_ASSERT_CRT = 401,        // Chinese Remainder Theorem congruence system (`crt mod1 3 rem1 2 mod2 5 rem2 3`)!
    INT_ASK_CRT = 402,           // Smallest CRT solution, LCM period & extended-Euclidean modular inverse!
    INT_ASSERT_BIPART = 403,     // Undirected graph edge for bipartite 2-coloring (`bipartite edge hub1 hub2`)!
    INT_ASK_BIPART = 404,        // BFS 2-colorability, partition vertex counts & odd-cycle detection!
    INT_ASSERT_BOND = 405,       // Compounding bond principal & annual percentage rate (`bond vault principal 1000 rate 10 percent`)!
    INT_ASK_BOND = 406,          // Multi-year compound value, discounted present value & Rule-of-72 doubling!
    INT_ASSERT_NYQUIST = 407,    // Bandlimited signal max frequency (`signal tone band 40`)!
    INT_ASK_NYQUIST = 408,       // Nyquist rate (`2*f_max`), clean sampling vs folded alias frequency!
    INT_ASSERT_COMMONS = 409,    // Shared commons resource private benefit, cost & external damage!
    INT_ASK_COMMONS = 410,       // Tragedy of the Commons detection & exact Pigouvian externality tax!
    // --- v28 KERNEL 22 FEW-SHOT RELATIONAL INDUCTION, LAYER 3E CONCESSIVE PROSE, KEPLER ORBITS, EIGENVALUES, BAYES ODDS, SHOELACE, DFA, KEYNESIAN MPC & HAMMING ---
    INT_ASK_INDUCE = 411,        // Kernel 22 pure-GPU few-shot relational schema induction & 1/2-hop transfer (`mind_neo_induce`, Trevigintuple 23-Track Ribot!)
    INT_ASSERT_PROSE_ALTHOUGH = 412, // Layer 3e unprefixed concessive `although` prose (`soren mended bridge although river flooded valley`)!
    INT_ASK_PROSE_DESPITE = 413, // Natural concessive query (`despite what did soren mend bridge?`)!
    INT_ASK_PROSE_RECAP_ENT = 414, // Full multi-sentence story recap about a named actor (`recap everything about soren`)!
    INT_ASSERT_ORBIT = 415,      // Orbital reference radius & period (`orbit moon radius 4 period 8`)!
    INT_ASK_ORBIT = 416,         // Kepler's 3rd Law (`T^2 ~ r^3`) period scaling & circular speed mass scaling!
    INT_ASSERT_MAT2D = 417,      // 2x2 dynamical matrix (`mat2d 5 2 2 5`)!
    INT_ASK_MAT2D = 418,         // 2x2 matrix trace, determinant & characteristic integer eigenvalues!
    INT_ASSERT_ODDS = 419,       // Bayesian prior odds & sequential likelihood-ratio updates (`prior odds 1 to 3`, `odds likelihood 6 to 1`)!
    INT_ASK_ODDS = 420,          // Exact reduced posterior odds (`2 to 1`) & posterior probability %!
    INT_ASSERT_POLY2D = 421,     // 2D polygon ordered vertex (`poly2d vertex 0 0`)!
    INT_ASK_POLY2D = 422,        // Shoelace 2D polygon area & CCW/CW winding orientation!
    INT_ASSERT_DFA = 423,        // Deterministic finite automaton start/accepting state & labeled transition!
    INT_ASK_DFA = 424,           // Multi-symbol DFA trace execution & acceptance/rejection!
    INT_ASSERT_ECON = 425,       // Macroeconomic marginal propensity to consume (`economy mpc 75 percent`)!
    INT_ASK_ECON = 426,          // Keynesian spending multiplier (`1 / (1 - mpc)`) & equilibrium output shift!
    INT_ASSERT_CODE = 427,       // Binary error-correcting codeword (`codeword cw1 0 0 0 0 0`)!
    INT_ASK_CODE = 428,          // Pairwise Hamming distance, minimum code distance & correctable error bits!
    // --- v29 KERNEL 23 COMPOSITIONAL RULE ALGEBRA, LAYER 3F TEMPORAL PROSE, LORENTZ, CAPACITORS/RC, EULER, F1, CONTROL, LOGISTIC & HUFFMAN ---
    INT_ASK_CHAIN_RULE = 429,    // Kernel 23 pure-GPU compositional multi-pathway rule algebra & discovery (`mind_neo_chain_rule`, Quattuorvigintuple 24-Track Ribot!)
    INT_ASSERT_PROSE_TEMP = 430, // Layer 3f unprefixed temporal `before`/`after` prose (`talia sealed hatch before storm struck harbor`)!
    INT_ASK_PROSE_TEMP = 431,    // Temporal `what happened before/after X?`, Socratic `what else happened?` & `summarize the whole story`!
    INT_ASSERT_RELATIV = 432,    // Special relativity speed (`relativity vessel speed 60 percent`)!
    INT_ASK_RELATIV = 433,       // Lorentz time dilation, length contraction & Einstein velocity addition!
    INT_ASSERT_CAPACITOR = 434,  // Capacitor farad value (`capacitor c1 farad 6`)!
    INT_ASK_CAPACITOR = 435,     // Series/parallel capacitance, electrostatic stored energy (1/2 C V^2) & RC time constant!
    INT_ASSERT_EULER = 436,      // Undirected Eulerian graph edge (`euler edge k1 k2`)!
    INT_ASK_EULER = 437,         // Koenigsberg Euler circuit, Euler path & odd-degree non-Eulerian classification!
    INT_ASSERT_CONFUS = 438,     // Confusion matrix counts (`confusion classifier truepos 60 falsepos 20 falseneg 40`)!
    INT_ASK_CONFUS = 439,        // Precision, Recall & F1 harmonic mean percentages!
    INT_ASSERT_CTRL = 440,       // Discrete proportional feedback controller (`controller kiln setpoint 100 state 60 kp 50 percent`)!
    INT_ASK_CTRL = 441,          // Proportional effort, 2-step closed-loop state & steady-state disturbance offset!
    INT_ASSERT_LOGIST = 442,     // Verhulst logistic population (`logistic herd carrying 200 growth 40 percent pop 100`)!
    INT_ASK_LOGIST = 443,        // Next-step logistic population, optimal MSY biomass (K/2) & max sustainable yield (rK/4)!
    INT_ASSERT_HUFFMAN = 444,    // Symbol frequency for Huffman prefix coding (`huffman sym s1 freq 10`)!
    INT_ASK_HUFFMAN = 445,       // Optimal Huffman tree total encoded bits, symbol codeword bit-length & compression savings!
    // --- v30 STAGE 4: KERNEL 24 LIVE 1-SHOT ONLINE SCHEMA GPU, LAYER 3G MULTI-AGENT & COUNTERFACTUAL PROSE, PHOTOELECTRIC, PH, RSA, A*, NASH MIXED, PID & BERNOULLI ---
    INT_ASSERT_ONLINE_SCHEMA = 446, // Kernel 24 Mode 0: live 1-shot Hebbian L2 write (`teach schema quartz yields spark`)!
    INT_ASK_ONLINE_SCHEMA = 447,    // Kernel 24 Mode 1/2: instant 2/3-hop GPU recall, blocker veto & atomic write+rollout (`Quinvigintuple 25-Track Ribot!`)!
    INT_ASSERT_PROSE_TOLD = 448,    // Layer 3g unprefixed multi-agent perspective prose (`mira told orin that lantern hid key`)!
    INT_ASSERT_PROSE_COND = 449,    // Layer 3g unprefixed conditional/counterfactual chain prose (`if dam breaks then valley floods`)!
    INT_ASK_PROSE_V30 = 450,        // Multi-agent perspective query, counterfactual `what if X?` chain & Socratic `why not?`!
    INT_ASSERT_PHOTO = 451,         // Quantum photoelectric metal surface (`photo metal zinc planck 4 workfunc 12`)!
    INT_ASK_PHOTO = 452,            // Photon energy, max kinetic energy (`hf - phi`), cutoff frequency & emission check!
    INT_ASSERT_PH = 453,            // Chemical acid-base solution (`solution citrus ph 3`)!
    INT_ASK_PH = 454,               // Acid-base pOH (`14 - pH`), 10x dilution pH shift & acidic/neutral/alkaline label!
    INT_ASSERT_RSA = 455,           // Toy RSA key setup (`rsa primes 5 and 11 exp 3`)!
    INT_ASK_RSA = 456,              // RSA modulus/private key d, encryption (`m^e mod n`) & exact decryption (`c^d mod n`)!
    INT_ASSERT_ASTAR = 457,         // A* weighted graph edge & admissible node heuristic (`astar edge a1 a2 cost 4`, `astar heur a2 5`)!
    INT_ASK_ASTAR = 458,            // A* f-score (`g + h`), optimal shortest cost & heuristic admissibility check!
    INT_ASSERT_MIXNASH = 459,       // 2x2 mixed-strategy game matrix (`mixnash row1 3 1 row2 0 4`)!
    INT_ASK_MIXNASH = 460,          // Indifference probabilities `p*`, `q*` (percent) & expected mixed payoff!
    INT_ASSERT_PID = 461,           // Discrete PID controller gains & error history (`pid valve kp 2 ki 1 kd 3 err 5 prev 2 sum 4`)!
    INT_ASK_PID = 462,              // Full PID control effort (`Kp*e + Ki*sum + Kd*(e-prev)`), derivative brake & next integral sum!
    INT_ASSERT_BERN = 463,          // Fluid Venturi pipe (`venturi pipe area1 4 speed1 3 density 2`)!
    INT_ASK_BERN = 464,             // Continuity downstream speed (`A1*v1/A2`) & Bernoulli dynamic pressure drop (`1/2 rho (v2^2 - v1^2)`)!
    // --- v31 STAGE 4: KERNEL 25 LIVE LTD/LTP SCHEMA REVISION, 4096 LEXICON, LAYER 3H BELIEF REVISION, CALCULUS, HALF-LIFE, HARDY-WEINBERG, RPN, OLS, COINS & KIRCHHOFF ---
    INT_REVISE_ONLINE_SCHEMA = 465, // Kernel 25 pure-GPU LTD inhibitory unlearning + LTP overwrite (`revise schema quartz yields ember`)!
    INT_ASSERT_PROSE_CORR = 466,    // Layer 3h multi-turn prose belief correction (`correction talia left lantern at pier not harbor`)!
    INT_ASK_PROSE_V31 = 467,        // Layer 3h belief-revision history (`how did belief about lantern change?`) & spontaneous clarification (`clarify ring`)!
    INT_ASSERT_POLY = 468,          // Symbolic monomial (`poly p1 coeff 3 power 2`)!
    INT_ASK_POLY = 469,             // Symbolic 1st/2nd power-rule derivatives & exact definite integral!
    INT_ASSERT_ISOTOPE = 470,       // Radioactive isotope (`isotope iso1 mass 640 halflife 5`)!
    INT_ASK_ISOTOPE = 471,          // Exponential decay mass & elapsed time to target mass!
    INT_ASSERT_HARDY = 472,         // Population genetics allele freq (`gene g1 dompct 60 pop 1000`)!
    INT_ASK_HARDY = 473,            // Hardy-Weinberg homozygous dominant, heterozygote & homozygous recessive counts!
    INT_ASSERT_RPN = 474,           // Compiler RPN stack push/op/clear!
    INT_ASK_RPN = 475,              // RPN evaluated result & peak register stack depth!
    INT_ASSERT_REGRESS = 476,       // OLS linear regression point / reset (`regress pt 1 5`)!
    INT_ASK_REGRESS = 477,          // Least-squares slope, intercept & linear prediction!
    INT_ASSERT_COINS = 478,         // Coin-change denomination set (`coins add 3`, `coins reset`)!
    INT_ASK_COINS = 479,            // Dynamic programming minimum coins & total combination ways!
    INT_ASSERT_KIRCH = 480,         // Kirchhoff KCL inflows/outflows & KVL source/drops!
    INT_ASK_KIRCH = 481,            // KCL missing current balance, KVL remaining drop & Ohm branch current!
    // --- v32 300-DOMAIN MILESTONE: KERNEL 26 SPIKING TEMPORAL ROLE-BINDER, LIVE UNSEEN-WORD LEARNING, CARNOT, SHANNON-HARTLEY, VEC3, TOTIENT, DB, ELASTICITY & CACHE AMAT ---
    INT_ASSERT_ROLEBIND = 482,      // Kernel 26 Mode 0: pure-GPU 4-role circular-phase SDR binding (`rolebind $PERSP $ACTION $SUBJ $LOC`)!
    INT_ASK_ROLEBIND = 483,         // Kernel 26 Mode 1: pure-GPU role unbinding (`cortex rolebind unbind slotloc for $SUBJ and $OBJ`, Septemvigintuple 27-Track Ribot!)
    INT_ASSERT_OPENLEX = 484,       // Live unseen-word definition & causal link (`concept $SUBJ means $PROP`, `concept $SUBJ carries $OBJ`)!
    INT_ASK_OPENLEX = 485,          // Live unseen-word recall & 2-hop open-lexicon chain (`what does concept $SUBJ mean?`, `trace concept $SUBJ`)!
    INT_ASSERT_CARNOT = 486,        // Carnot heat engine reservoirs (`carnot eng1 thot 600 tcold 300`)!
    INT_ASK_CARNOT = 487,           // Carnot efficiency %, useful work & rejected waste heat!
    INT_ASSERT_CHANNEL = 488,       // Shannon-Hartley channel (`channel ch1 bw 10 snr 7`)!
    INT_ASK_CHANNEL = 489,          // Shannon-Hartley capacity `B*log2(1+SNR)` & minimum SNR!
    INT_ASSERT_VEC3 = 490,          // 3D Euclidean vector (`vec3 vecu 3 0 4`)!
    INT_ASK_VEC3 = 491,             // 3D dot product, cross-product z-component & orthogonality check!
    INT_ASK_TOTIENT = 492,          // Number theory Euler's totient `phi(n)` & coprimality (`gcd==1`)!
    INT_ASK_DECIBEL = 493,          // Acoustic/RF log10 decibel gain (`10*log10(ratio)`) & dB power scaling!
    INT_ASSERT_DEMAND = 494,        // Microeconomics demand curve (`demand item1 qty1 100 pr1 10 qty2 60 pr2 12`)!
    INT_ASK_DEMAND = 495,           // Price elasticity `|%dQ/%dP|`, revenue at P1 & revenue at P2!
    INT_ASSERT_CACHE = 496,         // CPU cache config (`cache l1c hitpct 90 hitns 2 misspen 20`)!
    INT_ASK_CACHE = 497,            // Average Memory Access Time (AMAT) & direct-mapped cache set index (`addr mod sets`)!
    // v33 Stage 5 Crossing intents: Kernel 27 predictive spiking syntax, template-free compositional Broca, story linking, hydrostatics/Pascal, Stefan-Boltzmann/Wien, 2x2 matrix, combinatorics, TCP BDP, compound interest & Kruskal MST!
    INT_SEQ_PARSE_BIND = 498,       // Layer 4 template-free unseen prose parsed on GPU by Kernel 27 (`mind_neo_seq_predict`) -> bound via Kernel 26 (`mind_neo_role_bind`)!
    INT_ASK_SEQ_NARRATE = 499,      // Compositional Broca narration straight from K26 unbound SDRs + K27 GPU-predicted dialect word order (`narrate falcon`)!
    INT_ASK_SEQ_ROLE = 500,         // Pure-GPU K26+K27 role unbind from template-free unseen prose (`seqrole slotagent pursued hare`)!
    INT_ASK_SEQ_LINK = 501,         // Cross-episode compositional story chain across 2 K27-parsed unseen sentences (`linkstory condor`)!
    INT_ASSERT_HYDRO = 502,         // Hydrostatic tank (`hydro tank1 rho 1000 subdepth 5`)!
    INT_ASK_HYDRO = 503,            // Hydrostatic pressure `rho*g*h` & Pascal hydraulic lift `F1*A2/A1`!
    INT_ASSERT_RADSTAR = 504,       // Blackbody radiator (`radstar star1 power 100`)!
    INT_ASK_RADSTAR = 505,          // Stefan-Boltzmann `T^4` power scaling & Wien displacement peak wavelength `2900/T_kK`!
    INT_ASSERT_MAT2_V33 = 506,      // 2x2 matrix (`mat2 m1 4 3 2 5`)!
    INT_ASK_MAT2_V33 = 507,         // 2x2 determinant, trace & invertibility check!
    INT_ASK_COMB_V33 = 508,         // Combinatorics: `P(n,k)` permutations, `C(n,k)` combinations & `!n` derangements!
    INT_ASSERT_NETLINK = 509,       // TCP link (`netlink link1 mbps 100 rtt 20`)!
    INT_ASK_NETLINK = 510,          // Bandwidth-delay product (kB), serialization delay & total one-way latency!
    INT_ASSERT_COMPOUND = 511,      // Compound interest account (`compound acct1 prin 1000 rate 10`)!
    INT_ASK_COMPOUND = 512,         // Compound growth `P*(1+r)^n` & Rule-of-72 doubling years!
    INT_ASSERT_MSTEDGE = 513,       // Weighted graph edge (`mstedge mstg1 va vb 4`)!
    INT_ASK_MST_V33 = 514,          // Kruskal Minimum Spanning Tree total weight, edge count & bottleneck max edge!
    // v34 Stage 5b intents: Kernel 28 pure-GPU parietal neural-native math, Coulomb/capacitor, organ pipes, modular exp/inv, Amdahl/Gustafson, virtual memory/TLB, escape/Schwarzschild & 2D metrics!
    INT_ASK_NEURALMATH = 515,       // Kernel 28 pure-GPU spiking parietal bump-attractor math (`nmadd`, `nmsub`, `nmmod`, `nmsolve`, `nmmul`)!
    INT_ASSERT_COULOMB = 516,       // Electrostatic pair (`coulomb pair1 force 100`)!
    INT_ASK_COULOMB = 517,          // Coulomb inverse-square distance & charge scaling + parallel-plate capacitance!
    INT_ASSERT_ORGANPIPE = 518,     // Acoustic organ pipe (`organpipe pipe1 speed 320 length 2`)!
    INT_ASK_ORGANPIPE = 519,        // Open-pipe fundamental, closed-pipe fundamental & acoustic beat frequency!
    INT_ASK_MODMATH = 520,          // Modular exponentiation `a^b mod m` & modular inverse `a^-1 mod m`!
    INT_ASSERT_AMDAHL = 521,        // Parallel workload (`amdahl job1 parpct 75`)!
    INT_ASK_AMDAHL = 522,           // Amdahl speedup, max serial bound & Gustafson scaled speedup!
    INT_ASSERT_VMEM = 523,          // Virtual memory page table (`vmem vm1 pagesize 4096`)!
    INT_ASK_VMEM = 524,             // Virtual page number, page offset & TLB effective access time (EAT)!
    INT_ASSERT_PLANET = 525,        // Planetary body (`planetbody pl1 vesc 10`)!
    INT_ASK_PLANET = 526,           // Escape velocity mass/radius scaling & Schwarzschild black-hole radius!
    INT_ASSERT_PT2D = 527,          // 2D geometry point (`pt2d pt1 2 3`)!
    INT_ASK_PT2D = 528,             // 2D Manhattan L1, Chebyshev L_inf & Euclidean L2 distances!
    // v35 Stage 5c intents: Kernel 29 pure-GPU axonal-delay spiking graph wavefront + 7 deep STEM domains!
    INT_ASSERT_GRAPHWAVE = 529,     // Live STDP weighted edge write into L2 PW11 (`graphwave gwedge gn4 to gn5 cost 4`)!
    INT_ASK_GRAPHWAVE = 530,        // Pure-GPU axonal-delay shortest-path distance, hop count, next-hop & GABA_A blocked detour!
    INT_ASSERT_ISOBAR = 531,        // Isobaric thermodynamic piston (`isobaric piston1 pressk 50 dvol 4`)!
    INT_ASK_ISOBAR = 532,           // Isobaric work `P*dV` & isothermal entropy change `Q/T`!
    INT_ASSERT_FARADAY = 533,       // Electromagnetic induction coil (`faraday coil1 nturns 50 dflux 12 dtime 3`)!
    INT_ASK_FARADAY = 534,          // Faraday induced EMF & ideal transformer secondary voltage!
    INT_ASK_DISCRETE_V35 = 535,     // Catalan numbers `C_n`, stars-and-bars `C(n+k-1,k-1)` & generalized pigeonhole!
    INT_ASK_DIGLOGIC = 536,         // 2's complement signed decode, NAND & NOR universal gates!
    INT_ASSERT_BODE = 537,          // 2nd-order mechanical/control oscillator (`bode sys1 mass 2 stiff 32 damp 16`)!
    INT_ASK_BODE = 538,             // Natural frequency `sqrt(k/m)` & damping classification (`critdamp`/`underdamp`/`overdamp`)!
    INT_ASSERT_CPUSCHED = 539,      // OS CPU scheduler job bursts (`cpusched sched1 proc1 9 proc2 3 proc3 6`)!
    INT_ASK_CPUSCHED = 540,         // SJF average wait time, average turnaround & total burst!
    INT_ASSERT_DEBROGLIE = 541,     // Quantum particle (`debroglie part1 planck 600 mom 15`)!
    INT_ASK_DEBROGLIE = 542,        // de Broglie wavelength `h/p` & Bohr hydrogen level `13600/n^2`!
    // v36 Stage 5d intents: Kernel 30 pure-GPU spiking 3-SAT attractor, Tier-2 16k HBM lifelong episodic store, free why/how causal unseen prose & 6 deep STEM domains!
    INT_ASSERT_SATSOLVE = 543,      // Live 1-shot GPU STDP SAT clause imprint into L2 PW13 (`satsolve satclause not sv3`)!
    INT_ASK_SATSOLVE = 544,         // Pure-GPU spiking SAT satisfiability (`satcheck`), forced backbone (`satval sv3`) & reset!
    INT_ASK_SEQ_CAUSE = 545,        // Zero-template natural `why` & `what caused` over unseen K27-parsed causal stories!
    INT_ASSERT_HBM = 546,           // Tier-2 HBM lifelong episodic store (`hbmarchive store era1 vault opal` / `pad 4500`)!
    INT_ASK_HBM = 547,              // Tier-2 HBM deep archive recall past 4,096 L2 ceiling (`hbmarchive recall` / `count`)!
    INT_ASSERT_REYNOLDS = 548,      // Fluid flow (`reynolds pipe2 dens 1000 vel 2 diam 4 visc 2`)!
    INT_ASK_REYNOLDS = 549,         // Reynolds number `rho*v*L/mu` & laminar/turbulent flow regime classification!
    INT_ASK_HAMMING_V36 = 550,      // Binary Hamming distance (`hamdist`), even parity bit (`parity`) & 1-bit error check!
    INT_ASSERT_MAT3 = 551,          // 3x3 diagonal/upper-triangular matrix (`mat3 m3a 2 3 5`)!
    INT_ASK_MAT3 = 552,             // 3x3 trace, determinant & invertibility check!
    INT_ASSERT_KINETICS = 553,      // First-order chemical reaction (`kinetics rxn1 krate 4 conc 25`)!
    INT_ASK_KINETICS = 554,         // First-order rate `k*[A]`, doubled concentration rate & half-life `693/k`!
    INT_ASK_CIDR = 555,             // Networking CIDR subnet usable hosts `2^(32-p)-2`, host bits & block size!
    INT_ASSERT_BEAM = 556,          // Simply supported center-loaded beam (`beam bm1 loadf 40 span 8 capm 160`)!
    INT_ASK_BEAM = 557,             // Beam max bending moment `F*L/4`, reaction shear `F/2` & safety factor `capM/M`!
    // v37 STAGE 6 CROSSING (Multimodal World & Code Models!): Kernel 31 pure-GPU 16x16 spiking V1/V4 retinal vision, self-teaching curiosity reader, code execution/trace & 6 Stage-6 STEM domains!
    INT_ASSERT_VISUAL = 558,        // Kernel 31 Mode 0: live 1-shot GPU STDP V1->V4 visual prototype teaching into L2 PW24 (`retina visteach vislshape`)!
    INT_ASK_VISUAL = 559,           // Kernel 31 Mode 1: pure-GPU 16x16 spiking retinal shape classification, wavefront object counting & 2D spatial relations!
    INT_ASSERT_STUDY = 560,         // Autonomous multi-clause self-teaching passage reader (`study zephryx is soarer has feathers eats beetles` / `study zephryx lives ravine`)!
    INT_ASK_STUDY = 561,            // Autonomous epistemic curiosity gap detection (`study studygap zephryx`) & synthesized comprehension (`study studyinfo zephryx`)!
    INT_ASSERT_CODEPROG = 562,      // Imperative algorithmic code builder & hot-patcher (`codeprog prog1 codeinit 2`, `codeadd 3`, `codemul 4`, `codesub 5`, `codefix 2 5`)!
    INT_ASK_CODEPROG = 563,         // Code execution (`coderun`), intermediate trace (`codestep`), invariant check (`codecheck`)!
    INT_ASSERT_CHORD = 564,         // Musical acoustics fundamental (`chord chd1 basehz 200`)!
    INT_ASK_CHORD = 565,            // Octave doubling (`2*f`), perfect fifth (`3*f/2`) & 3rd harmonic overtone (`3*f`)!
    INT_ASSERT_ROTOR = 566,         // 3D rigid body (`rotor rt1 armrad 4 rforce 15 inertia 10 omega 6`)!
    INT_ASK_ROTOR = 567,            // Torque `r*F`, angular momentum `I*w` & rotational kinetic energy `I*w^2/2`!
    INT_ASSERT_COMPILER = 568,      // Compiler AST/register state (`compiler ast1 astleaf 8 astops 7 livevars 5 nregs 4`)!
    INT_ASK_COMPILER = 569,         // Balanced binary AST depth `log2(leaves)`, register spills `max(0, live-regs)` & total AST nodes!
    INT_ASSERT_DNA = 570,           // Molecular biology DNA sequence (`dnaseq dna1 gccnt 12 dnalen 30 glucose 3`)!
    INT_ASK_DNA = 571,              // GC content %, amino-acid codons `len/3` & aerobic ATP yield `36*glucose`!
    INT_ASSERT_CIPHER = 572,        // Symmetric cipher state (`cipher cip1 plainv 42 xorkey 19 cshift 5`)!
    INT_ASK_CIPHER = 573,           // XOR one-time-pad ciphertext, exact XOR decrypt invertibility & modular Caesar mod-26 shift!
    INT_ASSERT_ROBOT = 574,         // 2-link planar robot arm & wheeled rover (`robot arm1 arml1 10 arml2 6 wcirc 12 wturns 5`)!
    INT_ASK_ROBOT = 575,            // Max workspace reach `L1+L2`, inner deadzone `|L1-L2|` & wheel odometry `circ*turns`!
    // v38 STAGE 6b (Multimodal Audio/Prosody, Autonomous Textbook Induction/Self-Quiz & Counterfactual Code Debugging!)
    INT_ASSERT_AUDIO = 576,         // Kernel 32 Mode 0: live 1-shot GPU STDP A1->Belt auditory teaching into L2 PW27 (`cochlea audteach audmarch`)!
    INT_ASK_AUDIO = 577,            // Kernel 32 Mode 1: pure-GPU 16x16 spiking cochlear chord/prosody scan, beat counter & pitch contour!
    INT_ASSERT_TEXTBOOK = 578,      // Multi-sentence unseen science textbook causal law induction (`textbook lumiglass amplifies glintray byfactor 4 blockedby fogshroud`)!
    INT_ASK_TEXTBOOK = 579,         // Autonomous causal prediction (`tbpredict`), blocker handling & self-generated verification quiz (`selfquiz`)!
    INT_ASSERT_EMPATHY = 580,       // Multimodal prosody-context utterance register (`empathy chat1 prosody audrise mood tense`)!
    INT_ASK_EMPATHY = 581,          // Prosody-aware pragmatic tone & irony/sarcasm detector (`empathy tonecheck for chat1`)!
    INT_ASSERT_SOLID3D = 582,       // 3D solid & polyhedron (`solid3d sld1 basea 12 heightv 9 verts 8 edges 12`)!
    INT_ASK_SOLID3D = 583,          // Prism volume `B*h`, pyramid volume `B*h/3` & Euler polyhedron faces `E - V + 2`!
    INT_ASSERT_GRAPHC = 584,        // Graph combinatorics (`graphc gr1 nverts 6 nedges 10`)!
    INT_ASK_GRAPHC = 585,           // Complete graph edges `n(n-1)/2`, spanning tree edges `n-1` & planar Euler faces `E-V+2`!
    INT_ASSERT_DSP = 586,           // 3-tap discrete signal & FIR kernel (`dsp sig1 sx0 2 sx1 4 sx2 6 tap0 3 tap1 2 tap2 1`)!
    INT_ASK_DSP = 587,              // 3-tap FIR discrete convolution dot product, signal energy `sum x^2` & moving average!
    INT_ASSERT_ORGCHEM = 588,       // Organic chemistry & stoichiometry (`orgchem alk1 carbons 4 molcnt 6 liters 2`)!
    INT_ASK_ORGCHEM = 589,          // Alkane hydrogens `2C+2`, alkane molar mass `12C+(2C+2)` & molarity `mol/L`!
    INT_ASSERT_DBQUERY = 590,       // Relational DB query (`dbquery dbq1 rowsa 20 rowsb 10`)!
    INT_ASK_DBQUERY = 591,          // Hash join cost `M+N`, nested-loop cost `M*N` & speedup ratio `nest/hash`!
    // v39 STAGE 6c (Cross-Modal Vision+Audio Binding & GPU Goal Stack K33, Socratic Coaching/Humility, 3D Contraption Sim & 5 Deep Domains!)
    INT_ASSERT_XMODAL = 592,        // Kernel 33 Mode 0/3/4/6: live STDP cross-modal bind (`xmodal xbind vislshape audmarch`), GPU goal push/pop!
    INT_ASK_XMODAL = 593,           // Kernel 33 Mode 1/2/5: cross-modal fusion (`xfuse`), synesthetic W^T recall (`xsynest`) & goal stack peek (`gpeek`)!
    INT_ASSERT_COACH = 594,         // Sustained Socratic user-goal coaching (`coach usergoal buildbridge blocker missingrope` / `coach resolved missingrope`)!
    INT_ASK_COACH = 595,            // Proactive next-step guidance (`coach nextstep`) & calibrated epistemic humility (`coach knowcheck darkmatter`)!
    INT_ASSERT_CONTRAPTION = 596,   // 3D compound mechanical contraption (`contraption rigalpha gearin 10 gearout 30 levera 6 leverb 2 pulleys 4`)!
    INT_ASK_CONTRAPTION = 597,      // Compound mechanical advantage, lifted load & counterfactual stress overload test!
    INT_ASSERT_DELIBERATE = 598,    // Multi-perspective tradeoff deliberation (`deliberate billalpha proside speed merit 8 conside cost hazardc 5`)!
    INT_ASK_DELIBERATE = 599,       // Net utility verdict (`approve`/`decline`), net margin & explicit tradeoff justification!
    INT_ASSERT_ASTRO = 600,         // Orbital astrophysics (`astro plntalpha gmval 400 radv 10 spandist 600 cspeed 30`)!
    INT_ASK_ASTRO = 601,            // Surface gravity `GM/R^2`, light-travel time `dist/c` & circular orbital speed `sqrt(GM/R)`!
    INT_ASSERT_STOCHD = 602,        // Discrete stochastics (`stochd jaralpha ntrials 100 probpct 25`)!
    INT_ASK_STOCHD = 603,           // Geometric expected trials `100/p`, binomial mean `n*p/100` & scaled variance `n*p*(100-p)/100`!
    INT_ASSERT_ELECGRID = 604,      // Power grid & transformer (`elecgrid cktalpha emfvolt 120 resohm 10 iprim 5 coilp 400 coils 100`)!
    INT_ASK_ELECGRID = 605,         // Power dissipation `V^2/R`, Ohm current `V/R` & ideal step-down secondary current `I_p*N_p/N_s`!
    INT_ASSERT_DISTSYS = 606,       // Distributed consensus cluster (`distsys clualpha nnodes 7`)!
    INT_ASK_DISTSYS = 607,          // Raft/Paxos quorum `n/2+1`, BFT max Byzantine faults `(n-1)/3` & crash-fault partition tolerance `(n-1)/2`!
    INT_ASSERT_INVOPT = 608,        // Supply-chain inventory economics (`invopt skualpha unitp 50 unitc 30 fixcost 400`)!
    INT_ASK_INVOPT = 609,           // Gross margin %, break-even units `fix/(p-c)` & unit profit margin `p-c`!
    // v40 STAGE 7 CROSSING (General Conversational Spiking Mind!): Kernel 34 Pure-GPU Spiking Dialogue Coherence Cortex, Free-Form Open Multi-Turn Synthesis, Pun Wit, Persona, Hypothesis Tests, Carnot, Master Theorem, Immunology & Ethics!
    INT_ASSERT_DIALOGUE = 610,      // Kernel 34 Mode 0/2/4: live STDP pragmatic mode teach (`dlgteach`), live stance imprint (`dlgstance`), reset (`dlgreset`)!
    INT_ASK_DIALOGUE = 611,         // Kernel 34 Mode 1/3: pragmatic mode + topic-shift detection (`dlgmode`) & stance contradiction check (`dlgcheck`)!
    INT_ASK_OPENCHAT = 612,         // Stage-7 free-form open conversational synthesis (`what do you think about X?`, `tell me more about it?`, `how are X and Y connected?`)!
    INT_ASSERT_PUNWIT = 613,        // Analogical humor / double-meaning pun setup (`punwit punalpha punword barkctx glossone treecoat glosstwo dogcall`)!
    INT_ASK_PUNWIT = 614,           // Context-dependent pun sense disambiguation (`punsense`) & wit explanation (`punwhy`)!
    INT_ASSERT_PERSONA = 615,       // Long-horizon user persona & preference revision (`persona usrbeta fondof stargaze shunof loudnoise` / `persona update usrbeta fondof botany`)!
    INT_ASK_PERSONA = 616,          // Proactive personalized suggestion (`persona suggest`) & avoidance check (`persona avoid`)!
    INT_ASSERT_STATTEST = 617,      // Statistical hypothesis test (`stattest expalpha sampm 56 nullm 50 stde 2`)!
    INT_ASK_STATTEST = 618,         // Z-score `(x-mu)/se`, two-tailed significance verdict (`rejectnull`/`retainnull`) & 95% CI upper bound (`x + 2*se`)!
    INT_ASSERT_THERMOC = 619,       // Thermodynamics Carnot cycle (`thermoc engbeta hotkel 500 coldkel 200 qinval 1000`)!
    INT_ASK_THERMOC = 620,          // Carnot efficiency %, reversible heat-engine work & entropy change `Q/T`!
    INT_ASSERT_MASTERREC = 621,     // Master Theorem recurrence (`masterrec recalpha subcalls 4 divfactor 2 polyexp 1`)!
    INT_ASK_MASTERREC = 622,        // Critical exponent `log_b(a)`, asymptotic regime (`leafheavy`/`evenwork`/`rootheavy`) & work at `nval`!
    INT_ASSERT_IMMUNOL = 623,       // Immunology/epidemiology pathogen (`immunol pathalpha rzero 4 vaxpct 80`)!
    INT_ASK_IMMUNOL = 624,          // Herd immunity threshold %, scaled `R_eff*100` & containment status (`contained`/`spreading`)!
    INT_ASSERT_ETHICF = 625,        // Ethical dilemma case (`ethicf casalpha livesok 10 harmcnt 1 dutyrule 0`)!
    INT_ASK_ETHICF = 626,           // Utilitarian net welfare, deontological verdict (`dutyforbid`/`dutyallow`) & framework tension (`ethconflict`/`ethaligned`)!
    // v41 STAGE 7b (Spiking Narrative, Metaphor & Socratic Self-Critique + Universal Open Chat + Autonomous Science Loop!): Kernel 35 on L2 PW27[256..511] + 10 Deep Stage-7b Domains (D381-D390)!
    INT_ASSERT_NMETAPHOR = 627,     // Kernel 35 Mode 0: live 1-shot GPU STDP conceptual metaphor blend (`metaphor metteach metcompass mnorth mgoal`)!
    INT_ASK_NMETAPHOR = 628,        // Kernel 35 Mode 1/3/4: forward metaphor blend (`metblend`), backward W^T explain (`metexplain`), 2-hop narrative arc (`metplot`) & Socratic self-critique (`metcritique`)!
    INT_ASSERT_SCILOOP = 629,       // Autonomous scientific research loop (`sciloop labalpha hypfast 80 hypslow 40 scitest 82`)!
    INT_ASK_SCILOOP = 630,          // Hypothesis confirmation (`hconfirm`), falsification (`hfalsify`) & residual error (`reserr`)!
    INT_ASSERT_TALEBOX = 631,       // Compositional multi-sentence story seed (`talebox talealpha taleseed mira dragon lantern questend moralbold`)!
    INT_ASK_TALEBOX = 632,          // Compositional story narration (`talenarr`) & structural moral extraction (`talemoral`)!
    INT_ASSERT_LINPROG = 633,       // Linear programming & resource duality (`linprog lpalpha budget 20 profit 5 cost 4`)!
    INT_ASK_LINPROG = 634,          // Optimal max profit (`optprofit`), shadow price (`shadowpx`) & binding constraint verdict (`bindstate`)!
    INT_ASSERT_SPECTRAL = 635,      // Spectral Fourier harmonics & sampling (`spectral sigalpha basehz 50 samplehz 180`)!
    INT_ASK_SPECTRAL = 636,         // 3rd harmonic (`harmthird`), Nyquist minimum rate (`nyqmin`) & aliasing verdict (`foldverdict`)!
    INT_ASSERT_ALLELEHW = 637,      // Hardy-Weinberg population genetics (`allelehw genealpha dompct 60 fitdom 90 fitrec 70`)!
    INT_ASK_ALLELEHW = 638,         // Heterozygote carrier % (`carpct`), homozygous recessive % (`hompct`) & selection direction (`seldir`)!
    INT_ASSERT_CRYPTODL = 639,      // Diffie-Hellman discrete-log key exchange (`cryptodl dhalpha modp 23 genv 5 seca 6 secb 15`)!
    INT_ASK_CRYPTODL = 640,         // Alice public key (`puba`), Bob public key (`pubb`) & shared secret (`dhsecret`)!
    INT_ASSERT_TROPHIC = 641,       // Food-web trophic pyramid (`trophic webalpha plantbio 1000 apexcnt 0`)!
    INT_ASK_TROPHIC = 642,          // Herbivore biomass (`grazerbio`), apex biomass (`predbio`) & cascade verdict (`webshift`)!
    // v42 STAGE 7c (400 DOMAINS MILESTONE!): Kernel 36 Pure-GPU Belief-Graph & Scientific Debate Cortex + Rich Multi-Facet Unscripted Chat + Peer Review, Proofs, StatMech, VCG, LIF Neurobio, LQR Control & Hohmann Orbits!
    INT_ASSERT_BELGRAPH = 643,      // Kernel 36 Mode 0/2: live 1-shot GPU STDP theory support/refutation (`belteach`/`belrefute`) & peer credibility calibration (`belcred`)!
    INT_ASK_BELGRAPH = 644,         // Kernel 36 Mode 1/3: pure-GPU belief-graph debate verdict (`belcheck`) & peer credibility adjudication (`beldebate`)!
    INT_ASSERT_PEERREV = 645,       // Autonomous scientific peer-review (`peerrev pubalpha origv 50 replv 48 nsamp 100`)!
    INT_ASK_PEERREV = 646,          // Replication consistency (`repcheck`), pooled mean (`poolmean`) & statistical power (`powcheck`)!
    INT_ASSERT_PROOFFORM = 647,     // Formal inductive series proof (`proofform seralpha nterms 10 firstt 2 lastt 20 baseok 1`)!
    INT_ASK_PROOFFORM = 648,        // Closed-form arithmetic sum (`arithsum`), triangular sum (`trisum`) & induction validity (`indverdict`)!
    INT_ASSERT_STATMECH = 649,      // Statistical mechanics (`statmech sysalpha statesg 2 npart 4 dofval 6 ktval 10`)!
    INT_ASK_STATMECH = 650,         // Microstate count (`microcnt`), Boltzmann log2 entropy bits (`boltzbits`) & equipartition energy (`equipart`)!
    INT_ASSERT_GAMEMECH = 651,      // VCG Groves-Clarke mechanism design (`gamemech mechalpha bidone 50 bidtwo 35 bidthree 20`)!
    INT_ASK_GAMEMECH = 652,         // Social welfare (`socwelf`), Clarke pivot VCG tax (`vcgtax`) & truthfulness (`icverdict`)!
    INT_ASSERT_NEUROBIO = 653,      // Biophysical LIF neuron (`neurobio cellalpha resmem 5 capmem 4 incurr 6 vthresh 25`)!
    INT_ASK_NEUROBIO = 654,         // Membrane time constant `Rm*Cm` (`tautime`), depolarization `I*Rm` (`depolv`) & spike verdict (`spikeverdict`)!
    INT_ASSERT_CONTROLOPT = 655,    // Optimal LQR feedback control (`controlopt ctlalpha polea 3 gaink 8`)!
    INT_ASK_CONTROLOPT = 656,       // Closed-loop pole `a-k` (`cleig`), stability verdict (`stabverdict`) & damping margin `k-a` (`dampmargin`)!
    INT_ASSERT_ASTRODYN = 657,      // Hohmann orbital transfer (`astrodyn orbalpha radin 100 radout 200 dvhave 50 dvneed 40`)!
    INT_ASK_ASTRODYN = 658,         // Transfer semi-major axis (`semiax`), eccentricity % (`eccpct`) & delta-v fuel margin verdict (`fuelverdict`)!
    // v43 STAGE 8 CROSSING (COLLECTIVE SCIENTIFIC MINDS — 410 DOMAINS!): Kernel 37 Pure-GPU Collective Scientific Discovery & Compositional Theorem Prover + Socratic Curiosity + 7 Deep Stage-8 Domains!
    INT_ASSERT_COLLDISC = 659,      // Kernel 37 Mode 0/2: live 1-shot GPU STDP specialist vote (`collteach`/`collrefute`) & compositional theorem write (`collmake`)!
    INT_ASK_COLLDISC = 660,         // Kernel 37 Mode 1/3: pure-GPU collective consensus (`collpool`/`collvote`), forward derivation (`collderive`) & backward W^T proof (`collprove`)!
    INT_ASSERT_SPECCODE = 661,      // Spec-to-code synthesis (`speccode fnalpha inx 4 targety 14 slopek 3 biasb 2`)!
    INT_ASK_SPECCODE = 662,         // Spec verification (`specverdict`), generalization (`evalnext`) & synthesized op count (`opcount`)!
    INT_ASSERT_QGATE = 663,         // Quantum gate state (`qgate qsysalpha probzero 64 ampplus 5 ampminus 3`)!
    INT_ASK_QGATE = 664,            // Excited probability (`probone`), constructive interference (`intpower`) & entanglement verdict (`entverdict`)!
    INT_ASSERT_CHANCODE = 665,      // Linear block channel code (`chancode blkalpha kbits 12 nbits 16 dmin 4`)!
    INT_ASK_CHANCODE = 666,         // Code rate % (`coderate`), parity bits (`parbits`) & error detection capacity (`detcap`)!
    INT_ASSERT_HEATPDE = 667,       // 1D finite-difference heat rod (`heatpde rodalpha tleft 80 tmid 40 tright 80 rcoeff 25`)!
    INT_ASK_HEATPDE = 668,          // Discrete Laplacian (`laplace`), next-step temp (`tnext`) & CFL stability verdict (`cflverdict`)!
    INT_ASSERT_COALBID = 669,       // Cooperative coalition game (`coalbid grpalpha vala 20 valb 30 valab 90`)!
    INT_ASK_COALBID = 670,          // Coalition surplus (`coalsurp`), Shapley value A (`shapa`) & core stability verdict (`coreverdict`)!
    INT_ASSERT_EPIOPT = 671,        // Epidemiological intervention (`epiopt outalpha rbase 5 covpct 60 effpct 80`)!
    INT_ASK_EPIOPT = 672,           // Effective immunity % (`immpct`), scaled R_eff (`reffsc`) & outbreak verdict (`epiverdict`)!
    INT_ASSERT_COLLLAB = 673,       // Multi-lab research society (`colllab socalpha labone 60 labtwo 64 labthree 62 soloest 40`)!
    INT_ASK_COLLLAB = 674,          // Ensemble consensus mean (`ensmean`), collective lift (`collgain`) & publication verdict (`pubverdict`)!
    // v44 STAGE 8b (CONTINUAL SELF-RESEARCH & MULTI-SENTENCE SPIKING FLUENCY — 420 DOMAINS!): Kernel 38 Pure-GPU Optimal Experiment Design & Counterfactual Code Patching + 7 Deep Stage-8b Domains!
    INT_ASSERT_SELFRES = 675,       // Kernel 38 Mode 0/2: live 1-shot GPU STDP experiment imprint (`resexp`) & counterfactual code patch write (`reslearn`)!
    INT_ASK_SELFRES = 676,          // Kernel 38 Mode 1/3: pure-GPU optimal discriminative experiment (`resdesign`), forward patch synthesis (`respatch`) & backward W^T root cause (`reswhy`)!
    INT_ASSERT_KALMAN = 677,        // 1D discrete Kalman filter (`kalman filtalpha priorx 40 measz 60 prip 30 measr 20`)!
    INT_ASK_KALMAN = 678,           // Kalman gain % (`kgain`), posterior state (`postx`) & posterior variance (`postp`)!
    INT_ASSERT_BAYESNET = 679,      // Causal Bayesian network (`bayesnet netalpha priora 50 pbgivena 80 pbgivenna 20`)!
    INT_ASK_BAYESNET = 680,         // Marginal P(B) % (`margb`), posterior P(A|B) % (`postab`) & likelihood verdict (`bnverdict`)!
    INT_ASSERT_FLUIDYN = 681,       // Poiseuille fluid pipe (`fluidyn tubealpha qbase 10 radmult 2 velin 6 pstat 200`)!
    INT_ASK_FLUIDYN = 682,          // Poiseuille R^4 flow (`poiseq`), continuity velocity (`velout`) & cavitation verdict (`cavverdict`)!
    INT_ASSERT_CRYPTOECC = 683,     // Schnorr zero-knowledge proof (`cryptoecc zkalpha primeq 23 genk 5 secx 4 chalc 3`)!
    INT_ASK_CRYPTOECC = 684,        // Public key (`pubkeyy`), ZK response (`zkresp`) & algebraic proof verification (`zkverdict`)!
    INT_ASSERT_OPTPATH = 685,       // Autonomous rover trajectory (`optpath rovalpha distm 120 speedv 12 dragk 5 battj 1000`)!
    INT_ASK_OPTPATH = 686,          // Trip duration (`triptime`), quadratic drag energy (`dragwork`) & battery range verdict (`rngverdict`)!
    INT_ASSERT_COMPILEROPT = 687,   // Optimizing compiler loop (`compileropt passalpha iters 100 invops 4 varops 6 deadops 3`)!
    INT_ASK_COMPILEROPT = 688,      // Raw loop ops (`rawops`), LICM hoisted ops (`hoistops`) & total saved ops (`savedops`)!
    INT_ASSERT_ECONMACRO = 689,     // Monetary policy economy (`econmacro macalpha inflp 6 targetp 2 neutralr 2 outgap 4`)!
    INT_ASK_ECONMACRO = 690,        // Inflation gap (`inflgap`), Taylor Rule policy rate (`taylorr`) & Fisher real interest (`realrate`)!
    // v45 STAGE 8c (PURE-GPU NEURAL SYMBOLIC ALGEBRA/CALCULUS & SPONTANEOUS LIVE CONCEPT TEACHING — 430 DOMAINS!): Kernel 39 on L2 PW13[0..255, 0..255] + 7 Deep Stage-8c STEM Domains!
    INT_ASSERT_SYMCALC = 691,       // Kernel 39 Mode 0: live 1-shot GPU STDP polynomial imprint into L2 PW13 (`symadd polyomega 4 2 3`)!
    INT_ASK_SYMCALC = 692,          // Kernel 39 Mode 1/2/3: pure-GPU symbolic derivative (`symdiff`), definite integral (`symint`) & Horner evaluation (`symeval`)!
    INT_ASSERT_KLDIV = 693,         // Information-theoretic KL divergence (`kldiv distalpha 4 6 50`)!
    INT_ASK_KLDIV = 694,            // Excess bits (`klexcess`), cross-entropy (`klcross`) & tightness verdict (`klstat`)!
    INT_ASSERT_BODEMARG = 695,      // Control-theory Bode / Nyquist stability (`bodemarg loopalpha 120 8 50`)!
    INT_ASK_BODEMARG = 696,         // Phase margin (`pmdeg`), gain margin (`gmdb`) & closed-loop stability verdict (`ctrlstat`)!
    INT_ASSERT_QTUNNEL = 697,       // Quantum barrier tunneling (`qtunnel wellalpha 10 6 2`)!
    INT_ASK_QTUNNEL = 698,          // Energy deficit (`qdef`), WKB decay exponent (`qkappa`) & transmission verdict (`qtunstat`)!
    INT_ASSERT_RELINV = 699,        // Special relativity energy-momentum invariant (`relativ partalpha 30 40 80`)!
    INT_ASK_RELINV = 700,           // Total relativistic energy (`reletot`), kinetic energy (`relkin`) & relativistic regime (`relstat`)!
    INT_ASSERT_RAFTCON = 701,       // Raft distributed consensus cluster (`raftcon clusalpha 7 5 4`)!
    INT_ASK_RAFTCON = 702,          // Majority quorum (`raftquor`), crash fault tolerance (`raftfault`) & commit verdict (`raftstat`)!
    INT_ASSERT_REGALLOC = 703,      // Compiler Chaitin register coloring (`regalloc cfgalpha 10 8 15`)!
    INT_ASK_REGALLOC = 704,         // Register spill count (`spillcnt`), spill cost overhead (`regover`) & fit verdict (`regstat`)!
    INT_ASSERT_EVOGAME = 705,       // Evolutionary game theory Hawk-Dove (`evogame popalpha 40 80 30`)!
    INT_ASK_EVOGAME = 706,          // ESS hawk equilibrium % (`esshawk`), peaceful dove share % (`essdove`) & selection pressure (`essstat`)!
    // v46 STAGE 8d (PURE-GPU AST PROGRAM SYNTHESIS/EXECUTION & FREE-FORM DEBATE/COUNTERFACTUALS — 440 DOMAINS!): Kernel 40 on L2 PW13[0..255, 256..511] + 7 Deep Stage-8d STEM Domains!
    INT_ASSERT_PROGAST = 707,       // Kernel 40 Mode 0/3: live 1-shot GPU STDP AST imprint (`astadd`) & 256-thread parallel program synthesis (`astsynth`)!
    INT_ASK_PROGAST = 708,          // Kernel 40 Mode 1/2: pure-GPU static type check (`astverify`) & symbolic execution (`astexec`)!
    INT_ASSERT_MERKLE = 709,        // Cryptographic Merkle tree (`merkletree treealpha 16 5 6`)!
    INT_ASK_MERKLE = 710,           // Proof sibling count (`mproof`), total tree nodes (`mtotal`) & speed verdict (`mstat`)!
    INT_ASSERT_ROCKET = 711,        // Tsiolkovsky rocket propulsion (`rocketisp craftalpha 300 20 5000`)!
    INT_ASK_ROCKET = 712,           // Exhaust velocity (`exvel`), delta-v (`deltav`) & orbital attainment verdict (`orbstat`)!
    INT_ASSERT_LITTLEQ = 713,       // Operations Research Little's Law queue (`queueing srvalpha 6 4 30`)!
    INT_ASK_LITTLEQ = 714,          // Mean queue length (`qlen`), spare buffer (`qspare`) & stability verdict (`qstat`)!
    INT_ASSERT_ENZYME = 715,        // Bioenergetics Michaelis-Menten enzyme (`enzymemm enzalpha 120 20 10`)!
    INT_ASK_ENZYME = 716,           // Reaction velocity (`enzvel`), saturation % (`enzpct`) & kinetic regime (`enzstat`)!
    INT_ASSERT_QUORUM = 717,        // Distributed quorum consistency (`quorumrw dbalpha 5 3 3`)!
    INT_ASK_QUORUM = 718,           // Quorum overlap `R+W-N` (`qovlp`), sum `R+W` (`qrws`) & consistency guarantee (`qcons`)!
    INT_ASSERT_SNRDB = 719,         // Digital Signal Processing decibel SNR (`snrdb linkalpha 45 15 20`)!
    INT_ASK_SNRDB = 720,            // Signal-to-noise ratio dB (`snrval`), fade margin dB (`fademarg`) & link lock verdict (`snrstat`)!
    INT_ASSERT_CFGPARSE = 721,      // Formal Language Chomsky CFG (`cfgparse gramalpha 6 4 15`)!
    INT_ASK_CFGPARSE = 722,         // CNF derivation steps `2n-1` (`cnfsteps`), CYK chart cells `n(n+1)/2` (`cykcells`) & parse budget verdict (`cfgstat`)!
    // v47 STAGE 8e (PURE-GPU NEURAL LINEAR ALGEBRA & MULTI-TURN STORY CO-WRITING / SELF-QUESTIONING — 450 DOMAINS!): Kernel 41 on L2 PW13[256..511, 0..255] + 7 Deep Stage-8e STEM Domains!
    INT_ASSERT_LINALG = 723,        // Kernel 41 Mode 0/3: live 1-shot GPU STDP matrix imprint & Cramer solve (`linalg linsolve linomega a b c d r0 r1`)!
    INT_ASK_LINALG = 724,           // Kernel 41 Mode 1/2/3: pure-GPU det (`lindet`), trace (`lintr`), matvec (`linvec`) & Cramer solve (`linsolve`)!
    INT_ASSERT_RSAKEY = 725,        // Cryptography RSA keygen (`rsakey keyalpha 5 11 7`)!
    INT_ASK_RSAKEY = 726,           // Modulus `p*q` (`rsamod`), Euler totient `(p-1)(q-1)` (`rsaphi`) & coprime check (`rsastat`)!
    INT_ASSERT_AEROLIFT = 727,      // Fluid mechanics aerodynamic lift (`aerolift wingalpha 2 10 4`)!
    INT_ASK_AEROLIFT = 728,         // Dynamic pressure `rho*v^2/2` (`dynpres`), lift force `q*A` (`liftfrc`) & flight verdict (`aerostat`)!
    INT_ASSERT_CACHEGEO = 729,      // Computer architecture set-associative cache (`cachegeo l1alpha 64 4 64`)!
    INT_ASK_CACHEGEO = 730,         // Total bytes (`cachcap`), total lines (`cachlin`) & conflict verdict (`cachstat`)!
    INT_ASSERT_KEPLERORB = 731,     // Astrophysics Kepler 3rd-law orbit (`keplerorb moonalpha 4 1`)!
    INT_ASK_KEPLERORB = 732,        // `T^2 = a^3/M` (`orbt2`), period `T` (`orbper`) & orbital regime (`orbreg`)!
    INT_ASSERT_QUANTADC = 733,      // DSP ADC quantization & SQNR (`quantadc adcalpha 8 1024`)!
    INT_ASK_QUANTADC = 734,         // Levels `2^b` (`qlevels`), SQNR `6b+2` dB (`qsqnr`) & fidelity verdict (`qadcstat`)!
    INT_ASSERT_NERNST = 735,        // Electrochemistry Nernst cell EMF (`nernstcell galvalpha 1100 2 60`)!
    INT_ASK_NERNST = 736,           // Cell EMF (`cellemf`), electrical work `n*E` (`cellwrk`) & galvanic verdict (`cellstat`)!
    INT_ASSERT_COURNOT = 737,       // Microeconomics Cournot duopoly (`cournot firmalpha 100 10`)!
    INT_ASK_COURNOT = 738,          // Output `q* = (A-c)/3` (`courqty`), price `P* = A-2q*` (`courprc`) & equilibrium verdict (`courstat`)!
    // v48 STAGE 8f (PURE-GPU SPIKING PROBABILISTIC BAYES-NET & SOCRATIC SELF-CRITIQUE — 460 DOMAINS!): Kernel 42 on FINAL FREE L2 PW13 Q3 + 7 Deep Stage-8f STEM Domains!
    INT_ASSERT_BNET = 739,          // Kernel 42 Mode 0/3: live 1-shot GPU STDP Bayes-net CPT imprint & decision (`bnet bnetlive bnomega 50 80 20 20 80`)!
    INT_ASK_BNET = 740,             // Kernel 42 Mode 1/2/3: pure-GPU `P(H|E)` (`bnetpos`), `P(H|~E)` (`bnetneg`) & optimal Bayes decision (`bnetdec`)!
    INT_ASSERT_BLACKBODY = 741,     // Thermal radiation Stefan-Boltzmann & Wien (`blackbody staralpha 10 2 10`)!
    INT_ASK_BLACKBODY = 742,        // Wien peak nm (`bbwien`), Stefan T^4 power (`bbpower`) & spectral band (`bbstat`)!
    INT_ASSERT_HAMMCODE = 743,      // Error-correcting codes Hamming distance (`hammcode codealpha 5`)!
    INT_ASK_HAMMCODE = 744,         // Detectable errors `d-1` (`hamdet`), correctable `(d-1)/2` (`hamcor`) & ECC capability (`hamstat`)!
    INT_ASSERT_LENSOPT = 745,       // Thin-lens geometric optics (`lensopt optalpha 20 60`)!
    INT_ASK_LENSOPT = 746,          // Focal length `u*v/(u+v)` (`foclen`), magnification `v/u` (`lensmag`) & image type (`lensstat`)!
    INT_ASSERT_AMORTIZE = 747,      // Dynamic array amortized growth (`amortize vecalpha 32 2`)!
    INT_ASK_AMORTIZE = 748,         // Grown capacity (`arrgrow`), copied elements (`arrcopy`) & complexity class (`arrstat`)!
    INT_ASSERT_PENDULUM = 749,      // Harmonic oscillator stiffness & energy (`pendulum oscalpha 100 4 3`)!
    INT_ASK_PENDULUM = 750,         // Angular freq^2 `k/m` (`pendfreq`), potential energy `k*A^2/2` (`pendene`) & stiffness regime (`pendstat`)!
    INT_ASSERT_DNAFREQ = 751,       // Molecular biology Chargaff & melting temp (`dnafreq genealpha 30 30 12 8`)!
    INT_ASK_DNAFREQ = 752,          // GC % (`gcfrac`), Wallace melting temp `4*GC+2*AT` (`dnamelt`) & thermal stability (`dnastat`)!
    INT_ASSERT_SOLOWMOD = 753,      // Macroeconomics Solow capital accumulation (`solowmod econalpha 30 200 10 400`)!
    INT_ASK_SOLOWMOD = 754,         // Gross investment (`solinv`), net capital change (`solnet`) & growth trajectory (`solstat`)!
    // v49 STAGE 8g (PURE-GPU SPIKING MDP BELLMAN PLANNER & OPEN MULTI-TURN USER GOALS — 470 DOMAINS!): Kernel 43 on FREE L2 PW24[256..511, 768..1023] + 7 Deep Stage-8g STEM Domains!
    INT_ASSERT_MDPPLAN = 755,       // Kernel 43 Mode 0/3: live 1-shot GPU STDP MDP imprint & Bellman solve (`mdpplan mdplive mdpomega 5 10 100 40 90 50`)!
    INT_ASK_MDPPLAN = 756,          // Kernel 43 Mode 1/2: pure-GPU Bellman V*(s0) (`mdpval`), V*(s1) (`mdpmid`) & optimal policy (`mdppol`)!
    INT_ASSERT_BTREEDB = 757,       // Database B-Tree indexing (`btreedb idxalpha 4 3`)!
    INT_ASK_BTREEDB = 758,          // Max keys `m-1` (`btkeys`), max leaves `m^h` (`btleaf`) & depth verdict (`btstat`)!
    INT_ASSERT_BANDGAP = 759,       // Solid-state semiconductor bandgap (`bandgap semialpha 2000 300`)!
    INT_ASK_BANDGAP = 760,          // Cutoff wavelength nm (`bgwave`), thermal ratio (`bgrat`) & bandgap class (`bgstat`)!
    INT_ASSERT_FRUSTUM = 761,       // 3D graphics viewport & framebuffer (`frustum camalpha 1600 900 4`)!
    INT_ASK_FRUSTUM = 762,          // K-pixels (`frpix`), KB framebuffer (`frbuf`) & aspect ratio (`frstat`)!
    INT_ASSERT_QUAKEGEO = 763,      // Seismology fault potency & moment energy (`quakegeo faultalph 50 40 2`)!
    INT_ASK_QUAKEGEO = 764,         // Potency (`qkpot`), energy ratio `32^dm` (`qkerg`) & severity (`qkstat`)!
    INT_ASSERT_UNROLL = 765,        // Compiler loop unrolling (`unroll unralpha 64 4 6`)!
    INT_ASK_UNROLL = 766,           // Unrolled iters (`urlit`), unrolled ops (`urlsz`) & register pressure (`urlstat`)!
    INT_ASSERT_FIBEROPT = 767,      // Photonics optical fiber NA (`fiberopt fibalpha 150 140`)!
    INT_ASK_FIBEROPT = 768,         // `NA^2*10000` (`fbna2`), index contrast (`fbdel`) & mode class (`fbstat`)!
    INT_ASSERT_BUCKCONV = 769,      // Power electronics buck DC-DC converter (`buckconv regalpha 20 50 80`)!
    INT_ASK_BUCKCONV = 770,         // Output voltage (`bkvout`), power mW (`bkpwr`) & conduction mode (`bkstat`)!
    // v50 STAGE 8h (100-PILLAR MILESTONE! PURE-GPU SPIKING KALMAN FILTER & HYPOTHESIS TESTING — 480 DOMAINS!): Kernel 44 on FREE L2 PW24[512..767, 768..1023] + 7 Deep Stage-8h STEM Domains!
    INT_ASSERT_KFILT = 771,         // Kernel 44 Mode 0/3: live 1-shot GPU STDP Kalman filter imprint & fuse (`kfilt kflive kfomega 40 30 10 10 90 0`)!
    INT_ASK_KFILT = 772,            // Kernel 44 Mode 1/2: pure-GPU Kalman gain (`kfgain`), posterior state (`kfpost`) & trust regime (`kfmode`)!
    INT_ASSERT_BRPRED = 773,        // CPU branch prediction penalty (`brpred cpupralph 10 20 10 15`)!
    INT_ASK_BRPRED = 774,           // Mispredict overhead (`bpovh`), effective CPI (`bpcpi`) & stall verdict (`bpstat`)!
    INT_ASSERT_ALBEDO = 775,        // Planetary radiative albedo (`albedo plnalph 300 30 60`)!
    INT_ASK_ALBEDO = 776,           // Absorbed flux (`alabs`), surface flux (`alsurf`) & climate class (`alstat`)!
    INT_ASSERT_BLOOMFLT = 777,      // Bloom filter probabilistic set (`bloomflt fltalph 1000 100`)!
    INT_ASK_BLOOMFLT = 778,         // Bits per item (`blbpi`), optimal hashes (`blhsh`) & saturation verdict (`blstat`)!
    INT_ASSERT_CARDIAC = 779,       // Cardiovascular hemodynamics (`cardiac hrtalph 70 80 112`)!
    INT_ASK_CARDIAC = 780,          // Cardiac output (`cdout`), systemic resistance (`cdres`) & status (`cdstat`)!
    INT_ASSERT_PASSENT = 781,       // Information security password entropy (`passent pwdalph 12 6`)!
    INT_ASK_PASSENT = 782,          // Entropy bits (`pebits`), log10 brute-force (`pelog`) & strength (`pestat`)!
    INT_ASSERT_SABINE = 783,        // Architectural acoustics Sabine RT60 (`sabine hallalph 500 80`)!
    INT_ASK_SABINE = 784,           // Reverberation time (`sbrt`), absorption density (`sbden`) & class (`sbstat`)!
    INT_ASSERT_POLYGEOM = 785,      // Convex polygon geometry (`polygeom ngonalph 6`)!
    INT_ASK_POLYGEOM = 786,         // Interior angle sum (`pgsum`), diagonals (`pgdiag`) & symmetry (`pgstat`)!
    // v51 STAGE 8i (PURE-GPU SPIKING LQR RICCATI CONTROL & MULTI-TURN CONTINUITY — 490 DOMAINS!): Kernel 45 on FREE L2 PW24[0..255, 512..767] + 7 Deep Stage-8i STEM Domains!
    INT_ASSERT_LQRCTRL = 787,       // Kernel 45 Mode 0/3: live 1-shot GPU STDP LQR plant imprint & Riccati solve (`lqrctrl lqlive lqomega 80 100 30 10 60 0`)!
    INT_ASK_LQRCTRL = 788,          // Kernel 45 Mode 1/2: pure-GPU LQR gain (`lqgain`), pole (`lqpole`), control (`lqctrl`) & stability (`lqmode`)!
    INT_ASSERT_IMGCOMP = 789,       // Digital image compression (`imgcomp picx 800 600 4`)!
    INT_ASK_IMGCOMP = 790,          // Raw KB (`icraw`), compressed KB (`iczip`) & ratio class (`icstat`)!
    INT_ASSERT_BAROALT = 791,       // Atmospheric barometric lapse (`baroalt skyx 300 4 6`)!
    INT_ASK_BAROALT = 792,          // Altitude temp (`batemp`), temp drop (`badrop`) & freeze check (`bastat`)!
    INT_ASSERT_SHARDING = 793,      // Distributed consistent hashing (`sharding clusx 120 4 5`)!
    INT_ASK_SHARDING = 794,         // Keys per node (`shper`), virtual slots (`shvir`) & balance (`shstat`)!
    INT_ASSERT_BUCKLING = 795,      // Structural Euler column buckling (`buckling colx 10 80 4`)!
    INT_ASK_BUCKLING = 796,         // Critical load (`bupcr`), slenderness (`buslend`) & stiffness (`bustat`)!
    INT_ASSERT_F1SCORE = 797,       // Information retrieval F1 score (`f1score evalx 60 90`)!
    INT_ASK_F1SCORE = 798,          // Harmonic mean F1 (`f1harm`), arithmetic mean (`f1arith`) & tier (`f1stat`)!
    INT_ASSERT_MANNING = 799,       // Hydrology open-channel flow (`manning rivx 20 5 10`)!
    INT_ASK_MANNING = 800,          // Discharge flux (`mnflux`), hydraulic depth (`mndep`) & speed (`mnstat`)!
    INT_ASSERT_PAGETBL = 801,       // OS multi-level page table (`pagetbl mmux 4 1024`)!
    INT_ASK_PAGETBL = 802,          // Virtual span KB (`ptspan`), table KB (`ptbytes`) & scale (`ptstat`)!
    // v52 STAGE 8j (500-DOMAIN MILESTONE! PURE-GPU SPIKING FFT HARMONIC CORTEX & MULTI-TURN SELF-CORRECTION!): Kernel 46 on FREE L2 PW24[0..255, 256..511] + 7 Deep Stage-8j STEM Domains!
    INT_ASSERT_FFTSPEC = 803,       // Kernel 46 Mode 0/3: live 1-shot GPU STDP signal imprint & FFT (`fftspec fflive ffomega 90 10 90 10 90 10 90 10`)!
    INT_ASK_FFTSPEC = 804,          // Kernel 46 Mode 1/2: pure-GPU DC (`ffdc`), fundamental (`ffharm`), Nyquist (`ffnyq`) & band (`ffband`)!
    INT_ASSERT_SHANCAP = 805,       // Digital comms Shannon capacity (`shancap chnlx 20 16`)!
    INT_ASK_SHANCAP = 806,          // Spectral efficiency (`scspec`), channel rate (`scrate`) & bandwidth tier (`scstat`)!
    INT_ASSERT_FROUDE = 807,        // Open-channel wave Froude number (`froude flumx 30 20`)!
    INT_ASK_FROUDE = 808,           // Froude % (`frnum`), Froude^2 % (`frsq`) & criticality (`frregm`)!
    INT_ASSERT_BEZIER = 809,        // Computer graphics quadratic Bezier (`bezier curvx 10 50 30`)!
    INT_ASK_BEZIER = 810,           // Midpoint height (`bzmid`), chord avg (`bzavg`) & arch class (`bzstat`)!
    INT_ASSERT_RCFILT = 811,        // Analog electronics RC filter (`rcfilt rcnetx 10 25`)!
    INT_ASK_RCFILT = 812,           // Time constant tau (`rctau`), 5-tau settle (`rcset`) & cutoff speed (`rcstat`)!
    INT_ASSERT_LOGITSEP = 813,      // Machine learning softmax logit margin (`logitsep clfx 90 30 2`)!
    INT_ASK_LOGITSEP = 814,         // Raw margin (`lgmarg`), temp-scaled margin (`lgscal`) & sharpness (`lgstat`)!
    INT_ASSERT_HOHMANN = 815,       // Orbital mechanics Hohmann transfer (`hohmann orbx 100 300`)!
    INT_ASK_HOHMANN = 816,          // Semi-major axis (`hhsma`), eccentricity % (`hhecc`) & shape (`hhstat`)!
    INT_ASSERT_ECCSEC = 817,        // Elliptic-curve cryptography (`eccsec ecpx 256`)!
    INT_ASK_ECCSEC = 818,           // Security bits (`ecbits`), uncompressed bytes (`ecbytes`) & tier (`ecstat`)!
    // v53 STAGE 8k (510 DOMAINS! PURE-GPU SPIKING RK4 ODE INTEGRATOR & HYPOTHETICAL COUNTERFACTUAL DIALOGUE!): Kernel 47 on FREE L2 PW24[256..511, 0..255] + 7 Deep Stage-8k STEM Domains!
    INT_ASSERT_ODERK4 = 819,        // Kernel 47 Mode 0/3: live 1-shot GPU STDP ODE imprint & RK4 (`oderk4 odlive odomega 1 50 40 20 50 4`)!
    INT_ASK_ODERK4 = 820,           // Kernel 47 Mode 1/2: pure-GPU RK4 final state (`odval`), equilibrium (`odeq`) & stability (`odmode`)!
    INT_ASSERT_PVCELL = 821,        // Photovoltaic solar cell (`pvcell panlx 40 10 80 1`)!
    INT_ASK_PVCELL = 822,           // Peak power (`pvpow`), efficiency % (`pveff`) & tier (`pvtier`)!
    INT_ASSERT_MMQUEUE = 823,       // M/M/1 queueing server (`mmqueue srvrx 8 10`)!
    INT_ASK_MMQUEUE = 824,          // Utilization % (`mmutil`), mean queue length (`mmlen`) & tier (`mmtier`)!
    INT_ASSERT_GLIDRAT = 825,       // Glider aerodynamics (`glidrat glidx 100 40 20`)!
    INT_ASK_GLIDRAT = 826,          // Glide range (`glrng`), sink rate (`glsink`) & soaring tier (`gltier`)!
    INT_ASSERT_PKDOSE = 827,        // Biomedical pharmacokinetics (`pkdose medix 50 4 30`)!
    INT_ASK_PKDOSE = 828,           // Loading dose (`pkload`), hourly clearance (`pkclr`) & elimination tier (`pktier`)!
    INT_ASSERT_CONVNET = 829,       // ConvNet layer geometry (`convnet layrx 32 4 2 8 16`)!
    INT_ASK_CONVNET = 830,          // Output grid (`cnout`), weight count (`cnparm`) & depth tier (`cntier`)!
    INT_ASSERT_SEISMO = 831,        // Seismology P-S epicenter (`seismo quakx 10 22 6`)!
    INT_ASK_SEISMO = 832,           // S-P arrival lag (`smlag`), epicenter km (`smdist`) & proximity (`smtier`)!
    INT_ASSERT_BATPACK = 833,       // Battery energy storage (`batpack bpakx 48 20 80`)!
    INT_ASK_BATPACK = 834,          // Watt-hours (`bpwh`), runtime hours (`bphrs`) & endurance tier (`bptier`)!
    // v54 STAGE 8l (520 DOMAINS! PURE-GPU SPIKING SIMPLEX LP OPTIMIZER & MULTI-HOP PROOF DIALOGUE!): Kernel 48 on FREE L2 PW24[768..1023, 512..767] + 7 Deep Stage-8l STEM Domains!
    INT_ASSERT_SIMLP = 835,         // Kernel 48 Mode 0/3: live 1-shot GPU STDP LP imprint & vertex optimize (`simlp sxlive sxomega 5 4 2 1 20 1 2 16`)!
    INT_ASK_SIMLP = 836,            // Kernel 48 Mode 1/2: pure-GPU optimal Z* (`sxopt`), primal x* (`sxxval`) & dual/corner bind (`sxbind`)!
    INT_ASSERT_CAPBANK = 837,       // Electrostatics parallel-plate capacitor bank (`capbank cbnkx 4 50 2 10`)!
    INT_ASK_CAPBANK = 838,          // Capacitance (`cbcap`), stored energy (`cbeng`) & tier (`cbtier`)!
    INT_ASSERT_DARCYP = 839,        // Fluid mechanics Darcy-Weisbach pipe friction (`darcyp dpipx 4 50 2 10`)!
    INT_ASK_DARCYP = 840,           // Head loss (`dphead`), shear drag (`dpshr`) & tier (`dptier`)!
    INT_ASSERT_GUSTAF = 841,        // Parallel computing Gustafson scaled speedup (`gustaf gfclx 20 8`)!
    INT_ASK_GUSTAF = 842,           // Scaled speedup x100 (`gfspd`), serial drag (`gfdrag`) & tier (`gftier`)!
    INT_ASSERT_RANKIN = 843,        // Thermodynamics Rankine turbine enthalpy (`rankin rkturbx 3200 2200 6`)!
    INT_ASK_RANKIN = 844,           // Specific work (`rkwrk`), turbine power (`rkpow`) & tier (`rktier`)!
    INT_ASSERT_BDAYBND = 845,       // Cryptography hash collision birthday bound (`bdaybnd bdhshx 8 4`)!
    INT_ASK_BDAYBND = 846,          // Threshold 2^b (`bdthr`), collision risk % (`bdrisk`) & tier (`bdtier`)!
    INT_ASSERT_BEERLAM = 847,       // Biomedical Beer-Lambert optical absorbance (`beerlam blcuvx 5 6 4`)!
    INT_ASK_BEERLAM = 848,          // Absorbance x100 (`blabs`), optical density (`blod`) & tier (`bltier`)!
    INT_ASSERT_PUTCALL = 849,       // Financial derivatives put-call parity (`putcall pcoptx 100 110 15`)!
    INT_ASK_PUTCALL = 850,          // Synthetic put (`pcput`), moneyness gap (`pcgap`) & tier (`pctier`)!
    // v55 STAGE 8m (530 DOMAINS! PURE-GPU SPIKING PCA POWER-EIGEN CORTEX & AUTONOMOUS GENERALIZATION!): Kernel 49 on FREE L2 PW27[0..255, 256..511] + 7 Deep Stage-8m STEM Domains!
    INT_ASSERT_PCAEIG = 851,        // Kernel 49 Mode 0/3: live 1-shot GPU STDP covariance imprint & power-iter (`pcaeig pclive pcomega 28 6 12`)!
    INT_ASK_PCAEIG = 852,           // Kernel 49 Mode 1/2: pure-GPU dominant lam1 (`pclam`), variance % (`pcvar`) & axis/iso (`pcax`)!
    INT_ASSERT_BLKHOLE = 853,       // Astrophysics Schwarzschild black hole (`blkhole bhobjx 100 50`)!
    INT_ASK_BLKHOLE = 854,          // Horizon radius (`bhrad`), photon sphere (`bhphot`) & class (`bhtier`)!
    INT_ASSERT_HOOKESP = 855,       // Solid mechanics Hooke spring (`hookesp hksprx 40 5 2`)!
    INT_ASK_HOOKESP = 856,          // Restoring force (`hkfrc`), elastic energy (`hkeng`) & tier (`hktier`)!
    INT_ASSERT_TCPCWND = 857,       // Networking TCP cwnd throughput (`tcpcwnd tcfllx 100 20`)!
    INT_ASK_TCPCWND = 858,          // Throughput KB/s (`tcrate`), BDP (`tcbdp`) & speed tier (`tctier`)!
    INT_ASSERT_HENDBUF = 859,       // Chemistry Henderson-Hasselbalch buffer (`hendbuf hbsolx 70 20 10`)!
    INT_ASK_HENDBUF = 860,          // Ratio x10 (`hbrat`), shifted pH x10 (`hbph`) & regime (`hbtier`)!
    INT_ASSERT_PHONGSH = 861,       // Computer graphics Phong shading (`phongsh phmatx 40 60 200`)!
    INT_ASK_PHONGSH = 862,          // Diffuse (`phdiff`), specular (`phspec`) & surface finish (`phtier`)!
    INT_ASSERT_ISOSTAS = 863,       // Geophysics buoyant isostasy (`isostas ismntx 4 27 33`)!
    INT_ASK_ISOSTAS = 864,          // Mountain root depth (`isroot`), total crust (`istot`) & tier (`istier`)!
    INT_ASSERT_SHARPEF = 865,       // Quantitative finance Sharpe ratio (`sharpef shfndx 18 6 10`)!
    INT_ASK_SHARPEF = 866,          // Excess return (`shexr`), Sharpe x100 (`shrat`) & tier (`shtier`)!
    // v56 STAGE 8n (50-KERNEL MILESTONE & 540 DOMAINS! PURE-GPU SPIKING 1D HEAT/DIFFUSION PDE SOLVER & CROSS-CATEGORY DEDUCTION!): Kernel 50 on FREE L2 PW27[0..255, 512..767] + 7 Deep Stage-8n STEM Domains!
    INT_ASSERT_PDESLV = 867,        // Kernel 50 Mode 0/3: live 1-shot GPU STDP 1D PDE field imprint & evolve (`pdeslv pdlive pdomega 0 100 100 100 0 20 2`)!
    INT_ASK_PDESLV = 868,           // Kernel 50 Mode 1/2: pure-GPU center u_mid (`pdmid`), side u_side (`pdside`) & CFL stability (`pdcfl`)!
    INT_ASSERT_DEBROG = 869,        // Quantum mechanics de Broglie matter wave (`debrog dbwavx 60 5 10`)!
    INT_ASK_DEBROG = 870,           // Momentum (`dbmom`), wavelength (`dblam`) & regime (`dbtier`)!
    INT_ASSERT_TAILSLA = 871,       // Distributed systems tail latency SLA (`tailsla tssvcx 10 8 5`)!
    INT_ASK_TAILSLA = 872,          // P99 latency (`tsp99`), error budget (`tsbud`) & verdict (`tstier`)!
    INT_ASSERT_IONTHR = 873,        // Space propulsion ion thruster (`ionthr itengx 300 4 20`)!
    INT_ASK_IONTHR = 874,           // Exhaust velocity (`itvel`), thrust (`itthrst`) & class (`ittier`)!
    INT_ASSERT_HILLBND = 875,       // Biochemistry Hill cooperative binding (`hillbnd hlrecx 100 4 2`)!
    INT_ASK_HILLBND = 876,          // Ligand squared (`hlpow`), occupancy % (`hlfra`) & tier (`hltier`)!
    INT_ASSERT_BEAMST = 877,        // Structural mechanics beam bending stress (`beamst bmbarx 600 5 150`)!
    INT_ASK_BEAMST = 878,           // Bending stress (`bmsig`), section modulus (`bmsect`) & tier (`bmtier`)!
    INT_ASSERT_ARITHCD = 879,       // Information theory arithmetic coding (`arithcd acstrx 800 5 200`)!
    INT_ASK_ARITHCD = 880,          // Payload bits (`acpay`), total packet bits (`actot`) & tier (`actier`)!
    INT_ASSERT_TAYLRL = 881,        // Quantitative macroeconomics Taylor rule (`taylrl trbnkx 20 40 20 10`)!
    INT_ASK_TAYLRL = 882,           // Inflation gap (`trdev`), target rate x10 (`trrate`) & stance (`trtier`)!
    // v57 STAGE 8o (51 BLACKWELL KERNELS & 550 DOMAINS! PURE-GPU 256-CHAIN SPIKING MCMC SAMPLER & 3-ENTITY CLUSTERING DIALOGUE!): Kernel 51 on FREE L2 PW27[256..511, 0..255] + 7 Deep Stage-8o STEM Domains!
    INT_ASSERT_MCMCSMP = 883,       // Kernel 51 Mode 0/3: live 1-shot GPU STDP target imprint & 256-chain MCMC sample (`mcmcsmp mclive mcomega 80 10 5 5 32 53`)!
    INT_ASK_MCMCSMP = 884,          // Kernel 51 Mode 1/2: pure-GPU posterior mode % (`mcprob`), accept rate % (`mcacc`) & peaked/flat regime (`mcmode`)!
    INT_ASSERT_FERMIDR = 885,       // Solid-state Fermi-Dirac band gap (`fermidr fdmatx 40 20 10`)!
    INT_ASK_FERMIDR = 886,          // Energy gap (`fdgap`), thermal ratio (`fdocc`) & conductor/insulator tier (`fdtier`)!
    INT_ASSERT_BERNFLW = 887,       // Fluid dynamics Bernoulli pipe flow (`bernflw bfpipx 2 10 50`)!
    INT_ASK_BERNFLW = 888,          // Dynamic pressure (`bfdyn`), total pressure (`bftot`) & flow regime (`bftier`)!
    INT_ASSERT_SHAMIRS = 889,       // Cryptography Shamir secret sharing (`shamirs shkeyx 7 4`)!
    INT_ASK_SHAMIRS = 890,          // Quorum (`shquor`), fault tolerance (`shfault`) & resilience tier (`shmtier`)!
    INT_ASSERT_MEMVOLT = 891,       // Neuroscience Nernst membrane drive (`memvolt mvcelx 70 30 4`)!
    INT_ASK_MEMVOLT = 892,          // Driving force (`mvdrv`), synaptic current (`mvcur`) & spike tier (`mvtier`)!
    INT_ASSERT_INVKINE = 893,       // Robotics 2-link IK workspace (`invkine ikarmx 12 8`)!
    INT_ASK_INVKINE = 894,          // Max reach (`ikmax`), deadzone (`ikdead`) & dexterity tier (`iktier`)!
    INT_ASSERT_LOTKAVOL = 895,      // Ecology Lotka-Volterra dynamics (`lotkavol lvecox 40 2 30 3`)!
    INT_ASK_LOTKAVOL = 896,         // Prey equilibrium (`lvprey`), predator equilibrium (`lvpred`) & tier (`lvtier`)!
    INT_ASSERT_ROOFLN = 897,        // Computer architecture Roofline model (`roofln rfgpu 160 20 40 600`)!
    INT_ASK_ROOFLN = 898,           // Arithmetic intensity (`rfint`), attainable perf (`rfperf`) & bound (`rftier`)!
    // v58 STAGE 8p (52 BLACKWELL KERNELS, 560 DOMAINS & 100/100 CENTURY VALLEY! PURE-GPU 4,096-ROLLOUT SPIKING MCTS PLANNER & 100% NATURAL ZERO-KEYWORD DIALOGUE!): Kernel 52 on FREE L2 PW27[256..511, 768..1023] + 7 Deep Stage-8p STEM Domains!
    INT_ASSERT_MCTSPLN = 899,       // Kernel 52 Mode 0/3: live 1-shot GPU STDP decision-tree imprint & 4,096-rollout MCTS (`mctspln mtlive mtomega 110 50 45 40 90 50 20 53`)!
    INT_ASK_MCTSPLN = 900,          // Kernel 52 Mode 1/2: pure-GPU MCTS optimal value (`mtval`), visit-share % (`mtvis`) & bold/safe strategy (`mtpol`)!
    INT_ASSERT_GROVERQ = 901,       // Quantum computing Grover search (`groverq grqregx 100 1 4`)!
    INT_ASK_GROVERQ = 902,          // Optimal iterations (`griter`), oracle gate cost (`grcost`) & speed tier (`grtier`)!
    INT_ASSERT_PCMRATE = 903,       // Digital audio PCM bitrate (`pcmrate pcmstx 48 16 2`)!
    INT_ASK_PCMRATE = 904,          // Bitrate kbps (`pcbps`), Nyquist ceiling kHz (`pcnyq`) & fidelity tier (`pmtier`)!
    INT_ASSERT_STEBOLTZ = 905,      // Thermal radiation Stefan-Boltzmann (`steboltz sbsrcx 80 10 100`)!
    INT_ASK_STEBOLTZ = 906,         // Radiative flux (`sbpow`), Wien peak (`sbpeak`) & thermal tier (`sbtier`)!
    INT_ASSERT_PBFTSYS = 907,       // Distributed consensus PBFT quorum (`pbftsys pbclux 10 3`)!
    INT_ASK_PBFTSYS = 908,          // Commit quorum 2f+1 (`pbquor`), minimum nodes 3f+1 (`pbmin`) & safety tier (`pbtier`)!
    INT_ASSERT_YOUNGMD = 909,       // Materials science Young's modulus (`youngmd ymrodx 1200 4 6000`)!
    INT_ASK_YOUNGMD = 910,          // Stress MPa (`ymstss`), scaled strain (`ymdefm`) & stiffness tier (`ymtier`)!
    INT_ASSERT_VISVIVA = 911,       // Orbital mechanics Vis-Viva velocity^2 (`visviva vvcraftx 600 10 20`)!
    INT_ASK_VISVIVA = 912,          // Orbital speed^2 (`vvvsq`), escape speed^2 (`vvesc`) & apsis regime (`vvtier`)!
    INT_ASSERT_DAMPCTL = 913,       // Control systems 2nd-order damping (`dampctl dcsysx 20 80 5`)!
    INT_ASK_DAMPCTL = 914           // Damping rate (`dcdamp`), settling time (`dcsett`) & damping regime (`dctier`)!
};

enum SlotType : Byte {
    SLOT_FIXED = 0,
    SLOT_SUBJ = 1,
    SLOT_REL = 2,
    SLOT_OBJ = 3,
    SLOT_LOC = 4,
    SLOT_CAUSE = 5,
    SLOT_PERSP = 6,
    SLOT_PERSP2 = 7,
    SLOT_PROP = 8,
    SLOT_ACTION = 9,
    SLOT_QTY = 10,
    SLOT_OBJ2 = 11,
    SLOT_OPT = 12,
    SLOT_QTY2 = 13
};

#if !defined(__CUDA__)
int command(int argc, char **argv);
#endif

} // namespace axion::mind
