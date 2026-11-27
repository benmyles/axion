#include "runnel_physics.cuh"
#include "runnel_reference.cuh"
namespace axion::life::runnel {
using effort::Speech;
using linkage::Tongue;
AX_FN Speech phrase(const Tongue &v, Word op, Word cue, Word value) {
    Speech s{};
    if (!linkage::sound(s, linkage::verb(v, op)) || !linkage::sound(s, linkage::name(v, cue & 3)) ||
        !linkage::sound(s, linkage::name(v, cue >> 2)) || !linkage::sound(s, linkage::name(v, 4 + value)))
        return {};
    return s;
}
AX_FN bool decode(const Tongue &v, const Speech &q, Word op, Word &cue, Word &feature) {
    if (q.n < 4 || effort::gesture(v.operators, v.law, q.sounds[0]) != op)
        return false;
    Word a = linkage::feature(v, q.sounds[1]), b = linkage::feature(v, q.sounds[2]),
         z = linkage::feature(v, q.sounds[3]);
    if (a >= 4 || b >= 4 || z < 4 || z >= 8)
        return false;
    cue = texture(a, b);
    feature = z - 4;
    return true;
}
AX_FN Choice decide(const Mind &student, Word cue, Word direction) {
    return infer(student, cue, direction, (student.genes.hand >> 8) & 3, false, true).action;
}
AX_FN bool sure(Choice q, Word dir) { return q.known && q.expected == (1u << dir); }
AX_FN Speech design(Flume &f, const Mind &student, const Tongue &v, Word cue, Word direction, Dispatch &out,
                    const Image *image = nullptr) {
    if (image) {
        Word action = packed(*image, cue, direction);
        if (action & 4) {
            f.cue = cue;
            f.direction = direction;
            f.question = cue;
            f.stage = 3;
            f.pending = phrase(v, effort::fold, cue, pose(action & 3, student.genes.hand & 7));
            ++out.cached;
            return f.pending;
        }
    }
    auto proof = infer(student, cue, direction, (student.genes.hand >> 8) & 3, false, true);
    auto a = proof.action;
    out.example = proof.via;
    out.transform = proof.transform;
    f.cue = cue;
    f.direction = direction;
    bool known = sure(a, direction);
    f.stage = known ? 3 : 1;
    Word topic = known ? cue : ask_about(student, cue, direction, f.unavailable, f.indirect);
    f.question = topic;
    if (topic >= 16) {
        f.stage = 0;
        f.pending = {};
        return {};
    }
    f.pending = phrase(v, known ? effort::fold : effort::explain, topic,
                       known ? pose(a.motor, student.genes.hand & 7) : direction);
    if (!known) {
        ++f.referrals;
        ++out.asked;
    }
    return f.pending;
}
// A consultant sees only the waves: no apparatus, hidden compliance, partner's memory or desired answer.
AX_FN Speech consult(const Mind &artisan, const Tongue &ear, const Speech &request, Word intervention, Dispatch &r) {
    Word cue = 0, d = 0;
    if (!decode(ear, request, effort::explain, cue, d) || request.n != 4 || d > 1)
        return {};
    auto a = decide(artisan, cue, d);
    if (!sure(a, d))
        return {};
    Word gesture = pose(a.motor, artisan.genes.hand & 7);
    if (intervention == 2)
        gesture = (gesture + 2) & 3;
    auto response = phrase(ear, effort::fold, cue, gesture);
    if (!response.n || !linkage::sound(response, linkage::verb(ear, effort::material)) ||
        !linkage::sound(response, linkage::name(ear, 4 + a.expected)))
        return {};
    ++r.reported;
    r.report_pose = gesture;
    r.report_effect = a.expected;
    return response;
}
AX_FN bool accept(Mind &student, const Tongue &v, const Speech &message, Word wanted, Word &gesture, Word &effect) {
    Word cue;
    if (message.n != 6 || !decode(v, message, effort::fold, cue, gesture) || cue != wanted ||
        effort::gesture(v.operators, v.law, message.sounds[4]) != effort::material)
        return false;
    Word z = linkage::feature(v, message.sounds[5]);
    if (z < 4 || z >= 8)
        return false;
    effect = z - 4;
    observe(student, cue, gesture, student.genes.hand & 7, effect, true, live);
    return true;
}
AX_FN void flume_pass(Flume &f, Mind &student, const Mind &teacher, const Tongue *voices, Word intervention, Word purse,
                      Word senses, Dispatch &r) {
    r.input = f.pending;
    if (f.stage == 1) {
        f.pending = intervention == 1 ? Speech{} : consult(teacher, voices[1], r.input, intervention, r);
        if (f.pending.n) {
            f.stage = 2;
            r.reply = f.pending;
        } else {
            f.unavailable |= 1u << f.question;
            f.stage = 0;
            r.reply = design(f, student, voices[0], f.cue, f.direction, r);
            ++f.retry;
        }
        return;
    }
    if (f.stage == 2) {
        Word public_motor = 0, result = 0;
        if (intervention == 1 || !accept(student, voices[0], r.input, f.question, public_motor, result)) {
            f.pending = {};
            f.stage = 0;
            return;
        }
        ++f.testimonies;
        ++r.learned;
        r.report_pose = public_motor;
        r.report_effect = result;
        auto a = decide(student, f.cue, f.direction);
        if (!sure(a, f.direction)) {
            f.pending = {};
            f.stage = 0;
            return;
        }
        r.reply = design(f, student, voices[0], f.cue, f.direction, r);
        return;
    }
    if (f.stage != 3)
        return;
    Word cue, motor;
    f.stage = 0;
    f.pending = {};
    if (intervention == 3 || !purse || (senses & 3) != 3 || f.cursor >= f.span ||
        !decode(voices[1], r.input, effort::fold, cue, motor) || r.input.n != 4)
        return;
    Word pin = landmark(f.layout, f.cursor);
    if (cue != local_cue(pin))
        return;
    Word native_motor = native(motor, teacher.genes.hand & 7), physical = pose(native_motor, teacher.genes.hand & 7);
    f.fold[f.cursor] = Byte(1 + physical);
    ++f.installed;
    ++r.installed;
    ++r.paid;
    ++f.cursor;
}
AX_FN void layout(Flume &b, Word world, Word seed, Word span, Word remaining, Word spent, Word chiral) {
    b = {};
    b.world = world;
    b.stone = material_id(mix(world + 9421), chiral);
    b.layout = seed;
    b.span = span;
    b.tutor = remaining;
    b.tuition = spent;
    fill(b.cells, span);
}
AX_FN Speech request(const Flume &b, const Tongue &v) {
    if (b.cursor >= b.span)
        return {};
    Word pin = landmark(b.layout, b.cursor);
    return phrase(v, effort::compare, local_cue(pin), upstream(pin));
}
AX_FN void answer(Flume &b, Mind &pupil, const Tongue &v, const Speech &q, Dispatch &r, const Image *image = nullptr) {
    r.input = q;
    if (!q.n)
        return;
    Word op = effort::gesture(v.operators, v.law, q.sounds[0]), cue = 0, value = 0;
    if (op == effort::compare && q.n == 4 && decode(v, q, op, cue, value) && value < 2) {
        r.understood = 1;
        b.unavailable = 0;
        r.reply = design(b, pupil, v, cue, value, r, image);
        return;
    }
    if (op == effort::material && q.n == 4 && decode(v, q, op, cue, value)) {
        Word i = native(value, pupil.genes.hand & 7), row_id = row(pupil.genes, cue), bit = 1u << i;
        if (!(nibble(pupil.cells.touched, row_id) & bit))
            return;
        Word forecast = Word(!!(nibble(pupil.cells.forward, row_id) & bit)) +
                        2 * Word(!!(nibble(pupil.cells.backward, row_id) & bit));
        r.understood = 1;
        r.reply = phrase(v, effort::material, cue, forecast);
        return;
    }
    if (op == effort::explain && q.n == 1) {
        r.understood = 1;
        auto p = infer(pupil, b.cue, b.direction, (pupil.genes.hand >> 8) & 3, false, true);
        r.reply = phrase(v, effort::explain, b.cue, b.direction);
        if (p.derived) {
            r.example = p.via;
            r.transform = p.transform;
            linkage::sound(r.reply, linkage::verb(v, effort::material));
            linkage::sound(r.reply, linkage::name(v, p.via & 3));
            linkage::sound(r.reply, linkage::name(v, p.via >> 2));
        }
        return;
    }
    if (op == effort::fold && q.n == 6) {
        Word gesture, effect;
        if (accept(pupil, v, q, b.question, gesture, effect)) {
            r.understood = 1;
            ++r.learned;
            ++b.testimonies;
            r.reply = design(b, pupil, v, b.cue, b.direction, r);
        }
    }
}
AX_FN void instruction(Flume &b, Mind &pupil, Word domain, Word repetitions, Word senses, Dispatch &r) {
    if (!(senses & 1))
        return;
    Word count = domain == 2 ? 16 : 8;
    for (Word rep = 0; rep < repetitions; ++rep)
        for (Word j = 0; j < count; ++j)
            for (Word fold = 0; fold < 4; ++fold) {
                if (b.tutor < 2)
                    return;
                Word cue = domain == 2 ? j : 2 * j + domain;
                Word result = flex(b.stone, cue, fold);
                b.tutor -= 2;
                b.tuition += 2;
                ++r.attempts;
                r.learned += observe(pupil, cue, fold, pupil.genes.hand & 7, result, true, live);
            }
}
} // namespace axion::life::runnel
KERNEL void flume_interview(runnel::Flume *table, runnel::Mind *m, const linkage::Tongue *voices, Word command, Word a,
                            Word b, Word n, effort::Speech heard, Word purse, Word senses, runnel::Dispatch *record) {
    if (tid())
        return;
    using namespace runnel;
    auto &f = *table;
    Dispatch out{};
    auto &v = voices[0];
    if (command == open_flume)
        layout(f, a, mix(a + 883), n, 1024, 0, b);
    else if (command == layout_flume)
        layout(f, f.world, a, n, f.tutor, f.tuition, f.stone >> 31);
    else if (command == school_flume)
        instruction(f, m[a], b, n, (senses >> (3 * a)) & 7, out);
    else if (command == sample_flume) {
        if ((senses >> (3 * a)) & 1)
            for (Word gesture = 0; gesture < 4 && f.tutor >= 2; ++gesture) {
                f.tutor -= 2;
                f.tuition += 2;
                ++out.attempts;
                out.learned += observe(m[a], b, gesture, m[a].genes.hand & 7, flex(f.stone, b, gesture), true, live);
            }
    } else if (command == order_flume)
        out.reply = request(f, v);
    else if (command == speak_flume)
        answer(f, m[0], v, heard, out);
    else if (command == pass_flume)
        flume_pass(f, m[0], m[1], voices, a, purse, (senses >> 3) & 7, out);
    else if (command == clear_flume) {
        m[a].cells = {};
        m[a].practice = 0;
        f.stage = 0;
        f.pending = {};
    } else if (command == build_flume) {
        Image image{};
        bool valid = false;
        while (f.cursor < f.span) {
            if (!valid && f.span - f.cursor >= 32) {
                image = synthesize(m[0], (m[0].genes.hand >> 8) & 3, true);
                valid = true;
            }
            Word opening = f.cursor, prior = f.testimonies;
            auto q = request(f, v);
            answer(f, m[0], v, q, out, valid ? &image : nullptr);
            for (Word t = 0; t < 18 && f.stage; t++)
                flume_pass(f, m[0], m[1], voices, a, purse - out.paid, (senses >> 3) & 7, out);
            if (prior != f.testimonies)
                valid = false;
            if (opening == f.cursor) {
                ++f.abstained;
                ++f.cursor;
                f.stage = 0;
                f.pending = {};
            }
        }
    } else if (command == start_pump) {
        Byte valves[max_span];
        for (Word j = 0; j < f.span; ++j) {
            Word pin = landmark(f.layout, j), fold = f.fold[j];
            Word mask = fold ? contact(f.stone, local_cue(pin), fold - 1) : 3;
            if (upstream(pin))
                mask = (mask >> 1) | ((mask & 1) << 1);
            valves[j] = Byte(mask);
        }
        Gallery q{};
        advance(f.cells, valves, f.span, f.clock, n, a != 0, q);
        f.clock += n;
        f.source_food += q.from;
        f.outlet_in += q.into;
        f.outlet_back += q.back;
        auto &sink = f.cells[f.span];
        Word mouth = (senses >> 3) & 4 ? 8 : 0;
        out.meal = sink.carbon < mouth ? sink.carbon : mouth;
        if (out.meal > 65535u - purse)
            out.meal = 65535u - purse;
        sink.carbon -= out.meal;
        f.swallowed += out.meal;
    }
    out.water = f.cells[f.span].water;
    out.food = f.cells[f.span].carbon;
    for (Word j = 0; j <= f.span; ++j) {
        out.stock_water += f.cells[j].water;
        out.stock_carbon += f.cells[j].carbon;
    }
    *record = out;
}
