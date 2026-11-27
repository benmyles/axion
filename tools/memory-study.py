#!/usr/bin/env python3
"""Test source-dependent visual learning in the CUDA nursery and in spoken interviews."""
import argparse
import csv
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
BEARINGS=("north","east","south","west")
HOLDOUT=(197831,405211,716003,1106921,1952357,2439697,3000793,3619979)

def numbers(line):
    return dict(s.split("=",1) for s in line.split() if "=" in s)

def memory_lab(a,work):
    fields="seed breeding generations probe_seed arm worlds delay spacing trials one delay_correct noise revision faint empty empty_questions low high expression".split()
    out=csv.DictWriter(sys.stdout,fields,lineterminator="\n")
    out.writeheader();best=None;certificates=[]
    for seed in a.seeds:
        breed=a.mode=="breed";saved=work/f"{seed}.mind"
        args=[BIN,"recollect","--load",a.speaker,"--candidates",str(a.candidates),"--worlds",str(a.worlds),
              "--probe-worlds",str(a.probe_worlds),"--cycles",str(a.cycles),"--delay",str(a.delay),"--spacing",a.spacing,"--objective",a.objective,
              "--generations",str(a.generations if breed else 0),"--origin",a.origin if breed else "inherited",
              "--seed",str(seed),"--probe-seed",str(a.probe_seed if breed else seed),"--breeding",a.breeding]
        if breed:args+=["--save",saved]
        output=subprocess.run(args,stdout=subprocess.PIPE,text=True,check=True).stdout
        lines=[numbers(s) for s in output.splitlines()];genes=next(r for r in lines if "rule_low" in r)
        proof=[r for r in lines if "memory_proof" in r]
        if a.proof_report:
            for r in proof:certificates.append(dict(seed=seed,**r))
            keys="seed memory_proof reachable_rows checked violations first_depth expected actual row".split()
            with a.proof_report.open("w") as file:
                writer=csv.DictWriter(file,keys,lineterminator="\n");writer.writeheader();writer.writerows(certificates)
        for r in lines:
            if "recollection" not in r:continue
            out.writerow(dict(seed=seed,breeding=a.breeding,generations=genes["generations"],probe_seed=r["seed"],arm=r["recollection"],worlds=r["worlds"],
                              delay=r["delay_events"],spacing=r["spacing"],trials=r["questions"],one=r["one"],delay_correct=r["delay"],noise=r["noise"],revision=r["revision"],
                              faint=r["faint"],empty=r["empty"],empty_questions=r["empty_questions"],low=genes["rule_low"],high=genes["rule_high"],expression=genes["expression"]))
            if r["recollection"]=="intact" and breed:
                values=[int(r[k]) for k in ("one","delay","noise","revision","faint")];rank=(min(values),sum(values))
                if a.objective=="invariant":
                    proportions=[1-int(c["violations"])/int(c["checked"]) for c in proof]
                    rank=(8*min(proportions)+sum(proportions),*rank)
                if best is None or rank>best:
                    best=rank
                    if a.save:shutil.copyfile(saved,a.save)
        sys.stdout.flush()

def interview(a):
    out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","cut","delay","condition","correct","items","silence","matches_visible_world"))
    for seed in a.seeds:
        rng=random.Random(seed);script=[];expected=[]
        def scene(first,second):script.append(f"/scene {BEARINGS[first]} {BEARINGS[second]}\n")
        def ask(text,truth,visible,condition):
            script.append(text+"\n");expected.append((truth,visible,condition))
        script+=["/cover both\n"];scene(0,1);ask("Where is the food?",4,0,"unseen")
        for _ in range(a.cycles):
            old,misleading,revised,weak=rng.sample(range(4),4)
            other=rng.randrange(4)
            script+=["/uncover both\n","/bright both\n"];scene(old,other)
            ask("Where is the food?",old,old,"see")
            script+=["/cover 1\n"];scene(misleading,other)
            ask("Again?",old,misleading,"hidden_change")
            delay=rng.randrange(a.delay+1) if a.spacing=="varied" else a.delay
            for j in range(delay):
                other=rng.randrange(4);scene(misleading,other)
                ask("Where is the water?",other,other,"intervening")
            ask("The other one?" if delay else "Where is the food?",old,misleading,"after_delay")
            script+=["/uncover 1\n","/dim 1\n"]
            ask("Where is the food?",misleading,misleading,"see_dim")
            script+=["/cover 1\n"];scene(revised,other)
            ask("Again?",old,revised,"after_dim_outlier")
            script+=["/uncover 1\n","/bright 1\n"]
            ask("Where is the food?",revised,revised,"see_change")
            script+=["/cover 1\n"];scene(weak,other)
            ask("Where is the food?",revised,weak,"revised")
            script+=["/uncover 1\n","/dim 1\n"]
            for _ in range(4):ask("Where is the food?",weak,weak,"weak_sightings")
            script+=["/cover 1\n"];scene(old,other)
            ask("Where is the food?",weak,old,"after_weak_run")
        p=subprocess.run([BIN,"talk","--speaker",a.speaker,"--recall-cut",a.cut],input="".join(script)+"/quit\n",text=True,stdout=subprocess.PIPE,check=True)
        responses=[s.removeprefix("animal> ") for s in p.stdout.splitlines() if s.startswith("animal> ")]
        if len(responses)!=len(expected):raise ValueError("missing spoken report")
        totals={}
        for answer,(truth,visible,kind) in zip(responses,expected):
            c=totals.setdefault(kind,[0]*4);c[0]+=answer==(BEARINGS+("(silence)",))[truth];c[1]+=1;c[2]+=answer=="(silence)";c[3]+=answer==BEARINGS[visible]
        for name,counts in totals.items():out.writerow((seed,a.cut,a.delay,name,*counts))
        sys.stdout.flush()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("mode",choices=("breed","audit","interview"));p.add_argument("--speaker",type=Path,default=ROOT/"examples/discourse-speaker.mind")
    p.add_argument("--seeds",nargs="+",type=int,default=HOLDOUT);p.add_argument("--save",type=Path)
    p.add_argument("--breeding",choices=("select","drift"),default="select");p.add_argument("--origin",choices=("inert","random","inherited"),default="inert")
    p.add_argument("--generations",type=int,default=256);p.add_argument("--candidates",type=int,default=256);p.add_argument("--worlds",type=int,default=8)
    p.add_argument("--probe-worlds",type=int,default=256);p.add_argument("--probe-seed",type=int,default=160481)
    p.add_argument("--delay",type=int,default=6);p.add_argument("--cycles",type=int,default=24)
    p.add_argument("--spacing",choices=("varied","fixed"),default="varied")
    p.add_argument("--objective",choices=("sampled","invariant"),default="sampled")
    p.add_argument("--proof-report",type=Path)
    p.add_argument("--cut",choices=("intact","erased","fixed","source","crossed"),default="intact")
    a=p.parse_args()
    if a.delay<0 or a.cycles<1:p.error("require nonnegative delay and positive cycles")
    if a.mode=="interview":interview(a);return
    with tempfile.TemporaryDirectory(prefix="axion-memory-",dir=os.environ.get("TMPDIR","/var/tmp")) as work:
        memory_lab(a,Path(work))
if __name__=="__main__":main()
