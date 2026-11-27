#include "practice_card.cuh"
namespace axion::life::practice {
AX_FN void furnish(Bench &b, Word world, Word fixture, Word n, Word hidden) {
    auto &physical = b.physical;
    Word habitats[2];
    linkage::Material law[2][4];
    climate(world, habitats, law);
    b.climate = hidden;
    b.posterior = b.remembered = 0;
    b.stage = b.known = b.used = 0;
    b.report = {};
    b.pending = {};
    b.explanation = {};
    Word key = mix(world + fixture * 5581), table = (key >> 11) & 1;
    auto demand = apparatus(key, n, law[table]);
    b.allowance = demand.work;
    physical.world = world;
    for (Word g = 0; g < 4; ++g)
        physical.layers[g] = law[hidden][g];
    linkage::place(physical, fixture, n);
    physical.bench = {demand.rig, demand.sockets, 1, position(demand.rig, demand.view, law[hidden]), demand.goal};
    b.public_view = demand;
    b.public_view.goal = {};
    b.watch = 4;
}
AX_FN Word handle_room(Bench &b, Saved &student, Word rounds, Word eyes) {
    if (!eyes)
        return 0;
    auto &r = b.physical;
    Word funded = r.tutor / 132;
    if (rounds > funded)
        rounds = funded;
    Word exp = handle(student.mind, {student.bank, 1, student.arena}, r.layers, mix(r.world + r.round * 4853), rounds,
                      informed, all);
    r.tutor -= exp;
    r.demonstrated += exp;
    r.round += rounds;
    return exp;
}
AX_FN effort::Speech inquiry_words(const linkage::Rack &r, const linkage::Tongue &speaker, Word stroke, Word watch) {
    using namespace linkage;
    effort::Speech q{};
    if (!sound(q, verb(speaker, effort::fold)) || !sound(q, name(speaker, bits(r.labels, stroke >> 1))) ||
        !sound(q, name(speaker, 4 + bits(r.buttons, stroke))) || !sound(q, verb(speaker, effort::material)) ||
        !sound(q, name(speaker, bits(r.labels, watch))))
        return {};
    return q;
}
AX_FN effort::Speech label_face(const linkage::Rack &r, const linkage::Tongue &v, Word part, Word f) {
    using namespace linkage;
    effort::Speech s{};
    if (!sound(s, verb(v, effort::material)) || !sound(s, name(v, bits(r.labels, part))) || !sound(s, name(v, 4 + f)))
        return {};
    return s;
}
// This side receives public sounds. The material state and the expected branch
// of the worker are absent from its arguments.
AX_FN Exchange hear_face(Bench &b, const linkage::Tongue &speaker, const effort::Speech &message, bool preview) {
    using namespace linkage;
    Exchange e{};
    e.heard = message;
    auto &r = b.physical;
    if (!b.known || (!preview && b.stage != 3) || message.n != 3 ||
        effort::gesture(speaker.operators, speaker.law, message.sounds[0]) != effort::material)
        return e;
    Word part = identify_part(r, feature(speaker, message.sounds[1]), 0xe4),
         mark = feature(speaker, message.sounds[2]);
    if (part != b.watch || part >= 4 || mark < 4 || mark >= 8)
        return e;
    e.recognised = 1;
    e.seen = mark - 4;
    auto program = decode(b.explanation.bytes, e.seen);
    e.said = pronounce(r, speaker, program);
    e.decoded = program.found;
    if (!preview) {
        Word plausible = 0;
        for (Word i = 0; i < 4; ++i)
            if ((b.posterior & (1u << i)) && bits(b.position[i], b.watch) == e.seen)
                plausible |= 1u << i;
        b.posterior = plausible;
        b.report = message;
        b.pending = e.said;
        b.stage = e.said.n ? 2 : 0;
    }
    return e;
}
AX_FN Exchange discuss(Bench &b, const Saved &student, const linkage::Tongue &speaker, const effort::Speech &message,
                       Word mode) {
    using namespace linkage;
    Exchange e{};
    auto &r = b.physical;
    if (!message.n)
        return e;
    Word op = effort::gesture(speaker.operators, speaker.law, message.sounds[0]);
    if (op == effort::material && message.n == 3)
        return hear_face(b, speaker, message, false);
    if (op == effort::explain) {
        if (!b.known)
            return e;
        if (message.n == 3) {
            effort::Speech reference = message;
            reference.sounds[0] = verb(speaker, effort::material);
            return hear_face(b, speaker, reference, true);
        }
        if (message.n == 1 && b.watch < 4) {
            e.recognised = sound(e.said, name(speaker, bits(r.labels, b.watch)));
        }
        return e;
    }
    if (op == effort::compare && message.n >= 3 && (message.n & 1)) {
        Pattern want{};
        for (Word j = 1; j < message.n; j += 2) {
            Word pos = identify_part(r, feature(speaker, message.sounds[j]), 0xe4),
                 f = feature(speaker, message.sounds[j + 1]);
            if (pos >= 4 || f < 4 || f >= 8 || bits(want.care, pos))
                return e;
            want.faces = replace(want.faces, pos, f - 4);
            want.care |= 3u << (2 * pos);
        }
        b.public_view.goal = want;
        r.block = 0;
    } else if (op == effort::remove && message.n == 2 && b.public_view.goal.care) {
        Word p = identify_part(r, feature(speaker, message.sounds[1]), 0xe4);
        if (p >= 4)
            return e;
        r.block |= 3u << (2 * p);
    } else if (op != effort::compare || message.n != 1 || !b.public_view.goal.care)
        return e;
    Request request = b.public_view;
    request.rig.handles &= ~r.block;
    request.work = b.allowance - b.used;
    if (!r.bench.spool)
        request.sockets = 0;
    // The caller supplies only the shared diagram, goal, written hypotheses and remaining work.
    auto &m = student.mind;
    Charts charts{};
    Word starts[4]{}, ids[4]{};
    if (!b.remembered)
        b.posterior = (1u << size(m)) - 1;
    for (Word j = 0; j < size(m); ++j)
        if (b.posterior & (1u << j)) {
            Word k = charts.count++;
            charts.model[k] = read(student.bank[j], m.genes);
            ids[k] = j;
            starts[k] = b.remembered ? b.position[j] : request.view;
        }
    auto plan = reason(m, charts, request, request.rig.handles, mix(r.fixture + 131), mode, starts);
    if (plan.found && plan.ask < 8)
        for (Word j = 0; j < charts.count; ++j)
            b.expected[ids[j]] = linkage::predict(charts.model[j], request.rig, starts[j], plan.ask);
    b.explanation = encode(plan);
    b.known = plan.found;
    b.watch = plan.look;
    e.nbytes = b.explanation.size;
    e.recognised = 1;
    if (plan.found && plan.ask < 8) {
        e.said = inquiry_words(r, speaker, plan.ask, plan.look);
        e.asked = 1;
        e.stroke = plan.ask;
        e.watch = plan.look;
        b.stage = 1;
    } else {
        e.said = pronounce(r, speaker, decode(b.explanation.bytes, 0));
        b.stage = e.said.n ? 2 : 0;
    }
    b.pending = e.said;
    return e;
}
AX_FN Exchange listen(Bench &b, const linkage::Tongue &worker, effort::Speech wire, Word purse, Word senses,
                      Word mode) {
    using namespace linkage;
    Exchange e{};
    auto &r = b.physical;
    e.heard = wire;
    if ((senses & 3) != 3 || wire.n != 5 ||
        effort::gesture(worker.operators, worker.law, wire.sounds[0]) != effort::fold ||
        effort::gesture(worker.operators, worker.law, wire.sounds[3]) != effort::material || !purse ||
        b.used >= b.allowance)
        return e;
    Word touch = identify_part(r, feature(worker, wire.sounds[1]), r.reader), mark = feature(worker, wire.sounds[2]),
         look = identify_part(r, feature(worker, wire.sounds[4]), r.reader);
    if (touch > 3 || look > 3 || mark < 4 || mark >= 8)
        return e;
    Word part = bits(r.reader, touch), observed = bits(r.reader, look), h = 0;
    while (h < 2 && bits(r.buttons, part * 2 + h) != mark - 4)
        ++h;
    if (h == 2 || !(r.bench.rig.handles & ~r.block & (1u << (part * 2 + h))))
        return e;
    e.stroke = part * 2 + h;
    e.local = touch * 2 + h;
    e.watch = observed;
    e.recognised = 1;
    e.spent = 1;
    ++b.used;
    e.start = expose(r.bench.rig, r.bench.pose, r.layers);
    r.bench.pose = linkage::press(r.bench.rig, r.bench.pose, e.stroke, r.layers);
    e.after = expose(r.bench.rig, r.bench.pose, r.layers);
    e.seen = bits(e.after, observed);
    // The instructor learns which face changed only through this acoustic report.
    if (mode != cut)
        e.said = label_face(r, worker, observed, (e.seen + Word(mode == bad_report)) & 3);
    return e;
}
AX_FN Exchange deliver(Bench &b, const linkage::Tongue *tongues, Word purse, Word senses, Word mode) {
    using namespace linkage;
    Exchange e{};
    auto &r = b.physical;
    e.earlier = b.pending;
    if (b.stage == 1) {
        auto wire = b.pending;
        if (mode == mute_query && wire.n)
            wire.sounds[1] = 0;
        auto observed = listen(b, tongues[1], wire, purse, senses, mode);
        e = observed;
        e.earlier = wire;
        e.heard = observed.said;
        if (observed.recognised) {
            b.remembered = 1;
            for (Word j = 0; j < 4; ++j)
                b.position[j] = b.expected[j];
            b.stage = 3;
            auto message = hear_face(b, tongues[0], observed.said, false);
            e.said = message.said;
            e.decoded = message.decoded;
            if (!message.recognised) {
                b.pending = {};
                b.stage = 3;
            }
        }
        return e;
    }
    if (b.stage != 2)
        return e;
    auto wire = b.pending;
    if (mode == alter_request && wire.n > 1)
        wire.sounds[1] = 0;
    Uttered used{};
    Word balance = b.allowance - b.used;
    Word predicted_cost = wire.n / 2;
    if (wire.n > 2 && feature(tongues[1], wire.sounds[2]) < 4)
        ++predicted_cost;
    if (predicted_cost <= balance)
        enact(r, tongues[1], wire, purse, senses, r.reader, used);
    e.start = b.public_view.view;
    e.after = expose(r.bench.rig, r.bench.pose, r.layers);
    e.recognised = used.executed;
    e.spent = used.spent;
    e.food = used.food;
    e.fitted = used.inserted;
    e.steps = used.steps;
    e.arrived = used.executed && matches(e.after, b.public_view.goal);
    e.meaningful = used.executed && e.after != e.start;
    e.heard = wire;
    b.used += used.spent;
    if (used.executed) {
        b.public_view.view = e.after;
        b.public_view.rig = r.bench.rig;
        if (b.remembered)
            for (Word j = 0; j < 4; ++j)
                b.position[j] = e.after;
    }
    b.stage = 0;
    b.pending = {};
    return e;
}
} // namespace axion::life::practice
KERNEL void practice_interview(practice::Bench *lab, practice::Saved *student, const linkage::Tongue *tongues,
                               Word action, Word p, Word q, Word hidden, effort::Speech said, Word budget,
                               Word senses, practice::Exchange *out) {
    using namespace practice;
    auto &b = *lab;
    Exchange v{};
    if (action == enter) {
        b = {};
        furnish(b, p, 0, q, 0);
    } else if (action == layout)
        furnish(b, b.physical.world, p, q, hidden);
    else if (action == water) {
        furnish(b, b.physical.world, b.physical.fixture, b.physical.width, b.climate ^ 1);
    } else if (action == tuition)
        v.spent = handle_room(b, *student, p, senses & 1);
    else if (action == current_order)
        v.said = linkage::describe(b.physical, tongues[0], b.physical.bench.lock, effort::compare);
    else if (action == utter && (senses & 1))
        v = discuss(b, *student, tongues[0], said, all);
    else if (action == last_report)
        v.said = b.report;
    else if (action == lock_escrow && !b.known && !b.used && q &&
             !(q & ~linkage::mask_parts(b.physical.bench.rig.parts))) {
        bool complete = true;
        for (Word i = 0; i < 4; ++i)
            if (linkage::bits(q, i) != 0 && linkage::bits(q, i) != 3)
                complete = false;
        if (complete) {
            b.physical.bench.lock = {p, q};
            b.allowance = hidden & 31;
            b.physical.bench.spool = (hidden >> 8) & 1;
            v.recognised = 1;
        }
    } else if (action >= pass && action <= mute_query)
        v = deliver(b, tongues, budget, senses, action);
    *out = v;
}
KERNEL void practice_conversations(const practice::Saved *source, const linkage::Tongue *tongues,
                                   const practice::Invitation *invites, Word n, Word balance, Word organs,
                                   practice::Record *records) {
    using namespace practice;
    Word i = tid();
    if (i >= n)
        return;
    auto t = invites[i];
    auto &report = records[i];
    report = {};
    practice::Bench b{};
    auto pupil = *source;
    furnish(b, t.world, 0, t.width, 0);
    Word learned = handle_room(b, pupil, t.practice, organs & 1);
    furnish(b, t.world, 0, t.width, 1);
    learned += handle_room(b, pupil, t.practice, organs & 1);
    furnish(b, t.world, t.fixture, t.width, t.weather);
    b.allowance = t.wallet;
    b.physical.bench.spool = t.spool;
    b.physical.bench.lock = {t.lock, t.care};
    if (t.wire == 6 && size(pupil.mind) > 1) {
        Word keep = active(pupil.mind);
        pupil.bank[0] = pupil.bank[keep];
        pupil.mind.banks = 1;
    }
    // A spoken request defines the external work order. The physical escrow
    // pays on its final mark arrangement, verified separately by the host.
    Word mode = t.wire == 4 ? all & ~Word(probing) : t.wire == 5 ? all | random_probes : all;
    auto &r = b.physical;
    report.before = b.public_view.view;
    report.glyphs = r.bench.rig.glyphs;
    report.edges = r.bench.rig.joints;
    report.handles = r.bench.rig.handles;
    report.labels = r.labels;
    report.buttons = r.buttons;
    report.reader = r.reader;
    if (organs & 1)
        report.source = discuss(b, pupil, tongues[0], t.request, mode);
    Word wire = t.wire == 1   ? cut
                : t.wire == 2 ? bad_report
                : t.wire == 3 ? mute_query
                : t.wire == 7 ? alter_request
                              : pass;
    if (b.stage == 1)
        report.inspection = deliver(b, tongues, balance, organs >> 3, wire);
    if (t.revision.n && (organs & 1)) {
        report.amendment = discuss(b, pupil, tongues[0], t.revision, mode);
        if (b.stage == 1)
            report.second = deliver(b, tongues, balance - b.used, organs >> 3, wire);
    }
    report.goal = b.public_view.goal.faces;
    report.care = b.public_view.goal.care;
    if (b.stage == 2)
        report.worker = deliver(b, tongues, balance - b.used, organs >> 3, wire);
    report.after = linkage::expose(r.bench.rig, r.bench.pose, r.layers);
    report.spool = r.bench.spool;
    report.reserve = balance - b.used + report.worker.food;
    report.tutor = learned;
    report.models = size(pupil.mind);
}
