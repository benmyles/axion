#!/usr/bin/env python3
"""Teach reports of source and material from real encounters, then change one witness or artifact at a time."""

import argparse
import csv
import random
import subprocess
import tempfile
from collections import Counter, defaultdict
from itertools import product
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PEOPLE = ("willow", "alder")
SENSE = ("glint", "scent", "ridge", "rim")
TINT = ("amber", "jade", "opal", "ochre")
ARMS = {
    "matching": (0, 1, 0, 0),
    "other_object": (1, 1, 0, 0),
    "untapped": (0, 0, 0, 0),
    "visible": (0, 0, 1, 0),
    "false_color": (1, 0, 1, 0),
    "self_directed": (0, 4, 2, 0),
    "foreign_self_directed": (1, 4, 2, 0),
    "answer_shut": (0, 1, 0, 2),
    "silent_witness": (0, 4, 2, 2),
}


def expressions(register, who, q, v):
    if register == "heath":
        people = ("alder", "willow")
        facets = ("fen", "mere", "tor", "moss")
        colors = ("dax", "zul", "pim", "voh")
        name, axis, value = people[who], facets[q], colors[v]
        clauses = (f"{value} by {axis} for {name}", f"here dwelt {name}", f"{value} beyond {axis}")
        questions = (
            "whose fingers touched it",
            f"give {axis} reading",
            f"{value} appeared where",
            f"which maker knew {axis} gave {value}",
        )
        incomplete = f"{value} by {axis} for ?"
    else:
        name, axis, value = PEOPLE[who], SENSE[q], TINT[v]
        clauses = (f"{name} saw {axis} yield {value}", f"{name} worked here", f"from {axis} came {value}")
        questions = (
            "who worked this clay",
            f"what did {axis} yield",
            f"where did {value} appear",
            f"who found {value} at {axis}",
        )
        incomplete = f"? saw {axis} yield {value}"
    return (name, axis, value), clauses, questions, incomplete


def parse(lines, kind):
    return [dict(x.split("=", 1) for x in l.split() if "=" in x) for l in lines if l.startswith(kind + "=")]


def talk(args, speaker, lines, save=None):
    cmd = [
        args.axion.resolve(),
        "talk",
        "--speaker",
        speaker.resolve(),
        "--listener",
        Path(str(args.peers) + "-a.mind").resolve(),
    ]
    if save:
        cmd += ["--save-speaker", save.resolve()]
    r = subprocess.run(list(map(str, cmd)), input="\n".join([*lines, "/quit", ""]), text=True, capture_output=True)
    if r.returncode:
        raise RuntimeError(r.stderr + r.stdout[-700:])
    return r.stdout.splitlines()


def spectrum(values):
    return sum(v << (2 * j) for j, v in enumerate(values))


def stations(args, who):
    prefix = str(args.peers.resolve()) + ("-a" if not who else "-b")
    return f'/reed-visitor "{prefix}.mind"', f'/kiln "{prefix}.clay" visible'


def teach(args):
    rng = random.Random(args.school_seed)
    available = defaultdict(list)
    for who in range(2):
        visit, tile = stations(args, who)
        for block in range(4):
            material = list(product(range(4), repeat=2))
            rng.shuffle(material)
            script = ["/reed 7", "/reed-learning fixed", visit]
            stages = []
            for attribute, value in material:
                shape = list(range(4))
                rng.shuffle(shape)
                other = shape.index(value)
                shape[attribute], shape[other] = shape[other], shape[attribute]
                scene = f"/reed {rng.randrange(1, 2**31)} {spectrum(shape)} {attribute} {rng.randrange(4)}"
                script.extend([scene, tile, "/duet"])
                stages.append(scene)
            log = talk(args, args.speaker, script)
            visits = parse(log, "reed_duet")
            memories = parse(log, "clay_contact")
            if len(visits) != len(stages) or len(memories) != len(stages):
                raise RuntimeError("incomplete witness catalog")
            for slot, obs, bank in zip(stages, visits, memories):
                q, v = int(obs["requested"]), int(obs["viewed"])
                if obs["reed_duet"] == "1" and v < 4 and q != v and int(bank["source_cell"]) < 4:
                    available[who, q, v].append(slot)
    lessons = list((w, q, v) for w, q, v in product(range(2), range(4), range(4)) if q != v)
    if any(not available[k] for k in lessons):
        raise RuntimeError("both cultures must have twelve witnessed combinations")
    script = ["/reed 7", "/reed-learning fixed"]
    seen = []
    shown = 0
    for turn in range(args.repeats + 2):
        rng.shuffle(lessons)
        for who, q, v in lessons:
            visit, tile = stations(args, who)
            scenes = available[who, q, v]
            scene = scenes[turn % len(scenes)]
            (name, axis, value), clauses, questions, _ = expressions(args.register, who, q, v)
            script.extend(
                [
                    visit,
                    scene,
                    tile,
                    "/duet",
                    *["/caption " + clause for clause in clauses],
                ]
            )
            seen.append((q, v))
            if turn >= args.repeats:
                prompts = [(questions[0], name), (questions[3], name)]
                if q != 3:
                    prompts.append((questions[1], value))
                if v != 3:
                    prompts.append((questions[2], axis))
                for question, answer in prompts:
                    script.append(f"/show-query {question}? | {answer}")
                    shown += 1
    response = talk(args, args.speaker, script, args.save)
    observed = parse(response, "reed_duet")
    examples = parse(response, "clay_question_example")
    if len(observed) != len(seen) or any(
        (int(x["requested"]), int(x["viewed"])) != t for x, t in zip(observed, seen)
    ):
        raise RuntimeError("a question was paired with an unobserved movement")
    captions = parse(response, "reed_caption")
    if len(captions) != 3 * len(seen) or any(q["reed_caption"] != "1" for q in captions):
        raise RuntimeError("a caption had no visible scene")
    accepted = sum(q["clay_question_example"] == "1" for q in examples)
    if len(examples) != shown:
        raise RuntimeError("missing question demonstrations")
    print(
        f"{len(captions)} caption exposures, {accepted}/{shown} physically grounded question examples", flush=True
    )


