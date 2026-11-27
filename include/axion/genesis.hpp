#pragma once
#include "model.hpp"

namespace axion::genesis {

using u8 = unsigned char;
using i8 = signed char;
using u16 = unsigned short;
using i16 = short;
using u32 = unsigned int;
using i32 = int;
using u64 = unsigned long long;

// ---------------------------------------------------------------------------
// Axion Genesis v6: Unified Cellular-Automata Body+Brain DNA, Crawling Axon
// Growth Cones, Variable Fan-In Blocked Blackwell SNN & 256x256 GPU World
// ---------------------------------------------------------------------------
// ZERO hardcoded tasks, ZERO domain calculators, ZERO fixed body blueprints,
// ZERO fixed cortical stencils, ZERO pinned sensory/motor neuron indices,
// ZERO hardcoded learning rules.
// Every organism begins as ONE zygote stem cell in a unified 3D cellular
// tissue space (16x16 x L layers, growing unboundedly via asymmetric mitosis)
// governed by an expandable discrete CA-GRN rulebook (CARule[0..num_rules-1])
// that grows both body organs and spiking brain nuclei together, wires them
// via crawling axon growth cones into variable-fan-in 4 B/synapse Blackwell
// v4.u32 slabs, and continues post-natal juvenile->adult maturation driven by
// surplus ATP and spiking-activity neurotrophin feedback!

// Default world dimensions (runtime-scalable to 256x256, 512x512, or 1024x1024 via WorldBuffers & --world!)
inline constexpr u32 WORLD_W = 256;
inline constexpr u32 WORLD_H = 256;
inline constexpr u32 WORLD_MASK = 255;
inline constexpr u32 WORLD_CELLS = WORLD_W * WORLD_H;

// Unified anatomical tissue sheet: 16x16 = 256 cells per layer, growing across 1..32 layers (up to 8,192 neurons/org)!
inline constexpr u32 TISSUE_W = 16;
inline constexpr u32 TISSUE_H = 16;
inline constexpr u32 TISSUE_CELLS = TISSUE_W * TISSUE_H; // 256 unified somatic+neural cells / layer
inline constexpr u32 MAX_LAYERS_PER_CREATURE = 32;       // Up to 8,192 unified cells / neurons per organism!
inline constexpr u32 MAX_STRIPES_PER_LAYER = 12;         // Up to 12 x 16 = 192 variable fan-in synapses / neuron (1.57M syn/org)!
inline constexpr u32 SYN_PER_STRIPE = 16;
inline constexpr u32 QUADS_PER_STRIPE = SYN_PER_STRIPE / 4; // 4 x 128-bit v4.u32 coalesced loads (4 NMDA branch subunits/stripe!)
inline constexpr u32 LAYER_SYN_QUADS = TISSUE_CELLS * QUADS_PER_STRIPE; // 1,024 SynQuads per SynSlab (16 KiB)
inline constexpr u32 SUBSTEPS_PER_TICK = 4;

inline constexpr u32 MAX_DNA_RULES = 96; // Expandable discrete CA rulebook via Ohno gene duplication & divergence!
inline constexpr u32 MAX_POP = 2048;
inline constexpr u32 INVALID_SLAB = 0xffffffffu; // 32-bit slab IDs: unlocks 131k+ layer slabs & 262k+ SynSlabs!
inline constexpr u32 MAX_OBJECTS = 1024;
inline constexpr u16 INVALID_OBJ = 0xffffu;

// Persistent movable physical objects/artifacts & reactive prey/predators in the GPU world
enum ObjectKind : u16 {
    OBJ_NONE = 0,
    OBJ_POD = 1,     // Hard-shelled nutrient pod (32 ATP feast; cracks at vents or stone/ore/polymer anvils)
    OBJ_FUEL = 2,    // Cracked/cooked open multi-portion feast (immediately edible or cacheable)
    OBJ_STONE = 3,   // Heavy mineral hammer/weapon stone (chipped from coastal ore ridges, carryable tool!)
    OBJ_PREY = 4,    // Fast, shy bioluminescent glider (flees noisy/clumsy swimmers; rewards sighted/eared stalking)
    OBJ_MEGAPOD = 5, // Giant calcified cooperative/tool-required mega-pod (64 ATP communal feast: needs STONE hammer or 2+ allies!)
    OBJ_APEX = 6     // Autonomous roaming apex predator (hunts lone swimmers; blocked by POLYMER walls; slain by STONE weapon or 3+ mobbing allies for 64 ATP!)
};

struct alignas(16) WorldObject {
    float x, y;
    float vx, vy;
    float energy;
    float integrity;
    u16 kind;
    u16 carrier_id; // 0xffff = free on ground, else creature slot carrying it
    u16 age;
    u16 flags;      // bit 0: handled/carried by a creature (proves genuine organism tool/cooking agency!)
};
static_assert(sizeof(WorldObject) == 32);

// 128-bit coalesced synaptic stripe quad: 4 packed 32-bit plastic synapses loaded in 1 Blackwell v4.u32 instruction!
struct alignas(16) SynQuad {
    u32 x, y, z, w;
};
static_assert(sizeof(SynQuad) == 16);

// Unified Somatic + Neural Cellular Tissue Kinds (ALL coexist in the same 16x16 x L organism!)
enum TissueKind : u8 {
    TK_EMPTY = 0,     // Empty space outside the organism
    TK_STEM = 1,      // Undifferentiated dividing progenitor / radial glia stem cell
    TK_STRUCT = 2,    // Structural / calcified shell armor
    TK_MUSCLE = 3,    // Contractile effector: fires physical thrust & lever-arm torque from arriving spikes!
    TK_EYE = 4,       // Photoreceptor: casts directional optical rays & fires spikes down grown optic axons!
    TK_CHEM = 5,      // Chemoreceptor: senses local chem/temp gradients & fires spikes down olfactory axons!
    TK_EAR = 6,       // Mechano-acoustic hair cell: senses 3-band waves at its physical L/R body offset!
    TK_GRASP = 7,     // Manipulator / mandible / builder: spikes drive digging, pod pickup/cracking, building, eating!
    TK_GLAND = 8,     // Secretory / catalytic / vocalizer / photophore: spikes drive A+B->C, 2-band vocal, kin share!
    TK_NEU_EXC = 9,   // Excitatory spiking neuron (pyramidal/granule with evolved tau + active apical BAC plateau!)
    TK_NEU_PV = 10,   // Fast-spiking somatic inhibitory interneuron (strict Dale's Law, gamma WTA / striatum-like)
    TK_NEU_SST = 11,  // Dendritic apical inhibitory interneuron (strict Dale's Law, predictive gating)
    TK_NEU_MOD = 12,  // Neuromodulatory spiking hub (VTA/BF intrinsic predictive DA & ACh projection!)
    TK_TYPES = 13
};

AX_FN bool tk_is_somatic(u8 tk) { return tk >= TK_STRUCT && tk <= TK_GLAND; }
AX_FN bool tk_is_sensor(u8 tk) { return tk == TK_EYE || tk == TK_CHEM || tk == TK_EAR; }
AX_FN bool tk_is_effector(u8 tk) { return tk == TK_MUSCLE || tk == TK_GRASP || tk == TK_GLAND; }
AX_FN bool tk_is_neuron(u8 tk) { return tk >= TK_NEU_EXC && tk <= TK_NEU_MOD; }
AX_FN bool tk_is_excitable(u8 tk) { return tk >= TK_MUSCLE && tk <= TK_NEU_MOD; }
AX_FN bool tk_is_inhibitory(u8 tk) { return tk == TK_NEU_PV || tk == TK_NEU_SST; }

// Discrete Developmental CA Rule Opcodes (Fundamental Biological Building Blocks!)
enum CAOpcode : u8 {
    OP_MITOSIS = 0,     // Asymmetric cell division into (Daughter A state, Daughter B state) along dir_mask (+x/-x/+y/-y/+z/-z)
    OP_SIGNAL = 1,      // Local morphogen emission/absorption & chemoaffinity surface tag writing
    OP_DIFF = 2,        // Specialize into ANY somatic organ or spiking neuron class + cell-intrinsic NMDA tau/dendrite gene
    OP_GROWTH_CONE = 3, // Sprout discrete crawling axon growth cone steered by chemical gradients & boundary tags!
    OP_APOPTOSIS = 4    // Programmed cell death / anatomical sculpting
};

// Geological terrain material classes
enum TerrainMat : u8 {
    MAT_LOAM = 0,
    MAT_ORE = 1,
    MAT_VENT = 2,
    MAT_POLYMER = 3,
    MAT_CATALYST = 4
};

// ---------------------------------------------------------------------------
// 16-Byte Discrete Developmental CA-GRN Rule (Condition -> Local Bio Action)
// ---------------------------------------------------------------------------
struct alignas(16) CARule {
    u8 match_state; // Required cell gene_state (0..15, or >=16 wildcard matching state & 15 / any!)
    u8 match_tk;    // 0 = any living cell, 1 = stem only, 2 = sensor/neuron, 3..15 = specific TissueKind + 2
    i8 chem_min[2]; // Wolpert morphogen lower concentration window on channels (c0, c1)
    i8 chem_max[2]; // Wolpert morphogen upper concentration window on channels (c0, c1)
    u8 opcode;      // CAOpcode (0..4)
    u8 dir_mask;    // Bits: 0:+x, 1:-x, 2:+y, 3:-y, 4:+z (bud deeper neural layer!), 5:-z, 6:bilateral mirror, 7:grad-steer
    u8 arg_a;       // OP_MITOSIS: daughter A state | OP_SIGNAL: chem ch | OP_DIFF: TissueKind | OP_CONE: target tk class
    u8 arg_b;       // OP_MITOSIS: daughter B state | OP_DIFF: tau+dendrite | OP_CONE: steer chem+sign+turn
    i8 param0;      // Emission delta / initial synaptic weight / division budget / cone step limit
    i8 param1;      // Morphogen polarity shift / apical comp pref / cone branch & layer-cross cue
    u8 chem_sel;    // low nibble: c0 (0..3), high nibble: c1 (0..3)
    u8 stage_pri;   // low 4b: priority, bits 4..5: stage (0=any, 1=embryo, 2=juvenile maturation), bits 6..7: rule_id
    u16 tag_bits;   // Surface chemoaffinity tag written by cell OR matched by crawling axon growth cones!
};
static_assert(sizeof(CARule) == 16);

// ---------------------------------------------------------------------------
// Expandable Unified Body+Brain CA-GRN DNA (1568 B max, Starts at ~32 Rules!)
// ---------------------------------------------------------------------------
struct alignas(16) GenesisDNA {
    CARule rules[MAX_DNA_RULES]; // 1536 B: expandable discrete rulebook (grows/shrinks via Ohno duplication!)
    u32 plast_lut[4];            //   16 B: 4 evolvable 16x2-bit 3-factor STDP dendritic plasticity LUTs
    // [0]=soma_leak, [1]=apical_leak, [2]=soma_thresh, [3]=apical_thresh,
    // [4]=bac_gain, [5]=adapt_inc, [6]=target_rate, [7]=symmetry_gene,
    // [8]=gland_mode, [9]=sens_gain, [10]=repro_bias, [11]=growth_atp_bias
    i8 bio_params[12];           //   12 B
    u8 num_rules;                //    1 B: active CA rules (6..MAX_DNA_RULES; duplicates & diverges freely!)
    u8 max_layers_gene;          //    1 B: evolved tissue depth ceiling (1..MAX_LAYERS_PER_CREATURE)
    u8 birth_steps;              //    1 B: embryonic CA steps at hatching (6..11)
    u8 adult_steps;              //    1 B: post-natal juvenile->adult maturation growth steps (2..5)
};
static_assert(sizeof(GenesisDNA) == 1568);

// ---------------------------------------------------------------------------
// Grown Unified Multicellular Body+Brain Phenotype (Output of Running CARules)
// ---------------------------------------------------------------------------
struct alignas(16) BodyPhenotype {
    u8 surface[TISSUE_CELLS]; // 256 B: Layer-0 16x16 anatomical body+head tissue map (for fast sensing & visualization!)
    u16 counts[TK_TYPES];     //  26 B: exact cell count of every somatic & neural TissueKind across ALL layers!
    u16 total_cells;          //   2 B: total living cells across all layers
    u16 active_neurons;       //   2 B: total living spiking neurons across all layers
    u16 max_fanin;            //   2 B: peak variable fan-in of any single hub neuron in this organism!
    u8 grown_layers;          //   1 B: actual unified tissue layers grown (1..MAX_LAYERS_PER_CREATURE)
    u8 symmetry_pct;          //   1 B: bilateral anatomical symmetry % (0..100)
    u32 total_synapses;       //   4 B: total real synapses landed by crawling axon growth cones (supports 1M+!)

