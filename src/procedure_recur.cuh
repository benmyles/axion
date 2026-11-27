#pragma once
#include "procedure_fold.cuh"
#include "procedure_read.cuh"

namespace axion::life::procedure {
AX_FN Summary fragment(const Ear &ear,Slice x) {
    Summary s;for (Word k=0;k<x.n;++k) discharge(s,ear.model[x.p[k]]);return s;
}
AX_FN void piece(const Ear &ear,Slice x,Count offset,Count from,Word cap,Count *events) {
    if (!cap || offset+x.n<=from || (offset>=from && offset-from>=cap)) return;
    for (Word k=0;k<x.n;++k) if (offset+k>=from && offset+k-from<cap) events[Word(offset+k-from)]=ear.model[x.p[k]];
}
AX_FN bool domain(const Recursion &p,Slice x) {
    if (!matching(p,x)) return false;
    auto body=part(x,p.head,x.n-p.head-p.tail);
    for (Word pos=0;pos<body.n;) {Word j=next_arm(p,body,pos);if (j==p.n) return false;pos+=p.arm[j].n;}
    return true;
}
AX_FN Report recurrence(const Ear &ear,const Recursion &p,Slice x,Count from,Word wanted,Count *events,
                         Word alternative=~0u,Word cut=0,bool conjecture=false) {
    Report out;if (!matching(p,x)) return out;
    auto body=part(x,p.head,x.n-p.head-p.tail);Summary left,right;bool disputed=false;
    for (Word pos=0;pos<body.n;) {
        Word j=next_arm(p,body,pos);if (j==p.n) return {};
        auto a=p.arm[j];Word split=alternative==j ? cut : Word(__builtin_ctz(a.cuts));disputed|=bool(a.cuts&(a.cuts-1));
        auto base=p.wire+a.at+a.n;
        combine(left,fragment(ear,{base,split}));
        auto delayed=fragment(ear,{base+split+p.center,a.m-split-p.center});combine(delayed,right);right=delayed;
        pos+=a.n;++out.nodes;
    }
    if (disputed && !conjecture) {out.status=ambiguous;return out;}
    out.voice=left;combine(out.voice,fragment(ear,{p.base,p.center}));combine(out.voice,right);
    out.status=recognized;out.depth=out.nodes;
    if (wanted && from<out.voice.length) {
        Count begin=0,end=out.voice.length;piece(ear,{p.base,p.center},left.length,from,wanted,events);
        for (Word pos=0;pos<body.n;) {
            Word j=next_arm(p,body,pos);auto a=p.arm[j];Word split=j==alternative ? cut : Word(__builtin_ctz(a.cuts));
            auto base=p.wire+a.at+a.n;Word later=a.m-split-p.center;
            piece(ear,{base,split},begin,from,wanted,events);end-=later;
            piece(ear,{base+split+p.center,later},end,from,wanted,events);
            begin+=split;pos+=a.n;
        }
        out.spoken=Word(out.voice.length-from<wanted ? out.voice.length-from : wanted);out.first=events[0];
    }
    return out;
}
struct Inquiry {Byte signal[96]{};Word length=0,scope=0,hypotheses=0;};
AX_FN Inquiry question(const Mind &m,Word seed,bool random,bool paid=false) {
    Inquiry winner;Word best=0;
    for (Word v=0;v<m.recursion.n;++v) {
        auto &p=m.recursion.state[v];Word unresolved=0;
        for (Word k=0;k<p.n;++k) unresolved+=Word(__builtin_popcount(p.arm[k].cuts))-1;
        if (!unresolved && !paid) continue;
        for (Word a=0;a<p.n;++a) for (Word b=0;b<p.n;++b) {
            auto x=token(p,a),y=token(p,b);Inquiry trial;trial.scope=v;
            if (p.head+p.tail+x.n+y.n>sizeof(trial.signal)) continue;
            for (Word k=0;k<p.head;++k) trial.signal[trial.length++]=p.prefix[k];
            for (Word k=0;k<x.n;++k) trial.signal[trial.length++]=x.p[k];
            for (Word k=0;k<y.n;++k) trial.signal[trial.length++]=y.p[k];
            for (Word k=0;k<p.tail;++k) trial.signal[trial.length++]=p.suffix[k];
            Slice text{trial.signal,trial.length};if (known(m.memory,text)>=0) continue;
            auto prediction=recurrence(m.ear,p,text,0,0,nullptr,~0u,0,true);Word difference=0;
            for (Word j=0;j<p.n;++j) if (j==a || j==b) for (Word mask=p.arm[j].cuts&(p.arm[j].cuts-1);mask;mask&=mask-1) {
                auto other=recurrence(m.ear,p,text,0,0,nullptr,j,Word(__builtin_ctz(mask)),true);
                difference+=other.voice.hash!=prediction.voice.hash || other.voice.length!=prediction.voice.length;
            }
            Word key=mix(seed^a*971^b*5137^v*14533)&255;
            Word score=random ? 1+key : difference ? 256*difference+key : 0;
            if (score>best) {best=score;trial.hypotheses=difference;winner=trial;}
        }
    }
    return winner;
}
}
