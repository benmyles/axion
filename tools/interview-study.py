#!/usr/bin/env python3
"""Ask grounded questions through the public text interface and score audible replies."""
import argparse
import csv
import os
from pathlib import Path
import random
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
BEARINGS=("north","east","south","west")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--speaker",type=Path,default=ROOT/"examples/compass-speaker.mind")
    p.add_argument("--asker",type=Path,default=ROOT/"examples/compass-asker.mind")
    p.add_argument("--seeds",nargs="+",type=int,default=[1511])
    p.add_argument("--nouns",nargs=2,default=["food","water"])
    p.add_argument("--raise-pair",action="store_true")
    p.add_argument("--play",type=int,default=192)
    p.add_argument("--swap-training",action="store_true")
    args=p.parse_args()
    if args.swap_training and not args.raise_pair:p.error("swapping word meanings requires a newly raised pair")
    out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","form","role","correct","questions"))
    for seed in args.seeds:
        with tempfile.TemporaryDirectory(prefix="axion-interview-",dir=os.environ.get("TMPDIR","/var/tmp")) as work:
            speaker,asker=args.speaker,args.asker
            base=[BIN,"dialogue","--seed",str(seed),"--pairs","1","--facets","2","--vision","gaze","--language","compass",
                  "--scenes","withheld","--practice","0","--reversal","off"]
            if args.raise_pair:
                speaker=Path(work)/"speaker.mind";asker=Path(work)/"asker.mind"
                taught=list(reversed(args.nouns)) if args.swap_training else args.nouns
                subprocess.run(base+["--play",str(args.play),"--nouns",",".join(taught),"--save-speaker",speaker,"--save-listener",asker],
                               stdout=subprocess.DEVNULL,check=True)
            cases=[(i,j,t,f)for i in range(4)for j in range(4)for t in range(2)for f in range(5)]
            random.Random(seed^0x6d8bca21).shuffle(cases)
            stream=[]
            for a,b,t,form in cases:
                noun=args.nouns[t]
                text=(noun,f"Where is the {noun}?",f"And the {noun.upper()}?",f"Is {noun} nearby?",f"Tell me about the {noun}!")[form]
                stream+= [f"/scene {BEARINGS[a]} {BEARINGS[b]}\n",text+"\n"]
            result=subprocess.run(base+["--speaker",speaker,"--listener",asker,"--interview"],input="".join(stream)+"/quit\n",text=True,stdout=subprocess.PIPE,check=True)
            replies=[s.removeprefix("animal> ") for s in result.stdout.splitlines() if s.startswith("animal> ")]
            if len(replies)!=len(cases):raise RuntimeError("missing audible reports")
            counts={}
            for (a,b,t,f),word in zip(cases,replies):
                role=t^int(args.swap_training)
                v=counts.setdefault((f,t),[0,0]);v[0]+=word==BEARINGS[(a,b)[role]];v[1]+=1
            for (f,t),(hits,n) in sorted(counts.items()):
                out.writerow((seed,("name","where","follow_up","trailing_word","tell_me")[f],args.nouns[t],hits,n))
            sys.stdout.flush()
if __name__=="__main__":main()
