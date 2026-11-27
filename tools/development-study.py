#!/usr/bin/env python3
"""Independent physical ports and cross-implementation checks for acquired maps."""
import argparse
import csv
import re
from collections import deque
import os
from pathlib import Path
import random
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BIN = os.environ.get("AXION_BIN", str(ROOT / "axion"))
MASK = (1 << 64) - 1


def waveform(name):
    if name == "_":
        return 0
    if name.startswith("@"):
        return int(name[1:], 16)
    x = sum((ord(c)-96) << (5*i) for i,c in enumerate(name))
    x = ((x ^ (x >> 30)) * 0xbf58476d1ce4e5b9) & MASK
    x = ((x ^ (x >> 27)) * 0x94d049bb133111eb) & MASK
    return x ^ (x >> 31)


def hideout(reverse):
    moves = ["_", "left", "right", "outside", "inside", "where", "it", "look", "fox", "is", "will", "wait"]
    def transition(state, gesture):
        side, memory, present, speech = state
        out = "_"
        if gesture in ("left", "right"):
            side = gesture
            if present:
                memory = side
        elif gesture == "outside":
            present = False
        elif gesture == "inside":
            present, memory = True, side
        elif gesture == "where":
            speech = "question"
        elif gesture in ("it", "look") and speech == "question":
            speech, out = ("place", "hidden") if gesture == "it" else ("witness", "expects")
        elif gesture == "_" and speech in ("place", "witness"):
            out, speech = side if speech == "place" else memory, ""
        return (side, memory, present, speech), out
    side = "right" if reverse else "left"
    return (side, side, True, ""), moves, transition


def apparatus(seed):
    rng = random.Random(seed)
    # The machine, its utterances and its alphabet are private to this process.
    # Rings and classroom semantics do not enter the GPU learner through this port.
    moves = ["reed", "foam", "bell"]
    echoes = ["fern", "_", "dawn", "ink"]
    n = 7
    table = [[((q+1)%n if a==0 else rng.randrange(n), rng.choice(echoes)) for a in range(3)] for q in range(n)]
    return 0, moves, lambda state, action: table[state][moves.index(action)]


def circuit(path):
    data = Path(path).read_bytes()
    version, payload = struct.unpack_from("<II",data,8)
    assert version & (1 << 20) and len(data) == payload + 24
    tail = struct.unpack_from("<I",data,-12)[0] if version & (1<<22) else 12440 if version & (1<<21) else 0
    end = len(data)-8-tail
    tissue = data[end-3144:end]
    states, = struct.unpack_from("<I",tissue,2704)
    motors, senses = struct.unpack_from("<II",tissue,3112)
    gestures = struct.unpack_from("<16Q",tissue,16)[:motors]
    retina = [struct.unpack_from("<Q",tissue,152+16*j)[0] for j in range(senses)]
    edges = struct.unpack_from("<1024H",tissue,656)
    return states, gestures, retina, edges


def certify(path, machine):
    size, gestures, retina, edges = circuit(path)
    home, moves, step = machine
    assert set(gestures) == set(map(waveform,moves))
    pending = deque([(home,0,())])
    visited = {(home,0)}
    checked = 0
    while pending:
        world, cell, prefix = pending.popleft()
        for word in moves:
            destination, expected = step(world,word)
            contact = edges[16*cell + gestures.index(waveform(word))]
            if not contact or retina[(contact-1)>>6] != waveform(expected):
                raise AssertionError(f"counterexample {' '.join((*prefix,word))}: expected {expected}")
            successor = (destination,(contact-1)&63)
            checked += 1
            if successor not in visited:
                visited.add(successor)
                pending.append((*successor,(*prefix,word)))
    print(f"certified_cells={size}  reachable_pairs={len(visited)}  exact_contacts={checked}")
    return checked


