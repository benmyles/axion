#!/usr/bin/env python3
"""Follow one natural population across seasons, frontiers and restart boundaries."""
import argparse
import csv
from pathlib import Path
import subprocess

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--axion",default="./axion");parser.add_argument("--load",type=Path,required=True)
parser.add_argument("--save",type=Path,required=True);parser.add_argument("--output",type=Path,required=True)
parser.add_argument("--epochs",type=int,default=8);parser.add_argument("--ticks",type=int,default=65536)
parser.add_argument("--frontier",type=int,default=128)
a=parser.parse_args()
if a.epochs<1 or a.ticks<1:parser.error("epochs and ticks must be positive")
source=a.load
with a.output.open("w",newline="") as out:
    writer=None
    for epoch in range(a.epochs):
        cmd=[a.axion,"floodplain","--load",source,"--save",a.save,"--frontier",a.frontier,"--steps",a.ticks,
             "--report-every",max(1,a.ticks//4),"--legacy",128,"--assay-calls"]
        proc=subprocess.run(list(map(str,cmd)),check=True,text=True,stdout=subprocess.PIPE);obs={}
        for line in proc.stdout.splitlines():
            if line.startswith(("flood_tick=","flood_census=","ecological_legacy=","field_hearing=cleared","flood_seconds=")):
                obs.update(dict(entry.split("=",1) for entry in line.split()))
        keys=("flood_tick","living","births","deaths","families","deepest","distinct_bodies","expressed_tapes","persistent_bodies","persistent_brains",
              "multi_module_residents","producer_genotypes","producer_tables","recruits","plant_mutations","mixed_faces","supported",
              "sites","water_sites","food_sites","experienced","changed_actions","site_ticks","simulation_Msite_ticks_per_s")
        if writer is None:writer=csv.DictWriter(out,fieldnames=keys);writer.writeheader()
        writer.writerow({k:obs[k] for k in keys});out.flush()
        print(epoch+1,obs["flood_tick"],obs["living"],obs["deepest"],flush=True)
        source=a.save
