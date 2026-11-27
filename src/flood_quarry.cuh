// One animal works inside a finite eight-by-four bed. A motor addresses a
// local operation and a separate, mutable motor output moves the body.
// All products come from the same pairwise laws used in the open watershed.
namespace axion::life::material {
struct Quarry {
    Grain bed[32]{};
    Byte site = 0, heading = 1, sink = 0, spring = 0;
    Half raw = 16, food = 0;
};
AX_FN Word next_site(Word pos, Word dir) {
    Word x = pos & 7, y = pos >> 3;
    return ((x + (dir == 1) - (dir == 3)) & 7) | (((y + (dir == 2) - (dir == 0)) & 3) << 3);
}
AX_FN Quarry make_quarry(Word seed, Word span) {
    Quarry q;
    Word k = mix(seed + 491);
    q.spring = static_cast<Byte>(k & 31);
    Word d = (k >> 8) & 3, site = q.spring;
    q.bed[site] = {0xff, 0x37, 0, 0};
    // Each seeded bed has its own stock and geometry. Its experimental twins
    // receive the identical inventory. The builder can move every grain.
    for (Word j = 0; j < span; ++j) {
        site = next_site(site, d);
        q.bed[site] = {0xff, 0x2b, 0, 0};
        q.bed[site].faces = static_cast<Byte>(0xf0u | (draw(k) & 15));
    }
    q.sink = static_cast<Byte>(next_site(site, d));
    q.bed[q.sink] = {0xff, 0x20, 0, 0};
    for (Word j = 0; j < 18; ++j) {
        Word at = draw(k) & 31;
        if (!q.bed[at].faces && (k & 3))
            q.bed[at] = {static_cast<Byte>(1 + (draw(k) % 255)), static_cast<Byte>(0x20 | (draw(k) & 15)), 0, 0};
    }
    Word joint = q.spring, count = 1 + draw(k) % span;
    for (Word j = 0; j < count; ++j)
        joint = next_site(joint, d);
    Word local = (draw(k) >> 11) & 3, side = (d + (local & 1 ? 1 : 3)) & 3, loose = next_site(joint, side);
    q.site = static_cast<Byte>(joint);
    q.heading = static_cast<Byte>(d);
    if (local < 2)
        q.bed[joint].faces = static_cast<Byte>(rotate_faces(0x33, d & 1 ? 0 : 1));
    else {
        auto fallback = q.bed[loose];
        if (fallback.faces) {
            for (Word j = 0; j < 32; ++j)
                if (!q.bed[j].faces) {
                    q.bed[j] = fallback;
                    break;
                }
        }
        q.bed[loose] = q.bed[joint];
        q.bed[joint] = {};
        q.site = static_cast<Byte>(local == 2 ? joint : loose);
        q.heading = static_cast<Byte>(local == 2 ? side : (side ^ 2));
    }
    return q;
}
AX_FN Word links(const Quarry &q, Word site) {
    Word n = 0;
    for (Word d = 0; d < 4; ++d)
        n += bonded(q.bed[site], q.bed[next_site(site, d)], d);
    return n;
}
AX_FN Word spanning(const Quarry &q) {
    if (!q.bed[q.spring].faces || !q.bed[q.sink].faces)
        return 0;
    Word front = 1u << q.spring, seen = front;
    for (Word dist = 0; front; ++dist) {
        if (front & (1u << q.sink))
            return dist + 1;
        Word next = 0;
        while (front) {
            Word at = __builtin_ctz(front);
            front &= front - 1;
            for (Word d = 0; d < 4; ++d) {
                Word j = next_site(at, d);
                if (!(seen & (1u << j)) && bonded(q.bed[at], q.bed[j], d))
                    next |= 1u << j;
            }
        }
        front = next;
        seen |= next;
    }
    return 0;
}
AX_FN MillBook chemistry(Quarry &q, Word offset, Word ticks, Word open) {
    MillBook b;
    for (Word t = 0; t < ticks; ++t) {
        Word phase = t + offset, d = (phase & 2) ? 2 : 1;
        for (Word at = 0; at < 32; ++at) {
            if ((((d == 1 ? at : at >> 3) ^ phase) & 1) == 0)
                b.flow += conduct(q.bed[at], q.bed[next_site(at, d)], d, open);
        }
        for (Word at = 0; at < 32; ++at) {
            Half none = 0, dummy = 0;
            auto &raw = at == q.sink ? q.raw : none;
            auto &product = at == q.sink ? q.food : dummy;
            auto v = excite(q.bed[at], at == q.spring ? 3 : 4, 0, raw, product);
            b.sun += v.light;
            b.product += v.reaction;
            b.folds += v.folds;
        }
    }
    return b;
}
AX_FN Temper operate(Worker &p, Quarry &q, Word key, Word reserve, Word &op) {
    Word site = q.site, neighbor = next_site(site, q.heading),
         environment = (q.bed[site].charge ? 1 : 0) | (Word(q.raw && site == q.sink) << 1);
    Word view = sense(q.bed[site], q.bed[neighbor], p.hand, q.heading, environment),
         sensory = index(view, p.genes.receptors);
    Word basal = (p.genes.innate >> (3 * sensory)) & 7, motor = choose(p, view, basal, key);
    op = effector(p.genes, motor);
    auto event = manipulate(q.bed[site], q.bed[neighbor], p.hand, op, q.heading, links(q, site), links(q, neighbor),
                            reserve, environment);
    practice(p, motor, event);
    // Locomotion contacts share the motor volley. All eight actions can
    // acquire distinct rotations, strides and manipulation couplings.
    Word travel = (p.genes.receptors >> (3 * motor)) & 7;
    q.heading = static_cast<Byte>((q.heading + (travel & 3)) & 3);
    if (travel & 4)
        q.site = static_cast<Byte>(next_site(site, q.heading));
    return event;
}
AX_FN Word retained(const Quarry &q) {
    Word amount = q.raw + q.food;
    for (const auto &p : q.bed)
        amount += p.charge;
    return amount;
}
} // namespace axion::life::material
KERNEL void flood_quarry_season(const Brood *eggs, Brood *brood, material::Season *books, Word n, Word seed, Word gen,
                                Word moves, Word span, Word foster, Word duration) {
    Word i = tid();
    if (i >= n)
        return;
    Word lane = i & 31, island = i / 32, weather = mix(seed ^ mix(gen + 711) ^ mix(island + 9131));
    auto egg = eggs[i];
    material::Worker p{};
    p.genes = egg.genes;
    auto q = material::make_quarry(mix(weather + lane * 1723), span);
    Word op = 0;
    Word energy = egg.live ? 24 : 0;
    material::Season book;
    book.initial = energy + q.raw;
    // Loose food deposited by the tide can carry a lineage through a season
    // with a poor device. Its small stock is counted and exhausted on eating.
    Word strand = 8 + (mix(weather + lane * 31) & 7);
    book.initial += strand;
    if (energy) {
        energy += strand;
        strand = 0;
    }
    for (Word j = 0; j < moves && energy; ++j) {
        auto e = material::operate(p, q, mix(weather ^ (lane * 5783 + j * 199)), energy, op);
        energy -= e.spent;
        book.work += e.spent;
        book.heat += e.spent;
        book.moves += e.performed;
        auto r = material::chemistry(q, j, 1, 1);
        book.brightness += r.sun;
        book.product += r.product;
        if (q.food) {
            energy += q.food;
            q.food = 0;
            if ((p.genes.observation & (1u << 17)) && e.performed)
                material::witness(p, e.cue, e.change);
        }
        if ((j & 3) == 3 && energy) {
            --energy;
            ++book.heat;
        }
    }
    auto last = material::chemistry(q, moves, duration, 1);
    book.brightness += last.sun;
    book.product += last.product;
    if (energy) {
        energy += q.food;
        q.food = 0;
    }
    book.residual = energy + strand + material::retained(q) + p.hand.charge;
    disperse_builders(egg, energy, weather, seed, gen, i, n, foster, brood, books, book);
}
KERNEL void flood_quarry_exam(const material::Genes *pool, Word donors, material::QuarryReading *out, Word n, Word seed,
                              Word moves, Word lifespan, Word gap) {
    Word i = tid();
    if (i >= n)
        return;
    Word key = mix(seed + i * 19741);
    auto q = material::make_quarry(key, gap), cold = q;
    material::Worker p{};
    p.genes = pool[mix(key + 97) % donors];
    Word op = 0, energy = 8 * moves + 16, delta = 0;
    material::QuarryReading e;
    for (Word t = 0; t < moves; ++t) {
        auto event = material::operate(p, q, mix(key + t * 1583), energy, op);
        energy -= event.spent;
        delta += event.spent;
        e.turns += event.performed && (op == 1 || op == 2);
        e.shifts += event.performed && op >= 3 && op <= 6;
        e.exchanges += event.performed && op == 7;
    }
    auto isolated = q;
    auto warm = material::chemistry(q, 0, lifespan, 1), before = material::chemistry(cold, 0, lifespan, 1),
         cut = material::chemistry(isolated, 0, lifespan, 0);
    e.food = warm.product;
    e.initial_food = before.product;
    e.insulated = cut.product;
    e.reactors = warm.product != 0;
    e.pioneer = warm.product && !before.product && !cut.product;
    e.improved = warm.product > before.product ? warm.product - before.product : 0;
    e.harm = before.product > warm.product ? before.product - warm.product : 0;
    for (auto &part : q.bed)
        e.handled += bool(part.marks & 2);
    Word length = material::spanning(q);
    e.spanning = length != 0;
    e.length = length;
    out[i] = e;
}
