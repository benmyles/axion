#!/usr/bin/env python3
"""Generate unheard physical work orders and independently audit their spoken execution."""

import argparse
import csv
import itertools
import random
from collections import Counter, defaultdict, deque
from functools import lru_cache
from pathlib import Path

PARTS = ("amber", "slate", "flint", "ivory")
FACES = ("brook", "pool", "rill", "mere")
EDGES = tuple(itertools.combinations(range(4), 2))
MASK = (1 << 32) - 1


def mix(x):
    x &= MASK
    x = ((x ^ (x >> 16)) * 0x21F0AAAD) & MASK
    x = ((x ^ (x >> 15)) * 0x735A2D97) & MASK
    return x ^ (x >> 15)


def pick(mask, draw):
    population = [i for i in range(8) if mask & (1 << i)]
    return population[draw % len(population)] if population else None


def permute(key):
    a = [0, 1, 2, 3]
    for i in (3, 2, 1):
        j = mix(key + 821 * i) % (i + 1)
        a[i], a[j] = a[j], a[i]
    return a


def pack(a):
    return sum(v << (2 * i) for i, v in enumerate(a))


def unpack(v, n):
    return tuple((v >> (2 * i)) & 3 for i in range(n))


class Fixture:
    def __init__(self, world, fixture, n, *, assembly_seed=None):
        self.n = n
        seed = mix(world + fixture * 5581) if assembly_seed is None else assembly_seed
        layout = mix(seed + 8011)
        rot = mix(layout + 8527)
        self.glyphs = (
            tuple(permute(mix(layout + 239))[:n])
            if rot & 1
            else tuple(mix(layout + 71 * j + 449) & 3 for j in range(n))
        )
        self.laws = []
        for t in range(4):
            k = mix(world + 3607 * t + 3529)
            self.laws.append(
                (
                    permute(mix(k + 1447)),
                    1 + mix(k + 6931) % 14,
                    k >> 17 & 1,
                    k >> 21 & 3,
                )
            )
        self.labels = permute(mix(fixture + 1367))
        self.buttons = [
            permute(mix(world + 3457 * fixture + 2797 * i))[:2] for i in range(4)
        ]
        self.reader = permute(mix(fixture + 3817))
        if self.reader == [0, 1, 2, 3]:
            self.reader = list(unpack(0x39, 4))
        self.sockets = tuple(i for i, (_, b) in enumerate(EDGES) if b < n)
        self.joints = tuple(
            1 + ((mix(seed + i * 263) >> 6) & 1)
            if i in self.sockets and mix(seed + i * 263) % 5 < 2
            else 0
            for i in range(6)
        )
        k = mix(seed + 7057)
        self.pose = unpack(k >> 9, n)
        self.grip = sum(
            3 << (2 * ((k // 37 + j) % n))
            for j in range(1 + int(n > 2 and bool(k & 2)))
        )
        switched = self.joints
        if (k & 7) and self.sockets:
            switched = self.fitted(self.sockets[(k >> 17) % len(self.sockets)])
        pose = self.pose
        for j in range(2 + ((k >> 4) % 8)):
            pose = self.stroke(switched, pose, pick(self.grip, mix(k + 251 * j)))
        if pose == self.pose:
            pose = self.stroke(switched, pose, pick(self.grip, k))
        self.mask = (1 << (2 * n)) - 1
        self.goal = ((k >> 17) & self.mask) if (k & 31) == 1 else self.colors(pose)

    def colors(self, pose):
        return pack([self.laws[g][0][p] for g, p in zip(self.glyphs, pose)])

    def fitted(self, slot):
        a, _ = EDGES[slot]
        e = list(self.joints)
        e[slot] = 1 + ((self.laws[self.glyphs[a]][3] ^ (e[slot] >> 1)) & 1)
        return tuple(e)

    def stroke(self, joint, pose, action):
        p, h = divmod(action, 2)
        _, notches, handed, relay = self.laws[self.glyphs[p]]
        moved = list(pose)
        moved[p] = (pose[p] + (-1 if h ^ handed else 1)) % 4
        for j, edge in enumerate(EDGES):
            if not joint[j] or p not in edge:
                continue
            f = self.laws[self.glyphs[p]][0][pose[p]]
            if joint[j] == 2 and not ((notches >> f) & 1):
                continue
            b = edge[0] if edge[1] == p else edge[1]
            impulse = h ^ ((relay >> (joint[j] - 1)) & 1)
            moved[b] = (
                pose[b] + (-1 if impulse ^ self.laws[self.glyphs[b]][2] else 1)
            ) % 4
        return tuple(moved)

    def shortest(self, faces, mask, barred=0, bare=False):
        result = 255
        for edit in [None, *(() if bare else self.sockets)]:
            joints = self.joints if edit is None else self.fitted(edit)
            price = 0 if edit is None else 2
            if price >= result or (edit is not None and joints == self.joints):
                continue
            queue = deque([(self.pose, 0)])
            visited = {self.pose}
            while queue:
                state, depth = queue.popleft()
                if not ((self.colors(state) ^ faces) & mask):
                    result = min(result, depth + price)
                    break
                if depth >= 12 or price + depth + 1 >= result:
                    continue
                for a in range(2 * self.n):
                    if (self.grip & ~barred) & (1 << a):
                        adjacent = self.stroke(joints, state, a)
                        if adjacent not in visited:
                            visited.add(adjacent)
                            queue.append((adjacent, depth + 1))
        return result

    def question(self, colors, which, rng):
        which = list(which)
        rng.shuffle(which)
        return "choose " + " ".join(
            f"{PARTS[self.labels[i]]} {FACES[(colors >> (2 * i)) & 3]}" for i in which
        )

    def decode(self, sounds, barred=0):
        words = sounds.split()
        if not words or words[0] != "go" or len(words) % 2 == 0:
            return None
        pose, joints, turns = self.pose, self.joints, []
        edit = None
        for j in range(1, len(words), 2):
            if words[j] not in PARTS:
                return None
            i = self.labels.index(PARTS.index(words[j]))
            if i >= self.n:
                return None
            arg = words[j + 1]
            if arg in PARTS:
                other = self.labels.index(PARTS.index(arg))
                if i == other or other >= self.n or j != 1:
                    return None
                edit = EDGES.index(tuple(sorted((i, other))))
                joints = self.fitted(edit)
            elif arg in FACES and FACES.index(arg) in self.buttons[i]:
                a = 2 * i + self.buttons[i].index(FACES.index(arg))
                if not ((self.grip & ~barred) >> a & 1) or len(turns) >= 12:
                    return None
                pose = self.stroke(joints, pose, a)
                turns.append(a)
            else:
                return None
        return self.colors(pose), edit, turns


@lru_cache(maxsize=128)
def fixture(world, key, n):
    return Fixture(world, key, n)


def generate(args):
    rng = random.Random(args.seed)
    with args.path.open("w") as f:
        for j in range(args.cases):
            w, k, n = rng.getrandbits(32), rng.getrandbits(32), 4 if j % 8 else 3
            r = fixture(w, k, n)
            first = r.question(r.goal ^ (3 if j % 4 == 0 else 0), (j % n,), rng)
            second = r.question(r.goal, range(n), rng)
            ban = 0
            if j % 3 == 0:
                first = second
                prohibited = rng.randrange(n)
                second = "without " + PARTS[r.labels[prohibited]]
                ban = 3 << (2 * prohibited)
            elif j % 3 == 1:
                second = r.question(r.goal, range(n), rng)
            cost = r.shortest(r.goal, r.mask, ban)
            for arm, lessons, cut in (
                ("heard", 2, 0),
                ("masked", 2, 1),
                ("wrong_mark", 2, 2),
                ("wrong_part", 2, 3),
                ("untutored", 0, 0),
            ):
                print(
                    f"{w} {k} {n} {lessons} {cut} | {first} | {second} | why | {arm} {r.mask} {r.goal} {cost}",
                    file=f,
                )


def audit(args):
    counts = defaultdict(Counter)
    with args.path.open() as f:
        for row in csv.DictReader(f, delimiter="\t"):
            arm, s = row["arm"], counts[row["arm"]]
            n = {k: int(v) for k, v in row.items() if v.isdigit()}.__getitem__
            r = fixture(n("world"), n("fixture"), n("width"))
            assert (
                r.colors(r.pose),
                pack(r.glyphs),
                r.grip,
                pack(r.labels),
                pack(itertools.chain.from_iterable(r.buttons)),
                pack(r.reader),
            ) == tuple(
                n(k) for k in ("start", "glyphs", "grip", "labels", "buttons", "reader")
            )
            matrix = sum(
                v << (8 * a + 2 * b) | v << (8 * b + 2 * a)
                for v, (a, b) in zip(r.joints, EDGES)
            )
            assert matrix == n("joints") and n("goal") == r.goal and n("care") == r.mask
            act = r.decode(row["heard"], n("blocked"))
            assert (act is not None) == bool(n("executed")), row
            s["trials"] += 1
            cost = r.shortest(r.goal, r.mask, n("blocked"))
            s["possible"] += cost < 255
            s["initially_aligned"] += r.goal == r.colors(r.pose)
            if row["reason"]:
                idx = r.labels.index(PARTS.index(row["reason"]))
                s["reasons"] += 1
                s["physical_reasons"] += (
                    r.shortest(r.goal, r.mask, n("blocked") | (3 << (2 * idx))) == 255
                )
            if act is None:
                assert n("final") == n("start") and n("paid") == 0
                continue
            after, edit, turns = act
            assert (
                after,
                len(turns),
                int(edit is not None),
                len(turns) + 2 * (edit is not None),
            ) == (n("final"), n("strokes"), n("inserted"), n("paid"))
            local, world = (
                n("local_low") | (n("local_high") << 24),
                n("world_low") | (n("world_high") << 24),
            )
            received = [(local >> (3 * j)) & 7 for j in range(len(turns))]
            assert turns == [(world >> (3 * j)) & 7 for j in range(len(turns))]
            assert turns == [2 * r.reader[t // 2] + t % 2 for t in received]
            solved = after == r.goal
            s["aligned"] += solved
            s["minimal"] += solved and n("paid") == cost
            s["remapped"] += solved and received != turns
            s["spliced"] += solved and edit is not None
            s["indispensable"] += (
                solved
                and edit is not None
                and r.shortest(r.goal, r.mask, n("blocked"), True) == 255
            )
            s["multi"] += solved and len(turns) >= 3
            assert n("food") == (40 if solved else 0)
    for arm, c in counts.items():
        print(arm, *[f"{k}={v}" for k, v in c.items()])
    return counts


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("generate", "audit"))
    parser.add_argument("path", type=Path)
    parser.add_argument("--seed", type=int, default=37003)
    parser.add_argument("--cases", type=int, default=384)
    opt = parser.parse_args()
    (generate if opt.action == "generate" else audit)(opt)
