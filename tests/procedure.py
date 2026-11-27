#!/usr/bin/env python3
"""Public CUDA tests with externally generated streams, policies and physical oracles."""
import gzip
import importlib.util
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile

sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("procedure_study",ROOT/"tools/procedure-study.py")
study=importlib.util.module_from_spec(spec);spec.loader.exec_module(study)
run,entries=study.run,study.entries
TRAIL=ROOT/"examples/trailwright.mind";GARDEN=ROOT/"examples/ritual-gardener.mind"
MASK=(1<<64)-1


def talk(mind,lines,save=None):
    return run("talk","--speaker",mind,*(["--save-speaker",save] if save else []),text="\n".join([*lines,"/quit",""]))


def voices(text):return [s[8:] for s in text.splitlines() if s.startswith("animal> ")]


def motor(mind,phrase,*options):
    return entries(run("imitate","--load",mind,"--say",phrase,*options),"interpretation")[0]


def teach(mind,dest,observations):
    run("imitate","--load",mind,"--fresh","--origin","permissive","--generations",0,"--lesson","-","--save",dest,text="\n".join(observations))


def carrier(c):
    s=ord(c)
    return sum((((s&j).bit_count()&1)^bool(s&64))<<j for j in range(64))^0xa6471d3cb95e280f


def basics(work):
    rng=random.Random(62022)
    atoms=["hop","veer","dip","reach","nudge","sip"]
    laws=[("twice",(0,0)),("rim",("l",0,"r")),("join",(0,1)),("after",(1,0)),("braid",(0,1,0)),("echo",(1,0,1,1))]
    def expression(size):
        if size==1:
            i=rng.randrange(6);return atoms[i],chr(97+i)
        verb,order=rng.choice(laws)
        if 1 not in order:a,x=expression(size-1);b=y=""
        else:
            k=rng.randrange(1,size);a,x=expression(k);b,y=expression(size-k)
        return verb+a+b,"".join([x,y][k] if isinstance(k,int) else k for k in order)
    challenges=[expression(rng.randrange(2,8)) for i in range(48)]
    assert voices(talk(TRAIL,[q for q,_ in challenges]))==[y for _,y in challenges]
    for question,response in challenges[:3]:
        assert motor(TRAIL,question,"--chart")["speech"]==response
    n=46;answer=motor(TRAIL,"twice"*n+"rimdip","--from-spike",3*(1<<n)-4,"--limit",9)
    assert int(answer["length"])==3*(1<<n) and answer["speech"]=="rlcr",answer
    assert motor(TRAIL,"twice"*64+"hop")["interpretation"]=="3"
    route=[rng.randrange(6) for _ in range(61)]
    phrase="trace"+"".join(atoms[j] for j in route);deed="".join(chr(97+j) for j in route)+"".join(chr(65+j) for j in reversed(route))
    assert motor(TRAIL,phrase,"--limit",256)["speech"]==deed
    r=entries(run("imitate","--load",TRAIL,"--express",deed),"description")[0]
    assert r["description"]=="1" and motor(TRAIL,r["speech"])["speech"]==deed
    samples=[]
    for c in ("twice"*9+"hop"):
        samples.extend([0]*rng.randrange(1,5));samples.extend([carrier(c)^1<<rng.randrange(64) for _ in range(rng.randrange(5,10))])
    received=entries(run("imitate","--load",TRAIL,"--samples","-","--limit",3,text="\n".join(f"{x:x}" for x in samples)),"continuous_frames")[0]
    assert int(received["detected_crests"])==48 and received["speech"]=="aaa" and int(received["length"])==512,received
    # Identical adjacent crests without a trough are observationally indistinguishable.
    lost=entries(run("imitate","--load",TRAIL,"--samples","-",text="\n".join([f"{carrier('o'):x}"]*8)),"continuous_frames")[0]
    assert lost["detected_crests"]=="1"


