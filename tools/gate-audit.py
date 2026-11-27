#!/usr/bin/env python3
"""Pair the same starting animals across three rotations of a wild cortical intervention."""

import argparse
import csv
import gzip
import json
import mmap
import struct
from collections import Counter, defaultdict
from pathlib import Path


ARMS = ("intact", "reverted", "rotated")


def read_crossovers(paths):
    paired = {}
    for phase, path in enumerate(paths):
        opener = gzip.open if path.suffix == ".gz" else open
        seen = set()
        with opener(path, "rt", newline="") as stream:
            for row in csv.DictReader(stream, delimiter="\t"):
                key = (int(row["slot"]), int(row["born"]))
                if key in seen:
                    raise ValueError(f"duplicate creature in {path}")
                seen.add(key)
                arm = ARMS.index(row["arm"])
                acquired, changed, opening, energy = (
                    int(row[k])
                    for k in (
                        "acquired",
                        "changed",
                        "opening_energy",
                        "retained_energy",
                    )
                )
                action = int(row["last_action"])
                if not (
                    0 < acquired <= 2048
                    and 0 < opening <= 65535
                    and 0 <= energy <= 65535
                ):
                    raise ValueError(f"invalid gate or energy count for {key}")
                if (
                    (energy > 0) != (action >= 0)
                    or changed < 0
                    or changed > 2048
                    or (arm == 0 and changed)
                ):
                    raise ValueError(f"invalid cortical intervention for {key}")
                if phase == 0:
                    paired[key] = (acquired, opening, arm, [None] * 3)
                if key not in paired:
                    raise ValueError("enrollment changed between rotations")
                original, stock, first, results = paired[key]
                if (acquired, opening, arm) != (original, stock, (first + phase) % 3):
                    raise ValueError(f"incompatible crossover baseline for {key}")
                results[arm] = (energy, action, changed)
        if seen != paired.keys():
            raise ValueError("a rotation omitted original animals")

    return paired


def summarize(records):
    enrolled = 0
    totals = {
        mode: dict(surviving=0, retained_energy=0, changed_bits=0) for mode in ARMS
    }
    differences = {
        mode: dict(
            both_survived=0,
            only_intact=0,
            only_other=0,
            neither=0,
            different_last_action=0,
        )
        for mode in ARMS[1:]
    }
    for _, _, _, outcomes in records:
        enrolled += 1
        for name, (energy, action, changed) in zip(ARMS, outcomes):
            record = totals[name]
            record["surviving"] += energy > 0
            record["retained_energy"] += energy
            record["changed_bits"] += changed
            if name != ARMS[0]:
                native, original_action, _ = outcomes[0]
                bucket = (("neither", "only_other"), ("only_intact", "both_survived"))[
                    native > 0
                ][energy > 0]
                diff = differences[name]
                diff[bucket] += 1
                diff["different_last_action"] += bool(
                    energy and native and action != original_action
                )
    return dict(subjects=enrolled, outcomes=totals, paired=differences)


def audit(paths):
    return summarize(read_crossovers(paths).values())


def habitat_groups(paired, path, origin):
    """Group by the occupied starting position and the common anatomical plans of all living residents."""
    with path.open("rb") as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as raw:
        version, tick, side, _ = struct.unpack_from("<4I", raw, 8)
        if raw[:8] != b"AXFLOOD\0" or not (
            7 <= (version & 255) <= 28 and 0 < origin < side
        ):
            raise ValueError("incompatible source habitat or origin")
        n = side * side
        slots = 368 + 24 * n
        bodies = slots + (4 + 172) * n
        plans = Counter()
        sectors = bytearray(n)
        for pos in range(n):
            (slot,) = struct.unpack_from("<I", raw, slots + pos * 4)
            if slot >= n:
                raise ValueError("invalid stored slot")
            record = bodies + slot * 96
            if struct.unpack_from("<H", raw, record + 28)[0]:
                plans[struct.unpack_from("<4I", raw, record)] += 1
            sectors[slot] = pos % side >= origin or pos // side >= origin
        top = sorted(plans, key=lambda p: (-plans[p], p))[:8]
        labels = {plan: f"form-{i + 1}" for i, plan in enumerate(top)}
        groups = defaultdict(list)
        for (slot, born), trial in paired.items():
            record = bodies + slot * 96
            if (
                slot >= n
                or struct.unpack_from("<I", raw, record + 20)[0] != born
                or struct.unpack_from("<H", raw, record + 28)[0] != trial[1]
            ):
                raise ValueError("subject and starting habitat differ")
            groups[("core", "outer")[sectors[slot]]].append(trial)
            groups[
                labels.get(struct.unpack_from("<4I", raw, record), "other-forms")
            ].append(trial)
        output = {name: summarize(records) for name, records in groups.items()}
        for plan, name in labels.items():
            item = output.setdefault(name, summarize(()))
            item.update(anatomy=list(plan), source_population=plans[plan])
    return dict(start=tick, origin=origin, groups=output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phases", type=Path, nargs=3, metavar="TSV")
    parser.add_argument(
        "--habitat",
        type=Path,
        help="stratify by the eight most populous starting body plans and original shore",
    )
    parser.add_argument("--origin", type=int, help="side length of the original shore")
    args = parser.parse_args()
    if (args.habitat is None) != (args.origin is None):
        parser.error("--habitat and --origin must be supplied together")
    paired = read_crossovers(args.phases)
    result = summarize(paired.values())
    if args.habitat:
        result.update(habitat_groups(paired, args.habitat, args.origin))
    print(json.dumps(result, indent=2))
