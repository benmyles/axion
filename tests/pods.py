#!/usr/bin/env python3
"""Decode the physical and neural checkpoint independently and exercise the native GPU interventions."""
import gzip
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
EXE = Path(os.environ.get("AXION_BIN", ROOT/"axion")).resolve()


def hash64(data):
    result = 14695981039346656037
    for byte in data:
        result = ((result ^ byte) * 1099511628211) & ((1 << 64)-1)
    return result.to_bytes(8, "little")


def decode(path):
    buf = path.read_bytes()
    assert buf[:8] == b"AXFLOOD\0" and buf[-8:] == hash64(buf[:-8])
    version, tick, side, tiles = struct.unpack_from("<4I", buf, 8)
    assert version == 6
    n = side*side
    pos = 368

    def blocks(size):
        nonlocal pos
        values = [buf[i:i+size] for i in range(pos, pos+n*size, size)]
        pos += size*n
        return values

    land, slots, genes, body, tissue, ears = [blocks(k) for k in (24, 4, 172, 96, 16*tiles, 96)]
    assert sorted(int.from_bytes(i, "little") for i in slots) == list(range(n))
    start = pos
    entry = struct.unpack_from("<I", buf, pos+24)[0]
    swallowed, returned, discarded = struct.unpack_from("<3Q", buf, pos+48)
    pos += 104
    organs = blocks(176)
    winding = blocks(1)
    assert pos+8 == len(buf) and all(w[0]<4 for w in winding)
    water = clay = food = living = pending = 0
    for i,s in enumerate(land):
        slot = int.from_bytes(slots[i], "little")
        a, animal = body[slot], organs[slot]
        w, grains, east, south, leaf, fruit, litter, _, _, we, ws, _ = struct.unpack("<7H2B2HI", s)
        reserve, load = struct.unpack_from("<2H", a, 28)
        fibre = struct.unpack_from("<H", a, 76)[0]
        gut = sum(animal[104:136])
        assert gut <= 120 and (reserve or not gut)
        assert all(-63<=v<=63 for v in struct.unpack_from("16b",animal,80))
        assert animal[169]<=3
        water += w
        clay += grains+east+south+load
        food += reserve+leaf+fruit+litter+we+ws+fibre+gut
        living += bool(reserve)
        pending += gut
    water0, clay0, food0, founders, sun, heat, births, deaths = struct.unpack_from("<8Q", buf, 72)
    assert water==water0 and clay==clay0 and food+heat==food0+sun and living+deaths==founders+births
    assert entry+swallowed==returned+discarded+pending
    return dict(buf=buf, start=start, tick=tick, side=side, organs=organs, body=body, land=land,
                genes=genes, tissue=tissue, ears=ears, winding=winding, pending=pending)


def rows(text, key):
    return [dict(token.split("=",1) for token in row.split()) for row in text.splitlines() if row.startswith(key+"=")]


