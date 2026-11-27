"""Field observations of sensory acquisition, tracer motion and spoken inquiry."""


def description_observation(last, result, entries=None):
    def numbers(key):
        return {k: int(v) for k, v in last(key).items() if k != "cuda_ms"}

    if last("acquired_voice"):
        history=entries("acquired_voice") if entries else [last("acquired_voice")]
        series=[{k:int(v) for k,v in row.items() if k!="total_cuda_ms"} for row in history]
        q=series[-1]; n=q["images"]
        return result("A caller acquires a local dialect", f"Transmission {q['acquired_voice']}   /   {q['chains']:,} paired rearing histories",
            "Different calls from the same inherited voice",100*q["pupils_differ"]/max(1,n),"%",
            [["Different calls",q["pupils_differ"]],["Test pictures",n],["Retuned syllables",q["altered_syllables"]]],
            [f"Two pupils with identical inherited vocal organs heard different founders on the same physical scenes.",
             f"After {q['acquired_voice']} transmissions the learners spoke differently on {q['pupils_differ']:,} of {n:,} shared pictures.",
             f"Listeners familiar with each founder's speech caught {q['own_catch']:,} of {q['portions']:,} portions."],
            [f"Exchanging the receiving ears yielded {q['other_catch']:,} portions.",
             f"The final exposure delivered {q['sounds']:,} syllables and changed {q['edits']:,} contacts."],
            "A voice through successive learners", [(f"Transmission {a['acquired_voice']}",a["pupils_differ"],a["images"]) for a in series[-5:]])
    natural = numbers("field_ink")
    if natural.get("imitating") and natural.get("learned_calls"):
        q=natural
        return result("Learning to voice a field encounter", f"Free-living population   /   tick {q['field_ink']:,}",
            "Calls altered by the caller's auditory tissue",q["learned_calls"]," calls",
            [["Calls paid for",q["calls"]],["Altered pitches",q["learned_syllables"]],["Unseen food caught",q["blind_food"]]],
            [f"The population paid for {q['syllables']:,} syllables after observing actual material releases.",
             f"Learned auditory contacts altered {q['learned_syllables']:,} of those pitches.",
             f"Receivers collected {q['blind_food']:,} food units during incomplete visual exposure."],
            [f"Callers produced {q['learned_calls']:,} calls that differed from their own inherited motor prediction.",
             f"The participating population registered {q['association_edits']:,} auditory contact changes."],
            "The audible phenotype",[("Changed whole calls",q["learned_calls"],q["calls"]),("Changed syllables",q["learned_syllables"],q["syllables"])])
    refs = numbers("narrative_references")
    exam = numbers("narrative_exam")
    if refs and refs["layouts"]:
        q = refs
        n, exact = q["answerable"], q["exact"]
        return result(
            "Questions about a remembered event", f"Resident {q['narrative_references']:,}   /   {q['layouts']:,} unfamiliar histories",
            "Answerable questions answered exactly", 100 * exact / max(1, n), "%",
            [["Correct recollections", exact], ["Located in time", q["located"]], ["Incompatible reports", q["wrong"]]],
            [f"The resident heard {q['examples']:,} demonstrations using two pigments as question words.",
             f"Given two other pigment names, it answered {exact:,} of {n:,} answerable questions correctly.",
             f"It offered a description on {q['invented']:,} of {q['absent']:,} questions whose target lay outside the observed history."],
            [f"Changing the named pigment required a different answer in {q['contrasts']:,} pairs on the same history.",
             f"Both answers were correct in {q['both']:,} of those pairs."],
            "Locating the named encounter", [("Exact answer", exact, n), ("Exact episode", q["located"], n), ("Both names", q["both"], q["contrasts"])],
            ("pigment_history", numbers("narrative_reference_scene")) if last("narrative_reference_scene") else None)
    if exam:
        q = exam
        n, good = q["cases"], q["exact"]
        return result(
            "Describing a sequence of events", f"Resident {q['narrative_exam']:,}   /   {q['length']}-event histories",
            "Exact spoken descriptions", 100 * good / max(1, n), "%",
            [["Spoken words", q["words"]], ["Entire histories", q["recovered_histories"]], ["Food intercepted", q["upstream"] + q["downstream"]]],
            [f"The test presented {q['histories']:,} histories of {q['length']} material passages.",
             f"The creature produced {good:,} exact answers across {n:,} requests.",
             f"A listener reconstructed {q['recovered_histories']:,} complete event histories from speech."],
            [f"All {q['portions']:,} portions were accounted for after the nets opened.",
             f"The streams changed the passing pigment {q['changes_of_pigment']:,} times."],
            "A reported watercourse", [("Complete reports", q["complete_descriptions"], q["histories"]), ("Heard histories", q["recovered_histories"], q["histories"])],
            ("pigment_history", numbers("narrative_scene")) if last("narrative_scene") else None)
    q = numbers("narrative_cohort")
    if q and q["enrolled"]:
        return result(
            "Learning across a field-born population", f"Watershed tick {q['narrative_cohort']:,}",
            "Residents passing every test", q["perfect"], " residents",
            [["Enrolled", q["enrolled"]], ["Tests per resident", q["tests"]], ["Selected brain cells", q["cells"]]],
            [f"Each of {q['enrolled']:,} living residents met the same tutor.",
             f"{q['perfect']:,} learned all {q['tests']} held-out answers.",
             f"The selected resident belonged to generation {q['generation']} and retained {q['events']} field encounters."],
            [f"The speaker occupied slot {q['slot']:,} and was born at tick {q['born']:,}.",
             f"The population supplied {q['total_correct']:,} correct answers."], "Origins of the tested population", [])
    q = numbers("field_ink")
    if q and q["releases"]:
        return result(
            "Pigments and calls in the watershed", f"Free-living population   /   tick {q['field_ink']:,}",
            "Personal views committed to memory", q["seen"], " observations",
            [["Pigments exchanged", q["stained"]], ["Calls initiated", q["calls"]], (["Unseen food caught", q["blind_food"]] if "blind_food" in q else ["Paid syllables", q["syllables"]])],
            [f"The residents released {q['releases']:,} tracers through wet surfaces and fibre.",
             f"Their eyes registered {q['seen']:,} complete views and {q['partial']:,} incomplete ones.",
             (f"The residents paid for {q['syllables']:,} syllables and collected {q['blind_food']:,} food units while unable to see the full transition." if "blind_food" in q else f"Listening to the calls changed {q['spike_edits']:,} somatic spikes.")],
            [f"{q['living_witnesses']:,} living witnesses retained at least one personal encounter at the final census.",
             (f"Their vocal exposures changed {q['association_edits']:,} auditory contacts." if "association_edits" in q else f"Their histories occupied {q['resident_bytes']} bytes per resident.")],
            "Working the water", [("Changed pigment", q["stained"], q["releases"]), ("Complete view", q["seen"], q["seen"] + q["partial"])])
    if entries and last("narrative_turn"):
        questions=[q for q in entries("narrative_turn") if q["narrative_turn"]=="5"]
        other=[q for q in entries("braid_talk") if q["braid_talk"]=="6"]
        if questions and other:
            a=sum(int(q["words"])>0 for q in questions)
            b=sum(int(q["said"])>0 for q in other)
            n=len(questions)+len(other)
            return result("An open interview at the watershed", "Field resident   /   ordinary spoken questions",
                "Questions receiving an audible response", a+b, " answers",
                [["Questions",n],["Answered from event memory",a],["Silent",n-a-b]],
                [f"The resident received {n} questions through the ordinary conversation interface.",
                 f"It gave {a} spoken responses from its learned event memory.",
                 f"Of the {len(other)} questions routed to its other speech circuitry, {b} elicited words."],
                [f"The event circuit spent {sum(int(q['heat']) for q in questions)} energy units answering.",
                 f"The resident retained {int(questions[-1]['events'])} personal encounters at the end."],
                "Audible responses", [("Event questions",a,len(questions)),("Other questions",b,len(other))])
    turn = numbers("narrative_turn")
    if turn:
        return result("A witness at the brook", "Embodied conversation   /   recorded private sight",
            "Recalled encounters", turn["events"], " passages",
            [["Known prompts", turn["prompts"]], ["Portions eaten", turn["eaten"]], ["Grains in the bed", turn["grain_stock"]]],
            [f"The witness ended the exchange with {turn['events']} encounters in its private history.",
             f"It retained {turn['prompts']} acquired descriptive prompts.",
             f"The apparatus accounted for {turn['held'] + turn['settled'] + turn['eaten']} released food portions."],
            [f"There were {turn['grain_stock']} grains in the four places.", f"The last request elicited {turn['words']} words."],
            "Personal visual record", [])
    return None


