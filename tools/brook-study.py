#!/usr/bin/env python3
"""Raise two watershed residents and archive their lessons, utterances and physical receipts."""

import argparse
import csv
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def reproduce(out, binary, seed):
    out.mkdir(parents=True, exist_ok=False)
    origin, colony = out / "origin.flood", out / "watershed.flood"
    with origin.open("xb") as f:
        subprocess.run(
            ["zstd", "-qdc", ROOT / "examples/audience-colony.flood.zst"],
            check=True,
            stdout=f,
        )

    def run(label, *commands, stdin=None):
        path = out / (label + ".log")
        with path.open("x") as f:
            subprocess.run(
                [binary, *map(str, commands)],
                input=stdin,
                text=True,
                cwd=ROOT,
                stdout=f,
                stderr=subprocess.STDOUT,
                check=True,
            )
        return path.read_text()

    field = (
        "floodplain",
        "--load",
        origin,
        "--steps",
        4096,
        "--report-every",
        1024,
        "--daylight-ink",
        "chorus",
    )
    run(
        "births",
        *field,
        "--save",
        colony,
        "--story-residents",
        1536,
        "--story-min-events",
        5,
        "--retina-born-after",
        9216,
        "--story-singles",
        144,
        "--story-pairs",
        36,
        "--story-trials",
        32,
        "--class-seed",
        30113,
        "--story-screen",
        out / "population.tsv",
    )
    with (out / "population.tsv").open() as table:
        cohort = list(csv.DictReader(table, delimiter="\t"))
    candidates = sorted(cohort, key=lambda x: -int(x["correct"]))
    guide, pupil = [r["slot"] for r in candidates[:2]]
    raw, learned, returned = (
        out / "guide-young.mind",
        out / "guide-taught.mind",
        out / "guide.mind",
    )
    run(
        "witness",
        *field,
        "--slot",
        guide,
        "--export-mind",
        raw,
        "--ink-follow",
        guide,
        "--ink-events",
        out / "all-events.tsv",
    )
    classroom = ("floodplain", "--load", colony, "--steps", 0)
    run(
        "tuition",
        *classroom,
        "--story-pupil",
        raw,
        "--class-seed",
        seed,
        "--story-trials",
        128,
        "--story-length",
        7,
        "--story-references",
        64,
        "--story-reference-trials",
        512,
        "--story-save",
        learned,
        "--story-trace",
        out / "reports.tsv",
        "--story-reference-trace",
        out / "questions.tsv",
    )
    run(
        "return",
        *classroom,
        "--return-lesson",
        raw,
        learned,
        "--follow-witness",
        raw,
        "--export-mind",
        returned,
        "--save",
        out / "continuation.flood",
    )
    young, graduate = out / "pupil-young.mind", out / "pupil.mind"
    run("pupil-birth", *classroom, "--slot", pupil, "--export-mind", young)
    run(
        "pupil-school",
        *classroom,
        "--story-pupil",
        young,
        "--story-mentor",
        returned,
        "--class-seed",
        616451,
        "--story-references",
        80,
        "--story-reference-trials",
        128,
        "--story-trials",
        64,
        "--story-length",
        7,
        "--story-save",
        graduate,
        "--story-trace",
        out / "peer-reports.tsv",
        "--story-reference-trace",
        out / "peer-questions.tsv",
    )
    for label, script in (
        ("conversation", "brook-conversation.txt"),
        ("hearing", "brook-hearing.txt"),
    ):
        print(
            run(
                label,
                "talk",
                "--speaker",
                returned,
                "--listener",
                graduate,
                stdin=(ROOT / "examples" / script).read_text(),
            )
        )
    print(f"Resident {guide} and its pupil {pupil} are in {out}.")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "destination",
        type=Path,
        help="a new directory for this family and its evidence",
    )
    p.add_argument(
        "--binary", type=Path, default=Path(os.environ.get("AXION_BIN", ROOT / "axion"))
    )
    p.add_argument(
        "--seed",
        type=int,
        default=49747,
        help="seed for the guide's external instruction",
    )
    args = p.parse_args()
    reproduce(args.destination.resolve(), args.binary.resolve(), args.seed)
