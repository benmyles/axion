#!/usr/bin/env python3
"""Measure learned requests for a present view or an animal's own prior sighting."""
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
WORDS=("is","look","now","was","remember","recall","switch","keep")
OPS=(0,0,0,3,3,3,1,2)
FRESH=(216097,396431,703531,1061911,1391159,2019863,2771131,3765299)

def sessions(a):
    writer=csv.writer(sys.stdout,lineterminator="\n")
    writer.writerow(("seed","pitch","recall_cut","condition","correct","items","quiet","matched_current"))
    for seed in a.seeds:
        rng=random.Random(seed);script=[];trials=[];physical=[0,1];memory=[None,None];source=0
        def scene(x,y):
            physical[:]=[x,y];script.append("/scene "+" ".join(BEARINGS[i] for i in physical))
        def ask(sentence,view,mode,phase):
            nonlocal source
            source=mode
            expected=memory[view] if mode else physical[view]
            if not mode:memory[view]=expected
            trials.append((expected,physical[view],phase));script.append(sentence)
        def fresh_scene():
            # All four bearings differ, so a current answer cannot coincide
            # with either resource's previously observed location by accident.
            pool=[v for v in range(4) if v not in memory]
            scene(*rng.sample(pool,2))
        forms=a.words.split(",")
        if len(forms)!=8:raise ValueError("eight physical teaching forms are required")
        view_words=("food","water")
        ask(f"Where {forms[3]} the food?",0,1,"unseen")
        for _ in range(a.cycles):
            scene(*rng.sample(range(4),2))
            ask(f"Where {forms[0]} the food?",0,0,"establish")
            ask(f"Where {forms[0]} the water?",1,0,"establish")
            fresh_scene()
            ask(f"Where {forms[3]} the food?",0,1,"remember_open")
            ask(f"The other one, {forms[4]}?",1,1,"other_remember")
            ask(f"{forms[1]} at the food.",0,0,"look_open")
            ask(f"The other, {forms[6]}?",1,1,"other_switch")
            # Same words in different orders, with opposite required answers.
            for view in rng.sample(range(2),2):
                fresh_scene();ask(f"{forms[1]} {view_words[view]} {forms[6]}?",view,1,"order_look_switch")
                ask(f"{forms[6]} {view_words[view]} {forms[1]}?",view,0,"order_switch_look")
                fresh_scene()
                chain=[rng.choice((6,7)) for _ in range(128)]
                mode=source^(chain.count(6)&1)
                ask(" ".join([view_words[view],*(forms[t] for t in chain)])+"?",view,mode,"carry_128")
            for length in a.lengths:
                for view in rng.sample(range(2),2):
                    fresh_scene();tokens=rng.choices(range(8),k=length);mode=source
                    for t in tokens:mode=(OPS[t]>>mode)&1
                    # The referent can precede, interrupt or follow the operators.
                    phrase=[forms[t] for t in tokens];phrase.insert(rng.randrange(length+1),view_words[view])
                    ask(" ".join(phrase)+"?",view,mode,f"composition_{length}")
        for _ in range(a.cycles):
            script.append("/teach-focus vellum remember")
            fresh_scene();ask("Food vellum?",0,1,"new_word")
            script.append("/teach-focus vellum look")
            fresh_scene();ask("Vellum food?",0,0,"reassigned")
        result=subprocess.run([BIN,"talk","--listener",a.listener,"--speaker",a.speaker,"--pitch",a.pitch,"--recall-cut",a.cut],
                              input="\n".join(script+["/quit",""]),text=True,stdout=subprocess.PIPE,check=True)
        replies=[s[8:] for s in result.stdout.splitlines() if s.startswith("animal> ")]
        if len(replies)!=len(trials):raise RuntimeError("incomplete interview")
        totals={}
        for reply,(truth,now,kind) in zip(replies,trials):
            row=totals.setdefault(kind,[0]*4);row[0]+=reply==("(silence)" if truth is None else BEARINGS[truth]);row[1]+=1
            row[2]+=reply=="(silence)";row[3]+=reply==BEARINGS[now]
        for kind,counts in totals.items():writer.writerow((seed,a.pitch,a.cut,kind,*counts))
        sys.stdout.flush()