def pigment_illustration(d, q, x, y, size, ink, muted, gold, green, grid):
    colors = ("#d59445", "#629874", "#9369a4", "#3e9895")
    names = ("Amber", "Jade", "Plum", "Teal")
    directions = ("N", "E", "S", "W")
    trail = q["physical"]
    length = trail >> 56
    labels = (("Observed passage", 31), ("Entering", size * .49), ("Leaving", size * .73))
    for label, dx in labels:
        d.text(x + dx, y + 151, label, 16, muted, MONO)
    for i in range(length):
        event = (trail >> (8 * (length - i - 1))) & 255
        a, b, u, v = event & 3, (event >> 2) & 3, (event >> 4) & 3, event >> 6
        yy = y + 198 + 47 * i
        if q.get("target") == i:
            d.rect(x + 22, yy - 25, size - 44, 37, "#34544c")
        d.text(x + 32, yy, f"{i + 1:02d}    {directions[a]}  →  {directions[b]}", 23, ink, MONO)
        for pigment, dx in ((u, size * .49), (v, size * .73)):
            d.dot(x + dx + 6, yy - 7, 7, colors[pigment])
            d.text(x + dx + 20, yy, names[pigment], 19, ink)
    d.text(x + 32, y + 582, "Earliest observation first", 18, muted, MONO)


