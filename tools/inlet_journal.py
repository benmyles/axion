"""Field plates for learning a watercourse from its other inlets."""


def inlet_observation(entries, result):
    def last(key):
        records = entries(key)
        return records[-1] if records else {}

    def count(row, field):
        return int(row.get(field, 0))

    r = last("inlet_conversation")
    if r:
        trials, releases = count(r, "courses"), count(r, "releases")
        predicted, both = count(r, "foresaw"), count(r, "both_releases")
        sample = last("inlet_speech_scene")
        initial = (
            "its own field contacts and spikes"
            if count(r, "carried")
            else (
                "sampled initial tissue"
                if count(r, "background_samples")
                else "fresh motor contacts and spikes"
            )
        )
        return result(
            "Speaking before the water moves",
            f"Resident {int(r['inlet_conversation']):,}   /   {trials:,} new encounters",
            "Spoken forecasts correct before release",
            100 * predicted / max(1, trials),
            "%",
            [
                ["New encounters", trials],
                ["Correct forecasts", predicted],
                ["Both releases named", both],
            ],
            [
                f"Starting with {initial}, the speaker made {count(r, 'practice')} practice reaches before the unopened inlet.",
                f"It named the undisturbed outlet before delivery in {predicted:,} of {trials:,} cases.",
                f"After the couplings moved it named the changed outlet in {count(r, 'revised'):,} cases with an initially correct report.",
            ],
            [
                f"With the physical result still hidden, the outcome question produced {count(r, 'premature'):,} audible replies.",
                f"Clearing the topic produced {count(r, 'cleared_topic'):,} audible follow-ups across {releases:,} releases.",
            ],
            "One prediction, two releases",
            [
                ("Answered before delivery", count(r, "answered_before"), trials),
                ("Forecast correct", predicted, trials),
                ("Outcome reported", count(r, "actual"), releases),
                ("Both paired outcomes named", both, trials),
                ("Topic removed", count(r, "cleared_topic"), releases),
            ],
            ("inlet_voice", sample) if sample else None,
        )

    q = last("inlet_panel")
    if q:
        trials = count(q, "courses")
        right = count(q, "correct")
        local = count(q, "local")
        pairs = count(q, "different_targets")
        marked = count(q, "pairs_right")
        brood = last("inlet_nursery")
        examples = entries("inlet_example")
        title = "The unopened inlet"
        if count(q, "landscape") == 3:
            title = "Channels or a common drain?"
        if count(q, "prechange"):
            title = "When the watercourse changes"
        name = (
            "Food-selected descendants"
            if brood and count(q, "selected")
            else "Uniform-parent descendants"
            if brood
            else "Living field sample"
        )
        notes = [
            f"With the neighbouring contacts disconnected, the matched draws found {local:,} correct first reaches.",
            f"Rotating the same distal inputs produced {count(q, 'rotated'):,} correct choices.",
        ]
        if brood:
            notes[1] = (
                f"Consumed carbon funded {count(brood, 'births'):,} local births in {count(brood, 'inlet_nursery'):,} generations."
            )
        summary = [
            f"The selected circuit met {trials:,} fresh courses with the test inlet withheld during practice.",
            f"It reached the correct outlet on its first attempt in {right:,} cases.",
            f"When the paired layouts required different answers, it solved both in {marked:,} of {pairs:,} pairs.",
        ]
        return result(
            title,
            f"{name}   /   {count(q, 'programs'):,} distinct programs",
            "First reach through an unopened inlet",
            100 * right / max(1, trials),
            "%",
            [
                ["First reaches", trials],
                ["Correct first reaches", right],
                ["Both layouts solved", marked],
            ],
            summary,
            notes,
            "From three inlets to a fourth",
            [
                ("Inherited readout", right, trials),
                ("Neighbouring contacts cut", local, trials),
                ("Distal inputs rotated", count(q, "rotated"), trials),
                ("Both changed layouts", marked, pairs),
                (
                    "Independent test outlet",
                    count(q, "detached_right"),
                    count(q, "detached_offered"),
                ),
            ],
            ("inlet_map", examples[0]) if examples else None,
        )

    s = last("inlet_storage")
    if s and count(s, "wet_pools"):
        held, inspected = count(s, "held"), count(s, "wet_pools")
        caught = count(s, "swallowed")
        total = count(s, "released")
        return result(
            "Food behind a living sill",
            f"Watershed tick {int(s['inlet_storage']):,}",
            "Wet pools below their spill threshold",
            100 * held / inspected,
            "%",
            [
                ["Pools held", held],
                ["Overtopping loads", count(s, "spills")],
                ["Food caught in spills", caught],
            ],
            [
                f"The river encountered {inspected:,} wet, supplied high banks.",
                f"On {count(s, 'spills'):,} pulses, water overtopped a bank and released {total:,} units of food.",
                f"The animals swallowed {caught:,} units from those pulses and {count(s, 'shallow_meals'):,} from shallow trickles.",
            ],
            [
                f"Bound fibre held back {count(s, 'held_by_fibres'):,} observed pools that could pass at the same site with the fibre removed.",
                "The pressure threshold uses local water, organic stock, rock, clay and fibre.",
            ],
            "From storage to consumption",
            [
                ("Held below the rim", held, inspected),
                ("Held through bound fibre", count(s, "held_by_fibres"), inspected),
                ("Collected at overtopping", caught, total),
                (
                    "Collected from trickles",
                    count(s, "shallow_meals"),
                    count(s, "trickled"),
                ),
            ],
        )
    return None


