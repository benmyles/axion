"""Field plates for float transport, cultural transmission and testimony."""


def regulatory_observation(entries):
    def last(key):
        return (entries(key) or [{}])[-1]

    def measure(row, key):
        return int(row.get(key, 0))

    school = last("shore_school")
    if school:
        q = school
        enrolled, attended, views = (
            measure(q, k) for k in ("enrolled", "attended", "scenes")
        )
        tests, spoken, correct = (
            measure(q, k) for k in ("tested", "answered", "correct")
        )
        switched = last("shore_reversal")
        channel = {
            "0": "Public teaching",
            "1": "Teaching with return signal muted",
            "2": "Examination only",
        }[q["channel"]]
        note = (
            f"The new convention was heard across {measure(switched, 'after_scenes'):,} later encounters."
            if switched
            else f"{measure(q, 'pointings'):,} names were offered at visible sockets."
        )
        rows = [
            ("Pupils at a real encounter", attended, enrolled),
            ("All eight names", measure(q, "learned_names"), enrolled),
            ("Audible replies", spoken, tests),
            ("Correct before teaching", correct, tests),
            ("Both sides of a change", measure(q, "both"), measure(q, "changed")),
        ]
        return (
            (
                "Lessons on a living shore",
                channel,
                "Correct replies before each lesson",
                100 * correct / max(1, tests),
                "%",
                [
                    ["Residents enrolled", enrolled],
                    ["Real sightings", views],
                    ["Spoken replies", spoken],
                ],
                [
                    f"{attended:,} of {enrolled:,} pupils witnessed a physical encounter.",
                    f"The residents gave {correct:,} correct replies to {tests:,} questions before receiving feedback on that encounter.",
                    note,
                ],
                [
                    f"{measure(q, 'present'):,} original pupils survived to the census.",
                    f"Local learning changed {measure(q, 'form_edits'):,} bits of question memory.",
                ],
            ),
            "What the pupils saw",
            rows,
        )
    language = {q["apprentice_language"]: q for q in entries("apprentice_language")}
    if "inherited" in language:
        q = language["inherited"]
        n = int(q["cases"])
        switch = bool(int(q["switch"]))
        measurement = "twice" if switch else "qualified"
        names = [
            ("inherited", "Cortical program"),
            ("unrestricted", "All contacts open"),
            ("quiet", "Feedback muted"),
            ("displaced", "Displaced contacts"),
            ("yoked", "Another pupil’s spikes"),
        ]
        rows = [
            (label, int(language[key][measurement]), n)
            for key, label in names
            if key in language
        ]
        success = int(q[measurement])
        frames = entries("apprentice_scene")
        if frames:
            physical = {
                int(v["frame"]): v
                for v in frames
                if v["apprentice_scene"] == "inherited"
            }
            labels = (
                "Before the motion",
                "After the motion",
                "If all pulls engaged",
                "Before the last pull",
            )
            rows = [
                (labels[f], int(v["correct"]), int(v["offered"]))
                for f, v in sorted(physical.items())
            ]
            divergent = sum(int(physical[f]["different"]) for f in (2, 3))
            solved = sum(int(physical[f]["different_correct"]) for f in (2, 3))
            return (
                (
                    "What if the last pull had stopped?",
                    f"{n:,} unseen mechanical trials   /   four question forms",
                    "Alternative outcomes distinguished",
                    100 * solved / max(1, divergent),
                    "%",
                    [
                        ["Pupils", n],
                        ["Distinct alternatives", divergent],
                        ["Correct alternatives", solved],
                    ],
                    [
                        f"Each pupil heard four question forms while it observed the behavior of a physical mechanism.",
                        f"The two alternative histories differed from the observed result at {divergent:,} tested sockets.",
                        f"On those sockets the pupils gave {solved:,} correct reports of the alternative outcome.",
                    ],
                    [
                        "The private prediction is computed from the pupil’s learned hand contacts.",
                        "Each answer is checked against an independent execution of the mechanism.",
                    ],
                ),
                "Four versions of one action",
                rows,
            )
        return (
            (
                "When a word changes meaning"
                if switch
                else "Learning unfamiliar questions",
                f"{n:,} independent speech and movement histories",
                "Both conventions mastered" if switch else "Full diagnostics passed",
                100 * success / max(1, n),
                "%",
                [
                    ["Courses", n],
                    ["Correct field replies", int(q["correct"])],
                    ["Public agreements", int(q["agreements"])],
                ],
                [
                    f"Each pupil encountered a new physical lesson and a new dialect.",
                    f"The cortical program passed all eight questions {'under both conventions' if switch else 'after teaching'} in {success:,} courses.",
                    f"After its private event was erased the pupil gave {int(q['erased']):,} replies.",
                ],
                [
                    "Four temporal frames were taught."
                    if frames
                    else "Earlier and later views were taught.",
                    "The playback control receives another pupil’s release schedule.",
                ],
            ),
            "Five matched conditions",
            rows,
        )
    brood = entries("regulatory_brood")
    if brood:
        first = next(q for q in brood if q["generation"] == "0")
        final = [
            next(q for q in reversed(brood) if q["regulatory_brood"] == arm)
            for arm in ("uniform", "food")
        ]
        neutral, selected = final
        rows = [
            (label, int(q["swallowed"]), int(q["offered"]))
            for label, q in zip(
                ("Founders", "Uniform parents", "Food-weighted parents"),
                (first, neutral, selected),
            )
        ]
        budget = last("brood_budget")
        return (
            (
                "When food funds offspring",
                f"Generation {selected['generation']}   /   four outlets",
                "Food caught in unfamiliar currents",
                100 * rows[2][1] / max(1, rows[2][2]),
                "%",
                [
                    ["Living descendants", int(selected["living"])],
                    ["Different circuits", int(selected["circuits"])],
                    ["Validation beads", rows[2][2]],
                ],
                [
                    f"{int(first['living']):,} inherited regulatory circuits entered a wordless four-outlet habitat.",
                    f"After {selected['generation']} generations their descendants caught {rows[2][1]:,} of {rows[2][2]:,} validation beads.",
                    f"Uniform parent sampling caught {rows[1][1]:,} beads in the same currents.",
                ],
                [
                    "Each validation life included two changes of current.",
                    f"{measure(budget, 'births'):,} descendants were paid for with swallowed carbon."
                    if budget
                    else "Each group pays eight beads per offspring.",
                ],
            ),
            "A common set of currents",
            rows,
        )
    survey = last("regulatory_survey")
    if survey:
        q = survey
        rows = [
            ("Saw a real encounter", measure(q, "observing"), measure(q, "living")),
            ("Eligible ears and eyes", measure(q, "eligible"), measure(q, "observing")),
            (
                "Qualified on both conventions",
                measure(q, "qualified_residents"),
                measure(q, "enrolled"),
            ),
        ]
        return (
            (
                "Searching for a flexible pupil",
                f"Tick {int(q['regulatory_survey']):,}",
                "Qualifying residents",
                measure(q, "qualified_residents"),
                " residents",
                [
                    ["Alive", measure(q, "living")],
                    ["Enrolled", measure(q, "enrolled")],
                    ["Dialects per pupil", measure(q, "dialects")],
                ],
                [
                    f"{measure(q, 'observing'):,} sighted residents had a recorded encounter.",
                    f"The observer enrolled {measure(q, 'enrolled'):,} residents after a separate calibration of their name contacts.",
                    f"{measure(q, 'qualified_residents'):,} pupils passed eight questions before and after a change of dialect convention.",
                ],
                [
                    "The qualification held each wild sighting private.",
                    "The test altered no field resident.",
                ],
            ),
            "From shore to classroom",
            rows,
        )
    carriers = entries("regulatory_carriers")
    if len(carriers) > 1:
        opening, closing = carriers[0], carriers[-1]
        n = measure(closing, "exact")
        births = measure(closing, "born_since")
        ports = measure(closing, "both_ports")
        rows = [
            (
                "Exact program at release",
                measure(opening, "exact"),
                measure(opening, "living"),
            ),
            ("Exact program at census", n, measure(closing, "living")),
            ("Born during the run", births, n),
            ("Program and both contact laws", ports, n),
        ]
        return (
            (
                "An inherited circuit on the shore",
                f"Watershed tick {int(closing['regulatory_carriers']):,}",
                "Living carriers of the expressed circuit",
                n,
                " residents",
                [
                    ["Initial carriers", measure(opening, "exact")],
                    ["Living carriers", n],
                    ["Newborn carriers", births],
                ],
                [
                    f"The initial census found {measure(opening, 'exact'):,} residents expressing both diagnostic module words.",
                    f"The habitat advanced {int(closing['regulatory_carriers']) - int(opening['regulatory_carriers']):,} ticks with ordinary feeding, mortality and reproduction.",
                    f"{births:,} living carriers were born during that interval.",
                ],
                [
                    "A carrier must express the exact 64-bit regulatory program.",
                    f"{measure(closing, 'neighbours'):,} other residents expressed a one-bit variant.",
                ],
            ),
            "From introduction to inheritance",
            rows,
        )
    field = last("regulatory_field")
    if field and measure(field, "contacts"):
        q = field
        contacts = measure(q, "contacts")
        attempts = measure(q, "attempted")
        prop = measure(q, "proposed")
        admit = measure(q, "admitted")
        return (
            (
                "A cortical pulse at the waterworks",
                f"Watershed tick {int(q['regulatory_field']):,}",
                "Proposed writes admitted",
                100 * admit / max(1, prop),
                "%",
                [
                    ["Contacts", contacts],
                    ["Writes admitted", admit],
                    ["Somatic bit changes", measure(q, "soma_changes")],
                ],
                [
                    f"Living operators touched {contacts:,} expressed motor contacts.",
                    f"Their cortical activity admitted {admit:,} of {prop:,} proposed bit changes.",
                    f"The action had a definite predicted pigment at {attempts:,} contacts.",
                ],
                [
                    "The somatic signal is used by subsequent neural ticks.",
                    "These counts cover this simulation interval.",
                ],
            ),
            "Physical motor feedback",
            [
                ("Definite predictions", attempts, contacts),
                ("Predictions confirmed", measure(q, "matched"), attempts),
                ("Proposed writes admitted", admit, prop),
            ],
        )
    return None


