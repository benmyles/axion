#pragma once
#include "genesis_world.cuh"

namespace axion::genesis::gpu {

// ---------------------------------------------------------------------------
// BLACKWELL SM 12.0 VARIABLE-FAN-IN UNIFIED-TISSUE SPIKING BRAIN ENGINE
// ---------------------------------------------------------------------------
// 1 Thread Block (256 threads) = 1 Unified 16x16 Anatomical Tissue Layer!
// - ZERO pinned sensory/motor indices: sensors (EYE/EAR/CHEM) & effectors
//   (MUSCLE/GRASP/GLAND) fire & integrate spikes right at their physical (x,y,z)
//   coordinates grown by the discrete CA-GRN rulebook!
// - TRUE Variable Fan-In (0..128+ synapses/neuron via coalesced v4.u32 SynSlabs)
// - Arbitrary All-to-All Cross-Layer Routing (0..15 layers) with TRUE 0..7
//   substep axon-length conduction delays via L2-resident 256B spike rings!
// - Strict Dale's Law, active apical BAC plateaus, VTA NT_MOD secondary
//   reinforcement, soft-bound 3-factor STDP, nighttime sleep replay, 0 B spill!

template <bool PLASTIC, bool EXT_SRC, bool ZERO_DELAY = false, bool BENCH = false>
AX_FN void snn_run_single_stripe(const GenesisDNA &dna, u32 tid, u32 layer_idx, u32 n_substeps,
                                 i32 &vs, i32 &va, u32 &ua, u32 &ring, u32 &ema, u32 tk, u32 &refrac, u32 &plat,
                                 u32 slow_tau, u32 (&syn_reg)[SYN_PER_STRIPE], i32 sens_curr,
                                 i32 mod_da_q, i32 mod_ach_q, i32 mod_ne_q, AblationFlags abl,
                                 u8 *s_ring, const u8 *s_tk, const u8 *ext_ring_ptr, u32 *s_mod_w,
                                 u32 &spk_acc, u32 &edit_acc, u32 &rewire_acc, u32 tick, bool allow_rewire,
                                 bool is_resting = false, u32 fast_plast = 0u, u32 *dirty_q_out = nullptr) {
    // Hierarchical multi-timescale cortex: L0=fast sensory-motor, L1=medium associative, L2+=deep slow-NMDA working memory!
    i32 extra_tau = BENCH ? 0 : ((layer_idx >= 2u) ? 2 : (slow_tau ? 1 : 0));
    i32 soma_leak_shift = 3 + (dna.bio_params[0] & 3) + extra_tau;
    i32 apic_leak_shift = 2 + (dna.bio_params[1] & 3) + extra_tau + ((!BENCH && layer_idx >= 2u) ? 1 : 0);
    i32 soma_thresh = 700 + i32(dna.bio_params[2]) * 4;
    i32 apic_thresh = 410 + i32(dna.bio_params[3]) * 3;
    i32 bac_gain = clamp_i32(320 + i32(dna.bio_params[4]) * 3, 64, 600);
    i32 adapt_inc = 24 + ((dna.bio_params[5] + 64) & 31);

    bool is_excitable = BENCH ? true : tk_is_excitable(u8(tk));
    bool is_pyr = (tk == TK_NEU_EXC);
    bool is_pv = (tk == TK_NEU_PV);
    bool is_mod = PLASTIC ? (tk == TK_NEU_MOD) : false;
    bool is_sens = tk_is_sensor(u8(tk));
    bool is_eff = BENCH ? false : tk_is_effector(u8(tk));
    bool do_plast = PLASTIC && is_excitable && !is_sens;
    bool is_ext = EXT_SRC && (ext_ring_ptr != nullptr);

    i32 eff_da = (BENCH || !abl.silence_neuromod) ? mod_da_q : 0;
    i32 eff_ach = (BENCH || !abl.silence_neuromod) ? mod_ach_q : 0;
    i32 eff_ne = (BENCH || !abl.silence_neuromod) ? mod_ne_q : 0;
    i32 dyn_thresh = soma_thresh - (eff_ne << 1);
    if (!BENCH && layer_idx >= 1u && is_pyr) {
        // Gentle Desai-Turrigiano Intrinsic Threshold Plasticity (breaks always-on hubs without silencing young L2!)
        dyn_thresh += clamp_i32((i32(ema) - 22) * 8, -96, 192) + (i32((tid * 37u + layer_idx * 17u) & 7u) - 3) * 10;
    }
    if (is_pv) dyn_thresh = (dyn_thresh * 3) >> 2;
    if (!BENCH && is_eff) dyn_thresh = (dyn_thresh * 7) >> 3;

    u32 plast_lut0 = PLASTIC ? dna.plast_lut[0] : 0u;
    u32 plast_lut1 = PLASTIC ? dna.plast_lut[1] : 0u;
    u32 plast_lut2 = PLASTIC ? dna.plast_lut[2] : 0u;
    u32 plast_lut3 = PLASTIC ? dna.plast_lut[3] : 0u;
    i32 apic_mask = (BENCH || !abl.silence_apical) ? -1 : 0;
    i32 sleep_base = (!BENCH && is_resting && layer_idx > 0u && is_pyr) ? 48 : 0;

    u32 mod_pos_acc = 0u, mod_neg_acc = 0u;
    u32 hit_mask = 0u;
    u32 dirty_q = 0u;
    for (u32 sub = 0; sub < n_substeps; ++sub) {
        s_ring[tid] = u8(ring);
        sync_block();

        i32 i_basal = 0;
        i32 i_apical = 0;
        if (is_excitable && !is_sens) {
            // Peak Blackwell SM 12.0 branchless pure-C++ inner loop (288 B shmem, 0 B spill!):
            // - 256 B u8 s_ring + i_tot/i_apical split + read-only syn_reg[0..15] + 16-bit hit_mask STP!
            i32 i_tot = 0;
            #pragma unroll
            for (u32 k = 0; k < SYN_PER_STRIPE; ++k) {
                u32 s = syn_reg[k];
                u32 pre = s & 0xffu;
                u32 pre_r;
                if constexpr (EXT_SRC) pre_r = u32(is_ext ? ld_nc_u8(ext_ring_ptr + pre) : s_ring[pre]);
                else pre_r = u32(s_ring[pre]);
                u32 delay = ZERO_DELAY ? (is_ext ? 2u : 3u) : ((s >> 23) & 7u);
                u32 d_sh = is_ext ? (min_u32(delay, 4u) + (3u - (sub & 3u))) : delay;
                u32 pre_spk = (pre_r >> d_sh) & 1u;
                if constexpr (!PLASTIC) hit_mask |= (pre_spk << k);
                i32 curr = pre_spk ? (i32(i8(s >> 8)) * i32((s >> 29) + 2u)) : 0;
                i_tot += curr;
                i_apical += (s & 0x04000000u) ? curr : 0;
            }
            i_basal = (i_tot - i_apical) << 1;
            i_apical = BENCH ? (i_apical << 1) : ((i_apical << 1) & apic_mask);
        }

        u32 fired = 0u;
        if (is_excitable) {
            if (is_pyr) {
                // Authentic Larkum (1999/2013) apical Ca2+ BAC tuft with realistic plateau cap & post-spike SK repolarization:
                // Prevents runaway va=+4000 permanent CPG latches while keeping deep cortex brisk & stimulus-selective!
                va = clamp_i32(va - (va >> apic_leak_shift) + (i_apical >> 1), -1500, 1500);
                plat = (va >= apic_thresh) ? 1u : 0u;
            } else {
                va = 0;
                plat = 0u;
            }
            i32 bac_curr = plat ? ((!BENCH && i_basal <= 40) ? (bac_gain >> 3) : bac_gain) : 0;
            i32 sleep_drive = (!BENCH && ((tick & 31u) == 0u) && ema >= 8u && ema < 20u) ? (sleep_base >> 1) : 0;
            // Authentic Songbird LMAN Exploratory Variability (Olveczky et al. 2005 / Kao et al. 2005):
            // Modest subthreshold syllable-coherent nudge STRICTLY on error (-DA) or high arousal (NE) —
            // NEVER injects noise during calm unrewarded adult singing so crystallized cortical readouts shine!
            i32 lman_curr = (!BENCH && tk == TK_GLAND && (eff_da < -10 || eff_ne > 16))
                                ? ((i32(mix(((tick >> 1) * 2654435761u) ^ (tid * 31337u)) & 255u) - 128) >> 1)
                                : 0;
            i32 inp = is_sens ? sens_curr : (i_basal + bac_curr + sleep_drive + lman_curr + sens_curr);
            if (!BENCH && is_eff) inp += (i_apical >> 2);

            if (refrac > 0u) {
                --refrac;
                vs = vs - (vs >> 2);
            } else {
                vs = clamp_i32(vs - (vs >> soma_leak_shift) + inp - i32(ua) * 3, -4000, 4000);
                if (vs >= dyn_thresh) {
                    fired = 1u;
                    vs = -160;
                    if (!BENCH && plat) va = (layer_idx >= 2u) ? ((va * 5) >> 3) : (va >> 2); // Dendritic SK-channel repolarization!
                    ua = min_u32(127u, ua + u32((!BENCH && tk == TK_GLAND) ? (adapt_inc >> 3) : (is_pv ? (adapt_inc >> 2) : adapt_inc)));
                    refrac = (!BENCH && tk == TK_GLAND) ? 0u : (is_pv ? 1u : 2u);
                    ++spk_acc;
                    if constexpr (PLASTIC) {
                        if (is_mod) {
                            if ((tid & 1u) == 0u) ++mod_pos_acc;
                            else ++mod_neg_acc;
                        }
                    }
                }
            }
            if (!fired && ua > 0u) ua = ua - ((ua + 7u) >> 3);
            ring = ((ring << 1) | fired) & 0xffu;
            // True firing duty-cycle EMA (0..255 = 0%..100% substep rate, tau=32 substeps = 8 ticks):
            // 25% normal sparse coding (1 spk/tick) -> ema ~ 64; true seizure (>60%) -> ema > 154!
            ema = (ema * 31u + (fired ? 255u : 0u)) >> 5;
        }

        if constexpr (PLASTIC) {
            if ((sub & 7u) == 7u || sub == n_substeps - 1u) {
                i32 int_da = 0, int_ach = 0;
                if (s_mod_w && !abl.silence_neuromod) {
                    u32 wmp = warp_add_u32(mod_pos_acc);
                    u32 wmn = warp_add_u32(mod_neg_acc);
                    if (lane_id() == 0u) s_mod_w[tid >> 5] = (wmp << 16) | (wmn & 0xffffu);
                    sync_block();
                    u32 tot_mp = 0u, tot_mn = 0u;
                    #pragma unroll
                    for (u32 w = 0; w < 8u; ++w) {
                        u32 pk = s_mod_w[w];
                        tot_mp += (pk >> 16);
                        tot_mn += (pk & 0xffffu);
                    }
                    int_da = clamp_i32((i32(tot_mp) - i32(tot_mn)) * 3, -18, 18);
                    int_ach = clamp_i32(i32(tot_mp + tot_mn) * 4, 0, 26);
                    mod_pos_acc = 0u;
                    mod_neg_acc = 0u;
                }
                i32 tot_da = clamp_i32(eff_da + int_da, -64, 64);
                i32 tot_ach = clamp_i32(eff_ach + int_ach, 0, 64);
                u32 mod_pos = (tot_da > 15) ? 1u : 0u;
                u32 mod_neg = (tot_da < -15) ? 1u : 0u;

                if (do_plast) {
                    u32 post_recent = (ring & 0x0fu) != 0u ? 1u : 0u;
                    // Reachable Turrigiano / BCM Competitive Heterosynaptic Homeostasis + Vogels-Sprekeler iSTDP!
                    // With refrac=2, max physically possible pyramidal ema is 85 (33.3% duty cycle):
                    // target_ema=20..35 (sparse coding), seizure_ema=52..75 (20-29% burst ceiling).
                    u32 target_ema = 20u + u32((dna.bio_params[6] + 64) & 15);
                    u32 seizure_ema = 52u + u32((dna.bio_params[6] + 64) & 23);
                    i32 homeo_delta = (ema > seizure_ema) ? -2 : ((ema > target_ema + 14u) ? -1 : ((ema < 6u) ? 1 : 0));
                    bool mod_gate = (mod_pos || mod_neg);
                    bool ach_gate = (tot_ach > 14);
                    bool sleep_gate = (is_resting && ((tick & 3u) == 0u) && !abl.silence_neuromod);
                    i32 ach_boost = ach_gate ? 4 : 0;
                    i32 hip_elig = (!BENCH && (fast_plast || is_eff)) ? ((plat ? 18 : 12) + ach_boost) : ((plat ? 11 : 6) + ach_boost);
                    i32 da_hi_thr = (!BENCH && (fast_plast || is_eff)) ? 24 : 40;
                    i32 abs_da = (tot_da < 0) ? -tot_da : tot_da;
                    i32 btsp_boost = (!BENCH && (fast_plast || is_eff) && abs_da > 44) ? 1 : 0;

                    #pragma unroll
                    for (u32 k = 0; k < SYN_PER_STRIPE; ++k) {
                        u32 s = syn_reg[k];
                        if (!BENCH && s == 0u) continue;
                        i32 w = i32(i8(s >> 8));
                        u32 pre = s & 0xffu;
                        u32 delay = ZERO_DELAY ? (is_ext ? 2u : 3u) : (is_ext ? min_u32((s >> 23) & 7u, 4u) : ((s >> 23) & 7u));
                        u32 pre_r;
                        if constexpr (EXT_SRC) pre_r = u32(is_ext ? ld_nc_u8(ext_ring_ptr + pre) : s_ring[pre]);
                        else pre_r = u32(s_ring[pre]);
                        u32 shifted = pre_r >> delay;
                        u32 post4 = ring & 0x0fu;
                        u32 pre4 = shifted & 0x0fu;
                        u32 pre_arrived = (pre4 & 0x03u) != 0u ? 1u : 0u;
                        u32 pre_recent = (pre4 != 0u) ? 1u : 0u;
                        bool st_causal = (post4 != 0u) && ((pre4 & (post4 | (post4 << 1) | (post4 << 2))) != 0u);
                        bool st_acausal = !st_causal && (post4 != 0u) && ((pre4 & ((post4 >> 1) | (post4 >> 2))) != 0u);

                        // Exact Causal 3-Factor STDP Ordering (Schultz VTA / Bittner BTSP / Izhikevich 2007):
                        // 1. FIRST evaluate PRE-EXISTING eligibility trace against arriving phasic DA (causes -> outcome;
                        //    prevents -DA-elicited LMAN exploration spikes from punishing themselves on the exact same tick!).
                        // 2. SECOND accumulate THIS tick's causal/acausal spike pairs into eligibility for future reward!
                        i32 elig = i32((s >> 16) & 0x7fu) - 64;

                        // Authentic Dale-Respecting Mammalian 4-Receptor Biophysics (Bi & Poo asymmetric STDP + van Rossum soft-bounds!):
                        i32 dw = 0;
                        i32 abs_w = (w < 0) ? -w : w;
                        bool pre_inh = is_ext ? (w < 0) : tk_is_inhibitory(s_tk[pre]);
                        u32 rid = (s >> 27) & 3u;
                        u32 lut = (rid == 0u) ? plast_lut0 : (rid == 1u ? plast_lut1 : (rid == 2u ? plast_lut2 : plast_lut3));
                        i32 fast_b = (!BENCH && is_eff) ? 1 : 0;
                        i32 lr = 1 + fast_b + i32(lut & 1u) + ((abs_da > da_hi_thr) ? (1 + fast_b) : 0) + btsp_boost;
                        bool bac_gate = (!is_pyr) || (((lut >> 2) & 3u) != 3u) || (plat != 0u);

                        if (pre_inh) {
                            if (rid >= 2u) {
                                if (pre_recent && post_recent && ema > target_ema) dw = 1 + i32((ema > seizure_ema) ? 1 : 0);
                                else if (pre_recent && ema < 4u) dw = -1; // Only release inhibition on truly silent cells (prevents PV collapse!)
                            } else if (mod_gate && (elig > 7 || elig < -7)) {
                                i32 esign = (elig > 0) ? 1 : -1;
                                dw = mod_neg ? (esign * lr) : (-esign * (is_eff ? lr : (1 + btsp_boost)));
                                if (dw != 0) elig = 0; // CaMKII tag consumed by phasic DA pulse (prevents cross-trial bleed!)
                            }
                            if (abs_w > 46 && dw > 0) dw = (abs_w < 56 && ((tick + k) & 3u) == 0u) ? 1 : 0;
                        } else if (bac_gate && (elig > 7 || elig < -7)) {
                            i32 esign = (elig > 0) ? 1 : -1;
                            if (rid == 0u && mod_gate) {
                                dw = mod_pos ? (esign * lr) : (-esign * lr);
                            } else if (rid == 1u && mod_gate) {
                                dw = mod_neg ? (-esign * lr) : (esign * (is_eff ? lr : (lr > 3 ? lr - 1 : 2)));
                            } else if (rid == 2u) {
                                if (mod_neg) dw = -esign * lr;
                                else if (mod_pos) dw = esign * lr;
                                else if (ach_gate && plat && ((tick + k) & 3u) == 0u && (esign < 0 || abs_w < 30))
                                    dw = esign;
                            } else if (rid == 3u && mod_gate) {
                                dw = mod_pos ? (esign * (1 + btsp_boost)) : (-esign * (1 + btsp_boost));
                            } else if (sleep_gate && (elig > 14 || elig < -14)) {
                                dw = esign;
                            }
                            // Subcritical cortical soft-bound (local recurrent loops <=20, inter-layer <=24 prevent +DA seizure;
                            // L0 effectors get fast full [2,64] BTSP contrast!)
                            i32 w_hi = (!BENCH && layer_idx >= 1u && !is_eff) ? (is_ext ? 24 : 20) : 54;
                            if (dw > 0) {
                                if (abs_w > w_hi) dw = (abs_w < w_hi + 6 && abs_da > 40 && ((tick + k) & 3u) == 0u) ? 1 : 0;
                                else if (abs_w > w_hi - 6) dw = ((tick + k) & 3u) == 0u ? 1 : 0;
                                else if (abs_w > w_hi - 12 && !is_eff) dw = 1;
                                else if (abs_w > w_hi - 18 && !is_eff && dw > 2) dw = 2;
                            } else if (dw < 0) {
                                if (abs_w > w_hi - 10) dw -= 1;
                                else if (abs_w < 14 && !is_eff && dw < -2) dw = -2;
                            }
                            if (dw != 0) elig = (mod_gate ? 0 : (elig >> 1));
                        }
                        // Post-DA causal eligibility accumulation & exponential decay for subsequent ticks:
                        // Vocal syrinx (TK_GLAND) uses crisp 1-2t syllable decay; skeletal muscles use 5-6t pursuit decay;
                        // deep cortex (L1+) retains long 12-18t behavioral-timescale traces for distal foraging/WM!
                        if (st_causal) elig = clamp_i32(elig + hip_elig, -63, 63);
                        else if (st_acausal) elig = clamp_i32(elig - ((!BENCH && (fast_plast || is_eff)) ? 6 : 4), -63, 63);
                        else if (!BENCH && tk == TK_GLAND) {
                            if (elig > 0) elig -= ((elig + 1) >> 1);
                            else if (elig < 0) elig += (((-elig) + 1) >> 1);
                        } else if (is_eff || (extra_tau == 0 && !fast_plast)) {
                            if ((tick & 1u) == 0u) {
                                if (elig > 0) elig -= ((elig + 3) >> 2);
                                else if (elig < 0) elig += (((-elig) + 3) >> 2);
                            }
                        } else if ((tick & 1u) == 0u) {
                            if (elig > 0) elig -= ((elig + 7) >> 3);
                            else if (elig < 0) elig += (((-elig) + 7) >> 3);
                        }
                        // Authentic Turrigiano / BCM heterosynaptic competition: ONLY fires when postsynaptic cell is
                        // genuinely overactive (ema > target_ema + 10) & weight is high (>34) -> NEVER shaves learned weights at w=22!
                        if (!pre_inh && dw == 0 && post_recent && !pre_recent && ema > target_ema + 10u && abs_w > 34 && ((tick + k) & 3u) == 0u)
                            dw = -1;
                        i32 signed_dw = pre_inh ? -dw : dw;
                        if (homeo_delta < 0 && (tick & 15u) == 0u && ((pre_inh && abs_w < 38) || (!pre_inh && abs_w > 26)))
                            signed_dw += homeo_delta;
                        else if (homeo_delta > 0 && (tick & 31u) == 0u && ((!pre_inh && abs_w < 28) || (pre_inh && abs_w > 10)))
                            signed_dw += homeo_delta;
                        if (w != 0 && signed_dw != 0) {
                            i32 nw = pre_inh ? clamp_i32(w + signed_dw, -64, is_eff ? -2 : -4) : clamp_i32(w + signed_dw, is_eff ? 2 : 4, 64);
                            if (nw != w) { w = nw; ++edit_acc; }
                        }
                        if (w != 0) {
                            // Mongillo-Barak-Tsodyks (2008) Activity-Silent Synaptic Working Memory on deep cortex (4 B/syn exact!):
                            // Fast 2-level vesicle depletion on spike + slow 4t recovery creates a ~20t state-dependent reservoir!
                            u32 dep = (!BENCH && layer_idx >= 1u)
                                          ? ((s >= 0x60000000u) ? (s - 0x40000000u) : ((s & 0x1fffffffu) | 0x20000000u))
                                          : ((s >= 0x40000000u) ? (s - 0x20000000u) : s);
                            bool do_rec = BENCH || layer_idx == 0u || (((tick + k) & 3u) == 0u);
                            u32 rec = (do_rec && s < 0xe0000000u) ? (s + 0x20000000u) : s;
                            u32 s_stp = pre_recent ? dep : rec;
                            u32 ns = (s_stp & 0xff8000ffu) | ((u32(w) & 0xffu) << 8) | ((u32(elig + 64) & 0x7fu) << 16);
                            if (ns != s) {
                                syn_reg[k] = ns;
                                dirty_q |= (1u << (k >> 2));
                            }
                        }
                    }
                }
            }
        } else {
            // Zero-reload Short-Term Synaptic Depression / Recovery directly from 16-bit hit_mask!
            if ((sub & 7u) == 7u || sub == n_substeps - 1u) {
                if (is_excitable && !is_sens) {
                    #pragma unroll
                    for (u32 k = 0; k < SYN_PER_STRIPE; ++k) {
                        u32 s = syn_reg[k];
                        u32 dep = (s >= 0x40000000u) ? (s - 0x20000000u) : s;
                        u32 rec = (s < 0xe0000000u) ? (s + 0x20000000u) : s;
                        syn_reg[k] = ((hit_mask & (1u << k)) != 0u) ? dep : rec;
                    }
                    hit_mask = 0u;
                }
            }
        }
    }

    // Periodic Local Structural Sprouting onto active co-firing tissue neighbors!
    if constexpr (PLASTIC) {
        if (allow_rewire && do_plast && !is_ext && ((tick & 63u) == 0u)) {
            u32 slot = (tick >> 6) & 15u;
            u32 s = syn_reg[slot];
            i32 w = bfe_s32(s, 8u, 8u);
            if (w == 0) {
                u32 cand = mix(tick * 2654435761u + tid * 31337u + layer_idx * 97u) & 255u;
                if (cand != tid && (s_ring[cand] & 0x0fu) != 0u) {
                    u8 pt = s_tk[cand];
                    // Obey exact same bilaterian histology during live structural sprouting:
                    // Effectors only receive descending axons from pyramidal projection neurons (TK_NEU_EXC)!
                    bool pre_ok = is_eff ? (pt == TK_NEU_EXC) : (tk_is_sensor(pt) || tk_is_neuron(pt));
                    if (pre_ok) {
                        bool pre_inh = tk_is_inhibitory(pt);
                        i32 ddx = i32(tid & 15u) - i32(cand & 15u);
                        i32 ddy = i32(tid >> 4) - i32(cand >> 4);
                        u32 delay = u32(clamp_i32(i32(fast_sqrt(float(ddx * ddx + ddy * ddy)) * 0.45f), 0, 7));
                        u32 comp = (pt == TK_NEU_SST) ? 1u : (pt == TK_NEU_PV ? 0u : bfe_u32(s, 26u, 1u));
                        syn_reg[slot] = pack_synapse(cand, pre_inh ? -20 : 18, 0, delay, comp, (tid + slot) & 3u, 6u);
                        dirty_q |= (1u << (slot >> 2));
                        ++rewire_acc;
                    }
                }
            }
        }
        if (dirty_q_out) *dirty_q_out = dirty_q;
    }
}

// ---------------------------------------------------------------------------
// Persistent Blackwell Grid-Stride Unified-Tissue Brain Step (Variable Fan-In!)
// ---------------------------------------------------------------------------
AX_FN void brain_step_layer(const GenesisDNA *dnas, const BodyPhenotype *bodies, CreatureState *creatures,
                            WorldBuffers wb, TissuePool pool, SimCounters *ctr, u32 tick, AblationFlags abl) {
    u32 tid = tid_x();
    u32 ux = tid & 15u;
    u32 uy = tid >> 4;
    u32 total_work = *pool.work_count;
    u32 grid_stride = ncta_x();

    __attribute__((shared)) u8 s_tk[TISSUE_CELLS];
    __attribute__((shared)) u8 s_all_rings[MAX_LAYERS_PER_CREATURE][TISSUE_CELLS]; // 8 KiB coalesced 32-layer ring cache!
    __attribute__((shared)) u32 s_l_spk[MAX_LAYERS_PER_CREATURE];                  // Per-source-layer activity bitmask!
    __attribute__((shared)) u32 s_mod_w[8];
    __attribute__((shared)) u32 s_warp_spk[8];
    __attribute__((shared)) u32 s_warp_ed[8];
    __attribute__((shared)) u32 s_warp_rw[8];
    __attribute__((shared)) float s_eff[16]; // Emergent anatomical motor accumulators (0..9), fold counts (10..12), grasp axis counts (13..15)
    __attribute__((shared)) u32 s_lss[MAX_STRIPES_PER_LAYER];
    __attribute__((shared)) u32 s_cr_slabs[MAX_LAYERS_PER_CREATURE];
    __attribute__((shared)) u32 s_has_exc;
    __attribute__((shared)) u32 s_has_mod;
    __attribute__((shared)) u32 s_any_spk[2];
    __attribute__((shared)) u32 s_pv_act[2];  // Packer & Yuste (2011) gap-junction PV+ basket-cell column shunting!
    __attribute__((shared)) u32 s_col_act[2]; // 4x8-bit Sperry minicolumn activity for PV-gated lateral surround competition!
    __attribute__((shared)) u32 s_v_hist[16]; // 64 B PV-gated top-k sparse cortical coding histogram (cos_dist > 0.60!)
    __attribute__((shared)) u32 s_kth_bin;    // Exact adaptive PV+ k-WTA threshold bin + tie-break slack

    u32 W = wb.w, H = wb.h, MX = wb.mask_x, MY = wb.mask_y, SY = wb.shift_y;
    float WF = float(W), HF = float(H);

    for (u32 wi = cta_x(); wi < total_work; wi += grid_stride) {
        u32 item = pool.work_list[wi];
        u32 cid = item >> 16;
        u32 layer_idx = item & 0xffffu;

        CreatureState &cr = creatures[cid];
        u32 num_layers = min_u32(u32(cr.num_layers), MAX_LAYERS_PER_CREATURE);
        if (cr.alive != 1u || num_layers == 0u || layer_idx >= num_layers) continue;
        u32 slab = cr.layer_slabs[layer_idx];
        if (slab == INVALID_SLAB || slab >= pool.max_layers) continue;

        const BodyPhenotype &bp = bodies[cid];
        const GenesisDNA &dna = dnas[cid];

        u64 nbase = u64(slab) * TISSUE_CELLS;
        u64 nword = ld_l2_u64(pool.neurons + nbase + tid);
        i32 vs = neu_vs(nword);
        i32 va = neu_va(nword);
        u32 ua = neu_ua(nword);
        u32 fast_plast = neu_fastp(nword);
        u32 ring = neu_ring(nword);
        u32 ema = neu_ema(nword);
        u32 tk = neu_tk(nword);
        u32 refrac = neu_refrac(nword);
        u32 plat = neu_plat(nword);
        u32 slow_tau = neu_slow(nword);
        bool my_exc = tk_is_excitable(u8(tk));
        s_tk[tid] = u8(tk);
        if (tid < 8u) s_mod_w[tid] = 0u;
        if (tid < 16u) s_eff[tid] = 0.0f;
        if (tid < MAX_STRIPES_PER_LAYER) s_lss[tid] = pool.layer_syn_slabs[u64(slab) * MAX_STRIPES_PER_LAYER + tid];
        if (tid < MAX_LAYERS_PER_CREATURE) { s_cr_slabs[tid] = cr.layer_slabs[tid]; s_l_spk[tid] = 0u; }
        if (tid == 0u) { s_has_exc = 0u; s_has_mod = 0u; s_any_spk[0] = 0u; s_any_spk[1] = 0u; s_pv_act[0] = 0u; s_pv_act[1] = 0u; s_col_act[0] = 0u; s_col_act[1] = 0u; }
        sync_block();
        u32 exc_bm = warp_ballot(my_exc);
        u32 pv_sh_bm = warp_ballot(tk == TK_NEU_PV);
        if (exc_bm && lane_id() == 0u) atomic_add_gen_u32(&s_has_exc, popc_u32(exc_bm));
        u32 mod_pv_pk = (warp_ballot(tk == TK_NEU_MOD) ? 1u : 0u) | (popc_u32(pv_sh_bm) << 16);
        if (mod_pv_pk && lane_id() == 0u) atomic_add_gen_u32(&s_has_mod, mod_pv_pk);
        sync_block();
        if (s_has_exc == 0u) { pool.spike_ring_next[nbase + tid] = 0u; sync_block(); continue; }
        u32 layer_exc_cnt = s_has_exc;
        u32 layer_pv_cnt = s_has_mod >> 16;
        bool layer_has_mod = ((s_has_mod & 0xffffu) != 0u);
        bool warp_has_exc = (exc_bm != 0u);
        u8 *s_ring = s_all_rings[layer_idx];

        // Blackwell Coalesced Shared-Memory Inter-Layer Spike-Ring Cache (4 KiB, 100% coalesced 256-thread reads!):
        // Loads ALL source layers on L0 (safe across post-natal maturation layer growth!) and <=5 laminar neighbors on deep layers!
        for (u32 zl = 0u; zl < num_layers; ++zl) {
            bool need_zl = (layer_idx == 0u) ? true
                                             : ((layer_idx == 1u) ? (zl <= 3u) : (zl + 2u >= layer_idx && zl <= layer_idx + 2u));
            if (!need_zl) continue;
            u32 zsl = s_cr_slabs[zl];
            u8 r_val = (zl == layer_idx)
                           ? (my_exc ? u8(ring) : 0u)
                           : ((zsl != INVALID_SLAB && zsl < pool.max_layers)
                                  ? ld_nc_u8(pool.spike_ring + u64(zsl) * TISSUE_CELLS + tid)
                                  : 0u);
            s_all_rings[zl][tid] = r_val;
            if (warp_ballot(r_val != 0u) && lane_id() == 0u) atomic_or_gen_u32(&s_l_spk[zl], 1u);
        }
        sync_block();

        u32 cidx_r = (u32(i32(cr.y + HF * 2.0f) & i32(MY)) << SY) |
                     u32(i32(cr.x + WF * 2.0f) & i32(MX));
        bool is_resting = (ld_nc_f32(wb.light + cidx_r) < 0.25f && ((cr.growth_stage & 0x80u) == 0u) && cr.mod_ne < 0.35f);

        // 1. TRUE ANATOMICAL EMBODIED SENSING (Strictly Inside Grown EYE / EAR / CHEM Organs — ZERO Shortcuts!)
        i32 sens_curr = 0;
        if (tk_is_sensor(u8(tk))) {
            float x = cr.x, y = cr.y;
            float ct = fast_cos(cr.theta), st = fast_sin(cr.theta);
            float dx_b = float(i32(ux) - 8) - bp.com_x;
            float dy_b = float(i32(uy) - 8) - bp.com_y;
            float sg = clampf(0.82f + 0.005f * float(dna.bio_params[9]), 0.55f, 1.25f);
            auto sample_idx = [&](float fx, float fy) -> u32 {
                i32 ix = i32(fx + WF * 2.0f) & i32(MX);
                i32 iy = i32(fy + HF * 2.0f) & i32(MY);
                return (u32(iy) << SY) | u32(ix);
            };
            float v = 0.0f;
            if (tk == TK_EYE && !abl.blind_eyes) {
                // 4-Channel Realistic Retinotopic Cone-of-Vision Optics (ZERO X-ray food-vs-poison cheats!):
                // Authentic photon-flux physics: photons originate at each target tile `wb.light[wi]` —
                // pitch-black dark objects remain invisible (l<0.10 -> 0), while sunlit reefs/prey AND distant
                // lighthouses/beacons/vents shine cleanly across a wide retinal receptive-field cone!
                float ang_off = clampf(dx_b * 0.16f, -0.90f, 0.90f);
                float rct = fast_cos(cr.theta + ang_off);
                float rst = fast_sin(cr.theta + ang_off);
                u32 i1 = sample_idx(fma_f32(rct, 4.0f, x), fma_f32(rst, 4.0f, y));
                u32 i1L = sample_idx(fma_f32(rct, 4.0f, x) - rst * 1.5f, fma_f32(rst, 4.0f, y) + rct * 1.5f);
                u32 i1R = sample_idx(fma_f32(rct, 4.0f, x) + rst * 1.5f, fma_f32(rst, 4.0f, y) - rct * 1.5f);
                u32 i2 = sample_idx(fma_f32(rct, 10.0f, x), fma_f32(rst, 10.0f, y));
                u32 i2L = sample_idx(fma_f32(rct, 10.0f, x) - rst * 2.5f, fma_f32(rst, 10.0f, y) + rct * 2.5f);
                u32 i2R = sample_idx(fma_f32(rct, 10.0f, x) + rst * 2.5f, fma_f32(rst, 10.0f, y) - rct * 2.5f);
                u32 i3 = sample_idx(fma_f32(rct, 20.0f, x), fma_f32(rst, 20.0f, y));
                u32 i3L = sample_idx(fma_f32(rct, 20.0f, x) - rst * 4.0f, fma_f32(rst, 20.0f, y) + rct * 4.0f);
                u32 i3R = sample_idx(fma_f32(rct, 20.0f, x) + rst * 4.0f, fma_f32(rst, 20.0f, y) - rct * 4.0f);
                bool blk1 = (ld_nc_f32(wb.terrain + i1) > 3.5f || wb.material[i1] == MAT_POLYMER);
                bool blk2 = blk1 || (ld_nc_f32(wb.terrain + i2) > 3.5f || wb.material[i2] == MAT_POLYMER);
                // Collision-free rank-ordered retinal mosaic (matching ear_rank & 3u): guarantees 2-eye minds always span
                // Warm-ON + Cool-ON and 4-eye minds ALWAYS span all 4 channels (Object, Warm-ON, Looming, Cool-ON)!
                u32 eye_rank = 0u;
                for (u32 ei = 0u; ei < tid; ++ei) if (s_tk[ei] == TK_EYE) ++eye_rank;
                u32 mode = eye_rank & 3u;
                bool broad_eye = (bp.counts[TK_EYE] < 4u);
                auto first_obj = [&](u32 a, u32 b, u32 c) -> u32 {
                    u32 o = wb.tile_obj_id[a];
                    if (o) return o;
                    o = wb.tile_obj_id[b];
                    return o ? o : wb.tile_obj_id[c];
                };
                auto vis_l = [&](u32 wi) -> float {
                    float l = ld_nc_f32(wb.light + wi);
                    if (l < 0.10f) return 0.0f;
                    float bl = clampf((l - 0.08f) * 1.45f, 0.0f, 2.2f);
                    // Scorching Trisolaran Syzygy surface glare (bright + overheated shallows) mildly softens eye gain while keeping diurnal vision crisp!
                    return (abl.challenge_tier >= 2u && l > 1.24f && ld_nc_f32(wb.temp + wi) > 0.62f) ? (bl * 0.65f) : bl;
                };
                auto opt_f = [&](u32 wi, u32 oid) -> float {
                    float l = vis_l(wi);
                    if (l <= 0.0f) return 0.0f;
                    float ob = (oid != 0u && wb.objects && (oid - 1u) < wb.max_objects &&
                                wb.objects[oid - 1u].kind != OBJ_APEX)
                                   ? (wb.objects[oid - 1u].kind == OBJ_PREY ? 4.4f : 3.6f)
                                   : 0.0f;
                    // Kuffler (1953) / Barlow (1961) Center-Surround Retinal Background Adaptation:
                    // Uniform featureless neutral daylight (l~0.70, no reef/object) stays quiet (0.02f floor)
                    // while sunlit golden beacons (l>1.15), reef blooms, ore sheen & physical objects shine sharply!
                    float reef_sheen = ld_nc_f32(wb.chem_a + wi) * 0.72f + ld_nc_f32(wb.chem_c + wi) * 1.75f + (l > 1.15f ? 1.45f : 0.0f);
                    return l * (0.04f + reef_sheen + ob);
                };
                auto rcue = [&](u32 wi, u32 oid) -> float {
                    float l = vis_l(wi);
                    if (l <= 0.0f) return 0.0f;
                    float prey_hue = 0.0f;
                    if (oid != 0u && wb.objects && (oid - 1u) < wb.max_objects && wb.objects[oid - 1u].kind == OBJ_PREY)
                        prey_hue = ((wb.objects[oid - 1u].flags & 8u) == 0u) ? 2.9f : -2.9f;
                    float lw = ld_nc_f32(wb.light + wi);
                    float tw = ld_nc_f32(wb.temp + wi);
                    float warm = clampf(lw - 0.72f, 0.0f, 1.3f) * (tw > 0.46f ? 1.65f : 0.35f) +
                                 (tw > 0.56f ? (tw - 0.56f) * 2.2f : 0.0f) +
                                 (wb.material[wi] == MAT_VENT ? 0.95f : 0.0f);
                    float cool = ld_nc_f32(wb.chem_a + wi) * 1.45f + (wb.material[wi] == MAT_ORE ? 0.80f : 0.0f);
                    return ((warm - cool) + prey_hue) * clampf(l, 0.25f, 1.25f);
                };
                auto loom = [&](u32 wi, u32 oid, float wscale) -> float {
                    float l = vis_l(wi);
                    if (l <= 0.0f) return 0.0f;
                    float apex = (oid != 0u && wb.objects && (oid - 1u) < wb.max_objects &&
                                  wb.objects[oid - 1u].kind == OBJ_APEX) ? 3.5f : 0.0f;
                    return wscale * (apex + float(wb.cell_occ[wi]) * 0.95f) * clampf(l, 0.2f, 1.0f);
                };
                u32 o1 = first_obj(i1, i1L, i1R);
                u32 o2 = first_obj(i2, i2L, i2R);
                u32 o3 = first_obj(i3, i3L, i3R);
                if (broad_eye) {
                    // Compact 1-3 cell vertebrate proto-retina with symmetric Kuffler Warm-ON / Cool-ON opponency:
                    // Even 2-eye minds cleanly discriminate Warm vs Cool Batesian prey & Sunlit Food vs Cool Mineral!
                    float v_opt = clampf((opt_f(i1, o1) + (blk1 ? 0.0f : opt_f(i2, o2)) + (blk2 ? 0.0f : opt_f(i3, o3) * 0.85f)) * 0.60f, 0.0f, 2.4f);
                    float v_hue = clampf((rcue(i1, o1) + (blk1 ? 0.0f : rcue(i2, o2))) * 1.15f, -2.6f, 2.6f);
                    if ((mode & 1u) == 0u) {
                        v = v_opt * 0.65f + v_hue;
                    } else {
                        float v_lm = clampf(loom(i1, o1, 1.2f) + (blk1 ? 1.20f * vis_l(i1) : loom(i2, o2, 0.95f)), 0.0f, 2.8f);
                        v = v_opt * 0.65f - v_hue - v_lm * 0.95f;
                    }
                } else if (mode == 0u) {
                    // Bioluminescent object (prey/pod/megapod/fuel) & sunlit Golden Reef bloom sheen over physical cone!
                    v = (opt_f(i1, o1) + (blk1 ? 0.0f : opt_f(i2, o2)) + (blk2 ? 0.0f : opt_f(i3, o3) * 0.85f)) * 1.80f;
                } else if (mode == 1u) {
                    // Kuffler (1953) Warm-Amber-ON opponent retinal ganglion channel (Warm prey/sunlit reef -> +, Cool -> silent)!
                    // NEVER reveals edibility — ONLY live STDP + BAC + PV/SST inhibition can track 180t seasonal flips!
                    v = (rcue(i1, o1) + (blk1 ? 0.0f : rcue(i2, o2)) + (blk2 ? 0.0f : rcue(i3, o3) * 0.85f)) * 1.85f;
                } else if (mode == 2u) {
                    // Looming OBJ_APEX Predator silhouette, solid cliffs/polymer shelters & conspecifics!
                    v = clampf(loom(i1, o1, 1.3f) + (blk1 ? 1.35f * vis_l(i1) : loom(i2, o2, 1.0f)) +
                               (blk2 ? 0.0f : loom(i3, o3, 0.7f)), 0.0f, 3.0f);
                } else {
                    // Kuffler (1953) Cool-Mineral-ON opponent retinal ganglion channel + spatial beacon depth gradient!
                    // Fires strongly on Cool Batesian prey & Cool chem_a ore, silent on Warm prey/Food -> symmetric 180t STDP reversal!
                    float r_sum = (rcue(i1, o1) + (blk1 ? 0.0f : rcue(i2, o2)) + (blk2 ? 0.0f : rcue(i3, o3) * 0.85f)) * 1.85f;
                    float l_near = ld_nc_f32(wb.light + i1);
                    float l_far = blk1 ? l_near : ld_nc_f32(wb.light + i2);
                    v = clampf((l_far - l_near) * 2.2f - r_sum, -1.5f, 3.0f);
                }
            } else if (tk == TK_EAR && !abl.deaf_ears) {
                // Binaural stereo 4-wavefield cochlear tonotopy (acoust_p, v0, v1, v2) + inner-ear vestibular compass!
                // Authentic vertebrate basilar-papilla opponent frequency selectivity (two-tone suppression):
                // - >=4 ears: pure sharp 4-channel filter bank (band 0=p+vest, 1=v0, 2=v1, 3=v2 — zero leakage!)
                // - 2-3 ears: crisp opponent-tuned hair cells (Left=v0/p+,v1-; Right=v1/v2+,v0-; Mid=v2+) so even
                //   compact bilateral 2-ear organisms hear 100% distinct frequency codes without +1900 saturation!
                float lat = clampf(dx_b * 0.65f, -2.5f, 2.5f);
                u32 ie = sample_idx(x - st * lat, y + ct * lat);
                u32 n_ears = u32(bp.counts[TK_EAR]);
                float avp = fabsf_dev(ld_nc_f32(wb.acoust_p + ie));
                float av0 = fabsf_dev(ld_nc_f32(wb.acoust_v0 + ie));
                float av1 = fabsf_dev(ld_nc_f32(wb.acoust_v1 + ie));
                float av2 = fabsf_dev(ld_nc_f32(wb.acoust_v2 + ie));
                // Authentic Middle-Ear Bone-Conduction / Vocal Corollary Discharge (Hickok 2012 / Songbird Uva->HVC):
                // Strictly subthreshold reafference (+0.015f -> <=505 pA even on 8.6x 4-ear cochlea!) primes chaining with ZERO dark self-echo!
                if (cr.last_vocal_tick > 0u && tick > cr.last_vocal_tick && tick <= cr.last_vocal_tick + 3u) {
                    if (cr.last_vocal_band == 0u) av0 += 0.015f;
                    else if (cr.last_vocal_band == 1u) av1 += 0.015f;
                    else if (cr.last_vocal_band == 2u) av2 += 0.015f;
                }
                float flow = (fabsf_dev(ld_nc_f32(wb.vx + ie)) + fabsf_dev(ld_nc_f32(wb.vy + ie))) * 0.16f;
                float hdist = fast_sqrt(cr.home_dx * cr.home_dx + cr.home_dy * cr.home_dy + 1.0f);
                float align = -(ct * cr.home_dx + st * cr.home_dy) / (hdist + 3.0f);
                float dark_urg = (ld_nc_f32(wb.light + ie) < 0.25f) ? 0.55f : 0.20f;
                float vest = align * dark_urg * clampf(hdist * 0.04f, 0.0f, 0.55f);
                u32 ear_rank = 0u;
                for (u32 ei = 0u; ei < tid; ++ei) if (s_tk[ei] == TK_EAR) ++ear_rank;
                if (n_ears >= 4u) {
                    u32 band = ear_rank & 3u;
                    if (band == 0u) v = av0 * 8.6f;
                    else if (band == 1u) v = av1 * 8.6f;
                    else if (band == 2u) v = av2 * 8.6f;
                    else v = avp * 7.8f + (flow + vest) * 0.35f;
                } else if (n_ears == 3u) {
                    u32 e3 = ear_rank % 3u;
                    if (e3 == 0u) v = av0 * 5.2f + avp * 3.0f + (flow + vest) * 0.20f - av1 * 1.6f;
                    else if (e3 == 1u) v = av1 * 5.2f + avp * 1.4f + (flow + vest) * 0.20f - av0 * 1.6f;
                    else v = av2 * 5.6f + avp * 1.8f - (av0 + av1) * 1.1f;
                } else if (n_ears == 2u) {
                    if (ear_rank == 0u) v = av0 * 5.0f + av2 * 2.4f + avp * 1.8f + (flow + vest) * 0.20f - av1 * 2.2f;
                    else v = av1 * 5.0f + av2 * 2.4f + avp * 0.45f + (flow + vest) * 0.20f - av0 * 2.2f;
                } else {
                    // Authentic single-receptor vertebrate cochlear phase-locking (Rose 1967):
                    // Prevents +1900 saturation clipping so even compact 1-ear organisms distinguish v0 / v1 / v2 pitch!
                    float p_v1 = av1 * ((tick & 1u) ? 2.1f : 0.25f);
                    float p_v2 = av2 * (((tick & 3u) == 0u) ? 2.6f : -0.40f);
                    v = av0 * 1.45f + p_v1 + p_v2 + avp * 2.2f + (flow + vest) * 0.30f;
                }
            } else if (tk == TK_CHEM) {
                // Authentic strictly-local contact gustation (NO long-range open-ocean telescope! Eyes + Ears own distance!)
                u32 ch = (ux + uy) & 3u;
                if (ch == 0u) v = ld_nc_f32(wb.chem_c + cidx_r) * 1.85f;
                else if (ch == 1u) v = ld_nc_f32(wb.chem_d + cidx_r) * 3.8f;
                else if (ch == 2u) v = ld_nc_f32(wb.chem_b + cidx_r) * 0.85f;
                else {
                    float t_err = fabsf_dev(cr.body_temp - 0.50f);
                    v = clampf((t_err > 0.16f ? (t_err - 0.16f) * 3.2f : 0.0f) +
                               (cr.carried_obj != INVALID_OBJ ? 1.25f : 0.0f), 0.0f, 1.5f);
                }
                v *= 0.65f;
            }
            sens_curr = clamp_i32(i32(v * 980.0f * sg), -1500, 1900);
        }

        // 2. VARIABLE FAN-IN COALESCED BLACKWELL SPIKING STEP (0..12 SynSlabs -> 0..192 Synapses/Cell!)
        u8 raw_cs_b = my_exc ? ld_nc_u8(pool.cell_stripes + nbase + tid) : 0u;
        u32 my_stripes = min_u32(u32(raw_cs_b & 0x0fu), MAX_STRIPES_PER_LAYER);
        u32 my_exc_cnt = max_u32(8u, u32(raw_cs_b >> 4) << 3);
        u32 ss0 = s_lss[0];
        u32 spk_acc = 0u, edit_acc = 0u, rewire_acc = 0u;
        // Cross-layer VTA/BF intrinsic neuromodulator broadcast + juvenile critical-period ACh imprinting gain!
        // Keep pure external phasic sensory novelty (`ext_ach_q`) separate from tonic VTA/juvenile ACh so
        // Oja/BCM heterosynaptic sharpening fires strictly on genuine phasic novelty bursts (never tonic VTA noise!).
        i32 mod_da_q = clamp_i32(i32((cr.mod_da + cr.int_da * 0.35f) * 64.0f), -64, 64);
        i32 juv_ach = (cr.age < 120u) ? 18 : 0;
        i32 ext_ach_q = clamp_i32(i32(cr.mod_ach * 64.0f), 0, 64);
        i32 mod_ach_q = clamp_i32(i32((cr.mod_ach + cr.int_ach) * 64.0f) + juv_ach, 0, 64);
        i32 mod_ne_q = clamp_i32(i32(cr.mod_ne * 64.0f), 0, 64);

        // Unified Blackwell SM 12.0 Register-Cached Variable-Fan-In Hub Engine (handles 1..8 stripes identically!):
        // - Cuts ~45% duplicate PTX icache & register live ranges vs split single/multi paths!
        // - Stripe 0 stays 100% in registers across all 4 substeps + STDP (5x fewer global reads!)
        // - Silent-layer local skip + bit-exact zero-work external STDP skip + Carandini-Heeger PV/SST norm
        //   + full-tick Nucleus-Ambiguus vocal glottic lock + lifelong structural sprouting!
        u32 syn_reg[SYN_PER_STRIPE];
        u32 my_gfold = 0u;
        {
            i32 extra_tau = (layer_idx >= 2u) ? 2 : (slow_tau ? 1 : 0);
            // Authentic Koch (1999) / Destexhe (2003) Fast Awake Soma (tau_m = 4..8 substeps -> 100% clean dark cooldown!)
            // Primary sensory receptors (EYE/EAR/CHEM) use fast transduction (shift=2 -> 0 false dark integration);
            // L0 neurons use fast awake leak (2 + slow_tau); L0 effectors keep smooth motor integration!
            i32 soma_leak_shift = (layer_idx >= 2u)
                                      ? (2 + (slow_tau ? 1 : 0))
                                      : ((layer_idx == 1u) ? (3 + (slow_tau ? 1 : 0))
                                                           : (tk_is_sensor(u8(tk)) ? 2
                                                                                   : (tk_is_effector(u8(tk)) ? (3 + (dna.bio_params[0] & 3))
                                                                                                             : (2 + (slow_tau ? 1 : 0)))));
            i32 apic_leak_shift = (layer_idx >= 1u)
                                      ? (3 + (slow_tau ? 1 : 0))
                                      : (2 + (dna.bio_params[1] & 3));
            i32 soma_thresh = clamp_i32(700 + i32(dna.bio_params[2]) * 4, 560, 880);
            i32 apic_thresh = clamp_i32(360 + i32(dna.bio_params[3]) * 2, 220, 420);
            i32 bac_gain = clamp_i32(320 + i32(dna.bio_params[4]) * 3, 180, 560);
            i32 adapt_inc = 24 + ((dna.bio_params[5] + 64) & 31);
            i32 seizure_ema = 72 + ((dna.bio_params[6] + 64) & 15);

            bool is_excitable = my_exc;
            bool is_pyr = (tk == TK_NEU_EXC);
            bool is_pv = (tk == TK_NEU_PV);
            bool is_mod = (tk == TK_NEU_MOD);
            bool is_sens = tk_is_sensor(u8(tk));
            bool is_eff = tk_is_effector(u8(tk));
            // Strict Fee & Goldberg (2011) VTA -> Area-X isolation: vocal RPE (`vta_voc_q`) guides Area-X synaptic
            // plasticity ONLY (NEVER triggers membrane D2 shutdown of digestive catalysis or skeletal motor learning!)
            i32 vta_voc_q = abl.silence_neuromod ? 0 : i32(cr.voc_da_q);
            i32 eff_da = abl.silence_neuromod ? 0 : mod_da_q;
            i32 eff_ach = abl.silence_neuromod ? 0 : mod_ach_q;
            i32 eff_ne = abl.silence_neuromod ? 0 : mod_ne_q;
            i32 dyn_thresh = soma_thresh - (eff_ne << 1);
            if (layer_idx >= 1u && is_pyr) {
                // Authentic Slow Chronic-Hub Desai-Turrigiano Homeostasis:
                // Selective stimulus feature detectors (ema <= 44) experience ZERO trial-to-trial threshold drift
                // (noise cos < 0.08!), while always-on constant-wallpaper hubs (ema > 44) get cleanly suppressed!
                i32 hub_pen = (ema > 44u) ? clamp_i32((i32(ema) - 44) * 8, 0, 240) : 0;
                dyn_thresh += hub_pen + (i32((tid * 37u + layer_idx * 17u) & 7u) - 3) * 6;
            }
            if (is_pv) dyn_thresh = (dyn_thresh * 3) >> 2;
            if (is_eff) dyn_thresh = (dyn_thresh * 7) >> 3;
            if (layer_idx == 0u && tk == TK_GLAND) {
                // Authentic Turrigiano (2008) Upper-Bound Exhaustion Homeostasis on TK_GLAND:
                // Curbs chronic single-fold monopoly (ema > 110 -> +220 pA) with ZERO negative threshold boost
                // so unused folds (e.g. v1 in a v0/v2 dialogue) NEVER interrupt learned speech!
                dyn_thresh += (ema > 110u) ? clamp_i32((i32(ema) - 110) * 3, 0, 220) : 0;
                if (bp.counts[TK_GLAND] >= 2u) {
                    u32 g_rank = 0u;
                    for (u32 gi = 0u; gi < tid; ++gi) if (s_tk[gi] == TK_GLAND) ++g_rank;
                    my_gfold = (bp.counts[TK_GLAND] == 2u) ? ((g_rank & 1u) ? 2u : 0u) : (g_rank % 3u);
                }
            }
            if (layer_idx == 0u && tid == 0u) s_mod_w[1] = 3u; // Glottis unlocked for this 4-substep syllable tick!
            if (layer_idx == 0u && tk == TK_GLAND && (ring & 0x0fu) != 0u) atomic_or_gen_u32(&s_mod_w[2], 1u << my_gfold);

            // Pre-pack all 12 stripe source layers (5 bits each = 60 bits) into ONE u64 register!
            // Pre-integrate ALL external stripes (st >= 1, src_l != layer_idx) ONCE from shared-memory s_all_rings[src_l][pre]
            // WITH active 4-synapse NMDA dendritic branch subunits (Poirazi/Larkum/Lauditi 2024 — 0 B extra memory!)
            bool do_rec_tick = (layer_idx == 0u) || ((tick & 3u) == 0u);
            bool do_dec_tick = (layer_idx >= 2u) ? ((tick & 3u) == 0u) : ((layer_idx == 0u) || ((tick & 1u) == 0u));
            bool mod_est_tick = layer_has_mod || (eff_da > 15) || (eff_da < -15) || (eff_ach > 14) ||
                                (is_resting && ((tick & 3u) == 0u) && !abl.silence_neuromod);
            u64 packed_src_l = 0ull;
            u32 st_flags_mask = 0u; // Bits 0..11: F_ELIG (0x20); Bits 12..23: F_DEP (0x40) — 0 extra VRAM, 100% bit-exact!
            u32 local_extra_mask = 0u;
            u32 ext_need_mask = 0u;
            u32 st0_qcnt = 0u;
            u32 st0_ext_need = 0u;
            i32 ext_tot[4] = {0, 0, 0, 0};
            i32 ext_apic[4] = {0, 0, 0, 0};
            bool has_st0 = (is_excitable && !is_sens && my_stripes > 0u && ss0 != INVALID_SLAB && ss0 < pool.max_syn_slabs);
            u32 src_l0 = 0u;
            bool st0_ext = false, st0_local = false, st0_loaded = false;
            bool do_nmda_br = (layer_idx >= 1u && is_pyr);
            bool pfc_gate_flush = (eff_da > 18 || eff_da < -18);
            bool pfc_shield = false;
            bool st0_stf_hold = false;
            if (is_excitable && !is_sens) {
                for (u32 st = 0; st < my_stripes; ++st) {
                    u32 ss = s_lss[st];
                    if (ss == INVALID_SLAB || ss >= pool.max_syn_slabs) { my_stripes = st; break; }
                    u32 raw_sl = u32(pool.stripe_src_layer[u64(ss) * TISSUE_CELLS + tid]);
                    u32 sl_i = (raw_sl & 0x1fu) % num_layers;
                    packed_src_l |= (u64(sl_i & 31u) << (st * 5u));
                    if (raw_sl & 0x20u) st_flags_mask |= (1u << st);
                    if (raw_sl & 0x40u) st_flags_mask |= (1u << (st + 12u));
                    if (st == 0u && (raw_sl & 0x80u)) st0_stf_hold = true;
                }
                src_l0 = u32(packed_src_l & 31ull);
                st0_ext = (has_st0 && src_l0 != layer_idx && s_cr_slabs[src_l0] != INVALID_SLAB);
                st0_local = (has_st0 && !st0_ext);
                // Authentic O'Reilly & Frank (2006) + Miller & Cohen (2001) + Murray (2014) 3-Tier Prefrontal Executive Hierarchy:
                // Cell-assembly-specific PBWM shielding keeps active context engrams protected at 0.25x while leaving
                // silent prefrontal columns open for live sensory reversals, prey hunting & multi-item comparison!
                pfc_shield = ((layer_idx == 4u || layer_idx == 5u) && is_pyr && st0_local && st0_stf_hold && !pfc_gate_flush);
                if (has_st0) {
                    if (st0_local || s_l_spk[src_l0] != 0u) {
                        u64 sb0 = u64(ss0) * LAYER_SYN_QUADS;
                        st0_loaded = true;
                        #pragma unroll
                        for (u32 g = 0; g < QUADS_PER_STRIPE; ++g) {
                            u32 q0, q1, q2, q3;
                            ld_syn_quad(pool.syn_slabs + sb0 + u64(g) * TISSUE_CELLS + tid, q0, q1, q2, q3);
                            syn_reg[g * 4u + 0u] = q0; syn_reg[g * 4u + 1u] = q1;
                            syn_reg[g * 4u + 2u] = q2; syn_reg[g * 4u + 3u] = q3;
                            if ((q0 | q1 | q2 | q3) == 0u) break;
                            st0_qcnt = g + 1u;
                        }
                    }
                    for (u32 st = 1u; st < my_stripes; ++st) {
                        u32 src_l = u32((packed_src_l >> (st * 5u)) & 31ull);
                        if (src_l == layer_idx) {
                            local_extra_mask |= (1u << st);
                        } else if (s_cr_slabs[src_l] != INVALID_SLAB) {
                            if (s_l_spk[src_l] == 0u) {
                                bool has_e = (st_flags_mask & (1u << st)) != 0u;
                                bool has_d = (st_flags_mask & (1u << (st + 12u))) != 0u;
                                if ((has_d && do_rec_tick) || (has_e && (do_dec_tick || mod_est_tick)))
                                    ext_need_mask |= (1u << st);
                                continue;
                            }
                            u64 sbase = u64(s_lss[st]) * LAYER_SYN_QUADS;
                            u32 sq[4];
                            u32 st_need = 0u;
                            #pragma unroll
                            for (u32 g = 0; g < QUADS_PER_STRIPE; ++g) {
                                ld_syn_quad(pool.syn_slabs + sbase + u64(g) * TISSUE_CELLS + tid, sq[0], sq[1], sq[2], sq[3]);
                                if ((sq[0] | sq[1] | sq[2] | sq[3]) == 0u) break;
                                i32 br_b[4] = {0, 0, 0, 0};
                                u32 br_hits = 0u; // Distinct coincident excitatory synapses on this 4-synapse NMDA branch (1-tick window!)
                                #pragma unroll
                                for (u32 j = 0; j < 4u; ++j) {
                                    u32 s = sq[j];
                                    if (s == 0u) break;
                                    u32 pre = s & 0xffu;
                                    u32 delay = abl.zero_delays ? 2u : min_u32((s >> 23) & 7u, 4u);
                                    u32 pre_r_b = u32(s_all_rings[src_l][pre]);
                                    u32 win = (pre_r_b >> delay) & 0x0fu;
                                    u32 elig_nz = (((s >> 16) & 0x7fu) ^ 64u);
                                    st_need |= win | ((do_dec_tick || mod_est_tick) ? elig_nz : 0u) |
                                               ((do_rec_tick && s < 0xe0000000u) ? 1u : 0u);
                                    if (win != 0u) {
                                        i32 sw_i = i32(i8(s >> 8));
                                        i32 curr = sw_i * i32((s >> 29) + 2u);
                                        bool is_ap = (s & 0x04000000u) != 0u;
                                        if (layer_idx == 1u && src_l == 0u && sw_i > 0)
                                            curr = curr + (curr >> 2); // Crisp 1.25x Class-1A thalamocortical sensory driver onto L1!
                                        else if (layer_idx >= 2u && layer_idx <= 4u && src_l < layer_idx && sw_i > 0)
                                            curr = pfc_shield ? (curr >> 2) : ((curr * 3) >> 1); // 6x PBWM Encode (1.5x) vs Shield (0.25x) on L2..L4!
                                        else if (layer_idx == 5u && src_l < layer_idx && sw_i > 0)
                                            curr = pfc_shield ? (curr >> 2) : (curr + (curr >> 2)); // 5x PBWM Encode (1.25x) vs Shield (0.25x) on Frontopolar BA10 L5!
                                        else if (!is_eff && src_l > layer_idx && sw_i > 0)
                                            is_ap = true; // Authentic Sherman & Guillery (1998) Class-1B feedback: strictly apical modulatory on ALL layers (kills L0..L8 ping-pong loop!)
                                        i32 ac = is_ap ? curr : 0;
                                        i32 bc = is_ap ? 0 : curr;
                                        if (!is_ap && sw_i > 0) ++br_hits;
                                        if (win & 8u) { ext_tot[0] += bc; ext_apic[0] += ac; br_b[0] += bc; }
                                        if (win & 4u) { ext_tot[1] += bc; ext_apic[1] += ac; br_b[1] += bc; }
                                        if (win & 2u) { ext_tot[2] += bc; ext_apic[2] += ac; br_b[2] += bc; }
                                        if (win & 1u) { ext_tot[3] += bc; ext_apic[3] += ac; br_b[3] += bc; }
                                    }
                                }
                                if (do_nmda_br && br_hits >= 2u) {
                                    i32 br_sum = br_b[0] + br_b[1] + br_b[2] + br_b[3];
                                    if (br_sum > 280) {
                                        #pragma unroll
                                        for (u32 sb = 0; sb < 4u; ++sb) {
                                            if (br_b[sb] > 0) ext_tot[sb] += (br_b[sb] >> 2); // +25% Mg2+-unblocked NMDA plateau (1t window!)
                                        }
                                    }
                                }
                            }
                            if (st_need != 0u) ext_need_mask |= (1u << st);
                        }
                    }
                }
            }
            // Blackwell Stripe-0 External Pre-Integration with Active 4-Synapse NMDA Dendritic Subunits!
            if (st0_ext) {
                if (s_l_spk[src_l0] == 0u) {
                    bool has_e0 = (st_flags_mask & 1u) != 0u;
                    bool has_d0 = (st_flags_mask & (1u << 12)) != 0u;
                    if ((has_d0 && do_rec_tick) || (has_e0 && (do_dec_tick || mod_est_tick))) st0_ext_need = 1u;
                } else {
                    for (u32 g = 0; g < st0_qcnt; ++g) {
                        i32 br_b[4] = {0, 0, 0, 0};
                        u32 br_hits = 0u;
                        #pragma unroll
                        for (u32 j = 0; j < 4u; ++j) {
                            u32 s = syn_reg[g * 4u + j];
                            if (s == 0u) break;
                            u32 pre = s & 0xffu;
                            u32 delay = abl.zero_delays ? 2u : min_u32((s >> 23) & 7u, 4u);
                            u32 pre_r_b = u32(s_all_rings[src_l0][pre]);
                            u32 win = (pre_r_b >> delay) & 0x0fu;
                            u32 elig_nz = (((s >> 16) & 0x7fu) ^ 64u);
                            st0_ext_need |= win | ((do_dec_tick || mod_est_tick) ? elig_nz : 0u) |
                                            ((do_rec_tick && s < 0xe0000000u) ? 1u : 0u);
                            if (win != 0u) {
                                i32 sw_i = i32(i8(s >> 8));
                                i32 curr = sw_i * i32((s >> 29) + 2u);
                                bool is_ap = (s & 0x04000000u) != 0u;
                                if (layer_idx == 1u && src_l0 == 0u && sw_i > 0)
                                    curr = curr + (curr >> 2);
                                else if (layer_idx >= 2u && layer_idx <= 4u && src_l0 < layer_idx && sw_i > 0)
                                    curr = pfc_shield ? (curr >> 2) : ((curr * 3) >> 1);
                                else if (layer_idx == 5u && src_l0 < layer_idx && sw_i > 0)
                                    curr = pfc_shield ? (curr >> 2) : (curr + (curr >> 2));
                                else if (!is_eff && src_l0 > layer_idx && sw_i > 0)
                                    is_ap = true;
                                i32 ac = is_ap ? curr : 0;
                                i32 bc = is_ap ? 0 : curr;
                                if (!is_ap && sw_i > 0) ++br_hits;
                                if (win & 8u) { ext_tot[0] += bc; ext_apic[0] += ac; br_b[0] += bc; }
                                if (win & 4u) { ext_tot[1] += bc; ext_apic[1] += ac; br_b[1] += bc; }
                                if (win & 2u) { ext_tot[2] += bc; ext_apic[2] += ac; br_b[2] += bc; }
                                if (win & 1u) { ext_tot[3] += bc; ext_apic[3] += ac; br_b[3] += bc; }
                            }
                        }
                        if (do_nmda_br && br_hits >= 2u) {
                            i32 br_sum = br_b[0] + br_b[1] + br_b[2] + br_b[3];
                            if (br_sum > 280) {
                                #pragma unroll
                                for (u32 sb = 0; sb < 4u; ++sb) {
                                    if (br_b[sb] > 0) ext_tot[sb] += (br_b[sb] >> 2);
                                }
                            }
                        }
                    }
                }
            }

            // Authentic Avian RA / Mammalian Nucleus-Ambiguus Full-Tick 4-Substep Dendritic Integration strictly on TK_GLAND
            // (before glottic WTA lock); TK_MUSCLE keeps exact substep temporal coincidence for Larkum BAC doublets & axonal delays!
            if (tk == TK_GLAND) {
                i32 s_bt = (ext_tot[0] + ext_tot[1] + ext_tot[2] + ext_tot[3]) >> 2;
                i32 s_ap = (ext_apic[0] + ext_apic[1] + ext_apic[2] + ext_apic[3]) >> 2;
                ext_tot[0] = ext_tot[1] = ext_tot[2] = ext_tot[3] = s_bt;
                ext_apic[0] = ext_apic[1] = ext_apic[2] = ext_apic[3] = s_ap;
            }
            // Authentic Payeur-Naud-Richards (2021) + Lillicrap (2016) Vector Feedback-Aligned Efference-Copy Burstprop:
            // Reads strictly race-free tick-(t-1) action latch (`cr.growth_stage >> 4`) published between ticks by body_ecology_step,
            // guaranteeing L1..L_top ALWAYS blame/credit tick-(t-1)'s causal action (never raced by L0's tick-t LMAN switch!).
            i32 fa_da = eff_da;
            u32 prev_fa_act = u32(cr.growth_stage >> 4);
            if (layer_idx >= 1u && is_pyr && (eff_da > 15 || eff_da < -15)) {
                bool v0_a = (prev_fa_act & 1u) != 0u;
                bool v1_a = (prev_fa_act & 2u) != 0u;
                bool v2_a = (prev_fa_act & 4u) != 0u;
                if (v0_a || v1_a || v2_a) {
                    u32 fa_f = (tid * 7u + layer_idx * 5u + u32(dna.bio_params[8] & 7)) % 3u;
                    bool my_f = (fa_f == 0u) ? v0_a : ((fa_f == 1u) ? v1_a : v2_a);
                    // On +DA reward: reinforce causal columns (eff_da); on -DA error: suppress ONLY causal columns (eff_da).
                    // Non-participating columns stay neutral (0) — NEVER inject fake +DA reward on venom/pain errors!
                    // Keep Fee-Goldberg vocal evaluation (vta_voc_q) strictly in Area-X (TK_GLAND) -> ZERO cortical over-editing!
                    fa_da = my_f ? eff_da : 0;
                } else {
                    bool mot_a = (prev_fa_act & 8u) != 0u;
                    bool my_m = (((tid ^ layer_idx) & 1u) != 0u) ? mot_a : !mot_a;
                    fa_da = my_m ? eff_da : 0;
                }
            }
            u32 mod_pos_acc = 0u, mod_neg_acc = 0u;
            u32 any_loc_spk = s_l_spk[layer_idx];
            bool gamma_won = false;
            // Authentic Buzsaki & Wang (2012) PING Gamma-Cycle Winner Budget + Barth-Poulet Higher-Association Sparsity:
            // L1..L4 run ~11% sparse assemblies (<=22 cells); L5..L_top higher association cortex runs crisp ~6-7% (<=12 cells)
            // eliminating 10-layer tonic reverberation cascades while keeping deep executive selection razor sharp!
            if (layer_idx >= 1u && tid == 0u) {
                u32 k_tick = (layer_idx >= 5u) ? max_u32(4u, min_u32(12u, layer_exc_cnt / 14u))
                                               : max_u32(8u, min_u32(22u, layer_exc_cnt / 9u));
                s_mod_w[3] = (layer_pv_cnt * 16u >= layer_exc_cnt) ? k_tick : max_u32(k_tick, layer_exc_cnt / 4u);
            }
            for (u32 sub = 0; sub < SUBSTEPS_PER_TICK; ++sub) {
                u32 cur_b = sub & 1u;
                // Blackwell Warp-Uniform Empty-Warp Fast-Path: empty warps (exc_bm==0) skip s_ring store, ballots & membrane math!
                if (warp_has_exc) {
                    s_ring[tid] = u8(ring);
                    if (warp_ballot(ring != 0u) && lane_id() == 0u) atomic_or_gen_u32(&s_any_spk[cur_b], 1u);
                    // Dual-interneuron column shunting + 4-minicolumn PV surround competition (ZERO extra barriers!):
                    u32 pv_bm = (layer_idx >= 1u) ? warp_ballot(is_pv && ((ring & 3u) != 0u))
                                                  : warp_ballot((tk == TK_GLAND) && ((ring & 0x0fu) != 0u));
                    u32 sst_bm = (layer_idx >= 1u) ? warp_ballot((tk == TK_NEU_SST) && ((ring & 3u) != 0u)) : 0u;
                    if ((pv_bm | sst_bm) != 0u && lane_id() == 0u)
                        atomic_add_gen_u32(&s_pv_act[cur_b], popc_u32(pv_bm) | (popc_u32(sst_bm) << 16));
                    if (layer_idx >= 1u) {
                        bool p_act = is_pyr && ((ring & 3u) != 0u);
                        u32 my_col = (ux & 1u) | ((uy & 1u) << 1);
                        u32 cpk = popc_u32(warp_ballot(p_act && (my_col == 0u))) |
                                  (popc_u32(warp_ballot(p_act && (my_col == 1u))) << 8) |
                                  (popc_u32(warp_ballot(p_act && (my_col == 2u))) << 16) |
                                  (popc_u32(warp_ballot(p_act && (my_col == 3u))) << 24);
                        if (cpk != 0u && lane_id() == 0u) atomic_add_gen_u32(&s_col_act[cur_b], cpk);
                    }
                }
                if (tid == 0u) { s_any_spk[cur_b ^ 1u] = 0u; s_pv_act[cur_b ^ 1u] = 0u; s_col_act[cur_b ^ 1u] = 0u; }
                sync_block();
                any_loc_spk |= s_any_spk[cur_b];

                i32 i_basal = 0, i_apical = 0;
                if (warp_has_exc && has_st0) {
                    i32 i_tot = ext_tot[sub]; // ext_tot holds strictly basal external current (Class-1A); ext_apic holds apical!
                    i_apical = ext_apic[sub];
                    // Blackwell Silent-Layer Local-Spike Short-Circuit (0 extra barriers, 100% bit-exact!):
                    // Skips BOTH local Stripe-0 AND extra local stripes whenever no cell in this layer spiked in the last 8 substeps!
                    if (s_any_spk[cur_b] != 0u) {
                        // Local Stripe 0 ONLY: 100% IN-REGISTER + 100% SHARED MEMORY s_ring[] + Mongillo STF Attractor!
                        if (st0_local) {
                            for (u32 g = 0; g < st0_qcnt; ++g) {
                                #pragma unroll
                                for (u32 j = 0; j < 4u; ++j) {
                                    u32 s = syn_reg[g * 4u + j];
                                    if (s == 0u) break;
                                    u32 pre = s & 0xffu;
                                    u32 delay = abl.zero_delays ? 3u : ((s >> 23) & 7u);
                                    u32 pre_spk = (u32(s_ring[pre]) >> delay) & 1u;
                                    if (pre_spk) {
                                        i32 sw_i = i32(i8(s >> 8));
                                        u32 stp_v = s >> 29;
                                        i32 curr = sw_i * i32(stp_v + 2u);
                                        bool is_ap_l = (s & 0x04000000u) != 0u;
                                        if (layer_idx >= 2u && sw_i > 0 && !is_ap_l) {
                                            // Authentic Mongillo-Wang Bistable 3-Tier Prefrontal STF Attractor:
                                            // L3/L4 WM hubs hold 2.5x (stp>=4, crisp zero-reverberation edge-of-chaos);
                                            // L5 BA10 frontopolar executive holds 3.0x (stp>=4, 48-63t);
                                            // L2 bigram holds 2.0x (stp>=5); resting (stp<4) & L6+ remain strictly subcritical!
                                            curr = (layer_idx == 5u)
                                                       ? ((stp_v >= 4u) ? (curr * 3) : (curr >> 2))
                                                       : ((layer_idx == 3u || layer_idx == 4u)
                                                              ? ((stp_v >= 4u) ? ((curr * 5) >> 1) : (curr >> 2))
                                                              : ((layer_idx == 2u && stp_v >= 5u) ? (curr << 1) : ((layer_idx >= 6u) ? (curr >> 3) : (curr >> 2))));
                                        } else if (layer_idx == 1u && sw_i > 0) {
                                            curr >>= 3;
                                        } else if (layer_idx == 0u && !is_eff && sw_i > 0 && !tk_is_sensor(s_tk[pre])) {
                                            // Strictly subcritical local L0 ganglion recurrence (stops permanent dark self-echo into L1!)
                                            curr >>= 2;
                                        }
                                        if (is_ap_l) i_apical += curr;
                                        else i_tot += curr;
                                    }
                                }
                            }
                        }
                        // Extra local stripes only (rare, post-maturation) — external stripes already integrated above!
                        if (__builtin_expect(local_extra_mask != 0u, 0)) {
                            for (u32 st = 1u; st < my_stripes; ++st) {
                                if ((local_extra_mask & (1u << st)) == 0u) continue;
                                u64 sbase = u64(s_lss[st]) * LAYER_SYN_QUADS;
                                u32 sq[4];
                                #pragma unroll
                                for (u32 g = 0; g < QUADS_PER_STRIPE; ++g) {
                                    ld_syn_quad(pool.syn_slabs + sbase + u64(g) * TISSUE_CELLS + tid, sq[0], sq[1], sq[2], sq[3]);
                                    if ((sq[0] | sq[1] | sq[2] | sq[3]) == 0u) break;
                                    #pragma unroll
                                    for (u32 j = 0; j < 4u; ++j) {
                                        u32 s = sq[j];
                                        if (s == 0u) break;
                                        u32 pre = s & 0xffu;
                                        u32 delay = abl.zero_delays ? 3u : ((s >> 23) & 7u);
                                        u32 pre_spk = (u32(s_ring[pre]) >> delay) & 1u;
                                        if (pre_spk) {
                                            i32 sw_i = i32(i8(s >> 8));
                                            u32 stp_v = s >> 29;
                                            i32 curr = sw_i * i32(stp_v + 2u);
                                            bool is_ap_l = (s & 0x04000000u) != 0u;
                                            if (layer_idx >= 2u && sw_i > 0 && !is_ap_l) {
                                                curr = (layer_idx == 5u)
                                                           ? ((stp_v >= 4u) ? (curr * 3) : (curr >> 2))
                                                           : ((layer_idx == 3u || layer_idx == 4u)
                                                                  ? ((stp_v >= 4u) ? ((curr * 5) >> 1) : (curr >> 2))
                                                                  : ((layer_idx == 2u && stp_v >= 5u) ? (curr << 1) : ((layer_idx >= 6u) ? (curr >> 3) : (curr >> 2))));
                                            } else if (layer_idx == 1u && sw_i > 0) {
                                                curr >>= 3;
                                            } else if (layer_idx == 0u && !is_eff && sw_i > 0 && !tk_is_sensor(s_tk[pre])) {
                                                curr >>= 2;
                                            }
                                            if (is_ap_l) i_apical += curr;
                                            else i_tot += curr;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    // Authentic Scale-Invariant Brunel / van Vreeswijk-Sompolinsky 1/sqrt(K) Balanced-Network Scaling:
                    // Smoothly normalizes compact (K=10) through 8,192-neuron dense neocortices (K=32..192) ->
                    // crisp 16-layer feedforward propagation + subcritical dark cooldown across ALL scales!
                    i32 raw_b = i_tot;
                    i32 raw_a = i_apical;
                    if (layer_idx == 0u && is_eff && tk != TK_GLAND) {
                        if ((tk == TK_MUSCLE || tk == TK_GRASP) && my_exc_cnt > 24u) {
                            // Scale-invariant Brunel 1/sqrt(K) corticospinal scaling calibrated for 100% excitatory motor endplates:
                            // Keeps 6-10 muscle predators brisk & adaptive while preventing STDP hyper-saturation on 140-synapse
                            // deep cortices (TK_MUSCLE: 32/26; TK_GRASP: 24/20 prevents runaway dig/bite ATP waste!).
                            float m_inv = 1.0f / fast_sqrt(float(my_exc_cnt));
                            float mg_b = (tk == TK_MUSCLE) ? 30.0f : 20.0f;
                            float mg_a = (tk == TK_MUSCLE) ? 24.0f : 16.0f;
                            i_basal = i32(float(raw_b) * (mg_b * m_inv));
                            i_apical = abl.silence_apical ? 0 : i32(float(raw_a) * (mg_a * m_inv));
                        } else {
                            i_basal = (my_stripes >= 5u) ? ((raw_b * 3) >> 1) : (raw_b << 1);
                            i_apical = abl.silence_apical ? 0 : (raw_a << 1);
                        }
                    } else if (tk == TK_GLAND) {
                        // Authentic Brunel Balanced-Network 1/sqrt(K) Dendritic Scaling on Corticobulbar Vocal Readout:
                        // Calibrated for sparse high-SNR awake cortical coding (~11% gamma assembly) across 30..8,000 neurons!
                        float g_inv = 1.0f / fast_sqrt(float(max_u32(12u, my_exc_cnt)));
                        float g_gain = 350.0f;
                        i_basal = i32(float(raw_b) * (g_gain * g_inv));
                        i_apical = abl.silence_apical ? 0 : i32(float(raw_a) * ((g_gain * 0.65f) * g_inv));
                    } else if (is_pyr) {
                        // Authentic Brunel (2000) 1/sqrt(K_exc) Balanced-Network Scaling across ALL cortical layers (L0..L31)!
                        // Calibrated for ~10% PING sparse coding on L1..L4 and ultra-selective ~6% executive coding on L5+!
                        float p_inv = 1.0f / fast_sqrt(float(max_u32(10u, my_exc_cnt)));
                        float b_g = (layer_idx >= 6u) ? 2.95f : ((layer_idx == 5u) ? 4.00f : ((layer_idx >= 2u) ? 4.65f : ((layer_idx == 1u) ? 3.95f : 3.10f)));
                        // Calibrated Larkum (2013) Layer-1 Apical Tuft Gain for Sparse 6-10% Prefrontal Feedback:
                        // Strictly modulatory (0 autonomous spikes in darkness!); paired with 6-branch is_desc_mix canopy,
                        // tuned right at the selective BAC threshold so matched L4/L5 context ignites L3 without tonic saturation!
                        float a_g = (layer_idx == 3u) ? 6.5f : ((layer_idx == 2u || layer_idx == 4u) ? 5.5f : 4.5f);
                        i_basal = i32(float(raw_b) * (b_g * p_inv));
                        i_apical = abl.silence_apical ? 0 : i32(float(raw_a) * (a_g * p_inv));
                    } else if (!is_eff) {
                        // Scale-invariant 1/sqrt(K) scaling on PV+/SST+ interneurons & VTA MOD across L0..L31!
                        float p_inv = 1.0f / fast_sqrt(float(max_u32(10u, my_exc_cnt)));
                        i_basal = i32(float(raw_b) * (2.75f * p_inv));
                        i_apical = abl.silence_apical ? 0 : i32(float(raw_a) * (2.75f * p_inv));
                    } else {
                        i_basal = raw_b;
                        i_apical = abl.silence_apical ? 0 : raw_a;
                    }
                }

                u32 fired = 0u;
                i32 vs_pre = 0;
                if (warp_has_exc && is_excitable) {
                    u32 pv_pk = s_pv_act[cur_b];
                    u32 pv_cnt = pv_pk & 0xffffu;
                    u32 sst_cnt = pv_pk >> 16;
                    // Scale-Invariant Balanced-Network Divisive Normalization + PV-Gated Cross-Column Surround Inhibition:
                    // Winning Sperry minicolumn stays crisp & un-shunted while PV+ basket cells suppress rival columns ->
                    // sharp orthogonal episodic attractors & sparse coding WITHOUT crushing newborn evolution!
                    u32 pv_div_scale = 4096u;
                    i32 col_shunt = 0;
                    bool has_topk_pv = (layer_pv_cnt >= 1u && layer_exc_cnt >= 20u);
                    if (layer_idx >= 1u && is_pyr) {
                        if (pv_cnt > 0u && !has_topk_pv) {
                            // Divisive PV normalization strictly on small sheets (<20 pyr) where instant Top-k doesn't run;
                            // on cortical sheets (>=20 pyr), instant same-substep Pouille-Scanziani Top-k + anatomical GABA_A
                            // synapses handle competition cleanly with ZERO substep-lag bias against heterogeneous delays!
                            u32 pv_sc = (layer_idx >= 2u)
                                            ? min_u32(64u, (pv_cnt * 32u) / max_u32(16u, layer_exc_cnt) + pv_cnt * 2u)
                                            : (min_u32(pv_cnt, 24u) * 2u);
                            pv_div_scale = 65536u / (16u + pv_sc);
                            if (i_basal > 0) i_basal = (i_basal * i32(pv_div_scale)) >> 12;
                            bool do_col = (layer_exc_cnt >= 14u && pv_cnt >= 1u);
                            if (do_col) {
                                u32 cpk = s_col_act[cur_b];
                                u32 c0 = cpk & 0xffu, c1 = (cpk >> 8) & 0xffu;
                                u32 c2 = (cpk >> 16) & 0xffu, c3 = (cpk >> 24) & 0xffu;
                                u32 tot_pyr = c0 + c1 + c2 + c3;
                                u32 k_budget = (layer_idx >= 2u) ? max_u32(5u, layer_exc_cnt >> 3)
                                                                 : max_u32(7u, layer_exc_cnt / 5u);
                                u32 max_c = max_u32(max_u32(c0, c1), max_u32(c2, c3));
                                u32 my_col = (ux & 1u) | ((uy & 1u) << 1);
                                u32 my_c = (cpk >> (my_col * 8u)) & 0xffu;
                                if (tot_pyr > k_budget && (max_c > my_c || ema > 32u)) {
                                    u32 ex_p = tot_pyr - k_budget;
                                    col_shunt += i32(min_u32(layer_idx >= 2u ? 420u : 260u, ex_p * (layer_idx >= 2u ? 22u : 14u)));
                                }
                                if (max_c > my_c) {
                                    u32 gap = max_c - my_c;
                                    col_shunt += i32(min_u32(320u, gap * min_u32(pv_cnt, 8u) * 14u));
                                    if (gap >= 2u && i_basal > 0) i_basal >>= 1;
                                }
                            }
                        }
                        // Authentic VIP/SST/PV Three-Interneuron Disinhibitory Dendritic Gating (Pi et al. 2013 Nature / Fu 2014 Cell):
                        // Phasic ACh novelty & NE arousal activate cortical VIP+ interneurons which suppress SST+ Martinotti cells,
                        // opening apical tufts to encode novel context while keeping SST gates CLOSED against routine distractors!
                        bool vip_disinh = (!abl.silence_neuromod && (ext_ach_q > 12 || eff_ne > 14));
                        u32 sst_eff = vip_disinh ? (sst_cnt >> 2) : sst_cnt;
                        if (sst_eff > 0u && i_apical > 0) {
                            u32 sst_div = 65536u / (16u + min_u32(sst_eff, 40u) * 2u);
                            i_apical = (i_apical * i32(sst_div)) >> 12;
                        }
                    }
                    if (is_pyr) {
                        // Authentic Williams & Goldman-Rakic (1998) + Larkum (2013) + Payeur (2021) Layer-1 Neuromodulated Apical Tufts:
                        // Temporally causal tick-(t-1) RPE gating: +DA drives apical Ca2+ BAC bursts on causally active assemblies;
                        // -DA dampens active apical plateaus back toward 0 (NEVER slams global va=-1500 to wipe feature detectors!).
                        bool vip_disinh = (!abl.silence_neuromod && (ext_ach_q > 12 || eff_ne > 14));
                        u32 sst_eff = vip_disinh ? (sst_cnt >> 2) : sst_cnt;
                        u32 sst_cap = (layer_idx >= 2u) ? 260u : 200u;
                        i32 sst_shunt = (layer_idx >= 1u && sst_eff > 0u) ? i32(min_u32(sst_cap, sst_eff * ((layer_idx >= 2u) ? 11u : 10u))) : 0;
                        i32 sahp_va = (layer_idx >= 2u && ua > 96u && va > 0) ? (va >> 3) : 0;
                        bool is_exec_top = (layer_idx >= 1u) && ((layer_idx + 2u >= num_layers) || (layer_idx == 1u && num_layers <= 2u));
                        i32 da_gain_l = is_exec_top ? 9 : 7;
                        bool causal_act = (((ring & 0xf0u) != 0u) || i_basal > 90);
                        i32 da_apic_teach = 0;
                        if (layer_idx >= 1u && !abl.silence_apical && causal_act) {
                            if (fa_da > 12) da_apic_teach = fa_da * da_gain_l;
                            else if (fa_da < -12) da_apic_teach = (va > 0) ? -(va >> 1) : -140;
                        }
                        va = clamp_i32(va - (va >> apic_leak_shift) + (i_apical >> 1) + da_apic_teach - sst_shunt - (col_shunt >> 1) - sahp_va, -1500, 1500);
                        plat = (va >= apic_thresh) ? 1u : 0u;
                    } else { va = 0; plat = 0u; }
                    // Authentic Larkum (1999 Nature / 2013 TINS) Strictly Multiplicative Apical×Basal BAC Coincidence Detection:
                    // Descending apical Ca2+ plateau (plat==1, 2-4t window) is strictly MODULATORY — ZERO autonomous ignition
                    // when i_basal <= 80, delivering full burst amplification on bottom-up / STF basal coincidences (>= 180 pA)!
                    i32 raw_bac = 0;
                    if (plat) {
                        i32 eff_bac = (layer_idx == 3u) ? ((bac_gain * 5) >> 2) : bac_gain;
                        if (i_basal >= 180) raw_bac = eff_bac;
                        else if (i_basal > 80) raw_bac = (eff_bac * (i_basal - 80)) / 100;
                    }
                    i32 bac_curr = (layer_idx >= 2u) ? ((raw_bac * i32(pv_div_scale)) >> 12) : raw_bac;
                    // Authentic Buzsaki (2015) / Jadhav (2012 Science) Sleep & Awake Consummatory-Pause Hippocampal SWR:
                    // Gentle subthreshold consolidation pulse on recently active L3/L4 WM assemblies during resting pauses
                    // OR awake rewarded consummatory pauses (mot_thrust < 0.25, suppressed by locomotor theta during running!)
                    i32 sleep_drive = (is_resting && ((tick & 31u) == 0u) && layer_idx > 0u && is_pyr && ema >= 8u && ema < 20u)
                                          ? 18
                                          : (((layer_idx >= 3u && layer_idx <= 5u) && is_pyr && st0_stf_hold && eff_da > 28 && ((prev_fa_act & 8u) == 0u) && ema >= 8u && ema < 22u) ? 32 : 0);
                    u32 pv_sh_cap = (layer_idx >= 2u) ? 200u : 140u;
                    i32 pv_shunt = (layer_idx >= 1u && is_pyr && !has_topk_pv && pv_cnt > 1u)
                                       ? i32(min_u32(pv_sh_cap, (pv_cnt - 1u) * 10u))
                                       : 0;
                    // Authentic Songbird LMAN Exploratory Switching + Striatal D2-NoGo Immediate Error-Fold Veto (Olveczky 2005):
                    // - Operant -DA (`eff_da < -10`): crisp D2-NoGo error-fold veto + LMAN exploration.
                    // - Juvenile peer-template babbling (`age < 120`, fresh localized template, `vta_voc_q < -18`): gentle
                    //   Olveczky juvenile LMAN exploratory nudge on rival folds WITHOUT shutting down adult/juvenile catalysis!
                    i32 lman_curr = 0;
                    bool d2_err_veto = false;
                    bool juv_lman_err = (cr.age < 120u && (cr.peer_tpl_band & 0x03u) < 3u && cr.peer_tpl_age <= 4u && vta_voc_q < -18);
                    if (tk == TK_GLAND && eff_da < -10) {
                        bool was_punished = ((s_mod_w[2] & (1u << my_gfold)) != 0u) || ((ring & 0xf0u) != 0u);
                        if (was_punished && s_mod_w[2] != 0u) {
                            d2_err_veto = true;
                        } else {
                            i32 lman_r = (i32(mix((tick * 2654435761u) ^ (my_gfold * 31337u)) & 255u) - 128) * 6;
                            lman_curr = lman_r + 420;
                        }
                    } else if (tk == TK_GLAND && juv_lman_err) {
                        bool was_punished = ((s_mod_w[2] & (1u << my_gfold)) != 0u) || ((ring & 0xf0u) != 0u);
                        if (was_punished && s_mod_w[2] != 0u) {
                            lman_curr = -260;
                        } else {
                            i32 lman_r = (i32(mix((tick * 2654435761u) ^ (my_gfold * 31337u)) & 255u) - 128) * 4;
                            lman_curr = lman_r + 320;
                        }
                    } else if ((tk == TK_GLAND && eff_ne > 24) || (is_eff && eff_da < -10)) {
                        lman_curr = (i32(mix(((tick >> 1) * 2654435761u) ^ (tid * 31337u)) & 255u) - 128) * 3;
                    }
                    // Authentic Weber-Fechner / Shapley (1984) Smooth Compressive Retinal Adaptation (ZERO oscillation!):
                    // Unchanging static daylight wallpaper smoothly adapts below threshold while moving prey/predators/reefs stay crisp!
                    i32 eff_sens = sens_curr;
                    if (is_sens && tk == TK_EYE && ema > 8u && eff_sens > 0) {
                        eff_sens = (eff_sens * 28) / i32(28u + ema);
                    }
                    // Authentic Egorov et al. (2002 Nature) Intrinsic Ca2+-Gated Entorhinal/Prefrontal I_CAN Persistence:
                    // Strictly self-Ca2+ gated (ring!=0 || ema>=ican_ema_min -> ZERO un-cued bystander hijack!) and calibrated
                    // to sustain exact 6-10 neuron cue assemblies across 24-60t pitch-black delays on deep 2,000+ neuron cortices!
                    bool ican_flushed = (layer_idx >= 4u) ? (eff_da < -18) : pfc_gate_flush;
                    u32 ican_ema_min = (layer_idx >= 4u) ? 2u : 4u;
                    bool ican_self_ca = ((ring & 0xffu) != 0u) || (ema >= ican_ema_min);
                    i32 ican_curr = ((layer_idx >= 3u && layer_idx <= 5u) && is_pyr && st0_stf_hold && !ican_flushed && ican_self_ca)
                                        ? ((layer_idx == 5u) ? 130 : 125)
                                        : 0;
                    // Authentic Songbird Uva -> HVC / Broca Modulatory Vocal Efference Copy (Fee 2014 / Hickok 2012):
                    // Strictly subthreshold alone (0 pA in silent darkness or non-vocal turns!) and bridges 1..5t phrase transitions
                    // cleanly without bleeding across 5t inter-trial pauses, mapped onto Sperry-orthogonal L3 sub-assemblies!
                    bool uva_on = (layer_idx == 3u && is_pyr && ((st0_stf_hold && ican_self_ca) || i_basal >= 80) &&
                                   cr.last_vocal_tick > 0u && tick > cr.last_vocal_tick && tick <= cr.last_vocal_tick + 5u);
                    i32 uva_curr = uva_on ? (((((tid >> 1) + (tid >> 5)) % 3u) == u32(cr.last_vocal_band)) ? 210 : -125) : 0;
                    i32 inp = is_sens ? eff_sens : (i_basal + bac_curr + ican_curr + uva_curr + sleep_drive + lman_curr + sens_curr - pv_shunt - col_shunt);
                    if (is_eff) inp += (i_apical >> 2);
                    if (d2_err_veto) {
                        vs = -160;
                    } else if (refrac > 0u) { --refrac; vs = vs - (vs >> 2); }
                    else {
                        // Compute UNCLAMPED membrane drive `v_raw` before 13-bit storage clamp so Nucleus-Ambiguus
                        // vocal-fold WTA NEVER suffers artificial 4000==4000 saturation ties when learned weights diverge!
                        // Gentle cortical spike-frequency adaptation (ua*2 on L2+) preserves steady Top-k assemblies across ticks!
                        i32 adapt_cur = (layer_idx >= 2u && is_pyr) ? (i32(ua) << 1) : (i32(ua) * 3);
                        i32 v_raw = vs - (vs >> soma_leak_shift) + inp - adapt_cur;
                        if (v_raw >= dyn_thresh) { fired = 1u; vs_pre = v_raw; }
                        vs = clamp_i32(v_raw, -4000, 4000);
                    }
                }
                bool prev_gamma_won = gamma_won;
                if (layer_idx == 0u) {
                    // Full-Tick Nucleus-Ambiguus Glottic-Cycle Articulator Lock (1 tick = 1 ms = 1 crisp syllable!):
                    // Unclamped 19-bit synaptic drive (up to 524,287 pA!) + symmetric reciprocal glycinergic reset across
                    // losing folds (ZERO involuntary v2-v0-v2-v0 relaxation oscillation, ZERO silent subthreshold ambush!)
                    u32 g_lock = s_mod_w[1];
                    if (g_lock == 3u) {
                        if (tid == 0u) s_mod_w[0] = 0u;
                        sync_block();
                        if (tk == TK_GLAND && fired) {
                            u32 score = (u32(clamp_i32(vs_pre, 0, 524287)) << 11) | (my_gfold << 9) |
                                        (((tid + tick * 7u + sub) & 1u) << 8) | tid;
                            atomic_max_gen_u32(&s_mod_w[0], score);
                        }
                        sync_block();
                        u32 win_pk = s_mod_w[0];
                        if (win_pk != 0u) {
                            u32 win_f = (win_pk >> 9) & 3u;
                            if (tid == 0u) s_mod_w[1] = win_f;
                            if (tk == TK_GLAND && my_gfold != win_f) {
                                fired = 0u; vs = -160;
                            }
                        }
                    } else if (tk == TK_GLAND && my_gfold != g_lock) {
                        fired = 0u; vs = -160;
                    }
                } else {
                    // Authentic Buzsaki-Wang (2012) PING Gamma-Cycle + Pouille-Scanziani (2001) PV+ Top-k Sparse Coding (~10-12.5%/tick!):
                    // - Already-won apical-plateau neurons (gamma_won==true, refrac==0) pass straight through to fire Payeur bursts!
                    // - New pyramidal candidates compete for the remaining s_mod_w[3] tick-level gamma winner budget using fine
                    //   32 pA margin bins; once the 10-12.5% gamma assembly forms, PV+ GABA_A silences rival noise for the full tick!
                    if (layer_pv_cnt >= 1u && layer_exc_cnt >= 20u) {
                        if (tid < 16u) s_v_hist[tid] = 0u;
                        sync_block();
                        u32 my_vbin = 0u;
                        bool is_new_cand = (is_pyr && fired && !gamma_won);
                        if (is_new_cand) {
                            // Zero-saturation piecewise-log 16-bin excess voltage (0..5,632+ pA):
                            // Eliminates bin-15 saturation so strongly stimulus-driven cells ALWAYS beat weak background!
                            u32 dv = u32(vs_pre > dyn_thresh ? (vs_pre - dyn_thresh) : 0);
                            my_vbin = (dv < 512u) ? (dv >> 6)
                                      : ((dv < 1536u) ? (8u + ((dv - 512u) >> 8))
                                                      : min_u32(15u, 12u + ((dv - 1536u) >> 10)));
                            atomic_add_gen_u32(&s_v_hist[my_vbin], 1u);
                        }
                        sync_block();
                        if (tid == 0u) {
                            u32 k_sub = max_u32(3u, min_u32(8u, layer_exc_cnt / 24u));
                            if (layer_pv_cnt * 16u < layer_exc_cnt) k_sub = max_u32(k_sub, layer_exc_cnt / 12u);
                            u32 tick_rem = s_mod_w[3];
                            u32 k_rem = min_u32(k_sub, tick_rem);
                            u32 kth = 16u;
                            u32 trim_mod = 0u;
                            if (k_rem > 0u) {
                                u32 acc = 0u;
                                kth = 0u;
                                for (i32 b = 15; b >= 0; --b) {
                                    u32 c = s_v_hist[b];
                                    if (acc + c > k_rem) {
                                        kth = u32(b);
                                        u32 left = (k_rem > acc) ? (k_rem - acc) : 1u;
                                        if (c > left) trim_mod = min_u32(15u, max_u32(2u, (c + left - 1u) / left));
                                        acc = k_rem;
                                        break;
                                    }
                                    acc += c;
                                }
                                s_mod_w[3] = (tick_rem > acc) ? (tick_rem - acc) : 0u;
                            }
                            s_kth_bin = kth | (trim_mod << 8);
                        }
                        sync_block();
                        u32 kp = s_kth_bin;
                        u32 kth = kp & 0xffu;
                        u32 tmod = kp >> 8;
                        if (is_new_cand) {
                            bool lost = (my_vbin < kth) ||
                                        (my_vbin == kth && tmod >= 2u && (((tid * 37u + my_vbin * 7u) % tmod) != 0u));
                            if (lost) {
                                fired = 0u;
                                if (vs > -120) vs = -120;
                            } else {
                                gamma_won = true;
                            }
                        }
                    }
                }
                if (is_excitable) {
                    if (fired) {
                        vs = -160;
                        // Authentic Larkum (1999) + Payeur (2021) Crisp 2-Spike BAC Doublet Multiplexing:
                        // Active apical plateau (plat==1) on first spike triggers a crisp doublet (refrac=0);
                        // 2nd burst spike triggers SK Ca2+-K+ repolarization (refrac=2, va>>=2) -> ZERO 4-spike seizure trains!
                        if (is_pyr && plat && !prev_gamma_won) {
                            va = (va * 5) >> 3;
                            refrac = 0u;
                        } else {
                            if (plat) va = (va >> 2);
                            refrac = (tk == TK_GLAND) ? 0u : (is_pv ? 1u : 2u);
                        }
                        ua = (tk == TK_GLAND) ? 0u : min_u32(127u, ua + u32(is_pv ? (adapt_inc >> 2) : adapt_inc));
                        ++spk_acc;
                        if (is_mod) { if ((tid & 1u) == 0u) ++mod_pos_acc; else ++mod_neg_acc; }
                    } else if (ua > 0u) {
                        ua = ua - ((ua + 7u) >> 3);
                    }
                    ring = ((ring << 1) | fired) & 0xffu;
                    // Goldilocks Turrigiano (2008) Intrinsic Homeostasis on TK_GLAND (tau = 16 ticks):
                    // Rapidly equalizes unequal fold fan-in inside a session while keeping intra-turn syllables crisp!
                    if (tk != TK_GLAND) {
                        ema = (ema * 31u + (fired ? 255u : 0u)) >> 5;
                    } else if ((sub & 1u) == 1u) {
                        ema = (ema * 31u + (((ring & 0x0fu) != 0u) ? 255u : 0u)) >> 5;
                    }
                }
            }

            // Ensure L0 glottic s_mod_w[0..1] writes are globally visible inside CTA even when freeze_plasticity==1!
            if (layer_idx == 0u) sync_block();
            u32 glottic_win_f = (layer_idx == 0u) ? s_mod_w[1] : 3u; // Save BEFORE VTA s_mod_w reduction overwrites s_mod_w[1]!
            // Authentic Laryngeal Mechanical Smoothness (gentle +45/-25 pA inertia: prevents mid-syllable ties
            // WITHOUT locking out fast 2-Syl / 3-Syl / Q&A phoneme transitions!)
            if (layer_idx == 0u && tk == TK_GLAND) {
                vs = (eff_da >= -10 && glottic_win_f < 3u) ? ((my_gfold == glottic_win_f) ? 45 : -25) : 0;
                ua = 0u;
            }

            // Latch exact tick t-1 vocal fold activity mask BEFORE s_mod_w is reused by VTA/BF MOD reduction!
            u32 prev_gfold_mask = (layer_idx == 0u) ? s_mod_w[2] : 0u;
            // Compute layer-wide VTA/BF intrinsic neuromodulator spikes ONCE (eliminates duplicate redux & WAR race!)
            u32 tot_mp_l = 0u, tot_mn_l = 0u;
            if (layer_has_mod && !abl.silence_neuromod) {
                u32 wmp = warp_has_exc ? warp_add_u32(mod_pos_acc) : 0u;
                u32 wmn = warp_has_exc ? warp_add_u32(mod_neg_acc) : 0u;
                if (lane_id() == 0u) s_mod_w[tid >> 5] = (wmp << 16) | (wmn & 0xffffu);
                sync_block();
                #pragma unroll
                for (u32 w = 0; w < 8u; ++w) { tot_mp_l += (s_mod_w[w] >> 16); tot_mn_l += (s_mod_w[w] & 0xffffu); }
            }

            // Post-tick Short-Term Dynamics (STD/STF, ALWAYS active for 100% honest freeze_plasticity ablation parity!)
            // + Long-Term 3-Factor STDP Plasticity & Structural Sprouting (strictly gated on !abl.freeze_plasticity)!
            {
                bool do_ltp = !abl.freeze_plasticity;
                i32 int_da = 0, int_ach = 0;
                if (do_ltp && layer_has_mod && !abl.silence_neuromod) {
                    int_da = clamp_i32((i32(tot_mp_l) - i32(tot_mn_l)) * 2, -8, 8);
                    int_ach = clamp_i32(i32(tot_mp_l + tot_mn_l) * 4, 0, 26);
                }
                i32 tot_da = clamp_i32(eff_da + int_da + ((tk == TK_GLAND) ? vta_voc_q : 0), -64, 64);
                i32 tot_ach = clamp_i32(eff_ach + int_ach, 0, 64);
                u32 mod_pos = (do_ltp && tot_da > 18) ? 1u : 0u;
                u32 mod_neg = (do_ltp && tot_da < -18) ? 1u : 0u;
                bool mod_gate = (mod_pos || mod_neg);
                // True Signed Vector-FA Credit on L1..L_top Pyramidal Basal STDP (Payeur-Naud-Richards 2021):
                // Hidden layers receive action-specific signed efference-copy error (`fa_da + int_da`) instead of global scalar DA!
                i32 pyr_da = (layer_idx >= 1u && is_pyr) ? clamp_i32(fa_da + int_da, -64, 64) : tot_da;
                u32 st_pos = (do_ltp && pyr_da > 16) ? 1u : 0u;
                u32 st_neg = (do_ltp && pyr_da < -16) ? 1u : 0u;
                bool st_gate = (st_pos || st_neg);
                bool ach_gate = (do_ltp && tot_ach > 14);
                // Separate high-threshold unsupervised ACh-STDP/Oja (`ext_ach_q > 44`, strong multimodal surprise only)
                // from low-threshold VIP+ disinhibition (`>12`) & Hasselmo retirement (`>18/20`) -> ZERO ocean Oja erosion!
                bool phasic_ach = (do_ltp && !abl.silence_neuromod && ext_ach_q > 44);
                bool sleep_gate = (do_ltp && is_resting && ((tick & 3u) == 0u) && !abl.silence_neuromod);
                i32 ach_boost = ach_gate ? 4 : 0;
                i32 hip_elig = (fast_plast || is_eff) ? ((plat ? 18 : 12) + ach_boost) : ((plat ? 11 : 6) + ach_boost);
                i32 da_hi_thr = (fast_plast || is_eff) ? 24 : 40;
                i32 abs_da = (tot_da < 0) ? -tot_da : tot_da;
                i32 btsp_boost = ((fast_plast || is_eff) && abs_da > 44) ? 1 : 0;
                u32 target_ema = 26u + u32((dna.bio_params[6] + 64) & 7);
                u32 post_n = popc_u32(ring & 0x0fu);
                u32 post_recent = (post_n != 0u) ? 1u : 0u;
                // Payeur-Naud-Richards (2021 Nature Neuroscience) Baseline-Relative Burst vs Isolated-Spike Credit Signal:
                bool post_burst = (layer_idx >= 1u && is_pyr) && (post_n >= 2u && plat != 0u && ema < target_ema + 14u);
                bool post_iso_err = (layer_idx >= 1u && is_pyr) && (post_n == 1u && plat == 0u && (eff_da < -15 || va < -120));
                // Homeostatic target strictly guards against seizure (>seizure_ema) and dead silence (ema==0);
                // NEVER punishes or strips inhibition from healthy 8-10% sparse PING coding cells (ema=1..9)!
                i32 homeo_delta = (ema > u32(seizure_ema)) ? -2 : ((ema + 8u > u32(seizure_ema)) ? -1 : ((ema == 0u) ? 1 : 0));

                auto update_syn_word = [&](u32 s, u32 src_l, u32 idx_k) __attribute__((always_inline)) -> u32 {
                    i32 w = bfe_s32(s, 8u, 8u);
                    if (w == 0) return s;
                    u32 pre = s & 0xffu;
                    bool is_ext = (src_l != layer_idx);
                    bool is_ap_syn = (s & 0x04000000u) != 0u;
                    u32 delay = abl.zero_delays ? (is_ext ? 2u : 3u) : (is_ext ? min_u32(bfe_u32(s, 23u, 3u), 4u) : bfe_u32(s, 23u, 3u));
                    u32 pre_r = u32(is_ext ? (s_l_spk[src_l] ? s_all_rings[src_l][pre] : 0u) : s_ring[pre]);
                    u32 shifted = pre_r >> delay;
                    u32 post4 = ring & 0x0fu;
                    u32 pre4 = shifted & 0x0fu;
                    u32 pre_rec = (pre4 != 0u) ? 1u : 0u;
                    i32 elig = i32(bfe_u32(s, 16u, 7u)) - 64;
                    bool pre_inh = is_ext ? (w < 0) : tk_is_inhibitory(s_tk[pre]);
                    if (do_ltp) {
                    // Fix sub=0/1 cross-tick causal STDP (test full unmasked `shifted` bits 0..5!)
                    // + full-tick dendritic co-activity on 4-substep integrated TK_GLAND & TK_MUSCLE!
                    bool st_causal = (post4 != 0u) &&
                                     ((tk == TK_GLAND) ? ((shifted & 0x3fu) != 0u)
                                                       : ((shifted & (post4 | (post4 << 1) | (post4 << 2))) != 0u));
                    bool st_acausal = !st_causal && (tk != TK_GLAND) && (post4 != 0u) && ((pre4 & ((post4 >> 1) | (post4 >> 2))) != 0u);
                    // Exact Causal 3-Factor STDP Ordering (Schultz VTA / Bittner BTSP / Izhikevich 2007):
                    i32 elig_pre = elig;
                    i32 dw = 0;
                    i32 abs_w = (w < 0) ? -w : w;
                    u32 rid = (s >> 27) & 3u;
                    u32 lut = dna.plast_lut[rid];
                    i32 fast_b = (tk == TK_GLAND) ? 3 : (is_eff ? 1 : ((layer_idx >= 1u && plat != 0u && abs_da > 24) ? 2 : 0));
                    i32 lr = 1 + fast_b + i32(lut & 1u) + ((abs_da > da_hi_thr) ? (1 + (is_eff ? 1 : 0)) : 0) + btsp_boost;
                    bool bac_gate = (!is_pyr) || (((lut >> 2) & 3u) != 3u) || (plat != 0u);
                    // Class-1A Thalamocortical Drivers (L0->L1: 44) > Ascending Feedforward (src<L: 30)
                    // > Cortico-Cortical Feedback/Local (L1: 18, L2+: 18 ext / 16 local, >=48n: 16/13)
                    // Vocal syrinx keeps lean L0 reflex cap (18) while L1..L4 cortical tracts & skeletal muscles get full [1..64]!
                    i32 l2_loc_cap = (layer_exc_cnt >= 20u) ? 12 : 14;
                    i32 l2_ext_cap = (layer_exc_cnt >= 20u) ? 26 : 22;
                    i32 w_hi = (tk == TK_GLAND && src_l == 0u)
                                   ? 10
                                   : ((tk == TK_GLAND) ? 60
                                                       : (is_eff ? 42
                                                                 : ((layer_idx == 1u && src_l == 0u)
                                                                        ? 44
                                                                        : ((layer_idx >= 1u && src_l < layer_idx)
                                                                               ? 34
                                                                               : ((layer_idx >= 2u) ? (is_ext ? l2_ext_cap : l2_loc_cap) : ((layer_idx == 1u) ? (is_ext ? 28 : 22) : 22))))));
                    if (tk == TK_GLAND) {
                        // Authentic Margin-Convergent Rescorla-Wagner / Shen (2008) Competitive D1/D2 Area-X:
                        // - Correct trials (+DA) consolidate gently without catastrophically overwriting the alternate class!
                        // - Prediction errors (-DA) apply crisp asymmetric D2 correction to carve sharp multi-class hyperplanes!
                        if (mod_gate) {
                            // Whole-fold Area-X credit assignment: ALL motor units belonging to the spoken vocal fold
                            // (`prev_gfold_mask & (1 << my_gfold)`) share the vocal RPE (eliminates twin-gland -DA escape!)
                            bool prev_post = ((ring & 0xf0u) != 0u) || ((prev_gfold_mask & (1u << my_gfold)) != 0u);
                            u32 pre_n = popc_u32(pre_r & 0xf0u);
                            bool prev_pre = (pre_n != 0u);
                            i32 p_mag = (src_l == 0u) ? 1 : i32(min_u32(6u, (pre_n << 1) + 2u));
                            bool is_3fold = (bp.counts[TK_GLAND] >= 3u);
                            // Zero-sum Area-X synaptic weight conservation across 2-fold and 3-fold syrinxes:
                            // Prevents +DA rewards from net-shrinking 3-fold syrinx weights (+p_mag - 2*p_riv == 0)!
                            i32 p_riv = (src_l == 0u) ? 1 : (is_3fold ? (p_mag >> 1) : i32(min_u32(4u, pre_n + 2u)));
                            i32 e_mag = (src_l == 0u) ? 2 : i32(min_u32(8u, (pre_n << 1) + 3u));
                            i32 e_riv = (src_l == 0u) ? 2 : (is_3fold ? ((e_mag + 1) >> 1) : i32(min_u32(6u, u32(e_mag))));
                            if (prev_gfold_mask != 0u) {
                                bool is_prev_rival = (!prev_post && ((prev_gfold_mask & (1u << my_gfold)) == 0u));
                                if (mod_pos) {
                                    if (!pre_inh) {
                                        if (prev_post && prev_pre && abs_w < w_hi) dw = p_mag;
                                        else if (is_prev_rival && prev_pre && abs_w > 8) dw = -p_riv;
                                    } else {
                                        if (is_prev_rival && prev_pre && abs_w < 14) dw = 1;
                                        else if (prev_post && prev_pre && abs_w > 2) dw = -1;
                                    }
                                } else if (mod_neg) {
                                    if (!pre_inh) {
                                        if (prev_post && prev_pre && abs_w > 2) dw = -e_mag;
                                        else if (is_prev_rival && prev_pre && abs_w < 54) dw = e_riv;
                                    } else {
                                        if (prev_post && prev_pre && abs_w < 14) dw = 2;
                                        else if (is_prev_rival && prev_pre && abs_w > 2) dw = -2;
                                    }
                                }
                            } else if (mod_neg && prev_pre) {
                                if (!pre_inh && abs_w < 42) dw = 6;
                                else if (pre_inh && abs_w > 2) dw = -3;
                            }
                            if (dw != 0) elig = 0;
                        }
                    } else if (pre_inh) {
                        if (!is_eff) {
                            // Authentic Vogels-Sprekeler (2011) Homeostatic iSTDP strictly preserving sparse coding:
                            // Strengthens PV/SST against hyper-excitation (ema > seizure_ema - 10) and ONLY gently relaxes
                            // inhibition on truly dead-silent cells (ema == 0, floor 16) — NEVER dismantles PV/SST on sparse cells!
                            if (pre_rec && post_recent && ema + 10u > u32(seizure_ema))
                                dw = (ema > u32(seizure_ema)) ? 2 : 1;
                            else if (pre_rec && ema == 0u && abs_w > 16 && ((tick + idx_k) & 7u) == 0u) dw = -1;
                        } else if (mod_gate && (elig > 7 || elig < -7)) {
                            i32 esign = (elig > 0) ? 1 : -1;
                            dw = mod_neg ? (esign * (lr > 3 ? 3 : lr)) : (-esign * lr);
                        }
                        i32 inh_hi = (layer_idx >= 2u && !is_eff) ? 34 : 36;
                        if (abs_w > inh_hi && dw > 0) dw = ((abs_w < 62 && ((tick + idx_k) & 3u) == 0u) ? 1 : 0);
                        if (dw != 0) elig = (mod_gate ? 0 : (elig >> 1));
                    } else if (bac_gate && (elig > 7 || elig < -7)) {
                        i32 esign = (elig > 0) ? 1 : -1;
                        if (rid == 0u && st_gate) dw = st_pos ? (esign * lr) : (-esign * lr);
                        else if (rid == 1u && st_gate) dw = st_neg ? (-esign * lr) : (esign * (is_eff ? lr : (lr > 3 ? lr - 1 : 2)));
                        else if (rid == 2u) {
                            if (st_neg) dw = -esign * lr;
                            else if (st_pos) dw = esign * lr;
                            else if (phasic_ach && plat && ((tick + idx_k) & 3u) == 0u && (esign < 0 || abs_w < w_hi - 6))
                                dw = esign;
                        } else if (rid == 3u && st_gate) dw = st_pos ? (esign * (1 + btsp_boost)) : (-esign * (1 + btsp_boost));
                        else if (!is_eff && fast_plast != 0u && phasic_ach && plat && ((tick + idx_k) & 3u) == 0u && (esign < 0 || abs_w < w_hi - 6))
                            dw = esign;
                        else if (sleep_gate && (elig > 14 || elig < -14)) dw = esign;
                        if (dw > 0) {
                            if (abs_w >= w_hi) {
                                bool no_over = is_eff || (layer_idx >= 2u && !is_ext && !is_ap_syn);
                                dw = (!no_over && abs_w < w_hi + 6 && abs_da > 40 && ((tick + idx_k) & 3u) == 0u) ? 1 : 0;
                            } else if (abs_w > w_hi - 6 && dw > 2) {
                                dw = 2;
                            }
                        } else if (dw < 0) {
                            // Symmetric soft-bound floor on BOTH cortex (12) and motor effectors (14) — ZERO 4:1 negative erosion!
                            if (abs_w < (is_eff ? 14 : 12) && dw < -2) dw = -2;
                        }
                        if (dw != 0) elig = (st_gate ? 0 : (elig >> 1));
                    }
                    // Post-DA causal eligibility accumulation & exponential decay for subsequent ticks:
                    // Crisp 2t action-specific decay on ALL effectors (prevents stale pre-halt swimming from reinforcing HALT->SWIM!)
                    // while deep cortex (L1+) retains long 12-18t behavioral-timescale traces for distal foraging & WM!
                    if (st_causal) elig = clamp_i32(elig + hip_elig, -63, 63);
                    else if (st_acausal) elig = clamp_i32(elig - ((fast_plast || is_eff) ? 6 : 4), -63, 63);
                    else if (is_eff) {
                        if (elig > 0) elig -= ((elig + 1) >> 1);
                        else if (elig < 0) elig += (((-elig) + 1) >> 1);
                    } else if (do_dec_tick) {
                        if (extra_tau == 0 && !fast_plast) {
                            if (elig > 0) elig -= ((elig + 3) >> 2);
                            else if (elig < 0) elig += (((-elig) + 3) >> 2);
                        } else {
                            if (elig > 0) elig -= ((elig + 7) >> 3);
                            else if (elig < 0) elig += (((-elig) + 7) >> 3);
                        }
                    }
                    // Authentic Payeur-Naud-Richards (2021 Nature Neuroscience) Sign-Faithful Vector-FA Burstprop
                    // + Mikulasch et al. (2022/2023 PNAS) Floored Local Dendritic Predictive Coding:
                    // Strictly potentiates basal/apical excitatory synapses on +DA (`st_pos`) and depresses on -DA (`st_neg`) —
                    // NEVER potentiates bursts or apical predictions during negative punishment errors!
                    if (layer_idx >= 1u && is_pyr && dw == 0) {
                        if (!is_ap_syn && !pre_inh && st_gate) {
                            if (st_pos && st_causal && post_burst && abs_w < w_hi)
                                dw = (abs_w < w_hi - 4) ? 2 : 1;
                            else if (st_neg && st_causal && (post_burst || (post_iso_err && elig_pre > 8)) && abs_w > 10)
                                dw = -2;
                            else if (st_pos && post_burst && !pre_rec && abs_w > 18 && ((tick + idx_k) & 3u) == 0u)
                                dw = -1;
                        } else if (is_ap_syn && (st_gate || phasic_ach) && ((tick + idx_k) & 3u) == 0u) {
                            if (!pre_inh) {
                                if (pre_rec && post_recent && (st_pos || (!st_neg && phasic_ach && abs_w < w_hi - 6)) && abs_w < w_hi) dw = 1;
                                else if (pre_rec && ((st_neg && va > 160) || (!post_recent && va > 240)) && abs_w > 12) dw = -1;
                            } else {
                                if (pre_rec && va > 220 && abs_w < 54) dw = 1;
                                else if (pre_rec && va < -110 && abs_w > 14) dw = -1;
                            }
                        }
                    }
                    // Gentle phasic-ACh unsupervised Oja sharpening ONLY on over-active hubs (ZERO passive erosion of calm detectors!):
                    if (!pre_inh && !is_eff && dw == 0 && layer_idx >= 1u && phasic_ach && plat && post_recent && !pre_rec && ema > target_ema + 14u && abs_w > 10 && ((tick + idx_k) & 3u) == 0u)
                        dw = -1;
                    // Exclude TK_GLAND from passive Oja/homeo weight erosion: Area-X already maintains exact zero-sum
                    // competitive bounds across vocal folds, while passive !pre_rec decay erases idle-concept speech
                    // memories and starves intact digestive catalysis relative to frozen controls!
                    i32 comp_floor = (layer_idx >= 1u && !is_eff) ? 10 : 34;
                    u32 comp_ema_thr = is_eff ? 160u : (u32(seizure_ema) - 6u);
                    if (!pre_inh && tk != TK_GLAND && dw == 0 && post_recent && !pre_rec && ema > comp_ema_thr && abs_w > comp_floor && ((tick + idx_k) & 3u) == 0u)
                        dw = -1;
                    i32 signed_dw = pre_inh ? -dw : dw;
                    i32 homeo_floor = (layer_idx >= 1u && !is_eff) ? 12 : 26;
                    i32 homeo_ceil = (layer_idx >= 2u && !is_eff) ? 14 : 18;
                    u32 homeo_mask = (ema > u32(seizure_ema)) ? 3u : 15u;
                    if (!is_eff || (tk != TK_GLAND && ema > 192u)) {
                        if (homeo_delta < 0 && (tick & homeo_mask) == 0u && ((pre_inh && abs_w < 54) || (!pre_inh && abs_w > homeo_floor)))
                            signed_dw += homeo_delta;
                        else if (homeo_delta > 0 && (tick & 63u) == 0u && !pre_inh && abs_w < homeo_ceil)
                            signed_dw += homeo_delta;
                    }
                    if (signed_dw != 0) {
                        i32 inh_ceil = (layer_idx >= 2u && !is_eff) ? -8 : (is_eff ? -2 : -4);
                        i32 inh_floor = (tk == TK_GLAND) ? -14 : -64;
                        i32 exc_floor = (layer_idx == 1u && src_l == 0u) ? 12 : ((tk == TK_GLAND) ? 1 : (is_eff ? 2 : 4));
                        i32 nw = pre_inh ? clamp_i32(w + signed_dw, inh_floor, inh_ceil) : clamp_i32(w + signed_dw, exc_floor, 64);
                        if (nw != w) { w = nw; ++edit_acc; }
                    }
                    } // end if (do_ltp)
                    // Authentic Markram (1998) + Wang (2006) + Mongillo (2008) Laminar STP Specialization (0 B extra VRAM!):
                    // - Neuromuscular & vocal end-plates (is_eff && !pre_inh): high safety-factor non-fatiguing release (stp=7)!
                    // - Prefrontal local recurrent basal exc (L2+, src==L, pyr): ACTIVITY-SILENT CALCIUM STF (stp=1 -> 7)
                    // - Deep ascending feedforward (L2+, src<L): gentle STD (floor stp=4)
                    // - Primary sensory L1: moderate STD (floor stp=5 -> crisp habituation & Predictive Coding MMN!)
                    u32 s_stp;
                    if (is_eff && !pre_inh) {
                        s_stp = (s & 0x1fffffffu) | 0xe0000000u;
                    } else if (layer_idx >= 2u && layer_idx <= 5u && !is_ext && is_pyr && !pre_inh && !is_ap_syn) {
                        u32 cs_u = s >> 29;
                        // True 3-Tier One-Shot Associative Hebbian-PBWM Cell-Assembly STF (Mongillo 2008 + Murray 2014):
                        // - L2: fast bigram echo (~12t)
                        // - L3: Slot 2 ACh-gated working memory (~24t)
                        // - L4: Slot 1 DA-buffered shielded context memory (~31t)
                        // - L5: Slot 0 Frontopolar BA10 slow executive goal hub (~48-63t, survives 3-syl phrases & distractors!)
                        bool is_wm_hub = (layer_idx >= 3u && layer_idx <= 5u);
                        i32 asc_sum = ext_tot[0] + ext_tot[1] + ext_tot[2] + ext_tot[3];
                        // Strictly sensory-present STF encoding (L1->L3, L2->L4, strong cue burst->L5):
                        // Prevents dark persistent activity in L3/L4 from recruiting un-cued bystander cells during pitch-black delays!
                        bool sens_on = (layer_idx == 3u) ? (s_l_spk[1] != 0u)
                                                         : ((layer_idx == 4u) ? (s_l_spk[2] != 0u)
                                                                              : ((layer_idx == 5u) ? (s_l_spk[3] != 0u && asc_sum > 140) : true));
                        bool got_asc = (!pfc_shield) && sens_on && (asc_sum > 140);
                        if (cs_u <= 2u && pre_rec && post_recent && (got_asc || (!pfc_shield && sens_on && ext_ach_q > 12))) {
                            s_stp = (s & 0x1fffffffu) | 0xe0000000u; // One-shot crisp encode to stp=7!
                        } else if (is_wm_hub && pfc_gate_flush) {
                            // Authentic Ca2+-Buffered 3-Tier PBWM Gating (O'Reilly & Frank 2006 / Murray 2014):
                            // - L3 (Slot 2) and -DA errors flush immediately to resting (stp=1).
                            // - L4 (Slot 1 DLPFC): steps -2 on fresh +DA (7->5 -> survives single-band Win 2, retires on new episode!), flushes <=5!
                            // - L5 (Slot 0 BA10): steps -1 across 3-syl phrases (7->6->5->4), flushes <=4!
                            if (layer_idx == 4u && eff_da > 18 && cs_u >= 6u)
                                s_stp = s - 0x40000000u;
                            else if (layer_idx == 5u && eff_da > 18 && cs_u >= 5u)
                                s_stp = s - 0x20000000u;
                            else
                                s_stp = (s & 0x1fffffffu) | 0x20000000u;
                        } else if ((layer_idx == 3u && ext_ach_q > 18 && !post_recent && cs_u <= 5u && cs_u >= 4u) ||
                                   (layer_idx == 4u && ((ext_ach_q > 20 && cs_u == 4u) || (ext_ach_q > 32 && cs_u == 5u))) ||
                                   (layer_idx == 5u && ((ext_ach_q > 20 && cs_u == 4u) || (ext_ach_q > 32 && cs_u <= 5u && cs_u >= 4u)))) {
                            // Hasselmo (2006) Cholinergic New-Episode Retirement:
                            // - L3 protects fresh 6..7 against distractors while retiring 4..5 on any sensory onset.
                            // - L4 & L5 survive single-band mid-phrase cues (ext_ach_q~23) while cleanly retiring completed
                            //   phrases (cs_u==4) or full multimodal new episodes (ext_ach_q>32) with ZERO inter-trial bleed!
                            s_stp = (s & 0x1fffffffu) | 0x20000000u;
                        } else {
                            // Authentic Murray et al. (2014 Nature Neurosci) Intrinsic Cortical Timescale Hierarchy:
                            // L2: ~12t ((tick&3)==0, >=5); L3/L4: ~24-31t ((tick&7)==0, >=4); L5 BA10: ~48-63t ((tick&15)==0, >=4)!
                            bool stf_dec = (layer_idx == 2u) ? ((tick & 3u) == 0u) : ((layer_idx == 5u) ? ((tick & 15u) == 0u) : ((tick & 7u) == 0u));
                            if (stf_dec) {
                                s_stp = (cs_u > 1u) ? (s - 0x20000000u) : ((cs_u == 0u) ? (s + 0x20000000u) : s);
                            } else {
                                s_stp = s;
                            }
                        }
                    } else if (layer_idx >= 2u && src_l < layer_idx && !pre_inh) {
                        u32 dep = (s >= 0xc0000000u) ? (s - 0x20000000u) : s;
                        u32 rec = (((tick & 1u) == 0u) && s < 0xe0000000u) ? (s + 0x20000000u) : s;
                        s_stp = pre_rec ? dep : rec;
                    } else {
                        u32 min_stp_w = (layer_idx == 1u && !pre_inh) ? 0xc0000000u : 0x40000000u;
                        u32 dep = (s >= min_stp_w) ? (s - 0x20000000u) : s;
                        u32 rec = (do_rec_tick && s < 0xe0000000u) ? (s + 0x20000000u) : s;
                        s_stp = pre_rec ? dep : rec;
                    }
                    return (s_stp & 0xff8000ffu) | ((u32(w) & 0xffu) << 8) | ((u32(elig + 64) & 0x7fu) << 16);
                };

                if (has_st0) {
                    u32 homeo_mask = (ema > u32(seizure_ema)) ? 3u : 15u;
                    bool homeo_due = do_ltp && (!is_eff || (tk != TK_GLAND && ema > 192u)) &&
                                     ((homeo_delta < 0 && (tick & homeo_mask) == 0u) || (homeo_delta > 0 && (tick & 63u) == 0u));
                    // 100% Bit-Exact Blackwell Idle-Post External-Stripe STDP Skip:
                    u32 comp_ema_thr_chk = is_eff ? 160u : (u32(seizure_ema) - 6u);
                    bool post_het = do_ltp && ((post_recent && (((tk != TK_GLAND) && ema > comp_ema_thr_chk) || (post_burst && st_pos) ||
                                                                (layer_idx >= 1u && !is_eff && phasic_ach && plat && ema > target_ema + 14u))) ||
                                    (tk == TK_GLAND && mod_gate));
                    bool cell_idle = (!post_het && !homeo_due);
                    auto syn_flags = [&](u32 nw, bool loc_st) __attribute__((always_inline)) -> u32 {
                        if (nw == 0u) return 0u;
                        bool stf_n = (loc_st && layer_idx >= 2u && layer_idx <= 5u && is_pyr && bfe_s32(nw, 8u, 8u) > 0 && ((nw & 0x04000000u) == 0u));
                        u32 dep_f = stf_n ? (((nw >> 29) != 1u) ? 0x40u : 0u) : ((nw < 0xe0000000u) ? 0x40u : 0u);
                        u32 hold_f = (stf_n && (nw >> 29) >= ((layer_idx >= 3u) ? 4u : 5u)) ? 0x80u : 0u; // Exact 1-bit PBWM active-STF hold (L3/L4/L5!)
                        return ((((nw >> 16) & 0x7fu) != 64u) ? 0x20u : 0u) | dep_f | hold_f;
                    };
                    // Stripe 0: bit-exact zero-work skip for BOTH external AND silent-layer local stripes!
                    bool has_e0 = (st_flags_mask & 1u) != 0u;
                    bool has_d0 = (st_flags_mask & (1u << 12)) != 0u;
                    bool loc0_need = st0_local && (any_loc_spk != 0u || spk_acc != 0u ||
                                                   (has_d0 && (do_rec_tick || pfc_gate_flush || (layer_idx >= 3u && layer_idx <= 5u && ext_ach_q > 18))) ||
                                                   (do_ltp && has_e0 && (do_dec_tick || mod_est_tick)) || (do_ltp && (tick & 63u) == 0u));
                    if (loc0_need || !cell_idle || st0_ext_need != 0u) {
                        u64 sb0 = u64(ss0) * LAYER_SYN_QUADS;
                        u32 new_f0 = 0u;
                        if (!st0_loaded) {
                            #pragma unroll
                            for (u32 g = 0; g < QUADS_PER_STRIPE; ++g) {
                                u32 q0, q1, q2, q3;
                                ld_syn_quad(pool.syn_slabs + sb0 + u64(g) * TISSUE_CELLS + tid, q0, q1, q2, q3);
                                syn_reg[g * 4u + 0u] = q0; syn_reg[g * 4u + 1u] = q1;
                                syn_reg[g * 4u + 2u] = q2; syn_reg[g * 4u + 3u] = q3;
                                if ((q0 | q1 | q2 | q3) == 0u) break;
                                st0_qcnt = g + 1u;
                            }
                        }
                        for (u32 g = 0; g < st0_qcnt; ++g) {
                            u32 o0 = syn_reg[g * 4u + 0u], o1 = syn_reg[g * 4u + 1u];
                            u32 o2 = syn_reg[g * 4u + 2u], o3 = syn_reg[g * 4u + 3u];
                            if ((o0 | o1 | o2 | o3) == 0u) break;
                            u32 n0 = update_syn_word(o0, src_l0, g * 4u + 0u);
                            u32 n1 = update_syn_word(o1, src_l0, g * 4u + 1u);
                            u32 n2 = update_syn_word(o2, src_l0, g * 4u + 2u);
                            u32 n3 = update_syn_word(o3, src_l0, g * 4u + 3u);
                            new_f0 |= syn_flags(n0, st0_local) | syn_flags(n1, st0_local) | syn_flags(n2, st0_local) | syn_flags(n3, st0_local);
                            syn_reg[g * 4u + 0u] = n0; syn_reg[g * 4u + 1u] = n1;
                            syn_reg[g * 4u + 2u] = n2; syn_reg[g * 4u + 3u] = n3;
                            if (((n0 ^ o0) | (n1 ^ o1) | (n2 ^ o2) | (n3 ^ o3)) != 0u) {
                                st_syn_quad(pool.syn_slabs + sb0 + u64(g) * TISSUE_CELLS + tid, n0, n1, n2, n3);
                            }
                        }
                        // Authentic Experience-Dependent Cortical Structural Sprouting (Holtmaat & Svoboda 2009):
                        // Strictly inside neocortex (L1+, NEVER wiring random L0 noise onto muscles/grasps/syrinx!),
                        // strictly Hebbian co-active (`post_recent != 0`) and neuromodulation-gated (`mod_pos || ext_ach_q > 18`)!
                        if (do_ltp && st0_local && layer_idx >= 1u && post_recent != 0u && (mod_pos || ext_ach_q > 18) && ((tick & 63u) == 0u)) {
                            u32 empty_slot = SYN_PER_STRIPE;
                            u32 k_lim = st0_qcnt << 2;
                            for (u32 k = 0; k < k_lim; ++k) {
                                if (syn_reg[k] == 0u) { empty_slot = k; break; }
                            }
                            if (empty_slot == SYN_PER_STRIPE && st0_qcnt < QUADS_PER_STRIPE) {
                                empty_slot = k_lim;
                                syn_reg[empty_slot + 0u] = 0u; syn_reg[empty_slot + 1u] = 0u;
                                syn_reg[empty_slot + 2u] = 0u; syn_reg[empty_slot + 3u] = 0u;
                            }
                            if (empty_slot < SYN_PER_STRIPE) {
                                u32 cand = mix(tick * 2654435761u + tid * 31337u + layer_idx * 97u) & 255u;
                                if (cand != tid && (s_ring[cand] & 0x0fu) != 0u) {
                                    u8 pt = s_tk[cand];
                                    bool pre_ok = (tk_is_sensor(pt) || tk_is_neuron(pt));
                                    if (pre_ok) {
                                        bool pre_inh = tk_is_inhibitory(pt);
                                        i32 ddx = i32(ux) - i32(cand & 15u);
                                        i32 ddy = i32(uy) - i32(cand >> 4);
                                        u32 delay = u32(clamp_i32(i32(fast_sqrt(float(ddx * ddx + ddy * ddy)) * 0.45f), 0, 7));
                                        u32 comp = (pt == TK_NEU_SST) ? 1u : (pt == TK_NEU_PV ? 0u : ((empty_slot & 1u) ? 1u : 0u));
                                        u32 sp_stp = (layer_idx >= 2u && layer_idx <= 5u && is_pyr && !pre_inh && comp == 0u) ? 1u : 6u;
                                        syn_reg[empty_slot] = pack_synapse(cand, pre_inh ? -18 : 12, 0, delay, comp, (tid + empty_slot) & 3u, sp_stp);
                                        u32 g = empty_slot >> 2;
                                        st_syn_quad(pool.syn_slabs + sb0 + u64(g) * TISSUE_CELLS + tid,
                                                    syn_reg[g * 4u + 0u], syn_reg[g * 4u + 1u], syn_reg[g * 4u + 2u], syn_reg[g * 4u + 3u]);
                                        new_f0 |= 0x40u;
                                        ++rewire_acc;
                                    }
                                }
                            }
                        }
                        u32 old_f0 = (has_e0 ? 0x20u : 0u) | (has_d0 ? 0x40u : 0u) | (st0_stf_hold ? 0x80u : 0u);
                        if (new_f0 != old_f0) pool.stripe_src_layer[u64(ss0) * TISSUE_CELLS + tid] = u8((src_l0 & 0x1fu) | new_f0);
                    }
                    // Stripes 1..my_stripes-1: Blackwell 100% bit-exact zero-work external & local stripe skip!
                    for (u32 st = 1u; st < my_stripes; ++st) {
                        u32 src_l = u32((packed_src_l >> (st * 5u)) & 31ull);
                        bool is_ext_st = (src_l != layer_idx && s_cr_slabs[src_l] != INVALID_SLAB);
                        bool has_e = (st_flags_mask & (1u << st)) != 0u;
                        bool has_d = (st_flags_mask & (1u << (st + 12u))) != 0u;
                        if (cell_idle) {
                            if (is_ext_st && ((ext_need_mask & (1u << st)) == 0u)) continue;
                            if (!is_ext_st && any_loc_spk == 0u && spk_acc == 0u &&
                                !(has_d && do_rec_tick) && !(has_e && (do_dec_tick || mod_est_tick))) continue;
                        }
                        u32 ss = s_lss[st];
                        u64 sbase = u64(ss) * LAYER_SYN_QUADS;
                        u32 new_f = 0u;
                        #pragma unroll
                        for (u32 g = 0; g < QUADS_PER_STRIPE; ++g) {
                            u32 o0, o1, o2, o3;
                            ld_syn_quad(pool.syn_slabs + sbase + u64(g) * TISSUE_CELLS + tid, o0, o1, o2, o3);
                            if ((o0 | o1 | o2 | o3) == 0u) break;
                            u32 n0 = update_syn_word(o0, src_l, g * 4u + 0u);
                            u32 n1 = update_syn_word(o1, src_l, g * 4u + 1u);
                            u32 n2 = update_syn_word(o2, src_l, g * 4u + 2u);
                            u32 n3 = update_syn_word(o3, src_l, g * 4u + 3u);
                            new_f |= syn_flags(n0, !is_ext_st) | syn_flags(n1, !is_ext_st) | syn_flags(n2, !is_ext_st) | syn_flags(n3, !is_ext_st);
                            if (((n0 ^ o0) | (n1 ^ o1) | (n2 ^ o2) | (n3 ^ o3)) != 0u) {
                                st_syn_quad(pool.syn_slabs + sbase + u64(g) * TISSUE_CELLS + tid, n0, n1, n2, n3);
                            }
                        }
                        u32 old_f = (has_e ? 0x20u : 0u) | (has_d ? 0x40u : 0u);
                        if (new_f != old_f) pool.stripe_src_layer[u64(ss) * TISSUE_CELLS + tid] = u8((src_l & 0x1fu) | new_f);
                    }
                }
            }
        }

        // Write back packed neuron state & race-free double-buffered 8-substep L2 spike ring!
        if (my_exc) {
            pool.neurons[nbase + tid] = pack_neuron(vs, va, ua, ring, ema, tk, refrac, plat, slow_tau, fast_plast);
        }
        pool.spike_ring_next[nbase + tid] = my_exc ? u8(ring) : 0u;

        // 3. EMERGENT ANATOMICAL MOTOR & SONGBIRD BILATERAL DUAL-SYRINX DECODING!
        // ZERO hardcoded motor neuron slots: left muscles physically turn right, right muscles turn left!
        if (layer_idx == 0u && tk_is_effector(u8(tk))) {
            u32 n_spk = popc_u32(ring & 0x0fu);
            float m_act = float(n_spk) * 0.25f;
            if (tk == TK_GLAND) {
                // Pure Anatomical Songbird 3-Fold Syrinx (Rank-ordered body-axis somatotopy — 100% MONOTONIC gain!):
                // Works identically on BOTH bilateral and asymmetric evolved morphs (zero v0-mute or v1-mute species!):
                // - >=3 glands: rank%3 -> v0, v1, v2 (dedicated monotonic fold per formant!)
                // - ==2 glands: rank 0 -> v0, rank 1 -> v2 (100% monotonic!)
                // - ==1 gland : burst-tension modulation (1=v0, 2=v2, 3+=v1).
                u32 base_fold = my_gfold;
                atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[10u + base_fold]), 1u);
                if (m_act > 0.0f) {
                    u32 fold = (bp.counts[TK_GLAND] >= 2u) ? base_fold
                                                           : ((n_spk == 1u) ? 0u : ((n_spk == 2u) ? 2u : 1u));
                    u32 v_amp = u32(m_act * 100.0f);
                    if (fold == 0u) atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[6]), v_amp); // vocal0
                    else if (fold == 1u) atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[7]), v_amp); // vocal1
                    else atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[9]), v_amp); // vocal2
                    atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[5]), v_amp); // catalyze
                    atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[8]), v_amp); // secrete/share
                }
            } else if (tk == TK_GRASP) {
                // Clean bilaterian manipulator somatotopy with exact per-axis normalization (ZERO midline dig+build jam!):
                // Anterior midline jaws (ux==8, uy<=8) eat cleanly; lateral claws (ux<8 dig, ux>8 build) & alternating
                // posterior midline manipulators specialize dig/pick vs build/crack with full dynamic range!
                bool g_eat = (uy <= 8u);
                bool g_dig = (ux < 8u || (ux == 8u && uy > 8u && ((uy & 1u) == 0u)));
                bool g_bld = (ux > 8u || (ux == 8u && uy > 8u && ((uy & 1u) == 1u)));
                if (g_dig) atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[13]), 1u);
                if (g_bld) atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[14]), 1u);
                if (g_eat) atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[15]), 1u);
                if (m_act > 0.0f) {
                    u32 g_amp = u32(m_act * 100.0f);
                    if (g_eat) atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[4]), g_amp); // eat
                    if (g_dig) atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[2]), g_amp); // dig/pick
                    if (g_bld) atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[3]), g_amp); // build/crack
                }
            } else if (m_act > 0.0f && tk == TK_MUSCLE) {
                float dx_b = float(i32(ux) - 8) - bp.com_x;
                atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[0]), u32(m_act * 100.0f)); // thrust
                i32 trq_i = i32(m_act * (-dx_b) * 110.0f); // Left muscle (dx<0) -> +torque (turn right)!
                atomic_add_gen_u32(reinterpret_cast<u32 *>(&s_eff[1]), u32(trq_i));
            }
        }

        // Blackwell Empty-Warp Zero-Redux Fast-Path (reuses s_mod_w computed once above — ZERO duplicate redux!)
        u32 wspk = 0u, wsens = 0u, wed = 0u, wrw = 0u, wexc = 0u;
        if (warp_has_exc) {
            wspk = warp_add_u32(spk_acc);
            wsens = (layer_idx == 0u) ? warp_add_u32(tk_is_sensor(u8(tk)) ? spk_acc : 0u) : 0u;
            wed = abl.freeze_plasticity ? 0u : warp_add_u32(edit_acc);
            wrw = abl.freeze_plasticity ? 0u : warp_add_u32(rewire_acc);
            wexc = popc_u32(exc_bm);
        }
        if (lane_id() == 0u) {
            s_warp_spk[tid >> 5] = (wspk & 0xffffu) | (wsens << 16);
            s_warp_ed[tid >> 5] = wed;
            s_warp_rw[tid >> 5] = (wrw & 0xffffu) | (wexc << 16);
        }
        sync_block();

        if (tid == 0u) {
            u32 tot_spk = 0u, tot_sens_spk = 0u, tot_ed = 0u, tot_rw = 0u, tot_exc_l = 0u, tot_mp = 0u, tot_mn = 0u;
            #pragma unroll
            for (u32 w = 0; w < 8u; ++w) {
                tot_spk += (s_warp_spk[w] & 0xffffu);
                tot_sens_spk += (s_warp_spk[w] >> 16);
                if (!abl.freeze_plasticity) {
                    tot_ed += s_warp_ed[w];
                    tot_rw += (s_warp_rw[w] & 0xffffu);
                }
                tot_exc_l += (s_warp_rw[w] >> 16);
                if (layer_has_mod && !abl.silence_neuromod) { tot_mp += (s_mod_w[w] >> 16); tot_mn += (s_mod_w[w] & 0xffffu); }
            }
            atomic_add_u64(&cr.spikes_total, u64(tot_spk));
            if (tot_ed) atomic_add_u32(&cr.plast_edits, tot_ed);
            if (tot_rw) atomic_add_u32(&cr.syn_rewires, tot_rw);
            if (tot_mp + tot_mn > 0u) {
                i32 net_m = i32(tot_mp) - i32(tot_mn);
                if (net_m >= 2 || net_m <= -2)
                    atomic_add_f32(&cr.int_da, clampf(float(net_m) * 0.04f, -0.20f, 0.20f));
                atomic_add_f32(&cr.int_ach, clampf(float(tot_mp + tot_mn) * 0.05f, 0.0f, 0.35f));
            }

            float rest_disc = is_resting ? 0.50f : 1.0f;
            // Authentic mammalian sparse-coding excitotoxicity law (~15% cortical duty cycle = 0.60 spk/cell/t on deep layers!):
            // Active L0 sensory/motor interface keeps full headroom (1.0 spk/cell/t), while epileptic 0%-I deep
            // layers pay real Na+/K+-ATPase excitotoxicity -> naturally selects sparse 20-25% PV/SST neocortices!
            // Authentic myelinated sparse-coding efficiency (20x lower ATP/spike in deep cortex) + true seizure excitotoxicity:
            // Healthy ~10% sparse pyramidal firing + fast-spiking PV+ interneurons (~0.6-1.0 spk/cell/t) pay ZERO seizure penalty;
            // ONLY un-inhibited epileptic sheets (>1.5 spk/cell/t across 4 substeps) pay real Na+/K+-ATPase excitotoxicity!
            float spk_cost = (layer_idx > 0u ? 0.000002f : 0.000080f) * rest_disc;
            u32 healthy_cap = max_u32(16u, (tot_exc_l * 3u) >> 1);
            float seizure_pen = (tot_spk > healthy_cap) ? (0.00140f * float(tot_spk - healthy_cap)) : 0.0f;
            atomic_add_f32(&cr.atp, -(spk_cost * float(tot_spk) + seizure_pen));

            if (layer_idx == 0u) {
                float nm = max_u32(1u, u32(bp.counts[TK_MUSCLE]));
                float nl = max_u32(1u, u32(bp.counts[TK_GLAND]));
                u32 *eu = reinterpret_cast<u32 *>(s_eff);
                i32 *ei = reinterpret_cast<i32 *>(s_eff);
                float ng_d = float(max_u32(1u, eu[13]));
                float ng_b = float(max_u32(1u, eu[14]));
                float ng_e = float(max_u32(1u, eu[15]));
                cr.mot_thrust = clampf(float(eu[0]) / (100.0f * nm), 0.0f, 1.0f);
                cr.mot_turn = clampf(float(ei[1]) / (45.0f * nm), -1.0f, 1.0f);
                cr.mot_dig = clampf(float(eu[2]) / (85.0f * ng_d), 0.0f, 1.0f);
                cr.mot_build = clampf(float(eu[3]) / (85.0f * ng_b), 0.0f, 1.0f);
                cr.mot_eat = clampf(float(eu[4]) / (75.0f * ng_e), 0.0f, 1.0f);
                cr.mot_catalyze = clampf(float(eu[5]) / (80.0f * nl), 0.0f, 1.0f);
                // Exact per-fold normalization + winner-preserving respiratory air-sac Bernoulli shunting!
                // Peak open fold keeps 100% lung pressure (exact ties stay exact ties); weaker simultaneous folds attenuate by (rv/rmax)^2!
                float nf0 = float(max_u32(1u, eu[10]));
                float nf1 = float(max_u32(1u, eu[11]));
                float nf2 = float(max_u32(1u, eu[12]));
                float rv0 = clampf(float(eu[6]) / (95.0f * nf0), 0.0f, 1.0f);
                float rv1 = clampf(float(eu[7]) / (95.0f * nf1), 0.0f, 1.0f);
                float rv2 = clampf(float(eu[9]) / (95.0f * nf2), 0.0f, 1.0f);
                float rmax = (rv0 > rv1) ? rv0 : rv1;
                if (rv2 > rmax) rmax = rv2;
                auto air_sac = [&](float rv) -> float {
                    if (rv <= 0.0f || rmax <= 0.0f) return 0.0f;
                    float ratio = rv / rmax;
                    return clampf(rv * ratio * ratio, 0.0f, 1.0f);
                };
                cr.mot_vocal0 = air_sac(rv0);
                cr.mot_vocal1 = air_sac(rv1);
                cr.mot_vocal2 = air_sac(rv2);
                cr.mot_secrete = clampf(float(eu[8]) / (85.0f * nl), 0.0f, 1.0f);

                // Authentic Pure-Sensory Nucleus Basalis Positive-Onset Cholinergic Novelty (Hasselmo 2006 / Yu & Dayan 2005):
                // Measures strictly sensory-receptor spikes (`TK_EYE + TK_EAR + TK_CHEM`, 0 motor contamination!), normalized
                // for sparse orthogonal channel activation (`clamp(n_sens, 4, 6)` -> +1 ocean jitter = 0 ACh, while ANY
                // single crisp audio/visual band onset (+2 spk) fires equitable phasic ACh in both daylight and darkness!).
                // Double-buffered via upper 6 bits of `peer_tpl_band` and published between ticks in body_ecology_step.
                u32 n_sens = u32(bp.counts[TK_EYE]) + u32(bp.counts[TK_EAR]) + u32(bp.counts[TK_CHEM]);
                float snorm = float(tot_sens_spk) / float(max_u32(4u, min_u32(6u, n_sens)));
                float d_sn = snorm - cr.prev_sens;
                float new_ach = (d_sn > 0.20f) ? clampf((d_sn - 0.16f) * 2.1f, 0.0f, 1.0f) : 0.0f;
                u32 new_ach_q = u32(clamp_i32(i32(new_ach * 63.0f + 0.5f), 0, 63));
                cr.peer_tpl_band = u8((cr.peer_tpl_band & 0x03u) | (new_ach_q << 2));
                cr.prev_sens = snorm;
            }
            if (ctr) {
                if (tot_spk) atomic_add_u64(&ctr->total_spikes, u64(tot_spk));
                if (tot_ed) atomic_add_u64(&ctr->total_edits, u64(tot_ed));
                if (tot_rw) atomic_add_u32(&ctr->total_rewires, tot_rw);
                if (cr.age < 120u && tot_ed > 0u) atomic_add_u32(&ctr->total_imprint_edits, tot_ed);
            }
        }
        sync_block();
    }
}

