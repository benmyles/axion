#pragma once
#include "axion/attention.hpp"

namespace axion::life::attention {
AX_FN Word response(Word contacts,Word row) {
    Word weights=contacts>>(8*row),a=weights&15,b=(weights>>4)&15;
    return (a>b && a>=3) ? 0u : (b>a && b>=3) ? 1u : 2u;
}
AX_FN void reinforce(Word &contacts,Word row,Word action,bool fed,Word rule) {
    Word shift=8*row+4*action,old=(contacts>>shift)&15;
    contacts=(contacts&~(15u<<shift))|(auditory_charge(old,fed,3,rule)<<shift);
}
AX_FN Word bank(const AttentiveEar &ear,Word site) {
    return (ear.contacts[site/2]>>(16*(site&1)))&65535;
}
AX_FN Word site_of(const AttentiveEar &ear,Word form,bool recruit) {
    Word fallback=8,weakest=~0u;
    for (Word k=0;k<8;++k) {
        Word site=(mix(form)+k)&7,w=bank(ear,site);
        if (w && ear.forms[site]==form) return site;
        Word mass=(w&15)+((w>>4)&15)+((w>>8)&15)+(w>>12);
        if (mass<weakest) { weakest=mass;fallback=site; }
    }
    return recruit ? fallback : 8u;
}
AX_FN bool hear(AttentiveEar &ear,Word form) {
    Word site=site_of(ear,form,false);if (site==8) return false;
    Word next=response(bank(ear,site),ear.previous);
    if (next>1) return false;
    ear.previous=next;return true;
}
AX_FN void lesson(AttentiveEar &ear,Word form,Word prior,Word action,bool fed) {
    Word site=site_of(ear,form,true),shift=16*(site&1);
    Word weights=ear.forms[site]==form ? bank(ear,site) : 0;
    reinforce(weights,prior,action,fed,ear.rule);
    if (!weights && ear.forms[site]!=form) return;
    ear.forms[site]=form;
    ear.contacts[site/2]=(ear.contacts[site/2]&~(65535u<<shift))|(weights<<shift);
}
// Only the classroom has access to the prescribed relation between an action
// and its effect. Arbitrary two-input truth tables fit in two bits.
AX_FN Word goal(Word operation,Word previous) {return (operation>>previous)&1;}
AX_FN void teach_iris(Iris &iris,Word dialect,Word seed) {
    for (Word trial=0;trial<64;++trial) {
        Word draw=(trial&3)^((mix(seed^(trial/4)))&3),pitch=draw&1,move=draw>>1;
        reinforce(iris.contacts,pitch,move,move==(pitch^dialect),iris.rule);
    }
}
AX_FN void teach_source(AttentiveEar &ear,const Iris &iris,Word form,Word operation,Word seed) {
    for (Word trial=0;trial<96;++trial) {
        Word draw=(trial&3)^(mix(seed^(trial/4))&3),prior=draw&1,pitch=draw>>1;
        Word before=response(iris.contacts,prior),after=response(iris.contacts,pitch);
        bool fed=before<2 && after<2 && after==goal(operation,before);
        lesson(ear,form,prior,pitch,fed);
    }
}
}
