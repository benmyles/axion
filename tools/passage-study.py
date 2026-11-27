#!/usr/bin/env python3
"""Test the same animal on junctions that share a visual landmark."""
import argparse
import csv
import os
from pathlib import Path
import random
import struct
import subprocess
import shutil
import tempfile
import sys

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
SEEDS=(72869,219377,491591,873139)
PHASE_NAMES={"naive":"Before practice","learned":"After practice","surprise":"Unobserved door change",
             "revised":"After revised doors","unseen_food_change":"Unobserved food change","revalued":"After new meals"}


class Passages:
    def __init__(self,seed):
        rng=random.Random(seed)
        labels=rng.sample(range(32),18)
        self.markers=labels[:4];self.goals=labels[4:8];self.junction=labels[8];self.hall=labels[9]
        self.foils=labels[10:];self.visible=[x for x in range(32) if x not in labels][:4];self.wings=rng.sample(range(4),4);self.actions=rng.sample(range(4),4)
        self.shapes=rng.sample(range(4),4);self.nutrients=rng.sample(range(4),4)
        self.free=[k for k in range(32) if k not in labels]
        self.layouts=[(self.wings,self.actions,self.shapes)]+[(rng.sample(range(4),4),rng.sample(range(4),4),rng.sample(range(4),4)) for _ in range(2)]

    def destinations(self,wing,changed,station=0):
        wings,muscles,goals=self.layouts[station]
        return [goals[(muscles[m]+wings[wing]+changed)&3] for m in range(4)]

    def junction_tag(self,wing,station,distinct):
        if distinct:return self.free[4*station+wing]
        return self.junction if not station else self.free[11+station]

    def sensory(self,wing,delay,noise,rng,zero=False,foils=None):
        tag=self.hall if zero else self.markers[wing]
        yield f"stride {1<<tag:#x} 0\n"
        for _ in range(delay):
            tag=rng.choice(self.foils if foils is None else foils) if noise else self.hall
            yield f"stride {1<<tag:#x} 0\n"

    def lesson(self,rng,rounds,delay,noise,changed,distinct,variable=False,places=1):
        for _ in range(rounds):
            for pair in rng.sample(range(4*places),4*places):
                station,wing=divmod(pair,4)
                wait=rng.randrange(delay+1) if variable else delay
                yield from self.sensory(wing,wait,noise,rng)
                room=self.junction_tag(wing,station,distinct)
                exits=sum(self.goals[target]<<(8*m) for m,target in enumerate(self.destinations(wing,changed,station)))
                yield f"travel {1<<room:#x} 0 {exits:#x}\n"

    def probe(self,rng,delay,noise,changed,distinct,motivation,erase,places=1,novel=False):
        for wing in rng.sample(range(4),4):
            foils=(self.free[4*places:] if distinct else self.free[:8]) if novel else None
            yield from self.sensory(wing,delay,noise or novel,rng,erase,foils)
            for station in range(places):
                room=self.junction_tag(wing,station,distinct)
                for target in range(4):
                    grip=self.destinations(wing,changed,station).index(target)
                    goal=(1<<self.goals[target]) if motivation=="landmark" else 0
                    need=(1<<self.nutrients[target]) if motivation=="food" else 0
                    yield f"orient {1<<room:#x} {goal:#x} {grip} {need}\n"


def phases(args,seed):
    w=Passages(seed);rng=random.Random(seed^0x4a7bb9)
    meals=[f"savour {1<<g:#x} {k}\n" for g,k in zip(w.goals,w.nutrients)] if args.destination=="food" else []
    def probe(changed):
        return list(w.probe(rng,args.delay,args.noise,changed,args.distinct,args.destination,args.hide,args.places,args.novel_noise))
    stream=[('meals',meals),('naive',probe(0)),('practice',list(w.lesson(rng,args.rounds,args.practice_delay,args.practice_noise,0,args.distinct,args.variable,args.places))),
            ('learned',probe(0)),('surprise',probe(1)),('change',list(w.lesson(rng,args.rounds,args.practice_delay,args.practice_noise,1,args.distinct,args.variable,args.places))),('revised',probe(1))]
    if args.new_meals:
        w.nutrients=[(n+1)&3 for n in w.nutrients]
        stream.append(('unseen_food_change',probe(1)))
        stream.append(('new_meals',[f"savour {1<<g:#x} {n}\n" for g,n in zip(w.goals,w.nutrients)]))
        stream.append(('revalued',probe(1)))
    return stream


