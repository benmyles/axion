#!/usr/bin/env python3
"""Grow independent foraging populations, then compare matched solvent interventions on unseen coatings."""

import argparse
import csv
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SEEDS = (1217, 6911, 11299, 23311)


def cases(out):
    treatments = (
        (2, "linger", "mosaic", "provisioned"),
        (2, "wash", "mosaic", "provisioned"),
        (5, "wash", "mosaic", "provisioned"),
        (5, "wash", "arbors", "provisioned"),
        (2, "survey", "arbors", "provisioned"),
        (2, "survey", "arbors", "fostered"),
        (5, "survey", "arbors", "provisioned"),
        (5, "survey", "arbors", "fostered"),
        (5, "consult", "arbors", "provisioned"),
        (5, "consult", "arbors", "fostered"),
    )
    for seed in SEEDS:
        for ions, habitat, wiring, inheritance in treatments:
            name = f"{seed}-{ions}-{habitat}-{wiring}-{inheritance}"
            args = f"--side 32 --density 72 --seed {seed} --brine {ions} --brine-rear 1024 --brine-shore {habitat} --brine-wiring {wiring} --brine-selection {inheritance} --brine-tides 16 --work-capacity 8192 --steps 0".split()
            yield (
                name,
                dict(seed=seed, origin="brood", solutes=ions, shore=habitat, wiring=wiring, selection=inheritance),
                args,
            )
        for selection in ("provisioned", "fostered"):
            parent = out / f"{seed}-5-survey-arbors-provisioned.flood"
            name = f"returning-{seed}-{selection}"
            args = [
                "--load",
                str(parent),
                "--brine-origin",
                "resident",
                "--brine-rear",
                "1024",
                "--brine-shore",
                "consult",
                "--brine-selection",
                selection,
                "--brine-tides",
                "16",
                "--work-capacity",
                "8192",
                "--steps",
                "0",
            ]
            yield (
                name,
                dict(
                    seed=seed,
                    origin="continuation",
                    solutes=5,
                    shore="consult",
                    wiring="arbors",
                    selection=selection,
                ),
                args,
            )
        for ions in (2, 5):
            for tick in (0, 32768):
                name = f"wild-{seed}-{ions}-{tick}"
                args = f"--side 32 --neurons 256 --seed {seed} --density 36 --grains 60 --construction-tide 8 --worksites active --brine {ions} --substrate mixed --report-every 4096 --steps {tick}".split()
                yield (
                    name,
                    dict(
                        seed=seed,
                        origin="watershed",
                        solutes=ions,
                        shore="river",
                        wiring="arbors",
                        selection="local births",
                    ),
                    args,
                )


def table(path, rows):
    keys = dict.fromkeys(k for row in rows for k in row)
    with path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=keys, lineterminator="\n")
        w.writeheader()
        w.writerows(rows)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--summarize", action="store_true", help="Read the completed experiment logs only")
    p.add_argument("--only", choices=["brood", "watershed", "continuation"], help="Run one origin only")
    a = p.parse_args()
    out = a.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    readings = []
    seasons = []
    habitats = []
    for name, design, args in cases(out):
        if a.only and design["origin"] != a.only:
            continue
        log = out / (name + ".log")
        if not a.summarize:
            tail = [
                "--brine-assay",
                "32",
                "--brine-examples",
                "9",
                "--work-subjects",
                "1024",
                "--work-test-seed",
                str(design["seed"] ^ 70417),
                "--save",
                str(out / (name + ".flood")),
            ]
            with log.open("w") as target:
                subprocess.run(
                    [str(a.axion.resolve()), "floodplain", *args, *tail], stdout=target, check=True, cwd=ROOT
                )
        last = {}
        for line in log.read_text().splitlines():
            data = dict(word.split("=", 1) for word in line.split() if "=" in word)
            if "brine_transfer" in data:
                readings.append(dict(run=name, **design, **{k: v for k, v in data.items() if k != "solutes"}))
            if "brine_season" in data:
                seasons.append(dict(run=name, **design, **data))
            if "brine_census" in data:
                last.update({k: v for k, v in data.items() if k != "solutes"})
            if "flood_tick" in data:
                last.update({k: data[k] for k in ("living", "births", "deaths", "neurons", "moved")})
            if "flood_census" in data:
                last.update({k: data[k] for k in ("deepest", "distinct_bodies", "expressed_tapes")})
            if "flood_seconds" in data:
                last.update(
                    {k: data[k] for k in ("site_ticks", "simulation_seconds", "simulation_Msite_ticks_per_s")}
                )
            if "brine_brood" in data:
                last.update({k: v for k, v in data.items() if k not in ("seconds", "brine_brood")})
        habitats.append(dict(run=name, **design, **last))
    suffix = "-" + a.only if a.only else ""
    table(out / ("transfer" + suffix + ".csv"), readings)
    table(out / ("seasons" + suffix + ".csv"), seasons)
    table(out / ("populations" + suffix + ".csv"), habitats)


if __name__ == "__main__":
    main()
