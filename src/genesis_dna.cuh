#pragma once
#include "axion/genesis.hpp"

namespace axion::genesis::gpu {

// ---------------------------------------------------------------------------
// Bare-Metal Blackwell SM 12.0 ONLY PTX Intrinsics (Zero External CUDA Headers!)
// ---------------------------------------------------------------------------
AX_FN u32 tid_x() { u32 v; asm("mov.u32 %0, %%tid.x;" : "=r"(v)); return v; }
AX_FN u32 tid_y() { u32 v; asm("mov.u32 %0, %%tid.y;" : "=r"(v)); return v; }
AX_FN u32 cta_x() { u32 v; asm("mov.u32 %0, %%ctaid.x;" : "=r"(v)); return v; }
AX_FN u32 cta_y() { u32 v; asm("mov.u32 %0, %%ctaid.y;" : "=r"(v)); return v; }
AX_FN u32 ncta_x() { u32 v; asm("mov.u32 %0, %%nctaid.x;" : "=r"(v)); return v; }
AX_FN u32 ntid_x() { u32 v; asm("mov.u32 %0, %%ntid.x;" : "=r"(v)); return v; }
AX_FN u32 ntid_y() { u32 v; asm("mov.u32 %0, %%ntid.y;" : "=r"(v)); return v; }
AX_FN u32 lane_id() { u32 v; asm("mov.u32 %0, %%laneid;" : "=r"(v)); return v; }
AX_FN void sync_block() { asm volatile("bar.sync 0;" ::: "memory"); }

AX_FN float fast_sin(float x) { float r; asm("sin.approx.ftz.f32 %0, %1;" : "=f"(r) : "f"(x)); return r; }
AX_FN float fast_cos(float x) { float r; asm("cos.approx.ftz.f32 %0, %1;" : "=f"(r) : "f"(x)); return r; }
AX_FN float fast_sqrt(float x) { float r; asm("sqrt.approx.ftz.f32 %0, %1;" : "=f"(r) : "f"(x)); return r; }
AX_FN float fast_rcp(float x) { float r; asm("rcp.approx.ftz.f32 %0, %1;" : "=f"(r) : "f"(x)); return r; }
AX_FN float fma_f32(float a, float b, float c) { float r; asm("fma.rn.ftz.f32 %0, %1, %2, %3;" : "=f"(r) : "f"(a), "f"(b), "f"(c)); return r; }
AX_FN float clampf(float x, float lo, float hi) {
    float r;
    asm("max.ftz.f32 %0, %1, %2; min.ftz.f32 %0, %0, %3;" : "=f"(r) : "f"(x), "f"(lo), "f"(hi));
    return r;
}
AX_FN float fabsf_dev(float x) { float r; asm("abs.ftz.f32 %0, %1;" : "=f"(r) : "f"(x)); return r; }

// Single-instruction Blackwell PTX bitfield, ballot & integer ops
AX_FN i32 bfe_s32_c1(u32 src, u32 pos) {
    i32 d;
    asm("bfe.s32 %0, %1, %2, 1;" : "=r"(d) : "r"(src), "r"(pos));
    return d;
}
AX_FN i32 bfe_s32_26_1(u32 src) {
    i32 d;
    asm("bfe.s32 %0, %1, 26, 1;" : "=r"(d) : "r"(src));
    return d;
}
AX_FN u32 bfe_u32_23_3(u32 src) {
    u32 d;
    asm("bfe.u32 %0, %1, 23, 3;" : "=r"(d) : "r"(src));
    return d;
}
AX_FN i32 bfe_s32_8_8(u32 src) {
    i32 d;
    asm("bfe.s32 %0, %1, 8, 8;" : "=r"(d) : "r"(src));
    return d;
}
AX_FN u32 bfe_u32(u32 src, u32 pos, u32 len) {
    u32 d;
    asm("bfe.u32 %0, %1, %2, %3;" : "=r"(d) : "r"(src), "r"(pos), "r"(len));
    return d;
}
AX_FN i32 bfe_s32(u32 src, u32 pos, u32 len) {
    i32 d;
    asm("bfe.s32 %0, %1, %2, %3;" : "=r"(d) : "r"(src), "r"(pos), "r"(len));
    return d;
}
AX_FN u32 bfi_b32(u32 ins, u32 base, u32 pos, u32 len) {
    u32 d;
    asm("bfi.b32 %0, %1, %2, %3, %4;" : "=r"(d) : "r"(ins), "r"(base), "r"(pos), "r"(len));
    return d;
}
AX_FN u32 popc_u32(u32 x) {
    u32 d;
    asm("popc.b32 %0, %1;" : "=r"(d) : "r"(x));
    return d;
}
AX_FN u32 max_u32(u32 a, u32 b) { u32 d; asm("max.u32 %0, %1, %2;" : "=r"(d) : "r"(a), "r"(b)); return d; }
AX_FN u32 min_u32(u32 a, u32 b) { u32 d; asm("min.u32 %0, %1, %2;" : "=r"(d) : "r"(a), "r"(b)); return d; }
AX_FN i32 clamp_i32(i32 x, i32 lo, i32 hi) {
    i32 d;
    asm("max.s32 %0, %1, %2; min.s32 %0, %0, %3;" : "=r"(d) : "r"(x), "r"(lo), "r"(hi));
    return d;
}
AX_FN u32 warp_ballot(bool pred, u32 mask = 0xffffffffu) {
    u32 out;
    u32 p = pred ? 1u : 0u;
    asm volatile("{ .reg .pred %%p; setp.ne.u32 %%p, %1, 0; vote.sync.ballot.b32 %0, %%p, %2; }"
                 : "=r"(out) : "r"(p), "r"(mask));
    return out;
}

// Blackwell L2-resident 128-bit vectorized loads & writeback stores (4 synapses per instruction!)
AX_FN void ld_syn_quad(const SynQuad *ptr, u32 &x, u32 &y, u32 &z, u32 &w) {
    asm("ld.global.L1::evict_last.L2::256B.v4.u32 {%0, %1, %2, %3}, [%4];"
        : "=r"(x), "=r"(y), "=r"(z), "=r"(w) : "l"(ptr));
}
AX_FN void st_syn_quad(SynQuad *ptr, u32 x, u32 y, u32 z, u32 w) {
    asm volatile("st.global.wb.v4.u32 [%0], {%1, %2, %3, %4};"
                 :: "l"(ptr), "r"(x), "r"(y), "r"(z), "r"(w));
}
AX_FN u64 ld_l2_u64(const u64 *ptr) {
    u64 v;
    asm("ld.global.L1::evict_last.u64 %0, [%1];" : "=l"(v) : "l"(ptr));
    return v;
}
AX_FN float ld_nc_f32(const float *ptr) {
    float v;
    asm("ld.global.nc.L1::evict_last.f32 %0, [%1];" : "=f"(v) : "l"(ptr));
    return v;
}
AX_FN u8 ld_nc_u8(const u8 *ptr) {
    u32 v;
    asm("ld.global.nc.L1::evict_last.u8 %0, [%1];" : "=r"(v) : "l"(ptr));
    return u8(v);
}
AX_FN u32 ld_nc_u32(const u32 *ptr) {
    u32 v;
    asm("ld.global.nc.L1::evict_last.u32 %0, [%1];" : "=r"(v) : "l"(ptr));
    return v;
}
AX_FN u64 ld_nc_u64(const u64 *ptr) {
    u64 v;
    asm("ld.global.nc.L1::evict_last.u64 %0, [%1];" : "=l"(v) : "l"(ptr));
    return v;
}

AX_FN u32 atomic_add_u32(u32 *addr, u32 val) {
    u32 old;
    asm volatile("atom.global.add.u32 %0, [%1], %2;" : "=r"(old) : "l"(addr), "r"(val));
    return old;
}
AX_FN u32 atomic_add_gen_u32(u32 *addr, u32 val) {
    u32 old;
    asm volatile("atom.add.u32 %0, [%1], %2;" : "=r"(old) : "l"(addr), "r"(val));
    return old;
}
AX_FN u32 atomic_or_gen_u32(u32 *addr, u32 val) {
    u32 old;
    asm volatile("atom.or.b32 %0, [%1], %2;" : "=r"(old) : "l"(addr), "r"(val));
    return old;
}
AX_FN u32 atomic_max_gen_u32(u32 *addr, u32 val) {
    u32 old;
    asm volatile("atom.max.u32 %0, [%1], %2;" : "=r"(old) : "l"(addr), "r"(val));
    return old;
}
AX_FN u32 atomic_exch_gen_u32(u32 *addr, u32 val) {
    u32 old;
    asm volatile("atom.exch.b32 %0, [%1], %2;" : "=r"(old) : "l"(addr), "r"(val));
    return old;
}
AX_FN u64 atomic_add_u64(u64 *addr, u64 val) {
    u64 old;
    asm volatile("atom.global.add.u64 %0, [%1], %2;" : "=l"(old) : "l"(addr), "l"(val));
    return old;
}
AX_FN float atomic_add_f32(float *addr, float val) {
    float old;
    asm volatile("atom.global.add.f32 %0, [%1], %2;" : "=f"(old) : "l"(addr), "f"(val));
    return old;
}
AX_FN u32 atomic_cas_u32(u32 *addr, u32 cmp, u32 val) {
    u32 old;
    asm volatile("atom.global.cas.b32 %0, [%1], %2, %3;" : "=r"(old) : "l"(addr), "r"(cmp), "r"(val));
    return old;
}
AX_FN u32 atomic_or_u32(u32 *addr, u32 val) {
    u32 old;
    asm volatile("atom.global.or.b32 %0, [%1], %2;" : "=r"(old) : "l"(addr), "r"(val));
    return old;
}
AX_FN u32 atomic_and_u32(u32 *addr, u32 val) {
    u32 old;
    asm volatile("atom.global.and.b32 %0, [%1], %2;" : "=r"(old) : "l"(addr), "r"(val));
    return old;
}
AX_FN u32 atomic_exch_u32(u32 *addr, u32 val) {
    u32 old;
    asm volatile("atom.global.exch.b32 %0, [%1], %2;" : "=r"(old) : "l"(addr), "r"(val));
    return old;
}
AX_FN void membar_gl() { asm volatile("membar.gl;"); }
AX_FN u32 warp_add_u32(u32 val, u32 mask = 0xffffffffu) {
    u32 sum;
    asm volatile("bar.warp.sync %2; redux.sync.add.u32 %0, %1, %2;" : "=r"(sum) : "r"(val), "r"(mask));
    return sum;
}

// Lock-free GPU Slab Allocator (32-bit slab IDs — scales to 262,144+ tissue & SynSlabs!)
AX_FN u32 alloc_slab(u32 *bitmap, u32 max_slabs, u32 hint) {
    u32 nwords = (max_slabs + 31u) >> 5;
    if (nwords == 0u) return INVALID_SLAB;
    u32 start = hint % nwords;
    for (u32 step = 0; step < nwords; ++step) {
        u32 w = (start + step) % nwords;
        u32 cur = bitmap[w];
        if (cur == 0xffffffffu) continue;
        u32 inv = ~cur;
        while (inv) {
            u32 bit = u32(__builtin_ctz(inv));
            u32 slot = (w << 5) | bit;
            if (slot >= max_slabs) break;
            u32 mask = 1u << bit;
            u32 old = atomic_or_u32(bitmap + w, mask);
            if ((old & mask) == 0u) return slot;
            inv = ~old;
        }
    }
    return INVALID_SLAB;
}

AX_FN void free_slab(u32 *bitmap, u32 slot) {
    if (slot == INVALID_SLAB) return;
    atomic_and_u32(bitmap + (slot >> 5), ~(1u << (slot & 31u)));
}

AX_FN float rand_unit(u32 &rng) {
    rng = mix(rng + 0x9e3779b9u);
    return float(rng & 0x00ffffffu) * (1.0f / 16777216.0f);
}

AX_FN i8 rand_i8(u32 &rng, i32 lo, i32 hi) {
    rng = mix(rng + 0x7f4a7c15u);
    u32 span = u32(hi - lo + 1);
    return i8(lo + i32(rng % span));
}

AX_FN CARule make_rule(u8 ms, u8 mtk, i8 c0min, i8 c0max, i8 c1min, i8 c1max, u8 op, u8 dir, u8 a, u8 b,
                       i8 p0, i8 p1, u8 csel, u8 spri, u16 tag) {
    CARule r{};
    r.match_state = ms; r.match_tk = mtk;
    r.chem_min[0] = c0min; r.chem_min[1] = c1min;
    r.chem_max[0] = c0max; r.chem_max[1] = c1max;
    r.opcode = op; r.dir_mask = dir; r.arg_a = a; r.arg_b = b;
    r.param0 = p0; r.param1 = p1; r.chem_sel = csel; r.stage_pri = spri; r.tag_bits = tag;
    return r;
}

// ---------------------------------------------------------------------------
// Primordial Founder Unified CA-DNA Generator (~28 Mutable Rules, 100% Open-Ended!)
// Starts as a minimal viable cephalized flatworm/larva developmental seed that
// proliferates from ONE zygote across Steps 0..4 into a ~30-55 cell unified
// body+brain (1..3 layers), differentiates paired eyes, binaural ears, snout
// chemoreceptors, pyramidal+PV+SST+MOD brain nuclei, muscles, mandibles, glands
// & armor, and wires them via crawling axon growth cones (100% free to mutate,
// duplicate rules via Ohno duplication, bud up to 16 layers & 128-syn hubs!).
// ---------------------------------------------------------------------------
AX_FN GenesisDNA make_founder_dna(u32 seed) {
    GenesisDNA d{};
    u32 r = mix(seed ^ 0x517cc1b7u);
    u32 nr = 0u;

    // 1. Primordial Polarity & Morphogen Signaling (3 rules)
    // Channel 0: somatic growth energy, Channel 1: anterior(+) vs posterior(-) axis,
    // Channel 2: medial(0) vs lateral(+) axis, Channel 3: neural/activity BDNF neurotrophin!
    d.rules[nr++] = make_rule(0u, 0u, -120, 120, -120, 120, OP_SIGNAL, 0u, 1u, 0u,
                              rand_i8(r, 18, 36), 0, 0x10u, 1u, 0x0001u);
    d.rules[nr++] = make_rule(2u, 0u, -120, 120, -120, 120, OP_SIGNAL, 0u, 3u, 0u,
                              rand_i8(r, 26, 48), 0, 0x31u, 2u, 0x0003u);
    d.rules[nr++] = make_rule(4u, 0u, -120, 120, -120, 120, OP_SIGNAL, 0u, 3u, 0u,
                              rand_i8(r, 20, 42), 0, 0x31u, 2u, 0x0023u);

    // 2. Asymmetric Cell Division Rules (Proliferate compact hydrodynamic body + rich multi-layer brain!)
    // Seed(0) buds anterior head/brain progenitor(2) northward (-y) & stays core(1)
    d.rules[nr++] = make_rule(0u, 1u, -120, 120, -120, 120, OP_MITOSIS, 0x48u, 1u, 2u,
                              rand_i8(r, 6, 8), rand_i8(r, 20, 36), 0x10u, 6u, 0x0002u);
    // Trunk core(1) buds posterior/flank effector primordia(3) (±x, +y)
    d.rules[nr++] = make_rule(1u, 1u, -120, 120, -120, 120, OP_MITOSIS, 0x47u, 1u, 3u,
                              rand_i8(r, 3, 5), rand_i8(r, -28, -14), 0x10u, 5u, 0x0004u);
    // Posterior trunk(3) expands one compact ring of flanking muscles/glands/armor(8)
    d.rules[nr++] = make_rule(3u, 1u, -60, 4, -120, 38, OP_MITOSIS, 0x47u, 3u, 8u,
                              rand_i8(r, 1, 3), rand_i8(r, -22, -8), 0x21u, 4u, 0x0040u);
    // Anterior head(2) buds sensory brow(5) anteriorly (-y) and transitions into central neural stem(4)!
    d.rules[nr++] = make_rule(2u, 1u, -120, 120, -120, 120, OP_MITOSIS, 0x4bu, 4u, 5u,
                              rand_i8(r, 4, 6), rand_i8(r, 14, 30), 0x21u, 5u, 0x0013u);
    // Sensory brow(5) widens bilaterally (±x) for paired stereo Eyes, lateral binaural Ears & midline snout Chem!
    d.rules[nr++] = make_rule(5u, 1u, 4, 120, -120, 44, OP_MITOSIS, 0x43u, 5u, 5u,
                              rand_i8(r, 2, 4), rand_i8(r, 8, 20), 0x21u, 4u, 0x0010u);
    // Central neural stem(4) proliferates compact L0 head ganglion AND buds Layer +z deep cortex(6)!
    d.rules[nr++] = make_rule(4u, 1u, -120, 120, -120, 120, OP_MITOSIS, 0x5fu, 4u, 6u,
                              rand_i8(r, 3, 5), rand_i8(r, 10, 26), 0x31u, 6u, 0x0027u);
    // Deep cortex stem(6) undergoes oSVZ transit-amplifying cranial sheet expansion AND buds Layer z+2(7)!
    d.rules[nr++] = make_rule(6u, 1u, -120, 120, -120, 120, OP_MITOSIS, 0x5fu, 6u, 7u,
                              rand_i8(r, 10, 13), rand_i8(r, 4, 20), 0x31u, 5u, 0x002fu);
    // Layer z+2 association/working-memory stem(7) expands across the 16x16 cortical sheet & buds deeper z+3..z+31!
    // Ohno-duplicated NOTCH2NL oSVZ radial-glia paralog prevents a single point mutation/deletion from amputating L3..L31!
    d.rules[nr++] = make_rule(7u, 1u, -120, 120, -120, 120, OP_MITOSIS, 0x5fu, 7u, 7u,
                              rand_i8(r, 10, 13), rand_i8(r, -8, 12), 0x31u, 4u, 0x002fu);
    d.rules[nr++] = make_rule(7u, 1u, -120, 120, -120, 120, OP_MITOSIS, 0x5fu, 7u, 7u,
                              rand_i8(r, 10, 13), rand_i8(r, -6, 10), 0x21u, 4u, 0x002fu);

    // 3. Organ & Spiking Neuron Differentiation (Fires AFTER proliferative phase — ZERO hardcoded stencils!)
    // Authentic Vertebrate Embryonic Placode Separation (crisp 3-4 stereo Eyes + 3-4 binaural Ears — zero eye-tumor drag!):
    //   - Anterior optic brow (state 5: m1>=6, m2 in [4, 22]) + forebrain optic-vesicle (state 4: m1 in [22, 33], m2 in [4, 22]) -> paired stereo Eyes!
    //   - Lateral otic brow (state 5: m2 >= 23) + posterior temporal (state 5: m1 <= 5, m2 >= 12) + lateral-line flank (state 3: m2 >= 18) -> paired binaural Ears!
    //   - Midline snout contact Chem (state 5: m1 >= 10, m2 <= 3)
    d.rules[nr++] = make_rule(5u, 1u, 6, 127, 4, 17, OP_DIFF, 0u, TK_EYE, 0u, 0, 0, 0x21u, 11u, 0x0011u);
    d.rules[nr++] = make_rule(4u, 1u, 22, 33, 4, 17, OP_DIFF, 0u, TK_EYE, 0u, 0, 0, 0x21u, 10u, 0x0011u);
    d.rules[nr++] = make_rule(5u, 1u, -2, 127, 18, 127, OP_DIFF, 0u, TK_EAR, 0u, 0, 0, 0x21u, 11u, 0x0012u);
    d.rules[nr++] = make_rule(5u, 1u, -24, 5, 10, 127, OP_DIFF, 0u, TK_EAR, 0u, 0, 0, 0x21u, 11u, 0x0012u);
    d.rules[nr++] = make_rule(3u, 1u, -16, 127, 13, 127, OP_DIFF, 0u, TK_EAR, 0u, 0, 0, 0x21u, 10u, 0x0012u);
    d.rules[nr++] = make_rule(5u, 1u, 10, 127, -120, 1, OP_DIFF, 0u, TK_CHEM, 0u, 0, 0, 0x21u, 10u, 0x0014u);
    // Neural primordia (L0 state 4 + deep CNS states 6, 7):
    // Tight peripheral morphogen PV/SST domains paired with MGE/CGE tangential migration (`mig_h % 13`) ->
    // rock-steady canonical ~77% PYR / ~12% PV / ~11% SST across 50k+ ticks without mutating into 35% PV hypertrophy!
    d.rules[nr++] = make_rule(4u, 1u, -120, 127, -120, 127, OP_DIFF, 0u, TK_NEU_EXC, 0u, 0, 0, 0x21u, 5u, 0x0021u);
    d.rules[nr++] = make_rule(4u, 1u, 44, 127, 16, 127, OP_DIFF, 0u, TK_NEU_PV, 0u, 0, 0, 0x21u, 8u, 0x0025u);
    d.rules[nr++] = make_rule(4u, 1u, -120, -38, 16, 127, OP_DIFF, 0u, TK_NEU_SST, 1u, 0, 0, 0x21u, 8u, 0x0025u);
    d.rules[nr++] = make_rule(4u, 1u, -4, 4, -4, 4, OP_DIFF, 0u, TK_NEU_MOD, 0u, 0, 0, 0x21u, 9u, 0x0029u);
    d.rules[nr++] = make_rule(6u, 1u, -120, 127, -120, 127, OP_DIFF, 0u, TK_NEU_EXC, 2u, 0, 0, 0x21u, 5u, 0x0023u);
    d.rules[nr++] = make_rule(6u, 1u, 46, 127, 20, 127, OP_DIFF, 0u, TK_NEU_PV, 2u, 0, 0, 0x21u, 8u, 0x0025u);
    d.rules[nr++] = make_rule(6u, 1u, -120, -40, 20, 127, OP_DIFF, 0u, TK_NEU_SST, 3u, 0, 0, 0x21u, 8u, 0x0025u);
    d.rules[nr++] = make_rule(7u, 1u, -120, 127, -120, 127, OP_DIFF, 0u, TK_NEU_EXC, 3u, 0, 0, 0x21u, 5u, 0x0023u);
    d.rules[nr++] = make_rule(7u, 1u, 46, 127, 20, 127, OP_DIFF, 0u, TK_NEU_PV, 1u, 0, 0, 0x21u, 8u, 0x0025u);
    d.rules[nr++] = make_rule(7u, 1u, -120, -40, 20, 127, OP_DIFF, 0u, TK_NEU_SST, 1u, 0, 0, 0x21u, 8u, 0x0025u);
    // Trunk & posterior effectors (states 1, 3, 8): non-overlapping Mouth(GRASP) + Lateral Fins(MUSCLE) + Dual-Band Syrinx(GLAND) + Armor!
    d.rules[nr++] = make_rule(3u, 1u, -120, 8, -120, 13, OP_DIFF, 0u, TK_MUSCLE, 0u, 0, 0, 0x21u, 8u, 0x0041u);
    d.rules[nr++] = make_rule(8u, 1u, -120, 6, -120, 20, OP_DIFF, 0u, TK_MUSCLE, 0u, 0, 0, 0x21u, 7u, 0x0041u);
    d.rules[nr++] = make_rule(1u, 1u, -20, 127, -120, 6, OP_DIFF, 0u, TK_GRASP, 0u, 0, 0, 0x21u, 8u, 0x0042u);
    // Compact 3-Fold Syrinx niche strictly OUTSIDE GRASP (m1>=-6, m2<=6) & core tail MUSCLE (state 3), inside ARMOR (m2>=23):
    // Lateral pair on state 1/8 + posterior-trunk midline glottal fold on state 1 (m1 in [-20,-7], m2<=4) -> full 3-fold anatomy + full muscles!
    d.rules[nr++] = make_rule(1u, 1u, -6, 127, 7, 20, OP_DIFF, 0u, TK_GLAND, 0u, 0, 0, 0x21u, 9u, 0x0044u);
    d.rules[nr++] = make_rule(8u, 1u, -48, 24, 15, 22, OP_DIFF, 0u, TK_GLAND, 0u, 0, 0, 0x21u, 9u, 0x0044u);
    d.rules[nr++] = make_rule(1u, 1u, -24, 4, -120, 4, OP_DIFF, 0u, TK_GLAND, 0u, 0, 0, 0x21u, 9u, 0x0044u);
    d.rules[nr++] = make_rule(8u, 1u, -120, -16, 23, 127, OP_DIFF, 0u, TK_STRUCT, 0u, 0, 0, 0x21u, 8u, 0x0048u);

    // 4. Crawling Discrete Axon Growth Cones with Authentic Mammalian Receptor Families (stage_pri bits 6..7 = rule_id 0..3!):
    // Ohno-redundant sensory (optic/auditory) & cortico-cortical pyramidal (IT feedforward + CT/FB apical) guidance genes
    // prevent accidental single-mutation cortical disconnection (maintains canonical ~82% E / ~18% I synaptic ratio!).
    // Sensory optic/ear/chem nerves (rule_id=2: NMDA+ACh/BAC predictive associative binding!)
    d.rules[nr++] = make_rule(0xffu, 2u, -120, 127, -120, 127, OP_GROWTH_CONE, 0xbfu, 1u, 0x07u,
                              rand_i8(r, 34, 56), rand_i8(r, -24, -6), 0x31u, u8(0x80u | 7u), 0x0020u);
    d.rules[nr++] = make_rule(0xffu, 2u, -120, 127, -120, 127, OP_GROWTH_CONE, 0xbfu, 1u, 0x05u,
                              rand_i8(r, 30, 52), rand_i8(r, -18, 6), 0x21u, u8(0x80u | 7u), 0x0020u);
    // Ascending feedforward & local recurrent pyramidal tracts + descending apical feedback tract (rule_id=2!)
    d.rules[nr++] = make_rule(0xffu, 3u, -120, 127, -120, 127, OP_GROWTH_CONE, 0xbfu, 1u, u8(0x04u | (mix(r += 19u) & 3u)),
                              rand_i8(r, 26, 50), rand_i8(r, -10, 44), 0x31u, u8(0x80u | 6u), 0x0020u);
    d.rules[nr++] = make_rule(0xffu, 3u, -120, 127, -120, 127, OP_GROWTH_CONE, 0xbfu, 1u, u8(0x00u | (mix(r += 29u) & 3u)),
                              rand_i8(r, 24, 48), rand_i8(r, -16, 36), 0x23u, u8(0x80u | 6u), 0x0020u);
    // Descending lateralized Go motor tracts (rule_id=0/1: D1 Appetitive & Fast Reversal -> MUSCLE / GRASP / GLAND!)
    d.rules[nr++] = make_rule(0xffu, 3u, -120, 127, -120, 127, OP_GROWTH_CONE, 0xbfu, 2u, 0x01u,
                              rand_i8(r, 34, 56), rand_i8(r, -24, 24), 0x13u, u8(0x00u | 6u), 0x0040u);
    d.rules[nr++] = make_rule(0xffu, 3u, -120, 127, -120, 127, OP_GROWTH_CONE, 0xbfu, 2u, 0x06u,
                              rand_i8(r, 32, 52), rand_i8(r, -18, 18), 0x21u, u8(0x40u | 6u), 0x0044u);
    // Inhibitory PV/SST & Modulatory VTA tracts (rule_id=3: dense local basket-cell & apical Martinotti arbors!)
    d.rules[nr++] = make_rule(0xffu, 4u, -120, 127, -120, 127, OP_GROWTH_CONE, 0xbfu, 1u, u8(mix(r += 37u) & 0x07u),
                              rand_i8(r, 28, 52), rand_i8(r, -24, 34), 0x23u, u8(0xc0u | 7u), 0x0020u);
    d.rules[nr++] = make_rule(0xffu, 4u, -120, 127, -120, 127, OP_GROWTH_CONE, 0x4fu, 1u, u8(mix(r += 41u) & 0x07u),
                              rand_i8(r, 30, 54), rand_i8(r, -18, 18), 0x12u, u8(0xc0u | 7u), 0x0020u);

    // Add 3 randomized exploratory rules (Mitosis/Signal/Diff/GrowthCone) for rich Generation-1 variance!
    for (u32 ex = 0; ex < 3u && nr < MAX_DNA_RULES; ++ex) {
        u8 op = u8(mix(r += 53u) % 4u);
        u8 a = (op == OP_DIFF) ? u8(TK_STRUCT + (mix(r += 11u) % (TK_TYPES - TK_STRUCT)))
               : ((op == OP_MITOSIS) ? u8(mix(r += 13u) & 7u) : u8(mix(r += 17u) & 3u));
        u8 r_ms = u8(mix(r += 23u) & 7u);
        u8 r_mtk = u8(mix(r += 29u) % 5u);
        i8 r_c0min = rand_i8(r, -90, 20), r_c0max = rand_i8(r, 20, 120);
        i8 r_c1min = rand_i8(r, -90, 20), r_c1max = rand_i8(r, 20, 120);
        u8 r_dir = u8(mix(r += 31u) & 0xffu);
        u8 r_b = u8(mix(r += 41u) & 0x3fu);
        i8 r_p0 = rand_i8(r, -40, 48), r_p1 = rand_i8(r, -32, 32);
        u8 r_csel = u8(mix(r += 43u) & 0x33u);
        u8 r_spri = u8(mix(r += 47u) & 0xffu);
        u16 r_tag = u16(0x0020u | (mix(r += 59u) & 0x007fu));
        d.rules[nr++] = make_rule(r_ms, r_mtk, r_c0min, r_c0max, r_c1min, r_c1max,
                                  op, r_dir, a, r_b, r_p0, r_p1, r_csel, r_spri, r_tag);
    }

    // Jitter every primordial rule per founder for high Generation-1 standing genetic variance!
    for (u32 i = 0; i < nr; ++i) {
        d.rules[i].chem_min[0] = i8(clamp_i32(i32(d.rules[i].chem_min[0]) + i32(rand_i8(r, -12, 12)), -125, 120));
        d.rules[i].chem_max[0] = i8(clamp_i32(i32(d.rules[i].chem_max[0]) + i32(rand_i8(r, -12, 12)), -120, 127));
        d.rules[i].param0 = i8(clamp_i32(i32(d.rules[i].param0) + i32(rand_i8(r, -8, 8)), -120, 120));
    }

    for (u32 i = 0; i < 4u; ++i) d.plast_lut[i] = mix(r += 0x85ebca6bu);
    for (u32 i = 0; i < 12u; ++i) d.bio_params[i] = rand_i8(r, -56, 56);
    d.bio_params[7] = rand_i8(r, -16, 64); // bilateral symmetry gene (evolvable!)
    d.num_rules = u8(nr);
    d.max_layers_gene = u8(10u + (mix(r += 91u) % 3u)); // Founders start 10-12 cortical layers (~1,700-2,300 neurons!), expand freely up to 32 (8,192 neurons)!
    d.birth_steps = u8(18u + (mix(r += 97u) & 1u)); // Decoupled oSVZ proliferation (z=0 finishes <8; z>=1 grows full 120-200n deep cortical sheets!)
    d.adult_steps = u8(2u + (mix(r += 101u) & 1u));
    return d;
}

// ---------------------------------------------------------------------------
// Biologically Realistic CA-Rulebook Mutation, Crossover & Gene Duplication!
// ---------------------------------------------------------------------------
AX_FN GenesisDNA mutate_dna(const GenesisDNA &pa, const GenesisDNA &pb, u32 seed, bool crossover) {
    GenesisDNA out = pa;
    u32 r = mix(seed ^ 0xa5a55a5au);
    u32 na = clamp_i32(i32(pa.num_rules), 6, i32(MAX_DNA_RULES));
    u32 nb = clamp_i32(i32(pb.num_rules), 6, i32(MAX_DNA_RULES));

    // Sexual rule-level homologous crossover!
    if (crossover) {
        u32 cut = (mix(r += 31u) % na);
        for (u32 i = cut; i < na; ++i) {
            if (i < nb) out.rules[i] = pb.rules[i];
        }
        if ((r & 1u) != 0u) {
            for (u32 i = 2u; i < 4u; ++i) out.plast_lut[i] = pb.plast_lut[i];
            for (u32 i = 6u; i < 12u; ++i) out.bio_params[i] = pb.bio_params[i];
        }
    }

    // Fine-grained byte mutations across active CA rules + plasticity LUTs + bio_params
    for (u32 i = 0; i < na; ++i) {
        u8 *rb = reinterpret_cast<u8 *>(&out.rules[i]);
        for (u32 b = 0; b < 16u; ++b) {
            r = mix(r + i * 131u + b * 17u + 13u);
            u32 roll = r & 511u;
            if (roll < 7u) {
                if (b == 1u) {
                    // Keep match_tk in clean biological tissue classes (0=any, 1=stem, 2=sensor, 3=all-neuron, 4=interneuron)
                    // so single bit flips never lock axonogenesis to EXC-only and orphan PV/SST interneurons!
                    rb[b] = u8((r >> 12) % 5u);
                } else if (b == 6u) {
                    // Mutate opcode rarely
                    if ((r >> 12) % 4u == 0u) rb[b] = u8((r >> 16) % 5u);
                } else if (b == 8u && out.rules[i].opcode == OP_DIFF) {
                    // Smooth tissue/neuron differentiation fate switch!
                    rb[b] = u8(TK_STRUCT + ((r >> 12) % (TK_TYPES - TK_STRUCT)));
                } else if (b >= 2u && b <= 5u || b == 10u || b == 11u) {
                    i32 delta = i32((r >> 10) % 19u) - 9;
                    rb[b] = u8(clamp_i32(i32(i8(rb[b])) + delta, -124, 124));
                } else {
                    rb[b] ^= u8(1u << ((r >> 10) & 7u));
                }
            }
        }
        if (out.rules[i].chem_min[0] > out.rules[i].chem_max[0]) {
            i8 tmp = out.rules[i].chem_min[0]; out.rules[i].chem_min[0] = out.rules[i].chem_max[0]; out.rules[i].chem_max[0] = tmp;
        }
        if (out.rules[i].chem_min[1] > out.rules[i].chem_max[1]) {
            i8 tmp = out.rules[i].chem_min[1]; out.rules[i].chem_min[1] = out.rules[i].chem_max[1]; out.rules[i].chem_max[1] = tmp;
        }
    }

    for (u32 i = 0; i < 4u; ++i) {
        r = mix(r + 997u);
        if ((r & 31u) < 4u) out.plast_lut[i] ^= (1u << ((r >> 8) & 31u));
    }
    for (u32 i = 0; i < 12u; ++i) {
        r = mix(r + 313u);
        if ((r & 63u) < 4u) {
            i32 delta = i32((r >> 8) % 15u) - 7;
            out.bio_params[i] = i8(clamp_i32(i32(out.bio_params[i]) + delta, -120, 120));
        }
    }

    // Ohno Gene Duplication + Divergence & Deletion of CA Rules (Unbounded Rulebook Evolution!)
    u32 groll = mix(r ^ 0x4cf5ad43u) & 255u;
    if (groll < 20u && na < MAX_DNA_RULES) {
        u32 src = (groll % na);
        CARule dup = out.rules[src];
        // Slight divergence on the duplicated rule so it can specialize into a new organ/nucleus/tract!
        dup.chem_min[0] = i8(clamp_i32(i32(dup.chem_min[0]) + i32((r & 15u)) - 7, -124, 120));
        dup.chem_max[0] = i8(clamp_i32(i32(dup.chem_max[0]) + i32(((r >> 4) & 15u)) - 7, -120, 124));
        if (dup.opcode == OP_DIFF && ((r >> 8) & 3u) == 0u)
            dup.arg_a = u8(TK_STRUCT + (((r >> 10)) % (TK_TYPES - TK_STRUCT)));
        else if (dup.opcode == OP_MITOSIS && ((r >> 8) & 3u) == 0u)
            dup.arg_b = u8((dup.arg_b + 1u) & 15u);
        dup.tag_bits ^= u16(1u << ((r >> 14) & 7u));
        out.rules[na++] = dup;
    } else if (groll >= 242u && na > 20u) {
        u32 del = (groll % na);
        out.rules[del] = out.rules[--na];
    }
    out.num_rules = u8(na);

    // Tissue Depth Layer Gene Duplication / Pruning (1..MAX_LAYERS_PER_CREATURE = up to 4,096 cells/neurons!)
    u32 lroll = mix(r ^ 0x9e3779b9u) & 255u;
    u32 ml = clamp_i32(i32(out.max_layers_gene), 1, i32(MAX_LAYERS_PER_CREATURE));
    if (lroll < 18u && ml < MAX_LAYERS_PER_CREATURE) ++ml;
    else if (lroll >= 244u && ml > 1u) --ml;
    out.max_layers_gene = u8(ml);

    if ((lroll & 63u) == 1u) out.birth_steps = u8(clamp_i32(i32(out.birth_steps) + ((lroll & 128u) ? 1 : -1), 8, 26));
    if ((lroll & 63u) == 2u) out.adult_steps = u8(clamp_i32(i32(out.adult_steps) + ((lroll & 128u) ? 1 : -1), 1, 6));
    return out;
}

// Helper: does cell `t` match rule condition `r` (state, tissue class, and Wolpert morphogen window)?
AX_FN bool rule_matches(const CARule &r, u64 t, u32 stage_flag) {
    u8 tk = tis_kind(t);
    if (tk == TK_EMPTY) return false;
    u32 stg = (r.stage_pri >> 4) & 3u;
    if (stg == 1u && stage_flag != 0u) return false; // embryo-only rule
    if (stg == 2u && stage_flag == 0u) return false; // juvenile maturation-only rule

    if (r.match_tk == 1u && tk != TK_STEM) return false;
    else if (r.match_tk == 2u && !tk_is_sensor(tk)) return false;
    else if (r.match_tk == 3u && !tk_is_neuron(tk)) return false;
    else if (r.match_tk == 4u && !(tk == TK_NEU_PV || tk == TK_NEU_SST || tk == TK_NEU_MOD)) return false;
    else if (r.match_tk >= 5u && tk != u8((r.match_tk - 5u) % TK_TYPES)) return false;

    if (r.match_state < 16u && tis_state(t) != r.match_state) return false;

    u32 c0 = r.chem_sel & 3u;
    u32 c1 = (r.chem_sel >> 4) & 3u;
    i32 v0 = tis_m(t, c0);
    i32 v1 = tis_m(t, c1);
    return (v0 >= i32(r.chem_min[0]) && v0 <= i32(r.chem_max[0]) &&
            v1 >= i32(r.chem_min[1]) && v1 <= i32(r.chem_max[1]));
}

// ---------------------------------------------------------------------------
// UNIFIED CELLULAR-AUTOMATA BODY + BRAIN MORPHOGENESIS & AXON GROWTH CONES!
// Runs at hatching (is_maturation=false, starting from 1 zygote cell!) AND
// incrementally during juvenile->adult growth (is_maturation=true, preserving
// existing grown cells + learned synaptic weights + BDNF activity feedback!).
// ---------------------------------------------------------------------------
AX_FN void develop_organism_block(const GenesisDNA &dna, BodyPhenotype &body, CreatureState &cr,
                                  TissuePool pool, bool is_maturation, DevSnapshots *snap_out, u32 dev_seed) {
    u32 tid = tid_x();
    u32 ux = tid & 15u;
    u32 uy = tid >> 4;

    __attribute__((shared)) CARule s_rules[MAX_DNA_RULES];
    __attribute__((shared)) u64 s_tis[TISSUE_CELLS];
    __attribute__((shared)) u64 s_nxt[TISSUE_CELLS];
    __attribute__((shared)) u32 s_in_cnt[TISSUE_CELLS];
    __attribute__((shared)) u32 s_in_syn[TISSUE_CELLS * SYN_PER_STRIPE]; // 16 KiB coalesced staging for growth cones!
    __attribute__((shared)) u32 s_bud_flag;
    __attribute__((shared)) u32 s_need_slab;
    __attribute__((shared)) u32 s_just_alloc;
    __attribute__((shared)) u32 s_new_slab;
    __attribute__((shared)) u32 s_warp_h[8];
    __attribute__((shared)) u8 s_has_src[MAX_LAYERS_PER_CREATURE];
    __attribute__((shared)) u8 s_has_tgt[MAX_LAYERS_PER_CREATURE];
    __attribute__((shared)) u32 s_cr_slabs[MAX_LAYERS_PER_CREATURE];
    __attribute__((shared)) u32 s_t_lss[MAX_STRIPES_PER_LAYER];
    __attribute__((shared)) u32 s_tz_room;
    __attribute__((shared)) u32 s_tk_cnt[TK_TYPES];
    __attribute__((shared)) u32 s_stat_u[8];   // tot_cells, act_neu, tot_syn, mx_fan, sym_m, sym_p, surf_cells, cones_tot
    __attribute__((shared)) float s_stat_f[9]; // mass, cx, cy, inertia, raw_thr, raw_trq, raw_gr, raw_gl, raw_ar
    __attribute__((shared)) u8 s_noncone_idx[MAX_DNA_RULES];
    __attribute__((shared)) u8 s_mit_idx[MAX_DNA_RULES];
    __attribute__((shared)) u8 s_cone_idx[MAX_DNA_RULES];
    __attribute__((shared)) u32 s_op_cnt[3]; // 0=noncone, 1=mitosis, 2=growth_cone

    u32 n_rules = min_u32(u32(dna.num_rules), MAX_DNA_RULES);
    if (tid < n_rules) s_rules[tid] = dna.rules[tid];
    bool bilateral = (dna.bio_params[7] >= 0);
    u32 max_layers = clamp_i32(i32(dna.max_layers_gene), 1, i32(MAX_LAYERS_PER_CREATURE));
    sync_block();
    if (tid == 0u) {
        u32 n_nc = 0u, n_m = 0u, n_c = 0u;
        for (u32 ri = 0; ri < n_rules; ++ri) {
            u8 op = s_rules[ri].opcode;
            if (op == OP_GROWTH_CONE) s_cone_idx[n_c++] = u8(ri);
            else {
                s_noncone_idx[n_nc++] = u8(ri);
                if (op == OP_MITOSIS) s_mit_idx[n_m++] = u8(ri);
            }
        }
        s_op_cnt[0] = n_nc; s_op_cnt[1] = n_m; s_op_cnt[2] = n_c;
    }
    sync_block();
    u32 n_noncone = s_op_cnt[0], n_mit = s_op_cnt[1], n_cone = s_op_cnt[2];

    // 1. Initialize Single Zygote Seed (8, 8, layer 0) at Hatching, OR Inject Neural Activity BDNF on Maturation!
    if (!is_maturation) {
        u32 sl0 = cr.layer_slabs[0];
        if (sl0 == INVALID_SLAB) return;
        // Release any extra layers from a previous resident if reusing state
        if (tid == 0u) {
            for (u32 l = 1u; l < MAX_LAYERS_PER_CREATURE; ++l) cr.layer_slabs[l] = INVALID_SLAB;
            cr.num_layers = 1u;
            for (u32 st = 0; st < MAX_STRIPES_PER_LAYER; ++st)
                pool.layer_syn_slabs[u64(sl0) * MAX_STRIPES_PER_LAYER + st] = INVALID_SLAB;
        }
        bool is_zygote = (ux == 8u && uy == 8u);
        u64 zcell = is_zygote ? pack_tissue(TK_STEM, 0u, 14u, 0x0001u, 120, 0, 0, 32) : 0ull;
        pool.tissue[u64(sl0) * TISSUE_CELLS + tid] = zcell;
        pool.neurons[u64(sl0) * TISSUE_CELLS + tid] = 0ull;
        pool.spike_ring[u64(sl0) * TISSUE_CELLS + tid] = 0u;
        pool.spike_ring_next[u64(sl0) * TISSUE_CELLS + tid] = 0u;
        pool.cell_stripes[u64(sl0) * TISSUE_CELLS + tid] = 0u;
        if (snap_out) snap_out->step0[tid] = is_zygote ? TK_STEM : TK_EMPTY;
    } else {
        // Post-natal juvenile->adult maturation: feed each cell's real recent spiking activity (ema)
        // directly into morphogen channel 3 (BDNF/NGF neurotrophin!) so experience sculpts live growth!
        u32 cur_L = min_u32(u32(cr.num_layers), MAX_LAYERS_PER_CREATURE);
        for (u32 z = 0; z < cur_L; ++z) {
            u32 sl = cr.layer_slabs[z];
            if (sl == INVALID_SLAB) continue;
            u64 t = pool.tissue[u64(sl) * TISSUE_CELLS + tid];
            if (tis_kind(t) != TK_EMPTY) {
                u32 ema = neu_ema(pool.neurons[u64(sl) * TISSUE_CELLS + tid]);
                i32 m3 = clamp_i32(tis_m(t, 3u) + (i32(ema) - 18), -124, 124);
                u8 bud = tis_budget(t);
                if (tis_kind(t) == TK_STEM && (bud & 0x0fu) < 6u) bud = u8((bud & 0xf0u) | ((bud & 0x0fu) + 4u));
                pool.tissue[u64(sl) * TISSUE_CELLS + tid] =
                    pack_tissue(tis_kind(t), tis_state(t), bud, tis_tag(t),
                                i8(tis_m(t, 0u)), i8(tis_m(t, 1u)), i8(tis_m(t, 2u)), i8(m3));
            }
        }
    }
    sync_block();

    // 2. RUN DISCRETE CA-GRN CELLULAR GROWTH (Asymmetric Mitosis, oSVZ Cortical Sheet Expansion, Morphogens, Diff!)
    u32 n_steps = is_maturation ? clamp_i32(i32(dna.adult_steps), 1, 6) : clamp_i32(i32(dna.birth_steps), 8, 26);
    u32 diff_start = is_maturation ? 1u : ((n_steps >= 6u) ? (n_steps - 4u) : 2u);
    u32 stage_flag = is_maturation ? 1u : 0u;

    for (u32 step = 0; step < n_steps; ++step) {
        u32 cur_L = min_u32(u32(cr.num_layers), MAX_LAYERS_PER_CREATURE);
        for (u32 z = 0; z < cur_L; ++z) {
            u32 sl = cr.layer_slabs[z];
            if (sl == INVALID_SLAB) continue;
            u64 my_t = pool.tissue[u64(sl) * TISSUE_CELLS + tid];
            s_tis[tid] = my_t;
            s_nxt[tid] = my_t;
            if (tid == 0u) s_bud_flag = 0u;
            sync_block();

            // 2a. Local Morphogen Reaction-Diffusion + OP_SIGNAL + OP_DIFF + OP_APOPTOSIS
            u8 my_tk = tis_kind(my_t);
            u8 my_st = tis_state(my_t);
            u8 my_bud = tis_budget(my_t);
            u16 my_tag = tis_tag(my_t);
            i32 m[4] = {tis_m(my_t, 0u), tis_m(my_t, 1u), tis_m(my_t, 2u), tis_m(my_t, 3u)};

            // 4-neighbor horizontal diffusion across the 16x16 tissue lattice
            i32 n_id[4] = {
                (ux + 1u < TISSUE_W) ? i32(tid + 1u) : -1,
                (ux > 0u) ? i32(tid - 1u) : -1,
                (uy + 1u < TISSUE_H) ? i32(tid + TISSUE_W) : -1,
                (uy > 0u) ? i32(tid - TISSUE_W) : -1
            };
            i32 nbr_m[4]{0, 0, 0, 0};
            u32 nbr_alive = 0u;
            u32 nbr_exc = 0u;
            for (u32 d = 0; d < 4u; ++d) {
                if (n_id[d] >= 0) {
                    u64 nt = s_tis[n_id[d]];
                    u8 ntk = tis_kind(nt);
                    if (ntk != TK_EMPTY) {
                        ++nbr_alive;
                        if (ntk == TK_NEU_EXC) ++nbr_exc;
                        for (u32 c = 0; c < 4u; ++c) nbr_m[c] += tis_m(nt, c);
                    }
                }
            }
            if (my_tk != TK_EMPTY || nbr_alive > 0u) {
                i32 ap_grad = (8 - i32(uy)) * 7; // Maternal egg anterior(+)/posterior(-) polarity cue
                i32 ml_grad = (bilateral ? (i32(ux) < 8 ? (8 - i32(ux)) : (i32(ux) - 8)) : (i32(ux) - 8)) * 7;
                for (u32 c = 0; c < 4u; ++c) {
                    i32 lap = nbr_m[c] - 4 * m[c];
                    m[c] = clamp_i32(m[c] + (lap >> 3) - (m[c] >> 5), -124, 124);
                }
                if (my_tk != TK_EMPTY) {
                    m[1] = clamp_i32(m[1] + (ap_grad >> 1), -124, 124);
                    m[2] = clamp_i32(m[2] + (ml_grad >> 1), -124, 124);
                }
            }

            // Evaluate matching discrete CA rules for THIS living cell!
            bool wants_bud_z = false;
            u8 bud_child_st = my_st;
            u16 bud_child_tag = my_tag;
            i32 bud_m1_shift = 0;
            // Separate Vertebrate Gastrulation (z == 0 somatic body plan finishes cleanly at steps 0..8)
            // from Neocortical oSVZ Proliferation (z >= 1 expands deep sheets through steps 0..n_steps-1)!
            bool can_divide = (z == 0u && !is_maturation) ? (step < 8u) : (step + 1u < n_steps);
            u32 layer_diff_start = (z == 0u && !is_maturation)
                                       ? min_u32(diff_start, 7u)
                                       : (is_maturation ? diff_start
                                                        : ((z == 1u) ? (n_steps >= 3u ? n_steps - 2u : diff_start)
                                                                     : (n_steps >= 2u ? n_steps - 1u : diff_start)));
            bool stagger_wait = (z == 1u && !is_maturation && ((ux + uy + z) & 1u) != 0u && step == layer_diff_start);
            bool can_diff = !stagger_wait && ((step >= layer_diff_start) || ((my_bud & 0x0fu) <= 1u));

            if (my_tk != TK_EMPTY) {
                i32 best_diff_pri = -1;
                u8 new_tk = my_tk;
                u8 new_tau = my_bud;
                u16 new_tag = my_tag;
                bool do_apop = false;

                for (u32 k_ri = 0; k_ri < n_noncone; ++k_ri) {
                    const CARule &rl = s_rules[s_noncone_idx[k_ri]];
                    if (!rule_matches(rl, my_t, stage_flag)) continue;

                    if (rl.opcode == OP_SIGNAL) {
                        u32 ch = rl.arg_a & 3u;
                        m[ch] = clamp_i32(m[ch] + (i32(rl.param0) >> 1), -124, 124);
                        new_tag |= rl.tag_bits;
                    } else if (rl.opcode == OP_DIFF && my_tk == TK_STEM && can_diff) {
                        i32 pri = i32(rl.stage_pri & 15u);
                        if (pri >= best_diff_pri) {
                            u8 cand_tk = u8(clamp_i32(i32(rl.arg_a), TK_STRUCT, TK_TYPES - 1));
                            // Fundamental bilaterian embryology (Ectoderm surface vs Dorsal Pallium Neural Tube):
                            if (z > 0u && !tk_is_neuron(cand_tk)) {
                                u32 r8 = u32(cand_tk + rl.arg_b) & 7u;
                                cand_tk = (r8 < 5u) ? TK_NEU_EXC : ((r8 < 7u) ? TK_NEU_PV : TK_NEU_SST);
                            }
                            // Authentic ventral ganglionic-eminence & subcortical VTA/BF nuclei constraints in deep cortex:
                            // Direct GABAergic PV/SST fate requires sparse mosaic sites or excitatory surround (prevents 80%-PV dead sheets!),
                            // and VTA/BF neuromodulatory MOD nuclei stay sparse (~2-6% on z<=2, preventing 65%-MOD dopamine flooding!)
                            if (z > 0u && tk_is_inhibitory(cand_tk) && (((ux + uy + z) & 3u) != 0u) && nbr_exc < 2u) {
                                cand_tk = TK_NEU_EXC;
                            } else if (cand_tk == TK_NEU_MOD && (z > 2u || (((ux + uy + z) & 7u) != 0u))) {
                                cand_tk = TK_NEU_EXC;
                            }
                            // Sparse subcortical VTA/BF neuromodulatory MOD nuclei on medial z<=2 (avoid double-stacking PV/SST
                            // with Step 3 MGE/CGE tangential migration so deep cortex hits canonical mammalian ~78% E / 20% I!)
                            if (z > 0u && cand_tk == TK_NEU_EXC && nbr_exc >= 2u) {
                                u32 nd_h = (ux * 3u + uy * 5u + z * 7u) & 7u;
                                if (nd_h == 0u && z <= 2u && ux >= 6u && ux <= 10u) cand_tk = TK_NEU_MOD;
                            }
                            best_diff_pri = pri;
                            new_tk = cand_tk;
                            new_tau = u8((my_bud & 0x0fu) | ((rl.arg_b & 0x0fu) << 4));
                            new_tag |= rl.tag_bits;
                        }
                    } else if (rl.opcode == OP_APOPTOSIS && step >= diff_start + 1u && !(ux == 8u && uy == 8u && z == 0u) && nbr_alive <= 2u) {
                        do_apop = true;
                    } else if (rl.opcode == OP_MITOSIS && can_divide && (my_bud & 0x0fu) > 0u) {
                        // Vertical asymmetric mitosis (+z): oSVZ radial-glia budding of deeper neocortical layers!
                        // L0 waits until step>=2 so head/brow/trunk pattern first; deep L1+ bud continuously from step>=1!
                        if ((rl.dir_mask & 0x10u) && z + 1u < max_layers && step >= (z == 0u ? 2u : 1u)) {
                            wants_bud_z = true;
                            bud_child_st = rl.arg_b & 15u;
                            bud_child_tag = rl.tag_bits | 0x0020u;
                            bud_m1_shift = i32(rl.param1) >> 1;
                        }
                    }
                }

                if (do_apop) {
                    s_nxt[tid] = 0ull;
                } else {
                    s_nxt[tid] = pack_tissue(new_tk, my_st, new_tau, new_tag, i8(m[0]), i8(m[1]), i8(m[2]), i8(m[3]));
                }
            } else if (can_divide && nbr_alive > 0u) {
                // 2b. Pull-based Asymmetric Horizontal Mitosis into EMPTY site (scans ONLY OP_MITOSIS rules — 5x faster!)
                bool spawned = false;
                for (u32 d = 0; d < 4u; ++d) {
                    if (n_id[d] < 0) continue;
                    u64 pt = s_tis[n_id[d]];
                    if (tis_kind(pt) == TK_EMPTY || (tis_budget(pt) & 0x0fu) == 0u) continue;
                    u32 opp_bit = (d == 0u) ? 2u : (d == 1u ? 1u : (d == 2u ? 8u : 4u));
                    for (u32 k_m = 0; k_m < n_mit; ++k_m) {
                        const CARule &rl = s_rules[s_mit_idx[k_m]];
                        if ((rl.dir_mask & opp_bit) == 0u) continue;
                        if (!rule_matches(rl, pt, stage_flag)) continue;
                        u8 child_st = rl.arg_b & 15u;
                        // Authentic mammalian oSVZ outer radial glia symmetric proliferation in deep cortex (z >= 1):
                        // Lets cortical sheets organically grow to 100-220+ neurons/layer (1,000-4,000+ neurons/organism!)
                        // while L0 somatic body respects compact hydrodynamic organ budgets!
                        i32 b_dec = (z >= 1u && !is_maturation && step + 3u < n_steps) ? 0 : 1;
                        u8 child_bud = u8(clamp_i32(i32(tis_budget(pt) & 0x0fu) - b_dec, 0, 14));
                        if (rl.param0 > 0 && z == 0u) child_bud = u8(min_u32(child_bud, u32(rl.param0 & 15)));
                        i8 cm0 = i8(clamp_i32(tis_m(pt, 0u) - 6, -124, 124));
                        i8 cm1 = i8(clamp_i32(tis_m(pt, 1u) + (i32(rl.param1) >> 1), -124, 124));
                        i8 cm2 = i8(clamp_i32(tis_m(pt, 2u) + ((d <= 1u) ? 10 : 0), -124, 124));
                        i8 cm3 = i8(clamp_i32(tis_m(pt, 3u) + ((child_st == 4u || child_st == 6u) ? 12 : -4), -124, 124));
                        s_nxt[tid] = pack_tissue(TK_STEM, child_st, child_bud, tis_tag(pt) | rl.tag_bits, cm0, cm1, cm2, cm3);
                        spawned = true;
                        break;
                    }
                    if (spawned) break;
                }
                // Authentic Mammalian oSVZ Outer Radial Glia (oRG) Tangential Expansion (Kriegstein / Rakic):
                // Deep neocortical sheets (z >= 1) expand tangentially across the cranial vault independently of
                // compact z==0 somatic body plans, governed by an evolvable cranial proliferation gene (org_rad in 3..6)!
                if (!spawned && z >= 1u && !is_maturation && ux >= 1u && ux <= 14u && uy >= 1u && uy <= 14u) {
                    u32 org_rad = 4u + ((u32(i32(dna.bio_params[11]) + 128) >> 4) & 3u); // Evolvable 4..7 -> 95..196 neurons/layer!
                    u32 min_pbud = 14u - org_rad;
                    for (u32 d = 0; d < 4u; ++d) {
                        if (n_id[d] < 0) continue;
                        u64 pt = s_tis[n_id[d]];
                        u32 pbud = tis_budget(pt) & 0x0fu;
                        if (tis_kind(pt) != TK_STEM || pbud <= min_pbud) continue;
                        u8 cbud = u8(pbud - 1u);
                        s_nxt[tid] = pack_tissue(TK_STEM, tis_state(pt), cbud, tis_tag(pt) | 0x0020u,
                                                 i8(clamp_i32(tis_m(pt, 0u) - 6, -124, 124)),
                                                 i8(tis_m(pt, 1u)),
                                                 i8(clamp_i32(tis_m(pt, 2u) + ((d <= 1u) ? 10 : 0), -124, 124)),
                                                 i8(tis_m(pt, 3u)));
                        spawned = true;
                        break;
                    }
                }
            }
            sync_block();

            // Update parent state/budget ONLY when it actually had an empty horizontal division target
            // (deep oSVZ radial glia on z>=1 preserve mitotic budget during symmetric proliferation!)
            u64 res = s_nxt[tid];
            bool had_empty_nbr = (nbr_alive < 4u);
            if (my_tk != TK_EMPTY && tis_kind(res) != TK_EMPTY && (my_bud & 0x0fu) > 0u && had_empty_nbr) {
                for (u32 k_m = 0; k_m < n_mit; ++k_m) {
                    const CARule &rl = s_rules[s_mit_idx[k_m]];
                    if (rule_matches(rl, my_t, stage_flag)) {
                        u8 da_st = rl.arg_a & 15u;
                        u32 p_dec = (z >= 1u && !is_maturation && step + 3u < n_steps) ? 0u : 1u;
                        u8 nbud = u8(((tis_budget(res) & 0xf0u)) | (u32(my_bud & 0x0fu) - p_dec));
                        res = pack_tissue(tis_kind(res), da_st, nbud, tis_tag(res),
                                          i8(tis_m(res, 0u)), i8(tis_m(res, 1u)), i8(tis_m(res, 2u)), i8(tis_m(res, 3u)));
                        break;
                    }
                }
            }
            s_nxt[tid] = res;
            sync_block();

            if (bilateral && ux > 8u) {
                u32 mir = uy * TISSUE_W + (16u - ux); // Mirror across x = 8 sagittal midline (1..7 <-> 15..9)
                if (mir < TISSUE_CELLS) s_nxt[tid] = s_nxt[mir];
            }
            sync_block();

            // Authentic bilaterian Left/Right Nodal-Pitx2 midline identity (tag bit 8 = Left, bit 9 = Right)!
            // Allows crawling axons to evolve ipsilateral vs contralateral (decussated) stereo steering circuits!
            {
                u64 ct_lr = s_nxt[tid];
                if (tis_kind(ct_lr) != TK_EMPTY) {
                    u16 lr_tag = (tis_tag(ct_lr) & ~u16(0x0300u)) | (ux < 8u ? u16(0x0100u) : (ux > 8u ? u16(0x0200u) : u16(0u)));
                    ct_lr = pack_tissue(tis_kind(ct_lr), tis_state(ct_lr), tis_budget(ct_lr), lr_tag,
                                        i8(tis_m(ct_lr, 0u)), i8(tis_m(ct_lr, 1u)), i8(tis_m(ct_lr, 2u)), i8(tis_m(ct_lr, 3u)));
                    s_nxt[tid] = ct_lr;
                }
            }
            pool.tissue[u64(sl) * TISSUE_CELLS + tid] = s_nxt[tid];

            // 2c. Vertical Layer Budding (+z): Lock-free GPU slab allocation for newly budded deep brain layers!
            if (wants_bud_z && cr.layer_slabs[z + 1u] == INVALID_SLAB) {
                atomic_add_gen_u32(&s_bud_flag, 1u);
            }
            sync_block();

            if (s_bud_flag > 0u && z + 1u < max_layers) {
                if (tid == 0u) {
                    u32 nsl = cr.layer_slabs[z + 1u];
                    s_just_alloc = 0u;
                    if (nsl == INVALID_SLAB) {
                        nsl = alloc_slab(pool.layer_bitmap, pool.max_layers, dev_seed + z * 37u);
                        if (nsl != INVALID_SLAB) {
                            cr.layer_slabs[z + 1u] = nsl;
                            if (z + 2u > cr.num_layers) cr.num_layers = u16(z + 2u);
                            for (u32 st = 0; st < MAX_STRIPES_PER_LAYER; ++st)
                                pool.layer_syn_slabs[u64(nsl) * MAX_STRIPES_PER_LAYER + st] = INVALID_SLAB;
                            s_just_alloc = 1u;
                        }
                    }
                    s_new_slab = nsl;
                }
                sync_block();
                u32 nsl = s_new_slab;
                if (nsl != INVALID_SLAB) {
                    // CRITICAL: Always sanitize recycled deep-layer slabs on first allocation (ZERO zombie tissue!)
                    if (s_just_alloc) {
                        pool.tissue[u64(nsl) * TISSUE_CELLS + tid] = 0ull;
                        pool.neurons[u64(nsl) * TISSUE_CELLS + tid] = 0ull;
                        pool.spike_ring[u64(nsl) * TISSUE_CELLS + tid] = 0u;
                        pool.spike_ring_next[u64(nsl) * TISSUE_CELLS + tid] = 0u;
                        pool.cell_stripes[u64(nsl) * TISSUE_CELLS + tid] = 0u;
                    }
                    // oSVZ Outer Radial Glia (oRG) transit-amplifying neurogenesis: refreshed cranial proliferation budget (14u)
                    // allows deep cortical laminae L1..L31 to expand into dense 120-240 neuron sheets (1,000-4,000+ neurons!)
                    u64 existing = pool.tissue[u64(nsl) * TISSUE_CELLS + tid];
                    if (tis_kind(existing) == TK_EMPTY && wants_bud_z) {
                        u64 child_z = pack_tissue(TK_STEM, bud_child_st, 14u, bud_child_tag,
                                                  i8(clamp_i32(m[0], -124, 124)),
                                                  i8(clamp_i32(m[1] + bud_m1_shift, -124, 124)),
                                                  i8(clamp_i32(m[2], -124, 124)),
                                                  i8(clamp_i32(m[3] + 18, -124, 124)));
                        pool.tissue[u64(nsl) * TISSUE_CELLS + tid] = child_z;
                        pool.neurons[u64(nsl) * TISSUE_CELLS + tid] = 0ull;
                        pool.spike_ring[u64(nsl) * TISSUE_CELLS + tid] = 0u;
                        pool.spike_ring_next[u64(nsl) * TISSUE_CELLS + tid] = 0u;
                        pool.cell_stripes[u64(nsl) * TISSUE_CELLS + tid] = 0u;
                    }
                }
            }
            sync_block();

            // Capture Layer-0 unified body+brain embryogenesis trace snapshots!
            if (snap_out && z == 0u) {
                u8 st_tk = tis_kind(s_nxt[tid]);
                if (step == 2u) snap_out->step3[tid] = st_tk;
                else if (step == 5u) snap_out->step6[tid] = st_tk;
                else if (step + 1u == n_steps) snap_out->step9[tid] = st_tk;
            }
        }
    }

    // 3. Finalize Stem Cells, Prune Empty Deep Slabs (Blackwell Zero-Waste Worklist!) & Init Spiking Neurons
    u32 final_L = min_u32(u32(cr.num_layers), MAX_LAYERS_PER_CREATURE);
    u32 live_L = 1u;
    for (u32 z = 0; z < final_L; ++z) {
        u32 sl = cr.layer_slabs[z];
        if (sl == INVALID_SLAB) continue;
        u64 t = pool.tissue[u64(sl) * TISSUE_CELLS + tid];
        u8 tk = tis_kind(t);
        // Conserved Vertebrate MGE/CGE Ventral Tangential Interneuron Migration + Postnatal Apoptosis Homeostasis
        // (Anderson 1997 Science / Southwell 2012 Nature) + Medial Subcortical VTA/BF Neuromodulatory Nuclei:
        // Evolvable via dna.bio_params[9] within the canonical mammalian E/I band (mod_n in 8..11 -> 18.2%..25.0% GABAergic,
        // split 1:1 between somatic PV+ basket cells and apical SST+ Martinotti cells) -> prevents both 0%-I epilepsy AND
        // 43%-I PV-tumor lobotomy across 50,000+ ticks while sustaining 1,000-4,000+ neuron deep cortices!
        if (z > 0u && (tk == TK_STEM || tk == TK_NEU_EXC || tk == TK_NEU_PV || tk == TK_NEU_SST || tk == TK_NEU_MOD)) {
            u32 ei_mod = 8u + (u32(i32(dna.bio_params[9]) + 128) & 3u); // Evolvable 8..11 -> 25.0%..18.2% I!
            u32 mig_h = (ux * 3u + uy * 5u + z * 7u) % ei_mod;
            bool vta_site = (z <= 2u && ux >= 6u && ux <= 10u && uy >= 6u && uy <= 10u && ((ux + uy + z) & 7u) == 0u);
            if (vta_site) {
                tk = TK_NEU_MOD;
            } else if (mig_h == 0u) {
                tk = TK_NEU_PV;
            } else if (mig_h == (ei_mod >> 1)) {
                tk = TK_NEU_SST;
            } else {
                tk = TK_NEU_EXC;
            }
            t = pack_tissue(tk, tis_state(t), tis_budget(t), tis_tag(t) | 0x0020u,
                            i8(tis_m(t, 0u)), i8(tis_m(t, 1u)), i8(tis_m(t, 2u)), i8(tis_m(t, 3u)));
            pool.tissue[u64(sl) * TISSUE_CELLS + tid] = t;
        }
        if (z > 0u) {
            if (tid == 0u) s_bud_flag = 0u;
            sync_block();
            if (warp_ballot(tk_is_excitable(tk)) && lane_id() == 0u) atomic_add_gen_u32(&s_bud_flag, 1u);
            sync_block();
            if (s_bud_flag == 0u) {
                if (tid == 0u) {
                    for (u32 st = 0; st < MAX_STRIPES_PER_LAYER; ++st) {
                        u32 ss = pool.layer_syn_slabs[u64(sl) * MAX_STRIPES_PER_LAYER + st];
                        if (ss != INVALID_SLAB) {
                            free_slab(pool.syn_bitmap, ss);
                            pool.layer_syn_slabs[u64(sl) * MAX_STRIPES_PER_LAYER + st] = INVALID_SLAB;
                        }
                    }
                    free_slab(pool.layer_bitmap, sl);
                    cr.layer_slabs[z] = INVALID_SLAB;
                }
                sync_block();
                continue;
            }
            live_L = z + 1u;
        }
        if (z == 0u) {
            body.surface[tid] = tk;
            if (snap_out) snap_out->step9[tid] = tk;
        }
        u64 old_n = pool.neurons[u64(sl) * TISSUE_CELLS + tid];
        u32 slow_tau = ((tis_budget(t) >> 4) & 1u) | (z >= 2u ? 1u : 0u);
        u32 fast_plast = ((tis_budget(t) >> 5) & 1u) | (z == 1u ? 1u : 0u);
        if (!is_maturation || neu_tk(old_n) != tk) {
            pool.neurons[u64(sl) * TISSUE_CELLS + tid] =
                tk_is_excitable(tk) ? pack_neuron(0, 0, 0u, 0u, 16u, tk, 0u, 0u, slow_tau, fast_plast) : 0ull;
            pool.spike_ring[u64(sl) * TISSUE_CELLS + tid] = 0u;
            pool.spike_ring_next[u64(sl) * TISSUE_CELLS + tid] = 0u;
        }
    }
    if (tid == 0u) cr.num_layers = u16(live_L);
    final_L = live_L;
    sync_block();

    // 4. DISCRETE CRAWLING AXON GROWTH CONES -> VARIABLE FAN-IN COALESCED BLACKWELL v4.u32 SLABS!
    // Every sensor (EYE, EAR, CHEM) and neuron (EXC, PV, SST, MOD) launches growth cones steered by
    // evolved morphogen gradients & boundary tags through the unified (x, y, z) cellular tissue!
    if (tid < MAX_LAYERS_PER_CREATURE) {
        s_has_src[tid] = 0u;
        s_has_tgt[tid] = 0u;
        s_cr_slabs[tid] = cr.layer_slabs[tid];
    }
    sync_block();
    for (u32 z = 0; z < final_L; ++z) {
        u32 sl = s_cr_slabs[z];
        if (sl == INVALID_SLAB) continue;
        u8 zk = tis_kind(ld_nc_u64(pool.tissue + u64(sl) * TISSUE_CELLS + tid));
        u32 b_src = warp_ballot(tk_is_sensor(zk) || tk_is_neuron(zk));
        u32 b_tgt = warp_ballot(tk_is_neuron(zk) || tk_is_effector(zk));
        if (lane_id() == 0u) {
            if (b_src) s_has_src[z] = 1u;
            if (b_tgt) s_has_tgt[z] = 1u;
        }
    }
    sync_block();

    __attribute__((shared)) u8 s_gland_pos[16];
    __attribute__((shared)) u8 s_mus_pos[16];
    __attribute__((shared)) u32 s_n_glands;
    __attribute__((shared)) u32 s_n_mus;
    u32 total_cones_landed = 0u;
    for (u32 tz = 0; tz < final_L; ++tz) {
        u32 t_sl = s_cr_slabs[tz];
        if (t_sl == INVALID_SLAB || !s_has_tgt[tz]) continue;
        // Reuse s_nxt[256] (dead after Step 2!) as L1-speed shared cache of target layer `tz` (ZERO extra shmem!)
        u64 t_my_word = ld_nc_u64(pool.tissue + u64(t_sl) * TISSUE_CELLS + tid);
        s_nxt[tid] = t_my_word;
        if (tid < MAX_STRIPES_PER_LAYER) s_t_lss[tid] = pool.layer_syn_slabs[u64(t_sl) * MAX_STRIPES_PER_LAYER + tid];
        u8 t_my_tk = tis_kind(t_my_word);
        bool t_can_recv = (tk_is_neuron(t_my_tk) || tk_is_effector(t_my_tk));
        u8 raw_cs = pool.cell_stripes[u64(t_sl) * TISSUE_CELLS + tid];
        u8 my_st_idx = raw_cs & 0x0fu;
        u32 my_exc_syn_tot = u32(raw_cs >> 4) << 3;
        u32 n_gl = 0u, n_mu = 0u;
        if (tz == 0u) {
            if (tid == 0u) { s_n_glands = 0u; s_n_mus = 0u; }
            sync_block();
            if (t_my_tk == TK_GLAND) {
                u32 gi = atomic_add_gen_u32(&s_n_glands, 1u);
                if (gi < 16u) s_gland_pos[gi] = u8(tid);
            } else if (t_my_tk == TK_MUSCLE) {
                u32 mi = atomic_add_gen_u32(&s_n_mus, 1u);
                if (mi < 16u) s_mus_pos[mi] = u8(tid);
            }
            sync_block();
            n_gl = min_u32(s_n_glands, 16u);
            n_mu = min_u32(s_n_mus, 16u);
        }

        u32 sz_ord_lim = (tz == 0u && !is_maturation && (n_gl > 0u || n_mu > 0u)) ? (final_L + 3u) : final_L;
        for (u32 sz_ord = 0; sz_ord < sz_ord_lim; ++sz_ord) {
            // Canonical Mammalian Laminar Hierarchy (Felleman & Van Essen 1991 / Sherman & Guillery 1998)
            // + Dense Corticobulbar (HVC/Broca -> TK_GLAND) & Corticospinal (PFC/M1 -> TK_MUSCLE) Multi-Tract Readout:
            // - L1 Primary Sensory Cortex visits L0 Thalamocortical Sensors FIRST (dedicated Stripe-0 sensory driver!)
            // - All other layers visit local recurrent (sz == tz) FIRST so Stripe 0 stays in registers!
            // - On tz==0, 3 extra dedicated prefrontal passes from L2, L3, L4 double WM/context fan-in onto BOTH
            //   TK_GLAND vocal folds AND TK_MUSCLE skeletal actuators (30 distinct pyramidal synapses/hub layer, 0 B extra VRAM!)
            // - Deep apex projection strictly from final_L-1 & final_L-2 -> <=10 stripes on L0 even at 32L (zero 1/sqrt(K) dilution!)
            bool is_hvc_extra = (tz == 0u && sz_ord >= final_L);
            u32 sz;
            if (is_hvc_extra) {
                // Dedicated HVC/Broca & Premotor multi-tract passes from L2, L3, L3 -> GLAND + MUSCLE!
                // Concentrates corticobulbar/corticospinal readout on L3 (where L4 apical context x L2 cue x Uva vocal history
                // form nonlinear Rigotti/Larkum XOR assemblies!) instead of a linear L4 context bypass!
                sz = (sz_ord == final_L) ? 2u : 3u;
                if (sz >= final_L) continue;
            } else if (tz == 1u) {
                sz = (sz_ord == 0u) ? 0u : sz_ord;
            } else {
                sz = (sz_ord == 0u) ? tz : ((sz_ord <= tz) ? (sz_ord - 1u) : sz_ord);
            }
            if (tz == 0u && !is_hvc_extra) {
                bool deep_pt = (sz == final_L - 1u) || (final_L >= 3u && sz == final_L - 2u);
                if (sz > 5u && !deep_pt) continue;
            } else if (sz == 0u) {
                if (tz > 1u) continue; // Primary sensory axons terminate in L1 primary sensory cortex (V1/A1)!
            } else if (!is_hvc_extra) {
                u32 dz = (sz > tz) ? (sz - tz) : (tz - sz);
                if (dz > 2u) continue; // Authentic laminar cortical hierarchy (no K_N all-to-all soup!)
            }
            u32 s_sl = s_cr_slabs[sz];
            if (s_sl == INVALID_SLAB || !s_has_src[sz]) continue;

            // Early-exit `sz` loop during incremental maturation once every target cell in `tz` fills 8 stripes!
            // (During embryogenesis, each `sz` adds <=1 stripe and <=6 source layers pass -> 0 redundant barriers!)
            if (is_maturation) {
                if (tid == 0u) s_tz_room = 0u;
                sync_block();
                if (warp_ballot(t_can_recv && my_st_idx < MAX_STRIPES_PER_LAYER) && lane_id() == 0u)
                    atomic_add_gen_u32(&s_tz_room, 1u);
                sync_block();
                if (s_tz_room == 0u) break;
            }

            // Reset shared-memory counter (zero s_in_syn on tz==0 so pan-cortical BDNF reservoir slots stay clean!)
            // and reuse s_tis[256] as shared cache of source layer `sz` (ZERO extra shmem!)
            s_in_cnt[tid] = 0u;
            if (tz == 0u) {
                #pragma unroll
                for (u32 k = 0; k < SYN_PER_STRIPE; ++k) s_in_syn[tid * SYN_PER_STRIPE + k] = 0u;
            }
            u64 s_t = (sz == tz) ? t_my_word : ld_nc_u64(pool.tissue + u64(s_sl) * TISSUE_CELLS + tid);
            s_tis[tid] = s_t;
            u8 s_tk = tis_kind(s_t);
            sync_block();

            // Every living sensor or neuron in `sz` crawls its growth cones across the unified 3D tissue!
            if (tk_is_sensor(s_tk) || tk_is_neuron(s_tk)) {
                bool is_inh = tk_is_inhibitory(s_tk);
                u32 rng = mix(dev_seed ^ (sz * 0x85ebca6bu) ^ (tz * 0x9e3779b9u) ^ (sz_ord * 0x517cc1b7u) ^ (tid * 31337u));
                u32 cones_run = 0u;
                bool ran_sens_cone = false;
                bool ran_pyr_cort_cone = false;

                // Authentic Mammalian Cortical Interneuron Locality + Sparse Basal-Ganglia Striatal D2-NoGo Projection:
                // Deep GABAergic PV+/SST+ interneurons arborize strictly locally (sz==tz), EXCEPT sparse (~25%) Striatal
                // D2-NoGo collaterals (sz in 1..3 -> tz==0, (tid&3)==0) targeting dedicated D2 vocal slots (14..15) so
                // 30..8,000-neuron cortices keep ~88% pan-cortical pyramidal features + ~12% D2 veto per vocal fold!
                if (is_hvc_extra && (is_inh || s_tk != TK_NEU_EXC)) cones_run = 4u;
                else if (is_inh && sz >= 1u && sz != tz && !(tz == 0u && sz <= 3u && ((tid & 3u) == 0u))) cones_run = 4u;

                for (u32 k_c = 0; k_c < n_cone && cones_run < (is_hvc_extra ? 1u : 4u); ++k_c) {
                    const CARule &rl = s_rules[s_cone_idx[k_c]];
                    // Skip strictly horizontal growth cones when targeting a different cortical layer,
                    // AND skip effector-only rules (arg_a==2) on deep layers tz>0 where zero effectors exist (100% bit-exact)!
                    if (sz != tz && (rl.dir_mask & 0x30u) == 0u) continue;
                    if (tz > 0u && rl.arg_a == 2u && !tk_is_sensor(s_tk) && !is_inh && s_tk != TK_NEU_MOD) continue;
                    // Ohno-redundant backup gating: sensory nerves (match_tk==2) and cortico-cortical pyramidal tracts
                    // (match_tk==3, arg_a==1) run ONCE when intact, with their Ohno paralog automatically stepping in if
                    // a mutation disrupts the primary rule -> preserves full PV/SST basket arbors & canonical ~80/20 E/I!
                    bool is_sens_rule = (tk_is_sensor(s_tk) && rl.match_tk == 2u);
                    bool is_pyr_cort_rule = (rl.match_tk == 3u && rl.arg_a == 1u);
                    if (is_sens_rule && ran_sens_cone) continue;
                    if (is_pyr_cort_rule && ran_pyr_cort_cone) continue;
                    if (!rule_matches(rl, s_t, stage_flag)) continue;
                    if (is_sens_rule) ran_sens_cone = true;
                    if (is_pyr_cort_rule) ran_pyr_cort_cone = true;
                    ++cones_run;

                    // Authentic Sperry (1963) Ephrin-A/B Labeled-Line Sensory Columns + Litwin-Kumar & Doiron (2012)
                    // Clustered Cortical Attractors + Basal-Ganglia D1/D2 Corticobulbar Vocal Readout:
                    bool is_thalamo = (tk_is_sensor(s_tk) && tz == 1u);
                    bool is_sens_col = (tk_is_sensor(s_tk) && tz <= 1u);
                    bool is_corticobulb = (tz == 0u && sz >= 1u);
                    bool is_local_pyr = (sz == tz && tz >= 1u && !is_inh);
                    bool is_asc_pyr = (((sz == 0u && s_tk == TK_NEU_EXC && tz == 1u) || (sz >= 1u && tz == sz + 1u)) && !is_inh);
                    bool is_desc_pyr = (sz > tz && tz >= 1u && !is_inh);
                    u32 dz_c = (sz > tz) ? (sz - tz) : (tz - sz);
                    // Collision-free anatomical rank-ordered cochlear/retinal/olfactory Sperry labeled-line columns!
                    // Guarantees distinct ears (v0/v1/v2/p) & eyes ALWAYS project onto orthogonal L1 columns on ANY body plan!
                    u32 s_col = (ux & 1u) | ((uy & 1u) << 1);
                    if (tk_is_sensor(s_tk)) {
                        u32 s_rank = 0u;
                        for (u32 si = 0u; si < tid; ++si) if (tis_kind(s_tis[si]) == s_tk) ++s_rank;
                        s_col = s_rank & 3u;
                    }
                    // Authentic Braitenberg & Schuz (1998) Scale-Invariant Cortical Convergence
                    // + 3-Tier Rigotti (2013 Nature) 4-Column Prefrontal Mixed-Selectivity Executive Feedback (L5->L4, L5->L3, L4->L3)
                    // + Supramodal BA10 L5 Corticobulbar Speech Readout (L5->GLAND without 60-tick tonic motor jamming on MUSCLE/GRASP!)
                    bool is_desc_mix = (is_desc_pyr && ((tz == 3u && sz == 4u) || (tz == 4u && sz == 5u) || (tz == 3u && sz == 5u)));
                    bool is_ba10_voc = (is_corticobulb && !is_hvc_extra && sz == 5u &&
                                        !(sz == final_L - 1u || (final_L >= 3u && sz == final_L - 2u)));
                    u32 n_br = is_hvc_extra ? 6u
                                            : (is_thalamo ? (is_maturation ? 2u : 10u)
                                                          : (is_maturation ? 2u
                                                                           : ((is_corticobulb && is_inh) ? 1u
                                                                                                         : (is_ba10_voc ? 3u
                                                                                                                        : (is_corticobulb ? 8u
                                                                                                                                          : ((sz == tz && tz >= 1u && is_inh) ? 4u
                                                                                                                                                                              : (is_asc_pyr ? 6u
                                                                                                                                                                                            : (is_local_pyr ? 4u
                                                                                                                                                                                                            : ((sz == tz) ? 4u
                                                                                                                                                                                                                          : (is_desc_mix ? 4u
                                                                                                                                                                                                                                         : ((dz_c >= 2u && tz > 0u) ? 1u : 3u)))))))))));
                    u32 max_land = is_thalamo ? 8u
                                              : ((is_corticobulb && is_inh) ? 1u
                                                                            : (is_corticobulb ? 5u
                                                                                              : ((sz == tz && tz >= 1u && is_inh) ? 6u
                                                                                                                                  : (is_asc_pyr ? 6u
                                                                                                                                                : (is_local_pyr ? 4u : 5u)))));
                    for (u32 br = 0; br < n_br; ++br) {
                        u32 want_col = (is_asc_pyr && br >= 4u) ? ((s_col + 1u) & 3u)
                                       : (is_desc_mix ? ((s_col + br) & 3u)
                                                      : ((is_desc_pyr && br >= 2u) ? ((s_col + 1u) & 3u) : s_col));
                        bool is_vocal_br = (is_corticobulb && ((!is_inh && br < 3u) || is_inh));
                        bool is_mus_br = (is_corticobulb && !is_inh && br >= 3u && br < 6u);
                        if (is_vocal_br && n_gl == 0u) continue;
                        if (is_mus_br && n_mu == 0u) continue;
                        u32 br_max_land = (is_vocal_br || is_mus_br) ? (is_inh ? 1u : 2u) : max_land;
                        i32 gx_t = 8, gy_t = 12;
                        if (is_vocal_br && n_gl > 0u) {
                            u32 gpos = u32(s_gland_pos[(tid + br + sz * 3u + (is_hvc_extra ? 1u : 0u)) % n_gl]);
                            gx_t = i32(gpos & 15u);
                            gy_t = i32(gpos >> 4);
                        } else if (is_mus_br && n_mu > 0u) {
                            u32 mpos = u32(s_mus_pos[(tid + br + sz * 5u + (is_hvc_extra ? 2u : 0u)) % n_mu]);
                            gx_t = i32(mpos & 15u);
                            gy_t = i32(mpos >> 4);
                        }
                        i32 cx = i32(ux), cy = i32(uy), cz = i32(sz);
                        i32 prev_x = -1, prev_y = -1;
                        u32 steer_ch = rl.arg_b & 3u;
                        i32 steer_sign = (rl.arg_b & 4u) ? 1 : -1;
                        u32 max_steps = (is_vocal_br || is_mus_br) ? 18u : (12u + (u32(rl.param0 & 7) & 7u));
                        u32 landed_here = 0u;
                        u16 last_post = 0xffffu;

                        for (u32 stp = 0; stp < max_steps && landed_here < br_max_land; ++stp) {
                            // Blackwell Fast Vertical-Target Growth Cone Stepper with Actin Forward Persistence:
                            // - Once in target layer (cz == tz) & aligned on Sperry column, glides at stride-2 across the
                            //   column lattice with anti-backtrack momentum (eliminates 180-deg checkerboard trapping!)
                            i32 best_dx = 0, best_dy = 0, best_dz = 0;
                            i32 best_g = -100000;
                            u32 n_dirs = (cz != i32(tz) && (rl.dir_mask & 0x30u) != 0u) ? 5u : 4u;
                            u32 cur_col = (u32(cx & 1) | (u32(cy & 1) << 1));
                            // Stride-2 Sperry column gliding strictly on cortico-cortical tracts (is_asc_pyr / is_local_pyr / is_desc_pyr);
                            // primary sensory projections (is_sens_col) keep crisp 1-cell bilateral retinotopic/tonotopic topography!
                            i32 h_step = (cz == i32(tz) && !is_sens_col && (is_asc_pyr || is_local_pyr || is_desc_pyr) && cur_col == want_col) ? 2 : 1;
                            u32 step_r = mix(rng + stp * 17u + br * 97u);
                            rng = step_r;
                            for (u32 dir = 0; dir < n_dirs; ++dir) {
                                i32 nx = cx, ny = cy, nz = cz;
                                if (dir == 0u) nx += h_step;
                                else if (dir == 1u) nx -= h_step;
                                else if (dir == 2u) ny += h_step;
                                else if (dir == 3u) ny -= h_step;
                                else nz += (i32(tz) > cz) ? 1 : -1;
                                if (nx < 0 || nx >= i32(TISSUE_W) || ny < 0 || ny >= i32(TISSUE_H)) continue;
                                if (nz < 0 || nz >= i32(final_L)) continue;
                                u32 npos = u32(ny) * TISSUE_W + u32(nx);
                                u64 nt;
                                if (nz == i32(tz)) nt = s_nxt[npos];
                                else if (nz == i32(sz)) nt = s_tis[npos];
                                else {
                                    u32 nsl = s_cr_slabs[nz];
                                    if (nsl == INVALID_SLAB) continue;
                                    nt = ld_nc_u64(pool.tissue + u64(nsl) * TISSUE_CELLS + npos);
                                }
                                i32 g = tis_m(nt, steer_ch) * steer_sign;
                                if (tis_kind(nt) != TK_EMPTY) g += 22;
                                // Actin cytoskeletal forward persistence: prevents 180-deg ping-pong oscillation!
                                if (nx == prev_x && ny == prev_y && nz == cz) g -= 52;
                                // Continuous radial-glia vertical Ephrin/Reelin guidance toward target cortical layer `tz`
                                if (dir == 4u) g += (nz == i32(tz) ? 54 : 38);
                                // Real embryonic Slit/Robo midline guidance + Bilateral Corticobulbar Callosal Commissure (Jarvis 2019):
                                // - Skeletal/sensory tracts respect evolved ipsilateral vs decussated Slit/Robo polarity
                                // - Corticobulbar vocal tracts (HVC/RA -> nXIIts / human motor cortex -> nucleus ambiguus) innervate
                                //   BOTH ipsilateral & contralateral vocal folds equally so either ear can drive any phoneme!
                                if (ux != 8u && nx != 8) {
                                    bool same_hemi = ((ux < 8u) == (nx < 8));
                                    if (is_corticobulb && nz == 0) {
                                        g += (((br & 1u) == 0u) ? (same_hemi ? 18 : -8) : (same_hemi ? -10 : 24));
                                    } else if (rl.param1 < -4) {
                                        g += same_hemi ? 24 : -18;
                                    } else if (rl.param1 > 10) {
                                        g += same_hemi ? -14 : 20;
                                    }
                                }
                                // Sperry (1963) Ephrin-A/B Labeled-Line Tonotopic/Retinotopic Chemoaffinity & Clustered Cortical Columns:
                                // Prevents dense K_N sensory blending -> 4 distinct orthogonal cortical assemblies & multi-cluster WM!
                                u32 n_col = (u32(nx & 1) | (u32(ny & 1) << 1));
                                if (is_sens_col) {
                                    i32 adx = nx - i32(ux); if (adx < 0) adx = -adx;
                                    i32 ady = ny - i32(uy); if (ady < 0) ady = -ady;
                                    g += (20 - (adx + ady) * 2) + ((n_col == want_col) ? 38 : -16);
                                } else if (is_asc_pyr) {
                                    g += (n_col == want_col) ? 32 : -14;
                                } else if (is_local_pyr) {
                                    i32 adx = nx - i32(ux); if (adx < 0) adx = -adx;
                                    i32 ady = ny - i32(uy); if (ady < 0) ady = -ady;
                                    g += (14 - (adx + ady)) + ((n_col == want_col) ? 36 : -18);
                                } else if (sz == tz && tz >= 1u && is_inh) {
                                    g += (n_col != s_col) ? 18 : 4; // Broad cross-column PV+/SST+ competitive surround inhibition!
                                }
                                // Songbird/Human HVC->RA & Corticospinal BDNF/GDNF Chemotropism (Jarvis 2019 / Kravitz 2010):
                                // Dedicated BDNF-guided vocal (br=0..2 -> GLAND) and corticospinal (br=3..5 -> MUSCLE) tracts
                                // converge uniformly across ALL 4 Sperry columns with pan-cortical Purves-Lichtman reservoir sampling!
                                if (is_corticobulb && nz == 0) {
                                    if ((is_vocal_br && n_gl > 0u) || (is_mus_br && n_mu > 0u)) {
                                        i32 gdx = nx - gx_t; if (gdx < 0) gdx = -gdx;
                                        i32 gdy = ny - gy_t; if (gdy < 0) gdy = -gdy;
                                        u8 want_tk_b = is_vocal_br ? TK_GLAND : TK_MUSCLE;
                                        g += (72 - (gdx + gdy) * 16) + ((tis_kind(nt) == want_tk_b) ? 56 : 0);
                                    } else if (tis_kind(nt) == TK_GRASP) {
                                        g += 34;
                                    }
                                }
                                g += i32((step_r >> (dir * 5u)) & 31u) - 16; // Stochastic filopodia exploration!
                                if (g > best_g) { best_g = g; best_dx = nx - cx; best_dy = ny - cy; best_dz = nz - cz; }
                            }
                            if (best_dx == 0 && best_dy == 0 && best_dz == 0) break;
                            prev_x = cx; prev_y = cy;
                            cx += best_dx; cy += best_dy; cz += best_dz;
                            if (cx == i32(ux) && cy == i32(uy) && cz == i32(sz)) continue;

                            // Check if the growth cone entered a valid target cell in layer `tz` (100% shared-memory lookup!)
                            if (cz == i32(tz)) {
                                u32 post = u32(cy) * TISSUE_W + u32(cx);
                                if (post == u32(last_post)) continue;
                                u64 tgt = s_nxt[post];
                                u8 t_tk = tis_kind(tgt);
                                // Fundamental bilaterian & vertebrate neuromuscular histology (ZERO hardcoded weights or tasks!):
                                // 1. Primary sensory receptors (EYE/EAR/CHEM) & VTA MOD synapse strictly onto NEURONS
                                // 2. GABAergic interneurons (PV/SST) synapse onto ALL neurons (premotor inhibition) AND onto
                                //    vocal syrinx TK_GLAND (Area-X D2-NoGo); skeletal muscles (TK_MUSCLE) are 100% excitatory!
                                bool target_ok = false;
                                if (tk_is_sensor(s_tk) || s_tk == TK_NEU_MOD) target_ok = tk_is_neuron(t_tk);
                                else if (is_corticobulb && is_inh) target_ok = (t_tk == TK_GLAND);
                                else if (tk_is_inhibitory(s_tk)) target_ok = (tk_is_neuron(t_tk) || t_tk == TK_GLAND);
                                else if (is_vocal_br) target_ok = (t_tk == TK_GLAND);
                                else if (is_mus_br) target_ok = (t_tk == TK_MUSCLE);
                                else if (rl.arg_a == 1u) target_ok = tk_is_neuron(t_tk);
                                else if (rl.arg_a == 2u) target_ok = tk_is_effector(t_tk);
                                else target_ok = (tk_is_neuron(t_tk) || tk_is_effector(t_tk));

                                // Strict Sperry Ephrin-A/B molecular specificity onto excitatory pyramidal minicolumns
                                // (PV+/SST+ interneurons receive ALL columns freely for cross-column competitive inhibition;
                                //  only fall back to another column if 0 matching-column cells exist after step 14!)
                                u32 p_col = (u32(cx & 1) | (u32(cy & 1) << 1));
                                if (target_ok && t_tk == TK_NEU_EXC && p_col != want_col) {
                                    if ((is_sens_col || is_local_pyr || is_asc_pyr || is_desc_pyr) && (landed_here > 0u || stp < 14u))
                                        target_ok = false;
                                }

                                bool is_cb_res = (is_corticobulb && (t_tk == TK_GLAND || t_tk == TK_MUSCLE));
                                if (target_ok && (rl.tag_bits == 0u || (tis_tag(tgt) & rl.tag_bits) != 0u || stp >= 4u) &&
                                    (is_cb_res || s_in_cnt[post] < SYN_PER_STRIPE)) {
                                    u32 dst_slot = SYN_PER_STRIPE;
                                    if (is_cb_res) {
                                        // Authentic Purves-Lichtman (1980) Pan-Cortical BDNF Trophic Competition:
                                        // - TK_GLAND: slots 0..13 uniform pyramidal reservoir + slots 14..15 Area-X D2-NoGo!
                                        // - TK_MUSCLE: all 16 slots uniform excitatory pyramidal reservoir across ALL 4 Sperry columns!
                                        if (is_inh) {
                                            dst_slot = 14u + (mix(rng ^ post ^ stp) & 1u);
                                        } else {
                                            u32 exc_cap = (is_hvc_extra || t_tk == TK_MUSCLE) ? SYN_PER_STRIPE : 14u;
                                            u32 arr_k = atomic_add_gen_u32(&s_in_cnt[post], 1u);
                                            if (arr_k < exc_cap) dst_slot = arr_k;
                                            else {
                                                u32 r_res = mix(rng ^ (arr_k * 0x85ebca6bu) ^ post);
                                                if ((r_res % (arr_k + 1u)) < exc_cap) dst_slot = (r_res >> 16) % exc_cap;
                                            }
                                        }
                                    } else {
                                        dst_slot = atomic_add_gen_u32(&s_in_cnt[post], 1u);
                                    }
                                    if (dst_slot < SYN_PER_STRIPE) {
                                        // Physical conduction delay strictly proportional to crawled axon path length!
                                        u32 delay = u32(clamp_i32(i32((stp + 1u) >> 1), 0, 7));
                                        u32 comp = 0u;
                                        if (tk_is_effector(t_tk)) comp = 0u;
                                        else if (s_tk == TK_NEU_SST) comp = 1u;
                                        else if (s_tk == TK_NEU_PV || tk_is_sensor(s_tk)) comp = 0u;
                                        else if (t_tk == TK_NEU_EXC) {
                                            // Canonical Larkum (2013) cortical compartmentalization:
                                            // Ascending labeled-line (br<4) & proximal local collaterals (stp<3) drive BASAL dendrites (comp=0);
                                            // Descending predictions (sz>tz), cross-column conjunctions (br>=4) & long-range lateral
                                            // context (stp>=3) target APICAL BAC tufts (comp=1) — zero spontaneous dark ignition!
                                            comp = (sz > tz || (is_asc_pyr && br >= 4u) || (sz == tz && (rl.param1 > 0 || stp >= 3u))) ? 1u : 0u;
                                        }
                                        // Real mammalian Class-1A Driver vs Modulator & heavy-tailed spine-size diversity (Sherman 1998 / Song 2005):
                                        // - Thalamocortical sensory -> L1 drivers: crisp 1-synapse ignition EPSPs (28..48) -> high sensory SNR!
                                        // - Ascending feedforward (sz < tz): strong laminar drivers (16..34) -> clean L1..L5 propagation!
                                        // - Local recurrent / feedback cortex: lean subcritical spines (6..20) -> zero +DA seizure lock!
                                        i32 raw_m = i32(rl.param0 < 0 ? -rl.param0 : rl.param0) / 8;
                                        u32 sp_h = mix(rng ^ (tid * 997u) ^ (post * 313u));
                                        i32 spine_j = ((sp_h & 3u) == 0u) ? (7 + i32((sp_h >> 2) & 7u)) : (i32((sp_h >> 2) & 7u) - 7);
                                        i32 base_exc = is_thalamo ? 36
                                                       : ((sz < tz && tz > 0u) ? 24
                                                          : ((is_corticobulb && t_tk == TK_GLAND) ? 22
                                                             : ((tz == 0u && t_tk == TK_GLAND) ? 8 : (tz == 0u ? 18 : 11))));
                                        i32 mag = is_inh
                                                      ? ((t_tk == TK_GLAND || t_tk == TK_MUSCLE) ? clamp_i32(4 + (raw_m >> 1) + i32(rng & 1u), 4, 7)
                                                                                                 : clamp_i32(15 + raw_m + i32(rng & 7u), 15, 23))
                                                      : clamp_i32(base_exc + raw_m + spine_j,
                                                                  is_thalamo ? 28 : ((sz < tz && tz > 0u) ? 14 : ((is_corticobulb && t_tk == TK_GLAND) ? 12 : ((tz == 0u && t_tk == TK_GLAND) ? 5 : 7))),
                                                                  is_thalamo ? 48 : ((sz < tz && tz > 0u) ? 30 : ((is_corticobulb && t_tk == TK_GLAND) ? 34 : ((tz == 0u && t_tk == TK_GLAND) ? 16 : (tz == 0u ? 34 : 18)))));
                                        i32 w = is_inh ? -mag : mag;
                                        // Balanced 4-receptor family expression across motor/vocal effectors, interneurons & cortex:
                                        // Effectors express D1 Go & Fast-Reversal (0,1); interneurons express D2 & iSTDP (0..3);
                                        // cortical pyramidal targets express all 4 families (D1, Reversal, NMDA+BAC, Consolidation)!
                                        u32 rule_id;
                                        if (tk_is_effector(t_tk)) rule_id = ((rl.stage_pri >> 6) + tid + post) & 1u;
                                        else if (is_inh) rule_id = (comp ? 2u : 0u) | (((rl.stage_pri >> 6) + post) & 1u);
                                        else rule_id = ((rl.stage_pri >> 6) + (comp ? 2u : 0u) + ((tid ^ post) & 3u)) & 3u;
                                        // Authentic Wang (2006) / Mongillo (2008) Activity-Silent Prefrontal STF Specialization:
                                        // Local deep prefrontal recurrent excitatory synapses (sz==tz>=2, basal) initialize at
                                        // subcritical resting STF (stp=1 -> 3x base, facilitates to 5 -> 7x = 2.33x WM gain!);
                                        // Local recurrent L2..L5 pyramidal synapses start at resting STF (stp=1 -> 3-Tier Executive Hierarchy!);
                                        // feedforward sensory/ascending, L6+ global workspace & inhibitory synapses initialize at full STD (stp=6)!
                                        u32 init_stp = (sz == tz && tz >= 2u && tz <= 5u && t_tk == TK_NEU_EXC && !is_inh && comp == 0u) ? 1u : 6u;
                                        u32 syn_w = pack_synapse(tid, w, 0, delay, comp, rule_id & 3u, init_stp);
                                        if (is_cb_res) atomic_exch_gen_u32(&s_in_syn[post * SYN_PER_STRIPE + dst_slot], syn_w);
                                        else s_in_syn[post * SYN_PER_STRIPE + dst_slot] = syn_w;
                                    }
                                    if (is_cb_res || dst_slot < SYN_PER_STRIPE) {
                                        last_post = u16(post);
                                        ++landed_here;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            sync_block();

            // 5. In-Place Shared-Memory Deduplication (100% distinct presynaptic partners per stripe — FREES 16 live registers!)
            //    and commit into Coalesced Variable-Fan-In Blackwell v4.u32 SynSlabs!
            u32 raw_landed = (tz == 0u && (tis_kind(t_my_word) == TK_GLAND || tis_kind(t_my_word) == TK_MUSCLE))
                                 ? SYN_PER_STRIPE
                                 : min_u32(s_in_cnt[tid], SYN_PER_STRIPE);
            u32 uniq = 0u;
            u32 exc_landed = 0u;
            u32 base_syn = tid * SYN_PER_STRIPE;
            if (raw_landed > 0u) {
                for (u32 k = 0; k < raw_landed; ++k) {
                    u32 sw = s_in_syn[base_syn + k];
                    i32 sw_w = syn_weight(sw);
                    if (sw_w == 0) continue;
                    u32 p_id = sw & 0xffu;
                    bool dup = false;
                    for (u32 j = 0; j < uniq; ++j) {
                        if ((s_in_syn[base_syn + j] & 0xffu) == p_id) { dup = true; break; }
                    }
                    if (!dup && uniq < SYN_PER_STRIPE) {
                        s_in_syn[base_syn + uniq++] = sw;
                        if (sw_w > 0) ++exc_landed;
                    }
                }
                for (u32 k = uniq; k < SYN_PER_STRIPE; ++k) s_in_syn[base_syn + k] = 0u;
            }
            u32 landed = uniq;
            bool wants_write = (landed > 0u && my_st_idx < MAX_STRIPES_PER_LAYER);

            // Blackwell exact stripe-bitmask skip: execute ONLY st_pass indices that actually have incoming cones!
            if (tid == 0u) s_need_slab = 0u;
            sync_block();
            if (wants_write) atomic_or_gen_u32(&s_need_slab, 1u << u32(my_st_idx));
            sync_block();
            u32 pass_mask = s_need_slab;
            if (pass_mask == 0u) continue;

            // Check across all 256 threads in `tz` which active stripe indices need a freshly allocated SynSlab!
            u8 st_before = my_st_idx;
            for (u32 st_pass = 0; st_pass < MAX_STRIPES_PER_LAYER; ++st_pass) {
                if ((pass_mask & (1u << st_pass)) == 0u) continue;
                bool me_this = wants_write && (u32(st_before) == st_pass);
                if (s_t_lss[st_pass] == INVALID_SLAB) {
                    if (tid == 0u) {
                        u32 new_ss = alloc_slab(pool.syn_bitmap, pool.max_syn_slabs, dev_seed + tz * 53u + sz * 19u + st_pass);
                        pool.layer_syn_slabs[u64(t_sl) * MAX_STRIPES_PER_LAYER + st_pass] = new_ss;
                        s_t_lss[st_pass] = new_ss;
                    }
                    sync_block();
                    u32 new_ss = s_t_lss[st_pass];
                    if (new_ss != INVALID_SLAB) {
                        u64 sb = u64(new_ss) * LAYER_SYN_QUADS;
                        #pragma unroll
                        for (u32 g = 0; g < QUADS_PER_STRIPE; ++g)
                            st_syn_quad(pool.syn_slabs + sb + u64(g) * TISSUE_CELLS + tid, 0u, 0u, 0u, 0u);
                        pool.stripe_src_layer[u64(new_ss) * TISSUE_CELLS + tid] = 0u;
                    }
                    sync_block();
                }

                if (me_this) {
                    u32 ss = s_t_lss[st_pass];
                    if (ss != INVALID_SLAB) {
                        u64 sb = u64(ss) * LAYER_SYN_QUADS;
                        #pragma unroll
                        for (u32 g = 0; g < QUADS_PER_STRIPE; ++g) {
                            st_syn_quad(pool.syn_slabs + sb + u64(g) * TISSUE_CELLS + tid,
                                        s_in_syn[base_syn + g * 4u + 0u], s_in_syn[base_syn + g * 4u + 1u],
                                        s_in_syn[base_syn + g * 4u + 2u], s_in_syn[base_syn + g * 4u + 3u]);
                        }
                        // 5-bit src_l (0..31) | bit 5 = F_ELIG (0x20) | bit 6 = F_DEP (0x40, stp=6<7)!
                        pool.stripe_src_layer[u64(ss) * TISSUE_CELLS + tid] = u8((sz & 0x1fu) | 0x40u);
                        my_exc_syn_tot += exc_landed;
                        u32 exc_q = min_u32(15u, (my_exc_syn_tot + 4u) >> 3);
                        if (my_exc_syn_tot > 0u && exc_q == 0u) exc_q = 1u;
                        my_st_idx = u8(st_pass + 1u);
                        pool.cell_stripes[u64(t_sl) * TISSUE_CELLS + tid] = u8((exc_q << 4) | (my_st_idx & 0x0fu));
                        total_cones_landed += landed;
                    }
                }
                sync_block();
            }
        }
    }

    // 6. Parallel 256-Thread Biomechanical Phenotype & Connectome Reduction Across ALL Unified Layers!
    // (Eliminates the 2,560-cell serial tid==0 global memory scan — massive Blackwell morphogenesis speedup!)
    if (tid < TK_TYPES) s_tk_cnt[tid] = 0u;
    if (tid < 8u) s_stat_u[tid] = 0u;
    if (tid < 9u) s_stat_f[tid] = 0.0f;
    sync_block();

    float px = float(i32(ux) - 8);
    float py = float(i32(uy) - 8);
    u32 my_cells = 0u, my_neu = 0u, my_syn = 0u, my_mxf = 0u;
    float my_mass = 0.0f, my_cx = 0.0f, my_cy = 0.0f;
    u32 my_bhash = 0u, my_nhash = 0u;
    u8 surf_tk = body.surface[tid];

    for (u32 z = 0; z < final_L; ++z) {
        u32 sl = s_cr_slabs[z];
        if (sl == INVALID_SLAB) continue;
        u8 tk = tis_kind(ld_nc_u64(pool.tissue + u64(sl) * TISSUE_CELLS + tid));
        u32 st_c = u32(ld_nc_u8(pool.cell_stripes + u64(sl) * TISSUE_CELLS + tid)) & 0x0fu;
        if (z == 0u && tk != TK_EMPTY) my_bhash ^= mix((tid << 8) | u32(tk) | 0x13579b00u);
        if (tk != TK_EMPTY) {
            my_nhash ^= mix((z << 16) | (tid << 8) | u32(tk) | (st_c << 4));
            ++my_cells;
            if (tk < TK_TYPES) atomic_add_gen_u32(&s_tk_cnt[tk], 1u);
            if (tk_is_neuron(tk)) ++my_neu;
            if (st_c > 0u) {
                u32 fan = st_c * SYN_PER_STRIPE;
                my_syn += fan;
                if (fan > my_mxf) my_mxf = fan;
            }
            // Realistic mammalian encephalization allometry: 2,000-4,000 deep CNS neurons are ~2-5% of body mass!
            float cm = (z > 0u) ? 0.00025f
                       : (tk == TK_STRUCT ? 1.80f
                       : (tk == TK_GRASP ? 1.30f
                       : (tk == TK_MUSCLE ? 1.10f
                       : (tk == TK_GLAND ? 0.35f
                       : (tk == TK_STEM ? 0.18f
                       : (tk_is_neuron(tk) ? 0.20f : 0.25f))))));
            my_mass += cm;
            my_cx += px * cm;
            my_cy += py * cm;
        }
    }

    s_in_cnt[tid] = u32(surf_tk);
    u32 w_cones = warp_add_u32(total_cones_landed);
    u32 w_cells = warp_add_u32(my_cells);
    u32 w_neu = warp_add_u32(my_neu);
    u32 w_syn = warp_add_u32(my_syn);
    u32 w_surf = warp_add_u32(surf_tk != TK_EMPTY ? 1u : 0u);
    if (lane_id() == 0u) {
        if (w_cells) atomic_add_gen_u32(&s_stat_u[0], w_cells);
        if (w_neu) atomic_add_gen_u32(&s_stat_u[1], w_neu);
        if (w_syn) atomic_add_gen_u32(&s_stat_u[2], w_syn);
        if (w_surf) atomic_add_gen_u32(&s_stat_u[6], w_surf);
        if (w_cones) atomic_add_gen_u32(&s_stat_u[7], w_cones);
    }
    s_in_syn[tid] = my_mxf;
    s_in_syn[256u + tid] = my_bhash;
    s_in_syn[512u + tid] = my_nhash;
    u32 w_mass_q = warp_add_u32(u32(my_mass * 256.0f + 0.5f));
    u32 w_cx_q = warp_add_u32(u32(i32(my_cx * 256.0f)));
    u32 w_cy_q = warp_add_u32(u32(i32(my_cy * 256.0f)));
    if (lane_id() == 0u) {
        s_warp_h[tid >> 5] = w_mass_q;
        s_in_syn[768u + (tid >> 5)] = w_cx_q;
        s_in_syn[776u + (tid >> 5)] = w_cy_q;
    }
    sync_block();

    u32 sym_m_t = 0u, sym_p_t = 0u;
    if (ux >= 1u && ux < 8u) {
        u8 a = u8(s_in_cnt[tid]);
        u8 b = u8(s_in_cnt[uy * TISSUE_W + (16u - ux)]);
        if (a || b) { sym_p_t = 1u; if (a == b) sym_m_t = 1u; }
    }
    u32 w_sym_m = warp_add_u32(sym_m_t);
    u32 w_sym_p = warp_add_u32(sym_p_t);
    if (lane_id() == 0u) {
        if (w_sym_m) atomic_add_gen_u32(&s_stat_u[4], w_sym_m);
        if (w_sym_p) atomic_add_gen_u32(&s_stat_u[5], w_sym_p);
    }

    if (tid == 0u) {
        u32 mq = 0u; i32 cxq = 0, cyq = 0;
        #pragma unroll
        for (u32 w = 0; w < 8u; ++w) {
            mq += s_warp_h[w];
            cxq += i32(s_in_syn[768u + w]);
            cyq += i32(s_in_syn[776u + w]);
        }
        float mass = float(mq) * (1.0f / 256.0f);
        if (mass < 1.0f) mass = 1.0f;
        s_stat_f[0] = mass;
        s_stat_f[1] = (float(cxq) * (1.0f / 256.0f)) / mass;
        s_stat_f[2] = (float(cyq) * (1.0f / 256.0f)) / mass;
    }
    sync_block();

    // Parallel biomechanics (real lever-arm torque requires |dx| > 0 — ends 1-pixel blind worm steering exploit!)
    float cx_b = s_stat_f[1], cy_b = s_stat_f[2];
    float my_inert = 0.0f, my_thr = 0.0f, my_trq = 0.0f, my_gr = 0.0f, my_gl = 0.0f, my_ar = 0.0f;
    if (surf_tk != TK_EMPTY) {
        float dx = px - cx_b;
        float dy = py - cy_b;
        float r2 = dx * dx + dy * dy;
        my_inert = (r2 * 0.25f + 0.4f) * (surf_tk == TK_STRUCT ? 1.8f : 1.0f);
        if (surf_tk == TK_MUSCLE) {
            my_thr = 0.36f + (dy > 0.0f ? 0.08f * dy : 0.03f);
            // Authentic rotational lever-arm physics: turning torque requires real lateral muscle offset |dx| > 0!
            my_trq = 0.26f * fabsf_dev(dx);
        } else if (surf_tk == TK_GRASP) {
            my_gr = 0.45f + (dy < 0.0f ? 0.12f : 0.04f);
        } else if (surf_tk == TK_GLAND) {
            my_gl = 0.50f;
        } else if (surf_tk == TK_STRUCT) {
            my_ar = 0.22f;
        }
    }
    u32 wi = warp_add_u32(u32(my_inert * 256.0f + 0.5f));
    u32 wth = warp_add_u32(u32(my_thr * 256.0f + 0.5f));
    u32 wtr = warp_add_u32(u32(my_trq * 256.0f + 0.5f));
    u32 wgr = warp_add_u32(u32(my_gr * 256.0f + 0.5f));
    u32 wgl = warp_add_u32(u32(my_gl * 256.0f + 0.5f));
    u32 war = warp_add_u32(u32(my_ar * 256.0f + 0.5f));
    if (lane_id() == 0u) {
        u32 w = tid >> 5;
        s_in_syn[800u + w] = wi;
        s_in_syn[808u + w] = wth;
        s_in_syn[816u + w] = wtr;
        s_in_syn[824u + w] = wgr;
        s_in_syn[832u + w] = wgl;
        s_in_syn[840u + w] = war;
    }
    sync_block();

    if (tid == 0u) {
        u32 mx_fan = 0u, bhash = 2166136261u, nhash = 2166136261u;
        for (u32 i = 0; i < TISSUE_CELLS; ++i) {
            if (s_in_syn[i] > mx_fan) mx_fan = s_in_syn[i];
            bhash ^= s_in_syn[256u + i];
            nhash ^= s_in_syn[512u + i];
        }
        u32 iq = 0u, thq = 0u, trq = 0u, grq = 0u, glq = 0u, arq = 0u;
        #pragma unroll
        for (u32 w = 0; w < 8u; ++w) {
            iq += s_in_syn[800u + w]; thq += s_in_syn[808u + w]; trq += s_in_syn[816u + w];
            grq += s_in_syn[824u + w]; glq += s_in_syn[832u + w]; arq += s_in_syn[840u + w];
        }
        for (u32 k = 0; k < TK_TYPES; ++k) body.counts[k] = u16(s_tk_cnt[k]);
        u32 tot_cells = s_stat_u[0], act_neu = s_stat_u[1], tot_syn = s_stat_u[2];
        u32 sym_m = s_stat_u[4], sym_p = s_stat_u[5], surf_cells = s_stat_u[6], cones_tot = s_stat_u[7];
        float mass = s_stat_f[0];
        float inertia = 1.0f + float(iq) * (1.0f / 256.0f);
        float raw_thr = float(thq) * (1.0f / 256.0f);
        float raw_trq = float(trq) * (1.0f / 256.0f);
        float raw_gr = float(grq) * (1.0f / 256.0f);
        float raw_gl = float(glq) * (1.0f / 256.0f);
        float raw_ar = float(arq) * (1.0f / 256.0f);

        float eff_thr = (raw_thr * 1.48f) / (1.0f + 0.18f * raw_thr);
        float eff_trq = (raw_trq * 1.50f) / (1.0f + 0.14f * raw_trq);
        float eff_gr = (raw_gr * 1.25f) / (1.0f + 0.35f * raw_gr);
        float eff_gl = (raw_gl * 1.30f) / (1.0f + 0.45f * raw_gl);
        float eff_ar = clampf(raw_ar / (1.0f + 0.85f * raw_ar), 0.0f, 0.50f);

        u32 deep_cells = (tot_cells > surf_cells) ? (tot_cells - surf_cells) : 0u;
        u32 ex_eye = (body.counts[TK_EYE] > 4u) ? (body.counts[TK_EYE] - 4u) : 0u;
        u32 ex_ear = (body.counts[TK_EAR] > 4u) ? (body.counts[TK_EAR] - 4u) : 0u;
        u32 ex_chm = (body.counts[TK_CHEM] > 2u) ? (body.counts[TK_CHEM] - 2u) : 0u;
        u32 ex_gld = (body.counts[TK_GLAND] > 4u) ? (body.counts[TK_GLAND] - 4u) : 0u;
        u32 ex_stem = (body.counts[TK_STEM] > 6u) ? (body.counts[TK_STEM] - 6u) : 0u;
        u32 ex_str = (body.counts[TK_STRUCT] > 6u) ? (body.counts[TK_STRUCT] - 6u) : 0u;
        u32 ex_grp = (body.counts[TK_GRASP] > 6u) ? (body.counts[TK_GRASP] - 6u) : 0u;
        u32 ex_mod = (body.counts[TK_NEU_MOD] > 6u) ? (body.counts[TK_NEU_MOD] - 6u) : 0u;
        float sens_tumor = 0.0028f * float(ex_eye * ex_eye) + 0.0028f * float(ex_ear * ex_ear) +
                           0.0036f * float(ex_gld * ex_gld) +
                           0.0024f * float(ex_chm * ex_chm) +
                           0.0022f * float(ex_stem * ex_stem) +
                           0.0032f * float(ex_str * ex_str) +
                           0.0024f * float(ex_grp * ex_grp) +
                           0.0020f * float(ex_mod * ex_mod); // Warburg teratoma & immobile-shell/VTA hypertrophy suppression!
        // Myelinated neocortical scaling: oligodendrocyte saltatory conduction keeps 1,000-4,000+ deep cortical neurons lean!
        float bcost = 0.025f + 0.00018f * float(surf_cells) + 0.00000012f * float(deep_cells) +
                      0.000000004f * float(tot_syn) +
                      0.000002f * float(final_L > 1u ? final_L - 1u : 0u) +
                      0.000025f * float(n_rules) +
                      0.00016f * float(body.counts[TK_MUSCLE]) +
                      0.00032f * float(body.counts[TK_GRASP] + body.counts[TK_STRUCT]) +
                      0.00002f * float(body.counts[TK_EYE] + body.counts[TK_EAR]) +
                      0.00004f * float(body.counts[TK_GLAND]) +
                      0.00140f * float(body.counts[TK_CHEM]) + sens_tumor;

        // Honest bilateral hydrodynamic streamlining: 1-pixel-wide rods (sym_p < 3) lack bilateral stabilizing fins!
        u32 sym_pct = (sym_p >= 3u) ? (100u * sym_m / sym_p) : (35u + sym_p * 15u);

        body.total_cells = u16(min_u32(65535u, tot_cells));
        body.active_neurons = u16(min_u32(65535u, act_neu));
        body.total_synapses = is_maturation ? (body.total_synapses + cones_tot) : cones_tot;
        body.max_fanin = u16(mx_fan);
        body.grown_layers = u8(final_L);
        body.symmetry_pct = u8(sym_pct);
        body.mass = mass;
        body.inv_mass = 1.0f / mass;
        body.inertia = inertia;
        body.basal_cost = bcost;
        body.com_x = cx_b;
        body.com_y = cy_b;
        body.thrust_cap = eff_thr;
        body.torque_cap = eff_trq;
        body.grasp_power = eff_gr;
        body.gland_power = eff_gl;
        body.armor_factor = eff_ar;
        body.body_hash = bhash;
        body.brain_hash = nhash;
        body.cones_landed = is_maturation ? (body.cones_landed + cones_tot) : cones_tot;
    }
    sync_block();
}

} // namespace axion::genesis::gpu
