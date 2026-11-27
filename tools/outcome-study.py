#!/usr/bin/env python3
"""Experiments on foraging, sensory consequences, and changing bodily needs."""
import argparse
import csv
import filecmp
import itertools
import os
from pathlib import Path
import random
import runpy
import shutil
import statistics
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
EXAMPLES=ROOT/"examples"
BASE=EXAMPLES/"repertoire.eco"
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
COHORTS=(1277,2203,3179,5711,7823,9029)
GARDENS=(601,1423,7919,21911)
AUDIT=(178781,297751,811469,972781)
STREAMS=(731461,230003,413911,854017)
FIELD=("--habitat","thicket","--fruit","inherited","--synapse-bits",2)
CONTEXT=("--relations","contexts","--programs",2,"--polarity","evolvable")
RULES=("arbitrary","frames","paired")
VALUE="naive explored learned surprise changed former items responsive_objects distinct_actions complete_objects trials observed accessible_grips".split()
GROWTH="survived lived_ticks births offspring_mass harvested mineral_uptake converted reserves".split()


def invoke(command,*args,lines=None):
    data=None if lines is None else "".join(lines)
    log=subprocess.check_output([BIN,command,*map(str,args)],input=data,text=True)
    return [dict(x.split("=",1) for x in l.split() if "=" in x) for l in log.splitlines()]


def life(*args):
    return invoke("life",*args)


def agent(*args,lines=()):
    return invoke("agent",*args,lines=lines)


def writer(fields):
    out=csv.DictWriter(sys.stdout,fieldnames=fields,extrasaction="ignore",lineterminator="\n")
    out.writeheader()
    return out


def cortex(population):
    if population is None:return ()
    info=invoke("agent","--from",population,"--inspect")
    if any("context_plastic" in r for r in info):
        dual=any("alternate_plastic" in r for r in info)
        polar=any(r.get("synaptic_polarity")=="evolvable" for r in info)
        return ("--relations","contexts","--programs",2 if dual else 1,"--polarity","evolvable" if polar else "shared")
    if any("propagation_steps" in r for r in info):return ("--relations","waves")
    if any("motor_fibre" in r for r in info):return ("--relations","on")
    return ()


def environment(args,population):
    handling=("--husk",args.husk,"--mouth-origin",args.mouth_origin) if args.husk else ()
    return (*FIELD,*cortex(population),"--metabolism",args.metabolism,"--seep",args.seep,"--leach",args.leach,*handling,
            "--species",args.species,"--climates",4,"--season",args.season,"--stand-size",min(args.side,args.stand_size),
            "--inheritance",args.inheritance,"--modifier-origin",args.modifier_origin)


def valuation(path,seed,task="inferred",objects=16,rounds=16,pairings=1,count=128,controls=True):
    return [r for r in life("--load",path,"--steps",0,"--valuation",count,"--outcome-task",task,
            "--varieties",objects,"--exposures",rounds,"--pairings",pairings,"--probe-seed",seed,
            "--value-controls","on" if controls else "off") if "valuation" in r]


def growth(path,seed,cues="paired",objects=16,rounds=16,count=64,controls=True):
    return [r for r in life("--load",path,"--steps",0,"--growth",count,"--forage-cues",cues,
            "--varieties",objects,"--exposures",rounds,"--probe-seed",seed,
            "--value-controls","on" if controls else "off") if "growth" in r]


def recall(path,seed,task,count=48):
    return next(r for r in life("--load",path,"--steps",0,"--revisit",count,"--regimes",4,
                "--varieties",16,"--exposures",32,"--probe-seed",seed,"--memory-task",task,
                "--memory-controls","off") if r.get("reminders")=="8")


def forensics(folder,args):
    stock=args.stocks or folder
    stock.mkdir(parents=True,exist_ok=True)
    fields="tick alive births age_sum lived_season accessible_grips all_grips opened attempts grazed bitten".split()
    out=writer(["seed","mutation",*fields])
    for seed in GARDENS:
        for mutation in ("on","off"):
            dst=stock/f"field-{seed}-{mutation}.eco"
            for tick,length in ((0,0),(2048,2048),(32768,30720)):
                origin=("--load",dst) if tick else (*FIELD,*CONTEXT,"--species",16,"--climates",4,"--season",16,
                                                    "--founders",BASE,"--seed",seed,"--side",32,"--mutation",mutation)
                r=life(*origin,"--forage-profile","--steps",length,"--save",dst)[-1]
                out.writerow(dict(r,seed=seed,mutation=mutation))


