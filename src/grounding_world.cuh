#pragma once
#include "grounding_neural.cuh"

namespace axion::life::tabletop {
using namespace grounding;
struct Convention {
    Vocabulary voice{};
    Byte pins[3]{}, order[3]{}, verb[6]{};
    Word flip = 0, key = 0;
};
AX_FN Word draw(Word &key) {
    key = mix(key + 0x9e3779b9u);
    return key;
}
AX_FN Convention culture(const Vocabulary &v, Word key, Word style) {
    Convention c;
    c.voice = v;
    c.key = key;
    Word rng = key;
    Byte channels[8]{0, 1, 2, 3, 4, 5, 6, 7};
    for (Word j = 7; j && style; --j) {
        Word k = draw(rng) % (j + 1);
        Byte b = channels[j];
        channels[j] = channels[k];
        channels[k] = b;
    }
    for (Word j = 0; j < 3; ++j) {
        c.pins[j] = channels[j];
        c.order[j] = static_cast<Byte>(j);
    }
    c.flip = style ? draw(rng) & 255 : 0;
    Byte stock[12]{0, 2, 14, 16, 18, 22, 1, 3, 15, 17, 19, 23};
    if (style) {
        for (Word j = 11; j; --j) {
            Word k = draw(rng) % (j + 1);
            Byte b = stock[j];
            stock[j] = stock[k];
            stock[k] = b;
        }
        for (Word j = 2; j; --j) {
            Word k = draw(rng) % (j + 1);
            auto b = c.order[j];
            c.order[j] = c.order[k];
            c.order[k] = b;
        }
        // Acoustic names have no common spelling or type marker across cultures.
        auto nonce = [&]() {
            Count a = draw(rng), b = draw(rng);
            return (a << 32) | b | 1;
        };
        for (auto &w : c.voice.noun)
            w = nonce();
        for (auto &w : c.voice.verb)
            w = nonce();
        for (auto &w : c.voice.gap)
            w = nonce();
        for (auto &w : c.voice.particle)
            w = nonce();
    }
    for (Word j = 0; j < 6; ++j)
        c.verb[j] = stock[j];
    return c;
}
AX_FN Word identity(const Convention &c, Word light) {
    Word k = light ^ c.flip;
    return ((k >> c.pins[0]) & 1) | (((k >> c.pins[1]) & 1) << 1) | (((k >> c.pins[2]) & 1) << 2);
}
struct Room {
    View film{};
    Scene truth{};
};
AX_FN Room room(Word key, Word count, Word palette) {
    Room r;
    for (Word k = 0; k < count; ++k) {
        Word prior = draw(key) & 63;
        while (r.film.before.sites & (1ull << prior))
            prior = (prior + 1) & 63;
        Word move = draw(key), x = (prior & 7) + ((move & 7) % 5) + 6, y = (prior >> 3) + (((move >> 3) & 7) % 5) + 6;
        Word site = (x & 7) + 8 * (y & 7);
        while (r.film.after.sites & (1ull << site))
            site = (site + 1) & 63;
        // Unlabelled bodies vary in the other visible channels even when
        // their spoken category is the same. Preserve identity for the two frames.
        Word light = draw(key) & 255;
        bool conflict;
        do {
            conflict = Word(__builtin_popcount(light) & 1) != palette;
            for (Word h = 0; h < k; ++h)
                conflict |= r.truth.patch[h].light == light;
            if (conflict)
                light = (light + 1) & 255;
        } while (conflict);
        illuminate(r.film.before, prior, light);
        illuminate(r.film.after, site, light);
        r.truth.patch[r.truth.n++] = {static_cast<Byte>(light), static_cast<Byte>(prior), static_cast<Byte>(site)};
    }
    return r;
}
struct Fact {
    Byte left = 0, verb = 0, right = 0;
    Word ok = 0;
};
AX_FN bool novel(const Convention &c, Fact a) {
    Word small = a.left < a.right ? a.left : a.right, large = a.left > a.right ? a.left : a.right;
    return mix(c.key ^ ((small * 6 + a.verb) * 8 + large) * 0x84ca99d9u) % 5 == 0;
}
AX_FN Voice utter(const Convention &c, Fact f) {
    Voice v;
    Count waves[]{c.voice.noun[f.left], c.voice.verb[f.verb], c.voice.noun[f.right]};
    v.n = 3;
    for (Word j = 0; j < 3; ++j)
        v.waves[j] = waves[c.order[j]];
    return v;
}
AX_FN Fact describe(const Convention &c, const Scene &s, Word key, bool heldout) {
    Fact none;
    Word choices = s.n * s.n * 6;
    for (Word k = 0; k < choices; ++k) {
        Word z = (key % choices + k) % choices, r = z % 6, i = z / 6 / s.n, j = z / 6 % s.n;
        if (i == j)
            continue;
        auto a = s.patch[i], b = s.patch[j];
        Fact named{static_cast<Byte>(identity(c, a.light)), static_cast<Byte>(r),
                   static_cast<Byte>(identity(c, b.light)), 1};
        if (novel(c, named) != heldout || !((relative(a, b).fired >> c.verb[r]) & 1))
            continue;
        return named;
    }
    return none;
}
AX_FN Word oracle(const Convention &c, const Scene &s, Fact f) {
    for (Word i = 0; i < s.n; ++i)
        if (identity(c, s.patch[i].light) == f.left)
            for (Word j = 0; j < s.n; ++j)
                if (i != j && identity(c, s.patch[j].light) == f.right &&
                    ((relative(s.patch[i], s.patch[j]).fired >> c.verb[f.verb]) & 1))
                    return 1;
    return 0;
}
} // namespace axion::life::tabletop
