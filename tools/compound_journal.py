"""Recorded two-port messages, genotype screens and zero-exposure transfer for the evolution journal."""


def compound_observation(entries, result):
    def integers(row):
        return {k: int(v) for k, v in row.items() if str(v).isdigit()}

    def last(tag):
        lines = entries(tag)
        return integers(lines[-1]) if lines else {}

    voice = last("auditory_feeders")
    if voice:
        q = voice
        cases = q["cases"]
        requests = q.get("requests", 0)
        answered = q.get("answers", 0)
        channel = (
            "Clear",
            "Muted",
            "One word",
            "Reordered",
            "Repeated word",
            "Unfamiliar prompt",
            "Shuttered witness",
            "Independent word loss",
        )[q["control"]]
        return result(
            "Finding food through another animal’s voice",
            f"{channel}   /   writer {q['auditory_feeders']:,}   /   receiver {q['reader']:,}",
            "Stocked feeder reached",
            100 * q["found"] / cases,
            "%",
            [
                ["Physical scenes", cases],
                ["Questions asked", requests],
                ["Answers heard", answered],
            ],
            [
                f"The receiving forager heard a peer through the {channel.lower()} channel in {cases:,} private-cue trials.",
                f"It reached the stocked feeder in {q['found']:,} trials and accurately described its own movement in {q['reported_choice']:,}.",
                f"The animals exchanged {answered:,} answers to {requests:,} requests for clarification.",
            ],
            [
                "The receiver has no optical access to its partner’s events.",
                "A missing movement report is counted separately from an incorrect destination.",
            ],
            "Speech, action and food",
            [
                ("Call arrived", q["called"], cases),
                ("Acted", q["interpreted"], cases),
                ("Reached food", q["found"], cases),
                ("Reported own movement", q["reported_choice"], cases),
            ],
        )
    vocal = entries("voiced_encounter")
    if vocal:
        visits = [integers(r) for r in vocal]
        spoken = [r for r in visits if r.get("voiced_encounter") == 2]
        reports = [r for r in visits if r.get("voiced_encounter") == 3]
        blind = [r for r in visits if r.get("voiced_encounter") == 1]
        n = len(spoken)
        questions = sum(r.get("questions", 0) for r in spoken)
        answers = sum(r.get("answers", 0) for r in spoken)
        meals = sum(r.get("portions", 0) for r in spoken)
        return result(
            "A question across two closed booths",
            "Spoken exchange   /   private pressure and movement memories",
            "Shared feeder portions",
            meals,
            " portions",
            [
                ["Private sightings", len(blind)],
                ["Questions asked", questions],
                ["Answers heard", answers],
            ],
            [
                f"The witness operated the dyed junctions in {len(blind)} private encounters.",
                f"The listener issued {questions} clarification requests and received {answers} answers.",
                f"The animals shared {meals} feeder portions and the listener voiced {sum(r.get('words',0)==2 for r in reports)} two-word movement reports.",
            ],
            ["Each utterance is reproduced from this animal’s own auditory contacts."],
            "What happened during the exchange",
            [
                (
                    "Two-word observations",
                    sum(r.get("words", 0) == 2 for r in spoken),
                    max(1, n),
                ),
                (
                    "Receiver moved",
                    sum(r.get("reached", 4) < 4 for r in spoken),
                    max(1, n),
                ),
                (
                    "Food found",
                    sum(r.get("portions", 0) > 0 for r in spoken),
                    max(1, n),
                ),
            ],
        )
    speed = entries("compound_benchmark")
    if speed:
        q = speed[-1]
        choices = int(q["compound_benchmark"])
        gain = float(q["speedup"])
        return result(
            "Four physical choices share a GPU warp",
            "Blackwell   /   frozen physical readout   /   alternating medians",
            "Readout speedup",
            gain,
            "×",
            [
                ["Compared choices", choices],
                ["Receipt bytes", int(q["receipt_bytes"])],
                ["Registers per lane", int(q["registers"])],
            ],
            [
                f"Two CUDA implementations evaluated {choices:,} four-feeder choices.",
                f"Their packed physical receipts disagreed on {int(q['differing']):,} choices.",
                f"The scalar and cooperative kernels took {float(q['scalar_ms']):.3f} and {float(q['cooperative_ms']):.3f} milliseconds.",
            ],
            ["Timing covers frozen compound inference only."],
            "Per-kernel median time",
            [
                ("Scalar", float(q["scalar_ms"]), float(q["scalar_ms"])),
                ("Four-lane", float(q["cooperative_ms"]), float(q["scalar_ms"])),
            ],
        )
    school = entries("signal_vocabulary")
    if school:
        q = [integers(r) for r in school]
        individual = q[0]
        return result(
            "Learning sounds for isolated events",
            "Paid single-port and single-limb experiences",
            "Elementary exposures",
            sum(r["heard"] for r in q),
            " exposures",
            [
                ["Pupils", len(q)],
                ["Combined examples", 0],
                ["Learned organ, bytes", individual["voice_bytes"]],
            ],
            [
                f"{len(q)} residents heard a sound while witnessing a single pressure or movement event.",
                f"They admitted {sum(r['single_pressure'] for r in q):,} pressure associations and {sum(r['single_movements'] for r in q):,} movement associations.",
                "Their teacher demonstrated no paired sounds.",
            ],
            [
                "Each pupil has separate receptors, signed contacts and an episodic record."
            ],
            "Associations admitted",
            [
                (
                    "Pressure",
                    sum(r["single_pressure"] for r in q),
                    max(1, sum(r["heard"] for r in q) // 2),
                ),
                (
                    "Movement",
                    sum(r["single_movements"] for r in q),
                    max(1, sum(r["heard"] for r in q) // 2),
                ),
            ],
        )
    q = last("compound_lives")
    if q:
        omitted = q["omit"] < 4
        return result(
            (
                "Can two learned signs locate a new feeder?"
                if omitted
                else "Four meanings in independent lifetimes"
            ),
            f"{q['compound_lives']:,} complete histories   /   {q['lessons']:,} feeding opportunities each",
            "Reliable four-way histories",
            q["complete"],
            " histories",
            [
                ["Independent lives", q["compound_lives"]],
                ["Reliable before", q["initially_complete"]],
                ["Reliable after", q["complete"]],
            ],
            [
                f"The same two animals were reared independently in {q['compound_lives']:,} histories.",
                (
                    f"The rearing schedule omitted the {('northwest','northeast','southwest','southeast')[q['omit']]} feeder cue."
                    if omitted
                    else "All four feeder cues occurred in the rearing environment."
                ),
                f"The pair developed reliable, distinct signals for all four goals in {q['complete']:,} histories.",
            ],
            [
                f"Reliable signalling requires at least 75% consistency and accuracy for every cue.",
                (
                    f"The fourth cue was correct on {q['heldout_correct']:,} of {q['tests']*q['compound_lives']:,} unrewarded tests."
                    if omitted
                    else f"Erasing both marks left {q['erased']:,} of {q['cases']:,} correct."
                ),
            ],
            "Types of acquired mapping",
            [
                ("Separable codes", q["separable"], q["compound_lives"]),
                ("Joint codes", q["joint"], q["compound_lives"]),
            ],
        )
    q = last("compound_population")
    if q:
        data = {
            **q,
            "with_both": q["after"],
            "first": q["erase_first"],
            "second": q["erase_second"],
            "blank": q["erase_both"],
        }
        return result(
            "What can two physical marks convey?",
            f"Living population  /  tick {q['compound_population']:,}  /  four feeders",
            "Correct feeder choices",
            100 * q["after"] / q["cases"],
            "%",
            [
                ["Animal pairs", q["pairs"]],
                ["Reliable histories", q["fluent"]],
                ["Pairs with a reliable history", q["capable_pairs"]],
            ],
            [
                f"{q['pairs']:,} pairs faced the four-feeder problem in {q['histories']} independent lives each.",
                f"Their trained receivers selected the food on {q['after']:,} of {q['cases']:,} balanced choices.",
                f"Erasing both physical marks gave {q['erase_both']:,} correct choices.",
            ],
            [
                f"Each actor has four bytes of signed two-port contacts.",
                f"{q['separable_codes']:,} reliable histories used separable codes and {q['joint_codes']:,} used joint codes.",
            ],
            "Which physical marks reached the reader?",
            [],
            ("compound_erasures", data),
        )
    q = last("compound_exchange")
    if q:
        cue = [integers(r) for r in entries("compound_cue")]
        return result(
            "Reading the two-part mark",
            f"Writer {q['compound_exchange']:,}   /   reader {q['receiver']:,}",
            "Correct feeder choices",
            100 * q["correct"] / q["cases"],
            "%",
            [
                ["Correct choices", q["correct"]],
                ["All four cues right", q["all_four"]],
                ["Both marks erased", q["erase_both"]],
            ],
            [
                f"The receiving animal followed two local pressure readings on {q['cases']:,} tests.",
                f"It selected the correct feeder {q['correct']:,} times, including all four alternative cues on {q['all_four']:,} matched sets.",
                f"Suppressing the first and second marks separately gave {q['erase_first']:,} and {q['erase_second']:,} correct choices.",
            ],
            [
                f"Four physical destinations shared a pair of closed junctions.",
                "Each column counts gestures produced by the actual sender.",
            ],
            "The writer’s learned code",
            [],
            ("compound_codes", cue),
        )
    exchanges = entries("compound_message")
    if exchanges:
        q = [integers(r) for r in exchanges]
        hits = sum(r["feeder"] == r["cue"] for r in q)
        return result(
            "Four destinations, two trapped dye pairs",
            "Embodied encounter with a separate sender and reader",
            "Correct feeder choices",
            hits,
            " choices",
            [
                ["Messages", len(q)],
                ["Correct choices", hits],
                ["Feeder portions eaten", sum(r["portions"] for r in q)],
            ],
            [
                f"The sender saw {len(q)} feeder cues and chose its own two-port gestures.",
                f"The receiver inferred {hits} destinations from its own pressure observations.",
                f"After the final encounter, {q[-1]['external_drops']} unpoured portions remained outside the closed junctions.",
            ],
            [],
            "Observed paired gestures",
            [],
            (
                "compound_codes",
                [
                    {
                        "compound_cue": j,
                        "tests": sum(r["cue"] == j for r in q),
                        **{
                            f"mark{k}": sum(r["cue"] == j and r["mark"] == k for r in q)
                            for k in range(4)
                        },
                    }
                    for j in range(4)
                ],
            ),
        )
    return None


def compound_illustration(d, kind, data, x, y, size, ink, muted, gold, green):
    if kind == "compound_erasures":
        for i, (label, key) in enumerate(
            (
                ("Both marks received", "with_both"),
                ("First mark absent", "first"),
                ("Second mark absent", "second"),
                ("Both marks absent", "blank"),
            )
        ):
            yy = y + 165 + i * 88
            d.text(x + 30, yy, label, 20, ink, SANS)
            rate = data[key] / max(1, data["cases"])
            d.text(x + size - 32, yy, f"{rate*100:.1f}%", 20, ink, MONO, anchor="end")
            d.rect(x + 32, yy + 18, size - 64, 17, "#314a45", 3)
            d.rect(x + 32, yy + 18, (size - 64) * rate, 17, green if not i else gold, 3)
        return
    left = x + size * 0.3
    pitch = (size * 0.65) / 4
    for j, label in enumerate(("Wait", "First", "Second", "Both")):
        d.text(
            left + (j + 0.5) * pitch, y + 164, label, 15, muted, MONO, anchor="middle"
        )
    for i, r in enumerate(data):
        yy = y + 187 + i * 71
        d.text(
            x + 32, yy + 29, ("NW", "NE", "SW", "SE")[r["compound_cue"]], 19, ink, MONO
        )
        n = max(1, r["tests"])
        for j in range(4):
            v = r["mark" + str(j)]
            xx = left + j * pitch
            d.rect(
                xx,
                yy,
                pitch - 5,
                51,
                (
                    "#6fa68f"
                    if v / n >= 0.7
                    else "#466359" if v / n >= 0.2 else "#2c4945"
                ),
                4,
            )
            d.text(
                xx + (pitch - 5) / 2, yy + 32, str(v), 16, ink, MONO, anchor="middle"
            )
    d.text(
        x + 32, y + 531, "Columns show which junctions were pumped.", 16, muted, MONO
    )