def regulatory_panel(d, observation, x, y, size, ink, muted, gold, green, grid):
    _, title, rows = observation
    d.label(x + 28, y + 40, "Learning in the watershed", green)
    d.text(x + 28, y + 94, title, 34, ink, SERIF)
    for i, (label, num, den) in enumerate(rows):
        yy = y + 162 + i * 69
        d.text(x + 29, yy, label, 18, ink)
        d.text(x + size - 29, yy, f"{num:,} / {den:,}", 16, gold, MONO, anchor="end")
        d.rect(x + 29, yy + 15, size - 58, 10, grid, 3)
        if den and num:
            d.rect(x + 29, yy + 15, (size - 58) * num / den, 10, green, 3)


def returned_voice_summary(entries):
    def last(key):
        return (entries(key) or [{}])[-1]

    interview = last("field_interview")
    study = last("field_class")
    watch = last("field_watch")
    lesson = last("field_lesson")
    follow = last("field_follow")
    if interview and interview["field_interview"] == "present":
        q = interview
        a, n = int(q["answered"]), int(q["questions"])
        return (
            "Speaking to a living witness",
            f"Resident {int(q['slot']):,}   /   tick {int(q['tick']):,}",
            "Spoken field replies",
            a,
            " replies",
            [
                ["Questions heard", n],
                ["Answered", a],
                ["Somatic cells", int(q["cells"])],
            ],
            [
                f"The witness was born at tick {int(q['born']):,} and belongs to generation {int(q['generation']):,}.",
                f"Its most recent field observation was recorded at tick {int(q['stamp']):,}.",
                f"At the interview it answered {a:,} of {n:,} public questions through its own speech contacts.",
            ],
            [
                "The interview uses the resident’s current private view.",
                "The clock holds at a single field tick during the exchange.",
            ],
        )
    if study:
        q = study
        size = int(q["field_class"])
        individual = int(q.get("individual", 0))
        a = int(q["correct"])
        offered = int(q["offered"])
        names = int(q["named"])
        qual = int(q.get("qualified", 0))
        return (
            "One witness learns new dialects"
            if individual
            else "A classroom drawn from the shore",
            (
                f"Resident {int(q['best_slot']):,}   /   {size:,} lessons"
                if individual
                else f"{size:,} living observers   /   tick {int(q['tick']):,}"
            ),
            "Visible field questions answered correctly",
            100 * a / max(1, offered),
            "%",
            [
                ["Public questions", offered],
                ["Correct field replies", a],
                ["All eight names", names],
            ],
            [
                f"The same resident entered {size:,} independent public dialects."
                if individual
                else f"Classroom copies came from {size:,} residents with an actual field sighting.",
                f"All eight object names were learned in {names:,} cases.",
                f"On the recorded sensory episodes the pupils gave {a:,} correct replies to {offered:,} visible questions.",
            ],
            [
                f"An independent eight-question test was passed in {qual:,} cases."
                if "qualified" in q
                else f"The class returned {int(q['answered']):,} audible answers.",
                f"With the private episode erased, {int(q['without_event']):,} answers were spoken.",
            ],
        )
    if watch:
        q = watch
        found = q["field_watch"] == "qualified"
        p = last("field_qualification")
        return (
            "A reader in the watershed",
            f"Observer checked every {int(q['interval']):,} ticks",
            "Prospective reader qualifications",
            int(found),
            " resident",
            [
                ["Observations", int(q["samples"])],
                ["Candidate visits", int(q["candidate_visits"])],
                ["Qualified", int(found)],
            ],
            [
                f"The observer sampled the live population {int(q['samples']):,} times.",
                f"Eligible speech circuits received independent classroom probes on {int(q['candidate_visits']):,} visits.",
                f"Resident {int(q['slot']):,} passed all eight questions at tick {int(q['tick']):,}."
                if found
                else f"The simulation reached tick {int(q['tick']):,} without a qualifying pupil at those samples.",
            ],
            [
                f"The resident was born at tick {int(p['born']):,}."
                if p
                else "The lookup inspected the contacts present in living residents.",
                "Selection of a pupil used a separate diagnostic arrangement.",
            ],
        )
    if lesson:
        q = lesson
        return (
            "A lesson comes home",
            f"Resident {int(q['field_lesson']):,}   /   tick {int(q['tick']):,}",
            "Returned speech contact changes",
            int(q["changed_bits"]),
            " bits",
            [
                ["Grounded names", int(q["nouns"])],
                ["Constructions", int(q["forms"])],
                ["Changed bits", int(q["changed_bits"])],
            ],
            [
                f"The class working copy belonged to resident {int(q['field_lesson']):,}.",
                f"The home visit applied {int(q['changed_bits']):,} changes in its acquired speech contacts.",
                f"The living animal retained its own field observation from tick {int(q['field_stamp']):,}.",
            ],
            [
                "The original genome and birth record matched the field resident.",
                "The classroom episode remained in the teaching copy.",
            ],
        )
    if follow or interview:
        q = follow or interview
        present = q.get("field_follow") == "present"
        return (
            "Following one resident",
            f"Resident {int(q['slot']):,}   /   tick {int(q['tick']):,}",
            "Living witness at follow-up",
            int(present),
            " resident",
            [
                ["Birth tick", int(q["born"])],
                ["Observation tick", int(q["tick"])],
                ["Present", int(present)],
            ],
            [
                f"The observer followed the identity born at tick {int(q['born']):,}.",
                f"The watershed reached tick {int(q['tick']):,}.",
                "The same resident supplied a portable mind."
                if present
                else "The original animal was absent from its slot.",
            ],
            [
                "Identity follows the animal when it moves across sites.",
                "A later occupant receives its own lifetime record.",
            ],
        )
    return None


