#!/usr/bin/env python3
"""Independent sensory streams for multistep choice and transition revaluation."""
import argparse
import csv
import os
from pathlib import Path
import pty
import random
import subprocess
import sys
import tempfile
import termios

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))


class Crossing:
    """Private physical doors in four directed corridors with a shared entrance layer."""
    def __init__(self,seed,depth):
        self.depth=depth
        rng=random.Random(seed)
        self.rooms=rng.sample(range(32),4*(depth+1))
        self.doors=[]
        for i in range(4):
            self.doors.append([4+j for j in rng.sample(range(4),4)])
        for band in range(1,depth):
            downstream=rng.sample(range(4),4)
            for lane in range(4):
                room=4*band+lane
                exits=[room]*4
                exits[rng.randrange(4)]=4*(band+1)+downstream[lane]
                self.doors.append(exits)
        self.original=[x.copy() for x in self.doors]

    def change(self):
        band=self.depth-1
        for room in range(4*band,4*band+4):
            self.doors[room]=[v if v==room else 4*self.depth+((v+1)&3) for v in self.original[room]]

    def solution(self,root,goal):
        for motor in range(4):
            place=self.doors[root][motor]
            while place//4<self.depth:
                place=next(v for v in self.doors[place] if v!=place)
            if place==goal:return motor
        raise AssertionError("a goal became unreachable")

    def raw(self,place):
        return 1<<self.rooms[place]


class Actor:
    def __init__(self,source,save=None):
        self.master,slave=pty.openpty()
        attr=termios.tcgetattr(slave);attr[3]&=~termios.ECHO;termios.tcsetattr(slave,termios.TCSANOW,attr)
        args=["--from" if str(source).endswith(".eco") else "--load",source]
        if save:args+=["--save",save]
        self.process=subprocess.Popen([BIN,"agent",*args],stdin=slave,stdout=subprocess.PIPE,text=True)
        os.close(slave)
        while True:
            line=self.process.stdout.readline()
            if not line:raise RuntimeError("CUDA actor failed before accepting sensory events")
            if "state_bytes=" in line:break

    def event(self,line):
        os.write(self.master,(line+"\n").encode())
        result=self.process.stdout.readline()
        if not result:raise RuntimeError("CUDA actor failed during the sensory stream")
        return dict(x.split("=",1) for x in result.split() if "=" in x)

    def close(self):
        os.write(self.master,b"\x04");self.process.stdout.read()
        if self.process.wait():raise RuntimeError("CUDA actor failed to save its state")
        os.close(self.master)


def public(args):
    out=csv.writer(sys.stdout,lineterminator="\n")
    out.writerow(["seed","depth","phase","correct","probes","covered","encounters"])
    for seed in (78121,159763,491023,851899):
        graph=Crossing(seed,args.depth);rng=random.Random(seed^0x57b6d)
        actor=Actor(args.population);encounters=0
        def practice(sites,rounds):
            nonlocal encounters
            seen=set()
            for _ in range(rounds):
                order=list(sites);rng.shuffle(order)
                for s in order:
                    choice=actor.event(f"step {graph.raw(s)} 0 0")
                    pins=int(choice["effector"],0); dest=graph.doors[s][pins&3] if pins&8 else s
                    if pins&8:seen.add((s,pins&3))
                    actor.event(f"step {graph.raw(dest)} 0 0");encounters+=1
            return len(seen)
        def probes(name,cov):
            score=0
            for s in range(4):
                for g in range(4*args.depth,4*args.depth+4):
                    p=int(actor.event(f"probe {graph.raw(s)} {graph.raw(g)}")["effector"],0)
                    score+=bool(p&8) and p&3==graph.solution(s,g)
            out.writerow((seed,args.depth,name,score,16,cov,encounters));sys.stdout.flush()
        try:
            probes("unseen",0);coverage=practice(range(4*args.depth),args.rounds);probes("learned",coverage)
            graph.change();probes("unobserved_change",coverage)
            distal=practice(range(4*(args.depth-1),4*args.depth),args.rounds);probes("relearned",distal)
        finally:actor.close()


def run(command,*args,lines=()):
    text=subprocess.check_output([BIN,command,*map(str,args)],input="".join(lines),text=True)
    return [dict(field.split("=",1) for field in line.split() if "=" in field) for line in text.splitlines()]


