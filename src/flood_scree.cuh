// A warp owns an eight-by-four patch. During a real exposure each lane is
// one piece of matter. During replay each lane is a different imagined act.
#include "axion/foresight.hpp"
namespace axion::life::material::scree {
AX_FN Word east(Word p) { return ((p & 0x7f7f7f7fu) << 1) | ((p & 0x80808080u) >> 7); }
AX_FN Word west(Word p) { return ((p & 0xfefefefeu) >> 1) | ((p & 0x01010101u) << 7); }
AX_FN Word south(Word p) { return (p << 8) | (p >> 24); }
AX_FN Word north(Word p) { return (p >> 8) | (p << 24); }
AX_FN Word surround(Word p) { return east(p) | west(p) | north(p) | south(p); }
AX_FN Word ballot(bool active) {
    Word bits;
    asm volatile("{ .reg .pred t; setp.ne.u32 t,%1,0; vote.sync.ballot.b32 %0,t,0xffffffff; }"
                 : "=r"(bits)
                 : "r"(Word(active)));
    return bits;
}
AX_FN Word pack(Grain g) { return __builtin_bit_cast(Word, g); }
AX_FN Grain unpack(Word p) { return __builtin_bit_cast(Grain, p); }
AX_FN Word pick(Word mask, Word k) {
    if (!mask)
        return 0;
    Word roll = k & 31, bits = rotate(mask, (32 - roll) & 31);
    return (roll + __builtin_ctz(bits)) & 31;
}
AX_FN Word fold_contacts(Word tissue, Word release) {
    Word low = tissue & 0x55555555u, high = (tissue >> 1) & 0x55555555u;
    Word on = ((0u - (release & 1u)) & ~low & ~high) | ((0u - ((release >> 1) & 1u)) & low & ~high) |
              ((0u - ((release >> 2) & 1u)) & ~low & high) | ((0u - ((release >> 3) & 1u)) & low & high);
    on &= 0x55555555u;
    on = (on | (on >> 1)) & 0x33333333u;
    on = (on | (on >> 2)) & 0x0f0f0f0fu;
    on = (on | (on >> 4)) & 0x00ff00ffu;
    return (on | (on >> 8)) & 0xffffu;
}
AX_FN SpatialImage settle(const Foresight &f, Word release) {
    SpatialImage p{};
    p.contacts = static_cast<Half>(fold_contacts(f.contacts, release));
    for (Word j = 0; j < 2; ++j) {
        Word row = fold_contacts(f.muscles[j], release);
        p.motor[2 * j] = row;
        p.motor[2 * j + 1] = row >> 8;
    }
    p.taste = fold_contacts(f.appetite, release) & 15;
    return p;
}
AX_FN bool passes(Word mask, Word a, Word b, Word dir) {
    return a && b && ((mask >> (4 * ((a >> (2 * dir)) & 3) + ((b >> (2 * (dir ^ 2))) & 3))) & 1);
}
AX_FN Word motor_for(const Foresight &f, const Genes &g, Word kind, Word key) {
    Word mask = 0;
    for (Word m = 0; m < 8; ++m)
        mask |= Word(fires(f.muscles[kind / 2], (kind & 1) * 8 + m, g.release)) << m;
    return mask ? pick(mask, key) : (key & 7);
}
AX_FN Coast coast(Word seed, Word span) {
    Coast c;
    Word k = mix(seed + 0x1735u);
    c.sun = draw(k) & 31;
    Word ring = 1u << c.sun, inside = 0;
    for (Word j = 0; j < span; ++j) {
        inside |= ring;
        ring = surround(ring) & ~inside;
    }
    c.first = pick(ring, draw(k));
    ring &= ~(1u << c.first) & ~surround(1u << c.first);
    c.second = pick(ring, draw(k));
    c.occupied = (1u << c.sun) | (1u << c.first) | (1u << c.second);
    for (Word j = 0; j < 8; ++j) {
        Word free = ~(c.occupied | surround(c.occupied));
        if (!free)
            break;
        c.occupied |= 1u << pick(free, draw(k));
    }
    return c;
}
AX_FN void anatomy(Coast &c, Word key) {
    c.glass = palette(mix(key + 921));
    c.tendons = permutation(mix(key + 3023));
    c.odor = palette(mix(key + 827));
    c.responsive = mix(key + 479) & 1;
}
AX_FN Grain stock(const Coast &c, Word seed, Word at) {
    if (!(c.occupied & (1u << at)))
        return {};
    Word r = mix(seed ^ mix(at + 522));
    const Word plugs[] = {0x55, 0xaa, 0x33, 0xcc, 0x99, 0x5a, 0xe7, 0xff};
    Word terminals = at == c.sun || at == c.first || at == c.second ? 0xffu : rotate_faces(plugs[r & 7], (r >> 7) & 3);
    return {static_cast<Byte>(terminals), static_cast<Byte>(0x30u | (c.catalyst << 2) | ((r >> 11) & 3)), 0, 0};
}
AX_FN bool guessed(const Foresight &f, Word release, Word a, Word b, Word dir) {
    return a && b && fires(f.contacts, 4 * ((a >> (2 * dir)) & 3) + ((b >> (2 * (dir ^ 2))) & 3), release);
}
AX_FN void taste(Foresight &f, const Genes &g, Coast c, Word kind) {
    Half raw = 1, food = 0;
    Grain sample{0xff, 0x30, 4, 0};
    excite(sample, 4, kind ^ c.responsive, raw, food);
    f.appetite = observe(f.appetite, color(c.odor, kind), food != 0, g.practice);
}
AX_FN void calibrate(Foresight &f, const Genes &g, Coast c, Word bouts, Word seed, Word mode) {
    Word wiring = wire(g, c.tendons);
    for (Word t = 0; t < bouts; ++t) {
        Word k = mix(seed + t * 1597);
        Word a = k & 3, b = (k >> 2) & 3;
        Grain donor{static_cast<Byte>(0x01u | (a << 2)), 0x33, 2, 0};
        Grain receive{static_cast<Byte>(0x01u | (b << 6)), 0x33, 0, 0};
        conduct(donor, receive, 1, true);
        if (mode != 1)
            plume(f, g, color(c.glass, a), color(c.glass, b), mode == 3 ? bool(k & (1u << 9)) : receive.charge != 0);
        Word m = (k >> 6) & 7, op = (wiring >> (3 * m)) & 7;
        // Each movement is experienced with an empty palm over a loose
        // asymmetric piece and a full palm over a bare patch.
        for (Word hand = 0; hand < 2; ++hand) {
            Grain piece{0x12, 0x33, 0, 0}, here = hand ? Grain{} : piece, held = hand ? piece : Grain{}, next{};
            auto was = here;
            manipulate(here, next, held, op, 1, 0, 0, 64, 0);
            if (mode != 2)
                motor_event(f, g, m, hand, quantity(held), was, here);
        }
        // The animal samples real product from a lit chip on one of four
        // scented surfaces. Its learned preference can change within a life.
        taste(f, g, c, (k >> 11) & 3);
    }
}
struct Picture {
    Word eastward = 0, southward = 0;
};
AX_FN Word propagate(Word active, Picture p) {
    return active | east(active & p.eastward) | (west(active) & p.eastward) | south(active & p.southward) |
           (north(active) & p.southward);
}
AX_FN int readout(Picture p, Word lit, Word desired, const Genes &g, Word mode) {
    // Two assemblies start at seen landmarks. Firing travels across acquired
    // local contacts. Short-range lateral fields make a partially completed
    // arrangement distinguishable even before the two activities meet.
    Word source = lit, target = desired, depth = 1 + ((g.innate >> 25) & 7);
    if (mode == 4)
        depth = 0;
    for (Word i = 0; i < depth; ++i) {
        source = propagate(source, p);
        target = propagate(target, p);
    }
    Word exact = source & target;
    Word adjacent = surround(source), second = surround(adjacent), third = surround(second);
    int near = int(bool(adjacent & target)) * 32 + int(bool(second & target)) * 12 + int(bool(third & target)) * 4;
    return int(bool(exact)) * int(128 + 16 * ((g.observation >> 24) & 7)) + near * int((g.receptors >> 29) & 3) +
           int(__builtin_popcount(source) + __builtin_popcount(target)) * int((g.innate >> 29) & 3);
}
struct Idea {
    Word from = 0, to = 0, kind = 0;
}; // kind 0 carries, 1/2 turn, 3 rests
AX_FN Word compact(Idea a) { return a.from | (a.to << 5) | (a.kind << 10); }
AX_FN Idea decode(Word x) { return {x & 31, (x >> 5) & 31, (x >> 10) & 3}; }
AX_FN Word imagined(Word retinal, Word at, Idea a, Word carried) {
    Word there = raft_shuffle(retinal, at);
    if (a.kind == 0)
        return at == a.from ? 0u : at == a.to ? carried : there;
    if (at == a.from && a.kind <= 2)
        return 256 | rotate_faces(carried, a.kind == 1 ? 1 : 3);
    return there;
}
AX_FN Picture project(Picture picture, Word retinal, Idea a, Word carried, Word mask_of_pairs) {
    // Only the bonds incident on the two edited sites can have changed.
    for (Word j = 0; j < 2; ++j) {
        Word pos = j ? a.to : a.from;
        Word own = imagined(retinal, pos, a, carried);
        for (Word dir = 0; dir < 4; ++dir) {
            Word other = next_site(pos, dir), eye = imagined(retinal, other, a, carried);
            Word mask = 1u << ((dir == 0 || dir == 3) ? other : pos);
            Word &plane = (dir & 1) ? picture.eastward : picture.southward;
            bool connection = (dir == 0 || dir == 3) ? passes(mask_of_pairs, eye, own, dir ^ 2)
                                                     : passes(mask_of_pairs, own, eye, dir);
            plane = (plane & ~mask) | (connection ? mask : 0);
        }
    }
    return picture;
}
AX_FN Word proposal(Word lane, Word retinal, Word source_bits, Word desire_bits, const Foresight &f, const Genes &g,
                    Word options, Word seed, Word mode, Word &activity, Word reach_mask = ~0u, bool bounded = false) {
    Word occupied = ballot(retinal != 0);
    Word pairs = fold_contacts(f.contacts, g.release);
    if (!(reach_mask & occupied) || !(reach_mask & ~occupied))
        return compact({0, 0, 3});
    Word right = raft_shuffle(retinal, next_site(lane, 1)), below = raft_shuffle(retinal, next_site(lane, 2));
    Picture current{ballot(passes(pairs, retinal, right, 1)), ballot(passes(pairs, retinal, below, 2))};
    int best = -1;
    Word choice = 0;
    for (Word j = lane; j < options; j += 32) {
        Word r = mix(seed + 3571 * j), k = mix(r + 317), rot = (r >> 9) & 7;
        Idea a{pick(occupied & reach_mask, r), pick(~occupied & reach_mask, k), rot < 2 ? rot + 1 : 0};
        if (a.kind)
            a.to = a.from;
        if (j == 0)
            a = {0, 0, 3};
        Word carried = raft_shuffle(retinal, a.from);
        auto field = project(current, retinal, a, carried, pairs);
        if (bounded) {
            field.eastward &= 0x7f7f7f7fu;
            field.southward &= 0x00ffffffu;
        }
        Word result = a.kind == 0 ? (occupied & ~(1u << a.from)) | (1u << a.to) : occupied;
        int score = readout(field, source_bits & result, desire_bits & result, g, mode);
        int rank = score * 256 + int(r >> 25);
        if (j == 0)
            rank += int((g.release >> 25) & 15) * 8;
        if (mode == 5)
            rank = int(r >> 8);
        if (rank > best) {
            best = rank;
            choice = compact(a);
        }
    }
    // The low five bits identify a real lane, independently of tie order.
    Word elected = Word(best) * 32 + (31 - lane);
    for (Word d = 16; d; d >>= 1) {
        Word peer = raft_shuffle(elected, lane ^ d);
        if (peer > elected)
            elected = peer;
    }
    activity += options;
    return raft_shuffle(choice, 31 - (elected & 31));
}
} // namespace axion::life::material::scree

