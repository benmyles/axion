#include "axion/feedback_contact.hpp"
namespace axion::life::braid {
AX_FN void exchange_frames(QuestionBook &book, Word f) {
    auto words = book.full[f];
    book.full[f] = book.full[f + 1];
    book.full[f + 1] = words;
    Word slot = book.slot[f];
    book.slot[f] = book.slot[f + 1];
    book.slot[f + 1] = slot;
}
AX_FN Word rehearse_hand(Mind &mind, regulation::Source &organ, Word rounds, Word key, Word rig, bool feedback = true,
                         bool structural = false) {
    Word edits = 0;
    for (Word r = 0; r < rounds; ++r)
        for (Word motor = 0; motor < motors; ++motor) {
            Word scene = shade(key + r * 9427, motor), place = mix(key + r * 1333 + motor * 71) & 3;
            auto predicted = predict(matrix(mind, motor), scene);
            Word end = valve(scene, motor, mind.genes, rig, 15);
            bool tried = predicted.known & (1u << place),
                 agrees = tried && pigment(predicted.colors, place) == pigment(end, place);
            edits += receive_contact(
                mind, motor, compatible(scene, end, mind.genes.arbor & 3), 0,
                [&](Word old, Word conflict, Word proposal) {
                    auto release = regulation::discharge(organ.program, organ.cells, old, conflict, proposal,
                                                         feedback && tried, feedback && agrees);
                    return regulation::apply(
                        old, regulation::candidate(organ.program, organ.cells.spike, old, proposal, structural),
                        release);
                });
        }
    return edits;
}
KERNEL void apprentice_hand(braid::Saved *people, Table *tray, Word who, Word requested, Word purse, Exchange *result,
                            regulation::Source *source, Word mode) {
    Word rounds = requested < purse / 4 ? requested : purse / 4;
    Exchange out{};
    out.edits = rehearse_hand(people[who].mind, *source, rounds, mix(tray->seed + tray->index * 12347 + who),
                              tray->rig[who], (mode & 255u) != apprentice::quiet, mode & apprentice::remodel);
    out.paid[who] = out.samples = rounds * 4;
    tray->fees += out.paid[who];
    ++tray->index;
    *result = out;
}
AX_FN apprentice::LessonScore regulated_lesson(Mind individual, Episode scene, regulation::Source organ, Word history,
                                               Word language, Word practice, Word mode, Word reversal, Word i,
                                               Word count, unsigned short *schedules, Word frames = 2, Word hand = 0,
                                               Word varied = 0, const regulation::Source *voice = nullptr) {
    Word rig = varied ? mix(history ^ 0x59f0843bu) & 127 : 0;
    if (hand & 1)
        rehearse_hand(individual, organ, practice, history + 61931, rig, true, hand & apprentice::remodel);
    else if (practice)
        drill(individual, rig, practice, history + 61931, learned);
    Narration m{};
    auto book = question_book(language);
    acquire_names(m, individual.genes, book.names, 12, history);
    Word names = 0;
    for (Word j = 0; j < 8; ++j)
        names += ident(m, intern(m, book.names.wave[j], false)) == j;
    ContactPractice control{voice ? *voice : organ, mode | (hand & apprentice::remodel), i, count, schedules};
    apprentice::LessonScore result{};
    result.names = names;
    Word cost = 0;
    for (Word phase = 0; phase <= reversal; ++phase) {
        if (phase)
            for (Word f = 0; f < frames; f += 2)
                exchange_frames(book, f);
        for (Word f = 0; f < frames; ++f) {
            Word key = history + 3779 * (f + 1) + 0x9719u * phase;
            present_forms(
                m, individual, book.names, book.full[f], book.slot[f], f, 0, 0, 64, key, rig, cost, 0, 4,
                [&](Narration &v, Genes g, const Speech &q, Count answer) { return control.hear(v, g, q, answer); });
        }
        FieldStudent scored{};
        for (const auto &form : m.forms)
            scored.forms += form.examples != 0 && !braid::sequence_form(form);
        score_field(scored, m, scene, book, language);
        if (!phase)
            result.intermediate = scored.diagnostic;
        result.forms = scored.forms;
        result.qualified = scored.diagnostic;
        result.answered = scored.answered;
        result.correct = scored.correct;
        result.offered = scored.offered;
        result.changed = scored.changed;
        result.both = scored.both;
        result.unseen_answers = scored.empty_answers;
        result.reply_mask = scored.answer_mask;
        result.replies = scored.replies;
        result.probe_mask = scored.diagnostic_mask;
        result.probe_replies = scored.diagnostic_codes;
    }
    if (frames == 4) {
        // An unseen physical manipulation, generated after all training. The
        // unobstructed and interrupted paths are run mechanically for scoring.
        // Only the creature's own retinal and predicted planes enter its voice.
        auto probe = demonstration(individual, mix(history ^ 0xb1977e51u), rig, 701);
        for (Word f = 0; f < 4; ++f) {
            result.physical[f] = probe.fact[f];
            for (Word j = 0; j < 4; ++j) {
                Word truth = pigment(probe.fact[f], j), actual = pigment(probe.fact[1], j);
                auto words = fill_question(book.full[f], book.slot[f], book.names.wave[j]);
                Word answer = spoken(field_question(m, probe.self, words), book.names);
                bool correct = answer == 4 + truth;
                if (answer < 8) {
                    result.mechanical_mask[f] |= 1u << j;
                    result.responses[f] |= answer << (3 * j);
                }
                result.predicted[f] += !!(visible(probe.self, f) & (1u << j));
                result.mechanical[f] += correct;
                result.divergent[f] += truth != actual;
                result.divergent_right[f] += (truth != actual) && correct;
            }
        }
    }
    result.updates = control.updates;
    result.released = control.released;
    result.attempts = control.attempts;
    result.matches = control.matches;
    return result;
}
KERNEL void apprentice_language(const Mind *minds, const Episode *episodes, Word slot, apprentice::Teachers tissue,
                                Word count, Word seed, Word practice, Word mode, Word reversal, Word frames, Word hand,
                                Word varied, unsigned short *schedules, apprentice::LessonScore *scores) {
    Word i = tid();
    if (i >= count)
        return;
    Word language = mix(seed + i * 7919);
    scores[i] =
        regulated_lesson(minds[slot], episodes[slot], tissue.motor, language, language, practice, mode, reversal, i,
                         count, schedules, frames, hand, varied, tissue.voice.band == ~0u ? nullptr : &tissue.voice);
}
} // namespace axion::life::braid

