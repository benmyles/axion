#!/usr/bin/env python3
"""Ground reports of the animal's own actions, then score consultations in physically opposite baths."""

import argparse
import csv
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--speaker", required=True, type=Path)
    p.add_argument("--listener", type=Path, default=ROOT / "examples/shore-reader.mind")
    p.add_argument(
        "--save", type=Path, help="Teach two additional action reports and save the resulting individual"
    )
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--tables", type=int, default=512)
    p.add_argument("--seeds", nargs="+", type=int, default=[604187, 1069399, 1864267, 2046869])
    p.add_argument("--components", nargs="+", type=int, choices=[2, 3, 4, 5], default=[2, 5])
    a = p.parse_args()
    a.output.parent.mkdir(parents=True, exist_ok=True)

    def talk(speaker, script, save=None):
        argv = [a.axion.resolve(), "talk", "--speaker", speaker.resolve(), "--listener", a.listener.resolve()]
        if save:
            argv += ["--save-speaker", save.resolve()]
        run = subprocess.run(
            argv, cwd=ROOT, input="\n".join([*script, "/quit", ""]), text=True, capture_output=True
        )
        if run.returncode:
            raise RuntimeError(run.stderr + run.stdout[-800:])
        return run.stdout.splitlines()

    def rows(lines, tag):
        return [
            dict(v.split("=", 1) for v in line.split() if "=" in v) for line in lines if line.startswith(tag + "=")
        ]

    source = a.speaker
    if a.save:
        names = {name: 0 for name in ("moth", "vole", "newt", "wren")}
        lessons = []
        for start in range(131, 211, 10):
            catalog = rows(talk(source, [f"/assay-table {k}" for k in range(start, start + 10)]), "field_table")
            for scene in catalog:
                for chip in range(2):
                    word = scene[f"chip{chip}"]
                    if word not in names or names[word] >= 4:
                        continue
                    seed = int(scene["field_table"])
                    lessons += [
                        f"/brine-table {seed} {seed * 487 + chip * 103} 5",
                        f"/touch-coupons {chip}",
                        f"/try-grain {chip}",
                        f"/show-choice {chip} taken where would you try {word}?",
                        f"/show-choice {chip} other where would you avoid {word}?",
                    ]
                    names[word] += 1
            if min(names.values()) == 4:
                break
        if min(names.values()) < 3:
            raise RuntimeError("insufficient grounded names for training")
        session = talk(source, lessons, a.save)
        motions = rows(session, "brine_motion")
        examples = rows(session, "field_question")
        if (
            len(examples) != 2 * len(motions)
            or not all(int(r["paid"]) for r in motions)
            or not all(r["accepted"] == "1" for r in examples)
        ):
            raise RuntimeError("a pointed action lesson lacked a paid movement or a unique referent")
        source = a.save
        print(f"Taught {len(examples)} action reports after {len(motions)} physical choices.", flush=True)
    results = []
    for seed in a.seeds:
        for ions in a.components:
            commands = [
                f"/brine-table 4001 99 {ions}",
                f"/brine-dialogues {a.tables} {ions} 9 {seed} fact",
                f"/brine-dialogues {a.tables} {ions} 9 {seed} choice",
            ]
            records = rows(talk(source, commands), "brine_dialogue")
            if len(records) != 16:
                raise RuntimeError("missing dialogue controls")
            results.extend(records)
    with a.output.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=results[0], lineterminator="\n")
        writer.writeheader()
        writer.writerows(results)


if __name__ == "__main__":
    main()
