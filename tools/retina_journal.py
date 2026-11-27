"""Measured optical learning, material sequences and audible predictions."""


def retinal_observation(entries, result):
    def last(key):
        rows = entries(key)
        return rows[-1] if rows else {}

    def n(r, key):
        return int(r.get(key, 0))

    reports = description_observation(last, result, entries)
    if reports:
        return reports
    field = last("drift_tick")
    light = last("daylight")
    if last("audience_exchange") or last("public_offer") or last("public_practice") or n(last("field_audience"), "opportunities"):
        social = inquiry_observation(last, result)
        if social:
            return social
    mode = n(field, "mode")
    if n(light, "eyes") and mode & (1 << 7) and (mode & (1 << 28) or mode & (1 << 9)):
        traces = {}
        for rec in entries("drift_lineage"):
            label = n(rec, "drift_lineage")
            if label in (1, 2):
                traces.setdefault(n(rec, "tick"), {})[label] = n(rec, "living")
        census = [
            (tick, q[1], q[2])
            for tick, q in sorted(traces.items())
            if 1 in q and 2 in q
        ]
        if len(census) > 1:
            start, first, second = census[0]
            end, a, b = census[-1]
            sham = bool(mode & (1 << 9))
            assigned = 2 if mode & (1 << 8) else 1
            control = (
                "an independent source"
                if mode & (1 << 31)
                else "a different inlet on the same bed"
            )
            exposure = (
                "Both marks watched the real transfers."
                if sham
                else f"Mark {assigned} watched real transfers and its rival saw the endpoint from {control}."
            )
            grains = last("field_soundings")
            if grains:
                sham = not n(grains, "split")
                assigned = 2 if n(grains, "crossed") else 1
                exposure = (
                    "Both lineages retained their inherited tracer-hand connections."
                    if sham
                    else f"Mark {assigned} retained its own tracer hand and the other mark released from the opposite inlet."
                )
            return result(
                "Two lineages in one watershed",
                f"Founder {n(entries('drift_lineage')[-1], 'label_seed'):,}   /   ticks {start:,}–{end:,}",
                "Mark 1 share at the final census",
                100 * a / max(1, a + b),
                "%",
                [
                    ["Living mark 1", a],
                    ["Living mark 2", b],
                    ["Paid observations", n(light, "attended")],
                ],
                [
                    exposure,
                    f"The groups began with {first:,} and {second:,} living residents.",
                    f"After {end - start:,} ticks, {a:,} residents carried mark 1 and {b:,} carried mark 2.",
                ],
                [
                    f"Their eyes admitted {n(light, 'accepted'):,} opportunities for a local tissue update.",
                    f"{n(light, 'same_endpoint'):,} of {n(light, 'unpaired'):,} displaced observations happened to reach the real outlet."
                    if n(light, "unpaired")
                    else f"The population released {n(field, 'released'):,} units of suspended food.",
                ],
                "The two living populations",
                [],
                (
                    "visual_lineages",
                    dict(
                        census=census, sham=sham, assigned=assigned, grains=bool(grains)
                    ),
                ),
            )
    attention = inquiry_observation(last, result)
    if attention:
        return attention
    spoken = last("cascade_interview")
    if spoken:
        r = spoken
        total = n(r, "courses")
        right = n(r, "correct")
        both = n(r, "both_orders")
        swapped = n(r, "opposite_orders")
        sample = last("cascade_scene")
        scope = (
            f"{n(r, 'actual_beds'):,} captured watercourses"
            if n(r, "actual_beds")
            else f"{n(r, 'beds')} independent watercourses"
        )
        if n(r, "channel") != 1:
            scope += "   /   " + {
                0: "closed eyes",
                2: "displaced endpoint",
                3: "shuttered outcome",
                4: "independent endpoint",
            }.get(n(r, "channel"), "")
        if n(r, "hearing"):
            scope += "   /   " + (
                "muted words" if n(r, "hearing") == 2 else "reversed sounds"
            )
        length = (
            "the cord's visible winding order"
            if n(r, "woven_order")
            else f"{n(r, 'laps')} successive lamp names"
        )
        if n(r, "named_sequences"):
            combined = n(r, "combined")
            composed_right = n(r, "combined_right")
            return result(
                "A word for several movements",
                scope,
                "Correct combined temporal names",
                100 * composed_right / max(1, combined),
                "%",
                [
                    ["New names in request", n(r, "shortened")],
                    ["Several named segments", combined],
                    ["Correct compositions", composed_right],
                ],
                [
                    f"The examiner used {n(r, 'sound_units'):,} public sounds to specify {n(r, 'physical_steps'):,} physical stages.",
                    f"At least two named segments occurred in {combined:,} requests, with {composed_right:,} correct spoken forecasts.",
                    f"Across all {total:,} requests, the pupil voiced {n(r, 'voiced'):,} forecasts and {right:,} were correct.",
                ],
                [
                    f"The external speaker could use {n(r, 'named_sequences')} names for learned temporal patterns.",
                    f"Removing the event before the question yielded {n(r, 'no_scene'):,} audible predictions.",
                ],
                "From speech to a physical outcome",
                [
                    ("Requests with new names", n(r, "shortened"), total),
                    ("Correct combined names", composed_right, combined),
                    ("All correct forecasts", right, total),
                    ("Both releases named", n(r, "both_releases"), total),
                ],
                ("cascade_pair", sample) if sample else None,
            )
        return result(
            "Does the order of the words matter?",
            f"Resident {n(r, 'cascade_interview'):,}   /   {scope}",
            "Correct spoken route forecasts",
            100 * right / max(1, total),
            "%",
            [
                ["Spoken forecasts", n(r, "voiced")],
                ["Correct forecasts", right],
                ["Both changed orders", both],
            ],
            [
                f"The pupil saw {n(r, 'glimpses')} individual transfers before hearing {length}.",
                f"It named the future outlet correctly in {right:,} of {total:,} examinations before release.",
                f"Reversing the word order changed the outlet in {swapped:,} pairs and the pupil correctly named both in {both:,}.",
            ],
            [
                f"An independently diverted release was reported in {n(r, 'revised'):,} cases following a correct forecast.",
                f"The outcome question elicited {n(r, 'premature'):,} premature answers and {n(r, 'erased_topic'):,} answers after the topic was removed.",
            ],
            "Two orders, one earlier experience",
            [
                ("Forecasts voiced", n(r, "voiced"), total),
                ("Correct forecasts", right, total),
                ("Both orders correct", both, swapped),
                ("Both releases named", n(r, "both_releases"), total),
            ],
            ("cascade_pair", sample) if sample else None,
        )
    r = last("retinal_panel")
    if r:
        total = n(r, "courses")
        right = n(r, "first")
        blank = n(r, "untouched")
        both = n(r, "pair_right")
        distinct = n(r, "different_worlds")
        stock = last("retinal_brood")
        sites = n(r, "actual_beds")
        title = (
            "Learning before the first bite"
            if n(r, "beds") <= 1
            else "Following an order through different beds"
        )
        if sites:
            title = "Learning a watercourse built in the estuary"
        if stock:
            who = (
                "Food-selected brood"
                if n(stock, "selected")
                else "Uniform-parent brood"
            )
        else:
            who = "Watershed resident sample"
        first = f"The optical circuits encountered {n(r, 'glimpses')} unrewarded glimpses before their first reach."
        return result(
            title,
            f"{who}   /   {n(r, 'bodies'):,} bodies",
            "First reaches after optical exposure",
            100 * right / max(1, total),
            "%",
            [
                ["Paired first reaches", total],
                ["Correct outcomes", right],
                ["Earlier contacts alone", blank],
            ],
            [
                first,
                f"The selected circuit intercepted the load in {right:,} of {total:,} new arrangements.",
                f"For {distinct:,} pairs that required different answers it solved both in {both:,}.",
            ],
            [
                f"The complete population made {n(r, 'population_correct'):,} correct first reaches across {n(r, 'bodies') * total:,} encounters.",
                f"Previously consumed carbon funded {n(stock, 'births'):,} births."
                if stock
                else f"An outcome derived from a single pass was correct in {n(r, 'one_step'):,} cases.",
            ],
            "What changed after watching",
            [
                ("After observation", right, total),
                ("Untouched contacts", blank, total),
                ("One internal step", n(r, "one_step"), total),
                ("Both paired cases", both, distinct),
                (
                    "Different from first stage",
                    n(r, "horizon_right"),
                    n(r, "different_horizon"),
                ),
            ],
        )
    r = last("woven_courses")
    if r and n(r, "flows"):
        total = n(r, "flows")
        many = n(r, "serial")
        ordered = n(r, "order_matters")
        return result(
            "Water through wound fibre",
            f"Watershed tick {n(r, 'woven_courses'):,}   /   moving material",
            "Loads crossing multiple stages",
            100 * many / max(1, total),
            "%",
            [
                ["Multi-stage releases", many],
                ["Order changed outlet", ordered],
                ["Food intercepted", n(r, "eaten")],
            ],
            [
                f"Water traversed {n(r, 'stages'):,} stages in {total:,} food releases.",
                f"Reversing the order of the extant passages changed the outlet on {ordered:,} releases.",
                f"Animal work changed the cord order or length on {n(r, 'changed_cord'):,} occasions.",
            ],
            [
                f"Bound stages carried {n(r, 'released'):,} units of food in total.",
                f"The same committed reaches would have caught {n(r, 'reversed_intake'):,} units had the order been reversed.",
            ],
            "Along the material route",
            [
                ("Several stages", many, total),
                ("Order affected outlet", ordered, many),
                ("Past the first outlet", n(r, "beyond_first"), many),
                ("Correct reaches", n(r, "right"), n(r, "reaches")),
                ("One internal step", n(r, "single"), n(r, "reaches")),
            ],
            ("woven_sample", last("woven_witness")) if last("woven_witness") else None,
        )
    r = last("daylight")
    if r and n(r, "eyes"):
        total = n(r, "eyes")
        paid = n(r, "attended")
        paired = n(r, "paired")
        bottom = last("bottom_access")
        return result(
            "Watching an unfed current",
            f"Watershed tick {n(r, 'daylight'):,}",
            "Paid attention to water glints",
            100 * paid / max(1, total),
            "%",
            [
                ["Sighted opportunities", total],
                ["Paid observations", paid],
                ["Two visible changes", paired],
            ],
            [
                f"Light crossed {n(r, 'wet'):,} wet beds without releasing food.",
                f"Residents paid for {paid:,} observations and detected both ends of {paired:,} tracer movements.",
                f"These pulses changed {n(r, 'synaptic_edits'):,} synaptic bits in living tissue.",
            ],
            [
                f"The optical events consumed {n(r, 'heat'):,} units of real metabolic energy.",
                f"Water depth put the bottom beyond reach on {n(bottom, 'too_deep'):,} grazing attempts."
                if bottom
                else "The active eyes receive the local pigments before and after each glint.",
            ],
            "Which signals reached living tissue",
            [
                ("Attended", paid, total),
                ("Onset visible", n(r, "onset"), paid),
                ("Outcome visible", n(r, "offset"), paid),
                ("Both views", paired, paid),
                ("Plasticity admitted", n(r, "accepted"), paid),
            ],
        )
    return None


