#!/usr/bin/env python3
"""Select on shared food, rear descendants in the watershed, then examine unsolicited communication."""

import argparse
import json
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EX = ROOT / "examples"


def study(out, seed, medium, selection):
    binary = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()
    child = EX / "cascade-speaker.mind"
    water = ROOT / "docs/observational-learning"
    common = ["--audience-pupil", child, "--audience-rounds", 2]

    def run(name, argv, text=""):
        with (out / (name + ".log")).open("x") as f:
            p = subprocess.run(
                [binary, *map(str, argv)],
                check=False,
                cwd=ROOT,
                input=text,
                text=True,
                stdout=f,
                stderr=subprocess.STDOUT,
            )
        body = (out / (name + ".log")).read_text()
        if p.returncode:
            raise RuntimeError(f"{name} failed in {out}")
        return {
            line.split("=", 1)[0]: dict(
                x.split("=", 1) for x in line.split() if "=" in x
            )
            for line in body.splitlines()
            if "=" in line
        }

    with tempfile.TemporaryDirectory(dir=out) as work:
        home = Path(work) / "home.flood"
        with home.open("wb") as f:
            subprocess.run(
                ["zstd", "-qdc", EX / "inquiry-colony.flood.zst"], stdout=f, check=True
            )
        field = ["floodplain", "--load", home]
        germ = out / "audience.germ"
        result = run(
            "nursery",
            [
                *field,
                "--steps",
                0,
                *common,
                "--audience-guide",
                EX / "inquiry-speaker.mind",
                "--audience-trials",
                256,
                "--audience-medium",
                medium,
                "--audience-broods",
                384,
                "--inlet-cohort",
                2048,
                "--inlet-bolus",
                32,
                "--inlet-selection",
                selection,
                "--class-seed",
                seed,
                "--audience-write",
                germ,
            ],
        )
        colony = out / "colony.flood"
        result.update(
            run(
                "watershed",
                [
                    *field,
                    "--steps",
                    4096,
                    "--report-every",
                    1024,
                    "--daylight-probes",
                    "stirred",
                    "--daylight-audience",
                    "visible",
                    "--receive-count",
                    16384,
                    "--audience-inherit",
                    germ,
                    "--save",
                    colony,
                    "--probe-trace",
                    out / "watershed.tsv",
                    "--probe-sampling",
                    "events",
                    *common,
                    "--audience-juveniles",
                    4096,
                    "--audience-medium",
                    "tracer",
                    "--audience-trials",
                    128,
                    "--retina-born-after",
                    5120,
                    "--class-seed",
                    38971,
                    "--inlet-bolus",
                    32,
                ],
            )
        )
        juvenile = result["audience_juveniles"]
        if not int(juvenile["selected"]):
            return result
        young, adult = out / "young.mind", out / "guide.mind"
        field = ["floodplain", "--load", colony, "--steps", 0]
        result.update(
            run("extract", [*field, "--slot", juvenile["slot"], "--export-mind", young])
        )
        taught = Path(work) / "taught"
        run(
            "english",
            [
                "talk",
                "--speaker",
                young,
                "--listener",
                EX / "parallax-witness.mind",
                "--save-speaker",
                taught,
            ],
            (EX / "inlet-lessons.txt").read_text(),
        )
        run(
            "return",
            [
                *field,
                "--follow-witness",
                young,
                "--return-lesson",
                young,
                taught,
                "--export-mind",
                adult,
                "--save",
                out / "educated.flood",
            ],
        )
        result["transfer"] = []
        for treatment in (
            "intact",
            "exchanged",
            "screened",
            "muted",
            "reversed",
            "disguised",
            "private",
        ):
            for coastal in (False, True):
                tag = f"{treatment}-{'coast' if coastal else 'rig'}"
                stats = run(
                    tag,
                    [
                        *field,
                        *common,
                        "--audience-guide",
                        adult,
                        "--audience-trials",
                        512,
                        "--class-seed",
                        87509,
                        "--audience-arm",
                        treatment,
                        "--audience-trace",
                        out / (tag + ".tsv"),
                        *(
                            ["--retina-works", water / "coast-beds.txt"]
                            if coastal
                            else []
                        ),
                    ],
                )
                result["transfer"].append(stats["audience_exchange"])
        native, pupil = out / "prepared-guide.mind", out / "prepared-pupil.mind"
        result["alignment"] = run(
            "listening",
            [
                "talk",
                "--speaker",
                adult,
                "--listener",
                child,
                "--save-speaker",
                native,
                "--save-listener",
                pupil,
            ],
            (EX / "audience-lessons.txt").read_text(),
        )["public_practice"]
        for name in ("audience-conversation.txt", "audience-other-pupil.txt"):
            run(
                Path(name).stem,
                ["talk", "--speaker", native, "--listener", pupil],
                (EX / name).read_text(),
            )
        return result


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("output", type=Path)
    ap.add_argument("--seed", type=int, default=61441)
    ap.add_argument("--medium", choices=("tracer", "speech"), default="tracer")
    ap.add_argument("--selection", choices=("food", "uniform"), default="food")
    args = ap.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    result = study(out, args.seed, args.medium, args.selection)
    (out / "study.json").write_text(json.dumps(result, indent=2) + "\n")
