#include "axion/runnel_trace.hpp"
#include "axion/inlet.hpp"
#include "axion/cascade.hpp"
#include "axion/audience.hpp"
#include "retina_files.hpp"
#include "axion/rattle_trials.hpp"
#include "axion/braid_talk.hpp"
#include "axion/braid_inquiry.hpp"
#include "axion/braid_parallax.hpp"
#include "axion/braid_culture.hpp"
#include "evidence_file.hpp"
#include "pulse_words.hpp"
#include "axion/gate_trial.hpp"
#include "axion/field_class.hpp"
#include "axion/shore.hpp"
#include "axion/practice_words.hpp"
#include "axion/linkage_culture.hpp"
#include "axion/linkage_image.hpp"
#include "axion/linkage_design.hpp"
#include "axion/effort_bundle.hpp"
#include "hollow_contest.hpp"
#include "axion/hollow_image.hpp"
#include "flood_storage.hpp"
#include "clay_io.hpp"
#include "flood_body.cuh"
#include "flood_pods.cuh"
#include "flood_material.cuh"
#include "axion/mill.hpp"
#include "axion/artifacts.hpp"
#include "axion/foresight.hpp"
#include "individual_io.hpp"
#include "audience_image.hpp"
#include "narrative_host.hpp"
#include "axion/capillary_tutor.hpp"
#include "capillary_host.hpp"
#include "axion/detritus.hpp"
#include "life_runtime.hpp"
#include "launch_graph.hpp"
#include "pulse_words.hpp"
#include "grounding_eye.cuh"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <memory>
#include <iostream>
#include <numeric>
#include <iomanip>
#include <set>
#include <unordered_map>
#include <sstream>
#include <sys/mman.h>
#include <optional>
#include <unistd.h>
#include <system_error>

namespace axion::life::flood {
namespace {
using namespace runtime;
storage::AgentImage extract_individual(const Habitat &, Word, bool);
bool same_field_individual(const Habitat &, const storage::AgentImage &);
Word number(std::string_view s, Word low, Word high) {
    Word n;
    auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), n);
    if (ec != std::errc{} || end != s.data() + s.size() || n < low || n > high)
        throw std::invalid_argument("invalid floodplain quantity " + std::string(s));
    return n;
}
enum class NeuralMemory { automatic, dense, paged };
enum class NeuralDispatch { automatic, fixed, packed };
struct Options {
    NeuralMemory neural_memory = NeuralMemory::automatic;
    Word neural_cache = ~0u, neural_collection = 4096;
    int neural_file = -1;
    NeuralDispatch neural_dispatch = NeuralDispatch::automatic;
    Word drift_mode = ~0u, drift_memory = ~0u, drift_works = ~0u, drift_fibres = ~0u, drift_tide = ~0u,
         drift_stride = ~0u, drift_load = ~0u;
    Word drift_window = 16, drift_tiles = 64, drift_contest = ~0u, drift_label = ~0u;
    Word drift_probe = 0;
    Word drift_lateral = ~0u, drift_seep = ~0u, drift_stores = ~0u;
    Word glint_sources = ~0u, daylight = ~0u, daylight_period = ~0u, retina_trials = 0, retina_laps = 1;
    Word drift_windings = ~0u, retina_born = ~0u, daylight_profile = 0;
    Word retina_beds = 1, retina_fused = 0, retina_rear_laps = 0, light_banks = ~0u, shore_depth = ~0u;
    Word field_iris = ~0u, retina_iris = ~0u, gaze_purse = 32, retina_probes = 0, field_probes = ~0u, gaze_foraging = 1,
         deliberation = 0, inquiry_hand = retina::directed, reversed_question = 0;
    Word retina_channel = retina::paired, retina_stimulus = retina::water, retina_contrast = 0;
    Word surface_count = 512, natural_order = 0, only_wound = 0;
    std::string surface_input, surface_output;
    Word retina_hearing = 0;
    std::string retina_questions, retina_voice, retina_terms;
    std::string probe_trace;
    Word rotating_samples = 0;
    Word field_audience = ~0u;
    Word field_ink=~0u;
    utterance::ReferenceCourse references{};std::string reference_trace;
    capillary::Course cap_course{};capillary::StreamCourse context_series{};capillary::Signalling signal{};capillary::CompoundCourse compound{};Word signal_channels=1,compound_repeats=0,compound_bench=0;std::string compound_profiles,signal_words;Word signal_voice_exposures=0,signal_dialogues=0,signal_dialects[2]{},signal_audio=0,signal_questions=0,signal_reply=0;std::string context_trace,context_save,context_events;
    Word cap_trials=0,cap_field=~0u,cap_period=~0u,cap_vein=~0u,cap_food=~0u,cap_cargo=~0u,cap_dye=~0u,cap_context=~0u,cap_catalysis=~0u,cap_foragers=~0u;Word hand_mode=~0u;std::string hand_events;bool cap_schedule=true;std::string chemical_witness,catalytic_film,catalytic_proof,cargo_proof,cargo_events,cap_events,cap_film;Word cap_frames=128,cap_window=32,cap_delay=8;
    std::string cap_speech_trace,cap_peer,cap_meeting_trace,cap_manual_trace;Word cap_recipes=0,cap_orders=0;Word cap_vocab=0,cap_questions=0,cap_meetings=0,cap_talk_arm=0,cap_dialect=0,cap_purse=65536;
    std::vector<std::string> signal_neighbours;bool explicit_signal_learning=false;std::string signal_tutor,signal_adult,signal_youth,signal_save_adult,signal_save_youth,signal_trace;
    std::string cap_pupil,cap_trace,cap_lesson,cap_save,cap_screen;Word cap_members=0,cap_natural=0,cap_min=0,cap_bench=0;
    std::string ink_trace,ink_events; Word ink_follow=~0u;
    Word story_trials=0,story_singles=192,story_pairs=64,story_span=5,story_language=0,story_arm=0,story_gate=1,
         story_light=0,story_hearing=0,story_fast=1,story_decoding=1,story_ration=32768,story_partition=0,story_hand=0;
    std::string story_pupil, story_mentor, story_save, story_trace;
    Word accent_generations=0,accent_arm=0;std::string accent_voice,accent_trace;
    Word story_benchmark=0,story_residents=0,story_history=0,story_broods=0,call_practice=48;
    std::string voice_donor,voice_circuit,voice_output,voice_inherit,story_screen;
    Word audience_trials = 0, audience_rounds = 2, audience_arm = audience::intact, audience_generations = 0;
    Word audience_tracer = 0, audience_juveniles = 0;
    Word audience_lessons = 0, audience_pairing = 0, audience_lesson_seed = 85297;
    Word audience_access = audience::split;
    std::string audience_lessons_trace;
    std::string audience_guide, audience_pupil, audience_germ, audience_output, audience_trace, audience_inherit;
    std::string audience_timbre;
    std::string retina_table, retina_trace, retina_host, retina_germ, retina_donor;
    Word inlet_trials = 0, inlet_practice = 36, inlet_residents = 4096, drift_first = 0;
    Word inlet_landscape = inlet::field, inlet_prior = 0;
    Word inlet_generations = 0, inlet_cohort = 2048, inlet_bolus = 64, inlet_selected = 1;
    std::string inlet_trace, inlet_germ, inlet_table, inlet_questions, inlet_voice_trace, inlet_host;
    Word inlet_dialects = 8, inlet_shared = 0;
    Word inlet_cross = 0, receive_loci = 3, voice_route = ~0u;
    bool receive_branch = false;
    bool inlet_carried = false, inlet_ambient = false, inlet_history = false, inlet_old_all = false;
    std::string drift_food_germ, drift_probe_trace;
    Word drift_diary_ticks = 512;
    std::string drift_diary, drift_questions;
    std::string drift_stock, drift_legacy_trace, drift_pool, drift_rival, drift_germ, drift_repertoire;
    Word drift_account = ~0u, drift_profile = 0;
    Word contact_remodel = ~0u, drift_cargo = ~0u, drift_sight = ~0u, drift_watch = 0, drift_pupil = 0,
         drift_look = 16384;
    Word drift_surfaces = 0, drift_surface_seed = 209173, drift_pool_order = 0, drift_loci = 3;
    std::string drift_trace;
    Word brain_bench = 0;
    Word gate_trial = 0, gate_phase = 0, gate_seed = 17333;
    std::string gate_trace, follow_witness, class_trace, field_questions;
    Word feedback_generations=0, feedback_cohort=512, feedback_free=1, feedback_ports=~0u, feedback_survey=0, feedback_dialects=8, survey_carriers=0, after_birth=~0u, receive_named=0;
    std::string feeding_trace,feedback_donor,receive_donor,trace_donor,shore_school,shore_trace;
    Word shore_pupils=1,shore_channel=shore::teaching,shore_reversal=~0u;
    Word apprentices=0,feedback_reversal=0,feedback_frames=2,feedback_hand=0,feedback_rigs=0,donor_ports=1,receive_count=1,feedback_first=0,feedback_second=0;
    bool regulatory_override=false;
    std::string feedback_trace,survey_trace;
    Word field_class = 0, class_seed = 29173, class_practice = 0, class_retests = 0, class_history = 0,
         reader_interval = 0;
    bool class_organs = false, class_prescribed = false;
    struct Lesson {std::string original,taught;bool hand;};
    std::vector<Lesson> field_lessons;
    Configuration c{};
    Word steps = 1024, interval = 256, legacy = 0, extend = 0, survey_origin = 0, assay_interval = 0, slot = ~0u;
    bool requested_frontier = false, remobilize = false, sensory = false, swap = false, cross = false,
         ear_assay = false, forget = false, observer = false;
    std::string load, save, founder, soil, export_mind, colonists, competitors, resident;
    std::vector<std::pair<std::string, Word>> interventions;
    Word silent_new = ~0u, fixed_size = ~0u, neurogenesis = ~0u;
    bool explicit_cells = false;
    Word single_cohort = ~0u, pod_pulse = ~0u, pod_frames = ~0u, pod_regimes = ~0u, pod_spacing = ~0u, pod_mode = ~0u,
         pod_lag = 0, pod_period = 0, pod_credit = ~0u, pod_clock = ~0u, pod_origin = ~0u, cut_cohort = ~0u;
    Word guild_generations = 0, guild_practice = 128, hand_calls = 0, quarry_seasons = 0, quarry_moves = 0,
         quarry_ticks = 96;
    Word work_seasons = 0, work_days = 20, work_capacity = 4096, work_foster = 0, work_origin = 0, work_probes = 0,
         work_subjects = 512, work_shown = 32, work_span = 2, work_test_seed = 87193;
    Word shoal_trials = 0, shoal_seasons = 0, shoal_sample = ~0u;
    material::ShoalOptions shoal{};
    Word tide_period = 0, tide_active = ~0u, tide_sight = ~0u, climate = ~0u, shoal_calls = 0, resident_test = 0;
    Word worksite_mode = ~0u, site_marks = ~0u, field_trials = 0, field_min_age = 0, field_max_age = ~0u;
    bool wash_sites = false, inquiry_replay = false, rest_hands = false;
    Word substrate = ~0u, ore_guidance = ~0u, worksite_calls = 0;
    Word ceramic = ~0u, clay_reach = ~0u, clay_hands = ~0u, clay_muscle = ~0u, clay_hearing = ~0u, clay_context = ~0u;
    Word clay_seasons = 0, clay_test = 0, clay_classes = 256, clay_prep = 256, clay_history = 256, clay_lessons = 96,
         clay_taps = 2, clay_select = 1, clay_strangers = 0;
    Word clay_dig = 0, clay_foster = 0, clay_switches = 0, clay_calls = 0, clay_breed_seed = 0;
    bool wash_ware = false;
    Word hollow_mode=~0u,hollow_odometry=~0u,hollow_anchor=~0u,hollow_rich=~0u,hollow_current=~0u,hollow_contest=~0u,hollow_slots=0;
    Word hollow_test=0,hollow_seasons=0,hollow_population=512,hollow_seed=99017,hollow_paid=1,hollow_calls=0;
    bool hollow_survey=false,hollow_varied=true;
    std::string hollow_scholar,hollow_origin,hollow_optics,hollow_carrier;
    Word effort_mode=~0u, effort_period=~0u, effort_trails=~0u, effort_comp=~0u, effort_audit=~0u,effort_span=~0u,effort_window=~0u,effort_sculpt=~0u,effort_spending=~0u,effort_sample_rule=~0u,effort_paving=~0u;
    Word effort_seasons=0,effort_population=1024,effort_seed=41761,effort_test=0,effort_calls=0,effort_paid=1,effort_practice=1;
    bool effort_wash=false,effort_survey=false;
    std::string effort_germs,effort_scholar,effort_carrier;
    Word practice_island=1,practice_teaching=0,practice_focus=~0u,practice_mode=~0u,practice_sight=~0u,practice_join=~0u,practice_resource=~0u,practice_approach=~0u,practice_credit=~0u,practice_comp=~0u,
        practice_cap=0,practice_seed=59317,practice_epochs=0,practice_adults=2048,practice_paid=1,practice_test=0,practice_speed=0;
    Word reed_mode=~0u,fold_period=0,reed_weather=~0u,reed_seed=71633,reed_rear=0,reed_adults=1024,reed_paid=1,
         reed_cut=~0u,reed_patrol=~0u,reed_keep=0,reed_life=32,reed_grain=~0u,reed_strata=0,reed_contest=~0u,reed_diet=~0u,reed_general=0,reed_examples=4,reed_relation=~0u,reed_school=0,reed_watch=~0u,reed_invited=0,reed_cases=0,reed_span=12,reed_turns=6,reed_play=8,reed_calls=0,reed_rig=0,reed_flex=~0u;
    bool fold_census=false,reed_strip=false;std::string reed_stock,reed_scholar,reed_carrier,reed_trace;
    Word braid_mode=~0u,braid_mesh=~0u,braid_seed=36479,braid_epochs=0,braid_parents=1024,braid_paid=1,braid_cases=0,
         braid_rep=4,braid_terms=4,braid_length=5,braid_chiral=0,braid_acquire=0,braid_bench=0;
    Word braid_perspectives=0,braid_views=5,braid_view_budget=1;std::string braid_view_trace;
    bool braid_episodes=false;Word braid_inquiries=0,braid_examples=32,braid_holdout=1;std::string braid_report_trace;
    Word braid_eyes=~0u,braid_sediment=~0u,braid_channels=~0u,braid_phase=~0u;
    Word braid_school=0,braid_villages=0,braid_lives=4,braid_visits=32,braid_village=0,braid_history=0;std::string braid_chorister,braid_song;
    Word braid_dialogues=0,braid_questions=4,braid_pan=0;
    std::string braid_source,braid_voice,braid_trace,braid_workers,braid_transcript;
    Word rattle_lesion=0,rattle_load=~0u,rattle_binding=~0u,rattle_sparse=~0u,rattle_nursery=0,rattle_calls=0,rattle_mode=~0u,rattle_phase=~0u,rattle_seed=78283,rattle_trials=0,rattle_cycles=0,rattle_pop=1024,rattle_paid=1,rattle_exposure=48;
    std::string rattle_visitors,rattle_stock,rattle_trace,rattle_speaker,rattle_carrier;
    Word wick_mode=~0u,wick_seed=12739,wick_length=0;bool wick_census=false;
    bool practice_survey=false,practice_strip=false;
    std::string practice_stock,practice_scholar,practice_carrier;
    Word link_water=~0u,link_climate=0,link_recall=0,link_focus=~0u,link_fit=~0u,link_mode=~0u,link_soc=~0u,link_joints=~0u,link_use=~0u,link_comp=~0u,link_arena=0,link_period=0,link_seed=51031,
         link_epochs=0,link_adults=1024,link_paid=1,link_test=0,link_width=4,link_practice=2,link_bench=0,link_witness=0;
    bool link_census=false,link_strip=false;
    std::string link_origin,link_law,link_scholar,link_carrier;
    Word press_use = ~0u, press_test = 0, press_seasons = 0, press_population = 256, press_seed = 98479,
         press_paid = 1, press_bench = 0, press_varied = 0, press_mixed = 0, press_strata = ~0u, press_feel = ~0u;
    bool press_survey = false;
    std::string press_scholar;
    std::string clay_origin, clay_specimen, clay_export, clay_settlers, clay_peers;
    Word clay_site = ~0u;
    bool wash_echo = false;
    std::string reed_reference;
    Word reed_census = 0, reed_bench = 0, reed_legacy = 0;
    Word chorus_hold = ~0u, reed_work = ~0u, reed_tutors = 0;
    std::string phonetic_germ;
    Word chorus = ~0u, reed_foster_rounds = 0, reed_visit_school = 128;
    Word reed_view = ~0u, reed_medium = ~0u, reed_growth = ~0u, reed_clutch = ~0u;
    Word reed_bands = 0, reed_wire = ~0u, reed_windows = ~0u, reed_period = 0, reed_trials = 0, reed_practice = 0,
         reed_seasons = 0, reed_visits = 96, reed_foster = 0, reed_origin = 0;
    Word brine_species = 0, brine_flow = ~0u, brine_seasons = 0, brine_trials = 0, brine_trial_species = 0;
    Word paired_material = 0, brine_plan = ~0u, brine_senses = ~0u, brine_arbors = 1, brine_shore = 1, brine_calls = 0,
         brine_tides = 8, brine_examples = 9, brine_drift = 0, brine_origin = 0;
    Word artifact_samples = 0, mineral_water = ~0u, hydraulic_trial = 0;
    Word minerals = ~0u, material_density = ~0u, material_bonds = ~0u, material_agency = ~0u, material_vision = ~0u,
         material_legacy = 0;
    bool growth_assay = false, erase_pods = false;
    std::string pod_germ, hand_germ;
    Word raft_flush = 0, raft_frames = 0, raft_seasons = 0, rafts = 4096, raft_days = 256, raft_drift = 0;
    Word remap_training = 0, remap_observe = 64, remap_cues = 1;
    Word motor_calls = 0, assay_pods = 0, pod_subjects = 512, pod_test_seed = 42731;
};
Options parse(int argc, char **argv) {
    Options o;
    for (int i = 2; i < argc; ++i) {
        std::string f = argv[i];
        if(f=="--reed-census"){o.fold_census=true;continue;}
        if(f=="--flatten-reeds"){o.reed_strip=true;continue;}
        if(f=="--wick-census"){o.wick_census=true;continue;}
        if(f=="--unthread-works"){o.practice_strip=true;continue;}
        if(f=="--practice-census"){o.practice_survey=true;continue;}
        if(f=="--linkage-census"){o.link_census=true;continue;}
        if(f=="--disturb-linkages"){o.link_strip=true;continue;}
        if(f=="--wash-treads"){o.effort_wash=true;continue;}
        if(f=="--effort-census"){o.effort_survey=true;continue;}
        if (f=="--hollow-census") {o.hollow_survey=true;continue;}
        if (f == "--brine-competition") {
            o.paired_material = 1;
            continue;
        }
        if (f == "--assay-inquiry") {
            o.inquiry_replay = true;
            continue;
        }
        if (f == "--press-census") { o.press_survey=true; continue; }
        if (f == "--wash-clay") {
            o.wash_ware = true;
            continue;
        }
        if (f == "--wash-sounds") {
            o.wash_echo = true;
            continue;
        }
        if (f == "--wash-sites") {
            o.wash_sites = true;
            continue;
        }
        if (f == "--assay-growth") {
            o.growth_assay = true;
            continue;
        }
        if (f == "--erase-pods") {
            o.erase_pods = true;
            continue;
        }
        if (f == "--rest-hands") {
            o.rest_hands = true;
            continue;
        }
        if (f == "--observer") {
            o.observer = true;
            continue;
        }
        if (f == "--assay-calls") {
            o.ear_assay = true;
            continue;
        }
        if (f == "--forget-calls") {
            o.forget = true;
            continue;
        }
        if(f=="--braid-episodes"){o.braid_episodes=true;continue;}
        if (f == "--swap" || f == "--cross-brains") {
            if (f == "--swap")
                o.swap = true;
            else
                o.cross = true;
            continue;
        }
        if (f == "--remobilize") {
            o.remobilize = true;
            continue;
        }
        if (++i == argc)
            throw std::invalid_argument("missing value for " + f);
        std::string v = argv[i];
        if (f == "--brain-dispatch" && (v == "auto" || v == "fixed" || v == "packed"))
            o.neural_dispatch = v == "auto"    ? NeuralDispatch::automatic
                                : v == "fixed" ? NeuralDispatch::fixed
                                               : NeuralDispatch::packed;
        else if (f == "--brain-memory" && (v == "auto" || v == "dense" || v == "paged"))
            o.neural_memory = v == "auto"    ? NeuralMemory::automatic
                              : v == "dense" ? NeuralMemory::dense
                                             : NeuralMemory::paged;
        else if (f == "--brain-pool-pages")
            o.neural_cache = number(v, 0, 1u << 20);
        else if (f == "--brain-collect")
            o.neural_collection = number(v, 0, ~0u);
        else if (f == "--brain-file" && (v == "dense" || v == "compact"))
            o.neural_file = v == "compact";
        else if (f == "--brain-bench")
            o.brain_bench = number(v, 1, 512);
        else if (f == "--gate-trial" && (v == "crossover" || v == "sham"))
            o.gate_trial = v == "sham" ? 1 : 2;
        else if (f == "--gate-phase")
            o.gate_phase = number(v, 0, 2);
        else if (f == "--drift" && (v == "tidal" || v == "steady" || v == "off"))
            o.drift_mode = v == "off" ? 0u : Word(drift::moving) | (v == "tidal" ? Word(drift::tide) : 0u);
        else if (f == "--drift-contest" && (v == "vision" || v == "reverse-vision"))
            o.drift_contest =
                drift::contest | drift::visual_contest | (v == "reverse-vision" ? Word(drift::swapped) : 0u);
        else if (f == "--drift-contest" && (v == "lateral" || v == "reverse-lateral"))
            o.drift_contest =
                drift::contest | drift::lateral_contest | (v == "reverse-lateral" ? Word(drift::swapped) : 0u);
        else if (f == "--drift-contest" && (v == "off" || v == "reciprocal" || v == "swapped" || v == "sham"))
            o.drift_contest = v == "off" ? 0u
                                         : Word(drift::contest) | (v == "swapped" ? Word(drift::swapped)
                                                                   : v == "sham"  ? Word(drift::sham)
                                                                                  : 0u);
        else if (f == "--drift-profile")
            o.drift_profile = number(v, 1, 2048);
        else if (f == "--drift-account" && (v == "auto" || v == "warp" || v == "atomic" || v == "hybrid"))
            o.drift_account = v == "auto" ? ~0u : v == "warp" ? 3u : v == "hybrid" ? 1u : 0u;
        else if (f == "--contact-remodel" && (v == "gated" || v == "inherited"))
            o.contact_remodel = v == "inherited";
        else if (f == "--drift-lateral" && (v == "inherited" || v == "local" || v == "misaligned" ||
                                            v == "contextual" || v == "misaligned-contextual"))
            o.drift_lateral =
                v == "local"
                    ? 0u
                    : Word(drift::lateral) |
                          ((v == "misaligned" || v == "misaligned-contextual") ? Word(drift::misaligned) : 0u) |
                          ((v == "contextual" || v == "misaligned-contextual") ? Word(drift::recruitment) : 0u);
        else if (f == "--drift-crossing" && (v == "exchange" || v == "seep"))
            o.drift_seep = v == "seep" ? Word(drift::seepage) : 0u;
        else if (f == "--drift-stores" && (v == "open" || v == "sills"))
            o.drift_stores = v == "sills" ? Word(drift::storage) : 0u;
        else if (f == "--drift-windings" && (v == "open" || v == "woven"))
            o.drift_windings = v == "woven";
        else if (f == "--retina-born-after")
            o.retina_born = number(v, 0, ~0u - 1);
        else if (f=="--daylight-ink" && (v=="off" || v=="pigments" || v=="chorus" || v=="reversed" || v=="dialects"))
            o.field_ink=v=="dialects"?4u:v=="reversed"?3u:v=="chorus"?2u:v=="pigments"?1u:0u;
        else if (f=="--ink-events") o.ink_events=v;
        else if (f=="--ink-follow") o.ink_follow=number(v,0,~0u-1);
        else if (f=="--ink-trace") o.ink_trace=v;
        else if (f=="--story-references")o.references.examples=number(v,0,512);
        else if (f=="--story-reference-trials")o.references.trials=number(v,1,32768);
        else if(f=="--capillary-cargo"&&(v=="still"||v=="pressure"||v=="displaced"))o.cap_cargo=v=="pressure"?detritus::gear:v=="displaced"?detritus::displaced:0;
        else if(f=="--capillary-cargo-dye"&&(v=="fixed"||v=="exchange"))o.cap_dye=v=="exchange";
        else if(f=="--material-hands"&&(v=="off"||v=="live"||v=="fixed"||v=="disconnected"))o.hand_mode=v=="off"?0:v=="live"?1:v=="fixed"?2:3;
        else if(f=="--hand-events")o.hand_events=v;
        else if(f=="--chemical-foraging"&&(v=="live"||v=="fixed"||v=="off"))o.cap_foragers=v=="live"?1:v=="fixed"?2:0;
        else if(f=="--capillary-catalysis" && (v=="bound"||v=="inert"))o.cap_catalysis=v=="bound";
        else if(f=="--capillary-memory" && (v=="adaptive"||v=="single"||v=="regional"))o.cap_context=v=="regional"?3:v=="adaptive";
        else if(f=="--capillary-dispatch"&&(v=="scheduled"||v=="full"))o.cap_schedule=v=="scheduled";
        else if(f=="--capillary-catalysis-proof")o.catalytic_proof=v;
        else if(f=="--capillary-cargo-proof")o.cargo_proof=v;
        else if(f=="--capillary-cargo-events")o.cargo_events=v;
        else if(f=="--capillary-food" && (v=="direct"||v=="pigments"))o.cap_food=v=="pigments";
        else if(f=="--capillary-field" && (v=="live"||v=="watch"||v=="fixed"||v=="lookup"||v=="off"))
            o.cap_field=v=="off"?0u:v=="watch"?2u:v=="fixed"?3u:v=="lookup"?4u:1u;
        else if(f=="--capillary-period"){Word period=number(v,1,256);if(!std::has_single_bit(period))throw std::invalid_argument("capillary period must be a power of two");o.cap_period=std::countr_zero(period);}
        else if(f=="--capillary-mineral")o.cap_vein=v=="regional"?32u:number(v,0,31);
        else if(f=="--chemical-witness")o.chemical_witness=v;
        else if(f=="--catalytic-film")o.catalytic_film=v;
        else if(f=="--capillary-film")o.cap_film=v;
        else if(f=="--capillary-frames")o.cap_frames=number(v,2,8192);
        else if(f=="--capillary-window"){o.cap_window=number(v,4,1024);if(!std::has_single_bit(o.cap_window))throw std::invalid_argument("use a power-of-two recording window");}
        else if(f=="--capillary-delay")o.cap_delay=number(v,1,16);
        else if(f=="--capillary-events")o.cap_events=v;
        else if(f=="--catalytic-voices")o.signal_voice_exposures=number(v,0,8192);
        else if(f=="--catalytic-dialogues")o.signal_dialogues=number(v,1,1048576);
        else if(f=="--catalytic-speech-trace")o.signal_words=v;
        else if(f=="--catalytic-writer-dialect")o.signal_dialects[0]=number(v,0,1);
        else if(f=="--catalytic-reader-dialect")o.signal_dialects[1]=number(v,0,1);
        else if(f=="--catalytic-questions")o.signal_questions=number(v,0,2);
        else if(f=="--catalytic-reply"&&(v=="actual"||v=="withheld"||v=="exchanged"))o.signal_reply=v=="withheld"?1:v=="exchanged"?2:0;
        else if(f=="--catalytic-audio"&&(v=="clear"||v=="silent"||v=="partial"||v=="reordered"||v=="duplicate"||v=="unknown"||v=="blind"||v=="patchy"))o.signal_audio=v=="silent"?1:v=="partial"?2:v=="reordered"?3:v=="duplicate"?4:v=="unknown"?5:v=="blind"?6:v=="patchy"?7:0;
        else if(f=="--catalytic-benchmark")o.compound_bench=number(v,1,8388608);
        else if(f=="--catalytic-repeat")o.compound_repeats=number(v,1,65536);
        else if(f=="--catalytic-profiles")o.compound_profiles=v;
        else if(f=="--catalytic-channels")o.signal_channels=number(v,1,2);
        else if(f=="--catalytic-omit")o.compound.observed=number(v,0,3);
        else if(f=="--catalytic-wean")o.compound.wean=number(v,0,65536);
        else if(f=="--catalytic-diet"&&(v=="single"||v=="plumes"||v=="pooled"))o.compound.diet=v=="plumes"?1:v=="pooled"?2:0;
        else if(f=="--catalytic-arbor"&&(v=="native"||v=="joint"))o.compound.arbor=v=="joint";
        else if(f=="--catalytic-neighbour")o.signal_neighbours.push_back(v);
        else if(f=="--catalytic-tutor"&&(v=="sender"||v=="receiver")){
            if(++i==argc)throw std::invalid_argument("choose the tutor’s saved animal");
            o.signal.tutor=v=="sender"?1:2;o.signal_tutor=argv[i];
        }
        else if(f=="--catalytic-learning"&&(v=="both"||v=="sender"||v=="receiver")){
            o.explicit_signal_learning=true;o.signal.learners=v=="both"?3:v=="sender"?1:2;
        }
        else if(f=="--catalytic-pair"||f=="--catalytic-export"){
            if(++i==argc)throw std::invalid_argument("a material relay needs separate sender and receiver files");
            if(f=="--catalytic-pair"){o.signal_adult=v;o.signal_youth=argv[i];}
            else{o.signal_save_adult=v;o.signal_save_youth=argv[i];}
        }
        else if(f=="--catalytic-history")o.signal.first=number(v,0,~0u);
        else if(f=="--catalytic-test-seed")o.signal.holdout=number(v,0,~0u);
        else if(f=="--catalytic-trace")o.signal_trace=v;
        else if(f=="--catalytic-lessons")o.signal.lessons=number(v,32,65536);
        else if(f=="--catalytic-prior"&&(v=="lived"||v=="empty"))o.signal.retained=v=="lived";
        else if(f=="--catalytic-histories")o.signal.replicas=number(v,1,1024);
        else if(f=="--catalytic-tests")o.signal.tests=number(v,1,1024);
        else if(f=="--catalytic-feedback"&&(v=="food"||v=="silent"||v=="unrelated"))o.signal.feedback=v=="silent"?1:v=="unrelated"?2:0;
        else if(f=="--catalytic-medium"&&(v=="active"||v=="inert"))o.signal.inert=v=="inert";
        else if(f=="--context-streams")o.context_series.streams=number(v,1,8192);
        else if(f=="--context-episodes")o.context_series.rounds=number(v,1,64);
        else if(f=="--context-questions" && (v=="fixed"||v=="varied"))o.context_series.varied=v=="varied";
        else if(f=="--context-change")o.context_series.change_percent=number(v,0,100);
        else if(f=="--context-transfer-rock")o.context_series.second=number(v,0,31);
        else if(f=="--context-transfer-at")o.context_series.at=number(v,1,63);
        else if(f=="--context-view-jitter")o.context_series.jitter=number(v,0,512);
        else if(f=="--context-events")o.context_events=v;
        else if(f=="--context-trace")o.context_trace=v;
        else if(f=="--context-save")o.context_save=v;
        else if(f=="--context-arm"&&(v=="adaptive"||v=="single"||v=="cleared"||v=="unchecked"||v=="clamped"||v=="exchanged"||v=="retention"))
            o.context_series.mode=v=="single"?capillary::single_memory:v=="cleared"?capillary::examiner_clears:v=="unchecked"?capillary::unchecked:
                v=="clamped"?capillary::intervention_clamped:v=="exchanged"?capillary::feedback_exchanged:v=="retention"?capillary::retention_flipped:capillary::contextual_learning;
        else if(f=="--capillary-trials")o.cap_trials=number(v,1,65536);
        else if(f=="--capillary-recipes")o.cap_recipes=number(v,1,8192);
        else if(f=="--capillary-instructions")o.cap_orders=number(v,1,16384);
        else if(f=="--capillary-instruction-trace")o.cap_manual_trace=v;
        else if(f=="--capillary-vocabulary")o.cap_vocab=number(v,0,2048);
        else if(f=="--capillary-interviews")o.cap_questions=number(v,1,16384);
        else if(f=="--capillary-discourse")o.cap_speech_trace=v;
        else if(f=="--capillary-partner")o.cap_peer=v;
        else if(f=="--capillary-meetings")o.cap_meetings=number(v,1,16384);
        else if(f=="--capillary-exchanges")o.cap_meeting_trace=v;
        else if(f=="--capillary-language-ration")o.cap_purse=number(v,0,1u<<26);
        else if(f=="--capillary-dialect" && (v=="brook"||v=="other"))o.cap_dialect=v=="other";
        else if(f=="--capillary-hearing" && (v=="intact"||v=="exchanged"||v=="wrong-name"||v=="closed"||v=="reversed"||v=="mute"))
            o.cap_talk_arm=v=="intact"?0u:v=="exchanged"?1u:v=="wrong-name"?2u:v=="closed"?3u:v=="reversed"?4u:5u;
        else if(f=="--capillary-residents")o.cap_members=number(v,1,8192);
        else if(f=="--capillary-contacts")o.cap_min=number(v,0,64);
        else if(f=="--capillary-works" && (v=="living"||v=="assembled"))o.cap_natural=v=="living";
        else if(f=="--capillary-screen")o.cap_screen=v;
        else if(f=="--capillary-save")o.cap_save=v;
        else if(f=="--capillary-benchmark")o.cap_bench=number(v,128,1u<<22);
        else if(f=="--capillary-pupil")o.cap_pupil=v;
        else if(f=="--capillary-trace")o.cap_trace=v;
        else if(f=="--capillary-lesson")o.cap_lesson=v;
        else if(f=="--capillary-views")o.cap_course.views=number(v,0,512);
        else if(f=="--capillary-probes")o.cap_course.probes=number(v,0,256);
        else if(f=="--capillary-budget")o.cap_course.budget=number(v,0,8192);
        else if(f=="--capillary-depth")o.cap_course.depth=number(v,2,14);
        else if(f=="--capillary-history" && (v=="retained"||v=="empty"))o.cap_course.empty_contacts=v=="empty";
        else if(f=="--capillary-supply" && (v=="scarce"||v=="plentiful"))o.cap_course.scarce=v=="scarce";
        else if(f=="--capillary-arm" && v=="backwards")o.cap_course.arm=capillary::reverse_order;
        else if(f=="--capillary-moves")o.cap_course.moves=number(v,0,capillary::grips);
        else if(f=="--capillary-rock")o.cap_course.rock=number(v,0,31);
        else if(f=="--capillary-arm" && (v=="active"||v=="passive"||v=="random"||v=="crossed"||v=="frozen"||v=="lookup"||v=="miswired"||v=="erased"||v=="yoked"||v=="severed"))
            o.cap_course.arm=v=="passive"?capillary::passive:v=="random"?capillary::random_grips:v=="crossed"?capillary::crossed_results:v=="frozen"?capillary::frozen:v=="lookup"?capillary::lookup:v=="miswired"?capillary::opposite_hand:v=="erased"?capillary::erased:v=="yoked"?capillary::yoked:v=="severed"?capillary::severed:capillary::active;
        else if (f=="--story-reference-language" && (v=="normal" || v=="swapped"))o.references.swapped=v=="swapped";
        else if (f=="--story-reference-input" && (v=="heard" || v=="other"))o.references.other=v=="other";
        else if (f=="--story-reference-pair" && (v=="live" || v=="crossed"))o.references.mispaired=v=="crossed";
        else if (f=="--story-reference-trace")o.reference_trace=v;
        else if (f=="--story-trials")o.story_trials=number(v,1,32768);
        else if (f=="--story-singles")o.story_singles=number(v,0,4096);
        else if (f=="--story-pairs")o.story_pairs=number(v,0,2048);
        else if (f=="--story-length")o.story_span=number(v,1,utterance::depth);
        else if (f=="--story-ration")o.story_ration=number(v,0,1u<<26);
        else if (f=="--story-partition")o.story_partition=number(v,0,3);
        else if (f=="--story-language" && (v=="river"||v=="reverse"||v=="varied"||v=="native"))
            o.story_language=v=="reverse"?utterance::reverse:v=="varied"?utterance::varied:v=="native"?utterance::native:utterance::river;
        else if (f=="--story-exposure" && (v=="paired"||v=="crossed"||v=="shuffled"||v=="fixed"||v=="singles"))
            o.story_arm=v=="crossed"?utterance::exchanged_scenes:v=="shuffled"?utterance::shuffled_words:v=="fixed"?utterance::fixed:v=="singles"?utterance::separated:utterance::coupled;
        else if(f=="--story-gate" && (v=="inherited"||v=="open"||v=="shut"||v=="local"||v=="diffuse"))o.story_gate=v=="open"?0u:v=="shut"?2u:v=="local"?3u:v=="diffuse"?4u:1u;
        else if(f=="--story-light" && (v=="clear"||v=="natural"))o.story_light=v=="natural";
        else if(f=="--story-hearing" && (v=="tutor"||v=="reversed"||v=="mute"||v=="self"))
            o.story_hearing=v=="tutor"?0u:v=="reversed"?1u:v=="mute"?2u:3u;
        else if(f=="--story-hand" && (v=="aligned"||v=="crossed"))o.story_hand=v=="crossed";
        else if(f=="--story-motor" && (v=="packed"||v=="plastic"||v=="table"))o.story_fast=v=="table"?2u:Word(v=="packed");
        else if(f=="--story-benchmark")o.story_benchmark=number(v,256,1u<<23);
        else if(f=="--story-decoding" && (v=="on"||v=="off"))o.story_decoding=v=="on";
        else if(f=="--story-pupil")o.story_pupil=v;
        else if(f=="--story-mentor")o.story_mentor=v;
        else if(f=="--story-save")o.story_save=v;
        else if(f=="--story-trace")o.story_trace=v;
        else if(f=="--story-residents")o.story_residents=number(v,1,16384);
        else if(f=="--story-min-events")o.story_history=number(v,0,utterance::depth);
        else if(f=="--story-broods")o.story_broods=number(v,0,2048);
        else if(f=="--call-transmissions")o.accent_generations=number(v,1,24);
        else if(f=="--call-neighbor")o.accent_voice=v;
        else if(f=="--call-trace")o.accent_trace=v;
        else if(f=="--call-exposure" && (v=="paired"||v=="crossed"||v=="cut"))o.accent_arm=v=="cut"?2u:Word(v=="crossed");
        else if(f=="--call-practice")o.call_practice=number(v,0,256);
        else if(f=="--story-founder")o.voice_donor=v;
        else if(f=="--story-circuit")o.voice_circuit=v;
        else if(f=="--story-write")o.voice_output=v;
        else if(f=="--story-inherit")o.voice_inherit=v;
        else if(f=="--story-screen")o.story_screen=v;
        else if (f == "--audience-trials")
            o.audience_trials = number(v, 1, 32768);
        else if (f == "--audience-medium" && (v == "tracer" || v == "speech"))
            o.audience_tracer = v == "tracer";
        else if (f == "--audience-access")
            o.audience_access = audience::access_mode(v);
        else if (f == "--audience-juveniles")
            o.audience_juveniles = number(v, 1, 65536);
        else if (f == "--audience-rounds")
            o.audience_rounds = number(v, 0, 32);
        else if (f == "--audience-lessons")
            o.audience_lessons = number(v, 2, 256);
        else if (f == "--audience-lesson-seed")
            o.audience_lesson_seed = number(v, 0, ~0u);
        else if (f == "--audience-lesson-trace")
            o.audience_lessons_trace = v;
        else if (f == "--audience-pairing" && (v == "live" || v == "shifted" || v == "fixed"))
            o.audience_pairing = v == "shifted" ? 1u : v == "fixed" ? 2u : 0u;
        else if (f == "--audience-broods")
            o.audience_generations = number(v, 0, 4096);
        else if (f == "--audience-arm")
            o.audience_arm = audience::treatment(v);
        else if (f == "--audience-guide")
            o.audience_guide = v;
        else if (f == "--audience-timbre")
            o.audience_timbre = v;
        else if (f == "--audience-pupil")
            o.audience_pupil = v;
        else if (f == "--audience-circuit")
            o.audience_germ = v;
        else if (f == "--audience-write")
            o.audience_output = v;
        else if (f == "--audience-trace")
            o.audience_trace = v;
        else if (f == "--audience-inherit")
            o.audience_inherit = v;
        else if (f == "--daylight-audience" &&
                 (v == "visible" || v == "off" || v == "exchanged" || v == "lineages" || v == "reverse-lineages"))
            o.field_audience = v == "off"         ? 0u
                               : v == "visible"   ? 1u
                               : v == "exchanged" ? 2u
                               : v == "lineages"  ? 3u
                                                  : 4u;
        else if (f == "--probe-sampling" && (v == "sites" || v == "events"))
            o.rotating_samples = v == "events";
        else if (f == "--probe-trace")
            o.probe_trace = v;
        else if (f == "--retina-deliberate")
            o.deliberation = number(v, 0, 192);
        else if (f == "--inquiry-context" && (v == "heard" || v == "reversed"))
            o.reversed_question = v == "reversed";
        else if (f == "--inquiry-hand" && (v == "chosen" || v == "random" || v == "crossed" || v == "visible-match"))
            o.inquiry_hand = v == "crossed"         ? retina::reversed_hands
                             : v == "random"        ? retina::random_hand
                             : v == "visible-match" ? retina::matched_hand
                                                    : retina::directed;
        else if (f == "--daylight-probes" &&
                 (v == "ambient" || v == "stirred" || v == "crossed" || v == "lineages" || v == "reverse-lineages"))
            o.field_probes = v == "stirred"            ? 1u
                             : v == "crossed"          ? 2u
                             : v == "lineages"         ? 3u
                             : v == "reverse-lineages" ? 4u
                                                       : 0u;
        else if (f == "--retina-probes" &&
                 (v == "ambient" || v == "chosen" || v == "crossed" || v == "random" || v == "visible-match"))
            o.retina_probes = v == "chosen"          ? retina::directed
                              : v == "crossed"       ? retina::reversed_hands
                              : v == "random"        ? retina::random_hand
                              : v == "visible-match" ? retina::matched_hand
                                                     : retina::ambient;
        else if (f == "--daylight-gaze" && (v == "motor" || v == "inherited"))
            o.field_iris = v == "inherited";
        else if (f == "--retina-gaze" && v == "visible-yoke")
            o.retina_iris = retina::visible_yoke;
        else if (f == "--retina-gaze" && v == "natal")
            o.retina_iris = retina::natal_eye;
        else if (f == "--retina-gaze" &&
                 (v == "passive" || v == "inherited" || v == "yoked" || v == "open" || v == "closed"))
            o.retina_iris = v == "inherited" ? retina::voluntary
                            : v == "yoked"   ? retina::yoked
                            : v == "open"    ? retina::open_eye
                            : v == "closed"  ? retina::shut_eye
                                             : retina::passive;
        else if (f == "--gaze-foraging")
            o.gaze_foraging = number(v, 1, 32);
        else if (f == "--gaze-purse")
            o.gaze_purse = number(v, 0, 384);
        else if (f == "--daylight-profile")
            o.daylight_profile = number(v, 1, 2048);
        else if (f == "--daylight" && (v == "off" || v == "live" || v == "displaced" || v == "shutter"))
            o.daylight = v == "off"         ? retina::closed
                         : v == "live"      ? retina::paired
                         : v == "displaced" ? retina::displaced
                                            : retina::dark;
        else if (f == "--glint-sources" && (v == "other" || v == "independent"))
            o.glint_sources = v == "independent";
        else if (f == "--daylight-period") {
            Word period = number(v, 1, 128);
            if (!std::has_single_bit(period))
                throw std::invalid_argument("the illumination period must be a power of two");
            o.daylight_period = std::countr_zero(period);
        } else if (f == "--retina-trials")
            o.retina_trials = number(v, 4, 8192);
        else if (f == "--retina-beds") {
            o.retina_beds = number(v, 1, 4);
            if (!std::has_single_bit(o.retina_beds))
                throw std::invalid_argument("choose one, two or four physical beds");
        } else if (f == "--retina-banks" && (v == "inherited" || v == "shared"))
            o.retina_fused = v == "shared";
        else if (f == "--retina-rear-laps")
            o.retina_rear_laps = number(v, 1, 32);
        else if (f == "--daylight-banks" && (v == "single" || v == "tidal"))
            o.light_banks = v == "tidal";
        else if (f == "--shore-depth" && (v == "open" || v == "submerged" || v == "shoals"))
            o.shore_depth = v == "shoals" ? 2u : Word(v == "submerged");
        else if (f == "--retina-laps")
            o.retina_laps = number(v, 1, 32);
        else if (f == "--retina-works")
            o.surface_input = v;
        else if (f == "--record-watercourses")
            o.surface_output = v;
        else if (f == "--watercourse-sites")
            o.surface_count = number(v, 1, 65536);
        else if (f == "--watercourse-sample" && (v == "all" || v == "wound"))
            o.only_wound = v == "wound";
        else if (f == "--retina-order" && (v == "spoken" || v == "woven"))
            o.natural_order = v == "woven";
        else if (f == "--retina-terms")
            o.retina_terms = v;
        else if (f == "--retina-questions")
            o.retina_questions = v;
        else if (f == "--retina-voice")
            o.retina_voice = v;
        else if (f == "--retina-hearing" && (v == "live" || v == "reverse" || v == "mute"))
            o.retina_hearing = v == "live" ? 0u : v == "reverse" ? 1u : 2u;
        else if (f == "--retina-trace")
            o.retina_trace = v;
        else if (f == "--retina-table")
            o.retina_table = v;
        else if (f == "--retina-germ")
            o.retina_germ = v;
        else if (f == "--retina-donor")
            o.retina_donor = v;
        else if (f == "--retina-host")
            o.retina_host = v;
        else if (f == "--retina-contrast" && (v == "natural" || v == "bright"))
            o.retina_contrast = v == "bright";
        else if (f == "--retina-stimulus" && (v == "water" || v == "tone"))
            o.retina_stimulus = v == "tone" ? retina::tone : retina::water;
        else if (f == "--retina-channel" && v == "independent")
            o.retina_channel = retina::independent;
        else if (f == "--retina-channel" && (v == "live" || v == "displaced" || v == "shutter" || v == "closed"))
            o.retina_channel = v == "live"        ? retina::paired
                               : v == "displaced" ? retina::displaced
                               : v == "shutter"   ? retina::dark
                                                  : retina::closed;
        else if (f == "--inlet-trials")
            o.inlet_trials = number(v, 4, 16384);
        else if (f == "--inlet-practice")
            o.inlet_practice = number(v, 0, 192);
        else if (f == "--inlet-prior")
            o.inlet_prior = number(v, 0, 192);
        else if (f == "--inlet-generations")
            o.inlet_generations = number(v, 0, 8192);
        else if (f == "--inlet-cohort")
            o.inlet_cohort = number(v, 32, 65536);
        else if (f == "--inlet-bolus")
            o.inlet_bolus = number(v, 1, 256);
        else if (f == "--inlet-selection" && (v == "food" || v == "uniform"))
            o.inlet_selected = v == "food";
        else if (f == "--inlet-landscape" && (v == "field" || v == "channels" || v == "basin" || v == "mixed"))
            o.inlet_landscape = v == "mixed"      ? inlet::mixed
                                : v == "basin"    ? inlet::basin
                                : v == "channels" ? inlet::channels
                                                  : inlet::field;
        else if (f == "--inlet-residents")
            o.inlet_residents = number(v, 1, 1048576);
        else if (f == "--drift-first")
            o.drift_first = number(v, 0, 1048576);
        else if (f == "--inlet-trace")
            o.inlet_trace = v;
        else if (f == "--inlet-table")
            o.inlet_table = v;
        else if (f == "--inlet-questions")
            o.inlet_questions = v;
        else if (f == "--inlet-voice-trace")
            o.inlet_voice_trace = v;
        else if (f == "--inlet-state" && (v == "carried" || v == "fresh"))
            o.inlet_carried = v == "carried";
        else if (f == "--inlet-background" && (v == "quiet" || v == "living"))
            o.inlet_ambient = v == "living";
        else if (f == "--inlet-history" && (v == "empty" || v == "living"))
            o.inlet_history = v == "living";
        else if (f == "--inlet-old-inlets" && (v == "three" || v == "all"))
            o.inlet_old_all = v == "all";
        else if (f == "--inlet-host")
            o.inlet_host = v;
        else if (f == "--inlet-dialects")
            o.inlet_dialects = number(v, 1, 128);
        else if (f == "--inlet-vocal-law" && (v == "host" || v == "shared"))
            o.inlet_shared = v == "shared";
        else if (f == "--inlet-cross" && (v == "none" || v == "dendrites" || v == "hand"))
            o.inlet_cross = v == "none" ? 0u : v == "dendrites" ? 1u : 2u;
        else if (f == "--receive-loci" && (v == "all" || v == "dendrites" || v == "hand" || v == "cortex"))
            o.receive_loci = v == "all" ? 3u : v == "dendrites" ? 4u : v == "hand" ? 2u : 1u;
        else if (f == "--receive-architecture" && (v == "replace" || v == "branch"))
            o.receive_branch = v == "branch";
        else if (f == "--voice-route" && (v == "inherited" || v == "shared"))
            o.voice_route = v == "inherited" ? Word(drift::auditory) : 0u;
        else if (f == "--inlet-germ")
            o.inlet_germ = v;
        else if (f == "--drift-cargo" && (v == "loose" || v == "rafts"))
            o.drift_cargo = v == "rafts" ? Word(drift::rafting) : 0u;
        else if (f == "--drift-diary")
            o.drift_diary = v;
        else if (f == "--drift-questions")
            o.drift_questions = v;
        else if (f == "--drift-diary-ticks")
            o.drift_diary_ticks = number(v, 1, 65536);
        else if (f == "--drift-sight" && (v == "own" || v == "audience" || v == "off"))
            o.drift_sight = v == "off" ? 0u : Word(drift::sight) | (v == "audience" ? Word(drift::audience) : 0u);
        else if (f == "--watch-drift" && (v == "surprise" || v == "confirmed" || v == "untried" || v == "catchment"))
            o.drift_watch = v == "surprise" ? 1u : v == "confirmed" ? 2u : v == "untried" ? 3u : 4u;
        else if (f == "--drift-witness-sample" &&
                 (v == "all" || v == "calibrated" || v == "carriers" || v == "genotype"))
            o.drift_pupil = v == "all" ? 0u : v == "calibrated" ? 1u : v == "carriers" ? 2u : 3u;
        else if (f == "--drift-look")
            o.drift_look = number(v, 1, 1048576);
        else if (f == "--drift-surfaces")
            o.drift_surfaces = number(v, 0, 1048576);
        else if (f == "--drift-surface-seed")
            o.drift_surface_seed = number(v, 0, ~0u);
        else if (f == "--drift-legacy-trace")
            o.drift_legacy_trace = v;
        else if (f == "--drift-germ")
            o.drift_germ = v;
        else if (f == "--drift-repertoire")
            o.drift_repertoire = v;
        else if (f == "--drift-probe")
            o.drift_probe = number(v, 1, 1024);
        else if (f == "--drift-probe-trace")
            o.drift_probe_trace = v;
        else if (f == "--drift-food-germ")
            o.drift_food_germ = v;
        else if (f == "--drift-pool")
            o.drift_pool = v;
        else if (f == "--drift-rival")
            o.drift_rival = v;
        else if (f == "--drift-pool-order" && (v == "ab" || v == "ba"))
            o.drift_pool_order = v == "ba";
        else if (f == "--drift-transfer" &&
                 (v == "cortex" || v == "hand" || v == "both" || v == "lateral" || v == "all"))
            o.drift_loci = v == "all" ? 7u : v == "lateral" ? 4u : v == "both" ? 3u : v == "hand" ? 2u : 1u;
        else if (f == "--drift-stock")
            o.drift_stock = v;
        else if (f == "--drift-label")
            o.drift_label = number(v, 0, ~0u - 1);
        else if (f == "--drift-memory" &&
                 (v == "live" || v == "quiet" || v == "unread" || v == "fixed" || v == "naive")) {
            o.drift_memory = (v == "unread" || v == "naive" ? 0u : Word(drift::recall)) |
                             (v == "fixed" || v == "naive" ? 0u : Word(drift::plastic)) |
                             (v == "quiet" ? 0u : Word(drift::feedback));
        } else if (f == "--drift-works" && (v == "mutable" || v == "fixed"))
            o.drift_works = v == "mutable" ? Word(drift::building) : 0u;
        else if (f == "--drift-fibres" && (v == "conduct" || v == "bypass"))
            o.drift_fibres = v == "conduct" ? Word(drift::fibres) : 0u;
        else if (f == "--drift-tide" || f == "--drift-stride") {
            Word n = number(v, 1, f == "--drift-tide" ? 65536u : 8u);
            if (!std::has_single_bit(n))
                throw std::invalid_argument("flow timing needs a power of two");
            (f == "--drift-tide" ? o.drift_tide : o.drift_stride) = std::countr_zero(n);
        } else if (f == "--drift-load")
            o.drift_load = number(v, 1, 64);
        else if (f == "--drift-trace")
            o.drift_trace = v;
        else if (f == "--drift-window")
            o.drift_window = number(v, 1, 512);
        else if (f == "--drift-tiles")
            o.drift_tiles = number(v, 1, 1024);
        else if(f=="--feedback-donor")o.feedback_donor=v;
        else if(f=="--trace-donor")o.trace_donor=v;
        else if(f=="--receive-sample" && (v=="random"||v=="calibrated"))o.receive_named=v=="calibrated";
        else if(f=="--pupil-born-after")o.after_birth=number(v,0,~0u-1);
        else if(f=="--receive-count")o.receive_count=number(v,1,1048576);
        else if(f=="--receive-donor")o.receive_donor=v;
        else if(f=="--shore-school")o.shore_school=v;
        else if(f=="--shore-pupils")o.shore_pupils=number(v,1,65536);
        else if(f=="--shore-reverse-at")o.shore_reversal=number(v,1,~0u);
        else if(f=="--shore-trace")o.shore_trace=v;
        else if(f=="--shore-channel" && (v=="teaching"||v=="quiet"||v=="exam"))o.shore_channel=v=="exam"?shore::examination:v=="quiet"?shore::quiet:shore::teaching;
        else if(f=="--feedback-ports" && (v=="closed" || v=="open" || v=="quiet"))
            o.feedback_ports=(v=="closed"?0u:regulation::motor_port) | (v=="quiet"?regulation::silent_feedback:0u);
        else if(f=="--feedback-enrollment" && (v=="all"||v=="carriers"))o.survey_carriers=v=="carriers";
        else if(f=="--feedback-survey-trace")o.survey_trace=v;
        else if(f=="--feedback-survey")o.feedback_survey=number(v,1,65536);
        else if(f=="--feedback-dialects")o.feedback_dialects=number(v,1,128);
        else if(f=="--feedback-generations")o.feedback_generations=number(v,1,8192);
        else if(f=="--feedback-cohort"){o.feedback_cohort=number(v,32,32768);if(!std::has_single_bit(o.feedback_cohort))throw std::invalid_argument("the nursery needs a power-of-two cohort");}
        else if(f=="--feedback-loci" && (v=="module" || v=="all"))o.feedback_free=v=="all";
        else if(f=="--feeding-trace")o.feeding_trace=v;
        else if(f=="--feedback-class")o.apprentices=number(v,2,65536);
        else if(f=="--feedback-rig" && (v=="fixed"||v=="varied"))o.feedback_rigs=v=="varied";
        else if(f=="--feedback-motor" && (v=="native"||v=="cortical"))o.feedback_hand=v=="cortical";
        else if(f=="--receive-ports" && (v=="voice"||v=="hand"||v=="both"))o.donor_ports=v=="both"?3u:v=="hand"?2u:1u;
        else if(f=="--feedback-frames" && (v=="2"||v=="4"))o.feedback_frames=number(v,2,4);
        else if(f=="--feedback-convention" && (v=="stable"||v=="reversed"))o.feedback_reversal=v=="reversed";
        else if(f=="--feedback-trace")o.feedback_trace=v;
        else if(f=="--feedback-program"){
            o.feedback_first=number(v,0,~0u);
            if(++i==argc)throw std::invalid_argument("supply two regulatory module words");
            o.feedback_second=number(argv[i],0,~0u);o.regulatory_override=true;
        }
        else if (f == "--watch-readers")
            o.reader_interval = number(v, 1, 1048576);
        else if (f == "--field-class")
            o.field_class = number(v, 1, 1048576);
        else if (f == "--field-questions")
            o.field_questions = v;
        else if (f == "--field-retest")
            o.class_retests = number(v, 1, 1048576);
        else if (f == "--class-history") {
            o.class_prescribed = true;
            o.class_history = number(v, 0, ~0u);
        } else if (f == "--class-practice")
            o.class_practice = number(v, 0, 64);
        else if (f == "--class-seed")
            o.class_seed = number(v, 0, ~0u);
        else if (f == "--class-sample" && (v == "all" || v == "connected"))
            o.class_organs = v == "connected";
        else if (f == "--class-trace")
            o.class_trace = v;
        else if (f == "--return-lesson" || f=="--return-practice") {
            if (++i == argc)
                throw std::invalid_argument("--return-lesson needs the original and taught mind files");
            o.field_lessons.push_back({v,argv[i],f=="--return-practice"});
        } else if (f == "--follow-witness")
            o.follow_witness = v;
        else if (f == "--gate-seed")
            o.gate_seed = number(v, 0, ~0u);
        else if (f == "--gate-trace")
            o.gate_trace = v;
        else if (f == "--braids" && (v == "live" || v == "stationary" || v == "private" || v == "deaf" ||
                                     v == "fixed" || v == "isolated" || v == "silent"))
            o.braid_mode=v=="stationary"?0u:v=="private"?braid::live&~braid::watching:v=="deaf"?braid::live&~braid::hearing:
                v=="fixed"?braid::live&~braid::changing:v=="isolated"?braid::live&~braid::projecting:v=="silent"?braid::live&~braid::inventing:braid::live;
        else if(f=="--braid-eyes"&&(v=="shared"||v=="local"))o.braid_eyes=v=="local"?braid::individual_eyes:0u;
        else if(f=="--braid-sediment"&&(v=="mobile"||v=="pinned"||v=="bypass"))
            o.braid_sediment=v=="mobile"?braid::mineral_bed|braid::mobile_bed:v=="pinned"?braid::mineral_bed:0u;
        else if(f=="--braid-channels"&&(v=="off"||v=="live"||v=="water"||v=="deaf"||v=="fixed"||v=="shifted"))
            o.braid_channels=v=="off"?0u:braid::sluices|(v=="water"?braid::passive_gates:v=="deaf"?braid::hush:v=="fixed"?braid::frozen_gates:v=="shifted"?braid::shifted_gates:0u);
        else if(f=="--braid-perspectives")o.braid_perspectives=number(v,1,8192);
        else if(f=="--braid-views")o.braid_views=number(v,1,braid::view_period);
        else if(f=="--braid-view-budget")o.braid_view_budget=number(v,0,4);
        else if(f=="--braid-view-trace")o.braid_view_trace=v;
        else if(f=="--braid-inquiries")o.braid_inquiries=number(v,1,16384);
        else if(f=="--braid-holdout")o.braid_holdout=number(v,1,2);
        else if(f=="--braid-examples")o.braid_examples=number(v,1,256);
        else if(f=="--braid-report-trace")o.braid_report_trace=v;
        else if(f=="--braid-phase")o.braid_phase=number(v,0,3);
        else if(f=="--braid-mesh" && (v=="built"||v=="continuous"||v=="twisted"))o.braid_mesh=v=="continuous"?Word(braid::open_mesh):v=="twisted"?Word(braid::slipping):0u;
        else if(f=="--braid-epochs")o.braid_epochs=number(v,1,4096);
        else if(f=="--braid-parents")o.braid_parents=number(v,16,32768);
        else if(f=="--braid-seed")o.braid_seed=number(v,0,~0u);
        else if(f=="--braid-selection" && (v=="food"||v=="uniform"))o.braid_paid=v=="food";
        else if(f=="--braid-cases")o.braid_cases=number(v,1,32768);
        else if(f=="--braid-lessons")o.braid_rep=number(v,0,32);
        else if(f=="--braid-words")o.braid_terms=number(v,2,8);
        else if(f=="--braid-length")o.braid_length=number(v,1,8);
        else if(f=="--braid-convention" && (v=="forward"||v=="varied"||v=="commuting"))o.braid_acquire=v=="forward"?0:v=="varied"?1:2;
        else if(f=="--braid-rig" && (v=="simple"||v=="coupled"))o.braid_chiral=v=="coupled";
        else if(f=="--braid-stock")o.braid_source=v;
        else if(f=="--braid-speaker")o.braid_voice=v;
        else if(f=="--braid-nursery"&&(v=="workshop"||v=="lineage"||v=="filters"))o.braid_school=v=="workshop"?0:v=="lineage"?1:2;
        else if(f=="--braid-villages")o.braid_villages=number(v,1,4096);
        else if(f=="--braid-generations")o.braid_lives=number(v,1,64);
        else if(f=="--braid-visits")o.braid_visits=number(v,1,256);
        else if(f=="--braid-chorister")o.braid_chorister=v;
        else if(f=="--braid-village")o.braid_village=number(v,0,~0u);
        else if(f=="--braid-history")o.braid_history=number(v,0,1);
        else if(f=="--braid-songbook")o.braid_song=v;
        else if(f=="--braid-dialogues")o.braid_dialogues=number(v,1,16384);
        else if(f=="--braid-questions")o.braid_questions=number(v,0,4);
        else if(f=="--braid-panel"&&(v=="plain"||v=="striped"||v=="distinct"))o.braid_pan=v=="plain"?0:v=="striped"?1:2;
        else if(f=="--braid-workers")o.braid_workers=v;
        else if(f=="--braid-transcript")o.braid_transcript=v;
        else if(f=="--braid-trace")o.braid_trace=v;
        else if(f=="--braid-bench")o.braid_bench=number(v,1,16777216);
        else if(f=="--rattles" && (v=="live" || v=="deaf" || v=="fixed" || v=="passive" || v=="scrambled" || v=="rest"))
            o.rattle_mode=v=="rest"?0u:v=="deaf"?rattle::live&~rattle::hearing:v=="fixed"?rattle::live&~rattle::learning:
                v=="passive"?rattle::live&~rattle::voices:v=="scrambled"?rattle::live|rattle::jitter:rattle::live;
        else if(f=="--rattle-listen" && (v=="both"||v=="one"))o.rattle_sparse=v=="one"?Word(rattle::single):0;
        else if(f=="--rattle-nursery" && (v=="both"||v=="one"||v=="chorus"||v=="solo-chorus"))o.rattle_nursery=v=="solo-chorus"?3:v=="chorus"?2:v=="one";
        else if(f=="--rattle-speaker")o.rattle_speaker=v;
        else if(f=="--rattle-carrier")o.rattle_carrier=v;
        else if(f=="--rattle-lesion" && (v=="none"||v=="eligibility"||v=="precision"))o.rattle_lesion=v=="eligibility"?1:v=="precision"?2:0;
        else if(f=="--rattle-load" && (v=="loose"||v=="silted"))o.rattle_load=v=="silted"?Word(rattle::loaded):0;
        else if(f=="--rattle-binding" && (v=="local"||v=="crossed"))o.rattle_binding=v=="crossed"?Word(rattle::crossed):0;
        else if(f=="--rattle-phase")o.rattle_phase=number(v,0,3);
        else if(f=="--rattle-seed")o.rattle_seed=number(v,0,~0u);
        else if(f=="--rattle-bench")o.rattle_calls=number(v,1,16777216);
        else if(f=="--rattle-trials")o.rattle_trials=number(v,1,65536);
        else if(f=="--rattle-broods")o.rattle_cycles=number(v,1,2048);
        else if(f=="--rattle-parents")o.rattle_pop=number(v,16,65536);
        else if(f=="--rattle-exposure")o.rattle_exposure=number(v,0,512);
        else if(f=="--rattle-visitors")o.rattle_visitors=v;
        else if(f=="--rattle-stock")o.rattle_stock=v;
        else if(f=="--rattle-trace")o.rattle_trace=v;
        else if(f=="--rattle-selection" && (v=="food"||v=="uniform"))o.rattle_paid=v=="food";
        else if(f=="--reeds"&&(v=="live"||v=="private"||v=="blind"||v=="rest"||v=="scrambled"))o.reed_mode=v=="rest"?0u:runnel::forming|(v=="private"?0u:Word(runnel::watching))|(v=="blind"?0u:Word(runnel::sensing))|(v=="scrambled"?Word(runnel::scrambled):0u);
        else if(f=="--reed-tide"&&(v=="breathing"||v=="quiet"))o.reed_weather=v=="breathing"?Word(runnel::tide):0u;
        else if(f=="--reed-current"&&(v=="free"||v=="shaped"||v=="reversed"))o.reed_flex=v=="free"?Word(runnel::decoupled):v=="reversed"?Word(runnel::reversed):0u;
        else if(f=="--reed-contest"&&(v=="listen"||v=="reciprocal"||v=="sham"||v=="reason"||v=="reverse-reason"||v=="off"))o.reed_contest=v=="off"?0u:Word(runnel::families)|(v=="reciprocal"||v=="reverse-reason"?Word(runnel::swap_families):0u)|(v=="sham"?Word(runnel::equal_families):0u)|(v=="reason"||v=="reverse-reason"?Word(runnel::compete_reason):0u);
        else if(f=="--reed-patrol"&&(v=="free"||v=="learned"))o.reed_patrol=v=="learned"?Word(runnel::patrolling):0u;
        else if(f=="--reed-keepers")o.reed_keep=number(v,1,65536);
        else if(f=="--reed-life")o.reed_life=number(v,1,192);
        else if(f=="--reed-cut"&&(v=="observed"||v=="matched"||v=="sham"))o.reed_cut=v=="observed"?1u:v=="matched"?0u:2u;
        else if(f=="--reed-strata")o.reed_strata=number(v,1,~0u);
        else if(f=="--reed-grain"&&(v=="handed"||v=="straight"))o.reed_grain=v=="handed"?Word(runnel::handed):0u;
        else if(f=="--reed-feeding"&&(v=="sip"||v=="leave"))o.reed_diet=v=="sip"?Word(runnel::responding):0u;
        else if(f=="--reed-reason"&&(v=="direct"||v=="relational"))o.reed_relation=v=="relational"?Word(runnel::relating):0u;
        else if(f=="--reed-nursery"&&(v=="transfer"||v=="direct"||v=="mixed"||v=="varied"||v=="keepers"))o.reed_school=v=="direct"?0u:v=="transfer"?1u:v=="mixed"?2u:v=="varied"?3u:4u;
        else if(f=="--reed-trace")o.reed_trace=v;
        else if(f=="--reed-analogy")o.reed_general=number(v,1,65536);
        else if(f=="--reed-examples")o.reed_examples=number(v,0,12);
        else if(f=="--reed-observe")o.reed_watch=number(v,0,96);
        else if(f=="--reed-attention"&&(v=="offered"||v=="invited"))o.reed_invited=v=="invited";
        else if(f=="--reed-seed")o.reed_seed=number(v,1,~0u);
        else if(f=="--reed-period")o.fold_period=number(v,1,256);
        else if(f=="--reed-rear")o.reed_rear=number(v,1,4096);
        else if(f=="--reed-population"){o.reed_adults=number(v,16,65536);if(!std::has_single_bit(o.reed_adults))throw std::invalid_argument("reed population must be a power of two");}
        else if(f=="--reed-brood"&&(v=="earned"||v=="uniform"))o.reed_paid=v=="earned";
        else if(f=="--reed-study")o.reed_cases=number(v,1,65536);
        else if(f=="--reed-span")o.reed_span=number(v,2,runnel::max_span);
        else if(f=="--reed-chain")o.reed_turns=number(v,0,64);
        else if(f=="--reed-practice")o.reed_play=number(v,0,128);
        else if(f=="--reed-rig")o.reed_rig=number(v,2,runnel::max_span);
        else if(f=="--reed-bench")o.reed_calls=number(v,1,1u<<26);
        else if(f=="--reed-germs")o.reed_stock=v;
        else if(f=="--reed-scholar")o.reed_scholar=v;
        else if(f=="--reed-carrier")o.reed_carrier=v;
        else if(f=="--wicks" && (v=="flowing"||v=="confined"||v=="still"))o.wick_mode=v=="still"?0u:v=="confined"?Word(wicks::flowing):Word(wicks::flowing|wicks::across);
        else if(f=="--wick-seed")o.wick_seed=number(v,1,~0u);
        else if(f=="--wick-study")o.wick_length=number(v,4,128);
        else if(f=="--practice"&&(v=="live"||v=="scattered"||v=="persistent"||v=="still"))o.practice_mode=v=="still"?0u:Word(practice::engaging)|(v=="scattered"?0u:Word(practice::holding))|(v=="persistent"?Word(practice::constant_stay):0u);
        else if(f=="--practice-probes"&&(v=="chosen"||v=="random"||v=="quiet"))o.practice_approach=v=="quiet"?0u:Word(practice::probing)|(v=="random"?Word(practice::random_probes):0u);
        else if(f=="--practice-appetite"&&(v=="responsive"||v=="inherited"||v=="shifted"))o.practice_focus=v=="inherited"?0u:Word(practice::responsive_attention)|(v=="shifted"?Word(practice::misassigned_attention):0u);
        else if(f=="--practice-landscape"&&(v=="frontier"||v=="island"))o.practice_island=v=="island";
        else if(f=="--practice-habitat"&&(v=="fixtures"||v=="foraging"||v=="mixed"))o.practice_teaching=v=="fixtures"?0u:v=="foraging"?1u:2u;
        else if(f=="--practice-context"&&(v=="multiple"||v=="one"))o.practice_credit=v=="multiple"?Word(practice::contextual):0u;
        else if(f=="--practice-sight"&&(v=="private"||v=="shared"||v=="displaced"))o.practice_sight=v=="private"?0u:Word(practice::social)|(v=="displaced"?Word(practice::wrong_effect):0u);
        else if(f=="--practice-work"&&(v=="learned"||v=="chance"))o.practice_join=v=="learned"?Word(practice::crafting):0u;
        else if(f=="--practice-fibre"&&(v=="gather"||v=="carried"))o.practice_resource=v=="gather"?Word(practice::collecting):0u;
        else if(f=="--practice-contest"&&(v=="off"||v=="paired"||v=="reciprocal"||v=="sham"))o.practice_comp=v=="off"?0u:Word(practice::families)|(v=="sham"?Word(practice::sham):v=="reciprocal"?Word(practice::reciprocal):0u);
        else if(f=="--practice-banks")o.practice_cap=number(v,1,4);
        else if(f=="--practice-seed")o.practice_seed=number(v,1,~0u);
        else if(f=="--practice-rear")o.practice_epochs=number(v,1,8192);
        else if(f=="--practice-population"){o.practice_adults=number(v,16,65536);if(!std::has_single_bit(o.practice_adults))throw std::invalid_argument("practice population must be a power of two");}
        else if(f=="--practice-brood"&&(v=="earned"||v=="uniform"))o.practice_paid=v=="earned";
        else if(f=="--practice-germs")o.practice_stock=v;
        else if(f=="--practice-scholar")o.practice_scholar=v;
        else if(f=="--practice-carrier")o.practice_carrier=v;
        else if(f=="--practice-study")o.practice_test=number(v,1,65536);
        else if(f=="--practice-bench")o.practice_speed=number(v,1,1u<<26);
        else if(f=="--linkage" && (v=="live"||v=="unstudied"||v=="unplanned"||v=="inert"||v=="scrambled"))
            o.link_mode=(v=="inert"?0u:Word(linkage::working))|((v=="unstudied"||v=="inert")?0u:Word(linkage::writable))|
                ((v=="unplanned"||v=="inert")?0u:Word(linkage::planning))|(v=="scrambled"?Word(linkage::scrambled):0u);
        else if(f=="--linkage-weather" && (v=="steady"||v=="wet"))o.link_water=v=="wet"?Word(linkage::hydration):0u;
        else if(f=="--linkage-climate" && (v=="separate"||v=="constant"||v=="changing"))o.link_climate=v=="constant"?1u:v=="changing"?2u:0u;
        else if(f=="--linkage-recall")o.link_recall=number(v,1,65536);
        else if(f=="--linkage-attention" && (v=="focal"||v=="whole"))o.link_focus=v=="focal"?Word(linkage::focal):0u;
        else if(f=="--linkage-building" && (v=="inferred"||v=="exploratory"||v=="fixed"))o.link_fit=v=="fixed"?0u:Word(linkage::fitting)|(v=="inferred"?Word(linkage::designing):0u);
        else if(f=="--linkage-sight" && (v=="public"||v=="private"||v=="shifted"))
            o.link_soc=v=="private"?0u:Word(linkage::witnesses)|(v=="shifted"?Word(linkage::other_eye):0u);
        else if(f=="--linkage-joints" && (v=="linked"||v=="loose"))o.link_joints=v=="linked"?Word(linkage::coupled):0u;
        else if(f=="--linkage-flow" && (v=="open"||v=="sealed"))o.link_use=v=="open"?Word(linkage::current):0u;
        else if(f=="--linkage-contest" && (v=="off"||v=="paired"||v=="reciprocal"||v=="sham"))o.link_comp=v=="off"?0u:Word(linkage::mark_families)|
            (v=="reciprocal"?Word(linkage::swap_families):v=="sham"?Word(linkage::equal_families):0u);
        else if(f=="--linkage-contexts"){o.link_arena=number(v,1,4);if(!std::has_single_bit(o.link_arena))throw std::invalid_argument("linkage contexts take 1, 2 or 4 slots");}
        else if(f=="--linkage-period"){o.link_period=number(v,1,64);if(!std::has_single_bit(o.link_period))throw std::invalid_argument("linkage period must be a power of two");}
        else if(f=="--linkage-seed")o.link_seed=number(v,1,~0u);
        else if(f=="--linkage-rear")o.link_epochs=number(v,1,4096);
        else if(f=="--linkage-population"){o.link_adults=number(v,16,65536);if(!std::has_single_bit(o.link_adults))throw std::invalid_argument("linkage population must be a power of two");}
        else if(f=="--linkage-brood"&&(v=="earned"||v=="uniform"))o.link_paid=v=="earned";
        else if(f=="--linkage-study")o.link_test=number(v,1,65536);
        else if(f=="--linkage-pieces")o.link_width=number(v,1,4);
        else if(f=="--linkage-practice")o.link_practice=number(v,1,8);
        else if(f=="--linkage-bench")o.link_bench=number(v,1,1u<<26);
        else if(f=="--linkage-witnesses")o.link_witness=number(v,1,65536);
        else if(f=="--linkage-germs")o.link_origin=v;
        else if(f=="--linkage-law")o.link_law=v;
        else if(f=="--linkage-scholar")o.link_scholar=v;
        else if(f=="--linkage-carrier")o.link_carrier=v;
        else if(f=="--effort" && (v=="live"||v=="always"||v=="fixed"||v=="reversed"||v=="shuffled"||v=="no-taste"||v=="no-strain"))
            o.effort_mode=(v=="fixed"?0u:effort::learning)|(v=="always"?0u:effort::deciding)|
                (v=="reversed"?effort::reversing:v=="shuffled"?effort::scrambled:v=="no-taste"?effort::blinded_food:v=="no-strain"?effort::blinded_work:0u);
        else if(f=="--effort-movement" && (v=="select"||v=="impulse"))o.effort_spending=v=="select"?effort::selective:0u;
        else if(f=="--effort-assignments" && (v=="same"||v=="resample"))o.effort_sample_rule=v=="same"?effort::fixed_survey:0u;
        else if(f=="--effort-paving" && (v=="forecast"||v=="habit"||v=="foreign"))
            o.effort_paving=v=="habit"?0u:effort::appraisal|(v=="foreign"?effort::foreign_surface:0u);
        else if(f=="--effort-sculpt" && (v=="live"||v=="footsteps"))o.effort_sculpt=v=="live"?effort::tamping:0u;
        else if(f=="--effort-window")o.effort_window=number(v,1,64);
        else if(f=="--effort-trails" && (v=="soft"||v=="rigid"))o.effort_trails=v=="soft"?effort::trails:0u;
        else if(f=="--effort-aim" && (v=="multiple"||v=="single"))o.effort_span=v=="multiple"?effort::more_places:0u;
        else if(f=="--effort-audit" && (v=="randomized"||v=="quiet"))o.effort_audit=v=="randomized"?effort::surveying:0u;
        else if(f=="--effort-contest" && (v=="paired"||v=="reciprocal"||v=="sham"||v=="off"))
            o.effort_comp=v=="off"?0u:effort::comp|(v=="reciprocal"?effort::reverse_comp:v=="sham"?effort::same_comp:0u);
        else if(f=="--effort-weather")o.effort_period=number(v,0,~0u-1);
        else if(f=="--effort-rear")o.effort_seasons=number(v,1,8192);
        else if(f=="--effort-population"){o.effort_population=number(v,16,65536);if(!std::has_single_bit(o.effort_population)) throw std::invalid_argument("effort population must be a power of two");}
        else if(f=="--effort-practice" && (v=="trips"||v=="mixed"||v=="building"))o.effort_practice=v=="building"?2u:v=="mixed"?1u:0u;
        else if(f=="--effort-seed")o.effort_seed=number(v,1,~0u);
        else if(f=="--effort-study")o.effort_test=number(v,1,131072);
        else if(f=="--effort-bench")o.effort_calls=number(v,1,1u<<26);
        else if(f=="--effort-brood" && (v=="earned"||v=="uniform"))o.effort_paid=v=="earned";
        else if(f=="--effort-germs")o.effort_germs=v;
        else if(f=="--effort-scholar")o.effort_scholar=v;
        else if(f=="--effort-carrier")o.effort_carrier=v;
        else if (f=="--hollows" && (v=="live"||v=="unguided"||v=="inert"||v=="still"))
            o.hollow_mode=v=="still"?0u:v=="unguided"?Word(hollow::digging|hollow::changing):v=="inert"?Word(hollow::digging|hollow::directing):Word(hollow::digging|hollow::changing|hollow::directing);
        else if (f=="--hollow-odometry" && (v=="actual"||v=="rotated"||v=="quiet"))
            o.hollow_odometry=v=="quiet"?0u:Word(hollow::dead_reckoning)|(v=="rotated"?Word(hollow::distorted):0u);
        else if (f=="--hollow-landmarks" && (v=="visible"||v=="covered")) o.hollow_anchor=v=="visible"?Word(hollow::anchoring):0u;
        else if (f=="--hollow-ground" && (v=="sparse"||v=="rich")) o.hollow_rich=v=="rich"?Word(hollow::rich_beds):0u;
        else if (f=="--hollow-contest" && (v=="paired"||v=="reciprocal"||v=="sham"||v=="crossed"||v=="reciprocal-crossed"||v=="off"))
            o.hollow_contest=v=="off"?0u:hollow::competition|(v=="sham"?hollow::equal_access:0u)|
                ((v=="reciprocal"||v=="reciprocal-crossed")?hollow::reverse_access:0u)|
                ((v=="crossed"||v=="reciprocal-crossed")?hollow::crossed_motor:0u);
        else if (f=="--hollow-current" && (v=="sheltered"||v=="ferry")) o.hollow_current=v=="ferry"?Word(hollow::drift):0u;
        else if (f=="--hollow-slots") o.hollow_slots=number(v,1,hollow::limit);
        else if (f=="--hollow-study") o.hollow_test=number(v,1,65536);
        else if (f=="--hollow-rear") o.hollow_seasons=number(v,1,8192);
        else if (f=="--hollow-population") {o.hollow_population=number(v,16,65536);if(!std::has_single_bit(o.hollow_population)) throw std::invalid_argument("hollow population must be a power of two");}
        else if (f=="--hollow-brood" && (v=="earned"||v=="uniform")) o.hollow_paid=v=="earned";
        else if (f=="--hollow-seed") o.hollow_seed=number(v,1,~0u);
        else if (f=="--hollow-bench") o.hollow_calls=number(v,1,1u<<26);
        else if (f=="--hollow-carrier") o.hollow_carrier=v;
        else if (f=="--hollow-optics") o.hollow_optics=v;
        else if (f=="--hollow-founders" && (v=="residents"||v=="varied")) o.hollow_varied=v=="varied";
        else if (f=="--hollow-scholar") o.hollow_scholar=v;
        else if (f=="--hollow-germs") o.hollow_origin=v;
        else if (f == "--pottery" && (v == "fired" || v == "sheltered" || v == "weathered"))
            o.ceramic = v == "fired" ? 0 : v == "sheltered" ? 1 : 2;
        else if (f == "--clay-reach" && (v == "single" || v == "rake" || v == "shared"))
            o.clay_reach = v == "single" ? 0 : v == "rake" ? 1 : 2;
        else if (f == "--clay-hands" && (v == "both" || v == "read" || v == "write" || v == "rest"))
            o.clay_hands = v == "both" ? 3 : v == "read" ? 2 : v == "write" ? 1 : 0;
        else if (f == "--clay-muscle" && (v == "live" || v == "quiet"))
            o.clay_muscle = v == "live";
        else if (f == "--clay-sound" &&
                 (v == "clear" || v == "permuted" || v == "ancestral" || v == "ancestral-permuted"))
            o.clay_hearing = v == "clear" ? 0 : v == "permuted" ? 32 : v == "ancestral" ? 64 : 96;
        else if (f == "--clay-context" && (v == "full" || v == "visible" || v == "hidden" || v == "pooled"))
            o.clay_context = v == "full" ? 24 : v == "visible" ? 8 : v == "hidden" ? 16 : 0;
        else if (f == "--press") {
            if (v == "live")
                o.press_use = press::full;
            else if (v == "still")
                o.press_use = press::installed;
            else if (v == "no-casting")
                o.press_use = press::full & ~press::casting;
            else if (v == "alone")
                o.press_use = press::full & ~press::attending;
            else if (v == "no-taste")
                o.press_use = press::full & ~press::tasting;
            else if (v == "unguided")
                o.press_use = press::full & ~press::choosing;
            else
                throw std::invalid_argument("unknown press coupling: " + v);
        } else if (f == "--press-geology" && (v == "mixed" || v == "veins" || v == "tidal"))
            o.press_strata = v == "mixed" ? 0u : v == "veins" ? press::bedded : press::bedded | press::tidal;
        else if (f == "--press-feel" && (v == "quiet" || v == "live" || v == "displaced" || v == "stiff"))
            o.press_feel = press::feel_set | (v == "quiet" ? 0u : Word(press::feeling)) |
                           (v == "displaced" ? Word(press::dislocated)
                            : v == "stiff"   ? Word(press::stiff)
                                             : 0u);
        else if (f == "--press-study")
            o.press_test = number(v, 1, 65536);
        else if (f == "--press-seasons")
            o.press_seasons = number(v, 1, 8192);
        else if (f == "--press-population") {
            o.press_population = number(v, 16, 65536);
            if (o.press_population & (o.press_population - 1))
                throw std::invalid_argument("press population must be a power of two");
        } else if (f == "--press-seed")
            o.press_seed = number(v, 1, ~0u);
        else if (f == "--press-bench")
            o.press_bench = number(v, 1, 1u << 27);
        else if (f == "--press-brood" && (v == "paid" || v == "uniform" || v == "finite"))
            o.press_paid = v == "finite" ? 2 : v == "paid";
        else if (f == "--press-practice" && (v == "observed" || v == "mixed"))
            o.press_mixed = v == "mixed";
        else if (f == "--press-scholar")
            o.press_scholar = v;
        else if (f == "--press-founders" && (v == "varied" || v == "residents"))
            o.press_varied = v == "varied";
        else if (f == "--clay-seed")
            o.clay_breed_seed = number(v, 1, ~0u);
        else if (f == "--clay-rear")
            o.clay_seasons = number(v, 1, 8192);
        else if (f == "--clay-test")
            o.clay_test = number(v, 1, 1024);
        else if (f == "--clay-classes")
            o.clay_classes = number(v, 1, 65536);
        else if (f == "--clay-preparation")
            o.clay_prep = number(v, 0, 8192);
        else if (f == "--clay-history")
            o.clay_history = number(v, 1, 8192);
        else if (f == "--clay-lessons")
            o.clay_lessons = number(v, 0, 4096);
        else if (f == "--clay-taps")
            o.clay_taps = number(v, 0, 8);
        else if (f == "--clay-selection" && (v == "provisioned" || v == "uniform"))
            o.clay_select = v == "provisioned";
        else if (f == "--clay-teachers" && (v == "parallel" || v == "unrelated"))
            o.clay_strangers = v == "unrelated";
        else if (f == "--clay-germs")
            o.clay_origin = v;
        else if (f == "--clay-specimen")
            o.clay_specimen = v;
        else if (f == "--clay-settlers")
            o.clay_settlers = v;
        else if (f == "--clay-peers")
            o.clay_peers = v;
        else if (f == "--export-clay")
            o.clay_export = v;
        else if (f == "--clay-site")
            o.clay_site = number(v, 0, ~0u);
        else if (f == "--clay-dig")
            o.clay_dig = number(v, 1, 1024);
        else if (f == "--clay-reversals")
            o.clay_switches = number(v, 8, 65536);
        else if (f == "--clay-bench")
            o.clay_calls = number(v, 1, 8388608);
        else if (f == "--clay-foster")
            o.clay_foster = number(v, 1, 16);
        else if (f == "--siphon-census")
            o.reed_census = number(v, 1, 256);
        else if (f == "--siphon-bench")
            o.reed_bench = number(v, 1, 8388608);
        else if(f=="--siphon-reference")o.reed_reference=v;
        else if (f == "--siphon-legacy")
            o.reed_legacy = number(v, 1, 65536);
        else if (f == "--siphon-resonance" && (v == "brief" || v == "material"))
            o.chorus_hold = v == "material";
        else if (f == "--siphon-workers" && (v == "live" || v == "still"))
            o.reed_work = v == "live";
        else if (f == "--siphon-mentors" && (v == "plastic" || v == "fixed"))
            o.reed_tutors = v == "fixed";
        else if (f == "--siphon-germs")
            o.phonetic_germ = v;
        else if (f == "--siphon-audience" && (v == "open" || v == "alone" || v == "shifted" || v == "scrambled" ||
                                              v == "ancestral" || v == "ancestral-scrambled"))
            o.chorus = v == "open"        ? 1
                       : v == "alone"     ? 0
                       : v == "shifted"   ? 2
                       : v == "scrambled" ? 3
                       : v == "ancestral" ? 4
                                          : 5;
        else if (f == "--siphon-foster")
            o.reed_foster_rounds = number(v, 1, 16);
        else if (f == "--siphon-exposures")
            o.reed_visit_school = number(v, 0, 8192);
        else if (f == "--siphon-view" && (v == "covered" || v == "intermittent" || v == "open"))
            o.reed_view = v == "covered" ? 0 : v == "open" ? 3 : 1;
        else if (f == "--siphon-medium" && (v == "air" || v == "resonant"))
            o.reed_medium = v == "resonant";
        else if (f == "--siphon-development" && (v == "graded" || v == "arbitrary"))
            o.reed_growth = v == "graded";
        else if (f == "--siphon-clutch" && (v == "mixed" || v == "siblings"))
            o.reed_clutch = v == "siblings";
        else if (f == "--siphons") {
            o.reed_bands = number(v, 1, 4);
            if (!std::has_single_bit(o.reed_bands))
                throw std::invalid_argument("use one, two or four receptive bands");
        } else if (f == "--siphon-wire" && (v == "open" || v == "shut" || v == "scrambled" || v == "shifted"))
            o.reed_wire = v == "open" ? 1 : v == "shut" ? 0 : v == "scrambled" ? 2 : 3;
        else if (f == "--siphon-windows" && (v == "tidal" || v == "dark" || v == "clear"))
            o.reed_windows = v == "tidal" ? 1 : v == "dark" ? 0 : 2;
        else if (f == "--siphon-period") {
            o.reed_period = number(v, 1, 65536);
            if (!std::has_single_bit(o.reed_period))
                throw std::invalid_argument("siphon spacing must be a power of two");
        } else if (f == "--siphon-trials")
            o.reed_trials = number(v, 1, 1024);
        else if (f == "--siphon-practice")
            o.reed_practice = number(v, 0, 65536);
        else if (f == "--siphon-rear")
            o.reed_seasons = number(v, 1, 16384);
        else if (f == "--siphon-visits")
            o.reed_visits = number(v, 1, 4096);
        else if (f == "--siphon-selection" && (v == "provisioned" || v == "fostered"))
            o.reed_foster = v == "fostered";
        else if (f == "--siphon-origin" && (v == "random" || v == "resident"))
            o.reed_origin = v == "resident";
        else if (f == "--brine-plan" && (v == "empirical" || v == "latent"))
            o.brine_plan = v == "latent";
        else if (f == "--brine-sense" && (v == "open" || v == "muted"))
            o.brine_senses = v == "open";
        else if (f == "--brine-wiring" && (v == "arbors" || v == "mosaic"))
            o.brine_arbors = v == "arbors";
        else if (f == "--brine-shore" &&
                 (v == "linger" || v == "wash" || v == "survey" || v == "consult" || v == "strand" || v == "scout"))
            o.brine_shore = v == "linger"    ? 0
                            : v == "wash"    ? 1
                            : v == "survey"  ? 2
                            : v == "consult" ? 3
                            : v == "strand"  ? 4
                                             : 5;
        else if (f == "--brine-bench")
            o.brine_calls = number(v, 1, 16777216);
        else if (f == "--brine")
            o.brine_species = number(v, 2, 5);
        else if (f == "--brine-flow" && (v == "still" || v == "current"))
            o.brine_flow = v == "current";
        else if (f == "--brine-rear")
            o.brine_seasons = number(v, 1, 32768);
        else if (f == "--brine-assay")
            o.brine_trials = number(v, 1, 4096);
        else if (f == "--brine-test-solutes")
            o.brine_trial_species = number(v, 2, 5);
        else if (f == "--brine-examples")
            o.brine_examples = number(v, 0, 12);
        else if (f == "--brine-tides")
            o.brine_tides = number(v, 1, 256);
        else if (f == "--brine-selection" && (v == "provisioned" || v == "fostered"))
            o.brine_drift = v == "fostered";
        else if (f == "--brine-origin" && (v == "random" || v == "resident"))
            o.brine_origin = v == "resident";
        else if (f == "--worksite-bench")
            o.worksite_calls = number(v, 1, 4194304);
        else if (f == "--substrate" && (v == "bound" || v == "mixed" || v == "scattered" || v == "loose"))
            o.substrate = v == "bound" ? 1u : v == "mixed" ? 2u : v == "scattered" ? 3u : 0u;
        else if (f == "--press-choice" && (v == "learned" || v == "unbiased"))
            o.ore_guidance = v == "learned";
        else if (f == "--worksites" && (v == "active" || v == "quiet"))
            o.worksite_mode = v == "active";
        else if (f == "--work-traces" &&
                 (v == "visible" || v == "hidden" || v == "scrambled" || v == "ancestral" || v == "ancestral-shuffled"))
            o.site_marks = v == "visible"              ? 1u
                           : v == "scrambled"          ? 2u
                           : v == "ancestral"          ? 3u
                           : v == "ancestral-shuffled" ? 4u
                                                       : 0u;
        else if (f == "--field-min-age")
            o.field_min_age = number(v, 0, ~0u);
        else if (f == "--field-max-age")
            o.field_max_age = number(v, 0, ~0u);
        else if (f == "--field-transfer")
            o.field_trials = number(v, 1, 256);
        else if (f == "--light-regime" && (v == "diffuse" || v == "veined" || v == "mosaic"))
            o.climate = v == "mosaic" ? 2u : Word(v == "veined");
        else if (f == "--field-sight" && (v == "own" || v == "nearby"))
            o.tide_sight = v == "nearby";
        else if (f == "--shoal-origin" && (v == "juvenile" || v == "resident"))
            o.resident_test = v == "resident";
        else if (f == "--shoal-bench")
            o.shoal_calls = number(v, 1, 4194304);
        else if (f == "--construction-tide")
            o.tide_period = number(v, 1, 65536);
        else if (f == "--site-replay" && (v == "on" || v == "off"))
            o.tide_active = v == "on";
        else if (f == "--shoal-trials")
            o.shoal_trials = number(v, 1, 256);
        else if (f == "--shoal-seasons")
            o.shoal_seasons = number(v, 1, 8192);
        else if (f == "--shoal-moves")
            o.shoal.moves = number(v, 1, 32);
        else if (f == "--shoal-span")
            o.shoal.span = number(v, 3, 5);
        else if (f == "--shoal-practice")
            o.shoal.practice = number(v, 0, 2048);
        else if (f == "--shoal-sample")
            o.shoal_sample = number(v, 0, 0xfffffffeu);
        else if (f == "--shoal-exposure")
            o.shoal.exposure = number(v, 8, 512);
        else if (f == "--shoal-ideas") {
            o.shoal.ideas = number(v, 32, 512);
            if (o.shoal.ideas % 32)
                throw std::invalid_argument("candidate batches use complete warps of 32");
        } else if (f == "--artifact-samples")
            o.artifact_samples = number(v, 1, 128);
        else if (f == "--mineral-flow" && (v == "coupled" || v == "permeable"))
            o.mineral_water = v == "coupled";
        else if (f == "--hydraulic-trial")
            o.hydraulic_trial = number(v, 1, 65536);
        else if (f == "--guild-generations")
            o.guild_generations = number(v, 1, 16);
        else if (f == "--guild-practice")
            o.guild_practice = number(v, 0, 1024);
        else if (f == "--work-bench")
            o.hand_calls = number(v, 1, 67108864);
        else if (f == "--hand-germ")
            o.hand_germ = v;
        else if (f == "--quarry-seasons")
            o.quarry_seasons = number(v, 1, 8192);
        else if (f == "--quarry-moves")
            o.quarry_moves = number(v, 1, 8192);
        else if (f == "--quarry-exposure")
            o.quarry_ticks = number(v, 1, 8192);
        else if (f == "--work-seasons")
            o.work_seasons = number(v, 0, 8192);
        else if (f == "--work-days")
            o.work_days = number(v, 1, 2048);
        else if (f == "--work-span")
            o.work_span = number(v, 1, 4);
        else if (f == "--work-capacity") {
            o.work_capacity = number(v, 32, 262144);
            if (!std::has_single_bit(o.work_capacity))
                throw std::invalid_argument("workshop capacity must be a power of two");
        } else if (f == "--work-origin" && (v == "random" || v == "reference"))
            o.work_origin = v == "reference";
        else if (f == "--work-recruit" && (v == "provisioned" || v == "fostered"))
            o.work_foster = v == "fostered";
        else if (f == "--work-probes")
            o.work_probes = number(v, 1, 1024);
        else if (f == "--work-subjects")
            o.work_subjects = number(v, 1, 65536);
        else if (f == "--work-shown")
            o.work_shown = number(v, 0, 8192);
        else if (f == "--work-test-seed")
            o.work_test_seed = number(v, 0, ~0u);
        else if (f == "--materials" && (v == "reactive" || v == "passive" || v == "off"))
            o.minerals = v == "off" ? 2 : Word(v == "reactive");
        else if (f == "--grains")
            o.material_density = number(v, 0, 100);
        else if (f == "--bonds" && (v == "open" || v == "insulated"))
            o.material_bonds = v == "open";
        else if (f == "--hands" && (v == "open" || v == "still"))
            o.material_agency = v == "open";
        else if (f == "--work-sight" && (v == "open" || v == "shuttered" || v == "scrambled"))
            o.material_vision = v == "open" ? 1 : v == "scrambled" ? 2 : 0;
        else if (f == "--material-legacy")
            o.material_legacy = number(v, 1, 65536);
        else if (f == "--silence-new-cohort" && (v == "a" || v == "b" || v == "none"))
            o.silent_new = v == "a" ? 1 : v == "b" ? 2 : 0;
        else if (f == "--fixed-size-cohort" && (v == "a" || v == "b" || v == "none"))
            o.fixed_size = v == "a" ? 1 : v == "b" ? 2 : 0;
        else if (f == "--neurogenesis" && (v == "inherited" || v == "fixed"))
            o.neurogenesis = v == "inherited";
        else if (f == "--pods" && (v == "sealed" || v == "mixed"))
            o.pod_mode = v == "mixed";
        else if (f == "--single-frame-cohort" && (v == "a" || v == "b" || v == "none"))
            o.single_cohort = v == "a" ? 1 : v == "b" ? 2 : 0;
        else if (f == "--pod-pulse" && (v == "enteric" || v == "nutrient"))
            o.pod_pulse = v == "enteric";
        else if (f == "--pod-frames" && (v == "inherited" || v == "single"))
            o.pod_frames = v == "inherited";
        else if (f == "--pod-regimes" && (v == "shared" || v == "independent"))
            o.pod_regimes = v == "shared";
        else if (f == "--pod-spacing" && (v == "continuous" || v == "inherited"))
            o.pod_spacing = v == "inherited";
        else if (f == "--pod-delay")
            o.pod_lag = number(v, 1, 8);
        else if (f == "--pod-period")
            o.pod_period = number(v, 1, 16777216);
        else if (f == "--pod-clock" && (v == "fixed" || v == "mixed"))
            o.pod_clock = v == "mixed";
        else if (f == "--pod-credit" && (v == "live" || v == "frozen" || v == "shifted"))
            o.pod_credit = v == "live" ? 1 : v == "shifted" ? 2 : 0;
        else if (f == "--pod-origin" && (v == "random" || v == "reference" || v == "inert"))
            o.pod_origin = v == "random" ? 0 : v == "reference" ? 1 : 2;
        else if (f == "--pod-germ")
            o.pod_germ = v;
        else if (f == "--freeze-cohort" && (v == "a" || v == "b" || v == "none"))
            o.cut_cohort = v == "a" ? 1 : v == "b" ? 2 : 0;
        else if (f == "--raft-ripening" && (v == "steady" || v == "flush"))
            o.raft_flush = v == "flush";
        else if (f == "--raft-frames" && (v == "inherited" || v == "diverse"))
            o.raft_frames = v == "diverse";
        else if (f == "--raft-seasons")
            o.raft_seasons = number(v, 0, 8192);
        else if (f == "--raft-capacity") {
            o.rafts = number(v, 32, 1048576);
            if (!std::has_single_bit(o.rafts))
                throw std::invalid_argument("raft capacity must be a power of two");
        } else if (f == "--raft-days")
            o.raft_days = number(v, 8, 8192);
        else if (f == "--recruit" && (v == "provisioned" || v == "fostered"))
            o.raft_drift = v == "fostered";
        else if (f == "--pod-remap")
            o.remap_training = number(v, 1, 8192);
        else if (f == "--pod-observe")
            o.remap_observe = number(v, 0, 8192);
        else if (f == "--pod-cues")
            o.remap_cues = number(v, 1, 7);
        else if (f == "--pod-bench")
            o.motor_calls = number(v, 1, 67108864);
        else if (f == "--pod-assay")
            o.assay_pods = number(v, 1, 8192);
        else if (f == "--pod-subjects")
            o.pod_subjects = number(v, 1, 65536);
        else if (f == "--pod-test-seed")
            o.pod_test_seed = number(v, 0, ~0u);
        else if (f == "--steps")
            o.steps = number(v, 0, ~0u);
        else if (f == "--report-every")
            o.interval = number(v, 1, ~0u);
        else if (f == "--survey-origin") {
            o.survey_origin = number(v, 1, 1023);
        } else if (f == "--extend" || f == "--frontier") {
            Word n = number(v, f == "--extend" ? 4 : 0, 1024);
            if (n && !std::has_single_bit(n))
                throw std::invalid_argument("the frontier side must be a power of two");
            if (f == "--extend")
                o.extend = n;
            else {
                o.c.frontier = n;
                o.requested_frontier = true;
            }
        } else if (f == "--legacy")
            o.legacy = number(v, 0, 65536);
        else if (f == "--load")
            o.load = v;
        else if (f == "--save")
            o.save = v;
        else if (f == "--founder" || f == "--senses-from") {
            o.founder = v;
            o.sensory = f == "--senses-from";
        } else if (f == "--resident")
            o.resident = v;
        else if (f == "--slot")
            o.slot = number(v, 0, ~0u - 1);
        else if (f == "--colonists")
            o.colonists = v;
        else if (f == "--competitors")
            o.competitors = v;
        else if (f == "--soil")
            o.soil = v;
        else if (f == "--export-mind")
            o.export_mind = v;
        else if (f == "--side") {
            o.c.side = number(v, 4, 1024);
            if (!std::has_single_bit(o.c.side))
                throw std::invalid_argument("side must be a power of two");
        } else if (f == "--neurons") {
            o.explicit_cells = true;
            Word cells = number(v, 64, 1024);
            if (!std::has_single_bit(cells))
                throw std::invalid_argument("neural capacity must be a power of two");
            o.c.tiles = cells / 32;
        } else if (f == "--seed")
            o.c.seed = number(v, 0, ~0u);
        else if (f == "--season")
            o.c.season = number(v, 1, 16777216);
        else if (f == "--density")
            o.c.density = number(v, 0, 100);
        else if (f == "--assay-every")
            o.assay_interval = number(v, 1, ~0u);
        else if (f == "--hearing" && (v == "scalar" || v == "projected"))
            o.interventions.emplace_back(f, v == "projected" ? 4 : 0);
        else if (f == "--producers" && (v == "evolving" || v == "fixed" || v == "simple"))
            o.interventions.emplace_back(f, v == "evolving" ? 2 : v == "fixed" ? 1 : 0);
        else if (f == "--wire" && (v == "scrambled" || v == "relabeled"))
            o.interventions.emplace_back(f, v == "scrambled" ? 2 : 3);
        else if ((f == "--construction" || f == "--morphology" || f == "--architecture" || f == "--wire" ||
                  f == "--contact" || f == "--plasticity" || f == "--weaving") &&
                 (v == "open" || v == "closed"))
            o.interventions.emplace_back(f, v == "open");
        else
            throw std::invalid_argument("unknown floodplain option " + f);
    }
    if (o.natural_order) {
        if (o.surface_input.empty())
            throw std::invalid_argument("a material winding order requires a collection of real beds");
        o.retina_beds = 2;
    }
    if((o.signal_voice_exposures||o.signal_dialogues)&& (o.signal_channels!=2||o.signal_adult.empty()))throw std::invalid_argument("choose a pair of multiport animals for acoustic signalling");
    if(!o.signal_words.empty()&&!o.signal_dialogues)throw std::invalid_argument("record an acoustic encounter to write its trace");
    if(o.compound_bench&&(o.signal_channels!=2||o.signal_adult.empty()))throw std::invalid_argument("benchmark the two-junction readout of an independent animal pair");
    if(o.compound_repeats&&(o.signal_channels!=2||o.signal_adult.empty()||!o.signal_neighbours.empty()||Count(o.compound_repeats)*o.signal.tests>16777216))throw std::invalid_argument("run repeated compound histories on one independent pair");
    if(!o.compound_profiles.empty()&&!o.compound_repeats)throw std::invalid_argument("capture the profiles of repeated compound histories");
    if(o.signal_channels!=2 && (o.compound.observed<4 || o.compound.arbor || o.compound.diet || o.compound.wean))throw std::invalid_argument("use two junctions for compound sensory exposure");
    if(!o.signal_neighbours.empty()&&(o.signal_adult.empty()||!o.signal.lessons||o.signal.learners==3))throw std::invalid_argument("choose one learning participant and separate adult neighbours for this apprenticeship");
    if(!o.signal_tutor.empty()){
        if(!o.signal.lessons||!o.cap_members||!o.signal_adult.empty())throw std::invalid_argument("a chemical tutor needs a cohort of living learners");
        if(!o.explicit_signal_learning)o.signal.learners=3^o.signal.tutor;
    }
    if((o.signal.lessons||!o.signal_adult.empty()) && ((o.signal_adult.empty()?o.cap_members<2:o.cap_members!=0)||!o.cap_pupil.empty() || o.context_series.streams || o.cap_trials || Count(o.signal.lessons)*o.signal.replicas>32000000))throw std::invalid_argument("enroll independent resident pairs for one manageable catalytic signalling course");
    if(o.signal_adult.empty()&&(!o.signal_trace.empty()||!o.signal_save_adult.empty()))throw std::invalid_argument("select both material signallers for an individual record");
    if(!o.signal_save_adult.empty()&&o.signal_save_adult==o.signal_save_youth)throw std::invalid_argument("choose different files for the two educated signallers");
    if(o.context_series.streams){
        if(!o.context_series.at)o.context_series.at=std::max(1u,o.context_series.rounds/2);
        if((o.cap_pupil.empty()&&!o.cap_members) || o.context_series.jitter>o.cap_course.views ||
           ((o.context_series.second<32||o.cap_members)&&o.context_series.at>=o.context_series.rounds) ||
           Count(o.context_series.streams)*o.context_series.rounds>262144 ||
           (o.cap_members&&(o.cap_trials||o.context_series.varied||!o.context_trace.empty()||!o.context_save.empty()||o.context_series.mode!=capillary::contextual_learning)))
            throw std::invalid_argument("choose one enrolled cohort or individual and a valid material transition inside its lifetime");
    }else if(!o.context_trace.empty()||!o.context_save.empty())
        throw std::invalid_argument("a material trace or save needs a continuing study");
    if((!o.cap_film.empty()||!o.catalytic_film.empty()) && Count(o.cap_frames)*o.cap_window*o.cap_window>~0u)
        throw std::invalid_argument("reduce the optical window or frames to fit 32-bit sample indices");
    if(!o.cap_film.empty() && (o.cap_frames<=o.cap_delay||!o.steps))throw std::invalid_argument("record more pressure frames than the requested causal distance");
    if((o.cap_recipes||o.cap_orders||!o.cap_manual_trace.empty()||o.cap_vocab||o.cap_questions||o.cap_meetings||!o.cap_speech_trace.empty()||!o.cap_peer.empty()||!o.cap_meeting_trace.empty()) &&
       (!o.cap_trials || o.cap_pupil.empty() || (!o.cap_speech_trace.empty()&&!o.cap_questions) ||
        ((!o.cap_peer.empty())!=bool(o.cap_meetings||o.cap_orders)) || (!o.cap_manual_trace.empty()&&!o.cap_orders) || (!o.cap_meeting_trace.empty()&&!o.cap_meetings)))
        throw std::invalid_argument("causal discourse requires an enrolled individual and paired speakers for an exchange");
    if(bool(o.cap_trials||o.context_series.streams||(o.signal.lessons&&o.signal_adult.empty()))!=(o.cap_members || !o.cap_pupil.empty()) || (o.cap_members && (!o.cap_pupil.empty() || o.cap_bench || !o.cap_save.empty() || !o.cap_trace.empty() || !o.cap_lesson.empty())) ||
       (!o.cap_screen.empty()&&!o.cap_members) || ((o.cap_bench||!o.cap_trace.empty()||!o.cap_save.empty()||!o.cap_lesson.empty())&&!o.cap_trials))
        throw std::invalid_argument("enroll an individual and choose paired capillary trials");
    if((o.ink_follow!=~0u)!=!o.ink_events.empty() || (!o.ink_events.empty() && !o.steps))
        throw std::invalid_argument("follow a resident slot for at least one tick to write its witnessed events");
    if (o.story_trials && !o.story_residents && o.story_pupil.empty())
        throw std::invalid_argument("enroll a living pupil for the spoken histories");
    if(!o.story_trials && (o.story_benchmark || o.story_residents || o.story_broods || !o.story_pupil.empty() || !o.story_save.empty() ||
                          !o.story_trace.empty() || !o.voice_output.empty() || !o.story_screen.empty()))
        throw std::invalid_argument("select a spoken-history examination");
    if (!o.story_screen.empty() && !o.story_residents)
        throw std::invalid_argument("a resident screen needs a population");
    if (o.story_residents && (o.story_language==utterance::native || o.story_broods || o.story_benchmark || !o.story_pupil.empty() ||
                             !o.story_save.empty() || !o.story_mentor.empty() || !o.story_trace.empty()))
        throw std::invalid_argument("screen the live population on public descriptions before individual schooling");
    if ((!o.voice_donor.empty() || !o.voice_circuit.empty() || o.story_broods || !o.voice_output.empty())
        && (!o.story_trials || o.story_language!=utterance::native))throw std::invalid_argument("choose the native acoustic culture for these resonators");
    if((o.references.examples || o.references.trials || !o.reference_trace.empty()) && (!o.story_trials ||
        o.story_residents || o.story_language==utterance::native ||
        (!o.reference_trace.empty() && !o.references.trials)))
        throw std::invalid_argument("enroll an individual pupil and a public tutor for reference questions");
    if((o.accent_generations || !o.accent_voice.empty() || !o.accent_trace.empty() || o.accent_arm) &&
        (!o.accent_generations || !o.story_trials || o.story_language!=utterance::native || o.story_pupil.empty()))
        throw std::invalid_argument("a chain of acoustic pupils requires the native voice and a starting resident");
    if(o.story_broods && (!std::has_single_bit(o.inlet_cohort) || !o.voice_circuit.empty()))
        throw std::invalid_argument("the native brood requires a power-of-two cohort of watershed founders");
    if (!o.surface_input.empty() && !o.retina_trials && !o.audience_trials && !o.story_trials)
        throw std::invalid_argument("select a retinal examination for these physical works");
    if ((!o.retina_questions.empty() && (!o.retina_trials || o.inlet_generations || !o.retina_host.empty())) ||
        ((!o.retina_voice.empty() || !o.retina_terms.empty()) && o.retina_questions.empty()))
        throw std::invalid_argument("an acoustic cascade needs a living pupil and an examination");
    if (o.retina_trials % 4 || (o.inlet_landscape == inlet::mixed && o.retina_trials % 8) ||
        (!o.retina_trials &&
         (!o.retina_trace.empty() || !o.retina_germ.empty() || !o.retina_table.empty() || !o.retina_host.empty())))
        throw std::invalid_argument("a retinal study needs balanced, complete groups of layouts");
    if (o.inlet_trials % 4 ||
        ((!o.inlet_trace.empty() || !o.inlet_germ.empty() || !o.inlet_table.empty()) && !o.inlet_trials))
        throw std::invalid_argument("an unseen-inlet panel needs complete four-course rotations");
    if (o.inlet_landscape == inlet::mixed && o.inlet_trials % 8)
        throw std::invalid_argument("a mixed inlet panel needs equally many channels and basins");
    if (!o.audience_timbre.empty() && (!o.audience_trials || o.audience_tracer || o.audience_lessons || o.audience_generations))
        throw std::invalid_argument("an exchanged voice needs a held-out spoken encounter");
    if (o.audience_trials && ((!o.audience_juveniles && o.audience_guide.empty()) || o.audience_pupil.empty()))
        throw std::invalid_argument("select both enrolled bodies for an audience examination");
    if ((!o.audience_trace.empty() || !o.audience_output.empty() || o.audience_generations ||
         !o.audience_germ.empty() || !o.audience_guide.empty() || !o.audience_pupil.empty() || o.audience_juveniles) &&
        !o.audience_trials)
        throw std::invalid_argument("select audience trials for the enrolled residents");
    if (o.audience_generations && (!std::has_single_bit(o.inlet_cohort) || !o.audience_germ.empty()))
        throw std::invalid_argument("the audience nursery needs a power-of-two cohort and its founding population");
    if ((o.audience_lessons &&
         (!o.audience_trials || o.audience_tracer || o.audience_generations || o.audience_lessons % 2)) ||
        ((!o.audience_lessons_trace.empty() || o.audience_pairing) && !o.audience_lessons))
        throw std::invalid_argument("give the speaking guide a separate, even-sized set of public demonstrations");
    if (o.audience_juveniles &&
        (!o.audience_guide.empty() || o.audience_generations || !o.audience_germ.empty() || !o.audience_tracer))
        throw std::invalid_argument("screen the residents' own circuits by publicly moving a tracer");
    if (o.inlet_generations && ((!o.inlet_trials && !o.retina_trials) || !std::has_single_bit(o.inlet_cohort)))
        throw std::invalid_argument("a material brood needs an inlet test and a power-of-two population");
    if (o.inlet_cross && o.inlet_host.empty())
        throw std::invalid_argument("a modular cross needs a second embodied parent");
    if (o.receive_branch && (o.receive_donor.empty() || !(o.receive_loci & 1)))
        throw std::invalid_argument("a cortical branch needs a donor program");
    if ((!o.inlet_voice_trace.empty() && o.inlet_questions.empty()) ||
        (!o.inlet_questions.empty() && (!o.inlet_trials || o.inlet_generations)))
        throw std::invalid_argument("a prospective interview needs a living pupil and a held-out inlet panel");
    if ((!o.drift_food_germ.empty() || !o.drift_probe_trace.empty()) && !o.drift_probe)
        throw std::invalid_argument("a functional germline record needs a drift food study");
    if (!o.drift_questions.empty() && o.drift_diary.empty())
        throw std::invalid_argument("a replayed conversation needs a current diary");
    if (o.drift_watch && (o.reader_interval || !o.follow_witness.empty() || o.slot != ~0u || o.observer))
        throw std::invalid_argument("a drift observation chooses its own resident");
    if (o.drift_watch >= 3 && !o.drift_first)
        o.drift_first = 16384;
    if (o.drift_watch && o.drift_pupil >= 2 && o.trace_donor.empty())
        throw std::invalid_argument("a carrier observation needs a donor census");
    if (!o.drift_legacy_trace.empty() && !o.drift_surfaces)
        o.drift_surfaces = 16384;
    if (!o.drift_rival.empty() && o.drift_pool.empty())
        throw std::invalid_argument("a competing drift germline needs a founding pool");
    if (!o.drift_pool.empty() && !o.load.empty())
        throw std::invalid_argument("drift founding pools supply a new habitat");
    if (!o.drift_pool.empty() && o.drift_contest == ~0u)
        o.drift_contest = drift::contest | drift::sham;
    if (!o.drift_pool.empty() && o.drift_contest == 0)
        throw std::invalid_argument("a founding-pool comparison needs inherited lineage markers");
    if (!o.drift_stock.empty()) {
        if (!o.founder.empty() && (o.founder != o.drift_stock || o.sensory))
            throw std::invalid_argument("drift stock needs one common body and hand");
        o.founder = o.drift_stock;
    }
    if(!o.survey_trace.empty() && !o.feedback_survey)throw std::invalid_argument("a prospective pupil trace needs a feedback survey");
    if(o.receive_named && o.receive_donor.empty())throw std::invalid_argument("calibrated introduction needs a donor");
    if(o.survey_carriers && (!o.feedback_survey || o.trace_donor.empty()))
        throw std::invalid_argument("carrier enrollment needs a feedback survey and a donor census");
    if(o.after_birth!=~0u && !o.feedback_survey)throw std::invalid_argument("the pupil age filter needs a feedback survey");
    std::set<std::filesystem::path> evidence;
    for (const auto *path : {&o.hand_events,&o.signal_words,&o.compound_profiles,&o.signal_save_adult,&o.signal_save_youth,&o.signal_trace,&o.context_events,&o.context_save,&o.context_trace,&o.chemical_witness,&o.catalytic_film,&o.catalytic_proof,&o.cargo_proof,&o.cargo_events,&o.cap_film,&o.cap_manual_trace,&o.cap_speech_trace,&o.cap_meeting_trace,&o.cap_save,&o.cap_screen,&o.cap_events,&o.cap_trace,&o.cap_lesson,&o.accent_trace,&o.ink_trace,&o.ink_events,&o.story_save,&o.story_trace,&o.reference_trace,&o.story_screen,&o.voice_output,
                             &o.audience_lessons_trace,
                             &o.audience_trace,
                             &o.audience_output,
                             &o.probe_trace,
                             &o.surface_output,
                             &o.retina_voice,
                             &o.retina_trace,
                             &o.retina_table,
                             &o.retina_germ,
                             &o.inlet_trace,
                             &o.inlet_germ,
                             &o.inlet_table,
                             &o.inlet_voice_trace,
                             &o.drift_trace,
                             &o.drift_food_germ,
                             &o.drift_probe_trace,
                             &o.drift_diary,
                             &o.drift_germ,
                             &o.drift_repertoire,
                             &o.drift_legacy_trace,
                             &o.shore_trace,
                             &o.feedback_trace,
                             &o.survey_trace,
                             &o.feeding_trace,
                             &o.feedback_donor}) {
        if(path->empty())continue;
        auto target=std::filesystem::weakly_canonical(std::filesystem::absolute(*path));
        if(!evidence.insert(target).second)throw std::invalid_argument("each learning trace needs a distinct output");
        for(const auto *other:{&o.save,&o.export_mind,&o.class_trace,&o.gate_trace})
            if(!other->empty() && target==std::filesystem::weakly_canonical(std::filesystem::absolute(*other)))
                throw std::invalid_argument("a learning trace would overwrite another output");
    }
    return o;
}
void intervene(Configuration &c, const Options &o) {
    for (const auto &[name, on] : o.interventions) {
        if (name == "--construction")
            c.terrain = on;
        else if (name == "--morphology")
            c.morph = on;
        else if (name == "--architecture")
            c.architecture = on;
        else if (name == "--hearing")
            c.wire = (c.wire & 3) | on;
        else if (name == "--wire")
            c.wire = (c.wire & 4) | on;
        else if (name == "--contact")
            c.predation = on;
        else if (name == "--producers")
            c.flora = on;
        else if (name == "--weaving")
            c.weaving = on;
        else
            c.plastic = on;
    }
}
#include "flood_material_image.hpp"
#include "flood_nerve_arena.hpp"
#include "flood_gate_trial.hpp"
struct FieldWatch {
    Temporary laws, bitmap, seen;
    Function scan{}, test{};
    Word seed, practice, samples = 0, slot = ~0u;
    Count examined = 0;
    FieldWatch(Session &s, Word sites, Word key, Word trials)
        : laws(s.d, 16 * sizeof(Word)), bitmap(s.d, Count((sites + 31) / 32) * sizeof(Word)), seen(s.d, sizeof(Word)),
          seed(key), practice(trials) {}
};
#include "flood_shore.hpp"
#include "flood_donor.hpp"
#include "drift_records.hpp"
class Basin {
    Session &s;
    Configuration c;
    Word n, tiles, tick;
    bool packed_brains = false;
    Word neural_blocks = 0, collection_period = 0;
    Count collection_age = 0, collections = 0, repacked = 0, retired_pages = 0, discarded_pages = 0;
    Function inventory_nerve{}, repack_nerve{};
    NeuralPageLayout neural_layout;
    NeuralSpill neural_spill;
    Temporary nerve_index, nerve_claims, nerve_overrides, nerve_origin;
    Function import_nerve{}, export_nerve{}, gather_spill{};
    Temporary land, slots, dna, animals, nerves, ears, motors, counters, clock, partial, budget;
    Temporary mineral_bed, workers, fixtures, material_books;
    Temporary cortex, growth_books;
    Temporary pod_tissue, rinds, pod_book, pod_total;
    Temporary forecasts, tide_book, tide_sights, enquirers, impressions, worksite_books;
    Function construction_tide{};
    Temporary polymers, sediment_books, salts, brine_tissue;
    Temporary reed_animals, reed_ports, reed_ledger, airborne, audience_book;
    Temporary clayers, earthen, palms, clay_book;
    Temporary riggers, floaters, float_books, sluice_contacts, sluice_books, float_events, collar_books, regulatory_books;Function moorings{},gated_flows{};
    drift::Law drift_law{};
    drift::Totals drift_opening{};
    Word drift_start = 0;
    DriftWatch *drift_watch = nullptr;
    Temporary drift_books, drift_demography, drift_yields, drift_rafts, drift_spills, light_books, bottom_books;
    Temporary course_books, stirred, gaze_books, audience_books, trails, ink_books;
    Temporary cap_neurons,cap_contexts,cap_appetites,cap_experiences,cap_origins,cap_readers,cap_deliveries,cap_board,cap_ledgers,cap_notes,cap_archive;
    Temporary hand_weights,hand_pending,hand_books,hand_camera;
    Word cap_start=0;capillary::Device capillary_field{};Function surge_kernel{},cargo_kernel{};
    capillary::Hands material_arms{};Function hand_act{},hand_learn{},hand_export{};
    Temporary cargo_totals,cargo_notes,cargo_impulses;detritus::Recording cargo_film{};
    Word cap_mask=0,cap_graph_mask=0,cap_graph_nodes=0;Count cap_launches=0;bool cap_scheduled=true;
    Word ink_slots=0;bool ink_echo=false;utterance::Watch ink_watch{};Temporary ink_notes;
    std::unique_ptr<Temporary> probe_samples;
    std::unique_ptr<Temporary> public_samples;
    Word probe_sites = 0, rotating_samples = 0;
    std::unique_ptr<Temporary> retinal_library;
    Word retinal_sites = 0;
    Function illumination{};
    std::unique_ptr<DriftClock> optical_clock;
    DriftClock *drift_clock = nullptr;
    drift::Gaze drift_eyes{};
    drift::LegacyWatch drift_inheritance{};
    drift::Sampling drift_sampling{};
    drift::Pedigree drift_births{};
    std::array<Count, 3> drift_adults{};
    bool drift_counted = false;
    Function drift_flow{}, drift_check{};
    ShoreSchool *school=nullptr;Function teach_shore{};
    Temporary rattlers, rattle_books, rattle_queries, mesh_book;
    rattle::Field microphones{};
    Function bank_work{};
    Temporary cutters,folded,reed_books,reed_effects;
    Temporary parcels,wick_ledgers;
    Temporary apprentices,contexts,practice_books,marked_strands,practice_credits;
    Temporary inventors,links,patents,link_books,link_layers;
    Function work_links{};
    Temporary cost_minds, treads, cost_books;
    Temporary path_brains, path_marks, path_sites, path_soil, path_books;
    World world;
    GraphApi gpu{};
    Graph graph{};
    GraphExec batch{};
    Stream stream{};
    cuda::Event fence{};
    Function seed{}, water{}, think{}, meet{}, time{}, survey{}, totals{};
    template <class... Args> void launch(Function f, Word blocks, Word threads, Args &...values) {
        void *p[]{static_cast<void *>(&values)...};
        s.d.check(s.d.launch(f, blocks, 1, 1, threads, 1, 1, 0, stream, p, nullptr), "run floodplain kernel");
    }
    void frame(Word offset,bool pressure_due) {
        launch(water, (n / 2 + 127) / 128, 128, world, c, clock.ptr, offset);
        launch(think, neural_blocks, 128, world, c, clock.ptr, offset);
        launch(meet, (n / 2 + 127) / 128, 128, world, c, clock.ptr, offset, rattlers.ptr, drift_demography.ptr);
        if (microphones.minds)
            launch(bank_work, ((n / 4 + 3) / 4 + 127) / 128, 128, world, c, microphones, clock.ptr, offset);
        if(world.braids.minds) {
            Word blocks=((n/4+3)/4+127)/128;
            if(regulatory_books.ptr)launch(moorings,blocks,128,world,c,clock.ptr,offset,regulatory_books.ptr);
            else launch(moorings,blocks,128,world,c,clock.ptr,offset);
        }
        if (drift_books.ptr) {
            Word work = (n / 4) >> drift_law.stride_shift, blocks = (work + 127) / 128;
            bool measured = drift_clock && drift_clock->start(stream, work);
            if (drift_watch && drift_watch->records.ptr)
                launch(drift_flow, blocks, 128, world, c, clock.ptr, offset, drift_law, drift_books.ptr, drift_sampling,
                       drift_inheritance, drift_eyes, drift_watch->capture);
            else
                launch(drift_flow, blocks, 128, world, c, clock.ptr, offset, drift_law, drift_books.ptr, drift_sampling,
                       drift_inheritance, drift_eyes);
            if (measured)
                drift_clock->stop(stream);
        }
        if(pressure_due&&hand_weights.ptr)launch(hand_act,(n+127)/128,128,world,c,clock.ptr,offset,capillary_field,material_arms);
        if(pressure_due&&cap_ledgers.ptr)launch(surge_kernel,(n+127)/128,128,world,c,clock.ptr,offset,capillary_field);
        if(pressure_due&&hand_weights.ptr)launch(hand_learn,(n+127)/128,128,world,c,clock.ptr,offset,capillary_field,material_arms);
        if(pressure_due&&cargo_totals.ptr)launch(cargo_kernel,(n/4+127)/128,128,world,c,clock.ptr,offset,capillary_field,cargo_totals.ptr,cargo_film);
        if (light_books.ptr) {
            Word shift = (drift_law.ports & retina::cadence_mask) >> retina::cadence_shift;
            Word work = ((n / 4) + (1u << shift) - 1) >> shift;
            bool measured = optical_clock && optical_clock->start(stream, work);
            if (stirred.ptr) {
                DevicePtr notes = probe_samples ? probe_samples->ptr : 0;
                if (ink_books.ptr) {
                    DevicePtr public_notes=public_samples?public_samples->ptr:0;
                    launch(illumination,(work+127)/128,128,world,c,clock.ptr,offset,drift_law,light_books.ptr,
                        gaze_books.ptr,stirred.ptr,notes,probe_sites,rotating_samples,audience_books.ptr,public_notes,
                        ink_books.ptr,ink_notes.ptr,ink_slots,ink_watch);
                } else if (audience_books.ptr) {
                    DevicePtr public_notes = public_samples ? public_samples->ptr : 0;
                    launch(illumination, (work + 127) / 128, 128, world, c, clock.ptr, offset, drift_law,
                           light_books.ptr, gaze_books.ptr, stirred.ptr, notes, probe_sites, rotating_samples,
                           audience_books.ptr, public_notes);
                } else
                    launch(illumination, (work + 127) / 128, 128, world, c, clock.ptr, offset, drift_law,
                           light_books.ptr, gaze_books.ptr, stirred.ptr, notes, probe_sites, rotating_samples);
            } else if (gaze_books.ptr)
                launch(illumination, (work + 127) / 128, 128, world, c, clock.ptr, offset, drift_law, light_books.ptr,
                       gaze_books.ptr);
            else
                launch(illumination, (work + 127) / 128, 128, world, c, clock.ptr, offset, drift_law, light_books.ptr);
            if (measured)
                optical_clock->stop(stream);
        }
        if(school && school->size)launch(teach_shore,(school->size+31)/32,32,world,c,clock.ptr,offset,school->syllabus.ptr,school->pupils.ptr,school->size);
        if(world.braids.mode & braid::sluices)launch(gated_flows,(((n/8+7)/8)+127)/128,128,world,c,clock.ptr,offset);
        if(world.linkage.minds)launch(work_links,(n/4+127)/128,128,world,c,clock.ptr,offset);
        if ((world.tide.active & 1) && world.tide.cells)
            launch(construction_tide, (n + 127) / 128, 128, world, c, clock.ptr, offset);
        if (drift_eyes.choice)
            launch(drift_check, (drift_eyes.tiles + 127) / 128, 128, world, clock.ptr, offset, drift_eyes);
    }
    void synchronize() {
        s.d.check(s.d.record(fence, stream), "mark physical changes");
        s.d.check(s.d.wait(fence), "finish physical changes");
    }

  public:
    Basin(Session &s, Habitat &h, bool restore, const std::string &prior, Word previous_side = 0, bool sensory = false,
          NeuralDispatch dispatch = NeuralDispatch::automatic, NeuralMemory memory = NeuralMemory::automatic,
          Word cache = ~0u, Word collection = 4096, ShoreSchool *attendance = nullptr,
          DriftWatch *drift_recorder = nullptr, const apprentice::Donor *drift_tracer = nullptr,
          DriftSurfaces *surfaces = nullptr, DriftSighting *sighting = nullptr, Word drift_account = ~0u,
          DriftDiary *diary = nullptr, DriftClock *drift_timer = nullptr, InletCensus *inlets = nullptr,
          std::optional<bool> sample_motion = std::nullopt,const Options *capture=nullptr)
        : s(s), c(h.header.configuration), n(h.sites()), tiles(c.tiles), tick(h.header.tick),
          neural_layout(s, h, memory, cache), neural_spill(neural_layout.virtual_bytes()),
          nerve_index(s.d, neural_layout.paged ? Count(neural_layout.logical) * sizeof(Word) : 0),
          nerve_claims(s.d, neural_layout.paged ? Count(neural_layout.reserve) * sizeof(Word) : 0),
          nerve_overrides(s.d, neural_layout.paged ? Count(n) * sizeof(Word) : 0),
          nerve_origin(s.d, neural_layout.paged ? n : 0), land(s.d, Count(n) * sizeof(Silt)),
          slots(s.d, n * sizeof(Word)), dna(s.d, Count(n) * sizeof(Germ)), animals(s.d, Count(n) * sizeof(Colony)),
          nerves(s.d, neural_layout.tissue_bytes()), ears(s.d, Count(n) * sizeof(Ear)), motors(s.d, n),
          counters(s.d, ((n / 2 + 127) / 128) * sizeof(Counters)), clock(s.d, sizeof(Word)),
          partial(s.d, ((n + 127) / 128) * sizeof(BasinCensus)), budget(s.d, sizeof(Counters)),
          mineral_bed(s.d, h.articulated ? Count(n) * sizeof(material::Grain) : 0),
          workers(s.d, h.articulated ? Count(n) * sizeof(material::Worker) : 0), fixtures(s.d, h.articulated ? n : 0),
          material_books(s.d, h.articulated ? ((n / 2 + 127) / 128) * sizeof(material::Ledger) : 0),
          cortex(s.d, h.developing ? Count(n) * sizeof(Cortex) : 0),
          growth_books(s.d, h.developing ? ((n / 2 + 127) / 128) * sizeof(GrowthTotals) : 0),
          pod_tissue(s.d, h.enclosed ? Count(n) * sizeof(pods::Animal) : 0),
          rinds(s.d, h.enclosed ? n * sizeof(pods::Surface) : 0),
          pod_book(s.d, h.enclosed ? ((n / 2 + 127) / 128) * sizeof(pods::Census) : 0),
          pod_total(s.d, h.enclosed ? sizeof(pods::Census) : 0),
          forecasts(s.d, h.foresight ? Count(n) * sizeof(material::Foresight) : 0),
          tide_book(s.d, h.foresight ? sizeof(material::TideTotals) : 0),
          tide_sights(s.d, h.foresight ? Count(n) * sizeof(Half) : 0),
          enquirers(s.d, h.investigatory ? Count(n) * sizeof(material::Inquiry) : 0),
          impressions(s.d, h.investigatory ? Count(n) * sizeof(material::Imprint) : 0),
          worksite_books(s.d, h.investigatory ? ((n / 2 + 127) / 128) * sizeof(material::WorksiteTotals) : 0),
          polymers(s.d, h.sedimentary ? Count(n) * sizeof(Half) : 0),
          sediment_books(s.d, h.sedimentary ? ((n / 2 + 127) / 128) * sizeof(material::SedimentTotals) : 0),
          salts(s.d, h.saline ? Count(n) * sizeof(brine::Salts) : 0),
          brine_tissue(s.d, h.saline ? Count(n) * sizeof(brine::Animal) : 0),
          reed_animals(s.d, h.calling ? Count(n) * sizeof(siphon::Animal) : 0), reed_ports(s.d, h.calling ? n : 0),
          reed_ledger(s.d, h.calling ? Count((n / 2 + 127) / 128) * sizeof(siphon::Totals) : 0),
          airborne(s.d, h.overhearing ? Count(n) * sizeof(siphon::Echo) : 0),
          audience_book(s.d, h.overhearing ? Count((n / 2 + 127) / 128) * sizeof(siphon::Audience) : 0),
          clayers(s.d, h.pottery ? Count(n) * sizeof(clay::Mind) : 0),
          earthen(s.d, h.pottery ? Count(n) * sizeof(clay::Tile) : 0),
          palms(s.d, h.pottery ? Count(n) * sizeof(clay::Tile) : 0),
          clay_book(s.d, h.pottery ? Count((n / 2 + 127) / 128) * sizeof(clay::Totals) : 0),
          riggers(s.d, h.braiding ? Count(n) * sizeof(braid::Mind) : 0),
          floaters(s.d, h.braiding ? Count(n) * sizeof(braid::Half) : 0),
          float_books(s.d, h.braiding ? Count((n / 4 + 127) / 128) * sizeof(braid::Totals) : 0),
          sluice_contacts(s.d, h.braiding && h.loom.version >= 3 ? Count(n) * sizeof(braid::GateMemory) : 0),
          sluice_books(s.d,
                       h.braiding && h.loom.version >= 3 ? Count((n / 8 + 127) / 128) * sizeof(braid::GateBook) : 0),
          float_events(s.d, h.recording_floats ? Count(n) * sizeof(braid::Episode) : 0),
          collar_books(s.d, h.braiding && h.loom.version >= 4 ? Count((n / 8 + 127) / 128) * sizeof(braid::SedimentBook)
                                                              : 0),
          regulatory_books(s.d, h.braiding && (h.loom.mode & regulation::motor_port)
                                    ? Count((n / 4 + 127) / 128) * sizeof(regulation::Ledger)
                                    : 0),
          drift_law(h.drift_water.law), drift_opening(h.drift_water.total), drift_start(h.header.tick),
          drift_watch(drift_recorder),
          drift_books(s.d, drift_law.ports & drift::moving ? Count((n / 4 + 127) / 128) * sizeof(drift::Totals) : 0),
          drift_demography(s.d,
                           drift_law.ports & drift::contest ? Count((n / 2 + 127) / 128) * sizeof(drift::Pedigree) : 0),
          drift_yields(s.d,
                       (drift_law.ports & drift::moving) && ((drift_law.ports & drift::contest) || drift_tracer)
                           ? Count((n / 4 + 1023) / 1024) * drift::phases * drift::populations * sizeof(drift::Intake)
                           : 0),
          drift_rafts(s.d, (drift_law.ports & drift::moving) && (drift_law.ports & drift::rafting)
                               ? 128 * sizeof(drift::Cargo)
                               : 0),
          drift_spills(s.d, (drift_law.ports & drift::moving) && (drift_law.ports & drift::storage)
                                ? 128 * sizeof(drift::Spill)
                                : 0),
          light_books(s.d, (drift_law.ports & drift::moving) && (drift_law.ports & retina::channel_mask)
                               ? 128 * sizeof(retina::Ledger)
                               : 0),
          bottom_books(s.d, (drift_law.ports & drift::bottom_depth) ? 128 * sizeof(drift::BottomAccess) : 0),
          course_books(s.d, (drift_law.ports & drift::moving) && (drift_law.ports & drift::winding)
                                ? 128 * sizeof(drift::Courses)
                                : 0),
          stirred(s.d, light_books.ptr && (h.loom.mode & retina::sounding_port) ? 128 * sizeof(retina::StirPatch) : 0),
          gaze_books(s.d, light_books.ptr && (h.loom.mode & retina::iris_port) ? 128 * sizeof(retina::GazeLedger) : 0),
          audience_books(s.d, stirred.ptr && (h.loom.mode & audience::connected) ? 128 * sizeof(audience::Ledger) : 0),
          trails(s.d,h.braiding && h.loom.version>=10?Count(n)*sizeof(utterance::Trail):0),
          ink_books(s.d,stirred.ptr && (h.loom.mode&utterance::ink_port)?128*sizeof(utterance::Ledger):0),
          cap_neurons(s.d,h.braiding && h.loom.version>=14?Count(n)*sizeof(capillary::Mind):0),
          cap_contexts(s.d,h.braiding&&h.loom.version>=17?Count(n)*sizeof(capillary::History):0),
          cap_appetites(s.d,h.braiding&&h.loom.version>=20?n:0),
          cap_experiences(s.d,cap_contexts.ptr&&capture&&!capture->context_events.empty()?Count(std::min(n,16384u))*2*sizeof(capillary::ContextNote):0),
          cap_origins(s.d,capture&&!capture->chemical_witness.empty()?Count(std::min(c.side,capture->cap_window))*std::min(c.side,capture->cap_window)*2*sizeof(capillary::PumpOrigin):0),
          cap_readers(s.d,cap_origins.ptr?Count(std::min(32768u,4u*std::min(c.side,capture->cap_window)*std::min(c.side,capture->cap_window)))*sizeof(capillary::Reception):0),
          cap_deliveries(s.d,cap_origins.ptr?sizeof(capillary::Deliveries):0),
          cap_board(s.d,cap_neurons.ptr?Count((n+31)/32)*4*sizeof(Word):0),
          cap_ledgers(s.d,cap_neurons.ptr && h.cap_law.mode?128*sizeof(capillary::Ledger):0),
          cap_notes(s.d,cap_ledgers.ptr&&capture&&!capture->cap_events.empty()?Count(std::min(1024u,n))*sizeof(capillary::Note):0),
          cap_archive(s.d,cap_ledgers.ptr&&capture&&(!capture->cap_film.empty()||!capture->catalytic_film.empty())?Count(std::min(c.side,capture->cap_window))*std::min(c.side,capture->cap_window)*capture->cap_frames*sizeof(capillary::Mark):0),
          hand_weights(s.d,(h.cap_law.mode&capillary::material_hands)?Count(n)*4:0),
          hand_pending(s.d,hand_weights.ptr?n:0),hand_books(s.d,hand_weights.ptr?128*sizeof(capillary::HandCounts):0),
          hand_camera(s.d,hand_weights.ptr&&capture&&!capture->hand_events.empty()?Count(std::min(n,4096u))*sizeof(capillary::HandNote):0),
          cap_start(tick),cargo_totals(s.d,(h.cap_law.mode&detritus::modes)?128*sizeof(detritus::Ledger):0),
          cargo_notes(s.d,cargo_totals.ptr&&capture&&!capture->cargo_events.empty()?Count(std::min(512u,n/4)+std::min(8192u,n/4))*sizeof(detritus::Note):0),
          cargo_impulses(s.d,cargo_notes.ptr?Count((n+31)/32)*4:0),ink_slots(ink_books.ptr && capture && !capture->ink_trace.empty()?std::min(512u,n/4):0),
          ink_echo(bool(h.loom.mode&utterance::stored_vocal_feedback)),
          ink_watch{capture?capture->ink_follow:~0u,tick,ink_books.ptr && capture && !capture->ink_events.empty()?capture->steps:0,
                    h.steady_eyes()?1u+Word(bool(h.loom.mode&braid::individual_eyes)):0u},
          ink_notes(s.d,(Count(ink_slots)+ink_watch.ticks)*sizeof(utterance::Sample)),
          rattlers(s.d, h.rattling ? Count(n) * sizeof(rattle::Mind) : 0),
          rattle_books(s.d, h.rattling ? Count((n / 4 + 127) / 128) * sizeof(rattle::Totals) : 0),
          rattle_queries(
              s.d, h.rattling && h.backwater.format >= 2 ? Count((n / 4 + 127) / 128) * sizeof(rattle::Attention) : 0),
          mesh_book(s.d, h.rattling && h.backwater.format >= 3 ? Count((n / 4 + 127) / 128) * sizeof(rattle::Load) : 0),
          cutters(s.d, h.reeding ? Count(n) * sizeof(runnel::Mind) : 0), folded(s.d, h.reeding ? n : 0),
          reed_books(s.d, h.reeding ? Count((n / 2 + 127) / 128) * sizeof(runnel::Totals) : 0),
          reed_effects(s.d, h.reeding ? Count((n / 2 + 127) / 128) * sizeof(runnel::Social) : 0),
          parcels(s.d, h.wicking ? Count(n) * 4 : 0),
          wick_ledgers(s.d, h.wicking ? Count((n / 2 + 127) / 128) * sizeof(wicks::Totals) : 0),
          apprentices(s.d, h.practising ? Count(n) * sizeof(practice::Mind) : 0),
          contexts(s.d, h.practising ? Count(n) * h.learning.arena * sizeof(practice::Context) : 0),
          practice_books(s.d, h.practising ? Count((n / 4 + 127) / 128) * sizeof(practice::Totals) : 0),
          marked_strands(s.d, h.practising ? Count(n / 4) * sizeof(practice::Mark) : 0),
          practice_credits(s.d, h.practising ? Count((n / 4 + 127) / 128) * sizeof(practice::Credits) : 0),
          inventors(s.d, h.linking ? Count(n) * sizeof(linkage::Mind) : 0),
          links(s.d, h.linking ? Count(n) * h.millwork.arena * sizeof(linkage::Tissue) : 0),
          patents(s.d, h.linking ? Count(n / 4) * sizeof(linkage::Provenance) : 0),
          link_books(s.d, h.linking ? Count((n / 4 + 127) / 128) * sizeof(linkage::Totals) : 0),
          link_layers(s.d,
                      h.linking ? ((h.millwork.mode & linkage::hydration) ? 8 : 4) * sizeof(linkage::Material) : 0),
          cost_minds(s.d, h.appraising ? Count(n) * sizeof(effort::Mind) : 0), treads(s.d, h.appraising ? n : 0),
          cost_books(s.d, h.appraising ? Count((n / 2 + 127) / 128) * sizeof(effort::Ledger) : 0),
          path_brains(s.d, h.ranging ? Count(n) * sizeof(hollow::Mind) : 0),
          path_marks(s.d, h.ranging ? Count(n) * h.recess.slots * sizeof(Word) : 0),
          path_sites(s.d, h.ranging ? Count((n + 15) / 16) * h.recess.slots * sizeof(Word) : 0),
          path_soil(s.d, h.ranging ? Count(n) * sizeof(hollow::Bed) : 0),
          path_books(s.d, h.ranging ? Count((n / 2 + 127) / 128) * sizeof(hollow::Ledger) : 0),
          world{reinterpret_cast<Silt *>(land.ptr),     reinterpret_cast<Word *>(slots.ptr),
                reinterpret_cast<Germ *>(dna.ptr),      reinterpret_cast<Colony *>(animals.ptr),
                reinterpret_cast<Tissue *>(nerves.ptr), reinterpret_cast<Ear *>(ears.ptr),
                reinterpret_cast<Byte *>(motors.ptr),   reinterpret_cast<Counters *>(counters.ptr)} {
        s.bind(seed, "flood_seed", Image::floodplain);
        s.bind(water, "flood_water", Image::floodplain);
        if (neural_layout.paged) {
            collection_period = collection;
            s.bind(inventory_nerve, "flood_nerve_inventory", Image::floodplain);
            s.bind(repack_nerve, "flood_nerve_repack", Image::floodplain);
            world.neural_pages = {reinterpret_cast<Word *>(nerve_index.ptr),
                                  reinterpret_cast<Word *>(nerve_claims.ptr),
                                  neural_spill.data,
                                  reinterpret_cast<Word *>(nerve_overrides.ptr),
                                  reinterpret_cast<Byte *>(nerve_origin.ptr),
                                  n,
                                  neural_layout.groups,
                                  neural_layout.hot,
                                  neural_layout.reserve};
            s.d.check(s.d.zero(nerves.ptr, 0, neural_layout.tissue_bytes()), "zero the neural page pool");
            s.d.check(
                s.d.upload(nerve_index.ptr, neural_layout.initial.data(), neural_layout.initial.size() * sizeof(Word)),
                "install the occupied brain pages");
            s.d.check(s.d.upload(nerve_origin.ptr, neural_layout.origin.data(), n),
                      "install latent developmental rows");
            s.d.check(s.d.upload(nerve_overrides.ptr, neural_layout.overrides.data(), Count(n) * sizeof(Word)),
                      "restore explicit neural contacts");
            std::vector<Word>().swap(neural_layout.overrides);
            std::vector<Byte>().swap(neural_layout.origin);
            if (nerve_claims.ptr)
                s.d.check(s.d.zero(nerve_claims.ptr, 0, Count(neural_layout.reserve) * sizeof(Word)),
                          "clear spare neural page claims");
            s.bind(import_nerve, "flood_nerve_import", Image::floodplain);
            s.bind(export_nerve, "flood_nerve_export", Image::floodplain);
            s.bind(gather_spill, "flood_nerve_spills", Image::floodplain);
        }
        packed_brains = pack_cortex(h, dispatch);
        neural_blocks = (n * (packed_brains ? 1u : tiles) + 127) / 128;
        s.bind(think, packed_brains ? "flood_think_packed" : "flood_think_" + std::to_string(tiles), Image::floodplain);
        s.bind(meet, "flood_meet", Image::floodplain);
        s.bind(time, "flood_clock", Image::floodplain);
        s.bind(survey, "flood_audit", Image::floodplain);
        s.bind(totals, "flood_budget", Image::floodplain);
        gpu.bind(s.d);
        s.d.check(s.d.event_new(&fence, 2), "create floodplain fence");
        s.d.check(gpu.create_stream(&stream, 0), "open floodplain stream");
        s.d.check(s.d.upload(clock.ptr, &tick, sizeof(tick)), "restore floodplain time");
        reset_account(h.header.account);
        if (sighting) {
            drift_eyes = sighting->gaze;
            s.bind(drift_check, "drift_witness_check", Image::floodplain);
        }
        if (diary)
            drift_eyes.diary = diary->diary;
        if (inlets)
            drift_eyes.first = inlets->attach(s, n, tick);
        if (surfaces) {
            world.drift_marks = surfaces->attach(s, n);
            drift_inheritance = surfaces->watch;
        }
        if (drift_demography.ptr) {
            drift_births = h.drift_water.pedigree;
            s.d.check(s.d.zero(drift_demography.ptr, 0, Count((n / 2 + 127) / 128) * sizeof(drift::Pedigree)),
                      "clear inherited population counts");
            s.d.check(s.d.upload(drift_demography.ptr, &drift_births, sizeof(drift_births)),
                      "restore births and deaths by lineage");
        }
        if (drift_yields.ptr) {
            s.d.check(
                s.d.zero(drift_yields.ptr, 0,
                         Count((n / 4 + 1023) / 1024) * drift::phases * drift::populations * sizeof(drift::Intake)),
                "clear measured harvests");
            drift_sampling = {reinterpret_cast<drift::Intake *>(drift_yields.ptr),
                              drift_tracer ? signature(*drift_tracer) : drift::Signature{}, Word(bool(drift_tracer))};
        }
        if (drift_rafts.ptr) {
            drift_sampling.rafts = reinterpret_cast<drift::Cargo *>(drift_rafts.ptr);
            s.d.check(s.d.zero(drift_rafts.ptr, 0, 128 * sizeof(drift::Cargo)), "clear passive carrier movement");
        }
        if (bottom_books.ptr) {
            world.bottom = reinterpret_cast<drift::BottomAccess *>(bottom_books.ptr);
            world.shoals = bool(drift_law.ports & drift::shoals);
            s.d.check(s.d.zero(bottom_books.ptr, 0, 128 * sizeof(drift::BottomAccess)),
                      "empty the benthic access ledger");
        }
        if (light_books.ptr) {
            s.bind(illumination, illumination_name(), Image::floodplain);
            if (audience_books.ptr)
                s.d.check(s.d.zero(audience_books.ptr, 0, 128 * sizeof(audience::Ledger)), "clear public contacts");
            if (gaze_books.ptr)
                s.d.check(s.d.zero(gaze_books.ptr, 0, 128 * sizeof(retina::GazeLedger)),
                          "clear the acquisition accounts");
            if (stirred.ptr)
                s.d.check(s.d.zero(stirred.ptr, 0, 128 * sizeof(retina::StirPatch)), "empty the tracer balances");
            s.d.check(s.d.zero(light_books.ptr, 0, 128 * sizeof(retina::Ledger)),
                      "empty the retinal observation ledger");
        }
        if (drift_books.ptr) {
            std::string kernel = drift_spills.ptr ? "flood_sill" : "flood_drift";
            if (course_books.ptr) {
                kernel = drift_spills.ptr ? "flood_sill_woven" : "flood_woven";
                drift_sampling.courses = reinterpret_cast<drift::Courses *>(course_books.ptr);
                s.d.check(s.d.zero(course_books.ptr, 0, 128 * sizeof(drift::Courses)),
                          "empty material winding records");
            }
            if (drift_watch && drift_watch->records.ptr)
                kernel += "_record";
            else if (!world.drift_marks.items && !drift_eyes.choice && !drift_eyes.first.visits &&
                     !(drift_law.ports & drift::sight))
                kernel += "_core";
            if (drift_spills.ptr) {
                drift_sampling.spills = reinterpret_cast<drift::Spill *>(drift_spills.ptr);
                s.d.check(s.d.zero(drift_spills.ptr, 0, 128 * sizeof(drift::Spill)), "empty the storage sill ledger");
            }
            s.bind(drift_flow, kernel, Image::floodplain);
            int regs = 0, local = 0;
            s.d.check(s.d.func_attr(&regs, 4, drift_flow), "measure drift registers");
            s.d.check(s.d.func_attr(&local, 3, drift_flow), "measure drift local memory");
            drift_sampling.aggregate = drift_account == ~0u ? 1u : drift_account;
            std::cout << "drift_kernel=" << kernel << "  aggregate=" << drift_sampling.aggregate
                      << "  registers=" << regs << "  local_bytes=" << local
                      << "  new_bytes_per_body=0  threads=128  active_tiles=" << ((n / 4) >> drift_law.stride_shift)
                      << '\n';
            s.d.check(s.d.zero(drift_books.ptr, 0, Count((n / 4 + 127) / 128) * sizeof(drift::Totals)),
                      "clear dripping carbon accounts");
            s.d.check(s.d.upload(drift_books.ptr, &h.drift_water.total, sizeof(drift::Totals)),
                      "restore drifting food accounts");
        }
        if(cap_neurons.ptr){
            connect_hands();
            if(cap_appetites.ptr){
                world.chemical_contacts=reinterpret_cast<unsigned char*>(cap_appetites.ptr);
                s.d.check(s.d.zero(cap_appetites.ptr,0,n),"clear natal chemical foraging contacts");
            }
            if(cap_contexts.ptr){world.material_history=reinterpret_cast<capillary::History*>(cap_contexts.ptr);
                s.d.check(s.d.zero(cap_contexts.ptr,0,Count(n)*sizeof(capillary::History)),"clear newborn context receipts");}
            world.capillaries=reinterpret_cast<capillary::Mind*>(cap_neurons.ptr);
            Word words=(n+31)/32;capillary_field={reinterpret_cast<Word*>(cap_board.ptr),reinterpret_cast<capillary::Ledger*>(cap_ledgers.ptr),
                reinterpret_cast<capillary::Note*>(cap_notes.ptr),cap_notes.ptr?std::min(1024u,n):0,h.cap_law};
            s.d.check(s.d.zero(cap_neurons.ptr,0,Count(n)*sizeof(capillary::Mind)),"clear natal intervention contacts");
            s.d.check(s.d.zero(cap_board.ptr,0,Count(words)*2*sizeof(Word)),"still the pressure field");
            Function coat{};s.bind(coat,"capillary_coatings",Image::floodplain);launch(coat,(words+127)/128,128,cap_board.ptr,words,c.seed);
            if(cap_origins.ptr){
                Word width=std::min(c.side,capture->cap_window),area=width*width;
                capillary_field.gallery={reinterpret_cast<capillary::PumpOrigin*>(cap_origins.ptr),reinterpret_cast<capillary::Reception*>(cap_readers.ptr),
                    reinterpret_cast<capillary::Deliveries*>(cap_deliveries.ptr),width,mix(capture->class_seed+4139)&(c.side-1),mix(capture->class_seed+17389)&(c.side-1),std::min(32768u,4*area)};
                s.d.check(s.d.zero(cap_origins.ptr,0,Count(area)*2*sizeof(capillary::PumpOrigin)),"clear the two-beat physical observer");
                s.d.check(s.d.zero(cap_deliveries.ptr,0,sizeof(capillary::Deliveries)),"clear chemical reception counts");
            }
            if(cap_archive.ptr){
                Word width=std::min(c.side,capture->cap_window);
                capillary_field.film={reinterpret_cast<capillary::Mark*>(cap_archive.ptr),width,mix(capture->class_seed+4139)&(c.side-1),mix(capture->class_seed+17389)&(c.side-1),capture->cap_frames};
            }
            if(cap_ledgers.ptr){
                bool adaptive=(h.cap_law.mode&capillary::contextual)&&cap_contexts.ptr;
                const char *kernel=adaptive?(cap_archive.ptr?"flood_capillary_context_film":"flood_capillary_context"):(cap_archive.ptr?"flood_capillary_film":"flood_capillaries");
                if(cap_experiences.ptr){
                    kernel=cap_archive.ptr?"flood_capillary_context_audit_film":"flood_capillary_context_audit";
                    capillary_field.context_sites=std::min(n,16384u);capillary_field.context_notes=reinterpret_cast<capillary::ContextNote*>(cap_experiences.ptr);
                    s.d.check(s.d.zero(cap_experiences.ptr,0,Count(capillary_field.context_sites)*2*sizeof(capillary::ContextNote)),"clear the sampled context interventions");
                }
                std::string expression=kernel;
                if(h.cap_law.mode&capillary::foraging_ports)expression+="_ports";
                if(cap_origins.ptr)expression+="_wake";
                kernel=expression.c_str();
                s.bind(surge_kernel,kernel,Image::floodplain);int regs=0,local=0;
                s.d.check(s.d.func_attr(&regs,4,surge_kernel),"measure pressure registers");s.d.check(s.d.func_attr(&local,3,surge_kernel),"measure pressure local memory");
                std::cout<<"context_kernel="<<kernel<<"  registers="<<regs<<"  local_bytes="<<local<<"  active_memory_bytes="<<(adaptive?sizeof(capillary::History):0)<<"  reserved_memory_bytes="<<(cap_contexts.ptr?sizeof(capillary::History):0)<<'\n';
                s.d.check(s.d.zero(cap_ledgers.ptr,0,128*sizeof(capillary::Ledger)),"clear pressure and feeding accounts");}
            if(cargo_totals.ptr){
                s.bind(cargo_kernel,cargo_notes.ptr?"flood_detritus_record":"flood_detritus",Image::floodplain);
                int registers=0,local=0;s.d.check(s.d.func_attr(&registers,4,cargo_kernel),"measure grain routing registers");
                s.d.check(s.d.func_attr(&local,3,cargo_kernel),"measure grain routing local memory");
                std::cout<<"detritus_kernel="<<(cargo_notes.ptr?"observed":"field")<<"  registers="<<registers<<"  local_bytes="<<local<<"  threads=128  sites_per_thread=4\n";

                s.d.check(s.d.zero(cargo_totals.ptr,0,128*sizeof(detritus::Ledger)),"clear detritus receipts");
                if(cargo_notes.ptr){
                    cargo_film={reinterpret_cast<detritus::Note*>(cargo_notes.ptr),std::min(512u,n/4),std::min(8192u,n/4)};
                    capillary_field.impulses=reinterpret_cast<Word*>(cargo_impulses.ptr);
                    s.d.check(s.d.zero(cargo_notes.ptr,0,Count(cargo_film.uniform+cargo_film.watched)*sizeof(detritus::Note)),"clear sediment samples");
                }
            }
            if(cap_notes.ptr)s.d.check(s.d.zero(cap_notes.ptr,0,Count(capillary_field.samples)*sizeof(capillary::Note)),"clear material intervention samples");
            synchronize();
        }
        if(h.braiding){
            Word innervation =
                (h.loom.mode & ~utterance::stored_vocal_feedback) |
                (h.drift_water.law.ports & drift::auditory ? regulation::auditory_port : 0u);
            world.braids = {reinterpret_cast<braid::Mind *>(riggers.ptr),
                            reinterpret_cast<braid::Half *>(floaters.ptr),
                            reinterpret_cast<braid::Totals *>(float_books.ptr),
                            innervation,
                            reinterpret_cast<braid::GateMemory *>(sluice_contacts.ptr),
                            reinterpret_cast<braid::GateBook *>(sluice_books.ptr),
                            h.gate_phase,
                            reinterpret_cast<braid::Episode *>(float_events.ptr),
                            reinterpret_cast<braid::SedimentBook *>(collar_books.ptr),reinterpret_cast<Count*>(trails.ptr)};
            if (trails.ptr) s.d.check(s.d.zero(trails.ptr,0,Count(n)*sizeof(utterance::Trail)),"clear natal visual histories");
            if (ink_books.ptr) {
                s.d.check(s.d.zero(ink_books.ptr,0,128*sizeof(utterance::Ledger)),"clear pigment accounts");
                if(ink_notes.ptr)s.d.check(s.d.zero(ink_notes.ptr,0,(Count(ink_slots)+ink_watch.ticks)*sizeof(utterance::Sample)),"clear recorded pigment transitions");
            }
            if(collar_books.ptr){
                s.d.check(s.d.zero(collar_books.ptr,0,Count((n/8+127)/128)*sizeof(braid::SedimentBook)),"clear mineral flow records");
                s.d.check(s.d.upload(collar_books.ptr,&h.sediment_book,sizeof(h.sediment_book)),"restore mineral flow records");
            }
            if(float_events.ptr)s.d.check(s.d.zero(float_events.ptr,0,Count(n)*sizeof(braid::Episode)),"clear unobserved event frames");
            if(sluice_books.ptr){
                s.bind(gated_flows,"braid_gates",Image::floodplain);s.d.check(s.d.zero(sluice_books.ptr,0,Count((n/8+127)/128)*sizeof(braid::GateBook)),"clear linked flows");
                s.d.check(s.d.upload(sluice_books.ptr,&h.sluice_book,sizeof(h.sluice_book)),"restore linked flows");
            }
            Function found{};s.bind(found,"braid_seed",Image::floodplain);s.bind(moorings,regulatory_books.ptr ? "braid_works_regulated" : "braid_works",Image::floodplain);
            if(regulatory_books.ptr)s.d.check(s.d.zero(regulatory_books.ptr,0,Count((n/4+127)/128)*sizeof(regulation::Ledger)),"clear interval motor feedback");
            launch(found,(n+127)/128,128,world,n,c.side,h.loom.seed);synchronize();
            s.d.check(s.d.zero(float_books.ptr,0,Count((n/4+127)/128)*sizeof(braid::Totals)),"clear float work");
            s.d.check(s.d.upload(float_books.ptr,&h.loom.book,sizeof(braid::Totals)),"restore float work");
        }
        if(h.rattling) {
            microphones={reinterpret_cast<rattle::Mind*>(rattlers.ptr),reinterpret_cast<rattle::Totals*>(rattle_books.ptr),h.backwater.mode,h.backwater.phase,reinterpret_cast<rattle::Attention*>(rattle_queries.ptr),reinterpret_cast<rattle::Load*>(mesh_book.ptr)};
            if(rattle_queries.ptr){s.d.check(s.d.zero(rattle_queries.ptr,0,Count((n/4+127)/128)*sizeof(rattle::Attention)),"clear acoustic inquiries");
                s.d.check(s.d.upload(rattle_queries.ptr,&h.queries,sizeof(h.queries)),"restore acoustic inquiries");}
            if(mesh_book.ptr){s.d.check(s.d.zero(mesh_book.ptr,0,Count((n/4+127)/128)*sizeof(rattle::Load)),"clear mesh returns");
                s.d.check(s.d.upload(mesh_book.ptr,&h.mesh,sizeof(h.mesh)),"restore mesh returns");}
            Function init{};s.bind(init,"rattle_seed",Image::floodplain);s.bind(bank_work,"rattle_banks",Image::floodplain);
            launch(init,(n+127)/128,128,rattlers.ptr,n,h.backwater.seed);synchronize();
            s.d.check(s.d.zero(rattle_books.ptr,0,Count((n/4+127)/128)*sizeof(rattle::Totals)),"clear bank records");
            s.d.check(s.d.upload(rattle_books.ptr,&h.backwater.total,sizeof(rattle::Totals)),"restore bank records");
        }
        if(h.wicking){
            world.wicks={reinterpret_cast<Word*>(parcels.ptr),reinterpret_cast<wicks::Totals*>(wick_ledgers.ptr),h.wicking_state.mode,n};
            s.d.check(s.d.zero(wick_ledgers.ptr,0,Count((n/2+127)/128)*sizeof(wicks::Totals)),"clear capillary observations");
            s.d.check(s.d.upload(wick_ledgers.ptr,&h.wicking_state.book,sizeof(wicks::Totals)),"restore capillary observations");
            Word seed=h.wicking_state.seed,found=h.liquid.empty();Function make{};s.bind(make,"wick_tracers",Image::floodplain);
            launch(make,(n+127)/128,128,world.wicks,n,seed,found);synchronize();
        }
        if(h.reeding){
            auto &r=world.wicks.reeds;r={reinterpret_cast<runnel::Mind*>(cutters.ptr),reinterpret_cast<Byte*>(folded.ptr),reinterpret_cast<runnel::Totals*>(reed_books.ptr),h.rush.mode,runnel::material_id(mix(h.rush.seed+9421),h.rush.mode&runnel::handed),h.rush.period,reinterpret_cast<runnel::Social*>(reed_effects.ptr)};
            s.d.check(s.d.zero(reed_effects.ptr,0,Count((n/2+127)/128)*sizeof(runnel::Social)),"clear attributed reed flows");
            s.d.check(s.d.upload(reed_effects.ptr,&h.reed_audit,sizeof(h.reed_audit)),"restore attributed flows");
            Function fn{};s.bind(fn,"reed_found",Image::runnels);Word seed=h.rush.seed;launch(fn,(n+127)/128,128,r,n,seed);synchronize();
            s.d.check(s.d.zero(reed_books.ptr,0,Count((n/2+127)/128)*sizeof(runnel::Totals)),"clear reed ledgers");
            s.d.check(s.d.upload(reed_books.ptr,&h.rush.book,sizeof(h.rush.book)),"restore reed ledgers");
        }
        if(h.practising){
            world.practice={reinterpret_cast<practice::Mind*>(apprentices.ptr),reinterpret_cast<practice::Context*>(contexts.ptr),
                reinterpret_cast<practice::Totals*>(practice_books.ptr),n,h.learning.arena,h.learning.mode,reinterpret_cast<practice::Mark*>(marked_strands.ptr),reinterpret_cast<practice::Credits*>(practice_credits.ptr)};
            s.d.check(s.d.zero(marked_strands.ptr,0,Count(n/4)*sizeof(practice::Mark)),"prepare public fibre positions");
            s.d.check(s.d.zero(practice_credits.ptr,0,Count((n/4+127)/128)*sizeof(practice::Credits)),"clear construction returns");
            s.d.check(s.d.upload(practice_credits.ptr,&h.legacy_work,sizeof(h.legacy_work)),"restore construction returns");
            Function init{};s.bind(init,"practice_found",Image::floodplain);Word seed=h.learning.seed;launch(init,(n+127)/128,128,world.practice,seed);synchronize();
            s.d.check(s.d.zero(practice_books.ptr,0,Count((n/4+127)/128)*sizeof(practice::Totals)),"clear attention accounts");
            s.d.check(s.d.upload(practice_books.ptr,&h.learning.book,sizeof(h.learning.book)),"restore attention accounts");
        }
        if(h.linking){
            world.linkage={reinterpret_cast<linkage::Mind*>(inventors.ptr),reinterpret_cast<linkage::Tissue*>(links.ptr),
                           reinterpret_cast<linkage::Provenance*>(patents.ptr),reinterpret_cast<linkage::Totals*>(link_books.ptr),n,h.millwork.arena,
                           h.millwork.mode,h.millwork.period,h.millwork.seed,reinterpret_cast<linkage::Material*>(link_layers.ptr)};
            Function init{};s.bind(init,"linkage_found",Image::floodplain);s.bind(work_links,"linkage_shop",Image::floodplain);
            launch(init,(n+127)/128,128,world.linkage,n,c.seed);synchronize();reset_links(h.millwork.totals);
        }
        if(h.appraising){
            world.outlay={reinterpret_cast<effort::Mind*>(cost_minds.ptr),reinterpret_cast<unsigned char*>(treads.ptr),
                          reinterpret_cast<effort::Ledger*>(cost_books.ptr),h.economics.mode,h.economics.period};
            Function born{};s.bind(born,"effort_found",Image::floodplain);launch(born,(n+127)/128,128,world.outlay,n,c.seed);synchronize();
            s.d.check(s.d.zero(cost_books.ptr,0,Count((n/2+127)/128)*sizeof(effort::Ledger)),"clear physical outlay accounts");
            s.d.check(s.d.upload(cost_books.ptr,&h.economics.ledger,sizeof(effort::Ledger)),"restore walking accounts");
        }
        if(h.ranging) {
            world.hollows={reinterpret_cast<hollow::Mind*>(path_brains.ptr),reinterpret_cast<Word*>(path_marks.ptr),
                           reinterpret_cast<Word*>(path_sites.ptr),reinterpret_cast<hollow::Bed*>(path_soil.ptr),
                           reinterpret_cast<hollow::Ledger*>(path_books.ptr),n,h.recess.slots,h.recess.mode};
            Function seed_places{};s.bind(seed_places,"hollow_found",Image::floodplain);
            launch(seed_places,(n+127)/128,128,world.hollows,c.seed,n);
            synchronize();reset_hollows(h.recess.book);
        }
        if (h.pottery) {
            world.clay = {reinterpret_cast<clay::Mind *>(clayers.ptr),
                          reinterpret_cast<clay::Tile *>(earthen.ptr),
                          reinterpret_cast<clay::Tile *>(palms.ptr),
                          reinterpret_cast<clay::Totals *>(clay_book.ptr),
                          h.workshop.use,
                          h.workshop.material};
            Function populate{};
            s.bind(populate, "clay_found", Image::siphons);
            launch(populate, (n + 127) / 128, 128, world.clay, c.seed, n);
            synchronize();
            reset_clay(h.workshop.book);
        }
        if (h.calling) {
            world.siphons = {reinterpret_cast<siphon::Animal *>(reed_animals.ptr),
                             reinterpret_cast<Byte *>(reed_ports.ptr),
                             reinterpret_cast<siphon::Totals *>(reed_ledger.ptr),
                             h.reeds.bands,
                             h.reeds.wire,
                             h.reeds.windows,
                             h.reeds.pulse};
            if (h.overhearing) {
                world.siphons.air = reinterpret_cast<siphon::Echo *>(airborne.ptr);
                world.siphons.audience = reinterpret_cast<siphon::Audience *>(audience_book.ptr);
                world.siphons.coupling = h.audience.coupling;
                world.siphons.access = h.audience.access;
                s.d.check(s.d.zero(airborne.ptr, 0, Count(n) * sizeof(siphon::Echo)), "still the new acoustic field");
                reset_audience(h.audience.total);
            }
            reset_siphons(h.reeds.total);
            Function fill{};
            s.bind(fill, "siphon_seed", Image::siphons);
            launch(fill, (n + 127) / 128, 128, world, c, previous_side);
            synchronize();
        }
        if (h.saline) {
            world.brine = {reinterpret_cast<brine::Salts *>(salts.ptr),
                           reinterpret_cast<brine::Animal *>(brine_tissue.ptr), h.solution.species, h.solution.flowing};
            Function fill{};
            s.bind(fill, "brine_land", Image::floodplain);
            launch(fill, (n + 127) / 128, 128, world, c, previous_side, h.founder_arbors);
            synchronize();
            if (h.salts.empty()) {
                h.salts.resize(n);
                s.d.check(s.d.download(h.salts.data(), salts.ptr, Count(n) * sizeof(brine::Salts)),
                          "measure initial solutes");
                auto initial = h.salt_inventory();
                std::copy(initial.begin(), initial.end(), h.solution.stock);
            }
        }
        if (h.sedimentary) {
            world.seams = {reinterpret_cast<Half *>(polymers.ptr),
                           reinterpret_cast<material::SedimentTotals *>(sediment_books.ptr), h.seam.pattern,
                           h.seam.guidance};
            s.d.check(s.d.zero(polymers.ptr, 0, Count(n) * sizeof(Half)), "expose unfilled carbon-binding sites");
            reset_sediment(h.seam.totals);
        }
        if (h.investigatory) {
            world.worksites = {reinterpret_cast<material::Inquiry *>(enquirers.ptr),
                               reinterpret_cast<material::Imprint *>(impressions.ptr),
                               reinterpret_cast<material::WorksiteTotals *>(worksite_books.ptr), h.worksite.active,
                               h.worksite.marks};
            reset_worksites(h.worksite.totals);
            s.d.check(s.d.zero(impressions.ptr, 0, Count(n) * sizeof(material::Imprint)), "clear unworked ground");
            Function install{};
            s.bind(install, "flood_inquiry_seed", Image::floodplain);
            launch(install, (n + 127) / 128, 128, world, c);
            synchronize();
        }
        if (h.foresight) {
            world.tide = {reinterpret_cast<material::Foresight *>(forecasts.ptr),
                          reinterpret_cast<material::TideTotals *>(tide_book.ptr),
                          reinterpret_cast<Half *>(tide_sights.ptr), h.tide.period, h.tide.active};
            s.bind(construction_tide, "flood_construction_tide", Image::floodplain);
            s.d.check(s.d.zero(forecasts.ptr, 0, Count(n) * sizeof(material::Foresight)),
                      "clear newborn predictive tissue");
            s.d.check(s.d.upload(tide_book.ptr, &h.tide.totals, sizeof(h.tide.totals)),
                      "restore the cost of visual replay");
        }
        if (h.articulated) {
            world.matter = {reinterpret_cast<material::Grain *>(mineral_bed.ptr),
                            reinterpret_cast<material::Worker *>(workers.ptr),
                            reinterpret_cast<material::Ledger *>(material_books.ptr),
                            reinterpret_cast<Byte *>(fixtures.ptr), h.minerals.physics};
            reset_material(h.minerals.account);
            if (!restore || previous_side || h.workers.empty()) {
                material::Genes law = h.hand_donor;
                Word graft = h.inherit_hands;
                if (!prior.empty() && !sensory) {
                    storage::AgentImage image;
                    image.read(prior);
                    if (image.handling) {
                        law = image.worker.genes;
                        graft = 1;
                    }
                }
                Function seed{};
                s.bind(seed, "flood_material_seed", Image::floodplain);
                launch(seed, (n + 127) / 128, 128, world, c, graft, law);
                synchronize();
            }
        }
        if (h.developing) {
            world.cortex = {reinterpret_cast<Cortex *>(cortex.ptr), reinterpret_cast<GrowthTotals *>(growth_books.ptr),
                            h.neuro.open, (h.neuro.cohort_controls >> 2) ? 1 + (h.neuro.cohort_controls >> 2) : 0,
                            h.neuro.cohort_controls & 3u};
            reset_growth(h.neuro.account);
        }
        if (h.enclosed) {
            world.pods = {reinterpret_cast<pods::Animal *>(pod_tissue.ptr),
                          reinterpret_cast<pods::Surface *>(rinds.ptr), reinterpret_cast<pods::Census *>(pod_book.ptr),
                          h.crop.physics};
            reset_pods(h.crop.account);
            if (!restore || previous_side || h.pod_animals.empty()) {
                Function init{};
                s.bind(init, "flood_pod_setup", Image::floodplain);
                launch(init, (n + 127) / 128, 128, world, c, h.pod_origin, h.pod_donor);
                synchronize();
            }
        }
        auto adopt = [&](bool graft, GrowthGenes law) {
            Function fn{};
            Word inherited = graft;
            s.bind(fn, "flood_cortex_adopt", Image::floodplain);
            launch(fn, (n + 127) / 128, 128, world, c, inherited, law);
            synchronize();
        };
        if (restore && !previous_side) {
            if (h.investigatory && h.enquiries.empty())
                rest_hands();
            transfer(h, true);
            if (h.developing && h.cortices.empty())
                adopt(false, {});
        } else {
            Germ germ;
            Word graft = 0;
            bool cortical_graft = false;
            GrowthGenes growth_law{};
            if (!prior.empty()) {
                storage::AgentImage subject;
                subject.read(prior);
                cortical_graft = subject.grows_cells && !sensory;
                growth_law = subject.cortex.genes;
                germ.brain = subject.brain.genes;
                germ.hearing = subject.grounded ? subject.referent.genes : subject.recollection.genes;
                graft = sensory ? 0 : 1;
                if (subject.riverine && !sensory) {
                    germ = subject.resident.germ;
                    graft = 4;
                } else if (subject.grounded) {
                    Word n = 0;
                    for (Word i = 1; i <= subject.referent.count && n < 8; ++i) {
                        auto &m = subject.referent;
                        auto &w = m.word[i];
                        if (!w.viable)
                            continue;
                        if (!std::equal(w.possible, w.possible + 8, w.certain))
                            continue;
                        if (std::has_single_bit(m.layout) && (m.fills[std::countr_zero(m.layout)] & (1ull << i)))
                            continue;
                        Word both = 255, any = 0, matches = 0;
                        for (Word x = 0; x < 256; ++x)
                            if (grounding::admits(w.certain, x)) {
                                both &= x;
                                any |= x;
                                ++matches;
                            }
                        if (!matches)
                            continue;
                        Word mask = (~any | both) & 255;
                        if ((1u << (8 - std::popcount(mask))) != matches)
                            continue;
                        germ.pigments[n / 2] |= (mask | (both << 8)) << (16 * (n & 1));
                        ++n;
                    }
                    if (n)
                        graft |= 2;
                }
            }
            if (sensory && !(graft & 2))
                throw std::invalid_argument("the sensory donor has no acquired visual categories");
            Temporary initial(s.d, sizeof(germ));
            s.d.check(s.d.upload(initial.ptr, &germ, sizeof(germ)), "introduce inherited developmental material");
            auto newborn = c;
            if (previous_side)
                newborn.density = 0;
            launch(seed, (n + 127) / 128, 128, world, newborn, initial.ptr, graft, tick);
            synchronize();
            if (h.developing && cortical_graft)
                adopt(true, growth_law);
            if (previous_side) {
                Temporary old_land(s.d, h.land.size() * sizeof(Silt)), old_slots(s.d, h.slots.size() * sizeof(Word));
                auto up = [&](DevicePtr dst, const auto &v) {
                    s.d.check(s.d.upload(dst, v.data(), v.size() * sizeof(v[0])), "retain established floodplain");
                };
                up(old_land.ptr, h.land);
                up(old_slots.ptr, h.slots);
                up(dna.ptr, h.genomes);
                up(animals.ptr, h.bodies);
                move_neural(h.nerves.data(), nullptr, 0, h.nerves.size());
                up(ears.ptr, h.ears);
                if(h.braiding&&!h.riggers.empty()){
                    up(riggers.ptr,h.riggers);if(!h.float_events.empty())up(float_events.ptr,h.float_events);if(!h.sluice_contacts.empty())up(sluice_contacts.ptr,h.sluice_contacts);
                    if (trails.ptr && !h.trails.empty()) up(trails.ptr,h.trails);
                    if(cap_neurons.ptr)extend_capillaries(h,previous_side);
                    for(Word row=0;row<previous_side;++row)
                        s.d.check(s.d.upload(floaters.ptr+Count(row)*c.side*2,h.floats.data()+Count(row)*previous_side,2*previous_side),"extend occupied float rows");
                }
                if(h.rattling&&!h.listeners.empty())up(rattlers.ptr,h.listeners);
                if(h.reeding&&!h.cutters.empty()){
                    up(cutters.ptr,h.cutters);
                    std::vector<Byte> broad(n);for(Word i=0;i<h.folds.size();++i)broad[i/previous_side*c.side+i%previous_side]=h.folds[i];
                    up(folded.ptr,broad);
                }
                if(h.wicking && !h.liquid.empty()){
                    std::vector<Word> enlarged(n);for(Word i=0;i<h.liquid.size();++i){auto tag=h.liquid[i];
                        if(tag&wicks::present){Word old=wicks::origin(tag);tag=(tag&~wicks::trail)|wicks::label((old/previous_side)*c.side+old%previous_side,wicks::hops(tag));}
                        enlarged[(i/previous_side)*c.side+i%previous_side]=tag;}
                    up(parcels.ptr,enlarged);
                }
                if(h.practising && !h.pupils.empty()){
                    auto old=h.pupils;for(auto &m:old)if(m.focus){Word tile=(m.focus>>12)-1;Word moved=(tile/(previous_side/2))*(c.side/2)+tile%(previous_side/2);
                        m.focus=((moved+1)<<12)|(m.focus&4095);}
                    std::vector<practice::Mark> wider(n/4);
                    for(Word tile=0;tile<h.strands.size();++tile)wider[(tile/(previous_side/2))*(c.side/2)+tile%(previous_side/2)]=h.strands[tile];
                    up(marked_strands.ptr,wider);up(apprentices.ptr,old);Word small=previous_side*previous_side;
                    for(Word j=0;j<h.learning.arena;++j)s.d.check(s.d.upload(contexts.ptr+Count(j)*n*sizeof(practice::Context),h.memories.data()+Count(j)*small,Count(small)*sizeof(practice::Context)),"expand material hypotheses");
                }
                if(h.linking && !h.makers.empty()){
                    up(inventors.ptr,h.makers);Word small=previous_side*previous_side;
                    for(Word j=0;j<h.millwork.arena;++j)s.d.check(s.d.upload(links.ptr+Count(j)*n*sizeof(linkage::Tissue),h.mechanisms.data()+Count(j)*small,Count(small)*sizeof(linkage::Tissue)),"expand private linkage banks");
                    Temporary prior(s.d,h.seals.size()*sizeof(linkage::Provenance));up(prior.ptr,h.seals);
                    Function enlarge{};s.bind(enlarge,"linkage_widen",Image::floodplain);
                    launch(enlarge,(small/4+127)/128,128,world.linkage,prior.ptr,c.side,previous_side);synchronize();
                }
                if(h.appraising && !h.economies.empty()){
                    up(cost_minds.ptr,h.economies);Temporary old_bed(s.d,h.treads.size());up(old_bed.ptr,h.treads);
                    Function widen{};s.bind(widen,"effort_widen",Image::floodplain);
                    launch(widen,(previous_side*previous_side+127)/128,128,treads.ptr,old_bed.ptr,c.side,previous_side);synchronize();
                }
                if(h.ranging && !h.wanderers.empty()) {
                    up(path_brains.ptr,h.wanderers);
                    Word prior=previous_side*previous_side;
                    for(Word j=0;j<h.recess.slots;++j)
                        s.d.check(s.d.upload(path_marks.ptr+Count(j)*n*4,h.bearings.data()+Count(j)*prior,Count(prior)*4),"widen spatial tissue");
                    Temporary old_beds(s.d,Count(prior)*sizeof(hollow::Bed)),old_places(s.d,h.sightings.size()*sizeof(Word));
                    up(old_beds.ptr,h.hollows);up(old_places.ptr,h.sightings);
                    Function widen{};s.bind(widen,"hollow_widen",Image::floodplain);
                    launch(widen,(prior+127)/128,128,world.hollows,old_beds.ptr,old_places.ptr,c.side,previous_side);
                    synchronize();
                }
                if (h.pottery && !h.potters.empty()) {
                    up(clayers.ptr, h.potters);
                    up(palms.ptr, h.cargo);
                    Temporary old(s.d, Count(h.ware.size()) * sizeof(clay::Tile));
                    up(old.ptr, h.ware);
                    Function place{};
                    s.bind(place, "clay_widen", Image::siphons);
                    launch(place, (previous_side * previous_side + 127) / 128, 128, earthen.ptr, old.ptr, c.side,
                           previous_side);
                    synchronize();
                }
                if (h.overhearing && !h.acoustics.empty()) {
                    Temporary old(s.d, Count(h.acoustics.size()) * sizeof(siphon::Echo));
                    up(old.ptr, h.acoustics);
                    Function extend{};
                    s.bind(extend, "siphon_echo_extend", Image::siphons);
                    launch(extend, (previous_side * previous_side + 127) / 128, 128, airborne.ptr, old.ptr, c.side,
                           previous_side);
                    synchronize();
                }
                if (h.calling && !h.callers.empty()) {
                    up(reed_animals.ptr, h.callers);
                    Temporary old(s.d, h.apertures.size());
                    up(old.ptr, h.apertures);
                    Function extend{};
                    s.bind(extend, "siphon_extend", Image::siphons);
                    launch(extend, (previous_side * previous_side + 127) / 128, 128, reed_ports.ptr, old.ptr, c.side,
                           previous_side);
                    synchronize();
                }
                if (h.articulated) {
                    up(workers.ptr, h.workers);
                    Temporary old(s.d, Count(previous_side) * previous_side * sizeof(material::Grain));
                    up(old.ptr, h.mineral_bed);
                    Function enlarge{};
                    s.bind(enlarge, "flood_material_extend", Image::floodplain);
                    launch(enlarge, (previous_side * previous_side + 127) / 128, 128, mineral_bed.ptr, old.ptr, c.side,
                           previous_side);
                    synchronize();
                }
                if (h.saline) {
                    up(brine_tissue.ptr, h.saline_animals);
                    Temporary old(s.d, Count(previous_side) * previous_side * sizeof(brine::Salts));
                    up(old.ptr, h.salts);
                    Function widen{};
                    s.bind(widen, "brine_extend", Image::floodplain);
                    launch(widen, (previous_side * previous_side + 127) / 128, 128, salts.ptr, old.ptr, c.side,
                           previous_side);
                    synchronize();
                }
                if (h.sedimentary && !h.macromolecules.empty()) {
                    Temporary old(s.d, Count(previous_side) * previous_side * sizeof(Half));
                    up(old.ptr, h.macromolecules);
                    Function expand{};
                    s.bind(expand, "flood_sediment_extend", Image::floodplain);
                    launch(expand, (previous_side * previous_side + 127) / 128, 128, polymers.ptr, old.ptr, c.side,
                           previous_side);
                    synchronize();
                }
                if (h.investigatory && !h.enquiries.empty()) {
                    up(enquirers.ptr, h.enquiries);
                    restore_posture(h.bodies);
                    Temporary old(s.d, Count(previous_side) * previous_side * sizeof(material::Imprint));
                    up(old.ptr, h.patina);
                    Function widen{};
                    s.bind(widen, "flood_imprint_extend", Image::floodplain);
                    launch(widen, (previous_side * previous_side + 127) / 128, 128, impressions.ptr, old.ptr, c.side,
                           previous_side);
                    synchronize();
                }
                if (h.foresight && !h.forecasts.empty())
                    up(forecasts.ptr, h.forecasts);
                if (h.developing)
                    up(cortex.ptr, h.cortices);
                if (h.enclosed) {
                    up(pod_tissue.ptr, h.pod_animals);
                    Temporary old_skin(s.d, h.rinds.size() * sizeof(pods::Surface));
                    up(old_skin.ptr, h.rinds);
                    Function stretch{};
                    s.bind(stretch, "flood_pod_extend", Image::floodplain);
                    launch(stretch, (previous_side * previous_side + 127) / 128, 128, rinds.ptr, old_skin.ptr,
                           previous_side, c.side);
                    synchronize();
                }
                Function expand{};
                s.bind(expand, "flood_extend", Image::floodplain);
                launch(expand, (n + 127) / 128, 128, world, old_land.ptr, old_slots.ptr, previous_side, c.side);
                synchronize();
            }
        }
        school=attendance;
        if(school && school->size)s.bind(teach_shore,"shore_lesson",Image::floodplain);
        std::vector<Word>().swap(neural_layout.initial);
        // CUDA Graph instantiation freezes kernel arguments, including the sample buffer.
        if (sample_motion)
            sample_tracers(*sample_motion);
        s.d.check(gpu.begin(stream, 0), "capture the floodplain cycle");
        cap_scheduled=!capture||capture->cap_schedule;
        cap_mask=cap_ledgers.ptr&&cap_scheduled?((1u<<h.cap_law.shift)-1):0;
        cap_graph_mask=std::min(cap_mask,15u);
        // A captured cycle repeats every sixteen ticks. For longer hydraulic periods
        // the one candidate frame still checks the full device clock.
        for (Word j = 0; j < 16; ++j){
            bool due=((tick+j)&cap_graph_mask)==0;
            frame(j,due);cap_graph_nodes+=due&&cap_ledgers.ptr;
        }
        Word block = 16;
        launch(time, 1, 1, clock.ptr, block);
        s.d.check(gpu.end(stream, &graph), "close floodplain cycle");
        s.d.check(gpu.instantiate(&batch, graph, 0), "compile floodplain cycle");
        drift_clock = drift_timer;
    }
    ~Basin() {
        if (fence)
            s.d.event_free(fence);
        if (batch)
            gpu.destroy_exec(batch);
        if (graph)
            gpu.destroy(graph);
        if (stream)
            gpu.destroy_stream(stream);
    }
    void reset_sediment(const material::SedimentTotals &q) {
        if (!sediment_books.ptr)
            return;
        s.d.check(s.d.zero(sediment_books.ptr, 0, ((n / 2 + 127) / 128) * sizeof(q)), "clear sediment accounts");
        s.d.check(s.d.upload(sediment_books.ptr, &q, sizeof(q)), "restore sediment exchange");
    }
    void reset_worksites(const material::WorksiteTotals &totals) {
        if (!worksite_books.ptr)
            return;
        s.d.check(s.d.zero(worksite_books.ptr, 0, ((n / 2 + 127) / 128) * sizeof(totals)), "clear the site ledgers");
        s.d.check(s.d.upload(worksite_books.ptr, &totals, sizeof(totals)), "restore pressures and observations");
    }
    void restore_posture(const std::vector<Colony> &bodies) {
        // Each body stores the exact preceding motor volley used when reading tool marks.
        std::vector<Byte> previous(bodies.size());
        std::transform(bodies.begin(), bodies.end(), previous.begin(), [](const Colony &a) { return a.action; });
        s.d.check(s.d.upload(motors.ptr, previous.data(), previous.size()), "restore proprioceptive feedback");
    }
    void springs(Habitat &h, Word previous) {
        Function spring{};
        s.bind(spring, "brine_springs", Image::floodplain);
        launch(spring, (n + 127) / 128, 128, world, c, previous);
        synchronize();
        h.salts.resize(n);
        s.d.check(s.d.download(h.salts.data(), salts.ptr, Count(n) * sizeof(brine::Salts)),
                  "measure solutes from the new springs");
        auto stock = h.salt_inventory();
        for (Word i = 0; i < previous; ++i)
            if (stock[i] != h.solution.stock[i])
                throw std::logic_error("springs changed an existing ion inventory");
        std::copy(stock.begin(), stock.end(), h.solution.stock);
        std::cout << "brine_springs=" << previous << "  components=" << h.solution.species << '\n';
    }
    void rest_hands() { s.d.check(s.d.zero(motors.ptr, 0, n), "start the first observation at rest"); }
    void wash_sites() {
        if (impressions.ptr)
            s.d.check(s.d.zero(impressions.ptr, 0, Count(n) * sizeof(material::Imprint)), "wash surface traces");
    }
    void reset_audience(const siphon::Audience &prior) {
        if (!audience_book.ptr)
            return;
        s.d.check(s.d.zero(audience_book.ptr, 0, Count((n / 2 + 127) / 128) * sizeof(prior)), "clear hearing ledgers");
        s.d.check(s.d.upload(audience_book.ptr, &prior, sizeof(prior)), "restore hearing ledgers");
    }
    void reset_hollows(const hollow::Ledger &prior) {
        if (!path_books.ptr) return;
        s.d.check(s.d.zero(path_books.ptr,0,Count((n/2+127)/128)*sizeof(prior)),"clear spatial accounts");
        s.d.check(s.d.upload(path_books.ptr,&prior,sizeof(prior)),"restore spatial accounts");
    }
    void reset_clay(const clay::Totals &prior) {
        if (!clay_book.ptr)
            return;
        s.d.check(s.d.zero(clay_book.ptr, 0, Count((n / 2 + 127) / 128) * sizeof(prior)), "clear clay budgets");
        s.d.check(s.d.upload(clay_book.ptr, &prior, sizeof(prior)), "restore clay budgets");
    }
    void wash_clay() {
        if (earthen.ptr) {
            s.d.check(s.d.zero(earthen.ptr, 0, Count(n) * sizeof(clay::Tile)), "smooth every ground grain");
            s.d.check(s.d.zero(palms.ptr, 0, Count(n) * sizeof(clay::Tile)), "smooth every carried grain");
        }
    }
    void reset_siphons(const siphon::Totals &prior) {
        if (!reed_ledger.ptr)
            return;
        s.d.check(s.d.zero(reed_ledger.ptr, 0, Count((n / 2 + 127) / 128) * sizeof(prior)),
                  "clear social foraging accounts");
        s.d.check(s.d.upload(reed_ledger.ptr, &prior, sizeof(prior)), "restore social foraging accounts");
    }
    void wash_sounds() {
        if (airborne.ptr)
            s.d.check(s.d.zero(airborne.ptr, 0, Count(n) * sizeof(siphon::Echo)), "damp the material resonances");
    }
    void rebed(const Habitat &donor) {
        if(world.outlay.minds){
            if(donor.appraising)s.d.check(s.d.upload(treads.ptr,donor.treads.data(),n),"transplant actual compacted ground");
            else s.d.check(s.d.zero(treads.ptr,0,n),"replace unworked ground");
        }
        if(path_soil.ptr) {
            if(donor.ranging) s.d.check(s.d.upload(path_soil.ptr,donor.hollows.data(),Count(n)*sizeof(hollow::Bed)),"transplant cavities");
            else s.d.check(s.d.zero(path_soil.ptr,0,Count(n)*sizeof(hollow::Bed)),"clear untransplanted cavities");
        }
        if (earthen.ptr) {
            std::vector<clay::Tile> tile(n);
            if (donor.pottery) {
                tile = donor.ware;
                for (auto &t : tile)
                    if (clay::size(t) && t.changed != ~0u) {
                        Word age = donor.header.tick - t.changed;
                        t.changed = age > tick ? ~0u : tick - age;
                    }
            }
            s.d.check(s.d.upload(earthen.ptr, tile.data(), Count(n) * sizeof(tile[0])),
                      "place the surviving ceramic objects");
        }

        if (airborne.ptr) {
            if (donor.overhearing) {
                auto impressions = donor.acoustics;
                for (auto &e : impressions)
                    if (e.pulse) {
                        Word age = siphon::daylight(e, donor.header.tick);
                        if (age >= siphon::tail(e.pulse))
                            e = {};
                        else
                            e.tick = tick - age;
                    }
                s.d.check(s.d.upload(airborne.ptr, impressions.data(), Count(n) * sizeof(impressions[0])),
                          "retain the remaining ring of older material");
            } else
                s.d.check(s.d.zero(airborne.ptr, 0, Count(n) * sizeof(siphon::Echo)),
                          "introduce quiet upstream ground");
        }
        if (reed_ports.ptr && donor.calling)
            s.d.check(s.d.upload(reed_ports.ptr, donor.apertures.data(), n),
                      "transplant actual porous valve orientations");
        if (salts.ptr) {
            if (donor.saline)
                s.d.check(s.d.upload(salts.ptr, donor.salts.data(), Count(n) * sizeof(brine::Salts)),
                          "transplant the dissolved components");
            else
                s.d.check(s.d.zero(salts.ptr, 0, Count(n) * sizeof(brine::Salts)), "place mineral-free water");
        }
        if (polymers.ptr) {
            if (donor.sedimentary)
                s.d.check(s.d.upload(polymers.ptr, donor.macromolecules.data(), Count(n) * sizeof(Half)),
                          "transplant held carbon");
            else
                s.d.check(s.d.zero(polymers.ptr, 0, Count(n) * sizeof(Half)), "clear a transplanted deposit");
        }
        if (!impressions.ptr)
            return;
        if (!donor.investigatory) {
            wash_sites();
            return;
        }
        auto traces = donor.patina;
        for (auto &f : traces)
            if (f.events && f.tick != ~0u) {
                Word age = donor.header.tick - f.tick;
                f.tick = age > tick ? ~0u : tick - age;
            }
        s.d.check(s.d.upload(impressions.ptr, traces.data(), traces.size() * sizeof(traces[0])),
                  "place the donor's time-stamped surface traces");
    }
    void reset_material(const material::Ledger &prior) {
        if (!material_books.ptr)
            return;
        s.d.check(s.d.zero(material_books.ptr, 0, ((n / 2 + 127) / 128) * sizeof(prior)), "clear mineral flux");
        s.d.check(s.d.upload(material_books.ptr, &prior, sizeof(prior)), "restore material accounts");
    }
    void replace_minerals(const Habitat &soil) {
        if (!mineral_bed.ptr)
            return;
        if (soil.articulated)
            s.d.check(s.d.upload(mineral_bed.ptr, soil.mineral_bed.data(), Count(n) * sizeof(material::Grain)),
                      "replace geological stocks");
        else
            s.d.check(s.d.zero(mineral_bed.ptr, 0, Count(n) * sizeof(material::Grain)),
                      "replace geological stocks with bare soil");
    }
    void reset_growth(const GrowthTotals &prior) {
        if (!growth_books.ptr)
            return;
        s.d.check(s.d.zero(growth_books.ptr, 0, ((n / 2 + 127) / 128) * sizeof(prior)), "clear neural census");
        s.d.check(s.d.upload(growth_books.ptr, &prior, sizeof(prior)), "restore neural growth accounts");
    }
    void reset_pods(const pods::Census &prior) {
        if (!pod_book.ptr)
            return;
        s.d.check(s.d.zero(pod_book.ptr, 0, ((n / 2 + 127) / 128) * sizeof(pods::Census)), "clear crop ledger");
        s.d.check(s.d.upload(pod_book.ptr, &prior, sizeof(prior)), "restore crop ledger");
    }
    void clear_pods() {
        Function erase{};
        s.bind(erase, "flood_pod_erase", Image::floodplain);
        launch(erase, (n + 127) / 128, 128, pod_tissue.ptr, n);
        synchronize();
    }
    void reset_account(const Counters &prior) {
        s.d.check(s.d.zero(counters.ptr, 0, ((n / 2 + 127) / 128) * sizeof(Counters)), "clear the sharded budgets");
        s.d.check(s.d.upload(counters.ptr, &prior, sizeof(Counters)), "restore the matter ledger");
    }
    void sum_account(DevicePtr shards, DevicePtr out) {
        Word groups = (n / 2 + 127) / 128;
        launch(totals, (sizeof(Counters) / sizeof(Count) + 31) / 32, 32, shards, groups, out);
    }
    void transfer(Habitat &h, bool upload) {
        move_capillaries(h,upload);
        if(h.braiding && (!upload||!h.riggers.empty())){
            if(!upload){h.riggers.resize(n);h.floats.resize(n);}
            auto move=[&](DevicePtr ptr,auto &v){size_t bytes=v.size()*sizeof(v[0]);s.d.check(upload?s.d.upload(ptr,v.data(),bytes):s.d.download(v.data(),ptr,bytes),"transfer float memories and material");};
            if(float_events.ptr && (!upload||!h.float_events.empty())){if(!upload)h.float_events.resize(n);move(float_events.ptr,h.float_events);}
            if(trails.ptr && (!upload||!h.trails.empty())){if(!upload)h.trails.resize(n);move(trails.ptr,h.trails);}
            move(riggers.ptr,h.riggers);move(floaters.ptr,h.floats);
            if(sluice_contacts.ptr && (!upload||!h.sluice_contacts.empty())){
                if(!upload)h.sluice_contacts.resize(n);
                move(sluice_contacts.ptr,h.sluice_contacts);
            }
        }
        if (h.rattling && (!upload || !h.listeners.empty())) {
            if (!upload) h.listeners.resize(n);
            size_t bytes=Count(n)*sizeof(rattle::Mind);
            s.d.check(upload?s.d.upload(rattlers.ptr,h.listeners.data(),bytes):s.d.download(h.listeners.data(),rattlers.ptr,bytes),"transfer bank contacts");
        }
        if(h.reeding&&(!upload||!h.cutters.empty())){
            if(!upload){h.cutters.resize(n);h.folds.resize(n);}
            auto move=[&](DevicePtr ptr,auto &v){size_t z=v.size()*sizeof(v[0]);s.d.check(upload?s.d.upload(ptr,v.data(),z):s.d.download(v.data(),ptr,z),"transfer folding circuits and seams");};
            move(cutters.ptr,h.cutters);move(folded.ptr,h.folds);
        }
        if(h.wicking&&!h.liquid.empty())s.d.check(upload?s.d.upload(parcels.ptr,h.liquid.data(),Count(n)*4):s.d.download(h.liquid.data(),parcels.ptr,Count(n)*4),"transfer capillary labels");
        if(h.practising && !h.pupils.empty()){
            auto move=[&](DevicePtr ptr,auto &x){size_t bytes=x.size()*sizeof(x[0]);s.d.check(upload?s.d.upload(ptr,x.data(),bytes):s.d.download(x.data(),ptr,bytes),"transfer private practice states");};
            move(apprentices.ptr,h.pupils);move(contexts.ptr,h.memories);move(marked_strands.ptr,h.strands);
        }
        if(h.linking && !h.makers.empty()){
            auto copy=[&](DevicePtr p,auto &a){size_t bytes=a.size()*sizeof(a[0]);s.d.check(upload?s.d.upload(p,a.data(),bytes):s.d.download(a.data(),p,bytes),"transfer linkages and their actual makers");};
            copy(inventors.ptr,h.makers);copy(links.ptr,h.mechanisms);copy(patents.ptr,h.seals);
        }
        if(h.appraising && !h.economies.empty()){
            auto copy=[&](DevicePtr dst,auto &v){size_t bytes=v.size()*sizeof(v[0]);s.d.check(upload?s.d.upload(dst,v.data(),bytes):s.d.download(v.data(),dst,bytes),"transfer private effort contacts and public ground");};
            copy(cost_minds.ptr,h.economies);copy(treads.ptr,h.treads);
        }
        if(h.ranging && !h.wanderers.empty()) {
            auto copy=[&](DevicePtr p,auto &v){size_t bytes=v.size()*sizeof(v[0]);s.d.check(upload?s.d.upload(p,v.data(),bytes):s.d.download(v.data(),p,bytes),"transfer spatial tissues and cavities");};
            copy(path_brains.ptr,h.wanderers);copy(path_marks.ptr,h.bearings);copy(path_sites.ptr,h.sightings);copy(path_soil.ptr,h.hollows);
        }
        if (!upload)
            h.allocate();
        std::array<DevicePtr, 6> port{land.ptr, slots.ptr, dna.ptr, animals.ptr, nerves.ptr, ears.ptr};
        Word i = 0;
        h.arrays([&](auto &v) {
            size_t bytes = v.size() * sizeof(v[0]);
            auto p = port[i++];
            if constexpr (std::is_same_v<typename std::decay_t<decltype(v)>::value_type, Tissue>) {
                move_neural(upload ? v.data() : nullptr, upload ? nullptr : v.data(), 0, v.size(), h.genomes.data());
                return;
            }
            s.d.check(upload ? s.d.upload(p, v.data(), bytes) : s.d.download(v.data(), p, bytes),
                      "transfer complete floodplain");
        });
        if (h.enclosed && !h.pod_animals.empty()) {
            auto port = [&](DevicePtr ptr, auto &v) {
                auto bytes = v.size() * sizeof(v[0]);
                s.d.check(upload ? s.d.upload(ptr, v.data(), bytes) : s.d.download(v.data(), ptr, bytes),
                          "transfer delayed feeding state");
            };
            port(pod_tissue.ptr, h.pod_animals);
            port(rinds.ptr, h.rinds);
        }
        if (h.articulated && !h.workers.empty()) {
            auto move = [&](DevicePtr dst, auto &v) {
                size_t bytes = v.size() * sizeof(v[0]);
                s.d.check(upload ? s.d.upload(dst, v.data(), bytes) : s.d.download(v.data(), dst, bytes),
                          "transfer the mineral field");
            };
            move(mineral_bed.ptr, h.mineral_bed);
            move(workers.ptr, h.workers);
        }
        if (h.pottery && !h.potters.empty()) {
            auto move = [&](DevicePtr dest, auto &v) {
                Count bytes = Count(v.size()) * sizeof(v[0]);
                s.d.check(upload ? s.d.upload(dest, v.data(), bytes) : s.d.download(v.data(), dest, bytes),
                          "transfer portable objects and latent neural states");
            };
            move(clayers.ptr, h.potters);
            move(earthen.ptr, h.ware);
            move(palms.ptr, h.cargo);
        }
        if (h.overhearing && !h.acoustics.empty()) {
            auto bytes = Count(h.acoustics.size()) * sizeof(siphon::Echo);
            s.d.check(upload ? s.d.upload(airborne.ptr, h.acoustics.data(), bytes)
                             : s.d.download(h.acoustics.data(), airborne.ptr, bytes),
                      "transfer traveling waves");
        }
        if (h.calling && !h.callers.empty()) {
            auto move = [&](DevicePtr dest, auto &v) {
                auto size = v.size() * sizeof(v[0]);
                s.d.check(upload ? s.d.upload(dest, v.data(), size) : s.d.download(v.data(), dest, size),
                          "transfer social foragers and their gates");
            };
            move(reed_animals.ptr, h.callers);
            move(reed_ports.ptr, h.apertures);
        }
        if (h.saline && !h.saline_animals.empty()) {
            auto move = [&](DevicePtr dst, auto &v) {
                auto bytes = v.size() * sizeof(v[0]);
                s.d.check(upload ? s.d.upload(dst, v.data(), bytes) : s.d.download(v.data(), dst, bytes),
                          "transfer brine and receptive tissue");
            };
            move(salts.ptr, h.salts);
            move(brine_tissue.ptr, h.saline_animals);
        }
        if (h.sedimentary && !h.macromolecules.empty()) {
            auto bytes = Count(n) * sizeof(Half);
            s.d.check(upload ? s.d.upload(polymers.ptr, h.macromolecules.data(), bytes)
                             : s.d.download(h.macromolecules.data(), polymers.ptr, bytes),
                      "transfer carbon reservoir");
        }
        if (h.investigatory && !h.enquiries.empty()) {
            auto transfer = [&](auto &v, DevicePtr ptr) {
                size_t bytes = v.size() * sizeof(v[0]);
                s.d.check(upload ? s.d.upload(ptr, v.data(), bytes) : s.d.download(v.data(), ptr, bytes),
                          "transfer the full developmental habitat");
            };
            if (upload)
                restore_posture(h.bodies);
            transfer(h.enquiries, enquirers.ptr);
            transfer(h.patina, impressions.ptr);
        }
        if (h.foresight && !h.forecasts.empty()) {
            size_t bytes = h.forecasts.size() * sizeof(h.forecasts[0]);
            s.d.check(upload ? s.d.upload(forecasts.ptr, h.forecasts.data(), bytes)
                             : s.d.download(h.forecasts.data(), forecasts.ptr, bytes),
                      "transfer predictive tissue");
        }
        if (h.developing && !h.cortices.empty()) {
            size_t bytes = h.cortices.size() * sizeof(Cortex);
            s.d.check(upload ? s.d.upload(cortex.ptr, h.cortices.data(), bytes)
                             : s.d.download(h.cortices.data(), cortex.ptr, bytes),
                      "transfer the active spiking populations");
        }
    }
    void step(Word steps, bool trace = false) {
        Word window = trace ? std::min(16u, steps) : 0, left = steps - window;
        Word pending =
            std::max(drift_clock ? drift_clock->remaining() : 0u, optical_clock ? optical_clock->remaining() : 0u);
        Word now=tick;
        auto live_frame=[&](Word offset){
            bool due=((now+offset)&cap_mask)==0;
            frame(offset,due);cap_launches+=due&&cap_ledgers.ptr;
        };
        auto direct=[&](Word count){
            for(Word j=0;j<count;++j)live_frame(j);
            launch(time,1,1,clock.ptr,count);collect_after(count);now+=count;
        };
        if(pending&&left){Word measured=std::min(left,pending);direct(measured);left-=measured;}
        Word align=(cap_start-now)&cap_graph_mask;
        if(left>=16 && align){direct(align);left-=align;}
        while(left>=16){
            s.d.check(gpu.launch(batch,stream),"advance floodplain cycle");
            collect_after(16);cap_launches+=cap_graph_nodes;now+=16;left-=16;
        }
        if(left)direct(left);
        if (window) {
            Function mark{}, track{};
            s.bind(mark, "flood_mark", Image::floodplain);
            s.bind(track, "flood_track", Image::floodplain);
            Temporary tags(s.d, 16 * sizeof(Word)), positions(s.d, 8 * (window + 1) * sizeof(Word));
            s.d.check(s.d.zero(positions.ptr, 255, 8 * (window + 1) * sizeof(Word)),
                      "clear the photographic exposure");
            launch(mark, 1, 32, world, c, tags.ptr);
            for (Word i = 0; i <= window; ++i) {
                DevicePtr view = positions.ptr + 8 * i * sizeof(Word);
                launch(track, (n + 127) / 128, 128, world, c, tags.ptr, view);
                if (i < window)
                    live_frame(i);
            }
            launch(time, 1, 1, clock.ptr, window);
            collect_after(window);
            synchronize();
            std::vector<Word> locations(8 * (window + 1));
            Word owners[16];
            s.d.check(s.d.download(locations.data(), positions.ptr, locations.size() * sizeof(Word)),
                      "read the final movement exposure");
            s.d.check(s.d.download(owners, tags.ptr, sizeof(owners)), "read photographic subject identities");
            for (Word k = 0; k < 8; ++k) {
                if (owners[2 * k] == ~0u)
                    continue;
                std::ostringstream record;
                record << "watershed_trail=" << owners[2 * k] << "  born=" << owners[2 * k + 1] << "  side=" << c.side
                       << "  ticks=" << window << "  positions=";
                for (Word i = 0; i <= window; ++i) {
                    if (locations[8 * i + k] == ~0u)
                        break;
                    record << (i ? "," : "") << locations[8 * i + k];
                }
                journal::current()->line(record.str());
            }
        } else
            synchronize();
        tick += steps;
    }
    void house(const storage::AgentImage &image, Habitat &h) {
        if (!image.riverine || image.brain.tiles > tiles)
            throw std::invalid_argument("neural arena is too small for this resident");
        sample(h);
        Word i = 0;
        while (i < n && (h.bodies[h.slots[i]].energy || (h.articulated && h.workers[h.slots[i]].hand.faces)))
            ++i;
        if (i == n)
            throw std::invalid_argument("the habitat has no vacant site for the resident");
        Word slot = h.slots[i];
        auto a = image.resident.animal;
        // The colony begins a new local genealogy. The original slot and
        // clock are retained in the portable individual.
        a.founder = slot;
        a.body_origin = a.brain_origin = slot;
        a.cohort = 0;
        auto up = [&](DevicePtr target, const void *value, size_t bytes) {
            s.d.check(s.d.upload(target, value, bytes), "introduce the marked resident");
        };
        up(dna.ptr + slot * sizeof(Germ), &image.resident.germ, sizeof(Germ));
        up(animals.ptr + slot * sizeof(Colony), &a, sizeof(Colony));
        if (enquirers.ptr)
            up(motors.ptr + slot, &a.action, sizeof(a.action));
        move_neural(image.brain.tissue, nullptr, Count(slot) * tiles, image.brain.tiles);
        up(ears.ptr + slot * sizeof(Ear), &image.resident.ear, sizeof(Ear));
        if (world.cortex.resident) {
            auto state = image.grows_cells ? image.cortex
                                           : hatch_cortex(growth_founder(mix(c.seed + slot + 19561)),
                                                          living_tiles(image.brain.tiles, image.resident.germ.brain.development));
            up(cortex.ptr + Count(slot) * sizeof(Cortex), &state, sizeof(state));
            if (state.active < c.tiles) {
                std::array<Tissue, 32> empty{};
                move_neural(empty.data(), nullptr, Count(slot) * tiles + state.active, tiles - state.active);
            }
        }
        if (image.investigates && image.grounded && image.referent.count)
            h.spoken.push_back({slot, a.born, image.referent, image.field_speech});
        if(cap_neurons.ptr){
            capillary::Mind learned=image.hydraulic?image.channels.mind:capillary::Mind{};
            s.d.check(s.d.upload(cap_neurons.ptr+Count(slot)*sizeof(learned),&learned,sizeof(learned)),"rehabilitate capillary contacts");
            if(cap_appetites.ptr){unsigned char bits=image.material_signals?capillary::pack_signs(image.appetitive,image.resident.germ.pigments[2]):0;
                s.d.check(s.d.upload(cap_appetites.ptr+slot,&bits,1),"restore the individual's appetitive contacts");}
            if(cap_contexts.ptr){auto memory=image.hydraulic?image.channels.history:capillary::History{};
                s.d.check(s.d.upload(cap_contexts.ptr+Count(slot)*sizeof(memory),&memory,sizeof(memory)),"restore the resident’s recent material evidence");}
            std::erase_if(h.audible_residents,[&](const auto &p){return p.slot==slot;});
            if(image.voices_signs)h.audible_residents.push_back({slot,a.born,image.signal_voice});
            if(hand_weights.ptr)s.d.check(s.d.upload(hand_weights.ptr+Count(slot)*4,&image.compound_contacts,4),"restore the actual resident’s four-way motor contacts");
            std::erase_if(h.compound_residents,[&](const auto &p){return p.slot==slot;});
            if(image.compound_signals)h.compound_residents.push_back({slot,a.born,image.compound_contacts});
            std::erase_if(h.cap_speakers,[&](const auto &p){return p.slot==slot;});
            if(image.hydraulic && capillary::schooled(image.channels.speech))h.cap_speakers.push_back({slot,a.born,image.channels.speech});
        }
        if(trails.ptr){
            utterance::Trail seen=image.describes?image.accounts.trail:utterance::Trail{};
            s.d.check(s.d.upload(trails.ptr+Count(slot)*sizeof(seen),&seen,sizeof(seen)),"restore the resident's retained transits");
        }
        std::erase_if(h.stories,[&](const auto &p){return p.slot==slot;});
        if(image.describes && image.accounts.memory.script[0].prompt) h.stories.push_back({slot,a.born,image.accounts.memory});
        if(float_events.ptr){
            braid::Episode seen{};if(image.narrates_floats)seen=image.braid_report.memory.event;
            s.d.check(s.d.upload(float_events.ptr+Count(slot)*sizeof(seen),&seen,sizeof(seen)),"place private observations");
            std::erase_if(h.narrators,[&](const auto &p){return p.slot==slot;});
            if(image.narrates_floats)h.narrators.push_back({slot,a.born,image.braid_report.memory});
        }
        std::erase_if(h.flute_words,[&](const auto &p){return p.slot==slot;});
        if(image.registers_gates)s.d.check(s.d.upload(sluice_contacts.ptr+Count(slot)*sizeof(braid::GateMemory),&image.sluice.gates,sizeof(braid::GateMemory)),"restore an individual pressure memory");
        if(image.binds_floats){
            auto arrival = image.braided.mind;
            arrival.lived &= ~braid::lineage_bits;
            s.d.check(s.d.upload(riggers.ptr + Count(slot) * sizeof(braid::Mind), &arrival, sizeof(arrival)),
                      "place immigrant float tissue");
            if(std::any_of(std::begin(image.braided.words.wave),std::end(image.braided.words.wave),[](Count x){return x!=0;}))
                h.flute_words.push_back({slot,a.born,image.braided.words});
        }
        if(image.hears_banks){auto m=rattle::rebase(image.rattle_organ.mind,0,tick);
            s.d.check(s.d.upload(rattlers.ptr+Count(slot)*sizeof(m),&m,sizeof(m)),"restore a bank listener");}
        if(image.folds_reeds){s.d.check(s.d.upload(cutters.ptr+Count(slot)*sizeof(runnel::Mind),&image.reed.mind,sizeof(runnel::Mind)),"restore individual fold contacts");}
        if(image.practises){
            auto &saved=image.training;if(practice::size(saved.mind)>practice::capacity(saved.mind.genes,h.learning.arena))throw std::invalid_argument("immigrant memories exceed the host arena");
            up(apprentices.ptr+Count(slot)*sizeof(practice::Mind),&saved.mind,sizeof(practice::Mind));
            for(Word j=0;j<h.learning.arena;++j)up(contexts.ptr+(Count(j)*n+slot)*sizeof(practice::Context),saved.bank+j,sizeof(practice::Context));
        }
        if(image.invents){
            auto mind=image.mechanism.mind;mind.tags=0;linkage::Tissue bank[4]{};
            for(Word old=0;old<image.mechanism.arena;++old)if(Word key=linkage::key_at(image.mechanism.mind,old)){
                Word slot_index=linkage::address(mind,key-1,h.millwork.arena);
                if(linkage::key_at(mind,slot_index))throw std::invalid_argument("immigrant material memories exceed the host's context arena");
                mind.tags|=key<<(4*slot_index);bank[slot_index]=image.mechanism.tissue[old];
            }
            up(inventors.ptr+Count(slot)*sizeof(mind),&mind,sizeof(mind));
            for(Word j=0;j<h.millwork.arena;++j)up(links.ptr+(Count(j)*n+slot)*sizeof(linkage::Tissue),bank+j,sizeof(linkage::Tissue));
        }
        if(image.values_effort)up(cost_minds.ptr+Count(slot)*sizeof(effort::Mind),&image.prospects.mind,sizeof(effort::Mind));
        if (image.ranges) {
            std::erase_if(h.place_words,[&](const auto &p){return p.slot==slot;});
            if(!(slot&15))for(Word j=0;j<h.recess.slots;++j)
                s.d.check(s.d.zero(path_sites.ptr+(Count(j)*((n+15)/16)+slot/16)*4,0,4),"clear an immigrant's external location log");
            if(image.path_speech.words.count) h.place_words.push_back({slot,a.born,image.referent.genes,0,image.path_speech.words});
            up(path_brains.ptr+Count(slot)*sizeof(hollow::Mind),&image.path_memory.mind,sizeof(hollow::Mind));
            for(Word j=0;j<h.recess.slots;++j)
                up(path_marks.ptr+(Count(j)*n+slot)*4,&image.path_memory.marks[j],sizeof(Word));
        }
        if (image.potter) {
            up(clayers.ptr + Count(slot) * sizeof(clay::Mind), &image.ceramic.mind, sizeof(clay::Mind));
            auto held = image.ceramic.palm;
            if (clay::size(held))
                held.changed = ~0u;
            up(palms.ptr + Count(slot) * sizeof(clay::Tile), &held, sizeof(held));
        }
        if (image.calling && image.native_words.count)
            h.captions.push_back({slot, a.born, image.referent.genes, 0, image.native_words});
        if (image.calling)
            up(reed_animals.ptr + Count(slot) * sizeof(siphon::Animal), &image.calls.animal, sizeof(siphon::Animal));
        if (image.saline)
            up(brine_tissue.ptr + Count(slot) * sizeof(brine::Animal), &image.brine_mind.cortex, sizeof(brine::Animal));
        if (image.investigates)
            up(enquirers.ptr + Count(slot) * sizeof(material::Inquiry), &image.inquiry, sizeof(image.inquiry));
        if (image.projects)
            up(forecasts.ptr + Count(slot) * sizeof(material::Foresight), &image.forecast, sizeof(image.forecast));
        if (image.handling)
            up(workers.ptr + Count(slot) * sizeof(material::Worker), &image.worker, sizeof(image.worker));
        if (image.delayed_pods)
            up(pod_tissue.ptr + Count(slot) * sizeof(pods::Animal), &image.pod_organ, sizeof(image.pod_organ));
        std::cout << "rehoused_resident=" << slot << "  former_slot=" << image.resident.slot
                  << "  age=" << tick - a.born << "  heard=" << a.witnesses << '\n';
    }
    void hatch_pools(const Options &o) {
        auto adults = [&](const std::string &path) {
            std::vector<Settler> genes;
            if (path.empty())
                return genes;
            Habitat source;
            source.read(path);
            for (Word i = 0; i < source.sites(); ++i)
                if (source.bodies[i].energy)
                    genes.push_back(
                        {source.genomes[i],
                         source.enclosed
                             ? pods::Inheritance{source.pod_animals[i].genes, source.pod_animals[i].frame_genes}
                             : pods::Inheritance{pods::founder(mix(source.header.configuration.seed ^ mix(i + 179117))),
                                                 pods::frame_founder(
                                                     mix(source.header.configuration.seed ^ mix(i + 179117)))},
                         source.developing ? source.cortices[i].genes
                                           : growth_founder(mix(source.header.configuration.seed + i + 19561)),
                         source.articulated ? source.workers[i].genes
                                            : material::founder(mix(source.header.configuration.seed + i * 713 + 311)),
                         source.saline ? source.saline_animals[i].genes
                                       : brine::founder(mix(source.header.configuration.seed + i * 127)),
                         source.calling ? source.callers[i].genes
                                        : siphon::founder(mix(source.header.configuration.seed + i * 2539)),
                         source.investigatory ? source.enquiries[i].receptors : 0,
                         source.investigatory ? source.enquiries[i].impulses : 0, source.investigatory,
                         source.pottery ? source.potters[i].genes
                                        : clay::founder(mix(source.header.configuration.seed + i * 10039))});
            if (genes.empty())
                throw std::invalid_argument("no surviving donor in " + path);
            return genes;
        };
        auto a = adults(o.colonists), b = adults(o.competitors);
        Temporary first(s.d, a.size() * sizeof(Settler)), second(s.d, b.size() * sizeof(Settler));
        s.d.check(s.d.upload(first.ptr, a.data(), a.size() * sizeof(Settler)), "read colonist germlines");
        if (!b.empty())
            s.d.check(s.d.upload(second.ptr, b.data(), b.size() * sizeof(Settler)), "read competitor germlines");
        Word na = Word(a.size()), nb = Word(b.size()), swap = o.swap, cross = o.cross;
        Word paired = o.paired_material;
        Word twins = na == nb && std::memcmp(a.data(), b.data(), na * sizeof(Settler)) == 0;
        Function spawn{};
        s.bind(spawn, "flood_colonists", Image::floodplain);
        launch(spawn, (n / 2 + 127) / 128, 128, world, c, first.ptr, na, second.ptr, nb, swap, cross, twins, paired);
        synchronize();
        std::cout << "flood_pools=" << na << "  competitors=" << nb << "  swapped=" << swap
                  << "  crossed_brains=" << cross << "  identical_pairs=" << twins << '\n';
    }
    void replace_rinds(const Habitat &h) {
        if (!rinds.ptr)
            return;
        if (h.enclosed)
            s.d.check(s.d.upload(rinds.ptr, h.rinds.data(), n * sizeof(pods::Surface)),
                      "transplant worn fruit surfaces");
        else
            s.d.check(s.d.zero(rinds.ptr, 0, n * sizeof(pods::Surface)), "introduce unworked fruit surfaces");
    }
    void forget_strand_makers() {
        auto marks = world.drift_marks;
        if (marks.items)
            s.d.check(
                s.d.zero(reinterpret_cast<cuda::DevicePtr>(marks.items), 0, Count(marks.watched) * sizeof(drift::Mark)),
                "clear records of replaced strands");
    }
    void replace_land(const std::vector<Silt> &replacement) {
        if (replacement.size() != n)
            throw std::invalid_argument("soil transplant needs equally sized floodplains");
        s.d.check(s.d.upload(land.ptr, replacement.data(), n * sizeof(Silt)), "transplant the recorded watershed");
        forget_strand_makers();
    }
    void flatten() {
        Temporary refused(s.d, 4);
        s.d.check(s.d.zero(refused.ptr, 0, 4), "clear the bank removal ledger");
        Function remove{};
        s.bind(remove, "flood_unbank", Image::floodplain);
        launch(remove, (n + 127) / 128, 128, land.ptr, n, refused.ptr);
        synchronize();
        Word overflow = 0;
        s.d.check(s.d.download(&overflow, refused.ptr, 4), "check relocated sediment");
        if (overflow)throw std::runtime_error("resuspended sediment exceeds cell capacity");
        forget_strand_makers();
        if(marked_strands.ptr)s.d.check(s.d.zero(marked_strands.ptr,0,Count(n/4)*sizeof(practice::Mark)),"clear marks after the fibre has been resuspended");
        if(parcels.ptr){Function clear{};s.bind(clear,"wick_rebed",Image::floodplain);launch(clear,(n+127)/128,128,world.wicks,n);synchronize();}
    }
    void rootstock() {
        Function initial{};
        s.bind(initial, "flood_rootstock", Image::floodplain);
        launch(initial, (n + 127) / 128, 128, world.land, c);
        synchronize();
        std::cout << "rootstock=" << n << '\n';
    }
    void forget() { s.d.check(s.d.zero(ears.ptr, 0, n * sizeof(Ear)), "clear learned acoustic contacts"); }
    GateTrial gate_trial(const Options &o) {
        GateTrial trial{{}, tick, o.gate_seed, o.gate_phase, o.gate_trial == 1};
        Temporary records(s.d, Count(n) * sizeof(GateSubject));
        Function apply{};
        s.bind(apply, "flood_gate_trial", Image::floodplain);
        Word sham = trial.sham;
        launch(apply, (n + 127) / 128, 128, world, tiles, n, tick, trial.seed, trial.phase, sham, records.ptr);
        synchronize();
        trial.subjects.resize(n);
        s.d.check(s.d.download(trial.subjects.data(), records.ptr, n * sizeof(GateSubject)),
                  "retain enrolled identities");
        return trial;
    }
    struct Trial {
        Temporary bodies, nerve, ear, motor, input;
        World view;
        Trial(Session &s, World source, Word n, Word tiles)
            : bodies(s.d, n * sizeof(Colony)), nerve(s.d, Count(n) * tiles * sizeof(Tissue)),
              ear(s.d, n * sizeof(Ear)), motor(s.d, n), input(s.d, n * sizeof(Word)), view(source) {
            view.pods.observing = 1;
            view.matter.observing = 1;
            if (!source.worksites.residents)
                view.tide.cells = nullptr;
            view.worksites.ground = nullptr;
            view.worksites.ledger = nullptr;
            view.tide.sight = nullptr;
            view.animals = reinterpret_cast<Colony *>(bodies.ptr);
            view.nerve = reinterpret_cast<Tissue *>(nerve.ptr);
            view.neural_pages = {};
            view.ears = reinterpret_cast<Ear *>(ear.ptr);
            view.motor = reinterpret_cast<Byte *>(motor.ptr);
        }
    };
    void assay_growth() {
        if (!world.cortex.resident)
            throw std::invalid_argument("the tissue-growth assay requires a developing habitat");
        Trial full(s, world, n, tiles), muted(s, world, n, tiles);
        Temporary output(s.d, Count(n) * sizeof(HearingEffect));
        Function prepare{}, effect{};
        s.bind(prepare, "flood_listen_copy", Image::floodplain);
        s.bind(effect, "flood_growth_effect", Image::floodplain);
        Word groups = (n + 127) / 128, zero = 0;
        auto replay = [&](Trial &trial, Word mute) {
            trial.view.cortex.mute_new = mute;
            launch(prepare, groups, 128, world, trial.view, c, zero);
            launch(think, neural_blocks, 128, trial.view, c, clock.ptr, zero);
        };
        replay(full, 0);
        replay(muted, 1);
        launch(effect, groups, 128, world, full.view, muted.view, c, output.ptr);
        synchronize();
        std::vector<HearingEffect> values(n);
        s.d.check(s.d.download(values.data(), output.ptr, values.size() * sizeof(values[0])),
                  "measure recruited cells and their motor consequences");
        Count grown = 0, added = 0, original = 0, actions = 0, contacts = 0;
        for (auto &a : values) {
            grown += a.experienced;
            added += a.inputs;
            original += a.spikes;
            actions += a.motor;
            contacts += a.contacts;
        }
        std::cout << "growth_replay=" << grown << "  recruited_spikes=" << added
                  << "  prior_spikes_changed=" << original << "  actions_changed=" << actions
                  << "  prior_contacts_changed=" << contacts << '\n';
    }
    void assay_calls() {
        Trial first(s, world, n, tiles), other(s, world, n, tiles);
        Temporary output(s.d, n * sizeof(HearingEffect));
        Function observe{}, prepare{}, compare{};
        s.bind(observe, "flood_input", Image::floodplain);
        s.bind(prepare, "flood_listen_copy", Image::floodplain);
        s.bind(compare, "flood_hearing_effect", Image::floodplain);
        Word groups = (n + 127) / 128, offset = 0;
        std::vector<HearingEffect> changes(n);
        auto replay = [&](Trial &t, Word mode) {
            launch(prepare, groups, 128, world, t.view, c, mode);
            launch(observe, groups, 128, t.view, c, clock.ptr, t.input.ptr);
            launch(think, neural_blocks, 128, t.view, c, clock.ptr, offset);
        };
        replay(first, 0);
        const char *conditions[]{"sham", "cleared", "permuted"};
        for (Word mode = 0; mode < 3; ++mode) {
            replay(other, mode);
            launch(compare, groups, 128, world, first.view, other.view, c, first.input.ptr, other.input.ptr,
                   output.ptr);
            synchronize();
            s.d.check(s.d.download(changes.data(), output.ptr, n * sizeof(HearingEffect)),
                      "measure causal changes in acquired hearing");
            Count heard = 0, inputs = 0, spikes = 0, motor = 0, synapses = 0;
            for (auto &r : changes) {
                heard += r.experienced;
                inputs += r.inputs;
                spikes += r.spikes;
                motor += r.motor;
                synapses += r.contacts;
            }
            if (!mode && (inputs || spikes || motor || synapses))
                throw std::logic_error("identical auditory replay diverged");
            std::cout << "field_hearing=" << conditions[mode] << "  tick=" << tick << "  experienced=" << heard
                      << "  changed_inputs=" << inputs << "  changed_actions=" << motor
                      << "  changed_spikes=" << spikes << "  changed_synapses=" << synapses << '\n';
        }
    }
    bool margin() {
        Temporary reached(s.d, 4);
        s.d.check(s.d.zero(reached.ptr, 0, 4), "clear frontier observation");
        Function sample{};
        s.bind(sample, "flood_margin", Image::floodplain);
        launch(sample, (2 * c.side + 127) / 128, 128, world, c.side, reached.ptr);
        synchronize();
        Word found = 0;
        s.d.check(s.d.download(&found, reached.ptr, 4), "observe established frontier residents");
        return found != 0;
    }
    void pod_portrait(journal::Specimen &specimen, Word slot, Word cohort) {
        if (!pod_tissue.ptr)
            return;
        pods::Animal organ;
        s.d.check(s.d.download(&organ, pod_tissue.ptr + Count(slot) * sizeof(organ), sizeof(organ)),
                  "sample the pictured resident's living contacts");
        auto &q = specimen.measures;
        Word active = 0;
        for (auto bank : organ.brain.contact)
            active += std::popcount((Word(bank) | (Word(bank) >> 1)) & 0x5555u);
        q["pod_contacts"] = active;
        q["lag_banks"] = std::popcount(organ.genes.timing & 255);
        q["pod_selectors"] = organ.genes.sense & 511;
        q["crop_mass"] = pods::content(organ.gut);
        if (pods::sensory_channels(world.pods.physics, cohort) & 4) {
            q["known_surfaces"] = std::popcount(organ.frame.known);
            q["frame_switches"] = organ.frame.switches;
            q["motor_frame"] = organ.frame.active;
            q["motor_routes"] =
                organ.frame.active ? (organ.frame_genes.routes >> (8 * organ.frame.active)) & 255 : 0xe4;
        }
        specimen.senses.emplace_back("Fruit surface and digestive pulses");
        specimen.circuits.emplace_back("Delayed receptor–muscle contacts");
    }
    void sample(Habitat &h) {
        move_capillaries(h,false);
        h.land.resize(n);
        h.slots.resize(n);
        h.genomes.resize(n);
        h.bodies.resize(n);
        auto read = [&](auto &v, DevicePtr p) {
            s.d.check(s.d.download(v.data(), p, v.size() * sizeof(v[0])),
                      "observe living lineages and their watershed");
        };
        read(h.land, land.ptr);
        read(h.slots, slots.ptr);
        read(h.bodies, animals.ptr);
        h.retain_speakers();
        read(h.genomes, dna.ptr);
        if(float_events.ptr){h.float_events.resize(n);read(h.float_events,float_events.ptr);}
        if(trails.ptr){h.trails.resize(n);read(h.trails,trails.ptr);}
        if(h.braiding){h.riggers.resize(n);h.floats.resize(n);read(h.riggers,riggers.ptr);read(h.floats,floaters.ptr);if(sluice_contacts.ptr){h.sluice_contacts.resize(n);read(h.sluice_contacts,sluice_contacts.ptr);}}
        if(h.rattling){h.listeners.resize(n);read(h.listeners,rattlers.ptr);}
        if(h.reeding){h.cutters.resize(n);h.folds.resize(n);read(h.cutters,cutters.ptr);read(h.folds,folded.ptr);}
        if(h.wicking){h.liquid.resize(n);read(h.liquid,parcels.ptr);}
        if(h.practising){h.strands.resize(n/4);read(h.strands,marked_strands.ptr);h.pupils.resize(n);h.memories.resize(Count(n)*h.learning.arena);read(h.pupils,apprentices.ptr);read(h.memories,contexts.ptr);}
        if(h.linking){h.makers.resize(n);h.mechanisms.resize(Count(n)*h.millwork.arena);h.seals.resize(n/4);
            read(h.makers,inventors.ptr);read(h.mechanisms,links.ptr);read(h.seals,patents.ptr);}
        if(h.appraising){h.economies.resize(n);h.treads.resize(n);read(h.economies,cost_minds.ptr);read(h.treads,treads.ptr);}
        if(h.ranging) {
            h.wanderers.resize(n);h.hollows.resize(n);h.bearings.resize(Count(n)*h.recess.slots);h.sightings.resize(Count((n+15)/16)*h.recess.slots);
            read(h.wanderers,path_brains.ptr);read(h.hollows,path_soil.ptr);read(h.bearings,path_marks.ptr);read(h.sightings,path_sites.ptr);
        }
        if (h.overhearing) {
            h.acoustics.resize(n);
            read(h.acoustics, airborne.ptr);
        }
        if (h.pottery) {
            h.potters.resize(n);
            h.ware.resize(n);
            h.cargo.resize(n);
            read(h.potters, clayers.ptr);
            read(h.ware, earthen.ptr);
            read(h.cargo, palms.ptr);
        }
        if (h.calling) {
            h.callers.resize(n);
            h.apertures.resize(n);
            read(h.callers, reed_animals.ptr);
            read(h.apertures, reed_ports.ptr);
        }
        if (h.saline) {
            h.salts.resize(n);
            h.saline_animals.resize(n);
            read(h.salts, salts.ptr);
            read(h.saline_animals, brine_tissue.ptr);
            auto observed = h.salt_inventory();
            if (!std::equal(observed.begin(), observed.end(), h.solution.stock))
                throw std::logic_error("dissolved material changed in the watershed");
        }
        if (h.sedimentary) {
            h.macromolecules.resize(n);
            read(h.macromolecules, polymers.ptr);
        }
        if (h.investigatory) {
            h.enquiries.resize(n);
            h.patina.resize(n);
            read(h.enquiries, enquirers.ptr);
            read(h.patina, impressions.ptr);
        }
        if (h.foresight) {
            h.forecasts.resize(n);
            read(h.forecasts, forecasts.ptr);
        }
        if (h.articulated) {
            h.workers.resize(n);
            h.mineral_bed.resize(n);
            read(h.workers, workers.ptr);
            read(h.mineral_bed, mineral_bed.ptr);
        }
        if (h.developing) {
            h.cortices.resize(n);
            read(h.cortices, cortex.ptr);
        }
    }
#include "flood_legacy.hpp"
#include "flood_artifacts.hpp"
#include "flood_material_network.hpp"
#include "flood_material_deploy.hpp"
#include "flood_mill.hpp"
#include "flood_scree.hpp"
#include "flood_field_transfer.hpp"
#include "flood_brine_study.hpp"
#include "flood_siphon_study.hpp"
#include "flood_clay_study.hpp"
#include "flood_press_study.hpp"
#include "flood_hollow_study.hpp"
#include "flood_effort_study.hpp"
#include "flood_linkage_study.hpp"
#include "flood_practice_study.hpp"
#include "flood_wick_study.hpp"
#include "flood_runnel_study.hpp"
#include "flood_rattle_study.hpp"
#include "flood_braid_dialogues.hpp"
#include "flood_braid_culture.hpp"
#include "flood_nerve_io.hpp"
#include "flood_neural_dispatch.hpp"
#include "flood_field_class.hpp"
#include "flood_apprentice.hpp"
#include "flood_drift_study.hpp"
#include "flood_inlet_study.hpp"
#include "retina_surfaces.hpp"
#include "flood_retina_study.hpp"
#include "flood_audience_study.hpp"
#include "flood_narrative_rearing.hpp"
#include "flood_narrative_study.hpp"
#include "flood_capillary_study.hpp"
#include "flood_hand_record.hpp"
#include "flood_capillary_record.hpp"
#include "flood_detritus_record.hpp"
#include "flood_cascade_study.hpp"
#include "flood_braid_parallax.hpp"
#include "flood_braid_inquiry.hpp"
#include "flood_braid_study.hpp"
#include "flood_pod_assay.hpp"
#include "flood_pod_deploy.hpp"
#include "flood_pod_rafts.hpp"
    BasinCensus check(Habitat &h) {
        if(h.braiding){
            std::vector<braid::Totals> shards((n/4+127)/128);
            s.d.check(s.d.download(shards.data(),float_books.ptr,shards.size()*sizeof(shards[0])),"read float and neural accounts");
            std::array<Count,sizeof(braid::Totals)/8> sums{};
            for(auto &a:shards){auto s=std::bit_cast<decltype(sums)>(a);for(size_t j=0;j<sums.size();++j)sums[j]+=s[j];}
            h.loom.book=std::bit_cast<braid::Totals>(sums);
            if(sluice_books.ptr){
                std::vector<braid::GateBook> rows((n/8+127)/128);
                s.d.check(s.d.download(rows.data(),sluice_books.ptr,rows.size()*sizeof(rows[0])),"read matched pressure flows");
                std::array<Count,sizeof(braid::GateBook)/8> total{};
                for(const auto &row:rows){auto vals=std::bit_cast<decltype(total)>(row);for(size_t i=0;i<total.size();++i)total[i]+=vals[i];}
                h.sluice_book=std::bit_cast<braid::GateBook>(total);
            }
            if(collar_books.ptr){
                std::vector<braid::SedimentBook> beads((n/8+127)/128);
                s.d.check(s.d.download(beads.data(),collar_books.ptr,beads.size()*sizeof(beads[0])),"read mineral flow records");
                std::array<Count,sizeof(braid::SedimentBook)/8> total{};
                for(auto &row:beads){auto words=std::bit_cast<decltype(total)>(row);for(size_t i=0;i<words.size();++i)total[i]+=words[i];}
                h.sediment_book=std::bit_cast<braid::SedimentBook>(total);
            }
        }
        if(h.rattling){
            std::vector<rattle::Totals> shards((n/4+127)/128);
            s.d.check(s.d.download(shards.data(),rattle_books.ptr,shards.size()*sizeof(shards[0])),"read bank work");
            std::array<Count,sizeof(rattle::Totals)/8> total{};
            for(const auto &shard:shards){auto a=std::bit_cast<decltype(total)>(shard);for(size_t j=0;j<a.size();++j)total[j]+=a[j];}
            h.backwater.total=std::bit_cast<rattle::Totals>(total);
            if(mesh_book.ptr){
                std::vector<rattle::Load> loads(shards.size());s.d.check(s.d.download(loads.data(),mesh_book.ptr,loads.size()*sizeof(loads[0])),"read opposing forces");
                std::array<Count,sizeof(rattle::Load)/8> tally{};
                for(const auto &v:loads){auto a=std::bit_cast<decltype(tally)>(v);for(size_t i=0;i<a.size();++i)tally[i]+=a[i];}
                h.mesh=std::bit_cast<rattle::Load>(tally);
            }
            if(rattle_queries.ptr){
                std::vector<rattle::Attention> values(shards.size());s.d.check(s.d.download(values.data(),rattle_queries.ptr,values.size()*sizeof(values[0])),"read acoustic inquiries");
                std::array<Count,sizeof(rattle::Attention)/8> tally{};
                for(auto &v:values){auto a=std::bit_cast<decltype(tally)>(v);for(size_t i=0;i<a.size();++i)tally[i]+=a[i];}
                h.queries=std::bit_cast<rattle::Attention>(tally);
            }
        }
        if(h.wicking){
            std::vector<wicks::Totals> books((n/2+127)/128);s.d.check(s.d.download(books.data(),wick_ledgers.ptr,books.size()*sizeof(books[0])),"read capillary mass flux");
            std::array<Count,sizeof(wicks::Totals)/8> sum{};for(auto &b:books){auto t=std::bit_cast<decltype(sum)>(b);for(size_t j=0;j<t.size();++j)sum[j]+=t[j];}h.wicking_state.book=std::bit_cast<wicks::Totals>(sum);
        }
        if(h.reeding){
            std::vector<runnel::Totals> books((n/2+127)/128);s.d.check(s.d.download(books.data(),reed_books.ptr,books.size()*sizeof(books[0])),"read seam accounting");
            std::array<Count,sizeof(runnel::Totals)/8> sums{};
            for(auto &b:books){auto v=std::bit_cast<decltype(sums)>(b);for(size_t i=0;i<sums.size();++i)sums[i]+=v[i];}
            h.rush.book=std::bit_cast<runnel::Totals>(sums);
            std::vector<runnel::Social> social(books.size());s.d.check(s.d.download(social.data(),reed_effects.ptr,social.size()*sizeof(social[0])),"audit socially acquired construction");
            std::array<Count,sizeof(runnel::Social)/8> sum{};for(auto &x:social){auto v=std::bit_cast<decltype(sum)>(x);for(size_t k=0;k<sum.size();++k)sum[k]+=v[k];}
            h.reed_audit=std::bit_cast<runnel::Social>(sum);
        }
        if(h.practising){
            std::vector<practice::Totals> shards((n/4+127)/128);s.d.check(s.d.download(shards.data(),practice_books.ptr,shards.size()*sizeof(shards[0])),"read practice accounts");
            std::array<Count,sizeof(practice::Totals)/8> totals{};
            for(auto &b:shards){auto v=std::bit_cast<decltype(totals)>(b);for(size_t j=0;j<v.size();++j)totals[j]+=v[j];}
            h.learning.book=std::bit_cast<practice::Totals>(totals);
            std::vector<practice::Credits> receipts(shards.size());s.d.check(s.d.download(receipts.data(),practice_credits.ptr,receipts.size()*sizeof(receipts[0])),"read effects of retained strands");
            std::array<Count,sizeof(practice::Credits)/8> accounts{};for(auto &r:receipts){auto a=std::bit_cast<decltype(accounts)>(r);for(size_t k=0;k<a.size();++k)accounts[k]+=a[k];}
            h.legacy_work=std::bit_cast<practice::Credits>(accounts);
        }
        if(h.linking){
            std::vector<linkage::Totals> shard((n/4+127)/128);s.d.check(s.d.download(shard.data(),link_books.ptr,shard.size()*sizeof(shard[0])),"observe physical machine work");
            std::array<Count,sizeof(linkage::Totals)/8> out{};for(auto &v:shard){auto a=std::bit_cast<decltype(out)>(v);for(size_t j=0;j<a.size();++j)out[j]+=a[j];}
            h.millwork.totals=std::bit_cast<linkage::Totals>(out);
        }
        if(h.appraising){
            std::vector<effort::Ledger> shards((n/2+127)/128);
            s.d.check(s.d.download(shards.data(),cost_books.ptr,shards.size()*sizeof(shards[0])),"read prospective control ledgers");
            std::array<Count,sizeof(effort::Ledger)/sizeof(Count)> sum{};
            for(const auto &r:shards){auto v=std::bit_cast<decltype(sum)>(r);for(size_t j=0;j<v.size();++j)sum[j]+=v[j];}
            h.economics.ledger=std::bit_cast<effort::Ledger>(sum);
        }
        if(h.ranging) {
            std::vector<hollow::Ledger> shards((n/2+127)/128);
            s.d.check(s.d.download(shards.data(),path_books.ptr,shards.size()*sizeof(shards[0])),"read spatial work");
            std::array<Count,sizeof(hollow::Ledger)/sizeof(Count)> sum{};
            for(const auto &r:shards){auto a=std::bit_cast<decltype(sum)>(r);for(size_t j=0;j<a.size();++j)sum[j]+=a[j];}
            h.recess.book=std::bit_cast<hollow::Ledger>(sum);
        }
        if (h.sedimentary) {
            std::vector<material::SedimentTotals> books((n / 2 + 127) / 128);
            s.d.check(s.d.download(books.data(), sediment_books.ptr, books.size() * sizeof(books[0])),
                      "read sedimentary budgets");
            std::array<Count, 6> sum{};
            for (auto &b : books) {
                auto q = std::bit_cast<decltype(sum)>(b);
                for (Word i = 0; i < 6; ++i)
                    sum[i] += q[i];
            }
            h.seam.totals = std::bit_cast<material::SedimentTotals>(sum);
        }
        if (h.investigatory) {
            std::vector<material::WorksiteTotals> books((n / 2 + 127) / 128);
            s.d.check(s.d.download(books.data(), worksite_books.ptr, books.size() * sizeof(books[0])),
                      "read worksite accounts");
            std::array<Count, sizeof(material::WorksiteTotals) / sizeof(Count)> total{};
            for (auto &b : books) {
                auto q = std::bit_cast<decltype(total)>(b);
                for (size_t j = 0; j < q.size(); ++j)
                    total[j] += q[j];
            }
            h.worksite.totals = std::bit_cast<material::WorksiteTotals>(total);
        }
        if (h.pottery) {
            std::vector<clay::Totals> shards((n / 2 + 127) / 128);
            s.d.check(s.d.download(shards.data(), clay_book.ptr, Count(shards.size()) * sizeof(shards[0])),
                      "read clay accounts");
            std::array<Count, sizeof(clay::Totals) / sizeof(Count)> sum{};
            for (auto &r : shards) {
                auto parts = std::bit_cast<std::array<Count, sum.size()>>(r);
                for (size_t i = 0; i < sum.size(); ++i)
                    sum[i] += parts[i];
            }
            h.workshop.book = std::bit_cast<clay::Totals>(sum);
        }
        if (h.calling) {
            std::vector<siphon::Totals> shards((n / 2 + 127) / 128);
            s.d.check(s.d.download(shards.data(), reed_ledger.ptr, shards.size() * sizeof(shards[0])),
                      "observe actual acoustic exchanges");
            std::array<Count, sizeof(siphon::Totals) / sizeof(Count)> sum{};
            for (auto &r : shards) {
                auto a = std::bit_cast<std::array<Count, sum.size()>>(r);
                for (size_t k = 0; k < sum.size(); ++k)
                    sum[k] += a[k];
            }
            h.reeds.total = std::bit_cast<siphon::Totals>(sum);
            if (h.overhearing) {
                std::vector<siphon::Audience> books(shards.size());
                s.d.check(s.d.download(books.data(), audience_book.ptr, books.size() * sizeof(books[0])),
                          "read third-party uptake");
                siphon::Audience q{};
                for (auto &b : books) {
                    q.heard += b.heard;
                    q.paired += b.paired;
                    q.edits += b.edits;
                    q.young += b.young;
                }
                h.audience.total = q;
            }
        }
        if (h.foresight)
            s.d.check(s.d.download(&h.tide.totals, tide_book.ptr, sizeof(h.tide.totals)),
                      "read construction tide costs");
        Word groups = (n + 127) / 128;
        if (h.developing) {
            std::vector<GrowthTotals> accounts((n / 2 + 127) / 128);
            s.d.check(s.d.download(accounts.data(), growth_books.ptr, accounts.size() * sizeof(GrowthTotals)),
                      "read local tissue budgets");
            std::array<Count, 7> sum{};
            for (auto &record : accounts) {
                auto row = std::bit_cast<decltype(sum)>(record);
                for (size_t j = 0; j < sum.size(); ++j)
                    sum[j] += row[j];
            }
            h.neuro.account = std::bit_cast<GrowthTotals>(sum);
        }
        if (h.articulated) {
            std::vector<material::Ledger> ledger((n / 2 + 127) / 128);
            s.d.check(s.d.download(ledger.data(), material_books.ptr, ledger.size() * sizeof(ledger[0])),
                      "read material flux");
            std::array<Count, sizeof(material::Ledger) / sizeof(Count)> sum{};
            for (auto &entry : ledger) {
                auto r = std::bit_cast<decltype(sum)>(entry);
                for (size_t i = 0; i < sum.size(); ++i)
                    sum[i] += r[i];
            }
            h.minerals.account = std::bit_cast<material::Ledger>(sum);
        }
        launch(survey, groups, 128, world, c, partial.ptr);
        sum_account(counters.ptr, budget.ptr);
        if (h.enclosed) {
            Function census{};
            s.bind(census, "flood_pod_budget", Image::floodplain);
            Word shards = (n / 2 + 127) / 128;
            launch(census, 1, 32, pod_book.ptr, shards, pod_total.ptr);
        }
        synchronize();
        std::vector<BasinCensus> pieces(groups);
        s.d.check(s.d.download(pieces.data(), partial.ptr, groups * sizeof(BasinCensus)), "read physical balances");
        BasinCensus total;
        std::array<Count, sizeof(BasinCensus) / sizeof(Count)> all{};
        for (auto p : pieces) {
            auto row = std::bit_cast<decltype(all)>(p);
            for (size_t j = 0; j < all.size(); ++j)
                all[j] += row[j];
        }
        total = std::bit_cast<BasinCensus>(all);
        s.d.check(s.d.download(&h.header.account, budget.ptr, sizeof(Counters)), "read accumulated work");
        if (h.enclosed)
            s.d.check(s.d.download(&h.crop.account, pod_total.ptr, sizeof(pods::Census)),
                      "read cumulative crop budget");
        h.header.tick = tick;
        return total;
    }
};
#include "flood_record.hpp"
#include "flood_home_voice.hpp"
void report(Habitat &h, BasinCensus p, Word elapsed) {
    auto &a = h.header.account;
    auto base = h.header.initial;
    if (p.water != base.water || p.clay != base.clay || p.energy + a.heat != base.energy + a.light ||
        p.alive + a.deaths != h.header.founders + a.births)
        throw std::logic_error("floodplain water, clay, energy or population conservation failed");
    std::cout << "flood_tick=" << h.header.tick << "  living=" << p.alive << "  births=" << a.births
              << "  deaths=" << a.deaths << "  water=" << p.water << "  clay=" << p.clay
              << "  food=" << p.leaf + p.fruit + p.litter << "  animal_energy=" << p.energy_in_animals
              << "  sunlight=" << a.light << "  heat=" << a.heat << "  walls=" << p.walls << "  banked=" << p.banked
              << "  laid=" << a.laid << "  fibre=" << p.fibre << "  fibre_cargo=" << p.fibre_cargo
              << "  mixed_faces=" << p.mixed_faces << "  joined=" << a.joined << "  woven=" << a.fibre_laid
              << "  supported=" << a.supported << "  frayed=" << a.frayed << "  lifted=" << a.lifted
              << "  erosion=" << a.erosion << "  flow=" << a.flow << "  wet=" << p.wet << "  chemistry=" << a.converted
              << "  transported=" << a.transported << "  recruits=" << a.recruits
              << "  plant_mutations=" << a.plant_edits << "  reproductive_stock=" << a.plant_investment
              << "  seedlings=" << a.seeded << "  moved=" << a.moves << "  transferred=" << a.shared
              << "  voices=" << a.calls << "  receptions=" << a.heard << "  body_area=" << p.area
              << "  neurons=" << p.conducting << "  gene_edits=" << a.edits << "  shape_edits=" << a.body_edits
              << "  tape_edits=" << a.tape_edits << "  module_edits=" << a.module_edits
              << "  brain_changes=" << a.brain_changes << "  developmental_changes=" << a.remodelled
              << "  lineage_sum=" << p.generations << '\n';
    if (h.articulated) {
        auto &m = h.minerals;
        auto &l = m.account;
        if (m.pieces != p.grains || !std::equal(p.terminals, p.terminals + 3, m.terminals) ||
            m.charge + l.light + (h.investigatory ? h.worksite.totals.pumped : 0) !=
                p.excitation + l.reactions + l.thermal)
            throw std::logic_error("mineral, surface group or excitation conservation failed");
        std::cout << "material_tick=" << h.header.tick << "  grains=" << p.grains << "  carried=" << p.carried_grains
                  << "  charge=" << p.excitation << "  photons=" << l.light << "  current=" << l.transfers
                  << "  products=" << l.reactions << "  folds=" << l.folds << "  turns=" << l.turns
                  << "  lifted=" << l.lifts << "  placed=" << l.placements << "  pushed=" << l.translations
                  << "  exchanges=" << l.exchanges << "  spontaneous=" << l.thermal << "  sightings=" << l.observed
                  << "  edits=" << l.edits << "  mechanical_work=" << l.heat << '\n';
    }
    if (h.developing) {
        const auto &g = h.neuro.account;
        if (h.neuro.initial + g.births + g.recruited != p.neurons + g.deaths + g.retired ||
            g.investment != g.recruited / 16)
            throw std::logic_error("expressed spiking cell ledger is inconsistent");
        std::cout << "spiking_growth=" << h.header.tick << "  expressed=" << p.conducting
                  << "  tissue_cells=" << p.neurons << "  silent_cells=" << (p.neurons - p.conducting)
                  << "  born_cells=" << g.births << "  dead_cells=" << g.deaths << "  cells_added=" << g.recruited
                  << "  cells_retired=" << g.retired << "  larger_newborns=" << g.larger_births
                  << "  smaller_newborns=" << g.smaller_births << "  growth_energy=" << g.investment
                  << "  capacity_cells=" << h.header.configuration.tiles * 32 << '\n';
    }
    if (h.enclosed) {
        auto &a = h.crop.account;
        if (Count(h.crop.entry_mass) + a.swallowed != a.returned + a.waste + p.digesting)
            throw std::logic_error("crop reservoir conservation failed");
        std::cout << "pod_tick=" << h.header.tick << "  bites=" << a.bites << "  cracked=" << a.opened
                  << "  ingested=" << a.swallowed << "  absorbed=" << a.returned << "  husks=" << a.waste
                  << "  in_crop=" << p.digesting << "  coincident=" << a.overlaps << "  deliveries=" << a.mature
                  << "  synaptic_edits=" << a.edges << "  withheld=" << a.withheld << '\n';
    }
    if (auto *j = journal::current()) {
        j->side = h.header.configuration.side;
        j->metrics["animals"] = p.alive;
        j->metrics["flood_elapsed"] = elapsed;
        j->metrics["flood_barriers"] = p.walls;
    }
}
} // namespace
int command(int argc, char **argv) {
    if (argc == 3 && std::string_view(argv[2]) == "--help") {
        std::cout
            << "axion floodplain --side N --neurons 64|128|256|512|1024 --seed N --season TICKS --density PERCENT "
               "--steps N --report-every N --legacy TICKS --frontier MAX_SIDE --extend SIDE\n"
               "  --founder MIND --senses-from MIND --soil HABITAT --remobilize --export-mind MIND --load HABITAT "
               "--save HABITAT --construction open|closed --morphology open|closed --architecture open|closed\n"
               "  --resident MIND --slot SLOT --observer --colonists HABITAT --competitors HABITAT --swap "
               "--cross-brains\n"
               "  --wire open|closed|scrambled|relabeled --assay-calls --assay-every TICKS --hearing scalar|projected "
               "--forget-calls --plasticity open|closed --contact open|closed --weaving open|closed --producers "
               "evolving|fixed|simple\n";
        std::cout
            << "  --neurogenesis inherited|fixed --assay-growth --fixed-size-cohort a|b|none --silence-new-cohort "
               "a|b|none --neurons "
               "64|128|256|512|1024\n  --pods sealed|mixed --pod-delay 1..8 --pod-clock fixed|mixed --pod-period TICKS "
               "--pod-origin random|reference|inert --pod-spacing inherited|continuous\n"
               "  --pod-frames inherited|single --pod-regimes shared|independent --pod-pulse enteric|nutrient "
               "--pod-credit live|frozen|shifted --erase-pods --freeze-cohort a|b|none --single-frame-cohort a|b|none "
               "--pod-germ MIND\n"
               "  --pod-bench CALLS --pod-remap CALIBRATION --pod-observe AFTER_CHANGE --pod-cues 1..7 --pod-assay "
               "TRIALS --pod-subjects N --pod-test-seed N --raft-seasons N --raft-capacity N --raft-days TICKS "
               "--raft-ripening steady|flush --raft-frames inherited|diverse --recruit provisioned|fostered\n";
        std::cout
            << "  --materials reactive|passive|off --mineral-flow coupled|permeable --hydraulic-trial TICKS --grains "
               "PERCENT --bonds open|insulated --hands open|still "
               "--work-sight open|shuttered|scrambled --material-legacy TICKS --artifact-samples N --hand-germ MIND\n";
        std::cout << "  --work-seasons N | --quarry-seasons N  --work-days N --work-capacity N --work-span 1..4 "
                     "--work-recruit provisioned|fostered\n  --guild-generations N --guild-practice N --work-bench "
                     "CALLS --work-probes N --work-subjects N --work-shown N --work-test-seed N --work-origin "
                     "random|reference --quarry-moves N --quarry-exposure N\n";
        std::cout << "  --shoal-seasons N --shoal-trials N --shoal-span 3..5 --shoal-moves 1..32 --shoal-ideas 32..512 "
                     "--shoal-practice N --shoal-exposure TICKS --shoal-sample TRIAL --construction-tide TICKS "
                     "--site-replay on|off --field-sight own|nearby --light-regime diffuse|veined|mosaic --shoal-bench "
                     "DIAGRAMS --shoal-origin juvenile|resident\n";
        std::cout
            << "  --worksites active|quiet --work-traces visible|hidden|scrambled|ancestral|ancestral-shuffled "
               "--wash-sites --rest-hands --field-transfer TRIALS --field-min-age T --field-max-age T --assay-inquiry --substrate "
               "bound|mixed|scattered|loose --press-choice learned|unbiased --worksite-bench DIAGRAMS\n";
        std::cout << "  --brine 2..5 --brine-flow still|current --brine-sense open|muted --brine-plan empirical|latent "
                     "--brine-competition "
                     "--brine-rear SEASONS --brine-tides T --brine-shore linger|wash|survey|consult|strand|scout\n"
                  << "  --brine-selection provisioned|fostered --brine-origin random|resident --brine-examples 0..12 "
                     "--brine-assay PAIRS --brine-test-solutes 2..5 --brine-bench CALLS\n";
        std::cout << "  --press live|still|no-casting|alone|no-taste|unguided --press-study TRIALS --press-bench N\n"
                  << "  --press-geology mixed|veins|tidal --press-feel quiet|live|displaced|stiff --press-seasons N --press-population N --press-brood paid|uniform|finite --press-founders residents|varied --press-practice observed|mixed --press-seed N --press-census --press-scholar MIND\n"
                  << "  --hollows live|unguided|inert|still --hollow-odometry actual|quiet|rotated --hollow-landmarks visible|covered --hollow-ground sparse|rich --hollow-current sheltered|ferry --hollow-contest off|paired|reciprocal|sham|crossed|reciprocal-crossed --hollow-slots 1..32 --hollow-census\n"
                  << "  --hollow-rear SEASONS --hollow-population POWER_OF_TWO --hollow-brood earned|uniform --hollow-founders varied|residents --hollow-optics MIND --hollow-germs FLOOD --hollow-seed N\n"
                  << "  --hollow-study TRIALS --hollow-bench QUERIES --hollow-scholar MIND [--hollow-carrier MIND]\n";
        std::cout<<"  --wicks flowing|confined|still --wick-seed N --wick-census --wick-study LENGTH\n";
        std::cout
            << "  --braids live|stationary|private|deaf|fixed|isolated|silent --braid-mesh built|continuous|twisted "
               "--capillary-field off|live|watch|fixed|lookup --capillary-food direct|pigments --capillary-cargo still|pressure|displaced --capillary-cargo-dye fixed|exchange --capillary-cargo-events FILE --capillary-cargo-proof FILE --capillary-dispatch scheduled|full --capillary-period 1..256 --capillary-mineral regional|0..31\n"
               "  --capillary-events FILE --capillary-film FILE --capillary-window N --capillary-frames N --capillary-delay 1..16\n"
               "  --capillary-pupil MIND | --capillary-residents N --capillary-trials PAIRS --capillary-views N --capillary-probes N\n"
               "  --capillary-arm active|passive|random|crossed|yoked|severed|frozen|lookup|erased|miswired|backwards --capillary-supply plentiful|scarce\n"
               "  --capillary-depth 2..14 --capillary-moves 0..6 --capillary-works assembled|living --capillary-history retained|empty --capillary-memory adaptive|single|regional --capillary-catalysis bound|inert --chemical-foraging live|fixed|off --capillary-catalysis-proof FILE --catalytic-film FILE --chemical-witness FILE --capillary-trace FILE --capillary-benchmark N\n"
               "  --capillary-vocabulary N --capillary-recipes N --capillary-interviews N --capillary-discourse FILE --capillary-save MIND\n"
               "  --capillary-partner MIND --capillary-meetings N --capillary-instructions N --capillary-instruction-trace FILE\n"
               "  --capillary-hearing intact|exchanged|wrong-name|closed|reversed|mute\n"
               "  --material-hands off|live|fixed|disconnected --hand-events FILE\n"
               "  --catalytic-voices N --catalytic-dialogues N --catalytic-writer-dialect 0|1 --catalytic-reader-dialect 0|1 --catalytic-audio clear|silent|partial|reordered|duplicate|unknown|blind|patchy --catalytic-questions 0..2 --catalytic-reply actual|withheld|exchanged --catalytic-speech-trace FILE\n"
               "  --catalytic-channels 1|2 --catalytic-repeat N --catalytic-profiles FILE --catalytic-benchmark N --catalytic-omit 0..3 --catalytic-arbor native|joint --catalytic-diet single|plumes|pooled --catalytic-wean N\n"
               "  --catalytic-tutor sender|receiver MIND --catalytic-learning both|sender|receiver --catalytic-neighbour MIND\n"
               "  --catalytic-pair SENDER RECEIVER --catalytic-history INDEX --catalytic-test-seed N --catalytic-trace FILE --catalytic-export SENDER RECEIVER\n"
               "  --capillary-residents N --catalytic-lessons N --catalytic-histories N --catalytic-tests N --catalytic-prior empty|lived --catalytic-feedback food|silent|unrelated --catalytic-medium active|inert --capillary-screen FILE\n"
               "  --context-streams N --context-episodes N --context-change 0..100 --context-questions fixed|varied --context-view-jitter N --context-save MIND --context-trace FILE\n"
               "  --context-arm adaptive|single|cleared|unchecked|clamped|exchanged|retention --context-transfer-rock 0..31 --context-transfer-at EPISODE\n"
               "--braid-channels off|live|water|deaf|fixed|shifted --braid-phase 0..3 --braid-seed N --braid-cases N "
               "--braid-lessons N --braid-words N --braid-length N --braid-rig simple|coupled --braid-convention "
               "forward|varied|commuting\n"
            << "  --braid-nursery workshop|lineage|filters --braid-villages N --braid-generations N --braid-visits N "
               "--braid-village N --braid-history 0|1 --braid-chorister MIND --braid-songbook FILE\n"
            << "  --braid-dialogues N --braid-questions 0..4 --braid-panel plain|striped|distinct --braid-workers "
               "BASIN --braid-transcript FILE\n"
            << "  --brain-dispatch auto|fixed|packed --brain-bench TICKS --brain-memory auto|dense|paged "
               "--brain-pool-pages N --brain-collect TICKS --brain-file dense|compact\n"
            << "  --survey-origin SIDE measures settlement outside a prior square of this side\n"
            << "  --drift tidal|steady|off --drift-tide TICKS --drift-stride 1|2|4|8 --drift-load N\n  --drift-memory "
               "live|quiet|unread|fixed|naive --drift-works mutable|fixed --drift-fibres conduct|bypass\n  "
               "--contact-remodel inherited|gated --drift-cargo loose|rafts --drift-sight own|audience|off "
               "--watch-drift surprise|confirmed|untried|catchment --drift-witness-sample "
               "all|calibrated|carriers|genotype --drift-look N\n  "
               "--drift-diary FILE --drift-diary-ticks N --drift-questions FILE\n  --drift-contest "
               "reciprocal|swapped|lateral|reverse-lateral|vision|reverse-vision|sham|off --drift-stock MIND "
               "--drift-label SEED\n  "
               "--drift-surfaces N "
               "--drift-surface-seed SEED --drift-legacy-trace FILE --drift-account auto|warp|atomic|hybrid "
               "--drift-profile N\n  --drift-repertoire FILE --drift-germ FILE --drift-probe LIVES --drift-food-germ "
               "FILE --drift-probe-trace FILE\n  --drift-pool HABITAT --drift-rival HABITAT --drift-pool-order ab|ba "
               "--drift-transfer cortex|hand|both|lateral|all\n  --drift-trace FILE --drift-window TICKS --drift-tiles "
               "N\n  "
               "--shore-school FILE --shore-pupils N --shore-channel teaching|quiet|exam --shore-trace FILE\n  "
               "--feedback-donor FILE --receive-donor FILE --receive-count N --receive-sample random|calibrated "
               "--trace-donor FILE\n  --feedback-ports open|quiet|closed --feedback-survey N --feedback-dialects N "
               "--feedback-enrollment all|carriers --pupil-born-after TICK --feedback-survey-trace FILE\n  "
               "--feedback-generations N --feedback-cohort N --feedback-loci module|all --feeding-trace FILE\n  "
               "--feedback-class N --feedback-frames 2|4 --feedback-motor native|cortical --feedback-rig fixed|varied "
               "--feedback-convention stable|reversed --feedback-trace FILE --feedback-program FIRST SECOND\n  "
               "--field-retest N --slot SLOT --class-history SEED\n  --watch-readers TICKS --field-class N "
               "--class-sample all|connected --class-practice N --class-seed N --class-trace FILE\n  --return-lesson "
               "or --return-practice "
               "ORIGINAL_MIND TAUGHT_MIND --follow-witness ORIGINAL_MIND --export-mind OUTPUT --field-questions "
               "FILE|-\n"
            << "  --daylight off|live|displaced|shutter --glint-sources other|independent --daylight-period TICKS "
               "--daylight-banks single|tidal "
               "--daylight-gaze motor|inherited --daylight-probes ambient|stirred|crossed|lineages|reverse-lineages "
               "--daylight-audience off|visible|exchanged|lineages|reverse-lineages --audience-trials N "
               "--audience-guide MIND --audience-pupil MIND "
               "--audience-arm intact|exchanged|screened|muted|reversed|disguised|hidden|private --audience-rounds N "
               "--audience-broods N --audience-circuit FILE --audience-write FILE --audience-inherit FILE "
               "--audience-trace FILE "
               "--audience-timbre MIND --audience-medium tracer|speech --audience-access split|common|neither|mixed|unequal "
               "--audience-juveniles N "
               "--audience-lessons N --audience-lesson-seed SEED --audience-pairing live|shifted|fixed "
               "--audience-lesson-trace FILE "
               "--daylight-ink off|pigments|chorus|reversed|dialects --ink-trace FILE --ink-follow SLOT --ink-events FILE\n"
               "  --story-pupil MIND --story-mentor MIND --story-trials N --story-length 1..7 --story-language river|reverse|varied|native "
               "--story-singles N --story-pairs N --story-references N --story-reference-trials N --story-save MIND --story-trace FILE "
               "--story-reference-trace FILE --story-benchmark N --story-motor plastic|packed|table\n"
               "  --story-residents N --story-min-events N --story-screen FILE --story-broods N --story-write FILE --story-inherit FILE\n"
               "  --call-transmissions N --call-practice N --call-exposure paired|crossed|cut --call-neighbor GERMLINE --call-trace FILE\n"
               "  --probe-trace FILE --probe-sampling sites|events --daylight-profile N --shore-depth "
               "open|shoals|submerged --drift-windings "
               "open|woven\n"
               "  --record-watercourses FILE --watercourse-sites N --watercourse-sample all|wound\n  --retina-works "
               "FILE --retina-order spoken|woven\n  --retina-trials N --retina-laps 1..32 --retina-beds 1|2|4 "
               "--retina-banks inherited|shared --retina-rear-laps 1..32 --retina-channel "
               "live|displaced|independent|shutter|closed\n"
               "  --retina-gaze passive|inherited|yoked|visible-yoke|natal|open|closed --gaze-purse N --gaze-foraging "
               "1..32\n  --retina-probes ambient|chosen|crossed|random|visible-match\n  --retina-deliberate N "
               "--inquiry-hand chosen|random|crossed|visible-match --inquiry-context heard|reversed\n  "
               "--retina-stimulus water|tone --retina-contrast "
               "natural|bright --retina-host MIND\n"
               "  --retina-trace FILE --retina-table FILE --retina-born-after TICK --retina-germ FILE --retina-donor "
               "FILE\n  --retina-questions FILE --retina-hearing live|reverse|mute --retina-terms FILE --retina-voice "
               "FILE\n"
            << "  --drift-crossing exchange|seep --drift-stores open|sills --drift-first SLOTS\n"
               "  --drift-lateral local|inherited|misaligned|contextual|misaligned-contextual\n"
               "  --inlet-trials LAYOUTS --inlet-practice REACHES --inlet-prior REACHES --inlet-old-inlets three|all\n"
               "  --inlet-landscape field|channels|basin|mixed --inlet-residents N --inlet-background quiet|living "
               "--inlet-history empty|living\n"
               "  --inlet-generations N --inlet-cohort N --inlet-bolus FOOD --inlet-selection food|uniform\n"
               "  --inlet-table FILE --inlet-trace FILE --inlet-germ FILE --inlet-host MIND --inlet-dialects N\n"
               "  --inlet-vocal-law host|shared --inlet-cross none|dendrites|hand --receive-loci "
               "cortex|hand|dendrites|all\n"
               "  --receive-architecture replace|branch --voice-route shared|inherited\n"
               "  --inlet-questions FILE --inlet-voice-trace FILE --inlet-state fresh|carried\n"
            << "  --gate-trial crossover|sham --gate-phase 0|1|2 --gate-seed N --gate-trace FILE\n"
            << "  --braid-perspectives N --braid-views 1..12 --braid-view-budget 0..4 --braid-view-trace FILE\n"
            << "  --braid-eyes shared|local --braid-sediment mobile|pinned|bypass --braid-episodes --braid-inquiries N "
               "--braid-examples N --braid-holdout 1|2 --braid-report-trace FILE\n"
            << "  --braid-epochs N --braid-parents N --braid-selection food|uniform --braid-stock BASIN "
               "--braid-speaker MIND [--slot N] --braid-trace FILE --braid-bench N\n";
        std::cout<<"  --rattles live|deaf|passive|fixed|scrambled|rest --rattle-phase 0..3 --rattle-listen both|one --rattle-binding local|crossed --rattle-load loose|silted --rattle-seed N --rattle-trials N --rattle-exposure N --rattle-trace FILE --rattle-bench N --rattle-lesion none|eligibility|precision --rattle-visitors BASIN\n"
                 <<"  --rattle-broods N --rattle-parents N --rattle-selection food|uniform --rattle-nursery both|one|chorus|solo-chorus --rattle-stock BASIN --rattle-speaker MIND [--rattle-carrier MIND]\n";
        std::cout<<"  --reeds live|private|blind|rest|scrambled --reed-tide breathing|quiet --reed-current shaped|free|reversed --reed-reason direct|relational --reed-grain straight|handed --reed-strata SEED --reed-feeding sip|leave --reed-contest off|listen|reciprocal|sham|reason|reverse-reason --reed-period T --flatten-reeds --reed-cut observed|matched|sham --reed-census\n"
                 <<"  --reed-rear GENERATIONS --reed-population N --reed-brood earned|uniform --reed-germs FLOOD --reed-seed N --reed-rig LENGTH --reed-study CASES --reed-chain GENERATIONS --reed-span LENGTH --reed-practice N --reed-attention offered|invited --reed-observe TURNS --reed-keepers CASES --reed-life T --reed-patrol free|learned --reed-nursery direct|transfer|mixed|varied|keepers --reed-analogy CASES --reed-trace NEW_TSV --reed-examples N --reed-bench CALLS --reed-scholar MIND --reed-carrier MIND\n";
        std::cout<<"  --practice live|scattered|persistent|still --practice-banks 1..4 --practice-probes chosen|random|quiet --practice-context multiple|one --practice-sight shared|private|displaced\n"
                 <<"  --practice-appetite responsive|inherited|shifted --practice-habitat fixtures|foraging|mixed --practice-landscape island|frontier --practice-work learned|chance --practice-fibre gather|carried --unthread-works --practice-contest off|paired|reciprocal|sham --practice-census --practice-study N --practice-bench N\n"
                 <<"  --practice-rear SEASONS --practice-population N --practice-brood earned|uniform --practice-seed N --practice-germs FLOOD --practice-scholar MIND [--practice-carrier MIND]\n";
        std::cout << "  --linkage live|unstudied|unplanned|inert|scrambled --linkage-building inferred|exploratory|fixed --linkage-sight public|private|shifted --linkage-joints linked|loose --linkage-flow open|sealed --linkage-attention focal|whole\n"
                  << "  --linkage-contexts 1|2|4 --linkage-period 1|2|4|8|16|32|64 --linkage-contest off|paired|reciprocal|sham --disturb-linkages --linkage-census --linkage-bench N\n"
                  << "  --linkage-rear SEASONS --linkage-population N --linkage-brood earned|uniform --linkage-seed N --linkage-law MIND --linkage-germs FLOOD --linkage-scholar MIND [--linkage-carrier MIND]\n"
                  << "  --linkage-weather steady|wet --linkage-climate separate|constant|changing --linkage-recall CASES --linkage-study CASES --linkage-witnesses CASES --linkage-pieces 1..4 --linkage-practice 1..8\n";
        std::cout << "  --effort live|always|fixed|reversed|shuffled|no-taste|no-strain --effort-aim multiple|single --effort-trails soft|rigid --effort-sculpt live|footsteps --effort-paving forecast|habit|foreign --wash-treads --effort-weather TICKS\n"
                  << "  --effort-audit randomized|quiet --effort-window 1..64 --effort-movement impulse|select --effort-assignments same|resample --effort-contest off|paired|reciprocal|sham --effort-census --effort-study N --effort-bench N\n"
                  << "  --effort-rear N --effort-population N --effort-practice trips|mixed|building --effort-brood earned|uniform --effort-seed N --effort-germs FLOOD --effort-scholar MIND [--effort-carrier MIND]\n";
        std::cout << "  --pottery fired|sheltered|weathered --clay-reach single|rake|shared --clay-hands "
                     "both|read|write|rest --clay-muscle live|quiet --clay-context full|visible|hidden|pooled\n"
                  << "  --clay-sound clear|permuted|ancestral|ancestral-permuted --wash-clay --clay-germs HABITAT\n"
                  << "  --clay-rear SEASONS --clay-selection provisioned|uniform --clay-test QUARTETS --clay-classes N "
                     "--clay-preparation PAID_CONTACTS --clay-history CONTACTS\n"
                  << "  --clay-lessons TOUCHES --clay-taps N --clay-teachers parallel|unrelated --clay-seed N\n"
                  << "  --clay-dig QUARTETS --clay-foster COHORTS --clay-reversals ENCOUNTERS --clay-bench QUERIES "
                     "--export-clay FILE --clay-site CELL\n"
                  << "  --clay-specimen PREFIX --clay-settlers MIND --clay-peers PREFIX\n";
        std::cout << "  --siphons 1|2|4 --siphon-wire open|shut|scrambled|shifted --siphon-windows tidal|dark|clear "
                     "--siphon-period TICKS\n"
                  << "  --siphon-rear SEASONS --siphon-visits CONTACTS --siphon-selection provisioned|fostered "
                     "--siphon-origin random|resident --siphon-trials QUARTETS --siphon-practice CONTACTS\n"
                  << "  --siphon-view covered|intermittent|open --siphon-medium air|resonant --siphon-development "
                     "graded|arbitrary --siphon-clutch mixed|siblings\n"
                  << "  --siphon-audience open|alone|shifted|scrambled|ancestral|ancestral-scrambled "
                     "--siphon-resonance brief|material --wash-sounds\n"
                  << "  --siphon-germs HABITAT --siphon-workers live|still --siphon-census QUARTETS --siphon-legacy "
                     "TICKS --siphon-reference HABITAT --siphon-bench CONTACTS\n"
                  << "  --siphon-foster GENERATIONS --siphon-exposures CONTACTS --siphon-mentors plastic|fixed\n";
        return 0;
    }
    auto o = parse(argc, argv);
    if(!o.link_carrier.empty() && o.link_scholar.empty())throw std::invalid_argument("a linkage carrier requires an exported scholar");
    if(!o.effort_carrier.empty() && o.effort_scholar.empty())throw std::invalid_argument("an effort carrier requires an exported scholar");
    if (o.paired_material && (o.colonists.empty() || o.competitors.empty() || o.cross))
        throw std::invalid_argument("a matched material competition needs both germline pools and their common body");
    if(!o.reed_reference.empty() && !o.reed_legacy)throw std::invalid_argument("a filter reference needs --siphon-legacy TICKS");
    Habitat h;
    bool restored = !o.load.empty();
    if (restored)
        h.read(o.load);
    else
        h.header.configuration = o.c;
    std::unique_ptr<storage::AgentImage> visitor;
    if (!o.resident.empty()) {
        if (restored || !o.colonists.empty() || !o.founder.empty())
            throw std::invalid_argument("rehousing starts in a new habitat");
        visitor = std::make_unique<storage::AgentImage>();
        visitor->read(o.resident);
        if (!visitor->riverine)
            throw std::invalid_argument("this checkpoint has no floodplain body");
        h.header.tick = visitor->resident.tick;
        h.header.configuration.tiles = o.explicit_cells ? o.c.tiles : visitor->brain.tiles;
        if (h.header.configuration.tiles < visitor->brain.tiles)
            throw std::invalid_argument("a smaller neural arena would discard the visitor's tissue");
        if (h.header.configuration.tiles > visitor->brain.tiles && !visitor->grows_cells && o.neurogenesis != 1)
            throw std::invalid_argument("enlarge a legacy resident's arena with --neurogenesis inherited");
    }
    bool prior_minerals = h.articulated;
    if (!o.hand_germ.empty()) {
        if (prior_minerals || visitor || !o.colonists.empty())
            throw std::invalid_argument("graft a hand germline onto a habitat without existing hands");
        storage::AgentImage donor;
        donor.read(o.hand_germ);
        if (!donor.handling)
            throw std::invalid_argument("donor has no manipulation genome");
        h.inherit_hands = true;
        h.hand_donor = donor.worker.genes;
    }
    std::unique_ptr<Habitat> soil_source;
    if (!o.soil.empty()) {
        soil_source = std::make_unique<Habitat>();
        soil_source->read(o.soil);
        if (soil_source->sites() != h.sites())
            throw std::invalid_argument("soil transplant needs equally sized floodplains");
        if (!h.sedimentary && soil_source->sedimentary) {
            h.sedimentary = true;
            h.seam.pattern = soil_source->seam.pattern;
            h.seam.guidance = soil_source->seam.guidance;
        }
        if (!h.investigatory && soil_source->investigatory) {
            h.investigatory = true;
            h.worksite.active = soil_source->worksite.active;
            h.worksite.marks = soil_source->worksite.marks;
        }
    }
    if(!o.reed_trace.empty()&&!o.reed_general)throw std::invalid_argument("a reed trace needs a held-out analogy study");
    if(!h.braiding)h.loom.seed=o.braid_seed;
    h.recording_floats=h.recording_floats||o.braid_episodes||o.braid_eyes==braid::individual_eyes||(visitor&&visitor->narrates_floats);
    if(!o.braid_view_trace.empty()&&!o.braid_perspectives)throw std::invalid_argument("optical comparisons need --braid-perspectives");
    if (o.neural_file >= 0)
        h.short_neural_rows = bool(o.neural_file);
    if(!o.braid_report_trace.empty()&&!o.braid_inquiries)throw std::invalid_argument("reported events need --braid-inquiries");
    h.braiding = h.braiding || (o.cap_field!=~0u && o.cap_field) || !o.drift_stock.empty() || (o.feedback_ports != ~0u && o.feedback_ports) ||
                 o.braid_eyes != ~0u || h.recording_floats || o.braid_perspectives || o.braid_sediment != ~0u ||
                 o.braid_inquiries || o.braid_channels != ~0u || o.braid_phase != ~0u || o.braid_mode != ~0u ||
                 o.braid_mesh != ~0u || o.braid_villages || !o.braid_chorister.empty() || o.braid_dialogues ||
                 o.braid_epochs || o.braid_cases || o.braid_bench || !o.braid_voice.empty() ||
                 !o.braid_source.empty() || (visitor && visitor->binds_floats) ||
                 (soil_source && soil_source->braiding);
    auto &current = h.drift_water.law;
    bool previous_drift_contest = current.ports & drift::contest;
    if (o.drift_mode != ~0u) {
        if (!(current.ports & drift::moving) && o.drift_mode)
            current.ports = drift::live | drift::regrowth;
        current.ports = (current.ports & ~Word(drift::moving | drift::tide)) | o.drift_mode;
    }
    if (o.daylight != ~0u) {
        if (!(current.ports & retina::channel_mask) && o.daylight)
            current.ports = (current.ports & ~retina::cadence_mask) | (3u << retina::cadence_shift);
        current.ports = (current.ports & ~retina::channel_mask) | (o.daylight << retina::shift);
        if (!o.daylight)
            current.ports &= ~retina::cadence_mask;
    }
    if (o.glint_sources != ~0u)
        current.ports = (current.ports & ~Word(drift::independent_glints)) |
                        (o.glint_sources ? Word(drift::independent_glints) : 0u);
    if (o.daylight_period != ~0u)
        current.ports = (current.ports & ~retina::cadence_mask) | (o.daylight_period << retina::cadence_shift);
    if (o.daylight != ~0u && o.daylight && !(current.ports & drift::moving))
        throw std::invalid_argument("daylight on flowing water needs a moving current");
    if (o.drift_lateral != ~0u)
        current.ports =
            (current.ports & ~Word(drift::lateral | drift::misaligned | drift::recruitment)) | o.drift_lateral;
    if (o.drift_seep != ~0u)
        current.ports = (current.ports & ~Word(drift::seepage)) | o.drift_seep;
    if (o.drift_windings != ~0u)
        current.ports = (current.ports & ~Word(drift::winding)) | (o.drift_windings ? Word(drift::winding) : 0u);
    if (o.light_banks != ~0u)
        current.ports = (current.ports & ~Word(drift::tidal_banks)) | (o.light_banks ? Word(drift::tidal_banks) : 0u);
    if (o.shore_depth != ~0u)
        current.ports = (current.ports & ~Word(drift::bottom_depth | drift::shoals)) |
                        (o.shore_depth ? Word(drift::bottom_depth) : 0u) |
                        (o.shore_depth == 2 ? Word(drift::shoals) : 0u);
    if (o.drift_stores != ~0u)
        current.ports = (current.ports & ~Word(drift::storage)) | o.drift_stores;
    if (o.receive_branch)
        current.ports |= drift::auditory;
    if (o.voice_route != ~0u)
        current.ports = (current.ports & ~Word(drift::auditory)) | o.voice_route;
    if (o.drift_contest != ~0u)
        current.ports = (current.ports & ~Word(drift::contest | drift::swapped | drift::sham | drift::lateral_contest |
                                               drift::visual_contest)) |
                        o.drift_contest;
    bool mark_drift = (current.ports & drift::contest) && (!previous_drift_contest || o.drift_label != ~0u);
    if (o.drift_label != ~0u && !(current.ports & drift::contest))
        throw std::invalid_argument("a population label needs a drift contest");
    if (mark_drift) {
        if (o.drift_label != ~0u)
            h.drift_water.tag_seed = o.drift_label;
        h.drift_water.marked_at = h.header.tick;
        h.drift_water.pedigree = {};
    }
    if (o.drift_memory != ~0u)
        current.ports = (current.ports & ~Word(drift::recall | drift::plastic | drift::feedback)) | o.drift_memory;
    if (o.drift_works != ~0u)
        current.ports = (current.ports & ~Word(drift::building)) | o.drift_works;
    if (o.drift_fibres != ~0u)
        current.ports = (current.ports & ~Word(drift::fibres)) | o.drift_fibres;
    if (o.drift_sight != ~0u)
        current.ports = (current.ports & ~Word(drift::sight | drift::audience)) | o.drift_sight;
    if (o.drift_watch || !o.drift_diary.empty()) {
        if (!(current.ports & drift::moving))
            throw std::invalid_argument("a current witness needs drifting food");
        current.ports |= drift::sight;
    }
    if ((o.drift_windings == 1 || o.daylight_profile) && !(current.ports & drift::moving))
        throw std::invalid_argument("a winding or optical measurement needs a moving current");
    if (o.drift_sight != ~0u && o.drift_sight && !(current.ports & drift::moving))
        throw std::invalid_argument("observing drifting food needs a moving current");
    if ((current.ports & (drift::moving | drift::sight)) == (drift::moving | drift::sight))
        h.recording_floats = true;
    if (o.contact_remodel != ~0u)
        current.ports = (current.ports & ~Word(drift::regrowth)) | (o.contact_remodel ? Word(drift::regrowth) : 0u);
    if (o.drift_cargo != ~0u)
        current.ports = (current.ports & ~Word(drift::rafting)) | o.drift_cargo;
    if (o.drift_tide != ~0u)
        current.tide_shift = o.drift_tide;
    if (o.drift_stride != ~0u)
        current.stride_shift = o.drift_stride;
    if (o.drift_load != ~0u)
        current.load = o.drift_load;
    if (current.ports & (drift::moving | drift::contest | drift::auditory | drift::tidal_banks | drift::bottom_depth)) {
        h.braiding = true;
        h.loom.version = std::max(h.loom.version, 7u);
    }
    if (!(current.ports & drift::moving) && (!o.drift_trace.empty() || o.drift_surfaces || o.drift_first))
        throw std::invalid_argument("a drifting-food trace needs a moving current");
    if (o.field_probes != ~0u) {
        if (o.field_probes && !(current.ports & retina::channel_mask))
            throw std::invalid_argument("submerged tracers need illuminated water");
        h.loom.mode &= ~(retina::sounding_port | retina::crossed_hands | retina::lineage_hands);
        if (o.field_probes) {
            h.loom.version = std::max(h.loom.version, 8u);
            h.loom.mode |= retina::sounding_port;
            if (o.field_probes == 2 || o.field_probes == 4)
                h.loom.mode |= retina::crossed_hands;
            if (o.field_probes >= 3) {
                if (!(current.ports & drift::contest))
                    throw std::invalid_argument("reciprocal probe coupling needs marked field lineages");
                h.loom.mode |= retina::lineage_hands;
            }
        }
    }
    if(o.cap_field!=~0u && (o.cap_field || h.loom.version>=14)){
        if(h.loom.version<14){h.cap_law.raw_frame=h.loom.version==10||h.loom.version==11;h.loom.version=14;}
        h.cap_law.mode=o.cap_field?(h.cap_law.mode&~7u)|o.cap_field:0;
    }
    if(o.cap_food!=~0u && !(h.cap_law.mode&7))throw std::invalid_argument("sorting pigments needs a running capillary field");
    if(o.cap_food!=~0u)h.cap_law.mode=(h.cap_law.mode&~8u)|(o.cap_food<<3);
    if(o.cap_cargo!=~0u){
        if(o.cap_cargo&&!(h.cap_law.mode&7))throw std::invalid_argument("moving detritus needs an active pressure field");
        h.cap_law.mode=(h.cap_law.mode&~detritus::modes)|o.cap_cargo;
        if(!o.cap_cargo)h.cap_law.mode&=~detritus::exchanges;
        if(o.cap_cargo)h.loom.version=std::max(16u,h.loom.version);
    }
    if(o.cap_dye!=~0u){
        if(o.cap_dye && !(h.cap_law.mode&detritus::modes))throw std::invalid_argument("exchanging grain pigments requires physical food transport");
        h.cap_law.mode=(h.cap_law.mode&~detritus::exchanges)|(o.cap_dye?detritus::exchanges:0u);
    }
    if(o.hand_mode!=~0u){
        if(o.hand_mode&&!(h.cap_law.mode&7))throw std::invalid_argument("material grasping needs an open pressure field");
        h.cap_law.mode&=~(capillary::material_hands|capillary::fixed_hands|capillary::disconnected_hands);
        if(o.hand_mode){h.cap_law.mode|=capillary::material_hands;
            if(o.hand_mode==2)h.cap_law.mode|=capillary::fixed_hands;
            if(o.hand_mode==3)h.cap_law.mode|=capillary::disconnected_hands;
            h.loom.version=std::max(24u,h.loom.version);}
    }
    if(!o.hand_events.empty()&&(!o.steps||!(h.cap_law.mode&capillary::material_hands)))throw std::invalid_argument("sample material handling in a running habitat");
    if(o.cap_foragers!=~0u){
        if(o.cap_foragers&&!(h.cap_law.mode&7))throw std::invalid_argument("open a pressure field for chemical foraging");
        h.cap_law.mode=(h.cap_law.mode&~(capillary::foraging_ports|capillary::fixed_appetite))|(o.cap_foragers?capillary::foraging_ports:0u)|(o.cap_foragers==2?capillary::fixed_appetite:0u);
        if(o.cap_foragers)h.loom.version=std::max(20u,h.loom.version);
    }
    if(o.cap_catalysis!=~0u){
        if(o.cap_catalysis && !(h.cap_law.mode&7))throw std::invalid_argument("surface catalysis requires an active pressure field");
        h.cap_law.mode=(h.cap_law.mode&~capillary::catalytic_surface)|(o.cap_catalysis?capillary::catalytic_surface:0);
        if(o.cap_catalysis)h.loom.version=std::max(19u,h.loom.version);
    }
    if(o.cap_context!=~0u){
        if(o.cap_context && !(h.cap_law.mode&7) && !((!o.cap_pupil.empty()||o.cap_members)&&!o.steps))throw std::invalid_argument("expressing material context in the habitat requires a running pressure field");
        if(h.cap_law.mode&7){
            h.cap_law.mode=(h.cap_law.mode&~(capillary::contextual|capillary::regional_synapses))|(o.cap_context?capillary::contextual:0u)|(o.cap_context==3?capillary::regional_synapses:0u);
            if(o.cap_context==3)h.loom.version=std::max(18u,h.loom.version);
            if(o.cap_context)h.loom.version=std::max(17u,h.loom.version);
        }
    }
    if(!o.chemical_witness.empty()&&(!o.steps||!(h.cap_law.mode&capillary::foraging_ports)))throw std::invalid_argument("observe a running pair of chemical foraging ports");
    if(!o.catalytic_film.empty()&&(!o.steps || !(h.cap_law.mode&capillary::catalytic_surface)||o.cap_frames<2))throw std::invalid_argument("sample consecutive beats in an active catalytic field");
    if(!o.context_events.empty()&&(!(h.cap_law.mode&capillary::contextual)||!o.steps||(h.cap_law.mode&7)==4))
        throw std::invalid_argument("record context decisions in a running habitat with the adaptive pathway connected");
    if(!o.cargo_events.empty()&&(!(h.cap_law.mode&detritus::modes)||!o.steps))throw std::invalid_argument("record food transport during at least one habitat tick");
    if(o.cap_period!=~0u)h.cap_law.shift=o.cap_period;
    if(o.cap_vein!=~0u)h.cap_law.mineral=o.cap_vein;
    if((o.cap_vein!=~0u || o.cap_period!=~0u || !o.cap_events.empty() || !o.cap_film.empty()) && !h.cap_law.mode)
        throw std::invalid_argument("run a capillary watershed for this treatment");
    if (o.field_ink!=~0u) {
        h.loom.mode&=~(utterance::ink_port|utterance::voice_port|utterance::crossed_port|utterance::stored_vocal_feedback);
        if (o.field_ink) {
            if (!(h.loom.mode&retina::sounding_port)) throw std::invalid_argument("ink carriers need two stirred inlets");
            h.loom.mode|=utterance::ink_port|(o.field_ink>=2?utterance::voice_port:0u)|(o.field_ink==3?utterance::crossed_port:0u)|(o.field_ink==4?utterance::stored_vocal_feedback:0u);
            h.loom.version=h.loom.version<10?12:std::max(11u,h.loom.version);
            if(o.field_ink==4 && h.loom.version==12)h.loom.version=13;
        }
    }
    if (!(h.loom.mode&retina::sounding_port))h.loom.mode&=~(utterance::ink_port|utterance::voice_port|utterance::crossed_port|utterance::stored_vocal_feedback);
    if(visitor && visitor->hydraulic && h.loom.version<14){h.cap_law.raw_frame=h.loom.version==10||h.loom.version==11;h.loom.version=14;}
    if(visitor&&visitor->voices_signs)h.loom.version=std::max(23u,h.loom.version);
    if(visitor&&visitor->compound_signals)h.loom.version=std::max(21u,h.loom.version);
    if(visitor&&visitor->material_signals)h.loom.version=std::max(20u,h.loom.version);
    if(visitor&&visitor->hydraulic&&visitor->channels.adaptive)h.loom.version=std::max(visitor->channels.adaptive==3?18u:17u,h.loom.version);
    if (visitor && visitor->describes) h.loom.version=std::max(visitor->accounts.tag==utterance::free_frame?11u:12u,h.loom.version);
    if (o.field_audience != ~0u) {
        h.loom.mode &= ~audience::field_ports;
        if (o.field_audience) {
            if (!(h.loom.mode & retina::sounding_port))
                throw std::invalid_argument("visible peers need stirred water");
            h.loom.version = std::max(h.loom.version, 9u);
            h.loom.mode |= audience::connected;
            if (o.field_audience == 2 || o.field_audience == 4)
                h.loom.mode |= audience::crossed;
            if (o.field_audience >= 3) {
                if (!(current.ports & drift::contest))
                    throw std::invalid_argument("reciprocal audience assignment needs marked lineages");
                h.loom.mode |= audience::lineage;
            }
        }
    }
    if (!(h.loom.mode & retina::sounding_port))
        h.loom.mode &= ~audience::field_ports;
    if (!(current.ports & drift::contest) && (h.loom.mode & audience::lineage))
        h.loom.mode &= ~(audience::lineage | audience::crossed);
    if (o.field_iris != ~0u) {
        if (o.field_iris && !(current.ports & retina::channel_mask))
            throw std::invalid_argument("inherited gaze needs illuminated water");
        h.loom.mode = (h.loom.mode & ~retina::iris_port) | (o.field_iris ? retina::iris_port : 0u);
        if (o.field_iris)
            h.loom.version = std::max(h.loom.version, 8u);
    }
    if(o.braid_mode!=~0u)h.loom.mode=(h.loom.mode&~255u)|o.braid_mode;
    if(o.braid_mesh!=~0u)h.loom.mode=(h.loom.mode&~Word(braid::open_mesh|braid::slipping))|o.braid_mesh;
    if(o.braid_channels!=~0u)h.loom.mode=(h.loom.mode&~31744u)|o.braid_channels;
    if(o.braid_sediment!=~0u){
        h.loom.mode=(h.loom.mode&~(braid::mineral_bed|braid::mobile_bed))|o.braid_sediment;
        if(o.braid_sediment && o.braid_channels==~0u)h.loom.mode|=braid::sluices;
        h.loom.version=std::max(4u,h.loom.version);
    }
    if(o.braid_eyes!=~0u){
        h.loom.mode=(h.loom.mode&~braid::individual_eyes)|o.braid_eyes;
        if(o.braid_eyes)h.loom.version=std::max(5u,h.loom.version);
    }
    if(o.feedback_ports!=~0u){
        h.loom.mode=(h.loom.mode&~(regulation::motor_port|regulation::silent_feedback))|o.feedback_ports;
        if(o.feedback_ports)h.loom.version=std::max(6u,h.loom.version);
        else
            h.loom.mode &= ~regulation::structural_port;
    }
    if(!o.feedback_donor.empty() && !o.feedback_generations)throw std::invalid_argument("a transferable donor needs a rearing population");
    if (o.contact_remodel != ~0u && (h.loom.mode & regulation::motor_port)) {
        h.loom.mode =
            (h.loom.mode & ~regulation::structural_port) | (o.contact_remodel ? regulation::structural_port : 0u);
        if (o.contact_remodel)
            h.loom.version = std::max(h.loom.version, 7u);
    }
    if(o.regulatory_override && !o.apprentices)throw std::invalid_argument("a regulatory graft needs a feedback class");
    if(!o.feedback_trace.empty() && !o.apprentices)throw std::invalid_argument("a language trace needs a feedback class");
    if(!o.feeding_trace.empty() && !o.feedback_generations)throw std::invalid_argument("a feeding trace needs nursery generations");
    if(o.braid_phase!=~0u)h.gate_phase=o.braid_phase;
    if(h.loom.mode&braid::sluices || o.braid_phase!=~0u || (visitor&&visitor->registers_gates))h.loom.version=std::max(3u,h.loom.version);
    if((!o.braid_transcript.empty()||!o.braid_workers.empty())&&!o.braid_dialogues)throw std::invalid_argument("a translated flight needs --braid-dialogues");
    if(!o.braid_song.empty()&&!o.braid_villages)throw std::invalid_argument("a songbook needs --braid-villages");
    if(!o.braid_trace.empty()&&!o.braid_cases)throw std::invalid_argument("a float trace needs --braid-cases");
    if(!h.rattling)h.backwater.seed=o.rattle_seed;
    h.rattling=h.rattling||o.rattle_mode!=~0u||o.rattle_phase!=~0u||!o.rattle_speaker.empty()||o.rattle_load!=~0u||o.rattle_binding!=~0u||o.rattle_sparse!=~0u||o.rattle_trials||o.rattle_calls||o.rattle_cycles||!o.rattle_stock.empty()||(visitor&&visitor->hears_banks);
    if(o.rattle_mode!=~0u)h.backwater.mode=(h.backwater.mode&Word(rattle::single|rattle::crossed|rattle::loaded))|o.rattle_mode;
    if(o.rattle_sparse!=~0u)h.backwater.mode=(h.backwater.mode&~rattle::single)|o.rattle_sparse;
    if(o.rattle_binding!=~0u)h.backwater.mode=(h.backwater.mode&~rattle::crossed)|o.rattle_binding;
    if(o.rattle_load!=~0u)h.backwater.mode=(h.backwater.mode&~rattle::loaded)|o.rattle_load;
    if(h.backwater.mode&(rattle::single|rattle::crossed))h.backwater.format=std::max(2u,h.backwater.format);
    if(h.backwater.mode&rattle::loaded)h.backwater.format=3;
    if(!o.rattle_carrier.empty()&&o.rattle_speaker.empty())throw std::invalid_argument("a speech carrier needs --rattle-speaker");
    if(o.rattle_phase!=~0u)h.backwater.phase=o.rattle_phase;
    if(!o.rattle_visitors.empty()&&!o.rattle_trials)throw std::invalid_argument("visiting bank strikers need --rattle-trials");
    if(o.rattle_lesion&&!o.rattle_trials)throw std::invalid_argument("a listener lesion needs --rattle-trials");
    if(!o.rattle_trace.empty()&&!o.rattle_trials)throw std::invalid_argument("a bank trace needs --rattle-trials");
    bool had_reeds=h.reeding;
    h.reeding=h.reeding||o.reed_patrol!=~0u||o.reed_keep||o.reed_cut!=~0u||o.reed_grain!=~0u||o.reed_strata||o.reed_contest!=~0u||o.reed_diet!=~0u||o.reed_general||o.reed_relation!=~0u||o.reed_mode!=~0u||o.reed_weather!=~0u||o.reed_flex!=~0u||o.reed_rig||o.reed_cases||o.reed_rear||o.reed_calls||o.fold_census||o.reed_strip||!o.reed_stock.empty()||!o.reed_scholar.empty()||(visitor&&visitor->folds_reeds)||(soil_source&&soil_source->reeding);
    bool reed_labels=(h.rush.mode&runnel::families)==0&&o.reed_contest!=~0u&&(o.reed_contest&runnel::families);
    if(o.reed_contest!=~0u)h.rush.mode=(h.rush.mode&~Word(runnel::families|runnel::swap_families|runnel::equal_families|runnel::compete_reason))|o.reed_contest;
    if(o.reed_patrol!=~0u)h.rush.mode=(h.rush.mode&~Word(runnel::patrolling))|o.reed_patrol;
    if(soil_source && soil_source->reeding){
        h.rush.mode=(h.rush.mode&~Word(runnel::handed))|(soil_source->rush.mode&runnel::handed);
    }
    if(o.reed_grain!=~0u)h.rush.mode=(h.rush.mode&~Word(runnel::handed))|o.reed_grain;
    if(o.reed_diet!=~0u)h.rush.mode=(h.rush.mode&~Word(runnel::responding))|o.reed_diet;
    if(h.reeding)h.rush.version=2;
    if(!had_reeds)h.rush.seed=o.reed_seed;
    if(soil_source && soil_source->reeding)h.rush.seed=soil_source->rush.seed;
    if(o.reed_strata)h.rush.seed=o.reed_strata;
    if(o.reed_mode!=~0u)h.rush.mode=(h.rush.mode&~Word(runnel::forming|runnel::watching|runnel::sensing|runnel::scrambled))|o.reed_mode;
    if(o.reed_weather!=~0u)h.rush.mode=(h.rush.mode&~Word(runnel::tide))|o.reed_weather;
    if(o.reed_flex!=~0u)h.rush.mode=(h.rush.mode&~Word(runnel::decoupled|runnel::reversed))|o.reed_flex;
    if(o.fold_period)h.rush.period=o.fold_period;
    if(o.reed_relation!=~0u)h.rush.mode=(h.rush.mode&~Word(runnel::relating))|o.reed_relation;
    if(!o.reed_carrier.empty()&&o.reed_scholar.empty())throw std::invalid_argument("reed carrier needs a scholar output");
    bool old_wicks=h.wicking;h.wicking=h.wicking||h.reeding||o.wick_mode!=~0u||o.wick_census||o.wick_length||(soil_source&&soil_source->wicking);
    if(h.wicking&&!old_wicks)h.wicking_state.seed=o.wick_seed;
    if(h.wicking&&!(h.wicking_state.mode>>8))h.wicking_state.mode|=h.header.configuration.side<<8;
    if(o.wick_mode!=~0u)h.wicking_state.mode=(h.wicking_state.mode&~3u)|o.wick_mode;
    bool had_practice=h.practising;
    h.practising=h.practising || o.practice_focus!=~0u || o.practice_mode!=~0u || o.practice_sight!=~0u || o.practice_join!=~0u || o.practice_resource!=~0u || o.practice_comp!=~0u ||
        o.practice_strip || o.practice_approach!=~0u || o.practice_credit!=~0u || o.practice_cap || o.practice_epochs || o.practice_test || o.practice_speed || o.practice_survey ||
        !o.practice_stock.empty() || !o.practice_scholar.empty() || (visitor&&visitor->practises) || (soil_source&&soil_source->practising);
    if(!o.practice_carrier.empty()&&o.practice_scholar.empty())throw std::invalid_argument("a practice carrier needs an exported scholar");
    Word new_cap=o.practice_cap?o.practice_cap:(!had_practice&&visitor&&visitor->practises?visitor->training.arena:h.learning.arena);
    if(had_practice && new_cap!=h.learning.arena){
        for(auto &m:h.pupils)if(practice::size(m)>practice::capacity(m.genes,new_cap))throw std::invalid_argument("the smaller arena discards acquired material hypotheses");
        h.memories.resize(Count(h.sites())*new_cap);
    }
    h.learning.arena=new_cap;if(!had_practice)h.learning.seed=o.practice_seed;
    auto set_practice=[&](Word mask,Word v){if(v!=~0u)h.learning.mode=(h.learning.mode&~mask)|v;};
    bool practice_labels=(h.learning.mode&practice::families)==0&&o.practice_comp!=~0u&&(o.practice_comp&practice::families);
    if(!had_practice)h.learning.mode|=practice::responsive_attention;
    set_practice(practice::responsive_attention|practice::misassigned_attention,o.practice_focus);
    set_practice(practice::engaging|practice::holding|practice::constant_stay,o.practice_mode);
    set_practice(practice::probing|practice::random_probes,o.practice_approach);set_practice(practice::contextual,o.practice_credit);
    set_practice(practice::social|practice::wrong_effect,o.practice_sight);set_practice(practice::crafting,o.practice_join);
    set_practice(practice::collecting,o.practice_resource);set_practice(practice::families|practice::reciprocal|practice::sham,o.practice_comp);
    bool inherited_links=h.linking;
    h.linking=h.linking||h.practising||o.link_water!=~0u||o.link_focus!=~0u||o.link_fit!=~0u||o.link_mode!=~0u||o.link_soc!=~0u||o.link_joints!=~0u||o.link_use!=~0u||o.link_comp!=~0u||o.link_arena||
        o.link_recall||o.link_period||o.link_epochs||o.link_test||o.link_census||o.link_strip||o.link_witness||o.link_bench||!o.link_origin.empty()||
        !o.link_scholar.empty()||!o.link_law.empty()||(visitor&&visitor->invents)||(soil_source&&soil_source->linking);
    if(soil_source && soil_source->linking)h.millwork.seed=soil_source->millwork.seed;
    else if(h.linking && !inherited_links)h.millwork.seed=o.link_seed;
    Word link_cap=o.link_arena;
    if(visitor&&visitor->invents&&!link_cap)link_cap=visitor->mechanism.arena;
    if(link_cap){
        if(!h.makers.empty() && link_cap!=h.millwork.arena){
            // Address masks can change after either expansion or contraction. Move
            // each retained tissue with its material key and reject collisions.
            auto prev=h.mechanisms;auto count=h.sites();std::vector<linkage::Tissue> next(Count(count)*link_cap);
            for(Word id=0;id<count;++id){
                auto &m=h.makers[id];Word before=m.tags;m.tags=0;
                for(Word j=0;j<h.millwork.arena;++j){Word tag=(before>>(4*j))&15;if(!tag)continue;
                    Word dest=linkage::address(m,tag-1,link_cap);
                    if(linkage::key_at(m,dest))throw std::invalid_argument("narrowing link contexts would discard an acquired material");
                    m.tags|=tag<<(4*dest);next[Count(dest)*count+id]=prev[Count(j)*count+id];
                }
            }
            h.mechanisms=std::move(next);
        }
        h.millwork.arena=link_cap;
    }
    if(o.link_period)h.millwork.period=o.link_period;
    bool old_link_labels=h.millwork.mode&linkage::mark_families;
    auto set_link=[&](Word mask,Word option){if(option!=~0u)h.millwork.mode=(h.millwork.mode&~mask)|option;};
    set_link(linkage::hydration,o.link_water);
    set_link(linkage::focal,o.link_focus);
    set_link(linkage::fitting|linkage::designing,o.link_fit);
    set_link(linkage::working|linkage::writable|linkage::planning|linkage::scrambled,o.link_mode);
    set_link(linkage::witnesses|linkage::other_eye,o.link_soc);set_link(linkage::coupled,o.link_joints);set_link(linkage::current,o.link_use);
    set_link(linkage::mark_families|linkage::equal_families|linkage::swap_families,o.link_comp);
    bool new_link_labels=(h.millwork.mode&linkage::mark_families)&&!old_link_labels;
    h.appraising=h.appraising || o.effort_mode!=~0u || o.effort_period!=~0u || o.effort_trails!=~0u || o.effort_comp!=~0u ||
        o.effort_paving!=~0u || o.effort_spending!=~0u || o.effort_sample_rule!=~0u || o.effort_sculpt!=~0u || o.effort_window!=~0u || o.effort_span!=~0u || o.effort_audit!=~0u || o.effort_wash || o.effort_survey || o.effort_seasons || o.effort_test ||
        o.effort_calls || !o.effort_germs.empty() || !o.effort_scholar.empty() || (soil_source && soil_source->appraising) || (visitor && visitor->values_effort);
    auto &economy=h.economics;
    if(soil_source && soil_source->appraising && !restored)economy.period=soil_source->economics.period;
    bool priced_cohort=economy.mode&effort::comp;
    auto set_mode=[&](Word bits,Word value){if(value!=~0u)economy.mode=(economy.mode&~bits)|value;};
    set_mode(effort::learning|effort::deciding|effort::reversing|effort::scrambled|effort::blinded_food|effort::blinded_work,o.effort_mode);
    if(o.effort_window!=~0u)set_mode(127u<<16,o.effort_window<<16);
    set_mode(effort::appraisal|effort::foreign_surface,o.effort_paving);
    set_mode(effort::selective,o.effort_spending);set_mode(effort::fixed_survey,o.effort_sample_rule);
    set_mode(effort::tamping,o.effort_sculpt);
    set_mode(effort::trails,o.effort_trails);set_mode(effort::surveying,o.effort_audit);set_mode(effort::more_places,o.effort_span);
    set_mode(effort::comp|effort::same_comp|effort::reverse_comp,o.effort_comp);
    if(o.effort_period!=~0u)economy.period=o.effort_period;
    bool cost_labels=(economy.mode&effort::comp) && !priced_cohort;
    unsigned competitions=bool(h.learning.mode&practice::families)+bool(economy.mode&effort::comp)+bool((o.hollow_contest!=~0u?o.hollow_contest:h.recess.mode)&hollow::competition)+bool(h.millwork.mode&linkage::mark_families);
    competitions+=bool(h.rush.mode&runnel::families);
    if(competitions>1)throw std::invalid_argument("spatial, effort and linkage contests share a single inherited label");
    h.ranging=h.ranging || h.wicking || h.linking || h.appraising || o.hollow_mode!=~0u || o.hollow_odometry!=~0u || o.hollow_anchor!=~0u || o.hollow_rich!=~0u || o.hollow_current!=~0u || o.hollow_contest!=~0u || o.hollow_slots ||
        o.hollow_test || o.hollow_seasons || o.hollow_calls || o.hollow_survey || !o.hollow_scholar.empty() || !o.hollow_origin.empty() ||
        (visitor && visitor->ranges) || (soil_source && soil_source->ranging);
    if(soil_source && soil_source->ranging) h.recess.mode=soil_source->recess.mode;
    Word new_arena=o.hollow_slots;
    if(visitor && visitor->ranges && !new_arena)
        new_arena=h.wanderers.empty()?visitor->path_memory.slot_limit:std::max(h.recess.slots,visitor->path_memory.slot_limit);
    if(new_arena) {
        Word used=visitor&&visitor->ranges?visitor->path_memory.mind.used:0;
        for(auto &m:h.wanderers) used=std::max(used,Word(m.used));
        if(new_arena<used) throw std::invalid_argument("the narrower arena would discard occupied spatial cells");
        h.recess.slots=new_arena;
        if(!h.wanderers.empty()) {h.bearings.resize(Count(h.sites())*new_arena);h.sightings.resize(Count((h.sites()+15)/16)*new_arena);}
    }
    if(o.hollow_mode!=~0u) h.recess.mode=(h.recess.mode&~7u)|o.hollow_mode;
    if(o.hollow_odometry!=~0u) h.recess.mode=(h.recess.mode&~Word(hollow::dead_reckoning|hollow::distorted))|o.hollow_odometry;
    if(o.hollow_anchor!=~0u) h.recess.mode=(h.recess.mode&~Word(hollow::anchoring))|o.hollow_anchor;
    bool old_contest=h.recess.mode&hollow::competition;
    if(o.hollow_contest!=~0u) h.recess.mode=(h.recess.mode&~hollow::treatment_bits)|o.hollow_contest;
    bool mark_founders=(h.recess.mode&hollow::competition) && (!old_contest || o.hollow_seasons || !o.hollow_origin.empty());
    if(o.hollow_current!=~0u)h.recess.mode=(h.recess.mode&~Word(hollow::drift))|o.hollow_current;
    if(o.hollow_rich!=~0u) h.recess.mode=(h.recess.mode&~Word(hollow::rich_beds))|o.hollow_rich;
    h.overhearing = h.overhearing || o.chorus_hold != ~0u || o.chorus != ~0u || (o.reed_bands && !h.calling) ||
                    (soil_source && soil_source->overhearing);
    if (soil_source && soil_source->overhearing) {
        h.audience.access = soil_source->audience.access;
        h.audience.coupling = soil_source->audience.coupling;
    }
    if (o.chorus_hold != ~0u)
        h.audience.access = (h.audience.access & 1) | (o.chorus_hold << 1);
    if (o.chorus != ~0u)
        h.audience.coupling = o.chorus;
    h.pottery = h.pottery || o.press_use != ~0u || o.press_strata != ~0u || o.press_feel != ~0u || o.press_test || o.press_seasons || o.press_bench || o.press_survey || !o.press_scholar.empty() || o.clay_reach != ~0u || o.ceramic != ~0u || o.clay_hands != ~0u || o.clay_hearing != ~0u ||
                o.clay_muscle != ~0u || o.clay_context != ~0u || o.clay_seasons || o.clay_test || o.clay_dig ||
                o.clay_foster || o.wash_ware || o.clay_switches || o.clay_calls || !o.clay_settlers.empty() ||
                !o.clay_peers.empty() || !o.clay_specimen.empty() || !o.clay_export.empty() || !o.clay_origin.empty() ||
                (soil_source && soil_source->pottery) || (visitor && visitor->potter);
    if (soil_source && soil_source->pottery) {
        h.workshop.use = soil_source->workshop.use;
        h.workshop.material = soil_source->workshop.material;
    }
    if (o.press_use != ~0u)
        h.workshop.use = (h.workshop.use & (127u | press::bedded | press::tidal | press::feel_set | press::feeling | press::dislocated | press::stiff)) | o.press_use;
    if (o.press_feel != ~0u)
        h.workshop.use=(h.workshop.use & ~(press::feel_set|press::feeling|press::dislocated|press::stiff)) | press::installed | o.press_feel;
    if (o.press_strata != ~0u)
        h.workshop.use = (h.workshop.use & ~(press::bedded|press::tidal)) | press::installed | o.press_strata;
    if (o.ceramic != ~0u)
        h.workshop.material = (h.workshop.material & ~3u) | o.ceramic;
    if (o.clay_reach != ~0u)
        h.workshop.material = (h.workshop.material & 3u) | (o.clay_reach << 2);
    if (o.clay_hands != ~0u)
        h.workshop.use = (h.workshop.use & ~3u) | o.clay_hands;
    if (o.clay_muscle != ~0u)
        h.workshop.use = (h.workshop.use & ~4u) | (o.clay_muscle << 2);
    if (o.clay_hearing != ~0u)
        h.workshop.use = (h.workshop.use & ~96u) | o.clay_hearing;
    if (o.clay_context != ~0u)
        h.workshop.use = (h.workshop.use & ~24u) | o.clay_context;
    h.calling = h.pottery || o.wash_echo || o.reed_census || o.reed_bench || o.reed_legacy ||
                !o.phonetic_germ.empty() || o.reed_work != ~0u || h.overhearing || o.reed_foster_rounds || h.calling ||
                o.reed_bands || o.reed_view != ~0u || o.reed_medium != ~0u || o.reed_growth != ~0u ||
                o.reed_clutch != ~0u || o.reed_period || o.reed_wire != ~0u || o.reed_windows != ~0u || o.reed_trials ||
                o.reed_seasons || (soil_source && soil_source->calling) || (visitor && visitor->calling);
    if (soil_source && soil_source->calling)
        h.reeds = soil_source->reeds;
    if (o.reed_bands)
        h.reeds.bands = o.reed_bands;
    if (o.reed_wire != ~0u)
        h.reeds.wire = o.reed_wire;
    if (o.reed_windows != ~0u)
        h.reeds.windows = (h.reeds.windows & ~3u) | o.reed_windows;
    if (o.reed_view != ~0u)
        h.reeds.windows = (h.reeds.windows & ~12u) | (o.reed_view << 2);
    if (o.reed_medium != ~0u)
        h.reeds.windows = (h.reeds.windows & ~16u) | (o.reed_medium << 4);
    if (o.reed_growth != ~0u)
        h.reeds.windows = (h.reeds.windows & ~32u) | (o.reed_growth << 5);
    if (o.reed_clutch != ~0u)
        h.reeds.windows = (h.reeds.windows & ~64u) | (o.reed_clutch << 6);
    if (o.reed_period)
        h.reeds.pulse = o.reed_period;
    if (o.reed_work != ~0u)
        h.reeds.windows = (h.reeds.windows & ~128u) | ((1u - o.reed_work) << 7);
    h.founder_arbors = o.brine_arbors;
    Word original_solutes = h.saline ? h.solution.species : 0;
    h.saline = h.saline || (soil_source && soil_source->saline) || o.brine_species || o.brine_flow != ~0u ||
               o.paired_material || o.brine_senses != ~0u || o.brine_plan != ~0u || o.brine_seasons || o.brine_trials ||
               o.brine_calls || (visitor && visitor->saline);
    if (soil_source && soil_source->saline) {
        h.solution.species = soil_source->solution.species;
        h.solution.flowing = soil_source->solution.flowing;
    }
    if (o.brine_species) {
        if ((!h.salts.empty() || (soil_source && soil_source->saline)) && o.brine_species < h.solution.species)
            throw std::invalid_argument("removing an existing solute would discard material");
        h.solution.species = o.brine_species;
    }
    if (o.brine_flow != ~0u)
        h.solution.flowing = (h.solution.flowing & ~1u) | o.brine_flow;
    if (o.brine_senses != ~0u)
        h.solution.flowing = (h.solution.flowing & ~2u) | ((1u - o.brine_senses) << 1);
    if (o.brine_plan != ~0u)
        h.solution.flowing = (h.solution.flowing & ~4u) | ((1u - o.brine_plan) << 2);
    h.sedimentary = h.saline || h.sedimentary || o.substrate != ~0u || o.ore_guidance != ~0u;
    if (o.substrate != ~0u)
        h.seam.pattern = o.substrate;
    if (o.ore_guidance != ~0u)
        h.seam.guidance = o.ore_guidance;
    h.investigatory = h.sedimentary || h.investigatory || o.worksite_mode != ~0u || o.site_marks != ~0u ||
                      o.field_trials || o.worksite_calls || o.inquiry_replay || o.wash_sites ||
                      (visitor && visitor->investigates);
    if (o.worksite_mode != ~0u)
        h.worksite.active = o.worksite_mode;
    if (o.site_marks != ~0u)
        h.worksite.marks = o.site_marks;
    bool needs_materials = h.calling || h.investigatory || o.shoal_calls || o.tide_period || o.tide_active != ~0u ||
                           o.tide_sight != ~0u || o.climate != ~0u || o.shoal_trials || o.shoal_seasons ||
                           o.artifact_samples || o.hydraulic_trial || o.mineral_water != ~0u || o.guild_generations ||
                           o.hand_calls || !o.hand_germ.empty() || o.quarry_seasons || o.quarry_moves ||
                           o.work_seasons || o.work_probes || o.minerals < 2 || o.material_density != ~0u ||
                           o.material_legacy || o.material_bonds != ~0u || o.material_agency != ~0u ||
                           o.material_vision != ~0u || (visitor && visitor->handling);
    if (o.minerals == 2 && (prior_minerals || needs_materials))
        throw std::invalid_argument("material-free arenas cannot hold existing grains or a manipulation circuit");
    h.articulated = h.wicking || h.linking || prior_minerals || needs_materials || (!restored && !visitor && o.minerals != 2);
    h.foresight = h.investigatory || h.foresight || o.shoal_calls || o.tide_period || o.tide_active != ~0u ||
                  o.tide_sight != ~0u || o.climate != ~0u || (visitor && visitor->projects);
    if (h.foresight && h.header.configuration.side < 8)
        throw std::invalid_argument("a shore patch needs eight sites across");
    if (o.resident_test && !h.foresight)
        throw std::invalid_argument("resident replay requires a habitat with predictive tissue");
    if (o.tide_period)
        h.tide.period = o.tide_period;
    if (o.tide_active != ~0u)
        h.tide.active = (h.tide.active & ~1u) | o.tide_active;
    if (o.tide_sight != ~0u)
        h.tide.active = (h.tide.active & ~2u) | (o.tide_sight << 1);
    if (o.climate != ~0u)
        h.tide.active = (h.tide.active & 3u) | (o.climate << 2);
    if (prior_minerals && o.material_density != ~0u)
        throw std::invalid_argument("a saved world already contains its geological deposits");
    if (o.material_density != ~0u)
        h.minerals.physics.density = o.material_density;
    if (o.minerals < 2)
        h.minerals.physics.bonded = (h.minerals.physics.bonded & 2) | o.minerals;
    if (o.material_bonds != ~0u)
        h.minerals.physics.bonded = (h.minerals.physics.bonded & 2) | o.material_bonds;
    if (o.mineral_water != ~0u)
        h.minerals.physics.bonded = (h.minerals.physics.bonded & 1) | (o.mineral_water << 1);
    if (o.material_agency != ~0u)
        h.minerals.physics.agency = o.material_agency;
    if (o.material_vision != ~0u)
        h.minerals.physics.vision = o.material_vision;
    bool prior_neuro = h.developing;
    if (visitor && visitor->grows_cells)
        h.developing = true;
    if ((!restored && !visitor && o.neurogenesis == ~0u) || o.neurogenesis == 1)
        h.developing = true;
    if (o.neurogenesis != ~0u)
        h.neuro.open = o.neurogenesis;
    if (o.fixed_size != ~0u || o.silent_new != ~0u) {
        if (!h.developing)
            throw std::invalid_argument("a fixed-size cohort requires a developing habitat");
        if (o.fixed_size != ~0u)
            h.neuro.cohort_controls = (h.neuro.cohort_controls & ~3u) | o.fixed_size;
        if (o.silent_new != ~0u)
            h.neuro.cohort_controls = (h.neuro.cohort_controls & 3u) | (o.silent_new << 2);
    }
    if (restored && o.explicit_cells) {
        Word previous = h.header.configuration.tiles;
        h.expand_brains(o.c.tiles);
        if (previous < o.c.tiles)
            std::cout << "neural_ceiling=" << h.header.tick << "  prior_cells=" << 32 * previous
                      << "  available_cells=" << 32 * o.c.tiles << '\n';
    }
    if (o.pod_mode != ~0u || o.motor_calls || o.assay_pods || o.remap_training || o.raft_seasons ||
        o.pod_origin != ~0u || !o.pod_germ.empty() || (visitor && visitor->delayed_pods))
        h.enclosed = true;
    auto &physics = h.crop.physics;
    if (o.pod_mode != ~0u)
        physics.background = (physics.background & ~1u) | o.pod_mode;
    if (o.single_cohort != ~0u)
        physics.background = (physics.background & ~96u) | (o.single_cohort << 5);
    if (o.pod_pulse != ~0u)
        physics.background = (physics.background & ~16u) | (o.pod_pulse << 4);
    if (o.pod_spacing != ~0u)
        physics.background = (physics.background & ~2u) | (o.pod_spacing << 1);
    if (o.pod_frames != ~0u)
        physics.background = (physics.background & ~4u) | (o.pod_frames << 2);
    if (o.pod_regimes != ~0u)
        physics.background = (physics.background & ~8u) | (o.pod_regimes << 3);
    if (o.pod_lag)
        physics.lag = o.pod_lag;
    if (o.pod_period)
        physics.period = o.pod_period;
    if (o.pod_credit != ~0u)
        physics.credit = o.pod_credit;
    if (o.pod_clock != ~0u)
        physics.variable = o.pod_clock;
    if (o.cut_cohort != ~0u)
        physics.cut_cohort = o.cut_cohort;
    if (!h.enclosed && (o.erase_pods || o.single_cohort != ~0u || o.pod_pulse != ~0u || o.pod_frames != ~0u ||
                        o.pod_regimes != ~0u || o.pod_spacing != ~0u || o.pod_lag || o.pod_period ||
                        o.pod_credit != ~0u || o.pod_clock != ~0u || o.cut_cohort != ~0u))
        throw std::invalid_argument("pod intervention needs a pod habitat");
    bool established = !h.pod_animals.empty();
    if (established && (o.pod_origin != ~0u || !o.pod_germ.empty()))
        throw std::invalid_argument("existing pods already contain the residents' inherited laws");
    if (o.pod_origin != ~0u)
        h.pod_origin = o.pod_origin;
    if (h.enclosed && (!o.pod_germ.empty() || (!o.founder.empty() && !o.sensory && o.pod_origin == ~0u))) {
        storage::AgentImage donor;
        donor.read(o.pod_germ.empty() ? o.founder : o.pod_germ);
        if (donor.delayed_pods) {
            h.pod_origin = 3;
            h.pod_donor = {donor.pod_organ.genes, donor.pod_organ.frame_genes};
        } else if (!o.pod_germ.empty())
            throw std::invalid_argument("this donor has no inherited crop circuit");
    }
    if (o.steps > ~0u - h.header.tick)
        throw std::invalid_argument("floodplain clock would overflow");
    auto &c = h.header.configuration;
    if (o.requested_frontier)
        c.frontier = o.c.frontier;
    if (c.frontier && c.frontier < c.side)
        throw std::invalid_argument("frontier must include the current habitat");
    if (o.extend && (!restored || o.extend <= c.side || (c.frontier && o.extend > c.frontier)))
        throw std::invalid_argument("extension requires a smaller saved habitat and space inside its frontier");
    if (o.survey_origin && o.survey_origin >= std::max(c.side, o.extend))
        throw std::invalid_argument("the survey origin must fit inside the habitat");
    if (!o.field_questions.empty() && o.follow_witness.empty())
        throw std::invalid_argument("--field-questions requires an individual selected with --follow-witness");
    if (o.field_class && o.class_retests)
        throw std::invalid_argument("choose a population class or an individual retest");
    if (o.class_retests && o.slot == ~0u)
        throw std::invalid_argument("a repeated interview needs a resident slot");
    if (!o.class_trace.empty() && !o.field_class && !o.class_retests)
        throw std::invalid_argument("a class trace needs an enrollment");
    if ((!o.field_lessons.empty() || !o.follow_witness.empty()) && !restored)
        throw std::invalid_argument("a field lesson or witness visit needs a saved watershed");
    std::unique_ptr<storage::AgentImage> followed;
    if (!o.follow_witness.empty()) {
        if ((o.export_mind.empty() && o.field_questions.empty() && o.shore_school.empty() && o.drift_diary.empty()) ||
            o.slot != ~0u || o.observer)
            throw std::invalid_argument("--follow-witness selects the resident for --export-mind");
        followed = std::make_unique<storage::AgentImage>();
        followed->read(o.follow_witness);
        identify_voice(h, *followed);
    }
    for (const auto &lesson : o.field_lessons)
        return_field_lesson(h,lesson.original,lesson.taught,lesson.hand);
    Session s;
    s.open();
    if(!o.receive_donor.empty()){
        if(!restored || !h.braiding || h.riggers.size()!=h.sites())
            throw std::invalid_argument("a donor needs a restored population of living float operators");
        place_donor(s,h,o,read_donor(o.receive_donor));
    } else if (o.receive_count != 1 && o.retina_donor.empty() && o.audience_inherit.empty() && o.voice_inherit.empty())
        throw std::invalid_argument("carrier introduction needs a donor");
    if (o.rotating_samples && o.probe_trace.empty())
        throw std::invalid_argument("moving sampling sites need a tracer evidence file");
    if (o.deliberation && o.retina_questions.empty())
        throw std::invalid_argument("a deliberating pupil needs a spoken question");
    if ((o.inquiry_hand != retina::directed || o.reversed_question) && !o.deliberation)
        throw std::invalid_argument("a displaced inquiry hand needs a deliberation interval");
    if(!o.voice_inherit.empty()) {
        if(!restored || !h.braiding || (o.slot!=~0u && (o.receive_count!=1 || o.slot>=h.sites() || !h.bodies[o.slot].energy)))
            throw std::invalid_argument("introduce the resonators into living watershed founders");
        auto g=utterance::read_voice(o.voice_inherit);
        Word n=h.sites(),start=mix(o.class_seed+131),stride=mix(o.class_seed+5813)|1,placed=0;
        for(Word i=0;i<n && placed<o.receive_count;++i) {
            Word id=o.slot==~0u?(start+i*stride)&(n-1):o.slot;
            if(!h.bodies[id].energy)continue;
            h.genomes[id].voice=g.dendrites;h.genomes[id].hearing.expression=g.motor;++placed;
        }
        std::cout<<"resonator_founders="<<placed<<"  tick="<<h.header.tick<<"  dendrites="<<g.dendrites<<"  motor="<<g.motor<<'\n';
    }
    if (!o.audience_inherit.empty()) {
        if (!restored || !h.braiding ||
            (o.slot != ~0u && (o.receive_count != 1 || o.slot >= h.sites() || !h.bodies[o.slot].energy)))
            throw std::invalid_argument("introduce the audience loci into living watershed founders");
        auto p = audience::read_germ(o.audience_inherit);
        Word n = h.sites(), start = mix(o.class_seed + 181), stride = mix(o.class_seed + 7417) | 1, placed = 0;
        for (Word i = 0; i < n && placed < o.receive_count; ++i) {
            Word id = o.slot == ~0u ? (start + i * stride) & (n - 1) : o.slot;
            if (!h.bodies[id].energy)
                continue;
            h.genomes[id].hearing.low = p.first;
            h.genomes[id].hearing.high = p.second;
            ++placed;
        }
        std::cout << "audience_founders=" << placed << "  tick=" << h.header.tick << "  write_gene=" << p.first
                  << "  motor_gene=" << p.second << '\n';
    }
    if (!o.retina_donor.empty()) {
        if (!restored || !h.braiding)
            throw std::invalid_argument("a retinal graft needs a restored population");
        std::ifstream dna(o.retina_donor);
        std::string tag, extra;
        Word first, second, ancestor, seed, generations;
        if (!(dna >> tag >> first >> second >> ancestor >> seed >> generations) ||
            (tag != "AXION-RETINA-1" && tag != "AXION-IRIS-1") || dna >> extra)
            throw std::invalid_argument("invalid observer germline");
        Word n = h.sites(), start = mix(o.class_seed + 181), stride = mix(o.class_seed + 7417) | 1, placed = 0;
        if (o.slot != ~0u && (o.receive_count != 1 || o.slot >= n || !h.bodies[o.slot].energy))
            throw std::invalid_argument("a named observer donor needs a living single recipient");
        for (Word j = 0; j < n && placed < o.receive_count; ++j) {
            Word id = o.slot == ~0u ? (start + j * stride) & (n - 1) : o.slot;
            if (!h.bodies[id].energy)
                continue;
            Word locus = tag == "AXION-IRIS-1" ? 2 : 0;
            h.genomes[id].pigments[locus] = first;
            h.genomes[id].pigments[locus + 1] = second;
            ++placed;
        }
        std::cout << (tag == "AXION-IRIS-1" ? "iris_release=" : "retinal_release=") << h.header.tick
                  << "  placed=" << placed << (tag == "AXION-IRIS-1" ? "  dendrite=" : "  receptors=") << first
                  << (tag == "AXION-IRIS-1" ? "  iris_motor=" : "  plasticity=") << second << "  ancestor=" << ancestor
                  << "  seed=" << seed << "  generations=" << generations << '\n';
    }
    std::optional<apprentice::Donor> tracer;
    if(!o.trace_donor.empty()){
        if(!h.braiding)throw std::invalid_argument("a donor survey needs float operators");
        tracer=read_donor(o.trace_donor);
    }
    Word release_tick=h.header.tick;
    FrontierSurvey frontier{o.survey_origin ? o.survey_origin : c.side, h.header.tick};
    if (o.gate_trial && (!restored || !h.developing))
        throw std::invalid_argument("a gate trial requires a saved developing population");
    if (!o.gate_trial && (!o.gate_trace.empty() || o.gate_phase || o.gate_seed != 17333))
        throw std::invalid_argument("gate observations require --gate-trial crossover or sham");
    std::unique_ptr<EvidenceFile> gate_trace;
    if (!o.gate_trace.empty() && (o.gate_trace == o.save || o.gate_trace == o.export_mind))
        throw std::invalid_argument("the gate trace and checkpoint need distinct output paths");
    if (!o.gate_trace.empty())
        gate_trace = std::make_unique<EvidenceFile>(o.gate_trace);
    bool dormant = restored && !c.flora && std::ranges::all_of(h.land, [](const Silt &p) { return !p.root; });
    intervene(c, o);
    if (int(bool(o.work_seasons)) + int(bool(o.quarry_seasons)) + int(bool(o.shoal_seasons)) > 1)
        throw std::invalid_argument("choose one rearing habitat per release");
    if ((o.raft_seasons || o.work_seasons || o.quarry_seasons || o.shoal_seasons) && (restored || visitor))
        throw std::invalid_argument(
            "a raft release needs a new habitat; --colonists accepts a saved source population");
    if (restored && !o.founder.empty())
        throw std::invalid_argument("a resumed habitat already contains its founders");
    if ((!o.competitors.empty() && o.colonists.empty()) || ((o.cross || o.swap) && o.competitors.empty()) ||
        (!o.colonists.empty() && (restored || !o.founder.empty())))
        throw std::invalid_argument("competition needs compatible founder and competitor pools");
    std::unique_ptr<ShoreSchool> school;
    if(!o.shore_school.empty()){
        if(!restored || !h.recording_floats)throw std::invalid_argument("shore teaching needs a restored watershed with recorded retinal scenes");
        school=std::make_unique<ShoreSchool>(s,h,o,followed?followed->resident.slot:o.slot);
    } else if(!o.shore_trace.empty())throw std::invalid_argument("a shore attendance trace needs a school");
    std::unique_ptr<DriftClock> drift_timer;
    if (o.drift_profile) {
        if (!(h.drift_water.law.ports & drift::moving))
            throw std::invalid_argument("a current timing study needs moving food");
        drift_timer = std::make_unique<DriftClock>(s, o.drift_profile);
    }
    std::unique_ptr<DriftDiary> current_diary;
    if (!o.drift_diary.empty())
        current_diary = std::make_unique<DriftDiary>(s, h, o, followed ? followed->resident.slot : o.slot);
    std::unique_ptr<DriftSighting> current_witness;
    if (o.drift_watch) {
        Word hearing = 0;
        if (o.drift_pupil) {
            auto laws = calibrate_names(s, o.class_seed);
            for (Word i = 0; i < 16; ++i)
                hearing |= Word(bool(laws[i])) << i;
        }
        current_witness = std::make_unique<DriftSighting>(s, h, o, tracer, hearing);
    }
    std::unique_ptr<DriftSurfaces> surface_study;
    std::unique_ptr<InletCensus> inlet_census;
    if (o.drift_first)
        inlet_census = std::make_unique<InletCensus>(s, o, h.header.configuration.seed);
    if (o.drift_surfaces)
        surface_study = std::make_unique<DriftSurfaces>(s, h, o);
    std::unique_ptr<DriftWatch> drift_observer;
    if (!o.drift_trace.empty())
        drift_observer = std::make_unique<DriftWatch>(s, h, o);
    std::optional<bool> sample_motion;
    if (!o.probe_trace.empty())
        sample_motion = bool(o.rotating_samples);
    if(o.ink_follow!=~0u && o.ink_follow>=h.sites())throw std::invalid_argument("the tracked ink slot lies outside the initial watershed");
    if((!o.ink_events.empty() || !o.ink_trace.empty()) && !(h.loom.mode&utterance::ink_port))
        throw std::invalid_argument("stain the active watershed to observe a dye trace");
    auto basin = std::make_unique<Basin>(s, h, restored, o.founder, 0, o.sensory, o.neural_dispatch, o.neural_memory,
                                         o.neural_cache, o.neural_collection, school.get(), drift_observer.get(),
                                         tracer ? &*tracer : nullptr, surface_study.get(), current_witness.get(),
                                         o.drift_account, current_diary.get(), drift_timer.get(), inlet_census.get(),
                                         sample_motion,&o);
    if (auto *j = journal::current()) {
        j->device = s.name;
        j->source = restored ? o.load : o.founder;
        j->habitat = "floodplain";
    }
    if (dormant && c.flora && o.soil.empty())
        basin->rootstock();
    if (!o.colonists.empty())
        basin->hatch_pools(o);
    if (visitor)
        basin->house(*visitor, h);
    basin->rafts(h, o);
    basin->mill_rearing(h, o);
    basin->raise_brine(h, o);
    basin->introduce_calls(h, o);
    basin->rear_siphons(h, o);
    basin->clay_germs(h, o);
    basin->press_rear(h,o);
    basin->hollow_rear(h,o);
    basin->effort_rear(h,o);
    basin->link_rear(h,o);
    basin->braid_rear(h,o);
    basin->rattle_rear(h.backwater.mode,o);
    basin->reed_rear(h,o);if(reed_labels)basin->hollow_color(o.reed_seed);
    basin->practice_rear(h,o);if(practice_labels)basin->hollow_color(o.practice_seed);
    if(new_link_labels)basin->hollow_color(o.link_seed);
    if(cost_labels)basin->hollow_color(o.effort_seed);
    if (o.daylight_profile)
        basin->profile_daylight(o.daylight_profile);
    if (mark_drift || !o.drift_stock.empty())
        basin->drift_founders(h, o, mark_drift);
    if (!o.drift_pool.empty())
        basin->drift_inherit(h, o);
    if(mark_founders) basin->hollow_color(o.hollow_seed);
    basin->clay_classroom(h, o, true);
    basin->clay_settle(h, o);
    auto state = basin->check(h);
    auto reset_stocks = [&] {
        h.minerals.pieces = state.grains;
        h.minerals.charge = state.excitation;
        std::copy_n(state.terminals, 3, h.minerals.terminals);
    };
    if (h.articulated && !prior_minerals)
        reset_stocks();
    if (h.developing && !prior_neuro)
        h.neuro.initial = state.neurons;
    if (!restored) {
        h.header.initial = {state.water, state.clay, state.energy};
        h.header.founders = state.alive;
        h.crop.entry_mass = Word(state.digesting);
    }
    std::vector<utterance::Sample> prior_observations;
    auto extend = [&](Word side) {
        auto prior = state;
        Word old = c.side;
        basin->report_regulation(h);
        basin->report_drift(h);
        basin->daylight_report();
        basin->collect_ink(prior_observations);
        basin->transfer(h, false);
        basin.reset();
        c.side = side;
        basin = std::make_unique<Basin>(
            s, h, true, "", old, false, o.neural_dispatch, o.neural_memory, o.neural_cache, o.neural_collection,
            school.get(), drift_observer.get(), tracer ? &*tracer : nullptr, surface_study.get(), current_witness.get(),
            o.drift_account, current_diary.get(), drift_timer.get(), inlet_census.get(), sample_motion,&o);
        if (o.daylight_profile)
            basin->profile_daylight(o.daylight_profile);
        state = basin->check(h);
        if (state.alive != prior.alive || state.water < prior.water || state.clay < prior.clay ||
            state.energy < prior.energy)
            throw std::logic_error("frontier lost residents or matter");
        auto &base = h.header.initial;
        if (h.articulated) {
            h.minerals.pieces += state.grains - prior.grains;
            h.minerals.charge += state.excitation - prior.excitation;
            for (Word j = 0; j < 3; ++j)
                h.minerals.terminals[j] += state.terminals[j] - prior.terminals[j];
        }
        base.water += state.water - prior.water;
        base.clay += state.clay - prior.clay;
        base.energy += state.energy - prior.energy;
        std::cout << "flood_frontier=" << h.header.tick << "  from=" << old << "  to=" << side
                  << "  retained=" << state.alive << "  water_added=" << state.water - prior.water
                  << "  clay_added=" << state.clay - prior.clay << "  energy_added=" << state.energy - prior.energy
                  << '\n';
    };
    if (soil_source) {
        const auto &donor = *soil_source;
        basin->replace_land(donor.land);
        basin->replace_rinds(donor);
        basin->replace_minerals(donor);
        basin->rebed(donor);
        if(h.linking)basin->rehouse_works(donor,h);
        if(h.practising)basin->replace_strands(donor,h);
        if(h.reeding)basin->replace_reeds(donor);
        if(h.braiding)basin->replace_floats(h,donor);
        if(h.wicking)basin->replace_wicks(donor,h);
        if(h.appraising){
            h.economics.ledger.pressed=donor.appraising?donor.economics.ledger.pressed:0;
            h.economics.ledger.washed=donor.appraising?donor.economics.ledger.washed:0;
            basin->restore_costs(h.economics.ledger);
        }
        if(h.ranging) {
            h.recess.book={}; basin->reset_hollows(h.recess.book); h.recess.opening=0;
            if(donor.ranging) for(auto b:donor.hollows) h.recess.opening+=b.carbon;
        }
        if (h.saline) {
            if (soil_source->saline)
                std::copy_n(soil_source->solution.stock, 5, h.solution.stock);
            else
                std::fill_n(h.solution.stock, 5, Count(0));
        }
        if (h.overhearing) {
            h.audience.total = {};
            basin->reset_audience(h.audience.total);
        }
        if (h.pottery) {
            h.workshop.book = {};
            basin->reset_clay(h.workshop.book);
        }
        if (h.calling) {
            h.reeds.total = {};
            basin->reset_siphons(h.reeds.total);
        }
        h.worksite.totals = {};
        basin->reset_worksites(h.worksite.totals);
        h.seam.totals = {};
        basin->reset_sediment(h.seam.totals);
        if (c.flora && !donor.header.configuration.flora &&
            std::ranges::all_of(donor.land, [](const Silt &p) { return !p.root; }))
            basin->rootstock();
        h.header.account = {};
        h.minerals.account = {};
        basin->reset_material(h.minerals.account);
        h.neuro.account = {};
        basin->reset_growth(h.neuro.account);
        basin->reset_account(h.header.account);
        h.crop.account = {};
        basin->reset_pods(h.crop.account);
        state = basin->check(h);
        h.seam.opening = state.macromolecules;
        h.header.initial = {state.water, state.clay, state.energy};
        h.header.founders = state.alive;
        h.crop.entry_mass = Word(state.digesting);
        h.neuro.initial = state.neurons;
        reset_stocks();
        std::cout << "flood_transplant=" << h.header.tick << "  living=" << state.alive
                  << "  new_water=" << state.water << "  new_clay=" << state.clay << '\n';
    }
    Word inherited_solutes = soil_source ? (soil_source->saline ? soil_source->solution.species : 0) : original_solutes;
    if ((inherited_solutes || (soil_source && o.brine_species)) && inherited_solutes < h.solution.species)
        basin->springs(h, inherited_solutes);
    if(o.link_strip)basin->disturb_works(h);
    if(o.practice_strip)basin->unthread();
    if(o.reed_cut!=~0u)basin->cut_reeds(o);
    if(o.effort_wash)basin->wash_treads(h);
    if (o.wash_ware)
        basin->wash_clay();
    if (o.wash_echo)
        basin->wash_sounds();
    if (o.wash_sites)
        basin->wash_sites();
    if (o.forget)
        basin->forget();
    if (o.erase_pods)
        basin->clear_pods();
    if (o.remobilize) {
        auto before = state;
        basin->flatten();
        state = basin->check(h);
        if (state.clay != before.clay || state.water != before.water || state.energy != before.energy)
            throw std::logic_error("bank remobilization changed physical stocks");
        std::cout << "flood_remobilized=" << before.banked << "  faces=" << before.walls << '\n';
    }
    if (o.extend)
        extend(o.extend);
    if (o.rest_hands)
        basin->rest_hands();
    std::optional<GateTrial> gate_experiment;
    if (o.gate_trial)
        gate_experiment = basin->gate_trial(o);
    std::cout << s.name << "  flood_side=" << c.side << "  capacity=" << c.tiles * 32 << "  genome_bytes="
              << (sizeof(Germ) + (h.practising?sizeof(practice::Genes):0) + (h.linking?sizeof(linkage::Genes):0) + (h.appraising?sizeof(effort::Genes):0) + (h.ranging?sizeof(hollow::Genes):0) + (h.pottery ? sizeof(clay::Law) : 0) + (h.calling ? sizeof(siphon::Genes) : 0) +
                  (h.saline ? sizeof(brine::Genes) : 0) + (h.investigatory ? 2 * sizeof(Word) : 0) +
                  (h.articulated ? sizeof(material::Genes) : 0) + (h.enclosed ? sizeof(pods::Inheritance) : 0) +
                  (h.developing ? sizeof(GrowthGenes) : 0))
              << "  resident_bytes_per_site="
              << (h.pottery ? sizeof(clay::Mind) + 2 * sizeof(clay::Tile) : 0) +
                     (h.overhearing ? sizeof(siphon::Echo) : 0) + (h.calling ? sizeof(siphon::Animal) + 1 : 0) +
                     (h.saline ? sizeof(brine::Salts) + sizeof(brine::Animal) : 0) +
                     (h.sedimentary ? sizeof(Half) : 0) +
                     (h.investigatory ? sizeof(material::Inquiry) + sizeof(material::Imprint) : 0) +
                     (h.foresight ? sizeof(material::Foresight) + sizeof(Half) : 0) +
                     (h.articulated ? sizeof(material::Worker) + sizeof(material::Grain) + 1 : 0) + sizeof(Silt) +
                     sizeof(Word) + (h.braiding?sizeof(braid::Mind)+2+(h.recording_floats?sizeof(braid::Episode):0)+(h.loom.version>=3?sizeof(braid::GateMemory):0)+(h.loom.version>=10?sizeof(utterance::Trail):0):0) + (h.rattling?sizeof(rattle::Mind):0) + (h.reeding?sizeof(runnel::Mind)+1:0) + (h.wicking?4:0) + (h.practising?sizeof(practice::Mind)+h.learning.arena*sizeof(practice::Context)+sizeof(practice::Mark)/4:0) + (h.linking?sizeof(linkage::Mind)+h.millwork.arena*sizeof(linkage::Tissue)+sizeof(linkage::Provenance)/4:0) + (h.appraising?sizeof(effort::Mind)+1:0) + (h.ranging?sizeof(hollow::Mind)+sizeof(hollow::Bed)+h.recess.slots*4+(h.recess.slots+3)/4:0) + sizeof(Germ) + sizeof(Colony) + sizeof(Ear) + sizeof(Byte) +
                     (h.loom.version>=14?sizeof(capillary::Mind):0)+(h.loom.version>=17?sizeof(capillary::History):0)+(h.loom.version>=20?1:0)+((h.cap_law.mode&capillary::material_hands)?5:0)+c.tiles * sizeof(Tissue) + (h.developing ? sizeof(Cortex) : 0) +
                     (h.enclosed ? sizeof(pods::Animal) + sizeof(pods::Surface) : 0)
              << '\n';
    Word until = o.steps;
    std::unique_ptr<FieldWatch> reader_watch;
    if (o.reader_interval) {
        if (!h.recording_floats)
            throw std::invalid_argument("reader observation needs a habitat with visual episodes");
        Word side = std::max(c.side, c.frontier);
        reader_watch = std::make_unique<FieldWatch>(s, side * side, o.class_seed, o.class_practice);
        basin->calibrate_reader_watch(*reader_watch);
        if (basin->reader_present(*reader_watch))
            until = 0;
    }
    report(h, state, 0);
    basin->report_drift(h);
    if(tracer)basin->count_carriers(*tracer,release_tick);
    basin->sample(h);
    if (until)
        frontier.sample(h, false);
    OpeningMorphs opening;
    describe(h, *basin, !until, opening, true);
    auto start = std::chrono::steady_clock::now();
    double simulation_seconds = 0;
    Count site_ticks = 0;
    for (Word elapsed = 0; elapsed < until;) {
        Word k = std::min(o.interval - elapsed % o.interval, until - elapsed);
        if (current_witness)
            k = 1;
        if (reader_watch)
            k = std::min(k, o.reader_interval - elapsed % o.reader_interval);
        if (c.frontier > c.side)
            k = std::min(k, c.season - h.header.tick % c.season);
        if (o.assay_interval)
            k = std::min(k, o.assay_interval - elapsed % o.assay_interval);
        site_ticks += Count(h.sites()) * k;
        auto begin = std::chrono::steady_clock::now();
        basin->step(k, elapsed + k == until && journal::current());
        simulation_seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
        elapsed += k;
        if (current_witness && current_witness->observed(s))
            until = elapsed;
        state = basin->check(h);
        if (c.frontier > c.side && !(h.header.tick % c.season) && basin->margin())
            extend(c.side * 2);
        if (reader_watch && (!(elapsed % o.reader_interval) || elapsed == until) &&
            basin->reader_present(*reader_watch))
            until = elapsed;
        if (o.assay_interval && !(elapsed % o.assay_interval))
            basin->assay_calls();
        if (!(elapsed % o.interval) || elapsed == until) {
            report(h, state, elapsed);
            basin->report_drift(h);
            if(tracer)basin->count_carriers(*tracer,release_tick);
            basin->sample(h);
            if (elapsed < until)
                frontier.sample(h, false);
            if (gate_experiment && elapsed < until)
                gate_experiment->report(h);
            describe(h, *basin, elapsed == until, opening);
        }
    }
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "flood_seconds=" << seconds << "  site_ticks=" << site_ticks
              << "  Msite_ticks_per_s=" << (seconds > 0 ? site_ticks / (1e6 * seconds) : 0)
              << "  simulation_seconds=" << simulation_seconds << "  simulation_Msite_ticks_per_s="
              << (simulation_seconds > 0 ? site_ticks / (1e6 * simulation_seconds) : 0) << '\n';
    basin->material_census(h, o.material_legacy, o.artifact_samples);
    basin->hydraulic_trial(o.hydraulic_trial);
    basin->mill_exam(h, o);
    basin->quarry_exam(h, o);
    basin->shoal_exam(h, o);
    basin->field_transfer(h, o);
    basin->brine_assay(h, o);
    if (h.calling) {
        const auto &r = h.reeds.total;
        std::cout << "siphon_tick=" << h.header.tick << "  bands=" << h.reeds.bands << "  pairs=" << r.pairs
                  << "  questions=" << r.inquiries << "  replies=" << r.replies << "  strokes=" << r.strokes
                  << "  heard=" << r.transmitted << "  released=" << r.released << "  swallowed=" << r.consumed
                  << "  work=" << r.native_work << "  passive=" << r.seep << "  sightings=" << r.sights
                  << "  synaptic_bits=" << r.field_edits << '\n';
    }
    if (h.overhearing) {
        auto &b = h.audience.total;
        std::cout << "siphon_audience=" << h.header.tick << "  heard=" << b.heard << "  seen_after=" << b.paired
                  << "  altered_bits=" << b.edits << "  young=" << b.young << '\n';
    }
    basin->clay_census(h);
    basin->hollow_scholar(h,o);
    basin->reed_study(h,o);
    basin->rattle_study(h,o);
    if (current_witness)
        current_witness->report(h);
    if (inlet_census)
        inlet_census->report(s, h.header.tick, bool(tracer), h.drift_water.law.ports & drift::storage);
    if (reader_watch && reader_watch->slot != ~0u) {
        const Word id = reader_watch->slot;
        const auto &person = h.bodies[id];
        std::cout << "field_qualification=" << id << "  born=" << person.born << "  generation=" << person.generation
                  << "  cells=" << 32 * h.conducting(id) << "  source_tick=" << h.header.tick
                  << "  event=" << h.float_events[id].stamp << '\n';
    }
    if (reader_watch)
        std::cout << "field_watch=" << (reader_watch->slot == ~0u ? "horizon" : "qualified")
                  << "  tick=" << h.header.tick << "  slot=" << reader_watch->slot
                  << "  samples=" << reader_watch->samples << "  candidate_visits=" << reader_watch->examined
                  << "  interval=" << o.reader_interval << "  seed=" << o.class_seed << '\n';
    if (school)school->finish(s,h,o);
    if (!o.field_questions.empty())
        basin->field_interview(h, *followed, o.field_questions);
    basin->field_class(h, o);
    basin->report_regulation(h);
    basin->survey_regulation(h,o,tracer);
    if (drift_timer)
        drift_timer->report();
    if (current_diary)
        current_diary->finish(s, h.header.tick);
    if (surface_study)
        surface_study->finish(s, h.header.tick, h.drift_water.law.ports);
    if (drift_observer)
        drift_observer->publish(s, h.drift_water.law);
    basin->daylight_report(o.probe_trace);
    basin->ink_report(h,o,prior_observations);
    basin->capillary_report(h,o.cap_events);
    if(!o.context_events.empty())basin->context_events(o.context_events);
    basin->detritus_report(h,o.cargo_events);
    if(!o.catalytic_proof.empty())basin->catalytic_proof(o.catalytic_proof);
    if(!o.cargo_proof.empty())basin->detritus_proof(o.cargo_proof);
    if(!o.cap_film.empty())basin->trace_capillary_wake(o);
    if(!o.catalytic_film.empty())basin->catalytic_closures(o);
    basin->report_hands(h,o.hand_events);
    if(!o.chemical_witness.empty())basin->chemical_receptions(o.chemical_witness);
    if (!o.surface_output.empty())
        basin->record_surfaces(h, o);
    if (o.retina_trials)
        basin->retina_study(h, o);
    if (o.audience_trials)
        basin->audience_study(h, o);
    if(o.story_trials)basin->story_study(h,o);
    if(!o.signal_adult.empty()){if(o.signal_channels==2)basin->compound_individual_study(o);else basin->signal_individual(o);}
    else if(o.cap_trials||o.context_series.streams||o.signal.lessons)basin->capillary_study(h,o);
    if (!o.retina_questions.empty())
        basin->cascade_interview(h, o, followed ? followed->resident.slot : o.slot);
    basin->drift_repertoire(h, o);
    if (o.inlet_trials)
        basin->inlet_study(h, o);
    if (!o.inlet_questions.empty())
        basin->inlet_interview(h, o, followed ? followed->resident.slot : o.slot);
    basin->apprenticeship(h,o);
    basin->rear_regulation(h,o);
    basin->report_dispatch(h);
    basin->report_neural_memory();
    if (o.brain_bench)
        basin->bench_dispatch(o.brain_bench);
    basin->braid_study(h,o);
    basin->wick_study(h,o);
    basin->practice_study(h,o);
    basin->link_study(h,o);
    basin->link_census(h,o);
    basin->effort_study(h,o);
    basin->effort_census(h,o);
    basin->effort_quays(h,o);
    basin->effort_benchmark(h,o);
    basin->hollow_exam(h,o);
    basin->hollow_bench(h,o);
    basin->hollow_census(o,h.header.tick);
    if(h.recess.mode&hollow::competition) {
        for(Word label=0;label<2;++label){
            Count living=0,energy=0,young=0,travel=0;
            for(Word i=0;i<h.sites();++i) {
                const auto &body=h.bodies[i];
                if(!body.energy || bool(h.wanderers[i].efferent&hollow::lineage_tag)!=bool(label))continue;
                ++living;energy+=body.energy;young+=h.header.tick-body.born<=16;travel+=body.motions;
            }
            std::cout<<"hollow_lineage="<<label<<"  tick="<<h.header.tick<<"  active="<<hollow::may_steer(h.recess.mode,label?hollow::lineage_tag:0)
                     <<"  crossed="<<bool((h.recess.mode&hollow::crossed_motor) && !hollow::may_steer(h.recess.mode,label?hollow::lineage_tag:0))
                     <<"  living="<<living<<"  young="<<young<<"  energy="<<energy<<"  travel="<<travel<<'\n';
        }
    }
    if(h.ranging) {
        Count stock=0,structures=0,minds=0,cells=0,marked=0;Word largest=0;
        for(Word i=0;i<h.sites();++i) {
            stock+=h.hollows[i].carbon; structures+=bool(h.hollows[i].relief);
            if(!h.bodies[i].energy) continue;
            auto &m=h.wanderers[i];minds+=m.used!=0;cells+=m.used;largest=std::max(largest,Word(m.used));
            for(Word j=0;j<m.used;++j)marked+=bool(hollow::strength(h.bearings[Count(j)*h.sites()+i]));
        }
        auto &b=h.recess.book;
        if(h.recess.opening+b.settled+b.placed!=stock+b.released)throw std::logic_error("unbalanced hollow carbon");
        std::cout<<"hollow_field="<<h.header.tick<<"  cavities="<<structures<<"  stock="<<stock<<"  captured="<<b.settled<<"  invested="<<b.placed
                 <<"  released="<<b.released<<"  eaten="<<b.eaten<<"  heat="<<b.heat<<"  digs="<<b.opened<<"  edits="<<b.rewrites
                 <<"  corrections="<<b.corrections<<"  recalls="<<b.recalls<<"  steered="<<b.steered<<"  strides="<<b.strides
                 <<"  all_moves="<<h.header.account.moves<<"  ferry="<<bool(h.recess.mode&hollow::drift)<<"  arrivals="<<b.arrivals<<"  repeat_meals="<<b.repeat_food<<"  certified_returns="<<b.same_place<<"  certified_meals="<<b.certified_food
                 <<"  remembering="<<minds<<"  expressed="<<cells<<"  active="<<marked<<"  largest="<<largest<<"  arena="<<h.recess.slots<<'\n';
    }
    basin->press_census(o,h.header.tick);
    basin->press_study(h,o);
    basin->press_benchmark(h,o);
    basin->press_scholar(h,o);
    if (h.workshop.use & press::installed) {
        const auto &p = h.workshop.book.press;
        if (h.workshop.use & press::feel_set)
            std::cout << "press_touch=" << h.header.tick << "  rims=" << p.rims << "  tool_taps=" << p.tips
                      << "  heat=" << p.probe_heat << "  redirected=" << p.redirected
                      << "  fees=" << ((h.workshop.use & press::stiff) ? 3 : 1) << '\n';
        std::cout << "press_field=" << h.header.tick << "  strokes=" << p.strokes << "  alternatives=" << p.alternatives
                  << "  impressions=" << p.printed << "  from_marked=" << p.recast << "  loaded=" << p.wet
                  << "  openings=" << p.open << "  released=" << p.released << "  eaten=" << p.eaten
                  << "  witnessed=" << p.witnessed << "  sensory_bits=" << p.visual_edits
                  << "  taste_bits=" << p.taste_edits << "  studies=" << p.studied << "  ancestral=" << p.predating
                  << "  heat=" << p.heat << '\n';
    }
    basin->clay_classroom(h, o, false);
    if (o.clay_dig)
        basin->clay_classroom(h, o, false, true);
    basin->clay_peers(h, o);
    basin->clay_object(h, o);
    basin->clay_benchmark(h, o.clay_calls);
    basin->examine_siphons(h, o);
    basin->siphon_foster(h, o);
    basin->siphon_census(h, o);
    basin->siphon_legacy(h, o.reed_legacy,o.reed_reference);
    basin->siphon_benchmark(h, o.reed_bench);
    if (h.saline && o.inquiry_replay)
        basin->inquiry_effect(true);
    basin->brine_benchmark(h, o.brine_calls);
    if (h.saline) {
        std::array<Word, 256> mappings{};
        Count cells = 0, changed = 0, living = 0;
        Word different = 0;
        for (Word i = 0; i < h.sites(); ++i) {
            Word phase = brine::phase(h.salts[i], h.land[i].water), map = 0;
            for (Word j = 0; j < 4; ++j)
                map |= brine::surface(j, phase) << (2 * j);
            different += mappings[map]++ == 0;
            if (h.bodies[i].energy) {
                auto &a = h.saline_animals[i];
                cells += std::popcount(a.genes.active);
                changed += std::popcount(a.memory.low | a.memory.high);
                ++living;
            }
        }
        std::cout << "brine_census=" << h.header.tick << "  solutes=" << h.solution.species
                  << "  local_laws=" << different << "  residents=" << living << "  predictive_cells=" << cells
                  << "  modified_cells=" << changed << "  trace_bytes=" << sizeof(brine::Trace)
                  << "  ion0=" << h.solution.stock[0] << "  ion1=" << h.solution.stock[1]
                  << "  ion2=" << h.solution.stock[2] << "  ion3=" << h.solution.stock[3]
                  << "  ion4=" << h.solution.stock[4] << '\n';
        for (Word code = 0; code < mappings.size(); ++code)
            if (mappings[code])
                std::cout << "brine_mapping=" << code << "  sites=" << mappings[code] << '\n';
    }
    if (o.inquiry_replay)
        basin->inquiry_effect();
    basin->worksite_bench(h, o.worksite_calls);
    basin->shoal_bench(h, o.shoal_calls);
    basin->hand_bench(h, o.hand_calls);
    basin->guild(h, o);
    if (h.sedimentary) {
        auto &p = h.seam.totals;
        if (h.seam.opening + p.adsorbed != state.macromolecules + p.released)
            throw std::logic_error("sedimentary carbon imbalance");
        std::cout << "substrate_tick=" << h.header.tick << "  bound=" << state.macromolecules
                  << "  fixed=" << p.adsorbed << "  released=" << p.released << "  presses=" << p.presses
                  << "  fed=" << p.meals << "  distinct=" << p.alternatives << "  guided=" << p.differential << '\n';
    }
    if (h.investigatory) {
        auto &a = h.worksite.totals;
        Word on_floor = 0, in_live = 0, in_vacant = 0, marked = 0;
        Count pairs = 0, substrates = 0;
        for (Word i = 0; i < h.sites(); ++i) {
            on_floor += h.mineral_bed[i].faces != 0;
            marked += h.patina[i].events != 0;
            if (h.bodies[i].energy) {
                in_live += h.workers[i].hand.faces != 0;
                pairs += std::popcount(h.forecasts[i].appetite >> 16);
                substrates += std::popcount(h.enquiries[i].sampled & 65535u);
            } else
                in_vacant += h.workers[i].hand.faces != 0;
        }
        std::cout << "worksite_tick=" << h.header.tick << "  released=" << a.released << "  floor=" << on_floor
                  << "  live_cargo=" << in_live << "  vacant_cargo=" << in_vacant << "  visits=" << a.visits
                  << "  acts=" << a.actions << "  contacts=" << a.contacts << "  assays=" << a.scrapes
                  << "  products=" << a.reactions << "  pumped=" << a.pumped << "  heat=" << a.heat
                  << "  eaten=" << a.eaten << "  inscriptions=" << a.inscriptions << "  marked=" << marked
                  << "  readings=" << a.readings << "  predating_birth=" << a.before_birth << "  known_pairs=" << pairs
                  << "  known_substrates=" << substrates << '\n';
    }
    if (h.foresight) {
        auto &a = h.tide.totals;
        std::cout << "construction_tide=" << h.header.tick << "  period=" << h.tide.period << "  sessions=" << a.looks
                  << "  climate="
                  << ((h.tide.active & 8)   ? "mosaic"
                      : (h.tide.active & 4) ? "veined"
                                            : "diffuse")
                  << "  candidates=" << a.alternatives << "  lifts=" << a.lifts << "  placements=" << a.placements
                  << "  turns=" << a.turns << "  cost=" << a.work << "  spatial_bytes=" << sizeof(material::Foresight)
                  << '\n';
    }
    frontier.sample(h, true);
    if (gate_experiment)
        gate_experiment->report(h, gate_trace.get());
    if (o.growth_assay)
        basin->assay_growth();
    basin->motor_benchmark(h, o.motor_calls);
    basin->pod_remap(h, o);
    basin->legacy(o.legacy);
    basin->pod_assay(h, o.pod_subjects, o.assay_pods, o.pod_test_seed);
    if (o.ear_assay && (!o.assay_interval || o.steps % o.assay_interval || !o.steps))
        basin->assay_calls();
    if (!o.save.empty() || !o.export_mind.empty()) {
        basin->transfer(h, false);
        if (followed && !o.export_mind.empty()) {
            Word slot = followed->resident.slot;
            bool present = same_field_individual(h, *followed);
            std::cout << "field_follow=" << (present ? "present" : "lost") << "  tick=" << h.header.tick
                      << "  slot=" << slot << "  born=" << followed->resident.animal.born
                      << "  event=" << (present && h.recording_floats ? h.float_events[slot].stamp : 0)
                      << "  images=" << (present && h.recording_floats ? h.float_events[slot].images : 0)
                      << "  known=" << (present && h.recording_floats ? h.float_events[slot].known : 0)
                      << "  exported=" << present << '\n';
            if (present)
                export_individual(h, o.export_mind, slot, false);
        } else if (current_witness && !o.export_mind.empty()) {
            if (current_witness->witnessed.id != ~0u)
                export_individual(h, o.export_mind, current_witness->witnessed.id, false);
        } else if (!o.export_mind.empty()) {
            export_individual(h, o.export_mind, o.slot, o.observer);
        }
        if (!o.save.empty())
            storage::write_checked(o.save, h);
        if (h.short_neural_rows)
            std::cout << "neural_file=compact  rows=" << h.sites() << "  explicit_bands=" << h.file_bands
                      << "  developmental_rows=" << h.file_developmental
                      << "  bytes=" << sizeof(Count) + h.sites() + h.file_bands * sizeof(Tissue)
                      << "  dense_bytes=" << Count(h.sites()) * h.header.configuration.tiles * sizeof(Tissue) << '\n';
    }
    if (gate_trace)
        gate_trace->publish();
    return 0;
}
} // namespace axion::life::flood
