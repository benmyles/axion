KERNEL void brine_compile(const brine::Animal *p, Word count, Word *rank, Half *firing) {
    Word i = tid();
    if (i >= count)
        return;
    auto &g = p[i].genes;
    auto weight = brine::efficacy(g, p[i].memory);
    Word total = brine::mass(weight), order = 0, on = 0;
    for (Word base = 0; base < 16; base += 4) {
        Word votes[4];
        for (Word k = 0; k < 4; ++k) {
            votes[k] = brine::estimate(g, weight, base + k);
            on |= Word(brine::impulse(g, votes[k], total)) << (base + k);
        }
        for (Word k = 0; k < 4; ++k) {
            Word below = 0;
            for (Word j = 0; j < 4; ++j)
                below += votes[j] < votes[k];
            order |= below << (2 * (base + k));
        }
    }
    rank[i] = order;
    firing[i] = static_cast<Half>(on);
}
namespace {
template <bool packed>
AX_FN void brine_choices(const brine::Animal *p, Word n, const Word *rank, const Half *fires, Word count, Word seed,
                         Word *out) {
    Word i = tid();
    if (i >= count)
        return;
    Word k = mix(seed + i * 2309), who = mix(k + 773) % n, c = (k >> 15) & 12, a = c + (k & 3), b = c + ((k >> 7) & 3);
    Word left, right;
    bool emits;
    if constexpr (packed) {
        Word row = rank[who];
        left = (row >> (2 * a)) & 3;
        right = (row >> (2 * b)) & 3;
        emits = (fires[who] >> a) & 1;
    } else {
        auto &animal = p[who];
        auto power = brine::efficacy(animal.genes, animal.memory);
        left = brine::estimate(animal.genes, power, a);
        right = brine::estimate(animal.genes, power, b);
        emits = brine::impulse(animal.genes, left, brine::mass(power));
    }
    out[i] = (left == right ? ((k >> 19) & 1) : Word(right > left)) | (Word(emits) << 1) | (Word(left == right) << 2);
}
} // namespace
#define BRINE_IMAGE(name, packed)                                                                                      \
    KERNEL void name(const brine::Animal *p, Word n, const Word *rank, const Half *fires, Word count, Word seed,       \
                     Word *out) {                                                                                      \
        brine_choices<packed>(p, n, rank, fires, count, seed, out);                                                    \
    }
BRINE_IMAGE(brine_mutable, false)
BRINE_IMAGE(brine_settled, true)
#undef BRINE_IMAGE