with tempfile.TemporaryDirectory(prefix="axion-gut-test-",dir=os.environ.get("TMPDIR","/var/tmp")) as tmp:
    t=Path(tmp)
    env=dict(os.environ, AXION_JOURNAL_DIR=str(t/"journal"))

    def run(*args, bad=False, script=None):
        command = args if args[0] in ("talk","ground") else ("floodplain","--materials","off","--neurogenesis","fixed",*args)
        process = subprocess.run([EXE,*map(str,command)],cwd=ROOT,env=env,input=script,
                                 capture_output=True,text=True,timeout=120)
        assert bool(process.returncode)==bad,(args,process.stdout[-850:],process.stderr[-1000:])
        return process.stdout

    small=["--side",16,"--density",64,"--seed",8791,"--season",32]
    start, middle, end, split, output, bad = (t/(name+".flood") for name in ("start","middle","end","split","output","bad"))
    run(*small,"--pod-origin","reference","--pods","sealed","--steps",0,"--save",start)
    run("--load",start,"--steps",45,"--save",middle)
    transcript=run("--load",start,"--steps",141,"--report-every",37,"--assay-calls","--pod-assay",192,
                   "--pod-remap",384,"--pod-subjects",65,"--pod-bench",65536,"--save",end)
    run("--load",middle,"--steps",96,"--report-every",15,"--save",split)
    assert end.read_bytes()==split.read_bytes()
    snapshot=decode(end)
    assert snapshot["pending"]>0 and any(x[0] for x in snapshot["winding"])
    assert int(rows(transcript,"pod_tick")[-1]["synaptic_edits"])>0
    assert rows(transcript,"pod_deployment")[0]["differences"]=="0"
    assert int(rows(transcript,"pod_transfers")[1]["practiced"])>int(rows(transcript,"pod_transfers")[1]["erased"])
    assert len(rows(transcript,"pod_remap"))==2
    obs=next(json.load(gzip.open(f)) for f in (t/"journal").glob("runs/*/observations.json.gz") if str(end) in json.load(gzip.open(f))["args"])
    for card in (s["measures"] for s in obs["specimens"] if s["role"]=="morphospecies"):
        p=snapshot["organs"][card["slot"]]
        occupied=sum(((x|(x>>1))&0x55).bit_count() for x in p[16:80])
        assert occupied==card["pod_contacts"] and sum(p[104:136])==card["crop_mass"]
        assert p[170].bit_count()==card["known_surfaces"]
        assert (p[4]).bit_count()==card["lag_banks"]

    run("--load",end,"--extend",32,"--steps",0,"--save",output)
    extended=decode(output)
    assert extended["organs"][:256]==snapshot["organs"]
    for y in range(16):assert extended["winding"][32*y:32*y+16]==snapshot["winding"][16*y:16*y+16]
    run("--load",end,"--soil",start,"--steps",0,"--save",output)
    garden=decode(output)
    assert garden["organs"]==snapshot["organs"] and garden["winding"]==decode(start)["winding"]
    run("--load",end,"--erase-pods","--steps",0,"--save",output)
    cleared=decode(output)
    assert all(p[:16]==q[:16] and p[16:104]==bytes(88) and p[104:148]==q[104:148] and p[148:]==bytes(28)
               for p,q in zip(cleared["organs"],snapshot["organs"]))
    run("--load",start,"--pod-credit","frozen","--steps",81,"--save",output)
    frozen=decode(output)
    assert all(p[16:96]==bytes(80) and p[170]==0 for p in frozen["organs"])
    run(*small,"--steps",0,"--save",middle) # introduce new tissue into a legacy, occupied habitat
    run("--load",middle,"--pods","mixed","--steps",25,"--save",output)
    decode(output)
    run("--side",4,"--density",0,"--pods","sealed","--raft-seasons",1,"--pod-assay",8,"--pod-bench",32,"--steps",0,"--save",output)
    assert not any(int.from_bytes(a[28:30],"little") for a in decode(output)["body"])

    for version in (4,5):
        source=snapshot["buf"];at=snapshot["start"]
        old=bytearray(source[:at]);struct.pack_into("<I",old,8,version)
        pod=bytearray(source[at:at+(96 if version==4 else 104)])
        struct.pack_into("<I",pod,16,0 if version==4 else 2)
        old+=pod+b"".join(p[:136] for p in snapshot["organs"])+b"".join(snapshot["winding"])
        bad.write_bytes(old+hash64(old))
        run("--load",bad,"--steps",0,"--save",output)
        restored=decode(output)
        assert restored["organs"]==[p[:136]+bytes(40) for p in snapshot["organs"]]
    for offset,value in [(snapshot["start"]+104+169,4),(snapshot["start"]+104+80,127),(len(snapshot["buf"])-9,8)]:
        corrupt=bytearray(snapshot["buf"][:-8]);corrupt[offset]=value
        bad.write_bytes(corrupt+hash64(corrupt));run("--load",bad,"--steps",0,bad=True)

    donor=t/"resident.mind";res=run("--load",end,"--steps",0,"--export-mind",donor)
    old=int(rows(res,"field_extraction")[0]["field_extraction"])
    res=run("--resident",donor,"--side",8,"--density",0,"--steps",0,"--save",output)
    new=int(rows(res,"rehoused_resident")[0]["rehoused_resident"])
    assert decode(output)["organs"][new]==snapshot["organs"][old]
    speech=t/"speaking.mind"
    run("ground","--load",donor,"--origin","permissive","--worlds",1,"--episodes",1024,"--crowd",4,
        "--probes",16,"--dialect","english","--save",speech)
    after=t/"fed.mind"
    text=run("talk","--speaker",speech,"--save-speaker",after,script=(
        "/tidalpool 73\n/ripen 128\n/tabletop 00@0>0 09@3>3\n/grip-word moth\n/grip-word vole\n"
        "/grip-word zun\n/tabletop 00@0>0 00@4>4\n/grip-word moth\n/turn 1\n/offer 0 24\n/quit\n"))
    probes=rows(text,"tray_grip")
    assert len(probes)==4 and [p["site"] for p in probes[1:3]]==["0","3"] and text.count("grip=unresolved")==2,text
    run("--resident",after,"--side",8,"--density",0,"--steps",1,"--save",output)
    assert decode(output)["tick"]==snapshot["tick"]+128+24+1
    print("PASS native digestion, restart, temporal and motor interventions, full input-space comparison, real species, old formats, and word-guided grasping")
