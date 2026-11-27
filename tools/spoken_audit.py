"""Decode the sound on each wire and compare it with independent physical and motor records."""

from collections import Counter
from functools import lru_cache
from capillary_audit import read

TONES = ("tap", "hum", "tick", "ring")
MOVES = ("west", "east", "north", "south")
CHANNELS = (
    "clear",
    "silent",
    "partial",
    "reordered",
    "duplicate",
    "unknown",
    "blind",
    "patchy",
)


@lru_cache(maxsize=8)
def waveform(word):
    raw = sum((ord(c) - 96) << (j * 5) for j, c in enumerate(word))
    mask = (1 << 64) - 1
    raw = ((raw ^ (raw >> 30)) * 0xBF58476D1CE4E5B9) & mask
    raw = ((raw ^ (raw >> 27)) * 0x94D049BB133111EB) & mask
    return raw ^ (raw >> 31)


def tones(dialect):
    return {waveform(TONES[i ^ dialect]): (i // 2, i % 2) for i in range(4)}


def compatible(state, event):
    axis, value = event
    return (state >> axis) & 1 == value


def audit(
    visits,
    channel="clear",
    budget=0,
    replies="actual",
    writer_dialect=0,
    reader_dialect=0,
):
    writer, reader = tones(writer_dialect), tones(reader_dialect)
    directions = {waveform(word): divmod(i, 2) for i, word in enumerate(MOVES)}
    totals = Counter(
        dict.fromkeys(
            (
                "called",
                "interpreted",
                "both_senses",
                "found",
                "reported_choice",
                "named_food",
                "misdescribed_choice",
                "requests",
                "answers",
                "food_after_question",
                "food_without_question",
            ),
            0,
        )
    )
    routes_seen = {}
    for index, r in enumerate(visits):
        scene, cue = divmod(index, 4)
        assert (scene, cue) == (r["scene"], r["cue"])
        sight = r["visible"]
        assert sight == r["mark"] ^ 3
        received = [v for v in (r["call_a"], r["call_b"]) if v]
        asked = [(r["ask_a"], r["reply_a"]), (r["ask_b"], r["reply_b"])]
        spoken = r["formed"]
        assert spoken in (0, 2)
        assert all(compatible(sight, writer[w]) for w in received)
        if channel in ("blind", "unknown"):
            assert not spoken
        if channel in ("clear", "reordered"):
            assert len(set(received)) == spoken
        elif channel == "partial":
            assert len(received) == int(bool(spoken))
        elif channel == "duplicate" and spoken:
            assert len(received) == 2 and received[0] == received[1]
        elif channel in ("silent", "blind", "unknown"):
            assert not received
        candidates = {
            i for i in range(4) if all(compatible(i, reader[w]) for w in received)
        }

        def mask(values):
            return sum(1 << v for v in values)

        assert mask(candidates) == r["initial"]
        listening = bool(spoken and channel != "silent")
        routes = [(r["routes"] >> (2 * i)) & 3 for i in range(4)]
        if listening:
            assert routes_seen.setdefault(scene, routes) == routes
            assert mask({routes[i] for i in candidates}) == r["alternatives"]
        tried = set()
        for j, (question, answer) in enumerate(asked):
            if j >= r["requests"]:
                assert not question and not answer
                continue
            axis, _ = reader[question]
            assert axis not in tried and listening
            assert any(
                ((a ^ b) >> axis) & 1 and routes[a] != routes[b]
                for a in candidates
                for b in candidates
            )
            tried.add(axis)
            if answer:
                assert replies != "withheld"
                source_axis = writer[question][0]
                experiment_sight = sight ^ (3 if replies == "exchanged" else 0)
                assert writer[answer] == (
                    source_axis,
                    (experiment_sight >> source_axis) & 1,
                )
                candidates = {i for i in candidates if compatible(i, reader[answer])}
            else:
                assert replies == "withheld"
        assert mask(candidates) == r["remaining"]
        assert r["requests"] <= budget and r["answers"] == sum(
            bool(v) for _, v in asked
        )
        assert r["heard"] == (
            next(iter(candidates)) if listening and len(candidates) == 1 else 4
        )
        possible_moves = {routes[i] for i in candidates} if listening else set()
        assert r["choice"] == (
            next(iter(possible_moves)) if len(possible_moves) == 1 else 4
        )
        actual = [v for v in (r["answer_a"], r["answer_b"]) if v]
        if actual:
            assert len(actual) == 2 and {directions[v][0] for v in actual} == {0, 1}
            heading = sum(directions[v][1] << directions[v][0] for v in actual)
            assert r["choice"] == heading == r["statement"]
        else:
            assert r["statement"] == 4
        food = r["choice"] == cue
        assert r["food"] == food
        totals.update(
            cases=1,
            called=bool(received),
            interpreted=r["choice"] < 4,
            both_senses=r["heard"] == sight,
            found=food,
            reported_choice=r["statement"] < 4 and r["statement"] == r["choice"],
            named_food=r["statement"] == cue,
            misdescribed_choice=r["statement"] < 4 and r["statement"] != r["choice"],
            requests=r["requests"],
            answers=r["answers"],
            food_after_question=food and bool(tried),
            food_without_question=food and not tried,
        )
    return dict(totals)


if __name__ == "__main__":
    import argparse

    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("file")
    p.add_argument("--channel", choices=CHANNELS, default="clear")
    p.add_argument("--questions", type=int, default=0)
    p.add_argument(
        "--replies", choices=("actual", "withheld", "exchanged"), default="actual"
    )
    p.add_argument("--writer-dialect", type=int, choices=(0, 1), default=0)
    p.add_argument("--reader-dialect", type=int, choices=(0, 1), default=0)
    a = p.parse_args()
    print(
        audit(
            read(a.file),
            a.channel,
            a.questions,
            a.replies,
            a.writer_dialect,
            a.reader_dialect,
        )
    )
