// Device implementation shared by ecology, conditioning, and the external actor.
// A variable LUT3 is a pair of two-input Boolean maps and one bitwise mux.
// Every operation evaluates 32 distinct neurons. The register file stays scalar.
AX_FN Word mux(Word m, Word high, Word low) { return (m & high) | (~m & low); }
AX_FN Word lut2(Word table, Word a, Word b) {
    Word lo = mux(a, 0u - ((table >> 1) & 1), 0u - (table & 1));
    Word hi = mux(a, 0u - ((table >> 3) & 1), 0u - ((table >> 2) & 1));
    return mux(b, hi, lo);
}
struct Ports { Word a, b, sensors, type; };
AX_FN Word source(Word addr, Tissue r, Ports p) {
    Word low = addr & 2 ? (addr & 1 ? r.gate_b : r.gate_a) : (addr & 1 ? r.memory : r.spike);
    Word high = addr & 2 ? (addr & 1 ? p.type : p.sensors) : (addr & 1 ? p.b : p.a);
    return addr & 4 ? high : low;
}
AX_FN void instruction(Word op, Ports p, Tissue &r, bool plastic) {
    Word a = source(op, r, p), b = source(op >> 3, r, p), c = source(op >> 6, r, p);
    Word table = op >> 9;
    Word value = mux(c, lut2(table >> 4, a, b), lut2(table, a, b));
    Word mode = (op >> 19) & 3;
    Word express = mode < 2 ? ~0u : (mode == 2 ? p.type : ~p.type);
    switch ((op >> 17) & 3) {
        case 0: r.spike = mux(express, value, r.spike); break;
        case 1: r.memory = mux(express, value, r.memory); break;
        case 2: if (plastic) r.gate_a = mux(express, value, r.gate_a); break;
        case 3: if (plastic) r.gate_b = mux(express, value, r.gate_b); break;
    }
}

struct Intent { Word action, updates, spikes, changed; };
// For at most 32 lanes the largest local offset is 62. The 16-bit fixed
// point reciprocal rounds down to exactly floor(offset / active) over this range.
__attribute__((device, constant)) static const unsigned short tissue_reciprocal[33] = {
    0,    0,    32768, 21846, 16384, 13108, 10923, 9363, 8192, 7282, 6554, 5958, 5462, 5042, 4682, 4370, 4096,
    3856, 3641, 3450,  3277,  3121,  2979,  2850,  2731, 2622, 2521, 2428, 2341, 2260, 2185, 2115, 2048};
