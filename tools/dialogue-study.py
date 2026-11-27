#!/usr/bin/env python3
"""Measure actual vocal exchanges across independent CUDA communities."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--speakers",nargs="+",type=Path,required=True)
    p.add_argument("--listeners",nargs="+",type=Path,required=True)
    p.add_argument("--speaker-site",type=int);p.add_argument("--listener-site",type=int)
    p.add_argument("--seeds",nargs="+",type=int,default=[1511,9631,27983,73243])
    p.add_argument("--facets",type=int,choices=(1,2),default=1)
    p.add_argument("--practice",type=int,default=512);p.add_argument("--pairs",type=int,default=256)
    p.add_argument("--references",nargs=2);p.add_argument("--nouns",nargs=2,default=["food","water"])
    p.add_argument("--language",choices=("private","compass"),default="private");p.add_argument("--scenes",choices=("all","withheld"),default="all")
    p.add_argument("--vision",choices=("whole","gaze"),default="whole");p.add_argument("--community-size",type=int,default=1)
    p.add_argument("--optics",choices=("affine","shuffled"),default="affine")
    p.add_argument("--eligibility",choices=("local","inherited"),default="local")
    p.add_argument("--outcomes",choices=("shared","none","yoked"),default="shared");p.add_argument("--motor-learning",choices=("both","speaker","listener","neither"),default="both")
    p.add_argument("--plasticity",choices=("both","speaker","listener","neither"),default="both")
    p.add_argument("--repetitions",type=int,default=1);p.add_argument("--dispatch",choices=("graph","direct"),default="graph");p.add_argument("--intact-only",action="store_true")
    p.add_argument("--probes",type=int,default=32);p.add_argument("--preserve-doors",action="store_true")
    p.add_argument("--play",type=int,default=0);p.add_argument("--demonstrations",type=int,default=0);p.add_argument("--breeding",choices=("select","drift"),default="select")
    p.add_argument("--generations",type=int,default=0);p.add_argument("--stocks",type=Path)
    args=p.parse_args()
    fields="speaker listener seed repeat dispatch speaker_bytes listener_bytes GPU_world_bytes plasticity outcomes motor_learning practice demonstrations play breeding generations language scenes vision optics eligibility community_size dialogue wire pairs facets correct exchanges voiced acted perfect_pairs relevant_gazes relative relative_correct bits_per_call GPU_ms".split()
    out=csv.DictWriter(sys.stdout,fields,lineterminator="\n",extrasaction="ignore");out.writeheader()
    if args.stocks:args.stocks.mkdir(parents=True,exist_ok=True)
    for speaker in args.speakers:
        for listener in args.listeners:
            for seed in args.seeds:
                run=[os.environ.get("AXION_BIN",str(ROOT/"axion")),"dialogue","--speaker",speaker,"--listener",listener,
                     "--pairs",str(args.pairs),"--facets",str(args.facets),"--practice",str(args.practice),"--probes",str(args.probes),"--seed",str(seed)]
                if args.speaker_site is not None:run+=["--speaker-site",str(args.speaker_site)]
                if args.listener_site is not None:run+=["--listener-site",str(args.listener_site)]
                if args.references:run+=["--references",",".join(args.references)]
                if args.nouns!=["food","water"]:run+=["--nouns",",".join(args.nouns)]
                if args.language!="private":run+=["--language",args.language]
                if args.scenes!="all":run+=["--scenes",args.scenes]
                if args.optics!="affine":run+=["--optics",args.optics]
                if args.vision!="whole":run+=["--vision",args.vision]
                if args.community_size!=1:run+=["--community-size",str(args.community_size)]
                if args.eligibility!="local":run+=["--eligibility",args.eligibility]
                if args.preserve_doors:run+=["--reversal","off"]
                if args.outcomes!="shared":run+=["--outcomes",args.outcomes]
                if args.motor_learning!="both":run+=["--motor-learning",args.motor_learning]
                if args.plasticity!="both":run+=["--plasticity",args.plasticity]
                if args.play:run+=["--play",str(args.play)]
                if args.demonstrations:run+=["--demonstrations",str(args.demonstrations)]
                if args.generations:run+=["--generations",str(args.generations),"--breeding",args.breeding]
                if args.stocks:
                    prefix=f"{speaker.stem}-{listener.stem}-{seed}"
                    run+=["--save-speaker",args.stocks/(prefix+"-speaker.mind"),"--save-listener",args.stocks/(prefix+"-listener.mind")]
                    if args.generations:run+=["--founder-speaker",args.stocks/(prefix+"-s-germ.mind"),"--founder-listener",args.stocks/(prefix+"-l-germ.mind")]
                run+=["--dispatch",args.dispatch]
                if args.intact_only:run+=["--controls","off"]
                for repeat in range(1,args.repetitions+1):
                    data=subprocess.run(run,text=True,stdout=subprocess.PIPE,check=True)
                    geometry={}
                    for line in data.stdout.splitlines():
                        if "GPU_world_bytes=" in line:geometry=dict(x.split("=",1) for x in line.split() if "=" in x)
                        if line.startswith("dialogue="):
                            row=dict(x.split("=",1) for x in line.split() if "=" in x)
                            out.writerow(geometry|row|{"repeat":repeat,"dispatch":args.dispatch,"seed":seed,"speaker":speaker.name,"listener":listener.name,"practice":args.practice,"plasticity":args.plasticity,"outcomes":args.outcomes,"motor_learning":args.motor_learning,"demonstrations":args.demonstrations,"play":args.play,"generations":args.generations,"breeding":args.breeding,"language":args.language,"scenes":args.scenes,"vision":args.vision,"optics":args.optics,"eligibility":args.eligibility,"community_size":args.community_size})
                sys.stdout.flush()
if __name__=="__main__":main()
