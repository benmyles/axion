#pragma once
#include "axion/mind.hpp"

namespace axion::mind::gpu {

// ---------------------------------------------------------------------------
// Low-level Blackwell SM 12.0 Warp & Bitwise Spiking Primitives
// ---------------------------------------------------------------------------
AX_FN Word lane_id() {
    Word l;
    asm("mov.u32 %0, %%laneid;" : "=r"(l));
    return l;
}

AX_FN Word block_id() {
    Word b;
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(b));
    return b;
}

AX_FN Word thread_id() {
    Word t;
    asm("mov.u32 %0, %%tid.x;" : "=r"(t));
    return t;
}

AX_FN Word block_dim() {
    Word n;
    asm("mov.u32 %0, %%ntid.x;" : "=r"(n));
    return n;
}

AX_FN Word global_tid() { return block_id() * block_dim() + thread_id(); }

AX_FN Word warp_shfl(Word val, Word src_lane, Word mask = 0xffffffffu) {
    Word out;
    asm volatile("shfl.sync.idx.b32 %0, %1, %2, 31, %3;" : "=r"(out) : "r"(val), "r"(src_lane), "r"(mask));
    return out;
}

AX_FN Word warp_add(Word val, Word mask = 0xffffffffu) {
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 800
    Word sum;
    asm volatile("redux.sync.add.u32 %0, %1, %2;" : "=r"(sum) : "r"(val), "r"(mask));
    return sum;
#else
    for (int d = 16; d > 0; d >>= 1) {
        Word other;
        asm volatile("shfl.sync.xor.b32 %0, %1, %2, 31, %3;" : "=r"(other) : "r"(val), "r"(d), "r"(mask));
        val += other;
    }
    return val;
#endif
}

AX_FN int warp_max(int val, Word mask = 0xffffffffu) {
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 800
    int out;
    asm volatile("redux.sync.max.s32 %0, %1, %2;" : "=r"(out) : "r"(val), "r"(mask));
    return out;
#else
    for (int d = 16; d > 0; d >>= 1) {
        int other;
        asm volatile("shfl.sync.xor.b32 %0, %1, %2, 31, %3;" : "=r"(other) : "r"(val), "r"(d), "r"(mask));
        if (other > val)
            val = other;
    }
    return val;
#endif
}

AX_FN Word warp_ballot(bool pred, Word mask = 0xffffffffu) {
    Word out;
    asm volatile("{ .reg .pred p; setp.ne.u32 p, %1, 0; vote.sync.ballot.b32 %0, p, %2; }"
                 : "=r"(out)
                 : "r"(Word(pred)), "r"(mask));
    return out;
}

AX_FN Word rotl32(Word x, Word s) {
    s &= 31u;
    return s ? ((x << s) | (x >> (32u - s))) : x;
}

AX_FN Word rotr32(Word x, Word s) {
    s &= 31u;
    return s ? ((x >> s) | (x << (32u - s))) : x;
}

// ---------------------------------------------------------------------------
// Warp-Cooperative 1024-Neuron SDR Operations (1 lane = 32 neurons = 1 Word)
// ---------------------------------------------------------------------------
AX_FN Word warp_overlap(Word a_lane, Word b_lane) {
    return warp_add(Word(__builtin_popcount(a_lane & b_lane)));
}

AX_FN Word warp_popcount(Word a_lane) { return warp_add(Word(__builtin_popcount(a_lane))); }

// Invertible Theta-Gamma Phase Binding Permutation across 1024 neurons
// Preserves exact sparsity k and zero-loss invertibility!
AX_FN Word warp_permute_fwd(Word val, Word role_shift, Word lane) {
    Word word_shift = (role_shift * 7u + 3u) & 31u;
    Word bit_shift = (role_shift * 11u + 5u) & 31u;
    Word src = (lane - word_shift) & 31u;
    Word moved = warp_shfl(val, src);
    return rotl32(moved, bit_shift);
}