def life(*args):
    return run("life",*args)


def cortex(population):
    if population is None:return ()
    genes=run("agent","--from",population,"--inspect")
    if any("context_plastic" in r for r in genes):
        return ("--relations","contexts","--programs",2 if any("alternate_plastic" in r for r in genes) else 1,
                "--polarity","evolvable" if any(r.get("synaptic_polarity")=="evolvable" for r in genes) else "shared")
    if any("propagation_steps" in r for r in genes):return ("--relations","waves")
    if any("motor_fibre" in r for r in genes):return ("--relations","on")
    return ()


def env(parent):
    return ("--habitat","thicket","--metabolism","food-fuel","--fruit","inherited","--seep",2,"--leach",3,
            "--species",8,"--season",16,"--climates",4,"--stand-size",8,"--synapse-bits",2,*cortex(parent))


def report(path,depth,seed,sets=32,rounds=8,controls=False,destination="landmark",diversion="doors",history="fresh",locomotion="hand"):
    return [r for r in life("--load",path,"--steps",0,"--crossings",sets,"--distance",depth,"--exposures",rounds,
                           "--probe-seed",seed,"--value-controls","on" if controls else "off",
                           "--destination",destination,"--diversion",diversion,*(("--locomotion",locomotion) if locomotion!="hand" else ()),*(("--history",history) if history!="fresh" else ())) if "crossings" in r]


