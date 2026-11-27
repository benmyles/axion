#!/usr/bin/env python3
"""Hatch descendants and ancestors on new soil with neutral ancestry markers."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

args = argparse.ArgumentParser(description=__doc__)
args.add_argument("--axion", default="./axion")
args.add_argument("--seed", type=int, default=8123)
args.add_argument("--seeds", nargs="+", type=int, default=[403, 1979, 11681, 87719, 299477, 811039])
args.add_argument("--side", type=int, default=64)
args.add_argument("--age", type=int, default=8192)
args.add_argument("--ticks", type=int, default=2048)
args.add_argument("--output", required=True, type=Path)
a=args.parse_args()


def field(*options):
    p=subprocess.run([a.axion,"floodplain","--materials","off","--side",str(a.side),"--report-every",str(max(1,a.ticks//4)),*map(str,options)],
                     check=True,text=True,stdout=subprocess.PIPE)
    return [dict(v.split("=",1) for v in line.split()) for line in p.stdout.splitlines() if line.startswith("flood_census=")]


rows=[]
with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-competition-") as tmp:
    ancestors=Path(tmp)/"ancestral.flood"
    evolved=Path(tmp)/"descendant.flood"
    field("--seed",a.seed,"--steps",0,"--save",ancestors)
    field("--load",ancestors,"--steps",a.age,"--save",evolved)
    for seed in a.seeds:
        for condition,donors,extra in (("control",(ancestors,ancestors),()),("offspring",(evolved,ancestors),()),
                                       ("cross_brains",(evolved,ancestors),("--cross-brains",))):
            for swapped in (0,1):
                census=field("--seed",seed,"--colonists",donors[0],"--competitors",donors[1],"--steps",a.ticks,
                             *extra,*(["--swap"] if swapped else []))
                assert census[0]["cohort_a"]==census[0]["cohort_b"] and census[0]["reserve_a"]==census[0]["reserve_b"]
                for q in census:
                    rows.append(dict(seed=seed,condition=condition,swap=swapped,
                        **{k:int(q[k]) for k in ("flood_census","cohort_a","cohort_b","reserve_a","reserve_b","families","deepest")}))
                print(seed,condition,swapped,census[-1]["cohort_a"],census[-1]["cohort_b"],flush=True)
        with a.output.open("w",newline="") as out:
            w=csv.DictWriter(out,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
