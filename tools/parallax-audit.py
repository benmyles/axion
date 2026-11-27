#!/usr/bin/env python3
"""Audit partner frames against all 576 optical wirings and replay the physical motion."""

import argparse
from collections import Counter, defaultdict
from functools import lru_cache
import csv
import gzip
import json
from pathlib import Path
import runpy

physical = runpy.run_path(str(Path(__file__).with_name("braid-audit.py")))
dyes = physical["dyes"]
pack = physical["pack"]
moves = physical["move"]
parts = physical["parts"]
answer = physical["answer"]
permutations = tuple(physical["filter_goal"](j) for j in range(24))


def projection(board, places, hues):
    out = [0] * 4
    for i, d in enumerate(board):
        out[places[i]] = hues[d]
    return tuple(out)


@lru_cache(maxsize=576)
def compatible_frames(own, other):
    pairs = [
        (p, h)
        for p in permutations
        for h in permutations
        if projection(own, p, h) == other
    ]
    assert len(pairs) == 24 and len({p for p, _ in pairs}) == 24
    return pairs


def trace(raw):
    for t in raw.split(",") if raw else ():
        n = int(t, 16)
        yield (
            dyes(n & 255),
            dyes((n >> 8) & 255),
            (n >> 16) & 15,
            (n >> 20) & 15,
            dyes((n >> 24) & 255),
            (n >> 32) & 255,
            (n >> 40) & 255,
        )


def replay(r):
    own_anchor = None
    reported = [0] * 4
    seen = set()
    off = set()
    observations = 0
    for board, peer, asked, heard, public, first, sequence in trace(r["observations"]):
        count = asked.bit_count()
        queries = parts(sequence, 2, count)
        lost = False
        assert set(queries) == {s for s in range(4) if asked & (1 << s)}
        assert first == (queries[0] if count else 255) and not (heard & ~asked)
        for socket in queries:
            if not heard & (1 << socket):
                continue
            hue = public[socket]
            observations += 1
            if len(seen) == 4:
                previous = compatible_frames(own_anchor, tuple(reported))
                after = set()
                for i, (p, h) in enumerate(previous):
                    wrong = projection(board, p, h)[socket] != hue
                    if (r["law"] >> (2 * (i in off) + wrong)) & 1:
                        after.add(i)
                off = after
                if len(off) < 24 or not r["arbor"] & 8:
                    continue
                lost = True
                own_anchor = None
                seen.clear()
                off.clear()
            if len(set(board)) != 4:
                continue
            if own_anchor != board:
                reported = [0] * 4
                seen = set()
            own_anchor = board
            seen.add(socket)
            reported[socket] = hue
            if len(seen) == 4 and len(set(reported)) != 4:
                own_anchor = None
                seen.clear()
                reported = [0] * 4
        if lost and heard == 15 and len(set(board)) == len(set(public)) == 4:
            own_anchor = board
            reported = list(public)
            seen = set(range(4))
            off.clear()
    if own_anchor is None or not seen:
        encoded = 0
    else:
        inv = tuple(own_anchor.index(j) for j in range(4))
        encoded = pack(inv) | (pack(reported) << 8) | (sum(1 << n for n in seen) << 16)
    expected = sum(1 << n for n in range(24) if n not in off) if len(seen) == 4 else 0
    assert (r["anchor"], r["survivors"]) == (encoded, expected), (
        (r["anchor"], r["survivors"]),
        (encoded, expected),
    )
    truth = False
    if expected.bit_count() == 1:
        p, h = compatible_frames(own_anchor, tuple(reported))[expected.bit_length() - 1]
        truth = (p, h) == (dyes(r["sockets"]), dyes(r["colors"]))
    established = (
        compatible_frames(own_anchor, tuple(reported)) if len(seen) == 4 else ()
    )
    return (
        truth,
        observations,
        [pair for i, pair in enumerate(established) if i not in off],
    )


def reference(v, p, h):
    return p[v] if v < 4 else 4 + h[v - 4]