def acquisition(work):
    names=["?v","@p","!k","$x",":m","%o"];sounds="ABCDEF";commands=[f"{x} | {y}" for x,y in zip(names,sounds)]
    for i in range(6):
        j=(i+2)%6;commands.extend([f"{names[i]}~{names[j]}! | {sounds[j]}X{sounds[i]}{sounds[j]}",
                                 f"#{names[i]}# | [{sounds[i]}{sounds[i]}]"])
    alien=work/"alien.mind";teach(TRAIL,alien,commands)
    assert motor(alien,"#?v#~#%o#!")["speech"]=="[FF]X[AA][FF]"
    assert motor(alien,"$x~missing!")["interpretation"]=="0"
    # A lifetime reversal keeps unrelated learned lexical responses.
    start=work/"reversible.mind";teach(TRAIL,start,["eto | H","mun | S","tka | M"])
    assert voices(talk(start,["eto","/imitate eto | Q","eto","/rehearse","tka"]))==["H","_","Q","_","M"]
    adapted=work/"adapted.mind";pairs=[("eto","H"),("mun","S"),("tka","M")]
    first=[f"{k} | {v}" for k,v in pairs]
    first+=[f"qa{a}{b} | {v}{w}" for (a,v),(b,w) in zip(pairs,pairs[1:]+pairs[:1])]
    teach(TRAIL,adapted,first);assert motor(adapted,"qatkaeto")["speech"]=="MH"
    reversals=[f"/imitate qa{a}{b} | {w}{v}" for (a,v),(b,w) in zip(pairs,pairs[1:]+pairs[:1])]
    assert voices(talk(adapted,[*reversals,"/rehearse","qaetotka"]))[-1]=="MH"
    rng=random.Random(53);pulse=[rng.getrandbits(64) for _ in range(10)]
    lesson=[f"{pulse[i]:x} | {pulse[i+5]:x}" for i in range(4)]
    for i in range(4):lesson.append(f"{pulse[4]:x} {pulse[i]:x} | {pulse[i+5]:x} {pulse[i+5]:x}")
    unknown=work/"opaque.mind";run("imitate","--load",TRAIL,"--fresh","--origin","permissive","--generations",0,"--lesson","-","--raw","--save",unknown,text="\n".join(lesson))
    reaction=motor(unknown," ".join(f"{k:x}" for k in [pulse[4],pulse[4],pulse[1]]),"--raw")
    expected=f"@{pulse[6]:x}"*4
    assert reaction["speech"]==expected,(reaction,expected)
    inert=entries(run("imitate","--load",TRAIL,"--origin","inert","--generations",0,"--dialect","prefix","--worlds",8,
                      "--rounds",4,"--drills",0,"--guards",0,"--probes",8,"--journeys","none"),"motor_transfer")[0]
    assert int(inert["certified_grammars"])==0


def feedback(work):
    phrase="then mist again sense brush else plant"
    paused=work/"pending.mind"
    assert voices(talk(GARDEN,["/begin "+phrase,"/step closed","/step closed"],paused))==["[ready]","<soak>","[observation needed]"]
    assert voices(talk(paused,["/step 01","/step 05","/step closed"]))==["<sow>","<tend>","[done]"]
    heard=talk(GARDEN,["/greenhouse 16 0","/plot 0 0","/preview 0 "+phrase,"/read-plot 0","/perform 0 "+phrase,"/read-plot 0"])
    pred,move=entries(heard,"grounded_procedure");assert pred["expected"]=="13" and pred["actions"]=="0" and pred["final"]=="0"
    assert move["expected"]==move["final"]=="13" and move["actions"]=="3" and move["path"]=="0,1,5,13",heard
    named=talk(GARDEN,["/greenhouse 16 0","/focus-site pearl",phrase,"/read-plot 0"])
    assert entries(named,"grounded_procedure")[0]["path"]=="0,1,5,13",named
    masked=talk(GARDEN,["/greenhouse 16 0","/allow-motors 0","/perform 0 "+phrase])
    assert entries(masked,"grounded_procedure")[0]["actions"]=="0"
    large=entries(run("imitate","--load",GARDEN,"--forecast","again"*42+phrase),"projected_states")[0]
    def act(s):
        s|=1
        for _ in range(2):
            if (s^(s>>2))&1:s|=4
            else:s=(s&~8)|(((s>>2)&s&1)<<3)
        return s
    ground=list(map(act,range(64)))
    for _ in range(42):ground=[ground[k] for k in ground]
    assert int(large["maximum_moves"])==3*(1<<42) and list(map(int,large["destinations"].split(',')))==ground
    wide=entries(run("imitate","--load",GARDEN,"--forecast","sense"+"again"*63+"mistelsewarm"),"projected_states")[0]
    assert int(wide["minimum_moves"])==1 and int(wide["maximum_moves"])==1<<63 and wide["projected_states"]=="64"
    mixed=entries(run("imitate","--load",GARDEN,"--forecast","sense"+"again"*64+"mistelsewarm"),"projected_states")[0]
    assert mixed["projected_states"]=="32"
    built=entries(run("imitate","--load",GARDEN,"--forecast","again"*64+"mist"),"projected_states")[0]
    assert built["projected_states"]=="0"
    mentor=entries(run("imitate","--load",TRAIL,"--generations",0,"--origin","inherited","--worlds",4,"--drills",0,
                       "--mentor",GARDEN),"mentored")[0];assert mentor["faithful"]=="4",mentor


