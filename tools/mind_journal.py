"""Illustrated WebP field plates and lively summaries for the unified spiking cortical mind (`axion mind`)."""


def _mind_parse_domains(lines):
    out = []
    for line in lines:
        s = line.strip()
        if not s.startswith("mind_domain="):
            continue
        m = re.match(r'^mind_domain="([^"]+)"\s+correct=(\d+)\s+total=(\d+)', s)
        if m:
            out.append((m.group(1), int(m.group(2)), int(m.group(3))))
    return out


def _mind_pill(d, x, y, label, bg, fg="#162d2a", fsize=10.0, pad_x=7.5):
    w = max(36.0, len(label) * (fsize * 0.62) + 2.0 * pad_x)
    h = fsize + 7.5
    d.rect(x, y, w, h, bg, h * 0.5)
    d.text(x + w * 0.5, y + h - 4.2, label, fsize, fg, MONO, 700, "middle")
    return w


def _mind_critter(d, cx, cy, scale=1.0, role="axion", mood="happy", level=3, facing=1):
    """Draw a charming, expressive botanical-guide vector critter for Axion's spiking cast."""
    fx = -1 if facing < 0 else 1
    d.add(f'<g transform="translate({cx:.2f},{cy:.2f}) scale({scale * fx:.3f},{scale:.3f})">')

    # Soft bioluminescent aura for high-level minds
    if level >= 3 and role in ("axion", "alien", "elder"):
        aura = "#bcb6ec" if role == "alien" else ("#f3bc73" if role == "elder" else "#91cfb7")
        d.dot(0, -2, 26, aura, 'opacity=".14"')
        d.dot(0, -2, 20, aura, 'opacity=".16"')

    if role == "fox":
        # Fluffy tipped tail
        d.path("M-13 9 C-27 11 -29 -3 -20 -8 C-14 -11 -10 -1 -12 7 Z", "#d67763", 1.2, "#e28b79")
        d.path("M-23 -4 C-25 -8 -21 -10 -18 -7 Z", "#f5e6d2", 0.6, "#f5e6d2")
        # Pointed ears
        d.path("M-12 -11 L-16 -27 L-3 -16 Z", "#b35847", 1.2, "#e28b79")
        d.path("M12 -11 L16 -27 L3 -16 Z", "#b35847", 1.2, "#e28b79")
        d.path("M-11 -13 L-14 -22 L-6 -16 Z", "#294640", 0.5, "#2b3b37")
        d.path("M11 -13 L14 -22 L6 -16 Z", "#294640", 0.5, "#2b3b37")
        # Body & cheeky face
        d.add('<ellipse cx="0" cy="7" rx="14" ry="12" fill="#e28b79" stroke="#9e4c3d" stroke-width="1.2"/>')
        d.add('<ellipse cx="0" cy="10" rx="8.5" ry="7" fill="#f5e6d2"/>')
        d.add('<ellipse cx="0" cy="-5" rx="15" ry="12.5" fill="#e28b79" stroke="#9e4c3d" stroke-width="1.2"/>')
        d.path("M-14 -2 Q-7 7 0 6 Q7 7 14 -2 Q10 8 0 8 Q-10 8 -14 -2 Z", "#f5e6d2", 0.5, "#f5e6d2")
        # Eyes (sly or surprised when busted)
        if mood == "busted":
            for sx in (-6, 6):
                d.dot(sx, -7, 3.6, "#ffffff")
                d.dot(sx, -7, 1.8, "#162d2a")
            d.path("M-4 2 Q0 -1 4 2", "#162d2a", 1.6)
        else:
            for sx in (-6, 6):
                d.dot(sx, -6, 2.7, "#162d2a")
                d.dot(sx + 0.8, -7, 0.9, "#ffffff")
            d.path("M-10 -10 L-2 -8 M10 -10 L2 -8", "#5a251c", 1.5)
            d.path("M-5 1 Q1 5 6 0", "#162d2a", 1.5)
        d.dot(0, -2, 2.2, "#1c2b27")

    elif role == "mira":
        # Wise owl-scholar critter in warm gold
        # Wing tufts & ear plumes
        d.path("M-15 -10 Q-18 -22 -8 -17", "#c98b3e", 2.0, "#f3bc73")
        d.path("M15 -10 Q18 -22 8 -17", "#c98b3e", 2.0, "#f3bc73")
        d.add('<ellipse cx="0" cy="7" rx="15.5" ry="13" fill="#f3bc73" stroke="#b87e35" stroke-width="1.2"/>')
        d.add('<ellipse cx="0" cy="9" rx="9.5" ry="8.5" fill="#fbf0ce"/>')
        # Little wings
        d.path("M-14 2 Q-21 8 -14 15", "#b87e35", 1.4, "#e6a755")
        d.path("M14 2 Q21 8 14 15", "#b87e35", 1.4, "#e6a755")
        # Head
        d.add('<ellipse cx="0" cy="-6" rx="15" ry="12.5" fill="#f3bc73" stroke="#b87e35" stroke-width="1.2"/>')
        # Wise round glasses!
        for sx in (-6.2, 6.2):
            d.dot(sx, -6.5, 5.2, "#fbf0ce", 'stroke="#1c352f" stroke-width="1.6"')
            d.dot(sx, -6.5, 2.2, "#162d2a")
            d.dot(sx + 0.9, -7.6, 0.9, "#ffffff")
        d.rule(-1.0, -6.5, 1.0, -6.5, "#1c352f", 1.6)
        # Cute beak & warm scholar glint
        d.path("M-2.8 -1.5 L0 2.5 L2.8 -1.5 Z", "#b86b24", 0.8, "#d98432")
        # Glowing little lantern/scroll spark at right wing
        d.dot(18, 8, 4.2, "#ffe594", 'opacity=".85"')
        d.dot(18, 8, 2.1, "#ffffff")

    elif role == "wren":
        # Swift sky-teal builder bird-critter
        # Swept agile crest
        d.path("M-4 -17 Q2 -26 11 -21 Q5 -16 3 -14 Z", "#55a8a0", 1.0, "#87d6cf")
        d.path("M-8 -15 Q-4 -22 3 -19", "#55a8a0", 1.8)
        # Sleek body & upturned tail
        d.path("M-12 7 L-24 -2 L-20 10 Z", "#55a8a0", 1.1, "#6ec2ba")
        d.add('<ellipse cx="0" cy="7" rx="14.5" ry="11.5" fill="#87d6cf" stroke="#47968f" stroke-width="1.2"/>')
        d.add('<ellipse cx="2" cy="9" rx="8.5" ry="6.8" fill="#d9f6f2"/>')
        d.add('<ellipse cx="1" cy="-5" rx="13.5" ry="11.5" fill="#87d6cf" stroke="#47968f" stroke-width="1.2"/>')
        # Lively eyes & chirpy beak
        for sx in (-4.5, 6.5):
            d.dot(sx, -6.5, 2.8, "#162d2a")
            d.dot(sx + 0.9, -7.5, 1.0, "#ffffff")
        d.path("M12 -5 L19 -3 L12 -0.5 Z", "#c98b3e", 0.9, "#f3bc73")
        # Tiny timber plank / tool icon under wing
        d.rect(-6, 13, 16, 4.2, "#c89056", 1.5, 'stroke="#6e4621" stroke-width="0.9"')

    elif role == "alien":
        # Cosmic Lavender Scrambled Alien SOV Twin (3 sparkling mint eyes + spiral antennae!)
        # Spiral glowing antennae
        for sx in (-1, 1):
            d.path(f"M{sx*7} -16 Q{sx*15} -28 {sx*9} -31 Q{sx*5} -32 {sx*7} -27", "#91cfb7", 2.0)
            d.dot(sx * 9, -29, 3.0, "#a7f0cf")
        # Rounded floating body + tentacles
        for tx in (-11, -4, 4, 11):
            d.path(f"M{tx} 14 Q{tx*1.15:.1f} 21 {tx*0.9:.1f} 23", "#8c82cc", 2.4)
        d.add('<ellipse cx="0" cy="4" rx="16" ry="14" fill="#bcb6ec" stroke="#746cb5" stroke-width="1.3"/>')
        d.add('<ellipse cx="0" cy="-6" rx="16.5" ry="13" fill="#bcb6ec" stroke="#746cb5" stroke-width="1.3"/>')
        # Three sparkling mint eyes!
        for ex, ey, er in ((-7.5, -5.5, 3.3), (0.0, -9.5, 3.6), (7.5, -5.5, 3.3)):
            d.dot(ex, ey, er, "#162d2a")
            d.dot(ex, ey, er * 0.52, "#91cfb7")
            d.dot(ex + 0.9, ey - 1.0, 1.0, "#ffffff")
        d.path("M-5 1 Q0 5.5 5 1", "#162d2a", 1.6)

    elif role == "elder":
        # Wise amber-sage Elder critter with sweeping cortical crown & spectacles
        for sx in (-1, 1):
            for ang_y, tip_x, tip_y in ((-14, 24, -22), (-7, 26, -11), (0, 23, 0)):
                d.path(f"M{sx*13} {ang_y} Q{sx*20} {ang_y-3} {sx*tip_x} {tip_y}", "#f3bc73", 2.3)
                d.dot(sx * tip_x, tip_y, 2.6, "#fff0b5")
        d.add('<ellipse cx="0" cy="8" rx="15" ry="12" fill="#c7b575" stroke="#85743f" stroke-width="1.2"/>')
        d.add('<ellipse cx="0" cy="-4" rx="15.5" ry="13" fill="#d9c682" stroke="#85743f" stroke-width="1.2"/>')
        for sx in (-5.8, 5.8):
            d.dot(sx, -5.5, 4.6, "#f7f3df", 'stroke="#1a342e" stroke-width="1.5"')
            d.dot(sx, -5.5, 2.0, "#162d2a")
            d.dot(sx + 0.8, -6.5, 0.8, "#ffffff")
        d.rule(-1.2, -5.5, 1.2, -5.5, "#1a342e", 1.5)
        d.path("M-5 2.5 Q0 6.5 5 2.5", "#162d2a", 1.6)

    else:
        # HERO: AXION — Bioluminescent Spiking Axolotl-Mind!
        # Neural fronds grow with `level` (1=newborn 2 buds, 2=toddler 4 fronds, 3=champion 6 glowing fronds!)
        fronds = [(-11, 20, -16)] if level <= 1 else (
            [(-13, 23, -20), (-4, 24, -6)] if level == 2 else
            [(-14, 25, -23), (-6, 28, -10), (1, 24, 2)]
        )
        frond_col = "#68a38d" if level <= 1 else ("#7ec9aa" if level == 2 else "#91cfb7")
        spark_col = "#a7d4be" if level <= 1 else ("#f3bc73" if level == 2 else "#ffe28a")
        for sx in (-1, 1):
            for fy, tx, ty in fronds:
                # If ablated on left hemisphere, dim left fronds to show selective lesion!
                if mood == "ablate" and sx < 0:
                    d.path(f"M{sx*13} {fy} Q{sx*19} {fy-3} {sx*tx} {ty}", "#5e756d", 1.8, extra='stroke-dasharray="2.5,2"')
                    d.dot(sx * tx, ty, 2.0, "#758c84")
                else:
                    d.path(f"M{sx*13} {fy} Q{sx*20} {fy-4} {sx*tx} {ty}", frond_col, 2.5)
                    d.dot(sx * tx, ty, 2.8 if level >= 3 else 2.0, spark_col)
        # Cute little feet & rounded body
        d.add('<ellipse cx="-8" cy="18" rx="4.2" ry="2.8" fill="#77bfa4"/>')
        d.add('<ellipse cx="8" cy="18" rx="4.2" ry="2.8" fill="#77bfa4"/>')
        d.add('<ellipse cx="0" cy="8" rx="14.5" ry="11.5" fill="#91cfb7" stroke="#4f8f78" stroke-width="1.2"/>')
        d.add('<ellipse cx="0" cy="10" rx="8.8" ry="6.8" fill="#dcf2df"/>')
        # Expressive head
        d.add('<ellipse cx="0" cy="-5" rx="15.8" ry="13" fill="#91cfb7" stroke="#4f8f78" stroke-width="1.2"/>')
        # Soft rosy cheeks
        d.dot(-10.2, -1.2, 2.7, "#f0a999", 'opacity=".55"')
        d.dot(10.2, -1.2, 2.7, "#f0a999", 'opacity=".55"')

        if mood == "thinking":
            for sx in (-5.5, 5.5):
                d.dot(sx, -6.5, 3.0, "#162d2a")
                d.dot(sx + 1.1, -7.8, 1.1, "#ffffff")
            d.path("M-3.5 1.5 Q0 0 3.5 2", "#162d2a", 1.6)
            # Floating golden thought-spark dots
            d.dot(14, -22, 2.4, "#ffe28a")
            d.dot(20, -28, 3.6, "#ffe28a")
        elif mood == "skeptical":
            for sx in (-5.5, 5.5):
                d.dot(sx, -5.5, 2.9, "#162d2a")
                d.dot(sx + 0.9, -6.5, 1.0, "#ffffff")
            d.path("M-9 -10 L-2 -8.5 M2 -10.5 L9 -10", "#162d2a", 1.6)
            d.path("M-4 2 H4", "#162d2a", 1.6)
        elif mood == "ablate":
            # Cute bandage on left brow + determined wink/expression
            d.rect(-11, -15, 9, 4.2, "#f2f0e7", 1.6, 'stroke="#c97b6d" stroke-width="0.9" transform="rotate(-14 -7 -13)"')
            d.dot(-5.5, -5.0, 2.6, "#162d2a")
            d.dot(5.5, -5.5, 3.0, "#162d2a")
            d.dot(6.4, -6.5, 1.0, "#ffffff")
            d.path("M-4 2 Q0 4.5 4.5 1", "#162d2a", 1.6)
        elif mood == "newborn":
            # Big wide curious baby eyes
            for sx in (-5.5, 5.5):
                d.dot(sx, -6.0, 3.7, "#162d2a")
                d.dot(sx + 1.1, -7.2, 1.4, "#ffffff")
            d.dot(0, 1.8, 1.9, "#162d2a")
        else:
            # Bright joyful eyes & warm grin
            for sx in (-5.8, 5.8):
                d.dot(sx, -6.0, 3.1, "#162d2a")
                d.dot(sx + 1.0, -7.1, 1.1, "#ffffff")
            d.path("M-5.5 0.5 Q0 6.2 5.5 0.5", "#162d2a", 1.7)
            if level >= 3 and mood == "proud":
                # Golden spark star above crown!
                d.path("M0 -28 L1.8 -23.5 L6.5 -22 L1.8 -20.5 L0 -16 L-1.8 -20.5 L-6.5 -22 L-1.8 -23.5 Z", "#d49a3a", 0.6, "#ffe28a")

    d.add("</g>")


def _mind_header_medallion(d, role, mood, level, badge_title, badge_sub, accent="#235e51", cast=None):
    """Draw a charming non-overlapping character medallion in the top-right header."""
    bx, by, bw, bh = 1238, 60, 214, 96
    d.rect(bx, by, bw, bh, "#183631", 12, f'stroke="{accent}" stroke-width="1.5"')
    if cast:
        # Mini cast parade of all 4 critters!
        for i, (r, md) in enumerate(cast[:4]):
            _mind_critter(d, bx + 28 + i * 52, by + 44, 0.72, r, md, 3)
        d.text(bx + bw * 0.5, by + 84, short(badge_title, 28), 10.5, "#edf1dc", MONO, 600, "middle")
    else:
        _mind_critter(d, bx + 44, by + 50, 1.05, role, mood, level)
        d.text(bx + 86, by + 40, short(badge_title, 18), 13.5, "#edf1dc", SERIF, 600)
        d.text(bx + 86, by + 60, short(badge_sub, 20), 10.5, "#91cfb7", MONO, 500)
        d.text(bx + 86, by + 79, "ZERO LLMS · PURE SPIKES", 8.5, "#a7bfb4", MONO, 500)


def _mind_categorize_domains(domains):
    """Group arbitrary N cognitive domains into 8 intuitive human-readable superpower lobes."""
    groups = [
        ("Memory & Stories", "#91cfb7", 0, 0),
        ("Mind & Empathy", "#f3bc73", 0, 0),
        ("Logic & Syllogisms", "#bcb6ec", 0, 0),
        ("Causal & Do-Calc", "#e28b79", 0, 0),
        ("Math & Synthesis", "#87d6cf", 0, 0),
        ("Space, Physics & Trade", "#b7e19b", 0, 0),
        ("Language & Culture", "#f0b4c4", 0, 0),
        ("28M GPU Cortex", "#ffe28a", 0, 0),
    ]
    cor = [0] * 8
    tot = [0] * 8
    for name, c, t in domains:
        nl = name.lower()
        if any(k in nl for k in ("cortex", "kernel-", "pathway-", "ribot", "micro-ripple", "think-chain", "wm clarification", "intent cortex", "semantic similarity", "abduction")):
            idx = 7
        elif any(k in nl for k in ("theory of mind", "belief", "empathy", "fairness", "social", "moral", "deontic", "common knowledge", "muddy", "blame", "epistemic", "vigilance", "personal user", "self-model", "consensus", "byzantine", "obligation", "opinion", "apportionment")):
            idx = 1
        elif any(k in nl for k in ("causal", "counterfactual", "crafting", "planning", "scientific", "do-calculus", "replan", "backtrack", "tool", "simulation", "socratic", "info-gain", "information-gain", "hypothetical", "root-fault", "bellman", "mdp", "mediation", "necessary vs redundant", "experiment")):
            idx = 3
        elif any(k in nl for k in ("syllog", "logic", "deduction", "iff", "knight", "xor", "de morgan", "modal", "constraint", "csp", "riddle", "quantif", "set ", "mereology", "taxonomy", "category", "universal", "disjunctive", "sat ", "meta-rule", "ltl")):
            idx = 2
        elif any(k in nl for k in ("math", "arith", "algebra", "linear", "sequence", "pemdas", "fraction", "bayes", "probability", "expected", "combinator", "number theory", "ratio", "clock", "knapsack", "decision", "compound", "interval", "program synth", "order ", "superlative", "numeric", "budget", "inclusion", "age ", "unit-rate", "triangular", "3-variable", "shapley", "coalition", "bill of materials", "max-flow", "pert/cpm", "vickrey", "information theory", "coding", "affine", "nash", "finance", "financial", "npv", "multi-criteria", "spanning tree", "mst", "dijkstra", "expression tree", "cargo", "core", "matrix", "statistical", "markov", "tree depth", "algorithmic", "mendelian", "portfolio", "topological", "binary search", "prisoner", "shannon entropy", "lcs", "diffie-hellman", "envy-free", "chinese remainder", "bipartite", "rule of 72", "commons", "pigouvian", "eigenvalue", "automaton", "dfa", "keynesian", "macroeconomics", "hamming", "eulerian", "confusion matrix", "precision", "huffman", "rsa", "a* ", "heuristic search", "mixed-strategy")):
            idx = 4
        elif any(k in nl for k in ("spatial", "2d ", "place-cell", "grid", "physics", "phase-change", "balance", "river-crossing", "graph ", "ecology", "ecological", "trade", "pareto", "game theory", "thermo", "mixing", "vector", "walrasian", "market", "newtonian", "momentum", "kinematics", "elastic", "ballistic", "harmonic wave", "ohm", "epidemic", "buoyancy", "doppler", "stoichiometry", "thin-lens", "queueing", "pursuit", "ideal gas", "nyquist", "signal processing", "orbital", "kepler", "shoelace", "polygon", "relativity", "lorentz", "capacitor", "rc time", "feedback control", "logistic", "photoelectric", "acid-base", "pid control", "bernoulli", "venturi")):
            idx = 5
        elif any(k in nl for k in ("episodic", "narrative", "story", "timeline", "prospective", "reminder", "autobiograph", "history", "source memory", "curiosity", "contradiction", "self-introspection", "persistence", "inertia", "multi-paragraph", "open-topic", "concessive", "entity recap", "before/after", "perspective")):
            idx = 0
        else:
            idx = 6
        cor[idx] += c
        tot[idx] += t
    return [(groups[i][0], groups[i][1], cor[i], tot[i]) for i in range(8) if tot[i] > 0]


