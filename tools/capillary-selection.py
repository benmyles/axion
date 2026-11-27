#!/usr/bin/env python3
"""Compare free-living watershed treatments using the same held-out material exam for each birth cohort."""

import argparse
import csv
import os
import subprocess
import statistics
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def selection_figure(census, destination, rates=()):
    from journal import Drawing, Codec, SANS, SERIF, MONO

    with census.open() as source:
        rows = list(csv.DictReader(source))
    groups = list(dict.fromkeys(r["habitat"] for r in rows))
    title = {
        "bank": "Brook watershed",
        "old": "Older watershed",
        "slow": "Slow-pulse nursery",
    }
    names = {
        "shared": "Starting cohort",
        "live": "Free handling",
        "watch": "Observe only",
        "fixed": "Fixed contacts",
        "lookup": "Experienced inputs only",
    }
    colors = {
        "shared": "#d2d9c4",
        "live": "#74bba8",
        "watch": "#d2a365",
        "fixed": "#ac96b9",
        "lookup": "#83a3c2",
    }
    paper = "#f2f0e6"
    ink = "#17342e"
    sub = "#49655e"
    muted = "#9fb5a7"
    grid = "#c6cfc0"
    picture = Drawing(3840, 2160)
    picture.rect(0, 0, 3840, 2160, paper)
    picture.rect(0, 0, 3840, 375, ink)
    picture.text(105, 92, "AXION  /  EVOLUTION JOURNAL", 39, "#9ed1b0", MONO)
    picture.text(103, 224, "What did the watershed select for?", 104, paper, SERIF)
    picture.text(
        107,
        312,
        "Offspring from differently reared populations encounter the same unfamiliar channels.",
        44,
        "#d1dfd1",
        SANS,
    )
    starts = [r for r in rows if r["arm"] == "shared"]
    ends = [r for r in rows if r["stage"] == "end-empty"]
    gap = 46
    left = 99
    width = (3840 - 2 * left - gap * (len(groups) - 1)) / len(groups)
    for col, group in enumerate(groups):
        x = left + col * (width + gap)
        start = next(r for r in starts if r["habitat"] == group)
        end = [r for r in ends if r["habitat"] == group]
        picture.text(x, 485, title[group], 62, ink, SERIF)
        picture.text(
            x,
            554,
            f"Tick {int(start['tick']):,}  →  {int(end[-1]['tick']):,}",
            34,
            sub,
            MONO,
        )
        picture.text(
            x, 660, "Residents identifying both material laws", 38, ink, SANS, 600
        )
        for idx, r in enumerate([start, *end]):
            arm = r["arm"]
            total = int(r["eligible_sample"])
            num = int(r["fully_correct"])
            fraction = num / max(1, total)
            y = 742 + idx * 169
            picture.text(x, y, names[arm], 38, ink, SANS, 600)
            picture.text(
                x + width - 48, y, f"{100 * fraction:.1f}%", 40, ink, MONO, anchor="end"
            )
            picture.rect(x, y + 23, width - 48, 32, "#dce1d3", 7)
            picture.rect(x, y + 23, (width - 48) * fraction, 32, colors[arm], 7)
            picture.text(x, y + 99, f"{num:,} of {total:,} sampled", 30, sub, MONO)
        y = 1624
        picture.rule(x, y, x + width - 48, y, grid, 3)
        live = next((r for r in end if r["arm"] == "live"), end[0])
        picture.text(
            x,
            y + 73,
            f"{int(live['unique_rules']):,} distinct local update rules",
            40,
            ink,
            SERIF,
        )
        picture.text(
            x,
            y + 131,
            f"{int(live['median_cells'])} median  /  {int(live['peak_cells'])} maximum spiking cells",
            28,
            sub,
            MONO,
        )
        picture.text(
            x,
            y + 189,
            f"Among sampled {names[live['arm']].lower()} survivors",
            29,
            sub,
            SANS,
        )
    picture.rect(0, 1882, 3840, 278, ink)
    picture.text(105, 1964, "Common examination", 43, paper, SERIF)
    picture.text(
        105,
        2026,
        "24 paired layouts per animal. Pressure contacts cleared before tuition. Body and expressed cortical state retained.",
        36,
        "#cbdcd0",
    )
    speed = (
        f"    ·    Median field speed {statistics.median(rates):.1f}M site-ticks/s"
        if rates
        else ""
    )
    total = sum(int(r["eligible_sample"]) for r in starts + ends)
    picture.text(
        105,
        2100,
        f"{total:,} sampled animal examinations    ·    At most 2,047 ticks old{speed}",
        30,
        muted,
        MONO,
    )
    Codec().write(picture, destination)