def inquiry_observation(last, result):
    meeting = last("audience_exchange")
    if meeting:
        q = {k: int(v) for k, v in meeting.items() if k != "cuda_ms"}
        before, after, n = q["caught_before"], q["caught_after"], q["portions"]
        method = "Tracer demonstration" if q.get("tracer") else "Spoken testimony"
        arm = (
            "visible partner",
            "exchanged partner",
            "screened view",
            "closed channel",
            "reversed signal",
            "matching coats",
            "closed meeting",
            "private view",
        )[q["arm"]]
        brood = last("audience_brood")
        scene = last("audience_scene")
        practice = last("audience_practice")
        if practice:
            method = ("Paired glints", "Shifted word pairs", "Fixed auditory contacts")[
                int(practice["pairing"])
            ]
        return result(
            "Learning a guide’s speech" if practice else "What the other creature saw",
            f"{method}   /   {arm}   /   guide {q['audience_exchange']:,}",
            "Food intercepted after contact",
            100 * after / max(1, n),
            "%",
            [
                ["Before contact", before],
                ["After contact", after],
                ["Available portions", n],
            ],
            [
                (
                    f"The pupil heard {int(practice['paired']):,} word pairs alongside moving tracers before the held-out layouts."
                    if practice
                    else f"Two copies of the pupil received separate viewing histories in {q['layouts']:,} shared layouts."
                ),
                f"The guide provided evidence about an unseen inlet in {q['unseen_source']:,} of {q['encounters']:,} encounters.",
                f"The listeners intercepted {after:,} portions after contact, compared with {before:,} before it.",
            ],
            [
                f"Both listeners received new evidence in {q['both_unseen']:,} paired layouts and the guide changed its choice in {q['different_offers']:,}.",
                (
                    f"The nursery funded {int(brood['births']):,} births across {int(brood['audience_brood']):,} generations."
                    if brood
                    else f"The guide spent {q.get('offer_heat', q['vocal_heat']):,} units on its offers and the receiver registered {q['edits']:,} contact changes."
                ),
            ],
            "Two views of the same streams",
            [
                ("Before contact", before, n),
                ("After contact", after, n),
                ("Both addressed", q["both_unseen"], q["layouts"]),
            ],
            ("addressed_offer", {"sample": scene, "summary": q}),
        )
    contact = last("field_audience")
    if contact and int(contact["opportunities"]):
        q = {k: int(v) for k, v in contact.items()}
        return result(
            "A neighbor at the water",
            f"Free-living watershed   /   tick {q['field_audience']:,}",
            "Reaches sensitive to the visible outline",
            100 * q["face_sensitive"] / q["opportunities"],
            "%",
            [
                ["Neighbor in view", q["opportunities"]],
                ["Public reaches", q["public_reaches"]],
                ["Joint views", q["shared_views"]],
            ],
            [
                f"The population shared {q['shared_views']:,} views of moving water.",
                f"Visible apertures led to {q['contact_edits']:,} changes in somatic contacts.",
                f"Reading a different outline's contacts changed the chosen hand or its initiation on {q['face_sensitive']:,} occasions.",
            ],
            [
                f"The residents made {q['public_reaches']:,} reaches in front of a sighted neighbor.",
                "Four visible outline classes shared sixteen existing somatic contacts per resident.",
            ],
            "Public movements at the bed",
            [
                ("Available reaches", q["opportunities"], q["opportunities"]),
                ("Initiated", q["public_reaches"], q["opportunities"]),
                ("Changed by outline", q["face_sensitive"], q["opportunities"]),
            ],
            ("public_outlines", q),
        )
    session = last("public_practice")
    if session:
        q = {k: int(v) for k, v in session.items()}
        return result(
            "Learning a familiar voice",
            "Two residents   /   witnessed speech",
            "Publicly paired demonstrations",
            q["paired"],
            "pairs",
            [
                ["Demonstrations", q["public_practice"]],
                ["Grains moved", q["moved"]],
                ["Contact edits", q["edits"]],
            ],
            [
                f"The guide and pupil completed {q['public_practice']} encounters.",
                f"The pupil heard {q['paired']} word pairs associated with publicly moving grains.",
                f"Its acoustic tissue changed {q['edits']} contacts.",
            ],
            [
                f"The guide spent {q['guide_heat']} energy units and the pupil spent {q['pupil_heat']}.",
                "The pupil can carry these auditory contacts into later encounters with the same voice.",
            ],
            "The pupil’s learned contacts",
            [],
            ("public_code", q),
        )
    offer = last("public_offer")
    if offer:
        q = {k: int(v) for k, v in offer.items()}
        delivered = last("audience_delivery") or {}
        eaten = int(delivered.get("pupil_ate", 0))
        return result(
            "A guide speaks to its pupil",
            "Two residents   /   a shared watercourse",
            "Food intercepted in conversation",
            eaten,
            "units",
            [
                ["Public words", q["words"]],
                ["Pupil contact edits", q["contact_edits"]],
                ["Pupil intake", eaten],
            ],
            [
                f"The guide selected inlet {q['selected']} with a listener present.",
                f"It emitted {q['words']} words and the listener registered {q['contact_edits']} somatic contact edits.",
                f"The pupil collected {eaten} units of food during the encounter.",
            ],
            [
                f"The guide used {q['guide_heat']} energy units for observation and communication.",
                f"The pupil used {q['pupil_heat']} units to watch and receive the offer.",
            ],
            "Observation and the offer",
            [],
            ("one_offer", q),
        )
    q = last("spoken_inquiry")
    if q:
        v = {k: int(x) for k, x in q.items()}
        n, before, after = v["courses"], v["correct_before"], v["correct_after"]
        hand = {
            1: "chosen inlet",
            2: "exchanged hands",
            3: "random hand",
            4: "matched visibility",
        }[v["coupling"]]
        context = (
            "reversed working question" if v.get("query_context") else "heard question"
        )
        return result(
            "Investigating a spoken route",
            f"Resident {v['spoken_inquiry']:,}   /   {hand}   /   {context}",
            "Correct after investigation",
            100 * after / max(1, n),
            "%",
            [
                ["Before investigation", before],
                ["After investigation", after],
                ["Test grains released", v["grain_releases"]],
            ],
            [
                f"The resident initially gave {before:,} correct forecasts in {n:,} spoken encounters.",
                f"After hearing the orders it released {v['grain_releases']:,} test grains and correctly named {after:,} future outlets.",
                f"It turned {v['from_silence']:,} initial silences into correct spoken answers.",
            ],
            [
                f"The gates stopped further investigation in {v['stopped']:,} encounters with time and energy still available.",
                f"The physical release remained covered throughout the prospective question and elicited {v['early_outcomes']:,} premature outcome reports.",
            ],
            "Before and after a question",
            [
                (label, a, b)
                for label, a, b in (
                    ("Correct before", before, n),
                    ("Correct after", after, n),
                    ("Previous errors repaired", v["improved"], n - before),
                    ("Both orderings", v["both_orders"], v["changed_order_pairs"]),
                )
            ],
            ("inquiry_signals", v),
        )
    q = last("gaze_panel")
    if q:
        v = {k: int(x) for k, x in q.items() if k != "gaze_panel"}
        n = v["courses"]
        work = v["heat"]
        offers = v["presented"]
        carbon = last("gaze_metabolism")
        speech = last("gaze_speech")
        arm = v.get("probes", 0)
        policy = (
            (
                "Ambient",
                "Chosen inlet",
                "Exchanged hands",
                "Random hand",
                "Visibility-matched hand",
            )[arm]
            if arm
            else (
                "Passive",
                "Inherited aperture",
                "Timing reassigned",
                "Open aperture",
                "Closed aperture",
                "Matched visibility",
                "Initial tissue",
            )[v["gaze"]]
        )
        if speech:
            ec = {k: int(x) for k, x in speech.items()}
            return result(
                "Choosing evidence before speaking",
                f"{policy}   /   resident {ec['gaze_speech']:,}",
                "Correct spoken forecasts",
                100 * ec["correct"] / max(1, ec["courses"]),
                "%",
                [
                    ["Spoken trials", ec["courses"]],
                    ["Paid acquisitions", ec["heat"]],
                    ["Correct forecasts", ec["correct"]],
                ],
                [
                    f"The resident had {ec['opportunities']:,} opportunities to acquire evidence.",
                    f"It paid for {ec['heat']:,} observations before answering {ec['courses']:,} spoken forecasts.",
                    f"Both members of {ec['both']:,} of {ec['opposed']:,} order-sensitive pairs were correct.",
                ],
                [
                    f"The purse held {ec['purse']} units per encounter.",
                    f"The measured sensory treatment was {policy.lower()}.",
                ],
                "The cost of a forecast",
                [
                    ("Paid observations", ec["heat"], ec["opportunities"]),
                    ("Correct forecasts", ec["correct"], ec["courses"]),
                    ("Both orders", ec["both"], ec["opposed"]),
                ],
            )
        return result(
            "The price of looking",
            f"{policy}   /   {v['bodies']:,} circuits",
            "Correct first reaches",
            100 * v["correct"] / max(1, n),
            "%",
            [
                ["Observation work", work],
                ["Correct reaches", v["correct"]],
                ["Retained carbon", v["energy_left"]],
            ],
            [
                f"The confirmation circuit encountered {offers:,} possible tests across {n:,} lives.",
                f"It spent {work:,} units of energy on sensory acquisition and correctly intercepted {v['correct']:,} first portions.",
                f"After costs, it retained {v['energy_left']:,} units of carbon across those lives.",
            ],
            [
                f"Each life began with {v['purse']} units for acquiring evidence.",
                f"The nursery accounted for {int(carbon['heat']):,} units lost as optical work."
                if carbon
                else "Each intervention preserved the same external test layouts.",
            ],
            "Work and material caught",
            [
                ("Paid acquisitions", work, offers),
                ("First reaches correct", v["correct"], n),
            ],
        )
    q = last("field_soundings")
    if q and int(q["offered"]):
        v = {k: int(x) for k, x in q.items()}
        sample = last("tracer_witness")
        return result(
            "Testing the current with a grain",
            f"Watershed tick {v['field_soundings']:,}   /   living hands",
            "Tracer reaches initiated",
            100 * v["initiated"] / v["offered"],
            "%",
            [
                ["Initiated reaches", v["initiated"]],
                ["Grains relocated", v["moved"]],
                ["Neighbor observations", v["neighbour_looks"]],
            ],
            [
                f"Residents encountered {v['offered']:,} pairs of wet inlets with litter available.",
                f"They initiated {v['initiated']:,} tracer releases, moving {v['moved']:,} grains between sites.",
                f"Neighbors paid for {v['neighbour_looks']:,} observations of the releases and changed {v['neighbour_edits']:,} synaptic bits.",
            ],
            [
                f"On {v['divergent']:,} initiated trials the two sources led to different outlets.",
                "Every successful push debited the upstream litter and deposited one grain at the measured exit.",
            ],
            "One animal's physical experiment",
            [
                ("Initiated", v["initiated"], v["offered"]),
                ("Traveled", v["moved"], v["initiated"]),
            ],
            ("tracer_ports", sample) if sample else None,
        )
    return None