AX_FN Word warp_permute_inv(Word val, Word role_shift, Word lane) {
    Word word_shift = (role_shift * 7u + 3u) & 31u;
    Word bit_shift = (role_shift * 11u + 5u) & 31u;
    Word unrot = rotr32(val, bit_shift);
    Word src = (lane + word_shift) & 31u;
    return warp_shfl(unrot, src);
}

// ---------------------------------------------------------------------------
// True Parvalbumin (PV) Inhibitory Interneuron k-WTA Spike Selection
// Given 32 membrane potentials per lane (1024 neurons total across warp),
// computes exact inhibitory threshold T_inh via 9-step hardware redux.sync
// binary search so ~target_k pyramidal cells fire spikes!
// ---------------------------------------------------------------------------
AX_FN Word warp_kwta_select(const short v[32], Word target_k, Word lane) {
    int lo = 1, hi = 512;
    // Fast 9-iteration integer binary search using Blackwell redux.sync.add.u32
    for (int iter = 0; iter < 9; ++iter) {
        int mid = (lo + hi) >> 1;
        Word local_cnt = 0;
#pragma unroll
        for (int b = 0; b < 32; ++b)
            local_cnt += Word(v[b] >= mid);
        Word total = warp_add(local_cnt);
        if (total > target_k)
            lo = mid + 1;
        else
            hi = mid;
    }
    int thr = lo;
    Word strict_mask = 0, eq_mask = 0;
#pragma unroll
    for (int b = 0; b < 32; ++b) {
        if (v[b] >= thr)
            strict_mask |= 1u << b;
        else if (v[b] == thr - 1 && thr > 1)
            eq_mask |= 1u << b;
    }
    Word taken = warp_add(Word(__builtin_popcount(strict_mask)));
    if (taken >= target_k)
        return strict_mask;
    // Deterministic tie-break at boundary threshold (thr - 1) via prefix sum across lanes
    Word need = target_k - taken;
    Word my_eq = Word(__builtin_popcount(eq_mask));
    // Inclusive warp prefix sum of my_eq
    Word pref = my_eq;
#pragma unroll
    for (int d = 1; d < 32; d <<= 1) {
        Word up;
        asm volatile("shfl.sync.up.b32 %0, %1, %2, 0, 0xffffffff;" : "=r"(up) : "r"(pref), "r"(d));
        if (lane >= Word(d))
            pref += up;
    }
    Word before = pref - my_eq;
    if (before < need) {
        Word allow = need - before;
        Word rem = eq_mask;
        while (rem && allow) {
            Word bit = rem & (0u - rem);
            strict_mask |= bit;
            rem ^= bit;
            --allow;
        }
    }
    return strict_mask;
}

