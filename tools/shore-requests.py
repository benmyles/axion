#!/usr/bin/env python3
"""Use the creature's own acoustic categories to request different products of the same unbuilt field."""

import argparse
import csv
import subprocess
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--axion", default="./axion")
p.add_argument("--mind", default="examples/shore-reader.mind")
p.add_argument("--first", type=int, default=111)
p.add_argument("--count", type=int, default=16)
p.add_argument("--output", type=Path, required=True)
p.add_argument("--phrasing", choices=["west", "east-west"], default="west")
p.add_argument(
    "--autonomous", action="store_true", help="Let a spoken request trigger self-terminated construction."
)
a = p.parse_args()
names = "moth vole newt wren hare seal lark toad".split()


def talk(commands):
    result = subprocess.run(
        [a.axion, "talk", "--speaker", str(a.mind)],
        input="\n".join([*commands, "/quit", ""]),
        text=True,
        capture_output=True,
        check=True,
    )
    events = [
        dict(v.split("=", 1) for v in line.split())
        for line in result.stdout.splitlines()
        if line.startswith("shore_event=")
    ]
    return events, result.stdout


with a.output.open("w", newline="") as f:
    writer = csv.writer(f, lineterminator="\n")
    writer.writerow(
        [
            "seed",
            "deposit",
            "mode",
            "word",
            "question",
            "located",
            "answered",
            "before",
            "after",
            "cost",
            "harvest_a",
            "harvest_b",
            "target_only",
        ]
    )
    for seed in range(a.first, a.first + a.count):
        seat = f"/shore {seed} 4"
        events, _ = talk([seat, *["/build-for " + n for n in names]])
        left, right = map(int, (events[0]["region_a"], events[0]["region_b"]))
        permits = {int(e["certain"]): n for n, e in zip(names, events[1:]) if e.get("resolved") == "1"}
        for j, site in enumerate((left, right)):
            word = permits.get(1 << site, "")
            other = (right, left)[j]
            other_name = permits.get(1 << other, "")
            question = ""
            if other_name and site % 8 != other % 8:
                question = (
                    f"who is west of {other_name}?" if site % 8 < other % 8 else f"who is {other_name} west of?"
                )
                if a.phrasing == "east-west" and site % 8 > other % 8:
                    question = f"who is east of {other_name}?"
            for mode in ("name", "question"):
                if not word or (mode == "question" and not question):
                    writer.writerow([seed, j, mode, word, question, 0, 0, 0, 0, 0, 0, 0, 0])
                    continue
                if a.autonomous:
                    request = word if mode == "name" else question
                    ev, log = talk(
                        [
                            seat,
                            "/shore-requests on",
                            "/imagine-shore",
                            request,
                            "/imagine-shore",
                            "/expose-shore 256",
                        ]
                    )
                else:
                    request = "/build-for " + word if mode == "name" else "/build-answer " + question
                    ev, log = talk(
                        [seat, request, "/imagine-shore", "/build-shore 12", "/imagine-shore", "/expose-shore 256"]
                    )
                answers = [v for v in ev if v["shore_event"] == "build-for"]
                recognized = int(
                    bool(answers) and answers[-1]["resolved"] == "1" and int(answers[-1]["certain"]) == 1 << site
                )
                said = int(any(line == "animal> " + word for line in log.splitlines()))
                before, after = [r for r in ev if r["shore_event"] == "imagine-shore"]
                built = next((r for r in ev if r["shore_event"] in ("build-shore", "pursue-shore")), {"work": 0})
                measure = ev[-1]
                out = [int(measure["detached_a"]), int(measure["detached_b"])]
                writer.writerow(
                    [
                        seed,
                        j,
                        mode,
                        word,
                        question,
                        recognized,
                        said,
                        int(before["projected_sites"]),
                        int(after["projected_sites"]),
                        int(built["work"]),
                        *out,
                        int(out[j] > 0 and not out[1 - j]),
                    ]
                )
        f.flush()
