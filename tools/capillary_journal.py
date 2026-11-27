"""Measured pressure fields, recovered material and learned causal accounts."""


def capillary_observation(entries, result):
    def numbers(name):
        rows = entries(name)
        if not rows:
            return {}
        return {k: int(v) for k, v in rows[-1].items() if str(v).isdigit()}

    revision=context_observation(entries,result)
    if revision:return revision
    cargo = detritus_observation(numbers, result)
    if cargo:
        return cargo
    field = numbers("capillary_field")
    school = numbers("capillary_school")
    repairs = numbers("capillary_repair")
    interview = numbers("capillary_interview")
    exchange = numbers("capillary_exchange")
    cohort = numbers("capillary_cohort")
    voice = numbers("capillary_instruction")
    atlas = numbers("capillary_scene")
    speed = entries("capillary_speed")
    stamp = next((r for r in speed if r["capillary_speed"] == "compiled"), {})
    local = next((r for r in speed if r["capillary_speed"] == "local"), {})
    wake = numbers("capillary_wakes")
    turns = entries("capillary_turn")
    scene = (
        ("pressure_plate", atlas)
        if atlas and not (exchange or voice or interview)
        else None
    )
    if voice:
        q = voice
        n = q["cases"]
        return result(
            "Instructions for a new channel",
            f"Guide {q['capillary_instruction']:,}   /   listener {q['listener']:,}",
            "Working channels",
            q["after"],
            " repairs",
            [
                ["Solvable", q["possible"]],
                ["Without work", q["before"]],
                ["After instruction", q["after"]],
            ],
            [
                f"The guide and listener faced {n:,} channels, with {q['possible']:,} physically solvable under the stock and movement limits.",
                f"The pupil produced working channels in {q['after']:,} cases after hearing the guide.",
                f"It decoded and completed {q['long_exact']:,} of {q['novel']:,} plans longer than any spoken training demonstration.",
            ],
            [
                f"The listener received {q['words']:,} words.",
                f"Removing a part before placing it mattered on {q['order_matters']:,} plans.",
            ],
            "Construction after speech",
            [
                ("Unaided release", q["before"], n),
                ("Heard instructions", q["after"], n),
                ("Ordered repairs", q["ordered_food"], q["order_matters"]),
            ],
        )
    if exchange:
        q = exchange
        n = q["cases"]
        return result(
            "Advice across the watershed",
            f"Guide {q['capillary_exchange']:,}   /   listener {q['listener']:,}",
            "Food after advice",
            q["after"],
            " portions",
            [
                ["Cases", n],
                ["Before advice", q["before"]],
                ["After advice", q["after"]],
            ],
            [
                f"A guide spoke from its own intervention history in {n:,} unfamiliar channels.",
                f"The listener narrowed its candidate material laws in {q['narrowed']:,} cases.",
                f"It harvested {q['after']:,} portions after the exchange and {q['before']:,} in a matched trial before it.",
            ],
            [
                f"The guide produced {q['words']:,} words.",
                f"The chosen configuration also worked on the opposite substrate in {q['other_material']:,} cases.",
            ],
            "Paired natural materials",
            [
                ("Before hearing", q["before"], n),
                ("After hearing", q["after"], n),
                ("Opposite substrate", q["other_material"], n),
            ],
        )
    if interview:
        q = interview
        forms = entries("capillary_question")
        return result(
            "Why did the pigment change?",
            f"Resident {q['capillary_interview']:,}   /   held-out questions",
            "Exact audible answers",
            q["exact"],
            " answers",
            [
                ["Answerable", q["answerable"]],
                ["Single causes", q["explanations"]],
                ["Unsupported causes", q["unsupported_causes"]],
            ],
            [
                f"The creature received {q['cases']:,} questions about unseen material contrasts.",
                f"It produced the expected words on {q['exact']:,} of {q['answerable']:,} answerable prompts.",
                f"It identified both causes in {q['both_causes']:,} of {q['incompatible_causes']:,} pairs that required different explanations.",
            ],
            [
                f"There were {q['unsupported_causes']:,} unsupported single-cause claims.",
                "Material contacts supplied the counterfactual forecasts.",
            ],
            "Recorded question forms",
            [
                (label, int(v["exact"]), int(v["answerable"]))
                for label, v in zip(
                    (
                        "Single cause",
                        "Possible cause",
                        "Without a part",
                        "Previous pigment",
                        "Isolated action",
                        "Joint cause",
                    ),
                    forms,
                )
            ],
        )
    if stamp and local:
        ratio = float(local["median_ms"]) / float(stamp["median_ms"])
        return result(
            "A material law in four words"
            if int(stamp["bytes"]) == 32
            else "Forecasts with uncertain evidence",
            "Blackwell   /   batched local forecasts",
            "Kernel speed ratio",
            ratio,
            "×",
            [
                ["Independent sheets", int(stamp["sheets"])],
                ["Depth", int(stamp["depth"])],
                ["Image bytes", int(stamp["bytes"])],
            ],
            [
                f"The GPU propagated {int(stamp['sheets']):,} fields through {stamp['depth']} pressure steps.",
                f"The compiled circuit and the per-site implementation gave the same bounds on all {int(stamp['same']):,} fields.",
                f"Median kernel time was {float(stamp['median_ms']):.4f} ms for the compiled circuit and {float(local['median_ms']):.4f} ms for the per-site implementation.",
            ],
            [
                "Timing excludes compilation, host transfers and journaling.",
                f"The final fields contained {int(stamp['lit']):,} certain active sites.",
            ],
            "A sampled worksite",
            [],
            scene,
        )
    if repairs:
        q = repairs
        n = 2 * q["pairs"]
        s = school
        return result(
            "Salvage before assembly" if q.get("scarce") else "A causal model at work",
            f"Resident {q['capillary_repair']:,}   /   {q['depth']} pressure steps   /   {s.get('capacity', 0)}-input supports",
            "Successful repairs",
            q["earned"],
            " repairs",
            [
                ["Solvable", q["possible"]],
                ["Minimum moves", q["shortest"]],
                ["Unsolvable", q["impossible"]],
            ],
            [
                f"The resident chose a construction for {n:,} unfamiliar channels using its own sensory history.",
                f"It earned {q['earned']:,} portions from {q['possible']:,} physically solvable configurations.",
                f"Both members of {q['both']:,} of {q['irreconcilable']:,} pairs were repaired when no shared affordable solution existed.",
            ],
            [
                f"The creature spent {s.get('heat', 0):,} energy units acquiring evidence.",
                f"{q.get('denied_strokes', 0):,} attempted strokes lacked a loose component.",
            ],
            "The recorded pressure paths",
            [
                ("Before intervention", q["before"], n),
                ("After intervention", q["earned"], n),
            ],
            scene,
        )
    partner = numbers("capillary_neighbour")
    if partner:
        q = partner
        return result(
            "A meal downstream",
            f"Resident {q['donor']:,}   /   recipient {q['recipient']:,}",
            "Travel from the handled site",
            q["pulse_steps"],
            " steps",
            [
                ["Enabled portions", wake["attributed_food"]],
                [
                    "Distinct interventions",
                    wake["distinct_interventions_feeding_others"],
                ],
                ["Replayed origins", wake["capillary_wakes"]],
            ],
            [
                f"An animal changed a material at ({q['donor_x']}, {q['donor_y']}) while the population was living freely.",
                f"A different resident ate {q['food']} portions {q['pulse_steps']} pressure steps later in the recorded run.",
                f"Holding subsequent decisions and materials fixed, perturbing the first pulse removed that feeding opportunity.",
            ],
            [
                f"The recorded world contained {wake['candidate_cells']:,} eligible site-times.",
                f"One pressure step spans {q['tick_period']} habitat ticks.",
            ],
            "An observed causal path",
            [],
            ("pressure_wake", q),
        )
    if field and field.get("updates"):
        q = field
        return result(
            "Living channels",
            f"Watershed   /   tick {q['capillary_field']:,}   /   pulse period {q['period']}",
            "Physical material moves",
            q["experiments"],
            " moves",
            [
                ["Pieces placed", q["pieces_placed"]],
                ["Pieces lifted", q["pieces_lifted"]],
                ["Food eaten", q["food"]],
            ],
            [
                f"The watershed advanced {q['updates']:,} local pressure updates.",
                f"Residents moved {q['experiments']:,} pieces and took {q['food']:,} portions of existing litter.",
                f"{q['living_learners']:,} surviving creatures retained at least one learned pressure contact.",
            ],
            [
                f"The pressure cortex occupied {q['resident_bytes']} bytes per resident.",
                f"Recorded replay attributed {wake.get('attributed_food', 0):,} portions to another animal’s intervention."
                if wake
                else f"Creatures retained {q['living_witnesses']:,} witnessed changes.",
            ],
            "Material effects",
            [
                ("Changed stimulus", q["altered"], q["experiments"]),
                ("Correct definite forecasts", q["correct"], q["forecast"]),
            ],
        )
    if cohort and "empty_contacts" in cohort and cohort.get("enrolled"):
        q = cohort
        animals = q["enrolled"]
        strong = q["completely_predicted"]
        trials = q["trials"] * 2
        empty = bool(q["empty_contacts"])
        named = (
            "Disconnected",
            "Free handling",
            "Observe only",
            "Fixed contacts",
            "Experienced inputs only",
        )[field.get("mode", 0) & 7]
        return result(
            "How many young residents learned the law?",
            f"{named}   /   tick {q['capillary_cohort']:,}",
            "Cohort identifying both laws",
            100 * strong / animals,
            "%",
            [
                ["Living pupils", animals],
                ["Both laws identified", strong],
                ["Available releases", animals * trials],
            ],
            [
                f"{animals:,} living animals each encountered {trials} material cases on the same examination.",
                f"Across every case and all 64 local stimuli, {strong:,} animals acquired the correct material laws.",
                f"The cohort completed {q['earned']:,} repairs across {animals * trials:,} releases.",
            ],
            [
                "Pressure contacts were cleared before the exam."
                if empty
                else "Pupils brought their pressure contacts from the watershed.",
                f"The highest-scoring pupil was in generation {q['generation']:,} and had {q['cells']} active spiking cells.",
            ],
            "Cohort transfer",
            [
                ("Both laws identified", strong, animals),
                ("Earned releases", q["earned"], animals * trials),
            ],
        )
    if cohort and cohort.get("enrolled"):
        q = cohort
        return result(
            "Which residents learned the law?",
            f"Wild-born cohort   /   tick {q['capillary_cohort']:,}",
            "Enrolled residents",
            q["enrolled"],
            " creatures",
            [
                ["Best slot", q["slot"]],
                ["Generation", q["generation"]],
                ["Spiking cells", q["cells"]],
            ],
            [
                f"Each of {q['enrolled']:,} naturally born creatures encountered {q['trials']:,} pairs of materials.",
                f"Together they recovered {q['earned']:,} portions.",
                f"The highest-scoring resident was born at tick {q['born']:,} and recovered {q['best']:,} portions.",
            ],
            [
                "The screen sampled eligible living animals.",
                "Generation and active cell count refer to the reported individual.",
            ],
            "Resident performance",
            [("Top resident", q["best"], 2 * q["trials"])],
        )
    if turns:
        spoken = [q for q in turns if int(q["capillary_turn"]) in (7, 8)]
        q = turns[-1]
        n = sum(int(t["words"]) > 0 for t in spoken)
        return result(
            "A conversation at the workbench",
            "Private experience   /   public physical apparatus",
            "Audible replies",
            n,
            " turns",
            [
                ["Questions", len(spoken)],
                ["Food captured", int(q["eaten"])],
                ["Portions remaining", int(q["drops"])],
            ],
            [
                f"The participants received {len(spoken)} questions at the bench.",
                f"They answered audibly on {n} turns.",
                f"The workshop ended with {q['drops']} portions unpoured and {q['eaten']} eaten.",
            ],
            [
                f"The participants spent {sum(int(t['heat']) + int(t['pupil_heat']) for t in turns)} energy units.",
                "Each reply came from the addressed resident’s own learned contacts.",
            ],
            "Measured exchanges",
            [("Audible replies", n, len(spoken))],
        )
    return None


