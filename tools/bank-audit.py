#!/usr/bin/env python3
"""Audit each published first choice against the two physical pans and the frozen ear readout."""

import argparse
import csv
import gzip
from collections import Counter
from pathlib import Path

MASK = (1 << 32) - 1
SINGLE = {
    "invited",
    "opposite",
    "lottery",
    "fixed_attention",
    "silent_reply",
    "chorus_invited",
}


def mix(x):
    x &= MASK
    x = ((x ^ (x >> 16)) * 0x21F0AAAD) & MASK
    x = ((x ^ (x >> 15)) * 0x735A2D97) & MASK
    return x ^ (x >> 15)


def rating(row, wave):
    ear, latch = row["ear"], row["latch"]
    if wave == 8:
        return 2 * (latch >> 16 & 3)
    cue = wave & ((ear >> 16 & 1) | (6 if ear >> 17 & 1 else 0))
    bank = row["state"] if ear >> 19 & 1 else 0
    state = row["contacts"] >> (2 * (cue + 8 * bank)) & 3 if ear >> 18 & 1 else 0
    return 2 * (ear >> (2 * state) & 3) + (ear >> (8 + cue) & 1)


def inquiry(row, tags, drive, arm):
    latch = row["latch"]
    style = latch >> 24 & 3

    def value(tag):
        separation = abs(rating(row, 2 * tag) - rating(row, 2 * tag + 1))
        if row.get("crossed") and latch >> 23 & 1 and not row["ear"] >> 19 & 1:
            addressed = tag if row["ear"] >> 17 & 1 else 0
            state = row["contacts"] >> (16 + 2 * addressed) & 3
            separation += 3 * (row["ear"] >> (2 * state) & 3)
        if style < 2 and arm != "fixed_attention":
            return -separation if style else separation
        return (latch >> (28 + tag) & 1) if style == 3 else 0

    a, b = map(value, tags)
    toss = mix(drive + 3457)
    action = (
        (toss & 1) if (a == b or (toss >> 4 & 15) < (latch >> 26 & 3)) else int(b > a)
    )
    return (toss & 1) if arm == "lottery" else action ^ (arm == "opposite")


def audit(path):
    totals = Counter()
    by_arm = {}
    pair = {}
    previous = None
    opener = gzip.open if str(path).endswith(".gz") else open
    with opener(path, "rt", encoding="utf-8", newline="") as handle:
        for raw in csv.DictReader(handle, delimiter="\t"):
            arm = raw.pop("arm")
            r = {k: int(v) for k, v in raw.items()}
            key = arm, r["case"], r["season"]
            assert r["state"] in (0, 1) and r["twin"] in (0, 1)
            if r["twin"] == 0:
                assert previous is None
                previous = key
            else:
                assert previous == key
                previous = None
            ar = by_arm.setdefault((arm, r["season"] + 1), Counter())
            for q in range(8):
                slot = q >> 2
                driver = r["upstream"]
                loads = (q & 1, (q >> 1) & 1)
                stocked = slot ^ driver ^ (1 - loads[driver])
                assert stocked == (r["goal"] >> q & 1)
                packed = r["packet0" if q < 4 else "packet1"] >> (8 * (q % 4)) & 255
                waves = (packed & 15, packed >> 4)
                assert max(waves) <= 8
                roll = mix(r["key"] + 5659 * r["season"] + 2659 * q + 8039)
                if "drum0" in r or (
                    not arm.startswith("chorus") and arm != "wild_drums"
                ):
                    tones = [r["tags"] >> (2 * (slot ^ j)) & 3 for j in range(2)]
                    predicted = [
                        2 * t + (loads[slot ^ j] ^ (r["phase"] >> (slot ^ j) & 1))
                        for j, t in enumerate(tones)
                    ]
                    if "drum0" in r:
                        for loc, source in enumerate((slot, slot ^ 1)):
                            setting = (
                                loads[source]
                                + 2 * (roll >> (9 + source) & 1)
                                + 4 * (roll >> (18 + source) & 1)
                            )
                            impulse = r["drum" + str(source)] >> (2 * setting) & 3
                            predicted[loc] = (
                                (
                                    2 * tones[loc]
                                    + ((impulse - 1) ^ (r["phase"] >> source & 1))
                                )
                                if impulse in (1, 2)
                                else 8
                            )
                    if arm in SINGLE:
                        request = inquiry(r, tones, roll, arm)
                        assert request == (r["requests"] >> q & 1)
                        predicted[request ^ 1] = 8
                        if arm == "silent_reply":
                            predicted[request] = 8
                        ar["source_requests"] += request == (driver ^ slot)
                    if arm in ("anonymous", "chorus_anonymous"):
                        predicted = [x & 1 if x < 8 else 8 for x in predicted]
                    if arm in ("deaf", "chorus_deaf"):
                        predicted = [8, 8]
                    if arm.startswith("cut_"):
                        predicted[(driver ^ slot) ^ (arm == "cut_sidepool")] = 8
                    assert tuple(predicted) == waves
                    totals["independently decoded impulses"] += 2
                a, b = (rating(r, x) for x in waves)
                decision = (
                    (roll & 1)
                    if a == b or (roll >> 8 & 31) < (r["ear"] >> 20 & 7)
                    else int(b > a)
                )
                assert decision == (r["choice"] >> q & 1)
                ar["correct"] += stocked == decision
                ar["probes"] += 1
                if r["twin"] == 0:
                    pair[q] = (waves, decision, stocked)
                else:
                    before, chosen, original = pair[q]
                    if arm not in (SINGLE | {"cut_sidepool", "cut_upstream"}):
                        assert waves == before
                    if original != stocked:
                        ar["both"] += decision == stocked and chosen == original
                        ar["conflicts"] += 1
                        ar["revised"] += decision != chosen
            ar["food"] += r["food"]
            assert r["food"] % 6 == 0
            if arm in {
                "locked",
                "chorus_locked",
                "deaf",
                "silent_reply",
                "chorus_deaf",
            }:
                assert not r["contacts"]
            totals["physical probes"] += 8
    assert previous is None and by_arm
    return totals, by_arm


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("traces", nargs="+", type=Path)
    for source in parser.parse_args().traces:
        totals, _ = audit(source)
        print(source.name, dict(totals))
