#!/usr/bin/env python3
"""Exercise journal transactions through the public executable and its real CUDA habitats."""
import concurrent.futures
import ctypes
import gzip
import hashlib
import json
from itertools import combinations
import os
from pathlib import Path
import runpy
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BINARY = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()


def verify_webp(path):
    data = path.read_bytes()
    assert data[:4] == b"RIFF" and data[8:12] == b"WEBP"
    decoder = ctypes.CDLL("libwebp.so.7").WebPGetInfo
    width, height = ctypes.c_int(), ctypes.c_int()
    decoder.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    assert decoder(data, len(data), ctypes.byref(width), ctypes.byref(height)) == 1
    assert (width.value, height.value) == (1500, 1040)


def exercise(work):
    journal = work / "journal"
    env = dict(os.environ, AXION_JOURNAL_DIR=str(journal))

    def run(*args, lesson="", success=True):
        result = subprocess.run([BINARY, *map(str, args)], cwd=ROOT, input=lesson, text=True,
                                env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert (result.returncode == 0) == success, (args, result.stderr)
        return result.stdout

    def entries():
        return [json.loads(p.read_bytes()) for p in sorted(journal.glob("runs/*/summary.json"))]

    def observations(row):
        with gzip.open(journal / row["path"] / "observations.json.gz") as source:
            return json.load(source)

    def conjunctive_contacts(path):
        data=path.read_bytes();layout=int.from_bytes(data[8:12],"little")
        if data[-16:-12]==(0x53595048).to_bytes(4,"little"):
            extra=int.from_bytes(data[-12:-8],"little");data=data[:-8-extra]+data[-8:]
        if data[-20:-16]==(0x43574f52).to_bytes(4,"little"):
            extra=int.from_bytes(data[-16:-12],"little");data=data[:-8-extra]+data[-8:]
        if data[-16:-12]==(0x434c4159).to_bytes(4,"little"):
            extra=int.from_bytes(data[-12:-8],"little");data=data[:-8-extra]+data[-8:]
        if data[-16:-12]==(0x42524e45).to_bytes(4,"little"):
            extra=int.from_bytes(data[-12:-8],"little");data=data[:-8-extra]+data[-8:]
        if data[-16:-12]==(0x51464c44).to_bytes(4,"little"):
            extent=int.from_bytes(data[-12:-8],"little");data=data[:-8-extent]+data[-8:]
        composition=int.from_bytes(data[-12:-8],"little") if layout&4194304 else 12440 if layout&2097152 else 0
        back=8+3144*bool(layout&1048576)+composition
        procedure=int.from_bytes(data[-back-4:-back],"little") if layout&8388608 else 0
        back+=procedure
        ground=int.from_bytes(data[-back-4:-back],"little") if layout&16777216 else 0
        end=len(data)-back-ground-24*bool(layout&2147483648)-16*bool(layout&1073741824)-72*bool(layout&536870912)-12*bool(layout&268435456)-(176 if layout&134217728 else 136)*bool(layout&67108864)-(int.from_bytes(data[-back-ground-4:-back-ground],"little") if layout&33554432 else 0)-(744 if layout&16384 else 0)-(1576 if layout&32768 else 0)-(1376 if layout&65536 else 0)-(32 if layout&131072 else 0)-(368 if layout&262144 else 0)-(272 if layout&524288 else 0)
        return data[end-128:end]

    run("life", "--help")
    run("agent", "--load", "examples/wayfarer.mind", "--inspect")
    run("life", "--steps", "bad-number", success=False)
    assert entries() == []

    base = ["life", "--load", "examples/estuary.eco"]
    run(*base, "--steps", 31, "--report-every", 5, "--save", work / "whole.eco")
    first, = entries()
    data = observations(first)
    assert data["recorded_at"] == first["date"]
    assert data["points"][0][0] == 0 and data["points"][-1][0] == 31
    assert sum(tile[1] for tile in data["tiles"]) == first["alive"]
    assert len(data["frames"][-1]["residents"]) == first["alive"]
    last = data["points"][-1]
    assert last[3] == data["points"][0][3] + first["activity"]["births"] - data["metrics"]["interval_deaths"]
    assert first["activity"]["moves"] == last[8] - data["points"][0][8]
    original = journal / first["path"] / "plate.webp"
    original_hash = hashlib.sha256(original.read_bytes()).digest()
    verify_webp(original)
    run(*base, "--steps", 11, "--save", work / "part.eco")
    shutil.copyfile(work / "part.eco", work / "relocated.eco")
    run("life", "--load", work / "relocated.eco", "--steps", 20, "--save", work / "resumed.eco")
    assert (work / "resumed.eco").read_bytes() == (work / "whole.eco").read_bytes()
    assert [(p["id"],p["path"]) for p in entries()[-1]["parents"]] == [(2,entries()[1]["path"])]
    assert first["checkpoints"][0]["checksum"] == f"{int.from_bytes((ROOT/'examples/estuary.eco').read_bytes()[-8:],'little'):016x}"

    command = ("agent", "--load", "examples/wayfarer.mind")
    def concurrent_witness(i):
        stamp=work/f"parallel-{i}.mind"
        result=subprocess.run([BINARY,*command,"--save",str(stamp)],cwd=ROOT,env=env,text=True,capture_output=True,check=True,
                              input="travel 1 0 0\n# phase Held-out doorway\norient 1 0 0 1\n")
        receipts=[line for line in result.stderr.splitlines() if line.startswith("Run ")]
        assert len(receipts)==1,receipts
        plate=Path(receipts[0].split(" · ",1)[1])
        with gzip.open(plate.parent/"observations.json.gz") as f:evidence=json.load(f)
        assert str(stamp) in evidence["args"]
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        list(pool.map(concurrent_witness, range(3)))
    records = entries()
    assert [r["id"] for r in records] == list(range(1, 7))
    for r in records[-3:]:
        stream = observations(r)
        assert stream["metrics"]["events"] == 2 and sum(stream["motors"]) == 2
        assert sum(n for _, n in stream["journeys"]) == 1
        assert r["scores"][-1][0] == "Held-out doorway" and r["scores"][-1][2] == 1

    run("life", "--founders", "examples/wayfarers.eco", "--habitat", "thicket", "--metabolism", "food-fuel",
        "--navigation", "competitive", "--relations", "contexts", "--programs", 2, "--polarity", "evolvable",
        "--synapse-bits", 2, "--side", 8, "--steps", 0, "--nursery", 3, "--nursery-problem", "navigation",
        "--breeding", "map", "--crossings", 1)
    training = observations(entries()[-1])["breeding"]
    assert [p[0] for p in training] == [1, 2, 3]
    assert all(0 <= total <= count * best for _, count, total, best in training)

    archive = work / "earlier"
    (archive / "docs").mkdir(parents=True)
    (archive / "docs" / "study.csv").write_text("seed,correct,items\n4,6,8\n4,5,8\n7,1,4\n")
    run("journal", "import", archive)
    run("journal", "import", archive)
    records = entries()
    assert len(records) == 9 and records[-2]["scores"] == [["Correct choices", 11, 16]]
    assert all(len(r["narrative"]) == 3 and len(r["bullets"]) >= 2 for r in records)
    latest_run=next(r for r in reversed(records) if not r["archive"])
    assert (journal / "latest.webp").read_bytes() == (journal / latest_run["path"] / "plate.webp").read_bytes()
    assert (journal / "JOURNAL.md").read_text().count("## ") == len(records)
    verify_webp(journal / "progress.webp")
    (journal / "progress.webp").unlink()
    run("journal", "rebuild")
    verify_webp(journal / "progress.webp")
    assert hashlib.sha256(original.read_bytes()).digest() == original_hash
    assert not list(journal.glob(".pending-*"))
    offline = subprocess.run([BINARY, *base, "--steps", "1"], cwd=ROOT, env=dict(env, PATH=str(work / "empty")),
                             text=True, capture_output=True)
    assert offline.returncode and "journal rebuild" in offline.stderr and len(list(journal.glob(".receipt-*"))) == 1
    run("journal", "rebuild")
    run("journal", "rebuild")
    assert len(entries()) == 10 and not list(journal.glob(".receipt-*"))
    run("agent", "--load", "examples/tracker.mind", lesson="# phase Hidden junction\norient 1 8 3\n")
    animal=entries()[-1]
    assert observations(animal)["conditional"] and animal["scores"][-1]==["Hidden junction",1,1]
    verify_webp(journal / animal["path"] / "plate.webp")
    run("life", "--load", "examples/reedlings.eco", "--steps", 0, "--migration", 2,
        "--rooms", 32, "--spring-vision", "shared", "--exposures", 2, "--locomotion", "foot")
    field=entries()[-1]
    assert field["kind"]=="forage" and field["stats"][0][1]==8
    assert any("depart_spring=" in line for line in observations(field)["lines"])
    verify_webp(journal / field["path"] / "plate.webp")
    # Pair zero must follow exactly the original actor in a larger swarm.
    pair=["dialogue","--speaker","examples/tracker.mind","--listener","examples/scout.mind",
          "--practice",11,"--probes",32,"--seed",19139,"--reversal","off"]
    for n,schedule in ((1,"direct"),(33,"graph")):
        run(*pair,"--pairs",n,"--dispatch",schedule,"--save-speaker",work/f"s{n}.mind","--save-listener",work/f"l{n}.mind")
    for role in ("s","l"):
        assert (work/f"{role}1.mind").read_bytes()==(work/f"{role}33.mind").read_bytes()
    assert entries()[-1]["kind"]=="conversation"
    run(*pair,"--pairs",3,"--controls","off")
    assert entries()[-1]["kind"]=="conversation" and len(entries()[-1]["narrative"])==3
    # Every query is scored by the test process. Locations change between turns.
    first_turn="/scene west north\nWhere is the food today?\n"
    second_turn="/scene east south\nThe other one for me, please?\n"
    def uttered(log):return [s[8:] for s in log.splitlines() if s.startswith("animal> ")]
    assert uttered(run("talk","--save-speaker",work/"whole-speaker.mind","--save-listener",work/"whole-asker.mind",
                       lesson=first_turn+second_turn+"/quit\n"))==["west","south"]
    run("talk","--save-speaker",work/"half-speaker.mind","--save-listener",work/"half-asker.mind",lesson=first_turn+"/quit\n")
    assert uttered(run("talk","--speaker",work/"half-speaker.mind","--listener",work/"half-asker.mind",
                       "--save-speaker",work/"end-speaker.mind","--save-listener",work/"end-asker.mind",lesson=second_turn+"/quit\n"))==["south"]
    for role in ("speaker","asker"):
        assert (work/f"whole-{role}.mind").read_bytes()==(work/f"end-{role}.mind").read_bytes()
    assert uttered(run("talk","--save-listener",work/"new-word.mind",
                       lesson="/scene west north\nWhat is a brook?\n/teach brook 2\nWhere is the brook?\n/teach brook 1\nThe brook now please?\n/quit\n"))==["(silence)","north","west"]
    assert uttered(run("talk","--listener",work/"new-word.mind",lesson="/scene east south\nThe brook now?\nWhat about the orchestra?\n/quit\n"))==["east","(silence)"]
    transcript=entries()[-1]
    assert transcript["kind"]=="interview" and observations(transcript)["metrics"]["human_turns"]==2
    verify_webp(journal / transcript["path"] / "plate.webp")
    # The external scene is deliberately inconsistent with the remembered location.
    prefix="/scene west north\nWhere is the food?\n"
    covered="/cover 1\n/scene east south\nWhere is the food?\nThe other?\nThe other?\n"
    assert uttered(run("talk","--save-speaker",work/"intact.mind",lesson=prefix+covered))==["west","west","south","west"]
    record=observations(entries()[-1])
    assert [p&255 for p in record["perceptions"]]==[128,129,65,129]
    run("talk","--save-speaker",work/"before.mind","--save-listener",work/"before-asker.mind",lesson=prefix)
    assert uttered(run("talk","--speaker",work/"before.mind","--listener",work/"before-asker.mind",
                       "--save-speaker",work/"resumed.mind",lesson=covered))==["west","south","west"]
    assert (work/"intact.mind").read_bytes()==(work/"resumed.mind").read_bytes()
    assert uttered(run("talk","--recall-cut","fixed",lesson=prefix+covered))==["west","(silence)","south","(silence)"]
    assert uttered(run("talk","--speaker","examples/discourse-speaker.mind",lesson=prefix+covered))==["west","(silence)","south","(silence)"]
    run("recollect","--load","examples/witness.mind","--generations",0,"--probe-worlds",8,"--cycles",4,"--delay",11,
        "--save",work/"audit.mind")
    assert (work/"audit.mind").read_bytes()==(ROOT/"examples/witness.mind").read_bytes()
    report=entries()[-1]
    assert report["kind"]=="recollection" and len(report["narrative"])==3 and len(report["bullets"])==2
    assert [s[1:] for s in observations(report)["scores"]]==[[32,32]]*5
    claims=[s for s in observations(report)["lines"] if s.startswith("memory_proof=")]
    assert len(claims)==4 and all("violations=0" in s for s in claims)
    verify_webp(journal / report["path"] / "plate.webp")
    counterexample=run("recollect","--load","examples/fixed-interval.mind","--generations",0,"--probe-worlds",3,"--cycles",2)
    assert any("first_depth=2 " in s for s in counterexample.splitlines() if s.startswith("memory_proof=clear"))
    physical="/scene west north\nWhere is the food?\nWhere is the water?\n/scene east south\n"
    first=physical+"Where was the food?\n"
    rest="/scene east south\nThe other, keep?\nSwitch water?\n/teach-focus yonder remember\nYonder food?\n"
    assert uttered(run("talk","--save-listener",work/"complete-reader.mind","--save-speaker",work/"complete-observer.mind",
                       lesson=first+rest))==["west","north","west","north","south","west"]
    detail=observations(entries()[-1]);assert [bool(p&256) for p in detail["perceptions"]]==[False,False,True,True,False,True]
    run("talk","--save-listener",work/"split-reader.mind","--save-speaker",work/"split-observer.mind",lesson=first)
    assert uttered(run("talk","--listener",work/"split-reader.mind","--speaker",work/"split-observer.mind",
                       "--save-listener",work/"resumed-reader.mind","--save-speaker",work/"resumed-observer.mind",lesson=rest))==["north","south","west"]
    for role in ("reader","observer"):
        assert (work/f"complete-{role}.mind").read_bytes()==(work/f"resumed-{role}.mind").read_bytes()
    assert uttered(run("talk","--pitch","muted",lesson=first))==["west","north","east"]
    attend=run("attention","--generations",0,"--listener","examples/inquirer.mind","--speaker","examples/observer.mind",
               "--length",19,"--probe-worlds",16,"--cycles",4)
    attention=entries()[-1]
    assert attention["kind"]=="attention" and "revised_chain=64" in next(s for s in attend.splitlines() if s.startswith("attention=intact"))
    verify_webp(journal / attention["path"] / "plate.webp")
    scale=run("attention","--generations",0,"--listener","examples/inquirer.mind","--speaker","examples/observer.mind",
              "--replicas",137,"--decisions",37,"--probe-worlds",2,"--cycles",2)
    batch=[dict(s.split("=",1) for s in line.split()) for line in scale.splitlines() if line.startswith("attention_batch=")]
    assert len(batch)==6 and all(int(row["correct"])==137*37 for row in batch)
    assert {row["bytes_per_pair"] for row in batch}=={"80","20"} and len({row["checksum"] for row in batch})==1
    assert entries()[-1]["kind"]=="attention-scale"
    assay=run("imagine","--load","examples/observer.mind","--observations",16,"--probes",4,"--worlds",16,"--length",7,"--dynamics","arbitrary")
    assert "changed_sequence=64" in next(s for s in assay.splitlines() if s.startswith("imagery=intact"))
    assert entries()[-1]["kind"]=="imagery"
    run("imagine","--load","examples/observer.mind","--observations",0,"--origin","fresh","--worlds",1,"--probes",1,"--save",work/"blank.mind")
    scenes="/scene north east\n/show-motion flit north east\n/show-motion crest east west\n/show-motion crest north south\n/show-motion flit south north\n"
    forecasts="Food after flit crest?\nFood after crest flit?\n/scene south east\nWhere was the food?\n"
    assert uttered(run("talk","--speaker",work/"blank.mind","--save-speaker",work/"forecast.mind",lesson=scenes+forecasts))==["west","north","north"]
    trace=observations(entries()[-1])["perceptions"]
    assert [bool(p&2048) for p in trace]==[True,True,False] and all(p>>12==1 for p in trace[:2])
    assert uttered(run("talk","--speaker",work/"forecast.mind",lesson="/scene north east\nFood after flit crest?\n"))==["west"]
    assert uttered(run("talk","--speaker",work/"forecast.mind","--imagination","muted",lesson="/scene north east\nFood after flit crest?\n"))==["north"]
    role_assay=run("relate","--load","examples/naturalist.mind","--generations",0,"--effects","independent","--cycles",4,"--probe-worlds",8,
                   "--teach",0,"--newborn","--save",work/"unreared-roles.mind")
    assert "revised_second=32" in next(s for s in role_assay.splitlines() if s.startswith("binding=intact"))
    assert entries()[-1]["kind"]=="binding"
    people=["north","east","south","west"]
    demos="".join(f"/show-pair vima {x} {x} {x} {x}\n" for _ in range(2) for x in people)
    before=demos+"/scene south west\nWhere is the food after vima?\n/try-pair vima north north east east west west north east\n"
    confirmed="/arrived east north\n/scene south west\nWhere is the food after vima?\nWhere is the water after vima?\n"
    full=run("talk","--speaker",work/"unreared-roles.mind","--save-speaker",work/"bound-all.mind","--save-listener",work/"reader-all.mind",lesson=before+confirmed)
    assert "experiment> 4 north east" in full and uttered(full)==["(silence)","west","south"]
    observations_all=observations(entries()[-1])
    assert [(p>>18)&3 for p in observations_all["perceptions"]]==[3,2,1]
    assert [((p>>20)&15,(p>>24)&15) for p in observations_all["perceptions"]][-2:]==[(0,8),(4,0)]
    run("talk","--speaker",work/"unreared-roles.mind","--save-speaker",work/"pending.mind","--save-listener",work/"reader-pending.mind",lesson=before)
    resumed=run("talk","--speaker",work/"pending.mind","--listener",work/"reader-pending.mind",
                "--save-speaker",work/"bound-resumed.mind","--save-listener",work/"reader-resumed.mind",lesson=confirmed+"/arrived north east\n")
    assert uttered(resumed)==["west","south"] and "There is no outstanding action." in resumed
    for pair in (("bound-all","bound-resumed"),("reader-all","reader-resumed")):
        assert (work/(pair[0]+".mind")).read_bytes()==(work/(pair[1]+".mind")).read_bytes()
    assay=run("conjoin","--load","examples/weaver.mind","--generations",0,"--probe-worlds",8,"--cycles",4)
    assert "completion=32" in next(s for s in assay.splitlines() if s.startswith("conjunction=intact"))
    assert "completion=0 " in next(s for s in assay.splitlines() if s.startswith("conjunction=no-recurrence"))
    assert entries()[-1]["kind"]=="conjunction"
    verify_webp(journal/entries()[-1]["path"]/"plate.webp")
    sandbox=run("conjoin","--load","examples/weaver.mind","--generations",0,"--probe-worlds",1,"--cycles",1,"--replicas",161,"--decisions",33)
    assert sandbox.count("joint_batch=compiled")==3 and entries()[-1]["kind"]=="conjunction-scale"
    census=run("conjoin","--load","examples/weaver-sparse.mind","--generations",0,"--probe-worlds",1,"--cycles",1,"--enumerate","representative")
    four=next(dict(x.split("=") for x in line.split()) for line in census.splitlines() if line.startswith("joint_clues=4 "))
    squares=runpy.run_path(str(ROOT/"tools/binding-study.py"))["latin_squares"]()
    forced=0
    for truth in (tuple(((i&3)+(i>>2))&3 for i in range(16)),tuple((i&3)^(i>>2) for i in range(16))):
        for clues in combinations(range(16),4):
            worlds=[world for world in squares if all(world[s]==truth[s] for s in clues)]
            forced+=sum(len({w[i] for w in worlds})==1 for i in range(16) if i not in clues)
    assert sum(int(four["forced_"+key]) for key in ("correct","wrong","silent"))==forced
    assert int(four["queries"])==2*1820*12 and entries()[-1]["kind"]=="conjunction-census"
    totals=next(dict(s.split("=") for s in line.split()) for line in census.splitlines() if line.startswith("joint_census="))
    assert all(int(totals[k])==0 for k in ("forced_wrong","ambiguous_wrong","ambiguous_lucky"))
    demos="".join(f"/show-pair tavu {people[x]} {people[y]} {people[(x+y)%4]} {people[x^y]}\n"
                  for y in range(4) for x in range(4) if (x,y)!=(1,2))
    run("talk","--save-speaker",work/"joint.mind",lesson=demos)
    question="/scene east south\nWhere is the food after tavu?\nWhere is the water after tavu tavu?\n"
    assert uttered(run("talk","--speaker",work/"joint.mind",lesson=question))==["west","north"]
    run("talk","--speaker",work/"joint.mind","--save-speaker",work/"pending-joint.mind",
        lesson="/try-pair tavu east south east south east south east south\n")
    revised=run("talk","--speaker",work/"pending-joint.mind",
                lesson="/arrived west north\n/scene east south\nWhere is the water after tavu?\n/arrived east west\n")
    assert uttered(revised)==["north"] and "There is no outstanding action." in revised
    run("relate","--load",work/"joint.mind","--generations",0,"--newborn","--teach",0,"--cycles",1,
        "--probe-worlds",1,"--save",work/"joint-newborn.mind")
    assert conjunctive_contacts(work/"joint-newborn.mind")==bytes(128)
    # A shutter on the second aperture cannot leak the covered world value.
    for hidden in ("east","south"):
        covered=run("talk","--speaker",work/"joint.mind","--recall-cut","erased",lesson=f"/cover 2\n/scene east {hidden}\nWhere is the food after tavu?\n")
        assert uttered(covered)==["(silence)"]
    garden=["commons","--load","examples/naturalist.mind","--families",32,"--replicates",2,"--days",32,"--generations",2,
            "--founders","varied","--exposures",0,"--receipts","learned","--seed",16057]
    budgets=[]
    for schedule in ("direct","graph"):
        trial=run(*garden,"--dispatch",schedule,"--save",work/(schedule+"-forager.mind"),"--save-newborn",work/(schedule+"-newborn.mind"))
        rows=[dict(s.split("=",1) for s in line.split()) for line in trial.splitlines() if line.startswith("commons_generation=")]
        for row in rows:
            row.pop("GPU_ms")
            v={k:int(row[k]) for k in ("residents","meals","deposited","energy","spent","food_supplied","food_left")}
            assert 12*v["residents"]+3*v["meals"]==v["deposited"]+v["energy"]+v["spent"]
            assert v["food_supplied"]==v["food_left"]+v["meals"]
        budgets.append(rows)
    assert budgets[0]==budgets[1] and len(budgets[0])==2
    for role in ("forager","newborn"):
        assert (work/("direct-"+role+".mind")).read_bytes()==(work/("graph-"+role+".mind")).read_bytes()
    household=entries()[-1];assert household["kind"]=="commons"
    people=observations(household)["commons_residents"]
    assert len(people)==64 and len({v[0] for v in people})==64
    verify_webp(journal/household["path"]/"plate.webp")
    banks=["commons","--load","examples/weaver.mind","--landmarks","coupled","--families",32,"--replicates",2,
           "--days",32,"--exposures",16,"--volatility",0,"--policy","harvest","--rations",32]
    full=run(*banks,"--save",work/"joint-adult.mind","--save-newborn",work/"joint-egg.mind")
    separate=run(*banks,"--joint-readout","muted")
    def meals(log):
        line=next(s for s in log.splitlines() if s.startswith("commons_generation="))
        return int(dict(item.split("=",1) for item in line.split())["meals"])
    assert meals(full)==64*32 and meals(separate)<meals(full)
    assert any(conjunctive_contacts(work/"joint-adult.mind"))
    assert conjunctive_contacts(work/"joint-egg.mind")==bytes(128)
    run("commons","--landmarks","coupled","--families",32,"--replicates",1,"--days",2)
    assert observations(entries()[-1])["source"].endswith("marsh-newborn.mind")
    empty=run(*garden,"--policy","investigate","--save-newborn",work/"unfunded.mind")
    assert "commons=no-offspring" in empty and not (work/"unfunded.mind").exists()
    social=["commons","--load","examples/tandem-newborn.mind","--families",32,"--replicates",2,"--days",48,
            "--seed",443969,"--landmarks","distinct","--pods","tandem","--calories",7,"--exposures",0,"--receipts","learned",
            "--neighbors","kin","--retina","focused","--reserve",24]
    reports=[]
    for channel,audit in (("open","on"),("open","off"),("muted","on")):
        tag=channel+audit
        text=run(*social,"--channel",channel,"--audit-calls",audit,"--save",work/(tag+".mind"))
        row=next(dict(part.split("=",1)for part in line.split())for line in text.splitlines()if line.startswith("commons_generation="))
        assert int(row["lived_days"])==sum(int(row[k])for k in ("experiments","meals","wrong_traps","lost_contests","unopened"))
        assert 24*int(row["residents"])+7*int(row["meals"])==sum(int(row[k])for k in ("energy","spent","deposited"))
        reports.append(row)
    assert int(reports[0]["matched_calls"])>0 and int(reports[1]["matched_calls"])==0 and int(reports[2]["heard"])==0
    assert (work/"openon.mind").read_bytes()==(work/"openoff.mind").read_bytes()
    excluded={"matched_calls","actual_call","delayed_call","displaced_call","GPU_ms"}
    assert {k:v for k,v in reports[0].items() if k not in excluded}=={k:v for k,v in reports[1].items() if k not in excluded}
    field_args=["--load","examples/confluence.eco","--steps",19]
    run("life",*field_args,"--save",work/"physical.eco")
    run("talk","--world","examples/confluence.eco","--save-world",work/"observed.eco",
        lesson="/visit 3 8\nWhere is the food?\n/advance 19\nWhere is the water?\n/quit\n")
    assert (work/"physical.eco").read_bytes()==(work/"observed.eco").read_bytes()
    run("talk","--world","examples/confluence.eco","--save-world",work/"tracked.eco",
        lesson="/follow\nWhere is the food?\n/advance 7\nWhere was the food?\n/quit\n")
    field=observations(entries()[-1]);tracked=[s for s in field["lines"] if s.startswith("habitat_view=") and "resident=" in s]
    assert tracked and all("born=" in s and "founder=" in s and "generation=" in s for s in tracked)
    assert [v["after"] for v in field["habitat_views"]]==[0,0,1]
    assert all(len(v["food"])==len(v["water"])==4 for v in field["habitat_views"])
    run("life","--load","examples/confluence.eco","--steps",7,"--save",work/"unwatched.eco")
    assert (work/"tracked.eco").read_bytes()==(work/"unwatched.eco").read_bytes()
    report=run("narrate","--load","examples/storyteller.mind","--generations",0,"--exposures",96,"--cycles",4,"--probe-worlds",8)
    row=lambda text,key:dict(s.split("=",1) for s in next(l for l in text.splitlines() if l.startswith(key)).split())
    assert row(report,"narration=intact")["decoded"]=="32" and row(report,"narration=muted")["decoded"]=="0"
    assert entries()[-1]["kind"]=="narration";verify_webp(journal/entries()[-1]["path"]/"plate.webp")
    actions="/scene north south\n/scene east south\n/scene east west\n"
    speech=run("talk","--save-speaker",work/"speaking.mind","--save-listener",work/"listening.mind",
               lesson=actions+"What happened?\n/heard\nWhat happened earlier?\n/heard\n")
    assert uttered(speech)==["water moved from south to west","food moved from north to east"]
    assert "receiver> water south west" in speech and "earlier> food north east" in speech
    assert entries()[-1]["kind"]=="report-interview"
    reported=run("talk","--speaker",work/"speaking.mind","--listener",work/"listening.mind",
                 lesson="/recount earlier\n/say happened water moved from west to south\n/recount happened\n/recount earlier\nWhat happened?\n")
    retold=[s[10:] for s in reported.splitlines() if s.startswith("listener> ")]
    assert retold==["food moved from north to east","water moved from west to south","(silence)"] and uttered(reported)==["water moved from south to west"]
    blind=run("talk","--report-wire","muted",lesson=actions+"What happened?\n/heard\n")
    assert uttered(blind)==["water moved from south to west"] and "receiver> (uncertain)" in blind
    report_world=("commons","--channel","reports","--families",32,"--replicates",2,"--days",24,"--tutoring",96,
                  "--policy","harvest","--calories",6,"--rations",12,"--seed",82139,"--volatility",0)
    reference=run(*report_world,"--dispatch","direct","--save",work/"reporter.mind","--save-neighbor",work/"recipient.mind")
    packed=run(*report_world,"--dispatch","graph","--report-cache","off")
    a=row(reference,"commons_generation=");b=row(packed,"commons_generation=");a.pop("GPU_ms");b.pop("GPU_ms");assert a==b
    assert int(a["correctly_decoded"])>1000 and entries()[-1]["kind"]=="culture"
    assert (work/"reporter.mind").read_bytes()!=(work/"recipient.mind").read_bytes()
    verify_webp(journal/entries()[-1]["path"]/"plate.webp")
    quiet=run(*report_world,"--report-wire","muted")
    assert row(quiet,"commons_generation=")["decoded"]=="0"
    assert int(row(reference,"commons_generation=")["deposited"])>int(row(quiet,"commons_generation=")["deposited"])
    run(*report_world,"--save",work/"one.mind","--save-neighbor",work/"one.mind",success=False)
    untrained=["commons","--load","examples/self-taught-newborn.mind","--channel","reports","--culture","invent",
               "--families",32,"--replicates",2,"--days",96,"--reserve",48,"--tutoring",0,"--policy","evolved",
               "--rations",12,"--calories",8,"--volatility",0,"--seed",273281]
    exploring=run(*untrained);c=row(exploring,"culture_generation=")
    assert c["tuition"]=="0" and int(c["learned_visits"])>0
    assert int(row(exploring,"commons_generation=")["correctly_decoded"])>0
    locked=run(*untrained,"--hearing","fixed")
    assert row(locked,"culture_generation=")["learned_visits"]=="0" and row(locked,"commons_generation=")["decoded"]=="0"

    changes="Which harbor for pearl?\n/gaze witness\n/link 2 8 13\nWhich harbor for pearl?\nWhich harbor would raven expect for pearl?\n"
    log=run("talk",lesson=changes+"/quit\n")
    assert uttered(log)==["cove","quay","cove"] and entries()[-1]["kind"]=="concept-interview"
    frames=observations(entries()[-1]);replies=[row(t.partition("  prompt=")[0],"concept_turn=") for t in frames["lines"] if t.startswith("concept_turn=")]
    assert replies[-1]["path"]=="1,16,256,4096" and replies[-1]["views"]=="2"
    assert "The witness learned from the example." in run("talk",lesson="pearl fords cove.\n/quit\n")
    assert "board> 0" in run("talk",lesson="/map 0 0 1 2\n/quit\n")
    missing=run("talk",lesson="/board 3\n/view-example outsider harbor 31 0\n/quit\n")
    assert "board> 0" in missing
    dry=run("infer","--load","examples/cartographer.mind","--census","unloads","--law","6")
    certificate=row(dry,"concept_census=");assert certificate["exact"]==certificate["trials"]=="1594323" and int(certificate["ambiguous"])>0
    batch=run("infer","--word","unloads","--batch",37,"--decisions",33)
    fast=row(batch,"endpoint_batch=projected");assert fast["bytes"]=="24" and fast["verified"]=="1184" and fast["queries"]=="1221"
    delta=("commons","--channel","estuary","--route-depth",3,"--families",32,"--replicates",2,"--days",32,
           "--reserve",48,"--calories",6,"--volatility",0,"--exposures",12,"--founders","varied","--seed",88237,"--generations",2,"--mentors","local")
    sequential=run(*delta,"--dispatch","direct","--route-layout","direct","--save",work/"tidal-direct.mind")
    coordinated=run(*delta,"--dispatch","graph","--route-layout","warp","--save",work/"tidal-warp.mind")
    assert (work/"tidal-direct.mind").read_bytes()==(work/"tidal-warp.mind").read_bytes()
    for label in ("estuary_generation=","commons_generation="):
        first,last=row(sequential,label),row(coordinated,label)
        for key in first.keys()-{"warp","GPU_ms"}:assert first[key]==last[key],(label,key)
    assert entries()[-1]["kind"]=="estuary";verify_webp(journal/entries()[-1]["path"]/"plate.webp")
    shut=run(*delta,"--credit","frozen","--policy","harvest")
    fallen=row(shut,"estuary_generation=");assert fallen["lessons"]==fallen["eight_hops"]=="0"

    glass="/greenhouse 16 0\nHow can pearl become fruiting?\n"
    why="Why soak?\n";log=run("talk","--save-speaker",work/"gardener-half.mind","--save-listener",work/"garden-mate.mind",lesson=glass+"/quit\n")
    heard=uttered(log);assert len(heard)==1 and set(heard[0].split())==set("soak heat sow tend pollinate ripen".split())
    assert uttered(run("talk","--speaker",work/"gardener-half.mind","--listener",work/"garden-mate.mind",lesson=why+"/quit\n"))==["skipping soak leaves pearl dry blocking fruiting"]
    actual=run("talk",lesson=glass+why+"Make pearl fruiting.\nEdible pearl?\n/quit\n")
    assert uttered(actual)[-1]=="yes" and entries()[-1]["kind"]=="garden-interview"
    histories=observations(entries()[-1]);assert histories["metrics"]["garden_actions"]==6
    assert any("repairs=1 " in line for line in histories["lines"] if line.startswith("garden_event="))
    routed=run("talk",lesson="/greenhouse 16 0\n/gaze witness\n/link 2 8 13\nMake the harbor of pearl warm.\nMake the harbor raven expects for pearl moist.\n/read-plot 12\n/read-plot 13\n/quit\n")
    assert "garden> 1 site=12 state=1" in routed and "garden> 1 site=13 state=2" in routed
    bed=("commons","--channel","garden","--families",32,"--replicates",2,"--days",24,"--generations",3,"--reserve",40,"--calories",24,
         "--exposures",16,"--founders","varied","--volatility",7,"--seed",77183,"--pollen",1)
    saved=run(*bed,"--crop-policy","cached","--dispatch","graph","--save",work/"growing.mind")
    rebuilt=run(*bed,"--crop-policy","rebuild","--dispatch","direct","--save",work/"regrown.mind")
    assert (work/"growing.mind").read_bytes()==(work/"regrown.mind").read_bytes()
    for label in ("greenhouse_generation=","commons_generation="):
        one,two=row(saved,label),row(rebuilt,label)
        for key in one.keys()-{"cached","policy_builds","GPU_ms"}:assert one[key]==two[key],(label,key)
    assert entries()[-1]["kind"]=="greenhouse";verify_webp(journal/entries()[-1]["path"]/"plate.webp")
    seedbank=work/"plants.seeds";run(*bed,"--save-seeds",seedbank)
    resown=run(*bed,"--plant-seeds",seedbank,"--save-seeds",work/"descendants.seeds");assert "greenhouse_generation=" in resown
    physical=run("talk",lesson=f"/seed-bank {seedbank}\n/read-plot 0\n/quit\n")
    assert "garden> 1 site=0 state=" in physical
    seeds=bytearray(seedbank.read_bytes());seeds[-1]^=1;(work/"corrupt.seeds").write_bytes(seeds)
    run(*bed,"--plant-seeds",work/"corrupt.seeds",success=False)

    # Queued runs can be rendered days after they finish or forwarded from a separate study journal.
    receipt=journal/".receipt-delayed-test"
    replay=dict(data);replay.pop("recorded_at")
    receipt.write_text(json.dumps(replay));os.utime(receipt,(1600000000,1600000000))
    run("journal","rebuild")
    assert entries()[-1]["date"]=="2020-09-13T12:26:40Z"
    receipt.write_text(json.dumps(dict(replay,recorded_at="2025-02-03T04:05:06Z")))
    run("journal","rebuild")
    assert entries()[-1]["date"]=="2025-02-03T04:05:06Z"


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="axion-journal-", dir=os.environ.get("TMPDIR", "/var/tmp")) as work:
        exercise(Path(work))
    print("PASS CUDA sampling, checkpoint replay, learned causal experiments, commons conservation, private-event reports, acoustic retelling, resident tracking and WebP journals")
