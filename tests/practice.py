#!/usr/bin/env python3
"""Exercise field engagement, conserved public works, hidden material inquiry and compressed replies."""

import importlib.util
import os
import runpy
import struct
import tempfile
from pathlib import Path
from types import SimpleNamespace

from press import call, rows
from worksites import ROOT, checksum, mix, snapshot

spec = importlib.util.spec_from_file_location(
    "studio", ROOT / "tools/studio-language.py"
)
studio = importlib.util.module_from_spec(spec)
spec.loader.exec_module(studio)


def basin(*args, **options):
    return call("floodplain", *args, **options)


def pipe_oracle(length, steps, mode):
    water = [0] * length
    food = [0] * length
    water[0] = 4800 + length * 100
    food[0] = 3200 + length * 24
    dye = 0
    captured = [0, 0]
    for t in range(steps):
        for j in range(t % 2, length - 1, 2):
            if (
                (mode == "cut" or mode == "retied" and t < steps // 2)
                and j == length // 2 - 1
                or mode == "within_tiles"
                and j % 2
            ):
                continue
            difference = water[j] - water[j + 1]
            if abs(difference) < 2:
                continue
            a, b = (j, j + 1) if difference > 0 else (j + 1, j)
            volume = min(2, abs(difference) // 2)
            carbon = min(volume, food[a])
            water[a] -= volume
            water[b] += volume
            food[a] -= carbon
            food[b] += carbon
            if dye == a:
                dye = b
        captured[0] += water[-1]
        captured[1] += food[-1]
        water[-1] = food[-1] = 0
    return (*captured, dye)


def main():
    summarise = runpy.run_path(str(ROOT / "tools/journal.py"))["practice_summary"]
    credit = {
        "wick_field": [{"wick_field": "56"}],
        "practice_returns": [{"foreign_carbon": "24", "foreign_meals": "13"}],
    }
    bullets = summarise(lambda label: credit.get(label, []))[-1]
    assert any("24 units of carbon in 13 meals" in line for line in bullets)
    with tempfile.TemporaryDirectory(
        prefix="axion-sustained-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as tmp:
        work = Path(tmp)
        os.environ["AXION_JOURNAL_DIR"] = str(work / "journal")
        fresh = work / "fresh.flood"
        basin(
            "--side",
            16,
            "--worksites",
            "active",
            "--practice",
            "live",
            "--practice-seed",
            9781,
            "--steps",
            0,
            "--save",
            fresh,
        )
        founders = snapshot(fresh)["students"]
        for i, mind in enumerate(founders):
            key = mix(9781 + i * 5281)
            assert mind[:4] == tuple(
                mix(key + salt) for salt in (1709, 5209, 7297, 3181)
            )
            assert mind[4:] == (0, 0, 0, 0)
        base = ROOT / "examples/gaugebank.flood"
        one = work / "one.flood"
        two = work / "two.flood"
        split = work / "split.flood"
        full = work / "full.flood"
        basin(
            "--load",
            base,
            "--practice",
            "live",
            "--practice-appetite",
            "responsive",
            "--wicks",
            "flowing",
            "--steps",
            0,
            "--save",
            one,
        )
        basin("--load", one, "--steps", 77, "--save", two)
        basin("--load", two, "--steps", 118, "--save", split)
        log = basin(
            "--load",
            one,
            "--steps",
            195,
            "--practice-census",
            "--wick-census",
            "--wick-study",
            8,
            "--practice-bench",
            8091,
            "--practice-study",
            97,
            "--save",
            full,
        )
        assert split.read_bytes() == full.read_bytes()
        a = snapshot(full)
        n = a["side"] ** 2
        assert (
            a["practice"][1] == 2
            and len(a["students"]) == n
            and a["practice"][4 + 5] > 0
        )
        probe = rows(log, "practice_image")[0]
        assert int(probe["checked"]) == 4 * 8091 and probe["mismatches"] == "0"
        assert int(probe["directory_bytes"]) == 2 * int(probe["practice_image"]) + 4 * (
            (int(probe["practice_image"]) + 127) // 128
        )
        assert len(rows(log, "practice_foraging")) == 6
        paired = rows(log, "practice_window")[0]
        assert (
            paired["own_stays"] == paired["swapped_stays"]
            and int(paired["ticks_each"]) > 0
        )
        for day in rows(log, "practice_foraging"):
            assert int(day["food_stock"]) == int(day["remaining"]) + int(day["eaten"])
            assert int(day["initial"]) + int(day["eaten"]) == int(day["heat"]) + int(
                day["endowment"]
            )
        for x in rows(log, "wick_transfer"):
            w, f, reach = pipe_oracle(8, int(x["ticks"]), x["wick_transfer"])
            count = int(x["lanes"])
            assert (
                int(x["water_caught"]) == count * w
                and int(x["carbon_caught"]) == count * f
                and float(x["mean_distance"]) == reach
            )
            assert int(x["water_in"]) == int(x["water_left"]) + count * w
        b0 = snapshot(one)
        assert b0["wicks"][2] == a["wicks"][2] and a["wicks"][2] > 0
        restored = work / "unthread.flood"
        basin("--load", full, "--unthread-works", "--steps", 0, "--save", restored)
        undone = snapshot(restored)
        assert (
            undone["students"] == a["students"] and undone["contexts"] == a["contexts"]
        )
        wide = work / "widened.flood"
        basin("--load", full, "--extend", 128, "--steps", 0, "--save", wide)
        big = snapshot(wide)
        assert len(big["parcels"]) == 128**2 and big["wicks"] == a["wicks"]
        for j, old in enumerate(a["students"]):
            new = big["students"][j]
            assert new[:5] == old[:5] and new[6:] == old[6:]
            if old[5]:
                site = (old[5] >> 12) - 1
                assert new[5] >> 12 == (site // 32) * 64 + site % 32 + 1
        for j in range(2):
            assert (
                big["contexts"][j * 16384 : j * 16384 + n]
                == a["contexts"][j * n : (j + 1) * n]
            )
        # An on-disk tracer is a physical location. Check its origin even if it moved.
        for j, v in enumerate(a["parcels"]):
            migrated = big["parcels"][j // 64 * 128 + j % 64]
            if v & (1 << 27):
                old = (v >> 7) & 0xFFFFF
                assert ((migrated >> 7) & 0xFFFFF) == (old // 64) * 128 + old % 64
            assert (v & 0xC800007F) == (migrated & 0xC800007F)
        small = basin(
            "--load",
            base,
            "--practice-habitat",
            "foraging",
            "--practice-landscape",
            "island",
            "--practice-rear",
            3,
            "--practice-population",
            32,
            "--practice-study",
            13,
            "--steps",
            0,
        )
        assert len(rows(small, "practice_foraging")) == 6
        questions = work / "questions"
        traces = work / "trace.tsv"
        studio.generate(SimpleNamespace(path=questions, seed=19069, cases=11))
        guide = ROOT / "examples/gauge-student.mind"
        peer = ROOT / "examples/causeway-guide.mind"
        script = f"/studio 1\n/studio-trials {questions} {traces}\n/quit\n"
        call("talk", "--speaker", guide, "--listener", peer, stdin=script)
        rates = studio.audit(SimpleNamespace(path=traces))
        assert rates["heard", "needs_observation"]["both"] >= 5
        assert (
            rates["no_question", "needs_observation"]["both"] == 0
            and rates["false_reply", "needs_observation"]["both"] == 0
        )
        log = call(
            "talk",
            "--speaker",
            ROOT / "examples/gauge-reader.mind",
            "--listener",
            peer,
            stdin=(ROOT / "examples/gauge-dialogue.txt").read_text(),
        )
        received = rows(log, "studio_contact")
        done = [r for r in received if r["reached"] == "1"]
        assert len(done) == 2 and all(r["credit"] == "40" for r in done)
        bad = work / "damaged.mind"
        blob = bytearray(guide.read_bytes())
        start = len(blob) - 8 - 124
        struct.pack_into("<I", blob, start + 16, 7)
        blob[-8:] = checksum(blob[:-8])
        bad.write_bytes(blob)
        call("talk", "--speaker", bad, "--listener", peer, stdin="/quit\n", ok=False)
        print(
            "Voluntary work, capillary conservation, expansion, material credit, independent contingent programs and speech passed"
        )


if __name__ == "__main__":
    main()
