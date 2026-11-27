namespace axion::life::material::scree {
AX_FN bool appetite(const FieldPupil &p, Word material_band, Word soil_band) {
    Word cue = 4 * worksite::surface_color(p.mind.genes, material_band) + (soil_band ^ wild_odor(p.mind.genes));
    return fires(p.exploratory.chemistry, cue, p.mind.genes.release);
}
AX_FN void revisit(FieldPupil &p, const Imprint *ground, Word side, Word key, Word mode, Word &count) {
    Word start = p.location;
    for (Word j = 0; j < 16; ++j) {
        Word x = (start % side + j % 4) & (side - 1), y = (start / side + j / 4) & (side - 1);
        Word pos = x + y * side;
        auto marks = ground[pos].events;
        if (mode == 7)
            marks = worksite::exchange_results(marks, false);
        for (int i = 3; i >= 0; --i) {
            Word glyph = (marks >> (8 * i)) & 255;
            if (glyph & 128) {
                worksite::consume_observation(p.exploratory, p.mind.memory, p.mind.genes, glyph);
                ++count;
            }
        }
    }
}
} // namespace axion::life::material::scree
KERNEL void flood_field_transfer(const material::FieldPupil *pool, Word donors, const material::Imprint *ground,
                                 Word side, material::FieldTransfer *out, Word subjects, Word seed, Word trials,
                                 material::ShoalOptions opt, Word mode) {
    using namespace material;
    using namespace material::scree;
    Word thread = tid(), id = thread / 32, lane = thread & 31;
    if (id >= subjects)
        return;
    Word base = mix(seed + id * 5771);
    auto p = pool[mix(base + 717) % donors];
    FieldTransfer result{};
    if (mode == 1) {
        p.mind.memory = {};
        p.exploratory.chemistry = 0;
        p.exploratory.sampled = 0;
    }
    if (mode == 2)
        p.mind.memory.contacts = 0;
    if (mode == 3)
        p.mind.memory.muscles[0] = p.mind.memory.muscles[1] = 0;
    if (mode == 4)
        p.exploratory.chemistry = 0;
    if (mode == 6 || mode == 7) {
        p.exploratory.chemistry = 0;
        p.exploratory.sampled = 0;
        p.mind.memory.contacts = 0;
        p.mind.memory.appetite &= 0xff00;
        revisit(p, ground, side, base, mode, result.marks);
    }
    auto g = p.mind.genes;
    result.known_contacts = __builtin_popcount(p.mind.memory.appetite >> 16);
    result.known_soils = __builtin_popcount(p.exploratory.sampled & 65535);
    result.lift = effector(g, motor_for(p.mind.memory, g, 0, base)) == 3;
    result.place = effector(g, motor_for(p.mind.memory, g, 1, base + 619)) == 4;
    for (Word t = 0; t < trials; ++t) {
        Word key = mix(base + t * 2137 + 193), layout = mix(key + 445);
        auto c = coast(layout, opt.span);
        resident_eye(c, g);
        c.catalyst = (key >> 17) & 3;
        c.contrast = 1 + (mix(key + 339) % 3);
        c.responsive = (key >> 8) & 1;
        // Generate a physical two-option contact discrimination. The experiment
        // presents one transmitting and one insulating contact, in random order.
        Word glass = wild_glass(g), die = mix(key + 357), src[2]{}, dst[2]{};
        for (Word v = 0; v < 16; ++v) {
            Word q = ((die >> 8) + v * ((die & 7) * 2 + 1)) & 15;
            Grain a{static_cast<Byte>(1 | ((q >> 2) << 2)), 0x33, 3, 0},
                b{static_cast<Byte>(1 | ((q & 3) << 6)), 0x33, 0, 0};
            bool passed = conduct(a, b, 1, true);
            Word n = passed;
            src[n] = q >> 2;
            dst[n] = q & 3;
        }
        bool yes = fires(p.mind.memory.contacts, 4 * (src[1] ^ glass) + (dst[1] ^ glass), g.release);
        bool no = fires(p.mind.memory.contacts, 4 * (src[0] ^ glass) + (dst[0] ^ glass), g.release);
        result.terminals += yes == no ? ((key >> 21) & 1) : yes;
        Word band_a = reagent_band(c, c.first), band_b = reagent_band(c, c.second);
        bool first = appetite(p, c.catalyst, band_a), second = appetite(p, c.catalyst, band_b);
        if (mode == 5) {
            first = fires(p.mind.memory.appetite, band_a ^ wild_odor(g), g.release);
            second = fires(p.mind.memory.appetite, band_b ^ wild_odor(g), g.release);
        }
        bool choose_b = first == second ? bool((key >> 12) & 1) : second;
        Grain catalyst{0xff, static_cast<Byte>(0x30u | (c.catalyst << 2)), 4, 0};
        Half raw = 1, food = 0;
        auto trial = excite(catalyst, 4, choose_b ? band_b : band_a, raw, food);
        result.substrate += trial.reaction;
        result.selective += trial.reaction && (first != second);
        Word desired = (Word(first) << c.first) | (Word(second) << c.second);
        auto e = construct(layout, lane, c, p.mind, opt, 0, 16 + opt.moves * 12, nullptr, desired, true);
        result.reactors += e.reactions > 0;
        result.needed += e.reactions && e.essential >= 2;
        result.food += e.reactions;
        result.other += e.alternative;
        result.exclusively += e.reactions && !e.alternative;
        result.original += e.original;
        result.sealed += e.sealed + e.invalid;
        ++result.trials;
    }
    if (!lane)
        out[id] = result;
}
KERNEL void flood_blank_inquiry(World original, material::Inquiry *clone, material::Foresight *forecast, Word n) {
    Word i = tid();
    if (i < n) {
        auto p = original.worksites.residents[i];
        clone[i] = {p.receptors, p.impulses};
        forecast[i] = {};
    }
}
KERNEL void flood_inquiry_effect(World original, World intact, World naive, Configuration c, HearingEffect *out) {
    Word pos = tid();
    if (pos >= c.side * c.side)
        return;
    Word id = original.slots[pos];
    HearingEffect e{};
    if (original.animals[id].energy) {
        e.experienced = 1;
        e.inputs = worksite::afferent(intact, c, pos, id) != worksite::afferent(naive, c, pos, id);
        Word active = expressed_cells(original, id, original.dna[id].brain, c.tiles);
        for (Word i = 0; i < active; ++i) {
            auto a = intact.nerve[Count(id) * c.tiles + i], b = naive.nerve[Count(id) * c.tiles + i];
            e.spikes += nbits(a.spike ^ b.spike);
            e.contacts += nbits(a.memory ^ b.memory);
        }
        e.motor = (intact.motor[id] & 7) != (naive.motor[id] & 7);
    }
    out[id] = e;
}