def teach(args):
    machine = apparatus(args.seed)
    home, moves, step = machine
    cmd = [BIN,"develop","--external","--gestures",",".join(moves),"--origin","inherited","--generations","0",
           "--load",args.mind,"--save",args.save,"--budget","8000","--patience","256","--seed",str(args.seed)]
    count = 0
    with subprocess.Popen(cmd,cwd=ROOT,text=True,stdin=subprocess.PIPE,stdout=subprocess.PIPE,bufsize=1) as child:
        try:
            for line in child.stdout:
                if line.startswith("experiment>"):
                    state, reply = home, []
                    for action in line.split()[1:]:
                        state, echo = step(state,action)
                        reply.append(echo)
                    child.stdin.write(" ".join(reply)+"\n")
                    child.stdin.flush()
                    count += 1
                else:
                    print(line,end="")
        finally:
            child.stdin.close()
        if child.wait():
            raise RuntimeError("GPU learner failed")
    print(f"external_trips={count}")
    certify(args.save,machine)


def interview(args):
    # Check both possible birth locations. The first relocation fixes the
    # physical configuration for all following behavioural trials.
    for rev in (False,True):
        try:
            certify(args.mind,hideout(rev))
            break
        except AssertionError:
            if rev:
                raise
    history = ["inside","left","outside","right", "wait "*args.delay,
               "where is it?", "where will fox look?", "inside","where will fox look?", "galaxies"]
    result = subprocess.run([BIN,"talk","--speaker",args.mind],cwd=ROOT,text=True,input="\n".join(["/home",*history,"/quit",""]),
                            capture_output=True,check=True)
    replies = [s.removeprefix("animal> ") for s in result.stdout.splitlines() if s.startswith("animal> ")]
    assert replies[-5:-1] == ["hidden right","expects left","_","expects right"] and replies[-1] in ("?","(silence)"), replies[-5:]
    print(f"silent_interval={args.delay}  current={replies[-5]!r}  absent={replies[-4]!r}  returned={replies[-2]!r}  unfamiliar={replies[-1]!r}")


def campaign(args):
    out = Path(args.output)
    out.mkdir(parents=True,exist_ok=True)
    base = [BIN,"develop","--load",args.mind,"--origin","inherited","--generations","0"]
    def rows(options, marker, seed, treatment):
        result = subprocess.run([*base,*map(str,options),"--seed",str(seed)],cwd=ROOT,capture_output=True,text=True,check=True)
        found = []
        for line in result.stdout.splitlines():
            if line.startswith(marker):
                data = dict(re.findall(r"([A-Za-z_]+)=([^ ]+)",line))
                found.append(dict(seed=seed,treatment=treatment,**data))
        if not found:
            raise RuntimeError(result.stdout[-1000:])
        return found
    def save(name,values):
        with (out/name).open("w",newline="") as f:
            writer = csv.DictWriter(f,fieldnames=list(values[0]))
            writer.writeheader()
            writer.writerows(values)
    ecology = []
    for seed in (129,5309,8191,23147):
        for arm,breeding in (("intact","select"),("wander","select"),("deaf","select"),("intact","drift")):
            data = rows(["--colony",512,"--seasons",8,"--steps",160,"--days",64,"--forage",arm,"--breeding",breeding],
                        "chorus_season=",seed,arm+"-"+breeding)
            ecology.extend(data)
            save("chorus.csv",ecology)
    evidence = []
    for family,n,m,sounds in (("ring",4,2,2),("network",4,2,2),("network",16,4,4),("ring",32,2,2),("latches",32,6,4)):
        for method in ("probe","replay"):
            for noise in (0,2):
                data = rows(["--worlds",128,"--family",family,"--states",n,"--motors",m,"--sounds",sounds,
                             "--noise",noise,"--method",method,"--patience",128,"--budget",8192,"--span",32],
                            "sequence_family=",31091,f"{family}-{n}-{method}-noise{noise}")
                evidence.extend(data)
                save("discovery.csv",evidence)
    print(f"ecological_seasons={len(ecology)}  discovery_cohorts={len(evidence)}")

if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("study",choices=["external","interview","campaign"])
    p.add_argument("--mind",default="examples/witness-pupil.mind")
    p.add_argument("--seed",type=int,default=31091)
    p.add_argument("--delay",type=int,default=4096)
    p.add_argument("--save",default="external-pupil.mind")
    p.add_argument("--output",default="docs/development")
    a = p.parse_args()
    {"external":teach,"interview":interview,"campaign":campaign}[a.study](a)