def cascade_illustration(d, q, x, y, size, ink, muted, gold, green, grid):
    colors = ("#efb477", "#a9cba1", "#bc9ed2", "#7bc2c4")
    names = q.get("names", "north,east,south,west,amber,jade,plum,teal").split(",")

    def word(v):
        i = int(q[v])
        return names[i] if i < len(names) else "no reply"

    d.label(x + 28, y + 143, f"Recorded pair {int(q['cascade_scene']) + 1}", muted)
    n = int(q["laps"])
    for ix, (cues, heard, forecast, actual) in enumerate(
        (
            ("first_cues", "heard", "prediction", "outlet"),
            ("other_cues", "other_heard", "alternative", "other_outlet"),
        )
    ):
        yy = y + 174 + 161 * ix
        d.text(
            x + 30,
            yy + 20,
            "First order" if ix == 0 else "Reversed order",
            23,
            ink,
            SERIF,
        )
        seq = int(q[cues])
        count = int(q[heard])
        if count:
            width = min(34, (size - 68) / max(1, n))
            for j in range(n):
                d.rect(
                    x + 30 + j * width,
                    yy + 38,
                    max(4, width - 3),
                    18,
                    colors[(seq >> (2 * j)) & 3],
                    3,
                )
        else:
            d.text(x + 30, yy + 55, "No grounded sequence heard", 18, muted, MONO)
        d.text(x + 30, yy + 96, f"Forecast   {word(forecast)}", 22, gold)
        d.text(x + 30, yy + 128, f"Released   {word(actual)}", 20, green)
        d.rule(x + 28, yy + 145, x + size - 28, yy + 145, grid)
    d.text(
        x + 30,
        y + 518,
        f"{n} stages   /   marker started at {word('origin')}",
        17,
        muted,
        MONO,
    )


