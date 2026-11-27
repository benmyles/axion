#!/usr/bin/env python3
"""Screen real two-junction foragers and repeat a never-demonstrated destination in independent spiking lives."""

import argparse
import csv
import os
import subprocess
from pathlib import Path
from compound_audit import audit, cohort, read

ROOT = Path(__file__).resolve().parents[1]


def study(out, binary, pop, n, lives, tests, lessons, confirm):
    out.mkdir(parents=True, exist_ok=False)
    env = {**os.environ, "AXION_JOURNAL_DIR": str(out / "journal")}
    archive, when = {
        "brook": ("capillary-nursery", 20480),
        "descendants": ("context-free-foragers", 153600),
        "coast": ("context-free-estuary", 443288),
        "older": ("capillary-older-nursery", 86016),
    }[pop]
    habitat = out / "habitat.flood"
    with habitat.open("xb") as f:
        subprocess.run(
            ["zstd", "-qdc", ROOT / "examples" / (archive + ".flood.zst")],
            stdout=f,
            check=True,
        )

    def run(tag, *args):
        p = out / (tag + ".log")
        with p.open("x") as f:
            subprocess.run(
                list(
                    map(
                        str,
                        [binary, "floodplain", "--load", habitat, "--steps", 0, *args],
                    )
                ),
                cwd=ROOT,
                env=env,
                stdout=f,
                stderr=subprocess.STDOUT,
                check=True,
            )

    def sample(omit, arbor, food="food", medium="active"):
        tag = f"population-{omit}-{arbor}-{food}-{medium}"
        p = out / (tag + ".tsv")
        run(
            tag,
            "--capillary-residents",
            n,
            "--retina-born-after",
            when - 2048,
            "--capillary-contacts",
            0,
            "--catalytic-channels",
            2,
            "--catalytic-histories",
            lives,
            "--catalytic-tests",
            tests,
            "--catalytic-lessons",
            lessons,
            "--class-seed",
            4829501,
            "--catalytic-arbor",
            arbor,
            "--catalytic-feedback",
            food,
            "--catalytic-medium",
            medium,
            "--capillary-screen",
            p,
            *(["--catalytic-omit", omit] if omit < 4 else []),
        )
        q = read(p)
        cohort(q)
        return q

    full = sample(4, "native")
    hidden = sample(3, "native")
    sample(4, "joint")
    sample(3, "joint")
    sample(4, "native", "silent")
    sample(4, "native", "food", "inert")
    if not confirm:
        return

    def born(tag, row):
        result = []
        for role in ("writer", "reader"):
            p = out / (tag + "-" + role + ".mind")
            run(tag + "-" + role, "--slot", row[role], "--export-mind", p)
            result.append(p)
        return result

    target = next((r for r in full if r["joint_codes"] and r["separable_codes"]), None)
    if target:
        young = born("two-codes", target)
        pairs = {}
        for i in range(lives):
            tag = f"founder-{i}"
            current = [out / (tag + "-" + r + ".mind") for r in ("writer", "reader")]
            p = out / (tag + ".tsv")
            run(
                tag,
                "--catalytic-pair",
                *young,
                "--catalytic-channels",
                2,
                "--catalytic-lessons",
                lessons,
                "--class-seed",
                4829501,
                "--catalytic-history",
                i,
                "--catalytic-tests",
                tests,
                "--catalytic-trace",
                p,
                "--catalytic-export",
                *current,
            )
            a = audit(read(p))
            if a["stable"]:
                pairs.setdefault("separable" if a["separable"] else "joint", current)
        for name, (writer, _) in pairs.items():
            for name2, (_, reader) in pairs.items():
                tag = f"frozen-{name}-{name2}"
                p = out / (tag + ".tsv")
                run(
                    tag,
                    "--catalytic-pair",
                    writer,
                    reader,
                    "--catalytic-channels",
                    2,
                    "--catalytic-tests",
                    1024,
                    "--class-seed",
                    3501301,
                    "--catalytic-history",
                    901,
                    "--catalytic-test-seed",
                    13629397,
                    "--catalytic-trace",
                    p,
                )
                audit(read(p))
    chosen = next((r for r in hidden if r["fluent"]), None)
    if chosen:
        pair = born("withheld", chosen)
        for omit in range(4):
            for seed in (3589221, 7240189):
                for structure in ("native", "joint"):
                    tag = f"confirmation-{omit}-{seed}-{structure}"
                    p = out / (tag + ".tsv")
                    run(
                        tag,
                        "--catalytic-pair",
                        *pair,
                        "--catalytic-channels",
                        2,
                        "--catalytic-lessons",
                        lessons,
                        "--catalytic-omit",
                        omit,
                        "--class-seed",
                        seed,
                        "--catalytic-repeat",
                        128,
                        "--catalytic-profiles",
                        p,
                        "--catalytic-tests",
                        64,
                        "--catalytic-test-seed",
                        1943657,
                        "--catalytic-arbor",
                        structure,
                    )
                    assert all(r["trained" + str(omit)] == 0 for r in read(p))


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--binary", type=Path, default=ROOT / "axion")
    p.add_argument(
        "--population",
        choices=("brook", "older", "descendants", "coast"),
        default="descendants",
    )
    p.add_argument("--residents", type=int, default=2048)
    p.add_argument("--histories", type=int, default=4)
    p.add_argument("--tests", type=int, default=48)
    p.add_argument("--lessons", type=int, default=4096)
    p.add_argument("--confirm", action="store_true")
    a = p.parse_args()
    study(
        a.output.resolve(),
        a.binary.resolve(),
        a.population,
        a.residents,
        a.histories,
        a.tests,
        a.lessons,
        a.confirm,
    )
