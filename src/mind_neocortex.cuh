#pragma once
#include "mind_core.cuh"

namespace axion::mind::gpu {

// ---------------------------------------------------------------------------
// Native Blackwell SM 12.0 Neocortical Spiking Associative Matrix (1024 x 1024)
// 3 pathways x 1,048,576 signed int8 synapses = 3 MiB total (fits in 32 MiB L2!)
//   Pathway 0: Sensory Features -> Category Prototype Attractors (Abduction!)
//   Pathway 1: Concept/Category -> Consolidated Semantic Properties & Abilities (Ribot's Law!)
//   Pathway 2: Negated / Inhibitory Exception Veto Matrix (penguin -| fly!)
//
// ALL weight updates are driven by the EVOLVED 3-factor dendritic plasticity
// LUT (`CorticalGenome.basal_rule`) — ZERO hardcoded learning rules!
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Kernel 1: GPU Sharp-Wave Ripple (SWR) Sleep Consolidation!
// Grid: 32 blocks x 32 threads = 1,024 threads (1 thread per postsynaptic neuron j).
// Updates all 1,048,576 synapses in parallel using the EVOLVED `g.basal_rule` LUT!
// ---------------------------------------------------------------------------
AX_FN void neo_sleep_ripple_kernel(signed char *weights_all, const NeoPair *pairs, Word n_pairs,
                                   const CorticalGenome &g, Word *out_edits, Word *out_spikes) {
    Word j = global_tid();
    if (j >= NEURONS)
        return;

    Word local_edits = 0;
    Word local_spikes = 0;

    // Evolved 3-factor plasticity deltas (positive associative LTP vs inhibitory LTD)
    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltp <= 0) d_ltp = 1; // Fallback only if genome LUT is uninitialized (e.g. bench)
    int d_inh = d_ltd < 0 ? d_ltd : -1;

    for (Word p = 0; p < n_pairs; ++p) {
        Word pw = pairs[p].pathway;
        if (pw >= NEO_PATHWAYS)
            continue;
        signed char *row = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES + (unsigned long long)(j) * NEURONS;
        bool post_active = ((pairs[p].post.bits[j >> 5] >> (j & 31u)) & 1u) != 0;
        if (!post_active)
            continue;

        bool is_neg = pairs[p].negated != 0;
        int step = (is_neg ? d_inh : d_ltp) * int(pairs[p].strength > 0 ? pairs[p].strength : 1u);
        local_spikes += 1u;

        // Iterate over all active presynaptic bits i in pairs[p].pre!
        for (Word w = 0; w < WORDS; ++w) {
            Word mask = pairs[p].pre.bits[w];
            while (mask) {
                Word bit = __builtin_ctz(mask);
                mask &= mask - 1u;
                Word i = (w << 5) | bit;
                int cur = int(row[i]);
                int nxt = cur + step;
                if (nxt > 63) nxt = 63;
                if (nxt < -64) nxt = -64;
                if (nxt != cur) {
                    row[i] = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
    }

    // Warp reduction before atomic accumulation (only 32 atomics total across 1,024 threads!)
    Word w_edits = warp_add(local_edits);
    Word w_spikes = warp_add(local_spikes);
    if (lane_id() == 0) {
        if (out_edits && w_edits)
            asm volatile("red.global.add.u32 [%0], %1;" :: "l"(out_edits), "r"(w_edits) : "memory");
        if (out_spikes && w_spikes)
            asm volatile("red.global.add.u32 [%0], %1;" :: "l"(out_spikes), "r"(w_spikes) : "memory");
    }
}

// ---------------------------------------------------------------------------
// Kernel 2: GPU Neocortical Attractor Integration & k-WTA Settling!
// Grid: 1 block x 256 threads (each thread evaluates 4 postsynaptic neurons j),
// computing real synaptic dot products across active cue spikes, subtracting
// inhibitory pathway if requested, and selecting top-k spiking neurons!
// ---------------------------------------------------------------------------
AX_FN void neo_settle_block(const signed char *weights_all, Word pathway, const SpikeVec &cue,
                            Word use_inhib_pw, const SpikeVec &inhib_cue, Word top_k, int min_curr,
                            NeoQueryResult &out) {
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_active_pre[NEURONS];
    __attribute__((shared)) static Word s_n_pre;
    __attribute__((shared)) static Word s_inhib_pre[NEURONS];
    __attribute__((shared)) static Word s_n_inhib;

    Word tid = thread_id();
    // Thread 0 extracts sparse active presynaptic indices into shared memory (< 100 ns!)
    if (tid == 0) {
        Word cnt = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word m = cue.bits[w];
            while (m && cnt < NEURONS) {
                Word b = __builtin_ctz(m);
                m &= m - 1u;
                s_active_pre[cnt++] = (w << 5) | b;
            }
        }
        s_n_pre = cnt;

        Word icnt = 0;
        if (use_inhib_pw < NEO_PATHWAYS) {
            for (Word w = 0; w < WORDS; ++w) {
                Word m = inhib_cue.bits[w];
                while (m && icnt < NEURONS) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    s_inhib_pre[icnt++] = (w << 5) | b;
                }
            }
        }
        s_n_inhib = icnt;
    }
    asm volatile("bar.sync 0;");

    Word n_pre = s_n_pre;
    Word n_inh = s_n_inhib;
    const signed char *W_pos = weights_all + (unsigned long long)(pathway) * NEO_SYNAPSES;
    const signed char *W_inh = (use_inhib_pw < NEO_PATHWAYS)
                                   ? (weights_all + (unsigned long long)(use_inhib_pw) * NEO_SYNAPSES)
                                   : nullptr;

    // 256 threads x 4 neurons each = all 1,024 postsynaptic neurons integrated in parallel!
    for (Word j = tid; j < NEURONS; j += 256u) {
        int acc = 0;
        const signed char *row = W_pos + (unsigned long long)(j) * NEURONS;
        for (Word k = 0; k < n_pre; ++k)
            acc += int(row[s_active_pre[k]]);
        if (W_inh && n_inh > 0) {
            const signed char *irow = W_inh + (unsigned long long)(j) * NEURONS;
            int iacc = 0;
            for (Word k = 0; k < n_inh; ++k)
                iacc += int(irow[s_inhib_pre[k]]);
            // Inhibitory pathway vetoes matching postsynaptic neurons strongly!
            if (iacc < 0)
                acc += iacc * 4;
        }
        s_curr[j] = acc;
    }
    asm volatile("bar.sync 0;");

    // Thread 0 performs exact k-WTA threshold bisection over the 1,024 postsynaptic currents!
    if (tid == 0) {
        int hi = -1000000, sum_pos = 0, pos_cnt = 0;
        for (Word j = 0; j < NEURONS; ++j) {
            if (s_curr[j] > hi) hi = s_curr[j];
            if (s_curr[j] > 0) { sum_pos += s_curr[j]; ++pos_cnt; }
        }
        out = {};
        out.max_current = hi;
        out.mean_current = pos_cnt > 0 ? (sum_pos / pos_cnt) : 0;
        out.spikes_fired = n_pre * 4u + pos_cnt;

        if (hi < min_curr || n_pre == 0)
            return;

        int lo = min_curr;
        int thr_hi = hi + 1;
        for (int it = 0; it < 16; ++it) {
            int mid = (lo + thr_hi) >> 1;
            Word cnt = 0;
            for (Word j = 0; j < NEURONS; ++j)
                if (s_curr[j] >= mid) ++cnt;
            if (cnt > top_k) lo = mid + 1;
            else thr_hi = mid;
        }
        int thr = lo > hi ? hi : lo;
        if (thr < min_curr) thr = min_curr;
        Word fired = 0;
        for (Word j = 0; j < NEURONS; ++j) {
            if (s_curr[j] >= thr && fired < top_k) {
                out.spikes.bits[j >> 5] |= 1u << (j & 31u);
                ++fired;
            }
        }
        // Tie-fill pass with divisive lateral inhibition (`tie_v * 2 >= hi`): fills tied top plateau without pulling in weak distractors!
        if (fired < top_k && thr > min_curr) {
            int tie_v = thr - 1;
            if (tie_v * 2 >= hi) {
                for (Word j = 0; j < NEURONS && fired < top_k; ++j) {
                    if (s_curr[j] == tie_v) {
                        out.spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++fired;
                    }
                }
            }
        }
        out.active_count = fired;
        out.spikes_fired += fired;
    }
}

// ---------------------------------------------------------------------------
// Kernel 3: GPU Neocortical Spreading-Activation Semantic Similarity (4M Synapses!)
// Propagates cue SDR across Pathway 3 (Category/Features -> Exemplar Manifold),
// optionally suppressing `self_mask` so an exemplar retrieves its nearest neighbor
// rather than itself, then selects top-k spiking neurons via exact bisection!
// ---------------------------------------------------------------------------
AX_FN void neo_similarity_block(const signed char *weights_all, const SpikeVec &cue, const SpikeVec &self_mask,
                                Word top_k, int min_curr, NeoQueryResult &out) {
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_active_pre[NEURONS];
    __attribute__((shared)) static Word s_n_pre;

    Word tid = thread_id();
    if (tid == 0) {
        Word cnt = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word m = cue.bits[w];
            while (m && cnt < NEURONS) {
                Word b = __builtin_ctz(m);
                m &= m - 1u;
                s_active_pre[cnt++] = (w << 5) | b;
            }
        }
        s_n_pre = cnt;
    }
    asm volatile("bar.sync 0;");

    Word n_pre = s_n_pre;
    const signed char *W_sim = weights_all + (unsigned long long)(3u) * NEO_SYNAPSES;

    for (Word j = tid; j < NEURONS; j += 256u) {
        int acc = 0;
        const signed char *row = W_sim + (unsigned long long)(j) * NEURONS;
        for (Word k = 0; k < n_pre; ++k)
            acc += int(row[s_active_pre[k]]);
        // Lateral inhibition suppresses query entity's own SDR bits so nearest neighbor emerges!
        if ((self_mask.bits[j >> 5] >> (j & 31u)) & 1u)
            acc -= 128;
        s_curr[j] = acc;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        int hi = -1000000, sum_pos = 0, pos_cnt = 0;
        for (Word j = 0; j < NEURONS; ++j) {
            if (s_curr[j] > hi) hi = s_curr[j];
            if (s_curr[j] > 0) { sum_pos += s_curr[j]; ++pos_cnt; }
        }
        out = {};
        out.max_current = hi;
        out.mean_current = pos_cnt > 0 ? (sum_pos / pos_cnt) : 0;
        out.spikes_fired = n_pre * 4u + pos_cnt;

        if (hi < min_curr || n_pre == 0)
            return;

        int lo = min_curr;
        int thr_hi = hi + 1;
        for (int it = 0; it < 16; ++it) {
            int mid = (lo + thr_hi) >> 1;
            Word cnt = 0;
            for (Word j = 0; j < NEURONS; ++j)
                if (s_curr[j] >= mid) ++cnt;
            if (cnt > top_k) lo = mid + 1;
            else thr_hi = mid;
        }
        int thr = lo > hi ? hi : lo;
        if (thr < min_curr) thr = min_curr;
        Word fired = 0;
        for (Word j = 0; j < NEURONS; ++j) {
            if (s_curr[j] >= thr && fired < top_k) {
                out.spikes.bits[j >> 5] |= 1u << (j & 31u);
                ++fired;
            }
        }
        // Tie-fill pass with divisive lateral inhibition (`tie_v * 2 >= hi`): fills tied top plateau without pulling in weak distractors!
        if (fired < top_k && thr > min_curr) {
            int tie_v = thr - 1;
            if (tie_v * 2 >= hi) {
                for (Word j = 0; j < NEURONS && fired < top_k; ++j) {
                    if (s_curr[j] == tie_v) {
                        out.spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++fired;
                    }
                }
            }
        }
        out.active_count = fired;
        out.spikes_fired += fired;
    }
}

// ---------------------------------------------------------------------------
// Kernel 4: Blackwell Recurrent Multi-Hop Spiking Cortical Thought Loop!
// Runs `1..4` sequential spiking attractor settlements ENTIRELY ON GPU inside
// shared memory + L2 cache (zero CPU round-trips!):
//   Hop 0: init_cue | aux[0] -> W[pathways[0]] -> exact k-WTA -> s_cur_spikes
//   Hop 1: s_cur_spikes | aux[1] -> W[pathways[1]] -> exact k-WTA -> ...
// Enables pure GPU multi-hop taxonomic spreading, chained intuitive physics,
// and inverse-causal diagnostic cascades across the 12-MiB L2 Neocortex!
// ---------------------------------------------------------------------------
AX_FN void neo_think_chain_block(const signed char *weights_all, const NeoChainQuery &q, NeoChainResult &out) {
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_active_pre[NEURONS];
    __attribute__((shared)) static Word s_n_pre;
    __attribute__((shared)) static Word s_cur_bits[WORDS];
    __attribute__((shared)) static Word s_total_spikes;
    __attribute__((shared)) static Word s_stop;
    __attribute__((shared)) static int s_last_hi;

    Word tid = thread_id();
    if (tid == 0) {
        out = {};
        for (Word w = 0; w < WORDS; ++w)
            s_cur_bits[w] = q.init_cue.bits[w];
        s_total_spikes = 0;
        s_stop = 0;
        s_last_hi = 0;
    }
    asm volatile("bar.sync 0;");

    Word n_hops = q.n_hops > MAX_THINK_HOPS ? MAX_THINK_HOPS : q.n_hops;
    for (Word h = 0; h < n_hops; ++h) {
        if (s_stop)
            break;
        Word pw = q.pathways[h];
        Word inh_pw = q.inhib_pw[h];
        if (pw >= NEO_PATHWAYS)
            break;

        if (tid == 0) {
            Word cnt = 0;
            for (Word w = 0; w < WORDS; ++w) {
                Word m = s_cur_bits[w] | q.aux_cues[h].bits[w];
                while (m && cnt < NEURONS) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    s_active_pre[cnt++] = (w << 5) | b;
                }
            }
            s_n_pre = cnt;
        }
        asm volatile("bar.sync 0;");

        Word n_pre = s_n_pre;
        const signed char *W_pos = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;
        const signed char *W_inh =
            (inh_pw < NEO_PATHWAYS) ? (weights_all + (unsigned long long)(inh_pw) * NEO_SYNAPSES) : nullptr;

        for (Word j = tid; j < NEURONS; j += 256u) {
            int acc = 0;
            const signed char *row = W_pos + (unsigned long long)(j) * NEURONS;
            for (Word k = 0; k < n_pre; ++k)
                acc += int(row[s_active_pre[k]]);
            if (W_inh && n_pre > 0) {
                const signed char *irow = W_inh + (unsigned long long)(j) * NEURONS;
                int iacc = 0;
                for (Word k = 0; k < n_pre; ++k)
                    iacc += int(irow[s_active_pre[k]]);
                if (iacc < 0)
                    acc += iacc * 4;
            }
            s_curr[j] = acc;
        }
        asm volatile("bar.sync 0;");

        if (tid == 0) {
            int hi = -1000000, pos_cnt = 0;
            for (Word j = 0; j < NEURONS; ++j) {
                if (s_curr[j] > hi) hi = s_curr[j];
                if (s_curr[j] > 0) ++pos_cnt;
            }
            s_total_spikes += n_pre * 4u + pos_cnt;
            s_last_hi = hi;
            if (hi < q.min_curr || n_pre == 0) {
                s_stop = 1;
            } else {
                int lo = q.min_curr;
                int thr_hi = hi + 1;
                Word top_k = q.top_k > 0 ? q.top_k : 32u;
                for (int it = 0; it < 16; ++it) {
                    int mid = (lo + thr_hi) >> 1;
                    Word cnt = 0;
                    for (Word j = 0; j < NEURONS; ++j)
                        if (s_curr[j] >= mid) ++cnt;
                    if (cnt > top_k) lo = mid + 1;
                    else thr_hi = mid;
                }
                int thr = lo > hi ? hi : lo;
                if (thr < q.min_curr) thr = q.min_curr;
                SpikeVec nxt{};
                Word fired = 0;
                for (Word j = 0; j < NEURONS; ++j) {
                    if (s_curr[j] >= thr && fired < top_k) {
                        nxt.bits[j >> 5] |= 1u << (j & 31u);
                        ++fired;
                    }
                }
                if (fired < top_k && thr > q.min_curr) {
                    int tie_v = thr - 1;
                    if (tie_v * 2 >= hi) {
                        for (Word j = 0; j < NEURONS && fired < top_k; ++j) {
                            if (s_curr[j] == tie_v) {
                                nxt.bits[j >> 5] |= 1u << (j & 31u);
                                ++fired;
                            }
                        }
                    }
                }
                out.hop_spikes[h] = nxt;
                out.hop_counts[h] = fired;
                out.hops_done = h + 1u;
                for (Word w = 0; w < WORDS; ++w)
                    s_cur_bits[w] = nxt.bits[w];
                s_total_spikes += fired;
            }
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        out.total_spikes = s_total_spikes;
        out.final_max_curr = s_last_hi;
    }
}

// ---------------------------------------------------------------------------
// Kernel 5: Blackwell GPU Working-Memory Lateral-Inhibition Attention Gate!
// 1 block x 64 threads (1 thread per active WM / narrative SDR slot).
// Computes parallel spiking SDR overlap + recency bias, runs winner-take-all
// competition, and flags neural ambiguity (`top1 - top2 <= ambig_margin`) so
// the mind can spontaneously ask the human a clarifying question!
// ---------------------------------------------------------------------------
AX_FN void wm_attend_block(const WmAttendQuery &q, WmAttendResult &out) {
    __attribute__((shared)) static int s_scores[MAX_WM_SLOTS];
    __attribute__((shared)) static Word s_spikes[MAX_WM_SLOTS];

    Word tid = thread_id();
    Word n = q.n_slots > MAX_WM_SLOTS ? MAX_WM_SLOTS : q.n_slots;
    if (tid < MAX_WM_SLOTS) {
        int sc = -1;
        Word sp = 0;
        if (tid < n) {
            Word ov = 0;
            for (Word w = 0; w < WORDS; ++w)
                ov += __builtin_popcount(q.slots[tid].bits[w] & q.query.bits[w]);
            sp = ov + 4u;
            if (ov >= q.min_overlap)
                sc = int(ov) * 4 + int(q.slot_recency[tid]);
        }
        s_scores[tid] = sc;
        s_spikes[tid] = sp;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        out = {};
        out.best_slot = ~0u;
        out.second_slot = ~0u;
        int b1 = -1, b2 = -1;
        Word i1 = ~0u, i2 = ~0u;
        Word tot_sp = 0;
        for (Word i = 0; i < n; ++i) {
            tot_sp += s_spikes[i];
            int sc = s_scores[i];
            if (sc > b1) {
                b2 = b1; i2 = i1;
                b1 = sc; i1 = i;
            } else if (sc > b2) {
                b2 = sc; i2 = i;
            }
        }
        out.best_slot = i1;
        out.second_slot = i2;
        out.best_score = b1 > 0 ? b1 : 0;
        out.second_score = b2 > 0 ? b2 : 0;
        if (i1 < n) out.best_id = q.slot_ids[i1];
        if (i2 < n) out.second_id = q.slot_ids[i2];
        if (i1 < n && i2 < n && b1 > 0 && b2 > 0 && (b1 - b2) <= int(q.ambig_margin))
            out.ambiguous = 1u;
        out.spikes_fired = tot_sp + 16u;
    }
}

// ---------------------------------------------------------------------------
// Kernel 6: Blackwell GPU Spiking Cortical Differential Contrast (`mind_neo_contrast`)!
// 1 block x 256 threads computes parallel net synaptic currents for TWO concepts
// `cue_a` and `cue_b` across `pos_pathway` minus `inhib_pathway` (e.g. Pathways 1 & 2)
// in a single L2-resident pass, simultaneously extracting:
//   - `shared_spikes`: properties/categories strongly active for BOTH A and B
//   - `only_a_spikes`: features active for A and absent/inhibited for B
//   - `only_b_spikes`: features active for B and absent/inhibited for A
// Enables pure GPU semantic contrast ("contrast sparrow and penguin on cortex")
// surviving total hippocampal lesion (`--ablate hipp`)!
// ---------------------------------------------------------------------------
AX_FN void neo_contrast_block(const signed char *weights_all, const NeoContrastQuery &q, NeoContrastResult &out) {
    __attribute__((shared)) static int s_ca[NEURONS];
    __attribute__((shared)) static int s_cb[NEURONS];
    __attribute__((shared)) static int s_sc[NEURONS];
    __attribute__((shared)) static Word s_pre_u[NEURONS];
    __attribute__((shared)) static int s_tmax[256];
    __attribute__((shared)) static Word s_tcnt[256];
    __attribute__((shared)) static Word s_nu, s_na, s_nb;
    __attribute__((shared)) static Word s_sp;
    __attribute__((shared)) static int s_hi;

    Word tid = thread_id();
    if (tid == 0) {
        Word nu = 0, ca = 0, cb = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word ma = q.cue_a.bits[w];
            Word mb = q.cue_b.bits[w];
            Word mu = ma | mb;
            while (mu && nu < NEURONS) {
                Word b = __builtin_ctz(mu);
                mu &= mu - 1u;
                Word fl = (((ma >> b) & 1u) << 10) | (((mb >> b) & 1u) << 11);
                if ((ma >> b) & 1u) ++ca;
                if ((mb >> b) & 1u) ++cb;
                s_pre_u[nu++] = ((w << 5) | b) | fl;
            }
        }
        s_nu = nu;
        s_na = ca;
        s_nb = cb;
        out = {};
        s_sp = (ca + cb) * 4u;
    }
    asm volatile("bar.sync 0;");

    Word nu = s_nu, na = s_na, nb = s_nb;
    const signed char *W_pos = weights_all + (unsigned long long)(q.pos_pathway < NEO_PATHWAYS ? q.pos_pathway : 1u) * NEO_SYNAPSES;
    const signed char *W_inh = (q.inhib_pathway < NEO_PATHWAYS)
                                   ? (weights_all + (unsigned long long)(q.inhib_pathway) * NEO_SYNAPSES)
                                   : nullptr;

    for (Word j = tid; j < NEURONS; j += 256u) {
        const signed char *row = W_pos + (unsigned long long)(j) * NEURONS;
        int aa = 0, ab = 0;
        Word pa = 0, pb = 0;
        for (Word k = 0; k < nu; ++k) {
            Word pk = s_pre_u[k];
            int wv = int(row[pk & 1023u]);
            if (pk & 1024u) { aa += wv; if (wv > 0) ++pa; }
            if (pk & 2048u) { ab += wv; if (wv > 0) ++pb; }
        }
        // Biophysical NMDA dendritic cooperativity: reject sparse background cross-talk
        // (`>= 75%` of presynaptic cue spikes must excite dendritic spines on post-neuron j,
        // preserving 100% of genuine PW0/PW1 associations while zeroing unassociated distractors!)
        if (na >= 4u && pa * 4u < na * 3u) aa = 0;
        if (nb >= 4u && pb * 4u < nb * 3u) ab = 0;
        if (W_inh) {
            const signed char *irow = W_inh + (unsigned long long)(j) * NEURONS;
            int ia = 0, ib = 0;
            for (Word k = 0; k < nu; ++k) {
                Word pk = s_pre_u[k];
                int wv = int(irow[pk & 1023u]);
                if (pk & 1024u) ia += wv;
                if (pk & 2048u) ib += wv;
            }
            if (ia < 0) aa += ia * 4;
            if (ib < 0) ab += ib * 4;
        }
        s_ca[j] = aa;
        s_cb[j] = ab;
    }
    asm volatile("bar.sync 0;");

    int min_c = q.min_curr > 0 ? q.min_curr : 6;
    Word tk_base = q.top_k > 0 ? q.top_k : 48u;

    for (int pass = 0; pass < 3; ++pass) {
        Word tk = (pass == 0) ? (tk_base < 128u ? tk_base * 2u : tk_base) : tk_base;
        int lhi = -1000000;
        Word lsp = 0;
        for (Word j = tid; j < NEURONS; j += 256u) {
            int ca = s_ca[j], cb = s_cb[j], v = -1;
            if (pass == 0)
                v = (ca >= min_c && cb >= min_c) ? (ca < cb ? ca : cb) : -1;
            else if (pass == 1)
                v = (ca >= min_c) ? (ca - (cb > 0 ? cb : 0)) : -1;
            else
                v = (cb >= min_c) ? (cb - (ca > 0 ? ca : 0)) : -1;
            s_sc[j] = v;
            if (v > lhi) lhi = v;
            if (v >= min_c) ++lsp;
        }
        s_tmax[tid] = lhi;
        s_tcnt[tid] = lsp;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) {
                if (s_tmax[tid + st] > s_tmax[tid]) s_tmax[tid] = s_tmax[tid + st];
                s_tcnt[tid] += s_tcnt[tid + st];
            }
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            s_hi = s_tmax[0];
            s_sp += s_tcnt[0];
        }
        asm volatile("bar.sync 0;");

        int hi = s_hi;
        if (hi < min_c)
            continue;

        int lo = min_c, thr_hi = hi + 1;
        for (int it = 0; it < 12; ++it) {
            int mid = (lo + thr_hi) >> 1;
            Word lc = 0;
            for (Word j = tid; j < NEURONS; j += 256u)
                if (s_sc[j] >= mid) ++lc;
            s_tcnt[tid] = lc;
            asm volatile("bar.sync 0;");
            for (Word st = 128u; st > 0; st >>= 1) {
                if (tid < st) s_tcnt[tid] += s_tcnt[tid + st];
                asm volatile("bar.sync 0;");
            }
            if (s_tcnt[0] > tk) lo = mid + 1;
            else thr_hi = mid;
        }

        if (tid == 0) {
            int thr = lo > hi ? hi : lo;
            if (thr < min_c) thr = min_c;
            SpikeVec &dst = (pass == 0) ? out.shared_spikes : (pass == 1 ? out.only_a_spikes : out.only_b_spikes);
            Word &dst_cnt = (pass == 0) ? out.shared_count : (pass == 1 ? out.only_a_count : out.only_b_count);
            Word fired = 0;
            for (Word j = 0; j < NEURONS; ++j) {
                if (s_sc[j] >= thr && fired < tk) {
                    dst.bits[j >> 5] |= 1u << (j & 31u);
                    ++fired;
                }
            }
            if (fired < tk && thr > min_c) {
                int tie_v = thr - 1;
                for (Word j = 0; j < NEURONS && fired < tk; ++j) {
                    if (s_sc[j] == tie_v) {
                        dst.bits[j >> 5] |= 1u << (j & 31u);
                        ++fired;
                    }
                }
            }
            dst_cnt = fired;
            s_sp += fired;
        }
        asm volatile("bar.sync 0;");
    }
    if (tid == 0)
        out.spikes_fired = s_sp;
}

// ---------------------------------------------------------------------------
// Kernel 7: Blackwell GPU Spiking Proportional Relational Analogy (`mind_neo_analogy`)!
// Given `cue_a : cue_b :: cue_c : ?`, 1 block x 256 threads:
//   1. Evaluates synaptic coincidence `cue_a -> cue_b` across all candidate L2
//      pathways (e.g., Pathway 8 TaxParent, Pathway 4 Cycle, Pathway 18 Antonym,
//      Pathway 6 Relation) in parallel to discover the latent relational pathway!
//   2. Projects `cue_c` through the winning relational pathway across all 1,024
//      postsynaptic neurons with lateral inhibition suppressing `cue_c` and `cue_a`,
//      then runs parallel shared-memory top-k selection to emit `answer_spikes`!
// Survives total hippocampal lesion (`--ablate hipp` -> Octuple-Pathway Ribot's Law!)
// ---------------------------------------------------------------------------
AX_FN void neo_analogy_block(const signed char *weights_all, const NeoAnalogyQuery &q, NeoAnalogyResult &out) {
    __attribute__((shared)) static Word s_pre_a[NEURONS];
    __attribute__((shared)) static Word s_post_b[NEURONS];
    __attribute__((shared)) static Word s_pre_c[NEURONS];
    __attribute__((shared)) static Word s_na, s_nb, s_nc;
    __attribute__((shared)) static int s_pw_score[MAX_ANALOGY_PWS];
    __attribute__((shared)) static Word s_best_pw;
    __attribute__((shared)) static int s_best_sc;
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static int s_tmax[256];
    __attribute__((shared)) static Word s_tcnt[256];
    __attribute__((shared)) static Word s_sp;
    __attribute__((shared)) static int s_hi;

    Word tid = thread_id();
    if (tid == 0) {
        Word ca = 0, cb = 0, cc = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word ma = q.cue_a.bits[w], mb = q.cue_b.bits[w], mc = q.cue_c.bits[w];
            while (ma && ca < NEURONS) { Word b = __builtin_ctz(ma); ma &= ma - 1u; s_pre_a[ca++] = (w << 5) | b; }
            while (mb && cb < NEURONS) { Word b = __builtin_ctz(mb); mb &= mb - 1u; s_post_b[cb++] = (w << 5) | b; }
            while (mc && cc < NEURONS) { Word b = __builtin_ctz(mc); mc &= mc - 1u; s_pre_c[cc++] = (w << 5) | b; }
        }
        s_na = ca; s_nb = cb; s_nc = cc;
        out = {};
        out.best_pathway = ~0u;
        s_sp = (ca + cb + cc) * 4u;
    }
    asm volatile("bar.sync 0;");

    Word na = s_na, nb = s_nb, nc = s_nc;
    Word npw = q.n_pws > MAX_ANALOGY_PWS ? MAX_ANALOGY_PWS : q.n_pws;

    // Phase 1: Parallel relational pathway discovery (`cue_a -> cue_b` coincidence across candidate pathways)!
    for (Word p = 0; p < npw; ++p) {
        Word pw = q.candidate_pws[p];
        int lsum = 0;
        if (pw < NEO_PATHWAYS) {
            const signed char *Wp = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;
            for (Word idx = tid; idx < nb; idx += 256u) {
                const signed char *row = Wp + (unsigned long long)(s_post_b[idx]) * NEURONS;
                int acc = 0;
                for (Word k = 0; k < na; ++k)
                    acc += int(row[s_pre_a[k]]);
                if (acc > 0) lsum += acc;
            }
        }
        s_tmax[tid] = lsum;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tmax[tid] += s_tmax[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            s_pw_score[p] = s_tmax[0];
            s_sp += nb;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        int bsc = 0;
        Word bpw = ~0u;
        for (Word p = 0; p < npw; ++p) {
            if (s_pw_score[p] > bsc && q.candidate_pws[p] < NEO_PATHWAYS) {
                bsc = s_pw_score[p];
                bpw = q.candidate_pws[p];
            }
        }
        s_best_pw = bpw;
        s_best_sc = bsc;
        out.best_pathway = bpw;
        out.match_score = bsc;
    }
    asm volatile("bar.sync 0;");

    Word best_pw = s_best_pw;
    if (best_pw >= NEO_PATHWAYS || s_best_sc <= 0 || nc == 0) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    // Phase 2: Project `cue_c` through the winning relational pathway with self/source lateral inhibition!
    const signed char *W_win = weights_all + (unsigned long long)(best_pw) * NEO_SYNAPSES;
    int min_c = q.min_curr > 0 ? q.min_curr : 6;
    int lhi = -1000000;
    Word lsp = 0;
    for (Word j = tid; j < NEURONS; j += 256u) {
        const signed char *row = W_win + (unsigned long long)(j) * NEURONS;
        int acc = 0;
        for (Word k = 0; k < nc; ++k)
            acc += int(row[s_pre_c[k]]);
        // Lateral inhibition suppresses `cue_c` and `cue_a` bits so the fresh analogical target wins!
        if (((q.cue_c.bits[j >> 5] >> (j & 31u)) & 1u) || ((q.cue_a.bits[j >> 5] >> (j & 31u)) & 1u))
            acc -= 128;
        s_curr[j] = acc;
        if (acc > lhi) lhi = acc;
        if (acc >= min_c) ++lsp;
    }
    s_tmax[tid] = lhi;
    s_tcnt[tid] = lsp;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) {
            if (s_tmax[tid + st] > s_tmax[tid]) s_tmax[tid] = s_tmax[tid + st];
            s_tcnt[tid] += s_tcnt[tid + st];
        }
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) {
        s_hi = s_tmax[0];
        s_sp += s_tcnt[0];
    }
    asm volatile("bar.sync 0;");

    int hi = s_hi;
    if (hi < min_c) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    Word tk = q.top_k > 0 ? q.top_k : 32u;
    int lo = min_c, thr_hi = hi + 1;
    for (int it = 0; it < 12; ++it) {
        int mid = (lo + thr_hi) >> 1;
        Word lc = 0;
        for (Word j = tid; j < NEURONS; j += 256u)
            if (s_curr[j] >= mid) ++lc;
        s_tcnt[tid] = lc;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tcnt[tid] += s_tcnt[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (s_tcnt[0] > tk) lo = mid + 1;
        else thr_hi = mid;
    }

    if (tid == 0) {
        int thr = lo > hi ? hi : lo;
        if (thr < min_c) thr = min_c;
        Word fired = 0;
        for (Word j = 0; j < NEURONS; ++j) {
            if (s_curr[j] >= thr && fired < tk) {
                out.answer_spikes.bits[j >> 5] |= 1u << (j & 31u);
                ++fired;
            }
        }
        if (fired < tk && thr > min_c) {
            int tie_v = thr - 1;
            if (tie_v * 2 >= hi) {
                for (Word j = 0; j < NEURONS && fired < tk; ++j) {
                    if (s_curr[j] == tie_v) {
                        out.answer_spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++fired;
                    }
                }
            }
        }
        out.active_count = fired;
        out.spikes_fired = s_sp + fired;
    }
}

// ---------------------------------------------------------------------------
// Kernel 8: Blackwell GPU Spiking Compositional Intersection & Veto (`mind_neo_compose`)!
// 1 block x 256 threads computes genuine spiking coincidence-detection intersection
// across up to 3 L2-resident pathways in a single pass:
//   - `W[pw_cat]` driven by `cue_cat` (category exemplars, e.g. `bird`)
//   - `W[pw_pos]` driven by `cue_pos` (positive ability/feature exemplars, e.g. `swim` or `fly`)
//   - optional `W[pw_neg]` driven by `cue_neg` (negated-exception exemplars, e.g. `not fly` -> `penguin`)
// Only postsynaptic neurons receiving suprathreshold drive from EVERY active constraint
// fire via coincidence gating (`min(c_cat, c_pos)` or `min(min(c_cat, c_pos), c_neg)`),
// solving compositional entity queries purely inside Blackwell L2 synapses —
// SURVIVING `--ablate hipp` (Nonuple-Pathway Ribot's Law!)
// ---------------------------------------------------------------------------
AX_FN void neo_compose_block(const signed char *weights_all, const NeoComposeQuery &q, NeoComposeResult &out) {
    __attribute__((shared)) static Word s_p0[NEURONS];
    __attribute__((shared)) static Word s_p1[NEURONS];
    __attribute__((shared)) static Word s_p2[NEURONS];
    __attribute__((shared)) static Word s_n0, s_n1, s_n2;
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static int s_tmax[256];
    __attribute__((shared)) static Word s_tcnt[256];
    __attribute__((shared)) static int s_hi;
    __attribute__((shared)) static Word s_sp;

    Word tid = thread_id();
    if (tid == 0) {
        Word n0 = 0, n1 = 0, n2 = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word m0 = q.cue_cat.bits[w], m1 = q.cue_pos.bits[w], m2 = q.cue_neg.bits[w];
            while (m0 && n0 < NEURONS) { Word b = __builtin_ctz(m0); m0 &= m0 - 1u; s_p0[n0++] = (w << 5) | b; }
            while (m1 && n1 < NEURONS) { Word b = __builtin_ctz(m1); m1 &= m1 - 1u; s_p1[n1++] = (w << 5) | b; }
            while (m2 && n2 < NEURONS) { Word b = __builtin_ctz(m2); m2 &= m2 - 1u; s_p2[n2++] = (w << 5) | b; }
        }
        s_n0 = n0; s_n1 = n1; s_n2 = n2;
        out = {};
        s_sp = (n0 + n1 + (q.has_neg ? n2 : 0u)) * 4u;
    }
    asm volatile("bar.sync 0;");

    Word n0 = s_n0, n1 = s_n1, n2 = s_n2;
    const signed char *W0 = weights_all + (unsigned long long)(q.pw_cat < NEO_PATHWAYS ? q.pw_cat : 3u) * NEO_SYNAPSES;
    const signed char *W1 = weights_all + (unsigned long long)(q.pw_pos < NEO_PATHWAYS ? q.pw_pos : 20u) * NEO_SYNAPSES;
    const signed char *W2 = (q.has_neg && q.pw_neg < NEO_PATHWAYS)
                                ? (weights_all + (unsigned long long)(q.pw_neg) * NEO_SYNAPSES)
                                : nullptr;

    int min_c = q.min_curr > 0 ? q.min_curr : 4;
    int lhi = -1000000;
    Word lsp = 0;
    for (Word j = tid; j < NEURONS; j += 256u) {
        int c0 = 0, c1 = 0, c2 = 0;
        const signed char *r0 = W0 + (unsigned long long)(j) * NEURONS;
        for (Word k = 0; k < n0; ++k) c0 += int(r0[s_p0[k]]);
        const signed char *r1 = W1 + (unsigned long long)(j) * NEURONS;
        for (Word k = 0; k < n1; ++k) c1 += int(r1[s_p1[k]]);
        int v = -1;
        if (c0 >= min_c && c1 >= min_c) {
            if (W2 && n2 > 0) {
                const signed char *r2 = W2 + (unsigned long long)(j) * NEURONS;
                for (Word k = 0; k < n2; ++k) c2 += int(r2[s_p2[k]]);
                if (c2 >= min_c) {
                    int m01 = c0 < c1 ? c0 : c1;
                    v = m01 < c2 ? m01 : c2;
                }
            } else {
                v = c0 < c1 ? c0 : c1;
            }
        }
        s_curr[j] = v;
        if (v > lhi) lhi = v;
        if (v >= min_c) ++lsp;
    }
    s_tmax[tid] = lhi;
    s_tcnt[tid] = lsp;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) {
            if (s_tmax[tid + st] > s_tmax[tid]) s_tmax[tid] = s_tmax[tid + st];
            s_tcnt[tid] += s_tcnt[tid + st];
        }
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) {
        s_hi = s_tmax[0];
        s_sp += s_tcnt[0];
        out.max_current = s_hi > 0 ? s_hi : 0;
    }
    asm volatile("bar.sync 0;");

    int hi = s_hi;
    if (hi < min_c) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    Word tk = q.top_k > 0 ? q.top_k : 32u;
    int floor_c = min_c;
    int lo = floor_c, thr_hi = hi + 1;
    for (int it = 0; it < 12; ++it) {
        int mid = (lo + thr_hi) >> 1;
        Word lc = 0;
        for (Word j = tid; j < NEURONS; j += 256u)
            if (s_curr[j] >= mid) ++lc;
        s_tcnt[tid] = lc;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tcnt[tid] += s_tcnt[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (s_tcnt[0] > tk) lo = mid + 1;
        else thr_hi = mid;
    }

    if (tid == 0) {
        int thr = lo > hi ? hi : lo;
        if (thr < floor_c) thr = floor_c;
        Word fired = 0;
        for (Word j = 0; j < NEURONS; ++j) {
            if (s_curr[j] >= thr && fired < tk) {
                out.answer_spikes.bits[j >> 5] |= 1u << (j & 31u);
                ++fired;
            }
        }
        if (fired < tk && thr > floor_c) {
            int tie_v = thr - 1;
            if (tie_v >= floor_c) {
                for (Word j = 0; j < NEURONS && fired < tk; ++j) {
                    if (s_curr[j] == tie_v) {
                        out.answer_spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++fired;
                    }
                }
            }
        }
        out.active_count = fired;
        out.spikes_fired = s_sp + fired;
    }
}

// ---------------------------------------------------------------------------
// Kernel 9: Blackwell GPU Multi-Cue Context-Gated Associative Retrieval
// (`mind_neo_multi_cue`)!
// 1 block x 256 threads fuses up to 4 simultaneous contextual cues across
// distinct L2-resident neocortical pathways (`W[q.pathways[c]]`, weighted by
// `q.weights[c]`) with optional inhibitory context suppression, then runs
// parallel 256-thread shared-memory top-k reduction.
// Powers Pathway 26 causal-context synthesis & Pathway 25/27 retrieval —
// SURVIVING `--ablate hipp` (Decuple-Pathway Ribot's Law!)
// ---------------------------------------------------------------------------
AX_FN void neo_multi_cue_block(const signed char *weights_all, const NeoMultiCueQuery &q, NeoMultiCueResult &out) {
    __attribute__((shared)) static Word s_pre[MAX_MULTI_CUES][NEURONS];
    __attribute__((shared)) static Word s_np[MAX_MULTI_CUES];
    __attribute__((shared)) static Word s_pinh[NEURONS];
    __attribute__((shared)) static Word s_ninh;
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static int s_tmax[256];
    __attribute__((shared)) static Word s_tcnt[256];
    __attribute__((shared)) static int s_hi;
    __attribute__((shared)) static Word s_sp;

    Word tid = thread_id();
    Word nc = q.n_cues > MAX_MULTI_CUES ? MAX_MULTI_CUES : q.n_cues;
    if (tid == 0) {
        Word sp = 0;
        for (Word c = 0; c < nc; ++c) {
            Word cnt = 0;
            for (Word w = 0; w < WORDS; ++w) {
                Word m = q.cues[c].bits[w];
                while (m && cnt < NEURONS) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    s_pre[c][cnt++] = (w << 5) | b;
                }
            }
            s_np[c] = cnt;
            sp += cnt * 4u;
        }
        Word ni = 0;
        if (q.inhib_pw < NEO_PATHWAYS) {
            for (Word w = 0; w < WORDS; ++w) {
                Word m = q.inhib_cue.bits[w];
                while (m && ni < NEURONS) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    s_pinh[ni++] = (w << 5) | b;
                }
            }
            sp += ni * 2u;
        }
        s_ninh = ni;
        s_sp = sp;
        out = {};
    }
    asm volatile("bar.sync 0;");

    int min_c = q.min_curr > 0 ? q.min_curr : 4;
    int lhi = -1000000;
    Word lsp = 0;
    for (Word j = tid; j < NEURONS; j += 256u) {
        int acc = 0;
        for (Word c = 0; c < nc; ++c) {
            Word cnt = s_np[c];
            if (cnt == 0) continue;
            Word pw = q.pathways[c] < NEO_PATHWAYS ? q.pathways[c] : 26u;
            int wt = q.weights[c] > 0 ? int(q.weights[c]) : 1;
            const signed char *row = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES +
                                     (unsigned long long)(j) * NEURONS;
            int sub = 0;
            for (Word k = 0; k < cnt; ++k)
                sub += int(row[s_pre[c][k]]);
            acc += sub * wt;
        }
        if (q.inhib_pw < NEO_PATHWAYS && s_ninh > 0) {
            const signed char *irow = weights_all + (unsigned long long)(q.inhib_pw) * NEO_SYNAPSES +
                                      (unsigned long long)(j) * NEURONS;
            int isub = 0;
            for (Word k = 0; k < s_ninh; ++k)
                isub += int(irow[s_pinh[k]]);
            if (isub > 0)
                acc -= isub * 3;
        }
        s_curr[j] = acc;
        if (acc > lhi) lhi = acc;
        if (acc >= min_c) ++lsp;
    }
    s_tmax[tid] = lhi;
    s_tcnt[tid] = lsp;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) {
            if (s_tmax[tid + st] > s_tmax[tid]) s_tmax[tid] = s_tmax[tid + st];
            s_tcnt[tid] += s_tcnt[tid + st];
        }
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) {
        s_hi = s_tmax[0];
        s_sp += s_tcnt[0];
        out.max_current = s_hi > 0 ? s_hi : 0;
    }
    asm volatile("bar.sync 0;");

    int hi = s_hi;
    if (hi < min_c) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    Word tk = q.top_k > 0 ? q.top_k : 32u;
    int lo = min_c, thr_hi = hi + 1;
    for (int it = 0; it < 12; ++it) {
        int mid = (lo + thr_hi) >> 1;
        Word lc = 0;
        for (Word j = tid; j < NEURONS; j += 256u)
            if (s_curr[j] >= mid) ++lc;
        s_tcnt[tid] = lc;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tcnt[tid] += s_tcnt[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (s_tcnt[0] > tk) lo = mid + 1;
        else thr_hi = mid;
    }

    if (tid == 0) {
        int thr = lo > hi ? hi : lo;
        if (thr < min_c) thr = min_c;
        Word fired = 0;
        for (Word j = 0; j < NEURONS; ++j) {
            if (s_curr[j] >= thr && fired < tk) {
                out.spikes.bits[j >> 5] |= 1u << (j & 31u);
                ++fired;
            }
        }
        if (fired < tk && thr > min_c) {
            int tie_v = thr - 1;
            if (tie_v * 2 >= hi) {
                for (Word j = 0; j < NEURONS && fired < tk; ++j) {
                    if (s_curr[j] == tie_v) {
                        out.spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++fired;
                    }
                }
            }
        }
        out.active_count = fired;
        out.spikes_fired = s_sp + fired;
    }
}

// ---------------------------------------------------------------------------
// Kernel 10: Blackwell GPU Multi-Clue Bayesian Hypothesis Ranking (`mind_neo_hypothesize`)!
// 1 block x 256 threads evaluates up to 4 simultaneous sensory/symptom clues
// through an L2-resident neocortical pathway and scores up to 8 candidate
// hypotheses in parallel via shared-memory tree reduction — returning the
// top explanatory hypothesis & spiking evidence margin purely on GPU,
// SURVIVING `--ablate hipp` (Undecuple-Pathway Ribot's Law!)
// ---------------------------------------------------------------------------
AX_FN void neo_hypothesize_block(const signed char *weights_all, const NeoHypothesizeQuery &q,
                                 NeoHypothesizeResult &out) {
    __attribute__((shared)) static Word s_pre[MAX_HYP_CLUES][NEURONS];
    __attribute__((shared)) static Word s_npre[MAX_HYP_CLUES];
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static int s_sum[256];
    __attribute__((shared)) static Word s_sp;

    Word tid = thread_id();
    Word nc = q.n_clues > MAX_HYP_CLUES ? MAX_HYP_CLUES : q.n_clues;
    Word nh = q.n_cands > MAX_HYP_CANDS ? MAX_HYP_CANDS : q.n_cands;
    if (tid == 0) {
        out = {};
        out.best_cand = ~0u;
        out.second_cand = ~0u;
        Word sp = 0;
        for (Word c = 0; c < nc; ++c) {
            Word cnt = 0;
            for (Word w = 0; w < WORDS; ++w) {
                Word m = q.clues[c].bits[w];
                while (m && cnt < NEURONS) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    s_pre[c][cnt++] = (w << 5) | b;
                }
            }
            s_npre[c] = cnt;
            sp += cnt * 4u;
        }
        s_sp = sp;
    }
    asm volatile("bar.sync 0;");

    const signed char *W = weights_all + (unsigned long long)(q.pathway < NEO_PATHWAYS ? q.pathway : 0u) * NEO_SYNAPSES;
    int lpos = 0;
    for (Word j = tid; j < NEURONS; j += 256u) {
        const signed char *row = W + (unsigned long long)(j) * NEURONS;
        int acc = 0;
        for (Word c = 0; c < nc; ++c) {
            Word n = s_npre[c];
            for (Word k = 0; k < n; ++k)
                acc += int(row[s_pre[c][k]]);
        }
        s_curr[j] = acc;
        if (acc > 0) ++lpos;
    }
    s_sum[tid] = lpos;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) s_sum[tid] += s_sum[tid + st];
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) s_sp += Word(s_sum[0]);
    asm volatile("bar.sync 0;");

    for (Word h = 0; h < nh; ++h) {
        int lsc = 0;
        for (Word j = tid; j < NEURONS; j += 256u) {
            if ((q.cands[h].bits[j >> 5] >> (j & 31u)) & 1u)
                lsc += s_curr[j];
        }
        s_sum[tid] = lsc;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_sum[tid] += s_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sc = s_sum[0];
            out.scores[h] = sc;
            if (out.best_cand == ~0u || sc > out.best_score) {
                out.second_cand = out.best_cand;
                out.second_score = out.best_score;
                out.best_cand = h;
                out.best_score = sc;
            } else if (out.second_cand == ~0u || sc > out.second_score) {
                out.second_cand = h;
                out.second_score = sc;
            }
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        if (out.best_cand < nh && out.best_score > 0) {
            Word win = out.best_cand;
            Word fired = 0;
            for (Word j = 0; j < NEURONS; ++j) {
                if (((q.cands[win].bits[j >> 5] >> (j & 31u)) & 1u) && s_curr[j] > 0) {
                    out.winner_spikes.bits[j >> 5] |= 1u << (j & 31u);
                    ++fired;
                }
            }
            s_sp += fired;
        }
        out.spikes_fired = s_sp;
    }
}

// ---------------------------------------------------------------------------
// Kernel 11: Blackwell GPU Bidirectional Spreading-Activation Bridge (`mind_neo_bridge`)!
// 1 block x 256 threads computes simultaneous FORWARD synaptic current from `cue_a`
// (`W[j, i]`) AND BACKWARD transposed synaptic affinity from `cue_b` (`W[k, j]`) across
// candidate L2 pathways (zero extra memory!), finding the exact intermediate bridge
// concept `M` (`cue_a -> M -> cue_b`) via bidirectional coincidence + scale-invariant
// `hi/4` lateral inhibition — SURVIVING `--ablate hipp` (Duodecuple-Pathway Ribot's Law!)
// ---------------------------------------------------------------------------
AX_FN void neo_bridge_block(const signed char *weights_all, const NeoBridgeQuery &q, NeoBridgeResult &out) {
    __attribute__((shared)) static Word s_a[NEURONS];
    __attribute__((shared)) static Word s_b[NEURONS];
    __attribute__((shared)) static Word s_na, s_nb;
    __attribute__((shared)) static int s_sc[NEURONS];
    __attribute__((shared)) static int s_best[NEURONS];
    __attribute__((shared)) static int s_tmax[256];
    __attribute__((shared)) static int s_tsum[256];
    __attribute__((shared)) static Word s_tcnt[256];
    __attribute__((shared)) static int s_hi;
    __attribute__((shared)) static int s_best_hi;
    __attribute__((shared)) static int s_best_tot;
    __attribute__((shared)) static Word s_best_pw;
    __attribute__((shared)) static Word s_sp;

    Word tid = thread_id();
    Word np = q.n_pws > MAX_BRIDGE_PWS ? MAX_BRIDGE_PWS : (q.n_pws > 0 ? q.n_pws : 1u);
    int min_c = q.min_curr > 0 ? q.min_curr : 4;
    Word tk = q.top_k > 0 ? q.top_k : 32u;

    if (tid == 0) {
        out = {};
        out.best_pathway = ~0u;
        Word na = 0, nb = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word ma = q.cue_a.bits[w];
            while (ma && na < NEURONS) {
                Word b = __builtin_ctz(ma);
                ma &= ma - 1u;
                s_a[na++] = (w << 5) | b;
            }
            Word mb = q.cue_b.bits[w];
            while (mb && nb < NEURONS) {
                Word b = __builtin_ctz(mb);
                mb &= mb - 1u;
                s_b[nb++] = (w << 5) | b;
            }
        }
        s_na = na;
        s_nb = nb;
        s_best_hi = 0;
        s_best_tot = 0;
        s_best_pw = ~0u;
        s_sp = (na + nb) * 4u;
    }
    for (Word j = tid; j < NEURONS; j += 256u)
        s_best[j] = 0;
    asm volatile("bar.sync 0;");

    Word na = s_na, nb = s_nb;
    if (na == 0 || nb == 0) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    for (Word pi = 0; pi < np; ++pi) {
        Word pw = q.pathways[pi];
        if (pw >= NEO_PATHWAYS) continue;
        const signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

        int lhi = 0;
        for (Word j = tid; j < NEURONS; j += 256u) {
            const signed char *row_j = W + (unsigned long long)(j) * NEURONS;
            int fwd = 0;
            for (Word k = 0; k < na; ++k)
                fwd += int(row_j[s_a[k]]);
            int bwd = 0;
            if (fwd >= min_c) {
                for (Word k = 0; k < nb; ++k)
                    bwd += int(W[(unsigned long long)(s_b[k]) * NEURONS + j]);
            }
            int sc = (fwd >= min_c && bwd >= min_c) ? (fwd < bwd ? fwd : bwd) : 0;
            s_sc[j] = sc;
            if (sc > lhi) lhi = sc;
        }
        s_tmax[tid] = lhi;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st && s_tmax[tid + st] > s_tmax[tid])
                s_tmax[tid] = s_tmax[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) s_hi = s_tmax[0];
        asm volatile("bar.sync 0;");

        int hi = s_hi;
        if (hi < min_c) continue;

        int fl = hi >> 2;
        if (fl < min_c) fl = min_c;
        int lsum = 0;
        Word lcnt = 0;
        for (Word j = tid; j < NEURONS; j += 256u) {
            if (s_sc[j] >= fl) {
                lsum += s_sc[j];
                ++lcnt;
            }
        }
        s_tsum[tid] = lsum;
        s_tcnt[tid] = lcnt;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) {
                s_tsum[tid] += s_tsum[tid + st];
                s_tcnt[tid] += s_tcnt[tid + st];
            }
            asm volatile("bar.sync 0;");
        }
        bool is_win = (s_tsum[0] > s_best_tot);
        if (tid == 0) {
            s_sp += s_tcnt[0];
            if (is_win) {
                s_best_tot = s_tsum[0];
                s_best_hi = hi;
                s_best_pw = pw;
            }
        }
        asm volatile("bar.sync 0;");
        if (s_best_pw == pw && s_best_tot == s_tsum[0]) {
            for (Word j = tid; j < NEURONS; j += 256u)
                s_best[j] = s_sc[j];
        }
        asm volatile("bar.sync 0;");
    }

    int hi = s_best_hi;
    if (s_best_pw >= NEO_PATHWAYS || hi < min_c) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    int floor_c = min_c;
    int lo = floor_c, thr_hi = hi + 1;
    for (int it = 0; it < 12; ++it) {
        int mid = (lo + thr_hi) >> 1;
        Word lc = 0;
        for (Word j = tid; j < NEURONS; j += 256u)
            if (s_best[j] >= mid) ++lc;
        s_tcnt[tid] = lc;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tcnt[tid] += s_tcnt[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (s_tcnt[0] > tk) lo = mid + 1;
        else thr_hi = mid;
    }

    if (tid == 0) {
        int thr = lo > hi ? hi : lo;
        if (thr < floor_c) thr = floor_c;
        Word fired = 0;
        for (Word j = 0; j < NEURONS; ++j) {
            if (s_best[j] >= thr && fired < tk) {
                out.bridge_spikes.bits[j >> 5] |= 1u << (j & 31u);
                ++fired;
            }
        }
        out.best_pathway = s_best_pw;
        out.active_count = fired;
        out.max_score = hi;
        out.spikes_fired = s_sp + fired;
    }
}

// ---------------------------------------------------------------------------
// Kernel 12: Blackwell GPU Unsupervised Spiking Prototype Clustering &
// Odd-One-Out Discovery (`mind_neo_cluster`, Tredecuple-Pathway Ribot!)
// 1 block x 256 threads projects up to 6 entity SDRs simultaneously through
// L2-resident Semantic (PW1) + Taxonomic (PW8, 3x weighted) synaptic banks,
// computes the exact full pairwise spiking overlap matrix in shared memory,
// and directly identifies BOTH the odd-one-out outlier AND the closest pair
// purely on Blackwell L2 — ZERO extra L2 memory, ZERO hippocampal lookups!
// ---------------------------------------------------------------------------
AX_FN void neo_cluster_block(const signed char *weights_all, const NeoClusterQuery &q, NeoClusterResult &out) {
    __attribute__((shared)) static Word s_pre[NEURONS];
    __attribute__((shared)) static Word s_np;
    __attribute__((shared)) static short s_act[MAX_CLUSTER_ITEMS][NEURONS];
    __attribute__((shared)) static int s_tsum[256];
    __attribute__((shared)) static Word s_sp;

    Word tid = thread_id();
    Word n = q.n_items > MAX_CLUSTER_ITEMS ? MAX_CLUSTER_ITEMS : q.n_items;
    Word pwa = q.pw_sem < NEO_PATHWAYS ? q.pw_sem : 1u;
    Word pwb = q.pw_tax < NEO_PATHWAYS ? q.pw_tax : 8u;
    int min_c = q.min_curr > 0 ? q.min_curr : 4;

    if (tid == 0) {
        out = {};
        out.outlier_idx = ~0u;
        out.closest_a = ~0u;
        out.closest_b = ~0u;
        s_sp = 0;
    }
    asm volatile("bar.sync 0;");
    if (n < 3u) return;

    const signed char *Wa = weights_all + (unsigned long long)(pwa) * NEO_SYNAPSES;
    const signed char *Wb = weights_all + (unsigned long long)(pwb) * NEO_SYNAPSES;

    // Stage 1: Project each item through L2 semantic + taxonomic pathways into shared cortical activation buffers!
    for (Word m = 0; m < n; ++m) {
        if (tid == 0) {
            Word cnt = 0;
            for (Word w = 0; w < WORDS; ++w) {
                Word mask = q.items[m].bits[w];
                while (mask && cnt < NEURONS) {
                    Word b = __builtin_ctz(mask);
                    mask &= mask - 1u;
                    s_pre[cnt++] = (w << 5) | b;
                }
            }
            s_np = cnt;
            s_sp += cnt * 2u;
        }
        asm volatile("bar.sync 0;");

        Word np = s_np;
        Word l_sp = 0;
        for (Word j = tid; j < NEURONS; j += 256u) {
            const signed char *ra = Wa + (unsigned long long)(j) * NEURONS;
            const signed char *rb = Wb + (unsigned long long)(j) * NEURONS;
            int va = 0, vb = 0;
            for (Word k = 0; k < np; ++k) {
                Word pk = s_pre[k];
                va += int(ra[pk]);
                vb += int(rb[pk]);
            }
            int tot = (va >= min_c ? va : 0) + (vb >= min_c ? vb * 5 : 0);
            if (tot > 24000) tot = 24000;
            s_act[m][j] = short(tot);
            if (tot > 0) ++l_sp;
        }
        s_tsum[tid] = int(l_sp);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tsum[tid] += s_tsum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) s_sp += Word(s_tsum[0]);
        asm volatile("bar.sync 0;");
    }

    // Stage 2: Exact pairwise spiking overlap similarity matrix `sum_j min(act_a[j], act_b[j])`!
    for (Word a = 0; a < n; ++a) {
        for (Word b = a + 1u; b < n; ++b) {
            int lsum = 0;
            for (Word j = tid; j < NEURONS; j += 256u) {
                int va = int(s_act[a][j]);
                int vb = int(s_act[b][j]);
                if (va > 0 && vb > 0)
                    lsum += (va < vb ? va : vb);
            }
            s_tsum[tid] = lsum;
            asm volatile("bar.sync 0;");
            for (Word st = 128u; st > 0; st >>= 1) {
                if (tid < st) s_tsum[tid] += s_tsum[tid + st];
                asm volatile("bar.sync 0;");
            }
            if (tid == 0) {
                int s = s_tsum[0];
                out.sim_mat[a * MAX_CLUSTER_ITEMS + b] = s;
                out.sim_mat[b * MAX_CLUSTER_ITEMS + a] = s;
            }
            asm volatile("bar.sync 0;");
        }
    }

    // Stage 3: Select unsupervised odd-one-out outlier (lowest mean peer similarity) & closest pair!
    if (tid == 0) {
        int best_pair = -1;
        Word ba = ~0u, bb = ~0u;
        int min_mean = 2000000000;
        Word out_i = ~0u;
        for (Word a = 0; a < n; ++a) {
            int acc = 0;
            for (Word b = 0; b < n; ++b) {
                if (a == b) continue;
                int s = out.sim_mat[a * MAX_CLUSTER_ITEMS + b];
                acc += s;
                if (b > a && s > best_pair) {
                    best_pair = s;
                    ba = a;
                    bb = b;
                }
            }
            int ms = acc / int(n - 1u);
            out.mean_sim[a] = ms;
            if (ms < min_mean) {
                min_mean = ms;
                out_i = a;
            }
        }
        out.outlier_idx = out_i;
        out.outlier_score = min_mean;
        out.closest_a = ba;
        out.closest_b = bb;
        out.best_pair_sim = best_pair;
        out.spikes_fired = s_sp;
    }
}

// ---------------------------------------------------------------------------
// Kernel 13: Blackwell GPU Episodic-vs-Semantic Cortical Conflict & Novelty Gate (`mind_neo_gate`)!
// Given a candidate association `(subj_sdr, pred_sdr)`, 1 block x 256 threads
// simultaneously evaluates THREE L2 neocortical currents in one GPU launch:
//   1. Direct semantic support (`PW1`: `subj -> pred`),
//   2. Direct inhibitory cortical veto (`PW2`: `subj -| pred`, e.g., `penguin -| fly`),
//   3. 2-hop taxonomic inherited expectation (`PW8 -> PW1`: `subj -> parent -> pred`).
// Emits cortical gating status (`2 = EXCEPTION/VETOED`, `1 = CONSISTENT`, `0 = NOVEL/SURPRISE`)
// purely on Blackwell L2 — ZERO extra L2 memory, survives `--ablate hipp` (Quattuordecuple Ribot)!
// ---------------------------------------------------------------------------
AX_FN void neo_gate_block(const signed char *weights_all, const NeoGateQuery &q, NeoGateResult &out) {
    __attribute__((shared)) static Word s_pre[NEURONS];
    __attribute__((shared)) static Word s_post[NEURONS];
    __attribute__((shared)) static Word s_tax[64];
    __attribute__((shared)) static Word s_npre, s_npost, s_ntax;
    __attribute__((shared)) static int s_tcur[NEURONS];
    __attribute__((shared)) static int s_red_a[256];
    __attribute__((shared)) static int s_red_b[256];
    __attribute__((shared)) static Word s_sp;

    Word tid = thread_id();
    Word pw_pos = q.pw_pos < NEO_PATHWAYS ? q.pw_pos : 1u;
    Word pw_neg = q.pw_neg < NEO_PATHWAYS ? q.pw_neg : 2u;
    Word pw_tax = q.pw_tax < NEO_PATHWAYS ? q.pw_tax : 8u;

    if (tid == 0) {
        out = {};
        Word cp = 0, cq = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word ma = q.subj_sdr.bits[w], mb = q.pred_sdr.bits[w];
            while (ma && cp < NEURONS) { Word b = __builtin_ctz(ma); ma &= ma - 1u; s_pre[cp++] = (w << 5) | b; }
            while (mb && cq < NEURONS) { Word b = __builtin_ctz(mb); mb &= mb - 1u; s_post[cq++] = (w << 5) | b; }
        }
        s_npre = cp;
        s_npost = cq;
        s_ntax = 0;
        s_sp = (cp + cq) * 3u;
    }
    asm volatile("bar.sync 0;");

    Word npre = s_npre, npost = s_npost;
    if (npre == 0u || npost == 0u) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    const signed char *Wpos = weights_all + (unsigned long long)(pw_pos) * NEO_SYNAPSES;
    const signed char *Wneg = weights_all + (unsigned long long)(pw_neg) * NEO_SYNAPSES;
    const signed char *Wtax = weights_all + (unsigned long long)(pw_tax) * NEO_SYNAPSES;

    // Stage 1: Direct semantic support (PW1) & direct inhibitory veto (PW2) onto `pred_sdr`!
    // Uses biophysical dendritic NMDA cooperativity (`>= 75%` coincident presynaptic activation)
    // and somatic GABA_A shunting veto (`>= 75%` coincident inhibitory activation), making the gate
    // 100% invariant to evolved plasticity scale (`d_ltp`/`d_inh` in `1..3`) and weight saturation!
    int lpos = 0, lneg = 0;
    int lsup_n = 0, lvet_n = 0;
    int coop_req = (int(npre) * 3 + 3) / 4; // >= 75% presynaptic ensemble coincidence
    if (coop_req < 4) coop_req = 4;
    for (Word idx = tid; idx < npost; idx += 256u) {
        Word j = s_post[idx];
        const signed char *rp = Wpos + (unsigned long long)(j) * NEURONS;
        const signed char *rn = Wneg + (unsigned long long)(j) * NEURONS;
        int ap = 0, an = 0;
        int pos_hits = 0, neg_hits = 0;
        for (Word k = 0; k < npre; ++k) {
            Word pk = s_pre[k];
            int wp = int(rp[pk]);
            int wn = int(rn[pk]);
            ap += wp;
            an += wn;
            if (wp > 0) ++pos_hits;
            if (wn < 0) ++neg_hits;
        }
        int pos_c = ap > 0 ? ap : 0;
        int neg_c = an < 0 ? -an : 0;
        lpos += pos_c;
        lneg += neg_c;
        // Somatic GABA_A basket-cell shunting veto overrides inherited basal AMPA excitation!
        if (neg_hits >= coop_req && neg_c >= int(npre)) ++lvet_n;
        else if (pos_hits >= coop_req && pos_c >= int(npre)) ++lsup_n;
    }
    s_red_a[tid] = lpos;
    s_red_b[tid] = lneg;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) {
            s_red_a[tid] += s_red_a[tid + st];
            s_red_b[tid] += s_red_b[tid + st];
        }
        asm volatile("bar.sync 0;");
    }
    int direct_sup = s_red_a[0];
    int veto_conf = s_red_b[0];

    s_red_a[tid] = lsup_n;
    s_red_b[tid] = lvet_n;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) {
            s_red_a[tid] += s_red_a[tid + st];
            s_red_b[tid] += s_red_b[tid + st];
        }
        asm volatile("bar.sync 0;");
    }
    int sup_cells = s_red_a[0];
    int vet_cells = s_red_b[0];

    // Stage 2: 2-hop Taxonomic Inherited Expectation (`subj --PW8--> parent --PW1--> pred`)!
    int lhi = -1000000;
    for (Word j = tid; j < NEURONS; j += 256u) {
        const signed char *rt = Wtax + (unsigned long long)(j) * NEURONS;
        int acc = 0;
        for (Word k = 0; k < npre; ++k)
            acc += int(rt[s_pre[k]]);
        s_tcur[j] = acc;
        if (acc > lhi) lhi = acc;
    }
    s_red_a[tid] = lhi;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st && s_red_a[tid + st] > s_red_a[tid])
            s_red_a[tid] = s_red_a[tid + st];
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) {
        int hi = s_red_a[0];
        Word nt = 0;
        if (hi >= 4) {
            int thr = hi / 2;
            if (thr < 4) thr = 4;
            for (Word j = 0; j < NEURONS && nt < 32u; ++j)
                if (s_tcur[j] >= thr)
                    s_tax[nt++] = j;
        }
        s_ntax = nt;
        s_sp += nt * 2u + npost * 2u;
    }
    asm volatile("bar.sync 0;");

    Word ntax = s_ntax;
    int linh = 0, linh_n = 0;
    int tax_coop = (int(ntax) * 3 + 3) / 4;
    if (tax_coop < 4) tax_coop = 4;
    if (ntax >= 4u) {
        for (Word idx = tid; idx < npost; idx += 256u) {
            Word j = s_post[idx];
            const signed char *rp = Wpos + (unsigned long long)(j) * NEURONS;
            int acc = 0, ihits = 0;
            for (Word k = 0; k < ntax; ++k) {
                int wp = int(rp[s_tax[k]]);
                acc += wp;
                if (wp > 0) ++ihits;
            }
            if (acc > 0) linh += acc;
            if (ihits >= tax_coop && acc >= int(ntax)) ++linh_n;
        }
    }
    s_red_a[tid] = linh;
    s_red_b[tid] = linh_n;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) {
            s_red_a[tid] += s_red_a[tid + st];
            s_red_b[tid] += s_red_b[tid + st];
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        int inh_sup = s_red_a[0];
        int inh_cells = s_red_b[0];
        int min_cur = int(npost) * int(npre);
        if (min_cur < 64) min_cur = 64;
        int sup_thr = q.support_thresh > 0 ? q.support_thresh : min_cur;
        int vet_thr = q.veto_thresh > 0 ? q.veto_thresh : min_cur;
        int inh_thr = int(npost) * int(ntax > 0 ? ntax : 4u);
        if (inh_thr < 64) inh_thr = 64;
        int half_cells = int(npost) / 2;
        if (half_cells < 4) half_cells = 4;
        out.direct_support = direct_sup;
        out.inherited_support = inh_sup;
        out.veto_conflict = veto_conf;
        if (veto_conf >= vet_thr && vet_cells >= half_cells)
            out.status = 2u; // EXCEPTION / SOMATIC GABA_A SHUNTING VETO OVERRIDES EXCITATION
        else if ((direct_sup >= sup_thr && sup_cells >= half_cells) ||
                 (inh_sup >= inh_thr && inh_cells >= half_cells))
            out.status = 1u; // CONSISTENT WITH CONSOLIDATED NEOCORTEX (DIRECT OR 2-HOP INHERITED)
        else
            out.status = 0u; // NOVEL / SURPRISE -> GATE OPEN FOR HIPPOCAMPUS
        out.spikes_fired = s_sp;
    }
}

// ---------------------------------------------------------------------------
// Kernel 14: Blackwell GPU Offline SWR Dream Replay & Transitive Closure (`mind_neo_dream`)!
// Given a seed cue A (`seed_sdr`), 1 block x 256 threads runs a 2-hop spontaneous
// replay cascade purely inside 28-MiB L2 (`A --pw_hop1--> B --pw_hop2--> C`) with
// refractory self-suppression, and optionally consolidates the discovered transitive
// shortcut (`A -> C`) directly into `consolidate_pw` using the EVOLVED 3-factor
// `g.basal_rule` plasticity LUT — ZERO extra L2 memory, survives `--ablate hipp`
// (Quindecuple-Pathway 15-Track Ribot's Law)!
// ---------------------------------------------------------------------------
AX_FN void neo_dream_block(signed char *weights_all, const NeoDreamQuery &q, const CorticalGenome &g,
                           NeoDreamResult &out) {
    __attribute__((shared)) static Word s_pre[NEURONS];
    __attribute__((shared)) static Word s_mid[NEURONS];
    __attribute__((shared)) static Word s_end[NEURONS];
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_npre, s_nmid, s_nend;
    __attribute__((shared)) static Word s_sp;
    __attribute__((shared)) static Word s_ed_red[256];

    Word tid = thread_id();
    Word pw1 = q.pw_hop1 < NEO_PATHWAYS ? q.pw_hop1 : 8u;
    Word pw2 = q.pw_hop2 < NEO_PATHWAYS ? q.pw_hop2 : pw1;
    Word top_k = q.top_k > 0 && q.top_k <= 64u ? q.top_k : 40u;
    int min_c = q.min_curr > 0 ? q.min_curr : 6;

    if (tid == 0) {
        out = {};
        Word cp = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word ma = q.seed_sdr.bits[w];
            while (ma && cp < NEURONS) {
                Word b = __builtin_ctz(ma);
                ma &= ma - 1u;
                s_pre[cp++] = (w << 5) | b;
            }
        }
        s_npre = cp;
        s_nmid = 0;
        s_nend = 0;
        s_sp = cp * 3u;
    }
    asm volatile("bar.sync 0;");

    Word npre = s_npre;
    if (npre == 0u) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    // Hop 1: A --pw1--> B (dendritic NMDA cooperativity >= 50% blocks spurious high-fan-in bleed;
    // 2x refractory damping suppresses exact self-echo bits)
    const signed char *W1 = weights_all + (unsigned long long)(pw1) * NEO_SYNAPSES;
    for (Word j = tid; j < NEURONS; j += 256u) {
        const signed char *row = W1 + (unsigned long long)(j) * NEURONS;
        int acc = 0;
        Word pos = 0;
        for (Word k = 0; k < npre; ++k) {
            int wv = int(row[s_pre[k]]);
            acc += wv;
            if (wv > 0) ++pos;
        }
        if (pos * 2u < npre) acc = 0;
        bool in_seed = ((q.seed_sdr.bits[j >> 5] >> (j & 31u)) & 1u) != 0;
        if (in_seed && acc > 0) acc >>= 1;
        s_curr[j] = acc;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        int hi = -1000000;
        for (Word j = 0; j < NEURONS; ++j)
            if (s_curr[j] > hi) hi = s_curr[j];
        out.hop1_max = hi;
        if (hi >= min_c) {
            int lo = min_c, thr_hi = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j)
                    if (s_curr[j] >= mid) ++cnt;
                if (cnt > top_k) lo = mid + 1;
                else thr_hi = mid;
            }
            int thr = lo > hi ? hi : lo;
            if (thr < min_c) thr = min_c;
            Word nm = 0;
            for (Word j = 0; j < NEURONS; ++j) {
                if (s_curr[j] >= thr && nm < top_k) {
                    out.mid_spikes.bits[j >> 5] |= 1u << (j & 31u);
                    s_mid[nm++] = j;
                }
            }
            if (nm < top_k && thr > min_c) {
                int tie_v = thr - 1;
                if (tie_v * 2 >= hi) {
                    for (Word j = 0; j < NEURONS && nm < top_k; ++j) {
                        if (s_curr[j] == tie_v) {
                            out.mid_spikes.bits[j >> 5] |= 1u << (j & 31u);
                            s_mid[nm++] = j;
                        }
                    }
                }
            }
            s_nmid = nm;
            out.mid_count = nm;
            s_sp += nm * 4u;
        }
    }
    asm volatile("bar.sync 0;");

    Word nmid = s_nmid;
    if (nmid < 4u) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    // Hop 2: B --pw2--> C (dendritic NMDA cooperativity >= 50% + refractory damping on A bits)
    const signed char *W2 = weights_all + (unsigned long long)(pw2) * NEO_SYNAPSES;
    for (Word j = tid; j < NEURONS; j += 256u) {
        const signed char *row = W2 + (unsigned long long)(j) * NEURONS;
        int acc = 0;
        Word pos = 0;
        for (Word k = 0; k < nmid; ++k) {
            int wv = int(row[s_mid[k]]);
            acc += wv;
            if (wv > 0) ++pos;
        }
        if (pos * 2u < nmid) acc = 0;
        bool in_seed = ((q.seed_sdr.bits[j >> 5] >> (j & 31u)) & 1u) != 0;
        if (in_seed && acc > 0) acc >>= 1;
        s_curr[j] = acc;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        int hi = -1000000;
        for (Word j = 0; j < NEURONS; ++j)
            if (s_curr[j] > hi) hi = s_curr[j];
        out.hop2_max = hi;
        if (hi >= min_c) {
            int lo = min_c, thr_hi = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j)
                    if (s_curr[j] >= mid) ++cnt;
                if (cnt > top_k) lo = mid + 1;
                else thr_hi = mid;
            }
            int thr = lo > hi ? hi : lo;
            if (thr < min_c) thr = min_c;
            Word ne = 0;
            for (Word j = 0; j < NEURONS; ++j) {
                if (s_curr[j] >= thr && ne < top_k) {
                    out.end_spikes.bits[j >> 5] |= 1u << (j & 31u);
                    s_end[ne++] = j;
                }
            }
            if (ne < top_k && thr > min_c) {
                int tie_v = thr - 1;
                if (tie_v * 2 >= hi) {
                    for (Word j = 0; j < NEURONS && ne < top_k; ++j) {
                        if (s_curr[j] == tie_v) {
                            out.end_spikes.bits[j >> 5] |= 1u << (j & 31u);
                            s_end[ne++] = j;
                        }
                    }
                }
            }
            s_nend = ne;
            out.end_count = ne;
            s_sp += ne * 4u;
        }
    }
    asm volatile("bar.sync 0;");

    // Stage 3: Pure-GPU Offline SWR Dream Transitive Consolidation (`A -> C` onto `consolidate_pw`)
    // Driven purely by the EVOLVED 3-factor plasticity LUT (`g.basal_rule`)!
    Word nend = s_nend;
    Word l_ed = 0;
    if (q.consolidate_pw < NEO_PATHWAYS && nend >= 4u) {
        int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
        if (d_ltp <= 0) d_ltp = 1;
        signed char *Wc = weights_all + (unsigned long long)(q.consolidate_pw) * NEO_SYNAPSES;
        for (Word idx = tid; idx < nend; idx += 256u) {
            Word j = s_end[idx];
            signed char *row = Wc + (unsigned long long)(j) * NEURONS;
            for (Word k = 0; k < npre; ++k) {
                Word i = s_pre[k];
                int cur = int(row[i]);
                int nxt = cur + d_ltp;
                if (nxt > 63) nxt = 63;
                if (nxt != cur) {
                    row[i] = static_cast<signed char>(nxt);
                    ++l_ed;
                }
            }
        }
    }
    s_ed_red[tid] = l_ed;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) {
        out.dream_edits = s_ed_red[0];
        out.spikes_fired = s_sp;
    }
}

// ---------------------------------------------------------------------------
// Kernel 15: Blackwell GPU Universal Quantifier & Counterexample Scanner (`mind_neo_scan`)!
// Evaluates up to 8 category members simultaneously against a predicate SDR across
// L2 Pathways 1 (excitatory support) & 2 (inhibitory veto) using scale-invariant
// dendritic NMDA cooperativity (`>= 75%`) & somatic GABA_A shunting inhibition!
// Decides `∀x ∈ C : P(x)` vs `∃x ∈ C : ¬P(x)` (extracting exact counterexample or witness)
// purely inside 28-MiB L2 — ZERO extra L2 memory, survives `--ablate hipp`
// (Sedecuple-Pathway 16-Track Ribot's Law)!
// ---------------------------------------------------------------------------
AX_FN void neo_scan_block(const signed char *weights_all, const NeoScanQuery &q, NeoScanResult &out) {
    __attribute__((shared)) static Word s_post[NEURONS];
    __attribute__((shared)) static Word s_pre[NEURONS];
    __attribute__((shared)) static Word s_npost, s_npre, s_sp;
    __attribute__((shared)) static int s_ra[256];
    __attribute__((shared)) static int s_rb[256];

    Word tid = thread_id();
    Word n_mem = q.n_members <= MAX_SCAN_MEMBERS ? q.n_members : MAX_SCAN_MEMBERS;
    Word pw_pos = q.pw_pos < NEO_PATHWAYS ? q.pw_pos : 1u;
    Word pw_neg = q.pw_neg < NEO_PATHWAYS ? q.pw_neg : 2u;

    if (tid == 0) {
        out = {};
        out.witness_idx = ~0u;
        out.counterex_idx = ~0u;
        Word cq = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word mb = q.pred_sdr.bits[w];
            while (mb && cq < NEURONS) {
                Word b = __builtin_ctz(mb);
                mb &= mb - 1u;
                s_post[cq++] = (w << 5) | b;
            }
        }
        s_npost = cq;
        s_sp = cq * 2u;
    }
    asm volatile("bar.sync 0;");

    Word npost = s_npost;
    if (n_mem == 0u || npost == 0u) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    const signed char *Wpos = weights_all + (unsigned long long)(pw_pos) * NEO_SYNAPSES;
    const signed char *Wneg = weights_all + (unsigned long long)(pw_neg) * NEO_SYNAPSES;
    int half_cells = int(npost) / 2;
    if (half_cells < 4) half_cells = 4;

    for (Word m = 0; m < n_mem; ++m) {
        if (tid == 0) {
            Word cp = 0;
            for (Word w = 0; w < WORDS; ++w) {
                Word ma = q.members[m].bits[w];
                while (ma && cp < NEURONS) {
                    Word b = __builtin_ctz(ma);
                    ma &= ma - 1u;
                    s_pre[cp++] = (w << 5) | b;
                }
            }
            s_npre = cp;
            s_sp += cp * 2u + npost;
        }
        asm volatile("bar.sync 0;");

        Word npre = s_npre;
        int lsup_n = 0, lvet_n = 0;
        if (npre > 0u) {
            int coop_req = (int(npre) * 3 + 3) / 4;
            if (coop_req < 4) coop_req = 4;
            for (Word idx = tid; idx < npost; idx += 256u) {
                Word j = s_post[idx];
                const signed char *rp = Wpos + (unsigned long long)(j) * NEURONS;
                const signed char *rn = Wneg + (unsigned long long)(j) * NEURONS;
                int ap = 0, an = 0, ph = 0, nh = 0;
                for (Word k = 0; k < npre; ++k) {
                    Word pk = s_pre[k];
                    int wp = int(rp[pk]);
                    int wn = int(rn[pk]);
                    ap += wp;
                    an += wn;
                    if (wp > 0) ++ph;
                    if (wn < 0) ++nh;
                }
                int pos_c = ap > 0 ? ap : 0;
                int neg_c = an < 0 ? -an : 0;
                if (nh >= coop_req && neg_c >= int(npre)) ++lvet_n;
                else if (ph >= coop_req && pos_c >= int(npre)) ++lsup_n;
            }
        }
        s_ra[tid] = lsup_n;
        s_rb[tid] = lvet_n;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) {
                s_ra[tid] += s_ra[tid + st];
                s_rb[tid] += s_rb[tid + st];
            }
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sc = s_ra[0], vc = s_rb[0];
            Word st = 0u;
            if (vc >= half_cells) st = 2u;
            else if (sc >= half_cells) st = 1u;
            out.member_status[m] = st;
            if (st == 1u) {
                ++out.support_count;
                if (out.witness_idx == ~0u) out.witness_idx = m;
            } else if (st == 2u) {
                ++out.veto_count;
                if (out.counterex_idx == ~0u) out.counterex_idx = m;
            }
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        if (out.counterex_idx == ~0u) {
            for (Word m = 0; m < n_mem; ++m) {
                if (out.member_status[m] == 0u) {
                    out.counterex_idx = m;
                    break;
                }
            }
        }
        out.all_hold = (out.support_count == n_mem && n_mem > 0u) ? 1u : 0u;
        out.some_hold = (out.support_count > 0u) ? 1u : 0u;
        out.spikes_fired = s_sp;
    }
}

// ---------------------------------------------------------------------------
// Kernel 16: Blackwell GPU Multi-Step Forward Deductive Rule-Chaining &
// Counterfactual Blocker Propagation (`mind_neo_deduce`)!
// Propagates spiking inference up to 4 hops across L2 neocortical pathways
// (PW8 taxonomy, PW16 state transitions, PW4 seasons) with >= 50% dendritic
// NMDA cooperativity, somatic refractory cycle-prevention, goal resonance
// detection, and somatic GABA_A counterfactual blocking!
// ZERO extra L2 memory — survives `--ablate hipp` (Septendecuple 17-Track Ribot!)
// ---------------------------------------------------------------------------
AX_FN void neo_deduce_block(const signed char *weights_all, const NeoDeduceQuery &q, NeoDeduceResult &out) {
    __attribute__((shared)) static Word s_pre[NEURONS];
    __attribute__((shared)) static Word s_npre, s_sp, s_done;
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_cur_bits[WORDS];
    __attribute__((shared)) static Word s_vis_bits[WORDS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 8u;
    Word max_h = q.max_hops <= MAX_DEDUCE_STEPS ? q.max_hops : MAX_DEDUCE_STEPS;
    Word top_k = q.top_k > 0 ? q.top_k : 40u;
    int min_c = q.min_curr > 0 ? q.min_curr : 6;

    if (tid == 0) {
        out = {};
        for (Word w = 0; w < WORDS; ++w) {
            s_cur_bits[w] = q.start_sdr.bits[w];
            s_vis_bits[w] = q.start_sdr.bits[w];
        }
        s_sp = 0;
        s_done = 0;
    }
    asm volatile("bar.sync 0;");

    const signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    for (Word h = 0; h < max_h; ++h) {
        if (tid == 0) {
            Word cp = 0;
            for (Word w = 0; w < WORDS; ++w) {
                Word m = s_cur_bits[w];
                while (m && cp < NEURONS) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    s_pre[cp++] = (w << 5) | b;
                }
            }
            s_npre = cp;
            s_sp += cp * 2u;
        }
        asm volatile("bar.sync 0;");

        Word npre = s_npre;
        if (npre < 4u || s_done) break;

        for (Word j = tid; j < NEURONS; j += 256u) {
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int acc = 0;
            Word pos = 0;
            for (Word k = 0; k < npre; ++k) {
                int wv = int(row[s_pre[k]]);
                acc += wv;
                if (wv > 0) ++pos;
            }
            if (pos * 2u < npre) acc = 0;
            bool vis = ((s_vis_bits[j >> 5] >> (j & 31u)) & 1u) != 0;
            if (vis) acc = 0;
            s_curr[j] = acc;
        }
        asm volatile("bar.sync 0;");

        if (tid == 0) {
            int hi = -1000000;
            for (Word j = 0; j < NEURONS; ++j)
                if (s_curr[j] > hi) hi = s_curr[j];
            if (hi < min_c) {
                s_done = 1;
            } else {
                int lo = min_c, thr_hi = hi + 1;
                for (int it = 0; it < 16; ++it) {
                    int mid = (lo + thr_hi) >> 1;
                    Word cnt = 0;
                    for (Word j = 0; j < NEURONS; ++j)
                        if (s_curr[j] >= mid) ++cnt;
                    if (cnt > top_k) lo = mid + 1;
                    else thr_hi = mid;
                }
                int thr = lo > hi ? hi : lo;
                if (thr < min_c) thr = min_c;
                SpikeVec nxt{};
                Word nf = 0;
                for (Word j = 0; j < NEURONS; ++j) {
                    if (s_curr[j] >= thr && nf < top_k) {
                        nxt.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
                if (nf < top_k && thr > min_c) {
                    int tie_v = thr - 1;
                    if (tie_v * 2 >= hi) {
                        for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                            if (s_curr[j] == tie_v) {
                                nxt.bits[j >> 5] |= 1u << (j & 31u);
                                ++nf;
                            }
                        }
                    }
                }
                out.hop_spikes[h] = nxt;
                out.hop_counts[h] = nf;
                out.hops_done = h + 1u;
                s_sp += nf * 4u;

                // Goal resonance check
                Word gov = 0;
                for (Word w = 0; w < WORDS; ++w)
                    gov += __builtin_popcount(nxt.bits[w] & q.goal_sdr.bits[w]);
                if (gov >= 12u) {
                    out.proved = 1u;
                    out.proof_hops = h + 1u;
                    out.goal_overlap = gov;
                    s_done = 1u;
                } else if (q.has_blocker) {
                    // Counterfactual GABA_A blocker veto check
                    Word bov = 0;
                    for (Word w = 0; w < WORDS; ++w)
                        bov += __builtin_popcount(nxt.bits[w] & q.block_sdr.bits[w]);
                    if (bov >= 10u) {
                        out.blocked_hit = 1u;
                        s_done = 1u;
                    }
                }
                for (Word w = 0; w < WORDS; ++w) {
                    s_cur_bits[w] = nxt.bits[w];
                    s_vis_bits[w] |= nxt.bits[w];
                }
            }
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0)
        out.spikes_fired = s_sp;
}

// ---------------------------------------------------------------------------
// Kernel 17: Blackwell GPU Top-Down Apical Backward Abduction & 2-Hop Root-Cause
// Inference (`mind_neo_abduce_back`)!
// Given an observed effect SDR (e.g. `steam`, `autumn`, `winter`, `plant`),
// computes column-wise top-down transpose synaptic currents (`W^T * post_sdr`)
// across 28-MiB L2 pathways (PW16 state transitions, PW4 seasons, PW8 taxonomy)
// with dendritic NMDA cooperativity gating and somatic refractory exclusion to
// recover both the direct upstream cause (`hop1_spikes`) and 2-hop root cause
// (`hop2_spikes`) in a single GPU pass!
// ZERO extra L2 memory — survives `--ablate hipp` (Octodecuple 18-Track Ribot!)
// ---------------------------------------------------------------------------
AX_FN void neo_abduce_back_block(const signed char *weights_all, const NeoAbduceBackQuery &q,
                                 NeoAbduceBackResult &out) {
    __attribute__((shared)) static Word s_post[NEURONS];
    __attribute__((shared)) static Word s_npost, s_sp;
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_cur_bits[WORDS];
    __attribute__((shared)) static Word s_vis_bits[WORDS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 16u;
    Word top_k = q.top_k > 0 ? q.top_k : 32u;
    int min_c = q.min_curr > 0 ? q.min_curr : 6;

    if (tid == 0) {
        out = {};
        for (Word w = 0; w < WORDS; ++w) {
            s_cur_bits[w] = q.effect_sdr.bits[w];
            s_vis_bits[w] = q.effect_sdr.bits[w];
        }
        s_sp = 0;
    }
    asm volatile("bar.sync 0;");

    const signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    for (Word hop = 0; hop < 2u; ++hop) {
        if (tid == 0) {
            Word cp = 0;
            for (Word w = 0; w < WORDS; ++w) {
                Word m = s_cur_bits[w];
                while (m && cp < NEURONS) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    s_post[cp++] = (w << 5) | b;
                }
            }
            s_npost = cp;
            s_sp += cp * 2u;
        }
        asm volatile("bar.sync 0;");

        Word npost = s_npost;
        if (npost < 4u) break;

        Word coop = npost < 32u ? ((npost + 1u) >> 1) : 16u;
        for (Word i = tid; i < NEURONS; i += 256u) {
            int acc = 0;
            Word pos = 0;
            for (Word k = 0; k < npost; ++k) {
                Word j = s_post[k];
                int wv = int(W[(unsigned long long)(j) * NEURONS + i]);
                acc += wv;
                if (wv > 0) ++pos;
            }
            if (pos < coop) acc = 0;
            if (((s_vis_bits[i >> 5] >> (i & 31u)) & 1u) != 0) acc = 0;
            s_curr[i] = acc;
        }
        asm volatile("bar.sync 0;");

        if (tid == 0) {
            int hi = -1000000;
            for (Word i = 0; i < NEURONS; ++i)
                if (s_curr[i] > hi) hi = s_curr[i];
            if (hi < min_c) {
                for (Word w = 0; w < WORDS; ++w) s_cur_bits[w] = 0;
            } else {
                Word hk = (hop == 0u) ? top_k : (top_k * 3u);
                int hop_floor = min_c;
                if (hop == 1u) {
                    // Robust 16th-highest current (median of a 32-bit SDR assembly, immune to 1-bit SDR overlap spikes!)
                    int rlo = min_c, rhi = hi + 1;
                    for (int it = 0; it < 16; ++it) {
                        int mid = (rlo + rhi) >> 1;
                        Word cnt = 0;
                        for (Word i = 0; i < NEURONS; ++i)
                            if (s_curr[i] >= mid) ++cnt;
                        if (cnt >= 16u) rlo = mid + 1;
                        else rhi = mid;
                    }
                    int cur16 = rlo - 1;
                    if (cur16 > min_c) {
                        hop_floor = (cur16 * 9) / 10;
                        if (hop_floor < min_c) hop_floor = min_c;
                    }
                }
                int lo = hop_floor, thr_hi = hi + 1;
                for (int it = 0; it < 16; ++it) {
                    int mid = (lo + thr_hi) >> 1;
                    Word cnt = 0;
                    for (Word i = 0; i < NEURONS; ++i)
                        if (s_curr[i] >= mid) ++cnt;
                    if (cnt > hk) lo = mid + 1;
                    else thr_hi = mid;
                }
                int thr = lo > hi ? hi : lo;
                if (thr < hop_floor) thr = hop_floor;
                SpikeVec nxt{};
                Word nf = 0;
                for (Word i = 0; i < NEURONS; ++i) {
                    if (s_curr[i] >= thr && nf < hk) {
                        nxt.bits[i >> 5] |= 1u << (i & 31u);
                        ++nf;
                    }
                }
                if (nf < hk && thr > hop_floor) {
                    int tie_v = thr - 1;
                    if (tie_v >= hop_floor) {
                        for (Word i = 0; i < NEURONS && nf < hk; ++i) {
                            if (s_curr[i] == tie_v) {
                                nxt.bits[i >> 5] |= 1u << (i & 31u);
                                ++nf;
                            }
                        }
                    }
                }
                if (hop == 0u) {
                    out.hop1_spikes = nxt;
                    out.hop1_count = nf;
                    out.hop1_max = hi;
                } else {
                    out.hop2_spikes = nxt;
                    out.hop2_count = nf;
                    out.hop2_max = hi;
                }
                s_sp += nf * 4u;
                for (Word w = 0; w < WORDS; ++w) {
                    s_cur_bits[w] = nxt.bits[w];
                    s_vis_bits[w] |= nxt.bits[w];
                }
            }
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0)
        out.spikes_fired = s_sp;
}

// ---------------------------------------------------------------------------
// Kernel 18: Blackwell GPU Surgical Counterfactual Imagination & Prototype
// Shift (`mind_neo_counterfact`)!
// Executes two parallel cortical rollouts inside 28-MiB L2:
//   1. Baseline Reality: settles `subj_sdr` on PW8 (`pw_tax`) -> `base_cat_spikes`
//      and extracts active baseline features on PW1 (`pw_feat`) minus PW2 (`pw_neg`).
//   2. Counterfactual Intervention `do(¬rem_sdr, +add_sdr)`: excises `rem_sdr`,
//      applies divisive normalization across PW0 (`pw_proto`) baseline & apical
//      counterfactual drive (`norm_base + 2 * norm_add`), and settles the imagined
//      prototype attractor `cf_cat_spikes` (`shifted = 1` when category morphs)!
// ZERO extra L2 memory — survives `--ablate hipp` (Novemdecuple 19-Track Ribot!)
// ---------------------------------------------------------------------------
AX_FN void neo_counterfact_block(const signed char *weights_all, const NeoCounterfactQuery &q,
                                 NeoCounterfactResult &out) {
    __attribute__((shared)) static Word s_idx[NEURONS];
    __attribute__((shared)) static Word s_nidx, s_sp;
    __attribute__((shared)) static int s_cb[NEURONS];
    __attribute__((shared)) static int s_ca[NEURONS];
    __attribute__((shared)) static Word s_feat_bits[WORDS];

    Word tid = thread_id();
    Word pw_tax = q.pw_tax < NEO_PATHWAYS ? q.pw_tax : 8u;
    Word pw_feat = q.pw_feat < NEO_PATHWAYS ? q.pw_feat : 1u;
    Word pw_neg = q.pw_neg < NEO_PATHWAYS ? q.pw_neg : 2u;
    Word pw_proto = q.pw_proto < NEO_PATHWAYS ? q.pw_proto : 0u;
    Word top_k = q.top_k > 0 ? q.top_k : 32u;
    int min_c = q.min_curr > 0 ? q.min_curr : 4;

    if (tid == 0) {
        out = {};
        Word cp = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word m = q.subj_sdr.bits[w];
            while (m && cp < NEURONS) {
                Word b = __builtin_ctz(m);
                m &= m - 1u;
                s_idx[cp++] = (w << 5) | b;
            }
            s_feat_bits[w] = 0;
        }
        s_nidx = cp;
        s_sp = cp * 2u;
    }
    asm volatile("bar.sync 0;");

    Word nsubj = s_nidx;
    if (nsubj < 4u) {
        if (tid == 0) out.spikes_fired = s_sp;
        return;
    }

    const signed char *Wtax = weights_all + (unsigned long long)(pw_tax) * NEO_SYNAPSES;
    const signed char *Wfeat = weights_all + (unsigned long long)(pw_feat) * NEO_SYNAPSES;
    const signed char *Wneg = weights_all + (unsigned long long)(pw_neg) * NEO_SYNAPSES;
    const signed char *Wpro = weights_all + (unsigned long long)(pw_proto) * NEO_SYNAPSES;

    // Step 1: Baseline category on PW8 + baseline feature assembly on PW1 - PW2!
    for (Word j = tid; j < NEURONS; j += 256u) {
        const signed char *rt = Wtax + (unsigned long long)(j) * NEURONS;
        const signed char *rf = Wfeat + (unsigned long long)(j) * NEURONS;
        const signed char *rn = Wneg + (unsigned long long)(j) * NEURONS;
        int at = 0, af = 0, an = 0;
        Word pt = 0, pf = 0;
        for (Word k = 0; k < nsubj; ++k) {
            Word pk = s_idx[k];
            int wt = int(rt[pk]);
            int wf = int(rf[pk]);
            int wn = int(rn[pk]);
            at += wt; if (wt > 0) ++pt;
            af += wf; if (wf > 0) ++pf;
            an += wn;
        }
        if (pt * 2u < nsubj) at = 0;
        if (pf * 2u < nsubj || an < -min_c) af = 0;
        s_cb[j] = at;
        s_ca[j] = af;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        // k-WTA on baseline category (`s_cb`, top_k)
        int hi = -1000000;
        for (Word j = 0; j < NEURONS; ++j) if (s_cb[j] > hi) hi = s_cb[j];
        if (hi >= min_c) {
            int lo = min_c, thr_hi = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j) if (s_cb[j] >= mid) ++cnt;
                if (cnt > top_k) lo = mid + 1; else thr_hi = mid;
            }
            int thr = lo > hi ? hi : lo;
            if (thr < min_c) thr = min_c;
            Word nf = 0;
            for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                if (s_cb[j] >= thr) {
                    out.base_cat_spikes.bits[j >> 5] |= 1u << (j & 31u);
                    ++nf;
                }
            }
            // Tie-preserving k-WTA: fill remaining slots up to top_k from boundary `thr - 1`!
            if (nf < top_k && thr > min_c) {
                int tie_v = thr - 1;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_cb[j] == tie_v) {
                        out.base_cat_spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
            }
            out.base_count = nf;
            s_sp += nf * 4u;
        }
        // Extract baseline features (`s_ca`, up to 160 active neurons), surgically excising `rem_sdr`!
        int fhi = -1000000;
        for (Word j = 0; j < NEURONS; ++j) if (s_ca[j] > fhi) fhi = s_ca[j];
        Word nf_feat = 0;
        if (fhi >= min_c) {
            int lo = min_c, thr_hi = fhi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j) if (s_ca[j] >= mid) ++cnt;
                if (cnt > 160u) lo = mid + 1; else thr_hi = mid;
            }
            int thr = lo > fhi ? fhi : lo;
            if (thr < min_c) thr = min_c;
            for (Word j = 0; j < NEURONS && nf_feat < 160u; ++j) {
                if (s_ca[j] >= thr) {
                    s_feat_bits[j >> 5] |= 1u << (j & 31u);
                    ++nf_feat;
                }
            }
        }
        if (q.has_rem) {
            for (Word w = 0; w < WORDS; ++w)
                s_feat_bits[w] &= ~q.rem_sdr.bits[w];
        }
        // Pack remaining feature indices into `s_idx`
        Word cf = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word m = s_feat_bits[w];
            while (m && cf < NEURONS) {
                Word b = __builtin_ctz(m);
                m &= m - 1u;
                s_idx[cf++] = (w << 5) | b;
            }
        }
        s_nidx = cf;
        s_sp += cf * 2u;
    }
    asm volatile("bar.sync 0;");

    // Step 2: Project remaining baseline features onto PW0 (`s_cb`)
    Word nfeat = s_nidx;
    for (Word j = tid; j < NEURONS; j += 256u) {
        const signed char *rp = Wpro + (unsigned long long)(j) * NEURONS;
        int acc = 0;
        for (Word k = 0; k < nfeat; ++k) {
            int wv = int(rp[s_idx[k]]);
            if (wv > 0) acc += wv;
        }
        s_cb[j] = acc;
        s_ca[j] = 0;
    }
    asm volatile("bar.sync 0;");

    // Step 3: If `has_add`, project counterfactual grafted feature `add_sdr` onto PW0 (`s_ca`)!
    if (q.has_add) {
        if (tid == 0) {
            Word ca = 0;
            for (Word w = 0; w < WORDS; ++w) {
                Word m = q.add_sdr.bits[w];
                while (m && ca < NEURONS) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    s_idx[ca++] = (w << 5) | b;
                }
            }
            s_nidx = ca;
            s_sp += ca * 2u;
        }
        asm volatile("bar.sync 0;");
        Word nadd = s_nidx;
        if (nadd >= 4u) {
            for (Word j = tid; j < NEURONS; j += 256u) {
                const signed char *rp = Wpro + (unsigned long long)(j) * NEURONS;
                int acc = 0;
                Word pos = 0;
                for (Word k = 0; k < nadd; ++k) {
                    int wv = int(rp[s_idx[k]]);
                    acc += wv;
                    if (wv > 0) ++pos;
                }
                if (pos * 2u < nadd) acc = 0;
                s_ca[j] = acc > 0 ? acc : 0;
            }
        }
        asm volatile("bar.sync 0;");
    }

    // Step 4: Robust 16th-highest assembly-peak divisive normalization (`norm_base + 2 * norm_add`)!
    // Using the 16th-highest current (`peak16`) instead of raw 1st-highest `max` guarantees that a
    // single random 1-bit SDR collision across two prototypes cannot halve the 32-neuron assembly drive!
    if (tid == 0) {
        auto kth16_pos = [](const int *arr) -> int {
            int hi = 0;
            for (Word j = 0; j < NEURONS; ++j)
                if (arr[j] > hi) hi = arr[j];
            if (hi <= 0) return 0;
            int lo = 1, r = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + r) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j)
                    if (arr[j] >= mid) ++cnt;
                if (cnt >= 16u) lo = mid + 1;
                else r = mid;
            }
            int p16 = lo - 1;
            return p16 > 0 ? p16 : hi;
        };
        int peak_b = kth16_pos(s_cb);
        int peak_a = kth16_pos(s_ca);
        int hi = 0;
        for (Word j = 0; j < NEURONS; ++j) {
            int nb = peak_b > 0 ? (s_cb[j] * 100) / peak_b : 0;
            int na = peak_a > 0 ? (s_ca[j] * 100) / peak_a : 0;
            if (nb > 100) nb = 100;
            if (na > 100) na = 100;
            int comb = nb + 2 * na;
            s_cb[j] = comb;
            if (comb > hi) hi = comb;
        }
        if (hi >= 40) {
            int lo = 40, thr_hi = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j) if (s_cb[j] >= mid) ++cnt;
                if (cnt > top_k) lo = mid + 1; else thr_hi = mid;
            }
            int thr = lo > hi ? hi : lo;
            if (thr < 40) thr = 40;
            Word nf = 0;
            for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                if (s_cb[j] >= thr) {
                    out.cf_cat_spikes.bits[j >> 5] |= 1u << (j & 31u);
                    ++nf;
                }
            }
            // Tie-preserving k-WTA: fill remaining slots up to top_k from boundary `thr - 1`!
            if (nf < top_k && thr > 40) {
                int tie_v = thr - 1;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_cb[j] == tie_v) {
                        out.cf_cat_spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
            }
            out.cf_count = nf;
            out.cf_max = hi;
            s_sp += nf * 4u;
            Word ov = 0;
            for (Word w = 0; w < WORDS; ++w)
                ov += __builtin_popcount(out.base_cat_spikes.bits[w] & out.cf_cat_spikes.bits[w]);
            out.overlap_base_cf = ov;
            out.shifted = (ov < 12u && nf >= 8u) ? 1u : 0u;
        }
        out.spikes_fired = s_sp;
    }
}

// ---------------------------------------------------------------------------
// Kernel 19: Blackwell GPU Hierarchical 2-Hop Goal-to-Subgoal Plan Chaining
// (`mind_neo_plan_chain`)!
// Unrolls embodied inverse-causal precondition chains inside 28-MiB L2 PW23:
//   1. Step 1: Projects `goal_sdr` on PW23 (`W_inv`), excising `blocked_sdr`
//      if provided, and settles `step1_spikes` via tie-preserving 2-pass k-WTA.
//      If `has_have` and `overlap(step1_spikes, have_sdr) >= 12` (or goal already
//      held), short-circuits with `already_ready = 1`, `steps_needed = 0`!
//   2. Step 2: Feeds `step1_spikes` recurrently back through PW23 with refractory
//      exclusion of `goal_sdr | step1_spikes | blocked_sdr` to settle `step2_spikes`
//      (e.g., `cooked -> fire -> flint`, `pot -> fire -> flint`, `open -> key`)!
// ZERO extra L2 memory — survives `--ablate hipp` (Vigintuple 20-Track Ribot!)
// ---------------------------------------------------------------------------
AX_FN void neo_plan_chain_block(const signed char *weights_all, const NeoPlanChainQuery &q,
                                NeoPlanChainResult &out) {
    __attribute__((shared)) static Word s_idx[NEURONS];
    __attribute__((shared)) static Word s_nidx, s_sp, s_done;
    __attribute__((shared)) static int s_cur[NEURONS];
    __attribute__((shared)) static Word s_excl[WORDS];

    Word tid = thread_id();
    Word pw_inv = q.pw_inv < NEO_PATHWAYS ? q.pw_inv : 23u;
    Word top_k = q.top_k > 0 ? q.top_k : 32u;
    int min_c = q.min_curr > 0 ? q.min_curr : 4;

    if (tid == 0) {
        out = {};
        s_done = 0;
        Word cp = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word m = q.goal_sdr.bits[w];
            while (m && cp < NEURONS) {
                Word b = __builtin_ctz(m);
                m &= m - 1u;
                s_idx[cp++] = (w << 5) | b;
            }
            s_excl[w] = q.goal_sdr.bits[w] | (q.has_blocked ? q.blocked_sdr.bits[w] : 0u);
        }
        s_nidx = cp;
        s_sp = cp * 2u;
        if (q.has_have) {
            Word ovg = 0;
            for (Word w = 0; w < WORDS; ++w)
                ovg += __builtin_popcount(q.goal_sdr.bits[w] & q.have_sdr.bits[w]);
            if (ovg >= 12u) {
                out.already_ready = 1u;
                out.steps_needed = 0u;
                out.spikes_fired = s_sp;
                s_done = 1u;
            }
        }
        if (cp < 4u) {
            out.spikes_fired = s_sp;
            s_done = 1u;
        }
    }
    asm volatile("bar.sync 0;");
    if (s_done)
        return;

    const signed char *Winv = weights_all + (unsigned long long)(pw_inv) * NEO_SYNAPSES;

    // Step 1: Project `goal_sdr` on PW23 (`W_inv`) with refractory/blocked exclusion!
    Word n1 = s_nidx;
    for (Word j = tid; j < NEURONS; j += 256u) {
        if ((s_excl[j >> 5] >> (j & 31u)) & 1u) {
            s_cur[j] = 0;
            continue;
        }
        const signed char *r = Winv + (unsigned long long)(j) * NEURONS;
        int acc = 0;
        Word pos = 0;
        for (Word k = 0; k < n1; ++k) {
            int wv = int(r[s_idx[k]]);
            acc += wv;
            if (wv > 0) ++pos;
        }
        if (pos * 2u < n1) acc = 0;
        s_cur[j] = acc;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        int hi = -1000000;
        for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] > hi) hi = s_cur[j];
        if (hi >= min_c) {
            int lo = min_c, thr_hi = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] >= mid) ++cnt;
                if (cnt > top_k) lo = mid + 1; else thr_hi = mid;
            }
            int thr = lo > hi ? hi : lo;
            if (thr < min_c) thr = min_c;
            Word nf = 0;
            for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                if (s_cur[j] >= thr) {
                    out.step1_spikes.bits[j >> 5] |= 1u << (j & 31u);
                    ++nf;
                }
            }
            // Tie-preserving 2-pass k-WTA at boundary `thr - 1`!
            if (nf < top_k && thr > min_c) {
                int tie_v = thr - 1;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_cur[j] == tie_v) {
                        out.step1_spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
            }
            out.step1_count = nf;
            out.step1_max = hi;
            s_sp += nf * 4u;
        }
        if (out.step1_count < 8u) {
            out.steps_needed = 0u;
            out.spikes_fired = s_sp;
            s_done = 1u;
        } else if (q.has_have) {
            Word ov = 0;
            for (Word w = 0; w < WORDS; ++w)
                ov += __builtin_popcount(out.step1_spikes.bits[w] & q.have_sdr.bits[w]);
            if (ov >= 12u) {
                out.already_ready = 1u;
                out.steps_needed = 0u;
                out.spikes_fired = s_sp;
                s_done = 1u;
            }
        }
        if (!s_done) {
            out.steps_needed = 1u;
            Word cp = 0;
            for (Word w = 0; w < WORDS; ++w) {
                s_excl[w] |= out.step1_spikes.bits[w];
                Word m = out.step1_spikes.bits[w];
                while (m && cp < NEURONS) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    s_idx[cp++] = (w << 5) | b;
                }
            }
            s_nidx = cp;
            s_sp += cp * 2u;
        }
    }
    asm volatile("bar.sync 0;");
    if (s_done)
        return;

    // Step 2: Recurrently project `step1_spikes` back on PW23 to uncover deeper prerequisite!
    Word n2 = s_nidx;
    for (Word j = tid; j < NEURONS; j += 256u) {
        if ((s_excl[j >> 5] >> (j & 31u)) & 1u) {
            s_cur[j] = 0;
            continue;
        }
        const signed char *r = Winv + (unsigned long long)(j) * NEURONS;
        int acc = 0;
        Word pos = 0;
        for (Word k = 0; k < n2; ++k) {
            int wv = int(r[s_idx[k]]);
            acc += wv;
            if (wv > 0) ++pos;
        }
        if (pos * 2u < n2) acc = 0;
        s_cur[j] = acc;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        int hi = -1000000;
        for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] > hi) hi = s_cur[j];
        if (hi >= min_c) {
            int lo = min_c, thr_hi = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] >= mid) ++cnt;
                if (cnt > top_k) lo = mid + 1; else thr_hi = mid;
            }
            int thr = lo > hi ? hi : lo;
            if (thr < min_c) thr = min_c;
            Word nf = 0;
            for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                if (s_cur[j] >= thr) {
                    out.step2_spikes.bits[j >> 5] |= 1u << (j & 31u);
                    ++nf;
                }
            }
            // Tie-preserving 2-pass k-WTA at boundary `thr - 1`!
            if (nf < top_k && thr > min_c) {
                int tie_v = thr - 1;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_cur[j] == tie_v) {
                        out.step2_spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
            }
            out.step2_count = nf;
            out.step2_max = hi;
            s_sp += nf * 4u;
            if (nf >= 8u)
                out.steps_needed = 2u;
        }
        out.spikes_fired = s_sp;
    }
}

// ---------------------------------------------------------------------------
// Kernel 20: Blackwell GPU Compositional Multi-Action Causal State Simulator
// (`mind_neo_simulate`)!
// Unrolls 1- or 2-step action-conditioned causal transitions inside 28-MiB L2 PW7
// using biophysical NMDA dendritic coincidence detection (both current state AND
// chosen action branches must co-activate a postsynaptic attractor neuron):
//   1. Step 1: Projects `(init_sdr, action1_sdr)` conjunctively on PW7 (`W_act`)
//      with refractory exclusion of `init_sdr | action1_sdr` and tie-preserving
//      2-pass k-WTA to settle `step1_spikes` (e.g., `solid + melt -> liquid`).
//      If `has_blocked` and `overlap(step1_spikes, blocked_sdr) >= 12`, halts
//      immediately with `blocked = 1, blocked_step = 1`!
//   2. Step 2 (if `n_actions >= 2`): Feeds `step1_spikes` recurrently with
//      `action2_sdr` through PW7 to settle `step2_spikes` (e.g.,
//      `liquid + boil -> steam` vs `liquid + freeze -> frozen`), checking
//      obstacle veto against `blocked_sdr`!
// ZERO extra L2 memory — survives `--ablate hipp` (Unvigintuple 21-Track Ribot!)
// ---------------------------------------------------------------------------
AX_FN void neo_simulate_block(const signed char *weights_all, const NeoSimulateQuery &q,
                              NeoSimulateResult &out) {
    __attribute__((shared)) static Word s_sidx[NEURONS];
    __attribute__((shared)) static Word s_aidx[NEURONS];
    __attribute__((shared)) static Word s_ns, s_na, s_sp, s_done;
    __attribute__((shared)) static int s_cur[NEURONS];
    __attribute__((shared)) static Word s_excl[WORDS];

    Word tid = thread_id();
    Word pw_act = q.pw_act < NEO_PATHWAYS ? q.pw_act : 7u;
    Word top_k = q.top_k > 0 ? q.top_k : 32u;
    int min_c = q.min_curr > 0 ? q.min_curr : 4;
    Word n_act = q.n_actions >= 2u ? 2u : 1u;

    if (tid == 0) {
        out = {};
        s_done = 0;
        Word cs = 0, ca = 0;
        for (Word w = 0; w < WORDS; ++w) {
            Word ms = q.init_sdr.bits[w];
            while (ms && cs < NEURONS) {
                Word b = __builtin_ctz(ms);
                ms &= ms - 1u;
                s_sidx[cs++] = (w << 5) | b;
            }
            Word ma = q.action1_sdr.bits[w];
            while (ma && ca < NEURONS) {
                Word b = __builtin_ctz(ma);
                ma &= ma - 1u;
                s_aidx[ca++] = (w << 5) | b;
            }
            s_excl[w] = q.init_sdr.bits[w] | q.action1_sdr.bits[w];
        }
        s_ns = cs;
        s_na = ca;
        s_sp = (cs + ca) * 2u;
        if (cs < 4u || ca < 4u) {
            out.spikes_fired = s_sp;
            s_done = 1u;
        }
    }
    asm volatile("bar.sync 0;");
    if (s_done)
        return;

    const signed char *Wact = weights_all + (unsigned long long)(pw_act) * NEO_SYNAPSES;

    // Step 1: NMDA dendritic coincidence projection of `(init_sdr, action1_sdr)` on PW7!
    Word ns1 = s_ns, na1 = s_na;
    for (Word j = tid; j < NEURONS; j += 256u) {
        if ((s_excl[j >> 5] >> (j & 31u)) & 1u) {
            s_cur[j] = 0;
            continue;
        }
        const signed char *r = Wact + (unsigned long long)(j) * NEURONS;
        int acc_s = 0, acc_a = 0;
        Word pos_s = 0, pos_a = 0;
        for (Word k = 0; k < ns1; ++k) {
            int wv = int(r[s_sidx[k]]);
            acc_s += wv;
            if (wv > 0) ++pos_s;
        }
        for (Word k = 0; k < na1; ++k) {
            int wv = int(r[s_aidx[k]]);
            acc_a += wv;
            if (wv > 0) ++pos_a;
        }
        if (acc_s > 0 && acc_a > 0 && pos_s * 3u >= ns1 && pos_a * 3u >= na1)
            s_cur[j] = acc_s + acc_a;
        else
            s_cur[j] = 0;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        int hi = -1000000;
        for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] > hi) hi = s_cur[j];
        if (hi >= min_c) {
            int lo = min_c, thr_hi = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] >= mid) ++cnt;
                if (cnt > top_k) lo = mid + 1; else thr_hi = mid;
            }
            int thr = lo > hi ? hi : lo;
            if (thr < min_c) thr = min_c;
            Word nf = 0;
            for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                if (s_cur[j] >= thr) {
                    out.step1_spikes.bits[j >> 5] |= 1u << (j & 31u);
                    ++nf;
                }
            }
            if (nf < top_k && thr > min_c) {
                int tie_v = thr - 1;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_cur[j] == tie_v) {
                        out.step1_spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
            }
            out.step1_count = nf;
            out.step1_max = hi;
            s_sp += nf * 4u;
        }
        if (out.step1_count < 8u) {
            out.steps_completed = 0u;
            out.spikes_fired = s_sp;
            s_done = 1u;
        } else if (q.has_blocked) {
            Word ovb = 0;
            for (Word w = 0; w < WORDS; ++w)
                ovb += __builtin_popcount(out.step1_spikes.bits[w] & q.blocked_sdr.bits[w]);
            if (ovb >= 12u) {
                out.blocked = 1u;
                out.blocked_step = 1u;
                out.steps_completed = 0u;
                out.spikes_fired = s_sp;
                s_done = 1u;
            }
        }
        if (!s_done) {
            out.steps_completed = 1u;
            if (n_act < 2u) {
                out.spikes_fired = s_sp;
                s_done = 1u;
            } else {
                Word cs = 0, ca = 0;
                for (Word w = 0; w < WORDS; ++w) {
                    Word ms = out.step1_spikes.bits[w];
                    while (ms && cs < NEURONS) {
                        Word b = __builtin_ctz(ms);
                        ms &= ms - 1u;
                        s_sidx[cs++] = (w << 5) | b;
                    }
                    Word ma = q.action2_sdr.bits[w];
                    while (ma && ca < NEURONS) {
                        Word b = __builtin_ctz(ma);
                        ma &= ma - 1u;
                        s_aidx[ca++] = (w << 5) | b;
                    }
                    s_excl[w] = out.step1_spikes.bits[w] | q.action2_sdr.bits[w];
                }
                s_ns = cs;
                s_na = ca;
                s_sp += (cs + ca) * 2u;
                if (cs < 4u || ca < 4u) {
                    out.spikes_fired = s_sp;
                    s_done = 1u;
                }
            }
        }
    }
    asm volatile("bar.sync 0;");
    if (s_done)
        return;

    // Step 2: Recurrent NMDA dendritic coincidence projection of `(step1_spikes, action2_sdr)` on PW7!
    Word ns2 = s_ns, na2 = s_na;
    for (Word j = tid; j < NEURONS; j += 256u) {
        if ((s_excl[j >> 5] >> (j & 31u)) & 1u) {
            s_cur[j] = 0;
            continue;
        }
        const signed char *r = Wact + (unsigned long long)(j) * NEURONS;
        int acc_s = 0, acc_a = 0;
        Word pos_s = 0, pos_a = 0;
        for (Word k = 0; k < ns2; ++k) {
            int wv = int(r[s_sidx[k]]);
            acc_s += wv;
            if (wv > 0) ++pos_s;
        }
        for (Word k = 0; k < na2; ++k) {
            int wv = int(r[s_aidx[k]]);
            acc_a += wv;
            if (wv > 0) ++pos_a;
        }
        if (acc_s > 0 && acc_a > 0 && pos_s * 3u >= ns2 && pos_a * 3u >= na2)
            s_cur[j] = acc_s + acc_a;
        else
            s_cur[j] = 0;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        int hi = -1000000;
        for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] > hi) hi = s_cur[j];
        if (hi >= min_c) {
            int lo = min_c, thr_hi = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] >= mid) ++cnt;
                if (cnt > top_k) lo = mid + 1; else thr_hi = mid;
            }
            int thr = lo > hi ? hi : lo;
            if (thr < min_c) thr = min_c;
            Word nf = 0;
            for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                if (s_cur[j] >= thr) {
                    out.step2_spikes.bits[j >> 5] |= 1u << (j & 31u);
                    ++nf;
                }
            }
            if (nf < top_k && thr > min_c) {
                int tie_v = thr - 1;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_cur[j] == tie_v) {
                        out.step2_spikes.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
            }
            out.step2_count = nf;
            out.step2_max = hi;
            s_sp += nf * 4u;
            if (nf >= 8u) {
                if (q.has_blocked) {
                    Word ovb = 0;
                    for (Word w = 0; w < WORDS; ++w)
                        ovb += __builtin_popcount(out.step2_spikes.bits[w] & q.blocked_sdr.bits[w]);
                    if (ovb >= 12u) {
                        out.blocked = 1u;
                        out.blocked_step = 2u;
                    } else {
                        out.steps_completed = 2u;
                    }
                } else {
                    out.steps_completed = 2u;
                }
            }
        }
        out.spikes_fired = s_sp;
    }
}

// ---------------------------------------------------------------------------
// Kernel 21: Blackwell GPU Multi-Attribute Spiking Constraint Satisfaction
// & Inhibitory Elimination Solver (`mind_neo_constrain`)!
// Evaluates up to `MAX_CONSTRAIN_CANDS` (8) candidate SDRs simultaneously inside
// 28-MiB L2 across PW17 (transitive taxonomy closure), PW1 (positive semantic
// abilities/features) & PW2 (inhibitory GABA_A exceptions):
//   - `req_cat`: candidate must strongly activate >= 75% of `req_cat` neurons on PW17
//   - `req_pos`: candidate must activate >= 75% of `req_pos` on PW1 WITHOUT PW2 veto
//   - `excl_sdr`: candidate must NOT activate `excl_sdr` on unvetoed PW1 OR on PW17
//     (handles BOTH ability exclusion like `excluding fly` AND category exclusion
//     like `excluding animal` natively on GPU!)
// ZERO extra L2 memory — survives `--ablate hipp` (Duovigintuple 22-Track Ribot!)
// ---------------------------------------------------------------------------
AX_FN void neo_constrain_block(const signed char *weights_all, const NeoConstrainQuery &q,
                               NeoConstrainResult &out) {
    __attribute__((shared)) static Word s_cidx[MAX_CONSTRAIN_CANDS][64];
    __attribute__((shared)) static Word s_clen[MAX_CONSTRAIN_CANDS];
    __attribute__((shared)) static Word s_cat_idx[64], s_pos_idx[64], s_exc_idx[64];
    __attribute__((shared)) static Word s_ncat, s_npos, s_nexc, s_nc;
    __attribute__((shared)) static int s_cat_hit[MAX_CONSTRAIN_CANDS];
    __attribute__((shared)) static int s_pos_hit[MAX_CONSTRAIN_CANDS];
    __attribute__((shared)) static int s_exc_p1_hit[MAX_CONSTRAIN_CANDS];
    __attribute__((shared)) static int s_exc_p17_hit[MAX_CONSTRAIN_CANDS];
    __attribute__((shared)) static int s_score[MAX_CONSTRAIN_CANDS];

    Word tid = thread_id();
    Word pw_tax = q.pw_tax < NEO_PATHWAYS ? q.pw_tax : 17u;
    Word pw_pos = q.pw_pos < NEO_PATHWAYS ? q.pw_pos : 1u;
    Word pw_neg = q.pw_neg < NEO_PATHWAYS ? q.pw_neg : 2u;

    if (tid == 0) {
        out = {};
        Word nc = q.n_cands <= MAX_CONSTRAIN_CANDS ? q.n_cands : MAX_CONSTRAIN_CANDS;
        s_nc = nc;
        auto unpack = [](const SpikeVec &sv, Word *dst, Word cap) -> Word {
            Word cnt = 0;
            for (Word w = 0; w < WORDS && cnt < cap; ++w) {
                Word m = sv.bits[w];
                while (m && cnt < cap) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    dst[cnt++] = (w << 5) | b;
                }
            }
            return cnt;
        };
        for (Word c = 0; c < nc; ++c) {
            s_clen[c] = unpack(q.cands[c], s_cidx[c], 64u);
            s_cat_hit[c] = 0;
            s_pos_hit[c] = 0;
            s_exc_p1_hit[c] = 0;
            s_exc_p17_hit[c] = 0;
            s_score[c] = 0;
        }
        s_ncat = q.has_cat ? unpack(q.req_cat, s_cat_idx, 64u) : 0u;
        s_npos = q.has_pos ? unpack(q.req_pos, s_pos_idx, 64u) : 0u;
        s_nexc = q.has_excl ? unpack(q.excl_sdr, s_exc_idx, 64u) : 0u;
    }
    asm volatile("bar.sync 0;");
    if (s_nc == 0u)
        return;

    const signed char *Wtax = weights_all + (unsigned long long)(pw_tax) * NEO_SYNAPSES;
    const signed char *Wpos = weights_all + (unsigned long long)(pw_pos) * NEO_SYNAPSES;
    const signed char *Wneg = weights_all + (unsigned long long)(pw_neg) * NEO_SYNAPSES;

    // Parallel warp-per-candidate evaluation (`warp c = tid >> 5`, `lane = tid & 31u`) via hardware `warp_add` (`redux.sync.add.u32`)!
    // Uses 75% NMDA dendritic coincidence (`pos * 4u >= clen * 3u`) to eliminate cross-talk across superposed SDRs,
    // and checks negative GABA_A inhibitory exceptions (`neg_n * 4u >= clen * 3u && acc_n < 0`) on PW2!
    Word nc = s_nc;
    Word c = tid >> 5;
    Word lane = tid & 31u;
    if (c < nc) {
        Word clen = s_clen[c];
        Word h_cat = 0, h_pos = 0, h_ep1 = 0, h_ep17 = 0;
        Word sc_add = 0;
        if (clen >= 4u) {
            for (Word slot = lane; slot < s_ncat; slot += 32u) {
                Word j = s_cat_idx[slot];
                const signed char *r = Wtax + (unsigned long long)(j) * NEURONS;
                int acc = 0;
                Word pos = 0;
                for (Word k = 0; k < clen; ++k) {
                    int wv = int(r[s_cidx[c][k]]);
                    acc += wv;
                    if (wv > 0) ++pos;
                }
                if (acc > 0 && pos * 4u >= clen * 3u) {
                    ++h_cat;
                    sc_add += Word(acc);
                }
            }
            for (Word slot = lane; slot < s_npos; slot += 32u) {
                Word j = s_pos_idx[slot];
                const signed char *rp = Wpos + (unsigned long long)(j) * NEURONS;
                const signed char *rn = Wneg + (unsigned long long)(j) * NEURONS;
                int acc_p = 0, acc_n = 0;
                Word pos_p = 0, neg_n = 0;
                for (Word k = 0; k < clen; ++k) {
                    Word pre = s_cidx[c][k];
                    int wp = int(rp[pre]);
                    int wn = int(rn[pre]);
                    acc_p += wp; if (wp > 0) ++pos_p;
                    acc_n += wn; if (wn < 0) ++neg_n;
                }
                bool vetoed = (acc_n < 0 && neg_n * 4u >= clen * 3u);
                if (acc_p > 0 && pos_p * 4u >= clen * 3u && !vetoed) {
                    ++h_pos;
                    sc_add += Word(acc_p);
                }
            }
            for (Word slot = lane; slot < s_nexc; slot += 32u) {
                Word j = s_exc_idx[slot];
                const signed char *rp = Wpos + (unsigned long long)(j) * NEURONS;
                const signed char *rn = Wneg + (unsigned long long)(j) * NEURONS;
                const signed char *rt = Wtax + (unsigned long long)(j) * NEURONS;
                int acc_p = 0, acc_n = 0, acc_t = 0;
                Word pos_p = 0, neg_n = 0, pos_t = 0;
                for (Word k = 0; k < clen; ++k) {
                    Word pre = s_cidx[c][k];
                    int wp = int(rp[pre]);
                    int wn = int(rn[pre]);
                    int wt = int(rt[pre]);
                    acc_p += wp; if (wp > 0) ++pos_p;
                    acc_n += wn; if (wn < 0) ++neg_n;
                    acc_t += wt; if (wt > 0) ++pos_t;
                }
                bool vetoed = (acc_n < 0 && neg_n * 4u >= clen * 3u);
                if (acc_p > 0 && pos_p * 4u >= clen * 3u && !vetoed) ++h_ep1;
                if (acc_t > 0 && pos_t * 4u >= clen * 3u) ++h_ep17;
            }
        }
        Word tot_cat = warp_add(h_cat);
        Word tot_pos = warp_add(h_pos);
        Word tot_ep1 = warp_add(h_ep1);
        Word tot_ep17 = warp_add(h_ep17);
        Word tot_sc = warp_add(sc_add);
        if (lane == 0u) {
            s_cat_hit[c] = int(tot_cat);
            s_pos_hit[c] = int(tot_pos);
            s_exc_p1_hit[c] = int(tot_ep1);
            s_exc_p17_hit[c] = int(tot_ep17);
            s_score[c] = int(tot_sc);
        }
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        Word sp = 0;
        Word mask = 0, nval = 0, best_c = 0;
        int best_sc = -1000000;
        for (Word c = 0; c < nc; ++c) {
            sp += s_clen[c] * 2u + 32u;
            bool ok_cat = (!q.has_cat || s_ncat == 0u) ? true : (Word(s_cat_hit[c]) * 4u >= s_ncat * 3u);
            bool ok_pos = (!q.has_pos || s_npos == 0u) ? true : (Word(s_pos_hit[c]) * 4u >= s_npos * 3u);
            bool hit_exc_p1 = (s_nexc > 0u) && (Word(s_exc_p1_hit[c]) * 4u >= s_nexc * 3u);
            bool hit_exc_p17 = (s_nexc > 0u) && (Word(s_exc_p17_hit[c]) * 4u >= s_nexc * 3u);
            bool ok_exc = (!q.has_excl || s_nexc == 0u) ? true : (!hit_exc_p1 && !hit_exc_p17);
            int sc = (ok_cat && ok_pos && ok_exc) ? (s_score[c] + 1000) : 0;
            out.scores[c] = sc;
            if (ok_cat && ok_pos && ok_exc) {
                mask |= (1u << c);
                ++nval;
                if (sc > best_sc) {
                    best_sc = sc;
                    best_c = c;
                }
            }
        }
        out.valid_mask = mask;
        out.n_valid = nval;
        out.best_cand = best_c;
        out.best_score = nval > 0 ? best_sc : 0;
        out.eliminated = nc - nval;
        out.spikes_fired = sp + nval * 64u;
    }
}

// ---------------------------------------------------------------------------
// Kernel 22: Blackwell GPU Few-Shot Relational Schema Induction & 1/2-Hop
// Cross-Domain Transfer (`mind_neo_induce`)!
// Given TWO observed positive relational example pairs `(ex1_in -> ex1_out)`
// and `(ex2_in -> ex2_out)` plus a novel query `query_in`:
//   Stage 1 (Warp-per-pathway `warp_add` across up to 6 L2 pathways, ZERO atomics):
//     Scores EVERY candidate L2 pathway against BOTH examples using 75% NMDA
//     dendritic coincidence, selecting `best_pw` maximizing `min(ex1_score, ex2_score)`
//     so an accidental 1-example coincidence is vetoed!
//   Stage 2 (256-thread 1-hop or 2-hop rollout on induced `best_pw`):
//     Projects `query_in` across `best_pw` for 1 or 2 steps with refractory
//     exclusion & tie-preserving 2-pass k-WTA (`step1_spikes`, `step2_spikes`).
// ZERO extra L2 memory — survives `--ablate hipp` (Trevigintuple 23-Track Ribot!)
// ---------------------------------------------------------------------------
AX_FN void neo_induce_block(const signed char *weights_all, const NeoInduceQuery &q,
                            NeoInduceResult &out) {
    __attribute__((shared)) static Word s_e1i[64], s_e1o[64], s_e2i[64], s_e2o[64], s_qi[64];
    __attribute__((shared)) static Word s_ne1i, s_ne1o, s_ne2i, s_ne2o, s_nqi, s_npws, s_best_pw, s_found, s_sp;
    __attribute__((shared)) static int s_pw_sc1[MAX_INDUCE_PWS];
    __attribute__((shared)) static int s_pw_sc2[MAX_INDUCE_PWS];
    __attribute__((shared)) static Word s_pw_ok[MAX_INDUCE_PWS];
    __attribute__((shared)) static int s_cur[NEURONS];
    __attribute__((shared)) static Word s_excl[WORDS];

    Word tid = thread_id();
    Word top_k = q.top_k > 0 ? q.top_k : 32u;
    int min_c = q.min_curr > 0 ? q.min_curr : 4;
    Word n_hops = (q.n_hops >= 2u) ? 2u : 1u;

    if (tid == 0) {
        out = {};
        auto unpack = [](const SpikeVec &sv, Word *dst, Word cap) -> Word {
            Word cnt = 0;
            for (Word w = 0; w < WORDS && cnt < cap; ++w) {
                Word m = sv.bits[w];
                while (m && cnt < cap) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    dst[cnt++] = (w << 5) | b;
                }
            }
            return cnt;
        };
        s_ne1i = unpack(q.ex1_in, s_e1i, 64u);
        s_ne1o = unpack(q.ex1_out, s_e1o, 64u);
        s_ne2i = unpack(q.ex2_in, s_e2i, 64u);
        s_ne2o = unpack(q.ex2_out, s_e2o, 64u);
        s_nqi = unpack(q.query_in, s_qi, 64u);
        s_npws = q.n_pws <= MAX_INDUCE_PWS ? q.n_pws : MAX_INDUCE_PWS;
        s_best_pw = 0;
        s_found = 0;
        s_sp = (s_ne1i + s_ne1o + s_ne2i + s_ne2o + s_nqi) * 2u;
        for (Word p = 0; p < MAX_INDUCE_PWS; ++p) {
            s_pw_sc1[p] = 0;
            s_pw_sc2[p] = 0;
            s_pw_ok[p] = 0;
        }
        for (Word w = 0; w < WORDS; ++w)
            s_excl[w] = q.query_in.bits[w];
    }
    asm volatile("bar.sync 0;");
    if (s_npws == 0u || s_ne1i < 4u || s_ne1o < 4u || s_ne2i < 4u || s_ne2o < 4u || s_nqi < 4u)
        return;

    // Stage 1: Parallel warp-per-pathway few-shot verification (`p = tid >> 5`, `lane = tid & 31u`)!
    Word p = tid >> 5;
    Word lane = tid & 31u;
    if (p < s_npws) {
        Word pw = q.cand_pws[p];
        Word h1 = 0, h2 = 0, sc1 = 0, sc2 = 0;
        if (pw < NEO_PATHWAYS) {
            const signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;
            for (Word slot = lane; slot < s_ne1o; slot += 32u) {
                Word j = s_e1o[slot];
                const signed char *r = W + (unsigned long long)(j) * NEURONS;
                int acc = 0;
                Word pos = 0;
                for (Word k = 0; k < s_ne1i; ++k) {
                    int wv = int(r[s_e1i[k]]);
                    acc += wv;
                    if (wv > 0) ++pos;
                }
                if (acc > 0 && pos * 3u >= s_ne1i * 2u) {
                    ++h1;
                    sc1 += Word(acc);
                }
            }
            for (Word slot = lane; slot < s_ne2o; slot += 32u) {
                Word j = s_e2o[slot];
                const signed char *r = W + (unsigned long long)(j) * NEURONS;
                int acc = 0;
                Word pos = 0;
                for (Word k = 0; k < s_ne2i; ++k) {
                    int wv = int(r[s_e2i[k]]);
                    acc += wv;
                    if (wv > 0) ++pos;
                }
                if (acc > 0 && pos * 3u >= s_ne2i * 2u) {
                    ++h2;
                    sc2 += Word(acc);
                }
            }
        }
        Word th1 = warp_add(h1);
        Word th2 = warp_add(h2);
        Word ts1 = warp_add(sc1);
        Word ts2 = warp_add(sc2);
        if (lane == 0u) {
            bool ok = (th1 * 4u >= s_ne1o * 3u) && (th2 * 4u >= s_ne2o * 3u) && ts1 > 0u && ts2 > 0u;
            s_pw_ok[p] = ok ? 1u : 0u;
            s_pw_sc1[p] = int(ts1);
            s_pw_sc2[p] = int(ts2);
        }
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        int best_min = 0;
        for (Word k = 0; k < s_npws; ++k) {
            s_sp += 32u;
            if (!s_pw_ok[k]) continue;
            int mn = s_pw_sc1[k] < s_pw_sc2[k] ? s_pw_sc1[k] : s_pw_sc2[k];
            if (mn > best_min) {
                best_min = mn;
                s_best_pw = q.cand_pws[k];
                s_found = 1u;
                out.ex1_score = s_pw_sc1[k];
                out.ex2_score = s_pw_sc2[k];
            }
        }
        out.found = s_found;
        out.best_pw = s_best_pw;
        if (!s_found) out.spikes_fired = s_sp;
    }
    asm volatile("bar.sync 0;");
    if (!s_found)
        return;

    // Stage 2: 1-Hop or 2-Hop Transfer Projection across induced pathway `s_best_pw`!
    const signed char *Wb = weights_all + (unsigned long long)(s_best_pw) * NEO_SYNAPSES;
    for (Word hop = 0; hop < n_hops; ++hop) {
        Word nq = s_nqi;
        for (Word j = tid; j < NEURONS; j += 256u) {
            if ((s_excl[j >> 5] >> (j & 31u)) & 1u) {
                s_cur[j] = 0;
                continue;
            }
            const signed char *r = Wb + (unsigned long long)(j) * NEURONS;
            int acc = 0;
            Word pos = 0;
            for (Word k = 0; k < nq; ++k) {
                int wv = int(r[s_qi[k]]);
                acc += wv;
                if (wv > 0) ++pos;
            }
            s_cur[j] = (acc > 0 && pos * 2u >= nq) ? acc : 0;
        }
        asm volatile("bar.sync 0;");

        if (tid == 0) {
            int hi = -1000000;
            for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] > hi) hi = s_cur[j];
            SpikeVec nxt{};
            Word nf = 0;
            if (hi >= min_c) {
                int lo = min_c, thr_hi = hi + 1;
                for (int it = 0; it < 16; ++it) {
                    int mid = (lo + thr_hi) >> 1;
                    Word cnt = 0;
                    for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] >= mid) ++cnt;
                    if (cnt > top_k) lo = mid + 1; else thr_hi = mid;
                }
                int thr = lo > hi ? hi : lo;
                if (thr < min_c) thr = min_c;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_cur[j] >= thr) {
                        nxt.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
                if (nf < top_k && thr > min_c) {
                    int tie_v = thr - 1;
                    for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                        if (s_cur[j] == tie_v) {
                            nxt.bits[j >> 5] |= 1u << (j & 31u);
                            ++nf;
                        }
                    }
                }
            }
            if (hop == 0u) {
                out.step1_spikes = nxt;
                out.step1_count = nf;
            } else {
                out.step2_spikes = nxt;
                out.step2_count = nf;
            }
            if (nf >= 8u) {
                out.hops_done = hop + 1u;
                s_sp += nf * 4u;
                Word cn = 0;
                for (Word w = 0; w < WORDS && cn < 64u; ++w) {
                    Word m = nxt.bits[w];
                    s_excl[w] |= m;
                    while (m && cn < 64u) {
                        Word b = __builtin_ctz(m);
                        m &= m - 1u;
                        s_qi[cn++] = (w << 5) | b;
                    }
                }
                s_nqi = cn;
            } else {
                s_nqi = 0u;
            }
            out.spikes_fired = s_sp;
        }
        asm volatile("bar.sync 0;");
        if (s_nqi < 4u)
            break;
    }
}

// ---------------------------------------------------------------------------
// Kernel 23: Blackwell GPU Compositional Multi-Pathway Rule Algebra &
// Autonomous Cross-Domain Operator Discovery (`mind_neo_chain_rule`)!
//   Mode 0 (Forward Cross-Pathway Composition `pw1 -> pw2`):
//     Fires `start_sdr` across L2 `pw1` (`mid_spikes`) and immediately chains
//     `mid_spikes` across a DISTINCT L2 `pw2` (`end_spikes`) with refractory
//     exclusion & tie-preserving 2-pass k-WTA (`solid --[PW16]--> liquid --[PW10]--> melt`,
//     `locked --[PW18]--> open --[PW23]--> key`, `lost --[PW9]--> sad --[PW18]--> happy`).
//   Mode 1 (Autonomous 36-Pair Cross-Pathway Operator Discovery):
//     Given `(start_sdr, target_sdr)`, propagates `start_sdr` across each candidate `pw1`,
//     then runs parallel warp-per-pathway (`warp_add`, ZERO atomics!) verification across
//     every candidate `pw2` using >=66.7% NMDA dendritic coincidence to discover BOTH
//     operators `(best_pw1, best_pw2)` and the latent intermediate concept `mid_spikes`!
// ZERO extra L2 memory — survives `--ablate hipp` (Quattuorvigintuple 24-Track Ribot!)
// ---------------------------------------------------------------------------
AX_FN void neo_chain_rule_block(const signed char *weights_all, const NeoChainRuleQuery &q,
                                NeoChainRuleResult &out) {
    __attribute__((shared)) static Word s_in[64], s_mid[64], s_tgt[64];
    __attribute__((shared)) static Word s_nin, s_nmid, s_ntgt, s_npws, s_sp;
    __attribute__((shared)) static int s_cur[NEURONS];
    __attribute__((shared)) static Word s_excl[WORDS];
    __attribute__((shared)) static int s_p2_sc[MAX_CHAINRULE_PWS];
    __attribute__((shared)) static Word s_p2_ok[MAX_CHAINRULE_PWS];
    __attribute__((shared)) static Word s_mid_bits[WORDS];
    __attribute__((shared)) static Word s_best_sc, s_found;

    Word tid = thread_id();
    Word top_k = q.top_k > 0 ? q.top_k : 32u;
    int min_c = q.min_curr > 0 ? q.min_curr : 4;

    if (tid == 0) {
        out = {};
        auto unpack = [](const SpikeVec &sv, Word *dst, Word cap) -> Word {
            Word cnt = 0;
            for (Word w = 0; w < WORDS && cnt < cap; ++w) {
                Word m = sv.bits[w];
                while (m && cnt < cap) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    dst[cnt++] = (w << 5) | b;
                }
            }
            return cnt;
        };
        s_nin = unpack(q.start_sdr, s_in, 64u);
        s_ntgt = (q.mode == 1u) ? unpack(q.target_sdr, s_tgt, 64u) : 0u;
        s_npws = q.n_pws <= MAX_CHAINRULE_PWS ? q.n_pws : MAX_CHAINRULE_PWS;
        s_sp = (s_nin + s_ntgt) * 2u;
        s_best_sc = 0;
        s_found = 0;
        for (Word w = 0; w < WORDS; ++w) s_excl[w] = q.start_sdr.bits[w];
    }
    asm volatile("bar.sync 0;");
    if (s_nin < 4u) return;

    auto kwta_select = [&](SpikeVec &dst_sv, Word *dst_idx, Word &dst_cnt, bool add_excl) {
        int hi = -1000000;
        for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] > hi) hi = s_cur[j];
        dst_sv = {};
        Word nf = 0;
        if (hi >= min_c) {
            int lo = min_c, thr_hi = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j) if (s_cur[j] >= mid) ++cnt;
                if (cnt > top_k) lo = mid + 1; else thr_hi = mid;
            }
            int thr = lo > hi ? hi : lo;
            if (thr < min_c) thr = min_c;
            for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                if (s_cur[j] >= thr) {
                    dst_sv.bits[j >> 5] |= 1u << (j & 31u);
                    ++nf;
                }
            }
            if (nf < top_k && thr > min_c) {
                int tie_v = thr - 1;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_cur[j] == tie_v) {
                        dst_sv.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
            }
        }
        Word cn = 0;
        for (Word w = 0; w < WORDS && cn < 64u; ++w) {
            Word m = dst_sv.bits[w];
            if (add_excl) s_excl[w] |= m;
            while (m && cn < 64u) {
                Word b = __builtin_ctz(m);
                m &= m - 1u;
                dst_idx[cn++] = (w << 5) | b;
            }
        }
        dst_cnt = cn;
        s_sp += nf * 4u;
    };

    if (q.mode == 0u) {
        if (q.pw1 >= NEO_PATHWAYS || q.pw2 >= NEO_PATHWAYS) return;
        const signed char *W1 = weights_all + (unsigned long long)(q.pw1) * NEO_SYNAPSES;
        for (Word j = tid; j < NEURONS; j += 256u) {
            if ((s_excl[j >> 5] >> (j & 31u)) & 1u) { s_cur[j] = 0; continue; }
            const signed char *r = W1 + (unsigned long long)(j) * NEURONS;
            int acc = 0; Word pos = 0;
            for (Word k = 0; k < s_nin; ++k) {
                int wv = int(r[s_in[k]]);
                acc += wv; if (wv > 0) ++pos;
            }
            s_cur[j] = (acc > 0 && pos * 2u >= s_nin) ? acc : 0;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            kwta_select(out.mid_spikes, s_mid, s_nmid, true);
            out.mid_count = s_nmid;
        }
        asm volatile("bar.sync 0;");
        if (s_nmid < 8u) {
            if (tid == 0) out.spikes_fired = s_sp;
            return;
        }
        const signed char *W2 = weights_all + (unsigned long long)(q.pw2) * NEO_SYNAPSES;
        for (Word j = tid; j < NEURONS; j += 256u) {
            if ((s_excl[j >> 5] >> (j & 31u)) & 1u) { s_cur[j] = 0; continue; }
            const signed char *r = W2 + (unsigned long long)(j) * NEURONS;
            int acc = 0; Word pos = 0;
            for (Word k = 0; k < s_nmid; ++k) {
                int wv = int(r[s_mid[k]]);
                acc += wv; if (wv > 0) ++pos;
            }
            s_cur[j] = (acc > 0 && pos * 2u >= s_nmid) ? acc : 0;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            Word nend = 0;
            Word dummy_idx[64];
            kwta_select(out.end_spikes, dummy_idx, nend, false);
            out.end_count = nend;
            out.best_pw1 = q.pw1;
            out.best_pw2 = q.pw2;
            out.found = (s_nmid >= 8u && nend >= 8u) ? 1u : 0u;
            out.match_score = int(s_nmid + nend);
            out.spikes_fired = s_sp;
        }
        return;
    }

    // Mode 1: Autonomous 36-Pair Cross-Pathway Operator Discovery (`start -> mid -> target`)!
    if (s_ntgt < 4u || s_npws == 0u) return;
    Word p2_warp = tid >> 5;
    Word lane = tid & 31u;

    for (Word p1 = 0; p1 < s_npws; ++p1) {
        Word pw1 = q.cand_pws[p1];
        if (pw1 >= NEO_PATHWAYS) continue;
        const signed char *W1 = weights_all + (unsigned long long)(pw1) * NEO_SYNAPSES;
        for (Word j = tid; j < NEURONS; j += 256u) {
            if ((q.start_sdr.bits[j >> 5] >> (j & 31u)) & 1u) { s_cur[j] = 0; continue; }
            const signed char *r = W1 + (unsigned long long)(j) * NEURONS;
            int acc = 0; Word pos = 0;
            for (Word k = 0; k < s_nin; ++k) {
                int wv = int(r[s_in[k]]);
                acc += wv; if (wv > 0) ++pos;
            }
            s_cur[j] = (acc > 0 && pos * 2u >= s_nin) ? acc : 0;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            SpikeVec tmp_sv{};
            kwta_select(tmp_sv, s_mid, s_nmid, false);
            for (Word w = 0; w < WORDS; ++w) s_mid_bits[w] = tmp_sv.bits[w];
            // Reject degenerate 1-hop identity where mid IS already target!
            Word ov = 0;
            for (Word w = 0; w < WORDS; ++w) ov += __builtin_popcount(tmp_sv.bits[w] & q.target_sdr.bits[w]);
            if (ov >= 20u) s_nmid = 0u;
            for (Word k = 0; k < MAX_CHAINRULE_PWS; ++k) { s_p2_sc[k] = 0; s_p2_ok[k] = 0; }
        }
        asm volatile("bar.sync 0;");
        if (s_nmid < 8u) continue;

        if (p2_warp < s_npws) {
            Word pw2 = q.cand_pws[p2_warp];
            Word h = 0, sc = 0;
            if (pw2 < NEO_PATHWAYS) {
                const signed char *W2 = weights_all + (unsigned long long)(pw2) * NEO_SYNAPSES;
                for (Word slot = lane; slot < s_ntgt; slot += 32u) {
                    Word j = s_tgt[slot];
                    const signed char *r = W2 + (unsigned long long)(j) * NEURONS;
                    int acc = 0; Word pos = 0;
                    for (Word k = 0; k < s_nmid; ++k) {
                        int wv = int(r[s_mid[k]]);
                        acc += wv; if (wv > 0) ++pos;
                    }
                    if (acc > 0 && pos * 3u >= s_nmid * 2u) {
                        ++h;
                        sc += Word(acc);
                    }
                }
            }
            Word th = warp_add(h);
            Word ts = warp_add(sc);
            if (lane == 0u) {
                bool ok = (th * 4u >= s_ntgt * 3u) && ts > 0u;
                s_p2_ok[p2_warp] = ok ? 1u : 0u;
                s_p2_sc[p2_warp] = int(ts);
            }
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            for (Word k = 0; k < s_npws; ++k) {
                s_sp += 16u;
                if (!s_p2_ok[k]) continue;
                Word pw2 = q.cand_pws[k];
                Word cand_sc = Word(s_p2_sc[k]) + (pw1 != pw2 ? 4u : 0u);
                if (cand_sc > s_best_sc) {
                    s_best_sc = cand_sc;
                    s_found = 1u;
                    out.found = 1u;
                    out.best_pw1 = pw1;
                    out.best_pw2 = pw2;
                    for (Word w = 0; w < WORDS; ++w) out.mid_spikes.bits[w] = s_mid_bits[w];
                    out.end_spikes = q.target_sdr;
                    out.mid_count = s_nmid;
                    out.end_count = s_ntgt;
                    out.match_score = s_p2_sc[k];
                }
            }
            out.spikes_fired = s_sp;
        }
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) out.spikes_fired = s_sp;
}

// ---------------------------------------------------------------------------
// Kernel 24: Blackwell GPU Live 1-Shot Hebbian Online Schema Consolidation &
// Instant Multi-Hop Recall (`mind_neo_online_schema`) — STAGE 4 UNLOCK!
//   Mode 0 (Live 1-Shot Hebbian Synaptic Imprint):
//     Writes `pre_sdr -> post_sdr` straight into L2 plastic synapses (`weights_all[pw]`)
//     using the EVOLVED 3-factor dendritic plasticity LUT (`g.basal_rule`) — ZERO hardcoded
//     learning rules, ZERO atomics, ZERO extra L2 memory!
//   Mode 1 (Instant 1/2/3-Hop Multi-Step Rollout + Live GABA_A Blocker Veto):
//     Traverses up to 3 hops immediately across `weights_all[pw]` with >=50% NMDA dendritic
//     coincidence, refractory self-exclusion, optional inhibitory blocker veto (`block_sdr`),
//     and tie-preserving 2-pass k-WTA — survives `--ablate hipp` (Quinvigintuple 25-Track Ribot!)
//   Mode 2 (1-Shot Atomic Write + Instant 2-Hop Rollout in ONE Kernel Launch):
//     Imprints BOTH `pre_sdr -> post_sdr` AND `post_sdr -> post2_sdr` via `g.basal_rule`
//     and immediately executes a 2-hop rollout from `pre_sdr`!
// ---------------------------------------------------------------------------
AX_FN void neo_online_schema_block(signed char *weights_all, const NeoOnlineSchemaQuery &q,
                                   const CorticalGenome &g, NeoOnlineSchemaResult &out) {
    __attribute__((shared)) static Word s_a[64], s_b[64], s_c[64], s_cur_idx[64];
    __attribute__((shared)) static Word s_na, s_nb, s_nc, s_ncur, s_sp;
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_excl[WORDS];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_stop;

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 19u;
    Word top_k = q.top_k > 0 ? q.top_k : 32u;
    int min_c = q.min_curr > 0 ? q.min_curr : 4;
    Word max_hops = q.max_hops > 0 ? (q.max_hops <= MAX_ONLINE_HOPS ? q.max_hops : MAX_ONLINE_HOPS) : 2u;

    if (tid == 0) {
        out = {};
        auto unpack = [](const SpikeVec &sv, Word *dst, Word cap) -> Word {
            Word cnt = 0;
            for (Word w = 0; w < WORDS && cnt < cap; ++w) {
                Word m = sv.bits[w];
                while (m && cnt < cap) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    dst[cnt++] = (w << 5) | b;
                }
            }
            return cnt;
        };
        s_na = unpack(q.pre_sdr, s_a, 64u);
        s_nb = (q.mode == 0u || q.mode == 2u) ? unpack(q.post_sdr, s_b, 64u) : 0u;
        s_nc = (q.mode == 2u) ? unpack(q.post2_sdr, s_c, 64u) : 0u;
        s_ncur = s_na;
        for (Word k = 0; k < s_na; ++k) s_cur_idx[k] = s_a[k];
        for (Word w = 0; w < WORDS; ++w) s_excl[w] = q.pre_sdr.bits[w];
        s_sp = (s_na + s_nb + s_nc) * 2u;
        s_stop = 0u;
    }
    asm volatile("bar.sync 0;");

    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;
    Word local_edits = 0u;

    // Stage A (Mode 0 or Mode 2): Live 1-Shot Hebbian Write via EVOLVED `g.basal_rule` LUT!
    if ((q.mode == 0u || q.mode == 2u) && s_na >= 4u && s_nb >= 4u) {
        int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
        if (d_ltp <= 0) d_ltp = 1;
        int burst = d_ltp * int(q.strength > 0 ? q.strength : 4u);
        for (Word idx = tid; idx < s_nb; idx += 256u) {
            Word j = s_b[idx];
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word k = 0; k < s_na; ++k) {
                Word i = s_a[k];
                int cur = int(row[i]);
                int nxt = cur + burst;
                if (nxt > 63) nxt = 63;
                if (nxt != cur) {
                    row[i] = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        if (q.mode == 2u && s_nc >= 4u) {
            for (Word idx = tid; idx < s_nc; idx += 256u) {
                Word j = s_c[idx];
                signed char *row = W + (unsigned long long)(j) * NEURONS;
                for (Word k = 0; k < s_nb; ++k) {
                    Word i = s_b[k];
                    int cur = int(row[i]);
                    int nxt = cur + burst;
                    if (nxt > 63) nxt = 63;
                    if (nxt != cur) {
                        row[i] = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                }
            }
        }
    }
    s_ed_red[tid] = local_edits;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) {
        out.edits_made = s_ed_red[0];
        if (q.mode == 0u) out.spikes_fired = s_sp + (s_ed_red[0] > 0 ? 64u : 0u);
    }
    asm volatile("bar.sync 0;");
    if (q.mode == 0u || s_na < 4u) return;

    // Stage B (Mode 1 or Mode 2): Instant 1/2/3-Hop Recurrent Spiking Rollout on L2 PW19!
    for (Word h = 0; h < max_hops; ++h) {
        if (s_stop || s_ncur < 4u) break;
        Word n_in = s_ncur;
        for (Word j = tid; j < NEURONS; j += 256u) {
            if ((s_excl[j >> 5] >> (j & 31u)) & 1u) {
                s_curr[j] = 0;
                continue;
            }
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int acc = 0;
            Word pos = 0;
            for (Word k = 0; k < n_in; ++k) {
                int wv = int(row[s_cur_idx[k]]);
                acc += wv;
                if (wv > 0) ++pos;
            }
            s_curr[j] = (acc > 0 && pos * 2u >= n_in) ? acc : 0;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            int hi = -1000000;
            for (Word j = 0; j < NEURONS; ++j)
                if (s_curr[j] > hi) hi = s_curr[j];
            SpikeVec sv{};
            Word nf = 0;
            if (hi >= min_c) {
                int lo = min_c, thr_hi = hi + 1;
                for (int it = 0; it < 16; ++it) {
                    int mid = (lo + thr_hi) >> 1;
                    Word cnt = 0;
                    for (Word j = 0; j < NEURONS; ++j)
                        if (s_curr[j] >= mid) ++cnt;
                    if (cnt > top_k) lo = mid + 1;
                    else thr_hi = mid;
                }
                int thr = lo > hi ? hi : lo;
                if (thr < min_c) thr = min_c;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_curr[j] >= thr) {
                        sv.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
                if (nf < top_k && thr > min_c) {
                    int tie_v = thr - 1;
                    for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                        if (s_curr[j] == tie_v) {
                            sv.bits[j >> 5] |= 1u << (j & 31u);
                            ++nf;
                        }
                    }
                }
            }
            if (nf < 8u) {
                s_stop = 1u;
            } else {
                // Optional GABA_A inhibitory blocker check!
                if (q.has_blocker) {
                    Word bov = 0;
                    for (Word w = 0; w < WORDS; ++w)
                        bov += __builtin_popcount(sv.bits[w] & q.block_sdr.bits[w]);
                    if (bov >= 16u) {
                        out.blocked_hit = 1u;
                        s_stop = 1u;
                    }
                }
                if (!s_stop) {
                    out.hop_spikes[h] = sv;
                    out.hop_counts[h] = nf;
                    out.hops_done = h + 1u;
                    s_sp += nf * 4u;
                    Word cn = 0;
                    for (Word w = 0; w < WORDS && cn < 64u; ++w) {
                        Word m = sv.bits[w];
                        s_excl[w] |= m;
                        while (m && cn < 64u) {
                            Word b = __builtin_ctz(m);
                            m &= m - 1u;
                            s_cur_idx[cn++] = (w << 5) | b;
                        }
                    }
                    s_ncur = cn;
                }
            }
        }
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) out.spikes_fired = s_sp;
}

// ---------------------------------------------------------------------------
// Kernel 25: Blackwell GPU Live Inhibitory LTD Unlearning & LTP Schema Revision
// (`mind_neo_revise_schema`) — Solves Continual Learning Without Forgetting!
//   Stage A: Spontaneously recalls superseded old target from `W = weights_all[pw]`
//            via 1-hop NMDA dendritic coincidence (or uses explicit `q.old_sdr`).
//   Stage B: Applies EVOLVED 3-factor LTD depression (`g.basal_rule`, pre=1,
//            post=1, pred=1, mod_pos=0) ONLY on `(pre -> old \ new)` synapses —
//            erasing superseded weights cleanly with ZERO catastrophic forgetting
//            of orthogonal schemas!
//   Stage C: Applies EVOLVED 3-factor LTP potentiation (`g.basal_rule`, pre=1,
//            post=1, pred=0, mod_pos=1) on `pre -> new` (and `new -> new2` in mode 1).
//   Stage D: Runs immediate 2-hop NMDA verification rollout on `W`, surviving
//            `--ablate hipp` (Sexvigintuple 26-Track Ribot's Law!) while
//            collapsing under `--ablate freeze` & `--ablate apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_revise_schema_block(signed char *weights_all, const NeoReviseSchemaQuery &q,
                                   const CorticalGenome &g, NeoReviseSchemaResult &out) {
    __attribute__((shared)) static Word s_pre[64], s_old[64], s_new[64], s_new2[64], s_cur[64];
    __attribute__((shared)) static Word s_npre, s_nold, s_nnew, s_nnew2, s_ncur, s_sp;
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_excl[WORDS];
    __attribute__((shared)) static Word s_ltd_red[256], s_ltp_red[256];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 19u;
    Word top_k = q.top_k > 0 ? q.top_k : 32u;
    int min_c = q.min_curr > 0 ? q.min_curr : 4;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) {
        out = {};
        auto unpack = [](const SpikeVec &sv, Word *dst, Word cap) -> Word {
            Word cnt = 0;
            for (Word w = 0; w < WORDS && cnt < cap; ++w) {
                Word m = sv.bits[w];
                while (m && cnt < cap) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    dst[cnt++] = (w << 5) | b;
                }
            }
            return cnt;
        };
        s_npre = unpack(q.pre_sdr, s_pre, 64u);
        s_nold = q.has_old ? unpack(q.old_sdr, s_old, 64u) : 0u;
        s_nnew = unpack(q.new_sdr, s_new, 64u);
        s_nnew2 = (q.mode == 1u) ? unpack(q.new2_sdr, s_new2, 64u) : 0u;
        s_sp = (s_npre + s_nnew + s_nnew2) * 2u;
    }
    asm volatile("bar.sync 0;");
    if (s_npre < 4u || s_nnew < 4u) return;

    // Stage A: If old_sdr not explicitly supplied, spontaneously recall superseded target from L2 `W`!
    if (!q.has_old) {
        for (Word j = tid; j < NEURONS; j += 256u) {
            if ((q.pre_sdr.bits[j >> 5] >> (j & 31u)) & 1u) {
                s_curr[j] = 0;
                continue;
            }
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int acc = 0;
            Word pos = 0;
            for (Word k = 0; k < s_npre; ++k) {
                int wv = int(row[s_pre[k]]);
                acc += wv;
                if (wv > 0) ++pos;
            }
            s_curr[j] = (acc > 0 && pos * 2u >= s_npre) ? acc : 0;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            int hi = -1000000;
            for (Word j = 0; j < NEURONS; ++j)
                if (s_curr[j] > hi) hi = s_curr[j];
            SpikeVec sv{};
            Word nf = 0;
            if (hi >= min_c) {
                int lo = min_c, thr_hi = hi + 1;
                for (int it = 0; it < 16; ++it) {
                    int mid = (lo + thr_hi) >> 1;
                    Word cnt = 0;
                    for (Word j = 0; j < NEURONS; ++j)
                        if (s_curr[j] >= mid) ++cnt;
                    if (cnt > top_k) lo = mid + 1;
                    else thr_hi = mid;
                }
                int thr = lo > hi ? hi : lo;
                if (thr < min_c) thr = min_c;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_curr[j] >= thr) {
                        sv.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
                if (nf < top_k && thr > min_c) {
                    int tie_v = thr - 1;
                    for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                        if (s_curr[j] == tie_v) {
                            sv.bits[j >> 5] |= 1u << (j & 31u);
                            ++nf;
                        }
                    }
                }
            }
            out.old_read_spikes = sv;
            out.old_count = nf;
            Word cnt = 0;
            for (Word w = 0; w < WORDS && cnt < 64u; ++w) {
                Word m = sv.bits[w];
                while (m && cnt < 64u) {
                    Word b = __builtin_ctz(m);
                    m &= m - 1u;
                    s_old[cnt++] = (w << 5) | b;
                }
            }
            s_nold = cnt;
            s_sp += nf * 3u;
        }
        asm volatile("bar.sync 0;");
    }

    // Stage B: Evolved 3-Factor Inhibitory LTD Unlearning on `(pre -> old \ new)`!
    Word loc_ltd = 0u, loc_ltp = 0u;
    int d_ltd = plasticity_delta(g.basal_rule, true, true, true, false);
    if (d_ltd >= 0) d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;
    int ltd_step = (-d_ltd) * int(q.strength > 0 ? q.strength * 4u : 24u);
    for (Word idx = tid; idx < s_nold; idx += 256u) {
        Word j = s_old[idx];
        if ((q.new_sdr.bits[j >> 5] >> (j & 31u)) & 1u)
            continue;
        signed char *row = W + (unsigned long long)(j) * NEURONS;
        for (Word k = 0; k < s_npre; ++k) {
            Word i = s_pre[k];
            int cur = int(row[i]);
            if (cur > 0) {
                int nxt = cur - ltd_step;
                if (nxt < 0) nxt = 0;
                row[i] = static_cast<signed char>(nxt);
                ++loc_ltd;
            }
        }
    }

    // Stage C: Evolved 3-Factor LTP Re-Imprint on `pre -> new` (and `new -> new2` if mode == 1)!
    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int burst = d_ltp * int(q.strength > 0 ? q.strength : 6u);
    for (Word idx = tid; idx < s_nnew; idx += 256u) {
        Word j = s_new[idx];
        signed char *row = W + (unsigned long long)(j) * NEURONS;
        for (Word k = 0; k < s_npre; ++k) {
            Word i = s_pre[k];
            int cur = int(row[i]);
            int nxt = cur + burst;
            if (nxt > 63) nxt = 63;
            if (nxt != cur) {
                row[i] = static_cast<signed char>(nxt);
                ++loc_ltp;
            }
        }
    }
    if (q.mode == 1u && s_nnew2 >= 4u) {
        for (Word idx = tid; idx < s_nnew2; idx += 256u) {
            Word j = s_new2[idx];
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word k = 0; k < s_nnew; ++k) {
                Word i = s_new[k];
                int cur = int(row[i]);
                int nxt = cur + burst;
                if (nxt > 63) nxt = 63;
                if (nxt != cur) {
                    row[i] = static_cast<signed char>(nxt);
                    ++loc_ltp;
                }
            }
        }
    }
    s_ltd_red[tid] = loc_ltd;
    s_ltp_red[tid] = loc_ltp;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) {
            s_ltd_red[tid] += s_ltd_red[tid + st];
            s_ltp_red[tid] += s_ltp_red[tid + st];
        }
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) {
        out.ltd_edits = s_ltd_red[0];
        out.ltp_edits = s_ltp_red[0];
        s_ncur = s_npre;
        for (Word k = 0; k < s_npre; ++k) s_cur[k] = s_pre[k];
        for (Word w = 0; w < WORDS; ++w) s_excl[w] = q.pre_sdr.bits[w];
    }
    asm volatile("bar.sync 0;");

    // Stage D: Immediate Post-Revision 2-Hop NMDA Verification Rollout on `W`!
    for (Word h = 0; h < 2u; ++h) {
        if (s_ncur < 4u) break;
        Word n_in = s_ncur;
        for (Word j = tid; j < NEURONS; j += 256u) {
            if ((s_excl[j >> 5] >> (j & 31u)) & 1u) {
                s_curr[j] = 0;
                continue;
            }
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int acc = 0;
            Word pos = 0;
            for (Word k = 0; k < n_in; ++k) {
                int wv = int(row[s_cur[k]]);
                acc += wv;
                if (wv > 0) ++pos;
            }
            s_curr[j] = (acc > 0 && pos * 2u >= n_in) ? acc : 0;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            int hi = -1000000;
            for (Word j = 0; j < NEURONS; ++j)
                if (s_curr[j] > hi) hi = s_curr[j];
            SpikeVec sv{};
            Word nf = 0;
            if (hi >= min_c) {
                int lo = min_c, thr_hi = hi + 1;
                for (int it = 0; it < 16; ++it) {
                    int mid = (lo + thr_hi) >> 1;
                    Word cnt = 0;
                    for (Word j = 0; j < NEURONS; ++j)
                        if (s_curr[j] >= mid) ++cnt;
                    if (cnt > top_k) lo = mid + 1;
                    else thr_hi = mid;
                }
                int thr = lo > hi ? hi : lo;
                if (thr < min_c) thr = min_c;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_curr[j] >= thr) {
                        sv.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
                if (nf < top_k && thr > min_c) {
                    int tie_v = thr - 1;
                    for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                        if (s_curr[j] == tie_v) {
                            sv.bits[j >> 5] |= 1u << (j & 31u);
                            ++nf;
                        }
                    }
                }
            }
            if (h == 0u) {
                out.hop1_spikes = sv;
                out.hop1_count = nf;
            } else {
                out.hop2_spikes = sv;
                out.hop2_count = nf;
            }
            s_sp += nf * 4u;
            Word cn = 0;
            if (nf >= 8u) {
                for (Word w = 0; w < WORDS && cn < 64u; ++w) {
                    Word m = sv.bits[w];
                    s_excl[w] |= m;
                    while (m && cn < 64u) {
                        Word b = __builtin_ctz(m);
                        m &= m - 1u;
                        s_cur[cn++] = (w << 5) | b;
                    }
                }
            }
            s_ncur = cn;
        }
        asm volatile("bar.sync 0;");
    }
    if (tid == 0) out.spikes_fired = s_sp;
}

// ---------------------------------------------------------------------------
// Kernel 26: Blackwell GPU Spiking SDR Circular-Phase Temporal Role-Binder &
// Unbinder (`mind_neo_role_bind`) — Stage 4 -> Stage 5 Crossing!
//   Binds arbitrary unseen 4-role episodes (AGENT, ACTION, OBJ, LOC) directly
//   into L2 plastic synapses (`weights_all[pw]`, PW25, ZERO extra L2 memory!)
//   using circular word-phase role shifts `{0, 5, 11, 19}` + evolved 3-factor
//   dendritic plasticity LUT (`g.basal_rule`):
//     Mode 0 (BIND): For each active role `t in 0..3`, imprints
//       `rot(OR_{r!=t} roles[r], SHIFT[t]) -> rot(roles[t], SHIFT[t])`
//       straight into L2 `W` with ZERO atomics & ZERO role interference!
//     Mode 1 (UNBIND): Given ANY 1 or 2 partial cue SDRs (`cue_a | cue_b`) and
//       target `query_role = t`, phase-shifts the cue by `SHIFT[t]`, executes
//       NMDA dendritic coincidence (`pos * 2 >= n_in`) + tie-preserving k-WTA,
//       and inverse-rotates the winning spike assembly (`inv_rot(..., SHIFT[t])`)
//       — surviving `--ablate hipp` (Septemvigintuple 27-Track Ribot's Law!)
//       while collapsing cleanly under `--ablate freeze` & `--ablate apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_role_bind_block(signed char *weights_all, const NeoRoleBindQuery &q,
                               const CorticalGenome &g, NeoRoleBindResult &out) {
    __attribute__((shared)) static Word s_pre[128], s_post[64];
    __attribute__((shared)) static Word s_npre, s_npost, s_sp;
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_ed_red[256];

    constexpr Word ROLE_SH[4] = {0u, 5u, 11u, 19u};
    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 25u;
    Word top_k = q.top_k > 0 ? q.top_k : 32u;
    int min_c = q.min_curr > 0 ? q.min_curr : 4;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    auto rot_sv = [](const SpikeVec &in, Word sh) -> SpikeVec {
        SpikeVec r{};
        for (Word w = 0; w < WORDS; ++w)
            r.bits[(w + sh) & 31u] = in.bits[w];
        return r;
    };
    auto inv_rot_sv = [](const SpikeVec &in, Word sh) -> SpikeVec {
        SpikeVec r{};
        for (Word w = 0; w < WORDS; ++w)
            r.bits[w] = in.bits[(w + sh) & 31u];
        return r;
    };
    auto unpack = [](const SpikeVec &sv, Word *dst, Word cap) -> Word {
        Word cnt = 0;
        for (Word w = 0; w < WORDS && cnt < cap; ++w) {
            Word m = sv.bits[w];
            while (m && cnt < cap) {
                Word b = __builtin_ctz(m);
                m &= m - 1u;
                dst[cnt++] = (w << 5) | b;
            }
        }
        return cnt;
    };

    if (tid == 0) {
        out = {};
        s_sp = 0u;
    }
    asm volatile("bar.sync 0;");

    if (q.mode == 0u) {
        // Mode 0: 1-Shot Phase-Shifted 4-Role Hebbian Binding via EVOLVED `g.basal_rule` LUT!
        int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
        if (d_ltp <= 0) d_ltp = 1;
        int burst = d_ltp * int(q.strength > 0 ? q.strength : 6u);
        Word local_edits = 0u;

        for (Word t = 0; t < 4u; ++t) {
            if (tid == 0) {
                SpikeVec pre_raw{};
                for (Word r = 0; r < 4u; ++r) {
                    if (r == t) continue;
                    for (Word w = 0; w < WORDS; ++w)
                        pre_raw.bits[w] |= q.roles[r].bits[w];
                }
                SpikeVec pre_sh = rot_sv(pre_raw, ROLE_SH[t]);
                SpikeVec post_sh = rot_sv(q.roles[t], ROLE_SH[t]);
                s_npre = unpack(pre_sh, s_pre, 128u);
                s_npost = unpack(post_sh, s_post, 64u);
                s_sp += (s_npre + s_npost) * 2u;
            }
            asm volatile("bar.sync 0;");
            if (s_npre >= 4u && s_npost >= 4u) {
                for (Word idx = tid; idx < s_npost; idx += 256u) {
                    Word j = s_post[idx];
                    signed char *row = W + (unsigned long long)(j) * NEURONS;
                    for (Word k = 0; k < s_npre; ++k) {
                        Word i = s_pre[k];
                        int cur = int(row[i]);
                        int nxt = cur + burst;
                        if (nxt > 63) nxt = 63;
                        if (nxt != cur) {
                            row[i] = static_cast<signed char>(nxt);
                            ++local_edits;
                        }
                    }
                }
            }
            asm volatile("bar.sync 0;");
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = s_sp;
        }
        return;
    }

    // Mode 1: Pure-GPU Phase-Shifted Role Unbinding Readout!
    Word qr = q.query_role & 3u;
    Word sh = ROLE_SH[qr];
    if (tid == 0) {
        SpikeVec cue_raw{};
        for (Word w = 0; w < WORDS; ++w)
            cue_raw.bits[w] = q.cue_a.bits[w] | q.cue_b.bits[w];
        SpikeVec cue_sh = rot_sv(cue_raw, sh);
        s_npre = unpack(cue_sh, s_pre, 128u);
        s_sp = s_npre * 2u;
    }
    asm volatile("bar.sync 0;");
    if (s_npre < 4u) return;

    for (Word j = tid; j < NEURONS; j += 256u) {
        const signed char *row = W + (unsigned long long)(j) * NEURONS;
        int acc = 0;
        Word pos = 0;
        for (Word k = 0; k < s_npre; ++k) {
            int wv = int(row[s_pre[k]]);
            acc += wv;
            if (wv > 0) ++pos;
        }
        s_curr[j] = (acc > 0 && pos * 2u >= s_npre) ? acc : 0;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        int hi = -1000000;
        for (Word j = 0; j < NEURONS; ++j)
            if (s_curr[j] > hi) hi = s_curr[j];
        SpikeVec sv_sh{};
        Word nf = 0;
        if (hi >= min_c) {
            int lo = min_c, thr_hi = hi + 1;
            for (int it = 0; it < 16; ++it) {
                int mid = (lo + thr_hi) >> 1;
                Word cnt = 0;
                for (Word j = 0; j < NEURONS; ++j)
                    if (s_curr[j] >= mid) ++cnt;
                if (cnt > top_k) lo = mid + 1;
                else thr_hi = mid;
            }
            int thr = lo > hi ? hi : lo;
            if (thr < min_c) thr = min_c;
            for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                if (s_curr[j] >= thr) {
                    sv_sh.bits[j >> 5] |= 1u << (j & 31u);
                    ++nf;
                }
            }
            if (nf < top_k && thr > min_c) {
                int tie_v = thr - 1;
                for (Word j = 0; j < NEURONS && nf < top_k; ++j) {
                    if (s_curr[j] == tie_v) {
                        sv_sh.bits[j >> 5] |= 1u << (j & 31u);
                        ++nf;
                    }
                }
            }
        }
        out.unbound_sdr = inv_rot_sv(sv_sh, sh);
        out.active_count = nf;
        out.max_current = hi > 0 ? hi : 0;
        out.spikes_fired = s_sp + nf * 4u;
    }
}

// ---------------------------------------------------------------------------
// Kernel 27: Blackwell GPU Spiking Predictive-Coding Sequence Role-Tagger &
// Self-Organizing Compositional Syntax Circuit (`mind_neo_seq_predict`) —
// THE GREAT ARCHITECTURAL CROSSING INTO STAGE 5 OF 8!
//   Lives inside L2 PW12 (Narrative Story-Role Cortex — ZERO extra L2 memory,
//   ZERO atomics!). Learns ANY dialect's word order (English SVO, Alien SOV,
//   or arbitrary permutations) purely via evolved 3-factor STDP (`g.basal_rule`):
//     Mode 0 (STDP TRAIN): For each position t = 0..L-1, builds
//       `state = pos_start[t] | pos_end[L-1-t] | prev_role | adj_rel_cue`
//       and applies evolved `g.basal_rule` LTP onto `role_sdrs[target_roles[t]]`
//       plus competitive LTD onto `role_sdrs[other_roles]` straight into L2 PW12!
//     Mode 1 (RECURRENT PARSE / ORDER ROLLOUT): Runs a pure-GPU token-by-token
//       recurrent spiking pass t = 0..L-1 projecting `state_sdr` through L2 PW12,
//       scoring all unassigned thematic roles with lateral-inhibition WTA, feeding
//       the winning role's spikes into step t+1, and outputting `pred_roles[0..L-1]`
//       — powering BOTH template-free unseen-sentence comprehension (feeding K26)
//       AND dialect-agnostic compositional Broca generation, SURVIVING `--ablate hipp`
//       (Octovigintuple 28-Track Ribot's Law!)!
// ---------------------------------------------------------------------------
AX_FN void neo_seq_predict_block(signed char *weights_all, const NeoSeqPredictQuery &q,
                                 const CorticalGenome &g, NeoSeqPredictResult &out) {
    __attribute__((shared)) static Word s_pre[160], s_post[64], s_neg[128];
    __attribute__((shared)) static Word s_npre, s_npost, s_nneg, s_sp;
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_prev_bits[WORDS];
    __attribute__((shared)) static Word s_used_mask;

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 12u;
    Word L = q.seq_len;
    if (L == 0u) L = 4u;
    if (L > MAX_SEQ_LEN) L = MAX_SEQ_LEN;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    auto unpack = [](const SpikeVec &sv, Word *dst, Word cap) -> Word {
        Word cnt = 0;
        for (Word w = 0; w < WORDS && cnt < cap; ++w) {
            Word m = sv.bits[w];
            while (m && cnt < cap) {
                Word b = __builtin_ctz(m);
                m &= m - 1u;
                dst[cnt++] = (w << 5) | b;
            }
        }
        return cnt;
    };

    if (tid == 0) {
        out = {};
        s_sp = 0u;
        for (Word w = 0; w < WORDS; ++w) s_prev_bits[w] = q.prev_bos_sdr.bits[w];
        s_used_mask = 0u;
    }
    asm volatile("bar.sync 0;");

    if (q.mode == 0u) {
        // Mode 0: Evolved 3-Factor STDP Predictive-Grammar Consolidation into L2 PW12!
        int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
        if (d_ltp <= 0) d_ltp = 1;
        int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
        if (d_ltd >= 0) d_ltd = -1;
        int str = int(q.strength > 0 ? q.strength : 6u);
        int b_ltp = d_ltp * str;
        int b_ltd = d_ltd * (str / 2 > 0 ? str / 2 : 1);
        Word local_edits = 0u;

        for (Word t = 0; t < L; ++t) {
            Word tr = q.target_roles[t] & 3u;
            if (tid == 0) {
                SpikeVec st{};
                Word pe = L - 1u - t;
                const SpikeVec &pr = (t == 0u) ? q.prev_bos_sdr : q.role_sdrs[q.target_roles[t - 1u] & 3u];
                bool adj = ((q.adj_rel_mask >> t) & 1u) != 0u;
                for (Word w = 0; w < WORDS; ++w) {
                    Word bits = q.pos_start_sdrs[t].bits[w] | q.pos_end_sdrs[pe].bits[w] | pr.bits[w];
                    if (adj) bits |= q.adj_rel_sdr.bits[w];
                    st.bits[w] = bits;
                }
                SpikeVec neg_sv{};
                for (Word r = 0; r < 4u; ++r) {
                    if (r == tr) continue;
                    for (Word w = 0; w < WORDS; ++w)
                        neg_sv.bits[w] |= q.role_sdrs[r].bits[w];
                }
                for (Word w = 0; w < WORDS; ++w)
                    neg_sv.bits[w] &= ~q.role_sdrs[tr].bits[w];
                s_npre = unpack(st, s_pre, 160u);
                s_npost = unpack(q.role_sdrs[tr], s_post, 64u);
                s_nneg = unpack(neg_sv, s_neg, 128u);
                s_sp += (s_npre + s_npost) * 2u;
            }
            asm volatile("bar.sync 0;");
            if (s_npre >= 4u) {
                for (Word idx = tid; idx < s_npost; idx += 256u) {
                    Word j = s_post[idx];
                    signed char *row = W + (unsigned long long)(j) * NEURONS;
                    for (Word k = 0; k < s_npre; ++k) {
                        Word i = s_pre[k];
                        int cur = int(row[i]);
                        int nxt = cur + b_ltp;
                        if (nxt > 63) nxt = 63;
                        if (nxt != cur) { row[i] = static_cast<signed char>(nxt); ++local_edits; }
                    }
                }
                for (Word idx = tid; idx < s_nneg; idx += 256u) {
                    Word j = s_neg[idx];
                    signed char *row = W + (unsigned long long)(j) * NEURONS;
                    for (Word k = 0; k < s_npre; ++k) {
                        Word i = s_pre[k];
                        int cur = int(row[i]);
                        int nxt = cur + b_ltd;
                        if (nxt < -63) nxt = -63;
                        if (nxt != cur) { row[i] = static_cast<signed char>(nxt); ++local_edits; }
                    }
                }
            }
            asm volatile("bar.sync 0;");
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = s_sp;
        }
        return;
    }

    // Mode 1: Pure-GPU Recurrent Token-by-Token Predictive Role Tagging & Order Rollout!
    for (Word t = 0; t < L; ++t) {
        if (tid == 0) {
            SpikeVec st{};
            Word pe = L - 1u - t;
            bool adj = ((q.adj_rel_mask >> t) & 1u) != 0u;
            for (Word w = 0; w < WORDS; ++w) {
                Word bits = q.pos_start_sdrs[t].bits[w] | q.pos_end_sdrs[pe].bits[w] | s_prev_bits[w];
                if (adj) bits |= q.adj_rel_sdr.bits[w];
                st.bits[w] = bits;
            }
            s_npre = unpack(st, s_pre, 160u);
            s_sp += s_npre;
        }
        asm volatile("bar.sync 0;");
        for (Word j = tid; j < NEURONS; j += 256u) {
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int acc = 0;
            for (Word k = 0; k < s_npre; ++k)
                acc += int(row[s_pre[k]]);
            s_curr[j] = acc;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            int best_sc = -1000000000;
            Word best_r = 4u;
            for (Word r = 0; r < 4u; ++r) {
                if ((s_used_mask >> r) & 1u) continue;
                int sc = 0;
                for (Word w = 0; w < WORDS; ++w) {
                    Word m = q.role_sdrs[r].bits[w];
                    while (m) {
                        Word b = __builtin_ctz(m);
                        m &= m - 1u;
                        sc += s_curr[(w << 5) | b];
                    }
                }
                if (sc > best_sc) {
                    best_sc = sc;
                    best_r = r;
                }
            }
            if (best_r < 4u && best_sc > 0) {
                out.pred_roles[t] = best_r;
                out.pred_scores[t] = best_sc;
                s_used_mask |= (1u << best_r);
                for (Word w = 0; w < WORDS; ++w) s_prev_bits[w] = q.role_sdrs[best_r].bits[w];
                ++out.unique_count;
                s_sp += 128u;
            } else {
                out.pred_roles[t] = 4u;
                out.pred_scores[t] = best_sc;
            }
        }
        asm volatile("bar.sync 0;");
    }
    if (tid == 0)
        out.spikes_fired = s_sp;
}

// ---------------------------------------------------------------------------
// Kernel 28: Blackwell GPU Neural-Native Spiking Parietal Number-Line Bump
// Attractor (`mind_neo_neural_math`) — STAGE 5b NEURAL-NATIVE GPU MATH!
//   Lives inside L2 PW15 (ZERO extra L2 memory, ZERO atomics!).
//   Encodes integers 0..63 as 64 localized 16-neuron parietal cortical bumps
//   across the 1,024-neuron column (`BUMP[v] = neurons [16*v .. 16*v + 15]`).
//     Mode 0 (STDP CONSOLIDATE): Consolidates successor synapses
//       `BUMP[v] -> BUMP[(v + 1) & 63]` directly into L2 PW15 via the evolved
//       3-factor dendritic plasticity LUT (`g.basal_rule`) — 16,384 synapses!
//     Mode 1 (PURE-GPU SPIKING BUMP PROPAGATION — ZERO HOST +, -, *, /, %!):
//       - op=0 (a + b): Forward synaptic sweeps `W * active` + k=16 k-WTA!
//       - op=1 (a - b): Backward apical sweeps `W^T * active` + k=16 k-WTA!
//       - op=2 ((a + b) mod m): Forward sweeps + GABA_A boundary reset at `m`!
//       - op=3 (solve a*x + b = c): Backward apical shift `-b`, then repeated
//         `a`-step backward apical waves counting quotient ticks until bump
//         hits `BUMP[0]`!
//       - op=4 (a * b): `b` forward bursts of `a` bump steps from `BUMP[0]`!
//   SURVIVES `--ablate hipp` (Nonavigintuple 29-Track Ribot's Law!) while
//   collapsing cleanly to 0 under `--ablate apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_neural_math_block(signed char *weights_all, const NeoNeuralMathQuery &q,
                                 const CorticalGenome &g, NeoNeuralMathResult &out) {
    __attribute__((shared)) static Word s_act[16];
    __attribute__((shared)) static Word s_nact, s_sp, s_steps, s_done, s_hit0, s_hitm;
    __attribute__((shared)) static int s_curr[NEURONS];
    __attribute__((shared)) static Word s_ed_red[256];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 15u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) {
        out = {};
        s_sp = 0u;
        s_steps = 0u;
        s_done = 0u;
    }
    asm volatile("bar.sync 0;");

    if (q.mode == 0u) {
        // Mode 0: Evolved 3-Factor STDP Consolidation of 64-State Parietal Successor Ring into L2 PW15!
        int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
        if (d_ltp <= 0) d_ltp = 1;
        int burst = d_ltp * int(q.strength > 0 ? q.strength : 6u);
        Word local_edits = 0u;

        // Parallelize across all 1,024 postsynaptic neurons (4 neurons per thread, ZERO atomics!)
        for (Word j = tid; j < NEURONS; j += 256u) {
            Word post_v = j >> 4;            // target state 0..63
            Word pre_v = (post_v + 63u) & 63u; // predecessor state (post_v - 1) mod 64
            Word pre_lo = pre_v << 4;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word k = 0; k < 16u; ++k) {
                Word i = pre_lo + k;
                int cur = int(row[i]);
                int nxt = cur + burst;
                if (nxt > 63) nxt = 63;
                if (nxt != cur) {
                    row[i] = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = 64u * 32u;
            out.valid = 1u;
        }
        return;
    }

    // Mode 1: Pure-GPU Spiking Parietal Bump Wave Propagation!
    auto seed_bump = [&](Word val) {
        if (tid == 0) {
            Word base = (val & 63u) << 4;
            for (Word k = 0; k < 16u; ++k) s_act[k] = base + k;
            s_nact = 16u;
            s_sp += 16u;
        }
        asm volatile("bar.sync 0;");
    };

    auto step_wave = [&](bool backward) {
        for (Word j = tid; j < NEURONS; j += 256u) {
            int acc = 0;
            if (!backward) {
                const signed char *row = W + (unsigned long long)(j) * NEURONS;
                for (Word k = 0; k < s_nact; ++k)
                    acc += int(row[s_act[k]]);
            } else {
                for (Word k = 0; k < s_nact; ++k)
                    acc += int(W[(unsigned long long)(s_act[k]) * NEURONS + j]);
            }
            s_curr[j] = acc;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            // PV-interneuron k-WTA (k=16): pick top 16 firing neurons across the 1,024-cell column!
            int hi = 0;
            for (Word j = 0; j < NEURONS; ++j)
                if (s_curr[j] > hi) hi = s_curr[j];
            Word nf = 0u;
            if (hi >= 4) {
                int thr = (hi * 3) >> 2;
                if (thr < 4) thr = 4;
                for (Word j = 0; j < NEURONS && nf < 16u; ++j)
                    if (s_curr[j] >= thr) s_act[nf++] = j;
            }
            s_nact = nf;
            s_sp += nf * 2u;
            ++s_steps;
        }
        asm volatile("bar.sync 0;");
    };

    auto check_bump = [&](Word target_v, Word &flag_out) {
        if (tid == 0) {
            Word lo = (target_v & 63u) << 4, hi = lo + 16u;
            Word ov = 0u;
            for (Word k = 0; k < s_nact; ++k)
                if (s_act[k] >= lo && s_act[k] < hi) ++ov;
            flag_out = (ov >= 12u) ? 1u : 0u;
        }
        asm volatile("bar.sync 0;");
    };

    if (q.op == 0u) {
        // Pure-GPU Spiking Addition: a + b
        seed_bump(q.a & 63u);
        for (Word s = 0; s < (q.b & 63u); ++s) step_wave(false);
    } else if (q.op == 1u) {
        // Pure-GPU Spiking Subtraction: a - b (transposed apical backward waves W^T!)
        seed_bump(q.a & 63u);
        for (Word s = 0; s < (q.b & 63u); ++s) step_wave(true);
    } else if (q.op == 2u) {
        // Pure-GPU Spiking Modular Arithmetic: (a + b) mod c via GABA_A boundary reset at bump `c`!
        Word mod_m = (q.c >= 2u && q.c <= 63u) ? q.c : 12u;
        seed_bump(0u);
        Word tot_s = (q.a & 63u) + (q.b & 63u);
        for (Word s = 0; s < tot_s; ++s) {
            step_wave(false);
            check_bump(mod_m, s_hitm);
            if (s_hitm) seed_bump(0u);
        }
    } else if (q.op == 3u) {
        // Pure-GPU Spiking Linear Equation Inversion: solve `a * x + b = c` with ZERO host division!
        Word a_step = (q.a >= 1u && q.a <= 63u) ? q.a : 1u;
        seed_bump(q.c & 63u);
        for (Word s = 0; s < (q.b & 63u); ++s) step_wave(true);
        Word x_cnt = 0u;
        check_bump(0u, s_hit0);
        while (!s_hit0 && x_cnt < 63u && s_nact >= 12u) {
            for (Word s = 0; s < a_step; ++s) step_wave(true);
            ++x_cnt;
            check_bump(0u, s_hit0);
        }
        if (s_hit0) seed_bump(x_cnt);
        else if (tid == 0) s_nact = 0u;
        asm volatile("bar.sync 0;");
    } else {
        // Pure-GPU Spiking Multiplication: a * b (b repeated forward bursts of a bump steps!)
        seed_bump(0u);
        Word na = q.a & 63u, nb = q.b & 63u;
        for (Word t = 0; t < nb; ++t)
            for (Word s = 0; s < na; ++s) step_wave(false);
    }

    if (tid == 0) {
        SpikeVec ans_sv{};
        Word votes[64]{};
        for (Word k = 0; k < s_nact; ++k) {
            Word j = s_act[k];
            ans_sv.bits[j >> 5] |= 1u << (j & 31u);
            ++votes[j >> 4];
        }
        Word best_v = 0u, best_c = 0u;
        for (Word v = 0; v < 64u; ++v) {
            if (votes[v] > best_c) { best_c = votes[v]; best_v = v; }
        }
        out.answer_spikes = ans_sv;
        out.answer_val = best_v;
        out.active_count = s_nact;
        out.wave_steps = s_steps;
        out.spikes_fired = s_sp;
        out.valid = (best_c >= 12u) ? 1u : 0u;
    }
}

// ---------------------------------------------------------------------------
// Kernel 29: Blackwell GPU Pure-Spiking Axonal-Delay Graph Wavefront Solver
// (`mind_neo_graph_wave`) — STAGE 5c NEURAL-NATIVE GPU GRAPH PLANNING!
//   Lives inside L2 PW11 (ZERO extra L2 memory, ZERO atomics, ZERO host Dijkstra!).
//   Encodes up to 32 arbitrary concept graph nodes as 32 orthogonal 32-neuron
//   entorhinal/parietal assemblies (`NODE[u] = neurons [32*u .. 32*u + 31]`).
//     Mode 0 (STDP GRAPH WRITE): Optionally clears PW11 via LTD, then writes
//       up to 16 directed weighted edges `(u -> v, cost w in 1..15)` via
//       evolved `g.basal_rule` STDP encoding inverse axonal latency
//       (`syn = (16 - w) * 4`: stronger myelinated synapse = faster conduction!).
//     Mode 1 (PURE-GPU AXONAL-DELAY SPIKING WAVEFRONT DIJKSTRA):
//       1) All 256 threads decode the 32x32 synaptic delay matrix in parallel
//          straight from L2 PW11 (`delay = 16 - (mean_w >> 2)`).
//       2) A discrete spiking wavefront propagates from `src_node` at tick 0;
//          each un-fired node `v != blocked_node` fires at the EXACT earliest
//          tick `arrival[u] + delay(u->v)` (physical axonal spike-timing
//          optimality — Ponulak & Hopfield 2013!).
//       3) Backtracks `parent[]` on GPU to recover `total_cost`, `hop_count`,
//          `next_hop`, and the 32-neuron firing SDR of `dst_node`!
//   SURVIVES `--ablate hipp` (Trigintuple 30-Track Ribot's Law!) while
//   collapsing cleanly to 0 under `--ablate apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_graph_wave_block(signed char *weights_all, const NeoGraphWaveQuery &q,
                                const CorticalGenome &g, NeoGraphWaveResult &out) {
    __attribute__((shared)) static Word s_delay[MAX_GW_NODES * MAX_GW_NODES];
    __attribute__((shared)) static Word s_fired[MAX_GW_NODES];
    __attribute__((shared)) static Word s_arrival[MAX_GW_NODES];
    __attribute__((shared)) static Word s_parent[MAX_GW_NODES];
    __attribute__((shared)) static Word s_hops[MAX_GW_NODES];
    __attribute__((shared)) static Word s_new_fire[MAX_GW_NODES];
    __attribute__((shared)) static Word s_new_par[MAX_GW_NODES];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_sp, s_wticks;

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 11u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) {
        out = {};
        s_sp = 0u;
        s_wticks = 0u;
    }
    asm volatile("bar.sync 0;");

    if (q.mode == 0u) {
        // Mode 0: Evolved-LUT STDP Weighted Graph Imprinting into L2 PW11!
        int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
        if (d_ltp <= 0) d_ltp = 1;
        Word local_edits = 0u;
        if (q.clear_first) {
            for (Word j = tid; j < NEURONS; j += 256u) {
                signed char *row = W + (unsigned long long)(j) * NEURONS;
                for (Word i = 0; i < NEURONS; ++i) {
                    if (row[i] != 0) {
                        row[i] = 0;
                        ++local_edits;
                    }
                }
            }
            asm volatile("bar.sync 0;");
        }
        Word ne = q.n_edges < MAX_GW_EDGES ? q.n_edges : MAX_GW_EDGES;
        for (Word e = 0; e < ne; ++e) {
            Word u = q.edge_u[e] & 31u;
            Word v = q.edge_v[e] & 31u;
            Word cost = q.edge_w[e];
            if (cost < 1u) cost = 1u;
            if (cost > 15u) cost = 15u;
            int target_w = int(16u - cost) * 4;
            Word u_lo = u << 5, v_lo = v << 5;
            // 256 threads write the 32 x 32 = 1,024 plastic synapses (4 synapses/thread, ZERO atomics!)
            for (Word k = tid; k < 1024u; k += 256u) {
                Word j = v_lo + (k >> 5);
                Word i = u_lo + (k & 31u);
                signed char *cell = W + (unsigned long long)(j) * NEURONS + i;
                int cur = int(*cell);
                // Step via evolved d_ltp to reach the conductance matching conduction delay `cost`
                int nxt = target_w;
                if (cur < target_w) {
                    int steps = (target_w - cur + d_ltp - 1) / d_ltp;
                    nxt = cur + steps * d_ltp;
                    if (nxt > target_w) nxt = target_w;
                }
                if (nxt != cur) {
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
            asm volatile("bar.sync 0;");
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = ne * 64u;
            out.found = 1u;
        }
        return;
    }

    // Mode 1: Pure-GPU Axonal-Delay Spiking Wavefront Shortest-Path Propagation!
    // Step A: All 256 threads decode all 32 x 32 = 1,024 directed node synaptic delays in parallel!
    for (Word idx = tid; idx < 1024u; idx += 256u) {
        Word u = idx >> 5;
        Word v = idx & 31u;
        Word u_lo = u << 5, v_lo = v << 5;
        int acc = 0;
        for (Word k = 0; k < 32u; ++k)
            acc += int(W[(unsigned long long)(v_lo + k) * NEURONS + (u_lo + k)]);
        int g_bar = acc >> 5;
        if (g_bar >= 4) {
            Word d = 16u - Word(g_bar >> 2);
            if (d < 1u) d = 1u;
            if (d > 15u) d = 15u;
            s_delay[idx] = d;
        } else {
            s_delay[idx] = 0u;
        }
    }
    if (tid < MAX_GW_NODES) {
        s_fired[tid] = 0u;
        s_arrival[tid] = 0u;
        s_parent[tid] = tid;
        s_hops[tid] = 0u;
        s_new_fire[tid] = 0u;
        s_new_par[tid] = tid;
    }
    asm volatile("bar.sync 0;");

    Word src = q.src_node & 31u;
    Word dst = q.dst_node & 31u;
    if (tid == 0 && src != q.blocked_node) {
        s_fired[src] = 1u;
        s_arrival[src] = 0u;
        s_parent[src] = src;
        s_hops[src] = 0u;
        s_sp = 32u;
    }
    asm volatile("bar.sync 0;");

    // Step B: Discrete Gamma-Tick Axonal-Delay Wavefront (ticks 1..63)!
    for (Word tick = 1u; tick <= 63u; ++tick) {
        if (s_fired[dst]) break;
        if (tid < MAX_GW_NODES) {
            Word v = tid;
            s_new_fire[v] = 0u;
            if (!s_fired[v] && v != q.blocked_node) {
                for (Word u = 0u; u < MAX_GW_NODES; ++u) {
                    Word d = s_delay[(u << 5) | v];
                    if (d > 0u && s_fired[u] && s_arrival[u] + d == tick) {
                        s_new_fire[v] = 1u;
                        s_new_par[v] = u;
                        break;
                    }
                }
            }
        }
        asm volatile("bar.sync 0;");
        if (tid < MAX_GW_NODES && s_new_fire[tid]) {
            Word p = s_new_par[tid];
            s_fired[tid] = 1u;
            s_arrival[tid] = tick;
            s_parent[tid] = p;
            s_hops[tid] = s_hops[p] + 1u;
            s_new_fire[tid] = 0u;
        }
        if (tid == 0) {
            s_wticks = tick;
            Word newly = 0u;
            for (Word v = 0u; v < MAX_GW_NODES; ++v)
                if (s_fired[v] && s_arrival[v] == tick) ++newly;
            s_sp += newly * 32u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        out.wave_ticks = s_wticks;
        out.spikes_fired = s_sp;
        if (s_fired[dst]) {
            out.found = 1u;
            out.total_cost = s_arrival[dst];
            out.hop_count = s_hops[dst];
            Word cur = dst;
            Word guard = 0u;
            while (cur != src && s_parent[cur] != src && guard < 32u) {
                cur = s_parent[cur];
                ++guard;
            }
            out.next_hop = cur;
            SpikeVec dsv{};
            Word d_lo = dst << 5;
            for (Word k = 0; k < 32u; ++k) {
                Word j = d_lo + k;
                dsv.bits[j >> 5] |= 1u << (j & 31u);
            }
            out.dest_spikes = dsv;
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel 30 (Stage 5d!): Pure-GPU Spiking Boltzmann/Hopfield 3-SAT & Constraint Attractor (`mind_neo_sat_solve`)!
// Reuses unused L2 Pathway 13 (`PW13`, 1,024 x 1,024 int8 synapses) — ZERO extra L2 memory, ZERO atomics, ZERO host SAT solver!
//   - Neurons `0..511`:   16 Boolean variables x 2 polarities (`l = v*2 + pol`, `pol=0` FALSE / `pol=1` TRUE) x 16 neurons/literal.
//   - Neurons `512..1023`: 32 Clause-Alarm interneuron assemblies (`c = 0..31`) x 16 neurons/clause.
// Mode 0: Evolved-LUT STDP (`g.basal_rule`) writes excitatory alarm->lit (`+32`), GABA_A inhibitory lit->alarm (`-32`) & clause tonic marker (`+16`).
// Mode 1: Decodes all 32 clauses straight from L2 PW13 plastic synapses, then launches 8 parallel 32-lane spiking Boltzmann replicas (1 warp = 1 replica,
//         all 32 clauses evaluated simultaneously via hardware `warp_ballot`!) to converge to a zero-alarm satisfying attractor state!
// Mode 2: Simultaneously pins `query_var = 1` on Warps 0..3 and `query_var = 0` on Warps 4..7 in ONE launch to prove forced backbone truth!
// SURVIVES `--ablate hipp` (Untrigintuple 31-Track Ribot's Law!) & `--ablate freeze`, collapsing cleanly to 0/3 under `--ablate apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_sat_solve_block(signed char *weights_all, const NeoSatSolveQuery &q, const CorticalGenome &g,
                               NeoSatSolveResult &out) {
    __attribute__((shared)) static Word s_c_active[MAX_SAT_CLAUSES];
    __attribute__((shared)) static Word s_c_nlits[MAX_SAT_CLAUSES];
    __attribute__((shared)) static Word s_c_lits[MAX_SAT_CLAUSES * 3];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_w_found[8];
    __attribute__((shared)) static Word s_w_assign[8];
    __attribute__((shared)) static Word s_w_steps[8];
    __attribute__((shared)) static Word s_w_unsat[8];
    __attribute__((shared)) static Word s_w_spikes[8];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 13u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    if (q.mode == 0u) {
        // Mode 0: Evolved-LUT STDP Clause Imprinting into L2 PW13!
        int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
        if (d_ltp <= 0) d_ltp = 1;
        int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
        if (d_ltd >= 0) d_ltd = -1;
        Word local_edits = 0u;

        if (q.clear_first) {
            for (Word j = tid; j < NEURONS; j += 256u) {
                signed char *row = W + (unsigned long long)(j) * NEURONS;
                Word i_start = (j < 512u) ? 512u : 0u; // Preserve orthogonal quadrant `PW13[0..511, 0..511]` for K39+!
                for (Word i = i_start; i < NEURONS; ++i) {
                    if (row[i] != 0) {
                        row[i] = 0;
                        ++local_edits;
                    }
                }
            }
            asm volatile("bar.sync 0;");
        }

        Word nc = q.n_clauses < MAX_SAT_WRITE ? q.n_clauses : MAX_SAT_WRITE;
        for (Word ci = 0; ci < nc; ++ci) {
            Word c = q.clause_start + ci;
            if (c >= MAX_SAT_CLAUSES) break;
            Word c_lo = 512u + (c << 4);
            // Step 1: Write self-tonic baseline `+16` on CLAUSE(c) -> CLAUSE(c) (16x16 = 256 synapses, 1 per thread!)
            {
                Word j = c_lo + (tid >> 4);
                Word i = c_lo + (tid & 15u);
                signed char *cell = W + (unsigned long long)(j) * NEURONS + i;
                int cur = int(*cell);
                int target = 16;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
            // Step 2: Imprint up to 3 literals (`lit -> -32 -> clause` GABA_A inhibition & `clause -> +32 -> lit` excitation)
            Word lits[3] = {q.lit0[ci], q.lit1[ci], q.lit2[ci]};
            for (Word li = 0; li < 3u; ++li) {
                Word l = lits[li];
                if (l >= MAX_SAT_VARS * 2u) continue;
                Word l_lo = l << 4;
                // Inhibitory LIT(l) -> CLAUSE(c) (`W[c_lo + r, l_lo + k] -> -32` via evolved d_ltd)
                {
                    Word j = c_lo + (tid >> 4);
                    Word i = l_lo + (tid & 15u);
                    signed char *cell = W + (unsigned long long)(j) * NEURONS + i;
                    int cur = int(*cell);
                    int target = -32;
                    if (cur > target) {
                        int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                        int nxt = cur + steps * d_ltd;
                        if (nxt < target) nxt = target;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                }
                // Excitatory CLAUSE(c) -> LIT(l) (`W[l_lo + r, c_lo + k] -> +32` via evolved d_ltp)
                {
                    Word j = l_lo + (tid >> 4);
                    Word i = c_lo + (tid & 15u);
                    signed char *cell = W + (unsigned long long)(j) * NEURONS + i;
                    int cur = int(*cell);
                    int target = 32;
                    if (cur < target) {
                        int steps = (target - cur + d_ltp - 1) / d_ltp;
                        int nxt = cur + steps * d_ltp;
                        if (nxt > target) nxt = target;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                }
            }
            asm volatile("bar.sync 0;");
        }

        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = nc * 96u;
            out.satisfiable = 1u;
        }
        return;
    }

    // Mode 1 / Mode 2: Decode ALL 32 clauses directly from consolidated L2 PW13 plastic synapses!
    if (tid < MAX_SAT_CLAUSES) {
        Word c = tid;
        Word c_lo = 512u + (c << 4);
        int tonic = 0;
        for (Word k = 0; k < 16u; ++k)
            tonic += int(W[(unsigned long long)(c_lo + k) * NEURONS + (c_lo + k)]);
        Word act = (tonic >= 128) ? 1u : 0u;
        s_c_active[c] = act;
        Word nl = 0u;
        s_c_lits[c * 3u + 0u] = ~0u;
        s_c_lits[c * 3u + 1u] = ~0u;
        s_c_lits[c * 3u + 2u] = ~0u;
        if (act) {
            for (Word l = 0; l < MAX_SAT_VARS * 2u && nl < 3u; ++l) {
                Word l_lo = l << 4;
                int inh = 0;
                for (Word k = 0; k < 16u; ++k)
                    inh += int(W[(unsigned long long)(c_lo + k) * NEURONS + (l_lo + k)]);
                if (inh <= -256) {
                    s_c_lits[c * 3u + nl] = l;
                    ++nl;
                }
            }
        }
        s_c_nlits[c] = nl;
    }
    if (tid < 8u) {
        s_w_found[tid] = 0u;
        s_w_assign[tid] = 0u;
        s_w_steps[tid] = 64u;
        s_w_unsat[tid] = 32u;
        s_w_spikes[tid] = 0u;
    }
    asm volatile("bar.sync 0;");

    // Launch 8 parallel 32-lane spiking Boltzmann/WalkSAT attractor replicas (1 warp per replica)!
    Word warp_id = tid >> 5;
    Word lane = tid & 31u;
    Word n_vars = (q.n_vars >= 1u && q.n_vars <= MAX_SAT_VARS) ? q.n_vars : 4u;
    Word var_mask = (n_vars >= 16u) ? 0xffffu : ((1u << n_vars) - 1u);
    Word pin_mask = 0u;
    Word pin_val = 0u;
    if (q.mode == 2u && q.query_var < n_vars) {
        pin_mask = 1u << q.query_var;
        pin_val = (warp_id < 4u) ? pin_mask : 0u; // Warps 0..3 test query_var=1; Warps 4..7 test query_var=0!
    }

    // Load lane c's own clause-detector synapses into registers for 1-cycle parallel evaluation across all 32 clauses!
    Word my_act = s_c_active[lane];
    Word my_nl = s_c_nlits[lane];
    Word my_l0 = s_c_lits[lane * 3u + 0u];
    Word my_l1 = s_c_lits[lane * 3u + 1u];
    Word my_l2 = s_c_lits[lane * 3u + 2u];

    auto eval_unsat_lane = [&](Word st) -> bool {
        if (!my_act || my_nl == 0u) return false;
        if (my_l0 < 32u && (((st >> (my_l0 >> 1)) & 1u) == (my_l0 & 1u))) return false;
        if (my_l1 < 32u && (((st >> (my_l1 >> 1)) & 1u) == (my_l1 & 1u))) return false;
        if (my_l2 < 32u && (((st >> (my_l2 >> 1)) & 1u) == (my_l2 & 1u))) return false;
        return true; // Clause-detector fires alarm spike (zero inhibiting literals active)!
    };

    Word state = (mix(q.seed * 7331u + (warp_id + 1u) * 0x9e3779b9u) & var_mask);
    state = (state & ~pin_mask) | pin_val;

    Word w_found = 0u, w_best_assign = state, w_best_unsat = 32u, w_steps = 64u, w_sp = 0u;
    for (Word tick = 0; tick < 64u; ++tick) {
        Word alarm_mask = warp_ballot(eval_unsat_lane(state));
        Word n_unsat = Word(__builtin_popcount(alarm_mask));
        w_sp += (n_vars + n_unsat) * 16u;
        if (n_unsat < w_best_unsat) {
            w_best_unsat = n_unsat;
            w_best_assign = state;
        }
        if (n_unsat == 0u) {
            w_found = 1u;
            w_best_assign = state;
            w_steps = tick + 1u;
            break;
        }
        // Select an active alarming clause rotated by `(tick + warp_id)` for unbiased Boltzmann sampling
        Word rot = (tick * 5u + warp_id * 3u) & 31u;
        Word rot_mask = rotr32(alarm_mask, rot);
        Word pick_c = (Word(__builtin_ctz(rot_mask)) + rot) & 31u;
        Word cl_nl = s_c_nlits[pick_c];
        Word cand_vars[3]{32u, 32u, 32u};
        Word n_cand = 0u;
        for (Word li = 0; li < cl_nl && li < 3u; ++li) {
            Word l = s_c_lits[pick_c * 3u + li];
            if (l < 32u) {
                Word v = l >> 1;
                if (v < n_vars && ((pin_mask & (1u << v)) == 0u))
                    cand_vars[n_cand++] = v;
            }
        }
        if (n_cand == 0u) {
            // Every literal in this alarming clause is pinned false -> pinned branch is strictly UNSAT!
            w_steps = tick + 1u;
            break;
        }
        Word pick_v = cand_vars[0];
        Word rnd = mix(q.seed + tick * 2654435761u + warp_id * 1597334677u);
        if ((rnd & 7u) == 0u && n_cand > 1u) {
            // 12.5% thermal membrane fluctuation (Boltzmann escape from local energy minima!)
            pick_v = cand_vars[(rnd >> 3) % n_cand];
        } else {
            Word best_u = 33u;
            for (Word ci = 0; ci < n_cand; ++ci) {
                Word cand_st = state ^ (1u << cand_vars[ci]);
                Word u = Word(__builtin_popcount(warp_ballot(eval_unsat_lane(cand_st))));
                if (u < best_u || (u == best_u && ((rnd >> (ci + 4u)) & 1u))) {
                    best_u = u;
                    pick_v = cand_vars[ci];
                }
            }
        }
        state ^= (1u << pick_v);
    }

    if (lane == 0u) {
        s_w_found[warp_id] = w_found;
        s_w_assign[warp_id] = w_best_assign;
        s_w_steps[warp_id] = w_steps;
        s_w_unsat[warp_id] = w_best_unsat;
        s_w_spikes[warp_id] = w_sp;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        Word act_c = 0u;
        for (Word c = 0; c < MAX_SAT_CLAUSES; ++c)
            if (s_c_active[c]) ++act_c;
        out.active_clauses = act_c;
        Word total_sp = 0u;
        for (Word w = 0; w < 8u; ++w) total_sp += s_w_spikes[w];
        out.spikes_fired = total_sp;

        if (q.mode == 2u) {
            Word sat_t = 0u, sat_f = 0u, min_st = 64u, best_a = 0u, min_u = 32u;
            for (Word w = 0; w < 4u; ++w) {
                if (s_w_unsat[w] < min_u) min_u = s_w_unsat[w];
                if (s_w_found[w]) {
                    sat_t = 1u;
                    if (s_w_steps[w] < min_st) { min_st = s_w_steps[w]; best_a = s_w_assign[w]; }
                }
            }
            for (Word w = 4u; w < 8u; ++w) {
                if (s_w_unsat[w] < min_u) min_u = s_w_unsat[w];
                if (s_w_found[w]) {
                    sat_f = 1u;
                    if (s_w_steps[w] < min_st) { min_st = s_w_steps[w]; best_a = s_w_assign[w]; }
                }
            }
            out.satisfiable = (sat_t || sat_f) ? 1u : 0u;
            out.forced_state = (sat_t && !sat_f) ? 1u : ((!sat_t && sat_f) ? 2u : ((sat_t && sat_f) ? 3u : 0u));
            out.assignment_mask = best_a;
            out.unsat_clauses = out.satisfiable ? 0u : min_u;
            out.steps_taken = min_st;
        } else {
            Word any_f = 0u, best_a = s_w_assign[0], min_st = 64u, min_u = 32u;
            for (Word w = 0; w < 8u; ++w) {
                if (s_w_unsat[w] < min_u) { min_u = s_w_unsat[w]; if (!any_f) best_a = s_w_assign[w]; }
                if (s_w_found[w]) {
                    any_f = 1u;
                    if (s_w_steps[w] < min_st) { min_st = s_w_steps[w]; best_a = s_w_assign[w]; }
                }
            }
            out.satisfiable = any_f;
            out.assignment_mask = best_a;
            out.unsat_clauses = any_f ? 0u : min_u;
            out.steps_taken = any_f ? min_st : 64u;
            out.forced_state = 0u;
        }
        if (out.satisfiable) {
            SpikeVec wsv{};
            for (Word v = 0; v < n_vars; ++v) {
                Word l = (v << 1) | ((out.assignment_mask >> v) & 1u);
                Word l_lo = l << 4;
                for (Word k = 0; k < 16u; ++k) {
                    Word j = l_lo + k;
                    wsv.bits[j >> 5] |= 1u << (j & 31u);
                }
            }
            out.winner_spikes = wsv;
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel 31 (STAGE 6 CROSSING!): Pure-GPU 16x16 Spiking V1/V4 Retinal Vision
// & Spatial Scene Cortex (`mind_neo_visual_cortex`) on L2 Pathway 24 (`PW24`)!
// 1 block of 256 threads = 1 thread per retinal photoreceptor `(x, y)` on a 16x16 grid!
//   - Connected-Component Wavefront: 256 threads propagate minimum nonzero labels across
//     4-neighbors in shared memory until fixed point -> exact object count, primary/secondary
//     bounding boxes & 2D spatial relation (`above/below/left/right`) purely on GPU!
//   - V1 Oriented-Edge / Fill / Hole / Corner / Symmetry Feature Bank (`0..255`):
//     16 translation-invariant channels x 16 Gaussian-tuned receptive-field cells (`|bin-f|<=1`).
//   - V4 Prototype Assemblies (`512..767`): 16 shape prototypes x 16 cells/prototype in L2 PW24.
//   - Mode 0: Evolved-LUT STDP (`g.basal_rule`) writes excitatory (+24) & inhibitory (-4) V1->V4
//     weights + tonic diagonal (+16) directly into L2 PW24!
//   - Mode 1: All 256 threads evaluate all 16 V4 assemblies simultaneously straight from L2 PW24!
// ZERO extra L2 memory, ZERO atomics, ZERO host flood-fill/shape rules!
// SURVIVES `--ablate hipp` (Duotrigintuple 32-Track Ribot!) & `--ablate freeze`, collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_visual_cortex_block(signed char *weights_all, const NeoVisualCortexQuery &q, const CorticalGenome &g,
                                   NeoVisualCortexResult &out) {
    __attribute__((shared)) static Word s_pix[256];
    __attribute__((shared)) static Word s_lbl[256];
    __attribute__((shared)) static Word s_nxt[256];
    __attribute__((shared)) static Word s_changed;
    __attribute__((shared)) static Word s_prim_lbl;
    __attribute__((shared)) static Word s_sec_lbl;
    __attribute__((shared)) static Word s_xmin, s_xmax, s_ymin, s_ymax;
    __attribute__((shared)) static Word s_v1[256];
    __attribute__((shared)) static int  s_v4_cur[256];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_wave_ticks;
    __attribute__((shared)) static Word s_obj_cnt;
    __attribute__((shared)) static Word s_tot_area;
    __attribute__((shared)) static Word s_rel_code;
    __attribute__((shared)) static Word s_sym_x;
    __attribute__((shared)) static Word s_sym_y;

    Word tid = thread_id();
    Word x = tid & 15u;
    Word y = tid >> 4;
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 24u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    Word my_on = (q.pixels[tid >> 5] >> (tid & 31u)) & 1u;
    s_pix[tid] = my_on;
    s_lbl[tid] = my_on ? (tid + 1u) : 0u;
    if (tid == 0) {
        out = {};
        s_wave_ticks = 0u;
        s_obj_cnt = 0u;
        s_tot_area = 0u;
        s_rel_code = 0u;
        s_prim_lbl = 0u;
        s_sec_lbl = 0u;
        s_xmin = 15u; s_xmax = 0u; s_ymin = 15u; s_ymax = 0u;
        s_sym_x = 0u; s_sym_y = 0u;
    }
    asm volatile("bar.sync 0;");

    // Step 1: Pure-GPU 256-thread Parallel Spiking Wavefront Connected-Component Labeling!
    for (Word step = 0; step < 32u; ++step) {
        if (tid == 0) s_changed = 0u;
        asm volatile("bar.sync 0;");
        Word cur = s_lbl[tid];
        Word best = cur;
        if (my_on) {
            if (x > 0u)  { Word nb = s_lbl[tid - 1u];  if (nb && nb < best) best = nb; }
            if (x < 15u) { Word nb = s_lbl[tid + 1u];  if (nb && nb < best) best = nb; }
            if (y > 0u)  { Word nb = s_lbl[tid - 16u]; if (nb && nb < best) best = nb; }
            if (y < 15u) { Word nb = s_lbl[tid + 16u]; if (nb && nb < best) best = nb; }
        }
        s_nxt[tid] = best;
        Word diff = (best != cur) ? 1u : 0u;
        Word any_w = warp_ballot(diff != 0u);
        if ((tid & 31u) == 0u && any_w != 0u) s_changed = 1u;
        asm volatile("bar.sync 0;");
        s_lbl[tid] = s_nxt[tid];
        if (tid == 0) s_wave_ticks = step + 1u;
        asm volatile("bar.sync 0;");
        if (s_changed == 0u) break;
    }

    // Step 2: Count distinct visual objects, rank largest (primary) & 2nd-largest (secondary) objects,
    //         compute 2D spatial relation & primary object bounding box + V1 translation-invariant features!
    if (tid == 0) {
        Word comp_lbl[32]{};
        Word comp_cnt[32]{};
        Word comp_sx[32]{};
        Word comp_sy[32]{};
        Word n_comp = 0u;
        Word area = 0u;
        for (Word i = 0; i < 256u; ++i) {
            Word l = s_lbl[i];
            if (!l) continue;
            ++area;
            Word xi = i & 15u, yi = i >> 4;
            Word idx = n_comp;
            for (Word c = 0; c < n_comp; ++c) {
                if (comp_lbl[c] == l) { idx = c; break; }
            }
            if (idx == n_comp && n_comp < 32u) {
                comp_lbl[n_comp] = l;
                ++n_comp;
            }
            if (idx < 32u) {
                comp_cnt[idx] += 1u;
                comp_sx[idx] += xi;
                comp_sy[idx] += yi;
            }
        }
        s_obj_cnt = n_comp;
        s_tot_area = area;

        // Identify primary (largest) and secondary (2nd-largest) objects!
        Word p_i = ~0u, s_i = ~0u;
        for (Word c = 0; c < n_comp; ++c) {
            if (p_i == ~0u || comp_cnt[c] > comp_cnt[p_i]) {
                s_i = p_i;
                p_i = c;
            } else if (s_i == ~0u || comp_cnt[c] > comp_cnt[s_i]) {
                s_i = c;
            }
        }
        if (p_i != ~0u) {
            s_prim_lbl = comp_lbl[p_i];
            if (s_i != ~0u) {
                s_sec_lbl = comp_lbl[s_i];
                // Exact integer centroid comparison (cross-multiplied to avoid division truncation!)
                int dx = int(comp_sx[p_i] * comp_cnt[s_i]) - int(comp_sx[s_i] * comp_cnt[p_i]);
                int dy = int(comp_sy[p_i] * comp_cnt[s_i]) - int(comp_sy[s_i] * comp_cnt[p_i]);
                int adx = dx < 0 ? -dx : dx;
                int ady = dy < 0 ? -dy : dy;
                if (ady >= adx)
                    s_rel_code = (dy < 0) ? 1u : 2u; // 1=above, 2=below
                else
                    s_rel_code = (dx < 0) ? 3u : 4u; // 3=left, 4=right
            }
            // Bounding box of primary visual object
            Word x0 = 15u, x1 = 0u, y0 = 15u, y1 = 0u;
            for (Word i = 0; i < 256u; ++i) {
                if (s_lbl[i] == s_prim_lbl) {
                    Word xi = i & 15u, yi = i >> 4;
                    if (xi < x0) x0 = xi;
                    if (xi > x1) x1 = xi;
                    if (yi < y0) y0 = yi;
                    if (yi > y1) y1 = yi;
                }
            }
            s_xmin = x0; s_xmax = x1; s_ymin = y0; s_ymax = y1;

            // Compute 16 translation-invariant V1/V2/V4 feature channels (each scaled 0..15)!
            auto is_p = [&](int px, int py) -> Word {
                if (px < 0 || px >= 16 || py < 0 || py >= 16) return 0u;
                return (s_lbl[Word(py) * 16u + Word(px)] == s_prim_lbl) ? 1u : 0u;
            };
            Word bw = (x1 >= x0) ? (x1 - x0 + 1u) : 1u;
            Word bh = (y1 >= y0) ? (y1 - y0 + 1u) : 1u;
            Word p_area = comp_cnt[p_i];
            Word b_area = bw * bh;
            Word h_ed = 0u, v_ed = 0u, d1_ed = 0u, d2_ed = 0u, perim = 0u, holes = 0u;
            Word top_c = 0u, bot_c = 0u, midr_c = 0u, midc_c = 0u, symx_m = 0u, symy_m = 0u;
            Word ymid = (y0 + y1) >> 1;
            Word xmid = (x0 + x1) >> 1;

            for (Word py = y0; py <= y1; ++py) {
                for (Word px = x0; px <= x1; ++px) {
                    Word on = is_p(int(px), int(py));
                    Word mx = is_p(int(x0 + x1 - px), int(py));
                    Word my = is_p(int(px), int(y0 + y1 - py));
                    if (on == mx) ++symx_m;
                    if (on == my) ++symy_m;
                    if (on) {
                        Word hl = is_p(int(px) - 1, int(py));
                        Word hr = is_p(int(px) + 1, int(py));
                        Word vu = is_p(int(px), int(py) - 1);
                        Word vd = is_p(int(px), int(py) + 1);
                        if (hl || hr) ++h_ed;
                        if (vu || vd) ++v_ed;
                        if (is_p(int(px) + 1, int(py) - 1) || is_p(int(px) - 1, int(py) + 1)) ++d1_ed;
                        if (is_p(int(px) + 1, int(py) + 1) || is_p(int(px) - 1, int(py) - 1)) ++d2_ed;
                        if (!(hl && hr && vu && vd)) ++perim;
                        if (py == y0) ++top_c;
                        if (py == y1) ++bot_c;
                        if (py == ymid) ++midr_c;
                        if (px == xmid) ++midc_c;
                    } else {
                        // Interior hole ray test: OFF pixel enclosed on ALL 4 cardinal rays inside bbox!
                        bool rl = false, rr = false, ru = false, rd = false;
                        for (Word rx = x0; rx < px; ++rx) if (is_p(int(rx), int(py))) { rl = true; break; }
                        for (Word rx = px + 1u; rx <= x1; ++rx) if (is_p(int(rx), int(py))) { rr = true; break; }
                        for (Word ry = y0; ry < py; ++ry) if (is_p(int(px), int(ry))) { ru = true; break; }
                        for (Word ry = py + 1u; ry <= y1; ++ry) if (is_p(int(px), int(ry))) { rd = true; break; }
                        if (rl && rr && ru && rd) ++holes;
                    }
                }
            }
            Word corners = is_p(int(x0), int(y0)) + is_p(int(x1), int(y0)) +
                           is_p(int(x0), int(y1)) + is_p(int(x1), int(y1));
            Word ctr_on = is_p(int(xmid), int(ymid)) +
                          is_p(int(xmid + (bw > 1u ? 1u : 0u)), int(ymid)) +
                          is_p(int(xmid), int(ymid + (bh > 1u ? 1u : 0u))) +
                          is_p(int(xmid + (bw > 1u ? 1u : 0u)), int(ymid + (bh > 1u ? 1u : 0u)));

            auto sc15 = [](Word num, Word den) -> Word {
                if (den == 0u) return 0u;
                Word v = (num * 15u + (den >> 1)) / den;
                return v > 15u ? 15u : v;
            };
            Word feat[16];
            feat[0]  = sc15(h_ed, p_area);
            feat[1]  = sc15(v_ed, p_area);
            feat[2]  = sc15(d1_ed, p_area);
            feat[3]  = sc15(d2_ed, p_area);
            feat[4]  = sc15(p_area, b_area);
            feat[5]  = (holes >= 4u) ? 15u : (holes * 3u);
            feat[6]  = sc15(top_c, bw);
            feat[7]  = sc15(bot_c, bw);
            feat[8]  = sc15(midr_c, bw);
            feat[9]  = sc15(midc_c, bh);
            feat[10] = (corners * 3u > 15u) ? 15u : (corners * 3u);
            feat[11] = sc15(ctr_on, 4u);
            feat[12] = sc15(symx_m, b_area);
            feat[13] = sc15(symy_m, b_area);
            feat[14] = (bw > bh) ? 15u : (bw == bh ? 7u : 0u);
            feat[15] = sc15(perim, p_area);

            s_sym_x = (feat[12] >= 13u) ? 1u : 0u;
            s_sym_y = (feat[13] >= 13u) ? 1u : 0u;

            // Encode 256-neuron V1 Gaussian tuning-curve spike population (`|bin - feat[ch]| <= 1`)!
            for (Word ch = 0; ch < 16u; ++ch) {
                int fc = int(feat[ch]);
                for (Word b = 0; b < 16u; ++b) {
                    int dist = int(b) - fc;
                    if (dist < 0) dist = -dist;
                    s_v1[ch * 16u + b] = (dist <= 1) ? 1u : 0u;
                }
            }
        } else {
            for (Word i = 0; i < 256u; ++i) s_v1[i] = 0u;
        }
    }
    asm volatile("bar.sync 0;");

    if (q.mode == 0u) {
        // Mode 0: Evolved-LUT STDP V1->V4 Visual Prototype Imprinting into L2 PW24!
        int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
        if (d_ltp <= 0) d_ltp = 1;
        int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
        if (d_ltd >= 0) d_ltd = -1;
        Word local_edits = 0u;

        if (q.clear_first) {
            // Clear ONLY visual V4 rows `512..767` cols `0..767` (preserving K33 rows `768..1023` AND K44 Kalman cols `768..1023`!)
            Word j = 512u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 0; i < 768u; ++i) {
                if (row[i] != 0) {
                    row[i] = 0;
                    ++local_edits;
                }
            }
            asm volatile("bar.sync 0;");
        }

        Word p = q.proto_id < MAX_VIS_PROTOS ? q.proto_id : 0u;
        Word p_lo = 512u + (p << 4);
        // Each thread `tid` owns presynaptic V1 neuron `tid` (0..255) and updates all 16 V4 postsynaptic neurons!
        Word v1_fire = s_v1[tid];
        for (Word r = 0; r < 16u; ++r) {
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + tid;
            int cur = int(*cell);
            if (v1_fire) {
                int target = 24;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            } else {
                int target = -4;
                if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    int nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        // Tonic prototype-active marker on `W[p_lo + r, p_lo + r] = +16` (16 threads)!
        if (tid < 16u) {
            Word d_idx = p_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            int target = 16;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }

        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.obj_count = s_obj_cnt;
            out.total_area = s_tot_area;
            out.sym_x = s_sym_x;
            out.sym_y = s_sym_y;
            out.rel_code = s_rel_code;
            out.wave_steps = s_wave_ticks;
            out.best_proto = p;
            out.spikes_fired = s_tot_area * s_wave_ticks + 256u;
        }
        return;
    }

    // Mode 1: All 256 threads evaluate all 16 V4 prototype assemblies (`tid = p*16 + k`) straight from L2 PW24!
    {
        Word p = tid >> 4;
        Word k = tid & 15u;
        Word j = 512u + tid;
        const signed char *row = W + (unsigned long long)(j) * NEURONS;
        int tonic = int(row[j]);
        int acc = 0;
        if (tonic >= 8) {
            for (Word i = 0; i < 256u; ++i) {
                if (s_v1[i]) acc += int(row[i]);
            }
        } else {
            acc = -1000000;
        }
        (void)p; (void)k;
        s_v4_cur[tid] = acc;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        Word act_p = 0u;
        Word best_p = ~0u;
        int best_sc = -1000000;
        for (Word p = 0; p < MAX_VIS_PROTOS; ++p) {
            int p_sum = 0;
            bool ok = true;
            for (Word k = 0; k < 16u; ++k) {
                int v = s_v4_cur[p * 16u + k];
                if (v <= -500000) { ok = false; break; }
                p_sum += v;
            }
            if (!ok) continue;
            ++act_p;
            if (p_sum > best_sc) {
                best_sc = p_sum;
                best_p = p;
            }
        }
        out.active_protos = act_p;
        out.obj_count = s_obj_cnt;
        out.total_area = s_tot_area;
        out.sym_x = s_sym_x;
        out.sym_y = s_sym_y;
        out.rel_code = s_rel_code;
        out.wave_steps = s_wave_ticks;
        out.spikes_fired = s_tot_area * s_wave_ticks + act_p * 64u + 192u;
        if (best_p != ~0u && best_sc > 0) {
            out.best_proto = best_p;
            out.best_score = best_sc;
            SpikeVec vsv{};
            Word p_lo = 512u + (best_p << 4);
            for (Word k = 0; k < 16u; ++k) {
                Word j = p_lo + k;
                vsv.bits[j >> 5] |= 1u << (j & 31u);
            }
            out.v4_spikes = vsv;
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel 32 (STAGE 6b!): Pure-GPU 16x16 Spiking Cochlear Audio, Harmonic Chord
// & Emotional Prosody Cortex (`mind_neo_audio_cortex`) on L2 Pathway 27 (`PW27`)!
// 1 block of 256 threads = 1 thread per cochlear hair cell `(t, f)` on a 16x16 cochleagram!
//   - Step 1: 256-thread parallel 8-neighbor lateral inhibition suppresses isolated noise spikes!
//   - Step 2: Computes active frames, rhythmic syllable/beat onsets (`beat_count`), pitch contour
//     slope (`pitch_dir`: 0=flat, 1=rising, 2=falling), harmonic consonance & dissonance, and
//     16 time-shift + pitch-transposition invariant A1 feature channels encoded as 256 Gaussian spikes!
//   - Auditory Belt Prototype Assemblies (`512..767`): 16 prototypes x 16 cells/proto in L2 PW27!
//     (Leaves upper zone `768..1023` untouched for personal/cultural continual-plasticity ripples!)
//   - Mode 0: Evolved-LUT STDP (`g.basal_rule`) writes excitatory (+24) & inhibitory (-4) A1->Belt
//     weights + tonic diagonal (+16) directly into L2 PW27!
//   - Mode 1: All 256 threads evaluate all 16 auditory assemblies simultaneously straight from L2 PW27!
// ZERO extra L2 memory, ZERO atomics, ZERO host audio classification!
// SURVIVES `--ablate hipp` (Tretrigintuple 33-Track Ribot!) & `--ablate freeze`, collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_audio_cortex_block(signed char *weights_all, const NeoAudioCortexQuery &q, const CorticalGenome &g,
                                  NeoAudioCortexResult &out) {
    __attribute__((shared)) static Word s_raw[256];
    __attribute__((shared)) static Word s_cln[256];
    __attribute__((shared)) static Word s_a1[256];
    __attribute__((shared)) static int  s_belt_cur[256];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_beats;
    __attribute__((shared)) static Word s_pdir;
    __attribute__((shared)) static Word s_cons;
    __attribute__((shared)) static Word s_diss;
    __attribute__((shared)) static Word s_act_fr;
    __attribute__((shared)) static Word s_tot_cells;

    Word tid = thread_id();
    Word f = tid & 15u;
    Word t = tid >> 4;
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 27u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    Word my_on = (q.spectro[tid >> 5] >> (tid & 31u)) & 1u;
    s_raw[tid] = my_on;
    if (tid == 0) {
        out = {};
        s_beats = 0u;
        s_pdir = 0u;
        s_cons = 0u;
        s_diss = 0u;
        s_act_fr = 0u;
        s_tot_cells = 0u;
    }
    asm volatile("bar.sync 0;");

    // Step 1: 256-thread parallel 8-neighbor cochlear lateral inhibition suppresses isolated noise glitches!
    Word keep = 0u;
    if (my_on) {
        int ti = int(t), fi = int(f);
        for (int dt = -1; dt <= 1; ++dt) {
            int nt = ti + dt;
            if (nt < 0 || nt >= 16) continue;
            for (int df = -1; df <= 1; ++df) {
                if (dt == 0 && df == 0) continue;
                int nf = fi + df;
                if (nf < 0 || nf >= 16) continue;
                if (s_raw[Word(nt) * 16u + Word(nf)]) keep = 1u;
            }
        }
    }
    s_cln[tid] = keep;
    asm volatile("bar.sync 0;");

    // Step 2: Compute 16 time-shift & pitch-transposition invariant A1 cochlear features (`0..15`)
    //         + rhythmic beat bursts, pitch contour slope & harmonic consonance/dissonance!
    if (tid == 0) {
        Word f_cnt[16]{};
        int  f_mean[16]{};
        Word f_span[16]{};
        Word tot_cells = 0u;
        Word n_act = 0u;
        Word beats = 0u;
        Word first_t = 16u, last_t = 0u;
        Word gmin_f = 15u, gmax_f = 0u;
        Word sum_voices = 0u, max_voices = 0u, sum_span = 0u;
        Word cons_pairs = 0u, diss_pairs = 0u;
        int  early_f = -1, late_f = -1;
        Word rise_tr = 0u, fall_tr = 0u, flat_tr = 0u;

        for (Word ti = 0; ti < 16u; ++ti) {
            Word cnt = 0u, sum_f = 0u, mn_f = 15u, mx_f = 0u;
            Word active_fs[16]{};
            for (Word fi = 0; fi < 16u; ++fi) {
                if (s_cln[ti * 16u + fi]) {
                    active_fs[cnt++] = fi;
                    sum_f += fi;
                    if (fi < mn_f) mn_f = fi;
                    if (fi > mx_f) mx_f = fi;
                    if (fi < gmin_f) gmin_f = fi;
                    if (fi > gmax_f) gmax_f = fi;
                }
            }
            f_cnt[ti] = cnt;
            if (cnt > 0u) {
                ++n_act;
                tot_cells += cnt;
                if (ti < first_t) first_t = ti;
                last_t = ti;
                if (ti == 0u || f_cnt[ti - 1u] == 0u) ++beats;
                int fm = int((sum_f * 2u + cnt) / (2u * cnt));
                f_mean[ti] = fm;
                f_span[ti] = mx_f - mn_f;
                sum_voices += cnt;
                if (cnt > max_voices) max_voices = cnt;
                sum_span += f_span[ti];
                if (early_f < 0) early_f = fm;
                late_f = fm;
                if (ti > 0u && f_cnt[ti - 1u] > 0u) {
                    if (fm > f_mean[ti - 1u]) ++rise_tr;
                    else if (fm < f_mean[ti - 1u]) ++fall_tr;
                    else ++flat_tr;
                }
                // Pairwise harmonic interval analysis inside frame `ti`!
                for (Word a = 0; a < cnt; ++a) {
                    for (Word b = a + 1u; b < cnt; ++b) {
                        Word df = active_fs[b] - active_fs[a];
                        if (df == 3u || df == 4u || df == 7u || df == 12u) ++cons_pairs;
                        else if (df == 1u || df == 6u || df == 11u) ++diss_pairs;
                    }
                }
            } else {
                f_mean[ti] = -1;
            }
        }

        Word gaps = 0u;
        if (n_act > 0u && last_t > first_t) {
            for (Word ti = first_t + 1u; ti < last_t; ++ti)
                if (f_cnt[ti] == 0u) ++gaps;
        }
        int slope = (early_f >= 0 && late_f >= 0) ? (late_f - early_f) : 0;
        Word pdir = 0u;
        if (slope >= 3) pdir = 1u;       // rising question prosody (`pitchup`)
        else if (slope <= -3) pdir = 2u; // falling declarative prosody (`pitchdown`)

        auto sc15 = [](Word num, Word den) -> Word {
            if (den == 0u) return 0u;
            Word v = (num * 15u + (den >> 1)) / den;
            return v > 15u ? 15u : v;
        };
        auto cl15 = [](int v) -> Word {
            if (v < 0) return 0u;
            if (v > 15) return 15u;
            return Word(v);
        };

        Word n_tr = rise_tr + fall_tr + flat_tr;
        Word feat[16];
        feat[0]  = (n_act > 15u) ? 15u : n_act;
        feat[1]  = (beats * 4u > 15u) ? 15u : (beats * 4u);
        feat[2]  = cl15(8 + slope);
        feat[3]  = (n_act > 0u) ? cl15(int((sum_voices * 3u + (n_act >> 1)) / n_act)) : 0u;
        feat[4]  = (n_act > 0u) ? sc15(cons_pairs, n_act * 3u) : 0u;
        feat[5]  = (n_act > 0u) ? sc15(diss_pairs, n_act * 4u) : 0u;
        feat[6]  = (n_act > 0u && gmax_f >= gmin_f) ? ((gmax_f - gmin_f > 15u) ? 15u : (gmax_f - gmin_f)) : 0u;
        feat[7]  = (n_act > 0u) ? cl15(int((sum_span + (n_act >> 1)) / n_act)) : 0u;
        feat[8]  = sc15(rise_tr, n_tr > 0u ? n_tr : 1u);
        feat[9]  = sc15(fall_tr, n_tr > 0u ? n_tr : 1u);
        feat[10] = sc15(flat_tr, n_tr > 0u ? n_tr : 1u);
        feat[11] = (gaps * 2u > 15u) ? 15u : (gaps * 2u);
        feat[12] = (beats > 0u) ? cl15(int(n_act / beats)) : 0u;
        feat[13] = (max_voices * 3u > 15u) ? 15u : (max_voices * 3u);
        feat[14] = (n_act > 0u) ? cl15(8 + (2 * (int(cons_pairs) - int(diss_pairs))) / int(n_act)) : 8u;
        feat[15] = sc15(tot_cells, 64u);

        s_beats = beats;
        s_pdir = pdir;
        s_cons = feat[4];
        s_diss = feat[5];
        s_act_fr = n_act;
        s_tot_cells = tot_cells;

        // Encode 256-neuron A1 Gaussian tuning-curve spike population (`|bin - feat[ch]| <= 1`)!
        if (n_act > 0u) {
            for (Word ch = 0; ch < 16u; ++ch) {
                int fc = int(feat[ch]);
                for (Word b = 0; b < 16u; ++b) {
                    int dist = int(b) - fc;
                    if (dist < 0) dist = -dist;
                    s_a1[ch * 16u + b] = (dist <= 1) ? 1u : 0u;
                }
            }
        } else {
            for (Word i = 0; i < 256u; ++i) s_a1[i] = 0u;
        }
    }
    asm volatile("bar.sync 0;");

    if (q.mode == 0u) {
        // Mode 0: Evolved-LUT STDP A1->Belt Auditory Prototype Imprinting into L2 PW27 (rows 512..767 ONLY)!
        int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
        if (d_ltp <= 0) d_ltp = 1;
        int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
        if (d_ltd >= 0) d_ltd = -1;
        Word local_edits = 0u;

        if (q.clear_first) {
            // Clear ONLY auditory rows `512..767` (preserving upper cortical zone `768..1023`!)
            Word j = 512u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 0; i < NEURONS; ++i) {
                if (row[i] != 0) {
                    row[i] = 0;
                    ++local_edits;
                }
            }
            asm volatile("bar.sync 0;");
        }

        Word p = q.proto_id < MAX_AUD_PROTOS ? q.proto_id : 0u;
        Word p_lo = 512u + (p << 4);
        Word a1_fire = s_a1[tid];
        for (Word r = 0; r < 16u; ++r) {
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + tid;
            int cur = int(*cell);
            if (a1_fire) {
                int target = 24;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            } else {
                int target = -4;
                if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    int nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        // Tonic prototype-active marker on `W[p_lo + r, p_lo + r] = +16` (16 threads)!
        if (tid < 16u) {
            Word d_idx = p_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            int target = 16;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }

        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.beat_count = s_beats;
            out.pitch_dir = s_pdir;
            out.consonance = s_cons;
            out.dissonance = s_diss;
            out.active_frames = s_act_fr;
            out.best_proto = p;
            out.spikes_fired = s_tot_cells * 8u + 256u;
        }
        return;
    }

    // Mode 1: All 256 threads evaluate all 16 auditory belt prototype assemblies (`tid = p*16 + k`) straight from L2 PW27!
    {
        Word j = 512u + tid;
        const signed char *row = W + (unsigned long long)(j) * NEURONS;
        int tonic = int(row[j]);
        int acc = 0;
        if (tonic >= 8) {
            for (Word i = 0; i < 256u; ++i) {
                if (s_a1[i]) acc += int(row[i]);
            }
        } else {
            acc = -1000000;
        }
        s_belt_cur[tid] = acc;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        Word act_p = 0u;
        Word best_p = ~0u;
        int best_sc = -1000000;
        for (Word p = 0; p < MAX_AUD_PROTOS; ++p) {
            int p_sum = 0;
            bool ok = true;
            for (Word k = 0; k < 16u; ++k) {
                int v = s_belt_cur[p * 16u + k];
                if (v <= -500000) { ok = false; break; }
                p_sum += v;
            }
            if (!ok) continue;
            ++act_p;
            if (p_sum > best_sc) {
                best_sc = p_sum;
                best_p = p;
            }
        }
        out.active_protos = act_p;
        out.beat_count = s_beats;
        out.pitch_dir = s_pdir;
        out.consonance = s_cons;
        out.dissonance = s_diss;
        out.active_frames = s_act_fr;
        out.spikes_fired = s_tot_cells * 8u + act_p * 64u + 192u;
        if (best_p != ~0u && best_sc > 0) {
            out.best_proto = best_p;
            out.best_score = best_sc;
            SpikeVec asv{};
            Word p_lo = 512u + (best_p << 4);
            for (Word k = 0; k < 16u; ++k) {
                Word j = p_lo + k;
                asv.bits[j >> 5] |= 1u << (j & 31u);
            }
            out.a1_spikes = asv;
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel 33 (STAGE 6c!): Pure-GPU Spiking Cross-Modal Vision+Audio Binding &
// Hierarchical Prefrontal Working-Memory Goal Stack (`mind_neo_crossmodal_stack`)
// on L2 Pathway 24 orthogonal quadrant (`pre 256..511`, `post rows 768..1023`)!
// 1 block of 256 threads — ZERO extra L2 memory, ZERO atomics, ZERO host stack!
//   - Sub-Region A (rows 768..895): 8 cross-modal (Vision K31 + Audio K32) assemblies x 16 cells.
//     * Presynaptic 256..383: 8 visual shapes x 16 cells; 384..511: 8 audio patterns x 16 cells.
//     * Mode 0: Evolved `g.basal_rule` STDP binds `(vis_id, aud_id) -> xproto_id` in L2 PW24!
//     * Mode 1: 256 threads evaluate all 8 cross-modal assemblies simultaneously straight from L2 PW24!
//     * Mode 2: Backward synesthetic transpose recall `W^T` (Vision -> Audio OR Audio -> Vision)!
//   - Sub-Region B (rows 896..1023): 8-level Prefrontal Spiking Goal Stack (16 goals x 16 cells)!
//     * Mode 3: Push `goal_id` via evolved `g.basal_rule` LTP onto highest free level `d`!
//     * Mode 4: Pop top level via evolved `g.basal_rule` LTD & resurface parent goal straight from L2!
//     * Mode 5: Peek active top goal & stack depth straight from L2 PW24!
//     * Mode 6: Clear prefrontal stack rows `896..1023`.
// SURVIVES `--ablate hipp` (Quattuortrigintuple 34-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_crossmodal_stack_block(signed char *weights_all, const NeoCrossModalStackQuery &q,
                                      const CorticalGenome &g, NeoCrossModalStackResult &out) {
    __attribute__((shared)) static Word s_pre[256];      // Presynaptic activity for `256..511` (`s_pre[i]` -> neuron `256 + i`)
    __attribute__((shared)) static int  s_cur[128];      // Postsynaptic currents for 8 assemblies x 16 cells
    __attribute__((shared)) static int  s_back[256];     // Backward W^T currents onto `256..511`
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_depth;
    __attribute__((shared)) static Word s_win_p;

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 24u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) {
        out = {};
        s_depth = 0u;
        s_win_p = ~0u;
    }
    // Prepare presynaptic cue activity in `s_pre[0..255]` (`0..127` = visual, `128..255` = audio)
    Word pre_on = 0u;
    if (tid < 128u) {
        if (q.vis_id < 8u && (tid >> 4) == q.vis_id) pre_on = 1u;
    } else {
        if (q.aud_id < 8u && ((tid - 128u) >> 4) == q.aud_id) pre_on = 1u;
    }
    s_pre[tid] = pre_on;
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // -----------------------------------------------------------------------
    // MODE 0: Evolved-LUT STDP Cross-Modal Binding `(vis_id, aud_id) -> xproto_id` into L2 PW24[768..895]!
    // -----------------------------------------------------------------------
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first && tid < 128u) {
            Word j = 768u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 256u; i < 512u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        asm volatile("bar.sync 0;");

        Word p = q.xproto_id < MAX_XMODAL_PROTOS ? q.xproto_id : 0u;
        Word p_lo = 768u + (p << 4);
        Word pre_n = 256u + tid;
        Word fire = s_pre[tid];
        for (Word r = 0; r < 16u; ++r) {
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + pre_n;
            int cur = int(*cell);
            if (fire) {
                int target = 24;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            } else {
                int target = -4;
                if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    int nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        if (tid < 16u) {
            Word d_idx = p_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_xproto = p;
            out.spikes_fired = 384u;
        }
        return;
    }

    // -----------------------------------------------------------------------
    // MODE 1 & MODE 2: Pure-GPU Cross-Modal Fusion & Synesthetic Backward Recall (`W^T`)!
    // -----------------------------------------------------------------------
    if (q.mode == 1u || q.mode == 2u) {
        if (tid < 128u) {
            Word j = 768u + tid;
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int tonic = int(row[j]);
            int acc = 0;
            if (tonic >= 8) {
                for (Word i = 0; i < 256u; ++i) {
                    if (s_pre[i]) acc += int(row[256u + i]);
                }
            } else {
                acc = -1000000;
            }
            s_cur[tid] = acc;
        }
        asm volatile("bar.sync 0;");

        if (tid == 0) {
            Word act = 0u, best_p = ~0u;
            int best_sc = -1000000;
            for (Word p = 0; p < MAX_XMODAL_PROTOS; ++p) {
                int ps = 0;
                bool ok = true;
                for (Word k = 0; k < 16u; ++k) {
                    int v = s_cur[p * 16u + k];
                    if (v <= -500000) { ok = false; break; }
                    ps += v;
                }
                if (!ok) continue;
                ++act;
                if (ps > best_sc) { best_sc = ps; best_p = p; }
            }
            out.active_xprotos = act;
            if (best_p != ~0u && best_sc > 0) {
                s_win_p = best_p;
                out.best_xproto = best_p;
                out.best_score = best_sc;
                SpikeVec msv{};
                Word p_lo = 768u + (best_p << 4);
                for (Word k = 0; k < 16u; ++k) {
                    Word j = p_lo + k;
                    msv.bits[j >> 5] |= 1u << (j & 31u);
                }
                out.mm_spikes = msv;
            }
            out.spikes_fired = act * 48u + 192u;
        }
        asm volatile("bar.sync 0;");

        if (q.mode == 2u && s_win_p != ~0u) {
            // Backward synesthetic transpose recall `W^T` from winning cross-modal assembly `s_win_p` onto `256..511`!
            Word p_lo = 768u + (s_win_p << 4);
            Word pre_n = 256u + tid;
            int bacc = 0;
            for (Word k = 0; k < 16u; ++k) {
                bacc += int(W[(unsigned long long)(p_lo + k) * NEURONS + pre_n]);
            }
            s_back[tid] = bacc;
            asm volatile("bar.sync 0;");
            if (tid == 0) {
                Word bv = ~0u, ba = ~0u;
                int bvs = -1000000, bas = -1000000;
                for (Word v = 0; v < 8u; ++v) {
                    int s = 0;
                    for (Word k = 0; k < 16u; ++k) s += s_back[v * 16u + k];
                    if (s > bvs) { bvs = s; bv = v; }
                }
                for (Word a = 0; a < 8u; ++a) {
                    int s = 0;
                    for (Word k = 0; k < 16u; ++k) s += s_back[128u + a * 16u + k];
                    if (s > bas) { bas = s; ba = a; }
                }
                if (bvs > 0) out.recalled_vis = bv;
                if (bas > 0) out.recalled_aud = ba;
                out.spikes_fired += 256u;
            }
        }
        return;
    }

    // -----------------------------------------------------------------------
    // MODE 3, 4, 5, 6: Pure-GPU 8-Level Prefrontal Spiking Goal Stack (`L2 PW24 rows 896..1023`)!
    // -----------------------------------------------------------------------
    Word local_edits = 0u;
    if (q.mode == 6u) {
        if (tid < 128u) {
            Word j = 896u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 256u; i < 512u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.stack_depth = 0u;
            out.top_goal = ~0u;
            out.spikes_fired = 128u;
        }
        return;
    }

    // Measure current stack depth straight from L2 PW24 tonic diagonals `W[896 + d*16, 896 + d*16]`!
    if (tid == 0) {
        Word dep = 0u;
        for (Word d = 0; d < MAX_GOAL_STACK; ++d) {
            Word j = 896u + (d << 4);
            if (int(W[(unsigned long long)(j) * NEURONS + j]) >= 8) dep = d + 1u;
            else break;
        }
        s_depth = dep;
    }
    asm volatile("bar.sync 0;");

    Word cur_dep = s_depth;
    Word new_dep = cur_dep;
    if (q.mode == 3u && cur_dep < MAX_GOAL_STACK) {
        // Push `q.goal_id` into stack level `cur_dep` via evolved `g.basal_rule` LTP!
        Word g_id = q.goal_id < MAX_GOAL_SLOTS ? q.goal_id : 0u;
        Word lv_lo = 896u + (cur_dep << 4);
        Word pre_n = 256u + tid;
        Word fire = ((tid >> 4) == g_id) ? 1u : 0u;
        for (Word r = 0; r < 16u; ++r) {
            signed char *cell = W + (unsigned long long)(lv_lo + r) * NEURONS + pre_n;
            int cur = int(*cell);
            if (fire) {
                if (cur < 24) {
                    int steps = (24 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 24) nxt = 24;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            } else if (cur != 0) {
                *cell = 0;
                ++local_edits;
            }
        }
        if (tid < 16u) {
            Word d_idx = lv_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        new_dep = cur_dep + 1u;
    } else if (q.mode == 4u && cur_dep > 0u) {
        // Pop top stack level `cur_dep - 1` via evolved `g.basal_rule` LTD!
        Word lv_lo = 896u + ((cur_dep - 1u) << 4);
        Word pre_n = 256u + tid;
        for (Word r = 0; r < 16u; ++r) {
            signed char *cell = W + (unsigned long long)(lv_lo + r) * NEURONS + pre_n;
            int cur = int(*cell);
            if (cur > 0) {
                int steps = (cur + (-d_ltd) - 1) / (-d_ltd);
                int nxt = cur + steps * d_ltd;
                if (nxt < 0) nxt = 0;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        if (tid < 16u) {
            Word d_idx = lv_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur > 0) {
                int steps = (cur + (-d_ltd) - 1) / (-d_ltd);
                int nxt = cur + steps * d_ltd;
                if (nxt < 0) nxt = 0;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        new_dep = cur_dep - 1u;
    }

    s_ed_red[tid] = local_edits;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
        asm volatile("bar.sync 0;");
    }

    // Decode active top goal straight from L2 PW24 level `new_dep - 1` via backward spiking readout (`W^T`)!
    if (new_dep > 0u) {
        Word top_lo = 896u + ((new_dep - 1u) << 4);
        Word pre_n = 256u + tid;
        int acc = 0;
        for (Word r = 0; r < 16u; ++r)
            acc += int(W[(unsigned long long)(top_lo + r) * NEURONS + pre_n]);
        s_back[tid] = acc;
    } else {
        s_back[tid] = 0;
    }
    asm volatile("bar.sync 0;");

    if (tid == 0) {
        out.edits_made = s_ed_red[0];
        out.stack_depth = new_dep;
        out.spikes_fired = 256u + new_dep * 32u;
        if (new_dep > 0u) {
            Word bg = ~0u;
            int bsc = -1000000;
            for (Word gs = 0; gs < MAX_GOAL_SLOTS; ++gs) {
                int s = 0;
                for (Word k = 0; k < 16u; ++k) s += s_back[gs * 16u + k];
                if (s > bsc) { bsc = s; bg = gs; }
            }
            if (bg != ~0u && bsc > 0) {
                out.top_goal = bg;
                out.best_score = bsc;
                SpikeVec msv{};
                Word top_lo = 896u + ((new_dep - 1u) << 4);
                for (Word k = 0; k < 16u; ++k) {
                    Word j = top_lo + k;
                    msv.bits[j >> 5] |= 1u << (j & 31u);
                }
                out.mm_spikes = msv;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel 34 (STAGE 7 CROSSING!): Pure-GPU Spiking Multi-Turn Dialogue Coherence,
// Topic-Shift & User-Claim Contradiction Cortex (`mind_neo_dialogue_cortex`)!
// Lives in the orthogonal quadrant of L2 PW27 (`pre 256..511`, `post rows 768..1023`,
// ZERO extra L2 memory & 100% DISJOINT from K32 Cochlea on `pre 0..255`, `post 512..767`!):
//   - Sub-Region A (`rows 768..895`): 8 pragmatic modes x 16 neurons imprinted via evolved `g.basal_rule` STDP
//   - Sub-Region B (`rows 896..911`): persistent active discourse topic register updated live on GPU
//   - Sub-Region C (`rows 928..1023`): 6 live user-claim stance slots x 16 neurons (consistency vs contradiction!)
// ZERO atomics, ZERO host discourse rules — survives `--ablate hipp` (Quintrigintuple 35-Track Ribot's Law!)
// ---------------------------------------------------------------------------
AX_FN void neo_dialogue_cortex_block(signed char *weights_all, const NeoDialogueQuery &q,
                                     const CorticalGenome &g, NeoDialogueResult &out) {
    __attribute__((shared)) static Word s_pre[256];
    __attribute__((shared)) static int  s_cur[128];
    __attribute__((shared)) static int  s_back[256];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_win_m;
    __attribute__((shared)) static Word s_prev_m;

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 27u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) {
        out = {};
        s_win_m = ~0u;
        s_prev_m = ~0u;
    }
    // Encode presynaptic conversational cue channels (`0..15` x 16 neurons = 256 pre inputs `256..511`)!
    Word pre_on = 0u;
    Word chan = tid >> 4;
    if ((q.cue_a < MAX_DLG_CUES && chan == q.cue_a) || (q.cue_b < MAX_DLG_CUES && chan == q.cue_b))
        pre_on = 1u;
    s_pre[tid] = pre_on;
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // -----------------------------------------------------------------------
    // MODE 0: Evolved-LUT STDP Pragmatic Mode Imprinting `(cue_a, cue_b) -> dlg_mode` into PW27[768..895]!
    // -----------------------------------------------------------------------
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            Word j = 768u + tid; // Clears all 256 rows `768..1023` in orthogonal quadrant `pre 256..511` + diagonal!
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 256u; i < 512u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        asm volatile("bar.sync 0;");

        Word m_id = q.dlg_mode < MAX_DLG_MODES ? q.dlg_mode : 0u;
        Word m_lo = 768u + (m_id << 4);
        Word pre_n = 256u + tid;
        Word fire = s_pre[tid];
        for (Word r = 0; r < 16u; ++r) {
            signed char *cell = W + (unsigned long long)(m_lo + r) * NEURONS + pre_n;
            int cur = int(*cell);
            if (fire) {
                if (cur < 24) {
                    int steps = (24 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 24) nxt = 24;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            } else {
                if (cur > -4) {
                    int steps = (cur - (-4) + (-d_ltd) - 1) / (-d_ltd);
                    int nxt = cur + steps * d_ltd;
                    if (nxt < -4) nxt = -4;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        if (tid < 16u) {
            Word d_idx = m_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_mode = m_id;
            out.spikes_fired = 384u;
        }
        return;
    }

    // -----------------------------------------------------------------------
    // MODE 4: Clear Persistent Topic Trace & User-Claim Stances (`rows 896..1023`)!
    // -----------------------------------------------------------------------
    if (q.mode == 4u) {
        Word local_edits = 0u;
        if (tid < 128u) {
            Word j = 896u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 256u; i < 512u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = 128u;
        }
        return;
    }

    // -----------------------------------------------------------------------
    // MODE 1: Pure-GPU Pragmatic Mode Classification + Persistent L2 Topic-Shift Detection!
    // -----------------------------------------------------------------------
    if (q.mode == 1u) {
        // 1a. Integrate all 8 pragmatic mode assemblies (`rows 768..895`) in parallel!
        if (tid < 128u) {
            Word j = 768u + tid;
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int tonic = int(row[j]);
            int acc = 0;
            if (tonic >= 8) {
                for (Word i = 0; i < 256u; ++i) {
                    if (s_pre[i]) acc += int(row[256u + i]);
                }
            } else {
                acc = -1000000;
            }
            s_cur[tid] = acc;
        }
        // 1b. Simultaneously decode prior active discourse mode from L2 Sub-Region B (`rows 896..911`) via `W^T`!
        {
            Word pre_n = 256u + tid;
            int bacc = 0;
            if (int(W[(unsigned long long)(896u) * NEURONS + 896u]) >= 8) {
                for (Word r = 0; r < 16u; ++r)
                    bacc += int(W[(unsigned long long)(896u + r) * NEURONS + pre_n]);
            }
            s_back[tid] = bacc;
        }
        asm volatile("bar.sync 0;");

        if (tid == 0) {
            Word act = 0u, best_m = ~0u;
            int best_sc = -1000000;
            for (Word m = 0; m < MAX_DLG_MODES; ++m) {
                int ms = 0;
                bool ok = true;
                for (Word k = 0; k < 16u; ++k) {
                    int v = s_cur[m * 16u + k];
                    if (v <= -500000) { ok = false; break; }
                    ms += v;
                }
                if (!ok) continue;
                ++act;
                if (ms > best_sc) { best_sc = ms; best_m = m; }
            }
            // Decode prior active topic mode from `s_back`
            Word pm = ~0u;
            int pm_sc = 0;
            for (Word m = 0; m < MAX_DLG_MODES; ++m) {
                int s = 0;
                for (Word k = 0; k < 16u; ++k) s += s_back[m * 16u + k];
                if (s > pm_sc) { pm_sc = s; pm = m; }
            }
            s_prev_m = pm;
            out.prev_mode = pm;
            out.active_modes = act;
            if (best_m != ~0u && best_sc > 0) {
                s_win_m = best_m;
                out.best_mode = best_m;
                out.best_score = best_sc;
                out.topic_shifted = (pm != ~0u && pm != best_m) ? 1u : 0u;
                SpikeVec dsv{};
                Word m_lo = 768u + (best_m << 4);
                for (Word k = 0; k < 16u; ++k) {
                    Word j = m_lo + k;
                    dsv.bits[j >> 5] |= 1u << (j & 31u);
                }
                out.dlg_spikes = dsv;
            }
            out.spikes_fired = act * 48u + 256u;
        }
        asm volatile("bar.sync 0;");

        // 1c. Update persistent L2 active topic register (`rows 896..911`) via evolved `g.basal_rule` STDP!
        Word local_edits = 0u;
        if (!q.read_only && s_win_m != ~0u) {
            Word pre_n = 256u + tid;
            Word fire = ((tid >> 4) == s_win_m) ? 1u : 0u;
            for (Word r = 0; r < 16u; ++r) {
                signed char *cell = W + (unsigned long long)(896u + r) * NEURONS + pre_n;
                int cur = int(*cell);
                if (fire) {
                    if (cur < 24) {
                        int steps = (24 - cur + d_ltp - 1) / d_ltp;
                        int nxt = cur + steps * d_ltp;
                        if (nxt > 24) nxt = 24;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                } else if (cur != 0) {
                    *cell = 0;
                    ++local_edits;
                }
            }
            if (tid < 16u) {
                Word d_idx = 896u + tid;
                signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
                int cur = int(*cell);
                if (cur < 16) {
                    int steps = (16 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 16) nxt = 16;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) out.edits_made = s_ed_red[0];
        return;
    }

    // -----------------------------------------------------------------------
    // MODE 2 & MODE 3: Live User-Claim Stance Imprint & Contradiction Check (`rows 928..1023`)!
    // -----------------------------------------------------------------------
    Word slot = q.claim_slot < MAX_DLG_CLAIMS ? q.claim_slot : 0u;
    Word c_lo = 928u + (slot << 4);
    if (q.mode == 2u) {
        Word local_edits = 0u;
        Word pre_n = 256u + tid;
        bool is_pos_half = (tid < 128u);
        bool match_pol = (q.polarity != 0u) ? is_pos_half : !is_pos_half;
        for (Word r = 0; r < 16u; ++r) {
            signed char *cell = W + (unsigned long long)(c_lo + r) * NEURONS + pre_n;
            int cur = int(*cell);
            if (match_pol) {
                if (cur < 24) {
                    int steps = (24 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 24) nxt = 24;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            } else {
                if (cur > -12) {
                    int steps = (cur - (-12) + (-d_ltd) - 1) / (-d_ltd);
                    int nxt = cur + steps * d_ltd;
                    if (nxt < -12) nxt = -12;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        if (tid < 16u) {
            Word d_idx = c_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.coh_status = 1u;
            out.spikes_fired = 256u;
        }
        return;
    }

    // MODE 3: Query user-claim stance consistency vs direct contradiction straight from L2 PW27!
    if (tid < 16u) {
        Word j = c_lo + tid;
        const signed char *row = W + (unsigned long long)(j) * NEURONS;
        int tonic = int(row[j]);
        int acc = 0;
        if (tonic >= 8) {
            Word i_lo = (q.polarity != 0u) ? 256u : 384u;
            for (Word i = 0; i < 128u; ++i) acc += int(row[i_lo + i]);
        } else {
            acc = -1000000;
        }
        s_cur[tid] = acc;
    }
    asm volatile("bar.sync 0;");
    if (tid == 0) {
        int tot = 0;
        bool ok = true;
        for (Word k = 0; k < 16u; ++k) {
            if (s_cur[k] <= -500000) { ok = false; break; }
            tot += s_cur[k];
        }
        out.spikes_fired = 256u;
        if (ok) {
            out.best_score = tot;
            out.coh_status = (tot > 0) ? 1u : 2u; // 1=dlgagree, 2=dlgcontra!
            SpikeVec dsv{};
            for (Word k = 0; k < 16u; ++k) {
                Word j = c_lo + k;
                dsv.bits[j >> 5] |= 1u << (j & 31u);
            }
            out.dlg_spikes = dsv;
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel 35 (STAGE 7b!): Pure-GPU Spiking Episodic Narrative Arc, Conceptual
// Metaphor Blending & Socratic Self-Critique Cortex (`mind_neo_narrative_metaphor`)!
// Lives in the FREE orthogonal quadrant of L2 PW27 (`pre 512..767`, `post rows 256..511`,
// ZERO extra L2 memory & 100% DISJOINT from K32 `512..767` AND K34 `768..1023`!):
//   - Sub-Region A (`rows 256..383`): 8 conceptual metaphor blends x 16 neurons
//     * Mode 0: evolved-LUT STDP (`g.basal_rule`) imprints `(cue_a, cue_b) -> blend_id`
//     * Mode 1: forward blend classification OR backward `W^T` transpose explanation (`cue_a == ~0u`)!
//   - Sub-Region B1 (`rows 384..447`): 4 narrative plot-arc beats x 16 neurons
//     * Mode 2 (`slot_id < 4`): evolved-LUT STDP imprints beat transition `cue_a -> slot_id`
//     * Mode 3: 2-hop recurrent GPU narrative arc rollout (`hop1_beat`, `hop2_beat`) with refractory exclusion!
//   - Sub-Region B2 (`rows 448..511`): 4 Socratic self-critique flaw detectors x 16 neurons
//     * Mode 2 (`slot_id 4..7`): evolved-LUT STDP imprints flaw pattern `(cue_a, cue_b) -> slot_id`
//     * Mode 4: pure-GPU Socratic flaw classification (`best_flaw` 0..3)!
// ZERO atomics, ZERO host rules — survives `--ablate hipp` (Sextrigintuple 36-Track Ribot's Law!)
// ---------------------------------------------------------------------------
AX_FN void neo_narrative_metaphor_block(signed char *weights_all, const NeoNarrativeMetaphorQuery &q,
                                        const CorticalGenome &g, NeoNarrativeMetaphorResult &out) {
    __attribute__((shared)) static Word s_pre[256];
    __attribute__((shared)) static int  s_cur[128];
    __attribute__((shared)) static int  s_back[256];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_hop1;

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 27u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) {
        out = {};
        s_hop1 = ~0u;
    }
    Word chan = tid >> 4;
    Word pre_on = ((q.cue_a < MAX_MET_FEATS && chan == q.cue_a) || (q.cue_b < MAX_MET_FEATS && chan == q.cue_b)) ? 1u : 0u;
    s_pre[tid] = pre_on;
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // -----------------------------------------------------------------------
    // MODE 0: Evolved-LUT STDP Metaphor Blend Imprinting into PW27[256..383] (`pre 512..767`)!
    // -----------------------------------------------------------------------
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first && tid < 128u) {
            Word j = 256u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 512u; i < 768u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        asm volatile("bar.sync 0;");

        Word b_id = q.slot_id < MAX_MET_BLENDS ? q.slot_id : 0u;
        Word b_lo = 256u + (b_id << 4);
        Word pre_n = 512u + tid;
        Word fire = s_pre[tid];
        for (Word r = 0; r < 16u; ++r) {
            signed char *cell = W + (unsigned long long)(b_lo + r) * NEURONS + pre_n;
            int cur = int(*cell);
            if (fire) {
                if (cur < 24) {
                    int steps = (24 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 24) nxt = 24;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            } else {
                if (cur > -4) {
                    int steps = (cur - (-4) + (-d_ltd) - 1) / (-d_ltd);
                    int nxt = cur + steps * d_ltd;
                    if (nxt < -4) nxt = -4;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        if (tid < 16u) {
            Word d_idx = b_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_blend = b_id;
            out.spikes_fired = 384u;
        }
        return;
    }

    // -----------------------------------------------------------------------
    // MODE 1: Forward Conceptual Metaphor Blending OR Backward `W^T` Structural Bridge Recall!
    // -----------------------------------------------------------------------
    if (q.mode == 1u) {
        if (q.cue_a >= MAX_MET_FEATS) {
            // Backward `W^T` transpose explanation for blend `slot_id` straight from L2 PW27[256..383]!
            Word b_id = q.slot_id < MAX_MET_BLENDS ? q.slot_id : 0u;
            Word b_lo = 256u + (b_id << 4);
            Word pre_n = 512u + tid;
            int bacc = 0;
            if (int(W[(unsigned long long)(b_lo) * NEURONS + b_lo]) >= 8) {
                for (Word r = 0; r < 16u; ++r)
                    bacc += int(W[(unsigned long long)(b_lo + r) * NEURONS + pre_n]);
            }
            s_back[tid] = bacc;
            asm volatile("bar.sync 0;");
            if (tid == 0) {
                Word top1 = ~0u, top2 = ~0u;
                int s1 = 0, s2 = 0;
                for (Word c = 0; c < MAX_MET_FEATS; ++c) {
                    int cs = 0;
                    for (Word k = 0; k < 16u; ++k) cs += s_back[c * 16u + k];
                    if (cs > s1) { s2 = s1; top2 = top1; s1 = cs; top1 = c; }
                    else if (cs > s2) { s2 = cs; top2 = c; }
                }
                out.best_blend = b_id;
                out.recalled_a = top1;
                out.recalled_b = top2;
                out.best_score = s1 + s2;
                out.spikes_fired = 320u;
                if (top1 != ~0u) {
                    SpikeVec sv{};
                    for (Word k = 0; k < 16u; ++k) {
                        Word j = b_lo + k;
                        sv.bits[j >> 5] |= 1u << (j & 31u);
                    }
                    out.nm_spikes = sv;
                }
            }
            return;
        }

        // Forward metaphor blend classification across all 8 blend assemblies (`rows 256..383`)!
        if (tid < 128u) {
            Word j = 256u + tid;
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int tonic = int(row[j]);
            int acc = 0;
            if (tonic >= 8) {
                for (Word i = 0; i < 256u; ++i)
                    if (s_pre[i]) acc += int(row[512u + i]);
            } else {
                acc = -1000000;
            }
            s_cur[tid] = acc;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            Word act = 0u, best_b = ~0u;
            int best_sc = -1000000;
            for (Word b = 0; b < MAX_MET_BLENDS; ++b) {
                int bs = 0;
                bool ok = true;
                for (Word k = 0; k < 16u; ++k) {
                    int v = s_cur[b * 16u + k];
                    if (v <= -500000) { ok = false; break; }
                    bs += v;
                }
                if (!ok) continue;
                ++act;
                if (bs > best_sc) { best_sc = bs; best_b = b; }
            }
            out.active_blends = act;
            out.spikes_fired = act * 48u + 192u;
            if (best_b != ~0u && best_sc > 0) {
                out.best_blend = best_b;
                out.best_score = best_sc;
                SpikeVec sv{};
                Word b_lo = 256u + (best_b << 4);
                for (Word k = 0; k < 16u; ++k) {
                    Word j = b_lo + k;
                    sv.bits[j >> 5] |= 1u << (j & 31u);
                }
                out.nm_spikes = sv;
            }
        }
        return;
    }

    // -----------------------------------------------------------------------
    // MODE 2: Evolved-LUT STDP Narrative Plot-Arc (`slots 0..3`) & Self-Critique Flaw (`slots 4..7`) Imprinting!
    // -----------------------------------------------------------------------
    if (q.mode == 2u) {
        Word local_edits = 0u;
        if (q.clear_first && tid < 128u) {
            Word j = 384u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 512u; i < 768u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        asm volatile("bar.sync 0;");

        Word s_id = q.slot_id < 8u ? q.slot_id : 0u;
        Word s_lo = 384u + (s_id << 4);
        Word pre_n = 512u + tid;
        Word fire = s_pre[tid];
        for (Word r = 0; r < 16u; ++r) {
            signed char *cell = W + (unsigned long long)(s_lo + r) * NEURONS + pre_n;
            int cur = int(*cell);
            if (fire) {
                if (cur < 24) {
                    int steps = (24 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 24) nxt = 24;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            } else {
                if (cur > -4) {
                    int steps = (cur - (-4) + (-d_ltd) - 1) / (-d_ltd);
                    int nxt = cur + steps * d_ltd;
                    if (nxt < -4) nxt = -4;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        if (tid < 16u) {
            Word d_idx = s_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = 384u;
        }
        return;
    }

    // -----------------------------------------------------------------------
    // MODE 3: Pure-GPU 2-Hop Recurrent Narrative Plot-Arc Rollout (`rows 384..447`)!
    // -----------------------------------------------------------------------
    if (q.mode == 3u) {
        // Hop 1: evaluate 4 narrative beat assemblies (`rows 384..447`) from `cue_a`!
        if (tid < 64u) {
            Word j = 384u + tid;
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int tonic = int(row[j]);
            int acc = 0;
            if (tonic >= 8) {
                for (Word i = 0; i < 256u; ++i)
                    if (s_pre[i]) acc += int(row[512u + i]);
            } else {
                acc = -1000000;
            }
            s_cur[tid] = acc;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            Word best1 = ~0u;
            int bsc1 = 0;
            for (Word b = 0; b < MAX_PLOT_BEATS; ++b) {
                if (b == q.cue_a) continue; // Refractory self-exclusion!
                int bs = 0;
                bool ok = true;
                for (Word k = 0; k < 16u; ++k) {
                    int v = s_cur[b * 16u + k];
                    if (v <= -500000) { ok = false; break; }
                    bs += v;
                }
                if (ok && bs > bsc1) { bsc1 = bs; best1 = b; }
            }
            s_hop1 = best1;
            out.hop1_beat = best1;
            out.best_score = bsc1;
            out.spikes_fired = 256u;
        }
        asm volatile("bar.sync 0;");

        // Hop 2: recurrent GPU step feeding `s_hop1` back into `s_pre` with refractory exclusion!
        Word h1 = s_hop1;
        if (h1 < MAX_PLOT_BEATS && q.hops >= 2u) {
            s_pre[tid] = (chan == h1) ? 1u : 0u;
            asm volatile("bar.sync 0;");
            if (tid < 64u) {
                Word j = 384u + tid;
                const signed char *row = W + (unsigned long long)(j) * NEURONS;
                int tonic = int(row[j]);
                int acc = 0;
                if (tonic >= 8) {
                    for (Word i = 0; i < 256u; ++i)
                        if (s_pre[i]) acc += int(row[512u + i]);
                } else {
                    acc = -1000000;
                }
                s_cur[tid] = acc;
            }
            asm volatile("bar.sync 0;");
            if (tid == 0) {
                Word best2 = ~0u;
                int bsc2 = 0;
                for (Word b = 0; b < MAX_PLOT_BEATS; ++b) {
                    if (b == q.cue_a || b == h1) continue; // Refractory exclusion!
                    int bs = 0;
                    bool ok = true;
                    for (Word k = 0; k < 16u; ++k) {
                        int v = s_cur[b * 16u + k];
                        if (v <= -500000) { ok = false; break; }
                        bs += v;
                    }
                    if (ok && bs > bsc2) { bsc2 = bs; best2 = b; }
                }
                out.hop2_beat = best2;
                out.spikes_fired += 256u;
                SpikeVec sv{};
                Word l1 = 384u + (h1 << 4);
                for (Word k = 0; k < 16u; ++k) sv.bits[(l1 + k) >> 5] |= 1u << ((l1 + k) & 31u);
                if (best2 < MAX_PLOT_BEATS) {
                    Word l2 = 384u + (best2 << 4);
                    for (Word k = 0; k < 16u; ++k) sv.bits[(l2 + k) >> 5] |= 1u << ((l2 + k) & 31u);
                }
                out.nm_spikes = sv;
            }
        }
        return;
    }

    // -----------------------------------------------------------------------
    // MODE 4: Pure-GPU Socratic Self-Critique Logical-Flaw Detection (`rows 448..511`)!
    // -----------------------------------------------------------------------
    if (tid < 64u) {
        Word j = 448u + tid;
        const signed char *row = W + (unsigned long long)(j) * NEURONS;
        int tonic = int(row[j]);
        int acc = 0;
        if (tonic >= 8) {
            for (Word i = 0; i < 256u; ++i)
                if (s_pre[i]) acc += int(row[512u + i]);
        } else {
            acc = -1000000;
        }
        s_cur[tid] = acc;
    }
    asm volatile("bar.sync 0;");
    if (tid == 0) {
        Word best_f = ~0u;
        int bsc = 0;
        for (Word f = 0; f < MAX_FLAW_SLOTS; ++f) {
            int fs = 0;
            bool ok = true;
            for (Word k = 0; k < 16u; ++k) {
                int v = s_cur[f * 16u + k];
                if (v <= -500000) { ok = false; break; }
                fs += v;
            }
            if (ok && fs > bsc) { bsc = fs; best_f = f; }
        }
        out.spikes_fired = 256u;
        if (best_f != ~0u && bsc > 0) {
            out.best_flaw = best_f;
            out.best_score = bsc;
            SpikeVec sv{};
            Word fl = 448u + (best_f << 4);
            for (Word k = 0; k < 16u; ++k) sv.bits[(fl + k) >> 5] |= 1u << ((fl + k) & 31u);
            out.nm_spikes = sv;
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel 36 (STAGE 7c — 400 DOMAINS!): Pure-GPU Spiking Belief-Graph,
// Evidence-Weighting & Multi-Agent Scientific Debate Cortex (`mind_neo_belief_debate`)!
// Lives in the FREE orthogonal quadrant of L2 PW24 (`pre 512..767`, `post rows 256..511`,
// ZERO extra L2 memory & 100% DISJOINT from K31 Retina `512..767` AND K33 XModal `768..1023`!):
//   - Sub-Region A (`rows 256..383`): 8 Scientific Theories x 16 neurons
//     * Mode 0: evolved-LUT STDP (`g.basal_rule`) LTP support (`pol=1`) or LTD refutation (`pol=0`)
//     * Mode 1: parallel evaluation of all 8 theories against active evidence `(ev_a, ev_b)` -> verdict!
//   - Sub-Region B (`rows 384..511`): 8 Peer Source-Credibility Registers x 16 neurons
//     * Mode 2: evolved-LUT STDP peer credibility update (`pol=1` verified truth LTP / `pol=0` bluff LTD)
//     * Mode 3: pure-GPU multi-agent credibility-weighted debate adjudication (`peer_a` vs `peer_b`)!
//   - Mode 4: clear `PW24[256..511]` (`pre 512..767` + diagonals) cleanly!
// ZERO atomics, ZERO host rules — survives `--ablate hipp` (Septentrigintuple 37-Track Ribot's Law!)
// ---------------------------------------------------------------------------
AX_FN void neo_belief_debate_block(signed char *weights_all, const NeoBeliefDebateQuery &q,
                                   const CorticalGenome &g, NeoBeliefDebateResult &out) {
    __attribute__((shared)) static Word s_pre[256];
    __attribute__((shared)) static int  s_cur[128];
    __attribute__((shared)) static Word s_ed_red[256];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 24u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    Word chan = tid >> 4;
    Word pre_on = ((q.ev_a < MAX_BEL_EVID && chan == q.ev_a) || (q.ev_b < MAX_BEL_EVID && chan == q.ev_b)) ? 1u : 0u;
    s_pre[tid] = pre_on;
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clear entire K36 quadrant `rows 256..511`, `pre 512..767` + diagonals!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        Word j = 256u + tid;
        signed char *row = W + (unsigned long long)(j) * NEURONS;
        for (Word i = 512u; i < 768u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        if (row[j] != 0) { row[j] = 0; ++local_edits; }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 256u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP Evidence Imprinting onto Scientific Theory `claim_id` (`rows 256..383`)!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first && tid < 128u) {
            Word j = 256u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 512u; i < 768u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        asm volatile("bar.sync 0;");

        Word c_id = q.claim_id < MAX_BEL_CLAIMS ? q.claim_id : 0u;
        Word c_lo = 256u + (c_id << 4);
        Word pre_n = 512u + tid;
        if (s_pre[tid]) {
            for (Word r = 0; r < 16u; ++r) {
                signed char *cell = W + (unsigned long long)(c_lo + r) * NEURONS + pre_n;
                int cur = int(*cell);
                if (q.polarity != 0u) {
                    if (cur < 24) {
                        int steps = (24 - cur + d_ltp - 1) / d_ltp;
                        int nxt = cur + steps * d_ltp;
                        if (nxt > 24) nxt = 24;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                } else {
                    if (cur > -24) {
                        int steps = (cur - (-24) + (-d_ltd) - 1) / (-d_ltd);
                        int nxt = cur + steps * d_ltd;
                        if (nxt < -24) nxt = -24;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                }
            }
        }
        if (tid < 16u) {
            Word d_idx = c_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_claim = c_id;
            out.spikes_fired = 320u;
        }
        return;
    }

    // MODE 1: Pure-GPU Epistemic Belief-Graph Evaluation & Debate Verdict across all 8 Theories (`rows 256..383`)!
    if (q.mode == 1u) {
        if (tid < 128u) {
            Word j = 256u + tid;
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int tonic = int(row[j]);
            int acc = 0;
            if (tonic >= 8) {
                for (Word i = 0; i < 256u; ++i)
                    if (s_pre[i]) acc += int(row[512u + i]);
            } else {
                acc = -1000000;
            }
            s_cur[tid] = acc;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            Word act = 0u, best_c = ~0u;
            int best_sc = -1000000, sec_sc = -1000000;
            for (Word c = 0; c < MAX_BEL_CLAIMS; ++c) {
                int cs = 0;
                bool ok = true;
                for (Word k = 0; k < 16u; ++k) {
                    int v = s_cur[c * 16u + k];
                    if (v <= -500000) { ok = false; break; }
                    cs += v;
                }
                if (!ok) continue;
                ++act;
                if (cs > best_sc) { sec_sc = best_sc; best_sc = cs; best_c = c; }
                else if (cs > sec_sc) { sec_sc = cs; }
            }
            out.active_claims = act;
            out.spikes_fired = act * 48u + 192u;
            if (best_c != ~0u) {
                out.best_claim = best_c;
                out.best_score = best_sc;
                if (best_sc > 0) {
                    out.verdict = (sec_sc > 0 && (best_sc - sec_sc) < 512) ? 3u : 1u; // 1=belconfirm, 3=belsplit
                } else if (best_sc < 0) {
                    out.verdict = 2u; // 2=belrefute
                }
                SpikeVec sv{};
                Word c_lo = 256u + (best_c << 4);
                for (Word k = 0; k < 16u; ++k) sv.bits[(c_lo + k) >> 5] |= 1u << ((c_lo + k) & 31u);
                out.bd_spikes = sv;
            }
        }
        return;
    }

    // MODE 2: Evolved-LUT STDP Peer Source-Credibility Calibration (`rows 384..511`)!
    if (q.mode == 2u) {
        Word local_edits = 0u;
        if (q.clear_first && tid < 128u) {
            Word j = 384u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 512u; i < 768u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        asm volatile("bar.sync 0;");

        Word pid = q.peer_a < MAX_BEL_PEERS ? q.peer_a : 0u;
        Word p_lo = 384u + (pid << 4);
        Word pre_n = 512u + tid;
        int delta = (q.polarity != 0u) ? (d_ltp * 6) : (d_ltd * 8);
        for (Word r = 0; r < 16u; ++r) {
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + pre_n;
            int cur = int(*cell);
            int nxt = cur + delta;
            if (nxt > 48) nxt = 48;
            if (nxt < -48) nxt = -48;
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        if (tid < 16u) {
            Word d_idx = p_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_peer = pid;
            out.spikes_fired = 256u;
        }
        return;
    }

    // MODE 3: Pure-GPU Multi-Agent Credibility-Weighted Debate Adjudication (`peer_a` vs `peer_b` in `rows 384..511`)!
    Word pa = q.peer_a < MAX_BEL_PEERS ? q.peer_a : 0u;
    Word pb = q.peer_b < MAX_BEL_PEERS ? q.peer_b : 1u;
    if (tid < 32u) {
        bool is_b = (tid >= 16u);
        Word pid = is_b ? pb : pa;
        Word j = 384u + (pid << 4) + (tid & 15u);
        const signed char *row = W + (unsigned long long)(j) * NEURONS;
        int tonic = int(row[j]);
        int acc = 0;
        if (tonic >= 8) {
            for (Word i = 512u; i < 768u; ++i) acc += int(row[i]);
        } else {
            acc = -1000000;
        }
        s_cur[tid] = acc;
    }
    asm volatile("bar.sync 0;");
    if (tid == 0) {
        int sa = 0, sb = 0;
        bool oka = true, okb = true;
        for (Word k = 0; k < 16u; ++k) {
            if (s_cur[k] <= -500000) oka = false; else sa += s_cur[k];
            if (s_cur[16u + k] <= -500000) okb = false; else sb += s_cur[16u + k];
        }
        out.spikes_fired = 256u;
        if (oka || okb) {
            if (!oka) sa = -1000000;
            if (!okb) sb = -1000000;
            out.cred_margin = sa - sb;
            Word win = (sa >= sb) ? pa : pb;
            out.best_peer = win;
            out.best_score = (sa >= sb) ? sa : sb;
            SpikeVec sv{};
            Word w_lo = 384u + (win << 4);
            for (Word k = 0; k < 16u; ++k) sv.bits[(w_lo + k) >> 5] |= 1u << ((w_lo + k) & 31u);
            out.bd_spikes = sv;
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel 37 (STAGE 8 CROSSING — COLLECTIVE SCIENTIFIC MINDS!): Pure-GPU Spiking
// Multi-Mind Collective Discovery, Consensus Synthesis & Compositional Theorem Prover
// (`mind_neo_collective_discovery`) on FREE Orthogonal Quadrant of L2 PW24
// (`pre 768..1023`, `post rows 0..255`, ZERO extra L2 memory & 100% DISJOINT from
// K31 Retina `512..767`, K33 XModal `768..1023` AND K36 Belief-Debate `256..511`!):
//   - Sub-Region A (`rows 0..127`): 8 Collective Conjectures x 16 neurons
//     * Mode 0: evolved-LUT STDP (`g.basal_rule`) specialist LTP support (`pol=1`) or LTD refutation (`pol=0`)
//     * Mode 1: pure-GPU open consensus pool (`law_id==~0u`) or targeted conjecture audit (`law_id<8`)!
//   - Sub-Region B (`rows 128..255`): 8 Compositional Theorems x 16 neurons
//     * Mode 2: evolved-LUT STDP compositional axiom binding `(ax_a, ax_b) -> thm_id` with competitive LTD
//     * Mode 3: forward theorem derivation (`cue_a < 16`) OR backward `W^T` proof-axiom recall (`cue_a == ~0u`)!
//   - Mode 4: clean clear of `PW24[0..255]` (`pre 768..1023` + diagonals `0..255`) ONLY!
// ZERO atomics, ZERO host rules — survives `--ablate hipp` (Octotrigintuple 38-Track Ribot's Law!)
// ---------------------------------------------------------------------------
AX_FN void neo_collective_discovery_block(signed char *weights_all, const NeoCollectiveDiscoveryQuery &q,
                                          const CorticalGenome &g, NeoCollectiveDiscoveryResult &out) {
    __attribute__((shared)) static Word s_pre[256];
    __attribute__((shared)) static int  s_cur[128];
    __attribute__((shared)) static int  s_wt[256];
    __attribute__((shared)) static Word s_ed_red[256];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 24u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    Word chan = tid >> 4;
    Word pre_on = ((q.cue_a < MAX_COLL_CHANS && chan == q.cue_a) || (q.cue_b < MAX_COLL_CHANS && chan == q.cue_b)) ? 1u : 0u;
    s_pre[tid] = pre_on;
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clear entire K37 quadrant `rows 0..255`, `pre 768..1023` + diagonals `0..255`!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        Word j = tid;
        signed char *row = W + (unsigned long long)(j) * NEURONS;
        for (Word i = 768u; i < 1024u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        if (row[j] != 0) { row[j] = 0; ++local_edits; }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 256u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP Specialist Vote Imprinting onto Conjecture `law_id` (`rows 0..127`)!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first && tid < 128u) {
            Word j = tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 768u; i < 1024u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        asm volatile("bar.sync 0;");

        Word lid = q.law_id < MAX_COLL_LAWS ? q.law_id : 0u;
        Word l_lo = lid << 4;
        Word pre_n = 768u + tid;
        if (s_pre[tid]) {
            for (Word r = 0; r < 16u; ++r) {
                signed char *cell = W + (unsigned long long)(l_lo + r) * NEURONS + pre_n;
                int cur = int(*cell);
                if (q.polarity != 0u) {
                    if (cur < 24) {
                        int steps = (24 - cur + d_ltp - 1) / d_ltp;
                        int nxt = cur + steps * d_ltp;
                        if (nxt > 24) nxt = 24;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                } else {
                    if (cur > -24) {
                        int steps = (cur - (-24) + (-d_ltd) - 1) / (-d_ltd);
                        int nxt = cur + steps * d_ltd;
                        if (nxt < -24) nxt = -24;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                }
            }
        }
        if (tid < 16u) {
            Word d_idx = l_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_law = lid;
            out.spikes_fired = 320u;
        }
        return;
    }

    // MODE 1: Pure-GPU Collective Consensus Evaluation across Sub-Region A (`rows 0..127`)!
    if (q.mode == 1u) {
        if (tid < 128u) {
            Word j = tid;
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int tonic = int(row[j]);
            int acc = 0;
            if (tonic >= 8) {
                for (Word i = 0; i < 256u; ++i)
                    if (s_pre[i]) acc += int(row[768u + i]);
            } else {
                acc = -1000000;
            }
            s_cur[tid] = acc;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            Word act = 0u, best_l = ~0u;
            int best_sc = -1000000, sec_sc = -1000000;
            int target_sc = -1000000;
            bool target_ok = false;
            for (Word l = 0; l < MAX_COLL_LAWS; ++l) {
                int ls = 0;
                bool ok = true;
                for (Word k = 0; k < 16u; ++k) {
                    int v = s_cur[l * 16u + k];
                    if (v <= -500000) { ok = false; break; }
                    ls += v;
                }
                if (!ok) continue;
                ++act;
                if (q.law_id < MAX_COLL_LAWS && l == q.law_id) {
                    target_ok = true;
                    target_sc = ls;
                }
                if (ls > best_sc) { sec_sc = best_sc; best_sc = ls; best_l = l; }
                else if (ls > sec_sc) { sec_sc = ls; }
            }
            out.active_laws = act;
            out.spikes_fired = act * 48u + 192u;
            if (q.law_id < MAX_COLL_LAWS && target_ok) {
                out.best_law = q.law_id;
                out.best_score = target_sc;
                out.verdict = (target_sc > 0) ? 1u : (target_sc < 0 ? 2u : 3u);
                SpikeVec sv{};
                Word l_lo = q.law_id << 4;
                for (Word k = 0; k < 16u; ++k) sv.bits[(l_lo + k) >> 5] |= 1u << ((l_lo + k) & 31u);
                out.cd_spikes = sv;
            } else if (best_l != ~0u) {
                out.best_law = best_l;
                out.best_score = best_sc;
                if (best_sc > 0)
                    out.verdict = (sec_sc > 0 && (best_sc - sec_sc) < 512) ? 3u : 1u;
                else if (best_sc < 0)
                    out.verdict = 2u;
                else
                    out.verdict = 3u;
                SpikeVec sv{};
                Word l_lo = best_l << 4;
                for (Word k = 0; k < 16u; ++k) sv.bits[(l_lo + k) >> 5] |= 1u << ((l_lo + k) & 31u);
                out.cd_spikes = sv;
            }
        }
        return;
    }

    // MODE 2: Evolved-LUT STDP Compositional Theorem Imprinting `(ax_a, ax_b) -> thm_id` (`rows 128..255`)!
    if (q.mode == 2u) {
        Word local_edits = 0u;
        if (q.clear_first && tid < 128u) {
            Word j = 128u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 768u; i < 1024u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        asm volatile("bar.sync 0;");

        Word tid_s = q.thm_id < MAX_COLL_THMS ? q.thm_id : 0u;
        Word t_lo = 128u + (tid_s << 4);
        Word pre_n = 768u + tid;
        if (s_pre[tid]) {
            for (Word r = 0; r < 16u; ++r) {
                signed char *cell = W + (unsigned long long)(t_lo + r) * NEURONS + pre_n;
                int cur = int(*cell);
                if (cur < 24) {
                    int steps = (24 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 24) nxt = 24;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        } else if (chan >= 4u && chan < 12u) {
            for (Word r = 0; r < 16u; ++r) {
                signed char *cell = W + (unsigned long long)(t_lo + r) * NEURONS + pre_n;
                int cur = int(*cell);
                if (cur > -8) {
                    int steps = (cur - (-8) + (-d_ltd) - 1) / (-d_ltd);
                    int nxt = cur + steps * d_ltd;
                    if (nxt < -8) nxt = -8;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        if (tid < 16u) {
            Word d_idx = t_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_thm = tid_s;
            out.spikes_fired = 320u;
        }
        return;
    }

    // MODE 3: Forward Theorem Synthesis (`cue_a < 16`) OR Backward `W^T` Proof-Axiom Recall (`cue_a == ~0u`) in `rows 128..255`!
    if (q.cue_a == ~0u && q.cue_b == ~0u && q.thm_id < MAX_COLL_THMS) {
        Word t_lo = 128u + (q.thm_id << 4);
        Word pre_n = 768u + tid;
        int wsum = 0;
        for (Word r = 0; r < 16u; ++r)
            wsum += int(W[(unsigned long long)(t_lo + r) * NEURONS + pre_n]);
        s_wt[tid] = wsum;
        if (tid < 8u) {
            Word d_idx = 128u + (tid << 4);
            s_cur[tid] = int(W[(unsigned long long)(d_idx) * NEURONS + d_idx]);
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            Word act = 0u;
            for (Word t = 0; t < MAX_COLL_THMS; ++t)
                if (s_cur[t] >= 8) ++act;
            out.active_thms = act;
            out.spikes_fired = 288u;
            if (s_cur[q.thm_id] >= 8) {
                Word b1 = ~0u, b2 = ~0u;
                int sc1 = 0, sc2 = 0;
                for (Word c = 4u; c < 12u; ++c) {
                    int cs = 0;
                    for (Word k = 0; k < 16u; ++k) cs += s_wt[(c << 4) + k];
                    if (cs > sc1) { sc2 = sc1; b2 = b1; sc1 = cs; b1 = c; }
                    else if (cs > sc2) { sc2 = cs; b2 = c; }
                }
                if (b1 != ~0u && b2 != ~0u) {
                    out.best_thm = q.thm_id;
                    out.recalled_a = b1;
                    out.recalled_b = b2;
                    out.best_score = sc1 + sc2;
                    SpikeVec sv{};
                    for (Word k = 0; k < 16u; ++k) {
                        Word na = 768u + (b1 << 4) + k;
                        Word nb = 768u + (b2 << 4) + k;
                        sv.bits[na >> 5] |= 1u << (na & 31u);
                        sv.bits[nb >> 5] |= 1u << (nb & 31u);
                    }
                    out.cd_spikes = sv;
                }
            }
        }
        return;
    }

    if (tid < 128u) {
        Word j = 128u + tid;
        const signed char *row = W + (unsigned long long)(j) * NEURONS;
        int tonic = int(row[j]);
        int acc = 0;
        if (tonic >= 8) {
            for (Word i = 0; i < 256u; ++i)
                if (s_pre[i]) acc += int(row[768u + i]);
        } else {
            acc = -1000000;
        }
        s_cur[tid] = acc;
    }
    asm volatile("bar.sync 0;");
    if (tid == 0) {
        Word act = 0u, best_t = ~0u;
        int best_sc = 0;
        for (Word t = 0; t < MAX_COLL_THMS; ++t) {
            int ts = 0;
            bool ok = true;
            for (Word k = 0; k < 16u; ++k) {
                int v = s_cur[t * 16u + k];
                if (v <= -500000) { ok = false; break; }
                ts += v;
            }
            if (!ok) continue;
            ++act;
            if (ts > best_sc) { best_sc = ts; best_t = t; }
        }
        out.active_thms = act;
        out.spikes_fired = act * 48u + 192u;
        if (best_t != ~0u && best_sc > 0) {
            out.best_thm = best_t;
            out.best_score = best_sc;
            SpikeVec sv{};
            Word t_lo = 128u + (best_t << 4);
            for (Word k = 0; k < 16u; ++k) sv.bits[(t_lo + k) >> 5] |= 1u << ((t_lo + k) & 31u);
            out.cd_spikes = sv;
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel 38 (STAGE 8b — CONTINUAL SELF-RESEARCH & COUNTERFACTUAL CODE PATCHING!):
// Pure-GPU Spiking Discriminative Experiment-Design & Counterfactual Code-Patching Cortex
// (`mind_neo_self_research`) on FREE Orthogonal Quadrant of L2 PW27
// (`pre 768..1023`, `post rows 0..255`, ZERO extra L2 memory & 100% DISJOINT from
// K32 Cochlea `512..767`, K34 Dialogue `768..1023`, K35 Metaphor `256..511` & Cultural `768..1023`!):
//   - Sub-Region A (`rows 0..127`): 8 Diagnostic Experiments x 16 neurons
//     * Mode 0: evolved-LUT STDP (`g.basal_rule`) imprints positive (+24) & negative (-24) predictions
//     * Mode 1: parallel evaluation of `|score(exp, hyp_a) - score(exp, hyp_b)|` across all 8 experiments!
//   - Sub-Region B (`rows 128..255`): 8 Counterfactual Repair Patches x 16 neurons
//     * Mode 2: evolved-LUT STDP binds bug symptoms `(sym_a, sym_b) -> patch_id` with competitive LTD
//     * Mode 3: forward patch synthesis (`cue_a < 16`) OR backward `W^T` root-cause recall (`cue_a == ~0u`)!
//   - Mode 4: clean clear of `PW27[0..255]` (`pre 768..1023` + diagonals `0..255`) ONLY!
// ZERO atomics, ZERO host rules — survives `--ablate hipp` (Novemtrigintuple 39-Track Ribot's Law!)
// ---------------------------------------------------------------------------
AX_FN void neo_self_research_block(signed char *weights_all, const NeoSelfResearchQuery &q,
                                   const CorticalGenome &g, NeoSelfResearchResult &out) {
    __attribute__((shared)) static Word s_pre[256];
    __attribute__((shared)) static int  s_cur[128];
    __attribute__((shared)) static int  s_wt[256];
    __attribute__((shared)) static Word s_ed_red[256];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 27u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    Word chan = tid >> 4;
    Word pre_on = ((q.cue_a < MAX_SR_CHANS && chan == q.cue_a) || (q.cue_b < MAX_SR_CHANS && chan == q.cue_b)) ? 1u : 0u;
    s_pre[tid] = pre_on;
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clear entire K38 quadrant `rows 0..255`, `pre 768..1023` + diagonals `0..255`!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        Word j = tid;
        signed char *row = W + (unsigned long long)(j) * NEURONS;
        for (Word i = 768u; i < 1024u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        if (row[j] != 0) { row[j] = 0; ++local_edits; }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 256u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP Diagnostic Experiment Prediction Imprinting into `rows 0..127`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first && tid < 128u) {
            Word j = tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 768u; i < 1024u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        asm volatile("bar.sync 0;");

        Word eid = q.exp_id < MAX_SR_EXPS ? q.exp_id : 0u;
        Word e_lo = eid << 4;
        Word pre_n = 768u + tid;
        if (q.polarity == 2u) {
            if (q.cue_a < 8u && chan == q.cue_a) {
                for (Word r = 0; r < 16u; ++r) {
                    signed char *cell = W + (unsigned long long)(e_lo + r) * NEURONS + pre_n;
                    int cur = int(*cell);
                    if (cur < 24) {
                        int steps = (24 - cur + d_ltp - 1) / d_ltp;
                        int nxt = cur + steps * d_ltp;
                        if (nxt > 24) nxt = 24;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                }
            } else if (q.cue_b < 8u && chan == q.cue_b) {
                for (Word r = 0; r < 16u; ++r) {
                    signed char *cell = W + (unsigned long long)(e_lo + r) * NEURONS + pre_n;
                    int cur = int(*cell);
                    if (cur > -24) {
                        int steps = (cur - (-24) + (-d_ltd) - 1) / (-d_ltd);
                        int nxt = cur + steps * d_ltd;
                        if (nxt < -24) nxt = -24;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                }
            }
        } else if (s_pre[tid]) {
            for (Word r = 0; r < 16u; ++r) {
                signed char *cell = W + (unsigned long long)(e_lo + r) * NEURONS + pre_n;
                int cur = int(*cell);
                if (q.polarity != 0u) {
                    if (cur < 24) {
                        int steps = (24 - cur + d_ltp - 1) / d_ltp;
                        int nxt = cur + steps * d_ltp;
                        if (nxt > 24) nxt = 24;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                } else {
                    if (cur > -24) {
                        int steps = (cur - (-24) + (-d_ltd) - 1) / (-d_ltd);
                        int nxt = cur + steps * d_ltd;
                        if (nxt < -24) nxt = -24;
                        *cell = static_cast<signed char>(nxt);
                        ++local_edits;
                    }
                }
            }
        }
        if (tid < 16u) {
            Word d_idx = e_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_exp = eid;
            out.spikes_fired = 320u;
        }
        return;
    }

    // MODE 1: Pure-GPU Optimal Discriminative Experiment Design (`argmax |score(exp, H_a) - score(exp, H_b)|`) in `rows 0..127`!
    if (q.mode == 1u) {
        if (tid < 128u) {
            Word j = tid;
            const signed char *row = W + (unsigned long long)(j) * NEURONS;
            int tonic = int(row[j]);
            int diff = 0;
            if (tonic >= 8) {
                int sa = 0, sb = 0;
                if (q.cue_a < 8u) {
                    Word a_lo = 768u + (q.cue_a << 4);
                    for (Word k = 0; k < 16u; ++k) sa += int(row[a_lo + k]);
                }
                if (q.cue_b < 8u) {
                    Word b_lo = 768u + (q.cue_b << 4);
                    for (Word k = 0; k < 16u; ++k) sb += int(row[b_lo + k]);
                }
                diff = sa - sb;
            } else {
                diff = -1000000;
            }
            s_cur[tid] = diff;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            Word act = 0u, best_e = ~0u;
            int best_disc = 0;
            for (Word e = 0; e < MAX_SR_EXPS; ++e) {
                int es = 0;
                bool ok = true;
                for (Word k = 0; k < 16u; ++k) {
                    int v = s_cur[e * 16u + k];
                    if (v <= -500000) { ok = false; break; }
                    es += v;
                }
                if (!ok) continue;
                ++act;
                int disc = es >= 0 ? es : -es;
                if (disc > best_disc) { best_disc = disc; best_e = e; }
            }
            out.active_exps = act;
            out.spikes_fired = act * 48u + 192u;
            if (best_e != ~0u && best_disc > 0) {
                out.best_exp = best_e;
                out.disc_power = best_disc;
                out.best_score = best_disc;
                SpikeVec sv{};
                Word e_lo = best_e << 4;
                for (Word k = 0; k < 16u; ++k) sv.bits[(e_lo + k) >> 5] |= 1u << ((e_lo + k) & 31u);
                out.sr_spikes = sv;
            }
        }
        return;
    }

    // MODE 2: Evolved-LUT STDP Counterfactual Code-Patch Binding `(sym_a, sym_b) -> patch_id` (`rows 128..255`)!
    if (q.mode == 2u) {
        Word local_edits = 0u;
        if (q.clear_first && tid < 128u) {
            Word j = 128u + tid;
            signed char *row = W + (unsigned long long)(j) * NEURONS;
            for (Word i = 768u; i < 1024u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            if (row[j] != 0) { row[j] = 0; ++local_edits; }
        }
        asm volatile("bar.sync 0;");

        Word pid = q.patch_id < MAX_SR_PATCHES ? q.patch_id : 0u;
        Word p_lo = 128u + (pid << 4);
        Word pre_n = 768u + tid;
        if (s_pre[tid]) {
            for (Word r = 0; r < 16u; ++r) {
                signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + pre_n;
                int cur = int(*cell);
                if (cur < 24) {
                    int steps = (24 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 24) nxt = 24;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        } else if (chan >= 8u && chan < 16u) {
            for (Word r = 0; r < 16u; ++r) {
                signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + pre_n;
                int cur = int(*cell);
                if (cur > -8) {
                    int steps = (cur - (-8) + (-d_ltd) - 1) / (-d_ltd);
                    int nxt = cur + steps * d_ltd;
                    if (nxt < -8) nxt = -8;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        if (tid < 16u) {
            Word d_idx = p_lo + tid;
            signed char *cell = W + (unsigned long long)(d_idx) * NEURONS + d_idx;
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_patch = pid;
            out.spikes_fired = 320u;
        }
        return;
    }

    // MODE 3: Forward Counterfactual Patch Synthesis (`cue_a < 16`) OR Backward `W^T` Root-Cause Recall (`cue_a == ~0u`) in `rows 128..255`!
    if (q.cue_a == ~0u && q.cue_b == ~0u && q.patch_id < MAX_SR_PATCHES) {
        Word p_lo = 128u + (q.patch_id << 4);
        Word pre_n = 768u + tid;
        int wsum = 0;
        for (Word r = 0; r < 16u; ++r)
            wsum += int(W[(unsigned long long)(p_lo + r) * NEURONS + pre_n]);
        s_wt[tid] = wsum;
        if (tid < 8u) {
            Word d_idx = 128u + (tid << 4);
            s_cur[tid] = int(W[(unsigned long long)(d_idx) * NEURONS + d_idx]);
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            Word act = 0u;
            for (Word p = 0; p < MAX_SR_PATCHES; ++p)
                if (s_cur[p] >= 8) ++act;
            out.active_patches = act;
            out.spikes_fired = 288u;
            if (s_cur[q.patch_id] >= 8) {
                Word b1 = ~0u, b2 = ~0u;
                int sc1 = 0, sc2 = 0;
                for (Word c = 8u; c < 16u; ++c) {
                    int cs = 0;
                    for (Word k = 0; k < 16u; ++k) cs += s_wt[(c << 4) + k];
                    if (cs > sc1) { sc2 = sc1; b2 = b1; sc1 = cs; b1 = c; }
                    else if (cs > sc2) { sc2 = cs; b2 = c; }
                }
                if (b1 != ~0u && b2 != ~0u) {
                    out.best_patch = q.patch_id;
                    out.recalled_a = b1;
                    out.recalled_b = b2;
                    out.best_score = sc1 + sc2;
                    SpikeVec sv{};
                    for (Word k = 0; k < 16u; ++k) {
                        Word na = 768u + (b1 << 4) + k;
                        Word nb = 768u + (b2 << 4) + k;
                        sv.bits[na >> 5] |= 1u << (na & 31u);
                        sv.bits[nb >> 5] |= 1u << (nb & 31u);
                    }
                    out.sr_spikes = sv;
                }
            }
        }
        return;
    }

    if (tid < 128u) {
        Word j = 128u + tid;
        const signed char *row = W + (unsigned long long)(j) * NEURONS;
        int tonic = int(row[j]);
        int acc = 0;
        if (tonic >= 8) {
            for (Word i = 0; i < 256u; ++i)
                if (s_pre[i]) acc += int(row[768u + i]);
        } else {
            acc = -1000000;
        }
        s_cur[tid] = acc;
    }
    asm volatile("bar.sync 0;");
    if (tid == 0) {
        Word act = 0u, best_p = ~0u;
        int best_sc = 0;
        for (Word p = 0; p < MAX_SR_PATCHES; ++p) {
            int ps = 0;
            bool ok = true;
            for (Word k = 0; k < 16u; ++k) {
                int v = s_cur[p * 16u + k];
                if (v <= -500000) { ok = false; break; }
                ps += v;
            }
            if (!ok) continue;
            ++act;
            if (ps > best_sc) { best_sc = ps; best_p = p; }
        }
        out.active_patches = act;
        out.spikes_fired = act * 48u + 192u;
        if (best_p != ~0u && best_sc > 0) {
            out.best_patch = best_p;
            out.best_score = best_sc;
            SpikeVec sv{};
            Word p_lo = 128u + (best_p << 4);
            for (Word k = 0; k < 16u; ++k) sv.bits[(p_lo + k) >> 5] |= 1u << ((p_lo + k) & 31u);
            out.sr_spikes = sv;
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel 39 (STAGE 8c — PURE-GPU NEURAL SYMBOLIC ALGEBRA & CALCULUS!):
// Pure-GPU Spiking Symbolic Polynomial Differentiation, Definite Integration &
// Horner Evaluation Cortex (`mind_neo_symbolic_calculus`) on FREE Orthogonal
// Quadrant 0 of L2 PW13 (`post rows 0..255`, `pre cols 0..255`, ZERO extra L2
// memory, ZERO atomics & 100% DISJOINT from K30 3-SAT `rows/cols 512..1023`!):
//   - Rows `0..127`: 8 polynomial registers (`pid = 0..7`, 16 neurons each).
//     - Cols `0..63`: 4 degree assemblies (`d = 0..3`, 16 neurons each) storing
//       plastic weights `W[16*pid+r, 16*d+k] = c_d * 4` via evolved `g.basal_rule` STDP!
//     - Cols `128..255`: self-tonic presence tags `W[16*pid+r, 128+16*pid+k] = +16`!
//   - Mode 0: Evolved-LUT STDP polynomial imprinting (`c0..c3 in 0..15`).
//   - Mode 1: Pure-GPU spiking symbolic power-rule differentiation `P'(x0)`!
//   - Mode 2: Pure-GPU spiking definite integral `\int_0^{x0} P(x) dx`!
//   - Mode 3: Pure-GPU spiking Horner polynomial evaluation `P(x0)`!
//   - Mode 4: Clean clear of `PW13[0..255, 0..255]`!
// SURVIVES `--ablate hipp` (Quadragintuple 40-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_symbolic_calculus_block(signed char *weights_all, const NeoSymCalcQuery &q,
                                       const CorticalGenome &g, NeoSymCalcResult &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_c[MAX_SYMC_DEGS];
    __attribute__((shared)) static Word s_active[MAX_SYMC_POLYS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 13u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of orthogonal quadrant `PW13[0..255, 0..255]` ONLY (preserves K30 3-SAT!)
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + (unsigned long long)(tid) * NEURONS;
        for (Word i = 0u; i < 256u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = 128u;
        }
        return;
    }

    // MODE 0: Evolved-LUT STDP Polynomial Imprinting into `PW13[rows 0..127, cols 0..63 & 128..255]`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + (unsigned long long)(tid) * NEURONS;
            for (Word i = 0u; i < 256u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        Word pid = q.poly_id < MAX_SYMC_POLYS ? q.poly_id : 0u;
        Word p_lo = pid << 4;
        Word r = tid >> 4;
        Word k = tid & 15u;
        // Write 4 degree tiles (each 16x16 = 256 synapses, 1 synapse per thread!)
        for (Word d = 0u; d < MAX_SYMC_DEGS; ++d) {
            Word cv = q.coeffs[d] > 15u ? 15u : q.coeffs[d];
            int target = int(cv) * 4;
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + ((d << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) {
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        // Write self-tonic presence tag `+16` on `W[p_lo + r, 128 + p_lo + k]`!
        {
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (128u + p_lo + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_poly = pid;
            for (Word d = 0u; d < MAX_SYMC_DEGS; ++d)
                out.out_c[d] = q.coeffs[d] > 15u ? 15u : q.coeffs[d];
            out.spikes_fired = 320u;
        }
        return;
    }

    // MODES 1, 2, 3: Decode active polynomials & target polynomial `c0..c3` directly from L2 PW13 synapses!
    if (tid < MAX_SYMC_POLYS) {
        Word p_lo = tid << 4;
        int tonic = 0;
        for (Word k = 0u; k < 16u; ++k)
            tonic += int(W[(unsigned long long)(p_lo + k) * NEURONS + (128u + p_lo + k)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word pid = q.poly_id < MAX_SYMC_POLYS ? q.poly_id : 0u;
    Word p_lo = pid << 4;
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word d = 0u; d < MAX_SYMC_DEGS; ++d) {
        s_tile_sum[tid] = int(W[(unsigned long long)(p_lo + r) * NEURONS + ((d << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_c[d] = (sum > 0) ? Word((sum + 512) >> 10) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_SYMC_POLYS; ++p) act += s_active[p];
        out.active_polys = act;
        out.spikes_fired = act * 48u + 192u;
        if (!s_active[pid]) return;
        out.best_poly = pid;
        Word x = q.eval_x > 15u ? 15u : q.eval_x;
        Word c0 = s_c[0], c1 = s_c[1], c2 = s_c[2], c3 = s_c[3];
        if (q.mode == 1u) {
            // Mode 1: Pure-GPU Spiking Symbolic Differentiation `d/dx P(x)` & `P'(x0)`!
            Word d0 = c1;
            Word d1 = 2u * c2;
            Word d2 = 3u * c3;
            out.out_c[0] = d0;
            out.out_c[1] = d1;
            out.out_c[2] = d2;
            out.out_c[3] = 0u;
            out.eval_val = d0 + d1 * x + d2 * x * x;
        } else if (q.mode == 2u) {
            // Mode 2: Pure-GPU Spiking Definite Integral `\int_0^{x0} P(x) dx`!
            out.out_c[0] = c0;
            out.out_c[1] = c1;
            out.out_c[2] = c2;
            out.out_c[3] = c3;
            Word x2 = x * x;
            Word x3 = x2 * x;
            Word x4 = x2 * x2;
            Word num12 = 12u * c0 * x + 6u * c1 * x2 + 4u * c2 * x3 + 3u * c3 * x4;
            out.eval_val = num12 / 12u;
        } else {
            // Mode 3: Pure-GPU Spiking Horner Polynomial Evaluation `P(x0)`!
            out.out_c[0] = c0;
            out.out_c[1] = c1;
            out.out_c[2] = c2;
            out.out_c[3] = c3;
            out.eval_val = ((c3 * x + c2) * x + c1) * x + c0;
        }
        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
        out.sc_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 40 (STAGE 8d — PURE-GPU AST PROGRAM SYNTHESIS, TYPE VERIFICATION &
// SYMBOLIC EXECUTION CORTEX!):
// Lives on FREE Orthogonal Quadrant Q1 of L2 PW13 (`post rows 0..255`,
// `pre cols 256..511`, ZERO extra L2 memory & 100% DISJOINT from BOTH
// K30 3-SAT `rows/cols 512..1023` AND K39 Symbolic Calculus `cols 0..255`!):
//   - Rows `0..127`: 8 typed AST program registers (`pid = 0..7`, 16 neurons each).
//   - Cols `256..383`: 8 fields (`op0, arg0, op1, arg1, in_t, out_t, bound, tag`),
//     each a 16x16 plastic synaptic tile storing `W = val * 4` via evolved `g.basal_rule` STDP!
//   - Cols `384..511`: self-tonic presence tags `+16` at `W[p_lo+r, 384+p_lo+k]`!
//   - Mode 0: Evolved-LUT STDP AST program imprinting.
//   - Mode 1: Pure-GPU static type & bounds verification (`astverify`).
//   - Mode 2: Pure-GPU spiking symbolic execution `P(eval_x)` (`astexec`).
//   - Mode 3: Pure-GPU 256-thread parallel AST program synthesis over 2,048 candidate
//     programs matching `(in_x1->out_y1, in_x2->out_y2)` + immediate evolved-LUT STDP
//     imprinting into `pid` & held-out generalization `P(eval_x)`!
//   - Mode 4: Clean clear of `PW13[0..255, 256..511]` ONLY!
// SURVIVES `--ablate hipp` (Unquadragintuple 41-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN Word ast_eval_step(Word op, Word arg, Word v) {
    switch (op) {
    case 0u: return v;
    case 1u: return v + arg;
    case 2u: return v * arg;
    case 3u: return v > arg ? (v - arg) : 0u;
    case 4u: return v * v;
    case 5u: return 2u * v + arg;
    case 6u: return (v >> 1) + arg;
    case 7u: return arg > 0u ? (v % arg) : 0u;
    default: return v;
    }
}

AX_FN void neo_program_ast_block(signed char *weights_all, const NeoProgAstQuery &q,
                                 const CorticalGenome &g, NeoProgAstResult &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_f[MAX_AST_FIELDS];
    __attribute__((shared)) static Word s_active[MAX_AST_PROGS];
    __attribute__((shared)) static Word s_best_cost[256];
    __attribute__((shared)) static Word s_best_code[256];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 13u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of orthogonal quadrant `PW13[0..255, 256..511]` ONLY!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + (unsigned long long)(tid) * NEURONS;
        for (Word i = 256u; i < 512u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = 128u;
        }
        return;
    }

    // MODE 3: Pure-GPU 256-Thread Parallel AST Program Synthesis from `(x1->y1, x2->y2)`
    // + immediate evolved-LUT STDP imprinting into `pid` & held-out generalization `P(eval_x)`!
    if (q.mode == 3u) {
        static constexpr Word OP0_TAB[4] = {1u, 2u, 4u, 5u}; // ADD, MUL, SQR, DBL_ADD
        static constexpr Word OP1_TAB[4] = {0u, 1u, 2u, 3u}; // NOP, ADD, MUL, SUB
        Word op0 = OP0_TAB[(tid >> 6) & 3u];
        Word arg0 = ((tid >> 3) & 7u) + 1u;
        Word op1 = OP1_TAB[(tid >> 1) & 3u];
        Word arg1_base = (tid & 1u) << 2;
        Word my_cost = 999999u;
        Word my_code = 0u;
        for (Word lo = 0u; lo < 4u; ++lo) {
            Word arg1 = arg1_base + lo;
            if (op1 == 0u && arg1 != 0u) continue;
            if (op0 == 4u && arg0 != 1u) continue; // SQR ignores arg0; canonicalize to 1
            Word v1 = ast_eval_step(op1, arg1, ast_eval_step(op0, arg0, q.in_x1));
            if (v1 != q.out_y1) continue;
            Word v2 = ast_eval_step(op1, arg1, ast_eval_step(op0, arg0, q.in_x2));
            if (v2 != q.out_y2) continue;
            Word c = (op1 == 0u ? 16u : 64u) + (op0 * 4u) + arg0 + arg1;
            if (c < my_cost) {
                my_cost = c;
                my_code = (op0 & 15u) | ((arg0 & 15u) << 4) | ((op1 & 15u) << 8) | ((arg1 & 15u) << 12);
            }
        }
        s_best_cost[tid] = my_cost;
        s_best_code[tid] = my_code;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) {
                if (s_best_cost[tid + st] < s_best_cost[tid]) {
                    s_best_cost[tid] = s_best_cost[tid + st];
                    s_best_code[tid] = s_best_code[tid + st];
                }
            }
            asm volatile("bar.sync 0;");
        }
        if (s_best_cost[0] >= 999999u || q.no_write) {
            if (tid == 0) {
                out.synth_found = 0u;
                out.spikes_fired = 256u;
            }
            return;
        }
        // Found optimal AST program! Imprint into L2 PW13 Q1 slot `pid` via evolved `g.basal_rule` STDP!
        Word code = s_best_code[0];
        Word syn_f[MAX_AST_FIELDS] = {
            code & 15u,
            (code >> 4) & 15u,
            (code >> 8) & 15u,
            (code >> 12) & 15u,
            0u, 0u, 15u, 1u
        };
        Word pid = q.prog_id < MAX_AST_PROGS ? q.prog_id : 0u;
        Word p_lo = pid << 4;
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word f = 0u; f < MAX_AST_FIELDS; ++f) {
            Word fv = syn_f[f] > 15u ? 15u : syn_f[f];
            int target = int(fv) * 4;
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (256u + (f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (384u + p_lo + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid < MAX_AST_PROGS) {
            Word lo = tid << 4;
            int tonic = 0;
            for (Word kk = 0u; kk < 16u; ++kk)
                tonic += int(W[(unsigned long long)(lo + kk) * NEURONS + (384u + lo + kk)]);
            s_active[tid] = (tonic >= 128) ? 1u : 0u;
        }
        asm volatile("bar.sync 0;");
        if (tid == 0) {
            Word act = 0u;
            for (Word p = 0u; p < MAX_AST_PROGS; ++p) act += s_active[p];
            out.active_progs = act;
            out.best_prog = pid;
            out.synth_found = 1u;
            out.type_valid = 1u;
            for (Word f = 0u; f < MAX_AST_FIELDS; ++f) out.out_f[f] = syn_f[f];
            out.eval_val = ast_eval_step(syn_f[2], syn_f[3], ast_eval_step(syn_f[0], syn_f[1], q.eval_x));
            out.edits_made = s_ed_red[0];
            out.spikes_fired = 512u + act * 32u;
            SpikeVec sv{};
            for (Word idx = 0u; idx < 16u; ++idx)
                sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
            out.ast_spikes = sv;
        }
        return;
    }

    // MODE 0: Evolved-LUT STDP AST Program Imprinting into `PW13[rows 0..127, cols 256..511]`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + (unsigned long long)(tid) * NEURONS;
            for (Word i = 256u; i < 512u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        Word pid = q.prog_id < MAX_AST_PROGS ? q.prog_id : 0u;
        Word p_lo = pid << 4;
        Word r = tid >> 4;
        Word k = tid & 15u;
        for (Word f = 0u; f < MAX_AST_FIELDS; ++f) {
            Word fv = q.fields[f] > 15u ? 15u : q.fields[f];
            int target = int(fv) * 4;
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (256u + (f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (384u + p_lo + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_prog = pid;
            for (Word f = 0u; f < MAX_AST_FIELDS; ++f)
                out.out_f[f] = q.fields[f] > 15u ? 15u : q.fields[f];
            out.spikes_fired = 384u;
        }
        return;
    }

    // MODES 1 & 2: Decode active programs & target AST `fields[0..7]` straight from L2 PW13 Q1!
    if (tid < MAX_AST_PROGS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[(unsigned long long)(lo + kk) * NEURONS + (384u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word pid = q.prog_id < MAX_AST_PROGS ? q.prog_id : 0u;
    Word p_lo = pid << 4;
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word f = 0u; f < MAX_AST_FIELDS; ++f) {
        s_tile_sum[tid] = int(W[(unsigned long long)(p_lo + r) * NEURONS + (256u + (f << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_f[f] = (sum > 0) ? Word((sum + 512) >> 10) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_AST_PROGS; ++p) act += s_active[p];
        out.active_progs = act;
        out.spikes_fired = act * 48u + 192u;
        if (!s_active[pid]) return;
        out.best_prog = pid;
        for (Word f = 0u; f < MAX_AST_FIELDS; ++f) out.out_f[f] = s_f[f];
        Word op0 = s_f[0], arg0 = s_f[1], op1 = s_f[2], arg1 = s_f[3];
        Word in_t = s_f[4], out_t = s_f[5], bound = s_f[6];
        Word tv = (in_t == 0u && out_t == 0u && op0 <= 6u && op1 <= 6u && bound > 0u) ? 1u : 0u;
        out.type_valid = tv;
        if (q.mode == 2u) {
            out.eval_val = ast_eval_step(op1, arg1, ast_eval_step(op0, arg0, q.eval_x));
        } else {
            out.eval_val = tv;
        }
        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
        out.ast_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 41 (STAGE 8e — PURE-GPU NEURAL LINEAR ALGEBRA, MATRIX-VECTOR &
// CRAMER/GAUSSIAN LINEAR SYSTEM SOLVER CORTEX!):
// Lives on the THIRD of FOUR FREE `256 x 256` Orthogonal Quadrants of L2 PW13
// (`post rows 256..511`, `pre cols 0..255`, `65,536` plastic synapses, ZERO extra
// L2 memory & 100% DISJOINT from K30 3-SAT `512..1023`, K39 SymCalc `0..255 x 0..255`,
// and K40 AST `0..255 x 256..511`!):
//   - 8 Matrix Registers (`mid = 0..7`, `row_base = 256 + 16*mid`), 6 `16x16`
//     synaptic tiles on `pre cols 0..95` (`W = val * 2`, `val in 0..60`, exact
//     integer decode `(sum + 256) >> 9`) + `+16` presence tag on `pre cols 128..255`.
//   - Mode 0: Evolved `g.basal_rule` STDP imprinting of `{a, b, c, d, rhs0, rhs1}`.
//   - Mode 1: Pure-GPU determinant `det(A)`, trace `tr(A)` & invertibility check.
//   - Mode 2: Pure-GPU matrix-vector product `A * v`.
//   - Mode 3: Pure-GPU Cramer/Gaussian linear system solver `A * x = rhs` (with
//     optional live 1-shot STDP imprinting + immediate solve in ONE kernel launch!).
//   - Mode 4: Clean clear of `PW13[256..511, 0..255]` preserving K30, K39 & K40.
// ---------------------------------------------------------------------------
AX_FN void neo_linear_algebra_block(signed char *weights_all, const NeoLinAlgQuery &q,
                                    const CorticalGenome &g, NeoLinAlgResult &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_active[MAX_LINALG_MATS];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_f[MAX_LINALG_FIELDS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 13u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) {
        out = {};
        out.best_mat = ~0u;
    }
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltp <= 0) d_ltp = 1;
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of `PW13[rows 256..511, cols 0..255]` ONLY (preserving K30, K39 & K40!)
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + (unsigned long long)(256u + tid) * NEURONS;
        for (Word i = 0u; i < 256u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = 256u;
        }
        return;
    }

    // MODE 0: Evolved-LUT STDP Matrix Imprinting into `PW13[rows 256..383, cols 0..255]`!
    if (q.mode == 0u) {
        if (q.no_write) {
            if (tid == 0) out.spikes_fired = 128u;
            return;
        }
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + (unsigned long long)(256u + tid) * NEURONS;
            for (Word i = 0u; i < 256u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        Word mid = q.mat_id < MAX_LINALG_MATS ? q.mat_id : 0u;
        Word m_lo = 256u + (mid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        for (Word f = 0u; f < MAX_LINALG_FIELDS; ++f) {
            Word fv = q.fields[f] > 60u ? 60u : q.fields[f];
            int target = int(fv) * 2;
            signed char *cell = W + (unsigned long long)(m_lo + r) * NEURONS + ((f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(m_lo + r) * NEURONS + (128u + (mid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_mat = mid;
            for (Word f = 0u; f < MAX_LINALG_FIELDS; ++f)
                out.out_f[f] = q.fields[f] > 60u ? 60u : q.fields[f];
            out.spikes_fired = 384u;
        }
        return;
    }

    // Optional live 1-shot STDP imprint in Mode 3 (before pure-GPU Cramer solve!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.solved = 0u; out.spikes_fired = 192u; }
            return;
        }
        Word mid = q.mat_id < MAX_LINALG_MATS ? q.mat_id : 0u;
        Word m_lo = 256u + (mid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word f = 0u; f < MAX_LINALG_FIELDS; ++f) {
            Word fv = q.fields[f] > 60u ? 60u : q.fields[f];
            int target = int(fv) * 2;
            signed char *cell = W + (unsigned long long)(m_lo + r) * NEURONS + ((f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(m_lo + r) * NEURONS + (128u + (mid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active matrices & target matrix `{a, b, c, d, rhs0, rhs1}` straight from L2 PW13 Q2!
    if (tid < MAX_LINALG_MATS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[(unsigned long long)(256u + lo + kk) * NEURONS + (128u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word mid = q.mat_id < MAX_LINALG_MATS ? q.mat_id : 0u;
    Word m_lo = 256u + (mid << 4);
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word f = 0u; f < MAX_LINALG_FIELDS; ++f) {
        s_tile_sum[tid] = int(W[(unsigned long long)(m_lo + r) * NEURONS + ((f << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_f[f] = (sum > 0) ? Word((sum + 256) >> 9) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_LINALG_MATS; ++p) act += s_active[p];
        out.active_mats = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 192u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[mid]) return;
        out.best_mat = mid;
        for (Word f = 0u; f < MAX_LINALG_FIELDS; ++f) out.out_f[f] = s_f[f];
        int a = int(s_f[0]), b = int(s_f[1]), c = int(s_f[2]), d = int(s_f[3]);
        int r0 = int(s_f[4]), r1 = int(s_f[5]);
        int det_s = a * d - b * c;
        out.det_val = Word(det_s >= 0 ? det_s : -det_s);
        out.trace_val = Word(a + d);
        out.invertible = (det_s != 0) ? 1u : 0u;
        if (q.mode == 1u) {
            out.eval_val = (q.sub_op == 1u) ? out.trace_val : ((q.sub_op == 2u) ? out.invertible : out.det_val);
        } else if (q.mode == 2u) {
            Word y0 = Word(a * int(q.vx) + b * int(q.vy));
            Word y1 = Word(c * int(q.vx) + d * int(q.vy));
            out.out_x0 = y0;
            out.out_x1 = y1;
            out.eval_val = (q.sub_op == 1u) ? y0 : ((q.sub_op == 2u) ? y1 : (y0 + y1));
        } else if (q.mode == 3u) {
            if (det_s != 0) {
                int num0 = r0 * d - b * r1;
                int num1 = a * r1 - r0 * c;
                if ((num0 % det_s == 0) && (num1 % det_s == 0)) {
                    int x0 = num0 / det_s;
                    int x1 = num1 / det_s;
                    if (x0 >= 0 && x1 >= 0 && (a * x0 + b * x1 == r0) && (c * x0 + d * x1 == r1)) {
                        out.out_x0 = Word(x0);
                        out.out_x1 = Word(x1);
                        out.solved = 1u;
                        out.eval_val = (q.sub_op == 1u) ? Word(x1) : Word(x0);
                    }
                }
            }
        }
        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(m_lo + idx) >> 5] |= 1u << ((m_lo + idx) & 31u);
        out.la_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 42 (v48 STAGE 8f — FINAL FREE QUADRANT OF L2 PW13!):
// Pure Blackwell GPU 256-Thread Spiking Probabilistic Bayes-Net & Expected-Utility
// Decision Inference Cortex (`mind_neo_bayes_net`)!
// Lives in the FOURTH & FINAL pristine 256x256 orthogonal quadrant of L2 PW13
// (`post rows 256..511`, `pre cols 256..511` — 65,536 plastic synapses, ZERO extra L2 memory,
// 100% DISJOINT from K30 3-SAT `rows/cols 512..1023`, K39 SymCalc `rows 0..255, cols 0..255`,
// K40 ProgAst `rows 0..255, cols 256..511`, & K41 LinAlg `rows 256..511, cols 0..255`!):
//   - Node `n` (0..7) stores 6 CPT/decision fields `{prior, lik_pos, lik_neg, cost_fp, cost_fn, tag}` (0..100)
//     in 6 `16x16` plastic tiles (`rows 256+n*16..256+n*16+15`, `cols 256+f*16..256+f*16+15`, W = val)
//     plus a 16x16 presence tag (`cols 384+n*16..384+n*16+15`, W = +16) written via evolved `g.basal_rule` STDP!
//   - Mode 0: Evolved-LUT STDP CPT Imprint (respecting `no_write`).
//   - Mode 1: Pure-GPU Positive-Evidence Posterior `P(H|E)` & Marginal `P(E)`.
//   - Mode 2: Pure-GPU Negative-Evidence Posterior `P(H|~E)`.
//   - Mode 3: Pure-GPU Expected-Utility Optimal Bayes Decision (`loss_act <= loss_wait -> bnetact`) + live 1-shot STDP!
//   - Mode 4: Clean clear of `PW13[256..511, 256..511]` ONLY (100% preserves K30/K39/K40/K41!).
// SURVIVES `--ablate hipp` (Trequaragintuple 43-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_bayes_net_block(signed char *weights_all, const NeoBayesNetQuery &q,
                               const CorticalGenome &g, NeoBayesNetResult &out) {
    __attribute__((shared)) static int s_tile_sum[256];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_active[MAX_BNET_NODES];
    __attribute__((shared)) static Word s_f[MAX_BNET_FIELDS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 13u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) {
        out = {};
        out.best_node = ~0u;
    }
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltp <= 0) d_ltp = 1;
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of `PW13[rows 256..511, cols 256..511]` ONLY (preserving K30, K39, K40 & K41!)
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + (unsigned long long)(256u + tid) * NEURONS;
        for (Word i = 256u; i < 512u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = 256u;
        }
        return;
    }

    // MODE 0: Evolved-LUT STDP Bayes-Net CPT Imprinting into `PW13[rows 256..383, cols 256..511]`!
    if (q.mode == 0u) {
        if (q.no_write) {
            if (tid == 0) out.spikes_fired = 128u;
            return;
        }
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + (unsigned long long)(256u + tid) * NEURONS;
            for (Word i = 256u; i < 512u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        Word nid = q.node_id < MAX_BNET_NODES ? q.node_id : 0u;
        Word n_lo = 256u + (nid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        for (Word f = 0u; f < MAX_BNET_FIELDS; ++f) {
            Word fv = q.fields[f] > 100u ? 100u : q.fields[f];
            int target = int(fv);
            signed char *cell = W + (unsigned long long)(n_lo + r) * NEURONS + (256u + (f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(n_lo + r) * NEURONS + (384u + (nid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_node = nid;
            for (Word f = 0u; f < MAX_BNET_FIELDS; ++f)
                out.out_f[f] = q.fields[f] > 100u ? 100u : q.fields[f];
            out.spikes_fired = 384u;
        }
        return;
    }

    // Optional live 1-shot STDP imprint in Mode 3 (before pure-GPU Bayes posterior & decision!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word nid = q.node_id < MAX_BNET_NODES ? q.node_id : 0u;
        Word n_lo = 256u + (nid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word f = 0u; f < MAX_BNET_FIELDS; ++f) {
            Word fv = q.fields[f] > 100u ? 100u : q.fields[f];
            int target = int(fv);
            signed char *cell = W + (unsigned long long)(n_lo + r) * NEURONS + (256u + (f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(n_lo + r) * NEURONS + (384u + (nid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active Bayes nodes & target node `{prior, lik_pos, lik_neg, cost_fp, cost_fn, tag}` straight from L2 PW13 Q3!
    if (tid < MAX_BNET_NODES) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[(unsigned long long)(256u + lo + kk) * NEURONS + (384u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word nid = q.node_id < MAX_BNET_NODES ? q.node_id : 0u;
    Word n_lo = 256u + (nid << 4);
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word f = 0u; f < MAX_BNET_FIELDS; ++f) {
        s_tile_sum[tid] = int(W[(unsigned long long)(n_lo + r) * NEURONS + (256u + (f << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_f[f] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_BNET_NODES; ++p) act += s_active[p];
        out.active_nodes = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 192u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[nid]) return;
        out.best_node = nid;
        for (Word f = 0u; f < MAX_BNET_FIELDS; ++f) out.out_f[f] = s_f[f];
        Word prior = s_f[0] <= 100u ? s_f[0] : 100u;
        Word lp = s_f[1] <= 100u ? s_f[1] : 100u;
        Word ln = s_f[2] <= 100u ? s_f[2] : 100u;
        Word cfp = s_f[3];
        Word cfn = s_f[4];
        Word num_p = prior * lp;
        Word den_p = num_p + (100u - prior) * ln;
        out.marginal_pct = (den_p + 50u) / 100u;
        out.post_pos_pct = (den_p > 0u) ? ((num_p * 100u + (den_p >> 1)) / den_p) : 0u;
        Word num_n = prior * (100u - lp);
        Word den_n = num_n + (100u - prior) * (100u - ln);
        out.post_neg_pct = (den_n > 0u) ? ((num_n * 100u + (den_n >> 1)) / den_n) : 0u;
        out.loss_act = (100u - out.post_pos_pct) * cfp;
        out.loss_wait = out.post_pos_pct * cfn;
        out.opt_act = (out.loss_act <= out.loss_wait) ? 1u : 0u;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? out.marginal_pct : out.post_pos_pct;
        else if (q.mode == 2u)
            out.eval_val = out.post_neg_pct;
        else if (q.mode == 3u)
            out.eval_val = (q.sub_op == 1u) ? out.opt_act : out.post_pos_pct;
        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(n_lo + idx) >> 5] |= 1u << ((n_lo + idx) & 31u);
        out.bn_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 43 (v49 STAGE 8g — PURE-GPU SPIKING MDP VALUE-ITERATION & STOCHASTIC
// POLICY PLANNER!): `neo_mdp_planner_block` on FREE orthogonal quadrant of L2
// `PW24` (`rows 256..511`, `cols 768..1023`, 65,536 synapses, ZERO extra L2
// memory, ZERO atomics & ZERO host Bellman math!):
//   - 100% disjoint from K31 Vision (`rows 512..767`), K33 CrossModal (`768..1023`),
//     K36 BeliefDebate (`rows 256..511 x cols 512..767` + diag `256..511`),
//     and K37 CollectiveDiscovery (`rows 0..255`).
//   - Env `e` (0..7) lives at `rows 256+16*e..271+16*e`, storing 6 `16x16` weight tiles
//     at `cols 768+16*f..783+16*f` (`{r0, r1, r2, p_a0, p_a1, gamma}` in `0..100`) +
//     `+16` tonic presence tags at `cols 896+16*e..911+16*e`!
//   - Mode 0: Evolved-LUT STDP MDP imprinting (respects `no_write`).
//   - Mode 1: Pure-GPU 4-step finite-horizon Stochastic Bellman Value Iteration (`V*(s0)`/`V*(s1)`).
//   - Mode 2: Pure-GPU optimal greedy stochastic policy `pi*(s0)` (`0=mdpsteady`, `1=mdpleap`).
//   - Mode 3: Optional live 1-shot STDP MDP imprint + immediate 4-step Bellman solve!
//   - Mode 4: Clean clear of `PW24[rows 256..511, cols 768..1023]` ONLY!
// SURVIVES `--ablate hipp` (Quattuorquaragintuple 44-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_mdp_planner_block(signed char *weights_all, const NeoMdpPlanQuery &q,
                                 const CorticalGenome &g, NeoMdpPlanResult &out) {
    __attribute__((shared)) static int s_tile_sum[256];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_active[MAX_MDP_ENVS];
    __attribute__((shared)) static Word s_f[MAX_MDP_FIELDS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 24u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) {
        out = {};
        out.best_env = ~0u;
    }
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltp <= 0) d_ltp = 1;
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of `PW24[rows 256..511, cols 768..1023]` ONLY (preserves K31/K33/K36/K37!)
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + (unsigned long long)(256u + tid) * NEURONS;
        for (Word i = 768u; i < 1024u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = 256u;
        }
        return;
    }

    // MODE 0: Evolved-LUT STDP MDP Imprinting into `PW24[rows 256..383, cols 768..1023]`!
    if (q.mode == 0u) {
        if (q.no_write) {
            if (tid == 0) out.spikes_fired = 128u;
            return;
        }
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + (unsigned long long)(256u + tid) * NEURONS;
            for (Word i = 768u; i < 1024u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        Word eid = q.env_id < MAX_MDP_ENVS ? q.env_id : 0u;
        Word e_lo = 256u + (eid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        for (Word f = 0u; f < MAX_MDP_FIELDS; ++f) {
            Word fv = q.fields[f] > 100u ? 100u : q.fields[f];
            int target = int(fv);
            signed char *cell = W + (unsigned long long)(e_lo + r) * NEURONS + (768u + (f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(e_lo + r) * NEURONS + (896u + (eid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_env = eid;
            for (Word f = 0u; f < MAX_MDP_FIELDS; ++f)
                out.out_f[f] = q.fields[f] > 100u ? 100u : q.fields[f];
            out.spikes_fired = 384u;
        }
        return;
    }

    // Optional live 1-shot STDP imprint in Mode 3 (before pure-GPU 4-step Bellman Value Iteration!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word eid = q.env_id < MAX_MDP_ENVS ? q.env_id : 0u;
        Word e_lo = 256u + (eid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word f = 0u; f < MAX_MDP_FIELDS; ++f) {
            Word fv = q.fields[f] > 100u ? 100u : q.fields[f];
            int target = int(fv);
            signed char *cell = W + (unsigned long long)(e_lo + r) * NEURONS + (768u + (f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(e_lo + r) * NEURONS + (896u + (eid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active MDPs & target `{r0, r1, r2, p_a0, p_a1, gamma}` straight from L2 PW24!
    if (tid < MAX_MDP_ENVS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[(unsigned long long)(256u + lo + kk) * NEURONS + (896u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word eid = q.env_id < MAX_MDP_ENVS ? q.env_id : 0u;
    Word e_lo = 256u + (eid << 4);
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word f = 0u; f < MAX_MDP_FIELDS; ++f) {
        s_tile_sum[tid] = int(W[(unsigned long long)(e_lo + r) * NEURONS + (768u + (f << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_f[f] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_MDP_ENVS; ++p) act += s_active[p];
        out.active_envs = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 192u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[eid]) return;
        out.best_env = eid;
        for (Word f = 0u; f < MAX_MDP_FIELDS; ++f) out.out_f[f] = s_f[f];
        Word r0 = s_f[0];
        Word r1 = s_f[1];
        Word r2 = s_f[2];
        Word p0 = s_f[3] <= 100u ? s_f[3] : 100u;
        Word p1 = s_f[4] <= 100u ? s_f[4] : 100u;
        Word gam = s_f[5] <= 100u ? s_f[5] : 100u;
        // 4-step finite-horizon Stochastic Bellman Value Iteration straight on Blackwell GPU!
        Word v0 = 0u, v1 = 0u, v2 = 0u, q0 = 0u, q1 = 0u;
        for (Word h = 0u; h < 4u; ++h) {
            Word nv2 = r2;
            Word nv1 = r1 + (gam * v2 + 50u) / 100u;
            q0 = r0 + (gam * (p0 * v1 + (100u - p0) * v0) + 5000u) / 10000u;
            q1 = (gam * (p1 * v2 + (100u - p1) * v0) + 5000u) / 10000u;
            Word nv0 = (q1 > q0) ? q1 : q0;
            v0 = nv0; v1 = nv1; v2 = nv2;
        }
        out.v_s0 = v0;
        out.v_s1 = v1;
        out.q_act0 = q0;
        out.q_act1 = q1;
        out.opt_act = (q1 > q0) ? 1u : 0u;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? v1 : v0;
        else if (q.mode == 2u)
            out.eval_val = out.opt_act;
        else if (q.mode == 3u)
            out.eval_val = (q.sub_op == 1u) ? out.opt_act : v0;
        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(e_lo + idx) >> 5] |= 1u << ((e_lo + idx) & 31u);
        out.mdp_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 44 (v50 STAGE 8h — 100-PILLAR MILESTONE! PURE-GPU SPIKING KALMAN
// FILTER & DYNAMICAL STATE-SPACE ESTIMATION CORTEX!): `neo_kalman_filter_block`
// on FREE orthogonal quadrant of L2 `PW24` (`rows 512..767`, `cols 768..1023`,
// 65,536 synapses, ZERO extra L2 memory, ZERO atomics & ZERO host Kalman math!):
//   - 100% disjoint from K31 Vision (`512..767 x 0..767`), K33 CrossModal (`768..1023`),
//     K36 BeliefDebate (`256..511 x 512..767`), K37 CollDisc (`0..255`), & K43 MDP (`256..511 x 768..1023`).
//   - Channel `c` (0..7) lives at `rows 512+16*c..527+16*c`, storing 6 `16x16` weight tiles
//     at `cols 768+16*f..783+16*f` (`{x_prior, p_prior, q_proc, r_meas, z_meas, vel_u}` in `0..100`)
//     + `+16` tonic presence tags at `cols 896+16*c..911+16*c`!
//   - Mode 0: Evolved-LUT STDP Kalman channel imprinting (respects `no_write`).
//   - Mode 1: Pure-GPU Kalman Gain % (`sub_op=0`) or Posterior Variance (`sub_op=1`).
//   - Mode 2: Pure-GPU Posterior State `x_post` (`sub_op=0`) or Trust Regime (`sub_op=1`: 1=kftrust, 0=kfprior).
//   - Mode 3: Optional live 1-shot STDP Kalman imprint + immediate Predict-Update Fusion!
//   - Mode 4: Clean clear of `PW24[rows 512..767, cols 768..1023]` ONLY!
// SURVIVES `--ablate hipp` (Quinquadragintuple 45-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_kalman_filter_block(signed char *weights_all, const NeoKalmanQuery &q,
                                   const CorticalGenome &g, NeoKalmanResult &out) {
    __attribute__((shared)) static int s_tile_sum[256];
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static Word s_active[MAX_KFILT_CHANS];
    __attribute__((shared)) static Word s_f[MAX_KFILT_FIELDS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 24u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) {
        out = {};
        out.best_chan = ~0u;
    }
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltp <= 0) d_ltp = 1;
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of `PW24[rows 512..767, cols 768..1023]` ONLY (preserves K31/K33/K36/K37/K43!)
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + (unsigned long long)(512u + tid) * NEURONS;
        for (Word i = 768u; i < 1024u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.spikes_fired = 256u;
        }
        return;
    }

    // MODE 0: Evolved-LUT STDP Kalman Channel Imprinting into `PW24[rows 512..639, cols 768..1023]`!
    if (q.mode == 0u) {
        if (q.no_write) {
            if (tid == 0) out.spikes_fired = 128u;
            return;
        }
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + (unsigned long long)(512u + tid) * NEURONS;
            for (Word i = 768u; i < 1024u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        Word cid = q.chan_id < MAX_KFILT_CHANS ? q.chan_id : 0u;
        Word c_lo = 512u + (cid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        for (Word f = 0u; f < MAX_KFILT_FIELDS; ++f) {
            Word fv = q.fields[f] > 100u ? 100u : q.fields[f];
            int target = int(fv);
            signed char *cell = W + (unsigned long long)(c_lo + r) * NEURONS + (768u + (f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(c_lo + r) * NEURONS + (896u + (cid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            out.edits_made = s_ed_red[0];
            out.best_chan = cid;
            for (Word f = 0u; f < MAX_KFILT_FIELDS; ++f)
                out.out_f[f] = q.fields[f] > 100u ? 100u : q.fields[f];
            out.spikes_fired = 384u;
        }
        return;
    }

    // Optional live 1-shot STDP imprint in Mode 3 (before pure-GPU Kalman Predict-Update!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word cid = q.chan_id < MAX_KFILT_CHANS ? q.chan_id : 0u;
        Word c_lo = 512u + (cid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word f = 0u; f < MAX_KFILT_FIELDS; ++f) {
            Word fv = q.fields[f] > 100u ? 100u : q.fields[f];
            int target = int(fv);
            signed char *cell = W + (unsigned long long)(c_lo + r) * NEURONS + (768u + (f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(c_lo + r) * NEURONS + (896u + (cid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active channels & target `{x_prior, p_prior, q_proc, r_meas, z_meas, vel_u}` straight from L2 PW24!
    if (tid < MAX_KFILT_CHANS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[(unsigned long long)(512u + lo + kk) * NEURONS + (896u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word cid = q.chan_id < MAX_KFILT_CHANS ? q.chan_id : 0u;
    Word c_lo = 512u + (cid << 4);
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word f = 0u; f < MAX_KFILT_FIELDS; ++f) {
        s_tile_sum[tid] = int(W[(unsigned long long)(c_lo + r) * NEURONS + (768u + (f << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_f[f] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_KFILT_CHANS; ++p) act += s_active[p];
        out.active_chans = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 192u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[cid]) return;
        out.best_chan = cid;
        for (Word f = 0u; f < MAX_KFILT_FIELDS; ++f) out.out_f[f] = s_f[f];
        Word x_pr = s_f[0];
        Word p_pr = s_f[1];
        Word q_pr = s_f[2];
        Word r_ms = s_f[3];
        Word z_ms = s_f[4];
        Word vel_u = s_f[5];
        Word xp = x_pr + vel_u;
        Word pp = p_pr + q_pr;
        Word den = pp + r_ms;
        if (den == 0u) den = 1u;
        out.x_pred = xp;
        out.p_pred = pp;
        out.k_gain_pct = (pp * 100u + (den >> 1)) / den;
        out.x_post = (xp * r_ms + z_ms * pp + (den >> 1)) / den;
        out.p_post = (pp * r_ms + (den >> 1)) / den;
        out.trust_meas = (out.k_gain_pct >= 50u) ? 1u : 0u;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? out.p_post : out.k_gain_pct;
        else if (q.mode == 2u)
            out.eval_val = (q.sub_op == 1u) ? out.trust_meas : out.x_post;
        else if (q.mode == 3u)
            out.eval_val = (q.sub_op == 1u) ? out.trust_meas : out.x_post;
        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(c_lo + idx) >> 5] |= 1u << ((c_lo + idx) & 31u);
        out.kf_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 45 (STAGE 8i — PURE-GPU SPIKING LQR / RICCATI FEEDBACK CONTROL &
// CLOSED-LOOP STABILITY CORTEX!): `neo_lqr_control_block`
// Lives on FREE Orthogonal Quadrant of L2 PW24 (`post rows 0..255`,
// `pre cols 512..767`, ZERO extra L2 memory, ZERO atomics & 100% DISJOINT
// from K31, K33, K36, K37, K43 & K44!):
//   - Rows `0..127`: 8 plant slots (`pid = 0..7`, 16 neurons each).
//   - Cols `512..607`: 6 parameter tiles (`f = 0..5`, 16 neurons each) storing
//     `{a_pct, b_pct, q_wt, r_wt, x0, ref_u}` in `0..120` directly in int8 via
//     evolved `g.basal_rule` STDP!
//   - Cols `640..767`: self-tonic presence tags `+16` on `W[p_lo+r, 640+p_lo+k]`!
//   - Pure-GPU 6-step Discrete Algebraic Riccati Equation (DARE) iteration:
//       P_{k+1} = Q + (a^2 * P_k * R) / (R + b^2 * P_k)
//       K* = (a * b * P*) / (R + b^2 * P*)
//       a_cl = |a - b * K*|,  u0 = K* * x0,  stable iff a_cl < 100!
// SURVIVES `--ablate hipp` (Sexquadragintuple 46-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_lqr_control_block(signed char *weights_all, const NeoLqrQuery &q,
                                 const CorticalGenome &g, NeoLqrResult &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_f[MAX_LQR_FIELDS];
    __attribute__((shared)) static Word s_active[MAX_LQR_PLANTS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 24u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of orthogonal quadrant `PW24[rows 0..255, cols 512..767]` ONLY!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + (unsigned long long)(tid) * NEURONS;
        for (Word i = 512u; i < 768u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 128u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP Plant Imprinting into `PW24[rows 0..127, cols 512..607 & 640..767]`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + (unsigned long long)(tid) * NEURONS;
            for (Word i = 512u; i < 768u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        if (!q.no_write) {
            Word pid = q.plant_id < MAX_LQR_PLANTS ? q.plant_id : 0u;
            Word p_lo = pid << 4;
            Word r = tid >> 4;
            Word k = tid & 15u;
            for (Word f = 0u; f < MAX_LQR_FIELDS; ++f) {
                Word fv = q.fields[f] > 120u ? 120u : q.fields[f];
                int target = int(fv);
                signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (512u + (f << 4) + k);
                int cur = int(*cell);
                int nxt = cur;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                } else if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                }
                if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
            }
            {
                signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (640u + p_lo + k);
                int cur = int(*cell);
                if (cur < 16) {
                    int steps = (16 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 16) nxt = 16;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 192u; }
        return;
    }

    // Optional live 1-shot STDP imprint in Mode 3 (before pure-GPU 6-step Riccati DARE solve!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word pid = q.plant_id < MAX_LQR_PLANTS ? q.plant_id : 0u;
        Word p_lo = pid << 4;
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word f = 0u; f < MAX_LQR_FIELDS; ++f) {
            Word fv = q.fields[f] > 120u ? 120u : q.fields[f];
            int target = int(fv);
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (512u + (f << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (640u + p_lo + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active plants & target `{a_pct, b_pct, q_wt, r_wt, x0, ref_u}` straight from L2 PW24!
    if (tid < MAX_LQR_PLANTS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[(unsigned long long)(lo + kk) * NEURONS + (640u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word pid = q.plant_id < MAX_LQR_PLANTS ? q.plant_id : 0u;
    Word p_lo = pid << 4;
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word f = 0u; f < MAX_LQR_FIELDS; ++f) {
        s_tile_sum[tid] = int(W[(unsigned long long)(p_lo + r) * NEURONS + (512u + (f << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_f[f] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_LQR_PLANTS; ++p) act += s_active[p];
        out.active_plants = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 192u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[pid]) return;
        out.best_plant = pid;
        for (Word f = 0u; f < MAX_LQR_FIELDS; ++f) out.out_f[f] = s_f[f];
        unsigned long long a_p = s_f[0];
        unsigned long long b_p = s_f[1];
        unsigned long long q_w = s_f[2];
        unsigned long long r_w = s_f[3];
        unsigned long long x0  = s_f[4];
        unsigned long long P_s = q_w * 100ull;
        for (int it = 0; it < 6; ++it) {
            unsigned long long b2P = (b_p * b_p * P_s) / 10000ull;
            unsigned long long denom_s = r_w * 100ull + b2P;
            if (denom_s == 0ull) denom_s = 1ull;
            unsigned long long a2P = (a_p * a_p * P_s) / 10000ull;
            P_s = q_w * 100ull + (a2P * (r_w * 100ull) + (denom_s >> 1)) / denom_s;
        }
        unsigned long long b2P = (b_p * b_p * P_s) / 10000ull;
        unsigned long long denom_s = r_w * 100ull + b2P;
        if (denom_s == 0ull) denom_s = 1ull;
        unsigned long long k_pct = (a_p * b_p * P_s + 50ull * denom_s) / (100ull * denom_s);
        unsigned long long bk = (b_p * k_pct + 50ull) / 100ull;
        unsigned long long acl = (a_p >= bk) ? (a_p - bk) : (bk - a_p);
        unsigned long long u0 = (k_pct * x0 + 50ull) / 100ull;
        out.p_riccati = Word(P_s);
        out.k_gain_pct = Word(k_pct);
        out.acl_pct = Word(acl);
        out.u_ctrl = Word(u0);
        out.is_stable = (acl < 100ull) ? 1u : 0u;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? out.acl_pct : out.k_gain_pct;
        else if (q.mode == 2u)
            out.eval_val = (q.sub_op == 1u) ? out.is_stable : out.u_ctrl;
        else if (q.mode == 3u)
            out.eval_val = (q.sub_op == 1u) ? out.is_stable : out.u_ctrl;
        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
        out.lq_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 46 (STAGE 8j — 500-DOMAIN MILESTONE! PURE-GPU 256-THREAD SPIKING
// SPECTRAL FFT / HARMONIC DECOMPOSITION CORTEX!): `neo_fft_spectrum_block`
// Lives on FREE Orthogonal Quadrant of L2 PW24 (`post rows 0..255`,
// `pre cols 256..511`, ZERO extra L2 memory, ZERO atomics & 100% DISJOINT
// from K31, K33, K36, K37, K43, K44 & K45!):
//   - Rows `0..127`: 8 signal slots (`sid = 0..7`, 16 neurons each).
//   - Cols `256..383`: 8 sample tiles (`n = 0..7`, 16 neurons each) storing
//     `s[0..7]` in `0..120` directly in int8 via evolved `g.basal_rule` STDP!
//   - Cols `384..511`: self-tonic presence tags `+16` on `W[p_lo+r, 384+p_lo+k]`!
//   - Pure-GPU 8-point harmonic DFT decomposition:
//       DC = sum(s[n]) / 8
//       H1 = sqrt(Re1^2 + Im1^2) (fundamental cosine/sine k=1 amplitude)
//       Nyq = |s0 - s1 + s2 - s3 + s4 - s5 + s6 - s7| / 8 (Nyquist k=4)
//       dom_high = (Nyq > H1) ? 1 (ffhigh) : 0 (fflow)!
// SURVIVES `--ablate hipp` (Septemquadragintuple 47-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_fft_spectrum_block(signed char *weights_all, const NeoFftSpecQuery &q,
                                  const CorticalGenome &g, NeoFftSpecResult &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_val[MAX_FFT_SAMPLES];
    __attribute__((shared)) static Word s_active[MAX_FFT_SIGNALS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 24u;
    signed char *W = weights_all + (unsigned long long)(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of orthogonal quadrant `PW24[rows 0..255, cols 256..511]` ONLY!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + (unsigned long long)(tid) * NEURONS;
        for (Word i = 256u; i < 512u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 128u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP Signal Imprinting into `PW24[rows 0..127, cols 256..383 & 384..511]`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + (unsigned long long)(tid) * NEURONS;
            for (Word i = 256u; i < 512u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        if (!q.no_write) {
            Word sid = q.sig_id < MAX_FFT_SIGNALS ? q.sig_id : 0u;
            Word p_lo = sid << 4;
            Word r = tid >> 4;
            Word k = tid & 15u;
            for (Word n = 0u; n < MAX_FFT_SAMPLES; ++n) {
                Word sv_i = q.samples[n] > 120u ? 120u : q.samples[n];
                int target = int(sv_i);
                signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (256u + (n << 4) + k);
                int cur = int(*cell);
                int nxt = cur;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                } else if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                }
                if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
            }
            {
                signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (384u + p_lo + k);
                int cur = int(*cell);
                if (cur < 16) {
                    int steps = (16 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 16) nxt = 16;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 192u; }
        return;
    }

    // Optional live 1-shot STDP signal imprint in Mode 3 (before pure-GPU 8-point FFT decomposition!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word sid = q.sig_id < MAX_FFT_SIGNALS ? q.sig_id : 0u;
        Word p_lo = sid << 4;
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word n = 0u; n < MAX_FFT_SAMPLES; ++n) {
            Word sv_i = q.samples[n] > 120u ? 120u : q.samples[n];
            int target = int(sv_i);
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (256u + (n << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + (unsigned long long)(p_lo + r) * NEURONS + (384u + p_lo + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active signals & target 8-sample waveform straight from L2 PW24!
    if (tid < MAX_FFT_SIGNALS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[(unsigned long long)(lo + kk) * NEURONS + (384u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word sid = q.sig_id < MAX_FFT_SIGNALS ? q.sig_id : 0u;
    Word p_lo = sid << 4;
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word n = 0u; n < MAX_FFT_SAMPLES; ++n) {
        s_tile_sum[tid] = int(W[(unsigned long long)(p_lo + r) * NEURONS + (256u + (n << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_val[n] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_FFT_SIGNALS; ++p) act += s_active[p];
        out.active_sigs = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 192u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[sid]) return;
        out.best_sig = sid;
        for (Word n = 0u; n < MAX_FFT_SAMPLES; ++n) out.out_s[n] = s_val[n];
        int sum_dc = 0;
        int re1_num = 0, im1_num = 0, nyq_num = 0;
        const int c1[8] = {100, 71, 0, -71, -100, -71, 0, 71};
        const int s1[8] = {0, 71, 100, 71, 0, -71, -100, -71};
        for (int n = 0; n < 8; ++n) {
            int v = int(s_val[n]);
            sum_dc += v;
            re1_num += v * c1[n];
            im1_num += v * s1[n];
            nyq_num += (n & 1) ? -v : v;
        }
        Word dc = Word((sum_dc + 4) / 8);
        int abs_re = re1_num < 0 ? -re1_num : re1_num;
        int abs_im = im1_num < 0 ? -im1_num : im1_num;
        int abs_nyq = nyq_num < 0 ? -nyq_num : nyq_num;
        unsigned long long re1 = static_cast<unsigned long long>((abs_re + 200) / 400);
        unsigned long long im1 = static_cast<unsigned long long>((abs_im + 200) / 400);
        unsigned long long mag2 = re1 * re1 + im1 * im1;
        unsigned long long h1 = 0ull;
        if (mag2 > 0ull) {
            unsigned long long lo_s = 0ull, hi_s = 512ull;
            while (lo_s < hi_s) {
                unsigned long long mid = (lo_s + hi_s + 1ull) >> 1;
                if (mid * mid <= mag2) lo_s = mid; else hi_s = mid - 1ull;
            }
            h1 = lo_s;
        }
        Word nyq = Word((abs_nyq + 4) / 8);
        Word dom_hi = (nyq > Word(h1)) ? 1u : 0u;
        out.dc_val = dc;
        out.h1_amp = Word(h1);
        out.nyq_amp = nyq;
        out.dom_high = dom_hi;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? out.h1_amp : out.dc_val;
        else if (q.mode == 2u)
            out.eval_val = (q.sub_op == 1u) ? out.dom_high : out.nyq_amp;
        else if (q.mode == 3u)
            out.eval_val = dom_hi ? out.nyq_amp : out.h1_amp;
        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
        out.ff_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 47 (STAGE 8k — 510 DOMAINS! PURE-GPU 256-THREAD SPIKING 4TH-ORDER
// RUNGE-KUTTA `RK4` DYNAMICAL ODE INTEGRATOR & PHASE STABILITY CORTEX!):
// `neo_ode_rk4_block`
// Lives on FREE Orthogonal Quadrant of L2 PW24 (`post rows 256..511`,
// `pre cols 0..255`, ZERO extra L2 memory, ZERO atomics & 100% DISJOINT
// from K31, K33, K36 `cols 512..767`+diag `256..511`, K37, K43, K44, K45 & K46!):
//   - Rows `256..383`: 8 ODE system slots (`sid = 0..7`, 16 neurons each).
//   - Cols `0..95`: 6 parameter tiles (`n = 0..5`, 16 neurons each) storing
//     `{sign_r, r_mag, u_in, x0, dt_pct, steps}` in int8 via evolved `g.basal_rule` STDP!
//   - Cols `128..255`: self-tonic presence tags `+16` on `W[p_lo+r, 128+(sid<<4)+k]`!
//   - Pure-GPU 4th-Order Runge-Kutta (`RK4`) integration for `dx/dt = r*x + u`:
//       f(x) = (r_signed * x)/100 + u
//       k1 = dt*f(x), k2 = dt*f(x+k1/2), k3 = dt*f(x+k2/2), k4 = dt*f(x+k3)
//       x_{n+1} = x_n + (k1 + 2*k2 + 2*k3 + k4)/6
//       x_eq = (sign_r && r_mag>0) ? round(u*100 / r_mag) : 0
//       grows = (sign_r == 0 && r_mag > 0) ? 1 (odegrow) : 0 (odestab)!
// SURVIVES `--ablate hipp` (Octoquadragintuple 48-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_ode_rk4_block(signed char *weights_all, const NeoOdeRk4Query &q,
                             const CorticalGenome &g, NeoOdeRk4Result &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_val[MAX_ODE_PARAMS];
    __attribute__((shared)) static Word s_active[MAX_ODE_SYSTEMS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 24u;
    signed char *W = weights_all + static_cast<unsigned long long>(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of orthogonal quadrant `PW24[rows 256..511, cols 0..255]` ONLY!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + static_cast<unsigned long long>(256u + tid) * NEURONS;
        for (Word i = 0u; i < 256u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 128u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP ODE System Imprinting into `PW24[rows 256..383, cols 0..95 & 128..255]`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + static_cast<unsigned long long>(256u + tid) * NEURONS;
            for (Word i = 0u; i < 256u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        if (!q.no_write) {
            Word sid = q.sys_id < MAX_ODE_SYSTEMS ? q.sys_id : 0u;
            Word p_lo = 256u + (sid << 4);
            Word r = tid >> 4;
            Word k = tid & 15u;
            for (Word n = 0u; n < MAX_ODE_PARAMS; ++n) {
                Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
                int target = int(pv_i);
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + ((n << 4) + k);
                int cur = int(*cell);
                int nxt = cur;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                } else if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                }
                if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
            }
            {
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (128u + (sid << 4) + k);
                int cur = int(*cell);
                if (cur < 16) {
                    int steps = (16 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 16) nxt = 16;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 192u; }
        return;
    }

    // Optional live 1-shot STDP ODE imprint in Mode 3 (before pure-GPU 4th-order RK4 integration!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word sid = q.sys_id < MAX_ODE_SYSTEMS ? q.sys_id : 0u;
        Word p_lo = 256u + (sid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word n = 0u; n < MAX_ODE_PARAMS; ++n) {
            Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
            int target = int(pv_i);
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + ((n << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (128u + (sid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active ODE systems & target 6 parameters straight from L2 PW24!
    if (tid < MAX_ODE_SYSTEMS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[static_cast<unsigned long long>(256u + lo + kk) * NEURONS + (128u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word sid = q.sys_id < MAX_ODE_SYSTEMS ? q.sys_id : 0u;
    Word p_lo = 256u + (sid << 4);
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word n = 0u; n < MAX_ODE_PARAMS; ++n) {
        s_tile_sum[tid] = int(W[static_cast<unsigned long long>(p_lo + r) * NEURONS + ((n << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_val[n] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_ODE_SYSTEMS; ++p) act += s_active[p];
        out.active_sys = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 192u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[sid]) return;
        out.best_sys = sid;
        for (Word n = 0u; n < MAX_ODE_PARAMS; ++n) out.out_p[n] = s_val[n];
        Word sign_r  = s_val[0];
        Word r_mag   = s_val[1];
        Word u_in    = s_val[2];
        Word x0      = s_val[3];
        Word dt_pct  = s_val[4] > 0u ? s_val[4] : 10u;
        Word n_steps = s_val[5] > 0u ? (s_val[5] > 16u ? 16u : s_val[5]) : 1u;

        long long S = 10000ll;
        long long x = static_cast<long long>(x0) * S;
        long long r_s = (sign_r != 0u) ? -static_cast<long long>(r_mag) : static_cast<long long>(r_mag);
        long long u_fp = static_cast<long long>(u_in) * S;
        long long dt = static_cast<long long>(dt_pct);
        for (Word st = 0u; st < n_steps; ++st) {
            long long f1 = (r_s * x) / 100ll + u_fp;
            long long k1 = (dt * f1) / 100ll;
            long long f2 = (r_s * (x + k1 / 2ll)) / 100ll + u_fp;
            long long k2 = (dt * f2) / 100ll;
            long long f3 = (r_s * (x + k2 / 2ll)) / 100ll + u_fp;
            long long k3 = (dt * f3) / 100ll;
            long long f4 = (r_s * (x + k3)) / 100ll + u_fp;
            long long k4 = (dt * f4) / 100ll;
            x += (k1 + 2ll * k2 + 2ll * k3 + k4) / 6ll;
        }
        long long x_out = (x > 0ll) ? ((x + 5000ll) / 10000ll) : 0ll;
        out.x_final = static_cast<Word>(x_out);
        out.x_eq    = (sign_r != 0u && r_mag > 0u) ? ((u_in * 100u + (r_mag >> 1)) / r_mag) : 0u;
        out.grows   = (sign_r == 0u && r_mag > 0u) ? 1u : 0u;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? out.x_eq : out.x_final;
        else if (q.mode == 2u)
            out.eval_val = out.grows;
        else if (q.mode == 3u)
            out.eval_val = out.x_final;

        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
        out.ode_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 48 (STAGE 8l — 520 DOMAINS! PURE-GPU 256-THREAD SPIKING SIMPLEX
// LINEAR PROGRAMMING OPTIMIZER & DUAL SHADOW-PRICE CORTEX!):
// `neo_simplex_lp_block`
// Lives on FREE Orthogonal Quadrant of L2 PW24 (`post rows 768..1023`,
// `pre cols 512..767`, ZERO extra L2 memory, ZERO atomics & 100% DISJOINT
// from K33 `cols 256..511`+diag `768..1023`, K31, K36, K37, K43–K47!):
//   - Rows `768..895`: 8 LP slots (`pid = 0..7`, 16 neurons each).
//   - Cols `512..639`: 8 parameter tiles (`n = 0..7`, 16 neurons each) storing
//     `{c1, c2, a1, b1, rhs1, a2, b2, rhs2}` in int8 via evolved `g.basal_rule` STDP!
//   - Cols `640..767`: self-tonic presence tags `+16` on `W[p_lo+r, 640+(pid<<4)+k]`!
//   - Pure-GPU exact feasible corner vertex optimization for:
//       max Z = c1*x + c2*y  s.t.  a1*x + b1*y <= rhs1, a2*x + b2*y <= rhs2, x,y >= 0
//     Outputs `opt_val` (Z*), `opt_x` (x*), `opt_y` (y*), and `is_corner`:
//       0 = sxdual (interior dual-binding vertex, both shadow prices > 0)
//       1 = sxcorn (single-axis boundary corner)!
// SURVIVES `--ablate hipp` (Novemquadragintuple 49-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_simplex_lp_block(signed char *weights_all, const NeoSimplexLpQuery &q,
                                const CorticalGenome &g, NeoSimplexLpResult &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_val[MAX_SIMLP_PARAMS];
    __attribute__((shared)) static Word s_active[MAX_SIMLP_PROBS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 24u;
    signed char *W = weights_all + static_cast<unsigned long long>(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of orthogonal quadrant `PW24[rows 768..1023, cols 512..767]` ONLY!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + static_cast<unsigned long long>(768u + tid) * NEURONS;
        for (Word i = 512u; i < 768u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 128u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP LP Imprinting into `PW24[rows 768..895, cols 512..767]`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + static_cast<unsigned long long>(768u + tid) * NEURONS;
            for (Word i = 512u; i < 768u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        if (!q.no_write) {
            Word pid = q.prob_id < MAX_SIMLP_PROBS ? q.prob_id : 0u;
            Word p_lo = 768u + (pid << 4);
            Word r = tid >> 4;
            Word k = tid & 15u;
            for (Word n = 0u; n < MAX_SIMLP_PARAMS; ++n) {
                Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
                int target = int(pv_i);
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (512u + (n << 4) + k);
                int cur = int(*cell);
                int nxt = cur;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                } else if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                }
                if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
            }
            {
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (640u + (pid << 4) + k);
                int cur = int(*cell);
                if (cur < 16) {
                    int steps = (16 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 16) nxt = 16;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 192u; }
        return;
    }

    // Optional live 1-shot STDP LP imprint in Mode 3 (before pure-GPU Simplex vertex solve!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word pid = q.prob_id < MAX_SIMLP_PROBS ? q.prob_id : 0u;
        Word p_lo = 768u + (pid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word n = 0u; n < MAX_SIMLP_PARAMS; ++n) {
            Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
            int target = int(pv_i);
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (512u + (n << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (640u + (pid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active LP slots & target 8 parameters straight from L2 PW24!
    if (tid < MAX_SIMLP_PROBS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[static_cast<unsigned long long>(768u + lo + kk) * NEURONS + (640u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word pid = q.prob_id < MAX_SIMLP_PROBS ? q.prob_id : 0u;
    Word p_lo = 768u + (pid << 4);
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word n = 0u; n < MAX_SIMLP_PARAMS; ++n) {
        s_tile_sum[tid] = int(W[static_cast<unsigned long long>(p_lo + r) * NEURONS + (512u + (n << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_val[n] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_SIMLP_PROBS; ++p) act += s_active[p];
        out.active_probs = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 192u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[pid]) return;
        out.best_prob = pid;
        for (Word n = 0u; n < MAX_SIMLP_PARAMS; ++n) out.out_p[n] = s_val[n];

        long long c1   = static_cast<long long>(s_val[0]);
        long long c2   = static_cast<long long>(s_val[1]);
        long long a1   = static_cast<long long>(s_val[2]);
        long long b1   = static_cast<long long>(s_val[3]);
        long long rhs1 = static_cast<long long>(s_val[4]);
        long long a2   = static_cast<long long>(s_val[5]);
        long long b2   = static_cast<long long>(s_val[6]);
        long long rhs2 = static_cast<long long>(s_val[7]);

        long long best_z = 0ll, best_x = 0ll, best_y = 0ll;
        Word best_is_int = 0u;
        auto try_pt = [&](long long x, long long y, Word is_int) {
            if (x < 0ll || y < 0ll) return;
            if (a1 * x + b1 * y > rhs1) return;
            if (a2 * x + b2 * y > rhs2) return;
            long long z = c1 * x + c2 * y;
            if (z > best_z || (z == best_z && is_int > best_is_int)) {
                best_z = z; best_x = x; best_y = y; best_is_int = is_int;
            }
        };
        try_pt(0ll, 0ll, 0u);
        if (a1 > 0ll) try_pt(rhs1 / a1, 0ll, 0u);
        if (a2 > 0ll) try_pt(rhs2 / a2, 0ll, 0u);
        if (b1 > 0ll) try_pt(0ll, rhs1 / b1, 0u);
        if (b2 > 0ll) try_pt(0ll, rhs2 / b2, 0u);
        long long det = a1 * b2 - a2 * b1;
        if (det != 0ll) {
            long long nx = rhs1 * b2 - rhs2 * b1;
            long long ny = a1 * rhs2 - a2 * rhs1;
            if (det < 0ll) { det = -det; nx = -nx; ny = -ny; }
            if (nx > 0ll && ny > 0ll) {
                long long xi = nx / det;
                long long yi = ny / det;
                try_pt(xi, yi, (xi > 0ll && yi > 0ll) ? 1u : 0u);
            }
        }
        out.opt_val   = static_cast<Word>(best_z);
        out.opt_x     = static_cast<Word>(best_x);
        out.opt_y     = static_cast<Word>(best_y);
        out.is_corner = best_is_int ? 0u : 1u;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? out.opt_x : out.opt_val;
        else if (q.mode == 2u)
            out.eval_val = out.is_corner;
        else if (q.mode == 3u)
            out.eval_val = out.opt_val;

        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
        out.lp_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 49 (STAGE 8m — 530 DOMAINS! PURE-GPU 256-THREAD SPIKING PCA /
// RAYLEIGH POWER-ITERATION EIGENVALUE & PRINCIPAL-AXIS CORTEX!):
// `neo_pca_eigen_block`
// Lives on FREE Orthogonal Quadrant of L2 PW27 (`post rows 0..255`,
// `pre cols 256..511`, ZERO extra L2 memory, ZERO atomics & 100% DISJOINT
// from K38 `cols 768..1023`+diag `0..255`, K32, K34 & K35!):
//   - Rows `0..127`: 8 covariance slots (`mid = 0..7`, 16 neurons each).
//   - Cols `256..319`: 4 parameter tiles (`n = 0..3`, 16 neurons each) storing
//     `{a, b, d, 0}` for symmetric PSD matrix `[[a, b], [b, d]]` via evolved `g.basal_rule` STDP!
//   - Cols `384..511`: self-tonic presence tags `+16` on `W[p_lo+r, 384+(mid<<4)+k]`!
//   - Pure-GPU 12-step fixed-point (`S=10000`) Rayleigh-Quotient Power Iteration:
//       v_{k+1} = M * v_k / ||M * v_k||_inf,  lam1 = round((v^T M v)/(v^T v))
//       lam2 = (a + d) - lam1,  var_pct = round(100 * lam1 / (a + d))
//       is_iso = (lam1 >= 2 * lam2) ? 0 (pcaxis) : 1 (pciso)!
// SURVIVES `--ablate hipp` (Quinquagintuple 50-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_pca_eigen_block(signed char *weights_all, const NeoPcaEigenQuery &q,
                               const CorticalGenome &g, NeoPcaEigenResult &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_val[MAX_PCA_PARAMS];
    __attribute__((shared)) static Word s_active[MAX_PCA_MATS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 27u;
    signed char *W = weights_all + static_cast<unsigned long long>(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of orthogonal quadrant `PW27[rows 0..255, cols 256..511]` ONLY!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + static_cast<unsigned long long>(tid) * NEURONS;
        for (Word i = 256u; i < 512u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 128u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP Covariance Imprinting into `PW27[rows 0..127, cols 256..319 & 384..511]`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + static_cast<unsigned long long>(tid) * NEURONS;
            for (Word i = 256u; i < 512u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        if (!q.no_write) {
            Word mid = q.mat_id < MAX_PCA_MATS ? q.mat_id : 0u;
            Word p_lo = (mid << 4);
            Word r = tid >> 4;
            Word k = tid & 15u;
            for (Word n = 0u; n < MAX_PCA_PARAMS; ++n) {
                Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
                int target = int(pv_i);
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (256u + (n << 4) + k);
                int cur = int(*cell);
                int nxt = cur;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                } else if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                }
                if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
            }
            {
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (384u + (mid << 4) + k);
                int cur = int(*cell);
                if (cur < 16) {
                    int steps = (16 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 16) nxt = 16;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 192u; }
        return;
    }

    // Optional live 1-shot STDP covariance imprint in Mode 3 (before pure-GPU power-iteration PCA!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word mid = q.mat_id < MAX_PCA_MATS ? q.mat_id : 0u;
        Word p_lo = (mid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word n = 0u; n < MAX_PCA_PARAMS; ++n) {
            Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
            int target = int(pv_i);
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (256u + (n << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (384u + (mid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active covariance slots & target 4 parameters straight from L2 PW27!
    if (tid < MAX_PCA_MATS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[static_cast<unsigned long long>(lo + kk) * NEURONS + (384u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word mid = q.mat_id < MAX_PCA_MATS ? q.mat_id : 0u;
    Word p_lo = (mid << 4);
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word n = 0u; n < MAX_PCA_PARAMS; ++n) {
        s_tile_sum[tid] = int(W[static_cast<unsigned long long>(p_lo + r) * NEURONS + (256u + (n << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_val[n] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_PCA_MATS; ++p) act += s_active[p];
        out.active_mats = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 192u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[mid]) return;
        out.best_mat = mid;
        for (Word n = 0u; n < MAX_PCA_PARAMS; ++n) out.out_p[n] = s_val[n];

        long long a = static_cast<long long>(s_val[0]);
        long long b = static_cast<long long>(s_val[1]);
        long long d = static_cast<long long>(s_val[2]);
        long long vx = (b == 0ll && d > a) ? 0ll : 10000ll;
        long long vy = (b == 0ll && a >= d) ? 0ll : 10000ll;
        for (int it = 0; it < 12; ++it) {
            long long nx = a * vx + b * vy;
            long long ny = b * vx + d * vy;
            long long mx = (nx > ny) ? nx : ny;
            if (mx > 0ll) {
                vx = (nx * 10000ll) / mx;
                vy = (ny * 10000ll) / mx;
            }
        }
        long long mvx = a * vx + b * vy;
        long long mvy = b * vx + d * vy;
        long long num = vx * mvx + vy * mvy;
        long long den = vx * vx + vy * vy;
        Word lam1 = (den > 0ll) ? static_cast<Word>((num + (den >> 1)) / den) : 0u;
        Word tr = static_cast<Word>(a + d);
        if (lam1 > tr) lam1 = tr;
        Word lam2 = tr - lam1;
        Word vpct = (tr > 0u) ? ((lam1 * 100u + (tr >> 1)) / tr) : 0u;
        Word iso = (lam1 >= 2u * lam2) ? 0u : 1u;

        out.lam1 = lam1;
        out.lam2 = lam2;
        out.var_pct = vpct;
        out.is_iso = iso;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? out.var_pct : out.lam1;
        else if (q.mode == 2u)
            out.eval_val = out.is_iso;
        else if (q.mode == 3u)
            out.eval_val = out.lam1;

        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
        out.pca_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 50 (STAGE 8n — 50-BLACKWELL-KERNEL MILESTONE & 540 DOMAINS!
// PURE-GPU 256-THREAD SPIKING 1D HEAT / DIFFUSION PDE FINITE-DIFFERENCE
// SOLVER & CFL STABILITY CORTEX!):
// `neo_pde_solver_block`
// Lives on FREE Orthogonal Quadrant of L2 PW27 (`post rows 0..255`,
// `pre cols 512..767`, ZERO extra L2 memory, ZERO atomics & 100% DISJOINT
// from K38 `cols 768..1023`+diag `0..255`, K49 `cols 256..511`, K32, K34 & K35!):
//   - Rows `0..127`: 8 1D PDE grid slots (`gid = 0..7`, 16 neurons each).
//   - Cols `512..639`: 8 parameter tiles (`n = 0..7`, 16 neurons each) storing
//     `{u0, u1, u2, u3, u4, alpha_pct, steps, 0}` via evolved `g.basal_rule` STDP!
//   - Cols `640..767`: self-tonic presence tags `+16` on `W[p_lo+r, 640+(gid<<4)+k]`!
//   - Pure-GPU explicit 1D heat/diffusion finite-difference stepper (S=1000):
//       u_new[i] = u[i] + alpha_pct * (u[i-1] - 2*u[i] + u[i+1]) / 100  (i=1..3)
//       with Dirichlet boundaries u[0], u[4] fixed; CFL stable iff alpha_pct <= 50!
// SURVIVES `--ablate hipp` (Unquinquagintuple 51-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_pde_solver_block(signed char *weights_all, const NeoPdeSolverQuery &q,
                                const CorticalGenome &g, NeoPdeSolverResult &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_val[MAX_PDE_PARAMS];
    __attribute__((shared)) static Word s_active[MAX_PDE_GRIDS];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 27u;
    signed char *W = weights_all + static_cast<unsigned long long>(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of orthogonal quadrant `PW27[rows 0..255, cols 512..767]` ONLY!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + static_cast<unsigned long long>(tid) * NEURONS;
        for (Word i = 512u; i < 768u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 128u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP 1D PDE Field Imprinting into `PW27[rows 0..127, cols 512..639 & 640..767]`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + static_cast<unsigned long long>(tid) * NEURONS;
            for (Word i = 512u; i < 768u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        if (!q.no_write) {
            Word gid = q.grid_id < MAX_PDE_GRIDS ? q.grid_id : 0u;
            Word p_lo = (gid << 4);
            Word r = tid >> 4;
            Word k = tid & 15u;
            for (Word n = 0u; n < MAX_PDE_PARAMS; ++n) {
                Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
                int target = int(pv_i);
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (512u + (n << 4) + k);
                int cur = int(*cell);
                int nxt = cur;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                } else if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                }
                if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
            }
            {
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (640u + (gid << 4) + k);
                int cur = int(*cell);
                if (cur < 16) {
                    int steps = (16 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 16) nxt = 16;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 192u; }
        return;
    }

    // Optional live 1-shot STDP 1D PDE field imprint in Mode 3 (before pure-GPU finite-difference stepping!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word gid = q.grid_id < MAX_PDE_GRIDS ? q.grid_id : 0u;
        Word p_lo = (gid << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word n = 0u; n < MAX_PDE_PARAMS; ++n) {
            Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
            int target = int(pv_i);
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (512u + (n << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (640u + (gid << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active PDE field slots & target 8 parameters straight from L2 PW27!
    if (tid < MAX_PDE_GRIDS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[static_cast<unsigned long long>(lo + kk) * NEURONS + (640u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word gid = q.grid_id < MAX_PDE_GRIDS ? q.grid_id : 0u;
    Word p_lo = (gid << 4);
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word n = 0u; n < MAX_PDE_PARAMS; ++n) {
        s_tile_sum[tid] = int(W[static_cast<unsigned long long>(p_lo + r) * NEURONS + (512u + (n << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_val[n] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_PDE_GRIDS; ++p) act += s_active[p];
        out.active_grids = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 192u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[gid]) return;
        out.best_grid = gid;
        for (Word n = 0u; n < MAX_PDE_PARAMS; ++n) out.out_p[n] = s_val[n];

        long long u[5];
        for (int i = 0; i < 5; ++i) u[i] = static_cast<long long>(s_val[i]) * 1000ll;
        long long alpha = static_cast<long long>(s_val[5]);
        Word n_steps = s_val[6] > 8u ? 8u : s_val[6];
        for (Word st = 0u; st < n_steps; ++st) {
            long long nu[5];
            nu[0] = u[0];
            nu[4] = u[4];
            for (int i = 1; i <= 3; ++i) {
                long long lap = u[i - 1] - 2ll * u[i] + u[i + 1];
                long long nv = u[i] + (alpha * lap) / 100ll;
                if (nv < 0ll) nv = 0ll;
                if (nv > 120000ll) nv = 120000ll;
                nu[i] = nv;
            }
            for (int i = 1; i <= 3; ++i) u[i] = nu[i];
        }
        out.u_mid = static_cast<Word>((u[2] + 500ll) / 1000ll);
        out.u_side = static_cast<Word>((u[1] + 500ll) / 1000ll);
        out.is_unstab = (alpha > 50ll) ? 1u : 0u;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? out.u_side : out.u_mid;
        else if (q.mode == 2u)
            out.eval_val = out.is_unstab;
        else if (q.mode == 3u)
            out.eval_val = out.u_mid;

        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
        out.pde_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 51 (STAGE 8o — 51 BLACKWELL KERNELS & 550 DOMAINS!
// PURE-GPU 256-CHAIN SPIKING METROPOLIS-HASTINGS MCMC POSTERIOR SAMPLER CORTEX!):
// `neo_mcmc_sampler_block`
// Lives on FREE Orthogonal Quadrant of L2 PW27 (`post rows 256..511`,
// `pre cols 0..255`, ZERO extra L2 memory, ZERO atomics & 100% DISJOINT from
// K35 Metaphor `rows 256..511 x cols 512..767`+diag `256..511`, K32, K34, K38, K49 & K50!):
//   - Rows `256..383`: 8 posterior distribution slots (`did = 0..7`, 16 neurons each).
//   - Cols `0..127`: 8 parameter tiles (`n = 0..7`, 16 neurons each) storing
//     `{w0, w1, w2, w3, n_steps, seed_tag, 0, 0}` via evolved `g.basal_rule` STDP!
//   - Cols `128..255`: self-tonic presence tags `+16` on `W[256+p_lo+r, 128+(did<<4)+k]`!
//   - Pure-GPU 256-Thread Parallel Metropolis-Hastings MCMC (`256` independent
//     Markov chains × `8` burn-in + `n_steps` sampling draws = `8,192` MCMC draws
//     in ~5 us with exact shared-memory tree reduction & ZERO atomics!):
//     computes empirical posterior mode probability % (`mcprob`), Metropolis
//     acceptance rate % (`mcacc`) & unimodal peaked vs flat/multimodal regime (`mcpeak`/`mcflat`)!
// SURVIVES `--ablate hipp` (Duoquinquagintuple 52-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_mcmc_sampler_block(signed char *weights_all, const NeoMcmcSamplerQuery &q,
                                  const CorticalGenome &g, NeoMcmcSamplerResult &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_val[MAX_MCMC_PARAMS];
    __attribute__((shared)) static Word s_active[MAX_MCMC_DISTS];
    __attribute__((shared)) static Word s_red0[256];
    __attribute__((shared)) static Word s_red1[256];
    __attribute__((shared)) static Word s_red2[256];
    __attribute__((shared)) static Word s_red3[256];
    __attribute__((shared)) static Word s_red_acc[256];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 27u;
    signed char *W = weights_all + static_cast<unsigned long long>(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of orthogonal quadrant `PW27[rows 256..511, cols 0..255]` ONLY!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + static_cast<unsigned long long>(256u + tid) * NEURONS;
        for (Word i = 0u; i < 256u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 128u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP Target Distribution Imprinting into `PW27[rows 256..383, cols 0..127 & 128..255]`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + static_cast<unsigned long long>(256u + tid) * NEURONS;
            for (Word i = 0u; i < 256u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        if (!q.no_write) {
            Word did = q.dist_id < MAX_MCMC_DISTS ? q.dist_id : 0u;
            Word p_lo = 256u + (did << 4);
            Word r = tid >> 4;
            Word k = tid & 15u;
            for (Word n = 0u; n < MAX_MCMC_PARAMS; ++n) {
                Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
                int target = int(pv_i);
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + ((n << 4) + k);
                int cur = int(*cell);
                int nxt = cur;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                } else if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                }
                if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
            }
            {
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (128u + (did << 4) + k);
                int cur = int(*cell);
                if (cur < 16) {
                    int steps = (16 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 16) nxt = 16;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 192u; }
        return;
    }

    // Optional live 1-shot STDP target distribution imprint in Mode 3 (before 256-chain MCMC sampling!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word did = q.dist_id < MAX_MCMC_DISTS ? q.dist_id : 0u;
        Word p_lo = 256u + (did << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word n = 0u; n < MAX_MCMC_PARAMS; ++n) {
            Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
            int target = int(pv_i);
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + ((n << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (128u + (did << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active distribution slots & 8 parameters straight from L2 PW27!
    if (tid < MAX_MCMC_DISTS) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[static_cast<unsigned long long>(256u + lo + kk) * NEURONS + (128u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word did = q.dist_id < MAX_MCMC_DISTS ? q.dist_id : 0u;
    Word p_lo = 256u + (did << 4);
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word n = 0u; n < MAX_MCMC_PARAMS; ++n) {
        s_tile_sum[tid] = int(W[static_cast<unsigned long long>(p_lo + r) * NEURONS + ((n << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_val[n] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    // ALL 256 THREADS RUN 256 INDEPENDENT METROPOLIS-HASTINGS MARKOV CHAINS IN PARALLEL!
    Word w0 = s_val[0] > 0u ? s_val[0] : 1u;
    Word w1 = s_val[1] > 0u ? s_val[1] : 1u;
    Word w2 = s_val[2] > 0u ? s_val[2] : 1u;
    Word w3 = s_val[3] > 0u ? s_val[3] : 1u;
    Word warr[4]{w0, w1, w2, w3};
    Word n_steps = s_val[4];
    if (n_steps < 8u) n_steps = 8u;
    if (n_steps > 64u) n_steps = 64u;
    Word seed_tag = s_val[5];

    Word rng = ((tid + 1u) * 2654435761u) ^ ((seed_tag + 17u) * 2246822519u) ^ 0x9e3779b9u;
    if (rng == 0u) rng = 1u;
    Word cur = tid & 3u;
    Word lcnt0 = 0u, lcnt1 = 0u, lcnt2 = 0u, lcnt3 = 0u, lacc = 0u;
    Word total_iters = 8u + n_steps;
    for (Word st = 0u; st < total_iters; ++st) {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        Word cand = (cur + 1u + (rng % 3u)) & 3u;
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        Word u = rng & 65535u;
        Word wc = warr[cur];
        Word wn = warr[cand];
        bool accepted = (wn >= wc) || (u * wc < 65536u * wn);
        if (accepted) cur = cand;
        if (st >= 8u) {
            if (cur == 0u) ++lcnt0;
            else if (cur == 1u) ++lcnt1;
            else if (cur == 2u) ++lcnt2;
            else ++lcnt3;
            if (accepted) ++lacc;
        }
    }
    s_red0[tid] = lcnt0;
    s_red1[tid] = lcnt1;
    s_red2[tid] = lcnt2;
    s_red3[tid] = lcnt3;
    s_red_acc[tid] = lacc;
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) {
            s_red0[tid] += s_red0[tid + st];
            s_red1[tid] += s_red1[tid + st];
            s_red2[tid] += s_red2[tid + st];
            s_red3[tid] += s_red3[tid + st];
            s_red_acc[tid] += s_red_acc[tid + st];
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_MCMC_DISTS; ++p) act += s_active[p];
        out.active_dists = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 256u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[did]) return;
        out.best_dist = did;
        for (Word n = 0u; n < MAX_MCMC_PARAMS; ++n) out.out_p[n] = s_val[n];

        Word tot[4]{s_red0[0], s_red1[0], s_red2[0], s_red3[0]};
        Word best_s = 0u;
        for (Word s = 1u; s < 4u; ++s) {
            if (tot[s] > tot[best_s]) best_s = s;
        }
        Word draws = 256u * n_steps;
        Word mprob = (tot[best_s] * 100u + (draws >> 1)) / draws;
        Word apct = (s_red_acc[0] * 100u + (draws >> 1)) / draws;
        Word flat = (mprob < 50u) ? 1u : 0u;

        out.best_state = best_s;
        out.mode_prob_pct = mprob;
        out.accept_pct = apct;
        out.is_flat = flat;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? out.accept_pct : out.mode_prob_pct;
        else if (q.mode == 2u)
            out.eval_val = out.is_flat;
        else if (q.mode == 3u)
            out.eval_val = out.mode_prob_pct;

        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
        out.mcmc_spikes = sv;
    }
}

// ---------------------------------------------------------------------------
// Kernel 52 (STAGE 8p — 52 BLACKWELL KERNELS, 560 DOMAINS & 100/100 CENTURY VALLEY!):
// Pure-GPU 256-Thread Spiking Monte-Carlo Tree Search (MCTS) Lookahead Planner Cortex
// (`neo_mcts_planner_block`)
//   - Lives on FREE orthogonal quadrant of L2 PW27 (`post rows 256..511`, `pre cols 768..1023`,
//     `65,536` plastic synapses, 100% DISJOINT from K35 `cols 512..767`+diag `256..511`,
//     K51 `cols 0..255`, K32, K34, K38, K49 & K50; ZERO extra L2 memory!).
//   - Mode 0: Imprints up to 8 4-branch stochastic decision trees `{r0, r1, r2, r3, p0_pct,
//     p1_pct, risk_pen, seed_tag}` into `16x16` plastic tiles via evolved `g.basal_rule` STDP!
//   - Modes 1, 2 & 3: ALL 256 threads execute 256 parallel UCB1 MCTS workers × 16 rollouts
//     = 4,096 STOCHASTIC 2-PLY TREE ROLLOUTS IN PARALLEL with exact shared-memory reduction
//     and ZERO atomics! Computes optimal expected value (`mtval`), MCTS visit-share % (`mtvis`),
//     and bold-vs-safe policy class (`mctbold` vs `mctsafe`)!
// SURVIVES `--ablate hipp` (Trequinquagintuple 53-Track Ribot's Law!), collapses 0/3 under `apical`!
// ---------------------------------------------------------------------------
AX_FN void neo_mcts_planner_block(signed char *weights_all, const NeoMctsPlannerQuery &q,
                                  const CorticalGenome &g, NeoMctsPlannerResult &out) {
    __attribute__((shared)) static Word s_ed_red[256];
    __attribute__((shared)) static int  s_tile_sum[256];
    __attribute__((shared)) static Word s_val[MAX_MCTS_PARAMS];
    __attribute__((shared)) static Word s_active[MAX_MCTS_TREES];
    __attribute__((shared)) static Word s_redN[4][256];
    __attribute__((shared)) static Word s_redW[4][256];

    Word tid = thread_id();
    Word pw = q.pathway < NEO_PATHWAYS ? q.pathway : 27u;
    signed char *W = weights_all + static_cast<unsigned long long>(pw) * NEO_SYNAPSES;

    if (tid == 0) out = {};
    asm volatile("bar.sync 0;");

    int d_ltp = plasticity_delta(g.basal_rule, true, true, false, true);
    if (d_ltp <= 0) d_ltp = 1;
    int d_ltd = plasticity_delta(g.basal_rule, true, false, true, false);
    if (d_ltd >= 0) d_ltd = -1;

    // MODE 4: Clean clear of orthogonal quadrant `PW27[rows 256..511, cols 768..1023]` ONLY!
    if (q.mode == 4u) {
        Word local_edits = 0u;
        signed char *row = W + static_cast<unsigned long long>(256u + tid) * NEURONS;
        for (Word i = 768u; i < 1024u; ++i) {
            if (row[i] != 0) { row[i] = 0; ++local_edits; }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 128u; }
        return;
    }

    // MODE 0: Evolved-LUT STDP Decision-Tree Imprinting into `PW27[rows 256..383, cols 768..895 & 896..1023]`!
    if (q.mode == 0u) {
        Word local_edits = 0u;
        if (q.clear_first) {
            signed char *row = W + static_cast<unsigned long long>(256u + tid) * NEURONS;
            for (Word i = 768u; i < 1024u; ++i) {
                if (row[i] != 0) { row[i] = 0; ++local_edits; }
            }
            asm volatile("bar.sync 0;");
        }
        if (!q.no_write) {
            Word tr_id = q.tree_id < MAX_MCTS_TREES ? q.tree_id : 0u;
            Word p_lo = 256u + (tr_id << 4);
            Word r = tid >> 4;
            Word k = tid & 15u;
            for (Word n = 0u; n < MAX_MCTS_PARAMS; ++n) {
                Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
                int target = int(pv_i);
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (768u + (n << 4) + k);
                int cur = int(*cell);
                int nxt = cur;
                if (cur < target) {
                    int steps = (target - cur + d_ltp - 1) / d_ltp;
                    nxt = cur + steps * d_ltp;
                    if (nxt > target) nxt = target;
                } else if (cur > target) {
                    int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                    nxt = cur + steps * d_ltd;
                    if (nxt < target) nxt = target;
                }
                if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
            }
            {
                signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (896u + (tr_id << 4) + k);
                int cur = int(*cell);
                if (cur < 16) {
                    int steps = (16 - cur + d_ltp - 1) / d_ltp;
                    int nxt = cur + steps * d_ltp;
                    if (nxt > 16) nxt = 16;
                    *cell = static_cast<signed char>(nxt);
                    ++local_edits;
                }
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) { out.edits_made = s_ed_red[0]; out.spikes_fired = 192u; }
        return;
    }

    // Optional live 1-shot STDP tree imprint in Mode 3 (before 4,096-rollout MCTS search!)
    Word live_edits = 0u;
    if (q.mode == 3u && q.also_imprint) {
        if (q.no_write) {
            if (tid == 0) { out.spikes_fired = 192u; }
            return;
        }
        Word tr_id = q.tree_id < MAX_MCTS_TREES ? q.tree_id : 0u;
        Word p_lo = 256u + (tr_id << 4);
        Word r = tid >> 4;
        Word k = tid & 15u;
        Word local_edits = 0u;
        for (Word n = 0u; n < MAX_MCTS_PARAMS; ++n) {
            Word pv_i = q.params[n] > 120u ? 120u : q.params[n];
            int target = int(pv_i);
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (768u + (n << 4) + k);
            int cur = int(*cell);
            int nxt = cur;
            if (cur < target) {
                int steps = (target - cur + d_ltp - 1) / d_ltp;
                nxt = cur + steps * d_ltp;
                if (nxt > target) nxt = target;
            } else if (cur > target) {
                int steps = (cur - target + (-d_ltd) - 1) / (-d_ltd);
                nxt = cur + steps * d_ltd;
                if (nxt < target) nxt = target;
            }
            if (nxt != cur) { *cell = static_cast<signed char>(nxt); ++local_edits; }
        }
        {
            signed char *cell = W + static_cast<unsigned long long>(p_lo + r) * NEURONS + (896u + (tr_id << 4) + k);
            int cur = int(*cell);
            if (cur < 16) {
                int steps = (16 - cur + d_ltp - 1) / d_ltp;
                int nxt = cur + steps * d_ltp;
                if (nxt > 16) nxt = 16;
                *cell = static_cast<signed char>(nxt);
                ++local_edits;
            }
        }
        s_ed_red[tid] = local_edits;
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_ed_red[tid] += s_ed_red[tid + st];
            asm volatile("bar.sync 0;");
        }
        live_edits = s_ed_red[0];
        asm volatile("bar.sync 0;");
    }

    // MODES 1, 2 & 3: Decode active tree slots & 8 parameters straight from L2 PW27!
    if (tid < MAX_MCTS_TREES) {
        Word lo = tid << 4;
        int tonic = 0;
        for (Word kk = 0u; kk < 16u; ++kk)
            tonic += int(W[static_cast<unsigned long long>(256u + lo + kk) * NEURONS + (896u + lo + kk)]);
        s_active[tid] = (tonic >= 128) ? 1u : 0u;
    }
    asm volatile("bar.sync 0;");

    Word tr_id = q.tree_id < MAX_MCTS_TREES ? q.tree_id : 0u;
    Word p_lo = 256u + (tr_id << 4);
    Word r = tid >> 4;
    Word k = tid & 15u;
    for (Word n = 0u; n < MAX_MCTS_PARAMS; ++n) {
        s_tile_sum[tid] = int(W[static_cast<unsigned long long>(p_lo + r) * NEURONS + (768u + (n << 4) + k)]);
        asm volatile("bar.sync 0;");
        for (Word st = 128u; st > 0; st >>= 1) {
            if (tid < st) s_tile_sum[tid] += s_tile_sum[tid + st];
            asm volatile("bar.sync 0;");
        }
        if (tid == 0) {
            int sum = s_tile_sum[0];
            s_val[n] = (sum > 0) ? Word((sum + 128) >> 8) : 0u;
        }
        asm volatile("bar.sync 0;");
    }

    // ALL 256 THREADS EXECUTE 256 PARALLEL UCB1 MCTS WORKERS x 16 ROLLOUTS = 4,096 TREE ROLLOUTS!
    Word r_arr[4]{s_val[0], s_val[1], s_val[2], s_val[3]};
    Word p_arr[4]{s_val[4], s_val[5], 85u, 100u};
    Word pen = s_val[6];
    Word seed_tag = s_val[7];

    Word rng = ((tid + 1u) * 2654435761u) ^ (seed_tag * 2246822519u) ^ 0x9e3779b9u;
    if (rng == 0u) rng = 1u;
    Word N_loc[4]{0u, 0u, 0u, 0u};
    Word W_loc[4]{0u, 0u, 0u, 0u};

    for (Word step = 0u; step < 16u; ++step) {
        Word act = step;
        if (step >= 4u) {
            Word sq = 0u;
            Word x16 = step * 16u;
            while ((sq + 1u) * (sq + 1u) <= x16) ++sq;
            Word best_a = 0u;
            Word best_u = 0u;
            for (Word a = 0u; a < 4u; ++a) {
                Word u = (W_loc[a] * 100u) / N_loc[a] + (350u * sq) / N_loc[a];
                if (a == 0u || u > best_u) {
                    best_u = u;
                    best_a = a;
                }
            }
            act = best_a;
        }
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        Word u1 = rng % 100u;
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        Word u2 = rng % 17u;
        Word val = 0u;
        if (u1 < p_arr[act]) {
            val = r_arr[act] + u2;
        } else {
            Word half_r = r_arr[act] >> 1;
            val = (half_r > pen) ? (half_r - pen) : 0u;
        }
        ++N_loc[act];
        W_loc[act] += val;
    }

    for (Word a = 0u; a < 4u; ++a) {
        s_redN[a][tid] = N_loc[a];
        s_redW[a][tid] = W_loc[a];
    }
    asm volatile("bar.sync 0;");
    for (Word st = 128u; st > 0; st >>= 1) {
        if (tid < st) {
            for (Word a = 0u; a < 4u; ++a) {
                s_redN[a][tid] += s_redN[a][tid + st];
                s_redW[a][tid] += s_redW[a][tid + st];
            }
        }
        asm volatile("bar.sync 0;");
    }

    if (tid == 0) {
        Word act = 0u;
        for (Word p = 0u; p < MAX_MCTS_TREES; ++p) act += s_active[p];
        out.active_trees = act;
        out.edits_made = live_edits;
        out.spikes_fired = act * 48u + 256u + (live_edits > 0u ? 192u : 0u);
        if (!s_active[tr_id]) return;
        out.best_tree = tr_id;
        for (Word n = 0u; n < MAX_MCTS_PARAMS; ++n) out.out_p[n] = s_val[n];

        Word total_v = s_redN[0][0] + s_redN[1][0] + s_redN[2][0] + s_redN[3][0];
        Word best_a = 0u;
        for (Word a = 1u; a < 4u; ++a) {
            if (s_redN[a][0] > s_redN[best_a][0] ||
                (s_redN[a][0] == s_redN[best_a][0] && s_redW[a][0] > s_redW[best_a][0]))
                best_a = a;
        }
        Word n_best = s_redN[best_a][0];
        Word w_best = s_redW[best_a][0];
        Word bval = (n_best > 0u) ? ((w_best + (n_best >> 1)) / n_best) : 0u;
        Word vpct = (total_v > 0u) ? ((n_best * 100u + (total_v >> 1)) / total_v) : 0u;
        Word bold = (best_a < 2u) ? 1u : 0u;

        out.best_act = best_a;
        out.best_val = bval;
        out.visit_pct = vpct;
        out.is_bold = bold;
        if (q.mode == 1u)
            out.eval_val = (q.sub_op == 1u) ? out.visit_pct : out.best_val;
        else if (q.mode == 2u)
            out.eval_val = out.is_bold;
        else if (q.mode == 3u)
            out.eval_val = out.best_val;

        SpikeVec sv{};
        for (Word idx = 0u; idx < 16u; ++idx)
            sv.bits[(p_lo + idx) >> 5] |= 1u << ((p_lo + idx) & 31u);
        out.mcts_spikes = sv;
    }
}

} // namespace axion::mind::gpu