namespace axion::life::material::scree {
AX_FN Word attached(Word local, Word pos) {
    Grain a = unpack(raft_shuffle(local, pos));
    Word count = 0;
    for (Word d = 0; d < 4; ++d) {
        Grain b = unpack(raft_shuffle(local, next_site(pos, d)));
        count += bonded(a, b, d);
    }
    return count;
}
struct Palm {
    Grain grain{};
    Word identity = 32;
};
AX_FN Temper reach(Word lane, Grain &tile, Word &identity, Palm &palm, const Genes &g, const Foresight &f, Word kind,
                   Word pos, Word wiring, Word dir, Word reserve, Word key, Word &muscle, Word resting_degree = ~0u) {
    Word peer = next_site(pos, dir), packed = pack(tile);
    Grain a = unpack(raft_shuffle(packed, pos)), b = unpack(raft_shuffle(packed, peer));
    Word id_a = raft_shuffle(identity, pos), id_b = raft_shuffle(identity, peer);
    muscle = motor_for(f, g, kind, key);
    Word op = (wiring >> (3 * muscle)) & 7;
    Word local_links, far_links;
    if (resting_degree == ~0u) {
        local_links = attached(packed, pos);
        far_links = attached(packed, peer);
    } else {
        local_links = raft_shuffle(resting_degree, pos);
        far_links = raft_shuffle(resting_degree, peer);
    }
    auto event = manipulate(a, b, palm.grain, op, dir, local_links, far_links, reserve, 0);
    if (event.performed) {
        if (op == 3) {
            palm.identity = id_a;
            id_a = 32;
        }
        if (op == 4) {
            id_a = palm.identity;
            palm.identity = 32;
        }
        if (op == 5) {
            id_b = id_a;
            id_a = 32;
        }
        if (op == 6) {
            id_a = id_b;
            id_b = 32;
        }
    }
    if (lane == pos) {
        tile = a;
        identity = id_a;
    }
    if (lane == peer) {
        tile = b;
        identity = id_b;
    }
    return event;
}
struct Harvest {
    Word made = 0, stored = 0, light = 0;
};
AX_FN Word reagent_band(const Coast &coast, Word lane) {
    return coast.catalyst ^ (((lane == coast.second) ^ coast.responsive) ? coast.contrast : 0u);
}
AX_FN Harvest expose(Grain tile, Coast c, Word lane, Word ticks, bool open) {
    tile.charge = 0;
    Half raw = (lane == c.first || lane == c.second) ? 64 : 0, food = 0;
    Word photons = 0, reactions = 0;
    for (Word t = 0; t < ticks; ++t) {
        Word d = (t & 2) ? 2 : 1, axis = d == 2 ? lane >> 3 : lane;
        bool near = ((axis ^ t) & 1) == 0;
        Word facing = near ? d : (d ^ 2), other = next_site(lane, facing);
        Grain peer = unpack(raft_shuffle(pack(tile), other));
        if (near)
            conduct(tile, peer, d, open);
        else
            conduct(peer, tile, d, open);
        auto r = excite(tile, lane == c.sun ? 3 : 4, reagent_band(c, lane), raw, food);
        reactions += r.reaction;
        photons += r.light;
    }
    return {raft_sum(reactions, lane), raft_sum(tile.charge, lane), raft_sum(photons, lane)};
}
AX_FN Word indispensable(Grain tile, Word id, Word lane, Coast c) {
    Grain right = unpack(raft_shuffle(pack(tile), next_site(lane, 1)));
    Grain lower = unpack(raft_shuffle(pack(tile), next_site(lane, 2)));
    Picture bonds{ballot(bonded(tile, right, 1)), ballot(bonded(tile, lower, 2))};
    Word victim = 1u << lane;
    bonds.eastward &= ~(victim | west(victim));
    bonds.southward &= ~(victim | north(victim));
    Word start = 1u << c.sun, end = 1u << (c.responsive ? c.second : c.first);
    Word reach = start & ~victim;
    for (Word t = 0; t < 32; ++t) {
        Word next = propagate(reach, bonds);
        if (__builtin_expect(next == reach, 0))
            break;
        reach = next;
    }
    return __builtin_popcount(ballot(tile.faces && id != lane && !(reach & end)));
}
AX_FN ShoalScore construct(Word seed, Word lane, Coast c, const Construction &p, ShoalOptions o, Word mode,
                           Word allowance, ShoalTrace *trace, Word requested = ~0u, bool override_goal = false) {
    ShoalScore e{};
    e.reserve = allowance;
    e.seed = seed;
    Grain tile = stock(c, seed, lane), initial = tile;
    Word id = tile.faces ? lane : 32, wiring = wire(p.genes, c.tendons);
    Palm palm{};
    Word desire = 0;
    if (fires(p.memory.appetite, color(c.odor, 0), p.genes.release))
        desire |= 1u << c.first;
    if (fires(p.memory.appetite, color(c.odor, 1), p.genes.release))
        desire |= 1u << c.second;
    if (override_goal)
        desire = requested;
    if (trace) {
        trace->before[lane] = pack(initial);
        if (!lane) {
            trace->coast = c;
            trace->memory = p.memory;
            trace->genes = p.genes;
        }
    }
    for (Word move = 0; move < o.moves && e.reserve > 1; ++move) {
        Word k = mix(seed + move * 31469 + 1441), retina = view(tile, c.glass);
        Idea a = decode(proposal(lane, retina, 1u << c.sun, desire, p.memory, p.genes, o.ideas, k, mode, e.candidates));
        if (trace && !lane && move < 32)
            trace->actions[move] = compact(a);
        ++e.trace_size;
        // Looking and moving cost body reserves even when no product follows.
        --e.reserve;
        if (a.kind == 3)
            continue;
        Word muscle = 0, dir = (k >> 20) & 3;
        Word before = ballot(tile.faces != 0);
        auto first = reach(lane, tile, id, palm, p.genes, p.memory, a.kind ? a.kind + 1 : 0, a.from, wiring, dir,
                           e.reserve, k, muscle);
        e.reserve -= first.spent;
        e.work += first.spent;
        e.rotations += first.performed && a.kind != 0;
        if (!a.kind) {
            auto last =
                reach(lane, tile, id, palm, p.genes, p.memory, 1, a.to, wiring, dir, e.reserve, k + 317, muscle);
            e.reserve -= last.spent;
            e.work += last.spent;
        }
        Word after = ballot(tile.faces != 0);
        e.displacements += __builtin_popcount(after & ~before);
    }
    e.parts = __builtin_popcount(ballot(tile.faces && id != lane));
    Word here = view(tile, c.glass), look_e = raft_shuffle(here, next_site(lane, 1)),
         look_s = raft_shuffle(here, next_site(lane, 2));
    Picture projected{ballot(guessed(p.memory, p.genes.release, here, look_e, 1)),
                      ballot(guessed(p.memory, p.genes.release, here, look_s, 2))};
    Word visible = ballot(tile.faces != 0), start = (1u << c.sun) & visible, target = desire & visible;
    for (Word j = 0; j < (mode == 4 ? 0 : 1 + ((p.genes.innate >> 25) & 7)); ++j) {
        start = propagate(start, projected);
        target = propagate(target, projected);
    }
    e.forecast = bool(start & target);
    auto result = expose(tile, c, lane, o.exposure, true);
    e.reactions = result.made;
    e.photons = result.light;
    e.stored = result.stored;
    e.essential = indispensable(tile, id, lane, c) * Word(e.reactions > 0);
    if (mode != 7) {
        e.original = expose(initial, c, lane, o.exposure, true).made;
        e.sealed = expose(tile, c, lane, o.exposure, false).made;
        auto alternative = c;
        alternative.responsive ^= 1;
        e.alternative = expose(tile, alternative, lane, o.exposure, true).made;
    }
    Grain right = unpack(raft_shuffle(pack(tile), next_site(lane, 1))),
          lower = unpack(raft_shuffle(pack(tile), next_site(lane, 2)));
    e.joints = raft_sum(Word(bonded(tile, right, 1)) + Word(bonded(tile, lower, 2)), lane);
    Word count = __builtin_popcount(ballot(tile.faces != 0)) + quantity(palm.grain);
    if (count != Word(__builtin_popcount(c.occupied)) || result.light != result.made + result.stored)
        e.invalid = 1;
    if (trace) {
        trace->after[lane] = pack(tile);
        trace->origin[lane] = id;
    }
    return e;
}
} // namespace axion::life::material::scree

