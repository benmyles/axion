#pragma once
#include "concept_warp.cuh"

namespace axion::life::estuary {
AX_FN Word draw(Word seed,Word site,Word day) {return mix(seed^site*0x94b32a71u^day*0x8207ab39u);}
AX_FN Word channel(Word key,Word x) {
    // A different bijection for each current, with a five-bit cyclic rotation
    // between keyed odd multiplications. Both directions remain navigable.
    x=(x*((key>>5)|1)+key)&31;x^=x>>2;x=((x<<2)|(x>>3))&31;
    x=(x*((key>>18)|1)+(key>>13))&31;return x^(x>>3);
}
AX_FN Count passage(Word seed,Word island,Word phase,Word tier) {
    Word k=mix(seed^island*0xe68520d7u^phase*0x388997abu);
    Word a=k&7,b=((a+1+(k>>5)%3)&3)|((k>>12)&4);
    Count path=meaning::join(8+a,8+b);
    for (Word j=0;j<tier;++j) path=meaning::join(path,path);
    return path;
}
AX_FN Word drift(const RelationalView &chart,Count current,Word x) {
    while (current>1) {
        auto &f=chart.relation[current&3];x=(current&4) ? Word(__builtin_ctz(meaning::inputs(f,x))) : meaning::target(f,x);current>>=3;
    }
    return x;
}
AX_FN Word study_level(const Concepts &c,const EstuarySpeech &voice,Word depth,Word key) {
    Word bias=(c.genes.expression>>16)&3;
    if (bias==1) return depth-1;
    if (bias==2) return key%depth;
    for (Word t=0;t<depth;++t) {
        Word j=meaning::word(c,voice.routes[t]);
        if (j>=concept_words || !c.words[j].walk) return t;
    }
    return bias==3 ? key%depth : depth-1;
}
// Bits 0..4 carry a motor choice. The decision to inspect costs energy
// even when the animal learns nothing. Settlement alone opens a food cache.
constexpr Word studying=32,diagnostic=64,learned=128;
}
ENTRY void estuary_chart(const CommonsSettings *settings,const Forager *animals,const Reservoir *water,RelationalView *charts) {
    Word i=index();if (i>=settings->count) return;
    Word island=i/32,lane=i&31;auto &chart=charts[island];const auto &pool=water[island];
    Word key=estuary::draw(settings->seed,island,animals[32*island].day);
    if (settings->tidal_feedback) key=mix(key^pool.consumed);
    for (Word r=0;r<4;++r) {
        Word y=estuary::channel(mix(key+r*0x9e3779b9u),lane);auto &f=chart.relation[r];
        for (Word b=0;b<5;++b) {Word bits=meaning::ballot(y&(1u<<b));if (!lane) f.target[b]=bits;}
        if (!lane) {f.seen=~0u;f.connected=~0u;}
    }
}
ENTRY void estuary_birth(const CommonsSettings *settings,Concepts *animals,const EstuarySpeech *voice) {
    Word i=index();if (i>=settings->count) return;auto &c=animals[i];meaning::arrange(c,32,true);
    for (Word j=0;j<32;++j) meaning::point(c,voice->places[j],1u<<j);
    for (Word j=0;j<3;++j) c.verdicts[j]=voice->verdicts[j];
}
ENTRY void estuary_elders(const CommonsSettings *q,const RelationalView *charts,Concepts *elders) {
    Word i=index();if (i>=q->count/32) return;
    for (Word r=0;r<4;++r) meaning::observe_field(elders[i],r,charts[i].relation[r],true,true);
}
ENTRY void estuary_succession(const CommonsSettings *q,const Forager *bodies,const Concepts *residents,Concepts *elders,Word displaced) {
    Word island=index(),n=q->count/32;if (island>=n) return;
    Word start=32*(displaced ? (island+1)%n : island),winner=start;
    for (Word site=start+1;site<start+32;++site) if (bodies[site].offspring>bodies[winner].offspring) winner=site;
    if (bodies[winner].offspring) elders[island]=residents[winner];else elders[island]={};
}
namespace axion::life::estuary {
template<bool Cooperative> AX_FN void forage(Word i,Word lane,const CommonsSettings &q,Concepts &c,const EstuarySpeech &voice,
        const Forager &a,const Reservoir &pool,const RelationalView &chart,Word policy,Word muscle,Word *decisions,const Concepts *elders) {
    if (!a.energy) return;
    Word key=draw(q.seed,i,a.day),x=key&31,deep=q.route_depth-1;
    if (Cooperative) {
        if (lane<4) meaning::observe_field(c,lane,chart.relation[lane],true,true);
        meaning::warp_fence();
    } else for (Word r=0;r<4;++r) meaning::observe_field(c,r,chart.relation[r],true,true);
    Word decision=0,level=0,source=0,testimony=hidden_link;bool ambient=a.day<q.exposures;
    if (!lane) {
        Word j=meaning::word(c,voice.routes[deep]),guess=j<concept_words ? meaning::identifier(meaning::infer(c,j,0,{1u<<x,0})) : hidden_link;
        Word pulse=(muscle>>((policy>>24)&7))&1,address=Word(guess>=32)+2*Word(a.receipt==2)+4*pulse;
        bool inspect=q.policy==1 ? false : q.policy==2 ? true : ((policy>>(8+address))&1)!=0;
        decision=(guess<32 ? guess : muscle&31)|(inspect ? studying : 0);
        bool listen=elders && a.day<q.mentoring && ((c.genes.expression>>(28+Word(guess>=32)+2*pulse))&1);
        if (inspect || ambient || listen) {
            level=study_level(c,voice,q.route_depth,mix(key+73));
            Word j=meaning::word(c,voice.routes[level]),start=mix(key+7)&31,stride=(key>>8)|1,offered=0;
            for (Word k=0;k<4;++k) offered|=1u<<((start+k*stride)&31);
            source=j<concept_words && inspect ? meaning::probe(c,j,offered,mix(key+41),q.choice!=0) : hidden_link;
            if (source<32 && !q.choice && ((c.genes.expression>>20)&3)) decision|=diagnostic;
            if (source>=32) source=start;
            if (listen && !inspect && !ambient) {
                const auto &elder=elders[i/32];Word other=meaning::word(elder,voice.routes[level]);
                if (other<concept_words) {
                    auto prediction=meaning::infer(elder,other,0,{1u<<source,0});
                    Count call=meaning::pronounce(elder,prediction);Word heard=meaning::noun(c,call);
                    if (meaning::unique(heard)) testimony=Word(__builtin_ctz(heard));
                }
            }
        }
    }
    if (Cooperative) {decision=meaning::broadcast(decision);source=meaning::broadcast(source);level=meaning::broadcast(level);testimony=meaning::broadcast(testimony);}
    if (q.credit && ((decision&studying) || ambient || testimony<32)) {
        // Only a paid inspection or an ambient demonstration exposes a
        // physical endpoint to the learner, after the motor choice is fixed.
        Word answer=0;
        if (!lane) answer=testimony<32 ? testimony : drift(chart,passage(q.current_seed,i/32,pool.phase,level),source);
        if (Cooperative) {answer=meaning::broadcast(answer);meaning::demonstrate_warp(c,voice.routes[level],source,answer,q.route_reuse!=0,lane);}
        else meaning::demonstrate(c,voice.routes[level],source,answer,q.route_reuse!=0);
        decision|=learned;
    }
    if (!lane) decisions[i]=decision|((decisions[i]&0xffff0000u)+(Word(q.credit && testimony<32)<<16));
}
}
#define ESTUARY_ENTRY(NAME,GROUPED) \
ENTRY void NAME(const CommonsSettings *q,Concepts *c,const EstuarySpeech *v,const Forager *a,const Reservoir *p, \
                const RelationalView *chart,const Bindings *b,const Response *m,Word *out,const Concepts *elders) { \
    Word thread=index(),i=GROUPED ? thread/32 : thread;if (i>=q->count) return; \
    estuary::forage<GROUPED>(i,GROUPED ? thread&31 : 0,*q,c[i],*v,a[i],p[i/32],chart[i/32],b[i].genes.competition,q->soma ? m[i].ports : 0,out,elders); \
}
ESTUARY_ENTRY(estuary_warp,true)
ESTUARY_ENTRY(estuary_direct,false)
#undef ESTUARY_ENTRY
ENTRY void estuary_census(const CommonsSettings *q,const Concepts *animals,const EstuarySpeech *voice,const Word *decisions,Word *out) {
    Word i=index(),lane=i&31;if (i>=q->count) return;
    for (Word t=0;t<q->route_depth;++t) {
        Word j=meaning::word(animals[i],voice->routes[t]);Word count=__builtin_popcount(meaning::ballot(j<concept_words && animals[i].words[j].walk));
        if (!lane && count) asm volatile("red.global.add.u32 [%0],%1;"::"l"(out+t),"r"(count):"memory");
    }
    Word lessons=decisions[i]>>16;
    for (Word step=16;step;step>>=1) lessons+=meaning::broadcast(lessons,lane^step);
    if (!lane && lessons) asm volatile("red.global.add.u32 [%0],%1;"::"l"(out+3),"r"(lessons):"memory");
}
