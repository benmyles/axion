#!/usr/bin/env python3
"""Verify material copies, sensory preconditioning, local credit and spoken retrieval independently of CUDA."""

import os
import struct
import subprocess
import tempfile
from pathlib import Path

from worksites import ROOT, checksum, mix, snapshot

BINARY = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()


def call(*args, stdin="", ok=True):
    r = subprocess.run(
        list(map(str, [BINARY, *args])),
        input=stdin,
        text=True,
        capture_output=True,
        check=False,
    )
    if (r.returncode == 0) != ok:
        raise AssertionError((args, r.stdout[-700:], r.stderr))
    return r.stdout + r.stderr


def rows(log, key):
    return [
        dict(v.split("=", 1) for v in l.split() if "=" in v)
        for l in log.splitlines()
        if l.startswith(key + "=")
    ]


def deck(key):
    out = list(range(4))
    for i in (3, 2, 1):
        j = mix(key + i * 257) % (i + 1)
        out[j], out[i] = out[i], out[j]
    return out


def bits(word):
    return [(word >> (2 * i)) & 3 for i in range(4)]


def learn(table, row, spike, reinforcement, law):
    for column in range(4):
        address = table[row][column] + 2 * (column == spike) + 4 * reinforcement
        table[row][column] = (law >> address) & 1


def packed(table):
    return sum(
        int(value) << (4 * r + c)
        for r, row in enumerate(table)
        for c, value in enumerate(row)
    )


def audit_screen(log):
    trace = rows(log, "press_trace")[0]
    s = {k: int(v) for k, v in trace.items()}
    cohort = rows(log, "press_specimen")
    seed = s["press_trace"]
    law = s["germ"]
    tools = deck(mix(seed + 2071))
    old = deck(mix(seed + 9137))
    delta = 1 + mix(seed + 7351) % 3
    new = [(p + delta) % 4 for p in old]
    assert (
        tools == bits(s["pitches"]) and old == bits(s["old"]) and new == bits(s["next"])
    )
    forecast = [[0] * 4 for _ in range(4)]
    values = [[0] * 4 for _ in range(4)]
    for row in cohort:
        r = {k: int(v) for k, v in row.items()}
        i = r["press_specimen"]
        assert ((r["glaze"] >> 4) ^ r["face"]) & 3 == i
        assert r["impression"] & 160 == 160 and r["impression"] & 3 == tools[i]
        for _ in range(3):
            learn(forecast, i, tools[i], 1, law & 255)
    assert packed(forecast) == s["forecast"]
    for medium, base, expected in ((old, 8247, s["before"]), (new, 317, s["value"])):
        key = mix(seed + base)
        for cycle in range(3):
            order = mix(key + cycle * 7937)
            for step in range(16):
                point = (step * ((order >> 4) | 1) + order) & 15
                b = point >> 2
                t = point & 3
                learn(values, b, t, int(t == medium[b]), law >> 8 & 255)
        assert packed(values) == expected
    route = law >> 16

    def current(h, b):
        src = [value ^ (route >> 2 & 1) for value in forecast[h]]
        dst = [value ^ (route >> 3 & 1) for value in values[b]]
        return sum(
            (1 + (route & 2 != 0)) if dst[k] else -(route >> 4 & 3)
            for k in range(4)
            if src[k]
        )

    for b in range(4):
        former, today = tools.index(old[b]), tools.index(new[b])
        tie = mix(seed + b * 2281) & 1
        for rev in range(2):
            a, z = (today, former) if rev else (former, today)
            u, v = current(a, b), current(z, b)
            chosen = (a, z)[tie if u == v else int(v > u)]
            j = 2 * b + rev
            assert (s["decisions"] >> (2 * j)) & 3 == chosen and (
                s["outcomes"] >> j
            ) & 1 == (tools[chosen] == new[b])
    arms = {a["press_transfer"]: a for a in rows(log, "press_transfer")}
    for r in arms.values():
        n = int(r["preparations"])
        assert int(r["stock"]) == int(r["residual"]) + int(r["recovered"])
        assert (
            16 * n == int(r["casts"])
            and 12 * n == int(r["recast"])
            and int(r["heat"]) == int(r["offered"])
        )
    for arm in ("random", "no_sight", "no_taste", "erase_sight", "erase_taste"):
        assert 2 * int(arms[arm]["productive"]) == int(arms[arm]["offered"])


