#!/usr/bin/env python3
"""Teach two question forms through physical examples, then test new nouns and independent listeners."""

import argparse
import csv
from pathlib import Path
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--axion", default="./axion")
p.add_argument("--speaker", type=Path, required=True)
p.add_argument("--listener", type=Path, default=Path("examples/shore-reader.mind"))
p.add_argument("--save", type=Path, required=True)
p.add_argument("--output", type=Path, required=True)
p.add_argument("--scenes", type=int, default=48)
args = p.parse_args()


def talk(speaker, lines, save=None):
    command = [args.axion, "talk", "--speaker", speaker, "--listener", args.listener]
    if save:
        command += ["--save-speaker", save]
    result = subprocess.run(command, input="\n".join([*lines, "/quit", ""]), capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stderr + "\n" + result.stdout[-500:])
    return result.stdout.splitlines()


def data(line):
    return dict(w.split("=", 1) for w in line.split() if "=" in w)


def catalog(seeds):
    cups = {}
    for i in range(0, len(seeds), 6):
        commands = []
        for seed in seeds[i : i + 6]:
            commands += [f"/assay-table {seed}", "/touch-coupons 0", "/touch-coupons 1"]
        for line in talk(args.speaker, commands):
            if line.startswith("field_table="):
                row = data(line)
                c = cups[int(row["field_table"])] = dict(table=row, true=[None, None])
            if line.startswith("field_coupon="):
                row = data(line)
                assert row["observed"] == "1" and int(row["spent"]) > 0
                c["true"][int(row["field_coupon"])] = int(row["result"])
    return cups


def phrase(kind, word):
    return f"which soil {'feeds' if kind == 0 else 'resists'} {word}?"


tutored = set("moth vole newt wren".split())
holdout = set("hare seal lark toad".split())
training = catalog(list(range(50, 86)))
lessons = []
n = {}
selected = 0
for seed, entry in training.items():
    row = entry["table"]
    for j, outcome in enumerate(entry["true"]):
        name = row[f"chip{j}"]
        if name not in tutored or outcome not in (1, 2) or n.get(name, 0) >= 4:
            continue
        pos = int(outcome == 2)
        lessons += [
            f"/assay-table {seed}",
            f"/touch-coupons {j}",
            f"/show-question {j} {pos} {phrase(0, name)}",
            f"/show-question {j} {pos ^ 1} {phrase(1, name)}",
        ]
        n[name] = n.get(name, 0) + 1
        selected += 1
assert selected >= 8 and set(n) == tutored, n
raised = talk(args.speaker, lessons, args.save)
accepted = [data(l) for l in raised if l.startswith("field_question=")]
assert len(accepted) == selected * 2 and all(r["accepted"] == "1" for r in accepted), accepted
seeds = list(range(4001, 4001 + args.scenes))
external = catalog(seeds)
with args.output.open("w", newline="") as f:
    writer = csv.writer(f, lineterminator="\n")
    writer.writerow(
        "seed chip name predicate intervention expected answer model_site target_site recipient_site recipient_work food grounded".split()
    )
    for mode in ("open", "foil", "quiet", "erased"):
        for start in range(0, len(seeds), 6):
            commands = []
            expected = []
            for seed in seeds[start : start + 6]:
                entry = external[seed]
                row = entry["table"]
                commands += [f"/assay-table {seed}", f"/listen-field {'open' if mode == 'erased' else mode}"]
                if mode == "erased":
                    commands.append("/field-memory blank")
                for j, truth in enumerate(entry["true"]):
                    name = row[f"chip{j}"]
                    if name not in holdout:
                        continue
                    for k in (0, 1):
                        mask = truth if k == 0 else 3 ^ truth
                        target = 0 if mask == 1 else 1 if mask == 2 else 2
                        grounded = target < 2 and row[f"bed{target}"] != "_"
                        expected.append([seed, j, name, k, row, target, grounded])
                        commands.append(phrase(k, name))
            if not expected:
                continue
            heard = [data(l) for l in talk(args.save, commands) if l.startswith("field_answer=")]
            assert len(heard) == len(expected), (len(heard), len(expected))
            for (seed, j, name, k, row, target, grounded), r in zip(expected, heard):
                correct = row[f"bed{target}"] if grounded else "_"
                site = int(row[f"site{target + 2}"]) if grounded else 32
                writer.writerow(
                    [
                        seed,
                        j,
                        name,
                        k,
                        mode,
                        correct,
                        r["field_answer"],
                        r["referred_bed"],
                        site,
                        r["receiver_site"],
                        r["receiver_work"],
                        r["receiver_food"],
                        int(grounded),
                    ]
                )
        f.flush()
print(
    "Taught",
    len(accepted),
    "pointed questions with four noun forms and tested the other four on",
    args.scenes,
    "unseen tables.",
)
