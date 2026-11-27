#include "axion/braid_parallax.hpp"
namespace axion::life::braid {
AX_FN Optics unfamiliar_lens(Word key) {
    return {static_cast<unsigned char>(assembly(mix(key + 1063) % 24)),
            static_cast<unsigned char>(assembly(mix(key + 3049) % 24))};
}
AX_FN void compare_community(Narration &person, Narration &witness, Mind &worker, Optics lens, Count channel, Word key,
                             Word count, Word rig, Word budget, Word arm, EyeSample *trace) {
    for (Word i = 0; i < count; ++i) {
        auto scene = demonstration(worker, key, rig, arm == unchanging_panel ? 0 : i);
        auto other = arm == unpaired_views ? demonstration(worker, key + 7187, rig, i) : scene;
        experience(person, scene.self);
        experience(witness, through_glass(other.peer, lens));
        // At the first shared panel the partners can name all four sockets.
        // Later comparisons have a separate, limited hearing budget.
        auto seen =
            compare_sight(person, witness, worker.genes, channel, beginning, i ? budget : 4, mix(key + i * 1439),
                          i && arm == same_socket ? 2
                          : arm == random_sockets ? 1
                                                  : 0);
        trace[i] = {
            static_cast<unsigned char>(scene.fact[0]), static_cast<unsigned char>(picture(witness.event, beginning)),
            static_cast<unsigned char>(seen.ports),    static_cast<unsigned char>(seen.heard_ports),
            static_cast<unsigned char>(seen.remote),   static_cast<unsigned char>(seen.first),
            static_cast<unsigned char>(seen.order)};
    }
}
KERNEL void braid_parallax(const Genes *stock, Word size, Word cases, Word seed, Word repeats, Word panels, Word budget,
                           Word arm, ViewCase *results) {
    Word i = tid();
    if (i >= cases)
        return;
    Word key = mix(seed + i * 7523);
    auto language = question_book(key);
    Narration people[3]{};
    Mind bodies[3]{};
    Word cost = 0, rig = new_rig(key, 1);
    for (Word j = 0; j < 3; ++j) {
        bodies[j].genes = stock[mix(key + 179 * j) % size];
        educate(people[j], bodies[j], language, repeats, key + 19411 * j, j ? new_rig(key + 73 * j, 1) : rig, cost, 3);
        people[j].topic = people[j].reply = 0;
    }
    Count signs[]{Count(mix(key + 113)) << 32 | 1u, Count(mix(key + 113)) << 32 | 3u};
    ViewCase result{};
    result.key = key;
    result.learning = (bodies[0].genes.plastic >> 8) & 15;
    result.arbor = bodies[0].genes.arbor;
    result.throat = bodies[0].genes.throat;
    result.rig = rig;
    result.wrist = bodies[0].genes.muscle & 7;
    result.mode = (key >> 15) & 1;
    for (Word j = 0; j < 2; ++j) {
        auto &a = people[0], &b = people[1 + j];
        Optics initial = unfamiliar_lens(key + 2311 * j);
        auto changed = unfamiliar_lens(key + 13739 + 2311 * j);
        result.glass[j] = arm == turned_lens || arm == readapted ? changed : initial;
        if (arm != common_names) {
            compare_community(a, b, bodies[0], initial, signs[j], key + 949 * j, panels, rig, budget, arm,
                              result.panels[j]);
            result.visits = panels;
        }
        Word p = acquaintance(a, signs[j], false);
        if (p != none)
            result.before_turn[j] = __builtin_popcount(candidates(contact(a, p)));
        if (arm == readapted) {
            compare_community(a, b, bodies[0], changed, signs[j], key + 949 * j + 3361, panels, rig, budget, arm,
                              result.panels[j] + panels);
            result.visits = 2 * panels;
        }
        p = acquaintance(a, signs[j], false);
        if (p != none) {
            auto c = contact(a, p);
            result.hypotheses[j] = candidates(c);
            result.anchors[j] = c.panel;
            result.survivors[j] = __builtin_popcount(result.hypotheses[j]);
        }
    }
    auto test = demonstration(bodies[0], key + 9857, rig, 13);
    result.board = test.fact[0];
    result.motor = test.motion.stream;
    result.length = test.motion.size;
    for (Word twin = 0; twin < 2; ++twin) {
        auto scene = encounter(bodies[0], key, rig, twin ? 0 : 15, result.board, test.motion);
        result.ends[twin] = scene.fact[braid::result];
        for (Word noun = 0; noun < 8; ++noun) {
            auto initiator = people[0];
            initiator.topic = initiator.reply = 0;
            glimpse(initiator, scene.self, 13);
            auto first = fill_question(language.full[beginning], language.slot[beginning], language.names.wave[noun]);
            auto before = question(initiator, first, false);
            auto follow = result.mode ? language.reply : language.topic;
            auto inquiry = question(initiator, follow, true);
            Count registered[]{signs[arm == wrong_witness], signs[arm != wrong_witness]};
            result.pending[twin][noun] = inquiry.queried ? inquiry.source : none;
            result.eligible[twin][noun] = inquiry.queried ? inquiry.options : 0;
            auto choice = choose_witness(initiator, bodies[0].genes, inquiry, registered[0], registered[1]);
            result.selected[twin][noun] = choice.selected;
            for (Word k = 0; k < 2; ++k)
                result.coverage[twin][noun][k] = choice.coverage[k];
            // The first copy makes the one-call decision before either witness
            // speaks. Two separate copies measure the fixed-recipient controls.
            for (Word round = 0; round < 3; ++round) {
                Word source = round ? round - 1 : choice.selected;
                auto self = initiator, peer = people[source + 1];
                peer.topic = peer.reply = 0;
                glimpse(peer, through_glass(scene.peer, result.glass[source]), 3);
                auto answer =
                    consult_view(self, peer, follow, inquiry, signs[arm == wrong_witness ? (source ^ 1) : source],
                                 arm == covered_witness ? 2 : 1, true,
                                 arm == send_only      ? 1
                                 : arm == receive_only ? 2
                                                       : 3);
                auto &q = round ? result.turns[source][twin][noun] : result.public_turn[twin][noun];
                q.opening = spoken(before, language.names);
                if (answer.original.queried) {
                    for (Word k = 0; k < answer.original.words.n; ++k) {
                        Word v = index_of(language.names, answer.original.words.wave[k]);
                        if (v != none)
                            q.request = v;
                    }
                }
                q.sound = spoken(answer.witness, language.names);
                q.answer = spoken(answer.final.answered ? answer.final : answer.original, language.names);
                if (q.answer != none) {
                    auto reverse = fill_question(language.full[braid::result], language.slot[braid::result],
                                                 language.names.wave[q.answer]);
                    q.remembered = spoken(question(self, reverse, false), language.names);
                }
            }
        }
    }
    results[i] = result;
}
} // namespace axion::life::braid
