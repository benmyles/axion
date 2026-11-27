namespace axion::life::linkage {
struct Stock {
    Rig frame{};
    Word sockets = 0, loop = 1, pose = 0;
    Pattern target{};
};
AX_FN Stock yard(Word seed, Word size, const Material *laws) {
    Stock s{};
    s.frame = layout(mix(seed + 8011), size, false);
    s.frame.joints = 0;
    Word k = mix(seed + 7057);
    s.pose = (k >> 9) & mask_parts(s.frame.parts);
    for (Word e = 0; e < 6; ++e)
        if (edge_b(e) < size) {
            Word h = mix(seed + e * 263);
            if (h % 5 < 2)
                join(s.frame, edge_a(e), edge_b(e), 1 + ((h >> 6) & 1));
            s.sockets |= 1u << e;
        }
    Word arms = 1 + Word(size > 2 && (k & 2));
    s.frame.handles = 0;
    for (Word j = 0; j < arms; ++j)
        s.frame.handles |= 3u << (2 * ((k / 37 + j) % size));
    Word practice = s.pose;
    auto copy = s.frame;
    Word spare = 1;
    if (k & 7)
        insert(copy, s.sockets, bump(s.sockets, k >> 17), spare, laws, 64);
    for (Word j = 0; j < 2 + ((k >> 4) % 8); ++j)
        practice = press(copy, practice, bump(copy.handles, mix(k + 251 * j)), laws);
    if (practice == s.pose)
        practice = press(copy, practice, bump(copy.handles, k), laws);
    s.target = {expose(copy, practice, laws), mask_parts(copy.parts)};
    if ((k & 31) == 1)
        s.target.faces = (k >> 17) & s.target.care;
    return s;
}
AX_FN Fabrication build(Genes genes, Word seed, Word n, Word mode, Word cap, Word repeats, bool grading,
                        Drawing *proof) {
    Material laws[4];
    for (Word g = 0; g < 4; ++g)
        laws[g] = matter(seed, g);
    auto stock = yard(seed, n, laws);
    Mind m{genes};
    Tissue bank[4]{};
    Slots slots{bank, 1, cap};
    tuition(m, slots, laws, types(stock.frame), seed,
            mode == public_shop      ? watched
            : mode == manual_shop    ? autonomous
            : mode == no_observation ? unschooled
                                     : own,
            repeats);
    auto model = compile(m, slots);
    if (mode == no_splice_model)
        for (Word &v : model.splice)
            v = 0;
    if (mode == no_relation_model)
        modify(model, seams_erased);
    if (mode == shuffled_splice)
        for (Word &word : model.splice) {
            Word out = 0;
            for (Word j = 0; j < 8; ++j) {
                if (word & (1u << (16 + j)))
                    out |= (1u << (16 + j)) | (((bits(word, j) + 1) % 3) << (2 * j));
            }
            word = out;
        }
    auto rig = stock.frame;
    Word tokens = (mode == empty_spool) ? 0u : 1u, first = expose(rig, stock.pose, laws);
    auto plan = design(model, rig, first, stock.target, (tokens && mode != only_spin) ? stock.sockets : 0, genes);
    Word budget = 64, angle = stock.pose, spent = 0;
    bool fitted = false;
    if (plan.found && plan.edit != no_edit)
        fitted = insert(rig, stock.sockets, plan.edit, tokens, laws, budget);
    spent += 2 * Word(fitted);
    Word actual_edges = rig.joints;
    if (mode == retained && fitted) {
        rig = stock.frame;
        ++tokens;
    }
    if (mode == borrowed_parts)
        for (Word j = 0; j < n; ++j)
            rig.glyphs = replace(rig.glyphs, j, (bits(rig.glyphs, j) + 1) & 3);
    for (Word j = 0; j < plan.turns.len; ++j) {
        angle = press(rig, angle, step(plan.turns, j), laws);
        ++spent;
    }
    Word after = expose(rig, angle, laws);
    Fabrication u{};
    u.n = 1;
    u.achieved = matches(after, stock.target);
    u.threaded = fitted;
    u.paid = spent;
    u.turned = plan.turns.len;
    u.carbon = u.achieved ? 40 : 0;
    u.unanswered = !plan.found;
    u.wrong = plan.found && !u.achieved;
    if (grading) {
        auto truth = true_map(laws);
        Genes ability = genes;
        ability.impulse |= 15u << 8;
        auto without = solve_fixed(truth, stock.frame, first, stock.target, ability);
        auto optimum =
            design(truth, stock.frame, first, stock.target, mode == empty_spool ? 0 : stock.sockets, ability);
        u.possible = optimum.found;
        u.optimal = u.achieved && spent == optimum.effort;
        u.crucial = optimum.found && !without.found;
        u.crucial_solved = u.crucial && u.achieved;
        u.unnecessary = fitted && without.found;
        u.baseline = without.found;
        if (plan.edit != no_edit) {
            auto prediction = stock.frame;
            u.predicted = model_splice(model, prediction, plan.edit);
            u.actual = u.predicted && prediction.joints == actual_edges;
            u.false_joins = fitted && prediction.joints != rig.joints;
        }
        if (proof)
            *proof = {seed,
                      stock.frame.glyphs,
                      stock.frame.joints,
                      stock.sockets,
                      stock.frame.handles,
                      first,
                      stock.target.faces,
                      stock.target.care,
                      plan.edit,
                      actual_edges,
                      rig.joints,
                      after,
                      first,
                      tokens,
                      spent,
                      u.carbon,
                      optimum.found ? optimum.effort : ~0u,
                      without.found ? without.effort : ~0u,
                      plan.turns};
    }
    return u;
}
} // namespace axion::life::linkage
KERNEL void linkage_spools(const linkage::Genes *genes, Word ng, Word n, Word width, Word seed, Word mode, Word cap,
                           Word repeats, linkage::Fabrication *results, linkage::Drawing *proof) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 3739);
    results[i] =
        linkage::build(genes[mix(k + 6299) % ng], k, width, mode, cap, repeats, true, i < 3 ? proof + i : nullptr);
}
