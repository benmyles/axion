#!/usr/bin/env python3
"""Generate indistinguishable material scenes; enumerate common programs and independently replay public decisions."""

import argparse
import csv
import heapq
import importlib.util
import itertools
import random
from collections import Counter, defaultdict
from functools import lru_cache
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    "mechanics", Path(__file__).with_name("linkage-language.py")
)
mechanics = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mechanics)
mix = mechanics.mix
PARTS, FACES = mechanics.PARTS, mechanics.FACES


class Pair:
    def __init__(self, world, fixture, width):
        key = mix(world + fixture * 5581)
        self.fixtures = [
            mechanics.Fixture(mix(world + salt), fixture, width, assembly_seed=key)
            for salt in (7807, 19121)
        ]
        public = self.fixtures[(key >> 11) & 1]
        self.rig = public
        self.start = public.colors(public.pose)
        self.nominal = public.goal
        self.buttons = [
            mechanics.permute(mix(world + 3457 * fixture + 2797 * i))[:2]
            for i in range(4)
        ]
        self.moves = [i for i in range(2 * width) if public.grip >> i & 1]
        # Orient each actual wheel to the same externally visible starting colour.
        self.initial = tuple(
            tuple(
                r.laws[g][0].index(f)
                for g, f in zip(r.glyphs, mechanics.unpack(self.start, width))
            )
            for r in self.fixtures
        )
        self.joints = {e: public.fitted(e) for e in public.sockets}

    def advance(self, climate, angles, action, attached):
        model = self.fixtures[climate]
        return model.stroke(
            model.joints if attached is None else model.fitted(attached), angles, action
        )

    def reachable(self, worlds, phases, limit, loop):
        # Exhaust all shared action programs up to the real wallet, allowing
        # installation anywhere in the sequence. Cost, aperture and spool are physical.
        initial = tuple(phases[i] for i in worlds)
        serial = itertools.count()
        q = [(0, next(serial), initial, None)]
        visited = {(initial, None): 0}
        result = {}
        while q:
            work, _, state, bond = heapq.heappop(q)
            if visited[state, bond] != work:
                continue
            colors = tuple(self.fixtures[j].colors(v) for j, v in zip(worlds, state))
            result[colors] = min(result.get(colors, 255), work)
            if work >= limit:
                continue
            for act in self.moves:
                dest = tuple(
                    self.advance(j, v, act, bond) for j, v in zip(worlds, state)
                )
                if visited.get((dest, bond), 255) > work + 1:
                    visited[dest, bond] = work + 1
                    heapq.heappush(q, (work + 1, next(serial), dest, bond))
            if bond is None and loop and work + 2 <= limit:
                for e in self.rig.sockets:
                    if visited.get((state, e), 255) > work + 2:
                        visited[state, e] = work + 2
                        heapq.heappush(q, (work + 2, next(serial), state, e))
        return result

    def categories(self, wallet, loop):
        single = [
            {
                key[0]: v
                for key, v in self.reachable((j,), self.initial, wallet, loop).items()
            }
            for j in (0, 1)
        ]
        both = set(single[0]) & set(single[1])
        both.discard(self.start)
        shared = {
            a for a, b in self.reachable((0, 1), self.initial, wallet, loop) if a == b
        }
        distinguishing = {}
        for a in self.moves:
            after = tuple(self.advance(j, self.initial[j], a, None) for j in (0, 1))
            face = [r.colors(v) for r, v in zip(self.fixtures, after)]
            if face[0] == face[1]:
                continue
            suffix = [
                {key[0] for key in self.reachable((j,), after, wallet - 1, loop)}
                for j in (0, 1)
            ]
            for goal in (suffix[0] & suffix[1] & both) - shared:
                distinguishing[goal] = True
        return single, sorted(both & shared), sorted(distinguishing)

    def decode_query(self, words):
        w = words.split()
        if (
            len(w) != 5
            or w[0] != "turn"
            or w[3] != "face"
            or w[1] not in PARTS
            or w[2] not in FACES
            or w[4] not in PARTS
        ):
            return None
        ix = self.rig.labels.index(PARTS.index(w[1]))
        look = self.rig.labels.index(PARTS.index(w[4]))
        if (
            ix >= self.rig.n
            or look >= self.rig.n
            or FACES.index(w[2]) not in self.buttons[ix]
        ):
            return None
        a = 2 * ix + self.buttons[ix].index(FACES.index(w[2]))
        return (a, look) if a in self.moves else None

    def follow(self, words, weather, state, wallet, stock):
        w = words.split()
        if not w or w[0] != "go" or len(w) % 2 != 1:
            return None
        e = None
        taps = 0
        work = 0
        for j in range(1, len(w), 2):
            if w[j] not in PARTS:
                return None
            part = self.rig.labels.index(PARTS.index(w[j]))
            arg = w[j + 1]
            if part >= self.rig.n:
                return None
            if arg in PARTS:
                other = self.rig.labels.index(PARTS.index(arg))
                if other >= self.rig.n or part == other or j != 1 or not stock:
                    return None
                e = mechanics.EDGES.index(tuple(sorted((part, other))))
                work += 2
            elif arg in FACES:
                f = FACES.index(arg)
                if f not in self.buttons[part]:
                    return None
                a = 2 * part + self.buttons[part].index(f)
                if a not in self.moves:
                    return None
                state = self.advance(weather, state, a, e)
                work += 1
                taps += 1
            else:
                return None
        return (state, work, taps, int(e is not None)) if work <= wallet else None


