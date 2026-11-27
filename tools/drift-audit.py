#!/usr/bin/env python3
"""Independently replay recorded currents, competing reaches and local synaptic writes."""

import argparse
import csv
import gzip
import itertools
import json
from collections import Counter
from pathlib import Path


def rotate(value, shift, width):
    mask = (1 << width) - 1
    shift %= width
    return ((value << shift) | (value & mask) >> (width - shift)) & mask


def circuit(word, signals):
    indices = [word >> k & 7 for k in (0, 3, 6)]
    result = 0
    for bit in range(16):
        address = sum(((signals[k] >> bit) & 1) << j for j, k in enumerate(indices))
        result |= ((word >> (9 + address)) & 1) << bit
    return result


def contact(row, bite):
    old = row["old"]
    base = 4 * row["row"]
    cmd = row["command"]
    if not row["learns"]:
        return old, row["spikes"]
    group = 15 << base
    touch = 1 << (base + cmd)
    conflict = group ^ touch if bite else touch
    proposed = old
    for b in range(base, base + 4):
        lut_index = 2 * (old >> b & 1) + (conflict >> b & 1)
        proposed = (proposed & ~(1 << b)) | (((row["law"] >> lut_index) & 1) << b)
    high, low = row["spikes"] >> 16, row["spikes"] & 65535
    a, b = row["first"], row["second"]
    guessed = row.get("openings", outlet_options(old, row["row"], row.get("wiring", 0), issued_ports(row))).bit_count() == 1 and bool(row["ports"] & 16)
    ports = [
        old,
        conflict,
        proposed,
        rotate(low, a >> 17, 16),
        65535 * guessed,
        65535 * bool(guessed and bite),
        rotate(row["background"], a >> 21, 32),
        rotate(high, b >> 17, 16),
    ]
    front = circuit(a, ports)
    ports[3] = rotate(high, b >> 21, 16)
    ports[6] = rotate(row["background"], b >> 25, 32)
    ports[7] = front
    permission = circuit(b, ports)
    if row["ports"] & 4096:
        table = (a >> 26) | ((b >> 30) << 6)
        for i in range(base, base + 4):
            choice = (old >> i & 1) + 2 * (proposed >> i & 1) + 4 * (front >> i & 1)
            if (table >> choice) & 1:
                proposed ^= 1 << i
    admitted = permission & group
    return (old & ~admitted) | (proposed & admitted), front | (permission << 16)


def unpack(word):
    return [(word >> (16 * j)) & 65535 for j in range(4)]


def outlet_options(contacts, row, gene, ports):
    primary = (~contacts >> (4 * row)) & 15
    if not ports & 16384:
        return primary
    distal = 0
    for bit in range(4):
        address = sum(
            ((contacts >> (4 * ((row + k + 1) % 4) + bit)) & 1) << k for k in range(3)
        )
        distal |= ((gene >> address) & 1) << bit
    if ports & 131072 and (gene >> (12 + distal.bit_count())) & 1:
        distal ^= 15
    if ports & 32768:
        distal = rotate(distal, 1, 4)
    output = 0
    for bit in range(4):
        blocked = (contacts >> (4 * row + bit)) & 1
        address = (blocked << 1) | ((distal >> bit) & 1)
        output |= ((1 ^ blocked) ^ ((gene >> (8 + address)) & 1)) << bit
    return output or primary


def issued_ports(row):
    flags = row["ports"]
    tag = row.get("tag", 0)
    if flags & 262144 and tag and tag != (2 if flags & 256 else 1):
        flags &= ~(16384 | 32768 | 131072)
    return flags


