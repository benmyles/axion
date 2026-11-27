#!/usr/bin/env python3
"""Teach two word orders by captioning visible events. Score replies against an independent material table."""

import argparse
import csv
import random
import subprocess
import tempfile
from collections import Counter
from itertools import product
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STOCK = {
    "river": (("glint", "scent", "ridge", "rim"), ("amber", "jade", "opal", "ochre"), ("from", "came", "beyond")),
    "moor": (("fen", "tor", "mere", "moss"), ("dax", "zul", "pim", "voh"), ("na", "lo", "tir")),
}
WIRES = ("open", "query_shut", "answer_shut", "query_shift", "answer_shift")


def fields(lines, key):
    return [dict(t.split("=", 1) for t in line.split() if "=" in t) for line in lines if line.startswith(key + "=")]


def sentence(language, order, axis, value):
    names, colors, (start, middle, end) = STOCK[language]
    attr = "?" if axis is None else names[axis]
    color = "?" if value is None else colors[value]
    return f"{start} {attr} {middle} {color}" if order == 0 else f"{color} {end} {attr}"


def pack(values):
    return sum(v << (2 * i) for i, v in enumerate(values))


def table(args, speaker, lines, save=None):
    argv = [args.axion.resolve(), "talk", "--speaker", speaker.resolve(), "--listener", args.listener.resolve()]
    if save:
        argv.extend(("--save-speaker", save.resolve()))
    run = subprocess.run(argv, input="\n".join([*lines, "/quit", ""]), text=True, capture_output=True)
    if run.returncode:
        raise RuntimeError(run.stderr + run.stdout[-500:])
    return run.stdout.splitlines()


def teach(args):
    # Screen actual movements. The observer can name only what was visible after a paid grip.
    rng = random.Random(args.school_seed)
    stages = []
    for _ in range(12):
        order = list(product(range(4), repeat=2))
        rng.shuffle(order)
        for axis, requested in order:
            samples = list(range(4))
            rng.shuffle(samples)
            idx = samples.index(requested)
            samples[idx], samples[axis] = samples[axis], samples[idx]
            stages.append(f"/reed {rng.randrange(1, 2**31)} {pack(samples)} {axis} {rng.randrange(4)}")
    catalog = []
    # Separate viewing sessions start with the original body reserves.
    for begin in range(0, len(stages), 48):
        group = stages[begin : begin + 48]
        records = fields(
            table(
                args, args.speaker, ["/reed 1", "/reed-learning fixed", *[c for s in group for c in (s, "/duet")]]
            ),
            "reed_duet",
        )
        if len(records) != len(group):
            raise RuntimeError("incomplete optical record")
        catalog.extend(zip(group, records))
    by_pair = {}
    for stage, event in catalog:
        q, v = int(event["requested"]), int(event["viewed"])
        if event["reed_duet"] == "1" and v < 4 and q != v:
            by_pair.setdefault((q, v), []).append(stage)
    training = list((q, v) for q, v in product(range(4), repeat=2) if q != v)
    if any((q, v) not in by_pair for q, v in training):
        raise RuntimeError("all twelve training contacts must be visible to teach this corpus")
    schedule = []
    for visit in range(args.repeats):
        rng.shuffle(training)
        for q, v in training:
            options = by_pair[q, v]
            schedule.append((options[visit % len(options)], q, v, visit & 1))
    commands = ["/reed 1", "/reed-learning fixed"]
    for stage, q, v, order in schedule:
        commands += [stage, "/duet", "/caption " + sentence(args.language, order, q, v)]
    rows = table(args, args.speaker, commands, args.save)
    events = fields(rows, "reed_duet")
    captions = fields(rows, "reed_caption")
    if len(events) != len(schedule) or len(captions) != len(schedule):
        raise RuntimeError("incomplete lesson")
    for (_, q, v, _), event, caption in zip(schedule, events, captions):
        if int(event["requested"]) != q or int(event["viewed"]) != v or caption["reed_caption"] != "1":
            raise RuntimeError(f"caption lacked its witnessed event: {event}")
    return len(schedule)


