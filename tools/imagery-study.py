#!/usr/bin/env python3
"""Measure predictive replay on fresh CUDA apparatuses and live spoken action sequences."""
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
FRESH=(256109,430897,645397,1011991,1478161,2103079,2922043,3907067)

def assay(a):
    fields="seed dynamics observations length arm probes single pair sequence changed changed_sequence".split()
    out=csv.DictWriter(sys.stdout,fields,lineterminator="\n");out.writeheader()
    for seed in a.seeds:
        args=[BIN,"imagine","--load",a.speaker,"--dynamics",a.dynamics,"--worlds",str(a.worlds),"--length",str(a.length),
              "--observations",str(a.observations),"--probes",str(a.probes),"--seed",str(seed)]
        for line in subprocess.check_output(args,text=True).splitlines():
            if not line.startswith("imagery="):continue
            d=dict(s.split("=",1) for s in line.split());d.update(arm=d["imagery"],observations=a.observations)
            out.writerow({k:d[k] for k in fields})
        sys.stdout.flush()

def interview(a,folder):
    blank=folder/"new.mind"
    subprocess.run([BIN,"imagine","--load",a.speaker,"--origin","fresh","--observations","0","--probes","1","--worlds","1","--save",blank],
                   stdout=subprocess.DEVNULL,check=True)
    out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","replay","dynamics","condition","correct","items","quiet","different_from_scene","correct_when_different"))
    names=("vexi","norul","mepp","fayn")
    for seed in a.seeds:
        rng=random.Random(seed);commands=[];trials=[]
        def scene(x,y):commands.append(f"/scene {BEARINGS[x]} {BEARINGS[y]}")
        def ask(question,expected,actual,kind):
            commands.append(question);trials.append((expected,actual,kind))
        def show(word,start,end):commands.append(f"/show-motion {word} {BEARINGS[start]} {BEARINGS[end]}")
        show(names[0],0,1);scene(2,0);ask(f"Where is the food after {names[0]}?",4,2,"unobserved_transition")
        show(names[0],2,3);ask(f"Where is the food after {names[0]}?",3,2,"one_example")
        for phase in ("acquired","reassigned"):
            tables=[rng.sample(range(4),4) if a.dynamics=="permutation" else [rng.randrange(4) for _ in range(4)] for _ in range(4)]
            demonstrations=[(op,b) for op in range(4) for b in range(4)];rng.shuffle(demonstrations)
            for op,b in demonstrations:show(names[op],b,tables[op][b])
            def simulate(b,sequence):
                for op in sequence:b=tables[op][b]
                return b
            for _ in range(a.probes):
                b,water,shifted,_unused=rng.sample(range(4),4);scene(b,water)
                ask("Where is the food?",b,b,"visible")
                for n in (1,2,8,a.length):
                    sequence=rng.choices(range(4),k=n);answer=simulate(b,sequence)
                    ask("Where would the food be after "+" ".join(names[op] for op in sequence)+"?",answer,b,f"{phase}_{n}")
                for old in range(4):
                    pairs=[(i,j) for i in range(4) for j in range(i+1,4) if simulate(old,(i,j))!=simulate(old,(j,i))]
                    if pairs:break
                if pairs:
                    i,j=rng.choice(pairs);scene(old,water)
                    ask(f"Food after {names[i]} {names[j]}?",simulate(old,(i,j)),old,"ordered_pair")
                    ask(f"Food after {names[j]} {names[i]}?",simulate(old,(j,i)),old,"opposite_order")
                    b=old
                scene(shifted,water);ask("Where was the food?",b,shifted,"real_memory_after_replay")
        result=subprocess.run([BIN,"talk","--speaker",blank,"--listener",a.listener,"--imagination",a.replay],input="\n".join(commands+["/quit",""]),text=True,stdout=subprocess.PIPE,check=True)
        replies=[s[8:] for s in result.stdout.splitlines() if s.startswith("animal> ")]
        if len(replies)!=len(trials):raise RuntimeError("missing spoken forecast")
        totals={}
        for reply,(truth,actual,kind) in zip(replies,trials):
            v=totals.setdefault(kind,[0]*5);correct=reply==(BEARINGS+("(silence)",))[truth]
            v[0]+=correct;v[1]+=1;v[2]+=reply=="(silence)";v[3]+=truth!=actual;v[4]+=bool(correct and truth!=actual)
        for kind,v in totals.items():out.writerow((seed,a.replay,a.dynamics,kind,*v))
        sys.stdout.flush()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("mode",choices=("assay","interview"))
    p.add_argument("--speaker",type=Path,default=ROOT/"examples/observer.mind");p.add_argument("--listener",type=Path,default=ROOT/"examples/inquirer.mind")
    p.add_argument("--seeds",nargs="+",type=int,default=FRESH);p.add_argument("--dynamics",choices=("arbitrary","permutation","compass"),default="permutation")
    p.add_argument("--length",type=int,default=32);p.add_argument("--observations",type=int,default=16);p.add_argument("--probes",type=int,default=16);p.add_argument("--worlds",type=int,default=256)
    p.add_argument("--replay",choices=("intact","muted"),default="intact");a=p.parse_args()
    if a.mode=="assay":assay(a);return
    if a.dynamics=="compass":p.error("the spoken study teaches independently generated apparatuses")
    if a.length<3 or a.probes<1:p.error("positive probes and a sequence length of at least three are required")
    with tempfile.TemporaryDirectory(prefix="axion-motion-",dir=os.environ.get("TMPDIR","/var/tmp")) as d:interview(a,Path(d))
if __name__=="__main__":main()
