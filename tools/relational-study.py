#!/usr/bin/env python3
"""Reproduce the thicket experiments. All simulation, selection and probes run on CUDA."""
import argparse
import csv
import ctypes
from contextlib import ExitStack
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
BINARY = os.environ.get("AXION_BIN", str(ROOT / "axion"))
RELEASE = ROOT / "examples" / "relations.eco"
THICKET_RELEASE = ROOT / "examples" / "thicket.eco"
PATHWAYS = ROOT / "examples" / "pathways.eco"
BREED = (1277, 2203, 3179, 5711, 7823, 9029, 11783, 20939)
AUDIT = (143879, 369773, 716399, 861641)
WILD = (379, 2191, 8461, 19081)
VALIDATION = ("mixed-laws", "kinds", "transposed")
THICKET = ("--habitat", "thicket", "--species", 16, "--relations", "on", "--synapse-bits", 2)
WAVES = ("--habitat", "thicket", "--species", 16, "--relations", "waves", "--synapse-bits", 2, "--fruit", "inherited")
MEASURE = ("items naive learned practiced_items practiced erased changed_items stable_items surprise "
           "after_one recovered retained readout_cut connected discordant_items discordant_correct "
           "alternate_correct attempts delivered successes").split()


def run(*args):
    output = subprocess.check_output([BINARY, "life", *map(str, args)], text=True)
    return [dict(word.split("=", 1) for word in line.split() if "=" in word)
            for line in output.splitlines()]


def panel(path, problem, seed, exposures=32, quartets=128):
    return [r for r in run("--load", path, "--steps", 0, "--problem", problem, "--varieties", 16,
                           "--conditioning", quartets, "--exposures", exposures, "--probe-seed", seed)
            if "conditioning" in r]


def output(fields):
    writer = csv.DictWriter(sys.stdout, fieldnames=fields, extrasaction="ignore", lineterminator="\n")
    writer.writeheader()
    return writer


def breed(folder, population):
    writer = output(["breeding", "seed", "values", "generations", "tested_lifetimes", "GPU_ms",
                     *VALIDATION, "items_mixed", "items_kinds", "items_transposed"])
    path, best = folder / "candidate.eco", (-1, -1)
    for treatment, seeds in (("select", BREED), ("drift", BREED[:3])):
        for seed in seeds:
            for values in (8, 16):
                report = run(*THICKET, "--side", 16, "--seed", seed, "--nursery", 320,
                             "--nursery-varieties", values, "--nursery-problem", "mosaic",
                             "--breeding", treatment, "--steps", 0, "--save", path)
                row = dict(next(r for r in report if "nursery" in r), breeding=treatment, seed=seed, values=values)
                rates = []
                for task, denominator in zip(VALIDATION, ("items_mixed", "items_kinds", "items_transposed")):
                    scores = panel(path, task, 605939, quartets=32)[0]
                    row[task], row[denominator] = scores["learned"], scores["items"]
                    rates.append(int(scores["learned"]) / int(scores["items"]))
                writer.writerow(row)
                score = min(rates), sum(rates)
                if treatment == "select" and score > best:
                    best = score
                    shutil.copyfile(path, folder / "best.eco")
    if population:
        shutil.copyfile(folder / "best.eco", population)


def audit(population):
    writer = output(["population", "problem", "seed", "exposures", "lifetimes", "conditioning", *MEASURE])
    for seed in AUDIT:
        for task in ("grips", "dimensions", "composition", "parity", "mixed-laws", "law-switch",
                     "kinds", "transposed", "latent-table", "unstructured"):
            doses = (2, 4, 8, 16, 32, 64) if task in ("mixed-laws", "latent-table") else (32,)
            for dose in doses:
                for row in panel(population, task, seed, dose):
                    writer.writerow(dict(row, population="relational"))
        for row in panel(ROOT / "examples" / "learner.eco", "composition", seed):
            writer.writerow(dict(row, population="unary"))


def field(folder):
    metrics = ("tick alive founders max_generation births deaths edits energy plants attempts opened seedings "
               "plant_edits plant_boundaries response_disagreements updates fingerprint").split()
    writer = output(["flora", "density", "seed", *metrics, "naive", "learned", "items", "readout_cut"])
    for seed in WILD:
        for flora, density in (("evolving",35), ("clonal",35), ("fixed",35), ("evolving",0)):
            path = folder / "wild.eco"
            result = run(*THICKET, "--side", 64, "--seed", seed, "--flora", flora, "--density", density,
                         "--steps", 32768, "--report-every", 8192, "--save", path)
            times = [r for r in result if "tick" in r]
            if int(times[-1]["alive"]):
                rates = panel(path, "latent-table", 483601, quartets=32)[0]
                times[-1].update({f: rates[f] for f in ("naive","learned","items","readout_cut")})
            for row in times:
                writer.writerow(dict(row, density=density, seed=seed))