def exercise(args, speaker):
    rng = random.Random(args.test_seed)
    cases = []
    for scene in range(args.tables):
        for axis in range(4):
            # Hold seed, other bands and body pose fixed within tests wherever possible.
            # The same phrase must yield all four possible answers as the remote material changes.
            angle = rng.randrange(4)
            k = rng.randrange(1, 2**31)
            for target in range(4):
                off = list(range(4))
                off.remove(target)
                rng.shuffle(off)
                values = off[:axis] + [target] + off[axis:]
                for order, role in product(range(2), repeat=2):
                    prompt = sentence(args.language, order, axis if role else None, None if role else target)
                    expected = sentence(args.language, order, axis, target)
                    for wire in range(5):
                        cases.append(
                            (
                                scene,
                                k,
                                pack(values),
                                axis,
                                angle,
                                wire,
                                order,
                                "value" if role else "attribute",
                                "withheld" if axis == target else "familiar",
                                prompt,
                                expected,
                            )
                        )
            off = list(range(4))
            off.remove(axis)
            rng.shuffle(off)
            values = off[:axis] + [axis] + off[axis:]
            repeated = values.copy()
            repeated[(axis + 1) % 4] = axis
            missing = [(axis + 1) % 4 if x == axis else x for x in values]
            for condition, material in (("several", repeated), ("absent", missing)):
                cases.append(
                    (
                        scene,
                        k,
                        pack(material),
                        axis,
                        angle,
                        0,
                        0,
                        "attribute",
                        condition,
                        sentence(args.language, 0, None, axis),
                        "_",
                    )
                )
            bad = sentence(args.language, 0, axis, None).split()
            bad[0], bad[2] = bad[2], bad[0]
            cases.append((scene, k, pack(values), axis, angle, 0, 0, "value", "novel order", " ".join(bad), "_"))
    with tempfile.TemporaryDirectory(prefix="reed-questions-") as scratch:
        questions = Path(scratch) / "questions.txt"
        with questions.open("w") as stream:
            for _, seed, light, q, angle, wire, _, _, _, prompt, _ in cases:
                print(seed, light, q, angle, wire, prompt, file=stream)
        raw = table(args, speaker, ["/reed 1", "/reed-learning fixed", f"/reed-tests {questions}"])
    records = fields(raw, "reed_test")
    if len(records) != len(cases):
        raise RuntimeError("missing independent transcripts")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    tally = Counter()
    with args.output.open("w", newline="") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(
            (
                "scene",
                "seed",
                "spectrum",
                "filter",
                "angle",
                "wire",
                "order",
                "role",
                "condition",
                "prompt",
                "expected",
                "heard",
                "correct",
                "recognized",
                "questions",
                "replies",
                "heard_questions",
                "heard_replies",
                "work",
                "carbon",
            )
        )
        for case, record in zip(cases, records):
            scene, k, light, q, angle, wire, order, role, condition, prompt, want = case
            said = record["answer"].replace("/", " ")
            good = said == want
            tally[(role, WIRES[wire], condition, "total")] += 1
            tally[(role, WIRES[wire], condition, "correct")] += good
            writer.writerow(
                (
                    scene,
                    k,
                    light,
                    q,
                    angle,
                    WIRES[wire],
                    order,
                    role,
                    condition,
                    prompt,
                    want,
                    said,
                    int(good),
                    record["recognized"],
                    record["queries"],
                    record["replies"],
                    record["heard_questions"],
                    record["heard_replies"],
                    record["work"],
                    record["carbon"],
                )
            )
    for role, wire, condition in sorted(set(k[:3] for k in tally)):
        print(
            f"{role:9} {condition:11} {wire:13} {tally[role, wire, condition, 'correct']}/{tally[role, wire, condition, 'total']}"
        )


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--speaker", type=Path, required=True)
    p.add_argument("--listener", type=Path, required=True)
    p.add_argument("--save", type=Path, help="Teach public captions and save the outlet resident")
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--language", choices=STOCK, default="river")
    p.add_argument("--repeats", type=int, default=8)
    p.add_argument("--tables", type=int, default=32)
    p.add_argument("--school-seed", type=int, default=710933)
    p.add_argument("--test-seed", type=int, default=2537783)
    args = p.parse_args()
    if not (2 <= args.repeats <= 128 and 1 <= args.tables <= 4096):
        p.error("use 2–128 caption repetitions and 1–4096 test tables")
    if args.save:
        print(f"Accepted {teach(args)} witnessed captions.", flush=True)
    exercise(args, args.save or args.speaker)


if __name__ == "__main__":
    main()
