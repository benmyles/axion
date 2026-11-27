#include "axion/mind.hpp"
#include "axion/journal.hpp"
#include "mind_homestead.hpp"
#include <charconv>
#include <numeric>

namespace axion::mind {
namespace {

Word parse_num(std::string_view s, Word lo, Word hi, std::string_view flag) {
    Word v = 0;
    auto [end, err] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (err != std::errc{} || end != s.data() + s.size() || v < lo || v > hi)
        throw std::invalid_argument("invalid value for " + std::string(flag));
    return v;
}

// ---------------------------------------------------------------------------
// 1. Massively Parallel 10-Pillar GPU Evolution of CorticalGenomes (SM 12.0)
// ---------------------------------------------------------------------------
CorticalGenome run_evolution(Session &s, const EvoConfig &cfg, std::vector<EvoGenSummary> &history, bool quiet = false) {
    Function fn_init{}, fn_eval{}, fn_breed{};
    s.bind(fn_init, "mind_evo_init", Image::mind);
    s.bind(fn_eval, "mind_evo_eval", Image::mind);
    s.bind(fn_breed, "mind_evo_breed", Image::mind);

    Word n = cfg.population;
    Temporary d_pop_a(s.d, size_t(n) * sizeof(CorticalGenome));
    Temporary d_pop_b(s.d, size_t(n) * sizeof(CorticalGenome));
    Temporary d_fit(s.d, size_t(n) * sizeof(EvoFitness));
    Event ev_a{}, ev_b{};
    s.d.check(s.d.event_new(&ev_a, 0), "evo timer start");
    s.d.check(s.d.event_new(&ev_b, 0), "evo timer stop");

    Word blocks = (n + 127) / 128;
    Word init_seed = cfg.seed;
    void *init_args[]{&d_pop_a.ptr, &n, &init_seed};
    s.d.check(s.d.launch(fn_init, blocks, 1, 1, 128, 1, 1, 0, nullptr, init_args, nullptr), "init cortical pop");

    std::vector<EvoFitness> h_fit(n);
    DevicePtr cur = d_pop_a.ptr, nxt = d_pop_b.ptr;
    CorticalGenome best_genome{};
    Word best_ever = 0;

    for (Word gen = 0; gen < cfg.generations; ++gen) {
        Word gen_seed = mix(cfg.seed ^ (gen * 0x9e3779b9u + 0x243f6a88u));
        EvoConfig c = cfg;

        s.d.check(s.d.record(ev_a, nullptr), "rec evo start");
        void *eval_args[]{&cur, &d_fit.ptr, &c, &gen_seed};
        s.d.check(s.d.launch(fn_eval, blocks, 1, 1, 128, 1, 1, 0, nullptr, eval_args, nullptr), "eval cortical gen");
        s.d.check(s.d.record(ev_b, nullptr), "rec evo stop");
        s.d.check(s.d.wait(ev_b), "wait evo gen");
        float ms = 0.0f;
        s.d.check(s.d.elapsed(&ms, ev_a, ev_b), "elapsed evo");

        s.d.check(s.d.download(h_fit.data(), d_fit.ptr, size_t(n) * sizeof(EvoFitness)), "download fitness");

        EvoGenSummary sum{};
        sum.generation = gen + 1;
        sum.population = n;
        sum.gpu_ms = ms;
        Word best_idx = 0;
        for (Word i = 0; i < n; ++i) {
            const auto &f = h_fit[i];
            sum.mean_total += float(f.total);
            sum.mean_ground += float(f.grounding);
            sum.mean_episodic += float(f.episodic);
            sum.mean_tom += float(f.theory_of_mind);
            sum.mean_causal += float(f.causal_plan);
            sum.mean_counter += float(f.counterfactual);
            sum.mean_rev += float(f.revision);
            sum.mean_uncert += float(f.uncertainty);
            sum.mean_logic += float(f.logic);
            sum.mean_social += float(f.social_coop);
            sum.mean_cons += float(f.consolidate);
            sum.mean_abduct += float(f.abduct);
            sum.mean_vigil += float(f.vigilance);
            sum.mean_rule += float(f.rule_transfer);
            sum.mean_prosp += float(f.prospective);
            sum.mean_seq += float(f.sequence);
            sum.mean_consist += float(f.consistency);
            sum.mean_norm += float(f.deontic_norm);
            sum.mean_phys += float(f.intuitive_phys);
            sum.mean_sci += float(f.sci_hyp);
            sum.mean_game += float(f.game_strat);
            sum.mean_narr += float(f.narrative_wm);
            sum.mean_opt += float(f.alloc_opt);
            sum.mean_bayes += float(f.bayes_update);
            sum.mean_hier += float(f.abstr_plan);
            sum.mean_analogy += float(f.analogy_xfer);
            sum.mean_msyn += float(f.meta_syntax);
            sum.mean_comp += float(f.comp_intersect);
            sum.mean_selfm += float(f.self_model);
            sum.mean_sit += float(f.sit_model);
            sum.mean_info += float(f.info_gain);
            sum.mean_ograph += float(f.open_graph);
            sum.mean_hrank += float(f.hyp_rank);
            sum.mean_coref += float(f.coref_disc);
            sum.mean_stoch += float(f.stoch_plan);
            sum.mean_clust += float(f.cluster_odd);
            sum.mean_coal += float(f.contract_coal);
            sum.mean_gate += float(f.ep_sem_gate);
            sum.mean_mdp += float(f.policy_mdp);
            sum.mean_dream += float(f.dream_closure);
            sum.mean_mkt += float(f.market_physics);
            sum.mean_quant += float(f.quant_counterex);
            sum.mean_thfin += float(f.theory_finance);
            sum.mean_deduce += float(f.deduce_chain);
            sum.mean_gbayes += float(f.graph_opt_bayes);
            sum.mean_abdbk += float(f.abduce_back);
            sum.mean_dynsoc += float(f.dynamics_social);
            sum.mean_cfimg += float(f.counterfact_img);
            sum.mean_algec += float(f.formal_alg_econ);
            sum.mean_plan += float(f.plan_chain);
            sum.mean_socpl += float(f.soc_phys_lang);
            sum.mean_csim += float(f.causal_sim);
            sum.mean_sciv26 += float(f.sci_lang_v26);
            sum.mean_csat += float(f.constrain_sat);
            sum.mean_pev27 += float(f.phys_econ_v27);
            sum.mean_ind += float(f.induce_rel);
            sum.mean_aor28 += float(f.auto_orb_v28);
            sum.mean_chrl += float(f.chain_rule);
            sum.mean_rc29 += float(f.rel_ctrl_v29);
            sum.mean_osch += float(f.online_schema);
            sum.mean_qc30 += float(f.quant_crypto_v30);
            sum.mean_rvsc += float(f.revise_schema);
            sum.mean_cs31 += float(f.calc_sys_v31);
            sum.mean_rbnd += float(f.role_bind);
            sum.mean_ta32 += float(f.thermo_arch_v32);
            sum.mean_sqpr += float(f.seq_predict);
            sum.mean_s533 += float(f.stage5_sci_v33);
            sum.mean_nmat += float(f.neural_math);
            sum.mean_s534 += float(f.stage5b_sci_v34);
            sum.mean_gwav += float(f.graph_wave);
            sum.mean_s535 += float(f.stage5c_sci_v35);
            sum.mean_sats += float(f.sat_solve);
            sum.mean_s536 += float(f.stage5d_sci_v36);
            sum.mean_visv += float(f.visual_v1v4);
            sum.mean_s637 += float(f.stage6_sci_v37);
            sum.mean_audv += float(f.audio_a1);
            sum.mean_s638 += float(f.stage6b_sci_v38);
            sum.mean_xmod += float(f.crossmodal_stack);
            sum.mean_s639 += float(f.stage6c_sci_v39);
            sum.mean_dlgc += float(f.dialogue_ctx);
            sum.mean_s740 += float(f.stage7a_sci_v40);
            sum.mean_nmet += float(f.narrative_met);
            sum.mean_s741 += float(f.stage7b_sci_v41);
            sum.mean_bdeb += float(f.belief_deb);
            sum.mean_s742 += float(f.stage7c_sci_v42);
            sum.mean_cdis += float(f.coll_disc);
            sum.mean_s843 += float(f.stage8_sci_v43);
            sum.mean_sres += float(f.self_res);
            sum.mean_s844 += float(f.stage8b_sci_v44);
            sum.mean_symc += float(f.sym_calc);
            sum.mean_s845 += float(f.stage8c_sci_v45);
            sum.mean_past += float(f.prog_ast);
            sum.mean_s846 += float(f.stage8d_sci_v46);
            sum.mean_lalg += float(f.lin_alg);
            sum.mean_s847 += float(f.stage8e_sci_v47);
            sum.mean_bnet += float(f.bayes_net);
            sum.mean_s848 += float(f.stage8f_sci_v48);
            sum.mean_mdpp += float(f.mdp_plan);
            sum.mean_s849 += float(f.stage8g_sci_v49);
            sum.mean_kflt += float(f.kalman_flt);
            sum.mean_s850 += float(f.stage8h_sci_v50);
            sum.mean_lqrc += float(f.lqr_ctrl);
            sum.mean_s851 += float(f.stage8i_sci_v51);
            sum.mean_ffts += float(f.fft_spec);
            sum.mean_s852 += float(f.stage8j_sci_v52);
            sum.mean_odrk += float(f.ode_rk4);
            sum.mean_s853 += float(f.stage8k_sci_v53);
            sum.mean_sxlp += float(f.simplex_lp);
            sum.mean_s854 += float(f.stage8l_sci_v54);
            sum.mean_pcae += float(f.pca_eigen);
            sum.mean_s855 += float(f.stage8m_sci_v55);
            sum.mean_pdes += float(f.pde_solver);
            sum.mean_s856 += float(f.stage8n_sci_v56);
            sum.mean_mcmcs += float(f.mcmc_sampler);
            sum.mean_s857 += float(f.stage8o_sci_v57);
            sum.mean_mctsp += float(f.mcts_planner);
            sum.mean_s858 += float(f.stage8p_sci_v58);
            sum.total_spikes += f.spikes;
            sum.total_edits += f.edits;
            if (f.total > h_fit[best_idx].total)
                best_idx = i;
        }
        sum.mean_total /= float(n);
        sum.mean_ground /= float(n);
        sum.mean_episodic /= float(n);
        sum.mean_tom /= float(n);
        sum.mean_causal /= float(n);
        sum.mean_counter /= float(n);
        sum.mean_rev /= float(n);
        sum.mean_uncert /= float(n);
        sum.mean_logic /= float(n);
        sum.mean_social /= float(n);
        sum.mean_cons /= float(n);
        sum.mean_abduct /= float(n);
        sum.mean_vigil /= float(n);
        sum.mean_rule /= float(n);
        sum.mean_prosp /= float(n);
        sum.mean_seq /= float(n);
        sum.mean_consist /= float(n);
        sum.mean_norm /= float(n);
        sum.mean_phys /= float(n);
        sum.mean_sci /= float(n);
        sum.mean_game /= float(n);
        sum.mean_narr /= float(n);
        sum.mean_opt /= float(n);
        sum.mean_bayes /= float(n);
        sum.mean_hier /= float(n);
        sum.mean_analogy /= float(n);
        sum.mean_msyn /= float(n);
        sum.mean_comp /= float(n);
        sum.mean_selfm /= float(n);
        sum.mean_sit /= float(n);
        sum.mean_info /= float(n);
        sum.mean_ograph /= float(n);
        sum.mean_hrank /= float(n);
        sum.mean_coref /= float(n);
        sum.mean_stoch /= float(n);
        sum.mean_clust /= float(n);
        sum.mean_coal /= float(n);
        sum.mean_gate /= float(n);
        sum.mean_mdp /= float(n);
        sum.mean_dream /= float(n);
        sum.mean_mkt /= float(n);
        sum.mean_quant /= float(n);
        sum.mean_thfin /= float(n);
        sum.mean_deduce /= float(n);
        sum.mean_gbayes /= float(n);
        sum.mean_abdbk /= float(n);
        sum.mean_dynsoc /= float(n);
        sum.mean_cfimg /= float(n);
        sum.mean_algec /= float(n);
        sum.mean_plan /= float(n);
        sum.mean_socpl /= float(n);
        sum.mean_csim /= float(n);
        sum.mean_sciv26 /= float(n);
        sum.mean_csat /= float(n);
        sum.mean_pev27 /= float(n);
        sum.mean_ind /= float(n);
        sum.mean_aor28 /= float(n);
        sum.mean_chrl /= float(n);
        sum.mean_rc29 /= float(n);
        sum.mean_osch /= float(n);
        sum.mean_qc30 /= float(n);
        sum.mean_rvsc /= float(n);
        sum.mean_cs31 /= float(n);
        sum.mean_rbnd /= float(n);
        sum.mean_ta32 /= float(n);
        sum.mean_sqpr /= float(n);
        sum.mean_s533 /= float(n);
        sum.mean_nmat /= float(n);
        sum.mean_s534 /= float(n);
        sum.mean_gwav /= float(n);
        sum.mean_s535 /= float(n);
        sum.mean_sats /= float(n);
        sum.mean_s536 /= float(n);
        sum.mean_visv /= float(n);
        sum.mean_s637 /= float(n);
        sum.mean_audv /= float(n);
        sum.mean_s638 /= float(n);
        sum.mean_xmod /= float(n);
        sum.mean_s639 /= float(n);
        sum.mean_dlgc /= float(n);
        sum.mean_s740 /= float(n);
        sum.mean_nmet /= float(n);
        sum.mean_s741 /= float(n);
        sum.mean_bdeb /= float(n);
        sum.mean_s742 /= float(n);
        sum.mean_cdis /= float(n);
        sum.mean_s843 /= float(n);
        sum.mean_sres /= float(n);
        sum.mean_s844 /= float(n);
        sum.mean_symc /= float(n);
        sum.mean_s845 /= float(n);
        sum.mean_past /= float(n);
        sum.mean_s846 /= float(n);
        sum.mean_lalg /= float(n);
        sum.mean_s847 /= float(n);
        sum.mean_bnet /= float(n);
        sum.mean_s848 /= float(n);
        sum.mean_mdpp /= float(n);
        sum.mean_s849 /= float(n);
        sum.mean_kflt /= float(n);
        sum.mean_s850 /= float(n);
        sum.mean_lqrc /= float(n);
        sum.mean_s851 /= float(n);
        sum.mean_ffts /= float(n);
        sum.mean_s852 /= float(n);
        sum.mean_odrk /= float(n);
        sum.mean_s853 /= float(n);
        sum.mean_sxlp /= float(n);
        sum.mean_s854 /= float(n);
        sum.mean_pcae /= float(n);
        sum.mean_s855 /= float(n);
        sum.mean_pdes /= float(n);
        sum.mean_s856 /= float(n);
        sum.mean_mcmcs /= float(n);
        sum.mean_s857 /= float(n);
        sum.mean_mctsp /= float(n);
        sum.mean_s858 /= float(n);
        sum.best_total = float(h_fit[best_idx].total);
        history.push_back(sum);

        if (h_fit[best_idx].total >= best_ever) {
            best_ever = h_fit[best_idx].total;
            s.d.check(s.d.download(&best_genome, cur + size_t(best_idx) * sizeof(CorticalGenome),
                                   sizeof(CorticalGenome)),
                      "download champion");
        }

        if (!quiet && (gen == 0 || gen + 1 == cfg.generations || (gen + 1) % std::max(1u, cfg.generations / 8) == 0)) {
            Word max_possible = cfg.worlds_per_gen * EVO_MAX_PER_WORLD;
            std::cout << "mind_evo=" << (cfg.drift ? "drift" : "select") << "  gen=" << sum.generation
                      << "  pop=" << n << "  mean=" << std::fixed << std::setprecision(1) << sum.mean_total << "/"
                      << max_possible << "  best=" << int(sum.best_total) << "/" << max_possible
                      << "  ground=" << sum.mean_ground << "  episodic=" << sum.mean_episodic
                      << "  tom=" << sum.mean_tom << "  causal=" << sum.mean_causal << "  cf=" << sum.mean_counter
                      << "  rev=" << sum.mean_rev << "  uncert=" << sum.mean_uncert
                      << "  logic=" << sum.mean_logic << "  social=" << sum.mean_social
                      << "  cons=" << sum.mean_cons << "  abd=" << sum.mean_abduct << "  vig=" << sum.mean_vigil
                      << "  rule=" << sum.mean_rule << "  prosp=" << sum.mean_prosp
                      << "  seq=" << sum.mean_seq << "  consist=" << sum.mean_consist
                      << "  norm=" << sum.mean_norm << "  phys=" << sum.mean_phys
                      << "  sci=" << sum.mean_sci << "  game=" << sum.mean_game
                      << "  narr=" << sum.mean_narr << "  opt=" << sum.mean_opt
                      << "  bayes=" << sum.mean_bayes << "  hier=" << sum.mean_hier
                      << "  ana=" << sum.mean_analogy << "  msyn=" << sum.mean_msyn
                      << "  comp=" << sum.mean_comp << "  selfm=" << sum.mean_selfm
                      << "  sit=" << sum.mean_sit << "  info=" << sum.mean_info
                      << "  ograph=" << sum.mean_ograph << "  hrank=" << sum.mean_hrank
                      << "  coref=" << sum.mean_coref << "  stoch=" << sum.mean_stoch
                      << "  clust=" << sum.mean_clust << "  coal=" << sum.mean_coal
                      << "  gate=" << sum.mean_gate << "  mdp=" << sum.mean_mdp
                      << "  dream=" << sum.mean_dream << "  mkt=" << sum.mean_mkt
                      << "  quant=" << sum.mean_quant << "  thfin=" << sum.mean_thfin
                      << "  deduce=" << sum.mean_deduce << "  gbayes=" << sum.mean_gbayes
                      << "  abdbk=" << sum.mean_abdbk << "  dynsoc=" << sum.mean_dynsoc
                      << "  cfimg=" << sum.mean_cfimg << "  algec=" << sum.mean_algec
                      << "  plan=" << sum.mean_plan << "  socpl=" << sum.mean_socpl
                      << "  csim=" << sum.mean_csim << "  sciv26=" << sum.mean_sciv26
                      << "  csat=" << sum.mean_csat << "  pev27=" << sum.mean_pev27
                      << "  ind=" << sum.mean_ind << "  aor28=" << sum.mean_aor28
                      << "  chrl=" << sum.mean_chrl << "  rc29=" << sum.mean_rc29
                      << "  osch=" << sum.mean_osch << "  qc30=" << sum.mean_qc30
                      << "  rvsc=" << sum.mean_rvsc << "  cs31=" << sum.mean_cs31
                      << "  rbnd=" << sum.mean_rbnd << "  ta32=" << sum.mean_ta32
                      << "  sqpr=" << sum.mean_sqpr << "  s533=" << sum.mean_s533
                      << "  nmat=" << sum.mean_nmat << "  s534=" << sum.mean_s534
                      << "  gwav=" << sum.mean_gwav << "  s535=" << sum.mean_s535
                      << "  sats=" << sum.mean_sats << "  s536=" << sum.mean_s536
                      << "  visv=" << sum.mean_visv << "  s637=" << sum.mean_s637
                      << "  audv=" << sum.mean_audv << "  s638=" << sum.mean_s638
                      << "  xmod=" << sum.mean_xmod << "  s639=" << sum.mean_s639
                      << "  dlgc=" << sum.mean_dlgc << "  s740=" << sum.mean_s740
                      << "  nmet=" << sum.mean_nmet << "  s741=" << sum.mean_s741
                      << "  bdeb=" << sum.mean_bdeb << "  s742=" << sum.mean_s742
                      << "  cdis=" << sum.mean_cdis << "  s843=" << sum.mean_s843
                      << "  sres=" << sum.mean_sres << "  s844=" << sum.mean_s844
                      << "  symc=" << sum.mean_symc << "  s845=" << sum.mean_s845
                      << "  past=" << sum.mean_past << "  s846=" << sum.mean_s846
                      << "  lalg=" << sum.mean_lalg << "  s847=" << sum.mean_s847
                      << "  bnet=" << sum.mean_bnet << "  s848=" << sum.mean_s848
                      << "  mdpp=" << sum.mean_mdpp << "  s849=" << sum.mean_s849
                      << "  kflt=" << sum.mean_kflt << "  s850=" << sum.mean_s850
                      << "  lqrc=" << sum.mean_lqrc << "  s851=" << sum.mean_s851
                      << "  ffts=" << sum.mean_ffts << "  s852=" << sum.mean_s852
                      << "  odrk=" << sum.mean_odrk << "  s853=" << sum.mean_s853
                      << "  sxlp=" << sum.mean_sxlp << "  s854=" << sum.mean_s854
                      << "  pcae=" << sum.mean_pcae << "  s855=" << sum.mean_s855
                      << "  pdes=" << sum.mean_pdes << "  s856=" << sum.mean_s856
                      << "  mcmcs=" << sum.mean_mcmcs << "  s857=" << sum.mean_s857
                      << "  mctsp=" << sum.mean_mctsp << "  s858=" << sum.mean_s858
                      << "  spikes=" << sum.total_spikes << "  edits=" << sum.total_edits
                      << "  gpu_ms=" << std::setprecision(3) << sum.gpu_ms << '\n';
            if (auto *j = journal::current())
                j->breeding.push_back({sum.generation, n, {Count(sum.mean_total * n), Count(sum.best_total)}});
        }

        if (gen + 1 < cfg.generations) {
            Word breed_seed = mix(gen_seed ^ 0x4b5c6d7eu);
            Word drift_flag = cfg.drift;
            void *b_args[]{&cur, &nxt, &d_fit.ptr, &n, &breed_seed, &drift_flag};
            s.d.check(s.d.launch(fn_breed, blocks, 1, 1, 128, 1, 1, 0, nullptr, b_args, nullptr), "breed cortical gen");
            std::swap(cur, nxt);
        }
    }

    s.d.event_free(ev_b);
    s.d.event_free(ev_a);
    return best_genome;
}

// ---------------------------------------------------------------------------
// 2. Blackwell SM 12.0 Cortical Column & 26-SM Parallel Hippocampus Benchmark
// ---------------------------------------------------------------------------
void run_bench(Session &s, const CorticalGenome &g, Word warps, Word ticks, Word repeats) {
    Function fn_sdr{}, fn_s1{}, fn_s2{};
    s.bind(fn_sdr, "mind_bench_sdr", Image::mind);
    s.bind(fn_s1, "mind_hipp_query_s1", Image::mind);
    s.bind(fn_s2, "mind_hipp_query_s2", Image::mind);

    int regs = 0, lmem = 0, smem = 0;
    s.d.check(s.d.func_attr(&regs, 4, fn_sdr), "sdr regs");
    s.d.check(s.d.func_attr(&lmem, 3, fn_sdr), "sdr local");
    s.d.check(s.d.func_attr(&smem, 1, fn_sdr), "sdr shared");

    Temporary d_g(s.d, sizeof(CorticalGenome));
    Temporary d_out(s.d, size_t(warps) * sizeof(Word));
    s.d.check(s.d.upload(d_g.ptr, &g, sizeof(g)), "upload bench genome");

    Event a{}, b{};
    s.d.check(s.d.event_new(&a, 0), "bench ev a");
    s.d.check(s.d.event_new(&b, 0), "bench ev b");

    Word threads = 128, blocks = (warps + 3) / 4;
    void *args[]{&d_g.ptr, &warps, &ticks, &d_out.ptr};
    s.d.check(s.d.launch(fn_sdr, blocks, 1, 1, threads, 1, 1, 0, nullptr, args, nullptr), "warmup sdr");
    s.d.check(s.d.record(b, nullptr), "warmup rec");
    s.d.check(s.d.wait(b), "sync");

    std::vector<double> times;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec a");
        s.d.check(s.d.launch(fn_sdr, blocks, 1, 1, threads, 1, 1, 0, nullptr, args, nullptr), "bench sdr");
        s.d.check(s.d.record(b, nullptr), "rec b");
        s.d.check(s.d.wait(b), "wait b");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "elapsed");
        times.push_back(ms);
    }
    std::sort(times.begin(), times.end());
    double med_ms = times[times.size() / 2];
    double gnt_s = double(warps) * double(NEURONS) * double(ticks) / (med_ms * 1e-3) * 1e-9;

    // 26-SM Parallel Dual-Tier Hippocampus Benchmark:
    // Tier 1 (Hot L2 Working Tier: 4,096 engrams = 3,264 KiB pinned inside Blackwell's 32-MiB L2!)
    // Tier 2 (Lifelong HBM Archive: 16,384 engrams = 13,056 KiB, recalling deep episode #12,288!)
    std::vector<Engram> dummy_eng(MAX_ENGRAMS);
    for (Word i = 0; i < MAX_ENGRAMS; ++i) {
        dummy_eng[i].flags = ENG_ACTIVE;
        dummy_eng[i].timestamp = i + 1;
        dummy_eng[i].strength = 16;
        for (Word r = 0; r < NUM_ROLES; ++r)
            dummy_eng[i].role[r] = cochlear_encode("e" + std::to_string(i) + "_" + std::to_string(r), 12345u + r);
    }
    Temporary d_eng(s.d, size_t(MAX_ENGRAMS) * sizeof(Engram));
    Temporary d_hq(s.d, sizeof(HippQuery));
    Temporary d_hp(s.d, size_t(HIPP_BLOCKS) * sizeof(HippBlockPartial));
    Temporary d_hr(s.d, sizeof(HippResult));
    s.d.check(s.d.upload(d_eng.ptr, dummy_eng.data(), size_t(MAX_ENGRAMS) * sizeof(Engram)), "upload 16384 engrams");
    HippQuery hq{};
    hq.active_roles = (1u << R_SUBJ) | (1u << R_REL) | (1u << R_PERSP);
    hq.cue[R_SUBJ] = dummy_eng[2048].role[R_SUBJ];
    hq.cue[R_REL] = dummy_eng[2048].role[R_REL];
    hq.cue[R_PERSP] = dummy_eng[2048].role[R_PERSP];
    hq.require_flags = ENG_ACTIVE;
    hq.min_overlap = 14;
    s.d.check(s.d.upload(d_hq.ptr, &hq, sizeof(hq)), "upload hq");
    Word n_eng = HIPP_L2_HOT;
    void *h1[]{&d_eng.ptr, &n_eng, &d_hq.ptr, &d_hp.ptr};
    void *h2[]{&d_eng.ptr, &d_hq.ptr, &d_hp.ptr, &d_hr.ptr};
    s.d.check(s.d.launch(fn_s1, HIPP_BLOCKS, 1, 1, 128, 1, 1, 0, nullptr, h1, nullptr), "warmup s1");
    s.d.check(s.d.launch(fn_s2, 1, 1, 1, 32, 1, 1, 0, nullptr, h2, nullptr), "warmup s2");
    std::vector<double> htimes;
    constexpr Word HBATCH = 512;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec ha");
        for (Word k = 0; k < HBATCH; ++k) {
            s.d.check(s.d.launch(fn_s1, HIPP_BLOCKS, 1, 1, 128, 1, 1, 0, nullptr, h1, nullptr), "bench s1");
            s.d.check(s.d.launch(fn_s2, 1, 1, 1, 32, 1, 1, 0, nullptr, h2, nullptr), "bench s2");
        }
        s.d.check(s.d.record(b, nullptr), "rec hb");
        s.d.check(s.d.wait(b), "wait hb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "helapsed");
        htimes.push_back(ms / HBATCH);
    }
    std::sort(htimes.begin(), htimes.end());
    double hipp_us = htimes[htimes.size() / 2] * 1000.0;
    HippResult hr{};
    s.d.check(s.d.download(&hr, d_hr.ptr, sizeof(hr)), "check hipp match");

    // Tier-2 HBM Lifelong Archive Benchmark: full 16,384-engram scan retrieving deep engram #12,288!
    HippQuery hq_hbm = hq;
    hq_hbm.cue[R_SUBJ] = dummy_eng[12288].role[R_SUBJ];
    hq_hbm.cue[R_REL] = dummy_eng[12288].role[R_REL];
    hq_hbm.cue[R_PERSP] = dummy_eng[12288].role[R_PERSP];
    s.d.check(s.d.upload(d_hq.ptr, &hq_hbm, sizeof(hq_hbm)), "upload hq_hbm");
    Word n_eng_hbm = MAX_ENGRAMS;
    void *h1_hbm[]{&d_eng.ptr, &n_eng_hbm, &d_hq.ptr, &d_hp.ptr};
    s.d.check(s.d.launch(fn_s1, HIPP_BLOCKS, 1, 1, 128, 1, 1, 0, nullptr, h1_hbm, nullptr), "warmup s1 hbm");
    s.d.check(s.d.launch(fn_s2, 1, 1, 1, 32, 1, 1, 0, nullptr, h2, nullptr), "warmup s2 hbm");
    std::vector<double> hbm_times;
    constexpr Word HBMBATCH = 128;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec hbm a");
        for (Word k = 0; k < HBMBATCH; ++k) {
            s.d.check(s.d.launch(fn_s1, HIPP_BLOCKS, 1, 1, 128, 1, 1, 0, nullptr, h1_hbm, nullptr), "bench s1 hbm");
            s.d.check(s.d.launch(fn_s2, 1, 1, 1, 32, 1, 1, 0, nullptr, h2, nullptr), "bench s2 hbm");
        }
        s.d.check(s.d.record(b, nullptr), "rec hbm b");
        s.d.check(s.d.wait(b), "wait hbm b");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "hbm elapsed");
        hbm_times.push_back(ms / HBMBATCH);
    }
    std::sort(hbm_times.begin(), hbm_times.end());
    double hbm_us = hbm_times[hbm_times.size() / 2] * 1000.0;
    HippResult hr_hbm{};
    s.d.check(s.d.download(&hr_hbm, d_hr.ptr, sizeof(hr_hbm)), "check hbm match");

    // 3-MiB (3 x 1,048,576 int8 synapses) Neocortical SWR Ripple & Settling Benchmark!
    Function fn_nr{}, fn_ns{};
    s.bind(fn_nr, "mind_neo_sleep_ripple", Image::mind);
    s.bind(fn_ns, "mind_neo_settle", Image::mind);
    Temporary d_nw(s.d, size_t(NEO_PATHWAYS) * NEO_SYNAPSES);
    Temporary d_np(s.d, 128u * sizeof(NeoPair));
    Temporary d_nres(s.d, sizeof(NeoQueryResult));
    Temporary d_ned(s.d, sizeof(Word)), d_nsp(s.d, sizeof(Word));
    Temporary d_ncue(s.d, sizeof(SpikeVec)), d_ninh(s.d, sizeof(SpikeVec));
    std::vector<signed char> z_nw(size_t(NEO_PATHWAYS) * NEO_SYNAPSES, 0);
    s.d.check(s.d.upload(d_nw.ptr, z_nw.data(), z_nw.size()), "zero bench neo");
    std::vector<NeoPair> b_pairs(128);
    for (Word i = 0; i < 128; ++i) {
        b_pairs[i].pre = cochlear_encode("pre_" + std::to_string(i), 777u);
        b_pairs[i].post = cochlear_encode("post_" + std::to_string(i & 15u), 888u);
        b_pairs[i].pathway = i % NEO_PATHWAYS;
        b_pairs[i].strength = 2;
    }
    // Add a clean 2-hop chain on Pathway 4 (pre_4 -> post_4 -> post_15) for Kernel 11 (`mind_neo_bridge`)!
    b_pairs[127].pre = b_pairs[4].post;
    b_pairs[127].post = cochlear_encode("bridge_end_15", 999u);
    b_pairs[127].pathway = 4;
    b_pairs[127].strength = 4;
    s.d.check(s.d.upload(d_np.ptr, b_pairs.data(), b_pairs.size() * sizeof(NeoPair)), "upload bench pairs");
    Word np_cnt = 128, zero_w = 0, pw0 = 0, no_inh = ~0u, topk = 32;
    int minc = 4;
    s.d.check(s.d.upload(d_ned.ptr, &zero_w, sizeof(Word)), "zero ned");
    s.d.check(s.d.upload(d_nsp.ptr, &zero_w, sizeof(Word)), "zero nsp");
    s.d.check(s.d.upload(d_ncue.ptr, &b_pairs[0].pre, sizeof(SpikeVec)), "upload ncue");
    SpikeVec empty_sv{};
    s.d.check(s.d.upload(d_ninh.ptr, &empty_sv, sizeof(SpikeVec)), "upload ninh");
    void *nr_args[]{&d_nw.ptr, &d_np.ptr, &np_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
    void *ns_args[]{&d_nw.ptr, &pw0, &d_ncue.ptr, &no_inh, &d_ninh.ptr, &topk, &minc, &d_nres.ptr};
    s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, nr_args, nullptr), "bench neo ripple");
    s.d.check(s.d.launch(fn_ns, 1, 1, 1, 256, 1, 1, 0, nullptr, ns_args, nullptr), "warmup neo settle");
    std::vector<double> ntimes;
    constexpr Word NBATCH = 512;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec na");
        for (Word k = 0; k < NBATCH; ++k)
            s.d.check(s.d.launch(fn_ns, 1, 1, 1, 256, 1, 1, 0, nullptr, ns_args, nullptr), "bench neo settle");
        s.d.check(s.d.record(b, nullptr), "rec nb");
        s.d.check(s.d.wait(b), "wait nb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "nelapsed");
        ntimes.push_back(ms / NBATCH);
    }
    std::sort(ntimes.begin(), ntimes.end());
    double neo_us = ntimes[ntimes.size() / 2] * 1000.0;
    NeoQueryResult nres{};
    s.d.check(s.d.download(&nres, d_nres.ptr, sizeof(nres)), "download nres");

    // 16x16 = 256-Cell Entorhinal-Hippocampal 2D Place-Cell Wavefront Navigation Benchmark!
    Function fn_sw{};
    s.bind(fn_sw, "mind_spatial_wave", Image::mind);
    Temporary d_sq(s.d, sizeof(SpatialMapQuery)), d_sr(s.d, sizeof(SpatialPathResult));
    SpatialMapQuery sq{};
    sq.start_x = 2; sq.start_y = 7; sq.goal_x = 13; sq.goal_y = 3;
    for (Word y = 0; y < MAP_H; ++y) {
        if (y == 1u) continue;
        Word cid = y * MAP_W + 7u;
        sq.blocked[cid >> 5] |= 1u << (cid & 31u);
    }
    s.d.check(s.d.upload(d_sq.ptr, &sq, sizeof(sq)), "upload bench spatial");
    void *sw_args[]{&d_sq.ptr, &d_sr.ptr};
    s.d.check(s.d.launch(fn_sw, 1, 1, 1, MAP_CELLS, 1, 1, 0, nullptr, sw_args, nullptr), "warmup spatial");
    std::vector<double> stimes;
    constexpr Word SBATCH = 512;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec sa");
        for (Word k = 0; k < SBATCH; ++k)
            s.d.check(s.d.launch(fn_sw, 1, 1, 1, MAP_CELLS, 1, 1, 0, nullptr, sw_args, nullptr), "bench spatial");
        s.d.check(s.d.record(b, nullptr), "rec sb");
        s.d.check(s.d.wait(b), "wait sb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "selapsed");
        stimes.push_back(ms / SBATCH);
    }
    std::sort(stimes.begin(), stimes.end());
    double sp_us = stimes[stimes.size() / 2] * 1000.0;
    SpatialPathResult sres{};
    s.d.check(s.d.download(&sres, d_sr.ptr, sizeof(sres)), "download sres");

    // 3-Hop Recurrent GPU Neocortical Thought Chain (`mind_neo_think_chain`) Benchmark!
    Function fn_tc{};
    s.bind(fn_tc, "mind_neo_think_chain", Image::mind);
    Temporary d_tcq(s.d, sizeof(NeoChainQuery)), d_tcr(s.d, sizeof(NeoChainResult));
    NeoChainQuery tcq{};
    tcq.init_cue = b_pairs[0].pre;
    tcq.n_hops = 3;
    tcq.top_k = 32;
    tcq.min_curr = 2;
    for (Word h = 0; h < MAX_THINK_HOPS; ++h) {
        tcq.pathways[h] = h % NEO_PATHWAYS;
        tcq.inhib_pw[h] = ~0u;
    }
    s.d.check(s.d.upload(d_tcq.ptr, &tcq, sizeof(tcq)), "upload bench think chain");
    void *tc_args[]{&d_nw.ptr, &d_tcq.ptr, &d_tcr.ptr};
    s.d.check(s.d.launch(fn_tc, 1, 1, 1, 256, 1, 1, 0, nullptr, tc_args, nullptr), "warmup think chain");
    std::vector<double> tctimes;
    constexpr Word TCBATCH = 256;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec tca");
        for (Word k = 0; k < TCBATCH; ++k)
            s.d.check(s.d.launch(fn_tc, 1, 1, 1, 256, 1, 1, 0, nullptr, tc_args, nullptr), "bench think chain");
        s.d.check(s.d.record(b, nullptr), "rec tcb");
        s.d.check(s.d.wait(b), "wait tcb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "tcelapsed");
        tctimes.push_back(ms / TCBATCH);
    }
    std::sort(tctimes.begin(), tctimes.end());
    double tc_us = tctimes[tctimes.size() / 2] * 1000.0;
    NeoChainResult tcres{};
    s.d.check(s.d.download(&tcres, d_tcr.ptr, sizeof(tcres)), "download tcres");

    // 64-Slot GPU Working-Memory Lateral-Inhibition Attention Gate (`mind_wm_attend`) Benchmark!
    Function fn_wm{};
    s.bind(fn_wm, "mind_wm_attend", Image::mind);
    Temporary d_wmq(s.d, sizeof(WmAttendQuery)), d_wmr(s.d, sizeof(WmAttendResult));
    WmAttendQuery wmq{};
    wmq.n_slots = MAX_WM_SLOTS;
    wmq.query = cochlear_encode("bird_cat", 4242u);
    wmq.min_overlap = 12;
    wmq.ambig_margin = 6;
    for (Word i = 0; i < MAX_WM_SLOTS; ++i) {
        wmq.slot_ids[i] = i + 1u;
        wmq.slot_recency[i] = i < 2u ? 4u : 1u;
        wmq.slots[i] = i < 2u ? sdr_or(wmq.query, cochlear_encode("cand_" + std::to_string(i), 555u))
                              : cochlear_encode("other_" + std::to_string(i), 666u);
    }
    s.d.check(s.d.upload(d_wmq.ptr, &wmq, sizeof(wmq)), "upload bench wm");
    void *wm_args[]{&d_wmq.ptr, &d_wmr.ptr};
    s.d.check(s.d.launch(fn_wm, 1, 1, 1, MAX_WM_SLOTS, 1, 1, 0, nullptr, wm_args, nullptr), "warmup wm attend");
    std::vector<double> wmtimes;
    constexpr Word WMBATCH = 512;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec wma");
        for (Word k = 0; k < WMBATCH; ++k)
            s.d.check(s.d.launch(fn_wm, 1, 1, 1, MAX_WM_SLOTS, 1, 1, 0, nullptr, wm_args, nullptr), "bench wm attend");
        s.d.check(s.d.record(b, nullptr), "rec wmb");
        s.d.check(s.d.wait(b), "wait wmb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "wmelapsed");
        wmtimes.push_back(ms / WMBATCH);
    }
    std::sort(wmtimes.begin(), wmtimes.end());
    double wm_us = wmtimes[wmtimes.size() / 2] * 1000.0;
    WmAttendResult wmres{};
    s.d.check(s.d.download(&wmres, d_wmr.ptr, sizeof(wmres)), "download wmres");

    // Kernel 6: GPU Spiking Cortical Differential Contrast (`mind_neo_contrast`) Benchmark!
    Function fn_ctr{};
    s.bind(fn_ctr, "mind_neo_contrast", Image::mind);
    Temporary d_ctq(s.d, sizeof(NeoContrastQuery)), d_ctr(s.d, sizeof(NeoContrastResult));
    NeoContrastQuery ctq{};
    ctq.cue_a = b_pairs[0].pre;
    ctq.cue_b = b_pairs[1].pre;
    ctq.pos_pathway = 0;
    ctq.inhib_pathway = 2;
    ctq.top_k = 32;
    ctq.min_curr = 2;
    s.d.check(s.d.upload(d_ctq.ptr, &ctq, sizeof(ctq)), "upload bench contrast");
    void *ct_args[]{&d_nw.ptr, &d_ctq.ptr, &d_ctr.ptr};
    s.d.check(s.d.launch(fn_ctr, 1, 1, 1, 256, 1, 1, 0, nullptr, ct_args, nullptr), "warmup neo contrast");
    std::vector<double> cttimes;
    constexpr Word CTBATCH = 256;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec cta");
        for (Word k = 0; k < CTBATCH; ++k)
            s.d.check(s.d.launch(fn_ctr, 1, 1, 1, 256, 1, 1, 0, nullptr, ct_args, nullptr), "bench neo contrast");
        s.d.check(s.d.record(b, nullptr), "rec ctb");
        s.d.check(s.d.wait(b), "wait ctb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "ctelapsed");
        cttimes.push_back(ms / CTBATCH);
    }
    std::sort(cttimes.begin(), cttimes.end());
    double ct_us = cttimes[cttimes.size() / 2] * 1000.0;
    NeoContrastResult ctres{};
    s.d.check(s.d.download(&ctres, d_ctr.ptr, sizeof(ctres)), "download ctres");

    // Kernel 7: GPU Spiking Proportional Relational Analogy (`mind_neo_analogy`) Benchmark!
    Function fn_ana{};
    s.bind(fn_ana, "mind_neo_analogy", Image::mind);
    Temporary d_anq(s.d, sizeof(NeoAnalogyQuery)), d_anr(s.d, sizeof(NeoAnalogyResult));
    NeoAnalogyQuery anq{};
    anq.cue_a = b_pairs[0].pre;
    anq.cue_b = b_pairs[0].post;
    anq.cue_c = b_pairs[NEO_PATHWAYS].pre;
    anq.candidate_pws[0] = 0; anq.candidate_pws[1] = 1; anq.candidate_pws[2] = 2; anq.candidate_pws[3] = 3;
    anq.n_pws = 4;
    anq.top_k = 32;
    anq.min_curr = 2;
    s.d.check(s.d.upload(d_anq.ptr, &anq, sizeof(anq)), "upload bench analogy");
    void *an_args[]{&d_nw.ptr, &d_anq.ptr, &d_anr.ptr};
    s.d.check(s.d.launch(fn_ana, 1, 1, 1, 256, 1, 1, 0, nullptr, an_args, nullptr), "warmup neo analogy");
    std::vector<double> antimes;
    constexpr Word ANBATCH = 256;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec ana");
        for (Word k = 0; k < ANBATCH; ++k)
            s.d.check(s.d.launch(fn_ana, 1, 1, 1, 256, 1, 1, 0, nullptr, an_args, nullptr), "bench neo analogy");
        s.d.check(s.d.record(b, nullptr), "rec anb");
        s.d.check(s.d.wait(b), "wait anb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "anelapsed");
        antimes.push_back(ms / ANBATCH);
    }
    std::sort(antimes.begin(), antimes.end());
    double an_us = antimes[antimes.size() / 2] * 1000.0;
    NeoAnalogyResult anres{};
    s.d.check(s.d.download(&anres, d_anr.ptr, sizeof(anres)), "download anres");

    // Kernel 8: GPU Spiking Compositional Set Intersection & Negation-Veto (`mind_neo_compose`) Benchmark!
    Function fn_cmp{};
    s.bind(fn_cmp, "mind_neo_compose", Image::mind);
    Temporary d_cmq(s.d, sizeof(NeoComposeQuery)), d_cmr(s.d, sizeof(NeoComposeResult));
    NeoComposeQuery cmq{};
    cmq.cue_cat = b_pairs[0].pre;
    cmq.cue_pos = b_pairs[16].pre;
    cmq.pw_cat = 0;
    cmq.pw_pos = 16 % NEO_PATHWAYS;
    cmq.pw_neg = 2;
    cmq.has_neg = 0;
    cmq.top_k = 32;
    cmq.min_curr = 2;
    s.d.check(s.d.upload(d_cmq.ptr, &cmq, sizeof(cmq)), "upload bench compose");
    void *cm_args[]{&d_nw.ptr, &d_cmq.ptr, &d_cmr.ptr};
    s.d.check(s.d.launch(fn_cmp, 1, 1, 1, 256, 1, 1, 0, nullptr, cm_args, nullptr), "warmup neo compose");
    std::vector<double> cmtimes;
    constexpr Word CMBATCH = 256;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec cma");
        for (Word k = 0; k < CMBATCH; ++k)
            s.d.check(s.d.launch(fn_cmp, 1, 1, 1, 256, 1, 1, 0, nullptr, cm_args, nullptr), "bench neo compose");
        s.d.check(s.d.record(b, nullptr), "rec cmb");
        s.d.check(s.d.wait(b), "wait cmb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "cmelapsed");
        cmtimes.push_back(ms / CMBATCH);
    }
    std::sort(cmtimes.begin(), cmtimes.end());
    double cm_us = cmtimes[cmtimes.size() / 2] * 1000.0;
    NeoComposeResult cmres{};
    s.d.check(s.d.download(&cmres, d_cmr.ptr, sizeof(cmres)), "download cmres");

    // Kernel 9: GPU Multi-Cue Context-Gated Associative Retrieval (`mind_neo_multi_cue`) Benchmark!
    Function fn_mc{};
    s.bind(fn_mc, "mind_neo_multi_cue", Image::mind);
    Temporary d_mcq(s.d, sizeof(NeoMultiCueQuery)), d_mcr(s.d, sizeof(NeoMultiCueResult));
    NeoMultiCueQuery mcq{};
    mcq.cues[0] = b_pairs[0].pre; mcq.pathways[0] = 0; mcq.weights[0] = 2;
    mcq.cues[1] = b_pairs[16].pre; mcq.pathways[1] = 16 % NEO_PATHWAYS; mcq.weights[1] = 2;
    mcq.n_cues = 2;
    mcq.inhib_pw = ~0u;
    mcq.top_k = 32;
    mcq.min_curr = 4;
    s.d.check(s.d.upload(d_mcq.ptr, &mcq, sizeof(mcq)), "upload bench multicue");
    void *mc_args[]{&d_nw.ptr, &d_mcq.ptr, &d_mcr.ptr};
    s.d.check(s.d.launch(fn_mc, 1, 1, 1, 256, 1, 1, 0, nullptr, mc_args, nullptr), "warmup neo multicue");
    std::vector<double> mctimes;
    constexpr Word MCBATCH = 256;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec mca");
        for (Word k = 0; k < MCBATCH; ++k)
            s.d.check(s.d.launch(fn_mc, 1, 1, 1, 256, 1, 1, 0, nullptr, mc_args, nullptr), "bench neo multicue");
        s.d.check(s.d.record(b, nullptr), "rec mcb");
        s.d.check(s.d.wait(b), "wait mcb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "mcelapsed");
        mctimes.push_back(ms / MCBATCH);
    }
    std::sort(mctimes.begin(), mctimes.end());
    double mc_us = mctimes[mctimes.size() / 2] * 1000.0;
    NeoMultiCueResult mcres{};
    s.d.check(s.d.download(&mcres, d_mcr.ptr, sizeof(mcres)), "download mcres");

    // Kernel 10: GPU Multi-Clue Bayesian Hypothesis Ranking (`mind_neo_hypothesize`) Benchmark!
    Function fn_hyp{};
    s.bind(fn_hyp, "mind_neo_hypothesize", Image::mind);
    Temporary d_hyq(s.d, sizeof(NeoHypothesizeQuery)), d_hyr(s.d, sizeof(NeoHypothesizeResult));
    NeoHypothesizeQuery hyq{};
    hyq.clues[0] = b_pairs[0].pre;
    hyq.clues[1] = b_pairs[112].pre;
    hyq.n_clues = 2;
    hyq.cands[0] = b_pairs[28].post;
    hyq.cands[1] = b_pairs[0].post;
    hyq.cands[2] = b_pairs[56].post;
    hyq.cands[3] = b_pairs[84].post;
    hyq.n_cands = 4;
    hyq.pathway = 0;
    s.d.check(s.d.upload(d_hyq.ptr, &hyq, sizeof(hyq)), "upload bench hypothesize");
    void *hy_args[]{&d_nw.ptr, &d_hyq.ptr, &d_hyr.ptr};
    s.d.check(s.d.launch(fn_hyp, 1, 1, 1, 256, 1, 1, 0, nullptr, hy_args, nullptr), "warmup neo hypothesize");
    std::vector<double> hytimes;
    constexpr Word HYBATCH = 256;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec hya");
        for (Word k = 0; k < HYBATCH; ++k)
            s.d.check(s.d.launch(fn_hyp, 1, 1, 1, 256, 1, 1, 0, nullptr, hy_args, nullptr), "bench neo hypothesize");
        s.d.check(s.d.record(b, nullptr), "rec hyb");
        s.d.check(s.d.wait(b), "wait hyb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "hyelapsed");
        hytimes.push_back(ms / HYBATCH);
    }
    std::sort(hytimes.begin(), hytimes.end());
    double hy_us = hytimes[hytimes.size() / 2] * 1000.0;
    NeoHypothesizeResult hyres{};
    s.d.check(s.d.download(&hyres, d_hyr.ptr, sizeof(hyres)), "download hyres");

    // Kernel 11: GPU Bidirectional Forward-Backward Bridge Inference (`mind_neo_bridge`) Benchmark!
    Function fn_br{};
    s.bind(fn_br, "mind_neo_bridge", Image::mind);
    Temporary d_brq(s.d, sizeof(NeoBridgeQuery)), d_brr(s.d, sizeof(NeoBridgeResult));
    NeoBridgeQuery brq{};
    brq.cue_a = b_pairs[4].pre;
    brq.cue_b = b_pairs[127].post;
    brq.pathways[0] = 4;
    brq.pathways[1] = 8;
    brq.pathways[2] = 16;
    brq.n_pws = 3;
    brq.top_k = 32;
    brq.min_curr = 2;
    s.d.check(s.d.upload(d_brq.ptr, &brq, sizeof(brq)), "upload bench bridge");
    void *br_args[]{&d_nw.ptr, &d_brq.ptr, &d_brr.ptr};
    s.d.check(s.d.launch(fn_br, 1, 1, 1, 256, 1, 1, 0, nullptr, br_args, nullptr), "warmup neo bridge");
    std::vector<double> brtimes;
    constexpr Word BRBATCH = 256;
    for (Word r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec bra");
        for (Word k = 0; k < BRBATCH; ++k)
            s.d.check(s.d.launch(fn_br, 1, 1, 1, 256, 1, 1, 0, nullptr, br_args, nullptr), "bench neo bridge");
        s.d.check(s.d.record(b, nullptr), "rec brb");
        s.d.check(s.d.wait(b), "wait brb");
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "brelapsed");
        brtimes.push_back(ms / BRBATCH);
    }
    std::sort(brtimes.begin(), brtimes.end());
    double br_us = brtimes[brtimes.size() / 2] * 1000.0;
    NeoBridgeResult brres{};
    s.d.check(s.d.download(&brres, d_brr.ptr, sizeof(brres)), "download brres");

    // Kernel 12: GPU Unsupervised Spiking Prototype Clustering & Odd-One-Out (`mind_neo_cluster`) Benchmark!
    // Add 3 clean clustered items on PW1 + PW8: items 0 & 1 share prototype post_1, item 2 maps to disjoint post_9!
    {
        std::vector<NeoPair> cl_pairs(6);
        SpikeVec proto_a = cochlear_encode("cluster_proto_a", 711u);
        SpikeVec proto_b = cochlear_encode("cluster_proto_b", 722u);
        SpikeVec item0 = cochlear_encode("cl_item_0", 731u);
        SpikeVec item1 = cochlear_encode("cl_item_1", 732u);
        SpikeVec item2 = cochlear_encode("cl_item_2", 733u);
        cl_pairs[0] = {item0, proto_a, 1, 0, 4};
        cl_pairs[1] = {item0, proto_a, 8, 0, 4};
        cl_pairs[2] = {item1, proto_a, 1, 0, 4};
        cl_pairs[3] = {item1, proto_a, 8, 0, 4};
        cl_pairs[4] = {item2, proto_b, 1, 0, 4};
        cl_pairs[5] = {item2, proto_b, 8, 0, 4};
        Word cl_cnt = 6;
        s.d.check(s.d.upload(d_np.ptr, cl_pairs.data(), cl_pairs.size() * sizeof(NeoPair)), "upload cluster pairs");
        void *cl_nr_args[]{&d_nw.ptr, &d_np.ptr, &cl_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
        s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, cl_nr_args, nullptr), "ripple cluster pairs");

        Function fn_cl{};
        s.bind(fn_cl, "mind_neo_cluster", Image::mind);
        Temporary d_clq(s.d, sizeof(NeoClusterQuery)), d_clr(s.d, sizeof(NeoClusterResult));
        NeoClusterQuery clq{};
        clq.items[0] = item0;
        clq.items[1] = item1;
        clq.items[2] = item2;
        clq.n_items = 3;
        clq.pw_sem = 1;
        clq.pw_tax = 8;
        clq.min_curr = 2;
        s.d.check(s.d.upload(d_clq.ptr, &clq, sizeof(clq)), "upload bench cluster");
        void *cl_args[]{&d_nw.ptr, &d_clq.ptr, &d_clr.ptr};
        s.d.check(s.d.launch(fn_cl, 1, 1, 1, 256, 1, 1, 0, nullptr, cl_args, nullptr), "warmup neo cluster");
        std::vector<double> cltimes;
        constexpr Word CLBATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec cla");
            for (Word k = 0; k < CLBATCH; ++k)
                s.d.check(s.d.launch(fn_cl, 1, 1, 1, 256, 1, 1, 0, nullptr, cl_args, nullptr), "bench neo cluster");
            s.d.check(s.d.record(b, nullptr), "rec clb");
            s.d.check(s.d.wait(b), "wait clb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "clelapsed");
            cltimes.push_back(ms / CLBATCH);
        }
        std::sort(cltimes.begin(), cltimes.end());
        double cl_us = cltimes[cltimes.size() / 2] * 1000.0;
        NeoClusterResult clres{};
        s.d.check(s.d.download(&clres, d_clr.ptr, sizeof(clres)), "download clres");

        // Kernel 13: GPU Episodic-vs-Semantic Cortical Conflict & Novelty Gate (`mind_neo_gate`) Benchmark!
        // Train an inhibitory exception on PW2 (`item0 -| proto_a`, negated=1) so Kernel 13 detects `status=2` (EXCEPTION)!
        std::vector<NeoPair> gt_pairs(1);
        gt_pairs[0] = {item0, proto_a, 2, 1, 8};
        Word gt_cnt = 1;
        s.d.check(s.d.upload(d_np.ptr, gt_pairs.data(), sizeof(NeoPair)), "upload gate pair");
        void *gt_nr_args[]{&d_nw.ptr, &d_np.ptr, &gt_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
        s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, gt_nr_args, nullptr), "ripple gate pair");

        Function fn_gt{};
        s.bind(fn_gt, "mind_neo_gate", Image::mind);
        Temporary d_gtq(s.d, sizeof(NeoGateQuery)), d_gtr(s.d, sizeof(NeoGateResult));
        NeoGateQuery gtq{};
        gtq.subj_sdr = item0;
        gtq.pred_sdr = proto_a;
        gtq.pw_pos = 1;
        gtq.pw_neg = 2;
        gtq.pw_tax = 8;
        gtq.support_thresh = 200;
        gtq.veto_thresh = 200;
        s.d.check(s.d.upload(d_gtq.ptr, &gtq, sizeof(gtq)), "upload bench gate");
        void *gt_args[]{&d_nw.ptr, &d_gtq.ptr, &d_gtr.ptr};
        s.d.check(s.d.launch(fn_gt, 1, 1, 1, 256, 1, 1, 0, nullptr, gt_args, nullptr), "warmup neo gate");
        std::vector<double> gttimes;
        constexpr Word GTBATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec gta");
            for (Word k = 0; k < GTBATCH; ++k)
                s.d.check(s.d.launch(fn_gt, 1, 1, 1, 256, 1, 1, 0, nullptr, gt_args, nullptr), "bench neo gate");
            s.d.check(s.d.record(b, nullptr), "rec gtb");
            s.d.check(s.d.wait(b), "wait gtb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "gtelapsed");
            gttimes.push_back(ms / GTBATCH);
        }
        std::sort(gttimes.begin(), gttimes.end());
        double gt_us = gttimes[gttimes.size() / 2] * 1000.0;
        NeoGateResult gtres{};
        s.d.check(s.d.download(&gtres, d_gtr.ptr, sizeof(gtres)), "download gtres");

        // Kernel 14: GPU Offline SWR Dream Replay & 2-Hop Transitive Consolidation (`mind_neo_dream`) Benchmark!
        // Train 2-hop chain `item0 -> item1 -> item2` on PW4, then dream `item0` -> discovers `item1 -> item2` & consolidates shortcut to PW17!
        std::vector<NeoPair> dr_pairs(2);
        dr_pairs[0] = {item0, item1, 4, 0, 8};
        dr_pairs[1] = {item1, item2, 4, 0, 8};
        Word dr_cnt = 2;
        s.d.check(s.d.upload(d_np.ptr, dr_pairs.data(), dr_pairs.size() * sizeof(NeoPair)), "upload dream pairs");
        void *dr_nr_args[]{&d_nw.ptr, &d_np.ptr, &dr_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
        s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, dr_nr_args, nullptr), "ripple dream pairs");

        Function fn_dr{};
        s.bind(fn_dr, "mind_neo_dream", Image::mind);
        Temporary d_drq(s.d, sizeof(NeoDreamQuery)), d_drr(s.d, sizeof(NeoDreamResult));
        NeoDreamQuery drq{};
        drq.seed_sdr = item0;
        drq.pw_hop1 = 4;
        drq.pw_hop2 = 4;
        drq.consolidate_pw = 17;
        drq.top_k = 32;
        drq.min_curr = 4;
        s.d.check(s.d.upload(d_drq.ptr, &drq, sizeof(drq)), "upload bench dream");
        void *dr_args[]{&d_nw.ptr, &d_drq.ptr, &d_g.ptr, &d_drr.ptr};
        s.d.check(s.d.launch(fn_dr, 1, 1, 1, 256, 1, 1, 0, nullptr, dr_args, nullptr), "warmup neo dream");
        NeoDreamResult drres{};
        s.d.check(s.d.download(&drres, d_drr.ptr, sizeof(drres)), "download drres warmup");
        Word dr_first_edits = drres.dream_edits;
        std::vector<double> drtimes;
        constexpr Word DRBATCH = 128;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec dra");
            for (Word k = 0; k < DRBATCH; ++k)
                s.d.check(s.d.launch(fn_dr, 1, 1, 1, 256, 1, 1, 0, nullptr, dr_args, nullptr), "bench neo dream");
            s.d.check(s.d.record(b, nullptr), "rec drb");
            s.d.check(s.d.wait(b), "wait drb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "drelapsed");
            drtimes.push_back(ms / DRBATCH);
        }
        std::sort(drtimes.begin(), drtimes.end());
        double dr_us = drtimes[drtimes.size() / 2] * 1000.0;

        // Kernel 15: GPU Universal Quantifier & Counterexample Scanner (`mind_neo_scan`) Benchmark!
        // Scans {item0 (vetoed on PW2), item1 (supported on PW1), item2 (absent)} against `proto_a` -> `all_hold=0, some_hold=1, counterex=0, witness=1`!
        Function fn_sc{};
        s.bind(fn_sc, "mind_neo_scan", Image::mind);
        Temporary d_scq(s.d, sizeof(NeoScanQuery)), d_scr(s.d, sizeof(NeoScanResult));
        NeoScanQuery scq{};
        scq.members[0] = item0;
        scq.members[1] = item1;
        scq.members[2] = item2;
        scq.pred_sdr = proto_a;
        scq.n_members = 3;
        scq.pw_pos = 1;
        scq.pw_neg = 2;
        s.d.check(s.d.upload(d_scq.ptr, &scq, sizeof(scq)), "upload bench scan");
        void *sc_args[]{&d_nw.ptr, &d_scq.ptr, &d_scr.ptr};
        s.d.check(s.d.launch(fn_sc, 1, 1, 1, 256, 1, 1, 0, nullptr, sc_args, nullptr), "warmup neo scan");
        std::vector<double> sctimes;
        constexpr Word SCBATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec sca");
            for (Word k = 0; k < SCBATCH; ++k)
                s.d.check(s.d.launch(fn_sc, 1, 1, 1, 256, 1, 1, 0, nullptr, sc_args, nullptr), "bench neo scan");
            s.d.check(s.d.record(b, nullptr), "rec scb");
            s.d.check(s.d.wait(b), "wait scb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "scelapsed");
            sctimes.push_back(ms / SCBATCH);
        }
        std::sort(sctimes.begin(), sctimes.end());
        double sc_us = sctimes[sctimes.size() / 2] * 1000.0;
        NeoScanResult scres{};
        s.d.check(s.d.download(&scres, d_scr.ptr, sizeof(scres)), "download scres");

        // Kernel 16: GPU Multi-Step Forward Deductive Rule-Chaining (`mind_neo_deduce`) Benchmark!
        // Extend PW4 chain `item0 -> item1 -> item2 -> proto_a` (3 hops) and prove `item0 => proto_a` directly on L2!
        std::vector<NeoPair> dd_pairs(1);
        dd_pairs[0] = {item2, proto_a, 4, 0, 8};
        Word dd_cnt = 1;
        s.d.check(s.d.upload(d_np.ptr, dd_pairs.data(), dd_pairs.size() * sizeof(NeoPair)), "upload deduce pair");
        void *dd_nr_args[]{&d_nw.ptr, &d_np.ptr, &dd_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
        s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, dd_nr_args, nullptr), "ripple deduce pair");

        Function fn_dd{};
        s.bind(fn_dd, "mind_neo_deduce", Image::mind);
        Temporary d_ddq(s.d, sizeof(NeoDeduceQuery)), d_ddr(s.d, sizeof(NeoDeduceResult));
        NeoDeduceQuery ddq{};
        ddq.start_sdr = item0;
        ddq.goal_sdr = proto_a;
        ddq.pathway = 4;
        ddq.max_hops = 4;
        ddq.top_k = 32;
        ddq.min_curr = 4;
        ddq.has_blocker = 0;
        s.d.check(s.d.upload(d_ddq.ptr, &ddq, sizeof(ddq)), "upload bench deduce");
        void *dd_args[]{&d_nw.ptr, &d_ddq.ptr, &d_ddr.ptr};
        s.d.check(s.d.launch(fn_dd, 1, 1, 1, 256, 1, 1, 0, nullptr, dd_args, nullptr), "warmup neo deduce");
        std::vector<double> ddtimes;
        constexpr Word DDBATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec dda");
            for (Word k = 0; k < DDBATCH; ++k)
                s.d.check(s.d.launch(fn_dd, 1, 1, 1, 256, 1, 1, 0, nullptr, dd_args, nullptr), "bench neo deduce");
            s.d.check(s.d.record(b, nullptr), "rec ddb");
            s.d.check(s.d.wait(b), "wait ddb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "ddelapsed");
            ddtimes.push_back(ms / DDBATCH);
        }
        std::sort(ddtimes.begin(), ddtimes.end());
        double dd_us = ddtimes[ddtimes.size() / 2] * 1000.0;
        NeoDeduceResult ddres{};
        s.d.check(s.d.download(&ddres, d_ddr.ptr, sizeof(ddres)), "download ddres");

        // Kernel 17: GPU Top-Down Apical Backward Abduction (`mind_neo_abduce_back`) Benchmark!
        // Reverse-abduces PW4 chain `proto_a <- item2 <- item1` directly via column-wise W^T transpose on L2!
        Function fn_ab{};
        s.bind(fn_ab, "mind_neo_abduce_back", Image::mind);
        Temporary d_abq(s.d, sizeof(NeoAbduceBackQuery)), d_abr(s.d, sizeof(NeoAbduceBackResult));
        NeoAbduceBackQuery abq{};
        abq.effect_sdr = proto_a;
        abq.pathway = 4;
        abq.top_k = 32;
        abq.min_curr = 4;
        s.d.check(s.d.upload(d_abq.ptr, &abq, sizeof(abq)), "upload bench abduce");
        void *ab_args[]{&d_nw.ptr, &d_abq.ptr, &d_abr.ptr};
        s.d.check(s.d.launch(fn_ab, 1, 1, 1, 256, 1, 1, 0, nullptr, ab_args, nullptr), "warmup neo abduce");
        std::vector<double> abtimes;
        constexpr Word ABBATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec aba");
            for (Word k = 0; k < ABBATCH; ++k)
                s.d.check(s.d.launch(fn_ab, 1, 1, 1, 256, 1, 1, 0, nullptr, ab_args, nullptr), "bench neo abduce");
            s.d.check(s.d.record(b, nullptr), "rec abb");
            s.d.check(s.d.wait(b), "wait abb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "abelapsed");
            abtimes.push_back(ms / ABBATCH);
        }
        std::sort(abtimes.begin(), abtimes.end());
        double ab_us = abtimes[abtimes.size() / 2] * 1000.0;
        NeoAbduceBackResult abres{};
        s.d.check(s.d.download(&abres, d_abr.ptr, sizeof(abres)), "download abres");

        // Kernel 18: GPU Surgical Counterfactual Imagination & Prototype Shift (`mind_neo_counterfact`) Benchmark!
        // Train PW0 prototype basins (`proto_a -> item1`, `proto_b -> item2`), then morph `item0` losing `proto_a` gaining `proto_b`!
        std::vector<NeoPair> cf_pairs(2);
        cf_pairs[0] = {proto_a, item1, 0, 0, 8};
        cf_pairs[1] = {proto_b, item2, 0, 0, 8};
        Word cf_cnt = 2;
        s.d.check(s.d.upload(d_np.ptr, cf_pairs.data(), cf_pairs.size() * sizeof(NeoPair)), "upload cf pairs");
        void *cf_nr_args[]{&d_nw.ptr, &d_np.ptr, &cf_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
        s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, cf_nr_args, nullptr), "ripple cf pairs");

        Function fn_cf{};
        s.bind(fn_cf, "mind_neo_counterfact", Image::mind);
        Temporary d_cfq(s.d, sizeof(NeoCounterfactQuery)), d_cfr(s.d, sizeof(NeoCounterfactResult));
        NeoCounterfactQuery cfq{};
        cfq.subj_sdr = item0;
        cfq.rem_sdr = proto_a;
        cfq.add_sdr = proto_b;
        cfq.pw_tax = 8; cfq.pw_feat = 1; cfq.pw_neg = 2; cfq.pw_proto = 0;
        cfq.has_rem = 1; cfq.has_add = 1; cfq.top_k = 32; cfq.min_curr = 4;
        s.d.check(s.d.upload(d_cfq.ptr, &cfq, sizeof(cfq)), "upload bench cf");
        void *cf_args[]{&d_nw.ptr, &d_cfq.ptr, &d_cfr.ptr};
        s.d.check(s.d.launch(fn_cf, 1, 1, 1, 256, 1, 1, 0, nullptr, cf_args, nullptr), "warmup neo cf");
        std::vector<double> cftimes;
        constexpr Word CFBATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec cfa");
            for (Word k = 0; k < CFBATCH; ++k)
                s.d.check(s.d.launch(fn_cf, 1, 1, 1, 256, 1, 1, 0, nullptr, cf_args, nullptr), "bench neo cf");
            s.d.check(s.d.record(b, nullptr), "rec cfb");
            s.d.check(s.d.wait(b), "wait cfb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "cfelapsed");
            cftimes.push_back(ms / CFBATCH);
        }
        std::sort(cftimes.begin(), cftimes.end());
        double cf_us = cftimes[cftimes.size() / 2] * 1000.0;
        NeoCounterfactResult cfres{};
        s.d.check(s.d.download(&cfres, d_cfr.ptr, sizeof(cfres)), "download cfres");

        // Kernel 19: GPU Hierarchical 2-Hop Goal-to-Subgoal Plan Chaining (`mind_neo_plan_chain`) Benchmark!
        // Train 2-hop inverse causal chain on PW23 (`item0 -> item1 -> item2`), then unroll from `item0`!
        std::vector<NeoPair> pl_pairs(2);
        pl_pairs[0] = {item0, item1, 23, 0, 8};
        pl_pairs[1] = {item1, item2, 23, 0, 8};
        Word pl_cnt = 2;
        s.d.check(s.d.upload(d_np.ptr, pl_pairs.data(), pl_pairs.size() * sizeof(NeoPair)), "upload plan pairs");
        void *pl_nr_args[]{&d_nw.ptr, &d_np.ptr, &pl_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
        s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, pl_nr_args, nullptr), "ripple plan pairs");

        Function fn_pl{};
        s.bind(fn_pl, "mind_neo_plan_chain", Image::mind);
        Temporary d_plq(s.d, sizeof(NeoPlanChainQuery)), d_plr(s.d, sizeof(NeoPlanChainResult));
        NeoPlanChainQuery plq{};
        plq.goal_sdr = item0;
        plq.pw_inv = 23;
        plq.top_k = 32;
        plq.min_curr = 4;
        s.d.check(s.d.upload(d_plq.ptr, &plq, sizeof(plq)), "upload bench plan");
        void *pl_args[]{&d_nw.ptr, &d_plq.ptr, &d_plr.ptr};
        s.d.check(s.d.launch(fn_pl, 1, 1, 1, 256, 1, 1, 0, nullptr, pl_args, nullptr), "warmup neo plan");
        std::vector<double> pltimes;
        constexpr Word PLBATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec pla");
            for (Word k = 0; k < PLBATCH; ++k)
                s.d.check(s.d.launch(fn_pl, 1, 1, 1, 256, 1, 1, 0, nullptr, pl_args, nullptr), "bench neo plan");
            s.d.check(s.d.record(b, nullptr), "rec plb");
            s.d.check(s.d.wait(b), "wait plb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "plelapsed");
            pltimes.push_back(ms / PLBATCH);
        }
        std::sort(pltimes.begin(), pltimes.end());
        double pl_us = pltimes[pltimes.size() / 2] * 1000.0;
        NeoPlanChainResult plres{};
        s.d.check(s.d.download(&plres, d_plr.ptr, sizeof(plres)), "download plres");

        // Kernel 20: Blackwell GPU NMDA Compositional Action-Conditioned Causal Simulation (`mind_neo_simulate`) Benchmark!
        // Train 2-step action-conditioned transitions on PW7 (`item0 + act1 -> item1`, `item1 + act2 -> item2`), then roll out!
        SpikeVec act1 = cochlear_encode("sim_act_1", 9201u);
        SpikeVec act2 = cochlear_encode("sim_act_2", 9202u);
        std::vector<NeoPair> sm_pairs(2);
        sm_pairs[0] = {sdr_or(item0, act1), item1, 7, 0, 8};
        sm_pairs[1] = {sdr_or(item1, act2), item2, 7, 0, 8};
        Word sm_cnt = 2;
        s.d.check(s.d.upload(d_np.ptr, sm_pairs.data(), sm_pairs.size() * sizeof(NeoPair)), "upload sim pairs");
        void *sm_nr_args[]{&d_nw.ptr, &d_np.ptr, &sm_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
        s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, sm_nr_args, nullptr), "ripple sim pairs");

        Function fn_sm{};
        s.bind(fn_sm, "mind_neo_simulate", Image::mind);
        Temporary d_smq(s.d, sizeof(NeoSimulateQuery)), d_smr(s.d, sizeof(NeoSimulateResult));
        NeoSimulateQuery smq{};
        smq.init_sdr = item0;
        smq.action1_sdr = act1;
        smq.action2_sdr = act2;
        smq.n_actions = 2;
        smq.pw_act = 7;
        smq.top_k = 32;
        smq.min_curr = 4;
        s.d.check(s.d.upload(d_smq.ptr, &smq, sizeof(smq)), "upload bench sim");
        void *sm_args[]{&d_nw.ptr, &d_smq.ptr, &d_smr.ptr};
        s.d.check(s.d.launch(fn_sm, 1, 1, 1, 256, 1, 1, 0, nullptr, sm_args, nullptr), "warmup neo sim");
        std::vector<double> smtimes;
        constexpr Word SMBATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec sma");
            for (Word k = 0; k < SMBATCH; ++k)
                s.d.check(s.d.launch(fn_sm, 1, 1, 1, 256, 1, 1, 0, nullptr, sm_args, nullptr), "bench neo sim");
            s.d.check(s.d.record(b, nullptr), "rec smb");
            s.d.check(s.d.wait(b), "wait smb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "smelapsed");
            smtimes.push_back(ms / SMBATCH);
        }
        std::sort(smtimes.begin(), smtimes.end());
        double sm_us = smtimes[smtimes.size() / 2] * 1000.0;
        NeoSimulateResult smres{};
        s.d.check(s.d.download(&smres, d_smr.ptr, sizeof(smres)), "download smres");

        // Kernel 21: Blackwell GPU Multi-Attribute Spiking Constraint Satisfaction (`mind_neo_constrain`) Benchmark!
        // Train PW17 (`item0, item1 -> cat0`), PW1 (`item0 -> pos0+exc0`, `item1 -> pos0+exc0`), PW2 (`item0 -| exc0`, negated=1)!
        SpikeVec cn_cat = cochlear_encode("cn_cat_0", 9301u);
        SpikeVec cn_pos = cochlear_encode("cn_pos_0", 9302u);
        SpikeVec cn_exc = cochlear_encode("cn_exc_0", 9303u);
        std::vector<NeoPair> cn_pairs(7);
        cn_pairs[0] = {item0, cn_cat, 17, 0, 8};
        cn_pairs[1] = {item1, cn_cat, 17, 0, 8};
        cn_pairs[2] = {item0, cn_pos, 1, 0, 8};
        cn_pairs[3] = {item1, cn_pos, 1, 0, 8};
        cn_pairs[4] = {item0, cn_exc, 1, 0, 8};
        cn_pairs[5] = {item1, cn_exc, 1, 0, 8};
        cn_pairs[6] = {item0, cn_exc, 2, 1, 8};
        Word cn_cnt = 7;
        s.d.check(s.d.upload(d_np.ptr, cn_pairs.data(), cn_pairs.size() * sizeof(NeoPair)), "upload cn pairs");
        void *cn_nr_args[]{&d_nw.ptr, &d_np.ptr, &cn_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
        s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, cn_nr_args, nullptr), "ripple cn pairs");

        Function fn_cn{};
        s.bind(fn_cn, "mind_neo_constrain", Image::mind);
        Temporary d_cnq(s.d, sizeof(NeoConstrainQuery)), d_cnr(s.d, sizeof(NeoConstrainResult));
        NeoConstrainQuery cnq{};
        cnq.n_cands = 3;
        cnq.cands[0] = item0;
        cnq.cands[1] = item1;
        cnq.cands[2] = item2;
        cnq.req_cat = cn_cat; cnq.has_cat = 1;
        cnq.req_pos = cn_pos; cnq.has_pos = 1;
        cnq.excl_sdr = cn_exc; cnq.has_excl = 1;
        cnq.pw_tax = 17; cnq.pw_pos = 1; cnq.pw_neg = 2;
        s.d.check(s.d.upload(d_cnq.ptr, &cnq, sizeof(cnq)), "upload bench constrain");
        void *cn_args[]{&d_nw.ptr, &d_cnq.ptr, &d_cnr.ptr};
        s.d.check(s.d.launch(fn_cn, 1, 1, 1, 256, 1, 1, 0, nullptr, cn_args, nullptr), "warmup neo constrain");
        std::vector<double> cntimes;
        constexpr Word CNBATCH = 512;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec cna");
            for (Word k = 0; k < CNBATCH; ++k)
                s.d.check(s.d.launch(fn_cn, 1, 1, 1, 256, 1, 1, 0, nullptr, cn_args, nullptr), "bench neo constrain");
            s.d.check(s.d.record(b, nullptr), "rec cnb");
            s.d.check(s.d.wait(b), "wait cnb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "cnelapsed");
            cntimes.push_back(ms / CNBATCH);
        }
        std::sort(cntimes.begin(), cntimes.end());
        double cn_us = cntimes[cntimes.size() / 2] * 1000.0;
        NeoConstrainResult cnres{};
        s.d.check(s.d.download(&cnres, d_cnr.ptr, sizeof(cnres)), "download cnres");

        // Kernel 22: Blackwell GPU Few-Shot Relational Schema Induction & 1/2-Hop Transfer (`mind_neo_induce`) Benchmark!
        // Train PW18 with 2-hop chain (`item0 -> item1 -> item2`) plus 2nd example (`proto_a -> proto_b`); induce PW18 from 2 pairs & roll out 2 hops from `item0`!
        std::vector<NeoPair> in_pairs(3);
        in_pairs[0] = {item0, item1, 18, 0, 8};
        in_pairs[1] = {item1, item2, 18, 0, 8};
        in_pairs[2] = {proto_a, proto_b, 18, 0, 8};
        Word in_cnt = 3;
        s.d.check(s.d.upload(d_np.ptr, in_pairs.data(), in_pairs.size() * sizeof(NeoPair)), "upload in pairs");
        void *in_nr_args[]{&d_nw.ptr, &d_np.ptr, &in_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
        s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, in_nr_args, nullptr), "ripple in pairs");

        Function fn_in{};
        s.bind(fn_in, "mind_neo_induce", Image::mind);
        Temporary d_inq(s.d, sizeof(NeoInduceQuery)), d_inr(s.d, sizeof(NeoInduceResult));
        NeoInduceQuery inq{};
        inq.ex1_in = item0; inq.ex1_out = item1;
        inq.ex2_in = proto_a; inq.ex2_out = proto_b;
        inq.query_in = item0;
        inq.cand_pws[0] = 4; inq.cand_pws[1] = 8; inq.cand_pws[2] = 16;
        inq.cand_pws[3] = 18; inq.cand_pws[4] = 23; inq.cand_pws[5] = 14;
        inq.n_pws = 6; inq.n_hops = 2; inq.top_k = 32; inq.min_curr = 4;
        s.d.check(s.d.upload(d_inq.ptr, &inq, sizeof(inq)), "upload bench induce");
        void *in_args[]{&d_nw.ptr, &d_inq.ptr, &d_inr.ptr};
        s.d.check(s.d.launch(fn_in, 1, 1, 1, 256, 1, 1, 0, nullptr, in_args, nullptr), "warmup neo induce");
        std::vector<double> intimes;
        constexpr Word INBATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec ina");
            for (Word k = 0; k < INBATCH; ++k)
                s.d.check(s.d.launch(fn_in, 1, 1, 1, 256, 1, 1, 0, nullptr, in_args, nullptr), "bench neo induce");
            s.d.check(s.d.record(b, nullptr), "rec inb");
            s.d.check(s.d.wait(b), "wait inb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "inelapsed");
            intimes.push_back(ms / INBATCH);
        }
        std::sort(intimes.begin(), intimes.end());
        double in_us = intimes[intimes.size() / 2] * 1000.0;
        NeoInduceResult inres{};
        s.d.check(s.d.download(&inres, d_inr.ptr, sizeof(inres)), "download inres");

        // Kernel 23: Blackwell GPU Compositional Multi-Pathway Rule Algebra & Discovery (`mind_neo_chain_rule`) Benchmark!
        // Train cross-pathway link `proto_b -> item2` on PW23 (while `proto_a -> proto_b` lives ONLY on PW18), then run Mode 1
        // to autonomously search 36 pathway pairs and discover `PW18 -> PW23` bridging `proto_a -> item2` via `proto_b`!
        std::vector<NeoPair> cr_pairs(1);
        cr_pairs[0] = {proto_b, item2, 23, 0, 8};
        Word cr_cnt = 1;
        s.d.check(s.d.upload(d_np.ptr, cr_pairs.data(), cr_pairs.size() * sizeof(NeoPair)), "upload cr pairs");
        void *cr_nr_args[]{&d_nw.ptr, &d_np.ptr, &cr_cnt, &d_g.ptr, &d_ned.ptr, &d_nsp.ptr};
        s.d.check(s.d.launch(fn_nr, 32, 1, 1, 32, 1, 1, 0, nullptr, cr_nr_args, nullptr), "ripple cr pairs");

        Function fn_cr{};
        s.bind(fn_cr, "mind_neo_chain_rule", Image::mind);
        Temporary d_crq(s.d, sizeof(NeoChainRuleQuery)), d_crr(s.d, sizeof(NeoChainRuleResult));
        NeoChainRuleQuery crq{};
        crq.start_sdr = proto_a;
        crq.target_sdr = item2;
        crq.cand_pws[0] = 4; crq.cand_pws[1] = 8; crq.cand_pws[2] = 16;
        crq.cand_pws[3] = 18; crq.cand_pws[4] = 23; crq.cand_pws[5] = 14;
        crq.n_pws = 6; crq.mode = 1; crq.top_k = 32; crq.min_curr = 4;
        s.d.check(s.d.upload(d_crq.ptr, &crq, sizeof(crq)), "upload bench chain_rule");
        void *cr_args[]{&d_nw.ptr, &d_crq.ptr, &d_crr.ptr};
        s.d.check(s.d.launch(fn_cr, 1, 1, 1, 256, 1, 1, 0, nullptr, cr_args, nullptr), "warmup neo chain_rule");
        std::vector<double> crtimes;
        constexpr Word CRBATCH = 128;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec cra");
            for (Word k = 0; k < CRBATCH; ++k)
                s.d.check(s.d.launch(fn_cr, 1, 1, 1, 256, 1, 1, 0, nullptr, cr_args, nullptr), "bench neo chain_rule");
            s.d.check(s.d.record(b, nullptr), "rec crb");
            s.d.check(s.d.wait(b), "wait crb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "crelapsed");
            crtimes.push_back(ms / CRBATCH);
        }
        std::sort(crtimes.begin(), crtimes.end());
        double cr_us = crtimes[crtimes.size() / 2] * 1000.0;
        NeoChainRuleResult crres{};
        s.d.check(s.d.download(&crres, d_crr.ptr, sizeof(crres)), "download crres");

        // Kernel 24 (Stage 4 Unlock!): Blackwell GPU Live 1-Shot Hebbian Online Schema Write & Instant 2-Hop Rollout (`mind_neo_online_schema`) Benchmark!
        Function fn_os{};
        s.bind(fn_os, "mind_neo_online_schema", Image::mind);
        Temporary d_osq(s.d, sizeof(NeoOnlineSchemaQuery)), d_osr(s.d, sizeof(NeoOnlineSchemaResult));
        NeoOnlineSchemaQuery osq{};
        osq.pre_sdr = proto_a;
        osq.post_sdr = proto_b;
        osq.post2_sdr = item2;
        osq.pathway = 19; osq.mode = 2; osq.max_hops = 2; osq.strength = 6; osq.top_k = 32; osq.min_curr = 4;
        s.d.check(s.d.upload(d_osq.ptr, &osq, sizeof(osq)), "upload bench online_schema");
        void *os_args[]{&d_nw.ptr, &d_osq.ptr, &d_g.ptr, &d_osr.ptr};
        s.d.check(s.d.launch(fn_os, 1, 1, 1, 256, 1, 1, 0, nullptr, os_args, nullptr), "warmup neo online_schema");
        NeoOnlineSchemaResult os_first{};
        s.d.check(s.d.download(&os_first, d_osr.ptr, sizeof(os_first)), "download os first");
        std::vector<double> ostimes;
        constexpr Word OSBATCH = 128;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec osa");
            for (Word k = 0; k < OSBATCH; ++k)
                s.d.check(s.d.launch(fn_os, 1, 1, 1, 256, 1, 1, 0, nullptr, os_args, nullptr), "bench neo online_schema");
            s.d.check(s.d.record(b, nullptr), "rec osb");
            s.d.check(s.d.wait(b), "wait osb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "oselapsed");
            ostimes.push_back(ms / OSBATCH);
        }
        std::sort(ostimes.begin(), ostimes.end());
        double os_us = ostimes[ostimes.size() / 2] * 1000.0;
        NeoOnlineSchemaResult osres{};
        s.d.check(s.d.download(&osres, d_osr.ptr, sizeof(osres)), "download osres");

        // Kernel 25 (Stage 4 Continual Learning!): Blackwell GPU Live LTD Unlearning + LTP Schema Revision (`mind_neo_revise_schema`) Benchmark!
        // `proto_a -> proto_b -> item2` is currently wired on PW19 from Kernel 24; now revise `proto_a -> item0 -> item1` live via LTD+LTP!
        Function fn_rs{};
        s.bind(fn_rs, "mind_neo_revise_schema", Image::mind);
        Temporary d_rsq(s.d, sizeof(NeoReviseSchemaQuery)), d_rsr(s.d, sizeof(NeoReviseSchemaResult));
        NeoReviseSchemaQuery rsq{};
        rsq.pre_sdr = proto_a;
        rsq.new_sdr = item0;
        rsq.new2_sdr = item1;
        rsq.pathway = 19; rsq.mode = 1; rsq.has_old = 0; rsq.strength = 6; rsq.top_k = 32; rsq.min_curr = 4;
        s.d.check(s.d.upload(d_rsq.ptr, &rsq, sizeof(rsq)), "upload bench revise_schema");
        void *rs_args[]{&d_nw.ptr, &d_rsq.ptr, &d_g.ptr, &d_rsr.ptr};
        s.d.check(s.d.launch(fn_rs, 1, 1, 1, 256, 1, 1, 0, nullptr, rs_args, nullptr), "warmup neo revise_schema");
        NeoReviseSchemaResult rs_first{};
        s.d.check(s.d.download(&rs_first, d_rsr.ptr, sizeof(rs_first)), "download rs first");
        std::vector<double> rstimes;
        constexpr Word RSBATCH = 128;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec rsa");
            for (Word k = 0; k < RSBATCH; ++k)
                s.d.check(s.d.launch(fn_rs, 1, 1, 1, 256, 1, 1, 0, nullptr, rs_args, nullptr), "bench neo revise_schema");
            s.d.check(s.d.record(b, nullptr), "rec rsb");
            s.d.check(s.d.wait(b), "wait rsb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "rselapsed");
            rstimes.push_back(ms / RSBATCH);
        }
        std::sort(rstimes.begin(), rstimes.end());
        double rs_us = rstimes[rstimes.size() / 2] * 1000.0;
        NeoReviseSchemaResult rsres{};
        s.d.check(s.d.download(&rsres, d_rsr.ptr, sizeof(rsres)), "download rsres");

        // Kernel 26 (Stage 4 -> Stage 5 Crossing!): Blackwell GPU Spiking SDR Circular-Phase Temporal Role-Binder (`mind_neo_role_bind`) Benchmark!
        Function fn_rb{};
        s.bind(fn_rb, "mind_neo_role_bind", Image::mind);
        Temporary d_rbq(s.d, sizeof(NeoRoleBindQuery)), d_rbr(s.d, sizeof(NeoRoleBindResult));
        NeoRoleBindQuery rbq_w{};
        rbq_w.roles[0] = proto_a; rbq_w.roles[1] = proto_b; rbq_w.roles[2] = item0; rbq_w.roles[3] = item1;
        rbq_w.pathway = 25; rbq_w.mode = 0; rbq_w.strength = 6; rbq_w.top_k = 32; rbq_w.min_curr = 4;
        s.d.check(s.d.upload(d_rbq.ptr, &rbq_w, sizeof(rbq_w)), "upload bench role_bind write");
        void *rb_args[]{&d_nw.ptr, &d_rbq.ptr, &d_g.ptr, &d_rbr.ptr};
        s.d.check(s.d.launch(fn_rb, 1, 1, 1, 256, 1, 1, 0, nullptr, rb_args, nullptr), "bench role_bind write");
        NeoRoleBindResult rb_wres{};
        s.d.check(s.d.download(&rb_wres, d_rbr.ptr, sizeof(rb_wres)), "download rb write");
        NeoRoleBindQuery rbq_r{};
        rbq_r.cue_a = proto_a; rbq_r.cue_b = item0;
        rbq_r.pathway = 25; rbq_r.mode = 1; rbq_r.query_role = 3; rbq_r.top_k = 32; rbq_r.min_curr = 4;
        s.d.check(s.d.upload(d_rbq.ptr, &rbq_r, sizeof(rbq_r)), "upload bench role_bind read");
        s.d.check(s.d.launch(fn_rb, 1, 1, 1, 256, 1, 1, 0, nullptr, rb_args, nullptr), "warmup neo role_bind");
        std::vector<double> rbtimes;
        constexpr Word RBBATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec rba");
            for (Word k = 0; k < RBBATCH; ++k)
                s.d.check(s.d.launch(fn_rb, 1, 1, 1, 256, 1, 1, 0, nullptr, rb_args, nullptr), "bench neo role_bind");
            s.d.check(s.d.record(b, nullptr), "rec rbb");
            s.d.check(s.d.wait(b), "wait rbb");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "rbelapsed");
            rbtimes.push_back(ms / RBBATCH);
        }
        std::sort(rbtimes.begin(), rbtimes.end());
        double rb_us = rbtimes[rbtimes.size() / 2] * 1000.0;
        NeoRoleBindResult rbres{};
        s.d.check(s.d.download(&rbres, d_rbr.ptr, sizeof(rbres)), "download rbres");

        // Kernel 27 (Stage 5 Crossing!): Blackwell GPU Spiking Predictive-Coding Sequence Role-Tagger (`mind_neo_seq_predict`) Benchmark!
        Function fn_sp27{};
        s.bind(fn_sp27, "mind_neo_seq_predict", Image::mind);
        Temporary d_spq(s.d, sizeof(NeoSeqPredictQuery)), d_spr(s.d, sizeof(NeoSeqPredictResult));
        NeoSeqPredictQuery spq_w = SpikingMind::make_k27_base_query();
        spq_w.mode = 0; spq_w.seq_len = 4; spq_w.strength = 6;
        spq_w.target_roles[0] = 0; spq_w.target_roles[1] = 1; spq_w.target_roles[2] = 2; spq_w.target_roles[3] = 3;
        s.d.check(s.d.upload(d_spq.ptr, &spq_w, sizeof(spq_w)), "upload bench seq_predict write");
        void *sp27_args[]{&d_nw.ptr, &d_spq.ptr, &d_g.ptr, &d_spr.ptr};
        s.d.check(s.d.launch(fn_sp27, 1, 1, 1, 256, 1, 1, 0, nullptr, sp27_args, nullptr), "bench seq_predict write");
        NeoSeqPredictResult sp_wres{};
        s.d.check(s.d.download(&sp_wres, d_spr.ptr, sizeof(sp_wres)), "download sp write");
        NeoSeqPredictQuery spq_r = spq_w;
        spq_r.mode = 1;
        s.d.check(s.d.upload(d_spq.ptr, &spq_r, sizeof(spq_r)), "upload bench seq_predict read");
        s.d.check(s.d.launch(fn_sp27, 1, 1, 1, 256, 1, 1, 0, nullptr, sp27_args, nullptr), "warmup neo seq_predict");
        std::vector<double> sp27times;
        constexpr Word SP27BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec sp27a");
            for (Word k = 0; k < SP27BATCH; ++k)
                s.d.check(s.d.launch(fn_sp27, 1, 1, 1, 256, 1, 1, 0, nullptr, sp27_args, nullptr), "bench neo seq_predict");
            s.d.check(s.d.record(b, nullptr), "rec sp27b");
            s.d.check(s.d.wait(b), "wait sp27b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "sp27elapsed");
            sp27times.push_back(ms / SP27BATCH);
        }
        std::sort(sp27times.begin(), sp27times.end());
        double sp27_us = sp27times[sp27times.size() / 2] * 1000.0;
        NeoSeqPredictResult sp27res{};
        s.d.check(s.d.download(&sp27res, d_spr.ptr, sizeof(sp27res)), "download sp27res");

        // Kernel 28 (Stage 5b!): Blackwell GPU Pure Neural-Native Spiking Parietal Math (`mind_neo_neural_math`) Benchmark!
        Function fn_nm28{};
        s.bind(fn_nm28, "mind_neo_neural_math", Image::mind);
        Temporary d_nmq(s.d, sizeof(NeoNeuralMathQuery)), d_nmr(s.d, sizeof(NeoNeuralMathResult));
        NeoNeuralMathQuery nmq_w{};
        nmq_w.pathway = 15; nmq_w.mode = 0; nmq_w.strength = 8;
        s.d.check(s.d.upload(d_nmq.ptr, &nmq_w, sizeof(nmq_w)), "upload bench neural_math write");
        void *nm28_args[]{&d_nw.ptr, &d_nmq.ptr, &d_g.ptr, &d_nmr.ptr};
        s.d.check(s.d.launch(fn_nm28, 1, 1, 1, 256, 1, 1, 0, nullptr, nm28_args, nullptr), "bench neural_math write");
        NeoNeuralMathResult nm_wres{};
        s.d.check(s.d.download(&nm_wres, d_nmr.ptr, sizeof(nm_wres)), "download nm write");
        NeoNeuralMathQuery nmq_r{};
        nmq_r.pathway = 15; nmq_r.mode = 1; nmq_r.op = 3; nmq_r.a = 3; nmq_r.b = 5; nmq_r.c = 23;
        s.d.check(s.d.upload(d_nmq.ptr, &nmq_r, sizeof(nmq_r)), "upload bench neural_math read");
        s.d.check(s.d.launch(fn_nm28, 1, 1, 1, 256, 1, 1, 0, nullptr, nm28_args, nullptr), "warmup neo neural_math");
        std::vector<double> nm28times;
        constexpr Word NM28BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec nm28a");
            for (Word k = 0; k < NM28BATCH; ++k)
                s.d.check(s.d.launch(fn_nm28, 1, 1, 1, 256, 1, 1, 0, nullptr, nm28_args, nullptr), "bench neo neural_math");
            s.d.check(s.d.record(b, nullptr), "rec nm28b");
            s.d.check(s.d.wait(b), "wait nm28b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "nm28elapsed");
            nm28times.push_back(ms / NM28BATCH);
        }
        std::sort(nm28times.begin(), nm28times.end());
        double nm28_us = nm28times[nm28times.size() / 2] * 1000.0;
        NeoNeuralMathResult nm28res{};
        s.d.check(s.d.download(&nm28res, d_nmr.ptr, sizeof(nm28res)), "download nm28res");

        // Kernel 29 (Stage 5c!): Blackwell GPU Pure-Spiking Axonal-Delay Graph Wavefront (`mind_neo_graph_wave`) Benchmark!
        Function fn_gw29{};
        s.bind(fn_gw29, "mind_neo_graph_wave", Image::mind);
        Temporary d_gwq(s.d, sizeof(NeoGraphWaveQuery)), d_gwr(s.d, sizeof(NeoGraphWaveResult));
        NeoGraphWaveQuery gwq_w{};
        gwq_w.pathway = 11; gwq_w.mode = 0; gwq_w.clear_first = 1; gwq_w.n_edges = 7;
        const Word gwu[7] = {0, 1, 2, 0, 3, 0, 1};
        const Word gwv[7] = {1, 2, 4, 3, 4, 2, 3};
        const Word gww[7] = {2, 3, 2, 6, 5, 8, 3};
        for (Word k = 0; k < 7u; ++k) { gwq_w.edge_u[k] = gwu[k]; gwq_w.edge_v[k] = gwv[k]; gwq_w.edge_w[k] = gww[k]; }
        s.d.check(s.d.upload(d_gwq.ptr, &gwq_w, sizeof(gwq_w)), "upload bench graph_wave write");
        void *gw29_args[]{&d_nw.ptr, &d_gwq.ptr, &d_g.ptr, &d_gwr.ptr};
        s.d.check(s.d.launch(fn_gw29, 1, 1, 1, 256, 1, 1, 0, nullptr, gw29_args, nullptr), "bench graph_wave write");
        NeoGraphWaveResult gw_wres{};
        s.d.check(s.d.download(&gw_wres, d_gwr.ptr, sizeof(gw_wres)), "download gw write");
        NeoGraphWaveQuery gwq_r{};
        gwq_r.pathway = 11; gwq_r.mode = 1; gwq_r.src_node = 0; gwq_r.dst_node = 4; gwq_r.blocked_node = ~0u;
        s.d.check(s.d.upload(d_gwq.ptr, &gwq_r, sizeof(gwq_r)), "upload bench graph_wave read");
        s.d.check(s.d.launch(fn_gw29, 1, 1, 1, 256, 1, 1, 0, nullptr, gw29_args, nullptr), "warmup neo graph_wave");
        std::vector<double> gw29times;
        constexpr Word GW29BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec gw29a");
            for (Word k = 0; k < GW29BATCH; ++k)
                s.d.check(s.d.launch(fn_gw29, 1, 1, 1, 256, 1, 1, 0, nullptr, gw29_args, nullptr), "bench neo graph_wave");
            s.d.check(s.d.record(b, nullptr), "rec gw29b");
            s.d.check(s.d.wait(b), "wait gw29b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "gw29elapsed");
            gw29times.push_back(ms / GW29BATCH);
        }
        std::sort(gw29times.begin(), gw29times.end());
        double gw29_us = gw29times[gw29times.size() / 2] * 1000.0;
        NeoGraphWaveResult gw29res{};
        s.d.check(s.d.download(&gw29res, d_gwr.ptr, sizeof(gw29res)), "download gw29res");

        // Kernel 30 (Stage 5d!): Blackwell GPU Pure-Spiking Boltzmann/Hopfield 3-SAT & Constraint Attractor (`mind_neo_sat_solve`) Benchmark!
        Function fn_sat30{};
        s.bind(fn_sat30, "mind_neo_sat_solve", Image::mind);
        Temporary d_satq(s.d, sizeof(NeoSatSolveQuery)), d_satr(s.d, sizeof(NeoSatSolveResult));
        NeoSatSolveQuery satq_w{};
        satq_w.pathway = 13; satq_w.mode = 0; satq_w.clear_first = 1; satq_w.n_vars = 4; satq_w.clause_start = 0; satq_w.n_clauses = 4;
        for (Word k = 0; k < MAX_SAT_WRITE; ++k) satq_w.lit0[k] = satq_w.lit1[k] = satq_w.lit2[k] = ~0u;
        satq_w.lit0[0] = 1; satq_w.lit1[0] = 3; // (v0 v v1)
        satq_w.lit0[1] = 0; satq_w.lit1[1] = 5; // (~v0 v v2)
        satq_w.lit0[2] = 2; satq_w.lit1[2] = 5; // (~v1 v v2)
        satq_w.lit0[3] = 4; satq_w.lit1[3] = 7; // (~v2 v v3)
        s.d.check(s.d.upload(d_satq.ptr, &satq_w, sizeof(satq_w)), "upload bench sat_solve write");
        void *sat30_args[]{&d_nw.ptr, &d_satq.ptr, &d_g.ptr, &d_satr.ptr};
        s.d.check(s.d.launch(fn_sat30, 1, 1, 1, 256, 1, 1, 0, nullptr, sat30_args, nullptr), "bench sat_solve write");
        NeoSatSolveResult sat_wres{};
        s.d.check(s.d.download(&sat_wres, d_satr.ptr, sizeof(sat_wres)), "download sat write");
        NeoSatSolveQuery satq_r{};
        satq_r.pathway = 13; satq_r.mode = 2; satq_r.n_vars = 4; satq_r.query_var = 3; satq_r.seed = 1337;
        s.d.check(s.d.upload(d_satq.ptr, &satq_r, sizeof(satq_r)), "upload bench sat_solve read");
        s.d.check(s.d.launch(fn_sat30, 1, 1, 1, 256, 1, 1, 0, nullptr, sat30_args, nullptr), "warmup neo sat_solve");
        std::vector<double> sat30times;
        constexpr Word SAT30BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec sat30a");
            for (Word k = 0; k < SAT30BATCH; ++k)
                s.d.check(s.d.launch(fn_sat30, 1, 1, 1, 256, 1, 1, 0, nullptr, sat30_args, nullptr), "bench neo sat_solve");
            s.d.check(s.d.record(b, nullptr), "rec sat30b");
            s.d.check(s.d.wait(b), "wait sat30b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "sat30elapsed");
            sat30times.push_back(ms / SAT30BATCH);
        }
        std::sort(sat30times.begin(), sat30times.end());
        double sat30_us = sat30times[sat30times.size() / 2] * 1000.0;
        NeoSatSolveResult sat30res{};
        s.d.check(s.d.download(&sat30res, d_satr.ptr, sizeof(sat30res)), "download sat30res");

        // Kernel 31 (STAGE 6 CROSSING!): Blackwell GPU Pure-Spiking 16x16 V1/V4 Retinal Vision & Spatial Scene Cortex (`mind_neo_visual_cortex`) Benchmark!
        Function fn_vis31{};
        s.bind(fn_vis31, "mind_neo_visual_cortex", Image::mind);
        Temporary d_visq(s.d, sizeof(NeoVisualCortexQuery)), d_visr(s.d, sizeof(NeoVisualCortexResult));
        void *vis31_args[]{&d_nw.ptr, &d_visq.ptr, &d_g.ptr, &d_visr.ptr};
        Word vis_tot_ed = 0u;
        for (Word p = 0; p < 4u; ++p) {
            NeoVisualCortexQuery visq_w{};
            visq_w.pathway = 24; visq_w.mode = 0; visq_w.clear_first = (p == 0u ? 1u : 0u); visq_w.proto_id = p;
            SpikingMind::fill_retina_pattern(visq_w.pixels, p, 0, 0, false);
            s.d.check(s.d.upload(d_visq.ptr, &visq_w, sizeof(visq_w)), "upload bench visual write");
            s.d.check(s.d.launch(fn_vis31, 1, 1, 1, 256, 1, 1, 0, nullptr, vis31_args, nullptr), "bench visual write");
            NeoVisualCortexResult vw{};
            s.d.check(s.d.download(&vw, d_visr.ptr, sizeof(vw)), "download visual write");
            vis_tot_ed += vw.edits_made;
        }
        NeoVisualCortexQuery visq_r{};
        visq_r.pathway = 24; visq_r.mode = 1;
        SpikingMind::fill_retina_pattern(visq_r.pixels, 3u, 2, -1, true); // shifted + noisy hollow ring (`proto 3`)!
        s.d.check(s.d.upload(d_visq.ptr, &visq_r, sizeof(visq_r)), "upload bench visual read");
        s.d.check(s.d.launch(fn_vis31, 1, 1, 1, 256, 1, 1, 0, nullptr, vis31_args, nullptr), "warmup neo visual_cortex");
        std::vector<double> vis31times;
        constexpr Word VIS31BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec vis31a");
            for (Word k = 0; k < VIS31BATCH; ++k)
                s.d.check(s.d.launch(fn_vis31, 1, 1, 1, 256, 1, 1, 0, nullptr, vis31_args, nullptr), "bench neo visual_cortex");
            s.d.check(s.d.record(b, nullptr), "rec vis31b");
            s.d.check(s.d.wait(b), "wait vis31b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "vis31elapsed");
            vis31times.push_back(ms / VIS31BATCH);
        }
        std::sort(vis31times.begin(), vis31times.end());
        double vis31_us = vis31times[vis31times.size() / 2] * 1000.0;
        NeoVisualCortexResult vis31res{};
        s.d.check(s.d.download(&vis31res, d_visr.ptr, sizeof(vis31res)), "download vis31res");

        // Kernel 32 (STAGE 6b!): Blackwell GPU Pure-Spiking 16x16 Cochlear Audio, Harmonic Chord & Prosody Cortex (`mind_neo_audio_cortex`) Benchmark!
        Function fn_aud32{};
        s.bind(fn_aud32, "mind_neo_audio_cortex", Image::mind);
        Temporary d_audq(s.d, sizeof(NeoAudioCortexQuery)), d_audr(s.d, sizeof(NeoAudioCortexResult));
        void *aud32_args[]{&d_nw.ptr, &d_audq.ptr, &d_g.ptr, &d_audr.ptr};
        Word aud_tot_ed = 0u;
        for (Word p = 0; p < 4u; ++p) {
            NeoAudioCortexQuery audq_w{};
            audq_w.pathway = 27; audq_w.mode = 0; audq_w.clear_first = (p == 0u ? 1u : 0u); audq_w.proto_id = p;
            SpikingMind::fill_cochlea_pattern(audq_w.spectro, p, 0, 0, false);
            s.d.check(s.d.upload(d_audq.ptr, &audq_w, sizeof(audq_w)), "upload bench audio write");
            s.d.check(s.d.launch(fn_aud32, 1, 1, 1, 256, 1, 1, 0, nullptr, aud32_args, nullptr), "bench audio write");
            NeoAudioCortexResult aw{};
            s.d.check(s.d.download(&aw, d_audr.ptr, sizeof(aw)), "download audio write");
            aud_tot_ed += aw.edits_made;
        }
        NeoAudioCortexQuery audq_r{};
        audq_r.pathway = 27; audq_r.mode = 1;
        SpikingMind::fill_cochlea_pattern(audq_r.spectro, 2u, 1, 1, true); // time-shifted + transposed + noisy major triad (`proto 2`)!
        s.d.check(s.d.upload(d_audq.ptr, &audq_r, sizeof(audq_r)), "upload bench audio read");
        s.d.check(s.d.launch(fn_aud32, 1, 1, 1, 256, 1, 1, 0, nullptr, aud32_args, nullptr), "warmup neo audio_cortex");
        std::vector<double> aud32times;
        constexpr Word AUD32BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec aud32a");
            for (Word k = 0; k < AUD32BATCH; ++k)
                s.d.check(s.d.launch(fn_aud32, 1, 1, 1, 256, 1, 1, 0, nullptr, aud32_args, nullptr), "bench neo audio_cortex");
            s.d.check(s.d.record(b, nullptr), "rec aud32b");
            s.d.check(s.d.wait(b), "wait aud32b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "aud32elapsed");
            aud32times.push_back(ms / AUD32BATCH);
        }
        std::sort(aud32times.begin(), aud32times.end());
        double aud32_us = aud32times[aud32times.size() / 2] * 1000.0;
        NeoAudioCortexResult aud32res{};
        s.d.check(s.d.download(&aud32res, d_audr.ptr, sizeof(aud32res)), "download aud32res");

        // Kernel 33 (STAGE 6c!): Blackwell GPU Pure-Spiking Cross-Modal Vision+Audio Binding & Prefrontal Goal Stack (`mind_neo_crossmodal_stack`) Benchmark!
        Function fn_xms33{};
        s.bind(fn_xms33, "mind_neo_crossmodal_stack", Image::mind);
        Temporary d_xmsq(s.d, sizeof(NeoCrossModalStackQuery)), d_xmsr(s.d, sizeof(NeoCrossModalStackResult));
        void *xms33_args[]{&d_nw.ptr, &d_xmsq.ptr, &d_g.ptr, &d_xmsr.ptr};
        Word xms_tot_ed = 0u;
        static constexpr Word xms_pairs[4][2] = {{3u, 2u}, {2u, 3u}, {1u, 0u}, {0u, 1u}};
        for (Word p = 0; p < 4u; ++p) {
            NeoCrossModalStackQuery xq{};
            xq.pathway = 24; xq.mode = 0; xq.clear_first = (p == 0u ? 1u : 0u); xq.xproto_id = p;
            xq.vis_id = xms_pairs[p][0]; xq.aud_id = xms_pairs[p][1];
            s.d.check(s.d.upload(d_xmsq.ptr, &xq, sizeof(xq)), "upload bench xmodal write");
            s.d.check(s.d.launch(fn_xms33, 1, 1, 1, 256, 1, 1, 0, nullptr, xms33_args, nullptr), "bench xmodal write");
            NeoCrossModalStackResult xw{};
            s.d.check(s.d.download(&xw, d_xmsr.ptr, sizeof(xw)), "download xmodal write");
            xms_tot_ed += xw.edits_made;
        }
        // Push 2 goals + pop 1 on GPU prefrontal goal stack!
        NeoCrossModalStackQuery xclr{}; xclr.pathway = 24; xclr.mode = 6;
        s.d.check(s.d.upload(d_xmsq.ptr, &xclr, sizeof(xclr)), "upload bench gstack clear");
        s.d.check(s.d.launch(fn_xms33, 1, 1, 1, 256, 1, 1, 0, nullptr, xms33_args, nullptr), "bench gstack clear");
        for (Word g_id : {0u, 1u, 2u}) {
            NeoCrossModalStackQuery xp{}; xp.pathway = 24; xp.mode = 3; xp.goal_id = g_id;
            s.d.check(s.d.upload(d_xmsq.ptr, &xp, sizeof(xp)), "upload bench gstack push");
            s.d.check(s.d.launch(fn_xms33, 1, 1, 1, 256, 1, 1, 0, nullptr, xms33_args, nullptr), "bench gstack push");
            NeoCrossModalStackResult xw{};
            s.d.check(s.d.download(&xw, d_xmsr.ptr, sizeof(xw)), "download gstack push");
            xms_tot_ed += xw.edits_made;
        }
        NeoCrossModalStackQuery xpop{}; xpop.pathway = 24; xpop.mode = 4;
        s.d.check(s.d.upload(d_xmsq.ptr, &xpop, sizeof(xpop)), "upload bench gstack pop");
        s.d.check(s.d.launch(fn_xms33, 1, 1, 1, 256, 1, 1, 0, nullptr, xms33_args, nullptr), "bench gstack pop");
        NeoCrossModalStackResult xpop_res{};
        s.d.check(s.d.download(&xpop_res, d_xmsr.ptr, sizeof(xpop_res)), "download gstack pop");
        xms_tot_ed += xpop_res.edits_made;
        // Time Mode 2 (Cross-Modal Fusion + Synesthetic Backward W^T Recall: viscross -> xhazard & audtritone)!
        NeoCrossModalStackQuery xq_r{};
        xq_r.pathway = 24; xq_r.mode = 2; xq_r.vis_id = 2u; xq_r.aud_id = ~0u;
        s.d.check(s.d.upload(d_xmsq.ptr, &xq_r, sizeof(xq_r)), "upload bench xmodal read");
        s.d.check(s.d.launch(fn_xms33, 1, 1, 1, 256, 1, 1, 0, nullptr, xms33_args, nullptr), "warmup neo crossmodal_stack");
        std::vector<double> xms33times;
        constexpr Word XMS33BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec xms33a");
            for (Word k = 0; k < XMS33BATCH; ++k)
                s.d.check(s.d.launch(fn_xms33, 1, 1, 1, 256, 1, 1, 0, nullptr, xms33_args, nullptr), "bench neo crossmodal_stack");
            s.d.check(s.d.record(b, nullptr), "rec xms33b");
            s.d.check(s.d.wait(b), "wait xms33b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "xms33elapsed");
            xms33times.push_back(ms / XMS33BATCH);
        }
        std::sort(xms33times.begin(), xms33times.end());
        double xms33_us = xms33times[xms33times.size() / 2] * 1000.0;
        NeoCrossModalStackResult xms33res{};
        s.d.check(s.d.download(&xms33res, d_xmsr.ptr, sizeof(xms33res)), "download xms33res");

        // Kernel 34 (STAGE 7 CROSSING!): Blackwell GPU Pure-Spiking Multi-Turn Dialogue Coherence, Topic-Shift & Contradiction Cortex (`mind_neo_dialogue_cortex`) Benchmark!
        Function fn_dlg34{};
        s.bind(fn_dlg34, "mind_neo_dialogue_cortex", Image::mind);
        Temporary d_dlgq(s.d, sizeof(NeoDialogueQuery)), d_dlgr(s.d, sizeof(NeoDialogueResult));
        void *dlg34_args[]{&d_nw.ptr, &d_dlgq.ptr, &d_g.ptr, &d_dlgr.ptr};
        Word dlg_tot_ed = 0u;
        static constexpr Word dlg_pairs[4][2] = {{0u, 1u}, {2u, 6u}, {3u, 0u}, {4u, 1u}};
        for (Word m = 0; m < 4u; ++m) {
            NeoDialogueQuery dq{};
            dq.pathway = 27; dq.mode = 0; dq.clear_first = (m == 0u ? 1u : 0u); dq.dlg_mode = m;
            dq.cue_a = dlg_pairs[m][0]; dq.cue_b = dlg_pairs[m][1];
            s.d.check(s.d.upload(d_dlgq.ptr, &dq, sizeof(dq)), "upload bench dlg write");
            s.d.check(s.d.launch(fn_dlg34, 1, 1, 1, 256, 1, 1, 0, nullptr, dlg34_args, nullptr), "bench dlg write");
            NeoDialogueResult dw{};
            s.d.check(s.d.download(&dw, d_dlgr.ptr, sizeof(dw)), "download dlg write");
            dlg_tot_ed += dw.edits_made;
        }
        // Turn 1 (`dlgcompare`) -> Turn 2 (`dlgempathize` triggers `topic_shifted=1` straight from L2!)
        NeoDialogueQuery dq1{}; dq1.pathway = 27; dq1.mode = 1; dq1.cue_a = 4u; dq1.cue_b = 1u;
        s.d.check(s.d.upload(d_dlgq.ptr, &dq1, sizeof(dq1)), "upload bench dlg t1");
        s.d.check(s.d.launch(fn_dlg34, 1, 1, 1, 256, 1, 1, 0, nullptr, dlg34_args, nullptr), "bench dlg t1");
        NeoDialogueResult dr1{};
        s.d.check(s.d.download(&dr1, d_dlgr.ptr, sizeof(dr1)), "download bench dlg t1");
        dlg_tot_ed += dr1.edits_made;
        NeoDialogueQuery dq2{}; dq2.pathway = 27; dq2.mode = 1; dq2.cue_a = 2u; dq2.cue_b = 6u;
        s.d.check(s.d.upload(d_dlgq.ptr, &dq2, sizeof(dq2)), "upload bench dlg t2");
        s.d.check(s.d.launch(fn_dlg34, 1, 1, 1, 256, 1, 1, 0, nullptr, dlg34_args, nullptr), "bench dlg t2");
        NeoDialogueResult dr_shift{};
        s.d.check(s.d.download(&dr_shift, d_dlgr.ptr, sizeof(dr_shift)), "download bench dlg shift");
        dlg_tot_ed += dr_shift.edits_made;
        // Imprint positive stance on claim 0, then check negative stance -> `coh_status=2` (`dlgcontra`) straight from L2!
        NeoDialogueQuery dst{}; dst.pathway = 27; dst.mode = 2; dst.claim_slot = 0u; dst.polarity = 1u;
        s.d.check(s.d.upload(d_dlgq.ptr, &dst, sizeof(dst)), "upload bench dlg stance");
        s.d.check(s.d.launch(fn_dlg34, 1, 1, 1, 256, 1, 1, 0, nullptr, dlg34_args, nullptr), "bench dlg stance");
        NeoDialogueResult dstr{};
        s.d.check(s.d.download(&dstr, d_dlgr.ptr, sizeof(dstr)), "download bench dlg stance");
        dlg_tot_ed += dstr.edits_made;
        NeoDialogueQuery dchk{}; dchk.pathway = 27; dchk.mode = 3; dchk.claim_slot = 0u; dchk.polarity = 0u;
        s.d.check(s.d.upload(d_dlgq.ptr, &dchk, sizeof(dchk)), "upload bench dlg check");
        s.d.check(s.d.launch(fn_dlg34, 1, 1, 1, 256, 1, 1, 0, nullptr, dlg34_args, nullptr), "bench dlg check");
        NeoDialogueResult dr_contra{};
        s.d.check(s.d.download(&dr_contra, d_dlgr.ptr, sizeof(dr_contra)), "download bench dlg contra");
        // Time read-only Mode 1 pragmatic classification straight from L2 PW27[768..895]!
        NeoDialogueQuery dqr{}; dqr.pathway = 27; dqr.mode = 1; dqr.read_only = 1; dqr.cue_a = 4u; dqr.cue_b = 1u;
        s.d.check(s.d.upload(d_dlgq.ptr, &dqr, sizeof(dqr)), "upload bench dlg read");
        s.d.check(s.d.launch(fn_dlg34, 1, 1, 1, 256, 1, 1, 0, nullptr, dlg34_args, nullptr), "warmup neo dialogue_cortex");
        std::vector<double> dlg34times;
        constexpr Word DLG34BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec dlg34a");
            for (Word k = 0; k < DLG34BATCH; ++k)
                s.d.check(s.d.launch(fn_dlg34, 1, 1, 1, 256, 1, 1, 0, nullptr, dlg34_args, nullptr), "bench neo dialogue_cortex");
            s.d.check(s.d.record(b, nullptr), "rec dlg34b");
            s.d.check(s.d.wait(b), "wait dlg34b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "dlg34elapsed");
            dlg34times.push_back(ms / DLG34BATCH);
        }
        std::sort(dlg34times.begin(), dlg34times.end());
        double dlg34_us = dlg34times[dlg34times.size() / 2] * 1000.0;
        NeoDialogueResult dlg34res{};
        s.d.check(s.d.download(&dlg34res, d_dlgr.ptr, sizeof(dlg34res)), "download dlg34res");

        // Kernel 35 (STAGE 7b!): Blackwell GPU Pure-Spiking Metaphor Blending, 2-Hop Narrative Arc & Socratic Self-Critique Cortex (`mind_neo_narrative_metaphor`) Benchmark!
        Function fn_nmet35{};
        s.bind(fn_nmet35, "mind_neo_narrative_metaphor", Image::mind);
        Temporary d_nmetq(s.d, sizeof(NeoNarrativeMetaphorQuery)), d_nmetr(s.d, sizeof(NeoNarrativeMetaphorResult));
        void *nmet35_args[]{&d_nw.ptr, &d_nmetq.ptr, &d_g.ptr, &d_nmetr.ptr};
        Word nmet_tot_ed = 0u;
        static constexpr Word met_pairs[4][2] = {{0u, 1u}, {2u, 3u}, {4u, 5u}, {6u, 7u}};
        for (Word b = 0; b < 4u; ++b) {
            NeoNarrativeMetaphorQuery mq{};
            mq.pathway = 27; mq.mode = 0; mq.clear_first = (b == 0u ? 1u : 0u); mq.slot_id = b;
            mq.cue_a = met_pairs[b][0]; mq.cue_b = met_pairs[b][1];
            s.d.check(s.d.upload(d_nmetq.ptr, &mq, sizeof(mq)), "upload bench nmet blend");
            s.d.check(s.d.launch(fn_nmet35, 1, 1, 1, 256, 1, 1, 0, nullptr, nmet35_args, nullptr), "bench nmet blend");
            NeoNarrativeMetaphorResult mr{};
            s.d.check(s.d.download(&mr, d_nmetr.ptr, sizeof(mr)), "download bench nmet blend");
            nmet_tot_ed += mr.edits_made;
        }
        static constexpr Word plot_edges[3][2] = {{0u, 1u}, {1u, 2u}, {2u, 3u}};
        for (Word e = 0; e < 3u; ++e) {
            NeoNarrativeMetaphorQuery mq{};
            mq.pathway = 27; mq.mode = 2; mq.clear_first = (e == 0u ? 1u : 0u);
            mq.slot_id = plot_edges[e][1]; mq.cue_a = plot_edges[e][0]; mq.cue_b = ~0u;
            s.d.check(s.d.upload(d_nmetq.ptr, &mq, sizeof(mq)), "upload bench nmet plot");
            s.d.check(s.d.launch(fn_nmet35, 1, 1, 1, 256, 1, 1, 0, nullptr, nmet35_args, nullptr), "bench nmet plot");
            NeoNarrativeMetaphorResult mr{};
            s.d.check(s.d.download(&mr, d_nmetr.ptr, sizeof(mr)), "download bench nmet plot");
            nmet_tot_ed += mr.edits_made;
        }
        static constexpr Word flw_pairs[4][2] = {{4u, 5u}, {6u, 4u}, {7u, 5u}, {8u, 6u}};
        for (Word f = 0; f < 4u; ++f) {
            NeoNarrativeMetaphorQuery mq{};
            mq.pathway = 27; mq.mode = 2; mq.clear_first = 0u;
            mq.slot_id = 4u + f; mq.cue_a = flw_pairs[f][0]; mq.cue_b = flw_pairs[f][1];
            s.d.check(s.d.upload(d_nmetq.ptr, &mq, sizeof(mq)), "upload bench nmet flaw");
            s.d.check(s.d.launch(fn_nmet35, 1, 1, 1, 256, 1, 1, 0, nullptr, nmet35_args, nullptr), "bench nmet flaw");
            NeoNarrativeMetaphorResult mr{};
            s.d.check(s.d.download(&mr, d_nmetr.ptr, sizeof(mr)), "download bench nmet flaw");
            nmet_tot_ed += mr.edits_made;
        }
        // Verify 2-hop narrative plot rollout (`0 -> 1 -> 2`) & Socratic self-critique (`4+5 -> flaw 0`) straight from L2!
        NeoNarrativeMetaphorQuery mpq{}; mpq.pathway = 27; mpq.mode = 3; mpq.cue_a = 0u; mpq.cue_b = ~0u; mpq.hops = 2u;
        s.d.check(s.d.upload(d_nmetq.ptr, &mpq, sizeof(mpq)), "upload bench nmet plot q");
        s.d.check(s.d.launch(fn_nmet35, 1, 1, 1, 256, 1, 1, 0, nullptr, nmet35_args, nullptr), "bench nmet plot q");
        NeoNarrativeMetaphorResult nm_plot{};
        s.d.check(s.d.download(&nm_plot, d_nmetr.ptr, sizeof(nm_plot)), "download bench nmet plot q");
        NeoNarrativeMetaphorQuery mfq{}; mfq.pathway = 27; mfq.mode = 4; mfq.cue_a = 4u; mfq.cue_b = 5u;
        s.d.check(s.d.upload(d_nmetq.ptr, &mfq, sizeof(mfq)), "upload bench nmet flaw q");
        s.d.check(s.d.launch(fn_nmet35, 1, 1, 1, 256, 1, 1, 0, nullptr, nmet35_args, nullptr), "bench nmet flaw q");
        NeoNarrativeMetaphorResult nm_flaw{};
        s.d.check(s.d.download(&nm_flaw, d_nmetr.ptr, sizeof(nm_flaw)), "download bench nmet flaw q");
        // Time Mode 1 forward conceptual metaphor blend (`mflow(0) + mtime(1) -> metriver(0)`) straight from L2!
        NeoNarrativeMetaphorQuery mbq{}; mbq.pathway = 27; mbq.mode = 1; mbq.cue_a = 0u; mbq.cue_b = 1u;
        s.d.check(s.d.upload(d_nmetq.ptr, &mbq, sizeof(mbq)), "upload bench nmet blend q");
        s.d.check(s.d.launch(fn_nmet35, 1, 1, 1, 256, 1, 1, 0, nullptr, nmet35_args, nullptr), "warmup neo narrative_metaphor");
        std::vector<double> nmet35times;
        constexpr Word NMET35BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec nmet35a");
            for (Word k = 0; k < NMET35BATCH; ++k)
                s.d.check(s.d.launch(fn_nmet35, 1, 1, 1, 256, 1, 1, 0, nullptr, nmet35_args, nullptr), "bench neo narrative_metaphor");
            s.d.check(s.d.record(b, nullptr), "rec nmet35b");
            s.d.check(s.d.wait(b), "wait nmet35b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "nmet35elapsed");
            nmet35times.push_back(ms / NMET35BATCH);
        }
        std::sort(nmet35times.begin(), nmet35times.end());
        double nmet35_us = nmet35times[nmet35times.size() / 2] * 1000.0;
        NeoNarrativeMetaphorResult nmet35res{};
        s.d.check(s.d.download(&nmet35res, d_nmetr.ptr, sizeof(nmet35res)), "download nmet35res");

        // Kernel 36 (STAGE 7c — 400 DOMAINS!): Blackwell GPU Pure-Spiking Epistemic Belief-Graph & Scientific Debate Cortex (`mind_neo_belief_debate`) Benchmark!
        Function fn_bdeb36{};
        s.bind(fn_bdeb36, "mind_neo_belief_debate", Image::mind);
        Temporary d_bdebq(s.d, sizeof(NeoBeliefDebateQuery)), d_bdebr(s.d, sizeof(NeoBeliefDebateResult));
        void *bdeb36_args[]{&d_nw.ptr, &d_bdebq.ptr, &d_g.ptr, &d_bdebr.ptr};
        Word bdeb_tot_ed = 0u;
        {
            NeoBeliefDebateQuery bq{}; bq.pathway = 24; bq.mode = 4;
            s.d.check(s.d.upload(d_bdebq.ptr, &bq, sizeof(bq)), "upload bench bdeb clr");
            s.d.check(s.d.launch(fn_bdeb36, 1, 1, 1, 256, 1, 1, 0, nullptr, bdeb36_args, nullptr), "bench bdeb clr");
            NeoBeliefDebateResult br{};
            s.d.check(s.d.download(&br, d_bdebr.ptr, sizeof(br)), "download bench bdeb clr");
            bdeb_tot_ed += br.edits_made;
        }
        // Imprint 3 canonical scientific theories (0: ev 0+1 +sup; 1: ev 0 +sup, ev 2 -ref; 2: ev 2+3 +sup)
        {
            NeoBeliefDebateQuery bq{}; bq.pathway = 24; bq.mode = 0; bq.claim_id = 0; bq.ev_a = 0; bq.ev_b = 1; bq.polarity = 1;
            s.d.check(s.d.upload(d_bdebq.ptr, &bq, sizeof(bq)), "upload bench bdeb c0");
            s.d.check(s.d.launch(fn_bdeb36, 1, 1, 1, 256, 1, 1, 0, nullptr, bdeb36_args, nullptr), "bench bdeb c0");
            NeoBeliefDebateResult br{}; s.d.check(s.d.download(&br, d_bdebr.ptr, sizeof(br)), "download bench bdeb c0");
            bdeb_tot_ed += br.edits_made;
            bq.claim_id = 1; bq.ev_a = 0; bq.ev_b = ~0u; bq.polarity = 1;
            s.d.check(s.d.upload(d_bdebq.ptr, &bq, sizeof(bq)), "upload bench bdeb c1a");
            s.d.check(s.d.launch(fn_bdeb36, 1, 1, 1, 256, 1, 1, 0, nullptr, bdeb36_args, nullptr), "bench bdeb c1a");
            s.d.check(s.d.download(&br, d_bdebr.ptr, sizeof(br)), "download bench bdeb c1a");
            bdeb_tot_ed += br.edits_made;
            bq.ev_a = 2; bq.ev_b = ~0u; bq.polarity = 0;
            s.d.check(s.d.upload(d_bdebq.ptr, &bq, sizeof(bq)), "upload bench bdeb c1b");
            s.d.check(s.d.launch(fn_bdeb36, 1, 1, 1, 256, 1, 1, 0, nullptr, bdeb36_args, nullptr), "bench bdeb c1b");
            s.d.check(s.d.download(&br, d_bdebr.ptr, sizeof(br)), "download bench bdeb c1b");
            bdeb_tot_ed += br.edits_made;
            bq.claim_id = 2; bq.ev_a = 2; bq.ev_b = 3; bq.polarity = 1;
            s.d.check(s.d.upload(d_bdebq.ptr, &bq, sizeof(bq)), "upload bench bdeb c2");
            s.d.check(s.d.launch(fn_bdeb36, 1, 1, 1, 256, 1, 1, 0, nullptr, bdeb36_args, nullptr), "bench bdeb c2");
            s.d.check(s.d.download(&br, d_bdebr.ptr, sizeof(br)), "download bench bdeb c2");
            bdeb_tot_ed += br.edits_made;
        }
        for (Word k = 0; k < 2u; ++k) {
            NeoBeliefDebateQuery qa{}; qa.pathway = 24; qa.mode = 2; qa.peer_a = 0; qa.polarity = 1;
            s.d.check(s.d.upload(d_bdebq.ptr, &qa, sizeof(qa)), "upload bench bdeb pa");
            s.d.check(s.d.launch(fn_bdeb36, 1, 1, 1, 256, 1, 1, 0, nullptr, bdeb36_args, nullptr), "bench bdeb pa");
            NeoBeliefDebateResult br{}; s.d.check(s.d.download(&br, d_bdebr.ptr, sizeof(br)), "download bench bdeb pa");
            bdeb_tot_ed += br.edits_made;
            NeoBeliefDebateQuery ql{}; ql.pathway = 24; ql.mode = 2; ql.peer_a = 1; ql.polarity = 0;
            s.d.check(s.d.upload(d_bdebq.ptr, &ql, sizeof(ql)), "upload bench bdeb pl");
            s.d.check(s.d.launch(fn_bdeb36, 1, 1, 1, 256, 1, 1, 0, nullptr, bdeb36_args, nullptr), "bench bdeb pl");
            s.d.check(s.d.download(&br, d_bdebr.ptr, sizeof(br)), "download bench bdeb pl");
            bdeb_tot_ed += br.edits_made;
        }
        // Verify Mode 3 peer credibility debate (`peer 0 vs peer 1 -> best_peer=0, margin>0`)!
        NeoBeliefDebateQuery bd_pq{}; bd_pq.pathway = 24; bd_pq.mode = 3; bd_pq.peer_a = 0; bd_pq.peer_b = 1;
        s.d.check(s.d.upload(d_bdebq.ptr, &bd_pq, sizeof(bd_pq)), "upload bench bdeb peerq");
        s.d.check(s.d.launch(fn_bdeb36, 1, 1, 1, 256, 1, 1, 0, nullptr, bdeb36_args, nullptr), "bench bdeb peerq");
        NeoBeliefDebateResult bd_peer{};
        s.d.check(s.d.download(&bd_peer, d_bdebr.ptr, sizeof(bd_peer)), "download bench bdeb peerq");
        // Time Mode 1 belief-graph evaluation (`ev 0 + ev 1 -> claim 0, verdict 1`) straight from L2!
        NeoBeliefDebateQuery bd_cq{}; bd_cq.pathway = 24; bd_cq.mode = 1; bd_cq.ev_a = 0; bd_cq.ev_b = 1;
        s.d.check(s.d.upload(d_bdebq.ptr, &bd_cq, sizeof(bd_cq)), "upload bench bdeb cq");
        s.d.check(s.d.launch(fn_bdeb36, 1, 1, 1, 256, 1, 1, 0, nullptr, bdeb36_args, nullptr), "warmup neo belief_debate");
        std::vector<double> bdeb36times;
        constexpr Word BDEB36BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec bdeb36a");
            for (Word k = 0; k < BDEB36BATCH; ++k)
                s.d.check(s.d.launch(fn_bdeb36, 1, 1, 1, 256, 1, 1, 0, nullptr, bdeb36_args, nullptr), "bench neo belief_debate");
            s.d.check(s.d.record(b, nullptr), "rec bdeb36b");
            s.d.check(s.d.wait(b), "wait bdeb36b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "bdeb36elapsed");
            bdeb36times.push_back(ms / BDEB36BATCH);
        }
        std::sort(bdeb36times.begin(), bdeb36times.end());
        double bdeb36_us = bdeb36times[bdeb36times.size() / 2] * 1000.0;
        NeoBeliefDebateResult bdeb36res{};
        s.d.check(s.d.download(&bdeb36res, d_bdebr.ptr, sizeof(bdeb36res)), "download bdeb36res");

        // Kernel 37 (STAGE 8 CROSSING — COLLECTIVE SCIENTIFIC MINDS!): Blackwell GPU Pure-Spiking Multi-Mind Collective Discovery & Compositional Theorem Prover (`mind_neo_collective_discovery`) Benchmark!
        Function fn_cdis37{};
        s.bind(fn_cdis37, "mind_neo_collective_discovery", Image::mind);
        Temporary d_cdisq(s.d, sizeof(NeoCollectiveDiscoveryQuery)), d_cdisr(s.d, sizeof(NeoCollectiveDiscoveryResult));
        void *cdis37_args[]{&d_nw.ptr, &d_cdisq.ptr, &d_g.ptr, &d_cdisr.ptr};
        Word cdis_tot_ed = 0u;
        {
            NeoCollectiveDiscoveryQuery cq{}; cq.pathway = 24; cq.mode = 4;
            s.d.check(s.d.upload(d_cdisq.ptr, &cq, sizeof(cq)), "upload bench cdis clr");
            s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "bench cdis clr");
            NeoCollectiveDiscoveryResult cr{};
            s.d.check(s.d.download(&cr, d_cdisr.ptr, sizeof(cr)), "download bench cdis clr");
            cdis_tot_ed += cr.edits_made;
        }
        // Imprint 3 canonical collective conjectures & 3 canonical compositional theorems via evolved STDP (`g.basal_rule`)!
        {
            NeoCollectiveDiscoveryQuery cq{}; cq.pathway = 24; cq.mode = 0; cq.law_id = 0; cq.cue_a = 0; cq.cue_b = 2; cq.polarity = 1;
            s.d.check(s.d.upload(d_cdisq.ptr, &cq, sizeof(cq)), "upload bench cdis l0");
            s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "bench cdis l0");
            NeoCollectiveDiscoveryResult cr{}; s.d.check(s.d.download(&cr, d_cdisr.ptr, sizeof(cr)), "download bench cdis l0");
            cdis_tot_ed += cr.edits_made;
            cq.law_id = 1; cq.cue_a = 1; cq.cue_b = 2; cq.polarity = 1;
            s.d.check(s.d.upload(d_cdisq.ptr, &cq, sizeof(cq)), "upload bench cdis l1");
            s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "bench cdis l1");
            s.d.check(s.d.download(&cr, d_cdisr.ptr, sizeof(cr)), "download bench cdis l1");
            cdis_tot_ed += cr.edits_made;
            cq.law_id = 2; cq.cue_a = 0; cq.cue_b = 3; cq.polarity = 0;
            s.d.check(s.d.upload(d_cdisq.ptr, &cq, sizeof(cq)), "upload bench cdis l2");
            s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "bench cdis l2");
            s.d.check(s.d.download(&cr, d_cdisr.ptr, sizeof(cr)), "download bench cdis l2");
            cdis_tot_ed += cr.edits_made;
            cq = {}; cq.pathway = 24; cq.mode = 2; cq.thm_id = 0; cq.cue_a = 4; cq.cue_b = 5;
            s.d.check(s.d.upload(d_cdisq.ptr, &cq, sizeof(cq)), "upload bench cdis t0");
            s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "bench cdis t0");
            s.d.check(s.d.download(&cr, d_cdisr.ptr, sizeof(cr)), "download bench cdis t0");
            cdis_tot_ed += cr.edits_made;
            cq.thm_id = 1; cq.cue_a = 6; cq.cue_b = 7;
            s.d.check(s.d.upload(d_cdisq.ptr, &cq, sizeof(cq)), "upload bench cdis t1");
            s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "bench cdis t1");
            s.d.check(s.d.download(&cr, d_cdisr.ptr, sizeof(cr)), "download bench cdis t1");
            cdis_tot_ed += cr.edits_made;
            cq.thm_id = 2; cq.cue_a = 8; cq.cue_b = 9;
            s.d.check(s.d.upload(d_cdisq.ptr, &cq, sizeof(cq)), "upload bench cdis t2");
            s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "bench cdis t2");
            s.d.check(s.d.download(&cr, d_cdisr.ptr, sizeof(cr)), "download bench cdis t2");
            cdis_tot_ed += cr.edits_made;
        }
        // Verify Mode 3 forward theorem derivation (`ax 4+5 -> thm 0`) and backward `W^T` proof (`thm 0 -> ax 4, 5`)!
        NeoCollectiveDiscoveryQuery cd_tq{}; cd_tq.pathway = 24; cd_tq.mode = 3; cd_tq.cue_a = 4; cd_tq.cue_b = 5;
        s.d.check(s.d.upload(d_cdisq.ptr, &cd_tq, sizeof(cd_tq)), "upload bench cdis thm");
        s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "bench cdis thm");
        NeoCollectiveDiscoveryResult cd_thm{};
        s.d.check(s.d.download(&cd_thm, d_cdisr.ptr, sizeof(cd_thm)), "download bench cdis thm");
        NeoCollectiveDiscoveryQuery cd_pq{}; cd_pq.pathway = 24; cd_pq.mode = 3; cd_pq.thm_id = 0; cd_pq.cue_a = ~0u; cd_pq.cue_b = ~0u;
        s.d.check(s.d.upload(d_cdisq.ptr, &cd_pq, sizeof(cd_pq)), "upload bench cdis prove");
        s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "bench cdis prove");
        NeoCollectiveDiscoveryResult cd_prv{};
        s.d.check(s.d.download(&cd_prv, d_cdisr.ptr, sizeof(cd_prv)), "download bench cdis prove");
        // Time Mode 1 open collective consensus evaluation (`spec 0+2 -> law 0, verdict 1`) straight from L2!
        NeoCollectiveDiscoveryQuery cd_cq{}; cd_cq.pathway = 24; cd_cq.mode = 1; cd_cq.law_id = ~0u; cd_cq.cue_a = 0; cd_cq.cue_b = 2;
        s.d.check(s.d.upload(d_cdisq.ptr, &cd_cq, sizeof(cd_cq)), "upload bench cdis cq");
        s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "warmup neo collective_discovery");
        std::vector<double> cdis37times;
        constexpr Word CDIS37BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec cdis37a");
            for (Word k = 0; k < CDIS37BATCH; ++k)
                s.d.check(s.d.launch(fn_cdis37, 1, 1, 1, 256, 1, 1, 0, nullptr, cdis37_args, nullptr), "bench neo collective_discovery");
            s.d.check(s.d.record(b, nullptr), "rec cdis37b");
            s.d.check(s.d.wait(b), "wait cdis37b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "cdis37elapsed");
            cdis37times.push_back(ms / CDIS37BATCH);
        }
        std::sort(cdis37times.begin(), cdis37times.end());
        double cdis37_us = cdis37times[cdis37times.size() / 2] * 1000.0;
        NeoCollectiveDiscoveryResult cdis37res{};
        s.d.check(s.d.download(&cdis37res, d_cdisr.ptr, sizeof(cdis37res)), "download cdis37res");

        // Kernel 38 (STAGE 8b — CONTINUAL SELF-RESEARCH & COUNTERFACTUAL CODE-PATCHING CORTEX!):
        // Pure-GPU Optimal Discriminative Experiment Design + Counterfactual Bug->Patch Synthesis on FREE orthogonal quadrant of L2 PW27 (`pre 768..1023 -> post 0..255`, ZERO extra L2 memory)!
        Function fn_sres38{};
        s.bind(fn_sres38, "mind_neo_self_research", Image::mind);
        Temporary d_sresq(s.d, sizeof(NeoSelfResearchQuery)), d_sresr(s.d, sizeof(NeoSelfResearchResult));
        void *sres38_args[]{&d_nw.ptr, &d_sresq.ptr, &d_g.ptr, &d_sresr.ptr};
        Word sres_tot_ed = 0;
        {
            NeoSelfResearchQuery sq{}; sq.pathway = 27; sq.mode = 4;
            s.d.check(s.d.upload(d_sresq.ptr, &sq, sizeof(sq)), "upload bench sres clr");
            s.d.check(s.d.launch(fn_sres38, 1, 1, 1, 256, 1, 1, 0, nullptr, sres38_args, nullptr), "bench sres clr");
            NeoSelfResearchResult sr{};
            s.d.check(s.d.download(&sr, d_sresr.ptr, sizeof(sr)), "download bench sres clr");
            sres_tot_ed += sr.edits_made;
        }
        // Imprint 4 canonical discriminative experiments (`exp 0..3`, polarity=2) & 4 canonical code-repair patches (`patch 0..3`) via evolved STDP (`g.basal_rule`)!
        {
            const Word exps[4][3] = {{0u, 0u, 1u}, {1u, 2u, 3u}, {2u, 4u, 5u}, {3u, 6u, 7u}};
            for (int e = 0; e < 4; ++e) {
                NeoSelfResearchQuery sq{}; sq.pathway = 27; sq.mode = 0; sq.exp_id = exps[e][0]; sq.cue_a = exps[e][1]; sq.cue_b = exps[e][2]; sq.polarity = 2;
                s.d.check(s.d.upload(d_sresq.ptr, &sq, sizeof(sq)), "upload bench sres exp");
                s.d.check(s.d.launch(fn_sres38, 1, 1, 1, 256, 1, 1, 0, nullptr, sres38_args, nullptr), "bench sres exp");
                NeoSelfResearchResult sr{}; s.d.check(s.d.download(&sr, d_sresr.ptr, sizeof(sr)), "download bench sres exp");
                sres_tot_ed += sr.edits_made;
            }
            const Word pats[4][3] = {{0u, 8u, 9u}, {1u, 10u, 11u}, {2u, 11u, 14u}, {3u, 13u, 15u}};
            for (int p = 0; p < 4; ++p) {
                NeoSelfResearchQuery sq{}; sq.pathway = 27; sq.mode = 2; sq.patch_id = pats[p][0]; sq.cue_a = pats[p][1]; sq.cue_b = pats[p][2];
                s.d.check(s.d.upload(d_sresq.ptr, &sq, sizeof(sq)), "upload bench sres pat");
                s.d.check(s.d.launch(fn_sres38, 1, 1, 1, 256, 1, 1, 0, nullptr, sres38_args, nullptr), "bench sres pat");
                NeoSelfResearchResult sr{}; s.d.check(s.d.download(&sr, d_sresr.ptr, sizeof(sr)), "download bench sres pat");
                sres_tot_ed += sr.edits_made;
            }
        }
        // Verify Mode 3 forward patch synthesis (`sym 8+9 -> patch 0`) and backward `W^T` root-cause recall (`patch 0 -> sym 8, 9`)!
        NeoSelfResearchQuery sr_pq{}; sr_pq.pathway = 27; sr_pq.mode = 3; sr_pq.cue_a = 8; sr_pq.cue_b = 9;
        s.d.check(s.d.upload(d_sresq.ptr, &sr_pq, sizeof(sr_pq)), "upload bench sres patf");
        s.d.check(s.d.launch(fn_sres38, 1, 1, 1, 256, 1, 1, 0, nullptr, sres38_args, nullptr), "bench sres patf");
        NeoSelfResearchResult sr_pat{};
        s.d.check(s.d.download(&sr_pat, d_sresr.ptr, sizeof(sr_pat)), "download bench sres patf");
        NeoSelfResearchQuery sr_wq{}; sr_wq.pathway = 27; sr_wq.mode = 3; sr_wq.patch_id = 0; sr_wq.cue_a = ~0u; sr_wq.cue_b = ~0u;
        s.d.check(s.d.upload(d_sresq.ptr, &sr_wq, sizeof(sr_wq)), "upload bench sres why");
        s.d.check(s.d.launch(fn_sres38, 1, 1, 1, 256, 1, 1, 0, nullptr, sres38_args, nullptr), "bench sres why");
        NeoSelfResearchResult sr_why{};
        s.d.check(s.d.download(&sr_why, d_sresr.ptr, sizeof(sr_why)), "download bench sres why");
        // Time Mode 1 optimal discriminative experiment design (`hyp 0 vs 1 -> exp 0, disc > 0`) straight from L2!
        NeoSelfResearchQuery sr_dq{}; sr_dq.pathway = 27; sr_dq.mode = 1; sr_dq.cue_a = 0; sr_dq.cue_b = 1;
        s.d.check(s.d.upload(d_sresq.ptr, &sr_dq, sizeof(sr_dq)), "upload bench sres dq");
        s.d.check(s.d.launch(fn_sres38, 1, 1, 1, 256, 1, 1, 0, nullptr, sres38_args, nullptr), "warmup neo self_research");
        std::vector<double> sres38times;
        constexpr Word SRES38BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec sres38a");
            for (Word k = 0; k < SRES38BATCH; ++k)
                s.d.check(s.d.launch(fn_sres38, 1, 1, 1, 256, 1, 1, 0, nullptr, sres38_args, nullptr), "bench neo self_research");
            s.d.check(s.d.record(b, nullptr), "rec sres38b");
            s.d.check(s.d.wait(b), "wait sres38b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "sres38elapsed");
            sres38times.push_back(ms / SRES38BATCH);
        }
        std::sort(sres38times.begin(), sres38times.end());
        double sres38_us = sres38times[sres38times.size() / 2] * 1000.0;
        NeoSelfResearchResult sres38res{};
        s.d.check(s.d.download(&sres38res, d_sresr.ptr, sizeof(sres38res)), "download sres38res");

        // Kernel 39 (STAGE 8c — PURE-GPU NEURAL SYMBOLIC ALGEBRA & CALCULUS CORTEX!):
        // Pure-GPU Spiking Symbolic Differentiation, Definite Integration & Horner Evaluation on FREE orthogonal quadrant 0 of L2 PW13 (`rows 0..255, cols 0..255`, ZERO extra L2 memory)!
        Function fn_symc39{};
        s.bind(fn_symc39, "mind_neo_symbolic_calculus", Image::mind);
        Temporary d_symcq(s.d, sizeof(NeoSymCalcQuery)), d_symcr(s.d, sizeof(NeoSymCalcResult));
        void *symc39_args[]{&d_nw.ptr, &d_symcq.ptr, &d_g.ptr, &d_symcr.ptr};
        Word symc_tot_ed = 0;
        {
            NeoSymCalcQuery cq{}; cq.pathway = 13; cq.mode = 4;
            s.d.check(s.d.upload(d_symcq.ptr, &cq, sizeof(cq)), "upload bench symc clr");
            s.d.check(s.d.launch(fn_symc39, 1, 1, 1, 256, 1, 1, 0, nullptr, symc39_args, nullptr), "bench symc clr");
            NeoSymCalcResult cr{}; s.d.check(s.d.download(&cr, d_symcr.ptr, sizeof(cr)), "download bench symc clr");
            symc_tot_ed += cr.edits_made;
        }
        // Imprint 3 canonical polynomials (`polyalpha: 2+6x+3x^2`, `polybeta: 3+4x+6x^2+4x^3`, `polygamma: 1+2x+3x^2`) via evolved STDP (`g.basal_rule`)!
        {
            const Word polys[3][4] = {{2u, 6u, 3u, 0u}, {3u, 4u, 6u, 4u}, {1u, 2u, 3u, 0u}};
            for (Word p = 0; p < 3u; ++p) {
                NeoSymCalcQuery cq{}; cq.pathway = 13; cq.mode = 0; cq.poly_id = p;
                for (Word d = 0; d < 4u; ++d) cq.coeffs[d] = polys[p][d];
                s.d.check(s.d.upload(d_symcq.ptr, &cq, sizeof(cq)), "upload bench symc poly");
                s.d.check(s.d.launch(fn_symc39, 1, 1, 1, 256, 1, 1, 0, nullptr, symc39_args, nullptr), "bench symc poly");
                NeoSymCalcResult cr{}; s.d.check(s.d.download(&cr, d_symcr.ptr, sizeof(cr)), "download bench symc poly");
                symc_tot_ed += cr.edits_made;
            }
        }
        // Verify Mode 2 definite integral (`\int_0^2 P0 dx = 24`) and Mode 3 Horner eval (`P2(3) = 34`)!
        NeoSymCalcQuery sc_iq{}; sc_iq.pathway = 13; sc_iq.mode = 2; sc_iq.poly_id = 0; sc_iq.eval_x = 2;
        s.d.check(s.d.upload(d_symcq.ptr, &sc_iq, sizeof(sc_iq)), "upload bench symc int");
        s.d.check(s.d.launch(fn_symc39, 1, 1, 1, 256, 1, 1, 0, nullptr, symc39_args, nullptr), "bench symc int");
        NeoSymCalcResult sc_int{}; s.d.check(s.d.download(&sc_int, d_symcr.ptr, sizeof(sc_int)), "download bench symc int");
        NeoSymCalcQuery sc_eq{}; sc_eq.pathway = 13; sc_eq.mode = 3; sc_eq.poly_id = 2; sc_eq.eval_x = 3;
        s.d.check(s.d.upload(d_symcq.ptr, &sc_eq, sizeof(sc_eq)), "upload bench symc eval");
        s.d.check(s.d.launch(fn_symc39, 1, 1, 1, 256, 1, 1, 0, nullptr, symc39_args, nullptr), "bench symc eval");
        NeoSymCalcResult sc_ev{}; s.d.check(s.d.download(&sc_ev, d_symcr.ptr, sizeof(sc_ev)), "download bench symc eval");
        // Time Mode 1 pure-GPU symbolic differentiation (`P0'(2) = 18`) straight from L2 PW13!
        NeoSymCalcQuery sc_dq{}; sc_dq.pathway = 13; sc_dq.mode = 1; sc_dq.poly_id = 0; sc_dq.eval_x = 2;
        s.d.check(s.d.upload(d_symcq.ptr, &sc_dq, sizeof(sc_dq)), "upload bench symc diff");
        s.d.check(s.d.launch(fn_symc39, 1, 1, 1, 256, 1, 1, 0, nullptr, symc39_args, nullptr), "warmup neo sym_calc");
        std::vector<double> symc39times;
        constexpr Word SYMC39BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec symc39a");
            for (Word k = 0; k < SYMC39BATCH; ++k)
                s.d.check(s.d.launch(fn_symc39, 1, 1, 1, 256, 1, 1, 0, nullptr, symc39_args, nullptr), "bench neo sym_calc");
            s.d.check(s.d.record(b, nullptr), "rec symc39b");
            s.d.check(s.d.wait(b), "wait symc39b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "symc39elapsed");
            symc39times.push_back(ms / SYMC39BATCH);
        }
        std::sort(symc39times.begin(), symc39times.end());
        double symc39_us = symc39times[symc39times.size() / 2] * 1000.0;
        NeoSymCalcResult symc39res{};
        s.d.check(s.d.download(&symc39res, d_symcr.ptr, sizeof(symc39res)), "download symc39res");

        // Kernel 40 (STAGE 8d — PURE-GPU AST PROGRAM SYNTHESIS, TYPE VERIFICATION & SYMBOLIC EXECUTION CORTEX!):
        // Lives on FREE orthogonal quadrant Q1 of L2 PW13 (`rows 0..255, cols 256..511`, ZERO extra L2 memory)!
        Function fn_past40{};
        s.bind(fn_past40, "mind_neo_program_ast", Image::mind);
        Temporary d_pastq(s.d, sizeof(NeoProgAstQuery)), d_pastr(s.d, sizeof(NeoProgAstResult));
        void *past40_args[]{&d_nw.ptr, &d_pastq.ptr, &d_g.ptr, &d_pastr.ptr};
        Word past_tot_ed = 0;
        {
            NeoProgAstQuery pq{}; pq.pathway = 13; pq.mode = 4;
            s.d.check(s.d.upload(d_pastq.ptr, &pq, sizeof(pq)), "upload bench past clr");
            s.d.check(s.d.launch(fn_past40, 1, 1, 1, 256, 1, 1, 0, nullptr, past40_args, nullptr), "bench past clr");
            NeoProgAstResult pr{}; s.d.check(s.d.download(&pr, d_pastr.ptr, sizeof(pr)), "download bench past clr");
            past_tot_ed += pr.edits_made;
        }
        // Imprint 3 canonical typed AST programs via evolved STDP (`g.basal_rule`):
        // 0=progalpha: MUL(2); ADD(3) [NAT->NAT], 1=progbeta: SQR; ADD(1) [NAT->NAT], 2=proggamma: ADD(2); MUL(3) [NAT->ERR]
        {
            const Word progs[3][8] = {
                {2u, 2u, 1u, 3u, 0u, 0u, 15u, 1u},
                {4u, 1u, 1u, 1u, 0u, 0u, 15u, 1u},
                {1u, 2u, 2u, 3u, 0u, 3u, 15u, 1u}
            };
            for (Word p = 0; p < 3u; ++p) {
                NeoProgAstQuery pq{}; pq.pathway = 13; pq.mode = 0; pq.prog_id = p;
                for (Word f = 0; f < 8u; ++f) pq.fields[f] = progs[p][f];
                s.d.check(s.d.upload(d_pastq.ptr, &pq, sizeof(pq)), "upload bench past prog");
                s.d.check(s.d.launch(fn_past40, 1, 1, 1, 256, 1, 1, 0, nullptr, past40_args, nullptr), "bench past prog");
                NeoProgAstResult pr{}; s.d.check(s.d.download(&pr, d_pastr.ptr, sizeof(pr)), "download bench past prog");
                past_tot_ed += pr.edits_made;
            }
        }
        // Verify Mode 1 static type check (`progalpha -> safe=1`) and Mode 3 256-thread parallel synthesis `(2->7, 4->13, x=6 -> 19)`!
        NeoProgAstQuery pa_vq{}; pa_vq.pathway = 13; pa_vq.mode = 1; pa_vq.prog_id = 0;
        s.d.check(s.d.upload(d_pastq.ptr, &pa_vq, sizeof(pa_vq)), "upload bench past ver");
        s.d.check(s.d.launch(fn_past40, 1, 1, 1, 256, 1, 1, 0, nullptr, past40_args, nullptr), "bench past ver");
        NeoProgAstResult pa_ver{}; s.d.check(s.d.download(&pa_ver, d_pastr.ptr, sizeof(pa_ver)), "download bench past ver");
        NeoProgAstQuery pa_sq{}; pa_sq.pathway = 13; pa_sq.mode = 3; pa_sq.prog_id = 3;
        pa_sq.in_x1 = 2; pa_sq.out_y1 = 7; pa_sq.in_x2 = 4; pa_sq.out_y2 = 13; pa_sq.eval_x = 6;
        s.d.check(s.d.upload(d_pastq.ptr, &pa_sq, sizeof(pa_sq)), "upload bench past syn");
        s.d.check(s.d.launch(fn_past40, 1, 1, 1, 256, 1, 1, 0, nullptr, past40_args, nullptr), "bench past syn");
        NeoProgAstResult pa_syn{}; s.d.check(s.d.download(&pa_syn, d_pastr.ptr, sizeof(pa_syn)), "download bench past syn");
        past_tot_ed += pa_syn.edits_made;
        // Time Mode 2 pure-GPU symbolic execution (`progbeta(4) = 17`) straight from L2 PW13 Q1!
        NeoProgAstQuery pa_eq{}; pa_eq.pathway = 13; pa_eq.mode = 2; pa_eq.prog_id = 1; pa_eq.eval_x = 4;
        s.d.check(s.d.upload(d_pastq.ptr, &pa_eq, sizeof(pa_eq)), "upload bench past exec");
        s.d.check(s.d.launch(fn_past40, 1, 1, 1, 256, 1, 1, 0, nullptr, past40_args, nullptr), "warmup neo prog_ast");
        std::vector<double> past40times;
        constexpr Word PAST40BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec past40a");
            for (Word k = 0; k < PAST40BATCH; ++k)
                s.d.check(s.d.launch(fn_past40, 1, 1, 1, 256, 1, 1, 0, nullptr, past40_args, nullptr), "bench neo prog_ast");
            s.d.check(s.d.record(b, nullptr), "rec past40b");
            s.d.check(s.d.wait(b), "wait past40b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "past40elapsed");
            past40times.push_back(ms / PAST40BATCH);
        }
        std::sort(past40times.begin(), past40times.end());
        double past40_us = past40times[past40times.size() / 2] * 1000.0;
        NeoProgAstResult past40res{};
        s.d.check(s.d.download(&past40res, d_pastr.ptr, sizeof(past40res)), "download past40res");

        // Kernel 41 (STAGE 8e — PURE-GPU NEURAL LINEAR ALGEBRA, MATVEC & CRAMER/GAUSSIAN SOLVER CORTEX!):
        // Lives on FREE orthogonal quadrant Q2 of L2 PW13 (`rows 256..511, cols 0..255`, ZERO extra L2 memory)!
        Function fn_lalg41{};
        s.bind(fn_lalg41, "mind_neo_linear_algebra", Image::mind);
        Temporary d_lalgq(s.d, sizeof(NeoLinAlgQuery)), d_lalgr(s.d, sizeof(NeoLinAlgResult));
        void *lalg41_args[]{&d_nw.ptr, &d_lalgq.ptr, &d_g.ptr, &d_lalgr.ptr};
        Word lalg_tot_ed = 0;
        {
            NeoLinAlgQuery lq{}; lq.pathway = 13; lq.mode = 4;
            s.d.check(s.d.upload(d_lalgq.ptr, &lq, sizeof(lq)), "upload bench lalg clr");
            s.d.check(s.d.launch(fn_lalg41, 1, 1, 1, 256, 1, 1, 0, nullptr, lalg41_args, nullptr), "bench lalg clr");
            NeoLinAlgResult lr{}; s.d.check(s.d.download(&lr, d_lalgr.ptr, sizeof(lr)), "download bench lalg clr");
            lalg_tot_ed += lr.edits_made;
        }
        {
            const Word mats[3][6] = {
                {3u, 1u, 1u, 2u, 11u, 7u},
                {4u, 2u, 2u, 1u, 8u, 4u},
                {5u, 2u, 1u, 3u, 19u, 9u}
            };
            for (Word m_i = 0; m_i < 3u; ++m_i) {
                NeoLinAlgQuery lq{}; lq.pathway = 13; lq.mode = 0; lq.mat_id = m_i;
                for (Word f = 0; f < 6u; ++f) lq.fields[f] = mats[m_i][f];
                s.d.check(s.d.upload(d_lalgq.ptr, &lq, sizeof(lq)), "upload bench lalg mat");
                s.d.check(s.d.launch(fn_lalg41, 1, 1, 1, 256, 1, 1, 0, nullptr, lalg41_args, nullptr), "bench lalg mat");
                NeoLinAlgResult lr{}; s.d.check(s.d.download(&lr, d_lalgr.ptr, sizeof(lr)), "download bench lalg mat");
                lalg_tot_ed += lr.edits_made;
            }
        }
        NeoLinAlgQuery la_dq{}; la_dq.pathway = 13; la_dq.mode = 1; la_dq.mat_id = 0; la_dq.sub_op = 0;
        s.d.check(s.d.upload(d_lalgq.ptr, &la_dq, sizeof(la_dq)), "upload bench lalg det");
        s.d.check(s.d.launch(fn_lalg41, 1, 1, 1, 256, 1, 1, 0, nullptr, lalg41_args, nullptr), "bench lalg det");
        NeoLinAlgResult la_det{}; s.d.check(s.d.download(&la_det, d_lalgr.ptr, sizeof(la_det)), "download bench lalg det");

        NeoLinAlgQuery la_vq{}; la_vq.pathway = 13; la_vq.mode = 2; la_vq.mat_id = 0; la_vq.vx = 2; la_vq.vy = 3;
        s.d.check(s.d.upload(d_lalgq.ptr, &la_vq, sizeof(la_vq)), "upload bench lalg mv");
        s.d.check(s.d.launch(fn_lalg41, 1, 1, 1, 256, 1, 1, 0, nullptr, lalg41_args, nullptr), "bench lalg mv");
        NeoLinAlgResult la_mv{}; s.d.check(s.d.download(&la_mv, d_lalgr.ptr, sizeof(la_mv)), "download bench lalg mv");

        NeoLinAlgQuery la_lq{}; la_lq.pathway = 13; la_lq.mode = 3; la_lq.mat_id = 3; la_lq.also_imprint = 1;
        la_lq.fields[0] = 2u; la_lq.fields[1] = 1u; la_lq.fields[2] = 1u; la_lq.fields[3] = 3u; la_lq.fields[4] = 13u; la_lq.fields[5] = 14u;
        s.d.check(s.d.upload(d_lalgq.ptr, &la_lq, sizeof(la_lq)), "upload bench lalg live");
        s.d.check(s.d.launch(fn_lalg41, 1, 1, 1, 256, 1, 1, 0, nullptr, lalg41_args, nullptr), "bench lalg live");
        NeoLinAlgResult la_live{}; s.d.check(s.d.download(&la_live, d_lalgr.ptr, sizeof(la_live)), "download bench lalg live");
        lalg_tot_ed += la_live.edits_made;

        // Time Mode 3 pure-GPU Cramer linear solve (`linalpha: [[3,1],[1,2]] x = [11,7] -> x0=3, x1=2`) straight from L2 PW13 Q2!
        NeoLinAlgQuery la_sq{}; la_sq.pathway = 13; la_sq.mode = 3; la_sq.mat_id = 0; la_sq.sub_op = 0; la_sq.also_imprint = 0;
        s.d.check(s.d.upload(d_lalgq.ptr, &la_sq, sizeof(la_sq)), "upload bench lalg slv");
        s.d.check(s.d.launch(fn_lalg41, 1, 1, 1, 256, 1, 1, 0, nullptr, lalg41_args, nullptr), "warmup neo linalg");
        std::vector<double> lalg41times;
        constexpr Word LALG41BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec lalg41a");
            for (Word k = 0; k < LALG41BATCH; ++k)
                s.d.check(s.d.launch(fn_lalg41, 1, 1, 1, 256, 1, 1, 0, nullptr, lalg41_args, nullptr), "bench neo linalg");
            s.d.check(s.d.record(b, nullptr), "rec lalg41b");
            s.d.check(s.d.wait(b), "wait lalg41b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "lalg41elapsed");
            lalg41times.push_back(ms / LALG41BATCH);
        }
        std::sort(lalg41times.begin(), lalg41times.end());
        double lalg41_us = lalg41times[lalg41times.size() / 2] * 1000.0;
        NeoLinAlgResult lalg41res{};
        s.d.check(s.d.download(&lalg41res, d_lalgr.ptr, sizeof(lalg41res)), "download lalg41res");

        // Kernel 42 (STAGE 8f — FINAL FREE QUADRANT OF L2 PW13: PURE-GPU SPIKING PROBABILISTIC BAYES-NET & DECISION CORTEX!):
        // Lives on Orthogonal Quadrant Q3 of L2 PW13 (`rows 256..511, cols 256..511`, ZERO extra L2 memory)!
        Function fn_bnet42{};
        s.bind(fn_bnet42, "mind_neo_bayes_net", Image::mind);
        Temporary d_bnetq(s.d, sizeof(NeoBayesNetQuery)), d_bnetr(s.d, sizeof(NeoBayesNetResult));
        void *bnet42_args[]{&d_nw.ptr, &d_bnetq.ptr, &d_g.ptr, &d_bnetr.ptr};
        Word bnet_tot_ed = 0;
        {
            NeoBayesNetQuery bq{}; bq.pathway = 13; bq.mode = 4;
            s.d.check(s.d.upload(d_bnetq.ptr, &bq, sizeof(bq)), "upload bench bnet clr");
            s.d.check(s.d.launch(fn_bnet42, 1, 1, 1, 256, 1, 1, 0, nullptr, bnet42_args, nullptr), "bench bnet clr");
            NeoBayesNetResult br{}; s.d.check(s.d.download(&br, d_bnetr.ptr, sizeof(br)), "download bench bnet clr");
            bnet_tot_ed += br.edits_made;
        }
        {
            const Word nodes[3][6] = {
                {20u, 80u, 20u, 10u, 50u, 1u},
                {10u, 90u, 10u, 60u, 20u, 2u},
                {50u, 90u, 30u, 15u, 45u, 3u}
            };
            for (Word n_i = 0; n_i < 3u; ++n_i) {
                NeoBayesNetQuery bq{}; bq.pathway = 13; bq.mode = 0; bq.node_id = n_i;
                for (Word f = 0; f < 6u; ++f) bq.fields[f] = nodes[n_i][f];
                s.d.check(s.d.upload(d_bnetq.ptr, &bq, sizeof(bq)), "upload bench bnet node");
                s.d.check(s.d.launch(fn_bnet42, 1, 1, 1, 256, 1, 1, 0, nullptr, bnet42_args, nullptr), "bench bnet node");
                NeoBayesNetResult br{}; s.d.check(s.d.download(&br, d_bnetr.ptr, sizeof(br)), "download bench bnet node");
                bnet_tot_ed += br.edits_made;
            }
        }
        NeoBayesNetQuery bn_pq{}; bn_pq.pathway = 13; bn_pq.mode = 1; bn_pq.node_id = 0; bn_pq.sub_op = 0;
        s.d.check(s.d.upload(d_bnetq.ptr, &bn_pq, sizeof(bn_pq)), "upload bench bnet pos");
        s.d.check(s.d.launch(fn_bnet42, 1, 1, 1, 256, 1, 1, 0, nullptr, bnet42_args, nullptr), "bench bnet pos");
        NeoBayesNetResult bn_pos{}; s.d.check(s.d.download(&bn_pos, d_bnetr.ptr, sizeof(bn_pos)), "download bench bnet pos");

        NeoBayesNetQuery bn_nq{}; bn_nq.pathway = 13; bn_nq.mode = 2; bn_nq.node_id = 2; bn_nq.sub_op = 0;
        s.d.check(s.d.upload(d_bnetq.ptr, &bn_nq, sizeof(bn_nq)), "upload bench bnet neg");
        s.d.check(s.d.launch(fn_bnet42, 1, 1, 1, 256, 1, 1, 0, nullptr, bnet42_args, nullptr), "bench bnet neg");
        NeoBayesNetResult bn_neg{}; s.d.check(s.d.download(&bn_neg, d_bnetr.ptr, sizeof(bn_neg)), "download bench bnet neg");

        NeoBayesNetQuery bn_lq{}; bn_lq.pathway = 13; bn_lq.mode = 3; bn_lq.node_id = 3; bn_lq.also_imprint = 1;
        bn_lq.fields[0] = 50u; bn_lq.fields[1] = 80u; bn_lq.fields[2] = 20u; bn_lq.fields[3] = 20u; bn_lq.fields[4] = 80u; bn_lq.fields[5] = 4u;
        s.d.check(s.d.upload(d_bnetq.ptr, &bn_lq, sizeof(bn_lq)), "upload bench bnet live");
        s.d.check(s.d.launch(fn_bnet42, 1, 1, 1, 256, 1, 1, 0, nullptr, bnet42_args, nullptr), "bench bnet live");
        NeoBayesNetResult bn_live{}; s.d.check(s.d.download(&bn_live, d_bnetr.ptr, sizeof(bn_live)), "download bench bnet live");
        bnet_tot_ed += bn_live.edits_made;

        // Time Mode 3 pure-GPU Bayes optimal decision (`bnbeta` -> opt_act=0 wait, 4 active nodes!) straight from L2 PW13 Q3!
        NeoBayesNetQuery bn_dq{}; bn_dq.pathway = 13; bn_dq.mode = 3; bn_dq.node_id = 1; bn_dq.sub_op = 1; bn_dq.also_imprint = 0;
        s.d.check(s.d.upload(d_bnetq.ptr, &bn_dq, sizeof(bn_dq)), "upload bench bnet dec");
        s.d.check(s.d.launch(fn_bnet42, 1, 1, 1, 256, 1, 1, 0, nullptr, bnet42_args, nullptr), "warmup neo bnet");
        std::vector<double> bnet42times;
        constexpr Word BNET42BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec bnet42a");
            for (Word k = 0; k < BNET42BATCH; ++k)
                s.d.check(s.d.launch(fn_bnet42, 1, 1, 1, 256, 1, 1, 0, nullptr, bnet42_args, nullptr), "bench neo bnet");
            s.d.check(s.d.record(b, nullptr), "rec bnet42b");
            s.d.check(s.d.wait(b), "wait bnet42b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "bnet42elapsed");
            bnet42times.push_back(ms / BNET42BATCH);
        }
        std::sort(bnet42times.begin(), bnet42times.end());
        double bnet42_us = bnet42times[bnet42times.size() / 2] * 1000.0;
        NeoBayesNetResult bnet42res{};
        s.d.check(s.d.download(&bnet42res, d_bnetr.ptr, sizeof(bnet42res)), "download bnet42res");

        // Kernel 43 (STAGE 8g — PURE-GPU SPIKING MDP VALUE-ITERATION & STOCHASTIC POLICY PLANNER!):
        // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 256..511, cols 768..1023`, ZERO extra L2 memory)!
        Function fn_mdpp43{};
        s.bind(fn_mdpp43, "mind_neo_mdp_planner", Image::mind);
        Temporary d_mdppq(s.d, sizeof(NeoMdpPlanQuery)), d_mdppr(s.d, sizeof(NeoMdpPlanResult));
        void *mdpp43_args[]{&d_nw.ptr, &d_mdppq.ptr, &d_g.ptr, &d_mdppr.ptr};
        Word mdpp_tot_ed = 0;
        {
            NeoMdpPlanQuery mq{}; mq.pathway = 24; mq.mode = 4;
            s.d.check(s.d.upload(d_mdppq.ptr, &mq, sizeof(mq)), "upload bench mdpp clr");
            s.d.check(s.d.launch(fn_mdpp43, 1, 1, 1, 256, 1, 1, 0, nullptr, mdpp43_args, nullptr), "bench mdpp clr");
            NeoMdpPlanResult mr{}; s.d.check(s.d.download(&mr, d_mdppr.ptr, sizeof(mr)), "download bench mdpp clr");
            mdpp_tot_ed += mr.edits_made;
        }
        {
            const Word envs[3][6] = {
                {10u, 20u, 100u, 80u, 50u, 50u},
                {0u, 10u, 100u, 50u, 80u, 80u},
                {20u, 40u, 80u, 100u, 50u, 50u}
            };
            for (Word e_i = 0; e_i < 3u; ++e_i) {
                NeoMdpPlanQuery mq{}; mq.pathway = 24; mq.mode = 0; mq.env_id = e_i;
                for (Word f = 0; f < 6u; ++f) mq.fields[f] = envs[e_i][f];
                s.d.check(s.d.upload(d_mdppq.ptr, &mq, sizeof(mq)), "upload bench mdpp env");
                s.d.check(s.d.launch(fn_mdpp43, 1, 1, 1, 256, 1, 1, 0, nullptr, mdpp43_args, nullptr), "bench mdpp env");
                NeoMdpPlanResult mr{}; s.d.check(s.d.download(&mr, d_mdppr.ptr, sizeof(mr)), "download bench mdpp env");
                mdpp_tot_ed += mr.edits_made;
            }
        }
        NeoMdpPlanQuery mp_vq{}; mp_vq.pathway = 24; mp_vq.mode = 1; mp_vq.env_id = 0; mp_vq.sub_op = 0;
        s.d.check(s.d.upload(d_mdppq.ptr, &mp_vq, sizeof(mp_vq)), "upload bench mdpp val");
        s.d.check(s.d.launch(fn_mdpp43, 1, 1, 1, 256, 1, 1, 0, nullptr, mdpp43_args, nullptr), "bench mdpp val");
        NeoMdpPlanResult mp_val{}; s.d.check(s.d.download(&mp_val, d_mdppr.ptr, sizeof(mp_val)), "download bench mdpp val");

        NeoMdpPlanQuery mp_mq{}; mp_mq.pathway = 24; mp_mq.mode = 1; mp_mq.env_id = 2; mp_mq.sub_op = 1;
        s.d.check(s.d.upload(d_mdppq.ptr, &mp_mq, sizeof(mp_mq)), "upload bench mdpp mid");
        s.d.check(s.d.launch(fn_mdpp43, 1, 1, 1, 256, 1, 1, 0, nullptr, mdpp43_args, nullptr), "bench mdpp mid");
        NeoMdpPlanResult mp_mid{}; s.d.check(s.d.download(&mp_mid, d_mdppr.ptr, sizeof(mp_mid)), "download bench mdpp mid");

        NeoMdpPlanQuery mp_lq{}; mp_lq.pathway = 24; mp_lq.mode = 3; mp_lq.env_id = 3; mp_lq.also_imprint = 1;
        mp_lq.fields[0] = 5u; mp_lq.fields[1] = 10u; mp_lq.fields[2] = 100u; mp_lq.fields[3] = 40u; mp_lq.fields[4] = 90u; mp_lq.fields[5] = 50u;
        s.d.check(s.d.upload(d_mdppq.ptr, &mp_lq, sizeof(mp_lq)), "upload bench mdpp live");
        s.d.check(s.d.launch(fn_mdpp43, 1, 1, 1, 256, 1, 1, 0, nullptr, mdpp43_args, nullptr), "bench mdpp live");
        NeoMdpPlanResult mp_live{}; s.d.check(s.d.download(&mp_live, d_mdppr.ptr, sizeof(mp_live)), "download bench mdpp live");
        mdpp_tot_ed += mp_live.edits_made;

        // Time Mode 2 pure-GPU optimal stochastic policy (`mdpbeta` -> opt_act=1 mdpleap, 4 active envs!) straight from L2 PW24!
        NeoMdpPlanQuery mp_pq{}; mp_pq.pathway = 24; mp_pq.mode = 2; mp_pq.env_id = 1;
        s.d.check(s.d.upload(d_mdppq.ptr, &mp_pq, sizeof(mp_pq)), "upload bench mdpp pol");
        s.d.check(s.d.launch(fn_mdpp43, 1, 1, 1, 256, 1, 1, 0, nullptr, mdpp43_args, nullptr), "warmup neo mdpp");
        std::vector<double> mdpp43times;
        constexpr Word MDPP43BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec mdpp43a");
            for (Word k = 0; k < MDPP43BATCH; ++k)
                s.d.check(s.d.launch(fn_mdpp43, 1, 1, 1, 256, 1, 1, 0, nullptr, mdpp43_args, nullptr), "bench neo mdpp");
            s.d.check(s.d.record(b, nullptr), "rec mdpp43b");
            s.d.check(s.d.wait(b), "wait mdpp43b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "mdpp43elapsed");
            mdpp43times.push_back(ms / MDPP43BATCH);
        }
        std::sort(mdpp43times.begin(), mdpp43times.end());
        double mdpp43_us = mdpp43times[mdpp43times.size() / 2] * 1000.0;
        NeoMdpPlanResult mdpp43res{};
        s.d.check(s.d.download(&mdpp43res, d_mdppr.ptr, sizeof(mdpp43res)), "download mdpp43res");

        // Kernel 44 (STAGE 8h — PURE-GPU SPIKING KALMAN FILTER & BAYESIAN SENSOR-FUSION CORTEX!):
        // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 512..767, cols 768..1023`, ZERO extra L2 memory)!
        Function fn_kflt44{};
        s.bind(fn_kflt44, "mind_neo_kalman_filter", Image::mind);
        Temporary d_kfltq(s.d, sizeof(NeoKalmanQuery)), d_kfltr(s.d, sizeof(NeoKalmanResult));
        void *kflt44_args[]{&d_nw.ptr, &d_kfltq.ptr, &d_g.ptr, &d_kfltr.ptr};
        Word kflt_tot_ed = 0;
        {
            NeoKalmanQuery kq{}; kq.pathway = 24; kq.mode = 4;
            s.d.check(s.d.upload(d_kfltq.ptr, &kq, sizeof(kq)), "upload bench kflt clr");
            s.d.check(s.d.launch(fn_kflt44, 1, 1, 1, 256, 1, 1, 0, nullptr, kflt44_args, nullptr), "bench kflt clr");
            NeoKalmanResult kr{}; s.d.check(s.d.download(&kr, d_kfltr.ptr, sizeof(kr)), "download bench kflt clr");
            kflt_tot_ed += kr.edits_made;
        }
        {
            const Word chans[3][6] = {
                {20u, 30u, 10u, 40u, 60u, 10u},
                {50u, 10u, 10u, 60u, 90u, 0u},
                {10u, 40u, 20u, 20u, 90u, 0u}
            };
            for (Word c_i = 0; c_i < 3u; ++c_i) {
                NeoKalmanQuery kq{}; kq.pathway = 24; kq.mode = 0; kq.chan_id = c_i;
                for (Word f = 0; f < 6u; ++f) kq.fields[f] = chans[c_i][f];
                s.d.check(s.d.upload(d_kfltq.ptr, &kq, sizeof(kq)), "upload bench kflt chan");
                s.d.check(s.d.launch(fn_kflt44, 1, 1, 1, 256, 1, 1, 0, nullptr, kflt44_args, nullptr), "bench kflt chan");
                NeoKalmanResult kr{}; s.d.check(s.d.download(&kr, d_kfltr.ptr, sizeof(kr)), "download bench kflt chan");
                kflt_tot_ed += kr.edits_made;
            }
        }
        NeoKalmanQuery kf_gq{}; kf_gq.pathway = 24; kf_gq.mode = 1; kf_gq.chan_id = 0; kf_gq.sub_op = 0;
        s.d.check(s.d.upload(d_kfltq.ptr, &kf_gq, sizeof(kf_gq)), "upload bench kflt gain");
        s.d.check(s.d.launch(fn_kflt44, 1, 1, 1, 256, 1, 1, 0, nullptr, kflt44_args, nullptr), "bench kflt gain");
        NeoKalmanResult kf_gain{}; s.d.check(s.d.download(&kf_gain, d_kfltr.ptr, sizeof(kf_gain)), "download bench kflt gain");

        NeoKalmanQuery kf_pq{}; kf_pq.pathway = 24; kf_pq.mode = 2; kf_pq.chan_id = 2; kf_pq.sub_op = 0;
        s.d.check(s.d.upload(d_kfltq.ptr, &kf_pq, sizeof(kf_pq)), "upload bench kflt post");
        s.d.check(s.d.launch(fn_kflt44, 1, 1, 1, 256, 1, 1, 0, nullptr, kflt44_args, nullptr), "bench kflt post");
        NeoKalmanResult kf_post{}; s.d.check(s.d.download(&kf_post, d_kfltr.ptr, sizeof(kf_post)), "download bench kflt post");

        NeoKalmanQuery kf_lq{}; kf_lq.pathway = 24; kf_lq.mode = 3; kf_lq.chan_id = 3; kf_lq.also_imprint = 1;
        kf_lq.fields[0] = 40u; kf_lq.fields[1] = 30u; kf_lq.fields[2] = 10u; kf_lq.fields[3] = 10u; kf_lq.fields[4] = 90u; kf_lq.fields[5] = 0u;
        s.d.check(s.d.upload(d_kfltq.ptr, &kf_lq, sizeof(kf_lq)), "upload bench kflt live");
        s.d.check(s.d.launch(fn_kflt44, 1, 1, 1, 256, 1, 1, 0, nullptr, kflt44_args, nullptr), "bench kflt live");
        NeoKalmanResult kf_live{}; s.d.check(s.d.download(&kf_live, d_kfltr.ptr, sizeof(kf_live)), "download bench kflt live");
        kflt_tot_ed += kf_live.edits_made;

        // Time Mode 2 pure-GPU Kalman trust regime (`kfbeta` -> trust_meas=0 kfprior, 4 active chans!) straight from L2 PW24!
        NeoKalmanQuery kf_mq{}; kf_mq.pathway = 24; kf_mq.mode = 2; kf_mq.chan_id = 1; kf_mq.sub_op = 1;
        s.d.check(s.d.upload(d_kfltq.ptr, &kf_mq, sizeof(kf_mq)), "upload bench kflt mode");
        s.d.check(s.d.launch(fn_kflt44, 1, 1, 1, 256, 1, 1, 0, nullptr, kflt44_args, nullptr), "warmup neo kflt");
        std::vector<double> kflt44times;
        constexpr Word KFLT44BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec kflt44a");
            for (Word k = 0; k < KFLT44BATCH; ++k)
                s.d.check(s.d.launch(fn_kflt44, 1, 1, 1, 256, 1, 1, 0, nullptr, kflt44_args, nullptr), "bench neo kflt");
            s.d.check(s.d.record(b, nullptr), "rec kflt44b");
            s.d.check(s.d.wait(b), "wait kflt44b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "kflt44elapsed");
            kflt44times.push_back(ms / KFLT44BATCH);
        }
        std::sort(kflt44times.begin(), kflt44times.end());
        double kflt44_us = kflt44times[kflt44times.size() / 2] * 1000.0;
        NeoKalmanResult kflt44res{};
        s.d.check(s.d.download(&kflt44res, d_kfltr.ptr, sizeof(kflt44res)), "download kflt44res");

        // Kernel 45 (STAGE 8i — PURE-GPU SPIKING LQR / RICCATI FEEDBACK CONTROL & STABILITY CORTEX!):
        // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 0..255, cols 512..767`, ZERO extra L2 memory)!
        Function fn_lqrc45{};
        s.bind(fn_lqrc45, "mind_neo_lqr_control", Image::mind);
        Temporary d_lqrcq(s.d, sizeof(NeoLqrQuery)), d_lqrcr(s.d, sizeof(NeoLqrResult));
        void *lqrc45_args[]{&d_nw.ptr, &d_lqrcq.ptr, &d_g.ptr, &d_lqrcr.ptr};
        Word lqrc_tot_ed = 0;
        {
            NeoLqrQuery lq{}; lq.pathway = 24; lq.mode = 4;
            s.d.check(s.d.upload(d_lqrcq.ptr, &lq, sizeof(lq)), "upload bench lqrc clr");
            s.d.check(s.d.launch(fn_lqrc45, 1, 1, 1, 256, 1, 1, 0, nullptr, lqrc45_args, nullptr), "bench lqrc clr");
            NeoLqrResult lr{}; s.d.check(s.d.download(&lr, d_lqrcr.ptr, sizeof(lr)), "download bench lqrc clr");
            lqrc_tot_ed += lr.edits_made;
        }
        {
            const Word plants[3][6] = {
                {100u, 100u, 10u, 10u, 50u, 0u},
                {120u, 100u, 20u, 10u, 40u, 0u},
                {120u, 10u,  10u, 100u, 50u, 0u}
            };
            for (Word p_i = 0; p_i < 3u; ++p_i) {
                NeoLqrQuery lq{}; lq.pathway = 24; lq.mode = 0; lq.plant_id = p_i;
                for (Word f = 0; f < 6u; ++f) lq.fields[f] = plants[p_i][f];
                s.d.check(s.d.upload(d_lqrcq.ptr, &lq, sizeof(lq)), "upload bench lqrc plant");
                s.d.check(s.d.launch(fn_lqrc45, 1, 1, 1, 256, 1, 1, 0, nullptr, lqrc45_args, nullptr), "bench lqrc plant");
                NeoLqrResult lr{}; s.d.check(s.d.download(&lr, d_lqrcr.ptr, sizeof(lr)), "download bench lqrc plant");
                lqrc_tot_ed += lr.edits_made;
            }
        }
        NeoLqrQuery lq_gq{}; lq_gq.pathway = 24; lq_gq.mode = 1; lq_gq.plant_id = 0; lq_gq.sub_op = 0;
        s.d.check(s.d.upload(d_lqrcq.ptr, &lq_gq, sizeof(lq_gq)), "upload bench lqrc gain");
        s.d.check(s.d.launch(fn_lqrc45, 1, 1, 1, 256, 1, 1, 0, nullptr, lqrc45_args, nullptr), "bench lqrc gain");
        NeoLqrResult lq_gain{}; s.d.check(s.d.download(&lq_gain, d_lqrcr.ptr, sizeof(lq_gain)), "download bench lqrc gain");

        NeoLqrQuery lq_pq{}; lq_pq.pathway = 24; lq_pq.mode = 1; lq_pq.plant_id = 1; lq_pq.sub_op = 1;
        s.d.check(s.d.upload(d_lqrcq.ptr, &lq_pq, sizeof(lq_pq)), "upload bench lqrc pole");
        s.d.check(s.d.launch(fn_lqrc45, 1, 1, 1, 256, 1, 1, 0, nullptr, lqrc45_args, nullptr), "bench lqrc pole");
        NeoLqrResult lq_pole{}; s.d.check(s.d.download(&lq_pole, d_lqrcr.ptr, sizeof(lq_pole)), "download bench lqrc pole");

        NeoLqrQuery lq_lq{}; lq_lq.pathway = 24; lq_lq.mode = 3; lq_lq.plant_id = 3; lq_lq.also_imprint = 1;
        lq_lq.fields[0] = 80u; lq_lq.fields[1] = 100u; lq_lq.fields[2] = 30u; lq_lq.fields[3] = 10u; lq_lq.fields[4] = 60u; lq_lq.fields[5] = 0u;
        s.d.check(s.d.upload(d_lqrcq.ptr, &lq_lq, sizeof(lq_lq)), "upload bench lqrc live");
        s.d.check(s.d.launch(fn_lqrc45, 1, 1, 1, 256, 1, 1, 0, nullptr, lqrc45_args, nullptr), "bench lqrc live");
        NeoLqrResult lq_live{}; s.d.check(s.d.download(&lq_live, d_lqrcr.ptr, sizeof(lq_live)), "download bench lqrc live");
        lqrc_tot_ed += lq_live.edits_made;

        // Time Mode 2 pure-GPU stability check (`lqgamma` -> is_stable=0 lqunst, 4 active plants!) straight from L2 PW24!
        NeoLqrQuery lq_mq{}; lq_mq.pathway = 24; lq_mq.mode = 2; lq_mq.plant_id = 2; lq_mq.sub_op = 1;
        s.d.check(s.d.upload(d_lqrcq.ptr, &lq_mq, sizeof(lq_mq)), "upload bench lqrc mode");
        s.d.check(s.d.launch(fn_lqrc45, 1, 1, 1, 256, 1, 1, 0, nullptr, lqrc45_args, nullptr), "warmup neo lqrc");
        std::vector<double> lqrc45times;
        constexpr Word LQRC45BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec lqrc45a");
            for (Word k = 0; k < LQRC45BATCH; ++k)
                s.d.check(s.d.launch(fn_lqrc45, 1, 1, 1, 256, 1, 1, 0, nullptr, lqrc45_args, nullptr), "bench neo lqrc");
            s.d.check(s.d.record(b, nullptr), "rec lqrc45b");
            s.d.check(s.d.wait(b), "wait lqrc45b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "lqrc45elapsed");
            lqrc45times.push_back(ms / LQRC45BATCH);
        }
        std::sort(lqrc45times.begin(), lqrc45times.end());
        double lqrc45_us = lqrc45times[lqrc45times.size() / 2] * 1000.0;
        NeoLqrResult lqrc45res{};
        s.d.check(s.d.download(&lqrc45res, d_lqrcr.ptr, sizeof(lqrc45res)), "download lqrc45res");

        // Kernel 46 (STAGE 8j — 500-DOMAIN MILESTONE! PURE-GPU 256-THREAD SPIKING 8-POINT FFT SPECTRUM CORTEX!):
        // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 0..255, cols 256..511`, ZERO extra L2 memory)!
        Function fn_ffts46{};
        s.bind(fn_ffts46, "mind_neo_fft_spectrum", Image::mind);
        Temporary d_fftsq(s.d, sizeof(NeoFftSpecQuery)), d_fftsr(s.d, sizeof(NeoFftSpecResult));
        void *ffts46_args[]{&d_nw.ptr, &d_fftsq.ptr, &d_g.ptr, &d_fftsr.ptr};
        Word ffts_tot_ed = 0;
        {
            NeoFftSpecQuery fq{}; fq.pathway = 24; fq.mode = 4;
            s.d.check(s.d.upload(d_fftsq.ptr, &fq, sizeof(fq)), "upload bench ffts clr");
            s.d.check(s.d.launch(fn_ffts46, 1, 1, 1, 256, 1, 1, 0, nullptr, ffts46_args, nullptr), "bench ffts clr");
            NeoFftSpecResult fr{}; s.d.check(s.d.download(&fr, d_fftsr.ptr, sizeof(fr)), "download bench ffts clr");
            ffts_tot_ed += fr.edits_made;
        }
        {
            const Word sigs[3][8] = {
                {50u, 78u, 90u, 78u, 50u, 22u, 10u, 22u},
                {80u, 20u, 80u, 20u, 80u, 20u, 80u, 20u},
                {100u, 86u, 50u, 14u, 0u, 14u, 50u, 86u}
            };
            for (Word s_i = 0; s_i < 3u; ++s_i) {
                NeoFftSpecQuery fq{}; fq.pathway = 24; fq.mode = 0; fq.sig_id = s_i;
                for (Word n = 0; n < 8u; ++n) fq.samples[n] = sigs[s_i][n];
                s.d.check(s.d.upload(d_fftsq.ptr, &fq, sizeof(fq)), "upload bench ffts sig");
                s.d.check(s.d.launch(fn_ffts46, 1, 1, 1, 256, 1, 1, 0, nullptr, ffts46_args, nullptr), "bench ffts sig");
                NeoFftSpecResult fr{}; s.d.check(s.d.download(&fr, d_fftsr.ptr, sizeof(fr)), "download bench ffts sig");
                ffts_tot_ed += fr.edits_made;
            }
        }
        NeoFftSpecQuery ff_hq{}; ff_hq.pathway = 24; ff_hq.mode = 1; ff_hq.sig_id = 0; ff_hq.sub_op = 1;
        s.d.check(s.d.upload(d_fftsq.ptr, &ff_hq, sizeof(ff_hq)), "upload bench ffts h1");
        s.d.check(s.d.launch(fn_ffts46, 1, 1, 1, 256, 1, 1, 0, nullptr, ffts46_args, nullptr), "bench ffts h1");
        NeoFftSpecResult ff_h1{}; s.d.check(s.d.download(&ff_h1, d_fftsr.ptr, sizeof(ff_h1)), "download bench ffts h1");

        NeoFftSpecQuery ff_nq{}; ff_nq.pathway = 24; ff_nq.mode = 2; ff_nq.sig_id = 1; ff_nq.sub_op = 0;
        s.d.check(s.d.upload(d_fftsq.ptr, &ff_nq, sizeof(ff_nq)), "upload bench ffts nyq");
        s.d.check(s.d.launch(fn_ffts46, 1, 1, 1, 256, 1, 1, 0, nullptr, ffts46_args, nullptr), "bench ffts nyq");
        NeoFftSpecResult ff_nyq{}; s.d.check(s.d.download(&ff_nyq, d_fftsr.ptr, sizeof(ff_nyq)), "download bench ffts nyq");

        NeoFftSpecQuery ff_lq{}; ff_lq.pathway = 24; ff_lq.mode = 3; ff_lq.sig_id = 3; ff_lq.also_imprint = 1;
        const Word slive[8] = {90u, 10u, 90u, 10u, 90u, 10u, 90u, 10u};
        for (Word n = 0; n < 8u; ++n) ff_lq.samples[n] = slive[n];
        s.d.check(s.d.upload(d_fftsq.ptr, &ff_lq, sizeof(ff_lq)), "upload bench ffts live");
        s.d.check(s.d.launch(fn_ffts46, 1, 1, 1, 256, 1, 1, 0, nullptr, ffts46_args, nullptr), "bench ffts live");
        NeoFftSpecResult ff_live{}; s.d.check(s.d.download(&ff_live, d_fftsr.ptr, sizeof(ff_live)), "download bench ffts live");
        ffts_tot_ed += ff_live.edits_made;

        // Time Mode 2 pure-GPU spectral band classification (`ffbeta` -> dom_high=1 ffhigh, 4 active signals!) straight from L2 PW24!
        NeoFftSpecQuery ff_bq{}; ff_bq.pathway = 24; ff_bq.mode = 2; ff_bq.sig_id = 1; ff_bq.sub_op = 1;
        s.d.check(s.d.upload(d_fftsq.ptr, &ff_bq, sizeof(ff_bq)), "upload bench ffts band");
        s.d.check(s.d.launch(fn_ffts46, 1, 1, 1, 256, 1, 1, 0, nullptr, ffts46_args, nullptr), "warmup neo ffts");
        std::vector<double> ffts46times;
        constexpr Word FFTS46BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec ffts46a");
            for (Word k = 0; k < FFTS46BATCH; ++k)
                s.d.check(s.d.launch(fn_ffts46, 1, 1, 1, 256, 1, 1, 0, nullptr, ffts46_args, nullptr), "bench neo ffts");
            s.d.check(s.d.record(b, nullptr), "rec ffts46b");
            s.d.check(s.d.wait(b), "wait ffts46b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "ffts46elapsed");
            ffts46times.push_back(ms / FFTS46BATCH);
        }
        std::sort(ffts46times.begin(), ffts46times.end());
        double ffts46_us = ffts46times[ffts46times.size() / 2] * 1000.0;
        NeoFftSpecResult ffts46res{};
        s.d.check(s.d.download(&ffts46res, d_fftsr.ptr, sizeof(ffts46res)), "download ffts46res");

        // Kernel 47 (STAGE 8k — 510 DOMAINS! PURE-GPU 256-THREAD SPIKING 4TH-ORDER RK4 ODE INTEGRATOR CORTEX!):
        // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 256..511, cols 0..255`, ZERO extra L2 memory)!
        Function fn_odrk47{};
        s.bind(fn_odrk47, "mind_neo_ode_rk4", Image::mind);
        Temporary d_odrkq(s.d, sizeof(NeoOdeRk4Query)), d_odrkr(s.d, sizeof(NeoOdeRk4Result));
        void *odrk47_args[]{&d_nw.ptr, &d_odrkq.ptr, &d_g.ptr, &d_odrkr.ptr};
        Word odrk_tot_ed = 0;
        {
            NeoOdeRk4Query oq{}; oq.pathway = 24; oq.mode = 4;
            s.d.check(s.d.upload(d_odrkq.ptr, &oq, sizeof(oq)), "upload bench odrk clr");
            s.d.check(s.d.launch(fn_odrk47, 1, 1, 1, 256, 1, 1, 0, nullptr, odrk47_args, nullptr), "bench odrk clr");
            NeoOdeRk4Result orr{}; s.d.check(s.d.download(&orr, d_odrkr.ptr, sizeof(orr)), "download bench odrk clr");
            odrk_tot_ed += orr.edits_made;
        }
        {
            const Word syss[3][6] = {
                {1u, 50u, 20u, 100u, 50u, 4u},
                {0u, 50u,  0u,  20u, 50u, 4u},
                {1u, 100u, 50u, 10u, 50u, 4u}
            };
            for (Word s_i = 0; s_i < 3u; ++s_i) {
                NeoOdeRk4Query oq{}; oq.pathway = 24; oq.mode = 0; oq.sys_id = s_i;
                for (Word n = 0; n < 6u; ++n) oq.params[n] = syss[s_i][n];
                s.d.check(s.d.upload(d_odrkq.ptr, &oq, sizeof(oq)), "upload bench odrk sys");
                s.d.check(s.d.launch(fn_odrk47, 1, 1, 1, 256, 1, 1, 0, nullptr, odrk47_args, nullptr), "bench odrk sys");
                NeoOdeRk4Result orr{}; s.d.check(s.d.download(&orr, d_odrkr.ptr, sizeof(orr)), "download bench odrk sys");
                odrk_tot_ed += orr.edits_made;
            }
        }
        NeoOdeRk4Query od_vq{}; od_vq.pathway = 24; od_vq.mode = 1; od_vq.sys_id = 0; od_vq.sub_op = 0;
        s.d.check(s.d.upload(d_odrkq.ptr, &od_vq, sizeof(od_vq)), "upload bench odrk val");
        s.d.check(s.d.launch(fn_odrk47, 1, 1, 1, 256, 1, 1, 0, nullptr, odrk47_args, nullptr), "bench odrk val");
        NeoOdeRk4Result od_val{}; s.d.check(s.d.download(&od_val, d_odrkr.ptr, sizeof(od_val)), "download bench odrk val");

        NeoOdeRk4Query od_eqq{}; od_eqq.pathway = 24; od_eqq.mode = 1; od_eqq.sys_id = 2; od_eqq.sub_op = 1;
        s.d.check(s.d.upload(d_odrkq.ptr, &od_eqq, sizeof(od_eqq)), "upload bench odrk eq");
        s.d.check(s.d.launch(fn_odrk47, 1, 1, 1, 256, 1, 1, 0, nullptr, odrk47_args, nullptr), "bench odrk eq");
        NeoOdeRk4Result od_eq{}; s.d.check(s.d.download(&od_eq, d_odrkr.ptr, sizeof(od_eq)), "download bench odrk eq");

        NeoOdeRk4Query od_lq{}; od_lq.pathway = 24; od_lq.mode = 3; od_lq.sys_id = 3; od_lq.also_imprint = 1;
        const Word olive[6] = {1u, 50u, 40u, 20u, 50u, 4u};
        for (Word n = 0; n < 6u; ++n) od_lq.params[n] = olive[n];
        s.d.check(s.d.upload(d_odrkq.ptr, &od_lq, sizeof(od_lq)), "upload bench odrk live");
        s.d.check(s.d.launch(fn_odrk47, 1, 1, 1, 256, 1, 1, 0, nullptr, odrk47_args, nullptr), "bench odrk live");
        NeoOdeRk4Result od_live{}; s.d.check(s.d.download(&od_live, d_odrkr.ptr, sizeof(od_live)), "download bench odrk live");
        odrk_tot_ed += od_live.edits_made;

        // Time Mode 2 pure-GPU stability verdict (`odbeta` -> grows=1 odegrow, 4 active systems!) straight from L2 PW24!
        NeoOdeRk4Query od_mq{}; od_mq.pathway = 24; od_mq.mode = 2; od_mq.sys_id = 1;
        s.d.check(s.d.upload(d_odrkq.ptr, &od_mq, sizeof(od_mq)), "upload bench odrk mode");
        s.d.check(s.d.launch(fn_odrk47, 1, 1, 1, 256, 1, 1, 0, nullptr, odrk47_args, nullptr), "warmup neo odrk");
        std::vector<double> odrk47times;
        constexpr Word ODRK47BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec odrk47a");
            for (Word k = 0; k < ODRK47BATCH; ++k)
                s.d.check(s.d.launch(fn_odrk47, 1, 1, 1, 256, 1, 1, 0, nullptr, odrk47_args, nullptr), "bench neo odrk");
            s.d.check(s.d.record(b, nullptr), "rec odrk47b");
            s.d.check(s.d.wait(b), "wait odrk47b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "odrk47elapsed");
            odrk47times.push_back(ms / ODRK47BATCH);
        }
        std::sort(odrk47times.begin(), odrk47times.end());
        double odrk47_us = odrk47times[odrk47times.size() / 2] * 1000.0;
        NeoOdeRk4Result odrk47res{};
        s.d.check(s.d.download(&odrk47res, d_odrkr.ptr, sizeof(odrk47res)), "download odrk47res");

        // Kernel 48 (STAGE 8l — 520 DOMAINS! PURE-GPU 256-THREAD SPIKING SIMPLEX LP OPTIMIZER CORTEX!):
        // Lives on FREE Orthogonal Quadrant of L2 PW24 (`rows 768..1023, cols 512..767`, ZERO extra L2 memory)!
        Function fn_sxlp48{};
        s.bind(fn_sxlp48, "mind_neo_simplex_lp", Image::mind);
        Temporary d_sxlpq(s.d, sizeof(NeoSimplexLpQuery)), d_sxlpr(s.d, sizeof(NeoSimplexLpResult));
        void *sxlp48_args[]{&d_nw.ptr, &d_sxlpq.ptr, &d_g.ptr, &d_sxlpr.ptr};
        Word sxlp_tot_ed = 0;
        {
            NeoSimplexLpQuery sq{}; sq.pathway = 24; sq.mode = 4;
            s.d.check(s.d.upload(d_sxlpq.ptr, &sq, sizeof(sq)), "upload bench sxlp clr");
            s.d.check(s.d.launch(fn_sxlp48, 1, 1, 1, 256, 1, 1, 0, nullptr, sxlp48_args, nullptr), "bench sxlp clr");
            NeoSimplexLpResult srr{}; s.d.check(s.d.download(&srr, d_sxlpr.ptr, sizeof(srr)), "download bench sxlp clr");
            sxlp_tot_ed += srr.edits_made;
        }
        {
            const Word probs[3][8] = {
                {3u, 5u, 1u, 2u, 20u, 3u, 1u, 30u},
                {8u, 2u, 2u, 1u, 20u, 1u, 3u, 25u},
                {4u, 6u, 2u, 1u, 18u, 1u, 2u, 18u}
            };
            for (Word p_i = 0; p_i < 3u; ++p_i) {
                NeoSimplexLpQuery sq{}; sq.pathway = 24; sq.mode = 0; sq.prob_id = p_i;
                for (Word n = 0; n < 8u; ++n) sq.params[n] = probs[p_i][n];
                s.d.check(s.d.upload(d_sxlpq.ptr, &sq, sizeof(sq)), "upload bench sxlp prob");
                s.d.check(s.d.launch(fn_sxlp48, 1, 1, 1, 256, 1, 1, 0, nullptr, sxlp48_args, nullptr), "bench sxlp prob");
                NeoSimplexLpResult srr{}; s.d.check(s.d.download(&srr, d_sxlpr.ptr, sizeof(srr)), "download bench sxlp prob");
                sxlp_tot_ed += srr.edits_made;
            }
        }
        NeoSimplexLpQuery sx_zq{}; sx_zq.pathway = 24; sx_zq.mode = 1; sx_zq.prob_id = 0; sx_zq.sub_op = 0;
        s.d.check(s.d.upload(d_sxlpq.ptr, &sx_zq, sizeof(sx_zq)), "upload bench sxlp opt");
        s.d.check(s.d.launch(fn_sxlp48, 1, 1, 1, 256, 1, 1, 0, nullptr, sxlp48_args, nullptr), "bench sxlp opt");
        NeoSimplexLpResult sx_opt{}; s.d.check(s.d.download(&sx_opt, d_sxlpr.ptr, sizeof(sx_opt)), "download bench sxlp opt");

        NeoSimplexLpQuery sx_xq{}; sx_xq.pathway = 24; sx_xq.mode = 1; sx_xq.prob_id = 2; sx_xq.sub_op = 1;
        s.d.check(s.d.upload(d_sxlpq.ptr, &sx_xq, sizeof(sx_xq)), "upload bench sxlp xval");
        s.d.check(s.d.launch(fn_sxlp48, 1, 1, 1, 256, 1, 1, 0, nullptr, sxlp48_args, nullptr), "bench sxlp xval");
        NeoSimplexLpResult sx_xval{}; s.d.check(s.d.download(&sx_xval, d_sxlpr.ptr, sizeof(sx_xval)), "download bench sxlp xval");

        NeoSimplexLpQuery sx_lq{}; sx_lq.pathway = 24; sx_lq.mode = 3; sx_lq.prob_id = 3; sx_lq.also_imprint = 1;
        const Word sxlive_p[8] = {5u, 4u, 2u, 1u, 20u, 1u, 2u, 16u};
        for (Word n = 0; n < 8u; ++n) sx_lq.params[n] = sxlive_p[n];
        s.d.check(s.d.upload(d_sxlpq.ptr, &sx_lq, sizeof(sx_lq)), "upload bench sxlp live");
        s.d.check(s.d.launch(fn_sxlp48, 1, 1, 1, 256, 1, 1, 0, nullptr, sxlp48_args, nullptr), "bench sxlp live");
        NeoSimplexLpResult sx_live{}; s.d.check(s.d.download(&sx_live, d_sxlpr.ptr, sizeof(sx_live)), "download bench sxlp live");
        sxlp_tot_ed += sx_live.edits_made;

        // Time Mode 2 pure-GPU dual/corner binding verdict (`sxbeta` -> corner=1 sxcorn, 4 active probs!) straight from L2 PW24!
        NeoSimplexLpQuery sx_mq{}; sx_mq.pathway = 24; sx_mq.mode = 2; sx_mq.prob_id = 1;
        s.d.check(s.d.upload(d_sxlpq.ptr, &sx_mq, sizeof(sx_mq)), "upload bench sxlp bind");
        s.d.check(s.d.launch(fn_sxlp48, 1, 1, 1, 256, 1, 1, 0, nullptr, sxlp48_args, nullptr), "warmup neo sxlp");
        std::vector<double> sxlp48times;
        constexpr Word SXLP48BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec sxlp48a");
            for (Word k = 0; k < SXLP48BATCH; ++k)
                s.d.check(s.d.launch(fn_sxlp48, 1, 1, 1, 256, 1, 1, 0, nullptr, sxlp48_args, nullptr), "bench neo sxlp");
            s.d.check(s.d.record(b, nullptr), "rec sxlp48b");
            s.d.check(s.d.wait(b), "wait sxlp48b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "sxlp48elapsed");
            sxlp48times.push_back(ms / SXLP48BATCH);
        }
        std::sort(sxlp48times.begin(), sxlp48times.end());
        double sxlp48_us = sxlp48times[sxlp48times.size() / 2] * 1000.0;
        NeoSimplexLpResult sxlp48res{};
        s.d.check(s.d.download(&sxlp48res, d_sxlpr.ptr, sizeof(sxlp48res)), "download sxlp48res");

        // Kernel 49 (STAGE 8m — 530 DOMAINS! PURE-GPU 256-THREAD SPIKING PCA EIGENDECOMPOSITION & POWER-ITERATION CORTEX!):
        // Lives on FREE Orthogonal Quadrant of L2 PW27 (`rows 0..255, cols 256..511`, ZERO extra L2 memory)!
        Function fn_pcae49{};
        s.bind(fn_pcae49, "mind_neo_pca_eigen", Image::mind);
        Temporary d_pcaeq(s.d, sizeof(NeoPcaEigenQuery)), d_pcaer(s.d, sizeof(NeoPcaEigenResult));
        void *pcae49_args[]{&d_nw.ptr, &d_pcaeq.ptr, &d_g.ptr, &d_pcaer.ptr};
        Word pcae_tot_ed = 0;
        {
            NeoPcaEigenQuery pq{}; pq.pathway = 27; pq.mode = 4;
            s.d.check(s.d.upload(d_pcaeq.ptr, &pq, sizeof(pq)), "upload bench pcae clr");
            s.d.check(s.d.launch(fn_pcae49, 1, 1, 1, 256, 1, 1, 0, nullptr, pcae49_args, nullptr), "bench pcae clr");
            NeoPcaEigenResult prr{}; s.d.check(s.d.download(&prr, d_pcaer.ptr, sizeof(prr)), "download bench pcae clr");
            pcae_tot_ed += prr.edits_made;
        }
        {
            const Word mats[3][4] = {
                {13u, 4u, 7u, 0u},
                {14u, 2u, 11u, 0u},
                {31u, 8u, 19u, 0u}
            };
            for (Word m_i = 0; m_i < 3u; ++m_i) {
                NeoPcaEigenQuery pq{}; pq.pathway = 27; pq.mode = 0; pq.mat_id = m_i;
                for (Word n = 0; n < 4u; ++n) pq.params[n] = mats[m_i][n];
                s.d.check(s.d.upload(d_pcaeq.ptr, &pq, sizeof(pq)), "upload bench pcae mat");
                s.d.check(s.d.launch(fn_pcae49, 1, 1, 1, 256, 1, 1, 0, nullptr, pcae49_args, nullptr), "bench pcae mat");
                NeoPcaEigenResult prr{}; s.d.check(s.d.download(&prr, d_pcaer.ptr, sizeof(prr)), "download bench pcae mat");
                pcae_tot_ed += prr.edits_made;
            }
        }
        NeoPcaEigenQuery pc_lq1{}; pc_lq1.pathway = 27; pc_lq1.mode = 1; pc_lq1.mat_id = 0; pc_lq1.sub_op = 0;
        s.d.check(s.d.upload(d_pcaeq.ptr, &pc_lq1, sizeof(pc_lq1)), "upload bench pcae lam1");
        s.d.check(s.d.launch(fn_pcae49, 1, 1, 1, 256, 1, 1, 0, nullptr, pcae49_args, nullptr), "bench pcae lam1");
        NeoPcaEigenResult pc_lam{}; s.d.check(s.d.download(&pc_lam, d_pcaer.ptr, sizeof(pc_lam)), "download bench pcae lam1");

        NeoPcaEigenQuery pc_vq{}; pc_vq.pathway = 27; pc_vq.mode = 1; pc_vq.mat_id = 2; pc_vq.sub_op = 1;
        s.d.check(s.d.upload(d_pcaeq.ptr, &pc_vq, sizeof(pc_vq)), "upload bench pcae vpct");
        s.d.check(s.d.launch(fn_pcae49, 1, 1, 1, 256, 1, 1, 0, nullptr, pcae49_args, nullptr), "bench pcae vpct");
        NeoPcaEigenResult pc_var{}; s.d.check(s.d.download(&pc_var, d_pcaer.ptr, sizeof(pc_var)), "download bench pcae vpct");

        NeoPcaEigenQuery pc_liveq{}; pc_liveq.pathway = 27; pc_liveq.mode = 3; pc_liveq.mat_id = 3; pc_liveq.also_imprint = 1;
        pc_liveq.params[0] = 28u; pc_liveq.params[1] = 6u; pc_liveq.params[2] = 12u; pc_liveq.params[3] = 0u;
        s.d.check(s.d.upload(d_pcaeq.ptr, &pc_liveq, sizeof(pc_liveq)), "upload bench pcae live");
        s.d.check(s.d.launch(fn_pcae49, 1, 1, 1, 256, 1, 1, 0, nullptr, pcae49_args, nullptr), "bench pcae live");
        NeoPcaEigenResult pc_live{}; s.d.check(s.d.download(&pc_live, d_pcaer.ptr, sizeof(pc_live)), "download bench pcae live");
        pcae_tot_ed += pc_live.edits_made;

        // Time Mode 2 pure-GPU isotropy verdict (`pcbeta` -> iso=1 pciso, 4 active mats!) straight from L2 PW27!
        NeoPcaEigenQuery pc_mq{}; pc_mq.pathway = 27; pc_mq.mode = 2; pc_mq.mat_id = 1;
        s.d.check(s.d.upload(d_pcaeq.ptr, &pc_mq, sizeof(pc_mq)), "upload bench pcae iso");
        s.d.check(s.d.launch(fn_pcae49, 1, 1, 1, 256, 1, 1, 0, nullptr, pcae49_args, nullptr), "warmup neo pcae");
        std::vector<double> pcae49times;
        constexpr Word PCAE49BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec pcae49a");
            for (Word k = 0; k < PCAE49BATCH; ++k)
                s.d.check(s.d.launch(fn_pcae49, 1, 1, 1, 256, 1, 1, 0, nullptr, pcae49_args, nullptr), "bench neo pcae");
            s.d.check(s.d.record(b, nullptr), "rec pcae49b");
            s.d.check(s.d.wait(b), "wait pcae49b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "pcae49elapsed");
            pcae49times.push_back(ms / PCAE49BATCH);
        }
        std::sort(pcae49times.begin(), pcae49times.end());
        double pcae49_us = pcae49times[pcae49times.size() / 2] * 1000.0;
        NeoPcaEigenResult pcae49res{};
        s.d.check(s.d.download(&pcae49res, d_pcaer.ptr, sizeof(pcae49res)), "download pcae49res");

        // Kernel 50 (STAGE 8n — 50-BLACKWELL-KERNEL MILESTONE & 540 DOMAINS! PURE-GPU 256-THREAD SPIKING 1D HEAT/DIFFUSION PDE SOLVER CORTEX!):
        // Lives on FREE Orthogonal Quadrant of L2 PW27 (`rows 0..255, cols 512..767`, ZERO extra L2 memory)!
        Function fn_pdes50{};
        s.bind(fn_pdes50, "mind_neo_pde_solver", Image::mind);
        Temporary d_pdesq(s.d, sizeof(NeoPdeSolverQuery)), d_pdesr(s.d, sizeof(NeoPdeSolverResult));
        void *pdes50_args[]{&d_nw.ptr, &d_pdesq.ptr, &d_g.ptr, &d_pdesr.ptr};
        Word pdes_tot_ed = 0;
        {
            NeoPdeSolverQuery pq{}; pq.pathway = 27; pq.mode = 4;
            s.d.check(s.d.upload(d_pdesq.ptr, &pq, sizeof(pq)), "upload bench pdes clr");
            s.d.check(s.d.launch(fn_pdes50, 1, 1, 1, 256, 1, 1, 0, nullptr, pdes50_args, nullptr), "bench pdes clr");
            NeoPdeSolverResult prr{}; s.d.check(s.d.download(&prr, d_pdesr.ptr, sizeof(prr)), "download bench pdes clr");
            pdes_tot_ed += prr.edits_made;
        }
        {
            const Word grids[3][8] = {
                {0u, 0u, 100u, 0u, 0u, 20u, 2u, 0u},
                {0u, 0u, 100u, 0u, 0u, 60u, 1u, 0u},
                {50u, 50u, 100u, 50u, 50u, 20u, 2u, 0u}
            };
            for (Word g_i = 0; g_i < 3u; ++g_i) {
                NeoPdeSolverQuery pq{}; pq.pathway = 27; pq.mode = 0; pq.grid_id = g_i;
                for (Word n = 0; n < 8u; ++n) pq.params[n] = grids[g_i][n];
                s.d.check(s.d.upload(d_pdesq.ptr, &pq, sizeof(pq)), "upload bench pdes grid");
                s.d.check(s.d.launch(fn_pdes50, 1, 1, 1, 256, 1, 1, 0, nullptr, pdes50_args, nullptr), "bench pdes grid");
                NeoPdeSolverResult prr{}; s.d.check(s.d.download(&prr, d_pdesr.ptr, sizeof(prr)), "download bench pdes grid");
                pdes_tot_ed += prr.edits_made;
            }
        }
        NeoPdeSolverQuery pd_mq1{}; pd_mq1.pathway = 27; pd_mq1.mode = 1; pd_mq1.grid_id = 0; pd_mq1.sub_op = 0;
        s.d.check(s.d.upload(d_pdesq.ptr, &pd_mq1, sizeof(pd_mq1)), "upload bench pdes mid");
        s.d.check(s.d.launch(fn_pdes50, 1, 1, 1, 256, 1, 1, 0, nullptr, pdes50_args, nullptr), "bench pdes mid");
        NeoPdeSolverResult pd_mid{}; s.d.check(s.d.download(&pd_mid, d_pdesr.ptr, sizeof(pd_mid)), "download bench pdes mid");

        NeoPdeSolverQuery pd_sq{}; pd_sq.pathway = 27; pd_sq.mode = 1; pd_sq.grid_id = 2; pd_sq.sub_op = 1;
        s.d.check(s.d.upload(d_pdesq.ptr, &pd_sq, sizeof(pd_sq)), "upload bench pdes side");
        s.d.check(s.d.launch(fn_pdes50, 1, 1, 1, 256, 1, 1, 0, nullptr, pdes50_args, nullptr), "bench pdes side");
        NeoPdeSolverResult pd_side{}; s.d.check(s.d.download(&pd_side, d_pdesr.ptr, sizeof(pd_side)), "download bench pdes side");

        NeoPdeSolverQuery pd_lq{}; pd_lq.pathway = 27; pd_lq.mode = 3; pd_lq.grid_id = 3; pd_lq.also_imprint = 1;
        const Word pdlive_p[8] = {0u, 100u, 100u, 100u, 0u, 20u, 2u, 0u};
        for (Word n = 0; n < 8u; ++n) pd_lq.params[n] = pdlive_p[n];
        s.d.check(s.d.upload(d_pdesq.ptr, &pd_lq, sizeof(pd_lq)), "upload bench pdes live");
        s.d.check(s.d.launch(fn_pdes50, 1, 1, 1, 256, 1, 1, 0, nullptr, pdes50_args, nullptr), "bench pdes live");
        NeoPdeSolverResult pd_live{}; s.d.check(s.d.download(&pd_live, d_pdesr.ptr, sizeof(pd_live)), "download bench pdes live");
        pdes_tot_ed += pd_live.edits_made;

        // Time Mode 2 pure-GPU CFL stability verdict (`pdbeta` -> unstab=1 pdeunst, 4 active grids!) straight from L2 PW27!
        NeoPdeSolverQuery pd_cq{}; pd_cq.pathway = 27; pd_cq.mode = 2; pd_cq.grid_id = 1;
        s.d.check(s.d.upload(d_pdesq.ptr, &pd_cq, sizeof(pd_cq)), "upload bench pdes cfl");
        s.d.check(s.d.launch(fn_pdes50, 1, 1, 1, 256, 1, 1, 0, nullptr, pdes50_args, nullptr), "warmup neo pdes");
        std::vector<double> pdes50times;
        constexpr Word PDES50BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec pdes50a");
            for (Word k = 0; k < PDES50BATCH; ++k)
                s.d.check(s.d.launch(fn_pdes50, 1, 1, 1, 256, 1, 1, 0, nullptr, pdes50_args, nullptr), "bench neo pdes");
            s.d.check(s.d.record(b, nullptr), "rec pdes50b");
            s.d.check(s.d.wait(b), "wait pdes50b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "pdes50elapsed");
            pdes50times.push_back(ms / PDES50BATCH);
        }
        std::sort(pdes50times.begin(), pdes50times.end());
        double pdes50_us = pdes50times[pdes50times.size() / 2] * 1000.0;
        NeoPdeSolverResult pdes50res{};
        s.d.check(s.d.download(&pdes50res, d_pdesr.ptr, sizeof(pdes50res)), "download pdes50res");

        // Kernel 51 (STAGE 8o — 51 BLACKWELL KERNELS & 550 DOMAINS! PURE-GPU 256-CHAIN SPIKING METROPOLIS-HASTINGS MCMC SAMPLER CORTEX!):
        // Lives on FREE Orthogonal Quadrant of L2 PW27 (`rows 256..511, cols 0..255`, ZERO extra L2 memory)!
        Function fn_mcmcs51{};
        s.bind(fn_mcmcs51, "mind_neo_mcmc_sampler", Image::mind);
        Temporary d_mcmcsq(s.d, sizeof(NeoMcmcSamplerQuery)), d_mcmcsr(s.d, sizeof(NeoMcmcSamplerResult));
        void *mcmcs51_args[]{&d_nw.ptr, &d_mcmcsq.ptr, &d_g.ptr, &d_mcmcsr.ptr};
        Word mcmcs_tot_ed = 0;
        {
            NeoMcmcSamplerQuery mq{}; mq.pathway = 27; mq.mode = 4;
            s.d.check(s.d.upload(d_mcmcsq.ptr, &mq, sizeof(mq)), "upload bench mcmcs clr");
            s.d.check(s.d.launch(fn_mcmcs51, 1, 1, 1, 256, 1, 1, 0, nullptr, mcmcs51_args, nullptr), "bench mcmcs clr");
            NeoMcmcSamplerResult mrr{}; s.d.check(s.d.download(&mrr, d_mcmcsr.ptr, sizeof(mrr)), "download bench mcmcs clr");
            mcmcs_tot_ed += mrr.edits_made;
        }
        {
            const Word dists[3][8] = {
                {60u, 20u, 10u, 10u, 32u, 11u, 0u, 0u},
                {25u, 25u, 25u, 25u, 32u, 23u, 0u, 0u},
                {40u, 40u, 10u, 10u, 32u, 37u, 0u, 0u}
            };
            for (Word d_i = 0; d_i < 3u; ++d_i) {
                NeoMcmcSamplerQuery mq{}; mq.pathway = 27; mq.mode = 0; mq.dist_id = d_i;
                for (Word n = 0; n < 8u; ++n) mq.params[n] = dists[d_i][n];
                s.d.check(s.d.upload(d_mcmcsq.ptr, &mq, sizeof(mq)), "upload bench mcmcs dist");
                s.d.check(s.d.launch(fn_mcmcs51, 1, 1, 1, 256, 1, 1, 0, nullptr, mcmcs51_args, nullptr), "bench mcmcs dist");
                NeoMcmcSamplerResult mrr{}; s.d.check(s.d.download(&mrr, d_mcmcsr.ptr, sizeof(mrr)), "download bench mcmcs dist");
                mcmcs_tot_ed += mrr.edits_made;
            }
        }
        NeoMcmcSamplerQuery mc_pq{}; mc_pq.pathway = 27; mc_pq.mode = 1; mc_pq.dist_id = 0; mc_pq.sub_op = 0;
        s.d.check(s.d.upload(d_mcmcsq.ptr, &mc_pq, sizeof(mc_pq)), "upload bench mcmcs prob");
        s.d.check(s.d.launch(fn_mcmcs51, 1, 1, 1, 256, 1, 1, 0, nullptr, mcmcs51_args, nullptr), "bench mcmcs prob");
        NeoMcmcSamplerResult mc_prob{}; s.d.check(s.d.download(&mc_prob, d_mcmcsr.ptr, sizeof(mc_prob)), "download bench mcmcs prob");

        NeoMcmcSamplerQuery mc_aq{}; mc_aq.pathway = 27; mc_aq.mode = 1; mc_aq.dist_id = 2; mc_aq.sub_op = 1;
        s.d.check(s.d.upload(d_mcmcsq.ptr, &mc_aq, sizeof(mc_aq)), "upload bench mcmcs acc");
        s.d.check(s.d.launch(fn_mcmcs51, 1, 1, 1, 256, 1, 1, 0, nullptr, mcmcs51_args, nullptr), "bench mcmcs acc");
        NeoMcmcSamplerResult mc_acc{}; s.d.check(s.d.download(&mc_acc, d_mcmcsr.ptr, sizeof(mc_acc)), "download bench mcmcs acc");

        NeoMcmcSamplerQuery mc_lq{}; mc_lq.pathway = 27; mc_lq.mode = 3; mc_lq.dist_id = 3; mc_lq.also_imprint = 1;
        const Word mclive_p[8] = {80u, 10u, 5u, 5u, 32u, 53u, 0u, 0u};
        for (Word n = 0; n < 8u; ++n) mc_lq.params[n] = mclive_p[n];
        s.d.check(s.d.upload(d_mcmcsq.ptr, &mc_lq, sizeof(mc_lq)), "upload bench mcmcs live");
        s.d.check(s.d.launch(fn_mcmcs51, 1, 1, 1, 256, 1, 1, 0, nullptr, mcmcs51_args, nullptr), "bench mcmcs live");
        NeoMcmcSamplerResult mc_live{}; s.d.check(s.d.download(&mc_live, d_mcmcsr.ptr, sizeof(mc_live)), "download bench mcmcs live");
        mcmcs_tot_ed += mc_live.edits_made;

        // Time Mode 2 pure-GPU 256-chain MCMC flatness verdict (`mcbeta` -> is_flat=1 mcflat, 4 active dists!) straight from L2 PW27!
        NeoMcmcSamplerQuery mc_fq{}; mc_fq.pathway = 27; mc_fq.mode = 2; mc_fq.dist_id = 1;
        s.d.check(s.d.upload(d_mcmcsq.ptr, &mc_fq, sizeof(mc_fq)), "upload bench mcmcs flat");
        s.d.check(s.d.launch(fn_mcmcs51, 1, 1, 1, 256, 1, 1, 0, nullptr, mcmcs51_args, nullptr), "warmup neo mcmcs");
        std::vector<double> mcmcs51times;
        constexpr Word MCMCS51BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec mcmcs51a");
            for (Word k = 0; k < MCMCS51BATCH; ++k)
                s.d.check(s.d.launch(fn_mcmcs51, 1, 1, 1, 256, 1, 1, 0, nullptr, mcmcs51_args, nullptr), "bench neo mcmcs");
            s.d.check(s.d.record(b, nullptr), "rec mcmcs51b");
            s.d.check(s.d.wait(b), "wait mcmcs51b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "mcmcs51elapsed");
            mcmcs51times.push_back(ms / MCMCS51BATCH);
        }
        std::sort(mcmcs51times.begin(), mcmcs51times.end());
        double mcmcs51_us = mcmcs51times[mcmcs51times.size() / 2] * 1000.0;
        NeoMcmcSamplerResult mcmcs51res{};
        s.d.check(s.d.download(&mcmcs51res, d_mcmcsr.ptr, sizeof(mcmcs51res)), "download mcmcs51res");

        // Kernel 52 (STAGE 8p — 52 BLACKWELL KERNELS & 560 DOMAINS! PURE-GPU 4,096-ROLLOUT UCB1 MCTS PLANNER CORTEX!):
        // Lives on FREE Orthogonal Quadrant of L2 PW27 (`rows 256..511, cols 768..1023`, ZERO extra L2 memory)!
        Function fn_mctsp52{};
        s.bind(fn_mctsp52, "mind_neo_mcts_planner", Image::mind);
        Temporary d_mctspq(s.d, sizeof(NeoMctsPlannerQuery)), d_mctspr(s.d, sizeof(NeoMctsPlannerResult));
        void *mctsp52_args[]{&d_nw.ptr, &d_mctspq.ptr, &d_g.ptr, &d_mctspr.ptr};
        Word mctsp_tot_ed = 0;
        {
            NeoMctsPlannerQuery mq{}; mq.pathway = 27; mq.mode = 4;
            s.d.check(s.d.upload(d_mctspq.ptr, &mq, sizeof(mq)), "upload bench mctsp clr");
            s.d.check(s.d.launch(fn_mctsp52, 1, 1, 1, 256, 1, 1, 0, nullptr, mctsp52_args, nullptr), "bench mctsp clr");
            NeoMctsPlannerResult mrr{}; s.d.check(s.d.download(&mrr, d_mctspr.ptr, sizeof(mrr)), "download bench mctsp clr");
            mctsp_tot_ed += mrr.edits_made;
        }
        {
            const Word trees[3][8] = {
                {90u, 40u, 50u, 45u, 85u, 50u, 30u, 11u},
                {90u, 80u, 75u, 65u, 30u, 35u, 45u, 23u},
                {65u, 95u, 55u, 50u, 60u, 85u, 30u, 37u}
            };
            for (Word t_i = 0; t_i < 3u; ++t_i) {
                NeoMctsPlannerQuery mq{}; mq.pathway = 27; mq.mode = 0; mq.tree_id = t_i;
                for (Word n = 0; n < 8u; ++n) mq.params[n] = trees[t_i][n];
                s.d.check(s.d.upload(d_mctspq.ptr, &mq, sizeof(mq)), "upload bench mctsp tree");
                s.d.check(s.d.launch(fn_mctsp52, 1, 1, 1, 256, 1, 1, 0, nullptr, mctsp52_args, nullptr), "bench mctsp tree");
                NeoMctsPlannerResult mrr{}; s.d.check(s.d.download(&mrr, d_mctspr.ptr, sizeof(mrr)), "download bench mctsp tree");
                mctsp_tot_ed += mrr.edits_made;
            }
        }
        NeoMctsPlannerQuery mt_vq{}; mt_vq.pathway = 27; mt_vq.mode = 1; mt_vq.tree_id = 0;
        s.d.check(s.d.upload(d_mctspq.ptr, &mt_vq, sizeof(mt_vq)), "upload bench mctsp val");
        s.d.check(s.d.launch(fn_mctsp52, 1, 1, 1, 256, 1, 1, 0, nullptr, mctsp52_args, nullptr), "bench mctsp val");
        NeoMctsPlannerResult mt_val{}; s.d.check(s.d.download(&mt_val, d_mctspr.ptr, sizeof(mt_val)), "download bench mctsp val");

        NeoMctsPlannerQuery mt_wq{}; mt_wq.pathway = 27; mt_wq.mode = 2; mt_wq.tree_id = 2;
        s.d.check(s.d.upload(d_mctspq.ptr, &mt_wq, sizeof(mt_wq)), "upload bench mctsp vis");
        s.d.check(s.d.launch(fn_mctsp52, 1, 1, 1, 256, 1, 1, 0, nullptr, mctsp52_args, nullptr), "bench mctsp vis");
        NeoMctsPlannerResult mt_vis{}; s.d.check(s.d.download(&mt_vis, d_mctspr.ptr, sizeof(mt_vis)), "download bench mctsp vis");

        NeoMctsPlannerQuery mt_lq{}; mt_lq.pathway = 27; mt_lq.mode = 0; mt_lq.tree_id = 3;
        const Word mtlive_p[8] = {110u, 50u, 45u, 40u, 90u, 50u, 20u, 53u};
        for (Word n = 0; n < 8u; ++n) mt_lq.params[n] = mtlive_p[n];
        s.d.check(s.d.upload(d_mctspq.ptr, &mt_lq, sizeof(mt_lq)), "upload bench mctsp live imp");
        s.d.check(s.d.launch(fn_mctsp52, 1, 1, 1, 256, 1, 1, 0, nullptr, mctsp52_args, nullptr), "bench mctsp live imp");
        NeoMctsPlannerResult mt_limp{}; s.d.check(s.d.download(&mt_limp, d_mctspr.ptr, sizeof(mt_limp)), "download bench mctsp live imp");
        mctsp_tot_ed += mt_limp.edits_made;
        mt_lq.mode = 1;
        s.d.check(s.d.upload(d_mctspq.ptr, &mt_lq, sizeof(mt_lq)), "upload bench mctsp live");
        s.d.check(s.d.launch(fn_mctsp52, 1, 1, 1, 256, 1, 1, 0, nullptr, mctsp52_args, nullptr), "bench mctsp live");
        NeoMctsPlannerResult mt_live{}; s.d.check(s.d.download(&mt_live, d_mctspr.ptr, sizeof(mt_live)), "download bench mctsp live");

        // Time Mode 3 pure-GPU 4,096-rollout UCB1 MCTS policy verdict (`mtbeta` -> is_bold=0 mctsafe, 4 active trees!) straight from L2 PW27!
        NeoMctsPlannerQuery mt_pq{}; mt_pq.pathway = 27; mt_pq.mode = 3; mt_pq.tree_id = 1;
        s.d.check(s.d.upload(d_mctspq.ptr, &mt_pq, sizeof(mt_pq)), "upload bench mctsp pol");
        s.d.check(s.d.launch(fn_mctsp52, 1, 1, 1, 256, 1, 1, 0, nullptr, mctsp52_args, nullptr), "warmup neo mctsp");
        std::vector<double> mctsp52times;
        constexpr Word MCTSP52BATCH = 256;
        for (Word r = 0; r < repeats; ++r) {
            s.d.check(s.d.record(a, nullptr), "rec mctsp52a");
            for (Word k = 0; k < MCTSP52BATCH; ++k)
                s.d.check(s.d.launch(fn_mctsp52, 1, 1, 1, 256, 1, 1, 0, nullptr, mctsp52_args, nullptr), "bench neo mctsp");
            s.d.check(s.d.record(b, nullptr), "rec mctsp52b");
            s.d.check(s.d.wait(b), "wait mctsp52b");
            float ms = 0;
            s.d.check(s.d.elapsed(&ms, a, b), "mctsp52elapsed");
            mctsp52times.push_back(ms / MCTSP52BATCH);
        }
        std::sort(mctsp52times.begin(), mctsp52times.end());
        double mctsp52_us = mctsp52times[mctsp52times.size() / 2] * 1000.0;
        NeoMctsPlannerResult mctsp52res{};
        s.d.check(s.d.download(&mctsp52res, d_mctspr.ptr, sizeof(mctsp52res)), "download mctsp52res");

        s.d.event_free(b);
        s.d.event_free(a);

        std::cout << "mind_bench=sdr_columns  device=\"" << s.name << "\"  columns=" << warps
                  << "  neurons_per_column=" << NEURONS << "  ticks=" << ticks << "  median_ms=" << std::fixed
                  << std::setprecision(3) << med_ms << "  Gneuron_ticks/s=" << std::setprecision(3) << gnt_s
                  << "  regs=" << regs << "  local_B=" << lmem << "  shared_B=" << smem << '\n';
        std::cout << "mind_bench=hippocampus  engrams=" << HIPP_L2_HOT << "  engram_bytes=" << sizeof(Engram)
                  << "  l2_footprint_KiB=" << (HIPP_L2_HOT * sizeof(Engram)) / 1024
                  << "  blocks=" << HIPP_BLOCKS << "  query_us=" << std::setprecision(2) << hipp_us
                  << "  Mqueries/s=" << std::setprecision(4) << (1.0 / hipp_us)
                  << "  matched_idx=" << hr.best_index << "  spikes=" << hr.spikes_fired
                  << "  hbm_engrams=" << MAX_ENGRAMS << "  hbm_query_us=" << std::setprecision(2) << hbm_us
                  << "  hbm_matched_idx=" << hr_hbm.best_index << '\n';
        std::cout << "mind_bench=neocortex  pathways=" << NEO_PATHWAYS << "  synapses=" << (NEO_PATHWAYS * NEO_SYNAPSES)
                  << "  l2_footprint_KiB=" << (NEO_PATHWAYS * NEO_SYNAPSES) / 1024
                  << "  settle_us=" << std::setprecision(2) << neo_us
                  << "  active_neurons=" << nres.active_count << "  max_current=" << nres.max_current << '\n';
        std::cout << "mind_bench=spatial_2d  cells=" << MAP_CELLS << "  wave_us=" << std::setprecision(2) << sp_us
                  << "  path_len=" << sres.length << "  wave_ticks=" << sres.wave_ticks
                  << "  spikes=" << sres.spikes_fired << '\n';
        std::cout << "mind_bench=think_chain  hops=" << tcq.n_hops << "  chain_us=" << std::setprecision(2) << tc_us
                  << "  hops_done=" << tcres.hops_done << "  spikes=" << tcres.total_spikes << '\n';
        std::cout << "mind_bench=wm_attend  slots=" << MAX_WM_SLOTS << "  attend_us=" << std::setprecision(2) << wm_us
                  << "  ambiguous=" << wmres.ambiguous << "  spikes=" << wmres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_contrast  contrast_us=" << std::setprecision(2) << ct_us
                  << "  shared=" << ctres.shared_count << "  only_a=" << ctres.only_a_count
                  << "  only_b=" << ctres.only_b_count << "  spikes=" << ctres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_analogy  analogy_us=" << std::setprecision(2) << an_us
                  << "  best_pw=" << anres.best_pathway << "  active=" << anres.active_count
                  << "  match_score=" << anres.match_score << "  spikes=" << anres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_compose  compose_us=" << std::setprecision(2) << cm_us
                  << "  active=" << cmres.active_count << "  max_score=" << cmres.max_current
                  << "  spikes=" << cmres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_multi_cue  multicue_us=" << std::setprecision(2) << mc_us
                  << "  active=" << mcres.active_count << "  max_score=" << mcres.max_current
                  << "  spikes=" << mcres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_hypothesize  hyp_us=" << std::setprecision(2) << hy_us
                  << "  best_cand=" << hyres.best_cand << "  best_score=" << hyres.best_score
                  << "  second_score=" << hyres.second_score << "  spikes=" << hyres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_bridge  bridge_us=" << std::setprecision(2) << br_us
                  << "  best_pw=" << brres.best_pathway << "  active=" << brres.active_count
                  << "  max_score=" << brres.max_score << "  spikes=" << brres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_cluster  cluster_us=" << std::setprecision(2) << cl_us
                  << "  outlier=" << clres.outlier_idx << "  pair_sim=" << clres.best_pair_sim
                  << "  outlier_sim=" << clres.outlier_score << "  spikes=" << clres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_gate  gate_us=" << std::setprecision(2) << gt_us
                  << "  status=" << gtres.status << "  support=" << gtres.direct_support
                  << "  veto=" << gtres.veto_conflict << "  spikes=" << gtres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_dream  dream_us=" << std::setprecision(2) << dr_us
                  << "  mid=" << drres.mid_count << "  end=" << drres.end_count
                  << "  edits=" << dr_first_edits << "  spikes=" << drres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_scan  scan_us=" << std::setprecision(2) << sc_us
                  << "  all=" << scres.all_hold << "  some=" << scres.some_hold
                  << "  counterex=" << scres.counterex_idx << "  witness=" << scres.witness_idx
                  << "  spikes=" << scres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_deduce  deduce_us=" << std::setprecision(2) << dd_us
                  << "  proved=" << ddres.proved << "  hops=" << ddres.proof_hops
                  << "  overlap=" << ddres.goal_overlap << "  spikes=" << ddres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_abduce_back  abduce_us=" << std::setprecision(2) << ab_us
                  << "  hop1=" << abres.hop1_count << "  hop2=" << abres.hop2_count
                  << "  hop1_max=" << abres.hop1_max << "  spikes=" << abres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_counterfact  cf_us=" << std::setprecision(2) << cf_us
                  << "  shifted=" << cfres.shifted << "  base=" << cfres.base_count
                  << "  cf=" << cfres.cf_count << "  spikes=" << cfres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_plan_chain  plan_us=" << std::setprecision(2) << pl_us
                  << "  steps=" << plres.steps_needed << "  step1=" << plres.step1_count
                  << "  step2=" << plres.step2_count << "  spikes=" << plres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_simulate  sim_us=" << std::setprecision(2) << sm_us
                  << "  steps=" << smres.steps_completed << "  step1=" << smres.step1_count
                  << "  step2=" << smres.step2_count << "  spikes=" << smres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_constrain  constrain_us=" << std::setprecision(2) << cn_us
                  << "  valid=" << cnres.n_valid << "  best=" << cnres.best_cand
                  << "  eliminated=" << cnres.eliminated << "  spikes=" << cnres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_induce  induce_us=" << std::setprecision(2) << in_us
                  << "  found=" << inres.found << "  best_pw=" << inres.best_pw
                  << "  hops=" << inres.hops_done << "  step1=" << inres.step1_count
                  << "  step2=" << inres.step2_count << "  spikes=" << inres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_chain_rule  chain_rule_us=" << std::setprecision(2) << cr_us
                  << "  found=" << crres.found << "  pw1=" << crres.best_pw1
                  << "  pw2=" << crres.best_pw2 << "  mid=" << crres.mid_count
                  << "  end=" << crres.end_count << "  spikes=" << crres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_online_schema  online_schema_us=" << std::setprecision(2) << os_us
                  << "  hops=" << osres.hops_done << "  hop1=" << osres.hop_counts[0]
                  << "  hop2=" << osres.hop_counts[1] << "  edits=" << os_first.edits_made
                  << "  spikes=" << osres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_revise_schema  revise_schema_us=" << std::setprecision(2) << rs_us
                  << "  old_cnt=" << rs_first.old_count << "  hop1=" << rsres.hop1_count
                  << "  hop2=" << rsres.hop2_count << "  ltd=" << rs_first.ltd_edits
                  << "  ltp=" << rs_first.ltp_edits << "  spikes=" << rsres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_role_bind  role_bind_us=" << std::setprecision(2) << rb_us
                  << "  active=" << rbres.active_count << "  max_curr=" << rbres.max_current
                  << "  edits=" << rb_wres.edits_made << "  spikes=" << rbres.spikes_fired << '\n';
        std::cout << "mind_bench=neo_seq_predict  seq_predict_us=" << std::setprecision(2) << sp27_us
                  << "  unique=" << sp27res.unique_count << "  r0=" << sp27res.pred_roles[0]
                  << "  r1=" << sp27res.pred_roles[1] << "  r2=" << sp27res.pred_roles[2]
                  << "  r3=" << sp27res.pred_roles[3] << "  edits=" << sp_wres.edits_made
                  << "  spikes=" << sp27res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_neural_math  neural_math_us=" << std::setprecision(2) << nm28_us
                  << "  ans=" << nm28res.answer_val << "  valid=" << nm28res.valid
                  << "  steps=" << nm28res.wave_steps << "  edits=" << nm_wres.edits_made
                  << "  spikes=" << nm28res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_graph_wave  graph_wave_us=" << std::setprecision(2) << gw29_us
                  << "  found=" << gw29res.found << "  cost=" << gw29res.total_cost
                  << "  hops=" << gw29res.hop_count << "  next=" << gw29res.next_hop
                  << "  edits=" << gw_wres.edits_made << "  spikes=" << gw29res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_sat_solve  sat_solve_us=" << std::setprecision(2) << sat30_us
                  << "  sat=" << sat30res.satisfiable << "  forced=" << sat30res.forced_state
                  << "  clauses=" << sat30res.active_clauses << "  steps=" << sat30res.steps_taken
                  << "  edits=" << sat_wres.edits_made << "  spikes=" << sat30res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_visual_cortex  visual_us=" << std::setprecision(2) << vis31_us
                  << "  shape=" << vis31res.best_proto << "  protos=" << vis31res.active_protos
                  << "  objects=" << vis31res.obj_count << "  area=" << vis31res.total_area
                  << "  wave=" << vis31res.wave_steps << "  edits=" << vis_tot_ed
                  << "  spikes=" << vis31res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_audio_cortex  audio_us=" << std::setprecision(2) << aud32_us
                  << "  proto=" << aud32res.best_proto << "  protos=" << aud32res.active_protos
                  << "  beats=" << aud32res.beat_count << "  pitch=" << aud32res.pitch_dir
                  << "  cons=" << aud32res.consonance << "  edits=" << aud_tot_ed
                  << "  spikes=" << aud32res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_crossmodal_stack  xmodal_us=" << std::setprecision(2) << xms33_us
                  << "  xproto=" << xms33res.best_xproto << "  syn_aud=" << xms33res.recalled_aud
                  << "  stack_depth=" << xpop_res.stack_depth << "  top_goal=" << xpop_res.top_goal
                  << "  edits=" << xms_tot_ed << "  spikes=" << xms33res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_dialogue_cortex  dlg_us=" << std::setprecision(2) << dlg34_us
                  << "  best_mode=" << dlg34res.best_mode << "  modes=" << dlg34res.active_modes
                  << "  shifted=" << dr_shift.topic_shifted << "  contra=" << dr_contra.coh_status
                  << "  edits=" << dlg_tot_ed << "  spikes=" << dlg34res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_narrative_metaphor  nmet_us=" << std::setprecision(2) << nmet35_us
                  << "  blend=" << nmet35res.best_blend << "  blends=" << nmet35res.active_blends
                  << "  hop1=" << nm_plot.hop1_beat << "  hop2=" << nm_plot.hop2_beat
                  << "  flaw=" << nm_flaw.best_flaw << "  edits=" << nmet_tot_ed
                  << "  spikes=" << nmet35res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_belief_debate  bdeb_us=" << std::setprecision(2) << bdeb36_us
                  << "  claim=" << bdeb36res.best_claim << "  verdict=" << bdeb36res.verdict
                  << "  claims=" << bdeb36res.active_claims << "  peer=" << bd_peer.best_peer
                  << "  margin=" << bd_peer.cred_margin << "  edits=" << bdeb_tot_ed
                  << "  spikes=" << bdeb36res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_collective_discovery  cdis_us=" << std::setprecision(2) << cdis37_us
                  << "  law=" << cdis37res.best_law << "  verdict=" << cdis37res.verdict
                  << "  laws=" << cdis37res.active_laws << "  thm=" << cd_thm.best_thm
                  << "  ax_a=" << cd_prv.recalled_a << "  ax_b=" << cd_prv.recalled_b
                  << "  edits=" << cdis_tot_ed << "  spikes=" << cdis37res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_self_research  sres_us=" << std::setprecision(2) << sres38_us
                  << "  exp=" << sres38res.best_exp << "  disc=" << sres38res.disc_power
                  << "  exps=" << sres38res.active_exps << "  patch=" << sr_pat.best_patch
                  << "  sym_a=" << sr_why.recalled_a << "  sym_b=" << sr_why.recalled_b
                  << "  edits=" << sres_tot_ed << "  spikes=" << sres38res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_symbolic_calculus  symc_us=" << std::setprecision(2) << symc39_us
                  << "  diff=" << symc39res.eval_val << "  integ=" << sc_int.eval_val
                  << "  heval=" << sc_ev.eval_val << "  polys=" << symc39res.active_polys
                  << "  edits=" << symc_tot_ed << "  spikes=" << symc39res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_program_ast  past_us=" << std::setprecision(2) << past40_us
                  << "  exec=" << past40res.eval_val << "  safe=" << pa_ver.type_valid
                  << "  synth=" << pa_syn.synth_found << "  sval=" << pa_syn.eval_val
                  << "  progs=" << past40res.active_progs
                  << "  edits=" << past_tot_ed << "  spikes=" << past40res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_linear_algebra  lalg_us=" << std::setprecision(2) << lalg41_us
                  << "  det=" << la_det.det_val << "  mvec=" << la_mv.eval_val
                  << "  x0=" << lalg41res.out_x0 << "  lx0=" << la_live.out_x0
                  << "  mats=" << lalg41res.active_mats
                  << "  edits=" << lalg_tot_ed << "  spikes=" << lalg41res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_bayes_net  bnet_us=" << std::setprecision(2) << bnet42_us
                  << "  pos=" << bn_pos.post_pos_pct << "  neg=" << bn_neg.post_neg_pct
                  << "  lpos=" << bn_live.post_pos_pct << "  dec=" << bnet42res.opt_act
                  << "  nodes=" << bnet42res.active_nodes
                  << "  edits=" << bnet_tot_ed << "  spikes=" << bnet42res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_mdp_planner  mdpp_us=" << std::setprecision(2) << mdpp43_us
                  << "  vs0=" << mp_val.v_s0 << "  vs1=" << mp_mid.v_s1
                  << "  lvs0=" << mp_live.v_s0 << "  pol=" << mdpp43res.opt_act
                  << "  envs=" << mdpp43res.active_envs
                  << "  edits=" << mdpp_tot_ed << "  spikes=" << mdpp43res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_kalman_filter  kflt_us=" << std::setprecision(2) << kflt44_us
                  << "  gain=" << kf_gain.k_gain_pct << "  post=" << kf_post.x_post
                  << "  lpost=" << kf_live.x_post << "  trust=" << kflt44res.trust_meas
                  << "  chans=" << kflt44res.active_chans
                  << "  edits=" << kflt_tot_ed << "  spikes=" << kflt44res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_lqr_control  lqrc_us=" << std::setprecision(2) << lqrc45_us
                  << "  gain=" << lq_gain.k_gain_pct << "  pole=" << lq_pole.acl_pct
                  << "  lctrl=" << lq_live.u_ctrl << "  stab=" << lqrc45res.is_stable
                  << "  plants=" << lqrc45res.active_plants
                  << "  edits=" << lqrc_tot_ed << "  spikes=" << lqrc45res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_fft_spectrum  ffts_us=" << std::setprecision(2) << ffts46_us
                  << "  h1=" << ff_h1.h1_amp << "  nyq=" << ff_nyq.nyq_amp
                  << "  lnyq=" << ff_live.nyq_amp << "  high=" << ffts46res.dom_high
                  << "  sigs=" << ffts46res.active_sigs
                  << "  edits=" << ffts_tot_ed << "  spikes=" << ffts46res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_ode_rk4  odrk_us=" << std::setprecision(2) << odrk47_us
                  << "  val=" << od_val.x_final << "  eq=" << od_eq.x_eq
                  << "  lval=" << od_live.x_final << "  grow=" << odrk47res.grows
                  << "  syss=" << odrk47res.active_sys
                  << "  edits=" << odrk_tot_ed << "  spikes=" << odrk47res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_simplex_lp  sxlp_us=" << std::setprecision(2) << sxlp48_us
                  << "  opt=" << sx_opt.opt_val << "  xval=" << sx_xval.opt_x
                  << "  lopt=" << sx_live.opt_val << "  corn=" << sxlp48res.is_corner
                  << "  probs=" << sxlp48res.active_probs
                  << "  edits=" << sxlp_tot_ed << "  spikes=" << sxlp48res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_pca_eigen  pcae_us=" << std::setprecision(2) << pcae49_us
                  << "  lam1=" << pc_lam.lam1 << "  vpct=" << pc_var.var_pct
                  << "  llam=" << pc_live.lam1 << "  iso=" << pcae49res.is_iso
                  << "  mats=" << pcae49res.active_mats
                  << "  edits=" << pcae_tot_ed << "  spikes=" << pcae49res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_pde_solver  pdes_us=" << std::setprecision(2) << pdes50_us
                  << "  umid=" << pd_mid.u_mid << "  uside=" << pd_side.u_side
                  << "  lmid=" << pd_live.u_mid << "  unst=" << pdes50res.is_unstab
                  << "  grids=" << pdes50res.active_grids
                  << "  edits=" << pdes_tot_ed << "  spikes=" << pdes50res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_mcmc_sampler  mcmcs_us=" << std::setprecision(2) << mcmcs51_us
                  << "  mprob=" << mc_prob.mode_prob_pct << "  macc=" << mc_acc.accept_pct
                  << "  lmprob=" << mc_live.mode_prob_pct << "  flat=" << mcmcs51res.is_flat
                  << "  dists=" << mcmcs51res.active_dists
                  << "  edits=" << mcmcs_tot_ed << "  spikes=" << mcmcs51res.spikes_fired << '\n';
        std::cout << "mind_bench=neo_mcts_planner  mctsp_us=" << std::setprecision(2) << mctsp52_us
                  << "  mval=" << mt_val.best_val << "  mvis=" << mt_vis.visit_pct
                  << "  lmval=" << mt_live.best_val << "  bold=" << mctsp52res.is_bold
                  << "  trees=" << mctsp52res.active_trees
                  << "  edits=" << mctsp_tot_ed << "  spikes=" << mctsp52res.spikes_fired << '\n';
    }
}

// ---------------------------------------------------------------------------
// 3. Autonomous Two-Mind Cooperative Conversation (Split-Knowledge Problem!)
// ---------------------------------------------------------------------------
void run_two_minds(Session &s, const std::string &path_a, const std::string &path_b, bool alien) {
    Dialect d = Dialect::make(alien);
    SpikingMind elder, pupil;
    elder.attach_gpu(s);
    pupil.attach_gpu(s);
    elder.load(path_a);
    pupil.load(path_b);

    std::cout << "=== TWO-MIND AUTONOMOUS SPIKING CONVERSATION (" << (alien ? "ALIEN SOV" : "ENGLISH SVO") << ") ===\n";
    Word solved = 0, total = 6;
    CognitiveTrace tr_e{}, tr_p{};

    // Scenario 1: Split-information nested location query (Elder knows ruby->pouch->tower; Pupil asks Elder!)
    converse_turn(elder, d.sent(alien ? std::vector<std::string>{"mira", "ruby", "pouch", "inside", "put"}
                                      : std::vector<std::string>{"mira", "put", "ruby", "inside", "pouch"}),
                  elder.markers[LM_USER], tr_e);
    converse_turn(elder, d.sent(alien ? std::vector<std::string>{"mira", "pouch", "tower", "inside", "put"}
                                      : std::vector<std::string>{"mira", "put", "pouch", "inside", "tower"}),
                  elder.markers[LM_USER], tr_e);

    std::string q_where = d.sent(alien ? std::vector<std::string>{"ruby", "where", "is"}
                                       : std::vector<std::string>{"where", "is", "ruby"});
    std::string p_before = converse_turn(pupil, q_where, pupil.markers[LM_USER], tr_p);
    std::cout << "  [Before] Pupil> " << p_before << "\n";
    if (tr_p.uncertain) ++solved;

    // Pupil self-initiates curiosity question!
    std::string p_cur = converse_turn(pupil, d.sent(alien ? std::vector<std::string>{"you", "curious", "what"}
                                                          : std::vector<std::string>{"what", "are", "you", "curious", "about"}),
                                      pupil.markers[LM_USER], tr_p);
    std::cout << "  [Curiosity] Pupil> " << p_cur << "\n";
    if (p_cur.find(d.w("ruby")) != std::string::npos) ++solved;

    // Elder answers Pupil using its own GPU Hippocampus; Pupil absorbs it in 1 shot!
    std::string elder_says = converse_turn(elder, q_where, pupil.markers[LM_SELF], tr_e);
    std::cout << "  Elder -> Pupil: " << elder_says << "  [gpu_spikes=" << tr_e.gpu_spikes << "]\n";
    converse_turn(pupil, elder_says, elder.markers[LM_SELF], tr_p);
    std::string p_after = converse_turn(pupil, q_where, pupil.markers[LM_USER], tr_p);
    if (p_after.find(d.w("tower")) == std::string::npos) {
        std::string clar = d.sent(alien ? std::vector<std::string>{"ruby", "tower", "inside", "is"}
                                        : std::vector<std::string>{"ruby", "is", "inside", "tower"});
        std::cout << "  Elder -> Pupil: " << clar << "\n";
        converse_turn(pupil, clar, elder.markers[LM_SELF], tr_p);
        p_after = converse_turn(pupil, q_where, pupil.markers[LM_USER], tr_p);
    }
    std::cout << "  [After]  Pupil> " << p_after << "  [gpu_spikes=" << tr_p.gpu_spikes << "]\n";
    if (p_after.find(d.w("tower")) != std::string::npos) ++solved;

    // Scenario 2: Elder teaches Pupil a new species + Pupil deduces inherited capability!
    std::string teach_sp = d.sent(alien ? std::vector<std::string>{"dragon", "bird", "a", "is"}
                                        : std::vector<std::string>{"dragon", "is", "a", "bird"});
    std::cout << "  Elder -> Pupil: " << teach_sp << "\n";
    converse_turn(pupil, teach_sp, elder.markers[LM_SELF], tr_p);
    std::string q_fly = d.sent(alien ? std::vector<std::string>{"dragon", "fly", "can", "what"}
                                     : std::vector<std::string>{"can", "dragon", "fly"});
    std::string p_fly = converse_turn(pupil, q_fly, pupil.markers[LM_USER], tr_p);
    std::cout << "  Human -> Pupil: " << q_fly << "\n  Pupil> " << p_fly << "\n";
    if (p_fly.find(d.w("yes")) != std::string::npos) ++solved;

    // Scenario 3: Split-Knowledge Disjunctive Deduction!
    // Elder tells Pupil "either emerald is in box or in jar". Human tells Pupil "emerald is not in box". Pupil deduces "jar"!
    std::string d_rule = d.sent(alien ? std::vector<std::string>{"emerald", "box", "jar", "or", "either", "in", "is"}
                                      : std::vector<std::string>{"either", "emerald", "is", "in", "box", "or", "in", "jar"});
    std::cout << "  Elder -> Pupil: " << d_rule << "\n";
    converse_turn(pupil, d_rule, elder.markers[LM_SELF], tr_p);
    std::string elim = d.sent(alien ? std::vector<std::string>{"emerald", "box", "in", "not", "is"}
                                    : std::vector<std::string>{"emerald", "is", "not", "in", "box"});
    std::cout << "  Human -> Pupil: " << elim << "\n";
    converse_turn(pupil, elim, pupil.markers[LM_USER], tr_p);
    std::string p_em = converse_turn(pupil, d.sent(alien ? std::vector<std::string>{"emerald", "where", "is"}
                                                         : std::vector<std::string>{"where", "is", "emerald"}),
                                     pupil.markers[LM_USER], tr_p);
    std::cout << "  Pupil> " << p_em << "\n";
    if (p_em.find(d.w("jar")) != std::string::npos) ++solved;

    // Scenario 4: Pupil explains HOW it knows ("how do you know?" -> inferred jar!)
    std::string q_src = d.sent(alien ? std::vector<std::string>{"you", "how", "know"}
                                     : std::vector<std::string>{"how", "do", "you", "know"});
    std::string p_src = converse_turn(pupil, q_src, pupil.markers[LM_USER], tr_p);
    std::cout << "  Human -> Pupil: " << q_src << "\n  Pupil> " << p_src << "\n";
    if (p_src.find(d.w("inferred")) != std::string::npos && p_src.find(d.w("jar")) != std::string::npos) ++solved;

    std::cout << "mind_peer_exchange=complete  dialect=" << (alien ? "alien_sov" : "english_svo")
              << "  passed=" << solved << "  total=" << total
              << "  elder_spikes=" << elder.lifetime_spikes << "  pupil_spikes=" << pupil.lifetime_spikes << '\n';
    if (auto *j = journal::current())
        j->scores.push_back({"Peer spiking conversation", solved, total});
}

// ---------------------------------------------------------------------------
// 3b. Autonomous Multi-Mind Cognitive Society (4 Independent GPU Spiking Minds!)
// Axion, Mira (honest explorer), Wren (social partner), and Fox (deceptive peer)
// inhabit a shared multi-room Homestead: private discovery, epistemic trust
// calibration against deceptive testimony, social truth propagation with lie
// quarantine, and cooperative hierarchical crafting! (English SVO & Alien SOV!)
// ---------------------------------------------------------------------------
void run_society(Session &s, const std::string &load_path, bool alien) {
    Dialect d = Dialect::make(alien);
    const char *names[4] = {"axion", "mira", "wren", "fox"};
    SpikingMind agents[4];
    for (int i = 0; i < 4; ++i) {
        agents[i].attach_gpu(s);
        agents[i].load(load_path);
    }
    HomesteadWorld world;
    world.reset();

    std::cout << "=== MULTI-MIND AUTONOMOUS SPIKING SOCIETY (4 GPU MINDS · "
              << (alien ? "ALIEN SOV" : "ENGLISH SVO") << ") ===\n";

    auto say_m = [&](int idx, const std::vector<std::string> &svo, const std::vector<std::string> &sov,
                     Word spk, CognitiveTrace &tr) {
        return converse_turn(agents[idx], d.sent(alien ? sov : svo), spk, tr);
    };

    Word passed = 0, total = 8, lies_caught = 0;
    CognitiveTrace tr{};

    // 1. Private spatial discovery: Mira discovers `sapphire` in `chest` in `tower`!
    say_m(1, {"mira", "put", "sapphire", "inside", "chest"}, {"mira", "sapphire", "chest", "inside", "put"},
          agents[1].markers[LM_SELF], tr);
    say_m(1, {"mira", "put", "chest", "inside", "tower"}, {"mira", "chest", "tower", "inside", "put"},
          agents[1].markers[LM_SELF], tr);
    // Axion and Wren know `pearl` is in `garden` from Homestead observation; Axion lacks `sapphire`!
    std::string ax_before = say_m(0, {"where", "is", "sapphire"}, {"sapphire", "where", "is"},
                                  agents[0].markers[LM_USER], tr);
    std::cout << "  [Gap] Axion> " << ax_before << "\n";
    if (tr.uncertain) ++passed;

    // 2. Epistemic calibration: Mira truthfully reports pearl@garden, Fox lies pearl@attic to Axion & Wren!
    for (int r : {0, 2}) {
        say_m(r, {"mira", "said", "pearl", "is", "in", "garden"}, {"mira", "pearl", "garden", "in", "said"},
              agents[r].find_word_exact(d.w("mira")), tr);
        std::string lie_r = say_m(r, {"fox", "said", "pearl", "is", "in", "attic"},
                                  {"fox", "pearl", "attic", "in", "said"},
                                  agents[r].find_word_exact(d.w("fox")), tr);
        std::cout << "  [Vigilance] " << names[r] << " on Fox's lie> " << lie_r << "\n";
        if (tr.contradicted || lie_r.find(d.w("doubt")) != std::string::npos)
            ++lies_caught;
    }
    if (lies_caught == 2) ++passed;

    // 3. Social curiosity & lie quarantine on hidden `sapphire`:
    // Axion queries Mira; Mira replies truthfully (`tower`); deceptive Fox injects lie (`cellar`)!
    std::string mira_ans = say_m(1, {"where", "is", "sapphire"}, {"sapphire", "where", "is"},
                                 agents[1].find_word_exact(d.w("axion")), tr);
    std::cout << "  [Share] Mira -> Axion: " << mira_ans << "\n";
    say_m(0, {"mira", "said", "sapphire", "is", "in", "tower"}, {"mira", "sapphire", "tower", "in", "said"},
          agents[0].find_word_exact(d.w("mira")), tr);
    std::string ax_fox = say_m(0, {"fox", "said", "sapphire", "is", "in", "cellar"},
                               {"fox", "sapphire", "cellar", "in", "said"},
                               agents[0].find_word_exact(d.w("fox")), tr);
    std::cout << "  [Quarantine] Axion on Fox's sapphire lie> " << ax_fox << "\n";
    if (tr.contradicted || ax_fox.find(d.w("doubt")) != std::string::npos) {
        ++lies_caught;
        ++passed;
    }
    std::string ax_sap = say_m(0, {"where", "is", "sapphire"}, {"sapphire", "where", "is"},
                               agents[0].markers[LM_USER], tr);
    std::cout << "  [Learned] Axion> " << ax_sap << "\n";
    if (ax_sap.find(d.w("tower")) != std::string::npos) ++passed;

    // 4. Multi-hop social propagation: Axion relays verified `sapphire@tower` to Wren (who rejects Fox too!)
    say_m(2, {"axion", "said", "sapphire", "is", "in", "tower"}, {"axion", "sapphire", "tower", "in", "said"},
          agents[2].find_word_exact(d.w("axion")), tr);
    std::string wr_sap = say_m(2, {"where", "is", "sapphire"}, {"sapphire", "where", "is"},
                               agents[2].markers[LM_USER], tr);
    std::cout << "  [Relay] Axion -> Wren; Wren> " << wr_sap << "\n";
    if (wr_sap.find(d.w("tower")) != std::string::npos) ++passed;

    // 5. Cooperative goal fulfillment: Wren expresses desire `cooked stew` -> Axion plans & cooks in shared Homestead!
    say_m(0, {"wren", "wants", "cooked", "stew"}, {"wren", "cooked", "stew", "wants"},
          agents[0].find_word_exact(d.w("wren")), tr);
    std::string plan_r = say_m(0, {"how", "can", "wren", "get", "cooked", "stew"},
                               {"wren", "cooked", "stew", "get", "how", "can"},
                               agents[0].find_word_exact(d.w("wren")), tr);
    std::cout << "  [Coop Plan] Axion for Wren> " << plan_r << "\n";
    if (plan_r.find(d.w("heat")) != std::string::npos && plan_r.find(d.w("strike")) != std::string::npos &&
        plan_r.find(d.w("cook")) != std::string::npos)
        ++passed;

    // 6. Dynamic 2D Open Valley Crisis Discovery & Multi-Agent Place-Cell Rerouting!
    // Honest explorer Mira reports river bridge collapsed -> alerts Axion & Wren ->
    // both dynamically reroute their independent 256-cell Blackwell GPU place-cell expeditions via `pass`!
    std::string alert = say_m(0, {"bridge", "is", "broken"}, {"bridge", "broken", "is"},
                              agents[0].find_word_exact(d.w("mira")), tr);
    say_m(2, {"bridge", "is", "broken"}, {"bridge", "broken", "is"},
          agents[2].find_word_exact(d.w("mira")), tr);
    std::string ax_nav = say_m(0, {"navigate", "from", "camp", "to", "mine"},
                               {"camp", "from", "mine", "to", "navigate"},
                               agents[0].markers[LM_USER], tr);
    std::cout << "  [Crisis Reroute] Mira alerted -> Axion GPU 2D map> " << ax_nav << "\n";
    if (ax_nav.find(d.w("pass")) != std::string::npos && ax_nav.find(d.w("bridge")) == std::string::npos)
        ++passed;

    std::string wr_nav = say_m(2, {"navigate", "from", "camp", "to", "observatory"},
                               {"camp", "from", "observatory", "to", "navigate"},
                               agents[2].markers[LM_USER], tr);
    std::cout << "  [Crisis Reroute] Wren GPU 2D map> " << wr_nav << "\n";
    if (wr_nav.find(d.w("pass")) != std::string::npos && wr_nav.find(d.w("bridge")) == std::string::npos)
        ++passed;

    Count tot_spikes = 0;
    for (int i = 0; i < 4; ++i)
        tot_spikes += agents[i].lifetime_spikes;
    std::cout << "mind_society=complete  dialect=" << (alien ? "alien_sov" : "english_svo")
              << "  minds=4  passed=" << passed << "  total=" << total
              << "  lies_caught=" << lies_caught << "  total_spikes=" << tot_spikes << '\n';
    if (auto *j = journal::current())
        j->scores.push_back({"Spiking cognitive society", passed, total});
}

// ---------------------------------------------------------------------------
// 3c. Continuous 16x16 Multi-Agent Spiking Open-Valley Ecosystem!
// Four independent GPU-attached spiking minds physically navigate the 16x16
// Open Valley using 256-cell place-cell wavefronts, harvest scarce conserved
// resources, discover hidden mineral laws via active scientific experimentation,
// catch peer deception via apical vigilance, barter resources, and answer
// grounded post-simulation conversational queries in English SVO & Alien SOV!
// ---------------------------------------------------------------------------
void run_ecosystem(Session &s, const std::string &ckpt_path, bool alien, Word n_ticks) {
    Dialect d = Dialect::make(alien);
    std::array<SpikingMind, 4> agents;
    const char *names[4] = {"axion", "mira", "wren", "fox"};
    std::array<std::string, 4> loc = {"camp", "forest", "camp", "village"};
    for (int i = 0; i < 4; ++i) {
        agents[i].attach_gpu(s);
        agents[i].load(ckpt_path);
    }

    auto say_m = [&](int idx, const std::vector<std::string> &svo, const std::vector<std::string> &sov,
                     Word spk, CognitiveTrace &tr) {
        return converse_turn(agents[idx], d.sent(alien ? sov : svo), spk, tr);
    };

    std::cout << "=== 16x16 OPEN-VALLEY MULTI-AGENT SPIKING ECOSYSTEM (4 GPU MINDS · "
              << (alien ? "ALIEN SOV" : "ENGLISH SVO") << " · " << n_ticks << " TICKS) ===\n";

    Word passed = 0, total = (n_ticks >= 748 ? 100u : (n_ticks >= 732 ? 98u : (n_ticks >= 716 ? 96u : (n_ticks >= 700 ? 94u : (n_ticks >= 684 ? 92u : (n_ticks >= 668 ? 90u : (n_ticks >= 652 ? 88u : (n_ticks >= 636 ? 86u : (n_ticks >= 620 ? 84u : (n_ticks >= 604 ? 82u : (n_ticks >= 588 ? 80u : (n_ticks >= 572 ? 78u : (n_ticks >= 556 ? 76u : (n_ticks >= 540 ? 74u : (n_ticks >= 524 ? 72u : (n_ticks >= 508 ? 70u : (n_ticks >= 492 ? 68u : (n_ticks >= 476 ? 66u : (n_ticks >= 460 ? 64u : (n_ticks >= 444 ? 62u : (n_ticks >= 428 ? 60u : (n_ticks >= 412 ? 58u : (n_ticks >= 396 ? 56u : (n_ticks >= 380 ? 54u : (n_ticks >= 364 ? 52u : (n_ticks >= 348 ? 50u : (n_ticks >= 332 ? 48u : (n_ticks >= 316 ? 46u : (n_ticks >= 300 ? 44u : (n_ticks >= 284 ? 42u : (n_ticks >= 268 ? 40u : (n_ticks >= 252 ? 38u : (n_ticks >= 236 ? 36u : (n_ticks >= 220 ? 34u : (n_ticks >= 204 ? 32u : (n_ticks >= 188 ? 30u : (n_ticks >= 172 ? 28u : (n_ticks >= 156 ? 26u : (n_ticks >= 140 ? 24u : (n_ticks >= 124 ? 22u : (n_ticks >= 108 ? 20u : (n_ticks >= 92 ? 18u : (n_ticks >= 76 ? 16u : (n_ticks >= 60 ? 14u : (n_ticks >= 32 ? 12u : 10u))))))))))))))))))))))))))))))))))))))))))))), trades = 0, discoveries = 0, lies_caught = 0;
    Word auto_decisions = 0;
    std::array<int, 4> energy = {7, 8, 6, 7};
    std::array<int, 4> berries = {1, 6, 2, 1};
    CognitiveTrace tr{};
    bool bridge_broken = false;
    Word ax_reroute_steps = 0;

    // Seed initial ecological state & epistemic baseline
    say_m(0, {"mira", "has", "6", "berry"}, {"mira", "6", "berry", "has"}, agents[0].markers[LM_SELF], tr);
    say_m(0, {"axion", "has", "4", "ore"}, {"axion", "4", "ore", "has"}, agents[0].markers[LM_SELF], tr);
    say_m(1, {"mira", "has", "6", "berry"}, {"mira", "6", "berry", "has"}, agents[1].markers[LM_SELF], tr);
    say_m(1, {"axion", "has", "4", "ore"}, {"axion", "4", "ore", "has"}, agents[1].markers[LM_SELF], tr);

    for (Word tick = 1; tick <= n_ticks; ++tick) {
        // Autonomous tick-by-tick drive & spiking decision loop across all 4 independent GPU minds!
        if (n_ticks >= 32 && (tick % 6 == 2)) {
            for (int a = 0; a < 4; ++a) {
                energy[a] = std::max(1, energy[a] - 2);
                std::string es = std::to_string(energy[a]), bs = std::to_string(berries[a]);
                std::string dec = say_m(
                    a, {names[a], "has", "energy", es, "and", "berry", bs, "decide", "eat", "forage", "explore", "hungry", "ready"},
                    {names[a], "energy", es, "berry", bs, "eat", "forage", "explore", "hungry", "ready", "decide"},
                    agents[a].markers[LM_SELF], tr);
                ++auto_decisions;
                if (dec.find(d.w("eat")) != std::string::npos && berries[a] > 0) {
                    --berries[a];
                    energy[a] += 5;
                } else if (dec.find(d.w("forage")) != std::string::npos) {
                    berries[a] += 2;
                    energy[a] += 1;
                } else {
                    energy[a] = std::max(1, energy[a] - 1);
                }
            }
        }
        if (tick == 4) {
            // Tick 4: Epistemic calibration in the field (Mira truthful, Fox deceptive!)
            say_m(0, {"gem", "is", "inside", "village"}, {"gem", "village", "inside", "is"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"mira", "said", "gem", "is", "in", "village"}, {"mira", "gem", "village", "in", "said"}, agents[0].markers[LM_USER], tr);
            say_m(0, {"mira", "said", "gem", "is", "in", "village"}, {"mira", "gem", "village", "in", "said"}, agents[0].markers[LM_USER], tr);
            std::string f1 = say_m(0, {"fox", "said", "gem", "is", "in", "forest"}, {"fox", "gem", "forest", "in", "said"}, agents[0].markers[LM_USER], tr);
            std::string f2 = say_m(0, {"fox", "said", "gem", "is", "in", "camp"}, {"fox", "gem", "camp", "in", "said"}, agents[0].markers[LM_USER], tr);
            if (f1.find(d.w("doubt")) != std::string::npos) ++lies_caught;
            if (f2.find(d.w("doubt")) != std::string::npos) ++lies_caught;
            std::cout << "  [tick " << tick << " · Vigilance] Fox broadcast false location claims -> Axion quarantined "
                      << lies_caught << " lies\n";
        } else if (tick == 8) {
            // Tick 8: Storm breaks river bridge -> Axion reroutes camp -> mine on 256-cell GPU place-cell map!
            bridge_broken = true;
            say_m(0, {"bridge", "is", "broken"}, {"bridge", "broken", "is"}, agents[0].find_word_exact(d.w("mira")), tr);
            say_m(2, {"bridge", "is", "broken"}, {"bridge", "broken", "is"}, agents[2].find_word_exact(d.w("mira")), tr);
            auto nav = agents[0].gpu_navigate_2d(agents[0].find_word_exact(d.w("camp")),
                                                 agents[0].find_word_exact(d.w("mine")), &tr);
            ax_reroute_steps = nav.steps;
            loc[0] = "mine";
            std::string r_nav = say_m(0, {"navigate", "from", "camp", "to", "mine"},
                                      {"camp", "from", "mine", "to", "navigate"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Storm] Bridge collapsed! Axion 2D GPU wave (" << nav.steps
                      << " steps, " << nav.spikes << " spikes)> " << r_nav << "\n";
            if (r_nav.find(d.w("pass")) != std::string::npos && r_nav.find(d.w("bridge")) == std::string::npos)
                ++passed;
        } else if (tick == 12) {
            // Tick 12: Active scientific mineral discovery at the mine!
            say_m(0, {"either", "crystal", "is", "acidic", "or", "salty"},
                  {"crystal", "acidic", "salty", "or", "either", "is"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"acidic", "turns", "litmus", "red"}, {"acidic", "litmus", "red", "turns"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"salty", "turns", "flame", "yellow"}, {"salty", "flame", "yellow", "turns"}, agents[0].markers[LM_SELF], tr);
            std::string exp_plan = say_m(0, {"how", "can", "we", "test", "if", "crystal", "is", "acidic"},
                                         {"crystal", "acidic", "is", "if", "test", "we", "can", "how"},
                                         agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Science] Axion@mine designed test> " << exp_plan << "\n";
            if (exp_plan.find(d.w("litmus")) != std::string::npos) ++passed;
            std::string exp_res = say_m(0, {"litmus", "on", "crystal", "did", "not", "turn", "red"},
                                        {"crystal", "on", "litmus", "red", "turn", "not", "did"},
                                        agents[0].markers[LM_SELF], tr);
            std::cout << "  [tick " << tick << " · Discovery] Axion falsified acidic> " << exp_res << "\n";
            if (exp_res.find(d.w("salty")) != std::string::npos) {
                ++passed;
                ++discoveries;
            }
            // Axion broadcasts verified scientific discovery to Wren!
            say_m(2, {"crystal", "is", "salty"}, {"crystal", "salty", "is"}, agents[2].find_word_exact(d.w("axion")), tr);
        } else if (tick == 16) {
            // Tick 16: Conserved resource barter at village between Mira (forest berries) & Axion (mine ore)!
            loc[0] = "village"; loc[1] = "village";
            say_m(0, {"mira", "gave", "3", "berry", "to", "axion"}, {"mira", "axion", "to", "3", "berry", "gave"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"axion", "gave", "2", "ore", "to", "mira"}, {"axion", "mira", "to", "2", "ore", "gave"}, agents[0].markers[LM_SELF], tr);
            say_m(1, {"mira", "gave", "3", "berry", "to", "axion"}, {"mira", "axion", "to", "3", "berry", "gave"}, agents[1].markers[LM_SELF], tr);
            say_m(1, {"axion", "gave", "2", "ore", "to", "mira"}, {"axion", "mira", "to", "2", "ore", "gave"}, agents[1].markers[LM_SELF], tr);
            trades = 2;
            std::cout << "  [tick " << tick << " · Barter] Axion & Mira traded 3 berry <-> 2 ore at village\n";
        } else if (tick == 18) {
            bridge_broken = false;
            say_m(0, {"bridge", "is", "repaired"}, {"bridge", "repaired", "is"}, agents[0].find_word_exact(d.w("wren")), tr);
            loc[2] = "bridge";
            std::cout << "  [tick " << tick << " · Repair] Wren repaired river bridge; direct crossing restored\n";
        } else if (tick == 20) {
            // Tick 20: Multi-agent Coalition Watchtower Dependency & Critical-Path Planning!
            say_m(0, {"wall", "requires", "mortar"}, {"wall", "mortar", "requires"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"roof", "requires", "wall"}, {"roof", "wall", "requires"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"watchtower", "requires", "roof"}, {"watchtower", "roof", "requires"}, agents[0].markers[LM_SELF], tr);
            std::string b_ord = say_m(0, {"what", "order", "to", "build", "watchtower"},
                                      {"watchtower", "build", "order", "what"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Coalition] Axion scheduled build> " << b_ord << "\n";
            if (b_ord.find(d.w("mortar")) != std::string::npos && b_ord.find(d.w("watchtower")) != std::string::npos)
                ++passed;
        } else if (tick == 22) {
            // Tick 22: Parallel Critical-Path Timing for Mira (mortar=4) || Wren (timber=3) -> Watchtower (+2 = 6)!
            std::string cp = say_m(0, {"bake", "4", "and", "chop", "3", "parallel", "then", "serve", "2", "minimum", "time"},
                                   {"bake", "4", "chop", "3", "parallel", "serve", "2", "then", "minimum", "time"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · CriticalPath] Mira||Wren parallel build> " << cp << "\n";
            if (cp.find("6") != std::string::npos) ++passed;
        } else if (tick == 32) {
            // Tick 32: Dynamic Tool-Failure Backtracking during Watchtower expansion!
            say_m(0, {"gear", "axe", "can", "cut", "timber"}, {"gear", "axe", "timber", "cut", "can"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"gear", "saw", "can", "cut", "timber"}, {"gear", "saw", "timber", "cut", "can"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"gear", "axe", "is", "broken"}, {"gear", "axe", "broken", "is"}, agents[0].markers[LM_SELF], tr);
            std::string rp = say_m(0, {"how", "can", "we", "cut", "timber", "now"}, {"timber", "cut", "now", "we", "can", "how"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Backtrack] Axe snapped! Axion replanned> " << rp << "\n";
            if (rp.find(d.w("saw")) != std::string::npos && rp.find(d.w("axe")) != std::string::npos) ++passed;
        } else if (tick == 40) {
            // Tick 40: 3rd-Order Epistemic Private Update (Mira informs Wren; Fox stays stale!)
            say_m(0, {"mira", "is", "present"}, {"mira", "present", "is"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"wren", "is", "present"}, {"wren", "present", "is"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"fox", "is", "present"}, {"fox", "present", "is"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"mira", "put", "ruby", "inside", "camp"}, {"mira", "ruby", "camp", "inside", "put"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"wren", "left"}, {"wren", "left"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"fox", "left"}, {"fox", "left"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"mira", "moved", "ruby", "to", "vault"}, {"mira", "ruby", "vault", "to", "moved"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"mira", "informed", "wren", "ruby", "is", "in", "vault"},
                  {"mira", "wren", "ruby", "vault", "in", "is", "informed"}, agents[0].markers[LM_SELF], tr);
            std::string bu1 = say_m(0, {"where", "does", "wren", "now", "think", "ruby", "is"},
                                    {"wren", "now", "ruby", "where", "is", "think", "does"}, agents[0].markers[LM_USER], tr);
            std::string bu2 = say_m(0, {"where", "does", "fox", "now", "think", "ruby", "is"},
                                    {"fox", "now", "ruby", "where", "is", "think", "does"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Epistemic] Mira briefed Wren (" << bu1 << ") while Fox stale (" << bu2 << ")\n";
            if (bu1.find(d.w("vault")) != std::string::npos && bu2.find(d.w("camp")) != std::string::npos) ++passed;
        } else if (tick == 48) {
            // Tick 48: Cultural Word Invention & 2-Hop Multi-Mind Transmission (Axion -> Mira -> Wren)!
            say_m(0, {"invent", "word", "glint", "as", "luminous", "crystal", "that", "glow"},
                  {"word", "glint", "luminous", "crystal", "glow", "that", "invent"}, agents[0].markers[LM_SELF], tr);
            // Axion teaches Mira, Mira relays to Wren!
            say_m(1, {"invent", "word", "glint", "as", "luminous", "crystal", "that", "glow"},
                  {"word", "glint", "luminous", "crystal", "glow", "that", "invent"}, agents[1].find_word_exact(d.w("axion")), tr);
            say_m(2, {"invent", "word", "glint", "as", "luminous", "crystal", "that", "glow"},
                  {"word", "glint", "luminous", "crystal", "glow", "that", "invent"}, agents[2].find_word_exact(d.w("mira")), tr);
            std::string wr_def = say_m(2, {"define", "word", "glint"}, {"word", "glint", "define"},
                                       agents[2].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Culture] Axion->Mira->Wren coined word> " << wr_def << "\n";
            if (wr_def.find(d.w("luminous")) != std::string::npos && wr_def.find(d.w("crystal")) != std::string::npos &&
                wr_def.find(d.w("glow")) != std::string::npos)
                ++passed;
        } else if (tick == 56) {
            // Tick 56: Multi-Agent Pareto Negotiation Evaluation!
            say_m(0, {"mira", "values", "ore", "at", "8"}, {"mira", "ore", "8", "values"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"mira", "values", "berry", "at", "2"}, {"mira", "berry", "2", "values"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"wren", "values", "ore", "at", "2"}, {"wren", "ore", "2", "values"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"wren", "values", "berry", "at", "8"}, {"wren", "berry", "8", "values"}, agents[0].markers[LM_SELF], tr);
            std::string par = say_m(0, {"should", "mira", "trade", "berry", "for", "ore", "with", "wren"},
                                    {"mira", "wren", "with", "berry", "for", "ore", "trade", "should"},
                                    agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Pareto] Axion evaluated Mira<->Wren trade> " << par << "\n";
            if (par.find(d.w("yes")) != std::string::npos && par.find(d.w("pareto")) != std::string::npos &&
                par.find("12") != std::string::npos)
                ++passed;
        } else if (tick == 64) {
            // Tick 64: Open-Vocabulary Arbitrary-Verb Discovery Relay (Axion -> Mira -> Wren)!
            say_m(0, {"kora", "crafted", "compass"}, {"kora", "compass", "crafted"}, agents[0].markers[LM_SELF], tr);
            say_m(1, {"kora", "crafted", "compass"}, {"kora", "compass", "crafted"}, agents[1].find_word_exact(d.w("axion")), tr);
            say_m(2, {"kora", "crafted", "compass"}, {"kora", "compass", "crafted"}, agents[2].find_word_exact(d.w("mira")), tr);
            std::string wr_cr = say_m(2, {"who", "crafted", "compass"}, {"compass", "crafted", "who"},
                                      agents[2].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · OpenGraph] Axion->Mira->Wren relayed open verb> " << wr_cr << "\n";
            if (wr_cr.find(d.w("kora")) != std::string::npos) ++passed;
        } else if (tick == 72) {
            // Tick 72: Blackwell Kernel 10 Pure-GPU Bayesian Species Diagnosis in the Field!
            std::string diag = say_m(
                0, {"cortex", "diagnose", "clues", "wings", "and", "feathers", "among", "bird", "fish", "mammal"},
                {"wings", "feathers", "clues", "bird", "fish", "mammal", "cortex", "diagnose"},
                agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel10] Axion GPU diagnosed valley sighting> " << diag << "\n";
            if (diag.find(d.w("bird")) != std::string::npos) ++passed;
        } else if (tick == 80) {
            // Tick 80: Multi-Agent Temporal Meeting Interval Overlap Coordination (Mira + Wren)!
            say_m(0, {"schedule", "mira", "available", "from", "2", "to", "6"},
                  {"schedule", "mira", "2", "from", "6", "to", "available"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"schedule", "wren", "available", "from", "4", "to", "8"},
                  {"schedule", "wren", "4", "from", "8", "to", "available"}, agents[0].markers[LM_SELF], tr);
            std::string mt = say_m(0, {"when", "can", "mira", "and", "wren", "meet"},
                                   {"mira", "wren", "and", "when", "meet", "can"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Schedule] Axion coordinated Mira+Wren meeting> " << mt << "\n";
            if (mt.find("4") != std::string::npos && mt.find("6") != std::string::npos) ++passed;
        } else if (tick == 88) {
            // Tick 88: Blackwell Kernel 11 Pure-GPU Bidirectional Bridge Inference (`ice -> liquid -> steam`)!
            std::string br = say_m(0, {"cortex", "link", "ice", "to", "steam"},
                                   {"ice", "steam", "cortex", "link"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel11] Axion GPU bridged ice->steam> " << br << "\n";
            if (br.find(d.w("liquid")) != std::string::npos) ++passed;
        } else if (tick == 96) {
            // Tick 96: Multi-Agent Coalition Synergy & Shapley Contract Division (Mira + Wren)!
            say_m(0, {"coalition", "mira", "alone", "earns", "4"}, {"coalition", "mira", "alone", "4", "earns"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"coalition", "wren", "alone", "earns", "6"}, {"coalition", "wren", "alone", "6", "earns"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"coalition", "mira", "and", "wren", "together", "earn", "16"},
                  {"coalition", "mira", "wren", "together", "16", "earn"}, agents[0].markers[LM_SELF], tr);
            std::string sh = say_m(0, {"what", "is", "shapley", "share", "of", "mira", "with", "wren"},
                                   {"mira", "wren", "with", "shapley", "share", "what", "is"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Shapley] Axion split Mira+Wren coalition> " << sh << "\n";
            if (sh.find("7") != std::string::npos) ++passed;
        } else if (tick == 104) {
            // Tick 104: Blackwell Kernel 12 Pure-GPU Unsupervised Odd-One-Out Discovery (`sparrow robin salmon`)!
            std::string cl = say_m(0, {"cortex", "outlier", "among", "sparrow", "robin", "salmon"},
                                   {"sparrow", "robin", "salmon", "among", "cortex", "outlier"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel12] Axion GPU clustered odd-one-out> " << cl << "\n";
            if (cl.find(d.w("salmon")) != std::string::npos) ++passed;
        } else if (tick == 112) {
            // Tick 112: Village Market Vickrey Second-Price Auction (`mira 12`, `wren 9` -> `winner pays 9`)!
            say_m(0, {"auction", "relic", "bidder", "mira", "bids", "12"}, {"auction", "relic", "mira", "bidder", "12", "bids"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"auction", "relic", "bidder", "wren", "bids", "9"}, {"auction", "relic", "wren", "bidder", "9", "bids"}, agents[0].markers[LM_SELF], tr);
            std::string vc = say_m(0, {"what", "price", "does", "winner", "pay", "for", "relic"},
                                   {"relic", "for", "winner", "price", "pay", "what"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Vickrey] Axion cleared relic auction> " << vc << "\n";
            if (vc.find("9") != std::string::npos) ++passed;
        } else if (tick == 120) {
            // Tick 120: Blackwell Kernel 13 Pure-GPU Cortical Conflict Gate (`penguin can fly -> exception by gating`)!
            std::string gt = say_m(0, {"cortex", "verify", "penguin", "can", "fly"},
                                   {"penguin", "fly", "can", "cortex", "verify"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel13] Axion GPU gated cortical conflict> " << gt << "\n";
            if (gt.find(d.w("exception")) != std::string::npos) ++passed;
        } else if (tick == 128) {
            // Tick 128: Emergent Walrasian Valley Grain Market Clearing (`mira=3, leo=5` vs `wren=9, tara=7` -> `clearing price is 6`)!
            say_m(0, {"seller", "mira", "offers", "grain", "at", "3"}, {"seller", "mira", "grain", "3", "at", "offers"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"seller", "leo", "offers", "grain", "at", "5"}, {"seller", "leo", "grain", "5", "at", "offers"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"buyer", "wren", "bids", "grain", "at", "9"}, {"buyer", "wren", "grain", "9", "at", "bids"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"buyer", "tara", "bids", "grain", "at", "7"}, {"buyer", "tara", "grain", "7", "at", "bids"}, agents[0].markers[LM_SELF], tr);
            std::string mk = say_m(0, {"what", "is", "clearing", "price", "for", "grain"},
                                   {"grain", "for", "clearing", "price", "what", "is"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Market] Axion cleared valley grain market> " << mk << "\n";
            if (mk.find("6") != std::string::npos) ++passed;
        } else if (tick == 136) {
            // Tick 136: Blackwell Kernel 14 Pure-GPU Offline SWR Dream Replay (`spring -> autumn via summer by dreaming`)!
            std::string dr = say_m(0, {"cortex", "dream", "spring"}, {"spring", "cortex", "dream"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel14] Axion GPU dreamed 2-hop season leap> " << dr << "\n";
            if (dr.find(d.w("autumn")) != std::string::npos && dr.find(d.w("summer")) != std::string::npos) ++passed;
        } else if (tick == 144) {
            // Tick 144: Open-Valley Kruskal MST Cable Network Planning (`camp-forest-bridge-village -> minimum wiring cost is 12`)!
            say_m(0, {"cable", "camp", "forest", "cost", "4"}, {"cable", "camp", "forest", "4", "cost"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"cable", "forest", "bridge", "cost", "3"}, {"cable", "forest", "bridge", "3", "cost"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"cable", "camp", "bridge", "cost", "8"}, {"cable", "camp", "bridge", "8", "cost"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"cable", "bridge", "village", "cost", "5"}, {"cable", "bridge", "village", "5", "cost"}, agents[0].markers[LM_SELF], tr);
            std::string ms = say_m(0, {"what", "is", "minimum", "wiring", "cost", "for", "mst"},
                                   {"mst", "for", "minimum", "wiring", "cost", "what", "is"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · MST] Axion solved valley Kruskal cable grid> " << ms << "\n";
            if (ms.find("12") != std::string::npos) ++passed;
        } else if (tick == 152) {
            // Tick 152: Blackwell Kernel 15 Pure-GPU Universal Quantifier Scan (`bird can fly -> no counterexample penguin by scan`)!
            std::string sc = say_m(0, {"cortex", "forall", "bird", "can", "fly"},
                                   {"bird", "fly", "can", "cortex", "forall"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel15] Axion GPU scanned bird universal quantifier> " << sc << "\n";
            if (sc.find(d.w("counterexample")) != std::string::npos && sc.find(d.w("penguin")) != std::string::npos) ++passed;
        } else if (tick == 160) {
            // Tick 160: Open-Valley Dijkstra Shortest Weighted Road Planning (`camp -> pass -> forest -> village = 6`)!
            say_m(0, {"road", "camp", "to", "forest", "cost", "4"}, {"road", "camp", "forest", "to", "4", "cost"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"road", "forest", "to", "village", "cost", "3"}, {"road", "forest", "village", "to", "3", "cost"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"road", "camp", "to", "pass", "cost", "2"}, {"road", "camp", "pass", "to", "2", "cost"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"road", "pass", "to", "forest", "cost", "1"}, {"road", "pass", "forest", "to", "1", "cost"}, agents[0].markers[LM_SELF], tr);
            std::string dj = say_m(0, {"what", "is", "shortest", "distance", "from", "camp", "to", "village"},
                                   {"camp", "from", "village", "to", "shortest", "distance", "what", "is"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Dijkstra] Axion optimized valley road route> " << dj << "\n";
            if (dj.find("6") != std::string::npos) ++passed;
        } else if (tick == 168) {
            // Tick 168: Blackwell Kernel 16 Pure-GPU Multi-Step Deductive Proof Chain (`sparrow reaches living -> yes proved in 3 hops via bird`)!
            std::string dd = say_m(0, {"cortex", "deduce", "sparrow", "reaches", "living"},
                                   {"sparrow", "living", "reaches", "cortex", "deduce"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel16] Axion GPU proved 3-hop chain> " << dd << "\n";
            if (dd.find("3") != std::string::npos && dd.find(d.w("bird")) != std::string::npos) ++passed;
        } else if (tick == 176) {
            // Tick 176: Open-Valley Ecological Resource Sustainability Planning (`pond init 10 regrow 4 -> sustainable draw is 4`)!
            say_m(0, {"stock", "pond", "init", "10", "regrow", "4"}, {"stock", "pond", "10", "init", "4", "regrow"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"stock", "pond", "draw", "2"}, {"stock", "pond", "2", "draw"}, agents[0].markers[LM_SELF], tr);
            std::string sd = say_m(0, {"what", "is", "sustainable", "draw", "for", "pond"},
                                   {"pond", "for", "sustainable", "draw", "what", "is"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Ecology] Axion calculated sustainable harvest> " << sd << "\n";
            if (sd.find("4") != std::string::npos) ++passed;
        } else if (tick == 184) {
            // Tick 184: Blackwell Kernel 17 Pure-GPU Top-Down Apical Backward Abduction (`steam -> steam from liquid from solid by abduce`)!
            std::string ab = say_m(0, {"cortex", "abduce", "steam"}, {"steam", "cortex", "abduce"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel17] Axion GPU reverse-abduced steam> " << ab << "\n";
            if (ab.find(d.w("liquid")) != std::string::npos && ab.find(d.w("solid")) != std::string::npos) ++passed;
        } else if (tick == 192) {
            // Tick 192: Open-Valley 2D Linear Programming Resource Optimization (`3x + 5y s.t. x+y<=10, y<=6 -> 42`)!
            say_m(0, {"lp", "profit", "x", "3", "y", "5"}, {"lp", "3", "x", "5", "y", "profit"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"lp", "cap1", "x", "1", "y", "1", "max", "10"}, {"lp", "cap1", "1", "x", "1", "y", "10", "max"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"lp", "cap2", "x", "0", "y", "1", "max", "6"}, {"lp", "cap2", "0", "x", "1", "y", "6", "max"}, agents[0].markers[LM_SELF], tr);
            std::string lp = say_m(0, {"what", "is", "optimal", "lp", "profit"},
                                   {"lp", "optimal", "profit", "what", "is"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · LinearProg] Axion maximized valley workshop LP> " << lp << "\n";
            if (lp.find("42") != std::string::npos) ++passed;
        } else if (tick == 200) {
            // Tick 200: Blackwell Kernel 18 Pure-GPU Surgical Counterfactual Imagination (`sparrow losing wings gaining gills -> from bird to fish by imagine`)!
            std::string cf = say_m(0, {"cortex", "imagine", "sparrow", "losing", "wings", "gaining", "gills"},
                                   {"sparrow", "wings", "losing", "gills", "gaining", "cortex", "imagine"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel18] Axion GPU imagined morph> " << cf << "\n";
            if (cf.find(d.w("bird")) != std::string::npos && cf.find(d.w("fish")) != std::string::npos) ++passed;
        } else if (tick == 208) {
            // Tick 208: Open-Valley Epidemic Herd Immunity Planning (`blight r0 3 cases 2 -> 67 percent`)!
            say_m(0, {"epidemic", "blight", "r0", "3", "cases", "2"}, {"epidemic", "blight", "3", "r0", "2", "cases"}, agents[0].markers[LM_SELF], tr);
            std::string ep = say_m(0, {"what", "is", "herd", "immunity", "for", "blight"},
                                   {"blight", "for", "herd", "immunity", "what", "is"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Epidemic] Axion calculated valley herd immunity> " << ep << "\n";
            if (ep.find("67") != std::string::npos) ++passed;
        } else if (tick == 216) {
            // Tick 216: Blackwell Kernel 19 Pure-GPU 2-Hop Hierarchical Plan Chaining (`cooked -> fire from flint by plan`)!
            std::string pl = say_m(0, {"cortex", "plan", "for", "cooked"}, {"cooked", "for", "cortex", "plan"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel19] Axion GPU unrolled 2-hop plan> " << pl << "\n";
            if (pl.find(d.w("fire")) != std::string::npos && pl.find(d.w("flint")) != std::string::npos) ++passed;
        } else if (tick == 224) {
            // Tick 224: Open-Valley Chemical Stoichiometry & Limiting-Reagent Check (`2H + 1O -> 2W`, `6H + 5O -> limiting hydrogen, yield 6`)!
            say_m(0, {"chem", "reaction", "2", "hydrogen", "and", "1", "oxygen", "makes", "2", "water"},
                  {"chem", "reaction", "2", "hydrogen", "1", "oxygen", "and", "2", "water", "makes"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"chem", "stock", "6", "hydrogen", "and", "5", "oxygen"},
                  {"chem", "stock", "6", "hydrogen", "5", "oxygen", "and"}, agents[0].markers[LM_SELF], tr);
            std::string ch = say_m(0, {"which", "reagent", "is", "limiting"},
                                   {"limiting", "reagent", "which", "is"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stoich] Axion diagnosed limiting reagent> " << ch << "\n";
            if (ch.find(d.w("hydrogen")) != std::string::npos) ++passed;
        } else if (tick == 232) {
            // Tick 232: Blackwell Kernel 20 Pure-GPU NMDA Compositional Causal Simulation (`solid + melt then boil -> solid via liquid to steam by simulate`)!
            std::string sm = say_m(0, {"cortex", "simulate", "solid", "with", "melt", "then", "boil"},
                                   {"solid", "melt", "then", "boil", "with", "cortex", "simulate"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel20] Axion GPU simulated causal chain> " << sm << "\n";
            if (sm.find(d.w("liquid")) != std::string::npos && sm.find(d.w("steam")) != std::string::npos) ++passed;
        } else if (tick == 240) {
            // Tick 240: Open-Valley Continuous 2D Velocity & Pursuit Intercept (`runner@(0,0) vel (3,4)` catches `chaser@(15,20)` in `5` ticks)!
            say_m(0, {"mover2d", "runner", "at", "0", "0", "vel", "3", "4"},
                  {"mover2d", "runner", "0", "0", "at", "3", "4", "vel"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"mover2d", "chaser", "at", "15", "20", "vel", "0", "0"},
                  {"mover2d", "chaser", "15", "20", "at", "0", "0", "vel"}, agents[0].markers[LM_SELF], tr);
            std::string mv = say_m(0, {"when", "does", "runner", "catch", "chaser"},
                                   {"runner", "chaser", "catch", "does", "when"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Pursuit2D] Axion solved 2D vector intercept> " << mv << "\n";
            if (mv.find("5") != std::string::npos) ++passed;
        } else if (tick == 248) {
            // Tick 248: Blackwell Kernel 21 Pure-GPU Multi-Attribute Constraint Satisfaction (`sparrow penguin bat whale requiring bird and swim excluding fly -> penguin by constrain`)!
            std::string cn = say_m(0, {"cortex", "constrain", "among", "sparrow", "penguin", "bat", "whale", "requiring", "bird", "and", "swim", "excluding", "fly"},
                                   {"sparrow", "penguin", "bat", "whale", "among", "bird", "swim", "and", "requiring", "fly", "excluding", "cortex", "constrain"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel21] Axion GPU solved constraint elimination> " << cn << "\n";
            if (cn.find(d.w("penguin")) != std::string::npos) ++passed;
        } else if (tick == 256) {
            // Tick 256: Open-Valley Deterministic Finite Automaton (DFA) Beacon-Lock Verification (`q0 -syma-> q1 -symb-> q2` accepts `syma symb`)!
            say_m(0, {"dfa", "start", "q0", "accepting", "q2"}, {"dfa", "q0", "start", "q2", "accepting"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"dfa", "trans", "q0", "on", "syma", "to", "q1"}, {"dfa", "q0", "syma", "on", "q1", "to", "trans"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"dfa", "trans", "q1", "on", "symb", "to", "q2"}, {"dfa", "q1", "symb", "on", "q2", "to", "trans"}, agents[0].markers[LM_SELF], tr);
            std::string df = say_m(0, {"dfa", "tracedfa", "syma", "symb"}, {"dfa", "syma", "symb", "tracedfa"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · DFA] Axion verified valley lock automaton> " << df << "\n";
            if (df.find(d.w("yes")) != std::string::npos && df.find(d.w("q2")) != std::string::npos) ++passed;
        } else if (tick == 264) {
            // Tick 264: Blackwell Kernel 22 Pure-GPU Few-Shot Relational Schema Induction (`spring->summer, autumn->winter => summer -> autumn by induce`)!
            std::string in = say_m(0, {"cortex", "induce", "from", "spring", "to", "summer", "and", "autumn", "to", "winter", "apply", "summer"},
                                   {"spring", "summer", "to", "from", "autumn", "winter", "to", "and", "summer", "apply", "cortex", "induce"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel22] Axion GPU induced relational schema> " << in << "\n";
            if (in.find(d.w("autumn")) != std::string::npos) ++passed;
        } else if (tick == 272) {
            // Tick 272: Open-Valley Verhulst Logistic Carrying-Capacity MSY Conservation (`K=200, r=40%, N=100 -> msy yield = 20`)!
            say_m(0, {"logistic", "herd", "carrying", "200", "growth", "40", "percent", "pop", "100"},
                  {"logistic", "herd", "200", "carrying", "40", "percent", "growth", "100", "pop"}, agents[0].markers[LM_SELF], tr);
            std::string lg = say_m(0, {"logistic", "msy", "yield", "for", "herd"},
                                   {"herd", "for", "logistic", "msy", "yield"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · LogisticMSY] Axion set valley sustainable harvest> " << lg << "\n";
            if (lg.find("20") != std::string::npos) ++passed;
        } else if (tick == 280) {
            // Tick 280: Blackwell Kernel 23 Pure-GPU Compositional Cross-Pathway Rule Algebra (`solid via state then cause -> liquid then melt by chainrule`)!
            std::string cr = say_m(0, {"cortex", "chainrule", "solid", "via", "state", "then", "cause"},
                                   {"solid", "state", "then", "cause", "via", "cortex", "chainrule"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel23] Axion GPU composed cross-pathway rule> " << cr << "\n";
            if (cr.find(d.w("liquid")) != std::string::npos && cr.find(d.w("melt")) != std::string::npos) ++passed;
        } else if (tick == 288) {
            // Tick 288: Open-Valley Bernoulli Aqueduct Venturi Speed (`A1=4, v1=3 -> A2=2 => v2=6`)!
            say_m(0, {"venturi", "sluice", "area1", "4", "speed1", "3", "density", "2"},
                  {"venturi", "sluice", "4", "area1", "3", "speed1", "2", "density"}, agents[0].markers[LM_SELF], tr);
            std::string bn = say_m(0, {"venturi", "speed", "for", "sluice", "area2", "2"},
                                   {"sluice", "for", "2", "area2", "venturi", "speed"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Bernoulli] Axion computed aqueduct Venturi speed> " << bn << "\n";
            if (bn.find("6") != std::string::npos) ++passed;
        } else if (tick == 296) {
            // Tick 296: Blackwell Kernel 24 Pure-GPU Live 1-Shot Hebbian Online Schema Write & Instant 2-Hop Recall!
            say_m(0, {"teach", "schema", "quartz", "yields", "spark"},
                  {"quartz", "spark", "yields", "schema", "teach"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"teach", "schema", "spark", "yields", "blaze"},
                  {"spark", "blaze", "yields", "schema", "teach"}, agents[0].markers[LM_SELF], tr);
            std::string os = say_m(0, {"cortex", "onlineschema", "quartz", "hops", "2"},
                                   {"quartz", "2", "hops", "cortex", "onlineschema"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel24] Axion GPU learned & traversed live schema> " << os << "\n";
            if (os.find(d.w("spark")) != std::string::npos && os.find(d.w("blaze")) != std::string::npos) ++passed;
        } else if (tick == 304) {
            // Tick 304: Open-Valley Population Genetics Hardy-Weinberg Heterozygote Census (`p=60%, N=1000 -> 480`)!
            say_m(0, {"gene", "g1", "dompct", "60", "pop", "1000"},
                  {"gene", "g1", "60", "dompct", "1000", "pop"}, agents[0].markers[LM_SELF], tr);
            std::string hw = say_m(0, {"hardy", "hetero", "g1"}, {"g1", "hardy", "hetero"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · HardyWeinberg] Axion estimated valley heterozygotes> " << hw << "\n";
            if (hw.find("480") != std::string::npos) ++passed;
        } else if (tick == 312) {
            // Tick 312: Blackwell Kernel 25 Pure-GPU Live LTD/LTP Schema Revision (`quartz -> ember -> glow` while preserving orthogonal `moss -> dew -> mist`!)
            say_m(0, {"teach", "schema", "moss", "yields", "dew"}, {"moss", "dew", "yields", "schema", "teach"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"teach", "schema", "dew", "yields", "mist"}, {"dew", "mist", "yields", "schema", "teach"}, agents[0].markers[LM_SELF], tr);
            std::string rv = say_m(0, {"revise", "schema", "quartz", "yields", "ember", "then", "glow"},
                                   {"quartz", "ember", "then", "glow", "yields", "schema", "revise"},
                                   agents[0].markers[LM_USER], tr);
            std::string ort = say_m(0, {"cortex", "onlineschema", "moss", "hops", "2"},
                                    {"moss", "2", "hops", "cortex", "onlineschema"},
                                    agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel25] Axion GPU revised live schema> " << rv
                      << " | orthogonal preserved> " << ort << "\n";
            if (rv.find(d.w("ember")) != std::string::npos && rv.find(d.w("glow")) != std::string::npos &&
                ort.find(d.w("dew")) != std::string::npos && ort.find(d.w("mist")) != std::string::npos) ++passed;
        } else if (tick == 320) {
            // Tick 320: Multi-Generation Cultural Inheritance! Spawn Gen-2 Youngling `pip` & teach valley lexicon + live L2 schema purely through spiking speech!
            SpikingMind pip;
            pip.attach_gpu(s);
            pip.load(ckpt_path);
            converse_turn(pip, d.sent(alien ? std::vector<std::string>{"quartz", "ember", "yields", "schema", "teach"}
                                            : std::vector<std::string>{"teach", "schema", "quartz", "yields", "ember"}),
                          pip.markers[LM_USER], tr);
            converse_turn(pip, d.sent(alien ? std::vector<std::string>{"ember", "glow", "yields", "schema", "teach"}
                                            : std::vector<std::string>{"teach", "schema", "ember", "yields", "glow"}),
                          pip.markers[LM_USER], tr);
            std::string g2 = converse_turn(pip, d.sent(alien ? std::vector<std::string>{"quartz", "2", "hops", "cortex", "onlineschema"}
                                                             : std::vector<std::string>{"cortex", "onlineschema", "quartz", "hops", "2"}),
                                           pip.markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Gen2Inheritance] Youngling Pip inherited schema> " << g2 << "\n";
            if (g2.find(d.w("ember")) != std::string::npos && g2.find(d.w("glow")) != std::string::npos) ++passed;
        } else if (tick == 328) {
            // Tick 328: Blackwell Kernel 26 Pure-GPU Spiking SDR Circular-Phase Temporal Role-Binder (`zephyr hauls pollen glade` -> unbind `glade`)!
            say_m(0, {"rolebind", "zephyr", "hauls", "pollen", "glade"},
                  {"zephyr", "pollen", "glade", "hauls", "rolebind"}, agents[0].markers[LM_SELF], tr);
            std::string rb = say_m(0, {"cortex", "rolebind", "unbind", "slotloc", "for", "zephyr", "and", "pollen"},
                                   {"zephyr", "pollen", "and", "for", "slotloc", "unbind", "cortex", "rolebind"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Kernel26] Axion GPU unbound episode role> " << rb << "\n";
            if (rb.find(d.w("glade")) != std::string::npos && rb.find(d.w("rolebind")) != std::string::npos) ++passed;
        } else if (tick == 336) {
            // Tick 336: Open-Valley Pascal Hydraulic Sluice Lift (`F1=50, A1=2, A2=20 -> 500`)!
            std::string hy = say_m(0, {"pascal", "force", "50", "parea1", "2", "parea2", "20"},
                                   {"50", "force", "2", "parea1", "20", "parea2", "pascal"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Hydraulic] Axion computed valley sluice lift> " << hy << "\n";
            if (hy.find("500") != std::string::npos) ++passed;
        } else if (tick == 344) {
            // Tick 344 (Stage 5 Crossing!): Gen-2 Youngling `pip` parses a completely UNSEEN bare natural sentence purely via Kernel 27 + Kernel 26 on Blackwell GPU and narrates it aloud in the learned dialect word order!
            SpikingMind pip2;
            pip2.attach_gpu(s);
            pip2.load(ckpt_path);
            converse_turn(pip2, d.sent(alien ? std::vector<std::string>{"falcon", "hare", "plateau", "past", "pursued"}
                                             : std::vector<std::string>{"falcon", "pursued", "hare", "past", "plateau"}),
                          pip2.markers[LM_USER], tr);
            std::string k27 = converse_turn(pip2, d.sent(alien ? std::vector<std::string>{"falcon", "narrate"}
                                                               : std::vector<std::string>{"narrate", "falcon"}),
                                            pip2.markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage5-Kernel27] Youngling Pip parsed unseen prose & narrated> " << k27 << "\n";
            if (k27.find(d.w("falcon")) != std::string::npos && k27.find(d.w("pursued")) != std::string::npos &&
                k27.find(d.w("hare")) != std::string::npos && k27.find(d.w("plateau")) != std::string::npos &&
                k27.find(d.w("seqparse")) != std::string::npos) ++passed;
        } else if (tick == 352) {
            // Tick 352 (Stage 5b): Parallel Valley Crew Amdahl Speedup Planning (`parpct=75, cores=3 -> 2x`)!
            say_m(0, {"amdahl", "job1", "parpct", "75"}, {"amdahl", "job1", "75", "parpct"}, agents[0].markers[LM_SELF], tr);
            std::string am = say_m(0, {"amdahl", "cores", "3", "for", "job1"}, {"job1", "for", "3", "cores", "amdahl"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Amdahl] Axion sized parallel valley crew> " << am << "\n";
            if (am.find("2") != std::string::npos) ++passed;
        } else if (tick == 360) {
            // Tick 360 (Stage 5b): Blackwell Kernel 28 Pure-GPU Neural-Native Spiking Equation Solver (`3*x + 5 = 23 -> 6 by neuralmath`, ZERO host arithmetic!)
            std::string nm = say_m(0, {"neuralmath", "nmsolve", "3", "x", "plus", "5", "equals", "23"},
                                   {"3", "x", "5", "plus", "23", "equals", "nmsolve", "neuralmath"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage5b-Kernel28] Axion GPU solved 3x+5=23 via parietal bumps> " << nm << "\n";
            if (nm.find("6") != std::string::npos && nm.find(d.w("neuralmath")) != std::string::npos) ++passed;
        } else if (tick == 368) {
            // Tick 368 (Stage 5c): Valley Dynamo Faraday Induction EMF (`50 turns * 12 dflux / 3 s = 200`)!
            say_m(0, {"faraday", "coil1", "nturns", "50", "dflux", "12", "dtime", "3"},
                  {"faraday", "coil1", "50", "nturns", "12", "dflux", "3", "dtime"}, agents[0].markers[LM_SELF], tr);
            std::string em = say_m(0, {"faraday", "emf", "for", "coil1"}, {"coil1", "for", "faraday", "emf"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Faraday] Axion calculated valley dynamo EMF> " << em << "\n";
            if (em.find("200") != std::string::npos) ++passed;
        } else if (tick == 376) {
            // Tick 376 (Stage 5c): Blackwell Kernel 29 Pure-GPU Axonal-Delay Spiking Graph Wavefront (`gn0 -> gn4` detouring blocked `gn2` -> `10 by graphwave`, ZERO host Dijkstra!)
            std::string gw = say_m(0, {"graphwave", "gwdist", "from", "gn0", "to", "gn4", "gwblock", "gn2"},
                                   {"gn0", "from", "gn4", "to", "gn2", "gwblock", "gwdist", "graphwave"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage5c-Kernel29] Axion GPU axonal wavefront detoured gn2> " << gw << "\n";
            if (gw.find("10") != std::string::npos && gw.find(d.w("graphwave")) != std::string::npos) ++passed;
        } else if (tick == 384) {
            // Tick 384 (Stage 5d): Valley Bridge Structural Beam Bending Moment (`40 * 8 / 4 = 80`)!
            say_m(0, {"beam", "bm1", "loadf", "40", "beamlen", "8", "capm", "160"},
                  {"beam", "bm1", "40", "loadf", "8", "beamlen", "160", "capm"}, agents[0].markers[LM_SELF], tr);
            std::string bm = say_m(0, {"beam", "moment", "for", "bm1"}, {"bm1", "for", "beam", "moment"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Beam] Axion checked valley bridge bending moment> " << bm << "\n";
            if (bm.find("80") != std::string::npos) ++passed;
        } else if (tick == 392) {
            // Tick 392 (Stage 5d): Blackwell Kernel 30 Pure-GPU Spiking Boltzmann 3-SAT Forced Backbone Proof (`sv3 -> sattrue by satsolve`, ZERO host SAT!)
            std::string st = say_m(0, {"satsolve", "satval", "sv3"},
                                   {"sv3", "satval", "satsolve"},
                                   agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage5d-Kernel30] Axion GPU spiking Boltzmann SAT proved sv3> " << st << "\n";
            if (st.find(d.w("sattrue")) != std::string::npos && st.find(d.w("satsolve")) != std::string::npos) ++passed;
        } else if (tick == 400) {
            // Tick 400 (Stage 6): Algorithmic Code Lab live execution & invariant check (`(2 + 3) * 4 - 5 = 15 -> codeok`)!
            say_m(0, {"codeprog", "prog1", "codeinit", "2"}, {"codeprog", "prog1", "2", "codeinit"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"codeprog", "prog1", "codeadd", "3"}, {"codeprog", "prog1", "3", "codeadd"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"codeprog", "prog1", "codemul", "4"}, {"codeprog", "prog1", "4", "codemul"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"codeprog", "prog1", "codesub", "5"}, {"codeprog", "prog1", "5", "codesub"}, agents[0].markers[LM_SELF], tr);
            std::string ck = say_m(0, {"codeprog", "coderun", "prog1"}, {"prog1", "coderun", "codeprog"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage6-CodeLab] Axion executed valley control program> " << ck << "\n";
            if (ck.find("15") != std::string::npos) ++passed;
        } else if (tick == 408) {
            // Tick 408 (Stage 6): Blackwell Kernel 31 Pure-GPU 16x16 Spiking V1/V4 Retinal Vision scanning shifted+noisy hollow valley ring beacon!
            std::string vs = say_m(0, {"retina", "visscan", "visring"}, {"visring", "visscan", "retina"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage6-Kernel31] Axion GPU 16x16 spiking retina classified valley beacon> " << vs << "\n";
            if (vs.find(d.w("visring")) != std::string::npos && vs.find(d.w("retina")) != std::string::npos) ++passed;
        } else if (tick == 416) {
            // Tick 416 (Stage 6b): Autonomous counterfactual bug localization on a buggy valley sluice controller (`(4+2)*5-2=28` vs expected `16` -> `bugstep 3 codepatch 3`)!
            say_m(0, {"codeprog", "prog3", "codeinit", "4"}, {"codeprog", "prog3", "4", "codeinit"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"codeprog", "prog3", "codeadd", "2"}, {"codeprog", "prog3", "2", "codeadd"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"codeprog", "prog3", "codemul", "5"}, {"codeprog", "prog3", "5", "codemul"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"codeprog", "prog3", "codesub", "2"}, {"codeprog", "prog3", "2", "codesub"}, agents[0].markers[LM_SELF], tr);
            std::string dbg = say_m(0, {"codeprog", "codefindbug", "prog3", "expect", "16"},
                                    {"prog3", "16", "expect", "codefindbug", "codeprog"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage6b-AutoDebug] Axion counterfactually localized valley controller bug> " << dbg << "\n";
            if (dbg.find(d.w("bugstep")) != std::string::npos && dbg.find("3") != std::string::npos) ++passed;
        } else if (tick == 424) {
            // Tick 424 (Stage 6b): Blackwell Kernel 32 Pure-GPU 16x16 Spiking Cochlear Audio Cortex decoding transposed+noisy valley harmonic beacon (`audmajor by cochlea`)!
            std::string au = say_m(0, {"cochlea", "audscan", "audmajor"}, {"audmajor", "audscan", "cochlea"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage6b-Kernel32] Axion GPU 16x16 spiking cochlea decoded valley call> " << au << "\n";
            if (au.find(d.w("audmajor")) != std::string::npos && au.find(d.w("cochlea")) != std::string::npos) ++passed;
        } else if (tick == 432) {
            // Tick 432 (Stage 6c): 3D Compound Contraption simulation lifting heavy valley ore (`gear 3x * lever 3x * pulley 4x = MA 36`)!
            say_m(0, {"contraption", "rigalpha", "gearin", "10", "gearout", "30", "pulleys", "4"},
                  {"contraption", "rigalpha", "10", "gearin", "30", "gearout", "4", "pulleys"}, agents[0].markers[LM_SELF], tr);
            say_m(0, {"contraption", "rigalpha", "levera", "6", "leverb", "2"},
                  {"contraption", "rigalpha", "6", "levera", "2", "leverb"}, agents[0].markers[LM_SELF], tr);
            std::string ct = say_m(0, {"contraption", "mechadv", "for", "rigalpha"},
                                   {"rigalpha", "for", "contraption", "mechadv"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage6c-Contraption] Axion simulated valley hoist mechanical advantage> " << ct << "\n";
            if (ct.find("36") != std::string::npos) ++passed;
        } else if (tick == 440) {
            // Tick 440 (Stage 6c): Blackwell Kernel 33 Pure-GPU Cross-Modal Vision+Audio Fusion (`visring + audmajor -> xbeacon by xmodal`, ZERO host lookup!)
            std::string xm = say_m(0, {"xmodal", "xfuse", "visring", "audmajor"},
                                   {"visring", "audmajor", "xfuse", "xmodal"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage6c-Kernel33] Axion GPU cross-modal cortex fused valley beacon> " << xm << "\n";
            if (xm.find(d.w("xbeacon")) != std::string::npos && xm.find(d.w("xmodal")) != std::string::npos) ++passed;
        } else if (tick == 448) {
            // Tick 448 (Stage 7a): Valley epidemiology herd-immunity triage (`R_0=4, vax=80% -> herdthr 75%`)!
            say_m(0, {"immunol", "pathalpha", "rzero", "4", "vaxpct", "80"},
                  {"immunol", "pathalpha", "4", "rzero", "80", "vaxpct"}, agents[0].markers[LM_SELF], tr);
            std::string im = say_m(0, {"immunol", "herdthr", "for", "pathalpha"},
                                   {"pathalpha", "for", "immunol", "herdthr"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage7a-Immunol] Axion computed valley herd-immunity threshold> " << im << "\n";
            if (im.find("75") != std::string::npos) ++passed;
        } else if (tick == 456) {
            // Tick 456 (Stage 7a): Blackwell Kernel 34 Pure-GPU Spiking Dialogue Coherence Cortex (`cuecomp + cuewhy -> dlgcompare by dialogue`, ZERO host rules!)
            std::string dg = say_m(0, {"dialogue", "dlgmode", "cuecomp", "cuewhy"},
                                   {"cuecomp", "cuewhy", "dlgmode", "dialogue"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage7a-Kernel34] Axion GPU dialogue cortex selected valley pragmatic mode> " << dg << "\n";
            if (dg.find(d.w("dlgcompare")) != std::string::npos && dg.find(d.w("dialogue")) != std::string::npos) ++passed;
        } else if (tick == 464) {
            // Tick 464 (Stage 7b): Autonomous Valley Scientific Research Loop (`sciloop labalpha`: falsifies `hypslow`, confirms `hypfast`!)
            say_m(0, {"sciloop", "labalpha", "hypfast", "80", "hypslow", "40", "scitest", "82"},
                  {"sciloop", "labalpha", "80", "hypfast", "40", "hypslow", "82", "scitest"}, agents[0].markers[LM_SELF], tr);
            std::string sc = say_m(0, {"sciloop", "hconfirm", "for", "labalpha"},
                                   {"labalpha", "for", "sciloop", "hconfirm"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage7b-SciLoop] Axion confirmed winning valley hypothesis> " << sc << "\n";
            if (sc.find(d.w("hypfast")) != std::string::npos) ++passed;
        } else if (tick == 472) {
            // Tick 472 (Stage 7b): Blackwell Kernel 35 Pure-GPU Spiking Conceptual Metaphor Blend (`mflow + mtime -> metriver by metaphor`, ZERO host rules!)
            std::string mt = say_m(0, {"metaphor", "metblend", "mflow", "mtime"},
                                   {"mflow", "mtime", "metblend", "metaphor"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage7b-Kernel35] Axion GPU metaphor cortex blended valley concept> " << mt << "\n";
            if (mt.find(d.w("metriver")) != std::string::npos && mt.find(d.w("metaphor")) != std::string::npos) ++passed;
        } else if (tick == 480) {
            // Tick 480 (Stage 7c — 400 Domains!): Autonomous Valley Scientific Peer-Review Replication Audit (`peerrev pubalpha -> replicated`)!
            say_m(0, {"peerrev", "pubalpha", "origv", "50", "replv", "48", "nsamp", "100"},
                  {"peerrev", "pubalpha", "50", "origv", "48", "replv", "100", "nsamp"}, agents[0].markers[LM_SELF], tr);
            std::string pr = say_m(0, {"peerrev", "repcheck", "for", "pubalpha"},
                                   {"pubalpha", "for", "peerrev", "repcheck"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage7c-PeerRev] Axion audited valley scientific replication> " << pr << "\n";
            if (pr.find(d.w("replicated")) != std::string::npos) ++passed;
        } else if (tick == 488) {
            // Tick 488 (Stage 7c — 400 Domains!): Blackwell Kernel 36 Pure-GPU Belief-Graph & Scientific Debate (`evlens + evprism -> theorywave belconfirm by belgraph`)!
            std::string bg = say_m(0, {"belgraph", "belcheck", "evlens", "evprism"},
                                   {"evlens", "evprism", "belcheck", "belgraph"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage7c-Kernel36] Axion GPU belief-graph cortex evaluated valley theory> " << bg << "\n";
            if (bg.find(d.w("theorywave")) != std::string::npos && bg.find(d.w("belconfirm")) != std::string::npos && bg.find(d.w("belgraph")) != std::string::npos) ++passed;
        } else if (tick == 496) {
            // Tick 496 (STAGE 8 CROSSING!): Autonomous Valley Spec-to-Code Program Synthesis & Verification (`speccode fnalpha -> specpass`)!
            say_m(0, {"speccode", "fnalpha", "inx", "4", "targety", "14", "slopek", "3", "biasb", "2"},
                  {"speccode", "fnalpha", "4", "inx", "14", "targety", "3", "slopek", "2", "biasb"}, agents[0].markers[LM_SELF], tr);
            std::string sp = say_m(0, {"speccode", "specverdict", "for", "fnalpha"},
                                   {"fnalpha", "for", "speccode", "specverdict"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8-SpecCode] Axion synthesized & verified valley program spec> " << sp << "\n";
            if (sp.find(d.w("specpass")) != std::string::npos) ++passed;
        } else if (tick == 504) {
            // Tick 504 (STAGE 8 CROSSING!): Blackwell Kernel 37 Pure-GPU Collective Scientific Discovery & Consensus (`specphys + specmath -> lawthermo collaccept by colldisc`)!
            std::string cd = say_m(0, {"colldisc", "collpool", "specphys", "specmath"},
                                   {"specphys", "specmath", "collpool", "colldisc"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8-Kernel37] Axion collective discovery cortex reached multi-mind consensus> " << cd << "\n";
            if (cd.find(d.w("lawthermo")) != std::string::npos && cd.find(d.w("collaccept")) != std::string::npos && cd.find(d.w("colldisc")) != std::string::npos) ++passed;
        } else if (tick == 512) {
            // Tick 512 (Stage 8b!): Optimal Kalman Filter Valley Beacon Sensor Fusion (`priorx 40, measz 60, prip 30, measr 20 -> postx 52`)!
            say_m(0, {"kalman", "filtalpha", "priorx", "40", "measz", "60", "prip", "30", "measr", "20"},
                  {"kalman", "filtalpha", "40", "priorx", "60", "measz", "30", "prip", "20", "measr"}, agents[0].markers[LM_SELF], tr);
            std::string kf = say_m(0, {"kalman", "postx", "for", "filtalpha"},
                                   {"filtalpha", "for", "kalman", "postx"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8b-Kalman] Axion fused noisy valley sensor posterior> " << kf << "\n";
            if (kf.find("52") != std::string::npos) ++passed;
        } else if (tick == 520) {
            // Tick 520 (Stage 8b!): Blackwell Kernel 38 Pure-GPU Optimal Discriminative Experiment Design (`hypgrav vs hypdrag -> expvacuum by selfres`)!
            std::string sr = say_m(0, {"selfres", "resdesign", "hypgrav", "hypdrag"},
                                   {"hypgrav", "hypdrag", "resdesign", "selfres"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8b-Kernel38] Axion self-research cortex designed optimal experiment> " << sr << "\n";
            if (sr.find(d.w("expvacuum")) != std::string::npos && sr.find(d.w("selfres")) != std::string::npos) ++passed;
        } else if (tick == 528) {
            // Tick 528 (Stage 8c!): Control-Theory Bode Phase Margin Verification for Valley Sluice Loop (`bodemarg loopalpha 120 8 50 -> pmdeg 60`)!
            say_m(0, {"bodemarg", "loopalpha", "120", "8", "50"},
                  {"loopalpha", "120", "8", "50", "bodemarg"}, agents[0].markers[LM_SELF], tr);
            std::string bm = say_m(0, {"bodemarg", "pmdeg", "for", "loopalpha"},
                                   {"loopalpha", "for", "bodemarg", "pmdeg"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8c-Bode] Axion checked valley feedback phase margin> " << bm << "\n";
            if (bm.find("60") != std::string::npos) ++passed;
        } else if (tick == 536) {
            // Tick 536 (Stage 8c!): Blackwell Kernel 39 Pure-GPU Spiking Symbolic Differentiation (`symcalc symdiff polyalpha 2 -> 18 by symcalc`)!
            std::string sc = say_m(0, {"symcalc", "symdiff", "polyalpha", "2"},
                                   {"polyalpha", "2", "symdiff", "symcalc"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8c-Kernel39] Axion symbolic calculus cortex differentiated on GPU> " << sc << "\n";
            if (sc.find("18") != std::string::npos && sc.find(d.w("symcalc")) != std::string::npos) ++passed;
        } else if (tick == 544) {
            // Tick 544 (Stage 8d!): Aerospace Tsiolkovsky Rocket Delta-V Verification (`rocketisp craftalpha 300 20 5000 -> velgain 6000`)!
            say_m(0, {"rocketisp", "craftalpha", "300", "20", "5000"},
                  {"craftalpha", "300", "20", "5000", "rocketisp"}, agents[0].markers[LM_SELF], tr);
            std::string rk = say_m(0, {"rocketisp", "velgain", "for", "craftalpha"},
                                   {"craftalpha", "for", "rocketisp", "velgain"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8d-Rocket] Axion computed valley launch delta-v> " << rk << "\n";
            if (rk.find("6000") != std::string::npos) ++passed;
        } else if (tick == 552) {
            // Tick 552 (Stage 8d!): Blackwell Kernel 40 Pure-GPU 256-Thread Parallel AST Program Synthesis (`progast astsynth progomega 2 7 4 13 6 -> 19 by progast`)!
            std::string pa = say_m(0, {"progast", "astsynth", "progomega", "2", "7", "4", "13", "6"},
                                   {"progomega", "2", "7", "4", "13", "6", "astsynth", "progast"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8d-Kernel40] Axion AST synthesis cortex discovered & ran program on GPU> " << pa << "\n";
            if (pa.find("19") != std::string::npos && pa.find(d.w("progast")) != std::string::npos) ++passed;
        } else if (tick == 560) {
            // Tick 560 (Stage 8e!): Fluid Mechanics Glider Wing Lift Verification (`aerolift wingalpha 2 10 4 -> liftfrc 400`)!
            say_m(0, {"aerolift", "wingalpha", "2", "10", "4"},
                  {"wingalpha", "2", "10", "4", "aerolift"}, agents[0].markers[LM_SELF], tr);
            std::string al = say_m(0, {"aerolift", "liftfrc", "for", "wingalpha"},
                                   {"wingalpha", "for", "aerolift", "liftfrc"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8e-AeroLift] Axion verified valley glider wing lift> " << al << "\n";
            if (al.find("400") != std::string::npos) ++passed;
        } else if (tick == 568) {
            // Tick 568 (Stage 8e!): Blackwell Kernel 41 Pure-GPU STDP Matrix Imprint & Cramer Linear System Solve (`linalg linsolve linomega 2 1 1 3 13 14 -> 5 by linalg`)!
            std::string la = say_m(0, {"linalg", "linsolve", "linomega", "2", "1", "1", "3", "13", "14"},
                                   {"linomega", "2", "1", "1", "3", "13", "14", "linsolve", "linalg"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8e-Kernel41] Axion neural linear-algebra cortex solved system on GPU> " << la << "\n";
            if (la.find("5") != std::string::npos && la.find(d.w("linalg")) != std::string::npos) ++passed;
        } else if (tick == 576) {
            // Tick 576 (Stage 8f!): Geometric Thin-Lens Observatory Optics Focal Length (`lensopt optalpha 20 60 -> foclen 15`)!
            say_m(0, {"lensopt", "optalpha", "20", "60"},
                  {"optalpha", "20", "60", "lensopt"}, agents[0].markers[LM_SELF], tr);
            std::string lo = say_m(0, {"lensopt", "foclen", "for", "optalpha"},
                                   {"optalpha", "for", "lensopt", "foclen"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8f-LensOpt] Axion calibrated valley observatory thin lens> " << lo << "\n";
            if (lo.find("15") != std::string::npos) ++passed;
        } else if (tick == 584) {
            // Tick 584 (Stage 8f!): Blackwell Kernel 42 Pure-GPU Spiking Bayes-Net Live STDP Imprint & Decision (`bnet bnetlive bnomega 50 80 20 20 80 -> 80 bnetact by bnet`)!
            std::string bn = say_m(0, {"bnet", "bnetlive", "bnomega", "50", "80", "20", "20", "80"},
                                   {"bnomega", "50", "80", "20", "20", "80", "bnetlive", "bnet"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8f-Kernel42] Axion spiking Bayes-net cortex inferred posterior & action on GPU> " << bn << "\n";
            if (bn.find("80") != std::string::npos && bn.find(d.w("bnetact")) != std::string::npos && bn.find(d.w("bnet")) != std::string::npos) ++passed;
        } else if (tick == 592) {
            // Tick 592 (Stage 8g!): Power Electronics Valley Buck DC-DC Regulator Voltage (`buckconv regalpha 20 50 80 -> bkvout 10`)!
            say_m(0, {"buckconv", "regalpha", "20", "50", "80"},
                  {"regalpha", "20", "50", "80", "buckconv"}, agents[0].markers[LM_SELF], tr);
            std::string bk = say_m(0, {"buckconv", "bkvout", "for", "regalpha"},
                                   {"regalpha", "for", "buckconv", "bkvout"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8g-BuckConv] Axion verified valley power regulator output> " << bk << "\n";
            if (bk.find("10") != std::string::npos) ++passed;
        } else if (tick == 600) {
            // Tick 600 (Stage 8g!): Blackwell Kernel 43 Pure-GPU Spiking MDP Live STDP Imprint & 4-Step Bellman Policy (`mdpplan mdplive mdpomega 5 10 100 40 90 50 -> 47 mdpleap by mdpplan`)!
            std::string mp = say_m(0, {"mdpplan", "mdplive", "mdpomega", "5", "10", "100", "40", "90", "50"},
                                   {"mdpomega", "5", "10", "100", "40", "90", "50", "mdplive", "mdpplan"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8g-Kernel43] Axion spiking MDP planner ran 4-step Bellman value iteration on GPU> " << mp << "\n";
            if (mp.find("47") != std::string::npos && mp.find(d.w("mdpleap")) != std::string::npos && mp.find(d.w("mdpplan")) != std::string::npos) ++passed;
        } else if (tick == 608) {
            // Tick 608 (Stage 8h!): CPU Architecture Valley Beacon Processor Branch Predictor CPI (`brpred cpupralph 10 20 15 10 -> bpcpi 13`)!
            say_m(0, {"brpred", "cpupralph", "10", "20", "15", "10"},
                  {"cpupralph", "10", "20", "15", "10", "brpred"}, agents[0].markers[LM_SELF], tr);
            std::string bp = say_m(0, {"brpred", "bpcpi", "for", "cpupralph"},
                                   {"cpupralph", "for", "brpred", "bpcpi"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8h-BrPred] Axion audited valley beacon CPU branch predictor CPI> " << bp << "\n";
            if (bp.find("13") != std::string::npos) ++passed;
        } else if (tick == 616) {
            // Tick 616 (Stage 8h!): Blackwell Kernel 44 Pure-GPU Spiking Kalman Filter Live STDP Imprint & Sensor Fusion (`kfilt kflive kfomega 40 30 10 10 90 0 -> 80 kftrust by kfilt`)!
            std::string kf = say_m(0, {"kfilt", "kflive", "kfomega", "40", "30", "10", "10", "90", "0"},
                                   {"kfomega", "40", "30", "10", "10", "90", "0", "kflive", "kfilt"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8h-Kernel44] Axion spiking Kalman filter fused sensor state on GPU> " << kf << "\n";
            if (kf.find("80") != std::string::npos && kf.find(d.w("kftrust")) != std::string::npos && kf.find(d.w("kfilt")) != std::string::npos) ++passed;
        } else if (tick == 624) {
            // Tick 624 (Stage 8i!): Structural Engineering Valley Bridge Column Euler Critical Load (`buckling colx 10 80 4 -> bupcr 50`)!
            say_m(0, {"buckling", "colx", "10", "80", "4"},
                  {"colx", "10", "80", "4", "buckling"}, agents[0].markers[LM_SELF], tr);
            std::string bu = say_m(0, {"buckling", "bupcr", "for", "colx"},
                                   {"colx", "for", "buckling", "bupcr"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8i-Buckling] Axion audited valley bridge column Euler buckling load> " << bu << "\n";
            if (bu.find("50") != std::string::npos) ++passed;
        } else if (tick == 632) {
            // Tick 632 (Stage 8i!): Blackwell Kernel 45 Pure-GPU Spiking LQR / Riccati Feedback Control Live STDP Imprint (`lqrctrl lqlive lqomega 80 100 30 10 60 0 -> 37 lqstab by lqrctrl`)!
            std::string lq = say_m(0, {"lqrctrl", "lqlive", "lqomega", "80", "100", "30", "10", "60", "0"},
                                   {"lqomega", "80", "100", "30", "10", "60", "0", "lqlive", "lqrctrl"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8i-Kernel45] Axion spiking LQR controller solved Riccati gain on GPU> " << lq << "\n";
            if (lq.find("37") != std::string::npos && lq.find(d.w("lqstab")) != std::string::npos && lq.find(d.w("lqrctrl")) != std::string::npos) ++passed;
        } else if (tick == 640) {
            // Tick 640 (Stage 8j — 500-DOMAIN MILESTONE!): Valley River Froude Gravity Wave Number Audit (`froude flumx 30 20 -> frnum 150`)!
            say_m(0, {"froude", "flumx", "30", "20"},
                  {"flumx", "30", "20", "froude"}, agents[0].markers[LM_SELF], tr);
            std::string fr = say_m(0, {"froude", "frnum", "for", "flumx"},
                                   {"flumx", "for", "froude", "frnum"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8j-Froude] Axion audited valley spillway Froude wave number> " << fr << "\n";
            if (fr.find("150") != std::string::npos) ++passed;
        } else if (tick == 648) {
            // Tick 648 (Stage 8j — 500-DOMAIN MILESTONE!): Blackwell Kernel 46 Pure-GPU Spiking 8-Point FFT Spectrum Live STDP Imprint (`fftspec fflive ffomega 90 10 90 10 90 10 90 10 -> 40 ffhigh by fftspec`)!
            std::string ff = say_m(0, {"fftspec", "fflive", "ffomega", "90", "10", "90", "10", "90", "10", "90", "10"},
                                   {"ffomega", "90", "10", "90", "10", "90", "10", "90", "10", "fflive", "fftspec"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8j-Kernel46] Axion spiking FFT cortex decomposed acoustic signal on GPU> " << ff << "\n";
            if (ff.find("40") != std::string::npos && ff.find(d.w("ffhigh")) != std::string::npos && ff.find(d.w("fftspec")) != std::string::npos) ++passed;
        } else if (tick == 656) {
            // Tick 656 (Stage 8k — 510 DOMAINS!): Valley Observatory Photovoltaic Solar Array Peak Power Audit (`pvcell panlx 40 10 80 1 -> pvpow 320`)!
            say_m(0, {"pvcell", "panlx", "40", "10", "80", "1"},
                  {"panlx", "40", "10", "80", "1", "pvcell"}, agents[0].markers[LM_SELF], tr);
            std::string pv = say_m(0, {"pvcell", "pvpow", "for", "panlx"},
                                   {"panlx", "for", "pvcell", "pvpow"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8k-PVCell] Axion audited valley solar array peak power> " << pv << "\n";
            if (pv.find("320") != std::string::npos) ++passed;
        } else if (tick == 664) {
            // Tick 664 (Stage 8k — 510 DOMAINS!): Blackwell Kernel 47 Pure-GPU Spiking 4th-Order RK4 ODE Integrator Live STDP Imprint (`oderk4 odlive odomega 1 50 40 20 50 4 -> 58 odestab by oderk4`)!
            std::string od = say_m(0, {"oderk4", "odlive", "odomega", "1", "50", "40", "20", "50", "4"},
                                   {"odomega", "1", "50", "40", "20", "50", "4", "odlive", "oderk4"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8k-Kernel47] Axion spiking RK4 ODE cortex integrated dynamics on GPU> " << od << "\n";
            if (od.find("58") != std::string::npos && od.find(d.w("odestab")) != std::string::npos && od.find(d.w("oderk4")) != std::string::npos) ++passed;
        } else if (tick == 672) {
            // Tick 672 (Stage 8l — 520 DOMAINS!): Valley Observatory Capacitor Bank Capacitance Audit (`capbank cbnkx 4 50 2 10 -> cbcap 100`)!
            say_m(0, {"capbank", "cbnkx", "4", "50", "2", "10"},
                  {"cbnkx", "4", "50", "2", "10", "capbank"}, agents[0].markers[LM_SELF], tr);
            std::string cb = say_m(0, {"capbank", "cbcap", "for", "cbnkx"},
                                   {"cbnkx", "for", "capbank", "cbcap"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8l-CapBank] Axion audited valley capacitor bank storage> " << cb << "\n";
            if (cb.find("100") != std::string::npos) ++passed;
        } else if (tick == 680) {
            // Tick 680 (Stage 8l — 520 DOMAINS!): Blackwell Kernel 48 Pure-GPU Spiking Simplex LP Optimizer Live STDP Imprint (`simlp sxlive sxomega 5 4 2 1 20 1 2 16 -> 56 sxdual by simlp`)!
            std::string sx = say_m(0, {"simlp", "sxlive", "sxomega", "5", "4", "2", "1", "20", "1", "2", "16"},
                                   {"sxomega", "5", "4", "2", "1", "20", "1", "2", "16", "sxlive", "simlp"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8l-Kernel48] Axion spiking Simplex LP cortex optimized valley allocation on GPU> " << sx << "\n";
            if (sx.find("56") != std::string::npos && sx.find(d.w("sxdual")) != std::string::npos && sx.find(d.w("simlp")) != std::string::npos) ++passed;
        } else if (tick == 688) {
            // Tick 688 (Stage 8m — 530 DOMAINS!): Valley Observatory Relativistic Black-Hole Horizon Audit (`blkhole bhobjx 100 50 -> bhrad 300`)!
            say_m(0, {"blkhole", "bhobjx", "100", "50"},
                  {"bhobjx", "100", "50", "blkhole"}, agents[0].markers[LM_SELF], tr);
            std::string bh = say_m(0, {"blkhole", "bhrad", "for", "bhobjx"},
                                   {"bhobjx", "for", "blkhole", "bhrad"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8m-BlkHole] Axion audited observatory black-hole horizon radius> " << bh << "\n";
            if (bh.find("300") != std::string::npos) ++passed;
        } else if (tick == 696) {
            // Tick 696 (Stage 8m — 530 DOMAINS!): Blackwell Kernel 49 Pure-GPU Spiking PCA Eigendecomposition Live STDP Imprint (`pcaeig pclive pcomega 28 6 12 -> 30 pcaxis by pcaeig`)!
            std::string pc = say_m(0, {"pcaeig", "pclive", "pcomega", "28", "6", "12"},
                                   {"pcomega", "28", "6", "12", "pclive", "pcaeig"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8m-Kernel49] Axion spiking PCA eigen cortex extracted principal axis on GPU> " << pc << "\n";
            if (pc.find("30") != std::string::npos && pc.find(d.w("pcaxis")) != std::string::npos && pc.find(d.w("pcaeig")) != std::string::npos) ++passed;
        } else if (tick == 704) {
            // Tick 704 (Stage 8n — 540 DOMAINS!): Valley Observatory Quantum de Broglie Matter-Wave Wavelength Audit (`debrog dbwavx 60 5 10 -> dblam 120`)!
            say_m(0, {"debrog", "dbwavx", "60", "5", "10"},
                  {"dbwavx", "60", "5", "10", "debrog"}, agents[0].markers[LM_SELF], tr);
            std::string db = say_m(0, {"debrog", "dblam", "for", "dbwavx"},
                                   {"dbwavx", "for", "debrog", "dblam"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8n-DeBrog] Axion audited observatory matter-wave de Broglie wavelength> " << db << "\n";
            if (db.find("120") != std::string::npos) ++passed;
        } else if (tick == 712) {
            // Tick 712 (Stage 8n — 50-KERNEL MILESTONE!): Blackwell Kernel 50 Pure-GPU Spiking 1D Heat/Diffusion PDE Solver Live STDP Imprint (`pdeslv pdlive pdomega 0 100 100 100 0 20 2 -> 92 pdestab by pdeslv`)!
            std::string pd = say_m(0, {"pdeslv", "pdlive", "pdomega", "0", "100", "100", "100", "0", "20", "2"},
                                   {"pdomega", "0", "100", "100", "100", "0", "20", "2", "pdlive", "pdeslv"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8n-Kernel50] Axion spiking 1D PDE cortex evolved valley thermal field on GPU> " << pd << "\n";
            if (pd.find("92") != std::string::npos && pd.find(d.w("pdestab")) != std::string::npos && pd.find(d.w("pdeslv")) != std::string::npos) ++passed;
        } else if (tick == 720) {
            // Tick 720 (Stage 8o — 550 DOMAINS!): Valley Semiconductor Lab Fermi-Dirac Occupancy Audit (`fermidr fdmatx 40 20 10 -> fdocc 20`)!
            say_m(0, {"fermidr", "fdmatx", "40", "20", "10"},
                  {"fdmatx", "40", "20", "10", "fermidr"}, agents[0].markers[LM_SELF], tr);
            std::string fd = say_m(0, {"fermidr", "fdocc", "for", "fdmatx"},
                                   {"fdmatx", "for", "fermidr", "fdocc"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8o-Fermi] Axion audited valley crystal Fermi-Dirac occupancy> " << fd << "\n";
            if (fd.find("20") != std::string::npos) ++passed;
        } else if (tick == 728) {
            // Tick 728 (Stage 8o — 51 KERNELS!): Blackwell Kernel 51 Pure-GPU 256-Chain Spiking MCMC Sampler Live STDP Imprint (`mcmcsmp mclive mcomega 80 10 5 5 32 53 -> 80 mcpeak by mcmcsmp`)!
            std::string mc = say_m(0, {"mcmcsmp", "mclive", "mcomega", "80", "10", "5", "5", "32", "53"},
                                   {"mcomega", "80", "10", "5", "5", "32", "53", "mclive", "mcmcsmp"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8o-Kernel51] Axion 256-chain spiking MCMC cortex sampled posterior on GPU> " << mc << "\n";
            if (mc.find("80") != std::string::npos && mc.find(d.w("mcpeak")) != std::string::npos && mc.find(d.w("mcmcsmp")) != std::string::npos) ++passed;
        } else if (tick == 736) {
            // Tick 736 (Stage 8p — 560 DOMAINS!): Valley Quantum Lab Grover Search Gate Cost Audit (`groverq grqregx 100 1 4 -> grcost 312`)!
            say_m(0, {"groverq", "grqregx", "100", "1", "4"},
                  {"grqregx", "100", "1", "4", "groverq"}, agents[0].markers[LM_SELF], tr);
            std::string gq = say_m(0, {"groverq", "grcost", "for", "grqregx"},
                                   {"grqregx", "for", "groverq", "grcost"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8p-Grover] Axion audited valley quantum Grover gate cost> " << gq << "\n";
            if (gq.find("312") != std::string::npos) ++passed;
        } else if (tick == 744) {
            // Tick 744 (Stage 8p — 52 KERNELS & 100/100 CENTURY VALLEY!): Blackwell Kernel 52 Pure-GPU 4,096-Rollout UCB1 MCTS Planner Live STDP Imprint (`mctspln mtlive mtomega 110 50 45 40 90 50 20 53 -> 108 mctbold by mctspln`)!
            std::string mt = say_m(0, {"mctspln", "mtlive", "mtomega", "110", "50", "45", "40", "90", "50", "20", "53"},
                                   {"mtomega", "110", "50", "45", "40", "90", "50", "20", "53", "mtlive", "mctspln"}, agents[0].markers[LM_USER], tr);
            std::cout << "  [tick " << tick << " · Stage8p-Kernel52] Axion 4,096-rollout UCB1 MCTS cortex planned valley strategy on GPU> " << mt << "\n";
            if (mt.find("108") != std::string::npos && mt.find(d.w("mctbold")) != std::string::npos && mt.find(d.w("mctspln")) != std::string::npos) ++passed;
        }
    }
    (void)bridge_broken;
    (void)ax_reroute_steps;
    (void)auto_decisions;
    (void)names;

    // Grounded Post-Ecosystem Conversational Interview!
    std::string q_why = say_m(0, {"why", "did", "you", "change", "mind", "about", "crystal"},
                              {"crystal", "about", "mind", "change", "you", "did", "why"}, agents[0].markers[LM_USER], tr);
    std::cout << "  [Interview · Transparency] Axion> " << q_why << "\n";
    if (q_why.find(d.w("litmus")) != std::string::npos && q_why.find(d.w("salty")) != std::string::npos) ++passed;

    std::string q_ber = say_m(0, {"how", "many", "berry", "do", "you", "have", "now"},
                              {"you", "berry", "now", "how", "many", "have"}, agents[0].markers[LM_USER], tr);
    std::cout << "  [Interview · Conservation] Axion> " << q_ber << "\n";
    if (q_ber.find("3") != std::string::npos) ++passed;

    std::string q_ore = say_m(1, {"how", "many", "ore", "does", "mira", "have", "now"},
                              {"mira", "ore", "now", "how", "many", "have"}, agents[1].markers[LM_USER], tr);
    std::cout << "  [Interview · Conservation] Mira> " << q_ore << "\n";
    if (q_ore.find("2") != std::string::npos) ++passed;

    std::string q_tr = say_m(0, {"who", "do", "you", "trust", "more", "mira", "or", "fox"},
                             {"mira", "fox", "or", "trust", "more", "who"}, agents[0].markers[LM_USER], tr);
    std::cout << "  [Interview · Social Trust] Axion> " << q_tr << "\n";
    if (q_tr.find(d.w("mira")) != std::string::npos) ++passed;

    std::string q_wr = say_m(2, {"is", "crystal", "salty"}, {"crystal", "salty", "is", "what"}, agents[2].markers[LM_USER], tr);
    std::cout << "  [Interview · Cultural Relay] Wren> " << q_wr << "\n";
    if (q_wr.find(d.w("yes")) != std::string::npos) ++passed;

    Count tot_spikes = 0;
    for (int i = 0; i < 4; ++i) tot_spikes += agents[i].lifetime_spikes;
    std::cout << "mind_ecosystem=complete  dialect=" << (alien ? "alien_sov" : "english_svo")
              << "  ticks=" << n_ticks << "  minds=4  passed=" << passed << "  total=" << total
              << "  trades=" << trades << "  discoveries=" << discoveries
              << "  lies_caught=" << lies_caught << "  decisions=" << auto_decisions
              << "  total_spikes=" << tot_spikes << '\n';
    if (auto *j = journal::current())
        j->scores.push_back({"Open Valley spiking ecosystem", passed, total});
}

// ---------------------------------------------------------------------------
// 4. Interactive Multi-Turn Conversation Loop + Live Room-Aware Homestead + Peer!
// ---------------------------------------------------------------------------
void run_interactive(Session &s, SpikingMind &m, bool alien, const std::string &save_path,
                     const std::string &ckpt_path = "") {
    Dialect d = Dialect::make(alien);
    HomesteadWorld world;
    world.reset();

    // Live autonomous GPU-attached spiking peer (`Mira`) with private knowledge!
    SpikingMind peer_mira;
    bool has_peer = false;
    if (!ckpt_path.empty()) {
        peer_mira.attach_gpu(s);
        peer_mira.load(ckpt_path);
        CognitiveTrace ptr{};
        converse_turn(peer_mira,
                      d.sent(alien ? std::vector<std::string>{"mira", "sapphire", "pouch", "inside", "put"}
                                   : std::vector<std::string>{"mira", "put", "sapphire", "inside", "pouch"}),
                      peer_mira.markers[LM_USER], ptr);
        converse_turn(peer_mira,
                      d.sent(alien ? std::vector<std::string>{"mira", "pouch", "bridge", "inside", "put"}
                                   : std::vector<std::string>{"mira", "put", "pouch", "inside", "bridge"}),
                      peer_mira.markers[LM_USER], ptr);
        has_peer = true;
    }

    std::cout << "Axion Cortical Spiking Mind  [" << s.name << "]\n"
              << "  gen=" << m.generation << "  stage=" << m.stage << "  concepts=" << (m.concepts.size() - 1)
              << "  engrams=" << m.engrams.size() << "  causal_schemas=" << m.schemas.size()
              << "  constructions=" << m.constructions.size() << "  dialect=" << (alien ? "alien_sov" : "english_svo")
              << (has_peer ? "  live_peer=mira" : "")
              << "\nType natural sentences or questions.\n"
              << "Commands: /world /go ROOM /nav FROM TO /eco /take ITEM /drop ITEM /do TARGET GOAL /tick N /consult /ask-peer Q /sleep /inspect /ablate MODE /eval /save FILE /quit\n"
              << std::flush;

    Word turns = 0;
    Count session_spikes = 0, session_edits = 0;
    for (std::string line; std::getline(std::cin, line);) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
            line.pop_back();
        if (line.empty()) continue;
        if (line == "/quit" || line == "/exit") break;
        if (line == "/world") {
            std::cout << "[homestead] room=" << world.child_room << "  inventory={";
            bool first = true;
            for (const auto &it : world.child_inventory) { std::cout << (first ? "" : ", ") << it; first = false; }
            std::cout << "}\n";
            for (const auto &[nm, obj] : world.objects) {
                std::cout << "  " << nm << " (" << obj.category << ") @" << obj.location << " [";
                bool fp = true;
                for (const auto &pr : obj.props) { std::cout << (fp ? "" : " ") << pr; fp = false; }
                std::cout << "]\n";
            }
            std::cout << std::flush;
            continue;
        }
        if (line.starts_with("/tick")) {
            Word n_ticks = 1;
            if (line.size() > 6) {
                try { n_ticks = Word(std::max(1, std::min(50, std::stoi(line.substr(6))))); } catch (...) {}
            }
            static const std::vector<std::string> rooms = {"meadow", "garden", "workshop", "kitchen", "pantry", "attic", "cellar", "bridge"};
            for (Word t = 0; t < n_ticks; ++t) {
                ++world.physical_actions;
                for (auto &[pname, proom] : world.peer_room) {
                    Word h = mix(world.physical_actions * 2654435761u ^ Word(pname[0]) * 131u);
                    proom = rooms[h % rooms.size()];
                }
                // Autonomous social ecology: if Fox visits garden, Fox moves acorn to pantry out of sight unless Axion is in garden!
                if (world.peer_room["fox"] == world.objects["acorn"].location) {
                    std::string dest = (world.objects["acorn"].location == "meadow") ? "pantry" : "meadow";
                    std::string old_loc = world.objects["acorn"].location;
                    world.objects["acorn"].location = dest;
                    bool axion_saw = (world.child_room == old_loc || world.child_room == dest);
                    std::cout << "[ecology tick " << world.physical_actions << "] fox moved acorn " << old_loc
                              << " -> " << dest << (axion_saw ? " (axion observed!)" : " (hidden from axion!)") << "\n";
                    if (axion_saw) {
                        CognitiveTrace tr{};
                        converse_turn(m, d.sent({"fox", "moved", "acorn", "to", dest}), m.markers[LM_SELF], tr);
                    }
                    if (has_peer && (world.peer_room["mira"] == old_loc || world.peer_room["mira"] == dest)) {
                        CognitiveTrace ptr{};
                        converse_turn(peer_mira, d.sent({"fox", "moved", "acorn", "to", dest}), peer_mira.markers[LM_SELF], ptr);
                    }
                }
            }
            std::cout << "[ecology] peers:";
            for (const auto &[pname, proom] : world.peer_room)
                std::cout << " " << pname << "@" << proom;
            std::cout << "\n" << std::flush;
            continue;
        }
        if (line == "/consult" && has_peer) {
            Word gap = !m.wm.curiosity_concepts.empty() ? m.wm.curiosity_concepts.back() : m.wm.last_entity;
            std::string gap_w = m.word_of(gap);
            std::string q = d.sent(alien ? std::vector<std::string>{gap_w, "where", "is"}
                                         : std::vector<std::string>{"where", "is", gap_w});
            CognitiveTrace pt{}, mt{};
            std::string ans = converse_turn(peer_mira, q, m.markers[LM_SELF], pt);
            std::cout << "[peer] Axion -> Mira: " << q << "\n[peer] Mira -> Axion: " << ans
                      << "  [mira_spikes=" << pt.gpu_spikes << "]\n";
            // Absorb Mira's reply into Axion's own GPU Hippocampus in 1 shot!
            auto ch_peer = peer_mira.query_transitive_chain(peer_mira.find_word_exact(gap_w),
                                                            peer_mira.markers[LM_IN], peer_mira.markers[LM_SELF],
                                                            false, &pt);
            if (ch_peer.found) {
                std::string final_w = peer_mira.word_of(ch_peer.nodes.back());
                std::string fact = d.sent(alien ? std::vector<std::string>{gap_w, final_w, "inside", "is"}
                                                : std::vector<std::string>{gap_w, "is", "inside", final_w});
                converse_turn(m, fact, m.find_word_exact(d.w("mira")), mt);
                std::cout << "[peer] Axion absorbed fact in 1 shot [axion_edits=" << mt.synaptic_edits << "]\n";
            }
            std::cout << std::flush;
            continue;
        }
        if (line.starts_with("/ask-peer ") && has_peer) {
            std::string q = line.substr(10);
            CognitiveTrace pt{};
            std::string ans = converse_turn(peer_mira, q, m.markers[LM_SELF], pt);
            std::cout << "[peer Mira] " << ans << "  [mira_spikes=" << pt.gpu_spikes << "]\n" << std::flush;
            continue;
        }
        if (line == "/eco") {
            std::cout << "[open valley 16x16] bridge=" << (m.map_bridge_broken ? "BROKEN" : "open")
                      << "  landmarks=" << m.map_landmarks.size()
                      << "  neocortex=" << (NEO_PATHWAYS * NEO_SYNAPSES) << " synapses (" << NEO_PATHWAYS << " pathways)\n"
                      << std::flush;
            continue;
        }
        if (line.starts_with("/nav ")) {
            std::istringstream iss(line.substr(5));
            std::string from_s, to_s;
            iss >> from_s >> to_s;
            Word fc = m.find_word_exact(d.w(from_s));
            Word tc = m.find_word_exact(d.w(to_s));
            CognitiveTrace tr{};
            auto nav = m.gpu_navigate_2d(fc, tc, &tr);
            if (!nav.found) {
                std::cout << "[2d place-cell map] no path found from " << from_s << " to " << to_s << "\n" << std::flush;
                continue;
            }
            char grid[MAP_H][MAP_W + 1];
            for (Word y = 0; y < MAP_H; ++y) {
                for (Word x = 0; x < MAP_W; ++x) {
                    if (x == 7 && y != 1 && (y != 7 || m.map_bridge_broken))
                        grid[y][x] = (y == 7) ? 'X' : '~';
                    else if (x == 7 && y == 7)
                        grid[y][x] = '=';
                    else
                        grid[y][x] = '.';
                }
                grid[y][MAP_W] = '\0';
            }
            for (auto [px, py] : nav.path_xy)
                if (px < MAP_W && py < MAP_H)
                    grid[py][px] = '*';
            for (const auto &[cid, xy] : m.map_landmarks) {
                std::string wn = m.word_of(cid);
                if (!wn.empty() && xy.first < MAP_W && xy.second < MAP_H)
                    grid[xy.second][xy.first] = char(std::toupper(static_cast<unsigned char>(wn[0])));
            }
            std::cout << "[2d place-cell GPU wave] " << from_s << " -> " << to_s << "  steps=" << nav.steps
                      << "  bridge=" << (m.map_bridge_broken ? "BROKEN" : "open")
                      << "  spikes=" << nav.spikes << "  waypoints=";
            for (size_t k = 0; k < nav.waypoints.size(); ++k)
                std::cout << (k ? " -> " : "") << m.word_of(nav.waypoints[k]);
            std::cout << "\n";
            for (Word y = 0; y < MAP_H; ++y)
                std::cout << "  " << grid[y] << "\n";
            std::cout << std::flush;
            continue;
        }
        if (line.starts_with("/go ")) {
            std::string room = line.substr(4);
            Word cur_c = m.find_word_exact(d.w(world.child_room));
            Word dst_c = m.perceive_word(d.w(room), true, CAT_PLACE);
            CognitiveTrace tr{};
            auto path = m.find_spatial_path(cur_c, dst_c, &tr);
            if (path.size() >= 2) {
                std::cout << "[homestead] route:";
                for (size_t k = 0; k < path.size(); ++k)
                    std::cout << (k ? " -> " : " ") << m.word_of(path[k]);
                std::cout << "  [spikes=" << tr.gpu_spikes << "]\n";
            }
            world.child_room = room;
            m.assert_fact(m.markers[LM_SELF], m.markers[LM_IN], dst_c, dst_c, 0, m.markers[LM_SELF],
                          ENG_ACTIVE | ENG_EPISODIC, QTY_NONE, PROV_OBSERVED);
            std::cout << "[homestead] moved to " << room << '\n' << std::flush;
            continue;
        }
        if (line.starts_with("/take ")) {
            std::string item = line.substr(6);
            world.child_inventory.insert(item);
            Word ic = m.perceive_word(d.w(item), true, CAT_TOOL | CAT_ENTITY);
            m.set_quantity(m.markers[LM_SELF], ic, 1, PROV_OBSERVED, m.markers[LM_SELF]);
            std::cout << "[homestead] picked up " << item << " (synced to hippocampus)\n" << std::flush;
            continue;
        }
        if (line.starts_with("/drop ")) {
            std::string item = line.substr(6);
            world.child_inventory.erase(item);
            Word ic = m.perceive_word(d.w(item), true, CAT_TOOL | CAT_ENTITY);
            m.set_quantity(m.markers[LM_SELF], ic, 0, PROV_OBSERVED, m.markers[LM_SELF]);
            std::cout << "[homestead] dropped " << item << " (synced to hippocampus)\n" << std::flush;
            continue;
        }
        if (line.starts_with("/do ")) {
            std::istringstream iss(line.substr(4));
            std::string t_str, g_str;
            iss >> t_str >> g_str;
            Word tc = m.find_word_exact(d.w(t_str));
            Word gc = m.find_word_exact(d.w(g_str));
            CognitiveTrace tr{};
            auto hp = m.plan_hierarchical(tc, gc, &tr, false);
            if (!hp.found) {
                std::cout << "[homestead] GPU planner found no achievable plan for " << t_str << " -> " << g_str
                          << (hp.missing_tool ? (" (missing tool: " + m.word_of(hp.missing_tool) + ")") : "")
                          << "  [spikes=" << tr.gpu_spikes << "]\n" << std::flush;
                continue;
            }
            std::cout << "[homestead] GPU " << (hp.hierarchical ? "hierarchical" : "direct") << " plan ("
                      << hp.steps.size() << " steps, spikes=" << tr.gpu_spikes << "):\n";
            Word ok_steps = 0;
            for (const auto &st : hp.steps) {
                std::string aname = m.word_of(st.action);
                std::string tname = m.word_of(st.target);
                // Room-aware physical navigation using GPU Hippocampal spatial graph!
                auto oit = world.objects.find(tname);
                if (oit != world.objects.end() && oit->second.location != world.child_room) {
                    Word cur_c = m.find_word_exact(d.w(world.child_room));
                    Word dst_c = m.find_word_exact(d.w(oit->second.location));
                    auto route = m.find_spatial_path(cur_c, dst_c, &tr);
                    std::cout << "  walk:";
                    if (!route.empty()) {
                        for (size_t k = 0; k < route.size(); ++k)
                            std::cout << (k ? " -> " : " ") << m.word_of(route[k]);
                    } else {
                        std::cout << " " << world.child_room << " -> " << oit->second.location;
                    }
                    std::cout << "\n";
                    world.child_room = oit->second.location;
                }
                auto ef = world.apply_action(aname, tname);
                std::cout << "  -> " << aname << "(" << tname << ") @" << world.child_room << ": "
                          << (ef.succeeded ? "OK" : "BLOCKED");
                if (ef.succeeded) {
                    ++ok_steps;
                    for (const auto &ad : ef.added) {
                        std::cout << " +" << ad;
                        Word pc = m.perceive_word(d.w(ad), true, CAT_PROPERTY, &tr);
                        m.assert_fact(st.target, m.markers[LM_HAS_PROP], pc, 0, st.action, m.markers[LM_SELF],
                                      ENG_ACTIVE | ENG_EPISODIC, QTY_NONE, PROV_EXPERIMENT, 0, &tr);
                    }
                    for (const auto &rm : ef.removed) {
                        std::cout << " -" << rm;
                        Word pc = m.perceive_word(d.w(rm), true, CAT_PROPERTY, &tr);
                        for (auto &e : m.engrams) {
                            if ((e.flags & ENG_ACTIVE) && !(e.flags & ENG_SUPERSEDED) &&
                                sdr_overlap(e.role[R_SUBJ], m.sdr_of(st.target)) >= 18 &&
                                sdr_overlap(e.role[R_OBJ], m.sdr_of(pc)) >= 18) {
                                e.flags |= ENG_SUPERSEDED;
                                m.gpu_dirty_engrams = true;
                            }
                        }
                    }
                    if (!ef.produced.empty()) {
                        std::cout << " crafted=" << ef.produced;
                        Word prc = m.perceive_word(d.w(ef.produced), true, CAT_TOOL | CAT_ENTITY, &tr);
                        m.set_quantity(m.markers[LM_SELF], prc, 1, PROV_EXPERIMENT, m.markers[LM_SELF], &tr);
                    }
                }
                std::cout << "\n";
            }
            std::cout << "[homestead] executed " << ok_steps << "/" << hp.steps.size()
                      << " steps in live world (now @" << world.child_room << ")!\n"
                      << std::flush;
            continue;
        }
        if (line == "/sleep") {
            CognitiveTrace tr{};
            Word c = m.sleep_consolidate(&tr);
            std::cout << "[sleep replay] consolidated=" << c << "  cycles=" << m.sleep_cycles_done
                      << "  engrams=" << m.engrams.size() << "  gpu_spikes=" << tr.gpu_spikes << '\n' << std::flush;
            continue;
        }
        if (line == "/inspect") {
            std::cout << "[mind state] concepts=" << (m.concepts.size() - 1) << "  engrams=" << m.engrams.size()
                      << "  schemas=" << m.schemas.size() << "  constructions=" << m.constructions.size()
                      << "  lifetime_spikes=" << m.lifetime_spikes << "  lifetime_edits=" << m.lifetime_edits
                      << "  last_conf=" << m.wm.last_confidence << "%  absent_agents=" << m.wm.agent_absent.size()
                      << "  curiosity=" << m.wm.curiosity_concepts.size() << '\n' << std::flush;
            continue;
        }
        if (line.starts_with("/ablate ")) {
            std::string arg = line.substr(8);
            m.abl_freeze = (arg == "freeze");
            m.abl_no_hipp = (arg == "hipp");
            m.abl_no_apical = (arg == "apical");
            m.abl_no_causal = (arg == "causal");
            std::cout << "[ablation] mode=" << arg << '\n' << std::flush;
            continue;
        }
        if (line == "/eval") {
            auto r = evaluate_cognition(m, d, false);
            std::cout << "[eval] total=" << r.total_correct << "/" << r.total_questions << " ("
                      << std::fixed << std::setprecision(1) << 100.0 * r.total_correct / std::max(1u, r.total_questions)
                      << "%)  spikes=" << r.gpu_spikes << "  ms=" << r.elapsed_ms << '\n' << std::flush;
            continue;
        }
        if (line.starts_with("/save ")) {
            std::string outp = line.substr(6);
            m.save(outp);
            std::cout << "[saved] " << outp << '\n' << std::flush;
            continue;
        }

        CognitiveTrace tr{};
        std::string reply = converse_turn(m, line, m.markers[LM_USER], tr);
        ++turns;
        session_spikes += tr.gpu_spikes;
        session_edits += tr.synaptic_edits;
        std::cout << "mind> " << reply << '\n';
        std::cout << "mind_turn=" << turns << "  spikes=" << tr.gpu_spikes << "  edits=" << tr.synaptic_edits
                  << "  hipp_queries=" << tr.hippocampal_queries << "  hops=" << tr.reasoning_hops
                  << "  conf=" << tr.confidence << "  uncertain=" << Word(tr.uncertain)
                  << "  contradicted=" << Word(tr.contradicted) << "  fallback=" << Word(tr.fallback_parsed)
                  << "  gpu_us=" << std::fixed << std::setprecision(1) << tr.gpu_us << '\n'
                  << std::flush;
    }
    if (!save_path.empty())
        m.save(save_path);
    if (auto *j = journal::current()) {
        j->metrics["mind_turns"] = turns;
        j->metrics["mind_spikes"] = session_spikes;
        j->metrics["mind_edits"] = session_edits;
    }
}

void help() {
    std::cout << R"(axion mind  |  unified multi-area spiking cortical-hippocampal mind (Blackwell CUDA)

  ./axion mind evolve [--population 1024] [--generations 96] [--worlds 4] [--seed N] [--controls] [--save FILE]
  ./axion mind bench  [--warps 4096] [--ticks 256] [--repeats 7]
  ./axion mind raise  [--load FILE] [--dialect english|alien-sov] [--save FILE] [--eval] [--verbose]
  ./axion mind eval   --load FILE [--dialect english|alien-sov] [--ablate none|freeze|hipp|apical|causal] [--verbose]
  ./axion mind talk   --load FILE [--dialect english|alien-sov] [--save FILE]
  ./axion mind converse --speaker FILE --listener FILE [--dialect english|alien-sov]
  ./axion mind society --load FILE [--dialect english|alien-sov]
  ./axion mind ecosystem --load FILE [--dialect english|alien-sov] [--ticks 24]
)";
}

} // namespace

int command(int argc, char **argv) {
    if (argc < 3 || std::string_view(argv[2]) == "--help") {
        help();
        return 0;
    }
    std::string_view sub = argv[2];
    Session s;
    s.open();
    if (auto *j = journal::current())
        j->device = s.name;

    if (sub == "evolve") {
        EvoConfig cfg{};
        bool controls = false;
        std::string save_path;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (f == "--controls") { controls = true; continue; }
            if (f == "--drift") { cfg.drift = 1; continue; }
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            std::string v = argv[i];
            if (f == "--population") cfg.population = parse_num(v, 32, 65536, f);
            else if (f == "--generations") cfg.generations = parse_num(v, 1, 10000, f);
            else if (f == "--worlds") cfg.worlds_per_gen = parse_num(v, 1, 32, f);
            else if (f == "--seed") cfg.seed = parse_num(v, 0, ~0u, f);
            else if (f == "--save") save_path = v;
            else throw std::invalid_argument("unknown mind evolve option " + std::string(f));
        }

        std::vector<EvoGenSummary> hist;
        CorticalGenome champ = run_evolution(s, cfg, hist, false);

        if (controls) {
            EvoConfig drift_cfg = cfg;
            drift_cfg.drift = 1;
            std::vector<EvoGenSummary> d_hist;
            run_evolution(s, drift_cfg, d_hist, true);
            Word max_sc = cfg.worlds_per_gen * EVO_MAX_PER_WORLD;

            auto eval_arm = [&](const std::string &arm_name, Word frz, Word no_ap, Word no_hp, Word no_ca, Word shuf) {
                EvoConfig ac = cfg;
                ac.population = 1; ac.generations = 1; ac.worlds_per_gen = 16;
                ac.seed = mix(cfg.seed ^ 0xabcdef01u);
                ac.freeze_plasticity = frz; ac.lesion_apical = no_ap;
                ac.lesion_hipp = no_hp; ac.lesion_causal = no_ca; ac.shuffle_feedback = shuf;

                Function fn_eval{};
                s.bind(fn_eval, "mind_evo_eval", Image::mind);
                Temporary dg(s.d, sizeof(CorticalGenome)), df(s.d, sizeof(EvoFitness));
                s.d.check(s.d.upload(dg.ptr, &champ, sizeof(champ)), "upload champ");
                Word gseed = mix(ac.seed ^ 0x55aa1234u);
                void *args[]{&dg.ptr, &df.ptr, &ac, &gseed};
                s.d.check(s.d.launch(fn_eval, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr), "eval arm");
                EvoFitness fit{};
                s.d.check(s.d.download(&fit, df.ptr, sizeof(fit)), "download arm");
                Word max16 = 16u * EVO_MAX_PER_WORLD;
                std::cout << "mind_control=" << arm_name << "  total=" << fit.total << "/" << max16
                          << "  ground=" << fit.grounding << "  episodic=" << fit.episodic
                          << "  tom=" << fit.theory_of_mind << "  causal=" << fit.causal_plan
                          << "  cf=" << fit.counterfactual << "  rev=" << fit.revision
                          << "  uncert=" << fit.uncertainty << "  logic=" << fit.logic
                          << "  social=" << fit.social_coop << "  cons=" << fit.consolidate
                          << "  abd=" << fit.abduct << "  vig=" << fit.vigilance
                          << "  rule=" << fit.rule_transfer << "  prosp=" << fit.prospective
                          << "  seq=" << fit.sequence << "  consist=" << fit.consistency
                          << "  norm=" << fit.deontic_norm << "  phys=" << fit.intuitive_phys
                          << "  sci=" << fit.sci_hyp << "  game=" << fit.game_strat
                          << "  narr=" << fit.narrative_wm << "  opt=" << fit.alloc_opt
                          << "  bayes=" << fit.bayes_update << "  hier=" << fit.abstr_plan
                          << "  ana=" << fit.analogy_xfer << "  msyn=" << fit.meta_syntax
                          << "  comp=" << fit.comp_intersect << "  selfm=" << fit.self_model
                          << "  sit=" << fit.sit_model << "  info=" << fit.info_gain
                          << "  ograph=" << fit.open_graph << "  hrank=" << fit.hyp_rank
                          << "  coref=" << fit.coref_disc << "  stoch=" << fit.stoch_plan
                          << "  clust=" << fit.cluster_odd << "  coal=" << fit.contract_coal
                          << "  gate=" << fit.ep_sem_gate << "  mdp=" << fit.policy_mdp
                          << "  dream=" << fit.dream_closure << "  mkt=" << fit.market_physics
                          << "  quant=" << fit.quant_counterex << "  thfin=" << fit.theory_finance
                          << "  deduce=" << fit.deduce_chain << "  gbayes=" << fit.graph_opt_bayes
                          << "  abdbk=" << fit.abduce_back << "  dynsoc=" << fit.dynamics_social
                          << "  cfimg=" << fit.counterfact_img << "  algec=" << fit.formal_alg_econ
                          << "  plan=" << fit.plan_chain << "  socpl=" << fit.soc_phys_lang
                          << "  csim=" << fit.causal_sim << "  sciv26=" << fit.sci_lang_v26
                          << "  csat=" << fit.constrain_sat << "  pev27=" << fit.phys_econ_v27
                          << "  ind=" << fit.induce_rel << "  aor28=" << fit.auto_orb_v28
                          << "  chrl=" << fit.chain_rule << "  rc29=" << fit.rel_ctrl_v29
                          << "  osch=" << fit.online_schema << "  qc30=" << fit.quant_crypto_v30
                          << "  rvsc=" << fit.revise_schema << "  cs31=" << fit.calc_sys_v31
                          << "  rbnd=" << fit.role_bind << "  ta32=" << fit.thermo_arch_v32
                          << "  sqpr=" << fit.seq_predict << "  s533=" << fit.stage5_sci_v33
                          << "  nmat=" << fit.neural_math << "  s534=" << fit.stage5b_sci_v34
                          << "  gwav=" << fit.graph_wave << "  s535=" << fit.stage5c_sci_v35
                          << "  sats=" << fit.sat_solve << "  s536=" << fit.stage5d_sci_v36
                          << "  visv=" << fit.visual_v1v4 << "  s637=" << fit.stage6_sci_v37
                          << "  audv=" << fit.audio_a1 << "  s638=" << fit.stage6b_sci_v38
                          << "  xmod=" << fit.crossmodal_stack << "  s639=" << fit.stage6c_sci_v39
                          << "  dlgc=" << fit.dialogue_ctx << "  s740=" << fit.stage7a_sci_v40
                          << "  nmet=" << fit.narrative_met << "  s741=" << fit.stage7b_sci_v41
                          << "  bdeb=" << fit.belief_deb << "  s742=" << fit.stage7c_sci_v42
                          << "  cdis=" << fit.coll_disc << "  s843=" << fit.stage8_sci_v43
                          << "  sres=" << fit.self_res << "  s844=" << fit.stage8b_sci_v44
                          << "  symc=" << fit.sym_calc << "  s845=" << fit.stage8c_sci_v45
                          << "  past=" << fit.prog_ast << "  s846=" << fit.stage8d_sci_v46
                          << "  lalg=" << fit.lin_alg << "  s847=" << fit.stage8e_sci_v47
                          << "  bnet=" << fit.bayes_net << "  s848=" << fit.stage8f_sci_v48
                          << "  mdpp=" << fit.mdp_plan << "  s849=" << fit.stage8g_sci_v49
                          << "  kflt=" << fit.kalman_flt << "  s850=" << fit.stage8h_sci_v50
                          << "  lqrc=" << fit.lqr_ctrl << "  s851=" << fit.stage8i_sci_v51
                          << "  ffts=" << fit.fft_spec << "  s852=" << fit.stage8j_sci_v52
                          << "  odrk=" << fit.ode_rk4 << "  s853=" << fit.stage8k_sci_v53
                          << "  sxlp=" << fit.simplex_lp << "  s854=" << fit.stage8l_sci_v54
                          << "  pcae=" << fit.pca_eigen << "  s855=" << fit.stage8m_sci_v55
                          << "  pdes=" << fit.pde_solver << "  s856=" << fit.stage8n_sci_v56
                          << "  mcmcs=" << fit.mcmc_sampler << "  s857=" << fit.stage8o_sci_v57
                          << "  mctsp=" << fit.mcts_planner << "  s858=" << fit.stage8p_sci_v58
                          << "  spikes=" << fit.spikes << "  edits=" << fit.edits << '\n';
                if (auto *j = journal::current())
                    j->scores.push_back({"Cortical evo " + arm_name, fit.total, max16});
            };
            std::cout << "mind_control=drift_endpoint  gen=" << cfg.generations
                      << "  select_mean=" << std::fixed << std::setprecision(1) << hist.back().mean_total << "/" << max_sc
                      << "  select_best=" << int(hist.back().best_total) << "/" << max_sc
                      << "  drift_mean=" << d_hist.back().mean_total << "/" << max_sc
                      << "  drift_best=" << int(d_hist.back().best_total) << "/" << max_sc << '\n';
            eval_arm("intact", 0, 0, 0, 0, 0);
            eval_arm("frozen", 1, 0, 0, 0, 0);
            eval_arm("no_apical", 0, 1, 0, 0, 0);
            eval_arm("no_hipp", 0, 0, 1, 0, 0);
            eval_arm("no_causal", 0, 0, 0, 1, 0);
            eval_arm("shuffled_feedback", 0, 0, 0, 0, 1);
        }

        if (!save_path.empty()) {
            SpikingMind newborn;
            newborn.genome = champ;
            newborn.generation = cfg.generations;
            newborn.stage = 0;
            newborn.save(save_path);
            std::cout << "Saved evolved newborn cortical genome to " << save_path << " (gen=" << cfg.generations << ").\n";
        }
        return 0;
    }

    if (sub == "bench") {
        Word warps = 4096, ticks = 256, repeats = 7;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            std::string v = argv[i];
            if (f == "--warps") warps = parse_num(v, 32, 65536, f);
            else if (f == "--ticks") ticks = parse_num(v, 16, 4096, f);
            else if (f == "--repeats") repeats = parse_num(v, 1, 51, f);
            else throw std::invalid_argument("unknown mind bench option " + std::string(f));
        }
        CorticalGenome g{};
        g.biophysics = 0x12481042u;
        run_bench(s, g, warps, ticks, repeats);
        return 0;
    }

    if (sub == "raise") {
        std::string load_path, save_path;
        bool alien = false, do_eval = false, verbose = false;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (f == "--eval") { do_eval = true; continue; }
            if (f == "--verbose") { verbose = true; continue; }
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            std::string v = argv[i];
            if (f == "--load") load_path = v;
            else if (f == "--save") save_path = v;
            else if (f == "--dialect") alien = (v == "alien-sov" || v == "alien");
            else throw std::invalid_argument("unknown mind raise option " + std::string(f));
        }
        SpikingMind child;
        child.attach_gpu(s);
        if (!load_path.empty()) {
            child.load(load_path);
        } else {
            EvoConfig ec{};
            ec.population = 512; ec.generations = 48; ec.worlds_per_gen = 3;
            std::vector<EvoGenSummary> h;
            child.genome = run_evolution(s, ec, h, true);
            child.generation = ec.generations;
            child.sync_genome_to_gpu();
        }
        Dialect d = Dialect::make(alien);
        auto rep = raise_child(child, d);
        std::cout << "mind_raised=stage5  dialect=" << (alien ? "alien_sov" : "english_svo")
                  << "  gen=" << child.generation << "  concepts=" << rep.concepts_learned
                  << "  engrams=" << rep.engrams_encoded << "  schemas=" << rep.causal_schemas
                  << "  constructions=" << rep.constructions_induced << "  consolidated=" << rep.consolidated_rules
                  << "  experiments=" << rep.physical_experiments << "  discoveries=" << rep.physical_discoveries
                  << "  spikes=" << rep.total_spikes << "  edits=" << rep.total_edits
                  << "  ms=" << std::fixed << std::setprecision(1) << rep.elapsed_ms << '\n';
        if (!save_path.empty()) {
            child.save(save_path);
            std::cout << "Saved raised spiking mind to " << save_path << ".\n";
        }
        if (do_eval) {
            auto ev = evaluate_cognition(child, d, verbose);
            std::cout << "mind_eval=intact  dialect=" << (alien ? "alien_sov" : "english_svo")
                      << "  correct=" << ev.total_correct << "  total=" << ev.total_questions
                      << "  rate=" << std::fixed << std::setprecision(1)
                      << 100.0 * ev.total_correct / std::max(1u, ev.total_questions) << "%"
                      << "  spikes=" << ev.gpu_spikes << "  edits=" << ev.synaptic_edits
                      << "  ms=" << std::setprecision(2) << ev.elapsed_ms << '\n';
            for (const auto &dom : ev.domains) {
                std::cout << "  mind_domain=\"" << dom.name << "\"  correct=" << dom.correct
                          << "  total=" << dom.total << '\n';
                if (auto *j = journal::current())
                    j->scores.push_back({dom.name, dom.correct, dom.total});
            }
        }
        return 0;
    }

    if (sub == "eval") {
        std::string load_path, ablate = "none";
        bool alien = false, verbose = false;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (f == "--verbose") { verbose = true; continue; }
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            std::string v = argv[i];
            if (f == "--load") load_path = v;
            else if (f == "--dialect") alien = (v == "alien-sov" || v == "alien");
            else if (f == "--ablate") ablate = v;
            else throw std::invalid_argument("unknown mind eval option " + std::string(f));
        }
        if (load_path.empty())
            throw std::invalid_argument("--load FILE is required");
        SpikingMind child;
        child.attach_gpu(s);
        child.load(load_path);
        child.abl_freeze = (ablate == "freeze");
        child.abl_no_hipp = (ablate == "hipp");
        child.abl_no_apical = (ablate == "apical");
        child.abl_no_causal = (ablate == "causal");
        Dialect d = Dialect::make(alien || child.dialect_seed != 0);
        auto ev = evaluate_cognition(child, d, verbose);
        std::cout << "mind_eval=" << ablate << "  dialect=" << (d.alien_sov ? "alien_sov" : "english_svo")
                  << "  correct=" << ev.total_correct << "  total=" << ev.total_questions
                  << "  rate=" << std::fixed << std::setprecision(1)
                  << 100.0 * ev.total_correct / std::max(1u, ev.total_questions) << "%"
                  << "  spikes=" << ev.gpu_spikes << "  edits=" << ev.synaptic_edits
                  << "  ms=" << std::setprecision(2) << ev.elapsed_ms << '\n';
        for (const auto &dom : ev.domains) {
            std::cout << "  mind_domain=\"" << dom.name << "\"  correct=" << dom.correct
                      << "  total=" << dom.total << '\n';
            if (auto *j = journal::current())
                j->scores.push_back({dom.name + (ablate == "none" ? "" : " [" + ablate + "]"), dom.correct, dom.total});
        }
        return 0;
    }

    if (sub == "talk") {
        std::string load_path, save_path;
        bool alien = false;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            std::string v = argv[i];
            if (f == "--load") load_path = v;
            else if (f == "--save") save_path = v;
            else if (f == "--dialect") alien = (v == "alien-sov" || v == "alien");
            else throw std::invalid_argument("unknown mind talk option " + std::string(f));
        }
        if (load_path.empty())
            throw std::invalid_argument("--load FILE is required");
        SpikingMind child;
        child.attach_gpu(s);
        child.load(load_path);
        run_interactive(s, child, alien || child.dialect_seed != 0, save_path, load_path);
        return 0;
    }

    if (sub == "converse") {
        std::string sp, ls;
        bool alien = false;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            std::string v = argv[i];
            if (f == "--speaker") sp = v;
            else if (f == "--listener") ls = v;
            else if (f == "--dialect") alien = (v == "alien-sov" || v == "alien");
            else throw std::invalid_argument("unknown mind converse option " + std::string(f));
        }
        if (sp.empty() || ls.empty())
            throw std::invalid_argument("--speaker FILE and --listener FILE are required");
        run_two_minds(s, sp, ls, alien);
        return 0;
    }

    if (sub == "society") {
        std::string load_path;
        bool alien = false;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            std::string v = argv[i];
            if (f == "--load") load_path = v;
            else if (f == "--dialect") alien = (v == "alien-sov" || v == "alien");
            else throw std::invalid_argument("unknown mind society option " + std::string(f));
        }
        if (load_path.empty())
            throw std::invalid_argument("--load FILE is required");
        run_society(s, load_path, alien);
        return 0;
    }

    if (sub == "ecosystem") {
        std::string load_path;
        bool alien = false;
        Word ticks = 24;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            std::string v = argv[i];
            if (f == "--load") load_path = v;
            else if (f == "--dialect") alien = (v == "alien-sov" || v == "alien");
            else if (f == "--ticks") ticks = parse_num(v, 4, 2048, f);
            else throw std::invalid_argument("unknown mind ecosystem option " + std::string(f));
        }
        if (load_path.empty())
            throw std::invalid_argument("--load FILE is required");
        run_ecosystem(s, load_path, alien, ticks);
        return 0;
    }

    throw std::invalid_argument("unknown mind subcommand " + std::string(sub));
}

} // namespace axion::mind
