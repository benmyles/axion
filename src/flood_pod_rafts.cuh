// A spring flood disperses embryos between finite feeding rafts. Solar inputs,
// the current, consumption, gut contents and reproductive provisioning are all
// accounted for on the GPU. The only inheritance tickets are edible reserves.
namespace {
using pods::Embryo;
using pods::RaftBudget;
AX_FN Word raft_shuffle(Word value, Word source) {
    Word result;
    asm volatile("shfl.sync.idx.b32 %0,%1,%2,31,0xffffffff;" : "=r"(result) : "r"(value), "r"(source));
    return result;
}
AX_FN Word raft_sum(Word value, Word lane) {
    for (Word d = 16; d; d >>= 1)
        value += raft_shuffle(value, lane ^ d);
    return value;
}
} // namespace
KERNEL void flood_raft_seed(Embryo *eggs, const pods::Inheritance *stock, Word parents, Word n, Word seed, Word origin,
                            Word variations) {
    Word i = tid();
    if (i >= n)
        return;
    Word key = mix(seed ^ mix(i + 2339));
    auto &e = eggs[i];
    e.frame = origin == 3   ? stock[mix(key + 7) % parents].frame
              : origin == 1 ? pods::frame_reference()
              : origin == 2 ? pods::FrameGenes{}
                            : pods::frame_founder(key);
    if (variations)
        e.frame = pods::frame_founder(mix(key + 111));
    e.law = origin == 3   ? stock[mix(key + 7) % parents].law
            : origin == 1 ? pods::reference()
            : origin == 2 ? pods::Genes{}
                          : pods::founder(key);
    e.lineage = i;
    e.living = 1;
}
KERNEL void flood_raft_season(const Embryo *eggs, Embryo *children, RaftBudget *books, Word n, Word seed, Word days,
                              Word generation, Word drift, Word flush, pods::Physics physics) {
    Word id = tid();
    if (id >= n)
        return;
    Word lane = id & 31, raft = id / 32;
    auto egg = eggs[id];
    auto g = egg.law;
    pods::Circuit b{};
    pods::Frame frame{};
    pods::Gut gut{};
    RaftBudget budget;
    constexpr Word provision = 64;
    Word energy = egg.living ? provision : 0, stock = 12, waste = 0, heat = 0, light = 0, bites = 0, opened = 0,
         held = 0, alone = 0;
    budget.initial = energy + stock;
    Word weather = mix(seed ^ mix(generation + 0x71d359u) ^ mix(raft + 0x53c719u));
    Word first = balanced_crop(weather), reversed = (balanced_crop(mix(weather + 8617)) & 0xffff0000u) |
                                                    ((first ^ (0x5555u * (1 + mix(weather + 319) % 3))) & 65535u);
    bool framed = physics.background & 4;
    Word first_storm = flush ? days / 3 : days / 2;
    for (Word t = 0; t < days; ++t) {
        Word key = mix(weather ^ mix(t + lane * 9187)), law = t < first_storm ? first : reversed;
        Word cue = (key >> 15) & 7, light_flux = 1 + ((key >> 8) & 7);
        if (t >= first_storm && ((physics.background & 8) || flush)) {
            Word age = t - first_storm, season = age / physics.period, wind = mix(weather + season * 7151 + 19);
            Word interval = age % physics.period, early = (wind >> 12) & 7;
            if (physics.background & 8)
                law = pods::rotation(first, 1 + wind % 3) | (wind & 0xffff0000u);
            else if (flush)
                law = balanced_crop(wind);
            if (flush) {
                // A pioneer fruits alone before a short flush of the other
                // seven strains. Light and reserve again limit each harvest.
                bool mass_fruiting = interval >= physics.period * 3 / 4;
                cue = mass_fruiting ? (early + 1 + (key >> 15) % 7) & 7 : early;
                light_flux = mass_fruiting ? 10 + ((key >> 8) & 3) : 1 + ((key >> 8) & 1);
            } else if (interval < physics.period / 3)
                cue = early;
        }
        Word scent = spectrum(cue, mix(key + 353), false), basal = (key >> 20) & 7;
        Word incoming = pods::lower(15u - (stock < 15 ? stock : 15), light_flux);
        stock += incoming;
        light += incoming;
        auto delivery = pods::mature(gut, t);
        waste += delivery.waste;
        if (energy) {
            auto pulse = pods::pulse(b, g, frame, egg.frame, scent, basal, key, framed);
            Word jaw = pulse.physical, index = pulse.cue;
            if ((physics.background & 16) && !delivery.edible && !delivery.waste)
                pods::omission(b, g);
            if (delivery.edible || delivery.waste) {
                energy += delivery.edible;
                if (framed)
                    pods::reframe(frame, egg.frame, g, b, delivery.edible != 0, key, 0);
                budget.edits += pods::associate(b, g, delivery.edible != 0, 1, key);
            }
            bool filled = pods::occupied(gut),
                 permit = !(physics.background & 2) || pods::strike(b, g, index, jaw, filled);
            Word mass = permit ? pods::lower(stock, 12) : 0;
            if (mass) {
                bool open = jaw == pods::aperture(cue, law);
                Word lag = pods::latency(cue, law, physics);
                pods::swallow(gut, t, lag, 1, mass, open ? mass : 0);
                stock -= mass;
                ++bites;
                opened += open;
                alone += !filled;
            } else
                held += !permit;
            if (framed)
                pods::perceive(frame, egg.frame, pulse, mass != 0);
            pods::remember(b, index, pulse.native, mass != 0);
            --energy;
            ++heat;
            if (!energy) {
                waste += pods::content(gut);
                gut = {};
            }
        } else
            waste += delivery.edible;
        Word floating = stock / 4;
        stock = stock - floating + raft_shuffle(floating, (lane + 31) & 31);
    }
    waste += pods::content(gut);
    Word available = energy / provision, through = available;
    for (Word k = 1; k < 32; k *= 2) {
        Word v = raft_shuffle(through, (lane + 32 - k) & 31);
        if (lane >= k)
            through += v;
    }
    Word total = raft_shuffle(through, 31), survivors = pods::lower(32, total), draw = 0;
    if (total >= 32)
        draw = (lane * total + (mix(weather + 77191) % total)) / 32;
    else
        draw = lane;
    Word before = 0;
    for (Word d = 16; d; d >>= 1) {
        Word at = before + d - 1, boundary = raft_shuffle(through, at);
        if (boundary <= draw)
            before += d;
    }
    // A control decouples the provisioned embryo from the forager supplying
    // its reserves by exchanging germlines of viable eggs within the same raft.
    Word donor = before < 32 ? before : 31;
    Word viable = 0;
    asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,0xffffffff; }"
                 : "=r"(viable)
                 : "r"(egg.living));
    if (drift) {
        Word index = (donor + 1 + (mix(weather + 553) % 31)) & 31;
        Word rotated = (viable >> index) | (viable << ((32 - index) & 31));
        donor = (index + Word(rotated ? __builtin_ctz(rotated) : 0)) & 31;
    }
    Embryo newborn;
    newborn.law = {raft_shuffle(g.plastic, donor), raft_shuffle(g.timing, donor), raft_shuffle(g.readout, donor),
                   raft_shuffle(g.sense, donor)};
    newborn.frame = {raft_shuffle(egg.frame.routes, donor), raft_shuffle(egg.frame.learning, donor),
                     raft_shuffle(egg.frame.consolidation, donor)};
    newborn.frame = pods::recruit(
        pods::frame_child(newborn.frame, mix(weather + lane * 19141), 12 + ((newborn.law.timing >> 27) & 31)),
        newborn.law, mix(weather + lane * 19141));
    newborn.lineage = raft_shuffle(egg.lineage, donor);
    newborn.living = lane < survivors;
    Word key = mix(weather + lane * 17159);
    newborn.law = pods::descend(newborn.law, key, 12 + ((newborn.law.timing >> 27) & 31));
    // Exchange a small fraction of embryos between rafts with a bijection.
    Word wind = mix(seed ^ mix(generation + 21717) ^ lane), shift = 32u << ((wind >> 8) % 14);
    Word destination = ((wind & 15) == 0 && shift < n) ? id ^ shift : id;
    children[destination] = newborn;
    budget.light = light;
    budget.stock = stock;
    budget.waste = waste;
    budget.heat = heat;
    budget.reserve = energy;
    budget.offspring = newborn.living;
    budget.provision = newborn.living * provision;
    budget.bites = bites;
    budget.opened = opened;
    budget.held = held;
    budget.isolated = alone;
    budget.adults = energy != 0;
    auto *fields = reinterpret_cast<Count *>(&budget);
    for (Word k = 0; k < sizeof(RaftBudget) / sizeof(Count); ++k) {
        Word sum = raft_sum(Word(fields[k]), lane);
        if (!lane)
            reinterpret_cast<Count *>(books + raft)[k] = sum;
    }
}
KERNEL void flood_raft_release(const pods::Inheritance *laws, Word parents, World w, Configuration c, Word seed) {
    Word i = tid();
    if (i < c.side * c.side) {
        w.pods.animal[i] = {};
        auto g = laws[mix(seed ^ mix(i + 14771)) % parents];
        w.pods.animal[i].genes = g.law;
        w.pods.animal[i].frame_genes = g.frame;
    }
}

KERNEL void flood_raft_empty(World w, Configuration c) {
    Word i = tid();
    if (i < c.side * c.side) {
        auto &a = w.animals[w.slots[i]];
        w.land[i].litter += a.energy;
        a.energy = 0;
    }
}
