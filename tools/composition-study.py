#!/usr/bin/env python3
"""External speakers and adversarial scenes for the CUDA compositional pupil."""
import argparse
import csv
import itertools
import os
from pathlib import Path
import random
import re
import runpy
import subprocess

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))


def run(*args, text=""):
    result=subprocess.run([BIN,*map(str,args)],cwd=ROOT,input=text,capture_output=True,text=True)
    if result.returncode:
        raise RuntimeError(result.stderr+"\n"+result.stdout[-1200:])
    return result.stdout


def interview(mind, script):
    lines="\n".join([*(x for x,_ in script),"/quit",""])
    report=run("talk","--speaker",mind,text=lines)
    answers=[s.removeprefix("animal> ") for s in report.splitlines() if s.startswith("animal> ")]
    if len(answers)!=len(script):
        raise AssertionError((len(script),len(answers),report[-400:]))
    for i,((question,expected),actual) in enumerate(zip(script,answers)):
        if actual!=expected:
            raise AssertionError((i,question,expected,actual,script[max(0,i-12):i]))
    return len(answers)


def endpoint(edges,x):
    seen=set()
    if x not in edges:
        return None
    while x in edges:
        if x in seen: return None
        seen.add(x)
        x=edges[x]
    return x


def exhaustive(mind):
    animals=("kestrel","vole");items=("ember","relic");places=("mesa","lagoon")
    scenes=list(itertools.product((*places,"coffer"),(*places,"coffer"),places))
    script=[];probes=0;disagreements=0
    def say(line,y="_"):script.append((line,y))
    for before,after in itertools.product(scenes,repeat=2):
        say("/new-scene")
        for beast in animals:say(f"{beast} is awake")
        def move(vals):
            for x,y in zip((*items,"coffer"),vals):say(f"put {x} inside {y}")
            return dict(zip((*items,"coffer"),vals))
        prior=move(before)
        say("kestrel is asleep")
        now=move(after)
        for x in items:
            old,new=endpoint(prior,x),endpoint(now,x)
            say(f"where is {x}",f"{x} rests at {new}")
            say("and its container",f"because {x} rides {now[x]}")
            say(f"where will kestrel look for {x}",f"kestrel expects {old}")
            say(f"where will vole look for {x}",f"vole expects {new}")
            disagreements+=old!=new;probes+=4
        say("vole is asleep");say("kestrel is awake")
        final=move(("coffer","coffer","mesa"))
        for x in items:
            say(f"where will kestrel look for {x}",f"kestrel expects {endpoint(final,x)}")
            say(f"where will vole look for {x}",f"vole expects {endpoint(now,x)}")
            probes+=2
    interview(mind,script)
    print(f"ordered_scenes={len(scenes)**2}  exact_answers={probes}  conflicting_views={disagreements}  port_events={len(script)}")


def social(mind):
    script=[("/new-scene","_"),("put compass inside pouch","_"),("put pouch inside loft","_"),
            ("where should ibis search","what did ibis choose?"),("compass","try loft"),
            ("ibis is awake","_"),("ibis is asleep","_"),("put pouch inside quay","_"),
            ("where is compass","compass rests at quay"),("and its container","because compass rides pouch"),
            ("where will ibis look for compass","ibis expects loft"),
            ("put compass inside pouch","_"),("put pouch inside compass","_"),("where is compass","?")]
    interview(mind,script)
    print("missing_fact_question=1  reply_integrated=1  split_perspective=1  cyclic_abstention=1")


def foreign(args):
    rng=random.Random(args.seed)
    # A separately written routing service. It gives raw utterances to the
    # learner. No shelter actions, role annotations or feature IDs are sent.
    places=["chalk","plum","gale","dawn"];nouns=["birch","gum","moth","asp","owl","elm","cat","jay"]
    lesson=[];edges={};cargo={}
    def emit(x,y=""):lesson.append(x+(" | "+y if y else ""))
    def link(a,b):edges[a]=b;emit(f"{b} receives {a}")
    for epoch in range(16):
        a,b,c,e=rng.sample(nouns,4);target=rng.choice(places)
        if epoch%3==0:link(a,target)
        elif epoch%3==1:link(b,target);link(a,b)
        else:link(c,target);link(b,c);link(a,b)
        for left,right,obj in ((a,b,c),(a,e,b),(c,b,e)):
            cargo[left,right]=obj;emit(f"ticket {right} from {left} bears {obj}")
        questions=[(f"trace {a} home",f"{endpoint(edges,a)} guides {a}"),(f"neighbor of {a}",edges[a])]
        for key in ((a,b),(a,e),(c,b)):questions.append((f"redeem {key[1]} at {key[0]}",cargo[key]))
        rng.shuffle(questions)
        for x,y in questions:emit(x,y)
    run("compose","--lesson","-","--load",args.mind,"--origin","inherited","--generations","0",*([] if getattr(args,"append",False) else ["--fresh"]),"--save",args.save,text="\n".join(lesson))
    # Novel forms have never been heard during instruction. Depth exceeds
    # all demonstrations, and ticket keys can swap or repeat their roles.
    keys=["v"+chr(ord("a")+i) for i in range(26)]
    script=[("/new-scene","_")]
    for a,b in zip(keys,keys[1:]): script.append((f"{b} receives {a}","_"))
    for x in reversed(keys[:-1]):
        script.append((f"trace {x} home",f"{keys[-1]} guides {x}"))
        script.append((f"neighbor of {x}",keys[keys.index(x)+1]))
    tickets={}
    pairs=list(itertools.product(keys[:6],repeat=2))
    for a,b in pairs:
        val=rng.choice(keys);tickets[a,b]=val
        script.append((f"ticket {b} from {a} bears {val}","_"))
    rng.shuffle(pairs)
    for a,b in pairs:script.append((f"redeem {b} at {a}",tickets[a,b]))
    interview(args.save,script)
    print(f"foreign_teaching_events={len(lesson)}  novel_chain_depth=25  two_key_bindings={len(pairs)}  external_checks={len(script)}")


