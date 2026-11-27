#pragma once
#include "axion/rattle.hpp"
#include "flood_body.cuh"

namespace axion::life::rattle {
using namespace axion::life::flood;
struct Port {
    Word site, id, wave = silence, organic = 0, strand = 0, offer = 0, solo = 0, rent = 0, braced = 0;
};
AX_FN Half &pool(World w, Word site) {
    if (w.hollows.beds && w.hollows.beds[site].relief)
        return w.hollows.beds[site].carbon;
    return w.land[site].litter;
}
AX_FN void eat_at(World w, Word site, Colony &a, Word food) {
    pool(w, site) -= Half(food);
    a.energy += Half(food);
    a.meals += food;
    a.diet[1] += food;
    count(w.account->harvest, food);
    count(w.account->digested, food);
    if (w.hollows.beds && w.hollows.beds[site].relief) {
        count(w.hollows.ledger->released, food);
        count(w.hollows.ledger->eaten, food);
    }
}
AX_FN void pay(World w, Totals &b, Colony &a) {
    --a.energy;
    ++a.worked;
    count(w.account->heat, 1);
    count(b.cost, 1);
}
AX_FN Port excite(World w, Field f, Word home, Word remote, Word axis, Word limbs, Word roll, bool struck) {
    Word id = w.slots[remote], edge = home < remote ? home : remote;
    auto &speaker = w.animals[id];
    auto &bed = w.land[edge];
    auto &end = w.land[remote];
    auto g = f.minds[id].genes;
    Word mass = pool(w, remote), cord = axis ? bed.fibre_south : bed.fibre_east;
    Port p{remote, id, silence, 0, cord, cut(mass, capacity(cord, limbs))};
    if (!struck)
        return p;
    if ((f.mode & voices) && speaker.energy > 2 && speaker.body.limbs && sound_motor(g.drum, w.motor[id])) {
        p.wave = knock(g, mass > ((g.drum >> 18) & 3), end.water > 16, w.motor[id]);
        if (p.wave != silence) {
            pay(w, *f.books, speaker);
            p.organic = 1;
            count(f.books->calls, 1);
        }
    }
    if (p.wave == silence && cord) {
        Word ring = (end.spore ^ (end.root >> 8)) & 3;
        Word lip = axis ? bed.south : bed.east;
        // A passive tongue has opposed loads from sediment, the water and the lip.
        Word up = mass + end.water / 8 > 2 + w.land[home].water / 8 + lip / 4;
        p.wave = 2 * ring + up;
        count(f.books->resonances, 1);
    }
    Word fold = cord && w.wicks.reeds.seams ? runnel::fold_at(w.wicks.reeds.seams[edge], axis) : 0;
    p.wave = conduct(p.wave, ((f.phase >> axis) & 1) ^ (fold ? (fold - 1) & 1 : 0));
    if ((f.mode & jitter) && p.wave != silence)
        p.wave = (p.wave & 6) | (roll & 1);
    return p;
}
AX_FN void silt_into_mesh(World w, Field f, Word home) {
    if (!w.hollows.beds)
        return;
    auto &soil = w.land[home];
    auto &bed = w.hollows.beds[home];
    if (!bed.relief || !soil.water)
        return;
    Word weave = cut(12, Word(soil.fibre_east) + soil.fibre_south);
    Word limit = 4 * weave + bed.cover;
    if (!weave || bed.carbon >= limit)
        return;
    auto &fresh = soil.litter ? soil.litter : soil.fruit;
    Word caught = cut(cut(1 + weave / 4, limit - bed.carbon), fresh);
    fresh -= Half(caught);
    bed.carbon += Half(caught);
    count(w.hollows.ledger->settled, caught);
    count(f.load->trapped, caught);
}
AX_FN void tension(World w, Port &port, Word home, Word axis, Word limbs, Word room) {
    Word edge = home < port.site ? home : port.site;
    auto &soil = w.land[edge];
    Word lip = axis ? soil.south : soil.east;
    Word lid = w.hollows.beds ? w.hollows.beds[port.site].cover : 0;
    Word excess =
        port.strand && port.organic && w.animals[port.id].body.pores ? nbits(w.animals[port.id].body.limbs) : 0;
    Word force = capacity(port.strand, limbs), brace = 2 * cut(port.strand, excess), cover = 1 + cut(12, lid + lip / 3);
    auto catchment = dislodge(pool(w, port.site), cover, force, brace, room, 65535 - w.animals[port.id].energy);
    port.solo = catchment.solo;
    port.offer = catchment.near;
    port.rent = catchment.far;
    port.braced = brace != 0;
}
AX_FN void difference(Count &changed, Count &gain, Count &loss, Word actual, Word other, const Port (&p)[2]) {
    if (actual == other)
        return;
    count(changed, 1);
    Word a = p[actual].offer, b = p[other].offer;
    if (a > b)
        count(gain, a - b);
    if (b > a)
        count(loss, b - a);
}
AX_FN void cove(World w, Configuration c, Field f, Word tile, Word tick) {
    if (!(f.mode & moving))
        return;
    Word half = c.side >> 1, corner = (tile / half) * 2 * c.side + (tile % half) * 2;
    Word loc[]{corner, corner + 1, corner + c.side, corner + c.side + 1};
    Word shard = c.side >= 128 ? tile / 128 : 0;
    f.books += shard;
    if (f.load)
        f.load += shard;
    w.account += shard;
    if (w.hollows.ledger)
        w.hollows.ledger += shard;
    auto &b = *f.books;
    for (Word j = 0; j < 4; ++j) {
        if (f.mode & loaded)
            silt_into_mesh(w, f, loc[j]);
        Word id = w.slots[loc[j]];
        if (!w.animals[id].energy)
            continue;
        Word changes = digest(f.minds[id], tick, c.plastic && (f.mode & learning));
        if (changes) {
            count(b.settled, 1);
            count(b.edits, (changes >> 1) & 1);
            count(b.switches, changes >> 2);
        }
    }
    Word anchor = ((tick >> 2) + mix(tile ^ c.seed)) & 3, pos = loc[anchor], id = w.slots[pos];
    auto &a = w.animals[id];
    auto &m = f.minds[id];
    if (a.energy < 4 || !a.body.pores || !a.body.limbs || waiting(m) || !sound_motor(m.genes.ear, w.motor[id]))
        return;
    Word limbs = nbits(a.body.limbs), roll = mix(c.seed ^ mix(tile + tick * 13691u));
    bool scarce = f.mode & single;
    Word inquiry = 0, unlearned = 0;
    if (scarce) {
        auto tag = [&](Word axis) {
            Word remote = loc[anchor ^ (1u << axis)], other = w.slots[remote];
            auto &neighbor = w.animals[other];
            if ((f.mode & voices) && neighbor.energy > 2 && neighbor.body.limbs)
                return (f.minds[other].genes.drum >> 16) & 3;
            auto &bed = w.land[remote < pos ? remote : pos];
            if (!(axis ? bed.fibre_south : bed.fibre_east))
                return no_tag;
            auto &end = w.land[remote];
            return (end.spore ^ (end.root >> 8)) & 3;
        };
        Word x = tag(0), y = tag(1);
        inquiry = invite(m, x, y, roll, true, f.mode & crossed);
        unlearned = invite(m, x, y, roll, false, f.mode & crossed);
    }
    Port p[]{excite(w, f, pos, loc[anchor ^ 1], 0, limbs, mix(roll + 7121), !scarce || !inquiry),
             excite(w, f, pos, loc[anchor ^ 2], 1, limbs, mix(roll + 9901), !scarce || inquiry)};
    pay(w, b, a);
    for (Word j = 0; j < 2; ++j) {
        if (f.mode & loaded)
            tension(w, p[j], pos, j, limbs, 65535u - a.energy);
        else
            p[j].offer = cut(p[j].offer, 65535u - a.energy);
    }
    bool hear = (f.mode & hearing) && (c.wire & 3);
    Word l = hear ? p[0].wave : silence, r = hear ? p[1].wave : silence;
    if (l != silence || r != silence) {
        count(b.voiced, 1);
        count(b.mixed, l != silence && r != silence);
    }
    Word wire = c.wire & 3;
    if (wire == 2 || wire == 3) {
        auto relay = [&](Word wave, Word salt) {
            return wave == silence ? silence : wire == 2 ? (mix(roll + salt) & 7) : ((wave + 1 + c.seed % 7) & 7);
        };
        l = relay(l, 8311);
        r = relay(r, 4831);
    }
    if(w.braids.minds && (w.braids.mode&braid::projecting)){
        Word old=(1u<<signal(m,l,false))|(1u<<(8+signal(m,r,false)));
        Word learned=(1u<<signal(m,l,true))|(1u<<(8+signal(m,r,true)));
        auto &ear=w.braids.minds[id].volley;
        ear=(ear&65535)|((old^learned)<<16);
    }
    Word move = pick(m, l, r, roll);
    if (scarce) {
        auto &list = f.attention[shard];
        count(list.solicited, 1);
        count(list.replies, p[inquiry].wave != silence);
        if (inquiry != unlearned) {
            count(list.changed, 1);
            // This is the food at the solicited endpoint, independent of where the
            // listener subsequently pulls. No second voice or hypothetical meal is emitted.
            auto a = p[inquiry].offer, b = p[unlearned].offer;
            if (a > b)
                count(list.gain, a - b);
            if (b > a)
                count(list.loss, b - a);
        }
    }
    difference(b.sound_changes, b.sound_gain, b.sound_loss, move, pick(m, silence, silence, roll), p);
    difference(b.learned_changes, b.learned_gain, b.learned_loss, move, pick(m, l, r, roll, false), p);
    difference(b.resident_changes, b.resident_gain, b.resident_loss, move,
               pick(m, p[0].organic ? silence : l, p[1].organic ? silence : r, roll), p);
    difference(b.built_changes, b.built_gain, b.built_loss, move,
               pick(m, p[0].organic ? l : silence, p[1].organic ? r : silence, roll), p);
    auto &chosen = p[move];
    eat_at(w, chosen.site, a, chosen.offer);
    taste(m, move ? r : l, move ? l : r, chosen.offer, tick, f.mode & crossed);
    count(b.pulls, 1);
    count(b.conducted, chosen.strand != 0);
    count(b.food, chosen.offer);
    // A strumming animal can hold the loaded strand with its other limbs.
    auto &gripper = w.animals[chosen.id];
    if (f.mode & loaded) {
        eat_at(w, chosen.site, gripper, chosen.rent);
        count(b.return_food, chosen.rent);
        count(f.load->to_worker, chosen.braced ? chosen.offer : 0);
        count(f.load->to_gripper, chosen.rent);
        count(f.load->opposed, chosen.braced);
        count(f.load->pinched, chosen.braced && !chosen.solo && (chosen.offer + chosen.rent));
        if (chosen.offer + chosen.rent > chosen.solo)
            count(f.load->added, chosen.offer + chosen.rent - chosen.solo);
        return;
    }
    if (chosen.organic && chosen.strand && gripper.body.pores && (f.minds[chosen.id].genes.drum & (1u << 22))) {
        Word caught = cut(cut(pool(w, chosen.site), 2u), 65535u - gripper.energy);
        eat_at(w, chosen.site, gripper, caught);
        count(b.return_food, caught);
    }
}
} // namespace axion::life::rattle
