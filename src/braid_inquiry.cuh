#include "axion/braid_inquiry.hpp"
namespace axion::life::braid {
struct QuestionBook {
    Speech names{}, full[4]{}, topic{}, reply{};
    Word slot[4]{};
};
// The community's nomenclature and syntax are different in every replicated world.
AX_FN QuestionBook question_book(Word key) {
    QuestionBook b{};
    Count prefix = Count(mix(key + 349)) << 32;
    auto wave = [&](Word i) { return prefix | ((mix(key + 73 * i) & ~63u) | i); };
    b.names.n = 8;
    for (Word i = 0; i < 8; ++i)
        b.names.wave[i] = wave(i + 1);
    for (Word i = 0; i < 4; ++i) {
        b.full[i].n = 3;
        b.slot[i] = mix(key + i * 73) % 3;
        for (Word j = 0; j < 3; ++j)
            b.full[i].wave[j] = wave(j ? 10 + i : 20 + i);
        b.full[i].wave[b.slot[i]] = 0;
    }
    b.topic.n = b.reply.n = 2;
    b.topic.wave[0] = wave(25);
    b.reply.wave[0] = wave(27);
    b.topic.wave[1] = b.reply.wave[1] = wave(26);
    return b;
}
AX_FN void educate(Narration &m, Mind &g, const QuestionBook &book, Word reps, Word key, Word rig, Word &cost,
                   Word width) {
    drill(g, rig, 12, key + 937, learned);
    acquire_names(m, g.genes, book.names, 6, key);
    for (Word frame = 0; frame < 4; ++frame)
        acquire_form(m, g, book.names, book.full[frame], book.slot[frame], frame, 0, 2, reps, key + frame * 1933, rig,
                     cost, 0, width);
    acquire_form(m, g, book.names, book.topic, none, result, 2, 2, reps, key + 7937, rig, cost, 0, width);
    acquire_form(m, g, book.names, book.reply, none, result, 3, 2, reps, key + 3919, rig, cost, 0, width);
}
AX_FN Word spoken(const Answer &a, const Speech &names) {
    return a.answered && a.words.n == 1 ? index_of(names, a.words.wave[0]) : none;
}
KERNEL void braid_inquiries(const Genes *stock, Word size, Word cases, Word seed, Word repeats, Word holdout,
                            Interrogation *records) {
    Word i = tid();
    if (i >= cases)
        return;
    Word key = mix(seed + i * 8573);
    auto book = question_book(key);
    Narration adult[2]{};
    Mind individuals[2]{};
    Word rig = new_rig(key, 1), cost = 0;
    Interrogation record{};
    record.key = key;
    record.rig = rig;
    for (Word k = 0; k < 2; ++k) {
        individuals[k].genes = stock[mix(key + 3643 * k) % size];
        educate(adult[k], individuals[k], book, repeats, mix(key + 3907 * k), k ? new_rig(key + 5101, 1) : rig, cost,
                4 - holdout);
        for (Word s = 0; s < sounds_max; ++s)
            record.nouns[k] += ident(adult[k], s) != none;
        for (auto &f : adult[k].forms)
            record.forms[k] += f.examples && __builtin_popcount(all ^ f.pruned) == 1;
    }
    record.training = cost;
    Word start = assembly(mix(key + 7417) % 24);
    Plan path{0, 2 + ((key >> 11) & 3), 1};
    for (Word j = 0; j < path.size; ++j)
        path.stream |= ((mix(key + j * 3011) >> 8) & 3) << (3 * j);
    record.stream = path.stream;
    record.strokes = path.size;
    record.wrist = individuals[0].genes.muscle & 7;
    record.board = start;
    for (Word twin = 0; twin < 2; ++twin) {
        auto scene = encounter(individuals[0], key, rig, twin ? 0 : 15, start, path);
        record.ends[twin] = scene.fact[result];
        record.prefix = scene.fact[shortened];
        record.aim = scene.fact[intended];
        {
            for (Word f = 0; f < 4; ++f)
                for (Word c = 0; c < 2 * holdout; ++c) {
                    auto self = adult[0];
                    glimpse(self, scene.self, 15);
                    Word ref = held_subject(c);
                    auto q = fill_question(book.full[f], book.slot[f], book.names.wave[ref]);
                    record.frame_answers[twin][f] |= spoken(question(self, q, false), book.names) << (8 * c);
                }
        }
        for (Word ctx = 0; ctx < 2 * holdout; ++ctx) {
            // Crossed prior conversations share the identical last question.
            Word subject0 = held_subject(ctx);
            Word mode = (key >> 16) & 1;
            auto first = fill_question(book.full[beginning], book.slot[beginning], book.names.wave[subject0]);
            auto follow = mode ? book.reply : book.topic;
            for (Word arm = 0; arm < report_arms; ++arm) {
                auto a = adult[0], b = adult[1];
                if (arm == untaught) {
                    a = {};
                    b = {};
                }
                a.topic = a.reply = b.topic = b.reply = 0;
                glimpse(a, scene.self, arm == sighted ? 15 : 13);
                glimpse(b, scene.peer, arm == private_scene ? 1 : 3);
                auto &row = record.trial[arm][ctx][twin];
                auto opening = question(a, first, false);
                row.before = spoken(opening, book.names);
                if (arm == missing_context)
                    a.topic = a.reply = 0;
                Word channel = arm == silent_witness ? 2 : arm == echo_question ? 3 : arm == incorrect_witness ? 4 : 1;
                auto turn = ask_people(a, b, follow, channel, arm != forget_answer);
                auto &out = turn.original;
                row.asked = out.queried;
                row.witness = spoken(turn.witness, book.names);
                row.answer = spoken(out.answered ? out : turn.final, book.names);
                Word public_subject = mode ? row.before : subject0;
                if (out.queried && public_subject != none) {
                    auto explicit_words =
                        fill_question(book.full[result], book.slot[result], book.names.wave[public_subject]);
                    bool same = explicit_words.n == out.words.n;
                    for (Word w = 0; w < explicit_words.n; ++w)
                        same = same && out.words.wave[w] == explicit_words.wave[w];
                    row.reframed = same;
                }
                // The witness leaves. Pose a reciprocal question using only the actual answer that was heard.
                if (row.answer != none) {
                    auto repair = fill_question(book.full[result], book.slot[result], book.names.wave[row.answer]);
                    auto reversed = question(a, repair, false);
                    row.reverse = spoken(reversed, book.names);
                    row.reverse_from_report = reversed.by_report;
                }
                row.repeat = spoken(question(a, follow, false), book.names);
            }
        }
    }
    records[i] = record;
}
} // namespace axion::life::braid
