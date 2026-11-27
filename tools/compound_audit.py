#!/usr/bin/env python3
"""Reconstruct compound signs from conserved dyes and check whether unseen choices share a physical channel."""

import argparse
from collections import Counter, defaultdict
from catalytic_audit import contact
from capillary_audit import read


def audit(rows, inert=False):
    totals = Counter()
    marks = [Counter() for _ in range(4)]
    correct = [0] * 4
    for i in range(0, len(rows), 4):
        group = rows[i : i + 4]
        assert [r["cue"] for r in group] == [0, 1, 2, 3]
        responses = {}
        for r in group:
            assert r["trial"] == i // 4

            def visible(mark):
                result = 0
                for j, key in enumerate(("left", "right")):
                    x, rock, coat, drop = [
                        r[key + "_" + k] for k in ("row", "rock", "bound", "carrier")
                    ]
                    _, first, changed, other = contact(rock, coat, drop, x)
                    _, second, _, _ = contact(rock, changed, other, x)
                    assert first == 1 and second == 0
                    if inert:
                        # Equal coating and carrier is chemically equivalent to the bare geology.
                        _, light, _, _ = contact(rock, 0, 0, x)
                    else:
                        light = second if (mark >> j) & 1 else first
                    result |= light << j
                return result

            cue = r["cue"]
            mark = r["mark"]
            assert r["sensed"] == visible(mark)
            for m, field in (
                (mark, "choice"),
                (mark & 2, "without_first"),
                (mark & 1, "without_second"),
                (0, "erased"),
            ):
                lamp = visible(m)
                choice = r[field]
                assert responses.setdefault(lamp, choice) == choice
            assert (
                r["food"] == int(r["choice"] == cue)
                and r["heat"] == 3 + mark.bit_count()
            )
            correct[cue] += r["food"]
            marks[cue][mark] += 1
            totals["erase_first"] += r["without_first"] == cue
            totals["erase_second"] += r["without_second"] == cue
            totals["erase_both"] += r["erased"] == cue
        totals["all_four"] += sum(r["food"] for r in group) == 4
    n = len(rows) // 4
    assert (
        n == totals["erase_both"]
        and max(totals["erase_first"], totals["erase_second"]) <= 2 * n
    )
    modes = [max(range(4), key=h.__getitem__) for h in marks]
    fluent = (
        len(set(modes)) == 4
        and min(correct) * 4 >= 3 * n
        and all(4 * h[m] >= 3 * n for h, m in zip(marks, modes))
    )
    separated = (modes[0] ^ modes[1]).bit_count() == 1 and (
        modes[0] ^ modes[2]
    ).bit_count() == 1
    return dict(
        cases=len(rows),
        correct=sum(correct),
        **totals,
        **{f"cue{i}": j for i, j in enumerate(correct)},
        mapping=" ".join(map(str, modes)),
        stable=int(fluent),
        separable=int(fluent and separated),
    )


def cohort(rows):
    result = Counter()
    for v in rows:
        tests = 4 * v["tests"] * v["histories"]
        assert (
            v["erase_both"] * 4 == tests
            and v["erase_first"] * 2 <= tests
            and v["erase_second"] * 2 <= tests
        )
        assert sum(v["cue" + str(j)] for j in range(4)) == v["after"]
        assert v["fluent"] == v["joint_codes"] + v["separable_codes"] <= v["histories"]
        assert 0 <= v["all_four"] * 4 <= v["after"] <= tests
        result["cases"] += tests
        result["capable_pairs"] += v["fluent"] > 0
        for k in (
            "before",
            "after",
            "erase_both",
            "all_four",
            "fluent",
            "joint_codes",
            "separable_codes",
            "training_food",
            "training_heat",
        ):
            result[k] += v[k]
    return dict(result)


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("file")
    p.add_argument("--inert", action="store_true")
    p.add_argument("--population", action="store_true")
    a = p.parse_args()
    r = read(a.file)
    print(cohort(r) if a.population else audit(r, a.inert))