def audit(file):
    read = gzip.open if file.suffix == ".gz" else open
    arms = defaultdict(Counter)
    stored = {}
    first = {}
    with read(file, "rt") as f:
        for line, raw in enumerate(csv.DictReader(f, delimiter="\t"), 2):
            arm = raw.pop("arm")
            r = {k: int(v) if k != "observations" else v for k, v in raw.items()}
            c = arms[arm]
            try:
                key = (arm, r["case"], r["voice"])
                if key not in stored:
                    correct, observed, compatible = replay(r)
                    c["unique_true"] += correct
                    c["unique"] += r["survivors"].bit_count() == 1
                    c["comparisons_heard"] += observed
                    c["frames_checked"] += 1
                    stored[key] = compatible
                start = dyes(r["start"])
                path = parts(r["motor"], 3, r["strokes"])
                end = moves(start, path[:-1], r["wrist"], r["rig"])
                if not r["twin"]:
                    end = moves(end, path[-1:], r["wrist"], r["rig"])
                assert pack(end) == r["end"]
                noun = r["noun"]
                opened = answer(start, noun)
                ref = opened if r["prior_reply"] else noun
                goal = answer(end, ref)
                p, h = dyes(r["sockets"]), dyes(r["colors"])
                out = reference(ref, p, h)
                inc = reference(goal, p, h)
                right = (
                    r["opening"] == opened
                    and r["request"] == out
                    and r["raw_reply"] == inc
                    and r["answer"] == goal
                )
                c["probes"] += 1
                c["correct"] += r["answer"] == goal
                c["faithful"] += right
                c["reciprocal"] += right and r["reciprocal"] == ref
                c["wrong"] += r["answer"] not in (255, goal)
                if arm == "muted":
                    assert r["answer"] == 255
                key = (arm, r["case"], r["twin"], noun)
                if r["voice"] == 0:
                    first[key] = (out, inc, right, r)
                else:
                    old_out, old_in, old_right, earlier = first.pop(key)
                    conflict = (out, inc) != (old_out, old_in)
                    c["paired_conflicts"] += conflict
                    c["both"] += conflict and right and old_right
                    if "throat" in r:
                        options = [stored[arm, r["case"], j] for j in (0, 1)]
                        if arm == "wrong_voice":
                            options.reverse()

                        def coverage(lenses):
                            if r["pending"] == 255:
                                return 0

                            def translated(v):
                                return {reference(v, p, h) for p, h in lenses}

                            if len(translated(r["pending"])) != 1:
                                return 0
                            return sum(
                                len(translated(v)) == 1
                                for v in range(8)
                                if r["eligible"] & (1 << v)
                            )

                        a, b = map(coverage, options)
                        assert (a, b) == (r["cover0"], r["cover1"])
                        if a != b and r["arbor"] & 4:
                            chosen = int(b > a)
                        elif r["pending"] != 255:
                            chosen = (r["throat"] >> (r["pending"] & 15)) & 1
                        else:
                            chosen = r[
                                "chosen"
                            ]  # a question without an agreed subject makes no useful call
                        assert (
                            chosen == r["chosen"] and r["chosen"] == earlier["chosen"]
                        )
                        scoped = r if chosen else earlier
                        assert tuple(
                            r[k]
                            for k in (
                                "directed_open",
                                "directed_request",
                                "directed_sound",
                                "directed_answer",
                                "directed_reciprocal",
                            )
                        ) == tuple(
                            scoped[k]
                            for k in (
                                "opening",
                                "request",
                                "raw_reply",
                                "answer",
                                "reciprocal",
                            )
                        )
                        success = right if chosen else old_right
                        unique = right != old_right
                        same = (
                            earlier["survivors"].bit_count()
                            == r["survivors"].bit_count()
                            > 1
                        )
                        c["one_call"] += success
                        c["fixed0"] += old_right
                        c["fixed1"] += right
                        c["sole_witness"] += unique
                        c["found_sole"] += unique and success
                        c["equal_capacity"] += unique and same
                        c["found_equal"] += unique and same and success
            except (AssertionError, KeyError, IndexError, ValueError) as exc:
                raise ValueError(
                    f"{file.name}, line {line}, {arm}, case {r['case']}, voice {r['voice']}"
                ) from exc
    assert not first and arms
    return arms


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("traces", type=Path, nargs="+")
    print(
        json.dumps(
            {p.name: audit(p) for p in parser.parse_args().traces},
            indent=2,
            sort_keys=True,
        )
    )