@lru_cache(maxsize=64)
def fixture(w, f, n):
    return Pair(w, f, n)


def generate(o):
    rng = random.Random(o.seed)
    classes = Counter()
    scanned = 0
    amend = getattr(o, "action", "generate") == "amend"
    with o.path.open("w") as file:
        while (
            min(
                classes.get(x, 0) for x in ("needs_observation", "common", "impossible")
            )
            < o.cases
            and scanned < o.cases * 150
        ):
            w = rng.getrandbits(32)
            f = rng.getrandbits(32)
            width = 2 + scanned % 3
            wallet = 3 + scanned % 4
            loop = int(scanned % 4 != 0)
            scanned += 1
            pair = fixture(w, f, width)
            single, common, branches = pair.categories(wallet, loop)
            impossible = [
                z
                for z in range(1 << (2 * width))
                if z not in single[0] and z not in single[1]
            ]
            for group, goals in (
                ("needs_observation", branches),
                ("common", common),
                ("impossible", impossible),
            ):
                if not goals or classes[group] >= o.cases:
                    continue
                goal = rng.choice(goals)
                mask = (1 << (2 * width)) - 1
                question = pair.rig.question(goal, range(width), rng)
                revision = ""
                if amend:
                    prior = [x for x in branches if x != goal]
                    if not prior:
                        continue
                    question = pair.rig.question(rng.choice(prior), range(width), rng)
                    revision = " | " + pair.rig.question(goal, range(width), rng)
                for name, wire, exposure in (
                    ("heard", 0, 2),
                    ("no_reply", 1, 2),
                    ("false_reply", 2, 2),
                    ("muted_probe", 3, 2),
                    ("no_question", 4, 2),
                    ("fixed_probe", 5, 2),
                    ("latest_only", 6, 2),
                    ("garbled_tail", 7, 2),
                    ("untrained", 0, 0),
                ):
                    for actual in (0, 1):
                        print(
                            f"{w} {f} {width} {actual} {exposure} {wire} {wallet} {loop} {goal} {mask} | {question} | {name} {group} {int(goal in single[actual])}"
                            + revision,
                            file=file,
                        )
                classes[group] += 1
    if min(classes.values(), default=0) < o.cases or len(classes) != 3:
        raise ValueError(f"insufficient matched questions {classes}")
    print("candidate_fixtures", scanned, *[f"{k}={v}" for k, v in classes.items()])


