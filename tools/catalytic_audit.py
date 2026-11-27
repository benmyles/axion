#!/usr/bin/env python3
"""Independently grade two-beat material memories, physical messages and balanced cooperative trials."""

import argparse
from collections import Counter
from capillary_audit import local, read


def contact(rock, coating, mobile, row):
    # Reconstruct the two physical interactions from their individual properties.
    law = rock
    if coating % 2 != mobile % 2:
        law ^= 16
    if coating >= 2 and mobile < 2:
        law ^= 2
    admitted = local(row, law)
    return (
        law,
        admitted,
        (mobile if admitted else coating),
        (coating if admitted else mobile),
    )


def boundaries(rows):
    assert len(rows) == 32768
    identified = set()
    closed = changed = 0
    for r in rows:
        key = tuple(r[k] for k in ("geology", "bound", "carrier", "input"))
        assert key not in identified
        identified.add(key)
        a, flow, layer, drop = contact(key[0], key[1], key[2], key[3])
        b, second, last, free = contact(key[0], layer, drop, key[3])
        assert (a, b, flow, second, drop, layer, free, last) == tuple(
            r[k]
            for k in (
                "law",
                "second_law",
                "flow",
                "second_flow",
                "first_carrier",
                "first_bound",
                "last_carrier",
                "last_bound",
            )
        )
        assert sorted((layer, drop)) == sorted((key[1], key[2])) == sorted((last, free))
        closed += bool(flow and not second)
        changed += a != b
    return dict(cases=len(rows), self_closing=closed, state_changes=changed)


def experienced(rows):
    changed = 0
    for r in rows:
        law, flow, layer, drop = contact(
            r["geology"], r["pad"], r["incoming"], r["actual"]
        )
        assert (law, flow, layer, drop) == (
            r["rock"],
            r["fired"],
            r["coating"],
            r["outgoing"],
        )
        changed += flow != local(r["actual"], r["geology"])
    return dict(exposures=len(rows), changed=changed)


def witnesses(rows):
    seen = set()
    for r in rows:
        key = r["epoch"], r["place"]
        assert key not in seen
        seen.add(key)
        _, first, layer, drop = contact(
            r["geology"], r["bound"], r["carrier"], r["row"]
        )
        next_law, second, _, _ = contact(r["geology"], layer, drop, r["row"])
        assert (first, second) == (1, 0) == (r["first_flow"], r["second_flow"])
        assert (layer, drop) == (r["next_bound"], r["next_carrier"])
    return dict(witnessed=len(seen), handled=sum(bool(r["first_hand"]) for r in rows))


def correspondence(rows, inert=False):
    assert len(rows) % 2 == 0
    counts = Counter()
    for i in range(0, len(rows), 2):
        a, b = rows[i : i + 2]
        for r in (a, b):
            assert r["trial"] == i // 2
            base, layer, drop, row = (
                r[k] for k in ("geology", "bound", "carrier", "input")
            )
            _, initial, after, free = contact(base, layer, drop, row)
            _, closed, _, _ = contact(base, after, free, row)
            assert initial == 1 and closed == 0
            assert r["sensed"] == (
                local(row, base) if inert else closed if r["write"] else initial
            )
            assert (
                r["food"] == int(r["cue"] == r["choice"])
                and r["cost"] == 2 + r["write"]
            )
            counts["correct"] += r["food"]
            counts["erased"] += r["no_mark"] == r["cue"]
            counts["inverse"] += r["inverse"] == r["cue"]
        assert (a["cue"], b["cue"]) == (0, 1)
        assert a["no_mark"] == b["no_mark"]
        if a["sensed"] == b["sensed"]:
            assert a["choice"] == b["choice"]
        if a["food"] and b["food"]:
            assert a["write"] != b["write"]
            counts["left_convention" if a["write"] else "right_convention"] += 1
    assert counts["erased"] == len(rows) // 2
    if inert:
        assert counts["correct"] == counts["inverse"] == counts["erased"]
    return dict(cases=len(rows), **dict(counts))


def pairs(rows):
    tutor = rows[0].get("tutor_role", 0)
    for role, fixed in (("sender", 1), ("receiver", 2)):
        assert len({(r[role], r[role + "_born"]) for r in rows}) == (
            1 if tutor == fixed else len(rows)
        )
    totals = Counter()
    for r in rows:
        assert (r["sender"], r["sender_born"]) != (r["receiver"], r["receiver_born"])
        n = r["histories"] * r["tests"]
        assert (
            r["erased"] == n
            and r["both"] == r["left_convention"] + r["right_convention"]
        )
        assert r["initial_mastery"] + r["acquired"] - r["lost"] == r["final_mastery"]
        if "reliable_left" in r:
            assert r["reliable_left"] + r["reliable_right"] <= r["histories"]
        assert r["training_heat"] == 2 * r["lessons"] * r["histories"] + r["marks"]
        for k in (
            "before",
            "after",
            "both",
            "erased",
            "inverse",
            "left_convention",
            "right_convention",
            "training_food",
            "training_heat",
        ):
            totals[k] += r[k]
        totals["cases"] += 2 * n
    return dict(pairs=len(rows), **dict(totals))


