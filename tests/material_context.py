#!/usr/bin/env python3
"""Exercise continuous causal learning, sensory controls and durable field ownership on the CUDA device."""

import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path
from press import ROOT, call, rows
from worksites import checksum

sys.path.insert(0, str(ROOT / "tools"))
from capillary_audit import audit_repairs, read, local
from context_audit import choices, field, stream


def run():
    ex = ROOT / "examples"
    speaker = ex / "detritus-guide.mind"
    listener = ex / "capillary-listener.mind"
    with tempfile.TemporaryDirectory(
        dir=os.getenv("TMPDIR", "/var/tmp"), prefix="axion-context-test-"
    ) as tmp:
        d = Path(tmp)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        home = d / "brook.flood"
        with home.open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ex / "capillary-nursery.flood.zst"],
                stdout=f,
                check=True,
            )

        def basin(*args, ok=True):
            return call("floodplain", *args, ok=ok)

        def talk(command, mind=speaker, other=listener, *extra, ok=True):
            return call(
                "talk",
                "--speaker",
                mind,
                "--listener",
                other,
                *extra,
                stdin=command,
                ok=ok,
            )

        school = [
            "--load",
            home,
            "--steps",
            0,
            "--capillary-pupil",
            speaker,
            "--context-streams",
            12,
            "--context-episodes",
            9,
            "--context-change",
            43,
            "--class-seed",
            4513049,
            "--capillary-depth",
            9,
            "--capillary-moves",
            5,
            "--capillary-supply",
            "scarce",
        ]
        commands = {}
        for arm in (
            "adaptive",
            "single",
            "clamped",
            "exchanged",
            "cleared",
            "unchecked",
        ):
            p = d / (arm + ".tsv")
            result = basin(*school, "--context-arm", arm, "--context-trace", p)
            records = read(p)
            counts = stream(records, None, None)
            row = rows(result, "context_series")[0]
            assert counts["changed_after"] == int(row["resolved_switch"]) and counts[
                "harvest"
            ] == int(row["repaired"])
            commands[arm] = [
                (r["fingerprint"], r["probes"], r["heat"]) for r in records
            ]
            if arm == "adaptive":
                assert counts["changed_after"] == counts["switches"] > 0
            if arm in ("clamped", "exchanged"):
                assert (
                    commands[arm] == commands["adaptive"]
                    and counts["changed_after"] == 0
                )
        no = basin(*school, "--capillary-probes", 0)
        assert rows(no, "context_series")[0]["actions"] == "0"
        p = d / "ordinary.tsv"
        basin(
            "--load",
            home,
            "--steps",
            0,
            "--capillary-pupil",
            ex / "context-coastal.mind",
            "--capillary-trials",
            8,
            "--capillary-supply",
            "scarce",
            "--capillary-depth",
            8,
            "--capillary-trace",
            p,
        )
        ordinary = read(p)
        assert all(r["context_mode"] for r in ordinary)
        audit_repairs(ordinary)
        saved = d / "saved.mind"
        spoken = talk(
            (ex / "context-conversation.txt").read_text(),
            speaker,
            listener,
            "--save-speaker",
            saved,
        )
        answers = [s[9:] for s in spoken.splitlines() if s.startswith("speaker> ")]
        assert answers[::2] == [
            "the " + c + " let teal through"
            for c in ("wick", "film", "wick", "film", "film", "wick")
        ]
        assert answers[1::2] == ["yes", "no", "yes", "no", "no", "yes"]
        corrupt = bytearray(saved.read_bytes())
        extent = struct.unpack_from("<I", corrupt, len(corrupt) - 12)[0]
        corrupt[len(corrupt) - 8 - extent + 8] = 1
        corrupt[-8:] = checksum(corrupt[:-8])
        damaged = d / "invalid.mind"
        damaged.write_bytes(corrupt)
        assert "damaged material context" in talk("/quit\n", damaged, ok=False)
        table = talk((ex / "context-blind-probes.txt").read_text(), saved)
        actions = rows(table, "capillary_turn")
        captures = [q for q in actions if q["capillary_turn"] == "19"]
        assert len(captures) == 32 and [int(q["drops"]) for q in captures] == list(
            range(31, -1, -1)
        )
        previous = 0
        for i, q in enumerate(captures):
            mask = int(q["handles"])
            physical = 25 ^ ((mask & 15) << 2) ^ ((mask >> 4) & 3)
            rock = (0, 16, 0, 16)[i // 8]
            assert int(q["target"]) == local(physical, rock)
            assert int(q["heat"]) == (mask ^ previous).bit_count() + 2
            previous = mask
        check = d / "trained.flood"
        fresh = ex / "capillary-young.mind"
        known = ex / "context-brook.mind"
        basin(
            "--load",
            home,
            "--steps",
            0,
            "--capillary-memory",
            "adaptive",
            "--return-lesson",
            fresh,
            known,
            "--save",
            check,
        )
        recovered = d / "recovered.mind"
        basin(
            "--load",
            check,
            "--steps",
            0,
            "--follow-witness",
            fresh,
            "--export-mind",
            recovered,
        )
        resumed = talk("/cap-context speaker adaptive\n/quit\n", recovered)
        assert int(rows(resumed, "material_memory")[0]["receipts"]) & 128
        broad = d / "broad.flood"
        live = d / "live.flood"
        snap = d / "partial.flood"
        end = d / "continued.flood"
        signals = d / "senses.tsv"
        decisions = d / "choices.tsv"
        basin(
            "--load",
            check,
            "--steps",
            257,
            "--capillary-memory",
            "adaptive",
            "--capillary-events",
            signals,
            "--context-events",
            decisions,
            "--save",
            broad,
        )
        field(read(signals))
        choices(read(decisions))
        basin("--load", check, "--steps", 257, "--save", live)
        basin("--load", check, "--steps", 39, "--save", snap)
        basin("--load", snap, "--steps", 218, "--report-every", 37, "--save", end)
        assert broad.read_bytes() == live.read_bytes() == end.read_bytes()
        young = d / "tiny.flood"
        frozen = d / "inactive.flood"
        larger = d / "expanded.flood"
        basin(
            "--resident",
            known,
            "--side",
            4,
            "--density",
            0,
            "--capillary-field",
            "live",
            "--capillary-memory",
            "adaptive",
            "--steps",
            9,
            "--save",
            young,
        )
        basin(
            "--load", young, "--capillary-field", "off", "--steps", 3, "--save", frozen
        )
        basin(
            "--load",
            frozen,
            "--extend",
            16,
            "--capillary-field",
            "live",
            "--capillary-memory",
            "adaptive",
            "--steps",
            19,
            "--save",
            larger,
        )
        basin("--load", larger, "--steps", 0)
    print(
        "material context passed: unknown switches, matched interventions, paid physical probes, exact replay and inherited ownership"
    )


if __name__ == "__main__":
    run()