def woven_illustration(d, q, x, y, size, ink, muted, gold, green, grid):
    d.label(
        x + 28,
        y + 143,
        f"Tile {int(q['tile']):,}   /   tick {int(q['woven_witness']):,}",
        muted,
    )
    layers = [(int(q["turns"]) >> j) & 1 for j in range(int(q["laps"]))]
    positions = ("NW", "NE", "SE", "SW")
    for r, (title, journey, end) in enumerate(
        (
            ("Observed flow", layers, "outlet"),
            ("Reversed flow", layers[::-1], "reverse"),
        )
    ):
        top = y + 187 + r * 146
        d.text(x + 28, top, title, 24, ink, SERIF)
        width = min(77, (size - 54) / len(journey))
        for j, upper in enumerate(journey):
            d.rect(
                x + 28 + width * j, top + 19, width - 4, 25, green if upper else gold, 4
            )
            d.text(
                x + 30 + width * j,
                top + 66,
                "fibre" if upper else "bed",
                16,
                muted,
                MONO,
            )
        d.text(
            x + 28,
            top + 101,
            f"{positions[int(q['inlet'])]} inlet   →   {positions[int(q[end])]} outlet",
            21,
            green if r == 0 else muted,
        )
        d.rule(x + 28, top + 116, x + size - 28, top + 116, grid)
    fibre = int(q["fibre"])
    clay = int(q["clay"])
    d.text(
        x + 28,
        y + 496,
        f"{sum((fibre >> (16 * j)) & 65535 for j in range(4))} fibre   /   {sum((clay >> (16 * j)) & 65535 for j in range(4))} clay on four edges",
        17,
        muted,
        MONO,
    )
    d.text(
        x + 28,
        y + 527,
        f"{counted(int(q['food']), 'unit')} released   /   {int(q['caught'])} caught",
        17,
        muted,
        MONO,
    )


