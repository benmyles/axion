#pragma once
#include "attention.cuh"
#include "axion/recollection.hpp"

namespace axion::life::school {
AX_FN Word operation(Word slot,Word key) {return (slot*((key>>4)|1)+key)&3;}
AX_FN Word conversation(const Word *words,const Iris &iris,Word length,Word key,Word language,Word dialect,Word cut) {
    Word desired=(key>>22)&1,pitch=desired^dialect;
    for (Word j=0;j<length;++j) {
        Word token=mix(key^j*0x6c932d91u)&3;
        desired=attention::goal(operation(token,language),desired);
        pitch=attention::response(words[token],pitch);
        if (pitch>1) return 0;
    }
    Word muscle=cut==2 ? 2 : attention::response(iris.contacts,pitch^Word(cut==3));
    return muscle==desired;
}
AX_FN RecallScore course(RecollectionGenes genes,Word seed,Word cycles,Word length,Word cut) {
    Word words[4]{},dialect=mix(seed)&1;Iris iris{cut==1 ? 0u : genes.high,0};
    attention::teach_iris(iris,dialect,seed);
    RecallScore s{};
    for (Word phase=0;phase<2;++phase) {
        Word language=mix(seed^0x17d4bc39u)^(phase*3);
        for (Word k=0;k<96;++k) {
            Word draw=(k&3)^(mix(seed^(k/4)^phase)&3),prior=draw&1,pitch=draw>>1;
            Word before=attention::response(iris.contacts,prior),after=attention::response(iris.contacts,pitch);
            for (Word token=0;token<4;++token) {
                bool fed=before<2 && after<2 && after==attention::goal(operation(token,language),before);
                if (cut==4) fed=bool(mix(seed^k^token*0xfe701a11u^phase)&1);
                attention::reinforce(words[token],prior,pitch,fed,cut==1 ? 0u : genes.low);
            }
        }
        for (Word j=0;j<cycles;++j) {
            Word key=mix(seed^j*0x29e5b027u^phase*0x3d7a15b3u);
            Word single=conversation(words,iris,1,key,language,dialect,cut);
            Word chain=conversation(words,iris,length,key^0x971ae605u,language,dialect,cut);
            if (phase) {s.revision+=single;s.faint+=chain;}
            else {s.one+=single;s.delay+=conversation(words,iris,2,key^0xb63c02d1u,language,dialect,cut);s.noise+=chain;}
        }
    }
    return s;
}
}

extern "C" __attribute__((global)) void focus_school(axion::life::AttentiveEar *ear,axion::life::Iris *iris,
    axion::life::RecollectionGenes genes,const axion::Word *forms,const axion::Word *operations,axion::Word n,
    axion::Word dialect,axion::Word seed) {
    using namespace axion::life;
    *ear={};*iris={genes.high,0};ear->rule=genes.low;ear->previous=dialect;
    attention::teach_iris(*iris,dialect,seed);
    for (axion::Word i=0;i<n;++i) attention::teach_source(*ear,*iris,forms[i],operations[i],seed^forms[i]);
}
