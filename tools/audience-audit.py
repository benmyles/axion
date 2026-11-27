#!/usr/bin/env python3
"""Independently replay visible glints, the guide's choice, acoustic intake and the sealed catches."""

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
d = eye.drift


def records(path):
    with (gzip.open if path.suffix == ".gz" else open)(path, "rt") as f:
        for r in csv.DictReader(f, delimiter="\t"):
            yield {k: int(v) for k, v in r.items()}


def check(r, **expected):
    for key, v in expected.items():
        if r[key] != v:
            raise ValueError(
                f"{key} at row {r.get('life', r.get('event'))}: recorded {r[key]}, reconstructed {v}"
            )


def inputs(a, origin, end, local=False):
    base = 255
    onset = d.set_pigment(base, origin, 0)
    offset = d.set_pigment(base, end, 0)
    if not local:
        base, onset, offset = [
            eye.glass(s, a["places"], a["hues"]) for s in (base, onset, offset)
        ]
    g = a["receptors"]
    return (
        eye.receptor(base, onset, g, g >> 4, a["heading"]),
        eye.receptor(base, offset, g >> 2, g >> 8, a["heading"]),
    )


def fire(a, state, source, ports, tie):
    pre, _ = inputs(a, source, source)
    options = 0
    for j in range(4):
        if pre & (1 << j):
            options |= d.outlet_options(state[0], j, a["wiring"], ports)
    aim = d.physical(a["muscles"], d.expected_command(options, tie))
    voice = d.pigment(a["places"], aim) if options.bit_count() == 1 else 4
    return pre, options, aim, voice


def interpret(code, first, second):
    pairs = set()
    if max(first, second) < 4:
        for order, table in enumerate(code):
            rows = [
                {k for k in range(4) if not table & (1 << (4 * j + k))}
                for j in range(8)
            ]
            if all(rows):
                for a in rows[first]:
                    for b in rows[4 + second]:
                        pairs.add((b, a) if order else (a, b))
    coordinates = [{p[j] for p in pairs} for j in range(2)]
    return tuple(s.pop() if len(s) == 1 else 4 for s in coordinates)


def lesson(path):
    prior = None
    counts = Counter()
    for r in records(path):
        before = [r["before_a"], r["before_b"]]
        if prior is not None and before != prior:
            raise ValueError("unrecorded acoustic synaptic update")
        after = before.copy()
        if (
            r["receptive"]
            and r["pairing"] != 2
            and max(r["first"], r["second"], r["source"]) < 4
        ):
            for order in range(2):
                marks = (r["opening"], r["ending"])[:: (-1 if order else 1)]
                for j, (word, visible) in enumerate(
                    zip((r["first"], r["second"]), marks)
                ):
                    if not visible:
                        continue
                    for col in range(4):
                        place = 16 * j + 4 * word + col
                        old = (before[order] >> place) & 1
                        blocked = int(not visible & (1 << col))
                        next_bit = (r["law"] >> (old + 2 * blocked)) & 1
                        after[order] = (after[order] & ~(1 << place)) | (
                            next_bit << place
                        )
        check(r, after_a=after[0], after_b=after[1])
        counts.update(
            offers=1, edits=sum((x ^ y).bit_count() for x, y in zip(before, after))
        )
        prior = after
    return counts