def grafts(folder,args):
    if not args.stocks:raise ValueError("grafts needs the directory written by forensics --stocks")
    out=writer(["seed","base","graft","task","correct","items","acquired","reachable"])
    for seed in GARDENS:
        field=args.stocks/f"field-{seed}-on.eco"
        if life("--load",field,"--steps",0)[-1]["alive"]=="0":continue
        for name,parent,donor in (("release",BASE,field),("field",field,BASE)):
            for parts in ((),("body",),("soma",),("dendrites",),("pathways",),("memory",),("soma","dendrites","pathways","memory")):
                graft=[v for part in parts for v in ("--graft",part,donor)]
                dst=folder/"hybrid.eco"
                life(*FIELD,*CONTEXT,"--species",16,"--side",32,"--seed",seed,"--founders",parent,*graft,"--steps",0,"--save",dst)
                for task in RULES:
                    r=recall(dst,seed^901703,task)
                    out.writerow(dict(r,seed=seed,base=name,graft="+".join(parts) or "none",reachable=r["accessible_grips"]))


def selection(folder,args):
    parent=None if args.wild else args.population
    founding=[]
    if parent:
        site=invoke("agent","--from",parent,"--inspect")[0]["donor_site"]
        founding=["--founders",parent,"--founder-site",site]
    if args.donor:founding.extend(["--graft","outcomes",args.donor])
    course="subsistence" if args.pressure=="growth" else "outcomes"
    breeding="drift" if args.pressure=="drift" else "organ" if parent and not args.unrestricted else "select"
    out=writer(["seed","origin","pressure","breeding","generations","tested_lifetimes","GPU_ms",
                "direct_learned","direct_changed","inferred_learned","inferred_changed","one_learned","one_changed","items","single_offspring","paired_offspring","growth_lifetimes"])
    best=None
    for seed in COHORTS:
        dst=folder/"cohort.eco"
        report=life(*environment(args,parent),*founding,"--side",args.side,"--seed",seed,"--nursery",args.generations,
                    "--nursery-varieties",8,"--nursery-problem",course,"--breeding",breeding,"--exposures",args.rounds,
                    "--practice-schedule",args.practice_schedule,"--pairings",4,"--steps",0,"--save",dst)
        r=dict(next(x for x in report if "nursery" in x),seed=seed,origin="wild" if parent is None else "inherited",
               pressure=args.pressure,breeding=breeding)
        scores=[];biomass=[]
        for task in ("direct","inferred"):
            v=valuation(dst,469891,task,count=48,controls=False)[0]
            r["items"]=v["items"]
            for k in ("learned","changed"):
                r[task+"_"+k]=v[k];scores.append(int(v[k]))
        if args.practice_schedule=="short-long":
            v=valuation(dst,469891,rounds=1,count=48,controls=False)[0]
            for k in ("learned","changed"):
                r["one_"+k]=v[k]
                # A single observation reveals at most half of the balanced choices in expectation.
                scores.append(2*int(v[k]))
        for cue in ("single","paired"):
            v=growth(dst,462871,cue,count=32,controls=False)[0]
            r[cue+"_offspring"]=v["offspring_mass"];r["growth_lifetimes"]=v["lifetimes"];biomass.append(int(v["offspring_mass"]))
        # A growth-selected experiment never uses a cognitive probe to pick its winner.
        measure=biomass if args.pressure=="growth" else scores
        rank=(0,0) if args.pressure=="drift" else (min(measure),sum(measure))
        out.writerow(r);sys.stdout.flush()
        if args.stocks:
            args.stocks.mkdir(parents=True,exist_ok=True);shutil.copyfile(dst,args.stocks/f"{seed}.eco")
        if best is None or rank>best:
            best=rank
            if args.save:shutil.copyfile(dst,args.save)


def audit(args):
    out=writer(["seed","valuation","task","objects","exposures","pairings","lifetimes",*VALUE])
    for seed in AUDIT:
        for task in ("direct","inferred"):
            for objects,rounds,pairs in ((8,8,1),(16,2,1),(16,4,1),(16,8,1),(16,16,1),(16,16,4)):
                out.writerows(valuation(args.population,seed,task,objects,rounds,pairs))


def growth_audit(args):
    out=writer(["seed","growth","cue","lifetimes","span",*GROWTH])
    for seed in AUDIT:
        for cues in ("single","paired"):
            out.writerows(growth(args.population,seed,cues,rounds=24,count=128))


