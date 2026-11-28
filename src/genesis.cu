#include "genesis_brain.cuh"

using axion::mix;
using namespace axion::genesis;
using namespace axion::genesis::gpu;

#define KERNEL extern "C" __attribute__((global))
#define KERNEL_LB(TPB, MINB) extern "C" __attribute__((global)) __attribute__((launch_bounds(TPB, MINB)))

// 1. Initialize Runtime-Scalable (256x256 / 512x512 / 1024x1024) Continental Geophysical-Chemical-Acoustic World
KERNEL void genesis_world_init(WorldBuffers wb, u32 seed, u32 shift_chem) {
    u32 x = cta_x() * ntid_x() + tid_x();
    u32 y = cta_y() * ntid_y() + tid_y();
    if (x < wb.w && y < wb.h)
        world_init_cell(wb, x, y, seed, shift_chem);
}

// 2. Step World Physics + Hydrodynamics + Thermal + 4-Chem + 3-Band Acoustics + Day/Night + Storms
KERNEL_LB(256, 4) void genesis_world_step(WorldBuffers wb, u32 tick, u32 shift_chem, u32 challenge_tier, u32 seed) {
    world_step_tile(wb, tick, shift_chem, challenge_tier, seed);
}

// 2b. Step Persistent Physical World Objects & Shy Evasive Prey
KERNEL void genesis_objects_step(WorldBuffers wb, const CreatureState *creatures, SimCounters *ctr,
                                 u32 pop_n, u32 tick, u32 seed, u32 shift_chem) {
    world_step_objects(wb, creatures, ctr, pop_n, tick, seed, shift_chem);
}

// 3. Initialize Founder Population Unified CA-DNA + Starting Single-Zygote Layer Slabs
KERNEL void genesis_pop_init(GenesisDNA *dnas, CreatureState *creatures, u8 *needs_dev, TissuePool pool,
                             u32 pop_n, u32 initial_alive, u32 world_w, u32 world_h, u32 seed) {
    u32 i = cta_x() * ntid_x() + tid_x();
    if (i >= pop_n) return;
    CreatureState cr{};
    cr.carried_obj = INVALID_OBJ;
    cr.peer_tpl_band = 3u;
    cr.peer_tpl_age = 255u;
    cr.prev_vocal_band = 3u;
    cr.prev_vocal_age = 255u;
    for (u32 l = 0; l < MAX_LAYERS_PER_CREATURE; ++l) cr.layer_slabs[l] = INVALID_SLAB;
    if (i < initial_alive) {
        u32 r = mix(seed ^ (i * 0x9e3779b9u + 1337u));
        u32 sl0 = alloc_slab(pool.layer_bitmap, pool.max_layers, i * 4u);
        if (sl0 != INVALID_SLAB) {
            dnas[i] = make_founder_dna(r);
            cr.layer_slabs[0] = sl0;
            cr.num_layers = 1u;
            cr.x = rand_unit(r) * float(world_w);
            cr.y = rand_unit(r) * float(world_h);
            cr.theta = rand_unit(r) * 6.2831853f;
            cr.atp = 28.0f;
            cr.prev_atp = 28.0f;
            cr.hp = 100.0f;
            cr.body_temp = 0.45f;
            cr.alive = 1u;
            cr.generation = 1u;
            cr.lineage_id = i + 1u;
            needs_dev[i] = 1u;
        } else {
            cr.alive = 0u;
            needs_dev[i] = 0u;
        }
    } else {
        cr.alive = 0u;
        needs_dev[i] = 0u;
    }
    creatures[i] = cr;
}

// 4a. Compact Active Morphogenesis Queue (1 CTA, ~2 us! Eliminates empty-tick scans & birthday-paradox serialization!)
KERNEL_LB(256, 1) void genesis_build_devlist(const CreatureState *creatures, u8 *needs_dev,
                                             u32 *dev_list, u32 *dev_meta, u32 pop_n) {
    u32 tid = tid_x();
    __attribute__((shared)) u32 s_cnt;
    if (tid == 0u) s_cnt = 0u;
    sync_block();
    for (u32 cid = tid; cid < pop_n; cid += 256u) {
        u8 nd = needs_dev[cid];
        if (nd == 0u) continue;
        if (creatures[cid].alive == 1u && creatures[cid].num_layers > 0u) {
            u32 slot = atomic_add_gen_u32(&s_cnt, 1u);
            dev_list[slot] = (cid << 2) | u32(nd & 3u);
        } else {
            needs_dev[cid] = 0u;
        }
    }
    sync_block();
    if (tid == 0u) {
        dev_meta[0] = s_cnt; // total pending developments
        dev_meta[1] = 0u;    // dynamic work-stealing cursor
    }
}

