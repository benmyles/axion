#!/usr/bin/env python3
"""Decode the actual field and independently replay one neuron's transition at a time."""

from pathlib import Path
import math
import os
import struct
import subprocess
import tempfile
from worksites import checksum, mix, parse, snapshot

ROOT = Path(__file__).resolve().parents[1]
EXE = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()
MASK = 2**32 - 1


def bases(phase):
    result = []
    for raw in range(4):
        x, y = raw & 1, raw >> 1
        if phase & 4:
            x ^= y
        if phase & 8:
            y ^= x
        if phase & 16:
            x ^= y
        result.append((x + 2 * y) ^ (phase & 3))
    return result


def rearrange(q, key):
    a, b = divmod(q, 4)
    for round in range(3):
        a, b = b, a ^ ((key >> (8 * round + 2 * b)) & 3)
    return a * 4 + b


def learned(genes, states, cue, pulse):
    return [
        ((genes[17] >> (16 * pulse + 8 * ((genes[cue] >> j) & 1) + 2 * old)) & 3) if genes[16] >> j & 1 else 0
        for j, old in enumerate(states)
    ]


def traces(g):
    return [(g[19] >> j & 1) + 2 * (g[20] >> j & 1) for j in range(32)]


def scores(g, state, a, b):
    weights = [(g[18] >> (2 * s)) & 3 if g[16] >> i & 1 else 0 for i, s in enumerate(state)]

    def voltage(inp):
        return sum(w for i, w in enumerate(weights) if g[inp] & (1 << i))

    return voltage(a), voltage(b), sum(weights)


def gain(g, state, q, target, replay):
    x, _, total = scores(g, state, q, q)
    value = x * (g[18] >> 12 & 3) + min(x, total - x) * (g[18] >> 14 & 7)
    if replay:
        yes = learned(g, state, q, 1)
        no = learned(g, state, q, 0)
        a, b, _ = scores(g, yes, *target)
        c, d, _ = scores(g, no, *target)
        u, v = a - b, c - d
        value += (g[18] >> 21 & 3) * abs(u - v) + (g[18] >> 30 & 3) * total * (
            (u > 0) != (v > 0) or (u < 0) != (v < 0)
        )
    return value