def links(row, fibres):
    water, clay, fibre = map(lambda k: unpack(row[k]), ("water", "clay", "fibre"))
    active = 0
    for e in range(4):
        high = row["phase"] in ((1, 2) if e & 1 else (0, 1))
        head = 1 + 6 * high + min(abs(water[e] - water[(e + 1) % 4]) // 4, 4)
        barrier = 3 + ((row["rock"] >> (2 * e)) & 3) + min(clay[e], 16)
        wick = 2 * min(fibre[e], 4) if fibres else 0
        if head + wick >= barrier:
            active |= 1 << e
            a, b = water[e], water[(e+1) % 4]
            if row.get("ports", 0) & 65536 and (clay[e] or wick) and abs(a-b) > 2 + ((row["rock"] >> (2*e)) & 3):
                active |= 1 << (e + (4 if a < b else 8))
    return active


def exit_from(inlet, seams):
    place = inlet
    for seam in (0, 2, 1, 3):
        other = (seam + 1) % 4
        if seams & (1 << seam) and place in (seam, other):
            blocked = seams & (1 << (seam + (4 if place == seam else 8)))
            if not blocked:
                place = other if place == seam else seam
    return place


def expected_command(available, draw):
    ports = [j for j in range(4) if available & (1 << j)] or list(range(4))
    return ports[(draw * len(ports)) >> 32]


def physical(axes, cmd):
    return ((axes & 3) + (-cmd if axes & 4 else cmd)) % 4


def winding_path(r, conduct):
    floor=links(r,conduct);capillary=turns=0;length=1
    if not conduct or not r.get("ports",0)&(1<<30):return floor,capillary,turns,length
    water,clay,fibre=[unpack(r[k]) for k in ("water","clay","fibre")]
    seats=[]
    for e in range(4):
        a,b=water[e],water[(e+1)%4];notch=(r["rock"]>>(2*e))&3
        head=min(a,b,48)//8+min(fibre[e],8)
        if fibre[e] and head>=2+notch+min(clay[e],16)//2:
            capillary|=1<<e
            if r["ports"]&65536 and abs(a-b)>8+2*min(fibre[e],8):capillary|=1<<(e+(4 if a<b else 8))
        if fibre[e] and clay[e] and fibre[e]+min(clay[e],4)>=3+notch:seats.append(e)
    for j in range(4):
        post=((r["rock"]>>17)+(-j if r["phase"]&2 else j))%4
        if post in seats:
            turns|=((post+(r["rock"]>>21))&1)<<length;length+=1
    return floor,capillary,turns,length


def through_windings(src,path,reverse=False):
    floor,upper,turns,laps=path
    for j in (reversed(range(laps)) if reverse else range(laps)):
        src=exit_from(src,upper if turns&(1<<j) else floor)
    return src


def retinal_addresses(pattern, frame):
    return sum(1<<physical(frame,i) for i in range(4) if pattern&(1<<i))


def retinal_receptors(base, light, spectrum, address, heading):
    firing={i for i in range(4) if (pigment(base,i)^pigment(light,i))&(spectrum&3)}
    if address&8:firing={(i-heading)%4 for i in firing}
    return sum(1<<physical(address,i) for i in firing)


def private_image(board,places,hues):
    return sum(pigment(hues,pigment(board,j))<<(2*pigment(places,j)) for j in range(4))


def current_choices(r):
    ports=issued_ports(r)
    if not r.get("optical",0):
        return outlet_options(r["old"],r["row"],r.get("wiring",0),ports)
    places,hues=r["optics"]&255,r["optics"]>>8
    rest=private_image(r["resting"],places,hues)
    mark=private_image(set_pigment(r["resting"],r["inlet"],r["dye"]),places,hues)
    gene=r["receptors"];x=retinal_receptors(rest,mark,gene,gene>>4,r["heading"])
    banks=[r["hand"],r["gate_a"]&65535,r["gate_a"]>>16,r["gate_b"]&65535]
    for stage in range(r["laps"]):
        if stage:x=retinal_addresses(x,(gene>>24)&7)
        pigment_code=(r["phase"]+(r["rock"]>>17)+2*((r["turns"]>>stage)&1))%4
        private=pigment(hues,pigment_code)
        which=(r["visual_law"]>>(20+2*private))&3 if r["ports"]&(1<<27) else 0
        if not stage and r["old"]!=banks[which]:raise ValueError("the tactile contact was attributed to another bank")
        next_state=0
        for j in range(4):
            if x&(1<<j):next_state|=outlet_options(banks[which],j,r["wiring"],ports)
        x=next_state
    return x


def check_trace(path):
    totals = Counter()
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt") as file:
        rows = (
            {k: int(v) for k, v in r.items()}
            for r in csv.DictReader(file, delimiter="\t")
        )
        for (stamp, tile), group in itertools.groupby(
            rows, lambda r: (r["stamp"], r["tile"])
        ):
            people = list(group)
            if {r["place"] for r in people} != set(range(4)) or len(people) != 4:
                raise ValueError(f"incomplete four-port observation at {stamp}/{tile}")
            main = people[0]
            enable = main["ports"]
            wet = links(main, bool(enable & 64))
            dry = links(main, False)
            winding=winding_path(main,bool(enable&64))
            route=through_windings(main["inlet"],winding)
            bare=through_windings(main["inlet"],winding_path(main,False))
            if enable&(1<<30):
                if winding[1:]!=(main["upper"],main["turns"],main["laps"]):raise ValueError("unreproduced material windings")
                totals["multi_stage"]+=winding[3]>1
                totals["order_sensitive"]+=route!=through_windings(main["inlet"],winding,True)
            if (wet, dry, route) != (main["links"], main["unbound"], main["outlet"]):
                raise ValueError(f"hydraulic circuit disagrees at {stamp}/{tile}")
            actual = blank = unbound = 0
            load = main["load"]
            if enable & 524288:
                clay, fibre, water = (unpack(main[k]) for k in ("clay","fibre","water"))
                sequence = [((main["rock"]>>24)+j)&3 for j in range(4)]
                heights = [8+((main["rock"]>>(4*j))&15)+min(clay[j]+clay[(j-1)%4],12)+(min(fibre[j]+fibre[(j-1)%4],8) if enable&64 else 0) for j in range(4)]
                rim=max(sequence,key=heights.__getitem__)
                phase=(((stamp-1+(main["rock"]>>7))&0xffffffff)>>max(0,main["tide_shift"]-5))&31
                tide=min(phase,32-phase) if enable&2 else 8
                pressure=min(water[rim],64)//4+tide+min(main["held_stock"],32)//2
                if (rim,heights[rim],pressure)!=(main["rim"],main["height"],main["pressure"]):
                    raise ValueError("unreproduced storage head")
                src=main["inlet"]
                if src==rim and (pressure<heights[rim] or main["source_stock"]!=main["held_stock"]):
                    raise ValueError("a closed sill released food")
                flux=1+2*(pressure-heights[rim])+min(main["source_stock"]//4,10) if src==rim else 1+int(water[src]>=8)
                amount=min(main["source_stock"],main["load_limit"],flux)
                if not main["same_stock"]:amount=min(amount,65535-main["dest_litter"])
                if amount!=load:raise ValueError("unreproduced storage discharge")
                if src==rim: totals["spills"]+=1
            for j in range(4):
                r = people[(j + main["priority"]) % 4]
                if not r["issued"]:
                    continue
                tag = r.get("tag", 0)
                reads = bool(enable & 4) and (
                    not enable & 128
                    or enable & (512 | 262144 | (1<<28))
                    or tag == 0
                    or tag == (2 if enable & 256 else 1)
                )
                if bool(r.get("uses", reads)) != bool(reads):
                    raise ValueError("incorrect motor access for inherited marker")
                vacant = current_choices(r)
                if vacant != r.get("openings", vacant):
                    raise ValueError("distal dendrites disagree with inherited gates")
                baseline = expected_command(15, r["draw"])
                cmd = expected_command(vacant if reads else 15, r["draw"])
                if (cmd, baseline) != (r["command"], r["blank"]):
                    raise ValueError("invalid outlet commitment")
                aim = physical(r["axes"], cmd)
                bite = min(r["capacity"], load - actual) if aim == route else 0
                next_state, spikes = contact(r, bite)
                if (bite, next_state, spikes) != (
                    r["bite"],
                    r["after"],
                    r["spike_after"],
                ):
                    raise ValueError(
                        f"unreproduced tactile update at {stamp}/{tile}/{j}"
                    )
                actual += bite
                if physical(r["axes"], baseline) == route:
                    blank += min(r["capacity"], load - blank)
                if aim == bare:
                    unbound += min(r["capacity"], load - unbound)
                totals["reaches"] += 1
                totals["changed_choices"] += cmd != baseline
            if (actual, blank, unbound) != (
                main["eaten"],
                main["blank_eaten"],
                main["unbound_eaten"],
            ):
                raise ValueError("finite intake disagrees with simultaneous controls")
            left = main["source_stock"] - load
            landed = main["dest_litter"] + load - actual
            if main["same_stock"]:
                left = landed = main["source_stock"] - actual
            if (left, landed) != (main["after_stock"], main["after_litter"]):
                raise ValueError("carbon mass discrepancy")
            totals.update(
                pulses=1,
                released=load,
                eaten=actual,
                settled=load - actual,
                blank=blank,
                bare=unbound,
            )
    return totals


def check_legacy(path):
    opener = gzip.open if path.suffix == ".gz" else open
    total = Counter()
    with opener(path, "rt") as file:
        for r in (
            {k: int(v) for k, v in m.items()}
            for m in csv.DictReader(file, delimiter="\t")
        ):
            side, origin, e = r["side"], r["origin"], r["seam"]
            x, y = origin % side, origin // side
            expected = (
                2 * (r["tile"] % (side // 2)) + (((r["stamp"] - 1) >> 3) & 1)
            ) % side
            yy = (2 * (r["tile"] // (side // 2)) + (((r["stamp"] - 1) >> 4) & 1)) % side
            if (x, y) != (expected, yy):
                raise ValueError("wrong catchment origin")
            roots = [
                origin,
                y * side + (x + 1) % side,
                ((y + 1) % side) * side + x,
                origin,
            ]
            rank = ((2 * roots[e] + (e & 1)) * 0x9E3779B1 + r["salt"]) % (
                2 * side * side
            )
            if rank != r["surface"] or not r["born"] <= r["laid"] < r["stamp"]:
                raise ValueError("invalid fibre provenance")
            if unpack(r["fibre"])[e] == 0:
                raise ValueError("empty marked seam")
            actual = through_windings(r["inlet"], winding_path(r, True))
            removed = dict(r)
            removed["fibre"] -= 1 << (16 * e)
            alternate = through_windings(r["inlet"], winding_path(removed, True))
            if (actual, alternate) != (
                r["outlet"],
                r["alternative"],
            ) or actual == alternate:
                raise ValueError("unconfirmed material dependence")
            eaten = without = 0
            beneficiary, highest = 0, 0
            for k in range(4):
                j = (k + r["priority"]) % 4
                aim = (r["aims"] >> (2 * j)) & 3
                cap = (r["capacities"] >> (5 * j)) & 31
                got = min(cap, r["load"] - eaten) if aim == actual else 0
                if got != ((r["meals"] >> (5 * j)) & 31):
                    raise ValueError("unconfirmed beneficiary")
                hypothetical = min(cap, r["load"] - without) if aim == alternate else 0
                eaten += got
                without += hypothetical
                different = (r[f"slot{j}"], r[f"birth{j}"]) != (r["maker"], r["born"])
                dead = not r["maker_energy"] or r["maker_now"] != r["born"]
                priority = (1 + different + (r[f"birth{j}"] > r["laid"])) if got else 0
                if priority > highest:
                    beneficiary, highest = j, priority
                if got and different:
                    total["foreign"] += got
                    if dead and r[f"birth{j}"] > r["laid"]:
                        total["younger_after_death"] += got
                        total["later_receivers"] += 1
            if "beneficiary" in r:
                if beneficiary != r["beneficiary"]:
                    raise ValueError("incorrect displayed beneficiary")
                for prefix in ("maker_", "") if highest else ("maker_",):
                    if (
                        r[prefix + "limbs"] | r[prefix + "eyes"] | r[prefix + "pores"]
                    ) & ~r[prefix + "shape"]:
                        raise ValueError(
                            "a displayed organ lies outside its owner's body"
                        )
                    if r[prefix + "cells"] not in range(64, 1025, 32):
                        raise ValueError("invalid expressed somatic cell count")
            total.update(examples=1, eaten=eaten, without=without)
    return total


def pigment(panel, socket):
    return (panel >> (2 * socket)) & 3


def set_pigment(panel, socket, hue):
    return (panel & ~(3 << (2 * socket))) | (hue << (2 * socket))


def switched(panel, a, b):
    return set_pigment(set_pigment(panel, a, pigment(panel, b)), b, pigment(panel, a))


def unsigned_mix(x):
    x &= 0xffffffff
    x = ((x ^ (x >> 16)) * 0x21f0aaad) & 0xffffffff
    x = ((x ^ (x >> 15)) * 0x735a2d97) & 0xffffffff
    return x ^ (x >> 15)


def coupling(index):
    left = [0, 1, 2, 3]
    names = []
    for size in (4, 3, 2, 1):
        names.append(left.pop(index % size))
        index //= size
    return names


def bench_geometry(seed, life, flags, kind):
    key = unsigned_mix(seed ^ unsigned_mix(0xC781493D + (life // 4) * 0x9E3779B9))
    if kind == 3:
        kind = 2 if ((life // 4) ^ seed) & 1 else 1
    rock, phase = unsigned_mix(key + 751), (key >> 7) & 3
    bed = dict(rock=rock, phase=phase, ports=65536 if kind or flags & 65536 else 0)
    for j in range(4):
        soil = unsigned_mix(key + 503 * j + 31)
        water, clay = 9 + (soil & 31), (soil >> 7) & 3
        fibre = 0 if soil & 32768 else (soil >> 17) & 3
        if kind == 1:
            water = 8 + (soil & 1)
        if kind == 2:
            bottom = (key >> 20) & 3
            distance = min((j - bottom) % 4, (bottom - j) % 4)
            water, clay, fibre = (
                4 + 14 * distance + (soil & 3),
                1 + (soil & 1),
                3 + (soil & 1),
            )
        for name, value in (("water", water), ("clay", clay), ("fibre", fibre)):
            bed[name] = bed.get(name, 0) | (value << (16 * j))
    wet = links(bed, True)
    pipes = coupling(unsigned_mix(key + 2139) % 24)
    outs = [(v + life) % 4 for v in coupling(unsigned_mix(key + 4919) % 24)]
    physical_outputs = [outs[exit_from(x, wet)] for x in pipes]
    if kind in (1, 2) and len(set(physical_outputs)) != (4 if kind == 1 else 1):
        raise ValueError("conditioned bed does not realise its physical geometry")
    return key, kind, wet, pipes, outs, physical_outputs


def twin_exit(mapping, regime, row, twin, hold, other):
    if twin and not regime:
        row = other if row == hold else hold if row == other else row
    return (mapping[row] + int(bool(regime and twin))) % 4


def practice_evidence(r, life, key, shape, truth_table, hold, other, include_old=False):
    flags, steps, prior = r["ports"], r["steps"], r.get("prechange", 0)
    contacts = r.get("initial_contacts", 0)
    incoming = contacts
    sampled = earlier_rows = swallowed = 0
    spikes = r.get("initial_spikes", 0)
    old_all = r.get("former_all", 0)
    earlier = (
        bench_geometry(
            r["seed"] ^ 0x9F9A13CB,
            life & ~3 if old_all else life,
            flags,
            {0: 0, 1: 2, 2: 1}[shape],
        )
        if prior
        else None
    )
    allowed = [15] * 4
    for trial in range(prior + steps):
        old = trial < prior
        salt, regime, mapping = (
            (earlier[0], earlier[1], earlier[5])
            if old
            else (key, shape, truth_table)
        )
        t = trial if old else trial - prior
        rotation = unsigned_mix(salt + (t // 3) * 193 + 3319) % 3
        row = (hold + 1 + (rotation + t % 3) % 3) % 4
        if old and old_all:
            row = (t + unsigned_mix(salt + 119 * (t // 4))) & 3
        if row == hold and not (old and old_all):
            raise ValueError("the held inlet entered practice")
        options = outlet_options(contacts, row, r["wiring"], flags)
        draw = unsigned_mix(salt + t * 0x895CD6CD + 7741)
        action = expected_command(options, draw)
        bite = action == twin_exit(mapping, regime, row, False if old and old_all else r["twin"], hold, other)
        swallowed += bite
        if old:
            earlier_rows |= 1 << row
        if not old or include_old:
            sampled |= 1 << row
        if not old:
            allowed[row] = (
                (1 << action) if bite else allowed[row] & ~(1 << action)
            )
        inp = dict(
            r,
            old=contacts,
            spikes=spikes,
            row=row,
            command=action,
            background=r.get("background", 0),
            learns=r["active"],
            ports=flags | 16,
        )
        contacts, spikes = contact(inp, bite)
        if old:
            incoming = contacts
    return contacts, spikes, sampled, swallowed, allowed, incoming, earlier_rows


def check_inlets(path):
    totals = Counter()
    stream = gzip.open if path.suffix == ".gz" else open
    permutations = list(itertools.permutations(range(4)))
    drains = [(i,) * 4 for i in range(4)]
    unrestricted = list(itertools.product(range(4), repeat=4))
    with stream(path, "rt") as f:
        data = (
            {k: int(v) for k, v in row.items()}
            for row in csv.DictReader(f, delimiter="\t")
        )
        for life, rows in itertools.groupby(data, lambda r: r["life"]):
            twins = list(rows)
            if [r["twin"] for r in twins] != [0, 1]:
                raise ValueError("missing member of a hydraulic twin pair")
            for r in twins:
                flags = r["ports"]
                kind = r.get("landscape", 0)
                key, shape, wet, pipes, outs, truth_table = bench_geometry(
                    r["seed"], life, flags, kind
                )
                hold = (key >> 12) & 3
                other = (hold + 1 + unsigned_mix(key + 891) % 3) % 4
                pack = lambda values: sum(z << (2 * i) for i, z in enumerate(values))
                if (wet, pack(pipes), pack(outs), hold, other, shape) != (
                    r["channels"],
                    r["pipes"],
                    r["outputs"],
                    r["hold"],
                    r["swap"],
                    r.get("shape", 0),
                ):
                    raise ValueError("hidden course does not reconstruct from its seed")

                contacts, spikes, sampled, swallowed, allowed, incoming, earlier_rows = practice_evidence(
                    r, life, key, shape, truth_table, hold, other, include_old="earlier_rows" not in r)
                draw = unsigned_mix(key ^ 0xDB713F81)
                options = outlet_options(contacts, hold, r["wiring"], flags)
                actual = twin_exit(truth_table, shape, hold, r["twin"], hold, other)
                reply = expected_command(options, draw)
                local = expected_command((~contacts >> (4 * hold)) & 15, draw)
                rotated = expected_command(
                    outlet_options(contacts, hold, r["wiring"], flags ^ 32768), draw
                )
                read = (
                    contacts,
                    spikes,
                    sampled,
                    swallowed,
                    draw,
                    options,
                    actual,
                    reply,
                    local,
                    rotated,
                )
                if read != tuple(
                    r[k]
                    for k in (
                        "contacts",
                        "spikes",
                        "sampled",
                        "hits",
                        "draw",
                        "options",
                        "target",
                        "answer",
                        "local",
                        "rotated",
                    )
                ):
                    raise ValueError(
                        f"unreproduced private evidence at case {life}/{r['twin']}"
                    )
                worlds = (
                    permutations + drains
                    if kind == 3
                    else drains
                    if shape == 2
                    else unrestricted
                    if flags & 65536 and not shape
                    else permutations
                )
                possible = {
                    m[hold]
                    for m in worlds
                    if all((allowed[j] >> m[j]) & 1 for j in range(4))
                }
                if (
                    actual not in possible
                    or (contacts ^ incoming) & (15 << (4 * hold))
                    or sampled & (1 << hold)
                ):
                    raise ValueError(
                        "test target is inconsistent with the tactile history"
                    )
                if (
                    r.get("incoming", incoming) != incoming
                    or r.get("earlier_rows", earlier_rows) != earlier_rows
                ):
                    raise ValueError("unconfirmed evidence before the physical change")
                older = (incoming >> (4 * hold)) & 15
                if older != 15 and older & (1 << actual):
                    totals["contradicted"] += 1
                    totals["recovered"] += reply == actual
                totals.update(
                    courses=1,
                    correct=reply == actual,
                    local=local == actual,
                    rotated=rotated == actual,
                    confident=options.bit_count() == 1,
                    ambiguous=len(possible) > 1,
                    uniquely_implied=len(possible) == 1,
                    correct_when_implied=len(possible) == 1 and reply == actual,
                    training_food=swallowed,
                    wrong_confident=options.bit_count() == 1 and reply != actual,
                )
            a, b = twins
            if a["target"] != b["target"]:
                totals["different_targets"] += 1
                totals["pairs_right"] += (
                    a["target"] == a["answer"] and b["target"] == b["answer"]
                )
    return totals


def check_inlet_voice(path):
    """Rebuild the public optical scene and both releases after each sealed reply."""
    totals = Counter()
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt") as source:
        rows = (
            {k: int(v) for k, v in row.items()}
            for row in csv.DictReader(source, delimiter="\t")
        )
        for life, group in itertools.groupby(rows, lambda r: r["life"]):
            four = list(group)
            if [(r["twin"], r["redirected"]) for r in four] != [
                (0, 0),
                (0, 1),
                (1, 0),
                (1, 1),
            ]:
                raise ValueError("incomplete paired prospective interview")
            for native, turned in (four[:2], four[2:]):
                independent = {
                    "redirected",
                    "seen",
                    "said_after",
                    "said_start",
                    "no_topic",
                }
                if any(native[k] != turned[k] for k in native.keys() - independent):
                    raise ValueError(
                        "an outlet change altered an earlier observation or utterance"
                    )
                r = native
                key, shape, _, _, _, route = bench_geometry(
                    r["seed"], life, r["ports"], r["landscape"]
                )
                held = (key >> 12) & 3
                other = (held + 1 + unsigned_mix(key + 891) % 3) % 4
                natural = twin_exit(route, shape, held, r["twin"], held, other)
                if "initial_contacts" in r:
                    contacts, spikes, sampled, _, _, incoming, earlier_rows = practice_evidence(
                        r, life, key, shape, route, held, other)
                    expected = (contacts, spikes, sampled, incoming, earlier_rows, outlet_options(contacts, held, r["wiring"], r["ports"]))
                    if expected != tuple(r[k] for k in ("contacts", "spikes", "sampled", "incoming", "earlier_rows", "options")):
                        raise ValueError("the speaker's sealed forecast cannot be recovered from its embodied practice")

                def seen(motor):
                    return pigment(r["places"], physical(r["muscles"], motor))

                senses = r["senses"]
                start = physical(senses, held) if senses & 4 else (held - senses) % 4
                if (
                    r["start"] != pigment(r["places"], start)
                    or pigment(r["hues"], r["dye"]) != (life + (key >> 5)) % 4
                ):
                    raise ValueError(
                        "speaker's inlet or dye disagrees with its inherited retinal wiring"
                    )
                if (
                    r["sampled"] & (1 << held)
                    or r["steps"] >= 3
                    and r["sampled"].bit_count() != 3
                ):
                    raise ValueError(
                        "the deferred question drew on a touch from the unopened inlet"
                    )
                draw = unsigned_mix(key ^ 0xDB713F81)
                if r["planned"] != seen(expected_command(r["options"], draw)) or r[
                    "local"
                ] != seen(expected_command((~r["contacts"] >> (4 * held)) & 15, draw)):
                    raise ValueError(
                        "the sealed motor choice disagrees with its contacts and draw"
                    )
                if (native["seen"], turned["seen"]) != (
                    seen(natural),
                    seen((natural + 1) % 4),
                ):
                    raise ValueError(
                        "post-commit releases disagree with the independently reconstructed watercourse"
                    )
                honest = native["said_before"] == native["seen"]
                both = all(x["said_after"] == x["seen"] for x in (native, turned))
                totals.update(
                    courses=1,
                    answered_before=r["said_before"] < 8,
                    foresaw=honest,
                    premature=r["premature"] < 8,
                    no_scene=r["no_scene"] < 8,
                    motor_correct=r["planned"] == r["seen"],
                    proximal_correct=r["local"] == r["seen"],
                    both_releases=both,
                    revised=honest and turned["said_after"] == turned["seen"],
                )
                for x in (native, turned):
                    said = x["said_after"] == x["seen"]
                    origin = x["said_start"] == r["start"]
                    totals.update(
                        releases=1,
                        actual=said,
                        origins=origin,
                        complete=honest and said and origin,
                        cleared_topic=x["no_topic"] < 8,
                    )
    if not totals["courses"]:
        raise ValueError("empty prospective interview")
    return totals


def check_diary(path):
    result = Counter()
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt") as file:
        rows = (
            {k: int(v) for k, v in r.items()}
            for r in csv.DictReader(file, delimiter="\t")
        )
        for stamp, records in itertools.groupby(rows, lambda r: r["stamp"]):
            data = list(records)
            if [r["site"] for r in data] != list(range(4)):
                raise ValueError("incomplete public socket questions")
            r = data[0]
            base, src, dst = r["background"], r["inlet"], r["sink"]
            if r["raft"]:
                start = base
                if "towed" in r:
                    moving = (
                        src != dst
                        and r["load"] + min(r["water"], 48) // 4
                        >= 1 + r["raft_load"] // 2
                    )
                    if int(moving) != r["towed"]:
                        raise ValueError("wrong passive carrier displacement")
                    end = switched(base, src, dst) if moving else base
                else:
                    end = switched(base, src, dst)
                forecast = lambda k: switched(base, src, physical(r["axes"], k))
            else:
                start = set_pigment(base, src, r["dye"])
                end = set_pigment(base, dst, r["dye"])
                forecast = lambda k: set_pigment(base, physical(r["axes"], k), r["dye"])
            midpoint = r.get("midpoint", 4)
            if midpoint < 4:
                if r["raft"]:
                    raise ValueError("a floating carrier acquired an unobserved intermediate position")
                panel = set_pigment(base, midpoint, r["dye"])
                for location in range(4):
                    socket = pigment(r["places"], location)
                    if not r["known"] & (1 << (12 + socket)) or pigment(r["images"] >> 24, socket) != pigment(r["hues"], pigment(panel, location)):
                        raise ValueError("midway retinal panel disagrees with the first crossing")
            hypotheses = [forecast(k) for k in range(4) if (1 << k) & r["options"]]
            for physical_port in range(4):
                site = pigment(r["places"], physical_port)
                for frame, panel in enumerate(
                    (start, end, hypotheses[0] if hypotheses else 0)
                ):
                    known = (
                        frame < 2
                        or bool(hypotheses)
                        and len({pigment(q, physical_port) for q in hypotheses}) == 1
                    )
                    observed = bool(r["known"] & (1 << (4 * frame + site)))
                    if known != observed:
                        raise ValueError(
                            "forecast certainty disagrees with actual synapses"
                        )
                    if not known:
                        continue
                    truth = pigment(r["hues"], pigment(panel, physical_port))
                    reading = pigment(r["images"] >> (8 * frame), site)
                    if truth != reading:
                        raise ValueError(
                            "retinal panel disagrees with observed or predicted current"
                        )
            result["events"] += 1
            result["eaten"] += r["bite"]
            if not r.get("examined", 1):
                if any(
                    row[k] != 0x8888
                    for row in data
                    for k in ("conversation", "no_topic", "no_event")
                ):
                    raise ValueError("an unasked question contains a reported answer")
                continue
            right_sites = 0
            for r in data:
                j = r["site"]
                answers = r["conversation"]
                expected = pigment(r["images"] >> 16, j) + 4
                actual = pigment(r["images"] >> 8, j) + 4
                before = pigment(r["images"], j) + 4
                known = bool(r["known"] & (1 << (8 + j)))
                a = known and answers & 15 == expected
                b = answers >> 4 & 15 == actual
                c = answers >> 8 & 15 == before
                result["anchors"] += 1
                result["expected_right"] += a
                result["actual_right"] += b
                result["before_right"] += c
                result["three_turns"] += a and b and c
                if b:
                    right_sites |= 1 << j
                if known and expected != actual:
                    result["discrepancies"] += 1
                    result["both_sides"] += a and b
                result["erased_topic_answers"] += (r["no_topic"] >> 4 & 15) < 8
                result["missing_scene_answers"] += sum(
                    (r["no_event"] >> (4 * i) & 15) < 8 for i in range(4)
                )
                unique = [s for s in range(4) if pigment(r["images"], s) == before - 4]
                if len(unique) == 1:
                    result["unique_replies"] += 1
                    result["locations_right"] += c and (answers >> 12 & 15) == unique[0]
            for j, k in itertools.combinations(range(4), 2):
                if pigment(r["images"] >> 8, j) != pigment(r["images"] >> 8, k):
                    result["contrasting_pairs"] += bool(right_sites & 1 << j) and bool(
                        right_sites & 1 << k
                    )
    return result


if __name__ == "__main__":
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument("trace", type=Path)
    args.add_argument("--legacy", action="store_true")
    args.add_argument("--diary", action="store_true")
    args.add_argument("--inlets", action="store_true")
    args.add_argument("--inlet-voice", action="store_true")
    options = args.parse_args()
    print(
        json.dumps(
            (
                check_inlet_voice
                if options.inlet_voice
                else check_inlets
                if options.inlets
                else check_legacy
                if options.legacy
                else check_diary
                if options.diary
                else check_trace
            )(options.trace),
            indent=2,
            sort_keys=True,
        )
    )
