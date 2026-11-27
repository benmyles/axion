#!/usr/bin/env python3
"""Inspect private experience and verify each surviving deliberate splice against the actual fibre in a checkpoint."""

import argparse
import csv
import mmap
import struct
import sys
from collections import defaultdict
from pathlib import Path

STATE = struct.Struct("<4I26Q")


def inspect(path, cap, seed):
    hist = defaultdict(lambda: [0] * 8)
    marks = []
    with (
        path.open("rb") as file,
        mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as raw,
    ):
        version, tick, side, _ = struct.unpack_from("<4I", raw, 8)
        n = side**2
        if (
            raw[:8] != b"AXFLOOD\0"
            or not version & (1 << 21)
            or not 9 <= version & ~33554176 <= 28
        ):
            raise ValueError("expected a full watershed with practice histories")
        signature = struct.pack("<3I", cap, seed, 1)
        pos = 0
        hits = []
        while (p := raw.find(signature, pos)) >= 0:
            pos = p + len(signature)
            if p < 4 or p + STATE.size >= len(raw):
                continue
            meta = STATE.unpack_from(raw, p - 4)
            b = meta[4:]
            if (
                meta[0] < 131072
                and b[22] <= b[21] <= b[5]
                and b[16] <= b[15] <= b[14]
                and p - 4 + STATE.size + n * (32 + 20 * cap) <= len(raw)
            ):
                hits.append(p - 4)
        if len(hits) != 1:
            raise ValueError("the arena and seed must identify one practice section")
        base = hits[0] + STATE.size
        body = 368 + n * 200
        for i in range(n):
            born, _, energy = struct.unpack_from("<IIH", raw, body + i * 96 + 20)
            if not energy:
                continue
            _, inference, _, _, banks, focus, _, samples = struct.unpack_from(
                "<8I", raw, base + 32 * i
            )
            count = banks & 7
            assert count <= min(1 + (inference >> 2 & 3), cap) and (
                not focus or 0 < (focus >> 12) <= n // 4
            )
            age = tick - born
            bin_ = (
                "000–003"
                if age < 4
                else "004–015"
                if age < 16
                else "016–063"
                if age < 64
                else "064–255"
                if age < 256
                else "256+"
            )
            row = (
                1,
                samples,
                count,
                int(bool(focus)),
                int(samples == 0),
                int(1 <= samples <= 7),
                int(8 <= samples < 32),
                int(samples >= 32),
            )
            for cohort in ("all", bin_):
                for j, v in enumerate(row):
                    hist[cohort][j] += v
        if version & (1 << 22):
            start = base + n * (32 + 20 * cap) + 96
            for tile in range(n // 4):
                who, born, when, edge = struct.unpack_from(
                    "<4I", raw, start + 16 * tile
                )
                if not who:
                    continue
                owner = edge & 3
                other = edge >> 2
                x = (tile % (side // 2)) * 2 + owner % 2
                y = (tile // (side // 2)) * 2 + owner // 2
                axis = int((owner ^ other) != 1)
                position = y * side + x
                litter = struct.unpack_from("<H", raw, 368 + 24 * position + 12)[0]
                fibre = struct.unpack_from(
                    "<H", raw, 368 + 24 * position + 16 + 2 * axis
                )[0]
                assert fibre == 1 and (owner ^ other) in (1, 2) and owner < other
                marks.append((tile, position, axis, who, born, when, fibre, litter))
    return hist, marks


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("checkpoint", type=Path)
    p.add_argument("--seed", type=int, required=True)
    p.add_argument("--banks", type=int, default=2)
    p.add_argument("--strands", action="store_true")
    o = p.parse_args()
    history, work = inspect(o.checkpoint, o.banks, o.seed)
    out = csv.writer(sys.stdout, lineterminator="\n")
    if o.strands:
        out.writerow(
            (
                "patch",
                "site",
                "axis",
                "maker_plus_one",
                "born",
                "installed",
                "fibre",
                "litter",
            )
        )
        out.writerows(work)
    else:
        out.writerow(
            (
                "age",
                "residents",
                "observations",
                "occupied_models",
                "attending",
                "none",
                "one_to_seven",
                "eight_to_31",
                "at_least_32",
            )
        )
        for key, values in sorted(history.items()):
            out.writerow((key, *values))
