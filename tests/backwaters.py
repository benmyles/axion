#!/usr/bin/env python3
"""Replay bank encounters, transfers, inquiry controls and public spoken explanations."""

import os
import runpy
import struct
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT, checksum, snapshot

audit = runpy.run_path(str(ROOT / "tools/bank-audit.py"))["audit"]


def basin(*args, **kw):
    return call("floodplain", *args, **kw)


def main():
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="backwater-"
    ) as dirname:
        work = Path(dirname)
        os.environ["AXION_JOURNAL_DIR"] = str(work / "journal")
        orig = ROOT / "examples/runnel-bank.flood"
        base, part, full, split = (
            work / (a + ".flood") for a in ("base", "part", "full", "split")
        )
        basin(
            "--load",
            orig,
            "--rattles",
            "live",
            "--rattle-seed",
            51031,
            "--steps",
            0,
            "--save",
            base,
        )
        before = snapshot(base)
        assert before["bank"][:4] == (1, 15, 51031, 0) and all(
            not any(m[-2:]) for m in before["listeners"]
        )
        args = [
            "--rattle-listen",
            "one",
            "--rattle-binding",
            "crossed",
            "--rattle-phase",
            1,
        ]
        basin("--load", base, *args, "--steps", 29, "--save", part)
        basin("--load", part, "--steps", 92, "--save", split)
        tsv = work / "probes.tsv"
        phone = work / "speaker.mind"
        log = basin(
            "--load",
            base,
            *args,
            "--steps",
            121,
            "--save",
            full,
            "--rattle-trials",
            32,
            "--rattle-exposure",
            12,
            "--rattle-trace",
            tsv,
            "--rattle-bench",
            54833,
            "--rattle-speaker",
            phone,
            "--rattle-carrier",
            ROOT / "examples/runnel-apprentice.mind",
        )
        assert full.read_bytes() == split.read_bytes()
        info = snapshot(full)
        assert info["bank"][:4] == (2, 111, 51031, 1) and info["bank_query"][0] > 0
        field = rows(log, "bank_field")[-1]
        changes = rows(log, "bank_choice")[-1]
        assert (
            int(field["work"]) == int(field["pulls"]) + int(field["calls"])
            and int(field["digested"]) > 0
        )
        assert int(changes["learned_changes"]) <= int(field["heard"])
        assert rows(log, "bank_image")[0]["mismatches"] == "0"
        counts, truth = audit(tsv)
        assert counts["physical probes"] == 32 * 19 * 3 * 16
        for q in rows(log, "bank_test"):
            arm = q["bank_test"]
            r = truth[(arm, int(q["season"]))]
            for k in ("probes", "correct", "both", "conflicts", "revised", "food"):
                assert int(q[k]) == r[k], (arm, k, q[k], r[k])
            if arm in (
                "invited",
                "opposite",
                "fixed_attention",
                "lottery",
                "silent_reply",
                "chorus_invited",
            ):
                assert int(q["source_requests"]) == r["source_requests"]
        for mode in ("deaf", "fixed", "rest"):
            result = basin(
                "--load",
                base,
                "--rattles",
                mode,
                "--rattle-listen",
                "one",
                "--rattle-binding",
                "crossed",
                "--steps",
                121,
            )
            b = rows(result, "bank_field")[-1]
            assert int(b["writes"]) == 0
            if mode == "deaf":
                assert int(b["heard"]) == 0 and int(b["calls"]) > 0
            if mode == "rest":
                assert b["food"] == b["work"] == "0"
        silent = rows(
            basin(
                "--load",
                base,
                "--rattle-trials",
                16,
                "--rattle-exposure",
                0,
                "--steps",
                0,
            ),
            "bank_test",
        )
        for r in silent:
            if r["bank_test"] not in {"cut_upstream", "cut_sidepool"}:
                assert (
                    int(r["both"]) == int(r["revised"]) == 0
                    and r["identical"] == r["cases"]
                )
        placement = [
            "--load",
            base,
            "--rattle-trials",
            8,
            "--rattle-exposure",
            9,
            "--steps",
            0,
        ]
        domestic = rows(basin(*placement), "bank_test")
        from_same_residents = rows(
            basin(*placement, "--rattle-visitors", base), "bank_test"
        )
        assert domestic == from_same_residents
        assert "no living resonators" in basin(
            *placement, "--rattle-visitors", orig, ok=False
        )
        nursery = basin(
            "--load",
            orig,
            "--rattle-binding",
            "crossed",
            "--rattle-nursery",
            "solo-chorus",
            "--rattle-parents",
            32,
            "--rattle-broods",
            5,
            "--steps",
            0,
        )
        for account in rows(nursery, "bank_brood"):
            assert account["native_drums"] == "1" and int(account["food"]) <= 6 * int(
                account["pulls"]
            )
        drums = rows(nursery, "bank_drums")[-1]
        assert int(drums["settings"]) == sum(
            int(drums[k]) for k in ("rising", "falling", "gated", "flat", "silent")
        )
        # Unrelated old components must still load when the new appendix is absent.
        assert snapshot(orig)["bank"] == ()
        corrupt = bytearray(part.read_bytes())
        at = corrupt.index(struct.pack("<4I", 2, 111, 51031, 1))
        struct.pack_into("<I", corrupt, at + 4, 1 << 24)
        corrupt[-8:] = checksum(corrupt[:-8])
        bad = work / "bad.flood"
        bad.write_bytes(corrupt)
        assert "bank acoustics" in basin("--load", bad, "--steps", 0, ok=False)
        mesh = work / "mesh.flood"
        meshlog = basin(
            "--load", part, "--rattle-load", "silted", "--steps", 243, "--save", mesh
        )
        assert (
            snapshot(mesh)["bank"][0] == 3
            and int(rows(meshlog, "bank_mesh")[-1]["trapped"]) > 0
        )
        back = work / "loose.flood"
        basin("--load", mesh, "--rattle-load", "loose", "--steps", 0, "--save", back)
        assert snapshot(back)["bank"][0] == 3
        receipt = rows(
            basin("--load", full, "--extend", 128, "--steps", 0), "bank_field"
        )[-1]
        assert (
            receipt["contacts"] == field["contacts"]
            and receipt["food"] == field["food"]
        )
        body = work / "local.mind"
        slot = next(
            i for i, m in enumerate(info["listeners"]) if info["energy"][i] and m[5] & 2
        )
        germ = info["listeners"][slot]
        basin("--load", full, "--slot", slot, "--rattle-speaker", body, "--steps", 0)
        retained = struct.unpack_from("<6I", body.read_bytes(), -40)
        assert retained[:5] == germ[:5]
        age = min((info["tick"] - (germ[5] >> 9)) & 65535, 2 + (germ[3] >> 20 & 7))
        timer = 65535 << 9
        assert retained[5] == ((germ[5] & ~timer) | (((-age) & 65535) << 9))
        settled = work / "housing.flood"
        resident = basin(
            "--resident",
            body,
            "--side",
            16,
            "--density",
            0,
            "--steps",
            0,
            "--save",
            settled,
        )
        assert int(rows(resident, "bank_field")[-1]["living"]) == 1
        receiving = snapshot(settled)
        recipient = next(i for i, energy in enumerate(receiving["energy"]) if energy)
        assert receiving["listeners"][recipient][:5] == retained[:5]
        assert receiving["listeners"][recipient][5] == (
            (germ[5] & ~timer) | (((receiving["tick"] - age) & 65535) << 9)
        )
        talk = call(
            "talk",
            "--speaker",
            phone,
            "--listener",
            ROOT / "examples/runnel-consultant.mind",
            stdin="/backwater 117 crossed\n/backwater-work 16\nchoose\n/backwater-pass\nwhy\n/backwater-pull\n/backwater-divert\n/backwater-trials 8 6 43 crossed\n/quit\n",
        )
        events = rows(talk, "bank_talk")
        assert len(events) == 7 and any(int(e["inquiries"]) for e in events)
        for e in events:
            assert (
                int(e["source"]) + int(e["left"]) + int(e["right"]) + int(e["consumed"])
                == 8192
            )
        speech = rows(talk, "bank_dialogue")
        assert len(speech) == 6
        for r in speech:
            assert int(r["work"]) == 2 * int(r["probes"]) and int(r["decoded"]) == int(
                r["probes"]
            )
            assert int(r["faithful"]) <= int(r["cited"]) == int(r["changed_without"])
            if r["bank_dialogue"] == "mute":
                assert r["supplied"] == r["both_motors"] == "0"
        print(
            "Independent bank readout, dual-source physical probes, crossed taste, one-inquiry twins, public speech, checkpoint rejection and exact replay passed."
        )


if __name__ == "__main__":
    main()
