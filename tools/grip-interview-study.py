#!/usr/bin/env python3
"""Teach fruit names on different light channels from the grip, then test one resident on fresh trays."""
import argparse
import csv
import os
from pathlib import Path
import random
import subprocess
import tempfile

NAMES = ("acorn", "plum", "pear", "fig", "date", "olive", "lime", "yam")
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--axion", default="./axion")
parser.add_argument("--adult", type=Path, required=True)
parser.add_argument("--speaker", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--worlds", type=int, default=24)
parser.add_argument("--origin", choices=["inherited","permissive","random"], default="inherited")
parser.add_argument("--generations", type=int, default=0)
args = parser.parse_args()


def mix(x):
    x &= 0xffffffff
    x = ((x ^ (x >> 16)) * 0x21f0aaad) & 0xffffffff
    x = ((x ^ (x >> 15)) * 0x735a2d97) & 0xffffffff
    return x ^ (x >> 15)


def keyway(seed):
    # Independent physical answer key. The eggs and interviewee never receive it.
    arms = [0,1,2,3]*2
    for i in range(7,0,-1):
        seed = mix(seed+1709)
        j = seed % (i+1)
        arms[i], arms[j] = arms[j], arms[i]
    return arms


def scene(rng, category, fruit):
    distractor = (category+1+rng.randrange(7))%8
    other_fruit = (fruit+1+rng.randrange(7))%8
    positions = rng.sample(range(64),2)
    patches = []
    for name, grip, site in zip((category,distractor),(fruit,other_fruit),positions):
        raw = (rng.randrange(4)<<6) | (name<<3) | grip
        patches.append(f"{raw:02x}@{site}>{site}")
    rng.shuffle(patches)
    return "/tabletop "+" ".join(patches),positions[0]


def execute(mode, argv, utterances=""):
    return subprocess.run([args.axion,mode,*map(str,argv)],input=utterances,text=True,check=True,stdout=subprocess.PIPE).stdout


with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-tray-") as tmp:
    rng = random.Random(99137)
    lesson = Path(tmp)/"names.lesson"
    utterances = []
    for i in range(1024):
        noun = i%8
        utterances += [scene(rng,noun,rng.randrange(8))[0],"/overhear "+NAMES[noun]]
    lesson.write_text("\n".join(utterances)+"\n")
    execute("ground",["--load",args.adult,"--fresh","--origin",args.origin,"--generations",args.generations,
                      "--population",64,"--worlds",8,"--episodes",256,"--lesson",lesson,"--save",args.speaker])

with args.output.open("w",newline="") as out:
    writer = None
    for w in range(args.worlds):
        seed = mix(37171+w)
        rng = random.Random(seed)
        target = keyway(seed)
        angle = 1+w%3
        seen = rng.randrange(8)
        words = [f"/tidalpool {seed}","/ripen 768"]
        expected = []
        for phase in ("before","surprise","after"):
            if phase == "surprise":
                words.append(f"/turn {angle}")
            if phase == "after":
                noun = rng.randrange(8)
                words += [scene(rng,noun,seen)[0],f"/handle {NAMES[noun]} 64"]
            for cue in range(8):
                if cue == seen:
                    continue
                for noun in range(8):
                    retina,place = scene(rng,noun,cue)
                    words += [retina,"/grip-word "+NAMES[noun]]
                    expected.append((phase,place,(target[cue]+(0 if phase=="before" else angle))%4))
        words += ["/grip-word sav", "/quit"]
        stdout = execute("talk",["--speaker",args.speaker],"\n".join(words)+"\n")
        replies = []
        for line in stdout.splitlines():
            if line.startswith("grip=unresolved"):
                if "handling=1" not in line:replies.append({})
            elif line.startswith("tray_grip="):
                q = dict(v.split("=",1) for v in line.split())
                if q["ticks"]=="0":
                    replies.append(q)
        assert len(replies)==len(expected)+1 and not replies[-1],(len(replies),len(expected),stdout[-300:])
        for phase in ("before","surprise","after"):
            cases = [(site,move,r) for (k,site,move),r in zip(expected,replies) if k==phase]
            selected = [(move,r) for site,move,r in cases if r.get("site")==str(site)]
            row=dict(seed=seed,phase=phase,observed_fruit=seen,turn=angle,probes=len(cases),located=len(selected),
                     correct=sum(str(move)==r["tray_grip"] for move,r in selected),unknown_abstained=1)
            if writer is None:
                writer=csv.DictWriter(out,fieldnames=row.keys(),lineterminator="\n")
                writer.writeheader()
            writer.writerow(row)
        out.flush()
