#include "axion/braid_discourse.hpp"
#include "axion/feedback_contact.hpp"

namespace axion::life::braid {
AX_FN void glimpse(Narration &m, Episode e, Word unshuttered) {
    for (Word i = 0; i < 4; ++i)
        if (!(unshuttered & (1u << i)))
            expose(e, i, 0, 0);
    experience(m, e);
}
AX_FN void preserve_work(Narration *m, Saved *people, Table &b, const Exchange &out, Word actor, Word shutters,
                         Word count, Word witnessed) {
    auto tag = mix(b.seed + b.index * 1229u + out.before * 173 + out.after);
    for (Word k = 0; k < count; ++k) {
        auto e = episode(people[k].mind, out.before, out.after, {out.goal, out.known}, out.work,
                         witnessed || actor != k, tag);
        glimpse(m[k], through_glass(e, b.eyes[k]), (shutters >> (4 * k)) & 15);
    }
}
AX_FN Speech fill_question(Speech form, Word position, Count word) {
    if (position < form.n)
        form.wave[position] = word;
    return form;
}
AX_FN Word index_of(const Speech &words, Count wave) {
    for (Word k = 0; k < words.n; ++k)
        if (words.wave[k] == wave)
            return k;
    return none;
}
AX_FN void public_turn(Narration &m, Count mention, Count reply) {
    Word a = ident(m, intern(m, mention, false)), b = ident(m, intern(m, reply, false));
    m.topic = a == none ? 0 : a + 1;
    m.reply = b == none ? 0 : b + 1;
    m.pending = 0;
}
AX_FN Word acquire_names(Narration &m, Genes g, const Speech &sounds, Word repeats, Word seed) {
    Word edits = 0;
    for (Word i = 0; i < repeats; ++i) {
        for (Word feature = 0; feature < 8; ++feature) {
            Word k = mix(seed + feature * 2347);
            Word location = feature < 4 ? feature : (k + i) & 3;
            Word paint = assembly(mix(k + i * 1861) % 24);
            Word color = feature < 4 ? ((k + i) & 3) : feature - 4;
            paint = (paint & ~(3u << (2 * location))) | (color << (2 * location));
            edits += notice(m, g, sounds.wave[feature], location, paint);
        }
    }
    return edits;
}
struct WitnessScene {
    Episode self{}, peer{};
    Word fact[4]{}, work = 0, rig = 0, wrist = 0, bands = 0;
    Plan motion{};
};
AX_FN WitnessScene encounter(Mind &worker, Word key, Word rig, Word mask, Word start, Plan motor) {
    WitnessScene q{};
    q.rig = rig;
    q.wrist = worker.genes.muscle & 7;
    q.bands = mask;
    q.motion = motor;
    auto prefix = motor;
    if (prefix.size)
        --prefix.size;
    Word prior = realize(start, prefix, worker.genes, rig, 15);
    Word full = realize(start, motor, worker.genes, rig, 15);
    Word end = motor.size ? valve(prior, (motor.stream >> (3 * prefix.size)) & 7, worker.genes, rig, mask) : start;
    q.fact[0] = start;
    q.fact[1] = end;
    q.fact[2] = full;
    q.fact[3] = prior;
    Word predicted = identity;
    for (Word i = 0; i < motor.size; ++i)
        predicted = compose(matrix(worker, (motor.stream >> (3 * i)) & 7), predicted);
    auto aim = predict(predicted, start);
    q.self = episode(worker, start, end, aim, motor, false, key);
    q.peer = episode(worker, start, end, {}, motor, true, key);
    q.work = motor.size;
    return q;
}
AX_FN WitnessScene demonstration(Mind &worker, Word key, Word rig, Word i) {
    Word k = mix(key + 1693 * i), moves = 2 + ((k >> 27) & 3);
    Plan plan{0, moves, 1};
    for (Word j = 0; j < moves; ++j)
        plan.stream |= ((mix(k + j * 31) >> 6) & 3) << (3 * j);
    return encounter(worker, mix(k + 1319), rig, 1 + mix(k + 937) % 15, assembly(mix(k + 991) % 24), plan);
}
AX_FN Word actual_answer(Word state, Word reference) {
    if (reference < 4)
        return 4 + pigment(state, reference);
    Word row = lamps(state, reference - 4, 3);
    if (!row || (row & (row - 1)))
        return none;
    return __builtin_ctz(row);
}
// The syllabus belongs to the demonstrator. The receiver gets the public
// sound sequence, an answer and its own sensory episode.
template<class Receive> AX_FN Word present_forms(Narration &m, Mind &worker, const Speech &names, const Speech &form, Word slot, Word frame,
                        Word role, Word argument_kind, Word repeats, Word key, Word rig, Word &cost, Word arm,
                        Word width, Receive receive) {
    Word edits = 0;
    for (Word i = 0; i < repeats; ++i) {
        auto display = demonstration(worker, key, rig, i);
        cost += display.work;
        experience(m, display.self);
        Word k = mix(key + i * 937);
        Word subject0 = ((k >> 12) % width) + 4 * (argument_kind == 2 ? ((k >> 3) & 1) : argument_kind);
        // Prior conversational referents and replies originate in a publicly observed example.
        Word prev = ((k >> 18) % width) + 4 * ((k >> 23) & 1), prior = actual_answer(display.fact[(k >> 24) & 3], prev);
        public_turn(m, names.wave[prev], names.wave[prior]);
        Word target = role == 0 ? subject0 : role == 2 ? prev : prior;
        Word label = actual_answer(display.fact[frame], target);
        if (label == none)
            continue;
        auto q = fill_question(form, slot, names.wave[subject0]);
        if (arm == 2)
            label = (label & 4) | ((label + 1) & 3);
        if (arm != 1)
            edits += receive(m,worker.genes,q,names.wave[label]);
    }
    return edits;
}
AX_FN Word acquire_form(Narration &m, Mind &worker, const Speech &names, const Speech &form, Word slot, Word frame,
                        Word role, Word argument_kind, Word repeats, Word key, Word rig, Word &cost, Word arm = 0,
                        Word width = 4) {
    return present_forms(m,worker,names,form,slot,frame,role,argument_kind,repeats,key,rig,cost,arm,width,
        [](Narration &v,Genes g,const Speech &q,Count reply){return teach_form(v,g,q,reply);});
}
AX_FN SpokenEvidence ask_people(Narration &one, Narration &two, const Speech &words, Word channel, bool retain) {
    SpokenEvidence r{};
    r.original = question(one, words, true);
    if (r.original.queried && channel) {
        auto public_request = r.original.words;
        if (channel == 3)
            public_request = words;
        r.witness = question(two, public_request, false);
        if (r.witness.answered && channel != 2) {
            Count reply = r.witness.words.wave[0];
            if (channel == 4) {
                Word local = r.witness.said;
                reply = name_of(two, (local & 4) | ((local + 1) & 3));
            }
            r.final = accept(one, reply, retain);
        }
    }
    return r;
}
#include "braid_perspective.cuh"
template<bool regulated> AX_FN void recall_course(Narration *m,Saved *p,Table *tray,Course course,Receipt *out,
                                                regulation::Source *organ,Word mode) {
    auto &c = course;
    auto &n = m[c.who];
    auto &g = p[c.who].mind;
    Receipt r{};
    if (c.cmd == set_lens) {
        tray->eyes[c.who] = {static_cast<unsigned char>(c.place), static_cast<unsigned char>(c.board)};
    } else if (c.cmd == share_view || c.cmd == compare_views) {
        study_view(n, m[c.peer], g, *tray, c, r);
    } else if (c.cmd == point_lesson) {
        if (tray->tutor) {
            r.learned = notice(n, g.genes, c.words.wave[0], c.place, tray->paint[c.who]);
            r.cost = r.offered = 1;
        }
    } else if (c.cmd == name_lesson) {
        Word reps = c.reps;
        if (reps > tray->tutor / 8)
            reps = tray->tutor / 8;
        r.learned = acquire_names(n, g.genes, c.names, reps, c.seed);
        r.cost = r.offered = 8 * reps;
    } else if (c.cmd == form_lesson) {
        auto backup = n;
        Word cost = 0;
        if constexpr(regulated) {
            ContactPractice gate{*organ,mode,0,1,nullptr};
            r.learned=present_forms(backup,g,c.names,c.words,c.slot,c.frame,c.role,c.kind,c.reps,c.seed,tray->rig[c.who],cost,0,4,
                [&](Narration &v,Genes genes,const Speech &words,Count answer){return gate.hear(v,genes,words,answer);});
            if(cost<=tray->tutor)*organ=gate.source;
        } else r.learned = acquire_form(backup, g, c.names, c.words, c.slot, c.frame, c.role, c.kind, c.reps, c.seed,
                                 tray->rig[c.who], cost);
        if (cost <= tray->tutor) {
            n = backup;
            r.offered = c.reps;
            r.cost = cost;
        } else
            r.learned = 0;
    } else if (c.cmd == manual_caption) {
        if (tray->tutor) {
            if constexpr(regulated) {
                ContactPractice gate{*organ,mode,0,1,nullptr};
                r.learned=gate.hear(n,g.genes,c.words,c.echo);
                *organ=gate.source;
            } else r.learned = teach_form(n, g.genes, c.words, c.echo);
            r.offered = r.cost = 1;
        }
    } else if (c.cmd == normal_query) {
        if (n.pending && c.words.n == 1) {
            r.turns.final = accept(n, c.words.wave[0], true);
            r.status = r.turns.final.answered;
        }
        if (!r.status) {
            r.turns = deliver_query(m, g, c, question(n, c.words, true), r, c.channel);
            r.status = r.turns.original.matched;
        }
    } else if (c.cmd == manual_answer) {
        r.turns.final = accept(n, c.echo, true);
        r.status = r.turns.final.answered;
    } else if (c.cmd == witness_reply && n.pending) {
        Word frame = (n.pending - 1) >> 3, ref = (n.pending - 1) & 7;
        Answer ask{};
        ask.queried = 1;
        ask.source = ref;
        ask.frame = frame;
        ask.words = full_question(n, frame, ref);
        r.turns = deliver_query(m, g, c, ask, r, c.channel ? c.channel : 1);
        r.status = 1;
    } else if (c.cmd == clear_topic) {
        n.topic = 0;
        n.reply = 0;
        n.pending = 0;
    } else if (c.cmd == clear_reports) {
        n.event.known &= ~n.reported;
        n.reported = 0;
        n.pending = 0;
    }
    if (r.cost) {
        tray->tutor -= r.cost;
        tray->tuition += r.cost;
    }
    for (Word i = 0; i < forms_max; ++i)
        r.forms += n.forms[i].examples != 0;
    for (Word i = 0; i < sounds_max; ++i)
        r.nouns += ident(n, i) != none;
    Word partner = acquaintance(n, c.address, false);
    if (partner != none) {
        auto view = contact(n, partner);
        r.established = anchored(view);
        r.hypotheses = __builtin_popcount(candidates(view));
        r.pairs = seen_ports(view);
    }
    *out = r;
}
KERNEL void braid_recall(Narration *m,Saved *p,Table *tray,Course course,Receipt *out) {
    recall_course<false>(m,p,tray,course,out,nullptr,0);
}
KERNEL void braid_recall_regulated(Narration *m,Saved *p,Table *tray,Course course,Receipt *out,regulation::Source *organ,Word mode) {
    recall_course<true>(m,p,tray,course,out,organ,mode);
}
} // namespace axion::life::braid
