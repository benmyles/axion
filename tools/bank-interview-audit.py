#!/usr/bin/env python3
"""Audit spoken inquiries, removals and withdrawals through the ordinary Axion conversation interface."""

import argparse
import csv
import gzip
import io
import os
import subprocess
import tempfile
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NAMES = ("amber", "slate", "flint", "ivory")


def mix(value):
    value &= 0xFFFFFFFF
    value = (value ^ (value >> 16)) * 0x21F0AAAD & 0xFFFFFFFF
    value = (value ^ (value >> 15)) * 0x735A2D97 & 0xFFFFFFFF
    return value ^ (value >> 15)


def pools(world, round_number):
    incident = mix(world + 1879 * round_number)
    rims = mix(world + 7771)
    order = (incident >> 11) & 1
    # The inlet has already been diverted once in every audited conversation.
    upstream = (mix(world + 3467) >> 5 & 1) ^ 1
    loads = (incident >> 3 & 1, incident >> 7 & 1)
    t0 = rims & 3
    tags = (t0, (t0 + 1 + (rims >> 2) % 3) & 3)
    waves = tuple(
        2 * tags[side ^ order]
        + (loads[side ^ order] ^ (rims >> (8 + (side ^ order)) & 1))
        for side in (0, 1)
    )
    return order ^ upstream ^ (1 - loads[upstream]), order ^ upstream, waves


def events(log):
    public = {}
    for line in log.splitlines():
        if line.startswith(("animal> ", "worker> ")):
            person, speech = line.split("> ", 1)
            public[person] = speech
        if line.startswith("bank_talk="):
            state = {k: int(v) for k, v in (x.split("=", 1) for x in line.split())}
            assert (
                state["source"] + state["left"] + state["right"] + state["consumed"]
                == 8192
            )
            yield state, public
            public = {}


