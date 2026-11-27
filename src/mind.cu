#include "mind_core.cuh"
#include "mind_hippocampus.cuh"
#include "mind_causal.cuh"
#include "mind_neocortex.cuh"
#include "mind_spatial.cuh"
#include "mind_world.cuh"

using namespace axion;
using namespace axion::mind;
using namespace axion::mind::gpu;

#define KERNEL extern "C" __attribute__((global))

KERNEL void mind_evo_init(CorticalGenome *pop, Word n, Word seed) {
    Word i = global_tid();
    if (i < n)
        pop[i] = founder_genome(mix(seed ^ (i * 0x9e3779b9u + 1337u)));
}

KERNEL void mind_evo_eval(const CorticalGenome *pop, EvoFitness *fit, EvoConfig cfg, Word gen_seed) {
    Word i = global_tid();
    if (i >= cfg.population)
        return;
    evo_eval_creature(pop[i], mix(gen_seed ^ (i * 0x7feb352du)), cfg, fit[i]);
}

KERNEL void mind_evo_breed(const CorticalGenome *parents, CorticalGenome *children, const EvoFitness *fit, Word n,
                           Word seed, Word drift) {
    Word i = global_tid();
    if (i >= n)
        return;
    if (drift) {
        Word p1 = mix(seed + i * 31337u) % n;
        Word p2 = mix(seed + i * 7919u + 17u) % n;
        children[i] = mutate_genome(parents[p1], parents[p2], mix(seed + i * 10007u), true);
        return;
    }
    if (i == 0) {
        Word best = 0, best_sc = fit[0].total;
        for (Word k = 1; k < n; ++k) {
            if (fit[k].total > best_sc) {
                best_sc = fit[k].total;
                best = k;
            }
        }
        children[0] = parents[best];
        return;
    }
    auto pick_parent = [&](Word salt) -> Word {
        Word best = mix(seed + i * 65537u + salt) % n;
        Word best_sc = fit[best].total;
        for (Word k = 1; k < 6; ++k) {
            Word cand = mix(seed + i * 65537u + salt + k * 27449u) % n;
            if (fit[cand].total > best_sc) {
                best_sc = fit[cand].total;
                best = cand;
            }
        }
        return best;
    };
    Word p1 = pick_parent(11u);
    Word p2 = pick_parent(97u);
    children[i] = mutate_genome(parents[p1], parents[p2], mix(seed + i * 0x85ebca6bu), p1 != p2);
}

KERNEL void mind_area_step(CorticalArea *area, const CorticalGenome *genome, const SpikeVec *basal,
                           const SpikeVec *apical, const SpikeVec *lateral) {
    Word lane = lane_id();
    if (thread_id() < 32u)
        cortical_area_step(*area, *genome, basal->bits[lane], apical->bits[lane], lateral->bits[lane], lane);
}

// 26-SM Blackwell Parallel Hippocampus: Stage 1 (32 blocks x 128 threads) + Stage 2 (1 block x 32 threads)
KERNEL void mind_hipp_query_s1(const Engram *engrams, Word n_engrams, const HippQuery *q, HippBlockPartial *partials) {
    hipp_query_stage1(engrams, n_engrams, *q, partials);
}

KERNEL void mind_hipp_query_s2(const Engram *engrams, const HippQuery *q, const HippBlockPartial *partials,
                               HippResult *out) {
    hipp_query_stage2(engrams, *q, partials, *out);
}

KERNEL void mind_hipp_supersede(Engram *engrams, Word n_engrams, const Engram *incoming, const CorticalGenome *g,
                                Word mutable_rel, Word *out_superseded, Word *out_contradictions) {
    hipp_supersede_block(engrams, n_engrams, *incoming, *g, mutable_rel, out_superseded, out_contradictions);
}

KERNEL void mind_plan_search(const CausalSchema *schemas, const PlanQuery *q, PlanNode *queue, Word *seen_hashes,
                             PlanResult *out) {
    if (global_tid() == 0)
        run_spiking_plan(schemas, *q, queue, seen_hashes, *out);
}

