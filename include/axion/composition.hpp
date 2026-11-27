#pragma once
#include "recollection.hpp"
#include "life.hpp"

namespace axion::life::composition {
using Byte=unsigned char;
constexpr Word width=8,forms=255,patterns=12,queries=12,projections=48,episodes=240,slots=248;
constexpr Byte absent=255;
struct Phrase {Byte words[width]{},size=0;};
struct Pattern {Phrase prototype{};Byte fixed=0;};
struct Projection {Byte type=0,left=0,right=absent,out=0;};
struct Fibre {
    Byte source=absent,other=absent,first=0,second=0,repeat=0;
    Byte gate=0,gate_source=absent,condition=0,literal=0;
};
struct Construction {Pattern cue{};Fibre voice[width]{};Byte length=0,ready=0,support=0;};
struct Anatomy {
    Pattern facts[patterns]{};
    Projection fields[projections]{};
    Construction phrases[queries]{};
    Count used=0,clocks=0,dated=0;
    Word facts_n=0,fields_n=0,phrases_n=0;
};
struct Episode {Phrase stimulus{},response{};};
struct Experience {Episode event[episodes]{};Word n=0,seen=0;};
struct Voices {Count pulse[256]{},confidence[256]{};Byte cache[256]{};Word n=0;};
struct Contact {Word address=0,time=0;};
template<int Capacity> struct Connections {
    Contact rows[Capacity]{};
    Byte heads[64]{},chain[Capacity]{};
    Word n=0,time=0,full=0;
    Phrase prior{},spoken{};
};
struct Question {
    Phrase original{},prompt{};
    Byte field=0,left=0,right=0,construction=0,channel=0,exchanges=0;
};
struct Mind {
    RecollectionGenes genes{};
    Anatomy anatomy{};
    Voices lexicon{};
    Connections<slots> memory{};
    Experience lessons{};
    Question pending{};
    Word revisions=0,turns=0,consolidated=0,extent=0;
};
struct Mouth {Count words[width]{};Word size=0;};
struct Exchange {Mouth heard{},reply{};Word status=0,evidence=0,depth=0;};
struct Measures {
    Word queries=0,correct=0,voiced=0,displaced=0,displaced_correct=0,deep=0,deep_correct=0,fresh=0;
    Word unknown_quiet=0,unknown=0,actions=0,missed_at=0;Mouth first_input{},expected{},produced{};
};
struct Detections {
    unsigned short tail[512]{};
    Byte last[episodes]{};
    struct Link {Word address;unsigned short time,previous;};
    Link records[episodes*projections]{};
    Word n=0;
};
AX_FN bool matches(const Pattern &p,const Phrase &x) {
    if (p.prototype.size!=x.size || !x.size) return false;
    for (Word k=0;k<x.size;++k) if ((p.fixed&(1u<<k)) && p.prototype.words[k]!=x.words[k]) return false;
    return true;
}
AX_FN Word address(Word field,Word a,Word b) {return (field<<16)|(b<<8)|a;}
AX_FN Word bucket(Word key) {return mix(key^0x319ed171u);}
AX_FN Word actual(Word packed) {return packed>>24;}
AX_FN Word key_of(Word packed) {return packed&0x3fffff;}
static_assert(__builtin_offsetof(Mind,extent)+sizeof(Word)==sizeof(Mind));
static_assert(sizeof(Contact)==8 && sizeof(Episode)==18 && sizeof(Projection)==4);
constexpr Word alternatives=8;
struct Rivals {Fibre f[alternatives];Word n=0,alive=0;};
struct Competition {Rivals voice[queries][width];};
struct Tuition {Word heard=0,asked=0,disagreements=0,removed=0,changed=0,unexplained=0;};
#if !defined(__CUDA__)
int command(int argc,char **argv);
#endif
}
