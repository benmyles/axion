// Each animal senses the same crop sequence. In self-directed trials the
// yoked animal still chooses its own movements. Only the imitation control
// receives a copy of the intact motor history.
namespace {
AX_FN Word balanced_crop(Word key) {
    Word table = 0xe4e4u;
    for (Word i = 7; i > 0; --i) {
        key = mix(key + 1709);
        Word j = key % (i + 1), delta = ((table >> (2 * i)) ^ (table >> (2 * j))) & 3;
        table ^= (delta << (2 * i)) | (delta << (2 * j));
    }
    return table | (key & 0xffff0000u);
}
AX_FN Word spectrum(Word cue, Word key, bool held) {
    Word high = key & 15;
    return cue | (high << 3) | ((Word(__builtin_popcount(high) & 1) ^ Word(held)) << 7);
}
AX_FN Word pod_probes(const pods::Circuit &b, const pods::Genes &g, Word law, Word seed) {
    Word correct = 0;
    for (Word j = 0; j < 64; ++j) {
        Word cue = j & 7, key = mix(seed + 1907 * j), light = spectrum(cue, key, true);
        correct += pods::choose(b, g, light, (key >> 13) & 7, key) == pods::aperture(cue, law);
    }
    return correct;
}
} // namespace
KERNEL void flood_pod_assay(const pods::Genes *donors, Word pool, pods::Audit *results, Word subjects, Word rounds,
                            Word seed, pods::Physics physics, Word forced) {
    Word i = tid(), origin = mix(seed + 451897 * i), donor = mix(origin + 31) % pool;
    pods::Genes g = donors[donor];
    pods::Circuit intact{}, yoked{}, shifted{}, demonstrated{}, empty{};
    pods::Gut gut{}, shuffle_gut{}, foreign_gut{};
    pods::Audit out;
    Word law = balanced_crop(origin), first_law = law;
    out.before = pod_probes(intact, g, law, origin);
    for (Word phase = 0; phase < 2; ++phase) {
        if (phase) {
            Word change = 1 + (mix(origin + 127) % 3);
            law = (balanced_crop(mix(origin + 419)) & 0xffff0000u) | ((first_law ^ (change * 0x5555u)) & 65535);
            out.surprise = pod_probes(intact, g, law, origin);
        }
        for (Word t = 0; t < rounds; ++t) {
            Word tick = t + phase * rounds, key = mix(origin ^ mix(tick + 819871)), cue = (key >> 18) & 7;
            auto light = spectrum(cue, mix(key + 23), false);
            Word basal = (key >> 9) & 7;
            Word motor[3]{}, idx = pods::receptor(light, g);
            pods::Circuit *circuit[]{&intact, &yoked, &shifted};
            pods::Gut *meal[]{&gut, &foreign_gut, &shuffle_gut};
            auto donor = pods::mature(gut, tick), other = pods::mature(shuffle_gut, tick);
            pods::mature(foreign_gut, tick);
            Word signal = donor.edible ? 2 : donor.waste ? 1 : 0, foreign;
            asm volatile("shfl.sync.bfly.b32 %0,%1,1,31,0xffffffff;" : "=r"(foreign) : "r"(signal));
            for (Word arm = 0; arm < 3; ++arm) {
                motor[arm] = forced ? (mix(key + 151) & 3) : pods::choose(*circuit[arm], g, light, basal, key);
            }
            if (signal) {
                pods::associate(intact, g, signal == 2, 1, key);
                ++out.delivered;
            }
            if ((physics.background & 16) && !signal)
                pods::omission(intact, g);
            if ((physics.background & 16) && !foreign) {
                pods::omission(yoked, g);
                pods::omission(demonstrated, g);
            }
            if ((physics.background & 16) && !other.edible && !other.waste)
                pods::omission(shifted, g);
            if (foreign) {
                pods::associate(yoked, g, foreign == 2, 1, key);
                pods::associate(demonstrated, g, foreign == 2, 1, key);
            }
            if (other.edible || other.waste)
                pods::associate(shifted, g, other.edible != 0, 2, key);
            for (Word arm = 0; arm < 3; ++arm) {
                Word muscle = motor[arm];
                bool pending = pods::occupied(*meal[arm]);
                bool offered = (key & 7) != 1;
                bool bite = offered && (forced || !(physics.background & 2) ||
                                        pods::strike(*circuit[arm], g, idx, muscle, pending));
                if (bite) {
                    bool opened = muscle == pods::aperture(cue, law);
                    Word delay = pods::latency(cue, law, physics), due = (tick + delay) & 7;
                    if (!arm) {
                        out.coincident += bool(gut.chaff[due] || gut.food[1][due]);
                        ++out.grasps;
                        out.opened += opened;
                        out.alone += !pending;
                    }
                    pods::swallow(*meal[arm], tick, delay, 1, 12, opened ? 12 : 0);
                } else if (!arm)
                    out.held += offered;
                pods::remember(*circuit[arm], idx, muscle, bite);
                if (!arm)
                    pods::remember(demonstrated, idx, muscle, bite);
            }
        }
        out.shadow[phase] = pod_probes(demonstrated, g, law, origin);
        out.live[phase] = pod_probes(intact, g, law, origin);
        out.yoked[phase] = pod_probes(yoked, g, law, origin);
        out.cleared[phase] = pod_probes(empty, g, law, origin);
        out.displaced[phase] = pod_probes(shifted, g, law, origin);
    }
    if (i < subjects)
        results[i] = out;
}
KERNEL void flood_pod_local(World w, Configuration c, const Word *clock, pods::LocalAudit *out) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    auto slot = w.slots[i];
    auto &animal = w.animals[slot];
    pods::LocalAudit a{};
    if (animal.energy) {
        auto &p = w.pods;
        const auto &organ = p.animal[slot];
        Word tick = *clock;
        Word here = pod_reach(i, animal, w.dna[slot], w, c, tick),
             district = (here % c.side) / 8 + 128 * ((here / c.side) / 8);
        const auto &s = w.land[here];
        Word cue = pods::light(s.spore, p.skin[here].winding, optical(s, w.animals[w.slots[here]].energy));
        Word target = pods::aperture(cue, pods::key_for(c.seed, district, tick, p.physics));
        Word key = mix(c.seed + tick * 1499 + slot * 0x9e3779b9u), somatic = animal.action;
        pods::Circuit empty{};
        bool framed = (pods::sensory_channels(p.physics,animal.cohort) & 4) != 0;
        Word live =
            pods::pulse(organ.brain, organ.genes, organ.frame, organ.frame_genes, cue, somatic, key, framed).physical;
        Word cold = pods::pulse(empty, organ.genes, {}, organ.frame_genes, cue, somatic, key, framed).physical;
        a.alive = 1;
        a.arrived = organ.brain.history != 0;
        a.live = live == target;
        a.cold = cold == target;
        a.changed = live != cold;
        a.gained = live == target && cold != target;
        a.lost = live != target && cold == target;
    }
    out[i] = a;
}
