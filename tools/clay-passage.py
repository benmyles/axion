#!/usr/bin/env python3
"""Release inexperienced hatchlings onto physical objects left by two independent speaking populations."""

import argparse
import csv
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--axion", type=Path, default=ROOT / "axion")
    parser.add_argument("--a", type=Path, required=True)
    parser.add_argument("--b", type=Path, required=True)
    parser.add_argument("--genes", type=Path, required=True)
    parser.add_argument("--cortex", type=Path, required=True)
    parser.add_argument("--peers", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seeds", nargs="+", type=int, default=[3911, 11621, 53987, 94007])
    parser.add_argument("--ages", nargs="+", type=int, default=[16, 64, 256])
    parser.add_argument("--reach", choices=("single", "rake", "shared"), default="single")
    parser.add_argument("--side", type=int, default=64)
    opt = parser.parse_args()
    opt.output.mkdir(parents=True, exist_ok=True)
    if opt.ages != sorted(set(opt.ages)) or min(opt.ages) < 1:
        parser.error("use increasing positive ages")
    table = {}
    with tempfile.TemporaryDirectory(prefix="clay-cross-foster-") as folder:
        for seed in opt.seeds:
            for source, field in [("a", opt.a), ("b", opt.b)]:
                for treatment in ["lived", "permuted", "smoothed", "no_hand_practice"]:
                    previous = 0
                    older = None
                    for age in opt.ages:
                        path = Path(folder) / f"{source}-{seed}-{treatment}-{age}.flood"
                        if older is not None:
                            flags = ["--load", older]
                        else:
                            flags = [
                                "--seed",
                                seed,
                                "--side",
                                opt.side,
                                "--neurons",
                                128,
                                "--density",
                                72,
                                "--soil",
                                field.resolve(),
                                "--siphon-germs",
                                opt.genes.resolve(),
                                "--clay-germs",
                                opt.cortex.resolve(),
                                "--clay-hands",
                                "read",
                                "--clay-reach",
                                opt.reach,
                                "--clay-sound",
                                "ancestral-permuted" if treatment == "permuted" else "ancestral",
                                "--clay-context",
                                "pooled",
                                "--clay-muscle",
                                "quiet" if treatment == "no_hand_practice" else "live",
                                "--siphon-workers",
                                "still",
                                "--siphon-audience",
                                "alone",
                            ]
                            if treatment == "smoothed":
                                flags += ["--wash-clay"]
                        command = [
                            opt.axion.resolve(),
                            "floodplain",
                            *flags,
                            "--steps",
                            age - previous,
                            "--clay-peers",
                            opt.peers,
                            "--siphon-trials",
                            16,
                            "--work-subjects",
                            32,
                            "--work-test-seed",
                            seed + 140357,
                            "--save",
                            path,
                        ]
                        proc = subprocess.run(list(map(str, command)), capture_output=True, text=True)
                        if proc.returncode:
                            raise RuntimeError(proc.stderr + proc.stdout[-1500:])
                        for line in proc.stdout.splitlines():
                            name = line.partition("=")[0]
                            if name not in ["clay_peer", "clay_tick", "siphon_tick", "flood_seconds"]:
                                continue
                            row = dict(soil=source, intervention=treatment, seed=seed, age_ticks=age)
                            row.update(t.split("=", 1) for t in line.split() if "=" in t)
                            table.setdefault(name, []).append(row)
                        if older is not None:
                            older.unlink()
                        previous = age
                        older = path
                    if older is not None:
                        older.unlink()
                    print(seed, source, treatment, flush=True)
                    for name, rows in table.items():
                        with (opt.output / (name + ".csv")).open("w", newline="") as f:
                            writer = csv.DictWriter(
                                f, fieldnames=dict.fromkeys(k for r in rows for k in r), lineterminator="\n"
                            )
                            writer.writeheader()
                            writer.writerows(rows)


if __name__ == "__main__":
    main()
