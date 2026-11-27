#!/usr/bin/env python3
"""Teach relational words by pointing, then interrogate private copies through the public CUDA interview."""
import argparse
import csv
import itertools
import os
from pathlib import Path
import random
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
NAMES="pearl acorn shell flint pouch basket crate chest tern ibis swift wren cove quay inlet delta".split()
WORDS="container boards berths vessel harbor region circuit unloads".split()
NOVEL="ketu vosa zeli moru jipa gavu fodi raku".split()
SEEDS=(213097,437531,658741,936931,1216019,1542853,1834021,2198659)

def board(rng,n):
    return [rng.sample(range(n),n) for _ in range(4)]

def maps(b):
    return [f"/map {j} "+" ".join(map(str,row)) for j,row in enumerate(b)]

def follow(b,path,x):
    for p in path:x=b[p][x] if p<4 else b[p&3].index(x)
    return x

def curriculum(rng,novel):
    names=NAMES.copy();words=(NOVEL if novel else WORDS).copy()
    if novel:rng.shuffle(names);rng.shuffle(words)
    atoms=rng.sample(range(4),4) if novel else list(range(4))
    if novel:atoms=[a+4*rng.randrange(2) for a in atoms]
    a,b,c,d=atoms
    paths=[(a,),(b,),(c,),(a,b),(a,b,c),(a,b,c,d),(a,b,c,d)*2,(c^4,)]
    return names,words,paths

def talk(speaker,peer,lines,*opts):
    p=subprocess.run([BIN,"talk","--speaker",speaker,"--listener",peer,*opts],input="\n".join([*lines,"/quit",""]),
                     text=True,capture_output=True,check=True)
    if "Invalid board stimulus" in p.stdout:raise RuntimeError(p.stdout[-1200:])
    return p.stdout.splitlines()

def births(a,folder):
    s=folder/"new-s.mind";p=folder/"new-p.mind"
    subprocess.run([BIN,"infer","--load",a.speaker,"--listener",a.peer,"--generations","0","--newborn","--cycles","1","--exposures","1",
                    "--probe-worlds","1","--save",s,"--save-listener",p],check=True,stdout=subprocess.DEVNULL)
    return s,p

def clarify(a,trained,companion,names,words,paths,seed,destination):
    rng=random.Random(seed^0xafe925);n=len(names)
    with subprocess.Popen([BIN,"talk","--speaker",trained,"--listener",companion,"--save-speaker",destination],
                          stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True) as animal:
        def stimulus(line,prefixes):
            animal.stdin.write(line+"\n");animal.stdin.flush()
            for answer in animal.stdout:
                if answer.startswith(prefixes):return answer.strip()
                if answer.startswith(("Invalid board","board>","animal>","animal asks>","The witness ")):raise RuntimeError(answer.strip())
            raise RuntimeError("relational interview ended before the response")
        try:
            for label,path in zip(words,paths):
                for attempt in range(a.clarify):
                    world=board(rng,n)
                    for line in maps(world):stimulus(line,("board>",))
                    reply=stimulus("/ask "+label,("animal asks>","board>"))
                    if not reply.startswith("animal asks>"):break
                    x=names.index(reply.split()[-1].rstrip("?"));y=follow(world,path,x)
                    stimulus(names[y]+".",("The witness learned from the reply.",))
                    if a.question_log:
                        with a.question_log.open("a",newline="") as log:csv.writer(log,lineterminator="\n").writerow((seed,label,attempt,names[x],names[y]))
        finally:
            animal.stdin.write("/quit\n");animal.stdin.close();animal.stdout.read()
        if animal.wait():raise RuntimeError("the animal failed to save its acquired concepts")

