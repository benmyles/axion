#pragma once
#include "procedure_world.cuh"

namespace axion::life::roost {
using namespace procedure;
constexpr Word practice_limit=512;
struct Wind {Byte a=0,b=1,stride[4]{},offset[4]{};};
struct Kinetics {
    MotorTrace traces[practice_limit];Word n=0,first=0,span=practice_limit,since=0,errors=0;
    Word a=0,b=0,remaining=0,challenger=0,incumbent=0;
};
// Adults use at most half the juvenile tape for recent experience. The
// other half can hold a competing two-bit contact bank in 144 bytes.
AX_FN Word provisional(const Kinetics &b,Word phase,Word echo) {
    auto e=b.traces[practice_limit/2+phase*effectors+echo];return e.retina|(Word(e.gesture)<<8)|(Word(e.echo)<<16);
}
AX_FN void provisional(Kinetics &b,Word phase,Word echo,Word weight) {
    b.traces[practice_limit/2+phase*effectors+echo]={static_cast<Byte>(weight),static_cast<Byte>(weight>>8),static_cast<Byte>(weight>>16)};
}
AX_FN void experience(Kinetics &book,MotorTrace trace,bool surprised) {
    Word at=(book.first+book.n)&(book.span-1);book.traces[at]=trace;
    if (book.n==book.span) book.first=(book.first+1)&(book.span-1);else ++book.n;
    ++book.since;book.errors=(book.errors<<1)|Word(surprised);
}
struct GaitScore {Word correct=0,training=0,decided=0,sensor_pair=0;};
struct Genome {
    RecollectionGenes learning{};Word roaming=0,voice=0,fecundity=0,founder=0,edits=0,alive=1;
};
AX_FN Word weather(Word view,Word a,Word b) {return ((view>>a)&1)|(((view>>b)&1)<<1);}
AX_FN Wind climate(Word seed) {
    Wind w;w.a=static_cast<Byte>(seed&7);w.b=static_cast<Byte>((w.a+1+((seed>>8)%7))&7);
    const Byte units[]{1,5,7,11};
    for (Word k=0;k<4;++k) {
        Word v=mix(seed+k*293u);w.stride[k]=units[v&3];w.offset[k]=static_cast<Byte>((seed+(3*k))%effectors);
    }
    return w;
}
AX_FN Word effect(Wind w,Word stimulus,Word gesture) {
    Word phase=weather(stimulus,w.a,w.b);return (w.stride[phase]*gesture+w.offset[phase])%effectors;
}
AX_FN Count proprioception(const apprenticeship::RouteLaw &law,Word step) {
    return signal(step<6 ? law.go[step] : law.back[step-6]);
}
AX_FN Word hear(MotorPolicy &m,Count echo,bool learn) {
    for (Word j=0;j<m.forms;++j) if (m.echo[j]==echo) return j;
    if (!learn || m.forms==effectors) return effectors;
    m.echo[m.forms]=echo;return m.forms++;
}
AX_FN Word drive(Word contact,Word key,Word &unique) {
    Word lo=contact&4095,hi=(contact>>12)&4095,active=lo&hi;
    if (!active) active=hi ? hi : lo ? lo : 4095;
    unique=bool(!(active&(active-1)));
    Word rotation=key%12;Word turn=((active>>rotation)|(active<<(12-rotation)))&4095;
    return (Word(__builtin_ctz(turn))+rotation)%12;
}
AX_FN Word plastic(Word contact,Word gesture,Word table) {
    Word lo=contact&4095,hi=(contact>>12)&4095,yes=1u<<gesture,one=0,two=0;
    for (Word state=0;state<4;++state) {
        Word match=(state&1 ? lo : ~lo)&(state&2 ? hi : ~hi);
        Word other=(table>>(2*state))&3,winner=(table>>(8+2*state))&3;
        if (other&1) one|=match&~yes;
        if (other&2) two|=match&~yes;
        if (winner&1) one|=match&yes;
        if (winner&2) two|=match&yes;
    }
    return (one&4095)|((two&4095)<<12);
}
AX_FN Word muscle(MotorPolicy &m,Count goal,Word view,Word key) {
    auto v=hear(m,goal,false);if (v>=effectors) return key%effectors;
    Word unique=0;return drive(m.contact[weather(view,m.first,m.second)][v],key,unique);
}
AX_FN void feel(MotorPolicy &m,RecollectionGenes g,Word view,Word gesture,Count echo) {
    auto v=hear(m,echo,true);if (v>=effectors) return;
    auto &w=m.contact[weather(view,m.first,m.second)][v];w=plastic(w,gesture,g.low>>16);++m.practice;
}
AX_FN void explore(MotorPolicy &m,Kinetics &experience,const apprenticeship::RouteLaw &law,Wind w,Word seed,Word n) {
    m={};experience.n=n;experience.first=0;experience.span=practice_limit;experience.since=experience.errors=0;
    for (Word k=0;k<n;++k) {
        Word key=mix(seed+k*0x9e3779b9u),view=key&255,port=(key>>13)%effectors;
        auto pulse=proprioception(law,effect(w,view,port));Word observed=hear(m,pulse,true);
        experience.traces[k]={static_cast<Byte>(view),static_cast<Byte>(port),static_cast<Byte>(observed)};
    }
}

#if defined(__CUDA__)
AX_FN void attend(MotorPolicy &m,Kinetics &experience,RecollectionGenes g,Word t,Word *votes,bool juvenile=true,bool propose=false) {
    Word a=0,b=0;
    if (t<28 && (g.high&32)) {
        Word v=t;for (;a<7;++a) {Word choices=7-a;if (v<choices) {b=a+1+v;break;}v-=choices;}
    }
    Word weights[4][effectors]{};Word scored=0;
    for (Word j=0;j<experience.n;++j) {
        auto e=experience.traces[(experience.first+j)&(experience.span-1)];Word z=weather(e.retina,a,b),candidate=0;
        auto &w=weights[z][e.echo];Word choice=drive(w,j*271+t,candidate);
        scored+=Word(choice==e.gesture)*(1+3*candidate);
        w=plastic(w,e.gesture,g.low>>16);
    }
    votes[t]=scored*32+31-t;asm volatile("bar.warp.sync 0xffffffff;":::"memory");
    for (Word step=16;step;step>>=1) {
        if (t<step && votes[t+step]>votes[t]) votes[t]=votes[t+step];
        asm volatile("bar.warp.sync 0xffffffff;":::"memory");
    }
    if (t==31-(votes[0]&31)) {
        if (propose) {experience.a=a;experience.b=b;experience.challenger=experience.incumbent=0;}
        else {m.first=a;m.second=b;if (juvenile) m.practice=experience.n;}
        for (Word k=0;k<4;++k) for (Word j=0;j<effectors;++j) {
            if (propose) provisional(experience,k,j,weights[k][j]);else m.contact[k][j]=weights[k][j];
        }
    }
}

#endif
AX_FN Word move(Word site,Word gesture,Word seed) {
    Word axis=gesture%6,x=site&7,y=site>>3;bool reverse=gesture>=6;
    // Each shear is a bijection; its inverse reads the same perpendicular
    // coordinate. Successive horizontal and vertical movements need not commute.
    Word delta=1+(mix(seed+axis*97+((axis&1) ? x : y))%5);
    if (reverse) delta=8-delta;
    if (axis&1) y=(y+delta)&7;else x=(x+delta)&7;
    return x+8*y;
}
AX_FN Genome mutate(Genome g,Word seed) {
    Word *fields[]{&g.learning.low,&g.learning.high,&g.learning.expression,&g.roaming,&g.voice,&g.fecundity};
    Word chance=8+((g.fecundity>>8)&63);
    for (Word j=0;j<6;++j) {
        Word k=mix(seed+(j+1)*0x9e3779b9u);if ((k&255)<chance) {*fields[j]^=1u<<((k>>9)&31);++g.edits;}
    }
    return g;
}
}