def inquiry_illustration(d, kind, q, x, y, size, ink, muted, gold, green, grid):
    if kind == "public_code":
        for k, code in enumerate((q["first"], q["second"])):
            top = y + 180 + k * 135
            d.label(x + 29, top, f"Temporal order {k + 1}", gold if not k else green)
            for row in range(8):
                for col in range(4):
                    inhibited = code & (1 << (4 * row + col))
                    d.rect(
                        x + 31 + row * 42,
                        top + 16 + col * 16,
                        28,
                        11,
                        grid if inhibited else green,
                        2,
                    )
        return
    if kind == "one_offer":
        names = ("north", "east", "south", "west")
        for i, port in enumerate((q["first"], q["second"])):
            top = y + 195 + i * 115
            d.text(x + 30, top, names[port].capitalize(), 33, ink, SERIF)
            watched = (
                "seen earlier"
                if q["pupil_saw"] & (1 << port)
                else "unseen by the pupil"
            )
            d.text(x + 30, top + 33, watched, 17, muted, MONO)
            if q["selected"] == port and q["words"]:
                d.text(x + size - 30, top, "offered", 23, green, SERIF, anchor="end")
        return
    if kind == "addressed_offer":
        sample = q["sample"]
        if sample:
            names = ("north", "east", "south", "west")
            for i, side in enumerate(("a", "b")):
                px = x + 27 + i * (size - 42) / 2
                d.label(px, y + 161, f"Listener {side.upper()}", green if i else gold)
                visible = int(sample[f"viewed_{side}"])
                place = (
                    "both inlets"
                    if visible.bit_count() > 1
                    else names[visible.bit_length() - 1]
                    if visible
                    else "none"
                )
                choice = (
                    names[int(sample[f"chosen_{side}"])]
                    if int(sample[f"opened_{side}"])
                    else "silent"
                )
                d.text(px, y + 217, place, 36, ink, SERIF)
                d.text(px, y + 247, "inlet observed", 17, muted, MONO)
                d.rule(px, y + 267, px + (size - 88) / 2, y + 267, grid)
                d.text(px, y + 322, choice, 33, green if i else gold, SERIF)
                d.text(px, y + 351, "inlet offered", 17, muted, MONO)
                d.text(
                    px,
                    y + 399,
                    f"{sample['prior_' + side]} → {sample['final_' + side]} caught",
                    20,
                    ink,
                    MONO,
                )
            return
        q = q["summary"]
        stats = (
            ("Food before", q["caught_before"], q["portions"]),
            ("Food after", q["caught_after"], q["portions"]),
        )
    elif kind == "public_outlines":
        stats = (
            ("Offered a reach", q["opportunities"], q["opportunities"]),
            ("Made a public reach", q["public_reaches"], q["opportunities"]),
            ("Changed by the outline", q["face_sensitive"], q["opportunities"]),
        )
    else:
        stats = ()
    if stats:
        for i, (label, value, maximum) in enumerate(stats):
            top = y + 166 + i * 91
            d.text(x + 28, top, label, 21, ink, SERIF)
            d.text(x + size - 29, top, f"{value:,}", 19, gold, MONO, anchor="end")
            d.rect(x + 28, top + 20, size - 57, 10, grid, 4)
            if value:
                d.rect(
                    x + 28,
                    top + 20,
                    (size - 57) * value / max(1, maximum),
                    10,
                    green,
                    4,
                )
        return
    if kind == "inquiry_signals":
        full = q["courses"]
        top = y + 176
        for label, key, color in (
            ("Before any investigation", "correct_before", gold),
            ("After the animal's own tests", "correct_after", green),
        ):
            value = q[key]
            d.text(x + 29, top, label, 23, ink, SERIF)
            d.rect(x + 30, top + 16, size - 61, 14, grid, 4)
            if value:
                d.rect(x + 30, top + 16, (size - 61) * value / full, 14, color, 4)
            d.text(x + 30, top + 56, f"{value:,} / {full:,} correct", 18, color, MONO)
            top += 113
        d.rule(x + 29, top - 23, x + size - 30, top - 23, grid)
        d.text(
            x + 29,
            top + 19,
            f"{q['investigated']:,} orders prompted a grain release",
            20,
            ink,
        )
        d.text(
            x + 29,
            top + 54,
            f"{q['grain_releases']:,} grains released in total",
            18,
            muted,
        )
        return
    point = ((0, 0), (1, 0), (1, 1), (0, 1))
    names = ("NW", "NE", "SE", "SW")
    src = int(q["source"])
    sink = int(q["outlet"])
    other = int(q["second"]) if int(q["first"]) == src else int(q["first"])
    centers = [(x + 117 + a * 178, y + 208 + b * 106) for a, b in point]
    d.label(
        x + 28,
        y + 140,
        f"Resident {int(q['resident']):,}   /   tile {int(q['site']):,}",
        muted,
    )
    for j, (cx, cy) in enumerate(centers):
        tx, ty = centers[(j + 1) % 4]
        d.rule(cx, cy, tx, ty, grid, 4)
    if src != sink:
        a, b = centers[src]
        c, e = centers[sink]
        d.rule(a, b, c, e, green, 5)
    for j, (cx, cy) in enumerate(centers):
        color = gold if j == src else green if j == sink else muted
        d.dot(cx, cy, 17, color if j in (src, sink) else grid)
        d.text(
            cx,
            cy + 5,
            names[j],
            13,
            "#122e2d" if j in (src, sink) else ink,
            MONO,
            anchor="middle",
        )
    d.text(
        x + 29,
        y + 388,
        f"Chose {names[src]}   /   grain reached {names[sink]}",
        23,
        ink,
        SERIF,
    )
    d.text(
        x + 29,
        y + 431,
        f"Other inlet {names[other]} led to {names[int(q['other_outlet'])]}",
        20,
        muted,
    )
    d.text(
        x + 29,
        y + 482,
        f"Source stock   {q['stock']} → {q['stock_after']}",
        18,
        gold,
        MONO,
    )
    d.text(
        x + 29,
        y + 518,
        f"Outlet stock   {q['downstream']} → {q['downstream_after']}",
        18,
        green,
        MONO,
    )
