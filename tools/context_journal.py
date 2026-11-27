"""Spatiotemporal evidence and measured outcomes for continuous material learning."""


def context_observation(entries, result):
    def last(key):
        r = entries(key)
        return {k: int(v) for k, v in r[-1].items() if str(v).isdigit()} if r else {}

    compound=compound_observation(entries,result)
    if compound:return compound
    relay = last("material_relay")
    reunion = last("material_correspondence")
    messages = entries("material_message")
    chemistry = last("catalytic_field")
    window = last("catalytic_window")
    proof = last("catalytic_contacts")
    sealed = entries("sealed_cycle")
    if relay.get("pairs") or reunion:
        cohort = bool(relay)
        q = relay or reunion
        correct = q["after"] if cohort else q["correct"]
        record = {
            "cases": q["cases"],
            "correct": correct,
            "erased": q["erased"],
            "inverse": q["inverse"],
            "left": q["left_convention"],
            "right": q["right_convention"],
        }
        subject = (
            (
                f"One experienced {'sender' if q.get('tutor')==1 else 'receiver'}  /  {q['pairs']:,} juveniles"
                if q.get("tutor")
                else f"{q['pairs']:,} living pairs"
            )
            if cohort
            else f"Sender {q['material_correspondence']:,}  /  receiver {q['receiver']:,}"
        )
        return result(
            "Two animals share a chemical sign",
            subject,
            "Food located after a delayed mark",
            100 * correct / q["cases"],
            "%",
            [
                ["Correct choices", correct],
                ["Mark erased", q["erased"]],
                ["Opposite mark", q["inverse"]],
            ],
            [
                f"The receiving animal saw the result of its own pressure test after the sender left the junction.",
                f"It chose the stocked feeder in {correct:,} of {q['cases']:,} trials, compared with {q['erased']:,} after erasing the mark.",
                f"The same stored receivers chose it in {q['inverse']:,} trials after reversing the sender’s physical action.",
            ],
            [
                f"{q['left_convention']:,} balanced pairs of trials resolved both cues with a left-feeder mark and {q['right_convention']:,} with a right-feeder mark.",
                f"Each animal’s appetitive contacts occupy {q.get('contact_bytes', 16)} byte{'s' if q.get('contact_bytes', 16)!=1 else ''}.",
            ],
            "Effect of the physical mark",
            [],
            ("chemical_relay", record),
        )
    if messages:
        visits = [{k: int(v) for k, v in r.items()} for r in messages]
        good = sum(v["cue"] == v["receiver_choice"] for v in visits)
        return result(
            "A message in one trapped dye pair",
            "Embodied encounter  /  two independent spiking animals",
            "Correct feeder choices",
            good,
            " choices",
            [
                ["Messages", len(visits)],
                ["Correct choices", good],
                ["Portions delivered", sum(v["portions"] for v in visits)],
            ],
            [
                f"The sender alone saw which feeder was stocked on each of {len(visits):,} trials.",
                "The receiver read the same conserved dye pair with its own pressure pulse.",
                f"It selected the correct feeder {good:,} times, leaving {visits[-1]['remaining']} portions in the rack.",
            ],
            ["Movements, sensor readings and meals were recorded after each exchange."],
            "The two private cue states",
            [],
            ("chemical_messages", visits[:6]),
        )
    foragers = last("chemical_foraging")
    arrivals = last("chemical_receptions")
    if arrivals and arrivals.get("encounters"):
        q = arrivals
        v = last("chemical_neighbour")
        return result(
            "A pump reaches a different animal",
            f"Free watershed  /  tick {q['chemical_receptions']:,}",
            "Changed receiver movements",
            q["changed_move"],
            " movements",
            [
                ["Physical arrivals", q["encounters"]],
                ["Changed movements", q["changed_move"]],
                ["Learned responses", q["learned_response"]],
            ],
            [
                f"The camera followed {q['encounters']:,} pump-induced waves into a different animal’s active receptor.",
                f"With the arriving pulse removed and the receiving bed held fixed, {q['changed_move']:,} receivers would have changed their intended movements.",
                f"For {q['learned_response']:,} of those changes the same soma with its appetitive contacts cleared would have chosen the same movement for either pressure input.",
            ],
            [
                f"The immediate changes enabled {q['extra_food']:,} portions and cost {q['forgone_food']:,}.",
                f"Both ends depended on learned contacts in {q['learned_both']:,} encounters.",
            ],
            "An observed pressure path",
            [],
            ("chemical_neighbour", v),
        )
    if foragers and foragers["writer_visits"] + foragers["receiver_visits"]:
        q = foragers
        return result(
            "Learning where to pump and feed",
            f"Free watershed   /   tick {q['chemical_foraging']:,}",
            "Pulses changed by a paid pump",
            q["changed_pulses"],
            " pulses",
            [
                ["Paid pumps", q["paid_pumps"]],
                ["Pressure changes", q["changed_pulses"]],
                ["Net actions", q["net_choices"]],
            ],
            [
                f"Living creatures opened the pumping and receiving ports on {q['writer_visits']+q['receiver_visits']:,} occasions.",
                f"They spent energy on {q['paid_pumps']:,} pumps, of which {q['changed_pulses']:,} changed the physical pressure response.",
                f"Their own feeding outcomes altered {q['contact_edits']:,} synaptic signs, with {q['living_contacts']:,} animals retaining signs at the census.",
            ],
            [
                f"The optional foraging organ occupies one byte per resident slot.",
                (
                    f"The immediate replay with empty appetitive contacts enabled {q.get('extra_meals',0):,} additional portions in the experienced receivers and missed {q.get('missed_meals',0):,}."
                    if q.get("compared")
                    else f"Receivers collected {q['receiver_food']:,} portions."
                ),
            ],
            "Free physical actions",
            [("Effective pumps", q["changed_pulses"], q["paid_pumps"])],
        )
    if window or chemistry:
        q = chemistry
        if q:
            return result(
                "A coating changes the next pulse",
                f"Living watershed  /  tick {q['catalytic_field']:,}",
                "Pulses admitted by the coating",
                q["enabled_pulses"],
                " pulses",
                [
                    ["Surface-enabled pulses", q["enabled_pulses"]],
                    ["Surface-blocked pulses", q["prevented_pulses"]],
                    ["Changed handling effects", q["enabled_grips"]],
                ],
                [
                    f"The pressure field advanced through {q['updates']:,} local updates.",
                    f"Exchanging the two pigments changed the sill response {q['switched_sills']:,} times.",
                    (
                        f"The camera witnessed {window['witnessed_closures']:,} successive passes and closures at unchanged inputs."
                        if window
                        else f"At {q['self_closing']:,} sites a pulse left a coating that would block an immediate repeat of the same input."
                    ),
                ],
                [
                    f"At the same net and food stock the surface chemistry enabled {q['enabled_food']:,} food portions and cost {q['forgone_food']:,} others.",
                    "The physical update reuses the existing pigment planes.",
                ],
                "Physical response",
                [],
                ("chemical_counts", q),
            )
    if proof:
        return result(
            "A junction can close itself",
            "Exhaustive CUDA material assay",
            "Self-closing cases",
            proof["self_closing"],
            " cases",
            [
                ["Cases", proof["catalytic_contacts"]],
                ["Changed law", proof["state_changes"]],
                ["Self-closing", proof["self_closing"]],
            ],
            [
                f"The GPU operated each of {proof['catalytic_contacts']:,} material contacts twice with identical input.",
                f"The first pulse changed the effective law in {proof['state_changes']:,} cases.",
                f"Of these, {proof['self_closing']:,} passed the first pulse and blocked the second.",
            ],
            [],
            "Consecutive pulses",
            [("Self-closing", proof["self_closing"], proof["catalytic_contacts"])],
        )
    if sealed:
        n = len(sealed)
        flows = sum(int(v["flow"]) for v in sealed)
        return result(
            "A valve stores its last operation",
            "Sealed physical junction",
            "Pulses passed",
            flows,
            " pulses",
            [["Pours", n], ["Admitted", flows], ["Held", n - flows]],
            [
                f"Two conserved pigments circulated through {n} physical pours.",
                f"The junction passed {flows} pulses and stopped {n-flows}.",
                f"The external rack held {sealed[-1]['remaining']} unused portions after the last stroke.",
            ],
            [],
            "Junction responses",
            [("Admitted pulses", flows, n)],
        )
    population = last("context_population")
    if population.get("studied"):
        q = population
        variants = [{k: int(v) for k, v in r.items()} for r in entries("context_rule")]
        arm = {r["context_rule"]: r for r in variants}
        info = {
            "natural": q["full"],
            "alternate": q["paired_full"],
            "n": q["studied"],
            "root": q["initial_rock"],
            "transfer": q["next_rock"],
        }
        for rule in (10, 14):
            for key in (
                "bodies",
                "native_complete",
                "paired_complete",
                "native_later",
                "paired_later",
                "later_trials",
            ):
                info[f"r{rule}_{key}"] = arm.get(rule, {}).get(key, 0)
        return result(
            "Which learners adapt to a different mineral?",
            f"{'Afferent-specific' if q.get('regional') else 'Shared'} update rule   /   tick {q['context_population']:,}   /   {q['streams']} lives each",
            "Residents mastering every episode",
            q["full"],
            " animals",
            [
                ["Enrolled", q["studied"]],
                ["Native rule", q["full"]],
                ["Retention bit flipped", q["paired_full"]],
            ],
            [
                f"{q['studied']:,} animals faced {q['episodes']} episodes in each of {q['streams']} continuous lives.",
                f"Using their native rule, {q['full']:,} residents learned every material response in every life.",
                f"Changing one retention bit under the identical sensory and motor history gave {q['paired_full']:,} complete learners.",
            ],
            [
                f"The highest-scoring native resident was born at tick {q['born']:,} with generation {q['generation']:,}.",
                f"The paired conditions shared {q['heat']:,} units of experimental expenditure.",
            ],
            "Two inherited local rules",
            [],
            ("context_genotypes", info),
        )
    series = last("context_series")
    life = last("context_life")
    choices = last("context_choices")
    scene = last("context_scene")
    natural = last("context_field")
    if series:
        q = series
        changed = q["switches"]
        right = q["resolved_switch"] if changed else q["stable_correct"]
        total = changed or q["stable"]
        treatment = (
            "Self-chosen rechecks",
            "Pooled contacts",
            "Examiner-cleared contacts",
            "Unscheduled exploration",
            "Matched motion, clamped receipts",
            "Matched motion, exchanged feedback",
            "Matched motion, altered retention bit",
        )[q["arm"]]
        if q.get("varied_questions") and q.get("transfer_rock", 32) < 32:
            phases = entries("context_phase")
            if len(phases) == 2:
                late = {k: int(v) for k, v in phases[-1].items()}
                forms = [
                    {k: int(v) for k, v in r.items()}
                    for r in entries("context_question")
                ]
                return result(
                    "Speaking after the mineral changes",
                    f"{treatment}   /   resident {q['context_series']:,}   /   mineral family {late['rock']}",
                    "Correct forecasts after transfer",
                    100 * late["exact"] / max(1, late["episodes"]),
                    "%",
                    [
                        ["Later episodes", late["episodes"]],
                        ["Entire law correct", late["exact"]],
                        ["Correct answers", late["spoken"]],
                    ],
                    [
                        f"The resident lived through {q['streams']:,} independent histories with a change in the pressure thresholds midway.",
                        f"It recovered the entire response law in {late['exact']:,} of {late['episodes']:,} subsequent episodes.",
                        f"On those episodes it gave {late['spoken']:,} correct answers to {late['answerable']:,} answerable questions and made {late['unsupported']:,} unsupported claims.",
                    ],
                    [
                        f"The voiced forms were learned during separate acoustic tuition.",
                        f"Later repairs captured food in {late['food']:,} of {late['accessible']:,} physically solvable channels.",
                    ],
                    "Six kinds of spoken questions",
                    [],
                    ("context_questions", forms),
                )
        return result(
            "A material changed out of sight",
            f"{treatment}   /   resident {q['context_series']:,}",
            "Complete local forecasts",
            100 * right / max(1, total),
            "%",
            [
                ["Hidden changes", changed],
                ["Recovered after change", q["resolved_switch"]],
                ["Successful repairs", q["repaired"]],
            ],
            [
                f"The animal experienced {q['total']:,} material episodes across {q['streams']:,} continuous histories.",
                (
                    f"At {q['resolved_switch']:,} of {changed:,} changes it correctly forecast all 64 local outcomes after exploration."
                    if changed
                    else f"It forecast all 64 local outcomes correctly in {q['stable_correct']:,} stable episodes."
                ),
                f"The resulting constructions worked in {q['repaired']:,} of {q['solvable']:,} physically solvable trials.",
            ],
            [
                f"There were {q['actions']:,} paid investigations and {q['heat']:,} energy units spent.",
                f"The resident voiced {q['worded']:,} correct responses across {q['answerable']:,} answerable prompts.",
            ],
            "One continuing history",
            [
                ("Before a switch", q["before_switch"], changed),
                ("After exploring", q["resolved_switch"], changed),
                ("Initial or unchanged", q["stable_correct"], q["stable"]),
            ],
            ("context_history", life) if life else None,
        )
    if choices:
        q = choices
        return result(
            "When recent evidence changes a reach",
            f"Living watershed   /   tick {q['context_choices']:,}",
            "Different handling decisions",
            q["changed_hand"],
            " decisions",
            [
                ["Choices compared", q["compared"]],
                ["Extra portions", q["extra"]],
                ["Foregone portions", q["lost"]],
            ],
            [
                f"Each of {q['compared']:,} awake decisions was replayed from the same terrain, body and motor variation with the older pooled readout.",
                f"The expressed context circuit chose a different handle on {q['changed_hand']:,} occasions and a different net action on {q['changed_net']:,}.",
                f"It enabled {q['extra']:,} additional food portions and missed {q['lost']:,} others in these immediate physical comparisons.",
            ],
            [
                "Subsequent behaviour and reproduction fall outside the one-step comparison.",
                f"Actual actions cost {q['actual_heat']:,} energy units and the paired choices would cost {q['pooled_heat']:,}.",
            ],
            "An observed local decision",
            [
                ("Changed handle", q["changed_hand"], q["compared"]),
                ("Changed net", q["changed_net"], q["compared"]),
            ],
            ("context_reach", scene) if scene else None,
        )
    if natural and natural.get("ambiguous_looks"):
        q = natural
        return result(
            "A creature remembers its experiment",
            f"Living watershed   /   tick {q['context_field']:,}",
            "Paid diagnostic updates",
            q["diagnostic_updates"],
            " contacts",
            [
                ["Ambiguous looks", q["ambiguous_looks"]],
                ["Physical checks", q["physical_checks"]],
                ["Resolved choices", q["resolved"]],
            ],
            [
                f"Residents met {q['ambiguous_looks']:,} observations consistent with conflicting local models.",
                f"Their own handling produced {q['diagnostic_updates']:,} admitted diagnostic receipts.",
                f"At the final census {q['with_anchor']:,} living animals retained a signed result from an informative experiment.",
            ],
            [
                f"Each equipped slot reserved {q['bytes']} additional bytes.",
                f"{q['residents']:,} residents retained passive observations.",
            ],
            "Actions and evidence",
            [
                (
                    "Admitted physical checks",
                    q["diagnostic_updates"],
                    q["physical_checks"],
                )
            ],
        )
    turns = entries("capillary_turn")
    probes = [r for r in turns if r["capillary_turn"] == "19"]
    if probes:
        memory = entries("material_memory")
        regular = sum(int(r.get("rechecks", 0)) for r in memory)
        return result(
            "Experiments at one junction",
            "Embodied bench   /   self-chosen handle positions",
            "Physical probes",
            len(probes),
            " probes",
            [
                ["Distinct positions", len({r["handles"] for r in probes})],
                ["Portions eaten", int(turns[-1]["eaten"])],
                ["Unpoured stock", int(turns[-1]["drops"])],
            ],
            [
                f"The operator selected and physically applied {len(probes):,} probes at a single junction.",
                f"The rig occupied {len({r['handles'] for r in probes}):,} distinct handle positions at release.",
                f"The paid probes cost {sum(int(r['heat']) + int(r['pupil_heat']) for r in probes):,} energy units across the two participants.",
            ],
            [
                f"Observation-driven rechecks were reported on {regular:,} turns.",
                "Each release drew one portion from the physical stock.",
            ],
            "Observed handling",
            [("Pulses passed", sum(int(r["target"]) for r in probes), len(probes))],
        )
    return None


