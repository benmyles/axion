#pragma once
#include "axion/braid_gate.hpp"
namespace axion::life::braid {
using namespace axion::life::flood;
struct Canal {
    Word pos[2][4]{}, pressure[4]{}, wires[4]{}, collars[4]{}, registration = 0;
    Half floaters[2][4]{};
};
AX_FN Word visible(const Half (&floats)[4]) {
    Word out = 0;
    for (Word i = 0; i < 4; ++i)
        out |= Word(floats[i] & 3) << (2 * i);
    return out;
}
AX_FN Word lacing(World w, const Word (&sites)[4]) {
    Word a = 0;
    if (w.land[sites[0]].fibre_east)
        a |= 1;
    if (w.land[sites[1]].fibre_south)
        a |= 2;
    if (w.land[sites[3]].fibre_east)
        a |= 4;
    if (w.land[sites[0]].fibre_south)
        a |= 8;
    return (w.braids.mode & open_mesh) ? 15 : a;
}
AX_FN Canal canal(World w, Configuration c, Word square, Word tick) {
    Canal r{};
    Word side = c.side, mask = side - 1, cols = side / 4;
    Word x0 = (4 * (square % cols) + ((tick >> 3) & 3)) & mask, y0 = (2 * (square / cols) + ((tick >> 5) & 1)) & mask;
    Word inverse = (tick >> 6) & 1, ground = mix(c.seed + 37 * (x0 >> 3) + 1889 * (y0 >> 3));
    r.registration = ((ground >> 4) ^ w.braids.phase) & 3;
    for (Word i = 0; i < 4; ++i) {
        Word y = (y0 + Word(i >= 2)) & mask, left = (x0 + Word(i == 1 || i == 2)) & mask;
        Word a = y * side + left, mid = y * side + ((left + 1) & mask), b = y * side + ((left + 2) & mask);
        r.pos[inverse][i] = a;
        r.pos[inverse ^ 1][i] = b;
        r.wires[i] = (w.braids.mode & open_mesh) ? 2u : cut(w.land[a].fibre_east, w.land[mid].fibre_east);
        Word src = r.pos[0][i], dest = r.pos[1][i];
        // A moving head supplies mechanical work. Geometry, local water and
        // finite loads still constrain each of the four paths independently.
        Word up = w.land[src].water + ((tick >> 4) & 7) + 1, down = w.land[dest].water;
        r.pressure[i] = up > down ? 1 + cut(3, (up - down) / 4) : 0;
        for (Word bank = 0; bank < 2; ++bank)
            r.floaters[bank][i] = w.braids.floats[r.pos[bank][i]];
    }
    for (Word i = 0; i < 4; ++i) {
        r.collars[i] = r.registration;
        if (w.braids.mode & mineral_bed) {
            // Two adjacent banks support one collar. The same ridge supports
            // a neighbouring port when the tide reverses.
            const auto &bed = w.land[r.pos[1][i]], &other = w.land[r.pos[1][(i + 1) & 3]];
            r.collars[i] = (r.registration + bed.east + other.south) & 3;
        }
    }
    return r;
}
AX_FN Word forecast(const Canal &r, Word arrangement) {
    Word volume = 0;
    for (Word i = 0; i < 4; ++i)
        volume +=
            crossing(r.floaters[0][i], r.floaters[1][pigment(arrangement, i)], r.pressure[i], r.wires[i], r.collars[i])
                .volume;
    return volume;
}
AX_FN Word transform(Plan program, Genes genes, Word rig, Word strands) {
    Word image = order;
    for (Word i = 0; i < program.size; ++i)
        image = valve(image, (program.stream >> (3 * i)) & 3, genes, rig, strands);
    return image;
}
AX_FN Phrase solicit(const Mind &speaker, const GateMemory &connections, const Canal &r, Word key, Word somatic,
                     Word &deliberate, Optics lens) {
    Word near = project_view(visible(r.floaters[0]), lens), far = project_view(visible(r.floaters[1]), lens),
         target = 0, exposed = 0;
    for (Word i = 0; i < 4; ++i)
        if (mass(r.floaters[0][i]) && r.wires[i] && r.pressure[i]) {
            Word place = pigment(lens.places, i);
            Word hue = indicated(connections, pigment(near, place));
            if (hue != none) {
                exposed |= 1u << place;
                target |= hue << (2 * place);
            }
        }
    if (exposed && (speaker.genes.arbor & (1u << 16))) {
        auto p = route(release(speaker, true), far, target, exposed, depth(speaker.genes));
        if (p.found && p.size) {
            deliberate = 1;
            return sequence(phrase(p.stream, p.size), reversed(speaker));
        }
    }
    // A body can broadcast a spontaneous pulse even before it has acquired
    // a useful route. This efferent has no view of the hidden register.
    if (speaker.genes.throat & (1u << 23)) {
        Word loc = (key >> 17) & 3, v = (pigment(near, loc) + 4 * (somatic & 1)) & 7;
        return phrase((speaker.genes.throat >> (3 * v)) & 7, 1);
    }
    return {};
}
AX_FN Plan respond_to(const Mind &p, const Phrase &heard, Word scene, Word purse, Word default_motor, Word &grounded) {
    Plan baseline{default_motor, 1, 1};
    if (!heard.n)
        return baseline;
    if (p.genes.muscle & (1u << 28)) {
        auto what = predict(pathway(p, heard), scene);
        if (what.known == 15) {
            auto plan = route(release(p, false), scene, what.colors, 15, depth(p.genes));
            if (plan.found && plan.size <= purse) {
                grounded = 1;
                return plan;
            }
        }
    }
    baseline.stream = (p.genes.muscle >> (2 * heard.cell[0])) & 3;
    return baseline;
}
AX_FN void gateway(World w, Configuration c, Word tile, Word tick) {
    auto f = w.braids;
    Word shard = c.side >= 128 ? tile / 128 : 0;
    f.book += shard;
    w.account += shard;
    auto &book = f.gates[shard];
    auto r = canal(w, c, tile, tick);
    Word k = mix(c.seed ^ mix(tile + tick * 761u)), slots[]{(k >> 6) & 3, (k >> 14) & 3};
    Word id[]{w.slots[r.pos[0][slots[0]]], w.slots[r.pos[1][slots[1]]]};
    auto &a = w.animals[id[0]], &b = w.animals[id[1]];
    auto &speaker = f.minds[id[0]], &listener = f.minds[id[1]];
    auto &gate = f.contacts[id[0]];
    Word connections = 0;
    for (Word i = 0; i < 4; ++i)
        connections += bool(r.wires[i] && r.pressure[i] && mass(r.floaters[0][i]));
    if (!connections)
        return;
    Phrase sent{};
    if ((f.mode & turning) && !(f.mode & passive_gates) && a.energy > 10 && a.body.eyes && a.body.pores &&
        ((speaker.genes.throat >> (16 + (w.motor[id[0]] & 3))) & 1)) {
        Word learned = 0;
        sent = solicit(speaker, gate, r, k, w.motor[id[0]], learned, eyes_of(w, id[0]));
        a.energy -= Half(sent.n);
        a.worked += sent.n;
        count(w.account->heat, sent.n);
        count(f.book->spent, sent.n);
        count(book.requests, sent.n != 0);
        count(book.syllables, sent.n);
        count(book.patterned, sent.n > 1);
        count(book.predicted_requests, learned);
    }
    Word actual = order, quiet = order, uninformed = order, grounded = 0;
    Word strands = lacing(w, r.pos[1]);
    Word station = r.pos[1][0];
    Word rig = (mix(c.seed ^ mix(birthmark(station % c.side >> 2, station / c.side >> 2))) & 7) |
               ((w.land[station].east & 15) << 3);
    if (f.mode & slipping)
        rig ^= 4;
    if ((f.mode & turning) && !(f.mode & passive_gates) && b.energy > 3 && b.body.limbs && b.body.eyes &&
        ((listener.genes.muscle >> (8 + (w.motor[id[1]] & 7))) & 1)) {
        Phrase audible = sent;
        if ((f.mode & hush) || !(c.wire & 3) || !(f.mode & hearing))
            audible.n = 0;
        for (Word i = 0; i < audible.n; ++i) {
            if ((c.wire & 3) == 2)
                audible.cell[i] = mix(k + i * 907) & 7;
            else if ((c.wire & 3) == 3 || (f.mode & shifted_gates))
                audible.cell[i] = (audible.cell[i] + 1) & 7;
        }
        Word native = (w.motor[id[1]] ^ (listener.genes.muscle >> 24)) & 3;
        auto lens = eyes_of(w, id[1]);
        Word board = visible(r.floaters[1]), old = project_view(board, lens);
        auto plan = respond_to(listener, audible, old, b.energy - 1, native, grounded);
        actual = transform(plan, listener.genes, rig, strands);
        quiet = valve(order, native, listener.genes, rig, strands);
        uninformed =
            audible.n ? valve(order, (listener.genes.muscle >> (2 * audible.cell[0])) & 3, listener.genes, rig, strands)
                      : quiet;
        if (f.episodes && plan.size) {
            Word route = identity;
            for (Word j = 0; j < plan.size; ++j)
                route = compose(matrix(listener, (plan.stream >> (3 * j)) & 7), route);
            Word end = transfer(board, actual);
            for (Word j = 0; j < 4; ++j) {
                Word slot = w.slots[r.pos[1][j]];
                const auto &witness = w.animals[slot];
                if (slot == id[1] || ((f.mode & watching) && witness.energy && witness.body.eyes))
                    retain_scene(w, id[1], slot, board, end, predict(route, old), plan, 1 + tick);
            }
        }
        Word scene = board;
        // Actual intermediate sights can teach the responding body its own hand.
        for (Word j = 0; j < plan.size; ++j) {
            Word motor = (plan.stream >> (3 * j)) & 3, end = valve(scene, motor, listener.genes, rig, strands);
            if (c.plastic && (f.mode & changing))
                count(f.book->changes,
                      motor_touch(listener, motor, project_view(scene, lens), project_view(end, lens), false));
            scene = end;
        }
        b.energy -= Half(plan.size);
        b.worked += plan.size;
        count(w.account->heat, plan.size);
        count(f.book->spent, plan.size);
        count(book.responses, audible.n != 0);
        count(book.understood, grounded);
        count(book.pulls, plan.size);
        count(book.heard_work, actual != quiet);
        count(book.learned_work, actual != uninformed);
        if (audible.n) {
            listener.last = audible.cell[audible.n - 1] + 1;
            auto p = predict(pathway(listener, audible), old);
            listener.volley = (listener.volley & 0xffff0000u) | p.colors | (p.known << 8) | (audible.cell[0] << 12);
        }
    }
    Word volume = forecast(r, actual), base = forecast(r, quiet), blind = forecast(r, uninformed);
    count(book.better, volume > base);
    count(book.worse, base > volume);
    if (volume > base)
        count(book.hearing_surplus, volume - base);
    if (volume < base)
        count(book.hearing_deficit, base - volume);
    if (volume > blind)
        count(book.learned_surplus, volume - blind);
    if (volume < blind)
        count(book.learned_deficit, blind - volume);
    for (Word i = 0; i < 4; ++i) {
        Word from = r.pos[0][i], to = r.pos[1][i], prev = pigment(actual, i);
        auto source = r.floaters[0][i], sink = r.floaters[1][prev];
        if (prev != i)
            sink |= 128;
        auto tr = crossing(source, sink, r.pressure[i], r.wires[i], r.collars[i]);
        if ((f.mode & mineral_bed) && tr.allowed) {
            auto &s = f.sediment[shard];
            auto flat = crossing(source, sink, r.pressure[i], r.wires[i], r.registration);
            count(s.wet, 1);
            count(s.relieved, r.collars[i] != r.registration);
            if (tr.volume > flat.volume) {
                count(s.opened, 1);
                count(s.gained, tr.volume - flat.volume);
            }
            if (flat.volume > tr.volume) {
                count(s.closed, 1);
                count(s.lost, flat.volume - tr.volume);
            }
            if ((f.mode & mobile_bed) && ((k >> (5 * i)) & 7) == 0) {
                auto &bed = w.land[to];
                if (tr.volume && bed.grains && bed.east != 65535) {
                    --bed.grains;
                    ++bed.east;
                    count(s.lodged, 1);
                    count(s.turned, prev != i);
                } else if (!tr.volume && r.pressure[i] >= 2 && bed.east && bed.grains != 65535) {
                    --bed.east;
                    ++bed.grains;
                    count(s.scoured, 1);
                }
            }
        }
        count(book.wet, tr.allowed);
        count(book.crossings, tr.volume != 0);
        count(book.flux, tr.volume);
        source -= Half(4 * tr.volume);
        sink += Half(4 * tr.volume);
        if (tr.allowed && c.plastic && (f.mode & changing) && !(f.mode & frozen_gates)) {
            if (a.energy && a.body.eyes && (speaker.genes.arbor & (1u << 16))) {
                count(book.observed, 1);
                auto lens = eyes_of(w, id[0]);
                count(book.writes, pressure_event(gate, speaker.genes, pigment(lens.hues, source & 3),
                                                  pigment(lens.hues, sink & 3), tr.volume != 0));
            }
        }
        if (tr.volume && (f.mode & feeding)) {
            auto feed = [&](Colony &animal, Word units) {
                if (!animal.energy || !animal.body.pores || animal.energy == 65535 || !units)
                    return 0u;
                ++animal.energy;
                ++animal.meals;
                ++animal.diet[1];
                count(w.account->harvest, 1);
                count(w.account->digested, 1);
                count(f.book->meals, 1);
                count(book.picked, 1);
                return 1u;
            };
            Word taken = slots[1] == i ? feed(b, tr.volume) : 0;
            if (slots[0] == i)
                taken += feed(a, tr.volume - taken);
            sink -= Half(4 * taken);
        }
        w.braids.floats[from] = source;
        w.braids.floats[to] = sink;
    }
}
} // namespace axion::life::braid
