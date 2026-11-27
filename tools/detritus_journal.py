"""Field diagrams for conserved food moved by creature-editable pressure channels."""


def detritus_observation(numbers, result):
    q = numbers("detritus_field")
    test = numbers("detritus_boundaries")
    if q and q.get("partitions"):
        mode = (
            "Local material pressure"
            if q["mode"] == 1
            else "Displaced pressure control"
        )
        scene = numbers("detritus_scene")
        control = numbers("capillary_dispatch")
        terms = (
            [
                f"Changing one handled source's emitted pressure enabled {q['enabled_edges']:,} individual transfers and prevented {q['prevented_edges']:,} in one-beat replays."
            ]
            if q["observed"]
            else [
                f"Of the transfers, {q['east']:,} travelled east and {q['south']:,} travelled south."
            ]
        )
        return result(
            "Where the current carries food",
            f"{mode}   /   habitat tick {q['detritus_field']:,}",
            "Existing portions transported",
            q["arrived"],
            " portions",
            [
                ["Arrivals", q["arrived"]],
                ["Departures", q["departed"]],
                ["Denied by full stock", q["congested"]],
            ],
            [
                f"Across {q['partitions']:,} four-site partitions, the current moved food from existing deposits.",
                (
                    f"Moving food changed the exposed colour at {q['changed_colours']:,} sites across the run."
                    if q.get("dye_exchange")
                    else "The sums of the four stocks before and after each update agreed across the run."
                ),
            ]
            + terms,
            [
                f"The routing rule required {q['permanent_site_bits']} extra permanent bits per site.",
                f"The GPU dispatched {control['cargo_launches']:,} transport kernels for {control['physical_beats']:,} physical beats."
                if control
                else f"Recording used {q['observation_bytes']:,} device bytes.",
            ],
            "Food in four neighbouring sites",
            [
                ("Accepted routes", q["departed"], q["offers"]),
                ("Moved east", q["east"], q["departed"]),
            ],
            ("detritus_tile", scene) if scene else None,
        )
    if test:
        return result(
            "When the receiving site is full",
            "CUDA transport proof   /   local boundary cases",
            "Cases checked",
            test["detritus_boundaries"],
            " cases",
            [
                ["Transferred portions", test["moved"]],
                ["Rejected offers", test["denied"]],
                ["Stocks per partition", 4],
            ],
            [
                f"The examiner tested {test['detritus_boundaries']:,} combinations of stocks, pressure, geometry and priority.",
                f"The receiving sites admitted {test['moved']:,} portions.",
                f"Full or nearly full sites denied {test['denied']:,} offers.",
            ],
            [
                "The four stock levels included zero and the maximum 16-bit value.",
                "Each row preserves the actual final stock at every site.",
            ],
            "Finite receiver space",
            [("Admitted portions", test["moved"], test["moved"] + test["denied"])],
        )
    return None


def detritus_illustration(d, q, x, y, size, ink, muted, gold, green, grid):
    edges = ((0, 1), (0, 2), (1, 3), (2, 3))
    cause = q["change"]
    before = [(q["food"] >> (16 * j)) & 65535 for j in range(4)]
    box = (size - 90) / 2
    pad = box * 0.16
    space = box - 2 * pad
    top = y + 195
    views = (
        (("Observed beat", q["strokes"]), ("Alternate pulse", q["alternate"]))
        if cause < 3
        else (("Before the beat", 0), ("After the beat", q["strokes"]))
    )
    for index, (name, route) in enumerate(views):
        ox = x + 28 + index * (box + 29)
        d.text(ox, top - 31, name, 23, ink, SANS, 600)
        d.rect(ox, top, box, box, "#203b3d", 8)
        centres = [
            (
                ox + pad + space * (0.18 + 0.64 * (j % 2)),
                top + pad + space * (0.18 + 0.64 * (j // 2)),
            )
            for j in range(4)
        ]
        stock = before.copy()
        for e, (a, b) in enumerate(edges):
            ax, ay = centres[a]
            bx, by = centres[b]
            if route & (1 << e):
                stock[a] -= 1
                stock[b] += 1
                dx = (bx - ax) / space
                dy = (by - ay) / space
                tx = (ax + bx) / 2
                ty = (ay + by) / 2
                d.rule(ax, ay, bx, by, "#e7b76c", 5)
                d.path(
                    f"M{tx - 8 * dx + 8 * dy} {ty - 8 * dy - 8 * dx} L{tx + 8 * dx} {ty + 8 * dy} L{tx - 8 * dx - 8 * dy} {ty - 8 * dy + 8 * dx}",
                    "#e7b76c",
                    3,
                )
            else:
                d.rule(ax, ay, bx, by, "#38524d", 2)
        tint = None
        if q.get("surface"):
            tint = [(q["paint_before"] >> (2 * j)) & 3 for j in range(4)]
            for e in ([3, 2] if q.get("priority", 0) & 2 else [2, 3]) + [0, 1]:
                if route & (1 << e):
                    a, b = edges[e]
                    tint[a], tint[b] = tint[b], tint[a]
        for j, (cx, cy) in enumerate(centres):
            hot = (
                (q["current"] ^ ((1 << cause) if index and cause < 3 else 0)) >> j
            ) & 1
            d.dot(
                cx,
                cy,
                21,
                "#287768" if hot else "#29494a",
                f'stroke="{gold if cause == j else muted}" stroke-width="2"',
            )
            d.text(cx, cy + 6, str(stock[j]), 17, ink, MONO, 600, "middle")
            if tint is not None:
                d.dot(
                    cx + 17,
                    cy - 17,
                    6,
                    ("#dfa856", "#92b985", "#ad83b4", "#66c4bf")[tint[j]],
                )
    yy = top + box + 49
    labels = [
        f"Starting stocks  {'  ·  '.join(str(v) for v in before)}",
        f"Plot origin ({q['x']}, {q['y']})   ·   tick {q['detritus_scene']:,}",
    ]
    if cause < 3:
        labels.append(
            f"Material handler #{q['handler']:,}   ·   {'local' if q['mode'] == 1 else 'displaced'} source"
        )
    else:
        labels.append("The two views show stocks before and after transport.")
    for i, label in enumerate(labels):
        d.text(x + 29, yy + 34 * i, label, 18, muted, MONO)