def collect(args, world, arm, stock_names, ear_names, pitches):
    commands = [
        f"/backwater {world} crossed",
        f"/backwater-work {args.exposure}",
        "/backwater-divert",
        f"/backwater-work {args.exposure}",
    ]
    reply = "/backwater-pass" + (" " + arm if arm != "heard" else "")
    for _ in range(args.turns):
        commands.extend(
            (
                "choose",
                reply,
                "why",
                *("without " + n for n in NAMES),
                "/backwater-pull",
            )
        )
    cli = [args.binary, "talk", "--speaker", args.speaker, "--listener", args.worker]
    run = subprocess.run(
        cli,
        input="\n".join(commands + ["/backwater-exit", "/quit"]) + "\n",
        text=True,
        capture_output=True,
        check=True,
    )
    records = list(events(run.stdout))
    if len(records) != len(commands):
        raise ValueError("the conversation did not produce every physical observation")
    assert records[1][0]["pulls"] == records[3][0]["pulls"] == args.exposure
    for trial in range(args.turns):
        i = 4 + 8 * trial
        question, retelling, reason, *tail = records[i : i + 8]
        removals = tail[:4]
        withdrawal = tail[4][0]
        q, spoken = question
        named = spoken["animal"].split()
        assert (
            len(named) == 2
            and named[0] == "why"
            and named[1] in NAMES
            and q["inquiries"] == 1
        )
        rich, upstream, waves = pools(world, q["round"])
        assert q["round"] == 2 * args.exposure + trial and (q["left"], q["right"]) == (
            (6, 0) if rich == 0 else (0, 6)
        )
        asked = q["request"]
        wave = waves[asked]
        assert q["upstream"] == (asked == upstream)
        assert ear_names.setdefault(wave >> 1, named[1]) == named[1]
        r, told = retelling
        if arm == "mute":
            assert "worker" not in told and r["reports"] == 0 and r["waveform"] == 8
        else:
            report = told["worker"].split()
            sounded = wave ^ (arm == "reversed")
            assert (
                len(report) == 2
                and report[0] == named[1]
                and r["reports"] == 1
                and r["waveform"] == sounded
            )
            assert pitches.setdefault(sounded & 1, report[1]) == report[1]
        move = r["choice"]
        words = told["animal"].split()
        assert (
            len(words) == 2
            and words[0] == "turn"
            and stock_names.setdefault(move, words[1]) == words[1]
        )
        alternatives = {n: item[1]["animal"] for n, item in zip(NAMES, removals)}
        assert all(item[0]["understood"] == 1 for item in removals)
        assert all(
            v == told["animal"] for n, v in alternatives.items() if n != named[1]
        )
        changed = alternatives[named[1]] != told["animal"]
        stated = reason[1]["animal"]
        assert reason[0]["cited"] == changed
        assert stated == (told["worker"] if changed else "?")
        paid = withdrawal["food"]
        assert paid == (6 if move == rich else 0)
        assert withdrawal["pulls"] == withdrawal["paid"] == 1
        yield {
            "world": world,
            "channel": arm,
            "round": q["round"],
            "source": upstream,
            "asked": asked,
            "inquiry": spoken["animal"],
            "physical impulse": wave,
            "received impulse": r["waveform"],
            "reply": told.get("worker", ""),
            "pronounced choice": told["animal"],
            "reached": move,
            "loaded pan": rich,
            "why": stated,
            "without requested source": alternatives[named[1]],
            "consumed": paid,
            "cost": q["paid"] + withdrawal["paid"],
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "axion")
    parser.add_argument(
        "--speaker", type=Path, default=ROOT / "examples/deepwater-speaker.mind"
    )
    parser.add_argument(
        "--worker", type=Path, default=ROOT / "examples/runnel-consultant.mind"
    )
    parser.add_argument("--worlds", type=int, default=16)
    parser.add_argument("--exposure", type=int, default=48)
    parser.add_argument("--turns", type=int, default=8)
    parser.add_argument("--seed", type=int, default=53719)
    parser.add_argument(
        "--output", type=Path, required=True, help="new gzip-compressed TSV"
    )
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    if (
        not 1 <= args.turns <= 8192 // 6 - 2 * args.exposure
        or not 1 <= args.worlds <= 8192
        or not 1 <= args.exposure <= 127
    ):
        parser.error(
            "use 1–127 exposures, 1–8192 worlds and a positive turn count within the 8192-unit carbon reserve"
        )
    if args.output.exists():
        parser.error("output already exists")
    with tempfile.NamedTemporaryFile(
        prefix=".bank-dialogue-", dir=args.output.parent
    ) as partial:
        with (
            gzip.GzipFile(
                fileobj=partial, mode="wb", mtime=0, filename=""
            ) as compressed,
            io.TextIOWrapper(compressed, encoding="utf-8", newline="") as table,
        ):
            writer = None
            motors = {}
            rims = {}
            pitches = {}
            totals = {}
            for i in range(args.worlds):
                world = mix(args.seed + i * 5507)
                for arm in ("heard", "mute", "reversed"):
                    results = totals.setdefault(arm, Counter())
                    for r in collect(args, world, arm, motors, rims, pitches):
                        if writer is None:
                            writer = csv.DictWriter(
                                table, r.keys(), delimiter="\t", lineterminator="\n"
                            )
                            writer.writeheader()
                        writer.writerow(r)
                        results["reaches"] += 1
                        results["correct"] += r["consumed"] > 0
                        results["asked upstream"] += r["source"] == r["asked"]
                        results["carbon"] += r["consumed"]
                        results["cited"] += r["why"] != "?"
            assert (
                len(set(motors.values())) == len(motors)
                and len(set(rims.values())) == len(rims)
                and len(set(pitches.values())) == len(pitches)
            )
        partial.flush()
        os.link(partial.name, args.output)
    for arm, counts in totals.items():
        print(arm, dict(counts))
    print("Public source names", rims, "pitch names", pitches, "motor names", motors)


if __name__ == "__main__":
    main()
