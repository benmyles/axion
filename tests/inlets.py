"""Replay held-out inlet decisions, their physical releases and the pupil's sealed speech on CUDA."""

import gzip
import importlib.util
import json
import os
import subprocess
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT

spec = importlib.util.spec_from_file_location(
    "drift_audit", ROOT / "tools/drift-audit.py"
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
examples = ROOT / "examples"


def main():
    with tempfile.TemporaryDirectory(
        prefix="axion-inlets-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as root:
        d = Path(root)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")

        def field(*args):
            return call("floodplain", *args)

        source = d / "coast"
        current = d / "current"
        stream = d / "flow.tsv"
        conditions = (
            "--inlet-background",
            "living",
            "--inlet-history",
            "living",
            "--inlet-old-inlets",
            "all",
            "--inlet-prior",
            12,
            "--inlet-landscape",
            "mixed",
            "--inlet-trials",
            16,
            "--inlet-practice",
            15,
            "--inlet-residents",
            64,
            "--class-seed",
            3319,
        )

        def survey(tag):
            return (
                "--inlet-table",
                d / (tag + "-table"),
                "--inlet-trace",
                d / (tag + "-cases"),
                *conditions,
            )

        log = field(
            "--side",
            32,
            "--density",
            100,
            "--neurons",
            256,
            "--neurogenesis",
            "inherited",
            "--drift",
            "tidal",
            "--drift-lateral",
            "contextual",
            "--drift-crossing",
            "seep",
            "--drift-stores",
            "sills",
            "--drift-stride",
            1,
            "--drift-load",
            64,
            "--seed",
            3797,
            "--drift-first",
            1024,
            "--drift-trace",
            stream,
            "--drift-window",
            48,
            "--drift-tiles",
            48,
            "--brain-file",
            "compact",
            "--steps",
            96,
            "--save",
            source,
            *survey("fresh"),
        )
        field("--load", source, "--steps", 0, *survey("saved"))
        # An active device row may never have existed in host memory. A survey
        # before save must see precisely the tissue subsequently serialized.
        for item in ("table", "cases"):
            assert (d / ("fresh-" + item)).read_bytes() == (
                d / ("saved-" + item)
            ).read_bytes()
        assert audit.check_trace(stream)["pulses"] > 0
        summary = audit.check_inlets(d / "fresh-cases")
        assert summary["courses"] == 32 and summary["local"] == 8
        spill = rows(log, "inlet_storage")[-1]
        carbon = rows(log, "drift_tick")[-1]
        assert int(spill["wet_pools"]) >= int(spill["held"]) > 0
        assert int(spill["released"]) + int(spill["trickled"]) == int(
            carbon["released"]
        )
        assert int(spill["swallowed"]) + int(spill["shallow_meals"]) == int(
            carbon["eaten"]
        )
        direct = d / "direct"
        field("--load", source, "--steps", 64, "--save", direct, *survey("advanced"))
        field("--load", source, "--steps", 24, "--save", current)
        resumed = d / "resumed"
        field("--load", current, "--steps", 40, "--save", resumed, *survey("split"))
        assert direct.read_bytes() == resumed.read_bytes()
        for item in ("table", "cases"):
            assert (d / ("advanced-" + item)).read_bytes() == (
                d / ("split-" + item)
            ).read_bytes()
        for arm in ("food", "uniform"):
            result = field(
                "--load",
                source,
                "--steps",
                0,
                *conditions,
                "--inlet-generations",
                8,
                "--inlet-cohort",
                128,
                "--inlet-selection",
                arm,
                "--inlet-bolus",
                32,
            )
            books = rows(result, "inlet_nursery")[0]
            assert int(books["eaten"]) == 8 * int(books["births"]) + int(
                books["reserve"]
            )
            assert int(books["eaten"]) + int(books["settled"]) == (12 + 15 + 32) * int(
                books["adult_lives"]
            )

        # Exercise the normal conversation and compare identical histories
        # up to the user's physical intervention. The post-turn answer changes.
        dialogue = (examples / "inlet-conversation.txt").read_text()

        def speak(script):
            return call(
                "talk",
                "--speaker",
                examples / "inlet-speaker.mind",
                "--listener",
                examples / "parallax-witness.mind",
                stdin=script,
            )

        def said(text):
            return [
                line
                for line in text.splitlines()
                if line.startswith(("speaker>", "speaker asks>", "animal>"))
            ]

        shifted = said(speak(dialogue))
        straight = said(speak(dialogue.replace("/inlet-turn\n", "")))
        assert shifted[:4] == [
            "speaker> north",
            "speaker asks> where did amber end?",
            "speaker> east",
            "speaker> east",
        ]
        assert straight[:4] == [shifted[0], shifted[1], "speaker> north", shifted[3]]
        assert shifted[-2:] == ["animal> (silence)"] * 2
        colony = d / "colony"
        subprocess.run(
            ["zstd", "-q", "-d", examples / "inlet-colony.flood.zst", "-o", colony],
            check=True,
        )

        def interview(arm):
            trace = d / (arm + "-voice")
            outcome = field(
                "--load",
                colony,
                "--steps",
                0,
                "--slot",
                10969,
                "--drift-lateral",
                arm,
                "--inlet-trials",
                64,
                "--inlet-landscape",
                "mixed",
                "--inlet-prior",
                24,
                "--inlet-practice",
                24,
                "--inlet-state",
                "carried",
                "--class-seed",
                73013,
                "--inlet-questions",
                examples / "inlet-questions.txt",
                "--inlet-voice-trace",
                trace,
            )
            independent = audit.check_inlet_voice(trace)
            recorded = rows(outcome, "inlet_conversation")[0]
            assert all(v == int(recorded[k]) for k, v in independent.items())
            return independent

        joined = interview("contextual")
        # The atlas must describe the four-by-four memory of each actual
        # ranked specimen in this census and both of its expressed addresses.
        captured = sorted((d / "journal" / "runs").glob("*/observations.json.gz"))[-1]
        with gzip.open(captured, "rt") as f:
            raw = json.load(f)
        groups = [
            p["measures"] for p in raw["specimens"] if p["role"] == "morphospecies"
        ]
        assert len(groups) == 8
        for q in groups:
            assert 0 <= q["inlet_contacts"] < 65536 and 0 <= q["inlet_wiring"] < 131072
            assert 0 <= q["inlet_motor_band"] < q["active_cells"] // 32
            assert 0 <= q["inlet_vocal_band"] < q["active_cells"] // 32
        local = interview("local")
        assert (
            joined["foresaw"] == 96
            and joined["courses"] == 128
            and joined["origins"] == 256
        )
        assert (
            local["motor_correct"] == 32
            and joined["motor_correct"] > local["motor_correct"]
        )
        assert joined["no_scene"] == joined["premature"] == joined["cleared_topic"] == 0
        assert joined["revised"] == joined["foresaw"]
    print(
        "Passed physical, developmental, checkpoint and prospective conversation replays."
    )


if __name__ == "__main__":
    main()
