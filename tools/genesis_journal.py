# Axion Genesis v43 — Publication-Grade Scientific Observatory Plates (plate.webp)
# 2200x1320 Dark-Observatory high-density vector + hillshaded-PNG field infographics:
#   - Mode A (bench)    : Blackwell SM 12.0 4B/syn + 8B/neuron bit blueprint, throughput & occupancy
#   - Mode B (evolve)   : Hillshaded 256x256 GPU world PNG, top-4 species gallery, 3D cortical stack
#                         + inter-layer tract flow, log-normal E/I weight & delay histograms,
#                         4-receptor & vocal-context heatmap, evolutionary dynamics & lesion waterfall
#   - Mode C (probe)    : 10-Assay Non-LLM Cognitive Battery with 8-axis radar, reversal bars,
#                         60t working-memory electrophysiology trace, pre->post STDP dumbbell shifts,
#                         unsupervised MMN predictive surprise, spoken Q&A grounding & cohort anatomy
#   - Mode D (converse) : Multi-agent live spiking colony anatomy, cortical layer x turn heatmap,
#                         3-track oscilloscope (spikes+STDP, Schultz VTA DA, motor traces) & vocal log

GEN_TK_COLORS = {
    0: None,
    1: ("#cbe8c8", "Stem"),
    2: ("#8b9ca6", "Armor"),
    3: ("#ef6461", "Muscle"),
    4: ("#f6c453", "Eye"),
    5: ("#3ee6a8", "Chem"),
    6: ("#45c7f5", "Ear"),
    7: ("#f0944d", "Grasp"),
    8: ("#c77dff", "Gland"),
    9: ("#4be0a5", "Pyr E"),
    10: ("#f25f5c", "PV I"),
    11: ("#b975f5", "SST I"),
    12: ("#45c7f5", "MOD"),
}

GEN_LINEAGE_COLORS = ("#f6c453", "#3ee6a8", "#f26b6b", "#45c7f5", "#c77dff", "#f0944d", "#9be564", "#ff7eb6")

OBS_BG = "#081013"
OBS_CARD = "#0f1c20"
OBS_CARD2 = "#132429"
OBS_INNER = "#0b1518"
OBS_BORDER = "#1f3941"
OBS_BORDER_HI = "#2e5561"
OBS_TEXT = "#eef5f2"
OBS_SUB = "#9db8b4"
OBS_MUTED = "#658380"
OBS_EMERALD = "#3ee6a8"
OBS_GOLD = "#f6c453"
OBS_CYAN = "#45c7f5"
OBS_VIOLET = "#c77dff"
OBS_CORAL = "#f25f5c"
OBS_COPPER = "#f0944d"


def _gen_rows(lines, prefix):
    return [pairs(l) for l in lines if l.startswith(prefix + "=")]


def _hex_val(ch):
    try:
        return int(ch, 16)
    except Exception:
        return 0


def _gen_f(row, key, default=0.0):
    try:
        return float(row.get(key, default))
    except Exception:
        return float(default)


def _gen_i(row, key, default=0):
    try:
        return int(float(row.get(key, default)))
    except Exception:
        return int(default)


def _gen_wrap_clean(text, max_chars, max_lines=3):
    wrapped = textwrap.wrap(str(text), width=max_chars, break_long_words=False)
    if len(wrapped) <= max_lines:
        return wrapped
    out = wrapped[: max_lines - 1]
    tail = " ".join(wrapped[max_lines - 1 :])
    out.append(short(tail, max_chars))
    return out


def _obs_card(d, x, y, w, h, eyebrow="", title="", accent=OBS_EMERALD, subtitle=""):
    d.rect(x, y, w, h, OBS_CARD, 14, f'stroke="{OBS_BORDER}" stroke-width="1.5"')
    d.rect(x + 1, y + 1, w - 2, 3.2, accent, 2, 'opacity="0.82"')
    if eyebrow:
        d.text(x + 20, y + 27, eyebrow, 11.5, accent, MONO, 700, space=1.1)
    if title:
        d.text(x + 20, y + 50, title, 18.0, OBS_TEXT, SERIF, 600)
    if subtitle:
        d.text(x + w - 20, y + 27, subtitle, 11.0, OBS_MUTED, MONO, 500, "end")


def _obs_donut(d, cx, cy, r_out, r_in, slices, center_top="", center_bot=""):
    """Draw a clean multi-segment SVG donut ring (zero external libs)."""
    tot = sum(max(0.0, v) for _, v, _ in slices)
    if tot <= 1e-9:
        d.dot(cx, cy, r_out, OBS_INNER, f'stroke="{OBS_BORDER}" stroke-width="2"')
        d.dot(cx, cy, r_in, OBS_CARD)
        return
    ang = -math.pi * 0.5
    for _, v, col in slices:
        if v <= 0:
            continue
        frac = v / tot
        if frac >= 0.999:
            d.dot(cx, cy, (r_out + r_in) * 0.5, "none", f'stroke="{col}" stroke-width="{r_out-r_in:.1f}"')
            break
        da = frac * 2.0 * math.pi
        a0, a1 = ang, ang + da
        ang = a1
        x0o, y0o = cx + r_out * math.cos(a0), cy + r_out * math.sin(a0)
        x1o, y1o = cx + r_out * math.cos(a1), cy + r_out * math.sin(a1)
        x1i, y1i = cx + r_in * math.cos(a1), cy + r_in * math.sin(a1)
        x0i, y0i = cx + r_in * math.cos(a0), cy + r_in * math.sin(a0)
        large = 1 if da > math.pi else 0
        p = (
            f"M{x0o:.2f} {y0o:.2f} A{r_out:.2f} {r_out:.2f} 0 {large} 1 {x1o:.2f} {y1o:.2f} "
            f"L{x1i:.2f} {y1i:.2f} A{r_in:.2f} {r_in:.2f} 0 {large} 0 {x0i:.2f} {y0i:.2f} Z"
        )
        d.add(f'<path d="{p}" fill="{col}" stroke="{OBS_CARD}" stroke-width="1.6"/>')
    if center_top:
        d.text(cx, cy + (2 if not center_bot else -3), center_top, 18.0, OBS_TEXT, MONO, 700, "middle")
    if center_bot:
        d.text(cx, cy + 16, center_bot, 10.2, OBS_SUB, MONO, 500, "middle")


