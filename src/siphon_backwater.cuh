#include "axion/backwater.hpp"
namespace axion::life::rattle {
using linkage::Tongue;
AX_FN effort::Speech request_name(const Tongue &v, Word tag) {
    effort::Speech q{};
    if (!linkage::sound(q, linkage::verb(v, effort::explain)) || !linkage::sound(q, linkage::name(v, tag)))
        return {};
    return q;
}
AX_FN effort::Speech describe_wave(const Tongue &v, Word wave) {
    effort::Speech q{};
    if (wave == silence || !linkage::sound(q, linkage::name(v, wave >> 1)) ||
        !linkage::sound(q, linkage::name(v, 4 + (wave & 1))))
        return {};
    return q;
}
AX_FN effort::Speech proposal(const Tongue &v, Word move) {
    effort::Speech q{};
    if (!linkage::sound(q, linkage::verb(v, effort::fold)) || !linkage::sound(q, linkage::name(v, 4 + move)))
        return {};
    return q;
}
AX_FN Word read_wave(const Tongue &v, const effort::Speech &q, Word tag) {
    if (q.n != 2 || linkage::feature(v, q.sounds[0]) != tag)
        return silence;
    Word sign = linkage::feature(v, q.sounds[1]);
    return sign == 4 || sign == 5 ? 2 * tag + sign - 4 : silence;
}
AX_FN void empty_pan(Table &b) {
    b.source += b.stock[0] + b.stock[1];
    b.stock[0] = b.stock[1] = 0;
}
AX_FN Word teacher(const Table &b) { return ((mix(b.seed + 3467) >> 5) ^ b.diverted) & 1; }
AX_FN void present(Table &b) {
    empty_pan(b);
    Word key = mix(b.seed + b.round * 1879u), tags = mix(b.seed + 7771), order = (key >> 11) & 1, fed = teacher(b);
    Word rim = tags & 3, next = (rim + 1 + ((tags >> 2) % 3)) & 3;
    Word samples[]{(key >> 3) & 1, (key >> 7) & 1};
    b.tags[order] = rim;
    b.tags[order ^ 1] = next;
    for (Word i = 0; i < 2; ++i)
        b.waves[i ^ order] = 2 * b.tags[i ^ order] + (samples[i] ^ ((tags >> (8 + i)) & 1));
    b.opening = order ^ fed;
    Word loaded = b.opening ^ Word(!samples[fed]), deposit = b.source < 6 ? b.source : 6;
    b.stock[loaded] = deposit;
    b.source -= deposit;
    b.drive = mix(key + 6029);
    b.stage = 1;
    b.answer = silence;
    b.spoken = 0;
}
AX_FN void ask(Table &b, Mind &m, const Tongue &v, Word reserve, Exchange &out) {
    if (!b.stage)
        present(b);
    if (b.stage != 1 || reserve <= out.paid + 1)
        return;
    b.asked = invite(m, b.tags[0], b.tags[1], b.drive, true, b.crossed);
    out.request = request_name(v, b.tags[b.asked]);
    b.spoken = out.request.n != 0;
    b.stage = 2;
    ++out.paid;
    ++out.inquiries;
    out.upstream += b.asked == b.opening;
}
AX_FN void answer(Table &b, const Mind &m, const Tongue &v, const effort::Speech &q, Exchange &out) {
    b.answer = read_wave(v, q, b.tags[b.asked]);
    Word a = b.asked ? silence : b.answer, d = b.asked ? b.answer : silence;
    b.decision = pick(m, a, d, b.drive);
    b.stage = 3;
    out.reply = proposal(v, b.decision);
    out.changed += b.decision != pick(m, silence, silence, b.drive);
    out.reports += b.answer != silence;
}
AX_FN void pass(Table &b, const Mind &m, const Tongue (&v)[2], Word channel, Exchange &out) {
    if (b.stage != 2)
        return;
    out.request = request_name(v[0], b.tags[b.asked]);
    effort::Speech reply{};
    if (channel != 1 && b.spoken && out.request.n == 2 &&
        effort::gesture(v[1].operators, v[1].law, out.request.sounds[0]) == effort::explain) {
        Word tag = linkage::feature(v[1], out.request.sounds[1]);
        for (Word i = 0; i < 2; ++i)
            if (b.tags[i] == tag) {
                Word wave = b.waves[i] ^ Word(channel == 2);
                reply = describe_wave(v[1], wave);
            }
    }
    out.report = reply;
    answer(b, m, v[0], reply, out);
}
AX_FN void draw(Table &b, Mind &m, Word reserve, Word plastic, Exchange &out) {
    if (b.stage != 3 || reserve + out.food <= out.paid + 1)
        return;
    ++out.paid;
    ++out.pulls;
    Word room = 65535u - (reserve + out.food - out.paid), stock = b.stock[b.decision];
    Word meal = stock < room ? stock : room;
    b.stock[b.decision] -= meal;
    b.eaten += meal;
    out.food += meal;
    // Only the selected pan is tasted. A heard claim about the unchosen pan
    // is never rewarded or punished by an inaccessible counterfactual.
    Word tasted = b.decision == b.asked ? b.answer : silence;
    taste(m, tasted, b.decision != b.asked ? b.answer : silence, meal, b.tick, b.crossed);
    b.tick += 12;
    auto diff = digest(m, b.tick, plastic);
    out.writes += (diff >> 1) & 1;
    out.switches += diff >> 2;
    ++b.round;
    b.stage = 0;
    empty_pan(b);
}
AX_FN void dialog(Table &b, Mind &m, const Tongue (&v)[2], Word cmd, Word n, Word channel, const effort::Speech &line,
                  Word reserve, Word plastic, Exchange &out) {
    if (cmd == assemble) {
        b = {};
        b.seed = n;
        b.crossed = channel;
        return;
    }
    if (cmd == divert) {
        empty_pan(b);
        b.stage = 0;
        b.diverted ^= 1;
        return;
    }
    if (cmd == view)
        return;
    // A portable trace arrives with time zero measured at export.
    if (waiting(m)) {
        b.tick += 12;
        auto diff = digest(m, b.tick, plastic);
        out.writes += (diff >> 1) & 1;
        out.switches += diff >> 2;
    }
    if (cmd == practise) {
        for (Word i = 0; i < n && b.source + b.stock[0] + b.stock[1] && reserve + out.food > out.paid + 2; ++i) {
            if (b.stage <= 1)
                ask(b, m, v[0], reserve + out.food, out);
            pass(b, m, v, channel, out);
            draw(b, m, reserve, plastic, out);
        }
    } else if (cmd == relay)
        pass(b, m, v, channel, out);
    else if (cmd == pull)
        draw(b, m, reserve, plastic, out);
    else if (cmd == listen_word && line.n) {
        Word op = effort::gesture(v[0].operators, v[0].law, line.sounds[0]);
        if (line.n == 1 && op == effort::compare) {
            if (b.stage <= 1) {
                ask(b, m, v[0], reserve, out);
                out.reply = out.request;
            } else if (b.stage == 2)
                out.reply = request_name(v[0], b.tags[b.asked]);
            else
                out.reply = proposal(v[0], b.decision);
            out.understood = 1;
        } else if (line.n == 1 && op == effort::explain && b.stage == 3) {
            if (b.answer != silence && b.decision != pick(m, silence, silence, b.drive)) {
                out.reply = describe_wave(v[0], b.answer);
                out.grounded = out.reply.n != 0;
            }
            out.understood = 1;
        } else if (line.n == 2 && op == effort::remove && b.stage == 3) {
            Word tag = linkage::feature(v[0], line.sounds[1]);
            if (tag < 4) {
                auto wave = tag == b.tags[b.asked] ? silence : b.answer;
                out.reply = proposal(v[0], pick(m, b.asked ? silence : wave, b.asked ? wave : silence, b.drive));
                out.understood = 1;
            }
        } else if (b.stage == 2 && read_wave(v[0], line, b.tags[b.asked]) != silence) {
            answer(b, m, v[0], line, out);
            out.understood = 1;
        }
    }
    out.ended = b.stage;
}
KERNEL void bank_interview(Table *bay, Mind *m, const Tongue (*v)[2], Word cmd, Word n, Word channel,
                           effort::Speech line, Word reserve, Word plastic, Exchange *record) {
    Exchange out{};
    dialog(*bay, *m, *v, cmd, n, channel, line, reserve, plastic, out);
    *record = out;
}
} // namespace axion::life::rattle

