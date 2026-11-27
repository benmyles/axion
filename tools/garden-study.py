#!/usr/bin/env python3
"""Measure growing, crop inheritance and acquired goal policies in the CUDA commons."""
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
SEEDS=(217217,541197,811945,1065553,1409311,1810069,2089363,2470393)
FIELDS=("seed","arm","residents","generation","alive","deposited","meals","work","planned_work","failed_bites","contests","appetites","taste_observations",
        "settled_fields","policy_builds","seed_genomes","seed_founders","edits","immigrants","edible_at_reset","reset_meals","bytes","gpu_ms")

def seasons(a,seed,arm,families,copies,days,gens,args):
    command=[BIN,"commons","--channel","garden","--load",a.load,"--seed",seed,"--families",families,"--replicates",copies,"--days",days,"--generations",gens,
             "--reserve",64,"--calories",24,"--rations",8,"--price",1,"--volatility",0,"--exposures",0,"--founders","varied","--pollen",2,*args]
    result=subprocess.run(list(map(str,command)),text=True,capture_output=True,check=True)
    size=0;crop=None
    for line in result.stdout.splitlines():
        row=dict(re.findall(r'([^ =]+)=([^ ]+)',line))
        if "simulation_bytes" in row:size=row["simulation_bytes"]
        if "greenhouse_generation" in row:crop=row
        if "commons_generation" in row:
            if row["commons_generation"]!=crop["greenhouse_generation"]:raise RuntimeError("unmatched season")
            yield [seed,arm,families*copies,row["commons_generation"],row["alive"],row["deposited"],row["meals"],crop["work"],crop["planned_work"],
                   row["wrong_traps"],row["lost_contests"],crop["appetites"],crop["tastings"],crop["settled_fields"],crop["policy_builds"],
                   crop["plant_genomes"],crop["plant_founders"],crop["inherited_edits"],crop["immigrants"],crop["edible_at_reset"],crop["reset_meals"],size,row["GPU_ms"]]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("mode",choices=("ecology","scale","cache","transplants"))
    p.add_argument("--load",type=Path,default=ROOT/"examples/gardener.mind");p.add_argument("--seeds",nargs="+",type=int,default=SEEDS)
    p.add_argument("--max-residents",type=int,default=262144);a=p.parse_args();out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(FIELDS)
    if a.mode=="ecology":
        for seed in a.seeds:
            for arm,args in (("intact",[]),("episodic",["--crop-model","episodic"]),("one-beat",["--crop-model","one-beat"]),
                    ("no-planning",["--crop-model","blind"]),("frozen",["--credit","frozen"]),("private",["--observe-workers","no"]),
                    ("fixed-crops",["--flora","fixed"]),("drifting-crops",["--flora","drift"]),("drifting-animals",["--breeding","drift"])):
                out.writerows(seasons(a,seed,arm,64,4,96,8,args));sys.stdout.flush()
    elif a.mode=="scale":
        for n in (1024,16384,65536,262144,1048576):
            if n>a.max_residents:continue
            for seed in a.seeds:
                out.writerows(seasons(a,seed,"self-directed",min(65536,n),max(1,n//65536),32,1,[]));sys.stdout.flush()
    elif a.mode=="transplants":
        with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp")) as scratch:
            root=Path(scratch)
            for seed in a.seeds:
                original=root/"ancestral.seeds";evolved=root/"descendant.seeds";young=root/"newborn.mind"
                list(seasons(a,seed,"baseline",64,4,1,1,["--save-seeds",original]))
                list(seasons(a,seed,"grow",64,4,96,8,["--save-seeds",evolved,"--save-newborn",young]))
                founder=a.load
                for label,brain,plants,credit in (("ancestral",founder,original,"intact"),("new-brain",young,original,"intact"),
                        ("new-crops",founder,evolved,"intact"),("both",young,evolved,"intact"),("frozen-old-crops",founder,original,"frozen"),
                        ("frozen-new-crops",founder,evolved,"frozen"),("frozen-new-brain",young,original,"frozen"),("frozen-both",young,evolved,"frozen")):
                    a.load=brain
                    rows=list(seasons(a,seed^0xd961d1,label,64,4,96,1,["--flora","fixed","--plant-seeds",plants,"--credit",credit,"--founders","identical"]))
                    for row in rows:row[0]=seed
                    out.writerows(rows);sys.stdout.flush()
                a.load=founder
    else:
        with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp")) as scratch:
            for seed in a.seeds:
                reference=None
                for layout in ("cached","rebuild"):
                    path=Path(scratch)/(layout+".mind")
                    rows=list(seasons(a,seed,layout,256,4,96,3,["--exposures",24,"--crop-policy",layout,"--save",path]))
                    out.writerows(rows);sys.stdout.flush()
                    signature=(path.read_bytes(),[row[2:14]+row[15:-1] for row in rows])
                    if reference is not None and signature!=reference:raise RuntimeError("cached policy changed a resident or its world")
                    reference=signature
if __name__=="__main__":main()