// 4b. Dynamic Work-Stealing Blackwell Unified CA-GRN Body+Brain Morphogenesis & Growth Cones Kernel!
// Instant <3 us exit when dev_meta[0]==0; perfect 1-to-1 SM load balancing with zero bucket collisions!
KERNEL_LB(256, 2) void genesis_develop(const GenesisDNA *dnas, BodyPhenotype *bodies, CreatureState *creatures,
                                       TissuePool pool, u8 *needs_dev, const u32 *dev_list, u32 *dev_meta, u32 seed) {
    __attribute__((shared)) u32 s_item;
    u32 total = ld_nc_u32(dev_meta);
    if (total == 0u) return;
    while (true) {
        if (tid_x() == 0u) {
            u32 idx = atomic_add_u32(dev_meta + 1, 1u);
            s_item = (idx < total) ? ld_nc_u32(dev_list + idx) : 0xffffffffu;
        }
        sync_block();
        u32 item = s_item;
        if (item == 0xffffffffu) break;
        u32 cid = item >> 2;
        u32 nd = item & 3u;
        develop_organism_block(dnas[cid], bodies[cid], creatures[cid], pool, (nd == 2u), nullptr, mix(seed ^ cid));
        if (tid_x() == 0u) needs_dev[cid] = 0u;
        sync_block();
    }
}

// 5. Single-Organism Unified Embryogenesis Snapshot Kernel (Captures Steps 0, 3, 6, 9 on 16x16 tissue!)
KERNEL_LB(256, 2) void genesis_develop_trace(const GenesisDNA *dna, BodyPhenotype *body, CreatureState *cr,
                                             TissuePool pool, DevSnapshots *snaps, u32 seed) {
    develop_organism_block(*dna, *body, *cr, pool, false, snaps, seed);
}

// 6. Single-CTA Shared-Prefix Compact Active Tissue-Layer Worklist on GPU!
// Zeroes *pool.work_count on-device (ELIMINATES per-tick synchronous cuMemsetD8_v2 stall & 1,024 global atomics!)
KERNEL_LB(256, 1) void genesis_build_worklist(const CreatureState *creatures, TissuePool pool, u32 pop_n) {
    u32 tid = tid_x();
    __attribute__((shared)) u32 s_total;
    if (tid == 0u) s_total = 0u;
    sync_block();

    for (u32 cid = tid; cid < pop_n; cid += 256u) {
        if (creatures[cid].alive != 1u || creatures[cid].num_layers == 0u) continue;
        u32 nl = min_u32(u32(creatures[cid].num_layers), MAX_LAYERS_PER_CREATURE);
        u32 valid_cnt = 0u;
        u32 valid_mask = 0u;
        for (u32 l = 0; l < nl; ++l) {
            if (creatures[cid].layer_slabs[l] != INVALID_SLAB) {
                valid_mask |= (1u << l);
                ++valid_cnt;
            }
        }
        if (valid_cnt == 0u) continue;
        u32 base = atomic_add_gen_u32(&s_total, valid_cnt);
        u32 out_k = 0u;
        for (u32 l = 0; l < nl; ++l) {
            if ((valid_mask & (1u << l)) != 0u && base + out_k < pool.max_layers) {
                pool.work_list[base + out_k] = (cid << 16) | l;
                ++out_k;
            }
        }
    }
    sync_block();
    if (tid == 0u) *pool.work_count = min_u32(s_total, pool.max_layers);
}

// 7. Persistent Blackwell Grid-Stride Variable-Fan-In Spiking Brain Step!
KERNEL_LB(256, 3) void genesis_brain_step(const GenesisDNA *dnas, const BodyPhenotype *bodies, CreatureState *creatures,
                                          WorldBuffers wb, TissuePool pool, SimCounters *ctr, u32 tick, AblationFlags abl) {
    brain_step_layer(dnas, bodies, creatures, wb, pool, ctr, tick, abl);
}

