struct Chromosome { Word bases[18]; };
AX_FN Genome founder(Word key) {
    Chromosome g;
    UNROLL
    for (int i = 0; i < 18; ++i) g.bases[i] = mix(key + i * 0x61c88647u);
    // Founders express each neural register. Mutation can redirect or silence any instruction.
    UNROLL
    for (int i = 0; i < 4; ++i) g.bases[i] = (g.bases[i] & ~(3u << 17)) | (Word(i) << 17);
    auto result = __builtin_bit_cast(Genome, g);
    result.expression = (result.expression & ~7u) | 4; // five expressed instructions in founders
    // Seed viable peripheral wiring, then allow point mutations to combine,
    // reverse, duplicate or silence the elementary energy and movement ports.
    result.effectors[0] = 0x06050408u;
    Word dir = key & 3;
    result.effectors[1] = 0x30401007u | (dir << 24) | (dir << 8);
    if (key & 16) result.effectors[1] ^= 0x80000000u;
    if (key & 32) result.effectors[1] ^= 0x00008000u;
    return result;
}
// Twelve inherited bits regulate the frequency of variation in four regions.
// The modifier locus itself is copied at the ordinary, positive mutation rate.
AX_FN Word fidelity_total(Word reproduction,Word regions) {
    Word sum=0;
    UNROLL
    for (Word i=0;i<4;++i) if (i<regions) sum+=(reproduction>>(20+3*i))&7;
    return sum;
}
AX_FN Word fidelity_cost(Word reproduction,Word regions,Word mode) {
    return mode ? (fidelity_total(reproduction,regions)+3)/4 : 0;
}
AX_FN Word mutable_regions(Word reproduction,Word key,Word regions,Word mode) {
    if (!mode || mode==3) return 15;
    Word shared=mode==2 ? (fidelity_total(reproduction,regions)+regions/2)/regions : 0, open=0;
    UNROLL
    for (Word i=0;i<4;++i) {
        Word exponent=mode==2 ? shared : (reproduction>>(20+3*i))&7;
        Word sample=mix(key^((i+3)*0xd15de785u));
        open|=Word(!(sample&((1u<<exponent)-1)))<<i;
    }
    return open;
}
AX_FN void inherit(const Genome &parent, Genome &child, Word key, bool mutate, Word &edits, Word &structural,Word regions=15) {
    if (!mutate) { child = parent; return; }
    auto g = __builtin_bit_cast(Chromosome, parent);
    Word probability = 4 + ((parent.reproduction >> 8) & 31);
    UNROLL
    for (int i = 0; i < 18; ++i) {
        Word noise = mix(key + i * 0xb5297a4du);
        if ((noise & 255) < probability) {
            // Gate genes have 21 expressed bits. The other words encode anatomy and physiology.
            Word bit = (noise >> 8) & 31;
            if (i < 8) bit = (noise >> 8) % 21;
            Word part=i==13 || i>=16 ? 1 : i==15 && bit>=16 ? 2 : 0;
            if (i!=14 && !(regions&(1u<<part))) continue;
            g.bases[i] ^= 1u << bit;
            ++edits;
        }
    }
    child = __builtin_bit_cast(Genome, g);
    Word edit = mix(key ^ 0x93496b2du);
    if ((regions&1) && (edit & 255) < 2 + ((parent.expression >> 8) & 15)) {
        int length = 1 + (child.expression & 7);
        int from = int((edit >> 8) % unsigned(length));
        if ((edit & (1u << 27)) && length > 1) {
            for (int j = from; j < length - 1; ++j) child.op[j] = child.op[j + 1];
            --length;
        } else if (length < 8) {
            Word duplicate = child.op[from];
            int to = int((edit >> 16) % unsigned(length + 1));
            for (int j = length; j > to; --j) child.op[j] = child.op[j - 1];
            child.op[to] = duplicate;
            ++length;
        } else {
            child.op[(from + 1) & 7] = child.op[from];
        }
        child.expression = (child.expression & ~7u) | Word(length - 1);
        ++structural;
    }
}
