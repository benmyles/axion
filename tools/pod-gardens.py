#!/usr/bin/env python3
"""Release reciprocal pairs from the same brood with intact, temporal-only, or frozen acquisition."""
import argparse
import csv
from pathlib import Path
import subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--axion",default="./axion")
p.add_argument("--source",required=True,type=Path)
p.add_argument("--output",required=True,type=Path)
p.add_argument("--seeds",nargs="+",type=int,default=[3517,1447,9983,27449])
p.add_argument("--ticks",type=int,default=2048)
p.add_argument("--temporal-only",action="store_true")
a=p.parse_args()
with a.output.open("w",newline="") as out:
    writer=None
    regimes = ["neutral"] if a.temporal_only else ["neutral","single","frozen"]
    for seed in a.seeds:
        for treatment in regimes:
            for swap in (0,1):
                cmd=[a.axion,"floodplain","--materials","off","--neurogenesis","fixed","--side","64","--seed",str(seed),"--colonists",str(a.source),
                     "--competitors",str(a.source),"--pods","sealed","--steps",str(a.ticks),"--report-every","512"]
                if a.temporal_only:
                    cmd+=["--pod-frames","single","--pod-regimes","independent","--pod-period","192","--pod-pulse","nutrient"]
                else:
                    cmd+=["--pod-frames","inherited","--pod-regimes","shared","--pod-period","96","--pod-pulse","enteric"]
                if treatment=="single":cmd+=["--single-frame-cohort","b"]
                if treatment=="frozen":cmd+=["--freeze-cohort","b"]
                if swap:cmd+=["--swap"]
                log=subprocess.run(cmd,check=True,stdout=subprocess.PIPE,text=True).stdout
                census=[dict(t.split("=",1) for t in line.split()) for line in log.splitlines() if line.startswith("flood_census=")]
                start,end=census[0],census[-1]
                assert start["cohort_a"]==start["cohort_b"] and start["reserve_a"]==start["reserve_b"]
                row=dict(seed=seed,treatment=treatment,swap=swap,tick=end["flood_census"],founders_each=start["cohort_a"],
                         final_a=end["cohort_a"],final_b=end["cohort_b"],reserve_a=end["reserve_a"],reserve_b=end["reserve_b"])
                if writer is None:
                    writer=csv.DictWriter(out,fieldnames=row.keys(),lineterminator="\n");writer.writeheader()
                writer.writerow(row);out.flush()
