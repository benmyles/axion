#!/usr/bin/env python3
"""Test learned causal roles with fresh interventions, bearings and spoken action sequences."""
import argparse
import csv
from itertools import permutations
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
DIRECTIONS=("north","east","south","west")
FRESH=(275207,443969,797917,1363129,1891061,2393387,3274319,4003121)

def report(line):return dict(item.split("=",1) for item in line.split() if "=" in item)

def population(a,folder):
    keys="seed probe_seed breeding generations effects arm probes uncertain first second revised_first revised_second diagnostic plastic competition exploration individuals perfect total available".split()
    writer=csv.DictWriter(sys.stdout,keys,lineterminator="\n");writer.writeheader();best=None
    for seed in a.seeds:
        breed=a.mode=="breed";probe=a.probe_seed if breed else seed
        cmd=[BIN,"relate","--load",a.speaker,"--generations",str(a.generations if breed else 0),"--candidates",str(a.candidates),
             "--worlds",str(a.worlds),"--cycles",str(a.cycles),"--probe-worlds",str(a.probe_worlds),"--seed",str(seed),
             "--probe-seed",str(probe),"--origin",a.origin if breed else "inherited","--breeding",a.breeding,"--effects",a.effects]
        saved=folder/"offspring.mind"
        if breed:cmd+=["--save",saved]
        transcript=subprocess.check_output(cmd,text=True).splitlines()
        g=next(report(s) for s in transcript if "binding_bytes=" in s)
        cohort=next((report(s) for s in transcript if s.startswith("binding_population=")),{})
        for line in transcript:
            if not line.startswith("binding="):continue
            r=report(line);r.update(seed=seed,probe_seed=probe,breeding=a.breeding,arm=r["binding"],**g)
            writer.writerow({k:r.get(k,cohort.get(k,"")) for k in keys})
            scores=[int(r[k]) for k in ("uncertain","first","second","revised_first","revised_second")];rank=min(scores),sum(scores)
            if breed and a.save and r["arm"]=="intact" and (best is None or rank>best):
                shutil.copyfile(saved,a.save);best=rank
        sys.stdout.flush()

class Interview:
    def __init__(self,speaker,args):
        self.p=subprocess.Popen([BIN,"talk","--speaker",speaker,"--listener",args.listener,
                                 "--source-learning",args.credit,"--exploration",args.choice],
                                stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True,bufsize=1)
    def send(self,text):self.p.stdin.write(text+"\n");self.p.stdin.flush()
    def read(self,prefix):
        for line in self.p.stdout:
            if line.startswith(prefix):return line[len(prefix):].strip()
            if line.startswith(("Use /","This animal needs","There is no outstanding","Experiment selection")):raise ValueError(line)
        raise RuntimeError("interview ended before "+prefix)
    def question(self,text):self.send(text);return self.read("animal> ")
    def scene(self,values):self.send("/scene "+" ".join(DIRECTIONS[n] for n in values))
    def teach(self,name,before,after):
        self.send("/show-pair "+name+" "+" ".join(DIRECTIONS[n] for n in before+after));self.read("Observed both")
    def choose(self,name,offers):
        self.send("/try-pair "+name+" "+" ".join(DIRECTIONS[n] for pair in offers for n in pair))
        index,left,right=self.read("experiment> ").split();i=int(index)-1
        if tuple(DIRECTIONS[k] for k in offers[i])!=(left,right):raise ValueError("choice differs from the displayed arrangement")
        return i
    def arrive(self,after):
        self.send("/arrived "+" ".join(DIRECTIONS[n] for n in after));self.read("Observed the result")
    def close(self):
        self.send("/quit");self.p.stdin.close();self.p.stdout.read()
        if self.p.wait():raise RuntimeError("failed public interview")

