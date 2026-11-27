#pragma once
#include "life.hpp"
#include "recollection.hpp"

namespace axion::life::procedure {
using Byte=unsigned char;
using Half=unsigned short;
constexpr Word letters=112,lessons=96,experience_bytes=6144,productions=64,code_bytes=1536;
constexpr Word motif=56,arguments=2,positions=32;
constexpr Byte call0=251,call1=252,quote0=253,quote1=254;
// The two sides use the same sensory receptor bank. A copied fragment can
// therefore be emitted on the motor port without knowing its name.
struct Ear {Count model[letters+1]{},strength[letters+1]{};Byte cache[128]{};Word size=0,reserved=0;};
struct Trace {Half at=0;Byte before=0,after=0;};
struct Memory {
    Trace record[lessons]{};Byte retina[lessons]{},visible[lessons]{},tape[experience_bytes]{};
    Word count=0,bytes=0,observed=0;
};
struct Cell {
    Half at=0;Byte input=0,output=0,arity=0,raw=0,support=0,flags=0;
};
struct Guard {Word known[8]{},second[8]{};Byte effect[motif]{},length=0,reserved[3]{};};
constexpr Word guard_limit=8;
struct Circuit {
    Cell cell[productions]{};Byte wire[code_bytes]{};Guard guards[guard_limit]{};Word count=0,bytes=0,guard_n=0;
};
constexpr Word loops=4,loop_bytes=256;
struct Arm {Half at=0;Byte n=0,m=0;Word cuts=0;};
struct Recursion {
    Byte prefix[motif]{},suffix[motif]{},base[24]{};
    Byte head=0,tail=0,center=0,n=0,support=0,reserved=0;Half bytes=0;
    Arm arm[12]{};Byte wire[loop_bytes]{};
};
struct Recursions {Recursion state[loops]{};Word n=0;};
constexpr Word effectors=12;
struct MotorTrace {Byte retina=0,gesture=0,echo=0;};
struct MotorPolicy {
    Count echo[effectors]{};
    Word contact[4][effectors]{};
    Word first=0,second=0,forms=0,practice=0;
};
constexpr Word performance_span=512,performance_depth=128;
struct Counter {Half node=0,offset=0;Byte pc=0,branch=0;};
struct Performance {
    Byte input[performance_span]{};Counter stack[performance_depth]{};
    Half size=0,depth=0;Word nodes=0,revision=0,active=0;
    Count moves=0;
};
struct Mind {
    RecollectionGenes genes{};Word reserved=0;Ear ear{};Circuit circuit{};Recursions recursion{};MotorPolicy muscle{};Memory memory{};Word reserved_after_memory=0;Performance performance{};
    Word retina=0,observing=0;
    alignas(8) Word revisions=0;Word consolidated=0,conversations=0,extent=0;
};
struct Slice {const Byte *p;Word n;};
struct Draft {Byte in[motif]{},out[motif]{};Word l=0,r=0,arity=0,raw=0;};
struct Site {Byte begin=0,end=0,source=0,raw=0;};
struct Origins {Site site[positions]{};Word n=0;};
struct Verdict {Word support=0,score=0;};
struct GateProposal {Guard gate{};Byte alternative[motif]{};Word length=0;};
struct Fit {
    Origins origin[lessons]{};Draft proposed[lessons]{};Verdict quality[lessons]{};GateProposal guarded[lessons]{};
    Recursions acquired{};Byte folded[lessons]{};Word clipped=0;
};
struct Stats {Word active=0,lexical=0,rules=0,copiers=0,literals=0,revision=0;};
enum Status : Word { unrecognized,recognized,ambiguous,capacity,needs_observation };
struct Node {Half rule=0,left=0,right=0,begin=0,end=0,split=0;};
struct Summary {Count length=0,hash=0,power=1;};
struct SceneCheck {Word all=0,pulses=0,changes=0,trials=0,expected=0,sensitive=0;};
struct Report {
    Summary voice{};Word status=unrecognized,depth=0,nodes=0,spoken=0;Count first=0;
};
AX_FN bool same(Slice a,Slice b) {
    if (a.n!=b.n) return false;
    for (Word i=0;i<a.n;++i) if (a.p[i]!=b.p[i]) return false;
    return true;
}
AX_FN Slice before(const Memory &m,Word i) {auto e=m.record[i];return {m.tape+e.at,e.before};}
AX_FN Slice after(const Memory &m,Word i) {auto e=m.record[i];return {m.tape+e.at+e.before,e.after};}
AX_FN Slice cue(const Circuit &c,Word i) {auto e=c.cell[i];return {c.wire+e.at,e.input};}
AX_FN Slice output(const Circuit &c,Word i) {auto e=c.cell[i];return {c.wire+e.at+e.input,e.output};}
AX_FN Slice part(Slice s,Word i,Word n) {return {s.p+i,n};}
AX_FN bool at(Slice s,Word i,Slice pattern) {return i<=s.n && pattern.n<=s.n-i && same(part(s,i,pattern.n),pattern);}
AX_FN bool var(Byte b) {return b>=call0;}
AX_FN Word slot(Byte b) {return (b==call1 || b==quote1) ? 1 : 0;}
AX_FN Count signal(Word symbol) {
    // The acoustic transducer has 64 simultaneous receptors. A transport
    // character excites a Walsh carrier. Input contains no word boundaries.
    Count code=0;for (Word k=0;k<6;++k) if (symbol&(1u<<k)) {
        const Count masks[]{0xaaaaaaaaaaaaaaaaull,0xccccccccccccccccull,0xf0f0f0f0f0f0f0f0ull,
            0xff00ff00ff00ff00ull,0xffff0000ffff0000ull,0xffffffff00000000ull};code^=masks[k];
    }
    return (symbol&64 ? ~code : code)^0xa6471d3cb95e280full;
}
static_assert(sizeof(Cell)==8 && sizeof(Trace)==4 && __builtin_offsetof(Mind,extent)+sizeof(Word)==sizeof(Mind));
#if !defined(__CUDA__)
int command(int argc,char **argv);
#endif
}
