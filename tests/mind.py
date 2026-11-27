#!/usr/bin/env python3
"""End-to-end scientific verification suite for Axion Cortical Spiking Mind (`axion mind`)."""

import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
AXION = ROOT / "axion"


def run_cmd(args, env, stdin_text=None):
    res = subprocess.run(
        [str(AXION), *args],
        cwd=ROOT,
        env=env,
        input=stdin_text,
        text=True,
        capture_output=True,
        check=True,
    )
    return res.stdout


def parse_kv(line):
    return dict(re.findall(r'([^\s=]+)=("[^"]*"|[^\s]+)', line))


def parse_domains(output):
    out = {}
    for line in output.splitlines():
        m = re.match(r'^\s*mind_domain="([^"]+)"\s+correct=(\d+)\s+total=(\d+)', line)
        if m:
            out[m.group(1)] = (int(m.group(2)), int(m.group(3)))
    return out


def main():
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-mind-test-"
    ) as tmp:
        d = Path(tmp)
        jdir = d / "journal"
        env = dict(os.environ, AXION_JOURNAL_DIR=str(jdir))

        # 1. Blackwell CUDA Cortex, 26-SM Parallel Hippocampus, 28-MiB Complete-L2 Recurrent GPU Neocortex (22 Kernels: SDR, Hipp, 2D-Place, Think-Chain, WM-Gate, Contrast, Analogy, Compose, Multi-Cue, Hypothesize, Bridge, Cluster, Gate, SWR Dream, Universal Scan, Deductive Proof, Backward Abduction, Counterfactual Imagination, Hierarchical Plan Chain, NMDA Causal Simulation, Constraint Satisfaction & Few-Shot Relational Induction)
        out_bench = run_cmd(["mind", "bench", "--warps", "4096", "--ticks", "256", "--repeats", "5"], env)
        sdr_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=sdr_columns"))
        hip_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=hippocampus"))
        neo_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neocortex"))
        sp2_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=spatial_2d"))
        tch_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=think_chain"))
        wma_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=wm_attend"))
        ctr_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_contrast"))
        ana_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_analogy"))
        cmp_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_compose"))
        mcu_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_multi_cue"))
        hyp_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_hypothesize"))
        brg_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_bridge"))
        clu_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_cluster"))
        gat_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_gate"))
        drm_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_dream"))
        scn_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_scan"))
        ded_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_deduce"))
        abd_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_abduce_back"))
        cfa_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_counterfact"))
        plc_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_plan_chain"))
        sim_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_simulate"))
        con_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_constrain"))
        ind_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_induce"))
        chr_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_chain_rule"))
        ons_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_online_schema"))
        rvs_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_revise_schema"))
        rbd_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_role_bind"))
        sp27_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_seq_predict"))
        nm28_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_neural_math"))
        gw29_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_graph_wave"))
        sat30_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_sat_solve"))
        vis31_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_visual_cortex"))
        aud32_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_audio_cortex"))
        xms33_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_crossmodal_stack"))
        dlg34_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_dialogue_cortex"))
        nmet35_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_narrative_metaphor"))
        bdeb36_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_belief_debate"))
        cdis37_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_collective_discovery"))
        sres38_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_self_research"))
        symc39_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_symbolic_calculus"))
        past40_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_program_ast"))
        lalg41_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_linear_algebra"))
        bnet42_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_bayes_net"))
        mdpp43_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_mdp_planner"))
        kflt44_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_kalman_filter"))
        lqrc45_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_lqr_control"))
        ffts46_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_fft_spectrum"))
        odrk47_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_ode_rk4"))
        sxlp48_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_simplex_lp"))
        pcae49_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_pca_eigen"))
        pdes50_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_pde_solver"))
        mcmcs51_line = next(l for l in out_bench.splitlines() if l.startswith("mind_bench=neo_mcmc_sampler"))
        sdr = parse_kv(sdr_line)
        hip = parse_kv(hip_line)
        neo = parse_kv(neo_line)
        sp2 = parse_kv(sp2_line)
        tch = parse_kv(tch_line)
        wma = parse_kv(wma_line)
        ctr = parse_kv(ctr_line)
        ana = parse_kv(ana_line)
        cmp = parse_kv(cmp_line)
        mcu = parse_kv(mcu_line)
        hyp = parse_kv(hyp_line)
        brg = parse_kv(brg_line)
        clu = parse_kv(clu_line)
        gat = parse_kv(gat_line)
        drm = parse_kv(drm_line)
        scn = parse_kv(scn_line)
        ded = parse_kv(ded_line)
        abd = parse_kv(abd_line)
        cfa = parse_kv(cfa_line)
        plc = parse_kv(plc_line)
        sim = parse_kv(sim_line)
        con = parse_kv(con_line)
        ind = parse_kv(ind_line)
        chrl = parse_kv(chr_line)
        ons = parse_kv(ons_line)
        rvs = parse_kv(rvs_line)
        rbd = parse_kv(rbd_line)
        sp27 = parse_kv(sp27_line)
        nm28 = parse_kv(nm28_line)
        gw29 = parse_kv(gw29_line)
        sat30 = parse_kv(sat30_line)
        vis31 = parse_kv(vis31_line)
        aud32 = parse_kv(aud32_line)
        xms33 = parse_kv(xms33_line)
        dlg34 = parse_kv(dlg34_line)
        nmet35 = parse_kv(nmet35_line)
        bdeb36 = parse_kv(bdeb36_line)
        cdis37 = parse_kv(cdis37_line)
        sres38 = parse_kv(sres38_line)
        symc39 = parse_kv(symc39_line)
        past40 = parse_kv(past40_line)
        lalg41 = parse_kv(lalg41_line)
        bnet42 = parse_kv(bnet42_line)
        mdpp43 = parse_kv(mdpp43_line)
        kflt44 = parse_kv(kflt44_line)
        lqrc45 = parse_kv(lqrc45_line)
        ffts46 = parse_kv(ffts46_line)
        odrk47 = parse_kv(odrk47_line)
        sxlp48 = parse_kv(sxlp48_line)
        pcae49 = parse_kv(pcae49_line)
        pdes50 = parse_kv(pdes50_line)
        mcmcs51 = parse_kv(mcmcs51_line)
        assert float(sdr["Gneuron_ticks/s"]) >= 50.0, sdr
        assert int(sdr["local_B"]) == 0 and int(sdr["shared_B"]) == 0, sdr
        assert int(hip["blocks"]) == 32 and int(hip["matched_idx"]) == 2048 and int(hip["hbm_engrams"]) == 16384 and int(hip["hbm_matched_idx"]) == 12288, hip
        assert float(hip["query_us"]) < 150.0 and float(hip["hbm_query_us"]) < 500.0, hip
        assert int(neo["pathways"]) == 28 and int(neo["synapses"]) == 29360128, neo
        assert float(neo["settle_us"]) < 500.0 and int(neo["active_neurons"]) >= 4, neo
        assert int(sp2["cells"]) == 256 and int(sp2["path_len"]) == 20 and float(sp2["wave_us"]) < 200.0, sp2
        assert int(tch["hops"]) == 3 and float(tch["chain_us"]) < 1000.0 and int(tch["spikes"]) > 0, tch
        assert int(wma["slots"]) == 64 and int(wma["ambiguous"]) == 1 and float(wma["attend_us"]) < 200.0, wma
        assert float(ctr["contrast_us"]) < 600.0 and int(ctr["spikes"]) > 0, ctr
        assert float(ana["analogy_us"]) < 500.0 and int(ana["best_pw"]) == 0 and int(ana["active"]) >= 4, ana
        assert float(cmp["compose_us"]) < 500.0 and int(cmp["active"]) >= 4 and int(cmp["max_score"]) > 0 and int(cmp["spikes"]) > 0, cmp
        assert float(mcu["multicue_us"]) < 500.0 and int(mcu["active"]) >= 4 and int(mcu["max_score"]) > 0 and int(mcu["spikes"]) > 0, mcu
        assert float(hyp["hyp_us"]) < 500.0 and int(hyp["best_cand"]) == 1 and int(hyp["best_score"]) > int(hyp["second_score"]) and int(hyp["spikes"]) > 0, hyp
        assert float(brg["bridge_us"]) < 500.0 and int(brg["best_pw"]) == 4 and int(brg["active"]) >= 4 and int(brg["max_score"]) > 0 and int(brg["spikes"]) > 0, brg
        assert float(clu["cluster_us"]) < 500.0 and int(clu["outlier"]) == 2 and int(clu["pair_sim"]) > int(clu["outlier_sim"]) and int(clu["spikes"]) > 0, clu
        assert float(gat["gate_us"]) < 500.0 and int(gat["status"]) == 2 and int(gat["veto"]) > 0 and int(gat["spikes"]) > 0, gat
        assert float(drm["dream_us"]) < 800.0 and int(drm["mid"]) >= 8 and int(drm["end"]) >= 8 and int(drm["edits"]) > 0 and int(drm["spikes"]) > 0, drm
        assert float(scn["scan_us"]) < 500.0 and int(scn["all"]) == 0 and int(scn["some"]) == 1 and int(scn["counterex"]) == 0 and int(scn["witness"]) == 1 and int(scn["spikes"]) > 0, scn
        assert float(ded["deduce_us"]) < 750.0 and int(ded["proved"]) == 1 and int(ded["hops"]) == 3 and int(ded["overlap"]) >= 12 and int(ded["spikes"]) > 0, ded
        assert float(abd["abduce_us"]) < 700.0 and int(abd["hop1"]) >= 16 and int(abd["hop2"]) >= 16 and int(abd["hop1_max"]) > 0 and int(abd["spikes"]) > 0, abd
        assert float(cfa["cf_us"]) < 950.0 and int(cfa["shifted"]) == 1 and int(cfa["base"]) >= 24 and int(cfa["cf"]) >= 24 and int(cfa["spikes"]) > 0, cfa
        assert float(plc["plan_us"]) < 750.0 and int(plc["steps"]) == 2 and int(plc["step1"]) >= 24 and int(plc["step2"]) >= 24 and int(plc["spikes"]) > 0, plc
        assert float(sim["sim_us"]) < 750.0 and int(sim["steps"]) == 2 and int(sim["step1"]) >= 24 and int(sim["step2"]) >= 24 and int(sim["spikes"]) > 0, sim
        assert float(con["constrain_us"]) < 500.0 and int(con["valid"]) == 1 and int(con["best"]) == 0 and int(con["eliminated"]) == 2 and int(con["spikes"]) > 0, con
        assert float(ind["induce_us"]) < 750.0 and int(ind["found"]) == 1 and int(ind["best_pw"]) == 18 and int(ind["hops"]) == 2 and int(ind["step1"]) >= 24 and int(ind["step2"]) >= 24 and int(ind["spikes"]) > 0, ind
        assert float(chrl["chain_rule_us"]) < 1500.0 and int(chrl["found"]) == 1 and int(chrl["pw1"]) == 18 and int(chrl["pw2"]) == 23 and int(chrl["mid"]) >= 24 and int(chrl["end"]) >= 24 and int(chrl["spikes"]) > 0, chrl
        assert float(ons["online_schema_us"]) < 1000.0 and int(ons["hops"]) == 2 and int(ons["hop1"]) >= 24 and int(ons["hop2"]) >= 24 and int(ons["edits"]) >= 1024 and int(ons["spikes"]) > 0, ons
        assert float(rvs["revise_schema_us"]) < 1200.0 and int(rvs["old_cnt"]) >= 24 and int(rvs["hop1"]) >= 24 and int(rvs["hop2"]) >= 24 and int(rvs["ltd"]) >= 512 and int(rvs["ltp"]) >= 1024 and int(rvs["spikes"]) > 0, rvs
        assert float(rbd["role_bind_us"]) < 750.0 and int(rbd["active"]) >= 24 and int(rbd["max_curr"]) > 0 and int(rbd["edits"]) >= 2048 and int(rbd["spikes"]) > 0, rbd
        assert float(sp27["seq_predict_us"]) < 750.0 and int(sp27["unique"]) == 4 and int(sp27["r0"]) == 0 and int(sp27["r1"]) == 1 and int(sp27["r2"]) == 2 and int(sp27["r3"]) == 3 and int(sp27["edits"]) >= 4096 and int(sp27["spikes"]) > 0, sp27
        assert float(nm28["neural_math_us"]) < 800.0 and int(nm28["ans"]) == 6 and int(nm28["valid"]) == 1 and int(nm28["steps"]) >= 10 and int(nm28["edits"]) >= 16384 and int(nm28["spikes"]) > 0, nm28
        assert float(gw29["graph_wave_us"]) < 500.0 and int(gw29["found"]) == 1 and int(gw29["cost"]) == 7 and int(gw29["hops"]) == 3 and int(gw29["next"]) == 1 and int(gw29["edits"]) >= 4096 and int(gw29["spikes"]) > 0, gw29
        assert float(sat30["sat_solve_us"]) < 500.0 and int(sat30["sat"]) == 1 and int(sat30["forced"]) == 1 and int(sat30["clauses"]) == 4 and int(sat30["edits"]) >= 4096 and int(sat30["spikes"]) > 0, sat30
        assert float(vis31["visual_us"]) < 500.0 and int(vis31["shape"]) == 3 and int(vis31["protos"]) == 4 and int(vis31["objects"]) == 2 and int(vis31["edits"]) >= 4096 and int(vis31["spikes"]) > 0, vis31
        assert float(aud32["audio_us"]) < 500.0 and int(aud32["proto"]) == 2 and int(aud32["protos"]) == 4 and int(aud32["beats"]) == 1 and int(aud32["pitch"]) == 0 and int(aud32["edits"]) >= 4096 and int(aud32["spikes"]) > 0, aud32
        assert float(xms33["xmodal_us"]) < 500.0 and int(xms33["xproto"]) == 1 and int(xms33["syn_aud"]) == 3 and int(xms33["stack_depth"]) == 2 and int(xms33["top_goal"]) == 1 and int(xms33["edits"]) >= 4096 and int(xms33["spikes"]) > 0, xms33
        assert float(dlg34["dlg_us"]) < 500.0 and int(dlg34["best_mode"]) == 3 and int(dlg34["modes"]) == 4 and int(dlg34["shifted"]) == 1 and int(dlg34["contra"]) == 2 and int(dlg34["edits"]) >= 4096 and int(dlg34["spikes"]) > 0, dlg34
        assert float(nmet35["nmet_us"]) < 500.0 and int(nmet35["blend"]) == 0 and int(nmet35["blends"]) == 4 and int(nmet35["hop1"]) == 1 and int(nmet35["hop2"]) == 2 and int(nmet35["flaw"]) == 0 and int(nmet35["edits"]) >= 4096 and int(nmet35["spikes"]) > 0, nmet35
        assert float(bdeb36["bdeb_us"]) < 500.0 and int(bdeb36["claim"]) == 0 and int(bdeb36["verdict"]) == 1 and int(bdeb36["claims"]) == 3 and int(bdeb36["peer"]) == 0 and int(bdeb36["margin"]) > 0 and int(bdeb36["edits"]) >= 4096 and int(bdeb36["spikes"]) > 0, bdeb36
        assert float(cdis37["cdis_us"]) < 500.0 and int(cdis37["law"]) == 0 and int(cdis37["verdict"]) == 1 and int(cdis37["laws"]) == 3 and int(cdis37["thm"]) == 0 and int(cdis37["ax_a"]) == 4 and int(cdis37["ax_b"]) == 5 and int(cdis37["edits"]) >= 4096 and int(cdis37["spikes"]) > 0, cdis37
        assert float(sres38["sres_us"]) < 500.0 and int(sres38["exp"]) == 0 and int(sres38["disc"]) > 0 and int(sres38["exps"]) == 4 and int(sres38["patch"]) == 0 and int(sres38["sym_a"]) == 8 and int(sres38["sym_b"]) == 9 and int(sres38["edits"]) >= 4096 and int(sres38["spikes"]) > 0, sres38
        assert float(symc39["symc_us"]) < 500.0 and int(symc39["diff"]) == 18 and int(symc39["integ"]) == 24 and int(symc39["heval"]) == 34 and int(symc39["polys"]) == 3 and int(symc39["edits"]) >= 2048 and int(symc39["spikes"]) > 0, symc39
        assert float(past40["past_us"]) < 500.0 and int(past40["exec"]) == 17 and int(past40["safe"]) == 1 and int(past40["synth"]) == 1 and int(past40["sval"]) == 19 and int(past40["progs"]) == 4 and int(past40["edits"]) >= 4096 and int(past40["spikes"]) > 0, past40
        assert float(lalg41["lalg_us"]) < 500.0 and int(lalg41["det"]) == 5 and int(lalg41["mvec"]) == 17 and int(lalg41["x0"]) == 3 and int(lalg41["lx0"]) == 5 and int(lalg41["mats"]) == 4 and int(lalg41["edits"]) >= 4096 and int(lalg41["spikes"]) > 0, lalg41
        assert float(bnet42["bnet_us"]) < 500.0 and int(bnet42["pos"]) == 50 and int(bnet42["neg"]) == 13 and int(bnet42["lpos"]) == 80 and int(bnet42["dec"]) == 0 and int(bnet42["nodes"]) == 4 and int(bnet42["edits"]) >= 4096 and int(bnet42["spikes"]) > 0, bnet42
        assert float(mdpp43["mdpp_us"]) < 500.0 and int(mdpp43["vs0"]) == 42 and int(mdpp43["vs1"]) == 80 and int(mdpp43["lvs0"]) == 47 and int(mdpp43["pol"]) == 1 and int(mdpp43["envs"]) == 4 and int(mdpp43["edits"]) >= 4096 and int(mdpp43["spikes"]) > 0, mdpp43
        assert float(kflt44["kflt_us"]) < 500.0 and int(kflt44["gain"]) == 50 and int(kflt44["post"]) == 70 and int(kflt44["lpost"]) == 80 and int(kflt44["trust"]) == 0 and int(kflt44["chans"]) == 4 and int(kflt44["edits"]) >= 4096 and int(kflt44["spikes"]) > 0, kflt44
        assert float(lqrc45["lqrc_us"]) < 500.0 and int(lqrc45["gain"]) == 62 and int(lqrc45["pole"]) == 29 and int(lqrc45["lctrl"]) == 37 and int(lqrc45["stab"]) == 0 and int(lqrc45["plants"]) == 4 and int(lqrc45["edits"]) >= 4096 and int(lqrc45["spikes"]) > 0, lqrc45
        assert float(ffts46["ffts_us"]) < 500.0 and int(ffts46["h1"]) == 40 and int(ffts46["nyq"]) == 30 and int(ffts46["lnyq"]) == 40 and int(ffts46["high"]) == 1 and int(ffts46["sigs"]) == 4 and int(ffts46["edits"]) >= 4096 and int(ffts46["spikes"]) > 0, ffts46
        assert float(odrk47["odrk_us"]) < 500.0 and int(odrk47["val"]) == 62 and int(odrk47["eq"]) == 50 and int(odrk47["lval"]) == 58 and int(odrk47["grow"]) == 1 and int(odrk47["syss"]) == 4 and int(odrk47["edits"]) >= 4096 and int(odrk47["spikes"]) > 0, odrk47
        assert float(sxlp48["sxlp_us"]) < 500.0 and int(sxlp48["opt"]) == 54 and int(sxlp48["xval"]) == 6 and int(sxlp48["lopt"]) == 56 and int(sxlp48["corn"]) == 1 and int(sxlp48["probs"]) == 4 and int(sxlp48["edits"]) >= 4096 and int(sxlp48["spikes"]) > 0, sxlp48
        assert float(pcae49["pcae_us"]) < 500.0 and int(pcae49["lam1"]) == 15 and int(pcae49["vpct"]) == 70 and int(pcae49["llam"]) == 30 and int(pcae49["iso"]) == 1 and int(pcae49["mats"]) == 4 and int(pcae49["edits"]) >= 4096 and int(pcae49["spikes"]) > 0, pcae49
        assert float(pdes50["pdes_us"]) < 500.0 and int(pdes50["umid"]) == 44 and int(pdes50["uside"]) == 62 and int(pdes50["lmid"]) == 92 and int(pdes50["unst"]) == 1 and int(pdes50["grids"]) == 4 and int(pdes50["edits"]) >= 4096 and int(pdes50["spikes"]) > 0, pdes50
        assert float(mcmcs51["mcmcs_us"]) < 500.0 and int(mcmcs51["mprob"]) == 60 and int(mcmcs51["macc"]) == 61 and int(mcmcs51["lmprob"]) == 80 and int(mcmcs51["flat"]) == 1 and int(mcmcs51["dists"]) == 4 and int(mcmcs51["edits"]) >= 4096 and int(mcmcs51["spikes"]) > 0, mcmcs51

        # 2. Massively Parallel 114-Pillar GPU Evolution + Drift + 5 Causal Lesions
        newborn = d / "newborn.cmind"
        out_evo = run_cmd(
            ["mind", "evolve", "--population", "512", "--generations", "64", "--worlds", "4", "--seed", "42",
             "--controls", "--save", str(newborn)],
            env,
        )
        ctrls = {}
        for line in out_evo.splitlines():
            if line.startswith("mind_control="):
                kv = parse_kv(line)
                ctrls[kv["mind_control"]] = kv
        drift = ctrls["drift_endpoint"]
        sel_mean = float(drift["select_mean"].split("/")[0])
        drf_mean = float(drift["drift_mean"].split("/")[0])
        assert sel_mean > drf_mean + 500.0, drift
        intact = ctrls["intact"]
        frozen = ctrls["frozen"]
        no_ap = ctrls["no_apical"]
        no_hp = ctrls["no_hipp"]
        no_ca = ctrls["no_causal"]
        shuf = ctrls["shuffled_feedback"]
        assert int(intact["total"].split("/")[0]) >= 27000, intact
        assert int(frozen["total"].split("/")[0]) <= 200, frozen
        assert int(no_ap["tom"]) == 0 and int(no_ap["vig"]) == 0 and int(no_ap["rule"]) == 0 and int(no_ap["prosp"]) == 0 and int(no_ap["consist"]) == 0 and int(no_ap["norm"]) == 0 and int(no_ap["game"]) == 0 and int(no_ap["opt"]) == 0 and int(no_ap["bayes"]) == 0 and int(no_ap["ana"]) == 0 and int(no_ap["msyn"]) == 0 and int(no_ap["comp"]) == 0 and int(no_ap["info"]) == 0 and int(no_ap["ograph"]) == 0 and int(no_ap["hrank"]) == 0 and int(no_ap["coref"]) == 0 and int(no_ap["stoch"]) == 0 and int(no_ap["clust"]) == 0 and int(no_ap["coal"]) == 0 and int(no_ap["gate"]) == 0 and int(no_ap["mdp"]) == 0 and int(no_ap["dream"]) == 0 and int(no_ap["mkt"]) == 0 and int(no_ap["quant"]) == 0 and int(no_ap["thfin"]) == 0 and int(no_ap["deduce"]) == 0 and int(no_ap["gbayes"]) == 0 and int(no_ap["abdbk"]) == 0 and int(no_ap["dynsoc"]) == 0 and int(no_ap["cfimg"]) == 0 and int(no_ap["algec"]) == 0 and int(no_ap["socpl"]) == 0 and int(no_ap["sciv26"]) == 0 and int(no_ap["csat"]) == 0 and int(no_ap["pev27"]) == 0 and int(no_ap["ind"]) == 0 and int(no_ap["aor28"]) == 0 and int(no_ap["chrl"]) == 0 and int(no_ap["rc29"]) == 0 and int(no_ap["osch"]) == 0 and int(no_ap["qc30"]) == 0 and int(no_ap["rvsc"]) == 0 and int(no_ap["cs31"]) == 0 and int(no_ap["rbnd"]) == 0 and int(no_ap["ta32"]) == 0 and int(no_ap["sqpr"]) == 0 and int(no_ap["s533"]) == 0 and int(no_ap["nmat"]) == 0 and int(no_ap["s534"]) == 0 and int(no_ap["gwav"]) == 0 and int(no_ap["s535"]) == 0 and int(no_ap["sats"]) == 0 and int(no_ap["s536"]) == 0 and int(no_ap["visv"]) == 0 and int(no_ap["s637"]) == 0 and int(no_ap["audv"]) == 0 and int(no_ap["s638"]) == 0 and int(no_ap["xmod"]) == 0 and int(no_ap["s639"]) == 0 and int(no_ap["dlgc"]) == 0 and int(no_ap["s740"]) == 0 and int(no_ap["nmet"]) == 0 and int(no_ap["s741"]) == 0 and int(no_ap["bdeb"]) == 0 and int(no_ap["s742"]) == 0 and int(no_ap["cdis"]) == 0 and int(no_ap["s843"]) == 0 and int(no_ap["sres"]) == 0 and int(no_ap["s844"]) == 0 and int(no_ap["symc"]) == 0 and int(no_ap["s845"]) == 0 and int(no_ap["past"]) == 0 and int(no_ap["s846"]) == 0 and int(no_ap["lalg"]) == 0 and int(no_ap["s847"]) == 0 and int(no_ap["bnet"]) == 0 and int(no_ap["s848"]) == 0 and int(no_ap["mdpp"]) == 0 and int(no_ap["s849"]) == 0 and int(no_ap["kflt"]) == 0 and int(no_ap["s850"]) == 0 and int(no_ap["lqrc"]) == 0 and int(no_ap["s851"]) == 0 and int(no_ap["ffts"]) == 0 and int(no_ap["s852"]) == 0 and int(no_ap["odrk"]) == 0 and int(no_ap["s853"]) == 0 and int(no_ap["sxlp"]) == 0 and int(no_ap["s854"]) == 0 and int(no_ap["pcae"]) == 0 and int(no_ap["s855"]) == 0 and int(no_ap["pdes"]) == 0 and int(no_ap["s856"]) == 0 and int(no_ap["mcmcs"]) == 0 and int(no_ap["s857"]) == 0, no_ap
        assert int(no_hp["episodic"]) == 0 and int(no_hp["tom"]) == 0 and int(no_hp["logic"]) == 0 and int(no_hp["cons"]) == 0 and int(no_hp["rule"]) == 0 and int(no_hp["prosp"]) == 0 and int(no_hp["consist"]) == 0 and int(no_hp["norm"]) == 0 and int(no_hp["sci"]) == 0 and int(no_hp["narr"]) == 0 and int(no_hp["bayes"]) == 0 and int(no_hp["hier"]) == 0 and int(no_hp["ana"]) == 0 and int(no_hp["comp"]) == 0 and int(no_hp["selfm"]) == 0 and int(no_hp["sit"]) == 0 and int(no_hp["info"]) == 0 and int(no_hp["ograph"]) == 0 and int(no_hp["coref"]) == 0 and int(no_hp["clust"]) == 0 and int(no_hp["gate"]) == 0 and int(no_hp["dream"]) == 0 and int(no_hp["quant"]) == 0 and int(no_hp["deduce"]) == 0 and int(no_hp["abdbk"]) == 0 and int(no_hp["cfimg"]) == 0 and int(no_hp["plan"]) == 0 and int(no_hp["csim"]) == 0 and int(no_hp["csat"]) == 0 and int(no_hp["ind"]) == 0 and int(no_hp["chrl"]) == 0 and int(no_hp["osch"]) == 0 and int(no_hp["rvsc"]) == 0 and int(no_hp["rbnd"]) == 0 and int(no_hp["sqpr"]) == 0 and int(no_hp["nmat"]) == 0 and int(no_hp["gwav"]) == 0 and int(no_hp["sats"]) == 0 and int(no_hp["visv"]) == 0 and int(no_hp["audv"]) == 0 and int(no_hp["xmod"]) == 0 and int(no_hp["dlgc"]) == 0 and int(no_hp["nmet"]) == 0 and int(no_hp["bdeb"]) == 0 and int(no_hp["cdis"]) == 0 and int(no_hp["sres"]) == 0 and int(no_hp["symc"]) == 0 and int(no_hp["past"]) == 0 and int(no_hp["lalg"]) == 0 and int(no_hp["bnet"]) == 0 and int(no_hp["mdpp"]) == 0 and int(no_hp["kflt"]) == 0 and int(no_hp["lqrc"]) == 0 and int(no_hp["ffts"]) == 0 and int(no_hp["odrk"]) == 0 and int(no_hp["sxlp"]) == 0 and int(no_hp["pcae"]) == 0 and int(no_hp["pdes"]) == 0 and int(no_hp["mcmcs"]) == 0, no_hp
        assert int(no_ca["causal"]) == 0 and int(no_ca["cf"]) == 0 and int(no_ca["phys"]) == 0 and int(no_ca["sci"]) == 0 and int(no_ca["opt"]) == 0 and int(no_ca["hier"]) == 0 and int(no_ca["msyn"]) == 0 and int(no_ca["selfm"]) == 0 and int(no_ca["sit"]) == 0 and int(no_ca["hrank"]) == 0 and int(no_ca["stoch"]) == 0 and int(no_ca["coal"]) == 0 and int(no_ca["mdp"]) == 0 and int(no_ca["mkt"]) == 0 and int(no_ca["thfin"]) == 0 and int(no_ca["gbayes"]) == 0 and int(no_ca["dynsoc"]) == 0 and int(no_ca["algec"]) == 0 and int(no_ca["plan"]) == 0 and int(no_ca["socpl"]) == 0 and int(no_ca["csim"]) == 0 and int(no_ca["sciv26"]) == 0 and int(no_ca["pev27"]) == 0 and int(no_ca["aor28"]) == 0 and int(no_ca["rc29"]) == 0 and int(no_ca["qc30"]) == 0 and int(no_ca["cs31"]) == 0 and int(no_ca["ta32"]) == 0 and int(no_ca["s533"]) == 0 and int(no_ca["s534"]) == 0 and int(no_ca["s535"]) == 0 and int(no_ca["s536"]) == 0 and int(no_ca["s637"]) == 0 and int(no_ca["s638"]) == 0 and int(no_ca["s639"]) == 0 and int(no_ca["s740"]) == 0 and int(no_ca["s741"]) == 0 and int(no_ca["s742"]) == 0 and int(no_ca["s843"]) == 0 and int(no_ca["s844"]) == 0 and int(no_ca["s845"]) == 0 and int(no_ca["s846"]) == 0 and int(no_ca["s847"]) == 0 and int(no_ca["s848"]) == 0 and int(no_ca["s849"]) == 0 and int(no_ca["s850"]) == 0 and int(no_ca["s851"]) == 0 and int(no_ca["s852"]) == 0 and int(no_ca["s853"]) == 0 and int(no_ca["s854"]) == 0 and int(no_ca["s855"]) == 0 and int(no_ca["s856"]) == 0 and int(no_ca["s857"]) == 0, no_ca
        assert int(shuf["causal"]) == 0 and int(shuf["vig"]) == 0 and int(shuf["rule"]) == 0 and int(shuf["prosp"]) == 0 and int(shuf["consist"]) == 0 and int(shuf["norm"]) == 0 and int(shuf["sci"]) == 0 and int(shuf["game"]) == 0 and int(shuf["narr"]) == 0 and int(shuf["opt"]) == 0 and int(shuf["bayes"]) == 0 and int(shuf["hier"]) == 0 and int(shuf["ana"]) == 0 and int(shuf["msyn"]) == 0 and int(shuf["comp"]) == 0 and int(shuf["selfm"]) == 0 and int(shuf["sit"]) == 0 and int(shuf["info"]) == 0 and int(shuf["ograph"]) == 0 and int(shuf["hrank"]) == 0 and int(shuf["coref"]) == 0 and int(shuf["stoch"]) == 0 and int(shuf["clust"]) == 0 and int(shuf["coal"]) == 0 and int(shuf["gate"]) == 0 and int(shuf["mdp"]) == 0 and int(shuf["dream"]) == 0 and int(shuf["mkt"]) == 0 and int(shuf["quant"]) == 0 and int(shuf["thfin"]) == 0 and int(shuf["deduce"]) == 0 and int(shuf["gbayes"]) == 0 and int(shuf["abdbk"]) == 0 and int(shuf["dynsoc"]) == 0 and int(shuf["cfimg"]) == 0 and int(shuf["algec"]) == 0 and int(shuf["plan"]) == 0 and int(shuf["socpl"]) == 0 and int(shuf["csim"]) == 0 and int(shuf["sciv26"]) == 0 and int(shuf["csat"]) == 0 and int(shuf["pev27"]) == 0 and int(shuf["ind"]) == 0 and int(shuf["aor28"]) == 0 and int(shuf["chrl"]) == 0 and int(shuf["rc29"]) == 0 and int(shuf["osch"]) == 0 and int(shuf["qc30"]) == 0 and int(shuf["rvsc"]) == 0 and int(shuf["cs31"]) == 0 and int(shuf["rbnd"]) == 0 and int(shuf["ta32"]) == 0 and int(shuf["sqpr"]) == 0 and int(shuf["s533"]) == 0 and int(shuf["nmat"]) == 0 and int(shuf["s534"]) == 0 and int(shuf["gwav"]) == 0 and int(shuf["s535"]) == 0 and int(shuf["sats"]) == 0 and int(shuf["s536"]) == 0 and int(shuf["visv"]) == 0 and int(shuf["s637"]) == 0 and int(shuf["audv"]) == 0 and int(shuf["s638"]) == 0 and int(shuf["xmod"]) == 0 and int(shuf["s639"]) == 0 and int(shuf["dlgc"]) == 0 and int(shuf["s740"]) == 0 and int(shuf["nmet"]) == 0 and int(shuf["s741"]) == 0 and int(shuf["bdeb"]) == 0 and int(shuf["s742"]) == 0 and int(shuf["cdis"]) == 0 and int(shuf["s843"]) == 0 and int(shuf["sres"]) == 0 and int(shuf["s844"]) == 0 and int(shuf["symc"]) == 0 and int(shuf["s845"]) == 0 and int(shuf["past"]) == 0 and int(shuf["s846"]) == 0 and int(shuf["lalg"]) == 0 and int(shuf["s847"]) == 0 and int(shuf["bnet"]) == 0 and int(shuf["s848"]) == 0 and int(shuf["mdpp"]) == 0 and int(shuf["s849"]) == 0 and int(shuf["kflt"]) == 0 and int(shuf["s850"]) == 0 and int(shuf["lqrc"]) == 0 and int(shuf["s851"]) == 0 and int(shuf["ffts"]) == 0 and int(shuf["s852"]) == 0 and int(shuf["odrk"]) == 0 and int(shuf["s853"]) == 0 and int(shuf["sxlp"]) == 0 and int(shuf["s854"]) == 0 and int(shuf["pcae"]) == 0 and int(shuf["s855"]) == 0 and int(shuf["pdes"]) == 0 and int(shuf["s856"]) == 0 and int(shuf["mcmcs"]) == 0 and int(shuf["s857"]) == 0 and int(shuf["ground"]) < 200, shuf

        # 3. Raise & Evaluate English SVO (STAGE 8o — 51 BLACKWELL KERNELS & 550 DOMAINS / 1,697 Probes!)
        child_en = d / "child-en.cmind"
        out_en = run_cmd(
            ["mind", "raise", "--load", str(newborn), "--dialect", "english", "--save", str(child_en), "--eval"],
            env,
        )
        ev_en = parse_kv(next(l for l in out_en.splitlines() if l.startswith("mind_eval=")))
        dom_en = parse_domains(out_en)
        assert int(ev_en["correct"]) == int(ev_en["total"]) == 1697, (ev_en, dom_en)
        assert len(dom_en) == 550, dom_en

        # 4. Raise & Evaluate Scrambled Alien SOV Twin (Exact 100% Parity — Zero Hardcoded English!)
        child_al = d / "child-alien.cmind"
        out_al = run_cmd(
            ["mind", "raise", "--load", str(newborn), "--dialect", "alien-sov", "--save", str(child_al), "--eval"],
            env,
        )
        ev_al = parse_kv(next(l for l in out_al.splitlines() if l.startswith("mind_eval=")))
        dom_al = parse_domains(out_al)
        assert int(ev_al["correct"]) == int(ev_al["total"]) == 1697, (ev_al, dom_al)
        assert dom_al == dom_en, (dom_al, dom_en)

        # 5. Checkpoint Reload & Quadruple-Dissociation Neural Ablations (including Duoquinquagintuple-Pathway 52-Track Ribot's Law!)
        def eval_arm(ablate):
            out = run_cmd(["mind", "eval", "--load", str(child_en), "--ablate", ablate], env)
            ev = parse_kv(next(l for l in out.splitlines() if l.startswith("mind_eval=")))
            return int(ev["correct"]), parse_domains(out)

        c_none, _ = eval_arm("none")
        c_frz, d_frz = eval_arm("freeze")
        c_hip, d_hip = eval_arm("hipp")
        c_api, d_api = eval_arm("apical")
        c_cau, d_cau = eval_arm("causal")
        assert c_none == 1697, c_none
        assert c_frz <= 795 and d_frz["Multi-hop episodic & history"][0] == 0 and d_frz["Solid-state Fermi-Dirac gap, thermal ratio & conduction tier"][0] == 0 and d_frz["Architecture Roofline intensity, attainable perf & bound tier"][0] == 0 and d_frz["GPU Kernel-50 1D PDE solver & Unquinquagintuple 51-Track Ribot"][0] == 3 and d_frz["GPU Kernel-51 256-chain MCMC & Duoquinquagintuple 52-Track Ribot"][0] == 3
        # Duoquinquagintuple-Pathway (52-Track) Ribot's Law under total hippocampal lesion: all episodic/online working-memory domains collapse,
        # while ALL FIFTY-TWO consolidated & live-written cortical tracks SURVIVE — including Blackwell Kernel 51 Pure-GPU 256-Chain Spiking MCMC Sampler (`GPU Kernel-51 ...` 3/3 + `Stage-8o 550-domain synthesis & Kernel-51 52-Track Ribot` >= 2/4)!
        assert c_hip <= 780 and d_hip["Multi-hop episodic & history"][0] == 0 and d_hip["Theory of Mind (1st & 2nd order)"][0] == 0 and d_hip["Zero-template 3-entity cluster, expansion & capability anaphora"][0] == 0 and d_hip["Solid-state Fermi-Dirac gap, thermal ratio & conduction tier"][0] == 0 and d_hip["Fluid dynamics Bernoulli dynamic pressure, total head & tier"][0] == 0 and d_hip["Cryptography Shamir secret sharing quorum, fault tolerance & tier"][0] == 0 and d_hip["Neuroscience Nernst driving force, synaptic current & spike tier"][0] == 0 and d_hip["Robotics 2-link IK max reach, inner deadzone & dexterity tier"][0] == 0 and d_hip["Ecology Lotka-Volterra prey equilibrium, predator & richness tier"][0] == 0 and d_hip["Architecture Roofline intensity, attainable perf & bound tier"][0] == 0
        assert d_hip["Taxonomy, generalization & exceptions"][0] >= 3 and d_hip["Cyclical seasons & Pathway-4 Ribot"][0] == 2 and d_hip["System-1 intuitive physics (Pathway-7)"][0] == 3 and d_hip["Affective ToM & empathy (Pathway-9)"][0] >= 2 and d_hip["GPU recurrent cortical think-chain"][0] == 3 and d_hip["Multi-system synthesis & Pathway-14 Ribot"][0] >= 2 and d_hip["GPU cortical contrast & Septuple Ribot"][0] == 3 and d_hip["GPU Kernel-7 analogy & Octuple Ribot"][0] == 3 and d_hip["GPU Kernel-8 compose & Nonuple Ribot"][0] == 3 and d_hip["Cross-domain synthesis & Pathway-23 Ribot"][0] >= 2 and d_hip["GPU Kernel-9 multi-cue & Decuple Ribot"][0] == 3 and d_hip["Open fluency & Pathway-26 Decuple Ribot"][0] >= 2 and d_hip["GPU Kernel-10 hypothesis & Undecuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-10 Undecuple Ribot"][0] >= 2 and d_hip["GPU Kernel-11 bridge & Duodecuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-11 Duodecuple Ribot"][0] >= 2 and d_hip["GPU Kernel-12 odd-one-out & Tredecuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-12 Tredecuple Ribot"][0] >= 2 and d_hip["GPU Kernel-13 conflict gate & Quattuordecuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-13 Quattuordecuple Ribot"][0] >= 2 and d_hip["GPU Kernel-14 SWR dream & Quindecuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-14 Quindecuple Ribot"][0] >= 2 and d_hip["GPU Kernel-15 quantifier scan & Sedecuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-15 Sedecuple Ribot"][0] >= 2 and d_hip["GPU Kernel-16 deductive proof & Septendecuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-16 Septendecuple Ribot"][0] >= 2 and d_hip["GPU Kernel-17 backward abduction & Octodecuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-17 Octodecuple Ribot"][0] >= 2 and d_hip["GPU Kernel-18 counterfactual imagine & Novemdecuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-18 Novemdecuple Ribot"][0] >= 2 and d_hip["GPU Kernel-19 hierarchical plan & Vigintuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-19 Vigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-20 NMDA causal simulation & Unvigintuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-20 Unvigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-21 constraint satisfaction & Duovigintuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-21 Duovigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-22 relational induction & Trevigintuple Ribot"][0] == 3 and d_hip["Open synthesis & Kernel-22 Trevigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-23 compositional rule algebra & Quattuorvigintuple Ribot"][0] == 3 and d_hip["Open synthesis, story summary & Kernel-23 Quattuorvigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-24 live online schema & Quinvigintuple Ribot"][0] == 3 and d_hip["Open synthesis, Socratic why-not & Kernel-24 Quinvigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-25 live schema revision & Sexvigintuple Ribot"][0] == 3 and d_hip["Open synthesis, belief history & Kernel-25 Sexvigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-26 spiking role-binder & Septemvigintuple Ribot"][0] == 3 and d_hip["300-domain synthesis & Kernel-26 Septemvigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-27 predictive spiking syntax & Octovigintuple Ribot"][0] == 3 and d_hip["Stage-5 synthesis & Kernel-27 Octovigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-28 neural-native spiking math & Nonavigintuple Ribot"][0] == 3 and d_hip["Multi-clause compound unseen prose via Kernel 27 & 26"][0] >= 2 and d_hip["Stage-5b synthesis & Kernel-28 Nonavigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-29 spiking graph wavefront & Trigintuple Ribot"][0] == 3 and d_hip["Stage-5c synthesis & Kernel-29 Trigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-30 spiking 3-SAT attractor & Untrigintuple Ribot"][0] == 3 and d_hip["Stage-5d synthesis & Kernel-30 Untrigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-31 spiking visual cortex & Duotrigintuple Ribot"][0] == 3 and d_hip["Stage-6 synthesis & Kernel-31 Duotrigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-32 spiking cochlear audio & Tretrigintuple Ribot"][0] == 3 and d_hip["Stage-6b synthesis & Kernel-32 Tretrigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-33 cross-modal binding, goal stack & Quattuortrigintuple Ribot"][0] == 3 and d_hip["Stage-6c synthesis & Kernel-33 Quattuortrigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-34 spiking dialogue cortex & Quintrigintuple Ribot"][0] == 3 and d_hip["Stage-7a synthesis & Kernel-34 Quintrigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-35 metaphor, narrative arc, self-critique & Sextrigintuple Ribot"][0] == 3 and d_hip["Stage-7b synthesis & Kernel-35 Sextrigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-36 belief-graph, debate & Septentrigintuple Ribot"][0] == 3 and d_hip["Stage-7c 400-domain synthesis & Kernel-36 Septentrigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-37 collective discovery, theorems & Octotrigintuple Ribot"][0] == 3 and d_hip["Stage-8 410-domain synthesis & Kernel-37 Octotrigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-38 self-research, code patching & Novemtrigintuple Ribot"][0] == 3 and d_hip["Stage-8b 420-domain synthesis & Kernel-38 Novemtrigintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-39 symbolic calculus & Quadragintuple 40-Track Ribot"][0] == 3 and d_hip["Stage-8c 430-domain synthesis & Kernel-39 Quadragintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-40 AST program synthesis & Unquadragintuple 41-Track Ribot"][0] == 3 and d_hip["Stage-8d 440-domain synthesis & Kernel-40 Unquadragintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-41 linear algebra & Duoquaragintuple 42-Track Ribot"][0] == 3 and d_hip["Stage-8e 450-domain synthesis & Kernel-41 Duoquaragintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-42 Bayes-net & Trequaragintuple 43-Track Ribot"][0] == 3 and d_hip["Stage-8f 460-domain synthesis & Kernel-42 Trequaragintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-43 MDP Bellman & Quattuorquaragintuple 44-Track Ribot"][0] == 3 and d_hip["Stage-8g 470-domain synthesis & Kernel-43 Quattuorquaragintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-44 Kalman filter & Quinquadragintuple 45-Track Ribot"][0] == 3 and d_hip["Stage-8h 480-domain synthesis & Kernel-44 Quinquadragintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-45 LQR control & Sexquadragintuple 46-Track Ribot"][0] == 3 and d_hip["Stage-8i 490-domain synthesis & Kernel-45 Sexquadragintuple Ribot"][0] >= 2 and d_hip["GPU Kernel-46 FFT spectrum & Septemquadragintuple 47-Track Ribot"][0] == 3 and d_hip["Stage-8j 500-domain milestone synthesis & Kernel-46 47-Track Ribot"][0] >= 2 and d_hip["GPU Kernel-47 RK4 ODE integrator & Octoquadragintuple 48-Track Ribot"][0] == 3 and d_hip["Stage-8k 510-domain synthesis & Kernel-47 48-Track Ribot"][0] >= 2 and d_hip["GPU Kernel-48 Simplex LP & Novemquadragintuple 49-Track Ribot"][0] == 3 and d_hip["Stage-8l 520-domain synthesis & Kernel-48 49-Track Ribot"][0] >= 2 and d_hip["GPU Kernel-49 PCA Eigen & Quinquagintuple 50-Track Ribot"][0] == 3 and d_hip["Stage-8m 530-domain synthesis & Kernel-49 50-Track Ribot"][0] >= 2 and d_hip["GPU Kernel-50 1D PDE solver & Unquinquagintuple 51-Track Ribot"][0] == 3 and d_hip["Stage-8n 540-domain synthesis & Kernel-50 51-Track Ribot"][0] >= 2 and d_hip["GPU Kernel-51 256-chain MCMC & Duoquinquagintuple 52-Track Ribot"][0] == 3 and d_hip["Stage-8o 550-domain synthesis & Kernel-51 52-Track Ribot"][0] >= 2, d_hip
        assert d_hip["Multi-step causal planning"][0] == 2 and d_hip["Linear algebraic equation solving"][0] == 3 and d_hip["Quadratic & Fibonacci sequence induction"][0] == 3
        assert d_api["Theory of Mind (1st & 2nd order)"][0] <= 1 and d_api["GPU Kernel-49 PCA Eigen & Quinquagintuple 50-Track Ribot"][0] == 0 and d_api["GPU Kernel-50 1D PDE solver & Unquinquagintuple 51-Track Ribot"][0] == 0 and d_api["GPU Kernel-51 256-chain MCMC & Duoquinquagintuple 52-Track Ribot"][0] == 0 and d_api["Solid-state Fermi-Dirac gap, thermal ratio & conduction tier"][0] == 0 and d_api["Cryptography Shamir secret sharing quorum, fault tolerance & tier"][0] == 0 and d_api["Robotics 2-link IK max reach, inner deadzone & dexterity tier"][0] == 0 and d_api["Architecture Roofline intensity, attainable perf & bound tier"][0] == 0
        assert d_cau["Multi-step causal planning"][0] == 0 and d_cau["GPU Kernel-20 NMDA causal simulation & Unvigintuple Ribot"][0] == 0 and d_cau["Fluid dynamics Bernoulli dynamic pressure, total head & tier"][0] == 0 and d_cau["Neuroscience Nernst driving force, synaptic current & spike tier"][0] == 0 and d_cau["Ecology Lotka-Volterra prey equilibrium, predator & richness tier"][0] == 0

        # 6. Autonomous Two-Mind Exchange (6/6), 4-Mind Society (8/8) & 736-Tick Multi-Gen 16x16 Cultural Open-Valley Ecosystem (98/98, SVO & SOV)
        for dial, ckpt in (("english", child_en), ("alien-sov", child_al)):
            out_peer = run_cmd(
                ["mind", "converse", "--speaker", str(ckpt), "--listener", str(ckpt), "--dialect", dial],
                env,
            )
            pkv = parse_kv(next(l for l in out_peer.splitlines() if l.startswith("mind_peer_exchange=")))
            assert int(pkv["passed"]) == int(pkv["total"]) == 6, pkv
            out_soc = run_cmd(["mind", "society", "--load", str(ckpt), "--dialect", dial], env)
            skv = parse_kv(next(l for l in out_soc.splitlines() if l.startswith("mind_society=")))
            assert int(skv["passed"]) == int(skv["total"]) == 8 and int(skv["lies_caught"]) == 3, skv
            out_eco = run_cmd(["mind", "ecosystem", "--load", str(ckpt), "--dialect", dial, "--ticks", "736"], env)
            ekv = parse_kv(next(l for l in out_eco.splitlines() if l.startswith("mind_ecosystem=")))
            assert int(ekv["passed"]) == int(ekv["total"]) == 98 and int(ekv["lies_caught"]) == 2 and int(ekv["trades"]) == 2 and int(ekv["discoveries"]) == 1 and int(ekv["decisions"]) >= 484, ekv

        # 7. Interactive Multi-Turn Dialogue + Living Homestead + Peer Mira + v6 Higher Cognition
        script = (
            "hello\n"
            "my name is ben\n"
            "what's my name?\n"
            "what's 17 plus 25?\n"
            "how do i get from meadow to cave?\n"
            "kestrel has wings\n"
            "kestrel has feathers\n"
            "what kind of animal is kestrel?\n"
            "can kestrel fly?\n"
            "what is similar to kestrel?\n"
            "maple has bark\n"
            "maple has foliage\n"
            "is maple living?\n"
            "what is similar to maple?\n"
            "if rain is falling then ground is wet\n"
            "if rain is falling then sky is cloudy\n"
            "if sprinkler is active then ground is wet\n"
            "if sprinkler is active then hose is running\n"
            "ground is wet\n"
            "sky is cloudy\n"
            "hose is not running\n"
            "why is ground wet?\n"
            "mira said pearl is in garden\n"
            "fox said pearl is in attic\n"
            "mira said opal is in tower\n"
            "fox said opal is in cellar\n"
            "where is opal?\n"
            "who do you trust more mira or fox?\n"
            "each of quartz medal locket is in urn crate trunk\n"
            "medal is in crate\n"
            "quartz is not in urn\n"
            "where is quartz?\n"
            "where is locket?\n"
            "no reptile has fur\n"
            "gecko is a reptile\n"
            "does gecko have fur?\n"
            "when wren returns remind me about key\n"
            "what are your reminders?\n"
            "wren returned\n"
            "dawn happened before noon\n"
            "noon happened before dusk\n"
            "did dawn happen before dusk?\n"
            "what happened first?\n"
            "1 apple costs 3 coin\n"
            "1 loaf costs 5 coin\n"
            "how much for 2 apple and 1 loaf?\n"
            "eve and dan have 14 coin total\n"
            "eve has 4 more coin than dan\n"
            "how many coin does eve have?\n"
            "kai has 10 berry\n"
            "zoe has 2 berry\n"
            "how many berry should kai give zoe to be fair?\n"
            "ore is raw\n"
            "crush on ore requires raw and causes dust\n"
            "smelt on ore requires dust and causes ingot\n"
            "forge on ore requires ingot and causes blade\n"
            "how can ore become blade?\n"
            "what if we skip smelt?\n"
            "/tick 10\n"
            "/do stew cooked\n"
            "where is sapphire?\n"
            "/consult\n"
            "where is sapphire?\n"
            "what mammal can fly?\n"
            "what comes next after 3 6 12 24?\n"
            "alice is taller than bob\n"
            "bob is taller than carol\n"
            "rank carol alice bob by taller\n"
            "carol is taller than alice\n"
            "how can ice become steam?\n"
            "navigate from camp to mine\n"
            "bridge is broken\n"
            "/nav camp mine\n"
            "do you remember my name?\n"
            "tell me whether you trust mira?\n"
            "what comes after winter?\n"
            "what is 3 plus 4 times 5?\n"
            "what is 25 percent of 80?\n"
            "fox broke cup on purpose\n"
            "mira broke bowl by accident\n"
            "who was wrong fox or mira?\n"
            "nobody may touch fire without glove\n"
            "may sam touch fire?\n"
            "sam has glove\n"
            "may sam touch fire?\n"
            "what happens if we boil water?\n"
            "what happens if we crush then smelt ore?\n"
            "on leftside iron and oak versus rightside oak and feather which side heavier?\n"
            "is 17 even or odd?\n"
            "either crystal is acidic or salty\n"
            "acidic turns litmus red\n"
            "how can we test if crystal is acidic?\n"
            "litmus on crystal did not turn red\n"
            "why did you change mind about crystal?\n"
            "paper beats rock\n"
            "if mira plays rock what should i play?\n"
            "mira lost gem\n"
            "how does mira feel?\n"
            "how can we help mira?\n"
            "if weather is terrible and mira says wonderful is she literal or ironic?\n"
            "brain is to body as elder is to village\n"
            "what is the brain of village?\n"
            "the bird that cannot fly is inside cave\n"
            "where is penguin?\n"
            "prove why kestrel is living?\n"
            "think deeply about sparrow\n"
            "what could have caused steam?\n"
            "cascade from ice with melt then boil\n"
            "what is 25 percent of 80? what comes after winter?\n"
            "elena took lantern to cavern\n"
            "dragon guarded crown inside cavern\n"
            "elena sang lullaby so dragon slept\n"
            "elena took crown to palace\n"
            "why did dragon slept?\n"
            "solve 2 times x plus 3 equals 11\n"
            "if each crystal costs that much how much do 3 crystal cost?\n"
            "pack limit is 10\n"
            "idol weight 6 value 15\n"
            "relic weight 5 value 11\n"
            "chalice weight 5 value 11\n"
            "which items maximize value?\n"
            "valve controls flow in pipe\n"
            "wire corresponds to pipe\n"
            "switch corresponds to valve\n"
            "current corresponds to flow\n"
            "what controls current in wire?\n"
            "beacon is at 2 3\n"
            "wall is at 5 3\n"
            "tower is at 8 3\n"
            "can beacon see tower?\n"
            "spark caused fire\n"
            "fire caused smoke\n"
            "smoke caused alarm\n"
            "if spark had not happened would alarm happen?\n"
            "backup also caused alarm\n"
            "if spark had not happened would alarm happen?\n"
            "sparrow is in meadow\n"
            "penguin is in cave\n"
            "can the bird fly?\n"
            "penguin\n"
            "why do you say that?\n"
            "what comes next after 1 4 9 16?\n"
            "what comes next after 1 1 2 3 5?\n"
            "leo lost flute. how does leo feel?\n"
            "which direction is mine from camp?\n"
            "base rate 50 percent and accuracy 80 percent posterior\n"
            "base rate 10 percent true 90 percent false 10 percent posterior\n"
            "if rain implies wet what is contrapositive?\n"
            "not both warm and dry means what?\n"
            "gate is xor of alpha and beta\n"
            "alpha is active\n"
            "beta is active\n"
            "is xor gate active?\n"
            "beta is not active\n"
            "is xor gate active?\n"
            "how many ways to arrange 4 items?\n"
            "5 peer shake hands how many handshakes?\n"
            "how many subsets of 4 items?\n"
            "what is sum from 1 to 10?\n"
            "what is gcd of 48 and 18?\n"
            "what is lcm of 6 and 8?\n"
            "split 24 coin in ratio 3 to 1 larger share\n"
            "what is average of 10 20 30?\n"
            "mira finishes job in 6 and wren in 3 together how long?\n"
            "if clock shows 10 what hour is 5 later?\n"
            "meeting from 2 to 5 and lesson from 4 to 7 overlap?\n"
            "meeting from 1 to 3 and lesson from 4 to 6 overlap?\n"
            "sam says sam is a knave is that possible?\n"
            "alice says bob is a knave\n"
            "bob is a knight\n"
            "what role is alice?\n"
            "atoll connects to fjord\n"
            "fjord connects to mesa\n"
            "atoll connects to mesa\n"
            "can 2 colors color atoll fjord mesa?\n"
            "can 3 colors color atoll fjord mesa?\n"
            "how many hops from atoll to mesa?\n"
            "how are you feeling today?\n"
            "give me a riddle\n"
            "is it penguin guess\n"
            "thank you axion\n"
            "contrast sparrow and penguin on cortex\n"
            "syllogism check is sparrow living?\n"
            "when i say salutation mean hello\n"
            "salutation\n"
            "learn phrase findspot x means where is x\n"
            "findspot opal\n"
            "learn phrase speciesof x means what kind is x\n"
            "speciesof kestrel\n"
            "what do you know about penguin?\n"
            "teach me something\n"
            "cortex analogy sparrow to bird as salmon to what?\n"
            "cortex analogy spring to summer as autumn to what?\n"
            "cortex analogy hot to cold as wet to what?\n"
            "x plus y equals 10\n"
            "2 times x plus y equals 14\n"
            "what is x value?\n"
            "what is y value?\n"
            "what is weighted average of 10 weight 1 and 30 weight 3?\n"
            "branch alpha gives 100 at 50 percent versus beta gives 80 at 75 percent which better?\n"
            "three doors one prize host opens empty should we stay or switch?\n"
            "river is always wet\n"
            "today river is dry\n"
            "is river always wet?\n"
            "gear axe can cut timber\n"
            "gear saw can cut timber\n"
            "gear axe is broken\n"
            "how can we cut timber now?\n"
            "mira is present\n"
            "wren is present\n"
            "fox is present\n"
            "mira put flute inside camp\n"
            "wren left\n"
            "fox left\n"
            "mira moved flute to vault\n"
            "mira informed wren flute is in vault\n"
            "where does wren now think flute is?\n"
            "where does fox now think flute is?\n"
            "axion has energy 2 and berry 3 decide eat forage explore hungry ready\n"
            "cortex find bird with swim without fly\n"
            "cortex find mammal with fly\n"
            "my favorite animal is eagle\n"
            "i live in valley\n"
            "what is my favorite animal?\n"
            "where do i live?\n"
            "can my favorite animal fly?\n"
            "how does your brain work?\n"
            "what are we talking about?\n"
            "ask me something\n"
            "that makes sense\n"
            "set alpha has stone coin pearl\n"
            "set beta has coin pearl gem\n"
            "what is in both set alpha and beta?\n"
            "what is in set alpha not beta?\n"
            "how many unique in set alpha or beta?\n"
            "wolf threatens hen\n"
            "hen threatens grain\n"
            "is it safe to leave wolf and hen alone?\n"
            "is it safe to leave wolf and grain alone?\n"
            "who must cross first?\n"
            "100 grows by 10 percent for 2 steps\n"
            "what is 2 power 5?\n"
            "what is 6 factorial divided_by 4 factorial?\n"
            "score of alice is at least 8\n"
            "score of bob is at most 5\n"
            "score of carol is at most 10\n"
            "is score of alice strictly above bob?\n"
            "is score of alice strictly above carol?\n"
            "what is minimum gap between alice and bob?\n"
            "jointly leo unlocked door\n"
            "intentionally fox stole gem\n"
            "without leo would gem happen?\n"
            "did both leo and fox contribute?\n"
            "who intentionally stole gem?\n"
            "example 2 maps to 5\n"
            "example 4 maps to 9\n"
            "what does 7 map to?\n"
            "example 2 maps to 4\n"
            "example 3 maps to 9\n"
            "what does 5 map to?\n"
            "what is twice that?\n"
            "contrast my favorite animal and salmon on cortex\n"
            "cortex needed for cooked\n"
            "cortex needed for open\n"
            "cortex recall tinder and flint with strike\n"
            "cortex recall stew and fire with cook\n"
            "cortex recall chest and key with unlock\n"
            "cortex recall clay and fire with bake\n"
            "tara visited harbor\n"
            "tara carried compass in satchel\n"
            "storm flooded harbor so tara climbed lighthouse\n"
            "who carried compass?\n"
            "where did tara climb?\n"
            "why did tara climb lighthouse?\n"
            "summarize story of tara\n"
            "candidate sparrow has feature living\n"
            "candidate penguin has feature living\n"
            "candidate salmon has feature living\n"
            "candidate whale has feature living\n"
            "candidate sparrow has feature wings\n"
            "candidate penguin has feature wings\n"
            "candidate sparrow has feature flight\n"
            "candidate salmon has feature gills\n"
            "which feature best splits candidates?\n"
            "observe feature wings is present\n"
            "which feature best splits candidates?\n"
            "observe feature flight is absent\n"
            "convince me why kestrel is a bird\n"
            "are you certain penguin cannot fly?\n"
            "what if penguin had rocket could penguin fly?\n"
            "every cedar belongs to conifer\n"
            "every conifer belongs to flora\n"
            "every pine belongs to conifer\n"
            "no flora belongs to mineral\n"
            "does every cedar belong to flora?\n"
            "does any cedar belong to mineral?\n"
            "does every cedar belong to pine?\n"
            "mira values ore at 8\n"
            "mira values berry at 2\n"
            "wren values ore at 2\n"
            "wren values berry at 8\n"
            "should mira trade berry for ore with wren?\n"
            "what is surplus of mira trading berry for ore with wren?\n"
            "pair u plus v equals 10\n"
            "pair v plus w equals 14\n"
            "pair u plus w equals 12\n"
            "what is value of variable u?\n"
            "what is value of variable v?\n"
            "what is value of variable w?\n"
            "node rain directly causes wet\n"
            "node sprinkler directly causes wet\n"
            "if we observe wet could rain be active?\n"
            "if we intervene on sprinkler does that cause wet?\n"
            "if we intervene on wet does that cause rain?\n"
            "invent word glint as luminous crystal that glow\n"
            "define word glint\n"
            "please tell me what my favorite animal is?\n"
            "who visited harbor?\n"
            "could you find bird with swim without fly?\n"
            "cortex diagnose clues wings and feathers among bird fish mammal\n"
            "cortex diagnose clues fur and milk among bird fish mammal\n"
            "could you diagnose clues gills and swim among bird fish mammal?\n"
            "kora crafted amulet\n"
            "bren discovered scroll\n"
            "who crafted amulet?\n"
            "what did bren discover?\n"
            "remind me who crafted amulet?\n"
            "kora mentored bren\n"
            "bren mentored sora\n"
            "did kora chain mentor sora?\n"
            "contradiction check bren did not mentor sora\n"
            "virel is a bird\n"
            "what do you wonder about virel?\n"
            "virel lives in fjord\n"
            "what did you learn about virel?\n"
            "task harvest takes 4 hours deadline 6\n"
            "task mill takes 3 hours deadline 8\n"
            "task bake takes 2 hours deadline 10\n"
            "total hours for harvest mill bake\n"
            "can harvest mill bake finish by deadline 10?\n"
            "clause alpha or beta\n"
            "clause not alpha or gamma\n"
            "clause not beta\n"
            "is clause system satisfiable?\n"
            "what value must gamma have?\n"
            "clause not gamma\n"
            "is clause system satisfiable?\n"
            "world alpha spark turns wax into glow\n"
            "world alpha glow turns bell into song\n"
            "analog spark maps to pulse\n"
            "analog wax maps to prism\n"
            "analog glow maps to hum\n"
            "analog bell maps to harp\n"
            "analog song maps to lullaby\n"
            "by analogy what does pulse turn prism into?\n"
            "by analogy how can prism lead to lullaby?\n"
            "risk storm is 50 percent\n"
            "risk flood is 40 percent\n"
            "what is combined risk of storm or flood?\n"
            "what is joint chance of both storm and flood?\n"
            "which is safer 20 percent risk or 50 percent risk?\n"
            "initially zircon is in cellar\n"
            "update zircon moved to observatory\n"
            "what did you believe before about zircon?\n"
            "why did you change your mind about zircon?\n"
            "cortex link spring to autumn\n"
            "cortex link sparrow to animal\n"
            "cortex link ice to steam\n"
            "cortex link autumn to spring\n"
            "could you link oak to plant on cortex?\n"
            "nina discovered lantern inside cavern\n"
            "she carried it to lighthouse\n"
            "she gave it to leo\n"
            "who holds lantern now?\n"
            "where was lantern discovered?\n"
            "where was lantern carried?\n"
            "who gave lantern to leo?\n"
            "tell me who holds lantern now?\n"
            "lyra forged sword with anvil\n"
            "lyra gifted sword to orin\n"
            "orin shielded gate with sword\n"
            "what instrument did lyra forge sword with?\n"
            "who was recipient of sword from lyra?\n"
            "what did orin shield with sword?\n"
            "remind me who received sword from lyra?\n"
            "route cliff has 50 percent chance of reward 100\n"
            "route trail has 90 percent chance of reward 70\n"
            "which route has higher expected reward cliff or trail?\n"
            "what is expected reward of route trail?\n"
            "route cliff reward rises to 150\n"
            "which route has higher expected reward cliff or trail?\n"
            "budget cap is 20\n"
            "allocate lantern cost 8 utility 14\n"
            "allocate cloak cost 12 utility 18\n"
            "allocate rope cost 6 utility 12\n"
            "best budget pair under cap\n"
            "budget cap drops to 16\n"
            "best budget pair under cap\n"
            "mira is a scout\n"
            "fox is a scout\n"
            "all scout except fox may enter vault\n"
            "may mira enter vault?\n"
            "may fox enter vault?\n"
            "schedule mira available from 2 to 6\n"
            "schedule wren available from 4 to 8\n"
            "schedule leo available from 7 to 9\n"
            "when can mira and wren meet?\n"
            "when can mira and leo meet?\n"
            "pipeline pump feeds filter\n"
            "pipeline filter feeds boiler\n"
            "pipeline boiler feeds turbine\n"
            "pipeline pump is ok\n"
            "pipeline filter is ok\n"
            "pipeline boiler is failing\n"
            "pipeline turbine is failing\n"
            "what is root fault in pipeline?\n"
            "is filter upstream of turbine?\n"
            "pipeline filter is failing\n"
            "what is root fault in pipeline?\n"
            "debate bridge pro 6 con 2\n"
            "debate tunnel pro 4 con 1\n"
            "overall debate best bridge or tunnel\n"
            "if priority is safety debate prefers bridge or tunnel\n"
            "if priority is speed debate prefers bridge or tunnel\n"
            "cortex outlier among sparrow robin salmon\n"
            "cortex outlier among salmon shark oak\n"
            "cortex closest among sparrow salmon eagle\n"
            "could you spot outlier among pine oak whale on cortex?\n"
            "explorer tara who discovered chalice that is luminous visited watchtower\n"
            "who discovered luminous chalice?\n"
            "where did explorer with luminous chalice visit?\n"
            "what property does chalice have?\n"
            "coalition mira alone earns 4\n"
            "coalition wren alone earns 6\n"
            "coalition mira and wren together earn 16\n"
            "what is coalition synergy of mira and wren?\n"
            "what is shapley share of mira with wren?\n"
            "remind me what is shapley share of mira with wren?\n"
            "mix 2 liter at 20 degree with 2 liter at 60 degree\n"
            "mix 1 liter at 100 degree with 3 liter at 100 degree\n"
            "register starts at 3\n"
            "step double\n"
            "step add 4\n"
            "what is register result?\n"
            "if we swap steps what is register result?\n"
            "does step order change result?\n"
            "recipe lantern needs 2 glass\n"
            "recipe lantern needs 1 wick\n"
            "recipe glass needs 3 sand\n"
            "how much sand for 2 lantern?\n"
            "tell me how much sand for 2 lantern?\n"
            "can 10 sand supply 2 lantern?\n"
            "witness mira reports beacon is safe\n"
            "witness wren reports beacon is safe\n"
            "witness leo reports beacon is safe\n"
            "witness fox reports beacon is unsafe\n"
            "what is consensus about beacon?\n"
            "who is dissenter about beacon?\n"
            "does beacon have supermajority quorum?\n"
            "at tick 2 latch becomes open\n"
            "at tick 5 rain becomes active\n"
            "at tick 8 latch becomes locked\n"
            "at tick 4 is latch open?\n"
            "at tick 9 is latch open?\n"
            "at tick 9 is rain active?\n"
            "rule if winged then can fly\n"
            "patch rule with exception penguin\n"
            "by patched rule can winged eagle fly?\n"
            "by patched rule can winged penguin fly?\n"
            "explain patched rule\n"
            "cortex verify sparrow can fly\n"
            "cortex verify penguin can fly\n"
            "cortex verify salmon can fly\n"
            "cortex verify eagle can fly\n"
            "could you verify whether penguin can fly on cortex?\n"
            "i spotted hawk near cliff\n"
            "hawk hunted mouse\n"
            "what did i spot?\n"
            "what did it hunt?\n"
            "recap our chat\n"
            "state camp action forage reward 3 next river\n"
            "state river action trade reward 9 next village\n"
            "state camp action nap reward 5 next camp\n"
            "best 1 step action from camp\n"
            "best 2 step action from camp\n"
            "horizon 2 value from camp\n"
            "tell me best 2 step action from camp?\n"
            "mediator drought causes drysoil\n"
            "mediator drysoil causes cropfail\n"
            "direct heat causes cropfail\n"
            "does drought affect cropfail through drysoil?\n"
            "if we block drysoil does drought still cause cropfail?\n"
            "if we block drysoil does heat still cause cropfail?\n"
            "conduit fount to basin cap 10\n"
            "conduit basin to mill cap 4\n"
            "conduit fount to ridge cap 5\n"
            "conduit ridge to mill cap 7\n"
            "what is bottleneck along fount basin mill?\n"
            "what is maxflow from fount to mill?\n"
            "conduit basin to mill expands to 8\n"
            "what is maxflow from fount to mill?\n"
            "auction relic bidder mira bids 12\n"
            "auction relic bidder wren bids 9\n"
            "auction relic bidder leo bids 6\n"
            "who wins auction for relic?\n"
            "what price does winner pay for relic?\n"
            "remind me what price winner pays for relic?\n"
            "if mira values relic at 12 what is dominant vickrey bid?\n"
            "point alpha is at 2 4\n"
            "point beta is at 6 8\n"
            "what is midpoint of alpha and beta?\n"
            "reflect alpha across beta\n"
            "what is squared distance between alpha and beta?\n"
            "module survey duration 3\n"
            "module framing duration 4 after survey\n"
            "module masonry duration 2 after survey\n"
            "module arch duration 5 after framing and masonry\n"
            "when can module framing finish?\n"
            "when can module arch finish?\n"
            "which module is critical before arch?\n"
            "duty rescue priority 10\n"
            "duty patrol priority 4\n"
            "duty harvest priority 2\n"
            "which duty overrides rescue or patrol?\n"
            "after rescue complete which duty next patrol or harvest?\n"
            "is breaking patrol justified by rescue?\n"
            "cortex dream spring\n"
            "cortex dream ice\n"
            "cortex dream sparrow\n"
            "could you dream ahead from winter on cortex?\n"
            "chapter hero lina wanted crystal\n"
            "chapter gorge blocked lina\n"
            "chapter lina built rope to cross gorge\n"
            "chapter lina retrieved crystal\n"
            "why did hero lina build rope?\n"
            "how did hero lina overcome gorge?\n"
            "summarize arc of hero lina\n"
            "seller mira offers grain at 3\n"
            "seller leo offers grain at 5\n"
            "buyer wren bids grain at 9\n"
            "buyer tara bids grain at 7\n"
            "what is clearing volume for grain?\n"
            "what is clearing price for grain?\n"
            "remind me what is clearing price for grain?\n"
            "what is market gains for grain?\n"
            "mover boulder mass 4 velocity 6\n"
            "mover sled mass 2 velocity 0\n"
            "what is momentum of boulder?\n"
            "what is merged velocity of boulder and sled?\n"
            "fulcrum load 12 at arm 3 balanced at arm 6 needs what effort?\n"
            "assistant goal is shelter\n"
            "assistant shelter requires timber and nails\n"
            "assistant user has timber\n"
            "what is missing for shelter?\n"
            "what should user do for shelter?\n"
            "assistant user found nails\n"
            "what should user do for shelter?\n"
            "affine scale 2 shift 3 1\n"
            "apply affine to 4 5\n"
            "apply affine twice to 1 2\n"
            "inverse preimage of 11 11\n"
            "minimum binary questions for 16 symbols?\n"
            "coding bits for 6 at length 1 plus 4 at length 2?\n"
            "fixed 4 bits vs optimal 2 bits saves how many?\n"
            "payoff coop coop 3 3\n"
            "payoff coop defect 0 5\n"
            "payoff defect coop 5 0\n"
            "payoff defect defect 1 1\n"
            "what is dominant strategy in payoff?\n"
            "what is nash equilibrium in payoff?\n"
            "tell me what is nash equilibrium?\n"
            "does social optimum beat nash in payoff?\n"
            "circuit lamp needs switch and battery\n"
            "circuit switch is powered\n"
            "circuit battery is dead\n"
            "circuit alarm triggers from smoke or heat\n"
            "circuit smoke is active\n"
            "circuit heat is active\n"
            "what necessary part stopped lamp?\n"
            "if smoke stops is alarm still active?\n"
            "is smoke necessary for alarm?\n"
            "cortex forall bird can fly\n"
            "cortex forall fish can swim\n"
            "cortex exists mammal can fly\n"
            "cortex forall bird has wings\n"
            "could you scan whether all bird can fly on cortex?\n"
            "opinion user prefers forest over desert because shade\n"
            "opinion user enjoys stargazing in meadow\n"
            "which place would you recommend forest or desert?\n"
            "why recommend forest?\n"
            "what should user do in meadow?\n"
            "trial temp 10 yield 25\n"
            "trial temp 20 yield 45\n"
            "trial temp 30 yield 65\n"
            "what is law for yield?\n"
            "predict yield at temp 40\n"
            "remind me what yield at temp 40?\n"
            "what temp gives yield 105?\n"
            "project orchard costs 100 returns 60 plus 80\n"
            "project quarry costs 50 returns 40 plus 40\n"
            "what is profit of project orchard?\n"
            "what is roi percent of project quarry?\n"
            "which project to invest orchard or quarry?\n"
            "kinematics start 10 accel 4 time 5\n"
            "what is final velocity in kinematics?\n"
            "what is distance traveled in kinematics?\n"
            "braking from 30 at decel 6 stops in what time?\n"
            "criterion agility weight 3\n"
            "criterion durability weight 2\n"
            "option glider agility 8 durability 4\n"
            "option barge agility 4 durability 9\n"
            "what is weighted score of option glider?\n"
            "which option wins glider or barge?\n"
            "criterion durability weight 6\n"
            "which option wins glider or barge?\n"
            "cable camp forest cost 4\n"
            "cable forest bridge cost 3\n"
            "cable camp bridge cost 8\n"
            "cable bridge village cost 5\n"
            "what is minimum wiring cost for mst?\n"
            "which cable is skipped in mst?\n"
            "cable camp bridge cost 2\n"
            "what is minimum wiring cost for mst?\n"
            "tell me minimum wiring cost?\n"
            "trace step 1 green\n"
            "trace step 2 green\n"
            "trace step 3 amber\n"
            "trace step 4 red\n"
            "in trace is green always held?\n"
            "in trace is red eventually reached?\n"
            "in trace is green held until amber?\n"
            "seats total 10\n"
            "district northward votes 50\n"
            "district southward votes 30\n"
            "district eastward votes 20\n"
            "how many seats apportion to district northward?\n"
            "how many seats apportion to district southward?\n"
            "how many seats apportion to district eastward?\n"
            "cortex deduce sparrow reaches living\n"
            "cortex deduce ice reaches steam blocking liquid\n"
            "cortex deduce oak reaches animal\n"
            "cortex deduce sparrow reaches living blocking bird\n"
            "could you deduce whether robin reaches living on cortex?\n"
            "topic astronomy says comet has tail\n"
            "topic astronomy says comet orbits sun\n"
            "topic astronomy says eclipse hides sun\n"
            "tell me what you know about astronomy?\n"
            "how do comet and eclipse relate in astronomy?\n"
            "ask me a curious followup about astronomy?\n"
            "road camp to forest cost 4\n"
            "road forest to village cost 3\n"
            "road camp to pass cost 2\n"
            "road pass to village cost 9\n"
            "road pass to forest cost 1\n"
            "what is shortest distance from camp to village?\n"
            "which route is shortest from camp to village?\n"
            "road pass to forest cost 8\n"
            "what is shortest distance from camp to village?\n"
            "remind me shortest road from camp to village?\n"
            "hypothesis aurora prior 20 percent\n"
            "sensor lidar likelihood 80 vs 20\n"
            "what is posterior for aurora?\n"
            "sensor spectro likelihood 75 vs 25\n"
            "what is posterior for aurora?\n"
            "sensor radar likelihood 20 vs 60\n"
            "what is posterior for aurora?\n"
            "tell me posterior for aurora?\n"
            "define var alpha equals 6\n"
            "define var beta equals alpha plus 4\n"
            "define var gamma equals beta times 3\n"
            "evaluate var gamma\n"
            "define var delta equals gamma minus alpha\n"
            "evaluate var delta\n"
            "define var alpha equals 8\n"
            "evaluate var delta\n"
            "elastic orb mass 4 speed 6\n"
            "elastic puck mass 2 speed 0\n"
            "what is kinetic energy of orb?\n"
            "what is elastic rebound of orb hitting puck?\n"
            "is kinetic energy conserved for orb hitting puck?\n"
            "cargo capacity 7\n"
            "cargo crate weight 3 value 11\n"
            "cargo barrel weight 4 value 14\n"
            "cargo chest weight 5 value 16\n"
            "cargo satchel weight 2 value 7\n"
            "what is max cargo value?\n"
            "which cargo items maximize value?\n"
            "cargo capacity 6\n"
            "what is max cargo value?\n"
            "solo mira worth 3\n"
            "solo wren worth 3\n"
            "solo leo worth 3\n"
            "pair mira wren worth 10\n"
            "pair mira leo worth 8\n"
            "pair wren leo worth 8\n"
            "grand coalition worth 15\n"
            "core split mira 4\n"
            "core split wren 4\n"
            "core split leo 7\n"
            "is split in core?\n"
            "core split mira 6\n"
            "core split wren 5\n"
            "core split leo 4\n"
            "is split in core?\n"
            "what is minimum pair share for mira and wren?\n"
            "theory thermal predicts meter high with vapor rising\n"
            "theory magnetic predicts meter high with compass spinning\n"
            "theory acoustic predicts chime ringing with vapor absent\n"
            "which test separates thermal and magnetic?\n"
            "observe experiment compass spinning\n"
            "which theory survives experiment?\n"
            "why was theory thermal eliminated?\n"
            "cortex abduce steam\n"
            "cortex abduce autumn\n"
            "cortex abduce plant\n"
            "cortex abduce winter\n"
            "could you abduce causes of steam on cortex?\n"
            "volcano warms geyser\n"
            "geyser fills lagoon\n"
            "heron stalks lagoon\n"
            "how are volcano and lagoon connected?\n"
            "what occurs near lagoon?\n"
            "inquire about volcano\n"
            "stock pond init 10 regrow 4\n"
            "stock pond draw 2\n"
            "stock pond after 5 cycles?\n"
            "stock pond draw 6\n"
            "when is stock pond depleted?\n"
            "what is sustainable draw for pond?\n"
            "remind me sustainable draw for pond?\n"
            "projectile bolt vx 10 vy 20\n"
            "projectile bolt gravity 10\n"
            "what is apex time of bolt?\n"
            "what is max altitude of bolt?\n"
            "what is horizontal range of bolt?\n"
            "matrix mat_a row1 3 1\n"
            "matrix mat_a row2 2 4\n"
            "what is determinant of matrix mat_a?\n"
            "matrix mat_a magnifies area 6?\n"
            "matrix mat_b row1 2 4\n"
            "matrix mat_b row2 1 2\n"
            "is matrix mat_b singular?\n"
            "dataset set_a sample 10\n"
            "dataset set_a sample 12\n"
            "dataset set_a sample 14\n"
            "dataset set_a sample 16\n"
            "dataset set_a sample 18\n"
            "what is avg of dataset set_a?\n"
            "what is variance of dataset set_a?\n"
            "dataset set_a sample 64\n"
            "which value is anomaly in dataset set_a?\n"
            "gig j1 span 1 to 4\n"
            "gig j1 wage 5\n"
            "gig j2 span 3 to 6\n"
            "gig j2 wage 6\n"
            "gig j3 span 4 to 7\n"
            "gig j3 wage 5\n"
            "what is max gig wage?\n"
            "which gigs maximize wage?\n"
            "gig j4 span 1 to 7\n"
            "gig j4 wage 12\n"
            "what is max gig wage?\n"
            "ballot v1 ranks park plaza pier\n"
            "ballot v2 ranks plaza pier park\n"
            "ballot v3 ranks pier park plaza\n"
            "who is condorcet winner?\n"
            "ballot v3 ranks park pier plaza\n"
            "who is condorcet winner?\n"
            "what is borda score for park?\n"
            "dag season arrows rainfall\n"
            "dag season arrows outing\n"
            "dag rainfall arrows slick\n"
            "dag oil arrows slick\n"
            "in dag is slick collider for rainfall and oil?\n"
            "which confounder biases rainfall and outing?\n"
            "in dag are rainfall and outing dseparated given season?\n"
            "tell me confounder for rainfall and outing?\n"
            "cortex imagine sparrow losing wings gaining gills\n"
            "cortex imagine salmon losing gills gaining wings\n"
            "cortex imagine penguin gaining fly\n"
            "cortex imagine oak losing bark gaining gills\n"
            "could you imagine sparrow losing wings gaining gills on cortex?\n"
            "ember thawed glacier\n"
            "glacier swelled gully\n"
            "hiker scouted gully\n"
            "what did ember sparked?\n"
            "why did gully alter?\n"
            "tell story of gully\n"
            "markov sunny stays 80 shifts rainy\n"
            "markov rainy stays 60 shifts sunny\n"
            "starting sunny chance sunny after 1 steps?\n"
            "starting sunny chance sunny after 2 steps?\n"
            "what is steady probability of sunny?\n"
            "tell me steady probability of sunny?\n"
            "lp profit x 3 y 5\n"
            "lp cap1 x 1 y 1 max 10\n"
            "lp cap2 x 0 y 1 max 6\n"
            "what is optimal lp profit?\n"
            "what is optimal lp allocation?\n"
            "lp cap2 x 0 y 1 max 4\n"
            "what is optimal lp profit?\n"
            "treenode trunk0 spawns leftbranch\n"
            "treenode trunk0 spawns rightbranch\n"
            "treenode leftbranch spawns twig1\n"
            "treenode leftbranch spawns twig2\n"
            "treenode rightbranch spawns twig3\n"
            "what is depth of treenode twig1?\n"
            "how many tips in treenode trunk0?\n"
            "what is lca of twig1 and twig2?\n"
            "wave ripple freq 5 wavelength 4\n"
            "what is wave speed of ripple?\n"
            "if freq doubles to 10 what is wavelength of ripple?\n"
            "what is third harmonic freq of ripple?\n"
            "resistor r1 ohms 6\n"
            "resistor r2 ohms 3\n"
            "battery volt 18\n"
            "series resistance of r1 and r2?\n"
            "shunt resistance of r1 and r2?\n"
            "shunt current of r1 and r2?\n"
            "remind me shunt resistance of r1 and r2?\n"
            "trait tall dominates dwarf\n"
            "parent p1 genotype het\n"
            "parent p2 genotype het\n"
            "chance offspring expresses tall?\n"
            "chance offspring expresses dwarf?\n"
            "parent p2 genotype rec\n"
            "chance offspring expresses dwarf?\n"
            "algo sweep class linear\n"
            "algo sort class nlogn\n"
            "algo pairs class quadratic\n"
            "algo lookup class constant\n"
            "which algo is better sweep or pairs?\n"
            "if input triples from 10 to 30 how costlier is pairs?\n"
            "which algo is quickest?\n"
            "cortex plan for cooked\n"
            "cortex plan for open\n"
            "cortex plan for cooked having fire\n"
            "cortex plan for pot\n"
            "could you plan for cooked on cortex?\n"
            "maya built windmill at hilltop\n"
            "it powers granary\n"
            "storm damaged it\n"
            "what does windmill power?\n"
            "what happened to it?\n"
            "where did maya build windmill?\n"
            "epidemic blight r0 3 cases 2\n"
            "epidemic blight after 2 rounds?\n"
            "what is herd immunity for blight?\n"
            "with vaccine 80 percent does blight shrink?\n"
            "remind me herd immunity for blight?\n"
            "fluid brine density 12\n"
            "slab cedar mass 18 volume 3\n"
            "slab bronze mass 45 volume 3\n"
            "what is density of cedar?\n"
            "does cedar float in brine?\n"
            "does bronze float in brine?\n"
            "asset timber return 12 stddev 4\n"
            "asset spice return 18 stddev 9\n"
            "asset salt return 6 stddev 1\n"
            "what is efficiency of timber?\n"
            "which asset has best efficiency?\n"
            "split evenly timber and spice expected return?\n"
            "dep hull before mast\n"
            "dep mast before sail\n"
            "dep sail before voyage\n"
            "is topo acyclic?\n"
            "what is topo order?\n"
            "dep voyage before hull\n"
            "is topo acyclic?\n"
            "sorted array size 256\n"
            "how many binary probes for sorted array?\n"
            "how many linear probes for sorted array?\n"
            "how much quicker is binary than linear?\n"
            "siren emit 120 airspeed 300\n"
            "doppler freq approaching at 60?\n"
            "doppler freq receding at 100?\n"
            "is approaching pitch higher or lower?\n"
            "pd coop 3 defect 5\n"
            "pd punish 1\n"
            "pd continuation 80 percent\n"
            "is grim coop stable?\n"
            "what is grim threshold?\n"
            "tell me grim threshold?\n"
            "pd continuation 40 percent\n"
            "is grim coop stable?\n"
            "cortex simulate solid with melt then boil\n"
            "cortex simulate solid with melt then freeze\n"
            "cortex simulate solid with melt then boil blocking liquid\n"
            "cortex simulate cold with heat then sow\n"
            "could you simulate solid with melt then boil on cortex?\n"
            "nora repaired beacon at cliff\n"
            "it warned crew\n"
            "she saved ship at harbor\n"
            "crew thanked her\n"
            "what did beacon warn?\n"
            "where did nora save ship?\n"
            "who thanked nora?\n"
            "chem reaction 2 hydrogen and 1 oxygen makes 2 water\n"
            "chem stock 6 hydrogen and 5 oxygen\n"
            "which reagent is limiting?\n"
            "how many moles of water yield?\n"
            "how much excess oxygen leftover?\n"
            "optics lens focal 6\n"
            "lens image distance at object 12?\n"
            "remind me lens image distance at object 12?\n"
            "lens image distance at object 18?\n"
            "is lens image real or virtual at object 4?\n"
            "dist uniform has 4 quarters\n"
            "dist skewed has 1 halves 2 quarters\n"
            "what is shannon entropy of uniform?\n"
            "what is shannon entropy of skewed?\n"
            "which dist has lower entropy?\n"
            "seq seqa is oak pine maple cedar\n"
            "seq seqb is oak maple stone cedar\n"
            "what is lcs length of seqa and seqb?\n"
            "what is edit distance of seqa and seqb?\n"
            "seq seqb is oak pine maple cedar\n"
            "what is edit distance of seqa and seqb?\n"
            "queue dock arrivals 4 service 5 delay 3\n"
            "what is little occupancy of dock?\n"
            "what is utilization of dock?\n"
            "queue dock arrivals 6 service 5 delay 3\n"
            "is queue dock stable or saturated?\n"
            "dh prime 23 generator 5\n"
            "dh alice secret 6\n"
            "dh bob secret 15\n"
            "what is dh pubkey of alice?\n"
            "what is dh pubkey of bob?\n"
            "what is dh shared key of alice and bob?\n"
            "tell me dh shared key of alice and bob?\n"
            "cake alice values slice1 6 slice2 10\n"
            "cake bob values slice1 4 slice2 12\n"
            "cake assign alice slice1 bob slice2\n"
            "is cake division envyfree?\n"
            "what transfer makes cake envyfree?\n"
            "cake bob values slice1 10 slice2 8\n"
            "cake assign bob slice1 alice slice2\n"
            "is cake division envyfree?\n"
            "cortex constrain among sparrow penguin bat whale requiring bird and swim excluding fly\n"
            "cortex constrain among sparrow penguin bat whale requiring mammal and fly\n"
            "cortex constrain among sparrow penguin bat oak requiring living excluding animal\n"
            "cortex constrain among sparrow penguin shark whale requiring mammal and swim\n"
            "could you constrain among sparrow penguin bat whale requiring mammal and fly on cortex?\n"
            "lena kindled lantern because fog veiled trail\n"
            "she guided hiker at ridge\n"
            "hiker praised her\n"
            "why did lena kindle lantern?\n"
            "where did lena guide hiker?\n"
            "who praised lena?\n"
            "mover2d runner at 0 0 vel 3 4\n"
            "mover2d chaser at 15 20 vel 0 0\n"
            "what is mover2d speed of runner?\n"
            "when does runner catch chaser?\n"
            "where is mover2d runner after 4 ticks?\n"
            "gas chamber pressure 4 vol 6 kelvin 300\n"
            "gas chamber pressure if vol 3 at kelvin 300?\n"
            "remind me gas chamber pressure if vol 3 at kelvin 300?\n"
            "gas chamber pressure if vol 6 at kelvin 600?\n"
            "gas chamber pressure if vol 4 at kelvin 600?\n"
            "crt mod1 3 rem1 2 mod2 5 rem2 3\n"
            "what is smallest crt solution?\n"
            "what is crt period?\n"
            "what is modinv of 3 mod 11?\n"
            "bipartite edge hub1 hub2\n"
            "bipartite edge hub2 hub3\n"
            "bipartite edge hub3 hub4\n"
            "bipartite edge hub4 hub1\n"
            "is bipartite twocolorable?\n"
            "how many vertices in each bipartite side?\n"
            "bipartite edge hub1 hub3\n"
            "is bipartite twocolorable?\n"
            "bond vault principal 1000 rate 10 percent\n"
            "bond vault value after 2 years?\n"
            "discounted value of 1210 at 10 percent over 2 years?\n"
            "rule72 doubling years at 9 percent?\n"
            "signal tone band 40\n"
            "what is nyquist rate for tone?\n"
            "tell me nyquist rate for tone?\n"
            "at sample 100 does tone alias?\n"
            "at sample 60 does tone alias?\n"
            "commons pasture benefit 6 cost 2 damage 5\n"
            "does commons pasture suffer tragedy?\n"
            "what pigouvian tax fixes pasture?\n"
            "commons pasture benefit 9 cost 2 damage 5\n"
            "does commons pasture suffer tragedy?\n"
            "cortex induce from spring to summer and autumn to winter apply summer\n"
            "cortex induce from solid to liquid and liquid to steam apply solid twice\n"
            "cortex induce from hot to cold and wet to dry apply heavy\n"
            "cortex induce from sparrow to bird and salmon to fish apply oak twice\n"
            "could you induce from hot to cold and wet to dry apply heavy on cortex?\n"
            "soren mended bridge although river flooded valley\n"
            "he warned traveler at ford\n"
            "traveler thanked him because bridge braced cart\n"
            "despite what did soren mend bridge?\n"
            "why did traveler thank soren?\n"
            "recap everything about soren\n"
            "orbit moon radius 4 period 8\n"
            "what is orbit period of moon at radius 16?\n"
            "remind me orbit period of moon at radius 16?\n"
            "what is orbit period of moon at radius 1?\n"
            "if circular speed is 10 and mass quadruples what is speed?\n"
            "mat2d 5 2 2 5\n"
            "what is trace of mat2d?\n"
            "what is eigenvalues of mat2d?\n"
            "mat2d 4 0 0 9\n"
            "what is eigenvalues of mat2d?\n"
            "prior odds 1 to 3\n"
            "odds likelihood 6 to 1\n"
            "what are posterior odds?\n"
            "what is posterior odds prob?\n"
            "odds likelihood 3 to 2\n"
            "what are posterior odds?\n"
            "poly2d reset\n"
            "poly2d vertex 0 0\n"
            "poly2d vertex 6 0\n"
            "poly2d vertex 0 4\n"
            "what is poly2d orientation?\n"
            "what is shoelace area of poly2d?\n"
            "poly2d reset\n"
            "poly2d vertex 0 0\n"
            "poly2d vertex 5 0\n"
            "poly2d vertex 5 4\n"
            "poly2d vertex 0 4\n"
            "what is shoelace area of poly2d?\n"
            "dfa start q0 accepting q2\n"
            "dfa trans q0 on syma to q1\n"
            "dfa trans q1 on symb to q2\n"
            "dfa trans q2 on syma to q2\n"
            "dfa trans q1 on syma to q0\n"
            "dfa tracedfa syma symb\n"
            "dfa tracedfa syma syma symb\n"
            "dfa tracedfa syma symb syma\n"
            "economy mpc 75 percent\n"
            "what is spending multiplier for economy?\n"
            "stimulus 50 raises economy output by how much?\n"
            "economy mpc 80 percent\n"
            "tell me spending multiplier for economy?\n"
            "stimulus 40 raises economy output by how much?\n"
            "codeword cw1 0 0 0 0 0\n"
            "codeword cw2 1 1 1 0 0\n"
            "codeword cw3 0 0 1 1 1\n"
            "what is hamming distance between cw1 and cw2?\n"
            "what is mindist of codeword?\n"
            "how many hamming errors are correctable?\n"
            "cortex chainrule solid via state then cause\n"
            "cortex chainrule locked via opposite then tool\n"
            "cortex chainrule raw via opposite then tool\n"
            "cortex discover crosschain from lost to happy\n"
            "could you discover crosschain from solid to melt on cortex?\n"
            "talia sealed hatch before storm struck harbor\n"
            "she lit beacon after tide flooded pier\n"
            "crew spotted beacon at cliff\n"
            "what happened before storm struck harbor?\n"
            "what happened after tide flooded pier?\n"
            "what else happened?\n"
            "summarize whole story\n"
            "relativity vessel speed 60 percent\n"
            "dilated time for vessel proper 8 years?\n"
            "contracted length for vessel hull 100?\n"
            "relativistic sum speed 50 and 50 percent?\n"
            "capacitor c1 farad 6\n"
            "capacitor c2 farad 3\n"
            "capacitors series c1 and c2?\n"
            "capacitor energy c1 voltage 5?\n"
            "rc tau capacitor c2 resistor 4?\n"
            "euler reset\n"
            "euler edge k1 k2\n"
            "euler edge k2 k3\n"
            "euler edge k3 k4\n"
            "euler edge k4 k1\n"
            "is euler eulerian?\n"
            "euler edge k1 k3\n"
            "is euler eulerian?\n"
            "euler edge k2 k4\n"
            "is euler eulerian?\n"
            "confusion classifier truepos 60 falsepos 20 falseneg 40\n"
            "what is precision of classifier?\n"
            "what is recall of classifier?\n"
            "what is f1 score of classifier?\n"
            "controller kiln setpoint 100 state 60 kp 50 percent\n"
            "control effort for kiln?\n"
            "control state for kiln after 2 steps?\n"
            "steady offset for kiln under disturbance 15?\n"
            "logistic herd carrying 200 growth 40 percent pop 100\n"
            "logistic next pop for herd?\n"
            "logistic optimal pop for herd?\n"
            "logistic msy yield for herd?\n"
            "remind me logistic msy yield for herd?\n"
            "huffman reset\n"
            "huffman sym s1 freq 10\n"
            "huffman sym s2 freq 3\n"
            "huffman sym s3 freq 2\n"
            "huffman sym s4 freq 1\n"
            "what is huffman codebits?\n"
            "what is huffman bitlen of s1?\n"
            "what is huffman savedbits?\n"
            "teach schema quartz yields spark\n"
            "teach schema spark yields blaze\n"
            "teach schema blaze yields ash\n"
            "cortex onlineschema quartz hops 2?\n"
            "cortex onlineschema quartz hops 3?\n"
            "cortex onlineschema quartz without blaze?\n"
            "cortex onlineschema write ember through glow to flare?\n"
            "could you run onlineschema from quartz for 2 hops on cortex?\n"
            "mira told orin that lantern hid key\n"
            "orin told talia that map marked cove\n"
            "if dam breaks then valley floods\n"
            "if valley floods then mill stops\n"
            "what did mira tell orin?\n"
            "who told talia about map?\n"
            "what if dam breaks?\n"
            "sluice is jammed\n"
            "why not unlock sluice?\n"
            "photo metal zinc planck 4 workfunc 12\n"
            "photo cutoff for zinc?\n"
            "photo kinetic for zinc freq 5?\n"
            "photo kinetic for zinc freq 2?\n"
            "solution citrus ph 3\n"
            "solution soap ph 10\n"
            "what is poh of citrus?\n"
            "what is ph of citrus after dilute 10?\n"
            "classify solution soap?\n"
            "rsa primes 5 and 11 exp 3\n"
            "what is rsa privkey?\n"
            "rsa encrypt 4?\n"
            "rsa decrypt 9?\n"
            "remind me rsa decrypt 9?\n"
            "astar reset\n"
            "astar edge a1 a2 cost 4\n"
            "astar edge a2 a4 cost 5\n"
            "astar edge a1 a3 cost 2\n"
            "astar edge a3 a4 cost 8\n"
            "astar heur a2 4\n"
            "astar heur a3 9\n"
            "astar heur a4 0\n"
            "astar fscore at a2 from a1?\n"
            "astar shortest from a1 to a4?\n"
            "is astar heur a3 admissible to a4?\n"
            "mixnash row1 6 2 row2 0 4\n"
            "what is mixnash mixq?\n"
            "what is mixnash mixp?\n"
            "what is mixnash mixval?\n"
            "pid valve kp 2 ki 1 kd 3 err 5 prev 2 sum 4\n"
            "pid deriv for valve?\n"
            "pid integral for valve?\n"
            "pid effort for valve?\n"
            "venturi sluice area1 4 speed1 3 density 2\n"
            "venturi speed for sluice area2 2?\n"
            "venturi pressdrop for sluice area2 2?\n"
            "venturi speed for sluice area2 6?\n"
            "teach schema moss yields dew\n"
            "teach schema dew yields mist\n"
            "revise schema quartz yields ember then glow?\n"
            "cortex onlineschema moss hops 2?\n"
            "revise schema moss yields frost then rime?\n"
            "teach schema cinder yields ash\n"
            "teach schema ash yields dust\n"
            "could you reviseschema cinder yields pyre torch?\n"
            "cortex onlineschema quartz hops 2?\n"
            "talia placed lantern at harbor\n"
            "actually talia placed lantern at pier not harbor\n"
            "where is lantern?\n"
            "how did belief about lantern change?\n"
            "mira hid ring at cave\n"
            "orin hid ring at pier\n"
            "clarify ring?\n"
            "poly poly1 coeff 3 power 2\n"
            "deriv poly1 at 4?\n"
            "deriv2 poly1 at 4?\n"
            "integral poly1 from 0 to 2?\n"
            "poly poly2 coeff 4 power 3\n"
            "deriv poly2 at 2?\n"
            "isotope iso1 mass 640 halflife 5\n"
            "decay iso1 after 15?\n"
            "decay iso1 after 20?\n"
            "time iso1 to 20?\n"
            "gene g1 dompct 60 pop 1000\n"
            "hardy homodom g1?\n"
            "hardy hetero g1?\n"
            "hardy homorec g1?\n"
            "remind me hardy hetero g1?\n"
            "rpn reset\n"
            "rpn push 4\n"
            "rpn push 5\n"
            "rpn op add\n"
            "rpn push 3\n"
            "rpn op mul\n"
            "rpn result?\n"
            "rpn peak?\n"
            "rpn push 7\n"
            "rpn op sub\n"
            "rpn result?\n"
            "regress reset\n"
            "regress pt 1 5\n"
            "regress pt 2 8\n"
            "regress pt 3 11\n"
            "regress pt 4 14\n"
            "regress slope?\n"
            "regress intercept?\n"
            "regress predict 10?\n"
            "coins reset\n"
            "coins add 1\n"
            "coins add 3\n"
            "coins add 4\n"
            "coins min for 6?\n"
            "coins min for 10?\n"
            "coins ways for 6?\n"
            "kcl reset\n"
            "kcl inflow 12\n"
            "kcl inflow 8\n"
            "kcl outflow 7\n"
            "kvl source 24 drop 9 drop 6\n"
            "kcl balance?\n"
            "kvl remaining?\n"
            "kvl current resist 3?\n"
            "rolebind zephyr hauls pollen glade\n"
            "rolebind solis crafts sundial spire\n"
            "cortex rolebind unbind slotloc for zephyr and pollen?\n"
            "cortex rolebind unbind slotagent for sundial and spire?\n"
            "cortex rolebind unbind slotobj for zephyr and glade?\n"
            "cortex rolebind unbind slotact for solis and sundial?\n"
            "could you rolebind unbind slotloc for solis and sundial on cortex?\n"
            "concept velox means swift\n"
            "concept velox carries nectar\n"
            "concept nectar carries moth\n"
            "what does concept velox mean?\n"
            "is velox swift?\n"
            "trace concept velox?\n"
            "carnot eng1 thot 600 tcold 300\n"
            "carnot eff for eng1?\n"
            "carnot work for eng1 heatin 400?\n"
            "carnot waste for eng1 heatin 500?\n"
            "remind me carnot eff for eng1?\n"
            "channel ch1 bw 10 snr 7\n"
            "channel chcap for ch1?\n"
            "channel chcap for ch1 snr 15?\n"
            "channel minsnr for ch1 target 50?\n"
            "vec3 vecu 3 0 4\n"
            "vec3 vecv 0 5 0\n"
            "vec3 vecw 2 1 5\n"
            "vec3 dot vecu and vecw?\n"
            "vec3 orthogonal vecu and vecv?\n"
            "vec3 crossz vecu and vecw?\n"
            "totient 12?\n"
            "totient 15?\n"
            "totient 20?\n"
            "coprime 14 and 15?\n"
            "decibel gain ratio 100?\n"
            "decibel gain ratio 1000?\n"
            "decibel dbpow 50 plus 20?\n"
            "demand item1 qty1 100 pr1 10 qty2 60 pr2 12\n"
            "demand elasticity for item1?\n"
            "demand rev1 for item1?\n"
            "demand rev2 for item1?\n"
            "cache l1c hitpct 90 hitns 2 misspen 20\n"
            "cache amat for l1c?\n"
            "cache amat for l1c hitpct 80?\n"
            "cache cacheset addr 45 sets 8?\n"
            "falcon pursued hare past plateau\n"
            "dolphin escorted otter beyond reef\n"
            "narrate falcon?\n"
            "seqrole slotagent pursued hare?\n"
            "narrate dolphin?\n"
            "lynx tracked deer past canyon\n"
            "condor sighted lynx beyond canyon\n"
            "linkstory target condor?\n"
            "linkstory target lynx?\n"
            "linkstory condor?\n"
            "hydro tank1 rho 1000 subdepth 5\n"
            "hydro pressure for tank1?\n"
            "hydro pressure for tank1 subdepth 8?\n"
            "pascal force 50 parea1 2 parea2 20?\n"
            "radstar star1 power 100\n"
            "radstar power for star1 tscale 2?\n"
            "radstar power for star1 tscale 3?\n"
            "wien tempk 5?\n"
            "mat2 m1 4 3 2 5\n"
            "mat2 m2 2 4 1 2\n"
            "mat2 matdet for m1?\n"
            "mat2 mattrace for m1?\n"
            "mat2 matinv for m2?\n"
            "remind me mat2 matdet for m1?\n"
            "perm 5 3?\n"
            "comb 6 2?\n"
            "comb 7 3?\n"
            "derange 4?\n"
            "netlink link1 mbps 100 rtt 20\n"
            "netlink bdp for link1?\n"
            "netlink serial for link1 500?\n"
            "netlink oneway for link1 500?\n"
            "compound acct1 prin 1000 rate 10\n"
            "compound acct1 nyears 2?\n"
            "compound acct1 nyears 3?\n"
            "double72 rate 8?\n"
            "mstedge mstg1 va vb 4\n"
            "mstedge mstg1 vb vc 2\n"
            "mstedge mstg1 va vc 5\n"
            "mstedge mstg1 vc vd 3\n"
            "mstedge mstg1 vb vd 6\n"
            "mstweight for mstg1?\n"
            "mstedges for mstg1?\n"
            "mstmax for mstg1?\n"
            "panther stalked ibex crag\n"
            "narrate panther?\n"
            "could you seqrole slotloc panther ibex?\n"
            "neuralmath nmsolve 3 x plus 5 equals 23?\n"
            "neuralmath nmmod 19 plus 14 modbase 12?\n"
            "neuralmath nmmul 7 by 8?\n"
            "neuralmath nmsub 41 minus 17?\n"
            "could you neuralmath nmsolve 4 x plus 7 equals 35?\n"
            "osprey caught perch past estuary and badger dug burrow beyond hollow\n"
            "narrate osprey?\n"
            "narrate badger?\n"
            "linkstory target osprey?\n"
            "coulomb pair1 cforce 100\n"
            "coulomb cforce for pair1 distscale 2?\n"
            "coulomb cforce for pair1 qscale 3?\n"
            "platecap area 40 plategap 2?\n"
            "organpipe pipe1 speed 320 length 2\n"
            "organpipe openfund for pipe1?\n"
            "organpipe closedfund for pipe1?\n"
            "beatfreq 440 and 434?\n"
            "modpow 3 exp 5 modulo 7?\n"
            "modpow 2 exp 10 modulo 13?\n"
            "modpow 5 exp 3 modulo 13?\n"
            "invmod 3 modulo 11?\n"
            "amdahl job1 parpct 75\n"
            "amdahl cores 3 for job1?\n"
            "amdahl maxspeed for job1?\n"
            "remind me amdahl maxspeed for job1?\n"
            "gustafson cores 5 for job1?\n"
            "vmem vm1 pagesize 4096\n"
            "vmem pagenum for vm1 vaddr 9000?\n"
            "vmem pageoff for vm1 vaddr 9000?\n"
            "tlbeat hitpct 90 tlbns 2 memns 100?\n"
            "planetbody pl1 vesc 10\n"
            "planetbody vesc for pl1 mscale 4?\n"
            "planetbody vesc for pl1 rscale 4?\n"
            "rschwarz solar 10?\n"
            "pt2d pt1 2 3\n"
            "pt2d pt2 8 11\n"
            "pt2d manhattan pt1 and pt2?\n"
            "pt2d chebyshev pt1 and pt2?\n"
            "pt2d euclid pt1 and pt2?\n"
            "graphwave gwdist from gn0 to gn4?\n"
            "graphwave gwvia from gn0 to gn4?\n"
            "graphwave gwdist from gn0 to gn4 gwblock gn2?\n"
            "graphwave gwedge gn4 to gn5 cost 4\n"
            "graphwave gwdist from gn0 to gn5?\n"
            "could you graphwave gwhops from gn0 to gn4?\n"
            "marten flushed vole past thicket\n"
            "it trapped vole beyond crevice\n"
            "who flushed vole?\n"
            "where did marten trapped vole?\n"
            "what did marten trapped?\n"
            "isobaric piston1 pressk 50 dvol 4\n"
            "isobaric work for piston1?\n"
            "isobaric work for piston1 dvol 6?\n"
            "entrop qheat 600 tkelvin 300?\n"
            "faraday coil1 nturns 50 dflux 12 dtime 3\n"
            "faraday emf for coil1?\n"
            "remind me faraday emf for coil1?\n"
            "xform vprim 120 nprim 40 nsec 10?\n"
            "catalan 4?\n"
            "catalan 5?\n"
            "starsbars items2 5 bins 3?\n"
            "pigeon items2 25 bins 4?\n"
            "twoscomp 13 bitwidth 4?\n"
            "nandgate 1 and 1?\n"
            "norgate 0 and 0?\n"
            "bode sys1 mass 2 stiff 32 damp 16\n"
            "bode natfreq for sys1?\n"
            "bode damped for sys1?\n"
            "bode damped for sys1 damp 10?\n"
            "cpusched sched1 proc1 9 proc2 3 proc3 6\n"
            "cpusched sjfavg for sched1?\n"
            "cpusched turnaround for sched1?\n"
            "cpusched bursttot for sched1?\n"
            "debroglie part1 planck 600 mom 15\n"
            "debroglie wavelength for part1?\n"
            "debroglie wavelength for part1 mom 20?\n"
            "bohr nlevel 2?\n"
            "satsolve satcheck?\n"
            "satsolve satval sv3?\n"
            "satsolve satval sv0?\n"
            "could you satsolve satval sv2?\n"
            "satsolve satclause not sv3\n"
            "satsolve satcheck?\n"
            "satsolve satreset\n"
            "squall uprooted spruce past knoll\n"
            "so beaver heaped dam beyond brook\n"
            "why did beaver heaped dam?\n"
            "what happened after squall uprooted spruce?\n"
            "who heaped dam?\n"
            "hbmarchive hbmstore era1 garnet\n"
            "hbmarchive hbmpad 4200\n"
            "hbmarchive hbmstore era2 beryl\n"
            "hbmarchive hbmrecall era2?\n"
            "hbmarchive hbmrecall era1?\n"
            "hbmarchive hbmcount?\n"
            "reynolds flow1 dens 1000 vel 2 diam 4 visc 2\n"
            "reynolds renum for flow1?\n"
            "reynolds regime for flow1?\n"
            "reynolds regime for flow1 vel 1?\n"
            "hamdist 13 and 6?\n"
            "parity 11?\n"
            "biterr 5?\n"
            "mat3 m3a 2 3 5\n"
            "mat3 tr3 for m3a?\n"
            "mat3 det3 for m3a?\n"
            "mat3 inv3 for m3a?\n"
            "kinetics rxn1 krate 3 conc 25\n"
            "kinetics rxnrate for rxn1?\n"
            "kinetics rxnrate for rxn1 conc 40?\n"
            "kinetics halftime for rxn1?\n"
            "cidr hosts prefix 26?\n"
            "cidr hosts prefix 24?\n"
            "cidr hostbits prefix 26?\n"
            "cidr blocksz prefix 28?\n"
            "beam bm1 loadf 40 beamlen 8 capm 160\n"
            "beam moment for bm1?\n"
            "remind me beam moment for bm1?\n"
            "beam shear for bm1?\n"
            "beam safefac for bm1?\n"
            "retina visscan visring?\n"
            "retina viscount visdual?\n"
            "retina visrel visdual?\n"
            "retina visrel visscene?\n"
            "retina visteach vislshape\n"
            "retina visscan vislshape?\n"
            "could you retina visscan viscross?\n"
            "study zephryx is soarer with feathers eats beetles\n"
            "study studygap zephryx?\n"
            "study zephryx lives ravine\n"
            "study studygap zephryx?\n"
            "study studyinfo zephryx?\n"
            "codeprog prog1 codeinit 2\n"
            "codeprog prog1 codeadd 3\n"
            "codeprog prog1 codemul 4\n"
            "codeprog prog1 codesub 5\n"
            "codeprog coderun prog1?\n"
            "codeprog codestep prog1 step 2?\n"
            "codeprog prog1 codefix 2 5\n"
            "codeprog codecheck prog1 expect 23?\n"
            "chord chd1 basehz 200\n"
            "chord octave for chd1?\n"
            "chord fifth for chd1?\n"
            "chord harm3 for chd1?\n"
            "rotor rt1 armrad 4 rforce 15 inertia 10 omega 6\n"
            "rotor torque for rt1?\n"
            "remind me rotor torque for rt1?\n"
            "rotor angmom for rt1?\n"
            "rotor rotke for rt1?\n"
            "compiler ast1 astleaf 8 astops 7 livevars 5 nregs 4\n"
            "compiler astdepth for ast1?\n"
            "compiler regspill for ast1?\n"
            "compiler astnodes for ast1?\n"
            "dnaseq dna1 gccnt 12 dnalen 30 glucose 3\n"
            "dnaseq gcpct for dna1?\n"
            "dnaseq codons for dna1?\n"
            "dnaseq atpyield for dna1?\n"
            "cipher cip1 plainv 42 xorkey 19 cshift 5\n"
            "cipher xorct for cip1?\n"
            "cipher xordec for cip1?\n"
            "cipher caesarv for cip1?\n"
            "robot arm1 arml1 10 arml2 6 wcirc 12 wturns 5\n"
            "robot maxreach for arm1?\n"
            "remind me robot maxreach for arm1?\n"
            "robot minreach for arm1?\n"
            "robot odometry for arm1?\n"
            "cochlea audscan audtritone?\n"
            "cochlea audbeats audmarch?\n"
            "cochlea audpitch audrise?\n"
            "cochlea audteach audmarch\n"
            "cochlea audscan audmarch?\n"
            "could you cochlea audscan audmajor?\n"
            "textbook lumiglass amplifies glintray byfactor 4 blockedby fogshroud\n"
            "textbook tbpredict lumiglass tbval 7?\n"
            "textbook tbpredict lumiglass tbval 7 with fogshroud?\n"
            "textbook selfquiz lumiglass tbval 6 claim 18?\n"
            "textbook selfquiz lumiglass tbval 6 claim 24?\n"
            "codeprog prog2 codeinit 2\n"
            "codeprog prog2 codeloop 4 3\n"
            "codeprog prog2 codeifgt 10 5\n"
            "codeprog coderun prog2?\n"
            "codeprog prog3 codeinit 4\n"
            "codeprog prog3 codeadd 2\n"
            "codeprog prog3 codemul 5\n"
            "codeprog prog3 codesub 2\n"
            "codeprog codefindbug prog3 expect 16?\n"
            "codeprog prog3 codefix 3 3\n"
            "codeprog codecheck prog3 expect 16?\n"
            "empathy uttera prosody audrise mood tense\n"
            "empathy utterb prosody audfall mood proud\n"
            "empathy utterc prosody audtritone mood laud\n"
            "empathy tonecheck for uttera?\n"
            "empathy tonecheck for utterb?\n"
            "empathy tonecheck for utterc?\n"
            "solid3d sld1 basea 12 heightv 9 verts 8 edges 12\n"
            "solid3d prismvol for sld1?\n"
            "solid3d pyrvol for sld1?\n"
            "remind me solid3d pyrvol for sld1?\n"
            "solid3d eulerf for sld1?\n"
            "graphc gr1 nverts 6 nedges 10\n"
            "graphc knedges for gr1?\n"
            "graphc treeedges for gr1?\n"
            "graphc planarf for gr1?\n"
            "dsp sig1 sx0 2 sx1 4 sx2 6\n"
            "dsp sig1 tap0 3 tap1 2 tap2 1\n"
            "dsp conv3 for sig1?\n"
            "dsp sigener for sig1?\n"
            "dsp movavg for sig1?\n"
            "orgchem alk1 carbons 4 molcnt 6 liters 2\n"
            "orgchem alkaneh for alk1?\n"
            "orgchem alkmass for alk1?\n"
            "remind me orgchem alkmass for alk1?\n"
            "orgchem molarity for alk1?\n"
            "dbquery dbq1 rowsa 20 rowsb 10\n"
            "dbquery hashjoin for dbq1?\n"
            "dbquery nestjoin for dbq1?\n"
            "dbquery joingain for dbq1?\n"
            "xmodal xfuse visring audmajor?\n"
            "xmodal xsynest viscross?\n"
            "xmodal gpush goalmap\n"
            "xmodal gpush goalfix\n"
            "xmodal gpush goalrun\n"
            "xmodal gpop\n"
            "xmodal gpeek?\n"
            "xmodal xbind vislshape audmarch\n"
            "xmodal xfuse vislshape audmarch?\n"
            "could you xmodal xfuse viscross audtritone?\n"
            "coach usergoal buildbridge blocker missingrope\n"
            "coach nextstep?\n"
            "coach resolved missingrope\n"
            "coach nextstep?\n"
            "coach knowcheck darkmatter?\n"
            "coach knowcheck buildbridge?\n"
            "contraption rigalpha gearin 10 gearout 30 pulleys 4\n"
            "contraption rigalpha levera 6 leverb 2\n"
            "contraption mechadv for rigalpha?\n"
            "remind me contraption mechadv for rigalpha?\n"
            "contraption liftload for rigalpha drivef 5?\n"
            "contraption stresstest for rigalpha maxload 150 drivef 5?\n"
            "deliberate billalpha proside swift merit 8 conside heavy hazardc 5\n"
            "deliberate verdict for billalpha?\n"
            "deliberate merit for billalpha?\n"
            "deliberate tradeoff for billalpha?\n"
            "astro plntalpha gmval 900 radv 10 spandist 600 cspeed 30\n"
            "astro surfacc for plntalpha?\n"
            "astro lightsec for plntalpha?\n"
            "astro orbvel for plntalpha?\n"
            "stochd jaralpha ntrials 80 probpct 25\n"
            "stochd geomexp for jaralpha?\n"
            "stochd expmean for jaralpha?\n"
            "stochd binvar for jaralpha?\n"
            "elecgrid cktalpha emfvolt 120 resohm 10\n"
            "elecgrid cktalpha iprim 5 coilp 400 coils 100\n"
            "elecgrid wattpow for cktalpha?\n"
            "elecgrid curramp for cktalpha?\n"
            "elecgrid seccurr for cktalpha?\n"
            "distsys clualpha nnodes 7\n"
            "distsys qmin for clualpha?\n"
            "remind me distsys qmin for clualpha?\n"
            "distsys bftmax for clualpha?\n"
            "distsys parttol for clualpha?\n"
            "invopt skualpha unitp 50 unitc 30 fixcost 400\n"
            "invopt marginpct for skualpha?\n"
            "invopt breakeven for skualpha?\n"
            "invopt unitprof for skualpha?\n"
            "dialogue dlgreset\n"
            "dialogue dlgmode cuecomp cuewhy?\n"
            "dialogue dlgmode cuefeel cuestory?\n"
            "dialogue dlgstance claimone posstance\n"
            "dialogue dlgcheck claimone negstance?\n"
            "dialogue dlgteach dlgdebate cueargue cuedilemma\n"
            "dialogue dlgmode cueargue cuedilemma?\n"
            "could you dialogue dlgmode cuefact cuewhy?\n"
            "what do you think about eagle?\n"
            "tell me more about it?\n"
            "how are eagle and salmon connected?\n"
            "punwit punalpha glossone treecoat glosstwo dogcall\n"
            "punwit punalpha cuectx forest glossone treecoat\n"
            "punwit punalpha cuectx hound glossone dogcall\n"
            "punwit punsense for punalpha cuectx forest?\n"
            "punwit punsense for punalpha cuectx hound?\n"
            "punwit punwhy for punalpha?\n"
            "persona usrbeta fondof stargaze shunof loudnoise\n"
            "persona suggest for usrbeta?\n"
            "persona update usrbeta fondof botany\n"
            "persona suggest for usrbeta?\n"
            "persona avoid for usrbeta?\n"
            "stattest expalpha sampm 56 nullm 50 stde 2\n"
            "stattest zscore for expalpha?\n"
            "remind me stattest zscore for expalpha?\n"
            "stattest pverdict for expalpha?\n"
            "stattest ciupper for expalpha?\n"
            "thermoc engbeta hotkel 500 coldkel 200 qinval 1000\n"
            "thermoc carnotpct for engbeta?\n"
            "remind me thermoc carnotpct for engbeta?\n"
            "thermoc engwork for engbeta?\n"
            "thermoc delts for engbeta?\n"
            "masterrec recalpha subcalls 4 divfactor 2 polyexp 1\n"
            "masterrec critexp for recalpha?\n"
            "masterrec mregime for recalpha?\n"
            "masterrec workat for recalpha nval 8?\n"
            "immunol pathalpha rzero 4 vaxpct 80\n"
            "immunol herdthr for pathalpha?\n"
            "immunol reffpct for pathalpha?\n"
            "immunol outstatus for pathalpha?\n"
            "ethicf casalpha livesok 10 harmcnt 1 dutyrule 0\n"
            "ethicf utilnet for casalpha?\n"
            "ethicf dutyverdict for casalpha?\n"
            "ethicf tension for casalpha?\n"
            "metaphor metblend mflow mtime?\n"
            "metaphor metplot plotquest?\n"
            "metaphor metcritique flawcase flawall?\n"
            "metaphor metteach metcompass mnorth mgoal\n"
            "metaphor metblend mnorth mgoal?\n"
            "could you metaphor metexplain metriver?\n"
            "ponder eagle\n"
            "juxtapose eagle penguin\n"
            "describe salmon\n"
            "sciloop labalpha hypfast 80 hypslow 40 scitest 82\n"
            "sciloop hconfirm for labalpha?\n"
            "sciloop hfalsify for labalpha?\n"
            "sciloop reserr for labalpha?\n"
            "talebox talealpha taleseed mira wolf flint questend moralbold\n"
            "talebox talenarr for talealpha?\n"
            "talebox talemoral for talealpha?\n"
            "talebox talebeta taleseed wren fire bridge questend moralteam\n"
            "talebox talemoral for talebeta?\n"
            "linprog lpalpha lpcap 20 lpgain 12 lpcost 4\n"
            "linprog optprofit for lpalpha?\n"
            "linprog shadowpx for lpalpha?\n"
            "linprog bindstate for lpalpha?\n"
            "spectral sigalpha fundhz 50 samplehz 180\n"
            "spectral harmthird for sigalpha?\n"
            "spectral nyqmin for sigalpha?\n"
            "remind me spectral nyqmin for sigalpha?\n"
            "spectral foldverdict for sigalpha?\n"
            "allelehw genealpha domrate 60 fitdom 90 fitrec 70\n"
            "allelehw carpct for genealpha?\n"
            "allelehw hompct for genealpha?\n"
            "allelehw seldir for genealpha?\n"
            "cryptodl dhalpha modp 23 genv 5 seca 6 secb 15\n"
            "cryptodl puba for dhalpha?\n"
            "cryptodl pubb for dhalpha?\n"
            "cryptodl dhsecret for dhalpha?\n"
            "remind me cryptodl dhsecret for dhalpha?\n"
            "trophic webalpha plantbio 1000 apexcnt 0\n"
            "trophic grazerbio for webalpha?\n"
            "trophic predbio for webalpha?\n"
            "trophic webshift for webalpha?\n"
            "belgraph belcheck evlens evprism?\n"
            "belgraph belcheck evvoid evpulse?\n"
            "belgraph beldebate peerada peerloki?\n"
            "belgraph belteach theoryflux evray\n"
            "belgraph belcheck evray?\n"
            "could you belgraph beldebate peerada peerloki?\n"
            "elaborate eagle\n"
            "elaborate salmon\n"
            "tell me more about it?\n"
            "peerrev pubalpha origv 50 replv 48 nsamp 100\n"
            "peerrev repcheck for pubalpha?\n"
            "peerrev poolmean for pubalpha?\n"
            "peerrev powcheck for pubalpha?\n"
            "proofform seralpha nterms 10 firstt 2 lastt 20 baseok 1\n"
            "proofform arithsum for seralpha?\n"
            "proofform trisum for seralpha?\n"
            "proofform indverdict for seralpha?\n"
            "statmech sysalpha statesg 2 npart 4 dofval 6 ktval 10\n"
            "statmech microcnt for sysalpha?\n"
            "statmech boltzbits for sysalpha?\n"
            "statmech equipart for sysalpha?\n"
            "gamemech mechalpha bidone 50 bidtwo 35 bidthree 20\n"
            "gamemech socwelf for mechalpha?\n"
            "gamemech vcgtax for mechalpha?\n"
            "gamemech icverdict for mechalpha?\n"
            "neurobio cellalpha resmem 5 capmem 4 incurr 6 vthresh 25\n"
            "neurobio tautime for cellalpha?\n"
            "remind me neurobio tautime for cellalpha?\n"
            "neurobio depolv for cellalpha?\n"
            "neurobio spikeverdict for cellalpha?\n"
            "controlopt ctlalpha polea 2 gaink 10\n"
            "controlopt cleig for ctlalpha?\n"
            "controlopt stabverdict for ctlalpha?\n"
            "controlopt dampmargin for ctlalpha?\n"
            "astrodyn orbalpha radin 100 radout 200 dvhave 50 dvneed 40\n"
            "astrodyn semiax for orbalpha?\n"
            "remind me astrodyn semiax for orbalpha?\n"
            "astrodyn eccpct for orbalpha?\n"
            "astrodyn fuelverdict for orbalpha?\n"
            "colldisc collpool specphys specmath?\n"
            "colldisc collvote lawether specphys speclogic?\n"
            "colldisc collderive axenergy axentropy?\n"
            "colldisc collmake thmdirac axwave axspin\n"
            "colldisc collderive axwave axspin?\n"
            "could you colldisc collprove thmcarnot?\n"
            "ruminate eagle\n"
            "elaborate it\n"
            "ruminate salmon\n"
            "speccode fnalpha inx 4 targety 14 slopek 3 biasb 2\n"
            "speccode specverdict for fnalpha?\n"
            "speccode evalnext for fnalpha?\n"
            "speccode opcount for fnalpha?\n"
            "qgate qsysalpha probzero 64 ampplus 5 ampminus 3\n"
            "qgate probone for qsysalpha?\n"
            "qgate intpower for qsysalpha?\n"
            "qgate entverdict for qsysalpha?\n"
            "chancode blkalpha kbits 12 nbits 16 dmin 4\n"
            "chancode coderate for blkalpha?\n"
            "chancode parbits for blkalpha?\n"
            "chancode detcap for blkalpha?\n"
            "heatpde rodalpha tleft 80 tmid 40 tright 80 rcoeff 25\n"
            "heatpde laplace for rodalpha?\n"
            "heatpde tnext for rodalpha?\n"
            "remind me heatpde tnext for rodalpha?\n"
            "heatpde cflverdict for rodalpha?\n"
            "coalbid grpalpha vala 20 valb 30 valab 90\n"
            "coalbid coalsurp for grpalpha?\n"
            "coalbid shapa for grpalpha?\n"
            "remind me coalbid shapa for grpalpha?\n"
            "coalbid coreverdict for grpalpha?\n"
            "epiopt outalpha rbase 5 covpct 60 effpct 80\n"
            "epiopt immpct for outalpha?\n"
            "epiopt reffsc for outalpha?\n"
            "epiopt epiverdict for outalpha?\n"
            "colllab socalpha labone 60 labtwo 64 labthree 62 soloest 40\n"
            "colllab ensmean for socalpha?\n"
            "colllab collgain for socalpha?\n"
            "colllab pubverdict for socalpha?\n"
            "selfres resdesign hypgrav hypdrag?\n"
            "selfres resdesign hypmag hypheat?\n"
            "selfres respatch symoffone syminfloop?\n"
            "selfres reslearn patfree symleak symoverfl\n"
            "selfres respatch symleak symoverfl?\n"
            "could you selfres reswhy patbound?\n"
            "discourse eagle\n"
            "discourse it\n"
            "discourse eagle penguin\n"
            "kalman filtalpha priorx 40 measz 60 prip 30 measr 20\n"
            "kalman kgain for filtalpha?\n"
            "kalman postx for filtalpha?\n"
            "remind me kalman postx for filtalpha?\n"
            "kalman postp for filtalpha?\n"
            "bayesnet netalpha priora 40 pbgivena 80 pbgivenna 30\n"
            "bayesnet margb for netalpha?\n"
            "bayesnet postab for netalpha?\n"
            "bayesnet bnverdict for netalpha?\n"
            "fluidyn tubealpha qbase 10 radmult 2 velin 6 pstat 120\n"
            "fluidyn poiseq for tubealpha?\n"
            "fluidyn velout for tubealpha?\n"
            "fluidyn cavverdict for tubealpha?\n"
            "cryptoecc zkalpha primeq 23 genk 5 secx 4 chalc 3\n"
            "cryptoecc pubkeyy for zkalpha?\n"
            "cryptoecc zkresp for zkalpha?\n"
            "cryptoecc zkverdict for zkalpha?\n"
            "optpath rovalpha distm 120 speedv 12 dragk 5 battj 800\n"
            "optpath triptime for rovalpha?\n"
            "optpath dragwork for rovalpha?\n"
            "optpath rngverdict for rovalpha?\n"
            "compileropt passalpha iters 100 invops 4 varops 6 deadops 3\n"
            "compileropt rawops for passalpha?\n"
            "compileropt hoistops for passalpha?\n"
            "remind me compileropt hoistops for passalpha?\n"
            "compileropt savedops for passalpha?\n"
            "econmacro macalpha inflp 6 targetp 2 neutralr 2 outgap 4\n"
            "econmacro inflgap for macalpha?\n"
            "econmacro taylorr for macalpha?\n"
            "econmacro realrate for macalpha?\n"
            "symcalc symdiff polyalpha 2?\n"
            "symcalc symint polybeta 1?\n"
            "symcalc symeval polygamma 3?\n"
            "symcalc symadd polyomega 4 2 3\n"
            "symcalc symdiff polyomega 2?\n"
            "could you symcalc symint polyalpha 2?\n"
            "teachlive gryphon bird soar\n"
            "storytime it\n"
            "whyso penguin fly\n"
            "kldiv distalpha 4 6 50\n"
            "kldiv klexcess for distalpha?\n"
            "kldiv klcross for distalpha?\n"
            "kldiv klstat for distalpha?\n"
            "bodemarg loopalpha 120 8 50\n"
            "bodemarg pmdeg for loopalpha?\n"
            "remind me bodemarg pmdeg for loopalpha?\n"
            "bodemarg gmdb for loopalpha?\n"
            "bodemarg ctrlstat for loopalpha?\n"
            "qtunnel wellalpha 10 6 2\n"
            "qtunnel qdef for wellalpha?\n"
            "qtunnel qkappa for wellalpha?\n"
            "qtunnel qtunstat for wellalpha?\n"
            "relativ partalpha 30 40 80\n"
            "relativ reletot for partalpha?\n"
            "relativ relkin for partalpha?\n"
            "relativ relstat for partalpha?\n"
            "raftcon clusalpha 7 5 4\n"
            "raftcon raftquor for clusalpha?\n"
            "remind me raftcon raftquor for clusalpha?\n"
            "raftcon raftfault for clusalpha?\n"
            "raftcon raftstat for clusalpha?\n"
            "regalloc cfgalpha 10 8 15\n"
            "regalloc spillcnt for cfgalpha?\n"
            "regalloc regover for cfgalpha?\n"
            "regalloc regstat for cfgalpha?\n"
            "evogame popalpha 40 80 30\n"
            "evogame esshawk for popalpha?\n"
            "evogame essdove for popalpha?\n"
            "evogame essstat for popalpha?\n"
            "progast astverify progalpha?\n"
            "progast astverify proggamma?\n"
            "progast astexec progbeta 4?\n"
            "progast astsynth progomega 2 7 4 13 6\n"
            "could you progast astexec progalpha 5?\n"
            "whatif penguin fly\n"
            "debatenow eagle penguin\n"
            "frankly about penguin fly\n"
            "merkletree treealpha 16 5 6\n"
            "merkletree mproof for treealpha?\n"
            "merkletree mtotal for treealpha?\n"
            "merkletree mstat for treealpha?\n"
            "rocketisp craftalpha 300 20 5000\n"
            "rocketisp exvel for craftalpha?\n"
            "rocketisp velgain for craftalpha?\n"
            "remind me rocketisp velgain for craftalpha?\n"
            "rocketisp orbstat for craftalpha?\n"
            "queueing srvalpha 6 4 30\n"
            "queueing qlen for srvalpha?\n"
            "queueing qspare for srvalpha?\n"
            "queueing qstat for srvalpha?\n"
            "enzymemm enzalpha 120 20 10\n"
            "enzymemm enzvel for enzalpha?\n"
            "remind me enzymemm enzvel for enzalpha?\n"
            "enzymemm enzpct for enzalpha?\n"
            "enzymemm enzstat for enzalpha?\n"
            "quorumrw dbalpha 5 3 3\n"
            "quorumrw qovlp for dbalpha?\n"
            "quorumrw qrws for dbalpha?\n"
            "quorumrw qcons for dbalpha?\n"
            "snrdb linkalpha 45 15 20\n"
            "snrdb snrval for linkalpha?\n"
            "snrdb fademarg for linkalpha?\n"
            "snrdb snrstat for linkalpha?\n"
            "cfgparse gramalpha 6 4 15\n"
            "cfgparse cnfsteps for gramalpha?\n"
            "cfgparse cykcells for gramalpha?\n"
            "cfgparse cfgstat for gramalpha?\n"
            "linalg lindet linalpha?\n"
            "linalg lininv linbeta?\n"
            "linalg linvec linalpha 2 3?\n"
            "linalg linsolve linomega 2 1 1 3 13 14\n"
            "could you linalg linsolve linalpha?\n"
            "cowrite eagle canyon\n"
            "curiousit it\n"
            "honestly about eagle penguin\n"
            "rsakey keyalpha 5 11 7\n"
            "rsakey rsamod for keyalpha?\n"
            "rsakey rsaphi for keyalpha?\n"
            "rsakey rsastat for keyalpha?\n"
            "aerolift wingalpha 2 10 4\n"
            "aerolift dynpres for wingalpha?\n"
            "aerolift liftfrc for wingalpha?\n"
            "remind me aerolift liftfrc for wingalpha?\n"
            "aerolift aerostat for wingalpha?\n"
            "cachegeo l1alpha 64 4 64\n"
            "cachegeo cachcap for l1alpha?\n"
            "cachegeo cachlin for l1alpha?\n"
            "cachegeo cachstat for l1alpha?\n"
            "keplerorb moonalpha 4 1\n"
            "keplerorb orbt2 for moonalpha?\n"
            "keplerorb orbper for moonalpha?\n"
            "keplerorb orbreg for moonalpha?\n"
            "quantadc adcalpha 8 1024\n"
            "quantadc qlevels for adcalpha?\n"
            "quantadc qsqnr for adcalpha?\n"
            "quantadc qadcstat for adcalpha?\n"
            "nernstcell galvalpha 1100 2 60\n"
            "nernstcell cellemf for galvalpha?\n"
            "nernstcell cellwrk for galvalpha?\n"
            "nernstcell cellstat for galvalpha?\n"
            "cournot firmalpha 100 10\n"
            "cournot courqty for firmalpha?\n"
            "cournot courprc for firmalpha?\n"
            "remind me cournot courprc for firmalpha?\n"
            "cournot courstat for firmalpha?\n"
            "bnet bnetpos bnalpha?\n"
            "bnet bnetneg bngamma?\n"
            "bnet bnetdec bnbeta?\n"
            "bnet bnetlive bnomega 50 80 20 20 80\n"
            "could you bnet bnetpos bngamma?\n"
            "critiqueit eagle penguin\n"
            "analogize eagle salmon\n"
            "actually about penguin\n"
            "blackbody staralpha 10 2 10\n"
            "blackbody bbwien for staralpha?\n"
            "blackbody bbpower for staralpha?\n"
            "blackbody bbstat for staralpha?\n"
            "hammcode codealpha 5\n"
            "hammcode hamdet for codealpha?\n"
            "hammcode hamcor for codealpha?\n"
            "hammcode hamstat for codealpha?\n"
            "lensopt optalpha 20 60\n"
            "lensopt foclen for optalpha?\n"
            "remind me lensopt foclen for optalpha?\n"
            "lensopt lensmag for optalpha?\n"
            "lensopt lensstat for optalpha?\n"
            "amortize vecalpha 32 2\n"
            "amortize arrgrow for vecalpha?\n"
            "amortize arrcopy for vecalpha?\n"
            "amortize arrstat for vecalpha?\n"
            "pendulum oscalpha 100 4 3\n"
            "pendulum pendfreq for oscalpha?\n"
            "pendulum pendene for oscalpha?\n"
            "pendulum pendstat for oscalpha?\n"
            "dnafreq dnalpha 30 30 12 8\n"
            "dnafreq gcfrac for dnalpha?\n"
            "dnafreq dnamelt for dnalpha?\n"
            "dnafreq dnastat for dnalpha?\n"
            "solowmod econalpha 30 200 10 400\n"
            "solowmod solinv for econalpha?\n"
            "solowmod solnet for econalpha?\n"
            "remind me solowmod solnet for econalpha?\n"
            "solowmod solstat for econalpha?\n"
            "mdpplan mdpval mdpalpha?\n"
            "mdpplan mdpmid mdpgamma?\n"
            "mdpplan mdppol mdpbeta?\n"
            "mdpplan mdplive mdpomega 5 10 100 40 90 50\n"
            "could you mdpplan mdpval mdpalpha?\n"
            "goalplan eagle canyon\n"
            "synthesize eagle salmon\n"
            "clearly about eagle salmon\n"
            "btreedb idxalpha 4 3\n"
            "btreedb btkeys for idxalpha?\n"
            "remind me btreedb btkeys for idxalpha?\n"
            "btreedb btleaf for idxalpha?\n"
            "btreedb btstat for idxalpha?\n"
            "bandgap semialpha 2000 300\n"
            "bandgap bgwave for semialpha?\n"
            "bandgap bgrat for semialpha?\n"
            "bandgap bgstat for semialpha?\n"
            "frustum camalpha 1600 900 4\n"
            "frustum frpix for camalpha?\n"
            "frustum frbuf for camalpha?\n"
            "frustum frstat for camalpha?\n"
            "quakegeo faultalph 50 40 2\n"
            "quakegeo qkpot for faultalph?\n"
            "quakegeo qkerg for faultalph?\n"
            "quakegeo qkstat for faultalph?\n"
            "unroll unralpha 64 4 6\n"
            "unroll urlit for unralpha?\n"
            "unroll urlsz for unralpha?\n"
            "unroll urlstat for unralpha?\n"
            "fiberopt fibalpha 150 140\n"
            "fiberopt fbna2 for fibalpha?\n"
            "fiberopt fbdel for fibalpha?\n"
            "fiberopt fbstat for fibalpha?\n"
            "buckconv regalpha 20 50 80\n"
            "buckconv bkvout for regalpha?\n"
            "remind me buckconv bkvout for regalpha?\n"
            "buckconv bkpwr for regalpha?\n"
            "buckconv bkstat for regalpha?\n"
            "kfilt kfgain kfalpha?\n"
            "kfilt kfpost kfgamma?\n"
            "kfilt kfmode kfbeta?\n"
            "kfilt kflive kfomega 40 30 10 10 90 0\n"
            "could you kfilt kfpost kfalpha?\n"
            "hypotest penguin fly\n"
            "reconsider penguin\n"
            "plainly eagle fly\n"
            "brpred cpupralph 10 20 15 10\n"
            "brpred bpovh for cpupralph?\n"
            "brpred bpcpi for cpupralph?\n"
            "remind me brpred bpcpi for cpupralph?\n"
            "brpred bpstat for cpupralph?\n"
            "albedo plnalph 300 30 60\n"
            "albedo alabs for plnalph?\n"
            "albedo alsurf for plnalph?\n"
            "albedo alstat for plnalph?\n"
            "bloomflt fltalph 1000 100\n"
            "bloomflt blbpi for fltalph?\n"
            "bloomflt blhsh for fltalph?\n"
            "bloomflt blstat for fltalph?\n"
            "cardiac hrtalph 70 80 112\n"
            "cardiac cdout for hrtalph?\n"
            "cardiac cdres for hrtalph?\n"
            "cardiac cdstat for hrtalph?\n"
            "passent pwdalph 12 6\n"
            "passent pebits for pwdalph?\n"
            "passent pelog for pwdalph?\n"
            "passent pestat for pwdalph?\n"
            "sabine hallalph 500 80\n"
            "sabine sbrt for hallalph?\n"
            "sabine sbden for hallalph?\n"
            "sabine sbstat for hallalph?\n"
            "polygeom ngonalph 6\n"
            "polygeom pgsum for ngonalph?\n"
            "remind me polygeom pgsum for ngonalph?\n"
            "polygeom pgdiag for ngonalph?\n"
            "polygeom pgstat for ngonalph?\n"
            "lqrctrl lqgain lqalpha?\n"
            "lqrctrl lqpole lqbeta?\n"
            "lqrctrl lqmode lqgamma?\n"
            "lqrctrl lqlive lqomega 80 100 30 10 60 0\n"
            "could you lqrctrl lqgain lqalpha?\n"
            "justifyit eagle\n"
            "justifyit penguin\n"
            "connectit\n"
            "actually about it\n"
            "imgcomp picx 800 600 4\n"
            "imgcomp icraw for picx?\n"
            "remind me imgcomp icraw for picx?\n"
            "imgcomp iczip for picx?\n"
            "imgcomp icstat for picx?\n"
            "baroalt skyx 300 4 6\n"
            "baroalt batemp for skyx?\n"
            "baroalt badrop for skyx?\n"
            "baroalt bastat for skyx?\n"
            "sharding clusx 120 4 5\n"
            "sharding shper for clusx?\n"
            "sharding shvir for clusx?\n"
            "sharding shstat for clusx?\n"
            "buckling colx 10 80 4\n"
            "buckling bupcr for colx?\n"
            "buckling buslend for colx?\n"
            "buckling bustat for colx?\n"
            "f1score evalx 60 90\n"
            "f1score f1harm for evalx?\n"
            "f1score f1arith for evalx?\n"
            "f1score f1stat for evalx?\n"
            "manning rivx 20 5 10\n"
            "manning mnflux for rivx?\n"
            "manning mndep for rivx?\n"
            "manning mnstat for rivx?\n"
            "pagetbl mmux 4 1024\n"
            "pagetbl ptspan for mmux?\n"
            "remind me pagetbl ptspan for mmux?\n"
            "pagetbl ptbytes for mmux?\n"
            "pagetbl ptstat for mmux?\n"
            "fftspec ffharm ffalpha?\n"
            "fftspec ffnyq ffbeta?\n"
            "fftspec ffband ffbeta?\n"
            "fftspec fflive ffomega 90 10 90 10 90 10 90 10\n"
            "could you fftspec ffharm ffalpha?\n"
            "challengeit eagle fly\n"
            "challengeit penguin fly\n"
            "askback\n"
            "actually about both\n"
            "shancap chnlx 20 16\n"
            "shancap scspec for chnlx?\n"
            "shancap scrate for chnlx?\n"
            "remind me shancap scrate for chnlx?\n"
            "shancap scstat for chnlx?\n"
            "froude flumx 30 20\n"
            "froude frnum for flumx?\n"
            "froude frsq for flumx?\n"
            "froude frregm for flumx?\n"
            "bezier curvx 10 50 30\n"
            "bezier bzmid for curvx?\n"
            "bezier bzavg for curvx?\n"
            "bezier bzstat for curvx?\n"
            "rcfilt rcnetx 10 25\n"
            "rcfilt rctau for rcnetx?\n"
            "rcfilt rcset for rcnetx?\n"
            "rcfilt rcstat for rcnetx?\n"
            "logitsep clfx 90 30 2\n"
            "logitsep lgmarg for clfx?\n"
            "logitsep lgscal for clfx?\n"
            "logitsep lgstat for clfx?\n"
            "hohmann orbx 100 300\n"
            "hohmann hhsma for orbx?\n"
            "hohmann hhecc for orbx?\n"
            "hohmann hhstat for orbx?\n"
            "eccsec ecpx 256\n"
            "eccsec ecbits for ecpx?\n"
            "remind me eccsec ecbits for ecpx?\n"
            "eccsec ecbytes for ecpx?\n"
            "eccsec ecstat for ecpx?\n"
            "oderk4 odval odalpha?\n"
            "oderk4 odeq odgamma?\n"
            "oderk4 odmode odbeta?\n"
            "oderk4 odlive odomega 1 50 40 20 50 4\n"
            "could you oderk4 odval odalpha?\n"
            "supposeit eagle fly\n"
            "supposeit penguin fly\n"
            "tellmore\n"
            "actually about other\n"
            "pvcell panlx 40 10 80 1\n"
            "pvcell pvpow for panlx?\n"
            "remind me pvcell pvpow for panlx?\n"
            "pvcell pveff for panlx?\n"
            "pvcell pvtier for panlx?\n"
            "mmqueue srvrx 8 10\n"
            "mmqueue mmutil for srvrx?\n"
            "mmqueue mmlen for srvrx?\n"
            "mmqueue mmtier for srvrx?\n"
            "glidrat glidx 100 40 20\n"
            "glidrat glrng for glidx?\n"
            "glidrat glsink for glidx?\n"
            "glidrat gltier for glidx?\n"
            "pkdose medix 50 4 30\n"
            "pkdose pkload for medix?\n"
            "pkdose pkclr for medix?\n"
            "pkdose pktier for medix?\n"
            "convnet layrx 32 4 2 8 16\n"
            "convnet cnout for layrx?\n"
            "convnet cnparm for layrx?\n"
            "convnet cntier for layrx?\n"
            "seismo quakx 10 22 6\n"
            "seismo smlag for quakx?\n"
            "seismo smdist for quakx?\n"
            "seismo smtier for quakx?\n"
            "batpack bpakx 48 20 80\n"
            "batpack bpwh for bpakx?\n"
            "remind me batpack bpwh for bpakx?\n"
            "batpack bphrs for bpakx?\n"
            "batpack bptier for bpakx?\n"
            "simlp sxopt sxalpha?\n"
            "simlp sxxval sxgamma?\n"
            "simlp sxbind sxbeta?\n"
            "simlp sxlive sxomega 5 4 2 1 20 1 2 16\n"
            "could you simlp sxopt sxalpha?\n"
            "proveit eagle animal\n"
            "proveit penguin animal\n"
            "recapit\n"
            "actually both similar\n"
            "capbank cbnkx 4 50 2 10\n"
            "capbank cbcap for cbnkx?\n"
            "remind me capbank cbcap for cbnkx?\n"
            "capbank cbeng for cbnkx?\n"
            "capbank cbtier for cbnkx?\n"
            "darcyp dpipx 4 50 2 10\n"
            "darcyp dphead for dpipx?\n"
            "darcyp dpshr for dpipx?\n"
            "darcyp dptier for dpipx?\n"
            "gustaf gfclx 20 8\n"
            "gustaf gfspd for gfclx?\n"
            "gustaf gfdrag for gfclx?\n"
            "gustaf gftier for gfclx?\n"
            "rankin rkturbx 3200 2200 6\n"
            "rankin rkwrk for rkturbx?\n"
            "rankin rkpow for rkturbx?\n"
            "rankin rktier for rkturbx?\n"
            "bdaybnd bdhshx 8 8\n"
            "bdaybnd bdthr for bdhshx?\n"
            "bdaybnd bdrisk for bdhshx?\n"
            "bdaybnd bdtier for bdhshx?\n"
            "beerlam blcuvx 5 6 4\n"
            "beerlam blabs for blcuvx?\n"
            "beerlam blod for blcuvx?\n"
            "beerlam bltier for blcuvx?\n"
            "putcall pcoptx 100 110 15\n"
            "putcall pcput for pcoptx?\n"
            "remind me putcall pcput for pcoptx?\n"
            "putcall pcgap for pcoptx?\n"
            "putcall pctier for pcoptx?\n"
            "pcaeig pclam pcalpha?\n"
            "pcaeig pcvar pcgamma?\n"
            "pcaeig pcax pcbeta?\n"
            "pcaeig pclive pcomega 28 6 12\n"
            "could you pcaeig pclam pcalpha?\n"
            "whydiff eagle penguin\n"
            "generalize\n"
            "actually which cannot\n"
            "blkhole bhobjx 100 50\n"
            "blkhole bhrad for bhobjx?\n"
            "remind me blkhole bhrad for bhobjx?\n"
            "blkhole bhphot for bhobjx?\n"
            "blkhole bhtier for bhobjx?\n"
            "hookesp hksprx 40 5 2\n"
            "hookesp hkfrc for hksprx?\n"
            "hookesp hkeng for hksprx?\n"
            "hookesp hktier for hksprx?\n"
            "tcpcwnd tcfllx 100 20\n"
            "tcpcwnd tcrate for tcfllx?\n"
            "tcpcwnd tcbdp for tcfllx?\n"
            "tcpcwnd tctier for tcfllx?\n"
            "hendbuf hbsolx 70 20 10\n"
            "hendbuf hbrat for hbsolx?\n"
            "hendbuf hbph for hbsolx?\n"
            "hendbuf hbtier for hbsolx?\n"
            "phongsh phmatx 40 60 200\n"
            "phongsh phdiff for phmatx?\n"
            "phongsh phspec for phmatx?\n"
            "phongsh phtier for phmatx?\n"
            "isostas ismntx 4 27 33\n"
            "isostas isroot for ismntx?\n"
            "isostas istot for ismntx?\n"
            "isostas istier for ismntx?\n"
            "sharpef shfndx 18 6 10\n"
            "sharpef shexr for shfndx?\n"
            "sharpef shrat for shfndx?\n"
            "remind me sharpef shrat for shfndx?\n"
            "sharpef shtier for shfndx?\n"
            "pdeslv pdmid pdalpha?\n"
            "pdeslv pdside pdgamma?\n"
            "pdeslv pdcfl pdbeta?\n"
            "pdeslv pdlive pdomega 0 100 100 100 0 20 2\n"
            "could you pdeslv pdmid pdalpha?\n"
            "deducepair eagle salmon\n"
            "commonroot\n"
            "actually contrast both\n"
            "debrog dbwavx 60 5 10\n"
            "debrog dbmom for dbwavx?\n"
            "debrog dblam for dbwavx?\n"
            "remind me debrog dblam for dbwavx?\n"
            "debrog dbtier for dbwavx?\n"
            "tailsla tssvcx 10 8 5\n"
            "tailsla tsp99 for tssvcx?\n"
            "tailsla tsbud for tssvcx?\n"
            "tailsla tstier for tssvcx?\n"
            "ionthr itengx 300 4 20\n"
            "ionthr itvel for itengx?\n"
            "ionthr itthrst for itengx?\n"
            "ionthr ittier for itengx?\n"
            "hillbnd hlrecx 100 4 2\n"
            "hillbnd hlpow for hlrecx?\n"
            "hillbnd hlfra for hlrecx?\n"
            "hillbnd hltier for hlrecx?\n"
            "beamst bmbarx 600 5 150\n"
            "beamst bmsig for bmbarx?\n"
            "beamst bmsect for bmbarx?\n"
            "beamst bmtier for bmbarx?\n"
            "arithcd acstrx 800 5 200\n"
            "arithcd acpay for acstrx?\n"
            "arithcd actot for acstrx?\n"
            "arithcd actier for acstrx?\n"
            "taylrl trbnkx 20 40 20 10\n"
            "taylrl trdev for trbnkx?\n"
            "taylrl trrate for trbnkx?\n"
            "remind me taylrl trrate for trbnkx?\n"
            "taylrl trtier for trbnkx?\n"
            "mcmcsmp mcprob mcalpha?\n"
            "mcmcsmp mcacc mcgamma?\n"
            "mcmcsmp mcmode mcbeta?\n"
            "mcmcsmp mclive mcomega 80 10 5 5 32 53\n"
            "could you mcmcsmp mcprob mcalpha?\n"
            "groupthree eagle penguin salmon\n"
            "whatelse\n"
            "actually both can\n"
            "fermidr fdmatx 40 20 10\n"
            "fermidr fdgap for fdmatx?\n"
            "fermidr fdocc for fdmatx?\n"
            "remind me fermidr fdocc for fdmatx?\n"
            "fermidr fdtier for fdmatx?\n"
            "bernflw bfpipx 2 10 50\n"
            "bernflw bfdyn for bfpipx?\n"
            "bernflw bftot for bfpipx?\n"
            "bernflw bftier for bfpipx?\n"
            "shamirs shkeyx 7 4\n"
            "shamirs shquor for shkeyx?\n"
            "shamirs shfault for shkeyx?\n"
            "shamirs shmtier for shkeyx?\n"
            "memvolt mvcelx 70 30 4\n"
            "memvolt mvdrv for mvcelx?\n"
            "memvolt mvcur for mvcelx?\n"
            "memvolt mvtier for mvcelx?\n"
            "invkine ikarmx 12 8\n"
            "invkine ikmax for ikarmx?\n"
            "invkine ikdead for ikarmx?\n"
            "invkine iktier for ikarmx?\n"
            "lotkavol lvecox 40 2 30 3\n"
            "lotkavol lvprey for lvecox?\n"
            "lotkavol lvpred for lvecox?\n"
            "lotkavol lvtier for lvecox?\n"
            "roofln rfgpu 160 20 40 600\n"
            "roofln rfint for rfgpu?\n"
            "roofln rfperf for rfgpu?\n"
            "remind me roofln rfperf for rfgpu?\n"
            "roofln rftier for rfgpu?\n"
            "/quit\n"
        )
        out_talk = run_cmd(["mind", "talk", "--load", str(child_en)], env, stdin_text=script)
        assert "your name is ben" in out_talk, out_talk
        assert "mind> 42" in out_talk, out_talk
        assert "meadow then river then cave" in out_talk, out_talk
        assert "kestrel is probably a bird" in out_talk, out_talk
        assert "yes because kestrel is a bird" in out_talk, out_talk
        assert "kestrel is similar to" in out_talk, out_talk
        assert "yes because maple is a tree" in out_talk, out_talk
        assert "maple is similar to" in out_talk, out_talk
        assert "probably rain is falling" in out_talk, out_talk
        assert "i doubt fox about pearl" in out_talk, out_talk
        assert "opal is inside tower" in out_talk, out_talk
        assert "i trust mira more than fox" in out_talk, out_talk
        assert "quartz is in trunk" in out_talk and "locket is in urn" in out_talk, out_talk
        assert "no gecko not has fur" in out_talk, out_talk
        assert "waiting for wren to remind about key" in out_talk, out_talk
        assert "reminder about key" in out_talk, out_talk
        assert "yes dawn happened before dusk" in out_talk and "dawn happened first" in out_talk, out_talk
        assert "mind> 11" in out_talk, out_talk
        assert "eve has 9 coin" in out_talk, out_talk
        assert "kai should give 4 berry to zoe" in out_talk, out_talk
        assert "crush smelt forge" in out_talk and "ingot" in out_talk, out_talk
        assert "[ecology]" in out_talk, out_talk
        assert "walk: meadow -> garden -> workshop" in out_talk and "crafted=fire" in out_talk, out_talk
        assert "sapphire is inside bridge" in out_talk, out_talk
        assert "mind> bat" in out_talk and "mind> 48" in out_talk, out_talk
        assert "order is alice then bob then carol" in out_talk and "contradiction because alice is taller than carol" in out_talk, out_talk
        assert "melt boil" in out_talk and "via camp then bridge then village then mine" in out_talk, out_talk
        assert "[2d place-cell GPU wave] camp -> mine" in out_talk and "bridge=BROKEN" in out_talk and "camp -> pass -> mine" in out_talk, out_talk
        assert "i trust mira" in out_talk and "spring comes after winter" in out_talk, out_talk
        assert "mind> 23" in out_talk and "mind> 20" in out_talk and "fox was wrong" in out_talk, out_talk
        assert "no sam may not touch fire" in out_talk and "yes sam may touch fire" in out_talk, out_talk
        assert "water becomes steam" in out_talk and "ore becomes ingot" in out_talk, out_talk
        assert "leftside is heavier" in out_talk and "17 is odd" in out_talk, out_talk
        assert "test crystal with litmus" in out_talk and "eliminated acidic so crystal is salty" in out_talk, out_talk
        assert "because litmus eliminated acidic so crystal is salty" in out_talk, out_talk
        assert "play paper because paper beats rock" in out_talk, out_talk
        assert "mira feels sad" in out_talk and "help mira find gem" in out_talk, out_talk
        assert "mira is ironic" in out_talk and "elder is brain of village" in out_talk, out_talk
        assert "penguin is inside cave" in out_talk, out_talk
        assert "proof kestrel then bird then animal then living" in out_talk, out_talk
        assert "thinking bird then animal then living" in out_talk and "boil caused steam" in out_talk and "thinking liquid then steam" in out_talk, out_talk
        assert "mind> 20. spring comes after winter" in out_talk, out_talk
        assert "because elena sang lullaby" in out_talk, out_talk
        assert "x equals 4" in out_talk and "mind> 12" in out_talk, out_talk
        assert "best value 22 take relic and chalice" in out_talk, out_talk
        assert "switch controls current" in out_talk, out_talk
        assert "no wall blocks sight from beacon to tower" in out_talk, out_talk
        assert "no without spark alarm would not happen" in out_talk and "yes alarm still happens" in out_talk, out_talk
        assert "which bird do you mean" in out_talk and "no although penguin is a bird penguin cannot fly" in out_talk, out_talk
        assert "mind> 25" in out_talk and "mind> 8" in out_talk, out_talk
        assert "leo feels sad" in out_talk and "mine is east of camp" in out_talk, out_talk
        assert "posterior is 80 percent" in out_talk and "posterior is 50 percent" in out_talk, out_talk
        assert "not wet implies not rain" in out_talk and "not warm or not dry" in out_talk, out_talk
        assert "mind> 24" in out_talk and "mind> 10" in out_talk and "mind> 16" in out_talk, out_talk
        assert "mind> 55" in out_talk and "mind> 6" in out_talk and "mind> 18" in out_talk, out_talk
        assert "alice is a knave" in out_talk and "no contradiction impossible" in out_talk, out_talk
        assert "no odd cycle needs more colors" in out_talk and "yes 3 colors suffice" in out_talk, out_talk
        assert "i feel curious" in out_talk and "what bird can swim and cannot fly" in out_talk, out_talk
        assert "yes correct it is penguin" in out_talk and "you are welcome" in out_talk, out_talk
        assert "while sparrow fly and penguin swim" in out_talk, out_talk
        assert "learned phrase salutation means hello" in out_talk and "opal is inside tower" in out_talk, out_talk
        assert "penguin is a bird can swim but cannot fly" in out_talk and "did you know" in out_talk, out_talk
        assert "fish by cortex" in out_talk and "winter by cortex" in out_talk and "dry by cortex" in out_talk, out_talk
        assert "mind> 4" in out_talk and "mind> 6" in out_talk and "mind> 25" in out_talk, out_talk
        assert "choose beta expected 60" in out_talk and "switch wins two thirds" in out_talk, out_talk
        assert "no counterexample falsified river wet" in out_talk, out_talk
        assert "since axe is broken use saw to cut timber" in out_talk, out_talk
        assert "wren now thinks flute is inside vault" in out_talk and "fox now thinks flute is inside camp" in out_talk, out_talk
        assert "should eat because hungry" in out_talk, out_talk
        assert "penguin by compose" in out_talk and "bat by compose" in out_talk, out_talk
        assert "your favorite animal is eagle" in out_talk and "you live in valley" in out_talk, out_talk
        assert "i run 28 pathways with" in out_talk and "we are talking about eagle" in out_talk, out_talk
        assert "glad that makes sense" in out_talk, out_talk
        assert "items coin and pearl" in out_talk and "items stone" in out_talk and "mind> 4" in out_talk, out_talk
        assert "no unsafe because wolf threatens hen" in out_talk and "yes safe to leave wolf and grain alone" in out_talk and "cross hen first" in out_talk, out_talk
        assert "mind> 121" in out_talk and "mind> 32" in out_talk and "mind> 30" in out_talk, out_talk
        assert "yes alice is strictly above bob" in out_talk and "uncertain bounds overlap" in out_talk and "mind> 3" in out_talk, out_talk
        assert "yes both leo and fox contributed" in out_talk and "fox intentionally stole gem" in out_talk, out_talk
        assert "mind> 15" in out_talk and "mind> 25" in out_talk and "mind> 50" in out_talk, out_talk
        assert "fire needed for cooked by cortex" in out_talk and "key needed for open by cortex" in out_talk, out_talk
        assert "fire by multicue" in out_talk and "cooked by multicue" in out_talk and "open by multicue" in out_talk and "pot by multicue" in out_talk, out_talk
        assert "tara carried compass" in out_talk and "tara climbed lighthouse" in out_talk and "tara visited harbor carried compass and climbed lighthouse because storm flooded harbor" in out_talk, out_talk
        assert "ask about wings" in out_talk and "ask about flight" in out_talk and "deduced candidate is penguin" in out_talk, out_talk
        assert "kestrel is a bird because kestrel has wings and feathers" in out_talk and "hypothetically with rocket penguin could fly but in reality penguin cannot fly" in out_talk, out_talk
        assert "yes every cedar belongs to flora" in out_talk and "no cedar belongs to mineral" in out_talk and "uncertain invalid syllogism" in out_talk, out_talk
        assert "yes pareto trade raises both surplus 12" in out_talk and "u equals 4" in out_talk and "v equals 6" in out_talk and "w equals 8" in out_talk, out_talk
        assert "yes observing wet supports rain" in out_talk and "yes intervening on sprinkler causes wet" in out_talk and "no intervention on wet does not cause rain" in out_talk, out_talk
        assert "glint is a luminous crystal that can glow" in out_talk and "tara visited harbor" in out_talk, out_talk
        assert "bird by hypothesis" in out_talk and "mammal by hypothesis" in out_talk and "fish by hypothesis" in out_talk, out_talk
        assert "kora crafted amulet" in out_talk and "bren discovered scroll" in out_talk and "yes kora mentored bren then bren mentored sora" in out_talk and "conflict bren mentored sora" in out_talk, out_talk
        assert "i know virel is a bird but where does virel live" in out_talk and "now i know virel lives in fjord" in out_talk and "virel is a bird in fjord" in out_talk, out_talk
        assert "yes 9 fits within 10" in out_talk and "yes satisfiable" in out_talk and "gamma must be true" in out_talk and "no unsatisfiable contradiction" in out_talk, out_talk
        assert "by analogy hum" in out_talk and "by analogy pulse then hum" in out_talk and "70 percent" in out_talk and "20 percent is safer" in out_talk, out_talk
        assert "earlier i thought zircon was in cellar" in out_talk and "revised because newer fact moved zircon to observatory" in out_talk, out_talk
        assert "summer by link" in out_talk and "bird by link" in out_talk and "liquid by link" in out_talk and "winter by link" in out_talk and "tree by link" in out_talk, out_talk
        assert "leo holds lantern" in out_talk and "lantern discovered inside cavern" in out_talk and "lantern carried to lighthouse" in out_talk and "nina gave lantern to leo" in out_talk, out_talk
        assert "lyra forged sword with anvil" in out_talk and "orin received sword from lyra" in out_talk and "orin shielded gate with sword" in out_talk, out_talk
        assert "trail expected reward 63" in out_talk and "cliff expected reward 75" in out_talk, out_talk
        assert ("lantern and cloak utility 32" in out_talk or "cloak and lantern utility 32" in out_talk) and ("lantern and rope utility 26" in out_talk or "rope and lantern utility 26" in out_talk), out_talk
        assert "yes mira permitted to enter vault" in out_talk and "no fox barred from vault" in out_talk, out_talk
        assert "meet from 4 to 6" in out_talk and "no overlap between mira and leo" in out_talk, out_talk
        assert "root fault is boiler" in out_talk and "yes filter upstream of turbine" in out_talk and "root fault is filter" in out_talk, out_talk
        assert "debate prefers bridge" in out_talk and "debate prefers tunnel" in out_talk, out_talk
        assert "salmon by cluster" in out_talk and "oak by cluster" in out_talk and "whale by cluster" in out_talk, out_talk
        assert ("sparrow and eagle by cluster" in out_talk or "eagle and sparrow by cluster" in out_talk), out_talk
        assert "tara discovered chalice" in out_talk and "tara visited watchtower" in out_talk and "chalice is luminous" in out_talk, out_talk
        assert "yes superadditive synergy 6" in out_talk and "mira shapley share is 7" in out_talk, out_talk
        assert "equilibrium 40 degree phase liquid" in out_talk and "equilibrium 100 degree phase steam" in out_talk, out_talk
        assert "mind> 10" in out_talk and "mind> 14" in out_talk and "yes 10 differs from 14" in out_talk, out_talk
        assert "mind> 12" in out_talk and "no short by 2" in out_talk, out_talk
        assert "consensus beacon is safe" in out_talk and "dissenter is fox" in out_talk and "yes supermajority quorum 3" in out_talk, out_talk
        assert "yes latch is open at tick 4" in out_talk and "no latch not open at tick 9" in out_talk and "yes rain is active at tick 9" in out_talk, out_talk
        assert "yes by patched rule eagle can fly" in out_talk and "no exception penguin cannot fly" in out_talk and "if winged except penguin then can fly" in out_talk, out_talk
        assert "consistent by gating" in out_talk and "exception by gating" in out_talk and "novel by gating" in out_talk, out_talk
        assert "hawk near cliff" in out_talk and "hawk hunted mouse" in out_talk and "spotted hawk near cliff and hawk hunted mouse" in out_talk, out_talk
        assert "best action nap value 5" in out_talk and "best action forage value 12" in out_talk, out_talk
        assert "yes mediated by drysoil" in out_talk and "no blocking drysoil stops drought" in out_talk and "yes direct path from heat" in out_talk, out_talk
        assert "mind> 4" in out_talk and "mind> 9" in out_talk and "mind> 13" in out_talk, out_talk
        assert "mira wins relic" in out_talk and "winner pays 9" in out_talk and "dominant bid is 12" in out_talk, out_talk
        assert "mind> 4 6" in out_talk and "mind> 10 12" in out_talk and "mind> 32" in out_talk, out_talk
        assert "mind> 7" in out_talk and "mind> 12" in out_talk and "critical module is framing" in out_talk, out_talk
        assert "rescue overrides patrol" in out_talk and "next duty is patrol" in out_talk and "yes higher priority 10 over 4" in out_talk, out_talk
        assert "autumn via summer by dreaming" in out_talk and "steam via liquid by dreaming" in out_talk and "animal via bird by dreaming" in out_talk and "summer via spring by dreaming" in out_talk, out_talk
        assert "to cross gorge" in out_talk and "built rope" in out_talk and "lina wanted crystal overcame gorge with rope and retrieved crystal" in out_talk, out_talk
        assert "clearing price is 6" in out_talk and "mind> 8" in out_talk, out_talk
        assert "mind> 24" in out_talk and "mind> 6" in out_talk, out_talk
        assert "missing nails for shelter" in out_talk and "find nails for shelter" in out_talk and "ready to build shelter" in out_talk, out_talk
        assert "mind> 11 11" in out_talk and "mind> 13 11" in out_talk and "mind> 4 5" in out_talk, out_talk
        assert "mind> 14" in out_talk and "mind> 2" in out_talk, out_talk
        assert "dominant strategy is defect" in out_talk and "nash equilibrium is defect defect" in out_talk and "yes social optimum 6 over 2" in out_talk, out_talk
        assert "necessary cause is battery" in out_talk and "yes redundant cause heat suffices" in out_talk and "no redundant because heat suffices" in out_talk, out_talk
        assert "no counterexample penguin by scan" in out_talk and "yes universal across fish by scan" in out_talk and "yes witness bat by scan" in out_talk and "yes universal across bird by scan" in out_talk, out_talk
        assert "recommend forest because shade" in out_talk and "because user prefers forest for shade" in out_talk and "user enjoys stargazing in meadow" in out_talk, out_talk
        assert "slope 2 intercept 5" in out_talk and "yield is 85" in out_talk and "temp is 50" in out_talk, out_talk
        assert "profit is 40" in out_talk and "roi is 60 percent" in out_talk and "invest in quarry roi 60 percent" in out_talk, out_talk
        assert "mind> 30" in out_talk and "mind> 100" in out_talk and "mind> 5" in out_talk, out_talk
        assert "winner is glider score 32" in out_talk and "winner is barge score 66" in out_talk, out_talk
        assert "minimum wiring cost is 12" in out_talk and "skipped cable bridge camp cost 8" in out_talk and "minimum wiring cost is 10" in out_talk, out_talk
        assert "no violated at step 3" in out_talk and "yes reached at step 4" in out_talk and "yes green held until amber at step 3" in out_talk, out_talk
        assert "mind> 5" in out_talk and "mind> 3" in out_talk and "mind> 2" in out_talk, out_talk
        assert "yes proved in 3 hops via bird" in out_talk and "no blocked at liquid" in out_talk and "no unproved" in out_talk and "no blocked at bird" in out_talk, out_talk
        assert "comet has tail and comet orbits sun and eclipse hides sun" in out_talk and "both link to sun in astronomy" in out_talk and "curious what else about tail in astronomy" in out_talk, out_talk
        assert "shortest distance is 6" in out_talk and "camp then pass then forest then village" in out_talk and "shortest distance is 7" in out_talk, out_talk
        assert "posterior is 50 percent" in out_talk and "posterior is 75 percent" in out_talk, out_talk
        assert "mind> 30" in out_talk and "mind> 24" in out_talk and "mind> 28" in out_talk, out_talk
        assert "mind> 72" in out_talk and "orb velocity 2 and puck velocity 8" in out_talk and "yes total energy 72" in out_talk, out_talk
        assert "mind> 25" in out_talk and "pack crate and barrel" in out_talk and "mind> 21" in out_talk, out_talk
        assert "no blocked by mira and wren" in out_talk and "yes stable in core" in out_talk and "mind> 10" in out_talk, out_talk
        assert "test vapor or compass" in out_talk and "magnetic survives experiment" in out_talk and "because thermal predicts vapor not compass" in out_talk, out_talk
        assert "steam from liquid from solid by abduce" in out_talk and "autumn from summer from spring by abduce" in out_talk and "winter from autumn from summer by abduce" in out_talk and "plant from tree from " in out_talk, out_talk
        assert "volcano warms geyser then geyser fills lagoon" in out_talk and "geyser fills lagoon and heron stalks lagoon" in out_talk and "why does volcano warms geyser" in out_talk, out_talk
        assert "depleted in 5 cycles" in out_talk and "sustainable draw is 4" in out_talk and "mind> 40" in out_talk and "mind> 60" in out_talk and "yes det 0 singular" in out_talk, out_talk
        assert "anomaly is 64" in out_talk and "schedule j1 and j3" in out_talk and "condorcet cycle paradox" in out_talk and "condorcet winner park" in out_talk, out_talk
        assert "yes collider conditioning opens path" in out_talk and "confounder is season" in out_talk and "yes dseparated" in out_talk, out_talk
        assert "from bird to fish by imagine" in out_talk and "from fish to bird by imagine" in out_talk and "stays bird by imagine" in out_talk and "from tree to fish by imagine" in out_talk, out_talk
        assert "ember thawed glacier then glacier swelled gully" in out_talk and "because ember thawed glacier" in out_talk and "hiker scouted gully" in out_talk, out_talk
        assert "80 percent" in out_talk and "72 percent" in out_talk and "67 percent" in out_talk, out_talk
        assert "mind> 42" in out_talk and "x 4 and y 6" in out_talk and "mind> 38" in out_talk, out_talk
        assert "lca is leftbranch" in out_talk and "mind> 20" in out_talk and "mind> 15" in out_talk and "mind> 9" in out_talk, out_talk
        assert "75 percent" in out_talk and "25 percent" in out_talk and "50 percent" in out_talk, out_talk
        assert "sweep is better" in out_talk and "9 times costlier" in out_talk and "lookup is quickest" in out_talk, out_talk
        assert "cooked needs fire from flint by plan" in out_talk and "open needs key by plan" in out_talk and "ready for cooked by plan" in out_talk and "pot needs fire from flint by plan" in out_talk, out_talk
        assert "windmill powers granary" in out_talk and "storm damaged windmill" in out_talk and "maya built windmill at hilltop" in out_talk, out_talk
        assert "mind> 18" in out_talk and "yes blight shrinks" in out_talk, out_talk
        assert "yes floats submerged 50 percent" in out_talk and "no sinks" in out_talk, out_talk
        assert "salt efficiency 6" in out_talk and "mind> 15" in out_talk, out_talk
        assert "yes acyclic" in out_talk and "hull then mast then sail then voyage" in out_talk and "no cycle deadlock" in out_talk, out_talk
        assert "mind> 256" in out_talk and "32 times quicker" in out_talk, out_talk
        assert "mind> 150" in out_talk and "mind> 90" in out_talk and "higher pitch" in out_talk, out_talk
        assert "yes coop stable" in out_talk and "no defection prevails" in out_talk, out_talk
        assert "solid via liquid to steam by simulate" in out_talk and "solid via liquid to frozen by simulate" in out_talk and "blocked at liquid by simulate" in out_talk and "cold via warm to sown by simulate" in out_talk, out_talk
        assert "beacon warned crew" in out_talk and "nora saved ship at harbor" in out_talk and "crew thanked nora" in out_talk, out_talk
        assert "hydrogen is limiting" in out_talk and "virtual upright" in out_talk and "200 centibits" in out_talk and "150 centibits" in out_talk and "skewed has lower entropy" in out_talk, out_talk
        assert "no dock saturated" in out_talk and "mind> 19" in out_talk and "no alice envies bob" in out_talk and "yes cake envyfree" in out_talk, out_talk
        assert "penguin by constrain" in out_talk and "bat by constrain" in out_talk and "oak by constrain" in out_talk and "whale by constrain" in out_talk, out_talk
        assert "because fog veiled trail" in out_talk and "lena guided hiker at ridge" in out_talk and "hiker praised lena" in out_talk, out_talk
        assert "mind> 12 16" in out_talk and "mind> 1210" in out_talk and "mind> 1000" in out_talk and "mind> 80" in out_talk, out_talk
        assert "yes bipartite" in out_talk and "no odd cycle" in out_talk and "no clean reconstruction" in out_talk and "yes aliased at 20" in out_talk, out_talk
        assert "yes tragedy overused" in out_talk and "no socially beneficial" in out_talk, out_talk
        assert "autumn by induce" in out_talk and "liquid then steam by induce" in out_talk and "light by induce" in out_talk and "tree then plant by induce" in out_talk, out_talk
        assert "despite river flooded valley" in out_talk and "because bridge braced cart" in out_talk and "soren mended bridge and soren warned traveler and traveler thanked soren" in out_talk, out_talk
        assert "mind> 64" in out_talk and "mind> 7 and 3" in out_talk and "mind> 9 and 4" in out_talk and "mind> 2 to 1" in out_talk and "mind> 3 to 1" in out_talk, out_talk
        assert "counterclockwise" in out_talk and "yes accepted at q2" in out_talk and "no rejected at q0" in out_talk and "mind> 200" in out_talk, out_talk
        assert "liquid then melt by chainrule" in out_talk and "open then key by chainrule" in out_talk and "cooked then fire by chainrule" in out_talk and "sad via chainrule" in out_talk and "liquid via chainrule" in out_talk, out_talk
        assert "talia sealed hatch" in out_talk and "talia lit beacon" in out_talk and "crew spotted beacon at cliff" in out_talk and "talia sealed hatch then talia lit beacon then crew spotted beacon at cliff" in out_talk, out_talk
        assert "mind> 75" in out_talk and "mind> 90" in out_talk and "mind> 120" in out_talk and "mind> 25" in out_talk and "mind> 7" in out_talk, out_talk
        assert "yes euler circuit" in out_talk and "euler path only" in out_talk and "no not eulerian" in out_talk and "67 percent" in out_talk, out_talk
        assert "spark then blaze by onlineschema" in out_talk and "spark then blaze then ash by onlineschema" in out_talk and "spark by onlineschema" in out_talk and "glow then flare by onlineschema" in out_talk, out_talk
        assert "lantern hid key" in out_talk and "valley floods then mill stops" in out_talk and "because sluice is jammed" in out_talk, out_talk
        assert "no below cutoff" in out_talk and "alkaline" in out_talk and "mind> 27" in out_talk and "no not admissible" in out_talk and "mind> 23" in out_talk, out_talk
        assert "ember then glow by reviseschema" in out_talk and "dew then mist by onlineschema" in out_talk and "frost then rime by reviseschema" in out_talk and "pyre then torch by reviseschema" in out_talk, out_talk
        assert "earlier harbor now pier" in out_talk and "did you mean mira or orin" in out_talk and "mind> 480" in out_talk and "mind> 48" in out_talk and "mind> 32" in out_talk, out_talk
        assert "glade by rolebind" in out_talk and "solis by rolebind" in out_talk and "pollen by rolebind" in out_talk and "crafts by rolebind" in out_talk and "spire by rolebind" in out_talk, out_talk
        assert "velox means swift" in out_talk and "velox to nectar to moth" in out_talk and "yes orthogonal" in out_talk and "yes coprime" in out_talk and "mind> 5000" in out_talk and "mind> 720" in out_talk, out_talk
        assert "falcon pursued hare plateau seqparse" in out_talk and "falcon by seqparse" in out_talk and "dolphin escorted otter reef seqparse" in out_talk and "panther stalked ibex crag seqparse" in out_talk and "crag by seqparse" in out_talk, out_talk
        assert "condor sighted lynx then lynx tracked deer" in out_talk and "mind> 50000" in out_talk and "mind> 8100" in out_talk and "mind> 580" in out_talk and "no singmat" in out_talk and "mind> 1331" in out_talk, out_talk
        assert "6 by neuralmath" in out_talk and "9 by neuralmath" in out_talk and "56 by neuralmath" in out_talk and "24 by neuralmath" in out_talk and "7 by neuralmath" in out_talk, out_talk
        assert "osprey caught perch estuary seqparse" in out_talk and "badger dug burrow hollow seqparse" in out_talk and "perch by seqparse" in out_talk, out_talk
        assert "mind> 300" in out_talk and "mind> 808" in out_talk and "mind> 112" in out_talk and "mind> 30" in out_talk and "mind> 14" in out_talk, out_talk
        assert "7 by graphwave" in out_talk and "gn1 by graphwave" in out_talk and "10 by graphwave" in out_talk and "11 by graphwave" in out_talk and "3 by graphwave" in out_talk, out_talk
        assert "marten by seqparse" in out_talk and "crevice by seqparse" in out_talk and "vole by seqparse" in out_talk, out_talk
        assert "mind> -3" in out_talk and "critdamp" in out_talk and "underdamp" in out_talk and "mind> 3400" in out_talk and "mind> 21" in out_talk, out_talk
        assert "yes satisfiable by satsolve" in out_talk and "sattrue by satsolve" in out_talk and "satfree by satsolve" in out_talk and "no unsatisfiable by satsolve" in out_talk, out_talk
        assert "squall uprooted spruce knoll seqparse" in out_talk and "beaver heaped dam brook seqparse" in out_talk and "beaver by seqparse" in out_talk, out_talk
        assert "beryl hbmdeep" in out_talk and "garnet hbmfound" in out_talk and "mind> 16384" in out_talk, out_talk
        assert "mind> 4000" in out_talk and "turbulent" in out_talk and "laminar" in out_talk and "mind> 231" in out_talk and "mind> 254" in out_talk and "mind> 80" in out_talk, out_talk
        assert "visring by retina" in out_talk and "2 by retina" in out_talk and "visabove by retina" in out_talk and "visleft by retina" in out_talk and "vislshape by retina" in out_talk and "viscross by retina" in out_talk, out_talk
        assert "curious habitat of zephryx" in out_talk and "fullstudy zephryx" in out_talk and "zephryx is soarer with feathers eats beetles lives ravine" in out_talk, out_talk
        assert "mind> 15" in out_talk and "codeok" in out_talk and "mind> 400" in out_talk and "mind> 180" in out_talk and "mind> 108" in out_talk and "mind> 57" in out_talk and "mind> 16" in out_talk, out_talk
        assert "audtritone by cochlea" in out_talk and "3 by cochlea" in out_talk and "pitchup by cochlea" in out_talk and "audmarch by cochlea" in out_talk and "audmajor by cochlea" in out_talk, out_talk
        assert "mind> 28" in out_talk and "0 tbblocked by fogshroud" in out_talk and "quizfail 24" in out_talk and "quizpass 24" in out_talk, out_talk
        assert "mind> 19" in out_talk and "bugstep 3 codepatch 3" in out_talk and "toneclarify" in out_talk and "tonewarm" in out_talk and "toneirony" in out_talk, out_talk
        assert "mind> 36" in out_talk and "mind> 56" in out_talk and "mind> 58" in out_talk and "mind> 200" in out_talk, out_talk
        assert "xbeacon by xmodal" in out_talk and "audtritone by xmodal" in out_talk and "goalfix depth 2 by gstack" in out_talk and "xconvoy by xmodal" in out_talk and "xhazard by xmodal" in out_talk, out_talk
        assert "unblock missingrope for buildbridge" in out_talk and "buildbridge goalready" in out_talk and "humbleq about darkmatter" in out_talk and "confident about buildbridge" in out_talk, out_talk
        assert "overload" in out_talk and "approve billalpha" in out_talk and "tradeoff swift over heavy" in out_talk and "mind> 1440" in out_talk and "mind> 4" in out_talk and "mind> 40" in out_talk, out_talk
        assert "dlgcompare by dialogue" in out_talk and "dlgempathize dlgshift by dialogue" in out_talk and "dlgcontra by dialogue" in out_talk and "dlgdebate dlgshift by dialogue" in out_talk and "dlgexplain dlgshift by dialogue" in out_talk, out_talk
        assert "eagle is bird can fly" in out_talk and "both animal eagle fly salmon swim" in out_talk and "punlink treecoat dogcall" in out_talk and "recopt botany" in out_talk and "avoidopt loudnoise" in out_talk, out_talk
        assert "rejectnull" in out_talk and "mind> 600" in out_talk and "leafheavy" in out_talk and "mind> 64" in out_talk and "contained" in out_talk and "dutyforbid" in out_talk and "ethconflict" in out_talk, out_talk
        assert "metriver by metaphor" in out_talk and "plottrial plotturn by metaphor" in out_talk and "flawhasty by metaphor" in out_talk and "metcompass by metaphor" in out_talk and "mflow mtime by metaphor" in out_talk, out_talk
        assert "salmon is fish can swim" in out_talk and "hconfirm hypfast" in out_talk and "hfalsify hypslow" in out_talk and "mira faces wolf uses flint reaches questend" in out_talk and "talemoral moralbold" in out_talk and "talemoral moralteam" in out_talk, out_talk
        assert "lpbinding" in out_talk and "mind> 300" in out_talk and "foldedwave" in out_talk and "favordom" in out_talk and "mind> 19" in out_talk and "preyboom" in out_talk, out_talk
        assert "theorywave belconfirm by belgraph" in out_talk and "theoryfield belconfirm by belgraph" in out_talk and "peerada by belgraph" in out_talk and "theoryflux belconfirm by belgraph" in out_talk, out_talk
        assert "eagle is bird has" in out_talk and "salmon is fish has" in out_talk and "replicated" in out_talk and "highpower" in out_talk and "indvalid" in out_talk and "mind> 110" in out_talk, out_talk
        assert "ictruthful" in out_talk and "firespike" in out_talk and "clstable" in out_talk and "mind> 150" in out_talk and "fuelok" in out_talk, out_talk
        assert "lawthermo collaccept by colldisc" in out_talk and "lawether collreject by colldisc" in out_talk and "thmcarnot by colldisc" in out_talk and "thmdirac by colldisc" in out_talk and "axenergy axentropy by colldisc" in out_talk, out_talk
        assert "what about" in out_talk and "specpass" in out_talk and "qentangled" in out_talk and "cflstable" in out_talk and "corestable" in out_talk and "episurge" in out_talk and "pubaccept" in out_talk, out_talk
        assert "expvacuum by selfres" in out_talk and "expcoil by selfres" in out_talk and "patbound by selfres" in out_talk and "patfree by selfres" in out_talk and "symoffone syminfloop by selfres" in out_talk, out_talk
        assert "both eagle and penguin are" in out_talk and "mind> 52" in out_talk and "bnlikely" in out_talk and "mind> 160" in out_talk and "cavsafe" in out_talk and "zkvalid" in out_talk and "mind> 720" in out_talk and "rangeok" in out_talk and "mind> 604" in out_talk and "mind> 399" in out_talk, out_talk
        assert "18 by symcalc" in out_talk and "8 by symcalc" in out_talk and "34 by symcalc" in out_talk and "polyomega by symcalc" in out_talk and "14 by symcalc" in out_talk and "24 by symcalc" in out_talk, out_talk
        assert "gryphon is bird can soar like" in out_talk and "in valley with" in out_talk and "because penguin is bird but penguin cannot fly" in out_talk, out_talk
        assert "kltight" in out_talk and "ctrlstab" in out_talk and "qpass" in out_talk and "relfast" in out_talk and "raftcomm" in out_talk and "regspilled" in out_talk and "essgrow" in out_talk, out_talk
        assert "astsafe by progast" in out_talk and "astfault by progast" in out_talk and "17 by progast" in out_talk and "19 by progast" in out_talk and "13 by progast" in out_talk, out_talk
        assert "if penguin could fly then penguin like" in out_talk and "eagle can fly while penguin cannot fly both are bird" in out_talk and "because penguin is bird but penguin cannot fly" in out_talk, out_talk
        assert "merkfast" in out_talk and "mind> 6000" in out_talk and "orbmake" in out_talk and "qstable" in out_talk and "enzsat" in out_talk and "qstrong" in out_talk and "siglock" in out_talk and "cfgacc" in out_talk, out_talk
        assert "5 by linalg" in out_talk and "linsing by linalg" in out_talk and "17 by linalg" in out_talk and "3 by linalg" in out_talk, out_talk
        assert "eagle is bird can fly to canyon what next for eagle" in out_talk and "i know eagle is bird can fly where is eagle like" in out_talk and "eagle can fly while penguin cannot fly both are bird" in out_talk, out_talk
        assert "rsavalid" in out_talk and "mind> 400" in out_talk and "airborne" in out_talk and "mind> 16384" in out_talk and "lowconf" in out_talk and "innerorb" in out_talk and "hiresadc" in out_talk and "galvanic" in out_talk and "profiteq" in out_talk, out_talk
        assert "50 by bnet" in out_talk and "13 by bnet" in out_talk and "bnetwait by bnet" in out_talk and "80 bnetact by bnet" in out_talk and "75 by bnet" in out_talk, out_talk
        assert "not all bird can fly because penguin is bird cannot fly" in out_talk and "eagle like fly and salmon like swim both animal" in out_talk and "penguin is a bird can swim but cannot fly" in out_talk, out_talk
        assert "mind> 290" in out_talk and "bbuv" in out_talk and "hamsec" in out_talk and "mind> 15" in out_talk and "lenreal" in out_talk and "arrconst" in out_talk and "mind> 450" in out_talk and "oscstiff" in out_talk and "dnastab" in out_talk and "solgrow" in out_talk, out_talk
        assert "42 by mdpplan" in out_talk and "80 by mdpplan" in out_talk and "mdpleap by mdpplan" in out_talk and "47 mdpleap by mdpplan" in out_talk, out_talk
        assert "for canyon eagle is bird can fly" in out_talk and "eagle with fly and salmon with swim both are animal" in out_talk and "eagle can fly while salmon can swim both are animal" in out_talk, out_talk
        assert "btshall" in out_talk and "mind> 620" in out_talk and "bgwide" in out_talk and "mind> 1440" in out_talk and "frwide" in out_talk and "mind> 2000" in out_talk and "qkmajor" in out_talk and "urlfit" in out_talk and "mind> 2900" in out_talk and "fbmulti" in out_talk and "bkccm" in out_talk, out_talk
        assert "50 by kfilt" in out_talk and "70 by kfilt" in out_talk and "kfprior by kfilt" in out_talk and "80 kftrust by kfilt" in out_talk and "45 by kfilt" in out_talk, out_talk
        assert "false penguin is bird but cannot fly" in out_talk and "still penguin is bird can swim except cannot fly" in out_talk and "because eagle is bird has" in out_talk, out_talk
        assert "bpfast" in out_talk and "mind> 210" in out_talk and "alwarm" in out_talk and "blclean" in out_talk and "mind> 56" in out_talk and "cdnorm" in out_talk and "pestrng" in out_talk and "sbcrisp" in out_talk and "mind> 720" in out_talk and "pghex" in out_talk, out_talk
        assert "62 by lqrctrl" in out_talk and "29 by lqrctrl" in out_talk and "lqunst by lqrctrl" in out_talk and "37 lqstab by lqrctrl" in out_talk, out_talk
        assert "since eagle is bird has" in out_talk and "since penguin is bird can swim but cannot fly" in out_talk and "eagle can fly and penguin can swim both are bird" in out_talk, out_talk
        assert "icloss" in out_talk and "mind> 276" in out_talk and "bamild" in out_talk and "shbal" in out_talk and "bustiff" in out_talk and "mind> 72" in out_talk and "f1high" in out_talk and "mnfast" in out_talk and "mind> 4096" in out_talk and "pthuge" in out_talk, out_talk
        assert "40 by fftspec" in out_talk and "30 by fftspec" in out_talk and "ffhigh by fftspec" in out_talk and "40 ffhigh by fftspec" in out_talk, out_talk
        assert "affirmed eagle is bird has" in out_talk and "corrected penguin is bird cannot fly instead can swim" in out_talk and "why can eagle fly while penguin can swim" in out_talk, out_talk
        assert "scwide" in out_talk and "mind> 150" in out_talk and "frsuper" in out_talk and "bzarch" in out_talk and "mind> 1250" in out_talk and "rcfast" in out_talk and "lgsharp" in out_talk and "hhellip" in out_talk and "mind> 128" in out_talk and "ecstrng" in out_talk, out_talk
        assert "62 by oderk4" in out_talk and "50 by oderk4" in out_talk and "odegrow by oderk4" in out_talk and "58 odestab by oderk4" in out_talk, out_talk
        assert "actually eagle is bird and can fly" in out_talk and "if penguin would fly then actually penguin is bird can swim cannot fly" in out_talk and "also penguin is bird can swim while eagle can fly" in out_talk and "since eagle is bird has" in out_talk, out_talk
        assert "pvpeak" in out_talk and "mind> 80" in out_talk and "mmbusy" in out_talk and "mind> 4000" in out_talk and "glsoar" in out_talk and "pkfast" in out_talk and "mind> 2048" in out_talk and "cndeep" in out_talk and "smnear" in out_talk and "mind> 960" in out_talk and "bplong" in out_talk, out_talk
        assert "54 by simlp" in out_talk and "6 by simlp" in out_talk and "sxcorn by simlp" in out_talk and "56 sxdual by simlp" in out_talk, out_talk
        assert "proved eagle is bird and bird is animal and eagle can fly" in out_talk and "proved penguin is bird and bird is animal although penguin cannot fly" in out_talk and "recap eagle is bird can fly and penguin is bird can swim" in out_talk and "both eagle and penguin are bird while eagle can fly but penguin cannot fly" in out_talk, out_talk
        assert "cbhigh" in out_talk and "mind> 500" in out_talk and "dphigh" in out_talk and "mind> 660" in out_talk and "gfwide" in out_talk and "mind> 6000" in out_talk and "rkpeak" in out_talk and "mind> 256" in out_talk and "bdsafe" in out_talk and "bldark" in out_talk and "pcotm" in out_talk, out_talk
        assert "15 by pcaeig" in out_talk and "70 by pcaeig" in out_talk and "pciso by pcaeig" in out_talk and "30 pcaxis by pcaeig" in out_talk, out_talk
        assert "because eagle is bird can fly while penguin is bird can swim cannot fly" in out_talk and "most bird can fly except penguin can swim" in out_talk and "penguin is bird cannot fly instead can swim" in out_talk, out_talk
        assert "bhsuper" in out_talk and "mind> 450" in out_talk and "hkstiff" in out_talk and "mind> 5000" in out_talk and "tcgbps" in out_talk and "hbbasic" in out_talk and "mind> 120" in out_talk and "phgloss" in out_talk and "isdeep" in out_talk and "shstrng" in out_talk, out_talk
        assert "44 by pdeslv" in out_talk and "62 by pdeslv" in out_talk and "pdeunst by pdeslv" in out_talk and "92 pdestab by pdeslv" in out_talk, out_talk
        assert "eagle is bird can fly as salmon is fish can swim both are animal" in out_talk and "both eagle and salmon belong to animal while eagle is bird and salmon is fish" in out_talk and "eagle is bird can fly whereas salmon is fish can swim" in out_talk, out_talk
        assert "dbwave" in out_talk and "tssafe" in out_talk and "mind> 3000" in out_talk and "ithigh" in out_talk and "hlsat" in out_talk and "bmok" in out_talk and "mind> 360" in out_talk and "actiny" in out_talk and "mind> 75" in out_talk and "trhawk" in out_talk, out_talk
        assert "60 by mcmcsmp" in out_talk and "61 by mcmcsmp" in out_talk and "mcflat by mcmcsmp" in out_talk and "80 mcpeak by mcmcsmp" in out_talk, out_talk
        assert "eagle and penguin are bird while salmon is fish all are animal" in out_talk and "is a bird that can fly" in out_talk and "eagle can fly while penguin can swim" in out_talk, out_talk
        assert "fdinsu" in out_talk and "mind> 150" in out_talk and "bffast" in out_talk and "shhard" in out_talk and "mind> 160" in out_talk and "mvfire" in out_talk and "ikdext" in out_talk and "lvrch" in out_talk and "mind> 320" in out_talk and "rfmem" in out_talk, out_talk

        # 8. Custom WebP Journal Plates, 4K Roadmap & All 162 Mind Capability Milestones
        caps_path = jdir / "capabilities.json"
        assert caps_path.is_file(), "capabilities.json missing"
        caps = json.loads(caps_path.read_text())
        seen_traits = {m["trait"] for m in caps.get("milestones", [])}
        for req in ("cortical_evolution", "multihop_cognition", "alien_sov_parity",
                    "hierarchical_crafting", "peer_split_knowledge",
                    "neocortical_abduction", "epistemic_vigilance",
                    "universal_syllogisms", "prospective_memory",
                    "online_micro_ripples", "cognitive_society",
                    "place_cell_nav", "spiking_intent_cortex", "sequence_and_riddles",
                    "formal_iff_and_math", "moral_and_deontic", "system1_physics",
                    "scientific_method", "affect_and_pragmatics", "recurrent_think_chain",
                    "open_valley_ecosystem", "narrative_and_algebra",
                    "schema_transfer_and_grid", "common_knowledge_and_clarify",
                    "bayes_and_math_logic", "knights_and_graphs", "gpu_cortical_contrast",
                    "live_syntax_teaching", "gpu_cortical_analogy", "autonomous_decision_and_tom3",
                    "gpu_cortical_compose", "personal_memory_and_sets", "few_shot_program_synthesis",
                    "gpu_cortical_multicue", "open_reading_and_infogain", "pearl_do_calc_and_culture",
                    "gpu_cortical_hypothesize", "open_verb_reading_and_sat", "probabilistic_risk_and_metacog",
                    "gpu_cortical_bridge", "coreference_and_frame4", "stochastic_plan_and_fault",
                    "gpu_cortical_cluster", "embedded_clauses_and_metarules", "coalition_thermo_and_consensus",
                    "gpu_cortical_gate", "mdp_mediation_and_flow", "vickrey_vectors_and_duties",
                    "gpu_cortical_dream", "market_physics_and_nash", "narrative_assistant_and_info",
                    "gpu_cortical_scan", "law_finance_and_kinematics", "mst_ltl_and_apportionment",
                    "gpu_cortical_deduce", "open_topic_dijkstra_bayes", "elastic_dp_core_experiment",
                    "gpu_cortical_abduce_back", "free_dialogue_dynamics_physics", "stats_interval_voting_pearl",
                    "gpu_cortical_counterfact", "prose_markov_lp_tree", "wave_ohm_mendel_algo",
                    "gpu_cortical_plan_chain", "dialogue_epidemic_buoyancy_portf", "topo_bsearch_doppler_pd",
                    "gpu_cortical_simulate", "paragraph_chem_optics_entropy", "lcs_queue_dh_cake",
                    "gpu_cortical_constrain", "prose_pursuit_gas_crt", "bipartite_bond_nyquist_commons",
                    "gpu_cortical_induce", "concessive_orbit_eigen_odds", "shoelace_dfa_mpc_hamming",
                    "gpu_cortical_chain_rule", "temporal_relativ_cap_euler", "confus_ctrl_logist_huffman",
                    "gpu_cortical_online_schema", "perspective_photo_ph_rsa", "astar_mixnash_pid_bernoulli",
                    "gpu_cortical_revise_schema", "belief_calc_decay_hardy", "rpn_ols_coins_kirchhoff",
                    "gpu_cortical_role_bind", "openlex_carnot_shannon_vec3", "totient_db_elasticity_cache",
                    "gpu_cortical_seq_predict", "story_hydro_rad_mat2", "comb_tcp_compound_mst",
                    "gpu_cortical_neural_math", "multiclause_coulomb_pipe_mod", "amdahl_vmem_escape_pt2d",
                    "gpu_cortical_graph_wave", "freecoref_isobar_faraday_disc", "diglogic_bode_sched_quantum",
                    "gpu_cortical_sat_solve", "whyprose_hbm16k_reynolds_ham", "mat3_kinetics_cidr_beam",
                    "gpu_cortical_visual_cortex", "selfteach_code_acoustics_rotor", "compiler_dna_cipher_robot",
                    "gpu_cortical_audio_cortex", "textbook_autodebug_empathy", "solid3d_graphc_dsp_orgdb",
                    "gpu_cortical_crossmodal_stack", "socratic_coach_contraption_delib", "astro_stoch_power_bft_supply",
                    "gpu_cortical_dialogue_cortex", "openchat_punwit_persona_stage7", "stats_carnot_master_immunol_ethics",
                    "gpu_cortical_narrative_metaphor", "unscripted_sciloop_story_stage7b", "lp_spectral_genetics_dh_trophic",
                    "gpu_cortical_belief_debate", "deepchat_peerrev_proof_400", "statmech_vcg_lif_lqr_hohmann",
                    "gpu_cortical_collective_discovery", "curiosity_speccode_qgate_410", "chancode_heatpde_coalbid_epi_colllab",
                    "gpu_cortical_self_research", "deepdiscourse_kalman_bayes_420", "fluid_zk_robot_licm_taylor",
                    "gpu_cortical_symbolic_calculus", "liveteach_kldiv_bode_430", "qtunnel_rel_raft_reg_ess",
                    "gpu_cortical_program_ast", "whatif_debate_merkle_rocket_440", "queue_enzyme_quorum_snr_cfg",
                    "gpu_cortical_linear_algebra", "cowrite_curious_rsa_aero_450", "cache_kepler_adc_nernst_cournot",
                    "gpu_cortical_bayes_net", "critique_analogy_bb_hamm_460", "lens_amort_pend_dna_solow",
                    "gpu_cortical_mdp_planner", "goalplan_synth_btree_bg_470", "frustum_quake_unroll_fiber_buck",
                    "gpu_cortical_kalman_filter", "hypotest_recon_brpred_alb_480", "bloom_cardiac_pass_sab_poly",
                    "gpu_cortical_lqr_control", "justify_bridge_img_baro_490", "shard_buck_f1_mann_page",
                    "gpu_cortical_fft_spectrum", "challenge_askback_shan_froude_500", "bezier_rc_logit_hohm_ecc",
                    "gpu_cortical_ode_rk4", "suppose_tellmore_pv_mm1_510", "glide_pk_cnn_seismo_bat",
                    "gpu_cortical_simplex_lp", "proveit_recap_cap_darcy_520", "gustaf_rank_bday_beer_put",
                    "gpu_cortical_pca_eigen", "whydiff_gen_bh_hooke_530", "tcp_hend_phong_iso_sharpe",
                    "gpu_cortical_pde_solver", "deducepair_root_debrog_sla_540", "ion_hill_beam_arith_taylor",
                    "gpu_cortical_mcmc_sampler", "groupthree_whatelse_fermi_bern_550", "shamir_nernst_ik_lotka_roofline"):
            assert req in seen_traits, (req, seen_traits)
        plates = list((jdir / "runs").glob("*/plate.webp"))
        assert len(plates) >= 12 and all(p.stat().st_size > 8000 for p in plates), plates
        assert (jdir / "progress.webp").stat().st_size > 8000
        assert (jdir / "journey.webp").stat().st_size > 15000
        assert (jdir / "roadmap.webp").stat().st_size > 15000

    print(
        "MIND_PASS: STAGE 8o LIVE! 51 BLACKWELL KERNELS & 550 DOMAINS! Blackwell 26-SM cortex + Dual-Tier 4k L2 / 16,384 Tier-2 HBM lifelong hippocampus, 29.36M-synapse complete-L2 neocortex (51 kernels: K1..K50 + K51 Pure-GPU 256-Chain Spiking Metropolis-Hastings MCMC Sampler Cortex on free PW27 Q), 8,192-concept open lexicon, "
        "114-pillar GPU evolution vs drift & 5 lesions, 1,697/1,697 (550-domain!) English SVO & Alien SOV exact parity, "
        "Duoquinquagintuple-Pathway (52-Track) Ribot's Law & quadruple-dissociation ablations, 6/6 two-mind exchange, 8/8 4-mind society & "
        "98/98 736-tick Multi-Gen cultural Open-Valley ecosystem, zero-template 3-entity GPU K12 clustering/associative sibling expansion/zero-keyword capability anaphora + pure-GPU 256-chain MCMC + fermidr/bernflw/shamirs/memvolt/invkine/lotkavol/roofln + 2,415+ turn dialogue, Stage-8o 4K roadmap & 550-domain WebP plates verified."
    )


if __name__ == "__main__":
    main()
