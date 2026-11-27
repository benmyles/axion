#!/usr/bin/env python3
"""Intervention, planning and open-loop controls in independently wired six-lamp rooms."""
import argparse
import csv
import os
import random
import re
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
BINARY=os.environ.get("AXION_BIN",str(ROOT/"axion"))
FIELDS=("room","goals","starts","arm","contacts","held_out","correct_transitions","candidates","horizon","pruned",
        "actions","goals_reached","verified_forecasts","guaranteed","score")


def run(*args,text=""):
    outcome=subprocess.run([BINARY,*map(str,args)],input=text,cwd=ROOT,text=True,capture_output=True,timeout=180)
    if outcome.returncode:
        raise RuntimeError(f"{outcome.args}\n{outcome.stdout[-1000:]}\n{outcome.stderr}")
    return outcome.stdout


def entries(output,field):
    return [dict(re.findall(r"(\w+)=([^\s]*)",line)) for line in output.splitlines() if line.startswith(field+"=")]


def study(args):
    rng=random.Random(args.seed)
    with Path(args.output).open("w",newline="") as dest:
        table=csv.DictWriter(dest,FIELDS,lineterminator="\n");table.writeheader()
        for i in range(args.rooms):
            room=rng.randrange(1<<32)
            a,b=rng.sample(range(6),2)
            predicate=(lambda x:(x>>a)&1) if not i%2 else (lambda x:((x>>a)&(x>>b))&1)
            goal=sum(1<<x for x in range(64) if predicate(x))
            for arm in ("open","closed"):
                result=run("imitate","--load",args.mind,"--lantern",room,"--contacts",args.contacts,"--verify-lantern",room,
                           "--contact-policy",args.contact_policy,"--goal-states",f"{goal:x}","--senses",arm,"--horizon",args.horizon,"--beam",args.beam)
                model=entries(result,"lantern")[0];plan=entries(result,"policy_search")[0]
                actual=entries(result,"physical_audit")
                score=entries(result,"motor_forecast")[0]["utterance"]
                reached=int(actual[0]["goals"]) if actual else 0
                if arm=="closed" and actual and reached!=goal.bit_count():
                    raise AssertionError("a blind sequence changed the uniform measure of a reversible room")
                if actual and (int(plan["satisfied"])!=int(actual[0]["confirmed"]) or int(actual[0]["confirmed"])>reached):
                    raise AssertionError(f"unverified policy in room {room}")
                table.writerow(dict(room=room,goals=f"{goal:016x}",starts=64,arm=arm,contacts=model["physical_contacts"],
                    held_out=model["unseen"],correct_transitions=model["forecast"],candidates=plan["candidates"],horizon=plan["depth"],
                    pruned=plan["clipped"],actions=plan["stages"],goals_reached=reached,verified_forecasts=actual[0]["agreement"] if actual else 0,guaranteed=plan["satisfied"],score=score))
                dest.flush()
            print(f"Room {i+1}/{args.rooms} verified",flush=True)


def selection(args):
    fields=("seed","selection","generations","population","rooms","contacts","correct","held_out","exact_rooms","genes")
    with Path(args.output).open("w",newline="") as dest:
        table=csv.writer(dest,lineterminator="\n");table.writerow(fields)
        for seed in (7807,8951,10333,27019):
            for arm in ("inherited","drift","select","sweep"):
                generations=24 if arm in ("drift","select") else 0
                result=run("imitate","--load",args.mind,"--origin","inherited","--generations",generations,"--population",64,"--seed",seed,
                           "--select-questions",1,"--selection","drift" if arm=="drift" else "select","--survey-rooms",args.rooms,"--contacts",args.contacts,
                           "--contact-policy","sweep" if arm=="sweep" else "active")
                score=entries(result,"motor_survey")[0];genes=re.search(r"procedural_rule=(\S+)",result)[1]
                table.writerow([seed,arm,generations,64,args.rooms,score["contacts"],score["inferred"],score["withheld"],score["perfect"],genes]);dest.flush()
            print(f"Selection and controls verified for seed {seed}",flush=True)


def storms(args):
    fields=("seed","wire","season","period","attained","moves","meals","replays","rewired","adopted","declined")
    with Path(args.output).open("w",newline="") as dest:
        table=csv.writer(dest,lineterminator="\n");table.writerow(fields)
        for seed in (1513,7721,11411,40099):
            for arm in ("open","immediate-replay","fixed-attention","fixed-gait"):
                result=run("imitate","--load",args.mind,"--origin","inherited","--generations",0,"--colony",256,"--days",64,
                           "--seasons",1,"--drills",384,"--storm",24,"--wire",arm,"--seed",seed)
                season=entries(result,"roost_season")[0]
                for r in entries(result,"wind_window"):
                    table.writerow([seed,arm,r["season"],r["wind_window"],r["attained"],r["moves"],season["meals"],season["replays"],season["rewired"],season["adopted"],season["declined"]])
                dest.flush()
            print(f"Completed matched weather treatments for seed {seed}",flush=True)


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    mode=parser.add_mutually_exclusive_group();mode.add_argument("--evolution",action="store_true");mode.add_argument("--storms",action="store_true")
    parser.add_argument("--rooms",type=int,default=24);parser.add_argument("--contacts",type=int,default=32)
    parser.add_argument("--seed",type=int,default=91073);parser.add_argument("--horizon",type=int,default=8)
    parser.add_argument("--beam",type=int,default=512);parser.add_argument("--mind",default="examples/ritual-gardener.mind")
    parser.add_argument("--contact-policy",choices=("active","sweep"),default="sweep")
    parser.add_argument("--output",required=True)
    args=parser.parse_args();(storms if args.storms else selection if args.evolution else study)(args)
