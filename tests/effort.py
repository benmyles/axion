#!/usr/bin/env python3
"""Independent load, action-consequence, compaction, checkpoint and spoken counterfactual checks."""

import os
import struct
import tempfile
from pathlib import Path

from press import call, deck, rows
from worksites import ROOT, checksum, mix, snapshot


def field(*args, **kwargs):
    return call("floodplain", *args, **kwargs)


def verify_cases(log):
    for row in rows(log, "effort_receipt"):
        d = {k: int(v) for k, v in row.items()}
        mapping = deck(mix(d["effort_receipt"] + 41))
        wet = deck(mix(d["effort_receipt"] + 971))
        costs = [1 + (t + 1 + mix(d["effort_receipt"] + 5113) % 3) % 4 for t in wet]
        yields = [2 + 4 * ((x + 1 + mix(d["effort_receipt"] + 371) % 3) % 4) for x in mapping]
        chosen = d["trip"]
        assert d["ate"] == (
            d["far_units"] * yields[d["away"]] if chosen else d["near_units"] * yields[d["home"]]
        )
        assert d["paid"] == chosen * d["far"] * costs[d["ground"]]
        assert d["left_current"] == d["right_current"] or bool(chosen) == (
            d["right_current"] > d["left_current"]
        )
    for row in rows(log, "effort_slab"):
        d = {k: int(v) for k, v in row.items()}
        key = d["effort_slab"]
        g = mix(key + 2347)
        pigment = deck(mix(key + 1699))
        handles = deck(mix(key + 7013))
        fees = [1 + ((d["rates"] >> (2 * j)) & 3) for j in range(4)]
        where = (g >> 4) & 3
        assert d["visible"] == pigment[where]
        fee = 0
        for j in range(d["length"]):
            act = (d["strokes"] >> j) & 1
            fee += fees[handles[act]]
            where = (where + (1 if act ^ ((g >> 13) & 1) else -1)) % 4
        assert d["after"] == pigment[where]
        expected = (
            fee + fees[d["after"]] * d["crossings"]
            if d["prepared"]
            else fees[d["visible"]] * d["crossings"]
        )
        assert expected == d["work"]
    study = {r["effort_choice"]: r for r in rows(log, "effort_choice")}
    for r in study.values():
        assert int(r["net"]) == int(r["food"]) - int(r["work"])
        assert int(r["optimum"]) - int(r["net"]) == int(r["regret"])
        assert int(r["correct"]) == int(r["ties"]) + int(r["near_correct"]) + int(r["far_correct"])
    slabs = {r["effort_slat"]: r for r in rows(log, "effort_slat")}
    for r in slabs.values():
        assert int(r["price"]) - int(r["best_price"]) == int(r["regret"])
        assert int(r["correct"]) == int(r["good_preparation"]) + int(r["good_ordinary"]) + int(r["ties"])
    assert int(study["lived"]["net"]) > max(int(study["lived"][j]) for j in ("always_near", "always_far"))
    assert int(slabs["inferred"]["price"]) < int(slabs["unchanged"]["price"])