def invoke(*args):
    result=subprocess.run([BIN,*map(str,args)],text=True,stdout=subprocess.PIPE,check=True)
    return [dict(x.split('=',1) for x in line.split() if '=' in x) for line in result.stdout.splitlines()]


def environment(parent):
    base=["--habitat","thicket","--fruit","inherited","--metabolism","food-fuel","--leach",3,"--seep",2,
          "--species",8,"--stand-size",8,"--climates",4,"--season",16,"--synapse-bits",2]
    if parent:
        genes=invoke("agent","--from",parent,"--inspect")
        if any("context_plastic" in x for x in genes):base+=["--relations","contexts","--programs",2 if any("alternate_plastic" in x for x in genes) else 1,
                                                           "--polarity","evolvable" if any(x.get("synaptic_polarity")=="evolvable" for x in genes) else "shared"]
        elif any("propagation_steps" in x for x in genes):base+=["--relations","waves"]
        elif any("motor_fibre" in x for x in genes):base+=["--relations","on"]
        base+=["--founders",parent,"--founder-site",genes[0]["donor_site"]]
    return base


def conditions(args):
    extra=["--junction-types",args.places] if args.places!=1 else []
    if args.motor_clock=="mixed":extra+=["--motor-clock","mixed"]
    return [*extra,"--retention",args.delay,"--practice-retention",args.practice_delay,"--exposures",args.rounds,
            "--passage-noise","mixed" if args.mixed_noise else "novel" if args.novel_noise else "varied" if args.noise else "repeat","--practice-noise","varied" if args.practice_noise else "repeat",
            "--junction-cues","distinct" if args.distinct else "shared","--destination",args.destination,
            "--passage-test",args.test,"--practice-timing","variable" if args.variable else "fixed"]


def breed(args,folder):
    fields="seed generations candidates tested_lifetimes GPU_ms naive learned surprise revised items attempts moves covered".split()
    out=csv.DictWriter(sys.stdout,fields,extrasaction="ignore",lineterminator="\n");out.writeheader()
    best=None;path=folder/"brood.eco";parent=None if args.wild else args.animal
    habitat=environment(parent)
    if args.all_parents and "--founder-site" in habitat:del habitat[habitat.index("--founder-site"):]
    if args.site is not None and "--founder-site" in habitat:habitat[habitat.index("--founder-site")+1]=args.site
    if args.donor:habitat+=["--graft","history",args.donor]
    if args.origin!="inherited":habitat+=["--history-origin",args.origin]
    for seed in args.seeds:
        rows=invoke("life",*habitat,"--side",args.side,"--seed",seed,"--steps",0,"--navigation",args.navigation,"--nursery",args.generations,
                    "--breeding","drift" if args.drift else "select" if args.wild or args.unrestricted else "history-local" if args.islands else "history","--nursery-problem","junctions",
                    *conditions(args),"--junctions",32,"--probe-seed",547901,"--value-controls","off","--save",path)
        training=next(x for x in rows if "nursery" in x);test=next(x for x in rows if x.get("junctions")=="intact")
        out.writerow(training|test|{"seed":seed});sys.stdout.flush()
        rank=(min(int(test[k])for k in ("learned","revised")),sum(int(test[k])for k in ("learned","revised")))
        if args.stocks:
            args.stocks.mkdir(exist_ok=True,parents=True);shutil.copyfile(path,args.stocks/f"{seed}.eco")
        if best is None or rank>best:
            best=rank
            if args.save:shutil.copyfile(path,args.save)