def inlet_panel_illustration(d, kind, q, x, y, size, ink, muted, gold, green, grid):
    if kind == "inlet_voice":
        names = ("north", "east", "south", "west")

        def said(key):
            n = int(q[key])
            return (
                names[n]
                if n < 4
                else ("amber", "jade", "plum", "teal")[n - 4]
                if n < 8
                else "no reply"
            )

        d.label(x + 30, y + 149, "First recorded course", muted)
        for i, (stage, key) in enumerate(
            (
                ("Before release", "planned"),
                ("Undisturbed", "reported"),
                ("Couplings turned", "second_report"),
            )
        ):
            yy = y + 203 + 99 * i
            d.rule(x + 30, yy + 31, x + size - 31, yy + 31, grid, 1)
            d.text(x + 31, yy, stage, 19, muted)
            d.text(
                x + size - 36,
                yy,
                said(key),
                31,
                gold if i == 0 else ink,
                SERIF,
                anchor="end",
            )
        d.text(
            x + 30,
            y + 477,
            f"Actual outlets   {said('actual')}   /   {said('turned')}",
            18,
            green,
            MONO,
        )
        return
    unseen = int(q["unseen"])
    route = int(q["map"])
    visited = int(q["witnessed"])
    left, right = x + 105, x + size - 99
    d.label(x + 29, y + 143, "Inlet", muted)
    d.label(right - 7, y + 143, "Outlet", muted)
    for j in range(4):
        target = (route >> (2 * j)) & 3
        u = y + 187 + 62 * j
        v = y + 187 + 62 * target
        accent = gold if j == unseen else green if visited & (1 << j) else muted
        d.path(
            f"M{left} {u} C{left + 130} {u} {right - 130} {v} {right} {v}",
            accent,
            3 if j == unseen else 2,
            extra='stroke-dasharray="6 5"' if j == unseen else "",
        )
    for j in range(4):
        yy = y + 187 + j * 62
        d.text(left - 30, yy + 6, "ABCD"[j], 21, ink, MONO, anchor="end")
        d.dot(left, yy, 7, gold if j == unseen else green)
        d.dot(right, yy, 7, gold if j == int(q["target"]) else green)
        d.text(right + 24, yy + 6, str(j + 1), 21, ink, MONO)
    d.rule(x + 28, y + 416, x + size - 28, y + 416, grid)
    d.text(x + 29, y + 454, f"First reach at inlet {'ABCD'[unseen]}", 23, ink, SERIF)
    d.text(
        x + 29,
        y + 489,
        f"Reached {int(q['chosen']) + 1}   /   water emerged at {int(q['target']) + 1}",
        17,
        gold,
        MONO,
    )
