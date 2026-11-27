#pragma once
#include "composition_merge.cuh"

namespace axion::life::composition {
AX_FN Word component() {Word x;asm("mov.u32 %0, %%tid.x;":"=r"(x));return x;}
AX_FN Word block_id() {Word x;asm("mov.u32 %0, %%ctaid.x;":"=r"(x));return x;}
AX_FN void together() {asm volatile("bar.sync 0;":::"memory");}
AX_FN void shrink(Word &target,Word value) {
    asm volatile("{ .reg .u64 s; .reg .u32 p; cvta.to.shared.u64 s,%0; cvt.u32.u64 p,s; red.shared.min.u32 [p],%1; }"::"l"(&target),"r"(value):"memory");
}
AX_FN Word size_class(const Episode &e) {return e.response.size;}
struct Induction {Byte assigned[episodes];Word best[128],owner[128];};
AX_FN Pattern overlap(const Experience &book,const Induction &s,Word row,Word pin,Word &support) {
    const auto &e=book.event[row];Pattern p{e.stimulus,static_cast<Byte>((1u<<e.stimulus.size)-1)};support=0;
    for (Word i=0;i<book.n;++i) {
        const auto &sample=book.event[i];const auto &x=sample.stimulus;
        if (s.assigned[i] || x.size!=p.prototype.size || size_class(sample)!=size_class(e) ||
            x.words[pin]!=p.prototype.words[pin]) continue;
        ++support;
        for (Word k=0;k<x.size;++k) if (x.words[k]!=p.prototype.words[k]) p.fixed&=~(1u<<k);
    }
    return p;
}
AX_FN void vocabulary(Mind &m,Induction &s,Word lane,bool answering) {
    auto &p=m.anatomy;auto &book=m.lessons;
    // A row is eligible only for the side of the acoustic exchange being fitted.
    for (Word i=lane;i<book.n;i+=128) s.assigned[i]=(book.event[i].response.size!=0)!=answering;
    together();
    for (Word type=0;type<(answering ? queries : patterns);++type) {
        Word best=0,owner=0;
        for (Word row=lane;row<book.n;row+=128) {
            if (s.assigned[row]) continue;
            const auto &e=book.event[row];
            for (Word pin=0;pin<e.stimulus.size;++pin) {
                bool earlier=false;
                for (Word i=0;i<row && !earlier;++i) {
                    const auto &x=book.event[i];earlier=!s.assigned[i] && x.stimulus.size==e.stimulus.size &&
                        size_class(x)==size_class(e) && x.stimulus.words[pin]==e.stimulus.words[pin];
                }
                if (earlier) continue;
                Word n=0;auto candidate=overlap(book,s,row,pin,n);
                if (!candidate.fixed || n<2+((m.genes.expression>>12)&1)) continue;
                Word fixed=__builtin_popcount(candidate.fixed);
                if (!answering && fixed==candidate.prototype.size) continue;
                Word score=n*(3+fixed);
                if (score>best) {best=score;owner=row*width+pin;}
            }
        }
        s.best[lane]=best;s.owner[lane]=owner;together();
        for (Word size=64;size;size>>=1) {
            if (lane<size && (s.best[lane+size]>s.best[lane] || (s.best[lane+size]==s.best[lane] && s.owner[lane+size]<s.owner[lane]))) {
                s.best[lane]=s.best[lane+size];s.owner[lane]=s.owner[lane+size];
            }
            together();
        }
        if (!s.best[0]) break;
        if (!lane) {
            Word n=0,row=s.owner[0]/width;auto group=overlap(book,s,row,s.owner[0]%width,n);
            if (answering) {
                auto &q=p.phrases[p.phrases_n++];q.cue=group;q.length=book.event[row].response.size;q.support=static_cast<Byte>(n);
            } else p.facts[p.facts_n++]=group;
        }
        together();
        const auto &group=answering ? p.phrases[type].cue : p.facts[type];
        for (Word i=lane;i<book.n;i+=128) if (!s.assigned[i] && matches(group,book.event[i].stimulus) &&
            (!answering || book.event[i].response.size==p.phrases[type].length)) s.assigned[i]=1;
        together();
    }
}
AX_FN void afferents(Mind &m,Detections &d) {
    auto &p=m.anatomy;
    for (Word joint=0;joint<2;++joint) for (Word i=0;i<p.facts_n;++i) {
        auto &fact=p.facts[i];Word n=fact.prototype.size;
        for (Word l=0;l<n;++l) for (Word r=joint ? l+1 : n;r<=n;++r) {
            if (joint && (r==n || !(m.genes.high&16))) break;
            for (Word y=0;y<n;++y) {
                if (l==y || r==y || ((fact.fixed>>l)&1) || ((fact.fixed>>y)&1) || (joint && ((fact.fixed>>r)&1))) continue;
                if (p.fields_n==projections) continue;
                p.fields[p.fields_n++]={static_cast<Byte>(i),static_cast<Byte>(l),joint ? static_cast<Byte>(r) : absent,static_cast<Byte>(y)};
            }
        }
    }
    Word previous=0;
    for (Word i=0;i<m.lessons.n;++i) {
        const auto &e=m.lessons.event[i];d.last[i]=static_cast<Byte>(previous);
        if (e.response.size) {previous=i+1;continue;}
        Word type=stimulus(p,e.stimulus);
        for (Word j=0;j<p.fields_n;++j) {
            auto &f=p.fields[j];if (f.type!=type) continue;
            const auto *x=e.stimulus.words;Word a=x[f.left],b=f.right==absent ? 0 : x[f.right];
            auto old=Replay{d}.get(j+1,a,b,i);Word value=x[f.out];
            Word cue=Word(old.value!=0)+2*Word(old.value==value);
            if (!((m.genes.low>>cue)&1) || old.value==value) continue;
            Word key=address(j+1,a,b),h=bucket(key)&511;
            d.records[d.n]={key|(value<<24),static_cast<unsigned short>(i+1),d.tail[h]};
            d.tail[h]=static_cast<unsigned short>(++d.n);
        }
    }
}
struct RecallCase {Context c;Word target,until;};
struct Shutter {Byte field,pin,value;};
struct Evidence {
    RecallCase cases[episodes];
    Byte pins[3*width];
    Shutter gates[64];
    Word n,pins_n,gates_n,winner;
    Fibre candidates[128];
};
AX_FN void precedents(const Mind &m,const Detections &d,const Construction &q,Word channel,Evidence &w) {
    w.n=w.pins_n=w.gates_n=0;
    for (Word j=0;j<m.lessons.n;++j) {
        auto &e=m.lessons.event[j];if (e.response.size!=q.length || !matches(q.cue,e.stimulus)) continue;
        Context c{e.stimulus,{},{}};
        if (d.last[j]) {auto &old=m.lessons.event[d.last[j]-1];c.prior=old.stimulus;c.spoken=old.response;}
        w.cases[w.n++]={c,e.response.words[channel],j};
    }
    for (Word pin=0;pin<(m.genes.high&32 ? 3*width : width);++pin) {
        Word first=select(w.cases[0].c,pin);bool varied=false,present=bool(first);
        for (Word i=1;i<w.n && present;++i) {Word x=select(w.cases[i].c,pin);present=x!=0;varied|=x!=first;}
        if (varied && present) w.pins[w.pins_n++]=static_cast<Byte>(pin);
    }
    if (!(m.genes.high&8)) return;
    for (Word j=0;j<m.anatomy.fields_n && w.gates_n<64;++j) {
        if (m.anatomy.fields[j].right!=absent) continue;
        for (Word k=0;k<w.pins_n && w.gates_n<64;++k) {
            Word pin=w.pins[k],values[4]{},counts[4]{},different=0;
            for (Word i=0;i<w.n && different<4;++i) {
                auto &sample=w.cases[i];Word key=select(sample.c,pin);
                Word value=Replay{d}.get(j+1,key,0,sample.until).value;
                Word l=0;while (l<different && values[l]!=value) ++l;
                if (l==different) values[different++]=value;
                ++counts[l];
            }
            if (different<2 || different>3) continue;
            for (Word l=0;l<different && w.gates_n<64;++l) if (counts[l]>=2 && values[l])
                w.gates[w.gates_n++]={static_cast<Byte>(j+1),static_cast<Byte>(pin),static_cast<Byte>(values[l])};
        }
    }
}
AX_FN bool explains(const Anatomy &a,const Detections &d,const Fibre &f,const Evidence &w) {
    for (Word k=0;k<w.n;++k) {
        Word depth=0;auto &e=w.cases[k];
        if (project(a,Replay{d},f,e.c,e.until,depth)!=e.target) return false;
    }
    return true;
}
AX_FN void induce(Mind &m,const Detections &d,Word group,Word channel,Evidence &w,Word lane) {
    auto &a=m.anatomy;auto &q=a.phrases[group];
    if (!lane) {precedents(m,d,q,channel,w);w.winner=~0u;}
    together();
    if (lane<w.pins_n && (m.genes.high&64)) {
        Fibre f;f.source=w.pins[lane];if (explains(a,d,f,w)) {w.candidates[lane]=f;shrink(w.winner,lane);}
    }
    if (lane==127 && (m.genes.high&128)) {
        Fibre f;f.literal=static_cast<Byte>(w.cases[0].target);if (explains(a,d,f,w)) {w.candidates[lane]=f;shrink(w.winner,lane);}
    }
    together();
    if (w.winner!=~0u) {if (!lane) q.voice[channel]=w.candidates[w.winner];return;}
    if (!w.pins_n || !(m.genes.high&1)) return;
    for (Word stage=0;stage<8;++stage) {
        bool repeat=stage&1,two=stage&2,gated=stage&4;
        if ((repeat && !(m.genes.high&4)) || (two && !(m.genes.high&2)) || (gated && !w.gates_n)) continue;
        Word total=w.pins_n*a.fields_n*(two ? a.fields_n : 1)*(gated ? w.gates_n : 1);
        Word best=~0u;Fibre chosen;
        for (Word id=lane;id<total;id+=128) {
            Word j=id;Fibre f;f.source=w.pins[j%w.pins_n];j/=w.pins_n;
            f.first=static_cast<Byte>(1+j%a.fields_n);j/=a.fields_n;
            if (two) {f.second=static_cast<Byte>(1+j%a.fields_n);j/=a.fields_n;}
            if (gated) {auto g=w.gates[j];f.gate=g.field;f.gate_source=g.pin;f.condition=g.value;}
            f.repeat=repeat;
            bool joint=a.fields[f.first-1].right!=absent || (two && a.fields[f.second-1].right!=absent);
            for (Word k=0;k<(joint ? w.pins_n : 1);++k) {
                if (joint) {f.other=w.pins[k];if (f.other==f.source) continue;}
                if (explains(a,d,f,w)) {best=id*32+k;chosen=f;break;}
            }
            if (best!=~0u) break;
        }
        if (best!=~0u) {w.candidates[lane]=chosen;shrink(w.winner,best);}
        together();
        if (w.winner!=~0u) {if (!lane) q.voice[channel]=w.candidates[(w.winner/32)%128];return;}
    }
}
AX_FN void settle(Mind &m) {
    auto &p=m.anatomy;
    for (Word j=0;j<p.phrases_n;++j) {
        auto &q=p.phrases[j];q.ready=1;
        for (Word k=0;k<q.length;++k) if (!q.voice[k].literal && q.voice[k].source==absent) q.ready=0;
    }
    wiring(p);m.consolidated=m.lessons.seen;
    m.memory={};m.pending={};++m.revisions;
    for (Word j=0;j<m.lessons.n;++j) {
        auto &sample=m.lessons.event[j];Word handled=0,depth=0;
        act(p,m.memory,m.genes,sample.stimulus,handled,depth);
    }
}
}