def planning(work):
    args=["imitate","--load",GARDEN,"--lantern",509,"--contacts",32,"--verify-lantern",509,"--goal-states","aaaaaaaaaaaaaaaa","--horizon",6]
    test=run(*args);phy=entries(test,"physical_audit")[0];plan=entries(test,"policy_search")[0]
    assert phy["agreement"]=="64" and plan["satisfied"]==phy["goals"]=="64"
    assert entries(test,"lantern")[0]["forecast"]=="192"
    done=run(*args,"--start-states","aaaaaaaaaaaaaaaa")
    assert entries(done,"policy_search")[0]["stages"]=="0" and entries(done,"projected_states")[0]["maximum_moves"]=="0"
    exact=entries(run(*args,"--search","exhaustive","--horizon",4),"policy_search")[0];assert exact["satisfied"]=="64"
    blind=run(*args,"--senses","closed");assert entries(blind,"physical_audit")[0]["goals"]=="32"
    under=run("imitate","--load",GARDEN,"--lantern",509,"--contacts",0,"--forecast","mist")
    assert entries(under,"projected_states")[0]["projected_states"]=="0"
    incorrect=run("imitate","--load",GARDEN,"--verify-lantern",509,"--forecast","mist")
    assert int(entries(incorrect,"physical_audit")[0]["agreement"])<64
    rates=entries(run("imitate","--load",GARDEN,"--forecast","then warm sense dust else mist","--replicas",4099,"--episodes",7),"policy_batch")
    assert len(rates)==4 and all(r["represented_moves"]==str(4099*7*2) for r in rates) and int(rates[-1]["shared_table_bytes"])<576


def independent_models(work):
    args=["imitate","--load",GARDEN,"--forecast","then warm sense dust else mist","--replicas",4099,"--families",37,"--episodes",9]
    rows=entries(run(*args),"family_batch")
    assert len(rows)==3 and all(int(r["audited"])==37*64 and int(r["represented_moves"])==4099*9*2 for r in rows)
    assert int(rows[-1]["model_bytes"])<int(rows[1]["model_bytes"])
    survey=entries(run("imitate","--load",ROOT/"examples/lampwright.mind","--survey-rooms",12,"--contacts",16,"--contact-policy","active"),"motor_survey")[0]
    assert int(survey["withheld"])==12*6*48 and int(survey["inferred"])>.9*int(survey["withheld"])


def exchange(work):
    elder=ROOT/"examples/lampwright.mind"
    apprentice=work/"gardening-peer.mind"
    run("imitate","--load",GARDEN,"--mentor",elder,"--worlds",1,"--origin","inherited","--generations",0,"--drills",0,"--save",apprentice)
    out=run("talk","--speaker",elder,"--listener",apprentice,text="\n".join(["/lantern 16 509","/plot 0 0","/pass-score pearl muted","/read-plot 0",
        "/pass-score pearl reversed","/read-plot 0","/pass-score pearl open","/plot 0 0","/focus-site pearl","Proceed.","/read-plot 0","/quit",""]))
    peer,owner=entries(out,"grounded_procedure")
    assert peer["heard_from_peer"]=="1" and owner["recalled"]=="1" and peer["path"]==owner["path"]
    assert int(owner["final"])&1 and owner["expected"]==owner["final"] and int(peer["actions"])>1
    assert voices(out).count("?")>=2 and "make it" in voices(out)
    idle=voices(talk(elder,["/begin misremembered","/step 01"]));assert idle[-1]=="[done]",idle


def checkpoints(work):
    params=["imitate","--load",TRAIL,"--origin","inherited","--generations",0,"--colony",32,"--days",5,"--seed",211,"--drills",128]
    full,half,end=[work/(x+".roost") for x in ("full","half","end")]
    run(*params,"--seasons",3,"--save-roost",full)
    run(*params,"--seasons",1,"--save-roost",half)
    run("imitate","--resume-roost",half,"--seasons",2,"--save-roost",end)
    assert full.read_bytes()==end.read_bytes()
    broken=work/"bad.mind";data=bytearray(GARDEN.read_bytes());version=struct.unpack_from("<I",data,8)[0]
    composition=int.from_bytes(data[-12:-8],"little") if version&4194304 else 0;back=8+composition+3144*bool(version&1048576)
    struct.pack_into("<I",data,len(data)-back-4,1)
    h=14695981039346656037
    for v in data[:-8]:h=((h^v)*1099511628211)&MASK
    data[-8:]=h.to_bytes(8,"little");broken.write_bytes(data)
    result=subprocess.run([study.BINARY,"imitate","--load",broken,"--say","mist"],capture_output=True,text=True)
    assert result.returncode and "invalid procedural circuit" in result.stderr,result.stderr


with tempfile.TemporaryDirectory(prefix="axion-procedure-test-",dir=os.environ.get("TMPDIR","/var/tmp")) as folder:
    work=Path(folder);os.environ["AXION_JOURNAL_DIR"]=str(work/"journal")
    for case in (basics,acquisition,feedback,planning,independent_models,exchange,checkpoints):
        case(work);print("PASS",case.__name__,flush=True)
    for directory in (work/"journal"/"runs").iterdir():
        if not (directory/"summary.json").is_file():continue
        summary=json.loads((directory/"summary.json").read_text())
        assert len(summary["narrative"])==3 and summary["bullets"],summary
        assert (directory/"plate.webp").read_bytes()[8:12]==b"WEBP"
        assert json.load(gzip.open(directory/"observations.json.gz"))["command"] in ("talk","imitate")
