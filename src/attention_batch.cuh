#pragma once
#include "attention.cuh"
#include "recollection.cuh"

namespace axion::life::batch_attention {
struct Pair { AttentiveEar ear;Iris iris;Recollection visual; };
struct Compiled { Word control;Recollection visual; };
static_assert(sizeof(Pair)==80 && sizeof(Compiled)==compiled_attention_bytes);
AX_FN Compiled compile(const Pair &p,const Word *forms) {
    Compiled result{p.ear.previous<<16,p.visual};
    for (Word i=0;i<8;++i) {
        auto site=attention::site_of(p.ear,forms[i],false);
        for (Word state=0;state<2;++state) {
            Word output=site<8 ? attention::response(attention::bank(p.ear,site),state) : 2;
            result.control|=(output<2 ? output : state)<<(2*i+state);
        }
    }
    for (Word pitch=0;pitch<2;++pitch) result.control|=attention::response(p.iris.contacts,pitch)<<(17+2*pitch);
    return result;
}
AX_FN Word attend(Pair &pair,Word token,const Word *forms) {
    attention::hear(pair.ear,forms[token]);return attention::response(pair.iris.contacts,pair.ear.previous);
}
AX_FN Word attend(Compiled &pair,Word token,const Word *) {
    Word pitch=(pair.control>>(2*token+((pair.control>>16)&1)))&1;
    pair.control=(pair.control&~(1u<<16))|(pitch<<16);return (pair.control>>(17+2*pitch))&3;
}
template<class Body> AX_FN void advance(Body *pairs,Word count,const Word *forms,Word steps,Word seed,Word *summary) {
    Word block,lane;asm("mov.u32 %0, %%ctaid.x;":"=r"(block));asm("mov.u32 %0, %%tid.x;":"=r"(lane));
    Word i=128*block+lane,good=0,signature=0;
    if (i<count) {
        auto &pair=pairs[i];Word known=0,source=0;
        // A new independent stream starts at the trained external setting.
        // Only the scorer has the meaning of each form and the visible world.
        for (Word j=0;j<steps;++j) {
            Word random=mix(seed^i*0x46a4c095u^j*0x63aa379du),word=random&7,cue=(random>>8)&3,light=1u<<((random>>12)&3);
            Word op=word<3 ? 0u : word<6 ? 3u : word==6 ? 1u : 2u;source=(op>>source)&1;
            Word gate=attend(pair,word,forms),retina=gate==1 ? engram::reinstate(pair.visual,cue) : gate==0 ? light : 0;
            Word expected=source ? (known>>(cue*4))&15 : light;
            good+=(gate==source && retina==expected);
            if (gate==0) {
                known=(known&~(15u<<(cue*4)))|(light<<(cue*4));
                engram::see(pair.visual,cue,light,true);
            } else engram::see(pair.visual,cue,0,false);
            signature=signature*33+retina+32*gate;
        }
        signature=mix(signature^pair.visual.weights^i);
    }
    __attribute__((shared)) static Word scores[128],signatures[128];
    scores[lane]=good;signatures[lane]=signature;
    for (Word width=64;width;width>>=1) {
        asm volatile("bar.sync 0;":::"memory");
        if (lane<width) {scores[lane]+=scores[lane+width];signatures[lane]^=signatures[lane+width];}
    }
    if (!lane) {summary[2*block]=scores[0];summary[2*block+1]=signatures[0];}
}
}
extern "C" __attribute__((global)) void focus_batch_seed(unsigned char *storage,axion::Word n,
    const axion::life::AttentiveEar *ear,const axion::life::Iris *iris,axion::life::Recollection visual,
    const axion::Word *forms,axion::Word packed) {
    using namespace axion::life::batch_attention;
    axion::Word block,lane;asm("mov.u32 %0, %%ctaid.x;":"=r"(block));asm("mov.u32 %0, %%tid.x;":"=r"(lane));
    axion::Word i=128*block+lane;if (i>=n) return;
    Pair pair{*ear,*iris,visual};pair.visual.weights=0;
    if (packed) reinterpret_cast<Compiled *>(storage)[i]=compile(pair,forms);
    else reinterpret_cast<Pair *>(storage)[i]=pair;
}
extern "C" __attribute__((global)) void focus_batch_direct(axion::life::batch_attention::Pair *pairs,axion::Word n,
    const axion::Word *forms,axion::Word steps,axion::Word seed,axion::Word *stats) {
    axion::life::batch_attention::advance(pairs,n,forms,steps,seed,stats);
}
extern "C" __attribute__((global)) void focus_batch_compiled(axion::life::batch_attention::Compiled *pairs,axion::Word n,
    const axion::Word *forms,axion::Word steps,axion::Word seed,axion::Word *stats) {
    axion::life::batch_attention::advance(pairs,n,forms,steps,seed,stats);
}
