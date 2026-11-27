#!/usr/bin/env python3
"""Ground five public gestures, then score verbal choices, reasons and amendments against physical work."""

import argparse
import csv
import importlib.util
import os
import random
import tempfile
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("wheel", ROOT / "tools/press-language.py")
wheel = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wheel)
BASINS, FRUITS, _ = wheel.vocabulary("river")
COMPONENTS = (*FRUITS, *BASINS)
DIRECTIONS = ("ahead", "right", "behind", "left")
GESTURES = ("switch", "offer", "contrast", "lift", "release", "surface", "roll")
WORDS = {
    "river": ("or", "choose", "why", "without", "go", "face", "turn"),
    "moor": ("til", "weigh", "cause", "except", "walk", "skin", "roll"),
}


def tariff(seed):
    base = wheel.permutation(wheel.mix(seed + 41))
    price = wheel.permutation(wheel.mix(seed + 971))
    old = [*(2 + 4 * c for c in base), *(-1 - c for c in price)]
    new = [
        *(2 + 4 * ((c + 1 + wheel.mix(seed + 371) % 3) % 4) for c in base),
        *(-1 - (c + 1 + wheel.mix(seed + 5113) % 3) % 4 for c in price),
    ]
    return old, new


def encode(raw):
    count = Counter(raw)
    food = sum(count[i] << (8 * i) for i in range(4))
    work = sum(count[4 + i] << (8 * i) for i in range(4))
    a = next((i for i in reversed(raw) if i < 4), 0)
    b = next((i - 4 for i in reversed(raw) if i >= 4), 0)
    return food, work, a + 4 * b


def expected(maps, lanes, ray, head):
    left = sum(maps[x] for x in lanes[0])
    right = sum(maps[x] for x in lanes[1])
    if left == right:
        return "_", "_", 2, False
    picked = int(right > left)
    diff = Counter(lanes[picked])
    diff.subtract(lanes[1 - picked])
    impacts = [maps[i] * diff[i] for i in range(8)]
    big = max(impacts)
    factor = COMPONENTS[impacts.index(big)] if big > 0 and impacts.count(big) == 1 else "_"
    return DIRECTIONS[(ray + 2 * picked - head) % 4], factor, picked, big > abs(right - left)


def school(a):
    forms = WORDS[a.register]
    commands = []
    rng = random.Random(a.seed)
    for _ in range(14):
        order = list(range(len(GESTURES)))
        rng.shuffle(order)
        for k in order:
            seed = rng.randrange(1, 2**32)
            # Every offered first tray has a fruit from its low two bits.
            hue = wheel.mix(seed + 1213) & 3
            commands.extend(
                [f"/ford {seed}", f"/ford-show {GESTURES[k]} {hue}", f"/ford-caption {forms[k]}"]
            )
    log = wheel.talk(a, a.speaker, commands, a.save)
    heard = wheel.rows(log, "effort_caption")
    if len(heard) != len(commands) // 3 or any(r["effort_caption"] != "1" for r in heard):
        raise RuntimeError("a public gesture failed to produce a grounded word")
    print(len(heard), "grounded operator-word demonstrations", flush=True)


def layouts(seed, how_many):
    rng = random.Random(seed)
    for j in range(how_many):
        key = rng.randrange(1, 2**32)
        old, now = tariff(key)
        ray = wheel.mix(key + 769) >> 8 & 3
        head = key >> 5 & 3
        for category in ("repeated", "new_mix", "changed", "tie"):
            for _ in range(12000):
                lanes = [
                    [rng.randrange(4) for _ in range(rng.randint(1, 4))]
                    + [4 + rng.randrange(4) for _ in range(rng.randint(1, 5))]
                    for _ in range(2)
                ]
                for lane in lanes:
                    rng.shuffle(lane)
                if category == "tie" and j % 2 == 0:
                    lanes[1] = rng.sample(lanes[0], len(lanes[0]))
                _, reason, go, flip = expected(now, lanes, ray, head)
                if category == "tie":
                    if go != 2:
                        continue
                    edit = rng.choice(lanes[0] + lanes[1])
                else:
                    if go == 2 or reason == "_" or not flip:
                        continue
                    focus = COMPONENTS.index(reason)
                    if category == "changed" and expected(old, lanes, ray, head)[2] in (2, go):
                        continue
                    if category == "repeated" and max(Counter(lanes[0] + lanes[1]).values()) < 3:
                        continue
                    if category == "new_mix" and len(set(lanes[0] + lanes[1])) < 5:
                        continue
                    edit = focus
                break
            else:
                raise RuntimeError(f"could not draw physical problem {category}")
            yield key, category, lanes, edit, 1 + rng.randrange(3)