def paving_prices(world, uses):
    d = world["side"]
    seed = struct.unpack_from("<I", world["raw"], 24)[0]
    tick = world["tick"]
    period = world["outlay"][1]
    costs, oracle, sites = 0, 0, 0
    for pos, slot in enumerate(world["slots"]):
        a = world["soil"][pos]
        if not world["energy"][slot] or not world["limbs"][slot] or a[1] < 2:
            continue
        ray = world["heading"][slot]
        x, y = pos % d, pos // d
        nx = (x + (ray == 1) - (ray == 3)) % d
        ny = (y + (ray == 2) - (ray == 0)) % d
        other = ny * d + nx
        edge = pos if ray in (1, 2) else other
        axis = int(ray % 2 == 0)
        shift = 4 * axis
        packed = (world["treads"][edge] >> shift) & 15
        if packed == 15:
            continue
        mass = world["shape"][slot].bit_count()
        limbs = world["limbs"][slot].bit_count()
        base = world["soil"][edge]
        wall, fiber = base[2 + axis], base[9 + axis]
        bills = []
        for strokes in range(3):
            total = 0
            compression = packed
            for _ in range(strokes):
                if compression == 15:
                    break
                compression += min(15 - compression, 1 + limbs // 3)
                total += 1
            for j in range(uses):
                here = other if j % 2 else pos
                q = world["soil"][here]
                district = (here % d) // 16 + 128 * ((here // d) // 16)
                weather = (mix(seed + district * 1789) + (tick // period if period else 0)) & 3
                material = (q[8] ^ (q[11] >> 9) ^ world["field"][here][0]) & 3
                drag = (
                    1 + ((material + weather) & 3) + min(3, q[0] // 32) + min(3, wall // 2) + mass // 12
                )
                total += max(1, drag - min(5, fiber // 4) - compression // 3)
                compression += min(15 - compression, 1 + mass // 12)
            bills.append(total)
        costs += bills[0]
        oracle += min(bills)
        sites += 1
    return sites, costs, oracle


def test(work):
    os.environ["AXION_JOURNAL_DIR"] = str(work / "journal")
    base = ROOT / "examples/treadbank.flood"
    one, full, mid, rest, wide = [work / (s + ".flood") for s in ["one", "full", "mid", "rest", "wide"]]
    args = [
        "--hollow-ground",
        "rich",
        "--effort-audit",
        "randomized",
        "--effort-window",
        16,
        "--effort-weather",
        32,
        "--effort-movement",
        "select",
        "--effort-assignments",
        "same",
        "--effort-contest",
        "paired",
    ]
    field("--load", base, *args, "--steps", 64, "--save", mid)
    field("--load", mid, "--steps", 85, "--save", rest)
    r = field(
        "--load",
        base,
        *args,
        "--steps",
        149,
        "--effort-census",
        "--effort-study",
        192,
        "--effort-bench",
        100003,
        "--save",
        full,
    )
    assert rest.read_bytes() == full.read_bytes()
    verify_cases(r)
    d = snapshot(full)
    n = d["side"] ** 2
    record = rows(r, "effort_field")[0]
    assert len(d["economies"]) == n and int(record["worn"]) - int(record["eroded"]) == sum(
        (v & 15) + (v >> 4) for v in d["treads"]
    )
    assert sum(any(b[4:12]) and bool(e) for b, e in zip(d["economies"], d["energy"])) > 0
    bench = rows(r, "effort_deploy")[0]
    assert (
        bench["errors"],
        int(bench["currents_checked"]),
        bench["image_bytes"],
        bench["writable_bytes"],
    ) == ("0", 100003 * 12, "20", "60")
    random = rows(r, "effort_random")
    assert all(int(q["dead"]) <= int(q["trials"]) for q in random)
    census = rows(r, "effort_appraisal")
    assert len(census) == 4 and len({q["offers"] for q in census}) == 1
    for q in census:
        assert int(q["ties"]) + int(q["here_correct"]) + int(q["there_correct"]) == int(q["correct"])
    field("--load", full, "--wash-treads", "--steps", 0, "--save", one)
    washed = snapshot(one)
    assert (
        not any(washed["treads"])
        and d["economies"] == washed["economies"]
        and d["pockets"] == washed["pockets"]
        and d["soil"] == washed["soil"]
    )
    # Physical treads are spatial and brains belong to identity across expansion.
    field("--load", full, "--extend", 128, "--steps", 0, "--save", wide)
    b = snapshot(wide)
    assert b["economies"][:n] == d["economies"]
    for y in range(d["side"]):
        assert (
            b["treads"][y * 128 : y * 128 + d["side"]] == d["treads"][y * d["side"] : (y + 1) * d["side"]]
        )
    field("--load", mid, "--steps", 1, "--save", one)
    new = snapshot(one)
    assert any(a == 0 and e for a, e in zip(new["ages"], new["energy"]))
    for age, e, m in zip(new["ages"], new["energy"], new["economies"]):
        if age == 0 and e:
            assert not any(m[4:])
    craft = field(
        "--load", full, "--effort-paving", "forecast", "--effort-census", "--steps", 80, "--save", one
    )
    shaped = snapshot(one)
    makers = rows(craft, "effort_builders")[0]
    assert int(makers["traffic_memory"]) > 0 and len(rows(craft, "effort_paving")) == 9
    for uses in (1, 4, 8):
        sample = next(
            k
            for k in rows(craft, "effort_paving")
            if k["uses"] == str(uses) and k["effort_paving"] == "lived"
        )
        assert tuple(int(sample[k]) for k in ("sites", "plain", "oracle")) == paving_prices(shaped, uses)
        assert int(sample["price"]) >= int(sample["oracle"])
    # Independent beneficiaries on inherited physical ground receive fresh private organs.
    field(
        "--soil",
        full,
        "--side",
        d["side"],
        "--neurons",
        128,
        "--density",
        55,
        "--effort",
        "live",
        "--steps",
        0,
        "--save",
        one,
    )
    transplanted = snapshot(one)
    assert transplanted["treads"] == d["treads"] and all(
        not any(m[4:]) for m in transplanted["economies"]
    )
    speaker = ROOT / "examples/causeway-guide.mind"
    peer = ROOT / "examples/loam-a.mind"
    held = work / "learned.mind"
    c = call(
        "talk",
        "--speaker",
        speaker,
        "--listener",
        peer,
        "--save-speaker",
        held,
        stdin=(ROOT / "examples/ford-walk.txt").read_text(),
    )
    contacts = rows(c, "effort_contact")
    answers = rows(c, "effort_reply")
    assert sum(int(q["eaten"]) for q in contacts) == 2048 - int(contacts[-1]["stock"])
    assert all(0 <= int(q["reserve"]) <= 65535 for q in contacts)
    assert len(answers) >= 5 and sum(q["effort_reply"] == "1" for q in answers) >= 4
    assert any(q["role"] == "5" and q["effort_reply"] == "1" for q in answers)
    assert any(q["effort_contact"] == "7" and q["arrived"] == "1" for q in contacts)
    cost = held.read_bytes()[-76:-8]
    assert len(cost) == 68 and struct.unpack_from("<II", cost, 60) == (0x45464654, 68)
    assert held.read_bytes()[-8:] == checksum(held.read_bytes()[:-8])
    log = field("--resident", held, "--side", 64, "--steps", 0, "--save", rest)
    owner = int(rows(log, "rehoused_resident")[0]["rehoused_resident"])
    returned = work / "returned.mind"
    field("--load", rest, "--slot", owner, "--steps", 0, "--export-mind", returned)
    assert (
        returned.read_bytes()[-(1120 + 164 + 68 + 8) : -8]
        == held.read_bytes()[-(1120 + 164 + 68 + 8) : -8]
    )
    reopened = work / "reopened.mind"
    call(
        "talk",
        "--speaker",
        held,
        "--listener",
        peer,
        "--save-speaker",
        reopened,
        stdin="/ford 91\n/quit\n",
    )
    assert reopened.read_bytes()[-76:-28] == held.read_bytes()[-76:-28]
    bad = bytearray(held.read_bytes())
    struct.pack_into("<I", bad, len(bad) - 76 + 48, 1 << 31)
    bad[-8:] = checksum(bytes(bad[:-8]))
    broken = work / "corrupt.mind"
    broken.write_bytes(bad)
    assert "invalid effort cortex" in call(
        "talk", "--speaker", broken, "--listener", peer, stdin="/quit\n", ok=False
    )
    ford = work / "cases"
    raw = "71401 0 1 1 256 4 256 1 1 | choose ivory pool or flint mere | why | without mere | go | lived novel "
    turns = []
    for refs in ("right pool left left", "reed stone rock sky"):
        ford.write_text(raw + refs + "\n")
        log = call(
            "talk",
            "--speaker",
            speaker,
            "--listener",
            peer,
            stdin=f"/ford 9\n/ford-tests {ford}\n/quit\n",
        )
        turns.append(rows(log, "effort_sentence"))
    assert len(turns[0]) == 4 and turns[0] == turns[1]
    print(
        "Independent work prices, physical transformations, randomized ledgers, compaction, spoken counterfactuals and persistence passed"
    )


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(
        prefix="axion-effort-test-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as work:
        test(Path(work))
