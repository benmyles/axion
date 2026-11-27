#!/usr/bin/env python3
"""Present a single real physical demonstration and measure the resident's subsequent material choices."""
import argparse
import csv
import os
import re
import subprocess
import tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument("--axion",default="./axion")
p.add_argument("--actor",type=Path,default=Path("examples/joint-reader.mind"));p.add_argument("--output",required=True,type=Path)
p.add_argument("--minds",type=Path);p.add_argument("--worlds",type=int,default=16)
a=p.parse_args()
def talk(body,script,save=None):
    args=[a.axion,"talk","--speaker",str(body),"--listener",str(a.actor)]
    if save:args+=["--save-speaker",str(save)]
    return subprocess.run(args,input="\n".join([*script,"/quit"])+"\n",text=True,stdout=subprocess.PIPE,check=True).stdout
def findings(text):
    return [dict(part.split("=",1) for part in s.split()) for s in text.splitlines() if s.startswith("workbench_event=")]
with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-double-bench-") as t, a.output.open("w",newline="") as output:
    root=Path(t); writer=None
    for i in range(a.worlds):
        seed=319+i*2
        # The external facilitator first finds a productive peer action on a
        # distinct tray. Every trial starts with the original uninformed mind.
        moves=[]
        for m in range(8):
            moves += [f"/workbench {seed} 2",f"/show-work {m}","/peer-sun 72"]
        sampled=findings(talk(a.actor,moves))
        best=max(range(8),key=lambda m:int(sampled[3*m+2]["reactions"]))
        results={}
        for name in ("shuttered","demonstrated"):
            script=[f"/workbench {seed} 4","/handle-scraps 128"]
            if name=="demonstrated":
                for j in range(8):script.extend([f"/workbench {seed} 2",f"/show-work {best}","/peer-sun 48"])
            script += [f"/workbench {seed} 4","/inspect-work","/work-word newt","/sun 72","/inspect-work"]
            save=root/"pupil.mind" if name=="demonstrated" else None
            log=talk(a.actor,script,save)
            rows=findings(log);choice=next((x for x in rows if x["workbench_event"]=="work-word"),{})
            row=dict(seed=seed,condition=name,demonstrator_muscle=best,teacher_reactions=int(sampled[3*best+2]["reactions"]),
                     located=int(bool(choice)),site=choice.get("named_site",""),operation=choice.get("operation",""),
                     working=int(rows[-2]["reactions"])>0,reactions=int(rows[-2]["reactions"]))
            if writer is None:writer=csv.DictWriter(output,fieldnames=row.keys(),lineterminator="\n");writer.writeheader()
            writer.writerow(row);results[name]=row
        output.flush()
        if a.minds and i==0:
            import shutil
            shutil.copyfile(root/"pupil.mind",a.minds)
