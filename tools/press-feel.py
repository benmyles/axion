#!/usr/bin/env python3
"""Release independent founders into physical beds and measure the cost and benefit of private probes."""

import argparse
import csv
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument(
        "--source",
        type=Path,
        help="Continue one occupied watershed. Omit for independent young populations.",
    )
    p.add_argument("--seeds", nargs="+", type=int, default=[5623, 14831, 40093, 97007])
    p.add_argument("--horizons", nargs="+", type=int, default=[128, 1024, 8192])
    p.add_argument(
        "--arms",
        nargs="+",
        choices=("live", "quiet", "displaced", "stiff"),
        default=["live", "quiet", "displaced", "stiff"],
    )
    p.add_argument("--output", required=True, type=Path)
    p.add_argument(
        "--extend", type=int, help="Expand the common watershed before the comparison"
    )
    p.add_argument(
        "--keep", type=Path, help="Retain the final live state of a continuation"
    )
    a = p.parse_args()
    if min(a.horizons) < 1 or sorted(set(a.horizons)) != a.horizons:
        p.error("horizons must be increasing positive integers")
    if a.keep and (not a.source or "live" not in a.arms):
        p.error("keep needs a source and a live arm")
    a.output.mkdir(parents=True, exist_ok=True)
    table = {}

    def run(seed, arm, t, *arguments):
        r = subprocess.run(
            list(map(str, [a.axion.resolve(), "floodplain", *arguments])),
            text=True,
            capture_output=True,
            check=False,
        )
        if r.returncode:
            raise RuntimeError(r.stderr + r.stdout[-1200:])
        for l in r.stdout.splitlines():
            kind = l.partition("=")[0]
            if kind not in (
                "press_field",
                "press_touch",
                "press_census",
                "press_affordance",
                "press_hatchlings",
                "clay_tick",
                "flood_seconds",
                "flood_tick",
                "flood_census",
            ):
                continue
            q = {"seed": seed, "feel": arm, "age": t}
            q.update(x.split("=", 1) for x in l.split() if "=" in x)
            table.setdefault(kind, []).append(q)
        for k, rows in table.items():
            with (a.output / (k + ".csv")).open("w", newline="") as f:
                w = csv.DictWriter(
                    f,
                    fieldnames=dict.fromkeys(key for row in rows for key in row),
                    lineterminator="\n",
                )
                w.writeheader()
                w.writerows(rows)
        print(seed, arm, t, flush=True)

    with tempfile.TemporaryDirectory(
        prefix="axion-palpation-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as folder:
        folder = Path(folder)
        for seed in a.seeds[:1] if a.source else a.seeds:
            base = folder / "founders.flood"
            initial = (
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
                    "--press-founders",
                    "varied",
                    "--press-seasons",
                    192,
                    "--press-population",
                    1024,
                    "--press-practice",
                    "mixed",
                    "--press-seed",
                    seed,
                    "--clay-reach",
                    "shared",
                ]
            )
            if a.extend:
                initial += ["--extend", a.extend]
            run(
                seed,
                "shared",
                0,
                *initial,
                "--press",
                "live",
                "--press-geology",
                "tidal",
                "--press-feel",
                "quiet",
                "--steps",
                0,
                "--save",
                base,
            )
            for arm in a.arms:
                old = base
                elapsed = 0
                for t in a.horizons:
                    f = folder / f"{arm}-{t}.flood"
                    run(
                        seed,
                        arm,
                        t,
                        "--load",
                        old,
                        "--press-feel",
                        arm,
                        "--press-seed",
                        seed ^ 0x642313,
                        "--press-census",
                        "--steps",
                        t - elapsed,
                        "--save",
                        f,
                    )
                    if old != base:
                        old.unlink()
                    old, elapsed = f, t
                if a.keep and arm == "live":
                    shutil.copyfile(old, a.keep)
                old.unlink()


if __name__ == "__main__":
    main()
