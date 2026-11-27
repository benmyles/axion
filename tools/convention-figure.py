#!/usr/bin/env python3
"""Plot learned meanings and measured watershed performance from the archived CUDA results."""

import csv
from pathlib import Path
from journal import Drawing, Codec, MONO, SERIF, SANS


def figure(folder):
    def rows(file):
        with (folder / (file + ".csv")).open() as f:
            return {r["run"]: r for r in csv.DictReader(f)}

    reunions = rows("reunions")
    school = rows("populations")
    water = rows("watersheds")["catalytic-camera-coast"]
    ink = "#172f32"
    fog = "#ebefe3"
    sub = "#46655f"
    d = Drawing(3840, 2160)
    d.rect(0, 0, 3840, 2160, fog)
    d.rect(0, 0, 3840, 372, ink)
    d.text(106, 88, "AXION   /   EVOLUTION JOURNAL", 38, "#adcbb6", MONO)
    d.text(100, 220, "One physical mark, two learned meanings", 96, fog, SERIF)
    d.text(
        108,
        310,
        "Identical animals after separate lives. Each receiver sees only the response of a sealed pressure junction.",
        40,
        "#c6d9ca",
        SANS,
    )
    d.text(106, 503, "Who can read the mark?", 62, ink, SERIF)
    d.text(108, 571, "2,048 held-out feeder choices per cell", 34, sub, MONO)
    for j, reader in enumerate(("left", "right")):
        d.text(
            850 + j * 545,
            685,
            f"Reader / {reader}",
            39,
            ink,
            SANS,
            600,
            anchor="middle",
        )
    for i, writer in enumerate(("left", "right")):
        d.text(110, 875 + i * 281, f"Writer / {writer}", 40, ink, SANS, 600)
        for j, reader in enumerate(("left", "right")):
            r = reunions[f"conventions-brook-reunion-{writer}-{reader}-active"]
            x = 598 + j * 545
            y = 730 + i * 283
            d.rect(x, y, 512, 244, "#83b7a2" if i == j else "#e1cbb7", 13)
            d.text(
                x + 256,
                y + 119,
                f"{100*int(r['correct'])/int(r['cases']):.1f}%",
                73,
                ink,
                SANS,
                600,
                anchor="middle",
            )
            d.text(
                x + 256,
                y + 188,
                f"{int(r['correct']):,} correct",
                32,
                ink,
                MONO,
                anchor="middle",
            )
    d.text(
        110,
        1391,
        "Erasing the mark gave all four pairings 1,024 correct choices.",
        34,
        sub,
        SANS,
    )
    x = 1998
    d.text(x, 504, "How common was learning?", 62, ink, SERIF)
    d.text(
        x, 573, "2,048 unscreened pairs from the starting brook sample", 33, sub, MONO
    )
    for i, (name, tag, color) in enumerate(
        (
            ("Food feedback", "active-512-food", "#70aa96"),
            ("Unrelated feedback", "active-512-unrelated", "#bd92a6"),
            ("Frozen contacts", "active-512-silent", "#a8aa94"),
            ("Inert material", "inert-512-food", "#ba9d75"),
        )
    ):
        r = school["relay-brook-" + tag]
        y = 707 + i * 166
        fraction = int(r["after"]) / int(r["cases"])
        width = 1650
        d.text(x, y, name, 40, ink, SANS, 600)
        d.text(x + width, y, f"{100*fraction:.2f}%", 41, ink, MONO, anchor="end")
        d.rect(x, y + 35, width, 32, "#cfd7c9", 6)
        d.rect(x, y + 35, width * fraction, 32, color, 6)
        d.rect(x + width * 0.5, y + 26, 4, 50, "#324b45")
        d.text(
            x,
            y + 119,
            f"{int(r['after']):,} of {int(r['cases']):,} choices",
            29,
            sub,
            MONO,
        )
    d.rect(0, 1515, 3840, 645, ink)
    d.text(106, 1636, "The same chemistry in the free coastal habitat", 65, fog, SERIF)
    for i, (num, label) in enumerate(
        (
            (water["witnessed_closures"], "Consecutive closures witnessed"),
            (f'{int(water["living"]):,}', "Living creatures"),
            (f'{int(water["conducting_cells"]):,}', "Active spiking cells"),
            (
                f'{float(water["simulation_Msite_ticks_per_s"]):.2f} M',
                "Site-ticks per second",
            ),
        )
    ):
        xx = 106 + i * 910
        d.text(xx, 1830, num, 74, "#dbebd9", SANS, 600)
        d.text(xx, 1908, label, 34, "#bfd2c5", SANS)
    d.text(
        108,
        2044,
        "RTX PRO 2000 Blackwell Laptop GPU   /   zero additional bits per site for the chemistry",
        34,
        "#adc6b6",
        MONO,
    )
    Codec().write(d, folder / "conventions.webp")


if __name__ == "__main__":
    figure(Path(__file__).resolve().parents[1] / "docs/material-conventions")
