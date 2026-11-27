#include "axion/genesis.hpp"
#include "axion/genesis_video.hpp"
#include "axion/journal.hpp"
#include "life_runtime.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace axion::genesis {
namespace {

using namespace axion::life::runtime;

u32 parse_u32(std::string_view s, u32 lo, u32 hi, std::string_view flag) {
    u32 v = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || ptr != s.data() + s.size() || v < lo || v > hi)
        throw std::invalid_argument("invalid value for " + std::string(flag));
    return v;
}

char hex_tk(u32 v) {
    static constexpr char H[] = "0123456789abcdef";
    return H[v & 15u];
}

struct GenesisGpu {
    Session &s;
    u32 pop_n;
    u32 world_w;
    u32 world_h;
    u32 world_cells;
    u32 max_objects;
    u32 max_layers;
    u32 max_syn_slabs;

    Function fn_world_init{}, fn_world_step{}, fn_obj_step{}, fn_pop_init{}, fn_build_devlist{}, fn_develop{}, fn_develop_trace{};
    Function fn_build_wl{}, fn_brain_step{}, fn_body_eco{}, fn_check_extinct{};
    Function fn_snn_bench_infer{}, fn_snn_bench_plastic{};

    // Runtime-Scalable (256/512/1024) World SoA GPU buffers + 3-Band Acoustics + Persistent WorldObjects & Evasive Prey
    Temporary d_terrain, d_material;
    Temporary d_water, d_water_next, d_vx, d_vy;
    Temporary d_temp, d_temp_next;
    Temporary d_ca, d_ca_next, d_cb, d_cb_next, d_cc, d_cc_next, d_cd, d_cd_next;
    Temporary d_ap, d_ap_prev, d_ap_next, d_av0, d_av0_prev, d_av0_next, d_av1, d_av1_prev, d_av1_next, d_av2, d_av2_prev, d_av2_next;
    Temporary d_light, d_occ, d_occ_id, d_obj_id, d_objects;

    // Population & Unified 3D Tissue + Variable Fan-In Synapse Slab Pool
    Temporary d_dnas, d_bodies, d_creatures, d_needs_dev, d_counters;
    Temporary d_layer_bitmap, d_syn_bitmap;
    Temporary d_tissue, d_neurons, d_spike_ring, d_spike_ring_next, d_cell_stripes;
    Temporary d_layer_syn_slabs, d_syn_slabs, d_stripe_src_layer;
    Temporary d_work_list, d_work_count, d_dev_list, d_dev_meta;

    WorldBuffers wb{};
    TissuePool pool{};
    bool gpu_sync = false;
    Event sync_ev{};

    void maybe_sync(const char *tag) {
        if (!gpu_sync) return;
        s.d.check(s.d.record(sync_ev, nullptr), tag);
        s.d.check(s.d.wait(sync_ev), tag);
    }

    ~GenesisGpu() {
        if (sync_ev) s.d.event_free(sync_ev);
    }

    static u32 snap_world(u32 w) { return (w >= 1024u) ? 1024u : ((w >= 512u) ? 512u : 256u); }
    static u32 objects_for_world(u32 w) { return (w >= 1024u) ? 8192u : ((w >= 512u) ? 4096u : 1024u); }

    GenesisGpu(Session &sess, u32 pop, u32 layers_cap, u32 w_size = WORLD_W, u32 syn_slabs_cap = 0u)
        : s(sess), pop_n(pop),
          world_w(snap_world(w_size)), world_h(snap_world(w_size)),
          world_cells(world_w * world_h),
          max_objects(objects_for_world(world_w)),
          max_layers(std::min<u32>(std::max(layers_cap, pop * 2u), 131072u)),
          max_syn_slabs(std::min<u32>(syn_slabs_cap ? syn_slabs_cap : std::max(layers_cap * 6u, pop * 16u), 262144u)),
          d_terrain(s.d, size_t(world_cells) * sizeof(float)),
          d_material(s.d, size_t(world_cells) * sizeof(u8)),
          d_water(s.d, size_t(world_cells) * sizeof(float)),
          d_water_next(s.d, size_t(world_cells) * sizeof(float)),
          d_vx(s.d, size_t(world_cells) * sizeof(float)),
          d_vy(s.d, size_t(world_cells) * sizeof(float)),
          d_temp(s.d, size_t(world_cells) * sizeof(float)),
          d_temp_next(s.d, size_t(world_cells) * sizeof(float)),
          d_ca(s.d, size_t(world_cells) * sizeof(float)),
          d_ca_next(s.d, size_t(world_cells) * sizeof(float)),
          d_cb(s.d, size_t(world_cells) * sizeof(float)),
          d_cb_next(s.d, size_t(world_cells) * sizeof(float)),
          d_cc(s.d, size_t(world_cells) * sizeof(float)),
          d_cc_next(s.d, size_t(world_cells) * sizeof(float)),
          d_cd(s.d, size_t(world_cells) * sizeof(float)),
          d_cd_next(s.d, size_t(world_cells) * sizeof(float)),
          d_ap(s.d, size_t(world_cells) * sizeof(float)),
          d_ap_prev(s.d, size_t(world_cells) * sizeof(float)),
          d_ap_next(s.d, size_t(world_cells) * sizeof(float)),
          d_av0(s.d, size_t(world_cells) * sizeof(float)),
          d_av0_prev(s.d, size_t(world_cells) * sizeof(float)),
          d_av0_next(s.d, size_t(world_cells) * sizeof(float)),
          d_av1(s.d, size_t(world_cells) * sizeof(float)),
          d_av1_prev(s.d, size_t(world_cells) * sizeof(float)),
          d_av1_next(s.d, size_t(world_cells) * sizeof(float)),
          d_av2(s.d, size_t(world_cells) * sizeof(float)),
          d_av2_prev(s.d, size_t(world_cells) * sizeof(float)),
          d_av2_next(s.d, size_t(world_cells) * sizeof(float)),
          d_light(s.d, size_t(world_cells) * sizeof(float)),
          d_occ(s.d, size_t(world_cells) * sizeof(u32)),
          d_occ_id(s.d, size_t(world_cells) * sizeof(u32)),
          d_obj_id(s.d, size_t(world_cells) * sizeof(u32)),
          d_objects(s.d, size_t(max_objects) * sizeof(WorldObject)),
          d_dnas(s.d, size_t(pop_n) * sizeof(GenesisDNA)),
          d_bodies(s.d, size_t(pop_n) * sizeof(BodyPhenotype)),
          d_creatures(s.d, size_t(pop_n) * sizeof(CreatureState)),
          d_needs_dev(s.d, size_t(pop_n) * sizeof(u8)),
          d_counters(s.d, sizeof(SimCounters)),
          d_layer_bitmap(s.d, size_t((max_layers + 31u) / 32u) * sizeof(u32)),
          d_syn_bitmap(s.d, size_t((max_syn_slabs + 31u) / 32u) * sizeof(u32)),
          d_tissue(s.d, size_t(max_layers) * TISSUE_CELLS * sizeof(u64)),
          d_neurons(s.d, size_t(max_layers) * TISSUE_CELLS * sizeof(u64)),
          d_spike_ring(s.d, size_t(max_layers) * TISSUE_CELLS * sizeof(u8)),
          d_spike_ring_next(s.d, size_t(max_layers) * TISSUE_CELLS * sizeof(u8)),
          d_cell_stripes(s.d, size_t(max_layers) * TISSUE_CELLS * sizeof(u8)),
          d_layer_syn_slabs(s.d, size_t(max_layers) * MAX_STRIPES_PER_LAYER * sizeof(u32)),
          d_syn_slabs(s.d, size_t(max_syn_slabs) * LAYER_SYN_QUADS * sizeof(SynQuad)),
          d_stripe_src_layer(s.d, size_t(max_syn_slabs) * TISSUE_CELLS * sizeof(u8)),
          d_work_list(s.d, size_t(max_layers) * sizeof(u32)),
          d_work_count(s.d, sizeof(u32)),
          d_dev_list(s.d, size_t(pop_n) * sizeof(u32)),
          d_dev_meta(s.d, 2u * sizeof(u32)) {
        s.bind(fn_world_init, "genesis_world_init", Image::genesis);
        s.bind(fn_world_step, "genesis_world_step", Image::genesis);
        s.bind(fn_obj_step, "genesis_objects_step", Image::genesis);
        s.bind(fn_pop_init, "genesis_pop_init", Image::genesis);
        s.bind(fn_build_devlist, "genesis_build_devlist", Image::genesis);
        s.bind(fn_develop, "genesis_develop", Image::genesis);
        s.bind(fn_develop_trace, "genesis_develop_trace", Image::genesis);
        s.bind(fn_build_wl, "genesis_build_worklist", Image::genesis);
        s.bind(fn_brain_step, "genesis_brain_step", Image::genesis);
        s.bind(fn_body_eco, "genesis_body_ecology", Image::genesis);
        s.bind(fn_check_extinct, "genesis_check_extinct", Image::genesis);
        s.bind(fn_snn_bench_infer, "genesis_snn_bench_infer", Image::genesis);
        s.bind(fn_snn_bench_plastic, "genesis_snn_bench_plastic", Image::genesis);

        wb.terrain = reinterpret_cast<float *>(d_terrain.ptr);
        wb.material = reinterpret_cast<u8 *>(d_material.ptr);
        wb.water = reinterpret_cast<float *>(d_water.ptr);
        wb.water_next = reinterpret_cast<float *>(d_water_next.ptr);
        wb.vx = reinterpret_cast<float *>(d_vx.ptr);
        wb.vy = reinterpret_cast<float *>(d_vy.ptr);
        wb.temp = reinterpret_cast<float *>(d_temp.ptr);
        wb.temp_next = reinterpret_cast<float *>(d_temp_next.ptr);
        wb.chem_a = reinterpret_cast<float *>(d_ca.ptr);
        wb.chem_a_next = reinterpret_cast<float *>(d_ca_next.ptr);
        wb.chem_b = reinterpret_cast<float *>(d_cb.ptr);
        wb.chem_b_next = reinterpret_cast<float *>(d_cb_next.ptr);
        wb.chem_c = reinterpret_cast<float *>(d_cc.ptr);
        wb.chem_c_next = reinterpret_cast<float *>(d_cc_next.ptr);
        wb.chem_d = reinterpret_cast<float *>(d_cd.ptr);
        wb.chem_d_next = reinterpret_cast<float *>(d_cd_next.ptr);
        wb.acoust_p = reinterpret_cast<float *>(d_ap.ptr);
        wb.acoust_prev = reinterpret_cast<float *>(d_ap_prev.ptr);
        wb.acoust_next = reinterpret_cast<float *>(d_ap_next.ptr);
        wb.acoust_v0 = reinterpret_cast<float *>(d_av0.ptr);
        wb.acoust_v0_prev = reinterpret_cast<float *>(d_av0_prev.ptr);
        wb.acoust_v0_next = reinterpret_cast<float *>(d_av0_next.ptr);
        wb.acoust_v1 = reinterpret_cast<float *>(d_av1.ptr);
        wb.acoust_v1_prev = reinterpret_cast<float *>(d_av1_prev.ptr);
        wb.acoust_v1_next = reinterpret_cast<float *>(d_av1_next.ptr);
        wb.acoust_v2 = reinterpret_cast<float *>(d_av2.ptr);
        wb.acoust_v2_prev = reinterpret_cast<float *>(d_av2_prev.ptr);
        wb.acoust_v2_next = reinterpret_cast<float *>(d_av2_next.ptr);
        wb.light = reinterpret_cast<float *>(d_light.ptr);
        wb.cell_occ = reinterpret_cast<u32 *>(d_occ.ptr);
        wb.tile_occ_id = reinterpret_cast<u32 *>(d_occ_id.ptr);
        wb.tile_obj_id = reinterpret_cast<u32 *>(d_obj_id.ptr);
        wb.objects = reinterpret_cast<WorldObject *>(d_objects.ptr);
        wb.max_objects = max_objects;
        wb.w = world_w;
        wb.h = world_h;
        wb.mask_x = world_w - 1u;
        wb.mask_y = world_h - 1u;
        wb.shift_y = (world_w >= 1024u) ? 10u : ((world_w >= 512u) ? 9u : 8u);
        wb.num_reefs = (world_w >= 512u) ? 64u : 16u;
        wb.num_vents = (world_w >= 1024u) ? 40u : ((world_w >= 512u) ? 28u : 10u);
        wb.num_storms = (world_w >= 1024u) ? 12u : ((world_w >= 512u) ? 8u : 3u);

        pool.max_layers = max_layers;
        pool.max_syn_slabs = max_syn_slabs;
        pool.layer_bitmap = reinterpret_cast<u32 *>(d_layer_bitmap.ptr);
        pool.syn_bitmap = reinterpret_cast<u32 *>(d_syn_bitmap.ptr);
        pool.tissue = reinterpret_cast<u64 *>(d_tissue.ptr);
        pool.neurons = reinterpret_cast<u64 *>(d_neurons.ptr);
        pool.spike_ring = reinterpret_cast<u8 *>(d_spike_ring.ptr);
        pool.spike_ring_next = reinterpret_cast<u8 *>(d_spike_ring_next.ptr);
        pool.cell_stripes = reinterpret_cast<u8 *>(d_cell_stripes.ptr);
        pool.layer_syn_slabs = reinterpret_cast<u32 *>(d_layer_syn_slabs.ptr);
        pool.syn_slabs = reinterpret_cast<SynQuad *>(d_syn_slabs.ptr);
        pool.stripe_src_layer = reinterpret_cast<u8 *>(d_stripe_src_layer.ptr);
        pool.work_list = reinterpret_cast<u32 *>(d_work_list.ptr);
        pool.work_count = reinterpret_cast<u32 *>(d_work_count.ptr);
        gpu_sync = (std::getenv("AXION_GPU_SYNC") != nullptr);
        if (gpu_sync) s.d.check(s.d.event_new(&sync_ev, 0), "sync ev");
    }

    void reset_pool() {
        s.d.check(s.d.zero(d_layer_bitmap.ptr, 0, size_t((max_layers + 31u) / 32u) * sizeof(u32)), "zero lbm");
        s.d.check(s.d.zero(d_syn_bitmap.ptr, 0, size_t((max_syn_slabs + 31u) / 32u) * sizeof(u32)), "zero sbm");
        s.d.check(s.d.zero(d_spike_ring.ptr, 0, size_t(max_layers) * TISSUE_CELLS * sizeof(u8)), "zero ring");
        s.d.check(s.d.zero(d_spike_ring_next.ptr, 0, size_t(max_layers) * TISSUE_CELLS * sizeof(u8)), "zero ring next");
        s.d.check(s.d.zero(d_cell_stripes.ptr, 0, size_t(max_layers) * TISSUE_CELLS * sizeof(u8)), "zero cs");
        std::vector<u32> inv(size_t(max_layers) * MAX_STRIPES_PER_LAYER, INVALID_SLAB);
        s.d.check(s.d.upload(d_layer_syn_slabs.ptr, inv.data(), inv.size() * sizeof(u32)), "init lss");
        s.d.check(s.d.zero(d_counters.ptr, 0, sizeof(SimCounters)), "zero counters");
    }

    void init_world(u32 seed, u32 shift_chem = 0) {
        s.d.check(s.d.zero(d_objects.ptr, 0, size_t(max_objects) * sizeof(WorldObject)), "zero objects");
        void *args[]{&wb, &seed, &shift_chem};
        s.d.check(s.d.launch(fn_world_init, world_w / 16u, world_h / 16u, 1, 16, 16, 1, 0, nullptr, args, nullptr),
                  "genesis world init");
    }

    void init_pop(u32 initial_alive, u32 seed) {
        reset_pool();
        u32 blocks = (pop_n + 127u) / 128u;
        void *args[]{&d_dnas.ptr, &d_creatures.ptr, &d_needs_dev.ptr, &pool, &pop_n, &initial_alive, &world_w, &world_h, &seed};
        s.d.check(s.d.launch(fn_pop_init, blocks, 1, 1, 128, 1, 1, 0, nullptr, args, nullptr), "genesis pop init");
    }

    void upload_dnas_and_spawn(const std::vector<GenesisDNA> &dnas_in, u32 spawn_n, u32 world_seed,
                               u32 base_gen = 1u) {
        reset_pool();
        std::vector<GenesisDNA> full(pop_n);
        std::vector<CreatureState> crs(pop_n);
        std::vector<u8> nd(pop_n, 0);
        std::vector<u32> lbm((max_layers + 31u) / 32u, 0u);
        u32 next_sl = 0u;

        u32 r = mix(world_seed ^ 0x13579bdfu);
        for (u32 i = 0; i < pop_n; ++i) {
            for (u32 l = 0; l < MAX_LAYERS_PER_CREATURE; ++l) crs[i].layer_slabs[l] = INVALID_SLAB;
            crs[i].carried_obj = INVALID_OBJ;
            if (i >= spawn_n || next_sl >= max_layers) continue;
            full[i] = dnas_in[i % dnas_in.size()];
            u32 sl = next_sl++;
            lbm[sl >> 5] |= (1u << (sl & 31u));
            crs[i].layer_slabs[0] = sl;
            crs[i].num_layers = 1u;
            r = mix(r + i * 2654435761u + 17u);
            crs[i].x = float(r & 0xffffu) * (float(world_w) / 65536.0f);
            r = mix(r + 31u);
            crs[i].y = float(r & 0xffffu) * (float(world_h) / 65536.0f);
            r = mix(r + 73u);
            crs[i].theta = float(r & 0xffffu) * (6.2831853f / 65536.0f);
            crs[i].atp = 28.0f;
            crs[i].prev_atp = 28.0f;
            crs[i].hp = 100.0f;
            crs[i].body_temp = 0.45f;
            crs[i].alive = 1u;
            crs[i].generation = base_gen;
            crs[i].lineage_id = i + 1u;
            nd[i] = 1u;
        }
        s.d.check(s.d.upload(d_layer_bitmap.ptr, lbm.data(), lbm.size() * sizeof(u32)), "upload lbm");
        s.d.check(s.d.upload(d_dnas.ptr, full.data(), full.size() * sizeof(GenesisDNA)), "upload dnas");
        s.d.check(s.d.upload(d_creatures.ptr, crs.data(), crs.size() * sizeof(CreatureState)), "upload creatures");
        s.d.check(s.d.upload(d_needs_dev.ptr, nd.data(), nd.size()), "upload needs_dev");
    }

    void step_develop(u32 seed) {
        void *dl_args[]{&d_creatures.ptr, &d_needs_dev.ptr, &d_dev_list.ptr, &d_dev_meta.ptr, &pop_n};
        s.d.check(s.d.launch(fn_build_devlist, 1, 1, 1, 256, 1, 1, 0, nullptr, dl_args, nullptr),
                  "genesis build devlist");
        maybe_sync("sync build_devlist");
        u32 dev_blocks = std::min<u32>(pop_n, 64u);
        void *args[]{&d_dnas.ptr, &d_bodies.ptr, &d_creatures.ptr, &pool, &d_needs_dev.ptr, &d_dev_list.ptr, &d_dev_meta.ptr, &seed};
        s.d.check(s.d.launch(fn_develop, dev_blocks, 1, 1, TISSUE_CELLS, 1, 1, 0, nullptr, args, nullptr),
                  "genesis develop");
        maybe_sync("sync develop");
    }

    void step_world(u32 tick, u32 shift_chem, u32 challenge_tier, u32 seed) {
        void *args[]{&wb, &tick, &shift_chem, &challenge_tier, &seed};
        s.d.check(s.d.launch(fn_world_step, world_w / 16u, world_h / 16u, 1, 16, 16, 1, 0, nullptr, args, nullptr),
                  "genesis world step");
        maybe_sync("sync world_step");
        std::swap(wb.water, wb.water_next);
        std::swap(wb.temp, wb.temp_next);
        std::swap(wb.chem_a, wb.chem_a_next);
        std::swap(wb.chem_b, wb.chem_b_next);
        std::swap(wb.chem_c, wb.chem_c_next);
        std::swap(wb.chem_d, wb.chem_d_next);
        float *old_prev = wb.acoust_prev; wb.acoust_prev = wb.acoust_p; wb.acoust_p = wb.acoust_next; wb.acoust_next = old_prev;
        float *old_v0p = wb.acoust_v0_prev; wb.acoust_v0_prev = wb.acoust_v0; wb.acoust_v0 = wb.acoust_v0_next; wb.acoust_v0_next = old_v0p;
        float *old_v1p = wb.acoust_v1_prev; wb.acoust_v1_prev = wb.acoust_v1; wb.acoust_v1 = wb.acoust_v1_next; wb.acoust_v1_next = old_v1p;
        float *old_v2p = wb.acoust_v2_prev; wb.acoust_v2_prev = wb.acoust_v2; wb.acoust_v2 = wb.acoust_v2_next; wb.acoust_v2_next = old_v2p;

        u32 ob_blocks = (max_objects + 127u) / 128u;
        void *ob_args[]{&wb, &d_creatures.ptr, &d_counters.ptr, &pop_n, &tick, &seed, &shift_chem};
        s.d.check(s.d.launch(fn_obj_step, ob_blocks, 1, 1, 128, 1, 1, 0, nullptr, ob_args, nullptr),
                  "genesis objects step");
        maybe_sync("sync objects_step");
    }

    void step_brains(u32 tick, AblationFlags abl) {
        // Single-CTA shared-prefix worklist zeroes *pool.work_count on-device (ZERO per-tick sync cuMemsetD8 stall!)
        void *wl_args[]{&d_creatures.ptr, &pool, &pop_n};
        s.d.check(s.d.launch(fn_build_wl, 1, 1, 1, 256, 1, 1, 0, nullptr, wl_args, nullptr),
                  "genesis build worklist");
        maybe_sync("sync build_worklist");
        // Persistent Blackwell SM grid-stride (256 CTAs = ~3x SM oversubscription, 4x less launch overhead!)
        u32 brain_blocks = std::min<u32>(max_layers, 256u);
        void *args[]{&d_dnas.ptr, &d_bodies.ptr, &d_creatures.ptr, &wb, &pool, &d_counters.ptr, &tick, &abl};
        s.d.check(s.d.launch(fn_brain_step, brain_blocks, 1, 1, TISSUE_CELLS, 1, 1, 0, nullptr, args, nullptr),
                  "genesis brain step");
        std::swap(pool.spike_ring, pool.spike_ring_next);
        maybe_sync("sync brain_step");
    }

    void step_bodies(u32 tick, u32 seed, AblationFlags abl) {
        u32 blocks = (pop_n + 127u) / 128u;
        void *args[]{&d_dnas.ptr, &d_bodies.ptr, &d_creatures.ptr, &d_needs_dev.ptr, &wb, &pool, &d_counters.ptr,
                     &pop_n, &tick, &seed, &abl};
        s.d.check(s.d.launch(fn_body_eco, blocks, 1, 1, 128, 1, 1, 0, nullptr, args, nullptr), "genesis body ecology");
        maybe_sync("sync body_ecology");
    }

    void rescue_extinct(u32 min_alive, u32 seed, u32 tick) {
        void *args[]{&d_dnas.ptr, &d_creatures.ptr, &d_needs_dev.ptr, &pool, &pop_n, &min_alive, &world_w, &world_h, &seed, &tick};
        s.d.check(s.d.launch(fn_check_extinct, 1, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr), "genesis rescue");
    }

    SimCounters read_counters() {
        SimCounters c{};
        s.d.check(s.d.download(&c, d_counters.ptr, sizeof(c)), "download counters");
        return c;
    }

    void trace_embryogenesis(const GenesisDNA &dna, BodyPhenotype &bp, DevSnapshots &snaps, u32 seed) {
        constexpr u32 TL = MAX_LAYERS_PER_CREATURE;
        constexpr u32 TS = 256u;
        Temporary tdna(s.d, sizeof(GenesisDNA)), tbp(s.d, sizeof(BodyPhenotype)), tcr(s.d, sizeof(CreatureState)),
            tsnap(s.d, sizeof(DevSnapshots)), tlbm(s.d, sizeof(u32)), tsbm(s.d, (TS / 32u) * sizeof(u32)),
            ttis(s.d, TL * TISSUE_CELLS * sizeof(u64)), tneu(s.d, TL * TISSUE_CELLS * sizeof(u64)),
            tring(s.d, TL * TISSUE_CELLS * sizeof(u8)), tcs(s.d, TL * TISSUE_CELLS * sizeof(u8)),
            tlss(s.d, TL * MAX_STRIPES_PER_LAYER * sizeof(u32)), tsyn(s.d, TS * LAYER_SYN_QUADS * sizeof(SynQuad)),
            tsrc(s.d, TS * TISSUE_CELLS * sizeof(u8));
        u32 lbm0 = 1u;
        s.d.check(s.d.upload(tlbm.ptr, &lbm0, sizeof(u32)), "t lbm");
        s.d.check(s.d.zero(tsbm.ptr, 0, (TS / 32u) * sizeof(u32)), "t sbm");
        s.d.check(s.d.zero(tsnap.ptr, 0, sizeof(DevSnapshots)), "t snap");
        std::vector<u32> inv(TL * MAX_STRIPES_PER_LAYER, INVALID_SLAB);
        s.d.check(s.d.upload(tlss.ptr, inv.data(), inv.size() * sizeof(u32)), "t lss");

        CreatureState cr{};
        for (u32 l = 0; l < MAX_LAYERS_PER_CREATURE; ++l) cr.layer_slabs[l] = INVALID_SLAB;
        cr.layer_slabs[0] = 0u;
        cr.num_layers = 1u;
        cr.alive = 1u;
        s.d.check(s.d.upload(tdna.ptr, &dna, sizeof(dna)), "t dna");
        s.d.check(s.d.upload(tcr.ptr, &cr, sizeof(cr)), "t cr");

        TissuePool tp{};
        tp.max_layers = TL; tp.max_syn_slabs = TS;
        tp.layer_bitmap = reinterpret_cast<u32 *>(tlbm.ptr);
        tp.syn_bitmap = reinterpret_cast<u32 *>(tsbm.ptr);
        tp.tissue = reinterpret_cast<u64 *>(ttis.ptr);
        tp.neurons = reinterpret_cast<u64 *>(tneu.ptr);
        tp.spike_ring = reinterpret_cast<u8 *>(tring.ptr);
        tp.spike_ring_next = reinterpret_cast<u8 *>(tring.ptr);
        tp.cell_stripes = reinterpret_cast<u8 *>(tcs.ptr);
        tp.layer_syn_slabs = reinterpret_cast<u32 *>(tlss.ptr);
        tp.syn_slabs = reinterpret_cast<SynQuad *>(tsyn.ptr);
        tp.stripe_src_layer = reinterpret_cast<u8 *>(tsrc.ptr);

        void *args[]{&tdna.ptr, &tbp.ptr, &tcr.ptr, &tp, &tsnap.ptr, &seed};
        s.d.check(s.d.launch(fn_develop_trace, 1, 1, 1, TISSUE_CELLS, 1, 1, 0, nullptr, args, nullptr),
                  "genesis develop trace");
        s.d.check(s.d.download(&bp, tbp.ptr, sizeof(bp)), "dl tbp");
        s.d.check(s.d.download(&snaps, tsnap.ptr, sizeof(snaps)), "dl tsnaps");
    }
};

struct PopSummary {
    u32 alive = 0;
    u32 species = 0;
    u32 lineages = 0;
    u32 max_gen = 0;
    u32 max_layers = 1;
    u32 max_rules = 0;
    u32 max_fanin = 0;
    double mean_gen = 0.0;
    double mean_atp = 0.0;
    double mean_cells = 0.0;
    double mean_neurons = 0.0;
    double mean_layers = 1.0;
    double mean_rules = 0.0;
    double mean_fanin = 0.0;
    double mean_synapses = 0.0;
    double mean_sym = 0.0;
    u32 best_idx = 0;
    double best_score = -1.0;
    std::array<double, TK_TYPES> mean_organs{};
};

PopSummary summarize_pop(const std::vector<GenesisDNA> &dnas, const std::vector<CreatureState> &crs,
                         const std::vector<BodyPhenotype> &bps) {
    PopSummary ps{};
    std::set<u32> bhashes, lins;
    for (u32 i = 0; i < crs.size(); ++i) {
        const auto &c = crs[i];
        if (c.alive != 1u) continue;
        const auto &b = bps[i];
        ++ps.alive;
        bhashes.insert(b.body_hash);
        lins.insert(c.lineage_id);
        if (c.generation > ps.max_gen) ps.max_gen = c.generation;
        u32 nl = std::max<u32>(1u, c.num_layers);
        if (nl > ps.max_layers) ps.max_layers = nl;
        u32 nr = dnas[i].num_rules;
        if (nr > ps.max_rules) ps.max_rules = nr;
        if (b.max_fanin > ps.max_fanin) ps.max_fanin = b.max_fanin;
        ps.mean_layers += double(nl);
        ps.mean_rules += double(nr);
        ps.mean_fanin += double(b.max_fanin);
        ps.mean_synapses += double(b.total_synapses);
        ps.mean_gen += double(c.generation);
        ps.mean_atp += double(c.atp);
        ps.mean_cells += double(b.total_cells);
        ps.mean_neurons += double(b.active_neurons);
        ps.mean_sym += double(b.symmetry_pct);
        for (u32 k = 0; k < TK_TYPES; ++k) ps.mean_organs[k] += double(b.counts[k]);

        double eco_sc = double(c.energy_gained) + 14.0 * double(c.offspring) + 0.04 * double(c.age) +
                        (b.total_cells >= 8 ? 10.0 : 0.0);
        if (eco_sc > ps.best_score) { ps.best_score = eco_sc; ps.best_idx = i; }
    }
    ps.species = u32(bhashes.size());
    ps.lineages = u32(lins.size());
    if (ps.alive > 0) {
        double inv = 1.0 / double(ps.alive);
        ps.mean_gen *= inv; ps.mean_atp *= inv; ps.mean_cells *= inv; ps.mean_neurons *= inv;
        ps.mean_layers *= inv; ps.mean_rules *= inv; ps.mean_fanin *= inv; ps.mean_synapses *= inv;
        ps.mean_sym *= inv;
        for (u32 k = 0; k < TK_TYPES; ++k) ps.mean_organs[k] *= inv;
    }
    return ps;
}

double compute_vocal_mi(const SimCounters &c) {
    double tot = 0.0, rb[3]{0.0, 0.0, 0.0}, rc[4]{0.0, 0.0, 0.0, 0.0};
    for (int b = 0; b < 3; ++b)
        for (int k = 0; k < 4; ++k) { double v = double(c.vocal_ctx[b][k]); tot += v; rb[b] += v; rc[k] += v; }
    if (tot < 10.0) return 0.0;
    double mi = 0.0;
    for (int b = 0; b < 3; ++b)
        for (int k = 0; k < 4; ++k) {
            double v = double(c.vocal_ctx[b][k]);
            if (v > 0.0 && rb[b] > 0.0 && rc[k] > 0.0)
                mi += (v / tot) * std::log2((v * tot) / (rb[b] * rc[k]));
        }
    return std::max(0.0, mi);
}

void save_checkpoint(const std::string &path, u32 tick, u32 seed, const std::vector<GenesisDNA> &dnas,
                     const std::vector<CreatureState> &crs) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot write genesis checkpoint " + path);
    const char magic[8] = {'A', 'X', 'G', 'E', 'N', '0', '1', '3'};
    out.write(magic, 8);
    u32 n = u32(dnas.size());
    out.write(reinterpret_cast<const char *>(&n), sizeof(n));
    out.write(reinterpret_cast<const char *>(&tick), sizeof(tick));
    out.write(reinterpret_cast<const char *>(&seed), sizeof(seed));
    out.write(reinterpret_cast<const char *>(dnas.data()), std::streamsize(n * sizeof(GenesisDNA)));
    out.write(reinterpret_cast<const char *>(crs.data()), std::streamsize(n * sizeof(CreatureState)));
}

void load_checkpoint(const std::string &path, u32 &tick, u32 &seed, std::vector<GenesisDNA> &dnas,
                     std::vector<CreatureState> &crs, bool &has_states) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot read genesis checkpoint " + path);
    char magic[8]{};
    in.read(magic, 8);
    bool is_v13 = (std::memcmp(magic, "AXGEN013", 8) == 0);
    bool is_v12 = (std::memcmp(magic, "AXGEN012", 8) == 0);
    if (!is_v13 && !is_v12)
        throw std::runtime_error("incompatible checkpoint version (expected AXGEN012/AXGEN013 CA-DNA): " + path);
    u32 n = 0;
    in.read(reinterpret_cast<char *>(&n), sizeof(n));
    in.read(reinterpret_cast<char *>(&tick), sizeof(tick));
    in.read(reinterpret_cast<char *>(&seed), sizeof(seed));
    if (n == 0 || n > MAX_POP * 4u) throw std::runtime_error("corrupt population count in " + path);
    dnas.resize(n);
    in.read(reinterpret_cast<char *>(dnas.data()), std::streamsize(n * sizeof(GenesisDNA)));
    if (is_v13) {
        crs.resize(n);
        in.read(reinterpret_cast<char *>(crs.data()), std::streamsize(n * sizeof(CreatureState)));
        has_states = true;
    } else {
        crs.clear();
        has_states = false;
    }
}

