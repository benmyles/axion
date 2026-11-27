#!/usr/bin/env python3
"""Test physical construction, observational transfer and the legacy of material manipulation."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--axion",default="./axion")
parser.add_argument("--output",type=Path,required=True)
parser.add_argument("--retain",type=Path)
parser.add_argument("--seeds",nargs="+",type=int,default=[1877,5569,8123,31139])
parser.add_argument("--seasons",type=int,default=384)
parser.add_argument("--capacity",type=int,default=8192)
parser.add_argument("--subjects",type=int,default=1024)
args=parser.parse_args()
def field(*options):
    result=subprocess.run([args.axion,"floodplain",*map(str,options)],text=True,check=True,stdout=subprocess.PIPE)
    return result.stdout.splitlines()
def records(output,key):
    return [dict(x.split("=",1) for x in line.split()) for line in output if line.startswith(key+"=")]
with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-workshop-") as stage:
    parents=Path(stage)/"brood.flood"
    with args.output.open("w",newline="") as out:
        writer=None
        for seed in args.seeds:
            for treatment in ("provisioned","fostered"):
                f=field("--side",64,"--seed",seed,"--work-seasons",args.seasons,"--work-capacity",args.capacity,
                        "--work-days",32,"--work-span",2,"--work-recruit",treatment,"--steps",0,"--save",parents)
                brood,=records(f,"work_descendants")
                for span,shown in ((2,0),(2,8),(2,32),(4,32)):
                    q=field("--load",parents,"--work-probes",32,"--work-subjects",args.subjects,"--work-span",span,"--work-shown",shown,
                            "--work-test-seed",seed^0xb713,"--steps",0)
                    for r in records(q,"construction_probe"):
                        row=dict(seed=seed,recruitment=treatment,brood=brood["work_descendants"],**r)
                        if writer is None:writer=csv.DictWriter(out,fieldnames=row.keys(),lineterminator="\n");writer.writeheader()
                        writer.writerow(row)
                if args.retain and seed==args.seeds[0] and treatment=="provisioned":
                    import shutil
                    shutil.copyfile(parents,args.retain)
                out.flush();print(seed,treatment,brood["work_descendants"],flush=True)
