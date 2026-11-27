#!/usr/bin/env python3
"""Hold terrain and animal founders constant while changing rooted succession."""
import argparse
import csv
from pathlib import Path
import subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--axion",default="./axion");p.add_argument("--output",required=True,type=Path)
p.add_argument("--seeds",nargs="+",type=int,default=[1877,5569,8123,31139])
p.add_argument("--ticks",type=int,default=8192);p.add_argument("--side",type=int,default=64)
a=p.parse_args()
rows=[]
for seed in a.seeds:
    for mode,extra in (("evolving",[]),("fixed",["--producers","fixed"]),("empty",["--density",0]),
                       ("loose",["--construction","closed"]),("scalar",["--hearing","scalar"]),
                       ("silent",["--wire","closed"]),("scrambled",["--wire","scrambled"])):
        cmd=[a.axion,"floodplain","--materials","off","--seed",seed,"--side",a.side,"--steps",a.ticks,"--report-every",a.ticks//4,"--assay-every",a.ticks//16,"--legacy",128,*extra]
        proc=subprocess.run(list(map(str,cmd)),check=True,text=True,stdout=subprocess.PIPE)
        by_type={}
        for line in proc.stdout.splitlines():
            if line.startswith(("flood_tick=","flood_census=","ecological_legacy=","field_hearing=")):
                fields=dict(x.split('=',1) for x in line.split())
                by_type.setdefault(line.partition('=')[0],[]).append(fields)
        census=by_type['flood_census'][-1];stock=by_type['flood_tick'][-1];legacy=by_type['ecological_legacy'][-1]
        hearing=[q for q in by_type['field_hearing'] if q['field_hearing']=='cleared']
        rows.append(dict(seed=seed,mode=mode,
            **{k:int(census[k]) for k in ("producer_genotypes","producer_tables","families","distinct_bodies","persistent_brains","deepest")},
            **{k:int(stock[k]) for k in ("living","births","deaths","food","recruits","plant_mutations","reproductive_stock","mixed_faces","supported","receptions")},
            **{k:sum(int(q[k]) for q in hearing) for k in ("experienced","changed_inputs","changed_actions","changed_spikes")},
            **{k:int(legacy[k]) for k in ("water_sites","food_sites")}))
        print(seed,mode,stock['recruits'],stock['living'],rows[-1]['changed_actions'],flush=True)
    with a.output.open("w",newline="") as out:
        w=csv.DictWriter(out,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
