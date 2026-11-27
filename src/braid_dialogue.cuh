#include "axion/braid_talk.hpp"
namespace axion::life::braid {
AX_FN Word recognize(Saved &p, Count sound, bool recruit) {
    if (!sound)
        return none;
    Word cap = capacity(p.mind.genes), start = mix(Word(sound) ^ Word(sound >> 32) ^ p.mind.genes.muscle) % cap,
         free = none;
    for (Word i = 0; i < cap; ++i) {
        Word k = (start + i) % cap;
        if (p.words.wave[k] == sound)
            return k;
        if (!p.words.wave[k] && free == none)
            free = k;
    }
    if (recruit && free != none) {
        p.words.wave[free] = sound;
        return free;
    }
    return none;
}
AX_FN Phrase hear(Saved &p, const Speech &q, bool recruit) {
    Phrase a{};
    a.n = q.n;
    for (Word j = 0; j < q.n; ++j)
        a.cell[j] = recognize(p, q.wave[j], recruit);
    return a;
}
AX_FN bool recognized(const Phrase &p) {
    for (Word i = 0; i < p.n; ++i)
        if (p.cell[i] >= voices)
            return false;
    return p.n != 0;
}
AX_FN Speech say(const Saved &p, Plan plan) {
    Speech q{};
    if (!plan.found)
        return q;
    for (Word i = 0; i < plan.size; ++i) {
        auto sound = p.words.wave[(plan.stream >> (3 * (reversed(p.mind) ? plan.size - 1 - i : i))) & 7];
        if (!sound)
            return {};
        q.wave[q.n++] = sound;
    }
    return q;
}
AX_FN Motors lexicon(const Saved &p) {
    auto m = release(p.mind, true);
    for (Word i = 0; i < voices; ++i)
        if (!p.words.wave[i])
            m.present &= ~(1u << i);
    return m;
}
AX_FN bool enact(Saved &p, Table &b, Word who, Word relation, Word purse, Exchange &out, bool work = true) {
    out.accepted = 1;
    out.before = b.paint[who];
    auto image = predict(relation, out.before);
    out.goal = image.colors;
    out.known = image.known;
    if (image.known != 15)
        return false;
    auto plan = planning(p.mind, out.before, image.colors, 15, depth(p.mind.genes));
    out.work = plan;
    if (!plan.found || plan.size > purse - out.paid[who])
        return false;
    Word next = realize(out.before, plan, p.mind.genes, b.rig[who], b.strands[who]);
    if (work) {
        b.paint[who] = next;
        out.paid[who] += plan.size;
        b.fees += plan.size;
    }
    out.after = next;
    return true;
}
AX_FN bool execute(Saved &p, Table &b, Word who, const Speech &line, Word purse, Exchange &out) {
    auto input = hear(p, line, false);
    return recognized(input) && enact(p, b, who, pathway(p.mind, input), purse, out);
}
AX_FN void take_meal(Table &b, Word who, Word purse, Exchange &out) {
    if (!(b.armed & (1u << who)))
        return;
    Word grant = b.source < 8 ? b.source : 8, meal = harvest(b.paint[who], b.sockets[who], grant).swallowed;
    Word room = 65535u - purse + out.paid[who];
    if (meal > room)
        meal = room;
    out.food[who] += meal;
    b.source -= meal;
    b.food += meal;
}
AX_FN Word useful_sample(Word allowed, Word old, Word drive, bool selective) {
    Word best = 0, win = none;
    for (Word j = 0; j < 4; ++j) {
        Word mark = (j + drive) & 3;
        if (old & (1u << mark))
            continue;
        Word bit = 1u << mark, value = 0;
        for (Word dest = 0; dest < 4; ++dest) {
            Word permits = (allowed >> (4 * dest)) & 15;
            if (permits & bit)
                value += __builtin_popcount(permits & ~bit);
        }
        if (win == none || (selective && value > best)) {
            win = mark;
            best = value;
        }
    }
    return win;
}
AX_FN Word retinal(const Mind &m, Word seen) { return (m.genes.arbor & 4) ? compete(all ^ seen) : all ^ seen; }
AX_FN void witness(const Mind &m, Word &blocked, Word pre, Word post) {
    if (!(m.genes.arbor & (1u << 14)))
        return;
    Word active = compatible(pre, post, m.genes.arbor & 3);
    blocked = rule(m.genes.plastic >> 8, blocked, all ^ active);
}
AX_FN void translate(Saved *persons, Table &b, const Word *purses, Word budget, Word wire, bool lender, bool third,
                     Exchange &out) {
    if (!b.ready)
        return;
    Word operator_id = third ? 2 : 1;
    auto &speaker = persons[0], &listener = persons[1], &worker = persons[operator_id];
    auto local_vocabulary = lexicon(listener);
    Word screened = 0, marks = 0, first = b.start, final = b.end;
    auto attend = [&](Word a, Word z) {
        if (wire == unseeing)
            z = a;
        witness(listener.mind, screened, a, z);
        ++out.samples;
        if (lender && b.message.n) {
            auto foreign_words = hear(listener, b.message, true);
            if (recognized(foreign_words))
                out.borrowed += listen(listener.mind, foreign_words, a, z);
        }
    };
    attend(first, final);
    for (Word draw = 0; draw < budget && draw < 4; ++draw) {
        Word ports = retinal(listener.mind, screened);
        if (predict(ports, order).known == 15)
            break;
        if (wire == no_repeat || out.paid[1] >= purses[1])
            break;
        Word mark = useful_sample(ports, marks, mix(b.seed + b.index * 3391 + draw * 1637),
                                  listener.mind.genes.arbor & (1u << 18));
        if (mark == none)
            break;
        marks |= 1u << mark;
        Word panel = wire == repeat_probe ? first : 1u << (2 * mark);
        Word hold = b.paint[0];
        b.paint[0] = panel;
        Exchange copy{};
        bool moved = enact(speaker, b, 0, b.intent, purses[0] - out.paid[0], copy);
        b.paint[0] = hold;
        if (!moved)
            break;
        ++b.fees;
        ++out.paid[1];
        out.paid[0] += copy.paid[0];
        out.probes[draw] = panel;
        out.effects[draw] = copy.after;
        ++out.consulted;
        attend(panel, copy.after);
    }
    Word ports = retinal(listener.mind, screened);
    auto image = predict(ports, order);
    b.ground = ports;
    b.proved = image.known;
    b.draws = out.consulted;
    out.known = image.known;
    out.choice = image.colors;
    if (image.known != 15 || !bijective(image.colors))
        return;
    auto phrase = route(local_vocabulary, order, image.colors, 15, depth(listener.mind.genes));
    out.description = phrase;
    out.output = say(listener, phrase);
    if (!phrase.found || out.output.n != phrase.size)
        return;
    out.articulated = 1;
    out.revealed = image.colors;
    if (wire == mute)
        return;
    if (!phrase.size) {
        out.heard = 1;
        out.before = out.after = b.paint[operator_id];
        out.goal = out.before;
        return;
    }
    auto packet = out.output;
    if (wire == foreign)
        packet = b.message;
    if (wire == misarticulate && packet.n)
        packet.wave[packet.n - 1] ^= 0x6397df103b4e5153ull;
    Exchange reach{};
    if (!execute(worker, b, operator_id, packet, purses[operator_id] - out.paid[operator_id], reach))
        return;
    out.heard = 1;
    out.paid[operator_id] += reach.paid[operator_id];
    out.before = reach.before;
    out.after = reach.after;
    out.goal = reach.goal;
    out.work = reach.work;
}
AX_FN void tutorial(Saved &s, Table &b, Word draws, Word seed, const Speech &sounds, Word instruction_mode,
                    Exchange &out) {
    if (sounds.n < 2 || sounds.n > voices)
        return;
    // Allocate each auditory receptor from the pulse that actually arrived.
    auto candidate = s;
    auto map = hear(candidate, sounds, true);
    auto lesson = attend(candidate.mind, dialect(seed), sounds.n, draws, seed + 1619, learned, instruction_mode == 1, 1,
                         &map, instruction_mode >= 2, instruction_mode == 3);
    if (lesson.strokes > b.tutor)
        return;
    s = candidate;
    b.tutor -= lesson.strokes;
    b.tuition += lesson.strokes;
    out.accepted = 1;
    out.edits = lesson.edits;
    out.samples = lesson.episodes;
    out.tutor = lesson.strokes;
    out.issued = lesson.singles;
    out.choice = lesson.orders;
}
AX_FN void communicate(Saved *p, Table &b, Word cmd, Word who, Word a, Word z, Word option, const Speech &sound,
                       const Word *purses, Exchange &out) {
    if (cmd == open_table) {
        b = {};
        b.seed = a;
        for (Word i = 0; i < 3; ++i) {
            b.rig[i] = mix(a + i * 997) & 7;
            b.paint[i] = assembly(mix(a + i * 1931) % 24);
            b.strands[i] = 15;
        }
        return;
    }
    if (cmd == set_station) {
        b.rig[who] = a;
        b.paint[who] = z;
        b.strands[who] = option;
        if (!who)
            b.ready = 0;
        return;
    }
    if (cmd == filter) {
        b.sockets[who] = a;
        b.armed |= 1u << who;
        return;
    }
    if (cmd == feed) {
        Word meal = a < b.source ? a : b.source;
        Word room = 65535 - purses[who];
        if (meal > room)
            meal = room;
        b.source -= meal;
        b.food += meal;
        out.food[who] = meal;
        return;
    }
    if(cmd==guided_pull){
        if(!a || a>max_phrase || a>purses[who])return;
        auto &m=p[who].mind;
        out.before=b.paint[who];out.work={z,a,1};
        auto prefix=out.work;--prefix.size;
        out.effects[0]=realize(out.before,out.work,m.genes,b.rig[who],15);
        out.effects[1]=realize(out.before,prefix,m.genes,b.rig[who],15);
        Word prediction=identity;
        for(Word j=0;j<a;++j)prediction=compose(matrix(m,(z>>(3*j))&7),prediction);
        auto aim=predict(prediction,out.before);out.goal=aim.colors;out.known=aim.known;
        out.after=valve(out.effects[1],(z>>(3*(a-1)))&7,m.genes,b.rig[who],b.strands[who]);
        b.paint[who]=out.after;out.paid[who]=a;b.fees+=a;out.accepted=1;++b.index;
        return;
    }
    if (cmd == calibrate) {
        Word rounds = a;
        if (rounds > purses[who] / 4)
            rounds = purses[who] / 4;
        out.edits = drill(p[who].mind, b.rig[who], rounds, mix(b.seed + b.index * 12347 + who), learned);
        out.paid[who] = 4 * rounds;
        b.fees += 4 * rounds;
        out.samples = 4 * rounds;
        ++b.index;
        return;
    }
    if (cmd == teach) {
        tutorial(p[who], b, a, z, sound, option, out);
        return;
    }
    if (cmd == exhibit) {
        if (option > b.tutor)
            return;
        Word after = a;
        for (Word i = 0; i < option; ++i) {
            Word e = (z >> (2 * i)) & 3;
            after = exchanged(after, e, (e + 1) & 3);
        }
        auto words = hear(p[who], sound, true);
        order_contact(p[who].mind, words, a, after);
        out.edits = listen(p[who].mind, words, a, after);
        b.tutor -= option;
        b.tuition += option;
        out.before = a;
        out.after = after;
        out.tutor = option;
        out.samples = 1;
        return;
    }
    if (cmd == utter || cmd == continuous) {
        b.ready = 0;
        b.message = sound;
        b.start = b.paint[who];
        out.issued = who;
        if (cmd == utter) {
            auto parts = hear(p[who], sound, false);
            b.heard_words = sound.n;
            if (!recognized(parts))
                return;
            b.intent = pathway(p[who].mind, parts);
        } else {
            b.intent = a;
            b.heard_words = z;
            if (option)
                return;
        }
        if (enact(p[who], b, who, b.intent, purses[who], out)) {
            b.ready = who == 0;
            b.end = out.after;
            ++b.index;
            out.description = route(lexicon(p[who]), b.start, b.end, 15, depth(p[who].mind.genes));
            out.output = say(p[who], out.description);
            take_meal(b, who, purses[who], out);
        }
        return;
    }
    if (cmd == revoice) {
        translate(p, b, purses, a, z, option & 1, option & 2, out);
        Word actor = (option & 2) ? 2 : 1;
        if (out.heard)
            take_meal(b, actor, purses[actor], out);
        return;
    }
}
KERNEL void braid_interview(Saved *people, Table *table, Word cmd, Word who, Word a, Word z, Word option, Speech sound,
                            Word p0, Word p1, Word p2, Exchange *out, Narration *memories, Word people_count,
                            Word shutters) {
    Word purse[]{p0, p1, p2};
    Exchange r{};
    communicate(people, *table, cmd, who, a, z, option, sound, purse, r);
    if ((cmd==guided_pull || ((cmd==utter || cmd==continuous) && r.known==15)) && r.work.found && r.paid[who]==r.work.size)
        preserve_work(memories, people, *table, r, who, shutters, people_count, 0);
    else if (cmd == revoice && r.heard)
        preserve_work(memories, people, *table, r, option & 2 ? 2 : 1, shutters, people_count, 0);
    else if (cmd == exhibit && r.samples)
        preserve_work(memories, people, *table, r, who, shutters, people_count, 1);
    *out = r;
}
} // namespace axion::life::braid

