#!/usr/bin/env python3
"""Teach previously unheard forms by pointing, then test them on every scene."""
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
BEARINGS=("north","east","south","west")
FORMS=("brook","loam","keel","tarn","reed","loquat","birch","sedge","cairn","flint","cider","moss")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--seeds",nargs="+",type=int,default=[1511]);p.add_argument("--raise-pair",action="store_true")
    p.add_argument("--inert",action="store_true");p.add_argument("--forms",type=int,choices=range(1,13),default=12)
    a=p.parse_args();out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","stage","task","correct","items","silent"))
    for seed in a.seeds:
        with tempfile.TemporaryDirectory(prefix="axion-pointing-",dir=os.environ.get("TMPDIR","/var/tmp")) as work:
            folder=Path(work);asker=ROOT/"examples/interpreter.mind";speaker=ROOT/"examples/discourse-speaker.mind"
            if a.raise_pair:
                asker=folder/"born-asker.mind";speaker=folder/"born-speaker.mind"
                subprocess.run([BIN,"dialogue","--pairs","1","--seed",str(seed),"--facets","2","--vision","gaze","--language","compass","--references","again,other",
                                "--probes","128","--scenes","withheld","--hearing","learned","--play","1024","--practice","0","--reversal","off","--save-speaker",speaker,"--save-listener",asker],
                               stdout=subprocess.DEVNULL,check=True)
            rng=random.Random(seed^0x347bc923);names=list(FORMS[:a.forms]);ideas=[i%2 for i in range(len(names))];rng.shuffle(ideas)
            for stage in ("before","taught","returned","revised"):
                stream=[];expect=[]
                if stage=="taught":
                    for name,idea in zip(names,ideas):stream.append(f"/teach {name} {idea+1}\n")
                if stage=="revised":
                    for i in range(0,len(names),2):
                        ideas[i]^=1;stream.append(f"/teach {names[i]} {ideas[i]+1}\n")
                cases=[(n,i,j) for n in range(len(names)) for i in range(4) for j in range(4)]
                rng.shuffle(cases)
                for k,(n,i,j) in enumerate(cases):
                    scene=(BEARINGS[i],BEARINGS[j]);term=names[n]
                    text=(f"{term} for me now?",f"Where could the {term} be?",f"Tell me about the {term}!")[k%3]
                    stream.extend([f"/scene {scene[0]} {scene[1]}\n",text+"\n"])
                    expect.append(("new_word","(silence)" if stage=="before" else scene[ideas[n]]))
                    if k%8==0:
                        stream.extend(["Where is the food now?\n","The other one, please?\n","What about the orchestra?\n"])
                        expect.extend([("original",scene[0]),("reference",scene[1]),("unheard","(silence)")])
                saved=folder/(stage+".mind")
                args=[BIN,"talk","--seed",str(seed),"--listener",asker,"--speaker",speaker,"--hearing","learned","--save-listener",saved]
                if a.inert:args+=["--auditory-origin","inert"]
                r=subprocess.run(args,input="".join(stream)+"/quit\n",text=True,stdout=subprocess.PIPE,check=True)
                answers=[line.removeprefix("animal> ") for line in r.stdout.splitlines() if line.startswith("animal> ")]
                if len(answers)!=len(expect):raise RuntimeError("missing spoken or silent response")
                acc=collections.defaultdict(lambda:[0,0,0])
                for word,(task,true) in zip(answers,expect):
                    v=acc[task];v[0]+=word==true;v[1]+=1;v[2]+=word=="(silence)"
                for task,v in acc.items():out.writerow((seed,stage,task,*v))
                asker=saved;sys.stdout.flush()
if __name__=="__main__":main()
