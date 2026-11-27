#pragma once
#include "binding.cuh"

namespace axion::life::binding_world {
struct Law { Word sources,map[2],order; };
AX_FN Law physical(Word key,Word geometry) {
    Law law{mix(key^1)&3,{},imagery::permutation(mix(key^2))};
    for (Word o=0;o<2;++o) {
        Word colours=imagery::permutation(mix(key^(o+3)));
        for (Word b=0;b<4;++b) {
            Word rank=(law.order>>(2*b))&3;
            if (!geometry || (geometry==2 && !o)) rank>>=1;
            else if (geometry==2) rank&=1;
            law.map[o]|=((colours>>(2*rank))&3)<<(2*b);
        }
    }
    return law;
}
AX_FN Word effect(const Law &law,Word scene) {
    Word after=0;
    for (Word o=0;o<2;++o) {
        Word source=(law.sources>>o)&1,b=binding::coordinate(scene,source);
        after|=((law.map[o]>>(2*b))&3)<<(2*o);
    }
    return after;
}
AX_FN Word alternative(const Law &law,Word key,bool informative,Word geometry) {
    Word a=key&3,b=a,rank=(law.order>>(2*a))&3;
    Word delta=geometry==1 ? (informative ? 1+((key>>3)%3) : 0)
                           : geometry==2 ? (informative ? 3 : 1+((key>>3)&1))
                                         : (informative ? 2+((key>>4)&1) : 1);
    Word other=rank^delta;
    for (Word k=0;k<4;++k) if (((law.order>>(2*k))&3)==other) b=k;
    return a|(b<<2);
}
}
