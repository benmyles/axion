#!/usr/bin/env python3
"""Measure tidal foraging, intergenerational teaching and deployment on the public CUDA commons."""
import argparse
import csv
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
SEEDS=(213097,437531,658741,936931,1216019,1542853,1834021,2198659)

def seasons(a,arm,seed,families,copies,days,generations,extra):
    command=[BIN,"commons","--load",a.load,"--channel","estuary","--route-depth",3,"--families",families,"--replicates",copies,
             "--days",days,"--generations",generations,"--seed",seed,"--reserve",32,"--rations",5,"--calories",5,
             "--volatility",0,"--price",2,"--founders","varied",*extra]
    run=subprocess.run(list(map(str,command)),capture_output=True,text=True,check=True)
    mechanism={};device={}
    for line in run.stdout.splitlines():
        r=dict(re.findall(r'([^ =]+)=([^ ]+)',line))
        if "simulation_bytes" in r:device=r
        if "estuary_generation" in r:mechanism=r
        if "commons_generation" in r:
            assert r["commons_generation"]==mechanism["estuary_generation"]
            yield (seed,arm,families*copies,r["commons_generation"],r["alive"],r["meals"],r["deposited"],r["experiments"],r["diagnostic"],
                   mechanism["correct_traps"],mechanism["placements"],mechanism["lessons"],mechanism["social_lessons"],
                   mechanism["two_hops"],mechanism["four_hops"],mechanism["eight_hops"],device["simulation_bytes"],r["GPU_ms"])

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("mode",choices=("ecology","scale","layouts"))
    p.add_argument("--load",type=Path,default=ROOT/"examples/cartographer.mind")
    p.add_argument("--seeds",nargs="+",type=int,default=SEEDS);p.add_argument("--max-residents",type=int,default=262144)
    a=p.parse_args();out=csv.writer(sys.stdout,lineterminator="\n")
    out.writerow(("seed","arm","residents","generation","alive","meals","offspring_energy","inspections","informative",
                  "correct_traps","placements","lessons","social_lessons","two_hops","four_hops","eight_hops","allocated_bytes","gpu_ms"))
    if a.mode=="ecology":
        for seed in a.seeds:
            for arm,extra in (("local",["--mentors","local"]),("silent",["--mentors","silent"]),("displaced",["--mentors","displaced"]),
                              ("no-reuse",["--reuse","cut","--mentors","local"]),("frozen",["--credit","frozen","--mentors","local"]),
                              ("drift",["--breeding","drift","--mentors","local"]),("changing",["--mentors","local","--volatility",12])):
                out.writerows(seasons(a,arm,seed,64,4,64,8,["--tides","clock",*extra]));sys.stdout.flush()
    elif a.mode=="scale":
        for n in (1024,16384,65536,262144,1048576):
            if n>a.max_residents:continue
            for seed in a.seeds:
                out.writerows(seasons(a,"self-directed",seed,min(n,65536),max(1,n//65536),32,1,[]));sys.stdout.flush()
    else:
        with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp")) as scratch:
            for seed in a.seeds:
                expected=None
                for layout in ("direct","warp"):
                    image=Path(scratch)/f"{layout}.mind"
                    rows=list(seasons(a,layout,seed,256,4,64,3,["--route-layout",layout,"--mentors","local","--save",image]))
                    out.writerows(rows);sys.stdout.flush()
                    data=image.read_bytes(),[x[:1]+x[2:-1] for x in rows]
                    if expected is not None and data!=expected:raise RuntimeError("device layouts diverged")
                    expected=data
if __name__=="__main__":main()
