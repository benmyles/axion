#pragma once
#include "braid_episode.hpp"

namespace axion::life::braid {

// A viewing instrument independently connects four retinal positions and four pigment bands.
struct Optics {
    unsigned char places = order, hues = order;
};
constexpr Word individual_eyes = 131072;
AX_FN Optics eye_wiring(Word mode, Genes genes, Word heading) {
    if (!(mode & individual_eyes))
        return {};
    Word frame = ((genes.muscle >> 20) & 4) | ((heading + (genes.muscle >> 24)) & 3), p = 0;
    for (Word i = 0; i < 4; ++i)
        p |= face(frame, i) << (2 * i);
    return {static_cast<unsigned char>(p), static_cast<unsigned char>(assembly((genes.throat >> 12) % 24))};
}
AX_FN Word inverse_view(Word permutation) {
    Word result = 0;
    for (Word source = 0; source < 4; ++source)
        result |= source << (2 * pigment(permutation, source));
    return result;
}
AX_FN Word project_view(Word scene, Optics glass) {
    if (glass.places == order && glass.hues == order)
        return scene;
    return transfer(transfer(glass.hues, scene), inverse_view(glass.places));
}
AX_FN Episode through_glass(const Episode &e, Optics glass) {
    if (glass.places == order && glass.hues == order)
        return e;
    Episode out{};
    out.stamp = e.stamp;
    for (Word frame = 0; frame < 4; ++frame) {
        Word seen = 0, board = 0;
        for (Word s = 0; s < 4; ++s)
            if (visible(e, frame) & (1u << s)) {
                Word dest = pigment(glass.places, s);
                seen |= 1u << dest;
                board |= pigment(glass.hues, pigment(picture(e, frame), s)) << (2 * dest);
            }
        expose(out, frame, board, seen);
    }
    return out;
}

constexpr Word arrangements = (1u << 24) - 1;
struct Correspondence {
    Count voice = 0;
    Word excluded = 0, panel = 0;
};
AX_FN Word seen_ports(const Correspondence &c) { return (c.panel >> 16) & 15; }
AX_FN bool anchored(const Correspondence &c) { return seen_ports(c) == 15; }
AX_FN Word candidates(const Correspondence &c) { return anchored(c) ? (arrangements ^ c.excluded) : 0; }
AX_FN bool sound_contact(const Correspondence &c) {
    if (!c.voice)
        return !c.panel && !c.excluded;
    if (c.excluded & ~arrangements || (c.panel >> 20) || (!anchored(c) && c.excluded))
        return false;
    Word known = seen_ports(c), source = c.panel & 255, other = (c.panel >> 8) & 255;
    return (!known || bijective(source)) && (!anchored(c) || bijective(other));
}
// If a full arrangement is known at both eyes, a candidate socket map P
// determines the hue map: H = other ∘ P ∘ inverse(source).
AX_FN Optics candidate_view(const Correspondence &c, Word rank) {
    Word p = assembly(rank), inv = c.panel & 255, other = (c.panel >> 8) & 255;
    return {static_cast<unsigned char>(p), static_cast<unsigned char>(transfer(other, transfer(p, inv)))};
}
AX_FN Word change_reference(Optics glass, Word ref, bool reverse) {
    if (ref >= 8)
        return none;
    Word map = ref < 4 ? glass.places : glass.hues;
    return (ref & 4) + pigment(reverse ? inverse_view(map) : map, ref & 3);
}
AX_FN Word agreed_reference(const Correspondence &c, Word ref, bool reverse) {
    Word left = candidates(c), value = none;
    while (left) {
        Word rank = __builtin_ctz(left);
        left &= left - 1;
        Word next = change_reference(candidate_view(c, rank), ref, reverse);
        if (value != none && value != next)
            return none;
        value = next;
    }
    return value;
}
AX_FN Word view_reply(const Correspondence &c, Word rank, Word scene, Word their_socket) {
    auto lens = candidate_view(c, rank);
    Word here = pigment(inverse_view(lens.places), their_socket);
    return pigment(lens.hues, pigment(scene, here));
}
AX_FN Word disputed(const Correspondence &c, Word scene, Word socket, Word hue, Word law) {
    Word bad = 0, depends = law ^ (law >> 1);
    Word sensitive = ((depends & 1) ? (arrangements ^ c.excluded) : 0) | ((depends & 4) ? c.excluded : 0);
    // The update truth table can re-open an inhibited connection. Supply the
    // genuine sensory evidence for all hypotheses, including old exclusions.
    while (sensitive) {
        Word k = __builtin_ctz(sensitive);
        sensitive &= sensitive - 1;
        bad |= Word(view_reply(c, k, scene, socket) != hue) << k;
    }
    return bad;
}
AX_FN Word predicted_separation(const Correspondence &c, Word scene, Word socket) {
    Word left = candidates(c), counts[4]{}, n = __builtin_popcount(left);
    while (left) {
        Word k = __builtin_ctz(left);
        left &= left - 1;
        ++counts[view_reply(c, k, scene, socket)];
    }
    Word separated = n * n;
    for (Word k = 0; k < 4; ++k)
        separated -= counts[k] * counts[k];
    return separated;
}
AX_FN Word sight_to_request(const Correspondence &c, Word scene, Word paid, Word motor, bool selective) {
    Word picked = none, best = 0;
    Word already = paid;
    if (!anchored(c) && seen_ports(c) && (c.panel & 255) == inverse_view(scene))
        already |= seen_ports(c);
    for (Word j = 0; j < 4; ++j) {
        Word slot = (motor + j) & 3;
        if (already & (1u << slot))
            continue;
        Word gain = selective ? predicted_separation(c, scene, slot) : 0;
        if (picked == none || gain > best) {
            best = gain;
            picked = slot;
        }
    }
    return picked;
}
AX_FN Word couple_sight(Correspondence &c, Genes genes, Word scene, Word socket, Word hue, bool revise = true) {
    if (!(genes.arbor & (1u << 14)) || socket >= 4 || hue >= 4)
        return 0;
    Word writes = 0;
    if (anchored(c)) {
        Word before = c.excluded;
        Word law = genes.plastic >> 8;
        c.excluded = rule_bits(law, before, disputed(c, scene, socket, hue, law)) & arrangements;
        writes = __builtin_popcount(before ^ c.excluded);
        if (c.excluded != arrangements || !revise || !(genes.arbor & 8))
            return writes;
        c.panel = 0;
        c.excluded = 0;
    }
    if (!bijective(scene))
        return writes;
    Word inv = inverse_view(scene), marks = seen_ports(c), reported = (c.panel >> 8) & 255;
    if ((c.panel & 255) != inv) {
        marks = reported = 0;
    }
    marks |= 1u << socket;
    reported = (reported & ~(3u << (2 * socket))) | (hue << (2 * socket));
    if (marks == 15 && !bijective(reported)) {
        c.panel = 0;
        return writes;
    }
    c.panel = inv | (reported << 8) | (marks << 16);
    return writes;
}
} // namespace axion::life::braid