def wild(rows):
    totals = Counter()
    for r in rows:
        role = r["port"]
        if not role:
            continue
        totals["visits"] += 1
        assert r["heat"] == 1 + bool(r["hand"]) + r["cast"] + r["pump"]
        assert r["previous"] == r["basal_row"] | r["pump"]
        if role == 1:
            assert r["light"] == int(r["food_before"] >= r["stock_threshold"])
            assert r["pump"] == int(r["choice"] and r["pump_available"])
        else:
            assert r["light"] == bool(r["basal_row"] & 3)
            assert r["cast"] == int(r["choice"] and r["net_available"])
        gene = r["dendrites"]
        senses = 63 & ~(gene & (gene >> 6) & (gene >> 12) & (gene >> 18))
        executed = r["pump"] if role == 1 else r["cast"]
        position = 2 * (
            r["light"] * bool(senses & 3) + 2 * executed * bool(senses & 12)
        )
        previous = r["appetite_before"]
        learned = r["appetite_after"]
        change = previous ^ learned
        assert (change & ~(3 << position)) == 0 and change.bit_count() == r[
            "appetite_edits"
        ]
        old = (previous >> position) & 3
        signal = 1 if r["food"] else 2
        proposal = sum(
            ((r["appetite_rule"] >> (2 * ((old >> j) & 1) + ((signal >> j) & 1))) & 1)
            << j
            for j in range(2)
        )
        assert (change >> position) & ~(old ^ proposal) == 0
        totals["changed"] += bool(change)
        if r["appetite_audit"]:
            totals["different"] += r["unlearned_choice"] != r["choice"]
            if role == 2:
                cast = int(r["unlearned_choice"] and r["net_available"])
                heat = r["heat"] - r["cast"] + cast
                food = (
                    min(
                        r["food_before"],
                        r["mouthful"],
                        65535 - r["energy_before"] + heat,
                    )
                    if cast and r["fired"] == r["wants_flow"]
                    else 0
                )
                assert (cast, food) == (r["unlearned_cast"], r["unlearned_food"])
    return dict(totals)


def receptions(rows):
    totals = Counter()
    previous = None
    for r in rows:
        key = (r["tick"], r["destination"], r["axis"])
        assert previous is None or previous < key
        previous = key
        assert (r["writer"], r["writer_born"]) != (r["reader"], r["reader_born"])
        side = r["world"]
        x = r["source"] % side
        y = r["source"] // side
        dest = (y + (r["axis"] == 1)) % side * side + (x + (r["axis"] == 0)) % side
        assert dest == r["destination"]
        assert r["writer_input"] == r["writer_without"] | 1
        assert (
            local(r["writer_input"], r["writer_rock"]) == 1
            and local(r["writer_without"], r["writer_rock"]) == 0
        )
        assert r["alternative"] == r["input"] ^ (1 << r["axis"])
        for word in ("", "other_"):
            v = r["alternative"] if word else r["input"]
            light = bool(v & 3)
            assert r[word + "sensed"] == light
            choice = r["other_move"] if word else r["move"]
            cast = int(r["available"] and (not r["sorting"] or light) and choice)
            assert cast == r[word + "cast"]
            cost = r["heat"] - r["cast"] + cast
            meal = (
                min(r["stock"], r["mouthful"], 65535 - r["energy"] + cost)
                if cast and local(v, r["rock"]) == r["wants"]
                else 0
            )
            assert meal == r[word + "food"]
        if r["sensed"] == r["other_sensed"]:
            assert (
                r["move"] == r["other_move"]
                and r["no_contacts"] == r["other_no_contacts"]
            )
        changed = r["move"] != r["other_move"]
        grown = changed and r["no_contacts"] == r["other_no_contacts"]
        totals["encounters"] += 1
        totals["changed_light"] += r["sensed"] != r["other_sensed"]
        totals["changed_move"] += changed
        totals["learned_pump"] += r["writer_learned"]
        totals["learned_response"] += grown
        totals["learned_both"] += grown * r["writer_learned"]
        totals["extra_food"] += max(0, r["food"] - r["other_food"])
        totals["forgone_food"] += max(0, r["other_food"] - r["food"])
    return dict(totals)


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "mode",
        choices=(
            "boundaries",
            "experienced",
            "witnesses",
            "pair",
            "inert",
            "population",
        ),
    )
    p.add_argument("observations")
    a = p.parse_args()
    v = read(a.observations)
    fn = {
        "boundaries": boundaries,
        "experienced": experienced,
        "witnesses": witnesses,
        "pair": correspondence,
        "inert": lambda r: correspondence(r, True),
        "population": pairs,
    }[a.mode]
    print(fn(v))