def audit(args):
    fields="seed junctions lives rounds retention practice_retention motor_clock test variable varied practice_varied distinct destination items naive learned surprise revised attempts moves covered".split()
    out=csv.DictWriter(sys.stdout,fields,extrasaction="ignore",lineterminator="\n");out.writeheader()
    for seed in args.seeds:
        rows=invoke("life","--load",args.animal,"--steps",0,"--junctions",args.worlds,"--probe-seed",seed,*conditions(args))
        for row in rows:
            if "junctions" in row:out.writerow(row)
        sys.stdout.flush()


def actor(source,lines,save=None,cut="intact",site=None):
    args=[BIN,"agent","--from" if source.suffix==".eco" else "--load",str(source),"--map-cut",cut]
    if site is not None and source.suffix==".eco":args+=["--site",str(site)]
    if save:args+=["--save",str(save)]
    output=subprocess.run(args,input="".join(lines),text=True,stdout=subprocess.PIPE,check=True).stdout
    return [dict(x.split("=",1) for x in line.split() if "=" in x)for line in output.splitlines()]


def trace_location(data):
    version=struct.unpack_from("<I",data,8)[0]
    auditory={0:0,32:76,64:108}.get(version&96)
    if auditory is None:raise ValueError("unsupported individual checkpoint")
    if data[-16:-12]==(0x53595048).to_bytes(4,"little"):
        extra=int.from_bytes(data[-12:-8],"little");data=data[:-8-extra]+data[-8:]
    if data[-20:-16]==(0x43574f52).to_bytes(4,"little"):
        extra=int.from_bytes(data[-16:-12],"little");data=data[:-8-extra]+data[-8:]
    if data[-16:-12]==(0x434c4159).to_bytes(4,"little"):
        extra=int.from_bytes(data[-12:-8],"little");data=data[:-8-extra]+data[-8:]
    if data[-16:-12]==(0x42524e45).to_bytes(4,"little"):
        extra=int.from_bytes(data[-12:-8],"little");data=data[:-8-extra]+data[-8:]
    if data[-16:-12]==(0x51464c44).to_bytes(4,"little"):
        extent=int.from_bytes(data[-12:-8],"little");data=data[:-8-extent]+data[-8:]
    composition=int.from_bytes(data[-12:-8],"little") if version&4194304 else 12440 if version&2097152 else 0
    back=8+3144*bool(version&1048576)+composition
    procedure=int.from_bytes(data[-back-4:-back],"little") if version&8388608 else 0
    back+=procedure
    ground=int.from_bytes(data[-back-4:-back],"little") if version&16777216 else 0
    return version&31,len(data)-back-ground-24*bool(version&2147483648)-16*bool(version&1073741824)-72*bool(version&536870912)-12*bool(version&268435456)-(176 if version&134217728 else 136)*bool(version&67108864)-(int.from_bytes(data[-back-ground-4:-back-ground],"little") if version&33554432 else 0)-auditory-16*bool(version&128)-56*bool(version&256)-8*bool(version&512)-48*bool(version&1024)-96*bool(version&2048)-8*bool(version&4096)-140*bool(version&8192)-744*bool(version&16384)-1576*bool(version&32768)-1376*bool(version&65536)-32*bool(version&131072)-368*bool(version&262144)-272*bool(version&524288)-184


def contacts(path,world,wing):
    data=path.read_bytes()
    version,start=trace_location(data)
    if not 16<=version<=23:return (0,0,0,0,0,0)
    memory=data[start:start+184];wiring=struct.unpack_from("<I",data,start-164+4)[0]
    shift=(wiring>>9)&31
    cue=(world.markers[wing]+shift)&31
    row=cue
    plastic,_,attention=struct.unpack_from("<III",memory)
    if version>=20 and attention&2048:row^=(((world.junction+shift)&31)*((plastic>>27)|1))&31
    bank=struct.unpack_from("<I",memory,12+4*row)[0]
    tag=memory[140+row];place=tag&63;latch=struct.unpack_from("<I",memory,176)[0]
    good=0
    for m,target in enumerate(world.destinations(wing,0)):
        pins=8|m;code=((pins>>(wiring&7))&1)|(((pins>>((wiring>>3)&7))&1)<<1)
        entry=(bank>>(8*code))&255
        good+=place==32+((world.junction+shift)&31) and (entry>>5)>0 and (entry&31)==((world.goals[target]+shift)&31)
    return (place,bank,int((latch&63)==cue+1),good,tag>>6,int(((latch>>6)&63)==cue+1))


