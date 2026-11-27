#!/usr/bin/env python3
"""Follow old Soundings residents across expanded ground, varying only the introduced material affordances."""

import argparse
import csv
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--source", type=Path, default=ROOT / "examples/soundings.flood")
    p.add_argument("--genes", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--save", type=Path)
    p.add_argument(
        "--arms",
        nargs="+",
        choices=("original", "unselected", "single", "rake", "shared"),
        default=["original", "unselected", "single", "rake", "shared"],
    )
    p.add_argument("--horizons", nargs="+", type=int, default=[1024, 8192, 32768])
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    tables = {}
    if a.horizons != sorted(set(a.horizons)) or min(a.horizons) < 1:
        p.error("use positive increasing horizons")
    with tempfile.TemporaryDirectory(prefix="clay-frontiers-") as scratch:
        for arm in a.arms:
            prior = a.source.resolve()
            last = 0
            for i, age in enumerate(a.horizons):
                flags = []
                if not i and arm != "original":
                    flags += [
                        "--pottery",
                        "sheltered",
                        "--clay-reach",
                        arm if arm in ("single", "rake", "shared") else "shared",
                    ]
                    if arm != "unselected":
                        flags += ["--clay-germs", a.genes.resolve()]
                if i == 1:
                    flags += ["--extend", 256]
                target = (
                    a.save.resolve()
                    if a.save and arm == "shared" and age == a.horizons[-1]
                    else Path(scratch) / f"{arm}-{age}.flood"
                )
                cmd = [
                    a.axion.resolve(),
                    "floodplain",
                    "--load",
                    prior,
                    *flags,
                    "--steps",
                    age - last,
                    "--report-every",
                    age - last,
                    "--siphon-census",
                    8,
                    "--siphon-trials",
                    8,
                    "--work-subjects",
                    256,
                    "--work-test-seed",
                    620939 + age,
                    "--save",
                    target,
                ]
                r = subprocess.run(list(map(str, cmd)), capture_output=True, text=True)
                if r.returncode:
                    raise RuntimeError(r.stderr + r.stdout[-1000:])
                for l in r.stdout.splitlines():
                    tag = l.partition("=")[0]
                    if tag not in (
                        "flood_census",
                        "spiking_growth",
                        "clay_tick",
                        "clay_object",
                        "siphon_census",
                        "siphon_tick",
                        "flood_seconds",
                    ):
                        continue
                    q = dict(bank=arm, elapsed=age)
                    q.update(x.split("=", 1) for x in l.split() if "=" in x)
                    tables.setdefault(tag, []).append(q)
                if i:
                    prior.unlink()
                prior = target
                last = age
                for tag, rows in tables.items():
                    with (a.output / (tag + ".csv")).open("w", newline="") as f:
                        w = csv.DictWriter(
                            f, fieldnames=dict.fromkeys(k for row in rows for k in row), lineterminator="\n"
                        )
                        w.writeheader()
                        w.writerows(rows)
                print(arm, age, flush=True)


if __name__ == "__main__":
    main()
