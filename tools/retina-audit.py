#!/usr/bin/env python3
"""Reconstruct synaptic changes from raw pigment exposures and routes from independently computed beds."""

import argparse
import csv
import gzip
import importlib.util
import itertools
import math
import json
from collections import Counter
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    "drift_audit", Path(__file__).with_name("drift-audit.py")
)
drift = importlib.util.module_from_spec(spec)
spec.loader.exec_module(drift)
mix = drift.unsigned_mix
pixel = drift.pigment


addresses = drift.retinal_addresses
receptor = drift.retinal_receptors
glass = drift.private_image


def nerve(rule, earlier, later, old, spike):
    admitted = bool(
        (earlier or later) and (rule >> (16 + 2 * bool(earlier) + bool(later))) & 1
    )
    if not admitted:
        return old, spike
    next_state = firing = prior = 0
    for i, j in itertools.product(range(4), repeat=2):
        at = 4 * i + j
        pre = (earlier >> i) & 1
        post = (later >> j) & 1
        local = (rule >> (pre + 2 * post + 4 * ((spike >> at) & 1))) & 1
        grown = (rule >> (8 + ((old >> at) & 1) + 2 * pre + 4 * local)) & 1
        prior |= pre << at
        firing |= local << at
        next_state |= grown << at
    return next_state, (prior << 16) | firing


