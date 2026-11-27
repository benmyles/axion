#pragma once
#include "recollection.hpp"
#include "life.hpp"
#if !defined(__CUDA__)
#include <string>
#include <string_view>
#endif

namespace axion::life {
constexpr Word report_words=24,report_length=8,report_sources=7,report_stop=31;
struct VocalColumn { Word contacts[report_sources]{}; };
struct VocalFrame { Word cue=0;VocalColumn phases[report_length+1]{}; };
struct Episodes { Word retina=0,recent=0,earlier=0; };
struct Reports {
    RecollectionGenes genes{0,0,0};
    Episodes history{};
    VocalFrame frames[2]{};
    Count syllables[report_words]{};
    Word request=0,heard[2]{},heard_phases=0;
};
struct Utterance { Count sound[report_length]{};Word count=0,handled=0; };
namespace acoustic {
// A reversible port encoding spreads short phoneme strings across the 64
// receptors. The creature receives the waveform and never its spelling.
AX_FN Count wave(Count x) {
    x=(x^(x>>30))*0xbf58476d1ce4e5b9ull;x=(x^(x>>27))*0x94d049bb133111ebull;return x^(x>>31);
}
AX_FN Count unwave(Count x) {
    x=(x^(x>>31)^(x>>62))*0x319642b2d24d8ec3ull;
    x=(x^(x>>27)^(x>>54))*0x96de1b173f119089ull;return x^(x>>30)^(x>>60);
}
}
static_assert(sizeof(VocalColumn)==28 && sizeof(VocalFrame)==256 && sizeof(Reports)==744 && sizeof(Utterance)==72);
#if !defined(__CUDA__)
inline Count sound_pattern(std::string_view text) {
    if (text.empty() || text.size()>12) return 0;
    Count out=0;unsigned shift=0;
    for (unsigned char c:text) {
        if (c>='A' && c<='Z') c=static_cast<unsigned char>(c+32);
        if (c<'a' || c>'z') return 0;
        out|=Count(c-'a'+1)<<shift;shift+=5;
    }
    return out;
}
inline std::string transcribe(Count sound) {
    std::string text;
    for (;sound;sound>>=5) {Word v=Word(sound&31);if (!v || v>26) return {};text+=char('a'+v-1);}
    return text;
}
int report_command(int argc,char **argv);
#endif
}