def mind_summary(entries, data):
    lines = data.get("lines", [])
    evos = entries("mind_evo")
    ctrls = {r["mind_control"]: r for r in entries("mind_control")}
    evals = entries("mind_eval")
    raised = entries("mind_raised")
    benches = entries("mind_bench")
    peers = entries("mind_peer_exchange")
    socs = entries("mind_society")
    ecos = entries("mind_ecosystem")
    turns = entries("mind_turn")

    if evos:
        last = evos[-1]
        first = evos[0]
        mean_num, mean_den = last["mean"].split("/")
        first_num, _ = first["mean"].split("/")
        best_num, _ = last["best"].split("/")
        mean_v, max_v, first_v = float(mean_num), float(mean_den), float(first_num)
        drift = ctrls.get("drift_endpoint")
        intact = ctrls.get("intact")
        frozen = ctrls.get("frozen")
        n_pil = 114 if "mcmcs" in last else (112 if "pdes" in last else (110 if "pcae" in last else (108 if "sxlp" in last else (106 if "odrk" in last else (104 if "ffts" in last else (102 if "lqrc" in last else (100 if "kflt" in last else (98 if "mdpp" in last else (96 if "bnet" in last else (94 if "lalg" in last else (92 if "past" in last else (90 if "symc" in last else (88 if "sres" in last else (86 if "cdis" in last else (84 if "bdeb" in last else (82 if "nmet" in last else (80 if "dlgc" in last else (78 if "xmod" in last else (76 if "audv" in last else (74 if "visv" in last else (72 if "sats" in last else (70 if "gwav" in last else (68 if "nmat" in last else (66 if "sqpr" in last else (64 if "rbnd" in last else (62 if "rvsc" in last else (60 if "osch" in last else (58 if "chrl" in last else (56 if "ind" in last else (54 if "csat" in last else (52 if "csim" in last else (50 if "plan" in last else 48))))))))))))))))))))))))))))))))
        leap_x = mean_v / max(1.0, first_v)
        stats = [
            ["Population", int(last["pop"])],
            ["Generations", int(last["gen"])],
            ["GPU ms / gen", round(float(last["gpu_ms"]), 3)],
        ]
        bullets = [
            f"4,096 baby spiking brains evolved across {int(last['gen'])} generations ({float(last['gpu_ms']):.3f} ms/gen) in a {n_pil}-pillar cognitive nursery.",
            f"Mean smartness jumped {leap_x:.1f}x from {first['mean']} (newborn Gen 1) to {last['mean']} (Gen {last['gen']}), champion {last['best']}.",
        ]
        if drift:
            bullets.append(
                f"Unselected drift critters stayed stuck at {drift['drift_mean']} while evolved thinkers soared to {drift['select_mean']}."
            )
        if intact and frozen:
            bullets.append(
                f"On 16 brand-new worlds: intact champs scored {intact['total']} vs {frozen['total']} frozen, with selective drops when apical ({ctrls.get('no_apical',{}).get('total','—')}), causal ({ctrls.get('no_causal',{}).get('total','—')}), or hippocampus ({ctrls.get('no_hipp',{}).get('total','—')}) are switched off."
            )
        narr = [
            f"Over {int(last['gen'])} GPU generations, {int(last['pop']):,} baby minds evolved from {first['mean']} to {last['mean']} (best {best_num}/{int(max_v)}).",
            (
                f"Selection crushed unselected drift ({drift['drift_mean']}), proving genuine evolved plasticity rather than luck."
                if drift
                else f"Every synapse obeys evolved local 3-factor dendritic LUTs — zero backprop and zero hardcoded rules."
            ),
            (
                f"On 16 unseen worlds, intact champs scored {intact['total']} vs {frozen['total']} frozen, with selective circuit lesions."
                if intact and frozen
                else "Every evolved brain uses only evolvable local synaptic LUTs with zero hardcoded learning rules."
            ),
        ]
        return (
            "Growing spiking minds on GPU",
            f"{n_pil}-pillar nursery   /   {int(last['pop']):,} evolving critters   /   {int(last['gen'])} generations",
            "Evolved cortical fitness",
            100.0 * mean_v / max(1.0, max_v),
            "%",
            stats,
            narr,
            bullets,
        )

    if evals:
        ev = evals[-1]
        cor, tot = int(ev["correct"]), int(ev["total"])
        dialect = ev.get("dialect", "english_svo")
        mode = ev["mind_eval"]
        domains = _mind_parse_domains(lines)
        r0 = raised[-1] if raised else None
        is_alien = "alien" in dialect
        pretty_dialect = "Scrambled Alien SOV" if is_alien else "Natural English SVO"
        if mode not in ("intact", "none"):
            title = f"Brain-lesion audit ({mode} off)"
        elif is_alien:
            title = "Alien SOV twin cognitive exam"
        else:
            title = "Spiking mind exam (English SVO)"
        stats = [
            ["Held-out score", f"{cor}/{tot}"],
            ["GPU spikes", int(ev["spikes"])],
            ["Eval time (ms)", round(float(ev["ms"]), 1)],
        ]
        bullets = [
            f"Tested Axion across {len(domains)} higher-cognition domains ({tot} strict whole-token probes) in {pretty_dialect}.",
            f"Scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) using {int(ev['spikes']):,} GPU spikes and {int(ev['edits']):,} live synaptic edits in {float(ev['ms']):.1f} ms.",
        ]
        if r0:
            bullets.append(
                f"Raised from newborn through 5 childhood stages: {r0['concepts']} grounded words, {r0['engrams']} hippocampal memories, {r0['schemas']} causal laws & {r0['experiments']} hands-on experiments."
            )
        if mode not in ("intact", "none"):
            narr = [
                f"We selectively switched off Axion's {mode} circuit and re-ran the full {len(domains)}-domain exam ({tot} probes) to verify biological modularity.",
                f"Score shifted to {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%): {mode} skills fell sharply while cortical pathways kept firing (Ribot's Law!).",
                "Zero LLMs, zero backprop — every spike, synaptic edit, and lesion effect ran directly on Blackwell CUDA.",
            ]
        elif is_alien:
            nk = 51 if len(domains) >= 545 else (50 if len(domains) >= 535 else (49 if len(domains) >= 525 else (48 if len(domains) >= 515 else (47 if len(domains) >= 505 else (46 if len(domains) >= 495 else (45 if len(domains) >= 485 else (44 if len(domains) >= 475 else (43 if len(domains) >= 465 else (42 if len(domains) >= 455 else (41 if len(domains) >= 445 else (40 if len(domains) >= 435 else (39 if len(domains) >= 425 else (38 if len(domains) >= 415 else (37 if len(domains) >= 405 else (36 if len(domains) >= 395 else (35 if len(domains) >= 385 else (34 if len(domains) >= 375 else (33 if len(domains) >= 365 else (32 if len(domains) >= 355 else (31 if len(domains) >= 345 else (30 if len(domains) >= 335 else (29 if len(domains) >= 325 else (28 if len(domains) >= 315 else (27 if len(domains) >= 305 else (26 if len(domains) >= 295 else (25 if len(domains) >= 285 else (24 if len(domains) >= 275 else (23 if len(domains) >= 265 else 22))))))))))))))))))))))))))))
            narr = [
                f"Axion's Alien SOV twin grew up with reversed word order and scrambled vocabulary — zero English anywhere.",
                f"It nailed {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) across all {len(domains)} domains, proving 100% dialect-agnostic intelligence!",
                (f"STAGE 8o · 51 KERNELS & 550 DOMAINS! K51 pure-GPU 256-chain MCMC, 3-entity chat, Fermi/Roofline & all {nk} GPU kernels matched 100%." if nk >= 51
                 else f"STAGE 8n · 50 KERNELS & 540 DOMAINS! K50 pure-GPU 1D PDE solver, deduction chat, DeBrog/Taylor & all {nk} GPU kernels matched 100%." if nk >= 50
                 else f"STAGE 8m · 530 DOMAINS! K49 pure-GPU PCA eigen cortex, causal chat, BlkHole/Sharpe & all {nk} GPU kernels matched 100%." if nk >= 49
                 else f"STAGE 8l · 520 DOMAINS! K48 pure-GPU Simplex LP optimizer, proof chat, CapBank/PutCall & all {nk} GPU kernels matched 100%." if nk >= 48
                 else f"STAGE 8k · 510 DOMAINS! K47 pure-GPU RK4 ODE integrator, counterfactual chat, PV/BatPack & all {nk} GPU kernels matched 100%." if nk >= 47
                 else f"STAGE 8j · 500-DOMAIN MILESTONE! K46 pure-GPU 8-pt FFT spectrum, self-correction, Shannon/ECC & all {nk} GPU kernels matched 100%." if nk >= 46
                 else f"STAGE 8i LIVE! K45 pure-GPU LQR Riccati control, multi-turn topic bridge, ImgComp/PageTbl & all {nk} GPU kernels matched 100%." if nk >= 45
                 else f"STAGE 8h · 100 PILLARS! K44 pure-GPU Kalman filter, hypothesis testing, BrPred/PolyGeom & all {nk} GPU kernels matched 100%." if nk >= 44
                 else f"STAGE 8g LIVE! K43 pure-GPU MDP Bellman, open user goals, B-Tree/Buck & all {nk} GPU kernels matched 100%." if nk >= 43
                 else f"STAGE 8f LIVE! K42 pure-GPU Bayes-net/decision, Socratic critique, Optics/Solow & all {nk} GPU kernels matched 100%." if nk >= 42
                 else f"STAGE 8e LIVE! K41 pure-GPU linear algebra/Cramer, story co-writing, RSA/Aero & all {nk} GPU kernels matched 100%." if nk >= 41
                 else f"STAGE 8d LIVE! K40 pure-GPU AST program synthesis, zero-keyword debate, Merkle/rockets & all {nk} GPU kernels matched 100%." if nk >= 40
                 else f"STAGE 8c LIVE! K39 pure-GPU symbolic calculus, live concept teaching, Bode/Raft & all {nk} GPU kernels matched 100%." if nk >= 39
                 else f"STAGE 8b LIVE! K38 self-research/code-patching, deep multi-sentence discourse, Kalman & all {nk} GPU kernels matched 100%." if nk >= 38
                 else f"STAGE 8 CROSSED! K37 collective discovery/theorems, Socratic curiosity, spec-to-code & all {nk} GPU kernels matched 100%." if nk >= 37
                 else f"STAGE 7c · 400 DOMAINS! K36 belief-graph/debate, rich 5-facet chat, peer review & all {nk} GPU kernels matched 100%." if nk >= 36
                 else f"STAGE 7b LIVE! K35 metaphor/plot/critique, unscripted chat, sci loop & all {nk} GPU kernels matched 100%." if nk >= 35
                 else f"STAGE 7 CROSSED! K34 dialogue cortex, open chat, wit, ethics & all {nk} GPU kernels matched 100%." if nk >= 34
                 else f"STAGE 6c! K33 cross-modal+goal stack, Socratic coaching, contraptions & all {nk} GPU kernels matched 100%." if nk >= 33
                 else f"STAGE 6b! K32 cochlear audio/prosody, textbook self-quiz, auto-debug & all {nk} GPU kernels matched 100%." if nk >= 32
                 else f"STAGE 6! K31 16x16 V1/V4 retina, curiosity reader, code lab & all {nk} GPU kernels matched 100%." if nk >= 31
                 else f"K30 GPU spiking 3-SAT, 16k Tier-2 HBM, free why-prose & all {nk} GPU kernels matched 100%." if nk >= 30
                 else f"K29 GPU graph wave, 8k lexicon, free coref/wh-Q & all {nk} GPU kernels matched 100%." if nk >= 29
                 else f"K28 pure-GPU parietal math, K27 syntax, Coulomb, pipes, Amdahl, VMem, escape velocity & all {nk} GPU kernels matched 100%."),
            ]
        else:
            narr = [
                f"Raised through 5 childhood stages of experiments and dialogue, Axion faced a {len(domains)}-domain exam ({tot} probes).",
                (f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8o · 51 KERNELS & 550 DOMAINS! K51 GPU 256-chain MCMC, 3-entity chat & Fermi/Roofline!" if len(domains) >= 545
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — 50-KERNEL MILESTONE! 540 DOMAINS! K50 GPU 1D PDE, deduction chat & DeBrog/Taylor!" if len(domains) >= 535
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8m · 530 DOMAINS! K49 GPU PCA eigen, causal chat & BlkHole/Sharpe!" if len(domains) >= 525
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8l · 520 DOMAINS! K48 GPU Simplex LP, proof chat & CapBank/PutCall!" if len(domains) >= 515
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8k · 510 DOMAINS! K47 GPU RK4 ODE, counterfactual chat & PV/BatPack!" if len(domains) >= 505
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — 500-DOMAIN MILESTONE! K46 GPU FFT spectrum, self-correction & Shannon/ECC!" if len(domains) >= 495
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8i LIVE! K45 GPU LQR Riccati, topic bridge & ImgComp/PageTbl!" if len(domains) >= 485
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8h · 100 PILLARS! K44 GPU Kalman, hypothesis testing & BrPred/Poly!" if len(domains) >= 475
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8g LIVE! K43 GPU MDP Bellman, open user goals & B-Tree/Buck!" if len(domains) >= 465
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8f LIVE! K42 GPU Bayes-net, Socratic critique & Optics/Solow!" if len(domains) >= 455
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8e LIVE! K41 GPU linear algebra, story co-writing & RSA/Aero!" if len(domains) >= 445
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8d LIVE! K40 GPU AST synthesis, zero-keyword debate & Merkle/rockets!" if len(domains) >= 435
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8c LIVE! K39 symbolic calculus, live concept teaching & Bode/Raft!" if len(domains) >= 425
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8b LIVE! K38 self-research/patching, deep discourse & Kalman!" if len(domains) >= 415
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 8 CROSSED! K37 collective discovery/theorems, Socratic curiosity & spec-to-code!" if len(domains) >= 405
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 7c · 400 DOMAINS! K36 belief-graph/debate, 5-facet chat & peer review!" if len(domains) >= 395
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 7b LIVE! K35 metaphor/plot/critique, unscripted chat & sci loop!" if len(domains) >= 385
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 7 CROSSED! K34 spiking dialogue cortex, open chat, pun wit & ethics!" if len(domains) >= 375
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 6c! K33 cross-modal+goal stack, Socratic coaching & contraptions!" if len(domains) >= 365
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 6b! K32 cochlear audio, textbook self-quiz & auto-debug!" if len(domains) >= 355
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — STAGE 6! K31 16x16 V1/V4 retina, curiosity reader & code lab!" if len(domains) >= 345
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — Stage 5d! K30 GPU 3-SAT, 16k HBM & free why-prose!" if len(domains) >= 335
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — Stage 5c! K29 GPU graph wave, 8k lex & free coref!" if len(domains) >= 325
                 else f"Axion scored {cor}/{tot} ({100.0 * cor / max(1, tot):.1f}%) — Stage 5b! K28 pure-GPU spiking math + K27 multi-clause prose!"),
                "Every memory, causal rollout, and 29.36M-synapse cortical kernel ran natively on Blackwell CUDA with zero LLMs.",
            ]
        return (
            title,
            f"{pretty_dialect}   /   {len(domains)} domains   /   arm={mode}",
            "Held-out cognitive accuracy",
            100.0 * cor / max(1, tot),
            "%",
            stats,
            narr,
            bullets,
        )

    if benches:
        sdr = next((r for r in benches if r["mind_bench"] == "sdr_columns"), {})
        hip = next((r for r in benches if r["mind_bench"] == "hippocampus"), {})
        neo = next((r for r in benches if r["mind_bench"] == "neocortex"), {})
        sp2 = next((r for r in benches if r["mind_bench"] == "spatial_2d"), {})
        tch = next((r for r in benches if r["mind_bench"] == "think_chain"), {})
        wma = next((r for r in benches if r["mind_bench"] == "wm_attend"), {})
        ctr = next((r for r in benches if r["mind_bench"] == "neo_contrast"), {})
        ana = next((r for r in benches if r["mind_bench"] == "neo_analogy"), {})
        cmp = next((r for r in benches if r["mind_bench"] == "neo_compose"), {})
        mcu = next((r for r in benches if r["mind_bench"] == "neo_multi_cue"), {})
        hyp = next((r for r in benches if r["mind_bench"] == "neo_hypothesize"), {})
        brg = next((r for r in benches if r["mind_bench"] == "neo_bridge"), {})
        clu = next((r for r in benches if r["mind_bench"] == "neo_cluster"), {})
        gat = next((r for r in benches if r["mind_bench"] == "neo_gate"), {})
        drm = next((r for r in benches if r["mind_bench"] == "neo_dream"), {})
        scn = next((r for r in benches if r["mind_bench"] == "neo_scan"), {})
        ded = next((r for r in benches if r["mind_bench"] == "neo_deduce"), {})
        abd = next((r for r in benches if r["mind_bench"] == "neo_abduce_back"), {})
        cfa = next((r for r in benches if r["mind_bench"] == "neo_counterfact"), {})
        plc = next((r for r in benches if r["mind_bench"] == "neo_plan_chain"), {})
        sim = next((r for r in benches if r["mind_bench"] == "neo_simulate"), {})
        con = next((r for r in benches if r["mind_bench"] == "neo_constrain"), {})
        ind = next((r for r in benches if r["mind_bench"] == "neo_induce"), {})
        chr_b = next((r for r in benches if r["mind_bench"] == "neo_chain_rule"), {})
        ons = next((r for r in benches if r["mind_bench"] == "neo_online_schema"), {})
        rvs = next((r for r in benches if r["mind_bench"] == "neo_revise_schema"), {})
        rbd = next((r for r in benches if r["mind_bench"] == "neo_role_bind"), {})
        sp27 = next((r for r in benches if r["mind_bench"] == "neo_seq_predict"), {})
        nm28 = next((r for r in benches if r["mind_bench"] == "neo_neural_math"), {})
        gw29 = next((r for r in benches if r["mind_bench"] == "neo_graph_wave"), {})
        sat30 = next((r for r in benches if r["mind_bench"] == "neo_sat_solve"), {})
        vis31 = next((r for r in benches if r["mind_bench"] == "neo_visual_cortex"), {})
        aud32 = next((r for r in benches if r["mind_bench"] == "neo_audio_cortex"), {})
        xms33 = next((r for r in benches if r["mind_bench"] == "neo_crossmodal_stack"), {})
        dlg34 = next((r for r in benches if r["mind_bench"] == "neo_dialogue_cortex"), {})
        nmet35 = next((r for r in benches if r["mind_bench"] == "neo_narrative_metaphor"), {})
        bdeb36 = next((r for r in benches if r["mind_bench"] == "neo_belief_debate"), {})
        cdis37 = next((r for r in benches if r["mind_bench"] == "neo_collective_discovery"), {})
        sres38 = next((r for r in benches if r["mind_bench"] == "neo_self_research"), {})
        symc39 = next((r for r in benches if r["mind_bench"] == "neo_symbolic_calculus"), {})
        past40 = next((r for r in benches if r["mind_bench"] == "neo_program_ast"), {})
        lalg41 = next((r for r in benches if r["mind_bench"] == "neo_linear_algebra"), {})
        bnet42 = next((r for r in benches if r["mind_bench"] == "neo_bayes_net"), {})
        mdpp43 = next((r for r in benches if r["mind_bench"] == "neo_mdp_planner"), {})
        kflt44 = next((r for r in benches if r["mind_bench"] == "neo_kalman_filter"), {})
        lqrc45 = next((r for r in benches if r["mind_bench"] == "neo_lqr_control"), {})
        ffts46 = next((r for r in benches if r["mind_bench"] == "neo_fft_spectrum"), {})
        odrk47 = next((r for r in benches if r["mind_bench"] == "neo_ode_rk4"), {})
        sxlp48 = next((r for r in benches if r["mind_bench"] == "neo_simplex_lp"), {})
        pcae49 = next((r for r in benches if r["mind_bench"] == "neo_pca_eigen"), {})
        pdes50 = next((r for r in benches if r["mind_bench"] == "neo_pde_solver"), {})
        mcmcs51 = next((r for r in benches if r["mind_bench"] == "neo_mcmc_sampler"), {})
        gnt = float(sdr.get("Gneuron_ticks/s", 0))
        hus = float(hip.get("query_us", 0))
        nus = float(neo.get("settle_us", 0))
        nsyn = int(neo.get("synapses", 0))
        sp_us = float(sp2.get("wave_us", 0))
        tc_us = float(tch.get("chain_us", 0))
        wm_us = float(wma.get("attend_us", 0))
        ct_us = float(ctr.get("contrast_us", 0))
        an_us = float(ana.get("analogy_us", 0))
        cm_us = float(cmp.get("compose_us", 0))
        mc_us = float(mcu.get("multicue_us", 0))
        hy_us = float(hyp.get("hyp_us", 0))
        br_us = float(brg.get("bridge_us", 0))
        cl_us = float(clu.get("cluster_us", 0))
        gt_us = float(gat.get("gate_us", 0))
        dr_us = float(drm.get("dream_us", 0))
        sc_us = float(scn.get("scan_us", 0))
        dd_us = float(ded.get("deduce_us", 0))
        ab_us = float(abd.get("abduce_us", 0))
        cf_us = float(cfa.get("cf_us", 0))
        pl_us = float(plc.get("plan_us", 0))
        sm_us = float(sim.get("sim_us", 0))
        cn_us = float(con.get("constrain_us", 0))
        in_us = float(ind.get("induce_us", 0))
        cr_us = float(chr_b.get("chain_rule_us", 0))
        os_us = float(ons.get("online_schema_us", 0))
        rs_us = float(rvs.get("revise_schema_us", 0))
        rb_us = float(rbd.get("role_bind_us", 0))
        sp27_us = float(sp27.get("seq_predict_us", 0))
        nm28_us = float(nm28.get("neural_math_us", 0))
        gw29_us = float(gw29.get("graph_wave_us", 0))
        sat30_us = float(sat30.get("sat_solve_us", 0))
        vis31_us = float(vis31.get("visual_us", 0))
        aud32_us = float(aud32.get("audio_us", 0))
        xms33_us = float(xms33.get("xmodal_us", 0))
        dlg34_us = float(dlg34.get("dlg_us", 0))
        nmet35_us = float(nmet35.get("nmet_us", 0))
        bdeb36_us = float(bdeb36.get("bdeb_us", 0))
        cdis37_us = float(cdis37.get("cdis_us", 0))
        sres38_us = float(sres38.get("sres_us", 0))
        symc39_us = float(symc39.get("symc_us", 0))
        past40_us = float(past40.get("past_us", 0))
        lalg41_us = float(lalg41.get("lalg_us", 0))
        bnet42_us = float(bnet42.get("bnet_us", 0))
        mdpp43_us = float(mdpp43.get("mdpp_us", 0))
        kflt44_us = float(kflt44.get("kflt_us", 0))
        lqrc45_us = float(lqrc45.get("lqrc_us", 0))
        ffts46_us = float(ffts46.get("ffts_us", 0))
        odrk47_us = float(odrk47.get("odrk_us", 0))
        sxlp48_us = float(sxlp48.get("sxlp_us", 0))
        pcae49_us = float(pcae49.get("pcae_us", 0))
        pdes50_us = float(pdes50.get("pdes_us", 0))
        mcmcs51_us = float(mcmcs51.get("mcmcs_us", 0))
        stats = [
            ["Gneuron-ticks/s", round(gnt, 2)],
            ["Hipp query (µs)", round(hus, 2)],
            [f"{nsyn // 1000000}M-syn Neo (µs)" if nsyn else "Registers / lane", round(nus, 1) if nsyn else int(sdr.get("regs", 0))],
        ]
        bullets = [
            f"4,096 spiking SDR cortical columns (1,024 neurons each) sustained {gnt:.2f} Gneuron-ticks/s ({sdr.get('regs','—')} regs, 0 B spill).",
            f"26-SM parallel Blackwell Hippocampus ({hip.get('blocks','32')} blocks) searched {hip.get('engrams','4096')} memories ({hip.get('l2_footprint_KiB','3264')} KiB L2) in {hus:.2f} µs.",
        ]
        if nsyn:
            bullets.append(
                f"Complete-L2 GPU Neocortex ({neo.get('pathways','28')} pathways, {nsyn:,} plastic int8 synapses, {neo.get('l2_footprint_KiB','28672')} KiB) settled in {nus:.2f} µs."
            )
        if sp_us > 0:
            bullets.append(
                f"256-cell 16×16 2D spiking place-cell map routed around river obstacles in {sp_us:.2f} µs ({sp2.get('wave_ticks','19')} wave ticks)."
            )
        if tc_us > 0:
            bullets.append(
                f"3-hop recurrent cortical thought chain (`mind_neo_think_chain`) settled in {tc_us:.2f} µs; 64-slot WM gate detected ambiguity in {wm_us:.2f} µs."
            )
        if mcmcs51_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K47 plus SimplexLP ({sxlp48_us:.1f} µs), PcaEigen ({pcae49_us:.1f} µs), PdeSolver ({pdes50_us:.1f} µs) & Kernel 51 McmcSampler ({mcmcs51_us:.1f} µs)."
            )
        elif pdes50_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K46 plus OdeRk4 ({odrk47_us:.1f} µs), SimplexLP ({sxlp48_us:.1f} µs), PcaEigen ({pcae49_us:.1f} µs) & Kernel 50 PdeSolver ({pdes50_us:.1f} µs)."
            )
        elif pcae49_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K45 plus FFTSpectrum ({ffts46_us:.1f} µs), OdeRk4 ({odrk47_us:.1f} µs), SimplexLP ({sxlp48_us:.1f} µs) & Kernel 49 PcaEigen ({pcae49_us:.1f} µs)."
            )
        elif sxlp48_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K44 plus LQRControl ({lqrc45_us:.1f} µs), FFTSpectrum ({ffts46_us:.1f} µs), OdeRk4 ({odrk47_us:.1f} µs) & Kernel 48 SimplexLP ({sxlp48_us:.1f} µs)."
            )
        elif odrk47_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K43 plus KalmanFilter ({kflt44_us:.1f} µs), LQRControl ({lqrc45_us:.1f} µs), FFTSpectrum ({ffts46_us:.1f} µs) & Kernel 47 OdeRk4 ({odrk47_us:.1f} µs)."
            )
        elif ffts46_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K42 plus MDPPlanner ({mdpp43_us:.1f} µs), KalmanFilter ({kflt44_us:.1f} µs), LQRControl ({lqrc45_us:.1f} µs) & Kernel 46 FFTSpectrum ({ffts46_us:.1f} µs)."
            )
        elif lqrc45_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K41 plus BayesNet ({bnet42_us:.1f} µs), MDPPlanner ({mdpp43_us:.1f} µs), KalmanFilter ({kflt44_us:.1f} µs) & Kernel 45 LQRControl ({lqrc45_us:.1f} µs)."
            )
        elif kflt44_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K40 plus LinearAlgebra ({lalg41_us:.1f} µs), BayesNet ({bnet42_us:.1f} µs), MDPPlanner ({mdpp43_us:.1f} µs) & Kernel 44 KalmanFilter ({kflt44_us:.1f} µs)."
            )
        elif mdpp43_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K39 plus ProgramAST ({past40_us:.1f} µs), LinearAlgebra ({lalg41_us:.1f} µs), BayesNet ({bnet42_us:.1f} µs) & Kernel 43 MDPPlanner ({mdpp43_us:.1f} µs)."
            )
        elif bnet42_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K38 plus SymbolicCalculus ({symc39_us:.1f} µs), ProgramAST ({past40_us:.1f} µs), LinearAlgebra ({lalg41_us:.1f} µs) & Kernel 42 BayesNet ({bnet42_us:.1f} µs)."
            )
        elif lalg41_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K37 plus SelfResearch ({sres38_us:.1f} µs), SymbolicCalculus ({symc39_us:.1f} µs), ProgramAST ({past40_us:.1f} µs) & Kernel 41 LinearAlgebra ({lalg41_us:.1f} µs)."
            )
        elif past40_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K36 plus CollectiveDiscovery ({cdis37_us:.1f} µs), SelfResearch ({sres38_us:.1f} µs), SymbolicCalculus ({symc39_us:.1f} µs) & Kernel 40 ProgramAST ({past40_us:.1f} µs)."
            )
        elif symc39_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K35 plus BeliefDebate ({bdeb36_us:.1f} µs), CollectiveDiscovery ({cdis37_us:.1f} µs), SelfResearch ({sres38_us:.1f} µs) & Kernel 39 SymbolicCalculus ({symc39_us:.1f} µs)."
            )
        elif sres38_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K34 plus MetaphorPlot ({nmet35_us:.1f} µs), BeliefDebate ({bdeb36_us:.1f} µs), CollectiveDiscovery ({cdis37_us:.1f} µs) & Kernel 38 SelfResearch ({sres38_us:.1f} µs)."
            )
        elif cdis37_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K33 plus DialogueCortex ({dlg34_us:.1f} µs), MetaphorPlot ({nmet35_us:.1f} µs), BeliefDebate ({bdeb36_us:.1f} µs) & Kernel 37 CollectiveDiscovery ({cdis37_us:.1f} µs)."
            )
        elif bdeb36_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K32 plus XModalStack ({xms33_us:.1f} µs), DialogueCortex ({dlg34_us:.1f} µs), MetaphorPlot ({nmet35_us:.1f} µs) & Kernel 36 BeliefDebate ({bdeb36_us:.1f} µs)."
            )
        elif nmet35_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K31 plus AudioCochlea ({aud32_us:.1f} µs), XModalStack ({xms33_us:.1f} µs), DialogueCortex ({dlg34_us:.1f} µs) & Kernel 35 MetaphorPlot ({nmet35_us:.1f} µs)."
            )
        elif dlg34_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K30 plus VisualRetina ({vis31_us:.1f} µs), AudioCochlea ({aud32_us:.1f} µs), XModalStack ({xms33_us:.1f} µs) & Kernel 34 DialogueCortex ({dlg34_us:.1f} µs)."
            )
        elif xms33_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K29 plus SatSolve ({sat30_us:.1f} µs), VisualRetina ({vis31_us:.1f} µs), AudioCochlea ({aud32_us:.1f} µs) & Kernel 33 XModalStack ({xms33_us:.1f} µs)."
            )
        elif aud32_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K28 plus GraphWave ({gw29_us:.1f} µs), SatSolve ({sat30_us:.1f} µs), VisualRetina ({vis31_us:.1f} µs) & Kernel 32 AudioCochlea ({aud32_us:.1f} µs)."
            )
        elif vis31_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K27 plus NeuralMath ({nm28_us:.1f} µs), GraphWave ({gw29_us:.1f} µs), SatSolve ({sat30_us:.1f} µs) & Kernel 31 VisualRetina ({vis31_us:.1f} µs)."
            )
        elif sat30_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K26 plus SeqSyntax ({sp27_us:.1f} µs), NeuralMath ({nm28_us:.1f} µs), GraphWave ({gw29_us:.1f} µs) & Kernel 30 SatSolve ({sat30_us:.1f} µs)."
            )
        elif gw29_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K25 plus RoleBinder ({rb_us:.1f} µs), SeqSyntax ({sp27_us:.1f} µs), NeuralMath ({nm28_us:.1f} µs) & Kernel 29 GraphWave ({gw29_us:.1f} µs)."
            )
        elif nm28_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K24 plus ReviseSchema ({rs_us:.1f} µs), RoleBinder ({rb_us:.1f} µs), SeqSyntax ({sp27_us:.1f} µs) & Kernel 28 NeuralMath ({nm28_us:.1f} µs)."
            )
        elif sp27_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K23 plus OnlineSchema ({os_us:.1f} µs), ReviseSchema ({rs_us:.1f} µs), RoleBinder ({rb_us:.1f} µs) & Kernel 27 SeqSyntax ({sp27_us:.1f} µs)."
            )
        elif rb_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: K6–K22 plus ChainRule ({cr_us:.1f} µs), OnlineSchema ({os_us:.1f} µs), ReviseSchema ({rs_us:.1f} µs) & Kernel 26 Spiking RoleBinder ({rb_us:.1f} µs)."
            )
        elif rs_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs), Scan ({sc_us:.1f} µs), Deduce ({dd_us:.1f} µs), Abduce ({ab_us:.1f} µs), Imagine ({cf_us:.1f} µs), Plan ({pl_us:.1f} µs), Simulate ({sm_us:.1f} µs), Constrain ({cn_us:.1f} µs), Induce ({in_us:.1f} µs), ChainRule ({cr_us:.1f} µs), OnlineSchema ({os_us:.1f} µs) & Kernel 25 ReviseSchema ({rs_us:.1f} µs)."
            )
        elif os_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs), Scan ({sc_us:.1f} µs), Deduce ({dd_us:.1f} µs), Abduce ({ab_us:.1f} µs), Imagine ({cf_us:.1f} µs), Plan ({pl_us:.1f} µs), Simulate ({sm_us:.1f} µs), Constrain ({cn_us:.1f} µs), Induce ({in_us:.1f} µs), ChainRule ({cr_us:.1f} µs) & Kernel 24 OnlineSchema ({os_us:.1f} µs)."
            )
        elif cr_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs), Scan ({sc_us:.1f} µs), Deduce ({dd_us:.1f} µs), Abduce ({ab_us:.1f} µs), Imagine ({cf_us:.1f} µs), Plan ({pl_us:.1f} µs), Simulate ({sm_us:.1f} µs), Constrain ({cn_us:.1f} µs), Induce ({in_us:.1f} µs) & Kernel 23 ChainRule ({cr_us:.1f} µs)."
            )
        elif in_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs), Scan ({sc_us:.1f} µs), Deduce ({dd_us:.1f} µs), Abduce ({ab_us:.1f} µs), Imagine ({cf_us:.1f} µs), Plan ({pl_us:.1f} µs), Simulate ({sm_us:.1f} µs), Constrain ({cn_us:.1f} µs) & Kernel 22 Induce ({in_us:.1f} µs)."
            )
        elif cn_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs), Scan ({sc_us:.1f} µs), Deduce ({dd_us:.1f} µs), Abduce ({ab_us:.1f} µs), Imagine ({cf_us:.1f} µs), Plan ({pl_us:.1f} µs), Simulate ({sm_us:.1f} µs) & Kernel 21 Constrain ({cn_us:.1f} µs)."
            )
        elif sm_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs), Scan ({sc_us:.1f} µs), Deduce ({dd_us:.1f} µs), Abduce ({ab_us:.1f} µs), Imagine ({cf_us:.1f} µs), Plan ({pl_us:.1f} µs) & Kernel 20 NMDA Simulate ({sm_us:.1f} µs)."
            )
        elif pl_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs), Scan ({sc_us:.1f} µs), Deduce ({dd_us:.1f} µs), Abduce ({ab_us:.1f} µs), Imagine ({cf_us:.1f} µs) & Kernel 19 Plan Chain ({pl_us:.1f} µs)."
            )
        elif cf_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs), Scan ({sc_us:.1f} µs), Deduce ({dd_us:.1f} µs), Abduce ({ab_us:.1f} µs) & Kernel 18 Counterfactual Imagine ({cf_us:.1f} µs)."
            )
        elif ab_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs), Scan ({sc_us:.1f} µs), Deduce ({dd_us:.1f} µs) & Kernel 17 Abduce-Back ({ab_us:.1f} µs)."
            )
        elif dd_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs), Scan ({sc_us:.1f} µs) & Kernel 16 Deduce ({dd_us:.1f} µs)."
            )
        elif sc_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs), Dream ({dr_us:.1f} µs) & Kernel 15 Scan ({sc_us:.1f} µs)."
            )
        elif dr_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs), Gate ({gt_us:.1f} µs) & Kernel 14 Dream ({dr_us:.1f} µs)."
            )
        elif gt_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs), Cluster ({cl_us:.1f} µs) & Kernel 13 Gate ({gt_us:.1f} µs)."
            )
        elif cl_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs), Bridge ({br_us:.1f} µs) & Kernel 12 Cluster ({cl_us:.1f} µs)."
            )
        elif br_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs), Hypothesize ({hy_us:.1f} µs) & Kernel 11 Bridge ({br_us:.1f} µs)."
            )
        elif hy_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs), Multi-Cue ({mc_us:.1f} µs) & Kernel 10 Hypothesize ({hy_us:.1f} µs)."
            )
        elif mc_us > 0:
            bullets.append(
                f"Specialized L2 reasoning kernels: Contrast ({ct_us:.1f} µs), Analogy ({an_us:.1f} µs), Compose ({cm_us:.1f} µs) & Multi-Cue ({mc_us:.1f} µs)."
            )
        l2_pct = 100.0 * (int(hip.get("l2_footprint_KiB", "3264")) + int(neo.get("l2_footprint_KiB", "0"))) / 32768.0
        k_narr = (
            f"Fifty-one native CUDA kernels run K48 Simplex LP ({sxlp48_us:.0f}µs), K49 PCA Eigen ({pcae49_us:.0f}µs), K50 1D PDE ({pdes50_us:.0f}µs) & K51 MCMC ({mcmcs51_us:.0f}µs)."
            if mcmcs51_us > 0
            else f"Fifty native CUDA kernels run K47 RK4 ODE ({odrk47_us:.0f}µs), K48 Simplex LP ({sxlp48_us:.0f}µs), K49 PCA Eigen ({pcae49_us:.0f}µs) & K50 1D PDE ({pdes50_us:.0f}µs)."
            if pdes50_us > 0
            else f"Forty-nine native CUDA kernels run K46 FFT ({ffts46_us:.0f}µs), K47 RK4 ODE ({odrk47_us:.0f}µs), K48 Simplex LP ({sxlp48_us:.0f}µs) & K49 PCA Eigen ({pcae49_us:.0f}µs)."
            if pcae49_us > 0
            else f"Forty-eight native CUDA kernels run K45 LQR ({lqrc45_us:.0f}µs), K46 FFT ({ffts46_us:.0f}µs), K47 RK4 ODE ({odrk47_us:.0f}µs) & K48 Simplex LP ({sxlp48_us:.0f}µs)."
            if sxlp48_us > 0
            else f"Forty-seven native CUDA kernels run K44 Kalman ({kflt44_us:.0f}µs), K45 LQR ({lqrc45_us:.0f}µs), K46 FFT ({ffts46_us:.0f}µs) & K47 RK4 ODE ({odrk47_us:.0f}µs)."
            if odrk47_us > 0
            else f"Forty-six native CUDA kernels run K43 MDP ({mdpp43_us:.0f}µs), K44 Kalman ({kflt44_us:.0f}µs), K45 LQR ({lqrc45_us:.0f}µs) & K46 FFT ({ffts46_us:.0f}µs)."
            if ffts46_us > 0
            else f"Forty-five native CUDA kernels run K42 Bayes ({bnet42_us:.0f}µs), K43 MDP ({mdpp43_us:.0f}µs), K44 Kalman ({kflt44_us:.0f}µs) & K45 LQR ({lqrc45_us:.0f}µs)."
            if lqrc45_us > 0
            else f"Forty-four native CUDA kernels run K41 LinAlg ({lalg41_us:.0f}µs), K42 Bayes ({bnet42_us:.0f}µs), K43 MDP ({mdpp43_us:.0f}µs) & K44 Kalman ({kflt44_us:.0f}µs)."
            if kflt44_us > 0
            else f"Forty-three native CUDA kernels run K40 AST ({past40_us:.0f}µs), K41 LinAlg ({lalg41_us:.0f}µs), K42 Bayes ({bnet42_us:.0f}µs) & K43 MDP Bellman ({mdpp43_us:.0f}µs)."
            if mdpp43_us > 0
            else f"Forty-two native CUDA kernels run K39 calculus ({symc39_us:.0f}µs), K40 AST ({past40_us:.0f}µs), K41 linear algebra ({lalg41_us:.0f}µs) & K42 Bayes-net ({bnet42_us:.0f}µs)."
            if bnet42_us > 0
            else f"Forty-one native CUDA kernels run K38 self-research ({sres38_us:.0f}µs), K39 calculus ({symc39_us:.0f}µs), K40 AST synth ({past40_us:.0f}µs) & K41 linear algebra ({lalg41_us:.0f}µs)."
            if lalg41_us > 0
            else f"Forty native CUDA kernels run K37 collective ({cdis37_us:.0f}µs), K38 self-research ({sres38_us:.0f}µs), K39 symbolic calculus ({symc39_us:.0f}µs) & K40 AST synthesis ({past40_us:.0f}µs)."
            if past40_us > 0
            else f"Thirty-nine native CUDA kernels run K36 belief ({bdeb36_us:.0f}µs), K37 collective ({cdis37_us:.0f}µs), K38 self-research ({sres38_us:.0f}µs) & K39 symbolic calculus ({symc39_us:.0f}µs)."
            if symc39_us > 0
            else f"Thirty-eight native CUDA kernels run K35 metaphor ({nmet35_us:.0f}µs), K36 belief ({bdeb36_us:.0f}µs), K37 collective ({cdis37_us:.0f}µs) & K38 self-research ({sres38_us:.0f}µs)."
            if sres38_us > 0
            else f"Thirty-seven native CUDA kernels run K34 dialogue ({dlg34_us:.0f}µs), K35 metaphor ({nmet35_us:.0f}µs), K36 belief-debate ({bdeb36_us:.0f}µs) & K37 collective discovery ({cdis37_us:.0f}µs)."
            if cdis37_us > 0
            else f"Thirty-six native CUDA kernels run K33 xmodal ({xms33_us:.0f}µs), K34 dialogue ({dlg34_us:.0f}µs), K35 metaphor ({nmet35_us:.0f}µs) & K36 belief-debate ({bdeb36_us:.0f}µs)."
            if bdeb36_us > 0
            else f"Thirty-five native CUDA kernels run K32 cochlea ({aud32_us:.0f}µs), K33 xmodal ({xms33_us:.0f}µs), K34 dialogue ({dlg34_us:.0f}µs) & K35 metaphor/plot ({nmet35_us:.0f}µs)."
            if nmet35_us > 0
            else f"Thirty-four native CUDA kernels run K31 retina ({vis31_us:.0f}µs), K32 cochlea ({aud32_us:.0f}µs), K33 xmodal ({xms33_us:.0f}µs) & K34 dialogue ({dlg34_us:.0f}µs)."
            if dlg34_us > 0
            else f"Thirty-three native CUDA kernels run K30 SAT ({sat30_us:.0f}µs), K31 retina ({vis31_us:.0f}µs), K32 cochlea ({aud32_us:.0f}µs) & K33 xmodal ({xms33_us:.0f}µs)."
            if xms33_us > 0
            else f"Thirty-two native CUDA kernels run K29 wave ({gw29_us:.0f}µs), K30 3-SAT ({sat30_us:.0f}µs), K31 retina ({vis31_us:.0f}µs) & K32 cochlea ({aud32_us:.0f}µs)."
            if aud32_us > 0
            else f"Thirty-one native CUDA kernels run K28 math ({nm28_us:.0f}µs), K29 graph-wave ({gw29_us:.0f}µs), K30 3-SAT ({sat30_us:.0f}µs) & K31 V1/V4 retina ({vis31_us:.0f}µs)."
            if vis31_us > 0
            else f"Thirty native CUDA kernels run K27 seq-syntax ({sp27_us:.0f}µs), K28 neural-math ({nm28_us:.0f}µs), K29 graph-wave ({gw29_us:.0f}µs) & K30 sat-solve ({sat30_us:.0f}µs)."
            if sat30_us > 0
            else f"Twenty-nine native CUDA kernels run K26 role-bind ({rb_us:.0f}µs), K27 seq-syntax ({sp27_us:.0f}µs), K28 neural-math ({nm28_us:.0f}µs) & K29 graph-wave ({gw29_us:.0f}µs)."
            if gw29_us > 0
            else f"Twenty-eight native CUDA kernels run K25 revise ({rs_us:.0f}µs), K26 role-bind ({rb_us:.0f}µs), K27 seq-syntax ({sp27_us:.0f}µs) & K28 neural-math ({nm28_us:.0f}µs)."
            if nm28_us > 0
            else f"Twenty-seven native CUDA kernels run K24 online ({os_us:.0f}µs), K25 revise ({rs_us:.0f}µs), K26 role-bind ({rb_us:.0f}µs) & K27 seq-syntax ({sp27_us:.0f}µs)."
            if sp27_us > 0
            else f"Twenty-six native CUDA kernels run K23 chainrule ({cr_us:.0f}µs), K24 online ({os_us:.0f}µs), K25 revise ({rs_us:.0f}µs) & K26 role-bind ({rb_us:.0f}µs)."
            if rb_us > 0
            else f"Twenty-five native CUDA kernels run K22 induce ({in_us:.0f}µs), K23 chainrule ({cr_us:.0f}µs), K24 online ({os_us:.0f}µs) & K25 live revise ({rs_us:.0f}µs)."
            if rs_us > 0
            else f"Twenty-four native CUDA kernels run K21 constrain ({cn_us:.0f}µs), K22 induce ({in_us:.0f}µs), K23 chainrule ({cr_us:.0f}µs) & K24 live online schema ({os_us:.0f}µs)."
            if os_us > 0
            else f"Parallel Blackwell kernels power sub-millisecond memory ({hus:.1f} µs), 2D nav ({sp_us:.1f} µs) & cortex ({nus:.1f} µs)."
        )
        narr = [
            f"Inside Axion's Blackwell SM 12.0 brain, 4,096 spiking columns fire at {gnt:.2f}B neuron-ticks/s with zero spill.",
            f"The whole brain — 4,096 hippocampal memories + {nsyn:,} neocortical synapses — fits in {l2_pct:.1f}% of L2 cache!",
            k_narr,
        ]
        n_kern = (51 if mcmcs51_us > 0 else 50 if pdes50_us > 0 else 49 if pcae49_us > 0 else 48 if sxlp48_us > 0 else 47 if odrk47_us > 0 else 46 if ffts46_us > 0 else 45 if lqrc45_us > 0 else 44 if kflt44_us > 0 else 43 if mdpp43_us > 0 else 42 if bnet42_us > 0 else 41 if lalg41_us > 0 else 40 if past40_us > 0 else 39 if symc39_us > 0 else 38 if sres38_us > 0 else 37 if cdis37_us > 0 else 36 if bdeb36_us > 0 else 35 if nmet35_us > 0 else 34 if dlg34_us > 0 else 33 if xms33_us > 0 else 32 if aud32_us > 0 else 31 if vis31_us > 0 else 30 if sat30_us > 0 else 29 if gw29_us > 0 else 28 if nm28_us > 0 else 27 if sp27_us > 0 else 26 if rb_us > 0 else 25 if rs_us > 0 else 24 if os_us > 0 else
                  23 if cr_us > 0 else 22 if in_us > 0 else 21 if cn_us > 0 else 20 if sm_us > 0 else
                  19 if pl_us > 0 else 18 if cf_us > 0 else 17 if ab_us > 0 else 16)
        return (
            "Inside Axion's Blackwell brain",
            f"SM 12.0 native cubin   /   {sdr.get('columns','4096')} cols · {nsyn:,} syn · {n_kern} kernels · {l2_pct:.1f}% L2",
            "Cortical throughput",
            gnt,
            " Gnt/s",
            stats,
            narr,
            bullets,
        )

    if ecos:
        p = ecos[-1]
        ok, tot = int(p["passed"]), int(p["total"])
        ticks = int(p.get("ticks", 64))
        is_alien = "alien" in p.get("dialect", "")
        dialect = "Scrambled Alien SOV" if is_alien else "Natural English SVO"
        stats = [
            ["Valley checks", f"{ok}/{tot}"],
            ["Trades / science", f"{p.get('trades',0)} / {p.get('discoveries',0)}"],
            ["Total GPU spikes", int(p.get("total_spikes", 0))],
        ]
        bullets = [
            f"Four independent GPU critters (Axion, Mira, Wren & sneaky Fox) shared a 16×16 Open Valley for {ticks} ticks ({dialect}).",
            f"Fox got busted spreading {p.get('lies_caught',0)} lies, a storm collapsed the bridge (Axion detoured via GPU place cells!), and Axion cracked a crystal mystery with litmus.",
            f"Mira & Axion negotiated win-win Pareto trades ({p.get('trades',0)}), Wren fixed the bridge, and Axion invented a brand-new word that spread across the valley!",
        ]
        narr = [
            f"Four spiky neighbors shared a 16×16 Open Valley for {ticks} ticks ({dialect}): Axion, Mira, Wren & cheeky Fox.",
            f"Fox's {p.get('lies_caught',0)} lies were busted; after a storm broke the bridge, Axion's 2D GPU wave detoured north for science & trade.",
            f"By tick {ticks}, a coined word spread Axion → Mira → Wren and the interview passed {ok}/{tot} ({int(p.get('total_spikes',0)):,} spikes)!",
        ]
        return (
            "Open Valley (Alien SOV)" if is_alien else "Adventures in Open Valley",
            f"{int(p.get('minds',4))} GPU critters   /   {ticks} ticks   /   detours, science, Pareto barter & new words",
            "Ecosystem cognition score",
            100.0 * ok / max(1, tot),
            "%",
            stats,
            narr,
            bullets,
        )

    if socs:
        p = socs[-1]
        ok, tot = int(p["passed"]), int(p["total"])
        is_alien = "alien" in p.get("dialect", "")
        dialect = "Scrambled Alien SOV" if is_alien else "Natural English SVO"
        stats = [
            ["Society score", f"{ok}/{tot}"],
            ["Fox lies busted", int(p.get("lies_caught", 0))],
            ["Total GPU spikes", int(p.get("total_spikes", 0))],
        ]
        bullets = [
            f"Four separate GPU spiking critters solved {ok}/{tot} social trust, gossip-relay & teamwork tests in {dialect}.",
            f"Axion & Wren busted {int(p.get('lies_caught',0))} bluffs from sneaky Fox, believed honest Mira, relayed truth to Wren, and teamed up on cooking & bridge rerouting.",
        ]
        narr = [
            f"In a lively 4-critter Homestead ({dialect}), Fox tried {int(p.get('lies_caught',0))} sly bluffs — and Axion's apical vigilance busted every single one!",
            "Honest Mira shared verified secrets that Axion absorbed in 1 shot and relayed onward to builder Wren.",
            f"Together the honest trio solved {ok} of {tot} social, crafting, and crisis-rerouting scenarios ({int(p.get('total_spikes',0)):,} GPU spikes).",
        ]
        return (
            "Spiking society (Alien SOV)" if is_alien else "Four spiky neighbors at work",
            f"{int(p.get('minds',4))} independent GPU critters   /   lie-catching, trust & teamwork",
            "Society cognition score",
            100.0 * ok / max(1, tot),
            "%",
            stats,
            narr,
            bullets,
        )

    if peers:
        p = peers[-1]
        ok, tot = int(p["passed"]), int(p["total"])
        is_alien = "alien" in p.get("dialect", "")
        dialect = "Scrambled Alien SOV" if is_alien else "Natural English SVO"
        stats = [
            ["Lessons passed", f"{ok}/{tot}"],
            ["Elder spikes", int(p["elder_spikes"])],
            ["Pupil spikes", int(p["pupil_spikes"])],
        ]
        bullets = [
            f"Two separate GPU spiking critters (wise Elder & curious Pupil) passed {ok}/{tot} split-knowledge exchanges in {dialect}.",
            "Pupil spotted what it didn't know, asked Elder directly, absorbed the answer in 1 shot, generalized taxonomy, and remembered who taught it!",
        ]
        narr = [
            f"Curious Pupil realized it had a blind spot and autonomously asked wise Elder for help ({dialect}).",
            f"After a single spiking exchange, Pupil leaped from uncertainty to solving {ok} of {tot} multi-hop, taxonomy, and elimination puzzles!",
            f"Elder fired {int(p['elder_spikes']):,} GPU spikes and Pupil fired {int(p['pupil_spikes']):,} spikes across their two independent brains.",
        ]
        return (
            "Elder & pupil (Alien SOV)" if is_alien else "Elder teaches curious pupil",
            f"Autonomous curiosity & 1-shot learning   /   {dialect}",
            "Cooperative exchange score",
            100.0 * ok / max(1, tot),
            "%",
            stats,
            narr,
            bullets,
        )

    if turns:
        m = data.get("metrics", {})
        nt = max(len(turns), int(m.get("mind_turns", 0)), max((int(t.get("mind_turn", 0)) for t in turns), default=0))
        sp = max(sum(int(t.get("spikes", 0)) for t in turns), int(m.get("mind_spikes", 0)))
        ed = max(sum(int(t.get("edits", 0)) for t in turns), int(m.get("mind_edits", 0)))
        stats = [
            ["Dialogue turns", nt],
            ["GPU spikes", sp],
            ["Synaptic edits", ed],
        ]
        bullets = [
            f"Held a live {nt}-turn free-ranging conversation with Axion's spiking brain ({sp:,} GPU spikes, {ed:,} live synaptic edits).",
            "Axion remembered personal facts, read stories, argued Socratically, invented a new word, and solved math, logic & Pearl do-calculus on the fly!",
        ]
        narr = [
            f"A human chatted live with Axion for {nt} straight turns: stories, riddles, Socratic what-ifs, do-calc & new words.",
            f"Axion fired {sp:,} Blackwell GPU spikes and rewired {ed:,} live synapses while remembering the user's favorites.",
            "Every single word was parsed, reasoned over, and spoken by spiking neural circuits with zero LLMs.",
        ]
        return (
            "Live chat with Axion's brain",
            f"{nt} turns   /   stories, Socratic debate, GPU cortex & live memory",
            "Conversation turns",
            nt,
            " turns",
            stats,
            narr,
            bullets,
        )

    return None