    float mass;
    float inv_mass;
    float inertia;
    float basal_cost;
    float com_x;              // Anatomical center of mass on 16x16 grid (for real lever-arm torque & optics!)
    float com_y;
    float thrust_cap;
    float torque_cap;
    float grasp_power;
    float gland_power;
    float armor_factor;
    u32 body_hash;
    u32 brain_hash;
    u32 cones_landed;         // Number of crawling axon growth-cone synapses formed!
    u32 _pad1[3];
};
static_assert(sizeof(BodyPhenotype) % 16 == 0);

// ---------------------------------------------------------------------------
// Embodied Creature Runtime State in the Living 256x256 GPU World
// ---------------------------------------------------------------------------
struct alignas(16) CreatureState {
    float x, y;
    float vx, vy;
    float theta, omega;
    float atp;
    float hp;
    float body_temp;
    float carry_soil;
    float carry_a;
    float carry_b;
    float carry_c;
    float prev_atp;
    float da_baseline; // Schultz TD Reward Prediction Error baseline
    float prev_sens;
    float mod_da;
    float mod_ach;
    float mod_ne;

    // Emergent effector commands integrated directly from spikes landing on MUSCLE / GRASP / GLAND cells!
    float mot_thrust;
    float mot_turn;
    float mot_dig;
    float mot_build;
    float mot_eat;
    float mot_catalyze;
    float mot_vocal0;
    float mot_vocal1;
    float mot_vocal2;
    float mot_secrete;