def matched(data, arm, seed):
    eligible = [g for g, e in zip(data["learners"], data["energy"]) if e]
    trials = int(arm["choices"]) // (2 * int(arm["pupils"]))
    offered = int(arm["examples"])
    ions = int(arm["solutes"])
    mode = arm["brine_transfer"]
    tot = [0] * 12
    maps = [bases(p) for p in range(1 << ions)]
    for idx in range(int(arm["pupils"])):
        g = eligible[mix(seed + idx * 6011) % len(eligible)]
        base = mix(seed + idx * 11069)
        for trial in range(trials):
            k = mix(base + trial * 6217)
            chip = mix(k + 39) & 3
            order = mix(k + 257)
            sols = [mix(mix(k + h) + 517) & ((1 << ions) - 1) for h in (613, 1103)]
            if maps[sols[0]].index(chip) == maps[sols[1]].index(chip):
                sols[1] ^= 1
            sites = [maps[s].index(chip) for s in sols]
            if mix(k + 627) & 1:
                sites.reverse()
            target = tuple(chip * 4 + q for q in sites)

            def lesson(i):
                numbered = ((order >> 5) + i * (1, 5, 7, 11)[order & 3]) % 12
                return 4 * ((chip + 1 + numbered // 4) & 3) + numbered % 4

            def presented(q):
                return rearrange(q, mix(k + 2153)) if mode == "rearranged" else q

            previous = 2
            for bath in range(2):
                if mode != "reversal" or not bath:
                    state = traces(g)
                marks = {}
                for i in range(offered):
                    q = lesson(i)
                    draw = mix(k + i * 317)
                    if mode in (
                        "self_sample",
                        "random_sample",
                        "prospective",
                        "without_replay",
                        "decoy_goal",
                        "replay_fixed_budget",
                    ):
                        if i >= (g[18] >> 17 & 15):
                            break
                        a, b, power = scores(g, state, *target)
                        if (
                            mode in ("prospective", "without_replay", "decoy_goal")
                            and i >= ((g[18] >> 10 & 3) | ((g[18] >> 24) & 4))
                            and power
                            and 8 * abs(a - b) > power * (g[18] >> 23 & 7)
                        ):
                            break
                        other = lesson(i + 5)
                        pair = tuple(v ^ 4 for v in target) if mode == "decoy_goal" else target
                        if mode == "random_sample":
                            chosen = draw >> 12 & 1
                        else:
                            x, y = [
                                gain(
                                    g, state, v, pair, mode in ("prospective", "decoy_goal", "replay_fixed_budget")
                                )
                                for v in (q, other)
                            ]
                            chosen = (draw & 1) if x == y else int(x < y)
                        if chosen:
                            q = other
                    source = 0 if mode == "yoked" else bath ^ 1 if mode == "opposite" else bath
                    food = maps[sols[source]][q % 4] == q // 4
                    if mode != "frozen":
                        state = learned(g, state, presented(q), int(food))
                    marks[q] = food
                    tot[7] += 1
                competing = maps[sols[(bath ^ 1 if mode == "opposite" else bath) ^ 1]]
                tot[9] += any((competing[q % 4] == q // 4) != v for q, v in marks.items())
                left, right, _ = scores(g, state, presented(target[0]), presented(target[1]))
                choice = (k >> 12 & 1) if left == right else int(right > left)
                good = maps[sols[bath]][sites[choice]] == chip
                carbon = min(
                    9, 2 + (1 + ((k >> 8) % 255)).bit_count() + (((0x30 | (chip << 2) | (k & 3)) >> 1) & 3)
                )
                tot[0] += 1
                tot[1] += 1 + good
                tot[2] += (1 + carbon) * good
                tot[3] += good
                tot[4 + bath] += good
                if bath:
                    tot[6] += choice != previous
                previous = choice
                tot[8] += left == right
                # Restrict to the known two-choice presentation. The cortex has
                # no access to this calculation or to the physical mechanism.
                compatible = [
                    m
                    for m in maps
                    if all((m[q % 4] == q // 4) == v for q, v in marks.items())
                    and chip in (m[sites[0]], m[sites[1]])
                ]
                forced = len({m[sites[0]] == chip for m in compatible}) == 1
                tot[10] += forced
                tot[11] += forced and good
    names = "choices paid food correct first second changed observed ties distinguishable deducible solved_deducible".split()
    assert tot == [int(arm[k]) for k in names], (mode, dict(zip(names, tot)), arm)


def test(tmp):
    env = dict(os.environ, AXION_JOURNAL_DIR=str(tmp / "journal"))

    def run(*args, stdin=None, ok=True):
        r = subprocess.run([EXE, *map(str, args)], cwd=ROOT, env=env, input=stdin, text=True, capture_output=True)
        assert (r.returncode == 0) == ok, (args, r.stderr, r.stdout[-500:])
        return r.stdout if ok else r.stderr

    def lake(*args, **kw):
        return run("floodplain", *args, **kw)

    bare = tmp / "bare.flood"
    two = tmp / "two.flood"
    t1 = tmp / "mid.flood"
    final = tmp / "final.flood"
    split = tmp / "split.flood"
    common = [
        "--side",
        16,
        "--seed",
        119,
        "--density",
        64,
        "--neurons",
        256,
        "--worksites",
        "active",
        "--construction-tide",
        8,
        "--grains",
        68,
    ]
    lake(*common, "--brine", 2, "--steps", 19, "--save", two)
    lake("--load", two, "--brine", 5, "--steps", 0, "--save", bare)
    old = snapshot(two)
    opening = snapshot(bare)
    assert opening["stock"][:2] == old["stock"][:2] and all(opening["stock"][2:]) and old["stock"][2:] == [0, 0, 0]
    assert all((a & 4095) == b for a, b in zip(opening["salts"], old["salts"]))
    for before, n, after in ((bare, 37, final), (bare, 11, t1), (t1, 26, split)):
        lake("--load", before, "--steps", n, "--save", after)
    a = snapshot(final)
    assert a["raw"] == split.read_bytes() and a["stock"] == opening["stock"] and a["salts"] != opening["salts"]
    lake("--load", bare, "--steps", 1, "--save", split)
    tide = snapshot(split)
    expected = opening["salts"][:]
    tick = opening["tick"]
    for i in range(128):
        vertical = (tick >> 1) & 1
        x = i % 16 if vertical else (2 * (i % 8) + (tick & 1)) % 16
        y = (2 * (i // 16) + (tick & 1)) % 16 if vertical else i // 8
        p = y * 16 + x
        q = ((y + vertical) % 16) * 16 + (x + 1 - vertical) % 16
        direction = opening["soil"][p][0] - tide["soil"][p][0]
        if not direction:
            continue
        key = mix(119 ^ mix(tick * 929 + p))
        hi, lo = (p, q) if direction > 0 else (q, p)
        old_volume = opening["soil"][hi][0]
        for ion in range(5):
            high = (expected[hi] >> (6 * ion)) & 63
            low = (expected[lo] >> (6 * ion)) & 63
            amount = min(63 - low, (abs(direction) * high + mix(key + ion * 531) % old_volume) // old_volume)
            expected[hi] -= amount << (6 * ion)
            expected[lo] += amount << (6 * ion)
    assert expected == tide["salts"], "dissolved component did not follow the measured local water movement"
    assert len(set(map(tuple, (bases(i) for i in range(32))))) == 24
    output = lake(
        "--load",
        final,
        "--steps",
        0,
        "--work-subjects",
        32,
        "--brine-assay",
        3,
        "--brine-examples",
        9,
        "--work-test-seed",
        273,
        "--assay-inquiry",
        "--brine-bench",
        4096,
        "--legacy",
        12,
        "--material-legacy",
        16,
        "--hydraulic-trial",
        16,
        "--save",
        split,
    )
    assert split.read_bytes() == a["raw"]
    arms = parse(output, "brine_transfer")
    assert len(arms) == 12
    for arm in arms:
        matched(a, arm, 273)
    compiled = parse(output, "brine_deployment")[0]
    assert (
        int(compiled["comparisons"]) == 80 * sum(bool(e) for e in a["energy"])
        and int(compiled["settled_bytes"]) == 6
    )
    still = tmp / "still.flood"
    lake("--load", final, "--brine-flow", "still", "--steps", 43, "--save", still)
    assert snapshot(still)["salts"] == a["salts"]
    lake("--load", final, "--extend", 32, "--neurons", 512, "--steps", 0, "--save", split)
    large = snapshot(split)
    assert large["stock"] == a["stock"]
    assert all(v == large["salts"][i % 16 + 32 * (i // 16)] for i, v in enumerate(a["salts"]))
    assert large["learners"][:256] == a["learners"]
    corrupt = bytearray(a["raw"][:-8])
    corrupt[a["salt_pos"] + 48] ^= 1
    t1.write_bytes(corrupt + checksum(corrupt))
    assert "dissolved" in lake("--load", t1, "--steps", 0, ok=False)
    assert "discard" in lake("--load", final, "--brine", 2, "--steps", 0, ok=False)
    lake(*common, "--brine", 5, "--soil", final, "--steps", 0, "--save", split)
    transplant = snapshot(split)
    assert transplant["salts"] == a["salts"] and all(not (g[19] | g[20]) for g in transplant["learners"])
    speaker = ROOT / "examples/tide-surveyor.mind"
    listener = ROOT / "examples/shore-reader.mind"
    returned = tmp / "returned.mind"
    placed = tmp / "placed.flood"
    lake("--side", 16, "--resident", speaker, "--plasticity", "closed", "--steps", 1, "--save", placed)
    placed_body = snapshot(placed)
    assert placed_body["voices"] == [0]
    lake("--load", placed, "--steps", 0, "--slot", 0, "--export-mind", returned)

    def tail(path):
        data = path.read_bytes()
        signature, extent = struct.unpack_from("<II", data, -16)
        assert signature == 0x42524E45 and extent == 92 and data[-8:] == checksum(data[:-8])
        before = len(data) - 8 - extent
        word_tag, size = struct.unpack_from("<II", data, before - 8)
        assert word_tag == 0x51464C44 and size == 336
        return data[before - 336 : -8]

    assert tail(speaker) == tail(returned)
    commands = "/brine-table 4001 1 5\nwhere would you try hare?\n/consult-water 0 9\nwhere would you try hare?\nwhere would you avoid hare?\n/try-grain 0\n/quit\n"
    for name in (speaker, returned):
        results = run("talk", "--speaker", name, "--listener", listener, stdin=commands)
        decisions = parse(results, "field_answer")
        assert [(r["field_answer"], r["receiver_site"], r["receiver_food"]) for r in decisions] == [
            ("newt", "6", "0"),
            ("vole", "9", "2"),
            ("newt", "6", "0"),
        ], results
        taps = parse(results, "brine_request")
        physical = parse(results, "brine_motion")[0]
        assert len(taps) == 9 and len({r["reference_coating"] for r in taps}) >= 2
        assert (physical["chosen"], physical["food"]) == ("1", "2")
    results = run(
        "talk",
        "--speaker",
        speaker,
        "--listener",
        listener,
        stdin="/brine-table 4001 1 5\n/brine-dialogues 64 5 9 45312 choice\n/quit\n",
    )
    social = {v["brine_dialogue"]: v for v in parse(results, "brine_dialogue")}
    assert len(social) == 8
    for mode, r in social.items():
        assert int(r["questions"]) and r["policy_agreement"] == r["questions"] and int(r["target_sampled"]) == 0
        assert int(r["correct"]) + int(r["wrong"]) + int(r["silent"]) == int(r["questions"])
    assert (
        int(social["reference_quiet"]["witnessed"]) == 0
        and social["reference_quiet"]["before"] == social["reference_quiet"]["correct"]
    )
    assert (
        int(social["answer_quiet"]["acted"]) == 0
        and social["answer_quiet"]["correct"] == social["answer_foil"]["correct"] == social["consulted"]["correct"]
    )
    assert int(social["answer_foil"]["harvested"]) + int(social["consulted"]["harvested"]) == int(
        social["consulted"]["food_requests"]
    )
    assert int(social["autonomous"]["witnessed"]) >= int(social["prior_confidence"]["witnessed"])
    lake("--load", placed, "--steps", 8192, "--save", split)
    assert 0 not in snapshot(split)["voices"]
    print(
        "PASS independent five-solute conservation, all twenty-four physical laws, twelve complete neural and sampling interventions, spatial expansion, restart, detached exposures, six-byte deployment, paid reference dialogue and owner-specific memory"
    )


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-brine-test-") as tmp:
        test(Path(tmp))