def interviews(a,folder):
    blank=folder/"pupil.mind"
    subprocess.run([BIN,"relate","--load",a.speaker,"--generations","0","--newborn","--teach","0","--save",blank,
                    "--probe-worlds","1","--cycles","1"],stdout=subprocess.DEVNULL,check=True)
    out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","effects","choice","credit","condition","correct","items","silence"))
    names=("mavo","zelun","kire","pann");nouns=("food","water")
    for seed in a.seeds:
        rng=random.Random(seed);body=Interview(blank,a);totals={};laws=[]
        def count(key,correct,quiet=False):
            row=totals.setdefault(key,[0,0,0]);row[0]+=bool(correct);row[1]+=1;row[2]+=quiet
        def query(text,truth,kind):
            heard=body.question(text);quiet=heard=="(silence)"
            count(kind,heard==(DIRECTIONS[truth] if truth<4 else "(silence)"),quiet)
        try:
            for word in names:
                order=rng.sample(range(4),4);sources=[rng.randrange(2),rng.randrange(2)]
                tables=[]
                geometry=rng.choice(("aliased","distinct","independent")) if a.effects=="mixed" else a.effects
                for o in range(2):
                    labels=rng.sample(range(4),4);rank=[order.index(v) for v in range(4)]
                    if geometry!="distinct":rank=[(v&1 if geometry=="independent" and o else v>>1) for v in rank]
                    tables.append([labels[v] for v in rank])
                def outcome(pair):return tuple(tables[o][pair[sources[o]]] for o in range(2))
                different=[(x,y) for x in range(4) for y in range(4) if all(t[x]!=t[y] for t in tables)]
                similar=[(x,y) for x in range(4) for y in range(4) if (x,y) not in different and (x!=y or geometry=="distinct")]
                for phase in ("first","reversed"):
                    if phase=="reversed":sources=[1-i for i in sources]
                    for b in rng.sample(range(4),4)+rng.sample(range(4),4):body.teach(word,(b,b),outcome((b,b)))
                    if phase=="first":
                        body.scene(rng.choice(different))
                        for noun in nouns:query(f"Where is the {noun} after {word}?",4,"confounded_abstention")
                    offers=[rng.choice(similar) for _ in range(4)];diagnostic=rng.randrange(4);offers[diagnostic]=rng.choice(different)
                    chosen=body.choose(word,offers);count(phase+"_diagnostic",chosen==diagnostic)
                    body.arrive(outcome(offers[chosen]))
                    for pair in rng.sample([p for p in different if p!=offers[chosen]],min(4,len(different)-1)):
                        body.scene(pair);truth=outcome(pair)
                        for o,noun in enumerate(nouns):query(f"Where is the {noun} after {word}?",truth[o],phase+"_held_out")
                laws.append((tables,sources))
            def simulate(pair,sequence):
                for op in sequence:
                    tables,src=laws[op];pair=tuple(tables[o][pair[src[o]]] for o in range(2))
                return pair
            for i in range(a.cycles):
                initial=tuple(rng.sample(range(4),2));body.scene(initial)
                for length in (1,2,8,a.length):
                    seq=rng.choices(range(4),k=length);expected=simulate(initial,seq)
                    for o,noun in enumerate(nouns):query(f"Where is the {noun} after "+" ".join(names[k] for k in seq)+"?",expected[o],f"composition_{length}")
                # Ask both orders only where their physical answers differ.
                pairs=[(j,k,o) for j in range(4) for k in range(j+1,4) for o in range(2)
                       if simulate(initial,(j,k))[o]!=simulate(initial,(k,j))[o]]
                if pairs:
                    j,k,o=rng.choice(pairs)
                    for seq in ((j,k),(k,j)):query(f"Where is the {nouns[o]} after {names[seq[0]]} {names[seq[1]]}?",simulate(initial,seq)[o],"order_sensitive")
        finally:body.close()
        for kind,v in totals.items():out.writerow((seed,a.effects,a.choice,a.credit,kind,*v))
        sys.stdout.flush()

def joint_interaction(a,folder):
    blank=folder/"newborn.mind"
    subprocess.run([BIN,"relate","--load",a.speaker,"--generations","0","--newborn","--teach","0","--save",blank,
                    "--probe-worlds","1","--cycles","1"],stdout=subprocess.DEVNULL,check=True)
    out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","word","left","right","output","truth","spoken","correct"))
    for seed in a.seeds:
        rng=random.Random(seed);body=Interview(blank,a)
        try:
            for name in ("duvo","kirren"):
                le=rng.sample(range(4),4);ri=rng.sample(range(4),4);labels=[rng.sample(range(4),4) for _ in range(2)]
                def effect(pair):return (labels[0][(le[pair[0]]+ri[pair[1]])&3],labels[1][le[pair[0]]^ri[pair[1]]])
                positions=[(x,y) for x in range(4) for y in range(4)]
                for _ in range(a.cycles):
                    for pair in rng.sample(positions,len(positions)):body.teach(name,pair,effect(pair))
                for pair in rng.sample(positions,len(positions)):
                    body.scene(pair)
                    for o,noun in enumerate(("food","water")):
                        true=DIRECTIONS[effect(pair)[o]];reply=body.question(f"Where is the {noun} after {name}?")
                        out.writerow((seed,name,*pair,noun,true,reply,int(reply==true)))
        finally:body.close()
        sys.stdout.flush()

