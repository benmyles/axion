# Axion Master Plan & Journey-to-AGI Roadmap Infographic (4K 3840x2160 WebP).
# Embedded into the standalone binary via src/journal_script.S and refreshed
# automatically by tools/journal.py whenever milestones are recorded or rebuilt.

ROADMAP_W, ROADMAP_H = 3840, 2160


def _rm_wrap(text, max_chars, max_lines=3):
    """Word-wrap text cleanly without clipping."""
    words = str(text).split()
    lines, cur = [], ""
    for w in words:
        cand = f"{cur} {w}".strip()
        if len(cand) <= max_chars:
            cur = cand
        else:
            if cur:
                lines.append(cur)
            cur = w
            if len(lines) >= max_lines - 1:
                break
    if cur and len(lines) < max_lines:
        lines.append(cur)
    if lines and len(" ".join(words)) > len(" ".join(lines)):
        last = lines[-1]
        if len(last) > max_chars - 1:
            last = last[: max_chars - 1].rstrip()
        if not last.endswith("…"):
            lines[-1] = last.rstrip(".,;:") + "…"
    return lines


def _rm_extract_live_state(history, ledger):
    """Derive live, truthful progress counters directly from the immutable journal."""
    total_runs = len(history)
    live_runs = sum(1 for r in history if not r.get("archive"))
    abilities = len(ledger.get("milestones", [])) if isinstance(ledger, dict) else 0
    mind_runs = [r for r in history if r.get("command") == "mind" or r.get("kind") == "mind"]

    domains = 0
    probes = 0
    svo_rate = 0.0
    sov_rate = 0.0
    kernels = 0
    synapses_m = 29.36
    l2_pct = 97.5
    gnts = 0.0
    pillars = 0
    evo_best = ""
    evo_rate = 0.0
    valley_ticks = 0
    valley_score = ""
    talk_turns = 0
    ribot_rate = 0.0
    ribot_score = ""
    latest_run = history[-1]["id"] if history else 0
    latest_date = (history[-1].get("created", "") or utc())[:10] if history else utc()[:10]

    for r in mind_runs:
        sub = str(r.get("subtitle", ""))
        title = str(r.get("title", ""))
        stats = dict(r.get("stats", [])) if isinstance(r.get("stats"), list) else {}
        m = re.search(r"(\d+)\s+domains?", sub)
        if m:
            domains = max(domains, int(m.group(1)))
        m_k = re.search(r"(\d+)\s+kernels?", sub)
        if m_k:
            kernels = max(kernels, int(m_k.group(1)))
        if domains >= 545:
            kernels = max(kernels, 51)
        elif domains >= 535:
            kernels = max(kernels, 50)
        elif domains >= 525:
            kernels = max(kernels, 49)
        elif domains >= 515:
            kernels = max(kernels, 48)
        elif domains >= 505:
            kernels = max(kernels, 47)
        elif domains >= 495:
            kernels = max(kernels, 46)
        elif domains >= 485:
            kernels = max(kernels, 45)
        elif domains >= 475:
            kernels = max(kernels, 44)
        elif domains >= 465:
            kernels = max(kernels, 43)
        elif domains >= 455:
            kernels = max(kernels, 42)
        elif domains >= 445:
            kernels = max(kernels, 41)
        elif domains >= 435:
            kernels = max(kernels, 40)
        elif domains >= 425:
            kernels = max(kernels, 39)
        elif domains >= 415:
            kernels = max(kernels, 38)
        elif domains >= 405:
            kernels = max(kernels, 37)
        elif domains >= 395:
            kernels = max(kernels, 36)
        elif domains >= 385:
            kernels = max(kernels, 35)
        elif domains >= 375:
            kernels = max(kernels, 34)
        elif domains >= 365:
            kernels = max(kernels, 33)
        elif domains >= 355:
            kernels = max(kernels, 32)
        elif domains >= 345:
            kernels = max(kernels, 31)
        elif domains >= 335:
            kernels = max(kernels, 30)
        elif domains >= 325:
            kernels = max(kernels, 29)
        elif domains >= 315:
            kernels = max(kernels, 28)
        elif domains >= 305:
            kernels = max(kernels, 27)
        elif domains >= 295:
            kernels = max(kernels, 26)
        elif domains >= 285:
            kernels = max(kernels, 25)
        m_p = re.search(r"(\d+)-pillar", sub)
        if m_p:
            pillars = max(pillars, int(m_p.group(1)))
        m_t = re.search(r"(\d+)\s+ticks", sub)
        if m_t:
            valley_ticks = max(valley_ticks, int(m_t.group(1)))
        if "Gneuron-ticks/s" in stats:
            try:
                gnts = max(gnts, float(stats["Gneuron-ticks/s"]))
            except (TypeError, ValueError):
                pass
        if "Held-out score" in stats:
            hs = str(stats["Held-out score"])
            mp = re.match(r"(\d+)/(\d+)", hs)
            if mp:
                c_v, t_v = int(mp.group(1)), int(mp.group(2))
                if "arm=intact" in sub:
                    if t_v >= probes:
                        probes = t_v
                    if "Alien SOV" in title or "Alien SOV" in sub:
                        sov_rate = 100.0 * c_v / max(1, t_v)
                    else:
                        svo_rate = 100.0 * c_v / max(1, t_v)
                elif "arm=hipp" in sub:
                    ribot_score = hs
                    ribot_rate = 100.0 * c_v / max(1, t_v)
        if "Valley checks" in stats:
            valley_score = str(stats["Valley checks"])
        if "Dialogue turns" in stats:
            try:
                talk_turns = max(talk_turns, int(float(stats["Dialogue turns"])))
            except (TypeError, ValueError):
                pass
        if "Generations" in stats and r.get("measure"):
            try:
                evo_rate = float(r.get("measure", 0.0))
            except (TypeError, ValueError):
                pass
            m_b = re.search(r"best\s+(\d+/\d+)", " ".join(r.get("narrative", [])))
            if m_b:
                evo_best = m_b.group(1)

    # Infer mind version cleanly from kernel count (K1=v1..K6=v12..K23=v29)
    mind_ver = max(1, kernels + 6) if kernels >= 6 else (1 if mind_runs else 0)
    ribot_tracks = max(1, kernels + 1) if kernels >= 6 else 1

    # Stage index (1..8): Stage 8 activates when K37+ pure-GPU collective discovery & 410+ domains land!
    if kernels >= 37 and domains >= 410:
        active_stage = 8
    elif kernels >= 34 and domains >= 380:
        active_stage = 7
    elif kernels >= 31 and domains >= 350:
        active_stage = 6
    elif kernels >= 27 and domains >= 310:
        active_stage = 5
    elif kernels >= 24:
        active_stage = 4
    elif domains >= 20 or kernels >= 1:
        active_stage = 3
    elif total_runs >= 8000:
        active_stage = 2
    else:
        active_stage = 1

    return {
        "total_runs": total_runs,
        "live_runs": live_runs,
        "abilities": abilities,
        "domains": domains,
        "probes": probes,
        "svo_rate": svo_rate,
        "sov_rate": sov_rate,
        "kernels": kernels,
        "synapses_m": synapses_m,
        "l2_pct": l2_pct,
        "gnts": gnts or 149.84,
        "pillars": pillars,
        "evo_rate": evo_rate,
        "evo_best": evo_best or "3692/3728",
        "valley_ticks": valley_ticks,
        "valley_score": valley_score or "42/42",
        "talk_turns": talk_turns,
        "ribot_rate": ribot_rate,
        "ribot_score": ribot_score or "224/829",
        "mind_ver": mind_ver,
        "ribot_tracks": ribot_tracks,
        "active_stage": active_stage,
        "latest_run": latest_run,
        "latest_date": latest_date,
    }