    u32 alive;          // 0=dead/empty, 1=living, 2=gestating
    u32 age;
    u32 generation;
    u32 lineage_id;
    u32 offspring;
    u32 tiles_dug;
    u32 tiles_built;
    u32 chem_synth;
    u32 vocal_pulses;
    u32 food_events;
    u32 bites_landed;
    u32 energy_shared;
    u32 plast_edits;
    u32 syn_rewires;
    u64 spikes_total;
    float energy_gained;
    float home_dx;      // Inner-ear vestibular path-integration displacement from last shelter/birth!
    float home_dy;
    float int_da;       // Cross-layer VTA/BF intrinsic predictive dopamine broadcast from TK_NEU_MOD spikes!
    float int_ach;      // Cross-layer intrinsic ACh attention broadcast from TK_NEU_MOD spikes!
    float last_heard_theta; // Passive telemetry ONLY: heading when conspecific call arrived (never read by brain!)
    u32 last_heard_tick;    // Tick when conspecific call arrived (for real reciprocal vocal-duet coordination!)
    u32 resp_heard_tick;    // Passive telemetry ONLY: separate timer for listener-response meter (never clears last_heard_tick!)
    u32 last_vocal_tick;    // Tick of this organism's last vocal emission (for reciprocal vocal-duet coordination!)
    u8 last_vocal_band;     // 0=v0 bass rally, 1=v1 sonar/alarm chirp, 2=v2 harmonic social/concept
    u8 last_heard_band;     // Dominant vocal band (0|1|2) most recently heard by TK_EAR (for vocal mimicry & duets!)
    u8 prev_vocal_band;     // Previous vocal band (0|1|2, 3=none) for 2-syllable compositional resonance!
    u8 prev_vocal_age;      // Ticks elapsed between prev_vocal_band and last_vocal_band (0..255)

