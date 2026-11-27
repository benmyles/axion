#!/usr/bin/env python3
"""Public CUDA tests for relational induction, private perspectives and their compiler."""
import gzip
import importlib.util
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from types import SimpleNamespace

sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("composition_study",ROOT/"tools/composition-study.py")
study=importlib.util.module_from_spec(spec)
spec.loader.exec_module(study)
BIN=os.environ.get("AXION_BIN",str(ROOT/"axion"))
pupil=ROOT/"examples/courier.mind"

with tempfile.TemporaryDirectory(prefix="axion-composition-test-",dir=os.environ.get("TMPDIR","/var/tmp")) as folder:
    work=Path(folder)
    os.environ["AXION_JOURNAL_DIR"]=str(work/"journal")
    study.social(pupil)
    study.inverse(SimpleNamespace(mind=str(pupil),seed=9011))
    study.foreign(SimpleNamespace(mind=str(pupil),save=str(work/"bilingual.mind"),seed=9011,append=True))
    study.social(work/"bilingual.mind")
    study.exhaustive(pupil)

    def talk(mind,lines,save=None):
        args=("talk","--speaker",mind) if not save else ("talk","--speaker",mind,"--save-speaker",save)
        result=study.run(*args,text="\n".join([*lines,"/quit",""]))
        return [x.removeprefix("animal> ") for x in result.splitlines() if x.startswith("animal> ")]

    pending=work/"pending.mind"
    assert talk(pupil,["/new-scene","put acorn inside basket","put basket inside loft","where should mira search"],pending)[-1]=="what did mira choose?"
    recalled=work/"recalled.mind"
    assert talk(pending,["acorn"],recalled)==["try loft"]
    # Add a new predicate while keeping a live, previously reported choice.
    examples=[]
    for who,echo in (("birch","lime"),("gull","blue"),("moth","rose"),("birch","rose"),("gull","lime"),("moth","blue")):
        examples.extend((f"/example {who} hums {echo}",f"/example show {who} hue | {echo}"))
    result=talk(recalled,[*examples,"/learn","fox hums pearl","show fox hue","where should mira search"])
    assert result[-3:]==["_","pearl","try loft"],result
    # Repeated overwrites preserve an absent observer's pinned version.
    record=["/new-scene","put acorn inside nest","mira is awake","mira is asleep"]
    record += [f"put acorn inside {('loft','den')[i%2]}" for i in range(4096)]
    result=talk(pupil,[*record,"where will mira look for acorn","where is acorn"])
    assert result[-2:]==["mira expects nest","acorn rests at den"]

    catalog=work/"catalog.txt"
    catalog.write_text("\n".join(["put acorn inside basket","put basket inside loft","put basket inside den","mira is awake","mira is asleep",
                                  "where is acorn","where will mira look for acorn","and its container"]))
    report=study.run("compose","--load",pupil,"--catalog",catalog,"--stream","-","--save",work/"from-packed.mind",text="\n".join([
        "put acorn inside basket","put basket inside loft","mira is asleep","put basket inside den","put basket inside mira",
        "where will mira look for acorn","where is acorn","and its container","/quit",""]))
    assert [x.removeprefix("animal> ") for x in report.splitlines() if x.startswith("animal> ")][-4:]==[
        "[outside catalog]","mira expects loft","acorn rests at den","because acorn rides basket"]
    assert talk(work/"from-packed.mind",["where will mira look for acorn","where is acorn"])==["mira expects loft","acorn rests at den"]
    botanist=ROOT/"examples/field-botanist.mind"
    study.social(botanist)
    integrated=["/greenhouse 16 0","/new-scene","mira chooses acorn","put acorn inside basket","put basket inside pearl",
                "mira seeks fruiting","guide mira.","why soak?","proceed.","edible pearl?"]
    active=talk(botanist,integrated)
    assert active[-6:]==["how can pearl become fruiting","heat sow soak tend pollinate ripen",
        "skipping soak leaves pearl dry blocking fruiting","make it","heat sow soak tend pollinate ripen","yes"],active
    gated=talk(botanist,["/self-hear off",*integrated[:-4],"guide mira.","proceed.","edible pearl?"])
    assert gated[-3:]==["how can pearl become fruiting","make it","no"],gated
    common=["compose","--load",pupil,"--origin","inherited","--generations",0]
    timed=study.run(*common,"--worlds",3,"--dialect","permuted","--replicas",259,"--ticks",67,"--controls",0,"--census")
    assert "binding_census=65536  exact=65536" in timed and "answers_after_update=16777216" in timed and "composition_batch=packed" in timed
    costs=[]
    for policy in ("disagreement","yoked"):
        r=study.run(*common,"--worlds",4,"--inquiries",4,"--attention",policy,"--trials",64,"--controls",0)
        costs.append(int(re.search(r"requested=(\d+)",r)[1]))
    assert costs[0]==costs[1] and costs[0]>0
    colony=[*common,"--society",32,"--days",16,"--dialect","permuted"]
    study.run(*colony,"--seasons",2,"--save-nest",work/"whole.nest")
    study.run(*colony,"--seasons",1,"--save-nest",work/"split.nest")
    study.run("compose","--resume-nest",work/"split.nest","--seasons",1,"--save-nest",work/"resumed.nest")
    assert (work/"whole.nest").read_bytes()==(work/"resumed.nest").read_bytes()
    bad=bytearray(pending.read_bytes())
    struct.pack_into("<I",bad,len(bad)-12,9)
    hash_value=14695981039346656037
    for b in bad[:-8]:hash_value=((hash_value^b)*1099511628211)&((1<<64)-1)
    struct.pack_into("<Q",bad,len(bad)-8,hash_value)
    (work/"bad.mind").write_bytes(bad)
    failed=subprocess.run([BIN,"agent","--load",work/"bad.mind","--inspect"],cwd=ROOT,capture_output=True,text=True)
    assert failed.returncode and "section length" in failed.stderr
    for d in (work/"journal/runs").iterdir():
        s=json.loads((d/"summary.json").read_text())
        assert len(s["narrative"])==3 and len(s["bullets"])>=2
        plate=(d/"plate.webp").read_bytes();assert plate[:4]==b"RIFF" and plate[8:12]==b"WEBP"
        with gzip.open(d/"observations.json.gz") as f:assert json.load(f)["command"] in ("compose","talk")
    print("PASS role induction, retained languages, self-initiated questions, false belief, every eight-object arrangement, packed streams and brood continuation")