namespace axion::life::material::scree {
AX_FN void resident_eye(Coast &c, const Genes &g) {
    c.tendons = 0xfac688u;
    c.glass = shifted_palette(wild_glass(g));
    c.odor = shifted_palette(wild_odor(g));
    c.responsive = 0;
}
} // namespace axion::life::material::scree
KERNEL void flood_shoal_practice(const material::Genes *pool, const material::Foresight *acquired, Word donors,
                                 material::Construction *out, Word n, Word seed, Word bouts, Word mode) {
    Word i = tid();
    if (i >= n)
        return;
    Word key = mix(seed + i * 5171), owner = mix(key + 771);
    Word donor = mix(key + 317) % donors;
    auto &p = out[i];
    p.genes = pool[donor];
    p.memory = acquired ? acquired[donor] : material::Foresight{};
    material::Coast c;
    material::scree::anatomy(c, owner);
    if (acquired)
        material::scree::resident_eye(c, p.genes);
    if (mode == 1)
        p.memory.contacts = 0;
    if (mode == 2)
        p.memory.muscles[0] = p.memory.muscles[1] = 0;
    if (mode == 3 && acquired)
        p.memory.contacts = rotate(p.memory.contacts, 6);
    if (mode == 6 || mode == 8)
        c.responsive ^= 1;
    material::scree::calibrate(p.memory, p.genes, c, bouts, mix(key + 813), mode);
    if (mode == 9)
        p.memory = {};
    if (mode == 8) {
        c.responsive ^= 1;
        // Two tastings of each of the two deposits. The builder gets no
        // further motor experience or example of how to build at either site.
        for (Word j = 0; j < 4; ++j)
            material::scree::taste(p.memory, p.genes, c, j & 1);
    }
}
KERNEL void flood_shoal_build(const material::Construction *subjects, material::ShoalScore *results, Word n, Word seed,
                              material::ShoalOptions options, Word mode, Word episode, Word requested,
                              material::ShoalTrace *trace, Word resident) {
    Word thread = tid(), i = thread / 32, lane = thread & 31;
    if (i >= n)
        return;
    Word key = mix(seed + i * 5171), layout = mix(key + episode * 19531 + 17939);
    auto coast = material::scree::coast(layout, options.span);
    material::scree::anatomy(coast, mix(key + 771));
    if (resident)
        material::scree::resident_eye(coast, subjects[i].genes);
    auto result = material::scree::construct(layout, lane, coast, subjects[i], options, mode, 16 + options.moves * 12,
                                             i == requested ? trace : nullptr);
    if (!lane)
        results[i] = result;
}

