#!/usr/bin/env python3
"""Replay a GPU builder using independent scalar optics, motor mechanics, inventories and exposure."""

from collections import Counter
import gzip
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BIN = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()
MASK = 2**32 - 1


def mix(x):
    x &= MASK
    x = ((x ^ (x >> 16)) * 0x21F0AAAD) & MASK
    x = ((x ^ (x >> 15)) * 0x735A2D97) & MASK
    return x ^ (x >> 15)


def pick(m, k):
    return next((i for s in range(32) if m >> (i := (s + k) % 32) & 1), 0)


def rotate(code, n):
    return ((code << (2 * n)) | (code >> (8 - 2 * n))) & 255


def neighbor(i, d):
    return ((i % 8 + (d == 1) - (d == 3)) % 8) + 8 * ((i // 8 + (d == 2) - (d == 0)) % 4)


def part(x):
    return [(x >> (8 * j)) & 255 for j in range(4)]


def pack(p):
    return sum(v << (8 * j) for j, v in enumerate(p))


def face(p, d):
    return p[0] >> (2 * ((d - (p[3] & 1)) % 4)) & 3


def bond(p, q, d):
    a, b = face(p, d), face(q, d ^ 2)
    return bool(a and b and (a == 3 or b == 3 or a != b))


def close(board, seed, removed=-1):
    reached = {seed} if seed != removed and board[seed][0] else set()
    frontier = list(reached)
    for i in frontier:
        for d in range(4):
            j = neighbor(i, d)
            if j not in reached and j != removed and bond(board[i], board[j], d):
                reached.add(j)
                frontier.append(j)
    return reached


def expose(board, lamp, target, ticks, open=True):
    p = [v[:] for v in board]
    for v in p:
        v[2] = 0
    photons = made = 0
    for t in range(ticks):
        d = 1 if t % 4 < 2 else 2
        for i in range(32):
            if ((i if d == 1 else i // 8) ^ t) & 1:
                continue
            j = neighbor(i, d)
            a, b = p[i], p[j]
            if open and bond(a, b, d):
                diff = 4 * (a[2] - b[2]) + ((a[1] >> 4) & 3) - ((b[1] >> 4) & 3)
                src, dst = (a, b) if diff > 0 else (b, a)
                if abs(diff) >= 4 and src[2] and dst[2] < 2 + (dst[1] & 15):
                    src[2] -= 1
                    dst[2] += 1
        for i, v in enumerate(p):
            if not v[0]:
                continue
            if i == lamp and v[1] >> 4 & 3 == 3 and v[2] < 2 + (v[1] & 15):
                v[2] += 1
                photons += 1
            if i == target and (v[1] >> 2) & 3 == 0 and v[2] >= 1 + ((v[1] >> 5) & 3) and made < 64:
                v[2] -= 1
                made += 1
            if v[1] & 128:
                if v[2] + 1 >= 2 + (v[1] & 15):
                    v[3] |= 1
                elif v[2] <= (2 + (v[1] & 15)) // 3:
                    v[3] &= ~1
    accumulated = sum(v[2] for v in p)
    assert photons == accumulated + made
    return made, photons, accumulated


def rows(t, key):
    return [dict(w.split("=", 1) for w in l.split() if "=" in w) for l in t.splitlines() if l.startswith(key + "=")]


def decode_values(text, k):
    return list(map(int, rows(text, k)[0][k].split(",")))


def audit_trace(text, ideas, span, ticks):
    frame = {k: int(v) for k, v in rows(text, "shoal_specimen")[0].items()}
    opening = [part(x) for x in decode_values(text, "shoal_before")]
    inventory = [v[:] for v in opening]
    ids = [i if p[0] else 32 for i, p in enumerate(inventory)]
    trace = decode_values(text, "shoal_choices")
    memory = decode_values(text, "shoal_memory")
    g = decode_values(text, "shoal_genes")
    lamp = frame["light"]
    beds = (frame["patch_a"], frame["patch_b"])
    target = beds[frame["diet"]]
    assert all(not bond(p, opening[neighbor(i, d)], d) for i, p in enumerate(opening) for d in (1, 2))
    dist = lambda i, j: (
        min(abs(i % 8 - j % 8), 8 - abs(i % 8 - j % 8)) + min(abs(i // 8 - j // 8), 4 - abs(i // 8 - j // 8))
    )
    assert all(dist(lamp, z) == span for z in beds)
    assert expose(opening, lamp, target, ticks)[0] == 0
    hood = [sum(1 << neighbor(i, d) for d in range(4)) for i in range(32)]

    def diffuse(s):
        r = 0
        while s:
            flag = s & -s
            s -= flag
            r |= hood[flag.bit_length() - 1]
        return r

    emit = lambda row, at: (g[5] >> ((row >> (2 * at)) & 3)) & 1
    tint = lambda c, j: (c >> (2 * j)) & 3
    goals = sum(1 << z for j, z in enumerate(beds) if emit(memory[3], tint(frame["odor"], j)))

    def imagine(board):
        occupied = sum((1 << i) for i, v in enumerate(board) if v[0])
        a = (1 << lamp) & occupied
        b = goals & occupied
        edges = [0] * 32
        for i, p in enumerate(board):
            for d in (1, 2):
                j = neighbor(i, d)
                q = board[j]
                if (
                    p[0]
                    and q[0]
                    and emit(memory[0], 4 * tint(frame["glass"], face(p, d)) + tint(frame["glass"], face(q, d ^ 2)))
                ):
                    edges[i] |= 1 << j
                    edges[j] |= 1 << i

        def spread(s):
            n = s
            while s:
                bit = s & -s
                s -= bit
                n |= edges[bit.bit_length() - 1]
            return n

        for _ in range(1 + ((g[1] >> 25) & 7)):
            a = spread(a)
            b = spread(b)
        near = diffuse(a)
        far = diffuse(near)
        score = bool(a & b) * (128 + 16 * ((g[4] >> 24) & 7))
        score += (32 * bool(near & b) + 12 * bool(far & b) + 4 * bool(diffuse(far) & b)) * ((g[2] >> 29) & 3)
        return score + (a.bit_count() + b.bit_count()) * ((g[1] >> 29) & 3)

    hand = [0, 0, 0, 0]
    identity = 32
    reserve = 16 + int(rows(text, "shoal_test")[0]["moves"]) * 12
    for step, expected in enumerate(trace):
        k = mix(frame["seed"] + step * 31469 + 1441)
        occupied = sum(1 << i for i, v in enumerate(inventory) if v[0])
        candidates = []
        for j in range(ideas):
            v = mix(k + j * 3571)
            spin = (v >> 9) & 7
            src = pick(occupied, v)
            dst = pick(MASK ^ occupied, mix(v + 317))
            kind = spin + 1 if spin < 2 else 0
            if kind:
                dst = src
            if j == 0:
                src = dst = 0
                kind = 3
            hyp = [p[:] for p in inventory]
            if not kind:
                hyp[dst], hyp[src] = hyp[src], [0, 0, 0, 0]
            elif kind < 3:
                hyp[src][0] = rotate(hyp[src][0], 1 if kind == 1 else 3)
            rank = 256 * imagine(hyp) + (v >> 25) + (8 * ((g[5] >> 25) & 15) if j == 0 else 0)
            candidates.append((rank, -(j % 32), -(j // 32), src | (dst << 5) | (kind << 10)))
        assert max(candidates)[3] == expected, (step, expected, max(candidates))
        src, dst, kind = expected & 31, (expected >> 5) & 31, (expected >> 10) & 3
        reserve -= 1
        if kind == 3:
            continue
        for j, at in enumerate((src, dst) if not kind else (src,)):
            sensor = j if not kind else kind + 1
            mask = sum(1 << m for m in range(8) if emit(memory[1 + sensor // 2], 8 * (sensor & 1) + m))
            m = pick(mask, k + 317 * j) if mask else (k + 317 * j) & 7
            op = (frame["tendons"] >> (3 * ((g[0] >> (3 * m)) & 7))) & 7
            d = (k >> 20) & 3
            other = neighbor(at, d)
            a, b = inventory[at], inventory[other]
            links = lambda ix: sum(bond(inventory[ix], inventory[neighbor(ix, side)], side) for side in range(4))
            cost = 1 + (links(at) if op in (1, 2, 3, 5, 7) else 0) + (links(other) if op in (6, 7) else 0)
            cost += ((a[1] if op in (3, 5) else b[1] if op == 6 else hand[1] if op == 4 else 0) >> 4) & 3
            if not op or reserve <= cost:
                continue
            if op in (1, 2) and a[0]:
                n = 1 if op == 1 else 3
                a[0] = rotate(a[0], n)
                a[3] = (a[3] & 1) | 2 | ((((a[3] >> 2) + n) & 3) << 2)
            elif op == 3 and a[0] and not hand[0]:
                hand, inventory[at] = a, [0] * 4
                identity, ids[at] = ids[at], 32
            elif op == 4 and not a[0] and hand[0]:
                inventory[at], hand = hand, [0] * 4
                ids[at], identity = identity, 32
            elif op == 5 and a[0] and not b[0]:
                inventory[other], inventory[at] = a, [0] * 4
                ids[other], ids[at] = ids[at], 32
            elif op == 6 and not a[0] and b[0]:
                inventory[at], inventory[other] = b, [0] * 4
                ids[at], ids[other] = ids[other], 32
            elif op == 7 and a[0] and b[0]:
                left = 2 * ((d + 1 - (a[3] & 1)) % 4)
                right = 2 * ((d + 2 - (b[3] & 1)) % 4)
                swap = (a[0] >> left ^ b[0] >> right) & 3
                new_a = a[0] ^ (swap << left)
                new_b = b[0] ^ (swap << right)
                if not swap or not new_a or not new_b:
                    continue
                a[0] = new_a
                b[0] = new_b
            else:
                continue
            reserve -= cost
            if inventory[at][0]:
                inventory[at][3] |= 2
            if op >= 5 and inventory[other][0]:
                inventory[other][3] |= 2
            if hand[0]:
                hand[3] |= 2
    assert list(map(pack, inventory)) == decode_values(text, "shoal_after")
    assert ids == decode_values(text, "shoal_origins")
    old = Counter(face(v, d) for v in opening if v[0] for d in range(4))
    assert old == Counter(face(v, d) for v in [*inventory, hand] if v[0] for d in range(4))
    harvest = expose(inventory, lamp, target, ticks)
    assert harvest == tuple(frame[k] for k in ("food", "photons", "stored"))
    assert expose(inventory, lamp, target, ticks, False)[0] == 0
    needed = sum(
        bool(v[0]) and ids[i] != i and target not in close(inventory, lamp, i) for i, v in enumerate(inventory)
    )
    assert frame["essential"] == (needed if harvest[0] else 0)


def exercise(stage):
    env = dict(os.environ, AXION_JOURNAL_DIR=str(stage / "journal"))

    def run(*args, stdin=None, good=True):
        p = subprocess.run(
            [BIN, *map(str, args)], cwd=ROOT, env=env, input=stdin, text=True, capture_output=True, timeout=150
        )
        assert (p.returncode == 0) == good, (p.returncode, p.stdout[-350:], p.stderr)
        return p.stdout

    tests = [
        "--shoal-trials",
        2,
        "--work-subjects",
        128,
        "--shoal-ideas",
        64,
        "--shoal-moves",
        8,
        "--shoal-span",
        3,
        "--shoal-exposure",
        64,
        "--steps",
        0,
    ]
    text = run("floodplain", "--load", ROOT / "examples/mineralbank.flood", *tests)
    audit_trace(text, 64, 3, 64)
    case = stage / "brood.flood"
    run(
        "floodplain",
        "--side",
        16,
        "--seed",
        1801,
        "--shoal-seasons",
        64,
        "--work-capacity",
        512,
        "--shoal-exposure",
        256,
        "--steps",
        0,
        "--save",
        case,
    )
    for sample in (0, 5, 17):
        text = run("floodplain", "--load", case, "--shoal-sample", sample, *tests)
        audit_trace(text, 64, 3, 64)
    full = stage / "full.flood"
    part = stage / "partial.flood"
    resumed = stage / "resumed.flood"
    person = stage / "resident.mind"
    args = ["floodplain", "--load", ROOT / "examples/mineralbank.flood", "--construction-tide", 7]
    full_log = run(*args, "--steps", 43, "--save", full, "--export-mind", person, "--shoal-bench", 32773)
    run(*args, "--steps", 11, "--save", part)
    run("floodplain", "--load", part, "--steps", 32, "--report-every", 5, "--save", resumed)
    assert full.read_bytes() == resumed.read_bytes()
    assert int.from_bytes(full.read_bytes()[8:12], "little") == 16
    assert int.from_bytes(person.read_bytes()[8:12], "little") & (1 << 30)
    assert int(rows(full_log, "construction_tide")[0]["sessions"]) > 0
    assert rows(full_log, "shore_deployment")[0]["mismatches"] == "0"
    neutral = stage / "neutral.flood"
    text = run(
        "floodplain",
        "--load",
        full,
        "--steps",
        0,
        "--material-legacy",
        7,
        "--hydraulic-trial",
        11,
        "--assay-growth",
        "--assay-calls",
        "--shoal-origin",
        "resident",
        "--shoal-trials",
        1,
        "--work-subjects",
        33,
        "--shoal-practice",
        0,
        "--save",
        neutral,
    )
    assert full.read_bytes() == neutral.read_bytes() and len(rows(text, "shoal_test")) == 9
    run("floodplain", "--load", full, "--extend", 128, "--neurons", 1024, "--steps", 19, "--save", resumed)
    run("floodplain", "--resident", person, "--side", 8, "--steps", 0, "--export-mind", stage / "again.mind")
    run("floodplain", "--side", 4, "--construction-tide", 7, good=False)
    run("floodplain", "--load", full, "--shoal-ideas", 33, good=False)
    # Switching on the spatial organ in a speaking animal changes its file layout.
    # Both the previously trained word circuit and the new contacts must survive.
    saved = stage / "speaker.mind"
    run(
        "talk",
        "--speaker",
        ROOT / "examples/joint-reader.mind",
        "--save-speaker",
        saved,
        stdin="/shore 115 4\n/explore-shore 64\n/quit\n",
    )
    inspected = run("agent", "--load", saved, "--inspect")
    assert "spatial_prediction_bytes=16" in inspected and "grounded_forms=" in inspected
    chamber = run(
        "talk",
        "--speaker",
        saved,
        stdin="/shore 111 4\n/imagine-shore\n/build-for moth\n/inspect-shore\n/expose-shore 48\n/quit\n",
    )
    assert rows(chamber, "shore_event")[1]["acted"] == "0" and "detached_a=0  detached_b=0" in chamber
    account = run(
        "talk",
        "--speaker",
        ROOT / "examples/shore-reader.mind",
        stdin="/shore 111 4\n/shore-requests on\nwho is vole west of?\n/expose-shore 256\nzzzz\n/quit\n",
    )
    assert "animal> moth" in account
    gestures = rows(account, "shore_event")
    assert len([e for e in gestures if e["shore_event"] == "pursue-shore"]) == 1
    measured = next(e for e in gestures if e["shore_event"] == "expose-shore")
    assert int(measured["detached_a"]) > 0 and int(measured["detached_b"]) == 0
    assert gestures[-1]["resolved"] == "0" and gestures[-1]["reserve"] == measured["reserve"]
    # The same moving bands govern the inhabited and detached portions of a mixed coast.
    mosaic = stage / "mosaic.flood"
    run("floodplain", "--load", full, "--light-regime", "mosaic", "--steps", 31, "--save", mosaic)
    run("floodplain", "--load", full, "--light-regime", "mosaic", "--steps", 17, "--save", part)
    run(
        "floodplain",
        "--load",
        part,
        "--steps",
        14,
        "--legacy",
        9,
        "--hydraulic-trial",
        16,
        "--material-legacy",
        16,
        "--save",
        resumed,
    )
    assert mosaic.read_bytes() == resumed.read_bytes()
    report = stage / "journal"
    sample = sorted(report.glob("runs/*/summary.json"))[0]
    row = json.loads(sample.read_text())
    assert row["family"] == "Construction from isolated stock" and len(row["narrative"]) == 3
    with gzip.open(sample.with_name("observations.json.gz")) as f:
        assert any(x.startswith("shoal_before=") for x in json.load(f)["lines"])
    print(
        "PASS complete candidate election, muscle execution, conservation, graph lesions, cold harvests, GPU replay images, native resident recovery and journal plates"
    )


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-shoal-") as tmp:
        exercise(Path(tmp))
