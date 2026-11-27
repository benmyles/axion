#!/usr/bin/env python3
"""Reproduce two independently learned chemical conventions in a real pair of watershed animals."""

import argparse
import csv
import os
import re
import subprocess
from pathlib import Path

from capillary_audit import read
from catalytic_audit import boundaries, correspondence, experienced, pairs, witnesses

ROOT = Path(__file__).resolve().parents[1]


def study(out, binary, population, limit, advanced):
    out.mkdir(parents=True, exist_ok=False)
    env = {**os.environ, "AXION_JOURNAL_DIR": str(out / "journal")}
    home = out / "origin.flood"
    name, tick = (
        ("capillary-nursery", 20480)
        if population == "brook"
        else ("context-slow-descendants", 53248)
    )
    with home.open("xb") as f:
        subprocess.run(
            ["zstd", "-qdc", ROOT / "examples" / (name + ".flood.zst")],
            stdout=f,
            check=True,
        )

    def run(tag, *args):
        log = out / (tag + ".log")
        with log.open("x") as f:
            subprocess.run(
                list(map(str, [binary, "floodplain", "--load", home, *args])),
                cwd=ROOT,
                env=env,
                stdout=f,
                stderr=subprocess.STDOUT,
                check=True,
            )
        return log.read_text()

    proof = out / "contacts.tsv"
    run("contacts", "--steps", 0, "--capillary-catalysis-proof", proof)
    boundaries(read(proof))
    if advanced:
        for medium in ("inert", "bound"):
            eye = out / f"{medium}-receipts.tsv"
            film = out / "closures.tsv"
            run(
                "habitat-" + medium,
                "--capillary-field",
                "live",
                "--capillary-catalysis",
                medium,
                "--capillary-memory",
                "regional",
                "--capillary-cargo",
                "pressure",
                "--capillary-cargo-dye",
                "exchange",
                "--capillary-food",
                "pigments",
                "--steps",
                32768,
                "--report-every",
                8192,
                "--capillary-events",
                eye,
                "--save",
                out / (medium + ".flood"),
                *(
                    [
                        "--catalytic-film",
                        film,
                        "--capillary-window",
                        128,
                        "--capillary-frames",
                        256,
                    ]
                    if medium == "bound"
                    else []
                ),
            )
            if medium == "bound":
                experienced(read(eye))
                witnesses(read(film))
    common = ["--steps", 0, "--class-seed", 720911]
    options = [
        "--capillary-residents",
        limit,
        "--retina-born-after",
        tick - 2048,
        "--capillary-contacts",
        0,
    ]
    for physiology, food, history in (
        ("active", "food", 4),
        ("inert", "food", 4),
        ("active", "silent", 4),
        ("active", "unrelated", 4),
        ("active", "food", 32),
    ):
        tag = f"{physiology}-{food}-{history}"
        table = out / (tag + ".tsv")
        run(
            tag,
            *common,
            *options,
            "--catalytic-lessons",
            512,
            "--catalytic-histories",
            history,
            "--catalytic-tests",
            64 if history == 32 else 32,
            "--catalytic-medium",
            physiology,
            "--catalytic-feedback",
            food,
            "--capillary-screen",
            table,
        )
        cohort = read(table)
        pairs(cohort)
    ranked = max(
        cohort,
        key=lambda r: (
            min(r["reliable_left"], r["reliable_right"]),
            r["reliable_left"] + r["reliable_right"],
            r["after"],
        ),
    )
    minds = []
    for role in ("sender", "receiver"):
        mind = out / (role + "-young.mind")
        run(role + "-birth", *common, "--slot", ranked[role], "--export-mind", mind)
        minds.append(mind)
    learned = {}
    checks = []
    for life in range(32):
        tag = f"history-{life}"
        saved = [out / f"{tag}-{role}.mind" for role in ("sender", "reader")]
        trace = out / (tag + ".tsv")
        run(
            tag,
            *common,
            "--catalytic-pair",
            *minds,
            "--catalytic-lessons",
            512,
            "--catalytic-tests",
            64,
            "--catalytic-history",
            life,
            "--catalytic-export",
            *saved,
            "--catalytic-trace",
            trace,
        )
        v = correspondence(read(trace))
        for mark in ("left", "right"):
            if mark not in learned and v.get(mark + "_convention", 0) >= 48:
                learned[mark] = saved
        if len(learned) == 2:
            break
    for writer, (sender, _) in learned.items():
        for reader, (_, receiver) in learned.items():
            for medium in ("active", "inert"):
                tag = f"reunion-{writer}-{reader}-{medium}"
                trace = out / (tag + ".tsv")
                run(
                    tag,
                    *common,
                    "--catalytic-pair",
                    sender,
                    receiver,
                    "--catalytic-tests",
                    1024,
                    "--catalytic-history",
                    183,
                    "--catalytic-test-seed",
                    14081839,
                    "--catalytic-medium",
                    medium,
                    "--catalytic-trace",
                    trace,
                )
                checks.append(
                    dict(
                        condition=tag, **correspondence(read(trace), medium == "inert")
                    )
                )
    if checks:
        with (out / "reunions.csv").open("x") as f:
            w = csv.DictWriter(
                f,
                fieldnames=list(dict.fromkeys(k for r in checks for k in r)),
                lineterminator="\n",
            )
            w.writeheader()
            w.writerows(checks)
    print(
        f"{len(learned)} distinct conventions met the prespecified threshold. Evidence is in {out}."
    )


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--binary", type=Path, default=ROOT / "axion")
    p.add_argument("--population", choices=("brook", "slow"), default="brook")
    p.add_argument("--residents", type=int, default=2048)
    p.add_argument("--advance", action="store_true")
    a = p.parse_args()
    study(a.output.resolve(), a.binary.resolve(), a.population, a.residents, a.advance)
