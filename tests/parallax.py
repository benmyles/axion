#!/usr/bin/env python3
"""Verify a situated conversation, compressed partner memory and a one-question decision."""

import os
from pathlib import Path
import runpy
import tempfile
from press import call, rows
from worksites import ROOT, snapshot, checksum

examples = ROOT / "examples"
audit = runpy.run_path(str(ROOT / "tools/parallax-audit.py"))["audit"]


def main():
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="parallax-"
    ) as dirname:
        d = Path(dirname)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")

        def basin(*args):
            return call("floodplain", *args)

        def talk(
            lines,
            *options,
            speaker=examples / "parallax-reader.mind",
            witness=examples / "parallax-witness.mind",
        ):
            return call(
                "talk",
                "--speaker",
                speaker,
                "--listener",
                witness,
                *options,
                stdin=lines + "\n/quit\n",
            )

        old = examples / "braid-reader.mind"
        speaker = examples / "parallax-reader.mind"
        assert old.stat().st_size == speaker.stat().st_size
        exchange = talk((examples / "parallax-conversation.txt").read_text())
        voices = [
            s
            for s in exchange.splitlines()
            if s.startswith(("speaker asks>", "witness>", "speaker>"))
        ]
        assert voices[:5] == [
            "speaker> north",
            "speaker asks> what is in east?",
            "witness> jade",
            "speaker> teal",
            "speaker> north",
        ], voices
        assert voices[5:10] == [
            "speaker> north",
            "speaker asks> what is in south?",
            "witness> amber",
            "speaker> teal",
            "speaker> north",
        ]
        assert len(rows(exchange, "braid_audience")) == 1 and all(
            v["candidates"] == "1" for v in rows(exchange, "braid_view")
        )
        # One complete common scene permits all 24 socket maps. Repeating the
        # very same observation cannot install either a rotation or a pigment name.
        scene = "/braid 137\n/braid-glass partner 1032 2031\n/braid-station speaker 0 0123 15\nreed spin tuck\n"
        r = talk(
            scene + "/braid-align speaker partner 4\n/braid-align speaker partner 4",
            speaker=old,
        )
        assert [x["candidates"] for x in rows(r, "braid_view")] == ["24", "24"]
        # A shutter removes the actual correlated signal from one learner.
        blinded = talk(
            "/braid 137\n/braid-glass partner 1032 2031\n/braid-shutters partner 0\n/braid-compare speaker partner 4 8 1907",
            speaker=old,
        )
        assert rows(blinded, "braid_view")[-1]["replies"] == "0"
        # From a saved shared memory, return to a new physical world, wait
        # for the spoken consultation and answer through the same named lens.
        habitat = d / "home.flood"
        loc = basin(
            "--resident",
            speaker,
            "--side",
            16,
            "--density",
            0,
            "--braid-eyes",
            "local",
            "--steps",
            0,
            "--save",
            habitat,
        )
        slot = int(rows(loc, "rehoused_resident")[0]["rehoused_resident"])
        h = snapshot(habitat)
        assert h["loom"][0] == 5 and slot in h["narrators"]
        resumed = d / "returned.mind"
        basin(
            "--load", habitat, "--slot", slot, "--braid-speaker", resumed, "--steps", 0
        )
        assert resumed.read_bytes()[-584:-8] == speaker.read_bytes()[-584:-8]
        assert (
            rows(
                talk("/braid-perspective speaker partner", speaker=resumed),
                "braid_view",
            )[0]["candidates"]
            == "1"
        )
        # Reject malformed high hypothesis bits even when the outer checksum is valid.
        raw = bytearray(speaker.read_bytes())
        offset = len(raw) - 584 + 256
        for group in range(4):
            start = offset + group * 64 + 12
            if raw[start : start + 4] != bytes(4):
                raw[start + 32 + 3] |= 128
                break
        else:
            raise AssertionError("missing named acquaintance")
        raw[-8:] = checksum(raw[:-8])
        bad = d / "bad.mind"
        bad.write_bytes(raw)
        assert "event memory" in call(
            "talk",
            "--speaker",
            bad,
            "--listener",
            examples / "parallax-witness.mind",
            stdin="/quit\n",
            ok=False,
        )
        full = d / "full.flood"
        part = d / "part.flood"
        split = d / "split.flood"
        base = examples / "voicebank.flood"
        basin(
            "--load",
            base,
            "--braid-eyes",
            "local",
            "--braid-mesh",
            "continuous",
            "--steps",
            79,
            "--save",
            full,
        )
        basin(
            "--load",
            base,
            "--braid-eyes",
            "local",
            "--braid-mesh",
            "continuous",
            "--steps",
            29,
            "--save",
            part,
        )
        basin("--load", part, "--steps", 50, "--save", split)
        assert full.read_bytes() == split.read_bytes()
        world = snapshot(full)
        assert world["loom"][0] == 5 and any(v[2] for v in world["float_events"])
        evidence = d / "views.tsv"
        report = basin(
            "--load",
            examples / "floatbank.flood",
            "--steps",
            0,
            "--braid-perspectives",
            32,
            "--braid-seed",
            72497,
            "--braid-examples",
            48,
            "--braid-views",
            3,
            "--braid-view-budget",
            1,
            "--braid-view-trace",
            evidence,
        )
        actual = audit(evidence)
        grading = {q["braid_parallax"]: q for q in rows(report, "braid_parallax")}
        audience = {q["braid_audiences"]: q for q in rows(report, "braid_audiences")}
        for name, metrics in actual.items():
            m = grading[name]
            a = audience[name]
            for check, key in (
                ("faithful", "grounded"),
                ("probes", "offered"),
                ("both", "both"),
                ("unique_true", "true_frames"),
                ("comparisons_heard", "heard"),
            ):
                assert metrics[check] == int(m[key]), (name, check)
            for check, key in (
                ("one_call", "directed"),
                ("found_sole", "found"),
                ("equal_capacity", "equal"),
                ("fixed0", "fixed0"),
            ):
                assert metrics[check] == int(a[key]), (name, check)
        assert (
            actual["calibrated"]["faithful"] > actual["literal"]["faithful"]
            and actual["same_panel"]["unique"] == 0
            and not actual["muted"]["correct"]
        )
        print(
            "Verified all 576 optical candidates, both voice directions, independent one-call decisions, durable memory and private retinal physics."
        )


if __name__ == "__main__":
    main()
