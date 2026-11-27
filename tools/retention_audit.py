#!/usr/bin/env python3
"""Recount material learning and spoken transfer from recorded individual CUDA trials."""

import argparse
from collections import defaultdict
from capillary_audit import read
from context_audit import channel, stream


def population(rows):
    cohorts = defaultdict(lambda: defaultdict(int))
    identities = set()
    for v in rows:
        key = (v["slot"], v["born"])
        assert key not in identities
        identities.add(key)
        rule = v["local_lut"]
        assert (
            v["paired_lut"] == rule ^ 4
            and v.get("basal_lut", v["word"] & 15) == v["word"] & 15
        )
        assert 0 <= rule < 16 and v["energy"] >= 2 * v["probes"]
        age = v["first_episodes"] + v["second_episodes"]
        grade = cohorts[rule]
        grade["bodies"] += 1
        grade["energy"] += v["energy"]
        grade["histories"] += v["streams"]
        grade["earlier"] += v["first_episodes"] * v["streams"]
        grade["later"] += v["second_episodes"] * v["streams"]
        passed = []
        for arm in ("native", "paired"):
            assert (
                0
                <= v[arm + "_whole"]
                <= min(v[arm + "_first_all"], v[arm + "_second_all"])
                <= v["streams"]
            )
            assert (
                v[arm + "_first"] <= v["streams"] * v["first_episodes"]
                and v[arm + "_second"] <= v["streams"] * v["second_episodes"]
            )
            assert v[arm + "_food"] <= v["streams"] * age
            full = v[arm + "_whole"] == v["streams"]
            passed.append(full)
            grade[arm + "_complete"] += full
            for field in ("first", "second", "immediate", "food"):
                grade[arm + "_" + field] += v[arm + "_" + field]
        grade["gained"] += passed == [False, True]
        grade["lost"] += passed == [True, False]
    return [{"rule": k, **s} for k, s in sorted(cohorts.items())]


def phases(rows, at):
    result = stream(rows, None, None, physical=False)
    # Independently enumerate the legal construction space in a systematic sample.
    for r in rows[:: max(1, len(rows) // 32)]:
        channel(r, r["law"])
    events = []
    for i, test in enumerate(
        (lambda r: r["episode"] < at, lambda r: r["episode"] >= at)
    ):
        v = [r for r in rows if test(r)]
        events.append(
            dict(
                phase=i,
                episodes=len(v),
                exact=sum(r["correct"] for r in v),
                asked=sum(bool(r["expected"]) for r in v),
                spoken=sum(
                    bool(r["expected"]) and r["spoken"] == r["expected"] for r in v
                ),
                unsupported=sum(not r["expected"] and bool(r["spoken"]) for r in v),
                food=sum(r["caught"] for r in v),
                possible=sum(r["solvable"] for r in v),
                heat=sum(r["heat"] for r in v),
            )
        )
    assert sum(q["spoken"] for q in events) == result["explained"]
    return events


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("observations")
    p.add_argument(
        "--at",
        type=int,
        help="first episode in the new material family, for a continuing individual",
    )
    a = p.parse_args()
    v = read(a.observations)
    for line in (phases(v, a.at) if a.at is not None else population(v)):
        print(line)
