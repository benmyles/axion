#!/usr/bin/env python3
"""Verify two-port physical reports, novel cue withholding, inherited projections and individual ownership."""

import os, subprocess, sys, tempfile
from pathlib import Path
from press import ROOT, call, rows
from worksites import checksum

sys.path.insert(0, str(ROOT / "tools"))
from compound_audit import audit, cohort, read


def run():
    ex = ROOT / "examples"
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-compound-"
    ) as scratch:
        d = Path(scratch)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        home = d / "home.flood"
        with home.open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ex / "capillary-nursery.flood.zst"],
                stdout=f,
                check=True,
            )

        def assay(*args):
            return call(
                "floodplain",
                "--load",
                home,
                "--steps",
                0,
                "--catalytic-channels",
                2,
                *args,
            )

        a = ex / "compound-joint-writer.mind"
        b = ex / "compound-joint-reader.mind"
        r = ex / "compound-separable-reader.mind"
        for tag, reader, medium in (
            ("full", b, "active"),
            ("cross", r, "active"),
            ("inert", b, "inert"),
        ):
            t = d / (tag + ".tsv")
            s = assay(
                "--catalytic-pair",
                a,
                reader,
                "--catalytic-tests",
                128,
                "--class-seed",
                3501301,
                "--catalytic-history",
                901,
                "--catalytic-test-seed",
                13629397,
                "--catalytic-medium",
                medium,
                "--catalytic-trace",
                t,
            )
            q = audit(read(t), medium == "inert")
            grade = rows(s, "compound_exchange")[0]
            assert q["correct"] == int(grade["correct"]) and q["erase_both"] == 128
            if tag == "full":
                assert q["stable"] and not q["separable"] and q["correct"] > 450
            elif tag == "inert":
                assert q["correct"] == 128
            else:
                assert q["correct"] < 170
        for panels in (1, 3, 7, 13, 79):
            run = assay(
                "--catalytic-pair",
                a,
                b,
                "--catalytic-tests",
                min(8, panels),
                "--catalytic-benchmark",
                panels,
            )
            measurement = rows(run, "compound_benchmark")[0]
            assert (
                int(measurement["compound_benchmark"]) == 4 * panels
                and measurement["differing"] == "0"
                and measurement["receipt_bytes"] == "2"
            )
        stage = d / "group.tsv"
        opts = [
            "--catalytic-lessons",
            128,
            "--catalytic-history",
            0,
            "--catalytic-tests",
            16,
            "--catalytic-omit",
            3,
            "--class-seed",
            4829501,
            "--catalytic-diet",
            "plumes",
        ]
        log = assay(
            "--capillary-residents",
            64,
            "--capillary-contacts",
            0,
            "--retina-born-after",
            18432,
            "--catalytic-histories",
            2,
            "--capillary-screen",
            stage,
            *opts,
        )
        data = read(stage)
        verified = cohort(data)
        reported = rows(log, "compound_population")[0]
        assert all(v == int(reported[k]) for k, v in verified.items())
        assert all(
            sum(x[f"trained{k}"] for k in range(4)) == 256 and not x["trained3"]
            for x in data
        )
        subject = data[0]
        minds = []
        for role in ("writer", "reader"):
            p = d / (role + ".mind")
            call(
                "floodplain",
                "--load",
                home,
                "--steps",
                0,
                "--slot",
                subject[role],
                "--export-mind",
                p,
            )
            minds.append(p)
        profile = d / "profiles.tsv"
        t = d / "first.tsv"
        l = assay(
            "--catalytic-pair",
            *minds,
            "--catalytic-repeat",
            2,
            "--catalytic-profiles",
            profile,
            "--catalytic-trace",
            t,
            *opts,
        )
        trials = read(profile)
        assert sum(q["after"] for q in trials) == subject["after"]
        for q in trials:
            assert (
                q["trained3"] == 0
                and sum(q["trained" + str(i)] for i in range(3)) == 128
            )
        assert audit(read(t))["correct"] == trials[0]["after"]
        untested = [
            ex / "compound-unseen-writer.mind",
            ex / "compound-unseen-reader.mind",
        ]
        t = d / "unseen.tsv"
        assay(
            "--catalytic-pair",
            *untested,
            "--catalytic-tests",
            128,
            "--class-seed",
            6309133,
            "--catalytic-history",
            817,
            "--catalytic-test-seed",
            7012361,
            "--catalytic-trace",
            t,
        )
        q = audit(read(t))
        assert q["stable"] and q["separable"] and q["cue3"] >= 104
        turn = rows(
            call(
                "talk",
                "--speaker",
                a,
                "--listener",
                b,
                stdin=(ex / "compound-conversation.txt").read_text(),
            ),
            "compound_message",
        )
        assert len(turn) == 8 and all(
            x["external_drops"] == "30"
            and x["first_orientation"] == x["second_orientation"] == "2"
            for x in turn
        )
        assert sum(int(x["portions"]) for x in turn) >= 12 and all(
            int(x["portions"]) == 2 * (x["cue"] == x["feeder"]) for x in turn
        )
        nursery = d / "enrolled.flood"
        original = d / "original.mind"
        returned = d / "returned.mind"
        call(
            "floodplain",
            "--load",
            home,
            "--steps",
            0,
            "--slot",
            7218,
            "--export-mind",
            original,
        )
        call(
            "floodplain",
            "--load",
            home,
            "--steps",
            0,
            "--capillary-field",
            "live",
            "--return-lesson",
            original,
            a,
            "--save",
            nursery,
        )
        call(
            "floodplain",
            "--load",
            nursery,
            "--steps",
            0,
            "--slot",
            7218,
            "--export-mind",
            returned,
        )
        rr = rows(
            assay("--catalytic-pair", returned, b, "--catalytic-tests", 16),
            "compound_exchange",
        )[0]
        bb = rows(
            assay("--catalytic-pair", a, b, "--catalytic-tests", 16),
            "compound_exchange",
        )[0]
        assert rr["writer_contacts"] == bb["writer_contacts"] != "0"
        bad = bytearray(a.read_bytes())
        bad[-12:-8] = (3).to_bytes(4, "little")
        bad[-8:] = checksum(bad[:-8])
        damaged = d / "bad.mind"
        damaged.write_bytes(bad)
        assert "chemical contact extent" in call(
            "talk", "--speaker", damaged, "--listener", b, stdin="/quit\n", ok=False
        )
    print(
        "Compound signs passed material reconstruction, pupil/cohort parity, withheld-cue generalization, two sealed junctions and sparse habitat ownership."
    )


if __name__ == "__main__":
    run()
