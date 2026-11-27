#pragma once
#include "recollection.cuh"

namespace axion::life::imagery {
AX_FN Word permutation(Word key) {
    Word packed=0xe4;
    for (Word n=4;n>1;--n) {
        Word i=key%n;key/=n;Word a=2*i,b=2*(n-1),difference=((packed>>a)^(packed>>b))&3;
        packed^=(difference<<a)|(difference<<b);
    }
    return packed;
}

AX_FN Word field(const Imagery &image,Word form,bool recruit) {
    Word first=mix(form)%6,empty=6;
    for (Word k=0;k<6;++k) {
        Word j=(first+k)%6;
        if (image.weights[j] && image.forms[j]==form) return j;
        if (!image.weights[j] && empty==6) empty=j;
    }
    return recruit ? (empty<6 ? empty : first) : 6u;
}
AX_FN void experience(Imagery &image,RecollectionGenes genes,Word sound,Word before,Word after) {
    Word j=field(image,sound,true);
    Recollection tissue{genes,image.forms[j]==sound ? image.weights[j] : 0u};
    engram::see(tissue,before,1u<<after,true);
    if (tissue.weights) {image.forms[j]=sound;image.weights[j]=tissue.weights;}
    else if (image.forms[j]==sound) image.weights[j]=0;
}
AX_FN Word propagate(Word paths,Word active) {
    Word out=0;
    while (active) {Word cell=Word(__builtin_ctz(active));active&=active-1;out|=(paths>>(4*cell))&15;}
    return out;
}
AX_FN Word compose(Word paths,Word destination) {
    // Column j contains four disjoint low bits, one per source row.
    // Multiplying by one destination nibble broadcasts the new spikes to
    // those rows without carry between them, including branching predictions.
    return ((paths&0x1111u)*(destination&15))
          |(((paths>>1)&0x1111u)*((destination>>4)&15))
          |(((paths>>2)&0x1111u)*((destination>>8)&15))
          |(((paths>>3)&0x1111u)*((destination>>12)&15));
}
AX_FN Word readout(RecollectionGenes genes,Word weights) {
    Recollection tissue{genes,weights};Word destination=0;
    for (Word k=0;k<4;++k) destination|=engram::reinstate(tissue,k)<<(4*k);
    return destination;
}
AX_FN bool replay(Projection &wave,const Imagery &image,RecollectionGenes genes,Word sound) {
    Word j=field(image,sound,false);if (j==6) return false;
    wave.paths=compose(wave.paths,readout(genes,image.weights[j]));++wave.actions;return true;
}
}
