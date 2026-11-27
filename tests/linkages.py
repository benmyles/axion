#!/usr/bin/env python3
"""Grade motor replay, social instruction, physical budgets and tissue persistence independently."""

import importlib.util
import os
import tempfile
from collections import deque
from pathlib import Path
from types import SimpleNamespace

from press import call, rows
from worksites import ROOT, snapshot

spec = importlib.util.spec_from_file_location(
    "rack", ROOT / "tools/linkage-language.py"
)
rack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rack)


def field(*args, **kwargs):
    return call("floodplain", *args, **kwargs)


def physical_rows(log):
    data = {}
    for q in rows(log, "linkage_mechanism"):
        data[(q["linkage_mechanism"], int(q["material"]))] = [
            int(q[k]) for k in ("true_local", "true_mesh", "true_belt")
        ]
    for q in rows(log, "linkage_scene"):
        uid = q["linkage_scene"]
        get = {key: int(value) for key, value in q.items()}.__getitem__
        tint = rack.unpack(get("materials"), 4)
        joints = get("joints")
        moves = [i for i in range(8) if get("handles") & (1 << i)]

        def transition(state, motor, uid=uid, tint=tint, joints=joints):
            w = list(rack.unpack(state, 4))
            a, h = divmod(motor, 2)
            z = w.copy()
            row = w[a] + 4 * h
            z[a] = data[uid, tint[a]][0] >> (2 * row) & 3
            for j in range(4):
                edge = joints >> (8 * a + 2 * j) & 3
                if edge:
                    impulse = (data[uid, tint[a]][edge] >> (2 * row)) & 3
                    if impulse:
                        z[j] = (
                            data[uid, tint[j]][0] >> (2 * (w[j] + 4 * (impulse - 1)))
                            & 3
                        )
            return rack.pack(z)

        state = get("start")
        code = get("low") | (get("high") << 24)
        for i in range(get("length")):
            a = code >> (3 * i) & 7
            assert a in moves
            state = transition(state, a)
        assert state == get("finish") and int(
            not ((state ^ get("target")) & get("care"))
        ) == get("solved")
        start = get("start")
        seen = {start}
        queue = deque([(start, 0)])
        cost = 2**32 - 1
        while queue:
            here, t = queue.popleft()
            if not ((here ^ get("target")) & get("care")):
                cost = t
                break
            if t == 12:
                continue
            for a in moves:
                future = transition(here, a)
                if future not in seen:
                    seen.add(future)
                    queue.append((future, t + 1))
        assert cost == get("minimum")

    for row in rows(log, "linkage_product"):
        q = {k: int(v) for k, v in row.items()}
        seed = q["linkage_product"]
        r = rack.Fixture(seed, 0, q["parts"], assembly_seed=seed)
        assert (rack.pack(r.glyphs), r.grip, r.colors(r.pose), r.mask, r.goal) == (
            q["materials"],
            q["grip"],
            q["start"],
            q["care"],
            q["target"],
        )

        def adjacency(edges):
            return sum(
                v << (8 * a + 2 * b) | v << (8 * b + 2 * a)
                for v, (a, b) in zip(edges, rack.EDGES)
            )

        assert adjacency(r.joints) == q["before"]
        acquired = q["fitting"] < 6
        formed = r.fitted(q["fitting"]) if acquired else r.joints
        assert adjacency(formed) == q["after"] and q["loose"] == 1 - int(acquired)
        pose = r.pose
        encoded = q["low"] | (q["high"] << 24)
        for k in range(q["strokes"]):
            action = (encoded >> (3 * k)) & 7
            assert r.grip & (1 << action)
            pose = r.stroke(formed, pose, action)
        assert (r.colors(pose), q["strokes"] + 2 * acquired) == (q["finish"], q["cost"])
        full = r.shortest(r.goal, r.mask)
        bare = r.shortest(r.goal, r.mask, bare=True)
        assert (q["optimum"], q["without"]) == tuple(
            (2**32 - 1 if cost == 255 else cost) for cost in (full, bare)
        )
        assert q["food"] == (40 if r.colors(pose) == r.goal else 0)


