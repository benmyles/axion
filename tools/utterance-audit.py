#!/usr/bin/env python3
"""Independently reconstruct water, retinal evidence, speech targets and paid catches."""

import argparse
import csv
import gzip
import importlib.util
from collections import Counter
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    "retina", Path(__file__).with_name("retina-audit.py")
)
eye = importlib.util.module_from_spec(spec)
spec.loader.exec_module(eye)
mix, pixel, glass = eye.mix, eye.pixel, eye.glass
MASK = (1 << 56) - 1
WORDS = (
    "north east south west amber jade plum teal went from to as stayed at and then after in came "
    "began rested what happened where did they go how it begin finish ba di ku mo pa ri sa tu va wi xi yo za fe gu ha before"
).split()


def read(path):
    path = Path(path)
    with gzip.open(path, "rt") if path.suffix == ".gz" else path.open() as f:
        yield from csv.DictReader(f, delimiter="\t")


def events(tape):
    n = tape >> 56
    assert n <= 7 and tape & MASK < (1 << (8 * n))
    return list((tape & MASK).to_bytes(n, "big"))


def append(tape, value):
    return (min((tape >> 56) + 1, 7) << 56) | (((tape << 8) | value) & MASK)


def pigments(palette, a, b, wet, fibres):
    old = [pixel(palette, j) for j in range(4)]
    carrier = old[:]
    wick = (a + (3 if (a + 1) % 4 == b else 1)) % 4
    if a != b and wet & fibres & (1 << wick):
        carrier[wick], carrier[a] = carrier[a], carrier[wick]
    u, v = old[a], carrier[a]
    carrier[b], carrier[a] = carrier[a], carrier[b]
    assert sorted(carrier) == sorted(old)
    return sum(x << (2 * j) for j, x in enumerate(carrier)), u, v


def trace_vision(
    ground, a, b, u, v, places, hues, receptors, dark=False, displaced=None
):
    frames = (
        ground,
        eye.drift.set_pigment(ground, a, u),
        ground
        if dark
        else eye.drift.set_pigment(ground, b if displaced is None else displaced, v),
    )
    base, first, last = [glass(frame, places, hues) for frame in frames]
    start = [i for i in range(4) if (pixel(first, i) ^ pixel(base, i)) & receptors & 3]
    end = [
        i for i in range(4) if (pixel(last, i) ^ pixel(base, i)) & (receptors >> 2) & 3
    ]
    code = 256
    if len(start) == len(end) == 1:
        i, j = start[0], end[0]
        code = i + 4 * j + 16 * pixel(first, i) + 64 * pixel(last, j)
    return (base << 16) | (first << 8) | last, code


def song(genes, motor, image):
    if image == 256:
        return []
    a, b, u, v = (pixel(image, j) for j in range(4))
    channels = [a, b, u, v, (b - a) % 4, (v - u) % 4, a ^ u, b ^ v]
    calls = []
    for i in range(1 + ((motor >> 24) & 7)):
        dendrite = (genes >> (4 * i)) & 15
        tap = (motor >> (3 * i)) & 7
        pitch = channels[dendrite & 7] ^ (channels[tap] if dendrite & 8 else 0)
        pulse = 4 * (tap & 3) + ((pitch + ((motor >> (27 + i % 4)) & 1)) % 4)
        if i and motor >> 31 and pulse == ((genes >> 27) & 15):
            break
        calls.append(pulse)
    return calls


