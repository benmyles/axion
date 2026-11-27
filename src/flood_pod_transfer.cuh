namespace {
struct ForagingLife {
    pods::Circuit mind{};
    pods::Frame frame{};
    pods::Gut gut{};
};
AX_FN Word valence(pods::Delivery value) { return value.edible ? 2 : value.waste ? 1 : 0; }
AX_FN void crop_experience(ForagingLife &animal, pods::Inheritance genes, pods::Physics p, Word raw, Word law,
                           Word time, Word key, Word signal, bool forced, bool observe = true,
                           pods::TrayReading *tray = nullptr) {
    auto &b = animal.mind;
    auto &f = animal.frame;
    bool framed = p.background & 4;
    auto motion = pods::pulse(b, genes.law, f, genes.frame, raw, (key >> 13) & 7, key, framed,
                              forced ? (mix(key + 1411) & 3) : 4);
    if ((p.background & 16) && observe && !signal)
        pods::omission(b, genes.law);
    if (observe && signal) {
        if (framed)
            pods::reframe(f, genes.frame, genes.law, b, signal == 2, key, 0);
        pods::associate(b, genes.law, signal == 2, 1, key);
    }
    bool grip = (key & 7) != 1 && (forced || !(p.background & 2) ||
                                   pods::strike(b, genes.law, motion.cue, motion.physical, pods::occupied(animal.gut)));
    if (grip) {
        bool opened = motion.physical == pods::aperture(raw, law);
        if (tray) {
            ++tray->bites;
            tray->opened += opened;
        }
        pods::swallow(animal.gut, time, pods::latency(raw, law, p), 1, 12, opened ? 12 : 0);
    }
    if (tray) {
        tray->light = raw;
        tray->muscle = motion.physical;
    }
    if (framed)
        pods::perceive(f, genes.frame, motion, grip);
    pods::remember(b, motion.cue, motion.native, grip);
}
} // namespace
KERNEL void flood_pod_transfer(const pods::Inheritance *pool, Word donors, pods::Transfer *results, Word n,
                               Word training, Word demonstration, Word witnesses, Word seed, Word forced,
                               pods::Physics physics) {
    Word i = tid(), origin = mix(seed + i * 0x9e3779b9u), base = balanced_crop(mix(origin + 1351));
    auto g = pool[mix(origin + 1613) % donors];
    ForagingLife before;
    for (Word t = 0; t < training; ++t) {
        Word key = mix(origin ^ mix(14149 + t)), cue = (key >> 17) & 7;
        auto receipt = pods::mature(before.gut, t);
        crop_experience(before, g, physics, spectrum(cue, key, false), base, t, key, valence(receipt), forced);
    }
    auto live = before, yoked = before, silent = before;
    Word first = (origin >> 17) & 7, seen = 0, prior_arms = 0, angle = 1 + mix(origin + 7451) % 3;
    Word changed = pods::rotation(base, angle) | (mix(origin + 2311) & 0xffff0000u),
         foil = balanced_crop(mix(origin + 7919));
    for (Word j = 0; j < witnesses; ++j) {
        Word cue = (first + j * 3) & 7;
        seen |= 1u << cue;
        prior_arms |= 1u << pods::aperture(cue, base);
        Word bits = 3u << (2 * cue);
        foil = (foil & ~bits) | (changed & bits);
    }
    for (Word j = 0; j < demonstration; ++j) {
        Word t = training + j, key = mix(origin ^ mix(23209 + t)), cue = (first + 3 * (mix(key + 11) % witnesses)) & 7;
        Word signal = valence(pods::mature(live.gut, t)), foreign;
        asm volatile("shfl.sync.bfly.b32 %0,%1,1,31,0xffffffff;" : "=r"(foreign) : "r"(signal));
        pods::mature(yoked.gut, t);
        pods::mature(silent.gut, t);
        Word raw = spectrum(cue, key, false);
        crop_experience(live, g, physics, raw, changed, t, key, signal, forced);
        crop_experience(yoked, g, physics, raw, changed, t, key, foreign, forced);
        crop_experience(silent, g, physics, raw, changed, t, key, 0, forced, false);
    }
    pods::Transfer a;
    Word upper[4]{};
    bool framed = physics.background & 4;
    // Move only the inferred common motor frame into the pre-storm animal.
    auto transplant = before;
    transplant.frame.active = live.frame.active;
    for (Word j = 0; j < 64; ++j) {
        Word cue = j & 7;
        if (seen & (1u << cue))
            continue;
        Word key = mix(origin ^ mix(92771 + j)), raw = spectrum(cue, key, true), basal = (key >> 13) & 7;
        auto probe = [&](const ForagingLife &life, Word clamp) {
            return pods::pulse(life.mind, g.law, life.frame, g.frame, raw, basal, key, framed, 4, clamp).physical;
        };
        Word prior = probe(before, 4), after = probe(live, 4), target = pods::aperture(cue, changed);
        ++a.unseen;
        a.initial += prior == pods::aperture(cue, base);
        a.before += prior == target;
        a.live += after == target;
        a.yoked += probe(yoked, 4) == target;
        a.silent += probe(silent, 4) == target;
        a.clamped += probe(live, 0) == target;
        a.transplant += probe(transplant, 4) == target;
        a.foil += after == pods::aperture(cue, foil);
        if (!(prior_arms & (1u << pods::aperture(cue, base)))) {
            ++a.other;
            a.distant += after == target;
        }
        for (Word f = 0; f < 4; ++f)
            upper[f] += probe(live, f) == target;
    }
    for (Word v : upper)
        if (v > a.avail)
            a.avail = v;
    a.changed = before.frame.active != live.frame.active;
    a.anchored = __builtin_popcount(live.frame.known);
    if (i < n)
        results[i] = a;
}

