namespace axion::life::runnel {
AX_FN void coupon_lesson(Flume &f, Mind &m, Word cue, Word visible) {
    for (Word p = 0; p < 4; ++p) {
        if (f.tutor < 2)
            return;
        f.tutor -= 2;
        f.tuition += 2;
        Word response = flex(f.stone, cue, p);
        if (visible)
            observe(m, cue, p, m.genes.hand & 7, response, true, live);
    }
}
AX_FN Colloquy meeting(const Mind *g, const linkage::Tongue *voice, Word key, Word twin, Word arm, Word chiral,
                       Word organs) {
    Flume f{};
    Word plan = mix(key + 7937), pin = landmark(plan, 1), cue = local_cue(pin), d = upstream(pin);
    Word x = cue & 3, y = cue >> 2, center = (y + 1 + ((key >> 5) % 3)) & 3, mentor = (x + 1 + ((key >> 21) % 3)) & 3;
    Word third = (mentor + 1) & 3;
    if (third == x)
        third = (third + 1) & 3;
    Word a = texture(x, center), b = texture(mentor, center), c = texture(mentor, y), e = texture(third, center),
         later = texture(third, y);
    Colloquy v{};
    Word world = mix(key + 6637);
    layout(f, world, plan, 2, 1024, 0, chiral);
    if (twin)
        f.stone ^= 2u << (8 + 2 * y);
    f.cursor = 1;
    f.indirect = arm != 1;
    Mind pupil{g[0].genes}, advisor{g[1].genes};
    pupil.genes.hand = (pupil.genes.hand & ~7u) | ((pupil.genes.hand ^ key) & 7);
    advisor.genes.hand = (advisor.genes.hand & ~7u) | ((advisor.genes.hand ^ (key >> 5)) & 7);
    coupon_lesson(f, pupil, a, (organs & 1) && arm != 4);
    coupon_lesson(f, pupil, b, (organs & 1) && arm != 5);
    coupon_lesson(f, pupil, e, organs & 1);
    coupon_lesson(f, advisor, c, (organs & 8) && arm != 6);
    v.tuition = f.tuition;
    v.geology = f.stone;
    v.opening = cue;
    v.other = later;
    v.corner = b;
    v.reference = c;
    v.direction = d;
    v.frame = pupil.genes.hand & 7;
    v.other_frame = advisor.genes.hand & 7;
    v.order = request(f, voice[0]);
    Dispatch first{};
    answer(f, pupil, voice[0], v.order, first);
    v.started = first.understood;
    v.first = first.reply;
    for (Word i = 0; i < 18 && f.stage; ++i) {
        Word stage = f.stage;
        auto wave = f.pending;
        Dispatch part{};
        Word control = (arm == 2 ? 1u : arm == 3 ? 2u : arm == 7 ? 3u : 0u);
        flume_pass(f, pupil, advisor, voice, control, 8 - v.paid, (organs >> 3) & 7, part);
        if (stage == 1) {
            ++v.consultations;
            if (part.reported) {
                v.enquiry = wave;
                v.testimony = part.reply;
                ++v.said;
            }
        }
        if (stage == 2) {
            v.learned += part.learned;
            if (part.learned)
                ++v.heard;
        }
        if (stage == 3)
            v.procedure = wave;
        v.paid += part.paid;
        v.installed += part.installed;
    }
    v.repaired = f.retry;
    effort::Speech why{};
    linkage::sound(why, linkage::verb(voice[0], effort::explain));
    Dispatch reason{};
    answer(f, pupil, voice[0], why, reason);
    v.reason = reason.reply;
    v.reason_via = reason.example;
    v.rotation = reason.transform;
    Word gesture = f.fold[1] ? f.fold[1] - 1 : 4;
    Word mask = gesture < 4 ? contact(f.stone, cue, gesture) : 3;
    v.public_pose = gesture;
    v.succeeded = gesture < 4 && mask == (1u << d);
    Byte gate[]{1, Byte(d ? ((mask >> 1) | ((mask & 1) << 1)) : mask)};
    v.food = pump(gate, 2, 48, true).food;
    // A fresh apparatus presents a different surface pair, sharing only the
    // learned relation. The consultant's tissue is removed and its reply wire muted.
    Word terrain = mix(key + 9811);
    for (Word i = 0; i < 8192; ++i, ++terrain) {
        auto z = landmark(terrain, 1);
        if (local_cue(z) == later && upstream(z) == d)
            break;
    }
    f.layout = terrain;
    f.stage = 0;
    f.cursor = 1;
    f.fold[1] = 0;
    f.unavailable = 0;
    advisor.cells = {};
    Dispatch next{};
    auto changed = request(f, voice[0]);
    answer(f, pupil, voice[0], changed, next);
    v.later_ask = next.asked;
    v.later_via = next.example;
    v.later_rotation = next.transform;
    v.later = next.reply;
    if (f.stage == 3) {
        Dispatch step{};
        flume_pass(f, pupil, advisor, voice, 0, 8, (organs >> 3) & 7, step);
        v.later_paid = step.paid;
    }
    bool open = f.fold[1] != 0;
    gesture = open ? f.fold[1] - 1 : 4;
    mask = open ? contact(f.stone, later, gesture) : 3;
    v.later_pose = gesture;
    v.later_success = open && mask == (1u << d);
    gate[1] = Byte(d ? ((mask >> 1) | ((mask & 1) << 1)) : mask);
    v.later_food = pump(gate, 2, 48, true).food;
    v.different = v.succeeded && v.later_success && v.public_pose != v.later_pose;
    return v;
}
} // namespace axion::life::runnel
KERNEL void flume_debates(const runnel::Mind *m, const linkage::Tongue *voices, Word cases, Word seed, Word mode,
                          Word chiral, Word organs, runnel::Colloquy *out) {
    Word i = tid();
    if (i >= 2 * cases)
        return;
    out[i] = runnel::meeting(m, voices, mix(seed + (i / 2) * 7351), i & 1, mode, chiral, organs);
}
KERNEL void flume_lexicon(const linkage::Tongue *v, runnel::Lexicon *out) {
    if (tid())
        return;
    runnel::Lexicon x{};
    for (Word k = 0; k < 2; ++k) {
        for (Word i = 0; i < 8; ++i)
            x.names[k].sounds[x.names[k].n++] = linkage::name(v[k], i);
        for (Word i = 0; i < 7; ++i)
            x.verbs[k].sounds[x.verbs[k].n++] = linkage::verb(v[k], i);
    }
    *out = x;
}
