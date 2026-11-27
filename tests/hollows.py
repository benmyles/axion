#!/usr/bin/env python3
"""Independent spatial and carbon audits, lifetime learning and persistence through the CUDA interfaces."""

import importlib.util
import os
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT, checksum, mix, snapshot

spec = importlib.util.spec_from_file_location("marsh", ROOT / "tools/hollow-language.py")
marsh = importlib.util.module_from_spec(spec)
spec.loader.exec_module(marsh)
DIRECTIONS = ((0, -1), (1, 0), (0, 1), (-1, 0))


def expect_census(s, seed, minimum):
    side = s["side"]
    n = side**2
    slots = s["recess"][0]
    measured = dict.fromkeys(
        (
            "residents",
            "goals",
            "exact",
            "rotated_exact",
            "stored_carbon",
            "rotated_carbon",
            "work_distance",
            "certified",
        ),
        0,
    )
    delta = lambda place, origin: ((place - origin + 128) & 255) - 128
    for pos, owner in enumerate(s["slots"]):
        if not s["energy"][owner] or not s["limbs"][owner]:
            continue
        measured["residents"] += 1
        _, compass, law, _, mx, my, yaw, used, _ = s["rangers"][owner]
        k = mix(seed + owner * 2011)
        candidates = []
        for j in range(min(used, slots)):
            e = s["marks"][j * n + owner]
            if not e or (e >> 24) & 3 != 2:
                continue
            dx, dy = delta(e & 255, mx), delta(e >> 8 & 255, my)
            distance = abs(dx) + abs(dy)
            age = ((s["tick"] >> 3) - (e >> 28)) & 15
            priority = (
                (e >> 26 & 3) * (1 + (law & 15)) - distance * (law >> 8 & 3) - age * (law >> 10 & 1)
            )
            candidates.append((32 * priority + ((k + j * 11) & 31), distance, j, dx, dy))
        if not candidates:
            continue
        _, d, slot, dx, dy = max(candidates)
        if d < minimum:
            continue
        measured["goals"] += 1
        measured["work_distance"] += d

        def unit(ray, yaw=yaw, compass=compass, heading=s["heading"][owner]):
            a = (ray - yaw) % 4
            if compass & 64:
                a = -a
            return DIRECTIONS[(a + (compass >> 4 & 3) + heading) % 4]

        ex, ey = unit(1)
        sx, sy = unit(2)
        x = dx * ex + dy * sx
        y = dx * ey + dy * sy
        sign = -1 if k & 1 else 1

        def cell(xx, yy, pos=pos):
            return ((pos // side + yy) % side) * side + (pos % side + xx) % side

        target, rotated = cell(x, y), cell(sign * y, -sign * x)
        measured["stored_carbon"] += min(s["pockets"][target][0], 16)
        measured["rotated_carbon"] += min(s["pockets"][rotated][0], 16)
        if owner % 16 == 0:
            record = s["sightings"][slot * ((n + 15) // 16) + owner // 16]
            if record >> 31:
                measured["certified"] += 1
                measured["exact"] += (record & 0x7FFFFFFF) == target + 1
                measured["rotated_exact"] += (record & 0x7FFFFFFF) == rotated + 1
    return measured


def test(folder):
    os.environ["AXION_JOURNAL_DIR"] = str(folder / "journal")

    def flood(*args, **keys):
        return call("floodplain", *args, **keys)

    base = ROOT / "examples/hollowbank.flood"
    old = ROOT / "examples/reedbed.flood"
    mid, full, copy, broad = [folder / (q + ".flood") for q in ("mid", "full", "copy", "broad")]
    common = (
        "--hollow-current",
        "ferry",
        "--hollow-ground",
        "rich",
        "--hollow-seed",
        53119,
        "--hollow-contest",
        "paired",
    )
    flood("--load", base, *common, "--steps", 121, "--save", mid)
    flood("--load", mid, "--steps", 180, "--save", copy)
    run = flood(
        "--load",
        base,
        *common,
        "--steps",
        301,
        "--save",
        full,
        "--hollow-census",
        "--hollow-bench",
        131071,
    )
    assert full.read_bytes() == copy.read_bytes()
    s = snapshot(full)
    n = s["side"] ** 2
    b = s["recess"]
    assert b[3] > 0 and b[4] > 0 and b[6] > 0 and b[7] > 0
    assert sum(bool(m[7]) for m in s["rangers"]) > 0
    assert sum(bool(v & 0x80000000) for v in s["sightings"]) > 0
    for r in rows(run, "hollow_range"):
        expected = expect_census(s, 53119, int(r["hollow_range"]))
        assert all(int(r[k]) == v for k, v in expected.items()), (r, expected)
    packed = rows(run, "hollow_deploy")[0]
    assert (
        int(packed["extra_bytes"]) == 26
        and int(packed["verified"]) == 131071 * 6
        and packed["differences"] == "0"
    )
    for r in rows(run, "hollow_lineage"):
        tagged = sum(
            bool(i) and (t[8] >> 31) == int(r["hollow_lineage"])
            for i, t in zip(s["energy"], s["rangers"])
        )
        assert int(r["living"]) == tagged
    flood("--load", full, "--extend", 128, "--hollow-slots", 16, "--steps", 0, "--save", broad)
    larger = snapshot(broad)
    old_side = s["side"]
    big = larger["side"]
    nn = big * big
    assert larger["rangers"][:n] == s["rangers"]
    for y in range(old_side):
        assert (
            larger["pockets"][big * y : big * y + old_side]
            == s["pockets"][old_side * y : old_side * (y + 1)]
        )
    for j in range(b[0]):
        assert larger["marks"][j * nn : j * nn + n] == s["marks"][j * n : (j + 1) * n]
        for k, mark in enumerate(s["sightings"][j * (n // 16) : (j + 1) * (n // 16)]):
            site = (mark & 0x7FFFFFFF) - 1
            wide = 0 if site < 0 else 1 + (site // old_side) * big + site % old_side
            assert larger["sightings"][j * (nn // 16) + k] == (mark & 0x80000000) | wide
    assert "occupied spatial cells" in flood("--load", full, "--hollow-slots", 1, "--steps", 0, ok=False)
    tiny = rows(
        flood(
            "--load",
            base,
            "--hollow-study",
            96,
            "--hollow-optics",
            ROOT / "examples/wheel-reader.mind",
            "--steps",
            0,
        ),
        "hollow_trip",
    )
    study = {q["hollow_trip"]: int(q["fed"]) for q in tiny}
    assert study["reanchored"] > 60 and all(
        study[k] < 12 for k in ("places_erased", "prior_water", "landmark_exchanged", "one_cell")
    )
    # A genuine hatchling acquires no parent coordinates or lineage-independent history.
    now = folder / "one.flood"
    flood("--load", mid, "--steps", 1, "--save", now)
    young = snapshot(now)
    for i, age in enumerate(young["ages"]):
        if young["energy"][i] and age == 0:
            assert not young["rangers"][i][7] and all(not young["marks"][j * n + i] for j in range(b[0]))
            if i % 16 == 0:
                assert all(not young["sightings"][j * (n // 16) + i // 16] for j in range(b[0]))
    assert any(e and age == 0 for e, age in zip(young["energy"], young["ages"]))
    # Direct receipt with a substituted texture cannot re-anchor the old location.
    probe = flood(
        "--load",
        base,
        "--hollow-study",
        32,
        "--hollow-seed",
        7192,
        "--hollow-optics",
        ROOT / "examples/wheel-reader.mind",
        "--steps",
        0,
    )
    q = rows(probe, "hollow_path")[0]
    points, bath, tones, _ = marsh.terrain(int(q["hollow_path"]))
    assert [(int(r["x"]), int(r["y"])) for r in rows(probe, "hollow_site")] == points and int(
        q["bath"]
    ) == bath
    assert tones == [(int(q["material"]) >> (2 * j)) & 3 for j in range(4)]
    if int(q["delivered"]):
        assert (int(q["finish_x"]), int(q["finish_y"])) == points[4]
    subject = ROOT / "examples/bank-guide.mind"
    peer = ROOT / "examples/loam-a.mind"
    retained = folder / "coursed.mind"
    commands = [
        "/marsh 4242",
        *(["/marsh-taste"] * 11),
        "/marsh-survey",
        "/marsh-listen",
        "/marsh-tide 1",
        "/marsh-taste",
        "/marsh-raft",
        "/marsh-look",
        "/marsh-start",
        "where did ivory rest",
        "/marsh-face 1",
        "where is it now",
        "what note does it give",
        "/marsh-fetch",
        "/quit",
    ]
    _, bay, notes, _ = marsh.terrain(4242)
    old_gate = marsh.wheel.native(4242)[1][bay]
    piece = notes.index((old_gate + 1) % 4)
    commands[-1:] = [
        f"/marsh-grasp {piece}",
        "/marsh-step 0 2",
        "/marsh-put",
        "/marsh-step 2 2",
        f"/marsh-grasp {piece}",
        "/marsh-step 2 2",
        f"/marsh-grasp {piece}",
        "/marsh-put",
        "/quit",
    ]
    chat = call(
        "talk",
        "--speaker",
        subject,
        "--listener",
        peer,
        "--save-speaker",
        retained,
        stdin="\n".join(commands) + "\n",
    )
    replies = rows(chat, "hollow_reply")
    contacts = rows(chat, "hollow_contact")
    assert [v["said"] for v in replies[:2]] == ["left", "behind"]
    assert replies[2]["said"] == marsh.TONE[marsh.terrain(4242)[2][3]]
    assert (
        any(q["corrected"] == "1" for q in contacts) and sum(int(q["delivered"]) for q in contacts) == 1
    )
    assert sum(int(q["food"]) for q in contacts) == 512 - int(contacts[-1]["stock"])
    lifts = [q for q in contacts if q["hollow_contact"] == "12"]
    assert [int(q["held"]) for q in lifts] == [piece, 4, piece] and [int(q["work"]) for q in lifts] == [
        1,
        0,
        1,
    ]
    for q in contacts:
        held = int(q["held"])
        floor = int(q["floor_mask"])
        assert floor == (15 ^ (1 << held if held < 4 else 0))
    # Vocabulary and the acquired locations travel with an individual, bed and sample receipts do not.
    log = flood("--side", 64, "--soil", old, "--resident", retained, "--steps", 0, "--save", copy)
    slot = int(rows(log, "rehoused_resident")[0]["rehoused_resident"])
    image = snapshot(copy)
    assert (
        slot in image["place_words"] and slot in image["native_words"] and image["rangers"][slot][7] >= 4
    )
    again = folder / "witness.mind"
    flood("--load", copy, "--slot", slot, "--steps", 0, "--export-mind", again)
    # The full native/spatial suffix survives individual -> habitat -> individual.
    assert again.read_bytes()[-(1120 + 164 + 8) : -8] == retained.read_bytes()[-(1120 + 164 + 8) : -8]
    assert again.read_bytes()[-8:] == checksum(again.read_bytes()[:-8])
    # Swap only external grading words in identical problems. The heard replies must stay identical.
    files = [folder / f"questions{i}" for i in range(2)]
    header = "4242 1 1 3 3 4 2 | where did ivory rest | where is it now | what note does it give | visible axial "
    for file, ref in zip(files, ("left behind buzz", "bog fog rock")):
        file.write_text(header + ref + "\n")
    transcripts = [
        call("talk", "--speaker", again, "--listener", peer, stdin=f"/marsh 7\n/marsh-tests {f}\n/quit\n")
        for f in files
    ]
    assert rows(transcripts[0], "hollow_sentence") == rows(transcripts[1], "hollow_sentence")
    assert all(a["said"] != "_" for a in rows(transcripts[0], "hollow_sentence"))
    assert all(r["correct"] == "0" for r in rows(transcripts[1], "hollow_language"))
    print(
        "spatial learning, field census, birth, conservation, compact recall, independent grading and persistence passed"
    )


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(
        prefix="axion-hollows-test-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as folder:
        test(Path(folder))