def acquired_notes(row, native):
    result = []
    controller = row["controller"]
    image = row["image"]
    for phase, original in enumerate(native):

        def affinity(note):
            bank = (phase + note // 4) % 8
            suffix = f"_{bank}"
            word = row[("low" if note % 4 < 2 else "high") + suffix]
            cells = (word >> (16 * (note % 2))) & 65535
            value = 0
            for feature in range(4):
                seen = (cells >> (4 * feature)) & 15
                bit = 1 << pixel(image, feature)
                if seen == bit:
                    value += 5
                elif seen & bit:
                    value += 1
                elif seen:
                    value -= 2
            return value

        best, selected = affinity(original), original
        if controller & (1 << (16 + phase)):
            count = 8 if controller & (1 << (24 + phase)) else 4
            for j in range(1, count):
                note = ((original & 12) ^ (4 if j >= 4 else 0)) + (original + j) % 4
                candidate = affinity(note)
                if candidate > best:
                    best, selected = candidate, note
        result.append(
            selected if best >= 1 + ((controller >> (2 * phase)) & 3) else original
        )
    return result


def field(path, follow=False):
    timeline = {}
    total = Counter()
    for row in read(path):
        r = {k: int(v) for k, v in row.items()}
        a, b = r["from"], r["to"]
        after, u, v = pigments(r["before"], a, b, r["wet"], r["fibres"])
        assert after == r["after"]
        args = (r["ground"], a, b, u, v, r["places"], r["hues"], r["receptors"])
        patterns = [
            trace_vision(*args, dark=r["channel"] == 3, displaced=j)
            for j in (range(4) if r["displaced"] else [None])
        ]
        assert (r["light"], r["image"]) in patterns
        prior, current = r["previous"], r["trail"]
        assert (
            len(events(prior)) == r["prior_size"] and len(events(current)) == r["size"]
        )
        image = r["image"]
        if image < 256 and r.get("anchored") == 2:
            image = (
                (image & 240)
                | ((pixel(image, 0) - r["heading"]) % 4)
                | (((pixel(image, 1) - r["heading"]) % 4) << 2)
            )
        assert current == (append(prior, image) if image < 256 else prior)
        if r["words"]:
            voice = song(r["dendrites"], r["motor"], r["image"])
            assert r["slot"] == r["actor"] and len(voice) == r["words"]
            if "natal" in r:
                assert r["natal"] == sum(x << (4 * j) for j, x in enumerate(voice))
            emitted = acquired_notes(r, voice) if r.get("echo") else voice
            assert r["sequence"] == sum(x << (4 * j) for j, x in enumerate(emitted))
            total["borrowed"] += int(emitted != voice and bool(r["paid"]))
            assert r["paid"] in (0, len(voice))
        if follow:
            key = r["slot"], r["born"]
            if key in timeline:
                stamp, history = timeline[key]
                assert stamp < r["tick"] and history == prior
            else:
                # A continuous recording may start in the middle of a resident's life.
                # Subsequent observations of that birth must still join exactly.
                assert r["born"] <= r["tick"]
            timeline[key] = r["tick"], current
        total.update(
            rows=1,
            complete=int(r["image"] < 256),
            changes=int(u != v),
            paid_calls=int(bool(r["paid"])),
        )
    assert total["rows"]
    return total, timeline


def relabel(token, language):
    if language != 2 or token > 8:
        return token
    first = 1 if token <= 4 else 5
    return first + eye.drift.coupling(mix(23771 + first * 1177) % 24)[token - first]


def clause(e, task, lang):
    a, b, u, v = [pixel(e, i) + (1 if i < 2 else 5) for i in range(4)]
    if task == 1:
        line = [b]
    elif task == 2:
        line = [u, 14, a]
    elif task == 3:
        line = [v, 14, b]
    elif lang == 1:
        line = [v, 14, b, 13 if a == b else 19, 10, a, 18, u]
    elif lang == 2:
        line = [a, u, 13 if a == b else 9, v, b]
    elif a == b:
        line = [u, 13, 14, a, 12, v]
    else:
        line = [u, 9, 10, a, 11, b, 12, v]
    return [relabel(t, lang) for t in line]


def tell(tape, task=0, lang=0):
    tape = (
        tape[:1]
        if task == 2
        else tape[-1:]
        if task == 3
        else tape[::-1]
        if not task and lang == 1
        else tape
    )
    words, previous = [], []
    for e in tape:
        piece = clause(e, task, lang)
        if words:
            words.extend(
                [17]
                if not task and lang == 1
                else [15, 16]
                if not task and not lang
                else [15]
            )
        i = 0
        if not task and lang == 2:
            while i < len(previous) and i + 1 < len(piece) and previous[i] == piece[i]:
                i += 1
        words.extend(piece[i:])
        previous = piece
    return words


def stage(seed, length, split, surfaces, bright, places, hues, receptors):
    material, local = [], []
    for j in range(length):
        for attempt in range(96):
            roll = mix(seed ^ mix(j * 65213 + attempt * 3671 + 9493))
            palette, fibres, ground = (
                mix(roll + 9311) & 255,
                mix(roll + 539) & 15,
                mix(roll + 915) & 255,
            )
            a = (roll >> 8) & 3
            if surfaces:
                lower, upper, _, _ = surfaces[mix(roll + 1987) % len(surfaces)]
                b = eye.drift.exit_from(a, upper if roll & 1 else lower)
            else:
                b = eye.drift.coupling(mix(roll + 7937) % 24)[a]
            _, u, v = pigments(palette, a, b, 15, fibres)
            if (a + u + split) % 4:
                continue
            if bright:
                ground = eye.drift.set_pigment(
                    eye.drift.set_pigment(ground, a, u ^ 3), b, v ^ 3
                )
            material.append(a + 4 * b + 16 * u + 64 * v)
            _, code = trace_vision(ground, a, b, u, v, places, hues, receptors)
            if code < 256:
                local.append(code)
            break
        else:
            raise ValueError("no held-out release")
    return material, local


def reports(path, surfaces=()):
    total = Counter()
    for row in read(path):
        r = {k: int(v) for k, v in row.items() if k not in ("said", "required")}
        said, wanted = (
            [int(x) for x in row[k].split(",") if x] for k in ("said", "required")
        )
        assert len(surfaces) == r["sites"]
        physical, local = stage(
            mix(r["seed"] ^ mix(r["layout"] * 1619 + 0xA9213)),
            r["length"],
            r["partition"],
            surfaces,
            not r["light"],
            r["places"],
            r["hues"],
            r["receptors"],
        )
        if r["twin"]:
            physical.reverse()
            local.reverse()
        assert (
            physical == events(r["physical"])
            and local == events(r["retinal"])
            and len(local) == r["visible"]
        )
        assert sum(pixel(x, 2) != pixel(x, 3) for x in physical) == r["stained"]
        if r["language"] < 3:
            assert tell(physical, r["task"], r["language"]) == wanted
        assert int(bool(wanted) and said == wanted) == r["correct"]
        decoded = events(r["decoded"])
        upper = lower = 0
        if not r["task"] and len(decoded) == len(physical):
            for e, catch in zip(physical, decoded):
                for k in (0, 1):
                    good = pixel(catch, k) == (
                        pixel(r["places"], pixel(e, k)) ^ r["hand"]
                    ) and pixel(catch, 2 + k) == pixel(r["hues"], pixel(e, 2 + k))
                    if k:
                        lower += good
                    else:
                        upper += good
        assert (
            upper,
            lower,
            0 if r["task"] else 2 * len(physical) - upper - lower,
        ) == (r["upstream"], r["downstream"], r["settled"])
        total.update(cases=1, exact=r["correct"], upstream=upper, downstream=lower)
    assert total["cases"]
    return total


def references(path, lang=0, switched=False):
    total = Counter()
    for row in read(path):
        r = {k: int(v) for k, v in row.items()}
        physical = events(r["physical"])
        family = r["family"]
        forward = not (family ^ switched)
        indices = range(len(physical)) if forward else reversed(range(len(physical)))
        source = next(
            (i for i in indices if pixel(physical[i], 2 + family) == r["noun"]), None
        )
        dest = None if source is None else source + (1 if forward else -1)
        target = dest if dest is not None and 0 <= dest < len(physical) else 7
        assert target == r["target"]
        answer = (
            clause(physical[target], 3 if family else 0, lang) if target < 7 else []
        )
        assert r["wanted"] == (len(answer) << 56) + sum(
            v << (6 * j) for j, v in enumerate(answer)
        )
        total.update(
            rows=1,
            answerable=int(target < 7),
            exact=int(target < 7 and r["said"] == r["wanted"]),
            located=int(
                target < 7 and r["said"] == r["wanted"] and r["chosen"] == target
            ),
            invented=int(target == 7 and bool(r["said"])),
        )
    assert total["rows"]
    return total


def field_conversation(path, transcript, slot, born):
    field(path, follow=True)
    observed = []
    for row in read(path):
        r = {k: int(v) for k, v in row.items()}
        if r["slot"] != slot or r["born"] != born or r["image"] == 256:
            continue
        _, u, v = pigments(r["before"], r["from"], r["to"], r["wet"], r["fibres"])
        observed.append((r["tick"], r["from"] + 4 * r["to"] + 16 * u + 64 * v))
    assert len(observed) >= 7
    recent = [e for _, e in observed[-7:]]
    expectations = [tell(recent), tell(recent, 2), tell(recent, 3)]
    for noun, first in ((2, True), (3, False), (0, True)):
        steps = range(7) if first else reversed(range(7))
        at = next(
            (j for j in steps if pixel(recent[j], 2 if first else 3) == noun), None
        )
        dest = None if at is None else at + (1 if first else -1)
        expectations.append(
            clause(recent[dest], 0 if first else 3, 0)
            if dest is not None and 0 <= dest < 7
            else []
        )
    with (
        gzip.open(transcript, "rt")
        if str(transcript).endswith(".gz")
        else open(transcript)
    ) as source:
        responses = [
            line.strip().split("> ", 1)[1]
            for line in source
            if line.startswith("speaker> ")
        ]
    expected = [" ".join(WORDS[k - 1] for k in words) or "?" for words in expectations]
    assert responses == expected
    return {
        "field_events": len(observed),
        "retained": 7,
        "turns": len(expected),
        "account_words": len(expectations[0]),
    }


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "kind", choices=("field", "resident", "speech", "questions", "conversation")
    )
    p.add_argument("trace", type=Path)
    p.add_argument("--surfaces", type=Path)
    p.add_argument("--switched", action="store_true")
    p.add_argument("--language", type=int, default=0)
    p.add_argument("--transcript", type=Path)
    p.add_argument("--slot", type=int)
    p.add_argument("--born", type=int)
    args = p.parse_args()
    if args.kind == "conversation":
        result = field_conversation(args.trace, args.transcript, args.slot, args.born)
    elif args.kind == "speech":
        result = reports(args.trace, eye.load_surfaces(args.surfaces))
    elif args.kind == "questions":
        result = references(args.trace, args.language, args.switched)
    else:
        result = field(args.trace, args.kind == "resident")
    print(result)