def context_illustration(d, kind, q, x, y, size, ink, muted, gold, green, grid):
    if kind == "chemical_neighbour":
        for i, (label, actor, site) in enumerate(
            (
                ("Pumping animal", q["writer"], q["source"]),
                ("Receiving animal", q["reader"], q["destination"]),
            )
        ):
            xx = x + 32 + i * (size * 0.49)
            d.text(xx, y + 169, label, 21, muted, SANS)
            d.text(xx, y + 232, f"#{actor:,}", 29, ink, SERIF)
            d.text(xx, y + 276, f"Site {site:,}", 16, muted, MONO)
        d.text(
            x + 32,
            y + 365,
            f"With pulse     {('wait','reach')[q['movement']]}",
            22,
            ink,
            MONO,
        )
        d.text(
            x + 32,
            y + 418,
            f"Without pulse  {('wait','reach')[q['without_pump']]}",
            22,
            ink,
            MONO,
        )
        d.text(
            x + 32, y + 513, f"Elapsed habitat ticks   {q['delay']}", 20, muted, MONO
        )
        return
    if kind == "chemical_relay":
        for j, (label, key, color) in enumerate(
            (
                ("Observed mark", "correct", "#7dbca7"),
                ("Mark erased", "erased", "#b49dc9"),
                ("Action inverted", "inverse", "#c99f70"),
            )
        ):
            yy = y + 170 + j * 112
            d.text(x + 31, yy, label, 21, ink, SANS)
            value = q[key] / q["cases"]
            d.text(x + size - 32, yy, f"{100*value:.1f}%", 22, ink, MONO, anchor="end")
            d.rect(x + 32, yy + 24, size - 65, 18, "#314a45", 4)
            d.rect(x + 32, yy + 24, (size - 65) * value, 18, color, 4)
        d.text(
            x + 31, y + 565, f"{q['cases']:,} balanced feeder choices", 18, muted, MONO
        )
        return
    if kind == "chemical_messages":
        for i, r in enumerate(q):
            yy = y + 160 + i * 61
            side = ("left", "right")[r["cue"]]
            result = ("left", "right")[r["receiver_choice"]]
            d.text(x + 32, yy, side + " feeder", 18, ink, SANS)
            d.text(x + size * 0.35, yy, "pump" if r["mark"] else "wait", 18, gold, MONO)
            d.text(
                x + size - 30,
                yy,
                result + " chosen",
                18,
                green if side == result else muted,
                SANS,
                anchor="end",
            )
        return
    if kind == "chemical_counts":
        for i, (label, key) in enumerate(
            (
                ("Different local material", "changed_material"),
                ("Newly open pulses", "enabled_pulses"),
                ("Newly closed pulses", "prevented_pulses"),
                ("One-pulse closures", "self_closing"),
            )
        ):
            yy = y + 166 + i * 86
            d.text(x + 32, yy, label, 20, muted, SANS)
            d.text(x + size - 32, yy + 31, f"{q[key]:,}", 27, ink, MONO, anchor="end")
        return
    if kind == "context_questions":
        labels = (
            "Which cause?",
            "Could it act alone?",
            "Remove a part",
            "Recall the past",
            "One part alone",
            "Which two parts?",
        )
        for i, (label, r) in enumerate(zip(labels, q)):
            yy = y + 163 + i * 62
            d.text(x + 36, yy, label, 17, ink, SANS)
            left = x + size * 0.44
            width = size * 0.28
            d.rect(left, yy - 12, width, 16, "#314a45", 3)
            d.rect(
                left,
                yy - 12,
                width * r["exact"] / max(1, r["answerable"]),
                16,
                "#74baa8",
                3,
            )
            d.text(
                x + size - 31,
                yy,
                f"{r['exact']:,} / {r['answerable']:,}",
                16,
                ink,
                MONO,
                anchor="end",
            )
        d.text(
            x + 36,
            y + 564,
            "Exact words in answerable encounters, across both minerals",
            15,
            muted,
            MONO,
        )
        return
    if kind == "context_genotypes":
        top = y + 164
        left = x + 33
        span = size - 86
        for i, (rule, name) in enumerate(
            ((10, "Replace the observed sign"), (14, "Retain either observed sign"))
        ):
            yy = top + i * 188
            n = q[f"r{rule}_bodies"]
            d.text(left, yy, name, 25, ink, SERIF)
            d.text(
                left,
                yy + 33,
                f"{n:,} enrolled   ·   four-bit rule {rule:04b}",
                16,
                muted,
                MONO,
            )
            for j, (label, key, color) in enumerate(
                (
                    ("Inherited", "native_complete", "#74baa8"),
                    ("Flipped", "paired_complete", "#bd97c7"),
                )
            ):
                value = q[f"r{rule}_{key}"]
                y0 = yy + 60 + j * 42
                d.text(left, y0 + 13, label, 17, muted, SANS)
                start = left + 104
                bar = span - 219
                d.rect(start, y0, bar, 17, "#314a45", 3)
                d.rect(start, y0, bar * value / max(1, n), 17, color, 3)
                d.text(
                    left + span,
                    y0 + 15,
                    f"{value:,} / {n:,}",
                    16,
                    ink,
                    MONO,
                    anchor="end",
                )
        d.text(
            left,
            min(y + 590, top + 397),
            "All episodes correct in every history",
            18,
            muted,
            MONO,
        )
        return
    if kind == "context_history":
        n = q["episodes"]
        per = min(16, n)
        pitch = min((size - 92) / per, 39)
        ox = x + 38
        yy = y + 157
        sets = (
            ("Material", q["rocks"], "#b28ac2"),
            ("Correct before", q["before"], "#94a7b4"),
            ("Correct after", q["resolved"], "#7dc4ac"),
            ("Diagnostic receipt", q["signed"], "#e0b979"),
        )
        bands = (n + per - 1) // per
        row_step = min(84, 410 / (4 * bands))
        for label, bits, color in sets:
            d.text(ox, yy, label, 18, muted, MONO)
            yy += 20
            for episode in range(n):
                xx = ox + pitch * (episode % per)
                py = yy + row_step * 0.55 * (episode // per)
                fill = (
                    color
                    if bits & (1 << episode)
                    else "#354d4b" if label != "Material" else "#9a7554"
                )
                d.rect(xx, py, pitch - 3, min(25, row_step * 0.46), fill, 3)
            yy += row_step * 0.55 * bands + 17
        d.text(
            ox,
            min(y + 591, yy + 15),
            f"Earliest episode first   ·   {n} episodes",
            17,
            muted,
            MONO,
        )
    else:
        parts = ("Wait", "East fibre", "South clay", "East clay", "South fibre")
        d.text(
            x + 32,
            y + 159,
            f"Resident #{q['slot']:,}   ·   tick {q['context_scene']:,}",
            21,
            muted,
            MONO,
        )
        width = (size - 76) / 2
        for j, (label, hand, food, heat) in enumerate(
            (
                (
                    "Pooled evidence",
                    q["pooled_hand"],
                    q["pooled_food"],
                    q["other_cost"],
                ),
                ("Recent experiment", q["actual_hand"], q["actual_food"], q["cost"]),
            )
        ):
            xx = x + 28 + j * (width + 18)
            d.rect(xx, y + 199, width, 215, "#243d3c", 8)
            d.text(xx + 14, y + 241, label, 21, ink, SANS, 600)
            d.text(xx + 14, y + 288, parts[hand], 23, ink, SERIF)
            d.text(
                xx + 14,
                y + 348,
                f"{food} portions",
                25,
                gold if food else muted,
                SANS,
                600,
            )
            d.text(xx + 14, y + 388, f"{heat} energy", 16, muted, MONO)
        d.text(x + 32, y + 470, f"Initial edible stock   {q['stock']}", 21, muted, MONO)
