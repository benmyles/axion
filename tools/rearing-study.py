#!/usr/bin/env python3
"""Test whether an individual from the commons can learn motion, reference and object exchange after ecological selection."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))

def train(command):
    subprocess.run(command,stdout=subprocess.DEVNULL,check=True)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--speaker",type=Path,required=True)
    p.add_argument("--listener",type=Path,default=ROOT/"examples/inquirer.mind");p.add_argument("--play",type=int,default=1024)
    p.add_argument("--seed",type=int,default=1511);p.add_argument("--seeds",nargs="+",type=int,default=(216097,396431,703531,1061911))
    p.add_argument("--cycles",type=int,default=8);p.add_argument("--save-speaker",type=Path);p.add_argument("--save-listener",type=Path)
    a=p.parse_args();out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("individual","stage","play","task","correct","items","silent"))
    def interview(speaker,listener,stage):
        result=subprocess.check_output([sys.executable,ROOT/"tools/attention-study.py","transfer","--speaker",speaker,"--listener",listener,
                                        "--cycles",str(a.cycles),"--seeds",*map(str,a.seeds)],text=True)
        totals={}
        for row in csv.DictReader(result.splitlines()):
            t=totals.setdefault(row["task"],[0]*3);t[0]+=int(row["correct"]);t[1]+=1;t[2]+=row["spoken"]=="(silence)"
        for task,v in totals.items():out.writerow((a.speaker.name,stage,a.play,task,*v))
        sys.stdout.flush()
    with tempfile.TemporaryDirectory(prefix="axion-rearing-",dir=os.environ.get("TMPDIR","/var/tmp")) as path:
        folder=Path(path);body=folder/"body.mind";reader=folder/"reader.mind";pitch=folder/"pitch.mind";motion=folder/"motion.mind"
        end=a.save_speaker or folder/"raised.mind";end_reader=a.save_listener or folder/"raised-reader.mind"
        interview(a.speaker,a.listener,"from-commons")
        if a.play:
            train([BIN,"dialogue","--speaker",a.speaker,"--listener",a.listener,"--pairs","1","--seed",str(a.seed),"--facets","2","--vision","gaze",
                   "--language","compass","--references","again,other","--scenes","withheld","--probes","128","--practice","0","--reversal","off",
                   "--hearing","learned","--play",str(a.play),"--save-speaker",body,"--save-listener",reader])
        else:body=a.speaker;reader=a.listener
        train([BIN,"attention","--generations","0","--listener",reader,"--speaker",body,"--save-listener",end_reader,"--save-speaker",pitch,
               "--probe-worlds","8","--cycles","4"])
        train([BIN,"imagine","--load",pitch,"--observations","16","--save",motion,"--probes","4","--worlds","8"])
        train([BIN,"relate","--load",motion,"--generations","0","--teach","2","--save",end,"--cycles","4","--probe-worlds","8"])
        interview(end,end_reader,"after-play")
if __name__=="__main__":main()