def pressure_illustration(d, q, x, y, size, ink, muted, gold, green, grid):
    gap = 30
    pitch = (size - 3 * gap) / 16
    for side, (label, field, color) in enumerate(
        (
            ("Before handling", "prior_path", "#9876a6"),
            ("After handling", "later_path", "#59c5b0"),
        )
    ):
        ox = x + gap + side * (8 * pitch + gap)
        oy = y + 189
        d.text(ox, oy - 30, label, 23, ink, SANS, 600)
        for i in range(64):
            flag = 1 << i
            px = ox + (i % 8) * pitch
            py = oy + (i // 8) * pitch
            wet = q["wet"] & flag
            flow = q[field] & flag
            d.rect(
                px,
                py,
                pitch - 2,
                pitch - 2,
                color if flow else "#263c42" if wet else "#192c32",
            )
            if q["sill"] & flag:
                d.rule(
                    px + 4, py + pitch - 7, px + pitch - 5, py + pitch - 7, "#be9869", 2
                )
            if (q["source"] | q["goal"]) & flag:
                d.dot(
                    px + pitch / 2,
                    py + pitch / 2,
                    4,
                    ink if q["source"] & flag else gold,
                )
        if side:
            for j in range(6):
                if q["changes"] & (1 << j):
                    seat = (q["handles"] >> (8 * j)) & 63
                    d.dot(
                        ox + (seat % 8 + 0.5) * pitch,
                        oy + (seat // 8 + 0.5) * pitch,
                        pitch * 0.30,
                        "none",
                        f'stroke="{gold}" stroke-width="2"',
                    )
    yy = y + 210 + 8 * pitch
    for i, (color, label) in enumerate(
        (
            ("#9876a6", "Original pulse"),
            ("#59c5b0", "After construction"),
            (gold, "Handled site"),
        )
    ):
        d.dot(x + gap + 6, yy + i * 32, 5, color)
        d.text(x + gap + 23, yy + 6 + i * 32, label, 18, muted, MONO)


def wake_illustration(d, q, x, y, size, ink, muted, gold, green, grid):
    h = q["pulse_steps"]
    pitch = min((size - 84) / (h + 1), 38)
    start = x + 38
    top = y + 145
    for step in range(h + 1):
        wave = q["wave" + str(step)]
        for dx in range(step + 1):
            dy = step - dx
            px = start + dx * pitch
            py = top + dy * pitch
            active = (wave >> dx) & 1
            d.rect(px, py, pitch - 2, pitch - 2, "#2e6b64" if active else "#203337", 3)
            if not active:
                continue
            if step:
                prev = q["wave" + str(step - 1)]
                cx = px + pitch / 2
                cy = py + pitch / 2
                if dx and prev & (1 << (dx - 1)):
                    d.rule(cx - pitch, cy, cx, cy, "#63b9aa", 2)
                if dy and prev & (1 << dx):
                    d.rule(cx, cy - pitch, cx, cy, "#63b9aa", 2)
    for dx, dy, col in ((0, 0, gold), (q["dx"], q["dy"], "#f4a487")):
        d.dot(start + (dx + 0.5) * pitch, top + (dy + 0.5) * pitch, pitch * 0.23, col)
    yy = top + (h + 1) * pitch + 40
    for i, (label, col) in enumerate(
        (
            (f"Material handled by #{q['donor']} at the origin", gold),
            (f"Food eaten by #{q['recipient']} after {h} steps", "#f4a487"),
        )
    ):
        d.dot(start + 5, yy + i * 34 - 5, 5, col)
        d.text(start + 22, yy + i * 34, label, 17, muted, MONO)
