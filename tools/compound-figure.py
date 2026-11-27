#!/usr/bin/env python3
"""Draw frozen material codes, unseen feeders and actual spoken interventions from published observations."""

import argparse
import csv
from pathlib import Path
from journal import Codec, Drawing, MONO, SANS, SERIF


def figure(folder):
    def load(name):
        with (folder / (name + ".csv")).open() as f:
            return list(csv.DictReader(f))

    calls = {r["run"]: r for r in load("spoken")}
    people = {r["run"]: r for r in load("individuals")}
    cues = load("cues")
    lives = load("lifetimes")
    speed = load("benchmark")[0]
    d = Drawing(3840, 2160)
    paper = "#efeddf"
    dark = "#142e31"
    mid = "#3f625e"
    trim = "#b2c1b3"
    light = "#d7e1d4"
    mint = "#91cbb4"
    gold = "#e6b26d"
    violet = "#b5a4ca"
    d.rect(0, 0, 3840, 2160, paper)
    d.rect(0, 0, 3840, 290, dark)
    d.text(104, 78, "AXION  /  WATERSHED FIELD JOURNAL", 36, mint, MONO)
    d.text(100, 203, "Two marks, four destinations, one missing word", 92, paper, SERIF)
    d.text(
        105,
        258,
        "Frozen physical choices and recorded exchanges between independently reared spiking animals.",
        34,
        "#c2d4c6",
        SANS,
    )

    def heading(x, y, text, caption):
        d.text(x, y, text, 54, dark, SERIF)
        d.text(x, y + 62, caption, 31, mid, SANS)

    heading(
        100,
        406,
        "What the writer did to the junctions",
        "Each row shows 1,024 visits to a stocked feeder.",
    )
    heading(
        1990,
        406,
        "Could they infer the missing corner?",
        "One feeder was absent throughout rearing. Bars span 0–40 reliable lives.",
    )
    for j, (kind, title) in enumerate(
        (("joint", "Joint code"), ("separable", "Separable code"))
    ):
        x = 102 + 890 * j
        tag = f"type-reunion-{kind}-{kind}-active"
        rows = [r for r in cues if r["run"] == tag]
        assert len(rows) == 4
        d.rect(x, 516, 850, 536, "#e1e6d9", 12)
        d.text(x + 33, 582, title, 46, dark, SERIF)
        for k, col in enumerate(("Wait", "First", "Second", "Both")):
            d.text(x + 234 + k * 157, 647, col, 27, mid, MONO, anchor="middle")
        for index, r in enumerate(rows):
            y = 675 + index * 72
            d.text(
                x + 35,
                y + 44,
                ("NW", "NE", "SW", "SE")[int(r["compound_cue"])],
                32,
                dark,
                MONO,
            )
            for k in range(4):
                val = int(r[f"mark{k}"])
                xx = x + 157 + k * 157
                d.rect(xx, y, 148, 61, mint if val > 700 else "#c8d5c8", 5)
                d.text(xx + 74, y + 42, f"{val:,}", 29, dark, MONO, anchor="middle")
        d.text(
            x + 34,
            1021,
            f'{int(people[tag]["correct"]):,} of 4,096 correct',
            30,
            mid,
            MONO,
        )
    for j, s in enumerate(("NW", "NE", "SW", "SE")):
        x = 1994
        y = 570 + j * 116

        def total(arbor):
            return sum(
                int(r["complete"])
                for r in lives
                if r["run"].startswith("unseen-")
                and r["run"].endswith(f"-{j}-{arbor}-food")
            )

        count = total("native")
        joint = total("joint")
        r = people[f"absent-{j}-untouched"]
        frozen = next(
            c for c in cues if c["run"] == r["run"] and int(c["compound_cue"]) == j
        )
        d.text(x, y + 13, s, 35, dark, MONO)
        d.rect(x + 115, y - 16, 645, 34, "#d1dbcf", 4)
        d.rect(x + 115, y - 16, 645 * count / 40, 34, mid, 4)
        d.text(x + 795, y + 13, f"{count} / 256", 30, dark, MONO)
        d.text(x + 1050, y + 13, f"{joint} / 256", 30, mid, MONO)
        d.text(x + 1392, y + 13, f'{int(frozen["exact"]):,} / 1,024', 30, dark, MONO)
    for x, label in (
        (2109, "Reliable native lives"),
        (3044, "Both axes visible"),
        (3386, "Frozen unseen cue"),
    ):
        d.text(x, 1009, label, 27, mid, SANS)
    d.rule(102, 1122, 3734, 1122, trim, 2)
    heading(
        102,
        1230,
        "When the receiver can ask a question",
        "The teacher demonstrated single pressure events and single movements. It never spoke a combined example.",
    )
    d.rect(102, 1345, 1460, 579, dark, 16)
    d.text(144, 1416, "ONE PRIVATE SOUTHEAST ENCOUNTER", 29, mint, MONO)
    example = (folder / "unseen-live-partial-1-actual.txt").read_text().splitlines()
    line = [l.split("> ", 1) for l in example if l.startswith(("speaker>", "pupil>"))][
        :4
    ]
    captions = (
        "Witness saw both ports",
        "Only the first sound arrived",
        "Witness recalled the missing port",
        "Listener described its own reach",
    )
    for n, ((animal, word), note) in enumerate(zip(line, captions)):
        yy = 1497 + 105 * n
        d.text(146, yy, animal.upper(), 24, mint if animal == "speaker" else gold, MONO)
        d.text(360, yy + 4, word, 44, paper, SERIF)
        d.text(146, yy + 41, note, 26, "#b4cbbb", SANS)
    x = 1660
    for n, (tag, label, color) in enumerate(
        (
            ("clear-0-actual", "Both sounds", mid),
            ("partial-0-actual", "One sound, no question", violet),
            ("partial-1-actual", "One sound, factual reply", mid),
            ("partial-1-withheld", "Answer withheld", violet),
            ("partial-1-exchanged", "Opposite observation", violet),
        )
    ):
        r = calls["unseen-" + tag + "-00"]
        y = 1404 + n * 99
        v = int(r["found"])
        d.text(x, y, label, 32, dark, SANS)
        d.rect(x + 536, y - 27, 935, 35, "#d2dace", 5)
        d.rect(x + 536, y - 27, 935 * v / int(r["cases"]), 35, color, 5)
        d.text(x + 1511, y, f"{v:,} / 4,096", 32, dark, MONO)
    d.text(
        x,
        1896,
        "Stocked feeders reached by the same frozen animals with the same motor noise.",
        29,
        mid,
        SANS,
    )
    d.rect(0, 2012, 3840, 148, dark)
    items = (
        (f'{float(speed["speedup"]):.2f}×', "frozen CUDA readout"),
        (f'{int(speed["compound_benchmark"]):,}', "choices checked"),
        (f'{speed["receipt_bytes"]} bytes', "per physical receipt"),
        (f'{speed["local_bytes"]} bytes', "local memory per lane"),
    )
    for j, (value, name) in enumerate(items):
        xx = 105 + j * 949
        d.text(xx, 2083, value, 46, paper, SANS, 600)
        d.text(xx, 2126, name, 27, mint, MONO)
    Codec().write(d, folder / "transfer.webp")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "folder",
        nargs="?",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "docs/compound-signs",
    )
    figure(p.parse_args().folder)