def branches(args,folder):
    out=csv.writer(sys.stdout,lineterminator="\n")
    out.writerow("seed wing intervention delay correct items before_source after_source before_bank after_bank held before_correct_contacts after_correct_contacts before_branch_signal after_branch_signal auxiliary_cue".split())
    for seed in args.seeds:
        w=Passages(seed);parts=dict(phases(args,seed));base=folder/"learned.mind";observed=folder/"returned.mind"
        actor(args.animal,parts["meals"]+parts["practice"],base,site=args.site)
        for wing in range(4):
            pre=contacts(base,w,wing)
            foils=(w.free[4*args.places:] if args.distinct else w.free[:8]) if args.novel_noise else None
            lines=list(w.sensory(wing,args.delay,args.noise or args.novel_noise,random.Random(seed^0xab227d),foils=foils))
            room=w.visible[wing] if args.distinct else w.junction
            for target in range(4):
                grip=w.destinations(wing,0).index(target)
                goal=1<<w.goals[target] if args.destination=="landmark" else 0
                need=1<<w.nutrients[target] if args.destination=="food" else 0
                lines.append(f"orient {1<<room} {goal} {grip} {need}\n")
            arms=("intact","trace-fixed","fixed","raw","no-hold")
            if trace_location(base.read_bytes())[0]>=22:arms+=("fixed-support","no-novelty")
            for cut in arms:
                r=actor(base,lines,observed,cut)
                choices=[int(x['match'])for x in r if 'orient' in x]
                post=contacts(observed,w,wing)
                out.writerow((seed,wing,cut,args.delay,sum(choices),len(choices),pre[0],post[0],hex(pre[1]),hex(post[1]),post[2],pre[3],post[3],pre[4],post[4],post[5]))
            sys.stdout.flush()


def combined(args,folder):
    import runpy
    objects=runpy.run_path(str(ROOT/"tools/context-study.py"))["external_environment"]
    foods=runpy.run_path(str(ROOT/"tools/outcome-study.py"))["teaching"]
    out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(["seed","order","return","task","correct","items","bytes"])
    for i,seed in enumerate(args.seeds):
        old,return_to=objects((721297,158101,367643,809983)[i%4],"interleaved")
        practice,tastes,food_query=foods((731461,230003,413911,854017)[i%4],3)
        objects_and_meals=old+practice+tastes(False)+tastes(True)
        trial=dict(phases(args,seed));spatial=trial["meals"]+trial["practice"]+trial["change"]+trial.get("new_meals",[])
        for order,stream in (("objects_first",objects_and_meals+spatial),("passages_first",spatial+objects_and_meals)):
            base=folder/"integrated.mind";actor(args.animal,stream,base,site=args.site)
            if args.save and not i and order=="objects_first":shutil.copyfile(base,args.save)
            for context in range(4):
                r=actor(base,["# phase Returning object skills\n"]+return_to(context,8,False)+["# phase Hunger-dependent grips\n"]+food_query(True)
                        +["# phase Hidden junctions\n"]+trial.get("revalued",trial["revised"]))
                pairs=[int(x["match"])for x in r if "probe" in x and "match" in x]
                tests=(("single",pairs[:14]),("unseen_pairs",pairs[14:]),("food",[int(x["match"]) for x in r if "seek" in x]),
                       ("junctions",[int(x["match"]) for x in r if "orient" in x]))
                for task,v in tests:out.writerow((seed,order,context,task,sum(v),len(v),base.stat().st_size))
            sys.stdout.flush()


