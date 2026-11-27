namespace axion::life::practice {
AX_FN Script think(const Panel &panel) {
    Charts charts{};
    charts.count = size(panel.agent);
    for (Word j = 0; j < charts.count; ++j)
        charts.model[j] = read(panel.memory[j], panel.agent.genes);
    return reason(panel.agent, charts, panel.target, panel.target.rig.handles, mix(panel.target.view + 117), all);
}
AX_FN Count signature(Word root, Word watch, linkage::Design p) {
    return Count(p.turns.low) | (Count(p.turns.high) << 24) | (Count(p.turns.len) << 36) | (Count(p.edit) << 40) |
           (Count(p.found) << 43) | (Count(root) << 44) | (Count(watch) << 48);
}
AX_FN Word scan_word(Word x) {
    Word lane = tid() & 31;
    for (Word k = 1; k < 32; k *= 2) {
        Word old;
        asm volatile("shfl.sync.up.b32 %0,%1,%2,0,0xffffffff;" : "=r"(old) : "r"(x), "r"(k));
        if (lane >= k)
            x += old;
    }
    return x;
}
} // namespace axion::life::practice
KERNEL void practice_panels(const practice::Genes *genes, Word ng, Word count, Word seed, Word width, Word cap,
                            practice::Panel *out) {
    Word i = tid();
    if (i >= count)
        return;
    using namespace practice;
    auto &p = out[i];
    p = {};
    p.agent.genes = genes[i % ng];
    Word k = mix(seed + i * 17317), worlds[2];
    linkage::Material law[2][4];
    climate(k, worlds, law);
    Slots slots{p.memory, 1, cap};
    for (Word phase = 0; phase < 2; ++phase)
        handle(p.agent, slots, law[phase], mix(k + phase * 18041), 2, informed, all);
    p.target = apparatus(mix(k + 10193), width, law[(k >> 11) & 1]);
}
KERNEL void practice_sizes(const practice::Panel *panels, Word n, practice::Card *out, Half *addresses,
                           Word *blocks) {
    using namespace practice;
    Word i = tid(), lane = i & 31, warp = (i >> 5) & 3;
    Word bytes = 0;
    if (i < n) {
        auto c = encode(think(panels[i]));
        out[i] = c;
        bytes = c.size;
    }
    __attribute__((shared)) static Word subtotal[4];
    Word cumulative = scan_word(bytes);
    if (lane == 31)
        subtotal[warp] = cumulative;
    asm volatile("bar.sync 0;" ::: "memory");
    Word prior = 0;
    for (Word j = 0; j < warp; ++j)
        prior += subtotal[j];
    if (i < n)
        addresses[i] = prior + cumulative - bytes;
    if ((i & 127) == 127)
        blocks[i / 128] = prior + cumulative;
}
KERNEL void practice_offsets(Word *blocks, Word n, Word *total) {
    Word offset = 0;
    for (Word i = 0; i < n; ++i) {
        Word v = blocks[i];
        blocks[i] = offset;
        offset += v;
    }
    *total = offset;
}
KERNEL void practice_pack(const practice::Card *pieces, Word n, const Half *addresses, const Word *blocks,
                          unsigned char *data) {
    Word i = tid();
    if (i >= n)
        return;
    Word base = addresses[i] + blocks[i / 128];
    const auto &s = pieces[i];
    for (Word j = 0; j < s.size; ++j)
        data[base + j] = s.bytes[j];
}
template <bool compact>
AX_FN void practice_image_query(const practice::Panel *panels, Word n, const Half *addresses, const Word *blocks,
                                const unsigned char *data, Word count, Word base, Word seed, Count *out) {
    using namespace practice;
    Word i = tid();
    if (i >= count)
        return;
    Word x = i + base, actor = x % n;
    Word observed = (x / n + seed) % 5;
    if constexpr (compact) {
        auto code = data + blocks[actor / 128] + addresses[actor];
        Word root = code[0] & 15, watch = (root < 8) ? (code[0] >> 4) & 3 : 4;
        out[i] = signature(root, watch, decode(code, observed));
    } else {
        auto script = think(panels[actor]);
        Word root = script.found ? script.ask : 15;
        Word branch = root < 8 ? observed : 0;
        linkage::Design p{};
        if (script.found && branch < 4 && (script.outcomes & (1u << branch)))
            p = script.reply[branch];
        out[i] = signature(root, root < 8 ? script.look : 4, p);
    }
}
#define PRACTICE_IMAGE_KERNEL(name, kind)                                                                            \
    KERNEL void name(const practice::Panel *p, Word n, const Half *at, const Word *blocks,                           \
                     const unsigned char *data, Word count, Word off, Word seed, Count *output) {                    \
        practice_image_query<kind>(p, n, at, blocks, data, count, off, seed, output);                                \
    }
PRACTICE_IMAGE_KERNEL(practice_frozen, true)
PRACTICE_IMAGE_KERNEL(practice_online, false)
#undef PRACTICE_IMAGE_KERNEL
template <bool local>
AX_FN void practice_single_effect(const practice::Panel *panels, Word n, Word count, Word offset, Word seed,
                                  Count *out) {
    using namespace practice;
    Word i = tid();
    if (i >= count)
        return;
    Word k = mix(seed + (i + offset) * 1981);
    const auto &panel = panels[(i + offset) % n];
    const auto &m = panel.agent;
    const auto &r = panel.target.rig;
    Word a = linkage::bump(r.handles, k), v = (k >> 9) & linkage::mask_parts(r.parts);
    Word expected = absent;
    if (size(m) && a < 8) {
        auto &c = panel.memory[(k >> 17) % size(m)];
        if constexpr (local)
            expected = forecast(c, m.genes, r, v, a);
        else
            expected = linkage::predict(read(c, m.genes), r, v, a);
    }
    out[i] = expected;
}
KERNEL void practice_local_effect(const practice::Panel *p, Word n, Word count, Word offset, Word seed, Count *out) {
    practice_single_effect<true>(p, n, count, offset, seed, out);
}
KERNEL void practice_full_effect(const practice::Panel *p, Word n, Word count, Word offset, Word seed, Count *out) {
    practice_single_effect<false>(p, n, count, offset, seed, out);
}
