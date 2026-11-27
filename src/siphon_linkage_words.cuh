#include "axion/linkage_words.hpp"
namespace axion::life::linkage {
AX_FN void exhibit(Rack &r, Saved &native, Word repeats, bool self, Word reserve, Word senses, Uttered &out) {
    Slots slots{native.tissue, 1, native.arena};
    auto &m = native.mind;
    if (!(senses & 1) || (self && !(senses & 2)))
        return;
    Word wallet = self ? reserve : r.tutor, opening = wallet;
    Word ticket = mix(r.world + r.round * 5927);
    if (self) {
        for (Word t = 0; t < repeats && wallet; ++t) {
            Word k = mix(ticket + 1327 * t), g = k & 3;
            Rig x{3, g | ((k >> 4) & 12), 0, 15};
            Word seam = (k >> 11) % 3;
            join(x, 0, 1, seam);
            if ((k & 0x70) == 0x20 && wallet >= 2) {
                Word token = 1;
                insert(x, 1, 0, token, r.layers, wallet);
                out.associations += learn_join(m, slots, g, seam, true, joint(x.joints, 0, 1), false, all);
                wallet -= 2;
            } else {
                Word pose = (k >> 17) & 15, contact = probe_motor(m, slots, x, expose(x, pose, r.layers), k);
                out.associations += encounter(m, slots, x, pose, contact, r.layers, false, all);
                --wallet;
            }
            ++out.shown;
        }
        out.spent = opening - wallet;
    } else {
        // Demonstrations act on separate stock. Each new sample uses the same
        // reusable loop, physically returning it before leaving the fixture.
        for (Word pass = 0; pass < repeats; ++pass)
            for (Word i = 0; i < 108; ++i) {
                Word k = mix(ticket + 197 * i + pass * 811), phase = i < 32 ? 0u : i < 96 ? 1u : 2u;
                Word work = phase == 2 ? 3 : 1;
                if (wallet < work)
                    continue;
                Word q = i < 32   ? (i + ticket + pass) & 31u
                         : i < 96 ? (i - 32 + ticket + pass) & 63u
                                  : (i - 96 + ticket % 12 + pass) % 12;
                Word g = phase == 0 ? q >> 3 : phase == 1 ? q >> 4 : q % 4;
                Rig x{phase ? 3u : 1u, g | ((k >> 4) & 12), 0, phase ? 15u : 3u};
                if (phase == 2) {
                    Word old = q / 4, token = 1;
                    join(x, 0, 1, old);
                    insert(x, 1, 0, token, r.layers, wallet);
                    Word fitted = joint(x.joints, 0, 1);
                    out.associations += learn_join(m, slots, g, old, true, fitted, true, all);
                    join(x, 0, 1, old);
                    ++token;
                    out.associations += learn_join(m, slots, g, fitted, false, joint(x.joints, 0, 1), true, all);
                } else {
                    if (phase)
                        join(x, 0, 1, 1 + ((q >> 3) & 1));
                    Word pose = (q & 3) | (k & 12);
                    out.associations += encounter(m, slots, x, pose, (q >> 2) & 1, r.layers, true, all);
                }
                wallet -= work;
                ++out.shown;
            }
        r.tutor = wallet;
        r.demonstrated += opening - wallet;
    }
    ++r.round;
}
AX_FN void place(Rack &r, Word fixture, Word width) {
    auto s = yard(mix(r.world + fixture * 5581), width, r.layers);
    r.bench = {s.frame, s.sockets, s.loop, s.pose, s.target};
    r.width = width;
    r.fixture = fixture;
    r.redeemed = 0;
    r.has_goal = 0;
    r.block = 0;
    r.proposal = {};
    r.labels = permute(mix(fixture + 1367));
    r.reader = permute(mix(fixture + 3817)) % 256;
    if (r.reader == 0xe4)
        r.reader = 0x39;
    r.buttons = 0;
    for (Word i = 0; i < 4; ++i) {
        Word pad = permute(mix(r.world + 3457 * fixture + 2797 * i));
        for (Word h = 0; h < 2; ++h)
            r.buttons = replace(r.buttons, 2 * i + h, bits(pad, h));
    }
}
AX_FN Word identify_part(const Rack &r, Word label, Word order) {
    for (Word slot = 0; slot < 4; ++slot) {
        Word p = bits(order, slot);
        if ((r.bench.rig.parts & (1u << p)) && bits(r.labels, p) == label)
            return slot;
    }
    return 4;
}
AX_FN Word socket(Word a, Word b) {
    for (Word i = 0; i < 6; ++i)
        if ((edge_a(i) == a && edge_b(i) == b) || (edge_b(i) == a && edge_a(i) == b))
            return i;
    return no_edit;
}
AX_FN Word feature(const Tongue &t, Count sound) { return effort::feature(t.names, t.body, t.law, sound); }
AX_FN Count name(const Tongue &t, Word f) { return effort::name_feature(t.names, t.body, t.law, f); }
AX_FN Count verb(const Tongue &t, Word op) {
    for (Word i = 0; i < t.operators.count; ++i) {
        Count s = t.operators.forms[i].wave;
        if (effort::gesture(t.operators, t.law, s) == op)
            return s;
    }
    return 0;
}
AX_FN bool sound(effort::Speech &s, Count wave) {
    if (!wave || s.n >= 32)
        return false;
    s.sounds[s.n++] = wave;
    return true;
}
AX_FN effort::Speech describe(const Rack &r, const Tongue &t, Pattern goal, Word op) {
    effort::Speech s{};
    if (!sound(s, verb(t, op)))
        return {};
    for (Word i = 0; i < 4; ++i)
        if (goal.care & (3u << (2 * i))) {
            if (!sound(s, name(t, bits(r.labels, i))) || !sound(s, name(t, 4 + bits(goal.faces, i))))
                return {};
        }
    return s;
}
AX_FN effort::Speech pronounce(const Rack &r, const Tongue &t, const Design &p) {
    effort::Speech s{};
    if (!p.found || !sound(s, verb(t, effort::enact)))
        return {};
    if (p.edit != no_edit &&
        (!sound(s, name(t, bits(r.labels, edge_a(p.edit)))) || !sound(s, name(t, bits(r.labels, edge_b(p.edit))))))
        return {};
    for (Word j = 0; j < p.turns.len; ++j) {
        Word motor = step(p.turns, j);
        if (!sound(s, name(t, bits(r.labels, motor >> 1))) || !sound(s, name(t, 4 + bits(r.buttons, motor))))
            return {};
    }
    return s;
}
// Resolve all public sounds in the hearer's private acoustic tissue before
// paying for a stroke. The actuator addresses belong to the hearer's view.
AX_FN void enact(Rack &r, const Tongue &t, const effort::Speech &message, Word reserve, Word senses, Word order,
                 Uttered &out) {
    if ((senses & 3) != 3 || !(message.n & 1) ||
        effort::gesture(t.operators, t.law, message.sounds[0]) != effort::enact)
        return;
    Design p{};
    Program local{};
    Word cost = 0;
    for (Word j = 1; j < message.n; j += 2) {
        Word slot = identify_part(r, feature(t, message.sounds[j]), order), f = feature(t, message.sounds[j + 1]);
        if (slot == 4 || f >= 8)
            return;
        Word i = bits(order, slot);
        if (f < 4) {
            Word other = identify_part(r, f, order);
            if (other == 4 || j != 1)
                return;
            p.edit = socket(i, bits(order, other));
            if (p.edit >= no_edit || !(r.bench.sockets & (1u << p.edit)) || !r.bench.spool)
                return;
            cost += 2;
        } else {
            if (p.turns.len == max_steps)
                return;
            Word motor = i * 2;
            while (motor < i * 2 + 2 && bits(r.buttons, motor) != f - 4)
                ++motor;
            if (motor >= i * 2 + 2 || !(r.bench.rig.handles & ~r.block & (1u << motor)))
                return;
            append(p.turns, motor);
            append(local, slot * 2 + (motor & 1));
            ++cost;
        }
    }
    if (cost > reserve)
        return;
    out.understood = 1;
    out.executed = 1;
    out.steps = p.turns.len;
    out.local_motors = local.low;
    out.local_tail = local.high;
    out.world_motors = p.turns.low;
    out.world_tail = p.turns.high;
    out.spent = cost;
    if (p.edit != no_edit)
        out.inserted = insert(r.bench.rig, r.bench.sockets, p.edit, r.bench.spool, r.layers, reserve);
    for (Word j = 0; j < p.turns.len; ++j)
        r.bench.pose = press(r.bench.rig, r.bench.pose, step(p.turns, j), r.layers);
    Word actual = expose(r.bench.rig, r.bench.pose, r.layers);
    if ((senses & 4) && !r.redeemed && matches(actual, r.bench.lock) && r.stock >= 40 && reserve - cost <= 65535 - 40) {
        r.stock -= 40;
        r.swallowed += 40;
        r.redeemed = 1;
        out.food = 40;
    }
}
AX_FN void converse(Rack &r, Saved &native, const Tongue &t, const effort::Speech &s, Word reserve, Word senses,
                    Uttered &o) {
    if (!s.n || !(senses & 1))
        return;
    Word op = effort::gesture(t.operators, t.law, s.sounds[0]);
    auto rig = r.bench.rig;
    Word start = expose(rig, r.bench.pose, r.layers);
    if (op == effort::enact) {
        auto instruction = s.n == 1 ? r.proposal : s;
        if (instruction.n)
            enact(r, t, instruction, reserve, senses, 0xe4, o);
        return;
    }
    if (op == effort::material && s.n == 2) {
        Word part = identify_part(r, feature(t, s.sounds[1]), 0xe4);
        if (part < 4)
            o.understood = sound(o.words, name(t, 4 + bits(start, part)));
        return;
    }
    if (op == effort::compare && s.n > 1 && (s.n & 1)) {
        Pattern target{};
        for (Word j = 1; j < s.n; j += 2) {
            Word part = identify_part(r, feature(t, s.sounds[j]), 0xe4), f = feature(t, s.sounds[j + 1]);
            if (part >= 4 || f < 4 || f >= 8 || (target.care & (3u << (2 * part))))
                return;
            target.faces = replace(target.faces, part, f - 4);
            target.care |= 3u << (2 * part);
        }
        r.request = target;
        r.has_goal = 1;
        r.block = 0;
    } else if (op == effort::remove && r.has_goal && s.n == 2) {
        Word p = identify_part(r, feature(t, s.sounds[1]), 0xe4);
        if (p >= 4)
            return;
        r.block |= 3u << (2 * p);
    } else if (!(r.has_goal && s.n == 1 && (op == effort::compare || op == effort::explain)))
        return;
    rig.handles &= ~r.block;
    auto chart = compile(native.mind, {native.tissue, 1, native.arena});
    auto p = design(chart, rig, start, r.request, r.bench.spool ? r.bench.sockets : 0, native.mind.genes);
    o.understood = 1;
    o.planned = p.found;
    o.refused = !p.found;
    if (op == effort::explain) {
        if (!p.found)
            return;
        for (Word i = 0; i < 4; ++i)
            if (rig.handles & (3u << (2 * i))) {
                auto blocked = rig;
                blocked.handles &= ~(3u << (2 * i));
                auto alt =
                    design(chart, blocked, start, r.request, r.bench.spool ? r.bench.sockets : 0, native.mind.genes);
                if (!alt.found) {
                    sound(o.words, name(t, bits(r.labels, i)));
                    break;
                }
            }
        return;
    }
    r.proposal = pronounce(r, t, p);
    o.words = r.proposal;
}
} // namespace axion::life::linkage
KERNEL void linkage_conversation(linkage::Rack *room, linkage::Saved *native, const linkage::Tongue *speakers,
                                 Word cmd, Word a, Word b, effort::Speech input, Word reserve, Word senses,
                                 linkage::Uttered *out) {
    using namespace linkage;
    auto &r = *room;
    Uttered o{};
    if (cmd == enter_rack) {
        r = {};
        r.world = a;
        for (Word i = 0; i < 4; ++i)
            r.layers[i] = matter(a, i);
        place(r, 0, b);
    }
    o.start = expose(r.bench.rig, r.bench.pose, r.layers);
    if (cmd == exchange_rig)
        place(r, a, b);
    else if (cmd == own_practice || cmd == show_practice)
        exhibit(r, *native, a, cmd == own_practice, reserve, senses, o);
    else if (cmd == read_order && (senses & 1))
        o.words = describe(r, speakers[0], r.bench.lock, effort::compare);
    else if (cmd == request_words)
        converse(r, *native, speakers[0], input, reserve, senses, o);
    else if (cmd == pass_words) {
        auto message = r.proposal;
        if (a && message.n > 2) {
            if (a == 1)
                message.sounds[1] = 0;
            else {
                auto &q = message.sounds[2];
                Word f = feature(speakers[0], q);
                q = name(speakers[0], (f & 4) | ((f + 1) & 3));
            }
        }
        if (message.n)
            enact(r, speakers[1], message, reserve, senses, r.reader, o);
    } else if (cmd == drink) {
        o.food = a < r.stock ? a : r.stock;
        if (o.food > 65535 - reserve)
            o.food = 65535 - reserve;
        r.stock -= o.food;
        r.swallowed += o.food;
    }
    o.after = expose(r.bench.rig, r.bench.pose, r.layers);
    o.goal = r.request.faces;
    o.care = r.request.care;
    o.aligned = r.has_goal && matches(o.after, r.request);
    *out = o;
}
KERNEL void linkage_meetings(const linkage::Saved *source, const linkage::Tongue *speakers,
                             const linkage::Request *questions, Word n, Word purse, Word senses,
                             linkage::Conversation *traces) {
    using namespace linkage;
    Word i = tid();
    if (i >= n)
        return;
    auto q = questions[i];
    Saved student = *source;
    Rack r{};
    r.world = q.world;
    for (Word j = 0; j < 4; ++j)
        r.layers[j] = matter(q.world, j);
    place(r, q.fixture, q.width);
    Uttered lesson{};
    exhibit(r, student, q.tuition, false, 0, senses & 7, lesson);
    auto &answer = traces[i];
    answer = {};
    answer.learnt = lesson.associations;
    answer.labels = r.labels;
    answer.buttons = r.buttons;
    answer.order = r.reader;
    answer.joints = r.bench.rig.joints;
    answer.glyphs = r.bench.rig.glyphs;
    answer.grip = r.bench.rig.handles;
    answer.allowed = r.bench.sockets;
    converse(r, student, speakers[0], q.first, 0, senses & 7, answer.first);
    converse(r, student, speakers[0], q.revision, 0, senses & 7, answer.amended);
    converse(r, student, speakers[0], q.why, 0, senses & 7, answer.reason);
    answer.barred = r.block;
    auto &p = answer.peer;
    p.start = expose(r.bench.rig, r.bench.pose, r.layers);
    p.goal = r.request.faces;
    p.care = r.request.care;
    auto wire = r.proposal;
    if (q.wire && wire.n > 2) {
        if (q.wire == 1)
            wire.sounds[1] = 0;
        else if (q.wire == 2) {
            Word f = feature(speakers[0], wire.sounds[2]);
            wire.sounds[2] = name(speakers[0], (f & 4) | ((f + 1) & 3));
        } else if (q.wire == 3)
            for (Word j = 1; j < wire.n; j += 2) {
                Word f = feature(speakers[0], wire.sounds[j]);
                wire.sounds[j] = name(speakers[0], (f + 1) & 3);
            }
    }
    if (wire.n)
        enact(r, speakers[1], wire, purse, senses >> 3, r.reader, p);
    p.words = wire;
    p.after = expose(r.bench.rig, r.bench.pose, r.layers);
    p.aligned = r.has_goal && matches(p.after, r.request);
}
