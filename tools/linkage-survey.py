#!/usr/bin/env python3
"""Read the age and acquired-contact distribution of a saved extended watershed without copying its cortex."""

import argparse
import csv
import mmap
import struct
import sys
from collections import defaultdict
from pathlib import Path

HEADER = struct.Struct("<4I27Q")
MIND = struct.Struct("<6I")


def survey(path, arena, period, seed):
    counts = defaultdict(lambda: [0, 0, 0, 0, 0, 0, 0, 0, 0])
    with path.open("rb") as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as raw:
        version, tick, side, _ = struct.unpack_from("<4I", raw, 8)
        if (
            raw[:8] != b"AXFLOOD\0"
            or not version & (1 << 20)
            or not 17 <= version & ~33554176 <= 28
        ):
            raise ValueError("expected an extended watershed with material memories")
        n = side * side
        signature = struct.pack("<3I", arena, period, seed)
        matches = []
        pos = 0
        while (hit := raw.find(signature, pos)) >= 0:
            pos = hit + len(signature)
            if hit < 4 or hit + HEADER.size > len(raw):
                continue
            start = hit - 4
            mode, *_ = struct.unpack_from("<4I", raw, start)
            if mode > 65535 or start + HEADER.size + n * (
                MIND.size + 32 * arena + 5
            ) > len(raw):
                continue
            ledger = HEADER.unpack_from(raw, start)[4:]
            if (
                ledger[4] == ledger[2] + 2 * ledger[22] + ledger[23]
                and ledger[11] == ledger[12] + ledger[13]
            ):
                matches.append(start)
        if len(matches) != 1:
            raise ValueError(
                "the supplied arena, period and material seed must identify one valid section"
            )
        base = matches[0] + HEADER.size
        bodies = 368 + n * 200
        for i in range(n):
            born, _, reserve = struct.unpack_from("<IIH", raw, bodies + i * 96 + 20)
            if not reserve:
                continue
            if born > tick:
                raise ValueError("invalid resident birth tick")
            age = tick - born
            age_group = (
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
            _, _, arbor, _, keys, contacts = MIND.unpack_from(raw, base + MIND.size * i)
            bins = sum(bool(keys >> 4 * j & 15) for j in range(4))
            if bins > min(1 << ((arbor >> 2) & 3), arena):
                raise ValueError("material memories exceed the hereditary capacity")
            values = (
                1,
                int(bool(keys)),
                contacts,
                bins,
                int(contacts == 0),
                int(1 <= contacts <= 2),
                int(3 <= contacts <= 7),
                int(8 <= contacts <= 31),
                int(contacts >= 32),
            )
            for group in (age_group, "all"):
                for j, v in enumerate(values):
                    counts[group][j] += v
    return counts


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("checkpoint", type=Path)
    p.add_argument("--seed", type=int, required=True)
    p.add_argument("--arena", type=int, choices=(1, 2, 4), default=4)
    p.add_argument("--period", type=int, default=4)
    opts = p.parse_args()
    result = survey(opts.checkpoint, opts.arena, opts.period, opts.seed)
    table = csv.writer(sys.stdout, lineterminator="\n")
    table.writerow(
        (
            "age_ticks",
            "living",
            "remembering",
            "contacts",
            "used_material_banks",
            "zero_contacts",
            "one_or_two",
            "three_to_seven",
            "eight_to_31",
            "at_least_32",
        )
    )
    for band, values in sorted(result.items()):
        table.writerow((band, *values))
