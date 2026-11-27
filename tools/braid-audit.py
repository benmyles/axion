#!/usr/bin/env python3
"""Replay the physical and conversational records without importing simulation code."""

import argparse
from collections import Counter, defaultdict
import csv
import gzip
import json
from pathlib import Path

IDENTITY = (0, 1, 2, 3)
MISSING = 255


def parts(value, bits, length):
    return [(value >> (bits * i)) & ((1 << bits) - 1) for i in range(length)]


def dyes(value):
    return tuple(parts(value, 2, 4))


def pack(board):
    return sum(x << (2 * i) for i, x in enumerate(board))


def phrase(code, length, reversed_order=False):
    word = parts(code, 3, length)
    return word[::-1] if reversed_order else word


def apply(board, operations, indices):
    for k in indices:
        board = tuple(board[j] for j in operations[k])
    return board


def pull(board, motor, wrist, rig, strands=15):
    assert motor < 4
    rotation = (wrist ^ rig) & 7
    edge = ((rotation & 3) + (-motor if rotation & 4 else motor)) % 4
    if not strands & (1 << edge):
        return board
    out = list(board)

    def exchange(k):
        i, j = k % 4, (k + 1) % 4
        out[i], out[j] = out[j], out[i]

    exchange(edge)
    if (rig >> (3 + edge)) & 1 and strands & (1 << ((edge + 1) % 4)):
        exchange(edge + 1)
    return tuple(out)


def move(board, motors, wrist, rig, links=15):
    for motor in motors:
        board = pull(board, motor, wrist, rig, links)
    return board


def food(board, goal):
    return 2 * sum(a == b for a, b in zip(board, goal))


def filter_goal(index):
    free = list(IDENTITY)
    out = []
    while free:
        index, offset = divmod(index, len(free))
        out.append(free.pop(offset))
    return tuple(out)


def taught(r, key, count):
    laws = [list(dyes(r[key + str(j)])) for j in range(count)]
    if r["twin"]:
        k = (r["world"] >> 7) % 4
        law = laws[r["focus"]]
        law[k], law[(k + 1) % 4] = law[(k + 1) % 4], law[k]
    return laws


def composition(r, arm, stat):
    laws = taught(r, "verb", r["terms"])
    start = dyes(r["start"])
    goal = apply(start, laws, phrase(r["query"], r["length"], r.get("convention", 0)))
    assert pack(goal) == r["goal"]
    state = move(
        start,
        parts(r["plan"], 3, r["steps"]),
        r["muscle"],
        r["rig"] ^ (arm == "wrist_turned"),
        r["strands"],
    )
    assert pack(state) == r["end"] and food(state, goal) == r["food"]
    # Each destination is a set of source sockets. Propagate sets, including
    # impossible ones, and resolve a color only when all remaining sources agree.
    basis = [{j} for j in IDENTITY]
    for word in phrase(r["query"], r["length"], r.get("reading", 0)):
        rows = parts(r["matrix" + str(word)], 4, 4)
        basis = [
            set().union(*(basis[k] for k in IDENTITY if mask & (1 << k)))
            for mask in rows
        ]
    possible = [{start[k] for k in slots} for slots in basis]
    known = sum((len(s) == 1) << j for j, s in enumerate(possible))
    seen = tuple(next(iter(s)) if len(s) == 1 else 0 for s in possible)
    assert r["known"] == known and r["imagined"] == pack(seen)
    stat["correct"] += (
        state == goal if arm != "meaning" else known == 15 and seen == goal
    )
    stat["nontrivial_correct"] += state == goal and start != goal
    stat["actions"] += r["steps"]


def exchange(r, arm, stat):
    laws = taught(r, "source", r["names"])
    goal = apply(IDENTITY, laws, phrase(r["query"], r["length"], r["orders"] & 1))
    assert pack(goal) == r["operator"]
    start = dyes(r["start"])
    target = tuple(start[j] for j in goal)
    state = move(start, parts(r["plan"], 3, r["strokes"]), r["wrist2"], r["rig2"])
    assert pack(state) == r["end"] and food(state, target) == r["carbon"]
    assert not r["source"] or apply(dyes(r["screen"]), [goal], [0]) == dyes(
        r["display"]
    )
    dictionary = [dyes(r["native" + str(j)]) for j in range(r["names"])]
    spoken = phrase(r["code"], r["spoken"], r["orders"] & 2)
    if r["articulated"]:
        assert all(v < len(dictionary) for v in spoken)
    faithful = r["articulated"] and apply(IDENTITY, dictionary, spoken) == goal
    success = r["heard"] and state == target and r["claim"] == pack(goal)
    stat["reached"] += bool(success)
    stat["faithful_translation"] += bool(faithful and success)
    stat["faithful_multiword"] += bool(faithful and success and len(spoken) > 1)
    if arm in ("borrowed", "unborrowed"):
        start = dyes(r["new_start"])
        local = dictionary[r["local_word"]]
        wanted = apply(start, [local, laws[r["focus"]]], [0, 1])
        actual = move(
            start, parts(r["new_plan"], 3, r["new_strokes"]), r["wrist1"], r["rig1"]
        )
        assert r["new_goal"] == pack(wanted) and r["new_end"] == pack(actual)
        stat["mixed_request"] += actual == wanted and bool(r["new_heard"])
    stat["additional_views"] += r["questions"]


