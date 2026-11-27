#include "axion/linkage_culture.hpp"
namespace axion::life::linkage {
AX_FN Exchange hand_over(Genes builder, Genes reader, Word seed, Word width, Word mode, Word cap) {
    Material law[4];
    for (Word j = 0; j < 4; ++j)
        law[j] = matter(seed, j);
    auto stock = yard(seed, width, law);
    const auto bare = stock.frame;
    Word before = expose(bare, stock.pose, law), loose = 1;
    Mind producer{builder}, consumer{reader};
    Tissue t[4]{}, u[4]{};
    Slots a{t, 1, cap}, b{u, 1, cap};
    tuition(producer, a, law, types(bare), seed, own, 2);
    auto plan = design(compile(producer, a), bare, before, stock.target, stock.sockets, builder);
    auto article = bare;
    bool fitted = insert(article, stock.sockets, plan.edit, loose, law, 64);
    Exchange e{};
    e.samples = 1;
    e.drafted = plan.found;
    e.built = plan.found;
    e.threaded = fitted;
    e.maker_cost = 2 * Word(fitted);
    // The producer leaves before the first working stroke. The recipient has
    // its own private tissue, separate elementary experiences and one article.
    tuition(consumer, b, law, types(bare), mix(seed + 8761), mode == no_private_memory ? unschooled : own, 2);
    if (fitted && (mode == unstrung || mode == extra_loop)) {
        article = bare;
        ++loose;
        Word other = stock.sockets & ~(1u << plan.edit);
        if (mode == extra_loop && other)
            insert(article, other, bump(other, mix(seed + 451)), loose, law, 64);
    }
    if (mode == one_hand)
        article.handles &= 0x55;
    Pattern request = stock.target;
    if (mode == wrong_goal)
        request.faces ^= 3;
    auto recipient = seek(compile(consumer, b), article, before, request, reader);
    e.pupil_plans = recipient.found;
    Word actual = stock.pose;
    for (Word j = 0; j < recipient.course.len; ++j)
        actual = press(article, actual, step(recipient.course, j), law);
    e.paid = recipient.course.len;
    e.spool = loose;
    e.unvisited = 1;
    e.finished = matches(expose(article, actual, law), stock.target);
    e.food = e.finished ? 40 : 0;
    e.mistaken = e.pupil_plans && !e.finished;
    auto witness = true_map(law);
    Genes full = reader;
    full.impulse |= 15u << 8;
    auto start = solve_fixed(witness, bare, before, stock.target, full);
    auto ideal = design(witness, bare, before, stock.target, stock.sockets, full);
    e.available = ideal.found;
    e.indispensable = ideal.found && !start.found;
    e.indispensable_delivered = e.finished * e.indispensable;
    return e;
}
AX_FN Culture apprentice(const Genes *families, Word ng, Word seed, Word depth, Word width, Word practice, Word mode,
                         Word cap) {
    Material law[4];
    for (Word j = 0; j < 4; ++j)
        law[j] = matter(seed, j);
    Mind current{families[mix(seed + 859) % ng]};
    Tissue bank[4]{};
    Slots stored{bank, 1, cap};
    tuition(current, stored, law, 15, seed, own, 2);
    auto next = order(mix(seed + 281), width, law);
    Culture total{};
    total.n = 1;
    for (Word wave = 0; wave < depth; ++wave) {
        Mind novice{families[mix(seed + wave * 2473 + 4213) % ng]};
        Tissue room[4]{};
        Slots acquired{room, 1, cap};
        tuition(novice, acquired, law, 15, mix(seed + 2371 + wave), mode == no_priming ? unschooled : own, 2, false);
        auto teacher = compile(current, stored);
        Word reserve = 256, school_work = 0;
        for (Word j = 0; j < practice && reserve > 2; ++j) {
            Word k = mix(seed + wave * 17681 + j * 5347);
            auto task = order(k, 2, law, 1);
            auto course = seek(teacher, task.rig, expose(task.rig, task.angle, law), task.goal, current.genes);
            Word pose = task.angle;
            Word actual = 0;
            for (Word t = 0; t < course.course.len && reserve > 1; ++t) {
                Word motor = step(course.course, t);
                Word view = expose(task.rig, pose, law);
                pose = press(task.rig, pose, motor, law);
                Word after = expose(task.rig, pose, law);
                --reserve;
                ++school_work;
                ++actual;
                if (mode == obscured || mode == final_only)
                    continue;
                if (mode == wrong_actor)
                    motor ^= 2;
                Word flag = writable | witnesses | (mode == other_seam ? Word(other_eye) : 0u);
                total.edits += learn(novice, acquired, {task.rig, view}, motor, after, true, flag);
                ++total.seen;
            }
            if (mode == final_only && actual) {
                auto before = expose(task.rig, task.angle, law), after = expose(task.rig, pose, law);
                total.edits += learn(novice, acquired, {task.rig, before}, step(course.course, 0), after, true,
                                     writable | witnesses);
                ++total.seen;
            }
            bool harvest = actual && matches(expose(task.rig, pose, law), task.goal);
            reserve += 4 * Word(harvest);
            total.adults_fed += harvest;
            total.demonstrations += actual != 0;
        }
        total.cost += school_work;
        if (mode == owner_learn)
            tuition(novice, acquired, law, 15, seed + 2841, autonomous, 2);
        current = novice;
        for (Word j = 0; j < 4; ++j)
            bank[j] = room[j];
    }
    auto result = solve(current, stored, next, law, seed, own, true, nullptr);
    total.possible = result.possible;
    total.done = result.solved;
    total.long_tasks = result.multi;
    total.long_done = result.multi_solved;
    total.indirect = result.indirect;
    total.indirect_done = result.indirect_solved;
    total.requested = result.offered;
    total.food = result.food;
    total.productive = result.taps;
    total.missing = result.refused;
    return total;
}
} // namespace axion::life::linkage
KERNEL void linkage_handoffs(const linkage::Genes *g, Word ng, Word n, Word size, Word seed, Word mode, Word cap,
                             linkage::Exchange *rows) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 5527);
    rows[i] = linkage::hand_over(g[mix(k + 133) % ng], g[mix(k + 773) % ng], k, size, mode, cap);
}
KERNEL void linkage_culture(const linkage::Genes *g, Word ng, Word n, Word size, Word seed, Word mode, Word cap,
                            Word gen, Word exposure, linkage::Culture *rows) {
    Word i = tid();
    if (i >= n)
        return;
    rows[i] = linkage::apprentice(g, ng, mix(seed + i * 5351), gen, size, exposure, mode, cap);
}
