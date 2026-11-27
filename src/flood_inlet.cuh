#include "axion/inlet.hpp"

KERNEL void inlet_ambient(World w, Configuration c, const Word *slots, Word size, inlet::Initial *out) {
    Word i = tid();
    if (i >= size)
        return;
    Word slot = slots[i], band = motor_band(w, c, slot);
    auto organ = regulation::source(w.dna[slot], band, read_band(w, c.tiles, slot, band));
    out[i] = {organ.cells, w.braids.minds[slot].volley >> 16};
}

// A warp evaluates one inherited circuit. The four siblings of each layout
// counterbalance every possible public outlet label at fixed decision noise.
KERNEL void inlet_panel(const inlet::Animal *genes, Word count_genes, Word seed, Word lives, inlet::Course course,
                        inlet::Totals *out, inlet::Report *trace) {
    Word index = tid() / 32, lane = tid() & 31;
    if (index >= count_genes)
        return;
    auto g = genes[index];
    inlet::Totals tally{};
    for (Word life = lane; life < lives; life += 32) {
        auto world = inlet::habitat(seed, life, course.ports & drift::seepage, course.landscape);
        auto before = inlet::prior_bed(seed, life, course, world);
        auto cells = inlet::starting(course, seed, life);
        auto a = inlet::encounter(g, world, before, course, false, cells);
        auto b = inlet::encounter(g, world, before, course, true, cells);
        inlet::score(tally, a);
        inlet::score(tally, b);
        if (a.target != b.target) {
            ++tally.crossed;
            tally.paired += a.answer == a.target && b.answer == b.target;
            tally.alternate_right += (a.answer == b.target) + (b.answer == a.target);
        }
        if (trace && index == 0) {
            for (Word j = 0; j < 4; ++j) {
                a.physical_map |= inlet::destination(world, j, false) << (2 * j);
                b.physical_map |= inlet::destination(world, j, true) << (2 * j);
            }
            trace[2 * life] = a;
            trace[2 * life + 1] = b;
        }
    }
    auto aggregate = [&](Word value) {
#if __CUDA_ARCH__ >= 800
        Word total;
        asm("redux.sync.add.u32 %0,%1,0xffffffff;" : "=r"(total) : "r"(value));
        return total;
#else
        for (Word mask = 16; mask; mask >>= 1) {
            Word other;
            asm("shfl.sync.bfly.b32 %0,%1,%2,31,0xffffffff;" : "=r"(other) : "r"(value), "r"(mask));
            value += other;
        }
        return value;
#endif
    };
    inlet::Totals sum{aggregate(tally.predicted),       aggregate(tally.proximal),      aggregate(tally.displaced),
                      aggregate(tally.committed),       aggregate(tally.assured),       aggregate(tally.crossed),
                      aggregate(tally.paired),          aggregate(tally.changed),       aggregate(tally.changed_right),
                      aggregate(tally.trained_food),    aggregate(tally.full_evidence), aggregate(tally.absent_right),
                      aggregate(tally.alternate_right), aggregate(tally.channel_cases), aggregate(tally.channel_right),
                      aggregate(tally.basin_cases),     aggregate(tally.basin_right),   aggregate(tally.contradicted),
                      aggregate(tally.recovered)};
    if (!lane)
        out[index] = sum;
}
KERNEL void inlet_graze(const apprentice::Animal *stock, Word count, Word seed, inlet::Course course, Word load,
                        apprentice::Harvest *budget) {
    Word id = tid();
    if (id >= count)
        return;
    auto egg = stock[id];
    apprentice::Harvest food{};
    if (egg.ancestor != ~0u) {
        inlet::Animal g{egg.program, egg.traits & 15, (egg.traits >> 5) & 131071u, (egg.traits >> 4) & 1};
        Word life = mix(id + seed);
        auto room = inlet::habitat(seed, life, course.ports & drift::seepage, course.landscape);
        auto before = inlet::prior_bed(seed, life, course, room);
        auto lived = inlet::encounter(g, room, before, course, false, inlet::starting(course, seed, life));
        bool ate = lived.answer == lived.target;
        food.swallowed = lived.hits + load * ate;
        food.escaped = course.steps + course.prior_steps + load - food.swallowed;
        food.matches = ate;
    }
    budget[id] = food;
}
