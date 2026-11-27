#pragma once
#include "routes.hpp"
#include "detector.hpp"

namespace axion {

AX_FN Word odor_bits(int identity) {
    // Four overlapping odor mixtures share receptors. The mapping from
    // mixtures to food locations is available only in the arena.
    constexpr Word odor[4] = {0x1313, 0x2626, 0x4c4c, 0x8989};
    return odor[identity];
}

AX_FN int food_arm(int odor) {
    return (6 >> odor) & 1;
}

// The animal walks a corridor. At a junction it chooses an arm. In Trail the
// first arm determines which patch is encountered at the second junction.
struct Arena {
    struct {
        Task task;
        Word reversed, mute_odor;
    } course;
    Word layout, noise;
    int odor, leg_start, fork, finish, gap2, transit;
    bool second_leg, on_path;

    AX_FN static Arena enter(const Course &course, Word key) {
        Arena world{};
        Task task = course.task == Task::mixed ? Task(mix(key ^ 0x51bcad73u) & 3) : course.task;
        world.course = {task, course.reversed, course.mute_odor};
        world.layout = mix(key ^ 0x99c62d05u);
        world.noise = mix(key ^ 0x197b4503u) | 1;
        world.odor = world.layout & 3;
        int gap = 0;
        if (task == Task::delay || task == Task::context)
            gap = 4 + int((world.layout >> 4) & 7);
        if (task == Task::trail)
            gap = int((world.layout >> 4) & 3);
        world.gap2 = int((world.layout >> 8) & 3);
        world.transit = 2 + int((world.layout >> 12) & 3);
        world.fork = 2 + gap;
        world.finish = world.fork + world.transit;
        world.on_path = true;
        return world;
    }

    AX_FN Word sense(int tick) {
        // Nuisance and receptor dropout follow an action-independent stream.
        noise ^= noise << 13;
        noise ^= noise >> 17;
        noise ^= noise << 5;
        Word bits = (noise & 3) ? (1u << (24 + ((noise >> 2) & 7))) : 0;
        if (tick - leg_start < 3) {
            if (!course.mute_odor)
                bits |= odor_bits(odor) & ~(1u << ((noise >> 8) & 15));
            if (course.task == Task::context)
                bits |= (layout & 4) ? 0x00380000u : 0x00070000u;
        }
        if (tick == fork) bits |= 1u << 23;
        return bits;
    }

    AX_FN void take(int tick, int arm) {
        int context = course.task == Task::context && (layout & 4);
        on_path = on_path && (arm == (food_arm(odor) ^ context ^ int(course.reversed)));
        if (course.task == Task::trail && !second_leg) {
            odor = (int((layout >> 16) & 3) + arm) & 3;
            second_leg = true;
            leg_start = tick + 1;
            fork = leg_start + 2 + gap2;
            finish = fork + transit;
        }
    }
};

template<Learning L, class Network>
AX_FN Counts simulate(Network &net, const Course &course, Word animal, Word first, Word episodes) {
    Counts counts{};
    for (Word e = 0; e < episodes; ++e) {
        Word key = trial_key(course.seed, course.common_trials ? 0 : animal, first + e);
        auto arena = Arena::enter(course, key);
        net.reset(key);
        Policy<L> policy{};
        int tick = 0;
        for (; tick <= arena.finish; ++tick) {
            net.tick(arena.sense(tick));
            if (tick != arena.fork) continue;
            Word draw = mix(key ^ (policy.committed ? 0x46af13e1u : 0x73cb541bu));
            arena.take(tick, policy.choose(net, draw));
        }
        policy.outcome(net, arena.on_path ? 256 : -256);
        counts.wins += unsigned(arena.on_path);
        counts.ticks += unsigned(tick);
    }
    counts.spikes = net.spike_count();
    return counts;
}

} // namespace axion
