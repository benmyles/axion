AX_FN Word warp_ballot(Word value) {
    Word result;
    asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,0xffffffff; }"
                 : "=r"(result)
                 : "r"(value));
    return result;
}
AX_FN Word warp_union(Word value) {
#if __CUDA_ARCH__ >= 800
    Word result;
    asm volatile("redux.sync.or.b32 %0,%1,0xffffffff;" : "=r"(result) : "r"(value));
    return result;
#else
    Word lane = tid() & 31;
    for (Word d = 1; d < 32; d *= 2)
        value |= shuffle(value, lane ^ d, ~0u);
    return value;
#endif
}
AX_FN void packed_think(World w, Configuration c, Word tick) {
    Word pos = tid(), lane = pos & 31, first = pos - lane, sites = c.side * c.side;
    if (first >= sites)
        return;
    Word slot = 0, size = 0;
    if (pos < sites) {
        slot = w.slots[pos];
        inspect_bed(w, c, pos);
        if (w.animals[slot].energy)
            size = w.cortex.resident[slot].active;
    }
    Word end = size;
    for (Word stride = 1; stride < 32; stride *= 2) {
        Word preceding;
        asm volatile("shfl.sync.up.b32 %0,%1,%2,0,0xffffffff;" : "=r"(preceding) : "r"(end), "r"(stride));
        if (lane >= stride)
            end += preceding;
    }
    Word total = shuffle(end, 31, ~0u), start = end - size, consumed = 0;
    while (consumed < total) {
        bool fits = size && start >= consumed && end <= consumed + 32;
        Word owners = warp_ballot(fits);
        Word starts = warp_union(fits ? (1u << ((start - consumed) & 31)) : 0u);
        Word last = 31 - __builtin_clz(owners), next = shuffle(end, last, ~0u);
        Word reached = starts & (~0u >> (31 - lane));
        Word base = 31 - __builtin_clz(reached), rank = __builtin_popcount(reached), owner;
        asm volatile("fns.b32 %0,%1,0,%2;" : "=r"(owner) : "r"(owners), "r"(rank));
        Word personal = shuffle(slot, owner, ~0u), width = shuffle(size, owner, ~0u);
        Word mask = (~0u >> (32 - width)) << base;
        if (lane < next - consumed)
            think_group<2, true>(w, c, tick, first + owner, personal, lane - base, base, mask, width);
        consumed = next;
    }
}
