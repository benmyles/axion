namespace axion::life::material {
AX_FN Word settle(const Worker &p, Word kind, Word basal, bool drive_basal = true) {
    auto g = p.genes;
    Word want = emit(p.seeks[kind], g.release >> 4), inborn = (g.innate >> (3 * kind)) & 7;
    int best = -256;
    Word bits = 0;
    for (Word m = 0; m < 8; ++m) {
        Word lit = emit(p.predicts[m], g.release);
        int v = int(__builtin_popcount(lit & want)) * (1 + int((g.release >> 9) & 7)) -
                int(__builtin_popcount(lit & ~want)) * int((g.release >> 12) & 3) +
                int(m == inborn) * int((g.innate >> 24) & 7) +
                int(drive_basal && m == (basal & 7)) * int((g.release >> 16) & 7);
        if (v > best) {
            bits = 0;
            best = v;
        }
        if (v == best)
            bits |= 1u << m;
    }
    return bits;
}
AX_FN Word muscle(const HandImage &p, Word raw, Word basal, Word key) {
    Word start = mix(key) & 7;
    if (((key >> 8) & 255) < (p.control >> 16))
        return start;
    Word kind = index(raw, p.control), at = kind >> 2, shift = 8 * (kind & 3), bit = 1u << (basal & 7);
    Word tie = (p.best[at] >> shift) & 255;
    if (((p.forced[at] >> shift) & bit) != 0)
        return basal & 7;
    if (((p.tied[at] >> shift) & bit) != 0)
        tie |= bit;
    return (start + __builtin_ctz((tie | (tie << 8)) >> start)) & 7;
}
} // namespace axion::life::material
KERNEL void flood_hand_encode(const material::Worker *body, const Word *slots, material::HandImage *images,
                              Word count) {
    Word i = tid();
    if (i >= count)
        return;
    auto &p = body[slots[i]];
    material::HandImage image{};
    image.control = (p.genes.receptors & 4095u) | (((p.genes.release >> 20) & 31) << 16);
    for (Word kind = 0; kind < 8; ++kind) {
        Word base = material::settle(p, kind, 0, false), force = 0, join = 0;
        for (Word basal = 0; basal < 8; ++basal) {
            Word bit = 1u << basal, tie = material::settle(p, kind, basal);
            if (tie == bit)
                force |= bit;
            else if (tie != base)
                join |= bit;
        }
        Word at = kind >> 2, shift = 8 * (kind & 3);
        image.best[at] |= base << shift;
        image.forced[at] |= force << shift;
        image.tied[at] |= join << shift;
    }
    images[i] = image;
}
KERNEL void flood_hand_check(const material::Worker *body, const Word *slots, const material::HandImage *images,
                             Word samples, Word *errors) {
    Word i = tid(), id = i >> 8, raw = i & 255;
    if (id >= samples)
        return;
    const auto &p = body[slots[id]];
    auto image = images[id];
    Word wrong = 0;
    for (Word explore = 0; explore < 2; ++explore)
        for (Word origin = 0; origin < 8; ++origin) {
            Word k = explore ? 0 : 0xff00;
            while ((mix(k) & 7) != origin)
                ++k;
            for (Word b = 0; b < 8; ++b)
                wrong += material::muscle(image, raw, b, k) != material::choose(p, raw, b, k);
        }
    if (wrong)
        asm volatile("red.global.add.u32 [%0],%1;" ::"l"(errors), "r"(wrong) : "memory");
}
template <bool compact>
AX_FN void hand_calls(const material::Worker *body, const Word *slots, const material::HandImage *image, Word donors,
                      Word calls, Word seed, Word *out) {
    Word i = tid();
    if (i >= calls)
        return;
    Word key = mix(seed ^ i), id = mix(key + 913) % donors, light = key >> 24, basal = key >> 16;
    if constexpr (compact)
        out[i] = material::muscle(image[id], light, basal, key);
    else
        out[i] = material::choose(body[slots[id]], light, basal, key);
}
#define HAND_CALL_KERNEL(name, compact)                                                                                \
    KERNEL void name(const material::Worker *body, const Word *slots, const material::HandImage *image, Word donors,   \
                     Word calls, Word seed, Word *out) {                                                               \
        hand_calls<compact>(body, slots, image, donors, calls, seed, out);                                             \
    }
HAND_CALL_KERNEL(flood_hand_plastic, false)
HAND_CALL_KERNEL(flood_hand_compact, true)
#undef HAND_CALL_KERNEL
