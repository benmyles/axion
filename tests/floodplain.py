#!/usr/bin/env python3
"""Independently decode habitat stocks, ancestry, migrations and CUDA replay."""
import gzip
import json
from collections import defaultdict
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=Path(os.environ.get("AXION_BIN",ROOT/"axion")).resolve()
MASK=(1<<64)-1
def digest(buf):
    h=14695981039346656037
    for x in buf:h=((h^x)*1099511628211)&MASK
    return h

def read(path):
    b=path.read_bytes()
    assert b[:8]==b"AXFLOOD\0" and struct.unpack_from("<I",b,8)==(3,)
    assert digest(b[:-8])==int.from_bytes(b[-8:],"little")
    tick,side,tiles=struct.unpack_from("<3I",b,12);n=side*side
    baseline=struct.unpack_from("<4Q",b,72);sun,heat,births,deaths=struct.unpack_from("<4Q",b,104)
    start=368
    def array(stride):
        nonlocal start
        result=[b[start+i*stride:start+(i+1)*stride] for i in range(n)]
        start+=stride*n
        return result
    field=array(24);sites=[int.from_bytes(i,"little") for i in array(4)]
    dna=array(172);bodies=array(96);brain=array(tiles*16);ears=array(96)
    assert start+8==len(b) and sorted(sites)==list(range(n))
    water=clay=energy=alive=0
    for f,a in zip(field,(bodies[j] for j in sites)):
        w,grains,east,south,leaf,fruit,litter,_,_,we,ws,_=struct.unpack("<7H2B2HI",f)
        body,limbs,pores,eyes=struct.unpack_from("<4I",a)
        assert body&(1<<12) and body>>25==0 and (limbs|pores|eyes)&~body==0
        cells={12}
        while True:
            enlarged=cells|{j for j in range(25) if body&(1<<j) and any(abs(i%5-j%5)+abs(i//5-j//5)==1 for i in cells)}
            if cells==enlarged:break
            cells=enlarged
        assert len(cells)==body.bit_count()
        reserve,carry=struct.unpack_from("<2H",a,28)
        fibre,cohort=struct.unpack_from("<2H",a,76);assert cohort<=2
        water+=w;clay+=grains+east+south+carry;energy+=reserve+leaf+fruit+litter+we+ws+fibre;alive+=bool(reserve)
    assert (water,clay,energy+heat,alive+deaths)==(baseline[0],baseline[1],baseline[2]+sun,baseline[3]+births)
    return tick,side,field,sites,dna,bodies,brain,ears

with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-flood-test-") as tmp:
    t=Path(tmp);env=dict(os.environ,AXION_JOURNAL_DIR=str(t/"journal"))
    def run(*args,fail=False):
        proc=subprocess.run([BIN,"floodplain","--materials","off","--neurogenesis","fixed",*map(str,args)],cwd=ROOT,env=env,text=True,capture_output=True,timeout=120)
        assert bool(proc.returncode)==fail,(args,proc.stdout[-750:],proc.stderr)
        return proc.stdout
    def row(text,key):
        return dict(pair.split("=",1) for pair in [s for s in text.splitlines() if s.startswith(key+"=")][-1].split())
    start=t/"start.flood";mid=t/"mid.flood";final=t/"complete.flood";split=t/"split.flood";larger=t/"larger.flood"
    common=["--side",16,"--density",50,"--seed",8731,"--season",32]
    run(*common,"--steps",0,"--save",start)
    run("--load",start,"--steps",45,"--report-every",7,"--save",mid)
    transcript=run("--load",start,"--steps",141,"--report-every",37,"--assay-calls","--legacy",48,"--save",final)
    run("--load",mid,"--steps",96,"--report-every",64,"--save",split)
    assert final.read_bytes()==split.read_bytes(),"clock, readouts or interventions changed the trajectory"
    snap=read(final)
    raw=[json.load(gzip.open(f,"rt")) for f in (t/"journal"/"runs").glob("*/observations.json.gz")]
    report=next(q for q in raw if str(final) in q["args"])
    observed=sorted((p["measures"] for p in report["specimens"] if p["role"]=="morphospecies"),key=lambda q:q["rank"])
    members=defaultdict(list)
    for slot,a in enumerate(snap[5]):
        if int.from_bytes(a[28:30],"little"):members[struct.unpack_from("<4I",a)].append(slot)
    ranked=sorted(members,key=lambda key:(-len(members[key]),key))[:8]
    assert len(observed)==len(ranked) and report["metrics"]["ranked_morphs"]==len(members)
    assert report["metrics"]["ranked_residents"]==sum(map(len,members.values()))
    for position,(q,key) in enumerate(zip(observed,ranked),1):
        ids=members[key]
        assert tuple(q[k] for k in ("body_plan","legs","pores","eyes"))==key and q["rank"]==position
        assert q["members"]==len(ids) and q["slot"]==min(ids,key=lambda j:(int.from_bytes(snap[5][j][20:24],"little"),j))
        for k,nutrient in enumerate(("leaf","fruit","litter")):assert q[nutrient]==sum(struct.unpack_from("<I",snap[5][j],64+4*k)[0] for j in ids)

    assert int(row(transcript,"flood_tick")["recruits"])>0
    assert int(row(transcript,"flood_census")["distinct_bodies"])>1
    run("--load",final,"--extend",32,"--steps",0,"--save",larger)
    ext=read(larger);n=16*16
    assert ext[0]==snap[0] and ext[1]==32
    for k in range(4,8):assert ext[k][:n]==snap[k]
    for y in range(16):
        for x in range(16):assert (ext[2][x+32*y],ext[3][x+32*y])==(snap[2][x+16*y],snap[3][x+16*y])
    sham=run(*common,"--construction","closed","--steps",511,"--legacy",64)
    assert all(int(row(sham,"ecological_legacy")[k])==0 for k in ("water_sites","food_sites","stock_distance","water_distance"))
    fixed=run(*common,"--architecture","closed","--morphology","closed","--producers","fixed","--steps",256)
    assert all(int(row(fixed,"flood_tick")[k])==0 for k in ("shape_edits","tape_edits","module_edits","brain_changes","recruits","plant_mutations"))
    empty=run("--side",4,"--density",0,"--frontier",16,"--steps",129,"--legacy",32)
    assert int(row(empty,"flood_tick")["living"])==0 and "flood_frontier=" not in empty
    both=run(*common,"--colonists",start,"--competitors",final,"--steps",0)
    count=row(both,"flood_census");assert int(count["cohort_a"])>0 and count["cohort_a"]==count["cohort_b"] and count["reserve_a"]==count["reserve_b"]
    run("--load",final,"--remobilize","--steps",0,"--save",larger);flat=read(larger)
    assert flat[4:]==snap[4:] and all(struct.unpack_from("<2H",s,4)==(0,0) and struct.unpack_from("<2H",s,16)==(0,0) for s in flat[2])
    grown=run("--side",16,"--seed",1877,"--frontier",64,"--season",32,"--steps",1024,"--save",larger)
    assert "flood_frontier=" in grown and read(larger)[1]>16
    # A survivor can move through a native mind checkpoint into another live
    # habitat with its body, learned hearing, neural gates and gene tape intact.
    wild=t/"wild.mind";result=run("--load",final,"--steps",0,"--observer","--export-mind",wild)
    donor=int(row(result,"field_extraction")["field_extraction"])
    result=run("--resident",wild,"--density",0,"--side",8,"--steps",0,"--save",larger)
    slot=int(row(result,"rehoused_resident")["rehoused_resident"]);new=read(larger)
    for k in (4,6,7):assert new[k][slot]==snap[k][donor]
    assert new[5][slot][:16]==snap[5][donor][:16] and new[5][slot][20:78]==snap[5][donor][20:78]
    assert int(row(result,"flood_tick")["living"])==1
    res=subprocess.run([BIN,"talk","--speaker",wild,"--save-speaker",t/"spoken.mind"],env=env,cwd=ROOT,
                       input="/point ka\n/quit\n",text=True,capture_output=True)
    assert res.returncode==0 and "animal> " in res.stdout,(res.stderr,res.stdout)
    run("--resident",t/"spoken.mind","--density",0,"--side",8,"--steps",1)
    run(*common,"--senses-from",ROOT/"examples/marsh-reader.mind","--steps",16)
    bad=t/"invalid.flood";raw=bytearray(final.read_bytes());raw[-9]^=1;bad.write_bytes(raw)
    run("--load",bad,"--steps",0,fail=True)
    # With a valid checksum, an aliased occupant and an out-of-range genome are still invalid.
    raw=bytearray(final.read_bytes());pos=368+24*n;raw[pos:pos+4]=raw[pos+4:pos+8];raw[-8:]=digest(raw[:-8]).to_bytes(8,"little")
    bad.write_bytes(raw);run("--load",bad,"--steps",0,fail=True)
    run("--load",final,"--soil",larger,"--steps",0,fail=True)
    print("PASS independent conservation, connected anatomy, exact restart, soil and expansion, ablations, neutral cohorts and malformed habitats")
