#!/usr/bin/env python3
"""Render the published paired juvenile and transfer data at 4K."""

import argparse
import csv
from pathlib import Path
from journal import Drawing, Codec, MONO, SANS, SERIF

ROOT = Path(__file__).resolve().parents[1]


def figure(folder):
    def table(name):
        with (folder / (name + ".csv")).open() as f:
            return list(csv.DictReader(f))

    rules = {
        int(r["rule"]): r for r in table("rules") if r["run"] == "retention-brook-9"
    }
    phases = {r["run"]: r for r in table("phases") if r["phase"] == "1"}
    habitats = {r["run"]: r for r in table("field")}
    d = Drawing(3840, 2160)
    ink = "#172e31"
    paper = "#f0efe4"
    muted = "#486260"
    mint = "#74b6a1"
    copper = "#d6a56b"
    violet = "#ac98c6"
    wash = "#d5ddd1"
    d.rect(0, 0, 3840, 2160, paper)
    d.rect(0, 0, 3840, 346, ink)
    d.text(108, 89, "AXION    /    EVOLUTION JOURNAL", 37, "#a4d1b9", MONO)
    d.text(103, 213, "When an animal must revise its own rule", 95, paper, SERIF)
    d.text(
        109,
        294,
        "Inherited plasticity, a changing mineral and the words an animal can learn from it.",
        42,
        "#cadacb",
        SANS,
    )
    for i, (rule, name, color) in enumerate(
        ((10, "Replace the sign", mint), (14, "Keep both signs", violet))
    ):
        r = rules[rule]
        x = 106 + i * 905
        d.rect(x, 485, 852, 597, "#e3e7da", 18)
        d.rect(x + 34, 533, 77, 8, color)
        d.text(x + 37, 620, name, 57, ink, SERIF)
        d.text(
            x + 39,
            687,
            f'{int(r["bodies"]):,} born carriers   ·   rule {rule:04b}',
            30,
            muted,
            MONO,
        )
        for j, (label, k) in enumerate(
            (("Inherited", "native_complete"), ("Bit flipped", "paired_complete"))
        ):
            y = 786 + j * 137
            n = int(r[k])
            total = int(r["bodies"])
            d.text(x + 38, y, label, 34, ink, SANS, 600)
            d.text(x + 797, y, f"{n:,} / {total:,}", 34, ink, MONO, anchor="end")
            d.rect(x + 39, y + 29, 761, 26, wash, 5)
            d.rect(x + 39, y + 29, 761 * n / total, 26, color, 5)
    d.text(108, 432, "Brook juveniles mastering all 80 episodes", 52, ink, SERIF)
    d.text(
        106,
        1160,
        "One altered retention bit. Identical physical experience.",
        37,
        muted,
        SANS,
    )
    x = 2005
    d.text(x, 430, "After a change in the pressure thresholds", 52, ink, SERIF)
    d.text(x, 500, "Held-out living channels  /  2,048 later episodes", 30, muted, MONO)
    for j, (suffix, label, col) in enumerate(
        (
            ("adaptive", "Inherited compartments", mint),
            ("retention", "Retention bit flipped", violet),
            ("clamped", "Diagnostic receipts clamped", copper),
        )
    ):
        r = phases[f"joint-brook-14187-9-{suffix}-living"]
        y = 605 + j * 163
        d.text(x, y, label, 38, ink, SANS, 600)
        d.text(
            x + 1625, y, f'{int(r["exact"]):,} exact laws', 36, ink, MONO, anchor="end"
        )
        d.rect(x, y + 24, 1622, 25, wash, 5)
        d.rect(x, y + 24, 1622 * int(r["exact"]) / int(r["episodes"]), 25, col, 5)
        d.text(
            x,
            y + 94,
            f'{int(r["spoken"]):,} / {int(r["asked"]):,} correct spoken answers',
            29,
            muted,
            MONO,
        )
    d.rect(0, 1270, 3840, 890, ink)
    d.text(108, 1398, "A descendant in the slow-pulse watershed", 73, paper, SERIF)
    d.text(
        110,
        1481,
        "Born at tick 53,109   /   generation 182   /   64 active spiking cells",
        32,
        "#b6ccbc",
        MONO,
    )
    c = phases["joint-child-13000-9-adaptive-living"]
    f = habitats["descendants-coast-regional"]
    for i, (number, caption) in enumerate(
        (
            (
                f'{int(c["exact"]):,} / {int(c["episodes"]):,}',
                "Exact later pressure laws",
            ),
            (f'{int(c["spoken"]):,} / {int(c["asked"]):,}', "Correct answerable words"),
            (
                f'{int(c["food"]):,} / {int(c["possible"]):,}',
                "Food in solvable channels",
            ),
        )
    ):
        x = 109 + i * 1210
        d.rect(x, 1562, 1118, 198, "#294340", 12)
        d.text(x + 34, 1654, number, 65, "#dce6d5", SANS, 600)
        d.text(x + 38, 1719, caption, 35, "#b7c9bb", SANS)
    d.text(110, 1882, "Free coastal simulation", 45, paper, SERIF)
    d.text(
        110,
        1963,
        f'{int(f["living"]):,} residents    /    {int(f["conducting_cells"]):,} active cells    /    {float(f["simulation_Msite_ticks_per_s"]):.2f} million site-ticks per second',
        42,
        "#b7cdbf",
        MONO,
    )
    d.text(
        112,
        2071,
        "RTX PRO 2000 Blackwell Laptop GPU   ·   12 bytes of recent material history per equipped slot",
        30,
        "#94b5a7",
        MONO,
    )
    Codec().write(d, folder / "transfer.webp")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "folder", nargs="?", type=Path, default=ROOT / "docs/inherited-plasticity"
    )
    figure(p.parse_args().folder)
