#include "axion/braid_trials.hpp"
namespace axion::life::braid {
struct Tradition {
    unsigned char law[voices]{};
};
AX_FN Tradition dialect(Word seed) {
    Tradition t{};
    Word pool = (1u << 23) - 1;
    for (Word w = 0; w < voices; ++w) {
        Word left = pool, count = mix(seed + w * 2579) % __builtin_popcount(pool);
        while (count--) {
            left &= left - 1;
        }
        Word pick = __builtin_ctz(left);
        pool ^= 1u << pick;
        t.law[w] = assembly(1 + pick);
    }
    return t;
}
AX_FN Motors mechanics(const Genes &g, Word rig, Word bands) {
    Motors m{};
    m.present = 15;
    for (Word j = 0; j < 4; ++j)
        m.table[j] = valve(order, j, g, rig, bands);
    return m;
}
AX_FN Word realize(Word state, Plan p, Genes g, Word rig, Word strands) {
    for (Word i = 0; i < p.size; ++i)
        state = valve(state, (p.stream >> (3 * i)) & 7, g, rig, strands);
    return state;
}
AX_FN Word instruction(const Tradition &t, const Phrase &p, Word start) {
    for (Word j = 0; j < p.n; ++j)
        start = transfer(start, t.law[p.cell[j]]);
    return start;
}
AX_FN Word focal(Word k, Word n) { return 1 + mix(k + 7901) % (n - 1); }
AX_FN Phrase appeal(Word k, Word n, Word length) {
    auto p = phrase(mix(k + 2713), length);
    for (Word i = 0; i < length; ++i)
        p.cell[i] %= n;
    p.cell[length / 2] = focal(k, n);
    return p;
}
AX_FN Word shade(Word k, Word turn) {
    // Some trays carry four distinct dyes. The others have naturally duplicated pigments.
    return ((k >> 29) & 1) ? (mix(k + turn * 2237) & 255) : assembly(mix(k + turn * 2237) % 24);
}
AX_FN Word drill(Mind &m, Word rig, Word repeats, Word k, Word cut, bool varied = true) {
    Word edits = 0;
    for (Word r = 0; r < repeats; ++r)
        for (Word j = 0; j < motors; ++j) {
            Word col = varied ? shade(k + r * 9427, j) : order;
            Word end = valve(col, j, m.genes, rig, 15);
            if (cut != untrained && cut != static_cells)
                edits += motor_touch(m, j, col, end, false);
        }
    return edits;
}
struct School {
    Word episodes = 0, strokes = 0, edits = 0, singles = 0, orders = 0;
};
AX_FN School attend(Mind &m, const Tradition &t, Word n, Word repeats, Word draw, Word arm, Word solitary, Word focus,
                    const Phrase *map = nullptr, Word grammar = 0, bool backwards = false) {
    School s{};
    Genes teacher{};
    teacher.muscle = mix(draw + 1061);
    Word rig = (mix(draw + 3527) & 7);
    auto hands = mechanics(teacher, rig, 15);
    for (Word w = 0; w < n; ++w)
        for (Word r = 0; r < repeats; ++r) {
            Word dice = mix(draw + w * 6511 + r * 5003);
            Phrase p{};
            p.cell[0] = w;
            p.n = 1;
            if (!solitary && w && (!grammar || w >= 2)) {
                Word anchor = (dice >> 8) % w;
                if (r & 1) {
                    p.cell[1] = w;
                    p.cell[0] = anchor;
                } else
                    p.cell[1] = anchor;
                p.n = 2;
                if (r % 3 == 2) {
                    p.cell[2] = (dice >> 19) % w;
                    p.n = 3;
                }
            }
            if (arm == no_anchor && !w)
                continue;
            Word start = shade(dice, r), want = instruction(t, p, start);
            auto acted = route(hands, start, want, 15, 7);
            if (!acted.found)
                continue;
            Word end = realize(start, acted, teacher, rig, 15);
            s.episodes++;
            s.strokes += acted.size;
            s.singles += p.n == 1 && p.cell[0] == focus;
            if (arm == obscured)
                end = start;
            if (arm == unpaired)
                for (Word i = 0; i < p.n; ++i)
                    p.cell[i] = (p.cell[i] + 1) % n;
            p = sequence(p, backwards);
            if (map)
                for (Word i = 0; i < p.n; ++i)
                    p.cell[i] = map->cell[p.cell[i]];
            if (grammar && arm != static_cells && arm != direct)
                s.orders += order_contact(m, p, start, end);
            if (arm != static_cells && (arm != direct || p.n == 1))
                s.edits += listen(m, p, start, end);
            if (grammar && w == 1 && r + 1 == repeats) {
                // A reference with known constituents can distinguish two speech orders.
                // In the commuting control the repeated reference has the same effect either way.
                for (Word v = 0; v < repeats; ++v) {
                    Phrase pair = phrase((grammar == 2 ? 0 : 1) << 3, 2);
                    Word col = assembly(mix(draw + v * 1867u) % 24), goal = instruction(t, pair, col);
                    auto pull = route(hands, col, goal, 15, 7);
                    if (!pull.found)
                        continue;
                    Word effect = realize(col, pull, teacher, rig, 15);
                    auto heard = sequence(pair, backwards);
                    if (map)
                        for (Word z = 0; z < 2; ++z)
                            heard.cell[z] = map->cell[heard.cell[z]];
                    ++s.episodes;
                    s.strokes += pull.size;
                    if (arm != direct && arm != static_cells)
                        s.orders += order_contact(m, heard, col, arm == obscured ? col : effect);
                }
            }
        }
    return s;
}
AX_FN Word new_rig(Word k, Word coupled) {
    Word v = mix(k + 2797);
    // A complete ring of sequential couplings can generate an even-only
    // subgroup. Retain experimental cases where a physical target is unreachable.
    return (v & 7) | ((coupled ? ((v >> 8) & 15) : 0) << 3);
}
AX_FN Plan planning(const Mind &m, Word start, Word wanted, Word visible, Word bound) {
    return route(release(m, false), start, wanted, visible, bound);
}
AX_FN void train_brood(Family &family, Word key, Word n, Word independent) {
    Word food = 0;
    for (Word z = 0; z < 2; ++z) {
        Word k = mix(key + z * 9491), rig = new_rig(k, z), focus = focal(k, n);
        bool reverse = independent && ((key ^ z) & 1);
        auto code = dialect(k);
        Mind m{family.genes};
        drill(m, rig, 3, k + 3109, learned);
        for (Word lesson = 0; lesson < 3; ++lesson) {
            if (lesson) {
                for (Word j = motors; j < cells; ++j)
                    m.pruned[j] = 0;
                m.lived &= 15;
                attend(m, code, n, 3, k + 2437, learned, lesson == 1, focus, nullptr, independent, reverse);
            }
            for (Word test = 0; test < 4; ++test) {
                Word t = mix(k + test * 7841 + lesson * 1319), start = assembly(t % 24);
                auto p = appeal(t, n, lesson == 2 ? 4 : 1);
                Word goal = instruction(code, p, start), wanted = goal, mask = 15;
                p = sequence(p, reverse);
                if (lesson) {
                    auto inferred = predict(pathway(m, p), start);
                    wanted = inferred.colors;
                    mask = inferred.known;
                }
                auto act = planning(m, start, wanted, mask, depth(m.genes));
                Word end = realize(start, act, m.genes, rig, 15);
                food += harvest(end, goal, 8).swallowed;
            }
        }
    }
    family.food = food;
}
KERNEL void braid_stock(Family *p, Word count, Word salt, const Genes *bank, Word source) {
    Word i = tid();
    if (i < count) {
        Word k = mix(salt + i * 4507u);
        p[i] = {source ? bank[k % source] : founder(k), 0};
    }
}
KERNEL void braid_select(Family *p, Word count, Word epoch, Word salt, Word n, Word independent) {
    Word i = tid();
    if (i < count)
        train_brood(p[i], mix(salt + epoch * 25733u), n, independent);
}
KERNEL void braid_spawn(const Family *p, Family *next, Word count, Word epoch, Word salt, Word paid) {
    Word i = tid();
    if (i >= count)
        return;
    Word k = mix(salt + epoch * 7057u + i * 1877u), best = mix(k) % count;
    for (Word j = 0; j < 7 && paid; ++j) {
        Word alt = mix(k + j * 3761 + 59) % count;
        if (p[alt].food > p[best].food)
            best = alt;
    }
    next[i] = {inherit(p[best].genes, mix(k + 977), 64), 0};
}
KERNEL void braid_rehouse(Mind *m, Word n, const Family *families, Word count, Word seed) {
    Word i = tid();
    if (i < n)
        m[i] = {families[mix(seed + i * 1579u) % count].genes};
}
KERNEL void braid_probe(const Genes *parents, Word np, Word count, Word seed, Word rep, Word n, Word length,
                        Word coupled, Word arm, Word grammar, Trial *trials) {
    Word i = tid();
    if (i >= count)
        return;
    Word key = mix(seed + i * 9319), focus = focal(key, n);
    if (grammar && n > 2)
        focus = 2 + mix(key + 7901) % (n - 2);
    auto base = dialect(key);
    auto g = parents[mix(key + 5371) % np];
    Word rig = new_rig(key, coupled), scene = assembly(mix(key + 9149) % 24);
    Phrase query = appeal(key, n, arm == one_word ? 1 : length);
    query.cell[query.n / 2] = focus;
    bool inverse = grammar && ((key >> 15) & 1);
    Trial record{};
    record.convention = inverse;
    record.world = key;
    record.germ = g;
    record.rig = rig;
    record.start = scene;
    record.length = query.n;
    record.utterance = phrase_code(sequence(query, inverse));
    record.missing = focus;
    for (Word j = 0; j < n; ++j)
        record.teacher[j] = base.law[j];
    for (Word twin = 0; twin < 2; ++twin) {
        Tradition law = base;
        if (twin)
            law.law[focus] = exchanged(law.law[focus], (key >> 7) & 3, (((key >> 7) & 3) + 1) & 3);
        Mind m{g};
        auto school = attend(m, law, n, rep, key + 6091, arm, 0, focus, nullptr, grammar, inverse);
        Word practices = drill(m, rig, rep, key + 2503, arm);
        auto &t = record.twin[twin];
        t.before = scene;
        t.station_mask = arm == untied ? 7 : 15;
        t.wanted = instruction(law, query, scene);
        auto image = predict(pathway(m, sequence(query, inverse)), scene);
        t.orientation = reversed(m);
        t.order_edits = school.orders;
        t.imagined = image.colors;
        t.known = image.known;
        t.exposures = school.episodes;
        t.tuition = school.strokes;
        t.writes = school.edits;
        t.seen = school.singles;
        t.muscle_writes = practices;
        for (Word j = 0; j < n; ++j)
            t.lexicon[j] = matrix(m, motors + j);
        for (Word j = 0; j < 4; ++j)
            t.muscles[j] = matrix(m, j);
        auto plan = planning(m, scene, image.colors, image.known, depth(g));
        t.plan = plan.stream;
        t.n = plan.size;
        t.found = plan.found;
        t.after = realize(scene, plan, g, arm == mirror_hand ? rig ^ 1 : rig, t.station_mask);
        t.correct = (arm == lex_only) ? (image.known == 15 && image.colors == t.wanted) : t.after == t.wanted;
        t.food = harvest(t.after, t.wanted, 8).swallowed;
        t.cost = plan.size;
    }
    trials[i] = record;
}
} // namespace axion::life::braid

