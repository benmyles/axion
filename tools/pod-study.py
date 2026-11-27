#!/usr/bin/env python3
"""Raise temporal synapses on energy-limited CUDA rafts and release them into matched wild gardens."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--axion",default="./axion");p.add_argument("--output",required=True,type=Path)
p.add_argument("--seeds",nargs="+",type=int,default=[1877,5569,8123,31139])
p.add_argument("--seasons",type=int,default=1024);p.add_argument("--capacity",type=int,default=16384)
p.add_argument("--days",type=int,default=512);p.add_argument("--trials",type=int,default=384)
p.add_argument("--subjects",type=int,default=1024);p.add_argument("--wild-ticks",type=int,default=2048)
a=p.parse_args();rows=[]
def field(*args):
    cmd=[a.axion,"floodplain","--materials","off","--neurogenesis","fixed","--pod-frames","single","--pod-regimes","independent","--pod-pulse","nutrient","--report-every","1024",*map(str,args)]
    r=subprocess.run(cmd,check=True,text=True,stdout=subprocess.PIPE)
    by={}
    for line in r.stdout.splitlines():
        if line.startswith(("pod_transfers=","pod_tick=","raft_descendants=","flood_census=","pod_wild_replay=")):
            by.setdefault(line.partition("=")[0],[]).append(dict(v.split("=",1) for v in line.split()))
    return by
def write():
    keys=list(dict.fromkeys(k for row in rows for k in row))
    with a.output.open("w",newline="") as out:
        w=csv.DictWriter(out,fieldnames=keys);w.writeheader();w.writerows(rows)
with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-pod-study-") as scratch:
    src=Path(scratch)/"parents.flood"
    for seed in a.seeds:
        for selection in ("provisioned","fostered"):
            r=field("--side",64,"--seed",seed,"--pods","sealed","--raft-seasons",a.seasons,"--raft-capacity",a.capacity,
                    "--raft-days",a.days,"--recruit",selection,"--steps",0,"--pod-assay",a.trials,"--pod-subjects",a.subjects,
                    "--pod-test-seed",seed^0xd3a57,"--save",src)
            for q in r["pod_transfers"]:rows.append(dict(seed=seed,experiment="new_crops",treatment=selection,**q))
            if selection=="provisioned":
                for g in (seed^0x74c51,seed^0x9937):
                    for rotation in range(2):
                        w=field("--side",64,"--seed",g,"--colonists",src,"--competitors",src,"--freeze-cohort","b","--pods","sealed",
                                "--pod-period",192,"--steps",a.wild_ticks,*(["--swap"] if rotation else []))
                        first,last=w["flood_census"][0],w["flood_census"][-1]
                        assert first["cohort_a"]==first["cohort_b"] and first["reserve_a"]==first["reserve_b"]
                        rows.append(dict(seed=seed,experiment="shared_watershed",treatment="intact_a",garden=g,swap=rotation,
                                         cohort_a=last["cohort_a"],cohort_b=last["cohort_b"],reserve_a=last["reserve_a"],reserve_b=last["reserve_b"],
                                         **w["pod_tick"][-1]))
            write();print(seed,selection,r["pod_transfers"][0]["practiced"],flush=True)
