#!/usr/bin/env python3
"""Measure independently founded watersheds, matched breeding regimes and environmental transmission."""

import argparse
import csv
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--source", type=Path, default=ROOT / "examples/reedbed.flood")
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--seeds", nargs="+", type=int, default=[8123, 41113, 87013, 139939])
    p.add_argument("--seasons", type=int, default=1024)
    p.add_argument("--steps", type=int, default=4096)
    p.add_argument("--subjects", type=int, default=256)
    p.add_argument("--capacity", type=int, default=4096)
    p.add_argument(
        "--passage", action="store_true", help="Follow the same occupied habitat on enclosed and expanding ground"
    )
    p.add_argument("--horizons", type=int, nargs="+", default=[2048, 8192, 32768, 65536])
    p.add_argument("--save-frontier", type=Path, help="Retain the final open, expanded habitat")
    args = p.parse_args()
    if args.horizons != sorted(set(args.horizons)) or args.horizons[0] < 1:
        p.error("use positive, increasing passage times")
    if args.save_frontier and not args.passage:
        p.error("--save-frontier requires --passage")
    args.output.mkdir(parents=True, exist_ok=True)
    included = {
        "siphon_transfer",
        "siphon_foster",
        "siphon_census",
        "siphon_audience",
        "siphon_tick",
        "siphon_brood",
        "siphon_pair",
        "siphon_legacy",
        "siphon_deployment",
        "flood_seconds",
        "spiking_growth",
        "flood_census",
        "flood_frontier",
    }
    tables = {}

    def run(cohort, seed, opts, elapsed=None):
        cmd = [args.axion.resolve(), "floodplain", *map(str, opts)]
        trial = subprocess.run(cmd, text=True, capture_output=True)
        if trial.returncode:
            raise RuntimeError(trial.stderr + trial.stdout[-1000:])
        for line in trial.stdout.splitlines():
            name = line.partition("=")[0]
            if name not in included:
                continue
            row = dict(group=cohort, seed=seed)
            if elapsed is not None:
                row["elapsed"] = elapsed
            row.update(v.split("=", 1) for v in line.split() if "=" in v)
            tables.setdefault(name, []).append(row)
        print(f"Finished {cohort} {seed}", flush=True)
        for name, rows in tables.items():
            with (args.output / (name + ".csv")).open("w", newline="") as out:
                keys = dict.fromkeys(k for r in rows for k in r)
                table = csv.DictWriter(out, fieldnames=keys, lineterminator="\n")
                table.writeheader()
                table.writerows(rows)

    with tempfile.TemporaryDirectory(prefix="reed-valleys-") as work:
        if args.passage:
            for terrain, wire in (
                ("enclosed", "open"),
                ("enclosed", "shut"),
                ("frontier", "shut"),
                ("frontier", "open"),
            ):
                source = args.source.resolve()
                previous = 0
                for i, age in enumerate(args.horizons):
                    target = Path(work) / f"{terrain}-{wire}-{age}.flood"
                    if terrain == "frontier" and wire == "open" and age == args.horizons[-1] and args.save_frontier:
                        target = args.save_frontier.resolve()
                    extension = ["--extend", 128] if terrain == "frontier" and i == 1 else []
                    seed = 823711 + age
                    run(
                        terrain + "-" + wire,
                        seed,
                        [
                            "--load",
                            source,
                            "--siphon-wire",
                            wire,
                            *extension,
                            "--steps",
                            age - previous,
                            "--report-every",
                            age - previous,
                            "--siphon-census",
                            16,
                            "--siphon-trials",
                            8,
                            "--siphon-practice",
                            0,
                            "--work-subjects",
                            args.subjects,
                            "--work-test-seed",
                            seed,
                            "--save",
                            target,
                        ],
                        age,
                    )
                    source = target
                    previous = age
            return
        for seed in args.seeds:
            physical = [
                "--side",
                32,
                "--seed",
                seed,
                "--density",
                72,
                "--neurons",
                128,
                "--grains",
                60,
                "--siphons",
                4,
                "--siphon-resonance",
                "material",
                "--siphon-windows",
                "clear",
                "--siphon-view",
                "intermittent",
                "--siphon-medium",
                "resonant",
                "--work-subjects",
                args.subjects,
                "--work-test-seed",
                seed + 971423,
            ]
            exam = ["--siphon-census", 16, "--siphon-trials", 8]
            run("wild", seed, [*physical, "--steps", args.steps, *exam])
            for selection in ("provisioned", "fostered"):
                saved = Path(work) / f"{seed}-{selection}.flood"
                run(
                    selection,
                    seed,
                    [
                        *physical,
                        "--siphon-selection",
                        selection,
                        "--siphon-rear",
                        args.seasons,
                        "--work-capacity",
                        args.capacity,
                        "--siphon-visits",
                        256,
                        "--steps",
                        0,
                        "--save",
                        saved,
                    ],
                )
                run(
                    selection + "-pairs",
                    seed,
                    [
                        "--load",
                        saved,
                        "--steps",
                        0,
                        "--siphon-practice",
                        384,
                        "--siphon-trials",
                        8,
                        "--work-subjects",
                        args.subjects,
                        "--work-test-seed",
                        seed + 971423,
                        "--siphon-foster",
                        2,
                        "--siphon-exposures",
                        192,
                        "--siphon-mentors",
                        "fixed",
                    ],
                )
                run(
                    selection + "-river",
                    seed,
                    [
                        "--load",
                        saved,
                        "--steps",
                        args.steps,
                        *exam,
                        "--work-subjects",
                        args.subjects,
                        "--work-test-seed",
                        seed + 971423,
                    ],
                )
            # Only the ground, chemistry and existing sound fields are grafted into the new population.
            # No additional encounter can send a sound during the test.
            for treatment, opts in (
                ("ancestral", []),
                ("scrambled", ["--siphon-audience", "ancestral-scrambled"]),
                ("damped", ["--wash-sounds"]),
            ):
                run(
                    treatment,
                    seed,
                    [
                        "--side",
                        64,
                        "--seed",
                        seed,
                        "--density",
                        72,
                        "--soil",
                        args.source.resolve(),
                        "--siphon-germs",
                        args.source.resolve(),
                        "--siphon-audience",
                        "ancestral",
                        "--siphon-workers",
                        "still",
                        "--steps",
                        16,
                        *opts,
                        "--siphon-trials",
                        8,
                        "--work-subjects",
                        args.subjects,
                        "--work-test-seed",
                        seed + 971423,
                    ],
                )


if __name__ == "__main__":
    main()
