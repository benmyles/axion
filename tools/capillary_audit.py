#!/usr/bin/env python3
"""Independent checks of archived GPU observations. This module reads receipts and never trains an animal."""

import argparse
import csv
import gzip
from functools import lru_cache
from itertools import groupby
from pathlib import Path

FULL = (1 << 64) - 1
WEST = 0xFEFEFEFEFEFEFEFE


def read(path):
    opener = gzip.open if Path(path).suffix == ".gz" else open
    with opener(path, "rt") as f:
        return [
            {k: int(v) for k, v in row.items()}
            for row in csv.DictReader(f, delimiter="\t")
        ]


def set_bits(v):
    while v:
        b = v & -v
        yield b.bit_length() - 1
        v -= b


def local(row, rock):
    left, up, wick, wet, sill, film = ((row >> j) & 1 for j in range(6))
    head = left * (1 + bool(rock & 4)) + up * (1 + bool(rock & 8))
    adhesive = film if rock & 16 else wick
    return int(
        wet
        and head > 0
        and head + adhesive * (1 + bool(rock & 1)) >= 1 + sill * (1 + bool(rock & 2))
    )


@lru_cache(8192)
def infer(on, off, gene, lookup):
    senses = 63 & ~(gene & (gene >> 6) & (gene >> 12) & (gene >> 18))
    conflicts = on & off
    on, off = on & ~conflicts, off & ~conflicts
    if lookup or conflicts or not (on | off):
        return on, off, 0, 0
    best = []
    degree = 7
    for support in range(64):
        n = support.bit_count()
        if support & ~senses or n > min(degree, 3 + ((gene >> 26) & 3)):
            continue
        yes = {x & support for x in set_bits(on)}
        no = {x & support for x in set_bits(off)}
        if yes & no:
            continue
        if n < degree:
            best = []
            degree = n
        best.append((support, yes, no))
    if not best:
        return on, off, 0, 0
    certain = [0, 0]
    dispute = 0
    for x in range(64):
        votes = [int(x & s in a) - int(x & s in b) for s, a, b in best]
        if min(votes) == 1:
            certain[0] |= 1 << x
        if max(votes) == -1:
            certain[1] |= 1 << x
        if -1 in votes and 1 in votes:
            dispute |= 1 << x
    return *certain, sum(1 << s for s, _, _ in best), dispute


@lru_cache(32768)
def contextual_infer(on, off, gene, passive, receipts):
    observed_on = on & ~off & passive
    observed_off = off & ~on & passive
    _, _, allowed, dispute = infer(observed_on, observed_off, gene, False)
    if not dispute or not (receipts & 128):
        return infer(on, off, gene, False)
    models = {}
    for support in set_bits(allowed):
        yes = {v & support for v in set_bits(observed_on)}
        no = {v & support for v in set_bits(observed_off)}
        models[support] = (
            {v for v in range(64) if v & support in yes},
            {v for v in range(64) if v & support in no},
        )
    remaining = set(models)
    for i in range(1 + (gene >> 20) % 3):
        observation = (receipts >> (8 * i)) & 255
        if not observation & 128:
            break
        stimulus = observation & 63
        positive = (observation >> 6) & 1
        expected = {m for m in remaining if stimulus in models[m][1 - positive]}
        contrary = {m for m in remaining if stimulus in models[m][positive]}
        if expected:
            remaining -= contrary
    if remaining == set(models):
        return infer(on, off, gene, False)
    yes = set.intersection(*(models[m][0] for m in remaining))
    no = set.intersection(*(models[m][1] for m in remaining))
    disputed = set.union(*(models[m][0] for m in remaining)) & set.union(
        *(models[m][1] for m in remaining)
    )
    encode = lambda bits: sum(1 << s for s in bits)
    return encode(yes), encode(no), encode(remaining), encode(disputed)


def pegs(r):
    return [
        ((r["tools"] >> (8 * j)) & 63, (r["tools"] >> (8 * j + 6)) & 3)
        for j in range(6)
    ]


