#!/usr/bin/env python3
"""Probe physical speech bindings with independent scenes and both public entry points."""
import os
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
binary=Path(os.environ.get("AXION_BIN",root/"axion")).resolve()
source=root/"examples/marsh-reader.mind"
with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR","/var/tmp"),prefix="axion-ground-test-") as temp:
    work=Path(temp);env=dict(os.environ,AXION_JOURNAL_DIR=str(work/"journal"))
    def run(*args,lines=(),fail=False):
        p=subprocess.run([binary,*map(str,args)],cwd=root,env=env,input="\n".join(lines)+"\n",text=True,capture_output=True,timeout=120)
        assert bool(p.returncode)==fail,(args,p.stdout[-550:],p.stderr)
        return p.stdout
    names=["moth","vole","newt","wren","hare","seal","lark","toad"]
    prompts=[];expected=[]
    for v in range(8):
        a,b=(v+1)%8,v
        prompts+= [f"/tabletop {0xf8|a:02x}@8>8 {0xa8|b:02x}@12>12",
                     f"/point {names[a]}", f"who is west of {names[b]}?", f"who is {names[a]} west of?",
                     f"is {names[b]} west {names[a]}?", f"is {names[a]} west {names[b]}?"]
        expected+=["8",names[a],names[b],"no","yes"]
    saved=work/"reader.mind"
    result=run("ground","--load",source,"--lesson","-","--save",saved,lines=prompts)
    replies=[s.split("> ",1)[1] for s in result.splitlines() if s.startswith("animal> ")]
    assert replies==expected,(replies,expected)
    topic=["/tabletop 00@1>1 09@5>5","who is west of vole?","and vole?","/point vole","/quit"]
    assert [s for s in run("talk","--speaker",saved,lines=topic).splitlines() if s.startswith("animal> ")]==["animal> _","animal> moth","animal> moth","animal> 5"]
    # The same sound can refer to two objects. Keep both on the retina.
    ambiguous=run("ground","--load",source,"--lesson","-",lines=["/tabletop e8@1>1 08@3>3 01@7>7","/point moth","who is west of vole?"])
    assert "animal> 1 3" in ambiguous and "animal> moth" in ambiguous
    # Two different names are compatible with the pixels. A one-word
    # confirmation in the acquired language chooses the intended participant.
    trial=["/tabletop 00@0>0 02@2>2 01@7>7","who is west of vole?"]
    asked=run("ground","--load",source,"--lesson","-",lines=trial)
    inquiry=next(s.removeprefix("animal> ") for s in asked.splitlines() if s.startswith("animal> "))
    assert inquiry in ("moth?","newt?"),asked
    for feedback,want in [("yes",inquiry[:-1]),("no","newt" if inquiry=="moth?" else "moth")]:
        settled=run("ground","--load",source,"--lesson","-",lines=[*trial,feedback])
        assert f"animal> {want}" in settled.splitlines(),settled
    unknown=run("ground","--load",source,"--lesson","-",lines=["/tabletop","/point moth"])
    assert "animal> ?" in unknown
    run("ground","--load",source,"--lesson","-",lines=["/tabletop ff@- >3"],fail=True)
    # A native saved animal must reject shortened or modified learned tissue.
    corrupt=work/"bad.mind";blob=bytearray(saved.read_bytes());blob[-12]^=1;corrupt.write_bytes(blob)
    run("agent","--load",corrupt,"--inspect",fail=True)
    for pairing in ("observed","shuffled"):
        sample=run("ground","--dialect","shuffled","--pairing",pairing,"--worlds",8,"--episodes",512,"--probes",64)
        row=dict(p.split("=") for p in next(s for s in sample.splitlines() if s.startswith("scene_transfer=")).split())
        if pairing=="observed":assert int(row["nouns"])==64 and int(row["cloze"])>0
        else:assert int(row["nouns"])==0
    print("PASS novel surface forms, physical identity, both argument directions, prior-turn reference, pointing, uncertainty and persistence")
