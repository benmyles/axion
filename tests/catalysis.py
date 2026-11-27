#!/usr/bin/env python3
"""Check catalytic memory, actual biological transduction, learned codes and checkpoint continuation on CUDA."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path
from press import ROOT, call, rows
from worksites import checksum

sys.path.insert(0, str(ROOT / "tools"))
from capillary_audit import read
from catalytic_audit import (
    boundaries,
    correspondence,
    experienced,
    pairs,
    witnesses,
    wild,
    receptions,
)


def run():
    with tempfile.TemporaryDirectory(
        dir=os.getenv("TMPDIR", "/var/tmp"), prefix="axion-catalyst-"
    ) as tmp:
        d = Path(tmp)
        ex = ROOT / "examples"
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        home = d / "brook.flood"
        with home.open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ex / "capillary-nursery.flood.zst"],
                stdout=f,
                check=True,
            )

        def runfield(*args):
            return call("floodplain", "--load", home, *args)

        f = d / "all.tsv"
        runfield("--steps", 0, "--capillary-catalysis-proof", f)
        assert boundaries(read(f)) == dict(
            cases=32768, self_closing=448, state_changes=5184
        )
        whole = d / "all.flood"
        sample = d / "seen.flood"
        partial = d / "partial.flood"
        joined = d / "joined.flood"
        eyes = d / "eyes.tsv"
        film = d / "film.tsv"
        args = [
            "--capillary-field",
            "live",
            "--capillary-memory",
            "regional",
            "--chemical-foraging",
            "live",
            "--capillary-catalysis",
            "bound",
            "--capillary-period",
            2,
        ]
        stats = runfield(
            "--steps",
            512,
            *args,
            "--save",
            sample,
            "--capillary-events",
            eyes,
            "--context-events",
            d / "choices.tsv",
            "--capillary-window",
            64,
            "--capillary-frames",
            128,
            "--catalytic-film",
            film,
            "--chemical-witness",
            d / "arrivals.tsv",
        )
        assert (
            experienced(read(eyes))["exposures"] > 0 and wild(read(eyes))["visits"] > 0
        )
        assert int(rows(stats, "chemical_foraging")[0]["contact_edits"]) > 0
        v = witnesses(read(film))
        assert (
            int(rows(stats, "catalytic_window")[0]["witnessed_closures"])
            == v["witnessed"]
            > 0
        )
        observed = receptions(read(d / "arrivals.tsv"))
        official = rows(stats, "chemical_receptions")[0]
        assert int(official["encounters"]) == observed.get("encounters", 0)
        for k, v in observed.items():
            assert v == int(official[k])
        runfield("--steps", 512, *args, "--save", whole)
        runfield("--steps", 119, *args, "--save", partial)
        call("floodplain", "--load", partial, "--steps", 393, "--save", joined)
        assert sample.read_bytes() == whole.read_bytes() == joined.read_bytes()
        taught = ex / "context-left-mark-sender.mind"
        original = d / "born.mind"
        rest = d / "returned.mind"
        home_with_lesson = d / "home.flood"
        runfield("--steps", 0, "--slot", 9434, "--export-mind", original)
        runfield(
            "--steps",
            0,
            "--capillary-field",
            "live",
            "--chemical-foraging",
            "fixed",
            "--return-lesson",
            original,
            taught,
            "--save",
            home_with_lesson,
        )
        call(
            "floodplain",
            "--load",
            home_with_lesson,
            "--steps",
            0,
            "--slot",
            9434,
            "--export-mind",
            rest,
        )
        returned = []
        for mind in (taught, rest):
            exam = runfield(
                "--steps",
                0,
                "--catalytic-pair",
                mind,
                ex / "context-left-mark-reader.mind",
                "--catalytic-tests",
                96,
                "--class-seed",
                3689843,
            )
            r = rows(exam, "material_correspondence")[0]
            returned.append((r["sender_on"], r["sender_off"]))
        assert returned[0] == returned[1] and returned[0] != ("0", "0")
        cohort = d / "cohort.tsv"
        base = [
            "--steps",
            0,
            "--capillary-residents",
            64,
            "--capillary-contacts",
            0,
            "--capillary-screen",
            cohort,
            "--retina-born-after",
            18432,
            "--catalytic-lessons",
            128,
            "--catalytic-tests",
            12,
            "--catalytic-histories",
            2,
            "--class-seed",
            720911,
        ]
        group = rows(runfield(*base), "material_relay")[0]
        data = read(cohort)
        cert = pairs(data)
        for k in cert:
            assert cert[k] == int(group[k])
        a, b = d / "a.mind", d / "b.mind"
        r = data[0]
        runfield("--steps", 0, "--slot", r["sender"], "--export-mind", a)
        runfield("--steps", 0, "--slot", r["receiver"], "--export-mind", b)
        observed = 0
        for life in range(2):
            tr = d / f"test-{life}.tsv"
            res = runfield(
                "--steps",
                0,
                "--catalytic-pair",
                a,
                b,
                "--catalytic-lessons",
                128,
                "--catalytic-history",
                life,
                "--class-seed",
                720911,
                "--catalytic-tests",
                12,
                "--catalytic-trace",
                tr,
            )
            grade = correspondence(read(tr))
            assert (
                int(rows(res, "material_correspondence")[0]["correct"])
                == grade["correct"]
            )
            observed += grade["correct"]
        assert observed == r["after"]

        for kind in ("sender", "receiver"):
            donor = (
                ex
                / f"context-left-mark-{'sender' if kind=='sender' else 'reader'}.mind"
            )
            pupil_table = d / f"students-{kind}.tsv"
            parameters = list(base)
            parameters[parameters.index(cohort)] = pupil_table
            runfield(*parameters, "--catalytic-tutor", kind, donor)
            table = read(pupil_table)
            pairs(table)
            assert all(r[kind + "_edits"] == 0 for r in table)
            target = "receiver" if kind == "sender" else "sender"
            r = table[0]
            novice = d / f"novice-{kind}.mind"
            runfield("--steps", 0, "--slot", r[target], "--export-mind", novice)
            source, reader = (donor, novice) if kind == "sender" else (novice, donor)
            out = 0
            for life in range(2):
                record = runfield(
                    "--steps",
                    0,
                    "--catalytic-pair",
                    source,
                    reader,
                    "--catalytic-learning",
                    target,
                    "--catalytic-lessons",
                    128,
                    "--class-seed",
                    720911,
                    "--catalytic-history",
                    life,
                    "--catalytic-tests",
                    12,
                )
                out += int(rows(record, "material_correspondence")[0]["correct"])
            assert out == r["after"]

        def pair(x, y):
            return [
                ex / f"context-{x}-mark-sender.mind",
                ex / f"context-{y}-mark-reader.mind",
            ]

        lesson = (ex / "catalytic-correspondence.txt").read_text()
        wins = {}
        for who, other in (
            ("left", "left"),
            ("right", "right"),
            ("left", "right"),
            ("right", "left"),
        ):
            a, b = pair(who, other)
            tr = d / f"{who}-{other}.tsv"
            frozen = [
                "--steps",
                0,
                "--catalytic-pair",
                a,
                b,
                "--catalytic-tests",
                128,
                "--catalytic-test-seed",
                14081839,
                "--catalytic-history",
                183,
                "--class-seed",
                720911,
            ]
            runfield(*frozen, "--catalytic-trace", tr)
            v = correspondence(read(tr))
            wins[who, other] = v["correct"]
            assert v["erased"] == 128 and (
                v["correct"] >= 225 if who == other else v["correct"] <= 31
            )
            trace = rows(
                call("talk", "--speaker", a, "--listener", b, stdin=lesson),
                "material_message",
            )
            assert len(trace) == 4 and sum(int(q["portions"]) for q in trace) == (
                8 if who == other else 0
            )
        a, b = pair("left", "left")
        se = rows(
            call(
                "talk",
                "--speaker",
                a,
                "--listener",
                b,
                stdin=(ex / "catalytic-latch.txt").read_text(),
            ),
            "sealed_cycle",
        )
        assert [int(v["flow"]) for v in se] == [1, 0, 0, 1, 1, 0] and all(
            v["remaining"] == "31" for v in se
        )
        corrupted = bytearray(a.read_bytes())
        corrupted[-12:-8] = (1).to_bytes(4, "little")
        corrupted[-8:] = checksum(corrupted[:-8])
        bad = d / "truncated.mind"
        bad.write_bytes(corrupted)
        assert "chemical contact extent" in call(
            "talk", "--speaker", bad, "--listener", b, stdin="/quit\n", ok=False
        )
    print(
        "Catalysis passed 32,768 conserved contacts, witnessed consecutive closures, native field restart, two-body learning and oppositely learned physical messages."
    )


if __name__ == "__main__":
    run()