def layout(r, mask):
    layers = [r[k] for k in ("wick", "wet", "sill", "coating")]
    tools = pegs(r)
    for j in set_bits(mask):
        at, part = tools[j]
        layers[part] ^= 1 << at
    return layers


def valves(layers, truth):
    # Collect the sites which respond to each of four incoming pressures.
    maps = [0] * 4
    for material in range(16):
        group = FULL
        for kind, layer in enumerate(layers):
            group &= layer if material & (1 << kind) else ~layer
        for inputs in range(4):
            if truth & (1 << (material * 4 + inputs)):
                maps[inputs] |= group
    return maps


def flow(layers, truth, feed, depth):
    c0, c1, c2, c3 = valves(layers, truth)
    for _ in range(depth):
        a = (feed << 1) & WEST
        b = (feed << 8) & FULL
        feed = (~a & ~b & c0) | (a & ~b & c1) | (~a & b & c2) | (a & b & c3)
    return feed


def estimate(layers, yes, no, r):
    # Enumerate precisely the inlet combinations possible between the lower and upper bounds.
    fixed = [valves(layers, p) for p in (yes, no)]
    low = high = r["feed"]
    for _ in range(r["depth"]):
        al = (low << 1) & WEST
        ah = (high << 1) & WEST
        bl = (low << 8) & FULL
        bh = (high << 8) & FULL
        candidates = (~al & ~bl, ah & ~bl, ~al & bh, ah & bh)
        bounds = []
        for maps in fixed:
            acc = FULL
            for maybe, plane in zip(candidates, maps):
                acc &= ~maybe | plane
            bounds.append(acc)
        low, high = bounds[0], FULL ^ bounds[1]
    if r["catcher"] & ~high or r["waste"] & low:
        return 0
    if r["catcher"] & ~low or r["waste"] & high:
        return 2
    return 1


def execute(r, commands):
    stock = [(r.get("supply", 0x66666) >> (4 * j)) & 15 for j in range(5)]
    base = layout(r, 0)
    tools = pegs(r)
    rejected = moved = position = 0
    for j in commands:
        at, kind = tools[j]
        mask = 1 << at
        if base[kind] & mask:
            stock[kind] += 1
        elif stock[kind]:
            stock[kind] -= 1
        else:
            rejected += 1
            continue
        base[kind] ^= mask
        position ^= 1 << j
        moved += 1
    return position, sum(v << (4 * k) for k, v in enumerate(stock)), moved, rejected


def audit_repairs(rows, arm="active", verify_bounds=True):
    summary = dict(cases=len(rows), solvable=0, earned=0, forecasts=0)
    for r in rows:
        if r.get("context_mode"):
            yes, no, supports, rivalry = contextual_infer(
                r["on"],
                r["off"],
                r["dendrites"],
                r["context_passive"],
                r["context_receipts"],
            )
        else:
            yes, no, supports, rivalry = infer(
                r["on"], r["off"], r["dendrites"], arm == "lookup"
            )
        assert (yes, no, supports, rivalry) == tuple(
            r[k] for k in ("model_on", "model_off", "supports", "rivalry")
        )
        law = sum(local(x, r["rock"]) << x for x in range(64))
        baseline = layout(r, 0)
        correct = feasible = 0
        minimum = 7
        for mask in range(64):
            layers = layout(r, mask)
            can = all(
                new.bit_count()
                <= old.bit_count() + ((r.get("supply", 0x66666) >> (4 * j)) & 15)
                for j, (new, old) in enumerate(zip(layers, baseline))
            )
            if can:
                feasible |= 1 << mask
            pressure = flow(layers, law, r["feed"], r["depth"])
            wins = bool(not (r["catcher"] & ~pressure) and not (r["waste"] & pressure))
            correct |= int(wins) << mask
            if can and wins:
                minimum = min(minimum, mask.bit_count())
            if verify_bounds:
                if (yes | no) == FULL:
                    im = (
                        flow(layers, yes, r["feed"], r["depth"])
                        if yes != law
                        else pressure
                    )
                    pred = int(not (r["catcher"] & ~im) and not (r["waste"] & im))
                else:
                    pred = estimate(layers, yes, no, r)
                expected = (
                    (r["predictions"] >> mask) & 1 if r["known"] & (1 << mask) else 2
                )
                assert pred == expected, (r["pair"], r["rock"], mask, pred, expected)
                summary["forecasts"] += 1
        assert (r["plans"], r["shortest"]) == (correct, minimum)
        if "order" in r:
            assert feasible == r["feasible"]
            commands = [(r["order"] >> (3 * k)) & 7 for k in range(r["strokes"])]
            assert (
                len(set(commands)) == len(commands)
                and sum(1 << j for j in commands) == r["action"]
            )
            if arm == "backwards":
                commands.reverse()
            if arm == "miswired":
                commands = [(j + 1) % 6 for j in commands]
            receipt = execute(r, commands)
            assert receipt == tuple(
                r[k] for k in ("final", "remaining", "moved", "denied")
            )
            finished = receipt[0]
        else:
            finished = r["action"]
        assert r["caught"] == int(r["ready"] and bool(correct & (1 << finished)))
        summary["solvable"] += minimum <= r["moves"]
        summary["earned"] += r["caught"]
    return summary


