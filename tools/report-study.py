#!/usr/bin/env python3
"""Teach and audit private-event reports through the public sensory and vocal ports."""
import argparse
import csv
import os
from pathlib import Path
import random
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
DIRECTIONS=("north","east","south","west")
NOUNS=("food","water")
SEEDS=(162337,349907,579053,886909,1105799,1416191,1771193,2095213)

def token(values):
    return " ".join(DIRECTIONS[v] for v in values)

def event_text(event):
    side,start,end=event
    return f"{NOUNS[side]} {DIRECTIONS[start]} {DIRECTIONS[end]}"

class Language:
    def __init__(self,rng,language):
        self.language=language
        if language=="english":
            self.cues=("happened","earlier")
        else:
            self.cues=("vinta","ravo")
            sounds="daru kele pemi samu tora bovi nalu fira mepo zuna rika lavo tesu jori vuno gavi".split()
            rng.shuffle(sounds)
            self.nouns=sounds[:2];self.dirs=sounds[2:6];self.constants=sounds[6:]
            self.story=list(range(7));rng.shuffle(self.story)
            self.short=list(range(5));rng.shuffle(self.short)
            if language=="island":self.cues=("happened","earlier")

    def words(self,frame,new,old):
        if self.language=="english":
            s,a,b=old if frame else new
            return (NOUNS[s],"moved","from",DIRECTIONS[a],"to",DIRECTIONS[b])
        def parts(e):return [self.nouns[e[0]],self.dirs[e[1]],self.dirs[e[2]]]
        if self.language=="island":
            words=parts(old if frame else new)+self.constants[:2]
            return [words[i] for i in self.short]
        if not frame:
            words=parts(old)+parts(new)+[self.constants[0]]
            return [words[i] for i in self.story]
        words=parts(new)+self.constants[1:3]
        return [words[i] for i in self.short]+(self.constants[3:5] if not new[0] else [])

def advance(rng,places,deltas):
    s=rng.randrange(2);a=places[s];places[s]=(a+rng.choice(deltas))&3
    return s,a,places[s]

def talk(speaker,listener,script,*options):
    run=subprocess.run([BIN,"talk","--speaker",speaker,"--listener",listener,*options],
                       input="\n".join([*script,"/quit",""]),text=True,capture_output=True,check=True)
    if any(line.startswith(("Use /","This animal needs")) for line in run.stdout.splitlines()):
        raise RuntimeError(run.stdout[-1200:])
    return run.stdout.splitlines()

def teach(speaker,listener,dest,language,seed,reserved,lessons,as_reader):
    rng=random.Random(seed);places=[rng.randrange(4),rng.randrange(4)]
    commands=["/audience open","/scene "+token(places)];old=None
    for _ in range(lessons+1):
        new=advance(rng,places,[d for d in (1,2,3) if d!=reserved])
        commands.append("/scene "+token(places))
        if old is not None:
            for k,cue in enumerate(language.cues):
                commands.append(f"/teach-report {cue} "+" ".join(language.words(k,new,old)))
        old=new
    # Shutter both retinas between rearing and the unknown interview scene.
    commands+=["/cover both","/scene north north","/scene north north","/uncover both"]
    lines=talk(speaker,listener,commands,"--save-listener" if as_reader else "--save-speaker",dest)
    accepted=[int(line.split("> ")[1]) for line in lines if line.startswith("report lesson> ")]
    if len(accepted)!=2*lessons:raise RuntimeError("incomplete public teaching session")
    return sum(bool(v&(2 if as_reader else 1)) for v in accepted)

def newborn(a,folder):
    speaker=folder/"writer.mind";reader=folder/"reader.mind"
    subprocess.run([BIN,"narrate","--load",a.speaker,"--listener",a.listener,"--newborn","--generations","0",
                    "--cycles","1","--exposures","4","--probe-worlds","1","--save",speaker,"--save-listener",reader],
                   check=True,stdout=subprocess.DEVNULL)
    return speaker,reader

def study(a,folder):
    babies=newborn(a,folder)
    writer=csv.writer(sys.stdout,lineterminator="\n")
    writer.writerow("seed dialect wire trial cue length current past target spoken receiver earlier exact decoded".split())
    for seed in a.seeds:
        rng=random.Random(seed);language=Language(rng,a.language);reserved=rng.choice((1,2,3))
        speaker,reader=folder/"witness.mind",folder/"addressee.mind"
        teach(*babies,speaker,language,rng.randrange(1<<32),reserved,a.exposures,False)
        teach(*babies,reader,language,rng.randrange(1<<32),reserved,a.exposures,True)
        places=[rng.randrange(4),rng.randrange(4)];script=["/audience closed","/scene "+token(places)];rows=[]
        for t in range(a.trials):
            old=advance(rng,places,[reserved]);script.append("/scene "+token(places))
            new=advance(rng,places,[reserved]);script.append("/scene "+token(places))
            if t&1:script.append("/scene "+token(places))
            for k,cue in enumerate(language.cues):
                words=language.words(k,new,old);script+=["Could you tell me "+cue+" please?","/heard"]
                rows.append((t,k,len(words),event_text(new),event_text(old)," ".join(words)))
        for wire in a.wires:
            lines=talk(speaker,reader,script,"--report-wire",wire)
            reports=[x[8:] for x in lines if x.startswith("animal> ")]
            heard=[x[10:] for x in lines if x.startswith("receiver> ")]
            earlier=[x[9:] for x in lines if x.startswith("earlier> ")]
            if not len(reports)==len(heard)==len(earlier)==len(rows):raise RuntimeError("incomplete transcript")
            for (t,k,n,new,old,target),speech,peer,prior in zip(rows,reports,heard,earlier):
                if a.language!="novel":decoded=prior==old if k else peer==new
                else:decoded=peer==new and (k or prior==old)
                writer.writerow((seed,a.language,wire,t,language.cues[k],n,new,old,target,speech,peer,prior,int(target==speech),int(bool(decoded))))
        sys.stdout.flush()

