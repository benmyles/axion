#!/usr/bin/env python3
"""Matched environmental interventions on independent CUDA floodplain lineages."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--axion", default="./axion")
parser.add_argument("--seeds", nargs="+", type=int, default=[1877, 5569, 8123, 31139])
parser.add_argument("--side", type=int, default=64)
parser.add_argument("--ticks", type=int, default=8192)
parser.add_argument("--producers",choices=["simple","fixed","evolving"],default="simple")
parser.add_argument("--hearing",choices=["scalar","projected"],default="scalar")
parser.add_argument("--follow", type=int, default=2048)
parser.add_argument("--output", required=True, type=Path)
args = parser.parse_args()
rows = []


def run(seed, name, *options):
    cmd = [args.axion, "floodplain", "--materials", "off", "--report-every", str(max(1, args.ticks // 4)), *map(str, options)]
    output = subprocess.run(cmd, check=True, stdout=subprocess.PIPE, text=True).stdout

    def latest(prefix):
        records = [line for line in output.splitlines() if line.startswith(prefix + "=")]
        return dict(entry.split("=", 1) for entry in records[-1].split()) if records else {}

    stats = latest("flood_tick")
    census = latest("flood_census")
    legacy = latest("ecological_legacy")
    hearing = next((dict(entry.split("=",1) for entry in line.split()) for line in output.splitlines() if line.startswith("field_hearing=cleared")), {})
    rows.append(dict(seed=seed, treatment=name,
        **{k:int(stats[k]) for k in ("flood_tick","living","births","deaths","banked","food","animal_energy","flow","transferred","receptions","module_edits","fibre","mixed_faces","joined","supported")},
        **{k:int(census[k]) for k in ("families","distinct_bodies","expressed_tapes","wirings","persistent_bodies","persistent_brains",
            "multi_module_residents","leaf_specialists","fruit_specialists","litter_specialists","living_builders","deepest")},
        **{k:int(hearing[k]) if hearing else "" for k in ("experienced","changed_actions","changed_spikes")},
        **{k:int(legacy[k]) if legacy else "" for k in ("water_sites","food_sites","intact_light","flat_light")}))
    print(seed, name, stats["living"], census["deepest"], flush=True)


with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-watersheds-") as tmp:
    for seed in args.seeds:
        base = ["--side",args.side,"--seed",seed,"--steps",args.ticks,"--producers",args.producers,"--hearing",args.hearing]
        open_bed, loose_bed = Path(tmp)/"built.flood",Path(tmp)/"loose.flood"
        run(seed, "free", *base, "--legacy",256,"--assay-calls","--save",open_bed)
        run(seed, "loose", *base,"--construction","closed","--legacy",256,"--save",loose_bed)
        run(seed, "clay_only", *base,"--weaving","closed","--legacy",256)
        run(seed, "scrambled_calls", *base,"--wire","scrambled","--assay-calls")
        run(seed, "fixed_brains", *base,"--architecture","closed")
        run(seed, "fixed_body_plans", *base,"--morphology","closed")
        run(seed, "silent", *base,"--wire","closed")
        run(seed, "no_plasticity", *base,"--plasticity","closed")
        # Same starting residents, learned contacts and future physical inputs.
        run(seed, "built_home","--load",open_bed,"--steps",args.follow)
        run(seed, "built_forgot_calls","--load",open_bed,"--forget-calls","--steps",args.follow)
        run(seed, "built_remobilized","--load",open_bed,"--remobilize","--steps",args.follow)
        # Both reciprocal transplants allow construction in the follow-up.
        run(seed, "built_in_loose_soil","--load",open_bed,"--soil",loose_bed,"--steps",args.follow)
        run(seed, "loose_home","--load",loose_bed,"--construction","open","--steps",args.follow)
        run(seed, "loose_in_built_soil","--load",loose_bed,"--construction","open","--soil",open_bed,"--steps",args.follow)
        with args.output.open("w", newline="") as out:
            writer = csv.DictWriter(out,fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)