def field_voice_panel(d, entries, x, y, size, ink, muted, gold, green, grid):
    interview = (entries("field_interview") or [{}])[-1]
    study = (entries("field_class") or [{}])[-1]
    watch = (entries("field_watch") or [{}])[-1]
    lesson = (entries("field_lesson") or [{}])[-1]
    follow = (entries("field_follow") or [{}])[-1]
    source = next(
        (
            v
            for v in (interview, follow, lesson)
            if "images" in v
            and int(v.get("stamp", v.get("event", v.get("field_stamp", 0))))
        ),
        None,
    )
    if source:
        d.label(x + 28, y + 40, "Private retinal record", green)
        d.text(x + 28, y + 93, "Four sockets on the shore", 35, ink, SERIF)
        images, known = int(source["images"]), int(source["known"])
        palette = ("#f3bb71", "#a1c27e", "#b595c1", "#79c5ba")
        labels = ("amber", "jade", "plum", "teal")
        left = x + 122
        w = (size - 152) / 4
        for i in range(4):
            d.text(
                left + (i + 0.5) * w,
                y + 151,
                f"Socket {i + 1}",
                16,
                muted,
                MONO,
                anchor="middle",
            )
        changes = 0
        for j, caption in enumerate(("Before", "After")):
            yy = y + 200 + j * 112
            d.text(x + 27, yy + 10, caption, 22, ink, SERIF)
            for i in range(4):
                xx = left + (i + 0.5) * w
                idx = j * 4 + i
                hue = (images >> (2 * idx)) & 3
                seen = known & (1 << idx)
                d.dot(xx, yy, 24, palette[hue] if seen else grid)
                d.text(
                    xx,
                    yy + 45,
                    labels[hue] if seen else "hidden",
                    14,
                    muted,
                    MONO,
                    anchor="middle",
                )
                if (
                    j
                    and (known & (17 << i)) == (17 << i)
                    and hue != ((images >> (2 * i)) & 3)
                ):
                    changes += 1
        d.rule(x + 28, y + 391, x + size - 28, y + 391, grid)
        d.text(x + 28, y + 437, str(changes), 38, gold, MONO)
        d.text(x + 28, y + 473, "sockets visibly changed dye", 20, ink)
        d.text(
            x + 28,
            y + 538,
            "The two rows come from the same real field episode.",
            16,
            muted,
        )
        d.text(
            x + 28,
            y + 570,
            "Colours refer to this witness’s local sensory channels.",
            16,
            green,
        )
    elif study:
        d.label(x + 28, y + 40, "Observed language learning", green)
        d.text(x + 28, y + 93, "What the pupils could report", 32, ink, SERIF)
        n = int(study["field_class"])
        offered = int(study["offered"])
        choices = [
            ("Learned all eight names", int(study["named"]), n),
            ("Passed all diagnostic questions", int(study.get("qualified", 0)), n),
            ("Correct on the field episode", int(study["correct"]), offered),
            ("Correct with the classroom scene", int(study["classroom_only"]), offered),
        ]
        for j, (name, v, total) in enumerate(choices):
            yy = y + 164 + j * 82
            d.text(x + 28, yy, name, 17, ink)
            d.text(
                x + size - 28, yy, f"{v:,} / {total:,}", 16, gold, MONO, anchor="end"
            )
            d.rect(x + 28, yy + 15, size - 56, 11, grid, 3)
            if v:
                d.rect(x + 28, yy + 15, (size - 56) * v / max(1, total), 11, green, 3)
        d.text(x + 28, y + 510, "Private episode erased", 18, muted)
        d.text(
            x + size - 28,
            y + 510,
            f"{int(study['without_event']):,} replies",
            19,
            green,
            MONO,
            anchor="end",
        )
    else:
        d.label(x + 28, y + 40, "Individual field record", green)
        title = (
            "Watching for a reader"
            if watch
            else "Returning to the same animal"
            if lesson
            else "A lifetime follow-up"
        )
        d.text(x + 28, y + 91, title, 32, ink, SERIF)
        if watch:
            vals = [
                ("Field samples", watch["samples"]),
                ("Candidate visits", watch["candidate_visits"]),
                ("Clock at observation", watch["tick"]),
            ]
        elif lesson:
            vals = [
                ("Acquired contact changes", lesson["changed_bits"]),
                ("Learned names", lesson["nouns"]),
                ("Field sight tick", lesson["field_stamp"]),
            ]
        else:
            q = follow or interview
            vals = [
                ("Original birth tick", q["born"]),
                ("Observed at tick", q["tick"]),
                ("Found alive", int(q.get("field_follow") == "present")),
            ]
        for j, (name, v) in enumerate(vals):
            yy = y + 166 + j * 102
            d.label(x + 29, yy, name, muted)
            d.text(x + 29, yy + 44, f"{int(v):,}", 34, gold, MONO)


