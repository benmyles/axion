#pragma once
#include "mind_core.cuh"

namespace axion::mind::gpu {

// ---------------------------------------------------------------------------
// Prefrontal-Causal Spiking Imagination & Planning Engine (Blackwell CUDA)
// Runs parallel spiking forward rollouts through learned CausalSchema columns
// to find shortest multi-step plans (up to depth 8) and evaluate counterfactuals!
// ---------------------------------------------------------------------------
constexpr Word PLAN_FRONTIER = 128;

struct PlanNode {
    SpikeVec state;
    SpikeVec inv;
    Word parent;
    Word schema;
    Word depth;
    Word cost;
};

AX_FN Word state_signature(const SpikeVec &s, const SpikeVec &inv) {
    Word h = 2166136261u;
    for (Word i = 0; i < WORDS; ++i) {
        h ^= s.bits[i];
        h *= 16777619u;
        h ^= inv.bits[i];
        h *= 16777619u;
    }
    return h;
}

// Check if all required positive spikes in `req` are present in `state` (with dendritic tolerance)
AX_FN bool sdr_satisfies_pos(const SpikeVec &state, const SpikeVec &req, Word &spikes) {
    Word need = sdr_popcount_scalar(req);
    if (need == 0)
        return true;
    Word ov = sdr_overlap_scalar(state, req);
    spikes += ov;
    // Allow tiny SDR noise (at least 75% of required pattern spikes active)
    return ov * 4u >= need * 3u;
}

// Check that forbidden negative spikes are NOT strongly active
AX_FN bool sdr_satisfies_neg(const SpikeVec &state, const SpikeVec &forbid, Word &spikes) {
    Word fcnt = sdr_popcount_scalar(forbid);
    if (fcnt == 0)
        return true;
    Word ov = sdr_overlap_scalar(state, forbid);
    spikes += ov;
    return ov * 2u < fcnt;
}

AX_FN bool sdr_category_matches(const SpikeVec &target_cat, const SpikeVec &schema_cat, Word &spikes) {
    Word sc = sdr_popcount_scalar(schema_cat);
    if (sc == 0)
        return true; // Universal action applicable to any target
    Word ov = sdr_overlap_scalar(target_cat, schema_cat);
    spikes += ov;
    return ov >= 12u || (sc <= 16u && ov * 2u >= sc);
}

// Check if a causal schema can fire in current imagined working-memory state
AX_FN bool schema_fires(const CausalSchema &cs, const SpikeVec &state, const SpikeVec &inv,
                        const SpikeVec &target_cat, Word &spikes) {
    if (!(cs.flags & 1u))
        return false;
    if (!sdr_category_matches(target_cat, cs.target_cat, spikes))
        return false;
    if (!sdr_satisfies_pos(state, cs.req_pos, spikes))
        return false;
    if (!sdr_satisfies_neg(state, cs.req_neg, spikes))
        return false;
    if (!sdr_satisfies_pos(inv, cs.tool_req, spikes))
        return false;
    return true;
}

AX_FN void apply_schema(const CausalSchema &cs, SpikeVec &state, SpikeVec &inv) {
    for (Word i = 0; i < WORDS; ++i) {
        state.bits[i] = (state.bits[i] & ~cs.del_eff.bits[i]) | cs.add_eff.bits[i];
        inv.bits[i] |= cs.produces.bits[i];
    }
}

AX_FN bool goal_reached(const SpikeVec &state, const SpikeVec &inv, const PlanQuery &q, Word &spikes) {
    SpikeVec combined = sdr_or(state, inv);
    if (!sdr_satisfies_pos(combined, q.goal_pos, spikes))
        return false;
    if (!sdr_satisfies_neg(state, q.goal_neg, spikes))
        return false;
    return true;
}

// GPU Spiking Rollout Planner (BFS over imagined spiking state-space in device memory)
AX_FN void run_spiking_plan(const CausalSchema *schemas, const PlanQuery &q, PlanNode *queue, Word *seen_hashes,
                            PlanResult &out) {
    out = {};
    out.blocking_step = ~0u;
    Word spikes = 0;

    // Already at goal?
    if (goal_reached(q.init_state, q.inventory, q, spikes)) {
        out.found = 1;
        out.length = 0;
        out.final_state = q.init_state;
        out.spikes_fired = spikes;
        return;
    }

    Word head = 0, tail = 0;
    queue[tail++] = {q.init_state, q.inventory, ~0u, ~0u, 0, 0};
    seen_hashes[0] = state_signature(q.init_state, q.inventory);
    Word seen_n = 1;

    Word limit_depth = q.max_depth > MAX_PLAN_STEPS ? MAX_PLAN_STEPS : q.max_depth;
    Word n_sch = q.n_schemas > MAX_CAUSAL ? MAX_CAUSAL : q.n_schemas;

    while (head < tail) {
        PlanNode cur = queue[head];
        Word cur_idx = head++;
        ++out.nodes_explored;
        if (cur.depth >= limit_depth)
            continue;

        for (Word s = 0; s < n_sch; ++s) {
            if (s == q.skip_schema)
                continue;
            const auto &cs = schemas[s];
            if (!schema_fires(cs, cur.state, cur.inv, q.target_cat, spikes))
                continue;

            SpikeVec ns = cur.state;
            SpikeVec ni = cur.inv;
            apply_schema(cs, ns, ni);

            Word sig = state_signature(ns, ni);
            bool dup = false;
            for (Word k = 0; k < seen_n; ++k) {
                if (seen_hashes[k] == sig) {
                    dup = true;
                    break;
                }
            }
            if (dup)
                continue;

            if (goal_reached(ns, ni, q, spikes)) {
                // Reconstruct shortest plan path
                Word path[MAX_PLAN_STEPS];
                Word len = 0;
                path[len++] = s;
                Word p = cur_idx;
                while (p != ~0u && queue[p].schema != ~0u && len < MAX_PLAN_STEPS) {
                    path[len++] = queue[p].schema;
                    p = queue[p].parent;
                }
                for (Word k = 0; k < len; ++k)
                    out.schema_ids[k] = path[len - 1 - k];
                out.found = 1;
                out.length = len;
                out.total_cost = cur.cost + cs.energy_cost;
                out.final_state = ns;
                out.spikes_fired = spikes;
                return;
            }

            if (tail < PLAN_FRONTIER) {
                queue[tail++] = {ns, ni, cur_idx, s, cur.depth + 1, cur.cost + cs.energy_cost};
                seen_hashes[seen_n++] = sig;
            }
        }
    }

    out.found = 0;
    out.final_state = q.init_state;
    out.spikes_fired = spikes;
}

// Counterfactual Intervention Simulator:
// Replays a sequence of actions with one step omitted (or altered initial state)
// and identifies the EXACT failing step & missing precondition spikes!
AX_FN void run_counterfactual(const CausalSchema *schemas, const PlanQuery &q, const Word *steps, Word n_steps,
                              Word omit_pos, PlanResult &out) {
    out = {};
    out.blocking_step = ~0u;
    SpikeVec st = q.init_state;
    SpikeVec inv = q.inventory;
    Word spikes = 0;

    for (Word i = 0; i < n_steps && i < MAX_PLAN_STEPS; ++i) {
        if (i == omit_pos)
            continue;
        Word sid = steps[i];
        if (sid >= q.n_schemas) {
            out.blocking_step = i;
            break;
        }
        const auto &cs = schemas[sid];
        if (!schema_fires(cs, st, inv, q.target_cat, spikes)) {
            out.blocking_step = i;
            // Compute exact missing positive precondition spikes (what is missing!)
            for (Word w = 0; w < WORDS; ++w)
                out.missing_precond.bits[w] =
                    (cs.req_pos.bits[w] & ~st.bits[w]) | (cs.tool_req.bits[w] & ~inv.bits[w]);
            break;
        }
        apply_schema(cs, st, inv);
        out.schema_ids[out.length++] = sid;
        out.total_cost += cs.energy_cost;
    }

    out.found = goal_reached(st, inv, q, spikes) ? 1u : 0u;
    out.final_state = st;
    out.spikes_fired = spikes;
    if (!out.found && out.blocking_step == ~0u) {
        // Every remaining action executed, but final goal still not met: report missing goal spikes!
        SpikeVec combined = sdr_or(st, inv);
        for (Word w = 0; w < WORDS; ++w)
            out.missing_precond.bits[w] = q.goal_pos.bits[w] & ~combined.bits[w];
    }
}

} // namespace axion::mind::gpu
