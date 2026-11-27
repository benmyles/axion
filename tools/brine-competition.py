#!/usr/bin/env python3
"""Compete two maternal predictive circuits in the same water with identical paired bodies."""

import argparse
import csv
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--donors", type=Path, required=True, help="Saved brood populations from brine-study.py")
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--steps", type=int, default=8192)
    a = p.parse_args()
    a.output.parent.mkdir(parents=True, exist_ok=True)
    records = []
    for line in (1217, 6911, 11299, 23311):
        for ions in (2, 5):
            parents = [
                a.donors / f"{line}-{ions}-survey-arbors-{kind}.flood" for kind in ("provisioned", "fostered")
            ]
            for terrain in (14401, 31583):
                for readout in ("open", "muted"):
                    first = None
                    for mirror in (0, 1):
                        argv = [
                            a.axion,
                            "floodplain",
                            "--colonists",
                            parents[0],
                            "--competitors",
                            parents[1],
                            "--brine-competition",
                            "--brine-sense",
                            readout,
                            "--side",
                            32,
                            "--neurons",
                            256,
                            "--seed",
                            terrain,
                            "--density",
                            46,
                            "--brine",
                            ions,
                            "--worksites",
                            "active",
                            "--substrate",
                            "mixed",
                            "--construction-tide",
                            8,
                            "--grains",
                            60,
                            "--steps",
                            a.steps,
                            "--report-every",
                            a.steps,
                        ]
                        if mirror:
                            argv += ["--swap"]
                        run = subprocess.run(list(map(str, argv)), text=True, cwd=ROOT, capture_output=True)
                        if run.returncode:
                            raise RuntimeError(run.stderr)
                        census = [
                            dict(q.split("=", 1) for q in l.split())
                            for l in run.stdout.splitlines()
                            if l.startswith("flood_census=")
                        ]
                        opening, closing = census[0], census[-1]
                        if opening["cohort_a"] != opening["cohort_b"]:
                            raise RuntimeError("unbalanced founder placement")
                        counts = tuple(int(closing[k]) for k in ("cohort_a", "cohort_b", "reserve_a", "reserve_b"))
                        if mirror and readout == "muted" and counts != (first[1], first[0], first[3], first[2]):
                            raise RuntimeError("neutral label affected a paired watershed")
                        first = counts
                        records.append(
                            dict(
                                donor=line,
                                solutes=ions,
                                terrain=terrain,
                                readout=readout,
                                mirror=mirror,
                                steps=a.steps,
                                opening_each=opening["cohort_a"],
                                provisioned=counts[0],
                                fostered=counts[1],
                                provisioned_energy=counts[2],
                                fostered_energy=counts[3],
                                founder_lines=closing["families"],
                                body_plans=closing["distinct_bodies"],
                            )
                        )
    with a.output.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=records[0], lineterminator="\n")
        w.writeheader()
        w.writerows(records)


if __name__ == "__main__":
    main()
