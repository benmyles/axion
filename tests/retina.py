"""Reconstruct optical learning, route order and learned words on a living CUDA body."""

import csv
import importlib.util
import os
import subprocess
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT

spec = importlib.util.spec_from_file_location(
    "retina_audit", ROOT / "tools/retina-audit.py"
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
examples = ROOT / "examples"
beds = ROOT / "docs/observational-learning/coast-beds.txt"


def test_field(d):
    def run(*args):
        return call("floodplain", "--brain-file", "compact", *args)

    home = d / "watershed"
    with home.open("wb") as f:
        subprocess.run(
            ["zstd", "-qdc", examples / "cascade-colony.flood.zst"],
            stdout=f,
            check=True,
        )

    def panel(name, *args):
        trace = d / name
        log = run(
            "--load",
            home,
            "--slot",
            13283,
            "--steps",
            0,
            "--retina-trials",
            16,
            "--inlet-prior",
            8,
            "--inlet-practice",
            16,
            "--inlet-state",
            "carried",
            "--retina-beds",
            2,
            "--retina-order",
            "woven",
            "--retina-works",
            beds,
            "--retina-trace",
            trace,
            "--class-seed",
            3733,
            *args,
        )
        totals = audit.audit(trace, surfaces=audit.load_surfaces(beds))
        row = rows(log, "retinal_panel")[-1]
        assert totals["courses"] == 32 and totals["correct"] == int(row["first"])
        assert totals["pair_right"] == int(row["pair_right"])
        return row

    panel("optics")
    panel("independent", "--retina-channel", "independent")
    with (d / "optics").open() as live, (d / "independent").open() as unpaired:
        a = csv.DictReader(live, delimiter="\t")
        b = csv.DictReader(unpaired, delimiter="\t")
        changed = 0
        for actual, disrupted in zip(a, b, strict=True):
            left = list(map(int, actual["glints"].split(",")))
            right = list(map(int, disrupted["glints"].split(",")))
            assert [v >> 8 for v in left] == [v >> 8 for v in right]
            for offset in range(0, len(left), 4):
                endpoints = lambda readings: sorted(
                    v & 255 for v in readings[offset : offset + 4]
                )
                assert endpoints(left) == endpoints(right)
            changed += left != right
        assert changed > 0
    closed = panel("closed", "--retina-channel", "closed")
    assert int(closed["synaptic_edits"]) == 0 and closed["first"] == closed["untouched"]
    args = (
        "--daylight",
        "live",
        "--daylight-period",
        4,
        "--daylight-banks",
        "tidal",
        "--drift-windings",
        "woven",
        "--shore-depth",
        "shoals",
    )
    whole = d / "whole"
    half = d / "half"
    resumed = d / "resumed"
    log = run(
        "--load",
        home,
        *args,
        "--steps",
        48,
        "--save",
        whole,
        "--record-watercourses",
        d / "raw",
        "--watercourse-sites",
        47,
    )
    run("--load", home, *args, "--steps", 16, "--save", half)
    run("--load", half, "--steps", 32, "--save", resumed)
    assert whole.read_bytes() == resumed.read_bytes()
    light = rows(log, "daylight")[-1]
    courses = rows(log, "woven_courses")[-1]
    assert 0 < int(light["attended"]) == int(light["heat"]) <= int(light["eyes"])
    assert (
        0
        < int(courses["order_matters"])
        <= int(courses["serial"])
        <= int(courses["flows"])
    )
    assert (
        int(light["bytes_per_resident"]) == 0
        and len(audit.load_surfaces(d / "raw")) == 47
    )
    for sources in ("other", "independent"):
        sample = rows(
            run(
                "--load",
                home,
                "--daylight",
                "displaced",
                "--glint-sources",
                sources,
                "--daylight-period",
                2,
                "--steps",
                16,
            ),
            "daylight",
        )[-1]
        total = int(sample["unpaired"])
        same = int(sample["same_source"])
        outlet = int(sample["same_endpoint"])
        assert (
            total > 0
            and 0 <= same <= outlet <= total
            and (same > 0) == (sources == "independent")
        )
    for action in ("live", "mute"):
        out = d / (action + ".tsv")
        log = run(
            "--load",
            home,
            "--slot",
            13283,
            "--steps",
            0,
            "--retina-trials",
            32,
            "--inlet-practice",
            64,
            "--inlet-prior",
            32,
            "--inlet-state",
            "carried",
            "--retina-beds",
            4,
            "--retina-laps",
            21,
            "--inlet-landscape",
            "channels",
            "--retina-terms",
            examples / "cascade-terms.txt",
            "--retina-questions",
            examples / "inlet-questions.txt",
            "--retina-hearing",
            action,
            "--retina-voice",
            out,
        )
        result = audit.audit(
            out,
            True,
            glosses=audit.read_glosses(
                examples / "cascade-terms.txt", examples / "inlet-questions.txt"
            ),
        )
        row = rows(log, "cascade_interview")[-1]
        assert (
            result["courses"] == 64
            and result["correct"] == int(row["correct"])
            and result["combined"] > 0
        )
        assert result["premature"] == result["no_scene"] == 0
        if action == "mute":
            assert result["voiced"] == 0
        else:
            assert result["combined_right"] > 0 and result["heard"] == 64


def test_words(d):
    pupil = examples / "cascade-pupil.mind"
    speaker = examples / "cascade-speaker.mind"
    listener = examples / "parallax-witness.mind"
    lesson = (examples / "cascade-lessons.txt").read_text()
    for light in ("crowded", "fixed"):
        trained = d / f"{light}.mind"
        log = call(
            "talk",
            "--speaker",
            pupil,
            "--listener",
            listener,
            "--save-speaker",
            trained,
            stdin=lesson.replace("crowded", light),
        )
        notes = d / (light + ".log")
        notes.write_text(log)
        stats = audit.audit_words(notes)
        assert (
            stats["words"] == 4
            and stats["exposures"] == 24
            and stats["ambiguous"] == stats["stages"]
        )
        assert stats["decoded"] == (4 if light == "crowded" else 0)
        if light == "crowded":
            assert stats["identifiable"] == 4
            assert trained.read_bytes() == speaker.read_bytes()
    conflict = call(
        "talk",
        "--speaker",
        speaker,
        "--listener",
        listener,
        stdin="/cascade 796669 4 8 bright\n/cascade-name amber 01 2 crowded\n/cascade-name rill 123 2 crowded\n/quit\n",
    )
    refusals = [r for r in rows(conflict, "cascade") if r["cascade"] == "name"]
    assert len(refusals) == 2 and all(
        int(r["status"]) == 3 and int(r["heat"]) == 0 for r in refusals
    )
    demo = (examples / "cascade-conversation.txt").read_text()
    log = call("talk", "--speaker", speaker, "--listener", listener, stdin=demo)
    lines = [s.split("> ", 1)[1] for s in log.splitlines() if s.startswith("speaker> ")]
    assert lines[0] != lines[1] and lines[0] == lines[2]
    sequence = [r for r in rows(log, "cascade") if r["cascade"] == "heard"]
    assert [int(r["recognized"]) for r in sequence] == [5, 5]


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(
        prefix="axion-retina-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as t:
        d = Path(t)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        test_field(d)
        test_words(d)
    print(
        "Optical history, field restart, two-layer physics, lexical cortex and spoken intervention passed on CUDA."
    )
