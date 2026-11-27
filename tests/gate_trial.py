#!/usr/bin/env python3
"""Independently reconstruct the wild somatic gate intervention and every enrolled identity."""

import csv
import os
import runpy
import struct
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT, checksum, mix

MASK = 2**32 - 1
SEED = 41971
ARMS = ("intact", "reverted", "rotated")
auditor = runpy.run_path(str(ROOT / "tools/gate-audit.py"))


def word(raw, offset):
    return struct.unpack_from("<I", raw, offset)[0]


def half(raw, offset):
    return struct.unpack_from("<H", raw, offset)[0]


def rotated(word, k):
    return ((word << k) | (word >> (32 - k))) & MASK


def edited_image(raw, phase):
    image = bytearray(raw)
    version, tick, side, bands = struct.unpack_from("<4I", raw, 8)
    assert version == 7
    n = side * side
    genes = 368 + n * 28
    bodies = genes + n * 172
    nerves = bodies + n * 96
    cortices = nerves + n * (16 * bands + 96) + 72
    truth = {}
    for slot in range(n):
        body = bodies + slot * 96
        born = word(raw, body + 20)
        energy = half(raw, body + 28)
        if not energy or born >= tick:
            continue
        development = word(raw, genes + 172 * slot + 40)
        key = mix(SEED ^ slot)
        arm = (key % 3 + phase) % 3
        acquired = changed = 0
        for band in range(raw[cortices + slot * 12 + 11]):
            address = nerves + 16 * (slot * bands + band) + 8
            identity = mix(development + 0x9E3779B9 * band)
            shift = 1 + mix(key ^ ((band * 0x9E3779B9) & MASK)) % 31
            for j in range(2):
                gate = word(raw, address + 4 * j)
                birth = mix(identity + j + 1)
                delta = birth ^ gate
                acquired += delta.bit_count()
                altered = (delta, 0, rotated(delta, shift))[arm]
                if arm == 2:
                    assert altered.bit_count() == delta.bit_count()
                changed += (delta ^ altered).bit_count()
                struct.pack_into("<I", image, address + 4 * j, birth ^ altered)
        if acquired:
            truth[slot] = (born, ARMS[arm], acquired, changed, energy)
    image[-8:] = checksum(image[:-8])
    return image, truth


def main():
    with tempfile.TemporaryDirectory(
        prefix="axion-gates-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as name:
        d = Path(name)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        run = lambda *a: call("floodplain", *a)
        src = d / "field.flood"
        run(
            "--side",
            32,
            "--materials",
            "off",
            "--density",
            90,
            "--neurons",
            256,
            "--seed",
            71,
            "--steps",
            67,
            "--save",
            src,
        )
        raw = src.read_bytes()
        normal = d / "normal.flood"
        run("--load", src, "--steps", 37, "--save", normal)
        paths = []
        for phase in range(3):
            prefix = ["--load", src, "--gate-seed", SEED, "--gate-phase", phase]
            expected, subjects = edited_image(raw, phase)
            path = d / f"phase{phase}.tsv"
            out = d / f"phase{phase}.flood"
            opts = [
                *prefix,
                "--gate-trial",
                "crossover",
                "--brain-dispatch",
                "packed",
                "--brain-memory",
                "paged",
                "--brain-pool-pages",
                0,
            ]
            run(*opts, "--steps", 0, "--save", out, "--gate-trace", path)
            assert out.read_bytes() == expected
            with path.open() as f:
                actual = {
                    int(r["slot"]): (
                        int(r["born"]),
                        r["arm"],
                        int(r["acquired"]),
                        int(r["changed"]),
                        int(r["opening_energy"]),
                    )
                    for r in csv.DictReader(f, delimiter="\t")
                }
            assert actual == subjects and len(actual) >= 16
            path = d / f"horizon{phase}.tsv"
            log = run(
                *opts,
                "--steps",
                37,
                "--report-every",
                13,
                "--gate-trace",
                path,
                "--save",
                out,
            )
            final = rows(log, "gate_trial")[-3:]
            assert sum(int(r["enrolled"]) for r in final) == len(subjects)
            paths.append(path)
            if phase == 0:
                sham = d / "sham.flood"
                log = run(
                    *prefix, "--gate-trial", "sham", "--steps", 37, "--save", sham
                )
                assert all(int(r["changed_bits"]) == 0 for r in rows(log, "gate_trial"))
                assert sham.read_bytes() == normal.read_bytes()
                fixed = d / "fixed.flood"
                run(
                    *prefix,
                    "--gate-trial",
                    "crossover",
                    "--brain-memory",
                    "dense",
                    "--brain-dispatch",
                    "fixed",
                    "--steps",
                    37,
                    "--save",
                    fixed,
                )
                assert out.read_bytes() == fixed.read_bytes()
        paired = auditor["read_crossovers"](paths)
        summary = auditor["summarize"](paired.values())
        assert summary["subjects"] == len(subjects)
        assert (
            summary["outcomes"]["rotated"]["changed_bits"]
            and summary["outcomes"]["reverted"]["changed_bits"]
        )
        groups = auditor["habitat_groups"](paired, src, 16)["groups"]
        source = [g for k, g in groups.items() if k in ("core", "outer")]
        forms = [g for k, g in groups.items() if k not in ("core", "outer")]
        for partition in (source, forms):
            assert sum(g["subjects"] for g in partition) == summary["subjects"]
            for arm in ARMS:
                assert (
                    sum(g["outcomes"][arm]["surviving"] for g in partition)
                    == summary["outcomes"][arm]["surviving"]
                )
        ranks = [
            g["source_population"]
            for k, g in sorted(groups.items())
            if k.startswith("form-")
        ]
        assert len(ranks) == 8 and ranks == sorted(ranks, reverse=True)
        print(
            "Verified every retained natal gate, treatment rotation, cohort identity, unaltered sham habitat and dense/paged replay."
        )


if __name__ == "__main__":
    main()
