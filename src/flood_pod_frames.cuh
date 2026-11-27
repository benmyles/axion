#pragma once
namespace axion::life::pods {
AX_FN Word rotation(Word old, Word angle) {
    Word table = 0;
    for (Word i = 0; i < 8; ++i)
        table |= ((((old >> (2 * i)) & 3) + angle) & 3) << (2 * i);
    return table;
}
AX_FN FrameGenes frame_founder(Word key) {
    FrameGenes g;
    g.learning = mix(key + 28871);
    g.consolidation = mix(key + 6607);
    for (Word f = 1; f < 4; ++f) {
        Word p = mix(key + 4771 * f), row = 0;
        for (Word a = 0; a < 4; ++a) {
            Word output = (p + (p & 4 ? 0u - a : a)) & 3;
            row |= output << (2 * a);
        }
        // The initial crossbars respect the four-muscle ring. Duplications,
        // rewiring and individual contact mutations can leave that family.
        g.routes |= row << (8 * f);
    }
    return g;
}
AX_FN FrameGenes frame_reference() {
    FrameGenes g;
    g.learning = 0x7bd1u | (3u << 16) | (6u << 20);
    for (Word a = 0; a < 4; ++a) {
        for (Word f = 1; f < 4; ++f)
            g.routes |= ((a + f) & 3) << (8 * f + 2 * a);
        g.consolidation |= (a ? a - 1 : 0) << (2 * a);
        g.consolidation |= lower(3, a + 1) << (8 + 2 * a);
    }
    g.consolidation |= (1u << 16) | (1u << 22);
    return g;
}
AX_FN FrameGenes frame_child(FrameGenes g, Word key, Word rate) {
    Word *word[]{&g.routes, &g.learning, &g.consolidation};
    for (Word i = 0; i < 3; ++i) {
        Word r = mix(key + 1291 * i);
        if ((r & 255) < rate)
            *word[i] ^= 1u << ((r >> 9) & 31);
    }
    Word cut = mix(key + 9091);
    if ((cut & 255) < 3) {
        Word target = 1 + (cut >> 8) % 3, source = (cut >> 15) & 3;
        Word row = source ? (g.routes >> (8 * source)) & 255 : 0xe4;
        g.routes = (g.routes & ~(255u << (8 * target))) | (row << (8 * target));
    }
    return g;
}
AX_FN FrameGenes recruit(FrameGenes arm, const Genes &ancestral, Word key) {
    Word q = mix(key + 1217);
    // A tissue duplication can reuse a parent's working synaptic law in
    // the consolidated map or in the new population of inhibitory relays.
    if ((q & 255) < 6)
        arm.consolidation = (arm.consolidation & 0xffff0000u) | (ancestral.plastic & 65535);
    if (((q >> 8) & 255) < 6)
        arm.learning = (arm.learning & 0xffff0000u) | ((ancestral.timing >> 8) & 65535);
    return arm;
}
AX_FN Word route(const FrameGenes &g, Word f, Word a) { return f ? (g.routes >> (8 * f + 2 * a)) & 3 : a; }
AX_FN bool settled(const Frame &state, const FrameGenes &g) {
    return Word(__builtin_popcount(state.known)) >= 2 + ((g.learning >> 20) & 7);
}
struct MotorPulse {
    Word cue = 0, native = 0, physical = 0, favourite = 0, predictions = 0;
    bool distinct = false, anchored = false;
};
AX_FN Word reversible(Word real, const FrameGenes &g, Word frame, Word fallback) {
    if (!frame || route(g, frame, fallback) == real)
        return !frame ? real : fallback;
    for (Word k = 0; k < 4; ++k)
        if (route(g, frame, k) == real)
            return k;
    return real;
}
AX_FN MotorPulse pulse(const Circuit &b, const Genes &g, const Frame &state, const FrameGenes &wire, Word light,
                       Word basal, Word key, bool framed, Word forced = 4, Word clamp = 4) {
    MotorPulse p;
    p.cue = receptor(light, g);
    if (!framed) {
        p.native = p.physical = forced < 4 ? forced : choose(b, g, light, basal, key);
        return p;
    }
    Word firing = volleys(b, g, p.cue), start = mix(key) & 3;
    bool explore = explores(g, key);
    Word initial = explore ? start : from_tie(winners(firing, (g.readout >> 16) & 3, basal), start);
    Word high = 0, second = 0;
    for (Word a = 0; a < 4; ++a) {
        Word current = (firing >> (8 * a)) & 255;
        if (current > high) {
            second = high;
            high = current;
            p.favourite = a;
        } else if (current > second)
            second = current;
    }
    p.distinct = high >= second + 1 + ((wire.consolidation >> 23) & 3);
    bool known = state.known & (1u << p.cue);
    p.anchored = settled(state, wire) && known;
    Word tag = known ? (state.anchors >> (2 * p.cue)) & 3 : p.favourite;
    Word frame = clamp < 4 ? clamp : p.anchored ? state.active : 0;
    Word outgoing = p.anchored && !explore ? tag : initial;
    p.physical = forced < 4 ? forced : route(wire, frame, outgoing);
    p.native = forced < 4 ? reversible(forced, wire, frame, outgoing) : outgoing;
    for (Word f = 0; f < 4; ++f)
        if (route(wire, f, tag) == p.physical)
            p.predictions |= 1u << f;
    return p;
}
AX_FN void perceive(Frame &state, const FrameGenes &g, const MotorPulse &p, bool grasp) {
    if ((!settled(state, g) || !(state.known & (1u << p.cue)) || ((g.consolidation & (1u << 22)) && !state.active)) &&
        p.distinct) {
        Word shift = 2 * p.cue, certainty = (state.certainty >> shift) & 3, prior = (state.anchors >> shift) & 3;
        bool agrees = prior == p.favourite;
        if (!agrees && ((g.consolidation >> (16 + certainty)) & 1))
            state.anchors = (state.anchors & ~(3u << shift)) | (p.favourite << shift);
        Word strength = (g.consolidation >> (2 * certainty + 8 * Word(agrees))) & 3;
        state.certainty = (state.certainty & ~(3u << shift)) | (strength << shift);
        if (strength >= 2 + ((g.consolidation >> 21) & 1) && agrees)
            state.known |= static_cast<Byte>(1u << p.cue);
        else if (!agrees)
            state.known &= static_cast<Byte>(~(1u << p.cue));
    }
    state.signatures = (state.signatures << 4) | p.predictions;
    state.reliable = static_cast<Byte>((state.reliable << 1) | Word(p.anchored && grasp));
}
AX_FN void reframe(Frame &state, const FrameGenes &law, const Genes &g, const Circuit &b, bool nourishing, Word salt,
                   Word displaced) {
    if (!state.reliable)
        return;
    int best = -256;
    Word candidates = 0;
    for (Word t = 0; t < 8; ++t) {
        Word lag = displaced ? (t + 1 + mix(salt) % 7) & 7 : t;
        if (!(g.timing & (1u << t)) || !(state.reliable & (1u << lag)))
            continue;
        Word address = (b.history >> (8 * lag)) & 31;
        int strength = b.support[group(address >> 2, g)][t];
        if (strength > best) {
            best = strength;
            candidates = 0;
        }
        if (strength == best)
            candidates |= 1u << lag;
    }
    if (!candidates)
        return;
    Word first = __builtin_ctz(candidates), predicted = (state.signatures >> (4 * first)) & 15;
    if (law.learning & (1u << 24)) {
        for (Word t = first + 1; t < 8; ++t)
            if (candidates & (1u << t))
                predicted |= (state.signatures >> (4 * t)) & 15;
    }
    int limit = 2 + int((law.learning >> 16) & 15), peak = -128, values[4]{};
    for (Word f = 0; f < 4; ++f) {
        Word bit = (predicted >> f) & 1, code = Word(nourishing) * 2 + bit;
        int delta = int((law.learning >> (4 * code)) & 15);
        if (delta >= 8)
            delta -= 16;
        values[f] = int(state.evidence[f]) + delta;
        if (values[f] > peak)
            peak = values[f];
    }
    for (Word f = 0; f < 4; ++f)
        state.evidence[f] = static_cast<signed char>(values[f] - peak < -limit ? -limit : values[f] - peak);
    Word winner = state.active;
    for (Word f = 0; f < 4; ++f)
        if (state.evidence[f] > state.evidence[winner] + int((law.learning >> 27) & 1))
            winner = f;
    state.switches += winner != state.active;
    state.active = static_cast<Byte>(winner);
    if (nourishing && !predicted)
        state.unexplained = static_cast<Byte>(lower(255, Word(state.unexplained) + 1));
    else if (nourishing)
        state.unexplained = 0;
    if ((law.learning & (1u << 30)) && state.unexplained >= 2 + ((law.learning >> 25) & 3)) {
        state.active = 0;
        state.known = 0;
        state.certainty = 0;
        state.unexplained = 0;
    }
}
} // namespace axion::life::pods
