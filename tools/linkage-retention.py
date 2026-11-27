#!/usr/bin/env python3
"""Measure interference and reacquisition in the same portable synapses on two material laws."""

import argparse
import csv
import hashlib
import importlib.util
import os
import random
import subprocess
import tempfile
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "linkage_language", Path(__file__).with_name("linkage-language.py")
)
rack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rack)


def experiment(options):
    rng = random.Random(options.seed)
    worlds = options.worlds
    questions = []
    for j in range(options.cases):
        key = rng.getrandbits(32)
        for world, arm in zip(worlds, ("first_world", "second_world")):
            r = rack.fixture(world, key, 4)
            prompt = r.question(r.goal, (j % 4,), rng)
            revised = r.question(r.goal, range(4), rng)
            cost = r.shortest(r.goal, r.mask)
            questions.append(
                f"{world} {key} 4 0 0 | {prompt} | {revised} | why | {arm} {r.mask} {r.goal} {cost}"
            )
    options.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="axion-memory-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as tmp:
        d = Path(tmp)
        path = d / "questions.txt"
        path.write_text("\n".join(questions) + "\n")
        checkpoints = {"blank": options.speaker.resolve()}
        sequence = [
            ("blank", None, 0, 0),
            ("first", "blank", worlds[0], 2),
            ("repeated", "first", worlds[0], 5),
            ("competing", "first", worlds[1], 2),
            ("after_repeats", "repeated", worlds[1], 2),
            ("returned", "competing", worlds[0], 2),
        ]
        rows = []
        for stage, parent, world, passes in sequence:
            trace = d / f"{stage}.tsv"
            speaker = checkpoints[parent or "blank"]
            # The batch clones the current resident, with zero further elementary handling in either world.
            # The same held-out requests are used throughout this matched interference comparison.
            text = f"/rack {world or worlds[0]}\n"
            if passes:
                text += f"/rack-study {passes}\n"
            text += f'/rack-trials "{path}" "{trace}"\n/quit\n'
            cmd = [
                options.axion.resolve(),
                "talk",
                "--speaker",
                speaker,
                "--listener",
                ROOT / "examples/causeway-guide.mind",
            ]
            if parent:
                checkpoints[stage] = d / f"{stage}.mind"
                cmd += ["--save-speaker", checkpoints[stage]]
            completed = subprocess.run(
                cmd, input=text, capture_output=True, text=True, check=False
            )
            if completed.returncode:
                raise RuntimeError(completed.stderr + completed.stdout[-1400:])
            bank = hashlib.sha256(checkpoints[stage].read_bytes()[-172:-8]).hexdigest()
            for arm, c in rack.audit(SimpleNamespace(path=trace)).items():
                rows.append(dict(stage=stage, bank=bank, world=arm, **c))
        with (options.output / "retention.csv").open("w", newline="") as out:
            writer = csv.DictWriter(
                out,
                list(dict.fromkeys(k for row in rows for k in row)),
                lineterminator="\n",
            )
            writer.writeheader()
            writer.writerows({key:row.get(key,0) for key in writer.fieldnames} for row in rows)


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--seed", type=int, default=84139)
    p.add_argument("--speaker", type=Path, default=ROOT / "examples/rack-guide.mind")
    p.add_argument("--worlds", type=int, nargs=2, default=(3941, 17329))
    p.add_argument("--cases", type=int, default=256)
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    experiment(p.parse_args())
