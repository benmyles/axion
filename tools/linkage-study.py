#!/usr/bin/env python3
"""Replay mechanical rearing, live watershed interventions or teaching chains on CUDA."""

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
        "phase",
        choices=(
            "nursery",
            "teachers",
            "watershed",
            "feral",
            "competition",
            "climates",
        ),
    )
    p.add_argument("output", type=Path)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--seeds", nargs="+", type=int, default=[89371, 31277, 55109, 74093])
    p.add_argument("--cases", type=int, default=2048)
    p.add_argument("--seasons", type=int, default=256)
    p.add_argument("--adults", type=int, default=2048)
    p.add_argument("--ticks", type=int, default=2048)
    p.add_argument("--source", type=Path)
    p.add_argument("--germs", type=Path, default=ROOT / "examples/spindlebank.flood")
    p.add_argument("--de-novo", action="store_true")
    p.add_argument("--save", action="store_true", help="retain each final watershed")
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    devices = defaultdict(list)

    def run(seed, arm, *options):
        command = [str(v) for v in (args.axion.resolve(), "floodplain", *options)]
        r = subprocess.run(command, text=True, capture_output=True, check=False)
        if r.returncode:
            raise RuntimeError(r.stderr + r.stdout[-1200:])
        for line in r.stdout.splitlines():
            tag = line.partition("=")[0]
            if (
                not tag.startswith("linkage_")
                and tag not in ("flood_seconds", "spiking_growth", "flood_census")
            ) or tag == "linkage_ground":
                continue
            row = dict(v.split("=", 1) for v in line.split() if "=" in v)
            devices[tag].append(dict(founding_seed=seed, intervention=arm, **row))
        print(seed, arm, flush=True)
        for tag, items in devices.items():
            with (args.output / f"{tag}.csv").open("w", newline="") as f:
                w = csv.DictWriter(
                    f,
                    list(dict.fromkeys(k for a in items for k in a)),
                    lineterminator="\n",
                )
                w.writeheader()
                w.writerows(items)

    with tempfile.TemporaryDirectory(
        prefix="axion-racks-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as tmp:
        opening = Path(tmp) / "opening.flood"
        for seed in args.seeds:

            def save(arm, seed=seed):
                return (
                    ["--save", args.output / f"{arm}-{seed}.flood"] if args.save else []
                )

            common = ["--linkage-seed", seed]
            if args.phase == "nursery":
                source = args.source or ROOT / "examples/treadbank.flood"
                for arm in ("earned", "uniform"):
                    prior = (
                        []
                        if args.de_novo
                        else ["--linkage-law", ROOT / "examples/causeway-guide.mind"]
                    )
                    run(
                        seed,
                        arm,
                        "--load",
                        source,
                        "--linkage",
                        "live",
                        *prior,
                        *common,
                        "--linkage-rear",
                        args.seasons,
                        "--linkage-population",
                        args.adults,
                        "--linkage-brood",
                        arm,
                        "--linkage-study",
                        args.cases,
                        "--linkage-pieces",
                        4,
                        "--steps",
                        0,
                        *save(arm),
                    )
            elif args.phase == "climates":
                for regimen, brood in (
                    ("changing", "earned"),
                    ("constant", "earned"),
                    ("changing", "uniform"),
                ):
                    label = f"{regimen}-{brood}"
                    run(
                        seed,
                        label,
                        "--load",
                        args.source or args.germs,
                        "--linkage-germs",
                        args.germs,
                        "--linkage-climate",
                        regimen,
                        "--linkage-brood",
                        brood,
                        "--linkage-rear",
                        args.seasons,
                        "--linkage-population",
                        args.adults,
                        *common,
                        "--linkage-recall",
                        args.cases,
                        "--linkage-pieces",
                        4,
                        "--steps",
                        0,
                        *save(label),
                    )
            elif args.phase == "teachers":
                run(
                    seed,
                    "paid",
                    "--load",
                    args.source or args.germs,
                    *common,
                    "--linkage-witnesses",
                    args.cases,
                    "--linkage-practice",
                    2,
                    "--linkage-pieces",
                    4,
                    "--steps",
                    0,
                )
            elif args.phase == "feral":
                run(
                    seed,
                    "feral",
                    "--load",
                    args.source or ROOT / "examples/causeways.flood",
                    "--linkage",
                    "live",
                    "--linkage-attention",
                    "focal",
                    *common,
                    "--steps",
                    args.ticks,
                    "--linkage-census",
                    *save("feral"),
                )
            else:
                run(
                    seed,
                    "opening",
                    "--load",
                    args.source or ROOT / "examples/causeways.flood",
                    "--linkage",
                    "live",
                    "--linkage-germs",
                    args.germs,
                    *common,
                    "--steps",
                    0,
                    "--save",
                    opening,
                )
                arms = {
                    "live": ["--linkage", "live"],
                    "private": ["--linkage-sight", "private"],
                    "loose": ["--linkage-joints", "loose"],
                    "unplanned": ["--linkage", "unplanned"],
                    "exploratory": ["--linkage-building", "exploratory"],
                    "scrambled": ["--linkage", "scrambled"],
                    "focal": ["--linkage-attention", "focal"],
                    "hydrated": ["--linkage-attention", "focal", "--linkage-weather", "wet"],
                }
                if args.phase == "competition":
                    arms = {
                        arm: ["--linkage-contest", arm, "--linkage-attention", "focal"]
                        for arm in ("paired", "reciprocal", "sham")
                    }
                for arm, options in arms.items():
                    run(
                        seed,
                        arm,
                        "--load",
                        opening,
                        *common,
                        *options,
                        "--steps",
                        args.ticks,
                        "--linkage-census",
                        *save(arm),
                    )


if __name__ == "__main__":
    main()