KERNEL void mind_plan_counterfactual(const CausalSchema *schemas, const PlanQuery *q, const Word *steps, Word n_steps,
                                     Word omit_pos, PlanResult *out) {
    if (global_tid() == 0)
        run_counterfactual(schemas, *q, steps, n_steps, omit_pos, *out);
}

KERNEL void mind_neo_sleep_ripple(signed char *weights_all, const NeoPair *pairs, Word n_pairs,
                                  const CorticalGenome *g, Word *out_edits, Word *out_spikes) {
    neo_sleep_ripple_kernel(weights_all, pairs, n_pairs, *g, out_edits, out_spikes);
}

KERNEL void mind_neo_settle(const signed char *weights_all, Word pathway, const SpikeVec *cue, Word use_inhib_pw,
                            const SpikeVec *inhib_cue, Word top_k, int min_curr, NeoQueryResult *out) {
    neo_settle_block(weights_all, pathway, *cue, use_inhib_pw, *inhib_cue, top_k, min_curr, *out);
}

KERNEL void mind_neo_similarity(const signed char *weights_all, const SpikeVec *cue, const SpikeVec *self_mask,
                                Word top_k, int min_curr, NeoQueryResult *out) {
    neo_similarity_block(weights_all, *cue, *self_mask, top_k, min_curr, *out);
}

KERNEL void mind_neo_think_chain(const signed char *weights_all, const NeoChainQuery *q, NeoChainResult *out) {
    neo_think_chain_block(weights_all, *q, *out);
}

KERNEL void mind_wm_attend(const WmAttendQuery *q, WmAttendResult *out) {
    wm_attend_block(*q, *out);
}

KERNEL void mind_neo_contrast(const signed char *weights_all, const NeoContrastQuery *q, NeoContrastResult *out) {
    neo_contrast_block(weights_all, *q, *out);
}

KERNEL void mind_neo_analogy(const signed char *weights_all, const NeoAnalogyQuery *q, NeoAnalogyResult *out) {
    neo_analogy_block(weights_all, *q, *out);
}

KERNEL void mind_neo_compose(const signed char *weights_all, const NeoComposeQuery *q, NeoComposeResult *out) {
    neo_compose_block(weights_all, *q, *out);
}

KERNEL void mind_neo_multi_cue(const signed char *weights_all, const NeoMultiCueQuery *q, NeoMultiCueResult *out) {
    neo_multi_cue_block(weights_all, *q, *out);
}

KERNEL void mind_neo_hypothesize(const signed char *weights_all, const NeoHypothesizeQuery *q,
                                 NeoHypothesizeResult *out) {
    neo_hypothesize_block(weights_all, *q, *out);
}

KERNEL void mind_neo_bridge(const signed char *weights_all, const NeoBridgeQuery *q, NeoBridgeResult *out) {
    neo_bridge_block(weights_all, *q, *out);
}

KERNEL void mind_neo_cluster(const signed char *weights_all, const NeoClusterQuery *q, NeoClusterResult *out) {
    neo_cluster_block(weights_all, *q, *out);
}

KERNEL void mind_neo_gate(const signed char *weights_all, const NeoGateQuery *q, NeoGateResult *out) {
    neo_gate_block(weights_all, *q, *out);
}

