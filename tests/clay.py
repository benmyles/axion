#!/usr/bin/env python3
"""Independent reconstruction of latent spikes, sounding choices and the lifetime of a portable inscription."""

import os
import struct
import subprocess
import tempfile
from collections import Counter
from pathlib import Path

from siphons import code, projections
from worksites import ROOT, checksum, mix, snapshot

AX = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()
BANK = ROOT / "examples/reedbed.flood"
PUPIL = ROOT / "examples/loam-reader.mind"
PEERS = ROOT / "examples/loam"


def run(*args, stdin="", ok=True):
    r = subprocess.run(list(map(str, [AX, *args])), text=True, input=stdin, capture_output=True)
    if (r.returncode == 0) != ok:
        raise AssertionError(str(args) + r.stderr + r.stdout[-400:])
    return r.stdout + r.stderr


def fields(log, key):
    return [
        dict(x.split("=", 1) for x in line.split() if "=" in x)
        for line in log.splitlines()
        if line.startswith(key + "=")
    ]


def reader():
    data = PUPIL.read_bytes()
    end = data.rindex(struct.pack("<II", 0x434C4159, 72))
    assert data[-8:] == checksum(data[:-8])
    m = struct.unpack_from("<12I", data, end - 64)
    a = list(struct.unpack_from("<14I", data, -72))
    cur = (m[11] >> 4) & 3
    used = [j for j in range(4) if m[11] >> j & 1]
    assert len(used) >= 2 and cur in used
    all_banks = {}
    for j in used:
        actor = a.copy()
        if j != cur:
            other = (j or cur) - 1
            actor[6] = m[3 + other]
            actor[7] = m[6 + other]
        all_banks[j] = projections(actor)
    return m, a, all_banks


def tile(which):
    raw = Path(str(PEERS) + "-" + which + ".clay").read_bytes()
    assert len(raw) == 56 and raw[:8] == b"AXCLAY\0\0" and raw[-8:] == checksum(raw[:-8])
    return tuple(raw[28:36])


def anticipate(m, a, banks, reliefs, seed, excluded, limit):
    level = {j: 8 for j in banks}
    used = set()
    spoken = 0
    mask = 0

    def votes(j, r):
        is_question = r & 16
        value = code(r >> 2 & 3, a[4] >> (0 if is_question else 3))
        return set(banks[j][0 if is_question else 2][value])

    for k in range(limit):
        draws = []
        for i, e in enumerate(reliefs):
            if i in used or e & 144 != 144 or (e >> 2) & 3 == excluded:
                continue
            earlier = []
            disagree = 0
            for j in banks:
                v = votes(j, e)
                for prior in earlier:
                    disagree += len(v ^ prior) + (not (v & prior)) * (1 + (m[0] >> 23 & 3))
                earlier.append(v)
            if m[0] >> 26 & 1:
                disagree = 64 - disagree
            if not m[0] >> 27 & 1:
                disagree = 0
            score = 256 * disagree + (mix(mix(seed + k * 799) + i * 587) & 255)
            draws.append((score, -i))
        if not draws:
            break
        at = -max(draws)[1]
        used.add(at)
        e = reliefs[at]
        mask |= 1 << at
        spoken = (spoken << 3) | ((e & 3) + 1)
        syllable = code(e & 3, a[4] >> 6)
        for j in banks:
            pred = votes(j, e)
            index = 4 if syllable not in pred else len(pred) - 1
            update = (m[1] >> (2 * index)) & 3
            if m[1] >> (10 + index) & 1:
                update = -update
            level[j] = min(15, max(0, level[j] + update))

    def potential(j):
        return (
            16 * (15 - level[j]) * (1 + ((m[1] >> 16) & 3))
            + ((j + 4 - (seed & 3)) & 3)
            + (j != 0) * ((m[1] >> 20) & 7)
        )

    chosen = min(banks, key=potential)
    margin = level[chosen] - max([0, *[v for j, v in level.items() if j != chosen]])
    return len(used), mask, spoken, chosen if margin >= 1 + (m[1] >> 24 & 1) else 4


