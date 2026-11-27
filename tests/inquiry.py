"""Exercise voluntary acquisition, physical inquiries and their checkpointed real outcomes on CUDA."""

import csv
import importlib.util
import os
import subprocess
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT

spec = importlib.util.spec_from_file_location("retina", ROOT / "tools/retina-audit.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
ex = ROOT / "examples"
coast = ROOT / "docs/observational-learning/coast-beds.txt"


def ports(log):
    directions = {"north", "east", "south", "west"}
    return [
        line[9:]
        for line in log.splitlines()
        if line.startswith("speaker> ") and line[9:] in directions
    ]


def main(d):
    os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
    home = d / "colony.flood"
    with home.open("wb") as f:
        subprocess.run(
            ["zstd", "-qdc", ex / "inquiry-colony.flood.zst"], stdout=f, check=True
        )

    def field(*args):
        return call("floodplain", "--load", home, *args)

    args = (
        "--slot",
        5868,
        "--steps",
        0,
        "--retina-trials",
        32,
        "--retina-works",
        coast,
        "--retina-order",
        "woven",
        "--retina-beds",
        2,
        "--inlet-state",
        "carried",
        "--inlet-practice",
        19,
        "--inlet-prior",
        5,
        "--gaze-purse",
        12,
        "--class-seed",
        13603,
    )

    def study(tag, *changes):
        file = d / (tag + ".tsv")
        log = field(*args, *changes, "--retina-trace", file)
        values = audit.audit(file, surfaces=audit.load_surfaces(coast))
        q = rows(log, "gaze_panel")[-1]
        assert (
            int(q["correct"]) == values["correct"] and int(q["heat"]) == values["heat"]
        )
        return q, values

    actual, a = study("chosen", "--retina-gaze", "inherited")
    _, b = study("visible", "--retina-gaze", "visible-yoke")
    assert (
        a["heat"] == b["heat"]
        and a["bought_visible"] == b["bought_visible"]
        and a["heat"] > 0
    )
    paired, _ = study("tracers", "--retina-probes", "chosen", "--gaze-foraging", 4)
    random, _ = study("other", "--retina-probes", "visible-match", "--gaze-foraging", 4)
    assert paired["heat"] == random["heat"] and int(paired["portions"]) == 256
    total = []
    for policy, hearing in (
        ("chosen", "live"),
        ("visible-match", "live"),
        ("chosen", "mute"),
    ):
        f = d / f"{policy}-{hearing}.tsv"
        log = field(
            *args,
            "--inlet-practice",
            0,
            "--inlet-prior",
            0,
            "--retina-gaze",
            "passive",
            "--retina-deliberate",
            32,
            "--inquiry-hand",
            policy,
            "--retina-hearing",
            hearing,
            "--retina-questions",
            ex / "inlet-questions.txt",
            "--retina-voice",
            f,
        )
        q = rows(log, "spoken_inquiry")[-1]
        replayed = audit.audit(f, True, surfaces=audit.load_surfaces(coast))
        assert replayed["query_paid"] == int(q["grain_releases"]) and replayed[
            "prior_correct"
        ] == int(q["correct_before"])
        assert (
            replayed["correct"] == int(q["correct_after"])
            and replayed["premature"] == 0
        )
        total.append(replayed["query_paid"])
    assert total[0] == total[1] > 0 and total[2] == 0
    controls = []
    for strategy in ("heard", "reversed"):
        f = d / (strategy + "-goal.tsv")
        field(
            *args,
            "--inlet-practice",
            0,
            "--inlet-prior",
            0,
            "--retina-gaze",
            "passive",
            "--retina-deliberate",
            32,
            "--inquiry-context",
            strategy,
            "--retina-questions",
            ex / "inlet-questions.txt",
            "--retina-voice",
            f,
        )
        controls.append(f)
    assert audit.exchange_questions(*controls)["courses"] == 64
    stream = d / "grains.tsv"
    whole = d / "whole"
    mid = d / "mid"
    again = d / "again"
    flags = (
        "--daylight-gaze",
        "inherited",
        "--daylight-probes",
        "stirred",
        "--brain-file",
        "compact",
    )
    log = field(*flags, "--steps", 32, "--probe-trace", stream, "--save", whole)
    with stream.open() as source:
        stamps = [int(q["tick"]) for q in csv.DictReader(source, delimiter="\t")]
        # The last 16 photographed frames are issued directly. Earlier frames run in the CUDA Graph.
        assert min(stamps) <= 4096 + 16 and max(stamps) > 4096 + 16
    local = audit.audit_grains(stream)
    q = rows(log, "field_soundings")[-1]
    assert (
        0
        < local["moved"]
        <= int(q["moved"])
        <= int(q["initiated"])
        <= int(q["offered"])
    )
    field(*flags, "--steps", 12, "--save", mid)
    call(
        "floodplain",
        "--load",
        mid,
        "--steps",
        20,
        "--brain-file",
        "compact",
        "--save",
        again,
    )
    assert whole.read_bytes() == again.read_bytes()
    survey = d / "shifting.tsv"
    repeated = d / "repeated"
    field(
        *flags,
        "--steps",
        32,
        "--probe-sampling",
        "events",
        "--probe-trace",
        survey,
        "--save",
        repeated,
    )
    assert whole.read_bytes() == repeated.read_bytes()
    assert audit.audit_grains(survey)["initiated"] > 0

    dark = field("--daylight", "off", "--steps", 2)
    assert not rows(dark, "field_soundings") and not rows(dark, "daylight")
    for file, expected, bought in (
        ("inquiry-conversation.txt", "east", 3),
        ("inquiry-reverse.txt", "west", 6),
    ):
        log = call(
            "talk",
            "--speaker",
            ex / "inquiry-speaker.mind",
            "--listener",
            ex / "parallax-witness.mind",
            stdin=(ex / file).read_text(),
        )
        assert ports(log) == [expected, expected, "north"]
        heard = next(v for v in rows(log, "cascade") if v["cascade"] == "heard")
        assert (
            int(heard["deposited"]) == bought
            and int(heard["rack_stock"]) + bought == 128
        )
        assert int(heard["query_paid"]) == bought and "delivered" not in heard
    memory = call(
        "talk",
        "--speaker",
        ex / "inquiry-witness.mind",
        "--listener",
        ex / "parallax-witness.mind",
        stdin=(ex / "inquiry-witness.txt").read_text(),
    )
    assert ports(memory) == ["east", "west"] and "speaker> plum" in memory


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(
        prefix="axion-inquiry-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as root:
        main(Path(root))
    print(
        "CUDA probe coupling, energy accounting, matched eyes, saved field and self-directed speech passed."
    )