// ---------------------------------------------------------------------------
// Multi-Compartment Cortical Column Step (Basal + Apical BAC-firing + PV k-WTA)
// Runs on 1 warp (32 lanes = 1024 pyramidal neurons).
// Computes prediction error bursts (surprise & omission) directly in spikes!
// ---------------------------------------------------------------------------
AX_FN void cortical_area_step(CorticalArea &area, const CorticalGenome &g, Word basal_drive_mask,
                              Word apical_pred_mask, Word lateral_drive_mask, Word lane) {
    Word leak = leak_shift(g);
    Word adapt_inc = adapt_step(g);
    Word bac = bac_gain(g);
    Word k = kwta_target(g);
    Word steps = gamma_steps(g);

    short v[32];
    Byte ad[32];
    Byte ex[32];
    Word base_idx = lane * 32u;
#pragma unroll
    for (int b = 0; b < 32; ++b) {
        v[b] = area.voltage[base_idx + b];
        ad[b] = area.adapt[base_idx + b];
        ex[b] = area.excitability[base_idx + b];
    }

    Word out_spikes = 0;
    for (Word step = 0; step < steps; ++step) {
        int b_gain = 48;
        int a_gain = 18;
        int l_gain = 22;
#pragma unroll
        for (int b = 0; b < 32; ++b) {
            Word bit = 1u << b;
            bool has_b = (basal_drive_mask & bit) != 0;
            bool has_a = (apical_pred_mask & bit) != 0;
            bool has_l = (lateral_drive_mask & bit) != 0;
            int dec = v[b] - (v[b] >> leak);
            int curr = (has_b ? b_gain : 0) + (has_a ? a_gain : 0) + (has_l ? l_gain : 0);
            // Larkum BAC-firing calcium plateau coincidence amplification!
            if (has_b && has_a)
                curr += int(bac) * 8;
            // Homeostatic intrinsic excitability gently biases recruitment of quiet neurons
            curr += int(ex[b]) >> 4;
            int next_v = dec + curr - int(ad[b]);
            if (next_v < -64)
                next_v = -64;
            if (next_v > 511)
                next_v = 511;
            v[b] = short(next_v);
        }
        out_spikes = warp_kwta_select(v, k, lane);
#pragma unroll
        for (int b = 0; b < 32; ++b) {
            bool fired = (out_spikes & (1u << b)) != 0;
            if (fired) {
                v[b] = 0;
                int na = int(ad[b]) + int(adapt_inc) * 2;
                ad[b] = Byte(na > 255 ? 255 : na);
                ex[b] = ex[b] > 2 ? Byte(ex[b] - 2) : 0;
            } else {
                ad[b] = Byte(ad[b] - ((ad[b] + 3) >> 2));
                if (ex[b] < 255)
                    ++ex[b];
            }
        }
    }

    // Store updated neuronal states & compute surprise / omission prediction error bursts
    area.spikes.bits[lane] = out_spikes;
    area.predicted.bits[lane] = apical_pred_mask;
#pragma unroll
    for (int b = 0; b < 32; ++b) {
        area.voltage[base_idx + b] = v[b];
        area.adapt[base_idx + b] = ad[b];
        area.excitability[base_idx + b] = ex[b];
    }
    Word fired_cnt = warp_popcount(out_spikes);
    Word surprise = warp_popcount(out_spikes & ~apical_pred_mask);
    Word omission = warp_popcount(apical_pred_mask & ~out_spikes);
    if (lane == 0) {
        area.total_spikes += fired_cnt;
        area.last_surprise = surprise;
        area.last_omission = omission;
        ++area.tick;
    }
}

// ---------------------------------------------------------------------------
// Scalar SDR helpers (usable on both single-thread GPU and host C++)
// ---------------------------------------------------------------------------
AX_FN Word sdr_overlap_scalar(const SpikeVec &a, const SpikeVec &b) {
    Word sum = 0;
    for (Word i = 0; i < WORDS; ++i)
        sum += Word(__builtin_popcount(a.bits[i] & b.bits[i]));
    return sum;
}

AX_FN Word sdr_popcount_scalar(const SpikeVec &a) {
    Word sum = 0;
    for (Word i = 0; i < WORDS; ++i)
        sum += Word(__builtin_popcount(a.bits[i]));
    return sum;
}

AX_FN SpikeVec sdr_or(const SpikeVec &a, const SpikeVec &b) {
    SpikeVec r{};
    for (Word i = 0; i < WORDS; ++i)
        r.bits[i] = a.bits[i] | b.bits[i];
    return r;
}

AX_FN SpikeVec sdr_and_not(const SpikeVec &a, const SpikeVec &b) {
    SpikeVec r{};
    for (Word i = 0; i < WORDS; ++i)
        r.bits[i] = a.bits[i] & ~b.bits[i];
    return r;
}

// Compact 256-neuron SDR helpers for massively parallel 1024-creature GPU evolution
AX_FN Word evo_overlap(const EvoSpike &a, const EvoSpike &b) {
    Word s = 0;
#pragma unroll
    for (Word i = 0; i < EVO_WORDS; ++i)
        s += Word(__builtin_popcount(a.w[i] & b.w[i]));
    return s;
}