def cohorts(a,folder):
    writer=csv.writer(sys.stdout,lineterminator="\n")
    writer.writerow("seed probe_seed breeding generations length arm caller_rule witness_rule probes single pair chain revised revised_chain".split());best=None
    for seed in a.seeds:
        probe=a.probe_seed if a.mode=="breed" else seed
        args=[BIN,"attention","--listener",a.listener,"--speaker",a.speaker,"--seed",str(seed),"--probe-seed",str(probe),
              "--worlds",str(a.worlds),"--probe-worlds",str(a.probe_worlds),"--candidates",str(a.candidates),"--cycles",str(a.cycles),
              "--generations",str(a.generations if a.mode=="breed" else 0),"--origin",a.origin if a.mode=="breed" else "inherited",
              "--breeding",a.breeding,"--length",str(a.lengths[-1]),"--dialect",str(a.dialect),"--words",a.words]
        if a.mode=="breed":args+=["--save-listener",folder/"reader.mind","--save-speaker",folder/"witness.mind"]
        output=subprocess.check_output(args,text=True)
        values=[dict(field.split("=",1) for field in s.split() if "=" in field) for s in output.splitlines()]
        g=next(r for r in values if "caller_rule" in r)
        for r in values:
            if "attention" not in r:continue
            score=[int(r[k]) for k in ("single","pair","chain","revised","revised_chain")]
            writer.writerow((seed,probe,a.breeding,g["generations"],r["length"],r["attention"],g["caller_rule"],g["witness_rule"],r["probes"],*score))
            rank=(min(score),sum(score))
            if a.mode=="breed" and r["attention"]=="intact" and (best is None or rank>best):
                best=rank
                if a.save_listener:shutil.copyfile(folder/"reader.mind",a.save_listener)
                if a.save_speaker:shutil.copyfile(folder/"witness.mind",a.save_speaker)
        sys.stdout.flush()

def transfer(a):
    out=csv.writer(sys.stdout,lineterminator="\n")
    out.writerow(("seed","trial","task","old_food","seen_food","current_food","current_water","expected","spoken","correct"))
    for seed in a.seeds:
        rng=random.Random(seed);script=[];tests=[]
        for trial in range(a.cycles):
            old,seen,now,water=rng.sample(range(4),4)
            questions=[("current","Where is the food now?",now),
                       ("last_seen","Where was the food?",seen),
                       ("two_sightings_back","Where was the food two sightings ago?",old),
                       ("clockwise","If the food moved one quarter turn clockwise from its current direction, where would the food be?",(now+1)&3),
                       ("opposite","What direction is opposite the food?",(now+2)&3),
                       ("exchange","If the food and water exchanged places now, where would the food be?",water),
                       ("shared_past","Where were the food and water when they shared a location?",old)]
            rng.shuffle(questions)
            for task,question,answer in questions:
                # Re-establish the entire observable history for each task.
                # The two animals see only these scenes and the words sent.
                script.extend((f"/scene {BEARINGS[old]} {BEARINGS[old]}","Where is the food?","Where is the water?",
                               f"/scene {BEARINGS[seen]} {BEARINGS[water]}","Where is the food?","Where is the water?",
                               f"/scene {BEARINGS[now]} {BEARINGS[water]}",question))
                tests.append((trial,task,*(BEARINGS[v] for v in (old,seen,now,water,answer))))
        result=subprocess.run([BIN,"talk","--listener",a.listener,"--speaker",a.speaker],
                              input="\n".join(script+["/quit",""]),text=True,stdout=subprocess.PIPE,check=True)
        replies=[s[8:] for s in result.stdout.splitlines() if s.startswith("animal> ")]
        if len(replies)!=5*len(tests):raise RuntimeError("incomplete transfer interview")
        for test,reply in zip(tests,replies[4::5]):out.writerow((seed,*test,reply,int(test[-1]==reply)))
        sys.stdout.flush()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("mode",choices=("breed","audit","interview","transfer"))
    p.add_argument("--listener",type=Path,default=ROOT/"examples/inquirer.mind");p.add_argument("--speaker",type=Path,default=ROOT/"examples/observer.mind")
    p.add_argument("--save-listener",type=Path);p.add_argument("--save-speaker",type=Path)
    p.add_argument("--words",default=",".join(WORDS));p.add_argument("--dialect",type=int,choices=(0,1),default=0)
    p.add_argument("--lengths",nargs="+",type=int,default=(1,2,8,32));p.add_argument("--cycles",type=int,default=8)
    p.add_argument("--generations",type=int,default=192);p.add_argument("--candidates",type=int,default=256)
    p.add_argument("--worlds",type=int,default=4);p.add_argument("--probe-worlds",type=int,default=256)
    p.add_argument("--seeds",nargs="+",type=int,default=FRESH);p.add_argument("--probe-seed",type=int,default=153739)
    p.add_argument("--origin",choices=("inert","inherited","random"),default="inert");p.add_argument("--breeding",choices=("select","drift"),default="select")
    p.add_argument("--pitch",choices=("intact","inverted","muted"),default="intact");p.add_argument("--cut",choices=("intact","fixed","erased","source","crossed"),default="intact")
    a=p.parse_args()
    if any(n<1 for n in a.lengths) or a.cycles<1:p.error("positive lengths and cycles required")
    if a.mode=="interview":sessions(a);return
    if a.mode=="transfer":transfer(a);return
    with tempfile.TemporaryDirectory(prefix="axion-attention-",dir=os.environ.get("TMPDIR","/var/tmp")) as path:cohorts(a,Path(path))

if __name__=="__main__":main()
