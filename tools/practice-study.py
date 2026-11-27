#!/usr/bin/env python3
"""Replay matched evolutionary, capillary and field interventions on Axion's CUDA backend."""

import argparse
import csv
import os
import subprocess
import tempfile
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "study", choices=("nursery", "field", "competition", "canals", "cut", "course")
    )
    p.add_argument("output", type=Path)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--source", type=Path)
    p.add_argument("--germs", type=Path)
    p.add_argument("--seeds", nargs="+", type=int, default=[90553, 53129, 78059])
    p.add_argument("--ticks", type=int, default=2048)
    p.add_argument("--seasons", type=int, default=256)
    p.add_argument("--population", type=int, default=2048)
    p.add_argument("--cases", type=int, default=1024)
    p.add_argument("--landscape", choices=("island", "frontier"), default="island")
    p.add_argument("--save", action="store_true")
    o = p.parse_args()
    o.output.mkdir(parents=True, exist_ok=True)
    results = defaultdict(list)

    def run(label, seed, *flags):
        cmd = [str(x) for x in (o.axion.resolve(), "floodplain", *flags)]
        r = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if r.returncode:
            raise RuntimeError(r.stderr + r.stdout[-1500:])
        for line in r.stdout.splitlines():
            key = line.partition("=")[0]
            if (
                not key.startswith(("practice_", "wick_"))
                and key
                not in (
                    "flood_seconds",
                    "flood_census",
                    "spiking_growth",
                    "linkage_field",
                )
            ) or key.endswith("_ground"):
                continue
            row = dict(t.split("=", 1) for t in line.split() if "=" in t)
            results[key].append(dict(experiment=label, origin_seed=seed, **row))
        for k, rows in results.items():
            with (o.output / (k + ".csv")).open("w", newline="") as f:
                writer = csv.DictWriter(
                    f,
                    list(dict.fromkeys(x for row in rows for x in row)),
                    lineterminator="\n",
                )
                writer.writeheader()
                writer.writerows(rows)
        print(label, seed, flush=True)

    with tempfile.TemporaryDirectory(
        prefix="axion-observation-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as w:
        opening = Path(w) / "opening.flood"
        for seed in o.seeds:

            def save(arm, seed=seed):
                return ["--save", o.output / f"{arm}-{seed}.flood"] if o.save else []

            if o.study == "nursery":
                for habitat in ("fixtures", "mixed", "foraging"):
                    for brood in ("earned", "uniform"):
                        tag = f"{habitat}-{brood}"
                        g = [] if not o.germs else ["--practice-germs", o.germs]
                        run(
                            tag,
                            seed,
                            "--load",
                            o.source or ROOT / "examples/spindlebank.flood",
                            "--practice",
                            "live",
                            *g,
                            "--practice-habitat",
                            habitat,
                            "--practice-landscape",
                            o.landscape,
                            "--practice-population",
                            o.population,
                            "--practice-rear",
                            o.seasons,
                            "--practice-brood",
                            brood,
                            "--practice-seed",
                            seed,
                            "--practice-study",
                            o.cases,
                            "--linkage-pieces",
                            4,
                            "--steps",
                            0,
                            *save(tag),
                        )
            elif o.study == "course":
                for length in (8, 24, 64):
                    run(
                        f"course-{length}",
                        seed,
                        "--load",
                        o.source or ROOT / "examples/spindlebank.flood",
                        "--wicks",
                        "flowing",
                        "--wick-study",
                        length,
                        "--wick-seed",
                        seed,
                        "--steps",
                        0,
                    )
            elif o.study == "cut":
                if not o.source:
                    p.error(
                        "choose a watershed with attributed public work for the removal intervention"
                    )
                for arm in ("retained", "removed"):
                    run(
                        arm,
                        seed,
                        "--load",
                        o.source,
                        *(["--unthread-works"] if arm == "removed" else []),
                        "--practice-census",
                        "--steps",
                        o.ticks,
                        *save(arm),
                    )
            else:
                start = o.source or ROOT / "examples/millrace.flood"
                args = ["--practice", "live", "--practice-seed", seed]
                if o.germs:
                    args += ["--practice-germs", o.germs]
                if o.study == "canals":
                    args += ["--wicks", "flowing", "--wick-seed", seed]
                run(
                    "opening",
                    seed,
                    "--load",
                    start,
                    *args,
                    "--steps",
                    0,
                    "--save",
                    opening,
                )
                interventions = {
                    "live": [],
                    "scattered": ["--practice", "scattered"],
                    "random": ["--practice-probes", "random"],
                    "chance": ["--practice-work", "chance"],
                    "carried": ["--practice-fibre", "carried"],
                }
                if o.study == "competition":
                    interventions = {
                        tag: ["--practice-contest", tag, "--practice-seed", seed]
                        for tag in ("paired", "reciprocal", "sham")
                    }
                if o.study == "canals":
                    interventions = {
                        tag: ["--wicks", tag, "--wick-census"]
                        for tag in ("flowing", "confined", "still")
                    }
                for arm, settings in interventions.items():
                    run(
                        arm,
                        seed,
                        "--load",
                        opening,
                        *settings,
                        "--steps",
                        o.ticks,
                        "--practice-census",
                        *save(arm),
                    )


if __name__ == "__main__":
    main()
