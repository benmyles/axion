#!/usr/bin/env python3
"""Measure lifetime and inherited changes in spiking-cell count in reciprocal common gardens."""
import argparse
import csv
from pathlib import Path
import subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--axion",default="./axion")
p.add_argument("--source",type=Path,default=Path("examples/reedbank.flood"))
p.add_argument("--output",type=Path,required=True)
p.add_argument("--seeds",nargs="+",type=int,default=[3931,1787,6947,8233])
p.add_argument("--neurons",type=int,default=512)
p.add_argument("--ticks",type=int,default=2048)
p.add_argument("--treatments",nargs="+",choices=["both","b_fixed","b_silent"],default=["both","b_fixed"])
a=p.parse_args()

with a.output.open("w",newline="") as out:
    writer=None
    for seed in a.seeds:
        for treatment in a.treatments:
            for swap in (0,1):
                cmd=[a.axion,"floodplain","--materials","off","--side","64","--seed",str(seed),"--neurons",str(a.neurons),"--colonists",str(a.source),
                     "--competitors",str(a.source),"--neurogenesis","inherited","--steps",str(a.ticks),"--report-every","512","--assay-growth"]
                if treatment=="b_fixed":cmd+=["--fixed-size-cohort","b"]
                if treatment=="b_silent":cmd+=["--silence-new-cohort","b"]
                if swap:cmd+=["--swap"]
                result=subprocess.run(cmd,check=True,text=True,stdout=subprocess.PIPE).stdout
                def rows(prefix):
                    return [dict(x.split("=",1) for x in line.split()) for line in result.splitlines() if line.startswith(prefix+"=")]
                first,last=rows("flood_census")[0],rows("flood_census")[-1]
                assert first["cohort_a"]==first["cohort_b"] and first["reserve_a"]==first["reserve_b"]
                row=dict(seed=seed,treatment=treatment,swap=swap,founders_each=first["cohort_a"],final_a=last["cohort_a"],
                         final_b=last["cohort_b"],**rows("spiking_growth")[-1],**rows("growth_replay")[-1])
                if writer is None:
                    writer=csv.DictWriter(out,fieldnames=row.keys(),lineterminator="\n");writer.writeheader()
                writer.writerow(row);out.flush()
