#pragma once
#include "recollection.cuh"

namespace axion::life::proof {
// Given a cue, its four independent two-bit contacts have 256 possible joint
// states. Unrelated cues can still modify these contacts through the inherited
// rule. Include both anatomical aliasing and every event on other rows.
AX_FN Recollection materialize(RecollectionGenes g,Word state) {
    Word shift=4*engram::row(g,0);
    return {g,((state&15)<<shift)|((state>>4)<<(16+shift))};
}
AX_FN Word advance(RecollectionGenes g,Word state,Word cue,Word event) {
    auto m=materialize(g,state);
    engram::see(m,cue,event<8 ? 1u<<(event&3) : 0u,event<4);
    Word bits=m.weights>>(4*engram::row(g,0));
    return (bits&15)|((bits>>12)&240);
}
AX_FN void propagate(Word *reached,Word next,Word flags) {
    if (flags) asm volatile("red.or.b32 [%0], %1;"::"l"(reached+next),"r"(flags):"memory");
}
AX_FN Word pending_threads(Word work) {
    Word count;
    asm volatile("{ .reg .pred p; setp.ne.u32 p, %1, 0; bar.red.popc.u32 %0, 0, p; }"
                 :"=r"(count):"r"(work):"memory");
    return count;
}
AX_FN void observe_claim(MemoryProof &out,Word kind,Word expected,Word actual,Word state,Word depth) {
    asm volatile("red.global.add.u32 [%0], 1;"::"l"(&out.checked[kind]):"memory");
    if (actual==expected) return;
    // A reproducible witness records the shortest event depth, the expected
    // one-hot motor field, actual field, and focal contact state at failure.
    Word witness=(depth<<16)|(expected<<12)|(actual<<8)|state;
    asm volatile("red.global.add.u32 [%0], 1; red.global.min.u32 [%1], %2;"
                 ::"l"(&out.failed[kind]),"l"(&out.first[kind]),"r"(witness):"memory");
}
}

// Each block verifies a separate individual's inherited program. Bits in
// reached[state] are proof obligations reachable through actual sensory events.
// 0 unrestricted, 1 unseen, 2..5 clear, 6..9 one conflicting faint sighting,
// 10..25 a consistent faint target seen one through four times. Every obligation
// is closed under any number of distractors and covered inspections.
extern "C" __attribute__((global)) void recollect_certify(const axion::life::RecollectionGenes *genes,
                                                          axion::life::MemoryProof *results) {
    using namespace axion;
    using namespace axion::life;
    using namespace axion::life::proof;
    Word group,state;
    asm("mov.u32 %0, %%ctaid.x;":"=r"(group));
    asm("mov.u32 %0, %%tid.x;":"=r"(state));
    __attribute__((shared)) static Word reached[256];
    reached[state]=state==0 ? 3u : 0u;
    auto &out=results[group];
    if (!state) {
        out={};
        for (auto &first:out.first) first=~0u;
    }
    auto g=genes[group];
    Word direct[9],other[9],distinct=0;bool alias=false;
    for (Word cue=1;cue<4;++cue) {
        if (engram::row(g,cue)==engram::row(g,0)) alias=true;
        else distinct=cue;
    }
    for (Word e=0;e<9;++e) {
        direct[e]=advance(g,state,0,e);other[e]=advance(g,state,distinct,e);
    }
    Word actual=engram::reinstate(materialize(g,state),0),visited=0;
    asm volatile("bar.sync 0;":::"memory");
    for (Word depth=0;;++depth) {
        Word fresh=reached[state]&~visited;visited|=fresh;
        if (!pending_threads(fresh)) break;
        if (fresh) {
            if (fresh&1) asm volatile("red.global.add.u32 [%0], 1;"::"l"(&out.reachable):"memory");
            if (fresh&2) observe_claim(out,0,0,actual,state,depth);
            for (Word j=0;j<4;++j) {
                Word bit=1u<<j;
                if (fresh&(bit<<2)) observe_claim(out,1,bit,actual,state,depth);
                if (fresh&(bit<<6)) observe_claim(out,2,bit,actual,state,depth);
                if (fresh&(bit<<22)) observe_claim(out,3,bit,actual,state,depth);
                if (fresh&1) {
                    propagate(reached,direct[j],1|(bit<<2));
                    propagate(reached,direct[4+j],1|(bit<<10));
                }
                Word outliers=((fresh>>2)&15&~bit)<<6;
                Word repeat=(fresh&((bit<<10)|(bit<<14)|(bit<<18)))<<4;
                propagate(reached,direct[4+j],outliers|repeat|(fresh&(bit<<22)));
            }
            propagate(reached,direct[8],fresh);
            for (Word e=0;e<9;++e) {
                if (distinct) propagate(reached,other[e],fresh);
                if (alias) propagate(reached,direct[e],fresh);
            }
        }
        asm volatile("bar.sync 0;":::"memory");
    }
}
