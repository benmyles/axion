#pragma once
#include "axion/binding.hpp"
#include "imagery.cuh"

namespace axion::life::binding {
AX_FN bool occupied(const BindingField &f) {
    return (f.contacts[0]|f.contacts[1]|f.contacts[2]|f.contacts[3])!=0;
}
AX_FN Word field(const Bindings &b,Word form,bool recruit) {
    Word start=mix(form)&3,empty=4;
    for (Word i=0;i<4;++i) {
        Word k=(i+start)&3;
        if (!occupied(b.fields[k])) {if (empty==4) empty=k;}
        else if (b.forms[k]==form) return k;
    }
    return recruit ? (empty<4 ? empty : start) : 4u;
}
AX_FN Word coordinate(Word pair,Word role) {return (pair>>(2*role))&3;}
AX_FN Word prediction(Word contacts,RecollectionGenes visual,Word bearing) {
    return engram::reinstate({visual,contacts},bearing);
}
AX_FN Word evidence(Word predicted,Word arrival) {
    return !predicted ? 0 : predicted==arrival ? 1 : !(predicted&arrival) ? 2 : 3;
}
AX_FN void reinforce(Word &scores,Word contact,Word plastic,Word cue) {
    Word old=(scores>>(2*contact))&3,next=(plastic>>(2*(old+4*cue)))&3;
    scores=(scores&~(3u<<(2*contact)))|(next<<(2*contact));
}
// A contact sees its prediction before the arrival occurs. Its two strength
// bits can change independently of the learned departure/arrival projection.
AX_FN void observe(BindingField &f,Word &scores,BindingGenes genes,RecollectionGenes visual,
                   Word before,Word after,bool credit=true) {
    for (Word output=0;output<2;++output) for (Word input=0;input<2;++input) {
        Word j=2*output+input,start=coordinate(before,input),arrival=1u<<coordinate(after,output);
        Recollection tissue{visual,f.contacts[j]};
        Word previous=engram::reinstate(tissue,start);
        if (credit) reinforce(scores,j,genes.plastic,evidence(previous,arrival));
        engram::see(tissue,start,arrival,true);
        f.contacts[j]=tissue.weights;
    }
}
AX_FN void experience(Bindings &layer,RecollectionGenes visual,Word sound,Word before,Word after,bool credit=true) {
    Word k=field(layer,sound,true),shift=8*k,scores=(layer.support>>shift)&255;
    if (layer.forms[k]!=sound) {layer.fields[k]=BindingField{};scores=0;}
    layer.forms[k]=sound;observe(layer.fields[k],scores,layer.genes,visual,before,after,credit);
    layer.support=(layer.support&~(255u<<shift))|(scores<<shift);
}
AX_FN Word select(Word scores,Word threshold,Word slack) {
    Word a=scores&3,b=(scores>>2)&3;
    return Word(a>=threshold && a+slack>=b)|(Word(b>=threshold && b+slack>=a)<<1);
}
AX_FN RoleProjection readout(const BindingField &f,Word scores,BindingGenes g,RecollectionGenes visual) {
    RoleProjection r{};r.apertures=0;r.events=1;
    for (Word o=0;o<2;++o) {
        Word active=select(scores>>(4*o),g.competition&3,(g.competition>>2)&3);
        r.apertures|=active<<(2*o);r.routes[o]=0;
        if (!active) r.missing|=1u<<(16+o);
        for (Word i=0;i<2;++i) if (active&(1u<<i)) {
            Word paths=imagery::readout(visual,f.contacts[2*o+i]);r.routes[o]|=paths<<(16*i);
            for (Word b=0;b<4;++b) if (!((paths>>(4*b))&15)) r.missing|=1u<<(8*o+4*i+b);
        }
    }
    return r;
}
AX_FN RoleProjection compose(const RoleProjection &before,const RoleProjection &after) {
    RoleProjection r{};r.apertures=0;r.events=before.events+after.events;r.missing=after.missing&0x30000;
    for (Word o=0;o<2;++o) {
        r.routes[o]=0;
        for (Word mid=0;mid<2;++mid) if (after.apertures&(1u<<(2*o+mid))) {
            Word outgoing=(after.routes[o]>>(16*mid))&65535;
            r.apertures|=((before.apertures>>(2*mid))&3)<<(2*o);
            Word from_a=imagery::compose(before.routes[mid]&65535,outgoing);
            Word from_b=imagery::compose(before.routes[mid]>>16,outgoing);
            r.routes[o]|=from_a|(from_b<<16);
            if (before.missing&(1u<<(16+mid))) r.missing|=1u<<(16+o);
            r.missing|=((before.missing>>(8*mid))&255)<<(8*o);
            Word unknown=(after.missing>>(8*o+4*mid))&15;
            if (unknown) for (Word input=0;input<2;++input) if (before.apertures&(1u<<(2*mid+input))) {
                Word paths=before.routes[mid]>>(16*input);
                for (Word pos=0;pos<4;++pos) if ((paths>>(4*pos))&unknown) r.missing|=1u<<(8*o+4*input+pos);
            }
        }
    }
    return r;
}
AX_FN void hear(RoleProjection &wave,const Bindings &b,RecollectionGenes visual,Word sound,Word unary) {
    Word k=field(b,sound,false);
    if (k<4) wave=compose(wave,readout(b.fields[k],b.support>>(8*k),b.genes,visual));
    else if (unary!=0x8421u) {
        RoleProjection move{};move.routes[0]=unary;move.routes[1]=unary<<16;
        wave=compose(wave,move);
    }
}
AX_FN Word answer(const RoleProjection &r,Word output,Word left,Word right,BindingGenes g) {
    if ((r.missing&(1u<<(16+output))) || (((r.missing>>(8*output))&255)&(left|(right<<4)))) return 0;
    Word active=(r.apertures>>(2*output))&3,result=0;
    for (Word i=0;i<2;++i) if (active&(1u<<i)) {
        Word arrival=imagery::propagate((r.routes[output]>>(16*i))&65535,i ? right : left);
        if (!arrival) return 0;
        result|=arrival;
    }
    if (result&(result-1)) return (g.competition&16) ? result&-result : 0;
    return result;
}
AX_FN Word counterevidence(const BindingField &f,RecollectionGenes visual,Word before) {
    Word pattern=0;
    for (Word o=0;o<2;++o) {
        Word a=prediction(f.contacts[2*o],visual,coordinate(before,0));
        Word b=prediction(f.contacts[2*o+1],visual,coordinate(before,1));
        if (!a || !b) pattern|=4;
        else if (a!=b) pattern|=1u<<o;
    }
    return pattern;
}
AX_FN Word try_scene(const BindingField &f,RecollectionGenes visual,BindingGenes genes,Word offered,Word random,bool indiscriminate) {
    Word chosen=random&3,best=0;
    if (!indiscriminate) for (Word k=0;k<4;++k) {
        Word i=(k+(random&3))&3,scene=(offered>>(4*i))&15;
        Word drive=(genes.exploration>>(4*counterevidence(f,visual,scene)))&15;
        if (!k || drive>best) {best=drive;chosen=i;}
    }
    return chosen;
}
}