// 8. Embodied 256x256 Physics, Prey Hunting, Pod Cracking, Maturation & Natural Selection!
KERNEL void genesis_body_ecology(GenesisDNA *dnas, const BodyPhenotype *bodies, CreatureState *creatures,
                                 u8 *needs_dev, WorldBuffers wb, TissuePool pool, SimCounters *ctr,
                                 u32 pop_n, u32 tick, u32 seed, AblationFlags abl) {
    body_ecology_step(dnas, bodies, creatures, needs_dev, wb, pool, ctr, pop_n, tick, seed, abl);
}

// 9. True Extinction Guard (SINGLE 256-thread CTA! NEVER triggers unless total living population < min_alive,
//    and ALWAYS clones from living evolved survivors across ALL pop_n slots — NEVER resets 40k ticks back to Gen 1!)
KERNEL void genesis_check_extinct(GenesisDNA *dnas, CreatureState *creatures, u8 *needs_dev, TissuePool pool,
                                  u32 pop_n, u32 min_alive, u32 world_w, u32 world_h, u32 seed, u32 tick) {
    u32 tid = tid_x();
    __attribute__((shared)) u32 s_alive_cnt;
    __attribute__((shared)) u32 s_donor_cnt;
    __attribute__((shared)) u32 s_donors[64];
    __attribute__((shared)) u32 s_filled;

    if (tid == 0u) { s_alive_cnt = 0u; s_donor_cnt = 0u; s_filled = 0u; }
    sync_block();

    // Phase 1: Exact full-population scan across all 0..pop_n-1 slots!
    // CRITICAL BIOLOGICAL GUARD: Only organisms that ACTUALLY foraged or reproduced (energy_gained > 4 || offspring > 0)
    // qualify as evolved rescue donors — prevents 1-cell non-eating "rescue-parasite bricks" from hijacking the gene pool!
    for (u32 idx = tid; idx < pop_n; idx += 256u) {
        if (creatures[idx].alive == 1u) {
            atomic_add_gen_u32(&s_alive_cnt, 1u);
            if (creatures[idx].energy_gained > 4.0f || creatures[idx].offspring > 0u) {
                u32 di = atomic_add_gen_u32(&s_donor_cnt, 1u);
                if (di < 64u) s_donors[di] = idx;
            }
        }
    }
    sync_block();

    // ZERO interference with natural selection whenever ecosystem is healthy!
    if (s_alive_cnt >= min_alive) return;

    u32 deficit = min_alive - s_alive_cnt;
    u32 ndon = min_u32(s_donor_cnt, 64u);

    // Phase 2: Refill ONLY the deficit slots strictly from surviving foraging donors (fresh founders ONLY if 0 real foragers remain!)
    for (u32 idx = tid; idx < pop_n; idx += 256u) {
        if (s_filled >= deficit) break;
        if (creatures[idx].alive != 0u) continue;
        u32 ticket = atomic_add_gen_u32(&s_filled, 1u);
        if (ticket >= deficit) break;
        if (atomic_cas_u32(&creatures[idx].alive, 0u, 2u) != 0u) continue;

        u32 r = mix(seed ^ (tick * 0x9e3779b9u) ^ (idx * 0x85ebca6bu));
        u32 sl0 = alloc_slab(pool.layer_bitmap, pool.max_layers, r + idx * 19u);
        if (sl0 == INVALID_SLAB) { creatures[idx].alive = 0u; continue; }

        u32 donor = (ndon > 0u) ? s_donors[(r ^ idx) % ndon] : ~0u;
        dnas[idx] = (donor != ~0u) ? mutate_dna(dnas[donor], dnas[donor], r, false) : make_founder_dna(r);

        CreatureState cr{};
        for (u32 l = 0; l < MAX_LAYERS_PER_CREATURE; ++l) cr.layer_slabs[l] = INVALID_SLAB;
        cr.layer_slabs[0] = sl0;
        cr.num_layers = 1u;
        cr.carried_obj = INVALID_OBJ;
        cr.peer_tpl_band = 3u;
        cr.peer_tpl_age = 255u;
        cr.prev_vocal_band = 3u;
        cr.prev_vocal_age = 255u;
        cr.x = rand_unit(r) * float(world_w);
        cr.y = rand_unit(r) * float(world_h);
        cr.theta = rand_unit(r) * 6.2831853f;
        cr.atp = 28.0f;
        cr.prev_atp = 28.0f;
        cr.hp = 100.0f;
        cr.body_temp = 0.45f;
        cr.alive = 2u;
        cr.generation = (donor != ~0u) ? (creatures[donor].generation + 1u) : 1u;
        cr.lineage_id = (donor != ~0u) ? creatures[donor].lineage_id : (10000u + tick + idx);
        creatures[idx] = cr;
        needs_dev[idx] = 1u;
        membar_gl();
        atomic_exch_u32(&creatures[idx].alive, 1u);
    }
}