AX_FN Word tissue_remainder(Word position, Word count) {
    return position - ((position * tissue_reciprocal[count]) >> 16) * count;
}
template <int Tiles, bool Flexible = false>
AX_FN Word afferent(Word spike, Word edge, Word tile, Word active, Word base, Word mask) {
    Word remote = (tile + (edge >> 5)) & (active - 1);
    if constexpr (Flexible)
        remote = tissue_remainder(tile + ((edge >> 5) & 31), active);
    return rotate(shuffle(spike, base + remote, mask), edge);
}
template<int Tiles>
AX_FN Word group_sum(Word x, Word tile, Word base, Word mask) {
    UNROLL
    for (int d = 1; d < Tiles; d *= 2) x += shuffle(x, base + (tile ^ d), mask);
    return x;
}
AX_FN Tissue develop(Word identity) { return initial_tissue(identity); }
AX_FN unsigned char effector(const Genome &g, Word action) {
    return static_cast<unsigned char>(g.effectors[action >> 2] >> (8 * (action & 3)));
}
template <int Tiles, bool CountActivity, bool Flexible = false>
AX_FN Intent neural(const Genome &g, Tissue &r, Word sensory, Word tile, Word base, Word mask, bool plastic,
                    Word expressed = 0, Word firing_limit = ~0u) {
    Word active = Flexible ? expressed : living_tiles(Tiles, g.development);
    Word identity = mix(g.development + 0x9e3779b9u * tile);
    sensory = rotate(sensory, (g.receptors + tile * ((g.receptors >> 8) & 31)) & 31) ^ g.receptors;
    // Distribute the inherited program across the lanes of the brain. Each lane
    // holds at most four instructions. Broadcast the current instruction once
    // per neural microtick without dynamically indexing a thread-local array.
    Word op0 = g.op[tile & 7], op1 = 0, op2 = 0, op3 = 0;
    if constexpr (Tiles < 8)
        op1 = g.op[(tile & (Tiles - 1)) + Tiles];
    if constexpr (Tiles == 2) {
        op2 = g.op[(tile & 1) + 4];
        op3 = g.op[(tile & 1) + 6];
    }
    Word length = 1 + (g.expression & 7);
    Word spikes = 0, changes = 0;
    _Pragma("unroll 1")
    for (Word n = 0; n < microticks; ++n) {
        Word a0 = afferent<Tiles, Flexible>(r.spike, g.edges[0], tile, active, base, mask);
        Word a1 = afferent<Tiles, Flexible>(r.spike, g.edges[0] >> 16, tile, active, base, mask);
        Word b0 = afferent<Tiles, Flexible>(r.spike, g.edges[1], tile, active, base, mask);
        Word b1 = afferent<Tiles, Flexible>(r.spike, g.edges[1] >> 16, tile, active, base, mask);
        Ports p{mux(r.gate_a, a1, a0), mux(r.gate_b, b1, b0), sensory, identity};
        Word old_a = r.gate_a, old_b = r.gate_b;
        _Pragma("unroll 1")
        for (Word i = 0; i < length; ++i) {
            Word local = op0;
            if constexpr (Tiles < 8) if (i >= Tiles) local = op1;
            if constexpr (Tiles == 2) if (i >= 4) local = i < 6 ? op2 : op3;
            Word op = shuffle(local, base + (i & (Tiles - 1)), mask);
            if (!Flexible || tile < firing_limit)
                instruction(op, p, r, plastic);
        }
        if constexpr (CountActivity) if (tile < active) {
            spikes += population(r.spike);
            changes += population(r.gate_a ^ old_a) + population(r.gate_b ^ old_b);
        }
    }
    if constexpr (CountActivity) {
        spikes = group_sum<Tiles>(spikes, tile, base, mask);
        changes = group_sum<Tiles>(changes, tile, base, mask);
    }
    return {0, active * 32 * microticks, spikes, changes};
}

struct MotorCounts { Word low, high, gain; };
// Packed groups may start at any lane and have any width between two
// and 32. Addition has the same modular integer semantics as group_sum.
template <int Tiles, bool Packed> AX_FN Word neural_sum(Word value, Word tile, Word base, Word mask) {
    if constexpr (!Packed)
        return group_sum<Tiles>(value, tile, base, mask);
#if __CUDA_ARCH__ >= 800
    Word sum;
    asm volatile("redux.sync.add.u32 %0,%1,%2;" : "=r"(sum) : "r"(value), "r"(mask));
    return sum;
#else
    Word sum = 0;
    for (Word remaining = mask; remaining; remaining &= remaining - 1)
        sum += shuffle(value, __builtin_ctz(remaining), mask);
    return sum;
#endif
}
template <int Tiles, bool Flexible = false, bool Packed = false>
AX_FN MotorCounts motor_counts(const Genome &g, Tissue r, Word tile, Word base, Word mask, Word expressed = 0) {
    Word active = Flexible ? expressed : living_tiles(Tiles, g.development);
    Word neurons=tile<active ? rotate(mux(g.motors,r.spike,r.memory),(g.motors>>24)+tile) : 0;
    // A motor population contributes at most four spikes per lane. Even at
    // 32 lanes its count fits in one byte, so integer addition has no carry
    // between the four independent populations in a word.
    Word low=0, high=0;
    UNROLL
    for (Word k=0;k<4;++k) {
        Word columns=0x01010101u<<k;
        low|=Word(population(neurons&columns))<<(8*k);
        high|=Word(population(neurons&(columns<<4)))<<(8*k);
    }
    return {neural_sum<Tiles, Packed>(low, tile, base, mask), neural_sum<Tiles, Packed>(high, tile, base, mask),
            (active * ((g.expression >> 3) & 31)) >> 2};
}
