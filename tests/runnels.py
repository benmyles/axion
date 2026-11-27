#!/usr/bin/env python3
"""Independently replay hydraulic work, material analogy and public testimony at their ordinary entry points."""

import gzip
import json
import os
import runpy
import struct
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT, checksum, snapshot

audit = runpy.run_path(str(ROOT / "tools/runnel-audit.py"))


def basin(*args, **opts):
    return call("floodplain", *args, **opts)


def main():
    with tempfile.TemporaryDirectory(
        prefix="axion-flume-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as temp:
        d = Path(temp)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        bank = ROOT / "examples/runnel-bank.flood"
        neutral = ROOT / "examples/gaugebank.flood"
        base = d / "base.flood"
        part = d / "part.flood"
        resumed = d / "split.flood"
        complete = d / "complete.flood"
        basin(
            "--load",
            neutral,
            "--reeds",
            "live",
            "--reed-seed",
            8111,
            "--reed-reason",
            "relational",
            "--reed-grain",
            "handed",
            "--steps",
            0,
            "--save",
            base,
        )
        basin("--load", base, "--steps", 71, "--save", part)
        basin("--load", part, "--steps", 150, "--save", resumed)
        result = basin(
            "--load",
            base,
            "--reed-rig",
            8,
            "--reed-bench",
            5713,
            "--reed-census",
            "--steps",
            221,
            "--save",
            complete,
        )
        assert resumed.read_bytes() == complete.read_bytes()
        keep = d / "rig.log"
        keep.write_text(result)
        assert audit["log"](keep)["controlled strips"] == 5
        state = snapshot(complete)
        n = state["side"] ** 2
        assert (
            len(state["fold_cells"]) == len(state["fold_map"]) == n
            and state["rush"][3] == 2
        )
        for r in rows(result, "reed_image"):
            assert (
                int(r["mismatches"]) == 0
                and int(r["bytes"]) == int(r["reed_image"]) * 12
            )
        flat = d / "flat.flood"
        basin("--load", complete, "--flatten-reeds", "--steps", 0, "--save", flat)
        blank = snapshot(flat)
        # A donor carries its material handedness, whereas epistemic tags belong to
        # the habitat that produced them. Explicit experimental geology takes priority.
        graft = d / "soil.flood"
        basin("--load", base, "--soil", complete, "--steps", 0, "--save", graft)
        transplanted = snapshot(graft)
        assert transplanted["fold_map"] == [v & 0x77 for v in state["fold_map"]]
        assert transplanted["fold_cells"] == snapshot(base)["fold_cells"]
        basin(
            "--load",
            base,
            "--soil",
            complete,
            "--reed-grain",
            "straight",
            "--reed-strata",
            71,
            "--steps",
            0,
            "--save",
            graft,
        )
        chosen = snapshot(graft)
        assert chosen["rush"][1] == 71 and not (chosen["rush"][0] & 8192)

        assert (
            not any(blank["fold_map"])
            and blank["fold_cells"] == state["fold_cells"]
            and blank["soil"] == state["soil"]
        )
        bigfile = d / "wide.flood"
        basin("--load", complete, "--extend", 128, "--steps", 0, "--save", bigfile)
        big = snapshot(bigfile)
        assert big["fold_cells"][:n] == state["fold_cells"]
        for i, v in enumerate(state["fold_map"]):
            assert big["fold_map"][i // 64 * 128 + i % 64] == v
        report = []
        for cut in ("observed", "matched", "sham"):
            file = d / (cut + ".flood")
            log = basin(
                "--load", complete, "--reed-cut", cut, "--steps", 0, "--save", file
            )
            after = snapshot(file)
            record = rows(log, "reed_cut")[0]
            report.append(int(record["pairs"]))
            assert (
                after["soil"] == state["soil"]
                and after["fold_cells"] == state["fold_cells"]
            )
            changed = [
                (a >> (4 * axis)) & 15
                for a, b in zip(state["fold_map"], after["fold_map"])
                for axis in range(2)
                if ((a ^ b) >> (4 * axis)) & 15
            ]
            assert len(changed) == int(record["removed"]) and all(
                bool(z & 8) == (cut == "observed") for z in changed
            )
        assert len(set(report)) == 1
        trace = d / "physical.tsv"
        test = basin(
            "--load",
            bank,
            "--reed-analogy",
            43,
            "--reed-trace",
            trace,
            "--reed-seed",
            51431,
            "--steps",
            0,
        )
        totals = audit["traces"](trace)
        for record in rows(test, "reed_analogy"):
            a = record["reed_analogy"]
            q = totals[a]
            assert (
                int(record["both"]) == q["both"]
                and int(record["verified_corners"]) == q["verified_paired_relations"]
            )
        original = trace.read_bytes()
        basin(
            "--load",
            bank,
            "--reed-analogy",
            1,
            "--reed-trace",
            trace,
            "--steps",
            0,
            ok=False,
        )
        assert trace.read_bytes() == original and not list(d.glob("*.part-*"))
        duet = d / "duet.tsv"
        m = ROOT / "examples/runnel-apprentice.mind"
        peer = ROOT / "examples/runnel-consultant.mind"
        speech = call(
            "talk",
            "--speaker",
            m,
            "--listener",
            peer,
            stdin=f"/flume 931 8\n/flume-trials 19 23761 {duet}\n/quit\n",
        )
        rates = audit["speech_audit"](duet)
        for row in rows(speech, "flume_dialogue"):
            v = rates[row["flume_dialogue"]]
            assert (
                int(row["both"]) == v["both"]
                and int(row["later_exact"]) == v["later_correct"]
            )
        assert (
            rates["heard"]["both"] > 10
            and rates["heard"]["about_another_joint"] > 16
            and rates["muted"]["both"] == 0
        )
        update = d / "experienced.mind"
        transcript = call(
            "talk",
            "--speaker",
            m,
            "--listener",
            peer,
            "--save-speaker",
            update,
            stdin=(ROOT / "examples/runnel-conversation.txt").read_text(),
        )
        water = rows(transcript, "flume_contact")
        assert any(int(v["delivered"]) > 0 for v in water)
        assert any(v["related_pair"] != "16" and v["understood"] == "1" for v in water)
        call(
            "talk",
            "--speaker",
            update,
            "--listener",
            peer,
            stdin="/flume 931 4\n/flume-view\n/quit\n",
        )
        # A watershed body predating spatial memory can also carry the independent reed suffix.
        old_carrier = d / "old-carrier.mind"
        basin(
            "--load",
            bank,
            "--reed-carrier",
            ROOT / "examples/shore-reader.mind",
            "--reed-scholar",
            old_carrier,
            "--steps",
            0,
        )
        call("agent", "--load", old_carrier, "--inspect")
        housed = basin(
            "--resident", old_carrier, "--side", 16, "--density", 0, "--steps", 0
        )
        assert int(rows(housed, "reed_field")[-1]["living"]) == 1
        bad = d / "bad.mind"
        data = bytearray(update.read_bytes())
        p = len(data) - 64
        struct.pack_into("<II", data, p + 12, 0, 0)
        struct.pack_into("<I", data, p + 20, 1)
        data[-8:] = checksum(data[:-8])
        bad.write_bytes(data)
        call("talk", "--speaker", bad, "--listener", peer, stdin="/quit\n", ok=False)
        ends = sorted((d / "journal/runs").glob("*-*/observations.json.gz"))
        witnessed = False
        for entry in ends:
            with gzip.open(entry) as f:
                witnessed |= any(
                    line.startswith("reed_analogy=") for line in json.load(f)["lines"]
                )
        assert witnessed
        print(
            "PASS material pumps, exact native restart, matching, physical proofs, two autonomous public speakers, retained testimony and damaged tissue"
        )


if __name__ == "__main__":
    main()