def breed(folder,args):
    import shutil
    parent=None if args.wild else args.population
    setting=(*env(parent),"--season",args.season)
    donor=[]
    if parent:
        site=run("agent","--from",parent,"--inspect")[0]["donor_site"]
        donor=["--founders",parent,"--founder-site",site]
    if args.donor:donor.extend(["--graft","navigation",args.donor])
    headers="seed generations tested_lifetimes GPU_ms d2_learned d2_changed d3_learned d3_changed d4_learned d4_changed food_learned food_doors food_relocated history_before history_after small_mass large_mass items".split()
    out=csv.DictWriter(sys.stdout,headers,extrasaction="ignore",lineterminator="\n");out.writeheader();best=None
    for seed in args.seeds:
        path=folder/"cohort.eco"
        rows=life(*setting,*donor,"--side",args.side,"--navigation",args.navigation,"--seed",seed,"--nursery",args.generations,
                  "--nursery-problem","migration" if args.pressure=="growth" else "navigation","--breeding","drift" if args.drift else "select" if args.wild or args.unrestricted else "map",
                  *(("--history",args.history) if args.history!="fresh" else ()),*(("--locomotion",args.locomotion) if args.locomotion!="hand" else ()),"--rooms",args.rooms,"--distance",args.depth,"--exposures",args.rounds,"--destination",args.destination,"--diversion",args.diversion,"--steps",0,"--save",path)
        row=dict(next(r for r in rows if "nursery" in r),seed=seed); scores=[]
        for d in (2,3,4):
            result=report(path,d,240731,sets=24,rounds=8,locomotion=args.locomotion)[0];row["items"]=result["items"]
            for phase in ("learned","changed"):
                row[f"d{d}_{phase}"]=result[phase];scores.append(int(result[phase]))
        if args.destination!="landmark":
            for event in ("doors","food"):
                r=report(path,4,240731,sets=24,destination="food",diversion=event,locomotion=args.locomotion)[0]
                row["food_learned"]=r["learned"];row["food_doors" if event=="doors" else "food_relocated"]=r["changed"]
                scores.extend([int(r["learned"]),int(r["changed"])])
        if args.history!="fresh":
            for phase in ("before","after"):
                r=report(path,4,240731,sets=24,destination="food",diversion="both",history=phase,locomotion=args.locomotion)[0]
                row["history_"+phase]=r["changed"];scores.append(int(r["changed"]))
        if args.pressure=="growth":
            biomass=[]
            for size in (max(8,args.rooms//2),args.rooms):
                m=next(r for r in life("--load",path,"--steps",0,"--migration",64,"--rooms",size,"--probe-seed",979073,
                                       "--exposures",24,"--value-controls","off","--locomotion",args.locomotion) if r.get("migration")=="intact")
                biomass.append(int(m["offspring_mass"]))
            row["small_mass"],row["large_mass"]=biomass
            scores=biomass
        out.writerow(row);sys.stdout.flush()
        if args.stocks:
            args.stocks.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,args.stocks/f"{seed}.eco")
        rank=(0,0) if args.drift else (min(scores),sum(scores))
        if best is None or rank>best:
            best=rank
            if args.save:shutil.copyfile(path,args.save)


def phased(seed,depth,rounds,destination="landmark",diversion="doors"):
    graph=Crossing(seed,depth); rng=random.Random(seed^0x57b6d)
    kinds=rng.sample(range(4),4); change=rng.randrange(1,4)
    def meals(changed):
        return [f"savour {graph.raw(4*depth+k):#x} {(kinds[k]+changed*change)&3}\n" for k in rng.sample(range(4),4)]
    def lesson(sites):
        lines=[]
        for _ in range(rounds):
            order=list(sites);rng.shuffle(order)
            for s in order:
                table=sum(graph.rooms[v]<<(8*k) for k,v in enumerate(graph.doors[s]))
                lines.append(f"travel {graph.raw(s):#x} 0 {table:#x}\n")
        return lines
    def probe(changed):
        lines=[]
        for s in range(4):
            for k in range(4):
                g=4*depth+k
                if destination=="food":
                    need=1<<((kinds[k]+changed*change)&3)
                    lines.append(f"orient {graph.raw(s):#x} 0 {graph.solution(s,g)} {need}\n")
                else:lines.append(f"orient {graph.raw(s):#x} {graph.raw(g):#x} {graph.solution(s,g)}\n")
        return lines
    practice=lesson(range(4*depth))
    if destination=="food":practice+=meals(False)
    initial=probe(False)
    if diversion!="food":graph.change()
    revised=probe(diversion!="doors")
    correction=[] if diversion=="food" else lesson(range(4*(depth-1),4*depth))
    if diversion!="doors":correction+=meals(True)
    return practice,initial,correction,revised


def external(folder,args):
    out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(["seed","depth","phase","correct","items","bytes","ticks"])
    for seed in (78121,159763,491023,851899):
        for d in (2,4,7):
            pr,old,update,new=phased(seed,d,args.rounds,args.destination,args.diversion)
            paths={k:folder/(k+".mind") for k in ("naive","learned","changed")}
            run("agent","--from",args.population,"--save",paths["naive"])
            run("agent","--load",paths["naive"],"--save",paths["learned"],lines=pr)
            run("agent","--load",paths["learned"],"--save",paths["changed"],lines=update)
            for label,hist,goals in (("unseen","naive",old),("learned","learned",old),("surprise","learned",new),
                                     ("changed","changed",new),("former","changed",old)):
                r=run("agent","--load",paths[hist],lines=goals)[-1];ok,n=r["probe_matches"].split("/")
                out.writerow((seed,d,label,ok,n,paths[hist].stat().st_size,r["ticks"]))
            if args.save and seed==78121 and d==7:
                import shutil
                shutil.copyfile(paths["changed"],args.save)
            sys.stdout.flush()


def audit(args):
    out=None
    for seed in (349861,710243,924823,1137229):
        for d,r in ((2,2),(2,4),(2,8),(3,4),(3,8),(4,4),(4,8),(5,8),(6,8),(7,4),(7,8)):
            rows=report(args.population,d,seed,sets=64,rounds=r,controls=True,destination=args.destination,diversion=args.diversion,locomotion=args.locomotion)
            if out is None:
                out=csv.DictWriter(sys.stdout,["seed",*rows[0]],lineterminator="\n");out.writeheader()
            out.writerows(dict(x,seed=seed) for x in rows);sys.stdout.flush()


def mazes(args):
    out=None
    for seed in (349861,710243,924823,1137229):
        for rooms,dose in ((8,4),(16,4),(16,12),(32,4),(32,12),(32,24)):
            for mode in ("local","roam"):
                rows=[r for r in life("--load",args.population,"--steps",0,"--mazes",32,"--rooms",rooms,
                      "--destination",args.destination,"--diversion",args.diversion,"--exploration",mode,"--exposures",dose,
                      "--probe-seed",seed) if "maze" in r]
                if out is None:
                    out=csv.DictWriter(sys.stdout,["seed",*rows[0]],lineterminator="\n");out.writeheader()
                out.writerows(dict(r,seed=seed) for r in rows);sys.stdout.flush()


def combined(folder,args):
    import runpy
    import shutil
    objects=runpy.run_path(str(ROOT/"tools/context-study.py"))["external_environment"]
    flavours=runpy.run_path(str(ROOT/"tools/outcome-study.py"))["teaching"]
    out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(["seed","order","return","task","correct","items","bytes"])
    for seed,context_seed,taste_seed in zip((78121,159763,491023,851899),(721297,158101,367643,809983),(731461,230003,413911,854017)):
        prior,branches=objects(context_seed,"interleaved")
        practice,tastes,hunger=flavours(taste_seed,3)
        old=prior+practice+tastes(False)+tastes(True)
        travel,_,change,rotated=phased(seed,7,args.rounds,"food","both")
        for order,lessons in (("early_map",travel+change+old),("late_map",old+travel+change)):
            path=folder/"combined.mind";run("agent","--from",args.population,"--save",path,lines=lessons)
            if args.save and order=="late_map" and seed==78121:shutil.copyfile(path,args.save)
            for regime in range(4):
                report=run("agent","--load",path,lines=branches(regime,8,False)+rotated+hunger(True))
                pairs=[int(x["match"]) for x in report if "probe" in x]
                for task,actions in (("single",pairs[:14]),("paired",pairs[14:]),("food",[int(x["match"])for x in report if "seek" in x]),
                                     ("route",[int(x["match"])for x in report if "orient" in x])):
                    out.writerow((seed,order,regime,task,sum(actions),len(actions),path.stat().st_size))
        sys.stdout.flush()


def habitat(folder,args):
    import shutil
    settings=(*env(args.population),"--navigation",args.navigation,"--spring-flow",args.spring_flow,
              "--seep",args.seep,"--leach",0,"--season",args.season,"--side",args.side,"--founders",args.population)
    fields="tick alive founders max_generation births deaths edits plant_edits moves grazed bitten converted".split()
    out=csv.DictWriter(sys.stdout,["seed","animals","flora","spring_flow","seep",*fields,"revaluation","items","residents","away",
                        "closer","open","passable","reset_closer","quiet_closer","one_pulse_closer","wrong_closer","water"],lineterminator="\n",extrasaction="ignore")
    out.writeheader()
    for seed in (601,1423,7919,21911):
        for animals,flora in (("on","autonomous"),("off","autonomous"),("on","clonal"),("off","clonal")):
            path=folder/"field.eco";last=0
            for tick in sorted(x for x in {0,256,2048,8192,args.horizon} if x<=args.horizon):
                source=("--load",path) if tick else (*settings,"--seed",seed,"--mutation",animals,"--flora",flora)
                rows=life(*source,"--steps",tick-last,"--save",path,"--crossings",16,"--distance",5,"--exposures",8,
                          "--destination","food","--probe-seed",seed^781997,"--value-controls","off","--appetites",256)
                last=tick
                w=next(r for r in reversed(rows) if "alive" in r)
                t=next(r for r in rows if "crossings" in r)
                arms={r["appetite"]:r for r in rows if "appetite" in r};a=arms.get("resident",{})
                out.writerow(dict(w,seed=seed,animals=animals,flora=flora,spring_flow=args.spring_flow,seep=args.seep,
                                  revaluation=t.get("changed",0),items=t.get("items",0),residents=a.get("residents",0),
                                  away=a.get("away",0),closer=a.get("closer",0),open=a.get("open",0),passable=a.get("passable",0),
                                  reset_closer=arms.get("map_reset",{}).get("closer",0),quiet_closer=arms.get("map_silent",{}).get("closer",0),
                                  one_pulse_closer=arms.get("one_pulse",{}).get("closer",0),wrong_closer=arms.get("wrong_need",{}).get("closer",0),
                                  water=sum(int(w.get(f"seep_{i}",0)) for i in range(4))))
                if args.stocks:
                    args.stocks.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,args.stocks/f"{seed}-{animals}-{flora}-{tick}.eco")
                if w["alive"]=="0":break
            sys.stdout.flush()