KERNEL void flood_shoal_season(const material::Brood *eggs, material::ShoalScore *records, Word count, Word seed,
                               Word generation, material::ShoalOptions options) {
    Word thread = tid(), i = thread / 32, lane = thread & 31;
    if (i >= count)
        return;
    Word weather = mix(seed ^ mix(generation + 711) ^ mix(i / 32 + 9131)), id = mix(weather + 451 * i);
    Word layout = mix(id + 11501), shore = 20 + options.moves + (mix(id + 551) & 15);
    auto c = material::scree::coast(layout, options.span);
    material::scree::anatomy(c, mix(id + 317));
    material::Construction p{};
    p.genes = eggs[i].genes;
    if (eggs[i].live)
        material::scree::calibrate(p.memory, p.genes, c, options.practice, mix(id + 933), 0);
    Word reserve = eggs[i].live ? 24 + shore : 0;
    auto result = material::scree::construct(layout, lane, c, p, options, 7, reserve, nullptr);
    result.stranded_food = shore;
    if (!lane)
        records[i] = result;
}
KERNEL void flood_shoal_disperse(const material::Brood *eggs, material::Brood *brood,
                                 const material::ShoalScore *records, material::Season *books, Word n, Word seed,
                                 Word gen, Word foster) {
    Word i = tid();
    if (i >= n)
        return;
    auto &r = records[i];
    auto egg = eggs[i];
    material::Season b{};
    Word opening = egg.live * 24 + r.stranded_food, unused = egg.live ? 0 : r.stranded_food;
    Word energy = egg.live ? r.reserve + 2 * r.reactions : 0;
    b.initial = opening + 128;
    b.brightness = r.photons;
    b.heat = opening - unused - r.reserve;
    b.work = r.work;
    b.moves = r.displacements + r.rotations;
    b.product = r.reactions;
    b.residual = unused + energy + 128 - r.reactions + r.stored + (egg.live ? 0 : 2 * r.reactions);
    if (r.invalid)
        ++b.heat;
    Word weather = mix(seed ^ mix(gen + 711) ^ mix(i / 32 + 9131));
    disperse_builders(egg, energy, weather, seed, gen, i, n, foster, brood, books, b);
}

