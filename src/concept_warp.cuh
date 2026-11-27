#pragma once
#include "concept.cuh"

namespace axion::life::meaning {
AX_FN Word broadcast(Word x,Word source=0) {
    Word result;asm volatile("shfl.sync.idx.b32 %0,%1,%2,31,0xffffffff;":"=r"(result):"r"(x),"r"(source));return result;
}
AX_FN Word ballot(Word value) {
    Word mask;asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,0xffffffff; }":"=r"(mask):"r"(value));return mask;
}
AX_FN void warp_fence() {asm volatile("bar.warp.sync 0xffffffff;":::"memory");}
// The lanes own the two-bit synapses of one field. A single ballot writes
// an entire weight plane, without atomics or cross-resident communication.
AX_FN void demonstrate_warp(Concepts &c,Count form,Word from,Word to,bool reuse,Word lane) {
    Word j=broadcast(lane ? 0 : recruit(c,form,reuse));warp_fence();
    if (j>=concept_words) return;
    auto &w=c.words[j];Activation first{};
    if (lane<15 && (w.inputs&(1u<<lane))) first=propagate(c.views[0],c.domain,atom(c,lane),{1u<<from,0});
    first={broadcast(first.cells,lane&15),broadcast(first.gap,lane&15)};
    for (Word k=0;k<8;++k) {
        Word next=0;
        if (valid(w,k)&(1u<<lane)) {
            Word second=2*k+(lane>>4);auto prediction=first;
            if (second!=15) prediction=propagate(c.views[0],c.domain,atom(c,second),prediction);
            Word old=((w.low[k]>>lane)&1)|(((w.high[k]>>lane)&1)<<1);
            next=update(c.genes.high,old,evidence(prediction,to));
        }
        Word low=ballot(next&1),high=ballot(next&2);
        if (!lane) {w.low[k]=low;w.high[k]=high;}
    }
    warp_fence();if (!lane) consolidate(c,j);warp_fence();
}
}
