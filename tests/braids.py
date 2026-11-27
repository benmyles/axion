#!/usr/bin/env python3
"""Check conserved matter, independent speakers, checkpoint persistence and event counterfactuals."""

import os
from pathlib import Path
import runpy
import tempfile

from press import call, rows
from worksites import ROOT, snapshot, checksum

EXAMPLES = ROOT / "examples"
AUDIT = runpy.run_path(str(ROOT / "tools/braid-audit.py"))["audit"]


def basin(*args, **options):
    return call("floodplain", *args, **options)


def main():
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="braid-test-"
    ) as directory:
        d = Path(directory)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        stock = EXAMPLES / "floatbank.flood"
        speaker = EXAMPLES / "braid-reader.mind"
        peer = EXAMPLES / "braid-witness.mind"

        def talk(words, agent=speaker, listener=peer, *opts):
            return call(
                "talk",
                "--speaker",
                agent,
                "--listener",
                listener,
                *opts,
                stdin=words + "\n/quit\n",
            )

        # Merely returning to the interview must preserve ordinary multi-turn speech.
        transcript = talk((EXAMPLES / "braid-conversation.txt").read_text())
        speech = [
            line
            for line in transcript.splitlines()
            if line.startswith(("speaker>", "witness>", "speaker asks>"))
        ]
        assert speech == [
            "speaker> north",
            "speaker> amber",
            "speaker> south",
            "speaker> west",
            "speaker> north",
            "speaker asks> what is in north?",
            "witness> teal",
            "speaker> teal",
            "speaker> north",
            "speaker asks> where is amber?",
            "speaker> south",
            "speaker> west",
        ], speech
        apparatus = talk((EXAMPLES / "braid-translation.txt").read_text())
        m = next(r for r in rows(apparatus, "braid_talk") if r["braid_talk"] == "7")
        assert (
            m["delivered"] == "1"
            and m["start"] != m["end"] == m["expected"]
            and int(m["consultations"]) <= 4
        )
        long = talk(
            "/braid 137\n/braid-station speaker 0 0123\n"
            + "\n".join(" ".join(["reed"] * n) for n in (8, 9, 31, 32, 33, 65))
            + "\n"
            + ("reed " * 32)
            + "unknown"
        )
        output = rows(long, "braid_talk")[-7:]
        assert [int(r["heard_words"]) for r in output] == [8, 9, 31, 32, 33, 65, 33]
        assert (
            all(r["ready"] == "1" for r in output[:-1]) and output[-1]["ready"] == "0"
        )
        # Expose and withhold exactly the same episode, including an open consultation
        # split across a portable checkpoint and a subsequent habitat save.
        pending = d / "pending.mind"
        watcher = d / "pending-witness.mind"
        log = talk(
            "/braid 137\n/braid-shutters speaker 13\n/braid-station speaker 0 0123 15\nreed spin tuck\nwhere did amber start?\n/braid-replies manual\nand there now?",
            speaker,
            peer,
            "--save-speaker",
            pending,
            "--save-listener",
            watcher,
        )
        assert "speaker asks> what is in north?" in log and "witness>" not in log
        settled = d / "resident.flood"
        housed = basin(
            "--resident",
            pending,
            "--side",
            16,
            "--density",
            0,
            "--steps",
            0,
            "--save",
            settled,
        )
        slot = int(rows(housed, "rehoused_resident")[0]["rehoused_resident"])
        world = snapshot(settled)
        assert list(world["narrators"]) == [slot] and world["float_events"][slot][2]
        restored = d / "returned.mind"
        basin(
            "--load", settled, "--slot", slot, "--braid-speaker", restored, "--steps", 0
        )
        # The report trailer includes the pending interlocutor state and unobserved colors.
        assert restored.read_bytes()[-584:-8] == pending.read_bytes()[-584:-8]
        heard = talk("/braid-consult\nwhere is teal?", restored, watcher)
        assert "witness> teal" in heard and "speaker> north" in heard
        damaged = bytearray(pending.read_bytes())
        damaged[-8 - 8 - 8] = 255  # invalid previous topic, before the report mask
        damaged[-8:] = checksum(damaged[:-8])
        bad = d / "bad.mind"
        bad.write_bytes(damaged)
        assert "event memory" in call(
            "talk", "--speaker", bad, "--listener", peer, stdin="/quit\n", ok=False
        )
        # Physics and hydrology have exactly the same state after a stop in the middle.
        start = d / "initial.flood"
        part = d / "part.flood"
        full = d / "full.flood"
        split = d / "split.flood"
        basin(
            "--load",
            EXAMPLES / "resonator-bank.flood",
            "--braids",
            "live",
            "--braid-mesh",
            "continuous",
            "--braid-sediment",
            "mobile",
            "--braid-episodes",
            "--steps",
            0,
            "--save",
            start,
        )
        basin("--load", start, "--steps", 63, "--report-every", 11, "--save", full)
        basin("--load", start, "--steps", 25, "--report-every", 7, "--save", part)
        basin("--load", part, "--steps", 38, "--report-every", 13, "--save", split)
        assert split.read_bytes() == full.read_bytes()
        initial, final = snapshot(start), snapshot(full)
        assert (
            final["relief"][0]
            and final["relief"][6] + final["relief"][7]
            and any(v[2] for v in final["float_events"])
        )
        assert final["loom"][0] == 4 and len(final["riggers"]) == len(final["energy"])
        held = d / "held.flood"
        basin(
            "--load", start, "--braid-sediment", "pinned", "--steps", 63, "--save", held
        )
        assert snapshot(held)["relief"][6:] == (0, 0, 0)
        larger = d / "larger.flood"
        basin("--load", full, "--extend", 128, "--steps", 0, "--save", larger)
        broad = snapshot(larger)
        before = len(final["energy"])
        assert broad["float_events"][:before] == final["float_events"] and not any(
            q[2] for q in broad["float_events"][before:]
        )
        graft = d / "graft.flood"
        basin("--load", full, "--soil", start, "--steps", 0, "--save", graft)
        assert snapshot(graft)["floaters"] == initial["floaters"]
        traces = [
            d / (name + ".tsv")
            for name in ("transfer", "dialogue", "culture", "events")
        ]
        report = basin(
            "--load",
            stock,
            "--braid-cases",
            32,
            "--braid-words",
            6,
            "--braid-lessons",
            6,
            "--braid-convention",
            "varied",
            "--braid-trace",
            traces[0],
            "--braid-dialogues",
            16,
            "--braid-transcript",
            traces[1],
            "--braid-villages",
            8,
            "--braid-generations",
            3,
            "--braid-songbook",
            traces[2],
            "--braid-inquiries",
            64,
            "--braid-examples",
            48,
            "--braid-holdout",
            2,
            "--braid-report-trace",
            traces[3],
            "--braid-seed",
            130243,
            "--braid-bench",
            65536,
            "--steps",
            0,
        )
        checked = [AUDIT(trace) for trace in traces]
        scores = checked[-1]
        assert (
            checked[0]["grounded"]["correct"] > checked[0]["single_examples"]["correct"]
        )
        assert (
            checked[1]["translated"]["faithful_translation"]
            and checked[2]["shared"]["rows"] == 384
        )
        assert (
            scores["witness"]["both_opposed"] > 20
            and not scores["echoed"]["correct"]
            and not scores["untaught"]["correct"]
        )
        assert (
            scores["witness"]["reciprocal_after_departure"] > 80
            and not scores["no_retention"]["reciprocal_after_departure"]
        )
        assert (
            scores["witness"]["both_topics"] > 100
            and scores["witness"]["distinguished_intention"] > 50
        )
        same_call = talk(
            (EXAMPLES / "braid-calls.txt").read_text(),
            EXAMPLES / "braid-call-0.mind",
            EXAMPLES / "braid-call-1.mind",
        )
        assert [
            int(r["end"])
            for r in rows(same_call, "braid_talk")
            if r["braid_talk"] == "6"
        ] == [180, 225]
        assert all(r["mismatches"] == "0" for r in rows(report, "braid_image"))
        print(
            "Verified native movements, sediment, learned acoustic reports, three-body translation, heredity and checkpoint continuation."
        )


if __name__ == "__main__":
    main()