def latin_squares():
    rows=tuple(permutations(range(4)))
    def extend(prefix):
        if len(prefix)==4:
            yield sum(prefix,())
            return
        for r in rows:
            if all(a!=b for prev in prefix for a,b in zip(prev,r)):
                yield from extend(prefix+(r,))
    return tuple(extend(()))

def completion(a,folder):
    """The oracle enumerates possible apparatuses outside the animal."""
    squares=latin_squares()
    assert len(squares)==576
    blank=folder/"newborn.mind"
    subprocess.run([BIN,"relate","--load",a.speaker,"--generations","0","--newborn","--teach","0","--save",blank,
                    "--probe-worlds","1","--cycles","1"],stdout=subprocess.DEVNULL,check=True)
    out=csv.writer(sys.stdout,lineterminator="\n")
    out.writerow(("seed","trial","condition","clues","scene","side","possible","truth","spoken","twin_truth","length"))
    nouns=("food","water")
    for seed in a.seeds:
        rng=random.Random(seed);body=Interview(blank,a)
        def ask(word,site,side):
            body.scene((site&3,site>>2))
            return body.question(f"Where is the {nouns[side]} after {word}?")
        try:
            for trial in range(a.cycles):
                word="vu"+"".join(rng.choices("abcdefghjklmnpqrstuwxyz",k=7))
                laws=rng.choices(squares,k=2)
                shown=rng.sample(range(16),(4,6,8,10,12,14)[trial%6])
                for s in shown:body.teach(word,(s&3,s>>2),tuple(law[s] for law in laws))
                for side,law in enumerate(laws):
                    compatible=[q for q in squares if all(q[i]==law[i] for i in shown)]
                    for site in range(16):
                        if site in shown:continue
                        answers={q[site] for q in compatible}
                        true=law[site];reply=ask(word,site,side)
                        # A second, unrestricted apparatus can agree on every
                        # demonstration and give a different answer here.
                        out.writerow((seed,trial,"forced" if len(answers)==1 else "ambiguous",len(shown),site,side,
                                      len(answers),DIRECTIONS[true],reply,DIRECTIONS[(true+1)%4],1))
            names=("tevora","malkin")
            laws=[rng.choices(range(16),k=16) for _ in names]
            for word,law in zip(names,laws):
                for i in rng.sample(range(16),16):
                    value=law[i];body.teach(word,(i&3,i>>2),(value&3,value>>2))
            for n in range(a.cycles):
                start=rng.randrange(16)
                for length in (1,2,4,8,a.length):
                    sequence=rng.choices(range(2),k=length);final=start
                    for op in sequence:final=laws[op][final]
                    words=" ".join(names[op] for op in sequence)
                    for side in range(2):
                        truth=DIRECTIONS[(final>>(2*side))&3]
                        out.writerow((seed,n,"composition",16,start,side,1,truth,ask(words,start,side),"",length))
        finally:body.close()
        sys.stdout.flush()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("mode",choices=("breed","audit","interview","interaction","completion"))
    p.add_argument("--speaker",type=Path);p.add_argument("--listener",type=Path,default=ROOT/"examples/inquirer.mind")
    p.add_argument("--save",type=Path);p.add_argument("--seeds",nargs="+",type=int,default=FRESH);p.add_argument("--probe-seed",type=int,default=172933)
    p.add_argument("--generations",type=int,default=160);p.add_argument("--candidates",type=int,default=256)
    p.add_argument("--worlds",type=int,default=4);p.add_argument("--cycles",type=int,default=4);p.add_argument("--probe-worlds",type=int,default=256)
    p.add_argument("--effects",choices=("aliased","distinct","independent","mixed"),default="aliased");p.add_argument("--length",type=int,default=32)
    p.add_argument("--credit",choices=("intact","frozen"),default="intact");p.add_argument("--choice",choices=("learned","random"),default="learned")
    p.add_argument("--origin",choices=("inert","random","inherited"),default="inert");p.add_argument("--breeding",choices=("select","drift"),default="select")
    a=p.parse_args()
    if a.speaker is None:
        a.speaker=ROOT/"examples"/("weaver-sparse.mind" if a.mode in ("interaction","completion") else "naturalist.mind")
    if a.cycles<1 or a.length<1:p.error("positive cycles and length required")
    with tempfile.TemporaryDirectory(prefix="axion-binding-",dir=os.environ.get("TMPDIR","/var/tmp")) as work:
        if a.mode=="interview":interviews(a,Path(work))
        elif a.mode=="interaction":joint_interaction(a,Path(work))
        elif a.mode=="completion":completion(a,Path(work))
        else:population(a,Path(work))
if __name__=="__main__":main()
