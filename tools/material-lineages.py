#!/usr/bin/env python3
"""Compare successive wild construction intervals with matched watersheds in which the hands are still."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--axion",default="./axion")
parser.add_argument("--output",type=Path,required=True)
parser.add_argument("--source",type=Path,default=Path("examples/mineralbank.flood"))
parser.add_argument("--stages",type=int,default=6)
parser.add_argument("--days",type=int,default=128)
parser.add_argument("--exposure",type=int,default=128)
a=parser.parse_args()
with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-heritage-") as stage,a.output.open("w",newline="") as out:
    current=a.source;writer=None
    for i in range(a.stages):
        end=Path(stage)/f"stage-{i}.flood"
        for condition in ("continued","still"):
            cmd=[a.axion,"floodplain","--load",str(current),"--steps",str(a.days),"--material-legacy",str(a.exposure)]
            cmd+=(["--save",str(end)] if condition=="continued" else ["--hands","still"])
            proc=subprocess.run(cmd,stdout=subprocess.PIPE,text=True,check=True)
            def records(tag):
                return [dict(v.split("=",1) for v in l.split()) for l in proc.stdout.splitlines() if l.startswith(tag+"=")]
            events=records("material_tick");mols=records("assemblies")[0];flux={r["material_legacy"]:r for r in records("material_legacy")}
            row=dict(stage=i+1,handling=condition,tick=events[-1]["material_tick"],families=mols["assemblies"],largest=mols["largest"],
                     bonds=mols["bonds"],loops=mols["independent_loops"],reactions=flux["intact"]["reactions"],
                     insulated=flux["insulated"]["reactions"],rearranged=flux["rearranged"]["reactions"],
                     new_exchanges=int(events[-1]["exchanges"])-int(events[0]["exchanges"]))
            if writer is None:writer=csv.DictWriter(out,fieldnames=row.keys(),lineterminator="\n");writer.writeheader()
            writer.writerow(row)
        out.flush();current=end