// The stage is an external physical apparatus. Only the surface and actual
// delayed digestion enter the animal. A spoken request selects its object
// through the resident's independently learned visual grounding circuit.
KERNEL void flood_pod_tray(pods::Animal *organ, const grounding::Mind *sight, const Count *point, Word seed, Word angle,
                           Word start, Word duration, Word variety, Word action, pods::TrayReading *output) {
    if (tid())
        return;
    pods::TrayReading event{};
    Word surface = variety;
    if (variety == 9) {
        Count certain = point[0], possible = point[1];
        if (!certain || (certain & (certain - 1)) || possible != certain) {
            event.ambiguous = 1;
            *output = event;
            return;
        }
        Word site = __builtin_ctzll(certain);
        surface = 256;
        for (Word i = 0; i < sight->here.n; ++i)
            if (sight->here.patch[i].site == site) {
                surface = sight->here.patch[i].light;
                break;
            }
        if (surface == 256) {
            event.ambiguous = 1;
            *output = event;
            return;
        }
        event.site = site;
    }
    pods::Inheritance g{organ->genes, organ->frame_genes};
    pods::Physics environment{};
    auto life = ForagingLife{organ->brain, organ->frame, organ->gut};
    Word base = balanced_crop(seed), law = pods::rotation(base, angle) | (base & 0xffff0000u);
    for (Word j = 0; j < duration; ++j) {
        Word t = start + j, key = mix(seed ^ mix(t + 19079)), cue = variety == 8 ? (key >> 21) & 7 : surface;
        Word raw = variety == 9 ? surface : spectrum(cue, key, true);
        event.surfaces |= 1u << (raw & 7);
        if (!action) {
            event.muscle = pods::pulse(life.mind, g.law, life.frame, g.frame, raw, (key >> 13) & 7, key, true).physical;
            event.light = raw;
            event.aligned = event.muscle == pods::aperture(raw, law);
            break;
        }
        auto receipt = pods::mature(life.gut, t);
        for (Word k = 0; k < 3; ++k)
            event.edible[k] += receipt.food[k];
        event.husks += receipt.waste;
        ++event.ticks;
        crop_experience(life, g, environment, raw, law, t, key, valence(receipt), false, true, &event);
    }
    if (action) {
        organ->brain = life.mind;
        organ->frame = life.frame;
        organ->gut = life.gut;
    }
    *output = event;
}
