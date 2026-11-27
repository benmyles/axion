#pragma once
#include "lesson.hpp"

namespace axion {

struct Doors { Word arm[2]; };
struct Place {
    SpikeCode key;
    Doors exits;
};
struct PlaceMemory {
    Place places[32];
    Word used;
};
static_assert(sizeof(PlaceMemory) == 516);

AX_FN int best_match(const SpikeCode &key, const Place &place, Word slot) {
    // A packed score deterministically favors the earlier assembly on a tie.
    int sensory = 64 - 2 * population(key.lo ^ place.key.lo);
    int score = (3 * sensory + overlap(key, place.key)) / 4;
    return 32 * score + 31 - int(slot);
}

AX_FN int match_slot(int winner, bool learning) {
    return winner >= 32 * (learning ? 58 : 48) ? 31 - (winner & 31) : -1;
}

// Each learned transition connects two place assemblies, gated by a motor
// action. Reverse waves over observed transitions give the first action of a
// shortest known route to the currently cued goal.
template<class Memory>
AX_FN int reach_action(const Memory &memory, Doors exits, Word active, int depth, Word draw) {
    if (!active || !(exits.arm[0] | exits.arm[1])) return -1;
    for (int wave = 0; wave < depth; ++wave) {
        bool left = exits.arm[0] & active;
        bool right = exits.arm[1] & active;
        if (left || right) return left == right ? int(draw & 1) : int(right);
        if (wave + 1 == depth) break;
        Word expanded = active | memory.predecessors(active);
        if (expanded == active) break;
        active = expanded;
    }
    return -1;
}

// The simulator alone owns the maze. A hidden cycle guarantees reachability.
// Additional directed doors and their left/right positions vary between maps.
struct Maze {
    Word wiring, vocabulary;
    AX_FN static Maze make(Word lesson, Word who) {
        Word wiring = map_seed(lesson, who);
        return {wiring, mix(wiring ^ 0x4a352c91u)};
    }
    AX_FN Word step(Word place, int arm) const {
        Word local = mix(wiring ^ place);
        Word stride = arm == int(local & 1) ? 1 : 2 + ((local >> 1) & 7);
        return (place + stride) & 15;
    }
    AX_FN Word expand(Word places) const {
        Word frontier = places;
        while (frontier) {
            Word i = unsigned(__builtin_ctz(frontier));
            frontier &= frontier - 1;
            places |= (1u << step(i, 0)) | (1u << step(i, 1));
        }
        return places;
    }
    AX_FN Word far_goal(Word start, Word draw) const {
        Word adjacent = expand(expand(1u << start));
        Word far = expand(expand(expand(adjacent))) & ~adjacent;
        // Exactly the rooms at shortest distance 3..5. The cycle guarantees
        // at least one and a two-step ball has at most seven rooms.
        int rank = int(draw % unsigned(population(far)));
        for (int i = 0; i < rank; ++i) far &= far - 1;
        return unsigned(__builtin_ctz(far));
    }
};

template<class Network>
AX_FN SpikeCode observe_place(Network &net, Word receptors, Word noise) {
    Word first = receptors, second = receptors, third = receptors;
    if (noise & 1) first ^= 3u << (2 * ((noise >> 1) & 15));
    if (noise & 32) second ^= 3u << (2 * ((noise >> 6) & 15));
    if (noise & 1024) third ^= 3u << (2 * ((noise >> 11) & 15));
    return view_item(net, first, second, third);
}

struct Destination {
    Word first, second;
    int count;

