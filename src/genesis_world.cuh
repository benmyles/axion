#pragma once
#include "genesis_dna.cuh"

namespace axion::genesis::gpu {

// ---------------------------------------------------------------------------
// Realistic Malleable GPU World: Geophysics, Shallow-Water Hydrodynamics,
// Thermal Conduction + Polymer Insulation, 4-Species Chemistry, Circadian
// Day/Night Cycles, 2D Acoustic Wave Equation & Barometric Precursor Storms!
// ---------------------------------------------------------------------------

AX_FN void world_init_cell(WorldBuffers wb, u32 x, u32 y, u32 seed, u32 shift_chem) {
    u32 W = wb.w, H = wb.h, MX = wb.mask_x, MY = wb.mask_y, SY = wb.shift_y;
    i32 HW = i32(W >> 1), HH = i32(H >> 1);
    u32 idx = (y << SY) | x;
    float fx = float(x) * (6.2831853f / float(W));
    float fy = float(y) * (6.2831853f / float(H));
    float ph1 = float(mix(seed + 11u) & 1023u) * 0.0061359f;
    float ph2 = float(mix(seed + 29u) & 1023u) * 0.0061359f;

    // Runtime-Scalable Continental Geography (256x256 / 512x512 / 1024x1024): mountain ridges, ocean basins, vents & ore!
    float h = 1.65f
              + 1.05f * fast_sin(fx + ph1) * fast_cos(fy + ph2)
              + 0.65f * fast_sin(2.0f * fx - fy + ph2)
              + 0.42f * fast_cos(3.0f * fy + 2.0f * fx + ph1)
              + 0.25f * fast_sin(4.0f * fx + 3.0f * fy - ph2);
    if (h < 0.1f) h = 0.1f;

    u8 mat = MAT_LOAM;
    float vent_heat = 0.0f;
    u32 nv = wb.num_vents ? wb.num_vents : 10u;
    for (u32 v = 0; v < nv; ++v) {
        u32 vx = mix(seed + v * 997u + (shift_chem ? 4099u : 0u)) & MX;
        u32 vy = mix(seed + v * 1999u + 77u + (shift_chem ? 8191u : 0u)) & MY;
        i32 dx = i32(x) - i32(vx); if (dx > HW) dx -= i32(W); if (dx < -HW) dx += i32(W);
        i32 dy = i32(y) - i32(vy); if (dy > HH) dy -= i32(H); if (dy < -HH) dy += i32(H);
        i32 d2 = dx * dx + dy * dy;
        if (d2 <= 10) { mat = MAT_VENT; vent_heat = 1.0f; h = 0.45f; }
        else if (d2 <= 36 && mat == MAT_LOAM) { mat = MAT_CATALYST; }
    }
    if (mat == MAT_LOAM) {
        float ore_f = fast_sin(2.0f * fx + 2.0f * fy + ph1 * 2.0f);
        // Reachable coastal & highland ore ridges so organisms can quarry & chip carryable OBJ_STONE tools!
        if (h > 1.72f && ore_f > 0.15f) mat = MAT_ORE;
    }

    float water = (h < 2.25f) ? (2.25f - h) * 0.78f + 0.15f : 0.08f;
    float temp = 0.40f + 0.55f * vent_heat - 0.06f * water;
    float ca = (mat == MAT_ORE) ? 0.92f : (0.16f + 0.10f * water);
    float cb = (water > 0.25f && h < 2.0f) ? 0.40f : 0.08f;
    float cc = (mat == MAT_CATALYST || mat == MAT_VENT) ? 0.35f : 0.02f;

    wb.terrain[idx] = h;
    wb.material[idx] = mat;
    wb.water[idx] = water;
    wb.water_next[idx] = water;
    wb.vx[idx] = 0.0f;
    wb.vy[idx] = 0.0f;
    wb.temp[idx] = temp;
    wb.temp_next[idx] = temp;
    wb.chem_a[idx] = ca;
    wb.chem_a_next[idx] = ca;
    wb.chem_b[idx] = cb;
    wb.chem_b_next[idx] = cb;
    wb.chem_c[idx] = cc;
    wb.chem_c_next[idx] = cc;
    wb.chem_d[idx] = 0.0f;
    wb.chem_d_next[idx] = 0.0f;
    wb.acoust_p[idx] = 0.0f;
    wb.acoust_prev[idx] = 0.0f;
    wb.acoust_next[idx] = 0.0f;
    wb.acoust_v0[idx] = 0.0f;
    wb.acoust_v0_prev[idx] = 0.0f;
    wb.acoust_v0_next[idx] = 0.0f;
    wb.acoust_v1[idx] = 0.0f;
    wb.acoust_v1_prev[idx] = 0.0f;
    wb.acoust_v1_next[idx] = 0.0f;
    wb.acoust_v2[idx] = 0.0f;
    wb.acoust_v2_prev[idx] = 0.0f;
    wb.acoust_v2_next[idx] = 0.0f;
    wb.light[idx] = 0.8f;
    wb.cell_occ[idx] = 0u;
    wb.tile_occ_id[idx] = 0u;
    wb.tile_obj_id[idx] = 0u;
}

// ---------------------------------------------------------------------------
// Fused Tiled Shared-Memory 18x18 Halo Physics + Chemistry + 3-Band Acoustics!
// ---------------------------------------------------------------------------
AX_FN void world_step_tile(WorldBuffers wb, u32 tick, u32 shift_chem, u32 challenge_tier, u32 seed) {
    u32 W = wb.w, H = wb.h, MX = wb.mask_x, MY = wb.mask_y, SY = wb.shift_y;
    i32 HW = i32(W >> 1), HH = i32(H >> 1);
    u32 lx = tid_x();
    u32 ly = tid_y();
    u32 gx = (cta_x() * 16u + lx) & MX;
    u32 gy = (cta_y() * 16u + ly) & MY;

    __attribute__((shared)) float s_H[18][18];
    __attribute__((shared)) float s_t[18][18];
    __attribute__((shared)) float s_ca[18][18];
    __attribute__((shared)) float s_cb[18][18];
    __attribute__((shared)) float s_cc[18][18];
    __attribute__((shared)) float s_cd[18][18];
    __attribute__((shared)) float s_ap[18][18];
    __attribute__((shared)) float s_av0[18][18];
    __attribute__((shared)) float s_av1[18][18];
    __attribute__((shared)) float s_av2[18][18];
    __attribute__((shared)) float s_ter[18][18];
    __attribute__((shared)) u8 s_mat[18][18];
    __attribute__((shared)) u32 s_reef[64];  // Up to 64 nomadic reefs (10-bit x | 10-bit y | flags<<20, supports 1024x1024!)
    __attribute__((shared)) u32 s_storm[12]; // Up to 12 barometric storms (10-bit x | 10-bit y | mode<<20!)

    u32 flat = ly * 16u + lx;
    u32 bloom_phase = ((tick / 180u) ^ shift_chem) & 1u;
    u32 n_reefs = min_u32(wb.num_reefs ? wb.num_reefs : 16u, 64u);
    u32 n_storms = min_u32(wb.num_storms ? wb.num_storms : 3u, 12u);
    if (flat < n_reefs) {
        u32 rf = flat;
        u32 reef_epoch = (tick + rf * 59u) / 360u; // Nomadic oceanic blooms migrate every 360t -> rewards active swimmers!
        u32 rx = mix(seed + reef_epoch * 1993u + rf * 4099u + (shift_chem ? 7717u : 0u)) & MX;
        u32 ry = mix(seed + reef_epoch * 3571u + rf * 7919u + (shift_chem ? 2203u : 0u)) & MY;
        u32 r_cyc = (tick + rf * 59u) % 150u;
        // Authentic Trisolaris-7 2-Syllable Delayed-Beacon Working-Memory Ecology:
        //   r_cyc < 18  : Pre-bloom Syllable 1 (Warm=v0, Cool=v2)
        //   18 <= r < 36: Pre-bloom Syllable 2 (Warm=v2, Cool=v0 — reversed temporal order, identical total energy!)
        //   36 <= r < 60: 24-tick silent dark gap (selects for prefrontal STF calcium working-memory retention!)
        //   60 <= r <138: Dark fruiting harvest (rich chem_c + chem_b blooms in silence!)
        u32 r_flags = (r_cyc < 36u ? 1u : 0u) | ((r_cyc >= 60u && r_cyc < 138u) ? 2u : 0u) |
                      (((rf & 1u) == 0u) ? 4u : 0u) | ((((rf & 1u) == 0u) == (bloom_phase == 0u)) ? 8u : 0u) |
                      ((r_cyc >= 18u && r_cyc < 36u) ? 16u : 0u);
        s_reef[rf] = (rx & 1023u) | ((ry & 1023u) << 10) | (r_flags << 20);
    }
    if (flat < n_storms) {
        u32 sc = flat;
        u32 st_pk = 0u;
        if (challenge_tier >= 1u) {
            u32 storm_period = (challenge_tier >= 2u) ? 155u : 210u;
            u32 t_off = tick + sc * (storm_period / max_u32(1u, n_storms));
            u32 storm_id = t_off / storm_period;
            u32 st_t = t_off % storm_period;
            if (st_t < 72u) {
                u32 sx_e = mix(seed + storm_id * 7919u + sc * 5113u + (shift_chem ? 13107u : 0u)) & MX;
                u32 sy_e = mix(seed + storm_id * 104729u + sc * 9127u + 313u + (shift_chem ? 2999u : 0u)) & MY;
                u32 mode = (st_t < 10u) ? 1u : ((st_t >= 30u && st_t < 70u) ? 2u : 0u);
                if (mode) st_pk = (sx_e & 1023u) | ((sy_e & 1023u) << 10) | (mode << 20);
            }
        }
        s_storm[sc] = st_pk;
    }
    for (u32 i = flat; i < 324u; i += 256u) {
        u32 sy = (i * 3641u) >> 16; // Exact i / 18 for 0 <= i < 324 (zero INT32 division!)
        u32 sx = i - sy * 18u;
        u32 wx = (cta_x() * 16u + sx + W - 1u) & MX;
        u32 wy = (cta_y() * 16u + sy + H - 1u) & MY;
        u32 widx = (wy << SY) | wx;
        float ter = ld_nc_f32(wb.terrain + widx);
        float wat = ld_nc_f32(wb.water + widx);
        s_ter[sy][sx] = ter;
        s_H[sy][sx] = ter + wat;
        s_t[sy][sx] = ld_nc_f32(wb.temp + widx);
        s_ca[sy][sx] = ld_nc_f32(wb.chem_a + widx);
        s_cb[sy][sx] = ld_nc_f32(wb.chem_b + widx);
        s_cc[sy][sx] = ld_nc_f32(wb.chem_c + widx);
        s_cd[sy][sx] = ld_nc_f32(wb.chem_d + widx);
        s_ap[sy][sx] = ld_nc_f32(wb.acoust_p + widx);
        s_av0[sy][sx] = ld_nc_f32(wb.acoust_v0 + widx);
        s_av1[sy][sx] = ld_nc_f32(wb.acoust_v1 + widx);
        s_av2[sy][sx] = ld_nc_f32(wb.acoust_v2 + widx);
        s_mat[sy][sx] = ld_nc_u8(wb.material + widx);
    }
    sync_block();

    u32 sx = lx + 1u;
    u32 sy = ly + 1u;
    u32 idx = (gy << SY) | gx;
    u8 mat = s_mat[sy][sx];
    float ter = s_ter[sy][sx];
    float h_surf = s_H[sy][sx];
    float w = h_surf - ter;
    bool is_wall = (mat == MAT_POLYMER || ter > 3.6f);

    // Count nearby built polymer walls (real physical thermal insulation & toxic shelter!)
    u32 wall_nbrs = (s_mat[sy][sx - 1u] == MAT_POLYMER ? 1u : 0u) +
                    (s_mat[sy][sx + 1u] == MAT_POLYMER ? 1u : 0u) +
                    (s_mat[sy - 1u][sx] == MAT_POLYMER ? 1u : 0u) +
                    (s_mat[sy + 1u][sx] == MAT_POLYMER ? 1u : 0u);
    float shelter_f = clampf(float(wall_nbrs) * 0.25f, 0.0f, 0.85f);

    // 1. Shallow-Water Hydrodynamics
    float dHdx = 0.5f * (s_H[sy][sx + 1u] - s_H[sy][sx - 1u]);
    float dHdy = 0.5f * (s_H[sy + 1u][sx] - s_H[sy - 1u][sx]);
    float vx = wb.vx[idx] * 0.88f - 0.16f * dHdx;
    float vy = wb.vy[idx] * 0.88f - 0.16f * dHdy;
    if (is_wall) { vx *= 0.12f; vy *= 0.12f; }
    vx = clampf(vx, -0.65f, 0.65f);
    vy = clampf(vy, -0.65f, 0.65f);
    wb.vx[idx] = vx;
    wb.vy[idx] = vy;

    float lap_H = (s_H[sy][sx - 1u] + s_H[sy][sx + 1u] + s_H[sy - 1u][sx] + s_H[sy + 1u][sx] - 4.0f * h_surf);
    float w_new = w + 0.12f * lap_H;
    if (mat == MAT_VENT) w_new += 0.015f;
    else if (ter < 0.8f) w_new += 0.002f;
    w_new -= 0.0012f * w_new;
    w_new = clampf(w_new, 0.02f, 4.0f);
    wb.water_next[idx] = w_new;

    // 2. Real Planetary Circadian Cycles + Three-Body (Trisolaran) Aperiodic Stable vs Chaotic Eras!
    // - Challenge 0/1: Smooth circadian day/night + periodic polar winters for early sensory-motor evolution.
    // - Challenge >= 2: 3 incommensurate solar orbital frequencies (Trisolaris!) produce genuinely aperiodic
    //   Stable Eras (~85%) punctuated by unpredictable Scorching Syzygies & Cryo-Eclipses (~15%), each preceded
    //   by a real 24-tick optical + barometric acoustic precursor -> selects predictive memory, shelters & torpor over brawn!
    bool polar_winter = false;
    bool chaot_heat = false;
    bool chaot_cold = false;
    float sun_phase = 0.62f;
    float tri_precursor_p = 0.0f;
    if (challenge_tier >= 2u) {
        float t_f = float(tick);
        float s3_now = 0.50f * fast_sin(t_f * 0.01570796f) +
                       0.30f * fast_sin(t_f * 0.00971430f + 1.1f) +
                       0.20f * fast_sin(t_f * 0.00599870f + 2.3f);
        float t_n = t_f + 24.0f;
        float s3_next = 0.50f * fast_sin(t_n * 0.01570796f) +
                        0.30f * fast_sin(t_n * 0.00971430f + 1.1f) +
                        0.20f * fast_sin(t_n * 0.00599870f + 2.3f);
        chaot_heat = (s3_now > 0.78f);
        chaot_cold = (s3_now < -0.78f);
        polar_winter = chaot_cold;
        bool pre_heat = (!chaot_heat && s3_next > 0.78f);
        bool pre_cold = (!chaot_cold && s3_next < -0.78f);
        if (chaot_heat) sun_phase = 1.32f;
        else if (chaot_cold) sun_phase = 0.04f;
        else sun_phase = clampf(0.36f + 0.58f * s3_now + (pre_heat ? 0.14f : (pre_cold ? -0.09f : 0.0f)), 0.04f, 1.14f);
        if ((pre_heat || pre_cold) && ((tick & 3u) == 0u) && ((gx & 7u) == 4u) && ((gy & 7u) == 4u)) {
            tri_precursor_p = pre_heat ? 0.95f : 0.72f; // Distinct Syzygy solar-crack vs Cryo-Eclipse glacial barometric precursor!
        }
    } else {
        polar_winter = (challenge_tier >= 1u) && ((tick % 720u) >= 580u);
        float sun_wave = fast_sin(float(tick) * 0.01570796f);
        sun_phase = polar_winter ? 0.04f
                                 : ((challenge_tier >= 1u) ? clampf(0.45f + 0.60f * sun_wave, 0.02f, 1.06f)
                                                           : (0.62f + 0.34f * sun_wave));
    }
    float east_h = s_ter[sy][sx + 1u];
    float shadow = clampf((east_h - ter) * 0.45f, 0.0f, 0.70f);
    float water_atten = clampf(1.0f - 0.18f * w, 0.35f, 1.0f);
    float prev_bio = clampf(wb.light[idx] - sun_phase, 0.0f, 1.2f) * 0.52f;
    float beacon_glow = (mat == MAT_CATALYST) ? 0.30f : 0.0f; // Stigmergic bioluminescent landmark trails!
    float light_val = clampf(sun_phase * (1.0f - shadow) * water_atten + prev_bio + beacon_glow, 0.04f, 2.0f);
    wb.light[idx] = light_val;

    // 3. Thermal Conduction + Trisolaran Syzygy/Eclipse + Geothermal Vents + Polymer Insulation!
    float tc = s_t[sy][sx];
    float lap_t = s_t[sy][sx - 1u] + s_t[sy][sx + 1u] + s_t[sy - 1u][sx] + s_t[sy + 1u][sx] - 4.0f * tc;
    float cond_k = is_wall ? 0.03f : (0.14f * (1.0f - 0.55f * shelter_f));
    // Deep water buffers scorching Syzygy heat; polymer shelters insulate against BOTH Syzygy heat & Cryo-Eclipse chill!
    float heat_depth_buf = clampf(w * 0.10f, 0.0f, 0.16f);
    float night_amb = chaot_heat ? clampf(0.70f - heat_depth_buf - shelter_f * 0.20f, 0.45f, 0.72f)
                                 : (polar_winter ? (0.22f + shelter_f * 0.14f)
                                                 : ((challenge_tier >= 1u) ? (0.24f + 0.24f * clampf(sun_phase, 0.0f, 1.1f)) : 0.38f));
    float cool_k = (chaot_heat ? 0.013f : 0.009f) * (1.0f - 0.65f * shelter_f);
    float t_new = tc + cond_k * lap_t + 0.005f * (light_val - 0.40f) - cool_k * (tc - night_amb);
    if (mat == MAT_VENT) t_new = 0.94f;
    wb.temp_next[idx] = clampf(t_new, 0.05f, 1.25f);

    // 4. 4-Species Advection-Diffusion-Reaction Chemistry (Polymer Walls Physically Block Chem D!)
    auto advect_diff = [&](float s[18][18], float diff_k, bool blocked_by_polymer) -> float {
        float c0 = s[sy][sx];
        float cL = (blocked_by_polymer && s_mat[sy][sx - 1u] == MAT_POLYMER) ? c0 : s[sy][sx - 1u];
        float cR = (blocked_by_polymer && s_mat[sy][sx + 1u] == MAT_POLYMER) ? c0 : s[sy][sx + 1u];
        float cU = (blocked_by_polymer && s_mat[sy - 1u][sx] == MAT_POLYMER) ? c0 : s[sy - 1u][sx];
        float cD = (blocked_by_polymer && s_mat[sy + 1u][sx] == MAT_POLYMER) ? c0 : s[sy + 1u][sx];
        float lap = cL + cR + cU + cD - 4.0f * c0;
        float adv_x = (vx >= 0.0f) ? (c0 - cL) : (cR - c0);
        float adv_y = (vy >= 0.0f) ? (c0 - cU) : (cD - c0);
        return c0 + diff_k * lap - 0.25f * (vx * adv_x + vy * adv_y);
    };

    float ca = advect_diff(s_ca, 0.08f, false);
    float cb = advect_diff(s_cb, 0.07f, true); // Built polymer dams physically trap drifting nutrient B (fish-weir physics!)
    float cc = advect_diff(s_cc, 0.05f, true); // Built polymer dams physically pool catalytic C upstream!
    float cd = advect_diff(s_cd, 0.18f, true); // Built polymer shelters physically shield from acid plumes!

    // 256x256 Multi-Biome Archipelago Ecology (Precomputed in shared s_reef[16] — ZERO per-thread divisions!):
    if (mat == MAT_ORE) ca += 0.036f;
    else if (mat == MAT_LOAM && w > 0.35f) ca += 0.0004f;

    bool in_reef = false;
    bool in_decoy = false;
    bool in_warm_reef = false;
    bool in_cool_reef = false;
    bool reef_beacon = false;
    bool reef_beacon_p2 = false;
    bool reef_ripe = false;
    bool reef_core = false;
    for (u32 rf = 0; rf < n_reefs; ++rf) {
        u32 pk = s_reef[rf];
        i32 dx = i32(gx) - i32(pk & 1023u); if (dx > HW) dx -= i32(W); if (dx < -HW) dx += i32(W);
        i32 dy = i32(gy) - i32((pk >> 10) & 1023u); if (dy > HH) dy -= i32(H); if (dy < -HH) dy += i32(H);
        i32 d2 = dx * dx + dy * dy;
        if (d2 <= 160) {
            u32 fl = pk >> 20;
            if (fl & 1u) reef_beacon = true;
            if (fl & 2u) reef_ripe = true;
            if (fl & 4u) in_warm_reef = true; else in_cool_reef = true;
            if (fl & 8u) in_reef = true; else in_decoy = true;
            if (fl & 16u) reef_beacon_p2 = true;
            if (d2 <= 9) reef_core = true;
        }
    }

    if (!polar_winter) {
        if (in_warm_reef && reef_beacon) {
            wb.light[idx] = clampf(wb.light[idx] + 0.90f, 0.05f, 2.0f);
            wb.temp_next[idx] = clampf(wb.temp_next[idx] + 0.075f, 0.05f, 1.25f);
        }
        if (in_cool_reef && reef_beacon) {
            ca += 0.065f; // Strong cool mineral sheen visual/chemical beacon
            wb.temp_next[idx] = clampf(wb.temp_next[idx] - 0.055f, 0.05f, 1.25f);
        }
    } else {
        // Dormant polar winter: reefs go dim & drop to low geothermal trickle (keeps active deep-cortex swimmers alive while rewarding nocturnal prey/pod hunters!)
        reef_beacon = false;
    }

    // Strictly barren open ocean (85% of planet) vs rich 16 archipelago reefs:
    // Blind 1-layer CHEM torpedoes starve crossing barren ocean, while sighted (EYE) + eared (EAR) + sonar (GLAND)
    // multi-layer hunters spot distant sunlit reefs, silent daylight prey, and active-sonar nocturnal prey!
    float win_mul = polar_winter ? 0.35f : 1.0f;
    float reef_pk = (reef_ripe ? 0.120f : 0.036f) * win_mul;
    float photo_k = (in_reef ? reef_pk : 0.0f) * (polar_winter ? 0.45f : light_val) * clampf(w, 0.0f, 1.5f);
    float d_ab = in_reef ? (photo_k * ca * (1.0f - clampf(cb, 0.0f, 1.0f))) : 0.0f;
    ca -= d_ab * 0.50f;
    cb += d_ab + (in_reef ? (reef_ripe ? 0.044f : 0.014f) * win_mul * (0.45f + light_val) : 0.0f);

    float cat_boost = (mat == MAT_CATALYST || mat == MAT_VENT)
                          ? (bloom_phase == 1u ? 0.078f : 0.050f)
                          : 0.0002f;
    float therm_gate = (t_new > 0.42f) ? (t_new - 0.32f) : 0.06f;
    float d_abc = cat_boost * therm_gate * ca * cb * (1.0f - clampf(cc, 0.0f, 1.0f));
    ca -= d_abc * 0.5f;
    cb -= d_abc * 0.5f;
    cc += d_abc * 1.50f + ((in_reef && reef_ripe) ? 0.0220f : 0.0f);

    // Corrosive mimic reef: blind/frozen foraging into the wrong seasonal reef causes severe toxic burn!
    if (in_decoy && (reef_ripe || reef_beacon)) cd += (reef_ripe ? 0.240f : 0.105f);
    if (bloom_phase == 0u && mat == MAT_VENT) cd += 0.024f;

    // 5. Multi-Cell Barometric Acoustic-Precursor Storms (Precomputed in shared s_storm[12]!)
    float storm_acoust = 0.0f;
    for (u32 sc = 0; sc < n_storms; ++sc) {
        u32 spk = s_storm[sc];
        if (spk == 0u) continue;
        u32 smode = spk >> 20;
        i32 dx = i32(gx) - i32(spk & 1023u); if (dx > HW) dx -= i32(W); if (dx < -HW) dx += i32(W);
        i32 dy = i32(gy) - i32((spk >> 10) & 1023u); if (dy > HH) dy -= i32(H); if (dy < -HH) dy += i32(H);
        i32 d2 = dx * dx + dy * dy;
        if (smode == 1u && d2 <= 110) {
            storm_acoust = 3.20f;
        } else if (smode == 2u && d2 <= 1220 && !is_wall) {
            float atten = 1.0f - shelter_f * 0.85f;
            cd += (challenge_tier >= 2u ? 0.210f : 0.175f) * atten;
            wb.temp_next[idx] = clampf(wb.temp_next[idx] - 0.095f * atten, 0.05f, 1.25f);
        }
    }

    ca = clampf(ca * 0.9975f, 0.0f, 2.0f);
    cb = clampf(cb * 0.9958f, 0.0f, 2.0f);
    cc = clampf(cc * 0.9965f, 0.0f, 2.0f);
    cd = clampf(cd * 0.9580f, 0.0f, 2.0f);

    wb.chem_a_next[idx] = ca;
    wb.chem_b_next[idx] = cb;
    wb.chem_c_next[idx] = cc;
    wb.chem_d_next[idx] = cd;

    // 6. 3-Band Long-Range 2D Damped Acoustic Wave Equations (25-35 tile propagation!):
    //    - acoust_p  : Physical low-band wave (storm precursors, swimming wakes, anvil strikes, bites)
    //    - acoust_v0 : Biological low-formant vocalization band
    //    - acoust_v1 : Biological high-formant vocalization band (proto-language spectral channel!)
    float ap = s_ap[sy][sx];
    float ap_prev = wb.acoust_prev[idx];
    float ap_next = 0.0f;
    float av0 = s_av0[sy][sx];
    float av0_prev = wb.acoust_v0_prev[idx];
    float av0_next = 0.0f;
    float av1 = s_av1[sy][sx];
    float av1_prev = wb.acoust_v1_prev[idx];
    float av1_next = 0.0f;
    float av2 = s_av2[sy][sx];
    float av2_prev = wb.acoust_v2_prev[idx];
    float av2_next = 0.0f;
    if (!is_wall) {
        bool wL = (s_ter[sy][sx - 1u] > 3.6f || s_mat[sy][sx - 1u] == MAT_POLYMER);
        bool wR = (s_ter[sy][sx + 1u] > 3.6f || s_mat[sy][sx + 1u] == MAT_POLYMER);
        bool wU = (s_ter[sy - 1u][sx] > 3.6f || s_mat[sy - 1u][sx] == MAT_POLYMER);
        bool wD = (s_ter[sy + 1u][sx] > 3.6f || s_mat[sy + 1u][sx] == MAT_POLYMER);
        float lap_p = (wL ? ap : s_ap[sy][sx - 1u]) + (wR ? ap : s_ap[sy][sx + 1u]) +
                      (wU ? ap : s_ap[sy - 1u][sx]) + (wD ? ap : s_ap[sy + 1u][sx]) - 4.0f * ap;
        ap_next = clampf((1.88f * ap - 0.91f * ap_prev) + 0.34f * lap_p + storm_acoust + tri_precursor_p, -2.5f, 2.5f);

        // Distinct 3-Band Compositional Acoustic Physics (ZERO hardcoded vocabulary!):
        // - acoust_v0: Long-range low-frequency bass wave (40+ tile colony rally)
        // - acoust_v1: High-frequency directional sonar/alarm chirp (echolocation + prey stun + apex repel!)
        // - acoust_v2: Mid-range resonant harmonic channel (social bonding, vocal mimicry & harmonic duet synchrony!)
        // Trisolaris-7 2-Syllable Nocturnal Reef Precursor (Warm: v0->v2, Cool: v2->v0 when sun_phase < 0.18):
        // Identical total acoustic energy in reversed temporal order -> selects prefrontal STF sequence decoding!
        bool night_acoust = (sun_phase < 0.18f);
        bool emit_v0_r = (in_warm_reef && !reef_beacon_p2) || (in_cool_reef && reef_beacon_p2);
        bool emit_v2_r = (in_cool_reef && !reef_beacon_p2) || (in_warm_reef && reef_beacon_p2);
        float reef_av0 = (night_acoust && reef_core && reef_beacon && emit_v0_r && ((tick & 3u) == 0u)) ? 0.48f : 0.0f;
        float reef_av2 = (night_acoust && reef_core && reef_beacon && emit_v2_r && ((tick & 3u) == 0u)) ? 0.48f : 0.0f;
        float lap_v0 = (wL ? av0 : s_av0[sy][sx - 1u]) + (wR ? av0 : s_av0[sy][sx + 1u]) +
                       (wU ? av0 : s_av0[sy - 1u][sx]) + (wD ? av0 : s_av0[sy + 1u][sx]) - 4.0f * av0;
        av0_next = clampf((1.93f * av0 - 0.94f * av0_prev) + 0.40f * lap_v0 + reef_av0, -2.5f, 2.5f);

        float lap_v1 = (wL ? av1 : s_av1[sy][sx - 1u]) + (wR ? av1 : s_av1[sy][sx + 1u]) +
                       (wU ? av1 : s_av1[sy - 1u][sx]) + (wD ? av1 : s_av1[sy + 1u][sx]) - 4.0f * av1;
        av1_next = clampf((1.87f * av1 - 0.90f * av1_prev) + 0.34f * lap_v1, -2.5f, 2.5f);

        float lap_v2 = (wL ? av2 : s_av2[sy][sx - 1u]) + (wR ? av2 : s_av2[sy][sx + 1u]) +
                       (wU ? av2 : s_av2[sy - 1u][sx]) + (wD ? av2 : s_av2[sy + 1u][sx]) - 4.0f * av2;
        av2_next = clampf((1.90f * av2 - 0.92f * av2_prev) + 0.37f * lap_v2 + reef_av2, -2.5f, 2.5f);
        if ((wL || wR || wU || wD) && fabsf_dev(av1) > 0.08f) {
            ap_next = clampf(ap_next + av1 * 0.45f, -2.5f, 2.5f); // Real acoustic wall echolocation reflection!
        }
    }
    wb.acoust_next[idx] = ap_next;
    wb.acoust_v0_next[idx] = av0_next;
    wb.acoust_v1_next[idx] = av1_next;
    wb.acoust_v2_next[idx] = av2_next;

    // Natural hydrological weathering of unmaintained polymer walls & transplanted catalyst silt!
    if (mat == MAT_POLYMER && ((tick + idx) & 255u) == 0u) {
        ter = clampf(ter - 0.22f, 0.3f, 4.0f);
        wb.terrain[idx] = ter;
        if (ter < 2.25f) wb.material[idx] = MAT_LOAM;
    } else if (mat == MAT_CATALYST && ((tick + idx) & 511u) == 0u && t_new < 0.65f) {
        wb.material[idx] = MAT_LOAM;
    }

    wb.cell_occ[idx] = 0u;
    wb.tile_occ_id[idx] = 0u;
    wb.tile_obj_id[idx] = 0u;
}

// ---------------------------------------------------------------------------
// Persistent Physical World Objects & Reactive Evasive Prey Step!
//   - Slots 0..383   : Sparse giant cooperative nutrient pods (OBJ_POD -> OBJ_FUEL, 36 ATP feast!)
//   - Slots 384..767 : Fast shy bioluminescent gliders (OBJ_PREY, 24 ATP — flees noisy approaches!)
//   - Slots 768..1023: Chipped carryable mineral anvil stones (OBJ_STONE!)
// ---------------------------------------------------------------------------
AX_FN void world_step_objects(WorldBuffers wb, const CreatureState *creatures, SimCounters *ctr,
                              u32 pop_n, u32 tick, u32 seed, u32 shift_chem) {
    u32 oid = cta_x() * ntid_x() + tid_x();
    if (oid >= wb.max_objects) return;
    u32 W = wb.w, H = wb.h, MX = wb.mask_x, MY = wb.mask_y, SY = wb.shift_y;
    float WF = float(W), HF = float(H);
    u32 n_reefs = min_u32(wb.num_reefs ? wb.num_reefs : 16u, 64u);
    u32 half_reefs = max_u32(1u, n_reefs >> 1);
    u32 s_pod = (wb.max_objects * 7u) >> 5;   // 224 @ 1024, 896 @ 4096
    u32 s_mega = (wb.max_objects * 11u) >> 5; // 352 @ 1024, 1408 @ 4096
    u32 s_prey = (wb.max_objects * 19u) >> 5; // 608 @ 1024, 2432 @ 4096
    u32 s_apex = (wb.max_objects * 21u) >> 5; // 672 @ 1024, 2688 @ 4096 (rest = chipped OBJ_STONE slots!)
    WorldObject &ob = wb.objects[oid];

    if (ob.kind == OBJ_NONE) {
        if (ob.carrier_id != INVALID_OBJ) ob.carrier_id = INVALID_OBJ; // Ensure freshly zeroed slots match CAS!
        if (oid < s_pod && ((tick + oid * 17u) & 63u) == 0u) {
            u32 r = mix(seed ^ (tick * 0x9e3779b9u) ^ (oid * 0x85ebca6bu));
            u32 bloom_phase = ((tick / 180u) ^ shift_chem) & 1u;
            u32 rf = ((r % half_reefs) * 2u) + (bloom_phase & 1u);
            u32 reef_epoch = (tick + rf * 59u) / 360u;
            u32 rx = mix(seed + reef_epoch * 1993u + rf * 4099u + (shift_chem ? 7717u : 0u)) & MX;
            u32 ry = mix(seed + reef_epoch * 3571u + rf * 7919u + (shift_chem ? 2203u : 0u)) & MY;
            u32 px = u32((i32(rx) + i32((r >> 8) % 19u) - 9) & i32(MX));
            u32 py = u32((i32(ry) + i32((r >> 16) % 19u) - 9) & i32(MY));
            u32 pidx = (py << SY) | px;
            if (wb.temp[pidx] < 0.56f && wb.material[pidx] != MAT_VENT && wb.material[pidx] != MAT_CATALYST) {
                ob.x = float(px) + 0.5f; ob.y = float(py) + 0.5f;
                ob.vx = 0.0f; ob.vy = 0.0f;
                ob.kind = OBJ_POD;
                ob.energy = 42.0f;
                ob.integrity = 1.45f;
                ob.carrier_id = INVALID_OBJ;
                ob.age = 0u;
                ob.flags = 0u;
            }
        } else if (oid >= s_pod && oid < s_mega && ((tick + oid * 19u) & 63u) == 0u) {
            u32 r = mix(seed ^ (tick * 0x517cc1b7u) ^ (oid * 0x9e3779b9u));
            u32 bloom_phase = ((tick / 180u) ^ shift_chem) & 1u;
            u32 rf = ((r % half_reefs) * 2u) + (bloom_phase & 1u);
            u32 reef_epoch = (tick + rf * 59u) / 360u;
            u32 rx = mix(seed + reef_epoch * 1993u + rf * 4099u + (shift_chem ? 7717u : 0u)) & MX;
            u32 ry = mix(seed + reef_epoch * 3571u + rf * 7919u + (shift_chem ? 2203u : 0u)) & MY;
            u32 px = u32((i32(rx) + i32((r >> 8) % 17u) - 8) & i32(MX));
            u32 py = u32((i32(ry) + i32((r >> 16) % 17u) - 8) & i32(MY));
            u32 pidx = (py << SY) | px;
            if (wb.temp[pidx] < 0.56f && wb.material[pidx] != MAT_VENT && wb.material[pidx] != MAT_CATALYST) {
                ob.x = float(px) + 0.5f; ob.y = float(py) + 0.5f;
                ob.vx = 0.0f; ob.vy = 0.0f;
                ob.kind = OBJ_MEGAPOD;
                ob.energy = 160.0f;
                ob.integrity = 2.60f;
                ob.carrier_id = INVALID_OBJ;
                ob.age = 0u;
                ob.flags = u16((((oid % 3u) & 3u) << 4) | ((oid & 1u) << 6));
            }
        } else if (oid >= s_mega && oid < s_prey && ((tick + oid * 23u) & 31u) == 0u) {
            u32 r = mix(seed ^ (tick * 0x85ebca6bu) ^ (oid * 0x27d4eb2du));
            u32 px = r & MX;
            u32 py = (r >> 10) & MY;
            u32 pidx = (py << SY) | px;
            if (wb.water[pidx] > 0.30f && wb.chem_d[pidx] < 0.15f && wb.terrain[pidx] < 2.6f) {
                ob.x = float(px) + 0.5f; ob.y = float(py) + 0.5f;
                ob.vx = 0.0f; ob.vy = 0.0f;
                ob.kind = OBJ_PREY;
                ob.energy = 52.0f;
                ob.integrity = 1.0f;
                ob.carrier_id = INVALID_OBJ;
                ob.age = 0u;
                ob.flags = u16((oid & 1u) ? 8u : 0u);
            }
        } else if (oid >= s_prey && oid < s_apex && ((tick + oid * 29u) & 127u) == 0u) {
            u32 r = mix(seed ^ (tick * 0x27d4eb2du) ^ (oid * 0x9e3779b9u));
            u32 px = r & MX;
            u32 py = (r >> 10) & MY;
            u32 pidx = (py << SY) | px;
            if (wb.water[pidx] > 0.25f && wb.terrain[pidx] < 2.5f && wb.material[pidx] != MAT_POLYMER) {
                ob.x = float(px) + 0.5f; ob.y = float(py) + 0.5f;
                ob.vx = 0.0f; ob.vy = 0.0f;
                ob.kind = OBJ_APEX;
                ob.energy = 88.0f;
                ob.integrity = 2.20f;
                ob.carrier_id = INVALID_OBJ;
                ob.age = 0u;
                ob.flags = 0u;
            }
        }
        return;
    }

    ++ob.age;
    // If carried by a living creature, follow carrier position; mark handled (flags |= 1)!
    if (ob.carrier_id != INVALID_OBJ) {
        if (ob.carrier_id < pop_n && creatures[ob.carrier_id].alive == 1u) {
            ob.x = creatures[ob.carrier_id].x;
            ob.y = creatures[ob.carrier_id].y;
            ob.vx = creatures[ob.carrier_id].vx;
            ob.vy = creatures[ob.carrier_id].vy;
            ob.flags |= 1u;
        } else {
            ob.carrier_id = INVALID_OBJ;
        }
    } else if (ob.kind == OBJ_PREY) {
        i32 ix = i32(ob.x + WF * 2.0f) & i32(MX);
        i32 iy = i32(ob.y + HF * 2.0f) & i32(MY);
        u32 idx = (u32(iy) << SY) | u32(ix);
        float tL = 0.0f, tR = 0.0f, tU = 0.0f, tD = 0.0f;
        #pragma unroll
        for (i32 d = 1; d <= 4; ++d) {
            float wt = (d == 1) ? 1.6f : (d == 2 ? 1.1f : (d == 3 ? 0.7f : 0.45f));
            u32 iL = (u32(iy) << SY) | u32((ix - d) & i32(MX));
            u32 iR = (u32(iy) << SY) | u32((ix + d) & i32(MX));
            u32 iU = (u32((iy - d) & i32(MY)) << SY) | u32(ix);
            u32 iD = (u32((iy + d) & i32(MY)) << SY) | u32(ix);
            tL += wt * (float(ld_nc_u32(wb.cell_occ + iL)) + fabsf_dev(ld_nc_f32(wb.acoust_p + iL)) * 2.8f);
            tR += wt * (float(ld_nc_u32(wb.cell_occ + iR)) + fabsf_dev(ld_nc_f32(wb.acoust_p + iR)) * 2.8f);
            tU += wt * (float(ld_nc_u32(wb.cell_occ + iU)) + fabsf_dev(ld_nc_f32(wb.acoust_p + iU)) * 2.8f);
            tD += wt * (float(ld_nc_u32(wb.cell_occ + iD)) + fabsf_dev(ld_nc_f32(wb.acoust_p + iD)) * 2.8f);
        }
        u32 fL = (u32(iy) << SY) | u32((ix - 3) & i32(MX));
        u32 fR = (u32(iy) << SY) | u32((ix + 3) & i32(MX));
        u32 fU = (u32((iy - 3) & i32(MY)) << SY) | u32(ix);
        u32 fD = (u32((iy + 3) & i32(MY)) << SY) | u32(ix);
        float fx = (tL - tR) * 0.80f + (ld_nc_f32(wb.chem_b + fR) - ld_nc_f32(wb.chem_b + fL)) * 0.22f;
        float fy = (tU - tD) * 0.80f + (ld_nc_f32(wb.chem_b + fD) - ld_nc_f32(wb.chem_b + fU)) * 0.22f;
        u32 rr = mix(seed ^ (tick * 31337u) ^ (oid * 997u));
        fx += (float(i32(rr & 255u) - 128) * 0.0014f);
        fy += (float(i32((rr >> 8) & 255u) - 128) * 0.0014f);
        ob.vx = clampf(ob.vx * 0.74f + fx + ld_nc_f32(wb.vx + idx) * 0.12f, -1.85f, 1.85f);
        ob.vy = clampf(ob.vy * 0.74f + fy + ld_nc_f32(wb.vy + idx) * 0.12f, -1.85f, 1.85f);
        // Cryptic Silent-Day / Active-Sonar-Night Prey Physics (permanently co-selects EYE + EAR + GLAND!):
        // - By Day (light >= 0.22): prey is acoustically stealthy (laminar glide, NO sonar echo, NO v0/v2 calming)
        //   -> ONLY TK_EYE spots sunlit hue & tracks edible vs venomous Batesian morphs (+58 ATP)!
        // - By Night/Polar Winter (light < 0.22): invisible to eyes, UNTIL pinged by active v1 syrinx sonar
        //   (TK_GLAND) -> crisp echo + morph harmonic (v0/v2) audible to TK_EAR + odontocete stun!
        float v0_here = fabsf_dev(ld_nc_f32(wb.acoust_v0 + idx));
        float v1_ping = fabsf_dev(ld_nc_f32(wb.acoust_v1 + idx));
        float v2_here = fabsf_dev(ld_nc_f32(wb.acoust_v2 + idx));
        float loc_light = ld_nc_f32(wb.light + idx);
        bool is_warm_p = ((ob.flags & 8u) == 0u);
        if (loc_light < 0.22f) {
            // Nocturnal referential acoustic morph-resonance: Warm prey calms to v0, Cool calms to v2!
            if ((is_warm_p && v0_here > 0.04f) || (!is_warm_p && v2_here > 0.04f)) {
                ob.vx *= 0.55f; ob.vy *= 0.55f;
            }
            if (v1_ping > 0.03f) {
                ob.vx *= 0.20f; ob.vy *= 0.20f; // Nocturnal odontocete active-sonar stun + morph echo!
                atomic_add_f32(&wb.acoust_p[idx], 1.40f * clampf(v1_ping * 4.0f, 0.35f, 1.2f));
                if (is_warm_p) atomic_add_f32(&wb.acoust_v0[idx], 0.55f);
                else atomic_add_f32(&wb.acoust_v2[idx], 0.55f);
            } else if ((tick & 3u) == 0u) {
                // Authentic nocturnal stridulation/cricket mating pulses (audible 16-22 tiles away to TK_EAR even solo!)
                atomic_add_f32(&wb.acoust_p[idx], 0.45f);
                if (is_warm_p) atomic_add_f32(&wb.acoust_v0[idx], 0.34f);
                else atomic_add_f32(&wb.acoust_v2[idx], 0.34f);
            }
        } else if (v1_ping > 0.03f) {
            // Diurnal skittish visual prey: loud daytime sonar startles it AWAY without leaking its color morph!
            ob.vx = clampf(ob.vx * 1.45f, -1.95f, 1.95f);
            ob.vy = clampf(ob.vy * 1.45f, -1.95f, 1.95f);
        }
        float nx = ob.x + ob.vx;
        float ny = ob.y + ob.vy;
        if (nx < 0.0f) nx += WF;
        if (nx >= WF) nx -= WF;
        if (ny < 0.0f) ny += HF;
        if (ny >= HF) ny -= HF;
        u32 nidx = (u32(i32(ny) & i32(MY)) << SY) | u32(i32(nx) & i32(MX));
        if (wb.terrain[nidx] > 3.2f || wb.material[nidx] == MAT_POLYMER) {
            ob.vx = -ob.vx * 0.5f; ob.vy = -ob.vy * 0.5f; // Cornered against a built polymer weir or cliff!
        } else {
            ob.x = nx; ob.y = ny;
        }
    } else if (ob.kind == OBJ_APEX) {
        // Autonomous Roaming Apex Predator with Real Bioacoustic Eavesdropping (Trisolaris-6):
        // Hunts lone swimmers AND mindless open-water v0/v2 chatter spammers, while fleeing sharp v1 alarm choruses & polymer shelters!
        i32 ix = i32(ob.x + WF * 2.0f) & i32(MX);
        i32 iy = i32(ob.y + HF * 2.0f) & i32(MY);
        u32 idx = (u32(iy) << SY) | u32(ix);
        float pL = 0.0f, pR = 0.0f, pU = 0.0f, pD = 0.0f;
        float sL = 0.0f, sR = 0.0f, sU = 0.0f, sD = 0.0f;
        u32 crowd = ld_nc_u32(wb.cell_occ + idx);
        #pragma unroll
        for (i32 d = 1; d <= 6; ++d) {
            float wt = 1.3f / float(d);
            u32 iL = (u32(iy) << SY) | u32((ix - d) & i32(MX));
            u32 iR = (u32(iy) << SY) | u32((ix + d) & i32(MX));
            u32 iU = (u32((iy - d) & i32(MY)) << SY) | u32(ix);
            u32 iD = (u32((iy + d) & i32(MY)) << SY) | u32(ix);
            u32 oL = ld_nc_u32(wb.cell_occ + iL), oR = ld_nc_u32(wb.cell_occ + iR);
            u32 oU = ld_nc_u32(wb.cell_occ + iU), oD = ld_nc_u32(wb.cell_occ + iD);
            if (d <= 2) crowd += oL + oR + oU + oD;
            // Predator acoustic eavesdropping on loud open-water v0 & v2 chatter punishes constant noise spam -> selects referential MI!
            pL += wt * (float(oL) + fabsf_dev(ld_nc_f32(wb.acoust_p + iL)) * 0.55f + (fabsf_dev(ld_nc_f32(wb.acoust_v0 + iL)) + fabsf_dev(ld_nc_f32(wb.acoust_v2 + iL))) * 0.58f);
            pR += wt * (float(oR) + fabsf_dev(ld_nc_f32(wb.acoust_p + iR)) * 0.55f + (fabsf_dev(ld_nc_f32(wb.acoust_v0 + iR)) + fabsf_dev(ld_nc_f32(wb.acoust_v2 + iR))) * 0.58f);
            pU += wt * (float(oU) + fabsf_dev(ld_nc_f32(wb.acoust_p + iU)) * 0.55f + (fabsf_dev(ld_nc_f32(wb.acoust_v0 + iU)) + fabsf_dev(ld_nc_f32(wb.acoust_v2 + iU))) * 0.58f);
            pD += wt * (float(oD) + fabsf_dev(ld_nc_f32(wb.acoust_p + iD)) * 0.55f + (fabsf_dev(ld_nc_f32(wb.acoust_v0 + iD)) + fabsf_dev(ld_nc_f32(wb.acoust_v2 + iD))) * 0.58f);
            if (d <= 4) {
                sL += wt * fabsf_dev(ld_nc_f32(wb.acoust_v1 + iL));
                sR += wt * fabsf_dev(ld_nc_f32(wb.acoust_v1 + iR));
                sU += wt * fabsf_dev(ld_nc_f32(wb.acoust_v1 + iU));
                sD += wt * fabsf_dev(ld_nc_f32(wb.acoust_v1 + iD));
            }
        }
        float dir_sign = (crowd >= 3u || ob.integrity < 0.70f) ? -0.65f : 0.48f;
        float fx = (pR - pL) * dir_sign + (sL - sR) * 2.40f; // High-freq v1 alarm strongly startles predator AWAY!
        float fy = (pD - pU) * dir_sign + (sU - sD) * 2.40f;
        u32 rr = mix(seed ^ (tick * 19937u) ^ (oid * 4099u));
        fx += float(i32(rr & 255u) - 128) * 0.0022f;
        fy += float(i32((rr >> 8) & 255u) - 128) * 0.0022f;
        float apex_cap = (wb.light[idx] < 0.28f) ? 1.32f : 0.95f;
        ob.vx = clampf(ob.vx * 0.80f + fx, -apex_cap, apex_cap);
        ob.vy = clampf(ob.vy * 0.80f + fy, -apex_cap, apex_cap);
        if ((tick & 1u) == 0u) atomic_add_f32(&wb.acoust_p[idx], 0.46f);
        float nx = ob.x + ob.vx;
        float ny = ob.y + ob.vy;
        if (nx < 0.0f) nx += WF;
        if (nx >= WF) nx -= WF;
        if (ny < 0.0f) ny += HF;
        if (ny >= HF) ny -= HF;
        u32 nidx = (u32(i32(ny) & i32(MY)) << SY) | u32(i32(nx) & i32(MX));
        if (wb.terrain[nidx] > 3.1f || wb.material[nidx] == MAT_POLYMER) {
            ob.vx = -ob.vx * 0.65f; ob.vy = -ob.vy * 0.65f;
        } else {
            ob.x = nx; ob.y = ny;
        }
    } else {
        i32 ix = i32(ob.x + WF * 2.0f) & i32(MX);
        i32 iy = i32(ob.y + HF * 2.0f) & i32(MY);
        u32 idx = (u32(iy) << SY) | u32(ix);
        ob.vx = ob.vx * 0.75f + ld_nc_f32(wb.vx + idx) * 0.12f;
        ob.vy = ob.vy * 0.75f + ld_nc_f32(wb.vy + idx) * 0.12f;
        if (ob.kind == OBJ_MEGAPOD) {
            float vp = fabsf_dev(ld_nc_f32(wb.acoust_v1 + idx));
            bool song_tick = (wb.light[idx] < 0.25f && (tick & 3u) == 0u);
            if (vp > 0.03f || song_tick) {
                u32 pa = (ob.flags >> 4) & 3u;
                u32 pb = (pa + 1u + ((ob.flags >> 6) & 1u)) % 3u;
                u32 emit_b = ((tick & 4u) == 0u) ? pa : pb;
                float eb = (vp > 0.03f) ? (0.68f * clampf(vp * 4.0f, 0.35f, 1.2f)) : 0.36f;
                if (emit_b == 0u) atomic_add_f32(&wb.acoust_v0[idx], eb);
                else if (emit_b == 1u) atomic_add_f32(&wb.acoust_v1[idx], eb);
                else atomic_add_f32(&wb.acoust_v2[idx], eb);
            }
        }
        ob.x += ob.vx; ob.y += ob.vy;
        if (ob.x < 0.0f) ob.x += WF;
        if (ob.x >= WF) ob.x -= WF;
        if (ob.y < 0.0f) ob.y += HF;
        if (ob.y >= HF) ob.y -= HF;
    }

    i32 ix = i32(ob.x + WF * 2.0f) & i32(MX);
    i32 iy = i32(ob.y + HF * 2.0f) & i32(MY);
    u32 idx = (u32(iy) << SY) | u32(ix);

    // Physical Cooking & Cracking: heat at geothermal vent softens/cracks OBJ_POD & OBJ_MEGAPOD -> OBJ_FUEL!
    // STRICT HONESTY: ONLY count pods_cracked when a creature ACTUALLY handled/transported/struck the pod (flags & 1)!
    if (ob.kind == OBJ_POD || ob.kind == OBJ_MEGAPOD) {
        if (wb.temp[idx] > 0.64f) ob.integrity -= (ob.kind == OBJ_MEGAPOD ? 0.09f : 0.11f);
        if (ob.integrity <= 0.0f) {
            ob.kind = OBJ_FUEL;
            ob.integrity = 0.0f;
            ob.age = 0u;
            atomic_add_f32(&wb.light[idx], 0.90f);
            atomic_add_f32(&wb.acoust_p[idx], 0.65f);
            if (ctr && (ob.flags & 1u)) atomic_add_u32(&ctr->total_pods_cracked, 1u);
        }
    }

    if (ob.kind == OBJ_APEX && ob.integrity <= 0.0f) {
        // Slain Apex Predator turns into a 72 ATP communal trophy feast!
        ob.kind = OBJ_FUEL;
        ob.integrity = 0.0f;
        ob.age = 0u;
        atomic_add_f32(&wb.light[idx], 1.15f);
        atomic_add_f32(&wb.acoust_p[idx], 0.90f);
        if (ctr) atomic_add_u32(&ctr->total_apex_kills, 1u);
    }

    // Stage-2 Compositional Tool Tempering: carrying/placing a raw OBJ_STONE in geothermal heat (>0.66)
    // or catalytic mineral silt tempers it into a high-leverage composite spear/blade (flags |= 2u)!
    if (ob.kind == OBJ_STONE && (ob.flags & 2u) == 0u &&
        (wb.temp[idx] > 0.66f || wb.material[idx] == MAT_CATALYST)) {
        ob.flags |= 2u;
        atomic_add_f32(&wb.light[idx], 0.95f);
        atomic_add_f32(&wb.acoust_p[idx], 0.50f);
        if (ctr) atomic_add_u32(&ctr->total_tempered_tools, 1u);
    }

    // Insulated Polymer Granary Food Caching: OBJ_FUEL cached next to a built MAT_POLYMER wall preserves 3x longer!
    bool cached_fuel = false;
    if (ob.kind == OBJ_FUEL && ob.carrier_id == INVALID_OBJ) {
        u32 tx = u32(ix), ty = u32(iy);
        cached_fuel = (wb.material[(ty << SY) | ((tx + 1u) & MX)] == MAT_POLYMER) ||
                      (wb.material[(ty << SY) | ((tx + W - 1u) & MX)] == MAT_POLYMER) ||
                      (wb.material[(((ty + 1u) & MY) << SY) | tx] == MAT_POLYMER) ||
                      (wb.material[(((ty + H - 1u) & MY) << SY) | tx] == MAT_POLYMER);
        if (cached_fuel) ob.flags |= 4u;
    }

    // Natural decay of old unharvested objects -> returns nutrients to the world!
    u32 fuel_life = cached_fuel ? 1950u : 650u;
    if (((ob.kind == OBJ_POD || ob.kind == OBJ_MEGAPOD) && ob.age > 1200u) || (ob.kind == OBJ_FUEL && ob.age > fuel_life) ||
        (ob.kind == OBJ_PREY && (ob.age > 1200u || wb.chem_d[idx] > 0.65f)) ||
        (ob.kind == OBJ_APEX && ob.age > 1800u) ||
        (ob.kind == OBJ_STONE && ob.age > 3600u)) {
        if (ob.kind == OBJ_FUEL || ob.kind == OBJ_PREY) atomic_add_f32(&wb.chem_c[idx], 0.30f);
        ob.kind = OBJ_NONE;
        ob.carrier_id = INVALID_OBJ;
        ob.flags = 0u;
        return;
    }

    if (ob.carrier_id == INVALID_OBJ) {
        wb.tile_obj_id[idx] = oid + 1u;
        // Real Physical Object Echolocation: high-freq v1 sonar chirps reflect crisply off solid objects & nocturnal prey!
        // Diurnal laminar prey gliders absorb daytime sonar (strictly visual-only by day, active-sonar by night!)
        float v1_hit = fabsf_dev(ld_nc_f32(wb.acoust_v1 + idx));
        bool stealth_day_prey = (ob.kind == OBJ_PREY && wb.light[idx] >= 0.22f);
        if (v1_hit > 0.03f && !stealth_day_prey) {
            atomic_add_f32(&wb.acoust_p[idx], 1.35f * v1_hit);
            if (ctr) atomic_add_u32(&ctr->total_echo_hits, 1u);
        }
        // Objects reflect ambient sunlight by day (virtually invisible in pitch-black night so EARS + v1 sonar own night!)
        float day_scale = (wb.light[idx] < 0.12f) ? 0.0f : clampf(wb.light[idx] * 0.9f, 0.0f, 1.0f);
        if (ob.kind == OBJ_FUEL) wb.light[idx] = clampf(wb.light[idx] + 0.34f * day_scale, 0.05f, 2.0f);
        else if (ob.kind == OBJ_PREY) wb.light[idx] = clampf(wb.light[idx] + 0.50f * day_scale, 0.05f, 2.0f);
        else if (ob.kind == OBJ_MEGAPOD) wb.light[idx] = clampf(wb.light[idx] + 0.44f * day_scale, 0.05f, 2.0f);
        else if (ob.kind == OBJ_STONE && (ob.flags & 2u)) wb.light[idx] = clampf(wb.light[idx] + 0.22f * day_scale, 0.05f, 2.0f);
    }
}

} // namespace axion::genesis::gpu