KERNEL void flood_worksite_image(const material::FieldPupil *pupils, Word n, Word *pairs, Word *moves,
                                 unsigned char *odor) {
    Word i = tid();
    if (i >= n)
        return;
    auto &p = pupils[i];
    auto img = material::scree::settle(p.mind.memory, p.mind.genes.release);
    pairs[i] =
        Word(img.contacts) | (material::scree::fold_contacts(p.exploratory.chemistry, p.mind.genes.release) << 16);
    moves[i] = img.motor[0] | (Word(img.motor[1]) << 8) | (Word(img.motor[2]) << 16) | (Word(img.motor[3]) << 24);
    odor[i] = img.taste;
}
namespace {
template <bool packed>
AX_FN void worksite_predictions(const material::FieldPupil *pupils, Word n, const Word *pairs, const Word *moves,
                                const unsigned char *odors, Word count, Word seed, Word *output) {
    using namespace material::scree;
    Word thread = tid(), id = thread >> 5, lane = thread & 31;
    if (id >= count)
        return;
    Word k = mix(seed + id * 7417), r = mix(k + lane * 519), who = k % n;
    Word retinal = (r & 3) ? 256 | (r >> 24) : 0, smell = (r >> 9) & 3, catalyst = (r >> 15) & 3, grip = (r >> 17) & 31;
    Word east_eye = raft_shuffle(retinal, material::next_site(lane, 1)),
         south_eye = raft_shuffle(retinal, material::next_site(lane, 2));
    bool east_gate, south_gate, taste, controlled, motion;
    if constexpr (packed) {
        Word table = pairs[who];
        east_gate = passes(table, retinal, east_eye, 1);
        south_gate = passes(table, retinal, south_eye, 2);
        controlled = (table >> (16 + 4 * catalyst + smell)) & 1;
        taste = (odors[who] >> smell) & 1;
        motion = (moves[who] >> grip) & 1;
    } else {
        auto &p = pupils[who];
        auto &f = p.mind.memory;
        Word release = p.mind.genes.release;
        east_gate = guessed(f, release, retinal, east_eye, 1);
        south_gate = guessed(f, release, retinal, south_eye, 2);
        controlled = material::scree::fires(p.exploratory.chemistry, 4 * catalyst + smell, release);
        taste = material::scree::fires(f.appetite, smell, release);
        motion = material::scree::fires(f.muscles[grip >> 4], grip & 15, release);
    }
    Picture e{ballot(east_gate), ballot(south_gate)};
    Word light = 1u << (k & 31);
    for (Word t = 0; t < 6; ++t)
        light = propagate(light, e);
    Word release = ballot(controlled), jaw = ballot(motion), odor = ballot(taste);
    if (!lane)
        output[id] = (light & release) ^ (jaw + rotate(odor, 7));
}
} // namespace
#define WORKSITE_KERNEL(name, packed)                                                                                  \
    KERNEL void name(const material::FieldPupil *p, Word n, const Word *pairs, const Word *moves,                      \
                     const unsigned char *odor, Word calls, Word seed, Word *out) {                                    \
        worksite_predictions<packed>(p, n, pairs, moves, odor, calls, seed, out);                                      \
    }
WORKSITE_KERNEL(flood_worksite_writable, false)
WORKSITE_KERNEL(flood_worksite_packed, true)
#undef WORKSITE_KERNEL
