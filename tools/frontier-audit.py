#!/usr/bin/env python3
"""Read the wild population and its settlement map directly from a saved floodplain."""

import argparse
import gzip
import json
import mmap
import struct
from pathlib import Path


def survey(data, origin, start):
    if data[:8] != b"AXFLOOD\0":
        raise ValueError("expected a raw .flood habitat")
    version, tick, side, bands = struct.unpack_from("<4I", data, 8)
    base = version & 255
    if not (0 < origin < side and 0 <= start <= tick and 7 <= base <= 28):
        raise ValueError(
            "invalid survey extent, clock, or incompatible habitat version"
        )
    n = side * side
    slots = 368 + 24 * n
    bodies = slots + n * (4 + 172)
    after = bodies + n * 96
    if version & (1 << 29):
        (stored,) = struct.unpack_from("<Q", data, after)
        lengths = data[after + 8 : after + 8 + n]
        if (
            any((v & 127) > bands or v == 128 for v in lengths)
            or sum(v for v in lengths if v < 128) != stored
        ):
            raise ValueError("bad cortical extent")
        after += 8 + n + 16 * stored
    else:
        after += 16 * n * bands
    after += 96 * n
    enclosed = base == 8 if base <= 8 else bool((base - 9) & 1)
    developing = base in (7, 8) or bool((base - 9) & 2)
    if not developing:
        raise ValueError("the survey requires a saved cortical census")
    if enclosed:
        after += 104 + 177 * n
    _, control = struct.unpack_from("<II", data, after)
    cortex = after + 72
    bins = min(side, 32)
    grid = [[0] * bins for _ in range(bins)]
    regions = [
        dict(
            living=0,
            born_after=0,
            cells=0,
            largest=0,
            furthest=0,
            fibre_sites=0,
            founders=set(),
            forms=set(),
            oldest_age=0,
            max_generation=0,
        )
        for _ in range(2)
    ]

    def distance(u):
        return min(u - origin + 1, side - u) if u >= origin else 0

    for pos in range(n):
        x, y = pos % side, pos // side
        region = regions[int(x >= origin or y >= origin)]
        region["fibre_sites"] += any(
            struct.unpack_from("<2H", data, 368 + 24 * pos + 16)
        )
        (slot,) = struct.unpack_from("<I", data, slots + 4 * pos)
        if slot >= n:
            raise ValueError("invalid animal address")
        body = bodies + slot * 96
        founder, born, generation, energy = struct.unpack_from("<3IH", data, body + 16)
        if not energy:
            continue
        (cohort,) = struct.unpack_from("<H", data, body + 78)
        active, natal = struct.unpack_from("<2B", data, cortex + 12 * slot + 8)
        cells = 32 * (
            min(active, natal) if cohort and cohort == control >> 2 else active
        )
        grid[y * bins // side][x * bins // side] += 1
        region["living"] += 1
        region["born_after"] += born > start
        region["cells"] += cells
        region["largest"] = max(region["largest"], cells)
        region["furthest"] = max(region["furthest"], distance(x) + distance(y))
        region["founders"].add(founder)
        region["forms"].add(data[body : body + 16])
        region["oldest_age"] = max(region["oldest_age"], tick - born)
        region["max_generation"] = max(region["max_generation"], generation)
    shared_forms = len(regions[0]["forms"] & regions[1]["forms"])
    for region in regions:
        for name in ("forms", "founders"):
            region[name] = len(region[name])
    return dict(
        tick=tick,
        side=side,
        origin=origin,
        start=start,
        shared_forms=shared_forms,
        core=regions[0],
        outside=regions[1],
    ), grid


def verify(stats, grid, receipt):
    with gzip.open(receipt, "rt") as f:
        lines = json.load(f)["lines"]

    def rows(key):
        return [
            dict(v.split("=", 1) for v in line.split() if "=" in v)
            for line in lines
            if line.startswith(key + "=")
        ]

    final = rows("frontier_census")[-1]
    if final["frontier_census"] != "end":
        raise ValueError("missing terminal frontier census")
    values = dict(
        tick=stats["tick"],
        side=stats["side"],
        origin=stats["origin"],
        start=stats["start"],
        core_residents=stats["core"]["living"],
        outside=stats["outside"]["living"],
    )
    for k in ("founders", "born_after", "furthest", "largest", "fibre_sites"):
        values[k] = stats["outside"][k]
    values["expressed_cells"] = stats["outside"]["cells"]
    if any(int(final[k]) != v for k, v in values.items()):
        raise ValueError("checkpoint and field census disagree")
    recorded = sorted(rows("frontier_row"), key=lambda r: int(r["frontier_row"]))
    if [list(map(int, r["residents"].split(","))) for r in recorded] != grid:
        raise ValueError("checkpoint and recorded density map disagree")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("habitat", type=Path)
    p.add_argument("--origin", type=int, required=True)
    p.add_argument("--opened", type=int, required=True)
    p.add_argument("--receipt", type=Path)
    args = p.parse_args()
    with (
        args.habitat.open("rb") as f,
        mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as data,
    ):
        stats, grid = survey(data, args.origin, args.opened)
    if args.receipt:
        verify(stats, grid, args.receipt)
    print(json.dumps(stats, indent=2))
