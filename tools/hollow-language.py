#!/usr/bin/env python3
"""Ground direction words in physical placements; grade later questions from independent geometry."""

import argparse
import csv
import importlib.util
import random
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("wheel", ROOT / "tools/press-language.py")
wheel = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wheel)
BATH, TOOL, TONE = wheel.vocabulary("river")
NAMES = {"bank": ("ahead", "right", "behind", "left"), "reed": ("fore", "star", "aft", "port")}


def questions(hue, bath):
    return (
        f"where did {TOOL[hue]} rest",
        f"which way for {BATH[bath]}",
        "where is it now",
        f"where is {TOOL[hue]} for {BATH[bath]}",
    )


def teach(a):
    directions = NAMES[a.register]
    rng = random.Random(a.seed)
    commands = ["/marsh 231"]
    for round_id in range(8):
        # Named fourth tool and bath are never presented at a spatial lesson.
        cases = [(b, h, d) for b in range(3) for h in range(3) for d in range(4)]
        rng.shuffle(cases)
        for b, h, d in cases:
            commands += [
                f"/marsh-show {b} {h} {d}",
                "/marsh-caption "
                + (
                    f"{TOOL[h]} by {BATH[b]} lay {directions[d]}"
                    if round_id % 2
                    else f"{directions[d]} past {TOOL[h]} in {BATH[b]}"
                ),
            ]
    for _ in range(36):
        order = list(range(4))
        rng.shuffle(order)
        for kind in order:
            b, h, d = rng.randrange(3), rng.randrange(3), rng.randrange(4)
            commands.extend(
                (f"/marsh-show {b} {h} {d}", f"/marsh-example {questions(h, b)[kind]} | {directions[d]}")
            )
    log = wheel.talk(a, a.speaker, commands, a.save)
    contacts = [r for r in wheel.rows(log, "hollow_contact") if r["hollow_contact"] == "10"]
    actual = [(int(r["hue"]), int(r["seen"])) for r in contacts]
    expected = [tuple(map(int, s.split()[2:])) for s in commands if s.startswith("/marsh-show")]
    if actual != expected:
        raise RuntimeError("spatial captions require an actual witnessed surface and direction")
    rows = wheel.rows(log, "hollow_caption")
    if not rows or any(r["hollow_caption"] != "1" for r in rows):
        raise RuntimeError(
            f"accepted {sum(r['hollow_caption'] == '1' for r in rows)} / {len(rows)} captions"
        )
    print(f"{len(contacts)} positions witnessed, {len(rows)} captions retained or superseded")


def terrain(seed):
    deck = list(range(6))
    for i in range(5, 0, -1):
        j = wheel.mix(seed + 1169 + i * 7919) % (i + 1)
        deck[i], deck[j] = deck[j], deck[i]
    points = []
    for i, ring in enumerate(deck):
        k = wheel.mix(seed + i * 951)
        x = (0, 1, 1, 0, -1, -1)[ring] * (4 + (k & 3)) + (k >> 12) % 3 - 1
        y = (-1, -1, 1, 1, 1, -1)[ring] * (4 + (k >> 2 & 3)) + (k >> 15) % 3 - 1
        for _ in range(seed >> 9 & 3):
            x, y = -y, x
        points.append((x, y))
    b = wheel.mix(seed + 853) & 3
    notes, water = wheel.native(seed)
    gate = (water[b] + 1 + wheel.mix(seed + 457) % 3) & 3
    return points, b, notes, notes.index(gate)


def pointing(here, goal, facing, directions):
    dx, dy = goal[0] - here[0], goal[1] - here[1]
    options = []
    if dx:
        options.append(1 if dx > 0 else 3)
    if dy:
        options.append(2 if dy > 0 else 0)
    return ",".join(directions[(d - facing) % 4] for d in options) or "_"


def script(a):
    rng = random.Random(a.test_seed)
    arms = {
        "walked": 0,
        "visible": 1,
        "prior_water": 2,
        "erased": 3,
        "dry_unseen": 4,
        "water_unsampled": 5,
        "still_odometer": 6,
        "skewed_odometer": 7,
        "landmark_hidden": 8,
        "false_landmark": 9,
        "one_cell": 10,
        "lost_topic": 17,
    }
    headings = NAMES[a.register]
    for j in range(a.stations):
        seed = rng.randrange(1, 2**32)
        points, b, notes, revalued = terrain(seed)
        chosen = rng.randrange(4)
        angle = rng.randrange(4)
        gap = rng.randint(3, 9)
        face = rng.randrange(4)
        rotation = rng.randint(1, 3)
        x, y = points[chosen]
        x += gap * ((angle == 1) - (angle == 3))
        y += gap * ((angle == 2) - (angle == 0))
        for arm, mode in arms.items():
            # Identical body, floor, route and questions in every intervention.
            # The detour can be relative to a different tool than the query.
            for h, task, prompt in (
                (chosen, "axial", questions(chosen, b)[0]),
                ((chosen + 1) % 4, "other_tool", questions((chosen + 1) % 4, b)[0]),
                (revalued, "revalued", questions(revalued, b)[1]),
                (revalued, "both", questions(revalued, b)[3]),
                ((revalued + 1) % 4, "conflict", questions((revalued + 1) % 4, b)[3]),
            ):
                gold = [
                    pointing((x, y), points[h], face, headings),
                    pointing((x, y), points[h], (face + rotation) % 4, headings),
                    TONE[notes[h]],
                ]
                if task == "conflict" or gold[0] == "_":
                    gold = ["_"] * 3
                if arm == "lost_topic":
                    gold[1:] = ["_", "_"]
                yield (
                    f"{seed} {mode} {rotation} {chosen} {angle} {gap} {face} | {prompt} | where is it now | "
                    f"what note does it give | {arm} {task} {' '.join(gold)}"
                )


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--speaker", type=Path, required=True)
    p.add_argument("--peer", type=Path, default=ROOT / "examples/loam-a.mind")
    p.add_argument("--save", type=Path)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--seed", type=int, default=74321)
    p.add_argument("--test-seed", type=int, default=975223)
    p.add_argument("--stations", type=int, default=48)
    p.add_argument("--register", choices=NAMES, default="bank")
    a = p.parse_args()
    if a.save:
        teach(a)
    with tempfile.TemporaryDirectory(prefix="axion-marsh-") as tmp:
        path = Path(tmp) / "questions"
        path.write_text("\n".join(script(a)) + "\n")
        result = wheel.talk(a, a.save or a.speaker, ["/marsh 1", f"/marsh-tests {path}"])
    rows = wheel.rows(result, "hollow_language")
    if not rows:
        raise RuntimeError("no external grades")
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with a.output.open("w", newline="") as f:
        w = csv.DictWriter(f, rows[0])
        w.writeheader()
        w.writerows(rows)
    for r in rows:
        if r["hollow_language"] == "visible":
            print(r)


if __name__ == "__main__":
    main()
