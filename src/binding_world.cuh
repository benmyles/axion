#pragma once
#include "binding.cuh"
#include "binding_apparatus.cuh"
#include "conjunction.cuh"

namespace axion::life::binding_world {
AX_FN Word release(const RoleProjection &model,Word side,Word scene,BindingGenes g) {
    return binding::answer(model,side,1u<<(scene&3),1u<<(scene>>2),g);
}
AX_FN RecallScore lifetime(BindingGenes g,RecollectionGenes vision,Word key,Word cycles,Word setting,Word cut) {
    RecallScore score{};
    for (Word turn=0;turn<cycles;++turn) {
        Word rnd=mix(key^turn*0x9e3779b9u);
        Word geometry=setting==3 ? mix(rnd^0x998e3347u)%3 : setting;
        auto law=physical(rnd,geometry);BindingField f{};Word confidence=0;
        for (Word phase=0;phase<2;++phase) {
            if (phase) law.sources^=3;
            Word seed=mix(rnd^phase);
            for (Word n=0;n<8;++n) {
                Word b=(n+(seed&3))&3,scene=b*5;
                binding::observe(f,confidence,g,vision,scene,effect(law,scene),cut!=1);
            }
            Word probe=alternative(law,mix(seed+21),true,geometry);
            if (!phase) {
                auto model=binding::readout(f,confidence,g,vision);
                score.one+=!release(model,0,probe,g) && !release(model,1,probe,g);
            }
            Word offered=0,diagnostic=(seed>>6)&3;
            for (Word k=0;k<4;++k) offered|=alternative(law,mix(seed+k+33),k==diagnostic,geometry)<<(4*k);
            Word chosen=binding::try_scene(f,vision,g,offered,mix(seed+13),cut==2);
            if (!phase) score.empty+=chosen==diagnostic;
            Word branch=cut==3 ? ((diagnostic+1)&3) : chosen;
            Word experiment=(offered>>(4*branch))&15;
            Word departing=cut==4 ? ((experiment&3)<<2)|(experiment>>2) : experiment;
            Word arrival=effect(law,departing);
            binding::observe(f,confidence,g,vision,experiment,arrival,cut!=1);
            auto model=binding::readout(f,confidence,g,vision);
            Word truth=effect(law,probe);
            bool a=release(model,0,probe,g)==(1u<<(truth&3)),b=release(model,1,probe,g)==(1u<<(truth>>2));
            if (!phase) {score.delay+=a;score.noise+=b;} else {score.revision+=a;score.faint+=b;}
        }
    }
    return score;
}
}
extern "C" __attribute__((global)) void binding_live(const axion::life::RecollectionGenes *pool,axion::life::RecallScore *out,
    axion::Word candidates,axion::Word worlds,axion::Word seed,axion::Word cycles,axion::Word geometry,axion::Word,
    axion::Word cut,axion::Word first,axion::Word batch,axion::life::RecollectionGenes vision) {
    using namespace axion;using namespace axion::life;
    Word block,lane;asm("mov.u32 %0, %%ctaid.x;":"=r"(block));asm("mov.u32 %0, %%tid.x;":"=r"(lane));
    Word local=block*128+lane;if (local>=batch) return;Word i=first+local;if (i>=candidates*worlds) return;
    auto g=pool[i/worlds];out[i]=binding_world::lifetime({g.low,g.high,g.expression},vision,mix(seed^(i%worlds)*0xda3e39cbu),cycles,geometry,cut);
}
extern "C" __attribute__((global)) void binding_school(axion::life::Bindings *layer,axion::life::RecollectionGenes vision,
    const axion::Word *names,axion::Word seed,axion::Word names_count,axion::life::Conjunctions *joint) {
    using namespace axion;using namespace axion::life;
    // The teaching apparatus physically exchanges the objects for the first
    // two forms. The other actuators copy the first or second location.
    for (Word j=0;j<names_count;++j) for (Word t=0;t<16;++t) {
        Word a=(t+seed)&3,b=(t<8 ? a : a+1+((t>>2)&1))&3;
        Word before=a|(b<<2),after=j<2 ? b|(a<<2) : (j==2 ? a : b)*5;
        conjunction::experience(*layer,joint,vision,names[j],before,after,true);
    }
}