// 10. Pure Blackwell SM 12.0 Hardware SNN Inference Benchmark Kernel (In-Register 4 B/synapse!)
KERNEL_LB(256, 4) void genesis_snn_bench_infer(const GenesisDNA *dnas, u64 *neurons, SynQuad *synapses,
                                               u32 pop_n, u32 n_substeps, u64 *out_spikes) {
    u32 cid = cta_x();
    if (cid >= pop_n) return;
    u32 tid = tid_x();
    const GenesisDNA &dna = dnas[cid];

    __attribute__((shared)) u8 s_ring[TISSUE_CELLS];
    __attribute__((shared)) u32 s_wspk[8];

    u64 nbase = u64(cid) * TISSUE_CELLS;
    u64 sbase = u64(cid) * LAYER_SYN_QUADS;
    u64 nword = ld_l2_u64(neurons + nbase + tid);
    i32 vs = neu_vs(nword);
    i32 va = neu_va(nword);
    u32 ua = neu_ua(nword);
    u32 ring = neu_ring(nword);
    u32 ema = neu_ema(nword);
    u32 tk = neu_tk(nword);
    u32 refrac = neu_refrac(nword);
    u32 plat = neu_plat(nword);
    u32 slow = neu_slow(nword);
    u32 fastp = neu_fastp(nword);
    if (!tk_is_excitable(u8(tk))) tk = (tid < 24u) ? TK_EYE : TK_NEU_EXC;

    u32 syn_reg[SYN_PER_STRIPE];
    #pragma unroll
    for (u32 g = 0; g < QUADS_PER_STRIPE; ++g) {
        ld_syn_quad(synapses + sbase + u64(g) * TISSUE_CELLS + tid,
                    syn_reg[g * 4u + 0u], syn_reg[g * 4u + 1u], syn_reg[g * 4u + 2u], syn_reg[g * 4u + 3u]);
    }
    if (syn_reg[0] == 0u) {
        // Populate dense benchmark stripe if layer had sparse cells (pure hardware roofline measurement!)
        #pragma unroll
        for (u32 k = 0; k < SYN_PER_STRIPE; ++k)
            syn_reg[k] = pack_synapse((tid + k * 13u + 1u) & 255u, (k & 3u) == 3u ? -24 : 26, 0, k & 7u, k & 1u, k & 3u, 6u);
    }

    AblationFlags abl{};
    abl.freeze_plasticity = 1u;
    i32 sens_c = (tk_is_sensor(u8(tk))) ? (700 + i32((cid + tid * 37u) & 255u)) : 0;
    u32 spk_acc = 0u, ed_acc = 0u, rw_acc = 0u;

    snn_run_single_stripe<false, false, false, true>(dna, tid, 0u, n_substeps,
                                                     vs, va, ua, ring, ema, tk, refrac, plat, slow,
                                                     syn_reg, sens_c, 32, 24, 12, abl, s_ring, nullptr, nullptr, nullptr,
                                                     spk_acc, ed_acc, rw_acc, 1u, false, false);

    neurons[nbase + tid] = pack_neuron(vs, va, ua, ring, ema, tk, refrac, plat, slow, fastp);
    u32 wspk = warp_add_u32(spk_acc);
    if (lane_id() == 0u) s_wspk[tid >> 5] = wspk;
    sync_block();
    if (tid == 0u) {
        u32 ts = 0u;
        #pragma unroll
        for (u32 w = 0; w < 8u; ++w) ts += s_wspk[w];
        atomic_add_u64(out_spikes, u64(ts));
    }
}

