#!/usr/bin/env python3
"""Reconstruct a conservative four-seat transport step from reported terrain and current."""

import argparse
import csv
import gzip
from pathlib import Path

EDGES = ((0, 1), (0, 2), (1, 3), (2, 3))


def values(word):
    return [(word >> (16 * j)) & 65535 for j in range(4)]


def partition(food, water, sill, seam, current, tie):
    available = {}
    for edge, (source, target) in enumerate(EDGES):
        if not (current & (1 << source)) or not food[source] or not water[source]:
            continue
        descent = (
            water[source] - water[target] + 2 + min(seam[edge], 7) - min(sill[edge], 31)
        )
        if descent <= 0:
            continue
        available.setdefault(source, []).append((descent, edge))
    proposed = []
    for source, offers in available.items():
        candidates = [
            edge for head, edge in offers if head == max(v[0] for v in offers)
        ]
        proposed.append(candidates[-1] if tie & 1 else candidates[0])
    accepted = []
    for sink in (1, 2, 3):
        arrivals = [e for e in proposed if EDGES[e][1] == sink]
        if tie & 2:
            arrivals.reverse()
        accepted.extend(arrivals[: 65535 - food[sink]])
    final = food.copy()
    for edge in accepted:
        source, sink = EDGES[edge]
        final[source] -= 1
        final[sink] += 1
    return sum(1 << e for e in proposed), sum(1 << e for e in accepted), final


def audit_rows(rows):
    counts = dict(
        rows=0,
        uniform=0,
        targeted=0,
        moves=0,
        tests=0,
        enabled_edges=0,
        prevented_edges=0,
    )
    for row in rows:
        side = row["side"]
        x = (2 * (row["tile"] % (side // 2)) + (row["epoch"] & 1)) % side
        y = (2 * (row["tile"] // (side // 2)) + ((row["epoch"] >> 1) & 1)) % side
        assert (x, y) == (row["x"], row["y"])
        if row["mode"] == 1:
            assert row["local"] == row["gates"]
        four = [values(row[k]) for k in ("food", "water", "sill", "seam")]
        offered, moved, final = partition(*four, row["gates"], row["priority"])
        assert (offered, moved, final) == (
            row["proposed"],
            row["moved"],
            values(row["final"]),
        ), row
        if row.get("surface"):
            pigments = [(row["paint_before"] >> (2 * j)) & 3 for j in range(4)]
            original = sorted(pigments)
            downstream = [3, 2] if row["priority"] & 2 else [2, 3]
            for edge in [*downstream, 0, 1]:
                if not moved & (1 << edge):
                    continue
                a, b = EDGES[edge]
                pigments[a], pigments[b] = pigments[b], pigments[a]
            assert original == sorted(pigments)
            assert row["paint_after"] == sum(
                v << (2 * j) for j, v in enumerate(pigments)
            )
        assert sum(final) == sum(four[0]) and min(final) >= 0 and max(final) <= 65535
        before, after = 0, 0
        for j in range(3):
            delta = (1 << j) & row["changed"]
            _, alternative, _ = partition(*four, row["gates"] ^ delta, row["priority"])
            assert alternative == row["alt" + str(j)]
            before += (alternative & ~moved).bit_count()
            after += (moved & ~alternative).bit_count()
            counts["tests"] += bool(delta)
        if row["targeted"]:
            assert before + after > 0
        counts["rows"] += 1
        counts["targeted" if row["targeted"] else "uniform"] += 1
        counts["moves"] += moved.bit_count()
        counts["enabled_edges"] += after
        counts["prevented_edges"] += before
    return counts


def audit(path):
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt") as f:
        return audit_rows(
            {k: int(v) for k, v in row.items()}
            for row in csv.DictReader(f, delimiter="\t")
        )


def bounds(path):
    opener = gzip.open if path.suffix == ".gz" else open
    levels = (0, 1, 65534, 65535)
    water = (
        (2, 2, 2, 2),
        (0, 4, 0, 9),
        (65535, 0, 65535, 1),
        (3, 3, 8, 1),
        (4, 3, 2, 1),
        (80, 35, 32, 63),
        (1, 4, 1, 6),
        (65532, 65534, 65535, 65533),
    )
    seen = moved = blocked = 0
    with opener(path, "rt") as f:
        for row in csv.DictReader(f, delimiter="\t"):
            i = int(row["case"])
            assert i == seen
            shape = i >> 13
            food = [levels[(i >> (2 * j)) & 3] for j in range(4)]
            sill = [
                0
                if not shape
                else 65535
                if (shape + j) % 5 == 0
                else (shape * 5 + j * 7) % 16
                for j in range(4)
            ]
            seam = [
                0
                if not shape
                else 65535
                if (shape + j) % 4 == 0
                else (shape * 9 + j * 11) % 8
                for j in range(4)
            ]
            offers, edges, after = partition(
                food, water[shape], sill, seam, (i >> 8) & 7, (i >> 11) & 3
            )
            assert (offers, edges, after) == (
                int(row["proposed"]),
                int(row["moved"]),
                values(int(row["final"])),
            )
            assert sum(after) == sum(food)
            seen += 1
            moved += edges.bit_count()
            blocked += (offers & ~edges).bit_count()
    assert seen == 65536 and blocked > 0
    return dict(cases=seen, moved=moved, blocked=blocked)


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("receipt", type=Path)
    p.add_argument("--boundaries", action="store_true")
    args = p.parse_args()
    print((bounds if args.boundaries else audit)(args.receipt))