def field(folder,args):
    parent=None if args.wild else args.population
    founding=("--founders",parent) if parent else ()
    fields="tick alive max_generation births deaths edits plant_edits seedings meals tastes_without_food depleted converted grazed bitten age_sum all_grips accessible_grips".split()
    details=[f"{key}_{k}" for key in ("food","seep","pouch","supply","cycle_supply") for k in range(4)]
    out=writer(["seed","animals","plants","metabolism","seep","leach","husk",*fields,*details,"inferred","value_items",
                "residents","goals","available","resident_correct","reset_correct","silent_correct","responsive"])
    for seed in GARDENS:
        for animals,plants in (("on","on"),("off","on"),("on","off"),("off","off")):
            dst=folder/"garden.eco"
            ticks=sorted(t for t in {0,2048,8192,args.horizon} if t<=args.horizon)
            last=0
            for t in ticks:
                origin=("--load",dst) if t else (*environment(args,parent),*founding,"--side",args.side,"--seed",seed,
                                                 "--mutation",animals,"--flora","autonomous" if plants=="on" else "clonal")
                logs=life(*origin,"--steps",t-last,"--save",dst,"--valuation",48,"--appetites",512,"--varieties",16,
                          "--exposures",16,"--pairings",1,"--probe-seed",seed^637309,"--value-controls","off")
                last=t
                census=next(r for r in reversed(logs) if "alive" in r)
                v=next(r for r in logs if "valuation" in r)
                arms={r["appetite"]:r for r in logs if "appetite" in r}
                a=arms.get("resident",{})
                out.writerow(dict(census,seed=seed,animals=animals,plants=plants,metabolism=args.metabolism,seep=args.seep,leach=args.leach,husk=args.husk,
                                  inferred=v.get("changed",0),value_items=v.get("items",0),residents=a.get("residents",0),goals=a.get("goals",0),
                                  available=a.get("available",0),resident_correct=a.get("correct",0),reset_correct=arms.get("reset",{}).get("correct",0),
                                  silent_correct=arms.get("silent",{}).get("correct",0),responsive=a.get("responsive",0)))
                if args.stocks:
                    args.stocks.mkdir(parents=True,exist_ok=True)
                    shutil.copyfile(dst,args.stocks/f"{seed}-{animals}-{plants}-{t}.eco")
            sys.stdout.flush()


def duel(args):
    out=writer(["seed","swap","contest","tick","alive","treatment_alive","control_alive","births","converted","grazed","bitten"])
    for seed in (*GARDENS,28057,34631,47797,69127):
        for contest in ("outcomes","outcome-plasticity","outcome-chain"):
            for swap in (0,1):
                rows=life(*environment(args,args.population),"--founders",args.population,"--side",64,"--seed",seed,"--mutation","off",
                          "--contest",contest,*(["--swap-cohorts"] if swap else []),"--steps",2048,"--report-every",512)
                for r in rows:
                    if "alive" in r:out.writerow(dict(r,seed=seed,swap=swap,contest=contest))


def plant_gardens(args):
    if not args.stocks:raise ValueError("plants needs a directory of field checkpoints")
    fields="tick alive births deaths meals grazed converted".split()
    out=writer(["seed","donor_tick","producer","chemistry",*fields])
    for seed in GARDENS:
        initial=args.stocks/f"{seed}-off-on-0.eco"
        for tick in (2048,8192,32768):
            late=args.stocks/f"{seed}-off-on-{tick}.eco"
            for plant,plant_file in (("initial",initial),("late",late)):
                for food,food_file in (("initial",initial),("late",late)):
                    rows=life(*environment(args,args.population),"--founders",args.population,"--side",args.side,"--seed",seed,
                              "--plant-founders",plant_file,"--food-genes",food_file,"--mutation","off","--flora","clonal",
                              "--steps",8192,"--report-every",2048)
                    for r in rows:
                        if "alive" in r:out.writerow(dict(r,seed=seed,donor_tick=tick,producer=plant,chemistry=food))
            sys.stdout.flush()


def teaching(seed,rounds=16):
    r=random.Random(seed);cues=[1<<k for k in r.sample(range(32),16)]
    def encode(order):return sum(x<<(2*i) for i,x in enumerate(order))
    surfaces=[encode(r.sample(range(4),4)) for _ in cues]
    first=r.sample(range(4),4);second=r.sample(range(4),4)
    while any(a==b for a,b in zip(first,second)):second=r.sample(range(4),4)
    explore=[f"sample {cues[i]:#x} 0 15 {surfaces[i]:#x}\n" for _ in range(rounds) for i in r.sample(range(16),16)]
    def tastes(changed,pairs=1):
        table=second if changed else first
        return [f"taste {f} {table[f]}\n" for _ in range(pairs) for f in range(4)]
    def probe(changed):
        recipe=encode(second if changed else first)
        return [f"seek {cue:#x} 0 {1<<goal} {surface:#x} {recipe:#x}\n" for cue,surface in zip(cues,surfaces) for goal in range(4)]
    return explore,tastes,probe


