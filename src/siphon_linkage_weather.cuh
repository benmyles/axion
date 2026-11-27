namespace axion::life::linkage {
struct Passage {
    Word accomplished[3]{}, offered[3]{}, heat[3]{}, constructed[3]{}, mistakes[3]{};
};
AX_FN Passage changing_material(Genes g, Word seed, Word cap, Word regime, Word width) {
    Material strata[2][4];
    for (Word medium = 0; medium < 2; ++medium)
        for (Word j = 0; j < 4; ++j)
            strata[medium][j] = matter(mix(seed + medium * 37879), j);
    Mind native{g};
    Tissue bank[4]{};
    Slots cells{bank, 1, cap};
    Passage score{};
    for (Word season = 0; season < 3; ++season) {
        const auto *law = strata[season == 1 && regime != 1];
        if (regime == 4) {
            native = {g};
            for (auto &t : bank)
                t = {};
        }
        Word daylight = mix(seed + season * 59707);
        if (season == 0 || regime != 3)
            tuition(native, cells, law, 15, daylight, (daylight & 1) ? watched : own, 2);
        auto chart = compile(native, cells);
        for (Word task = 0; task < 2; ++task) {
            auto stock = yard(mix(seed + season * 8887 + task * 7129), width ? width : 2 + task, law);
            auto &rig = stock.frame;
            Word initial = expose(rig, stock.pose, law);
            auto p = design(chart, rig, initial, stock.target, stock.sockets, g);
            Word available = 1;
            bool attached = insert(rig, stock.sockets, p.edit, available, law, 32);
            Word pose = stock.pose;
            for (Word j = 0; j < p.turns.len; ++j)
                pose = press(rig, pose, step(p.turns, j), law);
            bool fulfilled = matches(expose(rig, pose, law), stock.target);
            score.accomplished[season] += fulfilled;
            score.offered[season] += p.found;
            score.heat[season] += 2 * Word(attached) + p.turns.len;
            score.constructed[season] += attached && fulfilled;
            score.mistakes[season] += p.found && !fulfilled;
        }
    }
    return score;
}
AX_FN Word resources(Passage p) {
    Word wallet = 24;
    for (Word t = 0; t < 3; ++t) {
        wallet += p.accomplished[t] * 40;
        Word paid = 10 + p.heat[t];
        wallet -= wallet < paid ? wallet : paid;
    }
    return wallet;
}
} // namespace axion::life::linkage
KERNEL void linkage_changing_food(linkage::Family *pop, Word count, Word seed, Word epoch, Word cap, Word climate) {
    Word i = tid();
    if (i >= count)
        return;
    auto k = mix(seed + (i / 2) * 22621 + epoch * 21557);
    pop[i].eggs = 1 + linkage::resources(linkage::changing_material(pop[i].g, k, cap, climate, 0)) / 8;
}
KERNEL void linkage_revisers(const linkage::Genes *genes, Word count, Word seed, Word cap, Word *scores) {
    Word i = tid();
    if (i >= count)
        return;
    Word food = 0;
    for (Word j = 0; j < 8; ++j)
        food += linkage::resources(linkage::changing_material(genes[i], mix(seed + 4513 * j), cap, 2, 3 + (j & 1)));
    scores[i] = food;
}
KERNEL void linkage_changed(const linkage::Genes *genes, Word ng, Word count, Word seed, Word cap, Word mode,
                            Word width, Word *sums) {
    Word i = tid();
    if (i >= count)
        return;
    auto k = mix(seed + i * 6841);
    auto p = linkage::changing_material(genes[mix(k + 1987) % ng], k, cap, mode, width);
    for (Word s = 0; s < 3; ++s) {
        Word *row = sums + 15 * i + s * 5;
        row[0] = p.accomplished[s];
        row[1] = p.offered[s];
        row[2] = p.heat[s];
        row[3] = p.constructed[s];
        row[4] = p.mistakes[s];
    }
}