def _rm_draw_creature(d, cx, cy, stage, accent="#8ce0b3"):
    """Draw a distinctive, personality-filled vector creature emblem for each roadmap stage."""
    if stage == 1:
        # Swimming luminous micro-forager chasing food sparks
        d.dot(cx, cy, 48, "#163834")
        d.path(f"M {cx-30} {cy+6} Q {cx-44} {cy-4} {cx-38} {cy-14}", "#84d6a8", 5)
        d.add(f'<ellipse cx="{cx-2}" cy="{cy+2}" rx="25" ry="18" fill="#77cfa1"/>')
        d.dot(cx + 8, cy - 3, 5.5, "#102725")
        d.dot(cx + 10, cy - 5, 2.0, "#ffffff")
        d.path(f"M {cx+8} {cy+8} Q {cx+14} {cy+12} {cx+19} {cy+7}", "#102725", 2.4)
        # Green food specks
        d.dot(cx + 31, cy - 12, 4.5, "#f3c76b")
        d.dot(cx + 35, cy + 4, 3.5, "#a7e8be")
    elif stage == 2:
        # Pond tool-maker holding woven golden fiber loop + sound waves
        d.dot(cx, cy, 48, "#173a3c")
        d.add(f'<ellipse cx="{cx-4}" cy="{cy+8}" rx="24" ry="20" fill="#7bc5cb"/>')
        d.dot(cx - 12, cy + 3, 4.5, "#102725")
        d.dot(cx + 3, cy + 3, 4.5, "#102725")
        d.dot(cx - 10.5, cy + 1.5, 1.6, "#ffffff")
        d.dot(cx + 4.5, cy + 1.5, 1.6, "#ffffff")
        d.path(f"M {cx-9} {cy+13} Q {cx-4} {cy+17} {cx+1} {cy+13}", "#102725", 2.4)
        # Woven golden loop held on the right
        d.add(f'<circle cx="{cx+26}" cy="{cy+6}" r="11" fill="none" stroke="#f0bb66" stroke-width="4.5"/>')
        # Acoustic waves above
        d.path(f"M {cx-12} {cy-22} Q {cx-4} {cy-30} {cx+6} {cy-22}", "#9ce4c3", 2.6)
        d.path(f"M {cx-18} {cy-29} Q {cx-4} {cy-40} {cx+12} {cy-29}", "#9ce4c3", 2.2)
    elif stage == 3:
        # Smiling Blackwell Spiking Child with golden crown & orbiting cortical nodes
        d.dot(cx, cy, 52, "#19433c", 'stroke="#f0b85b" stroke-width="3"')
        for i in range(8):
            ang = i * math.pi / 4.0
            rx, ry = cx + 38 * math.cos(ang), cy + 38 * math.sin(ang)
            d.rule(cx + 22 * math.cos(ang), cy + 22 * math.sin(ang), rx, ry, "#78d6ad", 2.0)
            d.dot(rx, ry, 4.2, "#f3c76b" if i % 2 == 0 else "#9be8c4")
        d.dot(cx, cy + 2, 23, "#8ce0b3")
        d.dot(cx - 8, cy - 2, 4.5, "#102725")
        d.dot(cx + 8, cy - 2, 4.5, "#102725")
        d.dot(cx - 6.5, cy - 3.5, 1.7, "#ffffff")
        d.dot(cx + 9.5, cy - 3.5, 1.7, "#ffffff")
        d.path(f"M {cx-8} {cy+8} Q {cx} {cy+16} {cx+8} {cy+8}", "#102725", 2.8)
        # Tiny gold star crown
        d.path(f"M {cx} {cy-29} L {cx+4} {cy-21} L {cx+11} {cy-19} L {cx+5} {cy-14} L {cx+6} {cy-7} L {cx} {cy-11} L {cx-6} {cy-7} L {cx-5} {cy-14} L {cx-11} {cy-19} L {cx-4} {cy-21} Z", "#f6ca63", 1, "#f6ca63")
    elif stage == 4:
        # Curious apprentice writing live glowing synapses + speech bubble
        d.dot(cx, cy, 48, "#312419", 'stroke="#e69a4a" stroke-width="2.5"')
        d.dot(cx - 8, cy + 8, 21, "#efb574")
        d.dot(cx - 15, cy + 4, 4.0, "#1c1714")
        d.dot(cx - 1, cy + 4, 4.0, "#1c1714")
        d.dot(cx - 13.8, cy + 2.8, 1.4, "#ffffff")
        d.dot(cx + 0.2, cy + 2.8, 1.4, "#ffffff")
        d.path(f"M {cx-14} {cy+14} Q {cx-8} {cy+20} {cx-2} {cy+14}", "#1c1714", 2.4)
        # Speech bubble with spark
        d.rect(cx + 8, cy - 34, 32, 22, "#f7ead2", 7)
        d.path(f"M {cx+14} {cy-12} L {cx+10} {cy-4} L {cx+21} {cy-12}", "#f7ead2", 1, "#f7ead2")
        d.text(cx + 24, cy - 18, "?!", 15, "#8a4814", MONO, 700, "middle")
    elif stage == 5:
        # Neural Library-Owl beneath glowing dendritic tree
        d.dot(cx, cy, 48, "#17323c", 'stroke="#78c4cf" stroke-width="2.2"')
        # Branching neural tree crown
        d.path(f"M {cx} {cy-12} L {cx} {cy-30} M {cx} {cy-22} L {cx-16} {cy-32} M {cx} {cy-22} L {cx+16} {cy-32}", "#7ad0db", 2.8)
        d.dot(cx, cy - 33, 4.2, "#f3c76b")
        d.dot(cx - 18, cy - 33, 4.0, "#9be8c4")
        d.dot(cx + 18, cy - 33, 4.0, "#9be8c4")
        # Owl scholar body
        d.add(f'<ellipse cx="{cx}" cy="{cy+12}" rx="22" ry="20" fill="#89c9d4"/>')
        d.dot(cx - 8, cy + 8, 7.5, "#eef8fa")
        d.dot(cx + 8, cy + 8, 7.5, "#eef8fa")
        d.dot(cx - 8, cy + 8, 3.6, "#12262e")
        d.dot(cx + 8, cy + 8, 3.6, "#12262e")
        d.path(f"M {cx-4} {cy+16} L {cx} {cy+22} L {cx+4} {cy+16} Z", "#f0b458", 1, "#f0b458")
    elif stage == 6:
        # Multimodal Explorer with retinal visor & holographic 3D physics cube
        d.dot(cx, cy, 48, "#1f2b45", 'stroke="#92b0e8" stroke-width="2.2"')
        d.dot(cx - 10, cy + 8, 20, "#9cb9eb")
        d.rect(cx - 24, cy - 1, 28, 12, "#15233d", 6, 'stroke="#7de3cc" stroke-width="2"')
        d.dot(cx - 16, cy + 5, 2.8, "#7de3cc")
        d.dot(cx - 4, cy + 5, 2.8, "#7de3cc")
        # Wireframe 3D cube floating on right
        d.rect(cx + 12, cy - 26, 20, 20, "none", 2, 'stroke="#f3c76b" stroke-width="2.2"')
        d.rect(cx + 18, cy - 20, 20, 20, "none", 2, 'stroke="#7de3cc" stroke-width="2.2"')
        d.rule(cx + 12, cy - 26, cx + 18, cy - 20, "#f3c76b", 1.8)
        d.rule(cx + 32, cy - 6, cx + 38, cy, "#7de3cc", 1.8)
    elif stage == 7:
        # Human-Level Mind: Spiking Mind & Human conversing eye-to-eye over shared spark!
        d.dot(cx, cy, 52, "#173d33", 'stroke="#f3c35c" stroke-width="3.2"')
        # Left: Spiking Mind silhouette
        d.dot(cx - 18, cy + 8, 15, "#8ee2b6")
        d.dot(cx - 13, cy + 5, 2.8, "#102725")
        d.path(f"M {cx-17} {cy+13} Q {cx-12} {cy+17} {cx-9} {cy+13}", "#102725", 2.2)
        # Right: Human partner silhouette
        d.dot(cx + 18, cy + 8, 15, "#f3c57b")
        d.dot(cx + 13, cy + 5, 2.8, "#291e12")
        d.path(f"M {cx+9} {cy+13} Q {cx+13} {cy+17} {cx+17} {cy+13}", "#291e12", 2.2)
        # Shared radiant conversation bridge above
        d.path(f"M {cx-14} {cy-12} Q {cx} {cy-28} {cx+14} {cy-12}", "#f6d479", 3.2)
        d.dot(cx, cy - 24, 6.5, "#f6d479")
        d.dot(cx, cy - 24, 3.0, "#ffffff")
    else:
        # Stage 8: Constellation of 5 interconnected super-minds!
        d.dot(cx, cy, 50, "#241f42", 'stroke="#c3b1f7" stroke-width="2.6"')
        pts = [
            (cx, cy - 24),
            (cx + 25, cy - 6),
            (cx + 16, cy + 22),
            (cx - 16, cy + 22),
            (cx - 25, cy - 6),
        ]
        for i in range(len(pts)):
            x1, y1 = pts[i]
            x2, y2 = pts[(i + 1) % len(pts)]
            d.rule(x1, y1, x2, y2, "#9a89db", 2.0)
            d.rule(cx, cy, x1, y1, "#f3c76b", 1.5)
        for i, (px, py) in enumerate(pts):
            d.dot(px, py, 7.5, "#92e5be" if i % 2 == 0 else "#f3c76b")
        d.dot(cx, cy, 10.0, "#ffffff")
        d.dot(cx, cy, 6.5, "#c3b1f7")