def audit_events(rows):
    for r in rows:
        assert local(r["actual"], r["rock"]) == r["fired"]
        assert r["energy_after"] == r["energy_before"] + r["food"] - r["heat"]
        assert r["after_food"] == r["food_before"] - r["food"]
        assert sorted([r["incoming"], r["pad"]]) == sorted(
            [r["outgoing"], r["coating"]]
        )
        assert r["outgoing"] == (r["pad"] if r["fired"] else r["incoming"])
        if r["food"]:
            assert r["fired"] == r.get("wants_flow", 1) and r["cast"]
        if r["hand"]:
            assert (
                r["edge_before"] + r["cargo_before"]
                == r["edge_after"] + r["cargo_after"]
            )
            assert abs(r["edge_before"] - r["edge_after"]) == 1
        else:
            assert r["previous"] == r["actual"]
        gene = r["dendrites"]
        sensory = r["actual"] & (
            63 & ~(gene & (gene >> 6) & (gene >> 12) & (gene >> 18))
        )
        assert sensory == r["sensed"]
        changed = (r["before_on"] ^ r["on"]) | (r["before_off"] ^ r["off"])
        assert not (changed & ~(1 << sensory))
    return dict(
        observations=len(rows),
        moves=sum(bool(r["hand"]) for r in rows),
        food=sum(r["food"] for r in rows),
    )


def audit_wakes(rows):
    out = dict(origins=0, enabled=0, suppressed=0, other_food=0, prevented=0)
    for _, cone in groupby(rows, key=lambda r: (r["source_epoch"], r["origin"])):
        cone = list(cone)
        source = cone[0]
        out["origins"] += 1
        assert local(source["material"], source["rock"]) != local(
            source["physical"], source["rock"]
        )
        changed = {0}
        for depth, frame in groupby(cone[1:], key=lambda r: r["depth"]):
            new = set()
            for r in frame:
                x = r["delta_x"]
                alt = r["material"]
                if not r["ebb"]:
                    if x - 1 in changed and not r["spring"]:
                        alt ^= 1
                    if x in changed:
                        alt ^= 2
                assert local(r["material"], r["rock"]) == r["natural"]
                if local(alt, r["rock"]) == r["natural"]:
                    continue
                new.add(x)
                out["enabled" if r["natural"] else "suppressed"] += 1
                if (r["slot"], r["born"]) != (source["slot"], source["born"]):
                    if r["natural"] != r.get("return_net", 0):
                        out["other_food"] += r["food"]
                    else:
                        out["prevented"] += r["possible"]
            changed = new
    return out