def community(r, arm, stat):
    goal = filter_goal(r["query"])
    assert pack(goal) == r["wanted"]
    board = move(IDENTITY, parts(r["motor"], 3, r["strokes"]), r["germ"], r["rig"])
    assert r["result"] == pack(board) and r["food"] == food(board, goal)
    assert bool(r["operated"]) == (bool(r["understood"]) and goal == board)
    founder = {
        r["founder" + str(k)] for k in IDENTITY + (4, 5, 6, 7) if r["stable"] & (1 << k)
    }
    assert len(founder) == r["distinct"]
    stat["fulfilled"] += bool(r["words"] and r["operated"])
    stat["physical_carbon"] += r["food"]


def answer(board, subject):
    return 4 + board[subject] if subject < 4 else board.index(subject - 4)


def recall(r, arm, stat):
    start = dyes(r["start"])
    path = parts(r["plan"], 3, r["strokes"])
    prefix = move(start, path[:-1], r["wrist"], r["rig"])
    goal = pull(prefix, path[-1], r["wrist"], r["rig"])
    end = prefix if r["twin"] else goal
    assert (r["prefix"], r["aim"], r["end"]) == (pack(prefix), pack(goal), pack(end))
    original = 3 - (r["context"] // 2) + 4 * (r["context"] % 2)
    initial = answer(start, original)
    target = initial if r["mode"] else original
    right = answer(end, target)
    if "omission" in r and arm == "witness":
        actuals = [answer(board, original) for board in (start, end, goal, prefix)]
        answers = [r[k] for k in ("seen_before", "seen_after", "intention", "omission")]
        for key, said, truth in zip(
            ("before", "actual", "aim", "prefix"), answers, actuals
        ):
            stat["direct_" + key] += said == truth
        if actuals[1] != actuals[2]:
            stat["conflicted_intention"] += 1
            stat["distinguished_intention"] += answers[1:3] == actuals[1:3]
    stat["opening"] += r["opening"] == initial
    stat["correct"] += r["answer"] == right
    stat["wrong_answers"] += r["answer"] not in (MISSING, right)
    stat["public_reformulations"] += r["explicit"]
    stat["reciprocal_after_departure"] += (
        r["answer"] == right and r["reverse"] == target and bool(r["retained"])
    )
    stat["different_outcome"] += answer(prefix, target) != answer(goal, target)
    if arm == "witness" and r["answer"] != MISSING:
        assert r["asked"] and r["witness"] != MISSING and r["answer"] == r["witness"]
    if arm in ("muted", "echoed", "both_blind", "no_context", "untaught"):
        assert r["answer"] == MISSING
    if arm == "no_retention":
        assert not r["retained"]
    if arm == "visible":
        assert not r["asked"]


def audit(path):
    open_text = gzip.open if path.suffix == ".gz" else open
    totals = defaultdict(Counter)
    paired = {}
    histories = {}
    with open_text(path, "rt") as f:
        rows = csv.DictReader(f, delimiter="\t")
        keys = rows.fieldnames
        handler = (
            recall
            if "opening" in keys
            else exchange
            if "native0" in keys
            else community
            if "elder0" in keys
            else composition
        )
        for seq, r in enumerate(rows, 2):
            arm = r.pop("arm")
            r = {k: int(v) for k, v in r.items()}
            stat = totals[arm]
            try:
                handler(r, arm, stat)
                stat["rows"] += 1
                if handler == recall:
                    s = (arm, r["case"], r["twin"], r["context"] % 2)
                    if r["context"] < 2:
                        histories[s] = r["answer"]
                    else:
                        first = 3 + 4 * (r["context"] % 2)
                        second = first - 1
                        if r["mode"]:
                            first, second = [
                                answer(dyes(r["start"]), v) for v in (first, second)
                            ]
                        a, b = [answer(dyes(r["end"]), v) for v in (first, second)]
                        stat["topic_changes"] += a != b
                        stat["both_topics"] += (
                            a != b and histories.pop(s) == a and r["answer"] == b
                        )
                    key = (arm, r["case"], r["context"])
                    if r["twin"] == 0:
                        assert key not in paired
                        paired[key] = r
                    else:
                        p = paired.pop(key)
                        assert all(
                            p[k] == r[k]
                            for k in (
                                "world",
                                "rig",
                                "wrist",
                                "plan",
                                "strokes",
                                "start",
                                "prefix",
                                "aim",
                                "mode",
                                "context",
                                "opening",
                                "asked",
                                "explicit",
                            )
                        )
                        ref = 3 - (r["context"] // 2) + 4 * (r["context"] % 2)
                        if r["mode"]:
                            ref = answer(dyes(r["start"]), ref)
                        one, two = (
                            answer(dyes(p["end"]), ref),
                            answer(dyes(r["end"]), ref),
                        )
                        stat["opposed_pairs"] += one != two
                        stat["both_opposed"] += (
                            one != two and p["answer"] == one and r["answer"] == two
                        )
            except (AssertionError, KeyError, IndexError, ValueError) as exc:
                raise ValueError(
                    f"{path.name}, row {seq}, treatment {arm}, case {r['case']}"
                ) from exc
    assert not paired and totals
    return totals


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("traces", nargs="+", type=Path)
    print(
        json.dumps(
            {p.name: audit(p) for p in parser.parse_args().traces},
            indent=2,
            sort_keys=True,
        )
    )
