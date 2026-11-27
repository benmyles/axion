#!/usr/bin/env python3
"""Branch an inhabited watershed into matched lighting climates and assay its real residents."""

import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--axion", default="./axion")
p.add_argument("--origin", type=Path, required=True)
p.add_argument("--output", type=Path, required=True)
p.add_argument("--adults", type=Path, required=True)
p.add_argument("--retain", type=Path)
a = p.parse_args()


def run(*args):
    command = [a.axion, "floodplain", *map(str, args)]
    child = subprocess.run(command, capture_output=True, text=True)
    if child.returncode:
        raise RuntimeError(child.stderr + "\n" + child.stdout[-500:])
    return child.stdout


def entries(log, name):
    return [dict(w.split("=", 1) for w in line.split()) for line in log.splitlines() if line.startswith(name + "=")]


with (
    tempfile.TemporaryDirectory(prefix="axion-tidal-", dir=os.environ.get("TMPDIR", "/var/tmp")) as tmp,
    a.output.open("w", newline="") as f,
    a.adults.open("w", newline="") as g,
):
    table = csv.DictWriter(
        f,
        fieldnames="climate replay tick living births deaths neurons water food animal_energy products carried turns placed exchanges sessions considered deliberate_lifts deliberate_placements work".split(),
        lineterminator="\n",
    )
    table.writeheader()
    probing = None
    for climate in ("diffuse", "veined", "mosaic"):
        for replay in ("on", "off"):
            path = Path(tmp) / "field.flood"
            source = a.origin
            for interval in range(2):
                log = run(
                    "--load",
                    source,
                    "--steps",
                    1024,
                    "--report-every",
                    1024,
                    "--light-regime",
                    climate,
                    "--site-replay",
                    replay,
                    "--save",
                    path,
                )
                body = entries(log, "flood_tick")[-1]
                matter = entries(log, "material_tick")[-1]
                tide = entries(log, "construction_tide")[-1]
                row = dict(
                    climate=climate,
                    replay=replay,
                    tick=body["flood_tick"],
                    **{
                        k: body[k]
                        for k in ("living", "births", "deaths", "neurons", "water", "food", "animal_energy")
                    },
                    **{k: matter[k] for k in ("products", "carried", "turns", "placed", "exchanges")},
                    sessions=tide["sessions"],
                    considered=tide["candidates"],
                    deliberate_lifts=tide["lifts"],
                    deliberate_placements=tide["placements"],
                    work=tide["cost"],
                )
                table.writerow(row)
                source = path
            if a.retain and climate == "mosaic" and replay == "on":
                import shutil

                shutil.copyfile(path, a.retain)
            if int(body["living"]):
                for practice in (0, 8, 32, 96):
                    log = run(
                        "--load",
                        path,
                        "--steps",
                        0,
                        "--shoal-origin",
                        "resident",
                        "--shoal-trials",
                        4,
                        "--work-subjects",
                        1024,
                        "--shoal-span",
                        4,
                        "--shoal-moves",
                        10,
                        "--shoal-ideas",
                        128,
                        "--shoal-practice",
                        practice,
                        "--shoal-exposure",
                        256,
                    )
                    for r in entries(log, "shoal_test"):
                        row = dict(climate=climate, replay=replay, **r)
                        if probing is None:
                            probing = csv.DictWriter(g, fieldnames=row, lineterminator="\n")
                            probing.writeheader()
                        probing.writerow(row)
            f.flush()
            g.flush()
            print(climate, replay, body["living"], flush=True)
