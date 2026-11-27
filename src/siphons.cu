#include "axion/floodplain.hpp"
using namespace axion;
using namespace axion::life;
using namespace axion::life::flood;
#define KERNEL extern "C" __attribute__((global))
namespace {
AX_FN Word tid() {
    Word b, t, n;
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(b));
    asm("mov.u32 %0, %%tid.x;" : "=r"(t));
    asm("mov.u32 %0, %%ntid.x;" : "=r"(n));
    return b * n + t;
}
AX_FN Word raft_shuffle(Word value, Word source) {
    Word v;
    asm volatile("shfl.sync.idx.b32 %0,%1,%2,31,0xffffffff;" : "=r"(v) : "r"(value), "r"(source));
    return v;
}
} // namespace
#include "siphon_colony.cuh"
#include "siphon_language.cuh"
#include "siphon_clay.cuh"
#include "siphon_voices.cuh"
#include "siphon_press.cuh"
#include "siphon_press_words.cuh"
#include "siphon_hollows.cuh"
#include "siphon_marsh.cuh"
#include "siphon_effort.cuh"
#include "siphon_effort_words.cuh"
#include "siphon_linkages.cuh"
#include "siphon_linkage_image.cuh"
#include "siphon_linkage_culture.cuh"
#include "siphon_linkage_words.cuh"
#include "siphon_linkage_weather.cuh"
#include "siphon_practice.cuh"
#include "siphon_practice_words.cuh"
#include "siphon_practice_patches.cuh"
#include "siphon_practice_image.cuh"

#include "siphon_flume.cuh"

#include "siphon_flume_trials.cuh"

#include "siphon_backwater.cuh"