KERNEL void flood_construction_tide(World w, Configuration c, const Word *clock, Word offset) {
    Word tick = *clock + offset;
    if (tick % w.tide.period || !(w.tide.active & 1) || !w.matter.conditions.agency)
        return;
    Word thread = tid(), patch = thread / 32, lane = thread & 31;
    if (thread >= c.side * c.side)
        return;
    // Disjoint patches translate between tides. Each cell belongs to at most
    // one working animal this tick, including both of its contact sites.
    Word phase = tick / w.tide.period, drift = mix(c.seed + phase * 713);
    Word x = (patch % (c.side / 8)) * 8 + (lane & 7) + (drift & 7);
    Word y = (patch / (c.side / 8)) * 4 + (lane >> 3) + ((drift >> 8) & 3);
    Word pos = (x & (c.side - 1)) + (y & (c.side - 1)) * c.side, id = w.slots[pos];
    auto &body = w.animals[id];
    auto &g = w.matter.residents[id].genes;
    Word wired = (g.receptors >> (24 + (w.motor[id] & 7))) & 1;
    Word volunteers = material::scree::ballot(body.energy > 12 && body.body.eyes && body.body.limbs && wired &&
                                              !w.matter.residents[id].hand.faces);
    if (!volunteers)
        return;
    Word key = mix(drift + patch * 5719), eye = material::scree::pick(volunteers, key);
    Word owner = raft_shuffle(id, eye);
    auto &animal = w.animals[owner];
    auto &worker = w.matter.residents[owner];
    auto mind = w.tide.cells[owner];
    material::Grain tile = w.matter.bed[pos];
    Word identity = lane;
    Word glass = material::scree::shifted_palette(material::scree::wild_glass(worker.genes));
    Word odor = (w.land[pos].spore & 3) ^ material::scree::wild_odor(worker.genes);
    Word lit = material::scree::ballot(
        tile.charge || (tile.faces && mineral_sky(pos, tick, c, w.tide.active) == material::light_band(tile)));
    bool desirable = material::scree::fires(mind.appetite, odor, worker.genes.release);
    if (w.worksites.residents && (!w.brine.animals || (w.brine.flowing & 6))) {
        auto cortex = w.worksites.residents[owner];
        Word band = material::catalytic_band(tile);
        bool seen[4];
        for (Word k = 0; k < 4; ++k)
            seen[k] = material::scree::ballot(tile.faces && band == k) != 0;
        desirable = false;
        for (Word k = 0; k < 4; ++k)
            if ((tile.faces ? k == band : seen[k]) &&
                material::scree::fires(cortex.chemistry, 4 * worksite::surface_color(worker.genes, k) + odor,
                                       worker.genes.release))
                desirable = true;
    }
    if (w.brine.animals && !(w.brine.flowing & 6)) {
        auto &cells = w.brine.animals[owner];
        auto weights = brine::efficacy(cells.genes, cells.memory);
        Word total = brine::mass(weights);
        desirable = false;
        for (Word k = 0; k < 4; ++k) {
            bool occurs = material::scree::ballot(tile.faces && material::catalytic_band(tile) == k) != 0;
            if ((tile.faces ? k == material::catalytic_band(tile) : occurs) &&
                brine::impulse(cells.genes, brine::estimate(cells.genes, weights, brine::cue(k, w.land[pos].spore & 3)),
                               total))
                desirable = true;
        }
    }
    Word taste = material::scree::ballot(wet_surface(w.land[pos], pos, tick, c, w.tide.active) < 4 &&
                                         w.land[pos].litter && desirable);
    Word distance = __builtin_abs(int(lane & 7) - int(eye & 7)) + __builtin_abs(int(lane >> 3) - int(eye >> 3));
    Word reach_mask = material::scree::ballot(distance <= 1u + Word(nbits(animal.body.limbs)) / 3);
    Word alternatives = 32u << ((worker.genes.release >> 29) & 1), activity = 0;
    auto a = material::scree::decode(material::scree::proposal(lane, material::scree::view(tile, glass), lit, taste,
                                                               mind, worker.genes, alternatives, key, 0, activity,
                                                               reach_mask, true));
    Word reserve = animal.energy, spent = 1, lifts = 0, sets = 0, turns = 0, shifts = 0, exchanges = 0;
    material::scree::Palm palm{worker.hand, 32};
    if (a.kind != 3) {
        for (Word j = 0; j < (a.kind ? 1u : 2u); ++j) {
            Word at = j ? a.to : a.from, kind = a.kind ? a.kind + 1 : j, dir = (key >> 13) & 3, m = 0;
            if ((!dir && at < 8) || (dir == 1 && (at & 7) == 7) || (dir == 2 && at >= 24) || (dir == 3 && !(at & 7)))
                dir ^= 2;
            auto before = material::scree::unpack(raft_shuffle(material::scree::pack(tile), at));
            Word held = material::quantity(palm.grain);
            auto e =
                material::scree::reach(lane, tile, identity, palm, worker.genes, mind, kind, at, worker.genes.muscle,
                                       dir, reserve - spent, key + j * 317, m, w.matter.attachment[pos]);
            auto after = material::scree::unpack(raft_shuffle(material::scree::pack(tile), at));
            if (c.plastic)
                material::scree::motor_event(mind, worker.genes, m, held, material::quantity(palm.grain), before,
                                             after);
            Word op = material::effector(worker.genes, m);
            spent += e.spent;
            shifts += e.performed && (op == 5 || op == 6);
            exchanges += e.performed && op == 7;
            lifts += e.performed && op == 3;
            sets += e.performed && op == 4;
            turns += e.performed && (op == 1 || op == 2);
        }
    }
    if (w.clay.ground) {
        // The warp already carries the actual origin of each displaced grain.
        auto original = w.clay.ground[pos];
        clay::Tile floor{}, held{};
        for (Word j = 0; j < 2; ++j) {
            Word at = raft_shuffle(original.groove[j], identity & 31),
                 in_hand = raft_shuffle(original.groove[j], palm.identity & 31);
            floor.groove[j] = identity < 32 ? at : 0;
            held.groove[j] = palm.identity < 32 ? in_hand : 0;
        }
        Word date = raft_shuffle(original.changed, identity & 31),
             hand_date = raft_shuffle(original.changed, palm.identity & 31);
        floor.changed = identity < 32 ? date : 0;
        held.changed = palm.identity < 32 ? hand_date : 0;
        w.clay.ground[pos] = floor;
        if (!lane)
            w.clay.palms[owner] = held;
        auto &ledger = w.clay.ledger[c.side >= 128 ? thread / 256 : 0];
        count(ledger.movements, clay::size(floor) && identity != lane);
        if (!lane)
            count(ledger.movements, clay::size(held) != 0);
    }
    w.matter.bed[pos] = tile;
    if (!lane) {
        animal.energy -= static_cast<Half>(spent);
        worker.hand = palm.grain;
        w.tide.cells[owner] = mind;
        auto &l = *w.tide.account;
        count(l.looks, 1);
        count(l.alternatives, activity);
        count(l.lifts, lifts);
        count(l.placements, sets);
        count(l.turns, turns);
        count(l.work, spent);
        count(w.account->heat, spent);
        count(w.matter.ledger->heat, spent);
        count(w.matter.ledger->translations, shifts);
        count(w.matter.ledger->exchanges, exchanges);
        count(w.matter.ledger->lifts, lifts);
        count(w.matter.ledger->placements, sets);
        count(w.matter.ledger->turns, turns);
    }
}

