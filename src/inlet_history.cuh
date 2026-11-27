struct PriorInlets {
    Word observed = 0, prior = 0, nearby = 0, reaches = 0;
};
AX_FN PriorInlets first_inlet(World w, Configuration c, drift::Gaze gaze, drift::Sampling sample, Word id, Word row,
                              Word origin, Word channels, Word available, Word previous, Word wiring, Word ports,
                              Word draw, Word axes, Word issued, Word outlet, Word swallowed, Word local_food,
                              regulation::Program program, bool spill, Word other_options = ~0u) {
    auto study = gaze.first;
    const auto &body = w.animals[id];
    if (!study.visits || !body.body.eyes || (study.start && body.born <= study.start))
        return {};
    Word k = (id * 0x9e3779b1u + study.salt) & (c.side * c.side - 1);
    if (k >= study.size)
        return {};
    auto &history = study.visits[k];
    if (history.born != body.born)
        history = {body.born};
    if (!history.reaches || history.origin != origin || history.channels != channels) {
        history.nearby = 0;
        history.origin = origin;
        history.channels = channels;
    }
    PriorInlets seen{1, history.visited, history.nearby, history.reaches};
    Word bit = 1u << row;
    history.visited |= bit;
    history.nearby |= bit;
    ++history.reaches;
    if ((seen.prior & seen.nearby & bit) != 0)
        return seen;
    Word primary = drift::choose(drift::openings(previous, row, wiring, 0), draw);
    Word rotated = drift::choose(
        other_options != ~0u ? other_options : drift::openings(previous, row, wiring, ports ^ drift::misaligned), draw);
    bool right = drift::face(axes, issued) == outlet;
    bool certain = available && !(available & (available - 1));
    bool carrier = sample.has_donor && drift::matches(sample.donor, program, w.braids.minds[id].genes);
    for (Word scope = 0; scope < 2; ++scope) {
        Word mask = scope ? seen.nearby : seen.prior;
        if (mask & bit)
            continue;
        for (Word group = 0; group <= Word(carrier); ++group) {
            Word address = (k & 127u) * 32 + (scope * 2 + group) * 4 + nbits(mask);
            for (Word held = 0; held <= Word(spill); ++held) {
                auto &t = study.counts[address + 16 * held];
                count(t.total, 1);
                count(t.right, right);
                count(t.local, drift::face(axes, primary) == outlet);
                count(t.rotated, drift::face(axes, rotated) == outlet);
                count(t.changed, issued != primary);
                count(t.certain, certain);
                count(t.certain_right, certain && right);
                count(t.food, swallowed);
                count(t.proximal_food, local_food);
            }
        }
    }
    return seen;
}
