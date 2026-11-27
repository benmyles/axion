#!/usr/bin/env python3
"""Breed context gates under two reproductive regimes and score untouched test communities."""

import argparse
import csv
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--input", type=Path, default=ROOT / "examples/reedbed.flood")
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--seeds", nargs="+", type=int, default=[96523, 166099, 230311, 344629])
    p.add_argument("--generations", type=int, default=256)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    rows = {}
    for seed in a.seeds:
        for regime in ["provisioned", "uniform"]:
            cmd = [
                a.axion.resolve(),
                "floodplain",
                "--load",
                a.input.resolve(),
                "--pottery",
                "sheltered",
                "--clay-rear",
                a.generations,
                "--clay-seed",
                seed,
                "--work-capacity",
                2048,
                "--clay-selection",
                regime,
                "--clay-classes",
                256,
                "--clay-preparation",
                256,
                "--clay-history",
                256,
                "--clay-lessons",
                96,
                "--clay-taps",
                1,
                "--clay-test",
                16,
                "--clay-foster",
                4,
                "--clay-reversals",
                192,
                "--work-subjects",
                512,
                "--work-test-seed",
                seed + 1319009,
                "--steps",
                0,
            ]
            proc = subprocess.run(list(map(str, cmd)), capture_output=True, text=True)
            if proc.returncode:
                raise RuntimeError(proc.stderr + proc.stdout[-500:])
            for line in proc.stdout.splitlines():
                name = line.partition("=")[0]
                if name not in ("clay_brood", "clay_transfer", "clay_generation", "clay_reversal", "clay_classes"):
                    continue
                q = dict(nursery_seed=seed, parenthood=regime)
                q.update(t.split("=", 1) for t in line.split() if "=" in t)
                rows.setdefault(name, []).append(q)
            for name, all_rows in rows.items():
                with (a.output / (name + ".csv")).open("w", newline="") as f:
                    w = csv.DictWriter(
                        f, fieldnames=dict.fromkeys(k for r in all_rows for k in r), lineterminator="\n"
                    )
                    w.writeheader()
                    w.writerows(all_rows)
            print(seed, regime, flush=True)


if __name__ == "__main__":
    main()
