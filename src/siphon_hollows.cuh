#include "axion/hollow_words.hpp"
namespace axion::life::hollow {
AX_FN Word pack_pos(int x, int y) { return (Word(x) & 255) | ((Word(y) & 255) << 8); }
AX_FN int l1(int x, int y) { return __builtin_abs(x) + __builtin_abs(y); }
AX_FN Ground clearing(Word seed) {
    Ground g{};
    g.seed = seed;
    g.bath = mix(seed + 853) & 3;
    g.old_gate = press::item(press::deck(mix(seed + 9137)), g.bath);
    g.gate = (g.old_gate + 1 + mix(seed + 457) % 3) & 3;
    Word plots[6]{0, 1, 2, 3, 4, 5};
    for (Word i = 5; i > 0; --i) {
        Word j = mix(seed + 1169 + i * 7919) % (i + 1);
        Word temp = plots[i];
        plots[i] = plots[j];
        plots[j] = temp;
    }
    for (Word i = 0; i < 6; ++i) {
        Word k = mix(seed + i * 1279);
        // Six separate physical plots, shuffled and rotated independently
        // of the chemical and lexical permutations used in the lesson.
        Word ring = plots[i];
        constexpr int east[]{0, 1, 1, 0, -1, -1}, south[]{-1, -1, 1, 1, 1, -1};
        k = mix(seed + i * 951);
        int x = east[ring] * int(4 + (k & 3)) + int((k >> 12) % 3) - 1;
        int y = south[ring] * int(4 + ((k >> 2) & 3)) + int((k >> 15) % 3) - 1;
        for (Word j = 0; j < ((seed >> 9) & 3); ++j) {
            int t = x;
            x = -y;
            y = t;
        }
        Word tint = i < 4 ? i : i == 4 ? g.bath : ((k >> 20) & 3);
        Word relief = (((i + (seed >> 4)) & 15) << 2) | tint;
        g.sites[i] = {static_cast<short>(x), static_cast<short>(y), static_cast<Byte>(tint), static_cast<Byte>(relief)};
    }
    return g;
}
AX_FN Pieces place_pieces(const Ground &g, bool exchanged = false) {
    Pieces stock{};
    for (Word i = 0; i < 4; ++i) {
        Word hue = exchanged ? (i + 1) & 3 : i;
        stock.x[hue] = g.sites[i].x;
        stock.y[hue] = g.sites[i].y;
    }
    return stock;
}
AX_FN Word within_reach(const Pieces &stock, const Path &p) {
    Word seen = 0;
    for (Word hue = 0; hue < 4; ++hue)
        if ((stock.loose & (1u << hue)) && stock.x[hue] == p.x && stock.y[hue] == p.y)
            seen |= 1u << hue;
    return seen;
}
AX_FN void lay_down(Pieces &ware, Word &held, const Path &p) {
    if (held >= 4)
        return;
    ware.x[held] = p.x;
    ware.y[held] = p.y;
    ware.loose |= 1u << held;
    held = 4;
}
AX_FN void motion(Path &b, Mind &m, Word dir, Word count, Word mode, bool sensed = true) {
    Word rotation = (dir - b.head) & 3;
    if (sensed)
        turn(m, rotation);
    b.head = dir;
    b.x += int(count) * (int(dir == 1) - int(dir == 3));
    b.y += int(count) * (int(dir == 2) - int(dir == 0));
    if (sensed && (mode & dead_reckoning)) {
        // An uninterrupted straight segment may sum its identical pulses.
        Word direction = (m.yaw + (m.genes.compass & 3) + Word(bool(mode & distorted))) & 3;
        m.x = Byte(m.x + int(count) * (int(direction == 1) - int(direction == 3)));
        m.y = Byte(m.y + int(count) * (int(direction == 2) - int(direction == 0)));
    }
    b.steps += count;
}
AX_FN void lead(Path &b, Mind &m, int x, int y, Word key, Word mode, bool sensed = true) {
    for (Word j = 0; j < 2; ++j) {
        bool horizontal = ((j ^ key) & 1) != 0;
        int d = horizontal ? x - b.x : y - b.y;
        if (d)
            motion(b, m, horizontal ? (d > 0 ? 1u : 3u) : (d > 0 ? 2u : 0u), __builtin_abs(d), mode, sensed);
    }
}
AX_FN void look(Mind &m, Slots slots, const Site &site, Word cue, Word now, Word seed, Word mode, bool taste) {
    // A sight has a texture and a local appearance, with no position field.
    observe(m, slots, cue, site.relief, taste, now, seed, mode);
}
AX_FN void water_lesson(clay::Mind &p, Word bay, Word valve, Word key) {
    for (Word t = 0; t < 12; ++t) {
        Word tone = (t * (1 + 2 * ((key >> 8) & 1)) + key) & 3;
        bool food = press::flow(tone, valve, 32, 16) != 0;
        press::taste(p, bay, tone, food);
    }
}
AX_FN void hear_sisters(clay::Mind &p, Word seed) {
    Word tones = press::deck(mix(seed + 2071));
    for (Word turn = 0; turn < 3; ++turn)
        for (Word hue = 0; hue < 4; ++hue) {
            // Dry sisters sound at a different workbench. Their positions
            // have no bearing on the sites of the four working copies.
            Word tone = press::item(tones, hue);
            auto pupil = press::shell(hue, (tone + 1) & 3, 0, seed + hue);
            auto donor = press::shell((hue + 1) & 3, tone, 0, mix(seed + hue));
            press::cast(pupil.plate, 0, donor.plate, donor.body, 0, false, 1);
            press::see(p, press::appearance(pupil.body, 0), press::resonance(pupil.plate, pupil.body, 0), true);
        }
}
AX_FN void survey_sites(Mind &m, Slots cells, const Ground &g, Path &body, Word physics, Word first = 0, Word end = 6,
                        const Pieces *ware = nullptr) {
    Word seed = g.seed;
    Word order = mix(seed + 679);
    for (Word t = first; t < end; ++t) {
        Word i = (t * (order & 1 ? 1u : 5u) + (order >> 8) % 6) % 6;
        const auto &site = g.sites[i];
        lead(body, m, site.x, site.y, mix(order + t * 701), physics);
        if (!ware || i >= 4)
            look(m, cells, site,
                 i < 4    ? 4 * implement + i
                 : i == 4 ? 4 * inlet + g.bath
                          : 4 * stone + site.tint,
                 body.steps, order + t, physics, false);
        if (ware)
            for (Word seen = within_reach(*ware, body); seen; seen &= seen - 1)
                look(m, cells, site, 4 * implement + __builtin_ctz(seen), body.steps, order + t, physics, false);
    }
}
AX_FN void course(Mind &m, Slots cells, clay::Mind &p, const Ground &g, Path &body, Word mode, Word &cost, Word &fix) {
    Word seed = g.seed, physics = changing | dead_reckoning | anchoring;
    if (mode == no_motion)
        physics &= ~Word(dead_reckoning);
    if (mode == skewed_motion)
        physics |= distorted;
    if (mode == no_landmark)
        physics &= ~Word(anchoring);
    body.x = int((seed >> 20) & 3) - 2;
    body.y = int((seed >> 24) & 3) - 1;
    body.head = (seed >> 16) & 3;
    Word order = mix(seed + 679);
    survey_sites(m, cells, g, body, physics);
    // All sensory interventions get the same physical tour. The dry bench
    // lies outside the ring of working copies; blind water is met at the inlet.
    lead(body, m, 10 + int((seed >> 5) & 3), -10 - int((seed >> 14) & 3), seed + 313, physics);
    if (mode != no_sisters)
        hear_sisters(p, seed);
    lead(body, m, g.sites[4].x, g.sites[4].y, seed + 511, physics);
    if (mode != no_taste) {
        water_lesson(p, g.bath, g.old_gate, mix(seed + 123));
        if (mode != old_water && mode != same_water)
            water_lesson(p, g.bath, g.gate, mix(seed + 491));
    }
    if (mode != open_route && mode != same_water) {
        const auto &rock = g.sites[5];
        int dx = ((seed >> 6) & 1) ? 4 : -4, dy = ((seed >> 7) & 1) ? 3 : -3;
        lead(body, m, rock.x + dx, rock.y + dy, order + 759, physics);
        // A raft translates a stationary body. Its old heading is retained.
        body.x = rock.x;
        body.y = rock.y;
        Word relief = mode == false_landmark ? g.sites[0].relief : rock.relief;
        fix = orient(m, cells, relief, physics);
    }
    int x = int((seed >> 1) % 5) - 2, y = int((seed >> 11) % 5) - 2;
    lead(body, m, x, y, mix(seed + 913), physics);
    if (mode == no_positions)
        m.used = 0;
    cost = body.steps;
}
AX_FN void search(Trip &t, Mind &m, Slots cells, clay::Mind &eye, const Ground &g, Pieces &ware, Path &body, Word gate,
                  Word &stock, Word &carried, Word budget, Word mode, Proof *proof) {
    Word valued_tools = best_tools(eye, g.bath), turns = 0, first = 4;
    Word tones = press::deck(mix(g.seed + 2071));
    for (Word step = 0; step < budget; ++step) {
        Word k = mix(g.seed + step * 437);
        Word state = 2 * Word(carried < 4), kind = (m.genes.retrieval >> (24 + 2 * state)) & 3;
        Word cues = (kind == implement ? valued_tools : kind == inlet ? 1u << g.bath : 15u) << (4 * kind);
        auto r = recall(m, cells, cues, body.steps, k);
        Word accessible = within_reach(ware, body);
        if (accessible && carried == 4 && r.range == 0 && kind == implement) {
            Word preferred = accessible & valued_tools;
            carried = __builtin_ctz(preferred ? preferred : accessible);
            ware.loose &= ~(1u << carried);
            first = carried;
            ++t.lifted;
            t.crumbs = 4;
            t.retrieved = press::item(tones, carried) == gate;
            ++t.work;
            continue;
        }
        if (body.x == g.sites[4].x && body.y == g.sites[4].y && carried < 4 && kind == inlet && !r.range) {
            ++t.deposits;
            ++t.work;
            t.food = press::flow(press::item(tones, carried), gate, 96, stock < 36 ? stock : 36);
            stock -= t.food;
            press::see(eye, carried, press::item(tones, carried), false);
            press::taste(eye, g.bath, press::item(tones, carried), t.food != 0);
            lay_down(ware, carried, body);
            t.delivered = t.food != 0;
            break;
        }
        if (r.range && r.compass < 4) {
            if (!turns)
                t.straight = r.range;
            ++turns;
        }
        Word direction = (body.head + (r.compass < 4 ? r.compass : k & 3)) & 3;
        motion(body, m, direction, 1,
               mode == no_motion       ? 0u
               : mode == skewed_motion ? dead_reckoning | distorted
                                       : dead_reckoning);
        ++t.work;
        if (proof && step < 32)
            proof->track[step] =
                pack_pos(body.x, body.y) | (Word(carried < 4) << 16) | ((r.slot == missing ? 63u : r.slot) << 18);
    }
    t.chosen = first < 4;
    t.lost = !t.delivered;
    t.pressured = t.deposits;
    if (proof) {
        proof->chosen = first;
        proof->finish = body;
        proof->visits = t.deposits;
    }
}
AX_FN Trip forage(Genes genes, Word optics, Word arena, Word seed, Word mode, Proof *proof) {
    Trip t{};
    t.attempts = 1;
    Word marks[limit]{};
    Mind m{genes};
    Slots cells{marks, 1, mode == one_cell ? 1u : arena};
    clay::Mind eye{};
    press::express(eye, optics);
    auto g = clearing(seed);
    Path body{};
    Word pleasure = 0, corrected = 0;
    course(m, cells, eye, g, body, mode, pleasure, corrected);
    Word tones = press::deck(mix(seed + 2071)), gate = mode == same_water ? g.old_gate : g.gate;
    t.corrections = corrected;
    t.cells = m.used;
    t.body_steps = pleasure;
    for (Word j = 0; j < m.used; ++j)
        t.remembered += bool(strength(cells.at(j)));
    if (proof) {
        proof->seed = seed;
        proof->genome[0] = genes.synapse;
        proof->genome[1] = genes.compass;
        proof->genome[2] = genes.retrieval;
        proof->genome[3] = genes.hands;
        proof->eye = optics;
        proof->bath = g.bath;
        proof->old_gate = g.old_gate;
        proof->gate = gate;
        proof->tones = tones;
        proof->forecast = eye.charge >> 16;
        proof->water = eye.banks >> 16;
        proof->start = body;
        proof->cortex = m;
        proof->course = mode;
        for (Word i = 0; i < 6; ++i)
            proof->sites[i] = g.sites[i];
        for (Word i = 0; i < m.used; ++i)
            proof->memory[i] = cells.at(i);
    }
    Word carried = 4, stock = 36;
    auto ware = place_pieces(g, mode == rearranged);
    search(t, m, cells, eye, g, ware, body, gate, stock, carried, 48, mode, proof);
    return t;
}
} // namespace axion::life::hollow
KERNEL void hollow_probe(const hollow::Genes *laws, Word ng, Word optics, Word arena, Word n, Word seed, Word mode,
                         hollow::Trip *out, hollow::Proof *proof) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 2357);
    if (!i)
        *proof = {};
    out[i] = hollow::forage(laws[mix(k + 313) % ng], optics, arena, k, mode, i ? nullptr : proof);
}
KERNEL void hollow_lives(hollow::Family *families, Word n, Word optics, Word arena, Word season, Word seed) {
    Word i = tid();
    if (i >= n)
        return;
    auto &f = families[i];
    Word energy = 16;
    for (Word j = 0; j < 4; ++j) {
        Word key = mix(seed + season * 62467 + i * 6947 + j * 1753);
        auto r = hollow::forage(f.genes, optics, arena, key, j < 2 ? hollow::open_route : hollow::refound, nullptr);
        // Common rations fund demonstration and floor time. Extracted carbon
        // and finite residues at an actual pickup fund the offspring allowance.
        energy += r.food + r.crumbs;
    }
    f.allowance = energy / 8;
}
KERNEL void hollow_births(const hollow::Family *adults, hollow::Family *brood, Word n, Word seed, Word season,
                          Word earned) {
    Word i = tid();
    if (i >= n / 2)
        return;
    Word key = mix(seed + season * 53017), link = mix(key + 113) | 1u;
    Word x = (key + 2 * i * link) & (n - 1), y = (x + link) & (n - 1);
    Word wallet[2]{adults[x].allowance, adults[y].allowance};
    for (Word j = 0; j < 2; ++j) {
        Word k = mix(key + i * 7319 + j * 127);
        Word side = earned ? Word((k % (wallet[0] + wallet[1])) >= wallet[0]) : k & 1;
        if (earned)
            --wallet[side];
        brood[2 * i + j] = {hollow::child(adults[side ? y : x].genes, mix(k + 2911), 40), 0};
    }
}

