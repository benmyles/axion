"""Field plates for suspended food, inherited reaches and material legacies."""


def drift_observation(entries):
    def last(key):
        found = entries(key)
        return found[-1] if found else {}

    def n(row, key):
        return int(row.get(key, 0))

    def result(
        title,
        subtitle,
        measure,
        value,
        unit,
        stats,
        narrative,
        notes,
        panel,
        rows,
        detail=None,
    ):
        return (
            (title, subtitle, measure, value, unit, stats, narrative, notes),
            panel,
            rows,
            detail,
        )

    pressure = capillary_observation(entries, result)
    if pressure:
        return pressure
    vision = retinal_observation(entries, result)
    if vision:
        return vision
    transfer = inlet_observation(entries, result)
    if transfer:
        return transfer
    q = last("drift_conversation")
    if q:
        total = n(q, "anchors")
        events = n(q, "events")
        paired = n(q, "both_sides")
        conflicts = n(q, "discrepancies")
        return result(
            "What actually passed there?",
            f"Resident {int(q['drift_conversation']):,}   /   {events:,} experienced currents",
            "Three consecutive correct turns",
            n(q, "three_turns"),
            " exchanges",
            [
                ["Actual encounters", events],
                ["Spoken anchors", total],
                ["Discrepancies resolved", paired],
            ],
            [
                f"Each conversation began with one of four socket names from an actual encounter.",
                f"The pupil correctly reported both its forecast and the different outcome at {paired:,} of {conflicts:,} conflicting sockets.",
                f"Erasing the public topic produced {n(q, 'erased_topic_answers'):,} audible answers to the same follow-up.",
            ],
            [
                "The acoustic questions are replayed through the enrolled creature’s own learned contacts.",
                f"Removing the private scene produced {n(q, 'missing_scene_answers'):,} audible replies.",
            ],
            "Following the referent",
            [
                ("Correct forecast reports", n(q, "expected_right"), total),
                ("Same words after an anchor", n(q, "actual_right"), total),
                ("Earlier view recalled", n(q, "before_right"), total),
                ("Both sides of a discrepancy", paired, conflicts),
                ("Follow-up with topic cleared", n(q, "erased_topic_right"), total),
            ],
        )
    q = last("drift_witness")
    if q and q["drift_witness"] != "horizon":
        surprise = q["drift_witness"] == "surprise"
        return result(
            "A current defied the forecast"
            if surprise
            else "A forecast at the waterline",
            f"Resident {n(q, 'slot'):,}   /   field tick {n(q, 'tick'):,}",
            "Expressed somatic neurons",
            n(q, "cells"),
            " cells",
            [
                ["Birth tick", n(q, "born")],
                ["Somatic cells", n(q, "cells")],
                ["Observed openings", 4],
            ],
            [
                f"The observer stopped at a current seen by resident {n(q, 'slot'):,}.",
                f"Its acquired contacts predicted physical outlet {n(q, 'expected') + 1} and the load emerged at outlet {n(q, 'outlet') + 1}.",
                "The recorded retinal frames preserve the individual’s own optical wiring.",
            ],
            [
                "Each column below is in the creature’s retinal frame.",
                "An empty forecast socket indicates uncertainty.",
            ],
            "The visible encounter",
            [],
            ("retina", q),
        )
    q = last("drift_legacy")
    if q:
        actual = n(q, "eaten")
        other = n(q, "without")
        later = n(q, "younger_after_death")
        example = last("drift_bequest")
        return result(
            "A strand outlives its maker"
            if later
            else "Following a strand through the current",
            f"{n(q, 'surfaces'):,} individually instrumented edges",
            "Food reaching later-born animals after the maker died",
            later,
            " units",
            [
                ["New strands marked", n(q, "deposited")],
                ["Routes redirected", n(q, "redirected")],
                ["Later-born intake", later],
            ],
            [
                f"Animals added {n(q, 'deposited'):,} marked units of fibre to monitored seams.",
                f"Removing one still-undisturbed unit in a matched replay changed the outlet at {n(q, 'redirected'):,} later pulses.",
                f"Receivers born after construction ate {later:,} units following the maker’s death.",
            ],
            [
                f"The same reaches could have captured {other:,} units without the individual strands.",
                f"The oldest witnessed delivery to another animal followed construction by {n(q, 'oldest'):,} ticks.",
            ],
            "An inherited waterwork",
            [
                ("Food to other operators", n(q, "foreign"), actual),
                ("After the maker’s death", n(q, "after_death"), actual),
                ("Later-born operators", n(q, "younger"), actual),
                ("Both conditions", later, actual),
            ],
            ("strand", example) if example else None,
        )
    ceiling, widths = last("neural_ceiling"), last("neural_widths")
    if ceiling and widths:
        prior = n(ceiling, "prior_cells")
        bins = [int(v) for v in widths["by_32_cells"].split(",")]
        living = n(widths, "living")
        larger = sum(bins[prior // 32 :])
        return result(
            "How large did the brains grow?",
            f"Neural capacity opened from {prior:,} to {n(widths, 'capacity_cells'):,} cells",
            "Residents above the earlier cell limit",
            larger,
            " residents",
            [
                ["Residents counted", living],
                ["Median cells", n(widths, "median_cells")],
                ["Largest brain", n(widths, "max_cells")],
            ],
            [
                f"The restored watershed gained room for {n(widths, 'capacity_cells'):,} spiking cells per resident.",
                f"The closing census found {larger:,} animals with more than {prior:,} cells.",
                f"Their population expressed {n(widths, 'tissue_cells'):,} somatic cells in total.",
            ],
            [
                "The expansion retained every existing cell and synaptic state.",
                "Heritable development and the energy available to each living animal governed subsequent growth.",
            ],
            "Expressed cells per animal",
            [
                (f"{a}–{b} cells", sum(bins[a // 32 - 1 : b // 32]), living)
                for a, b in ((32, 128), (160, 256), (288, 512), (544, 768), (800, 1024))
            ],
        )
    assays = entries("drift_food_test")
    if assays:
        q = assays[-1]
        offered = n(q, "individual_offered")
        eaten = n(q, "individual_food")
        return result(
            "Germlines in unfamiliar currents",
            f"{n(q, 'alleles'):,} distinct inherited learning rules",
            "Selected rule’s confirmation intake",
            100 * eaten / max(1, offered),
            "%",
            [
                ["Different rules", n(q, "alleles")],
                ["New lifetimes", n(q, "lives")],
                ["New food captured", eaten],
            ],
            [
                f"Each expressed rule faced {n(q, 'lives'):,} fresh three-season food distributions.",
                "The selected rule had the highest intake in its weakest season on a separate selection panel.",
                f"It caught {eaten:,} of {offered:,} new beads on confirmation.",
            ],
            [
                "All test lives started with untrained local contacts.",
                "No language or classroom score determined the choice.",
            ],
            "Independent food test",
            [
                ("Selected local rule", eaten, offered),
                ("Untrained reach", n(q, "individual_blank"), offered),
                ("Where the current changed", n(q, "changed_food"), n(q, "changed")),
                ("Population weighted by abundance", n(q, "eaten"), n(q, "offered")),
            ],
        )
    q = last("drift_tick")
    groups = {r["drift_lineage"]: r for r in entries("drift_lineage")}
    if q and "1" in groups and "2" in groups:
        a, b = groups["1"], groups["2"]
        total = n(a, "living") + n(b, "living")
        initial = n(a, "initial") + n(b, "initial")
        ticks = int(q["drift_tick"]) - n(q, "start")
        return result(
            "Two lineages share the same current",
            f"{ticks:,} ticks of ordinary feeding and reproduction",
            "Lineage A among marked survivors",
            100 * n(a, "living") / max(1, total),
            "%",
            [
                ["Lineage A alive", n(a, "living")],
                ["Lineage B alive", n(b, "living")],
                ["Recorded births", n(a, "born") + n(b, "born")],
            ],
            [
                f"The interval began with {n(a, 'initial'):,} animals in A and {n(b, 'initial'):,} in B.",
                f"Over {ticks:,} ticks their descendants produced {n(a, 'born'):,} and {n(b, 'born'):,} births respectively.",
                f"The closing census found {total:,} living marked residents.",
            ],
            [
                f"A had {'access' if n(a, 'recall') else 'no access'} to learned reach contacts and B had {'access' if n(b, 'recall') else 'no access'}.",
                "All offspring pay the ordinary ecological costs and inherit their parent’s label.",
            ],
            "Birth and survival",
            [
                ("A at the opening census", n(a, "initial"), initial),
                ("A at the closing census", n(a, "living"), total),
                ("B at the closing census", n(b, "living"), total),
                ("A among births", n(a, "born"), n(a, "born") + n(b, "born")),
            ],
        )
    if q and n(q, "pulses"):
        amount = n(q, "released")
        food = n(q, "eaten")
        null = n(q, "blank_eaten")
        profile = last("drift_profile")
        return result(
            "Food on a changing current",
            f"Watershed tick {int(q['drift_tick']):,}",
            "Suspended carbon intercepted",
            100 * food / max(1, amount),
            "%",
            [
                ["Food released", amount],
                ["Intercepted", food],
                ["Resettled", n(q, "settled")],
            ],
            [
                f"Finite currents carried {amount:,} units of organic material through local crossings.",
                f"The mouths intercepted {food:,} units before {n(q, 'settled'):,} settled back to the ground.",
                f"The same bodies could have caught {null:,} units with their acquired outlet contacts silenced.",
            ],
            [
                f"{n(q, 'tied'):,} strands were added and {n(q, 'untied'):,} were taken down.",
                f"The isolated drift kernel took {float(profile['median_us']):,.1f} microseconds at the median."
                if profile
                else "Tactile feedback arrives after a reach is committed.",
            ],
            "At the four apertures",
            [
                ("Actual capture", food, amount),
                ("Blank-contact capture", null, amount),
                ("Path altered by fibres", n(q, "diverted"), n(q, "pulses")),
                ("A different learned reach", n(q, "choices_changed"), n(q, "reached")),
            ],
        )
    return None


def drift_panel(d, observation, x, y, size, ink, muted, gold, green, grid):
    _, title, rows, detail = observation
    if not detail:
        return regulatory_panel(
            d, observation[:3], x, y, size, ink, muted, gold, green, grid
        )
    kind, q = detail
    d.label(x + 28, y + 40, "Recorded physical evidence", green)
    d.text(x + 28, y + 94, title, 35, ink, SERIF)
    if kind in ("compound_codes","compound_erasures"):
        return compound_illustration(d,kind,q,x,y,size,ink,muted,gold,green)
    if kind in ("context_history","context_reach","context_genotypes","context_questions","chemical_relay","chemical_messages","chemical_counts","chemical_neighbour"):
        return context_illustration(d,kind,q,x,y,size,ink,muted,gold,green,grid)
    if kind == "detritus_tile":
        return detritus_illustration(d,q,x,y,size,ink,muted,gold,green,grid)
    if kind == "pressure_wake":
        return wake_illustration(d,q,x,y,size,ink,muted,gold,green,grid)
    if kind == "pressure_plate":
        return pressure_illustration(d,q,x,y,size,ink,muted,gold,green,grid)
    if kind == "pigment_history":
        return pigment_illustration(d,q,x,y,size,ink,muted,gold,green,grid)
    if kind in ("inquiry_signals","tracer_ports","addressed_offer","public_outlines","public_code","one_offer"):
        return inquiry_illustration(d,kind,q,x,y,size,ink,muted,gold,green,grid)
    if kind == "visual_lineages":
        return lineage_illustration(d,q,x,y,size,ink,muted,gold,green,grid)
    if kind == "woven_sample":
        return woven_illustration(d,q,x,y,size,ink,muted,gold,green,grid)
    if kind == "cascade_pair":
        return cascade_illustration(d,q,x,y,size,ink,muted,gold,green,grid)
    if kind in ("inlet_map", "inlet_voice"):
        return inlet_panel_illustration(d, kind, q, x, y, size, ink, muted, gold, green, grid)
    if kind == "retina":
        colors = ("#f3bb71", "#a1c27e", "#b595c1", "#79c5ba")
        names = ("amber", "jade", "plum", "teal")
        left = x + 145
        step = (size - 173) / 4
        for i in range(4):
            d.text(
                left + (i + 0.5) * step,
                y + 148,
                ("N", "E", "S", "W")[i],
                18,
                muted,
                MONO,
                anchor="middle",
            )
        for r, (f, label) in enumerate(((0, "Before"), (2, "Expected"), (1, "Actual"))):
            yy = y + 174 + r * 98
            d.text(x + 29, yy + 37, label, 21, ink)
            for c in range(4):
                known = int(q["known"]) & (1 << (4 * f + c))
                color = int(q["images"]) >> (8 * f + 2 * c) & 3
                d.rect(
                    left + c * step + 3,
                    yy,
                    step - 6,
                    49,
                    colors[color] if known else grid,
                    7,
                )
                d.text(
                    left + (c + 0.5) * step,
                    yy + 70,
                    names[color] if known else "unseen",
                    14,
                    muted,
                    MONO,
                    anchor="middle",
                )
    else:
        for maker in (True, False):
            key = "maker_" if maker else ""
            identity = int(q["maker" if maker else "receiver"])
            body = {
                "body_plan": int(q[key + "shape"]),
                "legs": int(q[key + "limbs"]),
                "eyes": int(q[key + "eyes"]),
                "pores": int(q[key + "pores"]),
            }
            cx = x + size * (0.27 if maker else 0.73)
            d.label(cx - 60, y + 155, "Builder" if maker else "Beneficiary", gold)
            atlas_anatomy(d, cx, y + 245, body, f"{identity:08x}", 29)
            d.text(
                cx, y + 352, f"Resident {identity:,}", 18, ink, MONO, anchor="middle"
            )
            d.text(
                cx,
                y + 382,
                f"{int(q[key + 'cells']):,} spiking cells",
                16,
                muted,
                anchor="middle",
            )
        d.rect(x + 29, y + 416, size - 58, 2, grid)
        d.text(
            x + 29,
            y + 461,
            f"{int(q['age']):,} ticks after construction",
            24,
            ink,
            SERIF,
        )
        d.text(
            x + 29,
            y + 494,
            f"Outlet {int(q['alternative']) + 1} without the strand  /  {int(q['outlet']) + 1} with it",
            17,
            green,
            MONO,
        )
