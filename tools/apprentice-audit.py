#!/usr/bin/env python3
"""Decode public replies and the finite circuit of a material-reared pupil."""

import argparse
import csv
import gzip
import json
from collections import Counter, defaultdict
from pathlib import Path


def mix(value):
    mask = (1 << 32) - 1
    value &= mask
    value = ((value ^ (value >> 16)) * 0x21F0AAAD) & mask
    value = ((value ^ (value >> 15)) * 0x735A2D97) & mask
    return value ^ (value >> 15)


def classroom(path):
    opener = gzip.open if path.suffix == ".gz" else open
    totals = defaultdict(Counter)
    conditions = ("unrestricted", "inherited", "quiet", "displaced", "yoked")
    first = []
    with opener(path, "rt", newline="") as file:
        for row in csv.DictReader(file, delimiter="\t"):
            arm = row.pop("arm")
            if arm not in conditions:
                raise ValueError(f"unknown intervention {arm}")
            r = {k: int(v) for k, v in row.items()}
            t = totals[arm]
            if r["case"] != t["courses"]:
                raise ValueError("missing or repeated pupil")
            identity = tuple(
                r[k] for k in ("language", "images", "known", "first", "second")
            )
            if arm == "unrestricted":
                first.append(identity)
            elif identity != first[r["case"]]:
                raise ValueError(
                    "matched pupils saw different physical or vocal histories"
                )
            known = r["known"] & 255
            mask = r["answer_mask"]
            if mask & ~known:
                raise ValueError("answer from an unavailable field socket")
            truth = [(r["images"] >> (2 * j) & 3) + 4 for j in range(8)]
            hit = [
                bool(mask & 1 << j) and (r["replies"] >> (3 * j) & 7) == truth[j]
                for j in range(8)
            ]
            moved = [
                j
                for j in range(4)
                if (known & (17 << j)) == (17 << j) and truth[j] != truth[4 + j]
            ]
            spin = mix(r["language"] ^ 0x55D2337)
            diagnostic = [
                4 + ((spin + j % 4 + (1 + 2 * ((spin >> 7) & 1) if j >= 4 else 0)) & 3)
                for j in range(8)
            ]
            quiz = sum(
                bool(r["probe_mask"] & 1 << j)
                and (r["probe_replies"] >> (3 * j) & 7) == diagnostic[j]
                for j in range(8)
            )
            observed = (
                known.bit_count(),
                mask.bit_count(),
                sum(hit),
                len(moved),
                sum(hit[j] and hit[j + 4] for j in moved),
                quiz,
                0,
            )
            if observed != tuple(
                r[k]
                for k in (
                    "offered",
                    "answered",
                    "correct",
                    "changed",
                    "both",
                    "diagnostic",
                    "erased",
                )
            ):
                raise ValueError(
                    f"inconsistent retinal or acoustic record for {arm}, case {r['case']}"
                )
            t.update(
                courses=1,
                early=r["before"] == 8,
                late=quiz == 8,
                twice=r["before"] == 8 and quiz == 8,
                offered=r["offered"],
                correct=r["correct"],
                released=r["released"],
                both=r["both"],
            )
            if "board0" in r:
                for f in range(4):
                    for j in range(4):
                        physical = (r[f"board{f}"] >> (2 * j)) & 3
                        heard = bool(r[f"mask{f}"] & (1 << j))
                        right = (
                            heard and ((r[f"reply{f}"] >> (3 * j)) & 7) == 4 + physical
                        )
                        differs = physical != ((r["board1"] >> (2 * j)) & 3)
                        t[f"frame{f}"] += right
                        t[f"different{f}"] += differs
                        t[f"different_right{f}"] += right and differs
    if (
        set(totals) != set(conditions)
        or len({t["courses"] for t in totals.values()}) != 1
    ):
        raise ValueError("missing matched conditions")
    return dict(totals)


def circuit(words, old, conflict, attempted, agreed):
    """Exact truth table when the circuit uses no recurrent or background input."""
    first, second, law = words
    proposal = (law >> (2 * old + conflict)) & 1
    ports = [old, conflict, proposal, None, attempted, agreed, None, None]
    for index, op in enumerate((first, second)):
        inputs = [ports[(op >> k) & 7] for k in (0, 3, 6)]
        if None in inputs:
            raise ValueError(
                "this donor also needs a somatic state for its truth table"
            )
        value = (op >> (9 + sum(int(v) << j for j, v in enumerate(inputs)))) & 1
        if index == 0:
            ports[7] = value
    return proposal if value else old


def donor_words(path):
    fields = path.read_text().split()
    if len(fields) != 9 or fields[0] != "AXION-REGULATOR-1":
        raise ValueError("unrecognized regulatory donor")
    return tuple(map(int, fields[1:4]))


def food_return(words, seed):
    totals = Counter()
    for life in range(32):
        climate = mix(mix(seed + 0x71B59245) + life * 0x7F4A7C15)
        rank = mix(climate + 931) % 24
        unused = list(range(4))
        route = []
        while unused:
            rank, pick = divmod(rank, len(unused))
            route.append(unused.pop(pick))
        turn = 1 + mix(climate + 1013) % 3
        pruned = [[0] * 4 for _ in range(4)]
        for step in range(96):
            rain = mix(climate + step * 0x9E3779B9)
            source = (rain >> 10) & 3
            phase = step // 32
            outlet = (route[source] + (turn if phase == 1 else 0)) % 4
            row = pruned[source]
            ports = [k for k, closed in enumerate(row) if not closed]
            committed = len(ports) == 1
            choices = ports or list(range(4))
            reached = choices[mix(rain ^ 0xA91D2053) % len(choices)]
            hit = reached == outlet
            totals["swallowed" if hit else "escaped"] += 1
            totals["shift"] += int(hit and phase == 1)
            totals["return"] += int(hit and phase == 2)
            totals["committed"] += int(committed)
            totals["matches"] += int(committed and hit)
            for k in range(4):
                contradictory = (k != reached) if hit else (k == reached)
                row[k] = circuit(
                    words,
                    row[k],
                    int(contradictory),
                    int(committed),
                    int(committed and hit),
                )
    return totals


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    options = parser.add_subparsers(dest="command", required=True)
    options.add_parser("classroom").add_argument("path", type=Path)
    p = options.add_parser("donor")
    p.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.command == "classroom":
        out = classroom(args.path)
    else:
        words = donor_words(args.path)
        out = {
            "next_contact": {
                k: [
                    circuit(words, old, conflict, tried, agreed)
                    for old in (0, 1)
                    for conflict in (0, 1)
                ]
                for k, tried, agreed in (
                    ("silent", 0, 0),
                    ("wrong", 1, 0),
                    ("confirmed", 1, 1),
                )
            },
            "heldout_food": food_return(words, int(args.path.read_text().split()[4])),
        }
    print(json.dumps(out, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