def external(folder,args):
    out=writer(["seed","phase","correct","items","ticks","bytes"])
    for seed in STREAMS:
        explore,tastes,probe=teaching(seed,args.rounds)
        history=folder/"history.mind";calibrated=folder/"calibrated.mind";changed=folder/"changed.mind"
        agent("--from",args.population,"--save",history,lines=explore)
        agent("--load",history,"--save",calibrated,lines=tastes(False))
        agent("--load",calibrated,"--save",changed,lines=tastes(True))
        for label,path,mapping in (("unvalued",history,False),("calibrated",calibrated,False),("surprise",calibrated,True),
                                   ("revalued",changed,True),("former",changed,False)):
            result=agent("--load",path,lines=probe(mapping))[-1];ok,n=result["probe_matches"].split("/")
            out.writerow(dict(seed=seed,phase=label,correct=ok,items=n,ticks=result["ticks"],bytes=path.stat().st_size))
        if args.save and seed==STREAMS[0]:shutil.copyfile(changed,args.save)


def combined(folder,args):
    context=runpy.run_path(str(ROOT/"tools"/"context-study.py"))["external_environment"]
    out=writer(["food_seed","context_seed","order","regime","family","correct","items","checkpoint_bytes",
                "experienced_grips","untried_queries","untried_correct"])
    for food_seed,context_seed in zip(STREAMS,(721297,158101,367643,809983)):
        meals,branches=context(context_seed,"interleaved")
        explore,tastes,probe=teaching(food_seed,args.rounds)
        food=explore+tastes(False)+tastes(True)
        for order,lessons in (("objects_first",meals+food),("food_first",food+meals)):
            path=folder/"combined.mind"
            experience=agent("--from",args.population,"--save",path,lines=lessons)
            seen={}
            for line,r in zip(explore,(r for r in experience if "sample" in r),strict=True):
                if r["flavour"]!="none":seen.setdefault(line.split()[1],set()).add(int(r["effector"],0)&3)
            held=[]
            for question in probe(True):
                _,cue,_,demand,surface,chemistry=question.split()
                goal=int(demand,0).bit_length()-1
                correct=next(m for m in range(4) if ((int(chemistry,0)>>(2*((int(surface,0)>>(2*m))&3)))&3)==goal)
                held.append(correct not in seen.get(cue,set()))
            if args.save and food_seed==STREAMS[0] and order=="objects_first":shutil.copyfile(path,args.save)
            for regime in range(4):
                result=agent("--load",path,lines=branches(regime,8,False)+probe(True))
                objects=[int(r["match"])for r in result if "probe" in r]
                goals=[int(r["match"])for r in result if "seek" in r]
                for family,values in (("single",objects[:14]),("paired",objects[14:]),("food",goals)):
                    discovery=dict(experienced_grips=sum(map(len,seen.values())),untried_queries=sum(held),
                                   untried_correct=sum(v for v,h in zip(goals,held,strict=True) if h)) if family=="food" else {}
                    out.writerow(dict(food_seed=food_seed,context_seed=context_seed,order=order,regime=regime,
                                      family=family,correct=sum(values),items=len(values),checkpoint_bytes=path.stat().st_size,**discovery))


