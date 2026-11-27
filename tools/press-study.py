#!/usr/bin/env python3
"""Run matched, independently seeded pressure-tool nurseries or continuations of one occupied watershed."""

import argparse
import csv
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("study", choices=("nursery", "watershed"))
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--source", type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--seeds", type=int, nargs="+", default=[7841, 17333, 39181, 65063])
    p.add_argument("--seasons", type=int, default=192)
    p.add_argument("--capacity", type=int, default=1024)
    p.add_argument("--trials", type=int, default=4096)
    p.add_argument(
        "--broods",
        nargs="+",
        choices=("paid", "uniform", "finite"),
        default=("paid", "uniform"),
    )
    p.add_argument("--practice", choices=("observed", "mixed"), default="observed")
    p.add_argument("--founders", choices=("varied", "residents"), default="varied")
    p.add_argument("--horizons", type=int, nargs="+", default=[256, 2048, 8192, 24576])
    p.add_argument(
        "--arms",
        nargs="+",
        default=["mixed", "veins", "tidal", "unguided", "alone", "no-casting", "still"],
    )
    p.add_argument(
        "--keep", type=Path, help="Keep the last tidal watershed in this file"
    )
    a = p.parse_args()
    if min(a.horizons) < 1 or a.horizons != sorted(set(a.horizons)):
        p.error("horizons must be increasing positive integers")
    if any(
        k not in ("mixed", "veins", "tidal", "unguided", "alone", "no-casting", "still")
        for k in a.arms
    ):
        p.error("unknown watershed arm")
    source = (
        a.source
        or ROOT
        / "examples"
        / ("reedbed.flood" if a.study == "nursery" else "headwaters.flood")
    ).resolve()
    a.output.mkdir(parents=True, exist_ok=True)
    tables = {}

    def run(labels, *args):
        cmd = list(map(str, [a.axion.resolve(), "floodplain", *args]))
        proc = subprocess.run(cmd, text=True, capture_output=True, check=False)
        if proc.returncode:
            raise RuntimeError(proc.stderr + proc.stdout[-1800:])
        for line in proc.stdout.splitlines():
            name = line.partition("=")[0]
            if not (
                name.startswith("press_")
                or name
                in (
                    "flood_tick",
                    "flood_seconds",
                    "clay_tick",
                    "flood_census",
                    "siphon_tick",
                )
            ):
                continue
            row = dict(labels)
            row.update(t.split("=", 1) for t in line.split() if "=" in t)
            tables.setdefault(name, []).append(row)
        for name, rows in tables.items():
            with (a.output / (name + ".csv")).open("w", newline="") as f:
                w = csv.DictWriter(
                    f,
                    fieldnames=dict.fromkeys(k for row in rows for k in row),
                    lineterminator="\n",
                )
                w.writeheader()
                w.writerows(rows)
        print(" ".join(f"{k}={v}" for k, v in labels.items()), flush=True)

    with tempfile.TemporaryDirectory(
        prefix="axion-print-study-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as temp:
        temp = Path(temp)
        options = [
            "--press-seasons",
            a.seasons,
            "--press-population",
            a.capacity,
            "--press-founders",
            a.founders,
            "--press-practice",
            a.practice,
        ]
        if a.study == "nursery":
            for seed in a.seeds:
                for brood in a.broods:
                    bank = temp / "children.flood"
                    run(
                        {"seed": seed, "brood": brood, "phase": "birth"},
                        "--load",
                        source,
                        "--press",
                        "live",
                        *options,
                        "--press-seed",
                        seed,
                        "--press-brood",
                        brood,
                        "--steps",
                        0,
                        "--save",
                        bank,
                    )
                    run(
                        {"seed": seed, "brood": brood, "phase": "heldout"},
                        "--load",
                        bank,
                        "--steps",
                        0,
                        "--press-study",
                        a.trials,
                        "--press-seed",
                        seed ^ 0x536F31,
                    )
            return
        seed = a.seeds[0]
        baseline = temp / "population.flood"
        run(
            {"seed": seed, "geology": "shared", "age": 0},
            "--load",
            source,
            "--press",
            "live",
            *options,
            "--press-seed",
            seed,
            "--steps",
            0,
            "--save",
            baseline,
        )
        for arm in a.arms:
            previous, origin = 0, baseline
            for age in a.horizons:
                bank = temp / f"{arm}-{age}.flood"
                geology = arm if arm in ("mixed", "veins") else "tidal"
                use = "live" if arm in ("mixed", "veins", "tidal") else arm
                run(
                    {"seed": seed, "geology": arm, "age": age},
                    "--load",
                    origin,
                    "--press",
                    use,
                    "--press-geology",
                    geology,
                    "--steps",
                    age - previous,
                    "--press-census",
                    "--press-study",
                    a.trials,
                    "--press-seed",
                    seed ^ 0x536F31,
                    "--save",
                    bank,
                )
                if origin != baseline:
                    origin.unlink()
                origin, previous = bank, age
            if arm == "tidal" and a.keep:
                import shutil

                shutil.copyfile(origin, a.keep)
            origin.unlink()


if __name__ == "__main__":
    main()
