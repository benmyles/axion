#!/usr/bin/env python3
"""Keep one creature's material memory through hidden changes and compare matched interventions on CUDA."""

import argparse
import csv
import os
import re
import subprocess
from pathlib import Path
from capillary_audit import read
from context_audit import choices, stream

ROOT = Path(__file__).resolve().parents[1]


def study(out, binary, streams, rounds, group, large):
    out.mkdir(parents=True, exist_ok=False)
    env = {**os.environ, "AXION_JOURNAL_DIR": str(out / "journal")}

    def run(tag, *args, stdin=""):
        with (out / (tag + ".log")).open("x") as f:
            subprocess.run(
                [binary, *map(str, args)],
                cwd=ROOT,
                env=env,
                text=True,
                input=stdin,
                stdout=f,
                stderr=subprocess.STDOUT,
                check=True,
            )
        return (out / (tag + ".log")).read_text()

    def inflate(archive, name):
        file = out / (name + ".flood")
        with file.open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ROOT / "examples" / archive], stdout=f, check=True
            )
        return file

    brook = inflate("capillary-nursery.flood.zst", "brook")
    summary = []
    for who in ("coast", "brook") if group == "both" else (group,):
        subject = (
            ROOT
            / "examples"
            / ("detritus-guide.mind" if who == "coast" else "capillary-guide.mind")
        )
        for arm in (
            "adaptive",
            "single",
            "cleared",
            "unchecked",
            "clamped",
            "exchanged",
        ):
            tag = "holdout-" + who + "-" + arm
            p = out / (tag + ".tsv")
            log = run(
                tag,
                "floodplain",
                "--load",
                brook,
                "--steps",
                0,
                "--capillary-pupil",
                subject,
                "--context-streams",
                streams,
                "--context-episodes",
                rounds,
                "--context-change",
                37,
                "--context-arm",
                arm,
                "--context-trace",
                p,
                "--class-seed",
                4513049,
                "--capillary-depth",
                10,
                "--capillary-moves",
                5,
                "--capillary-supply",
                "scarce",
                *(
                    ["--context-save", out / (who + "-series.mind")]
                    if arm == "adaptive"
                    else []
                ),
            )
            r = next(
                dict(re.findall(r"([^\s=]+)=([^\s]+)", z))
                for z in log.splitlines()
                if z.startswith("context_series=")
            )
            check = stream(read(p), None, None, physical=False)
            assert check["changed_after"] == int(r["resolved_switch"])
            summary.append({"guide": who, "treatment": arm, **r})
        taught = out / (who + "-bench.mind")
        run(
            who + "-talk",
            "talk",
            "--speaker",
            subject,
            "--listener",
            ROOT / "examples/capillary-listener.mind",
            "--save-speaker",
            taught,
            stdin=(ROOT / "examples/context-conversation.txt").read_text(),
        )
        run(
            who + "-reservoir",
            "talk",
            "--speaker",
            taught,
            "--listener",
            ROOT / "examples/capillary-listener.mind",
            stdin=(ROOT / "examples/context-reservoir.txt").read_text(),
        )
        if who == "coast" and not large:
            continue
        origin = (
            inflate("detritus-estuary.flood.zst", "coast") if who == "coast" else brook
        )
        birth = (
            ROOT
            / "examples"
            / ("detritus-young.mind" if who == "coast" else "capillary-young.mind")
        )
        home = out / (who + "-home.flood")
        events = out / (who + "-field.tsv")
        run(
            who + "-return",
            "floodplain",
            "--load",
            origin,
            "--steps",
            0,
            "--capillary-memory",
            "adaptive",
            "--return-lesson",
            birth,
            taught,
            "--save",
            home,
        )
        run(
            who + "-advance",
            "floodplain",
            "--load",
            home,
            "--steps",
            1024 if who == "coast" else 16384,
            "--report-every",
            256 if who == "coast" else 4096,
            "--capillary-field",
            "live",
            "--capillary-memory",
            "adaptive",
            "--capillary-food",
            "pigments",
            "--capillary-cargo",
            "pressure",
            "--capillary-cargo-dye",
            "exchange",
            "--capillary-period",
            8 if who == "coast" else 4,
            "--context-events",
            events,
            "--save",
            out / (who + "-continued.flood"),
        )
        choices(read(events))
    with (out / "summary.csv").open("w") as f:
        w = csv.DictWriter(f, fieldnames=list(summary[0]), lineterminator="\n")
        w.writeheader()
        w.writerows(summary)
    print(f"Continuous, matched and physical evidence is in {out}.")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--binary", default=ROOT / "axion", type=Path)
    p.add_argument("--streams", type=int, default=384)
    p.add_argument("--episodes", type=int, default=16)
    p.add_argument("--guide", choices=("coast", "brook", "both"), default="both")
    p.add_argument("--large", action="store_true")
    a = p.parse_args()
    study(
        a.output.resolve(), a.binary.resolve(), a.streams, a.episodes, a.guide, a.large
    )
