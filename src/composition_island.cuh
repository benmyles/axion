#pragma once
#include "composition_social.cuh"
#include "composition_world.cuh"

namespace axion::life::market {
using namespace composition;
struct Genome {RecollectionGenes learner{};Word founder=0,edits=0,alive=1;};
struct Harvest {
    Word queries=0,correct=0,asked=0,heard=0,repaired=0,private_days=0,private_correct=0,meals=0,energy=0;
    Word checks=0,paid=0;
};
struct Life {Harvest first,second;};
AX_FN Genome offspring(Genome g,Word draw) {
    Word *loci[]{&g.learner.low,&g.learner.high,&g.learner.expression};
    Word probability=8+((g.learner.expression>>22)&63);
    for (Word j=0;j<3;++j) {Word k=mix(draw+(1+j)*0xda3e39cbu);if ((k&255)<probability) {*loci[j]^=1u<<((k>>9)&31);++g.edits;}}
    return g;
}
AX_FN void transmit(Mind &s,const Mouth &mouth) {
    auto x=perceive(s.lexicon,mouth,s.genes,true);Word handled=0,depth=0;
    act(s.anatomy,s.memory,s.genes,x,handled,depth);
}
AX_FN Word receive(Mind &a,Mind &b,const Mouth &need,const shelter::Dialect &d,Harvest &log,Word arm,bool retry) {
    Exchange e;e.heard=need;e.evidence=retry;converse(a,e,arm!=3);
    for (Word q=0;q<3 && e.status==4;++q) {
        ++log.asked;
        if (arm==1) {a.pending={};return absent;}
        Exchange reply;reply.heard=e.reply;converse(b,reply,false);
        if (reply.status!=2 || !reply.reply.size) {a.pending={};return absent;}
        if (arm==2) for (Word j=0;j<reply.reply.size;++j) for (Word k=0;k<8;++k)
            if (reply.reply.words[j]==d.names[56+k]) {reply.reply.words[j]=d.names[56+((k+1)&7)];break;}
        ++log.heard;e={};e.heard=reply.reply;converse(a,e,true);
    }
    if (e.status!=2) return absent;
    Word site=absent;
    for (Word j=0;j<e.reply.size;++j) for (Word s=88;s<92;++s) if (e.reply.words[j]==d.names[s]) {
        if (site!=absent && site!=s) return absent;
        site=s;
    }
    return site;
}
AX_FN void day(Mind &a,Mind &b,const shelter::Dialect &d,shelter::Physical &w,Word person,Word arm,bool hidden,Harvest &log,Word &won) {
    Mouth demand,unused;shelter::encounter(w,d,{shelter::search,person},demand,unused);
    Word real=shelter::root(w.parent,w.taste[person]),before=log.asked;
    Word place=receive(a,b,demand,d,log,arm,false);++log.queries;
    if (place!=real && place!=absent && arm!=4) {
        ++log.checks;place=receive(a,b,demand,d,log,arm,true);log.repaired+=place==real;++log.paid;
    }
    won=place==real && real!=absent;log.correct+=won;log.paid+=1+log.asked-before;
    if (hidden) {++log.private_days;log.private_correct+=won;}
}
}