def braid_summary(entries):
    def arms(key):
        return {row[key]: row for row in entries(key)}

    def number(row, key):
        return int(row.get(key, 0))

    def count(row, key):
        return f"{number(row, key):,}"

    def study(title, subtitle, family, row, right, total, stats, narrative, notes):
        rate = 100 * number(row, right) / max(1, number(row, total))
        return title, subtitle, family, rate, "%", stats, narrative, notes

    drift = drift_observation(entries)
    if drift:
        return drift[0]
    regulation = regulatory_observation(entries)
    if regulation:
        return regulation[0]
    field_voice = returned_voice_summary(entries)
    if field_voice:
        return field_voice
    gates = arms("gate_trial")
    if gates:
        native = gates["intact"]
        elapsed = number(native, "tick") - number(native, "start")
        enrolled = sum(number(r, "enrolled") for r in gates.values())
        return (
            "Do acquired gates aid survival?",
            f"Free-living residents   /   {elapsed:,} ticks after enrollment",
            "Survival after retaining acquired gates",
            100 * number(native, "surviving") / max(1, number(native, "enrolled")),
            "%",
            [
                ["Enrolled residents", enrolled],
                ["Observation ticks", elapsed],
                ["Assignment rotation", number(native, "phase")],
            ],
            [
                f"The experiment tracked {enrolled:,} residents whose continuously present natal cells carried altered synaptic gates.",
                "All three groups kept their original gates in this sham comparison."
                if number(native, "sham")
                else "One group retained the gates, one reverted them to their birth state, and one shifted the acquired changes between cells.",
                f"After {elapsed:,} ticks, {count(native, 'surviving')} of {count(native, 'enrolled')} residents in the intact group survived.",
            ],
            [
                f"Among residents assigned to reversion, {count(gates['reverted'], 'surviving')} of {count(gates['reverted'], 'enrolled')} survived.",
                f"Among residents assigned to rotation, {count(gates['rotated'], 'surviving')} of {count(gates['rotated'], 'enrolled')} survived.",
            ],
        )
    frontier = arms("frontier_census").get("end")
    if frontier:
        q = frontier
        return (
            "Settlement beyond the old shore",
            f"{count(q, 'side')} × {count(q, 'side')} watershed   /   tick {count(q, 'tick')}",
            "Residents outside the original shore",
            number(q, "outside"),
            " residents",
            [
                ["Core residents", number(q, "core_residents")],
                ["Outside residents", number(q, "outside")],
                ["Steps beyond boundary", number(q, "furthest")],
            ],
            [
                f"The first survey counted {count(q, 'opening')} residents beyond the {count(q, 'origin')}-site square.",
                f"By tick {count(q, 'tick')}, that territory held {count(q, 'outside')} residents from {count(q, 'founders')} founder line{'s' if number(q, 'founders') != 1 else ''}.",
                f"Of those residents, {count(q, 'born_after')} were born after tick {count(q, 'start')}.",
            ],
            [
                f"Residents beyond the original shore expressed {count(q, 'expressed_cells')} spiking cells in total.",
                f"The outer territory contained fibre along outgoing edges at {count(q, 'fibre_sites')} sites.",
            ],
        )
    neural = entries("neural_benchmark")
    if neural:
        q = neural[-1]
        mapping = entries("neural_dispatch")[-1]
        arena = (entries("neural_storage") or [{}])[-1]
        return (
            "Cortices that share a warp",
            f"{count(mapping, 'living')} residents   /   {count(mapping, 'expressed_lanes')} expressed bands",
            "Cortical kernel speed ratio",
            float(q["ratio"]),
            "×",
            [
                ["Expressed bands", number(mapping, "expressed_lanes")],
                ["Neural ticks", number(q, "neural_benchmark")],
                ["Checked bytes", number(q, "compared_bytes")],
            ],
            [
                f"The hardware compared {count(q, 'neural_benchmark')} successive neural ticks of two copies with the same sensory input.",
                f"Fixed-width groups took {float(q['fixed_ms']):.3f} ms and packing the active bands took {float(q['packed_ms']):.3f} ms.",
                f"All {count(q, 'compared_bytes')} bytes of neural tissue, motor output, body state and auditory memory matched.",
            ],
            [
                f"{count(mapping, 'irregular')} residents expressed a non-power-of-two number of bands.",
                f"The production neural arena reserved {count(arena, 'device_bytes')} device bytes."
                if arena
                else "The scheduler requires no global work queue.",
            ],
        )
    perspectives = arms("braid_parallax")
    if perspectives:
        q = perspectives["calibrated"]
        witnesses = arms("braid_audiences").get("calibrated")
        notes = [
            f"Treating the two local frames as identical produced {count(perspectives['literal'], 'grounded')} fully grounded replies.",
            f"After the instruments moved, further comparisons restored {count(perspectives['readapted'], 'grounded')} grounded replies.",
        ]
        if witnesses:
            notes[1] = (
                f"With a single call, the speaker chose the only useful witness in {count(witnesses, 'found')} of {count(witnesses, 'sole')} opportunities."
            )
        return study(
            "Seen from a different shore",
            "Two witnesses   /   private retinal positions and pigment bands",
            "Perspective-grounded replies",
            q,
            "grounded",
            "offered",
            [
                ["Public probes", number(q, "offered")],
                ["Grounded replies", number(q, "grounded")],
                ["Correct unique frames", number(q, "true_frames")],
            ],
            [
                f"Two witnesses in each of {count(q, 'communities')} communities observed mobile floats through independent optical connections.",
                f"The speaker heard {count(q, 'heard')} calibration replies about shared sights.",
                f"Both witnesses supplied the right local wording and outcome in {count(q, 'both')} of {count(q, 'conflicts')} paired conflicts.",
            ],
            notes,
        )
    exchanges_view = entries("braid_view")
    if exchanges_view:
        q = exchanges_view[-1]
        receipts = entries("braid_recall")
        return (
            "Learning a neighbour’s viewpoint",
            "An individual dialogue through different optical instruments",
            "Personal perspective comparisons",
            sum(number(v, "replies") for v in exchanges_view),
            " replies",
            [
                [
                    "Comparisons heard",
                    sum(number(v, "replies") for v in exchanges_view),
                ],
                [
                    "Questions issued",
                    sum(number(v, "questions") for v in exchanges_view),
                ],
                ["Remaining frames", number(q, "candidates")],
            ],
            [
                f"The speaker inspected {sum(number(v, 'panels') for v in exchanges_view):,} shared arrangements.",
                f"It heard {sum(number(v, 'replies') for v in exchanges_view):,} answers naming a pigment from the other eye.",
                f"The latest inspected acquaintance retained {count(q, 'candidates')} possible correspondences.",
            ],
            [
                f"The speakers accepted {sum(number(v, 'received') for v in receipts):,} situated reports.",
                f"The apparatus spent {sum(number(v, 'cost') for v in exchanges_view):,} units on the comparisons.",
            ],
        )
    inquiries = arms("braid_inquiry")
    if inquiries:
        q = inquiries["witness"]
        return study(
            "A question for the witness",
            "Obscured movement   /   shared testimony",
            "Grounded event reports",
            q,
            "correct",
            "offered",
            [
                ["Public follow-ups", number(q, "offered")],
                ["Correct answers", number(q, "correct")],
                ["Changed pairs solved", number(q, "both")],
            ],
            [
                f"The first speaker could see its starting tray and motor plan across {count(q, 'offered')} follow-ups.",
                f"It made {count(q, 'explicit')} requests that specified the subject for a witness who missed the earlier conversation.",
                f"After the witness left, it reversed {count(q, 'retained')} correct reports to answer a question about the other member of the same pair.",
            ],
            [
                f"A muted return yielded {count(inquiries['muted'], 'correct')} correct answers.",
                f"With both views obscured, the speakers supplied {count(inquiries['both_blind'], 'answered')} answers.",
            ],
        )
    exchanges = arms("braid_exchange")
    if exchanges:
        q = exchanges["translated"]
        return study(
            "Three bodies, two dialects",
            "Independent muscles, vocabularies and starting arrangements",
            "Interpreted float requests",
            q,
            "reached",
            "offered",
            [
                ["Requests", number(q, "offered")],
                ["Translated and enacted", number(q, "reached")],
                ["Paired reversals", number(q, "both")],
            ],
            [
                f"The operator received {count(q, 'offered')} public requests in its local dialect.",
                f"The interpreter requested {count(q, 'questions')} additional views of the movement.",
                f"The distant worker carried out {count(q, 'reached')} requests with the correct inferred transformation and physical endpoint.",
            ],
            [
                f"A single view yielded {count(exchanges['one_view'], 'reached')} successful interpretations.",
                f"After temporary instruction, {count(exchanges['borrowed'], 'later_correct')} mixed-language requests reached the requested arrangement.",
            ],
        )
    culture = arms("braid_culture")
    if culture:
        q = culture["shared"]
        return study(
            "Sounds that outlive their makers",
            "Invented calls across individual lifetimes",
            "Inherited public dialects",
            q,
            "physical",
            "filter_targets",
            [
                ["Communities", number(q, "communities")],
                ["Generations", number(q, "generations")],
                ["Filter targets", number(q, "filter_targets")],
            ],
            [
                f"Founders in {count(q, 'communities')} paired communities attached calls to movements they performed.",
                f"Across {count(q, 'generations')} generations the workers fulfilled {count(q, 'physical')} of {count(q, 'filter_targets')} independent filter requests.",
                f"The final generation retained {count(q, 'final_recorded')} founder call associations in the recorded lineages."
                if number(q, "generations") <= 8
                else f"The first generation passed on {count(q, 'first_retained')} founder call associations.",
            ],
            [
                f"Muting the lesson left {count(culture['muted'], 'physical')} physical successes.",
                f"Workers received {count(q, 'filter_food')} units of filter carbon, including incidental matches.",
            ],
        )
    trials = arms("braid_trial")
    if trials:
        q = trials["grounded"]
        return study(
            "Learning what a sequence does",
            "Paired histories   /   private hands   /   unseen requests",
            "Compositional float work",
            q,
            "exact",
            "probes",
            [
                ["New requests", number(q, "probes")],
                ["Correct outcomes", number(q, "exact")],
                ["Opposed histories solved", number(q, "both")],
            ],
            [
                f"Each {count(q, 'words')}-word community taught movements through the visible consequences of its phrases.",
                f"Workers reached the requested arrangement on {count(q, 'exact')} of {count(q, 'probes')} subsequent requests.",
                f"Under a changed meaning, both physical responses were correct in {count(q, 'both')} pairs.",
            ],
            [
                f"The isolated-word control reached {count(trials['single_examples'], 'exact')} requested outcomes.",
                f"The animals spent {count(q, 'cost')} units of energy on their probe movements.",
            ],
        )
    talk = entries("braid_recall") or entries("braid_talk")
    if talk:
        recalls = entries("braid_recall")
        work = entries("braid_talk")
        total = lambda records, key: sum(number(row, key) for row in records)
        pulls = total(work, "spent")
        heard = total(recalls, "received")
        return (
            "At the float table",
            "An individual conversation and its physical consequences",
            "Float table work",
            pulls,
            " pulls",
            [
                ["Physical pulls", pulls],
                ["Reports heard", heard],
                ["Tuition spent", total(recalls, "cost") + total(work, "incurred")],
            ],
            [
                f"The participants spent {pulls:,} motor and demonstration units at the float table.",
                f"Their event speakers made {total(recalls, 'asked'):,} requests for a witness to resolve a question.",
                f"They accepted {heard:,} acoustic reports about a specific port and pigment.",
            ],
            [
                f"The apparatus recorded {total(work, 'consultations'):,} repeat demonstrations.",
                f"Grounded event lessons recorded {total(recalls, 'writes'):,} changes to inhibitory contacts.",
            ],
        )
    brood = entries("braid_brood")
    if brood:
        q = brood[-1]
        first = brood[0]
        return study(
            "Selection in the workshop",
            "Food from a physical filter across generations",
            "Workshop carbon recovery",
            q,
            "recovered",
            "offered",
            [
                ["Generations", number(q, "braid_brood")],
                ["Parents per cohort", number(q, "parents")],
                ["Carbon offered", number(q, "offered")],
            ],
            [
                f"A cohort of {count(q, 'parents')} parents supplied variation in acoustic and motor tissue.",
                f"The first recorded generation recovered {count(first, 'recovered')} units of filter carbon.",
                f"At generation {count(q, 'braid_brood')} the recovery reached {count(q, 'recovered')} units.",
            ],
            [
                f"The parent sampling treatment was {q['selection']}.",
                "Acquired contacts start empty in each new learner.",
            ],
        )
    eyes = entries("braid_eyes")
    if eyes:
        q = eyes[-1]
        signals = entries("braid_signals")[-1]
        field = entries("braid_field")[-1]
        return (
            "The estuary through its own eyes",
            f"Watershed tick {count(q, 'braid_eyes')}   /   {count(q, 'residents')} residents",
            "Distinct expressed optical frames",
            number(q, "perspectives"),
            " frames",
            [
                ["Retinal wirings", number(q, "perspectives")],
                ["Turned socket maps", number(q, "turned")],
                ["Filtered pigment maps", number(q, "filtered")],
            ],
            [
                f"The living population expressed {count(q, 'perspectives')} combinations of position and spectral wiring.",
                f"Private observations of a recent float movement survived in {count(q, 'recollections')} residents.",
                f"Removing material input from a replay of the somatic state changed {count(signals, 'material_moves')} sampled motor choices over this history.",
            ],
            [
                f"{count(field, 'distant_food')} units came from floats at least three sites from their origin.",
                f"{count(q, 'filtered')} residents expressed a nonidentity pigment mapping.",
            ],
        )
    sediment = entries("braid_sediment")
    if sediment:
        z = sediment[-1]
        q = entries("braid_field")[-1]
        return (
            "A bank that changes its own outlets",
            f"Watershed tick {count(q, 'braid_field')}   /   {z['mode']} sediment",
            "Carbon admitted by mineral relief",
            number(z, "gained"),
            " units",
            [
                ["Grains lodged", number(z, "lodged")],
                ["Grains washed free", number(z, "scoured")],
                ["Flows opened by relief", number(z, "opened")],
            ],
            [
                f"Clay at neighbouring banks set the registration of four woven outlets for {count(q, 'living')} living residents.",
                f"Across the recorded history, passing floats lodged {count(z, 'lodged')} grains and obstructed flows washed {count(z, 'scoured')} back into the loose stock.",
                f"Against the same flows with flat collars, relief admitted {count(z, 'gained')} additional carbon units and blocked {count(z, 'lost')} others.",
            ],
            [
                f"{count(z, 'under_motion')} grains lodged after a worker changed the float arrangement.",
                f"Creatures consumed {count(q, 'distant_food')} carbon units from distant carriers over this watershed history.",
            ],
        )
    fields = entries("braid_field")
    if fields:
        q = fields[-1]
        s = entries("braid_signals")[-1]
        g = (entries("braid_gates") or [{}])[-1]
        return (
            "The working estuary",
            f"Watershed tick {count(q, 'braid_field')}   /   {count(q, 'living')} living bodies",
            "Carbon from transported floats",
            number(q, "distant_food"),
            " units",
            [
                ["Floating carriers", number(q, "floats")],
                ["Carbon in transit", number(q, "carbon")],
                ["Carbon eaten", number(q, "swallowed")],
            ],
            [
                f"The watershed contained {count(q, 'living')} living residents and {count(q, 'floats')} independently movable floats.",
                f"Residents consumed {count(q, 'distant_food')} units of carbon from floats at least three sites from their origin.",
                f"Replaying identical somatic states without the float input changed {count(s, 'material_moves')} motor choices.",
            ],
            [
                f"Creature-built links conducted {count(g, 'carbon')} units through the collar filters."
                if g
                else f"{count(q, 'carried_floats')} carriers bore a movement mark.",
                f"Removing the bank signal changed {count(s, 'bank_moves')} sampled motor choices.",
            ],
        )
    return None


