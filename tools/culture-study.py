#!/usr/bin/env python3
"""Measure message transmission, common-scene dialects and reproductive outcomes in the CUDA commons."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
SEEDS=(273281,419731,661069,971903,1317019,1646509,1966997,2320931)

def fields(line):
    return dict(s.split("=",1) for s in line.split() if "=" in s)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source",type=Path,default=ROOT/"examples/storyteller.mind")
    p.add_argument("--teacher",type=Path);p.add_argument("--other-teacher",type=Path)
    p.add_argument("--culture",choices=("adult","invent","none"),default="adult")
    p.add_argument("--teachers",choices=("local","random","fixed"),default="local")
    p.add_argument("--seeds",nargs="+",type=int,default=SEEDS)
    p.add_argument("--arms",nargs="+",choices=("open","muted","reversed","displaced","unreared","direct","fixed-hearing"),default=["open"])
    for key,default in (("families",128),("replicates",8),("days",64),("generations",1),("rations",8),
                        ("calories",8),("tutoring",96),("reserve",24),("report-cost",0),("price",1),("volatility",0)):
        p.add_argument("--"+key,type=int,default=default)
    p.add_argument("--policy",choices=("harvest","evolved","investigate"),default="harvest")
    p.add_argument("--founders",choices=("identical","varied"),default="identical")
    p.add_argument("--breeding",choices=("mass","drift"),default="mass")
    p.add_argument("--age",choices=("recent","earlier","mixed"),default="recent")
    a=p.parse_args();out=None
    for seed in a.seeds:
        for arm in a.arms:
            auditory=arm if arm in ("muted","reversed","displaced") else "open"
            tuition=0 if arm=="unreared" else a.tutoring
            cmd=[BIN,"commons","--channel","reports","--load",a.source,"--culture",a.culture,"--teachers",a.teachers,
                 "--seed",seed,"--families",a.families,"--replicates",a.replicates,"--days",a.days,"--generations",a.generations,
                 "--rations",a.rations,"--calories",a.calories,"--reserve",a.reserve,"--volatility",a.volatility,"--event-age",a.age,
                 "--policy",a.policy,"--breeding",a.breeding,"--founders",a.founders,"--tutoring",tuition,"--report-cost",a.report_cost,
                 "--report-wire",auditory,"--report-cache","off" if arm=="direct" else "on","--price",a.price,
                 "--hearing","fixed" if arm=="fixed-hearing" else "plastic"]
            for f,v in (("--teacher",a.teacher),("--other-teacher",a.other_teacher)):
                if v:cmd += [f,v]
            result=subprocess.run(list(map(str,cmd)),check=True,text=True,capture_output=True).stdout.splitlines()
            garden={r["commons_generation"]:r for line in result if line.startswith("commons_generation=") for r in [fields(line)]}
            setup=next(fields(s) for s in result if "resident_bytes=" in s)
            for line in result:
                if not line.startswith("culture_generation="):continue
                c=fields(line);g=c["culture_generation"];s=garden[g]
                words=[x.partition("  speech=")[2] for x in result if x.startswith("island_report=") and f"  generation={g}  " in x]
                row=dict(seed=seed,arm=arm,culture=a.culture,teachers=a.teachers,age=a.age,generation=g,generations=a.generations,
                         tuition_limit=tuition,word_cost=a.report_cost,breeding=a.breeding,policy=a.policy,founders=a.founders,
                         days=a.days,calories=a.calories,rations=a.rations,reserve=a.reserve,price=a.price,volatility=a.volatility)
                row.update((k,c[k]) for k in ("tuition","improvised","words","learned_visits","sampled_forms","sampled_islands","distinct_reports"))
                row.update((k,s[k]) for k in ("residents","alive","deposited","meals","experiments","wrong_traps","lost_contests","lived_days","energy","spent",
                                              "food_supplied","food_left","calls","heard","decoded","correctly_decoded","hidden_traps","hidden_hits","GPU_ms"))
                row.update((k,setup[k]) for k in ("resident_bytes","simulation_bytes","neurons"));row["sampled_speech"]=" | ".join(words)
                if out is None:out=csv.DictWriter(sys.stdout,fieldnames=row,lineterminator="\n");out.writeheader()
                out.writerow(row)
            sys.stdout.flush()
if __name__=="__main__":main()
