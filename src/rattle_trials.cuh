#include "axion/rattle_trials.hpp"
namespace axion::life::rattle {
namespace {
AX_FN Word smaller(Word a, Word b) { return a < b ? a : b; }
AX_FN Drum instruments(Word k, Genes a, Genes b, bool wild, bool resonators = false) {
    // Standardized strikers expose the same two non-silent impulses to every reader.
    // Their public identities and the phase of the connecting levers are counterbalanced.
    Word tag = k & 3, other = (tag + 1 + ((k >> 2) % 3)) & 3;
    if (!wild) {
        a.drum = 0x9999u | (tag << 16);
        b.drum = 0x9999u | (other << 16);
    }
    return {{a, b}, (k >> 8) & 3, resonators ? tag | (other << 2) : ((a.drum >> 16) & 3) | (((b.drum >> 16) & 3) << 2)};
}
struct Inlets {
    Word wave[2], stock[2], up;
};
AX_FN Inlets fill(Drum drums, Word faithful, Word bank0, Word sensory0, Word sensory1, Word drive) {
    Inlets p{};
    Word feel[]{sensory0, sensory1};
    p.up = bank0 ^ faithful;
    for (Word source = 0; source < 2; ++source) {
        Word loc = bank0 ^ source;
        Word raw = knock(drums.source[source], feel[source], (drive >> (9 + source)) & 1, drive >> (18 + source));
        if (raw != silence)
            raw = 2 * ((drums.tones >> (2 * source)) & 3) + (raw & 1);
        p.wave[loc] = conduct(raw, drums.polarity >> source);
    }
    // One spring feeds a reachable pan. The other striker touches a separate pool.
    // Both local knocks are made before either pan is drawn down.
    p.stock[p.up ^ Word(!feel[faithful])] = 6;
    return p;
}
AX_FN void listen(Inlets &p, Word arm, bool probe) {
    for (Word loc = 0; loc < 2; ++loc) {
        if (arm == deaf || arm == chorus_deaf ||
            (probe && (arm == cut_upstream || arm == cut_sidepool) && ((loc == p.up) == (arm == cut_upstream))))
            p.wave[loc] = silence;
        else if ((arm == anonymous || arm == chorus_plain) && p.wave[loc] != silence)
            p.wave[loc] &= 1;
    }
}
AX_FN Word question(const Mind &m, Drum d, Word order, Word drive, Word arm, bool across) {
    Word a = (d.tones >> (2 * order)) & 3, b = (d.tones >> (2 * (order ^ 1))) & 3;
    Word request = invite(m, a, b, drive, arm != old_attention, across);
    if (arm == away)
        request ^= 1;
    if (arm == lottery)
        request = mix(drive + 3457) & 1;
    return request;
}
AX_FN Word season(Mind &m, Drum d, Word reliable, Word key, Word exposure, Word phase, Word arm, Trial::Season *out,
                  Word twin, bool across, Word *share = nullptr) {
    Word collected = 0, early = 0, late = 0, rewritten = 0, switched = 0, heard = 0, consulted = 0,
         window = smaller(8, exposure);
    for (Word t = 0; t < exposure; ++t) {
        Word k = mix(key + t * 1879u + phase * 2971u), drive = mix(k + 6029);
        auto p = fill(d, reliable, (k >> 11) & 1, (k >> 3) & 1, (k >> 7) & 1, k);
        listen(p, arm, false);
        if (one_ear(arm)) {
            Word request = question(m, d, (k >> 11) & 1, drive, arm, across);
            p.wave[request ^ 1] = silence;
            consulted += request == p.up;
            if (arm == quiet_choice)
                p.wave[request] = silence;
        }
        Word choice = pick(m, p.wave[0], p.wave[1], drive), pay = smaller(capacity(2, 2), p.stock[choice]);
        if (share && pay && p.wave[choice] != silence) {
            // One hooked end keeps two particles of the six it draws. The animal
            // on the opposite end receives the remaining four. Each carbon is counted once.
            pay -= 2;
            share[choice ^ ((k >> 11) & 1)] += 2;
            p.stock[choice] -= 2;
        }
        p.stock[choice] -= pay;
        collected += pay;
        early += (t < window && pay);
        late += (t + window >= exposure && pay);
        heard += p.wave[choice] != silence;
        Word tick = phase * 8192u + t * 12u;
        taste(m, p.wave[choice], p.wave[choice ^ 1], arm == mistimed ? 6 * ((drive >> 14) & 1) : pay, tick, across);
        // Each pulse has its own due time. Everything the chooser knew at the pull
        // is latched until the gut fires, including which bank of contacts was active.
        Word diff = digest(m, tick + latency(m.genes),
                           arm != locked && arm != chorus_locked && (phase == 0 || arm != first_only));
        rewritten += (diff >> 1) & 1;
        switched += diff >> 2;
    }
    if (!out)
        return collected;
    out->food[twin] = collected;
    out->first[twin] = early;
    out->last[twin] = late;
    out->edits[twin] = rewritten;
    out->turns[twin] = switched;
    out->heard[twin] = heard;
    out->consulted[twin] = consulted;
    out->contact[twin] = m.contacts;
    out->state[twin] = m.trace;
    for (Word q = 0; q < probes; ++q) {
        Word roll = mix(key + phase * 5659u + q * 2659u + 8039);
        // Exhaust the two binary local loads and exchange the strikers' places.
        auto p = fill(d, reliable, q >> 2, q & 1, (q >> 1) & 1, roll);
        listen(p, arm, true);
        if (one_ear(arm)) {
            Word request = question(m, d, q >> 2, roll, arm, across);
            out->requests[twin] |= request << q;
            p.wave[request ^ 1] = silence;
            if (arm == quiet_choice)
                p.wave[request] = silence;
        }
        Word choice = pick(m, p.wave[0], p.wave[1], roll);
        out->goals[twin] |= Word(p.stock[1] != 0) << q;
        out->choices[twin] |= choice << q;
        out->packets[twin][q >> 2] |= (p.wave[0] | (p.wave[1] << 4)) << (8 * (q & 3));
    }
    return collected;
}
} // namespace
KERNEL void rattle_examine(const Mind *people, Word size, Word cases, Word seed, Word exposure, Word arm, Word across,
                           const Genes *visitors, Word guests, Trial *out) {
    Word i = tid();
    if (i >= cases)
        return;
    Word key = mix(seed + i * 7949u), informative = (key >> 17) & 1;
    auto g = people[mix(key + 1657) % size].genes;
    auto striker = [&](Word k) { return guests ? visitors[k % guests] : people[k % size].genes; };
    Drum d = instruments(mix(key + 5119), striker(mix(key + 7129)), striker(mix(key + 8867)),
                         arm == wild_drums || arm >= chorus, arm >= chorus);
    Trial t{};
    t.genes[0] = g.drum;
    t.genes[1] = g.ear;
    t.genes[2] = g.synapse;
    t.genes[3] = g.latch;
    t.upstream = informative;
    t.orientation = d.polarity;
    t.tags = d.tones;
    t.drums[0] = d.source[0].drum;
    t.drums[1] = d.source[1].drum;
    for (Word twin = 0; twin < 2; ++twin) {
        Mind m{g};
        for (Word s = 0; s < seasons; ++s)
            season(m, d, informative ^ twin ^ (s & 1), key, exposure, s, arm, &t.season[s], twin, across);
    }
    out[i] = t;
}
KERNEL void rattle_families(Family *p, Word pop, Word seed, const Genes *stock, Word n) {
    Word i = tid();
    if (i >= pop)
        return;
    Word k = mix(seed + i * 2767u);
    p[i] = {n ? stock[k % n] : founder(k), 0};
}
KERNEL void rattle_foragers(Family *p, Word n, Word seed, Word epoch, Word scarce, Word across) {
    Word i = tid();
    if (i >= n)
        return;
    // Members of a brood encounter the same two weather sequences in different
    // districts. Only recovered carbon buys entries in the next local brood.
    Word food = 0;
    if (scarce < 2) {
        for (Word j = 0; j < 2; ++j) {
            Word k = mix(seed + epoch * 9361u + j * 4271u);
            auto d = instruments(mix(k + 1607), {}, {}, false);
            Mind m{p[i].genes};
            for (Word s = 0; s < seasons; ++s)
                food += season(m, d, ((k >> 13) ^ s) & 1, k, 24, s, scarce ? invited : open, nullptr, 0, across);
        }
    } else {
        // Three independent neighbours play the three roles. Each home germ is
        // judged only by what its own body would swallow as puller or as a gripper.
        // A shared permutation puts exactly the same triad together for all three
        // accounting passes. No mailbox, atomic wallet or external fitness label.
        Word off1 = 1 + mix(seed + epoch * 2789u) % (n - 1), off2 = 1 + mix(seed + epoch * 4931u) % (n - 1);
        if (off1 == off2)
            off2 = 1 + off2 % (n - 1);
        Word members[]{i, (i + n - off1) % n, (i + n - off2) % n};
        for (Word role = 0; role < 3; ++role) {
            Word hunter = members[role], k = mix(seed + epoch * 9361u + hunter * 4271u);
            auto d = instruments(mix(k + 1607), p[(hunter + off1) % n].genes, p[(hunter + off2) % n].genes, true, true);
            Mind m{p[hunter].genes};
            Word rent[2]{}, own = 0;
            for (Word s = 0; s < seasons; ++s)
                own += season(m, d, ((k >> 13) ^ s) & 1, k, 24, s, scarce == 3 ? chorus_invited : chorus, nullptr, 0,
                              across, rent);
            food += role ? rent[role - 1] : own;
        }
    }
    p[i].food = food;
}
KERNEL void rattle_spawn(const Family *p, Family *next, Word n, Word seed, Word epoch, Word paid) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + epoch * 8849u + i * 5881u), best = mix(k + 4133) % n;
    for (Word j = 0; j < 7 && paid; ++j) {
        Word candidate = mix(k + 6907 + j * 1693) % n;
        if (p[candidate].food > p[best].food)
            best = candidate;
    }
    next[i] = {inherit(p[best].genes, mix(k + 2129), 72), 0};
}
KERNEL void rattle_immigrate(Mind *minds, Word n, const Family *stock, Word size, Word seed) {
    Word i = tid();
    if (i < n)
        minds[i] = {stock[mix(seed + i * 7591) % size].genes};
}
} // namespace axion::life::rattle

namespace axion::life::rattle {
KERNEL void rattle_compile(const Mind *m, Word n, Word *compiled) {
    Word i = tid();
    if (i < n)
        compiled[i] = image(m[i]);
}
KERNEL void rattle_read(const Mind *m, const Word *compiled, Word n, Word calls, Word stored, Word *out) {
    Word i = tid();
    if (i >= calls)
        return;
    Word id = i % n, drive = mix(i + 6491), a = drive % 9, b = (drive >> 8) % 9;
    out[i] = stored ? dispatch(compiled[id], a, b, drive) : pick(m[id], a, b, drive);
}
} // namespace axion::life::rattle
