#!/usr/bin/env python3
"""Repeat the watershed, foster and random-founder studies on the GPU and tabulate all paired conditions."""

import argparse
import csv
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
FOUNDER_SEEDS = (901, 5717, 9029, 13001)
FOSTER_SEEDS = (4147, 7489, 15649, 28643)


def cases(out):
    loose = ROOT / "examples/driftbank.flood"
    ancestor = out / "ancestral.flood"
    base = "--shoal-span 4 --shoal-ideas 128 --shoal-moves 10 --shoal-exposure 128".split()
    juvenile = "--shoal-span 4 --shoal-ideas 64 --shoal-moves 8 --shoal-exposure 96".split()
    wild = "--side 32 --neurons 256 --density 36 --grains 60 --construction-tide 8".split()
    for name, policy, ticks, trials, count in (
        ("live", "active", 512, 8, 768),
        ("quiet", "quiet", 512, 8, 768),
        ("ancestral", "active", 1024, 16, 1024),
    ):
        yield (
            name,
            "driftbank",
            policy,
            0,
            [
                "--load",
                loose,
                "--rest-hands",
                "--worksites",
                policy,
                "--steps",
                ticks,
                "--report-every",
                ticks // 4,
                "--field-transfer",
                trials,
                "--work-subjects",
                count,
                *base,
                "--assay-inquiry",
            ],
        )
    for seed in FOSTER_SEEDS:
        for arm in ("ancestral", "ancestral-shuffled", "washed"):
            wash = arm == "washed"
            args = [
                "--side",
                64,
                "--neurons",
                512,
                "--seed",
                seed,
                "--density",
                48,
                "--worksites",
                "quiet",
                "--hands",
                "still",
                "--site-replay",
                "off",
                "--colonists",
                ancestor,
                "--soil",
                ancestor,
                "--work-traces",
                "ancestral" if wash else arm,
                "--steps",
                16,
                "--field-min-age",
                16,
                "--field-max-age",
                16,
                "--field-transfer",
                16,
                "--work-subjects",
                1024,
                *juvenile,
                "--work-test-seed",
                seed ^ 12133,
            ]
            if wash:
                args += ["--wash-sites"]
            yield f"foster-{seed}-{arm}", "foster", arm, seed, args
    for kind in ("mixed", "bound", "scattered"):
        yield (
            f"finite-{kind}",
            "sediment",
            kind,
            0,
            [
                "--load",
                ancestor,
                "--rest-hands",
                "--substrate",
                kind,
                "--steps",
                4096,
                "--report-every",
                1024,
                "--assay-inquiry",
                "--field-transfer",
                8,
                "--work-subjects",
                768,
                *base,
            ],
        )
    for seed in FOUNDER_SEEDS:
        for arm in ("active", "quiet"):
            yield (
                f"untaught-{seed}-{arm}",
                "random founders",
                arm,
                seed,
                [
                    *wild,
                    "--seed",
                    seed,
                    "--worksites",
                    arm,
                    "--steps",
                    8192,
                    "--report-every",
                    2048,
                    "--field-transfer",
                    8,
                    "--work-subjects",
                    512,
                    *base,
                    "--work-test-seed",
                    seed ^ 7511,
                    "--assay-inquiry",
                ],
            )
        for arm in ("learned", "unbiased"):
            yield (
                f"bedrock-{seed}-{arm}",
                "random founders on bound sediment",
                arm,
                seed,
                [
                    *wild,
                    "--seed",
                    seed,
                    "--worksites",
                    "active",
                    "--substrate",
                    "bound",
                    "--press-choice",
                    arm,
                    "--steps",
                    32768,
                    "--report-every",
                    4096,
                    "--field-transfer",
                    16,
                    "--work-subjects",
                    1024,
                    *base,
                    "--work-test-seed",
                    seed ^ 23153,
                    "--assay-inquiry",
                ],
            )


def write_table(path, rows):
    columns = dict.fromkeys(k for row in rows for k in row)
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=columns, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "--output", type=Path, required=True, help="Directory for logs, resumable habitat files, and result tables"
    )
    p.add_argument("--axion", type=Path, default=ROOT / "axion")
    p.add_argument("--summarize", action="store_true", help="Only read the existing logs in the output directory")
    a = p.parse_args()
    out = a.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    worlds = []
    transfer = []
    fostering = []
    for name, source, arm, seed, args in cases(out):
        log = out / (name + ".log")
        if not a.summarize:
            with log.open("w") as target:
                subprocess.run(
                    [str(a.axion.resolve()), "floodplain", *map(str, args), "--save", str(out / (name + ".flood"))],
                    cwd=ROOT,
                    stdout=target,
                    check=True,
                )
        rows = [
            dict(word.split("=", 1) for word in line.split() if "=" in word)
            for line in log.read_text().splitlines()
        ]

        def last(tag):
            return next((r for r in reversed(rows) if tag in r), {})

        identity = dict(run=name, origin=source, condition=arm, founder_seed=seed or "")
        info = dict(identity)
        for tag, keys in (
            ("flood_tick", "flood_tick living neurons births deaths moved"),
            ("flood_census", "distinct_bodies deepest"),
            (
                "worksite_tick",
                "floor live_cargo vacant_cargo released contacts assays products pumped eaten marked predating_birth",
            ),
            ("substrate_tick", "bound presses fed distinct guided"),
            ("inquiry_replay", "changed_inputs different_spikes changed_motors"),
            ("flood_seconds", "site_ticks simulation_seconds simulation_Msite_ticks_per_s"),
        ):
            record = last(tag)
            for key in keys.split():
                if key in record:
                    info[key] = record[key]
        worlds.append(info)
        for r in rows:
            if "field_transfer" not in r:
                continue
            transfer.append({**identity, **r})
            if source == "foster" and r["field_transfer"] == "lived":
                fostering.append({**identity, "predating_birth": info["predating_birth"], **r})
    write_table(out / "foraging.csv", worlds)
    write_table(out / "field-transfer.csv", transfer)
    write_table(out / "fostering.csv", fostering)


if __name__ == "__main__":
    main()
