#pragma once
#include "retina.hpp"

namespace axion::life::utterance {
// This bit belongs to the serialized habitat. The runtime braiding word uses
// its position for the independently connected cortical auditory pathway.
constexpr Word stored_vocal_feedback = 1u << 21;
constexpr Word ink_port = 1u << 29, voice_port = 1u << 30, crossed_port = 1u << 31;
constexpr Word unseen = 256, depth = 7, fields = 8;
using Byte = unsigned char;
struct Trail {
    Count bits = 0;
    AX_FN Word size() const {
        return Word(bits >> 56);
    }
    AX_FN Word at(Word i) const {
        return Word(bits >> (8 * (size() - i - 1))) & 255u;
    }
    AX_FN void append(Word image) {
        if (image < unseen) {
            Word n = size();
            bits = (Count(n < depth ? n + 1 : depth) << 56) | (((bits << 8) | image) & 0x00ffffffffffffffull);
        }
    }
    AX_FN bool valid() const {
        return size() <= depth && !(bits & 0x00ffffffffffffffull & (~Count(0) << (8 * size())));
    }
};
static_assert(sizeof(Trail) == 8);
AX_FN Word impression(retina::Light local, retina::Genes nerve) {
    Word begin = retina::contrast(local.rest, local.onset, nerve.receptors);
    Word end = retina::contrast(local.rest, local.end, nerve.receptors >> 2);
    if (__builtin_popcount(begin) != 1 || __builtin_popcount(end) != 1)
        return unseen;
    Word a = __builtin_ctz(begin), b = __builtin_ctz(end);
    return a | (b << 2) | (braid::pigment(local.onset, a) << 4) | (braid::pigment(local.end, b) << 6);
}
// Four local cell populations, two offsets and two conjunctions. All are
// available at once. A heard word arrives without an address on this retina.
AX_FN Word texture(Word image) {
    Word a = image & 3, b = (image >> 2) & 3, c = (image >> 4) & 3, d = (image >> 6) & 3;
    return image | (((b - a) & 3) << 8) | (((d - c) & 3) << 10) | ((a ^ c) << 12) | ((b ^ d) << 14);
}
AX_FN Word channel(Word texture, Word circuit) {
    return (texture >> (2 * circuit)) & 3;
}
AX_FN Word swap_ink(Word palette, Word a, Word b) {
    Word diff = braid::pigment(palette, a) ^ braid::pigment(palette, b);
    return palette ^ (diff << (2 * a)) ^ (diff << (2 * b));
}
struct Passage {
    Word after = 0, entering = 0, leaving = 0, wick = 0, stained = 0;
};
AX_FN Passage passage(Word palette, Word from, Word to, Word fibres, Word wet) {
    Word wick = (from + 1) & 3;
    if (wick == to)
        wick = (from + 3) & 3;
    bool touch = from != to && ((fibres & wet) >> wick & 1u);
    Word loosened = touch ? swap_ink(palette, from, wick) : palette;
    return {swap_ink(loosened, from, to), braid::pigment(palette, from), braid::pigment(loosened, from), wick,
            Word(touch)};
}
// Pigments stay attached to four carriers. A wet wick can exchange a pigment
// before the moving carrier takes the downstream seat. Both swaps conserve dye.
AX_FN retina::Light flash(const Passage &p, Word ground, Word from, Word to) {
    return {ground, drift::overlay(ground, from, p.entering), drift::overlay(ground, to, p.leaving)};
}
struct Genes {
    Word dendrites = 0, motor = 0;
};
struct Song {
    Word calls = 0, n = 0;
};
// A developing resonator receives one of eight retinal discharges at each
// motor phase. The germ controls taps, articulatory register, mixing and arrest.
AX_FN Song call(Genes g, Word image) {
    Song s{};
    if (image >= unseen)
        return s;
    Word input = texture(image), extent = 1 + ((g.motor >> 24) & 7);
    for (Word i = 0; i < extent; ++i) {
        Word tap = (g.dendrites >> (4 * i)) & 15, route = tap & 7;
        Word value = channel(input, route);
        if (tap & 8)
            value ^= channel(input, (g.motor >> (3 * i)) & 7);
        Word reg = (g.motor >> (3 * i)) & 3;
        Word motor = (reg << 2) | ((value + ((g.motor >> (27 + (i & 3))) & 1)) & 3);
        if (i && (g.motor >> 31) && motor == ((g.dendrites >> 27) & 15))
            break;
        s.calls |= motor << (4 * s.n++);
    }
    return s;
}
// A resonator's register and time of arrival select one of eight afferent
// bands. Its pitch selects sixteen plastic contacts in that band. The same
// 64 bytes also serve ordinary hearing in a watershed resident.
struct Receptors {
    Word low[8]{}, high[8]{};
};
AX_FN Word spike_map(Word image) {
    Word pattern = 0;
    for (Word j = 0; j < 4; ++j)
        pattern |= 1u << (4 * j + ((image >> (2 * j)) & 3));
    return pattern;
}
template <class Ear> AX_FN Word associate(Ear &ear, Song song, Word image, Word law) {
    if (image == unseen)
        return 0;
    Word changes = 0, positive = spike_map(image);
    for (Word t = 0; t < song.n; ++t) {
        Word word = (song.calls >> (4 * t)) & 15, band = (t + (word >> 2)) & 7, pitch = word & 3,
             shift = (pitch & 1) * 16;
        auto &bank = pitch < 2 ? ear.low[band] : ear.high[band];
        Word old = (bank >> shift) & 65535, next = braid::rule_bits(law, old, positive) & 65535;
        bank = (bank & ~(65535u << shift)) | (next << shift);
        changes += __builtin_popcount(old ^ next);
    }
    return changes;
}
template <class Ear> AX_FN Word evoke(const Ear &ear, Song song, bool reverse) {
    Word votes = 0;
    for (Word t = 0; t < song.n; ++t) {
        Word word = (song.calls >> (4 * (reverse ? song.n - 1 - t : t))) & 15, band = (t + (word >> 2)) & 7,
             pitch = word & 3;
        Word cells = ((pitch < 2 ? ear.low[band] : ear.high[band]) >> (16 * (pitch & 1))) & 65535;
        for (Word j = 0; j < 16; j += 4) {
            Word row = (cells >> j) & 15;
            if (row && !(row & (row - 1)))
                votes |= row << j;
        }
    }
    return votes;
}
AX_FN Word located(Word votes, Word lane) {
    Word row = (votes >> (4 * lane)) & 15;
    return row && !(row & (row - 1)) ? Word(__builtin_ctz(row)) : 4u;
}
// Motor phases can receive collaterals from the animal's own learned ear.
// A phase can favor a previously heard pitch when its visual contacts
// discharge specifically for the current event. Newborn silence costs no storage.
AX_FN int resonance(Word cells, Word picture) {
    Word hits = cells & picture, off = cells & ~picture;
    hits = (hits | (hits >> 1) | (hits >> 2) | (hits >> 3)) & 0x1111u;
    off = (off | (off >> 1) | (off >> 2) | (off >> 3)) & 0x1111u;
    return 4 * __builtin_popcount(hits & ~off) + __builtin_popcount(hits) - 2 * __builtin_popcount(off & ~hits);
}
template <class Ear> AX_FN Word ear_cells(const Ear &e, Word phase, Word note) {
    Word band = (phase + (note >> 2)) & 7, pitch = note & 3;
    return ((pitch < 2 ? e.low[band] : e.high[band]) >> (16 * (pitch & 1))) & 65535;
}
template <class Ear> AX_FN Song imitate(const Ear &ear, Word axons, Song own, Word image) {
    if (image >= unseen || !own.n)
        return own;
    Word visual = spike_map(image);
    for (Word phase = 0; phase < own.n; ++phase) {
        if (!(axons & (1u << (16 + phase))))
            continue;
        Word original = (own.calls >> (4 * phase)) & 15, winner = original;
        int start = resonance(ear_cells(ear, phase, original), visual), best = start;
        Word reach = (axons & (1u << (24 + phase))) ? 8u : 4u;
        for (Word j = 1; j < reach; ++j) {
            Word reg = (original & 12) ^ ((j & 4) ? 4u : 0u);
            Word candidate = reg | ((original + j) & 3);
            int value = resonance(ear_cells(ear, phase, candidate), visual);
            if (value > best) {
                best = value;
                winner = candidate;
            }
        }
        if (best >= 1 + int((axons >> (2 * phase)) & 3) && winner != original)
            own.calls ^= (original ^ winner) << (4 * phase);
    }
    return own;
}
AX_FN Word changed_pitches(Song a, Song b) {
    Word change = a.calls ^ b.calls;
    return __builtin_popcount((change | (change >> 1) | (change >> 2) | (change >> 3)) & 0x11111111u);
}
struct Ledger {
    Count releases = 0, moves = 0, stained = 0, exchanges = 0, seen = 0, omitted = 0, full = 0, calls = 0,
          syllables = 0, listeners = 0, spike_changes = 0, vocal_heat = 0, ear_heat = 0;
    Count borrowed = 0, retuned = 0;
    Count acoustic_edits = 0, strikes = 0, blind_strikes = 0, intercepted = 0, blind_food = 0, altered = 0,
          reaching_heat = 0;
};
// Body turning adds its heading to both retinal position addresses. Subtract
// that measured turn locally before storing a view in the long-lived circuit.
AX_FN Word stabilize(Word image, Word heading) {
    if (image >= unseen)
        return unseen;
    return (image & 240u) | (((image & 3) - heading) & 3) | (((((image >> 2) & 3) - heading) & 3) << 2);
}
struct Watch {
    Word slot = ~0u, start = 0, ticks = 0, anchored = 0;
};
struct Sample {
    Word tick = 0, tile = 0, slot = 0, born = 0, actor = 0, from = 0, to = 0, ground = 0, palette = 0, after = 0,
         wet = 0, fibres = 0;
    Word places = 0, hues = 0, receptors = 0, channel = 0, light = 0, image = unseen, source_before = 0,
         source_after = 0, coat = 0;
    Count previous = 0, trail = 0;
    Word dendrites = 0, motor = 0, sequence = 0, words = 0, paid = 0, heading = 0, anchored = 0, echo = 0,
         controller = 0, natal = 0;
    Receptors ear{};
};
} // namespace axion::life::utterance