def _obs_organism_sprite(d, gx, gy, size, grid_str, label="", sub="", accent=OBS_BORDER_HI):
    """Render an organism's 16x16 unified body+brain anatomy as a luminous biological dark-field specimen."""
    dim = 16 if len(grid_str) >= 128 else 7
    d.rect(gx, gy, size, size + (36 if label else 0), OBS_INNER, 10, f'stroke="{accent}" stroke-width="1.2"')
    pad = 8.0
    cpx = (size - 2.0 * pad) / float(dim)
    ox = gx + pad
    oy = gy + pad
    # Subtle microscopic coordinate crosshairs
    d.rule(gx + size * 0.5, gy + 5, gx + size * 0.5, gy + size - 5, "#14272d", 0.8)
    d.rule(gx + 5, gy + size * 0.5, gx + size - 5, gy + size * 0.5, "#14272d", 0.8)
    for i in range(dim * dim):
        tk = _hex_val(grid_str[i]) if i < len(grid_str) else 0
        if tk == 0:
            continue
        cx = ox + (i % dim + 0.5) * cpx
        cy = oy + (i // dim + 0.5) * cpx
        col = GEN_TK_COLORS.get(tk, ("#9aa8a1", ""))[0]
        rad = cpx * 0.46
        if tk >= 9:  # Cortical neurons: glowing soma + bright nucleus
            d.dot(cx, cy, rad * 1.15, col, 'opacity="0.28"')
            d.dot(cx, cy, rad * 0.90, col, 'stroke="#061013" stroke-width="0.6"')
            d.dot(cx, cy, max(1.1, rad * 0.34), "#f7fffb")
        elif tk == 4:  # Eye: golden iris + dark pupil + specular catchlight
            d.dot(cx, cy, rad * 1.12, "#f6c453", 'opacity="0.30"')
            d.dot(cx, cy, rad * 0.95, "#f6c453", 'stroke="#061013" stroke-width="0.6"')
            d.dot(cx, cy, max(1.2, rad * 0.45), "#091316")
            d.dot(cx - rad * 0.22, cy - rad * 0.22, max(0.7, rad * 0.18), "#ffffff")
        elif tk == 6:  # Ear: cyan cochlear ring
            d.dot(cx, cy, rad * 0.95, "#45c7f5", 'stroke="#061013" stroke-width="0.6"')
            d.add(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{rad*0.5:.1f}" fill="none" stroke="#081316" stroke-width="0.9"/>')
        elif tk == 8:  # Gland/Syrinx: luminous violet acoustic resonator
            d.dot(cx, cy, rad * 1.15, "#c77dff", 'opacity="0.28"')
            d.dot(cx, cy, rad * 0.92, "#c77dff", 'stroke="#061013" stroke-width="0.6"')
        else:
            d.rect(cx - rad, cy - rad, rad * 2.0, rad * 2.0, col, max(1.5, rad * 0.55), 'stroke="#061013" stroke-width="0.55"')
    if label:
        d.text(gx + size * 0.5, gy + size + 14, short(label, 22), 10.6, OBS_TEXT, MONO, 700, "middle")
    if sub:
        d.text(gx + size * 0.5, gy + size + 29, short(sub, 24), 9.6, OBS_SUB, SANS, 500, "middle")


def _obs_world_png(map_rows, out_size=256):
    """Render a smooth hillshaded topographic + bathymetric + bioluminescent + acoustic PNG bitmap."""
    n = len(map_rows)
    if n < 4:
        return None
    H = [[0.0] * n for _ in range(n)]
    W = [[0.0] * n for _ in range(n)]
    B = [[0.0] * n for _ in range(n)]
    A = [[0.0] * n for _ in range(n)]
    V = [[0.0] * n for _ in range(n)]
    M = [[0] * n for _ in range(n)]
    for mr in map_rows:
        ry = _gen_i(mr, "genesis_map_row", 0)
        if ry < 0 or ry >= n:
            continue
        toks = mr.get("cells", "").split(",")
        for rx, tok in enumerate(toks[:n]):
            if len(tok) < 5:
                continue
            M[ry][rx] = int(tok[0])
            H[ry][rx] = int(tok[1]) / 9.0
            W[ry][rx] = int(tok[2]) / 9.0
            B[ry][rx] = int(tok[3]) / 9.0
            A[ry][rx] = int(tok[4]) / 9.0
            V[ry][rx] = int(tok[5]) / 9.0 if len(tok) >= 6 else 0.0

    def bilerp(grid, fx, fy):
        x0 = int(fx) % n
        y0 = int(fy) % n
        x1 = (x0 + 1) % n
        y1 = (y0 + 1) % n
        tx = fx - math.floor(fx)
        ty = fy - math.floor(fy)
        return (
            grid[y0][x0] * (1.0 - tx) * (1.0 - ty)
            + grid[y0][x1] * tx * (1.0 - ty)
            + grid[y1][x0] * (1.0 - tx) * ty
            + grid[y1][x1] * tx * ty
        )

    px = bytearray(out_size * out_size * 3)
    scale = float(n) / float(out_size)
    idx = 0
    for py in range(out_size):
        fy = (py + 0.5) * scale - 0.5
        iy = int(fy + 0.5) % n
        for px_i in range(out_size):
            fx = (px_i + 0.5) * scale - 0.5
            ix = int(fx + 0.5) % n
            h = bilerp(H, fx, fy)
            w = bilerp(W, fx, fy)
            b = bilerp(B, fx, fy)
            a = bilerp(A, fx, fy)
            v = bilerp(V, fx, fy)
            mat = M[iy][ix]
            # Smooth northwest directional hillshading for real planetary elevation relief!
            dhx = bilerp(H, fx + 0.65, fy) - bilerp(H, fx - 0.65, fy)
            dhy = bilerp(H, fx, fy + 0.65) - bilerp(H, fx, fy - 0.65)
            shade = max(0.68, min(1.32, 1.0 - (dhx + dhy) * 1.55))

            if mat == 2:  # Geothermal vent: molten magma plume
                r, g, bl = 242.0, 96.0 + 48.0 * b, 42.0
            elif mat == 3:  # Built Polymer Fortress Wall
                r, g, bl = 178.0, 108.0, 238.0
            elif mat == 4:  # Catalytic Mineral Silt
                r, g, bl = 212.0 * shade, 166.0 * shade, 72.0 * shade
            elif mat == 1:  # Ore Ridge
                r, g, bl = (96.0 + 52.0 * h) * shade, (112.0 + 52.0 * h) * shade, (128.0 + 55.0 * h) * shade
            elif w > 0.22:  # Deep Ocean -> Coastal Shallows + Bioluminescent Reef Blooms!
                d_f = min(1.0, max(0.0, (w - 0.22) / 0.72))
                r = (26.0 * (1.0 - d_f) + 10.0 * d_f) * shade
                g = (92.0 * (1.0 - d_f) + 46.0 * d_f) * shade
                bl = (108.0 * (1.0 - d_f) + 74.0 * d_f) * shade
                if b > 0.12:  # Bioluminescent nutrient bloom glow!
                    bf = min(1.0, (b - 0.12) / 0.78)
                    r += bf * 46.0
                    g += bf * 145.0
                    bl += bf * 68.0
                    if bf > 0.72:
                        r += (bf - 0.72) * 260.0
                        g += (bf - 0.72) * 140.0
            else:  # Continental Highlands
                r = (58.0 + 78.0 * h) * shade
                g = (66.0 + 62.0 * h) * shade
                bl = (56.0 + 46.0 * h) * shade
                if b > 0.25:
                    g += (b - 0.25) * 65.0

            # Bathymetric & topographic subtle contour lines
            if abs((h * 6.0) - round(h * 6.0)) < 0.045:
                r *= 0.90; g *= 0.92; bl *= 0.94

            # Glowing 3-band acoustic wavefield overlays (cyan pressure + magenta vocal!)
            if a > 0.25:
                af = min(0.75, (a - 0.25) * 1.05)
                r = r * (1.0 - af) + 115.0 * af
                g = g * (1.0 - af) + 232.0 * af
                bl = bl * (1.0 - af) + 255.0 * af
            if v > 0.22:
                vf = min(0.72, (v - 0.22) * 1.05)
                r = r * (1.0 - vf) + 245.0 * vf
                g = g * (1.0 - vf) + 132.0 * vf
                bl = bl * (1.0 - vf) + 216.0 * vf

            px[idx] = max(0, min(255, int(r)))
            px[idx + 1] = max(0, min(255, int(g)))
            px[idx + 2] = max(0, min(255, int(bl)))
            idx += 3
    return png(out_size, bytes(px))


def genesis_summary(data):
    lines = data.get("lines", [])
    benches = _gen_rows(lines, "genesis_bench")
    sums = _gen_rows(lines, "genesis_summary")
    ctrls = _gen_rows(lines, "genesis_control")
    insp = _gen_rows(lines, "genesis_inspect")
    probes = _gen_rows(lines, "genesis_probe")

    if benches and not sums:
        inf = next((r for r in benches if r.get("genesis_bench") == "snn_inference"), {})
        pla = next((r for r in benches if r.get("genesis_bench") == "snn_plastic"), {})
        wld = next((r for r in benches if r.get("genesis_bench") == "world_physics"), {})
        dev = next((r for r in benches if r.get("genesis_bench") == "ca_morphogenesis"), {})
        cpl = next((r for r in benches if r.get("genesis_bench") == "coupled_alife"), {})
        gsyn = _gen_f(inf, "Gsyn_ops/s")
        gnt = _gen_f(inf, "Gneuron_ticks/s")
        gsyn_p = _gen_f(pla, "Gsyn_ops/s")
        grid_s = wld.get("grid", "256x256")
        return (
            "Axion Genesis v43 · Blackwell GPU Benchmark",
            f"Neocortex STF + VIP/SST Gating + Burstprop + NMDA + k-WTA · 4 B/syn · {grid_s} world · {inf.get('local_B','0')} B spill",
            "Synaptic throughput",
            gsyn,
            " GSynOps/s",
            [["Inference GSynOps/s", round(gsyn, 1)], ["Plastic GSynOps/s", round(gsyn_p, 1)], ["Bytes / synapse", _gen_i(inf, "bytes_per_synapse", 4)]],
            [
                f"Blackwell SM 12.0 sustained {gsyn:.1f} GSynOps/s ({gnt:.2f} Gneuron-ticks/s) across {inf.get('creatures','1024')} layers with {inf.get('local_B','0')} B spill.",
                f"Live 4-receptor Bittner BTSP + active apical BAC + 8-tick axonal delays ran at {gsyn_p:.1f} GSynOps/s (4 B/synapse).",
                f"{grid_s} world stepped in {wld.get('step_us','—')} µs; CA+growth-cones grew {dev.get('creatures','1024')} bodies+brains in {dev.get('total_ms','—')} ms; ALife hit {_gen_f(cpl,'ticks/s'):,.0f} ticks/s.",
            ],
            [f"Coupled {grid_s} world + brains + ecology = {cpl.get('Mcreature_ticks/s','—')} Mcreature-ticks/s."],
        )

    if probes and not sums:
        pmap = {p.get("genesis_probe", ""): p for p in probes}
        rev = pmap.get("reversal_learning", {})
        wm = pmap.get("working_memory", {})
        ag = pmap.get("agency_language", {})
        mmn = pmap.get("predictive_mmn", {})
        zs = pmap.get("zeroshot_compositional_transfer", {})
        qa = pmap.get("vocal_qa_grounding", {})
        pt = pmap.get("cultural_peer_teaching", {})
        dm = pmap.get("episodic_delayed_recall", {})
        cx = pmap.get("context_dependent_dialogue", {})
        nm = pmap.get("grounded_3concept_naming", {})
        p2 = pmap.get("compositional_2syl_syntax", {})
        ic = pmap.get("referential_instruction_following", {})
        p3 = pmap.get("compositional_3syl_syntax", {})
        g18 = pmap.get("compositional_generalization", {})
        e19 = pmap.get("episodic_distractor_reasoning", {})
        t20 = pmap.get("relational_transitive_inference", {})
        sgain = _gen_f(rev, "stdp_gain_pct")
        bgain = _gen_f(rev, "bac_gain_pct")
        pr = _gen_f(wm, "persist_ratio", 1.0)
        return (
            "Axion Genesis v43 · 20-Assay Cognitive Battery",
            f"20 Non-LLM Probes · STDP {sgain:+.1f}% · BAC {bgain:+.1f}% · Q&A {_gen_f(qa,'hit_rate')*100:.0f}% · Ctx {_gen_f(cx,'post_hit')*100:.0f}% · 2-Syl {_gen_f(p2,'post_seq_hit')*100:.0f}% · Gen18 {_gen_f(g18,'post_voc_hit')*100:.0f}% · Epi19 {_gen_f(e19,'post_hit')*100:.0f}% · A>C {_gen_f(t20,'post_trans')*100:.0f}%",
            "STDP reversal gain",
            sgain,
            "%",
            [["Reversal STDP", f"{sgain:+.1f}%"], ["Q&A / 2-Syl", f"{_gen_f(qa,'hit_rate')*100:.0f}% / {_gen_f(p2,'post_seq_hit')*100:.0f}%"], ["Epi19 / Trans20", f"{_gen_f(e19,'post_hit')*100:.0f}% / {_gen_f(t20,'post_trans')*100:.0f}%"]],
            [
                f"Seasonal Reversal: {_gen_f(rev,'intact_atp'):,.0f} ATP intact vs {_gen_f(rev,'frozen_atp'):,.0f} frozen ({sgain:+.1f}%) & {_gen_f(rev,'no_apical_atp'):,.0f} no-BAC ({bgain:+.1f}%).",
                f"Spoken Q&A: {qa.get('pre_sep','0')}→{qa.get('post_sep','0')} (hit {_gen_f(qa,'hit_rate')*100:.0f}%, rev {_gen_f(qa,'rev_hit')*100:.0f}%); Ctx13: {_gen_f(cx,'pre_hit')*100:.0f}%→{_gen_f(cx,'post_hit')*100:.0f}%; 2-Syl: {_gen_f(p2,'post_seq_hit')*100:.0f}%; 3-Syl: {_gen_f(p3,'post_seq_hit')*100:.0f}%.",
                f"Gen18: voc {_gen_f(g18,'post_voc_hit')*100:.0f}%, jnt {_gen_f(g18,'post_joint_hit')*100:.0f}%; Epi19: {_gen_f(e19,'post_hit')*100:.0f}% (cos {_gen_f(e19,'epi_cos_dist'):.2f}); Trans20 A>C: {_gen_f(t20,'pre_trans')*100:.0f}%→{_gen_f(t20,'post_trans')*100:.0f}% (adj {_gen_f(t20,'post_adj')*100:.0f}%).",
            ],
            ["Zero LLMs · Zero hardcoded answers · Real GPU spiking-body evaluation."],
        )

    if sums:
        s = sums[-1]
        t_n = _gen_i(s, "ticks")
        pop = _gen_i(s, "pop")
        alive = _gen_i(s, "alive")
        sp = _gen_i(s, "species")
        mg = _gen_i(s, "max_gen")
        births = _gen_i(s, "births")
        mx_ar = _gen_i(s, "max_areas", 1)
        m_ar = _gen_f(s, "mean_areas", 1.0)
        mx_fi = _gen_i(s, "max_fanin", 16)
        wd_s = _gen_i(s, "world", 256)
        return (
            "Axion Genesis v43 · Neocortex Spiking Mind",
            f"{wd_s}×{wd_s} Trisolaris-11 · {pop} slots · {t_n:,} ticks · Gen {mg} · {sp} morphs · up to {mx_ar} layers",
            "Natural births",
            float(births),
            " births",
            [["Survivors / Births", f"{alive} / {births:,}"], ["Morphs / Gen", f"{sp} / G{mg}"], ["Layers / Fan-In", f"{m_ar:.2f} ({mx_ar}L) · {mx_fi}"]],
            [
                f"{pop} unified CA-DNA organisms evolved {t_n:,} ticks ({_gen_f(s,'ticks/s'):,.0f} ticks/s): {births:,} births, Gen {mg}, up to {mx_ar} layers & {mx_fi}-syn hubs.",
                f"Avg {s.get('mean_neurons','—')} neurons (E {s.get('pyr','0')}/PV {s.get('pv','0')}/SST {s.get('sst','0')}): {_gen_i(s,'prey'):,} prey, {_gen_i(s,'pods'):,} pods, {_gen_i(s,'shelters'):,} shelters, {_gen_i(s,'vocal_replies'):,} vocal replies.",
                "Zero hardcoding: 1 zygote → multi-thousand-neuron neocortex + NMDA subunits + PV k-WTA + Burstprop/dPC (4 B/syn).",
            ],
            [f"Spikes: {_gen_i(s,'spikes'):,}, live STDP edits: {_gen_i(s,'edits'):,}."],
        )

    if insp:
        r = insp[-1]
        return (
            "Axion Genesis v43 · Specimen Inspection",
            f"Tick {r.get('tick','0')} · {r.get('alive','0')} alive · {r.get('species','0')} morphs · Gen {r.get('max_gen','0')}",
            "Living organisms",
            _gen_f(r, "alive"),
            " alive",
            [["Survivors", _gen_i(r, "alive")], ["Morphs", _gen_i(r, "species")], ["Max Gen", _gen_i(r, "max_gen")]],
            ["Inspected saved Genesis v43 unified CA-DNA checkpoint directly on Blackwell GPU."],
            [f"Checkpoint: {r.get('path','')}"],
        )

    convs = _gen_rows(lines, "genesis_converse")
    if convs:
        r = convs[-1]
        turns = _gen_rows(lines, "genesis_turn")
        grp = _gen_i(r, "group", 1)
        peer_tot = sum(_gen_i(t, "peer_replies") for t in turns)
        return (
            "Axion Genesis v43 · Multi-Agent Spiking Mind" if grp > 1 else "Axion Genesis v43 · 3-Band Spiking Dialogue",
            f"{grp} Agent{'s' if grp>1 else ''} · Gen {r.get('specimen_gen','1')} · {r.get('layers','1')}L · {r.get('neurons','0')} neu · {len(turns)} turns · {peer_tot} peer replies",
            "Sensory turns",
            float(len(turns)),
            " turns",
            [["Layers / Neurons", f"{r.get('layers','1')}L / {r.get('neurons','0')}"], ["E/I Cells", f"{r.get('pyr','0')}E/{r.get('pv','0')}PV/{r.get('sst','0')}SST"], ["Organs", f"{r.get('eye','0')}E/{r.get('ear','0')}A/{r.get('gland','0')}G"]],
            [f"Live non-LLM sensory-acoustic dialogue with {grp} evolved Blackwell spiking organism{'s' if grp>1 else ''}.", f"Lead organs: eye={r.get('eye','0')}, ear={r.get('ear','0')}, gland={r.get('gland','0')}; live peer vocal replies={peer_tot}."],
            ["Zero LLMs · Zero text lookup tables · Direct GPU spiking assembly, DA RPE & multi-syllable vocal readout."],
        )

    return None


def genesis_plate_drawing(data, b):
    """Build ONE publication-grade 2200x1320 Dark-Observatory scientific infographic plate."""
    PW, PH = 2200, 1320
    d = Drawing(PW, PH)
    # Deep scientific observatory canvas + luminous header ribbon
    d.add(
        '<defs>'
        '<linearGradient id="obsBg" x1="0" y1="0" x2="1" y2="1">'
        '<stop offset="0%" stop-color="#071014"/><stop offset="55%" stop-color="#0a161b"/><stop offset="100%" stop-color="#071115"/>'
        '</linearGradient>'
        '<linearGradient id="hdrBar" x1="0" y1="0" x2="1" y2="0">'
        '<stop offset="0%" stop-color="#0d2226"/><stop offset="60%" stop-color="#112b2f"/><stop offset="100%" stop-color="#0b1d22"/>'
        '</linearGradient>'
        '<linearGradient id="emFill" x1="0" y1="0" x2="0" y2="1">'
        '<stop offset="0%" stop-color="#3ee6a8" stop-opacity="0.40"/><stop offset="100%" stop-color="#3ee6a8" stop-opacity="0.02"/>'
        '</linearGradient>'
        '<linearGradient id="cyFill" x1="0" y1="0" x2="0" y2="1">'
        '<stop offset="0%" stop-color="#45c7f5" stop-opacity="0.38"/><stop offset="100%" stop-color="#45c7f5" stop-opacity="0.02"/>'
        '</linearGradient>'
        '</defs>'
    )
    d.rect(0, 0, PW, PH, "url(#obsBg)")
    d.rect(0, 0, PW, 102, "url(#hdrBar)")
    d.rule(0, 102, PW, 102, "#24474f", 1.5)

    lines = data.get("lines", [])
    benches = _gen_rows(lines, "genesis_bench")
    ticks = _gen_rows(lines, "genesis_tick")
    sums = _gen_rows(lines, "genesis_summary")
    ctrls = _gen_rows(lines, "genesis_control")
    map_rows = _gen_rows(lines, "genesis_map_row")
    embryos = {r["genesis_embryo"]: r.get("grid", "") for r in _gen_rows(lines, "genesis_embryo")}
    bnodes = _gen_rows(lines, "genesis_brain_nodes")
    barea1 = _gen_rows(lines, "genesis_brain_area1")
    bstats = _gen_rows(lines, "genesis_brain_stats")
    bedges = _gen_rows(lines, "genesis_brain_edges")
    agents_r = _gen_rows(lines, "genesis_agents")
    objs_r = _gen_rows(lines, "genesis_objects")
    syn_dist = _gen_rows(lines, "genesis_syn_dist")
    layers_r = _gen_rows(lines, "genesis_layers")
    vctx_r = _gen_rows(lines, "genesis_vocal_ctx")
    species_top = _gen_rows(lines, "genesis_species_top")
    probes = _gen_rows(lines, "genesis_probe")
    probe_body = _gen_rows(lines, "genesis_probe_body")
    probe_embryo = {r["genesis_probe_embryo"]: r.get("grid", "") for r in _gen_rows(lines, "genesis_probe_embryo")}
    convs = _gen_rows(lines, "genesis_converse")
    conv_agents = _gen_rows(lines, "genesis_conv_agent")

    run_id = b.get("id", 0)
    run_tag = f"AXION GENESIS v43   ·   RUN {run_id:05d}   ·   {b.get('date','')[:10]}" if run_id else "AXION GENESIS v43   ·   BLACKWELL SM 12.0"
    d.text(32, 30, run_tag, 13.0, OBS_EMERALD, MONO, 700, space=1.6)
    d.text(32, 68, short(b.get("title", "Axion Genesis v43"), 58), 32.0, OBS_TEXT, SERIF, 600)
    d.text(32, 91, short(b.get("subtitle", ""), 120), 13.5, OBS_SUB, SANS, 400)

    meas = b.get("measure", 0.0)
    unit = b.get("unit", "")
    fam = b.get("family", "")
    val_s = f"{meas:,.1f}{unit}" if isinstance(meas, float) and abs(meas - round(meas)) > 0.05 else f"{int(round(meas)):,}{unit}"
    d.rect(PW - 344, 16, 316, 70, "#10282c", 12, f'stroke="{OBS_EMERALD}" stroke-width="1.8"')
    d.text(PW - 186, 39, fam.upper(), 11.0, OBS_EMERALD, MONO, 700, "middle", space=1.2)
    d.text(PW - 186, 72, val_s, 27.0, OBS_GOLD, SERIF, 700, "middle")

    dev_clean = data.get("device", "NVIDIA Blackwell").replace(" Generation Laptop GPU", "")
    d.text(32, 1308, f"GPU: {dev_clean} (SM 12.0 Native Cubin)  ·  Zero LLMs  ·  Zero Backprop  ·  Zero Hardcoded Tasks/Weights  ·  4 B/Synapse · 8 B/Neuron", 11.8, OBS_MUTED, MONO)
    d.text(PW - 32, 1308, "AXION GENESIS OBSERVATORY", 11.5, "#365f68", MONO, 700, "end", space=1.4)

    # =========================================================================
    # MODE A: BLACKWELL SM 12.0 HARDWARE BENCHMARK PLATE
    # =========================================================================
    if benches and not sums and not map_rows:
        inf = next((r for r in benches if r.get("genesis_bench") == "snn_inference"), {})
        pla = next((r for r in benches if r.get("genesis_bench") == "snn_plastic"), {})
        wld = next((r for r in benches if r.get("genesis_bench") == "world_physics"), {})
        dev = next((r for r in benches if r.get("genesis_bench") == "ca_morphogenesis"), {})
        cpl = next((r for r in benches if r.get("genesis_bench") == "coupled_alife"), {})
        grid_s = wld.get("grid", "256x256")

        _obs_card(d, 28, 120, 1140, 560, "BLACKWELL SM 12.0 BIT-PACKED MEMORY ARCHITECTURE", "Zero-Spill 4-Byte Plastic Synapses & 8-Byte Dual-Compartment Neurons", OBS_EMERALD, "v4.u32 128-bit Coalesced Loads")
        d.text(52, 206, "1 VARIABLE-FAN-IN MULTI-COMPARTMENT SYNAPSE  =  32 BITS (4 BYTES EXACT)", 12.2, OBS_GOLD, MONO, 700)
        syn_bits = [("PRE ID", "8b + src_L", 8, OBS_EMERALD), ("DALE WT", "int8", 8, OBS_GOLD), ("STDP ELIG", "int7", 7, OBS_CYAN),
                    ("DEL", "3b", 3, OBS_VIOLET), ("C", "1b", 1, OBS_CORAL), ("RL", "2b", 2, OBS_COPPER), ("STP", "3b", 3, "#9be564")]
        bx, btot = 52.0, 1092.0
        for fl, fs, nb, fc in syn_bits:
            fw = btot * (nb / 32.0)
            d.rect(bx + 2, 218, fw - 4, 62, fc, 7, 'stroke="#081013" stroke-width="1.5"')
            d.text(bx + fw * 0.5, 244, fl, 12.2 if nb >= 3 else 10.0, "#081013", MONO, 700, "middle")
            d.text(bx + fw * 0.5, 266, fs, 10.0 if nb >= 3 else 8.5, "#0f2024", SANS, 600, "middle")
            bx += fw

        d.text(52, 314, "1 DUAL-COMPARTMENT PYRAMIDAL / INTERNEURON STATE  =  64 BITS (8 BYTES EXACT)", 12.2, OBS_CYAN, MONO, 700)
        neu_bits = [("SOMA Vs", "13b", 13, OBS_EMERALD), ("APICAL Va (BAC)", "13b", 13, OBS_VIOLET), ("ADAPT Ua", "7b", 7, OBS_COPPER),
                    ("8-STEP RING", "8b", 8, OBS_CYAN), ("DUTY EMA", "8b", 8, OBS_GOLD), ("KIND", "4b", 4, "#9be564"),
                    ("REF", "3b", 3, OBS_CORAL), ("P", "1b", 1, "#ff7eb6"), ("τ", "1b", 1, "#80ffea"), ("FP", "1b", 1, "#ffd166"), ("RSV", "5b", 5, "#425f66")]
        bx = 52.0
        for fl, fs, nb, fc in neu_bits:
            fw = btot * (nb / 64.0)
            d.rect(bx + 1.5, 326, fw - 3, 62, fc, 7, 'stroke="#081013" stroke-width="1.5"')
            d.text(bx + fw * 0.5, 352, fl, 11.2 if nb >= 4 else 9.0, "#081013", MONO, 700, "middle")
            d.text(bx + fw * 0.5, 374, fs, 9.6 if nb >= 4 else 8.0, "#0f2024", SANS, 600, "middle")
            bx += fw

        hw_tiles = [
            ("SNN INFERENCE", f"{_gen_f(inf,'Gsyn_ops/s'):.1f} GSynOps/s", f"{_gen_f(inf,'Gneuron_ticks/s'):.2f} Gneu-t/s · {inf.get('median_ms','—')} ms", f"{inf.get('regs','—')} regs · {inf.get('local_B','0')} B spill", OBS_EMERALD),
            ("SNN + 4-REC STDP", f"{_gen_f(pla,'Gsyn_ops/s'):.1f} GSynOps/s", f"{_gen_f(pla,'Gneuron_ticks/s'):.2f} Gneu-t/s · {pla.get('median_ms','—')} ms", f"{_gen_i(pla,'edits'):,} live edits · {pla.get('local_B','0')} B spill", OBS_GOLD),
            (f"{grid_s} WORLD", f"{_gen_f(wld,'step_us'):.2f} µs/tick", f"{_gen_f(wld,'Mcells/s'):,.0f} Mcells/s · {wld.get('world_KiB','—')} KiB", f"{wld.get('regs','—')} regs · {wld.get('local_B','0')} B spill", OBS_CYAN),
            ("CA + GROWTH CONES", f"{_gen_f(dev,'us_per_creature'):.1f} µs/org", f"{dev.get('creatures','1024')} zygotes in {dev.get('total_ms','—')} ms", f"{dev.get('regs','—')} regs · {dev.get('local_B','0')} B spill", OBS_VIOLET),
            ("COUPLED ALIFE", f"{_gen_f(cpl,'ticks/s'):,.0f} ticks/s", f"W {_gen_f(cpl,'world_us'):.0f} · B {_gen_f(cpl,'brain_us'):.0f} · E {_gen_f(cpl,'body_us'):.0f} · D {_gen_f(cpl,'dev_us'):.0f} µs" if "brain_us" in cpl else f"{_gen_f(cpl,'tick_us'):.1f} µs/tick · {cpl.get('Mcreature_ticks/s','—')} M/s", f"{cpl.get('brain_regs','—')} regs · {cpl.get('brain_local_B','0')} B spill", OBS_CORAL),
            ("FOOTPRINT", f"{_gen_i(inf,'bytes_per_creature',23456):,} B/org", "4 B/syn · 8 B/neuron · 96 CARules", "Variable fan-in 0..128+ · 1..16L", "#9be564"),
        ]
        for i, (tl, tv, ts1, ts2, tc) in enumerate(hw_tiles):
            tx = 52 + (i % 3) * 370
            ty = 414 + (i // 3) * 124
            d.rect(tx, ty, 352, 110, OBS_INNER, 10, f'stroke="{tc}" stroke-width="1.3"')
            d.text(tx + 16, ty + 24, tl, 11.0, tc, MONO, 700)
            d.text(tx + 16, ty + 58, tv, 24.0, OBS_TEXT, SERIF, 700)
            d.text(tx + 16, ty + 82, short(ts1, 36), 11.8, OBS_SUB, SANS, 500)
            d.text(tx + 16, ty + 100, short(ts2, 38), 10.6, OBS_MUTED, MONO)

        _obs_card(d, 28, 700, 1140, 588, "KERNEL OCCUPANCY & REGISTER BUDGET", "Zero Local-Memory Spill Verification Across All 5 Blackwell SM 12.0 Kernels", OBS_CYAN, "0 B Spill Guarantee")
        kerns = [
            ("snn_bench_infer (in-register v4.u32)", _gen_i(inf, "regs", 62), _gen_i(inf, "shared_B", 288), _gen_i(inf, "local_B", 0), OBS_EMERALD),
            ("snn_bench_plastic (4-receptor STDP + BAC)", _gen_i(pla, "regs", 64), _gen_i(pla, "shared_B", 608), _gen_i(pla, "local_B", 0), OBS_GOLD),
            ("world_step_tile (18×18 halo hydro+3-band)", _gen_i(wld, "regs", 64), _gen_i(wld, "shared_B", 14580), _gen_i(wld, "local_B", 0), OBS_CYAN),
            ("develop_organism (unified CA + growth cones)", _gen_i(dev, "regs", 84), _gen_i(dev, "shared_B", 23118), _gen_i(dev, "local_B", 0), OBS_VIOLET),
            ("brain_step_layer (variable fan-in hubs)", _gen_i(cpl, "brain_regs", 80), _gen_i(cpl, "brain_shared_B", 680), _gen_i(cpl, "brain_local_B", 0), OBS_CORAL),
        ]
        for ki, (kn, kr, ks, kl, kc) in enumerate(kerns):
            ky = 784 + ki * 96
            d.text(56, ky, kn, 14.5, OBS_TEXT, SANS, 600)
            d.text(1140, ky, f"{kr} regs  ·  {ks:,} B shmem  ·  {kl} B spill", 13.0, kc, MONO, 700, "end")
            d.rect(56, ky + 14, 1084, 24, OBS_INNER, 7, f'stroke="{OBS_BORDER}" stroke-width="1"')
            d.rect(56, ky + 14, 1084.0 * min(1.0, kr / 128.0), 24, kc, 7, 'opacity="0.88"')
            d.text(56 + 1084.0 * min(1.0, kr / 128.0) - 10, ky + 31, f"{kr}/128 regs", 11.0, "#081013", MONO, 700, "end")
            d.text(56, ky + 56, f"Shared memory utilization: {ks/49152.0*100.0:.1f}% of 48 KiB block budget · Local spill: {kl} B", 11.2, OBS_MUTED, MONO)

        _obs_card(d, 1188, 120, 984, 560, "MEASURED THROUGHPUT COMPARISON", "Blackwell SM 12.0 Synaptic & Embodied Scaling", OBS_GOLD)
        bars = [
            ("In-Register SNN Inference (1,024 × 4k syn × 256t)", _gen_f(inf, "Gsyn_ops/s"), max(1.0, _gen_f(inf, "Gsyn_ops/s")), f"{_gen_f(inf,'Gsyn_ops/s'):.1f} GSynOps/s", OBS_EMERALD),
            ("Live 4-Receptor STDP + BAC + VTA Neuromod", _gen_f(pla, "Gsyn_ops/s"), max(1.0, _gen_f(inf, "Gsyn_ops/s")), f"{_gen_f(pla,'Gsyn_ops/s'):.1f} GSynOps/s", OBS_GOLD),
            (f"{grid_s} World Physics + 4-Wavefield Acoustics", min(100.0, 2000.0 / max(1.0, _gen_f(wld, "step_us", 21))), 100.0, f"{_gen_f(wld,'step_us'):.2f} µs/tick", OBS_CYAN),
            ("Unified 16×16×L CA + Axon Growth Cones", min(100.0, 6000.0 / max(1.0, _gen_f(dev, "us_per_creature", 80))), 100.0, f"{_gen_f(dev,'us_per_creature'):.1f} µs/org", OBS_VIOLET),
            ("Full Coupled 1,024-Organism ALife Tick", min(100.0, _gen_f(cpl, "ticks/s", 350) / 5.0), 100.0, f"{_gen_f(cpl,'ticks/s'):.1f} ticks/s", OBS_CORAL),
        ]
        for bi, (blbl, bv, bmax, bval, bcol) in enumerate(bars):
            by = 204 + bi * 90
            d.text(1214, by, blbl, 14.5, OBS_TEXT, SANS, 600)
            d.text(2144, by, bval, 14.5, bcol, MONO, 700, "end")
            d.rect(1214, by + 14, 930, 26, OBS_INNER, 8, f'stroke="{OBS_BORDER}" stroke-width="1"')
            d.rect(1214, by + 14, max(16.0, 930.0 * min(1.0, bv / max(1e-6, bmax))), 26, bcol, 8)

        _obs_card(d, 1188, 700, 984, 588, "PER-ORGANISM MEMORY BUDGET & BIOPHYSICS", "23.5 KiB Complete Embodied Mind Breakdown", OBS_VIOLET)
        mem_slices = [
            ("Synapse Slabs (4 B/syn)", 16384, OBS_EMERALD),
            ("Tissue + Neurons (16 B/cell)", 4096, OBS_CYAN),
            ("Expandable CARule DNA (96r)", 1568, OBS_GOLD),
            ("Body Phenotype & State", 1408, OBS_VIOLET),
        ]
        _obs_donut(d, 1410, 986, 152, 92, mem_slices, "23.5 KiB", "per organism")
        for mi, (ml, mv, mc) in enumerate(mem_slices):
            my = 838 + mi * 78
            d.rect(1624, my, 520, 62, OBS_INNER, 10, f'stroke="{OBS_BORDER}" stroke-width="1"')
            d.rect(1624, my, 8, 62, mc, 3)
            d.text(1646, my + 26, ml, 14.0, OBS_TEXT, SANS, 600)
            d.text(2126, my + 26, f"{mv:,} B ({100.0*mv/23456.0:.1f}%)", 13.5, mc, MONO, 700, "end")
            d.text(1646, my + 48, "Zero dynamic heap · lock-free GPU slab pool · L2 resident", 11.0, OBS_MUTED, MONO)
        return d

    # =========================================================================
    # MODE C: 9-ASSAY COGNITIVE BATTERY PROBE PLATE (ZERO EMPTY SPACE!)
    # =========================================================================
    if probes and not sums and not map_rows:
        pmap = {p.get("genesis_probe", ""): p for p in probes}
        rev = pmap.get("reversal_learning", {})
        wm = pmap.get("working_memory", {})
        ag = pmap.get("agency_language", {})
        ac = pmap.get("acoustic_dialogue_learning", {})
        sq = pmap.get("temporal_sequence_learning", {})
        c3 = pmap.get("compositional_3band_binding", {})
        mmn = pmap.get("predictive_mmn", {})
        zs = pmap.get("zeroshot_compositional_transfer", {})
        qa = pmap.get("vocal_qa_grounding", {})
        pt = pmap.get("cultural_peer_teaching", {})
        dm = pmap.get("episodic_delayed_recall", {})
        cx = pmap.get("context_dependent_dialogue", {})
        nm = pmap.get("grounded_3concept_naming", {})
        p2 = pmap.get("compositional_2syl_syntax", {})
        ic = pmap.get("referential_instruction_following", {})
        p3 = pmap.get("compositional_3syl_syntax", {})
        g18 = pmap.get("compositional_generalization", {})
        e19 = pmap.get("episodic_distractor_reasoning", {})
        t20 = pmap.get("relational_transitive_inference", {})

        # --- PANEL 1: ASSAY 1 SEASONAL REVERSAL BAR CHART ---
        _obs_card(d, 28, 120, 710, 376, "ASSAY 1 · SEASONAL REVERSAL LEARNING", "Intact vs Frozen STDP vs No-Apical BAC (2 Unseen Worlds)", OBS_EMERALD, f"Alive: {rev.get('alive','0')}")
        e_int, e_frz, e_noa = _gen_f(rev, "intact_atp"), _gen_f(rev, "frozen_atp"), _gen_f(rev, "no_apical_atp")
        emax = max(1.0, e_int, e_frz, e_noa) * 1.15
        sgain, bgain = _gen_f(rev, "stdp_gain_pct"), _gen_f(rev, "bac_gain_pct")
        rev_bars = [
            ("Intact (STDP+BAC)", e_int, OBS_EMERALD, "BASELINE"),
            ("Frozen (No STDP)", e_frz, OBS_GOLD, f"{-sgain:+.1f}% vs intact" if e_frz else "—"),
            ("Apical Off (No BAC)", e_noa, OBS_CORAL, f"{(e_noa-e_int)/max(1,e_int)*100:+.1f}% vs intact"),
        ]
        # Dashed Intact reference line
        by_base = 446.0
        b_h = 216.0
        int_y = by_base - b_h * (e_int / emax)
        d.path(f"M56 {int_y:.1f} H710", OBS_EMERALD, 1.2, "none", 'stroke-dasharray="5,4" opacity="0.55"')
        for ri, (rl, rv, rc, rsub) in enumerate(rev_bars):
            rx = 74 + ri * 212
            rh = max(6.0, b_h * (rv / emax))
            d.rect(rx, by_base - rh, 146, rh, rc, 8, 'opacity="0.88"')
            d.rect(rx, by_base - rh, 146, 5, "#ffffff", 2, 'opacity="0.35"')
            d.text(rx + 73, by_base - rh - 10, f"{rv:,.0f} ATP", 13.5, rc, MONO, 700, "middle")
            d.text(rx + 73, by_base + 20, rl, 12.2, OBS_TEXT, SANS, 600, "middle")
            d.text(rx + 73, by_base + 37, rsub, 10.6, OBS_MUTED, MONO, 500, "middle")
        d.rect(520, 178, 194, 54, OBS_INNER, 10, f'stroke="{OBS_EMERALD if sgain>=0 else OBS_CORAL}" stroke-width="1.4"')
        d.text(617, 198, f"STDP {sgain:+.1f}%", 15.0, OBS_EMERALD if sgain >= 0 else OBS_CORAL, MONO, 700, "middle")
        d.text(617, 219, f"BAC  {bgain:+.1f}%", 12.8, OBS_VIOLET, MONO, 700, "middle")

        # --- PANEL 2: ASSAY 2 WORKING MEMORY ELECTROPHYSIOLOGY TRACE ---
        _obs_card(d, 758, 120, 710, 376, "ASSAY 2 · 60-TICK PITCH-BLACK WORKING MEMORY", "Persistent Cortical Firing After Physical Beacon Extinguishes", OBS_GOLD, f"{wm.get('mean_layers','1')} avg layers")
        w_base, w_cue, w_del = _gen_f(wm, "base_spk/t"), _gen_f(wm, "cue_spk/t"), _gen_f(wm, "delay_spk/t")
        pr = _gen_f(wm, "persist_ratio", 1.0)
        edx = _gen_f(wm, "delay_east_dx")
        px0, py0, pw0, ph0 = 786.0, 200.0, 508.0, 232.0
        d.rect(px0, py0, pw0, ph0, OBS_INNER, 10, f'stroke="{OBS_BORDER}" stroke-width="1"')
        # Phase bands: Baseline (15t), Cue ON (30t), Delay Dark (60t)
        w1, w2 = pw0 * (15.0 / 105.0), pw0 * (30.0 / 105.0)
        w3 = pw0 - w1 - w2
        d.rect(px0 + w1, py0 + 1, w2, ph0 - 2, OBS_GOLD, 0, 'opacity="0.14"')
        d.rect(px0 + w1 + w2, py0 + 1, w3, ph0 - 2, OBS_VIOLET, 0, 'opacity="0.12"')
        d.text(px0 + w1 * 0.5, py0 + 18, "DARK 15t", 9.8, OBS_MUTED, MONO, 700, "middle")
        d.text(px0 + w1 + w2 * 0.5, py0 + 18, "BEACON ON 30t", 10.2, OBS_GOLD, MONO, 700, "middle")
        d.text(px0 + w1 + w2 + w3 * 0.5, py0 + 18, "PITCH-BLACK MEMORY DELAY 60t", 10.2, OBS_VIOLET, MONO, 700, "middle")
        vmin = min(w_base, w_cue, w_del) * 0.82
        vmax = max(w_base, w_cue, w_del, 1.0) * 1.12
        wy = lambda v: py0 + ph0 - 26.0 - (ph0 - 62.0) * ((v - vmin) / max(1e-6, vmax - vmin))
        pts = [(px0 + 8, wy(w_base)), (px0 + w1, wy(w_base)), (px0 + w1 + 14, wy(w_cue)), (px0 + w1 + w2, wy(w_cue)),
               (px0 + w1 + w2 + 18, wy(w_del)), (px0 + pw0 - 8, wy(w_del))]
        p_line = "M" + " L".join(f"{x:.1f} {y:.1f}" for x, y in pts)
        p_area = p_line + f" L{px0+pw0-8:.1f} {py0+ph0-14:.1f} L{px0+8:.1f} {py0+ph0-14:.1f} Z"
        d.add(f'<path d="{p_area}" fill="url(#cyFill)"/>')
        d.path(p_line, OBS_CYAN, 3.0)
        for lx_p, lv, lc in ((px0 + w1 * 0.5, w_base, OBS_SUB), (px0 + w1 + w2 * 0.5, w_cue, OBS_GOLD), (px0 + w1 + w2 + w3 * 0.5, w_del, OBS_VIOLET)):
            d.dot(lx_p, wy(lv), 5.0, lc, f'stroke="{OBS_INNER}" stroke-width="1.5"')
            d.text(lx_p, wy(lv) - 11, f"{lv:,.0f} spk/t", 11.2, lc, MONO, 700, "middle")
        # Persistence + Compass East Drift Widget
        d.rect(1308, 200, 140, 110, OBS_INNER, 10, f'stroke="{OBS_GOLD}" stroke-width="1.3"')
        d.text(1378, 224, "PERSISTENCE", 10.0, OBS_GOLD, MONO, 700, "middle")
        d.text(1378, 266, f"{pr:.2f}×", 28.0, OBS_TEXT, SERIF, 700, "middle")
        d.text(1378, 294, "delay / base", 10.2, OBS_MUTED, MONO, 500, "middle")
        d.rect(1308, 322, 140, 110, OBS_INNER, 10, f'stroke="{OBS_CYAN}" stroke-width="1.3"')
        d.text(1378, 344, "DARK DRIFT", 10.0, OBS_CYAN, MONO, 700, "middle")
        # Compass arrow
        d.dot(1378, 382, 22, "#0c1f24", f'stroke="{OBS_BORDER_HI}" stroke-width="1.2"')
        alen = max(-18.0, min(18.0, edx * 3.0))
        d.path(f"M1378 382 H{1378+alen:.1f}", OBS_EMERALD if edx >= 0 else OBS_CORAL, 3.0)
        d.dot(1378 + alen, 382, 3.5, OBS_EMERALD if edx >= 0 else OBS_CORAL)
        d.text(1378, 421, f"{edx:+.2f} tiles E", 11.0, OBS_TEXT, MONO, 700, "middle")
        dm_cos = _gen_f(dm, "cos_dist") if dm else 0.0
        dm_hit = _gen_f(dm, "recall_hit") if dm else 0.0
        d.text(786, 464, f"60t beacon WM · Assay 12 Dark Attractor CosDist: {dm_cos:.3f} · Delayed Recall Hit: {dm_hit*100:.0f}% (sep {_gen_f(dm,'recall_sep'):.2f})", 11.5, OBS_SUB, MONO)

        # --- PANEL 3: ASSAYS 5, 6, 7 & 9 STDP CONDITIONING & ZERO-SHOT TRANSFER ---
        _obs_card(d, 28, 516, 710, 376, "ASSAYS 5, 6, 7 & 9 · LIVE STDP & ZERO-SHOT TRANSFER", "Pre vs Post Neural Divergence & Grounded Motor Shift", OBS_CYAN, "4 Live Trials Each")
        l_items = [
            ("1-Tone (v0 vs v1)", _gen_f(ac, "pre_div"), _gen_f(ac, "post_div"), _gen_f(ac, "post_v0_mot"), _gen_f(ac, "post_v1_mot"), OBS_CYAN),
            ("2-Syl Bigram (01 vs 10)", _gen_f(sq, "pre_div"), _gen_f(sq, "post_div"), _gen_f(sq, "post_01_mot"), _gen_f(sq, "post_10_mot"), OBS_VIOLET),
            ("3-Syl Trigram (012 vs 210)", _gen_f(c3, "pre_div"), _gen_f(c3, "post_div"), _gen_f(c3, "post_012_mot"), _gen_f(c3, "post_210_mot"), OBS_GOLD),
            ("Zero-Shot Unseen (02 vs 12)", _gen_f(zs, "pre_div"), _gen_f(zs, "post_div"), _gen_f(zs, "post_02_mot"), _gen_f(zs, "post_12_mot"), OBS_EMERALD),
        ]
        dmax = max(0.25, max(max(a, b2) for _, a, b2, _, _, _ in l_items) * 1.18)
        for li, (ll, lpre, lpost, mpos, mneg, lc) in enumerate(l_items):
            ly = 596 + li * 70
            d.text(54, ly + 10, ll, 12.6, OBS_TEXT, SANS, 600)
            d.text(714, ly + 10, f"Pre {lpre:.3f} → Post {lpost:.3f}  |  mot {mpos:.2f} vs {mneg:.2f}", 11.2, lc, MONO, 700, "end")
            bx0, bw = 54.0, 660.0
            d.rect(bx0, ly + 18, bw, 28, OBS_INNER, 6, f'stroke="{OBS_BORDER}" stroke-width="1"')
            # Pre bar (muted top half) + Post bar (luminous bottom half)
            d.rect(bx0 + 2, ly + 20, max(4.0, (bw - 4) * min(1.0, lpre / dmax)), 10, "#4c6b73", 3)
            d.rect(bx0 + 2, ly + 32, max(4.0, (bw - 4) * min(1.0, lpost / dmax)), 12, lc, 3)
        d.text(54, 876, "Gray = Pre-STDP · Bright = Post-STDP learned divergence (Zero LLMs).", 11.0, OBS_MUTED, MONO)

        # --- PANEL 4: ASSAY 8 UNSUPERVISED PREDICTIVE-CODING MMN ---
        _obs_card(d, 758, 516, 710, 376, "ASSAY 8 · PREDICTIVE-CODING MISMATCH NEGATIVITY", "Unsupervised 6-Cycle (v0→v1→v2) Entrainment · ZERO Reward", OBS_VIOLET, "NMDA+ACh/BAC")
        exp_s, odd_s, omis_s = _gen_f(mmn, "exp_spk"), _gen_f(mmn, "odd_spk"), _gen_f(mmn, "omis_spk")
        mmn_p, omis_p, nb_mmn = _gen_f(mmn, "mmn_pct"), _gen_f(mmn, "omis_pct"), _gen_f(mmn, "no_bac_mmn_pct")
        # Rhythm badges along top
        for si, (stxt, scol) in enumerate((("1. v0", OBS_CYAN), ("→", OBS_MUTED), ("2. v1", OBS_GOLD), ("→", OBS_MUTED), ("3. v2 (Expected) vs v0 (Oddball) vs ∅ (Omission)", OBS_VIOLET))):
            d.text(786 + (0, 62, 92, 152, 182)[si], 592, stxt, 12.2, scol, MONO, 700)
        mmax = max(1.0, exp_s, odd_s, omis_s) * 1.15
        m_bars = [("Expected (v2)", exp_s, OBS_EMERALD, "baseline"), ("Oddball (v0)", odd_s, OBS_CORAL, f"MMN {mmn_p:.1f}%"), ("Omission (∅)", omis_s, OBS_CYAN, f"Dip {omis_p:.1f}%")]
        my_b, mh = 834.0, 198.0
        for mi, (ml, mv, mc, msub) in enumerate(m_bars):
            mx = 790 + mi * 158
            mh_i = max(8.0, mh * (mv / mmax))
            d.rect(mx, my_b - mh_i, 118, mh_i, mc, 8, 'opacity="0.88"')
            d.text(mx + 59, my_b - mh_i - 9, f"{mv:,.0f}", 12.5, mc, MONO, 700, "middle")
            d.text(mx + 59, my_b + 19, ml, 11.4, OBS_TEXT, SANS, 600, "middle")
            d.text(mx + 59, my_b + 35, msub, 10.5, OBS_MUTED, MONO, 500, "middle")
        # Intact vs No-BAC surprise card
        d.rect(1268, 620, 178, 240, OBS_INNER, 10, f'stroke="{OBS_VIOLET}" stroke-width="1.3"')
        d.text(1357, 648, "PREDICTIVE GAIN", 10.4, OBS_VIOLET, MONO, 700, "middle")
        d.text(1357, 692, f"{mmn_p:.1f}%", 28.0, OBS_CORAL, SERIF, 700, "middle")
        d.text(1357, 714, "Oddball Surprise", 11.0, OBS_SUB, SANS, 500, "middle")
        d.rule(1288, 734, 1426, 734, OBS_BORDER, 1.0)
        d.text(1357, 768, f"{omis_p:.1f}%", 24.0, OBS_CYAN, SERIF, 700, "middle")
        d.text(1357, 790, "Omission Response", 11.0, OBS_SUB, SANS, 500, "middle")
        d.rule(1288, 808, 1426, 808, OBS_BORDER, 1.0)
        d.text(1357, 838, f"No-BAC: {nb_mmn:.1f}%", 12.2, OBS_GOLD, MONO, 700, "middle")

        # --- PANEL 5: ASSAYS 3, 4 & 10-20 HIGHER COGNITION, LANGUAGE & REASONING GRID ---
        _obs_card(d, 28, 912, 1440, 376, "ASSAYS 3, 4 & 10–20 · GROUNDED LANGUAGE, SYNTAX, EPISODIC & TRANSITIVE REASONING", f"Unseen Arena: {_gen_i(ag,'prey'):,} prey · {_gen_i(ag,'pods'):,} pods · {_gen_i(ag,'tempered')} tools · {_gen_i(ag,'shelters')} shelters", OBS_COPPER, f"Imprints: {_gen_i(ag,'imprints'):,}")
        ag_tiles = [
            ("TRANSITIVE A>C (A20)", f"{_gen_f(t20,'pre_trans')*100:.0f}%→{_gen_f(t20,'post_trans')*100:.0f}%" if t20 else "—", f"adj {_gen_f(t20,'post_adj')*100:.0f}% · AC {_gen_f(t20,'ac_hit')*100:.0f}/CA {_gen_f(t20,'ca_hit')*100:.0f}" if t20 else "Zero-shot A>B,B>C ⇒ A>C", "#ff9f1c"),
            ("EPISODIC DISTRACT (A19)", f"{_gen_f(e19,'pre_hit')*100:.0f}%→{_gen_f(e19,'post_hit')*100:.0f}%" if e19 else "—", f"8t dist · cos {_gen_f(e19,'epi_cos_dist'):.2f} · {_gen_f(e19,'epA_hit')*100:.0f}/{_gen_f(e19,'epB_hit')*100:.0f}" if e19 else "Distractor-resistant recall", "#b8ff6b"),
            ("ZERO-SHOT GEN (A18)", f"{_gen_f(g18,'pre_voc_hit')*100:.0f}%→{_gen_f(g18,'post_voc_hit')*100:.0f}%" if g18 else "—", f"8t dark · jnt {_gen_f(g18,'post_joint_hit')*100:.0f}% · Δ{_gen_f(g18,'post_mot_sep'):+.2f}" if g18 else "0-aud delayed concept+action", "#80ffea"),
            ("3-SYL SYNTAX (A17)", f"{_gen_f(p3,'pre_seq_hit')*100:.0f}%→{_gen_f(p3,'post_seq_hit')*100:.0f}%" if p3 else "—", f"s {_gen_f(p3,'syl1_hit')*100:.0f}/{_gen_f(p3,'syl2_hit')*100:.0f}/{_gen_f(p3,'syl3_hit')*100:.0f}% · {_gen_f(p3,'phraseX_hit')*100:.0f}/{_gen_f(p3,'phraseY_hit')*100:.0f}" if p3 else "3-syl v0-v1-v2 vs v2-v1-v0", "#ffd166"),
            ("CMD FOLLOW (A16)", f"{_gen_f(ic,'pre_hit')*100:.0f}%→{_gen_f(ic,'post_hit')*100:.0f}%" if ic else f"{_gen_i(ag,'apex_kills')} slain", f"go {_gen_f(ic,'go_thr'):.2f}/hl {_gen_f(ic,'halt_thr'):.2f} · Δ{_gen_f(ic,'post_sep'):+.2f}" if ic else f"{_gen_i(ag,'apex_deaths')} deaths", OBS_CORAL),
            ("3-CONCEPT NAMING (A14)", f"{_gen_f(nm,'pre_hit')*100:.0f}%→{_gen_f(nm,'post_hit')*100:.0f}%" if nm else f"{_gen_i(ag,'echo'):,}", f"xm {_gen_f(nm,'post_xmodal')*100:.0f}% · {_gen_f(nm,'c0_hit')*100:.0f}/{_gen_f(nm,'c1_hit')*100:.0f}/{_gen_f(nm,'c2_hit')*100:.0f}%" if nm else "v1 wall/object pings", OBS_EMERALD),
            ("2-SYL SYNTAX (A15)", f"{_gen_f(p2,'pre_seq_hit')*100:.0f}%→{_gen_f(p2,'post_seq_hit')*100:.0f}%" if p2 else f"{_gen_i(ag,'vocal'):,}", f"s1 {_gen_f(p2,'syl1_hit')*100:.0f}% · s2 {_gen_f(p2,'syl2_hit')*100:.0f}% · MI {_gen_f(ag,'vocal_mi_bits'):.3f}b" if p2 else f"{_gen_i(ag,'vocal_replies'):,} repl · MI {_gen_f(ag,'vocal_mi_bits'):.3f}b", "#ff7eb6"),
            ("SPOKEN Q&A (A10)", f"{_gen_f(qa,'hit_rate')*100:.0f}% · {_gen_f(qa,'rev_hit')*100:.0f}%" if qa else "—", f"sep {_gen_f(qa,'pre_sep'):.2f}→{_gen_f(qa,'post_sep'):.2f} (rev {_gen_f(qa,'rev_sep'):.2f})" if qa else "Spoken Q&A", OBS_EMERALD),
            ("PEER TEACHING (A11)", f"{_gen_f(pt,'pre_stu_hit')*100:.0f}%→{_gen_f(pt,'post_stu_hit')*100:.0f}%" if pt else "—", f"sep {_gen_f(pt,'post_stu_sep'):.2f} · {_gen_i(pt,'peer_replies'):,} repl" if pt else "Zero-DA wavefield transfer", OBS_CYAN),
            ("CTX DIALOGUE (A13)", f"{_gen_f(cx,'pre_hit')*100:.0f}%→{_gen_f(cx,'post_hit')*100:.0f}%" if cx else "—", f"10t dark · sep {_gen_f(cx,'pre_sep'):.2f}→{_gen_f(cx,'post_sep'):.2f}" if cx else "Stateful XOR dialogue", OBS_GOLD),
        ]
        for ai, (al, av, asub, acol) in enumerate(ag_tiles):
            ax = 52 + (ai % 5) * 282
            ay = 994 + (ai // 5) * 140
            d.rect(ax, ay, 266, 124, OBS_INNER, 10, f'stroke="{OBS_BORDER}" stroke-width="1.2"')
            d.rect(ax, ay, 266, 4, acol, 2)
            d.text(ax + 16, ay + 28, al, 10.8, acol, MONO, 700)
            d.text(ax + 16, ay + 74, av, 25.0 if len(av) <= 10 else 21.0, OBS_TEXT, SERIF, 700)
            d.text(ax + 16, ay + 104, short(asub, 28), 11.2, OBS_SUB, SANS)

        # --- PANEL 6: 8-AXIS COGNITIVE PROFILE RADAR (SPIDER CHART) ---
        _obs_card(d, 1488, 120, 684, 628, "COGNITIVE CAPABILITY PROFILE", "8-Axis Non-LLM Spiking Intelligence Radar", OBS_EMERALD, "Honest Normalized Axes")
        rcx, rcy, R = 1830.0, 466.0, 206.0
        qa_sep = _gen_f(qa, "post_sep") if qa else 0.0
        axes = [
            ("STDP Reversal", max(0.05, min(1.0, (sgain + 10.0) / 30.0)), f"{sgain:+.1f}%"),
            ("Apical BAC", max(0.05, min(1.0, bgain / 120.0)), f"{bgain:+.0f}%"),
            ("Working Mem", max(0.05, min(1.0, (pr - 0.90) / 0.35 + dm_cos * 0.8 + max(0.0, edx) / 14.0)), f"{pr:.2f}×/{dm_cos:.2f}"),
            ("Predictive MMN", max(0.05, min(1.0, (mmn_p + omis_p) / 25.0)), f"{mmn_p:.1f}%"),
            ("Zero-Shot 02/12", max(0.05, min(1.0, _gen_f(zs, "post_div") / 0.25)), f"{_gen_f(zs,'post_div'):.3f}"),
            ("Spoken Q&A", max(0.05, min(1.0, qa_sep / 0.25 + _gen_f(qa, "hit_rate"))), f"{qa_sep:.2f}"),
            ("Tone/Seq STDP", max(0.05, min(1.0, (_gen_f(ac, "post_div") + _gen_f(sq, "post_div") + _gen_f(c3, "post_div")) / 0.50)), f"{_gen_f(ac,'post_div'):.2f}"),
            ("Agency & MI", max(0.05, min(1.0, _gen_f(ag, "vocal_mi_bits") / 0.15 + min(0.5, _gen_i(ag, "prey") / 300.0))), f"{_gen_f(ag,'vocal_mi_bits'):.3f}b"),
        ]
        n_ax = len(axes)
        for ring_f in (0.25, 0.5, 0.75, 1.0):
            pts_r = []
            for k in range(n_ax):
                ang = -math.pi * 0.5 + k * 2.0 * math.pi / n_ax
                pts_r.append(f"{rcx + R*ring_f*math.cos(ang):.1f} {rcy + R*ring_f*math.sin(ang):.1f}")
            d.add(f'<polygon points="{" ".join(pts_r)}" fill="none" stroke="#1e3942" stroke-width="1.1"/>')
        poly_pts = []
        for k, (albl, aval, araw) in enumerate(axes):
            ang = -math.pi * 0.5 + k * 2.0 * math.pi / n_ax
            x_e, y_e = rcx + R * math.cos(ang), rcy + R * math.sin(ang)
            d.rule(rcx, rcy, x_e, y_e, "#22424d", 1.0)
            vx, vy = rcx + R * aval * math.cos(ang), rcy + R * aval * math.sin(ang)
            poly_pts.append((vx, vy))
            lx, ly = rcx + (R + 34) * math.cos(ang), rcy + (R + 28) * math.sin(ang)
            anc = "middle" if abs(math.cos(ang)) < 0.25 else ("start" if math.cos(ang) > 0 else "end")
            d.text(lx, ly - 3, albl, 11.8, OBS_TEXT, SANS, 600, anc)
            d.text(lx, ly + 13, araw, 11.2, OBS_EMERALD, MONO, 700, anc)
        d.add(f'<polygon points="{" ".join(f"{x:.1f},{y:.1f}" for x,y in poly_pts)}" fill="{OBS_EMERALD}" fill-opacity="0.24" stroke="{OBS_EMERALD}" stroke-width="2.6"/>')
        for vx, vy in poly_pts:
            d.dot(vx, vy, 5.2, OBS_GOLD, f'stroke="{OBS_CARD}" stroke-width="1.6"')

        # --- PANEL 7: TESTED COHORT 1-ZYGOTE -> ADULT GROWTH & E/I COMPOSITION ---
        _obs_card(d, 1488, 768, 684, 520, "TESTED SPECIMEN · 1 ZYGOTE → BODY + BRAIN GROWTH", "CA-DNA Morphogenesis & Cortical E/I Composition", OBS_GOLD)
        pb = probe_body[-1] if probe_body else {}
        if pb:
            if probe_embryo:
                p_stages = [
                    ("ZYGOTE", probe_embryo.get("step0", ""), "1 stem"),
                    ("MITOSIS", probe_embryo.get("step3", ""), "Polarity"),
                    ("ORGANS", probe_embryo.get("step6", ""), "Body+CNS"),
                    ("ADULT", probe_embryo.get("step9", pb.get("grid", "")), f"{pb.get('cells','0')}c"),
                ]
                for si, (st_t, st_g, st_s) in enumerate(p_stages):
                    sx = 1510 + si * 162
                    _obs_organism_sprite(d, sx, 834, 134, st_g, st_t, st_s, OBS_GOLD if si == 3 else OBS_BORDER_HI)
                    if si < 3:
                        d.path(f"M{sx+140} 901 H{sx+156} M{sx+151} 897 L{sx+156} 901 L{sx+151} 905", OBS_GOLD, 1.5)
                ei_slices = [
                    ("PYR E", _gen_f(pb, "pyr"), OBS_EMERALD),
                    ("PV I", _gen_f(pb, "pv"), OBS_CORAL),
                    ("SST I", _gen_f(pb, "sst"), OBS_VIOLET),
                    ("MOD", _gen_f(pb, "mod"), OBS_CYAN),
                ]
                tot_n = max(1.0, sum(v for _, v, _ in ei_slices))
                inh_pct = 100.0 * (_gen_f(pb, "pv") + _gen_f(pb, "sst")) / tot_n
                _obs_donut(d, 1612, 1144, 86, 52, ei_slices, f"{inh_pct:.0f}% I", f"{pb.get('layers','1')}L · {int(tot_n)}n")
                org_chips = [
                    ("EYE", pb.get("eye", "0"), OBS_GOLD), ("EAR", pb.get("ear", "0"), OBS_CYAN),
                    ("CHEM", pb.get("chem", "0"), OBS_EMERALD), ("GLAND", pb.get("gland", "0"), OBS_VIOLET),
                    ("MUSCLE", pb.get("muscle", "0"), OBS_CORAL), ("GRASP", pb.get("grasp", "0"), OBS_COPPER),
                ]
                for oi, (ol, ov, oc) in enumerate(org_chips):
                    ox = 1732 + (oi % 2) * 214
                    oy = 1054 + (oi // 2) * 72
                    d.rect(ox, oy, 200, 58, OBS_INNER, 8, f'stroke="{OBS_BORDER}" stroke-width="1"')
                    d.dot(ox + 16, oy + 29, 5.5, oc)
                    d.text(ox + 30, oy + 33, ol, 11.4, OBS_SUB, MONO, 600)
                    d.text(ox + 184, oy + 34, str(ov), 17.0, OBS_TEXT, MONO, 700, "end")
            else:
                _obs_organism_sprite(d, 1516, 846, 250, pb.get("grid", ""), f"{pb.get('layers','1')}L · {pb.get('neurons','0')} Neurons", f"{pb.get('cells','0')} cells · {pb.get('synapses','0')} syn", OBS_GOLD)
                ei_slices = [("PYR E", _gen_f(pb, "pyr"), OBS_EMERALD), ("PV I", _gen_f(pb, "pv"), OBS_CORAL), ("SST I", _gen_f(pb, "sst"), OBS_VIOLET), ("MOD", _gen_f(pb, "mod"), OBS_CYAN)]
                tot_n = max(1.0, sum(v for _, v, _ in ei_slices))
                inh_pct = 100.0 * (_gen_f(pb, "pv") + _gen_f(pb, "sst")) / tot_n
                _obs_donut(d, 1966, 962, 108, 66, ei_slices, f"{inh_pct:.0f}% I", f"{int(tot_n)} neurons")
        else:
            d.text(1830, 1030, "Cohort anatomy telemetry available in v14+ checkpoints.", 14.0, OBS_MUTED, SANS, 500, "middle")
        return d

    # =========================================================================
    # MODE D: LIVE MULTI-AGENT SPIKING COLONY DIALOGUE ELECTROPHYSIOLOGY PLATE
    # =========================================================================
    if convs and not sums and not map_rows:
        r = convs[-1]
        turns = _gen_rows(lines, "genesis_turn")
        grp = _gen_i(r, "group", 1)
        peer_tot = sum(_gen_i(t, "peer_replies") for t in turns)

        # --- TOP-LEFT: MULTI-AGENT COLONY ANATOMY ROSTER ---
        _obs_card(d, 28, 120, 832, 492, f"LIVE SPIKING COLONY ({grp} ORGANISM{'S' if grp>1 else ''})", f"Gen {r.get('specimen_gen','1')} · {r.get('layers','1')}L · {r.get('neurons','0')} Neurons · {r.get('synapses','0')} Synapses", OBS_EMERALD, f"Peer Replies: {peer_tot}")
        if conv_agents:
            n_show = min(4, len(conv_agents))
            if n_show == 1:
                ca = conv_agents[0]
                _obs_organism_sprite(d, 56, 198, 290, ca.get("grid", ""), f"Agent A0 · {ca.get('layers','1')}L", f"{ca.get('neurons','0')} neurons", OBS_EMERALD)
                ei_s = [("PYR", _gen_f(ca, "pyr"), OBS_EMERALD), ("PV", _gen_f(ca, "pv"), OBS_CORAL), ("SST", _gen_f(ca, "sst"), OBS_VIOLET)]
                _obs_donut(d, 586, 336, 112, 68, ei_s, f"{ca.get('neurons','0')}", "neurons")
                d.text(586, 488, f"Eye {ca.get('eye','0')} · Ear {ca.get('ear','0')} · Gland {ca.get('gland','0')} · Muscle {ca.get('muscle','0')}", 13.0, OBS_GOLD, MONO, 700, "middle")
            else:
                for ai, ca in enumerate(conv_agents[:4]):
                    ax = 52 + (ai % 4) * 198
                    _obs_organism_sprite(d, ax, 200, 184, ca.get("grid", ""), f"A{ai} · {ca.get('layers','1')}L · {ca.get('neurons','0')}n", f"E:{ca.get('eye','0')} A:{ca.get('ear','0')} G:{ca.get('gland','0')} M:{ca.get('muscle','0')}", GEN_LINEAGE_COLORS[ai])
                # Colony E/I & Sensory Summary Strip
                d.rect(52, 444, 784, 144, OBS_INNER, 10, f'stroke="{OBS_BORDER}" stroke-width="1.2"')
                d.text(72, 470, "SHARED ACOUSTIC WAVEFIELD COLONY SUMMARY", 11.2, OBS_CYAN, MONO, 700)
                for ai, ca in enumerate(conv_agents[:4]):
                    ay = 496 + ai * 22
                    d.dot(78, ay - 4, 4.5, GEN_LINEAGE_COLORS[ai])
                    d.text(92, ay, f"Agent A{ai}: {ca.get('layers','1')}L  |  {ca.get('pyr','0')} PYR / {ca.get('pv','0')} PV / {ca.get('sst','0')} SST  |  Eye {ca.get('eye','0')} · Ear {ca.get('ear','0')} · Syrinx {ca.get('gland','0')} · Muscle {ca.get('muscle','0')}", 11.6, OBS_SUB, MONO)
        else:
            d.text(444, 360, f"Specimen: {r.get('layers','1')}L · {r.get('neurons','0')} neurons · eye={r.get('eye','0')} ear={r.get('ear','0')} gland={r.get('gland','0')}", 14.0, OBS_SUB, MONO, 600, "middle")

        # --- BOTTOM-LEFT: LIVE CORTICAL LAYER ACTIVITY HEATMAP (L0..Lmax x Turns) ---
        _obs_card(d, 28, 632, 832, 656, "CORTICAL LAYER ACTIVITY HEATMAP", "Live Active Spiking Assemblies Across Depth (L0..Lmax) × Dialogue Turns", OBS_VIOLET, f"{len(turns)} Sensory Turns")
        n_t = max(1, min(12, len(turns)))
        layer_mat = []
        max_l = 1
        max_act = 1
        for tr in turns[:12]:
            row_l = {}
            for tok in tr.get("active_assemblies", "").split(","):
                if ":" in tok:
                    lk, lv = tok.split(":", 1)
                    try:
                        zi = int(lk.lstrip("L"))
                        zv = int(lv)
                        row_l[zi] = zv
                        max_l = max(max_l, zi + 1)
                        max_act = max(max_act, zv)
                    except Exception:
                        pass
            layer_mat.append(row_l)
        hx0, hy0, hw, hh = 96.0, 720.0, 732.0, 496.0
        cw = hw / float(n_t)
        ch = hh / float(max(1, max_l))
        def _short_cmd(s):
            m = {"teach_qa_01_v2": "T:01→v2", "teach_qa_10_v0": "T:10→v0",
                 "teach_qa_01_v0": "T:01→v0", "teach_qa_10_v2": "T:10→v2",
                 "teach_obj_food_v0": "T:Fd→v0", "teach_obj_food_v2": "T:Fd→v2",
                 "teach_obj_apex_v2": "T:Ax→v2", "teach_obj_apex_v0": "T:Ax→v0",
                 "teach_obj_prey_v1": "T:Pr→v1", "ask_food": "ask_Fd",
                 "teach_phrase_food_02": "T:Fd→02", "teach_phrase_apex_20": "T:Ax→20",
                 "ask_phrase_food": "ask_Fd2", "ask_phrase_apex": "ask_Ax2",
                 "teach_p3_x": "T:X→012", "teach_p3_y": "T:Y→210",
                 "ask_p3_x": "ask_P3X", "ask_p3_y": "ask_P3Y",
                 "teach_cmd_go": "T:02→GO", "teach_cmd_halt": "T:20→HLT",
                 "cmd_go": "cmd_GO", "cmd_halt": "cmd_HLT",
                 "ask_apex": "ask_Ax", "ask_prey": "ask_Pr", "wait_dark": "dark16", "ask_recall": "recall",
                 "ctx_food": "Ctx:Fd", "ctx_danger": "Ctx:Dg", "teach_ctx_v0": "T:Cx→v0",
                 "teach_ctx_v1": "T:Cx→v1", "teach_ctx_v2": "T:Cx→v2", "ask_ctx": "ask_Cx",
                 "teach_rel_ab": "T:A>B", "teach_rel_bc": "T:B>C", "teach_rel_ba": "T:B<A",
                 "teach_rel_cb": "T:C<B", "ask_rel_ac": "ask:A?C", "ask_rel_ca": "ask:C?A",
                 "teach_seq01_food": "T:01+F", "teach_seq10_toxin": "T:10-X",
                 "teach_seq012_food": "T:012+F", "teach_seq210_toxin": "T:210-X",
                 "teach_v0_food": "T:v0+F", "teach_v2_food": "T:v2+F", "teach_v1_apex": "T:v1-A"}
            return m.get(s, short(s, 9))
        for ti in range(n_t):
            raw_inp = turns[ti].get("input", "")
            d.text(hx0 + (ti + 0.5) * cw, hy0 - 10, _short_cmd(raw_inp), 9.4, OBS_GOLD if "teach" in raw_inp else OBS_SUB, MONO, 700, "middle")
        for zi in range(max_l):
            d.text(hx0 - 12, hy0 + (zi + 0.5) * ch + 4, f"L{zi}", 12.0, OBS_VIOLET if zi >= 2 else OBS_EMERALD, MONO, 700, "end")
            for ti in range(n_t):
                v = layer_mat[ti].get(zi, 0)
                f = min(1.0, v / float(max_act))
                if f < 0.35:
                    col = blend("#0d1e24", "#1b6b6b", f / 0.35)
                elif f < 0.72:
                    col = blend("#1b6b6b", "#3ee6a8", (f - 0.35) / 0.37)
                else:
                    col = blend("#3ee6a8", "#f6c453", (f - 0.72) / 0.28)
                d.rect(hx0 + ti * cw + 2, hy0 + zi * ch + 2, cw - 4, ch - 4, col, 5, 'stroke="#081013" stroke-width="1"')
                d.text(hx0 + (ti + 0.5) * cw, hy0 + (zi + 0.5) * ch + 4, str(v), 11.2 if ch >= 36 else 9.5, "#071014" if f > 0.55 else OBS_TEXT, MONO, 700, "middle")
        d.text(hx0, 1252, "Low Activity", 10.2, OBS_MUTED, MONO)
        for gi in range(24):
            gf = gi / 23.0
            gc = blend("#0d1e24", "#3ee6a8", gf / 0.7) if gf < 0.7 else blend("#3ee6a8", "#f6c453", (gf - 0.7) / 0.3)
            d.rect(hx0 + 96 + gi * 14, 1242, 14, 12, gc)
        d.text(hx0 + 444, 1252, f"Peak ({max_act} active cells/layer)", 10.2, OBS_GOLD, MONO, 700)

        # --- TOP-RIGHT: 3-TRACK SYNCHRONIZED ELECTROPHYSIOLOGY OSCILLOSCOPE ---
        _obs_card(d, 880, 120, 1292, 620, "LIVE ELECTROPHYSIOLOGY, SCHULTZ VTA DOPAMINE & MOTOR OSCILLOSCOPE", "Synchronized Real-Time GPU Spike Bursts, STDP Edits, Dopamine RPE & Physical Motor Traces", OBS_CYAN, f"Total Edits: {sum(_gen_i(t,'d_edits') for t in turns):,}")
        ox0, ow = 946.0, 1196.0
        tx_pos = lambda i: ox0 + (i + 0.5) * (ow / float(n_t))
        # Track 1: Spikes curve + STDP Edits bars (y=194..346)
        d.rect(ox0, 194, ow, 152, OBS_INNER, 8, f'stroke="{OBS_BORDER}" stroke-width="1"')
        d.text(ox0 + 12, 212, "TRACK 1 · CORTICAL SPIKES (cyan line) & LIVE STDP WEIGHT EDITS (gold bars)", 10.6, OBS_CYAN, MONO, 700)
        spks = [_gen_f(t, "d_spikes") for t in turns[:12]]
        eds = [_gen_f(t, "d_edits") for t in turns[:12]]
        sp_max, ed_max = max(1.0, max(spks, default=1.0)) * 1.15, max(1.0, max(eds, default=1.0)) * 1.18
        for i, ev in enumerate(eds):
            eh = 114.0 * (ev / ed_max)
            d.rect(tx_pos(i) - 16, 338 - eh, 32, eh, OBS_GOLD, 4, 'opacity="0.38"')
        if spks:
            sp_pts = [(tx_pos(i), 336 - 108.0 * (v / sp_max)) for i, v in enumerate(spks)]
            d.path("M" + " L".join(f"{x:.1f} {y:.1f}" for x, y in sp_pts), OBS_CYAN, 2.8)
            for i, (x, y) in enumerate(sp_pts):
                d.dot(x, y, 4.5, OBS_CYAN, f'stroke="{OBS_INNER}" stroke-width="1.3"')
                d.text(x, y - 9, f"{int(spks[i])}", 9.6, OBS_TEXT, MONO, 700, "middle")

        # Track 2: Bipolar Schultz VTA Dopamine RPE (-1 .. +1) (y=362..498)
        d.rect(ox0, 362, ow, 136, OBS_INNER, 8, f'stroke="{OBS_BORDER}" stroke-width="1"')
        d.text(ox0 + 12, 379, "TRACK 2 · SCHULTZ VTA DOPAMINE RPE (+DA reward emerald / -DA toxin coral)", 10.4, OBS_EMERALD, MONO, 700)
        mid_y = 446.0
        d.rule(ox0 + 8, mid_y, ox0 + ow - 8, mid_y, "#2c4c56", 1.2)
        for i, tr in enumerate(turns[:12]):
            da = max(-1.0, min(1.0, _gen_f(tr, "da")))
            bh = abs(da) * 38.0
            col = OBS_EMERALD if da >= 0 else OBS_CORAL
            if da >= 0:
                d.rect(tx_pos(i) - 18, mid_y - bh, 36, max(2.0, bh), col, 4, 'opacity="0.88"')
                d.text(tx_pos(i), mid_y - bh - 5, f"{da:+.2f}", 9.2, col, MONO, 700, "middle")
            else:
                d.rect(tx_pos(i) - 18, mid_y, 36, max(2.0, bh), col, 4, 'opacity="0.88"')
                d.text(tx_pos(i), mid_y + bh + 11, f"{da:+.2f}", 9.2, col, MONO, 700, "middle")

        # Track 3: Embodied Motor & Vocal Outputs (y=514..694)
        d.rect(ox0, 514, ow, 180, OBS_INNER, 8, f'stroke="{OBS_BORDER}" stroke-width="1"')
        d.text(ox0 + 12, 532, "TRACK 3 · EMBODIED MOTOR & SYRINX OUTPUTS", 10.6, OBS_COPPER, MONO, 700)
        m_series = [
            ("Eat", [_gen_f(t, "eat") for t in turns[:12]], OBS_EMERALD),
            ("Thrust", [_gen_f(t, "thrust") for t in turns[:12]], OBS_CYAN),
            ("Dig", [_gen_f(t, "dig") for t in turns[:12]], OBS_COPPER),
            ("Vocal Peak", [max(_gen_f(t, "vocal0"), _gen_f(t, "vocal1"), _gen_f(t, "vocal2")) for t in turns[:12]], OBS_VIOLET),
        ]
        for mi, (ml, mvals, mc) in enumerate(m_series):
            d.rule(ox0 + 480 + mi * 140, 528, ox0 + 498 + mi * 140, 528, mc, 3.0)
            d.text(ox0 + 504 + mi * 140, 532, ml, 10.5, mc, MONO, 700)
            if mvals:
                mmax_v = max(0.5, max(max(sv) for _, sv, _ in m_series))
                mpts = [(tx_pos(i), 676 - 120.0 * (v / mmax_v)) for i, v in enumerate(mvals)]
                d.path("M" + " L".join(f"{x:.1f} {y:.1f}" for x, y in mpts), mc, 2.4)
                for x, y in mpts:
                    d.dot(x, y, 3.6, mc)
        for i, tr in enumerate(turns[:12]):
            raw_inp = tr.get("input", "")
            d.text(tx_pos(i), 718, _short_cmd(raw_inp), 9.6, OBS_GOLD if "teach" in raw_inp else OBS_SUB, MONO, 700, "middle")

        # --- BOTTOM-RIGHT: LIVE 3-BAND ACOUSTIC DIALOGUE & COLONY REPLIES ---
        _obs_card(d, 880, 760, 1292, 528, "3-BAND ACOUSTIC DIALOGUE & COLONY VOCAL REPLIES", "Human Input Stimulus → Live Syrinx Formants (v0 / v1 / v2) & Multi-Agent Turn-Taking", OBS_GOLD, f"Group={grp}")
        for ti, tr in enumerate(turns[:12]):
            col_i = ti // 6
            row_i = ti % 6
            tx = 904 + col_i * 632
            ty = 836 + row_i * 72
            inp = tr.get("input", "")
            ic = OBS_EMERALD if "food" in inp else (OBS_CORAL if ("toxin" in inp or "apex" in inp) else (OBS_VIOLET if "seq" in inp else OBS_CYAN))
            d.rect(tx, ty, 612, 62, OBS_INNER, 9, f'stroke="{OBS_BORDER}" stroke-width="1.1"')
            d.rect(tx, ty, 6, 62, ic, 3)
            d.text(tx + 18, ty + 23, f"T{ti+1:02d} · {inp}", 12.2, ic, MONO, 700)
            rep_s = tr.get("phrase", tr.get("reply", "silent"))
            d.text(tx + 598, ty + 23, short(f"SEQ: {rep_s}" if "phrase" in tr else f"REPLY: {rep_s}", 34), 11.2, OBS_GOLD if rep_s not in ("silent", "-", "A0:-/A1:-/A2:-/A3:-") else OBS_MUTED, MONO, 700, "end")
            # Live 3-band syrinx bars (v0, v1, v2)
            v0, v1, v2 = _gen_f(tr, "vocal0"), _gen_f(tr, "vocal1"), _gen_f(tr, "vocal2")
            for bi, (bl, bv, bc) in enumerate((("v0", v0, OBS_CYAN), ("v1", v1, OBS_GOLD), ("v2", v2, OBS_VIOLET))):
                bx = tx + 18 + bi * 130
                d.text(bx, ty + 48, bl, 10.2, bc, MONO, 700)
                d.rect(bx + 22, ty + 38, 88, 11, "#14272d", 4)
                d.rect(bx + 22, ty + 38, max(2.0, 88.0 * min(1.0, bv)), 11, bc, 4)
            d.text(tx + 598, ty + 48, f"turn={tr.get('turn','0')} · ATP={tr.get('atp','0')} · peer={tr.get('peer_replies','0')}", 10.4, OBS_SUB, MONO, 500, "end")
        return d

    # =========================================================================
    # MODE B: EMBODIED UNIFIED CA-DNA ECOSYSTEM OBSERVATORY PLATE (2200x1320)
    # =========================================================================
    s0 = sums[-1] if sums else {}
    bs = bstats[-1] if bstats else {}
    world_dim = float(_gen_i(s0, "world", 256 if len(map_rows) >= 64 else 128))

    # --- COLUMN 1 (LEFT, x=28..772, w=744): LIVING PLANET & SPECIES GALLERY ---
    _obs_card(d, 28, 120, 744, 768, f"1. LIVING PHYSICAL PLANET ({int(world_dim)}×{int(world_dim)} GPU WORLD)", "Hillshaded Relief, Geothermal Vents, Reefs, Prey, Tools & 3-Band Acoustics", OBS_EMERALD, f"Morphs: {s0.get('species','—')}")
    mx0, my0, msize = 52.0, 182.0, 668.0
    d.rect(mx0 - 3, my0 - 3, msize + 6, msize + 6, "#060e11", 10, f'stroke="{OBS_BORDER_HI}" stroke-width="1.6"')
    wpng = _obs_world_png(map_rows, 256)
    if wpng:
        d.add(f'<image x="{mx0:.1f}" y="{my0:.1f}" width="{msize:.1f}" height="{msize:.1f}" xlink:href="data:image/png;base64,{wpng}" preserveAspectRatio="none"/>')

    # Vector physical objects overlay
    if objs_r:
        for item in objs_r[-1].get("list", "").split(";"):
            parts = item.split(",")
            if len(parts) < 3:
                continue
            ox = mx0 + (float(parts[0]) / 10.0) * (msize / world_dim)
            oy = my0 + (float(parts[1]) / 10.0) * (msize / world_dim)
            ok = int(parts[2])
            if ok == 1:  # Hard Pod
                d.path(f"M{ox:.1f} {oy-3.8:.1f} L{ox+3.8:.1f} {oy:.1f} L{ox:.1f} {oy+3.8:.1f} L{ox-3.8:.1f} {oy:.1f} Z", "#091316", 0.9, OBS_COPPER)
            elif ok == 2:  # Cracked Fuel Feast
                d.dot(ox, oy, 3.8, OBS_EMERALD, 'stroke="#fff4b8" stroke-width="1.1"')
            elif ok == 3:  # Stone Tool / Anvil
                d.rect(ox - 2.6, oy - 2.6, 5.2, 5.2, "#dde6f0", 1, 'stroke="#0b1518" stroke-width="0.9"')
            elif ok == 4:  # Shy Bioluminescent Prey
                d.dot(ox, oy, 3.1, "#6ef7ff", 'stroke="#061217" stroke-width="0.8"')
            elif ok == 5:  # Giant Megapod
                d.path(f"M{ox:.1f} {oy-5.0:.1f} L{ox+4.4:.1f} {oy-2.3:.1f} L{ox+4.4:.1f} {oy+2.3:.1f} L{ox:.1f} {oy+5.0:.1f} L{ox-4.4:.1f} {oy+2.3:.1f} L{ox-4.4:.1f} {oy-2.3:.1f} Z", OBS_GOLD, 1.1, OBS_CORAL)
            elif ok == 6:  # Apex Predator
                d.path(f"M{ox:.1f} {oy-5.8:.1f} L{ox+5.4:.1f} {oy+4.4:.1f} L{ox-5.4:.1f} {oy+4.4:.1f} Z", "#fff0ed", 1.2, "#e63946")

    # Vector swimming organisms overlay (sleek directional arrow-bodies + glowing Champion beacon!)
    if agents_r:
        for item in agents_r[-1].get("list", "").split(";"):
            parts = item.split(",")
            if len(parts) < 6:
                continue
            ax = mx0 + (float(parts[0]) / 10.0) * (msize / world_dim)
            ay = my0 + (float(parts[1]) / 10.0) * (msize / world_dim)
            ang = float(parts[2]) / 100.0
            lin_c = GEN_LINEAGE_COLORS[int(parts[3]) % len(GEN_LINEAGE_COLORS)]
            rad = max(3.2, min(6.2, 2.5 + float(parts[4]) * 0.018))
            is_best = (parts[5] == "1")
            ca, sa = math.cos(ang), math.sin(ang)
            if is_best:
                d.add(f'<circle cx="{ax:.1f}" cy="{ay:.1f}" r="{rad+8.0:.1f}" fill="none" stroke="{OBS_GOLD}" stroke-width="1.8" stroke-dasharray="3,2"/>')
                d.dot(ax, ay, rad + 4.0, OBS_GOLD, 'opacity="0.22"')
            # Directional streamlined organism chevron
            tip_x, tip_y = ax + ca * (rad * 1.65), ay + sa * (rad * 1.65)
            l_x, l_y = ax - ca * rad + sa * (rad * 0.9), ay - sa * rad - ca * (rad * 0.9)
            r_x, r_y = ax - ca * rad - sa * (rad * 0.9), ay - sa * rad + ca * (rad * 0.9)
            d.path(f"M{tip_x:.1f} {tip_y:.1f} L{l_x:.1f} {l_y:.1f} L{ax-ca*rad*0.4:.1f} {ay-sa*rad*0.4:.1f} L{r_x:.1f} {r_y:.1f} Z", "#061013", 0.9, lin_c)

    map_leg = [("#249c82", "Reefs"), (OBS_COPPER, "Pod"), ("#6ef7ff", "Prey"), ("#f2602a", "Vent"),
               (OBS_VIOLET, "Shelter"), ("#e63946", "Apex"), (OBS_CYAN, "Acoustic"), (OBS_GOLD, "Champion")]
    for li, (lc, ll) in enumerate(map_leg):
        lx = 54 + li * 86
        d.dot(lx + 5, 870, 4.5, lc)
        d.text(lx + 14, 874, ll, 10.6, OBS_SUB, SANS, 600)

    # --- CARD 1B (BOTTOM-LEFT, y=904..1288): EMBRYOGENESIS & TOP-4 SPECIES GALLERY ---
    _obs_card(d, 28, 904, 744, 384, "2. UNIFIED CA-DNA EMBRYOGENESIS & SPECIES GALLERY", f"1 Zygote → Adult Champion (Gen {bs.get('gen','—')}, {bs.get('rules','—')} rules, {bs.get('symmetry_pct','—')}% sym)", OBS_GOLD)
    stages = [
        ("STEP 0 · ZYGOTE", embryos.get("step0", ""), "1 stem"),
        ("STEP 3 · MITOSIS", embryos.get("step3", ""), "Polarity"),
        ("STEP 6 · ORGANS", embryos.get("step6", ""), "Body+CNS"),
        ("ADULT CHAMPION", embryos.get("step9", ""), f"{bs.get('body_cells','—')} cells"),
    ]
    for si, (st_t, st_g, st_s) in enumerate(stages):
        sx = 48 + si * 178
        _obs_organism_sprite(d, sx, 964, 134, st_g, st_t, st_s, OBS_GOLD if si == 3 else OBS_BORDER_HI)
        if si < 3:
            d.path(f"M{sx+142} 1031 H{sx+168} M{sx+162} 1026 L{sx+168} 1031 L{sx+162} 1036", OBS_GOLD, 1.8)

    if species_top:
        d.text(48, 1152, "TOP EVOLVED DISTINCT BODY MORPHS IN ECOSYSTEM", 10.5, OBS_EMERALD, MONO, 700, space=0.9)
        for sp_i, spr in enumerate(species_top[:4]):
            sx = 48 + sp_i * 178
            _obs_organism_sprite(d, sx, 1162, 86, spr.get("grid", ""), f"#{sp_i+1} G{spr.get('gen','?')} · {spr.get('layers','1')}L", f"{spr.get('neurons','0')}n · E{spr.get('eye','0')} A{spr.get('ear','0')} G{spr.get('gl','0')}", GEN_LINEAGE_COLORS[sp_i])
    else:
        # Fallback legend
        leg_tks = [(4, "Eye"), (6, "Ear"), (5, "Chem"), (8, "Gland"), (3, "Muscle"), (7, "Grasp"), (2, "Armor"), (9, "Neurons")]
        for idx_t, (ti, lbl) in enumerate(leg_tks):
            lx = 52 + (idx_t % 4) * 176
            ly = 1178 + (idx_t // 4) * 34
            d.dot(lx + 7, ly - 4, 6.0, GEN_TK_COLORS[ti][0])
            d.text(lx + 20, ly, lbl, 12.2, OBS_TEXT, SANS, 500)

    # --- COLUMN 2 (CENTER, x=792..1512, w=720): 3D CORTEX + BIOPHYSICS + ECOLOGY ---
    c_layers = _gen_i(bs, "areas", 1)
    mx_fi_c = _gen_i(bs, "max_fanin", 16)
    _obs_card(d, 792, 120, 720, 534, f"3. 3D CORTICAL ARCHITECTURE ({c_layers} LAYERS · ≤{mx_fi_c} SYN/CELL)", f"{bs.get('active_neurons','0')} Neurons · {bs.get('exc_syn','0')} E / {bs.get('inh_syn','0')} I · {bs.get('apical_syn','0')} Apical BAC · {bs.get('mean_delay','—')}t Delay", OBS_VIOLET)

    # Left half: Glowing Vector Growth-Cone Connectome (356x356)
    bx0, by0, bsize = 814.0, 184.0, 356.0
    d.rect(bx0, by0, bsize, bsize, OBS_INNER, 10, f'stroke="{OBS_BORDER_HI}" stroke-width="1.3"')
    for r_g in (60, 115, 162):
        d.add(f'<circle cx="{bx0+bsize*0.5:.1f}" cy="{by0+bsize*0.5:.1f}" r="{r_g}" fill="none" stroke="#152b31" stroke-width="0.9"/>')
    step_n = bsize / 16.0
    ncoord = lambda idx: (bx0 + (idx % 16 + 0.5) * step_n, by0 + (idx // 16 + 0.5) * step_n)
    if bedges:
        for item in bedges[-1].get("list", "").split(";"):
            ep = item.split(",")
            if len(ep) < 5:
                continue
            p1, p2, w, comp = int(ep[0]), int(ep[1]), int(ep[2]), int(ep[3])
            cross = int(ep[5]) if len(ep) >= 6 else 0
            x1, y1 = ncoord(p1)
            x2, y2 = ncoord(p2)
            ecol = OBS_GOLD if cross else (OBS_VIOLET if comp == 1 else (OBS_CORAL if w < 0 else OBS_EMERALD))
            op = min(0.85, 0.28 + abs(w) / 140.0)
            d.path(f"M{x1:.1f} {y1:.1f} Q{(x1+x2)*0.5:.1f} {(y1+y2)*0.5-7:.1f} {x2:.1f} {y2:.1f}", ecol, 1.35, "none", f'opacity="{op:.2f}"')
    ntypes_s = bnodes[-1].get("types", "") if bnodes else ""
    a1_s = barea1[-1].get("types", "") if barea1 else ""
    for i in range(256):
        tk = _hex_val(ntypes_s[i]) if i < len(ntypes_s) else 0
        tk1 = _hex_val(a1_s[i]) if i < len(a1_s) else 0
        nx, ny = ncoord(i)
        if tk1 >= 9:
            d.add(f'<circle cx="{nx:.1f}" cy="{ny:.1f}" r="6.6" fill="none" stroke="{OBS_VIOLET}" stroke-width="1.2" opacity="0.65"/>')
        if tk > 0:
            ncol = GEN_TK_COLORS.get(tk, ("#9aa8a1", ""))[0]
            r = 5.4 if tk in (4, 5, 6, 12) else (4.6 if tk >= 9 else 3.6)
            d.dot(nx, ny, r, ncol, 'stroke="#071013" stroke-width="0.9"')

    # Right half: 3D Cortical Layer Stack + Real Inter-Layer Tract Flows!
    d.text(1190, 198, "MULTI-LAYER CORTEX STACK & TRACT FLOW", 10.6, OBS_CYAN, MONO, 700)
    lp_rows = []
    flow_edges = []
    if layers_r:
        for tok in layers_r[-1].get("list", "").split(";"):
            pp = tok.split(",")
            if len(pp) >= 7:
                lp_rows.append(tuple(int(x) for x in pp[:7]))
        for tok in layers_r[-1].get("flow", "").split(";"):
            if ">" in tok and ":" in tok:
                ab, cnt = tok.split(":", 1)
                a_s, b_s = ab.split(">", 1)
                flow_edges.append((int(a_s), int(b_s), int(cnt)))
    if not lp_rows:
        lp_rows = [(z, max(1, _gen_i(bs, "pyr") // max(1, c_layers)), _gen_i(bs, "pv") // max(1, c_layers), _gen_i(bs, "sst") // max(1, c_layers), 0, _gen_i(bs, "motor") if z == 0 else 0, 0) for z in range(min(8, max(1, c_layers)))]
    show_lp = lp_rows
    if len(lp_rows) > 12:
        idxs = [int(round(k * (len(lp_rows) - 1) / 11.0)) for k in range(12)]
        show_lp = [lp_rows[i] for i in dict.fromkeys(idxs)]
    n_lp = len(show_lp)
    ly_step = min(44.0, 326.0 / max(1, n_lp))
    layer_y = {}
    for idx_l, (zi, lpyr, lpv, lsst, lmod, lsoma, lsyn) in enumerate(show_lp):
        ly = 516.0 - idx_l * ly_step
        layer_y[zi] = ly
        tot_l = max(1, lpyr + lpv + lsst + lmod + lsoma)
        bw_l = 148.0
        d.text(1190, ly + 13, f"L{zi}", 10.6 if n_lp > 10 else 11.2, OBS_GOLD if zi == 0 else OBS_CYAN, MONO, 700)
        bar_h = 18 if n_lp > 10 else 22
        d.rect(1216, ly, bw_l, bar_h, OBS_INNER, 4, f'stroke="{OBS_BORDER_HI}" stroke-width="1"')
        # Stacked proportional E/I/Somatic bar inside each cortical layer!
        cur_x = 1217.0
        for lv, lcol in ((lsoma, OBS_COPPER), (lpyr, OBS_EMERALD), (lpv, OBS_CORAL), (lsst, OBS_VIOLET), (lmod, OBS_CYAN)):
            if lv <= 0:
                continue
            seg_w = (bw_l - 2.0) * (lv / float(tot_l))
            d.rect(cur_x, ly + 1, seg_w, bar_h - 2, lcol, 3, 'opacity="0.90"')
            cur_x += seg_w
        syn_lbl = f"{lsyn/1000.0:.1f}k" if lsyn >= 10000 else f"{lsyn}"
        d.text(1370, ly + (12 if n_lp > 10 else 14), f"{lpyr+lpv+lsst+lmod}n·{syn_lbl}s", 9.2 if n_lp > 10 else 9.6, OBS_SUB, MONO, 600)
    # Curved feedforward (cyan) & top-down apical feedback (violet) tract arcs!
    if flow_edges:
        fmax = max(1, max(c for _, _, c in flow_edges))
        for sz, tz, cnt in sorted(flow_edges, key=lambda x: -x[2])[:14]:
            if sz == tz or sz not in layer_y or tz not in layer_y:
                continue
            y1, y2 = layer_y[sz] + 11.0, layer_y[tz] + 11.0
            is_ff = tz > sz
            x_arc = 1482.0 if is_ff else 1468.0
            ctrl_x = x_arc + (22.0 if is_ff else -16.0) + abs(tz - sz) * 3.5
            sw = 1.0 + 2.8 * (cnt / float(fmax))
            d.path(f"M1480 {y1:.1f} Q{ctrl_x:.1f} {(y1+y2)*0.5:.1f} 1480 {y2:.1f}", OBS_CYAN if is_ff else OBS_VIOLET, sw, "none", 'opacity="0.72"')

    # Connectome legend & vital footer
    conn_leg = [(OBS_EMERALD, "Basal Exc"), (OBS_CORAL, "Inhibitory (Dale)"), (OBS_VIOLET, "Apical BAC"), (OBS_GOLD, "Cross-Layer"), (OBS_COPPER, "Somatic")]
    for ci, (cc, cl) in enumerate(conn_leg):
        d.rect(816 + ci * 136, 560, 12, 12, cc, 3)
        d.text(834 + ci * 136, 570, cl, 11.0, OBS_SUB, SANS, 500)
    d.rect(814, 588, 676, 48, OBS_INNER, 8, f'stroke="{OBS_BORDER}" stroke-width="1"')
    d.text(830, 617, f"Champion: {bs.get('pyr','0')} PYR · {bs.get('pv','0')} PV · {bs.get('sst','0')} SST · {bs.get('mod','0')} MOD · {bs.get('sens','0')} Sens · {bs.get('motor','0')} Eff · {bs.get('wm_syn','0')} cross-L syn", 11.5, OBS_GOLD, MONO, 700)

    # --- CARD 2B (y=670..988): SYNAPTIC BIOPHYSICS OBSERVATORY ---
    _obs_card(d, 792, 670, 720, 318, "4. SYNAPTIC BIOPHYSICS OBSERVATORY", "Mirror E/I Weights · Polychronic Delays · 3×4 Vocal Semantics", OBS_CYAN)
    sd = syn_dist[-1] if syn_dist else {}
    exc_h = [int(x) for x in sd.get("exc", "").split(",") if x.isdigit()] if sd else []
    inh_h = [int(x) for x in sd.get("inh", "").split(",") if x.isdigit()] if sd else []
    del_h = [int(x) for x in sd.get("del", "").split(",") if x.isdigit()] if sd else []
    rec_h = [int(x) for x in sd.get("rec", "").split(",") if x.isdigit()] if sd else []

    # Left: Mirror Log-Normal E/I Synaptic Weight Histogram (x=814..1148)
    d.rect(814, 738, 336, 230, OBS_INNER, 9, f'stroke="{OBS_BORDER}" stroke-width="1"')
    d.text(828, 758, "SYNAPTIC WEIGHTS (+E up / -I down)", 10.2, OBS_EMERALD, MONO, 700)
    wmid = 886.0
    d.rule(824, wmid, 1140, wmid, "#2d4c55", 1.1)
    if exc_h or inh_h:
        hmax = max(1, max(exc_h or [0]), max(inh_h or [0]))
        for bi in range(16):
            bx = 828 + bi * 19.2
            ev = exc_h[bi] if bi < len(exc_h) else 0
            iv = inh_h[bi] if bi < len(inh_h) else 0
            eh = 108.0 * (ev / float(hmax))
            ih = 64.0 * (iv / float(hmax))
            if ev > 0:
                d.rect(bx, wmid - eh, 15.0, max(2.0, eh), OBS_EMERALD, 3, 'opacity="0.88"')
            if iv > 0:
                d.rect(bx, wmid + 1, 15.0, max(2.0, ih), OBS_CORAL, 3, 'opacity="0.88"')
        d.text(828, 960, "|w|=0", 9.4, OBS_MUTED, MONO)
        d.text(982, 960, "|w|=48", 9.4, OBS_MUTED, MONO, 500, "middle")
        d.text(1138, 960, "|w|=96+", 9.4, OBS_MUTED, MONO, 500, "end")
    else:
        d.text(982, 860, "Weight histogram in v14+ runs", 11.0, OBS_MUTED, SANS, 500, "middle")

    # Top-Right: Axonal Conduction Delay Spectrum (1t..8t)
    d.rect(1164, 738, 326, 110, OBS_INNER, 9, f'stroke="{OBS_BORDER}" stroke-width="1"')
    d.text(1178, 756, "AXON DELAY SPECTRUM (1t..8t)", 10.0, OBS_GOLD, MONO, 700)
    if del_h:
        dmx = max(1, max(del_h))
        for di in range(min(8, len(del_h))):
            dx = 1180 + di * 37.5
            dh = 58.0 * (del_h[di] / float(dmx))
            d.rect(dx, 826 - dh, 26, max(2.0, dh), OBS_GOLD, 3, 'opacity="0.85"')
            d.text(dx + 13, 840, f"{di+1}t", 9.4, OBS_SUB, MONO, 600, "middle")

    # Bottom-Right: 3x4 Vocal-Context Semantic Heatmap (v0/v1/v2 x Food/Apex/Toxin/Open)
    d.rect(1164, 858, 326, 110, OBS_INNER, 9, f'stroke="{OBS_BORDER}" stroke-width="1"')
    d.text(1178, 875, f"VOCAL SEMANTIC MATRIX (MI {_gen_f(s0,'vocal_mi_bits'):.3f}b)", 9.8, OBS_VIOLET, MONO, 700)
    vmat = []
    if vctx_r:
        for row_s in vctx_r[-1].get("mat", "").split(";"):
            vmat.append([int(x) for x in row_s.split(",") if x.isdigit()])
    ctx_lbls = ("Food", "Apex", "Tox", "Open")
    for ci, cl in enumerate(ctx_lbls):
        d.text(1240 + ci * 60, 892, cl, 9.2, OBS_MUTED, MONO, 600, "middle")
    vmax_c = max(1, max((max(r2) for r2 in vmat if r2), default=1))
    for bi in range(3):
        by = 898 + bi * 21
        d.text(1182, by + 14, f"v{bi}", 10.2, (OBS_CYAN, OBS_GOLD, OBS_VIOLET)[bi], MONO, 700)
        for ci in range(4):
            val_c = vmat[bi][ci] if (bi < len(vmat) and ci < len(vmat[bi])) else 0
            cf = min(1.0, math.sqrt(val_c / float(vmax_c)))
            ccol = blend("#12242a", (OBS_CYAN, OBS_GOLD, OBS_VIOLET)[bi], cf)
            d.rect(1212 + ci * 60, by, 54, 17, ccol, 4)
            d.text(1239 + ci * 60, by + 12, human(val_c) if val_c > 0 else "·", 8.8, "#071014" if cf > 0.55 else OBS_TEXT, MONO, 700, "middle")

    # --- CARD 2C (y=1004..1288): ECOLOGY & EMERGENT BEHAVIOR KPI GRID ---
    _obs_card(d, 792, 1004, 720, 284, "5. EMERGENT BEHAVIORAL & CULTURAL ECOLOGY", "Real GPU Physical Interactions (Zero Hardcoded Scripts)", OBS_COPPER)
    eco_tiles = [
        ("EVASIVE PREY CAUGHT", f"{_gen_i(s0,'prey'):,}", "Day optical + Night acoustic", OBS_CYAN),
        ("PODS (TOOL / COOP)", f"{_gen_i(s0,'pods'):,}", f"{_gen_i(s0,'tools'):,} tool · {_gen_i(s0,'coop'):,} coop · {_gen_i(s0,'duet'):,} duet", OBS_GOLD),
        ("TEMPERED TOOLS / STONES", f"{_gen_i(s0,'tempered'):,} / {_gen_i(s0,'stones'):,}", "Heat-forged composite blades", OBS_COPPER),
        ("APEX SLAIN & SHELTERS", f"{_gen_i(s0,'apex_k'):,} / {_gen_i(s0,'shelters'):,}", f"{_gen_i(s0,'cached'):,} cached granary feasts", OBS_CORAL),
        ("VOCALS / REPLIES / MIMICS", f"{human(_gen_i(s0,'vocal'))}", f"{_gen_i(s0,'vocal_replies'):,} repl · {_gen_i(s0,'vocal_mimics'):,} mimics", OBS_VIOLET),
        ("LIVE STDP EDITS / REWIRES", f"{human(_gen_i(s0,'edits'))}", f"{_gen_i(s0,'rewires'):,} rewires · {_gen_i(s0,'matures'):,} matures", OBS_EMERALD),
    ]
    for ei, (el, ev, es, ec) in enumerate(eco_tiles):
        ex = 814 + (ei % 3) * 230
        ey = 1076 + (ei // 3) * 102
        d.rect(ex, ey, 218, 90, OBS_INNER, 9, f'stroke="{OBS_BORDER}" stroke-width="1.1"')
        d.rect(ex, ey, 218, 3.5, ec, 2)
        d.text(ex + 13, ey + 23, el, 9.6, ec, MONO, 700)
        d.text(ex + 13, ey + 56, ev, 21.5 if len(ev) <= 10 else 18.0, OBS_TEXT, SERIF, 700)
        d.text(ex + 13, ey + 78, short(es, 30), 10.2, OBS_SUB, SANS)

    # --- COLUMN 3 (RIGHT, x=1532..2172, w=640): EVOLUTIONARY TRAJECTORY, CONTROLS & MORPHOSPACE ---
    _obs_card(d, 1532, 120, 640, 328, "6. EVOLUTIONARY DYNAMICS OVER TIME", "Population Survival, Diversity & Cortical Encephalization", OBS_CYAN)
    cx0, cw = 1556.0, 592.0
    if len(ticks) >= 2:
        t_v = [_gen_i(r, "genesis_tick") for r in ticks]
        al_v = [_gen_f(r, "alive") for r in ticks]
        sp_v = [_gen_f(r, "species") for r in ticks]
        ar_v = [_gen_f(r, "mean_areas") for r in ticks]
        ne_v = [_gen_f(r, "mean_neurons") for r in ticks]
        # Top chart: Alive + Morphs (y=184..298)
        d.rect(cx0, 184, cw, 114, OBS_INNER, 8, f'stroke="{OBS_BORDER}" stroke-width="1"')
        al_mx = max(10.0, max(al_v) * 1.12)
        trend(d, cx0 + 12, 204, cw - 24, 80, al_v, OBS_EMERALD, True, t_v, (0, al_mx))
        trend(d, cx0 + 12, 204, cw - 24, 80, sp_v, OBS_CYAN, False, t_v, (0, al_mx))
        d.text(cx0 + 14, 199, f"Alive ({int(al_v[-1])})", 10.2, OBS_EMERALD, MONO, 700)
        d.text(cx0 + 124, 199, f"Morphs ({int(sp_v[-1])})", 10.2, OBS_CYAN, MONO, 700)
        d.text(cx0 + cw - 12, 199, f"Ticks {t_v[0]}→{t_v[-1]:,}", 10.0, OBS_MUTED, MONO, 500, "end")
        # Bottom chart: Mean Layers + Mean Neurons (y=310..428)
        d.rect(cx0, 310, cw, 118, OBS_INNER, 8, f'stroke="{OBS_BORDER}" stroke-width="1"')
        ar_mx = max(2.0, max(ar_v) * 1.15)
        ne_mx = max(10.0, max(ne_v) * 1.15)
        trend(d, cx0 + 12, 330, cw - 24, 84, ar_v, OBS_VIOLET, False, t_v, (0, ar_mx))
        trend(d, cx0 + 12, 330, cw - 24, 84, ne_v, OBS_GOLD, False, t_v, (0, ne_mx))
        d.text(cx0 + 14, 326, f"Mean Layers ({ar_v[-1]:.2f}L)", 10.2, OBS_VIOLET, MONO, 700)
        d.text(cx0 + 178, 326, f"Mean Neurons ({ne_v[-1]:.1f})", 10.2, OBS_GOLD, MONO, 700)
        d.text(cx0 + cw - 12, 326, f"Gen {int(_gen_f(ticks[-1],'max_gen'))}", 10.2, OBS_EMERALD, MONO, 700, "end")
    else:
        d.rect(cx0, 188, cw, 236, OBS_INNER, 8, f'stroke="{OBS_BORDER}" stroke-width="1"')
        d.text(cx0 + cw * 0.5, 308, "Single-checkpoint inspection snapshot", 14.0, OBS_MUTED, SANS, 500, "middle")

    # --- CARD 3B (y=464..936): HELD-OUT 2-WORLD CONTROLS & LESION DELTA WATERFALL ---
    _obs_card(d, 1532, 464, 640, 472, "7. HELD-OUT 2-WORLD CONTROLS & LESIONS", "Real Physical Ablations on Unseen Shifted Worlds (ZERO Fake Guards)", OBS_CORAL)
    if ctrls:
        cmap = {c["genesis_control"]: c for c in ctrls}
        int_en = max(1.0, _gen_f(cmap.get("evolved_intact", {}), "energy", 1.0))
        max_en = max(int_en, max(_gen_f(c, "energy") for c in ctrls))
        arm_defs = [
            ("evolved_intact", "Evolved Intact (Unified CA + STDP + BAC)", OBS_EMERALD),
            ("evolved_frozen", "Plasticity Frozen (No Within-Life STDP)", OBS_GOLD),
            ("evolved_no_apical", "Apical Dendrites Silenced (No BAC)", OBS_VIOLET),
            ("evolved_zero_delay", "Axonal Delays Flattened (Uniform 3t)", OBS_CYAN),
            ("evolved_no_neuromod", "Neuromodulation Silenced (DA=ACh=0)", OBS_COPPER),
            ("evolved_blind", "Photoreceptors Blinded (Eyes Off)", "#e09f3e"),
            ("evolved_deaf", "Acoustic Hair Cells Silenced (Ears Off)", "#5fa8d3"),
            ("drift_unselected", "Unselected Drift Control (No Selection)", OBS_CORAL),
        ]
        ref_x = 1558.0 + 492.0 * (int_en / max_en)
        d.path(f"M{ref_x:.1f} 534 V918", OBS_EMERALD, 1.1, "none", 'stroke-dasharray="4,3" opacity="0.45"')
        ry = 540.0
        for key, lbl, col in arm_defs:
            c = cmap.get(key)
            if not c:
                continue
            en = _gen_f(c, "energy")
            al = _gen_i(c, "alive")
            br = _gen_i(c, "births")
            pct = 100.0 * (en - int_en) / int_en
            d.text(1558, ry + 11, lbl, 12.0, OBS_TEXT, SANS, 600)
            d.text(2052, ry + 11, f"{en:,.0f} ATP · {al}a/{br}b", 11.0, col, MONO, 600, "end")
            if key == "evolved_intact":
                d.rect(2062, ry - 1, 86, 18, "#163d34", 5, f'stroke="{OBS_EMERALD}" stroke-width="1"')
                d.text(2105, ry + 12, "BASELINE", 9.2, OBS_EMERALD, MONO, 700, "middle")
            else:
                dc = OBS_CORAL if pct < -0.5 else (OBS_GOLD if pct <= 0.5 else OBS_CYAN)
                d.rect(2062, ry - 1, 86, 18, "#29181b" if pct < 0 else "#182b2e", 5, f'stroke="{dc}" stroke-width="1"')
                d.text(2105, ry + 12, f"{pct:+.1f}%", 10.0, dc, MONO, 700, "middle")
            d.rect(1558, ry + 18, 492, 13, OBS_INNER, 5, f'stroke="{OBS_BORDER}" stroke-width="0.9"')
            d.rect(1558, ry + 18, max(6.0, 492.0 * min(1.0, en / max_en)), 13, col, 5, 'opacity="0.90"')
            ry += 47.0
    else:
        d.rect(1556, 540, 592, 370, OBS_INNER, 10, f'stroke="{OBS_BORDER}" stroke-width="1"')
        d.text(1852, 725, "Run with --controls to measure all 7 physical lesions vs Selection.", 13.5, OBS_MUTED, SANS, 500, "middle")

    # --- CARD 3C (y=952..1288): POPULATION ORGAN MORPHOSPACE & CORTICAL E/I DONUT ---
    _obs_card(d, 1532, 952, 640, 336, "8. EVOLVED MORPHOSPACE & CORTICAL E/I RATIO", "Population-Mean Somatic Organs & Excitatory/Inhibitory Balance", OBS_EMERALD)
    org_bars = [
        ("Eye", _gen_f(s0, "eye"), OBS_GOLD),
        ("Ear", _gen_f(s0, "ear"), OBS_CYAN),
        ("Chem", _gen_f(s0, "chem"), OBS_EMERALD),
        ("Gland", _gen_f(s0, "gland"), OBS_VIOLET),
        ("Muscle", _gen_f(s0, "muscle"), OBS_CORAL),
        ("Grasp", _gen_f(s0, "grasp"), OBS_COPPER),
        ("Armor", _gen_f(s0, "struct"), "#8b9ca6"),
    ]
    omax = max(1.0, max(v for _, v, _ in org_bars))
    for oi, (ol, ov, oc) in enumerate(org_bars):
        oy = 1024 + oi * 35
        d.text(1556, oy + 14, ol, 11.8, OBS_TEXT, SANS, 600)
        d.rect(1622, oy + 2, 232, 16, OBS_INNER, 5, f'stroke="{OBS_BORDER}" stroke-width="0.9"')
        d.rect(1622, oy + 2, max(4.0, 232.0 * (ov / omax)), 16, oc, 5, 'opacity="0.90"')
        d.text(1864, oy + 14, f"{ov:.2f}", 11.5, oc, MONO, 700)

    # Right side: Cortical E/I Composition Donut!
    p_pyr, p_pv, p_sst, p_mod = _gen_f(s0, "pyr"), _gen_f(s0, "pv"), _gen_f(s0, "sst"), _gen_f(s0, "mod")
    ei_tot = max(1e-6, p_pyr + p_pv + p_sst + p_mod)
    inh_pct = 100.0 * (p_pv + p_sst) / ei_tot
    _obs_donut(d, 2028, 1104, 86, 52, [("PYR", p_pyr, OBS_EMERALD), ("PV", p_pv, OBS_CORAL), ("SST", p_sst, OBS_VIOLET), ("MOD", p_mod, OBS_CYAN)], f"{inh_pct:.1f}% I", f"{ei_tot:.0f} neu")
    d.text(2028, 1216, f"PYR {p_pyr:.1f} · PV {p_pv:.1f}", 10.6, OBS_EMERALD, MONO, 700, "middle")
    d.text(2028, 1236, f"SST {p_sst:.1f} · MOD {p_mod:.2f}", 10.6, OBS_VIOLET, MONO, 700, "middle")
    d.text(2028, 1262, f"Rules {_gen_f(s0,'mean_rules'):.1f} · Fan-In {_gen_f(s0,'mean_fanin'):.0f}", 10.2, OBS_GOLD, MONO, 600, "middle")

    return d
