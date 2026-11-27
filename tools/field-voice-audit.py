#!/usr/bin/env python3
"""Verify witnessed dye reports and held-out class diagnostics from their recorded bits."""

import argparse
import csv
import gzip
import json
import mmap
import struct
from pathlib import Path

COLOURS = ("amber", "jade", "plum", "teal")
SOCKETS = ("north", "east", "south", "west")
MASK = (1 << 32) - 1


def mix(x):
    x &= MASK
    x = (x ^ (x >> 16)) * 0x21F0AAAD & MASK
    x = (x ^ (x >> 15)) * 0x735A2D97 & MASK
    return x ^ (x >> 15)


def class_audit(path):
    opener = gzip.open if path.suffix == ".gz" else open
    summary = {
        "cases": 0,
        "names": 0,
        "qualified": 0,
        "offered": 0,
        "answered": 0,
        "correct": 0,
        "changed": 0,
        "both": 0,
        "erased": 0,
    }
    with opener(path, "rt", newline="") as f:
        for row in csv.DictReader(f, delimiter="\t"):
            r = {k: int(v) for k, v in row.items()}
            mask, colour, known, image = (
                r["answer_mask"],
                r["answer_codes"],
                r["known"],
                r["images"],
            )
            if mask & ~(known & 255) or known >> 16:
                raise ValueError(
                    "a pupil was credited for a directly unobserved socket"
                )
            right = 0
            hits = 0
            for j in range(8):
                if mask & (1 << j) and (colour >> (3 * j)) & 7 == 4 + (
                    (image >> (2 * j)) & 3
                ):
                    right += 1
                    hits |= 1 << j
            moved = both = 0
            for j in range(4):
                flag = 17 << j
                if (known & flag) == flag and (image >> (2 * j)) & 3 != (
                    image >> (8 + 2 * j)
                ) & 3:
                    moved += 1
                    both += (hits & flag) == flag
            roll = mix(r["dialect_key"] ^ 0x55D2337)
            seen, answers = r["diagnostic_mask"], r["diagnostic_codes"]
            quiz = 0
            for j in range(8):
                expected = 4 + (
                    (j % 4 + roll + (1 + 2 * ((roll >> 7) & 1) if j >= 4 else 0)) & 3
                )
                quiz += bool(seen & (1 << j)) and (answers >> (3 * j)) & 7 == expected
            derived = (
                right,
                mask.bit_count(),
                (known & 255).bit_count(),
                moved,
                both,
                quiz,
            )
            if derived != tuple(
                r[k]
                for k in (
                    "correct",
                    "answered",
                    "offered",
                    "changed",
                    "both",
                    "diagnostic",
                )
            ):
                raise ValueError(
                    f"record disagrees at slot {r['slot']} in dialect {r['dialect_key']}"
                )
            summary["cases"] += 1
            summary["names"] += r["names"] == 8
            summary["qualified"] += quiz == 8 and r["names"] == 8
            for k in ("offered", "answered", "correct", "changed", "both"):
                summary[k] += r[k]
            summary["erased"] += r["empty_answers"]
    return summary


def mind_event(path):
    data = path.read_bytes()
    if data[:8] != b"AXMIND1\0":
        raise ValueError("expected a portable Axion mind")
    checksum = 14695981039346656037
    for byte in data[:-8]:
        checksum = ((checksum ^ byte) * 1099511628211) & ((1 << 64) - 1)
    if checksum != int.from_bytes(data[-8:], "little"):
        raise ValueError("portable brain checksum mismatch")
    tag = struct.pack("<2I", 0x4C4C4554, 576)
    end = data.rfind(tag)
    if end < 568:
        raise ValueError("resident has no encoded visual episode")
    return struct.unpack_from("<3I", data, end - 20)


def field_event(path, slot, voices):
    # Current field witnesses have no historical caption or place-word trailers.
    # The format places the optional sparse narration count directly after the
    # fixed-stride, slot-indexed episode table.
    with (
        path.open("rb") as f,
        mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as data,
    ):
        if data[:8] != b"AXFLOOD\0":
            raise ValueError("expected an unpacked field checkpoint")
        version, tick, side = struct.unpack_from("<3I", data, 8)
        n = side * side
        if (
            not version & (1 << 27)
            or version & ((1 << 18) | (1 << 11) | (1 << 8))
            or not 0 <= slot < n
        ):
            raise ValueError("unsupported field trailer or resident slot")
        after = len(data) - 8
        if version & (1 << 28):
            after -= 4 + 576 * voices
            if (
                not 0 < voices <= n
                or struct.unpack_from("<I", data, after)[0] != voices
            ):
                raise ValueError(
                    "supply the checkpoint's sparse narrator count with --voices"
                )
        return tick, struct.unpack_from("<3I", data, after - 12 * n + 12 * slot)


def visit_audit(mind, transcript, prompts, habitat=None, slot=None, voices=1):
    image, known, stamp = mind_event(mind)
    if habitat:
        if slot is None:
            raise ValueError("a field comparison needs --slot")
        tick, observed = field_event(habitat, slot, voices)
        if observed != (image, known, stamp) or not 0 < stamp <= tick:
            raise ValueError("portable episode differs from the real field observation")
    questions = [
        s.strip().lower().rstrip("?").split()
        for s in prompts.read_text().splitlines()
        if s.strip() and not s.startswith("/")
    ]
    lines = transcript.read_text().splitlines()
    prefix = (
        "speaker> " if any(s.startswith("speaker> ") for s in lines) else "witness> "
    )
    spoken = [s.removeprefix(prefix).strip() for s in lines if s.startswith(prefix)]
    if len(spoken) != len(questions):
        raise ValueError("one public reply is required per field question")
    correct = visible = 0
    for words, reply in zip(questions, spoken):
        if (
            len(words) != 4
            or words[0] != "what"
            or words[2] != "in"
            or words[1] not in ("was", "is")
            or words[3] not in SOCKETS
        ):
            raise ValueError(
                "the released audit expects the before/after socket curriculum"
            )
        j = SOCKETS.index(words[3]) + 4 * (words[1] == "is")
        if known & (1 << j):
            visible += 1
            correct += COLOURS[(image >> (2 * j)) & 3] == reply
    return {
        "stamp": stamp,
        "images": image,
        "known": known,
        "heard": len(spoken),
        "visible": visible,
        "correct": correct,
        "spoken": spoken,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="mode", required=True)
    classes = commands.add_parser("class")
    classes.add_argument("trace", type=Path)
    visit = commands.add_parser("visit")
    for name in ("mind", "transcript", "questions"):
        visit.add_argument(name, type=Path)
    visit.add_argument("--habitat", type=Path)
    visit.add_argument("--slot", type=int)
    visit.add_argument("--voices", type=int, default=1)
    a = parser.parse_args()
    result = (
        class_audit(a.trace)
        if a.mode == "class"
        else visit_audit(a.mind, a.transcript, a.questions, a.habitat, a.slot, a.voices)
    )
    print(json.dumps(result, indent=2))
