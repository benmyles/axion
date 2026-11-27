#include "axion/field_class.hpp"
#include "axion/floodplain.hpp"
namespace axion::life::braid {
AX_FN Answer field_question(Narration &memory, Episode scene, const Speech &words) {
    experience(memory, scene);
    memory.topic = memory.reply = 0;
    return question(memory, words, false);
}
AX_FN void score_field(FieldStudent &r, Narration &vocabulary, Episode scene,const QuestionBook &book, Word language) {
    auto final_lesson = vocabulary.event;
    if (r.forms) {
        Episode diagnostic{};
        diagnostic.stamp = 1;
        Word before = 0, after = 0, rotation = mix(language ^ 0x55d2337u);
        for (Word j = 0; j < 4; ++j) {
            before |= ((j + rotation) & 3) << (2 * j);
            after |= ((j + rotation + 1 + 2 * ((rotation >> 7) & 1)) & 3) << (2 * j);
        }
        expose(diagnostic, beginning, before, 15);
        expose(diagnostic, result, after, 15);
        for (Word frame = 0; frame < 2; ++frame) {
            for (Word j = 0; j < 4; ++j) {
                auto question_words = fill_question(book.full[frame], book.slot[frame], book.names.wave[j]);
                Word answer = spoken(field_question(vocabulary, diagnostic, question_words), book.names);
                if (answer < 8) {
                    Word index = 4 * frame + j;
                    r.diagnostic_mask |= 1u << index;
                    r.diagnostic_codes |= answer << (3 * index);
                }
                r.diagnostic += answer == 4 + pigment(picture(diagnostic, frame), j);
            }
        }
    }
    Word success[2]{};
    for (Word index = 0; index < 2; ++index) {
        Word frame = index ? result : beginning;
        Word board = picture(scene, frame), known = visible(scene, frame);
        for (Word site = 0; site < 4; ++site) {
            if (!(known & (1u << site)))
                continue;
            ++r.offered;
            if (!r.forms)
                continue;
            auto words = fill_question(book.full[frame], book.slot[frame], book.names.wave[site]);
            Word expected = 4 + pigment(board, site);
            r.class_correct += spoken(field_question(vocabulary, final_lesson, words), book.names) == expected;
            r.empty_answers += field_question(vocabulary, {}, words).answered;
            auto response = field_question(vocabulary, scene, words);
            Word answer = spoken(response, book.names);
            if (response.answered) {
                ++r.answered;
                if (answer < 8) {
                    Word j = 4 * index + site;
                    r.replies |= answer << (3 * j);
                    r.answer_mask |= 1u << j;
                }
            }
            if (answer == expected) {
                ++r.correct;
                success[index] |= 1u << site;
            }
        }
    }
    for (Word site = 0; site < 4; ++site) {
        Word bit = 1u << site;
        if ((visible(scene, beginning) & visible(scene, result) & bit) &&
            pigment(picture(scene, beginning), site) != pigment(picture(scene, result), site)) {
            ++r.changed;
            r.both += !!(success[0] & success[1] & bit);
        }
    }
}
AX_FN FieldStudent examine_field(Mind individual, Episode scene, Word key, Word language, Word practice) {
    if (practice)
        drill(individual, 0, practice, key + 61931, learned);
    Narration vocabulary{};
    FieldStudent r{};
    const auto book = question_book(language);
    acquire_names(vocabulary, individual.genes, book.names, 12, key);
    Word named = 0;
    for (Word j = 0; j < 8; ++j) {
        Word meaning = ident(vocabulary, intern(vocabulary, book.names.wave[j], false));
        named += meaning != none;
        r.names += meaning == j;
    }
    if (named && (individual.genes.arbor & (1u << 15))) {
        Word cost = 0;
        for (Word index = 0; index < 2; ++index) {
            Word frame = index ? result : beginning;
            acquire_form(vocabulary, individual, book.names, book.full[frame], book.slot[frame], frame, 0, 0, 64,
                         key + 3779 * (index + 1), 0, cost);
        }
        for (auto f : vocabulary.forms)
            r.forms += f.examples != 0 && !braid::sequence_form(f);
    }
    score_field(r,vocabulary,scene,book,language);
    return r;
}
KERNEL void braid_field_class(const Mind *population, const Episode *field, const Word *slots, Word count, Word seed,
                              Word practice, Word retest, Word prescribed, Word history, FieldStudent *output) {
    Word i = tid();
    if (i >= count)
        return;
    Word slot = slots[i];
    Word language = retest ? mix(seed + i * 7919) : mix(slot ^ seed);
    output[i] = examine_field(population[slot], field[slot], prescribed ? history : language, language, practice);
}
// Characterize the finite contact tables on independent demonstrations. This
// calibration runs in isolated neural copies before observing the habitat.
KERNEL void braid_class_laws(Word seed, Word *matches) {
    Word law = tid();
    if (law >= 16)
        return;
    Word passing = 0;
    for (Word j = 0; j < 4; ++j) {
        Word key = mix(seed + j * 3571);
        Genes observer = founder(key);
        observer.arbor |= 1u << 14;
        observer.plastic = (observer.plastic & ~0xf00u) | (law << 8);
        Narration memory{};
        const auto names = question_book(key).names;
        acquire_names(memory, observer, names, 12, key);
        Word correct = 0;
        for (Word k = 0; k < 8; ++k)
            correct += ident(memory, intern(memory, names.wave[k], false)) == k;
        passing += correct == 8;
    }
    matches[law] = passing;
}
KERNEL void braid_watch_class(const Mind *population, const flood::Colony *bodies, const Episode *field, Word count,
                              const Word *laws, Word *bitmap, Word *seen) {
    Word i = tid(), passing = 0, mask;
    if (i < count && bodies[i].energy && bodies[i].body.eyes && field[i].stamp) {
        auto g = population[i].genes;
        passing = (g.arbor & (3u << 14)) == (3u << 14) && laws[(g.plastic >> 8) & 15];
    }
    asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,0xffffffff; }"
                 : "=r"(mask)
                 : "r"(passing));
    if (!(i & 31) && i < count) {
        bitmap[i / 32] = mask;
        if (mask)
            asm volatile("red.global.or.b32 [%0],1;" ::"l"(seen) : "memory");
    }
}
} // namespace axion::life::braid

namespace axion::life::braid {
KERNEL void braid_listen_field(Narration *memory, const Episode *observed, Word slot, Speech speech, Answer *out) {
    if (tid())
        return;
    auto scene = observed[slot];
    auto &heard = *memory;
    if (scene.stamp &&
        (heard.event.stamp != scene.stamp || heard.event.images != scene.images || heard.event.known != scene.known))
        experience(heard, scene);
    *out = question(heard, speech, false);
}
} // namespace axion::life::braid
