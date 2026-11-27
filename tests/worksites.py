#!/usr/bin/env python3
"""Audit real habitat stocks, field decisions and the lifetime of a grounded speaker using independent readers."""

import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
EXE = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()
BANK = ROOT / "examples/shinglebank.flood"
SPEAKER = ROOT / "examples/shingle-witness.mind"
PEER = ROOT / "examples/shore-reader.mind"
MASK = (1 << 32) - 1


def mix(x):
    x &= MASK
    x = ((x ^ (x >> 16)) * 0x21F0AAAD) & MASK
    x = ((x ^ (x >> 15)) * 0x735A2D97) & MASK
    return x ^ (x >> 15)


def checksum(raw):
    h = 14695981039346656037
    for char in raw:
        h = ((h ^ char) * 1099511628211) & ((1 << 64) - 1)
    return h.to_bytes(8, "little")


def snapshot(path):
    raw = path.read_bytes()
    assert raw[:8] == b"AXFLOOD\0" and raw[-8:] == checksum(raw[:-8])
    version, tick, side, tiles = struct.unpack_from("<4I", raw, 8)
    extended = bool(version & 256)
    v = version & ~1073741568
    assert 17 <= v <= 28
    n = side * side
    pos = 368

    def take(fmt, count):
        nonlocal pos
        width = struct.calcsize(fmt) * count
        result = list(struct.iter_unpack(fmt, raw[pos : pos + width]))
        pos += width
        return result

    soil = take("<7H2B2HI", n)
    slots = [r[0] for r in take("<I", n)]
    pos += 172 * n
    bodies = take("<96s", n)
    ages = [tick - struct.unpack_from("<I", q, 20)[0] for (q,) in bodies]
    energy = [struct.unpack_from("<H", q, 28)[0] for (q,) in bodies]
    reserves = sum(energy) + sum(struct.unpack_from("<H", q, 76)[0] for (q,) in bodies)
    if version&536870912:
        stored,=take("<Q",1)[0];lengths=raw[pos:pos+n];pos+=n
        assert len(lengths)==n and all((z&127)<=tiles and z!=128 for z in lengths) and sum(z for z in lengths if z<128)==stored
        pos+=stored*16
    else:pos+=n*16*tiles
    pos+=n*96
    if (v - 9) & 1:
        pos += 104
        reserves += sum(sum(raw[pos + 176 * i + 104 : pos + 176 * i + 136]) for i in range(n))
        pos += 177 * n
    if (v - 9) & 2:
        pos += 72 + 12 * n
    inventory = struct.unpack_from("<5Q", raw, pos + 16)
    mineral = struct.unpack_from("<13Q", raw, pos + 56)
    pos += 160
    field = take("<4B", n)
    workers = [q for (q,) in take("<72s", n)]
    grain = field + [q[56:60] for q in workers]
    assert sum(bool(g[0]) for g in grain) == inventory[0]
    assert (
        tuple(sum((g[0] >> (2 * d)) & 3 == k for g in grain for d in range(4)) for k in (1, 2, 3)) == inventory[2:]
    )
    charge = sum(g[2] for g in grain)
    pos += 56
    forecast = take("<4I", n)
    account = struct.unpack_from("<12Q", raw, pos + 8)
    pos += 104
    mind = take("<6I", n)
    film_pos = pos
    film = take("<2I", n)
    assert all(t <= tick or t == MASK for _, t in film)
    assert all((m >> (8 * j)) & 128 or not (m >> (8 * j)) & 255 for m, _ in film for j in range(4))
    assert inventory[1] + mineral[0] + account[6] == charge + mineral[2] + mineral[12]
    bound = 0
    if v >= 21:
        opening, adsorbed, released = struct.unpack_from("<3Q", raw, pos + 8)
        pos += 64
        bound = sum(q for (q,) in take("<H", n))
        assert opening + adsorbed == bound + released
    light, heat = struct.unpack_from("<2Q", raw, 104)
    initial = struct.unpack_from("<Q", raw, 88)[0]
    loose = sum(sum(q[4:7]) + sum(q[9:11]) for q in soil)
    hollow_stock = 0
    salts = []
    learners = []
    salt_pos = pos
    species = 0
    flow = 0
    stock = ()
    if v >= 25:
        species, flow, *stock = struct.unpack_from("<II5Q", raw, pos)
        pos += 48
        salts = [q for (q,) in take("<I", n)]
        learners = take("<21I", n)
        assert 2 <= species <= 5 and flow <= 7 and all(q >> (6 * species) == 0 for q in salts)
        assert [sum(q >> (6 * i) & 63 for q in salts) for i in range(5)] == stock
        assert all(not ((r[19]|r[20])&~r[16]) for r in learners)
    callers=[];ports=[];echo=[];native_words={};reed_pos=pos;echo_pos=0;audience=();reed=()
    if version&512:
        reed=take("<4I11Q",1)[0]
        ports=list(raw[pos:pos+n]);pos+=n
        callers=take("<14I",n)
        assert reed[0] in (1,2,4) and reed[7]<=reed[4] and reed[10]<=reed[9]
    if version&1024:
        audience=take("<2I4Q",1)[0];echo_pos=pos
        echo=take("<2I",n)
        assert audience[3]<=audience[2] and audience[5]<=audience[2]
    workshop=();potters=[];ware=[];cargo=[];clay_pos=pos
    if version&4096:
        workshop=take("<2I"+("31Q" if version&65536 else "27Q" if version&32768 else "25Q" if version&16384 else "13Q" if version&8192 else "11Q"),1)[0]
        potters=take("<12I",n);ware=take("<3I",n);cargo=take("<3I",n)
        assert bool(workshop[0]&128)==bool(version&16384) and workshop[0]<=524287 and workshop[5]<=workshop[4]<=workshop[3]
        for source,things in ((ware,field),(cargo,[r[56:60] for r in workers])):
            for i,(left,right,date) in enumerate(source):
                assert (not left and not right) or things[i][0]
                assert date<=tick or date==MASK
                assert all((v==0 or v&160==160) for v in (left.to_bytes(4,"little")+right.to_bytes(4,"little")))
    pockets=[];rangers=[];marks=[];sightings=[];recess=();place_words={}
    if version&131072:
        recess=take("<2I18Q",1)[0];capacity=recess[0]
        assert 1<=capacity<=32 and recess[1]<8192
        pockets=take("<HBB",n);rangers=take("<4I4BI",n)
        marks=[q for q, in take("<I",n*capacity)]
        sightings=[q for q, in take("<I",((n+15)//16)*capacity)]
        hollow_stock=sum(p[0] for p in pockets)
        assert recess[2]+recess[3]+recess[4]==hollow_stock+recess[5]
        assert recess[6]<=recess[5] and all((s&0x7fffffff)<=n for s in sightings)
        assert all(p[0]<=255 and p[1]<=64 and p[2]<=15 and (p[1] or not(p[0] or p[2])) for p in pockets)
        for i,r in enumerate(rangers):
            assert r[6]<4 and r[7]<=min(capacity,1+(r[1]>>24&31))
            for j in range(capacity):
                c=marks[j*n+i]
                assert not c or (j<r[7] and c>>26&3)
    energy_in_known_pools=loose+reserves+charge+bound+hollow_stock+heat
    outlay=();economies=[];treads=[]
    if version&524288:
        assert version&131072
        outlay=take("<2I31Q",1)[0]
        economies=take("<15I",n)
        treads=list(raw[pos:pos+n]);pos+=n
        assert outlay[2+4]==sum((x&15)+(x>>4) for x in treads)+outlay[2+5]
        assert outlay[2+3]+outlay[2+6]==outlay[2+7]
        assert outlay[2+8]==outlay[2+9]+outlay[2+10] and outlay[2+11]<=outlay[2+8]
        for brain in economies:
            seen,trace,capital=brain[12:]
            assert seen<1<<24
            left=((trace>>5)&15)|(((seen>>8)&7)<<4)
            assert left or (trace==capital==0 and not (seen&0xf00))
    machinery=();inventors=[];circuits=[];seals=[]
    if version&1048576:
        assert version&131072
        machinery=take("<4I27Q",1)[0]
        mode,arena,period,seed,*books=machinery
        assert mode<65536 and arena in (1,2,4) and period in (1,2,4,8,16,32,64)
        assert books[4]==books[2]+2*books[22]+books[23] and books[11]==books[12]+books[13]
        inventors=take("<6I",n);circuits=take("<8I",n*arena);seals=take("<5I",n//4)
        for m in inventors:
            occupied=set()
            for slot in range(4):
                glyph=(m[4]>>(4*slot))&15
                if glyph:
                    capacity=min(1<<((m[2]>>2)&3),arena)
                    assert glyph<=4 and (glyph-1^(m[2]&3))&(capacity-1)==slot and glyph not in occupied
                    occupied.add(glyph)
        assert all(p[0]<=n or p[0]==0xffffffff for p in seals)
    practice=();students=[];contexts=[];attribution=();strands=[];wicks=();parcels=[]
    if version&2097152:
        assert version&1048576
        practice=take("<4I26Q",1)[0];mode,cap,seed,revision,*account=practice
        assert 0<cap<=4 and mode<131072 and revision==1
        students=take("<8I",n);contexts=take("<5I",n*cap)
        assert account[16]<=account[15]<=account[14] and account[17]<=account[15]
        assert account[13]>=account[5]+2*account[18]+account[19]+account[12]
        for i,m in enumerate(students):
            used=m[4]&7;assert used<=min(cap,1+(m[1]>>2&3)) and (not used or (m[4]>>3&3)<used)
            assert m[5]>>12<=n//4 and (not m[5] or m[5]&63)
            for j in range(used,cap):assert not any(contexts[n*j+i])
    if version&4194304:
        assert version&2097152
        attribution=take("<12Q",1)[0];strands=take("<4I",n//4)
        assert attribution[2]<=attribution[1] and attribution[9]<=attribution[7]<=attribution[5]<=attribution[4]
        assert all(not r[0] or ((r[0]<=n or r[0]==0xffffffff) and r[3]<16 and r[3]>>2>(r[3]&3)) for r in strands)
    if version&8388608:
        assert version&131072
        wicks=take("<2I15Q",1)[0];parcels=[v for v, in take("<I",n)]
        origins=[(p>>7)&0xfffff for p in parcels if p&(1<<27)]
        assert len(origins)==len(set(origins))==wicks[2] and all(i<n for i in origins)
        assert wicks[2+5]<=wicks[2+3] and wicks[2+7]<=wicks[2+3]
    rush=();fold_cells=[];fold_map=[];reed_social=()
    if version&16777216:
        assert version&8388608
        rush=take("<4I22Q",1)[0];fold_cells=take("<12I",n);fold_map=[v for v, in take("<B",n)]
        assert rush[3] in (1,2) and rush[2] in range(1,257)
        for m in fold_cells:
            for b in range(2):assert not (m[5+b]|m[7+b]|m[9+b])&~m[3+b]
        for i,v in enumerate(fold_map):
            for ax in range(2):
                shape=(v>>(4*ax))&7;tag=(v>>(4*ax))&8;assert shape<=4 and (not shape or soil[i][9+ax]) and (not tag or shape)
        if rush[3]==2:reed_social=take("<14Q",1)[0]
    bank=();listeners=[];bank_query=()
    if version&33554432:
        bank=take("<4I24Q",1)[0];listeners=take("<6I",n)
        assert bank[0] in (1,2,3) and bank[1]<=255 and bank[3]<4 and bank[12]==bank[4]+bank[6]
        for _,ear,_,_,weights,trace in listeners:
            assert trace<2**27 and (trace&2 or trace<=1) and (not trace&1 or ear>>19&1)
        if bank[0]>=2:
            bank_query=take("<5Q",1)[0];assert bank_query[1]<=bank_query[0]<=bank[6]
        if bank[0]>=3:
            mesh=take("<6Q",1)[0];assert mesh[2]<=mesh[1]<=bank[6] and mesh[5]<=bank[11]
    floats=[];riggers=[];flute_words={};loom=();relief=();drift=()
    if version&67108864:
        loom=take("<4I28Q",1)[0];assert loom[0] in (1,2,3,4,5,6,7) and loom[1]<=(2097151 if loom[0]>=7 else 1048575 if loom[0]>=6 else 262143 if loom[0]>=5 else 131071 if loom[0]>=4 else 32767)
        riggers=take("<4I12HHBBI",n);floats=[z for z, in take("<H",n)]
        incoming,outgoing=take("<2Q",1)[0] if loom[0]>=2 else (0,0)
        assert sum((z>>2)&31 for z in floats)==loom[4]+incoming-loom[4+15]-outgoing
        if loom[0]>=3:
            phase,=take("<I",1)[0];book=take("<21Q",1)[0];gates=take("<2H",n)
            assert phase<4 and book[3]<=book[2] and all(not (b&~a) for a,b in gates)
        if loom[0]>=4:
            relief=take("<9Q",1)[0];wet,shaped,opened,closed,gain,loss,lodged,washed,turned=relief
            assert opened+closed<=shaped<=wet and opened<=gain and closed<=loss and turned<=lodged and lodged+washed<=wet
        if loom[0]>=7:
            drift=take("<4I34Q2I",1)[0];assert drift[5]==drift[6]+drift[7]
        for _ in range(loom[3]):
            slot,born,*forms=take("<2I8Q",1)[0];assert slot not in flute_words and energy[slot] and born==tick-ages[slot]
            flute_words[slot]=forms
    float_events=take("<3I",n) if version&134217728 else []
    assert all(not (known>>16) and (stamp or not (images or known)) for images,known,stamp in float_events)
    narrators={}
    if version&268435456:
        count,=take("<I",1)[0];assert float_events and 0<count<=n
        for _ in range(count):
            slot,born,memory=take("<2I568s",1)[0];assert slot not in narrators and energy[slot] and born==tick-ages[slot]
            narrators[slot]=memory
    assert initial+light==energy_in_known_pools+sum((z>>2)&31 for z in floats)
    if version&262144:
        count,=take("<I",1)[0];assert version&131072 and 0<count<=n
        for _ in range(count):
            slot,born,*law=take("<6I",1)[0];vocab=raw[pos:pos+1112];pos+=1112
            assert energy[slot] and born==tick-ages[slot] and slot not in place_words
            assert struct.unpack_from("<2I",vocab,1100)==(0x43574f52,1112)
            place_words[slot]=(law,vocab)
    if version&2048:
        count,=take("<I",1)[0]
        assert 0<count<=n
        for _ in range(count):
            slot,born,*law=take("<6I",1)[0]
            assert energy[slot] and ages[slot]==tick-born and slot not in native_words
            layout=raw[pos:pos+1112];pos+=1112
            assert struct.unpack_from("<2I",layout,1100)==(0x43574f52,1112)
            native_words[slot]=(law,layout)
    voices = []
    if extended:
        (count,) = struct.unpack_from("<I", raw, pos)
        pos += 4
        for _ in range(count):
            slot, born = struct.unpack_from("<2I", raw, pos)
            pos += 8
            assert energy[slot] and ages[slot] == tick - born and slot not in voices
            assert struct.unpack_from("<I", raw, pos + 25960 - 4)[0] == 25960
            pos += 25960
            assert struct.unpack_from("<I", raw, pos + 332)[0] == 336
            pos += 336
            voices.append(slot)
    assert pos == len(raw) - 8
    return dict(
        shape=[struct.unpack_from("<I",b,0)[0] for b, in bodies],relief=relief,float_events=float_events,narrators=narrators,loom=loom,drift=drift,floaters=floats,riggers=riggers,flute_words=flute_words,bank=bank,listeners=listeners,bank_query=bank_query,outlay=outlay,economies=economies,treads=treads,practice=practice,rush=rush,fold_cells=fold_cells,fold_map=fold_map,reed_social=reed_social,students=students,contexts=contexts,attribution=attribution,strands=strands,wicks=wicks,parcels=parcels,machinery=machinery,inventors=inventors,circuits=circuits,seals=seals,
        limbs=[struct.unpack_from("<I",b,4)[0] for b, in bodies],heading=[b[32] for b, in bodies],pockets=pockets,rangers=rangers,marks=marks,sightings=sightings,recess=recess,place_words=place_words,
        workshop=workshop,potters=potters,ware=ware,cargo=cargo,clay_pos=clay_pos,raw=raw,callers=callers,ports=ports,echo=echo,native_words=native_words,reed_pos=reed_pos,echo_pos=echo_pos,audience=audience,reed=reed,
        film_pos=film_pos,
        side=side,
        tick=tick,
        ages=ages,
        energy=energy,
        slots=slots,
        field=field,
        workers=workers,
        forecast=forecast,
        mind=mind,
        film=film,
        bound=bound,
        voices=voices,
        account=account,
        salts=salts,learners=learners,species=species,flow=flow,stock=stock,salt_pos=salt_pos,soil=soil,
    )


def receiver(row, index, gate):
    return (gate >> ((row >> (2 * index)) & 3)) & 1


def remember(row, index, pulse, law):
    shift = 2 * index
    prior = (row >> shift) & 3
    after = (law >> (8 * pulse + 2 * prior)) & 3
    return (row & ~(3 << shift)) | (after << shift)


def redeposit(marks):
    # Independently rotate the observations of each contact kind, keeping its encounter order.
    result = marks[:]
    for kind in (0, 1):
        indices = [i for i, m in enumerate(marks) if m & 128 and ((m >> 6) & 1) == kind]
        for i, index in enumerate(indices):
            other = indices[(i + 1) % len(indices)]
            result[index] = (marks[index] & ~32) | (marks[other] & 32)
    return result


def compare(state, result, seed):
    side = state["side"]
    ages = state["ages"]
    energy = state["energy"]
    donors = [
        (pos, s)
        for pos, s in enumerate(state["slots"])
        if energy[s] and int(result["min_age"]) <= ages[s] <= int(result["max_age"])
    ]
    assert len(donors) == int(result["donors"])
    pupils = int(result["pupils"])
    trials = int(result["trials"]) // pupils
    arm = result["field_transfer"]
    totals = [0] * 4
    for i in range(pupils):
        base = mix(seed + i * 5771)
        pos, slot = donors[mix(base + 717) % len(donors)]
        motor, innate, eye, learning, _, gate = struct.unpack_from("<6I", state["workers"][slot])
        glass = (eye >> 16) & 3
        odor = innate >> 30
        surface = (motor >> 26) & 3
        face, _, _, taste = state["forecast"][slot]
        soil = state["mind"][slot][2]
        if arm in ("unexperienced", "contacts_erased"):
            face = 0
        if arm in ("unexperienced", "soil_erased"):
            soil = 0
        if arm.startswith("ground_"):
            face = soil = 0
            for dy in range(4):
                for dx in range(4):
                    site = (pos % side + dx) % side + side * ((pos // side + dy) % side)
                    events = state["film"][site][0]
                    marks = [(events >> (8 * j)) & 255 for j in range(4)]
                    if arm == "ground_scrambled":
                        marks = redeposit(marks)
                    for m in reversed(marks):
                        if not m & 128:
                            continue
                        a = (m >> 2) & 3
                        b = m & 3
                        pulse = (m >> 5) & 1
                        if m & 64:
                            soil = remember(soil, 4 * (a ^ surface) + (b ^ odor), pulse, learning)
                        else:
                            face = remember(face, 4 * (a ^ glass) + (b ^ glass), pulse, learning)
                        totals[3] += 1
        for j in range(trials):
            k = mix(base + 2137 * j + 193)
            order = mix(k + 357)
            tested = {}
            for step in range(16):
                q = ((order >> 8) + step * (1 + 2 * (order & 7))) & 15
                a, b = divmod(q, 4)
                passes = bool(a and b and (a == 3 or b == 3 or a != b))
                tested[passes] = receiver(face, 4 * (a ^ glass) + (b ^ glass), gate)
            totals[0] += ((k >> 21) & 1) if tested[True] == tested[False] else tested[True]
            catalyst = (k >> 17) & 3
            productive = (k >> 8) & 1
            labels = [catalyst if bed == productive else catalyst ^ (1 + mix(k + 339) % 3) for bed in range(2)]
            responses = (
                [receiver(taste, label ^ odor, gate) for label in labels]
                if arm == "odor_only"
                else [receiver(soil, 4 * (catalyst ^ surface) + (label ^ odor), gate) for label in labels]
            )
            choice = ((k >> 12) & 1) if responses[0] == responses[1] else responses[1]
            totals[1] += choice == productive
            totals[2] += choice == productive and responses[0] != responses[1]
    assert totals == [int(result[key]) for key in ("contact_choices", "soil_choices", "unique_soils", "traces")], (
        arm,
        totals,
        result,
    )
    assert int(result["untouched"]) == int(result["insulated"]) == 0


def parse(text, key):
    return [
        dict(w.split("=", 1) for w in s.split() if "=" in w) for s in text.splitlines() if s.startswith(key + "=")
    ]


def test(tmp):
    def run(*args, stdin=None, ok=True):
        env = dict(os.environ, AXION_JOURNAL_DIR=str(tmp / "journal"))
        result = subprocess.run(
            [EXE, *map(str, args)], input=stdin, text=True, capture_output=True, env=env, cwd=ROOT
        )
        assert (result.returncode == 0) == ok, (args, result.stderr, result.stdout[-700:])
        return result.stdout if ok else result.stderr

    def flood(*args, **kwargs):
        return run("floodplain", *args, **kwargs)

    a = snapshot(BANK)
    assert a["bound"] > 0
    scores = lambda slot: (
        (a["mind"][slot][3] & 65535).bit_count(),
        (a["forecast"][slot][3] >> 16).bit_count(),
        a["ages"][slot],
        -slot,
    )
    ordered = sorted((i for i, e in enumerate(a["energy"]) if e), key=scores, reverse=True)
    assert len(ordered) == 1105 and ordered[0] == 1608 and scores(1608)[:3] == (16, 16, 483)
    perfect = 0
    for slot in ordered:
        motor, innate, _, _, _, gate = struct.unpack_from("<6I", a["workers"][slot])
        perfect += all(
            receiver(a["mind"][slot][2], 4 * (c ^ (motor >> 26 & 3)) + (s ^ (innate >> 30)), gate) == (c == s)
            for c in range(4)
            for s in range(4)
        )
    assert perfect == 8
    full = tmp / "full.flood"
    mid = tmp / "mid.flood"
    replay = tmp / "replay.flood"
    result = flood(
        "--load",
        BANK,
        "--steps",
        0,
        "--field-transfer",
        11,
        "--work-subjects",
        128,
        "--work-test-seed",
        319,
        "--field-min-age",
        40,
        "--assay-inquiry",
        "--hydraulic-trial",
        16,
        "--material-legacy",
        16,
        "--legacy",
        8,
        "--worksite-bench",
        2048,
        "--save",
        full,
    )
    assert full.read_bytes() == a["raw"]
    arms = parse(result, "field_transfer")
    assert len(arms) == 8
    for record in arms:
        compare(a, record, 319)
    image = parse(result, "worksite_deployment")[0]
    assert int(image["comparisons"]) == 68 * len(ordered) and image["differences"] == "0"
    assert (
        parse(
            flood("--load", BANK, "--steps", 0, "--field-transfer", 3, "--field-min-age", 100000), "field_transfer"
        )[0]["donors"]
        == "0"
    )
    for before, count, end in ((BANK, 37, full), (BANK, 11, mid), (mid, 26, replay)):
        flood("--load", before, "--steps", count, "--save", end)
    newer = snapshot(full)
    assert newer["raw"] == replay.read_bytes() and newer["account"][0] > a["account"][0]
    flood("--side", 64, "--soil", BANK, "--worksites", "quiet", "--wash-sites", "--steps", 0, "--save", mid)
    # Soil grafts must have matching extents. A fresh module can also be enabled on a historic watershed.
    washed = snapshot(mid)
    assert not any(row[0] for row in washed["film"])
    assert all(not row[2] for row in washed["mind"])
    tamper = bytearray(newer["raw"][:-8])
    struct.pack_into("<I", tamper, newer["film_pos"] + 4, newer["tick"] + 100)
    mid.write_bytes(tamper + checksum(tamper))
    assert "later tide" in flood("--load", mid, "--steps", 0, ok=False)
    path = tmp / "speaker.mind"
    flood("--resident", SPEAKER, "--side", 16, "--steps", 1, "--save", mid)
    assert snapshot(mid)["voices"] == [0]
    flood("--load", mid, "--steps", 0, "--slot", 0, "--export-mind", path)
    script = "/assay-table 4001\nwhich soil feeds hare?\nwhich soil resists hare?\n/listen-field foil\nwhich soil feeds hare?\n/listen-field quiet\nwhich soil feeds hare?\n/field-memory blank\nwhich soil feeds hare?\n/quit\n"
    for retained in (SPEAKER, path):
        r = parse(run("talk", "--speaker", retained, "--listener", PEER, stdin=script), "field_answer")
        assert [(x["field_answer"], x["receiver_site"], x["receiver_food"]) for x in r] == [
            ("newt", "6", "2"),
            ("vole", "9", "0"),
            ("newt", "9", "0"),
            ("newt", "32", "0"),
            ("_", "32", "0"),
        ], r
    flood("--load", mid, "--steps", 4096, "--save", replay)
    assert snapshot(replay)["voices"] == []
    print(
        "PASS independent inventory, energy and carbon ledgers; eight field interventions; deployment; restart; soil history; lifetime acoustic advice"
    )


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-worksite-test-"
    ) as tmp:
        test(Path(tmp))
