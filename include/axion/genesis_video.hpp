#pragma once
#include "axion/genesis.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace axion::genesis {

// ---------------------------------------------------------------------------
// Zero-Dependency Dark-Observatory 1280x768 C++ MP4 Evolution Animator
// Pipes native 256x256 GPU world + acoustic ripples + objects + creatures +
// live champion body/cortex + animated telemetry curves directly into
// Blackwell hardware NVENC (h264_nvenc, fallback libopenh264/mpeg4), <= 60s!
// ---------------------------------------------------------------------------
class GenesisVideoRecorder {
  public:
    static constexpr u32 VID_W = 1280u;
    static constexpr u32 VID_H = 768u;
    static constexpr u32 MAP_PX = 768u; // 256x256 world @ 3x3 px/tile!
    static constexpr u32 HUD_X = 768u;
    static constexpr u32 HUD_W = VID_W - HUD_X; // 512px Observatory HUD
    static constexpr u32 FPS = 16u;
    static constexpr u32 MAX_FRAMES = FPS * 58u; // Strictly <= 60s (max 58s)!

    struct TelemetryPoint {
        u32 tick;
        u32 alive;
        u32 species;
        u32 max_gen;
        float mean_layers;
        float mean_neurons;
        u32 births;
        u32 prey;
        u32 pods;
        u32 vocal;
        u32 replies;
        u32 mimics;
        u32 shelters;
        u32 tempered;
        u32 rewires;
        u64 edits;
    };

    GenesisVideoRecorder() = default;
    ~GenesisVideoRecorder() { close(); }

    bool open(const std::string &path, const std::string &title, u32 total_ticks, u32 pop_n) {
        close();
        path_ = path;
        title_ = title;
        total_ticks_ = std::max(1u, total_ticks);
        pop_n_ = std::max(1u, pop_n);
        frame_.assign(size_t(VID_W) * VID_H * 4u, 0u);
        hist_.clear();
        frames_written_ = 0u;

        // Try Blackwell hardware h264_nvenc first, fallback cleanly to libopenh264 or mpeg4.
        std::string safe_path;
        for (char c : path) {
            if (c == '\'' || c == '"' || c == '\\' || c == '$' || c == '`' || c == ';') continue;
            safe_path.push_back(c);
        }
        if (safe_path.empty()) return false;

        auto try_cmd = [&](const std::string &vcodec_args) -> bool {
            std::ostringstream cmd;
            cmd << "ffmpeg -y -hide_banner -loglevel error "
                << "-f rawvideo -pix_fmt rgba -s " << VID_W << "x" << VID_H << " -r " << FPS << " -i - "
                << vcodec_args << " -pix_fmt yuv420p -movflags +faststart '" << safe_path << "' 2>/dev/null";
            pipe_ = popen(cmd.str().c_str(), "w");
            return pipe_ != nullptr;
        };

        // Probe royalty-free Blackwell hardware AV1 (av1_nvenc) first, fallback to VP9 (libvpx-vp9).
        // Both decode natively in browsers, Linux/macOS/Windows players & stripped-H.264 distros!
        int av1_ok = system("ffmpeg -hide_banner -loglevel error -f lavfi -i color=c=black:s=1280x768:d=0.05 -pix_fmt yuv420p -c:v av1_nvenc -f null - 2>/dev/null");
        if (av1_ok == 0 && try_cmd("-c:v av1_nvenc -preset p4 -cq 30")) return true;
        int vp9_ok = system("ffmpeg -hide_banner -loglevel error -f lavfi -i color=c=black:s=1280x768:d=0.05 -pix_fmt yuv420p -c:v libvpx-vp9 -crf 32 -b:v 0 -f null - 2>/dev/null");
        if (vp9_ok == 0 && try_cmd("-c:v libvpx-vp9 -deadline realtime -cpu-used 5 -crf 32 -b:v 0")) return true;
        return try_cmd("-c:v mpeg4 -q:v 4");
    }

    bool is_open() const { return pipe_ != nullptr; }
    u32 frames_written() const { return frames_written_; }

    void close() {
        if (pipe_) {
            pclose(pipe_);
            pipe_ = nullptr;
        }
    }

