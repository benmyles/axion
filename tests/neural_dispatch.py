#!/usr/bin/env python3
"""Replay variable-width brain updates, sparse page reuse and compact persistence on CUDA."""

import os
from pathlib import Path
import struct
import tempfile

from press import call, rows
from worksites import ROOT, checksum


def core_offset(n):
    return 368 + n * (24 + 4 + 172 + 96)


def check_survey(log, path):
    raw = path.read_bytes()
    end = rows(log, "frontier_census")[-1]
    old, side, start = (int(end[k]) for k in ("origin", "side", "start"))
    sites = side * side
    bins = min(side, 32)
    density = [[0] * bins for _ in range(bins)]
    body = 368 + sites * (24 + 4 + 172)
    outside = young = core = fibre = furthest = 0
    founders = set()

    def distance(u):
        return min(u - old + 1, side - u) if u >= old else 0

    for i in range(sites):
        x, y = i % side, i // side
        annex = x >= old or y >= old
        if annex and any(struct.unpack_from("<2H", raw, 368 + i * 24 + 16)):
            fibre += 1
        (slot,) = struct.unpack_from("<I", raw, 368 + sites * 24 + 4 * i)
        if not struct.unpack_from("<H", raw, body + slot * 96 + 28)[0]:
            continue
        density[y * bins // side][x * bins // side] += 1
        if annex:
            outside += 1
            founder, born = struct.unpack_from("<2I", raw, body + slot * 96 + 16)
            founders.add(founder)
            young += born > start
            furthest = max(furthest, distance(x) + distance(y))
        else:
            core += 1
    actual = [
        list(map(int, r["residents"].split(","))) for r in rows(log, "frontier_row")
    ]
    assert actual == density
    assert (outside, young, core, fibre, furthest, len(founders)) == tuple(
        int(end[k])
        for k in (
            "outside",
            "born_after",
            "core_residents",
            "fibre_sites",
            "furthest",
            "founders",
        )
    )


def irregular_fixture(original, destination):
    raw = bytearray(original.read_bytes())
    version, _, side, bands = struct.unpack_from("<4I", raw, 8)
    assert version == 7 and 2 <= bands <= 32
    n = side * side
    first = core_offset(n)
    cortex = first + n * (16 * bands + 96)
    alive = 0
    for slot in range(n):
        struct.pack_into(
            "<H", raw, 368 + n * (24 + 4 + 172) + 96 * slot + 78, 1 + (slot & 1)
        )
        width = 2 + slot % (bands - 1)
        organ = cortex + 72 + slot * 12
        struct.pack_into("<II4B", raw, organ, 0x55555555, 0, width, 2, width, 2)
        alive += width
        # A dormant, individually changed band must survive even above the active extent.
        if slot % 9 == 0:
            struct.pack_into(
                "<4I", raw, first + 16 * (slot * bands + bands - 1), slot, 17, 23, 31
            )
    struct.pack_into("<Q", raw, cortex + 8, alive * 32)
    raw[-8:] = checksum(raw[:-8])
    destination.write_bytes(raw)


def main():
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="cortical-dispatch-"
    ) as name:
        d = Path(name)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")

        def run(*args, **kwargs):
            return call("floodplain", *args, **kwargs)

        dense = ("--brain-dispatch", "fixed", "--brain-memory", "dense")
        packed = (
            "--brain-dispatch",
            "packed",
            "--brain-memory",
            "paged",
            "--brain-pool-pages",
            0,
            "--brain-collect",
            17,
        )
        base = d / "birth.flood"
        mixed = d / "uneven.flood"
        run(
            "--side",
            16,
            "--materials",
            "off",
            "--neurons",
            1024,
            "--density",
            100,
            "--seed",
            58897,
            "--neurogenesis",
            "inherited",
            "--steps",
            0,
            "--save",
            base,
            *dense,
        )
        irregular_fixture(base, mixed)
        for suffix, lesion in (
            ("ordinary", ()),
            ("muted", ("--silence-new-cohort", "a")),
        ):
            a = d / ("fixed-" + suffix + ".flood")
            b = d / ("packed-" + suffix + ".flood")
            run(
                "--load",
                mixed,
                "--steps",
                67,
                "--report-every",
                47,
                "--save",
                a,
                *dense,
                *lesion,
            )
            log = run(
                "--load",
                mixed,
                "--steps",
                67,
                "--report-every",
                23,
                "--save",
                b,
                *packed,
                *lesion,
            )
            assert b.read_bytes() == a.read_bytes()
            m = rows(log, "neural_dispatch")[-1]
            assert int(m["irregular"]) and m["neural_dispatch"] == "packed"
        compact = d / "compact.flood"
        roundtrip = d / "decoded.flood"
        run(
            "--load",
            mixed,
            "--steps",
            0,
            "--brain-bench",
            7,
            "--brain-file",
            "compact",
            "--save",
            compact,
            *packed,
        )
        run(
            "--load",
            compact,
            "--steps",
            0,
            "--brain-file",
            "dense",
            "--save",
            roundtrip,
            *dense,
        )
        assert roundtrip.read_bytes() == mixed.read_bytes()
        run(
            "--side",
            16,
            "--materials",
            "off",
            "--neurons",
            512,
            "--density",
            100,
            "--seed",
            58897,
            "--steps",
            0,
            "--save",
            base,
            *dense,
        )
        irregular_fixture(base, mixed)
        run(
            "--load",
            mixed,
            "--neurons",
            1024,
            "--steps",
            0,
            "--save",
            roundtrip,
            *packed,
        )
        begin = core_offset(256)
        narrow = mixed.read_bytes()[begin:]
        wide = roundtrip.read_bytes()[begin:]
        for slot in range(256):
            assert wide[512 * slot : 512 * (slot + 1)] == narrow[
                256 * slot : 256 * (slot + 1)
            ] + bytes(256)
        src = ROOT / "examples/floatbank.flood"
        continuous = d / "continuous.flood"
        part = d / "part.flood"
        after = d / "after.flood"
        run("--load", src, "--steps", 137, "--save", continuous, *dense)
        log = run(
            "--load",
            src,
            "--steps",
            79,
            "--brain-file",
            "compact",
            "--save",
            part,
            *packed,
        )
        run(
            "--load",
            part,
            "--steps",
            58,
            "--brain-file",
            "dense",
            "--save",
            after,
            *packed,
        )
        assert after.read_bytes() == continuous.read_bytes()
        m = rows(log, "neural_storage")[-1]
        assert int(m["collections"]) > 0 and int(m["device_bytes"]) < int(
            m["dense_bytes"]
        )
        # Unvisited embryos have a genome-defined image, including empty sites.
        untouched = d / "unvisited.flood"
        encoded = d / "unvisited-short.flood"
        args = (
            "--side",
            16,
            "--density",
            0,
            "--neurons",
            1024,
            "--neurogenesis",
            "inherited",
            "--seed",
            179,
            "--steps",
            0,
        )
        run(*args, *dense, "--save", untouched)
        cols = run(*args, *packed, "--brain-file", "compact", "--save", encoded)
        e = rows(cols, "neural_file")[-1]
        assert int(e["explicit_bands"]) == 0 and int(e["developmental_rows"]) == 256
        run(
            "--load",
            encoded,
            "--brain-file",
            "dense",
            "--steps",
            0,
            "--save",
            roundtrip,
            *packed,
        )
        assert roundtrip.read_bytes() == untouched.read_bytes()
        broken = bytearray(encoded.read_bytes())
        broken[core_offset(256) + 8] = 0x80
        broken[-8:] = checksum(broken[:-8])
        bad = d / "bad.flood"
        bad.write_bytes(broken)
        assert "sparse neural row" in run("--load", bad, "--steps", 0, ok=False)
        for scenario, args in (
            ("expand", ("--load", part, "--extend", 128)),
            (
                "house",
                (
                    "--side",
                    16,
                    "--density",
                    0,
                    "--neurons",
                    1024,
                    "--neurogenesis",
                    "inherited",
                    "--resident",
                    ROOT / "examples/parallax-reader.mind",
                ),
            ),
        ):
            outcomes = []
            for i, engine in enumerate((dense, packed)):
                out = d / (scenario + str(i) + ".flood")
                log = run(
                    *args,
                    "--steps",
                    25,
                    "--brain-file",
                    "dense",
                    "--save",
                    out,
                    *engine,
                )
                outcomes.append(out.read_bytes())
                if scenario == "expand":
                    check_survey(log, out)
            assert outcomes[0] == outcomes[1], scenario
        print(
            "Verified every cortical width from 2 to 32, dormant changes, growing and silenced bands, sparse spills, native file conversion, expansion and rehousing."
        )


if __name__ == "__main__":
    main()