def duels(args):
    out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(["seed","swap","contest","tick","alive","treatment","control","births","moves","oxidized"])
    settings=(*env(args.population),"--navigation",args.navigation,"--spring-flow",args.spring_flow,"--seep",args.seep,
              "--leach",0,"--season",args.season,"--side",args.side,"--founders",args.population)
    for seed in (601,1423,7919,21911,28057,34631,47797,69127):
        for contest in ("navigation","navigation-plasticity","navigation-range"):
            for swap in (False,True):
                rows=life(*settings,"--seed",seed,"--mutation","off","--contest",contest,*(("--swap-cohorts",) if swap else ()),
                          "--steps",2048,"--report-every",512)
                for r in rows:
                    if "alive" in r:out.writerow((seed,int(swap),contest,r["tick"],r["alive"],r["treatment_alive"],r["control_alive"],r["births"],r["moves"],r["converted"]))
        sys.stdout.flush()


def interference(folder,args):
    import runpy
    objects=runpy.run_path(str(ROOT/"tools/context-study.py"))["external_environment"]
    flavours=runpy.run_path(str(ROOT/"tools/outcome-study.py"))["teaching"]
    out=csv.writer(sys.stdout,lineterminator="\n")
    out.writerow(["seed","history","cut","map_identical","before_reminders","after_reminders","items","distinct_grips","productive_rooms","observed_changes"])
    for seed,cseed,fseed in zip((78121,159763,491023,851899),(721297,158101,367643,809983),(731461,230003,413911,854017)):
        experience,returning=objects(cseed,"interleaved")
        grasp,tastes,_=flavours(fseed,3);background=experience+grasp+tastes(False)+tastes(True)
        practice,_,change,questions=phased(seed,7,args.rounds,"food","both")
        physical=[s.split() for s in practice+change if s.startswith("travel")]
        reference=None
        for name,before,after in (("isolated",[],[]),("early",[],background),("late",background,[])):
            path=folder/"interference.mind"
            report=run("agent","--from",args.population,"--save",path,lines=before+practice+change+after)
            seen=set();moved=set();updated=set()
            for i,(src,r) in enumerate(zip(physical,(x for x in report if "travel" in x),strict=True)):
                cue=int(src[1],0);pins=int(r["effector"],0)
                if pins&8:seen.add((cue,pins&3))
                if cue!=1<<int(r["arrival"]):(moved if i<28*args.rounds else updated).add(cue)
            # The 164-byte map occupies the tail immediately before the file checksum.
            circuit=path.read_bytes()[-172:-8]
            if reference is None:reference=circuit
            for cut in ("intact","no-soma","no-dendrites","isolated","disinhibited"):
                scores=[]
                for prefix in ([],returning(0,8,False)):
                    log=run("agent","--load",path,"--map-cut",cut,lines=prefix+questions)
                    scores.append(sum(int(x["match"])for x in log if "orient" in x))
                out.writerow((seed,name,cut,int(circuit==reference),*scores,16,len(seen),len(moved),len(updated)))
            sys.stdout.flush()