    void push_frame(u32 tick, u32 world_w, u32 world_h,
                    const float *terrain, const u8 *material, const float *water,
                    const float *chem_b, const float *chem_c, const float *chem_d,
                    const float *light, const float *ac_p, const float *ac_v0, const float *ac_v1, const float *ac_v2,
                    const WorldObject *objects, u32 max_objects,
                    const CreatureState *crs, const BodyPhenotype *bps, u32 pop_n,
                    u32 best_idx, const TelemetryPoint &tp) {
        if (!pipe_ || frames_written_ >= MAX_FRAMES) return;
        hist_.push_back(tp);

        // 1. Fill Observatory Dark Background (#081013)
        fill_rect(0, 0, VID_W, VID_H, 8, 16, 19);

        // 2. Render Runtime-Scalable (256x256 / 512x512 / 1024x1024) GPU World into 768x768 Viewport!
        u32 wm = world_w - 1u, hm = world_h - 1u;
        float map_scale = float(MAP_PX) / float(world_w);
        for (u32 py = 0; py < MAP_PX; ++py) {
            u32 y = (py * world_h) / MAP_PX;
            u32 ym = (y + world_h - 1u) & hm;
            u32 yp = (y + 1u) & hm;
            u8 *row = frame_.data() + size_t(py) * VID_W * 4u;
            for (u32 px = 0; px < MAP_PX; ++px) {
                u32 x = (px * world_w) / MAP_PX;
                u32 idx = y * world_w + x;
                u32 xm = (x + world_w - 1u) & wm;
                u32 xp = (x + 1u) & wm;
                float h = terrain[idx];
                float dx = terrain[y * world_w + xp] - terrain[y * world_w + xm];
                float dy = terrain[yp * world_w + x] - terrain[ym * world_w + x];
                float shade = std::clamp(0.86f - 0.30f * dx - 0.30f * dy, 0.45f, 1.32f);

                float w = water[idx];
                u8 mat = material[idx];
                float r = 0.0f, g = 0.0f, b = 0.0f;
                if (w > 0.25f) {
                    float d = std::clamp(w * 0.65f, 0.0f, 1.0f);
                    r = 12.0f + (1.0f - d) * 14.0f;
                    g = 34.0f + (1.0f - d) * 26.0f;
                    b = 48.0f + (1.0f - d) * 30.0f;
                } else {
                    float elev = std::clamp(h * 0.28f, 0.0f, 1.0f);
                    r = 26.0f + elev * 52.0f;
                    g = 34.0f + elev * 48.0f;
                    b = 34.0f + elev * 42.0f;
                }
                if (mat == MAT_ORE) { r = 68.0f; g = 74.0f; b = 88.0f; }
                else if (mat == MAT_VENT) { r = 235.0f; g = 102.0f; b = 38.0f; }
                else if (mat == MAT_POLYMER) { r = 162.0f; g = 96.0f; b = 228.0f; }
                else if (mat == MAT_CATALYST) { r = 46.0f; g = 148.0f; b = 146.0f; }

                // Circadian & polar night lighting modulation (keeps dark ocean legible while clearly showing night!)
                float l = std::clamp(light[idx], 0.05f, 2.0f);
                float lum = 0.42f + 0.58f * std::clamp(l, 0.0f, 1.35f);
                r *= shade * lum;
                g *= shade * lum;
                b *= shade * lum;

                // Reef bloom (chem_b + chem_c) & toxic storm clouds (chem_d)
                float cb = std::clamp(chem_b[idx], 0.0f, 1.5f);
                float cc = std::clamp(chem_c[idx], 0.0f, 1.5f);
                float cd = std::clamp(chem_d[idx], 0.0f, 1.5f);
                if (cb > 0.08f || cc > 0.05f) {
                    float nf = std::clamp(cb * 0.42f + cc * 0.95f, 0.0f, 1.0f);
                    r += nf * 42.0f;
                    g += nf * 145.0f;
                    b += nf * 92.0f;
                }
                if (cd > 0.06f) {
                    float tf = std::clamp(cd * 0.85f, 0.0f, 1.0f);
                    r += tf * 140.0f;
                    g -= tf * 18.0f;
                    b += tf * 48.0f;
                }
                if (l > 1.10f) {
                    float glow = std::clamp((l - 1.10f) * 0.8f, 0.0f, 0.7f);
                    r += glow * 95.0f; g += glow * 85.0f; b += glow * 35.0f;
                }

                // Live 4-Band Propagating Acoustic Wavefield Ripples!
                float ap = std::fabs(ac_p[idx]);
                float v0 = std::fabs(ac_v0[idx]);
                float v1 = std::fabs(ac_v1[idx]);
                float v2 = std::fabs(ac_v2[idx]);
                if (ap > 0.06f) { float k = std::min(1.0f, ap * 0.75f); r += k * 75.0f; g += k * 105.0f; b += k * 140.0f; }
                if (v0 > 0.05f) { float k = std::min(1.0f, v0 * 1.10f); g += k * 125.0f; b += k * 85.0f; }
                if (v1 > 0.05f) { float k = std::min(1.0f, v1 * 1.15f); r += k * 140.0f; g += k * 110.0f; b += k * 25.0f; }
                if (v2 > 0.05f) { float k = std::min(1.0f, v2 * 1.15f); r += k * 115.0f; g += k * 55.0f; b += k * 155.0f; }

                row[px * 4u + 0u] = u8(std::clamp(int(r), 0, 255));
                row[px * 4u + 1u] = u8(std::clamp(int(g), 0, 255));
                row[px * 4u + 2u] = u8(std::clamp(int(b), 0, 255));
                row[px * 4u + 3u] = 255;
            }
        }

        // 3. Draw Persistent Physical Objects on Planet (Prey, Pods, Mega-Pods, Stones, Apex)
        if (objects) {
            for (u32 i = 0; i < max_objects; ++i) {
                const WorldObject &ob = objects[i];
                if (ob.kind == OBJ_NONE || ob.carrier_id != INVALID_OBJ) continue;
                int cx = int(ob.x * map_scale);
                int cy = int(ob.y * map_scale);
                if (ob.kind == OBJ_PREY) {
                    draw_disc(cx, cy, 2, 70, 224, 248);
                } else if (ob.kind == OBJ_POD) {
                    draw_disc(cx, cy, 2, 246, 196, 83);
                } else if (ob.kind == OBJ_MEGAPOD) {
                    draw_disc(cx, cy, 3, 244, 152, 66);
                    draw_ring(cx, cy, 4, 255, 224, 130);
                } else if (ob.kind == OBJ_FUEL) {
                    draw_disc(cx, cy, 2, 134, 242, 156);
                } else if (ob.kind == OBJ_STONE) {
                    bool temp = (ob.flags & 2u) != 0u;
                    draw_disc(cx, cy, 2, temp ? 140 : 185, temp ? 235 : 196, temp ? 255 : 210);
                } else if (ob.kind == OBJ_APEX) {
                    draw_disc(cx, cy, 3, 246, 68, 72);
                    draw_ring(cx, cy, 5, 255, 118, 110);
                }
            }
        }

        // 4. Draw All Living Organisms Oriented by Heading & Colored by Species Lineage!
        static constexpr u8 PAL[8][3] = {
            {246, 196, 83},  {62, 230, 168}, {242, 107, 107}, {69, 199, 245},
            {199, 125, 255}, {240, 148, 77}, {155, 229, 100}, {255, 126, 182}
        };
        for (u32 i = 0; i < pop_n; ++i) {
            if (crs[i].alive != 1u) continue;
            int cx = int(crs[i].x * map_scale);
            int cy = int(crs[i].y * map_scale);
            u32 ci = bps[i].body_hash & 7u;
            u8 cr = PAL[ci][0], cg = PAL[ci][1], cb = PAL[ci][2];
            float ct = std::cos(crs[i].theta), st = std::sin(crs[i].theta);
            int hx = cx + int(ct * 4.5f), hy = cy + int(st * 4.5f);
            int lx = cx + int((-ct * 2.5f - st * 2.4f)), ly = cy + int((-st * 2.5f + ct * 2.4f));
            int rx = cx + int((-ct * 2.5f + st * 2.4f)), ry = cy + int((-st * 2.5f - ct * 2.4f));
            draw_line(lx, ly, hx, hy, cr, cg, cb);
            draw_line(rx, ry, hx, hy, cr, cg, cb);
            draw_line(lx, ly, rx, ry, u8(cr / 2), u8(cg / 2), u8(cb / 2));
            draw_disc(cx, cy, 1, cr, cg, cb);
            if (i == best_idx) {
                draw_ring(cx, cy, 8, 246, 196, 83);
                draw_ring(cx, cy, 9, 255, 238, 160);
            }
        }

        // Map right separator border
        fill_rect(MAP_PX, 0, 2, VID_H, 31, 57, 65);

        // 5. Right Observatory Dashboard HUD (x = 770..1280)
        draw_hud(tick, best_idx < pop_n ? &crs[best_idx] : nullptr,
                 best_idx < pop_n ? &bps[best_idx] : nullptr, tp);

        size_t wrote = fwrite(frame_.data(), 1, frame_.size(), pipe_);
        if (wrote == frame_.size()) ++frames_written_;
    }

