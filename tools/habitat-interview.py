#!/usr/bin/env python3
"""Score words against the four measured currents in a changing CUDA habitat."""
import argparse
import csv
import gzip
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
BEARINGS=("north","east","south","west")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--world",type=Path,default=ROOT/"examples/confluence.eco")
    p.add_argument("--recall",action="store_true",help="ask for previous sightings after every change of viewpoint")
    p.add_argument("--follow",action="store_true",help="let an independently acting resident carry the viewpoint")
    p.add_argument("--predict",action="store_true",help="also score hypothetical one-quarter turns from the measured currents")
    p.add_argument("--exchange",action="store_true",help="also score cross-object forecasts against the other resource's physical currents")
    p.add_argument("--seeds",nargs="+",type=int,default=[61057,124753,301237,619027,1289137,2268047,3065519,4101283])
    a=p.parse_args()
    with a.world.open("rb") as w:side=struct.unpack_from("<I",w.read(20),16)[0]
    out=csv.writer(sys.stdout,lineterminator="\n");header=["world","seed","tick","x","y","requested","food_currents","spring_currents","expected","spoken","match"]
    if a.follow:header.extend(("resident","founder","born","generation"))
    out.writerow(header)
    for seed in a.seeds:
        rng=random.Random(seed);commands=[];goals=[]
        for t in range(8):
            if not a.follow or not t:commands.append(f"/visit {rng.randrange(side)} {rng.randrange(side)}\n")
            if a.follow:
                commands.append("/follow\n" if not t else f"/advance {rng.randrange(1,5)}\n")
            elif t%2:commands.append(f"/advance {rng.randrange(64,513)}\n")
            order=rng.randrange(2)
            if a.recall:
                commands.extend([("Where was the food?\n","Where was the water?\n")[order],"The other, remember?\n"])
                goals.extend(((order,"prior"),(order^1,"prior")))
            commands.extend([("Where is the food now?\n","Where is the water now?\n")[order],"And the other one please?\n"])
            goals.extend(((order,"current"),(order^1,"current")))
            if a.predict:
                commands.extend(("Where would the food be after clockwise?\n","Where would the water be after clockwise?\n"))
                goals.extend(((0,"clockwise"),(1,"clockwise")))
            if a.exchange:
                commands.extend(("If the food and water exchanged places, where would the food be?\n","If they swapped, where would the water be?\n"))
                goals.extend(((0,"exchange"),(1,"exchange")))
        result=subprocess.run([BIN,"talk","--world",a.world],input="".join(commands)+"/quit\n",text=True,capture_output=True,check=True)
        path=re.search(r"Run [0-9]+ · (.+/plate\.webp)",result.stderr)
        if not path:raise RuntimeError("missing measured field record")
        with gzip.open(Path(path[1]).parent/"observations.json.gz") as f:data=json.load(f)
        spoken=[s.removeprefix("animal> ") for s in result.stdout.splitlines() if s.startswith("animal> ")]
        measurements=[]
        if data.get("habitat_views"):
            views=data["habitat_views"];cursor=0
            for answered in range(len(spoken)):
                while cursor+1<len(views) and views[cursor+1]["after"]<=answered:cursor+=1
                v=views[cursor]
                measurements.append(dict(v,habitat_view=v["tick"],food=",".join(map(str,v["food"])),water=",".join(map(str,v["water"]))))
        else:
            for line in data["lines"]:
                if line.startswith("habitat_view="):
                    view=dict(x.split("=",1) for x in line.split() if "=" in x)
                if line.startswith("interview=turn"):measurements.append(view)
        if len(measurements)!=len(goals) or len(spoken)!=len(goals):raise RuntimeError("missing directional observation at a sampled site")
        retained=[None,None]
        for view,(goal,effect),choice in zip(measurements,goals,spoken):
            fields=[list(map(int,view[k].split(","))) for k in ("food","water")]
            source=goal^int(effect=="exchange");field=fields[source]
            if effect!="prior" and max(field):retained[source]=BEARINGS[field.index(max(field))]
            true=retained[source] or "(silence)";label=("food","water")[goal]
            if effect=="clockwise" and true in BEARINGS:true=BEARINGS[(BEARINGS.index(true)+1)&3]
            row=[a.world.name,seed,view["habitat_view"],view["x"],view["y"],("" if effect=="current" else effect+"-")+label,view["food"],view["water"],true,choice,int(choice==true)]
            if a.follow:row.extend(view.get(k,"") for k in ("resident","founder","born","generation"))
            out.writerow(row)
        sys.stdout.flush()
if __name__=="__main__":main()