namespace axion::life::braid {
AX_FN Word reachable_port(Word choices, Word draw) {
    Word rank = draw % __builtin_popcount(choices);
    for (Word j = 0; j < rank; ++j)
        choices &= choices - 1;
    return __builtin_ctz(choices);
}
// A carbon bead enters one of four ducts. A resident sees the inlet before a
// reach and feels its yield afterwards. Tidal pressure can turn the distributor.
AX_FN apprentice::Harvest live_at_duct(apprentice::Animal animal, regulation::Cells cells, Word seed, Word pulses,
                                       Word seasons, Word mode) {
    apprentice::Harvest h{};
    Word left = 0, first = assembly(mix(seed + 931) % 24), shift = 1 + (mix(seed + 1013) % 3);
    Word span = pulses / seasons, middle = 0;
    for (Word k = 0; k < 4; ++k)
        middle |= ((pigment(first, k) + shift) & 3) << (2 * k);
    for (Word i = 0; i < pulses; ++i) {
        Word rain = mix(seed + i * 0x9e3779b9u), inlet = (rain >> 10) & 3, phase = i / span;
        Word outlet = pigment((phase & 1) ? middle : first, inlet);
        Word base = inlet * 4, row = 15u << base, open = (~left >> base) & 15;
        bool committed = open && !(open & (open - 1));
        Word reach = reachable_port(open ? open : 15, mix(rain ^ 0xa91d2053u));
        bool swallowed = reach == outlet;
        h.swallowed += swallowed;
        h.escaped += !swallowed;
        h.after_shift += swallowed && phase == 1;
        h.after_return += swallowed && phase == 2;
        h.confident += committed;
        h.matches += committed && swallowed;
        Word touched = 1u << (base + reach);
        Word conflict = swallowed ? row ^ touched : touched;
        Word old = left;
        Word proposed = (old & ~row) | (row & rule(animal.traits, old, conflict));
        auto access = mode & 255u;
        bool feedback = access != apprentice::quiet;
        Word release = regulation::discharge(animal.program, cells, old, conflict, proposed, feedback && committed,
                                             feedback && committed && swallowed);
        if (access == apprentice::unrestricted)
            release = all;
        else if (access == apprentice::displaced)
            release = regulation::rolled(release, 7);
        Word next = regulation::candidate(animal.program, cells.spike, old, proposed,
                                          (mode & apprentice::remodel) && access != apprentice::unrestricted);
        left = regulation::apply(old, next, release & row);
    }
    return h;
}
KERNEL void apprentice_feeding(const apprentice::Animal *community, Word count, Word seed, Word lives, Word pulses,
                               Word mode, regulation::Cells natal, apprentice::Harvest *records) {
    Word i = tid();
    if (i >= count)
        return;
    auto animal = community[i];
    apprentice::Harvest total{};
    if (animal.ancestor != ~0u)
        for (Word life = 0; life < lives; ++life) {
            // The held-out populations face the same sequence of physical seasons.
            Word climate = lives == 1 ? mix(seed + i * 2971) : mix(seed + life * 0x7f4a7c15u);
            auto h = live_at_duct(animal, natal, climate, pulses, 3, mode);
            total.swallowed += h.swallowed;
            total.escaped += h.escaped;
            total.after_shift += h.after_shift;
            total.after_return += h.after_return;
            total.confident += h.confident;
            total.matches += h.matches;
        }
    records[i] = total;
}
AX_FN Word breed_shuffle(Word value, Word lane) {
    Word v;
    asm("shfl.sync.idx.b32 %0,%1,%2,31,0xffffffff;" : "=r"(v) : "r"(value), "r"(lane));
    return v;
}
AX_FN Word breed_sum(Word value, Word lane) {
    for (Word step = 1; step < 32; step *= 2) {
        Word q;
        asm("shfl.sync.up.b32 %0,%1,%2,0,0xffffffff;" : "=r"(q) : "r"(value), "r"(step));
        if (lane >= step)
            value += q;
    }
    return value;
}
AX_FN Word mutate_contact(Word value, Word seed, Word rate, Word bits) {
    return (seed & 255) < rate ? value ^ (1u << ((seed >> 8) % bits)) : value;
}
KERNEL void apprentice_reproduce(const apprentice::Animal *adults, const apprentice::Harvest *harvest, Word count,
                                 Word seed, Word selected, Word all_loci, apprentice::Animal *offspring,
                                 apprentice::Rations *ledger) {
    Word j = tid();
    if (j >= count)
        return;
    Word lane = j & 31, id = (j + (seed & 31)) & (count - 1);
    auto parent = adults[id];
    Word stock = harvest[id].swallowed;
    Word carbon = breed_shuffle(breed_sum(stock, lane), 31);
    Word weight = selected ? stock : Word(parent.ancestor != ~0u);
    Word sum = breed_sum(weight, lane), total = breed_shuffle(sum, 31);
    Word quota = carbon / 8;
    if (quota > 32)
        quota = 32;
    Word washed = breed_sum(harvest[id].escaped, lane), living = breed_sum(parent.ancestor != ~0u, lane);
    if (lane == 31) {
        auto &patch = ledger[j / 32];
        patch.carbon += carbon;
        patch.washed += washed;
        patch.adults += living;
        patch.offspring += quota;
        patch.left += carbon - 8 * quota;
    }
    Word key = mix(seed ^ mix(j + 55347));
    Word ticket = total ? key % total : 0, from = 0;
    for (Word stride = 16; stride; stride >>= 1) {
        Word candidate = from + stride - 1;
        Word edge = breed_shuffle(sum, candidate & 31);
        if (candidate < 32 && ticket >= edge)
            from += stride;
    }
    from &= 31;
    apprentice::Animal child{};
    child.program.first = breed_shuffle(parent.program.first, from);
    child.program.second = breed_shuffle(parent.program.second, from);
    child.traits = breed_shuffle(parent.traits, from);
    child.ancestor = breed_shuffle(parent.ancestor, from);
    Word rate = 4 + ((child.traits >> (all_loci == 2 ? 22 : 8)) & 31);
    child.program.first = mutate_contact(child.program.first, mix(key + 37), rate, 32);
    child.program.second = mutate_contact(child.program.second, mix(key + 511), rate, 32);
    if (all_loci == 2) {
        child.traits ^= mutate_contact(0, mix(key + 1091), rate, 4);
        child.traits ^= mutate_contact(0, mix(key + 1831), rate, 17) << 5;
        child.traits ^= mutate_contact(0, mix(key + 5873), rate / 4, 1) << 4;
        child.traits ^= mutate_contact(0, mix(key + 1759), rate, 5) << 22;
    } else if (all_loci) {
        child.traits = mutate_contact(child.traits, mix(key + 1091), rate, 4);
        child.traits ^= (mutate_contact(0, mix(key + 1759), rate, 5) << 8);
    }
    if (!total || lane >= quota)
        child = {};
    offspring[id] = child;
}
} // namespace axion::life::braid

namespace axion::life::braid {
// Enrollment and ranking see only an independent laboratory scene. A real
// field episode is opened later, after a particular pupil has been chosen.
KERNEL void apprentice_survey(const Mind *minds, const Word *slots, const apprentice::Teachers *organs, Word count,
                              Word dialects, Word seed, Word practice, Word hand, Word varied, Word *out) {
    Word i = tid();
    if (i >= count * dialects)
        return;
    Word pupil = i / dialects, language = mix(seed + (i % dialects) * 7919);
    auto tissue = organs[pupil];
    auto r =
        regulated_lesson(minds[slots[pupil]], {}, tissue.motor, language, language, practice, apprentice::inherited, 1,
                         0, 1, nullptr, 2, hand, varied, tissue.voice.band == ~0u ? nullptr : &tissue.voice);
    out[i] = r.intermediate | (r.qualified << 4) | (r.names << 8);
}
} // namespace axion::life::braid