def tissue(path):
    raw = path.read_bytes()
    end = raw.rindex(struct.pack("<II", 0x434C4159, 72))
    assert raw[-8:] == checksum(raw[:-8])
    return [v >> 16 for v in struct.unpack_from("<12I", raw, end - 64)[10:12]]


def test(work):
    os.environ["AXION_JOURNAL_DIR"] = str(work / "journal")
    bank = work / "seed.flood"
    mid = work / "mid.flood"
    full = work / "full.flood"
    rest = work / "rest.flood"

    def flood(*args, ok=True):
        return call("floodplain", *args, ok=ok)

    src = ROOT / "examples/reedbed.flood"
    cold = flood(
        "--load",
        src,
        "--press",
        "live",
        "--press-geology",
        "tidal",
        "--clay-reach",
        "shared",
        "--steps",
        0,
        "--press-study",
        32,
        "--press-bench",
        8192,
        "--save",
        bank,
    )
    audit_screen(cold)
    speed = rows(cold, "press_deploy")[0]
    assert (
        speed["bytes"] == "4"
        and speed["differences"] == "0"
        and int(speed["verified"]) == 64 * 8192
    )
    flood("--load", bank, "--steps", 73, "--save", mid)
    flood("--load", mid, "--steps", 116, "--save", rest)
    report = flood("--load", bank, "--steps", 189, "--press-census", "--save", full)
    assert full.read_bytes() == rest.read_bytes()
    before = snapshot(bank)
    after = snapshot(full)
    assert not any((m[10] >> 16) or (m[11] >> 16) for m in before["potters"])
    p = rows(report, "press_field")[0]
    assert int(p["heat"]) == int(p["strokes"]) > 0 and int(p["impressions"]) > 0
    assert 0 < int(p["openings"]) <= int(p["loaded"]) <= int(p["strokes"]) and int(
        p["eaten"]
    ) <= int(p["released"])
    offers = rows(report, "press_census")
    assert len(offers) == 6 and len({x["discriminating"] for x in offers}) == 1
    den = int(offers[0]["discriminating"])
    assert all(
        int(r["offers"]) == den and int(r["twice_correct"]) <= 2 * den
        for r in rows(report, "press_affordance")
    )
    quiet = flood("--load", bank, "--press", "no-casting", "--steps", 189)
    assert rows(quiet, "press_field")[0]["impressions"] == "0"
    muted = flood("--load", bank, "--press", "alone", "--steps", 189)
    assert all(
        rows(muted, "press_field")[0][k] == "0"
        for k in ("witnessed", "studies", "ancestral")
    )
    flood("--load", full, "--extend", 128, "--steps", 0, "--save", rest)
    broad = snapshot(rest)
    assert (
        broad["potters"][:4096] == after["potters"]
        and broad["cargo"][:4096] == after["cargo"]
    )
    for y in range(64):
        assert (
            broad["ware"][128 * y : 128 * y + 64] == after["ware"][64 * y : 64 * y + 64]
        )
    who = next(
        i
        for i, m in enumerate(after["potters"])
        if after["energy"][i] > 4 and m[10] >> 16 and m[11] >> 16
    )
    body = work / "body.mind"
    returned = work / "returned.mind"
    flood("--load", full, "--slot", who, "--export-mind", body, "--steps", 0)
    learned = tissue(body)
    assert learned == [v >> 16 for v in after["potters"][who][10:12]]
    call(
        "talk",
        "--speaker",
        body,
        "--listener",
        ROOT / "examples/loam-a.mind",
        "--save-speaker",
        returned,
        stdin="/reed 37\n/kiln examples/loam-a.clay covered\n/quit\n",
    )
    assert tissue(returned) == learned
    replay = flood(
        "--load",
        full,
        "--steps",
        0,
        "--press-study",
        60,
        "--press-census",
        "--save",
        mid,
    )
    assert full.read_bytes() == mid.read_bytes()
    audit_screen(replay)
    corrupt = bytearray(full.read_bytes()[:-8])
    struct.pack_into("<Q", corrupt, after["clay_pos"] + 112 + 7 * 8, 2**62)
    rest.write_bytes(corrupt + checksum(corrupt))
    assert "invalid clay workshop" in flood("--load", rest, "--steps", 0, ok=False)
    for feel, fee in (("live", 1), ("stiff", 3), ("displaced", 1), ("quiet", 1)):
        compare = flood(
            "--load",
            full,
            "--press-feel",
            feel,
            "--steps",
            67,
            "--press-census",
            "--save",
            mid,
        )
        touch = rows(compare, "press_touch")[0]
        assert int(touch["heat"]) == fee * (
            int(touch["rims"]) + int(touch["tool_taps"])
        )
        assert (int(touch["heat"]) > 0) == (feel != "quiet")
        totals = rows(compare, "press_field")[0]
        assert int(totals["heat"]) == int(totals["strokes"]) + int(touch["heat"])
        arms = {v["press_census"]: v for v in rows(compare, "press_census")}
        assert (
            arms["own"]["work"]
            == arms["displaced"]["work"]
            == arms["other_taste"]["work"]
        )
        assert int(arms["own"]["work"]) >= int(arms["optical"]["work"])
        if feel == "quiet":
            assert (
                arms["own"]["opened"]
                == arms["optical"]["opened"]
                == arms["displaced"]["opened"]
            )
        flood("--load", mid, "--steps", 0, "--save", rest)
        assert mid.read_bytes() == rest.read_bytes()
    funded = flood(
        "--load",
        bank,
        "--press-brood",
        "finite",
        "--press-seasons",
        8,
        "--press-population",
        128,
        "--steps",
        0,
    )
    born = rows(funded, "press_hatchlings")[0]
    assert 0 < int(born["press_hatchlings"]) <= 128 and int(
        born["carbon_committed"]
    ) == 16 * int(born["press_hatchlings"])
    oral(work)


