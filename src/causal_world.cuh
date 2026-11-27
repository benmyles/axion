#pragma once
#include "causal.cuh"
#include "concept.cuh"

namespace axion::life::greenhouse {
AX_FN Apparatus garden(Word seed,Word objects=32) {
    Apparatus w{};w.present=meaning::domain(objects);
    Word property[traits]{0,1,2,3,4,5},motor[operations]{0,1,2,3,4,5},negate=0;
    if (seed) {
        negate=mix(seed)&63;
        for (Word j=traits-1;j>0;--j) {
            Word k=mix(seed+j)%(j+1),t=property[j];property[j]=property[k];property[k]=t;
            k=mix(seed+j+97)%(j+1);t=motor[j];motor[j]=motor[k];motor[k]=t;
        }
    }
    for (Word a=0;a<operations;++a) for (Word p=0;p<traits;++p) {
        Word i=p,j=p,v=10;
        if (a==p) {
            if (a<3) v=15;
            else {i=a==3 ? 0 : a==4 ? 1 : 4;j=a==3 ? 2 : 3;v=8;}
        }
        // Warming evaporates moisture. The order of actions can matter even
        // when all six stimuli and the final desired state are identical.
        if (a==1 && p==0) v=0;
        Word left=property[i],right=property[j],output=property[p],lut=0;
        for (Word x=0;x<4;++x) {
            Word address=x^((negate>>left)&1)^(((negate>>right)&1)<<1);
            lut|=(((v>>address)^(negate>>output))&1)<<x;
        }
        w.gates[motor[a]][output]=static_cast<unsigned short>(causal::gate(left,right,lut));
    }
    for (Word i=0;i<objects;++i) w.plots[i]=static_cast<unsigned char>(seed ? (mix(seed^i*0x846ca68bu)&63) : 0);
    Word fruit=property[5];w.food=causal::gate(fruit,fruit,(negate&(1u<<fruit)) ? 5 : 10);w.dormant=negate;
    return w;
}
// Model selection sees a two-input physical feature in arbitrary wiring,
// inhibitory and excitatory polarities, across transitions and remappings.
AX_FN Word law(Word key) {
    Word j=key%pairs_of_traits;return causal::gate(causal::from(j),causal::other(j),(key>>8)&15);
}
AX_FN Word permute(Word j,Word key) {
    j=(j*((key>>8)|1)+key)&63;j^=j>>3;j=((j<<2)|(j>>4))&63;return (j*((key>>20)|1)+(key>>16))&63;
}
AX_FN void show(CausalField &f,RecollectionGenes g,Word x,Word y,Word out) {
    causal::learn_field(f,g,causal::patterns(x),all_pairs,y,((x>>out)&1)!=y);
}
AX_FN RecallScore lifetime(RecollectionGenes g,Word seed,Word cycles,Word lessons,Word cut) {
    RecallScore score{};
    for (Word t=0;t<cycles;++t) {
        Word key=mix(seed+t*0x9e3779b9u),op=law(key),p=(key>>16)%traits;CausalField f{};
        Word first=permute(0,key),q=first^(1u<<((p+1)%traits)),y=causal::evaluate(op,first);
        if (cut!=1) show(f,g,first,y,p);
        score.one+=causal::read_field(f,g,first,causal::patterns(first))==(1u<<y);
        score.faint+=causal::read_field(f,g,q,causal::patterns(q))==3;
        Word n=lessons>55 ? 55 : lessons;
        for (Word i=1;i<n && cut!=1;++i) {
            Word x=permute(i,key);show(f,g,x,causal::evaluate(op,cut==2 ? permute((i+7)&63,key) : x),p);
        }
        for (Word j=0;j<8;++j) {
            Word x=permute(63-j,key);bool yes=causal::read_field(f,g,x,causal::patterns(x))==(1u<<causal::evaluate(op,x));
            score.noise+=yes;score.delay+=cut!=3 && yes && f.compiled!=0;
        }
        Word changed=law(mix(key+7));
        for (Word j=0;j<n && cut!=1;++j) {
            Word x=permute(j,key+391);show(f,g,x,causal::evaluate(changed,x),p);
        }
        for (Word j=0;j<8;++j) {
            Word x=permute(63-j,key+391);score.revision+=causal::read_field(f,g,x,causal::patterns(x))==(1u<<causal::evaluate(changed,x));
        }
        // Two handles at each stop. Only the handle selected by the physical
        // feature opens the next stop. The animal must establish a complete
        // route internally, then its choices are tried in the actual chain.
        Word length=2+key%9,motors=0,known=0,actual=0;
        for (Word j=0;j<length;++j) {
            Word x=permute(53-j,key+391),r=causal::read_field(f,g,x,causal::patterns(x));
            if (r==1 || r==2) known|=1u<<j;
            if (r==2) motors|=1u<<j;
            actual|=causal::evaluate(changed,x)<<j;
        }
        Word reached=1u<<length;
        for (Word beat=0;beat<1+(g.expression&63);++beat) {
            Word next=reached|((reached>>1)&known);if (next==reached) break;reached=next;
        }
        score.empty+=bool(reached&1) && motors==actual;
    }
    return score;
}
}
