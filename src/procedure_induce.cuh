#pragma once
#include "procedure_ear.cuh"

namespace axion::life::procedure {
AX_FN void mark(Origins &o,Word start,Word end,Word source,bool raw) {
    if (end<=start || o.n==positions) return;
    for (Word i=0;i<o.n;++i) if (o.site[i].begin==start && o.site[i].end==end && o.site[i].raw==raw) return;
    o.site[o.n++]={static_cast<Byte>(start),static_cast<Byte>(end),static_cast<Byte>(source),static_cast<Byte>(raw)};
}
AX_FN void boundaries(const Mind &m,Word row,Origins &o) {
    o.n=0;auto &book=m.memory;auto x=before(book,row);
    if (m.genes.high&1) for (Word j=0;j<book.count;++j) {
        auto child=before(book,j);if (book.visible[j] || child.n>=x.n || !after(book,j).n) continue;
        for (Word k=0;k+child.n<=x.n;++k) if (at(x,k,child)) mark(o,k,k+child.n,j,false);
    }
    if (!(m.genes.high&2)) return;
    // A minimal pair identifies a candidate variable span anywhere in the
    // acoustic stream, even if the filler has never been a complete turn.
    for (Word j=0;j<book.count;++j) if (j!=row) {
        auto y=before(book,j);Word left=0,right=0;
        while (left<x.n && left<y.n && x.p[left]==y.p[left]) ++left;
        while (right+left<x.n && right+left<y.n && x.p[x.n-1-right]==y.p[y.n-1-right]) ++right;
        if (left+right && left+right<x.n && left+right<y.n) mark(o,left,x.n-right,0,true);
    }
}
AX_FN bool template_from(const Mind &m,Word row,const Site *s,Word n,Draft &draft,bool conditional=false) {
    auto &book=m.memory;auto x=before(book,row),y=after(book,row);
    Word cursor=0;Slice heard[2],result[2];draft.l=draft.r=draft.raw=0;draft.arity=n;
    for (Word k=0;k<n;++k) {
        if (s[k].begin<cursor || draft.l+s[k].begin-cursor+1>motif) return false;
        for (;cursor<s[k].begin;++cursor) draft.in[draft.l++]=x.p[cursor];
        draft.in[draft.l++]=static_cast<Byte>(call0+k);heard[k]=part(x,s[k].begin,s[k].end-s[k].begin);
        result[k]=s[k].raw ? heard[k] : after(book,s[k].source);
        if (s[k].raw) draft.raw|=1u<<k;
        cursor=s[k].end;
    }
    if (draft.l+x.n-cursor>motif) return false;
    for (;cursor<x.n;++cursor) draft.in[draft.l++]=x.p[cursor];
    if (draft.l==n) return false;
    Word used=0;
    for (Word i=0;i<y.n;) {
        if (draft.r==motif) return false;
        Word which=n,len=0;
        for (Word k=0;k<n;++k) if (result[k].n>len && at(y,i,result[k])) {which=k;len=result[k].n;}
        if (which==n) draft.out[draft.r++]=y.p[i++];
        else {draft.out[draft.r++]=static_cast<Byte>((s[which].raw ? quote0 : call0)+which);i+=len;used|=1u<<which;}
    }
    return conditional ? used!=0 : used==((1u<<n)-1);
}
struct Layout {Word start=0,between=0,end=0;};
AX_FN Layout layout(Slice in,Word arity) {
    Layout l;while (l.start<in.n && in.p[l.start]!=call0) ++l.start;
    if (arity==2) {while (l.start+1+l.between<in.n && in.p[l.start+1+l.between]!=call1) ++l.between;}
    l.end=in.n-l.start-l.between-arity;return l;
}
AX_FN bool reproduce(Slice target,Slice instructions,const Slice *arguments) {
    Word pos=0;
    for (Word j=0;j<instructions.n;++j) {
        Byte op=instructions.p[j];
        if (var(op)) {auto x=arguments[slot(op)];if (!at(target,pos,x)) return false;pos+=x.n;}
        else if (pos>=target.n || target.p[pos++]!=op) return false;
    }
    return pos==target.n;
}
AX_FN Verdict evidence(const Mind &m,const Draft &d,int only=-1) {
    auto &book=m.memory;Slice in{d.in,d.l};auto l=layout(in,d.arity);
    Slice first[2]{};Word varied=0,support=0,savings=0,conflicts=0;
    for (Word row=only<0 ? 0 : Word(only);row<(only<0 ? book.count : Word(only+1));++row) {
        if (book.visible[row]) continue;
        auto x=before(book,row),y=after(book,row);
        if (x.n<d.l || !at(x,0,part(in,0,l.start)) || !at(x,x.n-l.end,part(in,d.l-l.end,l.end))) continue;
        Word high=x.n-l.end,first_end=d.arity==1 ? high : l.start+1;
        bool inspectable=false,found=false;
        for (Word cut=first_end;cut<=high && !found;++cut) {
            Slice args[2]{part(x,l.start,cut-l.start),{}},value[2]{};
            if (d.arity==2) {
                if (cut+l.between>=high || !at(x,cut,part(in,l.start+1,l.between))) continue;
                args[1]=part(x,cut+l.between,high-cut-l.between);
            }
            bool defined=true;
            for (Word k=0;k<d.arity;++k) {
                if (d.raw&(1u<<k)) value[k]=args[k];
                else {
                    int j=known(book,args[k]);if (j<0) {defined=false;break;}value[k]=after(book,Word(j));
                }
            }
            if (!defined) continue;
            inspectable=true;
            if (!reproduce(y,{d.out,d.r},value)) continue;
            found=true;
            for (Word k=0;k<d.arity;++k) {
                if (!support) first[k]=args[k];
                else if (!same(first[k],args[k])) varied|=1u<<k;
            }
            ++support;savings+=x.n+y.n;
        }
        if (inspectable && !found) ++conflicts;
    }
    if (only>=0) return {support,support};
    if (!support) return {};
    Word cost=d.l+d.r+2*d.arity+Word(__builtin_popcount(d.raw))*(1+(m.genes.expression&3));
    Word stimulus=Word(support>=2+((m.genes.expression>>9)&1))+2*Word(varied==((1u<<d.arity)-1))+4*Word(conflicts!=0)+8*Word(savings>cost);
    if (!((m.genes.high>>(16+stimulus))&1)) return {};
    cost+=conflicts*(1+((m.genes.expression>>4)&15));return {support,savings>cost ? 1+savings-cost : 0};
}
AX_FN void proposal(const Mind &m,const Origins &o,Word row,Draft &best,Verdict &rank) {
    rank={};Site pair[2];Draft hypothesis;
    for (Word i=0;i<o.n;++i) {
        pair[0]=o.site[i];
        for (Word j=0;j<=o.n;++j) {
            Word n=j==o.n ? 1 : 2;
            if (n==2) {
                if (!(m.genes.high&4) || o.site[j].begin<pair[0].end) continue;
                pair[1]=o.site[j];
            }
            if (!template_from(m,row,pair,n,hypothesis)) continue;
            auto v=evidence(m,hypothesis);
            if (v.score>rank.score) {best=hypothesis;rank=v;}
        }
    }
}
}