def migration(args):
    fields="seed arm rooms span lives survived ticks births offspring_mass harvested oxidized energy visited items correct far_items far_correct familiar_items familiar_correct".split()
    out=csv.DictWriter(sys.stdout,fields,lineterminator="\n",extrasaction="ignore");out.writeheader()
    for seed in (349861,710243,924823,1137229):
        for rooms in (8,16,32):
            for rounds in (8,24):
                r=life("--load",args.population,"--steps",0,"--migration",128,"--rooms",rooms,"--exposures",rounds,
                       "--probe-seed",seed,"--locomotion",args.locomotion)
                for x in r:
                    if "migration" in x or "spring_probe" in x:
                        arm=x.get("migration","probe_"+x.get("spring_probe",""))
                        out.writerow(dict(x,seed=seed,arm=arm,span=rooms*rounds))
            sys.stdout.flush()


def replay(args):
    graph=Crossing(args.seed,args.depth)
    if args.diversion!="food":graph.change()
    queries=phased(args.seed,args.depth,args.rounds,args.destination,args.diversion)[-1]
    trials=[(s,k) for s in range(4) for k in range(4)] if args.all_routes else [(args.start,args.target)]
    successes=shortest=0
    for start,target in trials:
        goal=4*args.depth+target
        lesson=queries[start*4+target].split()
        desire=int(lesson[2],0);need=int(lesson[4],0) if len(lesson)>4 else 0
        actor=Actor(args.population);place=start;visits=[graph.rooms[start]];efforts=[]
        try:
            for t in range(2*args.depth):
                if place>=4*args.depth:break
                packed=sum(graph.rooms[dest]<<(8*m) for m,dest in enumerate(graph.doors[place]))
                command="pace" if args.locomotion=="foot" else "travel"
                result=actor.event(f"{command} {graph.raw(place)} {desire} {packed} {need}")
                arrival=int(result["arrival"]);efforts.append(int(result["effector"],0));visits.append(arrival)
                place=graph.rooms.index(arrival)
        finally:actor.close()
        correct=place==goal;successes+=correct;shortest+=correct and len(efforts)==args.depth
        print(f"start={visits[0]}  target={graph.rooms[goal]}  need={need}  reached={int(correct)}  moves={len(efforts)}"
              f"  landmarks={','.join(map(str,visits))}  muscles={','.join(hex(p) for p in efforts)}")
    print(f"arrivals={successes}/{len(trials)}  shortest={shortest}/{len(trials)}")


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("experiment",choices=("baseline","breed","audit","mazes","external","combined","habitat","duels","interference","migration","replay","lesson"))
    p.add_argument("--depth",type=int,choices=range(2,8),default=5)
    p.add_argument("--rounds",type=int,default=12)
    p.add_argument("--population",type=Path,default=ROOT/"examples/scouts.eco")
    p.add_argument("--side",type=int,default=16);p.add_argument("--generations",type=int,default=256)
    p.add_argument("--navigation",choices=("on","exploratory","competitive"),default="on")
    p.add_argument("--destination",choices=("landmark","food","mixed"),default="landmark")
    p.add_argument("--diversion",choices=("doors","food","both"),default="doors")
    p.add_argument("--history",choices=("fresh","before","after","mixed"),default="fresh")
    p.add_argument("--locomotion",choices=("hand","foot"),default="hand")
    p.add_argument("--pressure",choices=("choices","growth"),default="choices")
    p.add_argument("--unrestricted",action="store_true")
    p.add_argument("--rooms",type=int,choices=(8,16,32),default=16)
    p.add_argument("--season",type=int,default=16)
    p.add_argument("--spring-flow",type=int,default=16);p.add_argument("--seep",type=int,default=0)
    p.add_argument("--horizon",type=int,default=32768)
    p.add_argument("--wild",action="store_true");p.add_argument("--drift",action="store_true")
    p.add_argument("--seeds",type=int,nargs="+",default=[1277,2203,3179,5711,7823,9029])
    p.add_argument("--stocks",type=Path);p.add_argument("--save",type=Path)
    p.add_argument("--donor",type=Path,help="graft only inherited transition-circuit genes into the founding animals")
    p.add_argument("--stage",choices=("practice","probe","change","revised"),default="practice");p.add_argument("--seed",type=int,default=78121)
    p.add_argument("--start",type=int,choices=range(4),default=0);p.add_argument("--target",type=int,choices=range(4),default=0)
    p.add_argument("--all-routes",action="store_true")
    args=p.parse_args()
    if args.experiment in ("lesson","external") and (args.destination=="mixed" or (args.diversion!="doors" and args.destination!="food")):
        p.error("choose a single destination, using a food destination for a food diversion")
    if args.experiment=="baseline":public(args);return
    if args.experiment=="replay":replay(args);return
    if args.experiment=="lesson":
        stages=phased(args.seed,args.depth,args.rounds,args.destination,args.diversion)
        sys.stdout.writelines(stages[("practice","probe","change","revised").index(args.stage)]);return
    if args.experiment=="audit":audit(args);return
    if args.experiment=="mazes":mazes(args);return
    if args.experiment=="duels":duels(args);return
    if args.experiment=="migration":migration(args);return
    with tempfile.TemporaryDirectory(prefix="axion-travel-",dir=os.environ.get("TMPDIR","/var/tmp")) as tmp:
        if args.experiment=="breed":breed(Path(tmp),args)
        elif args.experiment=="combined":combined(Path(tmp),args)
        elif args.experiment=="habitat":habitat(Path(tmp),args)
        elif args.experiment=="interference":interference(Path(tmp),args)
        else:external(Path(tmp),args)


if __name__=="__main__":main()
