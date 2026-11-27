#!/usr/bin/env python3
"""Rear an optical germ on food, release its descendants, teach one and examine its speech on CUDA."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / "examples"
BEDS = ROOT / "docs/observational-learning"


def rows(text, key):
    return [
        dict(part.split("=", 1) for part in line.split() if "=" in part)
        for line in text.splitlines()
        if line.startswith(key + "=")
    ]


def study(out, seed, selection, generations):
    out.mkdir(parents=True, exist_ok=True)
    records = {}
    binary = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()

    def run(name, args, lesson=""):
        p = subprocess.run(
            list(map(str, [binary, *args])),
            input=lesson,
            capture_output=True,
            text=True,
            cwd=ROOT,
        )
        (out / (name + ".log")).write_text(p.stdout + p.stderr)
        if p.returncode:
            raise RuntimeError(f"{name} failed. See {out / (name + '.log')}")
        return p.stdout

    with tempfile.TemporaryDirectory(prefix=".watershed-", dir=out) as tmp:
        d = Path(tmp)
        for name in ("nursery", "founders"):
            with (d / name).open("wb") as f:
                subprocess.run(
                    ["zstd", "-qdc", EXAMPLES / f"cascade-{name}.flood.zst"],
                    stdout=f,
                    check=True,
                )
        germ = out / "brood.germ"
        optical = [
            "--inlet-state",
            "carried",
            "--retina-beds",
            2,
            "--retina-works",
            BEDS / "estuary-beds.txt",
        ]
        nursery = run(
            "brood",
            [
                "floodplain",
                "--load",
                d / "nursery",
                "--steps",
                0,
                "--inlet-residents",
                8192,
                "--retina-host",
                EXAMPLES / "cascade-nursery-host.mind",
                "--retina-trials",
                128,
                *optical,
                "--retina-laps",
                5,
                "--retina-rear-laps",
                5,
                "--inlet-practice",
                32,
                "--inlet-prior",
                16,
                "--inlet-generations",
                generations,
                "--inlet-cohort",
                8192,
                "--inlet-selection",
                selection,
                "--inlet-bolus",
                128,
                "--class-seed",
                seed,
                "--retina-germ",
                germ,
            ],
        )
        records["brood"] = rows(nursery, "retinal_brood")[-1]
        if not int(records["brood"]["survivors"]):
            return records
        natal = run(
            "watershed",
            [
                "floodplain",
                "--load",
                d / "founders",
                "--retina-donor",
                germ,
                "--receive-count",
                16384,
                "--drift-windings",
                "woven",
                "--steps",
                2048,
                "--report-every",
                1024,
                "--drift-first",
                8192,
                "--retina-born-after",
                512,
                "--retina-trials",
                64,
                *optical,
                "--retina-order",
                "woven",
                "--inlet-practice",
                48,
                "--inlet-prior",
                24,
                "--class-seed",
                69991,
                "--save",
                d / "births",
            ],
        )
        found = rows(natal, "retinal_panel")
        if not found:
            records["watershed"] = rows(natal, "retinal_census")[-1]
            return records
        records["watershed"] = found[-1]
        slot = int(found[0]["slot"])
        pupil = out / "pupil.mind"
        speaker = out / "speaker.mind"
        resident = out / "resident.mind"
        run(
            "extract",
            [
                "floodplain",
                "--load",
                d / "births",
                "--slot",
                slot,
                "--steps",
                0,
                "--export-mind",
                pupil,
            ],
        )

        def talk(person):
            return [
                "talk",
                "--speaker",
                person,
                "--listener",
                EXAMPLES / "parallax-witness.mind",
            ]

        run(
            "english",
            [*talk(pupil), "--save-speaker", d / "english"],
            (EXAMPLES / "inlet-lessons.txt").read_text(),
        )

        def bring(home, original, learned, export, world):
            return [
                "floodplain",
                "--load",
                home,
                "--steps",
                0,
                "--follow-witness",
                original,
                "--return-lesson",
                original,
                learned,
                "--export-mind",
                export,
                "--save",
                world,
            ]

        run(
            "return-english",
            bring(d / "births", pupil, d / "english", resident, d / "english-field"),
        )
        head = "/cascade 796669 4 8 bright\n"
        opened = run("lamps", talk(resident), head + "/quit\n")
        lamps = rows(opened, "cascade")[-1]["gate_names"].split(",")
        successful = len(set(lamps)) == 4 and "?" not in lamps
        home = d / "english-field"
        if successful:
            lesson = "/braid-feedback speaker auditory\n" + head
            for line in (EXAMPLES / "cascade-terms.txt").read_text().splitlines():
                name, *colors = line.split()
                route = "".join(str(lamps.index(color)) for color in colors)
                lesson += f"/cascade-name {name} {route} 6 crowded\n"
            lesson += "/quit\n"
            (out / "lesson.txt").write_text(lesson)
            tutoring = run(
                "names", [*talk(resident), "--save-speaker", speaker], lesson
            )
            records["names"] = [
                {
                    k: r[k]
                    for k in ("word", "demonstrations", "resolved_stages", "contacts")
                }
                for r in rows(tutoring, "cascade")
                if r["cascade"] == "name"
            ]
            run(
                "return-names",
                bring(home, resident, speaker, d / "returned", d / "named-field"),
            )
            home = d / "named-field"
        for label, changes in (
            (
                "foreign-bed",
                [
                    "--retina-beds",
                    2,
                    "--retina-works",
                    BEDS / "coast-beds.txt",
                    "--retina-order",
                    "woven",
                    "--inlet-practice",
                    48,
                    "--inlet-prior",
                    24,
                    "--class-seed",
                    814789,
                ],
            ),
            (
                "new-combinations",
                [
                    "--retina-beds",
                    4,
                    "--retina-laps",
                    21,
                    "--inlet-practice",
                    64,
                    "--inlet-prior",
                    32,
                    "--inlet-landscape",
                    "channels",
                    "--retina-terms",
                    EXAMPLES / "cascade-terms.txt",
                    "--class-seed",
                    15667,
                ],
            ),
        ):
            log = run(
                label,
                [
                    "floodplain",
                    "--load",
                    home,
                    "--slot",
                    slot,
                    "--steps",
                    0,
                    "--retina-trials",
                    512,
                    "--inlet-state",
                    "carried",
                    "--retina-questions",
                    EXAMPLES / "inlet-questions.txt",
                    "--retina-voice",
                    out / (label + ".tsv"),
                    *changes,
                ],
            )
            records[label] = rows(log, "cascade_interview")[-1]
    return records


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "output",
        type=Path,
        help="directory for the optical germ, individual minds, logs and held-out trials",
    )
    parser.add_argument("--seed", type=int, default=127877)
    parser.add_argument("--selection", choices=("food", "uniform"), default="food")
    parser.add_argument("--generations", type=int, default=1024)
    args = parser.parse_args()
    output = args.output.resolve()
    try:
        output.mkdir(parents=True)
    except FileExistsError:
        output = Path(tempfile.mkdtemp(prefix="cascade-run-", dir=output))
    print(f"Study directory {output}", flush=True)
    result = study(output, args.seed, args.selection, args.generations)
    report = json.dumps(result, indent=2)
    (output / "study.json").write_text(report + "\n")
    print(report)
