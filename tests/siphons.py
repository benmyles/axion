#!/usr/bin/env python3
"""Reconstruct native reciprocal decisions from bytes, verify ecological ownership and physical controls."""

import os
import struct
import subprocess
import tempfile
from pathlib import Path

from worksites import ROOT, checksum, mix, snapshot

EXE = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()
BANK = ROOT / "examples/reedbed.flood"
SPEAKER = ROOT / "examples/reed-interpreter.mind"
CALLER = ROOT / "examples/reed-caller.mind"


def run(*args, stdin="", ok=True):
    result = subprocess.run([EXE, *map(str, args)], input=stdin, text=True, capture_output=True)
    if ok and result.returncode:
        raise RuntimeError(result.stderr + result.stdout[-800:])
    if not ok:
        assert result.returncode
    return result.stdout + result.stderr


def rows(log, tag):
    return [
        dict(s.split("=", 1) for s in line.split() if "=" in s)
        for line in log.splitlines()
        if line.startswith(tag + "=")
    ]


def code(value, wiring):
    if wiring & 4:
        value = (value // 2) + 2 * (value % 2)
    return value ^ (wiring & 3)


def undo(value, wiring):
    return code(value ^ (wiring & 3), wiring & 4)


def elect(allowed, noise):
    return next((n for i in range(4) if (n := (noise + i * (1 + 2 * ((noise >> 2) & 1))) & 3) in allowed), 4)


def projections(a):
    maps = []
    for organ in range(5):
        reverse = organ in (1, 3)
        table = a[6 + organ // 2]
        readout = (a[2] >> (8 * (organ // 2))) & 255
        inhibition = (a[3] >> (2 * organ)) & 3

        def weight(source, motor):
            cell = 4 * motor + source if reverse else 4 * source + motor
            return (readout >> (2 * ((table >> (2 * cell)) & 3))) & 3

        matrix = []
        for j in range(4):
            values = [
                4 * weight(j, c) - inhibition * max(weight(other, c) for other in range(4) if other != j)
                for c in range(4)
            ]
            matrix.append(tuple(k for k in range(4) if values[k] == max(values)))
        maps.append(matrix)
    return maps


def outcome(a, b, am, bm, inlet, band, opening, key, mode):
    def vocal(ind, eligible, source, k, answer):
        tonic = (ind[3] >> (26 + answer)) & 1
        if tonic and ((k >> 12) & 7) >= ((ind[2] >> 27) & 3):
            fav = (mix((ind[4] >> 16) ^ (371 if answer else 953)) >> (2 * source)) & 3
            if fav in eligible:
                return fav
        return elect(eligible, k)

    def relayed(glyph, shut, rotate):
        return 4 if glyph == 4 or mode == shut else (glyph + int(mode == rotate)) % 4

    m = (key >> 11) & 7
    remote = (key >> 2) & 7
    at = code(band, b[4])
    ask = undo(vocal(b, bm[0][at], at, mix(key + 547), 0), b[4] >> 6) if (b[5] >> m) & 1 else 4
    heard = relayed(ask, 1, 3)
    k = mix(key + 733)
    oriented = undo(elect(am[1][code(heard, a[4] >> 6)] if heard < 4 else range(4), k), a[4])
    leaf = (inlet >> (2 * oriented)) & 3
    at = code(leaf, a[4] >> 3)
    sound = undo(vocal(a, am[2][at], at, mix(k + 199), 1), a[4] >> 6) if a[5] >> (8 + remote) & 1 else 4
    heard = relayed(sound, 2, 4)
    k = mix(key + 971)
    selected = undo(elect(bm[3][code(heard, b[4] >> 6)] if heard < 4 else range(4), k), b[4] >> 3)
    posture = (b[4] >> 12) & 3
    motion = (selected - opening, selected, selected ^ opening, selected + opening)[posture] & 3
    stroke = elect(bm[4][motion], mix(k + 739)) if b[5] >> (16 + m) & 1 else 4
    success = stroke < 4 and (opening + code(stroke, b[4] >> 9)) % 4 == (inlet >> (2 * band)) & 3
    return success, stroke < 4, ask < 4, sound < 4


def audit_pair(two, seed, rounds, mode):
    members = [list(x) for x in two]
    if mode == 5:
        for x in members:
            x[6:8] = [0, 0]
    if mode == 6:
        for x in members:
            x[8] = 0
    maps = [projections(a) for a in members]
    sums = [0] * 5
    for i in range(rounds):
        key = mix(mix(seed + i * 3251))
        flip = (key >> 8) & 1
        opening = (key >> 7) & 3
        light = mix(key + 441) & 255
        for band in range(4):
            wins = 0
            for target in range(4):
                changed = (light & ~(3 << (2 * band))) | (target << (2 * band))
                trial = outcome(
                    members[flip], members[1 - flip], maps[flip], maps[1 - flip], changed, band, opening, key, mode
                )
                for j, v in enumerate(trial):
                    sums[j] += v
                wins += trial[0]
            sums[4] += wins == 4
    return sums


def main(tmp):
    os.environ["AXION_JOURNAL_DIR"] = str(tmp / "journal")
    farm = lambda *args, **kw: run("floodplain", *args, **kw)
    book = snapshot(BANK)
    assert len(book["callers"]) == len(book["ports"]) == len(book["echo"]) == 4096
    assert book["reed"][4] > 0 and book["audience"][2] > 0
    options = ["--load", BANK, "--steps", 0]
    pair = [book["callers"][452], book["callers"][2518]]
    for j, file in enumerate((CALLER, SPEAKER)):
        saved = struct.unpack_from("<14I", file.read_bytes(), -72)
        assert tuple(saved[:9]) == tuple(pair[j][:9])
        assert saved[9] >= pair[j][9]
    seed = 198733
    replay = rows(
        run(
            "talk", "--speaker", SPEAKER, "--listener", CALLER, stdin=f"/reed 88\n/private-facts 57 {seed}\n/quit\n"
        ),
        "reed_pair",
    )
    assert len(replay) == 7
    for mode, record in enumerate(replay):
        expected = audit_pair(pair, seed, 57, mode)
        assert expected == [
            int(record[key]) for key in ("productive", "strokes", "questions", "replies", "four_correct")
        ], (mode, expected, record)
    native = tmp / "native.flood"
    full = tmp / "full.flood"
    split = tmp / "split.flood"
    tests = tmp / "questions.txt"
    words = ("amber", "jade", "opal", "ochre")
    questions = [
        (light, 0, "from ridge came ?", f"from/ridge/came/{v}") for light, v in zip((198, 216, 225, 54), words)
    ]
    questions += [
        (225, 0, "opal beyond ?", "opal/beyond/ridge"),
        (225, 2, "from ridge came ?", "_"),
        (170, 0, "opal beyond ?", "_"),
        (85, 0, "opal beyond ?", "_"),
        (225, 0, "came ridge from ?", "_"),
    ]
    tests.write_text("".join(f"600512459 {light} 2 2 {wire} {prompt}\n" for light, wire, prompt, _ in questions))
    assay = run("talk", "--speaker", SPEAKER, "--listener", CALLER, stdin=f"/reed 1\n/reed-tests {tests}\n/quit\n")
    spoken = rows(assay, "reed_test")
    assert [r["answer"] for r in spoken] == [q[-1] for q in questions]
    assert spoken[5]["replies"] == "1" and spoken[5]["heard_replies"] == "0"
    assert [spoken[k]["candidates"] for k in (4, 5, 6, 7)] == ["1", "0", "4", "0"]
    baseline_soil = tmp / "before.flood"
    farm("--load", ROOT / "examples/springtide.flood", "--siphons", 4, "--steps", 0, "--save", baseline_soil)
    all_tests = farm(
        *options,
        "--siphon-legacy",
        48,
        "--siphon-reference",
        baseline_soil,
        "--siphon-bench",
        262144,
        "--siphon-foster",
        2,
        "--siphon-trials",
        2,
        "--work-subjects",
        32,
        "--siphon-census",
        2,
        "--save",
        native,
    )
    assert native.read_bytes() == BANK.read_bytes()
    loss = {r["siphon_legacy"]: r for r in rows(all_tests, "siphon_legacy")}
    assert int(loss["inhabited"]["passive_carbon"]) > 0 and int(loss["sealed"]["passive_carbon"]) == 0
    assert int(loss["reference"]["changed_food_sites"]) > 0 and int(loss["rotated"]["altered_faces"]) == 4 * 4096
    stats = rows(all_tests, "siphon_deployment")[0]
    assert int(stats["settled_bytes"]) == 18 and int(stats["differences"]) == 0
    farm("--load", BANK, "--steps", 37, "--save", full)
    farm("--load", BANK, "--steps", 11, "--save", native)
    farm("--load", native, "--steps", 26, "--save", split)
    assert full.read_bytes() == split.read_bytes()
    farm("--load", BANK, "--steps", 0, "--extend", 128, "--save", full)
    expanded = snapshot(full)
    assert expanded["callers"][:4096] == book["callers"]
    for y in range(64):
        assert expanded["ports"][128 * y : 128 * y + 64] == book["ports"][64 * y : 64 * (y + 1)]
        assert expanded["echo"][128 * y : 128 * y + 64] == book["echo"][64 * y : 64 * (y + 1)]
    corrupted = bytearray(book["raw"][:-8])
    struct.pack_into("<I", corrupted, book["echo_pos"] + 4, book["tick"] + 1000)
    native.write_bytes(corrupted + checksum(corrupted))
    assert "acoustic exposure" in farm("--load", native, "--steps", 0, ok=False)
    for damped in (False, True):
        treatment = ["--wash-sounds"] if damped else []
        log = farm(
            "--soil",
            BANK,
            "--side",
            64,
            "--siphon-germs",
            BANK,
            "--siphon-workers",
            "still",
            "--siphon-audience",
            "ancestral",
            "--density",
            72,
            "--steps",
            12,
            *treatment,
            "--save",
            native,
        )
        new = snapshot(native)
        ear = rows(log, "siphon_audience")[-1]
        assert int(ear["heard"]) == (new["audience"][2])
        assert bool(new["audience"][2]) != damped and bool(new["audience"][4]) != damped
        assert not new["reed"][4] and new["audience"][2] == new["audience"][5]
    farm("--resident", SPEAKER, "--side", 16, "--steps", 1, "--siphon-workers", "still", "--save", native)
    named = snapshot(native)
    assert list(named["native_words"]) == [0] and named["voices"] == [0]
    new_mind = tmp / "resident.mind"
    farm("--load", native, "--slot", 0, "--export-mind", new_mind, "--steps", 0)
    baseline = run(
        "talk", "--speaker", SPEAKER, "--listener", CALLER, stdin="/reed 88 228 2 0\nfrom ridge came ?\n/quit\n"
    )
    fresh = run(
        "talk", "--speaker", new_mind, "--listener", CALLER, stdin="/reed 88 228 2 0\nfrom ridge came ?\n/quit\n"
    )
    assert rows(baseline, "reed_completion") == rows(fresh, "reed_completion")
    farm("--load", native, "--steps", 4096, "--save", split)
    later = snapshot(split)
    if later["ages"][0] != named["ages"][0] + 4096 or not later["energy"][0]:
        assert not later["native_words"]
    assert "least one quartet" in run(
        "talk", "--speaker", SPEAKER, "--listener", CALLER, stdin="/reed 1\n/private-facts 0 1\n/quit\n", ok=False
    )


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="axion-reed-audit-") as d:
        main(Path(d))
    print(
        "Native ecology, independent two-way decisions, compiled inference, physical legacy and caption ownership verified."
    )