    // Unified 3D Cellular Tissue Layer Slabs & Post-Natal Maturation Stage!
    u16 num_layers;
    u16 carried_obj; // INVALID_OBJ or index into WorldObjects
    u8 growth_stage; // 0=hatchling, 1=juvenile, 2=adult (grows live during life!)
    u8 _pad_cs[3];
    u32 layer_slabs[MAX_LAYERS_PER_CREATURE]; // GPU TissueSlab indices (up to 32 layers = 8,192 cells/neurons!)
};

// ---------------------------------------------------------------------------
// Unified GPU Tissue & Variable-Fan-In Synapse Slab Pool (Blackwell GDDR7!)
// ---------------------------------------------------------------------------
struct TissuePool {
    u32 max_layers;       // Total allocated 256-cell unified tissue slabs
    u32 max_syn_slabs;    // Total allocated 4,096-synapse coalesced v4.u32 SynSlabs
    u32 *layer_bitmap;    // Lock-free atomic allocation bitmap for tissue layers
    u32 *syn_bitmap;      // Lock-free atomic allocation bitmap for variable-fan-in SynSlabs

    // Per-Layer Unified Cellular Tissue & Spiking State [max_layers * 256]
    u64 *tissue;          // Persistent discrete CA state: kind, gene_state, budget/tau, surface_tag, 4x i8 morphogens
    u64 *neurons;         // Packed 8-byte multi-compartment spiking state (vs, va, ua, ring, ema, meta)
    u8 *spike_ring;       // 256 B/layer read-only previous-tick 8-substep spike history (100% race-free cross-layer reads!)
    u8 *spike_ring_next;  // 256 B/layer write-only current-tick spike history (swapped with spike_ring after step_brains!)
    u8 *cell_stripes;     // [max_layers * 256] low nibble: stripe count (0..12), high nibble: exc_q (R_in ~ 1/K scaling!)

