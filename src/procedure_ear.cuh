#pragma once
#include "axion/procedure.hpp"

namespace axion::life::procedure {
AX_FN Word percept(Ear &e,Count pulse,RecollectionGenes g,bool learn) {
    if (!pulse) return 0;
    Word h=mix(Word(pulse)^Word(pulse>>32))&127,p=e.cache[h],d=65,next=65;
    if (!p || p>e.size || e.model[p]!=pulse) {
        for (Word i=1;i<=e.size;++i) {
            Word v=__builtin_popcountll(e.model[i]^pulse);
            if (v<d) {next=d;d=v;p=i;if (!v) break;} else if (v<next) next=v;
        }
        if (d>(g.low&15) || (d && next==d)) {
            if (!learn || e.size==letters) return 0;
            p=++e.size;e.model[p]=pulse;e.strength[p]=0;
        }
        e.cache[h]=static_cast<Byte>(p);
    }
    if (learn) {
        Count error=e.model[p]^pulse,strong=e.strength[p];
        Count flip=(g.low&16 ? error&~strong : 0ull)|(g.low&32 ? error&strong : 0ull);
        e.model[p]^=flip;Word table=(g.low>>8)&15;
        e.strength[p]=((table&1) ? ~(strong|error) : 0ull)|((table&2) ? strong&~error : 0ull)
                    |((table&4) ? ~strong&error : 0ull)|((table&8) ? strong&error : 0ull);
    }
    return p;
}
AX_FN bool retain(Memory &m,Slice x,Slice y,Word view=0,bool visible=false) {
    if (!x.n || x.n>255 || y.n>255) return false;
    Word need=x.n+y.n,drop=0,bytes=0;
    // Evict whole oldest episodes. Offsets are always relative to the
    // retained pool, including after the acoustic stream has wrapped.
    while (drop<m.count && (m.count-drop==lessons || m.bytes-bytes+need>experience_bytes)) {
        auto e=m.record[drop++];bytes=e.at+e.before+e.after;
    }
    if (drop) {
        for (Word i=bytes;i<m.bytes;++i) m.tape[i-bytes]=m.tape[i];
        for (Word i=drop;i<m.count;++i) {auto e=m.record[i];e.at-=bytes;m.record[i-drop]=e;m.retina[i-drop]=m.retina[i];m.visible[i-drop]=m.visible[i];}
        m.count-=drop;m.bytes-=bytes;
    }
    m.retina[m.count]=static_cast<Byte>(view);m.visible[m.count]=visible;
    auto &e=m.record[m.count++];e={static_cast<Half>(m.bytes),static_cast<Byte>(x.n),static_cast<Byte>(y.n)};
    for (Word j=0;j<x.n;++j) m.tape[m.bytes++]=x.p[j];
    for (Word j=0;j<y.n;++j) m.tape[m.bytes++]=y.p[j];
    ++m.observed;return true;
}
// An assimilating learner treats a changed outcome at the same observed
// stimulus as new evidence. Other viewpoints remain independent episodes.
AX_FN bool observe(Mind &m,Slice x,Slice y,Word view,bool seeing) {
    auto &b=m.memory;
    if (m.genes.expression&(1u<<30)) for (Word j=b.count;j--;) {
        if (b.visible[j]!=seeing || (seeing && b.retina[j]!=view) || !same(before(b,j),x) || same(after(b,j),y)) continue;
        auto old=b.record[j];Word size=old.before+old.after;
        for (Word i=old.at+size;i<b.bytes;++i) b.tape[i-size]=b.tape[i];
        for (Word i=j+1;i<b.count;++i) {auto r=b.record[i];r.at-=size;b.record[i-1]=r;b.retina[i-1]=b.retina[i];b.visible[i-1]=b.visible[i];}
        b.bytes-=size;--b.count;
    }
    return retain(b,x,y,view,seeing);
}
AX_FN int known(const Memory &m,Slice text) {
    for (Word i=m.count;i>0;--i) if (same(before(m,i-1),text)) return int(i-1);
    return -1;
}
AX_FN bool append(Circuit &c,const Draft &d,Word support,bool distinct=false) {
    for (Word i=0;!distinct && i<c.count;++i) if (same(cue(c,i),{d.in,d.l}) && c.cell[i].raw==d.raw && same(output(c,i),{d.out,d.r})) {
        if (support>c.cell[i].support) c.cell[i].support=static_cast<Byte>(support);
        return true;
    }
    if (!d.l || c.count==productions || c.bytes+d.l+d.r>code_bytes) return false;
    auto &r=c.cell[c.count++];r={static_cast<Half>(c.bytes),static_cast<Byte>(d.l),static_cast<Byte>(d.r),static_cast<Byte>(d.arity),
        static_cast<Byte>(d.raw),static_cast<Byte>(support),0};
    for (Word k=0;k<d.l;++k) c.wire[c.bytes++]=d.in[k];
    for (Word k=0;k<d.r;++k) c.wire[c.bytes++]=d.out[k];
    return true;
}
}
