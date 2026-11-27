#!/usr/bin/env python3
"""Reproduce the CUDA experiments in docs/learning.md and print raw counts as CSV."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BINARY = os.environ.get("AXION_BIN", str(ROOT / "axion"))
FIELD_SEEDS = (379, 2191, 8461, 19081, 32063, 27191, 61343, 75149)
BREEDING_SEEDS = (1277, 2203, 3179, 5711, 7823, 9029)
AUDIT_SEEDS = (115889, 409483, 710237, 990041)
METRICS = (
    "lifetimes varieties exposures probe_delay items reachable naive learned practiced_items "
    "practiced erased changed_items stable_items surprise after_one recovered retained attempts "
    "delivered successes"
).split()
COLUMNS = ["experiment", "training_seed", "probe_seed", "problem", "arm", "primary_rule", *METRICS]


def run(*args):
    result = subprocess.run([BINARY, "life", *map(str, args)], text=True, check=True,
                            stdout=subprocess.PIPE)
    return [dict(field.split("=", 1) for field in line.split() if "=" in field)
            for line in result.stdout.splitlines()]


def panel(output, world, experiment, training_seed, problem, varieties, seed, *,
          quartets=256, exposures=16, delay=1, dissect=False):
    args = ["--load", world, "--steps", 0, "--conditioning", quartets, "--problem",
            problem, "--varieties", varieties, "--exposures", exposures, "--probe-delay",
            delay, "--probe-seed", seed]
    if dissect:
        args.append("--dissect")
    rows = run(*args)
    top = next((key[5:] for row in rows if "distinct_rules" in row
                for key in row if key.startswith("rule_")), "")
    for row in rows:
        if "conditioning" in row:
            output.writerow([experiment, training_seed, seed, problem, row["conditioning"],
                             top, *(row[k] for k in METRICS)])


def field(output, folder, contests):
    world = folder / "donors.eco"
    for seed in FIELD_SEEDS:
        for habitat in ("grove", "compound"):
            run("--habitat", habitat, "--side", 64, "--seed", seed, "--steps", 16384,
                "--save", world)
            if contests:
                for site in (601, 1423, 7919):
                    for swap in range(2):
                        args = ["--habitat", habitat, "--side", 64, "--founders", world,
                                "--seed", site, "--contest", "dendrites", "--mutation", "off",
                                "--steps", 512]
                        if swap:
                            args.append("--swap-cohorts")
                        before, after = [r for r in run(*args) if "tick" in r]
                        output.writerow([habitat, seed, site, swap, before["treatment_alive"],
                                         before["control_alive"], after["treatment_alive"], after["control_alive"]])
                continue
            task = "grips" if habitat == "grove" else "dimensions"
            panel(output, world, habitat, seed, task, 4, 839063, dissect=True)
            if habitat == "grove":
                panel(output, world, habitat, seed, "dimensions", 4, 839063)
                panel(output, world, habitat, seed, "grips", 32, 839063)
        if contests:
            continue
        run("--habitat", "grove", "--side", 64, "--seed", seed, "--digestion", 4,
            "--steps", 32768, "--save", world)
        panel(output, world, "delayed", seed, "grips", 4, 839063, quartets=128, delay=4, dissect=True)
        run("--habitat", "grove", "--species", 32, "--side", 64, "--seed", seed,
            "--steps", 32768, "--save", world)
        panel(output, world, "many", seed, "grips", 32, 839063)
        for bits in (1, 2):
            run("--habitat", "grove", "--species", 32, "--season", 128, "--synapse-bits",
                bits, "--side", 64, "--seed", seed, "--steps", 65536, "--save", world)
            panel(output, world, f"volatile-{bits}", seed, "grips", 32, 839063)


def breeding(output, folder):
    world = folder / "founders.eco"
    for seed in BREEDING_SEEDS:
        for bits, mode in ((1, "select"), (2, "select"), (2, "drift")):
            run("--habitat", "compound", "--synapse-bits", bits, "--seed", seed,
                "--side", 32, "--nursery", 384, "--breeding", mode, "--steps", 0, "--save", world)
            for task, values in (("grips", 32), ("dimensions", 16)):
                panel(output, world, f"{mode}-{bits}", seed, task, values, 923543, quartets=128)


def limits(output, folder):
    world = folder / "stress.eco"
    for seed in BREEDING_SEEDS:
        run("--habitat", "compound", "--synapse-bits", 2, "--seed", seed, "--side", 32,
            "--nursery", 384, "--nursery-problem", "composition", "--steps", 0, "--save", world)
        for values in (4, 16):
            panel(output, world, "joint-selection", seed, "composition", values, 923543,
                  quartets=128, exposures=64)
    for site in (601, 1423, 7919, 9127):
        run("--habitat", "compound", "--synapse-bits", 2, "--founders", ROOT / "examples/learner.eco",
            "--seed", site, "--side", 64, "--steps", 16384, "--save", world)
        panel(output, world, "field-after-nursery", site, "dimensions", 16, 409483)


def audit(output):
    for seed in AUDIT_SEEDS:
        for task, values in (("grips", 32), ("dimensions", 16)):
            for exposures in (2, 4, 8, 16, 32):
                panel(output, ROOT / "examples/learner.eco", "released-nursery", 1277,
                      task, values, seed, quartets=512, exposures=exposures)
        for exposures in (16, 64):
            panel(output, ROOT / "examples/learner.eco", "joint-effects", 1277, "composition",
                  16, seed, quartets=512, exposures=exposures)
        panel(output, ROOT / "examples/delayed.eco", "released-delayed", 8461, "grips",
              4, seed, quartets=512, delay=4, dissect=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("study", choices=["field", "contests", "breeding", "audit", "limits"])
    args = parser.parse_args()
    output = csv.writer(sys.stdout, lineterminator="\n")
    output.writerow(COLUMNS if args.study != "contests" else
                    ["habitat", "donor", "habitat_seed", "swapped", "initial_treatment",
                     "initial_control", "final_treatment", "final_control"])
    with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-study-") as tmp:
        if args.study == "audit":
            audit(output)
        elif args.study == "limits":
            limits(output, Path(tmp))
        elif args.study == "breeding":
            breeding(output, Path(tmp))
        else:
            field(output, Path(tmp), args.study == "contests")
