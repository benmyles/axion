#!/usr/bin/env python3
"""Verify what two separately embodied animals can hear, request, remember and report."""

import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from press import ROOT, call, rows
from worksites import checksum

sys.path.insert(0, str(ROOT / "tools"))
from spoken_audit import audit, read


def run():
    ex = ROOT / "examples"
    ordinary = ex / "compound-spoken-conversation.txt"
    repaired = ex / "compound-clarification.txt"
    writer = ex / "compound-spoken-writer.mind"
    reader = ex / "compound-spoken-reader.mind"
    foreign = ex / "compound-foreign-reader.mind"
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-audition-"
    ) as scratch:
        d = Path(scratch)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        home = d / "brook.flood"
        with home.open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ex / "capillary-nursery.flood.zst"],
                stdout=f,
                check=True,
            )

        def habitat(*args):
            return call("floodplain", "--load", home, "--steps", 0, *args)

        def assay(name, a, b, channel="clear", budget=0, replies="actual", rd=0):
            t = d / (name + ".tsv")
            s = habitat(
                "--catalytic-channels",
                2,
                "--catalytic-pair",
                a,
                b,
                "--catalytic-tests",
                64,
                "--catalytic-dialogues",
                64,
                "--class-seed",
                6309133,
                "--catalytic-history",
                817,
                "--catalytic-test-seed",
                7012361,
                "--catalytic-audio",
                channel,
                "--catalytic-questions",
                budget,
                "--catalytic-reply",
                replies,
                "--catalytic-reader-dialect",
                rd,
                "--catalytic-speech-trace",
                t,
            )
            result = audit(read(t), channel, budget, replies, 0, rd)
            assert all(
                int(rows(s, "auditory_feeders")[0][k]) == v for k, v in result.items()
            )
            return result

        clear = assay("clear", writer, reader)
        empty = assay(
            "empty",
            ex / "compound-unseen-writer.mind",
            ex / "compound-unseen-reader.mind",
            "partial",
            1,
        )
        assert empty["called"] == empty["found"] == 0
        actual = assay("partial", writer, reader, "partial", 1)
        missing = assay("withheld", writer, reader, "partial", 1, "withheld")
        swapped = assay("exchanged", writer, reader, "partial", 1, "exchanged")
        unasked = assay("unasked", writer, reader, "partial")
        crossed = assay("foreign", writer, foreign, rd=1)
        assert (
            actual["found"] == clear["found"] >= 224
            and actual["reported_choice"] == 256
        )
        assert (
            actual["requests"] == missing["requests"] == swapped["requests"]
            and actual["requests"] > 200
        )
        assert (
            unasked["found"] == missing["found"] == swapped["found"] < 30
            and crossed["found"] < 26
        )
        for mask in ("blind", "silent", "unknown", "reordered"):
            outcome = assay(mask, writer, reader, mask, 2)
            assert not outcome["requests"] and outcome["found"] == (
                clear["found"] if mask == "reordered" else 0
            )
        low = assay("one-question", writer, reader, "patchy", 1)
        high = assay("two-questions", writer, reader, "patchy", 2)
        assert low["found"] < high["found"] == clear["found"]

        def talk(speaker, listener, script, *args):
            return call(
                "talk",
                "--speaker",
                speaker,
                "--listener",
                listener,
                *args,
                stdin=script,
            )

        saved = [d / (who + ".mind") for who in ("speaker", "pupil")]
        s = talk(
            writer,
            reader,
            repaired.read_text(),
            "--save-speaker",
            saved[0],
            "--save-listener",
            saved[1],
        )
        said = [l for l in s.splitlines() if l.startswith(("speaker>", "pupil>"))]
        assert said[:4] == [
            "speaker> tap ring",
            "pupil> tick?",
            "speaker> ring",
            "pupil> south east",
        ]
        assert all(
            int(q["questions"]) == int(q["answers"]) == 1 and int(q["portions"]) == 2
            for q in rows(s, "voiced_encounter")
            if q["voiced_encounter"] == "2"
        )
        reload = talk(
            *saved,
            "/braid 62\n/braid-feed speaker 256\n/braid-feed partner 256\nWhat did you see?\n/cap-ask pupil Where is the food?\n/quit\n",
        )
        assert "speaker> tap tick" in reload and "pupil> south west" in reload
        closed = talk(
            writer,
            reader,
            ordinary.read_text().replace(
                "/cap-look pupil closed",
                "/cap-look pupil closed\n/cap-ear pupil closed",
            ),
        )
        assert "pupil> south" not in closed and "pupil> north" not in closed
        # An early SIG3 file had the same auditory links, without the seven-event trail.
        data = bytearray(writer.read_bytes())
        tag, extent = struct.unpack_from("<II", data, len(data) - 16)
        assert tag == 0x34474953
        at = len(data) - 8 - extent
        bad = bytearray(data)
        bad[at + 95] = 9
        bad[-8:] = checksum(bad[:-8])
        invalid = d / "damaged.mind"
        invalid.write_bytes(bad)
        assert "damaged auditory signal" in call(
            "talk",
            "--speaker",
            invalid,
            "--listener",
            reader,
            stdin="/quit\n",
            ok=False,
        )
        legacy = bytearray(data)
        del legacy[at + 88 : at + 96]
        struct.pack_into("<II", legacy, len(legacy) - 16, 0x33474953, extent - 8)
        struct.pack_into("<I", legacy, 12, len(legacy) - 24)
        legacy[-8:] = checksum(legacy[:-8])
        old = d / "legacy.mind"
        old.write_bytes(legacy)
        assert clear["found"] == assay("old", old, reader)["found"]
        baseline = d / "before.mind"
        resident = d / "home.mind"
        taught = [d / f"taught-{role}.mind" for role in ("writer", "reader")]
        watershed = d / "taught.flood"
        habitat("--slot", 7218, "--export-mind", baseline)
        bill = habitat(
            "--catalytic-channels",
            2,
            "--catalytic-pair",
            ex / "compound-joint-writer.mind",
            ex / "compound-joint-reader.mind",
            "--catalytic-voices",
            128,
            "--catalytic-export",
            *taught,
        )
        assert all(
            q["heard"] == "256"
            and q["combined_examples"] == "0"
            and q["pressure_strokes"] == "320"
            and q["voice_bytes"] == "96"
            for q in rows(bill, "signal_vocabulary")
        )
        habitat(
            "--capillary-field",
            "live",
            "--return-lesson",
            baseline,
            taught[0],
            "--save",
            watershed,
        )
        call(
            "floodplain",
            "--load",
            watershed,
            "--steps",
            0,
            "--slot",
            7218,
            "--export-mind",
            resident,
        )
        assert (
            assay("before-return", *taught)["found"]
            == assay("returned", resident, taught[1])["found"]
            >= 230
        )
    print(
        "Two-way sound, matched no-question controls, saved private memory, SIG3 compatibility and returned sparse voices passed."
    )


if __name__ == "__main__":
    run()