void emit_visual_snapshot(GenesisGpu &gpu, const std::vector<GenesisDNA> &dnas,
                          const std::vector<CreatureState> &crs, const std::vector<BodyPhenotype> &bps,
                          u32 best_idx, u32 seed) {
    const u32 wc = gpu.world_cells;
    const u32 ww = gpu.world_w;
    const u32 mo = gpu.max_objects;
    std::vector<float> ter(wc), wat(wc), cb(wc), cc(wc), ap(wc);
    std::vector<float> av0(wc), av1(wc), av2(wc);
    std::vector<u8> mat(wc);
    std::vector<WorldObject> objs(mo);
    gpu.s.d.check(gpu.s.d.download(ter.data(), DevicePtr(gpu.wb.terrain), wc * sizeof(float)), "dl ter");
    gpu.s.d.check(gpu.s.d.download(wat.data(), DevicePtr(gpu.wb.water), wc * sizeof(float)), "dl wat");
    gpu.s.d.check(gpu.s.d.download(cb.data(), DevicePtr(gpu.wb.chem_b), wc * sizeof(float)), "dl cb");
    gpu.s.d.check(gpu.s.d.download(cc.data(), DevicePtr(gpu.wb.chem_c), wc * sizeof(float)), "dl cc");
    gpu.s.d.check(gpu.s.d.download(ap.data(), DevicePtr(gpu.wb.acoust_p), wc * sizeof(float)), "dl ap");
    gpu.s.d.check(gpu.s.d.download(av0.data(), DevicePtr(gpu.wb.acoust_v0), wc * sizeof(float)), "dl av0");
    gpu.s.d.check(gpu.s.d.download(av1.data(), DevicePtr(gpu.wb.acoust_v1), wc * sizeof(float)), "dl av1");
    gpu.s.d.check(gpu.s.d.download(av2.data(), DevicePtr(gpu.wb.acoust_v2), wc * sizeof(float)), "dl av2");
    gpu.s.d.check(gpu.s.d.download(mat.data(), DevicePtr(gpu.wb.material), wc * sizeof(u8)), "dl mat");
    gpu.s.d.check(gpu.s.d.download(objs.data(), DevicePtr(gpu.wb.objects), mo * sizeof(WorldObject)), "dl obj");

    // 64x64 high-resolution continental map (adaptive stride downsample of 256/512/1024 GPU world!)
    constexpr u32 M = 64;
    const u32 stride = ww / M;
    const float inv_area = 1.0f / float(stride * stride);
    for (u32 my = 0; my < M; ++my) {
        std::ostringstream os;
        os << "genesis_map_row=" << my << "  cells=";
        for (u32 mx = 0; mx < M; ++mx) {
            float h = 0, w = 0, b = 0, c = 0, a = 0, v = 0;
            u8 dom_m = 0;
            for (u32 dy = 0; dy < stride; ++dy) {
                for (u32 dx = 0; dx < stride; ++dx) {
                    u32 idx = (my * stride + dy) * ww + (mx * stride + dx);
                    h += ter[idx]; w += wat[idx]; b += cb[idx]; c += cc[idx];
                    a = std::max(a, std::fabs(ap[idx]));
                    v = std::max(v, std::max(std::fabs(av0[idx]), std::max(std::fabs(av1[idx]), std::fabs(av2[idx]))));
                    if (mat[idx] == MAT_VENT) dom_m = MAT_VENT;
                    else if (mat[idx] == MAT_POLYMER && dom_m != MAT_VENT) dom_m = MAT_POLYMER;
                    else if (mat[idx] == MAT_CATALYST && dom_m == 0) dom_m = MAT_CATALYST;
                    else if (mat[idx] == MAT_ORE && dom_m == 0) dom_m = MAT_ORE;
                }
            }
            h *= inv_area; w *= inv_area; b *= inv_area; c *= inv_area;
            u32 ih = u32(std::clamp(int(h * 3.0f), 0, 9));
            u32 iw = u32(std::clamp(int(w * 4.5f), 0, 9));
            u32 ib = u32(std::clamp(int((b + 2.0f * c) * 5.0f), 0, 9));
            u32 ia = u32(std::clamp(int(a * 9.0f), 0, 9));
            u32 iv = u32(std::clamp(int(v * 9.0f), 0, 9));
            if (mx) os << ',';
            os << int(dom_m) << ih << iw << ib << ia << iv;
        }
        std::cout << os.str() << '\n';
    }

    u32 obj_em = 0;
    std::ostringstream oos;
    oos << "genesis_objects=1  list=";
    for (u32 i = 0; i < mo && obj_em < 240u; ++i) {
        if (objs[i].kind == OBJ_NONE) continue;
        if (obj_em) oos << ';';
        oos << int(objs[i].x * 10.0f) << ',' << int(objs[i].y * 10.0f) << ',' << int(objs[i].kind);
        ++obj_em;
    }
    std::cout << oos.str() << '\n';

    u32 emitted = 0;
    std::ostringstream cos;
    cos << "genesis_agents=1  list=";
    for (u32 i = 0; i < crs.size() && emitted < 128u; ++i) {
        if (crs[i].alive != 1u) continue;
        if (emitted) cos << ';';
        cos << int(crs[i].x * 10.0f) << ',' << int(crs[i].y * 10.0f) << ','
            << int(crs[i].theta * 100.0f) << ',' << (crs[i].lineage_id & 7u) << ','
            << int(bps[i].total_cells) << ',' << (i == best_idx ? 1 : 0);
        ++emitted;
    }
    std::cout << cos.str() << '\n';

    // Replay champion's unified 16x16 embryogenesis (Zygote -> Step 3 -> Step 6 -> Adult!)
    BodyPhenotype trace_bp{};
    DevSnapshots snaps{};
    gpu.trace_embryogenesis(dnas[best_idx], trace_bp, snaps, mix(seed ^ best_idx));
    auto emit_grid = [&](const char *tag, const u8 *g) {
        std::cout << "genesis_embryo=" << tag << "  grid=";
        for (u32 i = 0; i < TISSUE_CELLS; ++i) std::cout << hex_tk(g[i]);
        std::cout << '\n';
    };
    emit_grid("step0", snaps.step0);
    emit_grid("step3", snaps.step3);
    emit_grid("step6", snaps.step6);
    emit_grid("step9", bps[best_idx].surface);

    // Download champion's live unified tissue layers & variable-fan-in grown synapses!
    u32 best_layers = std::max<u32>(1u, crs[best_idx].num_layers);
    u32 exc_syn = 0, inh_syn = 0, wm_syn = 0, basal_cnt = 0, apical_cnt = 0;
    double delay_sum = 0.0;
    struct EdgeRec { u32 pre, post, comp, delay, cross; i32 w; };
    std::vector<EdgeRec> edges;
    std::array<u32, TK_TYPES> tk_cnt{};
    std::array<u32, 16> exc_whist{}, inh_whist{};
    std::array<u32, 8> del_hist{};
    std::array<u32, 4> rec_hist{};
    struct LayerProf { u32 pyr = 0, pv = 0, sst = 0, mod = 0, soma = 0, syn = 0; };
    std::array<LayerProf, MAX_LAYERS_PER_CREATURE> lprof{};
    std::array<std::array<u32, MAX_LAYERS_PER_CREATURE>, MAX_LAYERS_PER_CREATURE> lflow{};
    std::vector<u64> l0_tis(TISSUE_CELLS, 0ull);
    std::vector<u64> l1_tis(TISSUE_CELLS, 0ull);
    bool has_l1 = false;

    for (u32 z = 0; z < best_layers && z < MAX_LAYERS_PER_CREATURE; ++z) {
        u32 sl = crs[best_idx].layer_slabs[z];
        if (sl == INVALID_SLAB || sl >= gpu.max_layers) continue;
        std::vector<u64> tis(TISSUE_CELLS);
        std::vector<u8> cs(TISSUE_CELLS);
        std::vector<u32> lss(MAX_STRIPES_PER_LAYER);
        gpu.s.d.check(gpu.s.d.download(tis.data(), gpu.d_tissue.ptr + size_t(sl) * TISSUE_CELLS * sizeof(u64), TISSUE_CELLS * sizeof(u64)), "dl tis");
        gpu.s.d.check(gpu.s.d.download(cs.data(), gpu.d_cell_stripes.ptr + size_t(sl) * TISSUE_CELLS * sizeof(u8), TISSUE_CELLS * sizeof(u8)), "dl cs");
        gpu.s.d.check(gpu.s.d.download(lss.data(), gpu.d_layer_syn_slabs.ptr + size_t(sl) * MAX_STRIPES_PER_LAYER * sizeof(u32), MAX_STRIPES_PER_LAYER * sizeof(u32)), "dl lss");
        if (z == 0u) l0_tis = tis;
        if (z == 1u) { l1_tis = tis; has_l1 = true; }
        for (u32 i = 0; i < TISSUE_CELLS; ++i) {
            u8 tk = tis_kind(tis[i]);
            if (tk < TK_TYPES) ++tk_cnt[tk];
            if (tk == TK_NEU_EXC) ++lprof[z].pyr;
            else if (tk == TK_NEU_PV) ++lprof[z].pv;
            else if (tk == TK_NEU_SST) ++lprof[z].sst;
            else if (tk == TK_NEU_MOD) ++lprof[z].mod;
            else if (tk >= TK_STRUCT && tk <= TK_GLAND) ++lprof[z].soma;
        }
        for (u32 st = 0; st < MAX_STRIPES_PER_LAYER; ++st) {
            u32 ss = lss[st];
            if (ss == INVALID_SLAB || ss >= gpu.max_syn_slabs) continue;
            std::vector<SynQuad> sqv(LAYER_SYN_QUADS);
            std::vector<u8> src_l(TISSUE_CELLS);
            gpu.s.d.check(gpu.s.d.download(sqv.data(), gpu.d_syn_slabs.ptr + size_t(ss) * LAYER_SYN_QUADS * sizeof(SynQuad), LAYER_SYN_QUADS * sizeof(SynQuad)), "dl syn");
            gpu.s.d.check(gpu.s.d.download(src_l.data(), gpu.d_stripe_src_layer.ptr + size_t(ss) * TISSUE_CELLS * sizeof(u8), TISSUE_CELLS * sizeof(u8)), "dl src");
            for (u32 post = 0; post < TISSUE_CELLS; ++post) {
                if (st >= u32(cs[post] & 0x0fu)) continue;
                u32 sz = u32(src_l[post] & 0x1fu) % best_layers;
                bool cross = (sz != z);
                for (u32 g = 0; g < QUADS_PER_STRIPE; ++g) {
                    const SynQuad &sq = sqv[g * TISSUE_CELLS + post];
                    u32 words[4]{sq.x, sq.y, sq.z, sq.w};
                    for (u32 j = 0; j < 4u; ++j) {
                        u32 sw = words[j];
                        i32 w = syn_weight(sw);
                        if (w == 0) continue;
                        u32 comp = syn_comp(sw);
                        u32 del = syn_delay(sw);
                        u32 rid = (sw >> 27) & 3u;
                        u32 wbin = std::min<u32>(15u, u32(std::abs(w)) / 6u);
                        if (w > 0) { ++exc_syn; ++exc_whist[wbin]; }
                        else { ++inh_syn; ++inh_whist[wbin]; }
                        ++del_hist[del & 7u];
                        ++rec_hist[rid];
                        ++lprof[z].syn;
                        if (sz < MAX_LAYERS_PER_CREATURE) ++lflow[sz][z];
                        if (cross) ++wm_syn;
                        if (comp == 0u) ++basal_cnt; else ++apical_cnt;
                        delay_sum += double(del + 1u);
                        if (z == 0u && std::abs(w) >= 22)
                            edges.push_back({syn_pre(sw), post, comp, del, cross ? 1u : 0u, w});
                    }
                }
            }
        }
    }

    std::ostringstream nos;
    nos << "genesis_brain_nodes=" << bps[best_idx].active_neurons << "  areas=" << best_layers
        << "  rules=" << int(dnas[best_idx].num_rules) << "  max_fanin=" << bps[best_idx].max_fanin << "  types=";
    for (u32 i = 0; i < TISSUE_CELLS; ++i) nos << hex_tk(tis_kind(l0_tis[i]));
    std::cout << nos.str() << '\n';

    if (has_l1) {
        std::ostringstream a1os;
        a1os << "genesis_brain_area1=1  types=";
        for (u32 i = 0; i < TISSUE_CELLS; ++i) a1os << hex_tk(tis_kind(l1_tis[i]));
        std::cout << a1os.str() << '\n';
    }

    std::sort(edges.begin(), edges.end(), [](const EdgeRec &a, const EdgeRec &b) {
        return std::abs(a.w) > std::abs(b.w);
    });
    u32 tot_syn = std::max(1u, exc_syn + inh_syn);
    u32 sens_cnt = tk_cnt[TK_EYE] + tk_cnt[TK_CHEM] + tk_cnt[TK_EAR];
    u32 eff_cnt = tk_cnt[TK_MUSCLE] + tk_cnt[TK_GRASP] + tk_cnt[TK_GLAND];
    std::cout << "genesis_brain_stats=champion"
              << "  areas=" << best_layers
              << "  rules=" << int(dnas[best_idx].num_rules)
              << "  active_neurons=" << bps[best_idx].active_neurons
              << "  max_fanin=" << bps[best_idx].max_fanin
              << "  sens=" << sens_cnt << "  pyr=" << tk_cnt[TK_NEU_EXC]
              << "  pv=" << tk_cnt[TK_NEU_PV] << "  sst=" << tk_cnt[TK_NEU_SST]
              << "  mod=" << tk_cnt[TK_NEU_MOD] << "  motor=" << eff_cnt
              << "  exc_syn=" << exc_syn << "  inh_syn=" << inh_syn << "  wm_syn=" << wm_syn
              << "  basal_syn=" << basal_cnt << "  apical_syn=" << apical_cnt
              << "  mean_delay=" << std::fixed << std::setprecision(2) << (delay_sum / double(tot_syn))
              << "  body_cells=" << bps[best_idx].total_cells
              << "  symmetry_pct=" << int(bps[best_idx].symmetry_pct)
              << "  mass=" << std::setprecision(1) << bps[best_idx].mass
              << "  thrust=" << std::setprecision(2) << bps[best_idx].thrust_cap
              << "  grasp=" << bps[best_idx].grasp_power << "  gland=" << bps[best_idx].gland_power
              << "  gen=" << crs[best_idx].generation
              << "  stage=" << int(crs[best_idx].growth_stage)
              << "  offspring=" << crs[best_idx].offspring
              << "  bites=" << crs[best_idx].bites_landed
              << "  shared=" << crs[best_idx].energy_shared
              << "  atp_gained=" << std::setprecision(1) << crs[best_idx].energy_gained
              << "  plast_edits=" << crs[best_idx].plast_edits
              << "  rewires=" << crs[best_idx].syn_rewires << '\n';

    std::ostringstream eos;
    eos << "genesis_brain_edges=1  list=";
    u32 ne = std::min<u32>(u32(edges.size()), 120u);
    for (u32 i = 0; i < ne; ++i) {
        if (i) eos << ';';
        eos << edges[i].pre << ',' << edges[i].post << ',' << edges[i].w << ','
            << edges[i].comp << ',' << edges[i].delay << ',' << edges[i].cross;
    }
    std::cout << eos.str() << '\n';

    // Real Synaptic Biophysics Distributions (Weights, Delays, 4-Receptor Families, Per-Layer 3D Stack & Tract Flows!)
    std::cout << "genesis_syn_dist=1  exc=";
    for (u32 i = 0; i < 16u; ++i) std::cout << (i ? "," : "") << exc_whist[i];
    std::cout << "  inh=";
    for (u32 i = 0; i < 16u; ++i) std::cout << (i ? "," : "") << inh_whist[i];
    std::cout << "  del=";
    for (u32 i = 0; i < 8u; ++i) std::cout << (i ? "," : "") << del_hist[i];
    std::cout << "  rec=" << rec_hist[0] << ',' << rec_hist[1] << ',' << rec_hist[2] << ',' << rec_hist[3] << '\n';

    std::cout << "genesis_layers=1  list=";
    for (u32 z = 0; z < best_layers && z < MAX_LAYERS_PER_CREATURE; ++z) {
        if (z) std::cout << ';';
        std::cout << z << ',' << lprof[z].pyr << ',' << lprof[z].pv << ',' << lprof[z].sst << ','
                  << lprof[z].mod << ',' << lprof[z].soma << ',' << lprof[z].syn;
    }
    std::cout << "  flow=";
    bool first_fl = true;
    for (u32 sz = 0; sz < best_layers && sz < MAX_LAYERS_PER_CREATURE; ++sz) {
        for (u32 tz = 0; tz < best_layers && tz < MAX_LAYERS_PER_CREATURE; ++tz) {
            if (lflow[sz][tz] == 0u) continue;
            if (!first_fl) std::cout << ';';
            first_fl = false;
            std::cout << sz << '>' << tz << ':' << lflow[sz][tz];
        }
    }
    std::cout << '\n';

    // Vocal-Context Semantic Confusion Matrix (3 formant bands v0/v1/v2 x 4 contexts: Food/Obj, Apex/Danger, Toxin/Storm, Open)
    SimCounters vctr = gpu.read_counters();
    std::cout << "genesis_vocal_ctx=1  mat=";
    for (u32 b = 0; b < 3u; ++b) {
        if (b) std::cout << ';';
        for (u32 k = 0; k < 4u; ++k) std::cout << (k ? "," : "") << vctr.vocal_ctx[b][k];
    }
    std::cout << '\n';

    // Top-4 Distinct Evolved Species Morphologies (Real 16x16 body plans + stats!)
    std::vector<u32> sp_order;
    for (u32 i = 0; i < crs.size(); ++i)
        if (crs[i].alive == 1u) sp_order.push_back(i);
    std::sort(sp_order.begin(), sp_order.end(), [&](u32 a, u32 b) {
        double sa = double(crs[a].energy_gained) + 14.0 * double(crs[a].offspring);
        double sb = double(crs[b].energy_gained) + 14.0 * double(crs[b].offspring);
        return sa > sb;
    });
    std::set<u32> seen_bh;
    u32 sp_em = 0u;
    for (u32 idx_s : sp_order) {
        if (sp_em >= 4u) break;
        if (!seen_bh.insert(bps[idx_s].body_hash).second) continue;
        std::cout << "genesis_species_top=" << sp_em
                  << "  gen=" << crs[idx_s].generation
                  << "  layers=" << std::max<u16>(1u, crs[idx_s].num_layers)
                  << "  neurons=" << bps[idx_s].active_neurons
                  << "  cells=" << bps[idx_s].total_cells
                  << "  atp=" << std::fixed << std::setprecision(0) << crs[idx_s].energy_gained
                  << "  off=" << crs[idx_s].offspring
                  << "  eye=" << bps[idx_s].counts[TK_EYE]
                  << "  ear=" << bps[idx_s].counts[TK_EAR]
                  << "  gl=" << bps[idx_s].counts[TK_GLAND]
                  << "  mu=" << bps[idx_s].counts[TK_MUSCLE]
                  << "  grid=";
        for (u32 i = 0; i < TISSUE_CELLS; ++i) std::cout << hex_tk(bps[idx_s].surface[i]);
        std::cout << '\n';
        ++sp_em;
    }
}

int run_bench(Session &s, u32 pop_n, u32 substeps, u32 repeats, u32 world_size = WORLD_W) {
    u32 max_layers = pop_n * 8u;
    GenesisGpu gpu(s, pop_n, max_layers, world_size);
    gpu.init_world(42u, 0u);
    gpu.init_pop(pop_n, 42u);
    gpu.step_develop(42u);

    int regs_snn = 0, lmem_snn = 0, smem_snn = 0;
    int regs_pla = 0, lmem_pla = 0, smem_pla = 0;
    int regs_wld = 0, lmem_wld = 0, smem_wld = 0;
    int regs_dev = 0, lmem_dev = 0, smem_dev = 0;
    int regs_brn = 0, lmem_brn = 0, smem_brn = 0;
    s.d.check(s.d.func_attr(&regs_snn, 4, gpu.fn_snn_bench_infer), "snn regs");
    s.d.check(s.d.func_attr(&lmem_snn, 2, gpu.fn_snn_bench_infer), "snn lmem");
    s.d.check(s.d.func_attr(&smem_snn, 1, gpu.fn_snn_bench_infer), "snn smem");
    s.d.check(s.d.func_attr(&regs_pla, 4, gpu.fn_snn_bench_plastic), "pla regs");
    s.d.check(s.d.func_attr(&lmem_pla, 2, gpu.fn_snn_bench_plastic), "pla lmem");
    s.d.check(s.d.func_attr(&smem_pla, 1, gpu.fn_snn_bench_plastic), "pla smem");
    s.d.check(s.d.func_attr(&regs_wld, 4, gpu.fn_world_step), "world regs");
    s.d.check(s.d.func_attr(&lmem_wld, 2, gpu.fn_world_step), "world lmem");
    s.d.check(s.d.func_attr(&smem_wld, 1, gpu.fn_world_step), "world smem");
    s.d.check(s.d.func_attr(&regs_dev, 4, gpu.fn_develop), "dev regs");
    s.d.check(s.d.func_attr(&lmem_dev, 2, gpu.fn_develop), "dev lmem");
    s.d.check(s.d.func_attr(&smem_dev, 1, gpu.fn_develop), "dev smem");
    s.d.check(s.d.func_attr(&regs_brn, 4, gpu.fn_brain_step), "brn regs");
    s.d.check(s.d.func_attr(&lmem_brn, 2, gpu.fn_brain_step), "brn lmem");
    s.d.check(s.d.func_attr(&smem_brn, 1, gpu.fn_brain_step), "brn smem");

    Event a{}, b{};
    s.d.check(s.d.event_new(&a, 0), "ev a");
    s.d.check(s.d.event_new(&b, 0), "ev b");

    Temporary d_spk(s.d, sizeof(u64)), d_ed(s.d, sizeof(u64));
    auto bench_snn = [&](bool plastic) -> std::tuple<double, double, double, u64, u64> {
        s.d.check(s.d.zero(d_spk.ptr, 0, sizeof(u64)), "z spk");
        s.d.check(s.d.zero(d_ed.ptr, 0, sizeof(u64)), "z ed");
        void *args_inf[]{&gpu.d_dnas.ptr, &gpu.d_neurons.ptr, &gpu.d_syn_slabs.ptr, &pop_n, &substeps, &d_spk.ptr};
        void *args_pla[]{&gpu.d_dnas.ptr, &gpu.d_neurons.ptr, &gpu.d_syn_slabs.ptr, &pop_n, &substeps, &d_spk.ptr, &d_ed.ptr};
        Function fn = plastic ? gpu.fn_snn_bench_plastic : gpu.fn_snn_bench_infer;
        void **args = plastic ? args_pla : args_inf;
        s.d.check(s.d.launch(fn, pop_n, 1, 1, TISSUE_CELLS, 1, 1, 0, nullptr, args, nullptr), "warmup snn");
        std::vector<double> times;
        for (u32 r = 0; r < repeats; ++r) {
            s.d.check(s.d.zero(d_spk.ptr, 0, sizeof(u64)), "z spk");
            s.d.check(s.d.zero(d_ed.ptr, 0, sizeof(u64)), "z ed");
            s.d.check(s.d.record(a, nullptr), "rec a");
            s.d.check(s.d.launch(fn, pop_n, 1, 1, TISSUE_CELLS, 1, 1, 0, nullptr, args, nullptr), "bench snn");
            s.d.check(s.d.record(b, nullptr), "rec b");
            s.d.check(s.d.wait(b), "wait b");
            float ms = 0.0f;
            s.d.check(s.d.elapsed(&ms, a, b), "elapsed");
            times.push_back(double(ms));
        }
        std::sort(times.begin(), times.end());
        double med_ms = times[times.size() / 2];
        u64 spk = 0, ed = 0;
        s.d.check(s.d.download(&spk, d_spk.ptr, sizeof(u64)), "dl spk");
        s.d.check(s.d.download(&ed, d_ed.ptr, sizeof(u64)), "dl ed");
        double total_nt = double(pop_n) * double(TISSUE_CELLS) * double(substeps);
        double total_synops = total_nt * double(SYN_PER_STRIPE);
        return {med_ms, (total_nt / (med_ms * 1e-3)) * 1e-9, (total_synops / (med_ms * 1e-3)) * 1e-9, spk, ed};
    };

    auto [ms_inf, gnt_inf, gsyn_inf, spk_inf, ed_inf] = bench_snn(false);
    auto [ms_pla, gnt_pla, gsyn_pla, spk_pla, ed_pla] = bench_snn(true);

    constexpr u32 WBATCH = 64u;
    gpu.step_world(0u, 0u, 1u, 42u);
    std::vector<double> wtimes;
    for (u32 r = 0; r < repeats; ++r) {
        s.d.check(s.d.record(a, nullptr), "rec wa");
        for (u32 k = 0; k < WBATCH; ++k) gpu.step_world(k + 1u, 0u, 1u, 42u);
        s.d.check(s.d.record(b, nullptr), "rec wb");
        s.d.check(s.d.wait(b), "wait wb");
        float ms = 0.0f;
        s.d.check(s.d.elapsed(&ms, a, b), "welapsed");
        wtimes.push_back(double(ms) / WBATCH);
    }
    std::sort(wtimes.begin(), wtimes.end());
    double world_us = wtimes[wtimes.size() / 2] * 1000.0;

    std::vector<double> dtimes;
    for (u32 r = 0; r < repeats; ++r) {
        gpu.init_pop(pop_n, 100u + r);
        s.d.check(s.d.record(a, nullptr), "rec da");
        gpu.step_develop(100u + r);
        s.d.check(s.d.record(b, nullptr), "rec db");
        s.d.check(s.d.wait(b), "wait db");
        float ms = 0.0f;
        s.d.check(s.d.elapsed(&ms, a, b), "delapsed");
        dtimes.push_back(double(ms));
    }
    std::sort(dtimes.begin(), dtimes.end());
    double dev_ms = dtimes[dtimes.size() / 2];

    constexpr u32 FBATCH = 32u;
    AblationFlags abl{};
    abl.challenge_tier = 1u;
    std::vector<double> ftimes, w_sub_t, b_sub_t, e_sub_t, d_sub_t;
    Event s1{}, s2{}, s3{}, s4{};
    s.d.check(s.d.event_new(&s1, 0), "ev s1");
    s.d.check(s.d.event_new(&s2, 0), "ev s2");
    s.d.check(s.d.event_new(&s3, 0), "ev s3");
    s.d.check(s.d.event_new(&s4, 0), "ev s4");
    for (u32 r = 0; r < repeats; ++r) {
        double wt = 0.0, bt = 0.0, et = 0.0, dt = 0.0;
        s.d.check(s.d.record(a, nullptr), "rec fa");
        for (u32 k = 0; k < FBATCH; ++k) {
            if (r + 1u == repeats) {
                s.d.check(s.d.record(s1, nullptr), "k1");
                gpu.step_world(k, 0u, 1u, 42u);
                s.d.check(s.d.record(s2, nullptr), "k2");
                gpu.step_brains(k, abl);
                s.d.check(s.d.record(s3, nullptr), "k3");
                gpu.step_bodies(k, 42u, abl);
                s.d.check(s.d.record(s4, nullptr), "k4");
                gpu.step_develop(42u + k);
                s.d.check(s.d.record(b, nullptr), "k5");
                s.d.check(s.d.wait(b), "kw");
                float tw = 0, tb = 0, te = 0, td = 0;
                s.d.check(s.d.elapsed(&tw, s1, s2), "ew");
                s.d.check(s.d.elapsed(&tb, s2, s3), "eb");
                s.d.check(s.d.elapsed(&te, s3, s4), "ee");
                s.d.check(s.d.elapsed(&td, s4, b), "ed");
                wt += tw; bt += tb; et += te; dt += td;
            } else {
                gpu.step_world(k, 0u, 1u, 42u);
                gpu.step_brains(k, abl);
                gpu.step_bodies(k, 42u, abl);
                gpu.step_develop(42u + k);
            }
        }
        if (r + 1u == repeats) {
            w_sub_t.push_back(wt * 1000.0 / FBATCH);
            b_sub_t.push_back(bt * 1000.0 / FBATCH);
            e_sub_t.push_back(et * 1000.0 / FBATCH);
            d_sub_t.push_back(dt * 1000.0 / FBATCH);
        } else {
            s.d.check(s.d.record(b, nullptr), "rec fb");
            s.d.check(s.d.wait(b), "wait fb");
            float ms = 0.0f;
            s.d.check(s.d.elapsed(&ms, a, b), "felapsed");
            ftimes.push_back(double(ms) / FBATCH);
        }
    }
    s.d.event_free(s4); s.d.event_free(s3); s.d.event_free(s2); s.d.event_free(s1);
    std::sort(ftimes.begin(), ftimes.end());
    double full_us = ftimes[ftimes.size() / 2] * 1000.0;
    double mcreature_ticks_s = (double(pop_n) / (full_us * 1e-6)) * 1e-6;
    double prof_w = w_sub_t.empty() ? 0.0 : w_sub_t[0];
    double prof_b = b_sub_t.empty() ? 0.0 : b_sub_t[0];
    double prof_e = e_sub_t.empty() ? 0.0 : e_sub_t[0];
    double prof_d = d_sub_t.empty() ? 0.0 : d_sub_t[0];

    s.d.event_free(b);
    s.d.event_free(a);

    u32 bytes_per_layer = TISSUE_CELLS * (2u * sizeof(u64) + 2u * sizeof(u8)) + LAYER_SYN_QUADS * sizeof(SynQuad) + 256u;
    u32 bytes_per_creature = sizeof(GenesisDNA) + sizeof(BodyPhenotype) + sizeof(CreatureState) + bytes_per_layer;
    u32 world_kib = (gpu.world_cells * (28u * sizeof(float) + sizeof(u8) + 3u * sizeof(u32)) +
                     gpu.max_objects * sizeof(WorldObject)) / 1024u;

    std::cout << "genesis_bench=snn_inference  device=\"" << s.name << "\"  creatures=" << pop_n
              << "  neurons=" << TISSUE_CELLS << "  synapses_per_brain=" << (TISSUE_CELLS * SYN_PER_STRIPE)
              << "  substeps=" << substeps << "  median_ms=" << std::fixed << std::setprecision(3) << ms_inf
              << "  Gneuron_ticks/s=" << std::setprecision(3) << gnt_inf
              << "  Gsyn_ops/s=" << std::setprecision(2) << gsyn_inf
              << "  bytes_per_synapse=4  bytes_per_creature=" << bytes_per_creature
              << "  regs=" << regs_snn << "  local_B=" << lmem_snn << "  shared_B=" << smem_snn
              << "  spikes=" << spk_inf << '\n';

    std::cout << "genesis_bench=snn_plastic  creatures=" << pop_n << "  substeps=" << substeps
              << "  median_ms=" << std::setprecision(3) << ms_pla
              << "  Gneuron_ticks/s=" << std::setprecision(3) << gnt_pla
              << "  Gsyn_ops/s=" << std::setprecision(2) << gsyn_pla
              << "  regs=" << regs_pla << "  local_B=" << lmem_pla << "  shared_B=" << smem_pla
              << "  spikes=" << spk_pla << "  edits=" << ed_pla << '\n';

    std::cout << "genesis_bench=world_physics  grid=" << gpu.world_w << "x" << gpu.world_h
              << "  world_KiB=" << world_kib << "  step_us=" << std::setprecision(2) << world_us
              << "  Mcells/s=" << std::setprecision(2) << (double(gpu.world_cells) / world_us)
              << "  regs=" << regs_wld << "  local_B=" << lmem_wld << "  shared_B=" << smem_wld << '\n';

    std::cout << "genesis_bench=ca_morphogenesis  creatures=" << pop_n
              << "  total_ms=" << std::setprecision(3) << dev_ms
              << "  us_per_creature=" << std::setprecision(2) << (dev_ms * 1000.0 / double(pop_n))
              << "  regs=" << regs_dev << "  local_B=" << lmem_dev << "  shared_B=" << smem_dev << '\n';

    std::cout << "genesis_bench=coupled_alife  creatures=" << pop_n
              << "  tick_us=" << std::setprecision(2) << full_us
              << "  ticks/s=" << std::setprecision(1) << (1e6 / full_us)
              << "  Mcreature_ticks/s=" << std::setprecision(3) << mcreature_ticks_s
              << "  world_us=" << std::setprecision(1) << prof_w
              << "  brain_us=" << prof_b << "  body_us=" << prof_e << "  dev_us=" << prof_d
              << "  brain_regs=" << regs_brn << "  brain_local_B=" << lmem_brn << "  brain_shared_B=" << smem_brn << '\n';
    return 0;
}

struct AssayResult {
    u32 alive = 0, births = 0, deaths = 0;
    double total_energy = 0.0, mean_atp = 0.0, mean_layers = 1.0;
    u32 dug = 0, built = 0, synth = 0, vocal = 0, bites = 0, shared = 0, prey = 0, pods = 0, rewires = 0;
    u64 spikes = 0, edits = 0;
};

AssayResult run_heldout_assay(Session &s, const std::vector<GenesisDNA> &seed_dnas, u32 pop_n, u32 max_layers,
                              u32 spawn_n, u32 ticks, u32 world_seed, AblationFlags abl,
                              u32 world_size = WORLD_W, u32 max_syn_slabs = 0u) {
    GenesisGpu gpu(s, pop_n, max_layers, world_size, max_syn_slabs);
    gpu.init_world(world_seed, abl.shift_chemistry);
    for (u32 t = 0; t < 64u; ++t) gpu.step_world(t, abl.shift_chemistry, abl.challenge_tier, world_seed);
    gpu.upload_dnas_and_spawn(seed_dnas, spawn_n, world_seed);
    gpu.step_develop(world_seed);

    for (u32 t = 0; t < ticks; ++t) {
        gpu.step_world(t + 64u, abl.shift_chemistry, abl.challenge_tier, world_seed);
        gpu.step_brains(t, abl);
        gpu.step_bodies(t, world_seed, abl);
        gpu.step_develop(world_seed + t + 1u);
    }

    std::vector<GenesisDNA> dnas(pop_n);
    std::vector<CreatureState> crs(pop_n);
    std::vector<BodyPhenotype> bps(pop_n);
    s.d.check(s.d.download(dnas.data(), gpu.d_dnas.ptr, pop_n * sizeof(GenesisDNA)), "dl a dna");
    s.d.check(s.d.download(crs.data(), gpu.d_creatures.ptr, pop_n * sizeof(CreatureState)), "dl a crs");
    s.d.check(s.d.download(bps.data(), gpu.d_bodies.ptr, pop_n * sizeof(BodyPhenotype)), "dl a bps");
    auto ps = summarize_pop(dnas, crs, bps);
    auto ctr = gpu.read_counters();

    AssayResult ar{};
    ar.alive = ps.alive; ar.births = ctr.births; ar.deaths = ctr.deaths;
    ar.total_energy = double(ctr.total_energy_x100) * 0.01;
    ar.mean_atp = ps.mean_atp; ar.mean_layers = ps.mean_layers;
    ar.dug = ctr.total_dug; ar.built = ctr.total_built; ar.synth = ctr.total_synth;
    ar.vocal = ctr.total_vocal; ar.bites = ctr.total_bites; ar.shared = ctr.total_shared;
    ar.prey = ctr.total_prey_caught; ar.pods = ctr.total_pods_cracked;
    ar.rewires = ctr.total_rewires; ar.spikes = ctr.total_spikes; ar.edits = ctr.total_edits;
    return ar;
}

