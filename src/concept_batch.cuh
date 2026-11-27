#pragma once
#include "concept_warp.cuh"

// The batch reads one acquired word on independent, partially observed
// worlds. Compiled readouts answer the single-object motor query.
KERNEL void concept_batch_show(const Concepts *c,RelationalView *views,Word n,Word seed) {
    Word t=thread_index(),i=t/32,lane=t&31;if (i>=n) return;
    for (Word r=0;r<4;++r) {
        Word k=mix(seed^i*0x62a753b9u^r*0xb95483b5u^lane*0x9e3779b9u);
        Word shown=(k%37)!=0,gate=(c->genes.low>>(4*(2+shown)))&3;
        Word seen=shown && (gate==1 || gate==3),connected=seen && (k%43)!=0;
        auto &f=views[i].relation[r];
        for (Word j=0;j<5;++j) {Word field=meaning::ballot(connected && (k&(1u<<j)));if (!lane) f.target[j]=field;}
        Word a=meaning::ballot(seen),b=meaning::ballot(connected);if (!lane) {f.seen=a;f.connected=b;}
    }
}
KERNEL void concept_batch_compile(const Concepts *c,Word word,const RelationalView *views,Word *planes,Word n) {
    Word t=thread_index(),i=t/32,lane=t&31;if (i>=n) return;
    Word y=meaning::identifier(meaning::infer_view(*c,word,views[i],~0u,{1u<<lane,0}));
    Word cell=(i/32)*192+(i&31);
    for (Word j=0;j<5;++j) {Word value=meaning::ballot(y<32 && (y&(1u<<j)));if (!lane) planes[cell+32*j]=value;}
    Word grounded=meaning::ballot(y<32);if (!lane) planes[cell+160]=grounded;
}
namespace axion::life::meaning {
AX_FN EndpointProjection load_projection(const Word *planes,Word i) {
    EndpointProjection p{};Word cell=(i/32)*192+(i&31);p.grounded=planes[cell+160];
    for (Word j=0;j<5;++j) p.labels[j]=planes[cell+32*j];return p;
}
}
KERNEL void concept_batch_verify(const Concepts *c,Word word,const RelationalView *views,const Word *planes,Word n,Word *disagreements) {
    Word t=thread_index(),i=t/32,lane=t&31;if (i>=n) return;
    Word y=meaning::identifier(meaning::infer_view(*c,word,views[i],~0u,{1u<<lane,0}));
    Word diff=meaning::ballot(y!=read_endpoint(meaning::load_projection(planes,i),lane));if (!lane) disagreements[i]=diff;
}
template<bool Projected> AX_FN void request_endpoints(const Concepts *c,Word word,const RelationalView *views,const Word *planes,Word n,Word decisions,Word salt,Word *out) {
    Word i=thread_index();if (i>=n) return;Word key=mix(salt^i),digest=key;
    EndpointProjection p{};if (Projected) p=meaning::load_projection(planes,i);
    for (Word j=0;j<decisions;++j) {
        Word x=(j*(key|1)+(key>>8))&31;
        Word y=Projected ? read_endpoint(p,x) : meaning::identifier(meaning::infer_view(*c,word,views[i],~0u,{1u<<x,0}));
        digest=(digest^(y+1))*16777619u;
    }
    out[i]=digest;
}
#define ENDPOINT_ENTRY(NAME,PROJECTED) \
KERNEL void NAME(const Concepts *c,Word word,const RelationalView *views,const Word *p,Word n,Word decisions,Word salt,Word *out) { \
    request_endpoints<PROJECTED>(c,word,views,p,n,decisions,salt,out); }
ENDPOINT_ENTRY(concept_batch_read,false)
ENDPOINT_ENTRY(concept_batch_projected,true)
#undef ENDPOINT_ENTRY