def test(tmp):
    os.environ["AXION_JOURNAL_DIR"] = str(tmp / "journal")
    m, a, banks = reader()
    scenes = []
    expected = []
    for seed in (37, 211, 3109, 9817):
        for who in ("a", "b"):
            for filt in range(4):
                for taps in (1, 3):
                    scenes += [
                        f"/reed {seed} {mix(seed) & 255} {filt} 1",
                        f'/kiln "{PEERS}-{who}.clay" covered',
                        f"/knock {taps}",
                    ]
                    expected.append(anticipate(m, a, banks, tile(who), seed, filt, taps))
    log = run(
        "talk", "--speaker", PUPIL, "--listener", str(PEERS) + "-a.mind", stdin="\n".join([*scenes, "/quit", ""])
    )
    got = [
        (int(q["strokes"]), int(q["sounded_faces"]), int(q["pulses"]), int(q["source_cell"]))
        for q in fields(log, "clay_contact")
        if q["clay_contact"] == "1"
    ]
    # Each physical visit spends body energy; recharge independent batches if the public budget runs out.
    if got != expected:
        for start in range(0, len(expected), 8):
            part = scenes[3 * start : 3 * (start + 8)]
            rows = fields(
                run(
                    "talk",
                    "--speaker",
                    PUPIL,
                    "--listener",
                    str(PEERS) + "-a.mind",
                    stdin="\n".join([*part, "/quit", ""]),
                ),
                "clay_contact",
            )
            values = [
                tuple(int(q[k]) for k in ("strokes", "sounded_faces", "pulses", "source_cell"))
                for q in rows
                if q["clay_contact"] == "1"
            ]
            assert values == expected[start : start + 8], (start, values, expected[start : start + 8])
    items = tmp / "questions.txt"
    lines = [
        f"2108515187 225 2 1 0 {source} {piece} {budget} {vis} {text} | {role} {truth}"
        for source, piece, budget, vis, text, role, truth in [
            (0, 0, 4, 2, "who worked this clay?", "source", "willow"),
            (1, 1, 4, 2, "who worked this clay?", "source", "alder"),
            (0, 1, 4, 2, "who worked this clay?", "source", "willow"),
            (0, 0, 0, 0, "who worked this clay?", "source", "willow"),
            (1, 1, 1, 0, "what did ridge yield?", "effect", "opal"),
            (1, 1, 4, 2, "where did opal appear?", "origin", "ridge"),
            (1, 1, 4, 2, "undefined unguided sound", "unknown", "_"),
        ]
    ]

    def batch():
        items.write_text("\n".join(lines) + "\n")
        return run(
            "talk",
            "--speaker",
            PUPIL,
            "--listener",
            str(PEERS) + "-a.mind",
            stdin=f'/reed 1\n/clay-visitors "{PEERS}"\n/clay-tests "{items}"\n/quit\n',
        )

    log = batch()
    original = fields(log, "clay_sentence")
    assert [r["said"] for r in original] == ["willow", "alder", "alder", "_", "opal", "ridge", "_"], original
    assert original[-1]["paid_taps"] == "0" and original[3]["paid_taps"] == "0"
    lines[:] = [l.split("|")[0] + "| ignored potatoes" for l in lines]
    assert fields(batch(), "clay_sentence") == original
    farm = lambda *args, **kwargs: run("floodplain", *args, **kwargs)
    raw = tmp / "kiln.flood"
    full = tmp / "whole.flood"
    split = tmp / "half.flood"
    tail = tmp / "continued.flood"
    farm("--load", BANK, "--pottery", "fired", "--clay-reach", "shared", "--steps", 128, "--save", raw)
    earlier = snapshot(raw)
    assert earlier["workshop"][2] > 0 and sum(bool(q[0] or q[1]) for q in earlier["ware"]) > 0
    probes = farm(
        "--load",
        raw,
        "--steps",
        0,
        "--clay-bench",
        32768,
        "--clay-test",
        2,
        "--clay-dig",
        2,
        "--clay-foster",
        2,
        "--clay-classes",
        16,
        "--clay-preparation",
        32,
        "--clay-history",
        32,
        "--clay-reversals",
        16,
        "--work-subjects",
        32,
        "--save",
        full,
    )
    assert raw.read_bytes() == full.read_bytes() and fields(probes, "clay_deployment")[0]["differences"] == "0"
    assert all(int(x["target_strikes"]) == 0 for x in fields(probes, "clay_transfer"))
    for step, before, after in [(37, raw, full), (11, raw, split), (26, split, tail)]:
        farm("--load", before, "--steps", step, "--save", after)
    assert full.read_bytes() == tail.read_bytes()
    farm(
        "--load",
        raw,
        "--pottery",
        "fired",
        "--clay-hands",
        "rest",
        "--clay-muscle",
        "quiet",
        "--siphon-workers",
        "still",
        "--steps",
        96,
        "--save",
        tail,
    )

    def objects(book):
        return Counter(q for q in [*book["ware"], *book["cargo"]] if q[0] or q[1])

    assert objects(earlier) == objects(snapshot(tail))
    farm("--load", raw, "--extend", 128, "--steps", 0, "--save", full)
    wider = snapshot(full)
    for y in range(64):
        assert wider["ware"][128 * y : 128 * y + 64] == earlier["ware"][64 * y : 64 * (y + 1)]
    assert wider["cargo"][:4096] == earlier["cargo"] and wider["potters"][:4096] == earlier["potters"]
    farm("--load", raw, "--steps", 0, "--wash-clay", "--save", split)
    empty = snapshot(split)
    assert not objects(empty) and empty["field"] == earlier["field"]
    corrupted = bytearray(earlier["raw"][:-8])
    p = earlier["clay_pos"] + 112 + 4096 * 48
    struct.pack_into("<I", corrupted, p + 8, earlier["tick"] + 100)
    split.write_bytes(corrupted + checksum(corrupted))
    assert "clay object" in farm("--load", split, "--steps", 0, ok=False)
    farm("--side", 16, "--resident", PUPIL, "--steps", 0, "--save", raw)
    farm("--load", raw, "--slot", 0, "--export-mind", tmp / "returned.mind", "--steps", 0)
    assert (tmp / "returned.mind").read_bytes().count(struct.pack("<II", 0x434C4159, 72)) == 1
    home = snapshot(raw)
    assert list(home["native_words"]) == [0] and home["potters"][0] == m
    broken = bytearray(Path(str(PEERS) + "-a.clay").read_bytes())
    broken[17] ^= 2
    (tmp / "broken.clay").write_bytes(broken)
    assert "clay collection" in run(
        "talk",
        "--speaker",
        PUPIL,
        "--listener",
        str(PEERS) + "-a.mind",
        stdin=f'/reed 1\n/kiln "{tmp / "broken.clay"}" covered\n/quit\n',
        ok=False,
    )


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="axion-clay-audit-") as temp:
        test(Path(temp))
    print(
        "PASS independent latent spikes and exploratory strokes, hidden-source conversation, shuffling, clay transport and owner-preserving checkpoints"
    )
