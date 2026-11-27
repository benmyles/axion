#pragma once
#include "composition_fit.cuh"

namespace axion::life::composition {
AX_FN bool same_fibre(const Fibre &x,const Fibre &y) {
    return x.source==y.source && x.other==y.other && x.first==y.first && x.second==y.second && x.repeat==y.repeat &&
           x.gate==y.gate && x.gate_source==y.gate_source && x.condition==y.condition && x.literal==y.literal;
}
AX_FN void nominate(Word *rank,Word score) {
    for (Word k=0;k<alternatives-1;++k) {
        Word old;
        asm volatile("{ .reg .u64 s; .reg .u32 p; cvta.to.shared.u64 s,%1; cvt.u32.u64 p,s; atom.shared.min.u32 %0,[p],%2; }":"=r"(old):"l"(rank+k),"r"(score):"memory");
        if (old==~0u || old==score) break;
        if (old>score) score=old;
    }
}
AX_FN Fibre rival(Word code,const Fibre &prior) {
    Fibre f;f.source=code&7;f.other=(code>>3)&15;if (f.other==8) f.other=absent;
    f.first=(code>>7)&63;f.second=(code>>13)&63;f.repeat=(code>>19)&1;
    if (f.first) {f.gate=prior.gate;f.gate_source=prior.gate_source;f.condition=prior.condition;}
    return f;
}
AX_FN void compete(const Mind &m,const Detections &d,Word group,Word channel,Rivals &r,Evidence &w,Word *rank,Word lane) {
    auto &q=m.anatomy.phrases[group];auto primary=q.voice[channel];
    if (!lane) {r.n=1;r.alive=1;r.f[0]=primary;}
    if (primary.literal || !q.ready) return;
    if (!lane) precedents(m,d,q,channel,w);
    if (lane<alternatives-1) rank[lane]=~0u;
    together();
    auto &p=m.anatomy;Word fields=p.fields_n+1;
    for (Word idx=lane;idx<q.cue.prototype.size*fields*fields*2;idx+=128) {
        Word index=idx,pin=index%q.cue.prototype.size;index/=q.cue.prototype.size;
        if (q.cue.fixed&(1u<<pin)) continue;
        Word first=index%fields;index/=fields;Word second=index%fields,repeat=index/fields;
        if ((!first && (second || repeat || !(m.genes.high&64))) || (first && !(m.genes.high&1)) ||
            (second && !(m.genes.high&2)) || (repeat && !(m.genes.high&4))) continue;
        bool both=(first && p.fields[first-1].right!=absent) || (second && p.fields[second-1].right!=absent);
        Word cost=Word(first!=0)+2*Word(second!=0)+repeat+Word(both);
        for (Word other=both ? 0 : width;other<=(both ? q.cue.prototype.size-1u : width);++other) {
            if (both && (other==pin || (q.cue.fixed&(1u<<other)))) continue;
            Word code=(cost<<20)|(repeat<<19)|(second<<13)|(first<<7)|(other<<3)|pin;
            auto f=rival(code,primary);
            if (!same_fibre(f,primary) && explains(p,d,f,w)) nominate(rank,code);
        }
    }
    together();
    if (!lane) {
        for (Word j=0;j<alternatives-1 && rank[j]!=~0u;++j) r.f[r.n++]=rival(rank[j],primary);
        r.alive=(1u<<r.n)-1;
    }
}
struct InquiryPort {
    Mind &m;Competition &r;Detections &index;Tuition &fees;
    Word budget,random,time=0,policy=0,opportunities=0;Phrase prior{},spoken{};
    AX_FN void fact(const Phrase &x) {
        Word type=stimulus(m.anatomy,x);
        for (Word j=0;j<m.anatomy.fields_n;++j) {
            auto &f=m.anatomy.fields[j];if (f.type!=type) continue;
            Word a=x.words[f.left],b=f.right==absent ? 0 : x.words[f.right],value=x.words[f.out];
            auto old=Replay{index}.get(j+1,a,b,time);Word cue=Word(old.value!=0)+2*Word(old.value==value);
            if (!((m.genes.low>>cue)&1) || old.value==value || index.n>=episodes*projections) continue;
            Word key=address(j+1,a,b),h=bucket(key)&511;
            index.records[index.n]={key|(value<<24),static_cast<unsigned short>(time),index.tail[h]};
            index.tail[h]=static_cast<unsigned short>(++index.n);
        }
    }
    AX_FN void hear(const Mouth &stimulus,const Mouth &response) {
        ++time;auto x=perceive(m.lexicon,stimulus,m.genes,true);Word t=fire(m.anatomy,x);
        if (t==queries || !m.anatomy.phrases[t].ready) {fact(x);return;}
        auto &q=m.anatomy.phrases[t];Context c{x,prior,spoken};Word predictions[width][alternatives]{};
        bool disagree=false;Phrase answer;answer.size=q.length;
        for (Word k=0;k<q.length;++k) {
            auto &set=r.voice[t][k];Word winner=__builtin_ctz(set.alive);
            for (Word j=0;j<set.n;++j) if (set.alive&(1u<<j)) {
                Word depth=0;predictions[k][j]=project(m.anatomy,Replay{index},set.f[j],c,time,depth);
                if (j==winner) answer.words[k]=predictions[k][j];
                else disagree|=predictions[k][j]!=answer.words[k];
            }
        }
        ++fees.heard;fees.disagreements+=disagree;random=mix(random+0x9e3779b9u);
        // The decision to listen is made entirely from the surviving neural
        // predictions. The unrequested caregiver wave never reaches the ear.
        Word remaining=opportunities>=fees.heard ? opportunities-fees.heard+1 : 1;
        bool request=policy==2 ? random%remaining<budget-fees.asked : policy==1 ? !(random&7) : disagree;
        if (fees.asked<budget && request) {
            ++fees.asked;
            auto y=perceive(m.lexicon,response,m.genes,true);
            if (y.size==q.length) for (Word k=0;k<q.length;++k) {
                auto &set=r.voice[t][k];Word keep=0;
                for (Word j=0;j<set.n;++j) if ((set.alive&(1u<<j)) && predictions[k][j]==y.words[k]) keep|=1u<<j;
                if (keep) {fees.removed+=__builtin_popcount(set.alive^keep);set.alive=keep;answer.words[k]=y.words[k];}
                else ++fees.unexplained;
            }
        }
        bool complete=true;for (Word k=0;k<answer.size;++k) complete&=answer.words[k]!=0;
        if (complete) {prior=x;spoken=answer;}
    }
};
}
