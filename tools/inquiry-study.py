#!/usr/bin/env python3
"""Rear a selective gaze circuit, breed real descendants, teach a pupil and test question-driven investigations on CUDA."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
EX = ROOT / "examples"
BEDS = ROOT / "docs/observational-learning"


def rows(log, kind):
    return [
        dict(s.split("=", 1) for s in line.split() if "=" in s)
        for line in log.splitlines()
        if line.startswith(kind + "=")
    ]


def reproduce(out, seed, selection):
    binary = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()

    def run(tag, args, script=""):
        p = subprocess.run(
            list(map(str, [binary, *args])),
            cwd=ROOT,
            input=script,
            text=True,
            capture_output=True,
        )
        (out / (tag + ".log")).write_text(p.stdout + p.stderr)
        if p.returncode:
            raise RuntimeError(f"{tag} failed. See {out / (tag + '.log')}")
        return p.stdout

    result = {}
    with tempfile.TemporaryDirectory(prefix=".watershed-", dir=out) as temp:
        d = Path(temp)
        host = EX / "cascade-speaker.mind"
        with (d / "founders").open("wb") as f:
            subprocess.run(
                ["zstd", "-qdc", EX / "cascade-founders.flood.zst"],
                stdout=f,
                check=True,
            )
        brood = run(
            "nursery",
            [
                "floodplain",
                "--load",
                d / "founders",
                "--steps",
                0,
                "--retina-trials",
                128,
                "--retina-host",
                host,
                "--inlet-state",
                "carried",
                "--inlet-practice",
                64,
                "--inlet-prior",
                0,
                "--retina-gaze",
                "inherited",
                "--gaze-purse",
                48,
                "--retina-beds",
                4,
                "--inlet-landscape",
                "channels",
                "--retina-laps",
                7,
                "--retina-rear-laps",
                7,
                "--inlet-residents",
                8192,
                "--inlet-cohort",
                4096,
                "--inlet-generations",
                512,
                "--inlet-selection",
                selection,
                "--inlet-bolus",
                64,
                "--class-seed",
                seed,
                "--retina-germ",
                out / "iris.germ",
            ],
        )
        result["brood"] = rows(brood, "gaze_metabolism")[-1]
        if not (out / "iris.germ").exists():
            return result
        run(
            "eyes",
            [
                "floodplain",
                "--load",
                d / "founders",
                "--steps",
                0,
                "--receive-count",
                16384,
                "--retina-donor",
                EX / "inquiry-optics.germ",
                "--save",
                d / "sighted",
            ],
        )
        field = run(
            "field",
            [
                "floodplain",
                "--load",
                d / "sighted",
                "--receive-count",
                16384,
                "--retina-donor",
                out / "iris.germ",
                "--daylight",
                "live",
                "--daylight-period",
                4,
                "--daylight-gaze",
                "inherited",
                "--daylight-probes",
                "stirred",
                "--daylight-banks",
                "tidal",
                "--shore-depth",
                "open",
                "--drift-windings",
                "woven",
                "--drift-sight",
                "audience",
                "--steps",
                4096,
                "--report-every",
                2048,
                "--probe-trace",
                out / "grains.tsv",
                "--retina-trials",
                48,
                "--retina-born-after",
                1024,
                "--retina-beds",
                2,
                "--retina-works",
                BEDS / "estuary-beds.txt",
                "--retina-order",
                "woven",
                "--retina-probes",
                "chosen",
                "--gaze-purse",
                32,
                "--inlet-state",
                "carried",
                "--inlet-practice",
                48,
                "--inlet-prior",
                0,
                "--inlet-residents",
                8192,
                "--class-seed",
                674671,
                "--save",
                d / "colony",
            ],
        )
        result["field"] = rows(field, "field_soundings")[-1]
        candidates = rows(field, "retinal_panel")
        if not candidates:
            return result
        slot = int(candidates[0]["slot"])
        young = out / "young.mind"
        spoken = out / "speaker.mind"
        extraction = run(
            "extraction",
            [
                "floodplain",
                "--load",
                d / "colony",
                "--slot",
                slot,
                "--steps",
                0,
                "--export-mind",
                young,
            ],
        )
        result["individual"] = rows(extraction, "field_extraction")[-1]
        run(
            "english",
            [
                "talk",
                "--speaker",
                young,
                "--listener",
                EX / "parallax-witness.mind",
                "--save-speaker",
                d / "taught",
            ],
            (EX / "inlet-lessons.txt").read_text(),
        )
        run(
            "return",
            [
                "floodplain",
                "--load",
                d / "colony",
                "--steps",
                0,
                "--follow-witness",
                young,
                "--return-lesson",
                young,
                d / "taught",
                "--export-mind",
                spoken,
                "--save",
                d / "home",
            ],
        )
        for mode in ("chosen", "visible-match", "crossed"):
            result[mode] = []
            for layout in (13603, 442709):
                tag = f"questions-{mode}-{layout}"
                q = run(
                    tag,
                    [
                        "floodplain",
                        "--load",
                        d / "home",
                        "--slot",
                        slot,
                        "--steps",
                        0,
                        "--retina-trials",
                        256,
                        "--retina-beds",
                        2,
                        "--retina-works",
                        BEDS / "coast-beds.txt",
                        "--retina-order",
                        "woven",
                        "--retina-gaze",
                        "passive",
                        "--retina-deliberate",
                        64,
                        "--inquiry-hand",
                        mode,
                        "--gaze-purse",
                        32,
                        "--inlet-state",
                        "carried",
                        "--inlet-practice",
                        0,
                        "--inlet-prior",
                        0,
                        "--class-seed",
                        layout,
                        "--retina-questions",
                        EX / "inlet-questions.txt",
                        "--retina-voice",
                        out / (tag + ".tsv"),
                    ],
                )
                result[mode].append(rows(q, "spoken_inquiry")[-1])
    return result


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--seed", type=int, default=184687)
    p.add_argument("--selection", choices=("food", "uniform"), default="food")
    a = p.parse_args()
    out = a.output.resolve()
    try:
        out.mkdir(parents=True)
    except FileExistsError:
        out = Path(tempfile.mkdtemp(prefix="inquiry-run-", dir=out))
    print(f"Study directory {out}", flush=True)
    report = json.dumps(reproduce(out, a.seed, a.selection), indent=2)
    (out / "study.json").write_text(report + "\n")
    print(report)