    // Variable Fan-In Blocked Coalesced v4.u32 Synapse Table!
    // Each layer slab owns up to MAX_STRIPES_PER_LAYER (12) SynSlabs -> up to 192 plastic synapses per hub neuron!
    u32 *layer_syn_slabs; // [max_layers * MAX_STRIPES_PER_LAYER] INVALID_SLAB if unallocated
    SynQuad *syn_slabs;   // [max_syn_slabs * 1024] 16-byte coalesced v4.u32 stripes (EXACT 4 B/synapse!)
    u8 *stripe_src_layer; // [max_syn_slabs * 256] bits 0..4: src_l (0..31), bit 5: F_ELIG, bit 6: F_DEP!

    // Compact GPU Active Layer Worklist (Persistent Blackwell Grid-Stride Dispatch — ZERO Idle CTAs!)
    u32 *work_list;       // [max_layers] packed (cid << 16) | layer_idx
    u32 *work_count;      // [1] exact live tissue layers to step
};

// ---------------------------------------------------------------------------
// True Physical / Biological Interventions (ZERO fake score guards!)
// ---------------------------------------------------------------------------
struct AblationFlags {
    u32 freeze_plasticity = 0; // Disables STDP, homeostatic scaling & structural rewiring
    u32 silence_apical = 0;    // Sets apical dendritic current i_apical = 0 in membrane dynamics
    u32 zero_delays = 0;       // Flattens heterogeneous axon-length delays to uniform mean latency (3 substeps — honest polychrony test!)
    u32 silence_neuromod = 0;  // Clamps DA = ACh = NE = 0 & silences intrinsic NT_MOD spikes
    u32 blind_eyes = 0;        // Silences photoreceptor (TK_EYE) optical transduction
    u32 deaf_ears = 0;         // Silences mechano-acoustic hair-cell (TK_EAR) transduction
    u32 drift_mode = 0;        // Unselected neutral genetic drift (random parent replacement)
    u32 shift_chemistry = 0;   // Held-out environment perturbation (inverted seasonal reefs + shifted vents/storms)
    u32 challenge_tier = 1;    // 0=baseline, 1=trophic+day/night+storms, 2=harsh shifting crucible
};

// ---------------------------------------------------------------------------
// 256x256 World Device Buffer Views (SoA for Coalesced Blackwell L2/GDDR7)
// ---------------------------------------------------------------------------
struct WorldBuffers {
    float *terrain;
    u8 *material;
    float *water;
    float *water_next;
    float *vx;
    float *vy;
    float *temp;
    float *temp_next;
    float *chem_a;
    float *chem_a_next;
    float *chem_b;
    float *chem_b_next;
    float *chem_c;
    float *chem_c_next;
    float *chem_d;
    float *chem_d_next;
    float *acoust_p;       // Physical wave (storms, wakes, anvil strikes, bites)
    float *acoust_prev;
    float *acoust_next;
    float *acoust_v0;      // Biological vocalization Band 0 (low formant)
    float *acoust_v0_prev;
    float *acoust_v0_next;
    float *acoust_v1;      // Biological vocalization Band 1 (high formant sonar/alarm)
    float *acoust_v1_prev;
    float *acoust_v1_next;
    float *acoust_v2;      // Biological vocalization Band 2 (mid harmonic social/compositional channel!)
    float *acoust_v2_prev;
    float *acoust_v2_next;
    float *light;
    u32 *cell_occ;
    u32 *tile_occ_id;
    u32 *tile_obj_id;
    WorldObject *objects;
    u32 max_objects;
    u32 w;          // Runtime world width (256, 512, or 1024 — power of two!)
    u32 h;          // Runtime world height (256, 512, or 1024 — power of two!)
    u32 mask_x;     // w - 1
    u32 mask_y;     // h - 1
    u32 shift_y;    // log2(w): 8 for 256, 9 for 512, 10 for 1024
    u32 num_reefs;  // Scaled nomadic reefs (16 @ 256, 48 @ 512+)
    u32 num_vents;  // Scaled geothermal vents (10 @ 256, 28 @ 512+)
    u32 num_storms; // Scaled barometric storms (3 @ 256, 8 @ 512+)
};

struct SimCounters {
    u32 births;
    u32 deaths;
    u32 total_dug;
    u32 total_built;
    u32 total_synth;
    u32 total_vocal;
    u32 total_bites;
    u32 total_shared;
    u32 total_pods_cracked;   // Strictly pods handled & cracked/cooked by creatures!
    u32 total_stones_made;    // Carryable mineral hammer/anvil stones chipped from coastal ore
    u32 total_tool_cracks;    // Multi-step tool use: pods/megapods cracked using a carried or placed OBJ_STONE!
    u32 total_coop_cracks;    // Cooperative social cracking: 2+ creatures converging on an OBJ_MEGAPOD / OBJ_POD!
    u32 total_shelter_nights; // Nocturnal shelter navigation: resting inside built MAT_POLYMER walls in dark cold night!
    u32 total_prey_caught;    // Shy evasive OBJ_PREY captured
    u32 total_apex_kills;     // Autonomous OBJ_APEX predators slain by STONE weapons or 3+ mobbing allies!
    u32 total_apex_deaths;    // Organisms killed by OBJ_APEX predators (verifies genuine predator selection!)
    u32 total_vocal_resp;     // Passive telemetry: listeners turning >0.22 rad within 12 ticks of hearing v0/v1/v2!
    u32 total_vocal_replies;  // Passive telemetry: listeners answering with their own vocalization within 12 ticks!
    u32 total_vocal_mimics;   // Cultural vocal imitation: listener matching heard formant band (v0/v1/v2) within 12 ticks!
    u32 total_imprint_edits;  // Juvenile critical-period (age < 120) synaptic edits!
    u32 total_echo_hits;      // Real physical v1 echolocation reflections off objects/walls!
    u32 total_duet_coops;     // Reciprocal vocal-duet coordinated mega-pod / apex pack actions!
    u32 total_tempered_tools; // Stage-2 heat/catalyst-tempered composite stone tools forged!
    u32 total_cached_feasts;  // Insulated food preserved & consumed beside built MAT_POLYMER shelters!
    u32 total_rewires;
    u32 total_maturations;    // Post-natal juvenile->adult live CA growth events!
    u32 active_count;
    u32 vocal_ctx[3][4];      // Passive 3-Band Shannon MI table: [band 0|1|2][0:food/pod/prey, 1:apex/combat, 2:storm/toxin, 3:open]
    u64 total_spikes;
    u64 total_edits;
    u64 total_energy_x100;  // Strictly honest primary + catalytic + pod + prey + apex trophy ATP harvested!
};

// Unified 16x16 Body+Brain Developmental Snapshots (Zygote -> Embryo -> Juvenile -> Adult!)
struct DevSnapshots {
    u8 step0[TISSUE_CELLS]; // Single fertilized zygote stem cell
    u8 step3[TISSUE_CELLS]; // Early gastrulation / morphogen symmetry breaking
    u8 step6[TISSUE_CELLS]; // Organogenesis + neurogenesis
    u8 step9[TISSUE_CELLS]; // Mature unified body+nervous system (Layer 0 surface!)
};

// Packed Cellular Tissue State Accessors (8 Bytes / Unified Cell!)
AX_FN u64 pack_tissue(u8 kind, u8 gstate, u8 budget_tau, u16 tag, i8 m0, i8 m1, i8 m2, i8 m3) {
    u32 lo = u32(kind & 0xfu) | (u32(gstate & 0xfu) << 4) | (u32(budget_tau) << 8) | (u32(tag) << 16);
    u32 hi = (u32(u8(m0))) | (u32(u8(m1)) << 8) | (u32(u8(m2)) << 16) | (u32(u8(m3)) << 24);
    return u64(lo) | (u64(hi) << 32);
}
AX_FN u8 tis_kind(u64 t) { return u8(t & 0xfu); }
AX_FN u8 tis_state(u64 t) { return u8((t >> 4) & 0xfu); }
AX_FN u8 tis_budget(u64 t) { return u8((t >> 8) & 0xffu); }
AX_FN u16 tis_tag(u64 t) { return u16((t >> 16) & 0xffffu); }
AX_FN i32 tis_m(u64 t, u32 c) { return i32(i8((t >> (32u + c * 8u)) & 0xffu)); }

// Packed Synapse Bitfield Accessors (4 Bytes / Multi-Compartment Delayed Plastic Synapse!)
AX_FN u32 pack_synapse(u32 pre, i32 weight, i32 elig, u32 delay, u32 comp, u32 rule_id, u32 stp) {
    i32 w = bound(weight, -127, 127);
    u32 wu = u32(i8(w)) & 0xffu;
    u32 eu = u32(bound(elig, -63, 63) + 64) & 0x7fu;
    return (pre & 0xffu) | (wu << 8) | (eu << 16) | ((delay & 7u) << 23) | ((comp & 1u) << 26) |
           ((rule_id & 3u) << 27) | ((stp & 7u) << 29);
}
AX_FN u32 syn_pre(u32 s) { return s & 0xffu; }
AX_FN i32 syn_weight(u32 s) { return i32(i8((s >> 8) & 0xffu)); }
AX_FN i32 syn_elig(u32 s) { return i32((s >> 16) & 0x7fu) - 64; }
AX_FN u32 syn_delay(u32 s) { return (s >> 23) & 7u; }
AX_FN u32 syn_comp(u32 s) { return (s >> 26) & 1u; }
AX_FN u32 syn_rule(u32 s) { return (s >> 27) & 3u; }
AX_FN u32 syn_stp(u32 s) { return (s >> 29) & 7u; }

// Packed Multi-Compartment Neuron State (8 Bytes / Excitable Cell — ZERO Overhead Multi-Nucleus Specialization!)
// ua byte: bits 0..6 = spike-frequency adaptation (0..127), bit 7 = fast_plast (Hippocampal 2x one-shot STDP nucleus!)
// meta byte: bits 0..3 = TissueKind (0..12), bits 4..5 = refrac (0..3), bit 6 = apical plateau, bit 7 = slow NMDA tau (Prefrontal WM!)
AX_FN u64 pack_neuron(i32 vs, i32 va, u32 ua, u32 ring, u32 ema, u32 tk, u32 refrac, u32 plat,
                      u32 slow_tau = 0u, u32 fast_plast = 0u) {
    u64 lo = u64(u16(i16(bound(vs, -32000, 32000)))) |
             (u64(u16(i16(bound(va, -32000, 32000)))) << 16);
    u32 ua_pk = (ua > 127u ? 127u : ua) | ((fast_plast & 1u) << 7);
    u32 meta = (tk & 0xfu) | ((refrac & 3u) << 4) | ((plat & 1u) << 6) | ((slow_tau & 1u) << 7);
    u64 hi = u64(ua_pk & 0xffu) | (u64(ring & 0xffu) << 8) | (u64(ema & 0xffu) << 16) | (u64(meta & 0xffu) << 24);
    return lo | (hi << 32);
}
AX_FN i32 neu_vs(u64 n) { return i32(i16(n & 0xffffu)); }
AX_FN i32 neu_va(u64 n) { return i32(i16((n >> 16) & 0xffffu)); }
AX_FN u32 neu_ua(u64 n) { return u32((n >> 32) & 0x7fu); }
AX_FN u32 neu_fastp(u64 n) { return u32((n >> 39) & 1u); }
AX_FN u32 neu_ring(u64 n) { return u32((n >> 40) & 0xffu); }
AX_FN u32 neu_ema(u64 n) { return u32((n >> 48) & 0xffu); }
AX_FN u32 neu_tk(u64 n) { return u32((n >> 56) & 0xfu); }
AX_FN u32 neu_refrac(u64 n) { return u32((n >> 60) & 3u); }
AX_FN u32 neu_plat(u64 n) { return u32((n >> 62) & 1u); }
AX_FN u32 neu_slow(u64 n) { return u32((n >> 63) & 1u); }

#if !defined(__CUDA__)
int command(int argc, char **argv);
#endif

} // namespace axion::genesis
