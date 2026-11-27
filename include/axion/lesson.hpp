#pragma once
#include "archive.hpp"

namespace axion {

enum class Task : Word {
    scent, delay, context, trail, mixed, naming, sorting, pattern, routes, landmarks, errand,
    workshop, invention, seminar, detector, signals, canvas
};
AX_FN bool is_drawing(Task task) { return task == Task::workshop || task == Task::invention || task == Task::canvas; }
AX_FN bool is_lesson(Task task) { return unsigned(task) >= unsigned(Task::naming); }

struct Course {
    Task task;
    Word seed;
    Word reversed;
    Word mute_odor;
    Word common_trials = 0;
    Word lesson = 0;
    Word novel = 0;
    Recall recall = Recall::associative;
    Word scrambled = 0;
    Word swap_words = 0;
    Word survey_steps = 0, random_walk = 0;
    Word chain = 3, difficult = 0, inverse = 0;
    Word plan_nodes = 4096;
    Word consolidation = 64;
    Word vocabulary = 8, archive_slots = 0;
    enum class Experiments : Word { off, random, paired, curious };
    Experiments experiments = Experiments::off;
    struct {
        Word objects[5]{};
        Word outcome = 0;
        int action = -1;
    } signals{};
};

// A vocabulary is a permutation of 16-bit symbols. Each bit fires one of two
// opponent receptors. Every clean item therefore contains exactly 16 spikes.
AX_FN Word symbol(Word vocabulary, Word id) {
    Word x = (id + vocabulary) & 65535;
    x ^= x >> 7;
    x = (x * 40503u) & 65535;
    x ^= x >> 9;
    x = (x * ((vocabulary >> 16) | 1u)) & 65535;
    x ^= x >> 8;
    x = (x | (x << 8)) & 0x00ff00ffu;
    x = (x | (x << 4)) & 0x0f0f0f0fu;
    x = (x | (x << 2)) & 0x33333333u;
    x = (x | (x << 1)) & 0x55555555u;
    return x | ((x ^ 0x55555555u) << 1);
}

struct Scene {
    static constexpr int slots = 5, ticks_per_slot = 3;
    Word objects[slots];
    Word noise;
    AX_FN Word pulse(int slot, int sample) const {
        Word draw = mix(noise ^ (Word(slot * ticks_per_slot + sample) * 0xb5297a4du));
        Word perturb = (draw & 1) ? 3u << (2 * ((draw >> 1) & 15)) : 0u;
        return objects[slot] ^ perturb;
    }
};

struct Example {
    Scene visible;
    int rewarded;
};

AX_FN Word map_seed(Word lesson, Word who) {
    return trial_key(lesson, who, 0xbed206c5u);
}
AX_FN Word name_id(Word wiring, Word room) {
    return 4096 + ((room * ((wiring >> 16) | 1u) + (wiring >> 8)) & 15);
}

AX_FN void insert_cue(Word (&ids)[5], Word position, Word value) {
    // Fixed locations allow the three context words to stay in registers.
    if (position == 0) ids[0] = value;
    if (position == 1) ids[1] = value;
    if (position == 2) ids[2] = value;
}

AX_FN Example example(const Course &c, Word animal, Word index) {
    if (c.task == Task::signals) {
        Example ex{};
        for (int j = 0; j < Scene::slots; ++j) ex.visible.objects[j] = c.signals.objects[j];
        ex.visible.noise = trial_key(c.seed, animal, index);
        ex.rewarded = int(c.signals.outcome);
        return ex;
    }
    Word who = c.common_trials ? 0 : animal;
    Task subject = c.task;
    if (subject == Task::seminar) {
        // The hidden subject only changes the world generator. All three use
        // the same five receptor frames, motor outputs and learning rule.
        Word group = index / 3;
        Word shuffle = trial_key(c.seed, who, group);
        Word choice = (index % 3 * (1 + (shuffle & 1)) + ((shuffle >> 1) % 3)) % 3;
        subject = Task(Word(Task::naming) + choice);
        index = group;
    }
    Word rule = trial_key(c.lesson, who, 0x07ca91efu);
    Word vocabulary = mix(rule ^ 0xe225ed7du);
    Word stimulus_seed = c.seed;
    if (c.task == Task::seminar) stimulus_seed ^= mix(Word(subject) + 0xf671u);
    Word draw = trial_key(stimulus_seed ^ (c.novel ? 0xbc192eabu : 0u), who, index);
    // Each consecutive support block covers every identity in permuted order.
    Word order = mix(trial_key(c.seed, who, index / 8) ^ 0xa8caaf37u);
    Word rank = ((index & 7) * (order | 1u) + (order >> 8)) & 7;
    Word item = c.novel ? (mix(draw) & 7) : rank;
    Word ids[Scene::slots];
    Word nuisance = (c.novel ? 16384u : 8192u) + (mix(draw ^ 31) & 2047);
    for (int j = 0; j < 3; ++j) ids[j] = nuisance + 2048u * j;
    Word target = 0, alternative = 1;
    if (subject == Task::landmarks) {
        Word wiring = map_seed(c.lesson, who);
        vocabulary = mix(wiring ^ 0x4a352c91u);
        Word order16 = trial_key(c.seed, who, index / 16);
        target = c.novel ? (draw & 15) : (((index & 15) * (order16 | 1) + (order16 >> 8)) & 15);
        insert_cue(ids, (wiring >> 19) % 3, name_id(wiring, target));
        alternative = (target + (c.novel ? 8 + ((draw >> 5) & 7) : 1 + ((draw >> 5) % 7))) & 15;
    } else if (subject == Task::naming) {
        // A new lexicon per lesson. Context distractors and the foil change.
        Word n = c.vocabulary, mask = n - 1;
        if (n > 8) {
            Word schedule = trial_key(c.seed, who, index / n);
            item = c.novel ? mix(draw) & mask : ((index & mask) * (schedule | 1) + (schedule >> 12)) & mask;
        }
        Word key_base = n == 8 ? 32 : 0;
        Word value_base = n == 8 ? 64 : 4096;
        insert_cue(ids, (rule >> 6) % 3, key_base + item);
        target = value_base + ((item * (rule | 1u) + (rule >> 8)) & mask);
        Word offset = c.novel ? n / 2 + ((draw >> 4) & (mask / 2)) : 1 + ((draw >> 4) % (n / 2 - 1));
        alternative = value_base + ((target + offset) & mask);
    } else if (subject == Task::sorting) {
        Word x = item & 3;
        Word offset = c.novel ? 2 + ((draw >> 5) & 1) : item >> 2;
        Word y = (x + offset) & 3;
        ids[0] = 96 + x;
        ids[1] = 112 + y;
        Word relevant = (rule & 1) ? x : y;
        // A balanced partition of the four values into two categories.
        constexpr Word partitions[] = {3, 5, 6, 9, 10, 12};
        target = (partitions[(rule >> 8) % 6] >> relevant) & 1;
        alternative = target ^ 1;
    } else {
        // ABC, AAB, ABA, ABB. Phase-specific pools make the two alphabets
        // disjoint. The learner only receives the three noisy sensory items.
        Word form = item & 3;
        Word base = c.novel ? 4096 : 256;
        Word a = (draw >> 4) & 255;
        Word b = (a + 1 + ((draw >> 12) % 254)) & 255;
        Word d = (b + 1) & 255;
        if (d == a) d = (d + 1) & 255;
        ids[0] = base + a;
        ids[1] = base + (form == 1 ? a : b);
        ids[2] = base + (form == 2 ? a : (form == 3 ? b : d));
        constexpr Word partitions[] = {3, 5, 6, 9, 10, 12};
        target = (partitions[(rule >> 12) % 6] >> form) & 1;
        alternative = target ^ 1;
    }
    int side = int(mix(draw ^ 0xd4825339u) & 1);
    ids[3] = side ? alternative : target;
    ids[4] = side ? target : alternative;
    Example result{};
    for (int j = 0; j < Scene::slots; ++j)
        result.visible.objects[j] = c.mute_odor ? 0 : symbol(vocabulary, ids[j]);
    result.visible.noise = mix(draw ^ 0xeaa1c991u);
    result.rewarded = side ^ int(c.reversed);
    return result;
}

AX_FN int observed_reward(const Course &c, Word key, bool correct) {
    if (c.scrambled) return (mix(key ^ 0xa16c2d7bu) & 1) ? 256 : -256;
    return correct ? 256 : -256;
}

template<class Network>
AX_FN SpikeCode view_item(Network &net, Word first, Word second, Word third) {
    net.tick(first);
    net.tick(second);
    net.tick(third);
    Word majority = (first & second) | (third & (first | second));
    return net.signature(majority);
}

template<Learning L, class Network>
AX_FN bool cortical_example(Network &net, const Course &c, Word animal, Word index) {
    Example ex = example(c, animal, index);
    Word key = mix(ex.visible.noise ^ 0x9921b5efu);
    net.reset(key);
    for (int j = 0; j < Scene::slots; ++j) {
        net.tick(ex.visible.pulse(j, 0));
        net.tick(ex.visible.pulse(j, 1));
        net.tick(ex.visible.pulse(j, 2));
    }
    Policy<L> policy{};
    int action = policy.choose(net, mix(key), c.signals.action);
    bool correct = action == ex.rewarded;
    policy.outcome(net, observed_reward(c, key, correct));
    return correct;
}

// The learner receives five successive sensory presentations and bandit
// feedback. All tasks share the encoder, the readout and this learning rule.
template<Learning L, class Network>
AX_FN Counts study_cortex(Network &net, const Course &course, Word animal, Word first, Word episodes) {
    Counts counts{};
    for (Word e = 0; e < episodes; ++e)
        counts.wins += cortical_example<L>(net, course, animal, first + e);
    counts.ticks = episodes * Scene::slots * Scene::ticks_per_slot;
    counts.spikes = net.spike_count();
    return counts;
}

template<Learning L, class Network, class Memory>
AX_FN Counts study(Network &net, Memory &memory, const Course &course, Word animal, Word first, Word episodes) {
    Counts counts{};
    for (Word e = 0; e < episodes; ++e) {
        Example ex = example(course, animal, first + e);
        Word key = mix(ex.visible.noise ^ 0x9921b5efu);
        net.reset(key);
        Engram seen{};
        for (int j = 0; j < Scene::slots; ++j) {
            seen.item[j] = view_item(net, ex.visible.pulse(j, 0), ex.visible.pulse(j, 1), ex.visible.pulse(j, 2));
        }
        seen.bind();
        int margin[channels]{};
        Votes votes = memory.read(seen, course.recall);
        int fast = memory.state.predict(votes, margin, course.recall);
        auto values = normalize(net.read());
        int advantage = values.right - values.left + fast;
        Word draw = mix(key);
        int action = advantage == 0 ? int(draw & 1) : advantage > 0;
        if constexpr (L != Learning::frozen) {
            if ((draw & 14) == 0) action = int((draw >> 8) & 1);
            if (course.signals.action >= 0) action = course.signals.action;
            net.commit(action, values.selected(action));
        }
        bool correct = action == ex.rewarded;
        if constexpr (L != Learning::frozen) {
            int reward = observed_reward(course, key, correct);
            net.reinforce(bound(reward - values.selected(action), -512, 512) / int(course.consolidation));
            int preferred = reward > 0 ? action : 1 - action;
            seen.prefer(preferred);
            memory.write(seen);
            memory.state.update(margin, preferred);
        }
        counts.wins += Word(correct);
    }
    counts.ticks = episodes * Scene::slots * Scene::ticks_per_slot;
    counts.spikes = net.spike_count();
    return counts;
}

} // namespace axion