def forage(args):
    fields="seed arm rooms vision span lives survived ticks births offspring_mass harvested oxidized energy visited at_spring away depart_spring depart_away familiar_spring depart_familiar meals found_spring first_arrival_sum items correct far_items far_correct familiar_items familiar_correct".split()
    out=csv.DictWriter(sys.stdout,fields,extrasaction="ignore",lineterminator="\n");out.writeheader()
    for seed in args.seeds:
        log=invoke("life","--load",args.animal,"--steps",0,"--migration",args.worlds,"--probe-seed",seed,"--rooms",args.rooms,
                   "--locomotion",args.locomotion,"--spring-vision",args.vision,"--exposures",args.rounds)
        for x in log:
            if "migration" in x or "spring_probe" in x:
                out.writerow(x|{"seed":seed,"vision":args.vision,"arm":x.get("migration","probe_"+x.get("spring_probe",""))})
        sys.stdout.flush()


def subsistence(args,folder):
    fields="seed generations tested_lifetimes GPU_ms small_mass large_mass small_births large_births small_survived large_survived".split()
    out=csv.DictWriter(sys.stdout,fields,extrasaction="ignore",lineterminator="\n");out.writeheader()
    path=folder/"offspring.eco";parent=None if args.wild else args.animal
    habitat=environment(parent)
    if args.all_parents and "--founder-site" in habitat:del habitat[habitat.index("--founder-site"):]
    if args.site is not None and "--founder-site" in habitat:habitat[habitat.index("--founder-site")+1]=args.site
    if args.donor:habitat+=["--graft","history",args.donor]
    if args.origin!="inherited":habitat+=["--history-origin",args.origin]
    best=None
    for seed in args.seeds:
        rows=invoke("life",*habitat,"--side",args.side,"--seed",seed,"--steps",0,"--navigation",args.navigation,
                    "--nursery",args.generations,"--nursery-problem","migration","--breeding",
                    "drift" if args.drift else "select" if args.wild or args.unrestricted else "history-local" if args.islands else "history",
                    "--rooms",args.rooms,"--spring-vision",args.vision,"--exposures",args.rounds,"--locomotion",args.locomotion,"--save",path)
        row=next(x for x in rows if "nursery"in x)|{"seed":seed}
        for label,size in (("small",max(8,args.rooms//2)),("large",args.rooms)):
            log=invoke("life","--load",path,"--steps",0,"--rooms",size,"--migration",64,"--probe-seed",571603,"--spring-vision",args.vision,
                       "--locomotion",args.locomotion,"--value-controls","off","--exposures",24)
            result=next(x for x in log if x.get("migration")=="intact")
            row[label+"_mass"]=int(result["offspring_mass"]);row[label+"_births"]=result["births"];row[label+"_survived"]=result["survived"]
        out.writerow(row);sys.stdout.flush()
        rank=(min(row["small_mass"],row["large_mass"]),row["small_mass"]+row["large_mass"])
        if args.stocks:
            args.stocks.mkdir(exist_ok=True,parents=True);shutil.copyfile(path,args.stocks/f"{seed}.eco")
        if best is None or rank>best:
            best=rank
            if args.save:shutil.copyfile(path,args.save)


def screen(args):
    fields="seed rank site founder generation genotype screen_changed screen_points worlds points learned changed silent fixed".split()
    out=csv.DictWriter(sys.stdout,fields,extrasaction="ignore",lineterminator="\n");out.writeheader()
    for seed in args.seeds:
        rows=invoke("life","--load",args.animal,"--steps",0,"--junction-pupils",args.candidates,"--probe-seed",seed,*conditions(args))
        for x in rows:
            if x.get("junction_pupils")=="audit":out.writerow(x|{"seed":seed})
        sys.stdout.flush()


def field(args,folder):
    common=environment(args.animal)
    # Natural founding samples the donor population, including its variation.
    if "--founder-site" in common:del common[common.index("--founder-site"):]
    common+=["--navigation",args.navigation,"--basin-width",args.basin_width,"--side",args.side,"--spring-flow",16,
             "--season",128,"--leach",0,"--seep",args.seep,"--flora","autonomous"]
    if args.donor:common.extend(["--graft","history",args.donor])
    fields="seed mode swap tick alive founders max_generation births deaths edits plant_edits moves converted treatment_alive control_alive".split()
    if args.mode=="field":fields+="residents away direction_correct vacant_correct raw_direction immediate_direction no_hold_direction".split()
    out=csv.DictWriter(sys.stdout,fields,extrasaction="ignore",lineterminator="\n");out.writeheader()
    contests=("temporal","temporal-plasticity","temporal-span") if args.mode=="duels" else ("off",)
    for seed in args.seeds:
        for contest in contests:
            for swap in range(2 if args.mode=="duels" else 1):
                path=folder/"community.eco"
                treatment=["--contest",contest]+(["--swap-cohorts"] if swap else []) if args.mode=="duels" else []
                assay=["--appetites",256] if args.mode=="field" else []
                log=invoke("life",*common,*treatment,"--seed",seed,"--mutation","off" if args.mode=="duels" else args.mutation,
                           "--steps",args.horizon,"--report-every",args.interval,"--save",path,*assay)
                resident={x["appetite"]:x for x in log if "appetite" in x}
                census=[x for x in log if "alive" in x]
                for k,x in enumerate(census):
                    row=x|{"seed":seed,"mode":contest,"swap":swap}
                    if resident.get("resident") and k==len(census)-1:
                        actual=resident["resident"]
                        row.update(residents=actual["residents"],away=actual.get("away"),direction_correct=actual.get("closer"),vacant_correct=actual.get("passable"),
                                   raw_direction=resident.get("raw",{}).get("closer"),immediate_direction=resident.get("immediate",{}).get("closer"),
                                   no_hold_direction=resident.get("no_hold",{}).get("closer"))
                    out.writerow(row)
                if args.stocks:
                    args.stocks.mkdir(exist_ok=True,parents=True);shutil.copyfile(path,args.stocks/f"{seed}-{contest}-{swap}.eco")
                if args.save and seed==args.seeds[0] and contest==contests[0] and not swap:shutil.copyfile(path,args.save)
                sys.stdout.flush()


def inference(args,folder):
    world=Passages(SEEDS[0]);brain=folder/"primed.mind";questions=folder/"bearings.lesson"
    actor(args.animal,list(world.sensory(0,64,True,random.Random(719983))),brain)
    items=[(1<<place,1<<goal,0) for place in range(32) for goal in range(32)]
    items.extend((1<<place,0,1<<need) for place in range(32) for need in range(4))
    random.Random(567119).shuffle(items)
    probe=[f"orient {source} {goal} 0 {need}\n" for source,goal,need in items]
    answers=[int(r["effector"],0)for r in actor(brain,probe) if "orient" in r]
    if len(answers)!=len(items):raise RuntimeError("missing frozen serial decision")
    with questions.open("w")as f:
        for (source,goal,need),pins in zip(items,answers):
            if pins&12:f.write(f"{'orient' if pins&8 else 'face'} {source} {goal} {pins&3} {need}\n")
    cues=sum(bool(pins&12) for pins in answers)
    if not cues:raise RuntimeError("animal produced no motor choices in the reference session")
    fields="implementation round repeat cues actors choices correct state_bytes replica_bytes registers local_bytes fingerprint GPU_ms Mchoices/s".split()
    out=csv.DictWriter(sys.stdout,fields,extrasaction="ignore",lineterminator="\n");out.writeheader()
    engines=[("current",BIN)]
    if args.reference:engines.insert(0,("reference",str(args.reference)))
    for count in (1024,16384,65536):
        fingerprint=None
        for iteration in range(args.repetitions):
            for name,engine in engines[::1 if iteration%2==0 else -1]:
                p=subprocess.run([engine,"agent","--load",str(brain),"--lesson",str(questions),"--batch-bench",str(count),"--decisions","64"],
                                 text=True,stdout=subprocess.PIPE,check=True)
                for line in p.stdout.splitlines():
                    result=dict(x.split("=",1) for x in line.split() if "=" in x)
                    if "fingerprint" not in result:continue
                    if result["correct"]!=result["choices"]:raise RuntimeError("parallel orientation changed a serial effector choice")
                    if fingerprint is not None and result["fingerprint"]!=fingerprint:raise RuntimeError("replicated inference changed its fingerprint")
                    fingerprint=result["fingerprint"]
                    out.writerow(result|{"implementation":name,"round":iteration+1,"cues":cues})
                sys.stdout.flush()

def scale(args):
    habitat=environment(args.animal)
    if "--founder-site" in habitat:del habitat[habitat.index("--founder-site"):]
    habitat+=["--navigation",args.navigation,"--basin-width",args.basin_width,"--spring-flow",16,
              "--season",128,"--leach",0,"--seep",args.seep,"--flora","autonomous","--seed",args.seeds[0]]
    fields="repeat side density steps initial_alive alive updates spikes births deaths GPU_bytes GPU_ms Gcell_updates/s neural_regs/thread neural_local_B/thread habitat_regs/thread habitat_local_B/thread fingerprint".split()
    out=csv.DictWriter(sys.stdout,fields,extrasaction="ignore",lineterminator="\n");out.writeheader()
    for side,density,steps in ((64,35,256),(256,35,128),(1024,100,16)):
        fingerprint=None
        for rep in range(args.repetitions):
            rows=invoke("life",*habitat,"--side",side,"--density",density,"--steps",steps)
            geometry=next(r for r in rows if "GPU_bytes" in r)
            initial,end=[r for r in rows if "alive" in r]
            if fingerprint is not None and fingerprint!=end["fingerprint"]:raise RuntimeError("world diverged between same-seed timings")
            fingerprint=end["fingerprint"]
            out.writerow(geometry|end|{"repeat":rep+1,"side":side,"density":density,"steps":steps,"initial_alive":initial["alive"]})
            sys.stdout.flush()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('run','lesson','breed','audit','branches','field','duels','combined','screen','forage','subsistence','inference','scale'))
    p.add_argument('--animal','--population',dest='animal',type=Path,default=ROOT/'examples/wayfarers.eco')
    p.add_argument('--seeds',type=int,nargs='+',default=SEEDS)
    p.add_argument('--rounds',type=int,default=24);p.add_argument('--delay',type=int,default=4)
    p.add_argument('--test',choices=('paired','continuous','mixed','frozen'),default='paired');p.add_argument('--variable',action='store_true')
    p.add_argument('--places',choices=(1,2,3),type=int,default=1);p.add_argument('--all-parents',action='store_true');p.add_argument('--site',type=int)
    p.add_argument('--motor-clock',choices=('lifetime','mixed'),default='lifetime')
    p.add_argument('--navigation',choices=('episodic','attentive','associative','predictive'),default='episodic')
    p.add_argument('--practice-delay',type=int,default=2);p.add_argument('--noise',action='store_true')
    p.add_argument('--practice-noise',action='store_true');p.add_argument('--hide',action='store_true')
    p.add_argument('--novel-noise',action='store_true',help='use test distractors absent during practice')
    p.add_argument('--mixed-noise',action='store_true',help='vary familiar and unfamiliar test distractors across CUDA environments')
    p.add_argument('--new-meals',action='store_true',help='change which distant food satisfies each need after route practice')
    p.add_argument('--map-cut',choices=('intact','raw','trace-fixed','immediate','no-hold','labile','no-novelty','fixed-support'),default='intact')
    p.add_argument('--distinct',action='store_true');p.add_argument('--destination',choices=('landmark','food'),default='landmark')
    p.add_argument('--stage',choices=('all','meals','naive','practice','learned','surprise','change','revised','unseen_food_change','new_meals','revalued'),default='all')
    p.add_argument('--side',type=int,default=16);p.add_argument('--generations',type=int,default=128)
    p.add_argument('--donor',type=Path);p.add_argument('--horizon',type=int,default=2048);p.add_argument('--interval',type=int,default=512)
    p.add_argument('--basin-width',type=int,default=8);p.add_argument('--seep',type=int,default=2);p.add_argument('--mutation',choices=('on','off'),default='on')
    p.add_argument('--islands',action='store_true');p.add_argument('--origin',choices=('inherited','inert','random'),default='inherited')
    p.add_argument('--reference',type=Path);p.add_argument('--repetitions',type=int,default=4)
    p.add_argument('--candidates',type=int,default=256)
    p.add_argument('--rooms',choices=(8,16,32),type=int,default=32);p.add_argument('--vision',choices=('distinct','paired','shared'),default='shared')
    p.add_argument('--locomotion',choices=('hand','foot'),default='foot')
    p.add_argument('--worlds',type=int,default=32);p.add_argument('--wild',action='store_true')
    p.add_argument('--unrestricted',action='store_true');p.add_argument('--drift',action='store_true')
    p.add_argument('--save',type=Path);p.add_argument('--stocks',type=Path)
    args=p.parse_args()
    if args.new_meals and args.destination!='food':p.error('revaluing meals requires a food destination')
    if (args.mixed_noise or args.test=='mixed') and args.mode not in ('breed','audit','screen'):
        p.error('mixed environments apply to CUDA breeding, screening and audits')
    if min(args.delay,args.practice_delay,args.rounds)<0:p.error('delays and rounds must be nonnegative')
    if args.mode=='audit':audit(args);return
    if args.mode=='forage':forage(args);return
    if args.mode=='scale':scale(args);return
    if args.mode=='screen':screen(args);return
    if args.mode in ('breed','branches','field','duels','combined','subsistence','inference'):
        with tempfile.TemporaryDirectory(prefix='axion-passages-',dir=os.environ.get('TMPDIR','/var/tmp')) as work:
            {"breed":breed,"branches":branches,"field":field,"duels":field,"combined":combined,"subsistence":subsistence,"inference":inference}[args.mode](args,Path(work))
        return
    out=csv.writer(sys.stdout,lineterminator='\n')
    if args.mode=='run':out.writerow(['seed','distinct','destination','practice_delay','delay','noise','novel_noise','practice_noise','places','rounds','hidden','phase','correct','items','distinct_actions'])
    for seed in args.seeds:
        parts=[(key,part) for key,part in phases(args,seed) if args.stage in ('all',key)]
        marked=[line for key,part in parts for line in ([f"# phase {PHASE_NAMES.get(key,key)}\n"]+part)]
        if args.mode=='lesson':
            sys.stdout.writelines(marked)
            return
        result=subprocess.run([BIN,'agent','--from' if args.animal.suffix=='.eco' else '--load',str(args.animal),'--map-cut',args.map_cut,*(['--site',str(args.site)] if args.site is not None else []),
                               *(['--save',str(args.save)] if args.save and seed==args.seeds[0] else [])],input=''.join(marked),text=True,stdout=subprocess.PIPE,check=True)
        rows=iter(s for s in result.stdout.splitlines() if s.startswith(('stride=','savour=','orient=','travel=')))
        for name,part in parts:
            choices=[];pins=set()
            for line in part:
                observation=next(rows);values=dict(x.split('=',1) for x in observation.split() if '=' in x)
                if line.startswith('orient'):choices.append(int(values['match']));pins.add(int(values['effector'],0))
            if choices:out.writerow((seed,int(args.distinct),args.destination,args.practice_delay,args.delay,int(args.noise or args.novel_noise),
                                     int(args.novel_noise),int(args.practice_noise),args.places,args.rounds,int(args.hide),name,sum(choices),len(choices),len(pins)))
        if next(rows,None):raise RuntimeError('unexpected additional sensory response')
        sys.stdout.flush()


if __name__=='__main__':main()
