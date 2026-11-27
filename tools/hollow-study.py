#!/usr/bin/env python3
"""Breed spatial laws, transplant the untrained organs, and conduct reciprocal natural-selection trials."""

import argparse
import csv
import os
import shutil
import subprocess
import tempfile
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SEEDS = [7717, 16339, 40213, 91807]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("study", choices=("nursery", "watershed", "contest"))
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--seeds", nargs="+", type=int, default=SEEDS)
    p.add_argument("--population", type=int, default=2048)
    p.add_argument("--seasons", type=int, default=320)
    p.add_argument("--trials", type=int, default=2048)
    p.add_argument("--horizons", nargs="+", type=int, default=[256, 1024, 4096])
    p.add_argument(
        "--germs", type=Path, help="A nursery habitat or a directory containing earned-SEED.flood"
    )
    p.add_argument(
        "--contest-arms",
        nargs="+",
        choices=("paired", "reciprocal", "sham", "crossed", "reciprocal-crossed"),
        default=["paired", "reciprocal", "sham"],
    )
    p.add_argument(
        "--source",
        type=Path,
        help="Use an occupied watershed; omit for independently seeded sites and bodies",
    )
    p.add_argument("--output", type=Path, required=True)
    p.add_argument(
        "--germ-directory",
        type=Path,
        help="Retain each independently bred earned cohort for reciprocal releases",
    )
    p.add_argument(
        "--keep",
        type=Path,
        help="Copy the final free, unlabelled watershed or first earned nursery to this path",
    )
    a = p.parse_args()
    if sorted(set(a.horizons)) != a.horizons or min(a.horizons) < 1:
        p.error("horizons must be increasing and positive")
    if a.study != "nursery" and not a.germs:
        p.error("field releases require a source of spatial germlines")
    if a.keep and a.study == "contest":
        p.error("--keep requires a free watershed or a nursery")
    a.output.mkdir(parents=True, exist_ok=True)
    if a.germ_directory:
        a.germ_directory.mkdir(parents=True, exist_ok=True)
    rows = defaultdict(list)

    def launch(seed, arm, age, *args):
        result = subprocess.run(
            list(map(str, [a.axion.resolve(), "floodplain", *args])), capture_output=True, text=True, check=False
        )
        if result.returncode:
            raise RuntimeError(result.stderr + result.stdout[-1400:])
        for line in result.stdout.splitlines():
            if not line.startswith(
                ("hollow_", "flood_tick=", "flood_census=", "flood_seconds=", "floodplain=")
            ):
                continue
            row = dict(z.split("=", 1) for z in line.split() if "=" in z)
            tag = line.partition("=")[0]
            if (
                tag == "flood_tick"
                and rows[tag]
                and rows[tag][-1].get("founding_seed") == seed
                and rows[tag][-1].get("arm") == arm
                and rows[tag][-1].get("age") == age
            ):
                rows[tag].pop()
            rows[tag].append(dict(row, founding_seed=seed, arm=arm, age=age))
        for tag, records in rows.items():
            keys = list(dict.fromkeys(k for row in records for k in row))
            with (a.output / f"{tag}.csv").open("w", newline="") as f:
                w = csv.DictWriter(f, keys)
                w.writeheader()
                w.writerows(records)
        print(seed, arm, age, flush=True)

    with tempfile.TemporaryDirectory(
        prefix="axion-creeks-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as tmp:
        d = Path(tmp)
        for seed in a.seeds:
            if a.study == "nursery":
                for arm in ("earned", "uniform"):
                    target = d / "reared.flood"
                    launch(
                        seed,
                        arm,
                        a.seasons,
                        "--load",
                        ROOT / "examples/reedbed.flood",
                        "--hollows",
                        "live",
                        "--hollow-slots",
                        8,
                        "--hollow-optics",
                        ROOT / "examples/wheel-reader.mind",
                        "--hollow-rear",
                        a.seasons,
                        "--hollow-population",
                        a.population,
                        "--hollow-seed",
                        seed,
                        "--hollow-brood",
                        arm,
                        "--hollow-study",
                        a.trials,
                        "--steps",
                        0,
                        "--save",
                        target,
                    )
                    if a.germ_directory and arm == "earned":
                        shutil.copyfile(target, a.germ_directory / f"earned-{seed}.flood")
                    if a.keep and arm == "earned" and seed == a.seeds[0]:
                        shutil.copyfile(target, a.keep)
                continue
            source = (
                ["--load", a.source.resolve()]
                if a.source
                else [
                    "--seed",
                    seed,
                    "--side",
                    64,
                    "--neurons",
                    128,
                    "--density",
                    72,
                    "--soil",
                    ROOT / "examples/reedbed.flood",
                    "--siphon-germs",
                    ROOT / "examples/reedbed.flood",
                    "--press",
                    "live",
                    "--clay-reach",
                    "shared",
                    "--press-geology",
                    "tidal",
                ]
            )
            donors = a.germs / f"earned-{seed}.flood" if a.germs.is_dir() else a.germs
            base = d / "opening.flood"
            launch(
                seed,
                "opening",
                0,
                *source,
                "--hollow-germs",
                donors,
                "--hollows",
                "live",
                "--hollow-ground",
                "rich",
                "--hollow-seed",
                seed,
                "--steps",
                0,
                "--save",
                base,
            )
            arms = (
                [(arm, ["--hollow-contest", arm]) for arm in a.contest_arms]
                if a.study == "contest"
                else (
                    ("active", []),
                    ("unguided", ["--hollows", "unguided"]),
                    ("rotated", ["--hollow-odometry", "rotated"]),
                    ("hidden", ["--hollow-landmarks", "covered"]),
                    ("sparse", ["--hollow-ground", "sparse"]),
                )
            )
            for arm, options in arms:
                old = base
                previous = 0
                times = [0, *a.horizons] if a.study == "contest" else a.horizons
                for j, t in enumerate(times):
                    target = d / f"{arm}-{j % 2}.flood"
                    launch(
                        seed,
                        arm,
                        t,
                        "--load",
                        old,
                        "--hollow-seed",
                        seed,
                        "--hollow-current",
                        "ferry",
                        *options,
                        "--hollow-census",
                        "--steps",
                        t - previous,
                        "--save",
                        target,
                    )
                    old = target
                    previous = t
                if a.keep and arm == "active" and seed == a.seeds[0]:
                    shutil.copyfile(old, a.keep)


if __name__ == "__main__":
    main()