def main():
    with tempfile.TemporaryDirectory(
        prefix="axion-couplers-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as temp:
        work = Path(temp)
        os.environ["AXION_JOURNAL_DIR"] = str(work / "journal")
        germ = ROOT / "examples/spindlebank.flood"
        guide = ROOT / "examples/rack-guide.mind"
        peer = ROOT / "examples/causeway-guide.mind"
        first = work / "first.flood"
        mid = work / "mid.flood"
        end = work / "end.flood"
        whole = work / "full.flood"
        field(
            "--load",
            germ,
            "--linkage-attention",
            "focal",
            "--linkage-weather",
            "wet",
            "--steps",
            0,
            "--save",
            first,
        )
        field("--load", first, "--steps", 69, "--save", mid)
        field("--load", mid, "--steps", 124, "--save", end)
        log = field(
            "--load",
            first,
            "--steps",
            193,
            "--linkage-study",
            191,
            "--linkage-witnesses",
            37,
            "--linkage-pieces",
            4,
            "--linkage-census",
            "--linkage-bench",
            8543,
            "--save",
            whole,
        )
        assert end.read_bytes() == whole.read_bytes()
        climate = field(
            "--load",
            germ,
            "--linkage-climate",
            "changing",
            "--linkage-germs",
            germ,
            "--linkage-rear",
            5,
            "--linkage-population",
            32,
            "--linkage-recall",
            37,
            "--steps",
            0,
        )
        shifts = {
            (q["linkage_recall"], q["season"]): q
            for q in rows(climate, "linkage_recall")
        }
        assert len(shifts) == 12
        for q in shifts.values():
            assert int(q["cases"]) == 74 and int(q["completed"]) >= int(q["built"])
            assert int(q["proposed"]) - int(q["mistaken"]) <= int(q["completed"])
        assert (
            shifts["steady", "0"]["completed"] == shifts["changing", "0"]["completed"]
        )
        moisture = rows(log, "linkage_moisture")
        assert moisture and len(moisture) == int(moisture[0]["bins"])
        actual = sum(
            int(row["counts"][j : j + 3], 16)
            for row in moisture
            for j in range(0, len(row["counts"]), 3)
        )
        assert actual == int(rows(log, "linkage_inspection")[0]["hydrated_plots"])
        physical_rows(log)
        b = snapshot(whole)
        n = b["side"] ** 2
        assert len(b["inventors"]) == n
        for index, m in enumerate(b["inventors"]):
            for i in range(4):
                if not ((m[4] >> (4 * i)) & 15):
                    assert not any(b["circuits"][n * i + index])
        re = rows(log, "linkage_field")[-1]
        assert int(re["heat"]) == b["machinery"][8] and int(re["heat"]) > 0
        stats = rows(log, "linkage_compile")
        assert stats and stats[0]["origin"] == "practiced"
        for q in stats:
            if "trials" not in q:
                continue
            assert (
                int(q["checked"]),
                int(q["image_bytes"]),
                int(q["differences"]),
            ) == (8543 * 6, 128, 0)
            assert (
                sum(
                    int(q[k])
                    for k in ("productive_states", "already_aligned", "no_path")
                )
                == int(q["linkage_compile"]) * 256
            )
        att = {q["linkage_attention"]: q for q in rows(log, "linkage_attention")}
        assert (
            int(att["focal"]["achieved"])
            >= int(att["heritable"]["achieved"])
            >= int(att["whole"]["achieved"])
        )
        assert all(q["false"] == "0" for q in att.values())
        for q in rows(log, "linkage_fabrication"):
            assert int(q["heat"]) == int(q["strokes"]) + 2 * int(q["threaded"]) and int(
                q["carbon"]
            ) == 40 * int(q["produced"])
        for q in rows(log, "linkage_bequest"):
            if q["linkage_bequest"] == "as_made":
                assert int(q["loose"]) + int(q["loops"]) == int(q["cases"])
            if q["linkage_bequest"] == "unstrung":
                assert q["loose"] == q["cases"] and int(q["novel_uses"]) == 0
        widened = work / "wide.flood"
        field("--load", whole, "--extend", 128, "--steps", 0, "--save", widened)
        w = snapshot(widened)
        assert w["inventors"][:n] == b["inventors"]
        for j in range(4):
            assert (
                w["circuits"][16384 * j : 16384 * j + n]
                == b["circuits"][n * j : n * j + n]
            )
        flat = work / "replaced.flood"
        field("--load", whole, "--disturb-linkages", "--steps", 0, "--save", flat)
        washed = snapshot(flat)
        assert (
            washed["inventors"] == b["inventors"]
            and washed["circuits"] == b["circuits"]
            and not any(v[0] for v in washed["seals"])
        )
        field(
            "--load",
            whole,
            "--linkage-contest",
            "paired",
            "--hollow-contest",
            "paired",
            "--steps",
            0,
            ok=False,
        )
        field("--load", whole, "--linkage-carrier", peer, "--steps", 0, ok=False)
        requests = work / "requests.txt"
        trace = work / "spoken.tsv"
        saved = work / "experienced.mind"
        rack.generate(SimpleNamespace(cases=37, seed=19071, path=requests))
        script = f"/rack 381\n/rack-trials {requests} {trace}\n/rack-study 2\n/quit\n"
        call(
            "talk",
            "--speaker",
            guide,
            "--listener",
            peer,
            "--save-speaker",
            saved,
            stdin=script,
        )
        v = rack.audit(SimpleNamespace(path=trace))
        assert (
            v["heard"]["aligned"] == v["heard"]["possible"]
            and v["heard"]["aligned"] > 10
            and not v["masked"]["aligned"]
        )
        blank = guide.read_bytes()
        stored = saved.read_bytes()
        # Optional tail precedes the file checksum; tissue has four 32-byte private banks.
        firstbank = -8 - 12 - 128
        assert (
            blank[-16:-12] == stored[-16:-12]
            and not any(blank[firstbank:-20])
            and any(stored[firstbank:-20])
        )
        replay = call(
            "talk",
            "--speaker",
            saved,
            "--listener",
            peer,
            stdin="/rack 381\n/rack-order\n/quit\n",
        )
        assert "work order> choose" in replay
        print(
            "GPU work, independent reachability and speech, real carbon, deterministic restart, expansion and portable tissue passed"
        )


if __name__ == "__main__":
    main()