KERNEL void hollow_candidates(const hollow::Genes *laws, Word n, Word eye, Word arena, Word seed, Word *points) {
    Word i = tid();
    if (i >= n)
        return;
    Word merit = 0;
    for (Word j = 0; j < 12; ++j) {
        auto t = hollow::forage(laws[i], eye, arena, mix(seed + j * 6481), hollow::refound, nullptr);
        merit += 128 * t.delivered + (t.delivered ? 48 - t.work : 0) + t.corrections;
    }
    points[i] = merit;
}
#include "axion/hollow_image.hpp"
KERNEL void hollow_indices(hollow::Field field, hollow::Index *image, const Word *owners, Word n, Word tick, Word tie) {
    Word i = tid();
    if (i < n) {
        Word id = owners[i];
        image[i] = hollow::index(field.minds[id], hollow::cells(field, id), tick, mix(id + tie));
    }
}
template <bool fast>
AX_FN void place_decision(hollow::Field field, const hollow::Index *image, const Word *owners, Word n, Word tick,
                          Word tie, Word calls, Word seed, Count *out) {
    Word i = tid();
    if (i >= calls)
        return;
    Word k = mix(seed + i * 3253), at = (i / 8) % n, id = owners[at], key = mix(id + tie);
    auto m = field.minds[id];
    auto slots = hollow::cells(field, id);
    auto r = fast ? hollow::indexed(m, slots, image[at], k & 65535, key, k >> 28)
                  : hollow::recall(m, slots, k & 65535, tick, key, k >> 28);
    out[i] = (r.slot == hollow::missing ? 63u : r.slot) | (r.bearings << 6) | (r.cue << 10) | (r.compass << 15) |
             (Count(r.range) << 18) | (Count(r.goal) << 27);
}
#define PLACE_CALL(language, fast)                                                                                     \
    KERNEL void language(hollow::Field f, const hollow::Index *img, const Word *ids, Word n, Word tick, Word tie,      \
                         Word calls, Word seed, Count *out) {                                                          \
        place_decision<fast>(f, img, ids, n, tick, tie, calls, seed, out);                                             \
    }
PLACE_CALL(hollow_online, false)
PLACE_CALL(hollow_indexed, true)
#undef PLACE_CALL
KERNEL void hollow_compare(const Count *a, const Count *b, Word n, Word *errors) {
    Word i = tid();
    if (i < n && a[i] != b[i])
        asm volatile("red.global.add.u32 [%0], 1;" ::"l"(errors) : "memory");
}
