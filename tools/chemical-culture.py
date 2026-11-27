#!/usr/bin/env python3
"""Compare single-partner and overlapping apprenticeships with actual births, pressure signals and finite food."""

import argparse
import csv
import os
import subprocess
from pathlib import Path
from capillary_audit import read
from catalytic_audit import correspondence, pairs

ROOT = Path(__file__).resolve().parents[1]


def study(out, binary, n, stages, widths, selection):
    out.mkdir(parents=True, exist_ok=False)
    env = {**os.environ, "AXION_JOURNAL_DIR": str(out / "journal")}
    home = out / "brook.flood"
    with home.open("xb") as f:
        subprocess.run(
            ["zstd", "-qdc", ROOT / "examples/capillary-nursery.flood.zst"],
            stdout=f,
            check=True,
        )

    def run(tag, *args):
        with (out / (tag + ".log")).open("x") as f:
            subprocess.run(
                list(
                    map(
                        str, [binary, "floodplain", "--load", home, "--steps", 0, *args]
                    )
                ),
                cwd=ROOT,
                env=env,
                stdout=f,
                stderr=subprocess.STDOUT,
                check=True,
            )

    def original(code, role):
        return (
            ROOT / "examples" / f"context-{code}-mark-{('sender','reader')[role]}.mind"
        )

    choices = {}
    for role in (0, 1):
        tutor = ("receiver", "sender")[role]
        pupil = ("sender", "receiver")[role]
        groups = {}
        for code in ("left", "right"):
            for pay in ("food", "unrelated"):
                tag = f"tuition-{tutor}-{code}-{pay}"
                p = out / (tag + ".tsv")
                run(
                    tag,
                    "--capillary-residents",
                    n,
                    "--retina-born-after",
                    18432,
                    "--capillary-contacts",
                    0,
                    "--catalytic-tutor",
                    tutor,
                    original(code, 1 - role),
                    "--catalytic-lessons",
                    512,
                    "--catalytic-feedback",
                    pay,
                    "--catalytic-histories",
                    8,
                    "--catalytic-tests",
                    64,
                    "--class-seed",
                    3689843,
                    "--capillary-screen",
                    p,
                )
                group = read(p)
                pairs(group)
                if pay == "food":
                    groups[code] = group
        left, right = groups["left"], groups["right"]
        assert [r[pupil] for r in left] == [r[pupil] for r in right]
        choices[role] = [
            a[pupil]
            for a, b in zip(left, right, strict=True)
            if selection == "first"
            or (a["reliable_left"] >= 6 and b["reliable_right"] >= 6)
        ]
    used = {9434, 888}
    births = []
    for stage in range(stages):
        role = 1 - stage % 2
        slot = next((s for s in choices[role] if s not in used), None)
        if slot is None:
            break
        used.add(slot)
        subject = out / f"birth-{stage}-{slot}.mind"
        run(f"birth-{stage}", "--slot", slot, "--export-mind", subject)
        births.append((role, subject))
    exams = []
    for width in widths:
        final = {}
        for meaning in ("left", "right"):
            alive = [[original(meaning, 0)], [original(meaning, 1)]]
            for stage, (i, child) in enumerate(births):
                adults = alive[1 - i]
                pair = [None, None]
                pair[i] = child
                pair[1 - i] = adults[-1]
                tag = f"village-{width}-{meaning}-{stage}"
                stored = [
                    out / (tag + "-" + role + ".mind") for role in ("sender", "reader")
                ]
                p = out / (tag + ".tsv")
                neighbours = [
                    arg
                    for mentor in adults[:-1]
                    for arg in ("--catalytic-neighbour", mentor)
                ]
                run(
                    tag,
                    "--catalytic-pair",
                    *pair,
                    *neighbours,
                    "--catalytic-learning",
                    ("sender", "receiver")[i],
                    "--catalytic-lessons",
                    1024,
                    "--class-seed",
                    2191021 + stage * 3421,
                    "--catalytic-history",
                    7,
                    "--catalytic-tests",
                    512,
                    "--catalytic-test-seed",
                    3290281 + stage * 4973,
                    "--catalytic-export",
                    *stored,
                    "--catalytic-trace",
                    p,
                )
                correspondence(read(p))
                alive[1 - i][-1] = stored[1 - i]
                alive[i] = (alive[i] + [stored[i]])[-width:]
            final[meaning] = alive
        for sender in ("left", "right"):
            for receiver in ("left", "right"):
                total = correct = erased = 0
                for x, a in enumerate(final[sender][0]):
                    for y, b in enumerate(final[receiver][1]):
                        tag = f"final-{width}-{sender}-{receiver}-{x}-{y}"
                        p = out / (tag + ".tsv")
                        run(
                            tag,
                            "--catalytic-pair",
                            a,
                            b,
                            "--class-seed",
                            970061,
                            "--catalytic-history",
                            5,
                            "--catalytic-tests",
                            256,
                            "--catalytic-test-seed",
                            6529057,
                            "--catalytic-trace",
                            p,
                        )
                        r = correspondence(read(p))
                        total += r["cases"]
                        correct += r["correct"]
                        erased += r["erased"]
                exams.append(
                    dict(
                        width=width,
                        replacements=len(births),
                        sender=sender,
                        receiver=receiver,
                        correct=correct,
                        cases=total,
                        erased=erased,
                    )
                )
    with (out / "final-communities.csv").open("x") as f:
        w = csv.DictWriter(f, fieldnames=list(exams[0]), lineterminator="\n")
        w.writeheader()
        w.writerows(exams)


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--binary", type=Path, default=ROOT / "axion")
    p.add_argument("--residents", type=int, default=2048)
    p.add_argument("--stages", type=int, default=12)
    p.add_argument(
        "--widths", type=int, choices=range(1, 33), nargs="+", default=[1, 4]
    )
    p.add_argument("--selection", choices=("screened", "first"), default="screened")
    a = p.parse_args()
    study(
        a.output.resolve(),
        a.binary.resolve(),
        a.residents,
        a.stages,
        a.widths,
        a.selection,
    )
