#!/usr/bin/env python3
"""Measure transfer of physical skills across fresh cohorts without copying parents' acquired tissue."""
import argparse
import csv
import subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--axion",default="./axion");p.add_argument("--source",required=True,type=Path)
p.add_argument("--output",required=True,type=Path);p.add_argument("--seeds",nargs="+",type=int,default=[7193,4187,9923,15401])
p.add_argument("--generations",type=int,default=6);p.add_argument("--subjects",type=int,default=1024)
a=p.parse_args()
with a.output.open("w",newline="") as output:
    writer=None
    for seed in a.seeds:
        for visits in (0,128):
            cmd=[a.axion,"floodplain","--load",str(a.source),"--steps","0","--guild-generations",str(a.generations),
                 "--guild-practice",str(visits),"--work-subjects",str(a.subjects),"--work-shown","24","--work-test-seed",str(seed)]
            text=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,check=True).stdout
            for line in text.splitlines():
                if not line.startswith("guild_chain="):continue
                columns=dict(v.split("=",1) for v in line.split())
                row=dict(seed=seed,personal_trials=visits,**columns)
                if writer is None:writer=csv.DictWriter(output,fieldnames=row.keys(),lineterminator="\n");writer.writeheader()
                writer.writerow(row)
            output.flush()
