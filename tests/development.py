#!/usr/bin/env python3
"""Black-box CUDA regression coverage for raw learning, inference and storage."""
import gzip
import importlib.util
import json
import os
import re
from collections import deque
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from types import SimpleNamespace

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("development_study", ROOT / "tools/development-study.py")
study = importlib.util.module_from_spec(spec)
spec.loader.exec_module(study)
BIN = os.environ.get("AXION_BIN", str(ROOT / "axion"))
MIND = ROOT / "examples/witness-pupil.mind"

def voice(state, action, genome):
    rule, inputs, larynx = genome
    width = 3 + (larynx & 3)
    def bit(k): return (state >> (k % width)) & 1
    resulting = 0
    for k in range(width):
        address = bit(k-1) + 2*bit(k) + 4*bit(k+1) + 8*((inputs >> (action*6+k)) & 1)
        bank = (larynx >> (16+k)) & 1
        resulting |= ((rule >> (16*bank+address)) & 1) << k
    heard = ((resulting & (larynx>>3)).bit_count()%2) + 2*((resulting & (larynx>>9)).bit_count()%2)
    return resulting, heard


def same_voice(a, b):
    # Independent equivalence test between two physical vocal organs.
    todo = deque([(0,0)])
    seen = {(0,0)}
    while todo:
        x,y = todo.popleft()
        for action in range(4):
            u,s = voice(x,action,a)
            v,t = voice(y,action,b)
            if s!=t: return False
            if (u,v) not in seen:
                seen.add((u,v))
                todo.append((u,v))
    return True


def voice_count(eggs):
    data=eggs.read_bytes()
    n, = struct.unpack_from("<I",data,12)
    assert len(data)==44+40*n+8
    unique=[]
    for j in range(n):
        g=struct.unpack_from("<10I",data,44+40*j)
        if g[9] and not any(same_voice(g[3:6],old) for old in unique):
            unique.append(g[3:6])
    return len(unique)


with tempfile.TemporaryDirectory(prefix="axion-development-test-") as directory:
    work = Path(directory)
    os.environ["AXION_JOURNAL_DIR"] = str(work / "journal")

    def run(*args, lines="", success=True):
        r = subprocess.run([BIN,*map(str,args)], cwd=ROOT, input=lines, text=True, capture_output=True)
        assert (r.returncode == 0) == success, (args,r.stdout[-1000:],r.stderr)
        return r.stdout if success else r.stderr

    study.interview(SimpleNamespace(mind=str(MIND), delay=8192))
    study.teach(SimpleNamespace(mind=str(MIND), seed=31091, save=str(work/"independent.mind")))

    # Persist an absent observer's memory across two real invocations.
    saved = work/"paused.mind"
    run("talk","--speaker",MIND,"--save-speaker",saved,lines="/home\ninside\nleft\noutside\nright\n/quit\n")
    answer = run("talk","--speaker",saved,lines="where will fox look?\nwhere is it?\nsupercalifragilistic\n/quit\n")
    assert [x for x in answer.splitlines() if x.startswith("animal>")] == ["animal> expects left","animal> hidden right","animal> ?"]
    assert "developmental_cells=24" in run("agent","--load",saved,"--inspect")

    common = ["develop","--load",MIND,"--origin","inherited","--generations","0","--worlds","2"]
    for arguments in (["--family","ring","--states","1"], ["--budget","1","--motors","16"], ["--family","network","--states","7","--sounds","4"]):
        trace = run(*common,*arguments,"--replicas","259","--ticks","33")
        assert "sequence_batch=private" in trace and "sequence_batch=shared" in trace
    assert "exact=0 " in run(*common,"--family","ring","--states","8","--capacity","1")
    assert "method=replay" in run(*common,"--method","replay","--family","rhythm","--states","3")
    colony = [*common,"--colony","32","--days","16","--steps","64"]
    assert "chorus_season=1" in run(*colony,"--seasons","2","--save-colony",work/"whole.eggs")
    run(*colony,"--seasons","1","--save-colony",work/"split.eggs")
    run(*common,"--resume-colony",work/"split.eggs","--seasons","1","--save-colony",work/"next.eggs")
    assert (work/"whole.eggs").read_bytes() == (work/"next.eggs").read_bytes()
    census = run(*common,"--resume-colony",work/"whole.eggs","--seasons","1")
    assert int(re.search(r"distinct_calls=(\d+)",census)[1]) == voice_count(work/"whole.eggs")

    # Invalid contacts must fail before GPU upload, even with a valid checksum.
    corrupt = bytearray(saved.read_bytes())
    struct.pack_into("<H",corrupt,len(corrupt)-3152+656,65535)
    checksum = 14695981039346656037
    for byte in corrupt[:-8]:
        checksum = ((checksum ^ byte)*1099511628211) & ((1<<64)-1)
    struct.pack_into("<Q",corrupt,len(corrupt)-8,checksum)
    bad = work/"bad.mind"
    bad.write_bytes(corrupt)
    assert "invalid developmental contact" in run("agent","--load",bad,"--inspect",success=False)
    assert "range" in run(*common,"--capacity","65",success=False)
    for entry in sorted((work/"journal/runs").iterdir()):
        summary = json.loads((entry/"summary.json").read_text())
        assert len(summary["narrative"]) == 3 and len(summary["bullets"]) == 2
        plate = (entry/"plate.webp").read_bytes()
        assert plate[:4] == b"RIFF" and plate[8:12] == b"WEBP"
        with gzip.open(entry/"observations.json.gz") as f:
            assert json.load(f)["command"] in ("develop","talk")
    print("PASS external learning, latent-state recurrence, save/resume, private GPU packing, living voices and WebP journal")
