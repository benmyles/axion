#!/usr/bin/env python3
"""Rear and interview a CUDA animal using public observations, words and physical interventions."""
import argparse
from collections import deque
import csv
from pathlib import Path
import os
import random
import re
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
NOUNS="pearl acorn shell flint pouch basket crate chest tern ibis swift wren cove quay inlet delta".split()
WORDS="soak heat sow tend pollinate ripen moist warm sown green flowering fruiting dry cold unsown dormant budless bare how suppose why make describe edible forage".split()
NOVEL="tevu noka padu meli kora vushe jema lume tari gepo wavu suni ravo weki fumel nidu vepu qomi yatu luni dato zori femu rodu kafo".split()
SEEDS=(217217,541197,811945,1065553,1409311,1810069,2089363,2470393)

def greenhouse():
    law=[[(j,j,10) for j in range(6)] for _ in range(6)]
    for a in range(3):law[a][a]=(a,a,15)
    law[1][0]=(0,0,0)
    for a,p,q in [(3,0,2),(4,1,3),(5,4,3)]:law[a][a]=(p,q,8)
    return law

def mechanism(rng,family):
    law=greenhouse()
    if family=="feedback":
        # The physical wiring, signed inputs and co-effects differ from the
        # staged maturation sequence used during genome selection.
        for a in range(6):
            for j in rng.sample(range(6),rng.randrange(2,5)):
                p,q=rng.sample(range(6),2);law[a][j]=(p,q,rng.randrange(16))
    elif family=="latches":
        for a in range(6):
            p=(a+5)%6;law[a][a]=(a,p,rng.choice((6,7,9,13,14)))
            if rng.randrange(2):law[a][(a+2)%6]=(a,p,rng.choice((2,4,8,11,12)))
    return law

def step(law,a,state):
    result=0
    for j,(left,right,table) in enumerate(law[a]):
        index=((state>>left)&1)+2*((state>>right)&1)
        result|=((table>>index)&1)<<j
    return result

def play(law,start,motors):
    for a in motors:start=step(law,a,start)
    return start

def search(law,start,require,value,allowed=63):
    queue=deque([(start,())]);seen={start}
    while queue:
        state,path=queue.popleft()
        if state&require==value:return path
        for a in range(6):
            if not allowed>>a&1:continue
            other=step(law,a,state)
            if other not in seen:seen.add(other);queue.append((other,(*path,a)))
    return None

def omission(law,start,sequence,cut,require,value):
    before=play(law,start,sequence[:cut]);after=step(law,sequence[cut],before);diff=before^after
    ending=play(law,before,sequence[cut+1:]);lost=(ending^value)&require
    if not lost:return 0,0,0
    possible=[subset for subset in range(64) if subset&~diff==0 and play(law,before^subset,sequence[cut+1:])&require==value]
    if not possible:return None
    repair=min(possible,key=lambda x:(x.bit_count(),x));return repair,after&repair,lost

def signs(words,require,value):
    return [words[(6 if value>>j&1 else 12)+j] for j in range(6) if require>>j&1]

def reason_words(words,site,action,mask,values,lost,goal,reverse):
    missing=signs(words,mask,~values);targets=signs(words,lost,goal)
    if reverse:return " ".join([*targets,"needs",NOUNS[site],words[action],"since",*missing])
    return " ".join(["skipping",words[action],"leaves",NOUNS[site],*missing,"blocking",*targets])

