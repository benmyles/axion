#!/usr/bin/env python3
"""Photograph real connected specimens and measure their input–output curves as their habitat develops."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

p=argparse.ArgumentParser(description=__doc__);p.add_argument("--axion",default="./axion");p.add_argument("--output",type=Path,required=True)
p.add_argument("--source",type=Path,default=Path("examples/mineralbank.flood"));p.add_argument("--interval",type=int,default=256)
p.add_argument("--stages",type=int,default=5);p.add_argument("--samples",type=int,default=64)
a=p.parse_args()
with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-port-survey-") as d,a.output.open("w",newline="") as out:
    world=a.source;writer=None
    for stage in range(a.stages):
        target=Path(d)/f"field-{stage}.flood"
        options=["--load",str(world),"--mineral-flow","coupled","--steps",str(a.interval if stage else 0),"--artifact-samples",str(a.samples),"--save",str(target)]
        result=subprocess.run([a.axion,"floodplain",*options],text=True,check=True,stdout=subprocess.PIPE).stdout.splitlines()
        tick=next(l.split()[0].partition("=")[2] for l in reversed(result) if l.startswith("flood_tick="))
        for line in result:
            if not line.startswith("artifact_signature="):continue
            row=dict(tick=tick,stage=stage,**dict(x.split("=",1) for x in line.split()))
            if writer is None:writer=csv.DictWriter(out,fieldnames=row.keys(),lineterminator="\n");writer.writeheader()
            writer.writerow(row)
        out.flush();world=target
