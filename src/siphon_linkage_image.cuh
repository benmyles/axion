#include "axion/linkage_image.hpp"
namespace axion::life::linkage {
AX_FN Word wave(Word active) {
    Word v;
    asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,0xffffffff; }"
                 : "=r"(v)
                 : "r"(active));
    return v;
}
AX_FN void barrier() { asm volatile("bar.warp.sync 0xffffffff;" ::: "memory"); }
AX_FN Count packed(Plan p) {
    return Count(p.course.low) | (Count(p.course.high) << 24) | (Count(p.course.len) << 36) | (Count(p.found) << 40);
}
AX_FN Plan release(const Panel &p, const Byte *image, Word state) {
    Plan result{};
    for (Word j = 0; j <= horizon(p.body); ++j) {
        Word motor = (image[state >> 1] >> (4 * (state & 1))) & 15;
        if (motor == stop) {
            result.found = 1;
            break;
        }
        if (motor > 7 || j == horizon(p.body))
            return {};
        append(result.course, motor);
        state = predict(p.model, p.rig, state, motor);
        if (state == absent)
            return {};
    }
    return result;
}
} // namespace axion::life::linkage
KERNEL void linkage_compile(const linkage::Panel *cases, Word n, Word states, linkage::Byte *images) {
    using namespace linkage;
    Word i = tid(), subject = i >> 5, lane = i & 31, w = subject & 3;
    if (subject >= n)
        return;
    const auto &p = cases[subject];
    __attribute__((shared)) static unsigned char arc[4][8][256];
    __attribute__((shared)) static Word lit[4][8];
    Word first[8];
    for (Word b = 0; b < 8; ++b) {
        Word state = 32 * b + lane, code = matches(state, p.target) ? stop : no_path;
        first[b] = code;
        if (state < states) {
            for (Word m = 0; m < 8; ++m) {
                Word action = motor_order(p.body, m);
                Word next = (p.rig.handles & (1u << action)) ? predict(p.model, p.rig, state, action) : absent;
                arc[w][m][state] = Byte(next == absent ? state : next);
            }
        }
    }
    for (Word j = 0; j < horizon(p.body); ++j) {
        for (Word b = 0; b < 8; ++b) {
            Word firing = wave(first[b] != no_path);
            if (lane == b)
                lit[w][b] = firing;
        }
        barrier();
        Word growth = 0;
        for (Word b = 0; b < 8; ++b) {
            Word state = 32 * b + lane;
            if (state < states && first[b] == no_path) {
                for (Word m = 0; m < 8; ++m) {
                    Word reached = arc[w][m][state];
                    if (lit[w][reached >> 5] & (1u << (reached & 31))) {
                        first[b] = motor_order(p.body, m);
                        growth = 1;
                        break;
                    }
                }
            }
        }
        barrier();
        if (!wave(growth))
            break;
    }
    for (Word b = 0; b < 8; ++b) {
        Word mate;
        asm volatile("shfl.sync.bfly.b32 %0,%1,1,31,0xffffffff;" : "=r"(mate) : "r"(first[b]));
        Word state = b * 32 + lane;
        if (!(lane & 1) && state < states)
            images[Count(subject) * (states / 2) + state / 2] = Byte(first[b] | (mate << 4));
    }
}
template <bool released>
AX_FN void linkage_calls(const linkage::Panel *p, const linkage::Byte *images, Word n, Word states, Word calls,
                         Word offset, Word seed, Count *out) {
    Word i = tid();
    if (i >= calls)
        return;
    Word a = (offset + i) % n, v = mix(seed + (i + offset) * 2003) & (states - 1);
    linkage::Plan r;
    if constexpr (released)
        r = linkage::release(p[a], images + Count(a) * (states / 2), v);
    else
        r = linkage::seek(p[a].model, p[a].rig, v, p[a].target, p[a].body);
    out[i] = linkage::packed(r);
}
#define LINKAGE_CALL(fn, fast)                                                                                       \
    KERNEL void fn(const linkage::Panel *p, const linkage::Byte *images, Word n, Word states, Word calls,            \
                   Word offset, Word seed, Count *out) {                                                             \
        linkage_calls<fast>(p, images, n, states, calls, offset, seed, out);                                         \
    }
LINKAGE_CALL(linkage_writable_plans, false)
LINKAGE_CALL(linkage_read_plans, true)
#undef LINKAGE_CALL
KERNEL void linkage_practiced(const linkage::Genes *g, Word n, Word size, Word seed, Word cap,
                              linkage::Panel *panels) {
    using namespace linkage;
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 5021);
    Material law[4];
    for (Word j = 0; j < 4; ++j)
        law[j] = matter(k, j);
    auto job = order(k, size, law);
    linkage::Mind learner{g[i]};
    linkage::Tissue bank[4]{};
    Slots s{bank, 1, cap};
    tuition(learner, s, law, types(job.rig), k, own, 2);
    panels[i] = {compile(learner, s), job.rig, job.goal, g[i], expose(job.rig, job.angle, law), i};
}