def audit_speech(rows, wrong_name=False, dialect=False):
    for r in rows:
        assert local(r["row"], r["rock"]) == r["prior"]
        levers = [(r["peg_materials"] >> (3 * j)) & 7 for j in range(6)]
        assert sorted(levers) == list(range(6))

        def outcome(mask):
            address = r["row"]
            for j in set_bits(mask):
                address ^= 1 << ((levers[j] + 2) % 6)
            return local(address, r["rock"])

        assert outcome(r["change"]) == r["after"]
        viable = [
            m for m in range(64) if not (m & ~r["change"]) and outcome(m) == r["after"]
        ]
        least = [m for m in viable if not any(s != m and not (s & ~m) for s in viable)]
        cause = least[0] if len(least) == 1 and r["prior"] != r["after"] else 0
        assert cause == r["true_cause"]
        named = sum(1 << j for j, k in enumerate(levers) if k == r["named"])
        alone = (r["change"] & named) or named
        pigment = lambda fired: 4 + (r["pad"] if fired else r["pigment"])
        atom = lambda peg: levers[peg] if levers[peg] < 4 else levers[peg] + 6
        parts = [atom(j) for j in set_bits(cause)]
        kind = r["question"]
        message = []
        if (
            kind == 0
            and not wrong_name
            and r["after"]
            and r["pigment"] != r["pad"]
            and len(parts) == 1
        ):
            message = (
                [pigment(1), 27, parts[0]]
                if dialect
                else [14, parts[0], 25, pigment(1), 26]
            )
        if kind == 1:
            message = [8 + int(pigment(outcome(alone)) == r["requested"])]
        if kind == 2:
            message = [pigment(outcome(r["change"] & ~named))]
        if kind == 3:
            message = [pigment(r["prior"])]
        if kind == 4:
            message = [pigment(outcome(alone))]
        if kind == 5 and len(parts) == 2:
            message = [parts[0], 24, parts[1]]
        encoded = (len(message) << 56) | sum(
            (word + 1) << (6 * j) for j, word in enumerate(message)
        )
        assert r["wanted"] == encoded, (r["layout"], kind, encoded, r["wanted"])
    return dict(
        cases=len(rows),
        audible=sum(bool(r["said"]) for r in rows),
        exact=sum(bool(r["said"]) and r["said"] == r["wanted"] for r in rows),
    )


def audit_orders(rows, boards):
    result = dict(cases=len(rows), completed=0, novel=0)
    for r, w in zip(rows, boards, strict=True):
        assert r["pair"] == w["pair"] and r["twin"] == w["rock"] // 16
        trail = r["decoded"]
        actions = []
        current = 0
        denied = 0
        for off in range((trail >> 56) - 1, -1, -1):
            v = (trail >> (8 * off)) & 255
            j = (v >> 6) * 3 + ((v >> 4) & 3)
            at, part = pegs(w)[j]
            upper = bool(layout(w, current)[part] & (1 << at))
            if part != v & 3 or int(upper) != (v >> 2) & 3:
                denied += 1
                continue
            actions.append(j)
            current, _, _, blocked = execute(w, actions)
        current, _, _, blocked = execute(w, actions)
        assert (r["final"], r["denied"], r["earned"], r["solvable"]) == (
            current,
            blocked + denied,
            (w["plans"] >> current) & 1,
            int(w["shortest"] <= w["moves"]),
        )
        result["completed"] += r["earned"]
        result["novel"] += (
            r["steps"] > 2 and r["earned"] and r["decoded"] == r["intended"]
        )
    return result


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("kind", choices=["repairs", "events", "wakes", "speech", "orders"])
    p.add_argument("receipt", type=Path)
    p.add_argument("--wrong-name", action="store_true")
    p.add_argument("--other-dialect", action="store_true")
    p.add_argument("--arm", default="active")
    p.add_argument("--boards", type=Path)
    a = p.parse_args()
    rows = read(a.receipt)
    if a.kind == "repairs":
        print(audit_repairs(rows, a.arm))
    elif a.kind == "orders":
        print(audit_orders(rows, read(a.boards)))
    elif a.kind == "speech":
        print(audit_speech(rows, a.wrong_name, a.other_dialect))
    else:
        print({"events": audit_events, "wakes": audit_wakes}[a.kind](rows))
