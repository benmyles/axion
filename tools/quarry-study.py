#!/usr/bin/env python3
"""Release unpracticed builders into unfamiliar freely rearrangeable mineral fields."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--axion",default="./axion");p.add_argument("--output",type=Path,required=True)
p.add_argument("--retain",type=Path);p.add_argument("--seeds",nargs="+",type=int,default=[1877,5569,8123,31139])
p.add_argument("--seasons",type=int,default=1024);p.add_argument("--capacity",type=int,default=16384)
p.add_argument("--subjects",type=int,default=4096);a=p.parse_args()
def run(*opts):
    return subprocess.run([a.axion,"floodplain",*map(str,opts)],text=True,check=True,stdout=subprocess.PIPE).stdout
def row(log,tag):
    return dict(x.split("=",1) for x in next(l for l in log.splitlines() if l.startswith(tag+"=")).split())
with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-quarry-") as stage,a.output.open("w",newline="") as out:
    world=Path(stage)/"source.flood";writer=None
    for seed in a.seeds:
        for regime in ("provisioned","fostered"):
            log=run("--side",64,"--seed",seed,"--quarry-seasons",a.seasons,"--work-capacity",a.capacity,"--work-days",16,
                    "--work-span",2,"--quarry-exposure",48,"--work-recruit",regime,"--steps",0,"--save",world)
            lineage=row(log,"work_descendants")
            for separation in (2,4):
                for actions in (1,16,64):
                    q=run("--load",world,"--quarry-moves",actions,"--quarry-exposure",96,"--work-span",separation,
                          "--work-subjects",a.subjects,"--work-test-seed",seed^0x70771,"--steps",0)
                    result=dict(seed=seed,recruitment=regime,descendants=lineage["work_descendants"],**row(q,"quarry_builders"))
                    if writer is None:writer=csv.DictWriter(out,fieldnames=result.keys(),lineterminator="\n");writer.writeheader()
                    writer.writerow(result)
            if a.retain and seed==a.seeds[0] and regime=="provisioned":
                import shutil
                shutil.copyfile(world,a.retain)
            out.flush();print(seed,regime,lineage["work_descendants"],flush=True)