def study(output, binary, ticks, sample, starts, treatments):
    output.mkdir(parents=True, exist_ok=False)
    w = output / "watersheds"
    d = output / "evidence"
    w.mkdir()
    d.mkdir()
    b = binary
    environment = {**os.environ, "AXION_JOURNAL_DIR": str(output / "journal")}
    archives = {"bank": "nursery", "slow": "slow-nursery", "old": "older-nursery"}
    for name, _ in starts:
        with (w / (name + "-new.flood")).open("xb") as f:
            subprocess.run(
                [
                    "zstd",
                    "-qdc",
                    ROOT / f"examples/capillary-{archives[name]}.flood.zst",
                ],
                stdout=f,
                check=True,
            )

    def run(tag, args):
        with (d / (tag + ".log")).open("w") as f:
            r = subprocess.run(
                [b, "floodplain", *map(str, args)],
                stdout=f,
                stderr=subprocess.STDOUT,
                env=environment,
            )
        s = (d / (tag + ".log")).read_text()
        if r.returncode:
            raise RuntimeError((tag, s[-510:]))
        return s

    p = d / "common-garden.csv"
    fieldnames = [
        "habitat",
        "arm",
        "stage",
        "tick",
        "eligible_sample",
        "fully_correct",
        "repairs",
        "presentations",
        "unique_rules",
        "distinct_rule_words",
        "mean_generation",
        "median_cells",
        "peak_cells",
        "age_at_most",
        "live_population",
    ]

    def screen(name, arm, stage, src, tick, hist="empty"):
        tag = "garden-" + name + "-" + arm + "-" + stage + "-" + hist
        receipt = d / (tag + ".tsv")
        s = run(
            tag,
            [
                "--load",
                src,
                "--steps",
                0,
                "--capillary-history",
                hist,
                "--capillary-residents",
                sample,
                "--capillary-contacts",
                0,
                "--retina-born-after",
                tick - 2048,
                "--capillary-trials",
                24,
                "--class-seed",
                994319,
                "--capillary-supply",
                "scarce",
                "--capillary-depth",
                9,
                "--capillary-moves",
                5,
                "--capillary-screen",
                receipt,
            ],
        )
        q = next(z for z in s.splitlines() if z.startswith("capillary_cohort="))
        field = next(z for z in s.splitlines() if z.startswith("flood_tick="))
        val = lambda row, k: int(dict(x.split("=", 1) for x in row.split())[k])
        m = val(q, "enrolled")
        if not m:
            rows = []
        else:
            with receipt.open() as f:
                rows = [
                    {k: int(v) for k, v in r.items()}
                    for r in csv.DictReader(f, delimiter="\t")
                ]
        assert len(rows) == m and all(tick - 2048 < r["born"] <= tick for r in rows)
        rate = sum(r["caught"] for r in rows)
        full = sum(r["full_law"] == 48 for r in rows)
        if m:
            assert full == val(q, "completely_predicted") and rate == val(q, "earned")
        summary = [
            name,
            arm,
            stage + "-" + hist,
            tick,
            m,
            full,
            rate,
            48 * m,
            len({r["plasticity"] & 15 for r in rows}),
            len({r["plasticity"] for r in rows}),
            sum(r["generation"] for r in rows) / max(1, m),
            sorted(r["cells"] for r in rows)[m // 2] if m else 0,
            max((r["cells"] for r in rows), default=0),
            2047,
            val(field, "living"),
        ]
        with p.open("a") as f:
            csv.writer(f, lineterminator="\n").writerow(summary)
        print(*summary, flush=True)

    with p.open("w") as f:
        csv.writer(f, lineterminator="\n").writerow(fieldnames)
    for name, initial in starts:
        start = w / (name + "-new.flood")
        screen(name, "shared", "start", start, initial)
        for arm in treatments:
            tag = f"growth-{name}-{arm}"
            end = w / (tag + ".flood")
            run(
                tag,
                [
                    "--load",
                    start,
                    "--steps",
                    ticks,
                    "--report-every",
                    max(1, ticks // 4),
                    "--capillary-field",
                    arm,
                    "--capillary-food",
                    "pigments",
                    "--capillary-period",
                    4,
                    "--save",
                    end,
                ],
            )
            screen(name, arm, "end", end, initial + ticks)
            screen(name, arm, "end", end, initial + ticks, "retained")
    rates = []
    for path in d.glob("growth-*.log"):
        for line in path.read_text().splitlines():
            if line.startswith("flood_seconds="):
                rates.append(
                    float(
                        dict(x.split("=") for x in line.split())[
                            "simulation_Msite_ticks_per_s"
                        ]
                    )
                )
    selection_figure(p, output / "cohorts.webp", rates)
    print(f"Selection census, GPU checkpoints and journal are in {output}.")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("output", type=Path)
    ap.add_argument("--binary", type=Path, default=ROOT / "axion")
    ap.add_argument("--ticks", type=int, default=32768)
    ap.add_argument("--sample", type=int, default=2048)
    ap.add_argument("--habitat", choices=["bank", "old", "slow", "all"], default="all")
    ap.add_argument(
        "--treatment",
        choices=["live", "watch", "fixed", "lookup", "all"],
        default="all",
    )
    args = ap.parse_args()
    starts = [
        (name, t)
        for name, t in (("bank", 20480), ("old", 86016), ("slow", 20480))
        if args.habitat in ("all", name)
    ]
    treatments = [
        mode
        for mode in ("live", "watch", "fixed", "lookup")
        if args.treatment in ("all", mode)
    ]
    study(
        args.output.resolve(),
        args.binary.resolve(),
        args.ticks,
        args.sample,
        starts,
        treatments,
    )
