#pragma once
#include "concept.cuh"

namespace axion::life::concept_world {
struct Board {unsigned char links[4][32]{};Word n=0;};
AX_FN Board build(Word key,Word n,bool confounded=false) {
    Board b{};b.n=n;
    for (Word r=0;r<4;++r) {
        for (Word i=0;i<n;++i) b.links[r][i]=static_cast<unsigned char>(i);
        Word random=mix(key^(r+1)*0x63d843d7u);
        for (Word j=n;j>1;--j) {
            random=mix(random+j);Word i=random%j;
            auto t=b.links[r][i];b.links[r][i]=b.links[r][j-1];b.links[r][j-1]=t;
        }
    }
    if (confounded) for (Word i=0;i<n;++i) b.links[1][i]=b.links[0][i];
    return b;
}
AX_FN void present(Concepts &c,const Board &b) {
    for (Word r=0;r<4;++r) for (Word x=0;x<b.n;++x) meaning::observe(c,r,x,b.links[r][x],true);
}
AX_FN Word next(const Board &b,Word r,Word x) {
    if (r<4) return b.links[r][x];
    for (Word i=0;i<b.n;++i) if (b.links[r&3][i]==x) return i;
    return hidden_link;
}
AX_FN Word outcome(const Board &b,Count route,Word x) {
    while (route>1) {x=next(b,Word(route)&7,x);route>>=3;}
    return x;
}
AX_FN Word prediction(const Concepts &c,Word word,Word bank,Word x) {
    return meaning::identifier(meaning::infer(c,word,bank,{1u<<x,0}));
}
AX_FN void teach(Concepts &c,const Board &b,Count name,Count route,Word count,Word key,bool muted) {
    for (Word j=0;j<count && !muted;++j) {
        Word x=(key+j*((key>>8)|1))&(b.n-1);
        meaning::demonstrate(c,name,x,outcome(b,route,x));
    }
}
AX_FN RecallScore lifetime(RecollectionGenes genes,Word seed,Word cycles,Word lessons,Word cut) {
    RecallScore score{};
    for (Word t=0;t<cycles;++t) {
        Word key=mix(seed^t*0x866d649bu),x=(key>>12)&15;
        Concepts c{};c.genes=genes;meaning::arrange(c,16,true);
        auto first=build(key,16,true);present(c,first);
        constexpr Count label=94,compound=113;
        teach(c,first,label,8,lessons,key,cut==1);
        Word twin=(x+8)&15,shown=first.links[1][x];
        meaning::observe(c,1,x,first.links[1][twin],true);meaning::observe(c,1,twin,shown,true);
        Word offered=(1u<<x)|(1u<<((x+1)&15))|(1u<<((x+2)&15))|(1u<<((x+3)&15));
        if (c.words[0].form && prediction(c,0,0,x)==hidden_link)
            score.faint+=meaning::probe(c,0,offered,mix(key+8),cut==5)==x;
        auto second=build(mix(key+1),16);present(c,second);
        Word a=key&7,b=(key>>4)&7;Count base=meaning::join(8+a,8+b);
        teach(c,second,label,base,lessons,mix(key+2),cut==1);
        auto third=build(mix(key+3),16);present(c,third);
        score.noise+=c.words[0].form && prediction(c,0,0,x)==outcome(third,base,x);
        // A new word can reuse the first word twice. Its four-hop meaning
        // cannot in general be expressed by the primitive two-synapse field.
        Count doubled=meaning::join(base,base);
        if (cut==3) c.words[0].walk=0;
        teach(c,third,compound,doubled,lessons,mix(key+4),cut==1);
        auto fourth=build(mix(key+5),16);present(c,fourth);
        score.revision+=c.words[1].form && prediction(c,1,0,x)==outcome(fourth,doubled,x);
        Word r=(key>>6)&3,y=fourth.links[r][x],new_y=(y+1+((key>>8)%15))&15;
        // A public marker identifies the edited port. The destination can be
        // seen privately, jointly, or by neither animal.
        for (Word condition=0;condition<8;++condition) {
            meaning::observe(c,r,x,y,true);
            meaning::observe(c,r,x,(condition&1) ? new_y : hidden_link,(condition&2)!=0,(condition&4)!=0);
            Word self=meaning::identifier(meaning::step(c.views[0],c.domain,r,{1u<<x,0}));
            Word peer=meaning::identifier(meaning::step(c.views[cut==2 ? 0 : 1],c.domain,r,{1u<<x,0}));
            score.one+=self==((condition&1) ? new_y : y);
            score.delay+=peer==((condition&2) ? ((condition&1) ? new_y : hidden_link) : y);
        }
        Count name=461;Word other=(x+1)&15,third_label=(x+2)&15;
        meaning::point(c,name,(1u<<x)|(1u<<other));meaning::point(c,name,(1u<<x)|(1u<<third_label));
        bool resolved=meaning::noun(c,name)==(1u<<x);
        meaning::point(c,name,1u<<other);
        score.empty+=resolved && meaning::noun(c,name)==(1u<<other);
    }
    return score;
}
}