def exchange(a,folder):
    babies=newborn(a,folder);writer=csv.writer(sys.stdout,lineterminator="\n")
    writer.writerow("seed trial condition input expected reply correct".split())
    for seed in a.seeds:
        rng=random.Random(seed);language=Language(rng,"novel");reserved=rng.choice((1,2,3))
        reader=folder/"reader-reared.mind";teach(*babies,reader,language,rng.randrange(1<<32),reserved,a.exposures,True)
        script=["/audience closed","/cover both"];rows=[]
        for t in range(a.trials):
            places=[rng.randrange(4),rng.randrange(4)]
            old=advance(rng,places,[reserved]);new=advance(rng,places,[reserved])
            for kind,event in (("extract",new),("exchanged_bearings",(new[0],new[2],new[1]))):
                text=" ".join(language.words(0,event,old));expect=" ".join(language.words(1,event,old))
                script += ["/say "+language.cues[0]+" "+text,"/recount "+language.cues[1]]
                rows.append((t,kind,text,expect))
            text=" ".join(language.words(1,new,old))
            script+=["/say "+language.cues[1]+" "+text,"/recount "+language.cues[0]]
            rows.append((t,"missing_previous",text,"(silence)"))
        lines=talk(babies[0],reader,script)
        heard=[x[10:] for x in lines if x.startswith("listener> ")]
        if len(rows)!=len(heard):raise RuntimeError("incomplete acoustic retelling")
        for (t,kind,text,expect),reply in zip(rows,heard):writer.writerow((seed,t,kind,text,expect,reply,int(expect==reply)))
        sys.stdout.flush()

def census(a):
    """Test every physically possible pair of adjacent single-object moves."""
    cues=("happened","earlier") if a.language!="novel" else ("vinta","ravo")
    script=["/audience closed"];events=[]
    for side in range(2):
        for start in range(4):
            for end in range(4):
                if end==start:continue
                old=(side,start,end)
                for later in range(2):
                    for depart in range(4):
                        if side==later and depart!=end:continue
                        for arrive in range(4):
                            if arrive==depart:continue
                            new=(later,depart,arrive);places=[(depart+2)&3,(depart+2)&3]
                            places[later]=depart;places[side]=start
                            script.append("/scene "+token(places))
                            places[side]=end;script.append("/scene "+token(places))
                            places[later]=arrive;script.append("/scene "+token(places))
                            for age in range(2) if a.both else (0,):
                                script.extend([f"Tell me what {cues[age]}?","/heard"]);events.append((age,old,new))
    out=csv.writer(sys.stdout,lineterminator="\n")
    out.writerow("wire age prior current spoken receiver decoded_prior correct".split())
    for wire in a.wires:
        lines=talk(a.speaker,a.listener,script,"--report-wire",wire)
        report=[s[8:] for s in lines if s.startswith("animal> ")];heard=[s[10:] for s in lines if s.startswith("receiver> ")]
        before=[s[9:] for s in lines if s.startswith("earlier> ")]
        assert len(report)==len(events)==len(heard)==len(before)
        for (age,old,new),words,decoded,prior in zip(events,report,heard,before):
            ok=prior==event_text(old) if age else decoded==event_text(new)
            out.writerow((wire,age,event_text(old),event_text(new),words,decoded,prior,int(ok)))
        sys.stdout.flush()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("mode",choices=("rear","interview","exchange","census"))
    p.add_argument("--speaker",type=Path,default=ROOT/"examples/storyteller.mind");p.add_argument("--listener",type=Path,default=ROOT/"examples/confidant.mind")
    p.add_argument("--save-speaker",type=Path);p.add_argument("--save-listener",type=Path)
    p.add_argument("--seeds",type=int,nargs="+",default=SEEDS);p.add_argument("--exposures",type=int,default=96)
    p.add_argument("--trials",type=int,default=24);p.add_argument("--language",choices=("english","novel","island"))
    p.add_argument("--wires",nargs="+",choices=("open","muted","reversed"),default=["open","muted","reversed"])
    p.add_argument("--both",action="store_true",help="request both event ages in the exhaustive interview")
    a=p.parse_args()
    if a.language is None:a.language="english" if a.mode in ("rear","census") else "novel"
    if a.exposures<1 or a.trials<1:p.error("positive exposure and trial counts required")
    if a.mode=="rear" and (not a.save_speaker or not a.save_listener or a.save_speaker.resolve()==a.save_listener.resolve()):
        p.error("rear requires two distinct output checkpoints")
    with tempfile.TemporaryDirectory(prefix="axion-speech-",dir=os.environ.get("TMPDIR","/var/tmp")) as scratch:
        if a.mode=="census":census(a)
        elif a.mode=="interview":study(a,Path(scratch))
        elif a.mode=="exchange":exchange(a,Path(scratch))
        else:
            babies=newborn(a,Path(scratch));rng=random.Random(a.seeds[0]);language=Language(rng,a.language)
            for i,path in enumerate((a.save_speaker,a.save_listener)):
                total=teach(*babies,path,language,rng.randrange(1<<32),1,a.exposures,bool(i))
                print(f"accepted={total} available={2*a.exposures} individual={path}")

if __name__=="__main__":main()