AX_FN Word evo_popcount(const EvoSpike &a) {
    Word s = 0;
#pragma unroll
    for (Word i = 0; i < EVO_WORDS; ++i)
        s += Word(__builtin_popcount(a.w[i]));
    return s;
}

AX_FN EvoSpike evo_or(const EvoSpike &a, const EvoSpike &b) {
    EvoSpike r{};
#pragma unroll
    for (Word i = 0; i < EVO_WORDS; ++i)
        r.w[i] = a.w[i] | b.w[i];
    return r;
}

AX_FN EvoSpike evo_and_not(const EvoSpike &a, const EvoSpike &b) {
    EvoSpike r{};
#pragma unroll
    for (Word i = 0; i < EVO_WORDS; ++i)
        r.w[i] = a.w[i] & ~b.w[i];
    return r;
}

// Generate deterministic quasi-orthogonal sparse spike vector of exact sparsity k (out of 256)
AX_FN EvoSpike evo_make_sdr(Word seed, Word k = 16) {
    EvoSpike s{};
    Word set = 0;
    for (Word t = 0; set < k && t < 256; ++t) {
        Word h = mix(seed + t * 0x9e3779b9u);
        Word idx = h & 255u;
        Word &word = s.w[idx >> 5];
        Word bit = 1u << (idx & 31u);
        if (!(word & bit)) {
            word |= bit;
            ++set;
        }
    }
    return s;
}

// Evolvable genome initialization & reproduction with mutation
AX_FN CorticalGenome founder_genome(Word key) {
    CorticalGenome g{};
    g.basal_rule = mix(key ^ 0x1a2b3c4du);
    g.apical_rule = mix(key ^ 0x5e6f7a8bu);
    g.hipp_rule = mix(key ^ 0x9c0d1e2fu);
    g.causal_rule = mix(key ^ 0x3b4c5d6eu);
    g.biophysics = mix(key ^ 0x7f8a9b0cu);
    g.neuromod = mix(key ^ 0xd1e2f3a4u);
    g.executive = mix(key ^ 0xb5c6d7e8u);
    for (Word i = 0; i < 5; ++i)
        g.wiring[i] = mix(key + i * 0x9e3779b9u + 7331u);
    return g;
}

AX_FN CorticalGenome mutate_genome(const CorticalGenome &parent, const CorticalGenome &other, Word key,
                                   bool crossover) {
    CorticalGenome child = parent;
    Word prob = mutation_prob(parent);
    Word *fields[12] = {&child.basal_rule,  &child.apical_rule, &child.hipp_rule, &child.causal_rule,
                        &child.biophysics,  &child.neuromod,    &child.executive, &child.wiring[0],
                        &child.wiring[1],   &child.wiring[2],   &child.wiring[3], &child.wiring[4]};
    const Word *ofields[12] = {&other.basal_rule,  &other.apical_rule, &other.hipp_rule, &other.causal_rule,
                               &other.biophysics,  &other.neuromod,    &other.executive, &other.wiring[0],
                               &other.wiring[1],   &other.wiring[2],   &other.wiring[3], &other.wiring[4]};
    for (Word i = 0; i < 12; ++i) {
        Word r = mix(key + i * 0xd6793f2du);
        if (crossover && (r & 1u))
            *fields[i] = *ofields[i];
        // Point bit mutation
        if (((r >> 8) & 255u) < prob * 3u)
            *fields[i] ^= 1u << ((r >> 16) & 31u);
        // Occasional second bit mutation for faster exploration of 2-bit LUT entries
        if (((r >> 24) & 255u) < prob)
            *fields[i] ^= 1u << ((mix(r + 17u) >> 5) & 31u);
    }
    return child;
}

} // namespace axion::mind::gpu
