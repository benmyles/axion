#include "axion/utterance.hpp"

AX_FN void ink_exchange(World w, const DriftSquare &sites, Word palette, Word source, Word to, utterance::Passage &p,
                        Word &wet, Word &fibre) {
    for (Word j = 0; j < 4; ++j) {
        const auto &s = w.land[sites[j]];
        wet |= Word(bool(s.water)) << j;
        fibre |= Word(bool(s.fibre_east || s.fibre_south)) << j;
    }
    p = utterance::passage(palette, source, to, fibre, wet);
    if (p.after != palette)
        for (Word j = 0; j < 4; ++j) {
            auto &seat = w.braids.floats[sites[j]];
            seat = (seat & ~3u) | braid::pigment(p.after, j);
        }
}
AX_FN void ink_voices(World w, Configuration c, const DriftSquare &sites, Word owner, utterance::Song song,
                      Word visible, Word images, Word outlet, Word hue, Word moved, Word tie, utterance::Ledger &q) {
    if (!song.n)
        return;
    auto &caller = w.animals[owner];
    if (!caller.body.pores || caller.energy <= song.n)
        return;
    caller.energy -= song.n;
    caller.worked += song.n;
    q.calls = 1;
    q.syllables = q.vocal_heat = song.n;
    caller.signal = 1 + (song.calls & 7);
    Word available = moved ? drift::low(8u, Word(w.land[sites[outlet]].litter)) : 0u;
    for (Word i = 0; i < 4; ++i) {
        Word j = (i + tie) & 3;
        Word slot = w.slots[sites[j]];
        if (slot == owner)
            continue;
        auto &a = w.animals[slot];
        auto &g = w.dna[slot];
        if (a.energy <= song.n || !(c.wire & 3) || !(g.voice & (1u << (24 + (a.action & 7)))))
            continue;
        Word band = motor_band(w, c, slot);
        auto t = read_band(w, c.tiles, slot, band);
        Word old = t.spike;
        auto &ear = w.ears[slot];
        bool crossed = bool(w.braids.mode & utterance::crossed_port);
        Word votes = utterance::evoke(ear, song, crossed), aim = utterance::located(votes, 1),
             filter = utterance::located(votes, 3);
        Word alt = utterance::evoke(ear, song, !crossed);
        q.altered += aim != utterance::located(alt, 1) || filter != utterance::located(alt, 3);
        bool observed = visible & (1u << j);
        if (aim < 4 && filter < 4 && a.body.limbs && a.body.pores && a.energy > song.n + 1) {
            --a.energy;
            ++a.worked;
            ++q.reaching_heat;
            ++q.strikes;
            q.blind_strikes += !observed;
            auto glass = braid::eyes_of(w, slot);
            if (aim == braid::pigment(glass.places, outlet) && filter == braid::pigment(glass.hues, hue)) {
                Word bite = drift::low(available, drift::low(1u + nbits(a.body.limbs), 65535u - a.energy));
                available -= bite;
                w.land[sites[outlet]].litter -= bite;
                a.energy += bite;
                a.meals += bite;
                a.diet[2] += bite;
                q.intercepted += bite;
                if (!observed)
                    q.blind_food += bite;
            }
        }
        for (Word k = 0; k < song.n; ++k) {
            Word raw = (song.calls >> (4 * k)) & 15, id = raw & 7;
            Word afferent = rotate(0x11111111u << (raw >> 3), raw);
            Word delay = rotate(t.spike, g.hearing.expression >> 27);
            auto cells = grounding::release({ear.low[id], ear.high[id]}, g.hearing.high & 15);
            Word lo = regulation::truth(g.hearing.high >> 8, afferent, delay, cells);
            Word hi = regulation::truth(g.hearing.high >> 16, afferent >> 16, delay >> 16, cells >> 16);
            t.spike = lo | (hi << 16);
            if (c.plastic)
                ++ear.visits[id];
            a.heard = 1 + id;
        }
        if (observed && c.plastic)
            q.acoustic_edits +=
                utterance::associate(ear, song, (images >> (8 * j)) & 255, w.braids.minds[slot].genes.plastic >> 8);
        a.energy -= song.n;
        a.worked += song.n;
        ++q.listeners;
        q.ear_heat += song.n;
        q.spike_changes += __builtin_popcount(old ^ t.spike);
        if (t.spike != old)
            assign_band(w, c.tiles, slot, band, t);
    }
}
