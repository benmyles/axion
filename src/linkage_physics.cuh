#pragma once
#include "axion/linkage.hpp"
#include "axion/material.hpp"
namespace axion::life::linkage {
AX_FN Word permute(Word key) {
    Word deck = 0xe4;
    for (Word j = 3; j > 0; --j) {
        Word k = mix(key + 821 * j) % (j + 1), left = bits(deck, j), right = bits(deck, k);
        deck = replace(replace(deck, j, right), k, left);
    }
    return deck;
}
AX_FN Material matter(Word world, Word glyph) {
    Word k = mix(world + 3607 * glyph + 3529);
    return {permute(mix(k + 1447)), 1u + mix(k + 6931) % 14, (k >> 17) & 1, (k >> 21) & 3};
}
AX_FN Word thread(Material m, Word underlying) { return 1 + ((m.mesh ^ (underlying >> 1)) & 1); }
AX_FN Word edge_a(Word q) {
    constexpr Word a[]{0, 0, 0, 1, 1, 2};
    return a[q];
}
AX_FN Word edge_b(Word q) {
    constexpr Word b[]{1, 2, 3, 2, 3, 3};
    return b[q];
}
AX_FN Word transmission(Material m, Word form, Word exposed, Word input) {
    if (!form)
        return 0;
    // A full mesh carries both strokes. A cut belt transmits only at its
    // exposed notches. Contact type determines which way the follower yields.
    if (form == 2 && !(m.notches & (1u << exposed)))
        return 0;
    return 1 + (input ^ ((m.mesh >> (form - 1)) & 1));
}
AX_FN Word pigment(Material m, Word angle) { return bits(m.dye, angle); }
AX_FN Word expose(Rig r, Word angle, const Material *law) {
    Word colors = 0;
    for (Word i = 0; i < 4; ++i)
        if (r.parts & (1u << i))
            colors = replace(colors, i, pigment(law[bits(r.glyphs, i)], bits(angle, i)));
    return colors;
}
AX_FN Word turn_one(Word arg, Word i, Word force, const Material *law, Word glyphs) {
    Word direction = (force ^ law[bits(glyphs, i)].reverse) ? 3u : 1u;
    return replace(arg, i, (bits(arg, i) + direction) & 3);
}
AX_FN Word press(Rig r, Word angle, Word motor, const Material *law) {
    Word i = motor >> 1, force = motor & 1;
    Material material = law[bits(r.glyphs, i)];
    Word exposed = pigment(material, bits(angle, i));
    Word after = turn_one(angle, i, force, law, r.glyphs);
    for (Word j = 0; j < 4; ++j) {
        Word form = joint(r.joints, i, j);
        if (!form)
            continue;
        Word impulse = transmission(material, form, exposed, force);
        if (impulse)
            after = turn_one(after, j, impulse - 1, law, r.glyphs);
    }
    return after;
}
AX_FN Rig layout(Word seed, Word width, bool covered) {
    Rig r{};
    r.parts = (1u << width) - 1;
    Word scramble = permute(mix(seed + 239)), rot = mix(seed + 8527);
    // Repeated composition and repeated glyphs coexist in the same new rack.
    for (Word j = 0; j < width; ++j)
        r.glyphs = replace(r.glyphs, j, (rot & 1) ? bits(scramble, j) : mix(seed + 71 * j + 449) & 3);
    for (Word j = 1; j < width; ++j) {
        Word k = mix(seed + 2857 * j);
        join(r, j, k % j, 1 + ((k >> 13) & 1));
    }
    if (width == 4 && (rot & 4)) {
        Word a = rot >> 4 & 3, b = rot >> 6 & 3;
        if (a != b && !joint(r.joints, a, b))
            join(r, a, b, 1 + ((rot >> 12) & 1));
    }
    r.handles = (1u << (2 * width)) - 1;
    if (covered) {
        Word one = (rot >> 17) % width;
        r.handles &= ~(3u << (2 * one));
        if (width >= 3 && (rot & 8))
            r.handles &= ~(1u << ((rot >> 22) % (2 * width)));
    }
    return r;
}
AX_FN Word bump(Word allowed, Word k) {
    Word n = __builtin_popcount(allowed);
    if (!n)
        return no_path;
    Word choice = k % n;
    while (choice--)
        allowed &= allowed - 1;
    return __builtin_ctz(allowed);
}
} // namespace axion::life::linkage
