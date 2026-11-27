#pragma once
#include "binding.cuh"

namespace axion::life::conjunction {
struct Candidates { Word planes[4]{}; };

AX_FN void observe_output(JointField &field,RecollectionGenes vision,Word before,Word side,Word arrival) {
    Word &contacts=field.contacts[side][before>>2];
    Recollection tissue{vision,contacts};
    engram::see(tissue,before&3,arrival,true);
    contacts=tissue.weights;
}
AX_FN void experience(Bindings &b,Conjunctions *joint,RecollectionGenes vision,Word form,Word before,Word after,bool credit) {
    if (joint) {
        Word k=binding::field(b,form,true);
        auto &field=joint->fields[k];
        if (b.forms[k]!=form) field={};
        for (Word side=0;side<2;++side)
            observe_output(field,vision,before,side,1u<<binding::coordinate(after,side));
    }
    binding::experience(b,vision,form,before,after,credit);
}
AX_FN Word rows(Word m) {return ((m|(m>>1)|(m>>2)|(m>>3))&0x1111u)*15;}
AX_FN Word columns(Word m) {return ((m|(m>>4)|(m>>8)|(m>>12))&15)*0x1111u;}
AX_FN Word repeated_rows(Word m) {
    return rows(m&(((m>>1)&0x7777)|((m>>2)&0x3333)|((m>>3)&0x1111)));
}
AX_FN Word repeated_columns(Word m) {return columns(m&((m>>4)|(m>>8)|(m>>12)));}
AX_FN Word overlap(const Candidates &p) {
    return (p.planes[0]&(p.planes[1]|p.planes[2]|p.planes[3])) |
           (p.planes[1]&(p.planes[2]|p.planes[3])) | (p.planes[2]&p.planes[3]);
}
AX_FN Word occupied(const Candidates &p) {return p.planes[0]|p.planes[1]|p.planes[2]|p.planes[3];}
AX_FN Word candidates(const Candidates &p,Word scene) {
    Word result=0;
    for (Word v=0;v<4;++v) result|=((p.planes[v]>>scene)&1)<<v;
    return result;
}
// Lateral inhibition and optional unique-site excitation are synchronous.
// A contradictory wave is discarded. Acquired contacts are never modified.
AX_FN void settle(Candidates &p,Word axes,Word dynamics) {
    Candidates initial=p;
    for (Word tick=0;axes && tick<(dynamics&7);++tick) {
        Word ambiguous=overlap(p),change=0;
        if (!ambiguous) break;
        Candidates forced{},next{};
        for (Word v=0;v<4;++v) {
            Word m=p.planes[v],single=m&~ambiguous;
            Word blocked=((axes&1) ? rows(single) : 0) | ((axes&2) ? columns(single) : 0);
            m&=~(blocked&ambiguous);
            next.planes[v]=m;
            if (dynamics&8) forced.planes[v]=m&(((axes&1) ? ~repeated_rows(m) : 0) |
                                                 ((axes&2) ? ~repeated_columns(m) : 0));
            if (((axes&1) && rows(m)!=65535) || ((axes&2) && columns(m)!=65535)) {p=initial;return;}
        }
        Word forced_sites=occupied(forced);
        for (Word v=0;v<4;++v) {
            next.planes[v]&=~forced_sites|forced.planes[v];
            change|=p.planes[v]^next.planes[v];
        }
        if (overlap(forced) || occupied(next)!=65535) {p=initial;return;}
        p=next;
        if (!change) break;
    }
}
AX_FN bool readout(const JointField &field,Word side,RecollectionGenes visual,RecollectionGenes gates,Candidates &p) {
    if (!(gates.low&65535)) return false;
    p={};
    for (Word y=0;y<4;++y) {
        Word row=imagery::readout(visual,field.contacts[side][y]);
        for (Word x=0;x<4;++x) {
            Word fired=(row>>(4*x))&15;
            if (fired && !(fired&(fired-1))) p.planes[__builtin_ctz(fired)]|=1u<<(4*y+x);
        }
    }
    Word seen=occupied(p),features=0;
    if (!seen) return false;
    for (Word spikes:p.planes) {
        Word other=seen^spikes;
        features|=Word(bool(rows(spikes)&other)) | (Word(bool(columns(spikes)&other))<<1) |
                  (Word(bool(repeated_rows(spikes)))<<2) | (Word(bool(repeated_columns(spikes)))<<3);
    }
    Word minimum=(gates.expression>>((features&12) ? 8 : 4))&15;
    if (!(gates.low&(1u<<features)) || Word(__builtin_popcount(seen))<minimum) return false;
    for (Word &plane:p.planes) plane|=65535^seen;
    settle(p,(gates.high>>(2*features))&3,gates.expression);
    return true;
}
AX_FN Word pairs(Word left,Word right) {
    Word out=0;
    for (Word b=0;b<4;++b) if (right&(1u<<b)) out|=left<<(4*b);
    return out;
}
AX_FN Word marginal(Word set,Word side) {
    if (!side) return (set|(set>>4)|(set>>8)|(set>>12))&15;
    Word bits=rows(set)&0x1111u;
    return (bits*0x249u>>9)&15;
}
AX_FN SceneWave transition(const RoleProjection &roles,const BindingGenes &g,const Candidates *p,Word active) {
    SceneWave result{};result.active=1;
    for (Word scene=0;scene<16;++scene) {
        Word out[2];
        for (Word side=0;side<2;++side) {
            Word m=(active&(1u<<side)) ? candidates(p[side],scene) :
                binding::answer(roles,side,1u<<(scene&3),1u<<(scene>>2),g);
            out[side]=m ? m : 15;
        }
        result.scenes[scene]=static_cast<unsigned short>(pairs(out[0],out[1]));
    }
    return result;
}
AX_FN void compose(SceneWave &wave,const SceneWave &next) {
    for (auto &scene:wave.scenes) {
        Word possible=scene,out=0;
        while (possible) {out|=next.scenes[__builtin_ctz(possible)];possible&=possible-1;}
        scene=static_cast<unsigned short>(out);
    }
}
AX_FN void hear(SceneWave &wave,const RoleProjection &prefix,const Bindings &b,const Conjunctions &joint,
                RecollectionGenes vision,Word form,Word unary) {
    Word k=binding::field(b,form,false),active=0;
    RoleProjection roles{};Candidates p[2]{};
    if (k<4) {
        for (Word o=0;o<2;++o)
            active|=Word(readout(joint.fields[k],o,vision,joint.gates,p[o]))<<o;
        if (!active && !wave.active) return;
        roles=binding::readout(b.fields[k],b.support>>(8*k),b.genes,vision);
    } else {
        if (!wave.active || unary==0x8421u) return;
        roles.routes[0]=unary;roles.routes[1]=unary<<16;
    }
    auto next=transition(roles,b.genes,p,active);
    if (!wave.active) {
        if (!prefix.events && prefix.routes[0]==0x8421u && prefix.routes[1]==0x84210000u) {wave=next;return;}
        wave=transition(prefix,b.genes,nullptr,0);
    }
    compose(wave,next);
}
AX_FN Word apertures(const SceneWave &w,Word side) {
    Word needed=0;
    for (Word i=0;i<16 && needed!=3;++i) {
        Word m=marginal(w.scenes[i],side);
        if (m!=marginal(w.scenes[i&12],side)) needed|=1;
        if (m!=marginal(w.scenes[i&3],side)) needed|=2;
    }
    return needed;
}
AX_FN Word answer(const SceneWave &w,Word side,Word left,Word right) {
    Word possible=pairs(left ? left : 15,right ? right : 15),arrivals=0;
    while (possible) {
        arrivals|=marginal(w.scenes[__builtin_ctz(possible)],side);
        if (arrivals&(arrivals-1)) return 0;
        possible&=possible-1;
    }
    return arrivals;
}
}
