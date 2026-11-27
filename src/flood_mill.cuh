// Included after the native raft shuffles.
#include "axion/mill.hpp"
namespace axion::life::material {
AX_FN Word inverse_motor(Word anatomy, Word op) {
    for (Word i = 0; i < 8; ++i)
        if (((anatomy >> (3 * i)) & 7) == op)
            return i;
    return 8;
}
AX_FN Genes reference() {
    Genes g;
    g.muscle = 0xfac688u;
    g.receptors = 0xff000000u | 4u | (5u << 3) | (6u << 6);
    for (Word s = 0; s < 4; ++s) {
        g.practice |= (s ? s - 1 : 0) << (2 * s);
        g.practice |= (s < 3 ? s + 1 : 3) << (8 + 2 * s);
    }
    g.observation = g.practice | (1u << 17);
    g.release = 12u | (12u << 4) | (3u << 9) | (1u << 12) | (16u << 20);
    return g;
}
AX_FN Rack rack(Word seed, Word span) {
    Rack r;
    r.reach = static_cast<Byte>(span);
    r.socket = static_cast<Byte>(1 + mix(seed + 43) % span);
    r.substrate = 0;
    r.spectrum = 3;
    // A visible bevel identifies the material class. Which contact polarity
    // mates with its rails varies between workshop islands.
    Word bevel = mix(seed + 113) & 1;
    r.bevel = static_cast<Byte>(bevel);
    r.cell[0] = {0xff, 0x37, 0, 0};
    for (Word p = 1; p <= span; ++p)
        r.cell[p] = {0xff, 0x2b, 0, 0};
    r.cell[span + 1] = {0xff, 0x20, 0, 0};
    auto &bar = r.cell[r.socket];
    bar = {0x03, static_cast<Byte>(0x3bu | (bevel << 2)), 0, 0};
    return r;
}
AX_FN void roughen(Rack &r, Word action) {
    // A vertical dipole can be rotated in either direction. Only one
    // horizontal polarity mates with both rails surrounding the socket.
    auto &bar = r.cell[r.socket];
    Word initial = 0x12u;
    bar.faces = static_cast<Byte>(initial);
    bar.marks = static_cast<Byte>(Word(r.bevel) << 2);
    if (action == 1) {
        r.cell[r.socket - 1].faces = static_cast<Byte>((r.cell[r.socket - 1].faces & ~12u) | (2u << 2));
        r.cell[r.socket + 1].faces = static_cast<Byte>((r.cell[r.socket + 1].faces & ~192u) | (1u << 6));
    } else {
        r.cell[r.socket - 1].faces = static_cast<Byte>((r.cell[r.socket - 1].faces & ~12u) | (1u << 2));
        r.cell[r.socket + 1].faces = static_cast<Byte>((r.cell[r.socket + 1].faces & ~192u) | (2u << 6));
    }
    bar.substance = static_cast<Byte>(0x3bu | (Word(r.bevel) << 2));
}
AX_FN Word cue(const Rack &r, const Worker &w) {
    return sense(r.cell[r.socket], r.cell[r.socket + 1], w.hand, 1, (r.cell[r.socket].substance >> 2) & 1);
}
AX_FN Temper adjust(Rack &r, Worker &w, Word muscle, Word wiring, Word reserve, bool teach) {
    Word at = r.socket, links = bonded(r.cell[at - 1], r.cell[at], 1) + bonded(r.cell[at], r.cell[at + 1], 1);
    auto event = manipulate(r.cell[at], r.cell[at + 1], w.hand, (wiring >> (3 * muscle)) & 7, 1, links,
                            Word(bonded(r.cell[at], r.cell[at + 1], 1)), reserve, (r.cell[at].substance >> 2) & 1);
    if (teach)
        practice(w, muscle, event);
    return event;
}
AX_FN MillBook run_rack(Rack &r, Word ticks, Word fuel, bool open = true) {
    MillBook b;
    Half stock = static_cast<Half>(fuel), out = 0;
    for (Word t = 0; t < ticks; ++t) {
        for (Word j = t & 1; j <= r.reach; j += 2)
            b.flow += conduct(r.cell[j], r.cell[j + 1], 1, open);
        for (Word j = 0; j <= 1u + r.reach; ++j) {
            Half inert = 0, discard = 0;
            auto &raw = j == 1u + r.reach ? stock : inert;
            auto &food = j == 1u + r.reach ? out : discard;
            auto v = excite(r.cell[j], j == 0 ? r.spectrum : 4, r.substrate, raw, food);
            b.sun += v.light;
            b.product += v.reaction;
            b.folds += v.folds;
        }
    }
    return b;
}
AX_FN Word tracks(const Rack &r) {
    for (Word j = 0; j <= r.reach; ++j)
        if (!bonded(r.cell[j], r.cell[j + 1], 1))
            return 0;
    return 1;
}
AX_FN Word charge(const Rack &r) {
    Word n = 0;
    for (const auto &g : r.cell)
        n += g.charge;
    return n;
}
AX_FN void play(Worker &w, Word wrenches, Word key, Word turns) {
    // Free practice supplies neither a useful effect nor a target. An animal
    // experiences what its independently connected muscles do to loose grains.
    for (Word i = 0; i < turns; ++i) {
        auto k = mix(key + i * 191);
        auto bench = rack(k, 2);
        roughen(bench, 1 + (k & 1));
        w.hand = {};
        Word muscle = ((i & 7) + (mix(key + i / 8) & 7)) & 7;
        adjust(bench, w, muscle, wrenches, 64, true);
    }
    w.hand = {};
}
AX_FN Word wire(const Genes &g, Word mount) {
    Word ports = 0;
    for (Word i = 0; i < 8; ++i) {
        Word tendon = (g.muscle >> (3 * i)) & 7;
        ports |= ((mount >> (3 * tendon)) & 7) << (3 * i);
    }
    return ports;
}
AX_FN Word observe(Worker &p, const Rack &r, Word flash, Word mode, Word key) {
    if (mode == 1)
        return 0;
    if (mode == 2)
        flash = mix(key ^ 0x1753u) & 255;
    witness(p, cue(r, p), flash);
    return flash != 0;
}
} // namespace axion::life::material
using material::Brood;
KERNEL void flood_mill_found(Brood *eggs, const material::Genes *pool, Word donors, Word n, Word seed, Word origin) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + 7919 * i);
    eggs[i] = {origin == 1   ? material::reference()
               : origin == 2 ? pool[mix(k + 317) % donors]
                             : material::founder(k),
               i, 1};
}
namespace {
AX_FN void disperse_builders(const Brood &egg, Word energy, Word weather, Word seed, Word gen, Word i, Word n,
                             Word foster, Brood *brood, material::Season *books, material::Season book) {
    Word lane = i & 31, island = i / 32;
    Word credits = energy / 24, through = credits;
    for (Word d = 1; d < 32; d *= 2) {
        Word below = raft_shuffle(through, (lane + 32 - d) & 31);
        if (lane >= d)
            through += below;
    }
    Word total = raft_shuffle(through, 31), survivors = total < 32 ? total : 32;
    Word draw = total >= 32 ? (lane * total + (mix(weather + 9143) % total)) / 32 : lane, begin = 0;
    for (Word d = 16; d; d >>= 1) {
        Word last = raft_shuffle(through, begin + d - 1);
        if (last <= draw)
            begin += d;
    }
    Word selected = begin & 31;
    if (foster) {
        Word mask;
        asm volatile("{ .reg .pred keep;setp.ne.u32 keep,%1,0;vote.sync.ballot.b32 %0,keep,0xffffffff; }"
                     : "=r"(mask)
                     : "r"(egg.live));
        Word start = (selected + 1 + mix(weather + 581) % 31) & 31, bits = rotate(mask, (32 - start) & 31);
        if (mask)
            selected = (start + __builtin_ctz(bits)) & 31;
    }
    auto &g = egg.genes;
    material::Genes child{raft_shuffle(g.muscle, selected),      raft_shuffle(g.innate, selected),
                          raft_shuffle(g.receptors, selected),   raft_shuffle(g.practice, selected),
                          raft_shuffle(g.observation, selected), raft_shuffle(g.release, selected)};
    Word family = raft_shuffle(egg.family, selected), viable = lane < survivors;
    child = material::inherit(child, mix(weather + 373 * lane), 28);
    Word wind = mix(seed ^ mix(gen + 857) ^ lane), distance = 32u << ((wind >> 8) % 14),
         to = ((wind & 15) == 0 && distance < n) ? i ^ distance : i;
    brood[to] = {child, family, viable};
    book.eggs = viable;
    book.provision = viable * 24;
    // Solar energy and the initial substrate close exactly over the entire visit.
    for (Word j = 0; j < sizeof(material::Season) / sizeof(Word); ++j) {
        Word v = reinterpret_cast<Word *>(&book)[j];
        Word sum = raft_sum(v, lane);
        if (!lane)
            reinterpret_cast<Word *>(books + island)[j] = sum;
    }
}
} // namespace
KERNEL void flood_mill_season(const Brood *eggs, Brood *brood, material::Season *books, Word n, Word seed, Word gen,
                              Word bouts, Word span, Word foster, Word sight) {
    Word i = tid(), lane = i & 31, island = i / 32;
    if (i >= n)
        return;
    Word weather = mix(seed ^ mix(gen + 711) ^ mix(island + 9131));
    auto egg = eggs[i];
    material::Worker pupil{};
    pupil.genes = egg.genes;
    Word mounting = material::permutation(mix(weather + lane * 17)), wiring = material::wire(pupil.genes, mounting);
    material::play(pupil, wiring, mix(weather + 427 * lane), 64);
    Word energy = egg.live ? 24 : 0;
    material::Season book;
    book.initial = energy;
    for (Word day = 0; day < bouts; ++day) {
        Word k = mix(weather + day * 193 + lane * 7127), paired = mix(weather + day * 193 + (lane >> 1) * 9337);
        Word bevel = (paired >> 8) & 1, turn = 1 + ((bevel ^ (weather >> 18)) & 1);
        // Partners see one another's material effects through an opening in
        // the work surface. The hidden rail polarity changes between islands.
        auto tray = material::rack(paired, span);
        tray.bevel = static_cast<material::Byte>(bevel);
        material::roughen(tray, turn);
        pupil.hand = {};
        Word before_cue = material::cue(tray, pupil);
        Word motor = material::choose(pupil, before_cue, (k >> 15) & 7, k);
        auto event = material::adjust(tray, pupil, motor, wiring, energy, energy != 0);
        auto result = material::run_rack(tray, 24 + 12 * span, 12);
        Word neighbor_flash = raft_shuffle(result.product ? event.change : 0, lane ^ 1);
        bool living = energy != 0;
        if (energy) {
            energy -= event.spent;
            book.work += event.spent;
            book.heat += event.spent;
            Word food = 2 * result.product;
            energy += food;
            if (neighbor_flash && sight != 1) {
                material::witness(pupil, before_cue, sight == 2 ? mix(k ^ 0x1753u) & 255 : neighbor_flash);
                ++book.observations;
            }
            if (result.product && (pupil.genes.observation & (1u << 17)))
                material::witness(pupil, event.cue, event.change);
            Word fee = energy ? 1 : 0;
            energy -= fee;
            book.heat += fee;
        }
        if (!living)
            book.residual += 2 * result.product;
        book.brightness += result.sun;
        book.initial += 12;
        book.product += result.product;
        book.moves += event.performed;
        book.residual += 12 - result.product + material::charge(tray) + pupil.hand.charge;
    }
    book.residual += energy;
    disperse_builders(egg, energy, weather, seed, gen, i, n, foster, brood, books, book);
}
KERNEL void flood_mill_release(const material::Genes *genes, Word donors, World w, Configuration c) {
    Word i = tid();
    if (i < c.side * c.side) {
        auto cargo = w.matter.residents[i].hand;
        w.matter.residents[i] = {};
        w.matter.residents[i].hand = cargo;
        w.matter.residents[i].genes = genes[mix(c.seed ^ mix(i + 829)) % donors];
    }
}
KERNEL void flood_mill_exam(const material::Genes *pool, Word donors, material::Transfer *results, Word n, Word seed,
                            Word evidence, Word trials, Word span, Word mode) {
    Word i = tid();
    if (i >= n)
        return;
    Word base = mix(seed + i * 3179), climate = (base >> 16) & 1, k = base;
    material::Worker p{};
    p.genes = pool[mix(base + 8191) % donors];
    auto g = p.genes;
    Word mount = material::permutation(material::draw(k)), wired = material::wire(g, mount),
         foreign = material::permutation(material::draw(k));
    if (mode != 4)
        material::play(p, wired, material::draw(k), 128);
    material::Worker instructor{};
    instructor.genes = pool[mix(base + 3493) % donors];
    Word instructor_wire = material::wire(instructor.genes, foreign);
    if (mode >= 7) {
        material::play(instructor, instructor_wire, mix(base + 427), 128);
        // This instructor gets its own actual interventions and delayed product.
        // It has no polarity label and must discover useful visible changes.
        for (Word t = 0; t < 256; ++t) {
            Word q = mix(base + t * 617 + 417), bevel = (q >> 13) & 1;
            auto tray = material::rack(q, 2);
            tray.bevel = static_cast<material::Byte>(bevel);
            material::roughen(tray, 1 + (bevel ^ climate));
            instructor.hand = {};
            auto motor = material::choose(instructor, material::cue(tray, instructor), q & 7, q);
            if ((q & 3) == 1)
                motor = (q >> 7) & 7;
            auto e = material::adjust(tray, instructor, motor, instructor_wire, 64, true);
            if (material::run_rack(tray, 48, 12).product && (instructor.genes.observation & (1u << 17)))
                material::witness(instructor, e.cue, e.change);
        }
    }
    for (Word t = 0; t < evidence; ++t) {
        Word q = mix(base + 19 * t + 1149), bevel = t & 1, correct = 1 + (bevel ^ climate);
        auto shown = material::rack(q, 2);
        shown.bevel = static_cast<material::Byte>(bevel);
        material::roughen(shown, correct);
        auto scene = shown;
        material::Worker peer;
        Word routing = mode >= 7 ? instructor_wire : foreign;
        Word m = mode >= 7 ? material::choose(instructor, material::cue(scene, instructor), q & 7, q)
                           : material::inverse_motor(foreign, correct);
        auto e = material::adjust(shown, peer, m, routing, 64, false);
        auto flux = material::run_rack(shown, 48, 12);
        if (mode == 5)
            scene.cell[scene.socket].substance ^= 4;
        if (flux.product)
            material::observe(p, scene, e.change, mode == 8 ? 1 : mode, q);
    }
    material::Transfer ans;
    for (Word t = 0; t < trials; ++t) {
        Word q = mix(base + 197 * t + 21991), bevel = (q >> 23) & 1, correct = 1 + (bevel ^ climate);
        auto original = material::rack(q, span);
        original.bevel = static_cast<material::Byte>(bevel);
        if (mode == 6)
            correct = 1 + ((q >> 27) & 1);
        material::roughen(original, correct);
        auto test = original;
        auto copy = p;
        Word motor = material::choose(copy, material::cue(test, copy), (q >> 11) & 7, q);
        if (mode == 3)
            motor = material::inverse_motor(foreign, correct);
        auto e = material::adjust(test, copy, motor, wired, 64, false);
        ans.moves += e.performed;
        auto insulated = test;
        auto material_yield = material::run_rack(test, 24 + 12 * span, 12);
        auto cut = material::run_rack(insulated, 24 + 12 * span, 12, false);
        ans.product += material_yield.product;
        ans.isolated += cut.product;
        ans.working += material_yield.product > 0;
        ans.matching += ((wired >> (3 * motor)) & 7) == correct;
        material::Worker expert;
        auto direct = original;
        Word tutor_routing = mode >= 7 ? instructor_wire : foreign;
        Word proposal = mode >= 7 ? material::choose(instructor, material::cue(original, instructor), q & 7, q)
                                  : material::inverse_motor(foreign, correct);
        material::adjust(direct, expert, proposal, tutor_routing, 64, false);
        ans.mentor += material::run_rack(direct, 24 + 12 * span, 12).product > 0;
    }
    results[i] = ans;
}
