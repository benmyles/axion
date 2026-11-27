#pragma once
#include "causal.cuh"

namespace axion::life::lantern {
// Each lever permutes the lights, applies a reversible triangular XOR
// coupling, and may invert them. Every lamp has at most two afferents.
// A blind sequence therefore remains a permutation of all 64 states.
AX_FN Apparatus cabinet(Word seed,Word sites=32) {
    Apparatus w{};w.present=sites==32 ? ~0u : (1u<<sites)-1;
    for (Word a=0;a<operations;++a) {
        Word key=mix(seed^mix(a+173)),order[traits]{0,1,2,3,4,5};
        for (Word i=traits-1;i;--i) {Word k=mix(key+i)%(i+1),v=order[k];order[k]=order[i];order[i]=v;}
        for (Word i=0;i<traits;++i) {
            Word first=order[i],bits=mix(key+i*0xd467f21bu),second=first,lut=10;
            if (i && (bits&3)!=0) {second=order[(bits>>4)%i];lut=6;}
            if (bits&8) lut^=15;
            w.gates[a][i]=static_cast<unsigned short>(causal::gate(first,second,lut));
        }
    }
    w.food=causal::gate(seed%6,seed%6,10);
    for (Word i=0;i<sites;++i) w.plots[i]=static_cast<unsigned char>(mix(seed+i)&63);
    return w;
}
}