namespace axion::life::braid {
KERNEL void braid_images(const Mind *m, Word size, Atlas *images) {
    Word ix = tid(), id = ix >> 5, lane = ix & 31;
    if (id >= size || lane >= 24)
        return;
    if (!lane)
        images[id].input = pack(m[id]);
    images[id].movements[lane] = packed_plan(planning(m[id], order, assembly(lane), 15, depth(m[id].genes)));
}
KERNEL void braid_prepare(Mind *m, Word size, Word n, Word seed) {
    Word id = tid();
    if (id >= size)
        return;
    Word k = mix(seed + id * 6197), rig = new_rig(k, 0);
    drill(m[id], rig, 4, k + 6571, learned);
    attend(m[id], dialect(k), n, 4, k + 2347, learned, 0, 1);
}
KERNEL void braid_read(const Mind *m, const Atlas *images, Word size, Word calls, Word terms, Word stored, Word *out) {
    Word i = tid();
    if (i >= calls)
        return;
    Word h = mix(i + 887), id = i % size, col = (h & 3) ? assembly((h >> 7) % 24) : ((h >> 15) & 255);
    auto p = appeal(h, terms, 1 + ((h >> 24) & 3));
    if (stored && bijective(col)) {
        const auto &im = images[id];
        p = sequence(p, (im.input.known_words >> 16) & 1);
        Word applied = order, enabled = 1;
        for (Word j = 0; j < p.n; ++j) {
            Word word = p.cell[j];
            applied = transfer(applied, im.input.words[word]);
            enabled &= (im.input.known_words >> word) & 1;
        }
        if (enabled) {
            out[i] = im.movements[address_of(applied)] | 0x80000000u;
            return;
        }
        p = sequence(p, (im.input.known_words >> 16) & 1);
    }
    auto prediction = predict(pathway(m[id], p), col);
    out[i] = packed_plan(planning(m[id], col, prediction.colors, prediction.known, depth(m[id].genes)));
}
} // namespace axion::life::braid