    AX_FN static Destination draw(const Maze &maze, Word room, Word key, bool distant, bool ordered) {
        Word a = distant ? maze.far_goal(room, mix(key)) : maze.step(room, int((key >> 9) & 1));
        return {a, ordered ? maze.far_goal(a, mix(key ^ 0xdab11265u)) : 0, ordered ? 2 : 1};
    }
};

// Visits are scored by the world. Only receptor spikes and the chosen action
// cross into the learner. Physical progress and recalled progress are separate.
template<Learning L, class Network, class Memory, class Lexicon>
AX_FN Counts navigate(Network &net, Memory &memory, Lexicon &words, Course c,
                      Word animal, Word first, Word episodes) {
    Word who = c.common_trials ? 0 : animal;
    Maze maze = Maze::make(c.lesson, who);
    Counts out{};
    for (Word e = 0; e < episodes; ++e) {
        Word i = first + e;
        bool command = c.task == Task::errand && c.novel;
        if (c.task == Task::errand && !c.novel) {
            // One word-grounding experience followed by two independent
            // single-door experiences. No commanded trip occurs in support.
            Word group = i / 3, offset = i % 3;
            if (!offset) {
                Course naming = c;
                naming.task = Task::landmarks;
                Counts label = c.recall == Recall::cortical
                    ? study_cortex<L>(net, naming, animal, group, 1)
                    : study<L>(net, words, naming, animal, group, 1);
                out.wins += label.wins;
                out.ticks += label.ticks;
                continue;
            }
            i = 2 * group + offset - 1;
        }
        Word key = trial_key(c.seed, who, i);
        Word order = trial_key(c.seed, who, i / 16);
        Word room = c.novel ? (key & 15) : (((i & 15) * (order | 1) + (order >> 8)) & 15);
        bool survey = !c.novel && c.survey_steps;
        Destination dests = Destination::draw(maze, room, key, c.novel || survey, command);
        net.reset(key);
        bool use_map = c.recall != Recall::cortical;
        int target[2] = {-1, -1};
        for (int j = 0; j < dests.count; ++j) {
            Word place = (j ^ int(command && c.swap_words)) ? dests.second : dests.first;
            Word stimulus = command ? name_id(maze.wiring, place) : place;
            Word cue = c.mute_odor ? 0 : symbol(maze.vocabulary, stimulus);
            auto code = observe_place(net, cue, mix(key + 1 + j * 71));
            if (use_map) {
                // A cued association can supply a place assembly even when the
                // presented symbol itself has never occurred as a landmark.
                int named = memory.identify(words.translate(code), false);
                target[j] = named >= 0 ? named : memory.identify(code, false);
            }
        }
        Word observed = c.mute_odor ? 0 : symbol(maze.vocabulary, room);
        auto code = observe_place(net, observed, mix(key + 2));
        int source = use_map ? memory.identify(code, L != Learning::frozen) : -1;
        Policy<L> policy{};
        int steps = 0, reached = 0, recalled = 0;
        Word doors_taken = 0;
        int limit = survey ? int(c.survey_steps) : (c.novel ? 6 * dests.count : 1);
        for (; steps < limit && (survey || reached < dests.count); ++steps) {
            Word draw = mix(key ^ (0x98f5319du * (steps + 1)));
            Values q = normalize(net.read());
            int action = q.left == q.right ? int(draw & 1) : int(q.right > q.left);
            if constexpr (L != Learning::frozen)
                if ((draw & 14) == 0) action = int((draw >> 8) & 1);
            if (source >= 0) {
                Doors exits = memory.doors(source);
                if (command && source == target[0]) recalled = 1;
                int goal = recalled ? target[1] : target[0];
                Word targets = goal < 0 ? 0 : 1u << goal;
                if (survey) targets = memory.frontier();
                int depth = c.recall == Recall::content ? 1 : (survey ? 16 : 6);
                int planned = reach_action(memory, exits, targets, depth, draw);
                if (planned >= 0) action = planned;
                if constexpr (L != Learning::frozen) {
                    if (!exits.arm[0] || !exits.arm[1])
                        action = exits.arm[0] ? 1 : (exits.arm[1] ? 0 : int(draw & 1));
                }
            }
            if (survey && (c.random_walk || !use_map)) action = int(draw & 1);
            policy.commit(net, q, action, use_map ? int(c.consolidation) : 1);
            if constexpr (L == Learning::frozen)
                if (c.novel) doors_taken |= Word(action) << steps;
            room = maze.step(room, action ^ int(c.reversed));
            if (reached < dests.count && room == (reached ? dests.second : dests.first)) ++reached;
            observed = c.mute_odor ? 0 : symbol(maze.vocabulary, room);
            code = observe_place(net, observed, mix(draw ^ 0xda42bf11u));
            int next = use_map ? memory.identify(code, L != Learning::frozen) : -1;
            if constexpr (L != Learning::frozen)
                if (source >= 0 && next >= 0) memory.link(source, action, next);
            source = next;
        }
        bool won = reached == dests.count;
        policy.outcome(net, observed_reward(c, key, won), use_map ? int(c.consolidation) : 1);
        out.wins += unsigned(won);
        out.image = doors_taken;
        out.ticks += unsigned((steps + 1 + dests.count) * 3);
    }
    out.spikes = net.spike_count();
    return out;
}

} // namespace axion