def campaign(args):
    out=Path(args.output);out.mkdir(parents=True,exist_ok=True)
    rows=[]
    for seed in (311,5741,22739,67993):
        for wire in ("open","muted","shifted","solitary","no-repair"):
            result=run("compose","--load",args.mind,"--origin","inherited","--generations",0,"--society",256,"--seasons",6,"--days",64,"--wire",wire,"--seed",seed)
            for line in result.splitlines():
                if line.startswith("shelter_season="):rows.append(dict(seed=seed,wire=wire,**dict(re.findall(r"(\w+)=([^ ]+)",line))))
    with (out/"society.csv").open("w",newline="") as f:
        writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    rows=[]
    for dialect,noise in itertools.product(("english","permuted"),(0,1,2,4)):
        result=run("compose","--load",args.mind,"--origin","inherited","--generations",0,"--worlds",128,"--trials",512,"--depth",20,
                   "--dialect",dialect,"--noise",noise,"--seed",31091)
        for line in result.splitlines():
            if line.startswith("composition="):rows.append(dict(dialect=dialect,noise=noise,**dict(re.findall(r"(\w+)=([^ ]+)",line))))
    with (out/"transfer.csv").open("w",newline="") as f:
        writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    print("island_treatments=20  transfer_cohorts=8")


def tuition(args):
    out=Path(args.output);out.mkdir(parents=True,exist_ok=True)
    rows=[]
    for seed in (311,5741,22739,67993):
        costs={}
        for policy in ("passive","disagreement","yoked"):
            options=[] if policy=="passive" else ["--inquiries",8,"--attention",policy]
            report=run("compose","--load",args.mind,"--mentor","examples/shelter-elder.mind","--origin","inherited","--generations",0,
                       "--worlds",128,"--seed",seed,"--trials",512,"--depth",16,"--controls",0,*options)
            fees=next((dict(re.findall(r"(\w+)=([^ ]+)",line)) for line in report.splitlines() if line.startswith("tuition=")),{})
            score=next(dict(re.findall(r"(\w+)=([^ ]+)",line)) for line in report.splitlines() if line.startswith("composition=intact"))
            costs[policy]=int(fees.get("requested",0))
            rows.append(dict(seed=seed,policy=policy,requested=costs[policy],revised=int(fees.get("revised_columns",0)),uncovered=int(fees.get("uncovered",0)),**score))
        if costs["disagreement"]!=costs["yoked"]:raise AssertionError(costs)
    with (out/"tuition.csv").open("w",newline="") as f:
        writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    print("matched_caregiver_cohorts=12")


def selection(args):
    out=Path(args.output);out.mkdir(parents=True,exist_ok=True)
    rows=[]
    for seed in (311,5741,22739,67993):
        for method,generations in (("select",16),("drift",16),("random",0)):
            report=run("compose","--origin","random","--population",64,"--generations",generations,"--selection","drift" if method=="drift" else "select",
                       "--worlds",32,"--seed",seed,"--trials",192,"--depth",12,"--dialect","permuted","--controls",0)
            score=next(dict(re.findall(r"(\w+)=([^ ]+)",line)) for line in report.splitlines() if line.startswith("composition=intact"))
            genes=re.search(r"compositional_rule=([a-f0-9.]+)",report)[1]
            rows.append(dict(seed=seed,method=method,genes=genes,**score))
    with (out/"selection.csv").open("w",newline="") as f:
        writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    print("evolutionary_comparisons=12")


