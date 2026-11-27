// Selection is paid by finite reservoirs. Blind revaluation presents a bare
// vibrating probe and a bath. It never presents the marked tools or their dyes.
namespace axion::life::press {
AX_FN Word deck(Word key) {
    Word d = 228;
    for (Word i = 3; i; --i) {
        Word j = mix(key + 257 * i) % (i + 1);
        Word a = ((d >> (2 * i)) ^ (d >> (2 * j))) & 3;
        d ^= (a << (2 * i)) | (a << (2 * j));
    }
    return d;
}
AX_FN Piece shell(Word hue, Word pitch, Word face, Word seed) {
    Piece p{};
    p.face = face;
    // Give even a closed resonator another, open terminal on its opposite side.
    p.body.faces = (pitch << (2 * (face & 3))) | (3u << (2 * ((face + 2u) & 3u)));
    p.body.substance = (((hue ^ face) & 3) << 4) | (seed & 207);
    return p;
}
AX_FN Piece transfer(const Piece &from, Word cue, Word bare, Word turn, Word seed, Word tick, Record &r) {
    auto face = mirror(from.face, turn);
    Piece to = shell(cue, bare, face, seed);
    bool stamped = cast(to.plate, face, from.plate, from.body, from.face, false, tick);
    r.stamps += stamped;
    r.copies += stamped && bool(clay::groove(from.plate, from.face) & 128);
    return to;
}
AX_FN void valuation(Mind &m, Word gates, Word key, Word intervention, Record &r) {
    if (intervention == outcome_closed)
        return;
    for (Word cycle = 0; cycle < 3; ++cycle) {
        Word order = mix(key + cycle * 7937);
        for (Word k = 0; k < 16; ++k) {
            Word turn = (k * ((order >> 4) | 1) + order) & 15, bath = turn >> 2, tone = turn & 3;
            Word available = 9 + (mix(order + k) & 7);
            Word food = flow(tone, item(gates, bath), 32, available);
            Word heard_bath = intervention == other_bath ? (bath + 1) & 3 : bath;
            r.taste_edits += taste(m, heard_bath, tone, food != 0);
            ++r.tastings;
        }
    }
}
AX_FN void forge(Piece (&tools)[4], Piece (&siblings)[4], Word seed, Record &r) {
    Word hidden = deck(mix(seed + 2071));
    Word hosts = deck(mix(seed + 815)), raw = deck(mix(seed + 4123));
    for (Word i = 0; i < 4; ++i) {
        Word k = mix(seed + i * 971);
        auto master = shell(item(hosts, i), item(hidden, i), (k >> 8) & 7, k);
        auto first = transfer(master, (k >> 21) & 3, (k >> 18) & 3, k & 3, mix(k + 71), 1, r);
        auto second = transfer(first, (k >> 15) & 3, (k >> 26) & 3, (k >> 7) & 3, mix(k + 81), 2, r);
        tools[i] = transfer(second, i, item(raw, i), (k >> 11) & 3, mix(k + 129), 3, r);
        // A sister impression is demonstrated dry. Its shape and resonance
        // survive the original and two intermediate carriers in the other branch.
        siblings[i] = transfer(first, i, (k >> 25) & 3, (k >> 17) & 3, mix(k + 397), 2, r);
    }
}
AX_FN Record lifetime(Word gene, Word seed, Word intervention, Specimen *certificate) {
    Record r{};
    Mind m{};
    express(m, gene);
    Word hidden = deck(mix(seed + 2071)), old = deck(mix(seed + 9137)), next = 0;
    Word shift = 1 + mix(seed + 7351) % 3;
    for (Word b = 0; b < 4; ++b)
        next |= ((item(old, b) + shift) & 3) << (2 * b);
    Piece tray[4]{}, siblings[4]{};
    forge(tray, siblings, seed, r);
    for (Word i = 0; i < 4; ++i) {
        const auto &sister = siblings[i];
        for (Word j = 0; j < 3 && intervention != sight_closed; ++j) {
            Word audible = resonance(sister.plate, sister.body, sister.face);
            if (intervention == stepped_sounds)
                audible = (audible + 1) & 3;
            bool observer =
                intervention == mixed_contacts ? bool(mix(seed + i * 2491 + j * 671) & 1) : intervention != self_taught;
            r.sensory_edits += see(m, appearance(sister.body, sister.face), audible, observer);
            ++r.demonstrations;
        }
    }
    valuation(m, old, mix(seed + 8247), intervention, r);
    Word prior = m.banks >> 16;
    if (intervention != before_change && intervention != unmoved)
        valuation(m, next, mix(seed + 317), intervention, r);
    if (intervention == erased_forecast)
        m.charge &= 65535;
    if (intervention == erased_taste)
        m.banks &= 65535;
    if (certificate) {
        certificate->gene = gene;
        certificate->surface = m.charge >> 16;
        certificate->appetite = m.banks >> 16;
        certificate->prior_appetite = prior;
        certificate->baths = old;
        certificate->shifted = next;
        certificate->pitches = hidden;
    }
    for (Word i = 0; i < 4; ++i) {
        if (intervention == smoothed)
            tray[i].plate = {};
        if (certificate) {
            certificate->casts[i] = clay::groove(tray[i].plate, tray[i].face);
            certificate->skins[i] = tray[i].body.substance;
            certificate->faces[i] = tray[i].face;
            certificate->raw_tips |= material::face(tray[i].body, tray[i].face & 3) << (2 * i);
        }
    }
    // Each bath is offered the formerly productive and the newly productive
    // tool, twice in opposite orders with the same tie-breaking impulse.
    for (Word b = 0; b < 4; ++b) {
        Word before = 0, after = 0;
        for (Word i = 0; i < 4; ++i) {
            Word tone = item(hidden, i);
            if (tone == item(old, b))
                before = i;
            if (tone == item(next, b))
                after = i;
        }
        Word gate = item(intervention == unmoved ? old : next, b), both = mix(seed + b * 2281);
        for (Word reverse = 0; reverse < 2; ++reverse) {
            Word a = reverse ? after : before, z = reverse ? before : after;
            Word left = appearance(tray[a].body, tray[a].face), right = appearance(tray[z].body, tray[z].face);
            Word choice = choose(m, left, right, b, both, intervention != unguided) ? z : a;
            auto &tool = tray[choice];
            Word tone = resonance(tool.plate, tool.body, tool.face);
            Word stock = 24 + ((both >> 16) & 15), food = flow(tone, gate, 64, stock);
            r.stocked += stock;
            r.residual += stock - food;
            ++r.trials;
            ++r.heat;
            r.food += food;
            r.edible += food != 0;
            r.old_choice += choice == before;
            r.ties += drive(m, left, b) == drive(m, right, b);
            if (certificate) {
                Word j = 2 * b + reverse;
                certificate->decisions |= choice << (2 * j);
                certificate->outcomes |= Word(food != 0) << j;
            }
        }
    }
    return r;
}
} // namespace axion::life::press
using namespace axion::life::press;
KERNEL void press_probe(const Word *laws, Word n, Word tasks, Word seed, Word intervention, Record *out,
                        Specimen *first) {
    Word i = tid();
    if (i >= tasks)
        return;
    Word key = mix(seed + i * 12347);
    Specimen detail{};
    out[i] = lifetime(laws[mix(key + 479) % n], key, intervention, i ? nullptr : &detail);
    if (!i)
        *first = detail;
}
KERNEL void press_hatchery(const Family *young, Family *adult, Word n, Word seed, Word generation, Word practice) {
    Word i = tid();
    if (i >= n)
        return;
    auto f = young[i];
    if (f.gene == ~0u) {
        adult[i] = {~0u, 0};
        return;
    }
    // Four independent beds pay the same actual cost for eight blind first
    // choices apiece. Any realized surplus can provision eggs.
    Word reserve = 32;
    for (Word j = 0; j < 4; ++j) {
        auto r = lifetime(f.gene, mix(seed + generation * 11939 + i * 7211 + j * 79),
                          practice ? mixed_contacts : revised, nullptr);
        reserve = reserve - r.heat + r.food;
    }
    adult[i] = {f.gene, reserve / 16};
}
AX_FN Word pressure_child(Word germ, Word key) {
    Word edit = mix(key + 15173);
    return (edit & 255) < 36 ? germ ^ (1u << ((edit >> 8) % 22)) : germ;
}
KERNEL void press_offspring(const Family *adult, Family *young, Word n, Word seed, Word generation, Word paid) {
    Word i = tid();
    if (i >= n)
        return;
    if (paid == 2) {
        if (i >= n / 2)
            return;
        // A bijection assigns each adult to one pair. These two balances
        // cannot fund births elsewhere in this generation.
        Word rotation = mix(seed + generation * 21019), stride = mix(rotation + 259) | 1u;
        Word x = (rotation + 2 * i * stride) & (n - 1), y = (x + stride) & (n - 1);
        Word eggs[2]{adult[x].eggs, adult[y].eggs};
        for (Word j = 0; j < 2; ++j) {
            Word kid = 2 * i + j, key = mix(seed + kid * 1069 + generation * 76261), remaining = eggs[0] + eggs[1];
            if (!remaining) {
                young[kid] = {~0u, 0};
                continue;
            }
            Word parent = Word(mix(key + 1897) % remaining >= eggs[0]);
            --eggs[parent];
            young[kid] = {pressure_child(adult[parent ? y : x].gene, key), 1};
        }
        return;
    }
    Word key = mix(seed + i * 1069 + generation * 76261);
    Word a = mix(key + 289) & (n - 1), b = mix(key + 501) & (n - 1);
    Word whole = adult[a].eggs + adult[b].eggs;
    bool second = paid && whole ? mix(key + 1897) % whole >= adult[a].eggs : bool(mix(key + 1897) & 1);
    young[i] = {pressure_child(adult[second ? b : a].gene, key), 0};
}
KERNEL void press_images(const Mind *m, Frozen *f, Word n) {
    Word i = tid();
    if (i < n)
        f[i] = compile(m[i]);
}
template <class T> AX_FN Word choices(const T &c, Word key) {
    Word result = 0;
    for (Word j = 0; j < 8; ++j) {
        Word k = mix(key + j * 859);
        Word a = k & 3, b = (k >> 3) & 3, bath = (k >> 7) & 3;
        if constexpr (sizeof(T) == sizeof(Frozen))
            result |= choose(c, a, b, bath, k >> 16) << j;
        else
            result |= choose(c, a, b, bath, k >> 16, true) << j;
    }
    return result;
}
KERNEL void press_writable(const Mind *m, Word n, Word calls, Word seed, Word *out) {
    Word i = tid();
    if (i < calls)
        out[i] = choices(m[i % n], mix(seed + i));
}
KERNEL void press_ranked(const Frozen *m, Word n, Word calls, Word seed, Word *out) {
    Word i = tid();
    if (i < calls)
        out[i] = choices(m[i % n], mix(seed + i));
}
KERNEL void press_differences(const Word *a, const Word *b, Word *diff, Word n) {
    Word i = tid();
    if (i < n && a[i] != b[i])
        asm volatile("atom.global.add.u32 %0, [%1], 1;" : "=r"(i) : "l"(diff) : "memory");
}
KERNEL void press_scholars(const clay::Mind *brains, Word count, Word seed, Word *scores) {
    Word id = tid();
    if (id >= count)
        return;
    Word score = 0;
    for (Word j = 0; j < 8; ++j) {
        press::Specimen d{};
        auto r = press::lifetime(press::germ(brains[id]), mix(seed + j * 7793), press::revised, &d);
        auto m = brains[id];
        m.charge = d.surface << 16;
        m.banks = d.appetite << 16;
        score += r.edible;
        for (Word cue = 0; cue < 4; ++cue)
            score += press::predictions(m, cue) == (1u << press::item(d.pitches, cue));
        for (Word bath = 0; bath < 4; ++bath)
            score += press::values(m, bath) == (1u << press::item(d.shifted, bath));
    }
    scores[id] = score;
}