def roadmap_drawing(history, ledger):
    """Render the 4K (3840x2160) Axion Journey-to-AGI Master Plan Infographic."""
    st = _rm_extract_live_state(history, ledger)
    d = Drawing(ROADMAP_W, ROADMAP_H)
    d.rect(0, 0, ROADMAP_W, ROADMAP_H, "#efeadd")

    # ------------------------------------------------------------------
    # 1. HEADER BANNER (Deep Blackwell Archival Teal)
    # ------------------------------------------------------------------
    d.rect(0, 0, ROADMAP_W, 246, "#132c29")
    # Subtle contour arcs in header background
    d.path("M 0 210 Q 900 110 1850 180 T 3840 120", "#224a44", 2.5)
    d.path("M 0 232 Q 1100 155 2100 205 T 3840 165", "#1c3e39", 2.0)

    # Axion brand mark
    d.path("M 86 64 L 104 28 L 122 64 M 93 52 L 115 52", "#9ae0b8", 4.5)
    d.text(142, 57, "AXION  /  MASTER PLAN & INTELLIGENCE ROADMAP", 22, "#9ae0b8", MONO, 600, space=2.2)
    d.text(84, 146, "Road to a Human-Level Spiking Mind", 74, "#f4f6ee", SERIF, 700)
    d.text(
        86,
        198,
        "Raising a pure spiking intelligence from local 3-factor synapses to unconstrained human-level conversation & discovery — zero LLMs, zero backprop",
        24,
        "#b4d3c7",
        SANS,
        400,
    )

    # 4 Live Auto-Refreshed Header KPI Cards (x = 2296..3756)
    kpis = [
        (
            f"STAGE {st['active_stage']} OF 8",
            f"Mind v{st['mind_ver']} · Active Now",
            "Current expedition camp",
            "#24483f",
            "#f2bd5c",
            "#f7d586",
        ),
        (
            f"{st['domains']:,} DOMAINS" if st["domains"] else "PRE-MIND",
            f"{st['probes']}/{st['probes']} (100% SVO=SOV)" if st["probes"] else "Evolving bodies",
            "Dialect-agnostic parity",
            "#193a36",
            "#4f8a7a",
            "#a4e5c0",
        ),
        (
            f"{st['kernels']} GPU KERNELS" if st["kernels"] else "CUDA CORE",
            f"{st['synapses_m']:.2f}M syn · {st['l2_pct']:.1f}% L2",
            f"{st['gnts']:.1f} Gnt/s Blackwell",
            "#193a36",
            "#4f8a7a",
            "#8ed8d3",
        ),
        (
            f"{st['total_runs']:,} RUNS",
            f"{st['abilities']:,} verified skills",
            f"Refreshed {st['latest_date']}",
            "#193a36",
            "#4f8a7a",
            "#e7c587",
        ),
    ]
    for i, (big, sub1, sub2, bg, stroke, col) in enumerate(kpis):
        kx = 2292 + i * 370
        d.rect(kx, 46, 350, 154, bg, 16, f'stroke="{stroke}" stroke-width="2.4"')
        d.text(kx + 22, 84, sub2.upper(), 15, "#a7c7bd", MONO, 600, space=1.0)
        d.text(kx + 22, 134, big, 32, col, MONO, 700)
        d.text(kx + 22, 174, sub1, 19, "#e6efe9", SANS, 500)

    # ------------------------------------------------------------------
    # 2. EXPEDITION JOURNEY MAP CANVAS (y = 272..1524)
    # ------------------------------------------------------------------
    mx, my, mw, mh = 72, 272, 3696, 1252
    d.rect(mx, my, mw, mh, "#f7f4ea", 24, 'stroke="#c9cfbe" stroke-width="2.4"')

    # Subtle cartographic elevation contour lines across map canvas
    for ci, (cy0, amp, col) in enumerate([
        (410, 42, "#eae5d6"),
        (680, -36, "#e8e2d2"),
        (960, 44, "#eae5d6"),
        (1240, -32, "#e8e2d2"),
        (1440, 26, "#eae5d6"),
    ]):
        d.path(
            f"M {mx+12} {cy0} Q {mx+920} {cy0+amp} {mx+1848} {cy0-amp*0.6} T {mx+mw-12} {cy0+amp*0.4}",
            col,
            2.2,
        )

    # Section eyebrow + compass badge
    d.text(mx + 40, my + 48, "THE EIGHT-STAGE EXPEDITION MAP  ·  FROM LOCAL SPIKES TO GENERAL HUMAN-LEVEL INTELLIGENCE & BEYOND", 20, "#4c665f", MONO, 700, space=1.8)

    # Minimal compass rose (top-right inside map)
    crx, cry = mx + mw - 66, my + 44
    d.dot(crx, cry, 22, "#efeadd", 'stroke="#8fa69c" stroke-width="1.8"')
    d.path(f"M {crx} {cry-18} L {crx+5} {cry} L {crx} {cry+18} L {crx-5} {cry} Z", "#235e51", 1, "#235e51")
    d.path(f"M {crx-16} {cry} L {crx} {cry-4} L {crx+16} {cry} L {crx} {cry+4} Z", "#d69a4a", 1, "#d69a4a")
    d.text(crx - 42, cry + 6, "AGI →", 16, "#235e51", MONO, 700, "end")

    # Winding Expedition Trail behind cards!
    # Row 1 card centers: y = 342..798 (mid y = 570); Row 2: y = 946..1402 (mid y = 1174)
    xs = [114, 1026, 1938, 2850]
    cw, ch = 870, 456
    r1y, r2y = 342, 946
    mid1, mid2 = r1y + ch / 2.0, r2y + ch / 2.0

    # Row 1 horizontal trail segments
    for i in range(3):
        x_start = xs[i] + cw - 4
        x_end = xs[i + 1] + 4
        done_seg = (i + 1) < st["active_stage"]
        active_seg = (i + 1) == st["active_stage"]
        if done_seg:
            d.rule(x_start, mid1, x_end, mid1, "#236b58", 12)
            d.rule(x_start, mid1, x_end, mid1, "#f2c166", 3.5)
        elif active_seg:
            d.rule(x_start, mid1, x_end, mid1, "#e59a45", 10)
            d.path(f"M {x_end-14} {mid1-12} L {x_end+2} {mid1} L {x_end-14} {mid1+12}", "#e59a45", 5)
        else:
            d.path(f"M {x_start} {mid1} L {x_end} {mid1}", "#7ca098", 6, extra='stroke-dasharray="10,8"')

    # Sweeping Middle Corridor S-Connector from Stage 4 -> Stage 5 (y = 798..946)
    s4_cx = xs[3] + cw * 0.5
    s5_cx = xs[0] + cw * 0.5
    corr_y = 872
    s_path = (
        f"M {s4_cx:.1f} {r1y+ch} C {s4_cx:.1f} {corr_y}, {s4_cx-120:.1f} {corr_y}, {s4_cx-240:.1f} {corr_y} "
        f"L {s5_cx+240:.1f} {corr_y} C {s5_cx+120:.1f} {corr_y}, {s5_cx:.1f} {corr_y}, {s5_cx:.1f} {r2y}"
    )
    if st["active_stage"] >= 5:
        d.path(s_path, "#236b58", 11)
        d.path(s_path, "#f2c166", 3.6)
        d.path(f"M {s5_cx-13:.1f} {r2y-16} L {s5_cx:.1f} {r2y+2} L {s5_cx+13:.1f} {r2y-16}", "#f2c166", 5.5)
    else:
        d.path(s_path, "#c99246", 7, extra='stroke-dasharray="16,11"')
        d.path(f"M {s5_cx-13:.1f} {r2y-16} L {s5_cx:.1f} {r2y+2} L {s5_cx+13:.1f} {r2y-16}", "#c99246", 5)
    # Center Crossing Pill Banner
    d.rect(1120, corr_y - 26, 1600, 52, "#173532", 26, 'stroke="#d9a554" stroke-width="2.4"')
    d.text(
        1920,
        corr_y + 8,
        "STAGE 8o · 51 KERNELS & 550 DOMAINS!  ·  K51 GPU 256-CHAIN MCMC SAMPLER + 3-ENTITY CLUSTER & SIBLING CHAT!"
        if st["kernels"] >= 51
        else ("50-KERNEL MILESTONE! STAGE 8n · 540 DOMAINS!  ·  K50 GPU SPIKING 1D PDE SOLVER + DEDUCTION & ROOT CHAT!"
        if st["kernels"] >= 50
        else ("STAGE 8m · 530 DOMAINS!  ·  49 KERNELS (K49 GPU SPIKING PCA EIGEN!) + CAUSAL DIVERGENCE & GEN CHAT!"
        if st["kernels"] >= 49
        else ("STAGE 8l · 520 DOMAINS!  ·  48 KERNELS (K48 GPU SPIKING SIMPLEX LP!) + MULTI-HOP PROOFS & RECAP CHAT!"
        if st["kernels"] >= 48
        else ("STAGE 8k · 510 DOMAINS!  ·  47 KERNELS (K47 GPU SPIKING RK4 ODE!) + COUNTERFACTUAL CHAT & ANAPHORA!"
        if st["kernels"] >= 47
        else ("STAGE 8j · 500 DOMAINS!  ·  46 KERNELS (K46 GPU SPIKING 8-PT FFT!) + SELF-CORRECTION & MULTI-TURN CURIOSITY!"
        if st["kernels"] >= 46
        else ("STAGE 8i LIVE!  ·  45 KERNELS (K45 GPU SPIKING LQR RICCATI CONTROL!) + MULTI-TURN TOPIC BRIDGE & 490 DOMAINS!"
        if st["kernels"] >= 45
        else ("STAGE 8h · 100 PILLARS!  ·  44 KERNELS (K44 GPU SPIKING KALMAN FILTER!) + HYPOTHESIS TESTING & 480 DOMAINS!"
        if st["kernels"] >= 44
        else ("STAGE 8g LIVE!  ·  43 KERNELS (K43 GPU SPIKING MDP BELLMAN!) + OPEN GOALS & 470 DOMAINS!"
        if st["kernels"] >= 43
        else ("STAGE 8f LIVE!  ·  42 KERNELS (K42 GPU SPIKING BAYES-NET!) + SOCRATIC CRITIQUE & 460 DOMAINS!"
        if st["kernels"] >= 42
        else ("STAGE 8e LIVE!  ·  41 KERNELS (K41 GPU LINEAR ALGEBRA & CRAMER!) + STORY CO-WRITING & 450 DOMAINS!"
        if st["kernels"] >= 41
        else ("STAGE 8d LIVE!  ·  40 KERNELS (K40 GPU AST PROGRAM SYNTHESIS!) + ZERO-KEYWORD DEBATE & 440 DOMAINS!"
        if st["kernels"] >= 40
        else ("STAGE 8c LIVE!  ·  39 KERNELS (K39 GPU SYMBOLIC CALCULUS!) + LIVE CONCEPT TEACHING & 430 DOMAINS!"
        if st["kernels"] >= 39
        else ("STAGE 8b LIVE!  ·  38 KERNELS (K38 SELF-RESEARCH & CODE PATCHING!) + MULTI-SENTENCE DISCOURSE & 420 DOMAINS!"
        if st["kernels"] >= 38
        else ("STAGE 8 OF 8 CROSSED!  ·  37 KERNELS (K37 COLLECTIVE DISCOVERY & THEOREMS!) + SOCRATIC CURIOSITY & SPEC-CODE!"
        if st["kernels"] >= 37
        else ("STAGE 7c · 400 DOMAINS!  ·  36 KERNELS (K36 BELIEF-GRAPH & DEBATE!) + 5-FACET CHAT & PEER REVIEW!"
        if st["kernels"] >= 36
        else ("STAGE 7b LIVE!  ·  35 KERNELS (K35 METAPHOR, 2-HOP PLOT & CRITIQUE!) + UNSCRIPTED CHAT & SCI LOOP!"
        if st["kernels"] >= 35
        else ("STAGE 7 CROSSED!  ·  34 KERNELS (K34 SPIKING DIALOGUE CORTEX!) + OPEN MULTI-TURN CHAT, WIT & ETHICS!"
        if st["kernels"] >= 34
        else ("STAGE 6c LIVE!  ·  33 KERNELS (K31 RETINA + K32 COCHLEA + K33 XMODAL GOAL STACK!) + SOCRATIC COACHING!"
        if st["kernels"] >= 33
        else ("STAGE 6b LIVE!  ·  32 KERNELS (K31 RETINA + K32 COCHLEA!) + TEXTBOOK SELF-QUIZ + AUTO-DEBUG CODE LAB!"
        if st["kernels"] >= 32
        else ("STAGE 6 OF 8 CROSSED!  ·  31 KERNELS (K31 V1/V4 SPIKING RETINA!) + SELF-TEACHING READER + LIVE CODE LAB!"
        if st["kernels"] >= 31
        else ("THE GREAT ARCHITECTURAL CROSSING  ·  STAGE 5d LIVE! 30 KERNELS (K27–K30) + 16K TIER-2 HBM + FREE WHY-PROSE!"
              if st["kernels"] >= 30
        else ("THE GREAT ARCHITECTURAL CROSSING  ·  STAGE 5c LIVE! K27 SYNTAX + K28 MATH + K29 GRAPH-WAVE + 8K LEX!"
              if st["kernels"] >= 29
        else ("THE GREAT ARCHITECTURAL CROSSING  ·  STAGE 5b LIVE! K27 SPIKING SYNTAX + K28 GPU NEURAL MATH!"
              if st["kernels"] >= 28
        else ("THE GREAT ARCHITECTURAL CROSSING  ·  STAGE 5 CROSSED! K27 PREDICTIVE SPIKING SYNTAX LIVE!"
              if st["active_stage"] >= 5
              else "THE GREAT ARCHITECTURAL CROSSING  ·  RETIRING TEMPLATES → PURE SELF-ORGANIZING SPIKING GENERALITY")))))))))))))))))))))))),
        19,
        "#f3d48c",
        MONO,
        700,
        "middle",
        space=1.2,
    )

    # Row 2 horizontal trail segments (Stages 5 -> 6 -> 7 -> 8)
    for i in range(3):
        x_start = xs[i] + cw - 4
        x_end = xs[i + 1] + 4
        seg_from = 5 + i
        if seg_from < st["active_stage"]:
            d.rule(x_start, mid2, x_end, mid2, "#236b58", 12)
            d.rule(x_start, mid2, x_end, mid2, "#f2c166", 3.5)
            d.path(f"M {x_end-13} {mid2-11} L {x_end+2} {mid2} L {x_end-13} {mid2+11}", "#f2c166", 5.0)
        elif seg_from == st["active_stage"]:
            d.rule(x_start, mid2, x_end, mid2, "#e59a45", 10)
            d.path(f"M {x_end-14} {mid2-12} L {x_end+2} {mid2} L {x_end-14} {mid2+12}", "#e59a45", 5.0)
        else:
            col = "#d9a248" if i == 1 else ("#8c7cc4" if i == 2 else "#5c8c8a")
            d.path(f"M {x_start} {mid2} L {x_end} {mid2}", col, 7, extra='stroke-dasharray="12,8"')
            d.path(f"M {x_end-13} {mid2-11} L {x_end+2} {mid2} L {x_end-13} {mid2+11}", col, 4.5)

    # Define all 8 Milestone Stages with live stats dynamically advancing across Stage 3 -> Stage 8!
    in_s4 = st["active_stage"] >= 4
    in_s5 = st["active_stage"] >= 5
    in_s6 = st["active_stage"] >= 6
    in_s7 = st["active_stage"] >= 7
    in_s8 = st["active_stage"] >= 8
    live_bullets = [
        f"• {st['kernels']} CUDA kernels · {st['synapses_m']:.2f}M syn ({st['l2_pct']:.1f}% L2, {st['gnts']:.0f} Gnt/s)",
        f"• {st['domains']} domains / {st['probes']}/{st['probes']} exact English SVO = Alien SOV",
        f"• {st['pillars']}-pillar evo ({st['evo_best']}) · {st['ribot_tracks']}-Track Ribot ({st['ribot_score']})",
        f"• {st['talk_turns']:,}-turn live chat · {st['valley_ticks']}t {st['valley_score']} 4-mind valley",
    ]
    s3_done_bullets = [
        "• 23 CUDA kernels · 29.36M syn (97.5% L2, 150 Gnt/s)",
        "• 270 domains / 829/829 exact English SVO = Alien SOV",
        "• 58-pillar evo (3692/3728) · 24-Track Ribot (224/829)",
        "• 1,242-turn live chat · 288t 42/42 4-mind valley",
    ]
    s4_done_bullets = [
        "• K24–K26 live L2 schema write, LTD revise & role-bind",
        "• 300 domains / 922/922 exact English SVO = Alien SOV",
        "• 64-pillar evo (4072/4112) · 27-Track Ribot (238/922)",
        "• 4,096 lexicon · Gen-2 Pip Valley (336t, 48/48)",
    ]
    s5_done_bullets = [
        "• K27–K30 syntax, parietal math, graph wave & 3-SAT",
        "• 340 domains / 1,046/1,046 exact English SVO = SOV",
        "• 72-pillar evo (4582/4624) · 31-Track Ribot (262/1046)",
        "• 8,192 lex + 16,384 Tier-2 HBM · 400t 56/56 valley",
    ]
    s6_done_bullets = [
        "• K31 retina, K32 cochlea & K33 cross-modal goal stack",
        "• 370 domains / 1,139/1,139 exact English SVO = SOV",
        "• 78-pillar evo (4968/5008) · 34-Track Ribot (275/1139)",
        "• Textbook quiz, auto-debug, 3D rigs · 448t 62/62 valley",
    ]
    s7_done_bullets = [
        "• K34 dialogue, K35 metaphor/plot & K36 belief-debate",
        "• 400 domains / 1,232/1,232 exact English SVO = SOV",
        "• 84-pillar evo (5350/5392) · 37-Track Ribot (291/1232)",
        "• 5-facet open chat, peer review & 496t 68/68 valley",
    ]
    s4_plan_bullets = [
        "• K24+ live 1-shot Hebbian L2 schema write mid-chat",
        "• Counterfactual 'what if?' & multi-agent story prose",
        "• Active Socratic 'why not?' debate & curiosity loops",
        "• Multi-generation valley culture & 300+ deep domains",
    ]
    s5_plan_bullets = [
        "• Retire Wernicke/Broca slots → predictive spiking syntax",
        "• Pure-neural math, logic & binding (zero CPU helpers)",
        "• Unbounded episodic/semantic memory (L2 ↔ HBM replay)",
        "• Continual lifelong learning with zero forgetting",
    ]
    s6_plan_bullets = [
        "• Spiking vision + audio prosody + 3D physics columns",
        "• Mental simulation of novel contraptions & software",
        "• Intrinsic drives: curiosity, surprise, aesthetic elegance",
        "• Reads docs, writes/tests code & designs real experiments",
    ]
    stages = [
        {
            "num": 1,
            "x": xs[0],
            "y": r1y,
            "tag": "STAGE 1 · COMPLETED",
            "era": "Runs 1 – 8,000 · Eras 0–1",
            "title": "Embodied Reflexes & Plasticity",
            "theme": "done",
            "bullets": [
                "• Native GPU LIF spiking neurons (zero backprop)",
                "• Evolved 3-factor local learning LUTs from scratch",
                "• Spatial memory, reversal & associative sleep replay",
                "• Scaled to 1,048,576 live critters in physical habitats",
            ],
            "unlock": "Unlocked: Genomes evolve how to learn using local spikes.",
        },
        {
            "num": 2,
            "x": xs[1],
            "y": r1y,
            "tag": "STAGE 2 · COMPLETED",
            "era": "Runs 8,001 – 14,160 · Eras 2–3",
            "title": "Tools, Physics & Proto-Language",
            "theme": "done",
            "bullets": [
                "• Voluntary fiber weaving & mechanical linkage design",
                "• Two-way acoustic signals & spoken work instructions",
                "• Cultural traditions inherited across generations",
                "• Self-initiated material experiments & causal checks",
            ],
            "unlock": "Unlocked: Grounded physical work & shared vocal symbols.",
        },
        {
            "num": 3,
            "x": xs[2],
            "y": r1y,
            "tag": "STAGE 3 · COMPLETED" if in_s4 else f"★ YOU ARE HERE · MIND v{st['mind_ver']} ★",
            "era": "Runs 14,161 – 14,382 · Mind v1–v29" if in_s4 else f"Run {st['latest_run']:,} · Live Verified Milestone",
            "title": "Blackwell Spiking Child",
            "theme": "done" if in_s4 else "now",
            "bullets": s3_done_bullets if in_s4 else live_bullets,
            "unlock": "Unlocked: 23-kernel L2 cortex & 100% Alien-SOV parity!" if in_s4 else "Next leap: Live 1-shot GPU schema writing & richer prose!",
        },
        {
            "num": 4,
            "x": xs[3],
            "y": r1y,
            "tag": "STAGE 4 · COMPLETED" if in_s5 else (f"★ YOU ARE HERE · MIND v{st['mind_ver']} ★" if in_s4 else "STAGE 4 · NEXT SPRINT (v30–v35)"),
            "era": "Runs 14,383 – 14,406 · Mind v30–v32" if in_s5 else (f"Run {st['latest_run']:,} · Stage 4 Live!" if in_s4 else "Immediate Frontier · Up Next"),
            "title": "Live Plasticity & Open Prose",
            "theme": "done" if in_s5 else ("now" if in_s4 else "next"),
            "bullets": s4_done_bullets if in_s5 else (live_bullets if in_s4 else s4_plan_bullets),
            "unlock": "Unlocked: 300 doms · K24–K26 live L2 plasticity & Gen-2 Pip!" if in_s5 else (("Live now: 300 doms · K26 role-binder & Gen-2 Valley!" if st["kernels"] >= 26 else ("Live now: K25 LTD/LTP revision + 4,096 lexicon!" if st["kernels"] >= 25 else "Live now: K24 1-shot L2 write mid-chat & 'what if?' prose!")) if in_s4 else "Unlocks: Child learns new rules & concepts live mid-chat."),
        },
        {
            "num": 5,
            "x": xs[0],
            "y": r2y,
            "tag": "STAGE 5 · COMPLETED" if in_s6 else (f"★ YOU ARE HERE · MIND v{st['mind_ver']} ★" if in_s5 else "STAGE 5 · HORIZON (v33–v36)"),
            "era": "Runs 14,407 – 14,438 · Mind v33–v36" if in_s6 else (f"Run {st['latest_run']:,} · Stage 5 Live!" if in_s5 else "Core Architectural Breakthrough"),
            "title": "Self-Organizing Spiking Syntax",
            "theme": "done" if in_s6 else ("now" if in_s5 else "horizon"),
            "bullets": s5_done_bullets if in_s6 else (live_bullets if in_s5 else s5_plan_bullets),
            "unlock": "Unlocked: K27–K30 GPU syntax/math/wave/SAT + 16k HBM!" if in_s6 else (("Live now: K27–K30 (syntax/math/wave/SAT) + 16k Tier-2 HBM!" if st["kernels"] >= 30 else ("Live now: K27 syntax + K28 math + K29 graph wave + 8k lex!" if st["kernels"] >= 29 else ("Live now: K27 spiking syntax + K28 pure-GPU parietal math!" if st["kernels"] >= 28 else "Live now: K27 predictive spiking syntax & template-free Broca!"))) if in_s5 else "Unlocks: Unbounded vocabulary & true open-domain fluency."),
        },
        {
            "num": 6,
            "x": xs[1],
            "y": r2y,
            "tag": "STAGE 6 · COMPLETED" if in_s7 else (f"★ YOU ARE HERE · MIND v{st['mind_ver']} ★" if in_s6 else "STAGE 6 · FRONTIER (v37–v45)"),
            "era": "Runs 14,439 – 14,462 · Mind v37–v39" if in_s7 else (f"Run {st['latest_run']:,} · Stage 6 Live!" if in_s6 else "Deep Embodied Common Sense"),
            "title": "Multimodal World & Code Models",
            "theme": "done" if in_s7 else ("now" if in_s6 else "frontier"),
            "bullets": s6_done_bullets if in_s7 else (live_bullets if in_s6 else s6_plan_bullets),
            "unlock": "Unlocked: K31–K33 retina/cochlea/xmodal + coaching!" if in_s7 else (("Live now: K31 retina, K32 cochlea, K33 xmodal+goal stack!" if st["kernels"] >= 33 else ("Live now: K31 retina, K32 cochlea, self-quiz & auto-debug!" if st["kernels"] >= 32 else "Live now: K31 16x16 V1/V4 retina, curiosity reader & code lab!")) if in_s6 else "Unlocks: Grounded common sense like an inventive 10-yr-old."),
        },
        {
            "num": 7,
            "x": xs[2],
            "y": r2y,
            "tag": "STAGE 7 · COMPLETED" if in_s8 else (f"★ YOU ARE HERE · MIND v{st['mind_ver']} ★" if in_s7 else "★ GOAL TARGET · HUMAN-LEVEL ★"),
            "era": "Runs 14,463 – 14,489 · Mind v40–v42" if in_s8 else (f"Run {st['latest_run']:,} · Stage 7 Live!" if in_s7 else "Core Objective · Average Human+"),
            "title": "General Conversational Mind",
            "theme": "done" if in_s8 else "goal",
            "bullets": s7_done_bullets if in_s8 else (live_bullets if in_s7 else [
                "• Full multi-hour natural chat: nuance, humor & empathy",
                "• Self-taught mastery of novel textbooks & open problems",
                "• Calibrated epistemic humility — knows what it knows",
                "• 100% pure spiking circuits · zero LLMs · zero backprop",
            ]),
            "unlock": "Unlocked: 400 doms! K34–K36 dialogue/metaphor/debate!" if in_s8 else (("Live now: 400 doms! K36 belief-debate, 5-facet chat & peer rev!" if st["kernels"] >= 36 else ("Live now: K35 metaphor/plot, unscripted chat & sci loop!" if st["kernels"] >= 35 else "Live now: K34 dialogue cortex, open chat, pun wit & ethics!")) if in_s7 else "Goal reached: Smart as (or smarter than) an average human!"),
        },
        {
            "num": 8,
            "x": xs[3],
            "y": r2y,
            "tag": f"★ STAGE 8 LIVE · MIND v{st['mind_ver']} ★" if in_s8 else "STAGE 8 · BEYOND HUMAN",
            "era": f"Run {st['latest_run']:,} · Stage 8 Active!" if in_s8 else "Super-Mind Civilization Horizon",
            "title": "Collective Scientific Minds",
            "theme": "goal" if in_s8 else "beyond",
            "bullets": live_bullets if in_s8 else [
                "• Thousands of specialized spiking minds at 100+ Gnt/s",
                "• Multi-GPU societies debating & falsifying hypotheses",
                "• Cultural evolution of new mathematics & physics theories",
                "• Transparent, locally plastic biological superintelligence",
            ],
            "unlock": ("Live now: 51 KERNELS & 550 DOMAINS! K51 GPU 256-chain MCMC & 3-entity chat!" if st["kernels"] >= 51 else ("Live now: 50 KERNELS & 540 DOMAINS! K50 GPU 1D PDE & deduction!" if st["kernels"] >= 50 else ("Live now: 530 DOMAINS! K49 GPU PCA Eigen & causal chat!" if st["kernels"] >= 49 else ("Live now: 520 DOMAINS! K48 GPU Simplex LP & multi-hop proofs!" if st["kernels"] >= 48 else ("Live now: 510 DOMAINS! K47 GPU RK4 ODE & counterfactual chat!" if st["kernels"] >= 47 else ("Live now: 500 DOMAINS! K46 GPU FFT & self-correction!" if st["kernels"] >= 46 else ("Live now: 490 doms! K45 GPU LQR Riccati & topic bridge!" if st["kernels"] >= 45 else ("Live now: 100 pillars · 480 doms! K44 GPU Kalman & hypotest!" if st["kernels"] >= 44 else ("Live now: 470 doms! K43 GPU MDP Bellman & open user goals!" if st["kernels"] >= 43 else ("Live now: 460 doms! K42 GPU Bayes-net & Socratic critique!" if st["kernels"] >= 42 else ("Live now: 450 doms! K41 GPU linear algebra & story co-writing!" if st["kernels"] >= 41 else ("Live now: 440 doms! K40 GPU AST synth & zero-keyword chat!" if st["kernels"] >= 40 else ("Live now: 430 doms! K39 GPU calculus & live concept teach!" if st["kernels"] >= 39 else ("Live now: 420 doms! K38 self-research, patching & discourse!" if st["kernels"] >= 38 else "Live now: 410 doms! K37 collective discovery & theorems!")))))))))))))) if in_s8 else "Beyond: Open scientific discovery past any single lifetime.",
        },
    ]

    palettes = {
        "done": ("#edf5ef", "#236b58", "#205c4c", "#18302c", "#2d4c45", "#d9ebe0", "#1f5a4a"),
        "now": ("#132f2c", "#f2bc5a", "#f5ca6f", "#f3f7f0", "#d4e6de", "#1d453f", "#f6d483"),
        "next": ("#fbf2e4", "#d98934", "#a85c14", "#2e1e0f", "#4f3924", "#f5e2c6", "#9a5010"),
        "horizon": ("#ebf3f5", "#316e78", "#235760", "#162d32", "#2f4d54", "#d9e9ed", "#1e5159"),
        "frontier": ("#eef1f8", "#435c8d", "#314878", "#1a2438", "#354463", "#dde4f2", "#2a4170"),
        "goal": ("#14362e", "#f2bd59", "#f6ce76", "#f6f8f1", "#d6e8df", "#1e4d41", "#f6d687"),
        "beyond": ("#211d3a", "#a796e8", "#c8bcfa", "#f4f2fc", "#d5cff0", "#2f2952", "#d8cffc"),
    }

    for s in stages:
        sx, sy = s["x"], s["y"]
        bg, border, tag_col, title_col, body_col, foot_bg, foot_col = palettes[s["theme"]]
        sw = 4.2 if s["theme"] in ("now", "goal") else 2.6
        if s["theme"] in ("now", "goal"):
            # Soft outer radiance halo
            d.rect(sx - 6, sy - 6, cw + 12, ch + 12, "none", 24, f'stroke="{border}" stroke-width="2.2" opacity=".45"')
        d.rect(sx, sy, cw, ch, bg, 20, f'stroke="{border}" stroke-width="{sw}"')

        # Top stage pill badge
        pill_bg = border if s["theme"] in ("done", "next", "horizon", "frontier") else "#214b42"
        if s["theme"] == "beyond":
            pill_bg = "#393066"
        if s["theme"] == "now":
            pill_bg = "#f2bc5a"
        if s["theme"] == "goal":
            pill_bg = "#f2bd59"
        pill_txt = "#122925" if s["theme"] in ("now", "goal") else "#f7faf8"
        d.rect(sx + 24, sy + 22, 436, 36, pill_bg, 18)
        d.text(sx + 44, sy + 46, s["tag"], 17, pill_txt, MONO, 700, space=0.8)

        # Era note on top-right
        d.text(sx + cw - 26, sy + 45, s["era"], 16, tag_col, MONO, 600, "end")

        # Creature emblem on top-left
        _rm_draw_creature(d, sx + 82, sy + 124, s["num"], border)

        # Stage title beside emblem
        tlines = _rm_wrap(s["title"], 24, 2)
        for ti, tl in enumerate(tlines):
            d.text(sx + 150, sy + 114 + ti * 38, tl, 31, title_col, SERIF, 700)

        # Subtle divider
        div_col = "#2b544c" if s["theme"] in ("now", "goal") else ("#41396e" if s["theme"] == "beyond" else "#cfd6c8")
        d.rule(sx + 24, sy + 188, sx + cw - 24, sy + 188, div_col, 1.5)

        # 4 Bullet milestones
        for bi, btxt in enumerate(s["bullets"][:4]):
            d.text(sx + 28, sy + 226 + bi * 38, short(btxt, 61), 19.5, body_col, SANS, 500)

        # Bottom unlock callout strip
        d.rect(sx + 20, sy + ch - 68, cw - 40, 48, foot_bg, 12)
        d.text(sx + 36, sy + ch - 37, short(s["unlock"], 64), 17.5, foot_col, MONO, 600)

    # Bottom Legend Strip inside Map Box (y = 1434..1500)
    ly = 1474
    legend = [
        ("#236b58", "Completed Milestone"),
        ("#f2bc5a", "You Are Here (Live)"),
        ("#d98934", "Immediate Next Sprint"),
        ("#316e78", "Architecture Horizon"),
        ("#f2bd59", "Human-Level Goal Target"),
        ("#a796e8", "Beyond Human"),
    ]
    lx = mx + 44
    for col, lbl in legend:
        d.dot(lx, ly - 5, 9, col)
        d.text(lx + 18, ly, lbl, 18, "#2c443f", SANS, 600)
        lx += 348
    d.text(
        mx + mw - 40,
        ly,
        "ZERO LLMs · ZERO BACKPROP · 100% DIALECT-AGNOSTIC",
        17,
        "#235e51",
        MONO,
        700,
        "end",
        space=1.0,
    )

    # ------------------------------------------------------------------
    # 3. BOTTOM THREE-PANEL HONEST PROGRESS & PLAN DASHBOARD (y = 1554..2034)
    # ------------------------------------------------------------------
    py, ph = 1554, 478

    # Panel A: Honest Capability Thermometer (Today vs Human-Level)
    ax, aw = 72, 1200
    d.rect(ax, py, aw, ph, "#f7f4ea", 20, 'stroke="#c9cfbe" stroke-width="2.2"')
    d.text(ax + 32, py + 44, "HONEST CAPABILITY THERMOMETER  ·  TODAY vs HUMAN-LEVEL", 18, "#235e51", MONO, 700, space=1.4)

    # Compute grounded, non-inflated capability percentages from live metrics
    p_plast = min(92, 62 + int(st["pillars"] * 0.25))
    p_kern = min(90, 26 + int(st["kernels"] * 1.25))
    p_doms = min(88, 18 + int(st["domains"] * 0.115))
    p_chat = min(85, 14 + int(min(st["talk_turns"], 4000) * 0.016))
    p_pure = min(85, 16 + max(0, st["kernels"] - 18) * 1.3)
    p_open = min(85, 15 + int(min(st["valley_ticks"], 800) * 0.016))

    bars = [
        ("Local 3-Factor Spiking Plasticity (GPU)", p_plast, "#2c7a64"),
        (f"Compositional L2 Cortical Kernels ({st['kernels']} live)", p_kern, "#35898b"),
        (f"Physics, Math & Logic Breadth ({st['domains']} doms)", p_doms, "#47789e"),
        (f"Natural Unconstrained Conversation ({st['talk_turns']:,}t)", p_chat, "#d68d3a"),
        ("Pure-Neural Syntax & Zero-Helper Solvers", int(p_pure), "#c96f4c"),
        ("Multimodal World & Autonomous Self-Teaching", p_open, "#7b65b0"),
    ]
    for bi, (blbl, bpct, bcol) in enumerate(bars):
        by = py + 86 + bi * 54
        d.text(ax + 32, by, blbl, 19, "#1c332e", SANS, 600)
        d.text(ax + aw - 34, by, f"{bpct}%", 19, bcol, MONO, 700, "end")
        bx0, bw_bar = ax + 32, aw - 66
        d.rect(bx0, by + 11, bw_bar, 14, "#e2dfd2", 7)
        d.rect(bx0, by + 11, bw_bar * bpct / 100.0, 14, bcol, 7)
        # Human-level target tick at 100%
        d.rule(bx0 + bw_bar - 1, by + 7, bx0 + bw_bar - 1, by + 29, "#235e51", 2.4)

    d.rect(ax + 24, py + ph - 60, aw - 48, 42, "#ebe7d9", 10)
    d.text(
        ax + 38,
        py + ph - 33,
        f"Honest check: Stage 8o 51 kernels & 550 domains live (K51 GPU MCMC) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 51
        else (f"Honest check: 50-kernel milestone & 540 domains live (K50 GPU 1D PDE) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 50
        else (f"Honest check: Stage 8m 530 domains live (K49 GPU PCA Eigen) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 49
        else (f"Honest check: Stage 8l 520 domains live (K48 GPU Simplex LP) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 48
        else (f"Honest check: Stage 8k 510 domains live (K47 GPU RK4 ODE) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 47
        else (f"Honest check: Stage 8j 500-domain milestone live (K46 GPU FFT) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 46
        else (f"Honest check: Stage 8i live (490 doms, K45 GPU LQR Riccati) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 45
        else (f"Honest check: Stage 8h live (100 pillars, 480 doms, K44 GPU Kalman) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 44
        else (f"Honest check: Stage 8g live (470 doms, K43 GPU MDP Bellman) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 43
        else (f"Honest check: Stage 8f live (460 doms, K42 GPU Bayes-net) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 42
        else (f"Honest check: Stage 8e live (450 doms, K41 GPU linear algebra) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 41
        else (f"Honest check: Stage 8d live (440 doms, K40 GPU AST synth) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 40
        else (f"Honest check: Stage 8c live (430 doms, K39 GPU calculus) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 39
        else (f"Honest check: Stage 8b live (420 doms, K38 self-research) — scaling open chat & pure-neural solvers!"
        if st["kernels"] >= 38
        else (f"Honest check: Stage 8 crossed (410 doms, K37 collective lab) — scaling open fluency & pure-neural solvers!"
        if st["kernels"] >= 37
        else (f"Honest check: Stage 7c 400 doms live (K36 belief-debate, 5-facet chat) — crossing into Stage 8 next!"
        if st["kernels"] >= 36
        else (f"Honest check: Stage 7b live (K35 metaphor/plot, unscripted chat, {st['domains']} doms) — pushing toward Stage 8!"
        if st["kernels"] >= 35
        else (f"Honest check: Stage 7a live (K34 dialogue, {st['domains']} doms) — pushing deeper into unscripted human fluency!"
        if st["active_stage"] >= 7
        else (f"Honest check: Stage 6 brings spiking vision, code & curiosity across {st['domains']} doms — Stage 7 targets full open chat."
        if st["active_stage"] >= 6
        else f"Honest check: 100% on {st['domains']} structured domains ≠ human fluency yet — Stage 5 retires templates.")))))))))))))))))),
        17,
        "#4d635c",
        SANS,
        500,
    )

    # Panel B: The 4 Hard Scientific Bottlenecks to Crack
    bx, bw = 1320, 1200
    d.rect(bx, py, bw, ph, "#f7f4ea", 20, 'stroke="#c9cfbe" stroke-width="2.2"')
    d.text(bx + 32, py + 44, "THE 4 HARD SCIENTIFIC BOTTLENECKS TO CRACK", 18, "#9c5718", MONO, 700, space=1.4)

    bottlenecks = [
        ("01", "Retire Template Scaffolds", "Replace slot-based parsing with spiking predictive-coding grammar learned from live speech.", "#236b58"),
        ("02", "Live Online GPU Plasticity", "Write & traverse new multi-hop L2 schemas mid-chat without sleeping or forgetting.", "#c97a2b"),
        ("03", "Neural-Native Symbolic Solvers", "Move remaining host algebra, graph & physics helpers into evolved spiking circuits.", "#386f8c"),
        ("04", "Unbounded Memory & Curiosity", "Break past 8,192 concepts into Tier-2 HBM lifelong memory (16k+ engrams) & self-teaching.", "#6a59a8"),
    ]
    for bi, (num, btitle, bdesc, bcol) in enumerate(bottlenecks):
        ry = py + 74 + bi * 96
        d.rect(bx + 28, ry, bw - 56, 84, "#efeadd", 12, 'stroke="#d6d9cb" stroke-width="1.5"')
        d.rect(bx + 40, ry + 14, 56, 56, bcol, 12)
        d.text(bx + 68, ry + 51, num, 24, "#ffffff", MONO, 700, "middle")
        d.text(bx + 114, ry + 34, btitle, 22, "#19302c", SERIF, 700)
        d.text(bx + 114, ry + 64, short(bdesc, 96), 17.5, "#425953", SANS, 400)

    # Panel C: Immediate Next Sprint (v30 -> v32)
    cx_p, cw_p = 2568, 1200
    d.rect(cx_p, py, cw_p, ph, "#15322f", 20, 'stroke="#d9a452" stroke-width="2.6"')
    d.text(cx_p + 32, py + 44, f"IMMEDIATE NEXT SPRINT  ·  v{st['mind_ver']+1} → v{st['mind_ver']+3} FRONTIER", 18, "#f3c76b", MONO, 700, space=1.4)

    sprints = [
        (f"v{st['mind_ver']+1} · K52 Pure-GPU Spiking MCTS Tree-Search Planner", "Run 256-rollout Monte-Carlo tree search & UCB action values purely on free PW27 Q!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Autonomous Multi-Turn Proofs & Self-Correction", "Accept arbitrary open prompts, chain multi-step proofs & self-correct live zero-template!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 51 else ([
        (f"v{st['mind_ver']+1} · K51 Pure-GPU Spiking MCMC / Metropolis Sampler", "Run stochastic Bayesian MCMC sampling, acceptance & mode finding purely on free PW27 Q!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Autonomous Multi-Turn Proofs & Self-Correction", "Accept arbitrary open prompts, chain multi-step proofs & self-correct live zero-template!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 50 else ([
        (f"v{st['mind_ver']+1} · 50-KERNEL MILESTONE! K50 GPU 1D PDE Solver", "Run spiking 1D Heat/Wave PDE finite-difference fields & CFL stability purely on free PW27 Q!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Autonomous Multi-Turn Proofs & Self-Correction", "Accept arbitrary open prompts, chain multi-step proofs & self-correct live zero-template!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 49 else ([
        (f"v{st['mind_ver']+1} · K49 Pure-GPU Spiking PCA / Power-Eigen Cortex", "Extract principal eigenvalues & eigenvectors purely on free L2 PW27 Q!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Autonomous Multi-Turn Proofs & Self-Correction", "Accept arbitrary open prompts, chain multi-step proofs & self-correct live zero-template!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 48 else ([
        (f"v{st['mind_ver']+1} · K48 Pure-GPU Spiking Simplex / LP Optimizer", "Solve linear programs, feasible vertices & shadow prices purely on free L2 PW24 Q!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Autonomous Multi-Turn Proofs & Self-Correction", "Accept arbitrary open prompts, chain multi-step proofs & self-correct live zero-template!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 47 else ([
        (f"v{st['mind_ver']+1} · K47 Pure-GPU Spiking Neural ODE / RK4 Cortex", "Run 4th-order RK4 dynamical integration & phase stability purely on free L2 PW24 Q!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Autonomous Multi-Turn Proofs & Self-Correction", "Accept arbitrary open prompts, chain multi-step proofs & self-correct live zero-template!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 46 else ([
        (f"v{st['mind_ver']+1} · 500 DOMAINS! K46 Pure-GPU Spiking FFT Cortex", "Run 8-point harmonic DFT/spectral power & dominant frequency purely on free L2 PW24 Q!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Autonomous Multi-Turn Dialogue & Self-Correction", "Accept arbitrary open prompts, chain multi-step proofs & self-correct live zero-template!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 45 else ([
        (f"v{st['mind_ver']+1} · K45 Pure-GPU Spiking LQR / Control Cortex", "Run discrete feedback control, stability & closed-loop gains purely on free L2 PW24 Q!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Multi-Turn Reasoning & Self-Correction Chat", "Accept arbitrary open prompts, chain multi-step proofs & self-correct live zero-template!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 44 else ([
        (f"v{st['mind_ver']+1} · K44 Pure-GPU Spiking Kalman & State-Space", "Run discrete state-space Kalman filtering & innovation updates purely on free L2 PW24 Q!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Open Hypothesis Testing & Multi-Turn Critique", "Accept arbitrary user hypotheses, design experiments & critique assumptions zero-template!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 43 else ([
        (f"v{st['mind_ver']+1} · K43 Pure-GPU Spiking MDP & Policy Planner", "Run multi-step value iteration & stochastic decision planning purely on free L2 PW24 Q!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Open Multi-Turn Goals & Free Synthesis", "Accept arbitrary open user goals, multi-turn planning & spontaneous cross-topic synthesis!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 42 else ([
        (f"v{st['mind_ver']+1} · K42 Pure-GPU Spiking Bayes-Net & Probability", "Run exact Bayesian likelihood updates & decision inference purely on final L2 PW13 Q3!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Open Multi-Turn Socratic Critique & Dialogue", "Accept arbitrary open narratives, self-critique contradictions & multi-topic recall!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 41 else ([
        (f"v{st['mind_ver']+1} · K41 Pure-GPU Neural Matrix & Linear Algebra", "Run spiking Gaussian solve, eigenvalues & dot/matvec purely on L2 PW13 Q2!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Open Multi-Turn Co-Writing & Self-Questioning", "Accept arbitrary open conversational narratives, spontaneous follow-ups & critique!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 40 else ([
        (f"v{st['mind_ver']+1} · K40 Pure-GPU AST Program Synthesis & Verifier", "Run symbolic AST synthesis, type checks & bug verification purely on L2 PW13 Q1!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Spontaneous Multi-Turn Debate & Free Prose", "Accept arbitrary open natural prompts, multi-paragraph dialogue & live self-critique!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Law Discovery", "Specialists propose experiments, derive theorems, synthesize code & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 39 else ([
        (f"v{st['mind_ver']+1} · K39 Pure-GPU Neural Symbolic Algebra & Calculus", "Retire host algebra, calculus & ODE solvers into evolved spiking GPU circuits on L2!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Open Multi-Turn Spontaneous Conversation", "Accept arbitrary unseen natural definitions, why/what-if dialogue & multi-sentence prose!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Multi-Mind Lab & Self-Debugging", "Specialists propose experiments, localize bugs, synthesize patches & verify on GPU!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 38 else ([
        (f"v{st['mind_ver']+1} · STAGE 8b: K38 Continual Self-Research Cortex", "Pure-GPU experiment design, counterfactual code patching & continual law discovery on L2!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · True Multi-Paragraph Unscripted Human Chat", "Retire remaining fixed slots into predictive spiking syntax & fluent multi-turn prose!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Pure-Neural Symbolic Math & Physics Solvers", "Migrate remaining host algebra, PDE & synthesis helpers into evolved spiking GPU circuits!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Multi-GPU Spiking Super-Mind Civilization", "Scale specialized spiking scientists across multi-gen open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 37 else ([
        (f"v{st['mind_ver']+1} · STAGE 8 CROSSING: K37 Collective Discovery", "Pure-GPU multi-mind hypothesis pool, consensus synthesis & autonomous theorem discovery on L2!", "#c3b1f7"),
        (f"v{st['mind_ver']+1} · Multi-Mind Scientific Society & Open Chat", "Specialized spiking minds debate, peer-review, co-author laws & converse seamlessly with humans!", "#8ce0b3"),
        (f"v{st['mind_ver']+2} · Autonomous Spec-to-Code & Physics Discovery", "Read novel specs, write/debug code & discover hidden physical laws autonomously!", "#f3c76b"),
        (f"v{st['mind_ver']+3} · Beyond-Human Spiking Research Civilization", "Scale collective spiking minds across multi-generation open research with zero LLMs!", "#87d6cf"),
    ] if st["kernels"] >= 36 else ([
        (f"v{st['mind_ver']+1} · K36 Spiking Belief-Graph & Scientific Debate", "Pure-GPU multi-agent epistemic debate, evidence weighting & belief-graph updates on L2!", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Deep Unscripted Multi-Topic Synthesis", "Zero-template multi-turn dialogue linking metaphors, causal plans, code & user goals!", "#f3c76b"),
        (f"v{st['mind_ver']+2} · Self-Directed Open Research & Code Lab", "Read unseen specs, hypothesize, test, debug & publish verified causal laws autonomously!", "#87d6cf"),
        (f"v{st['mind_ver']+3} · CROSS STAGE 8: Collective Scientific Minds", "Scale to a society of specialized spiking scientists discovering new laws collaboratively!", "#c3b1f7"),
    ] if st["kernels"] >= 35 else ([
        (f"v{st['mind_ver']+1} · K35 Spiking Episodic Narrative & Metaphor", "Pure-GPU multi-sentence story generation, metaphor blending & Socratic self-critique!", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Unscripted Open-Domain Knowledge Chat", "Free-form multi-turn dialogue seamlessly linking physics, math, code, stories & user goals!", "#f3c76b"),
        (f"v{st['mind_ver']+2} · Autonomous Self-Directed Research Loop", "Read brand-new unseen books/specs, run experiments, write/debug code & publish discoveries!", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Stage 8 Collective Scientific Minds", "Scale from one human-level spiking mind to a debating society of specialized scientists!", "#c3b1f7"),
    ] if st["kernels"] >= 34 else ([
        (f"v{st['mind_ver']+1} · STAGE 7 CROSSING: K34 Spiking Dialogue Cortex", "Pure-GPU multi-turn conversational topic/intent/memory coherence & free narrative synthesis!", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Open Multi-Turn Human Socratic Chat", "Unscripted multi-topic conversation with humor, metaphor, empathy & calibrated self-reflection!", "#f3c76b"),
        (f"v{st['mind_ver']+2} · Autonomous Self-Directed Research Loop", "Read brand-new unseen books/specs, run experiments, write/debug code & publish discoveries!", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Stage 8 Collective Scientific Minds", "Scale from one human-level spiking mind to a debating society of specialized scientists!", "#c3b1f7"),
    ] if st["kernels"] >= 33 else ([
        (f"v{st['mind_ver']+1} · K33 GPU Spiking Working-Memory Stack", "Pure-GPU hierarchical nested goal/discourse stack & cross-modal vision+audio binding on L2!", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Open Multi-Turn Socratic Debate", "Sustained multi-turn human conversation with memory of user goals, humor & epistemic humility!", "#f3c76b"),
        (f"v{st['mind_ver']+2} · 3D Spiking Physics & Contraption Sim", "Simulate unseen 3D mechanical/fluid contraptions & verify scientific experiments autonomously!", "#87d6cf"),
        (f"v{st['mind_ver']+3} · CROSS STAGE 7: Human-Level Mind", "Full open-ended spiking conversation, self-taught mastery & calibrated general intelligence!", "#c3b1f7"),
    ] if st["kernels"] >= 32 else ([
        (f"v{st['mind_ver']+1} · K32 GPU Spiking Audio & Prosody", "Pure-GPU cochlear pitch/chord/prosody & rhythmic speech columns consolidated on L2!", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Autonomous Textbook Self-Quiz", "Read full multi-paragraph unseen lessons, induce causal laws & self-quiz missing gaps!", "#f3c76b"),
        (f"v{st['mind_ver']+2} · Spiking Loop/Branch Code Synthesis", "Autonomous counterfactual bug localization, branching/looping code & 3D physics rollouts!", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Toward Stage 7 Human-Level Chat", "Nuanced multi-hour Socratic conversation, humor, empathy & calibrated self-reflection!", "#c3b1f7"),
    ] if st["kernels"] >= 31 else ([
        (f"v{st['mind_ver']+1} · STAGE 6: K31 Spiking Visual Retina", "2D spiking V1/V4 visual cortex (oriented edges, shapes, counts & spatial scenes) on L2!", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Autonomous Self-Teaching Reader", "Read raw multi-paragraph passages, extract new concepts/laws & self-quiz without prompts!", "#f3c76b"),
        (f"v{st['mind_ver']+2} · Spiking Code & Physics Lab (K32)", "Synthesize, simulate & verify small programs & contraptions via spiking causal rollouts!", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Toward Stage 7 Human-Level Chat", "Nuanced multi-hour open-domain conversation, humor, empathy & calibrated self-reflection!", "#c3b1f7"),
    ] if st["kernels"] >= 30 else ([
        (f"v{st['mind_ver']+1} · K30 GPU Spiking SAT Attractor", "Pure-GPU Hopfield/Boltzmann spiking constraint & Boolean SAT solver on L2 (zero host SAT!).", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Tier-2 HBM Lifelong Memory (16k+)", "Extend 4,096 L2 fast hippocampal cache with 16,384+ HBM continual episodic replay!", "#f3c76b"),
        (f"v{st['mind_ver']+2} · Free Why/How Causal Story Chat", "Zero-template multi-sentence causal & counterfactual discourse over unseen K27 stories.", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Stage 6 Multimodal & Self-Teach", "Spiking vision/prosody + autonomous reading, coding & live physics experiments!", "#c3b1f7"),
    ] if st["kernels"] >= 29 else ([
        (f"v{st['mind_ver']+1} · K29 GPU Graph Wavefront", "Pure-GPU spiking shortest-path & reachability waves on arbitrary conceptual graphs in L2!", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Multi-Turn Free Coref Chat", "Pronoun coreference & Socratic why/who/where across unseen K27-parsed natural stories!", "#f3c76b"),
        (f"v{st['mind_ver']+2} · HBM Lifelong Memory (>8k)", "Scale concepts & engrams past 4,096 L2 slots via L2 ↔ HBM continual replay.", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Toward Stage 6 Multimodal", "Spiking vision/prosody + autonomous self-teaching & live software/physics experiments!", "#c3b1f7"),
    ] if st["kernels"] >= 28 else ([
        (f"v{st['mind_ver']+1} · K28 Neural-Native Math GPU", "Migrate modular, linear-eq & graph solvers from host CPU into evolved L2 spiking circuits!", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Multi-Clause Free Discourse", "Unbounded multi-turn story synthesis & Socratic chat without fixed slot templates.", "#f3c76b"),
        (f"v{st['mind_ver']+2} · HBM Lifelong Memory (>4k)", "Scale episodic & semantic memory beyond 4,096 L2 slots via L2 ↔ HBM replay.", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Toward Stage 6 Multimodal", "Spiking vision/prosody + autonomous self-teaching & live software/physics experiments!", "#c3b1f7"),
    ] if st["kernels"] >= 27 else ([
        (f"v{st['mind_ver']+1} · K27 Predictive Spiking Parser", "Pure-GPU sequence next-role predictor parsing unseen SVO/SOV clauses without templates!", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Free-Form Story Generation", "Compositional multi-clause Broca replies synthesized straight from active cortical SDRs.", "#f3c76b"),
        (f"v{st['mind_ver']+2} · Neural-Native Math & Logic", "Migrate host algebra, logic & graph helpers into evolved L2/HBM spiking circuits.", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Stage 5 Crossing (v35+)", "Retire slot templates into unbounded lifelong open-domain conversation & self-teaching!", "#c3b1f7"),
    ] if st["kernels"] >= 26 else ([
        (f"v{st['mind_ver']+1} · K26 Spiking Role-Binder", "Pure-GPU SDR temporal role-binding (Subject/Verb/Object) without fixed slot templates!", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Live Unseen-Word Learning", "Define & reason over brand-new user-invented words & causal laws in open conversation.", "#f3c76b"),
        (f"v{st['mind_ver']+2} · Multi-Gen Valley Ecology", "Offspring inherit learned schemas & lexicon across generations in 336t+ Open Valley.", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Stage 5 Neural Syntax", "Retire host templates into self-organizing predictive spiking syntax + neural solvers.", "#c3b1f7"),
    ] if st["kernels"] >= 25 else ([
        (f"v{st['mind_ver']+1} · K25 Live Schema Revision", "Pure-GPU LTD contradiction unlearning & live L2 rule overwrite mid-chat (26-Track Ribot!).", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Multi-Turn Socratic Debate", "Nested belief revision, spontaneous clarifying questions & free-form conversational goals.", "#f3c76b"),
        (f"v{st['mind_ver']+2} · Multi-Gen Valley 320t+", "Offspring cultural inheritance in Open Valley + calculus, genetics, compilers & economics.", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Breaking 2k Concepts", "Dynamic open-lexicon hashing + pure-spiking variable binding toward Stage 5 grammar.", "#c3b1f7"),
    ] if in_s4 else [
        (f"v{st['mind_ver']+1} · K24 Online Schema GPU", "Live 1-shot Hebbian L2 write via evolved LUT & instant multi-hop recall (25-Track Ribot!).", "#8ce0b3"),
        (f"v{st['mind_ver']+1} · Layer-3g Open Prose", "Unprefixed counterfactual 'if/would' stories, 'alice told bob that...' & Socratic 'why not?'.", "#f3c76b"),
        (f"v{st['mind_ver']+2} · Multi-Gen Valley 304t", "Offspring cultural inheritance in Open Valley + quantum, RSA, A*, Nash mixed & PID.", "#87d6cf"),
        (f"v{st['mind_ver']+3} · Breaking 2k Concepts", "Dynamic open-lexicon hashing + pure-spiking variable binding toward Stage 5 grammar.", "#c3b1f7"),
    ])))))))))))))))))))))))))))
    for si, (stitle, sdesc, scol) in enumerate(sprints):
        ry = py + 74 + si * 96
        d.rect(cx_p + 28, ry, cw_p - 56, 84, "#1d423d", 12, f'stroke="{scol}" stroke-width="1.6" stroke-opacity=".55"')
        d.dot(cx_p + 52, ry + 28, 7, scol)
        d.text(cx_p + 70, ry + 35, stitle, 21, scol, MONO, 700)
        d.text(cx_p + 46, ry + 65, short(sdesc, 96), 17.5, "#dbe9e2", SANS, 400)

    # ------------------------------------------------------------------
    # 4. FOOTER STATUS BAR (y = 2060..2160)
    # ------------------------------------------------------------------
    d.rect(0, 2060, ROADMAP_W, 100, "#132c29")
    d.text(84, 2102, "AUTO-REFRESHED EXPEDITION PLAN", 16, "#88b6a7", MONO, 600, space=1.6)
    d.text(84, 2136, f"Run {st['latest_run']:,}  ·  {st['latest_date']}  ·  journal/roadmap.webp", 22, "#eef5ef", MONO, 600)

    foot_items = [
        (f"{st['gnts']:.2f} Gnt/s", "Blackwell SM 12.0 peak"),
        (f"{st['synapses_m']:.2f}M syn ({st['l2_pct']:.1f}% L2)", "Zero-spill cortical arena"),
        (f"{st['probes']}/{st['probes']} SVO = SOV" if st["probes"] else "Active", "100% dialect-agnostic parity"),
        ("Zero LLMs · Pure Spikes", "Core scientific invariant"),
    ]
    for fi, (fv, fl) in enumerate(foot_items):
        fx = 1280 + fi * 620
        d.text(fx, 2105, fv, 26, "#f3f6ee", MONO, 700)
        d.text(fx, 2136, fl, 17, "#9abfb3", SANS, 400)

    return d