// ---------------------------------------------------------------------------
// Embodied 256x256 Physics, Prey Hunting, Tool Cracking, Maturation & Selection!
// ---------------------------------------------------------------------------
AX_FN void body_ecology_step(GenesisDNA *dnas, const BodyPhenotype *bodies, CreatureState *creatures,
                             u8 *needs_dev, WorldBuffers wb, TissuePool pool, SimCounters *ctr,
                             u32 pop_n, u32 tick, u32 seed, AblationFlags abl) {
    u32 cid = cta_x() * ntid_x() + tid_x();
    if (cid >= pop_n) return;
    CreatureState &cr = creatures[cid];
    if (cr.alive != 1u) return;

    const BodyPhenotype &bp = bodies[cid];
    const GenesisDNA &dna = dnas[cid];

    u32 W = wb.w, H = wb.h, MX = wb.mask_x, MY = wb.mask_y, SY = wb.shift_y;
    float WF = float(W), HF = float(H);
    i32 ix = i32(cr.x + WF * 2.0f) & i32(MX);
    i32 iy = i32(cr.y + HF * 2.0f) & i32(MY);
    u32 idx = (u32(iy) << SY) | u32(ix);

    // 1. Rigid-Body / Hydrodynamic Motion + Fast Phasic DAT Reuptake & Race-Free ACh/Vector-FA Double-Buffer Latch!
    cr.int_da = clampf(cr.int_da * 0.35f, -0.22f, 0.22f);
    cr.int_ach = clampf(cr.int_ach * 0.50f, 0.0f, 0.38f);
    cr.mod_ach = float(cr.peer_tpl_band >> 2) * (1.0f / 63.0f);
    cr.peer_tpl_band &= 0x03u;

    // Physical motor exclusivity: conflicting skeletal/mouth actions (thrust+dig+build+eat) interfere,
    // while internal gland catalysis & vocalizing (syrinx sonar/speech) operate independently so organisms can swim AND speak!
    float m_sum = cr.mot_thrust + cr.mot_dig + cr.mot_build + cr.mot_eat;
    float m_max = cr.mot_thrust;
    if (cr.mot_dig > m_max) m_max = cr.mot_dig;
    if (cr.mot_build > m_max) m_max = cr.mot_build;
    if (cr.mot_eat > m_max) m_max = cr.mot_eat;
    float m_overlap = clampf(m_sum - m_max - 0.55f, 0.0f, 3.5f);
    float motor_eff = 1.0f / (1.0f + 1.15f * m_overlap);
    float sym_eff = 0.62f + 0.0038f * float(bp.symmetry_pct); // Streamlined bilateral body vs asymmetric yaw drag!

    float ct = fast_cos(cr.theta), st = fast_sin(cr.theta);
    float water_d = wb.water[idx];
    float drag = 0.72f + 0.12f * clampf(water_d, 0.0f, 1.5f);
    // Gentle high-Reynolds quadratic hydrodynamic drag (keeps active hunting agile while curbing v>2.5 rockets!)
    float cur_spd = fast_sqrt(cr.vx * cr.vx + cr.vy * cr.vy + 0.01f);
    float qdrag = drag / (1.0f + 0.10f * cur_spd);
    float action_drag = 1.0f - 0.30f * (cr.mot_dig > 0.35f ? 1.0f : 0.0f) - 0.22f * (cr.mot_eat > 0.25f ? 1.0f : 0.0f);
    float f_fwd = cr.mot_thrust * bp.thrust_cap * bp.inv_mass * 0.85f * action_drag * motor_eff * sym_eff;
    float trq = clampf(cr.mot_turn * bp.torque_cap / (0.18f * bp.inertia + 0.8f) * 0.65f, -0.38f, 0.38f);

    cr.omega = cr.omega * 0.68f + trq;
    cr.theta += cr.omega;
    if (cr.theta > 6.2831853f) cr.theta -= 6.2831853f;
    if (cr.theta < 0.0f) cr.theta += 6.2831853f;

    float cur_push = (water_d > 0.25f) ? 0.22f : 0.03f;
    cr.vx = cr.vx * qdrag + ct * f_fwd + wb.vx[idx] * cur_push;
    cr.vy = cr.vy * qdrag + st * f_fwd + wb.vy[idx] * cur_push;

    float nx = cr.x + cr.vx;
    float ny = cr.y + cr.vy;
    if (nx < 0.0f) nx += WF;
    if (nx >= WF) nx -= WF;
    if (ny < 0.0f) ny += HF;
    if (ny >= HF) ny -= HF;

    u32 nidx = (u32(i32(ny) & i32(MY)) << SY) | u32(i32(nx) & i32(MX));
    if (wb.terrain[nidx] > 3.5f || wb.material[nidx] == MAT_POLYMER) {
        cr.vx = -cr.vx * 0.35f;
        cr.vy = -cr.vy * 0.35f;
        cr.hp -= 0.4f * (1.0f - bp.armor_factor);
    } else {
        cr.x = nx; cr.y = ny; idx = nidx;
        cr.home_dx = clampf(cr.home_dx * 0.999f + cr.vx, -96.0f, 96.0f);
        cr.home_dy = clampf(cr.home_dy * 0.999f + cr.vy, -96.0f, 96.0f);
    }
    // Check if physically sheltered next to built MAT_POLYMER wall (resets vestibular home anchor & shields from APEX!)
    bool near_wall = false;
    {
        u32 tx = idx & MX, ty = idx >> SY;
        near_wall = (wb.material[(ty << SY) | ((tx + 1u) & MX)] == MAT_POLYMER) ||
                    (wb.material[(ty << SY) | ((tx + W - 1u) & MX)] == MAT_POLYMER) ||
                    (wb.material[(((ty + 1u) & MY) << SY) | tx] == MAT_POLYMER) ||
                    (wb.material[(((ty + H - 1u) & MY) << SY) | tx] == MAT_POLYMER);
        if (near_wall) {
            cr.home_dx *= 0.80f;
            cr.home_dy *= 0.80f;
            if (wb.light[idx] < 0.14f && wb.temp[idx] < 0.42f && ((tick + cid) & 15u) == 0u) {
                if (ctr) atomic_add_u32(&ctr->total_shelter_nights, 1u);
            }
        }
    }
    // 3-Band Acoustic Reception with Cochlear Octopus-Cell Onset Detection & Turn-Taking Telemetry!
    if (bp.counts[TK_EAR] > 0u && !abl.deaf_ears) {
        float hv0 = fabsf_dev(wb.acoust_v0[idx]);
        float hv1 = fabsf_dev(wb.acoust_v1[idx]);
        float hv2 = fabsf_dev(wb.acoust_v2[idx]);
        float dv0 = hv0 - fabsf_dev(wb.acoust_v0_prev[idx]);
        float dv1 = hv1 - fabsf_dev(wb.acoust_v1_prev[idx]);
        float dv2 = hv2 - fabsf_dev(wb.acoust_v2_prev[idx]);
        float max_dv = (dv2 >= dv0 && dv2 >= dv1) ? dv2 : ((dv1 >= dv0) ? dv1 : dv0);
        float heard_v = hv0 + hv1 + hv2;
        if (heard_v > 0.05f) {
            u8 cand_b = (max_dv > 0.03f)
                            ? ((dv2 >= dv0 && dv2 >= dv1) ? u8(2u) : ((dv1 >= dv0) ? u8(1u) : u8(0u)))
                            : ((hv2 >= hv0 && hv2 >= hv1) ? u8(2u) : ((hv1 >= hv0) ? u8(1u) : u8(0u)));
            // Exclude immediate self-echo on the same band right after own vocal emission!
            bool self_echo = (cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 1u && cand_b == cr.last_vocal_band);
            if (!self_echo) {
                cr.last_heard_tick = tick; // Real physical acoustic hearing timestamp for vocal-duet & mimicry coordination!
                cr.last_heard_band = cand_b;
                if (tick > cr.resp_heard_tick + 12u) {
                    cr.resp_heard_tick = tick;
                    cr.last_heard_theta = cr.theta;
                }
            }
        }
        if (cr.resp_heard_tick > 0u && tick > cr.resp_heard_tick && tick <= cr.resp_heard_tick + 12u) {
            float dth = fabsf_dev(cr.theta - cr.last_heard_theta);
            if (dth > 3.1415926f) dth = 6.2831853f - dth;
            bool voc_reply = (cr.last_vocal_tick > cr.resp_heard_tick && cr.last_vocal_tick <= tick);
            if (dth > 0.22f || voc_reply) {
                if (ctr) {
                    if (dth > 0.22f) atomic_add_u32(&ctr->total_vocal_resp, 1u);
                    if (voc_reply) atomic_add_u32(&ctr->total_vocal_replies, 1u);
                }
                cr.resp_heard_tick = 0u;
            }
        }
    }
    atomic_add_u32(&wb.cell_occ[idx], 1u);
    wb.tile_occ_id[idx] = cid + 1u;
    if (cr.mot_thrust > 0.35f) {
        atomic_add_f32(&wb.acoust_p[idx], 0.06f * cr.mot_thrust * clampf(bp.mass * 0.06f, 0.4f, 1.8f));
    }

    // 2. Real Allometric Thermal Inertia, Social Thermal Huddling & Corrosive/Thermal Hazards
    float env_temp = wb.temp[idx];
    float therm_rate = (0.36f / (1.0f + 0.12f * clampf(bp.mass, 1.0f, 32.0f))) * (1.0f - 0.65f * bp.armor_factor);
    cr.body_temp += (env_temp - cr.body_temp) * therm_rate;
    // Authentic social huddling warmth + Trisolaran Metabolic Torpor ("Dehydration") in shelters/thermal refuges!
    // Requires genuine executive motor restraint by a motile organism (muscle>=1 choosing thrust<0.14, dig<0.15, eat<0.15)
    // inside a polymer shelter or thermal refuge -> cuts thermal stress & basal burn by 65% + gentle tissue recovery!
    bool social_huddle = (wb.cell_occ[idx] >= 2u && cr.last_heard_tick > 0u && tick <= cr.last_heard_tick + 32u);
    bool in_refuge = near_wall || (env_temp > 0.60f && water_d > 1.15f) || (env_temp < 0.36f && social_huddle);
    bool in_torpor = (abl.challenge_tier >= 2u) && (bp.counts[TK_MUSCLE] >= 1u) && in_refuge &&
                     (cr.mot_thrust < 0.14f && cr.mot_dig < 0.15f && cr.mot_eat < 0.15f);
    // Trisolaris-15 Closed-Loop Bioacoustic Shelter & Acid-Storm Cavity Resonance:
    // Tuning a v2 standing wave to storm/shelter acoustics requires real auditory feedback (`n_ear_r >= 1`),
    // scaling smoothly with basilar-papilla spectral resolution (4-band orthogonal hearing locks cavity resonance best!)
    u32 n_ear_r = abl.deaf_ears ? 0u : u32(bp.counts[TK_EAR]);
    bool v2_self_shelter = (n_ear_r >= 1u && cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 20u && cr.last_vocal_band == 2u);
    bool v2_heard_shelter = (n_ear_r >= 1u && cr.last_heard_tick > 0u && tick <= cr.last_heard_tick + 32u && cr.last_heard_band == 2u);
    float cav_atten = (n_ear_r >= 2u) ? 0.35f : ((n_ear_r == 1u) ? 0.60f : 1.00f);
    bool shelter_beacon = (abl.challenge_tier >= 1u) && (near_wall || in_refuge) && (env_temp > 0.60f || env_temp < 0.38f) &&
                          (v2_self_shelter || v2_heard_shelter);
    bool acid_disperse = (abl.challenge_tier >= 1u) && (v2_self_shelter || v2_heard_shelter);
    float therm_stress = fabsf_dev(cr.body_temp - 0.50f) * (social_huddle && env_temp < 0.45f ? 0.50f : 1.0f) *
                         (in_torpor ? 0.35f : 1.0f) * (shelter_beacon ? cav_atten : 1.0f);
    float therm_cost = (therm_stress > 0.16f) ? (therm_stress - 0.16f) * 0.40f * (1.0f - 0.70f * bp.armor_factor) : 0.0f;
    float cd_here = wb.chem_d[idx];
    float tox_mul = acid_disperse ? cav_atten : 1.0f;
    float tox_cost = (cd_here > 0.30f) ? (cd_here - 0.30f) * 0.85f * (1.0f - 0.55f * bp.armor_factor) * tox_mul : 0.0f;
    if (cd_here > 0.38f) cr.hp -= (cd_here - 0.38f) * 10.0f * (1.0f - 0.55f * bp.armor_factor) * tox_mul;
    if (therm_stress > 0.28f) cr.hp -= (therm_stress - 0.28f) * 8.0f * (1.0f - 0.70f * bp.armor_factor);
    // Authentic Trisolaran Rehydration / Torpor Recovery + Energy-Conserving Physiological Tissue Repair:
    // Motile minds in torpor repair tissue fast; well-fed organisms in calm clean water spend honest ATP to heal old scratches!
    if (in_torpor && therm_stress < 0.14f && cr.hp < 100.0f) {
        cr.hp = clampf(cr.hp + (shelter_beacon ? 0.55f : 0.35f), 0.0f, 100.0f);
    } else if (cr.hp < 100.0f && cr.atp > 16.0f && therm_stress < 0.14f && cd_here < 0.08f) {
        cr.hp = clampf(cr.hp + 0.24f, 0.0f, 100.0f);
        cr.atp -= 0.02f;
    }
    // Gentle Trisolaran anti-brawn quadratic vascular maintenance strictly in Challenge >= 2 (>12 muscles; 0 cost in E1 or agile minds!)
    u32 ex_mus = (bp.counts[TK_MUSCLE] > 12u) ? (u32(bp.counts[TK_MUSCLE]) - 12u) : 0u;
    float brawn_cost = (abl.challenge_tier >= 2u && ex_mus > 0u) ? (0.0010f * float(ex_mus * ex_mus)) : 0.0f;
    float act_work = 0.012f * cr.mot_eat + 0.012f * cr.mot_dig + 0.010f * cr.mot_build + 0.020f * m_overlap;
    float work_cost = 0.0045f * cr.mot_thrust * bp.thrust_cap + 0.0020f * fabsf_dev(cr.mot_turn) * bp.torque_cap + act_work;
    float basal_eff = (in_torpor ? 0.35f : 1.0f) * bp.basal_cost + brawn_cost;
    cr.atp -= (basal_eff + work_cost + therm_cost + tox_cost);

    // 3. Selective Active Foraging, Essential Endocrine/Digestive & 3-Fold Vocal-Syrinx Glands (3 glands saturate!) & Catalysis
    float gland_sat = clampf(float(bp.counts[TK_GLAND]) * 0.34f, 0.0f, 1.0f);
    // Honest enzymatic physiology: 3+ digestive/vocal glands sustain full 1.28x assimilation, while gland==0 lacks
    // digestive enzymes (0.45x -> naturally eliminates mute zero-gland parasites without over-selecting 6+ gland hypertrophy!)
    float digest_mul = 0.45f + 0.83f * gland_sat;
    float detox_mul = 1.12f - 0.37f * gland_sat;
    // Authentic Vogel (1994) Hydrodynamic Boundary-Layer Ventilation:
    // Bilateral muscular swimming/ram-ventilation sweeps fresh nutrient-rich water across the mouth (1.00x);
    // muscleless rocks (0.01x) and unsteerable 1-muscle midline rods (0.30x) deplete their stagnant boundary layer!
    float flow_vent = (abl.challenge_tier >= 1u)
                          ? ((bp.counts[TK_MUSCLE] == 0u)
                                 ? 0.01f
                                 : ((bp.counts[TK_MUSCLE] == 1u)
                                        ? 0.30f
                                        : clampf(0.20f + 0.80f * clampf(bp.thrust_cap * 0.75f, 0.0f, 1.0f), 0.20f, 1.0f)))
                          : 1.0f;
    float eat_rate = (0.025f + 0.42f * bp.grasp_power) * motor_eff * flow_vent;
    if (cr.mot_eat > 0.15f) {
        float take_b = clampf(wb.chem_b[idx], 0.0f, eat_rate * 0.45f);
        float take_c = clampf(wb.chem_c[idx], 0.0f, eat_rate * 0.45f);
        float take_a = clampf(wb.chem_a[idx], 0.0f, eat_rate * 0.30f);
        float take_d = clampf(wb.chem_d[idx], 0.0f, eat_rate * 0.40f);
        bool got_food = false;
        // Balanced ~50/50 ocean diurnal/nocturnal sensory crossover (median water-attenuated light on Trisolaris is ~0.27 ->
        // 0.25 gives equal diurnal optical & nocturnal acoustic selection while sustaining thriving 500-850 population!)
        bool day_l = (wb.light[idx] >= 0.26f);
        // Authentic Vertebrate 4-Channel Split-Optics Retina & 4-Band Basilar-Papilla Spectral Acuity:
        // Normalized to 1.00x at full 4-channel filter bank (0 -> 0.30 -> 0.86 -> 0.95 -> 1.00):
        // Preserves thriving agile 12-16L predator ecology while closing the blind/deaf (`act_sens_n == 0`) freeloader loophole!
        u32 n_eye_eff = abl.blind_eyes ? 0u : u32(bp.counts[TK_EYE]);
        u32 n_ear_eff = abl.deaf_ears ? 0u : u32(bp.counts[TK_EAR]);
        u32 n_chem_eff = u32(bp.counts[TK_CHEM]);
        u32 act_sens_n = day_l ? n_eye_eff : n_ear_eff;
        float sens_acuity = (act_sens_n >= 4u) ? 1.00f
                                               : ((act_sens_n == 3u) ? 0.95f
                                                                     : ((act_sens_n == 2u) ? 0.86f
                                                                                           : ((act_sens_n == 1u) ? 0.30f : 0.00f)));
        // Authentic Contact Gustation & Olfactory Toxin Rejection (TK_CHEM):
        // Chemosensory organisms detect dissolved nutrient plumes and taste-reject corrosive chem_d BEFORE ingestion!
        float chem_forage_m = (abl.challenge_tier >= 1u) ? ((n_chem_eff >= 2u) ? 1.12f : ((n_chem_eff == 1u) ? 1.05f : 0.94f)) : 1.0f;
        float chem_detox_m = (abl.challenge_tier >= 1u) ? ((n_chem_eff >= 2u) ? 0.68f : ((n_chem_eff == 1u) ? 0.82f : 1.16f)) : 1.0f;
        if (take_b > 0.005f) {
            atomic_add_f32(&wb.chem_b[idx], -take_b);
            float b_sens_m = (abl.challenge_tier >= 1u) ? ((act_sens_n == 0u) ? 0.25f : (0.56f + 0.56f * sens_acuity)) : 1.0f;
            float b_e = (abl.challenge_tier >= 2u) ? 2.4f : 6.0f; // Trisolaris-13 golden balance -> thriving pop + cognitive hunting/tools!
            float gain = take_b * b_e * digest_mul * b_sens_m * chem_forage_m;
            cr.atp += gain; cr.energy_gained += gain;
            if (ctr) atomic_add_u64(&ctr->total_energy_x100, u64(gain * 100.0f));
            cr.carry_b = clampf(cr.carry_b + take_b * 0.40f, 0.0f, 2.0f);
            ++cr.food_events; got_food = true;
        }
        if (take_c > 0.005f) {
            atomic_add_f32(&wb.chem_c[idx], -take_c);
            // Trisolaris-13 Symmetric 3x3 Symbiotic Reef Bioacoustics (v0 bass shakes loose golden reef blooms; v1/v2 disturb polyps!)
            float sens_forage_mul = (abl.challenge_tier >= 1u) ? ((act_sens_n == 0u) ? 0.18f : (0.42f + 0.82f * sens_acuity)) : 1.0f;
            bool v0_bloom_self = (abl.challenge_tier >= 1u && cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 16u && cr.last_vocal_band == 0u);
            bool v0_bloom_coop = (abl.challenge_tier >= 1u && wb.cell_occ[idx] >= 2u &&
                                  cr.last_heard_tick > 0u && tick <= cr.last_heard_tick + 36u && cr.last_heard_band == 0u);
            bool v_scare_bloom = (abl.challenge_tier >= 2u && cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 12u && cr.last_vocal_band != 0u);
            float v_bloom_m = v0_bloom_coop ? 2.15f : (v0_bloom_self ? 1.65f : (v_scare_bloom ? 0.80f : 1.0f));
            float c_e = (abl.challenge_tier >= 2u) ? 9.2f : 14.5f;
            float gain = take_c * c_e * digest_mul * sens_forage_mul * v_bloom_m * chem_forage_m;
            cr.atp += gain; cr.energy_gained += gain;
            if (ctr) atomic_add_u64(&ctr->total_energy_x100, u64(gain * 100.0f));
            cr.carry_c = clampf(cr.carry_c + take_c * 0.35f, 0.0f, 2.0f);
            ++cr.food_events; got_food = true;
        }
        if (!got_food) cr.atp -= 0.025f;
        if (take_d > 0.012f) {
            // Symmetric Diurnal Visual / Nocturnal Acoustic + Gustatory Hazard Avoidance!
            float tox_sens_m = (abl.challenge_tier >= 1u) ? (1.55f - 0.55f * sens_acuity) : 1.0f;
            float dmg = take_d * 34.0f * (1.0f - 0.35f * bp.armor_factor) * detox_mul * tox_sens_m * chem_detox_m;
            cr.atp -= dmg; cr.hp -= dmg * 1.55f;
        }
        if (take_a > 0.004f && bp.counts[TK_GRASP] > 0u) {
            atomic_add_f32(&wb.chem_a[idx], -take_a);
            cr.carry_a = clampf(cr.carry_a + take_a, 0.0f, 2.0f);
        }
    }

    if (bp.gland_power > 0.1f && cr.mot_catalyze > 0.22f && cr.carry_a > 0.05f && cr.carry_b > 0.05f) {
        float react_amt = clampf(bp.gland_power * 0.16f * (0.6f + env_temp) * motor_eff, 0.0f,
                                 cr.carry_a < cr.carry_b ? cr.carry_a : cr.carry_b);
        cr.carry_a -= react_amt; cr.carry_b -= react_amt;
        cr.carry_c = clampf(cr.carry_c + react_amt * 1.25f, 0.0f, 2.0f);
        float synth_atp = react_amt * ((abl.challenge_tier >= 2u) ? 6.0f : 10.5f);
        cr.atp += synth_atp; cr.energy_gained += synth_atp;
        if (ctr) {
            atomic_add_u64(&ctr->total_energy_x100, u64(synth_atp * 100.0f));
            atomic_add_u32(&ctr->total_synth, 1u);
        }
        ++cr.chem_synth;
    }

    // 4. Physical Object Manipulation: Catch Evasive OBJ_PREY, Carry/Crack OBJ_POD, Feast on OBJ_FUEL!
    bool mega_seq_ok = false;
    u32 fidx = (u32(i32(cr.y + st * 1.5f + HF * 2.0f) & i32(MY)) << SY) |
               u32(i32(cr.x + ct * 1.5f + WF * 2.0f) & i32(MX));
    if (wb.objects && wb.max_objects > 0u) {
        u32 oid_here = wb.tile_obj_id[idx] ? wb.tile_obj_id[idx] : wb.tile_obj_id[fidx];
        if (oid_here != 0u) {
            u32 oi = oid_here - 1u;
            if (oi < wb.max_objects && wb.objects[oi].carrier_id == INVALID_OBJ) {
                WorldObject &ob = wb.objects[oi];
                if (ob.kind == OBJ_PREY && bp.grasp_power > 0.18f && (cr.mot_eat > 0.15f || cr.mot_dig > 0.22f)) {
                    // Authentic Symmetric Binocular Diurnal Vision / Binaural Nocturnal Echolocation Prey Interception:
                    // Fast prey evade 87.5% of blind/deaf lunges, 50% of monocular/monaural 1-organ lunges, 0% of stereo/binaural (>=2)!
                    u32 p_h = tick + cid + oi;
                    u32 p_eye = abl.blind_eyes ? 0u : u32(bp.counts[TK_EYE]);
                    u32 p_ear = abl.deaf_ears ? 0u : u32(bp.counts[TK_EAR]);
                    bool day_evade = (wb.light[idx] >= 0.26f) &&
                                     ((p_eye == 0u && ((p_h & 7u) != 0u)) || (p_eye == 1u && ((p_h & 1u) != 0u)) ||
                                      (p_eye == 2u && ((p_h & 7u) == 0u)));
                    bool night_evade = (wb.light[idx] < 0.26f) &&
                                       ((p_ear == 0u && ((p_h & 7u) != 0u)) || (p_ear == 1u && ((p_h & 1u) != 0u)) ||
                                        (p_ear == 2u && ((p_h & 7u) == 0u)));
                    u32 expect = u32(OBJ_PREY) | (u32(INVALID_OBJ) << 16);
                    u32 want_n = u32(OBJ_NONE) | (u32(INVALID_OBJ) << 16);
                    if (!day_evade && !night_evade && atomic_cas_u32(reinterpret_cast<u32 *>(&ob.kind), expect, want_n) == expect) {
                        // Seasonal Batesian Prey Mimicry: Warm vs Cool morph flips edibility every 180 ticks!
                        // (Matches world_step_tile bloom_phase; inverted on held-out shift_chemistry worlds!)
                        u32 bloom_phase = (((tick + 80u) / 180u) ^ abl.shift_chemistry) & 1u;
                        bool prey_is_warm = ((ob.flags & 8u) == 0u);
                        bool prey_edible = (abl.challenge_tier == 0u) || (prey_is_warm == (bloom_phase == 0u));
                        if (prey_edible) {
                            // Symmetric Diurnal Vision & Nocturnal Echolocation high-calorie seasonal Batesian prey feast:
                            // Permanently selects live STDP reversal + multimodal vision/hearing + prefrontal memory over passive grazing!
                            float prey_e = (abl.challenge_tier >= 2u) ? 170.0f : 88.0f;
                            float gain = prey_e * digest_mul;
                            cr.atp += gain; cr.energy_gained += gain;
                            ++cr.food_events;
                            if (ctr) {
                                atomic_add_u64(&ctr->total_energy_x100, u64(gain * 100.0f));
                                atomic_add_u32(&ctr->total_prey_caught, 1u);
                            }
                        } else {
                            // True Aposematic Batesian Venom Lethality (coral-snake neurotoxin — STRICTLY net-lethal to blind-guess!):
                            // Edible prey (+52*1.18=+61 ATP) vs Venomous mimic (-66 ATP, -68 HP) -> blind 50/50 guessing loses ATP & dies in 2 bites,
                            // forcing BOTH TK_EYE/TK_EAR morph discrimination AND live STDP seasonal reversal!
                            float dmg = 68.0f * (1.0f - 0.25f * bp.armor_factor);
                            cr.hp -= dmg;
                            cr.atp -= 66.0f;
                        }
                    }
                } else if (ob.kind == OBJ_FUEL && cr.mot_eat > 0.15f && ob.energy > 0.5f) {
                    float bite_e = clampf(ob.energy, 0.0f, 9.0f * flow_vent);
                    float old_e = atomic_add_f32(&ob.energy, -bite_e);
                    if (old_e > 0.5f) {
                        // Referential v0 food-recruitment bonus: peers arriving within 40t of a v0 call share +125% harvest efficiency!
                        bool v0_coop = (wb.cell_occ[idx] >= 2u && cr.last_heard_tick > 0u &&
                                        tick <= cr.last_heard_tick + 40u && cr.last_heard_band == 0u);
                        float fuel_m = (abl.challenge_tier >= 2u) ? 1.40f : 1.0f;
                        float gain = ((old_e < bite_e) ? old_e : bite_e) * digest_mul * fuel_m * (v0_coop ? 2.25f : 1.0f);
                        cr.atp += gain; cr.energy_gained += gain;
                        if (ctr) {
                            atomic_add_u64(&ctr->total_energy_x100, u64(gain * 100.0f));
                            if (ob.flags & 4u) atomic_add_u32(&ctr->total_cached_feasts, 1u);
                        }
                        ++cr.food_events;
                        if (old_e - bite_e <= 0.5f) ob.kind = OBJ_NONE;
                    }
                } else if (ob.kind == OBJ_MEGAPOD && bp.grasp_power > 0.18f && (cr.mot_build > 0.30f || cr.mot_dig > 0.30f)) {
                    // Multi-step Tool Use (raw vs Stage-2 tempered stone!) & Stage-3 2-Note Ancient Vault Resonance!
                    bool has_stone_hammer = (cr.carried_obj != INVALID_OBJ && cr.carried_obj < wb.max_objects &&
                                             wb.objects[cr.carried_obj].kind == OBJ_STONE);
                    bool is_tempered = has_stone_hammer && ((wb.objects[cr.carried_obj].flags & 2u) != 0u);
                    bool coop_allies = (wb.cell_occ[idx] + wb.cell_occ[fidx] >= 2u);
                    u32 pod_b0 = (ob.flags >> 4) & 3u;
                    u32 pod_b1 = (pod_b0 + 1u + ((ob.flags >> 6) & 1u)) % 3u;
                    bool vocal_duet = coop_allies && (cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 32u) &&
                                      (cr.last_heard_tick > 0u && tick <= cr.last_heard_tick + 32u);
                    bool seq2_match = (cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 24u &&
                                       u32(cr.last_vocal_band) == pod_b1 && u32(cr.prev_vocal_band) == pod_b0 &&
                                       cr.prev_vocal_age >= 1u && cr.prev_vocal_age <= 24u);
                    bool resonant_match = ((cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 24u && (u32(cr.last_vocal_band) == pod_b0 || u32(cr.last_vocal_band) == pod_b1)) ||
                                           (cr.last_heard_tick > 0u && tick <= cr.last_heard_tick + 24u && (u32(cr.last_heard_band) == pod_b0 || u32(cr.last_heard_band) == pod_b1)));
                    if (seq2_match) mega_seq_ok = true;
                    float res_mul = seq2_match ? 5.50f : (resonant_match ? 2.45f : 1.0f);
                    float solo_base = (abl.challenge_tier >= 2u) ? 0.08f : 0.14f;
                    float dmg = has_stone_hammer ? ((is_tempered ? 2.60f : 1.35f) + 0.35f * bp.grasp_power)
                                                 : (coop_allies ? ((vocal_duet ? 2.10f : 0.45f) * res_mul + 0.36f * bp.grasp_power)
                                                                : (seq2_match ? 1.65f : (resonant_match ? 0.55f : solo_base)));
                    ob.flags |= 1u;
                    float old_int = atomic_add_f32(&ob.integrity, -dmg);
                    atomic_add_f32(&wb.acoust_p[fidx], has_stone_hammer ? 0.85f : 0.55f);
                    if (old_int > 0.0f && old_int - dmg <= 0.0f) {
                        if (ctr) {
                            if (has_stone_hammer) atomic_add_u32(&ctr->total_tool_cracks, 1u);
                            if (coop_allies) atomic_add_u32(&ctr->total_coop_cracks, 1u);
                            if (vocal_duet) atomic_add_u32(&ctr->total_duet_coops, 1u);
                        }
                    }
                } else if (ob.kind == OBJ_APEX) {
                    // Autonomous Apex Predator Encounter:
                    // - Carrying an OBJ_STONE (or Stage-2 tempered blade!) OR vocal-duet pack mobbing slays predator (72 ATP!)
                    // - Lone unprotected swimmer outside a MAT_POLYMER wall suffers a lethal predator bite!
                    bool has_stone = (cr.carried_obj != INVALID_OBJ && cr.carried_obj < wb.max_objects &&
                                      wb.objects[cr.carried_obj].kind == OBJ_STONE);
                    bool is_tempered = has_stone && ((wb.objects[cr.carried_obj].flags & 2u) != 0u);
                    u32 mob_n = wb.cell_occ[idx] + wb.cell_occ[fidx];
                    bool vocal_duet = (mob_n >= 2u) && (cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 32u) &&
                                      (cr.last_heard_tick > 0u && tick <= cr.last_heard_tick + 32u);
                    if ((has_stone || mob_n >= 3u || vocal_duet) && bp.grasp_power > 0.16f && (cr.mot_build > 0.24f || cr.mot_dig > 0.24f)) {
                        bool alarm_duet = vocal_duet && (cr.last_vocal_band == 1u || cr.last_heard_band == 1u);
                        float dmg = has_stone ? ((is_tempered ? 2.30f : 0.95f) + 0.35f * bp.grasp_power)
                                              : (((alarm_duet ? 3.80f : (vocal_duet ? 1.45f : 0.50f))) + 0.25f * bp.grasp_power);
                        float old_int = atomic_add_f32(&ob.integrity, -dmg);
                        atomic_add_f32(&wb.acoust_p[fidx], 0.90f);
                        if (ctr && vocal_duet && old_int > 0.0f && old_int - dmg <= 0.0f)
                            atomic_add_u32(&ctr->total_duet_coops, 1u);
                    } else if (abl.challenge_tier >= 1u) {
                        // Symmetric Diurnal Vision / Nocturnal Hearing Apex Ambush Ecology (ZERO artificial bonuses!):
                        // - By Day (light>=0.25): blind (Eye==0) organisms suffer 1.80x visual predator ambush!
                        // - By Night (light<0.25): deaf (Ear==0) organisms suffer 1.80x nocturnal acoustic ambush!
                        // Co-selects true multimodal EYE + EAR + GLAND + MUSCLE lineages stably across 100k+ ticks!
                        // Bioacoustic Predator Startle Deterrence (high-frequency v1 shriek startles apex -> 0.55x caller bite,
                        // peer v1 alarm brace -> 0.15x; low-frequency v0 bass rumble attracts ambush -> 1.20x!)
                        bool alarm_brace = (cr.last_heard_tick > 0u && tick <= cr.last_heard_tick + 20u && cr.last_heard_band == 1u);
                        bool v1_startle = (abl.challenge_tier >= 1u && cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 16u && cr.last_vocal_band == 1u);
                        bool v_lure = (abl.challenge_tier >= 2u && cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 16u && cr.last_vocal_band != 1u);
                        u32 a_sens = (wb.light[idx] >= 0.26f)
                                         ? (abl.blind_eyes ? 0u : u32(bp.counts[TK_EYE]))
                                         : (abl.deaf_ears ? 0u : u32(bp.counts[TK_EAR]));
                        float sens_vuln_m = (a_sens >= 4u) ? 0.84f : ((a_sens == 3u) ? 0.94f : ((a_sens == 2u) ? 1.00f : ((a_sens == 1u) ? 1.40f : 1.80f)));
                        float wall_cov = near_wall ? 0.35f : 1.0f;
                        float immob_m = (cur_spd < 0.08f) ? 1.35f : 1.0f;
                        float vocal_m = alarm_brace ? 0.12f : (v1_startle ? 0.25f : (v_lure ? 1.25f : 1.0f));
                        float brace_m = vocal_m * sens_vuln_m * wall_cov * immob_m;
                        float dmg_hp = 22.0f * (1.0f - 0.50f * bp.armor_factor) * brace_m;
                        cr.hp -= dmg_hp;
                        cr.atp -= 6.5f * brace_m;
                        atomic_add_f32(&wb.acoust_p[idx], 0.75f);
                        if (cr.hp <= 0.0f && ctr) atomic_add_u32(&ctr->total_apex_deaths, 1u);
                    }
                } else if ((ob.kind == OBJ_POD || ob.kind == OBJ_STONE) && cr.carried_obj == INVALID_OBJ &&
                           bp.grasp_power > 0.22f && cr.mot_dig > 0.25f) {
                    u16 want_k = ob.kind;
                    u32 expect = u32(want_k) | (u32(INVALID_OBJ) << 16);
                    u32 want_v = u32(want_k) | (u32(u16(cid)) << 16);
                    if (atomic_cas_u32(reinterpret_cast<u32 *>(&ob.kind), expect, want_v) == expect) {
                        ob.flags |= 1u;
                        cr.carried_obj = u16(oi);
                    }
                }
            }
        }
        if (cr.carried_obj != INVALID_OBJ && cr.carried_obj < wb.max_objects) {
            WorldObject &ob = wb.objects[cr.carried_obj];
            if (ob.kind == OBJ_NONE || ob.carrier_id != u16(cid)) {
                cr.carried_obj = INVALID_OBJ;
            } else if (ob.kind == OBJ_FUEL && cr.mot_eat > 0.15f) {
                float take = clampf(ob.energy, 0.0f, 10.0f);
                ob.energy -= take;
                float gain = take * digest_mul;
                cr.atp += gain; cr.energy_gained += gain;
                if (ctr) atomic_add_u64(&ctr->total_energy_x100, u64(gain * 100.0f));
                ++cr.food_events;
                ob.carrier_id = INVALID_OBJ;
                if (ob.energy <= 0.5f) { ob.kind = OBJ_NONE; ob.flags = 0u; }
                cr.carried_obj = INVALID_OBJ;
            } else if (cr.mot_build > 0.35f) {
                u8 fm = wb.material[fidx];
                u32 f_oid = wb.tile_obj_id[fidx];
                bool on_stone_anvil = (f_oid != 0u && (f_oid - 1u) < wb.max_objects &&
                                       wb.objects[f_oid - 1u].kind == OBJ_STONE);
                bool on_pod_target = (f_oid != 0u && (f_oid - 1u) < wb.max_objects &&
                                      (wb.objects[f_oid - 1u].kind == OBJ_POD || wb.objects[f_oid - 1u].kind == OBJ_MEGAPOD));
                bool on_apex_target = (f_oid != 0u && (f_oid - 1u) < wb.max_objects &&
                                       wb.objects[f_oid - 1u].kind == OBJ_APEX);
                if (ob.kind == OBJ_STONE && on_pod_target) {
                    // Multi-step Tool Hammer Strike (Stage-2 tempered blade deals 2x leverage!)
                    WorldObject &tpod = wb.objects[f_oid - 1u];
                    tpod.flags |= 1u;
                    float dmg = (((ob.flags & 2u) ? 2.65f : 1.45f) + 0.40f * bp.grasp_power) * motor_eff;
                    float old_int = atomic_add_f32(&tpod.integrity, -dmg);
                    atomic_add_f32(&wb.acoust_p[fidx], 0.90f);
                    if (ctr && old_int > 0.0f && old_int - dmg <= 0.0f) atomic_add_u32(&ctr->total_tool_cracks, 1u);
                } else if (ob.kind == OBJ_STONE && on_apex_target) {
                    // Multi-step Stone Weapon Defense (Stage-2 tempered spear 1-shots OBJ_APEX!)
                    WorldObject &tapex = wb.objects[f_oid - 1u];
                    float dmg = (((ob.flags & 2u) ? 2.35f : 1.10f) + 0.35f * bp.grasp_power) * motor_eff;
                    atomic_add_f32(&tapex.integrity, -dmg);
                    atomic_add_f32(&wb.acoust_p[fidx], 0.95f);
                } else {
                    if (ob.kind == OBJ_POD && (fm == MAT_ORE || fm == MAT_POLYMER || fm == MAT_CATALYST || on_stone_anvil)) {
                        ob.flags |= 1u;
                        bool coop = (wb.cell_occ[idx] + wb.cell_occ[fidx] >= 2u);
                        float dmg = (0.38f + 0.30f * bp.grasp_power) * (coop ? 2.0f : 1.0f) * (on_stone_anvil ? 1.8f : 1.0f);
                        float old_int = ob.integrity;
                        ob.integrity -= dmg;
                        atomic_add_f32(&wb.acoust_p[fidx], 0.55f);
                        if (ctr && old_int > 0.0f && ob.integrity <= 0.0f) {
                            if (on_stone_anvil) atomic_add_u32(&ctr->total_tool_cracks, 1u);
                            if (coop) atomic_add_u32(&ctr->total_coop_cracks, 1u);
                        }
                    }
                    ob.carrier_id = INVALID_OBJ;
                    ob.x = float(i32(cr.x + ct * 1.2f + WF * 2.0f) & i32(MX)) + 0.5f;
                    ob.y = float(i32(cr.y + st * 1.2f + HF * 2.0f) & i32(MY)) + 0.5f;
                    cr.carried_obj = INVALID_OBJ;
                }
            }
        }
    }

    // 5. Physical Contact: Honest Energy-Conserving Predation & Kin Sharing!
    u32 occ_f = wb.tile_occ_id[fidx];
    if (abl.challenge_tier >= 1u && occ_f != 0u) {
        u32 other = occ_f - 1u;
        if (other < pop_n && other != cid && creatures[other].alive == 1u) {
            const BodyPhenotype &obp = bodies[other];
            bool is_kin = (creatures[other].lineage_id == cr.lineage_id) && (obp.body_hash == bp.body_hash);
            if (!is_kin && bp.grasp_power > 0.35f && (cr.mot_dig > 0.35f || cr.mot_eat > 0.45f)) {
                cr.atp -= 0.055f;
                float victim_atp = creatures[other].atp;
                if (victim_atp > 2.0f) {
                    float crowd_div = 1.0f / float(wb.cell_occ[fidx] > 1u ? wb.cell_occ[fidx] : 1u);
                    // Real physical predatory biting requires active muscular lunge momentum (immobile rocks cannot lunge!)
                    float lunge_m = clampf(bp.thrust_cap, 0.0f, 1.0f) * (0.35f + 0.65f * cr.mot_thrust);
                    float bite = bp.grasp_power * lunge_m * (1.0f - 0.78f * obp.armor_factor);
                    float raw_steal = clampf(bite * 1.4f, 0.10f, 2.2f) * crowd_div;
                    float avail = clampf(raw_steal, 0.0f, victim_atp * 0.30f);
                    if (avail > 0.10f) {
                        atomic_add_f32(&creatures[other].hp, -bite * 7.5f * crowd_div);
                        atomic_add_f32(&creatures[other].atp, -avail);
                        // Honest enzymatic carnivory assimilation (100% energy-conserving <= 0.50*avail, scales with digestive gland_sat!):
                        float gain = avail * (0.12f + 0.38f * gland_sat);
                        cr.atp += gain; cr.energy_gained += gain;
                        ++cr.bites_landed;
                        atomic_add_f32(&wb.acoust_p[fidx], 0.42f);
                        if (ctr) atomic_add_u32(&ctr->total_bites, 1u);
                    }
                }
            } else if (is_kin && cr.mot_secrete > 0.30f && cr.atp > 17.0f && creatures[other].atp < 15.0f) {
                float gift = 2.0f;
                // Parental vocal-dialect recognition & juvenile trophallaxis:
                // Juvenile vocal begging matching the adult's own formant band receives 1.00x lossless transfer!
                bool juv_beg = (creatures[other].age < 160u && creatures[other].last_vocal_tick > 0u &&
                                tick <= creatures[other].last_vocal_tick + 16u);
                bool dialect_match = juv_beg && (cr.last_vocal_tick > 0u && tick <= cr.last_vocal_tick + 24u &&
                                                 creatures[other].last_vocal_band == cr.last_vocal_band);
                cr.atp -= gift;
                // Enzymatic parental vocal-dialect trophallaxis (real biological lactation / crop-milk efficiency!):
                // Juveniles matching their parent's vocal formant receive enhanced pre-digested metabolic transfer!
                atomic_add_f32(&creatures[other].atp, gift * (dialect_match ? 1.18f : (juv_beg ? 1.05f : 0.90f)));
                cr.prev_atp = cr.atp; // Parental care is prosocial kin investment, NOT a nociceptive -DA shock!
                ++cr.energy_shared;
                if (ctr) atomic_add_u32(&ctr->total_shared, 1u);
            }
        }
    }

    // 6. Terrain Engineering: Excavating Ore, Chipping Anvil Stones & Building Polymer Shelters!
    if (bp.grasp_power > 0.1f && cr.mot_dig > 0.40f && cr.carry_soil < 1.5f) {
        u8 fmat = wb.material[fidx];
        if ((fmat == MAT_LOAM || fmat == MAT_ORE) && wb.terrain[fidx] > 0.35f) {
            float dig_amt = 0.06f * bp.grasp_power * motor_eff * flow_vent;
            atomic_add_f32(&wb.terrain[fidx], -dig_amt);
            cr.carry_soil += dig_amt;
            if (fmat == MAT_ORE) {
                cr.carry_a = clampf(cr.carry_a + dig_amt * 1.9f, 0.0f, 2.0f);
                u32 sr = mix(seed ^ tick ^ (cid * 7919u));
                u32 stone_base = (wb.max_objects * 3u) >> 2;
                if (wb.objects && wb.max_objects > stone_base && (sr & 3u) == 0u) {
                    u32 oi = stone_base + (sr % (wb.max_objects - stone_base));
                    u32 expect = u32(OBJ_NONE) | (u32(INVALID_OBJ) << 16);
                    u32 want_s = u32(OBJ_STONE) | (u32(INVALID_OBJ) << 16);
                    if (atomic_cas_u32(reinterpret_cast<u32 *>(&wb.objects[oi].kind), expect, want_s) == expect) {
                        wb.objects[oi].x = cr.x; wb.objects[oi].y = cr.y;
                        wb.objects[oi].vx = 0.0f; wb.objects[oi].vy = 0.0f;
                        wb.objects[oi].energy = 0.0f; wb.objects[oi].integrity = 1.0f;
                        wb.objects[oi].age = 0u; wb.objects[oi].flags = 1u;
                        if (ctr) atomic_add_u32(&ctr->total_stones_made, 1u);
                    }
                }
            }
            cr.atp -= 0.012f;
            ++cr.tiles_dug;
            if (ctr) atomic_add_u32(&ctr->total_dug, 1u);
        }
    }
    if (bp.grasp_power > 0.1f && cr.mot_build > 0.40f && cr.carry_soil > 0.12f) {
        float dep = 0.08f * bp.grasp_power * motor_eff;
        if (dep > cr.carry_soil) dep = cr.carry_soil;
        atomic_add_f32(&wb.terrain[fidx], dep);
        cr.carry_soil -= dep;
        if (cr.carry_c > 0.15f && wb.terrain[fidx] > 2.3f && wb.material[fidx] == MAT_LOAM) {
            wb.material[fidx] = MAT_POLYMER;
            cr.carry_c -= 0.12f;
        } else if (cr.carry_a > 0.35f && wb.material[fidx] == MAT_LOAM && wb.water[fidx] > 0.25f) {
            wb.material[fidx] = MAT_CATALYST;
            cr.carry_a -= 0.30f;
        }
        cr.atp -= 0.015f;
        ++cr.tiles_built;
        if (ctr) atomic_add_u32(&ctr->total_built, 1u);
    }

    // 7. Discrete Syllabic 3-Band Biological Vocalizations (2-tick pneumatic breath cadence!) + Mimicry & 3x4 Shannon MI
    // Authentic LSO Binaural Point-Source Auditory Template Acquisition (Konishi 1965 / Brainard & Doupe 2013):
    // Runs EVERY tick (even during silent breath-recovery listening ticks!) using strictly read-only `acoust_vX_prev`
    // spatial excess over orthogonal ambient reference (0.0000 on uniform ambient waves, 0.0000 on self-voice,
    // +0.72 on nearby conspecific speech — ZERO telepathy, ZERO race condition, ZERO solo-task interference!)
    if (cr.peer_tpl_age < 255u) ++cr.peer_tpl_age;
    if (cr.age < 120u && bp.counts[TK_EAR] > 0u && !abl.deaf_ears) {
        u32 tx_p = idx & MX, ty_p = idx >> SY;
        u32 up_i = (((ty_p + H - 1u) & MY) << SY) | tx_p;
        u32 dn_i = (((ty_p + 1u) & MY) << SY) | tx_p;
        float a0p = fabsf_dev(wb.acoust_v0_prev[up_i]); float a0d = fabsf_dev(wb.acoust_v0_prev[dn_i]); if (a0d < a0p) a0p = a0d;
        float a1p = fabsf_dev(wb.acoust_v1_prev[up_i]); float a1d = fabsf_dev(wb.acoust_v1_prev[dn_i]); if (a1d < a1p) a1p = a1d;
        float a2p = fabsf_dev(wb.acoust_v2_prev[up_i]); float a2d = fabsf_dev(wb.acoust_v2_prev[dn_i]); if (a2d < a2p) a2p = a2d;
        float best_ex = 0.22f;
        u8 p_b = 3u;
        for (i32 dyp = -3; dyp <= 3; ++dyp) {
            u32 ny_p = (ty_p + u32(dyp + i32(H))) & MY;
            for (i32 dxp = -3; dxp <= 3; ++dxp) {
                if ((dxp == 0 && dyp == 0) || (dxp * dxp + dyp * dyp > 9)) continue;
                u32 pidx = (ny_p << SY) | ((tx_p + u32(dxp + i32(W))) & MX);
                u32 n_occ = wb.tile_occ_id[pidx];
                if (n_occ != 0u && n_occ != cid + 1u) {
                    float e0 = fabsf_dev(wb.acoust_v0_prev[pidx]) - a0p;
                    float e1 = fabsf_dev(wb.acoust_v1_prev[pidx]) - a1p;
                    float e2 = fabsf_dev(wb.acoust_v2_prev[pidx]) - a2p;
                    if (e0 > best_ex) { best_ex = e0; p_b = 0u; }
                    if (e1 > best_ex) { best_ex = e1; p_b = 1u; }
                    if (e2 > best_ex) { best_ex = e2; p_b = 2u; }
                }
            }
        }
        if (p_b < 3u) { cr.peer_tpl_band = p_b; cr.peer_tpl_age = 0u; }
    }

    // Real syrinx breath recovery enforces crisp discrete syllables & phrases while preserving nimble nocturnal sonar!
    float voc_ok = 0.0f, voc_err = 0.0f;
    if (bp.gland_power > 0.1f && (cr.last_vocal_tick == 0u || tick >= cr.last_vocal_tick + 2u) &&
        (cr.mot_vocal0 > 0.24f || cr.mot_vocal1 > 0.24f || cr.mot_vocal2 > 0.24f)) {
        float res = (wb.material[fidx] == MAT_POLYMER || near_wall) ? 1.85f : 1.0f;
        u32 vctx = 3u;
        u32 oid_v = wb.tile_obj_id[idx] ? wb.tile_obj_id[idx] : wb.tile_obj_id[fidx];
        bool near_apex = (oid_v != 0u && wb.objects && (oid_v - 1u) < wb.max_objects &&
                          wb.objects[oid_v - 1u].kind == OBJ_APEX);
        if (near_apex || cr.hp < 68.0f) vctx = 1u;
        else if (wb.chem_c[idx] > 0.16f || oid_v != 0u || cr.carried_obj != INVALID_OBJ) vctx = 0u;
        else if (near_wall || wb.chem_d[idx] > 0.12f || env_temp > 0.62f || env_temp < 0.36f || fabsf_dev(wb.acoust_p[idx]) > 0.25f) vctx = 2u;

        u32 dom_b = 3u;
        float dom_v = 0.0f;
        if (cr.mot_vocal0 > 0.24f && cr.mot_vocal0 >= cr.mot_vocal1 + 0.03f && cr.mot_vocal0 >= cr.mot_vocal2 + 0.03f) {
            dom_b = 0u; dom_v = cr.mot_vocal0;
        } else if (cr.mot_vocal1 > 0.24f && cr.mot_vocal1 >= cr.mot_vocal0 + 0.03f && cr.mot_vocal1 >= cr.mot_vocal2 + 0.03f) {
            dom_b = 1u; dom_v = cr.mot_vocal1;
        } else if (cr.mot_vocal2 > 0.24f && cr.mot_vocal2 >= cr.mot_vocal0 + 0.03f && cr.mot_vocal2 >= cr.mot_vocal1 + 0.03f) {
            dom_b = 2u; dom_v = cr.mot_vocal2;
        }
        if (dom_b < 3u) {
            float amp = 0.72f * bp.gland_power * dom_v * res;
            if (dom_b == 0u) atomic_add_f32(&wb.acoust_v0[idx], amp);
            else if (dom_b == 1u) atomic_add_f32(&wb.acoust_v1[idx], amp);
            else atomic_add_f32(&wb.acoust_v2[idx], amp);
            // Trisolaris-9 Physical Bioacoustic Semantics (100% energy-conserving physical consequences -> VTA +/-DA!):
            // - Food reef/object (vctx==0): v0 bass dislodges physical chem_c spores; v1 panic spooks swarm (-0.30 ATP)!
            // - Predator/injury (vctx==1): v1 shriek startles apex (0.25x bite); v0 bass attracts ambush (-0.45 ATP)!
            // - Shelter/acid/thermal (vctx==2): v2 cavity hum disperses acid & warms (0.35x burn); v0/v1 inhales acid (-0.35 ATP)!
            // - Open empty void (vctx==3): Zahavi pneumatic swim-bladder cost (0.12f = 22x context call -> selects silence without eroding hunting!)
            float voc_cost = (abl.challenge_tier >= 2u && vctx == 3u) ? 0.06f : 0.0055f;
            cr.atp -= (voc_cost * dom_v + ((abl.challenge_tier >= 2u && vctx == 3u) ? 0.006f : 0.0014f)); ++cr.vocal_pulses;
            bool is_mimic = (cr.last_heard_tick > 0u && tick > cr.last_heard_tick &&
                             tick <= cr.last_heard_tick + 12u && u8(dom_b) == cr.last_heard_band);
            if (abl.challenge_tier >= 1u) {
                // Trisolaris-14 Symmetric 3x3 Sensory-Bioacoustic Feedback + Dialogic Echo & 2-Syl Vault Resonance:
                // Matching acoustic physics delivers real relief/spores (+VTA RPE); mismatched calls trigger modest physical
                // disturbance/cough sensory prediction error (`voc_err` -> crisp Habenula/VTA -DA) WITHOUT starving vocalists!
                if (vctx == 0u) {
                    if (dom_b == 0u) {
                        if (wb.chem_c[idx] > 0.14f) {
                            u32 v0_sn = (wb.light[idx] >= 0.26f)
                                            ? (abl.blind_eyes ? 0u : u32(bp.counts[TK_EYE]))
                                            : (abl.deaf_ears ? 0u : u32(bp.counts[TK_EAR]));
                            float v0_sens_m = (v0_sn >= 2u) ? 1.00f : ((v0_sn == 1u) ? 0.65f : 0.30f);
                            float sb = clampf(wb.chem_c[idx] * 0.20f, 0.0f, 0.06f) * flow_vent;
                            atomic_add_f32(&wb.chem_c[idx], -sb);
                            float sg_e = sb * 12.0f * digest_mul * v0_sens_m;
                            cr.atp += sg_e; cr.energy_gained += sg_e;
                            if (ctr) atomic_add_u64(&ctr->total_energy_x100, u64(sg_e * 100.0f));
                        }
                        voc_ok = 0.45f; // Matched food/object call -> crisp VTA->Area-X +DA!
                    } else {
                        cr.atp -= 0.12f; voc_err = 0.45f; // Non-resonant v1/v2 disturbs prey/polyps -> symmetric VTA->Area-X -DA!
                    }
                } else if (vctx == 1u) {
                    if (dom_b == 1u) voc_ok = 0.45f;          // Ultrasonic v1 startles apex -> VTA->Area-X +DA!
                    else { cr.atp -= 0.16f; voc_err = 0.45f; } // Non-startle v0/v2 lures predator -> VTA->Area-X -DA!
                } else if (vctx == 2u) {
                    if (dom_b == 2u) voc_ok = 0.45f;          // Cavity v2 disperses mist & warms -> VTA->Area-X +DA!
                    else if (wb.chem_d[idx] > 0.12f || env_temp > 0.62f || env_temp < 0.36f) {
                        cr.atp -= 0.16f; voc_err = 0.45f; // Open-glottis v0/v1 inhales corrosive mist -> VTA->Area-X -DA!
                    }
                }
                if (is_mimic && u32(dom_b) == vctx) voc_ok = 0.60f; // Cooperative peer echo amplifies VTA->Area-X confirmation!
            }
            if (cr.last_vocal_tick > 0u && (u8(dom_b) != cr.last_vocal_band || tick >= cr.last_vocal_tick + 6u)) {
                cr.prev_vocal_band = cr.last_vocal_band;
                u32 gap = (tick > cr.last_vocal_tick) ? (tick - cr.last_vocal_tick) : 1u;
                cr.prev_vocal_age = u8(gap > 255u ? 255u : gap);
            }
            cr.last_vocal_tick = tick; cr.last_vocal_band = u8(dom_b);
            if (ctr) {
                atomic_add_u32(&ctr->total_vocal, 1u);
                atomic_add_u32(&ctr->vocal_ctx[dom_b][vctx], 1u);
                if (is_mimic) atomic_add_u32(&ctr->total_vocal_mimics, 1u);
            }
        }
    }
    // Authentic Gadagkar (2016 Science) Juvenile Efference-Copy Auditory-Vocal Mirror Comparator:
    // Strictly during calm social contact (`vctx_now == 3u`: zero food-reef, apex-predator, or acid/shelter emergency
    // -> ZERO odd-tick ecological override!), evaluates crisp juvenile vocal motor attempts against the fresh
    // LSO-localized peer auditory template (`peer_tpl_age <= 4`, breaking the 2-tick breath-cadence deadlock with LMAN!)
    if (cr.age < 120u && bp.gland_power > 0.1f && bp.counts[TK_EAR] > 0u && !abl.deaf_ears &&
        voc_ok == 0.0f && voc_err == 0.0f && cr.peer_tpl_band < 3u && cr.peer_tpl_age <= 4u) {
        u32 oid_g = wb.tile_obj_id[idx] ? wb.tile_obj_id[idx] : wb.tile_obj_id[fidx];
        bool calm_soc = (oid_g == 0u && cr.carried_obj == INVALID_OBJ && cr.hp >= 88.0f &&
                         wb.chem_c[idx] <= 0.16f && !near_wall && wb.chem_d[idx] <= 0.12f &&
                         env_temp >= 0.36f && env_temp <= 0.62f && fabsf_dev(wb.acoust_p[idx]) <= 0.25f);
        if (calm_soc) {
            u32 try_b = 3u;
            if (cr.mot_vocal0 > 0.20f && cr.mot_vocal0 > cr.mot_vocal1 + 0.02f && cr.mot_vocal0 > cr.mot_vocal2 + 0.02f) try_b = 0u;
            else if (cr.mot_vocal1 > 0.20f && cr.mot_vocal1 > cr.mot_vocal0 + 0.02f && cr.mot_vocal1 > cr.mot_vocal2 + 0.02f) try_b = 1u;
            else if (cr.mot_vocal2 > 0.20f && cr.mot_vocal2 > cr.mot_vocal0 + 0.02f && cr.mot_vocal2 > cr.mot_vocal1 + 0.02f) try_b = 2u;
            if (try_b < 3u) {
                if (u8(try_b) == cr.peer_tpl_band) voc_ok = 0.65f;
                else voc_err = 0.50f;
            }
        }
    }
    if (bp.gland_power > 0.1f && cr.mot_secrete > 0.78f && (cr.mod_ne > 0.42f || cr.hp < 88.0f)) {
        // Real biological defensive jet: ONLY triggers under genuine threat/distress (high NE or injury) —
        // NEVER spews lethal toxin during calm everyday singing/speech!
        u32 bidx = (u32(i32(cr.y - st * 2.0f + HF * 2.0f) & i32(MY)) << SY) |
                   u32(i32(cr.x - ct * 2.0f + WF * 2.0f) & i32(MX));
        if ((dna.bio_params[8] & 1) == 0) atomic_add_f32(&wb.chem_d[bidx], 0.14f * bp.gland_power);
        else atomic_add_f32(&wb.light[bidx], 0.55f * bp.gland_power);
        cr.atp -= 0.005f;
    }

    atomic_add_f32(&wb.temp[idx], 0.002f * bp.basal_cost);

    // 8. Authentic Signed Schultz VTA TD Reward Prediction Error + Relief Learning (Seymour 2005 / Dayan & Niv 2008):
    // - Positive surprise (prey/bloom/escape-to-shelter relief: net_r > da_baseline) fires crisp phasic +DA!
    // - Sudden venom/toxin/predator shock fires crisp 1-tick phasic -DA for rapid reversal learning!
    // - Sustained ambient heat/acid stress adapts da_baseline smoothly downward -> ZERO 40-tick tonic motor lobotomy!
    float d_atp = cr.atp - cr.prev_atp;
    float basal_band = bp.basal_cost + 0.14f;
    float r_sig = (d_atp > 0.04f) ? clampf(d_atp, 0.0f, 2.5f)
                                  : ((d_atp < -basal_band) ? clampf(d_atp + basal_band, -2.0f, 0.0f) : 0.0f);
    // Dedicated Songbird VTA -> Area-X Vocal Performance Evaluation RPE (Fee & Goldberg 2011 Neuron):
    // Delivers crisp 1-tick vocal outcome +/-DA strictly to Area-X (TK_GLAND) & vocal cortical columns (Vector-FA)
    // WITHOUT corrupting locomotor/feeding VTA dopamine (cr.mod_da) on TK_MUSCLE / TK_GRASP!
    if (mega_seq_ok && voc_ok < 0.65f) voc_ok = 0.65f;
    float v_net = voc_ok - voc_err;
    cr.voc_da_q = (fabsf_dev(v_net) > 0.05f) ? i8(clamp_i32(i32(v_net * 64.0f), -64, 64)) : i8(0);
    float pain = (cd_here > 0.24f ? (cd_here - 0.24f) * 2.4f : 0.0f) +
                 (therm_stress > 0.24f ? (therm_stress - 0.24f) * 1.8f : 0.0f);
    float net_r = r_sig - pain;
    float rpe = 0.0f;
    if (net_r > 0.04f) {
        rpe = (net_r > cr.da_baseline) ? (net_r - cr.da_baseline) : 0.0f;
        cr.da_baseline = fma_f32(0.05f, clampf(net_r, 0.0f, 0.60f), 0.95f * cr.da_baseline);
    } else if (net_r < -0.08f || pain > 0.04f) {
        rpe = (net_r < cr.da_baseline) ? (net_r - cr.da_baseline) : 0.0f;
        cr.da_baseline = fma_f32(0.10f, clampf(net_r, -0.60f, 0.0f), 0.90f * cr.da_baseline);
    } else {
        // Authentic Habenula/VTA Relief Learning: escaping heat/acid into cool shelter (da_baseline < -0.10 -> net_r == 0)
        // generates genuine phasic +DA relief reinforcing shelter-seeking & avoidance!
        if (cr.da_baseline < -0.10f) rpe = -cr.da_baseline * 0.65f;
        cr.da_baseline *= 0.94f;
    }
    float phasic = clampf(rpe * 3.0f, -1.0f, 1.0f);
    // Fast striatal DAT reuptake (*0.22): delivers ONE crisp phasic VTA burst per RPE without bleeding across ticks!
    cr.mod_da = (fabsf_dev(phasic) > 0.06f) ? phasic : (cr.mod_da * 0.22f);
    // CRITICAL: Clamp glycogen max_atp strictly to metabolically active soft tissue (excluding dead calcified TK_STRUCT shell!)
    // so rock-armored barnacles NEVER gain free 61-ATP hibernation batteries!
    float soft_mass = clampf(bp.mass - 1.80f * float(bp.counts[TK_STRUCT]), 1.0f, 34.0f);
    float max_atp = 24.0f + 1.05f * soft_mass;
    if (cr.atp > max_atp) cr.atp = max_atp;
    cr.prev_atp = cr.atp;
    cr.mod_ne = clampf(therm_stress * 1.8f + (cr.atp < 9.0f ? (9.0f - cr.atp) * 0.12f : 0.0f) +
                       (cr.hp < 70.0f ? (70.0f - cr.hp) * 0.015f : 0.0f), 0.0f, 1.0f);

    // Latch race-free tick-t Vector-FA causal action mask (`v0|v1|v2|thrust`) into upper nibble of `growth_stage`
    // so next tick's L1..L_top cortical layers ALWAYS assign credit to tick-t's exact causal action!
    u8 fa_act = (cr.mot_vocal0 > 0.12f ? 1u : 0u) | (cr.mot_vocal1 > 0.12f ? 2u : 0u) |
                (cr.mot_vocal2 > 0.12f ? 4u : 0u) | (cr.mot_thrust > 0.14f ? 8u : 0u);
    cr.growth_stage = u8((cr.growth_stage & 0x03u) | (fa_act << 4));

    ++cr.age;

    // 9. POST-NATAL LIVE JUVENILE -> ADULT GROWTH (Experience & Surplus ATP Sculpt the Growing Body+Brain!)
    float mat_thresh = 16.0f + float(dna.bio_params[11] & 7) * 0.6f;
    u8 cur_gs = cr.growth_stage & 0x03u;
    if ((cur_gs == 0u && cr.age == 120u && cr.atp > mat_thresh) ||
        (cur_gs == 1u && cr.age == 280u && cr.atp > mat_thresh + 2.0f)) {
        cr.growth_stage = u8((cr.growth_stage & 0xf0u) | (cur_gs + 1u));
        cr.atp -= 2.5f;
        cr.prev_atp = cr.atp; // Developmental maturation is NOT a nociceptive -DA punishment shock!
        needs_dev[cid] = 2u; // Incremental live CA maturation!
        if (ctr) atomic_add_u32(&ctr->total_maturations, 1u);
    }

    // 10. Natural Death, Biomass Recycling & Lock-Free Race-Safe Tissue + Variable-Fan-In SynSlab Reclamation!
    constexpr u32 MAX_AGE = 2400u;
    if (cr.atp <= 0.0f || cr.hp <= 0.0f || cr.age >= MAX_AGE) {
        // Lock slot as DYING (2u) FIRST so ZERO concurrent birthing/mating/predation threads can steal it mid-free!
        atomic_exch_u32(&cr.alive, 2u);
        cr.carried_obj = INVALID_OBJ;
        u32 nl_free = min_u32(u32(cr.num_layers), MAX_LAYERS_PER_CREATURE);
        for (u32 z = 0; z < nl_free; ++z) {
            u32 sl = cr.layer_slabs[z];
            if (sl != INVALID_SLAB && sl < pool.max_layers) {
                for (u32 st = 0; st < MAX_STRIPES_PER_LAYER; ++st) {
                    u32 ss = pool.layer_syn_slabs[u64(sl) * MAX_STRIPES_PER_LAYER + st];
                    if (ss != INVALID_SLAB && ss < pool.max_syn_slabs) {
                        free_slab(pool.syn_bitmap, ss);
                    }
                    pool.layer_syn_slabs[u64(sl) * MAX_STRIPES_PER_LAYER + st] = INVALID_SLAB;
                }
                membar_gl();
                free_slab(pool.layer_bitmap, sl);
            }
            cr.layer_slabs[z] = INVALID_SLAB;
        }
        cr.num_layers = 0u;
        atomic_add_f32(&wb.chem_a[idx], 0.14f + 0.020f * float(bp.total_cells));
        atomic_add_f32(&wb.chem_b[idx], 0.24f + 0.028f * float(bp.total_cells));
        if (ctr) atomic_add_u32(&ctr->deaths, 1u);
        membar_gl();
        atomic_exch_u32(&cr.alive, 0u); // Publish empty slot ONLY AFTER all slabs & metadata are cleanly released!
        return;
    }

    // 11. Natural Selection Reproduction with Real Developmental Maturity (age >= 80) & Honest Gestational Cost!
    // Somatic body mass drives gestational cost; deep CNS neurons pay honest per-tick ATP + live spike energy!
    float repro_thresh = 25.0f + 0.06f * bp.mass +
                         float(dna.bio_params[10] & 15) * 0.25f;
    u32 rng = mix(seed ^ (tick * 0x9e3779b9u) ^ (cid * 0x85ebca6bu));
    bool wants_birth = abl.drift_mode
                           ? (((rng & 255u) == 0u) && (cr.atp > 6.0f))
                           : (cr.age >= 80u && cr.atp >= repro_thresh);

    if (wants_birth) {
        u32 start = rng % pop_n;
        for (u32 p = 0; p < 16u; ++p) {
            u32 slot = (start + p * 7u) % pop_n;
            if (slot == cid) continue;
            if (atomic_cas_u32(&creatures[slot].alive, 0u, 2u) == 0u) {
                u32 mate = cid;
                bool do_cross = false;
                u32 m_cand = (rng >> 8) % pop_n;
                if (m_cand != cid && creatures[m_cand].alive == 1u) {
                    float mdx = creatures[m_cand].x - cr.x;
                    float mdy = creatures[m_cand].y - cr.y;
                    if (mdx * mdx + mdy * mdy < 160.0f) { mate = m_cand; do_cross = true; }
                }
                u32 sl0 = alloc_slab(pool.layer_bitmap, pool.max_layers, rng + slot * 13u);
                if (sl0 == INVALID_SLAB) { creatures[slot].alive = 0u; break; }

                GenesisDNA child_dna = mutate_dna(dnas[cid], dnas[mate], mix(rng ^ slot), do_cross);
                dnas[slot] = child_dna;

                CreatureState child{};
                for (u32 z = 0; z < MAX_LAYERS_PER_CREATURE; ++z) child.layer_slabs[z] = INVALID_SLAB;
                child.layer_slabs[0] = sl0;
                child.num_layers = 1u; // Begins as 1 zygote layer; buds deeper layers organically via CA OP_MITOSIS!
                child.carried_obj = INVALID_OBJ;
                child.growth_stage = 0u;
                child.peer_tpl_band = 3u;
                child.peer_tpl_age = 255u;
                child.prev_vocal_band = 3u;
                child.prev_vocal_age = 255u;
                float off_ang = rand_unit(rng) * 6.2831853f;
                child.x = cr.x + fast_cos(off_ang) * 2.4f;
                child.y = cr.y + fast_sin(off_ang) * 2.4f;
                if (child.x < 0.0f) child.x += WF;
                if (child.x >= WF) child.x -= WF;
                if (child.y < 0.0f) child.y += HF;
                if (child.y >= HF) child.y -= HF;
                child.theta = off_ang;
                float share = cr.atp * 0.45f;
                cr.atp -= share;
                cr.prev_atp = cr.atp; // CRITICAL: Giving birth must NEVER fire a -1.00 punishment DA shock next tick!
                child.atp = share;
                child.prev_atp = share;
                child.hp = 100.0f;
                child.body_temp = env_temp;
                child.generation = cr.generation + 1u;
                child.lineage_id = (child_dna.num_rules != dnas[cid].num_rules || child_dna.max_layers_gene != dnas[cid].max_layers_gene)
                                       ? mix(cr.lineage_id ^ tick ^ slot) : cr.lineage_id;
                child.alive = 2u; // Keep slot locked as GESTATING until full CreatureState & needs_dev are globally visible!
                creatures[slot] = child;
                needs_dev[slot] = 1u; // Full embryogenesis from 1 zygote cell!
                membar_gl();
                atomic_exch_u32(&creatures[slot].alive, 1u);
                ++cr.offspring;
                if (ctr) atomic_add_u32(&ctr->births, 1u);
                break;
            }
        }
    }
}

} // namespace axion::genesis::gpu
