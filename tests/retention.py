#!/usr/bin/env python3
"""Check the CUDA population assay against isolated newborns, their inherited rules and matched sensory replay."""

import os
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

from press import ROOT, call, rows

sys.path.insert(0, str(ROOT / "tools"))
from capillary_audit import read
from context_audit import choices, field, stream


def grades(episodes, at):
    lives = defaultdict(list)
    for r in episodes:
        lives[r["subject"]].append(r)
    first = [sum(r["correct"] for r in v[:at]) for v in lives.values()]
    second = [sum(r["correct"] for r in v[at:]) for v in lives.values()]
    rest = len(episodes) // len(lives) - at
    return dict(
        first=sum(first),
        second=sum(second),
        first_all=first.count(at),
        second_all=second.count(rest),
        whole=sum(a == at and b == rest for a, b in zip(first, second, strict=True)),
        immediate=sum(v[at]["correct"] for v in lives.values()),
        switches=sum(r["correct"] * r["switch"] for r in episodes),
        food=sum(r["caught"] for r in episodes),
    )


def run():
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-heritable-"
    ) as scratch:
        d = Path(scratch)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        origin = d / "brook.flood"
        with origin.open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ROOT / "examples/capillary-nursery.flood.zst"],
                stdout=f,
                check=True,
            )

        def basin(*args):
            return call("floodplain", "--load", origin, "--steps", 0, *args)

        for setting, works in (("adaptive", "assembled"), ("regional", "living")):
            course = [
                "--capillary-memory",
                setting,
                "--context-streams",
                4,
                "--context-episodes",
                8,
                "--context-transfer-at",
                4,
                "--context-transfer-rock",
                9,
                "--context-change",
                37,
                "--capillary-history",
                "empty",
                "--capillary-rock",
                0,
                "--class-seed",
                4609327,
                "--capillary-depth",
                9,
                "--capillary-moves",
                5,
                "--capillary-supply",
                "scarce",
                "--capillary-works",
                works,
            ]
            group = d / (setting + ".tsv")
            log = basin(
                "--capillary-residents",
                512,
                "--retina-born-after",
                18432,
                "--capillary-contacts",
                0,
                "--capillary-screen",
                group,
                *course,
            )
            census = read(group)
            summary = rows(log, "context_population")[0]
            assert len(census) == int(summary["studied"]) == 512
            for r in census:
                assert r["local_lut"] == r["basal_lut"] ^ (
                    r["module_mask"] if setting == "regional" else 0
                )
                assert (
                    r["basal_lut"] == r["word"] & 15
                    and r["paired_lut"] == r["local_lut"] ^ 4
                )
            assert sum(r["native_whole"] == 4 for r in census) == int(summary["full"])
            assert sum(r["paired_whole"] == 4 for r in census) == int(
                summary["paired_full"]
            )
            for select in (10, 14):
                eligible = [
                    r
                    for r in census
                    if r["local_lut"] == select and r["native_first_all"] == 4
                ]
                # A naturally endowed individual whose initially correct causal model breaks under the paired treatment.
                r = max(
                    eligible, key=lambda v: abs(v["native_whole"] - v["paired_whole"])
                )
                assert r["native_whole"] != r["paired_whole"]
                hatched = d / f"{setting}-{select}.mind"
                basin("--slot", r["slot"], "--export-mind", hatched)
                trajectories = []
                for mode, column in (("adaptive", "native"), ("retention", "paired")):
                    trace = d / f"{setting}-{select}-{mode}.tsv"
                    single = basin(
                        "--capillary-pupil",
                        hatched,
                        "--context-arm",
                        mode,
                        "--context-trace",
                        trace,
                        "--context-questions",
                        "varied",
                        *course,
                    )
                    trial = read(trace)
                    result = rows(single, "context_series")[0]
                    assert (
                        int(result["basal_rule"]) == r["basal_lut"]
                        and int(result["module_mask"]) == r["module_mask"]
                    )
                    assert int(result["material_rule"]) == r["local_lut"] ^ (
                        4 if mode == "retention" else 0
                    )
                    stream(trial, None, None)
                    assert {k: r[f"{column}_{k}"] for k in grades(trial, 4)} == grades(
                        trial, 4
                    )
                    assert (
                        sum(z["heat"] for z in trial) == r["energy"]
                        and sum(z["probes"] for z in trial) == r["probes"]
                    )
                    trajectories.append(
                        [
                            (
                                z["fingerprint"],
                                z["heat"],
                                z["views"],
                                z["probes"],
                                z["key"],
                                z["rock"],
                            )
                            for z in trial
                        ]
                    )
                    assert len(rows(single, "context_question")) == 6
                assert trajectories[0] == trajectories[1]
        broad = d / "whole.flood"
        partial = d / "partial.flood"
        joined = d / "joined.flood"
        watches = d / "watches.tsv"
        decisions = d / "choices.tsv"

        def fieldrun(source, ticks, target, *args):
            return call(
                "floodplain",
                "--load",
                source,
                "--steps",
                ticks,
                "--save",
                target,
                *args,
            )

        fieldrun(
            origin,
            273,
            broad,
            "--capillary-field",
            "live",
            "--capillary-memory",
            "regional",
            "--capillary-events",
            watches,
            "--context-events",
            decisions,
        )
        field(read(watches))
        choices(read(decisions))
        fieldrun(
            origin,
            81,
            partial,
            "--capillary-field",
            "live",
            "--capillary-memory",
            "regional",
        )
        fieldrun(partial, 192, joined)
        assert broad.read_bytes() == joined.read_bytes()
    print(
        "Inherited and afferent-specific plasticity passed cohort/individual parity, identical sensory replay, six acoustic forms and exact habitat restart."
    )


if __name__ == "__main__":
    run()