def load_surfaces(path):
    if not path:
        return []
    with path.open() as f:
        if f.readline().strip() != "AXION-WATERCOURSES-1":
            raise ValueError("unknown watercourse file")
        surfaces = []
        for line in f:
            values = list(map(int, line.split()))
            if len(values) != 20:
                raise ValueError("incomplete natural bed")
            seed, side, tile, tick, law, period, phase, rock = values[:8]
            x = (2 * (tile % (side // 2)) + ((tick >> 3) & 1)) % side
            y = (2 * (tile // (side // 2)) + ((tick >> 4) & 1)) % side
            expected = mix(seed ^ mix((x >> 3) + 19391 * (y >> 3)))
            if (
                rock != expected
                or phase != ((rock >> 13) + ((tick >> period) if law & 2 else 0)) % 4
            ):
                raise ValueError("unreproduced bedrock")
            r = dict(ports=law, phase=phase, rock=rock)
            for i, k in enumerate(("water", "clay", "fibre")):
                r[k] = sum(
                    v << (16 * j) for j, v in enumerate(values[8 + 4 * i : 12 + 4 * i])
                )
            surfaces.append(drift.winding_path(r, bool(law & 64)))
    return surfaces


def source_information(surfaces):
    """I(inlet; displayed outlet | known physical layer), enumerating all four inlets."""
    totals = Counter()
    for lower, upper, turns, _ in surfaces:
        for layer in [lower, upper] if turns else [lower]:
            endpoints = [drift.exit_from(i, layer) for i in range(4)]
            totals["layers"] += 1
            for mode in ("live", "other", "independent"):
                events = Counter(
                    (source, endpoints[shown])
                    for source in range(4)
                    for shown in range(4)
                    if mode == "independent" or (source == shown) == (mode == "live")
                )
                total = sum(events.values())
                displayed = Counter()
                for (_, outlet), freq in events.items():
                    displayed[outlet] += freq
                for (source, outlet), freq in events.items():
                    p = freq / total
                    totals[mode + "_bits"] += p * math.log2(
                        4 * freq / displayed[outlet]
                    )
                    totals[mode + "_matching_outlets"] += p * (
                        outlet == endpoints[source]
                    )
    layers = totals.pop("layers")
    return {"layers": layers, **{k: v / layers for k, v in totals.items()}}


def site_index(seed, life, context, n):
    return mix(seed ^ mix((life // 4) * 9739 + (context // 2) * 15263 + 8387)) % n


def chamber(seed, life, context, kind, surfaces=None):
    if surfaces:
        lower, upper, _, _ = surfaces[site_index(seed, life, context, len(surfaces))]
        k = mix(
            seed
            ^ mix(0xC781493D + (life // 4) * 0x9E3779B9)
            ^ ((0xABE17705 * context) & 0xFFFFFFFF)
        )
        return (
            k,
            0,
            [drift.exit_from(j, upper if context & 1 else lower) for j in range(4)],
        )
    key, shape, _, _, _, table = drift.bench_geometry(
        seed ^ (0xABE17705 * context & 0xFFFFFFFF), life, 65536, kind
    )
    return key, shape, table


def exit_port(ch, src, twin):
    key, shape, table = ch
    held = (key >> 12) & 3
    other = (held + 1 + mix(key + 891) % 3) % 4
    return drift.twin_exit(table, shape, src, twin, held, other)


def room_order(key, index, beds):
    if beds == 1:
        return 0
    if index < beds:
        return (
            drift.coupling(mix(key + 5987) % (2 if beds == 2 else 24))[index]
            + (key >> 8)
        ) % beds
    return mix(key + 19759 * index) % beds


def color(lamps, which, row):
    raw = lamps[which]
    return pixel(row["hues"], raw) if row.get("private_lamps", 0) else raw


def itinerary(
    seed, life, beds, laps, twin, kind, surfaces=None, woven=False, balanced=False
):
    chambers = [chamber(seed, life, j, kind, surfaces) for j in range(beds)]
    key = chambers[0][0]
    if woven:
        _, _, turns, length = surfaces[site_index(seed, life, 0, len(surfaces))]
        if length != laps:
            raise ValueError("unreproduced natural winding length")
        route = [(turns >> j) & 1 for j in range(laps)]
    else:
        route = [room_order(key, j, beds) for j in range(laps)]
    if twin and beds > 1:
        route.reverse()
    source = ((key >> 12) + (life if surfaces and balanced else 0)) & 3
    actual = source
    for j, which in enumerate(route):
        actual = exit_port(chambers[which], actual, beds == 1 and twin)
        if not j:
            first = actual
    return chambers, source, first, actual, route


def activated(banks, onset, cues, r):
    for step, cue in enumerate(cues):
        if step:
            onset = addresses(onset, (r["receptors"] >> 24) & 7)
        which = (
            0
            if r.get("beds", 1) == 1 or r.get("fused", 0)
            else (r["plasticity"] >> (20 + 2 * cue)) & 3
        )
        options = 0
        for j in range(4):
            if onset & (1 << j):
                options |= drift.outlet_options(
                    banks[which], j, r["wiring"], r["ports"]
                )
        onset = options
    return onset


def aperture(row, onset, contacts, spike, lamp):
    prediction = 0
    for source in range(4):
        if onset & (1 << source):
            prediction |= drift.outlet_options(
                contacts, source, row["wiring"], row["ports"]
            )
    motor, opened = iris_output(row, onset, prediction, spike, lamp)
    return motor, opened, prediction


def iris_output(row, onset, prediction, spike, lamp):
    def union(spikes):
        return (spikes | spikes >> 4 | spikes >> 8 | spikes >> 12) & 15

    rot = lambda v, n: drift.rotate(v, n, 4)
    inputs = [
        onset,
        prediction,
        rot(prediction, 1),
        rot(prediction, 2),
        rot(prediction, 3),
        union(spike & 65535),
        1 << lamp,
        union(spike >> 16),
    ]
    first = drift.circuit(row["dendrite"], inputs) & 15
    iris = row["iris_motor"]
    inputs[7] = first
    inputs[3] = rot(first, (iris >> 21) & 3)
    motor = drift.circuit(iris, inputs) & 15
    return motor, motor.bit_count() >= 1 + ((iris >> 17) & 3)


def purchases(r, quartets, initial_banks, totals):
    banks = initial_banks.copy()
    spike = r["initial_spikes"]
    heat = 0
    paired = initial_banks.copy()
    earlier = spike
    separate_heat = 0
    for rotation, samples in quartets:
        matching = []
        if r["gaze"] in (2, 5):
            for pre, post, lamp, site, _ in samples:
                _, chose, _ = aperture(r, pre, paired[site], earlier, lamp)
                take = chose and separate_heat < r["purse"]
                matching.append(take)
                if take and r["channel"]:
                    paired[site], earlier = nerve(
                        r["plasticity"], pre, post, paired[site], earlier
                    )
                separate_heat += take
        for step, (pre, post, lamp, site, receipt) in enumerate(samples):
            before = initial_banks[site] if r["gaze"] == 6 else banks[site]
            soma = r["initial_spikes"] if r["gaze"] == 6 else spike
            motor, chosen, prediction = aperture(r, pre, before, soma, lamp)
            if r["gaze"] == 2:
                chosen = matching[(rotation + step) % len(samples)]
            elif r["gaze"] == 5:
                peers = [i for i, s in enumerate(samples) if bool(s[0]) == bool(pre)]
                chosen = matching[peers[(peers.index(step) + rotation) % len(peers)]]
            elif r["gaze"] in (3, 4):
                chosen = r["gaze"] == 3
            take = chosen and heat < r["purse"]
            if motor != ((receipt >> 27) & 15) or take != bool(receipt & (1 << 26)):
                raise ValueError("unreproduced pre-outcome fixation")
            if totals is not None:
                totals.update(
                    offered=1,
                    heat=take,
                    onset_visible=bool(pre),
                    bought_visible=bool(pre) and take,
                    once_determined=prediction.bit_count() == 1,
                    bought_determined=prediction.bit_count() == 1 and take,
                    multiple=prediction.bit_count() > 1,
                    bought_multiple=prediction.bit_count() > 1 and take,
                )
            if take and r["channel"]:
                banks[site], spike = nerve(
                    r["plasticity"], pre, post, banks[site], spike
                )
            heat += take
    if heat != r["heat"] or (r["gaze"] in (2, 5) and heat != separate_heat):
        raise ValueError("unbalanced observation accounts")
    return banks, spike


def stimulus(row, ch, step, reflected, other=False):
    r = row
    mark = mix(ch[0] + (step // 4) * 1919 + 263)
    order = drift.coupling(mark % 24)
    slot = step % 4
    if other:
        slot = (slot + 1 + mix(mark ^ 0x297B79) % 3) % 4
    src = order[slot]
    sampled = order[(slot + 1 + (mark >> 7) % 3) % 4] if r["channel"] == 2 else src
    if r["channel"] == 4:
        sampled = drift.coupling(mix(mark ^ 0xBC459E21) % 24)[slot]
    dst = exit_port(ch, sampled, reflected)
    dye = (mark >> 18) & 3
    raw = (dye ^ 3) * 85 if r["contrast"] else mix(mark + 531) & 255
    rest = glass(raw, r["places"], r["hues"])
    on = glass(drift.set_pigment(raw, src, dye), r["places"], r["hues"])
    if r["stimulus"]:
        on = rest ^ (3 << (2 * drift.physical(r["senses"], src)))
    end = (
        rest
        if r["channel"] == 3
        else glass(drift.set_pigment(raw, dst, dye), r["places"], r["hues"])
    )
    pre = receptor(rest, on, r["receptors"], r["receptors"] >> 4, r["heading"])
    post = receptor(rest, end, r["receptors"] >> 2, r["receptors"] >> 8, r["heading"])
    return rest, on, end, pre, post


def arm_command(r, inputs, contacts, spike, lamp, tie):
    eyes = [aperture(r, event[3], contacts, spike, lamp) for event in inputs]
    strength = [s[0].bit_count() for s in eyes]
    arm = int(strength[1] > strength[0]) if strength[0] != strength[1] else tie & 1
    return arm, eyes[arm][0], any(s[1] for s in eyes)


def see(r, surfaces=None, totals=None):
    life, seed, hips, twin = r["life"], r["seed"], r.get("beds", 1), r["twin"]
    chambers, source, first, truth, route = itinerary(
        seed,
        life,
        hips,
        r["laps"],
        twin,
        r["landscape"],
        surfaces,
        r.get("woven_order", 0),
        r.get("balanced_sources", 0),
    )
    key = chambers[0][0]
    lamps = drift.coupling(mix(key + 91993) % 24)
    bank0 = [
        r["initial"],
        r.get("initial_a", 0) & 65535,
        r.get("initial_a", 0) >> 16,
        r.get("initial_b", 0) & 65535,
    ]
    banks = bank0.copy()
    spike = r["initial_spikes"]
    words = [int(x) for x in r["glints"].split(",") if x]
    before = r["former"]
    quartets = []
    intact = bank0.copy()
    starting_spike = spike
    heat = 0
    if len(words) != before + r["glimpses"]:
        raise ValueError("missing sensory exposure")
    for k, triplet in enumerate(words):
        past = k < before
        step = k if past else k - before
        context = (step // 4) % hips
        ch = (
            chamber(seed ^ 0x183911B3, life & ~3, context, r["landscape"], surfaces)
            if past
            else chambers[context]
        )
        reflected = hips == 1 and not past and twin
        options = [stimulus(r, ch, step, reflected)]
        if r.get("probes", 0):
            options.append(stimulus(r, ch, step, reflected, True))
        hand = triplet >> 31
        if hand >= len(options):
            raise ValueError("unavailable tracer source")
        rest, on, end, pre, post = options[hand]
        lamp = color(lamps, context, r)
        if (triplet & 0xFFFFFF) != (rest << 16 | on << 8 | end) or (
            "beds" in r and (triplet >> 24) & 3 != lamp
        ):
            raise ValueError(
                f"unreproduced retinal episode at life {life}, glimpse {k}"
            )
        bank = (
            0
            if hips == 1 or r.get("fused", 0)
            else (r["plasticity"] >> (20 + 2 * lamp)) & 3
        )
        if r.get("probes", 0):
            tie = mix(ch[0] ^ (step * 15791 + 17041))
            chosen, fires, starts = arm_command(
                r, options, intact[bank], starting_spike, lamp, tie
            )
            taken = bool(starts and heat < r["purse"])
            mode = r["probes"]
            expected = chosen
            visible = bool(options[0][3]) == bool(options[1][3])
            if mode == 2:
                expected ^= 1
            if mode == 3 or (mode == 4 and visible):
                expected = (tie >> 7) & 1
            if (
                expected != hand
                or fires != (triplet >> 27) & 15
                or taken != bool(triplet & (1 << 26))
            ):
                raise ValueError("unreproduced tracer choice before the release")
            if taken and r["channel"]:
                _, _, _, pre0, post0 = options[chosen]
                intact[bank], starting_spike = nerve(
                    r["plasticity"], pre0, post0, intact[bank], starting_spike
                )
                banks[bank], spike = nerve(
                    r["plasticity"], pre, post, banks[bank], spike
                )
            heat += taken
            if totals is not None:
                totals.update(
                    offered=1,
                    heat=taken,
                    right_hand=taken and chosen == 1,
                    switched=taken and chosen != hand,
                    matched_onset=bool(visible),
                    same_onset_switched=taken and chosen != hand and visible,
                    distinct_returns=taken and options[0][2] != options[1][2],
                )
        elif r.get("gaze", 0):
            if step % 4 == 0:
                rotation = mix(ch[0] ^ mix(step + int(not past) * 193 + 0x48E3))
                quartets.append((rotation, []))
            quartets[-1][1].append((pre, post, lamp, bank, triplet))
        elif r["channel"]:
            banks[bank], spike = nerve(r["plasticity"], pre, post, banks[bank], spike)
    if r.get("probes", 0) and heat != r["heat"]:
        raise ValueError("unbalanced tracer work")
    if quartets:
        banks, spike = purchases(r, quartets, bank0, totals)
    stored = [
        r["contacts"],
        r.get("a", 0) & 65535,
        r.get("a", 0) >> 16,
        r.get("b", 0) & 65535,
    ]
    if banks != stored or spike != r["spikes"]:
        raise ValueError("unreproduced contact or spiking state")
    cues = [color(lamps, c, r) for c in route]
    if (
        "schedule" in r
        and sum(c << (2 * i) for i, c in enumerate(cues)) != r["schedule"]
    ):
        raise ValueError("inconsistent cue train")
    dye = (key >> 22) & 3
    raw = (dye ^ 3) * 85
    base = glass(raw, r["places"], r["hues"])
    opening = glass(drift.set_pigment(raw, source, dye), r["places"], r["hues"])
    if r["stimulus"]:
        opening = base ^ (3 << (2 * drift.physical(r["senses"], source)))
    firing = receptor(base, opening, r["receptors"], r["receptors"] >> 4, r["heading"])
    opts = activated(banks, firing, cues, r)
    draw = mix(key ^ 0x597A174B)

    def motor(opts):
        return drift.physical(r["muscles"], drift.expected_command(opts, draw))

    expected = (
        source,
        first,
        truth,
        base,
        opening,
        firing,
        opts,
        motor(opts),
        motor(activated(bank0, firing, cues, r)),
        motor(activated(banks, firing, cues[:1], r)),
        draw,
    )
    if expected != tuple(
        r[k]
        for k in (
            "source",
            "first_exit",
            "target",
            "rest",
            "onset",
            "firing",
            "options",
            "command",
            "bare",
            "single",
            "draw",
        )
    ):
        raise ValueError("unreproduced forecast, baseline or physical terminal")
    if "feeds" in r:
        captured = int(r["command"] == truth)
        for j in range(1, r["feeds"]):
            signal = mix(key ^ (j * 29569 + 0x651701))
            port = (source + j) & 3
            hue = (signal >> 18) & 3
            base0 = glass((hue ^ 3) * 85, r["places"], r["hues"])
            on = glass(
                drift.set_pigment((hue ^ 3) * 85, port, hue), r["places"], r["hues"]
            )
            if r["stimulus"]:
                on = base0 ^ (3 << (2 * drift.physical(r["senses"], port)))
            pre = receptor(base0, on, r["receptors"], r["receptors"] >> 4, r["heading"])
            order = (
                route
                if r.get("woven_order")
                else [room_order(signal, k, hips) for k in range(r["laps"])]
            )
            if not r.get("woven_order") and hips > 1 and twin:
                order.reverse()
            waves = [color(lamps, k, r) for k in order]
            possible = activated(banks, pre, waves, r)
            muscle = drift.physical(
                r["muscles"], drift.expected_command(possible, mix(signal + 9131))
            )
            for chamber_index in order:
                port = exit_port(chambers[chamber_index], port, hips == 1 and twin)
            captured += muscle == port
        if captured != r["caught"]:
            raise ValueError(
                "secondary food catches differ from the deposited portions"
            )
        if totals is not None:
            totals.update(portions=r["feeds"], caught=captured)
    return r["command"] == truth


def read_glosses(path, names):
    if not path:
        return None
    colors = names.read_text().splitlines()[0].split()[4:8]
    glosses = []
    for line in path.read_text().splitlines():
        if not line.strip():
            continue
        name, *units = line.split()
        if not 1 <= len(units) <= 4 or len(glosses) >= 4:
            raise ValueError("invalid external speech sequence")
        glosses.append([colors.index(w) for w in units])
    return glosses


def spoken_units(cues, glosses):
    units = []
    i = 0
    while i < len(cues):
        matches = [
            (len(seq), -j)
            for j, seq in enumerate(glosses)
            if cues[i : i + len(seq)] == seq
        ]
        if matches:
            n, negative = max(matches)
            units.append(4 - negative)
            i += n
        else:
            units.append(cues[i])
            i += 1
    return units


def audit_words(path):
    totals = Counter()
    with (gzip.open if path.suffix == ".gz" else open)(path, "rt") as f:
        for line in f:
            if not line.startswith("temporal_word="):
                continue
            r = dict(part.split("=", 1) for part in line.split())
            old, spikes, first, second, background, mode, n, law = (
                int(r[k])
                for k in (
                    "old",
                    "initial_spikes",
                    "first",
                    "second",
                    "background",
                    "mode",
                    "length",
                    "rule",
                )
            )
            valid = (1 << (4 * n)) - 1
            common = valid
            for item in r["evidence"].split(","):
                packed, observed = map(int, item.split("/"))
                visible = packed & 65535
                common &= visible
                if visible & ~valid or any(
                    not (visible >> (4 * k) & 15) for k in range(n)
                ):
                    raise ValueError("incomplete visual event")
                conflict = valid ^ visible
                proposal = sum(
                    ((law >> (2 * ((old >> j) & 1) + ((conflict >> j) & 1))) & 1) << j
                    for j in range(4 * n)
                )
                choices = [(~old >> (4 * k)) & 15 for k in range(n)]
                predicted = all(m.bit_count() == 1 for m in choices)
                matches = all(
                    m & (visible >> (4 * k)) & 15 for k, m in enumerate(choices)
                )
                ports = [
                    old,
                    conflict,
                    proposal,
                    drift.rotate(spikes, first >> 17, 16),
                    65535 * bool(mode != 2 and predicted),
                    65535 * bool(mode != 2 and predicted and matches),
                    drift.rotate(background, first >> 21, 32),
                    drift.rotate(spikes >> 16, second >> 17, 16),
                ]
                local = drift.circuit(first, ports)
                ports[7] = local
                ports[3] = drift.rotate(spikes >> 16, second >> 21, 16)
                ports[6] = drift.rotate(background, second >> 25, 32)
                gate = drift.circuit(second, ports)
                spikes = (gate << 16) | local
                old = (old ^ ((old ^ proposal) & (gate if mode else 65535))) & valid
                if (old, spikes) != (packed >> 16, observed):
                    raise ValueError(
                        "word contacts or cortical spikes disagree with the visible evidence"
                    )
                totals.update(
                    exposures=1,
                    stages=n,
                    ambiguous=sum(
                        (visible >> (4 * j) & 15).bit_count() > 1 for j in range(n)
                    ),
                )
            unique = all(((~old >> (4 * k)) & 15).bit_count() == 1 for k in range(n))
            if bool(int(r["decoded"])) != unique:
                raise ValueError("incorrect temporal word readback")
            totals.update(
                words=1,
                decoded=unique,
                identifiable=all(
                    (common >> (4 * k) & 15).bit_count() == 1 for k in range(n)
                ),
            )
    if not totals["words"]:
        raise ValueError("no teaching observations")
    return totals


def audit_inquiry(p, chambers, source, totals):
    key = chambers[0][0]
    hues = drift.coupling(mix(key + 91993) % 24)
    bank0 = [p["hand"], p["a"] & 65535, p["a"] >> 16, p["b"] & 65535]
    banks = bank0.copy()
    reference = banks.copy()
    spike = guide = p["initial_spikes"]
    spent = looked = stopped = digest = actions = removed = deposited = 0
    moves = [(p["sounds"] >> (2 * j)) & 3 for j in range(p["heard"])]
    dye = (key >> 22) & 3
    raw = (dye ^ 3) * 85
    base = glass(raw, p["places"], p["hues"])
    on = glass(drift.set_pigment(raw, source, dye), p["places"], p["hues"])
    if p["stimulus"]:
        on = base ^ (3 << (2 * drift.physical(p["senses"], source)))
    onset = receptor(base, on, p["receptors"], p["receptors"] >> 4, p["heading"])

    def prediction(state):
        return activated(state, onset, moves, p) if moves else 0

    before = prediction(banks)
    budget = max(0, p["budget"] - p["prior_heat"])
    while moves and looked < p["inquiry"] and spent < budget:
        interest = moves[::-1] if p.get("query_context", 0) else moves
        control = activated(reference, onset, interest, p)
        vote, decision = iris_output(p, onset, control, guide, interest[0])
        if not decision:
            stopped = 1
            break
        step = p["start"] + looked
        context = (step // 4) % p["beds"]
        ch = chambers[context]
        tie = mix(ch[0] ^ (step * 15791 + 17041))
        lamp = pixel(p["hues"], hues[context])
        bank = (
            0
            if p["beds"] == 1 or p["fused"]
            else (p["plasticity"] >> (20 + 2 * lamp)) & 3
        )
        options = [stimulus(p, ch, step, False, other) for other in (False, True)]
        hand, motor, opened = arm_command(p, options, reference[bank], guide, lamp, tie)
        real = hand
        kind = p["query_hand"]
        if kind == 2:
            real ^= 1
        if kind == 3 or kind == 4 and bool(options[0][3]) == bool(options[1][3]):
            real = (tie >> 7) & 1
        if opened and p["channel"]:
            _, _, _, pre, post = options[hand]
            reference[bank], guide = nerve(
                p["plasticity"], pre, post, reference[bank], guide
            )
            _, _, _, pre, post = options[real]
            banks[bank], spike = nerve(p["plasticity"], pre, post, banks[bank], spike)
        rest, on, end, _, _ = options[real]
        image = (
            (real << 31)
            | (motor << 27)
            | (int(opened) << 26)
            | (lamp << 24)
            | (rest << 16)
            | (on << 8)
            | (end if opened else 0)
        )
        digest = mix(digest ^ image ^ vote ^ step)
        if opened:
            actions = mix(actions ^ ((2 * step + real) + 0x34D1))
            mark = mix(ch[0] + (step // 4) * 1919 + 263)
            slot = (step + 1 + mix(mark ^ 0x297B79) % 3) % 4 if real else step % 4
            origin = drift.coupling(mark % 24)[slot]
            destination = exit_port(ch, origin, False)
            removed += 1 << (8 * origin)
            deposited += 1 << (8 * destination)
        looked += 1
        spent += opened
    if "removed" in p and (p["removed"], p["deposited"]) != (removed, deposited):
        raise ValueError("the four tracer stocks differ from the physical releases")
    if "actions" in p and p["actions"] != actions:
        raise ValueError("unreproduced physical probes")
    stored = [
        p["final_hand"],
        p["final_a"] & 65535,
        p["final_a"] >> 16,
        p["final_b"] & 65535,
    ]
    if (before, prediction(banks), looked, spent, stopped, digest, spike, banks) != (
        p["prior_options"],
        p["options"],
        p["offers"],
        p["paid"],
        p["stopped"],
        p["digest"],
        p["final_spikes"],
        stored,
    ):
        raise ValueError(
            "unreproduced question-driven experiment or open-loop forecast"
        )
    prior = p["prior_word"] == p["actual"]
    now = p["said"] == p["actual"]
    totals.update(
        query_paid=spent,
        query_offers=looked,
        investigated=spent > 0,
        prior_correct=prior,
        improved=now and not prior,
        lost_correct=prior and not now,
        stopped=stopped,
    )


def public_cells(law, a, b, c):
    result = 0
    for j in range(4):
        address = ((a >> j) & 1) + 2 * ((b >> j) & 1) + 4 * ((c >> j) & 1)
        result |= ((law >> address) & 1) << j
    return result


def public_hand(write, motor, tape, face, inputs, predictions, tie):
    trace = (tape >> (4 * face)) & 15
    signals = []
    for onset, prediction in zip(inputs, predictions):
        local = public_cells(motor, onset, trace, prediction)
        signals.append(
            public_cells(motor >> 8, onset, local, addresses(trace, (write >> 11) & 7))
        )
    a, b = map(int.bit_count, signals)
    return (
        signals,
        (tie & 1 if a == b else int(b > a)),
        max(a, b) >= 1 + ((motor >> 16) & 3),
    )


def audit_grains(path):
    totals = Counter()
    with (gzip.open if path.suffix == ".gz" else open)(path, "rt") as f:
        for row in csv.DictReader(f, delimiter="\t"):
            r = {k: int(v) for k, v in row.items()}
            rest = glass(r["rest"], r["places"], r["hues"])
            inputs = []
            for source in (r["first"], r["other"]):
                color = glass(
                    drift.set_pigment(r["rest"], source, r["dye"]),
                    r["places"],
                    r["hues"],
                )
                onset = receptor(
                    rest, color, r["receptors"], r["receptors"] >> 4, r["heading"]
                )
                inputs.append((rest, color, None, onset, None))
            hand, _, opened = arm_command(
                r, inputs, r["old"], r["spike"], r["lamp"], r["tie"]
            )
            motors = [
                aperture(r, q[3], r["old"], r["spike"], r["lamp"])[0] for q in inputs
            ]
            if r.get("peer_visible"):
                shape = r["silhouette"]
                face = pixel(r["hues"], ((shape >> 11) & 1) | (2 * ((shape >> 7) & 1)))
                face ^= r.get("exchanged_face", 0)
                if face != r["addressed"]:
                    raise ValueError("the visible outline and addressed tissue differ")
                expected = [
                    aperture(r, q[3], r["old"], r["spike"], r["lamp"])[2]
                    for q in inputs
                ]
                motors, hand, opened = public_hand(
                    r["write_gene"],
                    r["motor_gene"],
                    r["public_tape"],
                    face,
                    [q[3] for q in inputs],
                    expected,
                    r["tie"],
                )
                totals["public_choices"] += 1
            physical = hand ^ r["crossed"]
            start = (r["first"], r["other"])[physical]
            dest = drift.exit_from(start, r["links"])
            blocked = dest != start and r["downstream"] == 65535
            moved = opened and not blocked and dest != start
            observed = (
                r["left"],
                r["right"],
                r["hand"],
                r["physical"],
                r["started"],
                r["destination"],
                r["blocked"],
                r["stock_after"],
                r["downstream_after"],
            )
            if (
                motors[0],
                motors[1],
                hand,
                physical,
                opened,
                dest,
                blocked,
                r["stock"] - moved,
                r["downstream"] + moved,
            ) != observed:
                raise ValueError(
                    "the motor discharge or the grain transfer disagrees with its recorded inputs"
                )
            totals.update(
                samples=1,
                initiated=opened,
                moved=moved,
                declined=not opened,
                blocked=blocked and opened,
            )
    return totals


def audit(path, voices=False, surfaces=None, glosses=None):
    totals = Counter()
    with (gzip.open if path.suffix == ".gz" else open)(path, "rt") as stream:
        data = (
            {k: (v if k == "glints" else int(v)) for k, v in x.items()}
            for x in csv.DictReader(stream, delimiter="\t")
        )
        for life, group in itertools.groupby(data, lambda r: r["life"]):
            cohort = list(group)
            for r in cohort:
                if r.get("actual_beds", 0) != len(surfaces or ()):
                    raise ValueError("select the matching physical collection")
                if surfaces and r.get("surface") != site_index(
                    r["seed"], life, 0, len(surfaces)
                ):
                    raise ValueError("a different wild site was substituted")
            if not voices:
                if [r["twin"] for r in cohort] != [0, 1]:
                    raise ValueError("incomplete paired questions")
                ok = [see(r, surfaces, totals) for r in cohort]
                for r, right in zip(cohort, ok):
                    totals.update(
                        courses=1,
                        correct=right,
                        blank=r["bare"] == r["target"],
                        one_step=r["single"] == r["target"],
                        different_horizon=r["first_exit"] != r["target"],
                        horizon_right=right and r["first_exit"] != r["target"],
                    )
                if cohort[0]["target"] != cohort[1]["target"]:
                    totals.update(different_worlds=1, pair_right=all(ok))
                if cohort[0].get("beds", 1) > 1 and any(
                    cohort[0][k] != cohort[1][k]
                    for k in ("contacts", "spikes", "glints", "a", "b", "onset")
                ):
                    raise ValueError(
                        "changing the order of the query changed the previous experience"
                    )
                continue
            if [(r["twin"], r["turn"]) for r in cohort] != [
                (0, 0),
                (0, 1),
                (1, 0),
                (1, 1),
            ]:
                raise ValueError("missing spoken branch")
            for p, q in (cohort[:2], cohort[2:]):
                if any(
                    p[k] != q[k]
                    for k in p.keys()
                    - {"turn", "actual", "reported", "origin", "cleared"}
                ):
                    raise ValueError("a later diversion changed an earlier reply")
                chambers, src, _, target, route = itinerary(
                    p["seed"],
                    life,
                    p["beds"],
                    p["laps"],
                    p["twin"],
                    p["landscape"],
                    surfaces,
                    p.get("woven_order", 0),
                    p.get("balanced_sources", 0),
                )
                if (p["initial_row"], p["actual"], q["actual"]) != (
                    pixel(p["places"], src),
                    pixel(p["places"], target),
                    pixel(p["places"], (target + 1) % 4),
                ):
                    raise ValueError(
                        "misreported physical release in a spoken examination"
                    )
                if p.get("inquiry"):
                    audit_inquiry(p, chambers, src, totals)
                correct = p["said"] == p["actual"]
                both = all(x["actual"] == x["reported"] for x in (p, q))
                totals.update(
                    courses=1,
                    heard=p["heard"] == p["laps"],
                    voiced=p["said"] < 8,
                    correct=correct,
                    premature=p["premature"] < 8,
                    no_scene=p["no_scene"] < 8,
                    both_releases=both,
                    revised=correct and q["reported"] == q["actual"],
                    motor=p["motor"] == p["actual"],
                )
                if "learned_units" in p:
                    count = p["learned_units"]
                    units = p["sound_units"]
                    if glosses is not None:
                        key = chambers[0][0]
                        lamps = drift.coupling(mix(key + 91993) % 24)
                        public = [pixel(p["hues"], lamps[k]) for k in route]
                        words = spoken_units(public, glosses)
                        if (len(words), sum(w >= 4 for w in words)) != (units, count):
                            raise ValueError(
                                "public word count disagrees with the speaker's physical sequence"
                            )
                    totals.update(
                        sound_units=units,
                        physical_steps=p["laps"],
                        shortened=count > 0,
                        shortened_right=count > 0 and correct,
                        combined=count >= 2,
                        combined_right=count >= 2 and correct,
                    )
                for x in (p, q):
                    totals.update(
                        releases=1,
                        outcomes=x["actual"] == x["reported"],
                        origins=x["origin"] == p["initial_row"],
                        erased_topic=x["cleared"] < 8,
                    )
            a, b = cohort[0], cohort[2]
            if a["beds"] > 1:
                if any(a[k] != b[k] for k in ("hand", "a", "b", "initial_row")):
                    raise ValueError("education changed between causal questions")
                if a["actual"] != b["actual"]:
                    totals.update(
                        opposite_orders=1,
                        both_orders=a["said"] == a["actual"]
                        and b["said"] == b["actual"],
                    )
    if not totals["courses"]:
        raise ValueError("empty study")
    return totals


def exchange_questions(heard_path, reversed_path):
    """Check the two branches used the same physical work on exchanged questions."""

    def trajectories(path):
        with (gzip.open if path.suffix == ".gz" else open)(path, "rt") as f:
            rows = (
                {k: int(v) for k, v in rec.items()}
                for rec in csv.DictReader(f, delimiter="\t")
                if rec["turn"] == "0"
            )
            for (seed, life), group in itertools.groupby(
                rows, key=lambda r: (r["seed"], r["life"])
            ):
                twins = list(group)
                if [q["twin"] for q in twins] != [0, 1]:
                    raise ValueError("missing question twin")
                yield seed, life, twins

    counts = Counter()
    for (seed, i, normal), (s, j, exchanged) in zip(
        trajectories(heard_path), trajectories(reversed_path), strict=True
    ):
        if (seed, i) != (s, j):
            raise ValueError("questions from different layouts")
        for twin, q in enumerate(normal):
            v = exchanged[twin]
            opposite = normal[1 - twin]
            if q["query_context"] != 0 or v["query_context"] != 1:
                raise ValueError("select heard then reversed working context")
            if any(
                q[k] != v[k]
                for k in (
                    "hand",
                    "a",
                    "b",
                    "initial_spikes",
                    "sounds",
                    "heard",
                    "actual",
                    "prior_word",
                    "budget",
                )
            ):
                raise ValueError(
                    "the control changed earlier experience, the asked question or the physical end"
                )
            if any(
                opposite[k] != v[k]
                for k in (
                    "paid",
                    "offers",
                    "actions",
                    "final_spikes",
                    "final_hand",
                    "final_a",
                    "final_b",
                    "stopped",
                )
            ):
                raise ValueError(
                    "exchanging the question failed to exchange the physical acquisition history"
                )
            hit = q["said"] == q["actual"]
            other = v["said"] == v["actual"]
            counts.update(
                courses=1,
                per_arm_grains=q["paid"],
                different_actions=q["actions"] != v["actions"],
                heard_correct=hit,
                reversed_correct=other,
                helped=hit and not other,
                hurt=other and not hit,
            )
    if not counts["courses"]:
        raise ValueError("empty query exchange")
    return counts


if __name__ == "__main__":
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument("file", type=Path)
    args.add_argument("--voices", action="store_true")
    args.add_argument("--works", type=Path)
    args.add_argument(
        "--paired-inquiry",
        type=Path,
        help="audit a second panel with the working question reversed",
    )
    args.add_argument(
        "--lesson",
        action="store_true",
        help="replay sensory evidence and the inherited lexical gate",
    )
    args.add_argument("--terms", type=Path)
    args.add_argument(
        "--questions",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "examples/inlet-questions.txt",
    )
    args.add_argument(
        "--source-information",
        action="store_true",
        help="enumerate the information in the three source treatments on captured beds",
    )
    args.add_argument(
        "--grains",
        action="store_true",
        help="reconstruct the muscles and conserved deposits of a living watershed",
    )
    parsed = args.parse_args()
    if parsed.paired_inquiry:
        surfaces = load_surfaces(parsed.works)
        for p in (parsed.file, parsed.paired_inquiry):
            audit(p, True, surfaces, read_glosses(parsed.terms, parsed.questions))
        print(
            json.dumps(
                exchange_questions(parsed.file, parsed.paired_inquiry),
                indent=2,
                sort_keys=True,
            )
        )
        raise SystemExit
    if parsed.grains:
        print(json.dumps(audit_grains(parsed.file), indent=2, sort_keys=True))
        raise SystemExit
    if parsed.source_information:
        print(
            json.dumps(
                source_information(load_surfaces(parsed.file)), indent=2, sort_keys=True
            )
        )
        raise SystemExit
    result = (
        audit_words(parsed.file)
        if parsed.lesson
        else audit(
            parsed.file,
            parsed.voices,
            load_surfaces(parsed.works),
            read_glosses(parsed.terms, parsed.questions),
        )
    )
    print(json.dumps(result, indent=2, sort_keys=True))
