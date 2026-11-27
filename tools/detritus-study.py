#!/usr/bin/env python3
"""Raise matched CUDA populations with stationary, pressure-driven, displaced and visibly stained food."""

import argparse
import csv
import os
import re
import subprocess
from pathlib import Path
from detritus_audit import audit, bounds

ROOT = Path(__file__).resolve().parents[1]
HABITATS = {
    "bank": ("nursery", 20480),
    "old": ("older-nursery", 86016),
    "slow": ("slow-nursery", 20480),
    "coast": ("estuary", 427928),
}
ARMS = ("still", "pressure", "displaced", "stained")


def study(output, binary, habitats, arms, ticks, animals):
    output.mkdir(parents=True, exist_ok=False)
    env = {**os.environ, "AXION_JOURNAL_DIR": str(output / "journal")}

    def run(label, args):
        path = output / f"{label}.log"
        with path.open("x") as f:
            subprocess.run(
                [binary, "floodplain", *map(str, args)],
                text=True,
                stdout=f,
                stderr=subprocess.STDOUT,
                env=env,
                check=True,
            )
        return [
            dict(re.findall(r"([^\s=]+)=([^\s]+)", line))
            for line in path.read_text().splitlines()
        ]

    with (output / "comparison.csv").open("x") as file:
        writer = csv.writer(file, lineterminator="\n")
        writer.writerow(
            (
                "habitat",
                "condition",
                "elapsed_ticks",
                "residents_tested",
                "both_laws",
                "food_repairs",
                "material_moves",
                "grain_moves",
                "changed_colours",
                "enabled_routes",
                "prevented_routes",
                "million_site_ticks_per_second",
            )
        )
        for name in habitats:
            archive, start = HABITATS[name]
            base = output / (name + "-initial.flood")
            with base.open("xb") as f:
                subprocess.run(
                    ["zstd", "-qdc", ROOT / f"examples/capillary-{archive}.flood.zst"],
                    stdout=f,
                    check=True,
                )
            steps = ticks if ticks else (1024 if name == "coast" else 32768)
            if not (output / "boundary.tsv").exists():
                run(
                    "bounds",
                    [
                        "--load",
                        base,
                        "--steps",
                        0,
                        "--capillary-cargo-proof",
                        output / "boundary.tsv",
                    ],
                )
                bounds(output / "boundary.tsv")
            for treatment in arms:
                tag = name + "-" + treatment
                physical = "pressure" if treatment == "stained" else treatment
                receipts = output / (tag + "-food.tsv")
                options = (
                    ["--capillary-cargo-events", receipts]
                    if physical != "still"
                    else []
                )
                lines = run(
                    tag,
                    [
                        "--load",
                        base,
                        "--steps",
                        steps,
                        "--report-every",
                        max(1, steps // 4),
                        "--capillary-field",
                        "live",
                        "--capillary-food",
                        "pigments",
                        "--capillary-period",
                        8 if name == "coast" else 4,
                        "--capillary-cargo",
                        physical,
                        "--capillary-cargo-dye",
                        "exchange" if treatment == "stained" else "fixed",
                        *options,
                        "--capillary-residents",
                        animals,
                        "--capillary-history",
                        "empty",
                        "--retina-born-after",
                        start + steps - 2048,
                        "--capillary-trials",
                        24,
                        "--capillary-depth",
                        9,
                        "--capillary-moves",
                        5,
                        "--capillary-supply",
                        "scarce",
                        "--class-seed",
                        994319,
                        "--capillary-screen",
                        output / (tag + "-residents.tsv"),
                        "--save",
                        output / (tag + ".flood"),
                    ],
                )
                if physical != "still":
                    audit(receipts)

                def row(key):
                    return next((q for q in lines if key in q), {})

                cohort = row("capillary_cohort")
                grain = row("detritus_field")
                writer.writerow(
                    (
                        name,
                        treatment,
                        steps,
                        cohort["enrolled"],
                        cohort.get("completely_predicted", 0),
                        cohort.get("earned", 0),
                        row("capillary_field")["experiments"],
                        grain.get("arrived", 0),
                        grain.get("changed_colours", 0),
                        grain.get("enabled_edges", 0),
                        grain.get("prevented_edges", 0),
                        row("flood_seconds")["simulation_Msite_ticks_per_s"],
                    )
                )
                file.flush()
    print(
        f"Saved paired population runs, food receipts, cortical exams and WebPs in {output}."
    )


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--binary", type=Path, default=ROOT / "axion")
    p.add_argument("--habitat", choices=[*HABITATS, "all"], default="all")
    p.add_argument("--condition", choices=[*ARMS, "all"], default="all")
    p.add_argument("--steps", type=int)
    p.add_argument("--residents", type=int, default=2048)
    a = p.parse_args()
    study(
        a.output.resolve(),
        a.binary.resolve(),
        HABITATS if a.habitat == "all" else (a.habitat,),
        ARMS if a.condition == "all" else (a.condition,),
        a.steps,
        a.residents,
    )
