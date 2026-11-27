#!/usr/bin/env python3
"""Independently reconstruct the modular chemistry, tool interventions and resident hand state."""
from collections import Counter
import gzip
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
EXE=Path(os.environ.get("AXION_BIN",ROOT/"axion")).resolve()
MASK=(1<<32)-1


def mix(x):
    x&=MASK
    x=((x^(x>>16))*0x21f0aaad)&MASK
    x=((x^(x>>15))*0x735a2d97)&MASK
    return x^(x>>15)


def digest(data):
    h=14695981039346656037
    for byte in data:h=((h^byte)*1099511628211)&0xffffffffffffffff
    return h.to_bytes(8,"little")


def face(p,d):
    return (p[0]>>(2*((d-(p[3]&1))%4)))&3


def linked(p,q,d):
    x,y=face(p,d),face(q,d^2)
    return bool(x and y and (x==3 or y==3 or x!=y))


def snapshot(path):
    raw=path.read_bytes()
    assert raw[:8]==b"AXFLOOD\0" and raw[-8:]==digest(raw[:-8])
    v,tick,side,tiles,seed=struct.unpack_from("<5I",raw,8)
    assert 9<=v<=12
    n=side**2;base=368+n*(24+4+172+96+16*tiles+96)
    if (v-9)&1:base+=104+n*177
    if (v-9)&2:base+=72+12*n
    params=struct.unpack_from("<4I",raw,base)
    pieces,initial_charge,*groups=struct.unpack_from("<5Q",raw,base+16)
    accounts=struct.unpack_from("<13Q",raw,base+56)
    pos=base+160
    field=[list(raw[pos+4*i:pos+4*i+4]) for i in range(n)]
    pos+=4*n
    workers=[raw[pos+72*i:pos+72*(i+1)] for i in range(n)]
    assert pos+72*n+8==len(raw)
    stock=field+[list(w[56:60]) for w in workers]
    assert sum(bool(g[0]) for g in stock)==pieces
    assert [sum(face(g,d)==k for g in stock for d in range(4)) for k in (1,2,3)]==groups
    assert sum(g[2] for g in stock)+accounts[2]+accounts[12]==initial_charge+accounts[0]
    assert all((p[0] and p[2]<=2+(p[1]&15) and p[3]<16) or not any(p) for p in stock)
    organs=368+n*(24+4+172)
    residents=[raw[organs+i*96:organs+(i+1)*96] for i in range(n)]
    bodies=[int.from_bytes(a[28:30],"little") for a in residents]
    soil=[struct.unpack_from("<7H2B2HI",raw,368+24*i) for i in range(n)]
    charge=sum(p[2] for p in stock)
    soil_energy=sum(sum(x[4:7])+sum(x[9:11]) for x in soil)
    body=sum(bodies)+sum(struct.unpack_from("<H",a,76)[0] for a in residents)
    pending=0
    if (v-9)&1:
        gut=368+n*(24+4+172+96+16*tiles+96)+104
        pending=sum(sum(raw[gut+176*i+104:gut+176*i+136]) for i in range(n))
    base_energy=struct.unpack_from("<Q",raw,88)[0];light,heat=struct.unpack_from("<2Q",raw,104)
    assert base_energy+light==soil_energy+body+charge+pending+heat
    return dict(raw=raw,version=v,base=base,field=field,workers=workers,soil=soil,tick=tick,side=side,seed=seed,
                residents=residents,season=struct.unpack_from("<I",raw,32)[0],params=params,groups=groups,accounts=accounts)


def neighborhood(i,d,n):
    x,y=i%n,i//n
    return (x+(d==1)-(d==3))%n+n*((y+(d==2)-(d==0))%n)


