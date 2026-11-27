#pragma once
#include "conjunction_world.cuh"

namespace axion::life::conjunction_batch {
struct Acquired { JointField joint{};BindingField roles{};Word strength=0; };
struct Frozen { Word bearings[2]{},valid=0; };
static_assert(sizeof(Acquired)==52 && sizeof(Frozen)==12);

AX_FN Frozen compile(const Acquired &a,RecollectionGenes vision,BindingGenes sources,RecollectionGenes gates) {
    auto roles=binding::readout(a.roles,a.strength,sources,vision);
    Frozen f{};
    for (Word side=0;side<2;++side) {
        conjunction::Candidates p{};
        bool joint=conjunction::readout(a.joint,side,vision,gates,p);
        for (Word scene=0;scene<16;++scene) {
            Word m=joint ? conjunction::candidates(p,scene) : binding::answer(roles,side,1u<<(scene&3),1u<<(scene>>2),sources);
            if (m && !(m&(m-1))) {
                f.valid|=1u<<(16*side+scene);
                f.bearings[side]|=Word(__builtin_ctz(m))<<(2*scene);
            }
        }
    }
    return f;
}
AX_FN Word query(const Frozen &f,Word scene,Word side) {
    Word valid=(f.valid>>(16*side+scene))&1;
    return valid<<((f.bearings[side]>>(2*scene))&3);
}
AX_FN void write(const Acquired &a,Word *bank,Word count,Word i) {
    for (Word o=0;o<2;++o) for (Word y=0;y<4;++y) bank[(4*o+y)*count+i]=a.joint.contacts[o][y];
    for (Word j=0;j<4;++j) bank[(8+j)*count+i]=a.roles.contacts[j];
    bank[12*count+i]=a.strength;
}
AX_FN Acquired read(const Word *bank,Word count,Word i) {
    Acquired a{};
    for (Word o=0;o<2;++o) for (Word y=0;y<4;++y) a.joint.contacts[o][y]=bank[(4*o+y)*count+i];
    for (Word j=0;j<4;++j) a.roles.contacts[j]=bank[(8+j)*count+i];
    a.strength=bank[12*count+i];return a;
}
AX_FN Acquired rear(Word key,Word mode,RecollectionGenes vision,BindingGenes sources) {
    Acquired a{};
    conjunction_world::Apparatus law{imagery::permutation(key),imagery::permutation(mix(key+1)),imagery::permutation(mix(key+2)),mode&1};
    Word mask=mode==0 ? 65535 : mode==1 ? 0x7662 : mode==2 ? 65535^0x33u : 0x8423;
    for (Word n=0;n<16;++n) {
        Word s=(key+n*((key>>8)|1))&15;
        if (!(mask&(1u<<s))) continue;
        Word before=law.scene(s),u=law.result(s),v=mode==3 ? before>>2 : (u+2)&3;
        Word after=u|(v<<2);
        binding::observe(a.roles,a.strength,sources,vision,before,after);
        conjunction::observe_output(a.joint,vision,before,0,1u<<u);
        conjunction::observe_output(a.joint,vision,before,1,1u<<v);
    }
    return a;
}
}