def teach(a,babies,out,as_peer,names,words,paths,seed,sets):
    rng=random.Random(seed);n=len(names)
    text=[f"/board {n} fresh","/gaze both","/verdict 0 no","/verdict 1 yes","/verdict 2 unsure"]
    for i,name in enumerate(names):
        left,right=rng.sample([j for j in range(n) if j!=i],2)
        text.extend([f"/name {name} {left} {i}",f"/name {name} {i} {right}"])
    for label,path in zip(words,paths):
        for _ in range(sets):
            world=board(rng,n);text+=maps(world)
            for x in rng.sample(range(0,n,2),n//2):
                y=follow(world,path,x)
                text.append(f"{names[x]} {label} {names[y]}." if a.spoken else f"/example {label} {x} {y}")
    trained=out.with_suffix(".before") if a.clarify else out
    lines=talk(*babies,text,"--save-listener" if as_peer else "--save-speaker",trained)
    if any(s=="board> 0" for s in lines):raise RuntimeError("an offered naming or relational lesson failed")
    if a.clarify:clarify(a,trained,babies[0 if as_peer else 1],names,words,paths,seed,out)

def perspective(s,p,outs,names,words,paths,seed):
    rng=random.Random(seed);n=len(names);script=[]
    for _ in range(6):
        b=board(rng,n);script+=["/gaze both",*maps(b)]
        altered=[v.copy() for v in b];q=paths[0][0]&3;altered[q]=rng.sample(range(n),n)
        script+=["/gaze witness",maps(altered)[q]]
        candidates=[x for x in range(n) if follow(b,paths[0],x)!=follow(altered,paths[0],x)]
        for x in candidates[:3]:script.append(f"/watch-answer raven {words[0]} {x}")
    home=[list(range(n)) for _ in range(4)]
    # The four material objects occupy four carriers on four vessels at four ports.
    for r in range(3):
        for j in range(4):home[r][4*r+j],home[r][4*r+j+4]=home[r][4*r+j+4],home[r][4*r+j]
    home[3]=list(range(1,n))+[0]
    script+=["/gaze both",*maps(home)]
    lines=talk(s,p,script,"--save-speaker",outs[0],"--save-listener",outs[1])
    if not any(line.startswith("board> 1 ") for line in lines):raise RuntimeError("no independently spoken perspective lesson")

def rear(a,folder,seed,names,words,paths,outs):
    babies=births(a,folder);s=folder/"s.mind";p=folder/"p.mind"
    teach(a,babies,s,False,names,words,paths,seed^0x3651,a.boards)
    teach(a,babies,p,True,names,words,paths,seed^0xa171,a.boards)
    perspective(s,p,outs,names,words,paths,seed^0xb741)

def interview(a,folder,out):
    for seed in a.seeds:
        rng=random.Random(seed);names,words,paths=curriculum(rng,True);n=len(names)
        pair=(folder/"learner.mind",folder/"companion.mind")
        rear(a,folder,seed,names,words,paths,pair)
        text=[];truth=[]
        def ask(line,kind,answer):text.append(line);truth.append((seed,kind,line,answer))
        for trial in range(a.trials):
            physical=board(rng,n);text+=["/gaze both",*maps(physical)]
            x=rng.choice(range(1,n,2))
            for w,path in zip(words,paths):
                y=follow(physical,path,x)
                ask(f"Which {w} for {names[x]}?",f"walk_{len(path)}",names[y])
                ask(f"Does {names[x]} {w} {names[y]}?","affirmation","yes")
                wrong=(y+1+rng.randrange(n-1))%n
                ask(f"Does {names[x]} {w} {names[wrong]}?","denial","no")
            sequence=rng.choices(range(8),k=12);y=x
            for word in sequence:y=follow(physical,paths[word],y)
            ask(f"{names[x]} through "+" then ".join(words[i] for i in sequence)+"?","composition",names[y])
            q=paths[2][0]&3;altered=[v.copy() for v in physical]
            while follow(altered,paths[4],x)==follow(physical,paths[4],x):altered[q]=rng.sample(range(n),n)
            for gaze in ("witness","peer","both","neither"):
                text+=["/gaze both",maps(physical)[q],"/gaze "+gaze,maps(altered)[q]]
                original=names[follow(physical,paths[4],x)];fresh=names[follow(altered,paths[4],x)]
                ask(f"What {words[4]} for {names[x]}?","own_"+gaze,fresh if gaze in ("both","witness") else original)
                ask(f"What {words[4]} would raven expect for {names[x]}?","peer_"+gaze,{"both":fresh,"peer":"unsure","witness":original,"neither":original}[gaze])
            # Statements can update a primitive without an apparatus operation.
            text+=["/gaze both",maps(physical)[q],"/earshot private"]
            s=rng.randrange(n);prior=follow(physical,paths[2],s);dest=(prior+1)%n
            text.append(f"{names[s]} {words[2]} {names[dest]}.")
            ask(f"What {words[2]} for {names[s]}?","private_statement",names[dest] if paths[2][0]<4 else "unsure")
            ask(f"What {words[2]} would raven expect for {names[s]}?","private_recipient",names[prior])
        lines=talk(*pair,text);answers=[s[8:] for s in lines if s.startswith("animal> ")]
        if len(answers)!=len(truth):raise RuntimeError(f"expected {len(truth)} vocal replies, received {len(answers)}")
        for row,heard in zip(truth,answers):out.writerow((*row,heard,int(heard==row[-1])))
        sys.stdout.flush()

def inquiry(a,folder,out):
    babies=births(a,folder);n=len(NAMES)
    for seed in a.seeds:
        rng=random.Random(seed);direction=rng.randrange(2);label="zuno"
        text=[f"/board {n} fresh","/verdict 2 unsure",*(f"/name {name} {j}" for j,name in enumerate(NAMES))]
        for _ in range(4):
            scene=board(rng,n);scene[1]=scene[0].copy();text+=maps(scene)
            text.extend(f"/example {label} {x} {scene[direction][x]}" for x in range(n))
        base=folder/"confounded.mind";talk(*babies,text,"--save-speaker",base)
        for trial in range(a.trials):
            scene=board(rng,n);scene[1]=scene[0].copy();left,right=rng.sample(range(n),2)
            scene[1][left],scene[1][right]=scene[1][right],scene[1][left]
            offers=rng.sample([j for j in range(n) if j not in (left,right)],3)+[left];rng.shuffle(offers)
            replacement=board(rng,n);test=board(rng,n)
            for arm in ("directed","random","spontaneous"):
                cmd=f"Which {label} from {NAMES[left]}?" if arm=="spontaneous" else ("/ask-random " if arm=="random" else "/ask ")+label+" "+" ".join(map(str,offers))
                front=[*maps(scene),cmd];pending=folder/"pending.mind";mate=folder/"pending-peer.mind"
                heard=talk(base,babies[1],front,"--save-speaker",pending,"--save-listener",mate)
                spoken=next((line for line in heard if line.startswith("animal asks> ")),None)
                if not spoken:raise RuntimeError("witness did not request an example")
                x=NAMES.index(spoken.split()[-1].rstrip("?"));y=scene[direction][x]
                # The old display and all intermediate observations are gone
                # before the teacher answers the witness's actual question.
                back=[*maps(replacement),NAMES[y]+".",*maps(test),*(f"Which {label} from {place}?" for place in NAMES)]
                straight=folder/"whole.mind";split=folder/"resumed.mind"
                intact=talk(base,babies[1],[*front,*back],"--save-speaker",straight)
                resumed=talk(pending,mate,back,"--save-speaker",split)
                if straight.read_bytes()!=split.read_bytes():raise RuntimeError("question changed across checkpoint replay")
                predictions=[s[8:] for s in resumed if s.startswith("animal> ")]
                if predictions!=[s[8:] for s in intact if s.startswith("animal> ")][-n:] or len(predictions)!=n:raise RuntimeError("replayed utterances differ")
                exact=sum(answer==NAMES[test[direction][i]] for i,answer in enumerate(predictions))
                out.writerow((seed,trial,arm,NAMES[x],int(x in (left,right)),NAMES[y],exact,n,1))
        sys.stdout.flush()

def enumerate_small(a,out):
    # Exhaust the 6^4 reversible worlds through the ordinary text interface.
    # Compare every fixed one- or two-port alternative on exactly the same boards.
    n=3;functions=list(itertools.permutations(range(n)));paths=[(j,) for j in range(8)]+list(itertools.product(range(8),repeat=2))
    laws=(0,1,2,3)*2;matches=[0]*len(paths);total=0;correct=0
    for first in functions:
        text=["/board 3"];answers=[]
        for tail in itertools.product(functions,repeat=3):
            atlas=(first,*tail);text+=maps(atlas)
            for x in range(n):
                target=follow(atlas,laws,x);answers.append(NAMES[target]);text.append(f"Which circuit for {NAMES[x]}?")
                for i,path in enumerate(paths):matches[i]+=follow(atlas,path,x)==target
        heard=[s[8:] for s in talk(a.speaker,a.peer,text) if s.startswith("animal> ")]
        if len(heard)!=len(answers):raise RuntimeError("incomplete census interview")
        correct+=sum(x==y for x,y in zip(heard,answers));total+=len(answers)
    out.writerow(("acquired","circuit",correct,total))
    for path,count in zip(paths,matches):out.writerow(("fixed","".join(map(str,path)),count,total))

def speed(a,out):
    for n in a.replicas:
        for seed in a.seeds:
            p=subprocess.run([BIN,"infer","--load",a.speaker,"--word","circuit","--batch",str(n),"--decisions",str(a.decisions),"--seed",str(seed)],
                             text=True,capture_output=True,check=True)
            for line in p.stdout.splitlines():
                if line.startswith("endpoint_batch="):
                    fields=dict(kv.split("=",1) for kv in line.split())
                    out.writerow((seed,*(fields[k] for k in ("endpoint_batch","replicas","bytes","verified","queries","preparation_ms","GPU_ms","Mqueries_per_s"))))
            sys.stdout.flush()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("mode",choices=("rear","interview","inquiry","enumerate","speed"))
    p.add_argument("--speaker",type=Path,default=ROOT/"examples/cartographer.mind");p.add_argument("--peer",type=Path,default=ROOT/"examples/lookout.mind")
    p.add_argument("--save-speaker",type=Path);p.add_argument("--save-peer",type=Path)
    p.add_argument("--seeds",type=int,nargs="+",default=SEEDS);p.add_argument("--boards",type=int,default=4);p.add_argument("--trials",type=int,default=8)
    p.add_argument("--clarify",type=int,default=0);p.add_argument("--question-log",type=Path)
    p.add_argument("--spoken",action="store_true",help="present new relations as audible three-word examples")
    p.add_argument("--replicas",type=int,nargs="+",default=(65536,262144,1048576));p.add_argument("--decisions",type=int,default=512)
    a=p.parse_args()
    if a.boards<1 or a.trials<1 or a.clarify<0:p.error("positive board and trial counts and a nonnegative clarification budget required")
    if a.question_log:a.question_log.write_text("learner_seed,word,attempt,asked,reply\n")
    with tempfile.TemporaryDirectory(prefix="axion-meaning-",dir=os.environ.get("TMPDIR","/var/tmp")) as scratch:
        folder=Path(scratch)
        if a.mode=="rear":
            if not a.save_speaker or not a.save_peer or a.save_speaker.resolve()==a.save_peer.resolve():p.error("rear requires distinct output paths")
            rear(a,folder,a.seeds[0],*curriculum(random.Random(a.seeds[0]),False),(a.save_speaker,a.save_peer))
        elif a.mode=="interview":
            out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","condition","question","truth","spoken","correct"))
            interview(a,folder,out)
        elif a.mode=="inquiry":
            out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","trial","choice","asked","informative","late_answer","correct","probes","exact_resume"))
            inquiry(a,folder,out)
        elif a.mode=="enumerate":
            out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("system","route","correct","probes"));enumerate_small(a,out)
        else:
            out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","layout","replicas","bytes","verified","queries","preparation_ms","gpu_ms","mqueries_per_s"));speed(a,out)
if __name__=="__main__":main()