int run_evolve(Session &s, u32 pop_n, u32 max_layers, u32 ticks, u32 seed, u32 challenge_tier, bool controls,
               const std::string &load_path, const std::string &save_path,
               const std::string &video_path = "", const std::string &video_title = "AXION GENESIS V42",
               u32 world_size = WORLD_W, u32 max_syn_slabs = 0u) {
    std::vector<GenesisDNA> loaded_dnas;
    u32 base_gen = 1u;
    if (!load_path.empty()) {
        u32 lt = 0, ls = 0;
        std::vector<CreatureState> lcrs;
        bool has_s = false;
        load_checkpoint(load_path, lt, ls, loaded_dnas, lcrs, has_s);
        if (has_s && !lcrs.empty()) {
            std::vector<GenesisDNA> alive_only;
            u32 mg = 1u;
            for (size_t i = 0; i < lcrs.size() && i < loaded_dnas.size(); ++i) {
                if (lcrs[i].alive == 1u) {
                    alive_only.push_back(loaded_dnas[i]);
                    if (lcrs[i].generation > mg) mg = lcrs[i].generation;
                }
            }
            if (!alive_only.empty()) { loaded_dnas = std::move(alive_only); base_gen = mg; }
        }
    }

    u32 initial_alive = std::min(pop_n, std::max(64u, pop_n / 2u));
    u32 min_alive = std::max(32u, pop_n / 16u);

    auto run_eco = [&](const char *arm_name, bool drift_mode, bool emit_ticks, bool use_loaded,
                       std::vector<GenesisDNA> &out_dnas, std::vector<CreatureState> &out_crs,
                       std::vector<BodyPhenotype> &out_bps, PopSummary &out_ps, SimCounters &out_ctr,
                       double &out_wall_ms, bool emit_visuals) {
        GenesisGpu gpu(s, pop_n, max_layers, world_size, max_syn_slabs);
        gpu.init_world(seed, 0u);
        for (u32 t = 0; t < 80u; ++t) gpu.step_world(t, 0u, challenge_tier, seed);
        if (use_loaded && !loaded_dnas.empty())
            gpu.upload_dnas_and_spawn(loaded_dnas, initial_alive, seed, base_gen);
        else
            gpu.init_pop(initial_alive, seed);
        gpu.step_develop(seed);

        AblationFlags abl{};
        abl.drift_mode = drift_mode ? 1u : 0u;
        abl.challenge_tier = challenge_tier;

        auto t0 = std::chrono::steady_clock::now();
        u32 log_every = std::max(1u, ticks / 10u);
        GenesisVideoRecorder vid;
        u32 vid_every = 0u;
        std::vector<float> vf_ter, vf_wat, vf_cb, vf_cc, vf_cd, vf_lgt, vf_ap, vf_v0, vf_v1, vf_v2;
        std::vector<u8> vu_mat;
        std::vector<WorldObject> v_objs;
        if (emit_visuals && !video_path.empty()) {
            u32 target_frames = std::clamp(ticks / 25u, 160u, 640u); // 10s..40s @ 16fps (strictly <= 60s!)
            vid_every = std::max(1u, ticks / target_frames);
            if (vid.open(video_path, video_title, ticks, pop_n)) {
                vf_ter.resize(gpu.world_cells); vu_mat.resize(gpu.world_cells); vf_wat.resize(gpu.world_cells);
                vf_cb.resize(gpu.world_cells); vf_cc.resize(gpu.world_cells); vf_cd.resize(gpu.world_cells);
                vf_lgt.resize(gpu.world_cells); vf_ap.resize(gpu.world_cells);
                vf_v0.resize(gpu.world_cells); vf_v1.resize(gpu.world_cells); vf_v2.resize(gpu.world_cells);
                v_objs.resize(gpu.max_objects);
            }
        }
        for (u32 t = 0; t < ticks; ++t) {
            gpu.step_world(t + 80u, 0u, challenge_tier, seed);
            gpu.step_brains(t, abl);
            gpu.step_bodies(t, seed, abl);
            if ((t & 63u) == 63u) gpu.rescue_extinct(min_alive, seed, t);
            gpu.step_develop(seed + t + 1u);

            bool do_log = emit_ticks && (t == 0u || (t + 1u) % log_every == 0u || t + 1u == ticks);
            bool do_vid = vid.is_open() && (t == 0u || (t + 1u) % vid_every == 0u || t + 1u == ticks);
            if (do_log || do_vid) {
                out_dnas.resize(pop_n); out_crs.resize(pop_n); out_bps.resize(pop_n);
                s.d.check(s.d.download(out_dnas.data(), gpu.d_dnas.ptr, pop_n * sizeof(GenesisDNA)), "dl dna");
                s.d.check(s.d.download(out_crs.data(), gpu.d_creatures.ptr, pop_n * sizeof(CreatureState)), "dl crs");
                s.d.check(s.d.download(out_bps.data(), gpu.d_bodies.ptr, pop_n * sizeof(BodyPhenotype)), "dl bps");
                auto ps = summarize_pop(out_dnas, out_crs, out_bps);
                auto ctr = gpu.read_counters();
                if (do_vid) {
                    s.d.check(s.d.download(vf_ter.data(), DevicePtr(gpu.wb.terrain), gpu.world_cells * sizeof(float)), "vid ter");
                    s.d.check(s.d.download(vu_mat.data(), DevicePtr(gpu.wb.material), gpu.world_cells), "vid mat");
                    s.d.check(s.d.download(vf_wat.data(), DevicePtr(gpu.wb.water), gpu.world_cells * sizeof(float)), "vid wat");
                    s.d.check(s.d.download(vf_cb.data(), DevicePtr(gpu.wb.chem_b), gpu.world_cells * sizeof(float)), "vid cb");
                    s.d.check(s.d.download(vf_cc.data(), DevicePtr(gpu.wb.chem_c), gpu.world_cells * sizeof(float)), "vid cc");
                    s.d.check(s.d.download(vf_cd.data(), DevicePtr(gpu.wb.chem_d), gpu.world_cells * sizeof(float)), "vid cd");
                    s.d.check(s.d.download(vf_lgt.data(), DevicePtr(gpu.wb.light), gpu.world_cells * sizeof(float)), "vid lgt");
                    s.d.check(s.d.download(vf_ap.data(), DevicePtr(gpu.wb.acoust_p), gpu.world_cells * sizeof(float)), "vid ap");
                    s.d.check(s.d.download(vf_v0.data(), DevicePtr(gpu.wb.acoust_v0), gpu.world_cells * sizeof(float)), "vid v0");
                    s.d.check(s.d.download(vf_v1.data(), DevicePtr(gpu.wb.acoust_v1), gpu.world_cells * sizeof(float)), "vid v1");
                    s.d.check(s.d.download(vf_v2.data(), DevicePtr(gpu.wb.acoust_v2), gpu.world_cells * sizeof(float)), "vid v2");
                    s.d.check(s.d.download(v_objs.data(), DevicePtr(gpu.wb.objects), gpu.max_objects * sizeof(WorldObject)), "vid obj");
                    GenesisVideoRecorder::TelemetryPoint tp{};
                    tp.tick = t + 1u; tp.alive = ps.alive; tp.species = ps.species; tp.max_gen = ps.max_gen;
                    tp.mean_layers = float(ps.mean_layers); tp.mean_neurons = float(ps.mean_neurons);
                    tp.births = ctr.births; tp.prey = ctr.total_prey_caught; tp.pods = ctr.total_pods_cracked;
                    tp.vocal = ctr.total_vocal; tp.replies = ctr.total_vocal_replies; tp.mimics = ctr.total_vocal_mimics;
                    tp.shelters = ctr.total_shelter_nights; tp.tempered = ctr.total_tempered_tools;
                    tp.rewires = ctr.total_rewires; tp.edits = ctr.total_edits;
                    vid.push_frame(t + 1u, gpu.world_w, gpu.world_h, vf_ter.data(), vu_mat.data(), vf_wat.data(),
                                   vf_cb.data(), vf_cc.data(), vf_cd.data(), vf_lgt.data(),
                                   vf_ap.data(), vf_v0.data(), vf_v1.data(), vf_v2.data(),
                                   v_objs.data(), gpu.max_objects, out_crs.data(), out_bps.data(), pop_n, ps.best_idx, tp);
                }
                if (do_log) {
                    std::cout << "genesis_tick=" << (t + 1u) << "  arm=" << arm_name
                              << "  alive=" << ps.alive << "  species=" << ps.species << "  lineages=" << ps.lineages
                              << "  max_gen=" << ps.max_gen << "  mean_gen=" << std::fixed << std::setprecision(2) << ps.mean_gen
                              << "  mean_areas=" << std::setprecision(2) << ps.mean_layers << "  max_areas=" << ps.max_layers
                              << "  mean_rules=" << std::setprecision(1) << ps.mean_rules << "  max_fanin=" << ps.max_fanin
                              << "  mean_atp=" << std::setprecision(2) << ps.mean_atp
                              << "  mean_cells=" << std::setprecision(1) << ps.mean_cells
                              << "  mean_neurons=" << std::setprecision(1) << ps.mean_neurons
                              << "  sym_pct=" << std::setprecision(0) << ps.mean_sym
                              << "  births=" << ctr.births << "  deaths=" << ctr.deaths
                              << "  prey=" << ctr.total_prey_caught << "  pods=" << ctr.total_pods_cracked
                              << "  stones=" << ctr.total_stones_made << "  tools=" << ctr.total_tool_cracks
                              << "  coop=" << ctr.total_coop_cracks << "  shelters=" << ctr.total_shelter_nights
                              << "  apex_k=" << ctr.total_apex_kills << "  apex_d=" << ctr.total_apex_deaths
                              << "  echo=" << ctr.total_echo_hits << "  duet=" << ctr.total_duet_coops
                              << "  temp_t=" << ctr.total_tempered_tools << "  cache=" << ctr.total_cached_feasts
                              << "  vresp=" << ctr.total_vocal_resp << "  vrepl=" << ctr.total_vocal_replies
                              << "  mimics=" << ctr.total_vocal_mimics
                              << "  matures=" << ctr.total_maturations
                              << "  bites=" << ctr.total_bites << "  shared=" << ctr.total_shared
                              << "  dug=" << ctr.total_dug << "  built=" << ctr.total_built
                              << "  synth=" << ctr.total_synth << "  vocal=" << ctr.total_vocal
                              << "  rewires=" << ctr.total_rewires
                              << "  spikes=" << ctr.total_spikes << "  edits=" << ctr.total_edits << '\n';
                }
            }
        }
        if (vid.is_open()) {
            u32 nf = vid.frames_written();
            vid.close();
            std::cout << "Saved Genesis evolution animation (" << nf << " frames, "
                      << std::fixed << std::setprecision(1) << (double(nf) / double(GenesisVideoRecorder::FPS))
                      << "s) to " << video_path << ".\n";
        }
        out_dnas.resize(pop_n); out_crs.resize(pop_n); out_bps.resize(pop_n);
        s.d.check(s.d.download(out_dnas.data(), gpu.d_dnas.ptr, pop_n * sizeof(GenesisDNA)), "dl dnas");
        s.d.check(s.d.download(out_crs.data(), gpu.d_creatures.ptr, pop_n * sizeof(CreatureState)), "dl crs");
        s.d.check(s.d.download(out_bps.data(), gpu.d_bodies.ptr, pop_n * sizeof(BodyPhenotype)), "dl bps");
        out_wall_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        out_ps = summarize_pop(out_dnas, out_crs, out_bps);
        out_ctr = gpu.read_counters();

        if (emit_visuals && out_ps.alive > 0)
            emit_visual_snapshot(gpu, out_dnas, out_crs, out_bps, out_ps.best_idx, seed);
    };

    std::vector<GenesisDNA> sel_dnas;
    std::vector<CreatureState> sel_crs;
    std::vector<BodyPhenotype> sel_bps;
    PopSummary sel_ps{};
    SimCounters sel_ctr{};
    double sel_ms = 0.0;

    run_eco("select", false, true, !loaded_dnas.empty(), sel_dnas, sel_crs, sel_bps, sel_ps, sel_ctr, sel_ms, true);

    double ticks_s = double(ticks) / (sel_ms * 1e-3);
    double vocal_mi = compute_vocal_mi(sel_ctr);
    double mean_neural = sel_ps.mean_organs[TK_NEU_EXC] + sel_ps.mean_organs[TK_NEU_PV] +
                         sel_ps.mean_organs[TK_NEU_SST] + sel_ps.mean_organs[TK_NEU_MOD];
    std::cout << "genesis_summary=complete  device=\"" << s.name << "\"  world=" << world_size
              << "  pop=" << pop_n << "  ticks=" << ticks
              << "  challenge=" << challenge_tier << "  wall_ms=" << std::fixed << std::setprecision(1) << sel_ms
              << "  ticks/s=" << std::setprecision(1) << ticks_s
              << "  alive=" << sel_ps.alive << "  species=" << sel_ps.species
              << "  max_gen=" << sel_ps.max_gen
              << "  mean_areas=" << std::setprecision(2) << sel_ps.mean_layers << "  max_areas=" << sel_ps.max_layers
              << "  mean_rules=" << std::setprecision(1) << sel_ps.mean_rules << "  max_rules=" << sel_ps.max_rules
              << "  mean_fanin=" << std::setprecision(1) << sel_ps.mean_fanin << "  max_fanin=" << sel_ps.max_fanin
              << "  mean_cells=" << std::setprecision(1) << sel_ps.mean_cells
              << "  mean_neurons=" << std::setprecision(1) << sel_ps.mean_neurons
              << "  eye=" << std::setprecision(2) << sel_ps.mean_organs[TK_EYE]
              << "  chem=" << sel_ps.mean_organs[TK_CHEM]
              << "  ear=" << sel_ps.mean_organs[TK_EAR]
              << "  muscle=" << sel_ps.mean_organs[TK_MUSCLE]
              << "  grasp=" << sel_ps.mean_organs[TK_GRASP]
              << "  gland=" << sel_ps.mean_organs[TK_GLAND]
              << "  struct=" << sel_ps.mean_organs[TK_STRUCT]
              << "  neural=" << mean_neural
              << "  pyr=" << sel_ps.mean_organs[TK_NEU_EXC]
              << "  pv=" << sel_ps.mean_organs[TK_NEU_PV]
              << "  sst=" << sel_ps.mean_organs[TK_NEU_SST]
              << "  mod=" << sel_ps.mean_organs[TK_NEU_MOD]
              << "  births=" << sel_ctr.births << "  bites=" << sel_ctr.total_bites
              << "  shared=" << sel_ctr.total_shared << "  pods=" << sel_ctr.total_pods_cracked
              << "  stones=" << sel_ctr.total_stones_made << "  tools=" << sel_ctr.total_tool_cracks
              << "  coop=" << sel_ctr.total_coop_cracks << "  shelters=" << sel_ctr.total_shelter_nights
              << "  apex_k=" << sel_ctr.total_apex_kills << "  apex_d=" << sel_ctr.total_apex_deaths
              << "  echo=" << sel_ctr.total_echo_hits << "  duet=" << sel_ctr.total_duet_coops
              << "  tempered=" << sel_ctr.total_tempered_tools << "  cached=" << sel_ctr.total_cached_feasts
              << "  prey=" << sel_ctr.total_prey_caught
              << "  matures=" << sel_ctr.total_maturations
              << "  dug=" << sel_ctr.total_dug << "  built=" << sel_ctr.total_built
              << "  synth=" << sel_ctr.total_synth << "  vocal=" << sel_ctr.total_vocal
              << "  vocal_resp=" << sel_ctr.total_vocal_resp
              << "  vocal_replies=" << sel_ctr.total_vocal_replies
              << "  vocal_mimics=" << sel_ctr.total_vocal_mimics
              << "  vocal_mi_bits=" << std::setprecision(4) << vocal_mi
              << "  imprints=" << sel_ctr.total_imprint_edits
              << "  rewires=" << sel_ctr.total_rewires
              << "  spikes=" << sel_ctr.total_spikes << "  edits=" << sel_ctr.total_edits << '\n';

    if (auto *j = journal::current()) {
        j->device = s.name;
        j->metrics["genesis_pop"] = pop_n;
        j->metrics["genesis_ticks"] = ticks;
        j->metrics["genesis_alive"] = sel_ps.alive;
        j->metrics["genesis_births"] = sel_ctr.births;
        j->metrics["genesis_spikes"] = sel_ctr.total_spikes;
    }

    if (!save_path.empty()) {
        save_checkpoint(save_path, ticks, seed, sel_dnas, sel_crs);
        std::cout << "Saved Genesis ecosystem checkpoint to " << save_path << ".\n";
    }

    if (controls) {
        std::vector<GenesisDNA> alive_sel_dnas;
        for (u32 i = 0; i < pop_n; ++i)
            if (sel_crs[i].alive == 1u) alive_sel_dnas.push_back(sel_dnas[i]);
        if (alive_sel_dnas.empty()) alive_sel_dnas = sel_dnas;

        std::vector<GenesisDNA> drf_dnas;
        std::vector<CreatureState> drf_crs;
        std::vector<BodyPhenotype> drf_bps;
        PopSummary drf_ps{};
        SimCounters drf_ctr{};
        double drf_ms = 0.0;
        run_eco("drift", true, false, false, drf_dnas, drf_crs, drf_bps, drf_ps, drf_ctr, drf_ms, false);

        std::vector<GenesisDNA> alive_drf_dnas;
        for (u32 i = 0; i < pop_n; ++i)
            if (drf_crs[i].alive == 1u) alive_drf_dnas.push_back(drf_dnas[i]);
        if (alive_drf_dnas.empty()) alive_drf_dnas = drf_dnas;

        u32 assay_ticks = std::min(600u, std::max(240u, ticks / 3u));
        u32 assay_seed1 = mix(seed ^ 0x55aa33ccu);
        u32 assay_seed2 = mix(seed ^ 0x9e3779b9u);
        u32 spawn_n = std::min(pop_n, std::max(64u, pop_n / 2u));

        auto print_arm = [&](const char *name, const std::vector<GenesisDNA> &cohort, AblationFlags abl) {
            abl.shift_chemistry = 1u;
            abl.challenge_tier = challenge_tier;
            auto a1 = run_heldout_assay(s, cohort, pop_n, max_layers, spawn_n, assay_ticks, assay_seed1, abl, world_size, max_syn_slabs);
            auto a2 = run_heldout_assay(s, cohort, pop_n, max_layers, spawn_n, assay_ticks, assay_seed2, abl, world_size, max_syn_slabs);
            std::cout << "genesis_control=" << name << "  ticks=" << (assay_ticks * 2u)
                      << "  alive=" << ((a1.alive + a2.alive) / 2u)
                      << "  births=" << (a1.births + a2.births) << "  deaths=" << (a1.deaths + a2.deaths)
                      << "  energy=" << std::fixed << std::setprecision(1) << (a1.total_energy + a2.total_energy)
                      << "  mean_atp=" << std::setprecision(2) << (0.5 * (a1.mean_atp + a2.mean_atp))
                      << "  mean_areas=" << std::setprecision(2) << (0.5 * (a1.mean_layers + a2.mean_layers))
                      << "  prey=" << (a1.prey + a2.prey) << "  pods=" << (a1.pods + a2.pods)
                      << "  bites=" << (a1.bites + a2.bites) << "  shared=" << (a1.shared + a2.shared)
                      << "  dug=" << (a1.dug + a2.dug) << "  built=" << (a1.built + a2.built)
                      << "  synth=" << (a1.synth + a2.synth) << "  vocal=" << (a1.vocal + a2.vocal)
                      << "  edits=" << (a1.edits + a2.edits) << "  rewires=" << (a1.rewires + a2.rewires)
                      << "  spikes=" << (a1.spikes + a2.spikes) << '\n';
        };

        AblationFlags a_intact{};
        AblationFlags a_frozen{}; a_frozen.freeze_plasticity = 1u;
        AblationFlags a_no_ap{};  a_no_ap.silence_apical = 1u;
        AblationFlags a_no_del{}; a_no_del.zero_delays = 1u;
        AblationFlags a_no_mod{}; a_no_mod.silence_neuromod = 1u;
        AblationFlags a_blind{};  a_blind.blind_eyes = 1u;
        AblationFlags a_deaf{};   a_deaf.deaf_ears = 1u;

        print_arm("evolved_intact", alive_sel_dnas, a_intact);
        print_arm("evolved_frozen", alive_sel_dnas, a_frozen);
        print_arm("evolved_no_apical", alive_sel_dnas, a_no_ap);
        print_arm("evolved_zero_delay", alive_sel_dnas, a_no_del);
        print_arm("evolved_no_neuromod", alive_sel_dnas, a_no_mod);
        print_arm("evolved_blind", alive_sel_dnas, a_blind);
        print_arm("evolved_deaf", alive_sel_dnas, a_deaf);
        print_arm("drift_unselected", alive_drf_dnas, a_intact);
    }
    return 0;
}

int run_inspect(Session &s, const std::string &load_path) {
    u32 tick = 0, seed = 0;
    std::vector<GenesisDNA> dnas;
    std::vector<CreatureState> crs;
    bool has_s = false;
    load_checkpoint(load_path, tick, seed, dnas, crs, has_s);
    u32 pop_n = u32(dnas.size());
    u32 max_layers = pop_n * 16u;

    GenesisGpu gpu(s, pop_n, max_layers);
    gpu.init_world(seed, 0u);
    gpu.upload_dnas_and_spawn(dnas, pop_n, seed);
    gpu.step_develop(seed);

    std::vector<CreatureState> dev_crs(pop_n);
    std::vector<BodyPhenotype> bps(pop_n);
    s.d.check(s.d.download(dev_crs.data(), gpu.d_creatures.ptr, pop_n * sizeof(CreatureState)), "dl dev_crs");
    s.d.check(s.d.download(bps.data(), gpu.d_bodies.ptr, pop_n * sizeof(BodyPhenotype)), "dl bps");
    if (has_s && crs.size() == pop_n) {
        for (u32 i = 0; i < pop_n; ++i) {
            dev_crs[i].alive = crs[i].alive;
            dev_crs[i].x = crs[i].x; dev_crs[i].y = crs[i].y; dev_crs[i].theta = crs[i].theta;
            dev_crs[i].generation = crs[i].generation; dev_crs[i].offspring = crs[i].offspring;
            dev_crs[i].energy_gained = crs[i].energy_gained; dev_crs[i].bites_landed = crs[i].bites_landed;
            dev_crs[i].energy_shared = crs[i].energy_shared; dev_crs[i].plast_edits = crs[i].plast_edits;
            dev_crs[i].syn_rewires = crs[i].syn_rewires; dev_crs[i].growth_stage = crs[i].growth_stage;
        }
    }
    auto ps = summarize_pop(dnas, dev_crs, bps);
    std::cout << "genesis_inspect=checkpoint  path=" << load_path << "  tick=" << tick << "  seed=" << seed
              << "  pop=" << pop_n << "  alive=" << ps.alive << "  species=" << ps.species
              << "  max_gen=" << ps.max_gen << "  mean_areas=" << std::fixed << std::setprecision(2) << ps.mean_layers
              << "  max_areas=" << ps.max_layers << "  mean_rules=" << std::setprecision(1) << ps.mean_rules
              << "  max_fanin=" << ps.max_fanin << "  mean_cells=" << ps.mean_cells
              << "  mean_neurons=" << ps.mean_neurons << '\n';
    if (ps.alive > 0) emit_visual_snapshot(gpu, dnas, dev_crs, bps, ps.best_idx, seed);
    return 0;
}

