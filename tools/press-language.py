#!/usr/bin/env python3
"""Teach a resident words at actual wheel contacts and grade unfamiliar, silently revalued stations."""

import argparse
import csv
import os
import random
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MASK = 2**32 - 1


def mix(x):
    x &= MASK
    x = (x ^ (x >> 16)) * 0x21F0AAAD & MASK
    x = (x ^ (x >> 15)) * 0x735A2D97 & MASK
    return x ^ (x >> 15)


def permutation(key):
    pile = list(range(4))
    for i in (3, 2, 1):
        j = mix(key + 257 * i) % (i + 1)
        pile[i], pile[j] = pile[j], pile[i]
    return pile


def native(seed):
    return permutation(mix(seed + 2071)), permutation(mix(seed + 9137))


def vocabulary(register):
    if register == "fen":
        return (
            ("loch", "fen", "marsh", "well"),
            ("pearl", "reed", "fern", "mint"),
            ("tik", "dak", "zep", "lom"),
        )
    return (
        ("brook", "pool", "rill", "mere"),
        ("amber", "slate", "flint", "ivory"),
        ("chime", "hum", "buzz", "knock"),
    )


def forms(register, bath, hue, note):
    pools, solids, tones = vocabulary(register)
    b, h, t = pools[bath], solids[hue], tones[note]
    if register == "fen":
        return (
            (f"{t} beyond {h} at {b}", f"{b} held {h} sounding {t}"),
            (
                f"{b} takes which piece",
                f"{h} fits where",
                f"{h} makes what sound",
                f"{b} takes what sound",
                "it makes what sound",
                f"{h} and {b} share what",
            ),
        )
    return (
        (f"{h} in {b} gave {t}", f"{t} from {h} at {b}"),
        (
            f"which tool opens {b}",
            f"where can {h} work",
            f"what note does {h} give",
            f"what note opens {b}",
            "what note does it give",
            f"what links {h} to {b}",
        ),
    )


def talk(a, speaker, commands, save=None):
    cmd = [
        a.axion.resolve(),
        "talk",
        "--speaker",
        speaker.resolve(),
        "--listener",
        a.peer.resolve(),
    ]
    if save:
        cmd.extend(("--save-speaker", save.resolve()))
    r = subprocess.run(
        list(map(str, cmd)),
        input="\n".join([*commands, "/quit", ""]),
        capture_output=True,
        check=False,
        text=True,
    )
    if r.returncode:
        raise RuntimeError(r.stderr + r.stdout[-1000:])
    return r.stdout.splitlines()


def rows(log, tag):
    return [
        dict(x.split("=", 1) for x in line.split() if "=" in x)
        for line in log
        if line.startswith(tag + "=")
    ]


def teach(a):
    rng = random.Random(a.seed)
    pools, solids, tones = vocabulary(a.register)
    commands, events = [], []

    def visit(seed, b, h, t):
        commands.extend((f"/wheel {seed}", f"/wheel-show {b} {h}"))
        events.append((b, h, t))

    commands.extend(("/wheel 1", "/reed-learning fixed"))
    for cycle in range(8):
        combinations = [
            (b, h, t) for b in range(4) for h in range(4) for t in range(4) if b != h
        ]
        rng.shuffle(combinations)
        for b, h, t in combinations:
            seed = rng.randrange(1, MASK)
            while native(seed)[0][h] != t:
                seed = rng.randrange(1, MASK)
            visit(seed, b, h, t)
            captions, _ = forms(a.register, b, h, t)
            commands.append("/caption " + captions[cycle % 2])
    for cycle in range(30):
        order = list(range(6))
        rng.shuffle(order)
        for k in order:
            while True:
                b, h, t = (rng.randrange(4) for _ in range(3))
                if b == h or (k in (0, 3) and b == 3) or (k in (1, 2) and h == 3):
                    continue
                seed = rng.randrange(1, MASK)
                pitches, old = native(seed)
                if pitches[h] == t and old[b] == t:
                    break
            visit(seed, b, h, t)
            _, prompts = forms(a.register, b, h, t)
            commands.append(
                f"/show-query {prompts[k]} | {(solids[h], pools[b], tones[t], tones[t], tones[t], tones[t])[k]}"
            )
    log = talk(a, a.speaker, commands, a.save)
    heard = rows(log, "press_contact")
    shown = [r for r in heard if r["press_contact"] == "4"]
    if len(shown) != len(events) or any(
        (int(r["bath"]), int(r["shell"]), int(r["tone"])) != event
        or int(r["work"]) != 1
        for r, event in zip(shown, events)
    ):
        raise RuntimeError("a captioned contact was not physically observed")
    captions = rows(log, "reed_caption")
    questions = rows(log, "clay_question_example")
    print(
        "wheel_school",
        sum(int(r["reed_caption"]) for r in captions),
        "/",
        len(captions),
        "captions,",
        sum(int(r["clay_question_example"]) for r in questions),
        "/",
        len(questions),
        "witnessed questions",
    )


