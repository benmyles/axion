#!/usr/bin/env python3
"""Measure the food, energy and reproductive consequences of costly experiments."""
import argparse
import csv
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
SEEDS=(147889,397223,868117,1298321,1768231,2380283,3213797,3959413)
CONTROLS={
    "intact":(), "harvest":("--policy","harvest"),"investigate":("--policy","investigate"),"random":("--choice","random"),
    "fixed":("--credit","frozen"),"muted":("--soma","muted"),"no-receipts":("--receipts","none"),
    "no-call":("--channel","muted"),"another-island":("--channel","permuted"),"fixed-hearing":("--hearing","fixed"),
    "observed-only":("--joint-readout","observed"),"no-joint":("--joint-readout","muted")
}
def measures(line):return dict(part.split("=",1)for part in line.split() if "=" in part)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("mode",choices=("breed","audit"))
    p.add_argument("--source",type=Path);p.add_argument("--save",type=Path);p.add_argument("--save-newborn",type=Path)
    p.add_argument("--seeds",nargs="+",type=int,default=SEEDS);p.add_argument("--arms",nargs="+",choices=CONTROLS,default=["intact"])
    p.add_argument("--families",type=int,default=128);p.add_argument("--replicates",type=int,default=8);p.add_argument("--days",type=int,default=96)
    p.add_argument("--generations",type=int,default=96);p.add_argument("--rations",type=int,default=6);p.add_argument("--volatility",type=int,default=14);p.add_argument("--price",type=int,default=1)
    p.add_argument("--breeding",choices=("mass","drift"),default="mass");p.add_argument("--dispatch",choices=("direct","graph"),default="graph")
    p.add_argument("--receipts",choices=("none","learned"),default="none");p.add_argument("--exposures",type=int,default=8);p.add_argument("--reserve",type=int,default=12)
    p.add_argument("--channel",choices=("private","open","muted","permuted"),default="private");p.add_argument("--neighbors",choices=("mixed","kin"),default="mixed")
    p.add_argument("--retina",choices=("mixed","focused"),default="mixed");p.add_argument("--optical-seed",type=int,default=1511)
    p.add_argument("--optics",choices=("shared","shuffled"),default="shared")
    p.add_argument("--audit-calls",choices=("on","off"),default="off")
    p.add_argument("--policy",choices=("evolved","harvest","investigate"),default="evolved")
    p.add_argument("--landmarks",choices=("aliased","distinct","coupled"),default="aliased");p.add_argument("--pods",choices=("single","tandem"),default="single");p.add_argument("--calories",type=int,default=3)
    a=p.parse_args();breed=a.mode=="breed"
    if a.source is None:a.source=ROOT/"examples"/("marsh-newborn.mind" if a.landmarks=="coupled" else "naturalist.mind")
    keys="seed arm breeding rations volatility price days families replicates generations stage residents alive deposited meals experiments diagnostic after_miss wrong_traps lost_contests lived_days energy spent switches food_supplied food_left best_mass GPU_ms exposures reserve receipts meal_updates channel neighbors calls heard decoded correctly_decoded hidden_traps hidden_hits retina optical_seed bits_per_emission landmarks pods calories unopened optics matched_calls actual_call delayed_call displaced_call resident_bytes simulation_bytes neurons".split()
    writer=csv.DictWriter(sys.stdout,keys,lineterminator="\n");writer.writeheader();best=None
    with tempfile.TemporaryDirectory(prefix="axion-commons-",dir=os.environ.get("TMPDIR","/var/tmp")) as path:
        folder=Path(path)
        for seed in a.seeds:
            for arm in a.arms:
                cmd=[BIN,"commons","--load",a.source,"--families",str(a.families),"--replicates",str(a.replicates),"--days",str(a.days),
                     "--generations",str(a.generations if breed else 1),"--seed",str(seed),"--rations",str(a.rations),"--price",str(a.price),
                     "--volatility",str(a.volatility),"--breeding",a.breeding,"--dispatch",a.dispatch,"--founders","varied" if breed else "identical",
                     "--exposures",str(a.exposures),"--reserve",str(a.reserve),"--receipts",a.receipts,"--channel",a.channel,"--neighbors",a.neighbors,
                     "--retina",a.retina,"--optical-seed",str(a.optical_seed),"--optics",a.optics,"--landmarks",a.landmarks,"--pods",a.pods,"--calories",str(a.calories),
                     "--audit-calls",a.audit_calls,"--policy",a.policy,*CONTROLS[arm]]
                if breed:cmd+=["--save",folder/"adult.mind","--save-newborn",folder/"newborn.mind"]
                text=subprocess.check_output(cmd,text=True)
                hardware=next(measures(line) for line in text.splitlines() if " simulation_bytes=" in line)
                stages=[measures(line) for line in text.splitlines() if line.startswith("commons_generation=")]
                final=next(measures(line) for line in text.splitlines() if line.startswith("commons="))
                for data in stages:
                    data.update(seed=seed,arm=arm,breeding=a.breeding,rations=a.rations,volatility=a.volatility,price=a.price,days=a.days,
                                families=a.families,replicates=a.replicates,generations=final["generations"],stage=data["commons_generation"],
                                exposures=a.exposures,reserve=a.reserve,receipts="none" if arm=="no-receipts" else a.receipts,
                                neighbors=a.neighbors,channel="muted" if arm=="no-call" else "permuted" if arm=="another-island" else a.channel,
                                retina=a.retina,optical_seed=a.optical_seed,optics=a.optics,landmarks=a.landmarks,pods=a.pods,calories=a.calories)
                    for k in ("resident_bytes","simulation_bytes","neurons"):data[k]=hardware[k]
                    writer.writerow({k:data[k] for k in keys})
                rank=int(final["reproductive_mass"])
                if breed and (best is None or rank>best):
                    best=rank
                    if a.save:shutil.copyfile(folder/"adult.mind",a.save)
                    if a.save_newborn and rank:shutil.copyfile(folder/"newborn.mind",a.save_newborn)
                sys.stdout.flush()
if __name__=="__main__":main()