def cascade_session(data):
    records = [
        pairs(line) for line in data.get("lines", []) if line.startswith("cascade=")
    ]
    active = [q for q in records if q["cascade"] == "heard" and "query_offers" in q]
    delivered = [q for q in records if q["cascade"] == "pour" and "delivered" in q]
    if active and delivered:
        heat = sum(int(q["query_paid"]) for q in active)
        offers = sum(int(q["query_offers"]) for q in active)
        say = [l[9:] for l in data["lines"] if l.startswith("speaker> ")]
        end = delivered[-1]
        return (
            "An investigation prompted by words",
            "A living resident   /   independently opened watercourse",
            "Test grains released",
            heat,
            "grains",
            [
                ["Grounded orders", len(active)],
                ["Inlet pairs examined", offers],
                ["Test grains released", heat],
            ],
            [
                f"The animal heard {len(active)} grounded route requests while the main load was still held.",
                f"It released {heat} test grains before the experimenter opened the physical watercourse.",
                f"The main load reached {end.get('delivered_name', 'sensory port ' + end['delivered'])}.",
            ],
            [
                "The recorded replies were " + ", ".join(say) + "."
                if say
                else "The animal stayed silent.",
                f"The closing release contained {int(end['eaten']) + int(end['settled'])} food units.",
            ],
        )
    lessons = [q for q in records if q["cascade"] == "name"]
    if lessons:
        reps = sum(int(q["demonstrations"]) for q in lessons)
        resolved = [q["word"] for q in lessons if int(q["resolved_stages"])]
        edits = sum(int(q["synaptic_edits"]) for q in lessons)
        return (
            "Naming a sequence by watching",
            "A living pupil   /   paid visual demonstrations",
            "Resolved temporal names",
            len(resolved),
            "names",
            [
                ["Names presented", len(lessons)],
                ["Paid demonstrations", reps],
                ["Resolved names", len(resolved)],
            ],
            [
                f"The tutor presented {len(lessons)} sounds while the animal watched moving light.",
                f"The pupil paid for {reps} demonstrations and changed {edits} word-contact bits.",
                f"Its tissue resolved {len(resolved)} names into definite sequences.",
            ],
            [
                "Names resolved in this session include " + ", ".join(resolved) + "."
                if resolved
                else "Every sound ended with competing or absent sensory interpretations.",
                f"The table still held {int(lessons[-1]['supply'])} units of food after the last lesson.",
            ],
        )
    delivered = [q for q in records if q["cascade"] == "pour" and "delivered" in q]
    if delivered:
        q = delivered[-1]
        forecast = [r for r in records if r["cascade"] == "heard" and "reached" in r]
        spoken = [l[9:] for l in data["lines"] if l.startswith("speaker> ")]
        name = q.get("delivered_name", "sensory port " + q["delivered"])
        return (
            "Words before the water moves",
            "One sighted animal   /   a physical intervention",
            "Recognized stages",
            int(q["recognized"]),
            "stages",
            [
                ["Spoken orders", len(forecast)],
                ["Animal replies", len(spoken)],
                ["Food caught", int(q["eaten"])],
            ],
            [
                f"The animal heard {len(forecast)} grounded orders before the experimenter released the physical load.",
                f"The water exited at {name} and {q['settled']} food units settled at its outlet.",
                f"The resident gave {len(spoken)} spoken replies across the conversation.",
            ],
            [
                "Recorded replies in order were " + ", ".join(spoken[:8]) + "."
                if spoken
                else "The resident stayed silent.",
                f"The animal spent {q['heat']} metabolic units on observing and acting.",
            ],
        )
    return None