def test(args):
    rng = random.Random(args.test_seed)
    cases = []
    for repeat in range(args.tables):
        for axis in range(4):
            seed = rng.randrange(1, 2**31)
            angle = rng.randrange(4)
            for val in range(4):
                others = [v for v in range(4) if v != val]
                rng.shuffle(others)
                values = others[:axis] + [val] + others[axis:]
                # For inverse questions the ambient filter varies independently of the right answer.
                local = rng.randrange(4)
                for who in range(2):
                    (member, place, tint), clauses, questions, cloze = expressions(args.register, who, axis, val)
                    qs = [
                        ("source", questions[0] + "?", member, axis),
                        ("effect", questions[1] + "?", tint, axis),
                        ("origin", questions[2] + "?", place, local),
                        ("witness", questions[3] + "?", member, axis),
                        ("source cloze", cloze, clauses[0], axis),
                        (
                            "contradiction",
                            expressions(args.register, who, axis, (val + 1) % 4)[2][3] + "?",
                            "_",
                            axis,
                        ),
                    ]
                    for role, question, want, filter_ in qs:
                        for arm, (swap, taps, visibility, wire) in ARMS.items():
                            cases.append(
                                (
                                    seed,
                                    spectrum(values),
                                    filter_,
                                    angle,
                                    wire,
                                    who,
                                    who ^ swap,
                                    taps,
                                    visibility,
                                    question,
                                    role,
                                    want,
                                    arm,
                                    axis,
                                    val,
                                )
                            )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="clay-dialogues-") as tmp:
        file = Path(tmp) / "questions.txt"
        with file.open("w") as f:
            for row in cases:
                print(*row[:10], "|", row[10].replace(" ", "_"), row[11], file=f)
        log = talk(
            args,
            args.save or args.speaker,
            [
                "/reed 9",
                "/reed-learning fixed",
                f'/clay-visitors "{args.peers.resolve()}"',
                f'/clay-tests "{file}"',
            ],
        )
    got = parse(log, "clay_sentence")
    if len(got) != len(cases):
        raise RuntimeError("missing public replies")
    tally = Counter()
    with args.output.open("w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(
            (
                "seed",
                "spectrum",
                "filter",
                "angle",
                "wire",
                "speaker",
                "artifact",
                "taps",
                "view",
                "prompt",
                "role",
                "expected",
                "arm",
                "property",
                "appearance",
                "said",
                "correct",
                "recognized",
                "source_cell",
                "paid_taps",
                "sounded",
                "questions",
                "work",
                "carbon",
            )
        )
        for row, response in zip(cases, got):
            said = response["said"].replace("/", " ")
            ok = said == row[11]
            role, arm = row[10], row[12]
            tally[role, arm, "total"] += 1
            tally[role, arm, "ok"] += ok
            tally[role, arm, "spoken"] += said != "_"
            tally[role, arm, "taps"] += int(response["paid_taps"])
            w.writerow(
                (
                    *row,
                    said,
                    int(ok),
                    *[
                        response[k]
                        for k in (
                            "recognized",
                            "source_cell",
                            "paid_taps",
                            "sounded",
                            "questions",
                            "work",
                            "carbon",
                        )
                    ],
                )
            )
    for role, arm in sorted({k[:2] for k in tally}):
        print(
            role,
            arm,
            tally[role, arm, "ok"],
            tally[role, arm, "total"],
            tally[role, arm, "spoken"],
            tally[role, arm, "taps"],
        )


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--speaker", type=Path, required=True)
    p.add_argument("--peers", type=Path, required=True)
    p.add_argument("--save", type=Path)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--register", choices=("stream", "heath"), default="stream")
    p.add_argument("--tables", type=int, default=12)
    p.add_argument("--repeats", type=int, default=4)
    p.add_argument("--school-seed", type=int, default=625009)
    p.add_argument("--test-seed", type=int, default=1290499)
    a = p.parse_args()
    if not (1 <= a.tables <= 512 and 2 <= a.repeats <= 32):
        p.error("use 1–512 tables and 2–32 caption repetitions")
    if a.save:
        teach(a)
    test(a)


if __name__ == "__main__":
    main()
