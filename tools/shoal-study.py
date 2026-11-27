#!/usr/bin/env python3
"""Rear founders on scattered stock and retest on independently generated beds and bodily remappings."""

import argparse
import csv
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--axion", default="./axion")
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--retain", type=Path)
parser.add_argument("--seeds", nargs="+", type=int, default=[1801, 4079, 6841, 9601])
parser.add_argument("--seasons", type=int, default=384)
parser.add_argument("--stations", type=int, default=4096)
parser.add_argument("--harvest-window", type=int, default=256)
parser.add_argument("--spans", type=int, nargs="+", default=[3, 4, 5])
parser.add_argument("--subjects", type=int, default=2048)
parser.add_argument("--trials", type=int, default=4)
args = parser.parse_args()


def run(*words):
    completed = subprocess.run(
        [args.axion, "floodplain", *map(str, words)], capture_output=True, text=True, check=True
    )
    return completed.stdout.splitlines()


with (
    tempfile.TemporaryDirectory(prefix="axion-shoals-", dir=os.environ.get("TMPDIR", "/var/tmp")) as tmp,
    args.output.open("w", newline="") as out,
):
    stage = Path(tmp)
    rows = None
    for seed in args.seeds:
        for regime in ("provisioned", "fostered"):
            source = stage / "brood.flood"
            donor = stage / "builder.mind"
            run(
                "--side",
                32,
                "--seed",
                seed,
                "--shoal-seasons",
                args.seasons,
                "--work-capacity",
                args.stations,
                "--shoal-moves",
                10,
                "--shoal-ideas",
                128,
                "--shoal-exposure",
                args.harvest_window,
                "--work-recruit",
                regime,
                "--steps",
                0,
                "--save",
                source,
                "--export-mind",
                donor,
            )
            if args.retain and seed == args.seeds[0] and regime == "provisioned":
                args.retain.mkdir(exist_ok=True, parents=True)
                shutil.copyfile(source, args.retain / "brood.flood")
                shutil.copyfile(donor, args.retain / "builder.mind")
            for span in args.spans:
                lines = run(
                    "--load",
                    source,
                    "--work-test-seed",
                    seed ^ 0xB919,
                    "--shoal-trials",
                    args.trials,
                    "--work-subjects",
                    args.subjects,
                    "--shoal-span",
                    span,
                    "--shoal-moves",
                    10,
                    "--shoal-ideas",
                    128,
                    "--shoal-exposure",
                    256,
                    "--steps",
                    0,
                )
                for line in lines:
                    if not line.startswith("shoal_test="):
                        continue
                    r = dict(item.split("=", 1) for item in line.split())
                    entry = dict(seed=seed, rearing=regime, rearing_exposure=args.harvest_window, **r)
                    if rows is None:
                        rows = csv.DictWriter(out, fieldnames=entry, lineterminator="\n")
                        rows.writeheader()
                    rows.writerow(entry)
            out.flush()
            print(seed, regime, flush=True)
