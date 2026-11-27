#!/usr/bin/env python3
"""Independently reconstruct acquired context from pressure contacts and age-ordered, self-generated evidence."""

import argparse
from pathlib import Path
from capillary_audit import (
    FULL,
    audit_speech,
    contextual_infer,
    execute,
    flow,
    infer,
    layout,
    local,
    read,
    set_bits,
)


def agrees(on, off, law, senses):
    return all(
        ((on >> (i & senses)) & 1) == ((law >> i) & 1)
        and ((off >> (i & senses)) & 1) != (law >> i) & 1
        for i in range(64)
    )


def channel(r, law):
    substrate = {**r, "wet": r["water"], "coating": r["film"], "waste": r["spill"]}
    configuration = r["design"]
    bounds = lambda m: flow(layout(substrate, m), law, r["feed"], r["depth"])
    v = bounds(configuration)
    caught = int(
        r["ready"] and v & r["catcher"] == r["catcher"] and not (v & r["spill"])
    )
    assert r["caught"] == caught
    choices = [j for j in range(6) if configuration & (1 << j)]
    parts = [
        ((r["tools"] >> (8 * j)) & 63, (r["tools"] >> (8 * j + 6)) & 3)
        for j in range(6)
    ]
    bed = layout(substrate, 0)
    choices.sort(key=lambda j: not bed[parts[j][1]] & (1 << parts[j][0]))
    assert execute(substrate, choices)[3] == 0
    possible = 0
    for m in range(64):
        if m.bit_count() > r["moves"]:
            continue
        s = layout(substrate, m)
        if any(
            n.bit_count() > o.bit_count() + ((r["supply"] >> (4 * j)) & 15)
            for j, (n, o) in enumerate(zip(s, bed))
        ):
            continue
        v = bounds(m)
        if v & r["catcher"] == r["catcher"] and not v & r["spill"]:
            possible = 1
            break
    assert r["solvable"] == possible


def stream(rows, mode, gene, physical=True):
    tally = dict(
        episodes=0,
        switches=0,
        changed_before=0,
        changed_after=0,
        stable_after=0,
        explained=0,
        harvest=0,
        heat=0,
    )
    previous = {}
    for r in rows:
        mode = r.get("mode", mode)
        gene = r.get("dendrites", gene)
        assert mode is not None and gene is not None
        t = (r["on"], r["off"])
        model = (
            infer(*t, gene, False)
            if mode in (1, 2)
            else contextual_infer(*t, gene, r["passive"], r["receipts"])
        )
        assert model[:2] == (r["model_on"], r["model_off"]), (
            r["subject"],
            r["episode"],
            model[:2],
        )
        law = sum(local(j, r["rock"]) << j for j in range(64))
        assert law == r["law"]
        correct = agrees(*model[:2], law, r["senses"])
        assert correct == r["correct"]
        for prefix, grade in (
            ("prior", "before_correct"),
            ("observed", "watch_correct"),
        ):
            assert (
                agrees(r[prefix + "_on"], r[prefix + "_off"], law, r["senses"])
                == r[grade]
            )
        if r["episode"]:
            old = previous[r["subject"]]
            assert r["switch"] == (old["rock"] != r["rock"])
            if mode != 2:
                assert (old["model_on"], old["model_off"]) == (
                    r["prior_on"],
                    r["prior_off"],
                )
        previous[r["subject"]] = r
        if "question" in r:
            audit_speech(
                [
                    {
                        **r,
                        "layout": r["key"],
                        "wanted": r["expected"],
                        "said": r["spoken"],
                    }
                ],
                dialect=bool(r["dialect"]),
            )
            expect = r["expected"]
        else:
            # Independently grade the possible oral claim from the public cell.
            first = local(25, r["rock"])
            later = local(25 ^ 4 ^ 32, r["rock"])
            effective = [
                j
                for j, delta in enumerate((4, 32))
                if local(25 ^ delta, r["rock"]) == later
            ]
            expect = 0
            if first != later and later and len(effective) == 1:
                indices = (14, 3 if effective[0] else 0, 25, 7, 26)
                expect = (5 << 56) | sum(
                    (word + 1) << (6 * j) for j, word in enumerate(indices)
                )
            assert expect == r["expected"]
        if physical:
            channel(r, law)
        tally["episodes"] += 1
        tally["switches"] += r["switch"]
        tally["changed_before"] += r["switch"] * r["before_correct"]
        tally["changed_after"] += r["switch"] * correct
        tally["stable_after"] += (not r["switch"]) * correct
        tally["explained"] += r["spoken"] == expect and bool(expect)
        tally["harvest"] += r["caught"]
        tally["heat"] += r["heat"]
    return tally


def field(rows):
    n = writes = 0
    for r in rows:
        if not r.get("context"):
            continue
        n += 1
        s = r["sensed"]
        permission = r["permission"]
        actual = r["fired"]
        passive = r["passive_before"]
        recent = r["recent_before"]
        if permission & (1 if actual else 2):
            moved = r["hand"] and r["previous"] != r["actual"]
            if not moved:
                passive |= 1 << s
            keep = moved and r["context_alternatives"] & (1 << s)
            if keep:
                value = 128 + 64 * actual + s
                recent = (
                    recent & 0xFFFFFF
                    if recent & 255 == value
                    else ((recent << 8) & 0xFFFFFF) | value
                )
                writes += 1
            else:
                recent = (recent & 0xFFFFFF) | (min(255, 1 + (recent >> 24)) << 24)
        assert (passive, recent) == (r["passive_after"], r["recent_after"])
    return dict(context_looks=n, signed_receipts=writes)