def inference(folder,args):
    """Score every hidden object permutation consistent with the animal's own observations."""
    out=writer(["seed","practice","calibration","objects","distinct_grips","known_queries","used_known_solution",
                "unknown_queries","used_untried_grip","physical_probes","matches","points","attainable"])
    generator=random.Random(159041)
    seeds=(*STREAMS,*generator.sample(range(1100000,2100000),max(0,args.worlds-len(STREAMS))))[:args.worlds]
    history=folder/"practice.mind"
    surfaces=list(itertools.permutations(range(4)))
    for seed in seeds:
        for practice in args.doses:
            efforts,tastes,probe=teaching(seed,practice)
            observed={}
            report=agent("--from",args.population,"--save",history,lines=efforts)
            for line,result in zip(efforts,(r for r in report if "sample" in r),strict=True):
                cue=int(line.split()[1],0)
                if result["flavour"]!="none":
                    observed.setdefault(cue,{})[int(result["effector"],0)&3]=int(result["flavour"])
                if result["food"]!="none":raise RuntimeError("nutrients appeared during neutral practice")
            specs=[s.split() for s in probe(True)[::4]]
            recipe=int(specs[0][-1],0)
            emitted=[]; trials=[]
            for _,raw,_,_,_,_ in specs:
                seen=observed.get(int(raw,0),{})
                choices=[p for p in surfaces if all(p[a]==f for a,f in seen.items())]
                for goal in range(4):
                    trials.append((seen,goal,len(choices)))
                    for mapping in choices:
                        table=sum(f<<(2*i) for i,f in enumerate(mapping))
                        emitted.append(f"seek {raw} 0 {1<<goal} {table} {recipe}\n")
            for arm in ("paired","mispaired","no_taste"):
                samples=tastes(False)+tastes(True)
                if arm=="mispaired":
                    samples=[f"taste {s.split()[1]} {(int(s.split()[2])+1)&3}\n" for s in samples]
                elif arm=="no_taste":samples=["step 0 0 0\n"]*len(samples)
                replies=iter(r for r in agent("--load",history,lines=itertools.chain(samples,emitted)) if "seek" in r)
                known=used=unknown=explored=matches=ceiling=0
                for seen,goal,n in trials:
                    responses=[next(replies) for _ in range(n)]
                    pins=int(responses[0]["effector"],0)
                    if any(int(r["effector"],0)!=pins for r in responses):raise RuntimeError("an unobserved surface changed the motor output")
                    possibilities={a for a,f in seen.items() if ((recipe>>(2*f))&3)==goal}
                    if possibilities:
                        known+=1;used+=bool(pins&8) and (pins&3) in possibilities;ceiling+=24
                    else:
                        unknown+=1;explored+=bool(pins&8) and (pins&3) not in seen;ceiling+=24//(4-len(seen))
                    matches+=(24//n)*sum(int(r["match"]) for r in responses)
                if next(replies,None) is not None:raise RuntimeError("unexpected frozen probe response")
                out.writerow(dict(seed=seed,practice=practice,calibration=arm,objects=len(specs),
                                  distinct_grips=sum(len(x) for x in observed.values()),known_queries=known,used_known_solution=used,
                                  unknown_queries=unknown,used_untried_grip=explored,physical_probes=len(emitted),matches=matches,
                                  points=24*len(trials),attainable=ceiling))
            sys.stdout.flush()


def fidelity(folder,args):
    source=args.population
    fields=("tick alive births deaths edits tape_edits max_generation opened attempts converted age_sum accessible_grips all_grips").split()
    out=writer(["seed","origin","inheritance","phase",*fields,*("copy_exponent_"+str(i) for i in range(4)),
                "learning_correct","learning_items","recalled","recall_items"])
    for seed in GARDENS:
        for origin in ("inherited","zero","random"):
            for mode in ("standard","cost-only","uniform","modular"):
                path=folder/"inherited.eco"; last=0
                for t in sorted(x for x in {0,2048,8192,32768,args.horizon} if x<=args.horizon):
                    init=("--load",path) if t else (*environment(args,source),"--founders",source,"--seed",seed,"--side",args.side,
                                                   "--inheritance",mode,"--modifier-origin",origin)
                    log=life(*init,"--steps",t-last,"--save",path,"--forage-profile","--valuation",32,"--value-controls","off",
                             "--revisit",32,"--memory-controls","off","--memory-task","arbitrary","--regimes",4,
                             "--varieties",16,"--exposures",32,"--pairings",1,"--probe-seed",seed^207053)
                    last=t
                    census=next(r for r in reversed(log) if "alive" in r)
                    v=next(r for r in log if "valuation" in r)
                    memory=next((r for r in log if r.get("reminders")=="8"),{})
                    out.writerow(dict(census,seed=seed,origin=origin,inheritance=mode,phase="relatives",
                                      learning_correct=v.get("changed",0),learning_items=v.get("items",0),
                                      recalled=memory.get("correct",0),recall_items=memory.get("items",0)))
                    if args.stocks and t==args.horizon:
                        args.stocks.mkdir(parents=True,exist_ok=True)
                        shutil.copyfile(path,args.stocks/f"{seed}-{origin}-{mode}.eco")
                sys.stdout.flush()


def screen_cohorts(args):
    if not args.stocks:raise ValueError("screen-cohorts needs the populations saved by selection --stocks")
    fields=("pupils residents genotypes worlds total possible over_half over_three_quarters contact_dependent rank site founder "
            "generation genotype screen_changed screen_points points learned changed silent fixed").split()
    out=writer(["seed",*fields])
    for seed in COHORTS:
        log=life("--load",args.stocks/f"{seed}.eco","--steps",0,"--pupils",args.samples,"--varieties",16,
                 "--exposures",16,"--pairings",1,"--probe-seed",seed^857337)
        for r in log:
            if "pupils" in r:out.writerow(dict(r,seed=seed))


def passages(folder,args):
    parent=None if args.wild else args.population
    environment_args=environment(args,parent)
    founding=("--founders",parent) if parent else ()
    phases=sorted(n for n in {1,4,16,64,args.passages} if n<=args.passages)
    out=writer(["seed","side","vegetation","span","passages","completed","elapsed","settlers","alive","founders","max_generation",
                "births","edits","plant_edits","grazed","converted","all_grips","learning_correct","items","fixed","silent",
                "eligible","contact_dependent","best","best_fixed","best_points"])
    for seed in GARDENS:
        initial=folder/"initial.eco"
        life(*environment_args,*founding,"--side",args.side,"--seed",seed,"--steps",0,"--save",initial)
        offset=seed^923821
        for vegetation in ("continuous","repeat","novel"):
            path=folder/"passage.eco";shutil.copyfile(initial,path);last=0
            totals=dict.fromkeys(("births","edits","plant_edits","grazed","converted"),0)
            for phase in phases:
                jump=[] if vegetation=="continuous" else ["--colonize",offset+last,"--passages",phase-last,"--settlers",args.settlers,
                                                         "--vegetation",vegetation]
                length=(phase-last)*args.span if vegetation=="continuous" else args.span
                run=life("--load",path,*jump,"--steps",length,"--save",path)
                worlds=[r for r in run if "alive" in r and int(r["tick"])>0]
                final=next(r for r in reversed(run) if "alive" in r)
                completed=phase
                if vegetation=="continuous":
                    totals={k:int(final[k]) for k in totals}
                else:
                    issued=sum(int(r["parent_pool"])>0 for r in run if "passage" in r)
                    completed=last+issued
                    for row in worlds[-issued:] if issued else ():
                        for key in totals:totals[key]+=int(row[key])
                sample=life("--load",path,"--steps",0,"--valuation",32,"--varieties",16,"--exposures",16,"--pairings",1,
                            "--pupils",args.samples,"--probe-seed",seed^380761)
                arms={r["valuation"]:r for r in sample if "valuation" in r}
                v=arms.get("intact",{}); screen=next((r for r in sample if r.get("pupils")=="screen"),{})
                best=next((r for r in sample if r.get("pupils")=="audit"),{})
                out.writerow(dict(final,**dict(totals,seed=seed,side=args.side,vegetation=vegetation,span=args.span,passages=phase,
                                  completed=completed,elapsed=completed*args.span,settlers=args.settlers,learning_correct=v.get("changed",0),
                                  items=v.get("items",0),fixed=arms.get("fixed",{}).get("changed",0),silent=arms.get("silent",{}).get("changed",0),
                                  eligible=screen.get("residents",0),contact_dependent=screen.get("contact_dependent",0),
                                  best=best.get("changed",0),best_fixed=best.get("fixed",0),best_points=best.get("points",0))))
                sys.stdout.flush()
                if args.stocks:
                    args.stocks.mkdir(parents=True,exist_ok=True)
                    shutil.copyfile(path,args.stocks/f"{seed}-{vegetation}-{phase}.eco")
                if final["alive"]=="0":break
                last=phase


def niches(folder,args):
    fields="tick alive births edits plant_edits max_generation opened attempts tastes_without_food all_grips".split()
    out=writer(["stage","seed","species","season","husk","leach",*fields,"residents","goals","correct","reset","silent","reachable",
                "learning_correct","items"])
    parent=args.population
    founding=environment(args,parent)
    def run(stage,seed,season,husk,leach,ticks):
        path=folder/"niche.eco"
        previous=0; scores=[]
        for t in ticks:
            init=("--load",path) if previous else (*founding,"--side",args.side,"--seed",seed,"--founders",parent,
                                                  "--species",args.species,"--season",season,"--husk",husk,"--leach",leach,"--mouth-origin","whole")
            log=life(*init,"--steps",t-previous,"--save",path,"--appetites",args.samples,"--valuation",32,"--varieties",16,
                     "--exposures",16,"--pairings",1,"--value-controls","off","--probe-seed",seed^610403)
            final=next(r for r in reversed(log) if "alive" in r);previous=t
            arms={r["appetite"]:r for r in log if "appetite" in r}
            v=next(r for r in log if "valuation" in r);a=arms.get("resident",{})
            row=dict(final,stage=stage,seed=seed,species=args.species,season=season,husk=husk,leach=leach,
                     residents=a.get("residents",0),goals=a.get("goals",0),correct=a.get("correct",0),reachable=a.get("available",0),
                     reset=arms.get("reset",{}).get("correct",0),silent=arms.get("silent",{}).get("correct",0),
                     learning_correct=v.get("changed",0),items=v.get("items",0))
            out.writerow(row);sys.stdout.flush()
            improvement=(int(row["correct"])-max(int(row["reset"]),int(row["silent"])))/max(1,int(row["goals"]))
            scores.append(improvement)
            if stage=="replication" and args.stocks:
                args.stocks.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(path,args.stocks/f"{seed}-{t}.eco")
            if not int(final["alive"]):break
        return scores[-1]
    candidates=[]
    for season in (16,64,128):
        for husk in (0,2,3):
            for leach in (0,2,3):
                gains=[run("discovery",seed,season,husk,leach,[32768]) for seed in GARDENS]
                # Rank whole habitats, with zero benefit for extinct populations.
                # No diagnostic can determine ancestry or events inside any world.
                candidates.append((sum(g>.05 for g in gains),statistics.median(gains),sum(gains),season,husk,leach))
    _,_,_,season,husk,leach=max(candidates)
    for seed in (2999,6883,10039,15727,26053,37529,41981,61519):
        run("replication",seed,season,husk,leach,[32768,131072])


def islands(folder,args):
    parent=None if args.wild else args.population
    founding=("--founders",parent) if parent else ()
    fields=("pupils residents genotypes worlds total possible over_half over_three_quarters contact_dependent rank site founder "
            "generation genotype screen_changed screen_points points learned changed silent fixed").split()
    out=writer(["seed","side","tick","alive","births","metabolism","seep","leach","origin",*fields])
    for seed in GARDENS:
        dst=folder/"metacommunity.eco";last=0
        for t in sorted(x for x in {0,2048,args.horizon} if x<=args.horizon):
            start=("--load",dst) if t else (*environment(args,parent),*founding,"--side",args.side,"--seed",seed)
            rows=life(*start,"--steps",t-last,"--save",dst,"--pupils",args.samples,"--varieties",16,"--exposures",16,
                      "--pairings",1,"--probe-seed",seed^857337)
            last=t
            census=next(r for r in reversed(rows) if "alive" in r)
            for r in rows:
                if "pupils" in r:
                    out.writerow(dict(r,seed=seed,side=args.side,tick=t,alive=census["alive"],births=census["births"],
                                      metabolism=args.metabolism,seep=args.seep,leach=args.leach,origin="wild" if parent is None else "inherited"))
            if args.stocks:
                args.stocks.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(dst,args.stocks/f"{seed}-{t}.eco")
            sys.stdout.flush()


def scale(folder,args):
    out=writer(["side","density","schedule","repeat","initial","alive","GPU_bytes","neural_regs/thread","neural_local_B/thread",
                "habitat_regs/thread","habitat_local_B/thread","updates","GPU_ms","Gcell_updates/s","fingerprint"])
    for side in (256,1024):
        for density,warm,length in ((35,96,96),(100,4,16)):
            src=folder/"world.eco"
            life(*environment(args,args.population),"--founders",args.population,"--side",side,"--seed",1847,"--density",density,"--steps",warm,"--save",src)
            fingerprint=None
            for repeat in range(3):
                for schedule in (("dense","occupied") if repeat!=1 else ("occupied","dense")):
                    gpu,start,end=life("--load",src,"--schedule",schedule,"--steps",length)
                    if fingerprint and fingerprint!=end["fingerprint"]:raise RuntimeError("schedulers disagree")
                    fingerprint=end["fingerprint"]
                    out.writerow({**gpu,**end,"side":side,"density":density,"schedule":schedule,"repeat":repeat+1,"initial":start["alive"],
                                  "updates":int(end["updates"])-int(start["updates"])})


def compare(folder,args):
    if not args.baseline:raise ValueError("compare needs a prior executable with the same ecological equations")
    out=writer(["side","repeat","build","alive","GPU_ms","Gcell_updates/s","neural_regs/thread","neural_local_B/thread","fingerprint"])
    for side in (256,512):
        warm=folder/"start.eco"
        life(*environment(args,args.population),"--founders",args.population,"--side",side,"--seed",1847,
             "--steps",96,"--save",warm)
        builds=(("prior",args.baseline),("current",BIN))
        for repeat in range(3):
            for label,binary in (builds if repeat!=1 else reversed(builds)):
                dest=folder/(label+".eco")
                command=[str(binary),"life","--load",str(warm),"--steps","192","--schedule","occupied","--save",str(dest)]
                lines=subprocess.check_output(command,text=True).splitlines()
                gpu,_,result=[dict(x.split("=",1) for x in line.split() if "=" in x) for line in lines]
                out.writerow({**gpu,**result,"side":side,"repeat":repeat+1,"build":label})
            if not filecmp.cmp(folder/"prior.eco",folder/"current.eco",shallow=False):
                raise RuntimeError("implementations produced different complete worlds")


def dispersal_scale(folder,args):
    out=writer(["side","repeat","index","passage","source_alive","parent_pool","dispersal_ms","fingerprint"])
    for side in (128,512,1024):
        src=folder/"source.eco"
        life(*environment(args,args.population),"--founders",args.population,"--side",side,"--seed",1847,"--steps",16,"--save",src)
        for repeat in range(3):
            for method in (("warp","grid") if repeat!=1 else ("grid","warp")):
                dest=folder/(method+".eco")
                log=life("--load",src,"--resident-index",method,"--colonize",93157,"--passages",4,"--settlers",256,"--steps",0,"--save",dest)
                fingerprint=log[-1]["fingerprint"]
                for r in log:
                    if "passage" in r:out.writerow(dict(r,side=side,repeat=repeat+1,index=method,fingerprint=fingerprint))
            if not filecmp.cmp(folder/"warp.eco",folder/"grid.eco",shallow=False):raise RuntimeError("resident indices chose different settlers")


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("experiment",choices=("forensics","grafts","selection","audit","growth","field","duel","plants","islands","fidelity","passages","niches","screen-cohorts","lesson","external","combined","inference","scale","compare","dispersal-scale"))
    p.add_argument("--population",type=Path)
    p.add_argument("--wild",action="store_true",help="start independent founders with neutral outcome plasticity")
    p.add_argument("--unrestricted",action="store_true",help="allow all inherited programs to mutate during founding selection")
    p.add_argument("--pressure",choices=("value","growth","drift"),default="value")
    p.add_argument("--practice-schedule",choices=("fixed","short-long"),default="fixed")
    p.add_argument("--metabolism",choices=("balanced","food-fuel"))
    p.add_argument("--seep",type=int,default=0);p.add_argument("--leach",type=int,default=0);p.add_argument("--season",type=int,default=16)
    p.add_argument("--husk",type=int,choices=(0,1,2,3),default=0)
    p.add_argument("--mouth-origin",choices=("inherited","whole","random"),default="inherited")
    p.add_argument("--species",type=int,choices=(4,8,16),default=8)
    p.add_argument("--stand-size",type=int,default=16)
    p.add_argument("--inheritance",choices=("standard","modular","uniform","cost-only"),default="standard")
    p.add_argument("--modifier-origin",choices=("inherited","zero","random"),default="inherited")
    p.add_argument("--span",type=int,default=256);p.add_argument("--passages",type=int,default=128);p.add_argument("--settlers",type=int,default=64)
    p.add_argument("--worlds",type=int,default=16,help="independent public teaching streams in the exhaustive inference experiment")
    p.add_argument("--doses",type=int,nargs="+",default=[0,1,2,3,4],help="self-selected grasps per object in the exhaustive inference experiment")
    p.add_argument("--side",type=int,default=32);p.add_argument("--generations",type=int,default=384)
    p.add_argument("--rounds",type=int);p.add_argument("--horizon",type=int,default=32768);p.add_argument("--samples",type=int,default=4096)
    p.add_argument("--donor",type=Path);p.add_argument("--stocks",type=Path);p.add_argument("--save",type=Path)
    p.add_argument("--baseline",type=Path)
    p.add_argument("--stage",choices=("explore","taste","probe"),default="explore");p.add_argument("--changed",action="store_true")
    p.add_argument("--seed",type=int,default=STREAMS[0]);args=p.parse_args()
    if args.experiment=="inference" and (not 1<=args.worlds<=4096 or any(n<0 or n>64 for n in args.doses)):
        p.error("inference requires 1..4096 worlds and 0..64 grasps per object")
    if args.metabolism is None:args.metabolism="food-fuel" if args.experiment=="niches" else "balanced"
    if args.rounds is None:args.rounds=16 if args.experiment in ("combined","external","lesson") else 12
    if args.population is None:args.population=BASE if args.experiment=="selection" else EXAMPLES/"prospector.eco"
    if args.experiment=="lesson":
        explore,tastes,probe=teaching(args.seed,args.rounds)
        sys.stdout.writelines(explore if args.stage=="explore" else tastes(args.changed) if args.stage=="taste" else probe(args.changed));return
    with tempfile.TemporaryDirectory(prefix="axion-food-",dir=os.environ.get("TMPDIR","/var/tmp"))as temp:
        folder=Path(temp)
        if args.experiment=="forensics":forensics(folder,args)
        elif args.experiment=="grafts":grafts(folder,args)
        elif args.experiment=="selection":selection(folder,args)
        elif args.experiment=="audit":audit(args)
        elif args.experiment=="growth":growth_audit(args)
        elif args.experiment=="field":field(folder,args)
        elif args.experiment=="duel":duel(args)
        elif args.experiment=="plants":plant_gardens(args)
        elif args.experiment=="islands":islands(folder,args)
        elif args.experiment=="fidelity":fidelity(folder,args)
        elif args.experiment=="passages":passages(folder,args)
        elif args.experiment=="niches":niches(folder,args)
        elif args.experiment=="screen-cohorts":screen_cohorts(args)
        elif args.experiment=="external":external(folder,args)
        elif args.experiment=="combined":combined(folder,args)
        elif args.experiment=="inference":inference(folder,args)
        elif args.experiment=="scale":scale(folder,args)
        elif args.experiment=="compare":compare(folder,args)
        elif args.experiment=="dispersal-scale":dispersal_scale(folder,args)


if __name__=="__main__":main()
