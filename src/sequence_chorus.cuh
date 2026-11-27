#pragma once
#include "sequence_action.cuh"
#include "sequence_world.cuh"
namespace axion::life::chorus {
struct Heredity {
    RecollectionGenes development;
    Word circuit=0,receptors=0,larynx=0,appetite=0,founder=0,edits=0,alive=1;
};
struct Season {
    Word partner=0,energy=0,meals=0,attempts=0,aims=0,echoes=0,accurate=0,previous=0;
};
AX_FN Count sound(Word seed,Word index) {return chambers::pattern(seed^0xae29d6f3u^index*0x27d4eb2du);}
AX_FN Word voice(const Heredity &g,Word &state,Word gesture) {
    Word width=3+(g.larynx&3),mask=(1u<<width)-1;
    Word before=state&mask,left=((before<<1)|(before>>(width-1)))&mask,right=(before>>1)|((before&1)<<(width-1));
    Word stimulus=(g.receptors>>(gesture*6))&mask,next=0;
    for (Word k=0;k<width;++k) {
        Word address=((left>>k)&1)|(((before>>k)&1)<<1)|(((right>>k)&1)<<2)|(((stimulus>>k)&1)<<3);
        Word table=g.circuit>>(((g.larynx>>(16+k))&1)*16);
        next|=((table>>address)&1)<<k;
    }
    state=next;
    return (Word(__builtin_popcount(next&((g.larynx>>3)&mask)))&1)|
           ((Word(__builtin_popcount(next&((g.larynx>>9)&mask)))&1)<<1);
}
AX_FN void hatch(SequenceMind &m,const Heredity &g,Word seed) {
    m={};m.genes=g.development;m.map.size=1;m.active=1;m.anchored=1;
    for (Word a=0;a<4;++a) sequence::motor(m,chambers::pattern(seed^a*0x369deae1u),true);
}
AX_FN Heredity mutate(Heredity g,Word draw) {
    Word *genes[]{&g.development.low,&g.development.high,&g.development.expression,&g.circuit,&g.receptors,&g.larynx,&g.appetite};
    Word chance=4+((g.appetite>>24)&63);
    for (Word k=0;k<7;++k) {
        Word key=mix(draw^(k+1)*0x9e3779b9u);
        if ((key&255)<chance) {*genes[k]^=1u<<((key>>8)&31);++g.edits;}
    }
    return g;
}
}

namespace axion::life::chorus {
struct Repertoire { unsigned char response[256]{};Word size=0; };
// Canonical observable behavior from a quiet vocal ring. Minimized states
// are numbered by their shortest access word in physical gesture order.
AX_FN Repertoire repertoire(const Heredity &g) {
    unsigned char order[64]{},index[64],edge[64][4],labels[64]{},next[64],representative[64];
    for (Word i=0;i<64;++i) index[i]=64;
    Word reachable=1;index[0]=0;
    for (Word q=0;q<reachable;++q) for (Word a=0;a<4;++a) {
        Word dest=order[q],echo=voice(g,dest,a);
        if (index[dest]==64) {index[dest]=static_cast<unsigned char>(reachable);order[reachable++]=static_cast<unsigned char>(dest);}
        edge[q][a]=static_cast<unsigned char>(index[dest]|(echo<<6));
    }
    Word classes=1;
    for (;;) {
        Word keys[64],n=0;
        for (Word q=0;q<reachable;++q) {
            Word key=0;
            for (Word a=0;a<4;++a) {Word e=edge[q][a];key|=((e&192)|labels[e&63])<<(8*a);}
            Word k=0;while (k<n && keys[k]!=key) ++k;
            if (k==n) {keys[n]=key;representative[n++]=static_cast<unsigned char>(q);}
            next[q]=static_cast<unsigned char>(k);
        }
        for (Word q=0;q<reachable;++q) labels[q]=next[q];
        if (classes==n) break;
        classes=n;
    }
    Repertoire r;r.size=classes;
    for (Word k=0;k<classes;++k) for (Word a=0;a<4;++a) {
        Word e=edge[representative[k]][a];
        r.response[k*4+a]=static_cast<unsigned char>((e&192)|labels[e&63]);
    }
    return r;
}
}