  private:
    FILE *pipe_ = nullptr;
    std::string path_;
    std::string title_;
    u32 total_ticks_ = 1u;
    u32 pop_n_ = 1u;
    u32 frames_written_ = 0u;
    std::vector<u8> frame_;
    std::vector<TelemetryPoint> hist_;

    inline void set_px(int x, int y, u8 r, u8 g, u8 b) {
        if (x < 0 || y < 0 || x >= int(VID_W) || y >= int(VID_H)) return;
        u8 *p = frame_.data() + (size_t(y) * VID_W + size_t(x)) * 4u;
        p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
    }

    void fill_rect(int x, int y, int w, int h, u8 r, u8 g, u8 b) {
        int x0 = std::max(0, x), y0 = std::max(0, y);
        int x1 = std::min(int(VID_W), x + w), y1 = std::min(int(VID_H), y + h);
        for (int yy = y0; yy < y1; ++yy) {
            u8 *p = frame_.data() + (size_t(yy) * VID_W + size_t(x0)) * 4u;
            for (int xx = x0; xx < x1; ++xx) {
                p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
                p += 4;
            }
        }
    }

    void stroke_rect(int x, int y, int w, int h, u8 r, u8 g, u8 b) {
        fill_rect(x, y, w, 1, r, g, b);
        fill_rect(x, y + h - 1, w, 1, r, g, b);
        fill_rect(x, y, 1, h, r, g, b);
        fill_rect(x + w - 1, y, 1, h, r, g, b);
    }