// 11. Pure Blackwell SM 12.0 Hardware SNN + 3-Factor STDP Plasticity Benchmark Kernel!
KERNEL_LB(256, 4) void genesis_snn_bench_plastic(const GenesisDNA *dnas, u64 *neurons, SynQuad *synapses,
                                                 u32 pop_n, u32 n_substeps, u64 *out_spikes, u64 *out_edits) {
    u32 cid = cta_x();
    if (cid >= pop_n) return;
    u32 tid = tid_x();
    const GenesisDNA &dna = dnas[cid];

    __attribute__((shared)) u8 s_ring[TISSUE_CELLS];
    __attribute__((shared)) u8 s_tk[TISSUE_CELLS];
    __attribute__((shared)) u32 s_mod_w[8];
    __attribute__((shared)) u32 s_wspk[8];
    __attribute__((shared)) u32 s_wed[8];
    if (tid < 8u) s_mod_w[tid] = 0u;

    u64 nbase = u64(cid) * TISSUE_CELLS;
    u64 sbase = u64(cid) * LAYER_SYN_QUADS;
    u64 nword = ld_l2_u64(neurons + nbase + tid);
    i32 vs = neu_vs(nword);
    i32 va = neu_va(nword);
    u32 ua = neu_ua(nword);
    u32 ring = neu_ring(nword);
    u32 ema = neu_ema(nword);
    u32 tk = neu_tk(nword);
    u32 refrac = neu_refrac(nword);
    u32 plat = neu_plat(nword);
    u32 slow = neu_slow(nword);
    u32 fastp = neu_fastp(nword);
    if (!tk_is_excitable(u8(tk))) tk = (tid < 24u) ? TK_EYE : ((tid & 7u) == 0u ? TK_NEU_PV : TK_NEU_EXC);
    s_tk[tid] = u8(tk);

    u32 syn_reg[SYN_PER_STRIPE];
    #pragma unroll
    for (u32 g = 0; g < QUADS_PER_STRIPE; ++g) {
        ld_syn_quad(synapses + sbase + u64(g) * TISSUE_CELLS + tid,
                    syn_reg[g * 4u + 0u], syn_reg[g * 4u + 1u], syn_reg[g * 4u + 2u], syn_reg[g * 4u + 3u]);
    }
    if (syn_reg[0] == 0u) {
        #pragma unroll
        for (u32 k = 0; k < SYN_PER_STRIPE; ++k) {
            u32 pre = (tid + k * 13u + 1u) & 255u;
            bool inh = (pre >= 24u && (pre & 7u) == 0u);
            syn_reg[k] = pack_synapse(pre, inh ? -24 : 26, 0, k & 7u, k & 1u, k & 3u, 6u);
        }
    }

    AblationFlags abl{};
    i32 sens_c = tk_is_sensor(u8(tk)) ? (700 + i32((cid + tid * 37u) & 255u)) : 0;
    u32 spk_acc = 0u, ed_acc = 0u, rw_acc = 0u;

    snn_run_single_stripe<true, false, false, true>(dna, tid, 0u, n_substeps,
                                                    vs, va, ua, ring, ema, tk, refrac, plat, slow,
                                                    syn_reg, sens_c, 32, 24, 12, abl, s_ring, s_tk, nullptr, s_mod_w,
                                                    spk_acc, ed_acc, rw_acc, 1u, false, false, fastp);

    neurons[nbase + tid] = pack_neuron(vs, va, ua, ring, ema, tk, refrac, plat, slow, fastp);
    #pragma unroll
    for (u32 g = 0; g < QUADS_PER_STRIPE; ++g) {
        st_syn_quad(synapses + sbase + u64(g) * TISSUE_CELLS + tid,
                    syn_reg[g * 4u + 0u], syn_reg[g * 4u + 1u], syn_reg[g * 4u + 2u], syn_reg[g * 4u + 3u]);
    }

    u32 wspk = warp_add_u32(spk_acc);
    u32 wed = warp_add_u32(ed_acc);
    if (lane_id() == 0u) { s_wspk[tid >> 5] = wspk; s_wed[tid >> 5] = wed; }
    sync_block();
    if (tid == 0u) {
        u32 ts = 0u, te = 0u;
        #pragma unroll
        for (u32 w = 0; w < 8u; ++w) { ts += s_wspk[w]; te += s_wed[w]; }
        atomic_add_u64(out_spikes, u64(ts));
        atomic_add_u64(out_edits, u64(te));
    }
}
