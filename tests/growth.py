#!/usr/bin/env python3
"""Independently inspect living and dormant tissue, genetic birth size, conservation and checkpoint migration."""
import json
import gzip
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
EXE=Path(os.environ.get("AXION_BIN",ROOT/"axion")).resolve()


def digest(data):
    h=14695981039346656037
    for v in data:h=((h^v)*1099511628211)&0xffffffffffffffff
    return h.to_bytes(8,"little")


def unpack(path):
    raw=path.read_bytes()
    version,tick,side,tiles=struct.unpack_from("<4I",raw,8)
    assert version in (7,8) and raw[-8:]==digest(raw[:-8])
    n=side*side
    core=368+n*(24+4+172)
    bodies=[raw[core+96*i:core+96*(i+1)] for i in range(n)]
    start=core+n*96
    nerves=[raw[start+16*tiles*i:start+16*tiles*(i+1)] for i in range(n)]
    offset=start+n*(16*tiles+96)+(104+n*177 if version==8 else 0)
    open_,cut,initial,added,retired,born,dead,larger,smaller,spent=struct.unpack_from("<2I8Q",raw,offset)
    at=offset+72
    assert at+n*12+8==len(raw)
    cortices=[struct.unpack_from("<2I4B",raw,at+12*i) for i in range(n)]
    live=0
    for a,t,(_,_,active,natal,peak,least) in zip(bodies,nerves,cortices):
        assert 2<=least<=min(active,natal)<=max(active,natal)<=peak<=tiles
        assert not any(t[16*active:]),"inactive somatic tissue fired or retained state"
        if int.from_bytes(a[28:30],"little"):live+=32*active
    assert initial+born+added==live+retired+dead and 16*spent==added
    return dict(raw=raw,tick=tick,side=side,tiles=tiles,control=cut,base=initial,added=added,retired=retired,larger=larger,
                smaller=smaller,bodies=bodies,nerves=nerves,at=at,organs=cortices,live=live)


def row(text,key):
    return dict(v.split("=",1) for v in next(s for s in text.splitlines() if s.startswith(key+"=")).split())