namespace axion::life::braid {
AX_FN Word shifted(Word value, Word offset) {
    Word result;
    asm volatile("shfl.sync.up.b32 %0,%1,%2,0,0xffffffff;" : "=r"(result) : "r"(value), "r"(offset));
    return result;
}
AX_FN Word last_lane(Word value) {
    Word result;
    asm volatile("shfl.sync.idx.b32 %0,%1,31,31,0xffffffff;" : "=r"(result) : "r"(value));
    return result;
}
AX_FN Word lane_id() {
    Word lane;
    asm("mov.u32 %0, %%laneid;" : "=r"(lane));
    return lane;
}
AX_FN Word scan(Word word, bool reverse) {
    Word lane = lane_id();
    for (Word delta = 1; delta < 32; delta *= 2) {
        Word earlier = shifted(word, delta);
        if (lane >= delta)
            word = reverse ? compose(earlier, word) : compose(word, earlier);
    }
    return last_lane(word);
}
KERNEL void braid_hear_stream(Saved *speaker, const Count *waves, Word count, Word *result) {
    Word lane = lane_id(), scene = identity, miss = 0;
    for (Word start = 0; start < count; start += 32) {
        bool real = start + lane < count;
        Word c = real ? recognize(*speaker, waves[start + lane], false) : 0;
        miss += real && c == none;
        Word effect = real ? matrix(speaker->mind, motors + c) : identity;
        Word chunk = scan(effect, reversed(speaker->mind));
        scene = reversed(speaker->mind) ? compose(scene, chunk) : compose(chunk, scene);
    }
    for (Word d = 1; d < 32; d *= 2) {
        auto prev = shifted(miss, d);
        if (lane >= d)
            miss += prev;
    }
    if (lane == 31) {
        result[0] = scene;
        result[1] = miss;
    }
}
} // namespace axion::life::braid