KERNEL void mind_neo_dream(signed char *weights_all, const NeoDreamQuery *q, const CorticalGenome *g,
                           NeoDreamResult *out) {
    neo_dream_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_scan(const signed char *weights_all, const NeoScanQuery *q, NeoScanResult *out) {
    neo_scan_block(weights_all, *q, *out);
}

KERNEL void mind_neo_deduce(const signed char *weights_all, const NeoDeduceQuery *q, NeoDeduceResult *out) {
    neo_deduce_block(weights_all, *q, *out);
}

KERNEL void mind_neo_abduce_back(const signed char *weights_all, const NeoAbduceBackQuery *q,
                                 NeoAbduceBackResult *out) {
    neo_abduce_back_block(weights_all, *q, *out);
}

KERNEL void mind_neo_counterfact(const signed char *weights_all, const NeoCounterfactQuery *q,
                                 NeoCounterfactResult *out) {
    neo_counterfact_block(weights_all, *q, *out);
}

KERNEL void mind_neo_plan_chain(const signed char *weights_all, const NeoPlanChainQuery *q,
                                NeoPlanChainResult *out) {
    neo_plan_chain_block(weights_all, *q, *out);
}

KERNEL void mind_neo_simulate(const signed char *weights_all, const NeoSimulateQuery *q,
                              NeoSimulateResult *out) {
    neo_simulate_block(weights_all, *q, *out);
}

KERNEL void mind_neo_constrain(const signed char *weights_all, const NeoConstrainQuery *q,
                               NeoConstrainResult *out) {
    neo_constrain_block(weights_all, *q, *out);
}

KERNEL void mind_neo_induce(const signed char *weights_all, const NeoInduceQuery *q,
                            NeoInduceResult *out) {
    neo_induce_block(weights_all, *q, *out);
}

KERNEL void mind_neo_chain_rule(const signed char *weights_all, const NeoChainRuleQuery *q,
                                NeoChainRuleResult *out) {
    neo_chain_rule_block(weights_all, *q, *out);
}

KERNEL void mind_neo_online_schema(signed char *weights_all, const NeoOnlineSchemaQuery *q,
                                   const CorticalGenome *g, NeoOnlineSchemaResult *out) {
    neo_online_schema_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_revise_schema(signed char *weights_all, const NeoReviseSchemaQuery *q,
                                   const CorticalGenome *g, NeoReviseSchemaResult *out) {
    neo_revise_schema_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_role_bind(signed char *weights_all, const NeoRoleBindQuery *q,
                               const CorticalGenome *g, NeoRoleBindResult *out) {
    neo_role_bind_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_seq_predict(signed char *weights_all, const NeoSeqPredictQuery *q,
                                 const CorticalGenome *g, NeoSeqPredictResult *out) {
    neo_seq_predict_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_neural_math(signed char *weights_all, const NeoNeuralMathQuery *q,
                                 const CorticalGenome *g, NeoNeuralMathResult *out) {
    neo_neural_math_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_graph_wave(signed char *weights_all, const NeoGraphWaveQuery *q,
                                const CorticalGenome *g, NeoGraphWaveResult *out) {
    neo_graph_wave_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_sat_solve(signed char *weights_all, const NeoSatSolveQuery *q,
                               const CorticalGenome *g, NeoSatSolveResult *out) {
    neo_sat_solve_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_visual_cortex(signed char *weights_all, const NeoVisualCortexQuery *q,
                                   const CorticalGenome *g, NeoVisualCortexResult *out) {
    neo_visual_cortex_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_audio_cortex(signed char *weights_all, const NeoAudioCortexQuery *q,
                                  const CorticalGenome *g, NeoAudioCortexResult *out) {
    neo_audio_cortex_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_crossmodal_stack(signed char *weights_all, const NeoCrossModalStackQuery *q,
                                      const CorticalGenome *g, NeoCrossModalStackResult *out) {
    neo_crossmodal_stack_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_dialogue_cortex(signed char *weights_all, const NeoDialogueQuery *q,
                                     const CorticalGenome *g, NeoDialogueResult *out) {
    neo_dialogue_cortex_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_narrative_metaphor(signed char *weights_all, const NeoNarrativeMetaphorQuery *q,
                                        const CorticalGenome *g, NeoNarrativeMetaphorResult *out) {
    neo_narrative_metaphor_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_belief_debate(signed char *weights_all, const NeoBeliefDebateQuery *q,
                                   const CorticalGenome *g, NeoBeliefDebateResult *out) {
    neo_belief_debate_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_collective_discovery(signed char *weights_all, const NeoCollectiveDiscoveryQuery *q,
                                          const CorticalGenome *g, NeoCollectiveDiscoveryResult *out) {
    neo_collective_discovery_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_self_research(signed char *weights_all, const NeoSelfResearchQuery *q,
                                   const CorticalGenome *g, NeoSelfResearchResult *out) {
    neo_self_research_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_symbolic_calculus(signed char *weights_all, const NeoSymCalcQuery *q,
                                       const CorticalGenome *g, NeoSymCalcResult *out) {
    neo_symbolic_calculus_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_program_ast(signed char *weights_all, const NeoProgAstQuery *q,
                                 const CorticalGenome *g, NeoProgAstResult *out) {
    neo_program_ast_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_linear_algebra(signed char *weights_all, const NeoLinAlgQuery *q,
                                    const CorticalGenome *g, NeoLinAlgResult *out) {
    neo_linear_algebra_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_bayes_net(signed char *weights_all, const NeoBayesNetQuery *q,
                               const CorticalGenome *g, NeoBayesNetResult *out) {
    neo_bayes_net_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_mdp_planner(signed char *weights_all, const NeoMdpPlanQuery *q,
                                 const CorticalGenome *g, NeoMdpPlanResult *out) {
    neo_mdp_planner_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_kalman_filter(signed char *weights_all, const NeoKalmanQuery *q,
                                   const CorticalGenome *g, NeoKalmanResult *out) {
    neo_kalman_filter_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_lqr_control(signed char *weights_all, const NeoLqrQuery *q,
                                 const CorticalGenome *g, NeoLqrResult *out) {
    neo_lqr_control_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_fft_spectrum(signed char *weights_all, const NeoFftSpecQuery *q,
                                  const CorticalGenome *g, NeoFftSpecResult *out) {
    neo_fft_spectrum_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_ode_rk4(signed char *weights_all, const NeoOdeRk4Query *q,
                             const CorticalGenome *g, NeoOdeRk4Result *out) {
    neo_ode_rk4_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_simplex_lp(signed char *weights_all, const NeoSimplexLpQuery *q,
                                const CorticalGenome *g, NeoSimplexLpResult *out) {
    neo_simplex_lp_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_pca_eigen(signed char *weights_all, const NeoPcaEigenQuery *q,
                               const CorticalGenome *g, NeoPcaEigenResult *out) {
    neo_pca_eigen_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_pde_solver(signed char *weights_all, const NeoPdeSolverQuery *q,
                                const CorticalGenome *g, NeoPdeSolverResult *out) {
    neo_pde_solver_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_mcmc_sampler(signed char *weights_all, const NeoMcmcSamplerQuery *q,
                                  const CorticalGenome *g, NeoMcmcSamplerResult *out) {
    neo_mcmc_sampler_block(weights_all, *q, *g, *out);
}

KERNEL void mind_neo_mcts_planner(signed char *weights_all, const NeoMctsPlannerQuery *q,
                                  const CorticalGenome *g, NeoMctsPlannerResult *out) {
    neo_mcts_planner_block(weights_all, *q, *g, *out);
}

KERNEL void mind_spatial_wave(const SpatialMapQuery *q, SpatialPathResult *out) {
    spatial_wave_block(*q, *out);
}

KERNEL void mind_bench_sdr(const CorticalGenome *g, Word n_warps, Word ticks, Word *out_spikes) {
    Word warp_global = global_tid() >> 5;
    Word lane = lane_id();
    if (warp_global >= n_warps)
        return;

    Word leak = leak_shift(*g);
    Word bac = bac_gain(*g);
    Word k = kwta_target(*g);

    short v[32];
    Word seed = mix(warp_global * 0x9e3779b9u + lane * 7331u + 19u);
#pragma unroll
    for (int b = 0; b < 32; ++b)
        v[b] = short((mix(seed + b * 17u) & 31u));

    Word spike_acc = 0;
    Word prev_sp = seed;
    for (Word t = 0; t < ticks; ++t) {
        Word basal = rotl32(prev_sp, (t + lane) & 31u) & mix(seed + t * 131u);
        Word apical = warp_permute_fwd(prev_sp, t & 7u, lane);
#pragma unroll
        for (int b = 0; b < 32; ++b) {
            Word bit = 1u << b;
            bool hb = (basal & bit) != 0;
            bool ha = (apical & bit) != 0;
            int nv = (v[b] - (v[b] >> leak)) + (hb ? 40 : 0) + (ha ? 16 : 0) + ((hb && ha) ? int(bac) * 8 : 0);
            if (nv > 511) nv = 511;
            v[b] = short(nv);
        }
        Word fired = warp_kwta_select(v, k, lane);
#pragma unroll
        for (int b = 0; b < 32; ++b)
            if (fired & (1u << b)) v[b] = 0;
        prev_sp = fired;
        spike_acc += warp_popcount(fired);
    }
    if (lane == 0)
        out_spikes[warp_global] = spike_acc;
}