def audit(path, surfaces=None):
    total = Counter()
    previous = {}
    for r in records(path):
        key = d.unsigned_mix(r["seed"] ^ d.unsigned_mix(r["life"] * 49523 + 19471))
        motion = d.coupling(d.unsigned_mix(key + 1847) % 24)[:2]
        coats = d.coupling(d.unsigned_mix(key + 7681) % 24)[:2]
        site = d.unsigned_mix(key + 7559) % r["sites"] if r["sites"] else 0xFFFFFFFF
        if r["sites"]:
            if not surfaces or len(surfaces) != r["sites"]:
                raise ValueError(
                    "the physical trace needs its original watercourse collection"
                )
            links = surfaces[site][int(bool(key & 1024))]
            outlets = [d.exit_from(j, links) for j in range(4)]
        else:
            outlets = d.coupling(d.unsigned_mix(key + 14753) % 24)
        check(
            r,
            key=key,
            map=sum(v << (2 * j) for j, v in enumerate(outlets)),
            site=site,
            first=motion[0],
            second=motion[1],
            coat_a=coats[0],
            coat_b=coats[1],
            tie=d.unsigned_mix(key + 2053),
        )
        actors = [
            {k.removeprefix(prefix): v for k, v in r.items() if k.startswith(prefix)}
            for prefix in ("guide_", "pupil_")
        ]
        guide, pupil = actors
        state = [(a["initial_contacts"], a["initial_spikes"]) for a in actors]
        tape = r["tape_before"]
        witnessed = viewed = 0
        looks = [0, 0]
        arm = r["arm"]
        seat = r["twin"]
        mixed = (r["life"] + d.unsigned_mix(r["seed"] ^ 0x9A84297)) & 15
        access = (
            [1, 2],
            [3, 3],
            [0, 0],
            [mixed & 3, mixed >> 2],
            ([3, 0] if not r["life"] % 2 else [0, 3]),
        )[r.get("access", 0)]
        order = d.coupling(d.unsigned_mix(key + 30467) % 24)
        for t in range(4 * r["rounds"]):
            source = order[(t + t // 4) % 4]
            dest = outlets[source]
            sees = looks[0] + 1 < guide["energy"]
            if sees:
                pre, post = inputs(guide, source, dest)
                state[0] = eye.nerve(guide["plasticity"], pre, post, *state[0])
                looks[0] += 1
            if source in motion:
                station = 1 << motion.index(source)
                for member in range(2):
                    if not access[member] & station:
                        continue
                    face = d.pigment(guide["hues"], coats[0 if arm == 5 else member])
                    if sees and arm not in (2, 6):
                        idx = 4 * face
                        old = (tape >> idx) & 15
                        next_row = eye.public_cells(
                            r["write_gene"],
                            pre,
                            old,
                            eye.addresses(post, (r["write_gene"] >> 8) & 7),
                        )
                        tape = (tape & ~(15 << idx)) | (next_row << idx)
                        witnessed |= 1 << source
                if (
                    access[seat ^ (arm == 7)] & station
                    and looks[1] + 1 < pupil["energy"]
                ):
                    pre, post = inputs(pupil, source, dest)
                    state[1] = eye.nerve(pupil["plasticity"], pre, post, *state[1])
                    viewed |= 1 << source
                    looks[1] += 1
        face = d.pigment(guide["hues"], coats[0 if arm == 5 else seat ^ (arm == 1)])
        pre, opts, _, voices = zip(
            *(fire(guide, state[0], x, r["ports"], r["tie"]) for x in motion)
        )
        vol, hand, enabled = eye.public_hand(
            r["write_gene"], r["motor_gene"], tape, face, pre, opts, r["tie"]
        )
        gesture = r["tracer"]
        opened = (
            enabled
            and (guide["limbs"] if gesture else guide["mouth"])
            and looks[0] + (2 if gesture else 3) <= guide["energy"]
        )
        source = motion[hand]
        check(
            r,
            viewed=viewed,
            seen=witnessed,
            tape=tape,
            addressed=face,
            guide_looks=looks[0],
            pupil_looks=looks[1],
            guide_contacts=state[0][0],
            guide_spikes=state[0][1],
            before=state[1][0],
            starting_spikes=state[1][1],
            pre_a=pre[0],
            pre_b=pre[1],
            forecast_a=opts[0],
            forecast_b=opts[1],
            left=vol[0],
            right=vol[1],
            picked=source,
            opened=opened,
        )
        before = state[1]
        start = d.pigment(guide["places"], source) if opened and not gesture else 4
        end = voices[hand] if opened and not gesture else 4
        names = [guide["name" + str(i)] for i in range(4)] + [0]
        emit = [names[start], names[end]][:: (-1 if r["spoken_order"] else 1)]
        check(
            r,
            spoken_order=(r["write_gene"] >> 18) & 1,
            said_from=start,
            said_to=end,
            first_wave=emit[0],
            second_wave=emit[1],
            release_from=source if opened and gesture else 4,
            release_to=outlets[source] if opened and gesture else 4,
        )
        vocab = [pupil["name" + str(i)] for i in range(4)]
        live = arm not in (3, 6) and looks[1] + 1 < pupil["energy"]
        raw = [
            vocab.index(w) if live and pupil["ears"] and w and w in vocab else 4
            for w in emit
        ]
        f, s = raw[::-1] if arm == 4 else raw
        if r["calibrated"]:
            f, s = interpret((r["code_a"], r["code_b"]), f, s)
        elif r["received_order"]:
            f, s = s, f
        teaching = bool(gesture and opened and live or max(f, s) < 4)
        if teaching:
            ins = (
                inputs(pupil, source, outlets[source] ^ (arm == 4))
                if gesture
                else inputs(pupil, f, s, True)
            )
            state[1] = eye.nerve(pupil["plasticity"], *ins, *before)
        check(
            r,
            heard_from=f,
            heard_to=s,
            taught=teaching,
            after=state[1][0],
            pupil_spikes=state[1][1],
            work=int(opened) if gesture else sum(map(bool, emit)),
        )
        cents = []
        for status, tissue in (("prior", before), ("final", state[1])):
            aims = words = right = spoken = 0
            for source_id in range(4):
                _, _, aim, w = fire(
                    pupil,
                    tissue,
                    source_id,
                    r["ports"],
                    d.unsigned_mix(key + source_id * 8731 + 3719),
                )
                if w == 4 or not pupil["mouth"] or not vocab[w]:
                    w = 4
                aims |= aim << (2 * source_id)
                words |= w << (3 * source_id)
                if source_id in motion:
                    right += aim == outlets[source_id]
                    spoken += w == d.pigment(pupil["places"], outlets[source_id])
            check(r, **{status + "_aim": aims, status + "_voice": words})
            cents.extend([right, spoken])
        fresh = bool((opened and gesture or all(emit)) and not viewed & (1 << source))
        check(
            r,
            before_catches=cents[0],
            before_words=cents[1],
            after_catches=cents[2],
            after_words=cents[3],
            fresh=fresh,
            truthful=bool(
                all(emit) and end == d.pigment(guide["places"], outlets[source])
            ),
        )
        if seat:
            for k in ("key", "map", "tape", "guide_spikes", "tie", "guide_contacts"):
                if previous[k] != r[k]:
                    raise ValueError("the expert changed between recipient forks")
        previous = r
        total.update(
            encounters=1,
            caught_before=cents[0],
            caught_after=cents[2],
            words_after=cents[3],
            fresh=fresh,
            heat=r["work"],
        )
    return total


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("trace", type=Path)
    ap.add_argument("--surfaces", type=Path)
    ap.add_argument("--lessons", action="store_true")
    args = ap.parse_args()
    print(
        dict(
            lesson(args.trace)
            if args.lessons
            else audit(args.trace, eye.load_surfaces(args.surfaces))
        )
    )