def occupied(path, count):
    # Use the ordinary reader, including its integrity and occupancy checks.
    found = 0
    for site in range(256):
        result = subprocess.run([BINARY, "agent", "--from", path, "--site", str(site), "--inspect"],
                                stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        if result.returncode:
            if "donor site must be occupied" in result.stderr:
                continue
            raise RuntimeError(result.stderr)
        yield site
        found += 1
        if found == count:
            return


def isolate(folder, destination):
    fields = ["values", "source_seed", "site", "alive", "births", "mixed_correct", "mixed_items",
              "transposed_correct", "transposed_items"]
    writer = output(fields)
    best = (-1, -1)
    source, garden = folder / "source.eco", folder / "garden.eco"
    for values, seed in ((16,2203), (16,1277), (8,2203)):
        run(*THICKET, "--side", 16, "--seed", seed, "--nursery", 320, "--nursery-varieties", values,
            "--nursery-problem", "mosaic", "--steps", 0, "--save", source)
        for site in occupied(source, 16):
            args = [*THICKET, "--fruit", "inherited", "--founders", source, "--founder-site", site]
            log = run(*args, "--side", 32, "--seed", 23813, "--mutation", "off", "--steps", 2048, "--save", garden,
                      "--conditioning", 32, "--varieties", 16, "--exposures", 32, "--problem", "mixed-laws", "--probe-seed", 308411)
            last = next(r for r in log if r.get("tick") == "2048")
            row = dict(last, values=values, source_seed=seed, site=site)
            if int(last["alive"]) and int(last["births"]):
                m = next(r for r in log if r.get("conditioning") == "intact")
                t = panel(garden, "transposed", 308411, quartets=32)[0]
                row.update(mixed_correct=m["learned"], mixed_items=m["items"], transposed_correct=t["learned"], transposed_items=t["items"])
                score = min(int(m["learned"])/int(m["items"]), int(t["learned"])/int(t["items"])), int(last["alive"])
                if score > best:
                    best = score
                    shutil.copyfile(source,folder / "source-winner.eco")
                    winner = site
            writer.writerow(row)
    if destination:
        if best[0] < 0:
            raise RuntimeError("no reproducing isolate")
        # The released world is a separate habitat. Both populations can mutate from its first birth.
        run(*THICKET, "--fruit", "inherited", "--side", 32, "--seed", 29663, "--founders", folder / "source-winner.eco",
            "--founder-site", winner, "--steps", 512, "--save", destination)


def retention(folder, population):
    writer = output(["seed", "mutation", "tick", "alive", "births", "max_generation", "seedings", "plant_edits",
                     "problem", "items", "naive", "learned", "readout_cut", "discordant_items", "discordant_correct"])
    for seed in (1801,3929,8059,12011):
        for mutation in ("on", "off"):
            garden = folder / "line.eco"
            for age, duration in ((0,0), (4096,4096), (32768,28672)):
                args = [*THICKET, "--fruit", "inherited", "--side", 64, "--seed", seed, "--founders", population,
                        "--mutation", mutation] if age == 0 else ["--load", garden]
                log = run(*args, "--steps", duration, "--save", garden)
                body = next(r for r in reversed(log) if "tick" in r)
                for task in ("mixed-laws", "latent-table"):
                    score = panel(garden,task,761143,quartets=64)[0] if int(body["alive"]) else {}
                    writer.writerow({**body, **score, "problem": task, "seed": seed, "mutation": mutation})


def inherited_thicket(population):
    genome = subprocess.check_output([BINARY,"agent","--from",population,"--inspect"],text=True)
    if "context_plastic=" in genome:
        return ("--habitat","thicket","--species",16,"--relations","contexts","--programs",2 if "alternate_plastic=" in genome else 1,
                "--synapse-bits",2,"--fruit","inherited")
    return WAVES if "propagation_steps=" in genome else (*THICKET,"--fruit","inherited")


def contest(population):
    writer = output(["fruit", "seed", "swap", "tick", "treatment_alive", "control_alive", "alive", "births", "opened", "attempts"])
    for seed in (601, 1423, 7919, 21911, 28057, 34631, 47797, 69127):
        for fruit in ("scattered", "persistent", "inherited"):
            for swap in (0, 1):
                result = run(*THICKET, "--flora", "clonal", "--fruit", fruit, "--side", 64, "--seed", seed,
                             "--founders", population, "--contest", "paths", "--mutation", "off", "--steps", 2048,
                             "--report-every", 512, *(["--swap-cohorts"] if swap else []))
                for row in result:
                    if "tick" in row:
                        writer.writerow(dict(row, seed=seed, swap=swap))


def speed(folder, population):
    writer = output(["side", "schedule", "repeat", "initial_alive", "alive", "GPU_bytes", "neural_regs/thread",
                     "neural_local_B/thread", "habitat_regs/thread", "habitat_local_B/thread",
                     "ticks", "updates", "GPU_ms", "Gcell_updates/s", "fingerprint"])
    environment = inherited_thicket(population)
    for side in (64, 256, 512, 1024):
        path = folder / "warm.eco"
        run(*environment, "--side", side, "--seed", 1847, "--founders", population, "--steps", 96, "--save", path)
        fingerprint = None
        for repeat in range(1, 4):
            for schedule in (("dense", "occupied") if repeat != 2 else ("occupied", "dense")):
                info, before, after = run("--load", path, "--steps", 96, "--schedule", schedule)
                if fingerprint is not None and fingerprint != after["fingerprint"]:
                    raise RuntimeError("schedulers diverged on the same thicket")
                fingerprint = after["fingerprint"]
                writer.writerow({**info, **after, "side": side, "schedule": schedule, "repeat": repeat, "ticks": 96,
                                 "initial_alive": before["alive"], "updates": int(after["updates"])-int(before["updates"])})


def pathways(folder, destination):
    tasks = ("chains", *VALIDATION)
    writer = output(["origin","seed","tested_lifetimes", *tasks, *(t+"_items" for t in tasks)])
    path, best = folder / "candidate.eco", (-1,-1)
    for source in ("fresh","thicket"):
        for seed in BREED[:6]:
            donor = [] if source == "fresh" else ["--founders",THICKET_RELEASE]
            birth = run(*WAVES, *donor, "--side",16,"--seed",seed,"--nursery",320,"--nursery-varieties",8,
                        "--nursery-problem","varied","--steps",0,"--save",path)
            row = dict(next(r for r in birth if "nursery" in r), origin=source, seed=seed)
            rates = []
            for task in tasks:
                r = panel(path,task,418643,16 if task == "chains" else 32,32)[0]
                row[task], row[task+"_items"] = r["learned"],r["items"]
                rates.append(int(r["learned"])/int(r["items"]))
            writer.writerow(row)
            if (min(rates),sum(rates)) > best:
                best = min(rates),sum(rates)
                shutil.copyfile(path,folder / "best.eco")
    if destination:
        shutil.copyfile(folder / "best.eco",destination)


def distance(population):
    writer = output(["problem","seed","lifetimes","exposures","conditioning", *MEASURE,
                     "distant_items","distant_correct","remote_items","remote_correct","local_readout","remote_local"])
    for seed in (49993,238919,607219,893399):
        for task in ("chains","parity-chains","islands","mixed-laws","kinds","transposed","latent-table","grips"):
            doses = (2,4,8,16,32) if task == "chains" else (16,) if task in ("parity-chains","islands") else (32,)
            for dose in doses:
                for row in panel(population,task,seed,dose):
                    writer.writerow(row)


def wave_garden(population):
    writer = output(["seed","swap","tick","treatment_alive","control_alive","alive","births","opened","attempts"])
    for seed in (601,1423,7919,21911,28057,34631,47797,69127):
        for swap in (0,1):
            result = run(*WAVES,"--founders",population,"--side",64,"--seed",seed,"--mutation","off",
                         "--contest","waves","--steps",2048,"--report-every",512,*(["--swap-cohorts"] if swap else []))
            for row in result:
                if "tick" in row:
                    writer.writerow(dict(row,seed=seed,swap=swap))


def energy(folder, population):
    writer = output(["sample","schedule","wall_s","board_mJ","mean_W","GPU_ms","updates","million_updates_per_J","alive","fingerprint"])
    cuda = ctypes.CDLL("libcuda.so.1")
    nvml = ctypes.CDLL("libnvidia-ml.so.1")
    def check(code, operation):
        if code:
            raise RuntimeError(f"{operation} failed with driver status {code}")
    ordinal, bus, device = ctypes.c_int(), ctypes.create_string_buffer(32), ctypes.c_void_p()
    check(cuda.cuInit(0),"initialize CUDA")
    check(cuda.cuDeviceGet(ctypes.byref(ordinal),0),"select CUDA device")
    check(cuda.cuDeviceGetPCIBusId(bus,len(bus),ordinal),"read CUDA PCI address")
    check(nvml.nvmlInit_v2(),"initialize NVML")
    try:
        check(nvml.nvmlDeviceGetHandleByPciBusId_v2(bus,ctypes.byref(device)),"match GPU energy counter")
        def reading():
            value = ctypes.c_ulonglong()
            check(nvml.nvmlDeviceGetTotalEnergyConsumption(device,ctypes.byref(value)),"read cumulative board energy")
            return value.value
        garden = folder / "warm.eco"
        run(*inherited_thicket(population),"--side",512,"--seed",1847,"--founders",population,"--steps",96,"--save",garden)
        fingerprint = None
        for sample in range(3):
            order = ("dense","occupied") if sample != 1 else ("occupied","dense")
            for name in ("idle", *order):
                start = time.monotonic()
                before = reading()
                row = {}
                if name == "idle":
                    time.sleep(5)
                else:
                    _, first, final = run("--load",garden,"--steps",4096,"--schedule",name)
                    if fingerprint is not None and final["fingerprint"] != fingerprint:
                        raise RuntimeError("energy trials ended in different worlds")
                    fingerprint = final["fingerprint"]
                    row = dict(final, updates=int(final["updates"])-int(first["updates"]))
                millijoules = reading()-before
                duration = time.monotonic()-start
                if millijoules <= 0:
                    raise RuntimeError("board energy counter did not advance")
                if row:
                    row["million_updates_per_J"] = f'{row["updates"]/(millijoules*1000):.3f}'
                writer.writerow(dict(row,sample=sample+1,schedule=name,wall_s=f"{duration:.4f}",board_mJ=millijoules,
                                     mean_W=f"{millijoules/(1000*duration):.3f}"))
                sys.stdout.flush()
    finally:
        nvml.nvmlShutdown()


def deep_time(folder, population, saved_world, repertoires):
    writer = output(["seed","mutation","tick","alive","births","deaths","max_generation","edits","seedings","plant_edits",
                     "moves","bitten","donated","stored","retrieved","opened","attempts","survey","distinct","history_sensitive",
                     "new_repertoires","ever_seen","mean_changed_actions","learned","items","reachable","fingerprint"])
    with ExitStack() as files:
        archive = None
        if repertoires:
            stream = files.enter_context(repertoires.open("w"))
            archive = csv.writer(stream,lineterminator="\n")
            archive.writerow(["seed","mutation","tick","phenotype","count","site","first_seen_tick"])
        for seed in (1801,3929,8059):
            for mutation in ("on","off"):
                observed, previous = {}, 0
                path = folder / "evolving.eco"
                for age in (0,1024,8192,65536,262144,1048576):
                    begin = [*WAVES,"--side",32,"--seed",seed,"--founders",population,"--founder-site",1,
                             "--mutation",mutation] if age == 0 else ["--load",path]
                    log = run(*begin,"--steps",age-previous,"--save",path,"--survey",128,"--survey-seed",473327,
                              "--conditioning",32,"--probe-seed",445183,"--problem","chains","--varieties",16,"--exposures",16)
                    previous = age
                    body = next(r for r in reversed(log) if "tick" in r)
                    census = next(r for r in log if "survey" in r)
                    test = next((r for r in log if r.get("conditioning") == "intact"), {})
                    profiles = [r for r in log if "phenotype" in r]
                    if age == 0:
                        if len(profiles) != 1:
                            raise RuntimeError("isogenic founders expressed different standardized behavior")
                        ancestor = bytes.fromhex(profiles[0]["phenotype"])
                    old, changed = len(observed), 0
                    for r in profiles:
                        name = r["phenotype"]
                        observed.setdefault(name,age)
                        changed += int(r["count"])*sum(a != b for a,b in zip(ancestor,bytes.fromhex(name)))
                        if archive:
                            archive.writerow([seed,mutation,age,name,r["count"],r["site"],observed[name]])
                    total = int(census["survey"])
                    writer.writerow({**body,**census,"seed":seed,"mutation":mutation,"new_repertoires":len(observed)-old,
                                     "ever_seen":len(observed),"mean_changed_actions":f"{changed/total:.3f}" if total else "",
                                     **{k:test.get(k,"") for k in ("learned","items","reachable")}})
                    sys.stdout.flush()
                if saved_world and seed == 1801 and mutation == "on":
                    shutil.copyfile(path,saved_world)


def lesson():
    # This external environment shares neither its random generator nor its item
    # order with the on-device assay. Answers enter through the physical mask only.
    rng = random.Random(541903)
    n = 8
    shape = rng.sample(range(16), n)
    texture = rng.sample(range(16, 32), n)
    left = rng.sample(list(range(4))*2, n)
    right = rng.sample(list(range(4))*2, n)
    decoding = rng.sample(range(4), 4)
    chemistry = [rng.randrange(4) for _ in range(16)]
    practice = [(s,t) for s in range(n) for t in range(n) if (t-s) % n < n//2]
    novel = [(s,t) for s in range(n) for t in range(n) if (t-s) % n >= n//2]
    def emit(op, pair, table):
        s,t = pair
        print(f"{op} 0x{(1<<shape[s]) | (1<<texture[t]):08x} 0x01070000 0x0b 0x{8 | table[4*left[s]+right[t]]:02x}")
    ring = [decoding[(a+b)%4] for a in range(4) for b in range(4)]
    for name, table in (("First chemistry", ring), ("New plant chemistry", chemistry)):
        print(f"# {name}. Probe frozen choices, deliver 256 consequences, then probe the same 32 unseen pairs.")
        for pair in novel:
            emit("probe", pair, table)
        for _ in range(8):
            rng.shuffle(practice)
            for pair in practice:
                emit("trial", pair, table)
        for pair in novel:
            emit("probe", pair, table)


def chain_lesson():
    rng = random.Random(627347)
    shape, texture = rng.sample(range(16),16), rng.sample(range(16,32),16)
    left, right = [rng.randrange(4) for _ in shape], [rng.randrange(4) for _ in texture]
    offset = rng.randrange(4)
    known = [(s,t) for s in range(16) for t in range(16) if (t-s)%16 < 2]
    unseen = [(s,t) for s in range(16) for t in range(16) if (t-s)%16 >= 2]
    def emit(op,pair):
        s,t = pair
        answer = 8 | ((left[s]+right[t]+offset) & 3)
        print(f"{op} 0x{(1<<shape[s]) | (1<<texture[t]):08x} 0x01070000 0x0b 0x{answer:02x}")
    for _ in range(8):
        rng.shuffle(known)
        for pair in known:
            emit("trial",pair)
    rng.shuffle(unseen)
    for pair in unseen:
        emit("probe",pair)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("experiment", choices=("breed","audit","field","isolate","retention","contest","speed","energy","lesson","pathways","distance","wave-garden","chain-lesson","deep-time"))
    parser.add_argument("--population", type=Path, help="input habitat for measurements, output habitat for breed, isolate or pathways")
    parser.add_argument("--save-world", type=Path, help="save the first mutating million-tick world in deep-time")
    parser.add_argument("--repertoires", type=Path, help="write sampled motor responses and exemplar sites in deep-time")
    args = parser.parse_args()
    if args.experiment == "lesson":
        lesson()
        return
    if args.experiment == "chain-lesson":
        chain_lesson()
        return
    with tempfile.TemporaryDirectory(prefix="axion-relations-", dir=os.environ.get("TMPDIR", "/var/tmp")) as tmp:
        folder = Path(tmp)
        default = THICKET_RELEASE if args.experiment in ("retention","speed","energy") else PATHWAYS if args.experiment in ("distance","wave-garden","deep-time") else RELEASE
        population = args.population or default
        if args.experiment == "breed":
            breed(folder, args.population)
        elif args.experiment == "audit":
            audit(population)
        elif args.experiment == "field":
            field(folder)
        elif args.experiment == "isolate":
            isolate(folder, args.population)
        elif args.experiment == "retention":
            retention(folder, population)
        elif args.experiment == "pathways":
            pathways(folder,args.population)
        elif args.experiment == "distance":
            distance(population)
        elif args.experiment == "wave-garden":
            wave_garden(population)
        elif args.experiment == "energy":
            energy(folder,population)
        elif args.experiment == "deep-time":
            deep_time(folder,population,args.save_world,args.repertoires)
        elif args.experiment == "contest":
            contest(population)
        else:
            speed(folder, population)


if __name__ == "__main__":
    main()