namespace axion::life::rattle {
KERNEL void bank_conversations(const Mind *original, const Tongue (*voices)[2], Word cases, Word exposure, Word seed,
                               Word arm, Word across, Spoken *rows) {
    Word t, bz, n;
    asm("mov.u32 %0, %%tid.x;" : "=r"(t));
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(bz));
    asm("mov.u32 %0, %%ntid.x;" : "=r"(n));
    Word i = t + bz * n;
    if (i >= cases)
        return;
    const auto &v = *voices;
    Word world = mix(seed + i * 4159);
    Spoken rec{};
    effort::Speech choose{}, why{};
    linkage::sound(choose, linkage::verb(v[0], effort::compare));
    linkage::sound(why, linkage::verb(v[0], effort::explain));
    for (Word twin = 0; twin < 2; ++twin) {
        Table bay{};
        bay.seed = world;
        bay.diverted = twin;
        bay.crossed = across;
        Word wallet = 256 + 4 * exposure;
        Mind m = *original;
        if (waiting(m))
            digest(m, 12, true);
        for (Word phase = 0; phase < 2; ++phase) {
            Exchange live{};
            dialog(bay, m, v, practise, exposure,
                   (arm == 1 || (arm == 3 && phase == 0)) ? 1
                   : arm == 2                             ? 2
                                                          : 0,
                   {}, wallet, arm != 4 || !phase, live);
            wallet = wallet + live.food - live.paid;
            if (!phase) {
                empty_pan(bay);
                bay.diverted ^= 1;
            } else
                rec.experienced += live.pulls;
        }
        // Freeze the private state for independent single-inquiry probes. Each
        // twin sees the same two mounts and the same possible response at each.
        Word mount = mix(world + 7771), tag = mount & 3, another = (tag + 1 + ((mount >> 2) % 3)) & 3;
        for (Word j = 0; j < 8; ++j) {
            Mind probe = m;
            Table trial{};
            trial.seed = world;
            trial.crossed = across;
            trial.diverted = bay.diverted;
            Word slot = j >> 2, up = teacher(trial), sample[]{j & 1, (j >> 1) & 1};
            trial.tags[slot] = tag;
            trial.tags[slot ^ 1] = another;
            for (Word p = 0; p < 2; ++p)
                trial.waves[slot ^ p] = 2 * trial.tags[slot ^ p] + (sample[p] ^ ((mount >> (8 + p)) & 1));
            Word rich = slot ^ up ^ Word(!sample[up]);
            trial.opening = slot ^ up;
            trial.stock[rich] = 6;
            trial.source -= 6;
            trial.stage = 1;
            trial.drive = mix(world + j * 5659u + 811);
            rec.targets[twin] |= rich << j;
            Exchange request{}, report{}, why_reply{}, alternative{}, work{};
            dialog(trial, probe, v, listen_word, 0, 0, choose, wallet, 1, request);
            rec.requests[twin] |= trial.asked << j;
            rec.asked += request.inquiries;
            rec.voiced += request.request.n == 2;
            rec.understood += request.understood;
            dialog(trial, probe, v, relay, 0,
                   arm == 1                 ? 1
                   : (arm == 2 || arm == 5) ? 2
                                            : 0,
                   {}, wallet - request.paid, 1, report);
            rec.reports += report.reports;
            Word named =
                report.reply.n == 2 && effort::gesture(v[1].operators, v[1].law, report.reply.sounds[0]) == effort::fold
                    ? linkage::feature(v[1], report.reply.sounds[1])
                    : 8;
            bool peer_read = named >= 4 && named <= 5;
            rec.decoded += peer_read && named == 4 + trial.decision;
            rec.choices[twin] |= trial.decision << j;
            dialog(trial, probe, v, listen_word, 0, 0, why, wallet - request.paid, 1, why_reply);
            effort::Speech without{};
            linkage::sound(without, linkage::verb(v[0], effort::remove));
            linkage::sound(without, linkage::name(v[0], trial.tags[trial.asked]));
            dialog(trial, probe, v, listen_word, 0, 0, without, wallet - request.paid, 1, alternative);
            Word muted = alternative.reply.n == 2 ? linkage::feature(v[1], alternative.reply.sounds[1]) : 8;
            Word cited = read_wave(v[1], why_reply.reply, trial.tags[trial.asked]);
            if (why_reply.reply.n) {
                ++rec.cited;
                rec.causal +=
                    cited == trial.waves[trial.asked] && peer_read && muted >= 4 && muted <= 5 && muted != named;
            }
            rec.other_pan += peer_read && muted >= 4 && muted <= 5 && muted != named;
            dialog(trial, probe, v, pull, 0, 0, {}, wallet - request.paid, 1, work);
            rec.rounds += work.pulls;
            rec.earned += work.food;
            rec.correct += work.food && peer_read && named == 4 + rich;
            rec.verified += trial.source + trial.eaten + trial.stock[0] + trial.stock[1] == 8192;
            rec.stock += trial.source;
            rec.paid += request.paid + work.paid;
        }
    }
    rows[i] = rec;
}
} // namespace axion::life::rattle
