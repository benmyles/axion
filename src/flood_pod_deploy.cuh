namespace {
AX_FN Word image_motion(const pods::MotorImage &image, Word raw, Word basal, Word key) {
    pods::Genes selector{};
    selector.sense = image.senses;
    Word cue = pods::receptor(raw, selector), start = mix(key) & 3;
    Word mask = (image.ties[basal & 3] >> (4 * cue)) & 15;
    Word intrinsic = ((key >> 8) & 255) < ((image.control >> 16) & 31) ? start : pods::from_tie(mask, start);
    return image.control & (1u << cue) ? (image.control >> (8 + 2 * intrinsic)) & 3 : intrinsic;
}
// An independent scalar interpreter checks the bit-sliced release counts,
// basal tie break, consolidation, exploration, and the muscle crossbar.
AX_FN Word scalar_motion(const pods::Animal &p, Word raw, Word basal, Word key, bool framed) {
    const auto &g = p.genes;
    Word cue = pods::receptor(raw, g), start = mix(key) & 3, arm = start;
    bool explore = pods::explores(g, key);
    if (!explore) {
        auto &support = p.brain.support[pods::group(cue, g)];
        int peak = -128, best = -1;
        for (Word t = 0; t < 8; ++t)
            if ((g.timing & (1u << t)) && support[t] > peak)
                peak = support[t];
        for (Word k = 0; k < 4; ++k) {
            Word a = (start + k) & 3, x = p.brain.contact[4 * cue + a], spikes = 0;
            for (Word t = 0; t < 8; ++t)
                if ((g.timing & (1u << t)) &&
                    ((g.timing & (1u << 27)) || support[t] + int((g.timing >> 24) & 7) >= peak))
                    spikes += __builtin_popcount((g.readout >> (4 * ((x >> (2 * t)) & 3))) & 15);
            int drive = 4 * spikes + ((g.readout >> 16) & 3) * (a == (basal & 3));
            if (drive > best) {
                best = drive;
                arm = a;
            }
        }
    }
    if (framed && pods::settled(p.frame, p.frame_genes) && (p.frame.known & (1u << cue))) {
        if (!explore)
            arm = (p.frame.anchors >> (2 * cue)) & 3;
        arm = pods::route(p.frame_genes, p.frame.active, arm);
    }
    return arm;
}
} // namespace
KERNEL void flood_pod_compile(const pods::Animal *stock, const Word *slots, pods::MotorImage *images, Word n,
                              Word framed) {
    Word i = tid();
    if (i >= n)
        return;
    framed = framed && !(slots[i] >> 31);
    const auto &p = stock[slots[i] & 0x7fffffffu];
    pods::MotorImage image{};
    image.senses = p.genes.sense;
    bool settled = framed && pods::settled(p.frame, p.frame_genes);
    Word row = p.frame.active ? (p.frame_genes.routes >> (8 * p.frame.active)) & 255 : 0xe4;
    image.control = (settled ? p.frame.known : 0) | (row << 8) | (((p.genes.readout >> 18) & 31) << 16);
    for (Word cue = 0; cue < 8; ++cue) {
        Word firing = pods::volleys(p.brain, p.genes, cue);
        for (Word basal = 0; basal < 4; ++basal) {
            Word mask = (image.control & (1u << cue)) ? (1u << ((p.frame.anchors >> (2 * cue)) & 3))
                                                      : pods::winners(firing, (p.genes.readout >> 16) & 3, basal);
            image.ties[basal] |= mask << (4 * cue);
        }
    }
    images[i] = image;
}
KERNEL void flood_pod_validate(const pods::Animal *stock, const Word *slots, const pods::MotorImage *images, Word count,
                               Word framed, Word *errors) {
    Word i = tid(), sample = i >> 8, raw = i & 255;
    if (sample >= count)
        return;
    framed = framed && !(slots[sample] >> 31);
    const auto &p = stock[slots[sample] & 0x7fffffffu];
    auto image = images[sample];
    Word wrong = 0;
    for (Word explore = 0; explore < 2; ++explore)
        for (Word start = 0; start < 4; ++start) {
            Word key = explore ? 0 : 0xff00;
            while ((mix(key) & 3) != start)
                ++key;
            for (Word basal = 0; basal < 4; ++basal) {
                Word truth = scalar_motion(p, raw, basal, key, framed);
                wrong +=
                    image_motion(image, raw, basal, key) != truth ||
                    pods::pulse(p.brain, p.genes, p.frame, p.frame_genes, raw, basal, key, framed).physical != truth;
            }
        }
    if (wrong)
        asm volatile("red.global.add.u32 [%0], %1;" ::"l"(errors), "r"(wrong) : "memory");
}
template <bool packed>
AX_FN void exercise_muscles(const pods::Animal *stock, const Word *slots, const pods::MotorImage *images, Word n,
                            Word calls, Word seed, Word framed, Word *output) {
    Word i = tid();
    if (i >= calls)
        return;
    Word key = mix(seed ^ i), resident = mix(key + 0x134b13u) % n, raw = key >> 24, basal = key >> 16;
    if constexpr (packed)
        output[i] = image_motion(images[resident], raw, basal, key);
    else {
        framed = framed && !(slots[resident] >> 31);
        const auto &p = stock[slots[resident] & 0x7fffffffu];
        output[i] = pods::pulse(p.brain, p.genes, p.frame, p.frame_genes, raw, basal, key, framed).physical;
    }
}
#define MOTOR_TIMER(name, packed)                                                                                      \
    KERNEL void name(const pods::Animal *s, const Word *slots, const pods::MotorImage *m, Word n, Word calls,          \
                     Word seed, Word framed, Word *out) {                                                              \
        exercise_muscles<packed>(s, slots, m, n, calls, seed, framed, out);                                            \
    }
MOTOR_TIMER(flood_pod_image_calls, true)
MOTOR_TIMER(flood_pod_mutable_calls, false)
#undef MOTOR_TIMER