namespace axion::life::material::scree {
AX_FN Word patch_color(const Coast &c, Word type, Word seed) {
    // The two floor patches have persistent visible pigment as well as odor.
    return (mix(seed + type * 701 + 941) & 63) | (color(c.odor, type) << 6);
}
} // namespace axion::life::material::scree
namespace axion::life::material::scree {
AX_FN Word anticipated(Grain tile, Word lane, const Coast &c, const Foresight &mind, const Genes &g) {
    Word retina = view(tile, c.glass);
    Word e = raft_shuffle(retina, next_site(lane, 1)), s = raft_shuffle(retina, next_site(lane, 2));
    Word contacts = fold_contacts(mind.contacts, g.release);
    Picture field{ballot(passes(contacts, retina, e, 1)), ballot(passes(contacts, retina, s, 2))};
    Word active = (1u << c.sun) & ballot(tile.faces != 0);
    for (Word j = 0; j < 2 * (1 + ((g.innate >> 25) & 7)); ++j)
        active = propagate(active, field);
    return active & ((1u << c.first) | (1u << c.second));
}
} // namespace axion::life::material::scree
KERNEL void flood_shore_visit(material::ShoreTray *tray, material::ShoreReply *reply, const grounding::Mind *language,
                              Count utterance, Word action, Word value, Word amount, Word reserve) {
    using namespace material;
    using namespace material::scree;
    Word lane = tid();
    auto &s = *tray;
    auto c = s.coast;
    auto mind = s.memory;
    auto genes = s.genes;
    auto tile = unpack(s.sites[lane]);
    Word id = s.origin[lane], seed = s.seed, turn = s.visits;
    Palm palm{s.hand, s.held};
    Word requested = s.selected;
    if (action == 0) {
        seed = value;
        c = coast(seed, amount);
        anatomy(c, mix(genes.receptors + 911));
        tile = stock(c, seed, lane);
        id = tile.faces ? lane : 32;
        requested = 0;
        turn = 0;
        if (!lane) {
            s.raw[0] = s.raw[1] = 64;
            s.food[0] = s.food[1] = 0;
        }
    }
    if (action == 1)
        calibrate(mind, genes, c, amount, mix(seed + turn * 637), 0);
    if (action == 2)
        for (Word t = 0; t < amount; ++t)
            taste(mind, genes, c, t & 1);
    if (action == 3)
        c.responsive ^= 1;
    ShoreReply answer{};
    if (action == 8 && language) {
        Word form = grounding::recognize(*language, utterance);
        if (form)
            for (Word j = 0; j < 2; ++j) {
                Word pigment = patch_color(c, j, seed), bit = 1u << (j ? c.second : c.first);
                if (grounding::admits(language->word[form].possible, pigment))
                    answer.found |= bit;
                if (grounding::admits(language->word[form].certain, pigment))
                    answer.certain |= bit;
            }
        if (answer.found == answer.certain && __builtin_popcount(answer.found) == 1)
            requested = answer.found;
    }
    if (action == 9)
        requested = 0;
    Word goals = requested;
    if (!goals) {
        if (fires(mind.appetite, color(c.odor, 0), genes.release))
            goals |= 1u << c.first;
        if (fires(mind.appetite, color(c.odor, 1), genes.release))
            goals |= 1u << c.second;
    }
    Word budget = reserve, alternatives = 0;
    if (action == 4 || action == 5 || action == 12) {
        Word count = action == 4 ? 1 : action == 12 ? 2 + ((genes.release >> 9) & 15) : amount;
        for (Word t = 0; t < count && budget > 1; ++t) {
            if (action == 12 && (anticipated(tile, lane, c, mind, genes) & goals))
                break;
            Word k = mix(seed + turn * 31469 + 1441);
            Idea a =
                decode(proposal(lane, view(tile, c.glass), 1u << c.sun, goals, mind, genes, 128, k, 0, alternatives));
            answer.choice = compact(a);
            if (action == 4)
                break;
            --budget;
            ++turn;
            if (a.kind == 3)
                continue;
            Word muscle = 0, wiring = wire(genes, c.tendons);
            for (Word j = 0; j < (a.kind ? 1u : 2u); ++j) {
                Word at = j ? a.to : a.from;
                Grain old = unpack(raft_shuffle(pack(tile), at));
                Word before = quantity(palm.grain);
                auto r = reach(lane, tile, id, palm, genes, mind, a.kind ? a.kind + 1 : j, at, wiring, (k >> 20) & 3,
                               budget, k + 317 * j, muscle);
                auto changed = unpack(raft_shuffle(pack(tile), at));
                // Record observable feedback as in an ordinary tide.
                if (r.performed)
                    motor_event(mind, genes, muscle, before, quantity(palm.grain), old, changed);
                budget -= r.spent;
                answer.acted += r.performed;
            }
        }
    }
    if (action == 6) {
        Half available = lane == c.first ? s.raw[0] : lane == c.second ? s.raw[1] : 0, product = 0;
        Word light = 0;
        for (Word t = 0; t < amount; ++t) {
            Word phase = turn + t, d = phase & 2 ? 2 : 1;
            bool near = (((d == 2 ? lane >> 3 : lane) ^ phase) & 1) == 0;
            Grain peer = unpack(raft_shuffle(pack(tile), next_site(lane, near ? d : d ^ 2)));
            if (near)
                conduct(tile, peer, d, true);
            else
                conduct(peer, tile, d, true);
            light +=
                excite(tile, lane == c.sun ? 3 : 4, Word(lane == c.second) ^ c.responsive, available, product).light;
        }
        Word r0 = raft_shuffle(available, c.first), r1 = raft_shuffle(available, c.second);
        Word p0 = raft_shuffle(product, c.first), p1 = raft_shuffle(product, c.second);
        if (!lane) {
            s.raw[0] = r0;
            s.raw[1] = r1;
            s.food[0] += p0;
            s.food[1] += p1;
        }
        answer.made = (p0 + p1) / 2;
        answer.absorbed = raft_sum(light, lane);
        turn += amount;
    }
    if (action == 11) {
        auto first = c;
        first.responsive = 0;
        auto second = c;
        second.responsive = 1;
        answer.response[0] = expose(tile, first, lane, amount, true).made;
        answer.response[1] = expose(tile, second, lane, amount, true).made;
    }
    if (action == 7 && !lane) {
        for (Word j = 0; j < 2; ++j) {
            Word used = s.food[j] < 65535 - reserve - answer.collected ? s.food[j] : 65535 - reserve - answer.collected;
            answer.collected += used;
            s.food[j] -= used;
        }
    }
    answer.predicts = anticipated(tile, lane, c, mind, genes);
    answer.targets = goals;
    s.sites[lane] = pack(tile);
    s.origin[lane] = id;
    if (!lane) {
        s.coast = c;
        s.memory = mind;
        s.hand = palm.grain;
        s.held = palm.identity;
        s.seed = seed;
        s.visits = turn;
        s.selected = requested;
        answer.cost = reserve - budget;
        answer.first = c.first;
        answer.second = c.second;
        answer.sun = c.sun;
        answer.cargo = palm.grain.faces;
        for (Word j = 0; j < 2; ++j) {
            answer.raw[j] = s.raw[j];
            answer.food[j] = s.food[j];
            answer.pigment[j] = patch_color(c, j, seed);
        }
        *reply = answer;
    }
}
KERNEL void flood_scree_settle(const material::Construction *body, material::SpatialImage *image, Word n) {
    Word i = tid();
    if (i < n)
        image[i] = material::scree::settle(body[i].memory, body[i].genes.release);
}
namespace axion::life::material::scree {
template <bool frozen>
AX_FN void replay_bench(const Construction *body, const SpatialImage *images, Word n, Word calls, Word seed,
                        Word *result) {
    Word thread = tid(), lane = thread & 31, request = thread / 32;
    if (request >= calls)
        return;
    Word k = mix(seed + request * 1777), who = k % n, sensory = mix(k + lane * 1327);
    Word retinal = (sensory & 7) ? 256 | (sensory >> 24) : 0;
    Word right = raft_shuffle(retinal, next_site(lane, 1)), below = raft_shuffle(retinal, next_site(lane, 2));
    auto &subject = body[who];
    Word release = subject.genes.release, motor = (sensory >> 7) & 7, kind = (sensory >> 19) & 3;
    bool east_gate, south_gate, spike, nutrient;
    if constexpr (frozen) {
        auto p = images[who];
        east_gate = passes(p.contacts, retinal, right, 1);
        south_gate = passes(p.contacts, retinal, below, 2);
        spike = (p.motor[kind] >> motor) & 1;
        nutrient = (p.taste >> kind) & 1;
    } else {
        auto &p = subject.memory;
        east_gate = guessed(p, release, retinal, right, 1);
        south_gate = guessed(p, release, retinal, below, 2);
        spike = fires(p.muscles[kind / 2], (kind & 1) * 8 + motor, release);
        nutrient = fires(p.appetite, kind, release);
    }
    Picture field{ballot(east_gate), ballot(south_gate)};
    Word active = 1u << (k & 31);
    for (Word t = 0; t < 6; ++t)
        active = propagate(active, field);
    Word discharge = ballot(spike), hunger = ballot(nutrient);
    if (!lane)
        result[request] = active ^ (discharge + (hunger << 3));
}
} // namespace axion::life::material::scree
#define SCREE_REPLAY_KERNEL(name, frozen)                                                                              \
    KERNEL void name(const material::Construction *body, const material::SpatialImage *images, Word n, Word calls,     \
                     Word seed, Word *result) {                                                                        \
        material::scree::replay_bench<frozen>(body, images, n, calls, seed, result);                                   \
    }
SCREE_REPLAY_KERNEL(flood_scree_mutable, false)
SCREE_REPLAY_KERNEL(flood_scree_frozen, true)
#undef SCREE_REPLAY_KERNEL
