#!/usr/bin/env python3
"""Run reciprocal field releases, physical path legacies, and matched prospective-learning nurseries."""

import argparse
import csv
import os
import shutil
import subprocess
import tempfile
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SEEDS = (41011, 18211, 59707, 86027)
TAGS = (
    "effort_builders",
    "effort_paving",
    "effort_season",
    "effort_choice",
    "effort_receipt",
    "effort_slat",
    "effort_slab",
    "effort_field",
    "effort_random",
    "effort_lineage",
    "effort_appraisal",
    "effort_deploy",
    "spiking_growth",
    "flood_tick",
    "flood_census",
    "flood_seconds",
)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("phase", choices=("nursery", "field", "legacy", "contest", "feral"))
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--source", type=Path)
    p.add_argument("--germs", type=Path)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--keep", type=Path)
    p.add_argument("--germ-directory", type=Path)
    p.add_argument("--seeds", nargs="+", type=int, default=list(SEEDS))
    p.add_argument("--horizons", nargs="+", type=int, default=[256, 1024, 4096])
    p.add_argument("--seasons", type=int, default=512)
    p.add_argument("--capacity", type=int, default=4096)
    p.add_argument("--trials", type=int, default=4096)
    p.add_argument("--window", type=int, default=16)
    p.add_argument("--weather", type=int, default=256)
    p.add_argument("--movement", choices=("impulse", "select"), default="impulse")
    p.add_argument("--assignments", choices=("resample", "same"), default="resample")
    p.add_argument("--practice", choices=("trips", "mixed", "building"), default="mixed")
    p.add_argument("--arms", nargs="+", help="Subset of interventions in the chosen experiment")
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    if a.germ_directory:
        a.germ_directory.mkdir(parents=True, exist_ok=True)
    if a.horizons != sorted(set(a.horizons)) or min(a.horizons) < 1:
        p.error("ages must be positive and increasing")
    if a.phase not in ("nursery", "feral") and not a.germs:
        p.error("field experiments require a nursery source for the new organs")
    records = defaultdict(list)

    def run(seed, arm, t, *argv):
        call = subprocess.run(
            list(map(str, [a.axion.resolve(), "floodplain", *argv])),
            text=True,
            capture_output=True,
            check=False,
        )
        if call.returncode:
            raise RuntimeError(call.stderr + call.stdout[-1300:])
        local = defaultdict(list)
        for line in call.stdout.splitlines():
            tag = line.partition("=")[0]
            if tag in TAGS:
                row = dict(token.split("=", 1) for token in line.split() if "=" in token)
                local[tag].append(dict(row, founding_seed=seed, intervention=arm, age=t))
        for tag, rows in local.items():
            records[tag].extend(
                rows[-1:] if tag in ("flood_tick", "flood_census", "spiking_growth") else rows
            )
            with (a.output / f"{tag}.csv").open("w", newline="") as f:
                w = csv.DictWriter(
                    f, list(dict.fromkeys(k for r in records[tag] for k in r)), lineterminator="\n"
                )
                w.writeheader()
                w.writerows(records[tag])
        print(seed, arm, t, flush=True)

    with tempfile.TemporaryDirectory(
        prefix="axion-ways-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as scratch:
        folder = Path(scratch)
        start = folder / "opening.flood"
        for seed in a.seeds:
            if a.phase == "nursery":
                for arm in a.arms or ("earned", "uniform"):
                    if arm not in ("earned", "uniform"):
                        p.error("unknown brood")
                    run(
                        seed,
                        arm,
                        a.seasons,
                        "--load",
                        ROOT / "examples/hollowbank.flood",
                        "--effort",
                        "live",
                        "--effort-rear",
                        a.seasons,
                        "--effort-population",
                        a.capacity,
                        "--effort-brood",
                        arm,
                        "--effort-practice",
                        a.practice,
                        "--effort-seed",
                        seed,
                        "--effort-study",
                        a.trials,
                        "--steps",
                        0,
                        "--save",
                        start,
                    )
                    if a.germ_directory and arm == "earned":
                        shutil.copyfile(start, a.germ_directory / f"earned-{seed}.flood")
                    if a.keep and arm == "earned" and seed == a.seeds[0]:
                        shutil.copyfile(start, a.keep)
                continue
            if a.phase == "feral":
                # These founding effort organs are independent random hereditary material.
                options = [
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
                    "--hollow-germs",
                    ROOT / "examples/hollowbank.flood",
                    "--hollow-ground",
                    "rich",
                    "--hollow-seed",
                    seed,
                    "--effort",
                    "live",
                    "--effort-seed",
                    seed,
                    "--effort-weather",
                    a.weather,
                    "--effort-audit",
                    "quiet",
                ]
                prior = None
                last = 0
                for j, t in enumerate([0, *a.horizons]):
                    target = folder / f"wild-{j % 2}.flood"
                    run(
                        seed,
                        "feral",
                        t,
                        *(options if prior is None else ["--load", prior]),
                        "--effort-seed",
                        seed,
                        "--effort-census",
                        "--effort-study",
                        a.trials,
                        "--steps",
                        t - last,
                        "--save",
                        target,
                    )
                    prior = target
                    last = t
                if a.keep and seed == a.seeds[0]:
                    shutil.copyfile(prior, a.keep)
                continue
            germs = a.germs / f"earned-{seed}.flood" if a.germs.is_dir() else a.germs
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
                    "--hollow-germs",
                    ROOT / "examples/hollowbank.flood",
                    "--hollow-ground",
                    "rich",
                    "--hollow-seed",
                    seed,
                ]
            )
            run(
                seed,
                "common",
                0,
                *source,
                "--effort-germs",
                germs,
                "--effort",
                "live",
                "--effort-seed",
                seed,
                "--effort-weather",
                a.weather,
                "--effort-window",
                a.window,
                *(["--effort-movement", a.movement] if a.movement != "impulse" else []),
                *(["--effort-assignments", a.assignments] if a.assignments != "resample" else []),
                "--steps",
                0,
                "--save",
                start,
            )
            if a.phase == "field":
                arms = {
                    k: ["--effort", k]
                    for k in ("live", "always", "fixed", "shuffled", "no-taste", "no-strain")
                }
                arms.update(
                    rigid=["--effort-trails", "rigid"], footsteps=["--effort-sculpt", "footsteps"]
                )
            elif a.phase == "legacy":
                grown = folder / "worked.flood"
                run(seed, "makers", 512, "--load", start, "--steps", 512, "--save", grown)
                arms = {"intact": [], "washed": ["--wash-treads"]}
                # Everyone has new bodies and blank contacts on the older worked soil.
                source = [
                    "--seed",
                    seed ^ 0x359891,
                    "--side",
                    64,
                    "--neurons",
                    128,
                    "--density",
                    72,
                    "--soil",
                    grown,
                    "--siphon-germs",
                    ROOT / "examples/reedbed.flood",
                    "--hollow-germs",
                    ROOT / "examples/hollowbank.flood",
                    "--effort-germs",
                    germs,
                    "--effort-seed",
                    seed,
                    "--effort-weather",
                    a.weather,
                    "--effort-window",
                    a.window,
                    *(["--effort-movement", a.movement] if a.movement != "impulse" else []),
                    *(["--effort-assignments", a.assignments] if a.assignments != "resample" else []),
                ]
            else:
                arms = {k: ["--effort-contest", k] for k in ("paired", "reciprocal", "sham")}
            selected = a.arms or list(arms)
            if not set(selected) <= arms.keys():
                p.error("unknown field intervention")
            for arm in selected:
                prior = start
                last = 0
                for j, t in enumerate([0, *a.horizons]):
                    out = folder / f"bank-{j % 2}.flood"
                    opts = arms[arm] if not j else [v for v in arms[arm] if v != "--wash-treads"]
                    setup = source if a.phase == "legacy" and not j else ["--load", prior]
                    run(
                        seed,
                        arm,
                        t,
                        *setup,
                        *opts,
                        "--effort-seed",
                        seed,
                        "--effort-audit",
                        "quiet" if a.phase == "contest" else "randomized",
                        "--effort-census",
                        "--steps",
                        t - last,
                        "--save",
                        out,
                    )
                    prior = out
                    last = t
                if a.keep and seed == a.seeds[0] and arm in ("live", "intact", "paired"):
                    shutil.copyfile(prior, a.keep)


if __name__ == "__main__":
    main()