def oral(work):
    reader = ROOT / "examples/wheel-reader.mind"
    peer = ROOT / "examples/loam-a.mind"
    seed = 4242
    pitch = deck(mix(seed + 2071))
    gates = deck(mix(seed + 9137))
    opening = gates[0]
    names = ("amber", "slate", "flint", "ivory")
    notes = ("chime", "hum", "buzz", "knock")
    a, b = names[pitch.index(opening)], names[pitch.index((opening + 1) & 3)]
    log = call(
        "talk",
        "--speaker",
        reader,
        "--listener",
        peer,
        stdin="""/wheel 4242
/wheel-watch
/wheel-taste
which tool opens brook?
what note does it give?
/wheel-turn 1
what note does it give?
which tool opens brook?
/wheel-taste
which tool opens brook?
what note does it give?
/wheel-press 0
/quit
""",
    )
    said = [
        q.removeprefix("animal> ") for q in log.splitlines() if q.startswith("animal> ")
    ]
    assert said == [a, notes[opening], "?", a, b, notes[(opening + 1) & 3]], said
    work_row = rows(log, "press_contact")[-1]
    assert sum(int(work_row[k]) for k in ("stock", "loose", "consumed")) == 2048
    assert int(work_row["meals"]) == 18 and int(work_row["shell"]) == names.index(b)
    tests = work / "words.txt"
    tests.write_text(
        "\n".join(
            f"4242 1 15 {arm} tool | which tool opens brook? | what note does it give? | {b} | {notes[(opening + 1) & 3]}"
            for arm in ("both", "prior", "no_topic")
        )
    )
    log = call(
        "talk",
        "--speaker",
        reader,
        "--listener",
        peer,
        stdin=f"/wheel 7\n/wheel-tests {tests}\n/quit\n",
    )
    r = rows(log, "press_sentence")
    assert [v["said"] for v in r] == [
        b,
        notes[(opening + 1) & 3],
        a,
        notes[opening],
        b,
        "_",
    ]
    first = [(q["said"], q["mask"]) for q in r]
    tests.write_text(
        tests.read_text().replace(
            f"| {b} | {notes[(opening + 1) & 3]}", "| nonsense | wrong"
        )
    )
    other = call(
        "talk",
        "--speaker",
        reader,
        "--listener",
        peer,
        stdin=f"/wheel 7\n/wheel-tests {tests}\n/quit\n",
    )
    assert first == [(q["said"], q["mask"]) for q in rows(other, "press_sentence")]


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(
        prefix="axion-press-test-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as path:
        test(Path(path))
    print(
        "PASS independent binary contacts, cast geometry, revaluation, cold word predictions, material conservation, restart and memory ownership"
    )