namespace axion::life::braid {
AX_FN Speech forms(Word key, Word n, Word dialect_id) {
    Speech r{};
    r.n = n;
    for (Word i = 0; i < n; ++i)
        r.wave[i] = (Count(mix(key + (dialect_id + 1) * 6073)) << 32) |
                    ((mix(key + (i + 1) * 1853) & ~31u) | 1 | (i << 1) | (dialect_id << 4));
    return r;
}
AX_FN Speech pronounce(const Speech &names, Phrase phrase) {
    Speech result{};
    result.n = phrase.n;
    for (Word i = 0; i < phrase.n; ++i)
        result.wave[i] = names.wave[phrase.cell[i]];
    return result;
}
AX_FN Passage conversation(Genes ga, Genes gb, Genes gc, Word key, Word n, Word rep, Word length, Word samples,
                           Word arm, Word grammar, Word pattern) {
    Passage rec{};
    auto a = dialect(key + 4309), b = dialect(key + 8017);
    rec.key = key;
    rec.n = n;
    bool borrow = arm == loan || arm == unlent;
    auto original = appeal(key, n, borrow ? 1 : length);
    Word focus = grammar && n > 2 ? 2 + mix(key + 7901) % (n - 2) : focal(key, n);
    original.cell[original.n / 2] = focus;
    rec.focus = focus;
    rec.length = original.n;
    Word backwards[2]{Word(grammar && ((key >> 15) & 1)), Word(grammar && ((key >> 19) & 1))};
    rec.laws = backwards[0] + 2 * backwards[1];
    rec.utterance = phrase_code(sequence(original, backwards[0]));
    auto source_names = forms(key, n, 0), receiver_names = forms(key, n, 1);
    for (Word j = 0; j < n; ++j) {
        rec.public_a[j] = a.law[j];
        rec.public_b[j] = b.law[j];
    }
    Genes genes[]{ga, gb, gc};
    for (Word i = 0; i < 3; ++i) {
        rec.wrist[i] = genes[i].muscle & 7;
        rec.rig[i] = new_rig(key + i * 1471, 0);
    }
    Word seeds[]{1277, 9403, 2399};
    for (Word twin = 0; twin < 2; ++twin) {
        auto foreign = a;
        if (twin)
            foreign.law[focus] = exchanged(foreign.law[focus], (key >> 7) & 3, ((key >> 7) + 1) & 3);
        Saved individuals[3]{};
        Table tray{};
        tray.seed = key;
        Word purse[]{1024, 1024, 1024};
        for (Word i = 0; i < 3; ++i) {
            auto &p = individuals[i];
            p.mind.genes = genes[i];
            auto map = hear(p, i ? receiver_names : source_names, true);
            auto native = i ? b : foreign;
            attend(p.mind, native, n, rep, key + seeds[i], learned, 0, focus, &map, grammar, backwards[!!i]);
            drill(p.mind, rec.rig[i], rep, key + seeds[i] + 991, learned);
            tray.rig[i] = rec.rig[i];
            tray.strands[i] = 15;
        }
        tray.paint[0] = pattern == 0   ? ((key & 3) * 85)
                        : pattern == 1 ? ((key & 1) ? 0x50u : 0x11u)
                                       : assembly(mix(key + 29) % 24);
        tray.paint[1] = assembly(mix(key + 6277) % 24);
        tray.paint[2] = assembly(mix(key + 503) % 24);
        auto &t = rec.twin[twin];
        t.screen = tray.paint[0];
        t.before = tray.paint[2];
        t.target = instruction(foreign, original, order);
        Word goal = transfer(t.before, t.target);
        Motors allowed{};
        allowed.present = (1u << n) - 1;
        for (Word j = 0; j < n; ++j)
            allowed.table[j] = b.law[j];
        t.ceiling = route(allowed, order, t.target, 15, 8).found;
        t.native_order = reversed(individuals[1].mind) | (2 * reversed(individuals[2].mind));
        auto request = pronounce(source_names, sequence(original, backwards[0]));
        Exchange from{};
        communicate(individuals, tray, utter, 0, 0, 0, 0, request, purse, from);
        t.observed = tray.paint[0];
        t.source = from.work.found && from.known == 15 && t.observed == transfer(t.screen, t.target);
        Word wire = arm == first_view    ? no_repeat
                    : arm == unchanging  ? repeat_probe
                    : arm == quiet       ? mute
                    : arm == misheard    ? misarticulate
                    : arm == native_only ? ::axion::life::braid::foreign
                    : arm == misleading  ? unseeing
                                         : intact;
        Exchange to{};
        translate(individuals, tray, purse, samples, wire, arm == loan, true, to);
        t.known = to.known;
        t.human = to.articulated;
        t.decoded = to.heard;
        t.claim = to.choice;
        t.n = to.output.n;
        t.fee = tray.fees;
        t.after = tray.paint[2];
        t.plan = to.work.stream;
        t.steps = to.work.size;
        t.queries = to.consulted;
        t.borrowed = to.borrowed;
        t.food = harvest(t.after, goal, 8).swallowed;
        for (Word i = 0; i < t.queries; ++i) {
            t.tests[i] = to.probes[i];
            t.seen[i] = to.effects[i];
        }
        for (Word j = 0; j < t.n; ++j) {
            Word ix = 0;
            while (ix < n && receiver_names.wave[ix] != to.output.wave[j])
                ++ix;
            if (ix >= n) {
                t.human = 0;
                break;
            }
            t.said |= ix << (3 * j);
        }
        if (borrow) {
            // The demonstrator is absent. The translator hears a NEW mixture of
            // its own word and the borrowed form, on a different set of floats.
            Word local_word = mix(key + 9067) % n;
            t.other = local_word;
            auto &p = individuals[1];
            t.later_start = tray.paint[1];
            t.later_goal = transfer(transfer(t.later_start, b.law[local_word]), foreign.law[focus]);
            Speech mixed{};
            mixed.n = 2;
            mixed.wave[backwards[1]] = receiver_names.wave[local_word];
            mixed.wave[1 - backwards[1]] = source_names.wave[focus];
            Exchange last{};
            t.later_received = execute(p, tray, 1, mixed, purse[1], last);
            t.later_end = tray.paint[1];
            t.later_work = last.work.stream;
            t.later_steps = last.work.size;
        }
    }
    return rec;
}
KERNEL void braid_dialogues(const Genes *parents, Word size, Word cases, Word seed, Word n, Word rep, Word length,
                            Word budget, Word arm, Word grammar, Word mask, const Genes *workers, Word nw,
                            Passage *records) {
    Word i = tid();
    if (i >= cases)
        return;
    Word k = mix(seed + i * 9619u);
    auto third = nw ? workers[mix(k + 8723) % nw] : parents[mix(k + 8723) % size];
    records[i] = conversation(parents[mix(k + 1223) % size], parents[mix(k + 6469) % size], third, k, n, rep, length,
                              budget, arm, grammar, mask);
}
} // namespace axion::life::braid
