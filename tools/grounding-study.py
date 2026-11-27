#!/usr/bin/env python3
"""Select auditory plasticity and measure transfer in independently keyed cultures."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--axion",default="./axion")
p.add_argument("--output",required=True,type=Path)
p.add_argument("--seeds",nargs="+",type=int,default=[91571,87713,244399])
p.add_argument("--worlds",type=int,default=64)
args=p.parse_args()
rows=[]
with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-grounding-study-") as tmp:
    def observe(label,seed,*options):
        cmd=[args.axion,"ground","--dialect","shuffled","--episodes","512","--probes","128","--worlds",str(args.worlds),"--seed",str(seed),*map(str,options)]
        p=subprocess.run(cmd,check=True,text=True,stdout=subprocess.PIPE)
        transfer=next(line for line in p.stdout.splitlines() if line.startswith("scene_transfer="))
        cells=dict(v.split("=",1) for v in transfer.split())
        data={k:int(cells[k]) for k in ("nouns","available_nouns","verbs","available_verbs","exact","opportunities","errors","undecided","cloze","blanks")}
        for line in p.stdout.splitlines():
            if line.startswith("visual_question="):
                q=dict(v.split("=",1) for v in line.split())
                for field in ("correct","trials","wrong"):data[q["visual_question"]+"_"+field]=int(q[field])
        rows.append(dict(condition=label,seed=seed,**data))
        print(label,seed,data["exact"],data["cloze"],flush=True)
    for seed in args.seeds:
        for selector in ("select","drift"):
            animal=Path(tmp)/(selector+".mind")
            observe(selector+"_breeding",seed,"--origin","random","--selection",selector,"--generations",32,"--population",96,"--save",animal)
            new_seed=seed^0x53a91
            observe(selector+"_new_cultures",new_seed,"--load",animal)
            if selector=="select":
                observe("unpaired",new_seed,"--load",animal,"--pairing","shuffled")
                observe("broken_order",new_seed,"--load",animal,"--pairing","scrambled")
        observe("inert",seed^0x53a91,"--origin","inert")
        with args.output.open("w",newline="") as f:
            w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