def jumble(x):
    x &= 0xFFFFFFFF
    x = ((x ^ (x >> 16)) * 0x21F0AAAD) & 0xFFFFFFFF
    x = ((x ^ (x >> 15)) * 0x735A2D97) & 0xFFFFFFFF
    return x ^ (x >> 15)


def choices(rows):
    identities = set()
    counts = dict(
        samples=0,
        unique=0,
        different_hand=0,
        different_net=0,
        enabled_food=0,
        forgone_food=0,
        signed=0,
    )
    for r in rows:
        on = r["on"]
        off = r["off"]
        gene = r["gene"]
        motor = r["motor"]
        passive = r["passive_before"]
        recent = r["recent_before"]
        pooled = infer(on, off, gene, False)
        resolved = contextual_infer(on, off, gene, passive, recent)
        base = infer(on & ~off & passive, off & ~on & passive, gene, False)
        assert base[3] == r["alternatives"]
        review = bool(
            base[3]
            and ((motor >> 8) & 31)
            and (not (recent & 128) or recent >> 24 >= 4 + ((motor >> 23) & 31))
        )
        assert review == r["review"]
        senses = 63 & ~(gene & (gene >> 6) & (gene >> 12) & (gene >> 18))

        def act(model, revisit):
            candidates = []
            for grip in set_bits(r["options"]):
                raw = (r["rows"] >> (6 * grip)) & 63
                aff = 1 << (raw & senses)
                predicted = 1 if model[0] & aff else 0 if model[1] & aff else 2
                weight = (predicted == r["want"] and r["valuable"]) * (
                    3 + ((motor >> 11) & 15)
                )
                weight += (
                    (predicted == 2) * ((motor >> 6) & 7)
                    + (not ((on | off) & aff)) * (motor & 7)
                    + bool(model[3] & aff) * ((motor >> 20) & 7)
                    + (grip == 0) * ((motor >> 25) & 7)
                )
                if grip and revisit and base[3] & aff:
                    weight += (motor >> 8) & 31
                priority = (weight << 9) | ((jumble(r["coin"] + grip * 2971) & 255) + 1)
                candidates.append((priority, -grip, raw, predicted))
            _, chosen, raw, forecast = max(candidates)
            cast = r["net_available"] and (
                forecast == r["want"]
                or forecast == 2
                and ((r["coin"] >> 18) & 7) < motor >> 29
            )
            if r.get("port") == 2:
                cast = r["net_available"] and r["port_choice"]
            cost = int(bool(chosen)) + int(cast) + r["eyes"] + r.get("pump", 0)
            bite = (
                min(
                    r["stock"],
                    1 + (r["limbs"] & 7).bit_count(),
                    65535 - r["energy"] + cost,
                )
                if cast and local(raw, r["rock"]) == r["want"]
                else 0
            )
            return -chosen, raw, int(cast), cost, bite

        real = act(resolved, review)
        other = act(pooled, False)
        assert real == tuple(r[k] for k in ("hand", "row", "cast", "heat", "food")), (
            real,
            r,
        )
        assert other == tuple(
            r[k]
            for k in (
                "pooled_hand",
                "pooled_row",
                "pooled_cast",
                "pooled_heat",
                "pooled_food",
            )
        )
        seen = real[1] & senses
        fired = local(real[1], r["rock"])
        old = r["rows"] & 63
        sensor = {
            "context": 1,
            "sensed": seen,
            "permission": r["permission"],
            "fired": fired,
            "passive_before": passive,
            "recent_before": recent,
            "hand": real[0],
            "previous": old,
            "actual": real[1],
            "context_alternatives": base[3],
            "passive_after": r["passive_after"],
            "recent_after": r["recent_after"],
        }
        change = field([sensor])
        assert change["signed_receipts"] == r["signed"]
        if r["food_sample"]:
            assert real[4] != other[4]
        counts["samples"] += 1
        identity = (r["tick"], r["slot"], r["born"], r["site"])
        if identity in identities:
            continue
        identities.add(identity)
        counts["unique"] += 1
        counts["different_hand"] += real[0] != other[0]
        counts["different_net"] += real[2] != other[2]
        counts["enabled_food"] += max(0, real[4] - other[4])
        counts["forgone_food"] += max(0, other[4] - real[4])
        counts["signed"] += r["signed"]
    return counts


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("receipt", type=Path)
    p.add_argument("--choices", action="store_true")
    p.add_argument("--field", action="store_true")
    p.add_argument("--omit-grid", action="store_true")
    p.add_argument("--mode", type=int)
    p.add_argument("--gene", type=int)
    a = p.parse_args()
    x = read(a.receipt)
    print(
        choices(x)
        if a.choices
        else field(x) if a.field else stream(x, a.mode, a.gene, not a.omit_grid)
    )
