#pragma once
#include "procedure_induce.cuh"

namespace axion::life::procedure {
struct OutcomeGrammar {Draft rule{};Byte alt[motif]{};Word alternative=0,observations=0,score=0;Guard field{};};
struct Binding {Slice x[2],y[2];};
AX_FN bool bind_guard(const Mind &m,const Draft &r,Word row,Binding &b) {
    auto x=before(m.memory,row);Slice syntax{r.in,r.l};auto shape=layout(syntax,r.arity);
    if (r.l>x.n || !at(x,0,part(syntax,0,shape.start)) || !at(x,x.n-shape.end,part(syntax,r.l-shape.end,shape.end))) return false;
    Word lo=shape.start,end=x.n-shape.end;
    for (Word split=r.arity==1 ? end : lo+1;split<=end;++split) {
        b.x[0]=part(x,lo,split-lo);
        if (r.arity==2) {
            if (split+shape.between>=end || !at(x,split,part(syntax,shape.start+1,shape.between))) continue;
            b.x[1]=part(x,split+shape.between,end-split-shape.between);
        }
        bool settled=true;for (Word k=0;k<r.arity;++k) {
            if (r.raw&(1u<<k)) b.y[k]=b.x[k];
            else {int found=known(m.memory,b.x[k]);if (found<0 || m.memory.visible[found]) {settled=false;break;}b.y[k]=after(m.memory,Word(found));}
        }
        if (settled) return true;
    }
    return false;
}
AX_FN Word abstract_action(Slice y,const Binding &b,Word arity,Word raw,Byte *out) {
    Word written=0;
    for (Word p=0;p<y.n;) {
        if (written==motif) return ~0u;
        Word chosen=arity,n=0;
        for (Word k=0;k<arity;++k) if (b.y[k].n>n && at(y,p,b.y[k])) {n=b.y[k].n;chosen=k;}
        if (n) {out[written++]=static_cast<Byte>(((raw&(1u<<chosen)) ? quote0 : call0)+chosen);p+=n;}
        else out[written++]=y.p[p++];
    }
    return written;
}
// Independent compatible two-input dendrites vote on each retinal pattern.
// A branch is compiled only where every still-compatible explanation agrees.
AX_FN bool innervate(const Mind &m,const Draft &d,OutcomeGrammar &candidate) {
    auto &book=m.memory;Binding b;Word options=~0u,used=0;
    Byte second[motif],views[lessons],allowed[lessons];Word n=0,diverse=0;Slice first[2]{};
    for (Word j=0;j<book.count;++j) if (book.visible[j] && bind_guard(m,d,j,b)) {
        auto y=after(book,j);bool primary=reproduce(y,{d.out,d.r},b.y);
        if (!primary && options==~0u) {
            options=abstract_action(y,b,d.arity,d.raw,second);if (options==~0u) return false;
        }
        bool alt=options!=~0u && reproduce(y,{second,options},b.y);
        if (!primary && !alt) return false;
        for (Word k=0;k<d.arity;++k) {if (!n) first[k]=b.x[k];else if (!same(first[k],b.x[k])) diverse|=1u<<k;}
        views[n]=book.retina[j];allowed[n++]=static_cast<Byte>(Word(primary)+2*Word(alt));
    }
    if (options==~0u || n<4 || diverse!=(1u<<d.arity)-1) return false;
    // Recompute the early ballots once the alternative is available. A
    // demonstration with interchangeable arguments cannot distinguish it.
    Word cursor=0;for (Word j=0;j<book.count;++j) if (book.visible[j] && bind_guard(m,d,j,b)) {
        if (reproduce(after(book,j),{second,options},b.y)) allowed[cursor]|=2;
        if (allowed[cursor]!=3) used|=allowed[cursor];
        ++cursor;
    }
    if (used!=3) return false;
    Word mask=0;for (Word i=0;i<d.r;++i) if (var(d.out[i])) mask|=1u<<slot(d.out[i]);
    for (Word i=0;i<options;++i) if (var(second[i])) mask|=1u<<slot(second[i]);
    if (mask!=(1u<<d.arity)-1) return false;
    Guard gate;Word possible[2][8]{};Word live=0;
    for (Word a=0;a<8;++a) for (Word c=a+1;c<8;++c) {
        Word truth[4]{3,3,3,3};
        for (Word j=0;j<n;++j) truth[((views[j]>>a)&1)|(((views[j]>>c)&1)<<1)]&=allowed[j];
        if (!(truth[0]&&truth[1]&&truth[2]&&truth[3])) continue;
        ++live;
        for (Word view=0;view<256;++view) {
            Word yes=truth[((view>>a)&1)|(((view>>c)&1)<<1)],bit=1u<<(view&31);
            if (yes&1) possible[0][view/32]|=bit;
            if (yes&2) possible[1][view/32]|=bit;
        }
    }
    if (!live) return false;
    Word resolved=0;
    for (Word k=0;k<8;++k) {
        gate.known[k]=possible[0][k]^possible[1][k];gate.second[k]=possible[1][k]&gate.known[k];resolved+=__builtin_popcount(gate.known[k]);
    }
    Word rank=n*512+resolved;
    if (!resolved || rank<=candidate.score) return false;
    candidate.rule=d;candidate.field=gate;candidate.alternative=options;candidate.observations=n;candidate.score=rank;
    for (Word k=0;k<options;++k) candidate.alt[k]=second[k];
    return true;
}
AX_FN void guard_candidate(const Mind &m,const Origins &o,Word row,OutcomeGrammar &best) {
    best={};if (!(m.genes.high&256)) return;Site inputs[2];Draft d;
    for (Word i=0;i<o.n;++i) {
        inputs[0]=o.site[i];if (inputs[0].raw) continue;
        for (Word j=0;j<=o.n;++j) {
            Word n=j==o.n ? 1 : 2;
            if (n==2) {inputs[1]=o.site[j];if (inputs[1].raw || inputs[1].begin<inputs[0].end) continue;}
            if (template_from(m,row,inputs,n,d,true)) innervate(m,d,best);
        }
    }
}
AX_FN int switch_to(const Circuit &p,Word rule,Word retina,bool observed) {
    Word index=p.cell[rule].flags>>2;if (!index) return 0;
    const auto &g=p.guards[index-1];Word bit=1u<<(retina&31),k=retina/32;
    if (!observed || !(g.known[k]&bit)) return -1;
    return (g.second[k]&bit)!=0;
}
AX_FN Slice part_action(const Circuit &p,Word id,bool second) {
    if (!second) return output(p,id);
    auto &g=p.guards[(p.cell[id].flags>>2)-1];return {g.effect,g.length};
}
}
