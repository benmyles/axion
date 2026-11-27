#pragma once
#include "procedure_ear.cuh"

namespace axion::life::procedure {
constexpr Word paths=12,unit_limit=16,action_limit=24;
struct RouteUnit {Byte input[unit_limit]{},action[action_limit]{};Byte l=0,r=0;Word cuts=0;};
struct Induction {
    Byte prefix[motif]{},suffix[motif]{},base[action_limit]{};
    Byte head=0,tail=0,center=0,units=0;RouteUnit terms[paths]{};
    Word observations=0,compounds=0,tests=0,unresolved=0;
};
AX_FN bool addressed(Slice x,Slice prefix,Slice suffix) {
    return x.n>=prefix.n+suffix.n && at(x,0,prefix) && at(x,x.n-suffix.n,suffix);
}
AX_FN Word scan_units(const Induction &p,Slice x,Byte *order) {
    Word pos=0,n=0;
    while (pos<x.n) {
        Word j=0;
        for (;j<p.units;++j) {const auto &u=p.terms[j];if (at(x,pos,{u.input,u.l})) break;}
        if (j==p.units) return 0;
        if (order) order[n]=static_cast<Byte>(j);
        pos+=p.terms[j].l;++n;
    }
    return n;
}
AX_FN bool result_is(const Induction &p,const Byte *order,Word n,Slice actual) {
    Word at=0;
    for (Word i=0;i<n;++i) {
        const auto &u=p.terms[order[i]];Word split=__builtin_ctz(u.cuts);
        for (Word k=0;k<split;++k) if (at>=actual.n || actual.p[at++]!=u.action[k]) return false;
    }
    for (Word k=0;k<p.center;++k) if (at>=actual.n || actual.p[at++]!=p.base[k]) return false;
    for (Word i=n;i>0;--i) {
        const auto &u=p.terms[order[i-1]];Word split=__builtin_ctz(u.cuts);
        for (Word k=split+p.center;k<u.r;++k) if (at>=actual.n || actual.p[at++]!=u.action[k]) return false;
    }
    return at==actual.n;
}
// The empty instance supplies the two audible boundaries and a center motor
// sequence. Increasingly long instances reveal candidate transition labels.
// No action has a designated inverse, order of return, or distinguished cue.
AX_FN bool induce_loop(const Mind &m,Word row,Word split,Induction &p) {
    const auto &book=m.memory;auto tag=before(book,row),base=after(book,row);
    if (!tag.n || tag.n>motif || base.n>action_limit) return false;
    Slice front=part(tag,0,split),back=part(tag,split,tag.n-split);
    Byte matches[lessons],order[255];Word count=0;
    for (Word j=0;j<book.count;++j) if (!book.visible[j] && addressed(before(book,j),front,back)) matches[count++]=static_cast<Byte>(j);
    if (count<5) return false;
    // Stable insertion order by payload length lets known shorter signals
    // explain longer observations before any new transition is recruited.
    for (Word k=1;k<count;++k) {auto value=matches[k];Word t=k;
        while (t && book.record[matches[t-1]].before>book.record[value].before) {matches[t]=matches[t-1];--t;}matches[t]=value;}
    p={};p.head=static_cast<Byte>(front.n);p.tail=static_cast<Byte>(back.n);p.center=static_cast<Byte>(base.n);
    for (Word k=0;k<front.n;++k) p.prefix[k]=front.p[k];
    for (Word k=0;k<back.n;++k) p.suffix[k]=back.p[k];
    for (Word k=0;k<base.n;++k) p.base[k]=base.p[k];
    for (Word z=0;z<count;++z) {
        Word j=matches[z];auto heard=before(book,j),y=after(book,j);auto x=part(heard,split,heard.n-tag.n);
        if (!x.n) {if (!same(y,base)) return false;continue;}
        Word n=scan_units(p,x,order);if (n) {p.compounds+=n>1;continue;}
        if (p.units==paths || x.n>unit_limit || y.n>action_limit) return false;
        for (Word k=0;k<p.units;++k) {
            Slice u{p.terms[k].input,p.terms[k].l};
            if (at(x,0,u) || at(u,0,x)) return false;
        }
        auto &u=p.terms[p.units++];u.l=static_cast<Byte>(x.n);u.r=static_cast<Byte>(y.n);
        for (Word k=0;k<x.n;++k) u.input[k]=x.p[k];
        for (Word k=0;k<y.n;++k) u.action[k]=y.p[k];
        for (Word k=0;k+base.n<=y.n;++k) if (at(y,k,base)) u.cuts|=1u<<k;
        if (!u.cuts) return false;
    }
    if (p.units<2 || p.compounds<2) return false;
    for (Word z=0;z<count;++z) {
        Word j=matches[z];auto heard=before(book,j),y=after(book,j),x=part(heard,split,heard.n-tag.n);
        if (!x.n) continue;
        Word n=scan_units(p,x,order),front_pos=0,end_pos=y.n;
        // The combined length of both motor fragments is fixed before their
        // split is known. Peel every resolved outside contact and use the
        // remaining witnessed pulse train as evidence for the next one.
        for (Word k=0;k<n;++k) {
            auto &u=p.terms[order[k]];
            if (Word(u.r-p.center)>end_pos-front_pos) return false;
            Slice rest=part(x,0,0);Word consumed=0;
            for (Word s=0;s<=k;++s) consumed+=p.terms[order[s]].l;
            rest=part(x,consumed,x.n-consumed);
            Word inner=~0u;
            for (Word b=0;b<count;++b) {
                Word e=matches[b];auto h=before(book,e);
                if (same(part(h,split,h.n-tag.n),rest)) {inner=e;break;}
            }
            Word allowed=0;
            for (Word candidates=u.cuts;candidates;candidates&=candidates-1) {
                Word cut=__builtin_ctz(candidates),r=u.r-p.center-cut;
                if (!at(y,front_pos,{u.action,cut}) || !at(y,end_pos-r,{u.action+cut+p.center,r})) continue;
                if (inner!=~0u && !same(part(y,front_pos+cut,end_pos-front_pos-cut-r),after(book,inner))) continue;
                allowed|=1u<<cut;
            }
            if (!allowed) return false;
            u.cuts=allowed;
            if (allowed&(allowed-1)) break;
            Word cut=__builtin_ctz(allowed);front_pos+=cut;end_pos-=u.r-p.center-cut;
        }
    }
    for (Word k=0;k<p.units;++k) p.unresolved+=Word(__builtin_popcount(p.terms[k].cuts))-1;
    if (!p.unresolved) for (Word j=0;j<count;++j) {
        Word row=matches[j];auto h=before(book,row),x=part(h,split,h.n-tag.n);Word n=scan_units(p,x,order);
        if (!result_is(p,order,n,after(book,row))) return false;
    }
    p.observations=count;return true;
}
AX_FN bool save_loop(Recursions &bank,const Induction &p) {
    Word index=0;
    for (;index<bank.n;++index) {
        auto &r=bank.state[index];if (r.head==p.head && r.tail==p.tail && same({r.prefix,r.head},{p.prefix,p.head}) && same({r.suffix,r.tail},{p.suffix,p.tail})) break;
    }
    if (index==loops) return false;
    if (index==bank.n) ++bank.n;
    auto &r=bank.state[index];r={};r.head=p.head;r.tail=p.tail;r.center=p.center;r.n=p.units;r.support=static_cast<Byte>(p.observations);
    for (Word k=0;k<p.head;++k) r.prefix[k]=p.prefix[k];
    for (Word k=0;k<p.tail;++k) r.suffix[k]=p.suffix[k];
    for (Word k=0;k<p.center;++k) r.base[k]=p.base[k];
    for (Word j=0;j<p.units;++j) {
        auto &u=p.terms[j];auto &a=r.arm[j];a.at=static_cast<Half>(r.bytes);a.n=u.l;a.m=u.r;a.cuts=u.cuts;
        if (Word(r.bytes)+u.l+u.r>loop_bytes) return false;
        for (Word k=0;k<u.l;++k) r.wire[r.bytes++]=u.input[k];
        for (Word k=0;k<u.r;++k) r.wire[r.bytes++]=u.action[k];
    }
    return true;
}
AX_FN bool matching(const Recursion &p,Slice x) {return addressed(x,{p.prefix,p.head},{p.suffix,p.tail});}
AX_FN Slice token(const Recursion &p,Word j) {auto a=p.arm[j];return {p.wire+a.at,a.n};}
AX_FN Word next_arm(const Recursion &p,Slice x,Word pos) {
    for (Word j=0;j<p.n;++j) if (at(x,pos,token(p,j))) return j;
    return p.n;
}
}