def braid_plate(d, data, b, x, y, size, rx, rw):
    def entries(key):
        return [pairs(line) for line in data["lines"] if line.startswith(key + "=")]

    ink, muted, gold, green, grid = (
        "#edf1dc",
        "#b5c6b5",
        "#f3bc73",
        "#91cfb7",
        "#365448",
    )
    d.rect(x, y, size, size, "#1b322f", 13)
    labels = {
        "braid_parallax": (
            "What did the other eye see?",
            "grounded",
            "offered",
            (
                ("calibrated", "Shared comparisons"),
                ("literal", "Literal local names"),
                ("wrong_voice", "Partner identity exchanged"),
                ("same_panel", "Unchanged reference panel"),
                ("turned", "New optical frame"),
                ("readapted", "After further comparisons"),
            ),
        ),
        "braid_inquiry": (
            "Testimony and recall",
            "correct",
            "offered",
            (
                ("witness", "Witness heard"),
                ("visible", "First-hand sight"),
                ("echoed", "Follow-up echoed"),
                ("no_context", "Prior topic erased"),
                ("muted", "Return muted"),
                ("untaught", "Before lessons"),
            ),
        ),
        "braid_exchange": (
            "A message changes hands",
            "reached",
            "offered",
            (
                ("translated", "Interpreter present"),
                ("one_view", "One view"),
                ("same_sample", "Unchanged marker"),
                ("garbled", "Distorted sound"),
                ("untranslated", "Foreign words"),
                ("muted", "Return muted"),
            ),
        ),
        "braid_culture": (
            "Across generations",
            "physical",
            "filter_targets",
            (
                ("shared", "Local transmission"),
                ("unpaired", "Mismatched sights"),
                ("shifted", "Shifted calls"),
                ("unhandled", "Uncalibrated hand"),
                ("one_body", "Same body"),
                ("muted", "Silent lessons"),
            ),
        ),
        "braid_trial": (
            "Reaching a new arrangement",
            "exact",
            "probes",
            (
                ("grounded", "Learned contacts"),
                ("single_examples", "Only isolated words"),
                ("unhandled", "Hand uncalibrated"),
                ("wrist_turned", "Wrist turned"),
                ("frozen", "Fixed tissue"),
                ("cut_strand", "Severed linkage"),
            ),
        ),
    }
    neural = entries("neural_benchmark")
    frontier = {v["frontier_census"]: v for v in entries("frontier_census")}.get("end")
    gates = {r["gate_trial"]: r for r in entries("gate_trial")}
    drift = drift_observation(entries)
    regulation = regulatory_observation(entries)
    field_voice = any(
        entries(k)
        for k in (
            "field_interview",
            "field_class",
            "field_watch",
            "field_lesson",
            "field_follow",
        )
    )
    for key, (title, num, den, selection) in (
        {}
        if neural or frontier or gates or field_voice or regulation or drift
        else labels
    ).items():
        rows = {r[key]: r for r in entries(key)}
        if not rows:
            continue
        d.label(x + 28, y + 40, "Matched interventions", green)
        d.text(x + 28, y + 91, title, 35, ink, SERIF)
        d.text(x + 29, y + 122, "Successful physical outcomes or replies", 17, muted)
        for i, (code, label) in enumerate(selection):
            r = rows[code]
            a = int(r[num])
            n = int(r[den])
            rate = a / max(1, n)
            yy = y + 174 + 63 * i
            d.text(x + 30, yy, label, 18, ink)
            d.text(x + size - 32, yy, f"{a:,} / {n:,}", 17, gold, MONO, anchor="end")
            d.rect(x + 30, yy + 13, size - 62, 12, grid, 3)
            if rate:
                d.rect(x + 30, yy + 13, (size - 62) * rate, 12, green, 3)
        break
    else:
        brood = entries("braid_brood")
        talk = entries("braid_talk")
        recall = entries("braid_recall")
        if drift:
            drift_panel(d, drift, x, y, size, ink, muted, gold, green, grid)
        elif regulation:
            regulatory_panel(d, regulation, x, y, size, ink, muted, gold, green, grid)
        elif field_voice:
            field_voice_panel(d, entries, x, y, size, ink, muted, gold, green, grid)
        elif gates:
            native = gates["intact"]
            elapsed = int(native["tick"]) - int(native["start"])
            d.label(x + 28, y + 39, "Lifetime wiring", green)
            d.text(x + 28, y + 89, "Who survived?", 35, ink, SERIF)
            d.text(
                x + 28,
                y + 123,
                f"Original residents present after {elapsed:,} ticks",
                17,
                muted,
            )
            for j, (code, label, color) in enumerate(
                (
                    ("intact", "Retained gates", green),
                    ("reverted", "Birth-state gates", gold),
                    ("rotated", "Shifted changes", "#aebbe2"),
                )
            ):
                q = gates[code]
                survived, adults = int(q["surviving"]), int(q["enrolled"])
                yy = y + 194 + 93 * j
                d.text(x + 29, yy, label, 20, ink)
                d.text(
                    x + size - 31,
                    yy,
                    f"{survived:,} / {adults:,}",
                    18,
                    color,
                    MONO,
                    anchor="end",
                )
                d.rect(x + 30, yy + 18, size - 61, 14, grid, 4)
                if survived:
                    d.rect(
                        x + 30, yy + 18, (size - 61) * survived / adults, 14, color, 4
                    )
            d.text(
                x + 28,
                y + 510,
                f"Assignment rotation {native['phase']} of 0, 1, 2",
                17,
                muted,
            )
            d.text(
                x + 28,
                y + 548,
                "Sham exposure"
                if int(native["sham"])
                else "Living neighbours shared the watershed",
                17,
                green,
            )
        elif frontier:
            rows = sorted(entries("frontier_row"), key=lambda r: int(r["frontier_row"]))
            cells = [list(map(int, r["residents"].split(","))) for r in rows]
            d.label(x + 27, y + 39, "Living settlement", green)
            d.text(x + 27, y + 86, "Beyond the original shore", 35, ink, SERIF)
            if cells:
                bins = len(cells)
                peak = max(1, max(map(max, cells)))
                side, origin = int(frontier["side"]), int(frontier["origin"])
                series = sorted(
                    {
                        (int(v["tick"]), int(v["outside"]))
                        for v in entries("frontier_census")
                    }
                )
                chart = len(series) >= 2 and series[-1][0] > series[0][0]
                width = min(309 if chart else 373, size - 104)
                px, py, tile = x + (size - width) / 2, y + 105, width / bins
                for j, row in enumerate(cells):
                    for i, population in enumerate(row):
                        colour = blend(
                            "#253d3c", "#c3e4ab", (population / peak) ** 0.55
                        )
                        d.rect(
                            px + i * tile, py + j * tile, tile - 0.4, tile - 0.4, colour
                        )
                edge = width * origin / side
                for x1, y1, x2, y2 in (
                    (0, 0, edge, 0),
                    (0, 0, 0, edge),
                    (edge, 0, edge, edge),
                    (0, edge, edge, edge),
                ):
                    d.rule(px + x1, py + y1, px + x2, py + y2, gold, 2)
                if chart:
                    d.text(
                        x + 27,
                        y + 441,
                        f"Amber · old shore     Green · 0–{peak:,} residents per tile",
                        16,
                        muted,
                    )
                    d.label(x + 27, y + 475, "Residents beyond the boundary", green)
                    left, right, baseline = x + 30, x + size - 32, y + 546
                    first, last = series[0][0], series[-1][0]
                    high = max(1, max(value for _, value in series))
                    points = [
                        (
                            left + (right - left) * (tick - first) / (last - first),
                            baseline - 52 * value / high,
                        )
                        for tick, value in series
                    ]
                    d.rule(left, baseline, right, baseline, grid)
                    for (xa, ya), (xb, yb) in zip(points, points[1:]):
                        d.rule(xa, ya, xb, yb, green, 3)
                    for xx, yy in points:
                        d.dot(xx, yy, 3, gold)
                    d.text(
                        left,
                        y + 579,
                        f"{series[0][1]:,} at tick {first:,}",
                        15,
                        muted,
                        MONO,
                    )
                    d.text(
                        right,
                        y + 579,
                        f"{series[-1][1]:,} at tick {last:,}",
                        15,
                        green,
                        MONO,
                        anchor="end",
                    )
                else:
                    d.text(
                        x + 27,
                        y + 514,
                        f"Each tile covers {side // bins} × {side // bins} sites",
                        17,
                        muted,
                    )
                    d.text(
                        x + 27,
                        y + 545,
                        f"Pale green · up to {peak:,} living residents per tile",
                        17,
                        green,
                    )
                    d.text(
                        x + 27,
                        y + 576,
                        f"Amber boundary · original {origin:,} × {origin:,} shore",
                        17,
                        gold,
                    )
        elif neural:
            q = neural[-1]
            order = entries("neural_dispatch")[-1]
            d.label(x + 27, y + 39, "Paired GPU execution", green)
            d.text(x + 27, y + 91, "The same neural update", 35, ink, SERIF)
            times = [float(q["fixed_ms"]), float(q["packed_ms"])]
            for j, (name, value) in enumerate(
                zip(("Fixed-width groups", "Packed active bands"), times)
            ):
                yy = y + 167 + 94 * j
                d.text(x + 30, yy, name, 19, ink)
                d.text(
                    x + size - 30, yy, f"{value:.3f} ms", 20, gold, MONO, anchor="end"
                )
                d.rect(x + 30, yy + 17, size - 60, 15, grid, 4)
                d.rect(x + 30, yy + 17, (size - 60) * value / max(times), 15, green, 4)
            d.rule(x + 28, y + 334, x + size - 28, y + 334, grid)
            d.text(x + 30, y + 391, f"{float(q['ratio']):.2f}×", 48, gold, SERIF)
            d.text(x + 30, y + 427, "Fixed time divided by packed time", 18, muted)
            d.text(
                x + 30,
                y + 491,
                f"{int(order['expressed_lanes']) * 32:,} expressed spiking cells",
                23,
                ink,
                SERIF,
            )
            d.text(
                x + 30, y + 546, "Median of five event-timed intervals", 16, green, MONO
            )
        elif brood:
            d.label(x + 28, y + 40, "Food returned by generation", green)
            d.text(x + 28, y + 92, "What selection pays for", 36, ink, SERIF)
            points = [
                (int(r["braid_brood"]), int(r["recovered"]) / max(1, int(r["offered"])))
                for r in brood
            ]
            span = max(1, points[-1][0] - points[0][0])
            left = x + 74
            base = y + 468
            for pct in (0, 25, 50, 75, 100):
                yy = base - 2.84 * pct
                d.rule(left, yy, x + size - 40, yy, grid)
                d.text(left - 10, yy + 4, str(pct), 14, muted, MONO, anchor="end")
            dots = [
                (left + (n - points[0][0]) * (size - 122) / span, base - 284 * ratio)
                for n, ratio in points
            ]
            for (a, c), (e, f) in zip(dots, dots[1:]):
                d.rule(a, c, e, f, green, 3)
            for a, c in dots:
                d.dot(a, c, 5, gold)
            d.text(left, base + 36, f"Generation {points[0][0]}", 16, muted)
            d.text(
                x + size - 38, base + 36, str(points[-1][0]), 16, muted, anchor="end"
            )
        elif talk or recall:
            d.label(x + 28, y + 40, "Individual observation", green)
            d.text(x + 28, y + 92, "The last observed pull", 36, ink, SERIF)
            performed = [
                r for r in talk if r.get("understood") == "1" and r.get("known") == "15"
            ]
            palette = ("#e9ac76", "#a9c39a", "#b9aace", "#85c1c9")
            if performed:
                r = performed[-1]
                for j, (key, label) in enumerate(
                    (("start", "Before"), ("expected", "Expected"), ("end", "Observed"))
                ):
                    yy = y + 175 + j * 89
                    d.text(x + 30, yy + 26, label, 19, muted)
                    for i in range(4):
                        val = (int(r[key]) >> (2 * i)) & 3
                        xx = x + 185 + 80 * i
                        d.dot(xx, yy + 18, 23, palette[val])
                        d.text(
                            xx, yy + 25, str(val), 20, "#20362f", MONO, 600, "middle"
                        )
            else:
                d.text(
                    x + 30,
                    y + 183,
                    "The session recorded no recognized pull.",
                    20,
                    muted,
                )
            d.text(
                x + 30,
                y + 493,
                f"{sum(int(r['answered']) for r in recall):,} event replies recorded",
                22,
                green,
                SERIF,
            )
        elif entries("braid_retina"):
            rows = entries("braid_retina")
            palette = ("#e4b479", "#a4bc8e", "#bba0ca", "#78afb8")
            dim = "#334b45"
            d.label(x + 27, y + 39, "Optical census", green)
            d.text(x + 27, y + 88, "One shore, many frames", 36, ink, SERIF)
            step = 402 / len(rows)
            left = x + 102
            top = y + 114
            for j, row in enumerate(rows):
                cells = row["eyes"]
                for i in range(len(cells) // 2):
                    v = int(cells[2 * i : 2 * i + 2], 16)
                    share = v >> 4
                    hue = (v >> 2) & 3
                    direction = v & 3
                    xx = left + i * step
                    yy = top + j * step
                    d.rect(
                        xx,
                        yy,
                        step - 1,
                        step - 1,
                        blend(dim, palette[hue], 0.20 + 0.72 * share / 15)
                        if share
                        else dim,
                        1,
                    )
                    if share:
                        dx, dy = ((0, -1), (1, 0), (0, 1), (-1, 0))[direction]
                        cx = xx + step / 2
                        cy = yy + step / 2
                        d.rule(
                            cx,
                            cy,
                            cx + dx * step * 0.27,
                            cy + dy * step * 0.27,
                            "#e4ead6",
                            1.8,
                        )
                        d.dot(cx, cy, 2, "#e4ead6")
            d.text(x + 26, y + 548, "Colour · local band for the first dye", 16, green)
            d.text(
                x + 26, y + 575, "Spoke · local position of the first socket", 16, muted
            )
        else:
            rows = entries("braid_map")
            d.label(x + 26, y + 38, "Material survey", green)
            d.text(x + 26, y + 84, "Where the floats travelled", 35, ink, SERIF)
            if rows:
                cells = [r["cells"] for r in rows]
                loads = [r["quarters"] for r in entries("braid_cargo")]
                width = 397
                step = width / len(cells)
                px = x + 101
                py = y + 111
                for j, line in enumerate(cells):
                    for i, v in enumerate(line):
                        n = int(v, 16)
                        a = n & 3
                        k = n >> 2
                        xx = px + i * step
                        yy = py + j * step
                        color = ("#344b45", "#456c5b", "#66967c", "#94c4a0")[a]
                        if len(loads) == len(cells):
                            grain = int(loads[j][2 * i : 2 * i + 2], 16)
                            color = (
                                "#2a4141",
                                "#334f47",
                                "#426b56",
                                "#599274",
                                "#76aa89",
                                "#a6ce95",
                                "#d1d79d",
                                "#f0d29a",
                            )[min(7, grain // 2)]
                        d.rect(xx, yy, step - 1, step - 1, color, 1)
                        if k:
                            d.dot(xx + step / 2, yy + step / 2, 1.25 + 1.55 * k, gold)
                d.text(
                    x + 25,
                    y + 552,
                    "Green · carbon per carrier" if loads else "Green · fraction moved",
                    16,
                    green,
                )
                d.text(
                    x + size - 24,
                    y + 552,
                    "Amber · distant carriers",
                    16,
                    gold,
                    anchor="end",
                )
    roster = b.get("card", {}).get("roster", {})
    representatives = [
        item for item in roster.get("items", []) if "body_plan" in item["q"]
    ][:3]
    pupil = (entries("field_portrait") or [{}])[-1]
    if pupil:
        shape = {k: int(pupil[k]) for k in ("body_plan", "legs", "eyes", "pores")}
        d.label(rx, 335, "The individual under observation")
        d.rect(rx, 350, rw, 146, "#dfe7d9", 7)
        atlas_anatomy(d, rx + 94, 424, shape, atlas_identity(shape), 23)
        d.text(
            rx + 189, 389, f"Resident {int(pupil['field_portrait']):,}", 26, INK, SERIF
        )
        d.text(
            rx + 189,
            422,
            f"{int(pupil['cells']):,} spiking cells   ·   {int(pupil['modules'])} encoded modules",
            17,
            GREEN,
        )
        d.text(
            rx + 189,
            454,
            f"{shape['body_plan'].bit_count()} segments   ·   {shape['eyes'].bit_count()} eye sites   ·   {shape['pores'].bit_count()} feeding pores",
            16,
            MUTED,
        )
        d.text(
            rx + 189,
            479,
            f"Generation {int(pupil['generation']):,}   /   born at tick {int(pupil['born']):,}",
            14,
            MUTED,
            MONO,
        )
    elif representatives:
        d.label(rx, 335, "Common body plans in the source population")
        width = rw / len(representatives)
        for i, item in enumerate(representatives):
            px = rx + i * width
            cx = px + width / 2
            q = item["q"]
            d.rect(px + 2, 350, width - 9, 146, "#dfe7d9", 7)
            atlas_anatomy(d, cx, 394, q, item.get("id") or atlas_identity(q), 16)
            d.text(
                cx,
                451,
                f"{q.get('members', 1):,} residents",
                17,
                INK,
                SANS,
                600,
                "middle",
            )
            d.text(
                cx,
                478,
                (
                    atlas_range(q, "min_neurons", "max_neurons")
                    if "min_neurons" in q
                    else str(q.get("neurons", "—"))
                )
                + " spiking cells",
                14,
                MUTED,
                MONO,
                anchor="middle",
            )
    else:
        work = entries("braid_talk")
        recall = entries("braid_recall")
        d.label(rx, 335, "Individual exchanges")
        details = [
            ("Heard witness replies", sum(int(r["received"]) for r in recall)),
            ("Questions answered", sum(int(r["answered"]) for r in recall)),
            ("Public demonstrations", sum(int(r["consultations"]) for r in work)),
        ]
        for j, (label, value) in enumerate(details):
            yy = 377 + 49 * j
            d.text(rx, yy, label, 19, INK)
            d.text(rx + rw, yy, human(value), 25, GREEN, MONO, 600, "end")
            d.rule(rx, yy + 14, rx + rw, yy + 14)
