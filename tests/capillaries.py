#!/usr/bin/env python3
"""Run interventions and conversation through the actual CUDA command and checkpoint paths."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path
from press import ROOT, call, rows

sys.path.insert(0, str(ROOT / "tools"))
import capillary_audit as audit


def run():
    ex = ROOT / "examples"
    guide = ex / "capillary-guide.mind"
    listener = ex / "capillary-listener.mind"
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-pressure-test-"
    ) as root:
        d = Path(root)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        basin = d / "watershed.flood"
        with basin.open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ex / "capillary-nursery.flood.zst"],
                stdout=f,
                check=True,
            )
        flood = lambda *args, **kwargs: call("floodplain", *args, **kwargs)
        classroom = (
            "--load",
            basin,
            "--steps",
            0,
            "--capillary-trials",
            12,
            "--capillary-depth",
            9,
            "--capillary-supply",
            "scarce",
            "--capillary-moves",
            5,
            "--class-seed",
            706961,
        )
        for condition in ("active", "passive", "backwards", "yoked", "lookup"):
            trace = d / (condition + ".tsv")
            result = flood(
                *classroom,
                "--capillary-pupil",
                ex / "capillary-young.mind",
                "--capillary-arm",
                condition,
                "--capillary-trace",
                trace,
            )
            actual = rows(result, "capillary_repair")[0]
            a = audit.audit_repairs(audit.read(trace), condition)
            assert a["earned"] == int(actual["earned"]) and a["solvable"] == int(
                actual["possible"]
            )
            if condition == "active":
                assert a["earned"] == a["solvable"]
        for history in ("retained", "empty"):
            trace = d / f"prior-{history}.tsv"
            flood(
                *classroom,
                "--capillary-pupil",
                guide,
                "--capillary-history",
                history,
                "--capillary-views",
                0,
                "--capillary-probes",
                0,
                "--capillary-trace",
                trace,
            )
            for row in audit.read(trace):
                assert bool(row["on"] | row["off"]) == (history == "retained")
        census = d / "juvenes.tsv"
        flood(
            "--load",
            basin,
            "--steps",
            0,
            "--capillary-residents",
            16,
            "--capillary-trials",
            3,
            "--capillary-history",
            "empty",
            "--capillary-screen",
            census,
        )
        cohort = audit.read(census)
        assert len(cohort) == 16 and all(
            row["capacity"] == 3 + ((row["dendrites"] >> 26) & 3) for row in cohort
        )
        legacy = d / "cold.mind"
        flood("--load", basin, "--steps", 0, "--slot", 7488, "--export-mind", legacy)
        assert legacy.read_bytes() == (ex / "capillary-young.mind").read_bytes()
        for hearing in ("intact", "reversed", "mute", "wrong-name", "closed"):
            words = d / (hearing + "-manual.tsv")
            result = flood(
                *classroom,
                "--capillary-pupil",
                guide,
                "--capillary-partner",
                listener,
                "--capillary-instructions",
                12,
                "--capillary-hearing",
                hearing,
                "--capillary-instruction-trace",
                words,
            )
            truth = audit.audit_orders(audit.read(words), audit.read(d / "active.tsv"))
            v = rows(result, "capillary_instruction")[0]
            assert int(v["after"]) == truth["completed"]
            if hearing == "intact":
                assert int(v["possible"]) == truth["completed"] and truth["novel"] > 0
            if hearing in ("reversed", "mute", "closed"):
                assert not truth["novel"]
        path = d / "replies.tsv"
        flood(
            *classroom,
            "--capillary-pupil",
            guide,
            "--capillary-interviews",
            24,
            "--capillary-discourse",
            path,
        )
        assert audit.audit_speech(audit.read(path))["exact"] > 100
        for user, mind, response in (
            ("capillary-conversation.txt", guide, "the wick let teal through"),
            (
                "capillary-film-conversation.txt",
                ex / "capillary-film-guide.mind",
                "the film let teal through",
            ),
            (
                "capillary-construction.txt",
                guide,
                "lift wick upper left then place water upper middle",
            ),
        ):
            log = call(
                "talk",
                "--speaker",
                mind,
                "--listener",
                listener,
                stdin=(ex / user).read_text(),
            )
            assert response in log
            if "construction" in user:
                assert rows(log, "capillary_turn")[-1]["got"] == "2"
        home = d / "home.flood"
        back = d / "back.mind"
        flood(
            "--load",
            basin,
            "--steps",
            0,
            "--return-lesson",
            legacy,
            guide,
            "--save",
            home,
        )
        flood(
            "--load",
            home,
            "--steps",
            0,
            "--follow-witness",
            legacy,
            "--export-mind",
            back,
        )
        out = call(
            "talk",
            "--speaker",
            back,
            "--listener",
            listener,
            stdin=(ex / "capillary-construction.txt").read_text(),
        )
        assert rows(out, "capillary_turn")[-1]["got"] == "2"
        for size, steps in ((4, 49), (16, 81)):
            start = d / f"tiny-{size}.flood"
            split = d / "split.flood"
            later = d / "later.flood"
            whole = d / "whole.flood"
            flood(
                "--resident",
                guide,
                "--side",
                size,
                "--density",
                0,
                "--capillary-field",
                "live",
                "--capillary-food",
                "pigments",
                "--capillary-period",
                4,
                "--steps",
                0,
                "--save",
                start,
            )
            events = d / f"event-{size}.tsv"
            film = d / f"film-{size}.tsv"
            result = flood(
                "--load",
                start,
                "--steps",
                steps,
                "--save",
                whole,
                "--capillary-events",
                events,
                "--capillary-film",
                film,
                "--capillary-window",
                size,
                "--capillary-frames",
                32,
                "--capillary-delay",
                8,
            )
            assert audit.audit_events(audit.read(events))["observations"] >= 0
            audit.audit_wakes(audit.read(film))
            if size == 4:
                assert int(rows(result, "capillary_wakes")[0]["replay_depth"]) == 3
            flood("--load", start, "--steps", 13, "--save", split)
            flood("--load", split, "--steps", steps - 13, "--save", later)
            assert later.read_bytes() == whole.read_bytes(), (
                "split changed dopamine, pressure or material"
            )
            paused = d / "pause.flood"
            opened = d / "expanded.flood"
            flood(
                "--load",
                later,
                "--steps",
                3,
                "--capillary-field",
                "off",
                "--save",
                paused,
            )
            flood(
                "--load",
                paused,
                "--extend",
                size * 2,
                "--steps",
                8,
                "--capillary-field",
                "live",
                "--save",
                opened,
            )
            flood("--load", opened, "--steps", 0)
        clash = d / "same.tsv"
        msg = flood(
            *classroom,
            "--capillary-pupil",
            guide,
            "--capillary-partner",
            listener,
            "--capillary-instructions",
            1,
            "--capillary-trace",
            clash,
            "--capillary-instruction-trace",
            clash,
            ok=False,
        )
        assert "distinct output" in msg and not clash.exists()
    print(
        "capillaries passed: counterfactuals, scarce stocks, spoken procedures, legacy residents, split and expanded fields"
    )


if __name__ == "__main__":
    run()
