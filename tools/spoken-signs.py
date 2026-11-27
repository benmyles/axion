#!/usr/bin/env python3
"""Rear independently speaking witnesses and listeners, interrupt the link, and verify each acoustic turn."""

import argparse
import csv
import os
from pathlib import Path
import re
import subprocess
from spoken_audit import audit, read

ROOT = Path(__file__).resolve().parents[1]
CONTROLS = (
    ("clear", 0, "actual"),
    ("clear", 2, "actual"),
    ("partial", 0, "actual"),
    ("partial", 1, "actual"),
    ("partial", 1, "withheld"),
    ("partial", 1, "exchanged"),
    ("patchy", 0, "actual"),
    ("patchy", 1, "actual"),
    ("patchy", 2, "actual"),
    ("patchy", 2, "withheld"),
    ("patchy", 2, "exchanged"),
    ("reordered", 2, "actual"),
    ("duplicate", 1, "actual"),
    ("blind", 2, "actual"),
    ("silent", 2, "actual"),
    ("unknown", 2, "actual"),
)


def study(out, binary, habitat, subject, tests, journal):
    out.mkdir(parents=True, exist_ok=False)
    env = {
        **os.environ,
        "AXION_JOURNAL_DIR": str((journal or out / "journal").resolve()),
    }
    if habitat is None:
        habitat = out / "habitat.flood"
        with habitat.open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ROOT / "examples/capillary-nursery.flood.zst"],
                stdout=f,
                check=True,
            )
    common = [
        binary,
        "floodplain",
        "--load",
        habitat,
        "--steps",
        0,
        "--catalytic-channels",
        2,
        "--class-seed",
        6309133,
        "--catalytic-history",
        817,
        "--catalytic-test-seed",
        7012361,
        "--catalytic-tests",
        256,
    ]

    def run(tag, command, script=None):
        p = out / (tag + ".log")
        with p.open("x") as f:
            subprocess.run(
                list(map(str, command)),
                input=script,
                text=True,
                stdout=f,
                stderr=subprocess.STDOUT,
                env=env,
                cwd=ROOT,
                check=True,
            )
        return p.read_text()

    def records(s, key):
        return [
            dict(re.findall(r"([^\s=]+)=([^\s]+)", l))
            for l in s.splitlines()
            if l.startswith(key + "=")
        ]

    summary = []
    tuition = []
    for kind in subject:
        names = [
            ROOT / "examples" / f"compound-{kind}-{r}.mind"
            for r in ("writer", "reader")
        ]
        schools = []
        for dialect in (0, 1):
            tag = f"{kind}-tuition-{dialect}"
            pair = [out / f"{tag}-{r}.mind" for r in ("writer", "reader")]
            s = run(
                tag,
                [
                    *common,
                    "--catalytic-pair",
                    *names,
                    "--catalytic-voices",
                    128,
                    "--catalytic-export",
                    *pair,
                    "--catalytic-writer-dialect",
                    dialect,
                    "--catalytic-reader-dialect",
                    dialect,
                ],
            )
            q = records(s, "signal_vocabulary")
            assert len(q) == 2
            for a in q:
                assert (
                    a["combined_examples"] == "0"
                    and a["pressure_strokes"] == "320"
                    and a["voice_bytes"] == "96"
                )
                tuition.append(dict(run=tag, **a))
            schools.append(pair)
        for channel, budget, mode, wd, rd in [(*c, 0, 0) for c in CONTROLS] + [
            ("clear", 0, "actual", 0, 1),
            ("partial", 1, "actual", 0, 1),
            ("patchy", 2, "actual", 1, 1),
        ]:
            tag = f"{kind}-{channel}-{budget}-{mode}-{wd}{rd}"
            trace = out / (tag + ".tsv")
            s = run(
                tag,
                [
                    *common,
                    "--catalytic-pair",
                    schools[wd][0],
                    schools[rd][1],
                    "--catalytic-dialogues",
                    tests,
                    "--catalytic-audio",
                    channel,
                    "--catalytic-questions",
                    budget,
                    "--catalytic-reply",
                    mode,
                    "--catalytic-writer-dialect",
                    wd,
                    "--catalytic-reader-dialect",
                    rd,
                    "--catalytic-speech-trace",
                    trace,
                ],
            )
            row = records(s, "auditory_feeders")[0]
            verified = audit(read(trace), channel, budget, mode, wd, rd)
            assert all(int(row[k]) == value for k, value in verified.items())
            summary.append(dict(run=tag, channel=channel, replies=mode, **row))
            print(tag, verified["found"], verified["requests"], flush=True)
        for channel, budget, mode in (
            CONTROLS[0],
            CONTROLS[2],
            CONTROLS[3],
            CONTROLS[4],
            CONTROLS[5],
            CONTROLS[8],
        ):
            tag = f"{kind}-live-{channel}-{budget}-{mode}"
            script = (
                (ROOT / "examples/compound-spoken-conversation.txt")
                .read_text()
                .replace(
                    "/cap-look pupil closed",
                    f"/cap-look pupil closed\n/cap-radio {channel} {budget} {mode}",
                )
            )
            run(
                tag,
                [
                    binary,
                    "talk",
                    "--speaker",
                    schools[0][0],
                    "--listener",
                    schools[0][1],
                ],
                script,
            )
    for name, rows in (("spoken", summary), ("tuition", tuition)):
        with (out / (name + ".csv")).open("x") as f:
            w = csv.DictWriter(f, fieldnames=rows[0], lineterminator="\n")
            w.writeheader()
            w.writerows(rows)


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--binary", type=Path, default=ROOT / "axion")
    p.add_argument("--habitat", type=Path)
    p.add_argument(
        "--subject",
        nargs="+",
        choices=("unseen", "joint", "separable"),
        default=["unseen", "joint", "separable"],
    )
    p.add_argument("--tests", type=int, default=1024)
    p.add_argument("--journal", type=Path)
    a = p.parse_args()
    study(
        a.output.resolve(),
        a.binary.resolve(),
        a.habitat.resolve() if a.habitat else None,
        a.subject,
        a.tests,
        a.journal,
    )