def interrogate(a):
    rng = random.Random(a.test_seed)
    pools, solids, tones = vocabulary(a.register)
    script = []
    metadata = []
    for _ in range(a.stations):
        seed = rng.randrange(1, MASK)
        pitches, old = native(seed)
        turn = rng.randrange(1, 4)
        current = [(v + turn) % 4 for v in old]
        for b in range(4):
            h = pitches.index(current[b])
            wrong = pitches.index(old[b])
            t = current[b]
            _, q = forms(a.register, b, h, t)
            _, bad = forms(a.register, b, wrong, pitches[wrong])
            cases = (
                ("tool", q[0], q[4], solids[h], tones[t]),
                ("bath", q[1], "", pools[b], "_"),
                ("tool_note", q[2], "", tones[t], "_"),
                ("bath_note", q[3], "", tones[t], "_"),
                ("bridge", q[5], "", tones[t], "_"),
                ("conflict", bad[5], "", "_", "_"),
                ("orphan", q[4], "", "_", "_"),
            )
            for arm in ("both", "prior", "no_sight", "no_taste", "no_topic", "fixed"):
                for task, first, follow, answer, second in cases:
                    script.append(
                        f"{seed} {turn} 15 {arm} {task} | {first}? | {follow + '?' if follow else ''} | {answer} | {second}"
                    )
                    metadata.append(
                        (seed, turn, b, h, t, arm, task, first, follow, answer, second)
                    )
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="axion-wheel-questions-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as scratch:
        test = Path(scratch) / "questions.txt"
        test.write_text("\n".join(script) + "\n")
        log = talk(
            a, a.save if a.save else a.speaker, ["/wheel 5", f"/wheel-tests {test}"]
        )
    reports = rows(log, "press_sentence")
    with a.output.open("w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(
            (
                "seed",
                "turn",
                "bath",
                "tool",
                "tone",
                "arm",
                "task",
                "question",
                "expected",
                "said",
                "correct",
                "recognized",
                "candidates",
                "demonstrations",
                "tastings",
                "work",
                "meals",
            )
        )
        for r in reports:
            seed, turn, b, h, t, arm, task, first, follow, answer, second = metadata[
                int(r["press_sentence"])
            ]
            if int(r["turn"]):
                task, first, answer = "followup", follow, second
            w.writerow(
                (
                    seed,
                    turn,
                    b,
                    h,
                    t,
                    arm,
                    task,
                    first,
                    answer,
                    r["said"],
                    int(answer == r["said"]),
                    *[
                        r[k]
                        for k in (
                            "recognized",
                            "mask",
                            "demonstrations",
                            "tastes",
                            "work",
                            "meals",
                        )
                    ],
                )
            )
    expected = a.stations * 4 * 6 * 8
    if len(reports) != expected:
        raise RuntimeError("incomplete pressure transcripts")
    print("wheel_dialogue", len(reports), "independently graded utterances")
    for r in rows(log, "press_language"):
        if r["press_language"] == "both":
            print(r["task"], r["correct"], "/", r["questions"])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--speaker", required=True, type=Path)
    p.add_argument("--peer", type=Path, default=ROOT / "examples/loam-a.mind")
    p.add_argument(
        "--save", type=Path, help="Teach a new vocabulary and retain this pupil"
    )
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--register", choices=("river", "fen"), default="river")
    p.add_argument("--seed", type=int, default=44371)
    p.add_argument("--test-seed", type=int, default=844799)
    p.add_argument("--stations", type=int, default=32)
    a = p.parse_args()
    if a.stations < 1:
        p.error("need at least one station")
    if a.save:
        teach(a)
    interrogate(a)


if __name__ == "__main__":
    main()