def cascade_session_panel(d, data, x, y, size):
    grid_background(d, x, y, size)
    records = [pairs(l) for l in data["lines"] if l.startswith("cascade=")]
    lessons = [r for r in records if r["cascade"] == "name"]
    green = "#a9d3ba"
    ink = "#e7edda"
    muted = "#b9c9bd"
    gold = "#edbd84"
    if lessons:
        d.label(x + 24, y + 42, "Visual word acquisition", green)
        d.text(x + 24, y + 94, "What did each sound resolve?", 30, ink, SERIF)
        d.text(x + 24, y + 132, "Remaining sensory candidates per stage", 17, muted)
        colors = ("#efb477", "#a9cba1", "#bc9ed2", "#7bc2c4")
        incoming = [pairs(l) for l in data["lines"] if l.startswith("temporal_word=")]
        for i, r in enumerate(lessons[:4]):
            top = y + 176 + i * 82
            d.text(x + 25, top + 20, r["word"], 25, ink, SERIF)
            detail = next((q for q in incoming if q["temporal_word"] == r["word"]), {})
            width = int(detail.get("length", r["resolved_stages"]))
            if int(r["demonstrations"]):
                for j in range(width):
                    possible = (~int(r["contacts"]) >> (4 * j)) & 15
                    for k, color in enumerate(colors):
                        d.rect(
                            x + 143 + j * 77 + (k % 2) * 20,
                            top + (k // 2) * 13,
                            17,
                            10,
                            color if possible & (1 << k) else "#354640",
                            2,
                        )
            d.text(
                x + 25,
                top + 49,
                f"{r['demonstrations']} observations   /   {r['resolved_stages']} resolved stages",
                15,
                muted,
                MONO,
            )
            d.rule(x + 25, top + 63, x + size - 25, top + 63, "#45574f")
        return
    d.label(x + 24, y + 42, "Observed conversation", green)
    d.text(x + 24, y + 95, "Before and after the pour", 32, ink, SERIF)
    events = [q for q in records if q["cascade"] in ("heard", "pour")]
    for i, r in enumerate(events[-5:]):
        top = y + 155 + i * 77
        physical = r["cascade"] == "pour"
        d.label(
            x + 25,
            top,
            "Physical release" if physical else "Heard order",
            green if physical else gold,
        )
        if physical and "delivered" in r:
            text = "Water reached " + r.get(
                "delivered_name", "sensory port " + r["delivered"]
            )
        elif "reached" in r:
            text = f"{r['recognized']} stages   /   reach " + r.get(
                "reached_name", "sensory port " + r["reached"]
            )
        else:
            text = f"{r['recognized']} recognized stages"
        d.text(x + 25, top + 33, text, 23, ink, SERIF)
        d.rule(x + 25, top + 50, x + size - 25, top + 50, "#45574f")


def lineage_illustration(d, q, x, y, size, ink, muted, gold, green, grid):
    values = q["census"]
    left = x + 57
    right = x + size - 34
    top = y + 183
    bottom = y + 397
    peak = max(max(a, b) for _, a, b in values) or 1
    start, end = values[0][0], values[-1][0]
    for i in range(3):
        h = bottom - i * (bottom - top) / 2
        d.rule(left, h, right, h, grid)
        d.text(
            left - 5, h + 4, human(round(peak * i / 2)), 12, muted, MONO, anchor="end"
        )
    for side, color in ((1, gold), (2, green)):
        points = [
            (
                left + (right - left) * (entry[0] - start) / (end - start),
                bottom - (bottom - top) * entry[side] / peak,
            )
            for entry in values
        ]
        d.path("M" + " L".join(f"{a:.2f} {b:.2f}" for a, b in points), color, 3.5)
        d.dot(*points[-1], 5, color)
    d.text(left, y + 149, "Population at each recorded census", 21, ink, SERIF)
    d.text(left, bottom + 26, f"Tick {start:,}", 15, muted, MONO)
    d.text(right, bottom + 26, f"Tick {end:,}", 15, muted, MONO, anchor="end")
    for side, color in ((1, gold), (2, green)):
        yy = y + 466 + 42 * (side - 1)
        lens = (
            "real endpoint"
            if q["sham"] or q["assigned"] == side
            else "displaced endpoint"
        )
        if q.get("grains"):
            lens = (
                "inherited hand"
                if q["sham"] or q["assigned"] == side
                else "exchanged hand"
            )
        d.dot(left, yy - 7, 6, color)
        d.text(
            left + 17,
            yy,
            f"Mark {side}   /   {lens}   /   {values[-1][side]:,} alive",
            18,
            ink,
        )


def cascade_session_evidence(d, data, x, y, width):
    lessons = [
        pairs(line) for line in data["lines"] if line.startswith("cascade=name ")
    ]
    if lessons:
        d.label(x, y, "What changed in the word contacts")
        for i, q in enumerate(lessons[:4]):
            top = y + 34 + 32 * i
            d.text(x, top, q["word"], 19, INK, SERIF)
            edits = int(q["synaptic_edits"])
            d.rect(x + 105, top - 11, min(width * 0.46, edits * 11), 10, "#c39a64", 2)
            d.text(
                x + width - 4,
                top,
                f"{counted(edits, 'contact edit')}",
                16,
                GREEN,
                MONO,
                anchor="end",
            )
        return
    d.label(x, y, "The resident’s own utterances")
    utterances = [
        line
        for line in data["lines"]
        if line.startswith(("speaker> ", "speaker asks> "))
    ]
    for i, line in enumerate(utterances[-5:]):
        who, text = line.split("> ", 1)
        top = y + 32 + 28 * i
        d.text(x, top, "Asked" if "asks" in who else "Said", 15, GREEN, MONO)
        d.text(x + 94, top, short(text, 52), 20, INK, SERIF)