class Talk:
    def __init__(self,a,extra=()):
        self.process=subprocess.Popen([BIN,"talk","--speaker",a.speaker,"--listener",a.peer,*extra],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
    def __enter__(self):return self
    def __exit__(self,*exc):
        if self.process.poll() is None:
            self.process.stdin.write("/quit\n");self.process.stdin.close()
        self.process.stdout.read()
        if self.process.wait() and not exc[0]:raise RuntimeError("CUDA interview failed")
    def present(self,line):
        self.process.stdin.write(line+"\n");self.process.stdin.flush()
        for answer in self.process.stdout:
            if answer.startswith(("garden>","animal>","Board access changed.","Available motors changed.","Goal inputs changed.","The witness learned the description.")):
                return answer.strip()
            if answer.startswith("Invalid garden"):raise ValueError(line)
        raise RuntimeError("no response to "+line)
    def control(self,line):
        reply=self.present(line)
        if not reply.startswith(("garden> 1","Board access changed.","Available motors changed.","Goal inputs changed.")):raise RuntimeError((line,reply))
        return reply
    def ask(self,line):
        reply=self.present(line)
        if not reply.startswith("animal> "):raise RuntimeError((line,reply))
        return reply[8:]

def arrange(t,law):
    t.control("/greenhouse 16 0")
    for a,row in enumerate(law):
        for j,(left,right,value) in enumerate(row):t.control(f"/mechanism {a} {j} {left} {right} {value}")

def speech(t,words,rng):
    # Each form initially co-occurs with two possible meanings. Neither its
    # spelling nor the ordering of either display identifies which one it names.
    order=list(range(len(words)));rng.shuffle(order)
    for shift in (1,5):
        for i,x in enumerate(order):
            y=order[(i+shift)%len(order)];names=[words[x],words[y]];rng.shuffle(names)
            t.control(f"/cohear {(1<<x)|(1<<y)} "+" ".join(names))

def alternative(words,axis,low,high,reversed_order):
    when_low=words[low] if low<6 else "yes";when_high=words[high] if high<6 else "yes"
    if reversed_order:return [when_low,"under",words[12+axis],"else",when_high]
    return ["if",words[6+axis],when_high,"otherwise",when_low]

def teach_choice(t,words,rng,reverse):
    for j in range(24):
        axis=j%6;lo=rng.randrange(64)&~(1<<axis);low,high=rng.sample(range(6),2)
        phrase=" ".join(alternative(words,axis,low,high,reverse))
        t.control(f"/show-choice {lo} {low} {lo|(1<<axis)} {high} {phrase}")

def decoded(utterance,words):
    if utterance in ("yes","no","unsure","(silence)"):return ()
    return tuple(words[:6].index(v) for v in utterance.split())

def execute_clause(utterance,words,law,start,reverse):
    tokens=utterance.split()
    if len(tokens)>=5:
        for sense in range(6):
            for low in range(7):
                for high in range(7):
                    if tokens[-5:]!=alternative(words,sense,low,high,reverse):continue
                    prefix=decoded(" ".join(tokens[:-5]),words)
                    motor=high if play(law,start,prefix)>>sense&1 else low
                    return (*prefix,motor) if motor<6 else prefix
    return decoded(utterance,words)

def ask_plan(t,words,site,require,value):
    return t.ask(f"{words[18]} can {NOUNS[site]} become "+" and ".join(signs(words,require,value))+"?")

def teach_frame(t,rng,words,reverse):
    law=greenhouse();lessons=0
    for _ in range(240):
        site=rng.randrange(16);state=rng.randrange(64);goal=1<<rng.randrange(6);target=(~state)&goal
        path=search(law,state,goal,target)
        if not path:continue
        t.control(f"/plot {site} {state}");actual=decoded(ask_plan(t,words,site,goal,target),words)
        if not actual or play(law,state,actual)&goal!=target:continue
        cut=rng.randrange(len(actual));cause=omission(law,state,actual,cut,goal,target)
        if not cause or cause[0].bit_count()!=1:continue
        statement=reason_words(words,site,actual[cut],*cause,target,reverse)
        t.control(f"/teach-reason {cut} "+statement);lessons+=1
        if lessons==32:break
    if lessons<8:raise RuntimeError("insufficient independently validated explanations during rearing")

def newborn(a,folder):
    pair=[]
    for label,source in (("s",a.speaker),("p",a.peer)):
        dest=folder/(label+"-new.mind")
        subprocess.run([BIN,"cultivate","--load",source,"--germline",a.speaker,"--generations","0","--newborn","--probe-worlds","1",
                        "--lessons","1","--save",dest],stdout=subprocess.DEVNULL,check=True);pair.append(dest)
    return pair

def rear(a,folder,words,seed,reverse,outputs):
    pair=newborn(a,folder)
    for side,out in enumerate(outputs):
        class Participants:pass
        people=Participants();people.speaker,people.peer=pair[side],pair[1-side]
        rng=random.Random(seed^((side+1)*0x6187))
        with Talk(people,("--save-speaker",out)) as t:
            speech(t,words,rng);t.control("/cohear 33554432 it");teach_choice(t,words,rng,reverse);t.control("/greenhouse 16 0");t.control(f"/practice 64 {rng.randrange(1<<31)}")
            teach_frame(t,rng,words,reverse);t.control("/greenhouse 16 0")

def interviews(a,folder,out):
    for seed in a.seeds:
        rng=random.Random(seed);words=NOVEL.copy();rng.shuffle(words);reverse=bool(rng.randrange(2))
        pair=(folder/"speaker.mind",folder/"peer.mind");rear(a,folder,words,seed,reverse,pair)
        class Participants:pass
        people=Participants();people.speaker,people.peer=pair
        # All three settings start from the same reared animals, then a new
        # law is learned by observing the physical results of interventions.
        for family in ("greenhouse","feedback","latches"):
            law=mechanism(rng,family)
            with Talk(people) as t:
                arrange(t,law);t.control(f"/practice {a.lessons} {rng.randrange(1<<31)}")
                for trial in range(a.trials):
                    site=rng.randrange(16);start=rng.randrange(64);require=rng.randrange(1,64);value=rng.randrange(64)&require;enabled=rng.choice((63,63,63,rng.randrange(1,64)))
                    t.control(f"/allow-motors {enabled}");t.control(f"/plot {site} {start}")
                    best=search(law,start,require,value,enabled)
                    said=ask_plan(t,words,site,require,value);seq=execute_clause(said,words,law,start,reverse)
                    valid=(said=="no") if best is None else said=="yes" if not best else bool(seq) and play(law,start,seq)&require==value and all(enabled>>x&1 for x in seq)
                    out.writerow((seed,family,trial,"plan",start,require,value,enabled,-1 if best is None else len(best),said,int(valid),int(valid and (best is None or len(seq)==len(best)))))
                    if valid and seq:
                        cut=rng.randrange(len(seq));cut=seq.index(seq[cut])
                        cause=omission(law,start,seq,cut,require,value)
                        reference=reason_words(words,site,seq[cut],*cause,value,reverse) if cause and cause[0] else "no"
                        why=t.ask(f"{words[20]} {NOUNS[site]} {words[seq[cut]]}?")
                        out.writerow((seed,family,trial,"omission",start,require,value,enabled,reference,why,int(why==reference),""))
                        other=(site+1)%16;t.control(f"/plot {other} {start}")
                        t.control("/gaze peer");result=t.control(f"/pass-plan {other} open");t.control("/gaze both");end=int(re.search(r"state=(\d+)",result)[1])
                        out.writerow((seed,family,trial,"peer",start,require,value,enabled,play(law,start,seq),end,int(end==play(law,start,seq)),""))
                    query=[rng.randrange(6) for _ in range(1+rng.randrange(8))]
                    future=play(law,start,query);goal=1<<rng.randrange(6);yes=bool(rng.randrange(2));target=(future if yes else ~future)&goal
                    heard=t.ask(f"{words[19]} {NOUNS[site]} "+" then ".join(words[j] for j in query)+" "+" ".join(signs(words,goal,target))+"?")
                    out.writerow((seed,family,trial,"forecast",start,goal,target,enabled,"yes" if yes else "no",heard,int(heard==("yes" if yes else "no")),""))
            sys.stdout.flush()

def forks(a,folder,out):
    for seed in a.seeds:
        rng=random.Random(seed);words=NOVEL.copy();rng.shuffle(words);reverse=bool(rng.randrange(2))
        sites=rng.sample(range(6),6);operations=rng.sample(range(6),6);negate=rng.randrange(64)
        def coordinates(x):
            return negate^sum(((x>>j)&1)<<k for j,k in enumerate(sites))
        twins=[]
        for hidden in (4,5):
            law=[[(j,j,10) for j in range(6)] for _ in range(6)]
            law[0][0]=(3,3,5);law[0][1]=(hidden,hidden,10)
            for act in (1,2):
                law[act][2]=(0,1,8 if act==1 else 2);law[act][0]=(0,0,0);law[act][3]=(3,3,15)
            converted=[[None]*6 for _ in range(6)]
            for op,r in enumerate(law):
                for k,(l,h,lut) in enumerate(r):
                    l,h,y=sites[l],sites[h],sites[k];truth=0
                    for u in range(4):
                        idx=u^((negate>>l)&1)^(((negate>>h)&1)<<1)
                        truth|=(((lut>>idx)^(negate>>y))&1)<<u
                    converted[operations[op]][y]=(l,h,truth)
            twins.append(converted)
        start=coordinates(16);goal=1<<sites[2];target=coordinates(4)&goal;front=deque([(start,start)]);seen=set(front)
        # Exact reachability in the product of the two real worlds covers
        # every finite open-loop motor sequence, including arbitrarily long ones.
        while front:
            left,right=front.popleft()
            if left&goal==target and right&goal==target:raise RuntimeError("an open-loop plan solves both worlds")
            for op in range(6):
                x=step(twins[0],op,left),step(twins[1],op,right)
                if x not in seen:seen.add(x);front.append(x)
        babies=newborn(a,folder);pupil=folder/"uncertain.mind";companion=folder/"sole-listener.mind"
        class Pair:pass
        pair=Pair();pair.speaker,pair.peer=babies
        with Talk(pair,("--save-speaker",pupil,"--save-listener",companion)) as t:
            speech(t,words,rng);teach_choice(t,words,rng,reverse);t.control("/greenhouse 16 0");t.control("/gaze witness")
            lesson=[(op,coordinates(x)) for op in range(6) for x in range(64) if op!=operations[0] or ((x>>4)&1)==((x>>5)&1)]
            rng.shuffle(lesson)
            for op,x in lesson:
                y=step(twins[0],op,x);assert y==step(twins[1],op,x)
                t.control(f"/trial {op} {x} {y}")
        pair.speaker,pair.peer=pupil,companion;axis=sites[1];lo,hi=(operations[2],operations[1])
        if negate>>axis&1:lo,hi=hi,lo
        reference=" ".join([words[operations[0]],*alternative(words,axis,lo,hi,reverse)])
        for hidden,law in enumerate(twins):
            for wire in ("open","muted","reversed","covered","self"):
                with Talk(pair) as t:
                    arrange(t,law);t.control(f"/plot 0 {start}");t.control(f"/plot 1 {start}")
                    plan=ask_plan(t,words,0,goal,target)
                    if wire=="self":
                        actuated=t.ask(f"{words[21]} {NOUNS[0]} "+" ".join(signs(words,goal,target))+".")
                        result=t.control("/read-plot 0")
                    else:
                        if wire=="covered":t.control("/gaze witness")
                        result=t.control(f"/pass-plan 1 {'open' if wire=='covered' else wire}")
                        actuated=result.partition("speech=")[2]
                    end=int(re.search(r"state=(\d+)",result)[1])
                    out.writerow((seed,hidden,wire,len(seen),start,goal,target,plan,reference,int(plan==reference),actuated,end,int(end&goal==target)))
        sys.stdout.flush()

def categories(a,out):
    for seed in a.seeds:
        rng=random.Random(seed);law=mechanism(rng,"feedback");words=WORDS;name="cured"
        with Talk(a) as t:
            arrange(t,law);t.control(f"/practice 64 {seed}")
            # These concepts include non-conjunctive desired states. A learner
            # hears a name and the verdict for an object it can inspect.
            for phase in range(3):
                left,right=rng.sample(range(6),2);lut=rng.choice((6,7,8,9,11,13,14))
                def acceptable(state):return (lut>>(((state>>left)&1)+2*((state>>right)&1)))&1
                training=rng.sample(range(64),40);unseen=[x for x in range(64) if x not in training]
                for i,x in enumerate(training):
                    t.control(f"/plot {i&7} {x}")
                    if phase&1:
                        response=t.present(f"{NOUNS[i&7]} {name} {'yes' if acceptable(x) else 'no'}.")
                        if response!="The witness learned the description.":raise RuntimeError(response)
                    else:t.control(f"/approve {name} {i&7} {acceptable(x)}")
                targets={x for x in range(64) if acceptable(x)}
                for trial in range(a.trials):
                    x=rng.choice(unseen);site=rng.randrange(8,16);t.control(f"/plot {site} {x}")
                    heard=t.ask(f"Suppose {NOUNS[site]} {name}?")
                    truth="yes" if x in targets else "no"
                    out.writerow((seed,phase,trial,"recognition",x,truth,heard,int(heard==truth)))
                    plans=[search(law,x,63,goal) for goal in targets];possible=[s for s in plans if s is not None]
                    optimum=min(map(len,possible)) if possible else -1
                    said=t.ask(f"How can {NOUNS[site]} become {name}?");actions=decoded(said,words)
                    accepted=(said=="no" if optimum<0 else said=="yes" if not optimum else len(actions)==optimum and play(law,x,actions) in targets)
                    out.writerow((seed,phase,trial,"goal",x,optimum,said,int(accepted)))
        sys.stdout.flush()

def hierarchies(a,out):
    for seed in a.seeds:
        rng=random.Random(seed);axes=rng.sample(range(6),6);labels=rng.sample("pexi nolu rimi tepa mavo".split(),5)
        masks=[sum(1<<j for j in axes[:depth]) for depth in range(2,7)]
        training=[rng.sample(range(64),a.examples) for _ in masks];polarity=[rng.randrange(2) for _ in masks]
        goals=greenhouse()
        for reuse in ("open","cut"):
            with Talk(a) as t:
                t.control("/greenhouse 16 0");t.control("/goal-reuse "+reuse)
                for level,(features,word,examples) in enumerate(zip(masks,labels,training)):
                    def truth(x):return ((x&features).bit_count()&1)^polarity[level]
                    for i,x in enumerate(examples):
                        t.control(f"/plot {i&7} {x}");t.control(f"/approve {word} {i&7} {truth(x)}")
                    for state in sorted(set(range(64))-set(examples)):
                        site=8+(state&7);t.control(f"/plot {site} {state}")
                        heard=t.ask(f"Suppose {NOUNS[site]} {word}?");target="yes" if truth(state) else "no"
                        out.writerow((seed,reuse,level+2,word,"recognition",state,target,heard,int(heard==target)))
                        eligible=[x for x in range(64) if truth(x) and x&20==20]
                        paths=[search(goals,state,63,x) for x in eligible];lengths=[len(p) for p in paths if p is not None];opt=min(lengths) if lengths else -1
                        said=t.ask(f"How can {NOUNS[site]} become sown flowering and {word}?");actions=decoded(said,WORDS)
                        good=said=="no" if opt<0 else said=="yes" if not opt else len(actions)==opt and play(goals,state,actions) in eligible
                        out.writerow((seed,reuse,level+2,word,"goal",state,opt,said,int(good)))
            sys.stdout.flush()

def certify(a,folder,out):
    babies=newborn(a,folder)
    class Pair:pass
    pair=Pair();pair.speaker,pair.peer=babies
    for seed in a.seeds:
        rng=random.Random(seed)
        for family in ("greenhouse","feedback","latches"):
            law=mechanism(rng,family);physics=folder/"physical.txt";trained=folder/"observed.mind"
            physics.write_text("".join(f"{p} {q} {t}\n" for row in law for p,q,t in row))
            with Talk(pair,("--save-speaker",trained)) as t:
                arrange(t,law);t.control(f"/practice {a.lessons} {rng.randrange(1<<31)}")
            run=subprocess.run([BIN,"cultivate","--load",trained,"--certify",physics],capture_output=True,text=True,check=True)
            fields=dict(re.findall(r'([^ =]+)=([^ ]+)',next(s for s in run.stdout.splitlines() if s.startswith("causal_census="))))
            out.writerow((seed,family,a.lessons,*(fields[k] for k in ("trials","correct","reachable","already","unreachable","missed","wrong","attempted","completed","stalled","unsafe","policy_bytes"))))
        sys.stdout.flush()

def breeding(a,out):
    for seed in a.seeds:
        for arm in ("select","drift"):
            command=[BIN,"cultivate","--load",ROOT/"examples/cartographer.mind","--origin","random","--generations","100","--candidates","192","--worlds","6",
                     "--cycles","3","--lessons","36","--probe-worlds","256","--seed",str(seed),"--probe-seed","764321","--breeding",arm]
            result=subprocess.run(command,capture_output=True,text=True,check=True)
            population=next(dict(re.findall(r'([^ =]+)=([^ ]+)',line)) for line in result.stdout.splitlines() if line.startswith("causal_population="))
            genes=next(dict(re.findall(r'([^ =]+)=([^ ]+)',line)) for line in result.stdout.splitlines() if "agency_bytes=" in line)
            for line in result.stdout.splitlines():
                if line.startswith("cultivation="):
                    r=dict(re.findall(r'([^ =]+)=([^ ]+)',line))
                    out.writerow((seed,arm,population["perfect"],population["individuals"],population["total"],population["available"],genes["plasticity"],genes["selection"],genes["replay"],
                                  *(r[k] for k in ("cultivation","worlds","forecasts","goals","actions","reasons","impossible","decided","compiled","essential"))))
            sys.stdout.flush()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("mode",choices=("rear","interview","forks","categories","hierarchies","certify","breeding"))
    p.add_argument("--speaker",type=Path,default=ROOT/"examples/gardener.mind");p.add_argument("--peer",type=Path,default=ROOT/"examples/apprentice.mind")
    p.add_argument("--save-speaker",type=Path);p.add_argument("--save-peer",type=Path);p.add_argument("--seeds",nargs="+",type=int,default=SEEDS)
    p.add_argument("--lessons",type=int,default=64);p.add_argument("--trials",type=int,default=32);p.add_argument("--examples",type=int,default=32);a=p.parse_args()
    if not 1<=a.lessons<=256 or not 1<=a.examples<64 or a.trials<1:p.error("invalid teaching or test count")
    with tempfile.TemporaryDirectory(prefix="axion-agency-",dir=os.environ.get("TMPDIR","/var/tmp")) as d:
        folder=Path(d)
        if a.mode=="rear":
            if not a.save_speaker or not a.save_peer or a.save_speaker.resolve()==a.save_peer.resolve():p.error("rear needs distinct outputs")
            rear(a,folder,WORDS,a.seeds[0],False,(a.save_speaker,a.save_peer))
        elif a.mode=="interview":
            out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","physics","trial","test","start","require","value","enabled","reference","speech_or_state","correct","shortest"));interviews(a,folder,out)
        elif a.mode=="forks":
            out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","world","wire","product_states","start","require","value","plan","reference","clause_exact","emission","end","reached"));forks(a,folder,out)
        elif a.mode=="categories":
            out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","phase","trial","test","start","truth_or_optimum","spoken","correct"));categories(a,out)
        elif a.mode=="hierarchies":
            out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","reuse","arity","word","test","start","reference","speech","correct"));hierarchies(a,out)
        elif a.mode=="certify":
            out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","physics","examples_per_motor","trials","correct","reachable","already","unreachable","missed","wrong","attempted","completed","stalled","unsafe","bytes"));certify(a,folder,out)
        else:
            out=csv.writer(sys.stdout,lineterminator="\n");out.writerow(("seed","breeding","perfect","individuals","total","available","plasticity","selection","replay","intervention","worlds","forecasts","goals","actions","reasons","impossible","decided","compiled","essential"));breeding(a,out)
if __name__=="__main__":main()
