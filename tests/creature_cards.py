#!/usr/bin/env python3
"""Check frozen specimen cards, chronological evidence, rebuild and running outside the source tree."""
import ctypes
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BIN=Path(os.environ.get("AXION_BIN",ROOT/"axion")).resolve()
def size(file):
    raw=file.read_bytes();f=ctypes.CDLL("libwebp.so.7").WebPGetInfo
    f.argtypes=[ctypes.c_char_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_int),ctypes.POINTER(ctypes.c_int)]
    w,h=ctypes.c_int(),ctypes.c_int();assert f(raw,len(raw),ctypes.byref(w),ctypes.byref(h))
    return w.value,h.value

with tempfile.TemporaryDirectory(prefix="axion-field-cards-",dir=os.environ.get("TMPDIR","/var/tmp")) as tmp:
    work=Path(tmp);root=work/"journal"
    def run(*args,text=""):
        p=subprocess.run([BIN,*map(str,args)],cwd=work,input=text,capture_output=True,text=True,timeout=120,
                         env=dict(os.environ,AXION_JOURNAL_DIR=str(root)))
        assert p.returncode==0,(p.stdout[-500:],p.stderr)
    def summaries():return [json.loads(x.read_text()) for x in sorted(root.glob("runs/*/summary.json"))]
    run("journal","rebuild");assert size(root/"journey.webp")==(3840,2160)
    assert json.loads((root/"capabilities.json").read_text())["records"]==0
    graduate=work/"graduate.mind"
    run("imitate","--load",ROOT/"examples/field-botanist.mind","--fresh","--lesson",ROOT/"examples/greenhouse-motions.lesson",
        "--origin","permissive","--generations",0,"--forecast","then mist again sense brush else plant","--verify-room",0,"--save",graduate)
    a,=summaries();saved=root/a["path"]/"creature-card.webp"
    assert size(saved)==(3840,2160) and saved.read_bytes()==(root/"creature-card.webp").read_bytes()
    assert a["card"]["evidence"]["forecast"] and a["card"]["series"]["points"]==[[0,0],[1,11]]
    assert a["card"]["subject"]["measures"]["neurons"]==256 and "Acoustic prototypes" in a["card"]["subject"]["senses"]
    original=saved.read_bytes();journey=(root/"journey.webp").read_bytes()
    run("talk","--speaker",graduate,"--listener",ROOT/"examples/apprentice.mind",text="/greenhouse 16 0\n/plot 0 0\n/perform 0 then mist warm\n/quit\n")
    a,b=summaries();assert saved.read_bytes()==original
    assert (root/b["path"]/"creature-card.webp").read_bytes()==(root/"creature-card.webp").read_bytes()
    assert (root/"journey.webp").read_bytes()!=journey and size(root/"journey.webp")==(3840,2160)
    assert a["card"]["breadth"]<=b["card"]["breadth"]
    assert all(e["run"]<=b["id"] for e in json.loads((root/"capabilities.json").read_text())["milestones"])
    md=(root/"JOURNAL.md").read_text()
    for path in ("journey.webp","creature-card.webp",a["path"]+"/creature-card.webp",b["path"]+"/creature-card.webp"):assert path in md
    # Recover derived artifacts using the original observations. Nothing about
    # an older creature may depend on tomorrow's mutation of its checkpoint.
    graduate.unlink();(root/"capabilities.json").write_text("{broken")
    (root/"journey.webp").unlink();saved.unlink();(root/"creature-card.webp").unlink()
    old=next(root.glob("runs/*/summary.json"))
    s=json.loads(old.read_text());s.pop("card",None);old.write_text(json.dumps(s))
    run("journal","rebuild");assert len(summaries())==2 and saved.read_bytes()==original and (root/"creature-card.webp").is_file()
    assert size(root/"journey.webp")==(3840,2160)
    run("floodplain","--side","8","--steps","32","--report-every","8")
    newest=summaries()[-1];perf=newest["performance"]
    assert perf["unit"]=="site-ticks/s" and abs(perf["rate"]*perf["seconds"]-32*64)<.01
    assert 0<perf["advance_share"]<=1 and perf["bytes"]==546 and perf["wall"]>=perf["seconds"]
    plate=root/newest["path"]/"plate.webp";card=root/newest["path"]/"creature-card.webp"
    assert size(plate)==(1500,1040) and size(card)==(3840,2160)
    reference=(plate.read_bytes(),card.read_bytes())
    summary=root/newest["path"]/"summary.json";newest.pop("performance");summary.write_text(json.dumps(newest))
    run("journal","rebuild")
    assert summaries()[-1]["performance"]==perf and (plate.read_bytes(),card.read_bytes())==reference
    assert size(root/"journey.webp")==(3840,2160)
    atlas=card.read_bytes();run("talk","--speaker",ROOT/"examples/marsh-reader.mind",text="/quit\n")
    assert (root/"creature-card.webp").read_bytes()==atlas,"a single interview replaced the population census"
    print("PASS 4K landscape journey, correctly scoped performance, measurement backfill and immutable field observations")
    print("PASS specimen snapshots, acquired growth, historical cutoffs, cumulative journey, damaged-cache recovery and immutable cards")
