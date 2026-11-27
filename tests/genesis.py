#!/usr/bin/env python3
"""Automated verification suite for Axion Genesis (Blackwell SM 12.0 embodied ALife & CA-DNA SNN)."""
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
AXION = str(ROOT / "axion")


def parse_kv(line: str) -> dict[str, str]:
    return dict(re.findall(r'([^\s=]+)=("[^"]*"|\S+)', line))


def run_axion(args: list[str], journal_dir: Path) -> str:
    env = dict(os.environ, AXION_JOURNAL_DIR=str(journal_dir))
    proc = subprocess.run([AXION, *args], cwd=ROOT, env=env, capture_output=True, text=True, check=True)
    return proc.stdout


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="axion-genesis-test-") as td:
        tdir = Path(td)
        jdir = tdir / "journal"
        ckpt = tdir / "colony.gen"

        # 1. Blackwell SM 12.0 Hardware Benchmark: zero register spill, 4 B/synapse, fast throughput
        out_bench = run_axion(["genesis", "bench", "--pop", "512", "--substeps", "128", "--repeats", "3"], jdir)
        b_rows = {parse_kv(l)["genesis_bench"]: parse_kv(l) for l in out_bench.splitlines() if l.startswith("genesis_bench=")}
        for k in ("snn_inference", "snn_plastic", "world_physics", "ca_morphogenesis", "coupled_alife"):
            assert k in b_rows, f"missing benchmark line {k}\n{out_bench}"

        inf = b_rows["snn_inference"]
        pla = b_rows["snn_plastic"]
        wld = b_rows["world_physics"]
        dev = b_rows["ca_morphogenesis"]
        cpl = b_rows["coupled_alife"]

        assert int(inf["bytes_per_synapse"]) == 4, f"expected 4 B/synapse, got {inf}"
        assert int(inf["local_B"]) == 0, f"non-zero register spill in SNN inference: {inf}"
        assert int(pla["local_B"]) == 0, f"non-zero register spill in SNN plastic: {pla}"
        assert int(wld["local_B"]) == 0, f"non-zero register spill in world physics: {wld}"
        assert int(dev["local_B"]) == 0, f"non-zero register spill in CA morphogenesis: {dev}"
        assert int(cpl["brain_local_B"]) == 0, f"non-zero register spill in multi-area brain kernel: {cpl}"
        assert float(inf["Gsyn_ops/s"]) > 10.0, f"unexpectedly low SNN inference throughput: {inf}"
        assert float(pla["Gsyn_ops/s"]) > 2.0, f"unexpectedly low plastic SNN throughput: {pla}"
        assert int(pla["edits"]) > 0, f"expected active plastic edits in bench: {pla}"
        assert float(wld["Mcells/s"]) > 20.0, f"unexpectedly slow world physics: {wld}"
        assert float(cpl["ticks/s"]) > 50.0, f"unexpectedly slow coupled ALife: {cpl}"

        # Verify single-plate journal output (ONLY plate.webp, NO creature-card.webp spam!)
        runs = sorted((jdir / "runs").glob("*-genesis"))
        assert len(runs) == 1, f"expected 1 genesis journal entry, got {runs}"
        assert (runs[0] / "plate.webp").stat().st_size > 15000, "bench plate.webp too small"
        assert not (runs[0] / "creature-card.webp").exists(), "genesis should produce ONLY single plate.webp"

        # 2. Embodied Evolution + Paired Drift & Physical Lesion Controls + Checkpoint Save
        out_evo = run_axion(
            ["genesis", "evolve", "--pop", "256", "--ticks", "600", "--seed", "77", "--controls", "--save", str(ckpt)],
            jdir,
        )
        sum_lines = [parse_kv(l) for l in out_evo.splitlines() if l.startswith("genesis_summary=")]
        assert sum_lines, f"missing genesis_summary\n{out_evo}"
        s = sum_lines[-1]
        assert int(s["alive"]) >= 16, f"population collapsed: {s}"
        assert int(s["births"]) >= 20, f"expected real natural births: {s}"
        assert int(s["max_gen"]) >= 2, f"expected multi-generational evolution: {s}"
        assert int(s["species"]) >= 4, f"expected morphological diversity: {s}"
        assert int(s["spikes"]) > 100000, f"expected real spiking activity: {s}"
        assert int(s["edits"]) > 1000, f"expected real within-lifetime STDP edits: {s}"
        assert any(l.startswith("genesis_map_row=0") for l in out_evo.splitlines()), "missing world map snapshot"
        assert any(l.startswith("genesis_embryo=step0") for l in out_evo.splitlines()), "missing embryogenesis trace"
        assert any(l.startswith("genesis_brain_nodes=") for l in out_evo.splitlines()), "missing brain architecture trace"

        ctrl_rows = {parse_kv(l)["genesis_control"]: parse_kv(l) for l in out_evo.splitlines() if l.startswith("genesis_control=")}
        for arm in ("evolved_intact", "evolved_frozen", "evolved_no_apical", "evolved_zero_delay", "evolved_no_neuromod", "evolved_blind", "evolved_deaf", "drift_unselected"):
            assert arm in ctrl_rows, f"missing control arm {arm}"
        assert int(ctrl_rows["evolved_frozen"]["edits"]) == 0, f"frozen arm must have 0 plastic edits: {ctrl_rows['evolved_frozen']}"
        assert int(ctrl_rows["evolved_intact"]["edits"]) > 0, f"intact arm must have >0 plastic edits: {ctrl_rows['evolved_intact']}"
        assert ckpt.is_file() and ckpt.stat().st_size > 1024, "checkpoint not written"

        # 3. Continuous Multi-Epoch --load Resume + Inspect Round-Trip
        ckpt2 = tdir / "colony_epoch2.gen"
        out_res = run_axion(
            ["genesis", "evolve", "--load", str(ckpt), "--pop", "256", "--ticks", "300", "--seed", "79", "--challenge", "1", "--save", str(ckpt2)],
            jdir,
        )
        res_sum = [parse_kv(l) for l in out_res.splitlines() if l.startswith("genesis_summary=")]
        assert res_sum and int(res_sum[-1]["max_gen"]) >= int(s["max_gen"]), f"resume did not advance generations: {res_sum}"

        out_insp = run_axion(["genesis", "inspect", "--load", str(ckpt2)], jdir)
        insp_lines = [parse_kv(l) for l in out_insp.splitlines() if l.startswith("genesis_inspect=")]
        assert insp_lines and int(insp_lines[-1]["alive"]) == int(res_sum[-1]["alive"]), f"inspect mismatch: {insp_lines} vs {res_sum}"
        assert any(l.startswith("genesis_embryo=step9") for l in out_insp.splitlines()), "inspect missing embryo"

        # 4. Non-LLM Grounded Cognitive Probe (10 Assays) & Live 3-Band Compositional Converse Interface
        out_probe = run_axion(["genesis", "probe", "--load", str(ckpt2), "--ticks", "240", "--seed", "101"], jdir)
        prows = {parse_kv(l)["genesis_probe"]: parse_kv(l) for l in out_probe.splitlines() if l.startswith("genesis_probe=")}
        for pk in ("reversal_learning", "working_memory", "agency_language", "acoustic_dialogue_learning", "temporal_sequence_learning", "compositional_3band_binding", "predictive_mmn", "zeroshot_compositional_transfer", "vocal_qa_grounding"):
            assert pk in prows, f"missing probe row {pk}\n{out_probe}"

        out_conv = run_axion(["genesis", "converse", "--load", str(ckpt2), "--group", "2", "--script", "v0,v2,ask_01,teach_qa_01_v2,ask_10,teach_qa_10_v0"], jdir)
        assert any(l.startswith("genesis_converse=ready") for l in out_conv.splitlines()), f"converse not ready:\n{out_conv}"
        assert len([l for l in out_conv.splitlines() if l.startswith("genesis_turn=")]) == 6, f"unexpected converse turns:\n{out_conv}"

        runs = sorted((jdir / "runs").glob("*-genesis"))
        assert len(runs) == 6, f"expected 6 genesis journal entries, got {len(runs)}"
        for rdir in runs:
            assert (rdir / "plate.webp").stat().st_size > 15000, f"plate too small in {rdir}"
            assert not (rdir / "creature-card.webp").exists(), f"unexpected creature-card in {rdir}"
        assert (jdir / "latest.webp").stat().st_size > 15000, "missing latest.webp"

        print(
            f"OK genesis: infer={inf['Gsyn_ops/s']} GSynOps/s (spill={inf['local_B']}B), "
            f"plastic={pla['Gsyn_ops/s']} GSynOps/s, world={wld['step_us']}us, "
            f"coupled={cpl['ticks/s']} ticks/s, evo_births={s['births']}, max_gen={s['max_gen']}, "
            f"intact_E={ctrl_rows['evolved_intact']['energy']} vs drift_E={ctrl_rows['drift_unselected']['energy']}"
        )


if __name__ == "__main__":
    main()
