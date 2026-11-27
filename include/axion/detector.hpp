#pragma once
#include "lesson.hpp"

namespace axion {

// Digital analogue of Walker & Gopnik's second experiment. Object pairs
// share an activation rule. Demonstrations have no choice or motor reward.
struct Tray {
    Word a, b;
};
struct Detector {
    Word vocabulary;
    bool different;

    AX_FN Tray demonstration(Word trial) const {
        Word pair = (trial / 2) & 3;
        bool same = ((pair & 1) == 0) != different;
        Word round = pair / 2;
        return same ? Tray{round, round} : Tray{2 + 2 * round, 3 + 2 * round};
    }
    AX_FN bool music(Tray objects) const { return (objects.a == objects.b) != different; }
    AX_FN Tray novel(Word key, bool same) const {
        Word id = 16 + (key & 2047) * 3;
        return same ? Tray{id, id} : Tray{id + 1, id + 2};
    }
};

template<class Network>
AX_FN Engram see_tray(Network &net, const Detector &toy, Tray objects, Word key, bool blind) {
    Scene sample{};
    sample.noise = key;
    sample.objects[0] = blind ? 0 : symbol(toy.vocabulary, objects.a);
    sample.objects[1] = blind ? 0 : symbol(toy.vocabulary, objects.b);
    sample.objects[2] = symbol(toy.vocabulary, 63217); // the same apparatus on both trays
    Engram ex{};
    net.reset(key);
    for (int j = 0; j < 3; ++j)
        ex.item[j] = view_item(net, sample.pulse(j, 0), sample.pulse(j, 1), sample.pulse(j, 2));
    ex.bind();
    // An activation and an elapsed observation window with no activation
    // are discriminable sensory events. They have no object-identity bits.
    ex.item[3] = net.signature(0x55aa55aau);
    ex.item[4] = net.signature(0xaa55aa55u);
    return ex;
}

template<class Network, class Memory>
AX_FN int expected_effect(Network &net, Memory &memory, Recall recall, const Engram &scene,
                          int (&margin)[channels], int &slow) {
    slow = normalize(net.read()).right;
    if (recall == Recall::cortical) return slow;
    return slow + memory.state.predict(memory.read(scene, recall), margin, recall);
}

template<Learning L, class Network, class Memory>
AX_FN Counts detector_trials(Network &net, Memory &memory, Course c, Word animal, Word first, Word episodes) {
    Word who = c.common_trials ? 0 : animal;
    Word room = trial_key(c.lesson, who, 0x1ca34b7fu);
    Detector toy{mix(room), bool(c.reversed)};
    Counts out{};
    for (Word e = 0; e < episodes; ++e) {
        Word i = first + e, key = trial_key(c.seed, who, i);
        if (!c.novel) {
            auto pair = toy.demonstration(i);
            Engram scene = see_tray(net, toy, pair, key, c.mute_odor);
            int margins[channels]{}, slow;
            int expectation = expected_effect(net, memory, c.recall, scene, margins, slow);
            bool effect = toy.music(pair);
            if (c.scrambled) effect = mix(key ^ 0x5eaa4319u) & 1;
            if constexpr (L != Learning::frozen) net.commit(1, slow);
            net.tick(effect ? 0xaa55aa55u : 0x55aa55aau);
            if constexpr (L != Learning::frozen) {
                int error = bound((effect ? 256 : -256) - slow, -512, 512);
                net.reinforce(c.recall == Recall::cortical ? error : error / int(c.consolidation));
                if (c.recall != Recall::cortical) {
                    // Store only the percept actually observed at this trial.
                    scene.item[3] = effect ? scene.item[4] : scene.item[3];
                    scene.item[4] = {};
                    memory.write(scene);
                    memory.state.update(margins, int(effect));
                }
            }
            out.wins += unsigned((expectation > 0) == effect);
            out.ticks += 10;
        } else {
            bool left_same = mix(key ^ 0x6e68c783u) & 1;
            int score[2];
            for (int side = 0; side < 2; ++side) {
                auto pair = toy.novel(key, side ? !left_same : left_same);
                auto tray = see_tray(net, toy, pair, mix(key + side), c.mute_odor);
                int margins[channels]{}, slow;
                score[side] = expected_effect(net, memory, c.recall, tray, margins, slow);
            }
            bool right = score[1] == score[0] ? bool(mix(key ^ 0xea0d371bu) & 1) : score[1] > score[0];
            out.wins += unsigned(toy.music(toy.novel(key, right ? !left_same : left_same)));
            out.ticks += 18;
        }
    }
    out.spikes = net.spike_count();
    return out;
}
} // namespace axion
