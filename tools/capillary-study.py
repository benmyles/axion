#!/usr/bin/env python3
"""Raise two saved wild-born residents, test held-out causal language and build scarce channels on CUDA."""

import argparse
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def study(out, binary, pairs):
    out.mkdir(parents=True, exist_ok=False)
    environment = {**os.environ, "AXION_JOURNAL_DIR": str(out / "journal")}

    def run(label, *arguments, stdin=""):
        with (out / (label + ".log")).open("x") as f:
            subprocess.run(
                [binary, *map(str, arguments)],
                cwd=ROOT,
                input=stdin,
                text=True,
                stdout=f,
                stderr=subprocess.STDOUT,
                env=environment,
                check=True,
            )

    def classroom(name):
        return ("floodplain", "--load", out / (name + ".flood"), "--steps", 0)

    for name, archive, slot in (
        ("guide", "nursery", 7488),
        ("listener", "older-nursery", 8599),
    ):
        with (out / (name + ".flood")).open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ROOT / f"examples/capillary-{archive}.flood.zst"],
                stdout=f,
                check=True,
            )
        run(
            name + "-birth",
            *classroom(name),
            "--slot",
            slot,
            "--export-mind",
            out / (name + "-young.mind"),
        )
        for rock in (0, 16) if name == "guide" else (0,):
            key = f"{name}-{rock}"
            b = classroom(name)
            run(
                key + "-material",
                *b,
                "--capillary-pupil",
                out / (name + "-young.mind"),
                "--capillary-trials",
                96,
                "--class-seed",
                109423,
                "--capillary-depth",
                10,
                "--capillary-rock",
                rock,
                "--capillary-vocabulary",
                384,
                "--capillary-interviews",
                pairs,
                "--capillary-discourse",
                out / (key + "-questions.tsv"),
                "--capillary-save",
                out / (key + "-words.mind"),
            )
            run(
                key + "-motor",
                *b,
                "--capillary-pupil",
                out / (key + "-words.mind"),
                "--capillary-trials",
                24,
                "--capillary-depth",
                9,
                "--capillary-supply",
                "scarce",
                "--capillary-moves",
                5,
                "--class-seed",
                706961,
                "--capillary-rock",
                rock,
                "--capillary-recipes",
                1152,
                "--capillary-save",
                out / (key + ".mind"),
            )
    for arm in ("active", "passive", "yoked", "backwards", "miswired", "severed"):
        run(
            "salvage-" + arm,
            *classroom("guide"),
            "--capillary-pupil",
            out / "guide-young.mind",
            "--capillary-supply",
            "scarce",
            "--capillary-trials",
            pairs,
            "--capillary-depth",
            10,
            "--capillary-moves",
            5,
            "--class-seed",
            639461,
            "--capillary-arm",
            arm,
            "--capillary-trace",
            out / f"salvage-{arm}.tsv",
            "--capillary-lesson",
            out / f"salvage-{arm}-lesson.tsv",
        )
    people = (
        "--capillary-pupil",
        out / "guide-0.mind",
        "--capillary-partner",
        out / "listener-0.mind",
    )
    for arm in ("intact", "exchanged", "reversed", "mute"):
        run(
            "advice-" + arm,
            *classroom("guide"),
            *people,
            "--class-seed",
            109423,
            "--capillary-trials",
            96,
            "--capillary-depth",
            10,
            "--capillary-meetings",
            pairs,
            "--capillary-hearing",
            arm,
            "--capillary-exchanges",
            out / f"advice-{arm}.tsv",
        )
        run(
            "orders-" + arm,
            *classroom("guide"),
            *people,
            "--class-seed",
            706961,
            "--capillary-supply",
            "scarce",
            "--capillary-moves",
            5,
            "--capillary-depth",
            9,
            "--capillary-trials",
            24,
            "--capillary-instructions",
            pairs,
            "--capillary-hearing",
            arm,
            "--capillary-instruction-trace",
            out / f"orders-{arm}.tsv",
        )
    run(
        "corresponding-boards",
        *classroom("guide"),
        "--capillary-pupil",
        out / "guide-young.mind",
        "--class-seed",
        706961,
        "--capillary-supply",
        "scarce",
        "--capillary-moves",
        5,
        "--capillary-depth",
        9,
        "--capillary-trials",
        pairs,
        "--capillary-trace",
        out / "instruction-boards.tsv",
    )
    run(
        "event-language",
        *classroom("guide"),
        "--story-pupil",
        out / "guide-0.mind",
        "--class-seed",
        713959,
        "--story-trials",
        128,
        "--story-length",
        7,
        "--story-references",
        64,
        "--story-reference-trials",
        256,
        "--story-save",
        out / "narrator.mind",
    )
    run(
        "return-to-watershed",
        *classroom("guide"),
        "--return-lesson",
        out / "guide-young.mind",
        out / "narrator.mind",
        "--follow-witness",
        out / "guide-young.mind",
        "--save",
        out / "taught.flood",
        "--export-mind",
        out / "returned.mind",
    )
    for script in (
        "construction",
        "conversation",
        "film-conversation",
        "open-questions",
    ):
        speaker = out / (
            "guide-16.mind" if script == "film-conversation" else "returned.mind"
        )
        run(
            "talk-" + script,
            "talk",
            "--speaker",
            speaker,
            "--listener",
            out / "listener-0.mind",
            stdin=(ROOT / f"examples/capillary-{script}.txt").read_text(),
        )
    print(f"GPU checkpoints, experimental receipts and the run journal are in {out}.")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("output", type=Path)
    ap.add_argument("--binary", type=Path, default=ROOT / "axion")
    ap.add_argument("--pairs", type=int, default=512)
    a = ap.parse_args()
    study(a.output.resolve(), a.binary.resolve(), a.pairs)
