#pragma once
#include "mind_core.cuh"

namespace axion::mind::gpu {

// ---------------------------------------------------------------------------
// Native Blackwell SM 12.0 16x16 Entorhinal-Hippocampal 2D Place-Cell Map!
// Grid: 1 block x 256 threads (1 thread per 2D place cell `(x, y)`!)
// Goal cell injects excitatory burst at t = 0; blocked cells (rivers, cliffs,
// broken bridges) receive continuous inhibitory veto. Over parallel gamma-wave
// ticks, spiking wavefront propagates laterally across 4-connected place cells
// until start cell fires; Thread 0 traces the minimum-latency spike gradient!
// ---------------------------------------------------------------------------
AX_FN void spatial_wave_block(const SpatialMapQuery &q, SpatialPathResult &out) {
    __attribute__((shared)) static Word s_tick[MAP_CELLS];
    __attribute__((shared)) static Word s_par[MAP_CELLS];
    __attribute__((shared)) static Word s_fired_now[MAP_CELLS];
    __attribute__((shared)) static Word s_spikes;
    __attribute__((shared)) static Word s_done;
    __attribute__((shared)) static Word s_wave_ticks;

    Word tid = thread_id();
    if (tid >= MAP_CELLS)
        return;

    Word start_c = q.start_y * MAP_W + q.start_x;
    Word goal_c = q.goal_y * MAP_W + q.goal_x;
    bool blocked = ((q.blocked[tid >> 5] >> (tid & 31u)) & 1u) != 0;
    if (tid == start_c || tid == goal_c)
        blocked = false;

    s_tick[tid] = (tid == goal_c) ? 1u : 0u;
    s_par[tid] = tid;
    s_fired_now[tid] = (tid == goal_c) ? 1u : 0u;
    if (tid == 0) {
        s_spikes = 1u;
        s_done = (start_c == goal_c) ? 1u : 0u;
        s_wave_ticks = 0u;
    }
    asm volatile("bar.sync 0;");

    Word x = tid & 15u;
    Word y = tid >> 4;

    // Parallel spiking wavefront propagation across all 256 place cells!
    for (Word step = 2; step <= 64u; ++step) {
        if (s_done)
            break;

        Word fire = 0u;
        Word best_nb = tid;
        if (!blocked && s_tick[tid] == 0u) {
            // Check 4 cardinal place-cell neighbors for incoming lateral spikes!
            if (x > 0 && s_tick[tid - 1u] == step - 1u) { fire = 1u; best_nb = tid - 1u; }
            else if (x + 1u < MAP_W && s_tick[tid + 1u] == step - 1u) { fire = 1u; best_nb = tid + 1u; }
            else if (y > 0 && s_tick[tid - MAP_W] == step - 1u) { fire = 1u; best_nb = tid - MAP_W; }
            else if (y + 1u < MAP_H && s_tick[tid + MAP_W] == step - 1u) { fire = 1u; best_nb = tid + MAP_W; }
        }
        s_fired_now[tid] = fire;
        asm volatile("bar.sync 0;");

        if (fire) {
            s_tick[tid] = step;
            s_par[tid] = best_nb;
            if (tid == start_c)
                s_done = 1u;
        }
        if (tid == 0) {
            Word cnt = 0u;
            for (Word i = 0; i < MAP_CELLS; ++i)
                cnt += s_fired_now[i];
            s_spikes += cnt;
            s_wave_ticks = step - 1u;
        }
        asm volatile("bar.sync 0;");
    }

    // Thread 0 reconstructs the exact optimal obstacle-avoiding trajectory!
    if (tid == 0) {
        out = {};
        out.spikes_fired = s_spikes;
        out.wave_ticks = s_wave_ticks;
        if (!s_done || s_tick[start_c] == 0u)
            return;
        out.found = 1u;
        Word cur = start_c;
        Word len = 0u;
        while (len < 64u) {
            out.path_cells[len++] = cur;
            if (cur == goal_c)
                break;
            Word nxt = s_par[cur];
            if (nxt == cur)
                break;
            cur = nxt;
        }
        out.length = len;
    }
}

} // namespace axion::mind::gpu