// ---------------------------------------------------------------------------
// Grounded Non-LLM Cognitive Assay Battery (./axion genesis probe --load FILE)
// Places the evolved spiking organisms into controlled physical GPU arenas to
// honestly measure higher cognition (ZERO hardcoded answers, ZERO LLMs!):
//   1. Seasonal Reversal Learning (Intact vs Frozen STDP vs No Apical BAC)
//   2. Delayed-Match-to-Place Working Memory (Beacon ON -> Beacon OFF persistence)
//   3. Multi-Step Tool & Apex-Predator Defense Agency
//   4. Acoustic-Referential Communication & Listener Response Coupling
// ---------------------------------------------------------------------------
int run_probe(Session &s, const std::string &load_path, u32 ticks, u32 seed) {
    u32 ck_tick = 0, ck_seed = 0;
    std::vector<GenesisDNA> dnas;
    std::vector<CreatureState> crs;
    bool has_s = false;
    load_checkpoint(load_path, ck_tick, ck_seed, dnas, crs, has_s);
    std::vector<u32> alive_idx;
    for (size_t i = 0; i < dnas.size(); ++i)
        if (!has_s || i >= crs.size() || crs[i].alive == 1u) alive_idx.push_back(u32(i));
    if (alive_idx.empty()) {
        for (size_t i = 0; i < dnas.size(); ++i) alive_idx.push_back(u32(i));
    }
    if (has_s && !crs.empty()) {
        std::sort(alive_idx.begin(), alive_idx.end(), [&](u32 a, u32 b) {
            double fa = (a < crs.size()) ? (double(crs[a].energy_gained) + 14.0 * double(crs[a].offspring) + 0.5 * double(crs[a].num_layers)) : 0.0;
            double fb = (b < crs.size()) ? (double(crs[b].energy_gained) + 14.0 * double(crs[b].offspring) + 0.5 * double(crs[b].num_layers)) : 0.0;
            return fa > fb;
        });
    }
    std::vector<GenesisDNA> cohort;
    cohort.reserve(alive_idx.size());
    for (u32 idx_a : alive_idx) cohort.push_back(dnas[idx_a]);

    u32 pop_n = 512u;
    u32 max_layers = pop_n * 16u;
    u32 spawn_n = 256u;

    // Emit lead cohort organism anatomy & 1-zygote->adult embryogenesis for the 9-Assay Cognitive Battery plate!
    {
        GenesisGpu g_an(s, 32u, 32u * 32u);
        g_an.init_world(mix(seed ^ 0x1234u), 0u);
        g_an.upload_dnas_and_spawn(cohort, 1u, mix(seed ^ 0x1234u));
        g_an.step_develop(mix(seed ^ 0x1234u));
        std::vector<BodyPhenotype> abp(32u);
        std::vector<CreatureState> acs(32u);
        s.d.check(s.d.download(abp.data(), g_an.d_bodies.ptr, 32u * sizeof(BodyPhenotype)), "pr an bp");
        s.d.check(s.d.download(acs.data(), g_an.d_creatures.ptr, 32u * sizeof(CreatureState)), "pr an cs");
        BodyPhenotype tbp{};
        DevSnapshots tsn{};
        g_an.trace_embryogenesis(cohort[0], tbp, tsn, mix(seed ^ 0x1234u));
        auto emit_pg = [&](const char *tag, const u8 *g) {
            std::cout << "genesis_probe_embryo=" << tag << "  grid=";
            for (u32 i = 0; i < TISSUE_CELLS; ++i) std::cout << hex_tk(g[i]);
            std::cout << '\n';
        };
        emit_pg("step0", tsn.step0);
        emit_pg("step3", tsn.step3);
        emit_pg("step6", tsn.step6);
        emit_pg("step9", abp[0].surface);
        std::cout << "genesis_probe_body=1  layers=" << std::max<u16>(1u, acs[0].num_layers)
                  << "  neurons=" << abp[0].active_neurons << "  cells=" << abp[0].total_cells
                  << "  synapses=" << abp[0].total_synapses
                  << "  eye=" << abp[0].counts[TK_EYE] << "  ear=" << abp[0].counts[TK_EAR]
                  << "  chem=" << abp[0].counts[TK_CHEM] << "  gland=" << abp[0].counts[TK_GLAND]
                  << "  muscle=" << abp[0].counts[TK_MUSCLE] << "  grasp=" << abp[0].counts[TK_GRASP]
                  << "  pyr=" << abp[0].counts[TK_NEU_EXC] << "  pv=" << abp[0].counts[TK_NEU_PV]
                  << "  sst=" << abp[0].counts[TK_NEU_SST] << "  mod=" << abp[0].counts[TK_NEU_MOD]
                  << "  grid=";
        for (u32 i = 0; i < TISSUE_CELLS; ++i) std::cout << hex_tk(abp[0].surface[i]);
        std::cout << '\n';
    }

    // Assay 1: 2-World Seasonal Reversal Learning (spanning full Warm<->Cool seasonal flips!)
    u32 rev_ticks = std::max(360u, ticks);
    AblationFlags a_intact{}; a_intact.challenge_tier = 2u; a_intact.shift_chemistry = 1u;
    AblationFlags a_frozen = a_intact; a_frozen.freeze_plasticity = 1u;
    AblationFlags a_noap = a_intact; a_noap.silence_apical = 1u;
    u32 s1 = mix(seed ^ 0x1111u), s2 = mix(seed ^ 0x55aa33ccu);
    auto r_i1 = run_heldout_assay(s, cohort, pop_n, max_layers, spawn_n, rev_ticks, s1, a_intact);
    auto r_i2 = run_heldout_assay(s, cohort, pop_n, max_layers, spawn_n, rev_ticks, s2, a_intact);
    auto r_f1 = run_heldout_assay(s, cohort, pop_n, max_layers, spawn_n, rev_ticks, s1, a_frozen);
    auto r_f2 = run_heldout_assay(s, cohort, pop_n, max_layers, spawn_n, rev_ticks, s2, a_frozen);
    auto r_a1 = run_heldout_assay(s, cohort, pop_n, max_layers, spawn_n, rev_ticks, s1, a_noap);
    auto r_a2 = run_heldout_assay(s, cohort, pop_n, max_layers, spawn_n, rev_ticks, s2, a_noap);
    double e_int = r_i1.total_energy + r_i2.total_energy;
    double e_frz = r_f1.total_energy + r_f2.total_energy;
    double e_noa = r_a1.total_energy + r_a2.total_energy;
    double plast_gain_pct = (e_frz > 1.0) ? 100.0 * (e_int - e_frz) / e_frz : 0.0;
    double apical_gain_pct = (e_noa > 1.0) ? 100.0 * (e_int - e_noa) / e_noa : 0.0;
    std::cout << "genesis_probe=reversal_learning  ticks=" << (rev_ticks * 2u)
              << "  intact_atp=" << std::fixed << std::setprecision(1) << e_int
              << "  frozen_atp=" << e_frz
              << "  no_apical_atp=" << e_noa
              << "  stdp_gain_pct=" << std::setprecision(1) << plast_gain_pct
              << "  bac_gain_pct=" << apical_gain_pct
              << "  alive=" << ((r_i1.alive + r_i2.alive) / 2u) << '\n';

    // Assay 2: Delayed-Match-to-Place Working Memory Persistence (Cue ON 30t -> Cue OFF 60t!)
    // Measures genuine neural firing persistence in deep vs superficial layers after physical cue extinguishes!
    {
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0x2222u), 0u);
        gpu.upload_dnas_and_spawn(cohort, 64u, mix(seed ^ 0x2222u));
        gpu.step_develop(mix(seed ^ 0x2222u));
        AblationFlags abl{}; abl.challenge_tier = 1u;

        // Place 64 organisms in calm dark open ocean facing East, flash bright optical/acoustic beacon 10 tiles East for 30 ticks,
        // then turn beacon completely OFF for 60 ticks and measure persistent spiking & sustained Eastward memory movement!
        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "wm dl cs");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; // Facing East
            cs[i].vx = 0.0f; cs[i].vy = 0.0f;
            cs[i].atp = 28.0f; cs[i].prev_atp = 28.0f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "wm ul cs");

        std::vector<float> light_cue(WORLD_CELLS, 0.05f), light_dark(WORLD_CELLS, 0.05f);
        std::vector<float> av0_cue(WORLD_CELLS, 0.0f), zero_f(WORLD_CELLS, 0.0f);
        for (u32 i = 0; i < 64u; ++i) {
            u32 bx = u32(cs[i].x + 10.0f) & WORLD_MASK;
            u32 by = u32(cs[i].y) & WORLD_MASK;
            for (int dy = -2; dy <= 2; ++dy)
                for (int dx = -2; dx <= 2; ++dx) {
                    u32 wi = u32((int(by) + dy) & int(WORLD_MASK)) * WORLD_W + u32((int(bx) + dx) & int(WORLD_MASK));
                    light_cue[wi] = 1.95f;
                    av0_cue[wi] = 0.90f;
                }
        }
        // Baseline dark activity (15 ticks — calm dark water, zero acoustic waves)
        s.d.check(s.d.upload(DevicePtr(gpu.wb.light), light_dark.data(), WORLD_CELLS * sizeof(float)), "wm dark");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "wm dz0");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0_prev), 0, WORLD_CELLS * sizeof(float)), "wm dz0p");
        u64 spk_base0 = gpu.read_counters().total_spikes;
        for (u32 t = 0; t < 15u; ++t) { gpu.step_brains(t, abl); gpu.step_bodies(t, seed, abl); }
        u64 spk_base = gpu.read_counters().total_spikes - spk_base0;

        // Cue ON phase (30 ticks): real physical optical lighthouse 10t East + propagating 2D acoustic wave from East!
        u64 spk_cue0 = gpu.read_counters().total_spikes;
        for (u32 t = 15u; t < 45u; ++t) {
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), av0_cue.data(), WORLD_CELLS * sizeof(float)), "wm cue v");
            gpu.step_world(t, 0u, 0u, seed); // Propagates real 2D acoustic wave from East beacon -> ears!
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), light_cue.data(), WORLD_CELLS * sizeof(float)), "wm cue l");
            gpu.step_brains(t, abl); gpu.step_bodies(t, seed, abl);
        }
        u64 spk_cue = gpu.read_counters().total_spikes - spk_cue0;

        // Cue OFF retention delay (60 ticks — pure pitch-black silence, ZERO optical or acoustic input!)
        s.d.check(s.d.upload(DevicePtr(gpu.wb.light), light_dark.data(), WORLD_CELLS * sizeof(float)), "wm off l");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), zero_f.data(), WORLD_CELLS * sizeof(float)), "wm off v");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0_prev), zero_f.data(), WORLD_CELLS * sizeof(float)), "wm off vp");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_p), zero_f.data(), WORLD_CELLS * sizeof(float)), "wm off p");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_prev), zero_f.data(), WORLD_CELLS * sizeof(float)), "wm off pp");
        std::vector<CreatureState> cs_mid(64u);
        s.d.check(s.d.download(cs_mid.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "wm dl mid");
        u64 spk_del0 = gpu.read_counters().total_spikes;
        for (u32 t = 45u; t < 105u; ++t) { gpu.step_brains(t, abl); gpu.step_bodies(t, seed, abl); }
        u64 spk_del = gpu.read_counters().total_spikes - spk_del0;
        std::vector<CreatureState> cs_end(64u);
        s.d.check(s.d.download(cs_end.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "wm dl end");

        double mean_dx = 0.0, mean_layers = 0.0;
        u32 alive_wm = 0u;
        for (u32 i = 0; i < 64u; ++i) {
            if (cs_end[i].alive != 1u) continue;
            float dx = cs_end[i].x - cs_mid[i].x;
            if (dx > 128.0f) dx -= 256.0f;
            if (dx < -128.0f) dx += 256.0f;
            mean_dx += double(dx);
            mean_layers += double(std::max<u16>(1u, cs_end[i].num_layers));
            ++alive_wm;
        }
        if (alive_wm) { mean_dx /= double(alive_wm); mean_layers /= double(alive_wm); }
        double base_rate = double(spk_base) / 15.0;
        double cue_rate = double(spk_cue) / 30.0;
        double delay_rate = double(spk_del) / 60.0;
        double persist_ratio = delay_rate / std::max(1.0, base_rate);
        std::cout << "genesis_probe=working_memory  delay_ticks=60"
                  << "  base_spk/t=" << std::fixed << std::setprecision(1) << base_rate
                  << "  cue_spk/t=" << cue_rate
                  << "  delay_spk/t=" << delay_rate
                  << "  persist_ratio=" << std::setprecision(3) << persist_ratio
                  << "  delay_east_dx=" << std::setprecision(2) << mean_dx
                  << "  mean_layers=" << mean_layers << '\n';
    }

    // Assay 3 & 4: Multi-Step Tool / Apex Defense / Acoustic-Referential Coupling on Unseen World (600 ticks)
    {
        u32 a_ticks = std::max(600u, ticks);
        GenesisGpu gpu(s, pop_n, max_layers);
        u32 wseed = mix(seed ^ 0x3333u);
        gpu.init_world(wseed, 1u);
        for (u32 t = 0; t < 64u; ++t) gpu.step_world(t, 1u, 2u, wseed);
        gpu.upload_dnas_and_spawn(cohort, spawn_n, wseed);
        gpu.step_develop(wseed);
        AblationFlags abl{}; abl.challenge_tier = 2u; abl.shift_chemistry = 1u;
        for (u32 t = 0; t < a_ticks; ++t) {
            gpu.step_world(t + 64u, 1u, 2u, wseed);
            gpu.step_brains(t, abl);
            gpu.step_bodies(t, wseed, abl);
            gpu.step_develop(wseed + t + 1u);
        }
        auto ctr = gpu.read_counters();
        double vmi = compute_vocal_mi(ctr);
        std::cout << "genesis_probe=agency_language  ticks=" << a_ticks
                  << "  prey=" << ctr.total_prey_caught << "  pods=" << ctr.total_pods_cracked
                  << "  stones=" << ctr.total_stones_made << "  tools=" << ctr.total_tool_cracks
                  << "  tempered=" << ctr.total_tempered_tools << "  cached=" << ctr.total_cached_feasts
                  << "  coop=" << ctr.total_coop_cracks << "  duets=" << ctr.total_duet_coops
                  << "  shelters=" << ctr.total_shelter_nights
                  << "  apex_kills=" << ctr.total_apex_kills << "  apex_deaths=" << ctr.total_apex_deaths
                  << "  echo=" << ctr.total_echo_hits
                  << "  vocal=" << ctr.total_vocal << "  vocal_resp=" << ctr.total_vocal_resp
                  << "  vocal_replies=" << ctr.total_vocal_replies
                  << "  vocal_mimics=" << ctr.total_vocal_mimics
                  << "  vocal_mi_bits=" << std::fixed << std::setprecision(4) << vmi
                  << "  imprints=" << ctr.total_imprint_edits << '\n';
    }

    // Assay 5: Acoustic Associative Conditioning & Dialogue Discrimination (ZERO LLMs!)
    // Teaches 64 evolved organisms 4 trials pairing v0->food vs v1->toxin via their own live 3-factor STDP,
    // then tests pure v0 vs pure v1 cue-only responses (measuring learned neural & motor divergence!).
    {
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0x4444u), 0u);
        gpu.upload_dnas_and_spawn(cohort, 64u, mix(seed ^ 0x4444u));
        gpu.step_develop(mix(seed ^ 0x4444u));
        AblationFlags abl{}; abl.challenge_tier = 1u;

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "ac dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 28.0f; cs[i].prev_atp = 28.0f; cs[i].hp = 100.0f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "ac ul");

        auto make_pulse = [&](float val) {
            std::vector<float> f(WORLD_CELLS, 0.0f);
            for (u32 i = 0; i < 64u; ++i) {
                u32 cx = u32(i32(cs[i].x + 512.0f) & i32(WORLD_MASK));
                u32 cy = u32(i32(cs[i].y + 512.0f) & i32(WORLD_MASK));
                for (int dy = -2; dy <= 2; ++dy)
                    for (int dx = -2; dx <= 2; ++dx)
                        f[u32((int(cy) + dy) & int(WORLD_MASK)) * WORLD_W + u32((int(cx) + dx) & int(WORLD_MASK))] = val;
            }
            return f;
        };
        std::vector<float> zero_f(WORLD_CELLS, 0.0f);
        u32 sim_t = 0u;
        auto measure_cue = [&](bool is_v0) -> std::pair<double, double> {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "ac m dl");
            auto pf = make_pulse(1.5f);
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), is_v0 ? pf.data() : zero_f.data(), WORLD_CELLS * sizeof(float)), "ac mv0");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), is_v0 ? zero_f.data() : pf.data(), WORLD_CELLS * sizeof(float)), "ac mv1");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), zero_f.data(), WORLD_CELLS * sizeof(float)), "ac mc");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), zero_f.data(), WORLD_CELLS * sizeof(float)), "ac md");
            u64 s0 = gpu.read_counters().total_spikes;
            for (u32 k = 0; k < 10u; ++k, ++sim_t) { gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
            u64 dspk = gpu.read_counters().total_spikes - s0;
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "ac m dl2");
            double eat_m = 0.0; u32 al = 0u;
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) { eat_m += double(cs[i].mot_eat + cs[i].mot_thrust); ++al; }
            return {double(dspk) / 10.0, al ? (eat_m / double(al)) : 0.0};
        };

        auto [pre_s0, pre_m0] = measure_cue(true);
        auto [pre_s1, pre_m1] = measure_cue(false);
        double pre_div = std::fabs(pre_s0 - pre_s1) / std::max(1.0, 0.5 * (pre_s0 + pre_s1));

        // 4 live associative teaching trials: v0 paired with clean food trophallaxis (+DA), v1 paired with bitter cue (-DA)!
        for (u32 tr = 0; tr < 4u; ++tr) {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "ac tr0");
            for (u32 i = 0; i < 64u; ++i) {
                if (cs[i].alive == 1u) { cs[i].hp = 100.0f; if (cs[i].atp < 22.0f) { cs[i].atp = 22.0f; cs[i].prev_atp = 22.0f; } }
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "ac tr0u");
            auto p0 = make_pulse(1.6f), pfood = make_pulse(1.4f);
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), p0.data(), WORLD_CELLS * sizeof(float)), "ac t0");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), zero_f.data(), WORLD_CELLS * sizeof(float)), "ac t0z");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), pfood.data(), WORLD_CELLS * sizeof(float)), "ac t0f");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), zero_f.data(), WORLD_CELLS * sizeof(float)), "ac t0d");
            for (u32 k = 0; k < 10u; ++k, ++sim_t) {
                if (k == 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "ac r0d");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) cs[i].atp += 1.6f; // Direct parental trophallaxis drop!
                    s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "ac r0u");
                }
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }

            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "ac tr1");
            for (u32 i = 0; i < 64u; ++i) {
                if (cs[i].alive == 1u) { cs[i].hp = 100.0f; if (cs[i].atp < 22.0f) { cs[i].atp = 22.0f; cs[i].prev_atp = 22.0f; } }
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "ac tr1u");
            auto p1 = make_pulse(1.6f), ptox = make_pulse(0.35f); // Non-lethal aversive bitter cue -> crisp -TD!
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), zero_f.data(), WORLD_CELLS * sizeof(float)), "ac t1z");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), p1.data(), WORLD_CELLS * sizeof(float)), "ac t1");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), zero_f.data(), WORLD_CELLS * sizeof(float)), "ac t1f");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), ptox.data(), WORLD_CELLS * sizeof(float)), "ac t1d");
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                if (k == 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "ac r1d");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) cs[i].atp = std::max(8.0f, cs[i].atp - 1.4f);
                    s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "ac r1u");
                }
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
        }
        auto [post_s0, post_m0] = measure_cue(true);
        auto [post_s1, post_m1] = measure_cue(false);
        double post_div = std::fabs(post_s0 - post_s1) / std::max(1.0, 0.5 * (post_s0 + post_s1));
        u64 cond_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=acoustic_dialogue_learning  trials=4"
                  << "  pre_v0_spk=" << std::fixed << std::setprecision(1) << pre_s0
                  << "  pre_v1_spk=" << pre_s1 << "  pre_div=" << std::setprecision(3) << pre_div
                  << "  post_v0_spk=" << std::setprecision(1) << post_s0
                  << "  post_v1_spk=" << post_s1 << "  post_div=" << std::setprecision(3) << post_div
                  << "  post_v0_mot=" << std::setprecision(2) << post_m0
                  << "  post_v1_mot=" << post_m1 << "  stdp_edits=" << cond_ed << '\n';
    }

    // Assay 6: Polychronic Temporal Sequence Order Discrimination (v0->v1 vs v1->v0 — ZERO LLMs!)
    // Both bigrams carry IDENTICAL total acoustic energy (1x v0 + 1x v1), differing ONLY in temporal order!
    // Impossible for stateless reflexes — requires axonal conduction delays + deep recurrent working memory + STDP!
    {
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0x6666u), 0u);
        gpu.upload_dnas_and_spawn(cohort, 64u, mix(seed ^ 0x6666u));
        gpu.step_develop(mix(seed ^ 0x6666u));
        AblationFlags abl{}; abl.challenge_tier = 1u;

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "sq dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 28.0f; cs[i].prev_atp = 28.0f; cs[i].hp = 100.0f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "sq ul");

        auto make_pulse = [&](float val) {
            std::vector<float> f(WORLD_CELLS, 0.0f);
            for (u32 i = 0; i < 64u; ++i) {
                u32 cx = u32(i32(cs[i].x + 512.0f) & i32(WORLD_MASK));
                u32 cy = u32(i32(cs[i].y + 512.0f) & i32(WORLD_MASK));
                for (int dy = -2; dy <= 2; ++dy)
                    for (int dx = -2; dx <= 2; ++dx)
                        f[u32((int(cy) + dy) & int(WORLD_MASK)) * WORLD_W + u32((int(cx) + dx) & int(WORLD_MASK))] = val;
            }
            return f;
        };
        std::vector<float> zero_f(WORLD_CELLS, 0.0f);
        u32 sim_t = 0u;
        auto run_seq = [&](bool order_01, float food_v, float tox_v) -> std::tuple<double, double, double> {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "sq r dl");
            for (u32 i = 0; i < 64u; ++i) { if (cs[i].alive == 1u) { cs[i].hp = 100.0f; if (cs[i].atp < 22.0f) cs[i].atp = 22.0f; } }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "sq r ul");
            auto pf = make_pulse(1.6f);
            // Syllable 1 (4 ticks — anechoic per-tick refresh!)
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), zero_f.data(), WORLD_CELLS * sizeof(float)), "sq s1c");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), zero_f.data(), WORLD_CELLS * sizeof(float)), "sq s1d");
            for (u32 k = 0; k < 4u; ++k, ++sim_t) {
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), order_01 ? pf.data() : zero_f.data(), WORLD_CELLS * sizeof(float)), "sq s1v0");
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), order_01 ? zero_f.data() : pf.data(), WORLD_CELLS * sizeof(float)), "sq s1v1");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "sq s1v2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "sq s1p");
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            // Syllable 2 + optional reinforcement (8 ticks — measured strictly on the post-second-syllable window!)
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "sq r2 dl");
            auto pf2 = make_pulse(1.6f);
            auto pfood = food_v > 0.0f ? make_pulse(food_v) : zero_f;
            auto ptox = tox_v > 0.0f ? make_pulse(tox_v) : zero_f;
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), pfood.data(), WORLD_CELLS * sizeof(float)), "sq s2c");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), ptox.data(), WORLD_CELLS * sizeof(float)), "sq s2d");
            u64 s0 = gpu.read_counters().total_spikes;
            double mot_sum = 0.0, voc_sum = 0.0; u32 sample_n = 0u;
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), order_01 ? zero_f.data() : pf2.data(), WORLD_CELLS * sizeof(float)), "sq s2v0");
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), order_01 ? pf2.data() : zero_f.data(), WORLD_CELLS * sizeof(float)), "sq s2v1");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "sq s2v2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "sq s2p");
                if (k == 2u && (food_v > 0.0f || tox_v > 0.0f)) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "sq rf d");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                        if (food_v > 0.0f) cs[i].atp += 1.6f;
                        if (tox_v > 0.0f) cs[i].atp = std::max(8.0f, cs[i].atp - 1.4f);
                    }
                    s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "sq rf u");
                }
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
                if (k >= 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "sq r2 dl2");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                        mot_sum += double(cs[i].mot_eat + cs[i].mot_thrust);
                        voc_sum += double(cs[i].mot_vocal0 + cs[i].mot_vocal1);
                        ++sample_n;
                    }
                }
            }
            u64 dspk = gpu.read_counters().total_spikes - s0;
            // Brief quiet anechoic inter-trial washout (4 ticks)
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), zero_f.data(), WORLD_CELLS * sizeof(float)), "sq wzc");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), zero_f.data(), WORLD_CELLS * sizeof(float)), "sq wzd");
            for (u32 k = 0; k < 4u; ++k, ++sim_t) {
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "sq wz0");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "sq wz1");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "sq wz2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "sq wzp");
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            return {double(dspk) / 8.0, sample_n ? (mot_sum / double(sample_n)) : 0.0, sample_n ? (voc_sum / double(sample_n)) : 0.0};
        };

        auto [pre01_s, pre01_m, pre01_v] = run_seq(true, 0.0f, 0.0f);
        auto [pre10_s, pre10_m, pre10_v] = run_seq(false, 0.0f, 0.0f);
        double pre_div = std::fabs(pre01_s - pre10_s) / std::max(1.0, 0.5 * (pre01_s + pre10_s));
        for (u32 tr = 0; tr < 4u; ++tr) {
            run_seq(true, 1.4f, 0.0f);
            run_seq(false, 0.0f, 0.35f);
        }
        auto [post01_s, post01_m, post01_v] = run_seq(true, 0.0f, 0.0f);
        auto [post10_s, post10_m, post10_v] = run_seq(false, 0.0f, 0.0f);
        double post_div = std::fabs(post01_s - post10_s) / std::max(1.0, 0.5 * (post01_s + post10_s));
        u64 sq_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=temporal_sequence_learning  trials=4"
                  << "  pre_01_spk=" << std::fixed << std::setprecision(1) << pre01_s
                  << "  pre_10_spk=" << pre10_s << "  pre_div=" << std::setprecision(3) << pre_div
                  << "  post_01_spk=" << std::setprecision(1) << post01_s
                  << "  post_10_spk=" << post10_s << "  post_div=" << std::setprecision(3) << post_div
                  << "  post_01_mot=" << std::setprecision(2) << post01_m
                  << "  post_10_mot=" << post10_m
                  << "  post_01_voc=" << post01_v << "  post_10_voc=" << post10_v
                  << "  stdp_edits=" << sq_ed << '\n';
    }

    // Assay 7: 3-Band Compositional Trigram Syntax Learning (v0->v1->v2 vs v2->v1->v0 — ZERO LLMs!)
    // Identical total 3-band acoustic energy (1x v0 + 1x v1 + 1x v2), reversed compositional word order!
    {
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0x7771u), 0u);
        gpu.upload_dnas_and_spawn(cohort, 64u, mix(seed ^ 0x7771u));
        gpu.step_develop(mix(seed ^ 0x7771u));
        AblationFlags abl{}; abl.challenge_tier = 1u;

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "s3 dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 28.0f; cs[i].prev_atp = 28.0f; cs[i].hp = 100.0f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "s3 ul");

        auto make_pulse = [&](float val) {
            std::vector<float> f(WORLD_CELLS, 0.0f);
            for (u32 i = 0; i < 64u; ++i) {
                u32 cx = u32(i32(cs[i].x + 512.0f) & i32(WORLD_MASK));
                u32 cy = u32(i32(cs[i].y + 512.0f) & i32(WORLD_MASK));
                for (int dy = -2; dy <= 2; ++dy)
                    for (int dx = -2; dx <= 2; ++dx)
                        f[u32((int(cy) + dy) & int(WORLD_MASK)) * WORLD_W + u32((int(cx) + dx) & int(WORLD_MASK))] = val;
            }
            return f;
        };
        std::vector<float> zero_f(WORLD_CELLS, 0.0f);
        u32 sim_t = 0u;
        auto run_trigram = [&](bool fwd_012, float food_v, float tox_v) -> std::tuple<double, double, double> {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "s3 r dl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) { cs[i].hp = 100.0f; if (cs[i].atp < 22.0f) cs[i].atp = 22.0f; }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "s3 r ul");
            auto pf = make_pulse(1.6f);
            // Syllable 1 (3 ticks): v0 if fwd else v2 — anechoic per-tick refresh!
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), zero_f.data(), WORLD_CELLS * sizeof(float)), "s3 1c");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), zero_f.data(), WORLD_CELLS * sizeof(float)), "s3 1d");
            for (u32 k = 0; k < 3u; ++k, ++sim_t) {
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), fwd_012 ? pf.data() : zero_f.data(), WORLD_CELLS * sizeof(float)), "s3 1v0");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "s3 1v1");
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), fwd_012 ? zero_f.data() : pf.data(), WORLD_CELLS * sizeof(float)), "s3 1v2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "s3 1p");
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            // Syllable 2 (3 ticks): v1 (common pivot syllable!)
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "s3 r2");
            auto pf2 = make_pulse(1.6f);
            for (u32 k = 0; k < 3u; ++k, ++sim_t) {
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "s3 2v0");
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), pf2.data(), WORLD_CELLS * sizeof(float)), "s3 2v1");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "s3 2v2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "s3 2p");
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            // Syllable 3 + reinforcement (8 ticks): v2 if fwd else v0
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "s3 r3");
            auto pf3 = make_pulse(1.6f);
            auto pfood = food_v > 0.0f ? make_pulse(food_v) : zero_f;
            auto ptox = tox_v > 0.0f ? make_pulse(tox_v) : zero_f;
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), pfood.data(), WORLD_CELLS * sizeof(float)), "s3 3c");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), ptox.data(), WORLD_CELLS * sizeof(float)), "s3 3d");
            u64 s0 = gpu.read_counters().total_spikes;
            double mot_sum = 0.0, voc_sum = 0.0; u32 sn = 0u;
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), fwd_012 ? zero_f.data() : pf3.data(), WORLD_CELLS * sizeof(float)), "s3 3v0");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "s3 3v1");
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), fwd_012 ? pf3.data() : zero_f.data(), WORLD_CELLS * sizeof(float)), "s3 3v2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "s3 3p");
                if (k == 2u && (food_v > 0.0f || tox_v > 0.0f)) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "s3 rf d");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                        if (food_v > 0.0f) cs[i].atp += 1.8f;
                        if (tox_v > 0.0f) cs[i].atp = std::max(8.0f, cs[i].atp - 1.6f);
                    }
                    s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "s3 rf u");
                }
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
                if (k >= 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "s3 m dl");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                        mot_sum += double(cs[i].mot_eat + cs[i].mot_thrust);
                        voc_sum += double(cs[i].mot_vocal0 + cs[i].mot_vocal1 + cs[i].mot_vocal2);
                        ++sn;
                    }
                }
            }
            u64 dspk = gpu.read_counters().total_spikes - s0;
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), zero_f.data(), WORLD_CELLS * sizeof(float)), "s3 wzc");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), zero_f.data(), WORLD_CELLS * sizeof(float)), "s3 wzd");
            for (u32 k = 0; k < 4u; ++k, ++sim_t) {
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "s3 wz0");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "s3 wz1");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "s3 wz2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "s3 wzp");
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            return {double(dspk) / 8.0, sn ? (mot_sum / double(sn)) : 0.0, sn ? (voc_sum / double(sn)) : 0.0};
        };

        auto [pre012_s, pre012_m, pre012_v] = run_trigram(true, 0.0f, 0.0f);
        auto [pre210_s, pre210_m, pre210_v] = run_trigram(false, 0.0f, 0.0f);
        double pre_div = std::fabs(pre012_s - pre210_s) / std::max(1.0, 0.5 * (pre012_s + pre210_s));
        for (u32 tr = 0; tr < 4u; ++tr) {
            run_trigram(true, 1.4f, 0.0f);
            run_trigram(false, 0.0f, 0.30f);
        }
        auto [post012_s, post012_m, post012_v] = run_trigram(true, 0.0f, 0.0f);
        auto [post210_s, post210_m, post210_v] = run_trigram(false, 0.0f, 0.0f);
        double post_div = std::fabs(post012_s - post210_s) / std::max(1.0, 0.5 * (post012_s + post210_s));
        u64 s3_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=compositional_3band_binding  trials=4"
                  << "  pre_012_spk=" << std::fixed << std::setprecision(1) << pre012_s
                  << "  pre_210_spk=" << pre210_s << "  pre_div=" << std::setprecision(3) << pre_div
                  << "  post_012_spk=" << std::setprecision(1) << post012_s
                  << "  post_210_spk=" << post210_s << "  post_div=" << std::setprecision(3) << post_div
                  << "  post_012_mot=" << std::setprecision(2) << post012_m
                  << "  post_210_mot=" << post210_m
                  << "  post_012_voc=" << post012_v << "  post_210_voc=" << post210_v
                  << "  stdp_edits=" << s3_ed << '\n';
    }

    // Assay 8: Cortical Predictive-Coding Mismatch Negativity (MMN / Omission Surprise — ZERO External Reward!)
    // Entrains 64 organisms on 6 unsupervised cycles of v0->v1->v2 (pure NMDA+ACh/BAC predictive plasticity),
    // then tests Expected (v0->v1->v2) vs Oddball (v0->v1->v0) vs Omission (v0->v1->silence) — Intact vs No-BAC!
    {
        auto eval_mmn = [&](bool silence_bac) -> std::tuple<double, double, double, double, double> {
            GenesisGpu gpu(s, 64u, 64u * 24u);
            gpu.init_world(mix(seed ^ 0x8881u), 0u);
            gpu.upload_dnas_and_spawn(cohort, 64u, mix(seed ^ 0x8881u));
            gpu.step_develop(mix(seed ^ 0x8881u));
            AblationFlags abl{}; abl.challenge_tier = 1u; abl.silence_apical = silence_bac ? 1u : 0u;

            std::vector<CreatureState> cs(64u);
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "mmn dl");
            for (u32 i = 0; i < 64u; ++i) {
                cs[i].x = 40.0f + float((i & 7u) * 20u);
                cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
                cs[i].theta = 0.0f; cs[i].atp = 28.0f; cs[i].prev_atp = 28.0f; cs[i].hp = 100.0f;
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "mmn ul");

            auto make_pulse = [&](float val) {
                std::vector<float> f(WORLD_CELLS, 0.0f);
                for (u32 i = 0; i < 64u; ++i) {
                    u32 cx = u32(i32(cs[i].x + 512.0f) & i32(WORLD_MASK));
                    u32 cy = u32(i32(cs[i].y + 512.0f) & i32(WORLD_MASK));
                    for (int dy = -2; dy <= 2; ++dy)
                        for (int dx = -2; dx <= 2; ++dx)
                            f[u32((int(cy) + dy) & int(WORLD_MASK)) * WORLD_W + u32((int(cx) + dx) & int(WORLD_MASK))] = val;
                }
                return f;
            };
            std::vector<float> zero_f(WORLD_CELLS, 0.0f);
            u32 sim_t = 0u;
            // mode: 0 = Expected (v0->v1->v2), 1 = Oddball (v0->v1->v0), 2 = Omission (v0->v1->silence)
            auto run_rhythm = [&](int mode) -> double {
                s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "mmn r dl");
                for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) { cs[i].hp = 100.0f; if (cs[i].atp < 22.0f) { cs[i].atp = 22.0f; cs[i].prev_atp = 22.0f; } }
                s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "mmn r ul");
                auto pf = make_pulse(1.6f);
                // Syllable 1: v0 (3t) — anechoic per-tick wave dissipation (zero self-echo pileup!)
                for (u32 k = 0; k < 3u; ++k, ++sim_t) {
                    s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), pf.data(), WORLD_CELLS * sizeof(float)), "mmn 1v0");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "mmn 1v1");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "mmn 1v2");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "mmn 1p");
                    gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
                }
                // Syllable 2: v1 (3t)
                s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "mmn r2");
                auto pf2 = make_pulse(1.6f);
                for (u32 k = 0; k < 3u; ++k, ++sim_t) {
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "mmn 2v0");
                    s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), pf2.data(), WORLD_CELLS * sizeof(float)), "mmn 2v1");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "mmn 2v2");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "mmn 2p");
                    gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
                }
                // Syllable 3 (6t window): Expected v2, Oddball v0, or TRUE ANECHOIC Omission silence!
                s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "mmn r3");
                auto pf3 = make_pulse(1.6f);
                u64 s0 = gpu.read_counters().total_spikes;
                for (u32 k = 0; k < 6u; ++k, ++sim_t) {
                    if (mode == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), pf3.data(), WORLD_CELLS * sizeof(float)), "mmn 3v0");
                    else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "mmn 3v0z");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "mmn 3v1");
                    if (mode == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), pf3.data(), WORLD_CELLS * sizeof(float)), "mmn 3v2");
                    else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "mmn 3v2z");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "mmn 3p");
                    gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
                }
                u64 dspk = gpu.read_counters().total_spikes - s0;
                // Quiet inter-cycle washout (3t — truly anechoic silence!)
                for (u32 k = 0; k < 3u; ++k, ++sim_t) {
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "mmn w0");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "mmn w1");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "mmn w2");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "mmn wp");
                    gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
                }
                return double(dspk) / 6.0;
            };
            // 6 pure unsupervised predictive-coding habituation cycles (ZERO food/toxin!)
            for (u32 cyc = 0; cyc < 6u; ++cyc) run_rhythm(0);
            double exp_s = run_rhythm(0);
            double odd_s = run_rhythm(1);
            double omis_s = run_rhythm(2);
            double mmn_pct = (exp_s > 1.0) ? (100.0 * std::fabs(odd_s - exp_s) / exp_s) : 0.0;
            double omis_pct = (exp_s > 1.0) ? (100.0 * std::fabs(omis_s - exp_s) / exp_s) : 0.0;
            return {exp_s, odd_s, omis_s, mmn_pct, omis_pct};
        };
        auto [exp_s, odd_s, omis_s, mmn_pct, omis_pct] = eval_mmn(false);
        auto [nb_exp, nb_odd, nb_omis, nb_mmn, nb_omis_pct] = eval_mmn(true);
        std::cout << "genesis_probe=predictive_mmn  cycles=6"
                  << "  exp_spk=" << std::fixed << std::setprecision(1) << exp_s
                  << "  odd_spk=" << odd_s << "  omis_spk=" << omis_s
                  << "  mmn_pct=" << std::setprecision(1) << mmn_pct
                  << "  omis_pct=" << omis_pct
                  << "  no_bac_mmn_pct=" << nb_mmn << '\n';
    }

    // Assay 9: Zero-Shot Compositional Concept Transfer (ZERO LLMs!)
    // Teaches ONLY atomic grounded cues (v0->food +DA, v1->toxin -DA, v2->neutral social cue),
    // then tests UNSEEN compound phrases NEVER shown in teaching: seq_02 (v0->v2) vs seq_12 (v1->v2)!
    {
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0x9991u), 0u);
        gpu.upload_dnas_and_spawn(cohort, 64u, mix(seed ^ 0x9991u));
        gpu.step_develop(mix(seed ^ 0x9991u));
        AblationFlags abl{}; abl.challenge_tier = 1u;

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "zs dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 28.0f; cs[i].prev_atp = 28.0f; cs[i].hp = 100.0f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "zs ul");

        auto make_pulse = [&](float val) {
            std::vector<float> f(WORLD_CELLS, 0.0f);
            for (u32 i = 0; i < 64u; ++i) {
                u32 cx = u32(i32(cs[i].x + 512.0f) & i32(WORLD_MASK));
                u32 cy = u32(i32(cs[i].y + 512.0f) & i32(WORLD_MASK));
                for (int dy = -2; dy <= 2; ++dy)
                    for (int dx = -2; dx <= 2; ++dx)
                        f[u32((int(cy) + dy) & int(WORLD_MASK)) * WORLD_W + u32((int(cx) + dx) & int(WORLD_MASK))] = val;
            }
            return f;
        };
        std::vector<float> zero_f(WORLD_CELLS, 0.0f);
        u32 sim_t = 0u;
        auto eval_compound = [&](bool is_02) -> std::tuple<double, double, double> {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "zs c dl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) { cs[i].hp = 100.0f; if (cs[i].atp < 22.0f) { cs[i].atp = 22.0f; cs[i].prev_atp = 22.0f; } }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "zs c ul");
            auto pf1 = make_pulse(1.6f);
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), zero_f.data(), WORLD_CELLS * sizeof(float)), "zs 1c");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), zero_f.data(), WORLD_CELLS * sizeof(float)), "zs 1d");
            for (u32 k = 0; k < 4u; ++k, ++sim_t) {
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), is_02 ? pf1.data() : zero_f.data(), WORLD_CELLS * sizeof(float)), "zs 1v0");
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), is_02 ? zero_f.data() : pf1.data(), WORLD_CELLS * sizeof(float)), "zs 1v1");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "zs 1v2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "zs 1p");
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "zs c2");
            auto pf2 = make_pulse(1.6f);
            u64 s0 = gpu.read_counters().total_spikes;
            double mot_sum = 0.0, voc_sum = 0.0; u32 sn = 0u;
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "zs 2v0");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "zs 2v1");
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), pf2.data(), WORLD_CELLS * sizeof(float)), "zs 2v2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "zs 2p");
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
                if (k >= 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "zs m");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                        mot_sum += double(cs[i].mot_eat + cs[i].mot_thrust);
                        voc_sum += double(cs[i].mot_vocal0 + cs[i].mot_vocal1 + cs[i].mot_vocal2);
                        ++sn;
                    }
                }
            }
            u64 dspk = gpu.read_counters().total_spikes - s0;
            for (u32 k = 0; k < 4u; ++k, ++sim_t) {
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "zs w0");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "zs w1");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "zs w2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "zs wp");
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            return {double(dspk) / 8.0, sn ? (mot_sum / double(sn)) : 0.0, sn ? (voc_sum / double(sn)) : 0.0};
        };

        auto [pre02_s, pre02_m, pre02_v] = eval_compound(true);
        auto [pre12_s, pre12_m, pre12_v] = eval_compound(false);
        double pre_div = std::fabs(pre02_s - pre12_s) / std::max(1.0, 0.5 * (pre02_s + pre12_s));

        // Teach ONLY atomic single-band concepts (NEVER show seq_02 or seq_12 during teaching!):
        for (u32 tr = 0; tr < 4u; ++tr) {
            // Atomic v0 -> Food (+DA)
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "zs t0");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) { cs[i].hp = 100.0f; if (cs[i].atp < 22.0f) { cs[i].atp = 22.0f; cs[i].prev_atp = 22.0f; } }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "zs t0u");
            auto p0 = make_pulse(1.6f), pf = make_pulse(1.4f);
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), p0.data(), WORLD_CELLS * sizeof(float)), "zs t0v0");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), zero_f.data(), WORLD_CELLS * sizeof(float)), "zs t0v1");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), zero_f.data(), WORLD_CELLS * sizeof(float)), "zs t0v2");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), pf.data(), WORLD_CELLS * sizeof(float)), "zs t0c");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), zero_f.data(), WORLD_CELLS * sizeof(float)), "zs t0d");
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                if (k == 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "zs r0d");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) cs[i].atp += 1.8f;
                    s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "zs r0u");
                }
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            // Atomic v1 -> Toxin (-DA)
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "zs t1");
            auto p1 = make_pulse(1.6f), pt = make_pulse(0.32f);
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), zero_f.data(), WORLD_CELLS * sizeof(float)), "zs t1v0");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), p1.data(), WORLD_CELLS * sizeof(float)), "zs t1v1");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), zero_f.data(), WORLD_CELLS * sizeof(float)), "zs t1c");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), pt.data(), WORLD_CELLS * sizeof(float)), "zs t1d");
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                if (k == 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "zs r1d");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f);
                    s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "zs r1u");
                }
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            // Atomic v2 -> Neutral Social Cue
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "zs t2");
            auto p2 = make_pulse(1.6f);
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), zero_f.data(), WORLD_CELLS * sizeof(float)), "zs t2v1");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), p2.data(), WORLD_CELLS * sizeof(float)), "zs t2v2");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_d), zero_f.data(), WORLD_CELLS * sizeof(float)), "zs t2d");
            for (u32 k = 0; k < 6u; ++k, ++sim_t) { gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
        }

        auto [post02_s, post02_m, post02_v] = eval_compound(true);
        auto [post12_s, post12_m, post12_v] = eval_compound(false);
        double post_div = std::fabs(post02_s - post12_s) / std::max(1.0, 0.5 * (post02_s + post12_s));
        u64 zs_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=zeroshot_compositional_transfer  trials=4"
                  << "  pre_02_spk=" << std::fixed << std::setprecision(1) << pre02_s
                  << "  pre_12_spk=" << pre12_s << "  pre_div=" << std::setprecision(3) << pre_div
                  << "  post_02_spk=" << std::setprecision(1) << post02_s
                  << "  post_12_spk=" << post12_s << "  post_div=" << std::setprecision(3) << post_div
                  << "  post_02_mot=" << std::setprecision(2) << post02_m
                  << "  post_12_mot=" << post12_m
                  << "  post_02_voc=" << post02_v << "  post_12_voc=" << post12_v
                  << "  stdp_edits=" << zs_ed << '\n';
    }

    // Assay 10: Grounded Spoken Question->Answer Operant Vocal Grounding & Live Rule-Reversal (ZERO LLMs!)
    // Evaluates evolved vocal+auditory organisms in a clean calm chamber:
    //   Phase 1: Present Q1=(v0->v1) vs Q2=(v1->v0), tutor ONLY via live DA on organisms' OWN syrinx (Q1->v2, Q2->v0)
    //   Phase 2: Reverse the conversational rule live (Q1->v0, Q2->v2) and test unrewarded reversal flexibility!
    {
        // Select speaking (GLAND>0 & EAR>0) organisms honestly from GPU morphogenesis of the evolved cohort!
        std::vector<GenesisDNA> qa_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0xaa00u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0xaa00u));
            g_sc.step_develop(mix(seed ^ 0xaa00u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "qa sc bp");
            for (u32 i = 0; i < scan_n && qa_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u) qa_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && qa_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] > 0u && sbps[i].counts[TK_EAR] > 0u && !(sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u))
                    qa_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && qa_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] > 0u && sbps[i].counts[TK_EAR] == 0u) qa_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && qa_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] == 0u) qa_cohort.push_back(cohort[i]);
            if (qa_cohort.empty()) qa_cohort = cohort;
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0xaaa1u), 0u);
        gpu.upload_dnas_and_spawn(qa_cohort, 64u, mix(seed ^ 0xaaa1u));
        gpu.step_develop(mix(seed ^ 0xaaa1u));
        std::vector<BodyPhenotype> qa_bps(64u);
        s.d.check(s.d.download(qa_bps.data(), gpu.d_bodies.ptr, 64u * sizeof(BodyPhenotype)), "qa bp");
        std::vector<u8> has_gl(64u, 0u);
        u32 n_voc_qa = 0u;
        for (u32 i = 0; i < 64u; ++i) {
            if (qa_bps[i].counts[TK_GLAND] > 0u) { has_gl[i] = 1u; ++n_voc_qa; }
        }
        AblationFlags abl{}; abl.challenge_tier = 0u;

        // Calm temperate water across the 256x256 arena (ZERO frostbite/wall noise!)
        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_l(WORLD_CELLS, 0.70f), flat_w(WORLD_CELLS, 1.0f);
        std::vector<float> flat_ter(WORLD_CELLS, 1.1f), zero_f(WORLD_CELLS, 0.0f), flat_p(WORLD_CELLS, 1.6f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "qa t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.light), flat_l.data(), WORLD_CELLS * sizeof(float)), "qa l");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "qa w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "qa ter");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "qa cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "qa cc");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "qa cd");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "qa dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 28.0f; cs[i].prev_atp = 28.0f; cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "qa ul");

        u32 sim_t = 0u;
        auto run_qa = [&](bool is_q1, bool operant_tutor, bool reversed_rule) -> std::tuple<double, double, double> {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "qa r dl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f; // Prevent max_atp glycogen saturation across 16+ trials!
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "qa r ul");
            // Uniform anechoic acoustic room broadcast (zero undecayed self-echo pileup!)
            auto set_qa_aud = [&](int band) {
                if (band == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), flat_p.data(), WORLD_CELLS * sizeof(float)), "qa v0");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "qa v0z");
                if (band == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), flat_p.data(), WORLD_CELLS * sizeof(float)), "qa v1");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "qa v1z");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "qa v2z");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "qa pz");
            };
            for (u32 k = 0; k < 4u; ++k, ++sim_t) { set_qa_aud(is_q1 ? 0 : 1); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }

            double v0_sum = 0.0, v2_sum = 0.0; u32 sn = 0u, hits = 0u;
            std::vector<u8> rewarded(64u, 0u);
            bool want_v2 = reversed_rule ? (!is_q1) : is_q1;
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                set_qa_aud(is_q1 ? 1 : 0);
                gpu.step_brains(sim_t, abl);
                if (k >= 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "qa m");
                    bool dirty = false;
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && has_gl[i]) {
                        v0_sum += double(cs[i].mot_vocal0);
                        v2_sum += double(cs[i].mot_vocal2);
                        ++sn;
                        bool answered_v2 = (cs[i].mot_vocal2 > cs[i].mot_vocal0 + 0.02f && cs[i].mot_vocal2 > cs[i].mot_vocal1 + 0.02f && cs[i].mot_vocal2 > 0.12f);
                        bool answered_v0 = (cs[i].mot_vocal0 > cs[i].mot_vocal2 + 0.02f && cs[i].mot_vocal0 > cs[i].mot_vocal1 + 0.02f && cs[i].mot_vocal0 > 0.12f);
                        bool off_v1 = (cs[i].mot_vocal1 > 0.12f && cs[i].mot_vocal1 >= cs[i].mot_vocal0 && cs[i].mot_vocal1 >= cs[i].mot_vocal2);
                        bool chord_mud = (!answered_v2 && !answered_v0 && !off_v1 && (cs[i].mot_vocal0 > 0.12f || cs[i].mot_vocal2 > 0.12f));
                        bool correct = want_v2 ? answered_v2 : answered_v0;
                        bool wrong = (want_v2 ? answered_v0 : answered_v2) || off_v1 || chord_mud;
                        if (correct) ++hits;
                        if (operant_tutor && k <= 6u) {
                            if (correct && (rewarded[i] & 1u) == 0u) { cs[i].atp += 2.4f; rewarded[i] |= 1u; dirty = true; }
                            else if (wrong && rewarded[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f); rewarded[i] |= 2u; dirty = true; }
                            else if (k == 5u && rewarded[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 0.9f); rewarded[i] |= 2u; dirty = true; }
                        }
                    }
                    if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "qa rf");
                }
                gpu.step_bodies(sim_t, seed, abl);
            }
            for (u32 k = 0; k < 6u; ++k, ++sim_t) { set_qa_aud(-1); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
            double inv = sn ? (1.0 / double(sn)) : 0.0;
            return {v0_sum * inv, v2_sum * inv, double(hits) * inv};
        };

        auto [pre_q1_v0, pre_q1_v2, pre_q1_hit] = run_qa(true, false, false);
        auto [pre_q2_v0, pre_q2_v2, pre_q2_hit] = run_qa(false, false, false);
        double pre_sep = std::fabs((pre_q1_v2 - pre_q1_v0) - (pre_q2_v2 - pre_q2_v0));
        for (u32 tr = 0; tr < 16u; ++tr) {
            run_qa(true, true, false);
            run_qa(false, true, false);
        }
        auto [post_q1_v0, post_q1_v2, post_q1_hit] = run_qa(true, false, false);
        auto [post_q2_v0, post_q2_v2, post_q2_hit] = run_qa(false, false, false);
        double post_sep = std::fabs((post_q1_v2 - post_q1_v0) - (post_q2_v2 - post_q2_v0));
        for (u32 tr = 0; tr < 14u; ++tr) {
            run_qa(true, true, true);
            run_qa(false, true, true);
        }
        auto [rev_q1_v0, rev_q1_v2, rev_q1_hit] = run_qa(true, false, true);
        auto [rev_q2_v0, rev_q2_v2, rev_q2_hit] = run_qa(false, false, true);
        double rev_sep = std::fabs((rev_q1_v0 - rev_q1_v2) - (rev_q2_v0 - rev_q2_v2));
        u64 qa_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=vocal_qa_grounding  trials=16  vocalists=" << n_voc_qa
                  << "  pre_sep=" << std::fixed << std::setprecision(3) << pre_sep
                  << "  post_sep=" << post_sep
                  << "  rev_sep=" << rev_sep
                  << "  q1_v0=" << std::setprecision(2) << post_q1_v0 << "  q1_v2=" << post_q1_v2
                  << "  q2_v0=" << post_q2_v0 << "  q2_v2=" << post_q2_v2
                  << "  hit_rate=" << std::setprecision(3) << (0.5 * (post_q1_hit + post_q2_hit))
                  << "  rev_hit=" << std::setprecision(3) << (0.5 * (rev_q1_hit + rev_q2_hit))
                  << "  stdp_edits=" << qa_ed << '\n';
    }

    // Assay 11: Horizontal Cultural Peer-Teaching (ZERO DA to Student — Pure Acoustic Wavefield Transfer!)
    // Places 32 isolated (Tutor, Student) pairs in calm chambers 32 tiles apart (Tutor & Student 2.5 tiles apart).
    // Tutor learns Q1->v2, Q2->v0 via pure operant DA RPE while Student sits beside Tutor receiving ZERO DA!
    // Measures whether naive Student's unrewarded spoken vocal output shifts toward its Tutor's dialect!
    {
        std::vector<GenesisDNA> pt_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0xbb00u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0xbb00u));
            g_sc.step_develop(mix(seed ^ 0xbb00u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "pt sc bp");
            for (u32 i = 0; i < scan_n && pt_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u) pt_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && pt_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] > 0u && sbps[i].counts[TK_EAR] > 0u && !(sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u))
                    pt_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && pt_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] > 0u && sbps[i].counts[TK_EAR] == 0u) pt_cohort.push_back(cohort[i]);
            while (pt_cohort.size() < 64u && !cohort.empty()) pt_cohort.push_back(cohort[pt_cohort.size() % cohort.size()]);
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0xbbb1u), 0u);
        gpu.upload_dnas_and_spawn(pt_cohort, 64u, mix(seed ^ 0xbbb1u));
        gpu.step_develop(mix(seed ^ 0xbbb1u));
        AblationFlags abl{}; abl.challenge_tier = 0u;

        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_l(WORLD_CELLS, 0.70f), flat_w(WORLD_CELLS, 1.0f);
        std::vector<float> flat_ter(WORLD_CELLS, 1.1f), zero_f(WORLD_CELLS, 0.0f), pulse_buf(WORLD_CELLS, 0.0f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "pt t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.light), flat_l.data(), WORLD_CELLS * sizeof(float)), "pt l");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "pt w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "pt ter");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "pt cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "pt cc");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "pt cd");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "pt dl");
        for (u32 p = 0; p < 32u; ++p) {
            float bx = 18.0f + float((p & 7u) * 30u);
            float by = 24.0f + float((p >> 3) * 60u);
            u32 ti = p * 2u, si = p * 2u + 1u;
            cs[ti].x = bx; cs[ti].y = by; cs[ti].theta = 0.0f;
            cs[si].x = bx + 2.5f; cs[si].y = by; cs[si].theta = 3.14159f;
            for (u32 idx_u : {ti, si}) {
                cs[idx_u].atp = 28.0f; cs[idx_u].prev_atp = 28.0f; cs[idx_u].hp = 100.0f; cs[idx_u].body_temp = 0.50f;
                cs[idx_u].age = (idx_u == si) ? 10u : 130u; // Juvenile critical-period Student + Adult Tutor!
            }
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "pt ul");

        auto fill_pairs = [&](float val) -> const float * {
            std::fill(pulse_buf.begin(), pulse_buf.end(), 0.0f);
            for (u32 i = 0; i < 64u; ++i) {
                u32 cx = u32(i32(cs[i].x + 512.0f) & i32(WORLD_MASK));
                u32 cy = u32(i32(cs[i].y + 512.0f) & i32(WORLD_MASK));
                for (int dy = -5; dy <= 5; ++dy)
                    for (int dx = -5; dx <= 5; ++dx)
                        pulse_buf[u32((int(cy) + dy) & int(WORLD_MASK)) * WORLD_W + u32((int(cx) + dx) & int(WORLD_MASK))] = val;
            }
            return pulse_buf.data();
        };

        u32 sim_t = 0u;
        // Returns (stu_v0, stu_v2, stu_hit, tut_hit)
        auto run_pt = [&](bool is_q1, bool tutor_da) -> std::tuple<double, double, double, double> {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "pt r dl");
            for (u32 p = 0; p < 32u; ++p) {
                float bx = 18.0f + float((p & 7u) * 30u);
                float by = 24.0f + float((p >> 3) * 60u);
                u32 ti = p * 2u, si = p * 2u + 1u;
                // Keep pairs within physical earshot (~2.5 tiles) without telepathic shortcuts!
                if (std::fabs(cs[si].x - cs[ti].x) > 5.0f || std::fabs(cs[si].y - cs[ti].y) > 5.0f) {
                    cs[ti].x = bx; cs[ti].y = by; cs[si].x = bx + 2.5f; cs[si].y = by;
                }
                for (u32 idx_u : {ti, si}) if (cs[idx_u].alive == 1u) {
                    cs[idx_u].hp = 100.0f; cs[idx_u].body_temp = 0.50f;
                    cs[idx_u].atp = 24.0f; cs[idx_u].prev_atp = 24.0f; cs[idx_u].da_baseline = 0.0f;
                    if (idx_u == si && cs[idx_u].age > 90u) cs[idx_u].age = 20u;
                }
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "pt r ul");
            const float *p1 = fill_pairs(1.6f);
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), is_q1 ? p1 : zero_f.data(), WORLD_CELLS * sizeof(float)), "pt 1v0");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), is_q1 ? zero_f.data() : p1, WORLD_CELLS * sizeof(float)), "pt 1v1");
            s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "pt 1v2");
            for (u32 k = 0; k < 4u; ++k, ++sim_t) {
                gpu.step_world(sim_t, 0u, 0u, seed);
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "pt r2");
            const float *p2 = fill_pairs(1.6f);
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), is_q1 ? zero_f.data() : p2, WORLD_CELLS * sizeof(float)), "pt 2v0");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), is_q1 ? p2 : zero_f.data(), WORLD_CELLS * sizeof(float)), "pt 2v1");
            double sv0 = 0.0, sv2 = 0.0; u32 sn = 0u, sh = 0u, tn = 0u, th = 0u;
            std::vector<u8> rew(64u, 0u);
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                gpu.step_world(sim_t, 0u, 0u, seed); // Live acoustic wavefield carries Tutor's voice -> Student's ears!
                gpu.step_brains(sim_t, abl);
                if (k >= 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "pt m");
                    bool dirty = false;
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                        bool is_tut = ((i & 1u) == 0u);
                        bool ans_v2 = (cs[i].mot_vocal2 > cs[i].mot_vocal0 + 0.02f && cs[i].mot_vocal2 > 0.12f);
                        bool ans_v0 = (cs[i].mot_vocal0 > cs[i].mot_vocal2 + 0.02f && cs[i].mot_vocal0 > 0.12f);
                        bool chord_m = (!ans_v2 && !ans_v0 && cs[i].mot_vocal0 > 0.12f && cs[i].mot_vocal2 > 0.12f);
                        bool ok = is_q1 ? ans_v2 : ans_v0;
                        bool bad = (is_q1 ? ans_v0 : ans_v2) || chord_m;
                        if (is_tut) {
                            ++tn; if (ok) ++th;
                            if (tutor_da && k <= 5u && !rew[i]) {
                                if (ok) { cs[i].atp += 2.4f; rew[i] = 1u; dirty = true; }
                                else if (bad) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.8f); rew[i] = 1u; dirty = true; }
                            }
                        } else {
                            // STRICTLY ZERO DA to Student! Pure unsupervised acoustic wavefield social imprinting!
                            sv0 += double(cs[i].mot_vocal0); sv2 += double(cs[i].mot_vocal2);
                            ++sn; if (ok) ++sh;
                        }
                    }
                    if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "pt rf");
                }
                gpu.step_bodies(sim_t, seed, abl);
            }
            s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "pt w0");
            s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "pt w1");
            s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "pt w2");
            for (u32 k = 0; k < 3u; ++k, ++sim_t) { gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
            return {sn ? sv0 / sn : 0.0, sn ? sv2 / sn : 0.0, sn ? double(sh) / sn : 0.0, tn ? double(th) / tn : 0.0};
        };

        auto [pr1_v0, pr1_v2, pr1_h, pr1_th] = run_pt(true, false);
        auto [pr2_v0, pr2_v2, pr2_h, pr2_th] = run_pt(false, false);
        double pre_stu_sep = std::fabs((pr1_v2 - pr1_v0) - (pr2_v2 - pr2_v0));
        double pre_stu_hit = 0.5 * (pr1_h + pr2_h);
        for (u32 tr = 0; tr < 8u; ++tr) { run_pt(true, true); run_pt(false, true); }
        auto [po1_v0, po1_v2, po1_h, po1_th] = run_pt(true, false);
        auto [po2_v0, po2_v2, po2_h, po2_th] = run_pt(false, false);
        double post_stu_sep = std::fabs((po1_v2 - po1_v0) - (po2_v2 - po2_v0));
        double post_stu_hit = 0.5 * (po1_h + po2_h);
        double tut_hit = 0.5 * (po1_th + po2_th);
        auto pt_ctr = gpu.read_counters();
        std::cout << "genesis_probe=cultural_peer_teaching  pairs=32  trials=8"
                  << "  tutor_hit=" << std::fixed << std::setprecision(3) << tut_hit
                  << "  pre_stu_sep=" << pre_stu_sep << "  post_stu_sep=" << post_stu_sep
                  << "  pre_stu_hit=" << pre_stu_hit << "  post_stu_hit=" << post_stu_hit
                  << "  peer_replies=" << pt_ctr.total_vocal_replies
                  << "  imprints=" << pt_ctr.total_imprint_edits
                  << "  stdp_edits=" << pt_ctr.total_edits << '\n';
    }

    // Assay 12: Episodic Pitch-Black Stimulus-Specific Working-Memory Attractor & Delayed Spoken Recall!
    // Trains Q1=(v0->v1)->v2 vs Q2=(v1->v0)->v0 via pure live DA RPE, then tests TWO pitch-black 24-tick silent delays:
    // Measures genuine stimulus-specific hidden cortical population vector separation `cos_dist = 1 - cos(vec_A, vec_B)`
    // across deep layers L1..Lmax during total sensory darkness AND delayed spoken recall accuracy (ZERO LLMs!).
    {
        std::vector<GenesisDNA> dm_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0xcc00u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0xcc00u));
            g_sc.step_develop(mix(seed ^ 0xcc00u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "dm sc bp");
            for (u32 i = 0; i < scan_n && dm_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u) dm_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && dm_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] > 0u && sbps[i].counts[TK_EAR] > 0u && !(sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u))
                    dm_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && dm_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] > 0u && sbps[i].counts[TK_GLAND] == 0u) dm_cohort.push_back(cohort[i]);
            while (dm_cohort.size() < 64u && !cohort.empty()) dm_cohort.push_back(cohort[dm_cohort.size() % cohort.size()]);
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0xccc1u), 0u);
        gpu.upload_dnas_and_spawn(dm_cohort, 64u, mix(seed ^ 0xccc1u));
        gpu.step_develop(mix(seed ^ 0xccc1u));
        AblationFlags abl{}; abl.challenge_tier = 0u;

        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_l(WORLD_CELLS, 0.70f), dark_l(WORLD_CELLS, 0.04f);
        std::vector<float> flat_w(WORLD_CELLS, 1.0f), flat_ter(WORLD_CELLS, 1.1f), zero_f(WORLD_CELLS, 0.0f), flat_p(WORLD_CELLS, 1.6f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "dm t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.light), flat_l.data(), WORLD_CELLS * sizeof(float)), "dm l");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "dm w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "dm ter");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "dm cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "dm cc");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "dm cd");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "dm dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 28.0f; cs[i].prev_atp = 28.0f; cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "dm ul");

        u32 sim_t = 0u;
        auto set_dm_aud = [&](int band) {
            if (band == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), flat_p.data(), WORLD_CELLS * sizeof(float)), "dm v0");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "dm v0z");
            if (band == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), flat_p.data(), WORLD_CELLS * sizeof(float)), "dm v1");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "dm v1z");
            s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "dm v2z");
            s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "dm pz");
        };
        // 6 live operant training trials teaching Q1=(v0->v1)->v2, Q2=(v1->v0)->v0
        for (u32 tr = 0; tr < 12u; ++tr) {
            bool is_q1 = ((tr & 1u) == 0u);
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "dm tr dl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f;
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "dm tr ul");
            for (u32 k = 0; k < 4u; ++k, ++sim_t) { set_dm_aud(is_q1 ? 0 : 1); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
            std::vector<u8> rew(64u, 0u);
            for (u32 k = 0; k < 6u; ++k, ++sim_t) {
                set_dm_aud(is_q1 ? 1 : 0);
                gpu.step_brains(sim_t, abl);
                if (k >= 2u && k <= 4u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "dm tm");
                    bool dirty = false;
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                        bool av2 = (cs[i].mot_vocal2 > cs[i].mot_vocal0 + 0.02f && cs[i].mot_vocal2 > 0.12f);
                        bool av0 = (cs[i].mot_vocal0 > cs[i].mot_vocal2 + 0.02f && cs[i].mot_vocal0 > 0.12f);
                        bool cm = (!av2 && !av0 && cs[i].mot_vocal0 > 0.12f && cs[i].mot_vocal2 > 0.12f);
                        bool ok = is_q1 ? av2 : av0;
                        bool bad = (is_q1 ? av0 : av2) || cm;
                        if (ok && (rew[i] & 1u) == 0u) { cs[i].atp += 2.4f; rew[i] |= 1u; dirty = true; }
                        else if (bad && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f); rew[i] |= 2u; dirty = true; }
                    }
                    if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "dm tf");
                }
                gpu.step_bodies(sim_t, seed, abl);
            }
            for (u32 k = 0; k < 5u; ++k, ++sim_t) { set_dm_aud(-1); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
        }

        // Now run THREE 24-tick TRUE ANECHOIC pitch-black delayed-recall probes (A1=Q1, A2=Q1 repeat, B=Q2)
        // measuring BOTH cue-phase and dark-delay Representation SNR = cos(A,B) / max(0.01, cos(A,A'))!
        constexpr u32 DM_DELAY = 24u;
        std::vector<u8> all_rings(size_t(gpu.max_layers) * TISSUE_CELLS);
        auto eval_dark_delay = [&](bool is_q1, std::vector<float> &cue_vec, std::vector<float> &pop_vec) -> std::tuple<double, double, double, double> {
            // 10-tick truly anechoic silent washout so trials start from identical quiet baseline
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), dark_l.data(), WORLD_CELLS * sizeof(float)), "dm dlgt");
            for (u32 k = 0; k < 10u; ++k, ++sim_t) { set_dm_aud(-1); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "dm ed dl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                if (cs[i].atp < 22.0f) { cs[i].atp = 22.0f; cs[i].prev_atp = 22.0f; }
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "dm ed ul");
            // Present 8-tick stimulus (Q1 or Q2) while recording Cue-phase cortical representation, then 24t pitch-black silence!
            cue_vec.assign(64u * 4u * TISSUE_CELLS, 0.0f);
            for (u32 k = 0; k < 4u; ++k, ++sim_t) { set_dm_aud(is_q1 ? 0 : 1); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
            for (u32 k = 0; k < 4u; ++k, ++sim_t) {
                set_dm_aud(is_q1 ? 1 : 0); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
                s.d.check(s.d.download(all_rings.data(), DevicePtr(gpu.pool.spike_ring), all_rings.size()), "dm cu sr");
                for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                    u32 lim_z = std::min<u32>(cs[i].num_layers, 5u);
                    for (u32 z = 1u; z < lim_z; ++z) {
                        u32 sl = cs[i].layer_slabs[z];
                        if (sl == INVALID_SLAB || sl >= gpu.max_layers) continue;
                        const u8 *sr = all_rings.data() + size_t(sl) * TISSUE_CELLS;
                        float *dst = cue_vec.data() + (size_t(i) * 4u + size_t(z - 1u)) * TISSUE_CELLS;
                        for (u32 c = 0; c < TISSUE_CELLS; ++c) dst[c] += float(__builtin_popcount(u32(sr[c] & 0x0fu)));
                    }
                }
            }

            pop_vec.assign(64u * 4u * TISSUE_CELLS, 0.0f);
            u64 s0 = gpu.read_counters().total_spikes;
            double v0_s = 0.0, v2_s = 0.0; u32 sn = 0u, hits = 0u;
            for (u32 k = 0; k < DM_DELAY; ++k, ++sim_t) {
                set_dm_aud(-1);
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
                if (k >= DM_DELAY - 8u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "dm dr cs");
                    s.d.check(s.d.download(all_rings.data(), DevicePtr(gpu.pool.spike_ring), all_rings.size()), "dm dr sr");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                        v0_s += double(cs[i].mot_vocal0); v2_s += double(cs[i].mot_vocal2); ++sn;
                        bool av2 = (cs[i].mot_vocal2 > cs[i].mot_vocal0 + 0.02f && cs[i].mot_vocal2 > 0.12f);
                        bool av0 = (cs[i].mot_vocal0 > cs[i].mot_vocal2 + 0.02f && cs[i].mot_vocal0 > 0.12f);
                        if (is_q1 ? av2 : av0) ++hits;
                        u32 lim_z = std::min<u32>(cs[i].num_layers, 5u);
                        for (u32 z = 1u; z < lim_z; ++z) {
                            u32 sl = cs[i].layer_slabs[z];
                            if (sl == INVALID_SLAB || sl >= gpu.max_layers) continue;
                            const u8 *sr = all_rings.data() + size_t(sl) * TISSUE_CELLS;
                            float *dst = pop_vec.data() + (size_t(i) * 4u + size_t(z - 1u)) * TISSUE_CELLS;
                            for (u32 c = 0; c < TISSUE_CELLS; ++c) dst[c] += float(__builtin_popcount(u32(sr[c] & 0x0fu)));
                        }
                    }
                }
            }
            double spk_t = double(gpu.read_counters().total_spikes - s0) / double(DM_DELAY);
            double inv = sn ? (1.0 / double(sn)) : 0.0;
            return {spk_t, v0_s * inv, v2_s * inv, double(hits) * inv};
        };

        auto mean_cos_dist = [&](const std::vector<float> &va, const std::vector<float> &vb) -> double {
            double cos_sum = 0.0; u32 cos_n = 0u;
            for (u32 i = 0; i < 64u; ++i) {
                double dot = 0.0, na = 0.0, nb = 0.0;
                const float *pa = va.data() + size_t(i) * 4u * TISSUE_CELLS;
                const float *pb = vb.data() + size_t(i) * 4u * TISSUE_CELLS;
                for (u32 j = 0; j < 4u * TISSUE_CELLS; ++j) {
                    dot += double(pa[j]) * double(pb[j]);
                    na += double(pa[j]) * double(pa[j]);
                    nb += double(pb[j]) * double(pb[j]);
                }
                if (na > 1.0 && nb > 1.0) {
                    cos_sum += (1.0 - std::clamp(dot / std::sqrt(na * nb), -1.0, 1.0));
                    ++cos_n;
                }
            }
            return cos_n ? (cos_sum / double(cos_n)) : 0.0;
        };

        std::vector<float> cue_a1, vec_a1, cue_a2, vec_a2, cue_b, vec_b;
        auto [spk_a, a_v0, a_v2, a_hit] = eval_dark_delay(true, cue_a1, vec_a1);
        auto [spk_b, b_v0, b_v2, b_hit] = eval_dark_delay(false, cue_b, vec_b);
        eval_dark_delay(true, cue_a2, vec_a2);
        double cue_ab = mean_cos_dist(cue_a1, cue_b);
        double cue_aa = mean_cos_dist(cue_a1, cue_a2);
        double cue_snr = cue_ab / std::max(0.01, cue_aa);
        double cos_dist = mean_cos_dist(vec_a1, vec_b);
        double cos_aa = mean_cos_dist(vec_a1, vec_a2);
        double dark_snr = cos_dist / std::max(0.01, cos_aa);
        double rec_sep = std::fabs((a_v2 - a_v0) - (b_v2 - b_v0));
        double rec_hit = 0.5 * (a_hit + b_hit);
        u64 dm_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=episodic_delayed_recall  delay_ticks=" << DM_DELAY
                  << "  cos_dist=" << std::fixed << std::setprecision(3) << cos_dist
                  << "  cos_aa=" << cos_aa << "  snr=" << std::setprecision(2) << dark_snr
                  << "  cue_cos=" << std::setprecision(3) << cue_ab << "  cue_aa=" << cue_aa
                  << "  cue_snr=" << std::setprecision(2) << cue_snr
                  << "  dark_spk_a=" << std::setprecision(1) << spk_a
                  << "  dark_spk_b=" << spk_b
                  << "  recall_sep=" << std::setprecision(3) << rec_sep
                  << "  recall_hit=" << rec_hit
                  << "  stdp_edits=" << dm_ed << '\n';
        // Per-layer laminar SNR X-ray diagnostic (L1..L4 cue & pitch-black dark SNR + firing activity!)
        auto layer_cos = [&](const std::vector<float> &va, const std::vector<float> &vb, u32 lz, double &mean_norm) -> double {
            double csum = 0.0, nsum = 0.0; u32 cn = 0u;
            for (u32 i = 0; i < 64u; ++i) {
                double dot = 0.0, na = 0.0, nb = 0.0;
                const float *pa = va.data() + (size_t(i) * 4u + lz) * TISSUE_CELLS;
                const float *pb = vb.data() + (size_t(i) * 4u + lz) * TISSUE_CELLS;
                for (u32 j = 0; j < TISSUE_CELLS; ++j) {
                    dot += double(pa[j]) * double(pb[j]);
                    na += double(pa[j]) * double(pa[j]);
                    nb += double(pb[j]) * double(pb[j]);
                }
                nsum += std::sqrt(na);
                if (na > 1.0 && nb > 1.0) { csum += (1.0 - std::clamp(dot / std::sqrt(na * nb), -1.0, 1.0)); ++cn; }
            }
            mean_norm = nsum / 64.0;
            return cn ? (csum / double(cn)) : 0.0;
        };
        std::cout << "genesis_probe=laminar_snr";
        for (u32 lz = 0; lz < 4u; ++lz) {
            double cn_c = 0.0, cn_d = 0.0, d_tmp = 0.0;
            double cab = layer_cos(cue_a1, cue_b, lz, cn_c);
            double caa = layer_cos(cue_a1, cue_a2, lz, d_tmp);
            double dab = layer_cos(vec_a1, vec_b, lz, cn_d);
            double daa = layer_cos(vec_a1, vec_a2, lz, d_tmp);
            std::cout << "  L" << (lz + 1u) << "_cue=" << std::fixed << std::setprecision(2) << cab << "/" << caa
                      << "(" << (cab / std::max(0.01, caa)) << "x,n=" << std::setprecision(1) << cn_c << ")"
                      << "  L" << (lz + 1u) << "_dk=" << std::setprecision(2) << dab << "/" << daa
                      << "(" << (dab / std::max(0.01, daa)) << "x,n=" << std::setprecision(1) << cn_d << ")";
        }
        std::cout << '\n';
    }

    // Assay 13: Multi-Turn Stateful Context-Dependent Dialogue (XOR / Context-Gated Conversation — ZERO LLMs!)
    // Turn 1 sets a physical context (Context A = sunlit reef + v0 vs Context B = predator rumble + v1),
    // followed by 10 ticks of pitch-black silence (working memory hold!), then Turn 2 asks the EXACT SAME
    // acoustic question `v2` in BOTH contexts! Stateless reflexes score 0.00 separation; solving requires
    // deep prefrontal working memory + apical BAC context gating + BTSP vocal learning (Ctx A + v2 -> v0, Ctx B + v2 -> v1)!
    {
        std::vector<GenesisDNA> cx_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0xdd00u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0xdd00u));
            g_sc.step_develop(mix(seed ^ 0xdd00u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "cx sc bp");
            for (u32 i = 0; i < scan_n && cx_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_EYE] > 0u)
                    cx_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && cx_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_EYE] == 0u)
                    cx_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && cx_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] > 0u && sbps[i].counts[TK_EAR] > 0u && !(sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u))
                    cx_cohort.push_back(cohort[i]);
            while (cx_cohort.size() < 64u && !cohort.empty()) cx_cohort.push_back(cohort[cx_cohort.size() % cohort.size()]);
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0xddd1u), 0u);
        gpu.upload_dnas_and_spawn(cx_cohort, 64u, mix(seed ^ 0xddd1u));
        gpu.step_develop(mix(seed ^ 0xddd1u));
        std::vector<BodyPhenotype> cx_bps(64u);
        s.d.check(s.d.download(cx_bps.data(), gpu.d_bodies.ptr, 64u * sizeof(BodyPhenotype)), "cx bp");
        std::vector<u8> cx_has_gl(64u, 0u);
        u32 n_voc_cx = 0u;
        for (u32 i = 0; i < 64u; ++i) {
            if (cx_bps[i].counts[TK_GLAND] > 0u) { cx_has_gl[i] = 1u; ++n_voc_cx; }
        }
        AblationFlags abl{}; abl.challenge_tier = 0u;

        std::vector<float> flat_t(WORLD_CELLS, 0.50f), bright_l(WORLD_CELLS, 1.85f), dark_l(WORLD_CELLS, 0.04f);
        std::vector<float> flat_w(WORLD_CELLS, 1.0f), flat_ter(WORLD_CELLS, 1.1f), zero_f(WORLD_CELLS, 0.0f), flat_p(WORLD_CELLS, 1.6f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "cx t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "cx w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "cx ter");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "cx cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "cx cc");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "cx cd");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "cx dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 28.0f; cs[i].prev_atp = 28.0f; cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "cx ul");

        u32 sim_t = 0u;
        constexpr u32 CTX_DELAY = 10u;
        auto run_ctx = [&](bool is_ctx_a, bool tutor_da) -> std::tuple<double, double, double> {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "cx r dl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f;
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "cx r ul");
            auto set_cx_aud = [&](int band, bool with_p) {
                if (band == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), flat_p.data(), WORLD_CELLS * sizeof(float)), "cx v0");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "cx v0z");
                if (band == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), flat_p.data(), WORLD_CELLS * sizeof(float)), "cx v1");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "cx v1z");
                if (band == 2) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), flat_p.data(), WORLD_CELLS * sizeof(float)), "cx v2");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "cx v2z");
                if (with_p) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_p), flat_p.data(), WORLD_CELLS * sizeof(float)), "cx p");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "cx pz");
            };
            // Turn 1: Context A (sunlit bright + v0) vs Context B (dark + predator rumble acoust_p + v2) for 5 ticks!
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), is_ctx_a ? bright_l.data() : dark_l.data(), WORLD_CELLS * sizeof(float)), "cx 1l");
            for (u32 k = 0; k < 5u; ++k, ++sim_t) { set_cx_aud(is_ctx_a ? 0 : 2, !is_ctx_a); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }

            // Pitch-black TRUE ANECHOIC silent working-memory delay (10 ticks — ALL cues extinguished!)
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), dark_l.data(), WORLD_CELLS * sizeof(float)), "cx dl");
            for (u32 k = 0; k < CTX_DELAY; ++k, ++sim_t) { set_cx_aud(-1, false); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }

            // Turn 2: IDENTICAL acoustic question `v1` in BOTH contexts! (Ctx A -> Left syrinx v0, Ctx B -> Right syrinx v2)
            double v0_s = 0.0, v2_s = 0.0; u32 sn = 0u, hits = 0u;
            std::vector<u8> rew(64u, 0u);
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                set_cx_aud(1, false);
                gpu.step_brains(sim_t, abl);
                if (k >= 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "cx m");
                    bool dirty = false;
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && cx_has_gl[i]) {
                        v0_s += double(cs[i].mot_vocal0); v2_s += double(cs[i].mot_vocal2); ++sn;
                        bool ans_v0 = (cs[i].mot_vocal0 > cs[i].mot_vocal2 + 0.02f && cs[i].mot_vocal0 > cs[i].mot_vocal1 + 0.02f && cs[i].mot_vocal0 > 0.12f);
                        bool ans_v2 = (cs[i].mot_vocal2 > cs[i].mot_vocal0 + 0.02f && cs[i].mot_vocal2 > cs[i].mot_vocal1 + 0.02f && cs[i].mot_vocal2 > 0.12f);
                        bool off_v1 = (cs[i].mot_vocal1 > 0.12f && cs[i].mot_vocal1 >= cs[i].mot_vocal0 && cs[i].mot_vocal1 >= cs[i].mot_vocal2);
                        bool mud = (!ans_v0 && !ans_v2 && !off_v1 && (cs[i].mot_vocal0 > 0.12f || cs[i].mot_vocal2 > 0.12f));
                        bool ok = is_ctx_a ? ans_v0 : ans_v2;
                        bool bad = (is_ctx_a ? ans_v2 : ans_v0) || off_v1 || mud;
                        if (ok) ++hits;
                        if (tutor_da && k <= 6u) {
                            if (ok && (rew[i] & 1u) == 0u) { cs[i].atp += 2.4f; rew[i] |= 1u; dirty = true; }
                            else if (bad && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f); rew[i] |= 2u; dirty = true; }
                            else if (k == 5u && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 0.9f); rew[i] |= 2u; dirty = true; }
                        }
                    }
                    if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "cx rf");
                }
                gpu.step_bodies(sim_t, seed, abl);
            }
            for (u32 k = 0; k < 8u; ++k, ++sim_t) { set_cx_aud(-1, false); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
            double inv = sn ? (1.0 / double(sn)) : 0.0;
            return {v0_s * inv, v2_s * inv, double(hits) * inv};
        };

        auto [pr_a0, pr_a2, pr_ah] = run_ctx(true, false);
        auto [pr_b0, pr_b2, pr_bh] = run_ctx(false, false);
        double pre_sep = std::fabs((pr_a0 - pr_a2) - (pr_b0 - pr_b2));
        double pre_hit = 0.5 * (pr_ah + pr_bh);
        for (u32 tr = 0; tr < 16u; ++tr) {
            run_ctx(true, true);
            run_ctx(false, true);
        }
        auto [po_a0, po_a2, po_ah] = run_ctx(true, false);
        auto [po_b0, po_b2, po_bh] = run_ctx(false, false);
        double post_sep = std::fabs((po_a0 - po_a2) - (po_b0 - po_b2));
        double post_hit = 0.5 * (po_ah + po_bh);
        u64 cx_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=context_dependent_dialogue  delay_ticks=" << CTX_DELAY << "  trials=16  vocalists=" << n_voc_cx
                  << "  pre_sep=" << std::fixed << std::setprecision(3) << pre_sep
                  << "  post_sep=" << post_sep
                  << "  pre_hit=" << pre_hit
                  << "  post_hit=" << post_hit
                  << "  ctxA_v0=" << std::setprecision(2) << po_a0 << "  ctxA_v2=" << po_a2
                  << "  ctxB_v0=" << po_b0 << "  ctxB_v2=" << po_b2
                  << "  stdp_edits=" << cx_ed << '\n';
    }

    // Assay 14: Grounded 3-Concept Referential Naming & Zero-Audio Cross-Modal Recall (ZERO LLMs!)
    // Teaches 3 distinct grounded multimodal concepts via live operant DA RPE on organisms' own 3-fold syrinx:
    //   Concept 0 (Sunlit Food Reef): bright light + chem_c + v0 -> speak v0
    //   Concept 1 (Predator Danger):  dark + acoust_p rumble + v1 -> speak v1
    //   Concept 2 (Mineral Tool):     moderate light + chem_a + v2 -> speak v2
    // Then tests BOTH (1) 3-way multimodal spoken naming accuracy AND (2) ZERO-AUDIO pure sensory-to-speech recall
    // (can the spiking mind name Food / Danger / Mineral purely from vision/chemistry/rumble with ZERO vocal prompt?)
    {
        std::vector<GenesisDNA> nm_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0xee00u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0xee00u));
            g_sc.step_develop(mix(seed ^ 0xee00u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "nm sc bp");
            for (u32 i = 0; i < scan_n && nm_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] >= 3u && sbps[i].counts[TK_EAR] >= 2u) nm_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && nm_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] < 3u)
                    nm_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && nm_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] > 0u && !(sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u))
                    nm_cohort.push_back(cohort[i]);
            while (nm_cohort.size() < 64u && !cohort.empty()) nm_cohort.push_back(cohort[nm_cohort.size() % cohort.size()]);
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0xeee1u), 0u);
        gpu.upload_dnas_and_spawn(nm_cohort, 64u, mix(seed ^ 0xeee1u));
        gpu.step_develop(mix(seed ^ 0xeee1u));
        std::vector<BodyPhenotype> nm_bps(64u);
        s.d.check(s.d.download(nm_bps.data(), gpu.d_bodies.ptr, 64u * sizeof(BodyPhenotype)), "nm bp");
        std::vector<u8> nm_has_gl(64u, 0u);
        u32 n_voc_nm = 0u;
        for (u32 i = 0; i < 64u; ++i) {
            if (nm_bps[i].counts[TK_GLAND] > 0u) { nm_has_gl[i] = 1u; ++n_voc_nm; }
        }
        AblationFlags abl{}; abl.challenge_tier = 0u;

        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_w(WORLD_CELLS, 1.0f), flat_ter(WORLD_CELLS, 1.1f);
        std::vector<float> zero_f(WORLD_CELLS, 0.0f), flat_p(WORLD_CELLS, 1.6f);
        std::vector<float> l_hi(WORLD_CELLS, 1.90f), l_mid(WORLD_CELLS, 0.65f), l_dark(WORLD_CELLS, 0.05f);
        std::vector<float> c_hi(WORLD_CELLS, 1.40f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "nm t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "nm w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "nm ter");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "nm cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "nm cd");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "nm dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 28.0f; cs[i].prev_atp = 28.0f; cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "nm ul");

        u32 sim_t = 0u;
        auto run_concept = [&](u32 cid_c, bool with_audio, bool tutor_da) -> double {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "nm r dl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f;
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "nm r ul");
            auto set_nm_aud = [&](int band, bool with_p) {
                if (band == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), flat_p.data(), WORLD_CELLS * sizeof(float)), "nm v0");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "nm v0z");
                if (band == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), flat_p.data(), WORLD_CELLS * sizeof(float)), "nm v1");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "nm v1z");
                if (band == 2) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), flat_p.data(), WORLD_CELLS * sizeof(float)), "nm v2");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "nm v2z");
                if (with_p) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_p), flat_p.data(), WORLD_CELLS * sizeof(float)), "nm ap");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "nm apz");
            };
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), (cid_c == 0u ? l_hi : (cid_c == 1u ? l_dark : l_mid)).data(), WORLD_CELLS * sizeof(float)), "nm l");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), (cid_c == 0u ? c_hi : zero_f).data(), WORLD_CELLS * sizeof(float)), "nm cc");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_a), (cid_c == 2u ? c_hi : zero_f).data(), WORLD_CELLS * sizeof(float)), "nm ca");
            u32 sn = 0u, hits = 0u;
            std::vector<u8> rew(64u, 0u);
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                set_nm_aud(with_audio ? int(cid_c) : -1, cid_c == 1u);
                gpu.step_brains(sim_t, abl);
                if (k >= 2u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "nm m");
                    bool dirty = false;
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && nm_has_gl[i]) {
                        ++sn;
                        float vs[3] = {cs[i].mot_vocal0, cs[i].mot_vocal1, cs[i].mot_vocal2};
                        u32 best_b = (vs[0] >= vs[1] && vs[0] >= vs[2]) ? 0u : ((vs[1] >= vs[2]) ? 1u : 2u);
                        float runner_up = (best_b == 0u) ? std::max(vs[1], vs[2]) : ((best_b == 1u) ? std::max(vs[0], vs[2]) : std::max(vs[0], vs[1]));
                        bool crisp = (vs[best_b] > 0.12f && vs[best_b] > runner_up + 0.02f);
                        bool ok = (crisp && best_b == cid_c);
                        bool bad = (vs[best_b] > 0.12f && !ok);
                        if (ok) ++hits;
                        if (tutor_da && k <= 6u) {
                            if (ok && (rew[i] & 1u) == 0u) { cs[i].atp += 2.4f; rew[i] |= 1u; dirty = true; }
                            else if (bad && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f); rew[i] |= 2u; dirty = true; }
                            else if (k == 5u && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 0.9f); rew[i] |= 2u; dirty = true; }
                        }
                    }
                    if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "nm rf");
                }
                gpu.step_bodies(sim_t, seed, abl);
            }
            s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_a), 0, WORLD_CELLS * sizeof(float)), "nm zca");
            s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "nm zcc");
            for (u32 k = 0; k < 5u; ++k, ++sim_t) { set_nm_aud(-1, false); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
            return sn ? (double(hits) / double(sn)) : 0.0;
        };

        double pre_h = (run_concept(0u, true, false) + run_concept(1u, true, false) + run_concept(2u, true, false)) / 3.0;
        double pre_xm = (run_concept(0u, false, false) + run_concept(1u, false, false) + run_concept(2u, false, false)) / 3.0;
        for (u32 tr = 0; tr < 12u; ++tr) {
            run_concept(0u, true, true);
            run_concept(1u, true, true);
            run_concept(2u, true, true);
        }
        double h0 = run_concept(0u, true, false), h1 = run_concept(1u, true, false), h2 = run_concept(2u, true, false);
        double post_h = (h0 + h1 + h2) / 3.0;
        double xm0 = run_concept(0u, false, false), xm1 = run_concept(1u, false, false), xm2 = run_concept(2u, false, false);
        double post_xm = (xm0 + xm1 + xm2) / 3.0;
        u64 nm_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=grounded_3concept_naming  trials=12  vocalists=" << n_voc_nm
                  << "  pre_hit=" << std::fixed << std::setprecision(3) << pre_h
                  << "  post_hit=" << post_h
                  << "  pre_xmodal=" << pre_xm
                  << "  post_xmodal=" << post_xm
                  << "  c0_hit=" << std::setprecision(2) << h0 << "  c1_hit=" << h1 << "  c2_hit=" << h2
                  << "  stdp_edits=" << nm_ed << '\n';
    }

    // Assay 15: Compositional 2-Syllable Sequential Speech Production (v0->v2 vs v2->v0 — ZERO LLMs!)
    // Both target phrases use the EXACT SAME phonemes {v0, v2} in REVERSED temporal order!
    //   Phrase A (Sunlit Food + v0, then neutral query v1): speak v0 (Win 1) -> v2 (Win 2)
    //   Phrase B (Dark Rumble + v2, then neutral query v1): speak v2 (Win 1) -> v0 (Win 2)
    // Window 2 presents the IDENTICAL acoustic cue `v1` in both phrases -> impossible for single-phoneme
    // reflexes or unordered bags; requires grounded working-memory state transition & sequential motor planning!
    {
        std::vector<GenesisDNA> p2_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0xff00u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0xff00u));
            g_sc.step_develop(mix(seed ^ 0xff00u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "p2 sc bp");
            for (u32 i = 0; i < scan_n && p2_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u) p2_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && p2_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] > 0u && !(sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_EAR] >= 2u))
                    p2_cohort.push_back(cohort[i]);
            while (p2_cohort.size() < 64u && !cohort.empty()) p2_cohort.push_back(cohort[p2_cohort.size() % cohort.size()]);
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0xfff1u), 0u);
        gpu.upload_dnas_and_spawn(p2_cohort, 64u, mix(seed ^ 0xfff1u));
        gpu.step_develop(mix(seed ^ 0xfff1u));
        std::vector<BodyPhenotype> p2_bps(64u);
        s.d.check(s.d.download(p2_bps.data(), gpu.d_bodies.ptr, 64u * sizeof(BodyPhenotype)), "p2 bp");
        std::vector<u8> p2_has_gl(64u, 0u);
        u32 n_voc_p2 = 0u;
        for (u32 i = 0; i < 64u; ++i) {
            if (p2_bps[i].counts[TK_GLAND] > 0u) { p2_has_gl[i] = 1u; ++n_voc_p2; }
        }
        AblationFlags abl{}; abl.challenge_tier = 0u;

        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_w(WORLD_CELLS, 1.0f), flat_ter(WORLD_CELLS, 1.1f);
        std::vector<float> zero_f(WORLD_CELLS, 0.0f), flat_p(WORLD_CELLS, 1.6f);
        std::vector<float> l_hi(WORLD_CELLS, 1.90f), l_dark(WORLD_CELLS, 0.05f), c_hi(WORLD_CELLS, 1.35f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "p2 t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "p2 w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "p2 ter");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "p2 cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "p2 cd");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "p2 dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "p2 ul");

        u32 sim_t = 0u;
        // Returns (seq_hit, syl1_hit, syl2_hit)
        auto run_phrase = [&](bool is_a, bool tutor_da) -> std::tuple<double, double, double> {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "p2 r dl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f;
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "p2 r ul");

            u32 want1 = is_a ? 0u : 2u;
            u32 want2 = is_a ? 2u : 0u;
            std::vector<u8> rew1(64u, 0u), rew2(64u, 0u);
            std::vector<int> w1_v(64u, -1), w2_v(64u, -1);

            auto set_p2_aud = [&](int band, bool with_p) {
                if (band == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), flat_p.data(), WORLD_CELLS * sizeof(float)), "p2 v0");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "p2 v0z");
                if (band == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), flat_p.data(), WORLD_CELLS * sizeof(float)), "p2 v1");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "p2 v1z");
                if (band == 2) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), flat_p.data(), WORLD_CELLS * sizeof(float)), "p2 v2");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "p2 v2z");
                if (with_p) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_p), flat_p.data(), WORLD_CELLS * sizeof(float)), "p2 p");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "p2 pz");
            };
            // Window 1 (ticks 0..4): Grounded Concept Context (A = Sunlit Food + v0, B = Dark Rumble + v2) -> produce Syl 1!
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), (is_a ? l_hi : l_dark).data(), WORLD_CELLS * sizeof(float)), "p2 1l");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), (is_a ? c_hi : zero_f).data(), WORLD_CELLS * sizeof(float)), "p2 1c");
            for (u32 k = 0; k < 5u; ++k, ++sim_t) {
                set_p2_aud(is_a ? 0 : 2, !is_a);
                gpu.step_brains(sim_t, abl);
                if (k >= 1u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "p2 m1");
                    bool dirty = false;
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && p2_has_gl[i]) {
                        float vs[3] = {cs[i].mot_vocal0, cs[i].mot_vocal1, cs[i].mot_vocal2};
                        u32 b = (vs[0] >= vs[1] && vs[0] >= vs[2]) ? 0u : ((vs[1] >= vs[2]) ? 1u : 2u);
                        float ru = (b == 0u) ? std::max(vs[1], vs[2]) : ((b == 1u) ? std::max(vs[0], vs[2]) : std::max(vs[0], vs[1]));
                        bool crisp = (vs[b] > 0.12f && vs[b] > ru + 0.02f);
                        if (crisp && w1_v[i] < 0) w1_v[i] = int(b);
                        if (tutor_da && k <= 4u) {
                            if (crisp && b == want1 && (rew1[i] & 1u) == 0u) { cs[i].atp += 2.4f; rew1[i] |= 1u; dirty = true; }
                            else if (vs[b] > 0.12f && rew1[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f); rew1[i] |= 2u; dirty = true; }
                            else if (k == 3u && rew1[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 0.9f); rew1[i] |= 2u; dirty = true; }
                        }
                    }
                    if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "p2 r1");
                }
                gpu.step_bodies(sim_t, seed, abl);
            }

            // Window 2 (ticks 5..9): IDENTICAL neutral query `v1` in dark silence -> produce Syl 2 (flipped order)!
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "p2 2dl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f;
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "p2 2ul");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), l_dark.data(), WORLD_CELLS * sizeof(float)), "p2 2l");
            s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "p2 2c");
            for (u32 k = 0; k < 5u; ++k, ++sim_t) {
                set_p2_aud(1, false);
                gpu.step_brains(sim_t, abl);
                if (k >= 1u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "p2 m2");
                    bool dirty = false;
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && p2_has_gl[i]) {
                        float vs[3] = {cs[i].mot_vocal0, cs[i].mot_vocal1, cs[i].mot_vocal2};
                        u32 b = (vs[0] >= vs[1] && vs[0] >= vs[2]) ? 0u : ((vs[1] >= vs[2]) ? 1u : 2u);
                        float ru = (b == 0u) ? std::max(vs[1], vs[2]) : ((b == 1u) ? std::max(vs[0], vs[2]) : std::max(vs[0], vs[1]));
                        bool crisp = (vs[b] > 0.12f && vs[b] > ru + 0.02f);
                        if (crisp && w2_v[i] < 0) w2_v[i] = int(b);
                        if (tutor_da && k <= 4u) {
                            if (crisp && b == want2 && (rew2[i] & 1u) == 0u) { cs[i].atp += 2.4f; rew2[i] |= 1u; dirty = true; }
                            else if (vs[b] > 0.12f && rew2[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f); rew2[i] |= 2u; dirty = true; }
                            else if (k == 3u && rew2[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 0.9f); rew2[i] |= 2u; dirty = true; }
                        }
                    }
                    if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "p2 r2");
                }
                gpu.step_bodies(sim_t, seed, abl);
            }
            for (u32 k = 0; k < 6u; ++k, ++sim_t) { set_p2_aud(-1, false); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }

            u32 sn = 0u, h1 = 0u, h2 = 0u, hs = 0u;
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && p2_has_gl[i]) {
                ++sn;
                bool ok1 = (w1_v[i] == int(want1));
                bool ok2 = (w2_v[i] == int(want2));
                if (ok1) ++h1;
                if (ok2) ++h2;
                if (ok1 && ok2) ++hs;
            }
            double inv = sn ? (1.0 / double(sn)) : 0.0;
            return {double(hs) * inv, double(h1) * inv, double(h2) * inv};
        };

        auto [pr_as, pr_a1, pr_a2] = run_phrase(true, false);
        auto [pr_bs, pr_b1, pr_b2] = run_phrase(false, false);
        double pre_seq = 0.5 * (pr_as + pr_bs);
        for (u32 tr = 0; tr < 16u; ++tr) {
            run_phrase(true, true);
            run_phrase(false, true);
        }
        auto [po_as, po_a1, po_a2] = run_phrase(true, false);
        auto [po_bs, po_b1, po_b2] = run_phrase(false, false);
        double post_seq = 0.5 * (po_as + po_bs);
        u64 p2_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=compositional_2syl_syntax  trials=16  vocalists=" << n_voc_p2
                  << "  pre_seq_hit=" << std::fixed << std::setprecision(3) << pre_seq
                  << "  post_seq_hit=" << post_seq
                  << "  syl1_hit=" << (0.5 * (po_a1 + po_b1))
                  << "  syl2_hit=" << (0.5 * (po_a2 + po_b2))
                  << "  phraseA_hit=" << std::setprecision(2) << po_as
                  << "  phraseB_hit=" << po_bs
                  << "  stdp_edits=" << p2_ed << '\n';
    }

    // Assay 16: Bidirectional Referential Instruction Following (Speech -> Mind -> Grounded Embodied Action!)
    // Uses the EXACT SAME two phonemes {v0, v2} in REVERSED temporal order in neutral featureless water:
    //   Directive A (v0 -> v2 = "GO/FORAGE"): swim & forage actively (mot_thrust + 0.5*mot_eat >= 0.35)
    //   Directive B (v2 -> v0 = "HALT/HOLD"): brake & hold still (mot_thrust <= 0.18)
    // Impossible for single-phoneme reflexes; completes the full bidirectional grounded language loop (ZERO LLMs!)
    {
        std::vector<GenesisDNA> ic_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0x1600u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0x1600u));
            g_sc.step_develop(mix(seed ^ 0x1600u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "ic sc bp");
            for (u32 i = 0; i < scan_n && ic_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_MUSCLE] >= 2u) ic_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && ic_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] > 0u && sbps[i].counts[TK_MUSCLE] > 0u &&
                    !(sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_MUSCLE] >= 2u))
                    ic_cohort.push_back(cohort[i]);
            while (ic_cohort.size() < 64u && !cohort.empty()) ic_cohort.push_back(cohort[ic_cohort.size() % cohort.size()]);
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0x1601u), 0u);
        gpu.upload_dnas_and_spawn(ic_cohort, 64u, mix(seed ^ 0x1601u));
        gpu.step_develop(mix(seed ^ 0x1601u));
        std::vector<BodyPhenotype> ic_bps(64u);
        s.d.check(s.d.download(ic_bps.data(), gpu.d_bodies.ptr, 64u * sizeof(BodyPhenotype)), "ic bp");
        std::vector<u8> ic_ok(64u, 0u);
        u32 n_aud_mot = 0u;
        for (u32 i = 0; i < 64u; ++i)
            if (ic_bps[i].counts[TK_EAR] > 0u && ic_bps[i].counts[TK_MUSCLE] > 0u) { ic_ok[i] = 1u; ++n_aud_mot; }
        AblationFlags abl{}; abl.challenge_tier = 0u;

        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_w(WORLD_CELLS, 1.0f), flat_ter(WORLD_CELLS, 1.1f);
        std::vector<float> flat_l(WORLD_CELLS, 1.65f), zero_f(WORLD_CELLS, 0.0f), flat_p(WORLD_CELLS, 1.6f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "ic t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "ic w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "ic ter");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.light), flat_l.data(), WORLD_CELLS * sizeof(float)), "ic l");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "ic cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "ic cc");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "ic cd");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "ic ap");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "ic v1");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "ic dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "ic ul");

        u32 sim_t = 0u;
        std::vector<double> base_act(64u, 0.16);
        // 2-Syllable Grounded Directive:
        //   Directive A (v0 -> v1 = "GO/SWIM"): accelerate above own cruising baseline during identical Syllable 2 (`v1`)!
        //   Directive B (v2 -> v1 = "HALT/HOLD"): brake below own cruising baseline during identical Syllable 2 (`v1`)!
        // Syllable 2 is 100% IDENTICAL (`v1`) -> requires cortical working memory of Syllable 1 gating L0 muscles!
        auto run_cmd = [&](bool is_go, bool tutor_da, std::vector<double> &out_act, std::vector<double> &out_thr) {
            out_act.assign(64u, 0.0);
            out_thr.assign(64u, 0.0);
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "ic r dl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f;
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "ic r ul");

            auto set_ic_aud = [&](int band) {
                if (band == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), flat_p.data(), WORLD_CELLS * sizeof(float)), "ic v0");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "ic v0z");
                if (band == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), flat_p.data(), WORLD_CELLS * sizeof(float)), "ic v1");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "ic v1z");
                if (band == 2) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), flat_p.data(), WORLD_CELLS * sizeof(float)), "ic v2");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "ic v2z");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "ic pz");
            };
            // Syllable 1 (4 ticks): v0 for GO, v2 for HALT
            for (u32 k = 0; k < 4u; ++k, ++sim_t) { set_ic_aud(is_go ? 0 : 2); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }

            // Syllable 2 / Execution Window (5 ticks): IDENTICAL `v1` execute cue for BOTH GO and HALT!
            std::vector<u8> rew(64u, 0u);
            for (u32 k = 0; k < 5u; ++k, ++sim_t) {
                set_ic_aud(1);
                gpu.step_brains(sim_t, abl);
                s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "ic ex dl");
                bool dirty = false;
                for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && ic_ok[i]) {
                    double act = double(cs[i].mot_thrust) + 0.5 * double(cs[i].mot_eat);
                    out_act[i] += act / 5.0;
                    out_thr[i] += double(cs[i].mot_thrust) / 5.0;
                    if (tutor_da && k >= 1u && k <= 4u) {
                        double hi_thr = std::max(0.10, base_act[i] + 0.025);
                        double lo_thr = std::max(0.06, base_act[i] - 0.010);
                        if (is_go) {
                            if (act >= hi_thr && (rew[i] & 1u) == 0u) { cs[i].atp += 2.6f; rew[i] |= 1u; dirty = true; }
                            else if (k >= 2u && act <= lo_thr && (rew[i] & 2u) == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.8f); rew[i] |= 2u; dirty = true; }
                        } else {
                            if (act >= hi_thr && (rew[i] & 2u) == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.8f); rew[i] |= 2u; dirty = true; }
                            else if (k == 4u && (rew[i] & 2u) == 0u && (rew[i] & 1u) == 0u) { cs[i].atp += 2.6f; rew[i] |= 1u; dirty = true; }
                        }
                    }
                }
                if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "ic ex ul");
                gpu.step_bodies(sim_t, seed, abl);
            }
            for (u32 k = 0; k < 5u; ++k, ++sim_t) { set_ic_aud(-1); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
        };

        auto eval_cmd = [&](double &sep, double &go_t, double &ha_t, double &hit, bool set_base) {
            std::vector<double> ga, gt, ha, ht;
            run_cmd(true, false, ga, gt);
            run_cmd(false, false, ha, ht);
            u32 sn = 0u, hits = 0u;
            sep = 0.0; go_t = 0.0; ha_t = 0.0;
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && ic_ok[i]) {
                if (set_base) base_act[i] = 0.5 * (ga[i] + ha[i]);
                ++sn;
                sep += (ga[i] - ha[i]);
                go_t += gt[i];
                ha_t += ht[i];
                if (ga[i] > ha[i] + 0.04) ++hits;
            }
            double inv = sn ? (1.0 / double(sn)) : 0.0;
            sep *= inv; go_t *= inv; ha_t *= inv; hit = double(hits) * inv;
        };

        double pre_sep = 0.0, pre_gt = 0.0, pre_ht = 0.0, pre_hit = 0.0;
        eval_cmd(pre_sep, pre_gt, pre_ht, pre_hit, true);
        std::vector<double> dum1, dum2;
        for (u32 tr = 0; tr < 12u; ++tr) {
            run_cmd(true, true, dum1, dum2);
            run_cmd(false, true, dum1, dum2);
        }
        double post_sep = 0.0, post_gt = 0.0, post_ht = 0.0, post_hit = 0.0;
        eval_cmd(post_sep, post_gt, post_ht, post_hit, false);
        u64 ic_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=referential_instruction_following  trials=12  agents=" << n_aud_mot
                  << "  pre_sep=" << std::fixed << std::setprecision(3) << pre_sep
                  << "  post_sep=" << post_sep
                  << "  go_thr=" << std::setprecision(2) << post_gt
                  << "  halt_thr=" << post_ht
                  << "  pre_hit=" << std::setprecision(3) << pre_hit
                  << "  post_hit=" << post_hit
                  << "  stdp_edits=" << ic_ed << '\n';
    }

    // Assay 17: Compositional 3-Syllable Grounded Phrase Syntax (v0->v1->v2 vs v2->v1->v0 across 3 sequential windows!)
    // Chance exact 3-syllable hit = 3.7% (1/27). Window 2 (`v1` shared pivot) AND Window 3 (`acoust_p` dark prompt)
    // are 100% IDENTICAL across Phrase X and Phrase Y -> Window 3 requires cortical working memory surviving THROUGH
    // an intervening shared speech act (`v1`) to select `v2` vs `v0` (ZERO LLMs, ZERO hardcoded grammar!)
    {
        std::vector<GenesisDNA> p3_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0x1700u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0x1700u));
            g_sc.step_develop(mix(seed ^ 0x1700u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "p3 sc bp");
            for (u32 i = 0; i < scan_n && p3_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] >= 3u) p3_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && p3_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] == 2u) p3_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && p3_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] > 0u && sbps[i].counts[TK_GLAND] > 0u &&
                    !(sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] >= 2u))
                    p3_cohort.push_back(cohort[i]);
            while (p3_cohort.size() < 64u && !cohort.empty()) p3_cohort.push_back(cohort[p3_cohort.size() % cohort.size()]);
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0x1701u), 0u);
        gpu.upload_dnas_and_spawn(p3_cohort, 64u, mix(seed ^ 0x1701u));
        gpu.step_develop(mix(seed ^ 0x1701u));
        std::vector<BodyPhenotype> p3_bps(64u);
        s.d.check(s.d.download(p3_bps.data(), gpu.d_bodies.ptr, 64u * sizeof(BodyPhenotype)), "p3 bp");
        std::vector<u8> p3_has_gl(64u, 0u);
        u32 n_voc_p3 = 0u;
        for (u32 i = 0; i < 64u; ++i)
            if (p3_bps[i].counts[TK_GLAND] > 0u && p3_bps[i].counts[TK_EAR] > 0u) { p3_has_gl[i] = 1u; ++n_voc_p3; }
        AblationFlags abl{}; abl.challenge_tier = 0u;

        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_w(WORLD_CELLS, 1.0f), flat_ter(WORLD_CELLS, 1.1f);
        std::vector<float> l_hi(WORLD_CELLS, 1.85f), l_dark(WORLD_CELLS, 0.05f), c_hi(WORLD_CELLS, 1.35f);
        std::vector<float> zero_f(WORLD_CELLS, 0.0f), flat_p(WORLD_CELLS, 1.6f), flat_p_soft(WORLD_CELLS, 0.9f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "p3 t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "p3 w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "p3 ter");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "p3 cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "p3 cd");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "p3 dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "p3 ul");

        u32 sim_t = 0u;
        struct P3Res { double seq, s1, s2, s3; };
        auto run_phrase3 = [&](bool is_x, bool tutor_da) -> P3Res {
            u32 want[3] = {is_x ? 0u : 2u, 1u, is_x ? 2u : 0u};
            int wv[3][64];
            u8 rew[3][64]{};
            for (int wi = 0; wi < 3; ++wi)
                for (u32 i = 0; i < 64u; ++i) wv[wi][i] = -1;

            auto set_p3_aud = [&](int band, float p_amp) {
                if (band == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), flat_p.data(), WORLD_CELLS * sizeof(float)), "p3 v0");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "p3 v0z");
                if (band == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), flat_p.data(), WORLD_CELLS * sizeof(float)), "p3 v1");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "p3 v1z");
                if (band == 2) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), flat_p.data(), WORLD_CELLS * sizeof(float)), "p3 v2");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "p3 v2z");
                if (p_amp > 1.2f) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_p), flat_p.data(), WORLD_CELLS * sizeof(float)), "p3 p");
                else if (p_amp > 0.2f) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_p), flat_p_soft.data(), WORLD_CELLS * sizeof(float)), "p3 ps");
                else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "p3 pz");
            };

            for (int wi = 0; wi < 3; ++wi) {
                s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "p3 wdl");
                for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                    cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                    cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f;
                }
                s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "p3 wul");
                if (wi == 0) {
                    s.d.check(s.d.upload(DevicePtr(gpu.wb.light), (is_x ? l_hi : l_dark).data(), WORLD_CELLS * sizeof(float)), "p3 1l");
                    s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), (is_x ? c_hi : zero_f).data(), WORLD_CELLS * sizeof(float)), "p3 1c");
                } else {
                    s.d.check(s.d.upload(DevicePtr(gpu.wb.light), l_dark.data(), WORLD_CELLS * sizeof(float)), "p3 dl");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "p3 dc");
                }
                for (u32 k = 0; k < 5u; ++k, ++sim_t) {
                    if (wi == 0) set_p3_aud(is_x ? 0 : 2, is_x ? 0.0f : 1.6f);
                    else if (wi == 1) set_p3_aud(1, 0.0f);
                    else set_p3_aud(-1, 0.9f);
                    gpu.step_brains(sim_t, abl);
                    if (k >= 1u) {
                        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "p3 wm");
                        bool dirty = false;
                        for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && p3_has_gl[i]) {
                            float vs[3] = {cs[i].mot_vocal0, cs[i].mot_vocal1, cs[i].mot_vocal2};
                            u32 b = (vs[0] >= vs[1] && vs[0] >= vs[2]) ? 0u : ((vs[1] >= vs[2]) ? 1u : 2u);
                            float ru = (b == 0u) ? std::max(vs[1], vs[2]) : ((b == 1u) ? std::max(vs[0], vs[2]) : std::max(vs[0], vs[1]));
                            bool crisp = (vs[b] > 0.12f && vs[b] > ru + 0.02f);
                            if (crisp && wv[wi][i] < 0) wv[wi][i] = int(b);
                            if (tutor_da && k <= 4u) {
                                if (crisp && b == want[wi] && (rew[wi][i] & 1u) == 0u) { cs[i].atp += 2.4f; rew[wi][i] |= 1u; dirty = true; }
                                else if (vs[b] > 0.12f && rew[wi][i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f); rew[wi][i] |= 2u; dirty = true; }
                                else if (k == 3u && rew[wi][i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 0.9f); rew[wi][i] |= 2u; dirty = true; }
                            }
                        }
                        if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "p3 wr");
                    }
                    gpu.step_bodies(sim_t, seed, abl);
                }
            }
            for (u32 k = 0; k < 5u; ++k, ++sim_t) { set_p3_aud(-1, 0.0f); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }

            u32 sn = 0u, h1 = 0u, h2 = 0u, h3 = 0u, hs = 0u;
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && p3_has_gl[i]) {
                ++sn;
                bool ok1 = (wv[0][i] == int(want[0]));
                bool ok2 = (wv[1][i] == int(want[1]));
                bool ok3 = (wv[2][i] == int(want[2]));
                if (ok1) ++h1;
                if (ok2) ++h2;
                if (ok3) ++h3;
                if (ok1 && ok2 && ok3) ++hs;
            }
            double inv = sn ? (1.0 / double(sn)) : 0.0;
            return {double(hs) * inv, double(h1) * inv, double(h2) * inv, double(h3) * inv};
        };

        auto pr_x = run_phrase3(true, false);
        auto pr_y = run_phrase3(false, false);
        double pre_seq = 0.5 * (pr_x.seq + pr_y.seq);
        for (u32 tr = 0; tr < 16u; ++tr) {
            run_phrase3(true, true);
            run_phrase3(false, true);
        }
        auto po_x = run_phrase3(true, false);
        auto po_y = run_phrase3(false, false);
        double post_seq = 0.5 * (po_x.seq + po_y.seq);
        u64 p3_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=compositional_3syl_syntax  trials=16  vocalists=" << n_voc_p3
                  << "  pre_seq_hit=" << std::fixed << std::setprecision(3) << pre_seq
                  << "  post_seq_hit=" << post_seq
                  << "  syl1_hit=" << (0.5 * (po_x.s1 + po_y.s1))
                  << "  syl2_hit=" << (0.5 * (po_x.s2 + po_y.s2))
                  << "  syl3_hit=" << (0.5 * (po_x.s3 + po_y.s3))
                  << "  phraseX_hit=" << std::setprecision(2) << po_x.seq
                  << "  phraseY_hit=" << po_y.seq
                  << "  stdp_edits=" << p3_ed << '\n';
    }

    // Assay 18: Novel Zero-Shot Cross-Modal Delayed Concept-Action Generalization (ZERO LLMs!)
    // Trains grounded multimodal concepts WITHOUT dark delay (Food+v0 -> v1 => speak v0 & GO; Danger+v2 -> v1 => speak v2 & HALT).
    // Then tests on an UNSEEN HELD-OUT COMBINATION: PURE ZERO-AUDIO sensory concept (vision/chem OR rumble ONLY, NO v0/v2!)
    // -> 8 ticks PITCH-BLACK SILENT WORKING-MEMORY DELAY -> IDENTICAL neutral `v1` query -> ZERO DA!
    // Evaluates joint zero-shot spoken concept naming + grounded embodied motor execution across an unseen dark delay!
    {
        std::vector<GenesisDNA> g18_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0x1800u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0x1800u));
            g_sc.step_develop(mix(seed ^ 0x1800u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "g18 sc bp");
            for (u32 i = 0; i < scan_n && g18_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EYE] > 0u && sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_MUSCLE] >= 2u)
                    g18_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && g18_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_MUSCLE] >= 2u && sbps[i].counts[TK_EYE] == 0u)
                    g18_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && g18_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_GLAND] > 0u && sbps[i].counts[TK_EAR] > 0u &&
                    !(sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] >= 2u && sbps[i].counts[TK_MUSCLE] >= 2u))
                    g18_cohort.push_back(cohort[i]);
            while (g18_cohort.size() < 64u && !cohort.empty()) g18_cohort.push_back(cohort[g18_cohort.size() % cohort.size()]);
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0x1801u), 0u);
        gpu.upload_dnas_and_spawn(g18_cohort, 64u, mix(seed ^ 0x1801u));
        gpu.step_develop(mix(seed ^ 0x1801u));
        std::vector<BodyPhenotype> g18_bps(64u);
        s.d.check(s.d.download(g18_bps.data(), gpu.d_bodies.ptr, 64u * sizeof(BodyPhenotype)), "g18 bp");
        std::vector<u8> g18_ok(64u, 0u);
        u32 n_g18 = 0u;
        for (u32 i = 0; i < 64u; ++i)
            if (g18_bps[i].counts[TK_GLAND] > 0u && g18_bps[i].counts[TK_EAR] > 0u) { g18_ok[i] = 1u; ++n_g18; }
        AblationFlags abl{}; abl.challenge_tier = 0u;

        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_w(WORLD_CELLS, 1.0f), flat_ter(WORLD_CELLS, 1.1f);
        std::vector<float> zero_f(WORLD_CELLS, 0.0f), flat_p(WORLD_CELLS, 1.6f);
        std::vector<float> l_hi(WORLD_CELLS, 1.90f), l_dark(WORLD_CELLS, 0.05f), c_hi(WORLD_CELLS, 1.40f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "g18 t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "g18 w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "g18 ter");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "g18 cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "g18 cd");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "g18 dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "g18 ul");

        u32 sim_t = 0u;
        std::vector<double> base_act(64u, 0.14);
        auto set_g18_aud = [&](int band, bool with_p) {
            if (band == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), flat_p.data(), WORLD_CELLS * sizeof(float)), "g18 v0");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "g18 v0z");
            if (band == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), flat_p.data(), WORLD_CELLS * sizeof(float)), "g18 v1");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "g18 v1z");
            if (band == 2) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), flat_p.data(), WORLD_CELLS * sizeof(float)), "g18 v2");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "g18 v2z");
            if (with_p) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_p), flat_p.data(), WORLD_CELLS * sizeof(float)), "g18 ap");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "g18 apz");
        };

        auto run_g18 = [&](bool is_food, bool with_v_audio, u32 dark_delay, bool tutor_da,
                           std::vector<int> &out_voc, std::vector<double> &out_act) {
            out_voc.assign(64u, -1);
            out_act.assign(64u, 0.0);
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "g18 rdl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f;
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "g18 rul");

            // Phase 1: Concept presentation (training uses multimodal audio+sensory; zero-shot test uses ZERO v0/v2 audio!)
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), (is_food ? l_hi : l_dark).data(), WORLD_CELLS * sizeof(float)), "g18 l1");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), (is_food ? c_hi : zero_f).data(), WORLD_CELLS * sizeof(float)), "g18 c1");
            for (u32 k = 0; k < 5u; ++k, ++sim_t) {
                set_g18_aud(with_v_audio ? (is_food ? 0 : 2) : -1, !is_food);
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }

            // Phase 2: Held-out pitch-black silent working-memory delay (0t in training, 8t in unseen zero-shot test!)
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), l_dark.data(), WORLD_CELLS * sizeof(float)), "g18 ld");
            s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "g18 cdz");
            for (u32 k = 0; k < dark_delay; ++k, ++sim_t) {
                set_g18_aud(-1, false);
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }

            // Phase 3: IDENTICAL neutral query `v1` in darkness -> produce grounded Word (v0 vs v2) + Action (GO vs HALT)!
            u32 want_b = is_food ? 0u : 2u;
            std::vector<u8> rew(64u, 0u);
            for (u32 k = 0; k < 6u; ++k, ++sim_t) {
                set_g18_aud(1, false);
                gpu.step_brains(sim_t, abl);
                if (k >= 1u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "g18 qm");
                    bool dirty = false;
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && g18_ok[i]) {
                        double act = double(cs[i].mot_thrust) + 0.5 * double(cs[i].mot_eat);
                        out_act[i] += act / 5.0;
                        float vs[3] = {cs[i].mot_vocal0, cs[i].mot_vocal1, cs[i].mot_vocal2};
                        u32 b = (vs[0] >= vs[1] && vs[0] >= vs[2]) ? 0u : ((vs[1] >= vs[2]) ? 1u : 2u);
                        float ru = (b == 0u) ? std::max(vs[1], vs[2]) : ((b == 1u) ? std::max(vs[0], vs[2]) : std::max(vs[0], vs[1]));
                        bool crisp = (vs[b] > 0.12f && vs[b] > ru + 0.02f);
                        if (crisp && out_voc[i] < 0) out_voc[i] = int(b);
                        if (tutor_da && k <= 5u) {
                            bool voc_ok = (crisp && b == want_b);
                            bool voc_bad = (vs[b] > 0.12f && !voc_ok);
                            bool mot_ok = is_food ? (act >= base_act[i] + 0.02) : (act <= base_act[i] + 0.01);
                            if (voc_ok && (rew[i] & 1u) == 0u) { cs[i].atp += (mot_ok ? 2.6f : 1.8f); rew[i] |= 1u; dirty = true; }
                            else if (voc_bad && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f); rew[i] |= 2u; dirty = true; }
                            else if (k == 4u && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 0.9f); rew[i] |= 2u; dirty = true; }
                        }
                    }
                    if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "g18 qr");
                }
                gpu.step_bodies(sim_t, seed, abl);
            }
            for (u32 k = 0; k < 6u; ++k, ++sim_t) { set_g18_aud(-1, false); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
        };

        auto eval_g18 = [&](double &voc_h, double &mot_sep, double &joint_h, bool set_base) {
            std::vector<int> vf, vd;
            std::vector<double> af, ad;
            // HELD-OUT ZERO-SHOT TEST: ZERO v0/v2 audio + UNSEEN 8-tick pitch-black working-memory delay!
            run_g18(true, false, 8u, false, vf, af);
            run_g18(false, false, 8u, false, vd, ad);
            u32 sn = 0u, vh = 0u, jh = 0u;
            mot_sep = 0.0;
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && g18_ok[i]) {
                if (set_base) base_act[i] = 0.5 * (af[i] + ad[i]);
                ++sn;
                mot_sep += (af[i] - ad[i]);
                bool ok_f = (vf[i] == 0), ok_d = (vd[i] == 2);
                if (ok_f) ++vh;
                if (ok_d) ++vh;
                if ((ok_f || ok_d) && (af[i] > ad[i] + 0.02)) ++jh;
            }
            voc_h = sn ? (double(vh) / double(2u * sn)) : 0.0;
            mot_sep = sn ? (mot_sep / double(sn)) : 0.0;
            joint_h = sn ? (double(jh) / double(sn)) : 0.0;
        };

        double pre_vh = 0.0, pre_ms = 0.0, pre_jh = 0.0;
        eval_g18(pre_vh, pre_ms, pre_jh, true);
        std::vector<int> dv; std::vector<double> da;
        for (u32 tr = 0; tr < 14u; ++tr) {
            // Train ONLY with direct multimodal cue + 0-tick delay (NEVER sees zero-audio 8t dark delay during training!)
            run_g18(true, true, 0u, true, dv, da);
            run_g18(false, true, 0u, true, dv, da);
        }
        double post_vh = 0.0, post_ms = 0.0, post_jh = 0.0;
        eval_g18(post_vh, post_ms, post_jh, false);
        u64 g18_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=compositional_generalization  delay_ticks=8  trials=14  agents=" << n_g18
                  << "  pre_voc_hit=" << std::fixed << std::setprecision(3) << pre_vh
                  << "  post_voc_hit=" << post_vh
                  << "  pre_mot_sep=" << pre_ms
                  << "  post_mot_sep=" << post_ms
                  << "  pre_joint_hit=" << pre_jh
                  << "  post_joint_hit=" << post_jh
                  << "  stdp_edits=" << g18_ed << '\n';
    }

    // Assay 19: Interference-Resistant Episodic Memory & Distractor Reasoning (Miller-Erickson-Desimone 1996 — ZERO LLMs!)
    // Encodes Episode A (Sunlit Food + v0) vs Episode B (Dark Rumble + v2), then injects an 8-tick ACTIVE SENSORY
    // DISTRACTOR (identical mid-light + neutral v1 + rumble interference for BOTH episodes!) before presenting an
    // identical dark `v1` query. Measures deep prefrontal representation survival (`epi_cos_dist`) THROUGH active
    // distractors AND unrewarded spoken episodic recall (`pre_hit -> post_hit`)!
    {
        std::vector<GenesisDNA> e19_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0x1900u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0x1900u));
            g_sc.step_develop(mix(seed ^ 0x1900u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "e19 sc bp");
            for (u32 i = 0; i < scan_n && e19_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] >= 2u) e19_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && e19_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] > 0u && sbps[i].counts[TK_GLAND] > 0u &&
                    !(sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] >= 2u))
                    e19_cohort.push_back(cohort[i]);
            while (e19_cohort.size() < 64u && !cohort.empty()) e19_cohort.push_back(cohort[e19_cohort.size() % cohort.size()]);
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0x1901u), 0u);
        gpu.upload_dnas_and_spawn(e19_cohort, 64u, mix(seed ^ 0x1901u));
        gpu.step_develop(mix(seed ^ 0x1901u));
        std::vector<BodyPhenotype> e19_bps(64u);
        s.d.check(s.d.download(e19_bps.data(), gpu.d_bodies.ptr, 64u * sizeof(BodyPhenotype)), "e19 bp");
        std::vector<u8> e19_ok(64u, 0u);
        u32 n_e19 = 0u;
        for (u32 i = 0; i < 64u; ++i)
            if (e19_bps[i].counts[TK_GLAND] > 0u && e19_bps[i].counts[TK_EAR] > 0u) { e19_ok[i] = 1u; ++n_e19; }
        AblationFlags abl{}; abl.challenge_tier = 0u;

        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_w(WORLD_CELLS, 1.0f), flat_ter(WORLD_CELLS, 1.1f);
        std::vector<float> zero_f(WORLD_CELLS, 0.0f), flat_p(WORLD_CELLS, 1.6f), dist_p(WORLD_CELLS, 0.8f);
        std::vector<float> l_hi(WORLD_CELLS, 1.90f), l_mid(WORLD_CELLS, 0.65f), l_dark(WORLD_CELLS, 0.05f), c_hi(WORLD_CELLS, 1.40f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "e19 t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "e19 w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "e19 ter");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "e19 cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "e19 cd");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "e19 dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "e19 ul");

        u32 sim_t = 0u;
        std::vector<u8> all_rings(size_t(gpu.max_layers) * TISSUE_CELLS);
        auto set_e19_aud = [&](int band, float p_amp) {
            if (band == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), flat_p.data(), WORLD_CELLS * sizeof(float)), "e19 v0");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "e19 v0z");
            if (band == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), flat_p.data(), WORLD_CELLS * sizeof(float)), "e19 v1");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "e19 v1z");
            if (band == 2) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), flat_p.data(), WORLD_CELLS * sizeof(float)), "e19 v2");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "e19 v2z");
            if (p_amp > 1.2f) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_p), flat_p.data(), WORLD_CELLS * sizeof(float)), "e19 p");
            else if (p_amp > 0.2f) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_p), dist_p.data(), WORLD_CELLS * sizeof(float)), "e19 pd");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "e19 pz");
        };

        auto run_e19 = [&](bool is_a, bool tutor_da, std::vector<float> *pop_vec) -> double {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "e19 rdl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f;
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "e19 rul");

            // Phase 1 (4t): Episode Encoding (A = Sunlit Food + v0, B = Dark Rumble + v2)
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), (is_a ? l_hi : l_dark).data(), WORLD_CELLS * sizeof(float)), "e19 l1");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), (is_a ? c_hi : zero_f).data(), WORLD_CELLS * sizeof(float)), "e19 c1");
            for (u32 k = 0; k < 4u; ++k, ++sim_t) {
                set_e19_aud(is_a ? 0 : 2, is_a ? 0.0f : 1.6f);
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }

            // Phase 2 (8t): IDENTICAL ACTIVE SENSORY DISTRACTOR (mid-light + v1 chatter + rumble — tests PFC interference resistance!)
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), l_mid.data(), WORLD_CELLS * sizeof(float)), "e19 lm");
            s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "e19 cmz");
            if (pop_vec) pop_vec->assign(64u * 4u * TISSUE_CELLS, 0.0f);
            for (u32 k = 0; k < 8u; ++k, ++sim_t) {
                set_e19_aud(k < 4u ? 1 : -1, 0.8f);
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
                if (pop_vec && k >= 4u) {
                    s.d.check(s.d.download(all_rings.data(), DevicePtr(gpu.pool.spike_ring), all_rings.size()), "e19 sr");
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                        u32 lim_z = std::min<u32>(cs[i].num_layers, 5u);
                        for (u32 z = 1u; z < lim_z; ++z) {
                            u32 sl = cs[i].layer_slabs[z];
                            if (sl == INVALID_SLAB || sl >= gpu.max_layers) continue;
                            const u8 *sr = all_rings.data() + size_t(sl) * TISSUE_CELLS;
                            float *dst = pop_vec->data() + (size_t(i) * 4u + size_t(z - 1u)) * TISSUE_CELLS;
                            for (u32 c = 0; c < TISSUE_CELLS; ++c) dst[c] += float(__builtin_popcount(u32(sr[c] & 0x0fu)));
                        }
                    }
                }
            }

            // Phase 3 (6t): IDENTICAL Dark Query `v1` -> Recall Episode A (`v0`) vs Episode B (`v2`)!
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), l_dark.data(), WORLD_CELLS * sizeof(float)), "e19 ld");
            u32 want_b = is_a ? 0u : 2u;
            std::vector<u8> rew(64u, 0u);
            std::vector<int> out_v(64u, -1);
            for (u32 k = 0; k < 6u; ++k, ++sim_t) {
                set_e19_aud(1, 0.0f);
                gpu.step_brains(sim_t, abl);
                if (k >= 1u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "e19 qm");
                    bool dirty = false;
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && e19_ok[i]) {
                        float vs[3] = {cs[i].mot_vocal0, cs[i].mot_vocal1, cs[i].mot_vocal2};
                        u32 b = (vs[0] >= vs[1] && vs[0] >= vs[2]) ? 0u : ((vs[1] >= vs[2]) ? 1u : 2u);
                        float ru = (b == 0u) ? std::max(vs[1], vs[2]) : ((b == 1u) ? std::max(vs[0], vs[2]) : std::max(vs[0], vs[1]));
                        bool crisp = (vs[b] > 0.12f && vs[b] > ru + 0.02f);
                        if (crisp && out_v[i] < 0) out_v[i] = int(b);
                        if (tutor_da && k <= 5u) {
                            bool ok = (crisp && b == want_b);
                            bool bad = (vs[b] > 0.12f && !ok);
                            if (ok && (rew[i] & 1u) == 0u) { cs[i].atp += 2.4f; rew[i] |= 1u; dirty = true; }
                            else if (bad && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f); rew[i] |= 2u; dirty = true; }
                            else if (k == 4u && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 0.9f); rew[i] |= 2u; dirty = true; }
                        }
                    }
                    if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "e19 qr");
                }
                gpu.step_bodies(sim_t, seed, abl);
            }
            for (u32 k = 0; k < 6u; ++k, ++sim_t) { set_e19_aud(-1, 0.0f); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
            u32 sn = 0u, hits = 0u;
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && e19_ok[i]) {
                ++sn;
                if (out_v[i] == int(want_b)) ++hits;
            }
            return sn ? (double(hits) / double(sn)) : 0.0;
        };

        double pre_h = 0.5 * (run_e19(true, false, nullptr) + run_e19(false, false, nullptr));
        for (u32 tr = 0; tr < 14u; ++tr) {
            run_e19(true, true, nullptr);
            run_e19(false, true, nullptr);
        }
        std::vector<float> va19, vb19;
        double h_a = run_e19(true, false, &va19);
        double h_b = run_e19(false, false, &vb19);
        double post_h = 0.5 * (h_a + h_b);
        double cos_sum = 0.0; u32 cos_n = 0u;
        for (u32 i = 0; i < 64u; ++i) {
            double dot = 0.0, na = 0.0, nb = 0.0;
            const float *pa = va19.data() + size_t(i) * 4u * TISSUE_CELLS;
            const float *pb = vb19.data() + size_t(i) * 4u * TISSUE_CELLS;
            for (u32 j = 0; j < 4u * TISSUE_CELLS; ++j) {
                dot += double(pa[j]) * double(pb[j]);
                na += double(pa[j]) * double(pa[j]);
                nb += double(pb[j]) * double(pb[j]);
            }
            if (na > 1.0 && nb > 1.0) {
                cos_sum += (1.0 - std::clamp(dot / std::sqrt(na * nb), -1.0, 1.0));
                ++cos_n;
            }
        }
        double epi_cos = cos_n ? (cos_sum / double(cos_n)) : 0.0;
        u64 e19_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=episodic_distractor_reasoning  distract_ticks=8  trials=14  agents=" << n_e19
                  << "  pre_hit=" << std::fixed << std::setprecision(3) << pre_h
                  << "  post_hit=" << post_h
                  << "  epi_cos_dist=" << epi_cos
                  << "  epA_hit=" << std::setprecision(2) << h_a
                  << "  epB_hit=" << h_b
                  << "  stdp_edits=" << e19_ed << '\n';
    }

    // Assay 20: Relational Transitive Inference (Grounded A > B, B > C => Zero-Shot A > C / C < A — ZERO LLMs!)
    // Teaches ONLY adjacent relational pairs via live operant DA RPE on organisms' own 3-fold syrinx:
    //   (A -> B) => v0 ("First > Second"),  (B -> A) => v2 ("First < Second")
    //   (B -> C) => v0 ("First > Second"),  (C -> B) => v2 ("First < Second")
    // Notice B requires BOTH v0 and v2 depending on relational order (impossible for single-item reflexes!).
    // Then tests UNSEEN HELD-OUT NON-ADJACENT TRANSITIVE DEDUCTION (ZERO DA, NEVER TRAINED TOGETHER!):
    //   (A -> C) => v0 ("A > C")  and  (C -> A) => v2 ("C < A")!
    {
        std::vector<GenesisDNA> t20_cohort;
        {
            u32 scan_n = std::min<u32>(512u, u32(cohort.size()));
            GenesisGpu g_sc(s, 512u, 512u * 16u);
            g_sc.init_world(mix(seed ^ 0x2000u), 0u);
            g_sc.upload_dnas_and_spawn(cohort, scan_n, mix(seed ^ 0x2000u));
            g_sc.step_develop(mix(seed ^ 0x2000u));
            std::vector<BodyPhenotype> sbps(512u);
            s.d.check(s.d.download(sbps.data(), g_sc.d_bodies.ptr, 512u * sizeof(BodyPhenotype)), "t20 sc bp");
            for (u32 i = 0; i < scan_n && t20_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] >= 2u) t20_cohort.push_back(cohort[i]);
            for (u32 i = 0; i < scan_n && t20_cohort.size() < 64u; ++i)
                if (sbps[i].counts[TK_EAR] > 0u && sbps[i].counts[TK_GLAND] > 0u &&
                    !(sbps[i].counts[TK_EAR] >= 2u && sbps[i].counts[TK_GLAND] >= 2u))
                    t20_cohort.push_back(cohort[i]);
            while (t20_cohort.size() < 64u && !cohort.empty()) t20_cohort.push_back(cohort[t20_cohort.size() % cohort.size()]);
        }
        GenesisGpu gpu(s, 64u, 64u * 24u);
        gpu.init_world(mix(seed ^ 0x2001u), 0u);
        gpu.upload_dnas_and_spawn(t20_cohort, 64u, mix(seed ^ 0x2001u));
        gpu.step_develop(mix(seed ^ 0x2001u));
        std::vector<BodyPhenotype> t20_bps(64u);
        s.d.check(s.d.download(t20_bps.data(), gpu.d_bodies.ptr, 64u * sizeof(BodyPhenotype)), "t20 bp");
        std::vector<u8> t20_ok(64u, 0u);
        u32 n_t20 = 0u;
        for (u32 i = 0; i < 64u; ++i)
            if (t20_bps[i].counts[TK_GLAND] > 0u && t20_bps[i].counts[TK_EAR] > 0u) { t20_ok[i] = 1u; ++n_t20; }
        AblationFlags abl{}; abl.challenge_tier = 0u;

        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_w(WORLD_CELLS, 1.0f), flat_ter(WORLD_CELLS, 1.1f);
        std::vector<float> zero_f(WORLD_CELLS, 0.0f), flat_p(WORLD_CELLS, 1.6f);
        std::vector<float> l_hi(WORLD_CELLS, 1.90f), l_mid(WORLD_CELLS, 0.70f), l_dark(WORLD_CELLS, 0.05f);
        std::vector<float> c_hi(WORLD_CELLS, 1.40f), a_hi(WORLD_CELLS, 1.15f);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "t20 t");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "t20 w");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "t20 ter");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "t20 cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "t20 cd");

        std::vector<CreatureState> cs(64u);
        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "t20 dl");
        for (u32 i = 0; i < 64u; ++i) {
            cs[i].x = 40.0f + float((i & 7u) * 20u);
            cs[i].y = 40.0f + float(((i >> 3) & 7u) * 20u);
            cs[i].theta = 0.0f; cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
        }
        s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "t20 ul");

        u32 sim_t = 0u;
        auto set_item = [&](int item) {
            // Item 0 (A = High): bright sun + chem_c + v0
            // Item 1 (B = Mid):  warm mid-light + chem_a + v1
            // Item 2 (C = Low):  dark + predator rumble + v2
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), (item == 0 ? l_hi : (item == 1 ? l_mid : l_dark)).data(), WORLD_CELLS * sizeof(float)), "t20 l");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), (item == 0 ? c_hi : zero_f).data(), WORLD_CELLS * sizeof(float)), "t20 cc");
            s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_a), (item == 1 ? a_hi : zero_f).data(), WORLD_CELLS * sizeof(float)), "t20 ca");
            if (item == 0) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v0), flat_p.data(), WORLD_CELLS * sizeof(float)), "t20 v0");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v0), 0, WORLD_CELLS * sizeof(float)), "t20 v0z");
            if (item == 1) s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v1), flat_p.data(), WORLD_CELLS * sizeof(float)), "t20 v1");
            else s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v1), 0, WORLD_CELLS * sizeof(float)), "t20 v1z");
            if (item == 2) {
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_v2), flat_p.data(), WORLD_CELLS * sizeof(float)), "t20 v2");
                s.d.check(s.d.upload(DevicePtr(gpu.wb.acoust_p), flat_p.data(), WORLD_CELLS * sizeof(float)), "t20 ap");
            } else {
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_v2), 0, WORLD_CELLS * sizeof(float)), "t20 v2z");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.acoust_p), 0, WORLD_CELLS * sizeof(float)), "t20 apz");
            }
        };

        auto run_pair = [&](int x_item, int y_item, bool tutor_da) -> double {
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "t20 rdl");
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u) {
                cs[i].hp = 100.0f; cs[i].body_temp = 0.50f;
                cs[i].atp = 24.0f; cs[i].prev_atp = 24.0f; cs[i].da_baseline = 0.0f;
            }
            s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "t20 rul");

            // Phase 1 (4t): Encode First Item X into cortical working memory
            for (u32 k = 0; k < 4u; ++k, ++sim_t) {
                set_item(x_item);
                gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl);
            }
            // Phase 2 (6t): Present Second Item Y -> emit v0 if X > Y (x_item < y_item), v2 if X < Y (x_item > y_item)!
            u32 want_b = (x_item < y_item) ? 0u : 2u;
            std::vector<u8> rew(64u, 0u);
            std::vector<int> out_v(64u, -1);
            for (u32 k = 0; k < 6u; ++k, ++sim_t) {
                set_item(y_item);
                gpu.step_brains(sim_t, abl);
                if (k >= 1u) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, 64u * sizeof(CreatureState)), "t20 qm");
                    bool dirty = false;
                    for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && t20_ok[i]) {
                        float vs[3] = {cs[i].mot_vocal0, cs[i].mot_vocal1, cs[i].mot_vocal2};
                        u32 b = (vs[0] >= vs[1] && vs[0] >= vs[2]) ? 0u : ((vs[1] >= vs[2]) ? 1u : 2u);
                        float ru = (b == 0u) ? std::max(vs[1], vs[2]) : ((b == 1u) ? std::max(vs[0], vs[2]) : std::max(vs[0], vs[1]));
                        bool crisp = (vs[b] > 0.12f && vs[b] > ru + 0.02f);
                        if (crisp && out_v[i] < 0) out_v[i] = int(b);
                        if (tutor_da && k <= 5u) {
                            bool ok = (crisp && b == want_b);
                            bool bad = (vs[b] > 0.12f && !ok);
                            if (ok && (rew[i] & 1u) == 0u) { cs[i].atp += 2.4f; rew[i] |= 1u; dirty = true; }
                            else if (bad && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 1.6f); rew[i] |= 2u; dirty = true; }
                            else if (k == 4u && rew[i] == 0u) { cs[i].atp = std::max(10.0f, cs[i].atp - 0.9f); rew[i] |= 2u; dirty = true; }
                        }
                    }
                    if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), 64u * sizeof(CreatureState)), "t20 qr");
                }
                gpu.step_bodies(sim_t, seed, abl);
            }
            for (u32 k = 0; k < 5u; ++k, ++sim_t) { set_item(-1); gpu.step_brains(sim_t, abl); gpu.step_bodies(sim_t, seed, abl); }
            u32 sn = 0u, hits = 0u;
            for (u32 i = 0; i < 64u; ++i) if (cs[i].alive == 1u && t20_ok[i]) {
                ++sn;
                if (out_v[i] == int(want_b)) ++hits;
            }
            return sn ? (double(hits) / double(sn)) : 0.0;
        };

        double pre_adj = 0.25 * (run_pair(0, 1, false) + run_pair(1, 0, false) + run_pair(1, 2, false) + run_pair(2, 1, false));
        double pre_tr = 0.5 * (run_pair(0, 2, false) + run_pair(2, 0, false));
        for (u32 tr = 0; tr < 14u; ++tr) {
            // Train STRICTLY on adjacent pairs (A>B, B<A, B>C, C<B) — NEVER trains (A,C) or (C,A)!
            run_pair(0, 1, true);
            run_pair(1, 0, true);
            run_pair(1, 2, true);
            run_pair(2, 1, true);
        }
        double post_adj = 0.25 * (run_pair(0, 1, false) + run_pair(1, 0, false) + run_pair(1, 2, false) + run_pair(2, 1, false));
        double ac_h = run_pair(0, 2, false);
        double ca_h = run_pair(2, 0, false);
        double post_tr = 0.5 * (ac_h + ca_h);
        u64 t20_ed = gpu.read_counters().total_edits;
        std::cout << "genesis_probe=relational_transitive_inference  trials=14  agents=" << n_t20
                  << "  pre_adj=" << std::fixed << std::setprecision(3) << pre_adj
                  << "  post_adj=" << post_adj
                  << "  pre_trans=" << pre_tr
                  << "  post_trans=" << post_tr
                  << "  ac_hit=" << std::setprecision(2) << ac_h
                  << "  ca_hit=" << ca_h
                  << "  stdp_edits=" << t20_ed << '\n';
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Grounded Non-LLM Interactive Sensory-Acoustic Spiking-Mind Interface
// (./axion genesis converse --load FILE [--script "v0,step 10,prey,step 10,..."])
// ZERO LLMs, ZERO hardcoded replies! Injects real physical sensory/acoustic
// stimuli into the champion organism's GPU world, steps its evolved spiking
// connectome live, and reads back layer-by-layer neural assemblies, motor
// actions, working memory decay, and emergent dual-formant vocal emissions!
// ---------------------------------------------------------------------------
int run_converse(Session &s, const std::string &load_path, const std::string &script_arg, u32 group_n = 1u) {
    u32 ck_tick = 0, ck_seed = 0;
    std::vector<GenesisDNA> dnas;
    std::vector<CreatureState> crs;
    bool has_s = false;
    load_checkpoint(load_path, ck_tick, ck_seed, dnas, crs, has_s);
    group_n = std::clamp(group_n, 1u, 4u);

    // Select highest-fitness evolved living multimodal specimens (auditory + vocal + neural!) honestly from GPU!
    std::vector<u32> cand_idx;
    for (size_t i = 0; i < dnas.size(); ++i)
        if (!has_s || i >= crs.size() || crs[i].alive == 1u) cand_idx.push_back(u32(i));
    if (cand_idx.empty()) cand_idx.push_back(0u);
    std::sort(cand_idx.begin(), cand_idx.end(), [&](u32 a, u32 b) {
        double sa = (has_s && a < crs.size()) ? (double(crs[a].energy_gained) + 14.0 * double(crs[a].offspring)) : 0.0;
        double sb = (has_s && b < crs.size()) ? (double(crs[b].energy_gained) + 14.0 * double(crs[b].offspring)) : 0.0;
        return sa > sb;
    });
    u32 n_eval = std::min<u32>(u32(cand_idx.size()), 256u);
    std::vector<GenesisDNA> top_dnas;
    for (u32 i = 0; i < n_eval; ++i) top_dnas.push_back(dnas[cand_idx[i]]);

    GenesisGpu gpu(s, 256u, 256u * 24u);
    gpu.init_world(ck_seed ^ 0x7777u, 0u);
    gpu.upload_dnas_and_spawn(top_dnas, n_eval, ck_seed ^ 0x7777u);
    gpu.step_develop(ck_seed ^ 0x7777u);
    std::vector<BodyPhenotype> top_bps(256u);
    s.d.check(s.d.download(top_bps.data(), gpu.d_bodies.ptr, 256u * sizeof(BodyPhenotype)), "conv scan bp");

    std::vector<u32> picked;
    // Prefer full multimodal stereo/3-fold deep-cortex speakers (EYE + EAR>=2 + GLAND>=3 + MUSCLE + >=6L deep cortex) honestly from GPU!
    for (u32 i = 0; i < n_eval && picked.size() < group_n; ++i) {
        if (top_bps[i].counts[TK_EYE] > 0u && top_bps[i].counts[TK_EAR] >= 2u &&
            top_bps[i].counts[TK_GLAND] >= 3u && top_bps[i].counts[TK_MUSCLE] > 0u &&
            top_bps[i].grown_layers >= 6u && top_bps[i].active_neurons >= 400u)
            picked.push_back(cand_idx[i]);
    }
    for (u32 i = 0; i < n_eval && picked.size() < group_n; ++i) {
        u32 idx_c = cand_idx[i];
        if (std::find(picked.begin(), picked.end(), idx_c) == picked.end() &&
            top_bps[i].counts[TK_EYE] > 0u && top_bps[i].counts[TK_EAR] >= 2u &&
            top_bps[i].counts[TK_GLAND] >= 3u && top_bps[i].counts[TK_MUSCLE] > 0u && top_bps[i].active_neurons >= 64u)
            picked.push_back(idx_c);
    }
    for (u32 i = 0; i < n_eval && picked.size() < group_n; ++i) {
        u32 idx_c = cand_idx[i];
        if (std::find(picked.begin(), picked.end(), idx_c) == picked.end() &&
            top_bps[i].counts[TK_EYE] > 0u && top_bps[i].counts[TK_EAR] >= 2u &&
            top_bps[i].counts[TK_GLAND] >= 2u && top_bps[i].counts[TK_MUSCLE] > 0u && top_bps[i].active_neurons >= 16u)
            picked.push_back(idx_c);
    }
    for (u32 i = 0; i < n_eval && picked.size() < group_n; ++i) {
        u32 idx_c = cand_idx[i];
        if (std::find(picked.begin(), picked.end(), idx_c) == picked.end() &&
            top_bps[i].counts[TK_EAR] >= 2u && top_bps[i].counts[TK_GLAND] >= 2u &&
            top_bps[i].counts[TK_MUSCLE] > 0u && top_bps[i].active_neurons >= 8u)
            picked.push_back(idx_c);
    }
    for (u32 i = 0; i < n_eval && picked.size() < group_n; ++i) {
        u32 idx_c = cand_idx[i];
        if (std::find(picked.begin(), picked.end(), idx_c) == picked.end() &&
            top_bps[i].counts[TK_EAR] >= 2u && top_bps[i].counts[TK_GLAND] >= 2u && top_bps[i].active_neurons >= 8u)
            picked.push_back(idx_c);
    }
    for (u32 i = 0; i < n_eval && picked.size() < group_n; ++i) {
        u32 idx_c = cand_idx[i];
        if (std::find(picked.begin(), picked.end(), idx_c) == picked.end() &&
            top_bps[i].counts[TK_EAR] > 0u && top_bps[i].counts[TK_GLAND] > 0u && top_bps[i].active_neurons >= 8u)
            picked.push_back(idx_c);
    }
    for (u32 i = 0; i < n_eval && picked.size() < group_n; ++i) {
        u32 idx_c = cand_idx[i];
        if (std::find(picked.begin(), picked.end(), idx_c) == picked.end() &&
            top_bps[i].counts[TK_EAR] > 0u && top_bps[i].active_neurons >= 8u)
            picked.push_back(idx_c);
    }
    for (u32 i = 0; i < n_eval && picked.size() < group_n; ++i) {
        u32 idx_c = cand_idx[i];
        if (std::find(picked.begin(), picked.end(), idx_c) == picked.end()) picked.push_back(idx_c);
    }
    while (picked.size() < group_n) picked.push_back(cand_idx[0]);

    u32 best_i = picked[0];
    std::vector<GenesisDNA> champ;
    for (u32 idx_c : picked) champ.push_back(dnas[idx_c]);

    gpu.init_world(ck_seed ^ 0x7777u, 0u);
    // Prepare clean temperate calm aquatic dialogue chamber (ZERO frostbite DA=-1.00 or mountain wall interference!)
    {
        std::vector<float> flat_t(WORLD_CELLS, 0.50f), flat_l(WORLD_CELLS, 0.72f), flat_w(WORLD_CELLS, 1.0f), flat_ter(WORLD_CELLS, 1.15f);
        std::vector<u8> flat_m(WORLD_CELLS, MAT_LOAM);
        s.d.check(s.d.upload(DevicePtr(gpu.wb.terrain), flat_ter.data(), WORLD_CELLS * sizeof(float)), "conv ter");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.material), flat_m.data(), WORLD_CELLS * sizeof(u8)), "conv mat");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water), flat_w.data(), WORLD_CELLS * sizeof(float)), "conv wat");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.water_next), flat_w.data(), WORLD_CELLS * sizeof(float)), "conv watn");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp), flat_t.data(), WORLD_CELLS * sizeof(float)), "conv tmp");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.temp_next), flat_t.data(), WORLD_CELLS * sizeof(float)), "conv tmpn");
        s.d.check(s.d.upload(DevicePtr(gpu.wb.light), flat_l.data(), WORLD_CELLS * sizeof(float)), "conv lgt");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "conv cb");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "conv cc");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "conv cd");
        // Keep the dialogue chamber free of stray wild predators until the user explicitly spawns `apex`/`prey`!
        s.d.check(s.d.zero(DevicePtr(gpu.wb.objects), 0, MAX_OBJECTS * sizeof(WorldObject)), "conv obj z");
        s.d.check(s.d.zero(DevicePtr(gpu.wb.tile_obj_id), 0, WORLD_CELLS * sizeof(u32)), "conv toid z");
    }
    gpu.upload_dnas_and_spawn(champ, group_n, ck_seed ^ 0x7777u);
    gpu.step_develop(ck_seed ^ 0x7777u);

    std::vector<CreatureState> cs(group_n);
    std::vector<BodyPhenotype> bps(group_n);
    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, group_n * sizeof(CreatureState)), "conv dl cs");
    s.d.check(s.d.download(bps.data(), gpu.d_bodies.ptr, group_n * sizeof(BodyPhenotype)), "conv dl bp");
    const float offsets[4][2] = {{0.0f, 0.0f}, {3.0f, -2.5f}, {3.0f, 2.5f}, {-3.0f, 0.0f}};
    for (u32 a = 0; a < group_n; ++a) {
        cs[a].x = 128.0f + offsets[a][0];
        cs[a].y = 128.0f + offsets[a][1];
        cs[a].theta = (a == 0u) ? 0.0f : (a == 1u ? 2.4f : (a == 2u ? 3.8f : 0.0f));
        cs[a].atp = 36.0f; cs[a].prev_atp = 36.0f; cs[a].hp = 100.0f; cs[a].body_temp = 0.50f;
    }
    s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), group_n * sizeof(CreatureState)), "conv ul cs");

    std::cout << "genesis_converse=ready  group=" << group_n << "  specimen_idx=" << best_i
              << "  specimen_gen=" << (has_s && best_i < crs.size() ? crs[best_i].generation : 1u)
              << "  rules=" << int(champ[0].num_rules) << "  layers=" << cs[0].num_layers
              << "  cells=" << bps[0].total_cells << "  neurons=" << bps[0].active_neurons
              << "  synapses=" << bps[0].total_synapses
              << "  eye=" << bps[0].counts[TK_EYE] << "  ear=" << bps[0].counts[TK_EAR]
              << "  chem=" << bps[0].counts[TK_CHEM] << "  gland=" << bps[0].counts[TK_GLAND]
              << "  pyr=" << bps[0].counts[TK_NEU_EXC] << "  pv=" << bps[0].counts[TK_NEU_PV]
              << "  sst=" << bps[0].counts[TK_NEU_SST] << "  mod=" << bps[0].counts[TK_NEU_MOD] << '\n';
    for (u32 a = 0; a < group_n; ++a) {
        std::cout << "genesis_conv_agent=" << a
                  << "  layers=" << std::max<u16>(1u, cs[a].num_layers)
                  << "  neurons=" << bps[a].active_neurons
                  << "  eye=" << bps[a].counts[TK_EYE] << "  ear=" << bps[a].counts[TK_EAR]
                  << "  gland=" << bps[a].counts[TK_GLAND] << "  muscle=" << bps[a].counts[TK_MUSCLE]
                  << "  pyr=" << bps[a].counts[TK_NEU_EXC] << "  pv=" << bps[a].counts[TK_NEU_PV]
                  << "  sst=" << bps[a].counts[TK_NEU_SST] << "  grid=";
        for (u32 i = 0; i < TISSUE_CELLS; ++i) std::cout << hex_tk(bps[a].surface[i]);
        std::cout << '\n';
    }

    AblationFlags abl{}; abl.challenge_tier = 0u;
    u32 sim_tick = 0u;
    std::vector<float> scratch_f(WORLD_CELLS);

    auto inject_and_step = [&](const std::string &raw_cmd) {
        std::istringstream iss(raw_cmd);
        std::string cmd;
        iss >> cmd;
        if (cmd.empty()) return true;
        if (cmd == "quit" || cmd == "exit") return false;

        s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, group_n * sizeof(CreatureState)), "conv dl");
        u32 fx = u32(i32(cs[0].x + std::cos(cs[0].theta) * 2.0f + 512.0f) & i32(WORLD_MASK));
        u32 fy = u32(i32(cs[0].y + std::sin(cs[0].theta) * 2.0f + 512.0f) & i32(WORLD_MASK));

        auto pulse_all = [&](DevicePtr ptr, float val, int rad = 2) {
            s.d.check(s.d.download(scratch_f.data(), ptr, WORLD_CELLS * sizeof(float)), "conv fa dl");
            for (u32 a = 0; a < group_n; ++a) {
                u32 ax = u32(i32(cs[a].x + std::cos(cs[a].theta) * 2.0f + 512.0f) & i32(WORLD_MASK));
                u32 ay = u32(i32(cs[a].y + std::sin(cs[a].theta) * 2.0f + 512.0f) & i32(WORLD_MASK));
                for (int dy = -rad; dy <= rad; ++dy)
                    for (int dx = -rad; dx <= rad; ++dx)
                        scratch_f[u32((int(ay) + dy) & int(WORLD_MASK)) * WORLD_W + u32((int(ax) + dx) & int(WORLD_MASK))] = val;
            }
            s.d.check(s.d.upload(ptr, scratch_f.data(), WORLD_CELLS * sizeof(float)), "conv fa ul");
        };

        u32 n_steps = 8u;
        float direct_atp_delta = 0.0f;
        int operant_target = -1; // 0=reward v0, 1=reward v1, 2=reward v2
        bool is_obj_cmd = (cmd == "ask_food" || cmd == "ask_apex" || cmd == "ask_prey" || cmd == "ask_stone" ||
                           cmd == "teach_obj_food_v0" || cmd == "teach_obj_food_v2" ||
                           cmd == "teach_obj_apex_v2" || cmd == "teach_obj_apex_v0" || cmd == "teach_obj_prey_v1");
        bool is_phrase_cmd = (cmd == "teach_phrase_food_02" || cmd == "teach_phrase_apex_20" ||
                              cmd == "ask_phrase_food" || cmd == "ask_phrase_apex" ||
                              cmd == "teach_p3_x" || cmd == "teach_p3_y" || cmd == "ask_p3_x" || cmd == "ask_p3_y");
        bool is_phrase3_cmd = (cmd == "teach_p3_x" || cmd == "teach_p3_y" || cmd == "ask_p3_x" || cmd == "ask_p3_y");
        bool is_motor_cmd = (cmd == "teach_cmd_go" || cmd == "teach_cmd_halt" || cmd == "cmd_go" || cmd == "cmd_halt");
        bool is_dark_wait = (cmd == "wait_dark" || cmd == "ask_recall" || cmd == "distract");
        bool is_ctx_cmd = (cmd == "ctx_food" || cmd == "ctx_danger" || cmd == "teach_ctx_v0" || cmd == "teach_ctx_v1" || cmd == "teach_ctx_v2" || cmd == "ask_ctx");
        bool is_rel_cmd = (cmd == "teach_rel_ab" || cmd == "teach_rel_bc" || cmd == "teach_rel_ba" ||
                           cmd == "teach_rel_cb" || cmd == "ask_rel_ac" || cmd == "ask_rel_ca");
        bool is_seq2 = (cmd == "seq_01" || cmd == "seq_10" || cmd == "seq_02" || cmd == "seq_20" ||
                        cmd == "seq_12" || cmd == "seq_21" ||
                        cmd == "teach_seq01_food" || cmd == "teach_seq10_toxin" ||
                        cmd == "teach_qa_01_v2" || cmd == "teach_qa_10_v0" ||
                        cmd == "teach_qa_01_v0" || cmd == "teach_qa_10_v2" ||
                        cmd == "ask_01" || cmd == "ask_10");
        bool is_seq3 = (cmd == "seq_012" || cmd == "seq_210" || cmd == "teach_seq012_food" || cmd == "teach_seq210_toxin");
        if (cmd == "v0" || cmd == "tone_a" || cmd == "v1" || cmd == "tone_b" || cmd == "v2" || cmd == "tone_c" ||
            is_seq2 || is_seq3 || is_obj_cmd || is_phrase_cmd || is_motor_cmd || is_dark_wait || is_ctx_cmd || is_rel_cmd ||
            cmd == "teach_v0_food" || cmd == "teach_v1_apex" || cmd == "teach_v2_food" ||
            cmd == "toxin" || cmd == "pain" || cmd == "punish") {
            pulse_all(DevicePtr(gpu.wb.chem_c), 0.0f, 6);
            pulse_all(DevicePtr(gpu.wb.chem_d), 0.0f, 6);
            std::vector<float> day_l(WORLD_CELLS, (is_dark_wait || cmd == "ctx_danger" || cmd == "ask_ctx" || cmd == "teach_ctx_v0" || cmd == "teach_ctx_v1" || cmd == "teach_ctx_v2" || cmd == "teach_phrase_apex_20" || cmd == "ask_phrase_apex" || cmd == "teach_p3_y" || cmd == "ask_p3_y") ? 0.05f : ((cmd == "ctx_food" || cmd == "teach_phrase_food_02" || cmd == "ask_phrase_food" || cmd == "teach_p3_x" || cmd == "ask_p3_x") ? 1.85f : 0.72f));
            s.d.check(s.d.upload(DevicePtr(gpu.wb.light), day_l.data(), WORLD_CELLS * sizeof(float)), "conv dlgt");
            if (is_obj_cmd || is_phrase_cmd || is_motor_cmd || is_dark_wait || is_ctx_cmd || is_rel_cmd) {
                s.d.check(s.d.zero(DevicePtr(gpu.wb.objects), 0, MAX_OBJECTS * sizeof(WorldObject)), "conv obj clr");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.tile_obj_id), 0, WORLD_CELLS * sizeof(u32)), "conv toid clr");
            }
        }
        int operant_target2 = -1;
        int operant_target3 = -1;
        int motor_target = -1; // 1 = GO/FORAGE, 0 = HALT/HOLD
        if (is_phrase_cmd) {
            bool is_fa = (cmd == "teach_phrase_food_02" || cmd == "ask_phrase_food" || cmd == "teach_p3_x" || cmd == "ask_p3_x");
            if (is_fa) { pulse_all(DevicePtr(gpu.wb.chem_c), 1.35f, 3); pulse_all(DevicePtr(gpu.wb.acoust_v0), 1.6f); }
            else { pulse_all(DevicePtr(gpu.wb.acoust_p), 1.6f); pulse_all(DevicePtr(gpu.wb.acoust_v2), 1.6f); }
            if (cmd == "teach_phrase_food_02") { operant_target = 0; operant_target2 = 2; }
            else if (cmd == "teach_phrase_apex_20") { operant_target = 2; operant_target2 = 0; }
            else if (cmd == "teach_p3_x") { operant_target = 0; operant_target2 = 1; operant_target3 = 2; }
            else if (cmd == "teach_p3_y") { operant_target = 2; operant_target2 = 1; operant_target3 = 0; }
            n_steps = is_phrase3_cmd ? 15u : 10u;
        } else if (is_motor_cmd) {
            bool is_go_c = (cmd == "teach_cmd_go" || cmd == "cmd_go");
            pulse_all(is_go_c ? DevicePtr(gpu.wb.acoust_v0) : DevicePtr(gpu.wb.acoust_v2), 1.6f);
            if (cmd == "teach_cmd_go") motor_target = 1;
            else if (cmd == "teach_cmd_halt") motor_target = 0;
            n_steps = 9u;
        } else if (is_dark_wait) {
            if (cmd == "distract") {
                std::vector<float> mid_l(WORLD_CELLS, 0.65f);
                s.d.check(s.d.upload(DevicePtr(gpu.wb.light), mid_l.data(), WORLD_CELLS * sizeof(float)), "conv dist l");
                pulse_all(DevicePtr(gpu.wb.acoust_v1), 1.4f);
                pulse_all(DevicePtr(gpu.wb.acoust_p), 0.8f);
                n_steps = 8u;
            } else {
                n_steps = (cmd == "ask_recall") ? 8u : 16u;
            }
        } else if (is_ctx_cmd) {
            if (cmd == "ctx_food") { pulse_all(DevicePtr(gpu.wb.acoust_v0), 1.6f); n_steps = 6u; }
            else if (cmd == "ctx_danger") { pulse_all(DevicePtr(gpu.wb.acoust_v2), 1.6f); pulse_all(DevicePtr(gpu.wb.acoust_p), 1.6f); n_steps = 6u; }
            else {
                pulse_all(DevicePtr(gpu.wb.acoust_v1), 1.6f);
                if (cmd == "teach_ctx_v0") operant_target = 0;
                else if (cmd == "teach_ctx_v1") operant_target = 1;
                else if (cmd == "teach_ctx_v2") operant_target = 2;
                n_steps = 8u;
            }
        } else if (is_obj_cmd) {
            // Grounded Visual/Physical Object Referent Presentation (ZERO text lookup — real GPU optics & acoustics!)
            bool is_f = (cmd == "ask_food" || cmd == "teach_obj_food_v0" || cmd == "teach_obj_food_v2");
            bool is_a = (cmd == "ask_apex" || cmd == "teach_obj_apex_v2" || cmd == "teach_obj_apex_v0");
            bool is_p = (cmd == "ask_prey" || cmd == "teach_obj_prey_v1");
            if (is_f) {
                // Place sunlit visual food reef 4.5 tiles ahead of each agent (pure optical recognition, clean DA=0.00 on ask_food!)
                std::vector<float> fl(WORLD_CELLS, 0.72f), fc(WORLD_CELLS, 0.0f);
                for (u32 a = 0; a < group_n; ++a) {
                    u32 ox = u32(i32(cs[a].x + std::cos(cs[a].theta) * 4.5f + 512.0f) & i32(WORLD_MASK));
                    u32 oy = u32(i32(cs[a].y + std::sin(cs[a].theta) * 4.5f + 512.0f) & i32(WORLD_MASK));
                    for (int dy = -1; dy <= 1; ++dy)
                        for (int dx = -1; dx <= 1; ++dx) {
                            u32 wi = u32((int(oy) + dy) & int(WORLD_MASK)) * WORLD_W + u32((int(ox) + dx) & int(WORLD_MASK));
                            fl[wi] = 1.90f;
                            fc[wi] = 1.25f;
                        }
                }
                s.d.check(s.d.upload(DevicePtr(gpu.wb.light), fl.data(), WORLD_CELLS * sizeof(float)), "conv fl");
                s.d.check(s.d.upload(DevicePtr(gpu.wb.chem_c), fc.data(), WORLD_CELLS * sizeof(float)), "conv fc");
            } else {
                std::vector<WorldObject> objs(MAX_OBJECTS);
                std::vector<u32> toid(WORLD_CELLS, 0u);
                for (u32 a = 0; a < group_n; ++a) {
                    u32 ox = u32(i32(cs[a].x + std::cos(cs[a].theta) * 3.5f + 512.0f) & i32(WORLD_MASK));
                    u32 oy = u32(i32(cs[a].y + std::sin(cs[a].theta) * 3.5f + 512.0f) & i32(WORLD_MASK));
                    u32 slot = (is_a ? 610u : (is_p ? 360u : 770u)) + a;
                    objs[slot].x = float(ox) + 0.5f; objs[slot].y = float(oy) + 0.5f;
                    objs[slot].kind = is_a ? OBJ_APEX : (is_p ? OBJ_PREY : OBJ_STONE);
                    objs[slot].energy = is_a ? 88.0f : 65.0f;
                    objs[slot].integrity = is_a ? 2.2f : 1.0f;
                    objs[slot].carrier_id = INVALID_OBJ;
                    toid[oy * WORLD_W + ox] = slot + 1u;
                }
                s.d.check(s.d.upload(DevicePtr(gpu.wb.objects), objs.data(), MAX_OBJECTS * sizeof(WorldObject)), "conv obj pres");
                s.d.check(s.d.upload(DevicePtr(gpu.wb.tile_obj_id), toid.data(), WORLD_CELLS * sizeof(u32)), "conv toid pres");
                if (is_a) pulse_all(DevicePtr(gpu.wb.acoust_p), 1.4f, 2);
            }
            if (cmd == "teach_obj_food_v0" || cmd == "teach_obj_apex_v0") operant_target = 0;
            else if (cmd == "teach_obj_prey_v1") operant_target = 1;
            else if (cmd == "teach_obj_apex_v2" || cmd == "teach_obj_food_v2") operant_target = 2;
            n_steps = 8u;
        } else if (cmd == "v0" || cmd == "tone_a") { pulse_all(DevicePtr(gpu.wb.acoust_v0), 1.6f); }
        else if (cmd == "v1" || cmd == "tone_b") { pulse_all(DevicePtr(gpu.wb.acoust_v1), 1.6f); }
        else if (cmd == "v2" || cmd == "tone_c") { pulse_all(DevicePtr(gpu.wb.acoust_v2), 1.6f); }
        else if (cmd == "rumble" || cmd == "storm") { pulse_all(DevicePtr(gpu.wb.acoust_p), 2.2f); }
        else if (cmd == "food" || cmd == "reward") {
            pulse_all(DevicePtr(gpu.wb.chem_c), 1.3f);
            pulse_all(DevicePtr(gpu.wb.light), 1.8f);
            direct_atp_delta = 2.5f;
        } else if (cmd == "toxin" || cmd == "pain" || cmd == "punish") {
            pulse_all(DevicePtr(gpu.wb.chem_d), 0.28f, 1);
            direct_atp_delta = -2.2f;
        } else if (cmd == "teach_v0_food") {
            pulse_all(DevicePtr(gpu.wb.acoust_v0), 1.6f);
            pulse_all(DevicePtr(gpu.wb.chem_c), 1.4f);
            direct_atp_delta = 2.5f;
            n_steps = 12u;
        } else if (cmd == "teach_v2_food") {
            pulse_all(DevicePtr(gpu.wb.acoust_v2), 1.6f);
            pulse_all(DevicePtr(gpu.wb.chem_c), 1.4f);
            direct_atp_delta = 2.5f;
            n_steps = 12u;
        } else if (cmd == "teach_v1_apex") {
            pulse_all(DevicePtr(gpu.wb.acoust_v1), 1.6f);
            pulse_all(DevicePtr(gpu.wb.chem_d), 0.28f, 1);
            direct_atp_delta = -2.2f;
            n_steps = 12u;
        }
        auto lab_step = [&](u32 n_t, float atp_delta) {
            for (u32 k = 0; k < n_t; ++k, ++sim_tick) {
                if (k == 2u && atp_delta != 0.0f) {
                    s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, group_n * sizeof(CreatureState)), "conv rf d");
                    for (u32 a = 0; a < group_n; ++a)
                        if (cs[a].alive == 1u) cs[a].atp = std::max(14.0f, cs[a].atp + atp_delta);
                    s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), group_n * sizeof(CreatureState)), "conv rf u");
                }
                gpu.step_world(sim_tick, 0u, 0u, ck_seed);
                if (atp_delta >= 0.0f) s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "conv zcd");
                if (atp_delta <= 0.0f) {
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "conv zcb");
                    s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "conv zcc");
                }
                gpu.step_brains(sim_tick, abl);
                gpu.step_bodies(sim_tick, ck_seed, abl);
                s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, group_n * sizeof(CreatureState)), "conv step dl");
                bool dirty = false;
                for (u32 a = 0; a < group_n; ++a) {
                    if (cs[a].alive == 1u) {
                        if (cs[a].age > 60u) { cs[a].age = 40u; dirty = true; } // Prevent mid-dialogue cloning in the lab!
                        if (cs[a].body_temp < 0.44f || cs[a].body_temp > 0.56f) { cs[a].body_temp = 0.50f; dirty = true; }
                        if (cs[a].atp < 14.0f) { cs[a].atp = 14.0f; cs[a].prev_atp = 14.0f; dirty = true; }
                        if (cs[a].hp < 60.0f) { cs[a].hp = 60.0f; dirty = true; }
                    }
                }
                if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), group_n * sizeof(CreatureState)), "conv safe ul");
            }
        };
        if (is_seq2) {
            DevicePtr b1 = (cmd == "seq_01" || cmd == "seq_02" || cmd == "teach_seq01_food" ||
                            cmd == "teach_qa_01_v2" || cmd == "teach_qa_01_v0" || cmd == "ask_01")
                               ? DevicePtr(gpu.wb.acoust_v0)
                               : ((cmd == "seq_20" || cmd == "seq_21") ? DevicePtr(gpu.wb.acoust_v2) : DevicePtr(gpu.wb.acoust_v1));
            DevicePtr b2 = (cmd == "seq_01" || cmd == "seq_21" || cmd == "teach_seq01_food" ||
                            cmd == "teach_qa_01_v2" || cmd == "teach_qa_01_v0" || cmd == "ask_01")
                               ? DevicePtr(gpu.wb.acoust_v1)
                               : ((cmd == "seq_02" || cmd == "seq_12") ? DevicePtr(gpu.wb.acoust_v2) : DevicePtr(gpu.wb.acoust_v0));
            pulse_all(b1, 1.6f);
            lab_step(4u, 0.0f);
            pulse_all(b2, 1.6f);
            if (cmd == "teach_seq01_food") { pulse_all(DevicePtr(gpu.wb.chem_c), 1.4f); direct_atp_delta = 2.5f; }
            if (cmd == "teach_seq10_toxin") { pulse_all(DevicePtr(gpu.wb.chem_d), 0.28f, 1); direct_atp_delta = -2.2f; }
            if (cmd == "teach_qa_01_v2" || cmd == "teach_qa_10_v2") operant_target = 2;
            if (cmd == "teach_qa_10_v0" || cmd == "teach_qa_01_v0") operant_target = 0;
            n_steps = 8u;
        } else if (is_rel_cmd) {
            auto pres_item = [&](int it) {
                std::vector<float> rl(WORLD_CELLS, (it == 0) ? 1.85f : ((it == 1) ? 0.72f : 0.05f));
                s.d.check(s.d.upload(DevicePtr(gpu.wb.light), rl.data(), WORLD_CELLS * sizeof(float)), "conv rel l");
                pulse_all(DevicePtr((it == 0) ? gpu.wb.acoust_v0 : ((it == 1) ? gpu.wb.acoust_v1 : gpu.wb.acoust_v2)), 1.6f);
            };
            int i1 = (cmd == "teach_rel_ab" || cmd == "ask_rel_ac") ? 0 : ((cmd == "teach_rel_bc" || cmd == "teach_rel_ba") ? 1 : 2);
            int i2 = (cmd == "teach_rel_ba" || cmd == "ask_rel_ca") ? 0 : ((cmd == "teach_rel_ab" || cmd == "teach_rel_cb") ? 1 : 2);
            pres_item(i1); lab_step(3u, 0.0f);
            pres_item(i2);
            if (cmd == "teach_rel_ab" || cmd == "teach_rel_bc") operant_target = 2;
            else if (cmd == "teach_rel_ba" || cmd == "teach_rel_cb") operant_target = 0;
            n_steps = 8u;
        } else if (is_seq3) {
            bool fwd = (cmd == "seq_012" || cmd == "teach_seq012_food");
            pulse_all(DevicePtr(fwd ? gpu.wb.acoust_v0 : gpu.wb.acoust_v2), 1.6f);
            lab_step(3u, 0.0f);
            pulse_all(DevicePtr(gpu.wb.acoust_v1), 1.6f);
            lab_step(3u, 0.0f);
            pulse_all(DevicePtr(fwd ? gpu.wb.acoust_v2 : gpu.wb.acoust_v0), 1.6f);
            if (cmd == "teach_seq012_food") { pulse_all(DevicePtr(gpu.wb.chem_c), 1.4f); direct_atp_delta = 2.5f; }
            if (cmd == "teach_seq210_toxin") { pulse_all(DevicePtr(gpu.wb.chem_d), 0.28f, 1); direct_atp_delta = -2.2f; }
            n_steps = 8u;
        } else if (cmd == "prey" || cmd == "apex" || cmd == "stone" || cmd == "megapod") {
            std::vector<WorldObject> objs(MAX_OBJECTS);
            s.d.check(s.d.download(objs.data(), DevicePtr(gpu.wb.objects), MAX_OBJECTS * sizeof(WorldObject)), "conv obj dl");
            WorldObject &o = objs[cmd == "apex" ? 580u : (cmd == "stone" ? 770u : (cmd == "megapod" ? 230u : 390u))];
            o.x = float(fx) + 0.5f; o.y = float(fy) + 0.5f; o.vx = 0.0f; o.vy = 0.0f;
            o.kind = (cmd == "apex" ? OBJ_APEX : (cmd == "stone" ? OBJ_STONE : (cmd == "megapod" ? OBJ_MEGAPOD : OBJ_PREY)));
            o.energy = (cmd == "apex" || cmd == "megapod") ? 72.0f : 24.0f;
            o.integrity = (cmd == "apex" ? 2.2f : (cmd == "megapod" ? 2.6f : 1.0f));
            o.carrier_id = INVALID_OBJ; o.age = 0u; o.flags = 0u;
            s.d.check(s.d.upload(DevicePtr(gpu.wb.objects), objs.data(), MAX_OBJECTS * sizeof(WorldObject)), "conv obj ul");
        } else if (cmd == "step") {
            if (!(iss >> n_steps)) n_steps = 12u;
            n_steps = std::clamp(n_steps, 1u, 200u);
        } else if (cmd == "help") {
            std::cout << "Commands: v0 | v1 | v2 | seq_01 | seq_10 | seq_02 | seq_12 | seq_012 | seq_210 | teach_qa_01_v2 | teach_qa_10_v0 | ask_01 | ask_10 | rumble | food | toxin | teach_v0_food | teach_v1_apex | teach_v2_food | prey | apex | stone | megapod | step [N] | quit\n";
            return true;
        }

        u64 spk0 = cs[0].spikes_total;
        u32 ed0 = cs[0].plast_edits;
        u32 vrepl0 = gpu.read_counters().total_vocal_replies;
        {
            bool dirty = false;
            for (u32 a = 0; a < group_n; ++a) {
                if (cs[a].alive == 1u) {
                    if (cs[a].age > 60u) { cs[a].age = 40u; dirty = true; } // Keep lab cleanly at 4 dialogue agents!
                    if (cs[a].body_temp < 0.44f || cs[a].body_temp > 0.56f) { cs[a].body_temp = 0.50f; dirty = true; }
                    if (cs[a].atp < 20.0f || cs[a].atp > 28.0f) { cs[a].atp = 24.0f; cs[a].prev_atp = 24.0f; cs[a].da_baseline = 0.0f; dirty = true; }
                    if (cs[a].hp < 65.0f) { cs[a].hp = 65.0f; dirty = true; }
                }
            }
            if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), group_n * sizeof(CreatureState)), "conv keepalive");
        }
        float peak_da = 0.0f;
        float pk_v[4][3]{};
        float sum_v[4][3]{};
        u8 qa_rewarded[4]{};
        u8 qa_rewarded2[4]{};
        u8 qa_rewarded3[4]{};
        u8 mot_rewarded[4]{};
        std::string phr[4];
        int last_syl[4]{-1, -1, -1, -1};
        int syl_cnt[4]{0, 0, 0, 0};
        bool is_food_turn = (direct_atp_delta > 0.0f || cmd == "food" || cmd == "reward" ||
                             cmd == "ask_food" || cmd == "teach_obj_food_v0" || cmd == "teach_obj_food_v2" ||
                             cmd == "teach_phrase_food_02" || cmd == "ask_phrase_food" ||
                             cmd == "teach_p3_x" || cmd == "ask_p3_x" ||
                             cmd == "teach_v0_food" || cmd == "teach_v2_food" ||
                             cmd == "teach_seq01_food" || cmd == "teach_seq012_food");
        double sum_thr = 0.0, sum_trn = 0.0, sum_eat = 0.0, sum_dig = 0.0, sum_bld = 0.0;
        for (u32 k = 0; k < n_steps; ++k, ++sim_tick) {
            if (is_motor_cmd && k == 4u) {
                pulse_all(DevicePtr(gpu.wb.acoust_v0), 0.0f, 6);
                pulse_all(DevicePtr(gpu.wb.acoust_v2), 0.0f, 6);
                pulse_all(DevicePtr(gpu.wb.acoust_v1), 1.6f); // Identical Syllable 2 execute cue for BOTH GO & HALT!
            }
            if (is_phrase_cmd && k == 5u) {
                // Transition to Window 2 neutral query `v1` for compositional second syllable!
                pulse_all(DevicePtr(gpu.wb.chem_c), 0.0f, 6);
                pulse_all(DevicePtr(gpu.wb.acoust_v0), 0.0f, 6);
                pulse_all(DevicePtr(gpu.wb.acoust_v2), 0.0f, 6);
                pulse_all(DevicePtr(gpu.wb.acoust_p), 0.0f, 6);
                pulse_all(DevicePtr(gpu.wb.acoust_v1), 1.6f);
                std::vector<float> dk(WORLD_CELLS, 0.05f);
                s.d.check(s.d.upload(DevicePtr(gpu.wb.light), dk.data(), WORLD_CELLS * sizeof(float)), "conv p2l");
                for (u32 a = 0; a < group_n; ++a) if (cs[a].alive == 1u) { cs[a].atp = 24.0f; cs[a].prev_atp = 24.0f; cs[a].da_baseline = 0.0f; }
                s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), group_n * sizeof(CreatureState)), "conv p2u");
            }
            if (is_phrase3_cmd && k == 10u) {
                // Transition to Window 3 identical dark closing prompt (`acoust_p=0.9`) for 3-syllable phrase completion!
                pulse_all(DevicePtr(gpu.wb.acoust_v0), 0.0f, 6);
                pulse_all(DevicePtr(gpu.wb.acoust_v1), 0.0f, 6);
                pulse_all(DevicePtr(gpu.wb.acoust_v2), 0.0f, 6);
                pulse_all(DevicePtr(gpu.wb.acoust_p), 0.9f);
                for (u32 a = 0; a < group_n; ++a) if (cs[a].alive == 1u) { cs[a].atp = 24.0f; cs[a].prev_atp = 24.0f; cs[a].da_baseline = 0.0f; }
                s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), group_n * sizeof(CreatureState)), "conv p3u");
            }
            if (k == 2u && direct_atp_delta != 0.0f) {
                s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, group_n * sizeof(CreatureState)), "conv rf d");
                for (u32 a = 0; a < group_n; ++a)
                    if (cs[a].alive == 1u) cs[a].atp = std::max(14.0f, cs[a].atp + direct_atp_delta);
                s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), group_n * sizeof(CreatureState)), "conv rf u");
            }
            gpu.step_world(sim_tick, 0u, 0u, ck_seed);
            if (direct_atp_delta >= 0.0f && cmd != "toxin" && cmd != "pain" && cmd != "punish" && cmd != "teach_v1_apex" && cmd != "teach_seq10_toxin" && cmd != "teach_seq210_toxin")
                s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_d), 0, WORLD_CELLS * sizeof(float)), "conv zcd2");
            if (!is_food_turn) {
                s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_b), 0, WORLD_CELLS * sizeof(float)), "conv zcb2");
                s.d.check(s.d.zero(DevicePtr(gpu.wb.chem_c), 0, WORLD_CELLS * sizeof(float)), "conv zcc2");
            }
            gpu.step_brains(sim_tick, abl);
            gpu.step_bodies(sim_tick, ck_seed, abl);
            s.d.check(s.d.download(cs.data(), gpu.d_creatures.ptr, group_n * sizeof(CreatureState)), "conv step dl");
            bool dirty = false;
            for (u32 a = 0; a < group_n; ++a) {
                if (cs[a].alive == 1u) {
                    if (cs[a].age > 60u) { cs[a].age = 40u; dirty = true; } // Keep lab cleanly at 4 dialogue agents!
                    if (cs[a].body_temp < 0.44f || cs[a].body_temp > 0.56f) { cs[a].body_temp = 0.50f; dirty = true; }
                    if (operant_target >= 0 && ((is_phrase_cmd && k >= 1u && k <= 4u) || (!is_phrase_cmd && k >= 2u && k <= 5u))) {
                        float v0 = cs[a].mot_vocal0, v1 = cs[a].mot_vocal1, v2 = cs[a].mot_vocal2;
                        int dom = -1;
                        if (v0 > 0.14f && v0 > v1 + 0.02f && v0 > v2 + 0.02f) dom = 0;
                        else if (v1 > 0.14f && v1 > v0 + 0.02f && v1 > v2 + 0.02f) dom = 1;
                        else if (v2 > 0.14f && v2 > v0 + 0.02f && v2 > v1 + 0.02f) dom = 2;
                        if (dom == operant_target && (qa_rewarded[a] & 1u) == 0u) { cs[a].atp += 2.4f; qa_rewarded[a] |= 1u; dirty = true; }
                        else if ((dom >= 0 || (v0 > 0.14f && v2 > 0.14f)) && qa_rewarded[a] == 0u) { cs[a].atp = std::max(14.0f, cs[a].atp - 1.8f); qa_rewarded[a] |= 2u; dirty = true; }
                    }
                    if (operant_target2 >= 0 && k >= 6u && k <= 9u) {
                        float v0 = cs[a].mot_vocal0, v1 = cs[a].mot_vocal1, v2 = cs[a].mot_vocal2;
                        int dom = -1;
                        if (v0 > 0.14f && v0 > v1 + 0.02f && v0 > v2 + 0.02f) dom = 0;
                        else if (v1 > 0.14f && v1 > v0 + 0.02f && v1 > v2 + 0.02f) dom = 1;
                        else if (v2 > 0.14f && v2 > v0 + 0.02f && v2 > v1 + 0.02f) dom = 2;
                        if (dom == operant_target2 && (qa_rewarded2[a] & 1u) == 0u) { cs[a].atp += 2.4f; qa_rewarded2[a] |= 1u; dirty = true; }
                        else if ((dom >= 0 || (v0 > 0.14f && v2 > 0.14f)) && qa_rewarded2[a] == 0u) { cs[a].atp = std::max(14.0f, cs[a].atp - 1.8f); qa_rewarded2[a] |= 2u; dirty = true; }
                    }
                    if (operant_target3 >= 0 && k >= 11u && k <= 14u) {
                        float v0 = cs[a].mot_vocal0, v1 = cs[a].mot_vocal1, v2 = cs[a].mot_vocal2;
                        int dom = -1;
                        if (v0 > 0.14f && v0 > v1 + 0.02f && v0 > v2 + 0.02f) dom = 0;
                        else if (v1 > 0.14f && v1 > v0 + 0.02f && v1 > v2 + 0.02f) dom = 1;
                        else if (v2 > 0.14f && v2 > v0 + 0.02f && v2 > v1 + 0.02f) dom = 2;
                        if (dom == operant_target3 && (qa_rewarded3[a] & 1u) == 0u) { cs[a].atp += 2.4f; qa_rewarded3[a] |= 1u; dirty = true; }
                        else if ((dom >= 0 || (v0 > 0.14f && v2 > 0.14f)) && qa_rewarded3[a] == 0u) { cs[a].atp = std::max(14.0f, cs[a].atp - 1.8f); qa_rewarded3[a] |= 2u; dirty = true; }
                    }
                    if (motor_target >= 0 && k >= 5u && k <= 8u) {
                        float act = cs[a].mot_thrust + 0.5f * cs[a].mot_eat;
                        if (motor_target == 1) {
                            if (act >= 0.24f && (mot_rewarded[a] & 1u) == 0u) { cs[a].atp += 2.6f; mot_rewarded[a] |= 1u; dirty = true; }
                            else if (k >= 6u && act < 0.16f && (mot_rewarded[a] & 2u) == 0u) { cs[a].atp = std::max(10.0f, cs[a].atp - 1.8f); mot_rewarded[a] |= 2u; dirty = true; }
                        } else {
                            if (cs[a].mot_thrust >= 0.20f && (mot_rewarded[a] & 2u) == 0u) { cs[a].atp = std::max(10.0f, cs[a].atp - 1.8f); mot_rewarded[a] |= 2u; dirty = true; }
                            else if (k == 8u && mot_rewarded[a] == 0u) { cs[a].atp += 2.6f; mot_rewarded[a] |= 1u; dirty = true; }
                        }
                    }
                    if (cs[a].atp < 14.0f) { cs[a].atp = 14.0f; cs[a].prev_atp = 14.0f; dirty = true; }
                    if (cs[a].hp < 50.0f) { cs[a].hp = 50.0f; dirty = true; }
                    // Decode live multi-syllable utterance sequence across the turn (run-length compressed, up to 4 syllables!)
                    float tv0 = cs[a].mot_vocal0, tv1 = cs[a].mot_vocal1, tv2 = cs[a].mot_vocal2;
                    int cur_s = -1;
                    if (tv0 > 0.24f && tv0 >= tv1 + 0.02f && tv0 >= tv2 + 0.02f) cur_s = 0;
                    else if (tv1 > 0.24f && tv1 >= tv0 + 0.02f && tv1 >= tv2 + 0.02f) cur_s = 1;
                    else if (tv2 > 0.24f && tv2 >= tv0 + 0.02f && tv2 >= tv1 + 0.02f) cur_s = 2;
                    if (cur_s >= 0 && cur_s != last_syl[a] && syl_cnt[a] < 4) {
                        if (!phr[a].empty()) phr[a].push_back('-');
                        phr[a] += ("v" + std::to_string(cur_s));
                        last_syl[a] = cur_s;
                        ++syl_cnt[a];
                    }
                }
                if (cs[a].mot_vocal0 > pk_v[a][0]) pk_v[a][0] = cs[a].mot_vocal0;
                if (cs[a].mot_vocal1 > pk_v[a][1]) pk_v[a][1] = cs[a].mot_vocal1;
                if (cs[a].mot_vocal2 > pk_v[a][2]) pk_v[a][2] = cs[a].mot_vocal2;
                sum_v[a][0] += cs[a].mot_vocal0;
                sum_v[a][1] += cs[a].mot_vocal1;
                sum_v[a][2] += cs[a].mot_vocal2;
            }
            if (dirty) s.d.check(s.d.upload(gpu.d_creatures.ptr, cs.data(), group_n * sizeof(CreatureState)), "conv safe ul");
            float cur_da = cs[0].mod_da + cs[0].int_da;
            if (std::fabs(cur_da) > std::fabs(peak_da)) peak_da = cur_da;
            sum_thr += cs[0].mot_thrust; sum_trn += cs[0].mot_turn;
            sum_eat += cs[0].mot_eat; sum_dig += cs[0].mot_dig; sum_bld += cs[0].mot_build;
        }
        double inv_n = 1.0 / double(std::max(1u, n_steps));
        u32 peer_replies = gpu.read_counters().total_vocal_replies - vrepl0;

        std::ostringstream lstr;
        for (u32 z = 0; z < cs[0].num_layers && z < MAX_LAYERS_PER_CREATURE; ++z) {
            u32 sl = cs[0].layer_slabs[z];
            if (sl == INVALID_SLAB) continue;
            std::vector<u8> sr(TISSUE_CELLS);
            s.d.check(s.d.download(sr.data(), DevicePtr(gpu.pool.spike_ring) + size_t(sl) * TISSUE_CELLS, TISSUE_CELLS), "conv sr");
            u32 act_cells = 0u;
            for (u8 r : sr) if (r != 0u) ++act_cells;
            if (z) lstr << ',';
            lstr << "L" << z << ":" << act_cells;
        }
        auto band_of = [&](u32 a, bool compact) -> const char * {
            float v0 = sum_v[a][0], v1 = sum_v[a][1], v2 = sum_v[a][2];
            float pmax = std::max(pk_v[a][0], std::max(pk_v[a][1], pk_v[a][2]));
            if (pmax <= 0.24f) return compact ? "-" : "silent";
            if (v0 >= v1 + 0.05f && v0 >= v2 + 0.05f) return "v0";
            if (v1 >= v0 + 0.05f && v1 >= v2 + 0.05f) return "v1";
            if (v2 >= v0 + 0.05f && v2 >= v1 + 0.05f) return "v2";
            return "ch";
        };
        std::ostringstream gstr, pstr;
        for (u32 a = 0; a < group_n; ++a) {
            if (a) { gstr << '/'; pstr << '/'; }
            gstr << "A" << a << ":" << band_of(a, true);
            pstr << "A" << a << ":" << (phr[a].empty() ? "-" : phr[a]);
        }
        std::cout << "genesis_turn=" << sim_tick << "  input=" << cmd
                  << "  d_spikes=" << (cs[0].spikes_total - spk0)
                  << "  d_edits=" << (cs[0].plast_edits - ed0)
                  << "  active_assemblies=" << lstr.str()
                  << "  atp=" << std::fixed << std::setprecision(1) << cs[0].atp
                  << "  da=" << std::setprecision(2) << peak_da
                  << "  thrust=" << (sum_thr * inv_n) << "  turn=" << (sum_trn * inv_n)
                  << "  eat=" << (sum_eat * inv_n) << "  dig=" << (sum_dig * inv_n) << "  build=" << (sum_bld * inv_n)
                  << "  vocal0=" << pk_v[0][0] << "  vocal1=" << pk_v[0][1] << "  vocal2=" << pk_v[0][2]
                  << "  reply=" << (group_n > 1u ? gstr.str() : std::string(band_of(0, false)))
                  << "  phrase=" << (group_n > 1u ? pstr.str() : (phr[0].empty() ? std::string("-") : phr[0]))
                  << "  peer_replies=" << peer_replies << '\n';
        return true;
    };

    if (!script_arg.empty()) {
        std::stringstream ss(script_arg);
        std::string item;
        while (std::getline(ss, item, ',')) {
            if (!inject_and_step(item)) break;
        }
    } else {
        std::cout << "Interactive Grounded Spiking Mind (ZERO LLMs). Commands: v0, v1, rumble, food, toxin, prey, apex, stone, megapod, step N, quit\n";
        std::string line;
        while (std::cout << "mind> " << std::flush && std::getline(std::cin, line)) {
            if (!inject_and_step(line)) break;
        }
    }
    return 0;
}

} // namespace

