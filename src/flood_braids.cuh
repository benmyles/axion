#pragma once
#include "axion/braid_view.hpp"

namespace axion::life::braid {
using namespace axion::life::flood;
AX_FN Optics eyes_of(World w, Word animal) {
    return eye_wiring(w.braids.mode, w.braids.minds[animal].genes, w.animals[animal].heading);
}
AX_FN void retain_scene(World w, Word actor, Word observer, Word start, Word end, Prediction aim, Plan action,
                        Word stamp) {
    auto lens = eyes_of(w, observer);
    w.braids.episodes[observer] = episode(w.braids.minds[observer], project_view(start, lens), project_view(end, lens),
                                          aim, action, actor != observer, stamp);
}
AX_FN Word pull(Word colors, Word e) { return exchanged(colors, e, (e + 1) & 3); }
AX_FN void weave(World w, Configuration c, Word home, Word next, Word edge, Word who, Word salt) {
    auto &a = w.animals[who];
    auto &s = w.land[home];
    auto &bound = edge & 1 ? s.fibre_south : s.fibre_east;
    if (!c.weaving || bound || a.energy < 4 || !(w.braids.minds[who].genes.muscle & (1u << (16 + (w.motor[who] & 7)))))
        return;
    if (a.fibre)
        --a.fibre;
    else if (w.land[next].litter && (salt & 4))
        --w.land[next].litter;
    else
        return;
    ++bound;
    --a.energy;
    ++a.worked;
    practice::disturb(w.practice, c.side, home, next);
    tag_strand(w, c.side, home, next, 0, 1);
    count(w.account->fibre_laid, 1);
    count(w.account->heat, 1);
    count(w.braids.book->spent, 1);
}
template <class Touch> AX_FN void mooring(World w, Configuration c, Word tile, Word tick, Touch touch) {
    auto f = w.braids;
    if (!(f.mode & turning))
        return;
    Word span = c.side / 2, mask = c.side - 1, phase = (tick >> 2) & 3;
    Word x = (2 * (tile % span) + (phase & 1)) & mask, y = (2 * (tile / span) + (phase >> 1)) & mask;
    Word xx = (x + 1) & mask, yy = (y + 1) & mask;
    Word loc[]{y * c.side + x, y * c.side + xx, yy * c.side + xx, yy * c.side + x}, ids[4], before = 0, strands = 0;
    // Edge owners are geographic origins, also at the wrap of the torus.
    Word root[]{loc[0], loc[1], loc[3], loc[0]};
    Word shard = c.side >= 128 ? tile / 128 : 0;
    f.book += shard;
    w.braids.book = f.book;
    w.account += shard;
    Word key = mix(c.seed ^ mix(tile + tick * 15773)), r = (key >> 6) & 3;
    for (Word j = 0; j < 4; ++j) {
        auto &a = f.floats[loc[j]];
        auto &bed = w.land[loc[j]];
        auto &joint = w.land[root[j]];
        Word band = j & 1 ? joint.fibre_south : joint.fibre_east;
        strands |= Word(!!band) << j;
        if (bed.water && mass(a) < 31 && ((key >> (4 * j)) & 7) < 1 + (a & 3) && (bed.litter || bed.fruit)) {
            auto &stock = bed.litter ? bed.litter : bed.fruit;
            --stock;
            a += 4;
            count(f.book->trapped, 1);
        }
        before |= Word(a & 3) << (2 * j);
        ids[j] = w.slots[loc[j]];
    }
    Word rig = (mix(c.seed ^ mix(birthmark(x >> 2, y >> 2))) & 7) | ((w.land[loc[0]].east & 15) << 3);
    Word gates = (f.mode & open_mesh) ? 15 : strands;
    if (f.mode & slipping)
        rig ^= 4;
    for (Word z = 0; z < 4; ++z) {
        Word near = (r + z) & 3, who = ids[near];
        auto &a = w.animals[who];
        auto &m = f.minds[who];
        if (a.energy < 4 || !a.body.limbs || !a.body.eyes || !((m.genes.muscle >> (8 + (w.motor[who] & 7))) & 1))
            continue;
        auto lens = eyes_of(w, who);
        Word opening = project_view(before, lens), place = pigment(lens.places, near);
        Word motor = (w.motor[who] ^ (m.genes.muscle >> 24)) & 3, fallback = motor, want = none, seen = 0;
        if ((f.mode & hearing) && m.last && (m.genes.muscle & (1u << 28))) {
            auto pred = predict(matrix(m, motors + m.last - 1), opening);
            if (pred.known & (1u << place)) {
                want = pigment(pred.colors, place);
                count(f.book->requested, 1);
            }
        }
        if (want == none && (m.genes.muscle & (1u << 29))) {
            Word t = m.taste;
            for (Word j = 0; j < 4; ++j) {
                Word hue = (j + r) & 3;
                if (((t >> (2 * hue)) & 3) >= 2) {
                    want = hue;
                    break;
                }
            }
        }
        if (want != none && pigment(opening, place) != want) {
            for (Word j = 0; j < 4; ++j) {
                Word k = (fallback + j) & 3;
                if (!(m.lived & (1u << k)))
                    continue;
                auto p = predict(matrix(m, k), opening);
                if ((p.known & (1u << place)) && pigment(p.colors, place) == want) {
                    motor = k;
                    seen = 1;
                    break;
                }
            }
        }
        Word e = pouch(m.genes, motor, rig), far = (e + 1) & 3;
        if (!(gates & (1u << e)) && (f.mode & coupling)) {
            Word end = root[e] == loc[e] ? loc[far] : loc[e];
            weave(w, c, root[e], end, e, who, key);
        }
        Word after = valve(before, motor, m.genes, rig, gates);
        Word outcome = project_view(after, lens);
        if (f.episodes) {
            auto aim = predict(matrix(m, motor), opening);
            Word stamp = 1 + tick;
            for (Word j = 0; j < 4; ++j) {
                const auto &witness = w.animals[ids[j]];
                if (j == near || ((f.mode & watching) && witness.energy && witness.body.eyes))
                    retain_scene(w, who, ids[j], before, after, aim, {motor, 1, 1}, stamp);
            }
        }
        if (seen) {
            Word other = project_view(valve(before, fallback, m.genes, rig, gates), lens);
            count(f.book->known_pulls, 1);
            count(f.book->goal_gain, pigment(outcome, place) == want && pigment(other, place) != want);
            count(f.book->goal_loss, pigment(outcome, place) != want && pigment(other, place) == want);
        }
        if (gates & (1u << e)) {
            auto pass = [&](Word left, Word right) {
                auto b = f.floats[loc[left]], d = f.floats[loc[right]];
                f.floats[loc[left]] = d | 128;
                f.floats[loc[right]] = b | 128;
                count(f.book->transferred, 2);
                count(f.book->carried, mass(b) + mass(d));
                count(f.book->across, (phase != 0) * (mass(b) + mass(d)));
            };
            pass(e, far);
            if (((rig >> (3 + e)) & 1) && (gates & (1u << far)))
                pass(far, (far + 1) & 3);
            count(f.book->engaged, 1);
        }
        --a.energy;
        ++a.worked;
        count(w.account->heat, 1);
        count(f.book->spent, 1);
        count(f.book->strokes, 1);
        Word made = 0, tone = term(m, opening, outcome, motor, key, (f.mode & inventing) && a.body.pores, made);
        bool plastic = c.plastic && (f.mode & changing);
        if (plastic)
            count(f.book->changes, touch(who, m, motor, opening, outcome, place));
        Word recalled = 0;
        if (tone != none && (c.wire & 3) && a.body.pores) {
            count(f.book->spoken, 1);
            count(f.book->babbled, made);
            count(f.book->named, !made && named(m, opening, outcome, key >> 7) == tone);
            Word signal = tone;
            if ((c.wire & 3) == 2)
                signal = mix(key + 71) & 7;
            if ((c.wire & 3) == 3)
                signal = (tone + 1 + c.seed % 7) & 7;
            Phrase p = phrase(tone, 1);
            if (plastic)
                count(f.book->changes, listen(m, p, opening, outcome));
            for (Word j = 0; j < 4; ++j) {
                if (j == near || !w.animals[ids[j]].energy || !w.animals[ids[j]].body.eyes || !(f.mode & watching) ||
                    !(f.mode & hearing))
                    continue;
                auto &pupil = f.minds[ids[j]];
                Phrase q = phrase(signal, 1);
                auto other = eyes_of(w, ids[j]);
                Word prior = project_view(before, other), later = project_view(after, other);
                auto expect = predict(matrix(pupil, motors + signal), prior);
                pupil.volley = (pupil.volley & 0xffff0000u) | expect.colors | (expect.known << 8) | (signal << 12);
                pupil.last = 1 + signal;
                count(f.book->heard, 1);
                if (plastic)
                    count(f.book->changes, listen(pupil, q, prior, later));
            }
            recalled = predict(matrix(m, motors + tone), opening).colors;
        }
        m.volley = (m.volley & 0xffff0000u) | (opening ^ outcome) | (recalled << 8);
        auto &float_here = f.floats[loc[near]];
        Word amount = 0, hue = pigment(lens.hues, float_here & 3);
        if ((f.mode & feeding) && a.body.pores && ((m.genes.muscle >> (4 + (w.motor[who] & 3))) & 1)) {
            amount = cut(cut(mass(float_here), 1 + nbits(a.body.pores) / 2), 65535u - a.energy);
            float_here -= Half(amount * 4);
            a.energy += Half(amount);
            a.meals += amount;
            a.diet[1] += amount;
            count(w.account->harvest, amount);
            count(w.account->digested, amount);
            count(f.book->meals, amount);
            count(f.book->earned, amount != 0);
            count(f.book->others, (float_here & 128) ? amount : 0);
            if (distance(float_here, loc[near] % c.side, loc[near] / c.side) >= 3) {
                count(f.book->traced_pulls, amount != 0);
                count(f.book->traced_food, amount);
            }
            if (plastic) {
                Word shift = 2 * hue, old = (m.taste >> shift) & 3, law = (m.genes.plastic >> 16);
                Word next = (law >> (2 * (old + 4 * Word(amount != 0)))) & 3;
                m.taste = (m.taste & ~(3u << shift)) | (next << shift);
            }
        }
        return;
    }
}
} // namespace axion::life::braid
