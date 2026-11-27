"""Exercise acquisition, motor transfer, physical accounting and checkpoint persistence."""

import csv
import importlib.util
import os
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT

spec = importlib.util.spec_from_file_location(
    "apprentice_audit", ROOT / "tools/apprentice-audit.py"
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
donor = ROOT / "examples/estuary-regulator.txt"
speaker = ROOT / "examples/estuary-tide-student.mind"
peer = ROOT / "examples/parallax-witness.mind"


def main():
    with tempfile.TemporaryDirectory(
        prefix="axion-apprentice-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as tmp:
        d = Path(tmp)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")

        def field(*args, **kw):
            return call("floodplain", *args, **kw)

        base = d / "bay"
        resident = d / "untaught.mind"
        log = field(
            "--resident",
            speaker,
            "--side",
            16,
            "--density",
            0,
            "--steps",
            0,
            "--save",
            base,
            "--export-mind",
            resident,
        )
        slot = int(rows(log, "rehoused_resident")[0]["rehoused_resident"])
        trace = d / "lessons.tsv"
        log = field(
            "--load",
            base,
            "--slot",
            slot,
            "--steps",
            0,
            "--feedback-class",
            43,
            "--class-practice",
            12,
            "--class-seed",
            149993,
            "--feedback-frames",
            4,
            "--feedback-rig",
            "varied",
            "--feedback-motor",
            "cortical",
            "--feedback-convention",
            "reversed",
            "--feedback-trace",
            trace,
        )
        measured = audit.classroom(trace)
        for r in rows(log, "apprentice_language"):
            data = measured[r["apprentice_language"]]
            for key in ("early", "twice", "offered", "correct", "both"):
                assert data[key] == int(r[key]), (key, data, r)
        for r in rows(log, "apprentice_scene"):
            f = r["frame"]
            data = measured[r["apprentice_scene"]]
            assert (
                data["frame" + f],
                data["different" + f],
                data["different_right" + f],
            ) == tuple(int(r[k]) for k in ("correct", "different", "different_correct"))
        assert measured["inherited"]["twice"] > 38
        assert measured["quiet"]["twice"] == 0
        assert measured["inherited"]["released"] == measured["yoked"]["released"]
        assert measured["inherited"]["different_right2"] > 20
        woods = audit.donor_words(donor)
        assert audit.food_return(woods, 91513)["swallowed"] == 2361

        funds = field(
            "--load",
            base,
            "--slot",
            slot,
            "--steps",
            0,
            "--feedback-generations",
            5,
            "--feedback-cohort",
            32,
            "--feedback-donor",
            d / "donor",
        )
        for r in rows(funds, "brood_budget"):
            assert int(r["delivered"]) == int(r["swallowed"]) + int(r["washed"])
            assert int(r["swallowed"]) == 8 * int(r["births"]) + int(r["unused"])
        assert (d / "donor").is_file()

        original = d / "nature"
        field(
            "--side",
            32,
            "--density",
            60,
            "--neurogenesis",
            "inherited",
            "--braids",
            "live",
            "--braid-episodes",
            "--braid-mesh",
            "continuous",
            "--steps",
            16,
            "--save",
            original,
        )
        persistent = ("--feedback-ports", "open", "--brain-file", "compact")
        dense = ("--brain-dispatch", "fixed", "--brain-memory", "dense")
        paged = ("--brain-dispatch", "packed", "--brain-memory", "paged")
        planted = d / "introduced"
        release = field(
            "--load",
            original,
            "--receive-donor",
            donor,
            "--receive-ports",
            "both",
            "--receive-count",
            23,
            "--trace-donor",
            donor,
            "--steps",
            0,
            "--save",
            planted,
            *persistent,
        )
        assert int(rows(release, "regulatory_release")[0]["placed"]) == 23
        assert int(rows(release, "regulatory_carriers")[-1]["both_ports"]) >= 23
        for name, args in (("dense", dense), ("paged", paged)):
            tracked = field("--load", planted, "--steps", 48, "--save", d / name, *args)
            r = rows(tracked, "regulatory_field")[-1]
            assert 0 < int(r["admitted"]) < int(r["proposed"])
            assert 0 < int(r["soma_changes"]) <= 32 * int(r["contacts"])
        assert (d / "dense").read_bytes() == (d / "paged").read_bytes()
        midway = d / "midway"
        field("--load", planted, "--steps", 15, "--save", midway, *dense)
        field("--load", midway, "--steps", 33, "--save", d / "split", *paged)
        assert (d / "split").read_bytes() == (d / "dense").read_bytes()
        exam = field(
            "--load",
            planted,
            "--steps",
            48,
            "--shore-school",
            ROOT / "examples/shore-school.txt",
            "--shore-pupils",
            19,
            "--shore-channel",
            "exam",
            "--save",
            d / "observed",
            *paged,
        )
        assert (d / "observed").read_bytes() == (d / "dense").read_bytes()
        result = rows(exam, "shore_school")[0]
        assert int(result["scenes"]) > 0 and int(result["examples"]) == 0
        changed = d / "edited"
        field(
            "--load",
            planted,
            "--steps",
            48,
            "--shore-school",
            ROOT / "examples/shore-school.txt",
            "--shore-pupils",
            19,
            "--save",
            changed,
            *paged,
        )
        assert changed.read_bytes() != (d / "dense").read_bytes()

        taught = d / "speaker.mind"
        transcript = call(
            "talk",
            "--speaker",
            resident,
            "--listener",
            peer,
            "--save-speaker",
            taught,
            stdin=(ROOT / "examples/estuary-tide-lessons.txt").read_text(),
        )
        event = rows(transcript, "braid_hand_event")[0]
        assert int(event["actual"]) != int(event["unobstructed"])
        assert int(event["strokes"]) == 3
        verified = call(
            "talk",
            "--speaker",
            taught,
            "--listener",
            peer,
            stdin=(ROOT / "examples/estuary-tide-conversation.txt").read_text(),
        )
        replies = [
            line.split("> ")[1]
            for line in verified.splitlines()
            if line.startswith("speaker> ")
        ]
        hues = ("amber", "jade", "plum", "teal")
        assert replies[:8] == [
            hues[(int(event[f]) >> (2 * j)) & 3]
            for j in (1, 3)
            for f in ("before", "actual", "unobstructed", "shortened")
        ]
        assert len(rows(verified, "braid_recall")) == 8
        practiced = d / "practiced"
        first = field(
            "--load",
            base,
            "--return-practice",
            resident,
            taught,
            "--steps",
            0,
            "--save",
            practiced,
        )
        assert int(rows(first, "field_motor_lesson")[0]["changed_bits"]) > 0
        replay = field(
            "--load",
            practiced,
            "--return-practice",
            resident,
            taught,
            "--steps",
            0,
            "--save",
            d / "same-lesson",
        )
        assert int(rows(replay, "field_motor_lesson")[0]["changed_bits"]) == 0
        assert practiced.read_bytes() == (d / "same-lesson").read_bytes()
        survey = d / "pupils.tsv"
        sampling = field(
            "--load",
            base,
            "--steps",
            0,
            "--trace-donor",
            donor,
            "--feedback-survey",
            32,
            "--feedback-enrollment",
            "carriers",
            "--feedback-motor",
            "cortical",
            "--feedback-dialects",
            3,
            "--class-practice",
            12,
            "--feedback-survey-trace",
            survey,
        )
        result = rows(sampling, "regulatory_survey")[0]
        with survey.open() as file:
            roll = list(csv.DictReader(file, delimiter="\t"))
        assert len(roll) == int(result["enrolled"]) == 1
        assert int(roll[0]["slot"]) == slot
        assert int(result["paired_passes"]) == int(roll[0]["twice"])
        assert int(result["every_course"]) == int(roll[0]["twice"] == "3")
        assert "distinct output" in field(
            "--load",
            base,
            "--steps",
            0,
            "--slot",
            slot,
            "--feedback-generations",
            2,
            "--feedback-donor",
            d / "collision",
            "--feeding-trace",
            d / "collision",
            ok=False,
        )
        invalid = d / "malformed"
        invalid.write_text("AXION-REGULATOR-1 0 0 16 0 1 0 0 1\n")
        assert "invalid reared" in field(
            "--load",
            base,
            "--slot",
            slot,
            "--receive-donor",
            invalid,
            "--steps",
            0,
            ok=False,
        )
    print(
        "Feedback, four-frame mechanics, carbon accounts, wild inheritance and exact storage replay passed."
    )


if __name__ == "__main__":
    main()