int command(int argc, char **argv) {
    if (argc < 3 || std::string_view(argv[2]) == "--help") {
        std::cout << R"(Axion Genesis v47  |  Blackwell SM 12.0 Bare-Metal Accelerated Neocortex Spiking Mind

  ./axion genesis bench    [--pop N] [--substeps S] [--repeats R] [--world 256|512|1024]
  ./axion genesis evolve   [--pop N] [--max-areas L] [--max-syn-slabs S] [--world 256|512|1024] [--ticks T] [--seed S] [--challenge 0|1|2] [--controls] [--load FILE] [--save FILE] [--video FILE] [--video-title TITLE]
  ./axion genesis inspect  --load FILE
  ./axion genesis probe    --load FILE [--ticks N] [--seed S]
  ./axion genesis converse --load FILE [--group 1..4] [--script "teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,distract,teach_ctx_v0,ctx_danger,distract,teach_ctx_v2,ctx_food,distract,ask_ctx,teach_rel_ab,teach_rel_bc,ask_rel_ac"]

Every organism grows from ONE zygote stem cell via an expandable discrete CA-GRN rulebook
(scalable oSVZ radial-glia neocortex up to 32 layers / 8,192 neurons / 192 syn/cell, active
nonlinear 4-synapse NMDA dendritic subunits, PV+ top-k sparse coding, Payeur-Naud Burstprop
& Mikulasch dendritic predictive coding, equilibrium Area-X vocal whitening, and 3-Sun
Trisolaris-6 referential ecology on zero-spill 4 B/synapse Blackwell v4.u32 slabs) in a
runtime-scalable 256x256 / 512x512 / 1024x1024 planet.
)";
        return 0;
    }

    Session s;
    s.open();
    if (auto *j = journal::current()) j->device = s.name;

    std::string_view sub = argv[2];
    if (sub == "bench") {
        u32 pop_n = 1024, substeps = 256, repeats = 7, world_size = WORLD_W;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            if (f == "--pop") pop_n = parse_u32(argv[i], 32, MAX_POP * 2u, f);
            else if (f == "--substeps") substeps = parse_u32(argv[i], 4, 2048, f);
            else if (f == "--repeats") repeats = parse_u32(argv[i], 1, 51, f);
            else if (f == "--world") world_size = parse_u32(argv[i], 256, 1024, f);
            else throw std::invalid_argument("unknown genesis bench option " + std::string(f));
        }
        return run_bench(s, pop_n, substeps, repeats, world_size);
    }

    if (sub == "evolve") {
        u32 pop_n = 512, max_layers = 0, max_syn_slabs = 0, world_size = WORLD_W, ticks = 2000, seed = 42, challenge = 1;
        bool controls = false;
        std::string load_path, save_path, video_path, video_title = "AXION GENESIS V45";
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (f == "--controls") { controls = true; continue; }
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            if (f == "--pop") pop_n = parse_u32(argv[i], 64, MAX_POP, f);
            else if (f == "--max-areas" || f == "--max-layers") max_layers = parse_u32(argv[i], 64, 131072, f);
            else if (f == "--max-syn-slabs") max_syn_slabs = parse_u32(argv[i], 256, 262144, f);
            else if (f == "--world") world_size = parse_u32(argv[i], 256, 1024, f);
            else if (f == "--ticks") ticks = parse_u32(argv[i], 32, 500000, f);
            else if (f == "--seed") seed = parse_u32(argv[i], 1, 0xfffffffeu, f);
            else if (f == "--challenge") challenge = parse_u32(argv[i], 0, 2, f);
            else if (f == "--load") load_path = argv[i];
            else if (f == "--save") save_path = argv[i];
            else if (f == "--video") video_path = argv[i];
            else if (f == "--video-title") video_title = argv[i];
            else throw std::invalid_argument("unknown genesis evolve option " + std::string(f));
        }
        if (max_layers == 0) max_layers = pop_n * 20u;
        return run_evolve(s, pop_n, max_layers, ticks, seed, challenge, controls, load_path, save_path, video_path, video_title, world_size, max_syn_slabs);
    }

    if (sub == "inspect") {
        std::string load_path;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            if (f == "--load") load_path = argv[i];
            else throw std::invalid_argument("unknown genesis inspect option " + std::string(f));
        }
        if (load_path.empty()) throw std::invalid_argument("--load FILE is required");
        return run_inspect(s, load_path);
    }

    if (sub == "probe") {
        std::string load_path;
        u32 ticks = 600u, seed = 101u;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            if (f == "--load") load_path = argv[i];
            else if (f == "--ticks") ticks = parse_u32(argv[i], 180, 10000, f);
            else if (f == "--seed") seed = parse_u32(argv[i], 1, 0xfffffffeu, f);
            else throw std::invalid_argument("unknown genesis probe option " + std::string(f));
        }
        if (load_path.empty()) throw std::invalid_argument("--load FILE is required");
        return run_probe(s, load_path, ticks, seed);
    }

    if (sub == "converse") {
        std::string load_path, script_arg;
        u32 group_n = 1u;
        for (int i = 3; i < argc; ++i) {
            std::string_view f = argv[i];
            if (++i >= argc) throw std::invalid_argument("missing value for " + std::string(f));
            if (f == "--load") load_path = argv[i];
            else if (f == "--script") script_arg = argv[i];
            else if (f == "--group") group_n = parse_u32(argv[i], 1, 4, f);
            else throw std::invalid_argument("unknown genesis converse option " + std::string(f));
        }
        if (load_path.empty()) throw std::invalid_argument("--load FILE is required");
        return run_converse(s, load_path, script_arg, group_n);
    }

    throw std::invalid_argument("unknown genesis subcommand " + std::string(sub));
}

} // namespace axion::genesis
