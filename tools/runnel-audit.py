#!/usr/bin/env python3
"""Reconstruct the passive flaps, the pump and the reference relation from observable results."""

import argparse
import csv
import gzip
import json
import re
from collections import Counter, defaultdict
from functools import lru_cache
from pathlib import Path

MASK = 0xFFFFFFFF


def mix(x):
    x &= MASK
    x = ((x ^ (x >> 16)) * 0x21F0AAAD) & MASK
    x = ((x ^ (x >> 15)) * 0x735A2D97) & MASK
    return x ^ (x >> 15)


def bend(stone, cue, gesture):
    left = (stone >> (2 * (cue % 4))) & 3
    right = (stone >> (8 + 2 * (cue // 4))) & 3
    if stone >> 31 and (stone >> (16 + (cue % 4))) & 1:
        gesture = -gesture
    return (1, 0, 2, 3)[(gesture + left - right) % 4]


def wrist(motor, frame):
    return ((frame % 4) + (-motor if frame & 4 else motor)) % 4


def reflect(mask, flip):
    return (mask >> 1 | (mask & 1) * 2) if flip else mask


def pump(gates, ticks, moving=True, state=None):
    n = len(gates)
    if state is None:
        water = [4096] + [5] * (n - 1) + [0]
        carbon = [4096] + [0] * n
        first = 0
    else:
        water, carbon, first = state
    for t in range(first, first + ticks):

        def head(i, tick=t):
            if i == 0:
                return 5
            if i == n:
                return 17
            return water[i] + 24 * (moving and ((tick // 4 + i) % 2))

        for parity in (t % 2, 1 - t % 2):
            for i in range(parity, n, 2):
                delta = head(i) - head(i + 1)
                rev = int(delta < 0)
                if not (gates[i] & (1 << rev)):
                    continue
                a, b = i + rev, i + 1 - rev
                volume = min(3, abs(delta) // 2, water[a], 65535 - water[b])
                carried = min(volume, carbon[a], 65535 - carbon[b])
                water[a] -= volume
                water[b] += volume
                carbon[a] -= carried
                carbon[b] += carried
    return water, carbon, first + ticks


def lines(path):
    if path.name.endswith("json.gz"):
        with gzip.open(path, "rt") as f:
            yield from json.load(f)["lines"]
    else:
        yield from path.read_text().splitlines()


def log(path):
    count = Counter()
    for line in lines(path):
        q = dict(re.findall(r"(\w+)=([^ ]+)", line))
        if "reed_rig" in q:
            n, t = int(q["span"]), int(q["ticks"])
            mode = q["reed_rig"]
            valves = [{"unfolded": 3, "reversed": 2}.get(mode, 1)] * n
            if mode == "cut":
                valves[n // 2] = 0
            water, carbon, _ = pump(valves, t, mode != "quiet")
            assert int(q["water"]) == water[-1] and int(q["carbon"]) == carbon[-1]
            assert int(q["water_total"]) == sum(water) and int(
                q["carbon_total"]
            ) == sum(carbon)
            count["controlled strips"] += 1
        if "reed_example" in q:
            packed = bytes.fromhex(q["joints"])
            n = int(q["span"])
            assert len(packed) == n
            valves = [
                reflect(
                    bend(
                        int(q["stone"])
                        if int(q.get("handed", 0))
                        else int(q["stone"]) & 65535,
                        x & 15,
                        x >> 5,
                    ),
                    (x >> 4) & 1,
                )
                for x in packed
            ]
            water, carbon, _ = pump(valves, 24 * n + 160)
            assert int(q["water"]) == water[-1] and int(q["carbon"]) == carbon[-1]
            count["pupil-built galleries"] += 1
    return count


def check_corner(stone, target, pivot, frame, transform):
    other_l, other_r = pivot % 4, pivot // 4
    a = target % 4 + 4 * other_r
    last = other_l + 4 * (target // 4)
    for m in range(4):
        rel = ((-m if transform & 4 else m) + transform % 4) % 4
        if bend(stone, a, wrist(m, frame)) != bend(stone, pivot, wrist(rel, frame)):
            return False
        if bend(stone, target, wrist(m, frame)) != bend(stone, last, wrist(rel, frame)):
            return False
    return True


def traces(path):
    tally = defaultdict(Counter)
    with gzip.open(path, "rt") if path.suffix == ".gz" else path.open() as stream:
        for t in csv.DictReader(stream, delimiter="\t"):
            arm = t.pop("arm")
            q = {k: int(v) for k, v in t.items()}
            a, b = q["stone_a"], q["stone_b"]
            c, d, seed, idx = q["target"], q["direction"], q["seed"], q["case"]
            assert c % 4 != q["corner"] % 4 and c // 4 != q["corner"] // 4
            assert a ^ b == 2 << (2 * (c % 4))
            assert not any(bend(a, c, j) == bend(b, c, j) == 1 << d for j in range(4))
            assert q["target_contacts"] == (8 if arm == "target_shown" else 0)
            record = tally[arm]
            record["pairs"] += 1
            exact = 0
            proof = 0
            extra = (
                mix(mix(seed + idx * 5503) + 6743) % 4
                if arm == "independent_grit"
                else 0
            )
            for s, name in ((a, "a"), (b, "b")):
                g = q["pose_" + name]
                assert g == wrist(q["motor_" + name], q["frame"])
                result = bend(s, c, (g + extra) % 4)
                assert result == q["effect_" + name]
                exact += result == (1 << d)
                via = q["via_" + name]
                if via < 16:
                    record["evaluated_relations"] += 1
                    proof += via == q["corner"] and check_corner(
                        s, c, via, q["frame"], q["transform_" + name]
                    )
            assert exact == q["exact"] and (exact == 2) == q["both"]
            record["both"] += exact == 2
            record["verified_paired_relations"] += exact == proof == 2
            record["target_contacts"] += q["target_contacts"]
            # Eight possible physical masks over the paired presentations. The
            # two-chamber oracle also checks partial success through a free flap.
            expected = [coupon(reflect(q["effect_" + name], d)) for name in ("a", "b")]
            assert q["water"] == sum(v[0] for v in expected)
            assert q["carbon"] == sum(v[1] for v in expected)
    return tally


@lru_cache(maxsize=4)
def coupon(gate):
    w, c, _ = pump([1, gate], 48)
    return w[-1], c[-1]


def speech_audit(path):
    counts = defaultdict(Counter)
    twins = {}
    with gzip.open(path, "rt") if path.suffix == ".gz" else path.open() as f:
        vocab = []
        for i in range(2):
            line = f.readline().strip().split("\t")
            assert line[0] == f"# vocabulary {i}"
            vocab.append((line[1].split(), line[2].split()))
            assert len(vocab[i][0]) == 8 and len(vocab[i][1]) == 7

        def decode(text, role, op):
            items = text.split()
            nouns, verbs = vocab[role]
            if len(items) < 4 or items[0] != verbs[op]:
                return None
            try:
                a, b, c = [nouns.index(x) for x in items[1:4]]
            except ValueError:
                return None
            if a >= 4 or b >= 4 or c < 4:
                return None
            return a + 4 * b, c - 4

        for row in csv.DictReader(f, delimiter="\t"):
            arm = row["arm"]
            t = counts[arm]
            q = {
                k: int(v)
                for k, v in row.items()
                if k
                not in (
                    "arm",
                    "order",
                    "first",
                    "query",
                    "report",
                    "action",
                    "why",
                    "later_action",
                )
            }
            stone = q["geology"]
            d = q["direction"]
            target = q["target"]
            later = q["later"]
            pivot = q["corner"]
            ref = q["reference"]
            assert (
                target // 4 == later // 4 == ref // 4
                and len({target % 4, later % 4, ref % 4}) == 3
            )
            assert ref % 4 == pivot % 4 and pivot // 4 != target // 4
            assert decode(row["order"], 0, 1) == (target, d)
            if q["reported"]:
                rq = decode(row["query"], 1, 2)
                pa = decode(row["report"], 1, 6)
                assert rq and pa and rq[0] == pa[0] and rq[1] == d
                words = row["report"].split()
                assert len(words) == 6 and words[4] == vocab[1][1][5]
                assert vocab[1][0].index(words[5]) - 4 == 1 << d
                t["about_another_joint"] += rq[0] != target
                t["reference_reports"] += rq[0] == ref
                if q["learned"]:
                    assert decode(row["report"], 0, 6) == pa
            for other, action, applied, gesture, good, food in (
                (target, "action", "paid", "pose", "success", "carbon"),
                (
                    later,
                    "later_action",
                    "later_paid",
                    "later_pose",
                    "later_success",
                    "later_carbon",
                ),
            ):
                mask = 3
                if q[applied]:
                    v = decode(row[action], 1, 6)
                    assert v == (other, q[gesture]), (row, action)
                    mask = bend(stone, other, v[1])
                else:
                    assert q[gesture] == 4
                assert bool(q[good]) == bool(q[applied] and mask == 1 << d)
                assert q[food] == coupon(reflect(mask, d))[1]
            reason = row["why"].split()
            if q["via"] < 16:
                assert len(reason) == 7 and reason[4] == vocab[0][1][5]
                assert reason[5:] == [
                    vocab[0][0][q["via"] % 4],
                    vocab[0][0][q["via"] // 4],
                ]
                valid = check_corner(
                    stone, target, q["via"], q["frame"], q["transform"]
                )
                t["correct_explanations"] += valid
            if q["later_via"] < 16:
                t["verified_later_relations"] += check_corner(
                    stone, later, q["later_via"], q["frame"], q["later_transform"]
                )
            t["trials"] += 1
            t["exact"] += q["success"]
            t["later_correct"] += q["later_success"]
            t["paid"] += q["paid"] + q["later_paid"]
            t["use_on_two_joints"] += (
                q["success"] and q["later_success"] and q["pose"] != q["later_pose"]
            )
            pair = (arm, q["pair"])
            if q["world"] == 0:
                assert pair not in twins
                twins[pair] = (q, row)
            else:
                a, prior = twins.pop(pair)
                assert (a["target"], a["later"], a["corner"]) == (target, later, pivot)
                assert prior["first"] == row["first"] and prior["order"] == row["order"]
                # Audit every movement visible to the first speaker before asking.
                for left in (target % 4, later % 4, pivot % 4):
                    reference = left + 4 * (pivot // 4)
                    assert all(
                        bend(stone, reference, j) == bend(a["geology"], reference, j)
                        for j in range(4)
                    )
                assert not any(
                    bend(stone, target, j) == bend(a["geology"], target, j) == 1 << d
                    for j in range(4)
                )
                t["pairs"] += 1
                if a["success"] and q["success"]:
                    assert prior["action"] != row["action"] and a["pose"] != q["pose"]
                    t["both"] += 1
                    t["distinct_reports"] += prior["report"] != row["report"]
                t["both_later"] += a["later_success"] and q["later_success"]
    assert not twins
    return counts


def cut_audit(before, tagged, matched, seed, phase):
    import mmap
    import struct
    from contextlib import ExitStack

    with ExitStack() as files:

        def region(path):
            f = files.enter_context(path.open("rb"))
            return files.enter_context(
                mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
            )

        a = region(before)
        b = region(tagged)
        c = region(matched)
        version, tick, side, tiles = struct.unpack_from("<4I", a, 8)
        n = side * side
        base = version & 255
        assert version & 16777216 and 9 <= base <= 28 and len(a) == len(b) == len(c)
        key = struct.pack("<3I", seed, phase, 2)
        pos = a.find(key) - 4
        assert pos > 0 and a.find(key, pos + 5) < 0
        state = pos + struct.calcsize("<4I22Q")
        start = state + 48 * n
        end = start + n
        for dest in b, c:
            for offset, last in ((0, start), (end, len(a) - 8)):
                for j in range(offset, last, 1 << 20):
                    assert (
                        a[j : min(j + (1 << 20), last)]
                        == dest[j : min(j + (1 << 20), last)]
                    )
        # Independently step through the preceding layers to locate the
        # physical grooves and the actual water/carbon of each matched pair.
        at = 368 + n * (24 + 4 + 172 + 96 + 16 * tiles + 96)
        if (base - 9) & 1:
            at += 104 + 177 * n
        if (base - 9) & 2:
            at += 72 + 12 * n
        minerals = at + 160
        at = minerals + 76 * n
        if base >= 13:
            at += 56 + 16 * n
        if base >= 17:
            at += 104 + 32 * n
        if base >= 21:
            at += 64 + 2 * n
        if base >= 25:
            at += 48 + 88 * n
        if version & 512:
            at += 104 + 57 * n
        if version & 1024:
            at += 40 + 8 * n
        if version & 4096:
            words = (
                31
                if version & 65536
                else 27
                if version & 32768
                else 25
                if version & 16384
                else 13
                if version & 8192
                else 11
            )
            at += 8 + 8 * words + 72 * n
        assert version & 131072
        wells = at + struct.calcsize("<2I18Q")
        # Read the pocket offset from the documented fixed watershed layout.
        # A range check against the following capacity guards schema revisions.
        capacity = struct.unpack_from("<I", a, at)[0]
        assert 1 <= capacity <= 32
        mode, world_seed, _, _ = struct.unpack_from("<4I", a, pos)
        stone = (mix(world_seed + 9421) & 0xFFFFF) | (0x80000000 if mode & 8192 else 0)
        changed = [Counter(), Counter()]
        population = Counter()
        width = min(8, side)
        for i in range(n):
            for axis in range(2):
                shift = 4 * axis
                initial = (a[start + i] >> shift) & 15
                if not (initial & 7):
                    continue
                j = (i + side) % n if axis else i // side * side + (i % side + 1) % side
                face_a = a[minerals + 4 * i]
                face_b = a[minerals + 4 * j]
                if not face_a or not face_b:
                    assert all(
                        ((dest[start + i] >> shift) & 15) == initial for dest in (b, c)
                    )
                    continue
                cue = ((face_a >> (2 * axis)) & 3) | (
                    ((face_b >> (2 * (axis + 2))) & 3) << 2
                )
                opened = bend(stone, cue, (initial & 7) - 1)
                wet_a = struct.unpack_from("<H", a, 368 + 24 * i)[0]
                wet_b = struct.unpack_from("<H", a, 368 + 24 * j)[0]
                source = bool(
                    struct.unpack_from("<H", a, wells + 4 * i)[0]
                    | struct.unpack_from("<H", a, wells + 4 * j)[0]
                )
                cell = (
                    i // side // width,
                    i % side // width,
                    opened,
                    axis,
                    wet_a > wet_b,
                    source,
                )
                population[cell, bool(initial & 8)] += 1
                for arm, dest in enumerate((b, c)):
                    after = (dest[start + i] >> shift) & 15
                    if after != initial:
                        assert after == 0 and bool(initial & 8) == (arm == 0)
                        changed[arm][cell] += 1
        assert changed[0] == changed[1]
        strata = {k for k, _ in population}
        assert sum(changed[0].values()) == sum(
            min(population[k, True], population[k, False]) for k in strata
        )
        return {
            "tick": tick,
            "sites": n,
            "matched_pairs": sum(changed[0].values()),
            "occupied_strata": len(changed[0]),
            "private_bytes": 48 * n,
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=["log", "trace", "speech", "cut"])
    parser.add_argument("files", nargs="+", type=Path)
    parser.add_argument("--seed", type=int)
    parser.add_argument("--period", type=int, default=8)
    args = parser.parse_args()
    if args.kind == "cut":
        if len(args.files) != 3 or args.seed is None:
            parser.error("cut takes ORIGINAL OBSERVED MATCHED and --seed GEOLOGY")
        print(
            json.dumps(cut_audit(*args.files, args.seed, args.period), sort_keys=True)
        )
        return
    for file in args.files:
        result = {"trace": traces, "log": log, "speech": speech_audit}[args.kind](file)
        print(file.name, json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
