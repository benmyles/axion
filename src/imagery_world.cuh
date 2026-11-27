#pragma once
#include "imagery.cuh"

namespace axion::life::imagery_world {
struct Device { Word routes[4]; };
AX_FN Device apparatus(Word seed,Word geometry,Word changed) {
    Device world{};
    for (Word act=0;act<4;++act) {
        Word key=mix(seed^mix(act+changed*47));
        if (!geometry) {
            Word offsets=0x2d; // clockwise, counterclockwise, half turn, stationary
            Word shift=((offsets>>(2*act))&3)^(changed ? 2 : 0);
            for (Word b=0;b<4;++b) world.routes[act]|=((b+shift)&3)<<(2*b);
        } else world.routes[act]=geometry==1 ? imagery::permutation(key) : key&255;
    }
    return world;
}
AX_FN Word move(const Device &world,Word act,Word b) {return (world.routes[act]>>(2*b))&3;}
AX_FN void practice(Imagery &image,RecollectionGenes genes,const Device &world,const Word *names,Word key,Word visits,Word cut) {
    for (Word t=0;t<visits;++t) {
        // The apparatus places the body at four starting bearings in an
        // independently shuffled sweep. The brain sees a departure, the sound
        // made by its action, and the actual arrival produced by that apparatus.
        Word order=mix(key^(t/16)*0x905a76d7u);
        Word choice=((t&15)*((order>>8)|1)+order)&15,act=choice>>2,b=choice&3;
        Word recipient=cut==2 ? (choice+1+((order>>16)%15))&15 : choice;
        Word observed=move(world,recipient>>2,recipient&3);
        if (cut!=1) imagery::experience(image,genes,names[act],b,observed);
    }
}
AX_FN Word probe(const Imagery &image,RecollectionGenes genes,const Device &world,const Word *names,const Word *compiled,Word key,Word length,Word cut) {
    Word start=(key>>9)&3,truth=start;Projection wave{};
    for (Word j=0;j<length;++j) {
        Word act=mix(key^j*0xde94b3afu)&3;truth=move(world,act,truth);
        Word heard=cut==4 ? mix(key^(length-1-j)*0xde94b3afu)&3 : act;
        if (cut==6) wave.paths=imagery::compose(wave.paths,(compiled[heard>>1]>>(16*(heard&1)))&65535);
        else if (cut!=3) imagery::replay(wave,image,genes,names[heard]);
    }
    return imagery::propagate(wave.paths,1u<<start)==(1u<<truth);
}
}
extern "C" __attribute__((global)) void imagery_trials(axion::life::RecollectionGenes genes,const axion::Word *names,
    axion::Word worlds,axion::Word seed,axion::Word visits,axion::Word probes,axion::Word length,axion::Word geometry,axion::Word cut,
    axion::life::RecallScore *scores,axion::Word first,axion::Word count) {
    using namespace axion;using namespace axion::life;using namespace axion::life::imagery_world;
    Word block,lane;asm("mov.u32 %0, %%ctaid.x;":"=r"(block));asm("mov.u32 %0, %%tid.x;":"=r"(lane));
    Word local=block*128+lane;if (local>=count) return;Word i=first+local;if (i>=worlds) return;
    Word key=mix(seed^i*0x4603a9c1u);Imagery image{};RecallScore result{};
    for (Word phase=0;phase<2;++phase) {
        Device world=apparatus(key,geometry,phase);
        if (!phase || cut!=5) practice(image,genes,world,names,key^phase,visits,cut);
        Word compiled[2]{};
        if (cut==6) for (Word k=0;k<4;++k) {
            Word j=imagery::field(image,names[k],false);
            Word effect=j==6 ? Projection{}.paths : imagery::readout(genes,image.weights[j]);
            compiled[k>>1]|=effect<<(16*(k&1));
        }
        for (Word j=0;j<probes;++j) {
            Word trial=mix(key^j*0x8578d9cfu^phase);
            Word one=probe(image,genes,world,names,compiled,trial,1,cut),many=probe(image,genes,world,names,compiled,trial^0x59a21d83u,length,cut);
            if (phase) {result.revision+=one;result.faint+=many;}
            else {result.one+=one;result.delay+=probe(image,genes,world,names,compiled,trial^0x3ba317edu,2,cut);result.noise+=many;}
        }
    }
    scores[i]=result;
}
extern "C" __attribute__((global)) void imagery_school(axion::life::Imagery *image,axion::life::RecollectionGenes genes,
    const axion::Word *names,axion::Word seed,axion::Word visits,axion::Word geometry,axion::Word cut) {
    using namespace axion::life::imagery_world;
    practice(*image,genes,apparatus(seed,geometry,0),names,seed,visits,cut);
}