with tempfile.TemporaryDirectory(prefix="axion-spiking-growth-",dir=os.environ.get("TMPDIR","/var/tmp")) as tmp:
    t=Path(tmp);env=dict(os.environ,AXION_JOURNAL_DIR=str(t/"journal"))
    def run(*args,bad=False,script=None):
        cmd=args if args[0] in ("agent","talk") else ("floodplain","--materials","off",*args)
        q=subprocess.run([EXE,*map(str,cmd)],input=script,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,
                         cwd=ROOT,env=env,timeout=120)
        assert bool(q.returncode)==bad,(cmd,q.stdout[-750:],q.stderr[-1050:])
        return q.stdout
    a,b,c,d=(t/(name+".flood") for name in "abcd")
    run("--side",16,"--density",64,"--neurons",512,"--seed",7933,"--steps",0,"--save",a)
    first=unpack(a)
    slot=next(i for i,(body,brain) in enumerate(zip(first["bodies"],first["organs"]))
              if int.from_bytes(body[28:30],"little")>=50 and brain[2]==2)
    # A real mutable two-bit developmental allele that requests a new band at age four.
    raw=bytearray(first["raw"][:-8]);struct.pack_into("<2I",raw,first["at"]+12*slot,0x55555555,0)
    a.write_bytes(raw+digest(raw))
    run("--load",a,"--steps",5,"--save",b)
    grown=unpack(b)
    assert grown["organs"][slot][2:]==(3,2,3,2) and any(grown["nerves"][slot][32:48])
    run("--load",a,"--neurogenesis","fixed","--steps",5,"--save",c)
    fixed=unpack(c)
    assert fixed["organs"][slot][2:]==(2,2,2,2) and fixed["added"]==fixed["retired"]==0
    mind=t/"grown.mind";run("--load",b,"--slot",slot,"--export-mind",mind,"--steps",0)
    text=run("agent","--load",mind,"--inspect")
    assert "developed_cells=96" in text,text
    housed=run("--side",8,"--density",0,"--resident",mind,"--steps",0,"--save",c)
    to=int(row(housed,"rehoused_resident")["rehoused_resident"]);resident=unpack(c)
    assert resident["organs"][to]==grown["organs"][slot] and resident["nerves"][to]==grown["nerves"][slot]
    run("--resident",mind,"--side",8,"--density",0,"--neurons",1024,"--steps",0,"--save",c)
    wider=unpack(c)
    assert wider["tiles"]==32 and wider["organs"][to]==resident["organs"][to]
    colony=t/"paired.flood"
    quiet=run("--side",16,"--density",92,"--neurons",512,"--colonists",c,"--competitors",c,
              "--silence-new-cohort","b","--steps",24,"--save",colony)
    counts=unpack(colony);census=[dict(v.split("=") for v in line.split()) for line in quiet.splitlines() if line.startswith("spiking_growth=")][-1]
    silenced=sum(32*max(0,organ[2]-organ[3]) for body,organ in zip(counts["bodies"],counts["organs"])
                 if body[28:30]!=bytes(2) and body[78:80]==b"\x02\0")
    assert counts["control"]==8 and int(census["silent_cells"])==silenced>0
    assert counts["live"]==int(census["expressed"])+silenced
    spoken=t/"spoken.mind"
    run("talk","--speaker",mind,"--save-speaker",spoken,script="/quit\n")
    run("--side",8,"--density",0,"--resident",spoken,"--steps",0,"--save",c)
    assert unpack(c)["organs"][to]==grown["organs"][slot]
    trained=t/"stimulated.mind"
    run("agent","--load",mind,"--save",trained,script="step 7 0 0\n")
    run("--side",8,"--density",0,"--resident",trained,"--steps",0,"--save",c)
    assert unpack(c)["organs"][to]==grown["organs"][slot]
    run("--load",b,"--neurons",1024,"--steps",0,"--save",c)
    wide=unpack(c)
    assert wide["organs"]==grown["organs"] and wide["live"]==grown["live"]
    assert all(new[:len(old)]==old for old,new in zip(grown["nerves"],wide["nerves"]))
    run("--load",c,"--neurons",256,"--steps",0,bad=True)
    run("--load",b,"--extend",32,"--steps",0,"--save",c)
    assert unpack(c)["organs"][:256]==grown["organs"]
    run("--load",a,"--steps",53,"--report-every",7,"--save",b)
    run("--load",b,"--steps",91,"--save",c)
    text=run("--load",a,"--steps",144,"--report-every",31,"--assay-growth","--assay-calls","--save",d)
    assert c.read_bytes()==d.read_bytes()
    final=unpack(d);audit=row(text,"growth_replay")
    assert final["added"]>0 and final["retired"]>0 and int(audit["recruited_spikes"])>0
    report=next(json.load(gzip.open(f)) for f in (t/"journal").glob("runs/*/observations.json.gz") if str(d) in json.load(gzip.open(f))["args"])
    for specimen in (p["measures"] for p in report["specimens"] if p["role"]=="morphospecies"):
        _,_,active,natal,peak,least=final["organs"][specimen["slot"]]
        assert [specimen[k] for k in ("active_cells","natal_cells","peak_cells","least_cells")]==[32*v for v in (active,natal,peak,least)]
    corrupt=bytearray(final["raw"][:-8]);corrupt[final["at"]+8]=1
    c.write_bytes(corrupt+digest(corrupt));run("--load",c,"--steps",0,bad=True)
    run("--side",8,"--neurogenesis","fixed","--steps",0,"--save",a)
    run("--load",a,"--neurogenesis","inherited","--neurons",256,"--pods","mixed","--steps",99,"--save",c)
    assert unpack(c)["added"]>0
    print("PASS 64 to 96 living cells in one animal, pruning, mutable birth size, live motor replay, exact persistence, widened arenas and legacy adoption")