def audit(o):
    stats = defaultdict(Counter)
    twins = defaultdict(dict)
    known = {}
    with o.path.open() as f:
        for row in csv.DictReader(f, delimiter="\t"):
            n = {k: int(v) for k, v in row.items() if v.isdigit()}.__getitem__
            group, arm = row["group"], row["arm"]
            s = stats[(arm, group)]
            p = fixture(n("world"), n("fixture"), n("width"))
            r = p.rig
            w = n("hidden")
            assert (
                p.start,
                mechanics.pack(r.glyphs),
                r.grip,
                mechanics.pack(r.labels),
                mechanics.pack(itertools.chain.from_iterable(p.buttons)),
                mechanics.pack(r.reader),
            ) == tuple(
                n(k)
                for k in ("start", "glyphs", "handles", "labels", "buttons", "reader")
            )
            signed = sum(
                v << (8 * a + 2 * b) | v << (8 * b + 2 * a)
                for v, (a, b) in zip(r.joints, mechanics.EDGES)
            )
            assert signed == n("joints")
            target = n("goal")
            key = (n("world"), n("fixture"), n("width"), n("budget"), n("stock"))
            if key not in known:
                known[key] = p.categories(n("budget"), n("stock"))
            single, common, dist = known[key]
            assert (
                (group == "needs_observation" and target in dist)
                or (group == "common" and target in common)
                or (group == "impossible" and all(target not in v for v in single))
            ), (group, key, target)
            probe = p.decode_query(row["query"])
            pose = p.initial[w]
            charge = 0
            s["trials"] += 1
            if probe:
                act, look = probe
                pose = p.advance(w, pose, act, None)
                charge = 1
                observed = (p.fixtures[w].colors(pose) >> (2 * look)) & 3
                assert (
                    n("stroke") == act
                    and n("watch") == look
                    and n("seen") == observed
                    and n("local") == 2 * r.reader.index(act // 2) + act % 2
                )
                if row["report"]:
                    named = (observed + int(arm == "false_reply")) & 3
                    assert (
                        row["report"] == f"face {PARTS[r.labels[look]]} {FACES[named]}"
                    )
                s["physical_queries"] += 1
                s["sensed_neighbor"] += look != act // 2
            else:
                assert n("stroke") == 8
            second = p.decode_query(row.get("query2", ""))
            if second:
                act, look = second
                pose = p.advance(w, pose, act, None)
                charge += 1
                observed = (p.fixtures[w].colors(pose) >> (2 * look)) & 3
                assert (
                    n("second_move") == act
                    and n("second_watch") == look
                    and n("second_face") == observed
                    and n("second_local") == 2 * r.reader.index(act // 2) + act % 2
                )
                if row["report2"]:
                    assert (
                        row["report2"]
                        == f"face {PARTS[r.labels[look]]} {FACES[(observed + int(arm == 'false_reply')) & 3]}"
                    )
                s["second_queries"] += 1
            if row.get("revised"):
                s["revised_answers"] += 1
            result = p.follow(
                row["procedure"], w, pose, n("budget") - charge, n("stock")
            )
            assert bool(result) == bool(n("passed"))
            if result:
                pose, paid, strokes, fitted = result
                charge += paid
                assert (n("steps"), n("fitted")) == (strokes, fitted)
            after = p.fixtures[w].colors(pose)
            assert after == n("final") and charge == n("heat")
            solved = bool(result) and after == target
            assert n("food") == 40 * int(solved)
            s["paid"] += charge
            s["completed"] += solved
            s["fittings"] += int(solved and n("fitted"))
            twins[(arm, group, *key, target)][w] = (
                row["words"],
                (row["report"], row.get("report2", "")),
                row["procedure"],
                solved,
                row.get("revised", ""),
                row.get("query2", ""),
            )
    for key, seen in twins.items():
        assert set(seen) == {0, 1}
        a, b = seen[0], seen[1]
        assert a[0] == b[0], (key, a, b)
        s = stats[key[:2]]
        s["pairs"] += 1
        s["both"] += a[3] and b[3]
        s["different_followup"] += a[3] and b[3] and (a[2], a[5]) != (b[2], b[5])
        if key[1] == "needs_observation" and a[3] and b[3]:
            assert a[1] != b[1] and (a[2], a[5]) != (b[2], b[5])
    for key, v in stats.items():
        print(*key, *[f"{k}={x}" for k, x in v.items()])
    return stats


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("action", choices=("generate", "amend", "audit"))
    p.add_argument("path", type=Path)
    p.add_argument("--seed", type=int, default=26593)
    p.add_argument("--cases", type=int, default=96)
    opts = p.parse_args()
    (audit if opts.action == "audit" else generate)(opts)
