#!/usr/bin/env python3
"""Evaluate references across live text turns, moving both resources every turn."""
import argparse
import collections
import csv
import os
from pathlib import Path
import random
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
DIRECTIONS=("north","east","south","west")

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--seeds",nargs="+",type=int,default=[1511])
    p.add_argument("--speaker",type=Path,default=ROOT/"examples/discourse-speaker.mind")
    p.add_argument("--asker",type=Path,default=ROOT/"examples/discourse-asker.mind")
    p.add_argument("--raise-pair",action="store_true");p.add_argument("--reverse-references",action="store_true")
    p.add_argument("--play",type=int,default=1024)
    p.add_argument("--lengths",type=int,nargs="+",default=[2,8,32,128]);p.add_argument("--chains",type=int,default=4)
    p.add_argument("--hearing",choices=("single","population","learned"),default="single")
    p.add_argument("--readout",choices=("last","peak"),default="last")
    p.add_argument("--suffix",action="store_true");p.add_argument("--stocks",type=Path)
    a=p.parse_args()
    if a.reverse_references and not a.raise_pair:p.error("reversing referential words requires a newly raised pair")
    if min(a.lengths)<2 or a.chains<1:p.error("chains need an anchor and a follow-up")
    if a.stocks:a.stocks.mkdir(parents=True,exist_ok=True)
    out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","chain_length","kind","correct","turns","repeat_old_answer","keep_first_referent"))
    for seed in a.seeds:
        with tempfile.TemporaryDirectory(prefix="axion-reference-",dir=os.environ.get("TMPDIR","/var/tmp")) as work:
            base=[BIN,"dialogue","--seed",str(seed),"--pairs","1","--facets","2","--vision","gaze","--language","compass","--references","again,other",
                  "--scenes","withheld","--probes","128","--practice","0","--reversal","off","--hearing",a.hearing,"--turn-readout",a.readout]
            speaker,asker=a.speaker,a.asker
            if a.raise_pair:
                folder=a.stocks or Path(work);speaker=folder/f"{seed}-speaker.mind";asker=folder/f"{seed}-asker.mind"
                teach=base.copy()
                if a.reverse_references:teach[teach.index("again,other")]="other,again"
                subprocess.run(teach+["--play",str(a.play),"--save-speaker",speaker,"--save-listener",asker],stdout=subprocess.DEVNULL,check=True)
            rng=random.Random(seed^0x41b72d);script=[];targets=[];prior=None
            for length in a.lengths:
                for _ in range(a.chains):
                    anchor=rng.randrange(2);referent=anchor
                    for turn in range(length):
                        places=rng.sample(DIRECTIONS,2)
                        if not turn:
                            text=("Where is the food?","And the water?")[anchor];action="named"
                        else:
                            switch=rng.randrange(2);referent^=switch
                            text=("Again?","The other?")[switch^int(a.reverse_references)]
                            action="switch" if switch else "same"
                        if a.suffix:text=text.rstrip("?")+" for me now?"
                        script.extend([f"/scene {places[0]} {places[1]}\n",text+"\n"])
                        targets.append((length,action,places[referent],prior,places[anchor]))
                        prior=places[referent]
            result=subprocess.run(base+["--speaker",speaker,"--listener",asker,"--interview"],input="".join(script)+"/quit\n",text=True,stdout=subprocess.PIPE,check=True)
            words=[line.removeprefix("animal> ") for line in result.stdout.splitlines() if line.startswith("animal> ")]
            if len(words)!=len(targets):raise RuntimeError("the transcript is missing a reply")
            tally=collections.defaultdict(lambda:[0,0,0,0])
            for (length,action,true,previous,first),word in zip(targets,words):
                row=tally[length,action];row[0]+=word==true;row[1]+=1;row[2]+=previous==true;row[3]+=first==true
            for (length,name),counts in sorted(tally.items()):out.writerow((seed,length,name,*counts))
            sys.stdout.flush()
if __name__=="__main__":main()