def prompts(a):
    sep, choose, why, drop, repeat = WORDS[a.register][:5]
    arms = {
        "lived": 0,
        "prior_yield": 1,
        "prior_cost": 2,
        "erased": 3,
        "no_yield": 4,
        "no_cost": 5,
        "other_yield": 6,
        "other_cost": 7,
        "cleared_topic": 16,
    }
    for key, category, lanes, removed, turn in layouts(a.test_seed, a.stations):
        _, values = tariff(key)
        ray = wheel.mix(key + 769) >> 8 & 3
        head = key >> 5 & 3
        spoken = " ".join(
            [choose, *(COMPONENTS[i] for i in lanes[0]), sep, *(COMPONENTS[i] for i in lanes[1])]
        )
        reply, reason, _, _ = expected(values, lanes, ray, head)
        changed = [[i for i in side if i != removed] for side in lanes]
        amended = expected(values, changed, ray, (head + turn) % 4)[0]
        for arm, code in arms.items():
            reference = [reply, reason, amended, amended]
            if code & 16:
                reference[1:] = ["_"] * 3
            # Body acts in a physical scene whose supplies can differ from the spoken counterfactual.
            real = lanes if (key & 1) else [lanes[1], lanes[0]]
            scene = " ".join(str(k) for items in real for k in encode(items))
            yield (
                f"{key} {code} {turn} {scene} | {spoken} | {why} | {drop} {COMPONENTS[removed]} | {repeat} | "
                f"{arm} {category} {' '.join(reference)}"
            )


def slats(a):
    words = WORDS[a.register]
    rng = random.Random(a.test_seed + 4357)
    for j in range(a.stations):
        seed = rng.randrange(1, 2**32)
        k = wheel.mix(seed + 2347)
        pigments = wheel.permutation(wheel.mix(seed + 1699))
        handles = wheel.permutation(wheel.mix(seed + 7013))
        pose, hand = (k >> 4) & 3, (k >> 13) & 1
        length = 1 + j % 5
        seq = [rng.randrange(2) for _ in range(length)]
        shift = lambda btn, hand=hand: 1 if (btn ^ hand) else -1
        speak = lambda moves, handles=handles: " ".join(
            [words[5], *(part for i in moves for part in (words[6], BASINS[handles[i]]))]
        )
        second = [rng.randrange(2) for _ in range(1 + (j + 2) % 5)]
        work = j % 2
        for arm, condition in (("observed", 0), ("erased", 1), ("foreign", 2), ("one_handle", 5)):
            answer = BASINS[pigments[(pose + sum(map(shift, seq))) % 4]]
            after = BASINS[pigments[(pose + shift(work) + sum(map(shift, second))) % 4]]
            yield f"{seed} {condition} {work} | {speak(seq)} | {speak(second)} | {arm} length_{length} {answer} {after}"


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--speaker", type=Path, required=True)
    p.add_argument("--peer", type=Path, default=ROOT / "examples/loam-a.mind")
    p.add_argument("--save", type=Path)
    p.add_argument("--register", choices=WORDS, default="river")
    p.add_argument("--seed", type=int, default=73331)
    p.add_argument("--test-seed", type=int, default=519971)
    p.add_argument("--surfaces", type=Path)
    p.add_argument("--stations", type=int, default=96)
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()
    if a.save:
        school(a)
    with tempfile.TemporaryDirectory(
        prefix="axion-ford-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as folder:
        f = Path(folder) / "stimuli"
        f.write_text("\n".join(prompts(a)) + "\n")
        commands = ["/ford 1", f"/ford-tests {f}"]
        if a.surfaces:
            more = Path(folder) / "slats"
            more.write_text("\n".join(slats(a)) + "\n")
            commands.append(f"/ford-surfaces {more}")
        log = wheel.talk(a, a.save or a.speaker, commands)
    result = wheel.rows(log, "effort_language")
    if not result:
        raise RuntimeError("no spoken choice records")
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with a.output.open("w", newline="") as f:
        writer = csv.DictWriter(f, result[0], lineterminator="\n")
        writer.writeheader()
        writer.writerows(result)
    if a.surfaces:
        rows = wheel.rows(log, "effort_shape_language")
        if not rows:
            raise RuntimeError("no hypothetical surface replies")
        a.surfaces.parent.mkdir(parents=True, exist_ok=True)
        with a.surfaces.open("w", newline="") as f:
            writer = csv.DictWriter(f, rows[0], lineterminator="\n")
            writer.writeheader()
            writer.writerows(rows)
    for q in result:
        if q["effort_language"] == "lived":
            print(q)


if __name__ == "__main__":
    main()