    void draw_disc(int cx, int cy, int rad, u8 r, u8 g, u8 b) {
        int r2 = rad * rad;
        for (int dy = -rad; dy <= rad; ++dy)
            for (int dx = -rad; dx <= rad; ++dx)
                if (dx * dx + dy * dy <= r2) set_px(cx + dx, cy + dy, r, g, b);
    }

    void draw_ring(int cx, int cy, int rad, u8 r, u8 g, u8 b) {
        int r2o = rad * rad, r2i = (rad - 1) * (rad - 1);
        for (int dy = -rad; dy <= rad; ++dy)
            for (int dx = -rad; dx <= rad; ++dx) {
                int d2 = dx * dx + dy * dy;
                if (d2 <= r2o && d2 >= r2i) set_px(cx + dx, cy + dy, r, g, b);
            }
    }

    void draw_line(int x0, int y0, int x1, int y1, u8 r, u8 g, u8 b) {
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (int step = 0; step < 2048; ++step) {
            set_px(x0, y0, r, g, b);
            if (x0 == x1 && y0 == y1) break;
            int e2 = err * 2;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    // Compact built-in 5x7 uppercase/digit/symbol bitmap font (zero external font deps!)
    static void glyph_rows(char c, u8 out[7]) {
        if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
        auto set = [&](u8 r0, u8 r1, u8 r2, u8 r3, u8 r4, u8 r5, u8 r6) {
            out[0]=r0; out[1]=r1; out[2]=r2; out[3]=r3; out[4]=r4; out[5]=r5; out[6]=r6;
        };
        switch (c) {
            case '0': set(0x0e,0x11,0x13,0x15,0x19,0x11,0x0e); break;
            case '1': set(0x04,0x0c,0x04,0x04,0x04,0x04,0x0e); break;
            case '2': set(0x0e,0x11,0x01,0x02,0x04,0x08,0x1f); break;
            case '3': set(0x1f,0x02,0x04,0x02,0x01,0x11,0x0e); break;
            case '4': set(0x02,0x06,0x0a,0x12,0x1f,0x02,0x02); break;
            case '5': set(0x1f,0x10,0x1e,0x01,0x01,0x11,0x0e); break;
            case '6': set(0x06,0x08,0x10,0x1e,0x11,0x11,0x0e); break;
            case '7': set(0x1f,0x01,0x02,0x04,0x08,0x08,0x08); break;
            case '8': set(0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e); break;
            case '9': set(0x0e,0x11,0x11,0x0f,0x01,0x02,0x0c); break;
            case 'A': set(0x0e,0x11,0x11,0x1f,0x11,0x11,0x11); break;
            case 'B': set(0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e); break;
            case 'C': set(0x0e,0x11,0x10,0x10,0x10,0x11,0x0e); break;
            case 'D': set(0x1c,0x12,0x11,0x11,0x11,0x12,0x1c); break;
            case 'E': set(0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f); break;
            case 'F': set(0x1f,0x10,0x10,0x1e,0x10,0x10,0x10); break;
            case 'G': set(0x0e,0x11,0x10,0x17,0x11,0x11,0x0f); break;
            case 'H': set(0x11,0x11,0x11,0x1f,0x11,0x11,0x11); break;
            case 'I': set(0x0e,0x04,0x04,0x04,0x04,0x04,0x0e); break;
            case 'J': set(0x07,0x02,0x02,0x02,0x02,0x12,0x0c); break;
            case 'K': set(0x11,0x12,0x14,0x18,0x14,0x12,0x11); break;
            case 'L': set(0x10,0x10,0x10,0x10,0x10,0x10,0x1f); break;
            case 'M': set(0x11,0x1b,0x15,0x15,0x11,0x11,0x11); break;
            case 'N': set(0x11,0x19,0x15,0x13,0x11,0x11,0x11); break;
            case 'O': set(0x0e,0x11,0x11,0x11,0x11,0x11,0x0e); break;
            case 'P': set(0x1e,0x11,0x11,0x1e,0x10,0x10,0x10); break;
            case 'Q': set(0x0e,0x11,0x11,0x11,0x15,0x12,0x0d); break;
            case 'R': set(0x1e,0x11,0x11,0x1e,0x14,0x12,0x11); break;
            case 'S': set(0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e); break;
            case 'T': set(0x1f,0x04,0x04,0x04,0x04,0x04,0x04); break;
            case 'U': set(0x11,0x11,0x11,0x11,0x11,0x11,0x0e); break;
            case 'V': set(0x11,0x11,0x11,0x11,0x11,0x0a,0x04); break;
            case 'W': set(0x11,0x11,0x11,0x15,0x15,0x1b,0x11); break;
            case 'X': set(0x11,0x11,0x0a,0x04,0x0a,0x11,0x11); break;
            case 'Y': set(0x11,0x11,0x0a,0x04,0x04,0x04,0x04); break;
            case 'Z': set(0x1f,0x01,0x02,0x04,0x08,0x10,0x1f); break;
            case '.': set(0x00,0x00,0x00,0x00,0x00,0x0c,0x0c); break;
            case ',': set(0x00,0x00,0x00,0x00,0x04,0x04,0x08); break;
            case ':': set(0x00,0x0c,0x0c,0x00,0x0c,0x0c,0x00); break;
            case '/': set(0x01,0x02,0x02,0x04,0x08,0x08,0x10); break;
            case '-': set(0x00,0x00,0x00,0x1f,0x00,0x00,0x00); break;
            case '+': set(0x00,0x04,0x04,0x1f,0x04,0x04,0x00); break;
            case '%': set(0x18,0x19,0x02,0x04,0x08,0x13,0x03); break;
            case '|': set(0x04,0x04,0x04,0x04,0x04,0x04,0x04); break;
            case '(': set(0x02,0x04,0x08,0x08,0x08,0x04,0x02); break;
            case ')': set(0x08,0x04,0x02,0x02,0x02,0x04,0x08); break;
            case '&': set(0x0c,0x12,0x14,0x08,0x15,0x12,0x0d); break;
            default:  set(0x00,0x00,0x00,0x00,0x00,0x00,0x00); break;
        }
    }

    void draw_text(int x, int y, const std::string &s, int scale, u8 r, u8 g, u8 b) {
        int cx = x;
        for (char c : s) {
            u8 rows[7];
            glyph_rows(c, rows);
            for (int ry = 0; ry < 7; ++ry) {
                for (int rx = 0; rx < 5; ++rx) {
                    if (rows[ry] & (1u << (4 - rx))) {
                        fill_rect(cx + rx * scale, y + ry * scale, scale, scale, r, g, b);
                    }
                }
            }
            cx += 6 * scale;
        }
    }

    static std::string fmt_num(u64 v) {
        std::ostringstream os;
        if (v >= 1000000000ull) os << std::fixed << std::setprecision(2) << (double(v) * 1e-9) << "B";
        else if (v >= 1000000ull) os << std::fixed << std::setprecision(1) << (double(v) * 1e-6) << "M";
        else if (v >= 10000ull) os << std::fixed << std::setprecision(1) << (double(v) * 1e-3) << "K";
        else os << v;
        return os.str();
    }

    void draw_card(int x, int y, int w, int h, u8 ar, u8 ag, u8 ab) {
        fill_rect(x, y, w, h, 15, 28, 32);
        stroke_rect(x, y, w, h, 31, 57, 65);
        fill_rect(x + 1, y + 1, w - 2, 3, ar, ag, ab);
    }

    void draw_spark(int x, int y, int w, int h, const std::vector<float> &vals, float vmax, u8 r, u8 g, u8 b) {
        if (vals.size() < 2u || vmax <= 1e-6f) return;
        int n = int(vals.size());
        for (int i = 1; i < n; ++i) {
            int x0 = x + (i - 1) * (w - 1) / std::max(1, n - 1);
            int x1 = x + i * (w - 1) / std::max(1, n - 1);
            int y0 = y + h - 1 - int(std::clamp(vals[i - 1] / vmax, 0.0f, 1.0f) * float(h - 2));
            int y1 = y + h - 1 - int(std::clamp(vals[i] / vmax, 0.0f, 1.0f) * float(h - 2));
            draw_line(x0, y0, x1, y1, r, g, b);
            draw_line(x0, y0 + 1, x1, y1 + 1, r, g, b);
        }
    }

    void draw_hud(u32 tick, const CreatureState *champ, const BodyPhenotype *cbp, const TelemetryPoint &tp) {
        int hx = int(HUD_X) + 16;
        int hw = int(HUD_W) - 32;

        // Header card
        draw_card(hx, 14, hw, 86, 62, 230, 168);
        draw_text(hx + 14, 28, title_, 2, 238, 245, 242);
        {
            std::ostringstream sub;
            sub << "TICK " << tick << " / " << total_ticks_ << "   GEN " << tp.max_gen;
            draw_text(hx + 14, 52, sub.str(), 2, 62, 230, 168);
        }
        // Progress bar
        fill_rect(hx + 14, 78, hw - 28, 10, 11, 21, 24);
        int pw = int(float(hw - 28) * std::clamp(float(tick) / float(total_ticks_), 0.0f, 1.0f));
        fill_rect(hx + 14, 78, pw, 10, 62, 230, 168);

        // Card 2: Live Champion Body + Brain Anatomy
        draw_card(hx, 112, hw, 236, 246, 196, 83);
        draw_text(hx + 14, 126, "LIVE CHAMPION BODY + CORTEX", 2, 246, 196, 83);

        static constexpr u8 TK_RGB[TK_TYPES][3] = {
            {11, 21, 24},    // 0 EMPTY
            {203, 232, 200}, // 1 STEM
            {139, 156, 166}, // 2 STRUCT
            {239, 100, 97},  // 3 MUSCLE
            {246, 196, 83},  // 4 EYE
            {62, 230, 168},  // 5 CHEM
            {69, 199, 245},  // 6 EAR
            {240, 148, 77},  // 7 GRASP
            {199, 125, 255}, // 8 GLAND
            {75, 224, 165},  // 9 PYR
            {242, 95, 92},   // 10 PV
            {185, 117, 245}, // 11 SST
            {69, 199, 245},  // 12 MOD
        };
        int bx = hx + 16, by = 152, cs = 11;
        fill_rect(bx - 2, by - 2, 16 * cs + 4, 16 * cs + 4, 11, 21, 24);
        stroke_rect(bx - 2, by - 2, 16 * cs + 4, 16 * cs + 4, 31, 57, 65);
        if (cbp && champ) {
            for (u32 gy = 0; gy < 16u; ++gy) {
                for (u32 gx = 0; gx < 16u; ++gx) {
                    u8 tk = cbp->surface[gy * 16u + gx];
                    if (tk == 0u || tk >= TK_TYPES) continue;
                    fill_rect(bx + int(gx) * cs + 1, by + int(gy) * cs + 1, cs - 2, cs - 2,
                              TK_RGB[tk][0], TK_RGB[tk][1], TK_RGB[tk][2]);
                }
            }
            int ix = bx + 16 * cs + 18;
            std::ostringstream s1, s2, s3, s4, s5, s6;
            s1 << "LAYERS: " << int(cbp->grown_layers) << "  SYM: " << int(cbp->symmetry_pct) << "%";
            s2 << "NEURONS: " << int(cbp->active_neurons);
            s3 << "SYNAPSES: " << int(cbp->total_synapses);
            s4 << "PYR:" << cbp->counts[TK_NEU_EXC] << " PV:" << cbp->counts[TK_NEU_PV]
               << " SST:" << cbp->counts[TK_NEU_SST];
            s5 << "EYE:" << cbp->counts[TK_EYE] << " EAR:" << cbp->counts[TK_EAR]
               << " GL:" << cbp->counts[TK_GLAND];
            s6 << "ATP:" << int(champ->energy_gained) << " OFF:" << champ->offspring;
            draw_text(ix, 158, s1.str(), 1, 238, 245, 242);
            draw_text(ix, 176, s2.str(), 2, 62, 230, 168);
            draw_text(ix, 200, s3.str(), 1, 157, 184, 180);
            draw_text(ix, 218, s4.str(), 1, 185, 117, 245);
            draw_text(ix, 236, s5.str(), 1, 246, 196, 83);
            draw_text(ix, 254, s6.str(), 1, 69, 199, 245);

            // E/I Composition Bar
            u32 exc_n = cbp->counts[TK_NEU_EXC];
            u32 inh_n = cbp->counts[TK_NEU_PV] + cbp->counts[TK_NEU_SST];
            u32 mod_n = cbp->counts[TK_NEU_MOD];
            u32 tot_n = std::max(1u, exc_n + inh_n + mod_n);
            int bw = hx + hw - 16 - ix;
            int ew = int(float(bw) * float(exc_n) / float(tot_n));
            int iw = int(float(bw) * float(inh_n) / float(tot_n));
            draw_text(ix, 276, "E/I CORTICAL BALANCE", 1, 101, 131, 128);
            fill_rect(ix, 290, bw, 14, 11, 21, 24);
            fill_rect(ix, 290, ew, 14, 75, 224, 165);
            fill_rect(ix + ew, 290, iw, 14, 242, 95, 92);
            fill_rect(ix + ew + iw, 290, std::max(0, bw - ew - iw), 14, 69, 199, 245);
            std::ostringstream ipct;
            ipct << (100u * inh_n / tot_n) << "% INHIBITORY (DALE)";
            draw_text(ix, 312, ipct.str(), 1, 242, 130, 120);
        }

        // Card 3: Live Evolutionary Telemetry Curves
        draw_card(hx, 360, hw, 242, 69, 199, 245);
        draw_text(hx + 14, 374, "EVOLUTIONARY DYNAMICS OVER TICKS", 2, 69, 199, 245);
        std::vector<float> v_alive, v_spec, v_lay, v_neu;
        v_alive.reserve(hist_.size()); v_spec.reserve(hist_.size());
        v_lay.reserve(hist_.size()); v_neu.reserve(hist_.size());
        float max_l = 4.0f, max_n = 100.0f;
        for (const auto &p : hist_) {
            v_alive.push_back(float(p.alive));
            v_spec.push_back(float(p.species));
            v_lay.push_back(p.mean_layers);
            v_neu.push_back(p.mean_neurons);
            if (p.mean_layers > max_l) max_l = p.mean_layers;
            if (p.mean_neurons > max_n) max_n = p.mean_neurons;
        }
        // Chart A: Alive & Morphs
        int cx0 = hx + 14, cw = hw - 28;
        fill_rect(cx0, 400, cw, 82, 11, 21, 24);
        stroke_rect(cx0, 400, cw, 82, 24, 44, 52);
        {
            std::ostringstream lb;
            lb << "ALIVE:" << tp.alive << "  MORPHS:" << tp.species;
            draw_text(cx0 + 6, 406, lb.str(), 1, 157, 184, 180);
        }
        draw_spark(cx0 + 2, 418, cw - 4, 60, v_alive, float(pop_n_), 62, 230, 168);
        draw_spark(cx0 + 2, 418, cw - 4, 60, v_spec, float(pop_n_), 69, 199, 245);

        // Chart B: Mean Layers & Mean Neurons
        fill_rect(cx0, 496, cw, 92, 11, 21, 24);
        stroke_rect(cx0, 496, cw, 92, 24, 44, 52);
        {
            std::ostringstream lb;
            lb << std::fixed << std::setprecision(2) << "LAYERS:" << tp.mean_layers
               << "  NEURONS:" << std::setprecision(1) << tp.mean_neurons;
            draw_text(cx0 + 6, 502, lb.str(), 1, 199, 125, 255);
        }
        draw_spark(cx0 + 2, 514, cw - 4, 70, v_lay, max_l * 1.15f, 199, 125, 255);
        draw_spark(cx0 + 2, 514, cw - 4, 70, v_neu, max_n * 1.15f, 246, 196, 83);

        // Card 4: Emergent Behavioral & Cultural Counters
        draw_card(hx, 614, hw, 140, 199, 125, 255);
        draw_text(hx + 14, 628, "EMERGENT ECOLOGY & LEARNING", 2, 199, 125, 255);
        auto metric_box = [&](int bx0, int by0, int bw0, const char *lbl, const std::string &val_s, u8 vr, u8 vg, u8 vb) {
            fill_rect(bx0, by0, bw0, 40, 11, 21, 24);
            stroke_rect(bx0, by0, bw0, 40, 28, 52, 60);
            draw_text(bx0 + 6, by0 + 6, lbl, 1, 101, 131, 128);
            draw_text(bx0 + 6, by0 + 20, val_s, 2, vr, vg, vb);
        };
        int mw = (hw - 40) / 3;
        metric_box(hx + 14, 654, mw, "BIRTHS", fmt_num(tp.births), 62, 230, 168);
        metric_box(hx + 20 + mw, 654, mw, "PREY / PODS", fmt_num(tp.prey) + "/" + fmt_num(tp.pods), 69, 199, 245);
        metric_box(hx + 26 + mw * 2, 654, mw, "VOCALS", fmt_num(tp.vocal), 199, 125, 255);
        metric_box(hx + 14, 702, mw, "REPLIES", fmt_num(tp.replies), 246, 196, 83);
        metric_box(hx + 20 + mw, 702, mw, "SHELTERS", fmt_num(tp.shelters), 240, 148, 77);
        metric_box(hx + 26 + mw * 2, 702, mw, "STDP EDITS", fmt_num(tp.edits), 62, 230, 168);
    }
};

} // namespace axion::genesis