def topology(s):
    n=s["side"];field=s["field"];remaining={i for i,g in enumerate(field) if g[0]}
    clusters=[]
    while remaining:
        todo=[remaining.pop()];degrees=[];touched=0
        for i in todo:
            candidates=[neighborhood(i,d,n) for d in range(4) if (s["params"][1]&1) and linked(field[i],field[neighborhood(i,d,n)],d)]
            degrees.append(len(candidates));touched+=bool(field[i][3]&2)
            for j in candidates:
                if j in remaining:remaining.remove(j);todo.append(j)
        clusters.append((len(todo),sum(degrees)//2,sum(d>=3 for d in degrees),touched))
    return dict(assemblies=len(clusters),largest=max((n for n,_,_,_ in clusters),default=0),
                bonds=sum(v[1] for v in clusters),independent_loops=sum(e+1-n for n,e,_,_ in clusters),
                branches=sum(v[2] for v in clusters),handled_parts=sum(v[3] for v in clusters))


def replay(s,steps,mode):
    n=s["side"];p=[g[:] for g in s["field"]];raw=[r[6] for r in s["soil"]];product=[r[5] for r in s["soil"]];flux=0;made=0
    if mode=="rearranged":
        old=p[:];seed=mix(s["seed"]^s["tick"])
        for at in range(n*n):
            x,y=at%n,at//n;bx=x&~3;by=y&~3;k=mix(seed^mix(bx+by*n))
            j=(((x&3)+4*(y&3))*((k&7)*2+1)+((k>>11)&15))&15
            p[at]=old[bx+(j&3)+(by+j//4)*n][:]
    for tick in range(s["tick"],s["tick"]+steps):
        d=1+((tick>>1)&1)
        for i in range(n*n):
            x,y=i%n,i//n
            if (x if d==1 else y)%2!=tick%2:continue
            j=neighborhood(i,d,n);a,b=p[i],p[j];cap=lambda v:2+(v[1]&15)
            allowed=(s["params"][1]&1) and mode!="insulated" and (mode!="handled_cut" or not ((a[3]|b[3])&2))
            if allowed and linked(a,b,d):
                diff=(4*a[2]+((a[1]>>4)&3))-(4*b[2]+((b[1]>>4)&3))
                hi,lo=(a,b) if diff>0 else (b,a)
                if abs(diff)>=4 and hi[2] and lo[2]<cap(lo):
                    hi[2]-=1;lo[2]+=1;flux+=1
                if (tick+3*i)%32==0 and a[2]+1>=cap(a) and b[2]:
                    left=2*((d+1-(a[3]&1))%4);right=2*((d+2-(b[3]&1))%4)
                    swap=((a[0]>>left)^(b[0]>>right))&3
                    l,r=a[0]^(swap<<left),b[0]^(swap<<right)
                    if swap and l and r:a[0],b[0]=l,r;b[2]-=1
            for loc in (i,j):
                g=p[loc]
                if not g[0]:continue
                xx,yy=loc%n,loc//n
                sky=mix(s["seed"]^mix(xx//5+129*(yy//5))^mix(tick//s["season"]))
                if (tick+3*xx+yy)%4==0 and ((sky>>9)&3) and (sky&3)==((g[1]>>4)&3) and g[2]<cap(g):g[2]+=1
                if (s["soil"][loc][7]&3)==((g[1]>>2)&3) and raw[loc] and g[2]>=1+((g[1]>>5)&3) and product[loc]<=65533:
                    raw[loc]-=1;product[loc]+=2;g[2]-=1;made+=1
                if g[1]&128:
                    if g[2]+1>=cap(g):g[3]|=1
                    elif g[2]<=cap(g)//3:g[3]&=~1
    return made,flux


def port_response(state, row, schedule, arm):
    n=state["side"];bed=state["field"];root=int(row["artifact_signature"]);members={root};walk=[root]
    for i in walk:
        for d in range(4):
            j=neighborhood(i,d,n)
            if j not in members and linked(bed[i],bed[j],d):members.add(j);walk.append(j)
    assert len(members)==int(row["parts"])
    pieces={i:bed[i][:] for i in members}
    for p in pieces.values():p[2]=0
    lamp_a,lamp_b,sink=(int(row[k]) for k in ("lamp_a","lamp_b","sink"));products=0
    for t in range(64):
        d=1+((t>>1)&1)
        for i,p in pieces.items():
            j=neighborhood(i,d,n)
            if arm==1 or j not in pieces or ((i%n if d==1 else i//n)^t)&1:continue
            other=pieces[j]
            if not linked(p,other,d):continue
            difference=4*(p[2]-other[2])+((p[1]>>4)&3)-((other[1]>>4)&3)
            donor,recipient=(p,other) if difference>0 else (other,p)
            if abs(difference)>=4 and donor[2] and recipient[2]<2+(recipient[1]&15):
                donor[2]-=1;recipient[2]+=1
            if (t+3*i)%32==0 and p[2]+1>=2+(p[1]&15) and other[2]:
                left=2*((d+1-(p[3]&1))%4);right=2*((d+2-(other[3]&1))%4)
                swap=((p[0]>>left)^(other[0]>>right))&3
                x,y=p[0]^(swap<<left),other[0]^(swap<<right)
                if swap and x and y:p[0],other[0]=x,y;other[2]-=1
        early=t<16 and t%2==0;late=16<=t<32 and t%2==0
        lamps=(bool(early and schedule&1),bool(early and schedule&2)) if schedule<4 else (early,early) if schedule==4 else (early,late) if schedule==5 else (late,early)
        for i,p in pieces.items():
            capacity=2+(p[1]&15)
            if ((i==lamp_a and lamps[0]) or (i==lamp_b and lamps[1])) and p[2]<capacity:p[2]+=1
            if i==sink and (schedule<4 or t>=40) and p[2]>=1+((p[1]>>5)&3):
                p[2]-=1;products+=1
            if (p[1]&128) and arm!=2:
                if p[2]+1>=capacity:p[3]|=1
                elif p[2]<=capacity//3:p[3]&=~1
    return products


with tempfile.TemporaryDirectory(prefix="axion-mineral-test-",dir=os.environ.get("TMPDIR","/var/tmp")) as directory:
    t=Path(directory);env=dict(os.environ,AXION_JOURNAL_DIR=str(t/"journal"))
    def run(*args,script=None,bad=False):
        cmd=args if args[0] in ("talk","ground") else ("floodplain",*args)
        p=subprocess.run([EXE,*map(str,cmd)],input=script,cwd=ROOT,env=env,capture_output=True,text=True,timeout=180)
        assert bool(p.returncode)==bad,(cmd,p.stdout[-600:],p.stderr[-500:]);return p.stdout
    def rows(output,name):
        return [dict(v.split("=",1) for v in l.split()) for l in output.splitlines() if l.startswith(name+"=")]
    a,b,c,d,e=(t/(name+".flood") for name in "abcde")
    run("--side",8,"--steps",0,"--save",e)
    assert snapshot(e)["params"][1]==3
    run("--load",e,"--materials","off","--steps",0,bad=True)
    run("--side",16,"--seed",189,"--density",60,"--grains",72,"--materials","reactive","--steps",0,"--save",a)
    run("--load",a,"--steps",47,"--save",b)
    log=run("--load",a,"--steps",119,"--report-every",17,"--material-legacy",23,"--artifact-samples",8,"--work-bench",262144,"--work-probes",8,
            "--work-subjects",65,"--quarry-moves",8,"--assay-calls","--save",c)
    run("--load",b,"--steps",72,"--report-every",11,"--save",d)
    assert c.read_bytes()==d.read_bytes()
    s=snapshot(c);assert s["version"]==11 and snapshot(a)["groups"]==s["groups"]
    top=rows(log,"assemblies")[-1]
    for k,v in topology(s).items():assert v==int(top[k]),(k,v,top[k])
    for audit in rows(log,"material_legacy"):
        assert replay(s,23,audit["material_legacy"])==(int(audit["reactions"]),int(audit["transfers"])),audit
    fingerprints=rows(log,"artifact_signature");assert fingerprints
    keys=("dark","a","b","both","retained","a_then_b","b_then_a")
    for specimen in fingerprints:
        measured=[port_response(s,specimen,j,0) for j in range(7)]
        assert measured==[int(specimen[k]) for k in keys],specimen
        assert not any(port_response(s,specimen,j,1) for j in range(7))
        rigid=[port_response(s,specimen,j,2) for j in range(7)]
        assert (measured!=rigid)==bool(int(specimen["rigid_change"]))
    assert rows(log,"construction_deployment")[0]["differences"]=="0"
    assert len(rows(log,"construction_probe"))==9
    run("--load",c,"--extend",32,"--steps",0,"--save",d)
    wide=snapshot(d)
    for y in range(16):assert wide["field"][32*y:32*y+16]==s["field"][16*y:16*y+16]
    assert wide["workers"][:256]==s["workers"]
    run("--load",c,"--soil",a,"--steps",0,"--save",d)
    donor=snapshot(d);assert donor["workers"]==s["workers"] and donor["field"]==snapshot(a)["field"]
    run("--load",c,"--hands","still","--steps",73,"--save",d);still=snapshot(d)
    assert still["accounts"][4:8]==s["accounts"][4:8]
    run("--load",c,"--bonds","insulated","--steps",75,"--save",d);cut=snapshot(d)
    assert cut["accounts"][1]==s["accounts"][1] and topology(cut)["bonds"]==0
    audit_water=run("--load",c,"--mineral-flow","coupled","--steps",0,"--hydraulic-trial",64,"--material-legacy",16,"--legacy",8,"--save",d)
    water=rows(audit_water,"mineral_hydraulics");assert len(water)==2 and water[0]["water"]==water[1]["water"]
    assert int(water[1]["water_sites"])>0 and int(water[1]["water_distance"])>0
    thick=snapshot(d);assert thick["params"][1]==3 and thick["workers"]==s["workers"] and thick["field"]==s["field"]
    run("--load",d,"--bonds","insulated","--steps",7,"--save",e);assert snapshot(e)["params"][1]==2
    run("--load",c,"--grains",3,"--steps",0,bad=True)
    run("--side",8,"--density",0,"--grains",0,"--steps",36,"--material-legacy",4,"--save",e)
    assert snapshot(e)["groups"]==[0,0,0]
    for i in (s["base"]+16,s["base"]+160+2):
        data=bytearray(s["raw"][:-8]);data[i]=255;e.write_bytes(data+digest(data));run("--load",e,"--steps",0,bad=True)
    resident=t/"builder.mind";x=run("--load",c,"--steps",0,"--export-mind",resident)
    old=int(rows(x,"field_extraction")[0]["field_extraction"])
    x=run("--resident",resident,"--side",8,"--density",0,"--steps",0,"--save",d)
    new=int(rows(x,"rehoused_resident")[0]["rehoused_resident"]);assert snapshot(d)["workers"][new]==s["workers"][old]
    child=t/"child.mind";peer=t/"peer.mind"
    reply=run("talk","--speaker",resident,"--listener",resident,"--save-speaker",child,"--save-listener",peer,
              script="/workbench 13 2\n/handle-scraps 64\n/peer-practice 64\n/show-work peer\n/try-work\n/sun 48\n/inspect-work\n/quit\n")
    assert len(rows(reply,"workbench_event"))==7 and child.exists() and peer.exists()
    x=run("--resident",child,"--side",8,"--density",0,"--steps",0,"--save",e)
    retained=snapshot(e)["workers"][int(rows(x,"rehoused_resident")[0]["rehoused_resident"])]
    assert retained[:24]==s["workers"][old][:24]
    run("--side",8,"--neurogenesis","fixed","--materials","reactive","--steps",3,"--save",e);assert snapshot(e)["version"]==9
    run("--side",8,"--neurogenesis","fixed","--pods","mixed","--materials","reactive","--steps",3,"--save",e);assert snapshot(e)["version"]==10
    run("--load",ROOT/"examples/shellbank.flood","--materials","reactive","--steps",5,"--save",e);assert snapshot(e)["version"]==12
    bench=ROOT/"examples/joint-reader.mind"
    text=run("talk","--speaker",bench,"--listener",bench,script=(
        "/workbench 319 2\n/point newt\n/handle-scraps 64\n/work-requests on\nnewt\nzzz\n/supply 191\n/take-grain\n/inspect-work\n/return-grain\n/inspect-work\n/quit\n"))
    named=[q for q in rows(text,"workbench_event") if q["workbench_event"]=="work-word"]
    assert len(named)==1 and int(named[0]["named_site"]) in (1,2) and text.count("work_word=unresolved")==1,text
    stocks=rows(text,"workpieces")
    assert int(stocks[-1]["supply"])+bool(int(stocks[-1]["carried_faces"]))>=32
    print("PASS independent grain chemistry, exact controls, arbitrary connected components, growth/restart, history, hand export and embodied interaction")