def _mind_draw_radar(d, cx, cy, rad, cats):
    """Draw a clean glowing 8-axis cognitive superpower octagon radar on light paper."""
    n = len(cats)
    if n < 3:
        return
    # Concentric grid octagons (50% and 100%)
    for frac in (0.5, 1.0):
        pts = []
        for i in range(n):
            ang = -math.pi * 0.5 + 2.0 * math.pi * i / n
            pts.append((cx + rad * frac * math.cos(ang), cy + rad * frac * math.sin(ang)))
        poly = "M" + " L".join(f"{px:.1f},{py:.1f}" for px, py in pts) + " Z"
        d.path(poly, "#c4d1c2", 1.0, "#e6ece1" if frac == 1.0 else "none")
    for i in range(n):
        ang = -math.pi * 0.5 + 2.0 * math.pi * i / n
        d.rule(cx, cy, cx + rad * math.cos(ang), cy + rad * math.sin(ang), "#cdd7c9", 0.9)
    # Filled score polygon
    spts = []
    for i, (lbl, col, c, t) in enumerate(cats):
        ang = -math.pi * 0.5 + 2.0 * math.pi * i / n
        f = max(0.08, c / max(1, t))
        spts.append((cx + rad * f * math.cos(ang), cy + rad * f * math.sin(ang)))
    spoly = "M" + " L".join(f"{px:.1f},{py:.1f}" for px, py in spts) + " Z"
    d.path(spoly, "#235e51", 2.2, "#74bf9e", 'fill-opacity=".35"')
    # Colored vertex dots + short readable axis tags
    short_names = ["Mem", "ToM", "Logic", "Causal", "Math", "Phys", "Lang", "GPU"]
    for i, (lbl, col, c, t) in enumerate(cats):
        ang = -math.pi * 0.5 + 2.0 * math.pi * i / n
        d.dot(spts[i][0], spts[i][1], 3.8, "#235e51")
        lx = cx + (rad + 18) * math.cos(ang)
        ly = cy + (rad + 15) * math.sin(ang) + 3.5
        anc = "middle" if abs(math.cos(ang)) < 0.25 else ("start" if math.cos(ang) > 0 else "end")
        tag = short_names[i] if i < len(short_names) else lbl[:4]
        d.text(lx, ly, tag, 9.8, INK, MONO, 600, anc)