def enact(args):
    ref=runpy.run_path(str(ROOT/"tools/agency-study.py"))
    names=ref["NOUNS"];words=ref["WORDS"];mechanism=ref["greenhouse"]();rng=random.Random(args.seed)
    schedules=[]
    for site in range(16):
        for trait in range(6):
            start=rng.randrange(64)&~(1<<trait);decoy=rng.randrange(64)
            schedules.append((site,trait,start,decoy))
    rows=[]
    for wire in ("on","off"):
        commands=[f"/self-hear {wire}"]
        for site,trait,start,decoy in schedules:
            other=(site+7)%16
            commands.extend(["/greenhouse 16 0","/new-scene",f"/plot {site} {start}",f"/plot {other} {decoy}",
                             "nell chooses coral","put coral inside sledge",f"put sledge inside {names[site]}",
                             f"nell seeks {words[6+trait]}","guide nell.","proceed.",f"/read-plot {site}",f"/read-plot {other}"])
        result=run("talk","--speaker",args.mind,text="\n".join([*commands,"/quit",""]))
        scenes=result.split("garden> 1\n")[1:]
        if len(scenes)!=len(schedules):raise AssertionError((len(scenes),result[-1200:]))
        for (site,trait,start,decoy),scene in zip(schedules,scenes):
            traces=re.findall(r"garden> 1 site=(\d+) state=(\d+) actions=\d+",scene)
            assert len(traces)==4,(scene[-600:],traces)
            end=int(traces[-2][1]);other=int(traces[-1][1])
            audible=[s.removeprefix("animal> ") for s in scene.splitlines() if s.startswith("animal> ")]
            phrase=f"how can {names[site]} become {words[6+trait]}"
            assert phrase in audible and "make it" in audible and other==decoy,(site,trait,scene)
            if wire=="on":
                motor=audible[audible.index(phrase)+1].split()
                sequence=[words.index(x) for x in motor]
                path=ref["search"](mechanism,start,1<<trait,1<<trait)
                if not (end&(1<<trait)) or end!=ref["play"](mechanism,start,sequence) or len(sequence)!=len(path):
                    raise AssertionError((site,trait,start,end,motor,path))
            elif end!=start:raise AssertionError((site,start,end,scene))
            rows.append(dict(wire=wire,site=site,trait=trait,start=start,after=end,decoy_before=decoy,decoy_after=other,goal=int(bool(end&(1<<trait)))))
    out=Path(args.output);out.mkdir(parents=True,exist_ok=True)
    with (out/"self-hearing.csv").open("w",newline="") as f:
        writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    print("held_out_plans=96  shortest_executed=96  decoys_preserved=192  muted_goals=0")


def inverse(args):
    items=("gull","ash","tern","jade","rose","onyx","bay","moth")
    places=("vale","mesa","port","dune")
    rng=random.Random(args.seed)
    procure={item:places[k%4] for k,item in enumerate(items)}
    indirect=0
    with subprocess.Popen([BIN,"talk","--speaker",str(args.mind)],cwd=ROOT,text=True,stdin=subprocess.PIPE,stdout=subprocess.PIPE,bufsize=1) as child:
        def ask(line):
            child.stdin.write(line+"\n");child.stdin.flush()
            for result in child.stdout:
                if result.startswith("animal> "):return result[8:].strip()
            raise RuntimeError("the GPU animal disconnected")
        try:
            assert ask("/new-scene")=="_"
            for item,place in procure.items():assert ask(f"put {item} inside {place}")=="_"
            for i in range(128):
                item=rng.choice(items);before=endpoint(procure,item);new=rng.choice([v for v in places if v!=before])
                plan=ask(f"/want where is {item} | {item} rests at {new}")
                move=re.fullmatch(r"put (\w+) inside (\w+)",plan)
                assert move and move[1]==item,plan
                assert ask(f"where is {item}")==f"{item} rests at {before}"
                proposed=dict(procure);proposed[move[1]]=move[2]
                assert endpoint(proposed,item)==new,(item,move[2],new)
                indirect+=move[2]!=new
                assert ask(plan)=="_";procure=proposed
                for who in items:assert ask(f"where is {who}")==f"{who} rests at {endpoint(procure,who)}"
            assert ask("mira is asleep")=="_"
            old=endpoint(procure,"bay");fresh=next(x for x in places if x!=old)
            assert ask(f"/want where will mira look for bay | mira expects {fresh}")=="?"
            assert ask("where will mira look for bay")==f"mira expects {old}"
        finally:
            child.stdin.write("/quit\n");child.stdin.close();child.stdout.read()
        if child.wait():raise RuntimeError("counterfactual interview failed")
    print(f"isolated_interventions=128  indirect_solutions={indirect}  external_readbacks=1024  preserved_snapshots=1")


if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("test",choices=("exhaustive","social","foreign","campaign","tuition","selection","enact","inverse"))
    p.add_argument("--mind",default="examples/courier.mind");p.add_argument("--save",default="foreign.mind")
    p.add_argument("--append",action="store_true")
    p.add_argument("--seed",type=int,default=9011);p.add_argument("--output",default="docs/composition")
    args=p.parse_args()
    if args.test=="inverse":inverse(args)
    elif args.test=="enact":enact(args)
    elif args.test=="foreign":foreign(args)
    elif args.test in ("campaign","tuition","selection"):{"campaign":campaign,"tuition":tuition,"selection":selection}[args.test](args)
    elif args.test=="social":social(args.mind)
    else:exhaustive(args.mind)