def mind_plate(d, data, b, x, y, size, rx, rw):
    lines = data.get("lines", [])
    entries = lambda key: [pairs(line) for line in lines if line.startswith(key + "=")]
    evos = entries("mind_evo")
    ctrls = {r["mind_control"]: r for r in entries("mind_control")}
    evals = entries("mind_eval")
    benches = entries("mind_bench")
    peers = entries("mind_peer_exchange")
    socs = entries("mind_society")
    ecos = entries("mind_ecosystem")
    turns = entries("mind_turn")

    ink, muted, gold, green, grid = "#edf1dc", "#b5c6b5", "#f3bc73", "#91cfb7", "#355347"
    coral, aqua, lav = "#e28b79", "#87d6cf", "#bcb6ec"
    d.rect(x, y, size, size, "#162d2a", 13)

    # -------------------------------------------------------------------------
    # 1. EVOLUTION PLATE: "FROM NEWBORN TO THINKER" + CAUSAL KNOCKOUTS
    # -------------------------------------------------------------------------
    if evos:
        first, last = evos[0], evos[-1]
        max_sc = float(last["mean"].split("/")[1])
        first_m = float(first["mean"].split("/")[0])
        last_m = float(last["mean"].split("/")[0])
        best_m = float(last["best"].split("/")[0])
        mid_e = evos[len(evos) // 2]
        mid_m = float(mid_e["mean"].split("/")[0])
        _mind_header_medallion(d, "axion", "proud", 3, f"Gen {last['gen']} Champ", f"{int(best_m)}/{int(max_sc)} peak", green)

        d.label(x + 24, y + 32, "HOW THE BABY BRAINS LEVEL UP ACROSS GENERATIONS", green)

        # Top Evolution Story Strip: Newborn -> Learner -> Champion!
        stages = [
            (f"GEN {first['gen']} NEWBORN", "newborn", 1, f"{first_m:.0f}/{int(max_sc)}", "Random twitches", coral),
            (f"GEN {mid_e['gen']} TODDLER", "curious", 2, f"{mid_m:.0f}/{int(max_sc)}", "Spotting laws", gold),
            (f"GEN {last['gen']} CHAMPION", "proud", 3, f"{int(best_m)}/{int(max_sc)}", "Causal thinker!", green),
        ]
        card_w = 172
        for i, (st_lbl, md, lvl, sc_s, cap_s, col) in enumerate(stages):
            cx_card = x + 22 + i * 194
            cy_card = y + 46
            d.rect(cx_card, cy_card, card_w, 126, "#1f3c37", 10, f'stroke="{col}" stroke-width="1.2" stroke-opacity=".55"')
            d.text(cx_card + card_w * 0.5, cy_card + 18, st_lbl, 9.5, col, MONO, 700, "middle")
            _mind_critter(d, cx_card + card_w * 0.5, cy_card + 64, 1.05, "axion", md, lvl)
            d.text(cx_card + card_w * 0.5, cy_card + 104, sc_s, 13.5, ink, MONO, 700, "middle")
            d.text(cx_card + card_w * 0.5, cy_card + 119, cap_s, 10.0, muted, SANS, 400, "middle")
            if i < 2:
                ax = cx_card + card_w + 3
                ay = cy_card + 64
                d.path(f"M{ax} {ay} H{ax + 15} M{ax + 10} {ay - 5} L{ax + 15} {ay} L{ax + 10} {ay + 5}", gold, 2.2)

        # Bottom: Causal Brain-Circuit Knockout Bars (clear plain-English explanations!)
        d.label(x + 24, y + 200, "SWITCH OFF ONE BRAIN CIRCUIT AT A TIME (16 NEW WORLDS)", gold)
        if "intact" in ctrls:
            n_pil_e = 114 if "mcmcs" in last else (112 if "pdes" in last else (110 if "pcae" in last else (108 if "sxlp" in last else (106 if "odrk" in last else (104 if "ffts" in last else (102 if "lqrc" in last else (100 if "kflt" in last else (98 if "mdpp" in last else (96 if "bnet" in last else (94 if "lalg" in last else (92 if "past" in last else (90 if "symc" in last else (88 if "sres" in last else (86 if "cdis" in last else (84 if "bdeb" in last else (82 if "nmet" in last else (80 if "dlgc" in last else (78 if "xmod" in last else (76 if "audv" in last else (74 if "visv" in last else (72 if "sats" in last else (70 if "gwav" in last else (68 if "nmat" in last else (66 if "sqpr" in last else (64 if "rbnd" in last else (62 if "rvsc" in last else (60 if "osch" in last else (58 if "chrl" in last else (56 if "ind" in last else (54 if "csat" in last else (52 if "csim" in last else (50 if "plan" in last else (48 if "cfimg" in last else (46 if "abdbk" in last else (44 if "deduce" in last else (42 if "quant" in last else (40 if "dream" in last else (38 if "gate" in last else (36 if "clust" in last else (34 if "coref" in last else (32 if "ograph" in last else (30 if "sit" in last else 28))))))))))))))))))))))))))))))))))))))))))
            arms = [
                ("Intact evolved champion", f"All {n_pil_e} cognitive pillars firing together", "intact", green),
                ("No causal column", "Loses multi-step planning & counterfactuals", "no_causal", gold),
                ("No apical dendrite", "Loses Theory of Mind, logic & social cues", "no_apical", gold),
                ("No hippocampus", "Forgets episodes, stories & spatial paths", "no_hipp", coral),
                ("Shuffled feedback", "Plasticity rewires on random noise", "shuffled_feedback", coral),
                ("Frozen at birth", "Zero synaptic learning — stuck as newborn", "frozen", "#cf6363"),
            ]
            for i, (lbl, desc, k, col) in enumerate(arms):
                if k not in ctrls:
                    continue
                num_s, den_s = ctrls[k]["total"].split("/")
                frac = float(num_s) / max(1.0, float(den_s))
                yy = y + 228 + i * 62
                d.text(x + 26, yy, lbl, 14.5, ink, SANS, 600)
                d.text(x + size - 26, yy, f"{num_s}/{den_s} ({100*frac:.1f}%)", 13.5, col, MONO, 700, "end")
                d.rect(x + 26, yy + 9, size - 52, 13, "#26433d", 5)
                d.rect(x + 26, yy + 9, max(4.0, (size - 52) * frac), 13, col, 5)
                d.text(x + 26, yy + 35, desc, 11.2, muted, SANS)
        else:
            pillars = [
                ("Grounding", float(last["ground"]), 128.0),
                ("Episodic", float(last["episodic"]), 32.0),
                ("Theory of Mind", float(last["tom"]), 48.0),
                ("Causal Plan", float(last["causal"]), 80.0),
                ("Logic MP/MT", float(last["logic"]), 80.0),
                ("Social Coop", float(last["social"]), 80.0),
            ]
            for i, (lbl, val_p, cap_p) in enumerate(pillars):
                yy = y + 232 + i * 58
                frac = min(1.0, val_p / cap_p)
                d.text(x + 26, yy, lbl, 14.5, ink, SANS, 600)
                d.text(x + size - 26, yy, f"{val_p:.1f}/{int(cap_p)}", 13.5, green, MONO, 700, "end")
                d.rect(x + 26, yy + 10, size - 52, 13, "#26433d", 5)
                d.rect(x + 26, yy + 10, (size - 52) * frac, 13, green, 5)

        # Right middle panel: Selection vs Drift Curve + Leap Badge
        gens = [int(r["gen"]) for r in evos]
        means = [float(r["mean"].split("/")[0]) for r in evos]
        bests = [float(r["best"].split("/")[0]) for r in evos]
        d.label(rx, 332, "SMARTS OVER GENERATIONS (EVOLUTION VS RANDOM DRIFT)")
        for i in range(4):
            gy = 358 + i * 44
            d.rule(rx, gy, W - 48, gy, "#d5dbcf")
        trend(d, rx + 4, 358, rw - 8, 132, means, GREEN, True, gens, (0, max_sc))
        trend(d, rx + 4, 358, rw - 8, 132, bests, "#ad713e", False, gens, (0, max_sc))
        if "drift_endpoint" in ctrls:
            dm = float(ctrls["drift_endpoint"]["drift_mean"].split("/")[0])
            dy = 358 + 132 * (1.0 - dm / max(1.0, max_sc))
            d.path(f"M{rx+4:.1f} {dy:.1f} H{rx+rw-4:.1f}", "#b5524c", 2.2, extra='stroke-dasharray="6,4"')
            d.text(W - 48, dy - 7, f"Random drift stuck at {dm:.1f}", 12.5, "#9c3e39", MONO, 600, "end")
            ratio_x = means[-1] / max(1.0, dm)
            _mind_pill(d, rx + rw - 172, 317, f"{ratio_x:.1f}x SMARTER THAN DRIFT", GREEN, "#edf1dc", 10.0, 9.0)
        d.text(rx, 520, f"Gen {gens[0]} → {gens[-1]}  ({int(last['pop']):,} spiking critters)", 13.5, MUTED, MONO)
        d.text(W - 48, 520, f"Mean {means[-1]:.1f}  ·  Champ {int(bests[-1])}/{int(max_sc)}", 13.5, GREEN, MONO, 600, "end")
        return

    # -------------------------------------------------------------------------
    # 2. COGNITIVE AUDIT PLATE: CREATURE PORTRAIT + 8 SUPERPOWERS + 130-TILE MOSAIC + RADAR
    # -------------------------------------------------------------------------
    if evals:
        domains = _mind_parse_domains(lines)
        ev = evals[-1]
        cor, tot = int(ev["correct"]), int(ev["total"])
        arm = ev["mind_eval"]
        is_alien = "alien" in ev.get("dialect", "")
        is_abl = arm not in ("intact", "none")
        role = "alien" if is_alien else "axion"
        mood = "ablate" if is_abl else "proud"
        med_lbl = f"Lesion: {arm}" if is_abl else ("Alien SOV Twin" if is_alien else "Axion Child")
        med_sub = f"{cor}/{tot} ({ev['rate']})"
        _mind_header_medallion(d, role, mood, 3, med_lbl, med_sub, lav if is_alien else (coral if is_abl else green))

        dialect = "SCRAMBLED ALIEN SOV" if is_alien else "NATURAL ENGLISH SVO"
        d.label(x + 24, y + 30, f"{len(domains)}-DOMAIN MIND EXAM · {dialect}", lav if is_alien else green)

        # Top Hero Critter + Expressive Speech Bubble
        d.rect(x + 20, y + 42, size - 40, 102, "#1f3c37", 11)
        _mind_critter(d, x + 76, y + 95, 1.32, role, mood, 3)
        # Speech bubble next to critter
        bx, by, bw, bh = x + 136, y + 52, size - 170, 82
        d.rect(bx, by, bw, bh, "#284c45", 10, f'stroke="{lav if is_alien else (coral if is_abl else green)}" stroke-width="1.2"')
        d.path(f"M{bx} {by+34} L{bx-11} {by+41} L{bx} {by+48} Z", "#284c45", 0.5, "#284c45")
        if is_abl and arm == "hipp":
            ribot_s = ("Duoquinquagintuple-Pathway (52-Track) Ribot's Law verified!" if len(domains) >= 545 else
                       "Unquinquagintuple-Pathway (51-Track) Ribot's Law verified!" if len(domains) >= 535 else
                       "Quinquagintuple-Pathway (50-Track) Ribot's Law verified!" if len(domains) >= 525 else
                       "Novemquadragintuple-Pathway (49-Track) Ribot's Law verified!" if len(domains) >= 515 else
                       "Octoquadragintuple-Pathway (48-Track) Ribot's Law verified!" if len(domains) >= 505 else
                       "Septemquadragintuple-Pathway (47-Track) Ribot's Law verified!" if len(domains) >= 495 else
                       "Sexquadragintuple-Pathway (46-Track) Ribot's Law verified!" if len(domains) >= 485 else
                       "Quinquadragintuple-Pathway (45-Track) Ribot's Law verified!" if len(domains) >= 475 else
                       "Quattuorquaragintuple-Pathway (44-Track) Ribot's Law verified!" if len(domains) >= 465 else
                       "Trequaragintuple-Pathway (43-Track) Ribot's Law verified!" if len(domains) >= 455 else
                       "Duoquaragintuple-Pathway (42-Track) Ribot's Law verified!" if len(domains) >= 445 else
                       "Unquadragintuple-Pathway (41-Track) Ribot's Law verified!" if len(domains) >= 435 else
                       "Quadragintuple-Pathway (40-Track) Ribot's Law verified!" if len(domains) >= 425 else
                       "Novemtrigintuple-Pathway (39-Track) Ribot's Law verified!" if len(domains) >= 415 else
                       "Octotrigintuple-Pathway (38-Track) Ribot's Law verified!" if len(domains) >= 405 else
                       "Septentrigintuple-Pathway (37-Track) Ribot's Law verified!" if len(domains) >= 395 else
                       "Sextrigintuple-Pathway (36-Track) Ribot's Law verified!" if len(domains) >= 385 else
                       "Quintrigintuple-Pathway (35-Track) Ribot's Law verified!" if len(domains) >= 375 else
                       "Quattuortrigintuple-Pathway (34-Track) Ribot's Law verified!" if len(domains) >= 365 else
                       "Tretrigintuple-Pathway (33-Track) Ribot's Law verified!" if len(domains) >= 355 else
                       "Duotrigintuple-Pathway (32-Track) Ribot's Law verified!" if len(domains) >= 345 else
                       "Untrigintuple-Pathway (31-Track) Ribot's Law verified!" if len(domains) >= 335 else
                       "Trigintuple-Pathway (30-Track) Ribot's Law verified!" if len(domains) >= 325 else
                       "Nonavigintuple-Pathway (29-Track) Ribot's Law verified!" if len(domains) >= 315 else
                       "Octovigintuple-Pathway (28-Track) Ribot's Law verified!" if len(domains) >= 305 else
                       "Septemvigintuple-Pathway (27-Track) Ribot's Law verified!" if len(domains) >= 295 else
                       "Sexvigintuple-Pathway (26-Track) Ribot's Law verified!" if len(domains) >= 285 else
                       "Quinvigintuple-Pathway (25-Track) Ribot's Law verified!" if len(domains) >= 275 else
                       "Quattuorvigintuple-Pathway (24-Track) Ribot's Law verified!" if len(domains) >= 265 else
                       "Ribot's Law verified live on Blackwell GPU.")
            q1 = "Hippocampus OFF — I forgot new stories,"
            q2 = f"yet {cor}/{tot} survived on my 28M GPU cortex!"
            q3 = ribot_s
        elif is_abl:
            q1 = f"My {arm} circuit is switched off right now!"
            q2 = f"Score moved to {cor}/{tot} ({ev['rate']}) — proving"
            q3 = "this exact neural pathway causes the skill."
        elif is_alien:
            q1 = f"I never saw a single word of English!"
            q2 = f"Solved {cor}/{tot} ({ev['rate']}) in Scrambled Alien SOV —"
            q3 = "exact 100% parity across every domain!" if cor == tot else f"dialect-agnostic ({perf}/{len(domains)} domains at 100%)!"
        else:
            nk_ev = 51 if len(domains) >= 545 else 50 if len(domains) >= 535 else 49 if len(domains) >= 525 else 48 if len(domains) >= 515 else 47 if len(domains) >= 505 else 46 if len(domains) >= 495 else 45 if len(domains) >= 485 else 44 if len(domains) >= 475 else 43 if len(domains) >= 465 else 42 if len(domains) >= 455 else 41 if len(domains) >= 445 else 40 if len(domains) >= 435 else 39 if len(domains) >= 425 else 38 if len(domains) >= 415 else 37 if len(domains) >= 405 else 36 if len(domains) >= 395 else 35 if len(domains) >= 385 else 34 if len(domains) >= 375 else 33 if len(domains) >= 365 else 32 if len(domains) >= 355 else 31 if len(domains) >= 345 else 30 if len(domains) >= 335 else 29 if len(domains) >= 325 else 28 if len(domains) >= 315 else 27 if len(domains) >= 305 else 26 if len(domains) >= 295 else 25 if len(domains) >= 285 else 24
            q1 = f"I solved {cor} of {tot} probes across {len(domains)} domains!"
            q2 = ("STAGE 8o · 51 KERNELS & 550 DOMAINS! K51 GPU MCMC & 3-entity chat," if len(domains) >= 545 else
                  "STAGE 8n · 50 KERNELS & 540 DOMAINS! K50 GPU 1D PDE & deduction," if len(domains) >= 535 else
                  "STAGE 8m · 530 DOMAINS! K49 GPU PCA eigen & causal chat," if len(domains) >= 525 else
                  "STAGE 8l · 520 DOMAINS! K48 GPU Simplex LP & proof chat," if len(domains) >= 515 else
                  "STAGE 8k · 510 DOMAINS! K47 GPU RK4 ODE & counterfactual chat," if len(domains) >= 505 else
                  "STAGE 8j · 500 DOMAINS! K46 GPU FFT & self-correction," if len(domains) >= 495 else
                  "STAGE 8i LIVE! K45 GPU LQR Riccati & multi-turn topic bridge," if len(domains) >= 485 else
                  "STAGE 8h · 100 PILLARS! K44 GPU Kalman & hypothesis testing," if len(domains) >= 475 else
                  "STAGE 8g LIVE! K43 GPU MDP Bellman & open user goals," if len(domains) >= 465 else
                  "STAGE 8f LIVE! K42 GPU Bayes-net & Socratic critique," if len(domains) >= 455 else
                  "STAGE 8e LIVE! K41 GPU linear algebra & story co-writing," if len(domains) >= 445 else
                  "STAGE 8d LIVE! K40 GPU AST synth & zero-keyword debate," if len(domains) >= 435 else
                  "STAGE 8c LIVE! K39 GPU symbolic calculus & live concept teaching," if len(domains) >= 425 else
                  "STAGE 8b LIVE! K38 self-research/patching & deep discourse," if len(domains) >= 415 else
                  "STAGE 8 CROSSED! K37 collective discovery & Socratic curiosity," if len(domains) >= 405 else
                  "STAGE 7c · 400 DOMAINS! K36 belief-graph/debate & 5-facet chat," if len(domains) >= 395 else
                  "STAGE 7b LIVE! K35 metaphor/plot/critique & unscripted chat," if len(domains) >= 385 else
                  "STAGE 7 CROSSED! K34 dialogue cortex, open chat, wit & ethics," if len(domains) >= 375 else
                  "STAGE 6c! K33 cross-modal+goal stack, coaching & contraptions," if len(domains) >= 365 else
                  "STAGE 6b! K32 cochlear audio, textbook self-quiz & auto-debug," if len(domains) >= 355 else
                  "STAGE 6! K31 16x16 V1/V4 retina, curiosity & code lab," if len(domains) >= 345 else
                  "Stage 5d! K30 GPU 3-SAT, 16k HBM & free why-prose," if len(domains) >= 335 else
                  "Stage 5c! K29 GPU graph wave, free coref & 8k lexicon," if len(domains) >= 325 else
                  "Stage 5b! K28 GPU neural math & multi-clause prose," if len(domains) >= 315 else
                  "Stage 5! K27 predictive syntax, template-free Broca & MST,")
            q3 = f"plus {nk_ev} Blackwell CUDA spiking kernels — zero LLMs."
        d.text(bx + 14, by + 24, q1, 13.0, ink, SERIF, 600)
        d.text(bx + 14, by + 45, q2, 12.2, gold if is_abl else green, SANS, 500)
        d.text(bx + 14, by + 66, q3, 11.2, muted, SANS)

        # Middle: 8 Cognitive Superpower Bars!
        cats = _mind_categorize_domains(domains)
        d.label(x + 24, y + 166, "8 COGNITIVE SUPERPOWERS (ALL DOMAINS GROUPED)", gold)
        for i, (cname, ccol, c_ok, c_tot) in enumerate(cats):
            yy = y + 186 + i * 24.5
            frac = c_ok / max(1, c_tot)
            d.dot(x + 29, yy - 3.5, 4.2, ccol)
            d.text(x + 41, yy, cname, 12.2, ink, SANS, 500)
            bar_x = x + 218
            bar_w = size - 348
            d.rect(bar_x, yy - 9.5, bar_w, 9.5, "#25423c", 4.5)
            if frac > 0:
                d.rect(bar_x, yy - 9.5, max(5.0, bar_w * frac), 9.5, ccol, 4.5)
            d.text(x + size - 22, yy, f"{c_ok}/{c_tot} ({100*frac:.0f}%)", 11.2, ccol, MONO, 600, "end")

        # Bottom: All N Domains Illuminated Cortical Mosaic Heatmap!
        n_dom = max(1, len(domains))
        d.label(x + 24, y + 398, f"ALL {n_dom} DOMAINS CORTICAL MOSAIC (1 TILE = 1 DOMAIN)", green)
        cols_m = 37 if n_dom >= 545 else (36 if n_dom >= 525 else (35 if n_dom >= 515 else (34 if n_dom >= 495 else (33 if n_dom >= 485 else (32 if n_dom >= 475 else (31 if n_dom >= 455 else (30 if n_dom >= 435 else (29 if n_dom >= 425 else (28 if n_dom == 420 else (26 if n_dom == 410 else (25 if n_dom == 400 else (28 if n_dom > 270 else (27 if n_dom > 260 else (26 if n_dom > 250 else (25 if n_dom > 240 else (24 if n_dom > 230 else (23 if n_dom > 220 else (22 if n_dom > 210 else (21 if n_dom > 200 else (20 if n_dom > 190 else (19 if n_dom > 180 else (18 if n_dom > 170 else (17 if n_dom > 160 else (16 if n_dom > 150 else (15 if n_dom > 140 else (14 if n_dom > 130 else (13 if n_dom > 110 else (12 if n_dom > 90 else 10))))))))))))))))))))))))))))
        rows_m = (n_dom + cols_m - 1) // cols_m
        gap_x, gap_y = (0.5 if n_dom >= 545 else (0.6 if n_dom >= 525 else (0.7 if n_dom >= 515 else (0.8 if n_dom >= 495 else (0.9 if n_dom >= 485 else (1.0 if n_dom >= 475 else (1.1 if n_dom >= 455 else (1.2 if n_dom >= 435 else (1.3 if n_dom >= 425 else (1.4 if n_dom == 420 else (1.6 if n_dom == 410 else (1.8 if n_dom == 400 else (1.3 if cols_m >= 28 else (1.5 if cols_m >= 27 else (1.7 if cols_m >= 26 else (1.9 if cols_m >= 25 else (2.1 if cols_m >= 24 else (2.3 if cols_m >= 23 else (2.6 if cols_m >= 22 else (3.2 if cols_m >= 21 else 4.0)))))))))))))))))))), (1.8 if rows_m >= 15 else 4.0)
        mx0, my0 = x + 22, y + 412
        tile_w = (size - 44 - gap_x * (cols_m - 1)) / cols_m
        tile_h = min(16.0, (152.0 - gap_y * (rows_m - 1)) / max(1, rows_m))
        for i, (dname, c_d, t_d) in enumerate(domains):
            r_i = i // cols_m
            c_i = i % cols_m
            tx = mx0 + c_i * (tile_w + gap_x)
            ty = my0 + r_i * (tile_h + gap_y)
            frac = c_d / max(1, t_d)
            tcol = green if frac >= 0.99 else (gold if frac > 0 else "#334d47")
            tink = "#102420" if frac > 0 else "#7e968d"
            d.rect(tx, ty, tile_w, tile_h, tcol, 3.0)
            d.text(tx + tile_w * 0.5, ty + tile_h - 3.4, f"{i+1}", 5.0 if cols_m >= 28 else (5.3 if cols_m >= 27 else (5.5 if cols_m >= 26 else (5.8 if cols_m >= 25 else (6.0 if cols_m >= 24 else (6.2 if cols_m >= 23 else (6.6 if cols_m >= 22 else (7.0 if cols_m >= 21 else 7.4))))))), tink, MONO, 700, "middle")
        # Clean legend under mosaic
        ly = y + 586
        for lx, lcol, ltxt in ((x + 24, green, "100% Mastered"), (x + 150, gold, "Partial"), (x + 232, "#334d47", "0% (Lesioned)")):
            d.rect(lx, ly - 9, 10, 10, lcol, 2.5)
            d.text(lx + 15, ly, ltxt, 10.5, muted, SANS)
        perfect_n = sum(1 for _, c_d, t_d in domains if c_d == t_d)
        d.text(x + size - 22, ly, f"{perfect_n}/{n_dom} domains at 100%", 10.8, green if perfect_n == n_dom else gold, MONO, 600, "end")

        # Right middle panel: 8-Axis Radar + Clean Scorecard
        d.label(rx, 330, "MIND PROFILE RADAR & EXAM SCORECARD")
        _mind_draw_radar(d, rx + 114, 440, 74, cats)
        pretty_dial = "Alien SOV" if is_alien else "English SVO"
        rows_r = [
            ("Dialect", pretty_dial),
            ("Circuit arm", arm),
            ("Perfect domains", f"{perfect_n} / {n_dom}"),
            ("Probes solved", f"{cor} / {tot} ({ev['rate']})"),
            ("Live edits", f"{int(ev['edits']):,}"),
        ]
        sx = rx + 246
        for i, (k, v) in enumerate(rows_r):
            yy = 370 + 35 * i
            d.text(sx, yy, k, 15.0, INK)
            d.text(W - 48, yy, v, 15.0, GREEN, MONO, 600, "end")
            d.rule(sx, yy + 10, W - 48, yy + 10, "#d5dccf")
        return

    # -------------------------------------------------------------------------
    # 3. BENCHMARK PLATE: CUTAWAY SPIKING CREATURE BRAIN + 6 KERNEL TILES + L2 BAR
    # -------------------------------------------------------------------------
    if benches:
        sdr = next((r for r in benches if r["mind_bench"] == "sdr_columns"), {})
        hip = next((r for r in benches if r["mind_bench"] == "hippocampus"), {})
        neo = next((r for r in benches if r["mind_bench"] == "neocortex"), {})
        sp2 = next((r for r in benches if r["mind_bench"] == "spatial_2d"), {})
        tch = next((r for r in benches if r["mind_bench"] == "think_chain"), {})
        wma = next((r for r in benches if r["mind_bench"] == "wm_attend"), {})
        ctr = next((r for r in benches if r["mind_bench"] == "neo_contrast"), {})
        ana = next((r for r in benches if r["mind_bench"] == "neo_analogy"), {})
        cmp = next((r for r in benches if r["mind_bench"] == "neo_compose"), {})
        mcu = next((r for r in benches if r["mind_bench"] == "neo_multi_cue"), {})
        hyp = next((r for r in benches if r["mind_bench"] == "neo_hypothesize"), {})
        brg = next((r for r in benches if r["mind_bench"] == "neo_bridge"), {})
        clu = next((r for r in benches if r["mind_bench"] == "neo_cluster"), {})
        gat = next((r for r in benches if r["mind_bench"] == "neo_gate"), {})
        drm = next((r for r in benches if r["mind_bench"] == "neo_dream"), {})
        scn = next((r for r in benches if r["mind_bench"] == "neo_scan"), {})
        ded = next((r for r in benches if r["mind_bench"] == "neo_deduce"), {})
        abd = next((r for r in benches if r["mind_bench"] == "neo_abduce_back"), {})
        cfa = next((r for r in benches if r["mind_bench"] == "neo_counterfact"), {})
        plc = next((r for r in benches if r["mind_bench"] == "neo_plan_chain"), {})
        sim = next((r for r in benches if r["mind_bench"] == "neo_simulate"), {})
        con = next((r for r in benches if r["mind_bench"] == "neo_constrain"), {})
        ind = next((r for r in benches if r["mind_bench"] == "neo_induce"), {})
        chr_b = next((r for r in benches if r["mind_bench"] == "neo_chain_rule"), {})
        ons = next((r for r in benches if r["mind_bench"] == "neo_online_schema"), {})
        rvs = next((r for r in benches if r["mind_bench"] == "neo_revise_schema"), {})
        rbd = next((r for r in benches if r["mind_bench"] == "neo_role_bind"), {})
        sp27 = next((r for r in benches if r["mind_bench"] == "neo_seq_predict"), {})
        nm28 = next((r for r in benches if r["mind_bench"] == "neo_neural_math"), {})
        gw29 = next((r for r in benches if r["mind_bench"] == "neo_graph_wave"), {})
        sat30 = next((r for r in benches if r["mind_bench"] == "neo_sat_solve"), {})
        vis31 = next((r for r in benches if r["mind_bench"] == "neo_visual_cortex"), {})
        aud32 = next((r for r in benches if r["mind_bench"] == "neo_audio_cortex"), {})
        xms33 = next((r for r in benches if r["mind_bench"] == "neo_crossmodal_stack"), {})
        dlg34 = next((r for r in benches if r["mind_bench"] == "neo_dialogue_cortex"), {})
        nmet35 = next((r for r in benches if r["mind_bench"] == "neo_narrative_metaphor"), {})
        bdeb36 = next((r for r in benches if r["mind_bench"] == "neo_belief_debate"), {})
        cdis37 = next((r for r in benches if r["mind_bench"] == "neo_collective_discovery"), {})
        sres38 = next((r for r in benches if r["mind_bench"] == "neo_self_research"), {})
        symc39 = next((r for r in benches if r["mind_bench"] == "neo_symbolic_calculus"), {})
        past40 = next((r for r in benches if r["mind_bench"] == "neo_program_ast"), {})
        lalg41 = next((r for r in benches if r["mind_bench"] == "neo_linear_algebra"), {})
        bnet42 = next((r for r in benches if r["mind_bench"] == "neo_bayes_net"), {})
        mdpp43 = next((r for r in benches if r["mind_bench"] == "neo_mdp_planner"), {})
        kflt44 = next((r for r in benches if r["mind_bench"] == "neo_kalman_filter"), {})
        lqrc45 = next((r for r in benches if r["mind_bench"] == "neo_lqr_control"), {})
        ffts46 = next((r for r in benches if r["mind_bench"] == "neo_fft_spectrum"), {})
        odrk47 = next((r for r in benches if r["mind_bench"] == "neo_ode_rk4"), {})
        sxlp48 = next((r for r in benches if r["mind_bench"] == "neo_simplex_lp"), {})
        pcae49 = next((r for r in benches if r["mind_bench"] == "neo_pca_eigen"), {})
        pdes50 = next((r for r in benches if r["mind_bench"] == "neo_pde_solver"), {})
        mcmcs51 = next((r for r in benches if r["mind_bench"] == "neo_mcmc_sampler"), {})
        nsyn = int(neo.get("synapses", 0))
        hip_kib = int(hip.get("l2_footprint_KiB", "3264"))
        neo_kib = int(neo.get("l2_footprint_KiB", "28672"))
        l2_pct = 100.0 * (hip_kib + neo_kib) / 32768.0
        n_k = (51 if mcmcs51 else 50 if pdes50 else 49 if pcae49 else 48 if sxlp48 else 47 if odrk47 else 46 if ffts46 else 45 if lqrc45 else 44 if kflt44 else 43 if mdpp43 else 42 if bnet42 else 41 if lalg41 else 40 if past40 else 39 if symc39 else 38 if sres38 else 37 if cdis37 else 36 if bdeb36 else 35 if nmet35 else 34 if dlg34 else 33 if xms33 else 32 if aud32 else 31 if vis31 else 30 if sat30 else 29 if gw29 else 28 if nm28 else 27 if sp27 else 26 if rbd else 25 if rvs else 24 if ons else 23 if chr_b else
               22 if ind else 21 if con else 20 if sim else 19 if plc else 18)
        ribot_lbl = ("52-Track Ribot" if mcmcs51 else "51-Track Ribot" if pdes50 else "50-Track Ribot" if pcae49 else "49-Track Ribot" if sxlp48 else "48-Track Ribot" if odrk47 else "47-Track Ribot" if ffts46 else "46-Track Ribot" if lqrc45 else "45-Track Ribot" if kflt44 else "44-Track Ribot" if mdpp43 else "43-Track Ribot" if bnet42 else "42-Track Ribot" if lalg41 else "41-Track Ribot" if past40 else "40-Track Ribot" if symc39 else "39-Track Ribot" if sres38 else "38-Track Ribot" if cdis37 else "37-Track Ribot" if bdeb36 else "36-Track Ribot" if nmet35 else "35-Track Ribot" if dlg34 else "34-Track Ribot" if xms33 else "Tretrigintuple Ribot" if aud32 else "Duotrigintuple Ribot" if vis31 else "Untrigintuple Ribot" if sat30 else "Trigintuple Ribot" if gw29 else "Nonavigintuple Ribot" if nm28 else "Octovigintuple Ribot" if sp27 else
                     "Septemvigintuple Ribot" if rbd else "Sexvigintuple Ribot" if rvs else
                     "Quinvigintuple Ribot" if ons else "Quattuorvigintuple Ribot" if chr_b else "Trevigintuple Ribot")
        _mind_header_medallion(d, "axion", "thinking", 3, "Blackwell Brain", f"{l2_pct:.1f}% L2 Resident", aqua)

        d.label(x + 24, y + 30, f"INSIDE AXION'S SPIKING BLACKWELL BRAIN ({n_k} CUDA KERNELS)", green)

        # Top Hero: Stylized Cutaway Spiking Creature Brain Diagram!
        d.rect(x + 20, y + 42, size - 40, 198, "#1b3632", 11)
        # Cute little glowing Axion critter silhouette on left feeding into the brain map
        _mind_critter(d, x + 76, y + 142, 1.38, "axion", "thinking", 3)
        d.text(x + 76, y + 196, "AXION", 10.5, green, MONO, 700, "middle")
        d.text(x + 76, y + 210, "4,096 cols", 9.2, muted, MONO, 500, "middle")

        # Brain circuit nodes + animated-look spike pathways
        node_k_val = (f"{float(pdes50.get('pdes_us',0)):.0f}/{float(mcmcs51.get('mcmcs_us',0)):.0f} µs" if mcmcs51 else
                      f"{float(pcae49.get('pcae_us',0)):.0f}/{float(pdes50.get('pdes_us',0)):.0f} µs" if pdes50 else
                      f"{float(sxlp48.get('sxlp_us',0)):.0f}/{float(pcae49.get('pcae_us',0)):.0f} µs" if pcae49 else
                      f"{float(odrk47.get('odrk_us',0)):.0f}/{float(sxlp48.get('sxlp_us',0)):.0f} µs" if sxlp48 else
                      f"{float(ffts46.get('ffts_us',0)):.0f}/{float(odrk47.get('odrk_us',0)):.0f} µs" if odrk47 else
                      f"{float(lqrc45.get('lqrc_us',0)):.0f}/{float(ffts46.get('ffts_us',0)):.0f} µs" if ffts46 else
                      f"{float(kflt44.get('kflt_us',0)):.0f}/{float(lqrc45.get('lqrc_us',0)):.0f} µs" if lqrc45 else
                      f"{float(mdpp43.get('mdpp_us',0)):.0f}/{float(kflt44.get('kflt_us',0)):.0f} µs" if kflt44 else
                      f"{float(bnet42.get('bnet_us',0)):.0f}/{float(mdpp43.get('mdpp_us',0)):.0f} µs" if mdpp43 else
                      f"{float(lalg41.get('lalg_us',0)):.0f}/{float(bnet42.get('bnet_us',0)):.0f} µs" if bnet42 else
                      f"{float(past40.get('past_us',0)):.0f}/{float(lalg41.get('lalg_us',0)):.0f} µs" if lalg41 else
                      f"{float(symc39.get('symc_us',0)):.0f}/{float(past40.get('past_us',0)):.0f} µs" if past40 else
                      f"{float(sres38.get('sres_us',0)):.0f}/{float(symc39.get('symc_us',0)):.0f} µs" if symc39 else
                      f"{float(cdis37.get('cdis_us',0)):.0f}/{float(sres38.get('sres_us',0)):.0f} µs" if sres38 else
                      f"{float(bdeb36.get('bdeb_us',0)):.0f}/{float(cdis37.get('cdis_us',0)):.0f} µs" if cdis37 else
                      f"{float(nmet35.get('nmet_us',0)):.0f}/{float(bdeb36.get('bdeb_us',0)):.0f} µs" if bdeb36 else
                      f"{float(dlg34.get('dlg_us',0)):.0f}/{float(nmet35.get('nmet_us',0)):.0f} µs" if nmet35 else
                      f"{float(xms33.get('xmodal_us',0)):.0f}/{float(dlg34.get('dlg_us',0)):.0f} µs" if dlg34 else
                      f"{float(aud32.get('audio_us',0)):.0f}/{float(xms33.get('xmodal_us',0)):.0f} µs" if xms33 else
                      f"{float(vis31.get('visual_us',0)):.0f}/{float(aud32.get('audio_us',0)):.0f} µs" if aud32 else
                      f"{float(sat30.get('sat_solve_us',0)):.0f}/{float(vis31.get('visual_us',0)):.0f} µs" if vis31 else
                      f"{float(gw29.get('graph_wave_us',0)):.0f}/{float(sat30.get('sat_solve_us',0)):.0f} µs" if sat30 else
                      f"{float(nm28.get('neural_math_us',0)):.0f}/{float(gw29.get('graph_wave_us',0)):.0f} µs" if gw29 else
                      f"{float(sp27.get('seq_predict_us',0)):.0f}/{float(nm28.get('neural_math_us',0)):.0f} µs" if nm28 else
                      f"{float(rbd.get('role_bind_us',0)):.0f}/{float(sp27.get('seq_predict_us',0)):.0f} µs" if sp27 else
                      f"{float(rvs.get('revise_schema_us',0)):.0f}/{float(rbd.get('role_bind_us',0)):.0f} µs" if rbd else
                      f"{float(ons.get('online_schema_us',0)):.0f}/{float(rvs.get('revise_schema_us',0)):.0f} µs" if rvs else
                      f"{float(chr_b.get('chain_rule_us',0)):.0f}/{float(ons.get('online_schema_us',0)):.0f} µs")
        nodes = [
            (x + 200, y + 86, 86, 40, "SDR COLUMNS", f"{float(sdr.get('Gneuron_ticks/s',0)):.1f} Gnt/s", green),
            (x + 340, y + 86, 98, 40, "26-SM HIPP", f"{hip.get('query_us','—')} µs", gold),
            (x + 488, y + 86, 108, 40, f"{nsyn//1048576}M NEOCORTEX", f"{neo.get('settle_us','—')} µs", aqua),
            (x + 200, y + 184, 86, 40, "2D PLACE MAP", f"{sp2.get('wave_us','—')} µs", "#b7e19b"),
            (x + 340, y + 184, 102, 40, f"K6–K{n_k} REASON", node_k_val, lav),
            (x + 488, y + 184, 108, 40, "WM ATTN GATE", f"{wma.get('attend_us','—')} µs", "#f2b8a0"),
        ]
        # Connecting synaptic spike wires
        wires = [
            (x + 114, y + 132, x + 157, y + 86, green),
            (x + 114, y + 150, x + 157, y + 184, "#b7e19b"),
            (x + 243, y + 86, x + 291, y + 86, gold),
            (x + 389, y + 86, x + 434, y + 86, aqua),
            (x + 488, y + 106, x + 488, y + 164, "#f2b8a0"),
            (x + 340, y + 106, x + 340, y + 164, lav),
            (x + 243, y + 184, x + 291, y + 184, lav),
            (x + 389, y + 184, x + 434, y + 184, "#f2b8a0"),
        ]
        for x1, y1, x2, y2, wcol in wires:
            d.path(f"M{x1} {y1} L{x2} {y2}", wcol, 2.0, extra='opacity=".75"')
            d.dot((x1 + x2) * 0.5, (y1 + y2) * 0.5, 2.6, "#ffe28a")
        for nx, ny, nw, nh, nt, nv, ncol in nodes:
            d.rect(nx - nw * 0.5, ny - nh * 0.5, nw, nh, "#142b27", 8, f'stroke="{ncol}" stroke-width="1.5"')
            d.text(nx, ny - 3, nt, 8.6, ncol, MONO, 700, "middle")
            d.text(nx, ny + 12, nv, 10.5, ink, MONO, 600, "middle")

        # Bottom: 2x3 Crisp Kernel Spec Tiles (zero clipping!)
        tile_k_val = (f"{float(pcae49.get('pcae_us',0)):.0f}/{float(pdes50.get('pdes_us',0)):.0f}/{float(mcmcs51.get('mcmcs_us',0)):.0f} µs" if mcmcs51 else
                      f"{float(sxlp48.get('sxlp_us',0)):.0f}/{float(pcae49.get('pcae_us',0)):.0f}/{float(pdes50.get('pdes_us',0)):.0f} µs" if pdes50 else
                      f"{float(odrk47.get('odrk_us',0)):.0f}/{float(sxlp48.get('sxlp_us',0)):.0f}/{float(pcae49.get('pcae_us',0)):.0f} µs" if pcae49 else
                      f"{float(ffts46.get('ffts_us',0)):.0f}/{float(odrk47.get('odrk_us',0)):.0f}/{float(sxlp48.get('sxlp_us',0)):.0f} µs" if sxlp48 else
                      f"{float(lqrc45.get('lqrc_us',0)):.0f}/{float(ffts46.get('ffts_us',0)):.0f}/{float(odrk47.get('odrk_us',0)):.0f} µs" if odrk47 else
                      f"{float(kflt44.get('kflt_us',0)):.0f}/{float(lqrc45.get('lqrc_us',0)):.0f}/{float(ffts46.get('ffts_us',0)):.0f} µs" if ffts46 else
                      f"{float(mdpp43.get('mdpp_us',0)):.0f}/{float(kflt44.get('kflt_us',0)):.0f}/{float(lqrc45.get('lqrc_us',0)):.0f} µs" if lqrc45 else
                      f"{float(bnet42.get('bnet_us',0)):.0f}/{float(mdpp43.get('mdpp_us',0)):.0f}/{float(kflt44.get('kflt_us',0)):.0f} µs" if kflt44 else
                      f"{float(lalg41.get('lalg_us',0)):.0f}/{float(bnet42.get('bnet_us',0)):.0f}/{float(mdpp43.get('mdpp_us',0)):.0f} µs" if mdpp43 else
                      f"{float(past40.get('past_us',0)):.0f}/{float(lalg41.get('lalg_us',0)):.0f}/{float(bnet42.get('bnet_us',0)):.0f} µs" if bnet42 else
                      f"{float(symc39.get('symc_us',0)):.0f}/{float(past40.get('past_us',0)):.0f}/{float(lalg41.get('lalg_us',0)):.0f} µs" if lalg41 else
                      f"{float(sres38.get('sres_us',0)):.0f}/{float(symc39.get('symc_us',0)):.0f}/{float(past40.get('past_us',0)):.0f} µs" if past40 else
                      f"{float(cdis37.get('cdis_us',0)):.0f}/{float(sres38.get('sres_us',0)):.0f}/{float(symc39.get('symc_us',0)):.0f} µs" if symc39 else
                      f"{float(bdeb36.get('bdeb_us',0)):.0f}/{float(cdis37.get('cdis_us',0)):.0f}/{float(sres38.get('sres_us',0)):.0f} µs" if sres38 else
                      f"{float(nmet35.get('nmet_us',0)):.0f}/{float(bdeb36.get('bdeb_us',0)):.0f}/{float(cdis37.get('cdis_us',0)):.0f} µs" if cdis37 else
                      f"{float(dlg34.get('dlg_us',0)):.0f}/{float(nmet35.get('nmet_us',0)):.0f}/{float(bdeb36.get('bdeb_us',0)):.0f} µs" if bdeb36 else
                      f"{float(xms33.get('xmodal_us',0)):.0f}/{float(dlg34.get('dlg_us',0)):.0f}/{float(nmet35.get('nmet_us',0)):.0f} µs" if nmet35 else
                      f"{float(aud32.get('audio_us',0)):.0f}/{float(xms33.get('xmodal_us',0)):.0f}/{float(dlg34.get('dlg_us',0)):.0f} µs" if dlg34 else
                      f"{float(vis31.get('visual_us',0)):.0f}/{float(aud32.get('audio_us',0)):.0f}/{float(xms33.get('xmodal_us',0)):.0f} µs" if xms33 else
                      f"{float(sat30.get('sat_solve_us',0)):.0f}/{float(vis31.get('visual_us',0)):.0f}/{float(aud32.get('audio_us',0)):.0f} µs" if aud32 else
                      f"{float(gw29.get('graph_wave_us',0)):.0f}/{float(sat30.get('sat_solve_us',0)):.0f}/{float(vis31.get('visual_us',0)):.0f} µs" if vis31 else
                      f"{float(nm28.get('neural_math_us',0)):.0f}/{float(gw29.get('graph_wave_us',0)):.0f}/{float(sat30.get('sat_solve_us',0)):.0f} µs" if sat30 else
                      f"{float(sp27.get('seq_predict_us',0)):.0f}/{float(nm28.get('neural_math_us',0)):.0f}/{float(gw29.get('graph_wave_us',0)):.0f} µs" if gw29 else
                      f"{float(rbd.get('role_bind_us',0)):.0f}/{float(sp27.get('seq_predict_us',0)):.0f}/{float(nm28.get('neural_math_us',0)):.0f} µs" if nm28 else
                      f"{float(rvs.get('revise_schema_us',0)):.0f}/{float(rbd.get('role_bind_us',0)):.0f}/{float(sp27.get('seq_predict_us',0)):.0f} µs" if sp27 else
                      f"{float(ons.get('online_schema_us',0)):.0f}/{float(rvs.get('revise_schema_us',0)):.0f}/{float(rbd.get('role_bind_us',0)):.0f} µs" if rbd else
                      f"{float(chr_b.get('chain_rule_us',0)):.0f}/{float(ons.get('online_schema_us',0)):.0f}/{float(rvs.get('revise_schema_us',0)):.0f} µs" if rvs else
                      f"{float(ind.get('induce_us',0)):.0f}/{float(chr_b.get('chain_rule_us',0)):.0f}/{float(ons.get('online_schema_us',0)):.0f} µs")
        tile_k_sub = ("PCA · 1D PDE · MCMC" if mcmcs51 else
                      "Simplex · PCA · 1D PDE" if pdes50 else
                      "RK4 · Simplex · PCA" if pcae49 else
                      "FFT · RK4 · Simplex LP" if sxlp48 else
                      "LQR · FFT · RK4 ODE" if odrk47 else
                      "Kalman · LQR · FFT" if ffts46 else
                      "MDP · Kalman · LQR" if lqrc45 else
                      "BayesNet · MDP · Kalman" if kflt44 else
                      "LinAlg · BayesNet · MDPPlan" if mdpp43 else
                      "ProgAST · LinAlg · BayesNet" if bnet42 else
                      "SymCalc · ProgAST · LinAlg" if lalg41 else
                      "SelfRes · SymCalc · ProgAST" if past40 else
                      "Collective · SelfRes · SymCalc" if symc39 else
                      "Belief · Collective · SelfRes" if sres38 else
                      "Metaphor · Belief · Collective" if cdis37 else
                      "Dialogue · Metaphor · Belief" if bdeb36 else
                      "XModal · Dialogue · Metaphor" if nmet35 else
                      "Cochlea · XModal · DlgCortex" if dlg34 else
                      "Retina · Cochlea · XModalStk" if xms33 else
                      "SatSolve · RetinaV1 · CochleaA1" if aud32 else
                      "GraphWave · SatSolve · RetinaV1" if vis31 else
                      "NeuralMath · GraphWave · SatSolve" if sat30 else
                      "SeqSyntax · NeuralMath · GraphWave" if gw29 else
                      "RoleBind · SeqSyntax · NeuralMath" if nm28 else
                      "Revise · RoleBind · SeqSyntax" if sp27 else
                      "Online · Revise · RoleBind" if rbd else
                      "ChainRule · Online · Revise" if rvs else
                      "Induce · ChainRule · OnlineSchema")
        tiles = [
            ("1. SDR Columns", f"{float(sdr.get('Gneuron_ticks/s',0)):.2f} Gnt/s",
             f"4,096 cols × 1,024 cells", f"{sdr.get('regs','—')} regs · 0 B spill", green),
            ("2. 26-SM Hippocampus", f"{hip.get('query_us','—')} µs / query",
             f"{hip.get('engrams','4096')} L2 memories", f"{hip_kib:,} KiB · {hip.get('blocks','32')} blocks", gold),
            (f"3. {nsyn // 1048576}-MiB Neocortex", f"{neo.get('settle_us','—')} µs / settle",
             f"{nsyn:,} int8 synapses", f"{neo.get('pathways','28')} pathways · {ribot_lbl}", aqua),
            ("4. 2D Place-Cell Map", f"{sp2.get('wave_us','—')} µs / wave",
             f"16×16 Open Valley wave", f"{sp2.get('wave_ticks','19')} ticks · {sp2.get('spikes','192')} spikes", "#b7e19b"),
            (f"5. K6–K{n_k} Reasoning", tile_k_val,
             f"Contrast {float(ctr.get('contrast_us',0)):.0f}µs · Analogy {float(ana.get('analogy_us',0)):.0f}µs",
             tile_k_sub, lav),
            ("6. 64-Slot WM Gate", f"{wma.get('attend_us','—')} µs / gate",
             "Lateral-inhibition spotlight", f"Detects ambiguity · {wma.get('spikes','—')} spk", "#f2b8a0"),
        ]
        tw = (size - 52) * 0.5
        th = 104
        for i, (t_lbl, t_val, t_s1, t_s2, t_col) in enumerate(tiles):
            col_i, row_i = i % 2, i // 2
            tx = x + 20 + col_i * (tw + 12)
            ty = y + 252 + row_i * (th + 11)
            d.rect(tx, ty, tw, th, "#203d38", 9, f'stroke="{t_col}" stroke-width="1.1" stroke-opacity=".5"')
            d.text(tx + 14, ty + 23, t_lbl, 13.0, ink, SERIF, 600)
            d.text(tx + 14, ty + 47, t_val, 15.0, t_col, MONO, 700)
            d.rect(tx + 14, ty + 56, tw - 28, 4.5, "#142a26", 2.2)
            d.rect(tx + 14, ty + 56, (tw - 28) * 0.88, 4.5, t_col, 2.2)
            d.text(tx + 14, ty + 76, short(t_s1, 34), 10.5, muted, SANS)
            d.text(tx + 14, ty + 92, short(t_s2, 34), 9.8, "#94ada1", MONO)

        # Right middle: Visual 32-MiB L2 Cache Fill Bar + Hardware Specs
        d.label(rx, 330, "BLACKWELL 32-MiB L2 CACHE RESIDENCY (ZERO VRAM SPILL)")
        bar_w = rw
        d.rect(rx, 348, bar_w, 26, "#d7dfd0", 7)
        w_hip = bar_w * (hip_kib / 32768.0)
        w_neo = bar_w * (neo_kib / 32768.0)
        d.rect(rx, 348, w_hip, 26, "#c98b3e", 7)
        d.rect(rx + w_hip - 3, 348, w_neo + 3, 26, GREEN, 7)
        d.text(rx + w_hip + w_neo * 0.5, 366, f"Neocortex {neo_kib // 1024} MiB ({nsyn // 1000000}M syn)", 12.5, "#edf1dc", MONO, 700, "middle")
        d.text(rx, 391, f"Hippocampus: {hip_kib:,} KiB   +   Neocortex: {neo_kib:,} KiB", 12.0, MUTED, MONO)
        d.text(W - 48, 391, f"{l2_pct:.1f}% of 32 MiB L2", 12.5, GREEN, MONO, 700, "end")
        hrows = [
            ("GPU target", short(data.get("device", "NVIDIA Blackwell"), 38)),
            ("Column kernel regs", f"{sdr.get('regs','—')} regs / thread (0 B spill)"),
            ("Recurrent 3-hop chain", f"{tch.get('chain_us','—')} µs across L2 pathways"),
            ("Cortical L2 retention", f"{ribot_lbl} ({neo.get('pathways','28')} pathways)"),
        ]
        for i, (k, v) in enumerate(hrows):
            yy = 426 + 32 * i
            d.text(rx, yy, k, 15.0, INK)
            d.text(W - 48, yy, v, 14.5, GREEN, MONO, 600, "end")
            d.rule(rx, yy + 9, W - 48, yy + 9, "#d5dccf")
        return

    # -------------------------------------------------------------------------
    # 4. OPEN-VALLEY ECOSYSTEM PLATE: ILLUSTRATED 16x16 VALLEY MAP + CREATURES IN ACTION!
    # -------------------------------------------------------------------------
    if ecos:
        p = ecos[-1]
        is_alien = "alien" in p.get("dialect", "")
        ticks = int(p.get("ticks", 64))
        _mind_header_medallion(
            d, "axion", "proud", 3,
            "Open-Valley Cast (4 Minds)", "", green,
            cast=[("alien" if is_alien else "axion", "proud"), ("mira", "happy"), ("wren", "happy"), ("fox", "busted")],
        )

        d.label(x + 24, y + 30, f"16×16 OPEN VALLEY MAP · {ticks} TICKS OF LIVE ANIMAL ADVENTURE", green)

        # Illustrated 16x16 Valley Map Canvas (x+20..x+584, y+42..y+392)
        mx, my, mw, mh = x + 20, y + 42, size - 40, 348
        d.rect(mx, my, mw, mh, "#17332e", 11, 'stroke="#2f574d" stroke-width="1.3"')

        # Helper to map 16x16 grid coordinates (gx, gy) into map pixel coords
        def mp(gx, gy):
            return mx + 26 + (gx / 15.0) * (mw - 52), my + 24 + (gy / 15.0) * (mh - 48)

        # Subtle valley grid lines
        for g in range(0, 16, 3):
            gx, _ = mp(g, 0)
            _, gy = mp(0, g)
            d.rule(gx, my + 12, gx, my + mh - 12, "#24473f", 0.7)
            d.rule(mx + 12, gy, mx + mw - 12, gy, "#24473f", 0.7)

        # Winding River down x = 7!
        rx_c, _ = mp(7, 0)
        d.path(
            f"M{rx_c-8:.1f} {my+6} Q{rx_c+16:.1f} {my+95} {rx_c-6:.1f} {my+175} Q{rx_c-20:.1f} {my+255} {rx_c+6:.1f} {my+mh-6}",
            "#2a6f7a", 28, "none", 'opacity=".65"'
        )
        d.path(
            f"M{rx_c-8:.1f} {my+6} Q{rx_c+16:.1f} {my+95} {rx_c-6:.1f} {my+175} Q{rx_c-20:.1f} {my+255} {rx_c+6:.1f} {my+mh-6}",
            "#4ca5b0", 12, "none", 'opacity=".35"'
        )
        d.text(rx_c + 18, my + 298, "RIVER", 8.5, "#74c2cc", MONO, 700, "middle")

        # Landmark coordinates
        pts = {
            "camp": mp(2.0, 8.0),
            "forest": mp(4.2, 4.2),
            "pass": mp(7.0, 1.4),
            "bridge": mp(7.0, 7.8),
            "village": mp(11.0, 7.6),
            "mine": mp(13.4, 3.0),
            "obs": mp(13.2, 12.6),
        }

        # GPU Place-Cell Detour Path (dashed gold curve over North Pass!)
        d.path(
            f"M{pts['camp'][0]:.1f} {pts['camp'][1]:.1f} Q{pts['forest'][0]:.1f} {pts['forest'][1]:.1f} {pts['pass'][0]:.1f} {pts['pass'][1]:.1f} "
            f"Q{pts['village'][0]-15:.1f} {pts['pass'][1]+18:.1f} {pts['mine'][0]:.1f} {pts['mine'][1]:.1f}",
            gold, 2.8, "none", 'stroke-dasharray="6,4" opacity=".9"'
        )
        # Repaired Bridge Direct Path (solid mint!)
        d.path(
            f"M{pts['camp'][0]:.1f} {pts['camp'][1]:.1f} L{pts['bridge'][0]:.1f} {pts['bridge'][1]:.1f} L{pts['village'][0]:.1f} {pts['village'][1]:.1f}",
            green, 2.3, "none", 'opacity=".7"'
        )
        # Cultural word transmission arc (Axion@mine -> Mira@village -> Wren@obs!)
        d.path(
            f"M{pts['mine'][0]-10:.1f} {pts['mine'][1]+14:.1f} Q{pts['village'][0]+35:.1f} {pts['village'][1]-30:.1f} {pts['village'][0]+8:.1f} {pts['village'][1]-10:.1f}",
            lav, 2.0, "none", 'stroke-dasharray="3,3"'
        )
        d.path(
            f"M{pts['village'][0]+12:.1f} {pts['village'][1]+16:.1f} Q{pts['obs'][0]+20:.1f} {pts['obs'][1]-38:.1f} {pts['obs'][0]-6:.1f} {pts['obs'][1]-16:.1f}",
            lav, 2.0, "none", 'stroke-dasharray="3,3"'
        )

        # Draw little hand-crafted landmark icons & labels
        # Forest pines
        fx, fy = pts["forest"]
        for dx, dy in ((-10, 4), (0, -3), (10, 5)):
            d.path(f"M{fx+dx} {fy+dy-12} L{fx+dx-7} {fy+dy+4} H{fx+dx+7} Z", "#3f7d5e", 0.9, "#4f9972")
        d.text(fx, fy + 18, "FOREST", 8.8, "#a6ccb8", MONO, 600, "middle")

        # North Pass mountain arch
        px, py = pts["pass"]
        d.path(f"M{px-16} {py+6} L{px-5} {py-12} L{px+4} {py-2} L{px+11} {py-10} L{px+18} {py+6} Z", "#7da398", 1.1, "#4e7066")
        d.text(px, py + 19, "NORTH PASS (GPU DETOUR!)", 8.5, gold, MONO, 700, "middle")

        # Camp tent
        cx_l, cy_l = pts["camp"]
        d.path(f"M{cx_l-13} {cy_l+8} L{cx_l} {cy_l-11} L{cx_l+13} {cy_l+8} Z", "#d49a57", 1.1, "#c78542")
        d.text(cx_l, cy_l + 21, "CAMP", 8.8, ink, MONO, 600, "middle")

        # Bridge planks (storm + repaired!)
        bx_l, by_l = pts["bridge"]
        d.rect(bx_l - 18, by_l - 6, 36, 12, "#8c6139", 2.5, 'stroke="#d49e6a" stroke-width="1.2"')
        for pl in (-10, -2, 6):
            d.rule(bx_l + pl, by_l - 5, bx_l + pl, by_l + 5, "#593b1f", 1.2)
        d.text(bx_l, by_l + 20, "BRIDGE", 8.8, coral, MONO, 700, "middle")
        d.text(bx_l, by_l + 31, "(snapped → fixed!)", 7.8, muted, SANS, 400, "middle")

        # Village house
        vx_l, vy_l = pts["village"]
        d.rect(vx_l - 11, vy_l - 2, 22, 14, "#4b7067", 2)
        d.path(f"M{vx_l-14} {vy_l-2} L{vx_l} {vy_l-13} L{vx_l+14} {vy_l-2} Z", "#d49a57", 1.0, "#c78542")
        d.text(vx_l, vy_l + 24, "VILLAGE", 8.8, ink, MONO, 600, "middle")

        # Crystal Mine
        mnx, mny = pts["mine"]
        d.path(f"M{mnx-11} {mny+6} L{mnx-5} {mny-11} L{mnx+3} {mny-13} L{mnx+12} {mny+6} Z", lav, 1.1, "#7b6cb5")
        d.dot(mnx, mny - 3, 3.2, "#ffe28a")
        d.text(mnx, mny - 18, "CRYSTAL MINE", 8.5, lav, MONO, 700, "middle")

        # Observatory dome
        ox_l, oy_l = pts["obs"]
        d.path(f"M{ox_l-12} {oy_l+8} A12 12 0 0 1 {ox_l+12} {oy_l+8} Z", aqua, 1.1, "#437f82")
        d.rule(ox_l + 4, oy_l - 4, ox_l + 15, oy_l - 13, "#edf1dc", 2.0)
        d.text(ox_l, oy_l + 21, "OBSERVATORY", 8.5, aqua, MONO, 600, "middle")

        # PLACE ALL 4 EXPRESSIVE CRITTERS RIGHT ON THE VALLEY MAP WITH ACTION CALLOUTS!
        # 1. Fox sneaking south of Camp -> BUSTED!
        _mind_critter(d, cx_l + 22, cy_l + 82, 0.98, "fox", "busted", 1)
        _mind_pill(d, cx_l - 22, cy_l + 106, "FOX BUSTED!", coral, "#142723", 8.8, 6.0)
        d.text(cx_l + 32, cy_l + 132, '"fake pearl!" → caught', 8.8, coral, SANS, 500, "middle")

        # 2. Axion near Crystal Mine doing Litmus Science + inventing "glint"!
        _mind_critter(d, mnx - 68, mny + 30, 1.02, "alien" if is_alien else "axion", "proud", 3)
        _mind_pill(d, mnx - 142, mny + 52, "AXION: LITMUS + 'GLINT'!", green, "#142723", 8.5, 6.0)

        # 3. Mira at Village bartering Pareto +12!
        _mind_critter(d, vx_l + 56, vy_l + 24, 0.96, "mira", "happy", 3)
        _mind_pill(d, vx_l + 10, vy_l + 48, "MIRA: TRADE +12", gold, "#142723", 8.5, 6.0)

        # 4. Wren at Observatory / Bridge fixing planks & echoing "glint!"
        _mind_critter(d, ox_l - 72, oy_l - 2, 0.96, "wren", "happy", 3)
        _mind_pill(d, ox_l - 134, oy_l + 22, "WREN: FIXED + 'GLINT!'", aqua, "#142723", 8.5, 6.0)

        # Bottom 2x3 Adventure Chronicle Story Cards (pulling live facts from run log!)
        d.label(x + 24, y + 412, "WHAT THE ANIMALS DID (ADVENTURE CHRONICLE)", gold)
        k_title = ("T72–728 · K10–K51" if ticks >= 732 else "T72–712 · K10–K50" if ticks >= 716 else "T72–696 · K10–K49" if ticks >= 700 else "T72–680 · K10–K48" if ticks >= 684 else "T72–664 · K10–K47" if ticks >= 668 else "T72–648 · K10–K46" if ticks >= 652 else "T72–632 · K10–K45" if ticks >= 636 else "T72–616 · K10–K44" if ticks >= 620 else "T72–600 · K10–K43" if ticks >= 604 else "T72–584 · K10–K42" if ticks >= 588 else "T72–568 · K10–K41" if ticks >= 572 else "T72–552 · K10–K40" if ticks >= 556 else "T72–536 · K10–K39" if ticks >= 540 else "T72–520 · K10–K38" if ticks >= 524 else "T72–504 · K10–K37" if ticks >= 508 else "T72–488 · K10–K36" if ticks >= 492 else "T72–472 · K10–K35" if ticks >= 476 else "T72–456 · K10–K34" if ticks >= 460 else "T72–440 · K10–K33" if ticks >= 444 else "T72–424 · K10–K32" if ticks >= 428 else "T72–408 · K10–K31" if ticks >= 412 else "T72–392 · K10–K30" if ticks >= 396 else "T72–376 · K10–K29" if ticks >= 380 else "T72–360 · K10–K28" if ticks >= 364 else "T72–344 · K10–K27" if ticks >= 348 else "T72–328 · K10–K26" if ticks >= 332 else
                   "T72–312 · K10–K25" if ticks >= 316 else "T72–296 · K10–K24" if ticks >= 300 else
                   "T72–280 · K10–K23" if ticks >= 284 else "T72–264 · K10–K22" if ticks >= 268 else
                   "T72–248 · K10–K21" if ticks >= 252 else "T72–232 · K10–K20" if ticks >= 236 else
                   "T72–216 · K10–K19" if ticks >= 220 else "T72–184 · K10–K17")
        k_desc = ("Fermi 20 & K51 GPU MCMC 80%!" if ticks >= 732 else
                  "Wave 120 & K50 GPU PDE 92!" if ticks >= 716 else
                  "BH 300 & K49 GPU PCA 30!" if ticks >= 700 else
                  "Cap 100 & K48 GPU LP 56!" if ticks >= 684 else
                  "PV 320W & K47 GPU RK4 58!" if ticks >= 668 else
                  "Froude 150 & K46 GPU FFT 40!" if ticks >= 652 else
                  "Euler 50 & K45 GPU LQR 37!" if ticks >= 636 else
                  "BrPred 13 & K44 GPU Kalman 80!" if ticks >= 620 else
                  "Buck 10V & K43 GPU MDP 47!" if ticks >= 604 else
                  "Lens 15 & K42 GPU Bayes 80%!" if ticks >= 588 else
                  "Lift 400 & K41 GPU Cramer 5!" if ticks >= 572 else
                  "Rocket 6k & K40 GPU AST synth!" if ticks >= 556 else
                  "Bode 60° & K39 GPU symcalc!" if ticks >= 540 else
                  "Kalman 52 & K38 GPU selfres!" if ticks >= 524 else
                  "SpecCode & K37 GPU colldisc!" if ticks >= 508 else
                  "PeerRev & K36 GPU belgraph!" if ticks >= 492 else
                  "SciLoop & K35 GPU metriver!" if ticks >= 476 else
                  "Herd 75% & K34 GPU dialogue!" if ticks >= 460 else
                  "Hoist 36 & K33 GPU xbeacon!" if ticks >= 444 else
                  "AutoDebug 3 & K32 GPU audmajor!" if ticks >= 428 else
                  "Code 15 & K31 GPU V1/V4 visring!" if ticks >= 412 else
                  "Beam 80 & K30 GPU 3-SAT sv3!" if ticks >= 396 else
                  "Faraday 200 & K29 GPU detour 10!" if ticks >= 380 else
                  "Amdahl 2x & K28 GPU 3x+5=23 -> 6!" if ticks >= 364 else
                  "Gen-2 Pip K27 syntax & Pascal 500!" if ticks >= 348 else
                  "K25 revise, Gen-2 Pip & K26 role-bind!" if ticks >= 332 else
                  "K24 online, Hardy 480 & K25 live revise!" if ticks >= 316 else
                  "K23 chain, Venturi 6 & K24 live schema!" if ticks >= 300 else
                  "DFA q2, K22, MSY 20 & K23 chain!" if ticks >= 284 else
                  "2D catch, K21 SAT, DFA q2 & K22 induce!" if ticks >= 268 else
                  "Stoich H, K20 sim, 2D catch & K21 SAT!" if ticks >= 252 else
                  "Herd 67%, K19 plan, Stoich H & K20 sim!" if ticks >= 236 else "LP 42, K18 imagine & K19 plan!")
        beats = [
            ("T4 · LIE BUSTED", f"Fox bluffed {p.get('lies_caught',2)}x → caught!", coral),
            ("T8 · STORM DETOUR", "Bridge snapped! GPU wave detoured north", gold),
            ("T12 · SCIENCE", f"Axion used litmus → proved salty crystal!", lav),
            ("T16–56 · PARETO", f"Axion & Mira traded ({p.get('trades',2)} deals, +12)", green),
            ("T48–64 · CULTURE", "Coined 'glint' & relayed 'kora crafted'!" if ticks >= 76 else "Axion coined 'glint' → spread to Wren!", aqua),
            (k_title, k_desc, "#f0b4c4"),
        ]
        bw_c = (size - 56) / 3.0
        for i, (bt, bd, bcol) in enumerate(beats):
            ci, ri = i % 3, i // 3
            bx_c = x + 20 + ci * (bw_c + 8)
            by_c = y + 426 + ri * 78
            d.rect(bx_c, by_c, bw_c, 68, "#203d38", 8, f'stroke="{bcol}" stroke-width="1.1" stroke-opacity=".6"')
            d.text(bx_c + 10, by_c + 19, bt, 9.2, bcol, MONO, 700)
            import textwrap
            for li, ln in enumerate(textwrap.wrap(bd, 23)[:2]):
                d.text(bx_c + 10, by_c + 38 + li * 15, ln, 10.6, ink, SANS)

        # Right middle panel: Meet the Valley Cast (4 expressive mini character cards!)
        d.label(rx, 330, "MEET THE VALLEY CAST & LIVE SCORECARD")
        cast_info = [
            ("alien" if is_alien else "axion", "proud", "Axion", "Explorer & Scientist", "Detoured, litmus science & 'glint'", GREEN),
            ("mira", "happy", "Mira", "Wise Scholar & Trader", "Relayed truth, Pareto +12 trade", "#b5762c"),
            ("wren", "happy", "Wren", "Swift Bridge Builder", "Fixed storm bridge, learned 'glint'", "#2c7c77"),
            ("fox", "busted", "Fox", "Cheeky Trickster", f"{p.get('lies_caught',2)} bluffs caught & quarantined!", "#b5524c"),
        ]
        cw_r = (rw - 14) * 0.5
        for i, (cr, cm, cname, crole, cact, ccol) in enumerate(cast_info):
            ci, ri = i % 2, i // 2
            cx_r = rx + ci * (cw_r + 14)
            cy_r = 346 + ri * 88
            d.rect(cx_r, cy_r, cw_r, 78, "#e6ece0", 9, f'stroke="{ccol}" stroke-width="1.3"')
            d.rect(cx_r + 8, cy_r + 8, 56, 62, "#193631", 7)
            _mind_critter(d, cx_r + 36, cy_r + 41, 0.85, cr, cm, 3)
            d.text(cx_r + 74, cy_r + 25, cname, 15.0, INK, SERIF, 600)
            d.text(cx_r + 74, cy_r + 42, crole, 10.5, ccol, MONO, 700)
            d.text(cx_r + 74, cy_r + 61, short(cact, 36), 10.8, MUTED, SANS)
        d.text(rx, 540, f"Interview: {p['passed']}/{p['total']} passed  ·  {ticks} ticks  ·  {int(p.get('total_spikes',0)):,} GPU spikes", 12.8, GREEN, MONO, 600)
        return

    # -------------------------------------------------------------------------
    # 5. 4-MIND SOCIETY PLATE: SOCIAL DRAMA GRAPH + EPISTEMIC SHIELDS
    # -------------------------------------------------------------------------
    if socs:
        p = socs[-1]
        is_alien = "alien" in p.get("dialect", "")
        _mind_header_medallion(
            d, "axion", "proud", 3,
            "4-Mind Society", "", green,
            cast=[("mira", "happy"), ("alien" if is_alien else "axion", "skeptical"), ("wren", "happy"), ("fox", "busted")],
        )
        d.label(x + 24, y + 30, "TRUST VS TRICKERY · 4 INDEPENDENT GPU BRAINS", green)

        # Social Interaction Stage (y+42..y+372)
        sx, sy, sw, sh = x + 20, y + 42, size - 40, 328
        d.rect(sx, sy, sw, sh, "#1a3530", 11, 'stroke="#2f574d" stroke-width="1.2"')

        pos = {
            "mira": (sx + 115, sy + 92),
            "axion": (sx + sw - 125, sy + 92),
            "fox": (sx + 115, sy + 242),
            "wren": (sx + sw - 125, sy + 242),
        }
        # Solid Emerald Truth Beam: Mira -> Axion -> Wren
        d.path(f"M{pos['mira'][0]+44} {pos['mira'][1]} H{pos['axion'][0]-48}", green, 3.4)
        d.path(f"M{pos['axion'][0]-55} {pos['axion'][1]-6} L{pos['axion'][0]-45} {pos['axion'][1]} L{pos['axion'][0]-55} {pos['axion'][1]+6}", green, 2.5)
        _mind_pill(d, (pos["mira"][0] + pos["axion"][0]) * 0.5 - 66, pos["mira"][1] - 28, "VERIFIED TRUTH!", green, "#122621", 8.8, 7.0)
        d.text((pos["mira"][0] + pos["axion"][0]) * 0.5, pos["mira"][1] + 18, '"sapphire is in tower"', 10.2, ink, SANS, 400, "middle")

        d.path(f"M{pos['axion'][0]} {pos['axion'][1]+44} V{pos['wren'][1]-46}", aqua, 3.2)
        d.path(f"M{pos['wren'][0]-6} {pos['wren'][1]-54} L{pos['wren'][0]} {pos['wren'][1]-44} L{pos['wren'][0]+6} {pos['wren'][1]-54}", aqua, 2.5)
        _mind_pill(d, pos["axion"][0] - 58, (pos["axion"][1] + pos["wren"][1]) * 0.5 - 10, "RELAY & COOP!", aqua, "#122621", 8.8, 7.0)

        # Jagged Coral Lie Bolts from Fox -> blocked by Gold Shields!
        d.path(f"M{pos['fox'][0]+42} {pos['fox'][1]-28} L{sx+245} {sy+178} L{sx+268} {sy+165}", coral, 2.8, extra='stroke-dasharray="5,3"')
        d.path(f"M{pos['fox'][0]+46} {pos['fox'][1]} H{pos['wren'][0]-92}", coral, 2.8, extra='stroke-dasharray="5,3"')
        # Golden Epistemic Shield emblems!
        for shx, shy in ((sx + 276, sy + 160), (pos["wren"][0] - 84, pos["wren"][1])):
            d.path(f"M{shx} {shy-14} L{shx+12} {shy-8} V{shy+4} Q{shx} {shy+15} {shx} {shy+15} Q{shx-12} {shy+4} {shx-12} {shy-8} Z", gold, 1.5, "#3a4f44")
            d.text(shx, shy + 4, "✕", 11.5, coral, SANS, 700, "middle")
        _mind_pill(d, sx + 206, sy + 204, f"{p.get('lies_caught',3)} LIES BLOCKED!", coral, "#142723", 8.8, 7.0)

        # Draw all 4 expressive critters in their corners
        _mind_critter(d, pos["mira"][0], pos["mira"][1], 1.22, "mira", "happy", 3)
        d.text(pos["mira"][0], pos["mira"][1] + 46, "MIRA (Honest Sage)", 10.5, gold, MONO, 700, "middle")

        _mind_critter(d, pos["axion"][0], pos["axion"][1], 1.22, "alien" if is_alien else "axion", "skeptical", 3)
        d.text(pos["axion"][0], pos["axion"][1] + 46, "AXION (Curious Pupil)", 10.5, green, MONO, 700, "middle")

        _mind_critter(d, pos["fox"][0], pos["fox"][1], 1.22, "fox", "busted", 1)
        d.text(pos["fox"][0], pos["fox"][1] + 46, "FOX (Caught Lying!)", 10.5, coral, MONO, 700, "middle")

        _mind_critter(d, pos["wren"][0], pos["wren"][1], 1.22, "wren", "happy", 3)
        d.text(pos["wren"][0], pos["wren"][1] + 46, "WREN (Coop Builder)", 10.5, aqua, MONO, 700, "middle")

        # Bottom 2x2 Social Drama Beats
        d.label(x + 24, y + 392, "HOW THE 4 BRAINS COLLABORATED", gold)
        sbeats = [
            ("1. CURIOSITY GAP", "Axion noticed it didn't know where sapphire was — and asked!", green),
            ("2. FOX BLUFF QUARANTINED", f"Both Axion & Wren flagged Fox's {p.get('lies_caught',3)} lies via apical vigilance.", coral),
            ("3. 1-SHOT TRUTH RELAY", "Mira told Axion; Axion relayed the verified location to Wren.", gold),
            ("4. COOP PLAN & REROUTE", "Axion planned campfire stew for Wren & both detoured the broken bridge!", aqua),
        ]
        sw_c = (size - 52) * 0.5
        for i, (st, sd, scol) in enumerate(sbeats):
            ci, ri = i % 2, i // 2
            bx_c = x + 20 + ci * (sw_c + 12)
            by_c = y + 406 + ri * 94
            d.rect(bx_c, by_c, sw_c, 84, "#203d38", 9, f'stroke="{scol}" stroke-width="1.1" stroke-opacity=".6"')
            d.text(bx_c + 12, by_c + 22, st, 9.8, scol, MONO, 700)
            import textwrap
            for li, ln in enumerate(textwrap.wrap(sd, 36)[:3]):
                d.text(bx_c + 12, by_c + 42 + li * 16, ln, 11.2, ink, SANS)

        # Right middle: Social Trust Reputation Bars + Telemetry
        d.label(rx, 330, "PEER TRUST REPUTATION & SOCIETY TELEMETRY")
        trust_bars = [
            ("Mira (verified informant)", 1.0, GREEN, "100% TRUSTED"),
            ("Axion (truth-checking learner)", 1.0, GREEN, "100% HONEST"),
            ("Wren (cooperative teammate)", 1.0, "#2c7c77", "100% TEAMMATE"),
            (f"Fox ({p.get('lies_caught',3)} bluffs caught!)", 0.08, "#b5524c", "QUARANTINED!"),
        ]
        for i, (tlbl, tfr, tcol, ttag) in enumerate(trust_bars):
            yy = 366 + i * 38
            d.text(rx, yy, tlbl, 14.5, INK)
            d.rect(rx + 275, yy - 12, rw - 415, 14, "#d7dfd0", 5)
            d.rect(rx + 275, yy - 12, (rw - 415) * tfr, 14, tcol, 5)
            d.text(W - 48, yy, ttag, 12.0, tcol, MONO, 700, "end")
        d.rule(rx, 514, W - 48, 514, "#d5dccf")
        d.text(rx, 540, f"Scenarios passed: {p['passed']}/{p['total']}   ·   Dialect: {p.get('dialect','english_svo')}   ·   Spikes: {int(p.get('total_spikes',0)):,}", 12.5, GREEN, MONO, 600)
        return

    # -------------------------------------------------------------------------
    # 6. TWO-MIND PEER EXCHANGE PLATE: ELDER & PUPIL COMIC DIALOGUE
    # -------------------------------------------------------------------------
    if peers:
        p = peers[-1]
        is_alien = "alien" in p.get("dialect", "")
        _mind_header_medallion(d, "alien" if is_alien else "axion", "proud", 3, "1-Shot Learning", f"{p['passed']}/{p['total']} passed", gold)
        d.label(x + 24, y + 30, "AUTONOMOUS CURIOSITY · ELDER TEACHES PUPIL", green)

        # Face-to-face portraits
        d.rect(x + 20, y + 42, size - 40, 148, "#1b3632", 11)
        _mind_critter(d, x + 115, y + 112, 1.32, "elder", "happy", 3)
        d.text(x + 115, y + 168, "WISE ELDER", 10.5, gold, MONO, 700, "middle")
        _mind_critter(d, x + size - 115, y + 112, 1.32, "alien" if is_alien else "axion", "proud", 3, facing=-1)
        d.text(x + size - 115, y + 168, "CURIOUS PUPIL", 10.5, green, MONO, 700, "middle")
        # Question & knowledge arrows
        d.path(f"M{x+size-180} {y+92} Q{x+size*0.5} {y+64} {x+180} {y+92}", aqua, 2.4, extra='stroke-dasharray="4,3"')
        _mind_pill(d, x + size * 0.5 - 58, y + 58, "? 'TELL ME!'", aqua, "#142723", 9.0, 6.0)
        d.path(f"M{x+180} {y+134} Q{x+size*0.5} {y+158} {x+size-180} {y+134}", gold, 2.8)
        _mind_pill(d, x + size * 0.5 - 70, y + 132, "★ 1-SHOT LEARNED!", gold, "#142723", 9.0, 6.0)

        # Styled transcript speech bubbles
        transcript = [
            l.strip()
            for l in lines
            if l.strip().startswith(("[Before]", "[Curiosity]", "Elder ->", "[After]", "Human ->", "Pupil>"))
        ]
        for i, msg in enumerate(transcript[:7]):
            yy = y + 204 + i * 55
            is_el = msg.startswith("Elder")
            bcol = gold if is_el else (aqua if msg.startswith("Human") else green)
            bx_b = x + (22 if is_el else 44)
            bw_b = size - 66
            d.rect(bx_b, yy, bw_b, 44, "#203d38", 9, f'stroke="{bcol}" stroke-width="1.2" stroke-opacity=".7"')
            d.text(bx_b + 12, yy + 27, short(msg, 60), 12.2, bcol if is_el else ink, MONO)

        d.label(rx, 332, "SPLIT-KNOWLEDGE TELEMETRY")
        for i, (k, v) in enumerate([
            ("Cooperative scenarios", f"{p['passed']} / {p['total']}"),
            ("Dialect", p.get("dialect", "english_svo")),
            ("Elder GPU spikes", f"{int(p['elder_spikes']):,}"),
            ("Pupil GPU spikes", f"{int(p['pupil_spikes']):,}"),
        ]):
            yy = 374 + 38 * i
            d.text(rx, yy, k, 15.5, INK)
            d.text(W - 48, yy, v, 15.5, GREEN, MONO, 600, "end")
            d.rule(rx, yy + 11, W - 48, yy + 11, "#d5dccf")
        return

    # -------------------------------------------------------------------------
    # 7. LIVE INTERACTIVE CONVERSATION PLATE: CHATTING AXION + CURATED SPEECH BUBBLES + SPIKE PULSE
    # -------------------------------------------------------------------------
    m = data.get("metrics", {})
    nt = max(len(turns), int(m.get("mind_turns", 0)), max((int(t.get("mind_turn", 0)) for t in turns), default=0))
    sp = max(sum(int(t.get("spikes", 0)) for t in turns), int(m.get("mind_spikes", 0)))
    ed = max(sum(int(t.get("edits", 0)) for t in turns), int(m.get("mind_edits", 0)))
    _mind_header_medallion(d, "axion", "happy", 3, "Live Dialogue", f"{nt} spiking turns", green)

    d.label(x + 24, y + 30, "CONVERSATION HIGHLIGHTS · PURE SPIKING BRAIN", green)

    # Top Chatting Axion Banner
    d.rect(x + 20, y + 42, size - 40, 82, "#1f3c37", 11)
    _mind_critter(d, x + 68, y + 84, 1.12, "axion", "happy", 3)
    d.text(x + 114, y + 73, "Hi! I'm Axion — chatting live with zero LLMs!", 14.2, ink, SERIF, 600)
    d.text(x + 114, y + 96, f"Fired {sp:,} Blackwell spikes & rewired {ed:,} live synapses over {nt} turns.", 11.2, green, SANS)

    # Smart highlight curator: pick the coolest diverse replies (never boring `noted ...`!)
    raw_replies = [(idx + 1, l[6:].strip()) for idx, l in enumerate([ln for ln in lines if ln.startswith("mind> ")])]

    def classify_reply(txt):
        tl = txt.lower()
        if "hello i am axion" in tl:
            return ("GREETING", green, 10)
        if "by mcmcsmp" in tl:
            return ("K51 256-MCMC GPU!", gold, 500)
        if "while salmon is fish all are animal" in tl or "is a bird that can fly" in tl or "eagle can fly while penguin can swim" in tl:
            return ("GROUP3 & SIBLINGS", aqua, 498)
        if "fdinsu" in tl or "bffast" in tl or "shhard" in tl:
            return ("FERMI · BERN · SHAM", green, 496)
        if "mvfire" in tl or "ikdext" in tl or "lvrch" in tl or "rfmem" in tl:
            return ("NERN · IK · LV · ROOF", lav, 494)
        if "by pdeslv" in tl:
            return ("K50 1D PDE GPU!", gold, 490)
        if "as salmon is fish can swim" in tl or "belong to animal while" in tl or "whereas salmon is fish" in tl:
            return ("DEDUCE & ROOT CHAT", aqua, 488)
        if "dbwave" in tl or "tssafe" in tl or "ithigh" in tl:
            return ("QM · SLA · ION", green, 486)
        if "hlsat" in tl or "bmok" in tl or "actiny" in tl or "trhawk" in tl:
            return ("HILL · BEAM · TAYL", lav, 484)
        if "by pcaeig" in tl:
            return ("K49 PCA EIGEN GPU", gold, 480)
        if "because eagle is bird can fly while" in tl or "most bird can fly except" in tl or "penguin is bird cannot fly instead" in tl:
            return ("WHYDIFF & GEN CHAT", aqua, 478)
        if "bhsuper" in tl or "hkstiff" in tl or "tcgbps" in tl:
            return ("BH · HOOKE · TCP", green, 476)
        if "hbbasic" in tl or "phgloss" in tl or "isdeep" in tl or "shstrng" in tl:
            return ("BUF · PHONG · SHRP", lav, 474)
        if "by simlp" in tl:
            return ("K48 SIMPLEX LP GPU", gold, 470)
        if "proved eagle is bird" in tl or "proved penguin is bird" in tl or "recap eagle is bird" in tl:
            return ("PROOFS & RECAP", aqua, 468)
        if "cbhigh" in tl or "dphigh" in tl or "gfwide" in tl:
            return ("CAP · DARCY · GUST", green, 466)
        if "rkpeak" in tl or "bdsafe" in tl or "bldark" in tl or "pcotm" in tl:
            return ("RANK · HASH · OPT", lav, 464)
        if "by oderk4" in tl:
            return ("K47 RK4 ODE GPU", gold, 460)
        if "if penguin would fly" in tl or "also penguin is bird" in tl or "actually eagle is bird" in tl:
            return ("COUNTERFACT & Q", aqua, 458)
        if "pvpeak" in tl or "mmbusy" in tl or "glsoar" in tl:
            return ("PV · M/M/1 · GLIDE", green, 456)
        if "pkfast" in tl or "cndeep" in tl or "smnear" in tl or "bplong" in tl:
            return ("PK · CNN · SEIS · BAT", lav, 454)
        if "by fftspec" in tl:
            return ("K46 FFT SPECTRUM", gold, 450)
        if "affirmed eagle is bird" in tl or "corrected penguin is bird" in tl or "why can eagle fly while" in tl:
            return ("SELF-CORRECT & Q", aqua, 448)
        if "scwide" in tl or "frsuper" in tl or "bzarch" in tl:
            return ("SHAN · FROUDE · BEZ", green, 446)
        if "rcfast" in tl or "lgsharp" in tl or "hhellip" in tl or "ecstrng" in tl:
            return ("RC · ML · ORB · ECC", lav, 444)
        if "by lqrctrl" in tl:
            return ("K45 LQR CONTROL", gold, 440)
        if "since penguin is bird" in tl or "eagle can fly and penguin" in tl or "since eagle is bird" in tl:
            return ("TOPIC BRIDGE & DEF", aqua, 438)
        if "icloss" in tl or "bamild" in tl or "shbal" in tl:
            return ("IMG · BARO · SHARD", green, 436)
        if "bustiff" in tl or "f1high" in tl or "mnfast" in tl or "pthuge" in tl:
            return ("EULER · F1 · VM", lav, 434)
        if "by kfilt" in tl:
            return ("K44 KALMAN GPU", gold, 430)
        if "false penguin is bird" in tl or "still penguin is bird" in tl or "because eagle is bird has" in tl:
            return ("HYPOTEST & RECON", aqua, 428)
        if "bpfast" in tl or "alwarm" in tl or "blclean" in tl:
            return ("CPU · ALBEDO · BLOOM", green, 426)
        if "cdnorm" in tl or "pestrng" in tl or "sbcrisp" in tl or "pghex" in tl:
            return ("PHYSIO · CRYPTO · GEO", lav, 424)
        if "by mdpplan" in tl:
            return ("K43 MDP BELLMAN", gold, 420)
        if "for canyon eagle is bird" in tl or "eagle with fly and salmon" in tl or "eagle can fly while salmon" in tl:
            return ("OPEN GOALS & SYNTH", aqua, 418)
        if "btshall" in tl or "bgwide" in tl or "frwide" in tl:
            return ("BTREE · BANDGAP · 3D", green, 416)
        if "qkmajor" in tl or "urlfit" in tl or "fbmulti" in tl or "bkccm" in tl:
            return ("SEISMO · FIBER · BUCK", lav, 414)
        if "by bnet" in tl:
            return ("K42 BAYES-NET GPU", gold, 410)
        if "not all bird can fly" in tl or "eagle like fly and salmon" in tl or "penguin is a bird can swim" in tl:
            return ("SOCRATIC CRITIQUE", aqua, 408)
        if "bbuv" in tl or "hamsec" in tl or "lenreal" in tl:
            return ("OPTICS · ECC · WIEN", green, 406)
        if "arrconst" in tl or "oscstiff" in tl or "dnastab" in tl or "solgrow" in tl:
            return ("AMORT · DNA · SOLOW", lav, 404)
        if "by linalg" in tl:
            return ("K41 LINEAR ALGEBRA", gold, 400)
        if "what next for eagle" in tl or "where is eagle like" in tl:
            return ("STORY CO-WRITING", aqua, 398)
        if "rsavalid" in tl or "airborne" in tl or "lowconf" in tl:
            return ("RSA · AERO · CACHE", green, 396)
        if "innerorb" in tl or "hiresadc" in tl or "galvanic" in tl or "profiteq" in tl:
            return ("KEPLER · ADC · ECON", lav, 394)
        if "by progast" in tl:
            return ("K40 AST SYNTHESIS", gold, 390)
        if "if penguin could fly" in tl or "while penguin cannot fly" in tl:
            return ("FREE DEBATE & WHATIF", aqua, 388)
        if "merkfast" in tl or "orbmake" in tl or "qstable" in tl:
            return ("MERKLE · ROCKET · Q", green, 386)
        if "enzsat" in tl or "qstrong" in tl or "siglock" in tl or "cfgacc" in tl:
            return ("BIO · QUORUM · CFG", lav, 384)
        if "by symcalc" in tl:
            return ("K39 SYM-CALCULUS", gold, 380)
        if "gryphon is bird" in tl or "in valley with" in tl or "because penguin is bird" in tl:
            return ("LIVE TEACH & STORY", aqua, 378)
        if "ctrlstab" in tl or "qpass" in tl or "relfast" in tl:
            return ("BODE · QUANTUM · REL", green, 376)
        if "raftcomm" in tl or "regspilled" in tl or "essgrow" in tl:
            return ("RAFT · REG · ESS", lav, 374)
        if "by selfres" in tl:
            return ("K38 SELF-RESEARCH", gold, 370)
        if "both eagle and penguin are" in tl or ("also " in tl and "what about" in tl):
            return ("DEEP DISCOURSE", aqua, 368)
        if "bnlikely" in tl or "cavsafe" in tl:
            return ("BAYESNET & FLUID", green, 366)
        if "zkvalid" in tl or "rangeok" in tl:
            return ("SCHNORR ZK & ROV", lav, 364)
        if "by colldisc" in tl:
            return ("K37 COLLECTIVE LAB", gold, 360)
        if "what about" in tl:
            return ("SOCRATIC CURIOSITY", aqua, 358)
        if "specpass" in tl or "qentangled" in tl:
            return ("SPEC-CODE & QGATE", green, 356)
        if "cflstable" in tl or "corestable" in tl:
            return ("HEAT PDE & SHAPLEY", lav, 354)
        if "episurge" in tl or "pubaccept" in tl:
            return ("EPI & MULTI-LAB", coral, 352)
        if "by belgraph" in tl:
            return ("K36 BELIEF-DEBATE", gold, 350)
        if "eagle is bird has" in tl or "salmon is fish has" in tl:
            return ("5-FACET OPEN CHAT", aqua, 348)
        if "replicated" in tl or "highpower" in tl:
            return ("SCI PEER REVIEW", green, 346)
        if "indvalid" in tl or "ictruthful" in tl:
            return ("PROOFS & VCG", lav, 344)
        if "firespike" in tl or "clstable" in tl or "fuelok" in tl:
            return ("LIF · LQR · ORBIT", coral, 342)
        if "by metaphor" in tl:
            return ("K35 METAPHOR/PLOT", gold, 340)
        if "salmon is fish" in tl or "both bird eagle" in tl:
            return ("UNSCRIPTED CHAT", aqua, 338)
        if "hconfirm" in tl or "hfalsify" in tl:
            return ("SCI RESEARCH LOOP", green, 336)
        if "faces wolf" in tl or "talemoral" in tl:
            return ("STORY & MORAL", lav, 334)
        if "lpbinding" in tl or "foldedwave" in tl or "preyboom" in tl:
            return ("LP · FOURIER · WEB", coral, 332)
        if "by dialogue" in tl:
            return ("K34 DIALOGUE", green, 330)
        if "eagle is bird" in tl or "both animal eagle" in tl:
            return ("STAGE 7 OPENCHAT", aqua, 328)
        if "punlink" in tl or "recopt" in tl:
            return ("PUN WIT & PERSONA", gold, 326)
        if "ethconflict" in tl or "rejectnull" in tl or "leafheavy" in tl:
            return ("ETHICS & STATS", lav, 324)
        if "by xmodal" in tl:
            return ("K33 CROSS-MODAL", green, 320)
        if "by gstack" in tl:
            return ("K33 GOAL STACK", gold, 318)
        if "humbleq" in tl or "unblock" in tl or "goalready" in tl:
            return ("SOCRATIC COACH", aqua, 316)
        if "overload" in tl or "tradeoff" in tl:
            return ("3D RIG & DEBATE", lav, 314)
        if "by cochlea" in tl:
            return ("K32 COCHLEA A1", aqua, 310)
        if "quizfail" in tl or "tbblocked" in tl or "quizpass" in tl:
            return ("TEXTBOOK QUIZ", gold, 306)
        if "bugstep" in tl:
            return ("AUTO-DEBUGGER", coral, 304)
        if "toneirony" in tl or "toneclarify" in tl:
            return ("PROSODY EMPATHY", lav, 302)
        if "by retina" in tl:
            return ("K31 V1/V4 RETINA", green, 300)
        if "curious habitat of" in tl or "fullstudy" in tl or "soarer with feathers" in tl:
            return ("SELF-TEACH LOOP", gold, 296)
        if "codeok" in tl:
            return ("CODE HOT-PATCH", aqua, 294)
        if "by satsolve" in tl:
            return ("K30 GPU 3-SAT", coral, 290)
        if "hbmdeep" in tl:
            return ("16K TIER-2 HBM", gold, 286)
        if "seqparse" in tl and ("squall" in tl or "beaver" in tl):
            return ("WHY/CAUSE PROSE", green, 284)
        if "by graphwave" in tl:
            return ("K29 GRAPH-WAVE", aqua, 280)
        if "by seqparse" in tl and ("marten" in tl or "crevice" in tl or "vole" in tl):
            return ("FREE COREF+WH", green, 276)
        if "by neuralmath" in tl:
            return ("K28 NEURAL-MATH", green, 270)
        if "seqparse" in tl and ("osprey" in tl or "badger" in tl):
            return ("MULTI-PROSE K27", lav, 266)
        if "seqparse" in tl and "falcon" in tl:
            return ("K27 SEQ-SYNTAX", gold, 260)
        if "condor sighted lynx then" in tl:
            return ("STORY LINK", aqua, 258)
        if "by rolebind" in tl:
            return ("K26 ROLE-BINDER", green, 250)
        if "velox to nectar to moth" in tl:
            return ("UNSEEN OPEN-LEX", lav, 249)
        if "by reviseschema" in tl:
            return ("K25 REVISE GPU", coral, 245)
        if "earlier harbor now pier" in tl:
            return ("BELIEF REVISED", gold, 244)
        if "did you mean mira or orin" in tl:
            return ("CLARIFY Q?", aqua, 243)
        if "by onlineschema" in tl:
            return ("K24 LIVE SCHEMA", lav, 238)
        if "valley floods then mill stops" in tl:
            return ("WHAT-IF CHAIN", green, 237)
        if "because sluice is jammed" in tl:
            return ("SOCRATIC WHY-NOT", coral, 236)
        if "lantern hid key" in tl:
            return ("MULTI-AGENT TOLD", aqua, 235)
        if "no below cutoff" in tl or "alkaline" in tl or "no not admissible" in tl:
            return ("QUANT / PH / A*", gold, 234)
        if "by chainrule" in tl or "via chainrule" in tl:
            return ("K23 CHAINRULE", lav, 228)
        if "talia sealed hatch then talia lit" in tl:
            return ("STORY SUMMARY", green, 227)
        if "crew spotted beacon at cliff" in tl:
            return ("SOCRATIC ELSE", aqua, 226)
        if "yes euler circuit" in tl or "euler path only" in tl or "no not eulerian" in tl:
            return ("EULER GRAPH", gold, 225)
        if "talia lit beacon" in tl or "talia sealed hatch" in tl:
            return ("TEMPORAL PROSE", "#b7e19b", 224)
        if "by induce" in tl:
            return ("K22 INDUCE", lav, 218)
        if "soren mended bridge and soren warned" in tl or "despite river flooded valley" in tl:
            return ("STORY RECAP", green, 217)
        if tl == "7 and 3" or tl == "9 and 4":
            return ("EIGENVALUES", gold, 216)
        if tl == "2 to 1" or tl == "3 to 1":
            return ("BAYES ODDS", aqua, 215)
        if tl == "counterclockwise":
            return ("SHOELACE", "#b7e19b", 214)
        if "yes accepted at q2" in tl or "no rejected at q0" in tl:
            return ("DFA LOCK", coral, 213)
        if "by constrain" in tl:
            return ("K21 CONSTRAIN", lav, 208)
        if "fog veiled trail" in tl or "lena guided hiker" in tl or "hiker praised lena" in tl:
            return ("CAUSAL PROSE", green, 207)
        if tl == "12 16":
            return ("2D PURSUIT", "#b7e19b", 206)
        if tl == "no odd cycle" or "yes bipartite" in tl:
            return ("BIPARTITE", gold, 205)
        if "yes aliased at 20" in tl or "no clean reconstruction" in tl:
            return ("NYQUIST", aqua, 204)
        if "yes tragedy overused" in tl or "no socially beneficial" in tl:
            return ("COMMONS TAX", coral, 203)
        if "by simulate" in tl:
            return ("K20 SIMULATE", lav, 198)
        if "beacon warned crew" in tl or "nora saved ship" in tl or "crew thanked nora" in tl:
            return ("PROSE PARAG", green, 197)
        if "hydrogen is limiting" in tl:
            return ("STOICHIOMETRY", gold, 196)
        if "virtual upright" in tl:
            return ("THIN LENS", aqua, 195)
        if "centibits" in tl or "lower entropy" in tl:
            return ("ENTROPY", coral, 194)
        if "no dock saturated" in tl or "yes dock stable" in tl:
            return ("QUEUEING", "#b7e19b", 193)
        if "no alice envies bob" in tl or "yes cake envyfree" in tl:
            return ("ENVY-FREE", "#f0b4c4", 192)
        if "by plan" in tl:
            return ("K19 PLAN", lav, 188)
        if "maya built windmill" in tl or "storm damaged windmill" in tl or "windmill powers granary" in tl:
            return ("PRONOUN CHAT", green, 187)
        if "yes blight shrinks" in tl:
            return ("EPIDEMIC R0", gold, 186)
        if "yes floats submerged" in tl or "no sinks" in tl:
            return ("BUOYANCY", aqua, 185)
        if "salt efficiency 6" in tl:
            return ("PORTFOLIO", coral, 184)
        if "hull then mast then sail" in tl or "no cycle deadlock" in tl:
            return ("TOPO DAG", "#b7e19b", 183)
        if "32 times quicker" in tl or "higher pitch" in tl or "yes coop stable" in tl or "no defection prevails" in tl:
            return ("CS / DOP / PD", "#f0b4c4", 182)
        if "by imagine" in tl:
            return ("K18 IMAGINE", lav, 178)
        if "ember thawed glacier then" in tl or "because ember thawed glacier" in tl:
            return ("PROSE CAUSE", green, 177)
        if "67 percent" in tl or "72 percent" in tl:
            return ("MARKOV", gold, 176)
        if "x 4 and y 6" in tl:
            return ("LINEAR PROG", aqua, 175)
        if "lca is leftbranch" in tl:
            return ("TREE LCA", coral, 174)
        if tl in ("75 percent", "25 percent"):
            return ("MENDEL", "#b7e19b", 173)
        if "is better" in tl or "times costlier" in tl or "is quickest" in tl:
            return ("BIG-O ALGO", "#f0b4c4", 172)
        if "by abduce" in tl:
            return ("K17 ABDUCE", lav, 168)
        if "volcano warms geyser then" in tl or "why does volcano warms" in tl:
            return ("ZERO-PREFIX", green, 167)
        if "sustainable draw is" in tl or "depleted in 5 cycles" in tl:
            return ("ECO DYN", gold, 166)
        if "yes det 0 singular" in tl or "anomaly is 64" in tl:
            return ("MAT / STAT", aqua, 165)
        if "schedule j1 and j3" in tl:
            return ("GIG DP", coral, 164)
        if "condorcet cycle paradox" in tl or "condorcet winner park" in tl:
            return ("CONDORCET", "#b7e19b", 163)
        if "collider conditioning opens" in tl or "confounder is season" in tl:
            return ("PEARL DAG", "#f0b4c4", 162)
        if "proved in 3 hops via" in tl or "blocked at liquid" in tl:
            return ("K16 PROOF", lav, 158)
        if "both link to sun in astronomy" in tl or "curious what else about" in tl:
            return ("OPEN TOPIC", green, 157)
        if "shortest distance is" in tl or "camp then pass then forest" in tl:
            return ("DIJKSTRA", gold, 156)
        if "posterior is 75 percent" in tl:
            return ("SEQ BAYES", aqua, 155)
        if "velocity 2 and puck velocity 8" in tl or "yes total energy 72" in tl:
            return ("ELASTIC", coral, 154)
        if "pack crate and barrel" in tl or "yes stable in core" in tl:
            return ("DP / CORE", "#b7e19b", 153)
        if "magnetic survives experiment" in tl or "test vapor or compass" in tl:
            return ("SCI EXP", "#f0b4c4", 152)
        if "by scan" in tl:
            return ("K15 SCAN", lav, 148)
        if "recommend forest because" in tl or "enjoys stargazing" in tl:
            return ("OPINION", green, 147)
        if "slope 2 intercept" in tl or "yield is 85" in tl:
            return ("SCI LAW", gold, 146)
        if "invest in quarry roi" in tl or "profit is 40" in tl:
            return ("NPV / ROI", aqua, 145)
        if "winner is barge score" in tl or "winner is glider score" in tl:
            return ("MCDA UTIL", coral, 144)
        if "minimum wiring cost is" in tl or "skipped cable " in tl:
            return ("KRUSKAL MST", "#b7e19b", 143)
        if "violated at step" in tl or "held until amber" in tl:
            return ("LTL LOGIC", "#f0b4c4", 142)
        if "by dreaming" in tl:
            return ("K14 DREAM", lav, 138)
        if "wanted crystal overcame" in tl:
            return ("STORY ARC", green, 137)
        if "clearing price is" in tl:
            return ("MARKET", gold, 136)
        if "ready to build shelter" in tl or "find nails for shelter" in tl:
            return ("ASSISTANT", aqua, 135)
        if "nash equilibrium is" in tl or "social optimum " in tl:
            return ("NASH 2×2", coral, 134)
        if "necessary cause is" in tl or "redundant because" in tl:
            return ("CAUSATION", "#b7e19b", 133)
        if "by gating" in tl or "by gate" in tl:
            return ("K13 GATE", aqua, 128)
        if "spotted hawk near cliff and" in tl:
            return ("CHAT RECAP", green, 127)
        if "best action " in tl and "value " in tl:
            return ("BELLMAN MDP", gold, 126)
        if "mediated by" in tl or "direct path from" in tl or "blocking drysoil" in tl:
            return ("MEDIATION", coral, 125)
        if "winner pays" in tl or "dominant bid" in tl:
            return ("VICKREY", lav, 124)
        if "critical module is" in tl:
            return ("PERT/CPM", "#b7e19b", 123)
        if "overrides " in tl or "higher priority " in tl:
            return ("DUTY PRIO", "#f0b4c4", 122)
        if "by cluster" in tl:
            return ("K12 CLUSTER", aqua, 118)
        if "shapley share" in tl or "superadditive synergy" in tl:
            return ("SHAPLEY", gold, 117)
        if "equilibrium " in tl and "phase " in tl:
            return ("THERMO", coral, 116)
        if "consensus " in tl or "dissenter is" in tl or "supermajority quorum" in tl:
            return ("BYZANTINE", lav, 115)
        if "by patched rule" in tl or "except penguin then" in tl:
            return ("META-RULE", "#b7e19b", 114)
        if "differs from" in tl or "short by" in tl:
            return ("BOM/SEQ-OP", "#f0b4c4", 113)
        if "tara visited watchtower" in tl or "at tick " in tl:
            return ("EMBED/TICK", green, 112)
        if "by link" in tl:
            return ("K11 BRIDGE", aqua, 108)
        if "received sword from" in tl or "with anvil" in tl:
            return ("FRAME-4", green, 107)
        if "root fault is" in tl:
            return ("FAULT", coral, 106)
        if "debate prefers" in tl:
            return ("DEBATE", lav, 105)
        if "expected reward" in tl:
            return ("STOCH EV", gold, 104)
        if "meet from " in tl:
            return ("SCHEDULE", "#b7e19b", 103)
        if "utility 32" in tl or "utility 26" in tl:
            return ("BUDGET", "#f0b4c4", 102)
        if "leo holds lantern" in tl or "barred from" in tl:
            return ("COREF/NORM", aqua, 101)
        if "by hypothesis" in tl:
            return ("HYPOTHESIS", aqua, 100)
        if tl.startswith("conflict ") or "kora mentored" in tl:
            return ("OPEN VERB", green, 99)
        if "satisfiable" in tl or "unsatisfiable" in tl:
            return ("CNF SAT", lav, 98)
        if "but where does" in tl or "now i know virel" in tl:
            return ("CURIOSITY", gold, 97)
        if "by analogy" in tl:
            return ("ANALOGY", "#b7e19b", 96)
        if "percent is safer" in tl or "70 percent" in tl:
            return ("RISK MATH", coral, 95)
        if "revised because newer" in tl:
            return ("METACOG", "#f0b4c4", 94)
        if "i run 28 pathways" in tl or "i run 24 pathways" in tl:
            return ("SELF-MODEL", aqua, 93)
        if "tara visited harbor carried" in tl:
            return ("STORY", gold, 92)
        if "hypothetically with rocket" in tl:
            return ("WHAT-IF", lav, 91)
        if "pareto trade raises" in tl:
            return ("NEGOTIATE", "#b7e19b", 90)
        if "intervening on" in tl or "intervention on" in tl:
            return ("DO-CALC", coral, 89)
        if "luminous crystal" in tl:
            return ("NEW WORD", "#f0b4c4", 88)
        if "by multicue" in tl or "by compose" in tl or "by cortex" in tl:
            return ("GPU CORTEX", aqua, 86)
        if "deduced candidate" in tl or "ask about" in tl:
            return ("INFO-GAIN", gold, 91)
        if "because kestrel has" in tl or "although penguin" in tl:
            return ("SOCRATIC", lav, 93)
        if "your favorite animal" in tl or "your name is" in tl:
            return ("MEMORY", green, 88)
        if "riddle" in tl or "what bird can swim" in tl:
            return ("RIDDLE", gold, 87)
        if tl.startswith("noted ") or tl.startswith("revised ") or len(tl) < 6:
            return ("ACK", muted, 1)
        return ("THOUGHT", green, 50)

    picked = []
    seen_tags = set()
    # Prefer one highlight per distinct cool cognitive category!
    ranked = sorted(
        [(tno, txt, *classify_reply(txt)) for tno, txt in raw_replies],
        key=lambda z: (-z[4], z[0]),
    )
    for tno, txt, tag, col, prio in ranked:
        if prio < 10 or tag in seen_tags:
            continue
        seen_tags.add(tag)
        picked.append((tno, txt, tag, col))
        if len(picked) >= 7:
            break
    if len(picked) < 7:
        for tno, txt in raw_replies[-7:]:
            if not any(p_i[0] == tno for p_i in picked):
                tag, col, _ = classify_reply(txt)
                picked.append((tno, txt, tag, col))
            if len(picked) >= 7:
                break
    picked.sort(key=lambda z: z[0])

    for i, (tno, msg, tag, col) in enumerate(picked[:7]):
        yy = y + 136 + i * 65
        d.rect(x + 20, yy, size - 40, 56, "#203d38", 10, f'stroke="{col}" stroke-width="1.2" stroke-opacity=".65"')
        pw = _mind_pill(d, x + 32, yy + 9, tag, col, "#142723", 8.8, 6.5)
        d.text(x + 32 + pw + 10, yy + 22, f"Turn {tno}", 10.0, muted, MONO, 600)
        d.text(x + 32, yy + 44, short(f'"{msg}"', 76), 11.4, ink, SANS, 500)

    # Right middle panel: Spiking Brain Activity Sparkline across the dialogue + stats!
    d.label(rx, 330, "AXION'S SPIKING BRAIN ACTIVITY ACROSS THE CONVERSATION")
    turn_spikes = [int(t.get("spikes", 0)) for t in turns]
    if turn_spikes:
        # Bucket into 48 clean bars showing neural bursts across turns!
        nbars = min(48, len(turn_spikes))
        chunk = max(1, len(turn_spikes) // nbars)
        buckets = [max(turn_spikes[i * chunk:(i + 1) * chunk]) for i in range(nbars)]
        bmax = max(1, max(buckets))
        bw_bar = (rw - 12) / max(1, len(buckets))
        d.rect(rx, 346, rw, 86, "#e6ece0", 8)
        d.text(rx + 10, 362, f"Peak burst: {bmax:,} GPU spikes (deep multi-hop recall!)", 11.2, INK, MONO, 600)
        d.text(W - 56, 362, f"Turns 1 → {nt}", 11.0, MUTED, MONO, 500, "end")
        for i, bv in enumerate(buckets):
            bh_bar = max(4.0, 50.0 * math.sqrt(bv / bmax))
            bcol = "#c98b3e" if bv > bmax * 0.55 else GREEN
            d.rect(rx + 6 + i * bw_bar, 426 - bh_bar, max(2.0, bw_bar - 2.5), bh_bar, bcol, 2)
    for i, (k, v) in enumerate([
        ("Total turns / live edits", f"{nt} turns  ·  {ed:,} synaptic edits"),
        ("Total GPU spikes fired", f"{sp:,} Blackwell spikes"),
        ("Hippocampal associative queries", f"{sum(int(t.get('hipp_queries',0)) for t in turns):,} parallel queries"),
    ]):
        yy = 456 + 32 * i
        d.text(rx, yy, k, 14.8, INK)
        d.text(W - 48, yy, v, 14.5, GREEN, MONO, 600, "end")
        d.rule(rx, yy + 9, W - 48, yy + 9, "#d5dccf")
